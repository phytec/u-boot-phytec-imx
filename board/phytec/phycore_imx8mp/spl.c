// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#include <common.h>
#include <asm/arch/clock.h>
#include <asm/arch/ddr.h>
#include <asm/arch/imx8mp_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/mach-imx/iomux-v3.h>
#include <hang.h>
#include <init.h>
#include <log.h>
#include <power/pmic.h>
#include <power/pca9450.h>
#include <spl.h>

#include "../common/imx8m_som_detection.h"

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_ADDR		0x51
#define EEPROM_ADDR_FALLBACK	0x59

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	return BOOT_DEVICE_BOOTROM;
}

void spl_dram_init(void)
{
	int ret;

	ret = phytec_eeprom_data_init(0, EEPROM_ADDR);
	if (ret) {
		printf("phytec_eeprom_data_init: init failed. "
		       "Trying fall back address 0x%x\n", EEPROM_ADDR_FALLBACK);
		ret = phytec_eeprom_data_init(0, EEPROM_ADDR_FALLBACK);

		if (ret)
			goto err;
	}

	printf("phytec_eeprom_data_init: init successful\n");

	phytec_print_som_info();

	switch (phytec_get_imx8m_ddr_size()) {
	case 2:
		/* 1GB */
		dram_timing.ddrc_cfg[3].val = 0x1233;
		dram_timing.ddrc_cfg[5].val = 0x5b0087;
		dram_timing.ddrc_cfg[6].val = 0x61027f10;
		dram_timing.ddrc_cfg[7].val = 0x7b0;
		dram_timing.ddrc_cfg[11].val = 0xf30000;
		dram_timing.ddrc_cfg[23].val = 0x8d;
		dram_timing.ddrc_cfg[44].val = 0xf070707;
		dram_timing.ddrc_cfg[58].val = 0x1031;
		dram_timing.ddrc_cfg[61].val = 0xc0012;
		dram_timing.ddrc_cfg[76].val = 0x13;
		dram_timing.ddrc_cfg[83].val = 0x1031;
		dram_timing.ddrc_cfg[86].val = 0x30005;
		dram_timing.ddrc_cfg[101].val = 0x5;
		dram_timing.ddrphy_cfg[75].val = 0x1e3;
		dram_timing.ddrphy_cfg[77].val = 0x1e3;
		dram_timing.ddrphy_cfg[79].val = 0x1e3;
		dram_timing.fsp_msg[0].fsp_cfg[11].val = 0xf3;
		dram_timing.fsp_msg[0].fsp_cfg[16].val = 0xf3;
		dram_timing.fsp_msg[0].fsp_cfg[23].val = 0xf32d;
		dram_timing.fsp_msg[0].fsp_cfg[29].val = 0xf32d;
		dram_timing.fsp_msg[3].fsp_cfg[12].val = 0xf3;
		dram_timing.fsp_msg[3].fsp_cfg[17].val = 0xf3;
		dram_timing.fsp_msg[3].fsp_cfg[24].val = 0xf32d;
		dram_timing.fsp_msg[3].fsp_cfg[30].val = 0xf32d;
		ddr_init(&dram_timing);
		break;
	case 3:
		/* 2GB */
		ddr_init(&dram_timing);
		break;
	case 5:
		/* 4GB */
		dram_timing.ddrc_cfg[2].val = 0xa3080020;
		dram_timing.ddrc_cfg[39].val = 0x17;
		dram_timing.fsp_msg[0].fsp_cfg[9].val = 0x310;
		dram_timing.fsp_msg[0].fsp_cfg[21].val = 0x3;
		dram_timing.fsp_msg[1].fsp_cfg[10].val = 0x310;
		dram_timing.fsp_msg[1].fsp_cfg[22].val = 0x3;
		dram_timing.fsp_msg[2].fsp_cfg[10].val = 0x310;
		dram_timing.fsp_msg[2].fsp_cfg[22].val = 0x3;
		dram_timing.fsp_msg[3].fsp_cfg[10].val = 0x310;
		dram_timing.fsp_msg[3].fsp_cfg[22].val = 0x3;
		ddr_init(&dram_timing);
		break;
	default:
		goto err;
	}

	return;

err:
	printf("Could not detect correct RAM size. Fallback to default.\n");
	ddr_init(&dram_timing);
}

#define I2C_PAD_CTRL (PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PE)
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = MX8MP_PAD_I2C1_SCL__I2C1_SCL | PC,
		.gpio_mode = MX8MP_PAD_I2C1_SCL__GPIO5_IO14 | PC,
		.gp = IMX_GPIO_NR(5, 14),
	},
	.sda = {
		.i2c_mode = MX8MP_PAD_I2C1_SDA__I2C1_SDA | PC,
		.gpio_mode = MX8MP_PAD_I2C1_SDA__GPIO5_IO15 | PC,
		.gp = IMX_GPIO_NR(5, 15),
	},
};

int power_init_board(void)
{
	struct pmic *p;
	int ret;

	ret = power_pca9450_init(0);
	if (ret)
		printf("power init failed");
	p = pmic_get("PCA9450");
	pmic_probe(p);

	/* BUCKxOUT_DVS0/1 control BUCK123 output */
	pmic_reg_write(p, PCA9450_BUCK123_DVS, 0x29);

	/* Increase VDD_SOC and VDD_ARM to OD voltage 0.95V */
	pmic_reg_write(p, PCA9450_BUCK1OUT_DVS0, 0x1C);
	pmic_reg_write(p, PCA9450_BUCK2OUT_DVS0, 0x1C);

	/* Set BUCK1 DVS1 to suspend controlled through PMIC_STBY_REQ */
	pmic_reg_write(p, PCA9450_BUCK1OUT_DVS1, 0x14);
	pmic_reg_write(p, PCA9450_BUCK1CTRL, 0x59);

	/* Set WDOG_B_CFG to cold reset */
	pmic_reg_write(p, PCA9450_RESET_CTRL, 0xA1);

	return 0;
}

void spl_board_init(void)
{
	/* Set GIC clock to 500Mhz for OD VDD_SOC. */
	clock_enable(CCGR_GIC, 0);
	clock_set_target_val(GIC_CLK_ROOT, CLK_ROOT_ON | CLK_ROOT_SOURCE_SEL(5));
	clock_enable(CCGR_GIC, 1);
}

int board_fit_config_name_match(const char *name)
{
	return 0;
}

#define UART_PAD_CTRL   (PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL   (PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const uart_pads[] = {
	MX8MP_PAD_UART1_RXD__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX8MP_PAD_UART1_TXD__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	MX8MP_PAD_GPIO1_IO02__WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
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

	init_uart_clk(0);

	board_early_init_f();

	ret = spl_early_init();
	if (ret) {
		debug("spl_early_init() failed: %d\n", ret);
		hang();
	}

	preloader_console_init();

	enable_tzc380();

	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);

	power_init_board();

	/* DDR initialization */
	spl_dram_init();
}
