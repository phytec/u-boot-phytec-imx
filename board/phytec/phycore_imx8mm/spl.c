// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2019-2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#include <common.h>
#include <asm/arch/clock.h>
#include <asm/arch/ddr.h>
#include <asm/arch/imx8mm_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/iomux-v3.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <hang.h>
#include <init.h>
#include <log.h>
#include <spl.h>

#include "../common/imx8m_som_detection.h"

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_ADDR		0x51
#define EEPROM_ADDR_FALLBACK	0x59

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	switch (boot_dev_spl) {
	case SD2_BOOT:
	case MMC2_BOOT:
		return BOOT_DEVICE_MMC1;
	case SD3_BOOT:
	case MMC3_BOOT:
		return BOOT_DEVICE_MMC2;
	case QSPI_BOOT:
		return BOOT_DEVICE_NOR;
	case USB_BOOT:
		return BOOT_DEVICE_BOARD;
	default:
		return BOOT_DEVICE_NONE;
	}
}

static void spl_dram_init(void)
{
	int ret;

	ret = phytec_eeprom_data_setup_fallback(NULL, 0, EEPROM_ADDR,
						EEPROM_ADDR_FALLBACK);
	if (ret)
		goto out;

	ret = phytec_imx8m_detect(NULL);
	if (!ret)
		phytec_print_som_info(NULL);

	u8 size = phytec_get_imx8m_ddr_size(NULL);
	switch (size) {
		case 1:
			/* 1GB RAM */
			dram_timing.ddrc_cfg[5].val = 0x2d0087;
			dram_timing.ddrc_cfg[21].val = 0x8d;
			dram_timing.ddrc_cfg[42].val = 0xf070707;
			dram_timing.ddrc_cfg[58].val = 0x60012;
			dram_timing.ddrc_cfg[73].val = 0x13;
			dram_timing.ddrc_cfg[83].val = 0x30005;
			dram_timing.ddrc_cfg[98].val = 0x5;
			break;
		case 3:
			/* 2GB RAM */
			break;
		case 5:
			/* 4GB RAM */
			dram_timing.ddrc_cfg[2].val = 0xa3080020;
			dram_timing.ddrc_cfg[37].val = 0x17;
			dram_timing.fsp_msg[0].fsp_cfg[8].val = 0x310;
			dram_timing.fsp_msg[0].fsp_cfg[20].val = 0x3;
			dram_timing.fsp_msg[1].fsp_cfg[9].val = 0x310;
			dram_timing.fsp_msg[1].fsp_cfg[21].val = 0x3;
			dram_timing.fsp_msg[2].fsp_cfg[9].val = 0x310;
			dram_timing.fsp_msg[2].fsp_cfg[21].val = 0x3;
			dram_timing.fsp_msg[3].fsp_cfg[10].val = 0x310;
			dram_timing.fsp_msg[3].fsp_cfg[22].val = 0x3;
			break;
		default:
			goto out;
	}
	ddr_init(&dram_timing);
	return;
out:
	puts("Could not detect correct RAM size. Fall back to default.");
	ddr_init(&dram_timing);
}

void spl_board_init(void)
{
	/* Serial download mode */
	if (is_usb_boot()) {
		puts("Back to ROM, SDP\n");
		restore_boot_params();
	}
	puts("Normal Boot\n");
}

int board_fit_config_name_match(const char *name)
{
	return 0;
}

#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE)

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MM_PAD_GPIO1_IO02_WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

int board_early_init_f(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	return 0;
}

void board_init_f(ulong dummy)
{
	struct udevice *dev;
	int ret;

	arch_cpu_init();

	init_uart_clk(2);

	board_early_init_f();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	ret = spl_early_init();
	if (ret) {
		debug("spl_early_init() failed: %d\n", ret);
		hang();
	}

	preloader_console_init();

	enable_tzc380();

	/* DDR initialization */
	spl_dram_init();

	board_init_r(NULL, 0);
}
