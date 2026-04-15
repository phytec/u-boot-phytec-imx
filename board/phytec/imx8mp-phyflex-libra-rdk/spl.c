// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 PHYTEC Messtechnik GmbH
 */

#include <config.h>
#include <asm/arch/clock.h>
#include <asm/arch/ddr.h>
#include <asm/arch/imx8mp_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/mach-imx/boot_mode.h>
#include <hang.h>
#include <init.h>
#include <log.h>
#include <power/pmic.h>
#include <power/pca9450.h>
#include <spl.h>

#include "../common/imx8m_som_detection.h"
#include "lpddr4_timing.h"

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_ADDR		0x51

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	return BOOT_DEVICE_BOOTROM;
}

void spl_dram_init(void)
{
	enum phytec_phyflex_imx8mp_ddr_code size = PHYTEC_EEPROM_INVAL;
	int ret;

	ret = phytec_eeprom_data_setup(NULL, CONFIG_PHYTEC_EEPROM_BUS, EEPROM_ADDR);
	if (ret)
		goto out;

	ret = phytec_imx8m_detect(NULL);
	if (!ret)
		phytec_print_som_info(NULL);

	size = phytec_get_imx8m_ddr_size(NULL);
	switch (size) {
	case PHYTEC_PHYFLEX_IMX8MP_DDR_2GB:
		break;
	case PHYTEC_PHYFLEX_IMX8MP_DDR_4GB:
		set_dram_timings_4gb();
		break;
	default:
		goto out;
	}
	ddr_init(&dram_timing);
	return;
out:
	printf("Could not detect correct RAM size. Fallback to default.\n");
	ddr_init(&dram_timing);
}

int power_init_board(void)
{
	struct udevice *dev;
	int ret;

	ret = pmic_get("pmic@25", &dev);
	if (ret == -ENODEV) {
		puts("No pmic@25\n");
		return 0;
	}
	if (ret < 0)
		return ret;

	/* BUCKxOUT_DVS0/1 control BUCK123 output */
	pmic_reg_write(dev, PCA9450_BUCK123_DVS, 0x29);

	/* Increase VDD_SOC and VDD_ARM to OD voltage 0.95V */
	pmic_reg_write(dev, PCA9450_BUCK1OUT_DVS0, 0x1C);
	pmic_reg_write(dev, PCA9450_BUCK2OUT_DVS0, 0x1C);

	/* Set BUCK1 DVS1 to suspend controlled through PMIC_STBY_REQ */
	pmic_reg_write(dev, PCA9450_BUCK1OUT_DVS1, 0x14);
	pmic_reg_write(dev, PCA9450_BUCK1CTRL, 0x59);

	/* Set WDOG_B_CFG to cold reset */
	pmic_reg_write(dev, PCA9450_RESET_CTRL, 0xA1);

	return 0;
}

void spl_board_init(void)
{
	arch_misc_init();

	/* Set GIC clock to 500Mhz for OD VDD_SOC. */
	clock_enable(CCGR_GIC, 0);
	clock_set_target_val(GIC_CLK_ROOT, CLK_ROOT_ON | CLK_ROOT_SOURCE_SEL(5));
	clock_enable(CCGR_GIC, 1);
}

int board_fit_config_name_match(const char *name)
{
	return 0;
}

void board_init_f(ulong dummy)
{
	int ret;

	arch_cpu_init();

	init_uart_clk(3);

	ret = spl_early_init();
	if (ret) {
		debug("spl_early_init() failed: %d\n", ret);
		hang();
	}

	preloader_console_init();

	enable_tzc380();

	power_init_board();

	/* DDR initialization */
	spl_dram_init();
}
