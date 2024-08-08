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
#include <init.h>
#include <miiphy.h>
#include <usb.h>
#include <stdlib.h>
#include <extension_board.h>

#include "../common/imx8m_som_detection.h"

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_ADDR	     0x51
#define EEPROM_ADDR_FALLBACK    0x59

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
	int ret = phytec_eeprom_data_setup_fallback(NULL, 0,
			EEPROM_ADDR, EEPROM_ADDR_FALLBACK);
	if (ret)
		printf("%s: EEPROM data init failed\n", __func__);

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

void phytec_set_fit_extensions(void)
{
	char fit_extensions[128] = {};

	if (!phytec_get_imx8m_eth(NULL))
		sprintf(fit_extensions, "%s%s", fit_extensions, "#conf-imx8mp-phycore-no-eth.dtbo");

	if (!phytec_get_imx8m_spi(NULL))
		sprintf(fit_extensions, "%s%s", fit_extensions, "#conf-imx8mp-phycore-no-spiflash.dtbo");

	if (!phytec_get_imx8mp_rtc(NULL))
		sprintf(fit_extensions, "%s%s", fit_extensions, "#conf-imx8mp-phycore-no-rtc.dtbo");

	env_set("fit_extensions", fit_extensions);
}

#if (IS_ENABLED(CONFIG_CMD_EXTENSION))
int extension_board_scan(struct list_head *extension_list)
{
	struct extension *extension = NULL;
	int cnt = 0;

	if (!phytec_get_imx8m_eth(NULL)) {
		extension = phytec_add_extension("phyCORE-i.MX8MP no eth phy",
						 "imx8mp-phycore-no-eth.dtbo",
						 "eth phy not populated on SoM");
		list_add_tail(&extension->list, extension_list);
		cnt++;
	}

	if (!phytec_get_imx8m_spi(NULL)) {
		extension = phytec_add_extension("phyCORE-i.MX8MP no SPI flash",
						 "imx8mp-phycore-no-spiflash.dtbo",
						 "SPI flash not populated on SoM");
		list_add_tail(&extension->list, extension_list);
		cnt++;
	}

	if (!phytec_get_imx8mp_rtc(NULL)) {
		extension = phytec_add_extension("phyCORE-i.MX8MP no RTC",
						 "imx8mp-phycore-no-rtc.dtbo",
						 "RTC not populated on SoM");
		list_add_tail(&extension->list, extension_list);
		cnt++;
	}

	return cnt;
}
#endif

int board_late_init(void)
{
	switch (get_boot_device()) {
	case SD2_BOOT:
		env_set_ulong("mmcdev", 1);
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
	phytec_set_fit_extensions();

	return 0;
}

int board_phys_sdram_size(phys_size_t *size)
{
	if (!size)
		return -EINVAL;

	*size = get_ram_size((void *)PHYS_SDRAM, PHYS_SDRAM_SIZE + PHYS_SDRAM_2_SIZE);

	return 0;
}
