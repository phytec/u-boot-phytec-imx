// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (C) 2026 PHYTEC Messtechnik GmbH
 * Author: Christoph Stoidner <c.stoidner@phytec.de>
 */

#include <asm/arch/clock.h>
#include <asm/arch/ddr.h>
#include <asm/arch/mu.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/trdc.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/ele_api.h>
#include <asm/sections.h>
#include <hang.h>
#include <init.h>
#include <log.h>
#include <power/pmic.h>
#include <power/pca9450.h>
#include <spl.h>

#include "../common/imx91_93_som_detection.h"

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_ADDR            0x50

void set_dram_timings_2gb_lpddr4x(void);

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	return BOOT_DEVICE_BOOTROM;
}

void spl_board_init(void)
{
	int ret;

	ret = ele_start_rng();
	if (ret)
		printf("Fail to start RNG: %d\n", ret);

	puts("Normal Boot\n");
}

void spl_dram_init(void)
{
	int ret;
	enum phytec_imx91_93_ddr_eeprom_code ddr_opt = PHYTEC_IMX91_93_DDR_INVALID;

	ret = phytec_eeprom_data_setup(NULL, 1, EEPROM_ADDR);
	if (ret && !IS_ENABLED(CONFIG_PHYFLEX_IMX91_93_RAM_STATIC))
		goto out;

	ret = phytec_imx91_93_detect(NULL, PHYFLEX_IMX91_93_SOM);
	if (!ret)
		phytec_print_som_info(NULL);

	if (IS_ENABLED(CONFIG_PHYFLEX_IMX91_93_RAM_STATIC)) {
		if (IS_ENABLED(CONFIG_PHYFLEX_IMX91_93_RAM_LPDDR4X_1GB))
			ddr_opt = PHYTEC_IMX91_93_LPDDR4X_1GB;
		else if (IS_ENABLED(CONFIG_PHYFLEX_IMX91_93_RAM_LPDDR4X_2GB))
			ddr_opt = PHYTEC_IMX91_93_LPDDR4X_2GB;
	} else {
		ddr_opt = phytec_imx91_93_phyflex_get_opt(NULL, PHYTEC_IMX91_93_PHYFLEX_OPT_DDR);
	}

	switch (ddr_opt) {
#if defined(CONFIG_IMX91)
	case PHYTEC_IMX91_93_LPDDR4_1GB:
		/* nothing to do, right dram timings are statically set for 1GB */
		break;
#endif
#if defined(CONFIG_IMX93)
	case PHYTEC_IMX91_93_LPDDR4X_1GB:
		/* nothing to do, right dram timings are statically set for 1GB */
		break;
	case PHYTEC_IMX91_93_LPDDR4X_2GB:
		set_dram_timings_2gb_lpddr4x();
		break;
#endif
	default:
		goto out;
	}
	ddr_init(&dram_timing);
	return;
out:
	puts("Could not detect correct RAM type and size. Fall back to default.\n");
	ddr_init(&dram_timing);
}

int power_init_board(void)
{
	struct udevice *dev;
	int ret;
	unsigned int val = 0, buck_val;

	ret = pmic_get("pmic@25", &dev);
	if (ret == -ENODEV) {
		puts("No pca9450@25\n");
		return 0;
	}

	if (ret != 0)
		return ret;

	/* BUCKxOUT_DVS0/1 control BUCK123 output */
	pmic_reg_write(dev, PCA9450_BUCK123_DVS, 0x29);

	/* enable DVS control through PMIC_STBY_REQ */
	pmic_reg_write(dev, PCA9450_BUCK1CTRL, 0x59);

	ret = pmic_reg_read(dev, PCA9450_PWR_CTRL);
	if (ret < 0)
		return ret;
	val = ret;

	if (is_voltage_mode(VOLT_LOW_DRIVE)) {
		buck_val = 0x0c; /* 0.8v for Low drive mode */
		printf("PMIC: Low Drive Voltage Mode\n");
	} else if (is_voltage_mode(VOLT_NOMINAL_DRIVE)) {
		buck_val = 0x10; /* 0.85v for Nominal drive mode */
		printf("PMIC: Nominal Voltage Mode\n");
	} else {
		buck_val = 0x14; /* 0.9v for Over drive mode */
		printf("PMIC: Over Drive Voltage Mode\n");
	}

	if (val & PCA9450_REG_PWRCTRL_TOFF_DEB) {
		pmic_reg_write(dev, PCA9450_BUCK1OUT_DVS0, buck_val);
		pmic_reg_write(dev, PCA9450_BUCK3OUT_DVS0, buck_val);
	} else {
		pmic_reg_write(dev, PCA9450_BUCK1OUT_DVS0, buck_val + 0x4);
		pmic_reg_write(dev, PCA9450_BUCK3OUT_DVS0, buck_val + 0x4);
	}

	/* set standby voltage to 0.65v */
	if (val & PCA9450_REG_PWRCTRL_TOFF_DEB)
		pmic_reg_write(dev, PCA9450_BUCK1OUT_DVS1, 0x0);
	else
		pmic_reg_write(dev, PCA9450_BUCK1OUT_DVS1, 0x4);

	/* I2C_LT_EN*/
	pmic_reg_write(dev, 0xa, 0x3);

	return 0;
}

void board_init_f(ulong dummy)
{
	int ret;
	unsigned char mac[6];

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	timer_init();

	arch_cpu_init();

	spl_early_init();

	preloader_console_init();

	ret = imx9_probe_mu();
	if (ret) {
		printf("Fail to init ELE API\n");
	} else {
		printf("SOC: 0x%x\n", gd->arch.soc_rev);
		printf("LC: 0x%x\n", gd->arch.lifecycle);
	}

	clock_init_late();

	power_init_board();

	if (!is_voltage_mode(VOLT_LOW_DRIVE))
		set_arm_core_max_clk();

	/* Init power of mix */
	soc_power_init();

	/* Setup TRDC for DDR access */
	trdc_init();

	/* DDR initialization */
	spl_dram_init();

	/* print MAC address of ethernet 0, this is our serial number */
	imx_get_mac_from_fuse(0, mac);
	printf("MAC-Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

#if defined(CONFIG_IMX93)
	/* Put M33 into CPUWAIT for following kick */
	ret = m33_prepare();
	if (!ret)
		printf("M33 prepare ok\n");
#endif

	board_init_r(NULL, 0);
}
