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

void set_dram_timings_1gb(void);
void set_dram_timings_4gb(void);

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

enum phytec_imx8mm_ddr_eeprom_code {
	INVALID = PHYTEC_EEPROM_INVAL,
	PHYTEC_IMX8MM_DDR_1GB = 1,
	PHYTEC_IMX8MM_DDR_2GB = 3,
	PHYTEC_IMX8MM_DDR_4GB = 5,
};

static void spl_dram_init(void)
{
	int ret;
	enum phytec_imx8mm_ddr_eeprom_code size = INVALID;

	ret = phytec_eeprom_data_setup_fallback(NULL, 0, EEPROM_ADDR,
						EEPROM_ADDR_FALLBACK);
	if (ret && !IS_ENABLED(CONFIG_PHYCORE_IMX8MM_RAM_SIZE_FIX))
		goto out;

	ret = phytec_imx8m_detect(NULL);
	if (!ret)
		phytec_print_som_info(NULL);

	if (IS_ENABLED(CONFIG_PHYCORE_IMX8MM_RAM_SIZE_FIX)) {
		if (IS_ENABLED(CONFIG_PHYCORE_IMX8MM_RAM_SIZE_1GB))
			size = PHYTEC_IMX8MM_DDR_1GB;
		else if (IS_ENABLED(CONFIG_PHYCORE_IMX8MM_RAM_SIZE_2GB))
			size = PHYTEC_IMX8MM_DDR_2GB;
		else if (IS_ENABLED(CONFIG_PHYCORE_IMX8MM_RAM_SIZE_4GB))
			size = PHYTEC_IMX8MM_DDR_4GB;
	} else {
		size = phytec_get_imx8m_ddr_size(NULL);
	}

	switch (size) {
	case PHYTEC_IMX8MM_DDR_1GB:
		set_dram_timings_1gb();
		break;
	case PHYTEC_IMX8MM_DDR_2GB:
		break;
	case PHYTEC_IMX8MM_DDR_4GB:
		set_dram_timings_4gb();
		break;
	default:
		goto out;
	}
	ddr_init(&dram_timing);
	return;
out:
	puts("Could not detect correct RAM size. Fall back to default.\n");
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
