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
#include <hang.h>
#include <init.h>
#include <log.h>
#include <spl.h>

#include "../common/imx8m_som_detection.h"

extern struct dram_timing_info dram_timing_1gb;
extern struct dram_timing_info dram_timing_4gb;

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

	ret = phytec_eeprom_data_init(0, EEPROM_ADDR);
	if (ret) {
		printf("phytec_eeprom_data_init: init failed. "
		       "Trying fall back address 0x%x\n", EEPROM_ADDR_FALLBACK);
		ret = phytec_eeprom_data_init(0,
					      EEPROM_ADDR_FALLBACK);
		if (ret)
			goto err;
	}

	printf("phytec_eeprom_data_init: init successful\n");

	phytec_print_som_info();

	switch (phytec_get_imx8m_ddr_size()) {
	case 1:
		dram_timing_1gb.ddrphy_cfg[82].val = 0x3;
		dram_timing_1gb.ddrphy_cfg[83].val = 0x3;
		ddr_init(&dram_timing_1gb);
		break;
	case 3:
		ddr_init(&dram_timing);
		break;
	case 5:
		dram_timing_4gb.ddrphy_cfg[82].val = 0x3;
		dram_timing_4gb.ddrphy_cfg[83].val = 0x3;
		ddr_init(&dram_timing_4gb);
		break;
	default:
		goto err;
	}

	return;
err:
	printf("Could not detect correct RAM size. Fallback to default.\n");
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

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE)

static iomux_v3_cfg_t const uart_pads[] = {
	IMX8MM_PAD_UART3_RXD_UART3_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART3_TXD_UART3_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MM_PAD_GPIO1_IO02_WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

int board_early_init_f(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	return 0;
}

void board_init_f(ulong dummy)
{
	int ret;

	arch_cpu_init();

	init_uart_clk(2);

	board_early_init_f();

	preloader_console_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	ret = spl_early_init();
	if (ret) {
		debug("spl_early_init() failed: %d\n", ret);
		hang();
	}

	enable_tzc380();

	/* DDR initialization */
	spl_dram_init();

	board_init_r(NULL, 0);
}
