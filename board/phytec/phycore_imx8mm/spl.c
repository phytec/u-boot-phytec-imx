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
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <fsl_esdhc_imx.h>
#include <mmc.h>
#include <hang.h>
#include <init.h>
#include <i2c.h>
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
	int size = 0xf;

	ret = phytec_eeprom_data_setup(0, EEPROM_ADDR, EEPROM_ADDR_FALLBACK);
	if (ret && !IS_ENABLED(CONFIG_PHYCORE_IMX8MM_RAM_SIZE_FIX))
		goto err;

	if (!ret) {
		printf("phytec_eeprom_data_init: init successful\n");
		phytec_print_som_info();
	}

	if (IS_ENABLED(CONFIG_PHYCORE_IMX8MM_RAM_SIZE_FIX)) {
		if (IS_ENABLED(CONFIG_PHYCORE_IMX8MM_RAM_SIZE_1GB))
			size = 1;
		else if (IS_ENABLED(CONFIG_PHYCORE_IMX8MM_RAM_SIZE_2GB))
			size = 3;
		else if (IS_ENABLED(CONFIG_PHYCORE_IMX8MM_RAM_SIZE_4GB))
			size = 5;
	} else {
		size = phytec_get_imx8m_ddr_size();
	}

	switch (size) {
	case 1:
		/* 1GB RAM */
		dram_timing_1gb.ddrphy_cfg[82].val = 0x3;
		dram_timing_1gb.ddrphy_cfg[83].val = 0x3;
		ddr_init(&dram_timing_1gb);
		break;
	case 3:
		/* 2GB RAM */
		ddr_init(&dram_timing);
		break;
	case 5:
		/* 4GB RAM */
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

#define I2C_PAD_CTRL    (PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PE)
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = IMX8MM_PAD_I2C1_SCL_I2C1_SCL | PC,
		.gpio_mode = IMX8MM_PAD_I2C1_SCL_GPIO5_IO14 | PC,
		.gp = IMX_GPIO_NR(5, 14),
	},
	.sda = {
		.i2c_mode = IMX8MM_PAD_I2C1_SDA_I2C1_SDA | PC,
		.gpio_mode = IMX8MM_PAD_I2C1_SDA_GPIO5_IO15 | PC,
		.gp = IMX_GPIO_NR(5, 15),
	},
};

#define USDHC2_CD_GPIO  IMX_GPIO_NR(2, 12)

#define USDHC_PAD_CTRL  (PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PE | \
			 PAD_CTL_FSEL2)
#define USDHC_GPIO_PAD_CTRL (PAD_CTL_HYS)

static iomux_v3_cfg_t const usdhc2_pads[] = {
	IMX8MM_PAD_SD2_CLK_USDHC2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_SD2_CMD_USDHC2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_SD2_DATA0_USDHC2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_SD2_DATA1_USDHC2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_SD2_DATA2_USDHC2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_SD2_DATA3_USDHC2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_SD2_CD_B_GPIO2_IO12 | MUX_PAD_CTRL(USDHC_GPIO_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc3_pads[] = {
	IMX8MM_PAD_NAND_WE_B_USDHC3_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_WP_B_USDHC3_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA04_USDHC3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA05_USDHC3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA06_USDHC3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA07_USDHC3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_RE_B_USDHC3_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_CE2_B_USDHC3_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_CE3_B_USDHC3_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_CLE_USDHC3_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static struct fsl_esdhc_cfg usdhc_cfg[2] = {
	{.esdhc_base = USDHC2_BASE_ADDR, .max_bus_width = 4},
	{.esdhc_base = USDHC3_BASE_ADDR, .max_bus_width = 8},
};

int board_mmc_init(struct bd_info *bis)
{
	int i, ret;
	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-Boot device node)    (Physical Port)
	 * mmc0                    USDHC1
	 * mmc1                    USDHC2
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			init_clk_usdhc(1);
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			imx_iomux_v3_setup_multiple_pads(usdhc2_pads,
						ARRAY_SIZE(usdhc2_pads));
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
	case USDHC3_BASE_ADDR:
		return 1;
	case USDHC2_BASE_ADDR:
		ret = gpio_get_value(USDHC2_CD_GPIO);
		return ret ? 0 : 1;
	}

	return 0;
}

#define PHY_DET_0	IMX_GPIO_NR(1,  0)
#define PHY_DET_1	IMX_GPIO_NR(1, 16)

#define PHY_DET_GPIO_PAD_CTRL (PAD_CTL_DSE6 | PAD_CTL_FSEL3 | PAD_CTL_PE)

static iomux_v3_cfg_t const phy_det_pads[] = {
	IMX8MM_PAD_GPIO1_IO00_GPIO1_IO0 | MUX_PAD_CTRL(PHY_DET_GPIO_PAD_CTRL),
	IMX8MM_PAD_ENET_MDC_GPIO1_IO16 | MUX_PAD_CTRL(PHY_DET_GPIO_PAD_CTRL),
};

/*
 * PHY_DET_1	PHY_DET_0
 *    1		   0	   ADIN1300 (3,3V)
 *    0		   1	   DP83867  (2,5V)
 *
 *    Depending on the connected eth phy we have to
 *    increase LDO3 of the PMIC from 2,5V to 3,3V to
 *    make the eth phys functional.
 */
static int eth_phy_det(void)
{
	int phy_det = 0;

	imx_iomux_v3_setup_multiple_pads(phy_det_pads, ARRAY_SIZE(phy_det_pads));

	gpio_request(PHY_DET_0, "phy_det_0");
	gpio_direction_input(PHY_DET_0);

	gpio_request(PHY_DET_1, "phy_det_1");
	gpio_direction_input(PHY_DET_1);

	phy_det |= !!gpio_get_value(PHY_DET_0);
	phy_det |= !!gpio_get_value(PHY_DET_1) << 1;

	debug("phy_det: %x\n", phy_det);
	return phy_det;
}

#define PMIC_PF8121A_I2C_BUS		0x0
#define PMIC_PF8121A_I2C_ADDR		0x8
#define PMIC_PF8121A_LDO3_RUN_VOL_REG	0x94
#define PMIC_PF8121A_LDO_OUT_3_3_V	0xb

static int power_init_board(void)
{
	int phy_det;
	int ret;
	u8 ldo3;

	phy_det = eth_phy_det();
	/* Do not increase the LDO3 voltage for DP83867 phy. */
	if (phy_det == 0x1)
		return 0;

	i2c_set_bus_num(PMIC_PF8121A_I2C_BUS);

	ldo3 = PMIC_PF8121A_LDO_OUT_3_3_V;
	ret =  i2c_write(PMIC_PF8121A_I2C_ADDR, PMIC_PF8121A_LDO3_RUN_VOL_REG,
			 1, &ldo3, 1);
	if (ret) {
		printf("error writing pmic: %i\n", ret);
		return ret;
	}

	return 0;
}

void spl_board_init(void)
{
#ifndef CONFIG_SPL_USB_SDP_SUPPORT
	/* Serial download mode */
	if (is_usb_boot()) {
		puts("Back to ROM, SDP\n");
		restore_boot_params();
	}
#endif
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

	init_uart_clk(2);

	return 0;
}

void board_init_f(ulong dummy)
{
	int ret;

	arch_cpu_init();

	board_early_init_f();

	preloader_console_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

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
