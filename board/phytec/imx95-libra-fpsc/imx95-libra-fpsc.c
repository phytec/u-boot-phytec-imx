// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 PHYTEC Messtechnik GmbH
 */
#include <env.h>
#include <init.h>
#include <asm/global_data.h>
#include <asm/arch-imx9/ccm_regs.h>
#include <asm/arch/clock.h>
#include <fdt_support.h>
#include <usb.h>
#include <dwc3-uboot.h>
#include <asm/io.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <i2c.h>
#include <miiphy.h>
#include <asm/gpio.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>
#include <scmi_agent.h>
#include <scmi_protocols.h>
#include <dt-bindings/clock/fsl,imx95-clock.h>
#include <dt-bindings/power/fsl,imx95-power.h>

DECLARE_GLOBAL_DATA_PTR;

int board_early_init_f(void)
{
	/* UART7: A55, UART2: M33 */
	init_uart_clk(6);

	return 0;
}

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

int dm_usb_gadget_handle_interrupts(struct udevice *dev)
{
	dwc3_uboot_handle_interrupt(dev);
	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	if (index == 0 && init == USB_INIT_DEVICE)
		return dwc3_uboot_init(&dwc3_device_data);

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 0 && init == USB_INIT_DEVICE)
		dwc3_uboot_exit(index);

	return ret;
}

#define TUSB_PORT_POL_CRTL_REG	0xB
#define TUSB_CUSTOM_POL		BIT(7)
#define TUSB_P0_POL		BIT(0)

/*
 * WORKAROUND for PCM-937-L 1618.0, 1618.1.
 * USB HUB TUSB8042A has swapped upstream pin polarity.
 * Set i2c registers to inform the hub that the lines
 * are swapped.
 *
 * We also notice that the HUB i2c address might not be
 * as expected for unknown reasons. Test all 4 possible i2c
 * addresses to write to the device.
 *
 */
void tusb8042a_swap_lines(void)
{
	const u8 pol_swap_val = (TUSB_CUSTOM_POL | TUSB_P0_POL);
	const int addr[4] = {0x44, 0x45, 0x46, 0x47};
	struct udevice *dev;
	int i, ret;

	for (i = 0; i < 4; i++) {
		ret = i2c_get_chip_for_busnum(2, addr[i], 1, &dev);
		if (!ret) {
			dm_i2c_write(dev, TUSB_PORT_POL_CRTL_REG, &pol_swap_val, 1);
			break;
		};
	}

	if (ret)
		printf("TUSB8042A: Failed to fixup USB HUB.\n");
}

static int imx9_scmi_power_domain_enable(u32 domain, bool enable)
{
	return scmi_pwd_state_set(gd->arch.scmi_dev, 0, domain, enable ? 0 : BIT(30));
}

void netc_init(void)
{
	int ret;

	/* Power up the NETC MIX. */
	ret = imx9_scmi_power_domain_enable(IMX95_PD_NETC, true);
	if (ret) {
		printf("SCMI_POWWER_STATE_SET Failed for NETC MIX\n");
		return;
	}

	set_clk_netc(ENET_125MHZ);
	pci_init();
}

int board_init(void)
{
	tusb8042a_swap_lines();

	netc_init();

	return 0;
}

int board_late_init(void)
{
	pr_err("board_late_init get_boot_device\n");
	switch (get_boot_device()) {
	case SD2_BOOT:
		env_set_ulong("mmcdev", 1);
		if (!strcmp(env_get("boot_targets"), env_get_default("boot_targets")))
			env_set("boot_targets", "mmc1 mmc0 ethernet");
		break;
	case MMC1_BOOT:
		env_set_ulong("mmcdev", 0);
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

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, struct bd_info *bd)
{
	return 0;
}
#endif

int board_phys_sdram_size(phys_size_t *size)
{
	*size = PHYS_SDRAM_SIZE + PHYS_SDRAM_2_SIZE;

	return 0;
}
