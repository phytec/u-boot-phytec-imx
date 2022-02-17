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
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <fsl_esdhc_imx.h>
#include <hang.h>
#include <init.h>
#include <log.h>
#include <power/pmic.h>
#include <power/pca9450.h>
#include <spl.h>

#include "../common/imx8m_som_detection.h"

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_I2C_ADDR		0x59

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	return BOOT_DEVICE_BOOTROM;
}

extern struct dram_timing_info dram_timing_4gb;

void spl_dram_init(void)
{
	int ret;

	ret = phytec_eeprom_data_init(0, 0, EEPROM_I2C_ADDR);
	if (ret < 0)
		goto err;

	phytec_print_som_info();

	switch (phytec_get_imx8m_ddr_size()) {
	case 2:
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
		ddr_init(&dram_timing);
		break;
	case 5:
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

#define USDHC2_CD_GPIO  IMX_GPIO_NR(2, 12)
#define USDHC2_PWR_GPIO IMX_GPIO_NR(2, 19)

#define USDHC_PAD_CTRL  (PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE | \
			 PAD_CTL_PE | PAD_CTL_FSEL2)
#define USDHC_GPIO_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_DSE1)
#define USDHC_CD_PAD_CTRL (PAD_CTL_PE | PAD_CTL_PUE | PAD_CTL_HYS | \
			   PAD_CTL_DSE4)

static iomux_v3_cfg_t const usdhc3_pads[] = {
	MX8MP_PAD_NAND_WE_B__USDHC3_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_WP_B__USDHC3_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_DATA04__USDHC3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_DATA05__USDHC3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_DATA06__USDHC3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_DATA07__USDHC3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_RE_B__USDHC3_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_CE2_B__USDHC3_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_CE3_B__USDHC3_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_CLE__USDHC3_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc2_pads[] = {
	MX8MP_PAD_SD2_CLK__USDHC2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_SD2_CMD__USDHC2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_SD2_DATA0__USDHC2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_SD2_DATA1__USDHC2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_SD2_DATA2__USDHC2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_SD2_DATA3__USDHC2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_SD2_RESET_B__GPIO2_IO19 | MUX_PAD_CTRL(USDHC_GPIO_PAD_CTRL),
	MX8MP_PAD_SD2_CD_B__GPIO2_IO12    | MUX_PAD_CTRL(USDHC_CD_PAD_CTRL),
};

static struct fsl_esdhc_cfg usdhc_cfg[2] = {
	{USDHC2_BASE_ADDR, 0, 4},
	{USDHC3_BASE_ADDR, 0, 8},
};

int board_mmc_init(bd_t *bis)
{
	int i, ret;
	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-Boot device node)    (Physical Port)
	 * mmc0                    USDHC2
	 * mmc1                    USDHC3
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			init_clk_usdhc(1);
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			imx_iomux_v3_setup_multiple_pads(usdhc2_pads,
							 ARRAY_SIZE(usdhc2_pads));
			gpio_request(USDHC2_PWR_GPIO, "usdhc2_reset");
			gpio_direction_output(USDHC2_PWR_GPIO, 0);
			udelay(500);
			gpio_direction_output(USDHC2_PWR_GPIO, 1);
			gpio_request(USDHC2_CD_GPIO, "usdhc2 cd");
			gpio_direction_input(USDHC2_CD_GPIO);
			break;
		case 1:
			init_clk_usdhc(2);
			usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
			imx_iomux_v3_setup_multiple_pads(usdhc3_pads,
							 ARRAY_SIZE(usdhc3_pads));
			break;

		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return -EINVAL;
		}
		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
		if (ret)
			return ret;
	}

	return 0;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC2_BASE_ADDR:
		ret = !gpio_get_value(USDHC2_CD_GPIO);
		break;
	case USDHC3_BASE_ADDR:
		ret = 1;
		break;
	}

	return ret;
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

	ret = power_pca9450b_init(0);
	if (ret)
		printf("power init failed");
	p = pmic_get("PCA9450");
	pmic_probe(p);

	/* BUCKxOUT_DVS0/1 control BUCK123 output */
	pmic_reg_write(p, PCA9450_BUCK123_DVS, 0x29);

	/* Increase VDD_SOC and VDD_ARM to OD voltage 0.95V */
	pmic_reg_write(p, PCA9450_BUCK1OUT_DVS0, 0x1C);
	pmic_reg_write(p, PCA9450_BUCK2OUT_DVS0, 0x1C);

	/* Set BUCK1 DVS1 to suspend controled through PMIC_STBY_REQ */
	pmic_reg_write(p, PCA9450_BUCK1OUT_DVS1, 0x14);
	pmic_reg_write(p, PCA9450_BUCK1CTRL, 0x59);

	/* Set WDOG_B_CFG to cold reset */
	pmic_reg_write(p, PCA9450_RESET_CTRL, 0xA1);

	return 0;
}

void spl_board_init(void)
{
	/*
	* Set GIC clock to 500Mhz for OD VDD_SOC. Kernel driver does
	* not allow to change it. Should set the clock after PMIC
	* setting done. Default is 400Mhz (system_pll1_800m with div = 2)
	* set by ROM for ND VDD_SOC
	*/
	clock_enable(CCGR_GIC, 0);
	clock_set_target_val(GIC_CLK_ROOT, CLK_ROOT_ON | CLK_ROOT_SOURCE_SEL(5));
	clock_enable(CCGR_GIC, 1);
}

int board_fit_config_name_match(const char *name)
{
	/* Nothing to do for now */
	return 0;
}

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const uart1_pads[] = {
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

	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
	init_uart_clk(0);

	return 0;
}

void board_init_f(ulong dummy)
{
	int ret;

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	arch_cpu_init();

	board_early_init_f();

	timer_init();

	preloader_console_init();

	ret = spl_init();
	if (ret) {
		debug("spl_init() failed: %d\n", ret);
		hang();
	}

	enable_tzc380();

	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);

	power_init_board();

	/* DDR initialization */
	spl_dram_init();

	board_init_r(NULL, 0);
}
