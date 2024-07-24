// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#include <common.h>
#include <command.h>
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

#define EEPROM_ADDR			0x51
#define EEPROM_ADDR_FALLBACK		0x59
#define MODE_REG_VAL_1GB_RAM		0xff030008
#define MODE_REG_VAL_2GB_4GB_RAM	0xff000110
#define INIT_1GB_RAM_BOOTCOUNTER	3

static void warm_reset_pmic(void)
{
	struct pmic *p = pmic_get("PCA9450");

	pmic_reg_write(p, PCA9450_SW_RST, 0x35);
}

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	return BOOT_DEVICE_BOOTROM;
}

void set_4gb_dram_timing(void)
{
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
}

void set_1gb_dram_timing(void)
{
	dram_timing.ddrc_cfg[3].val = 0x1233;
	dram_timing.ddrc_cfg[5].val = 0x5b0087;
	dram_timing.ddrc_cfg[6].val = 0x61027f10;
	dram_timing.ddrc_cfg[7].val = 0x7b0;
	dram_timing.ddrc_cfg[11].val = 0xf30000;
	dram_timing.ddrc_cfg[23].val = 0x8d;
	dram_timing.ddrc_cfg[45].val = 0xf070707;
	dram_timing.ddrc_cfg[59].val = 0x1031;
	dram_timing.ddrc_cfg[62].val = 0xc0012;
	dram_timing.ddrc_cfg[77].val = 0x13;
	dram_timing.ddrc_cfg[84].val = 0x1031;
	dram_timing.ddrc_cfg[87].val = 0x30005;
	dram_timing.ddrc_cfg[102].val = 0x5;
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
}

void set_2ghz_4gb_dram_timing(void)
{
	dram_timing.ddrc_cfg[2].val = 0xa3080020;
	dram_timing.ddrc_cfg[3].val = 0x1323;
	dram_timing.ddrc_cfg[4].val = 0x1e84800;
	dram_timing.ddrc_cfg[5].val = 0x7a0118;
	dram_timing.ddrc_cfg[8].val = 0xc00307a3;
	dram_timing.ddrc_cfg[9].val = 0xc50000;
	dram_timing.ddrc_cfg[10].val = 0xf4003f;
	dram_timing.ddrc_cfg[11].val = 0xf30000;
	dram_timing.ddrc_cfg[14].val = 0x2028222a;
	dram_timing.ddrc_cfg[15].val = 0x8083f;
	dram_timing.ddrc_cfg[16].val = 0xe0e000;
	dram_timing.ddrc_cfg[17].val = 0x12040a12;
	dram_timing.ddrc_cfg[18].val = 0x2050f0f;
	dram_timing.ddrc_cfg[19].val = 0x1010009;
	dram_timing.ddrc_cfg[20].val = 0x502;
	dram_timing.ddrc_cfg[21].val = 0x20800;
	dram_timing.ddrc_cfg[22].val = 0xe100002;
	dram_timing.ddrc_cfg[23].val = 0x120;
	dram_timing.ddrc_cfg[24].val = 0xc80064;
	dram_timing.ddrc_cfg[25].val = 0x3e8001e;
	dram_timing.ddrc_cfg[26].val = 0x3207a12;
	dram_timing.ddrc_cfg[28].val = 0x4a3820e;
	dram_timing.ddrc_cfg[30].val = 0x230e;
	dram_timing.ddrc_cfg[37].val = 0x799;
	dram_timing.ddrc_cfg[38].val = 0x9141d1c;
	dram_timing.ddrc_cfg[39].val = 0x17;
	dram_timing.ddrc_cfg[74].val = 0x302;
	dram_timing.ddrc_cfg[83].val = 0x599;
	dram_timing.ddrc_cfg[99].val = 0x302;
	dram_timing.ddrc_cfg[108].val = 0x599;
	dram_timing.ddrphy_cfg[66].val = 0x18;
	dram_timing.ddrphy_cfg[75].val = 0x1e3;
	dram_timing.ddrphy_cfg[77].val = 0x1e3;
	dram_timing.ddrphy_cfg[79].val = 0x1e3;
	dram_timing.ddrphy_cfg[145].val = 0x3e8;
	dram_timing.fsp_msg[0].drate = 4000;
	dram_timing.fsp_msg[0].fsp_cfg[1].val = 0xfa0;
	dram_timing.fsp_msg[0].fsp_cfg[9].val = 0x310;
	dram_timing.fsp_msg[0].fsp_cfg[10].val = 0x3ff4;
	dram_timing.fsp_msg[0].fsp_cfg[11].val = 0xf3;
	dram_timing.fsp_msg[0].fsp_cfg[15].val = 0x3ff4;
	dram_timing.fsp_msg[0].fsp_cfg[16].val = 0xf3;
	dram_timing.fsp_msg[0].fsp_cfg[21].val = 0x3;
	dram_timing.fsp_msg[0].fsp_cfg[22].val = 0xf400;
	dram_timing.fsp_msg[0].fsp_cfg[23].val = 0xf33f;
	dram_timing.fsp_msg[0].fsp_cfg[28].val = 0xf400;
	dram_timing.fsp_msg[0].fsp_cfg[29].val = 0xf33f;
	dram_timing.fsp_msg[1].fsp_cfg[10].val = 0x310;
	dram_timing.fsp_msg[1].fsp_cfg[22].val = 0x3;
	dram_timing.fsp_msg[2].fsp_cfg[10].val = 0x310;
	dram_timing.fsp_msg[2].fsp_cfg[22].val = 0x3;
	dram_timing.fsp_msg[3].drate = 4000;
	dram_timing.fsp_msg[3].fsp_cfg[1].val = 0xfa0;
	dram_timing.fsp_msg[3].fsp_cfg[10].val = 0x310;
	dram_timing.fsp_msg[3].fsp_cfg[11].val = 0x3ff4;
	dram_timing.fsp_msg[3].fsp_cfg[12].val = 0xf3;
	dram_timing.fsp_msg[3].fsp_cfg[16].val = 0x3ff4;
	dram_timing.fsp_msg[3].fsp_cfg[17].val = 0xf3;
	dram_timing.fsp_msg[3].fsp_cfg[22].val = 0x3;
	dram_timing.fsp_msg[3].fsp_cfg[23].val = 0xf400;
	dram_timing.fsp_msg[3].fsp_cfg[24].val = 0xf33f;
	dram_timing.fsp_msg[3].fsp_cfg[29].val = 0xf400;
	dram_timing.fsp_msg[3].fsp_cfg[30].val = 0xf33f;
	dram_timing.ddrphy_pie[480].val = 0x465;
	dram_timing.ddrphy_pie[481].val = 0xfa;
	dram_timing.ddrphy_pie[482].val = 0x9c4;
	dram_timing.fsp_table[0] = 4000;
}

static unsigned int imx8mp_lpddr4_get_mr(void)
{
	unsigned int ddr_info = 0;
	unsigned int const regs[] = { 5, 6, 7, 8 };

	for (int attempts = 4; attempts >= 0; attempts--) {
		for (int i = 0 ; i < ARRAY_SIZE(regs) ; i++) {
			unsigned int data = lpddr4_mr_read(0xF, regs[i]);

			data >>= 8;
			ddr_info <<= 8;
			ddr_info += (data & 0xFF);
		}

		if (ddr_info != 0xFFFFFFFF && ddr_info != 0)
			break;

		printf("read mr attempt failed, %i attempts left\n", attempts);
	}
	return  ddr_info;
}

static void spl_ram_autodetect(u8 bootcounter)
{
	int ret = 0;
	int mode_reg = 0;
	u8 ram_size = 0;
	u8 *const bootcounter_ocram_s = (u8 *)CONFIG_SAVED_DRAM_TIMING_BASE;

	switch (bootcounter) {
	case 1:
		set_4gb_dram_timing();
		ram_size = 4;
		break;
	case 2:
		ram_size = 2;
		break;
	case 3:
		ram_size = 1;
		set_1gb_dram_timing();
		break;
	default:
		printf("failed to find any matching RAM timings\n");
		do_reset(NULL, 0, 0, NULL);
	}

	ret = ddr_init(&dram_timing);
	if (ret) {
		if (bootcounter == 3) {
			printf("RAM Autodetection failed\n");
			do_reset(NULL, 0, 0, NULL);
		} else {
			*bootcounter_ocram_s = bootcounter + 1;
			warm_reset_pmic();
		}
	}

	mode_reg = imx8mp_lpddr4_get_mr();
	if (mode_reg == MODE_REG_VAL_1GB_RAM && bootcounter != INIT_1GB_RAM_BOOTCOUNTER) {
		*bootcounter_ocram_s = 3;
		warm_reset_pmic();
	} else if (mode_reg == MODE_REG_VAL_2GB_4GB_RAM &&
		   bootcounter == INIT_1GB_RAM_BOOTCOUNTER) {
		*bootcounter_ocram_s = 1;
		warm_reset_pmic();
	} else if (mode_reg != MODE_REG_VAL_2GB_4GB_RAM &&
		   mode_reg != MODE_REG_VAL_1GB_RAM) {
		printf("error: mode register values do not match a valid RAM config\n");
	} else {
		printf("detected %uGB RAM\n", ram_size);
	}
}

void spl_dram_init(void)
{
	int ret = 1;
	int size = 0xf;
	u8 *const bootcounter_ocram_s = (u8 *)CONFIG_SAVED_DRAM_TIMING_BASE;
	u8 bootcounter = *bootcounter_ocram_s;

	if (bootcounter >= 4)
		bootcounter = 1;
	if (bootcounter == 1)
		ret = phytec_eeprom_data_setup(NULL, 0, EEPROM_ADDR, EEPROM_ADDR_FALLBACK);

	if (ret && !IS_ENABLED(CONFIG_PHYCORE_IMX8MP_RAM_SIZE_FIX)) {
		printf("EEPROM detection failed and fixed RAM size not set, falling back to " \
		       "autodetection\n");
		spl_ram_autodetect(bootcounter);
		return;
	}

	if (!ret) {
		printf("phytec_eeprom_data_init: init successful\n");
		phytec_print_som_info(NULL);
	}

	if (IS_ENABLED(CONFIG_PHYCORE_IMX8MP_RAM_SIZE_FIX)) {
		if (IS_ENABLED(CONFIG_PHYCORE_IMX8MP_RAM_SIZE_1GB))
			size = 2;
		else if (IS_ENABLED(CONFIG_PHYCORE_IMX8MP_RAM_SIZE_2GB))
			size = 3;
		else if (IS_ENABLED(CONFIG_PHYCORE_IMX8MP_RAM_SIZE_4GB))
			size = 5;
		else if (IS_ENABLED(CONFIG_PHYCORE_IMX8MP_RAM_SIZE_4GB_2GHZ))
			size = 8;
	} else {
		size = phytec_get_imx8m_ddr_size(NULL);
	}
	switch (size) {
	case 2:
		set_1gb_dram_timing();
		if (ddr_init(&dram_timing))
			goto err;
		printf("dram init complete for 1GB\n");
		break;
	case 3:
		if (ddr_init(&dram_timing))
			goto err;
		printf("dram init complete for 2GB\n");
		break;
	case 5:
		set_4gb_dram_timing();
		if (ddr_init(&dram_timing))
			goto err;
		printf("dram init complete for 4GB\n");
		break;
	case 8:
		set_2ghz_4gb_dram_timing();
		if (ddr_init(&dram_timing))
			goto err;
		printf("dram init complete for 4GB 2GHz timings\n");
		break;
	default:
		goto err;
	}
	return;
err:
	printf("Could not detect correct RAM size.\n");
	do_reset(NULL, 0, 0, NULL);
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

	/* Set LDO4 to 0,9V. Needed for ADIN eth phy */
	pmic_reg_write(p, PCA9450_LDO4CTRL, 0xC1);

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
