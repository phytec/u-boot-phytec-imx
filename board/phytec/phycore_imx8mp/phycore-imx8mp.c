// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#include <common.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/mach-imx/boot_mode.h>
#include <dwc3-uboot.h>
#include <env.h>
#include <fdt_support.h>
#include <jffs2/load_kernel.h>
#include <miiphy.h>
#include <mtd_node.h>
#include <usb.h>

#include "../common/imx8m_som_detection.h"

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_ADDR		0x51
#define EEPROM_ADDR_FALLBACK	0x59

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

int ft_board_setup(void *blob,struct bd_info *bd)
{
	u8 spi = phytec_get_imx8m_spi();
	/* Do nothing if no SPI is poulated or data invalid */
	if (spi == 0 || spi == PHYTEC_EEPROM_INVAL)
		return 0;

	static const struct node_info nodes[] = {
		{ "jedec,spi-nor", MTD_DEV_TYPE_NOR, },
	};

	fdt_fixup_mtdparts(blob, nodes, ARRAY_SIZE(nodes));

	return 0;
}

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Enable RGMII TX clk output */
	setbits_le32(&gpr->gpr[1],BIT(22));

	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	imx8m_usb_power(index, true);

	if (index == 0 && init == USB_INIT_DEVICE) {
		return dwc3_uboot_init(&dwc3_device_data);
	}

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	if (index == 0 && init == USB_INIT_DEVICE)
		dwc3_uboot_exit(index);

	imx8m_usb_power(index, false);

	return 0;
}

int usb_gadget_handle_interrupts(int index)
{
	dwc3_uboot_handle_interrupt(index);
	return 0;
}

int board_init(void)
{
	phytec_eeprom_data_setup(0, EEPROM_ADDR, EEPROM_ADDR_FALLBACK);

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
	u8 spi = phytec_get_imx8m_spi();

	if (spi != 0 && spi != PHYTEC_EEPROM_INVAL)
		env_set("spiprobe", "sf probe");

	switch (get_boot_device()) {
	case SD2_BOOT:
		env_set_ulong("mmcdev", 1);
		break;
	case MMC3_BOOT:
		env_set_ulong("mmcdev", 2);
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

	*size = imx8m_ddrc_sdram_size();

	return 0;
}
