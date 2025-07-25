// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 PHYTEC Messtechnik GmbH
 */

#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <linux/io.h>
#include <asm/mach-imx/boot_mode.h>
#include <dwc3-uboot.h>
#include <env.h>
#include <init.h>
#include <i2c.h>
#include <fdt_support.h>
#include <jffs2/load_kernel.h>
#include <miiphy.h>
#include <mtd_node.h>
#include <usb.h>

#if IS_ENABLED(CONFIG_PHYTEC_SOM_DETECTION)
#include "../common/imx8m_som_detection.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_ADDR		0x51

static struct dwc3_device dwc3_device_data = {
#ifdef CONFIG_SPL_BUILD
	.maximum_speed = USB_SPEED_HIGH,
#else
	.maximum_speed = USB_SPEED_SUPER,
#endif
	.base = USB1_BASE_ADDR,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
	.power_down_scale = 2,
};

int board_usb_init(int index, enum usb_init_type init)
{
	if (index == 0 && init == USB_INIT_DEVICE) {
		imx8m_usb_power(index, true);
		return dwc3_uboot_init(&dwc3_device_data);
	}
	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	if (index == 0 && init == USB_INIT_DEVICE) {
		dwc3_uboot_exit(index);
		imx8m_usb_power(index, false);
	}

	return 0;
}

int dm_usb_gadget_handle_interrupts(struct udevice *dev)
{
	dwc3_uboot_handle_interrupt(dev);
	return 0;
}

#define TUSB_PORT_POL_CRTL_REG	0xB
#define TUSB_CUSTOM_POL		BIT(7)
#define TUSB_P0_POL		BIT(0)

/*
 * WORKAROUND for PCM-937-L 1618.0, 1618.1.
 * USB HUB TUSB8042A has swapped upstream pin polarity.
 * Set i2c registers to inform the hub that the lines
 * are swapped.
 */
void tusb8042a_swap_lines(void)
{
	const u8 pol_swap_val = (TUSB_CUSTOM_POL | TUSB_P0_POL);
	const int addr = 0x44;
	struct udevice *dev = 0;

	int ret = i2c_get_chip_for_busnum(2, addr, 1, &dev);
	if (!ret)
		dm_i2c_write(dev, TUSB_PORT_POL_CRTL_REG, &pol_swap_val, 1);
	else
		printf("TUSB8042A: Failed to fixup USB HUB.\n");
}

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&gpr->gpr[1], 0x2000, 0);

	return 0;
}

int board_init(void)
{
#if IS_ENABLED(CONFIG_PHYTEC_SOM_DETECTION)
	int ret = phytec_eeprom_data_setup(NULL, 0, EEPROM_ADDR);

	if (ret)
		printf("%s: EEPROM data init failed\n", __func__);
#endif

	tusb8042a_swap_lines();

	setup_fec();

	init_usb_clk();

	return 0;
}

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no;
}

int board_late_init(void)
{
	switch (get_boot_device()) {
	case SD2_BOOT:
		env_set_ulong("mmcdev", 1);
		if (!strcmp(env_get("boot_targets"), env_get_default("boot_targets")))
			env_set("boot_targets", "mmc1 mmc2 ethernet");
		break;
	case MMC3_BOOT:
		env_set_ulong("mmcdev", 2);
		break;
	case USB_BOOT:
		printf("Detect USB boot. Will enter fastboot mode!\n");
		env_set_ulong("dofastboot", 1);
		break;
	default:
		break;
	}

	return 0;
}

int board_phys_sdram_size(phys_size_t *size)
{
	if (!size)
		return -EINVAL;

	*size = get_ram_size((void *)PHYS_SDRAM, PHYS_SDRAM_SIZE + PHYS_SDRAM_2_SIZE);

	return 0;
}
