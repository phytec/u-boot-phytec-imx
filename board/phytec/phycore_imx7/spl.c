// SPDX-License-Identifier: GPL-2.0+
/* Copyright (C) 2022 PHYTEC America, LLC */

#include <common.h>
#include <fsl_esdhc_imx.h>
#include <hang.h>
#include <init.h>
#include <i2c.h>
#include <spl.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch-mx7/clock.h>
#include <asm/arch-mx7/mx7-ddr.h>
#include <asm/arch-mx7/mx7-pins.h>
#include <asm/gpio.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <power/pfuze3000_pmic.h>
#include <power/pmic.h>

#define I2C_PAD_CTRL    (PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU100KOHM)

/* I2C1 for PMIC */
static struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = MX7D_PAD_I2C1_SCL__I2C1_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = MX7D_PAD_I2C1_SCL__GPIO4_IO8 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(4, 8),
	},
	.sda = {
		.i2c_mode = MX7D_PAD_I2C1_SDA__I2C1_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = MX7D_PAD_I2C1_SDA__GPIO4_IO9 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(4, 9),
	},
};

#define UART_PAD_CTRL  (PAD_CTL_DSE_3P3V_49OHM | \
	PAD_CTL_PUS_PU100KOHM | PAD_CTL_HYS)

static iomux_v3_cfg_t const uart1_pads[] = {
	MX7D_PAD_UART1_RX_DATA__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX7D_PAD_UART1_TX_DATA__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const uart5_pads[] = {
	MX7D_PAD_GPIO1_IO07__UART5_TX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX7D_PAD_GPIO1_IO06__UART5_RX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX7D_PAD_I2C3_SDA__UART5_DCE_RTS | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX7D_PAD_I2C3_SCL__UART5_DCE_CTS | MUX_PAD_CTRL(UART_PAD_CTRL),
};

#define USDHC_PAD_CTRL (PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU47KOHM)

static iomux_v3_cfg_t const phycore_imx7_usdhc1_pads[] = {
	MX7D_PAD_SD1_CLK__SD1_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_CMD__SD1_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA0__SD1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA1__SD1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA2__SD1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA3__SD1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_CD_B__GPIO5_IO0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static iomux_v3_cfg_t const phycore_imx7_usdhc3_pads[] = {
	MX7D_PAD_SD3_CLK__SD3_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_CMD__SD3_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA0__SD3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA1__SD3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA2__SD3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA3__SD3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA4__SD3_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA5__SD3_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA6__SD3_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA7__SD3_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_STROBE__SD3_STROBE	 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_RESET_B__GPIO6_IO11 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static struct fsl_esdhc_cfg phycore_imx7_usdhc_cfg[3] = {
	{USDHC1_BASE_ADDR, 0, 4},
	{USDHC3_BASE_ADDR},
};

#define USDHC1_CD_GPIO	IMX_GPIO_NR(5, 0)
#define USDHC1_PWR_GPIO	IMX_GPIO_NR(5, 2)
#define USDHC3_PWR_GPIO	IMX_GPIO_NR(6, 11)

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC1_BASE_ADDR:
		ret = !gpio_get_value(USDHC1_CD_GPIO);
		break;
	case USDHC3_BASE_ADDR:
		ret = 1; /* Assume uSDHC3 emmc is always present */
		break;
	}

	return ret;
}

int board_mmc_init(bd_t *bis)
{
	int i, ret;
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			imx_iomux_v3_setup_multiple_pads(
				phycore_imx7_usdhc1_pads,
				ARRAY_SIZE(phycore_imx7_usdhc1_pads));
			gpio_request(USDHC1_CD_GPIO, "usdhc1_cd");
			gpio_direction_input(USDHC1_CD_GPIO);
			gpio_request(USDHC1_PWR_GPIO, "usdhc1_pwr");
			gpio_direction_output(USDHC1_PWR_GPIO, 0);
			udelay(500);
			gpio_direction_output(USDHC1_PWR_GPIO, 1);
			phycore_imx7_usdhc_cfg[0].sdhc_clk =
				mxc_get_clock(MXC_ESDHC_CLK);
			break;
		case 1:
			imx_iomux_v3_setup_multiple_pads(
				phycore_imx7_usdhc3_pads,
				ARRAY_SIZE(phycore_imx7_usdhc3_pads));
			gpio_request(USDHC3_PWR_GPIO, "usdhc3_pwr");
			gpio_direction_output(USDHC3_PWR_GPIO, 0);
			udelay(500);
			gpio_direction_output(USDHC3_PWR_GPIO, 1);
			phycore_imx7_usdhc_cfg[1].sdhc_clk =
				mxc_get_clock(MXC_ESDHC3_CLK);
			break;
		default:
			debug("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return -EINVAL;
		}

		ret = fsl_esdhc_initialize(bis,
			&phycore_imx7_usdhc_cfg[i]);
		if (ret)
			return ret;
	}

	return 0;
}

#define I2C_PMIC	0
int power_init_board(void)
{
	struct pmic *p;
	int ret;
	unsigned int reg, rev_id;

	ret = power_pfuze3000_init(I2C_PMIC);
	if (ret)
		return ret;

	p = pmic_get("PFUZE3000");
	ret = pmic_probe(p);
	if (ret)
		return ret;

	pmic_reg_read(p, PFUZE3000_DEVICEID, &reg);
	pmic_reg_read(p, PFUZE3000_REVID, &rev_id);
	printf("PMIC: PFUZE3000 DEV_ID=0x%x REV_ID=0x%x\n", reg, rev_id);

	/* disable Low Power Mode during standby mode */
	pmic_reg_read(p, PFUZE3000_LDOGCTL, &reg);
	reg |= 0x1;
	pmic_reg_write(p, PFUZE3000_LDOGCTL, reg);

	/* SW1A/1B mode set to APS/APS */
	reg = 0x8;
	pmic_reg_write(p, PFUZE3000_SW1AMODE, reg);
	pmic_reg_write(p, PFUZE3000_SW1BMODE, reg);

	/* SW1A/1B standby voltage set to 0.975V */
	reg = 0xb;
	pmic_reg_write(p, PFUZE3000_SW1ASTBY, reg);
	pmic_reg_write(p, PFUZE3000_SW1BSTBY, reg);

	/* set SW1B normal voltage to 0.975V */
	pmic_reg_read(p, PFUZE3000_SW1BVOLT, &reg);
	reg &= ~0x1f;
	reg |= PFUZE3000_SW1AB_SETP(975);
	pmic_reg_write(p, PFUZE3000_SW1BVOLT, reg);

#ifdef CONFIG_PCM_061_SPL_DDR3_1V5
	/* set SW3 normal voltage to 1.5V for DDR3 */
	reg = PFUZE3000_SW3_SETP(15000);
	pmic_reg_write(p, PFUZE3000_SW3VOLT, reg);
#endif

	return 0;
}

static struct ddrc phycore_imx7_ddrc_regs_val = {
	.mstr		= 0x01040001,
	.rfshtmg	= 0x00400046,
	.init1		= 0x00690000,
	.init0		= 0x00020083,
	.init3		= 0x09300004,
	.init4		= 0x04480000,
	.init5		= 0x00100004,
	.rankctl	= 0x0000033f,
	.dramtmg0	= 0x090e110a,
	.dramtmg1	= 0x0007020e,
	.dramtmg2	= 0x03040407,
	.dramtmg3	= 0x00002006,
	.dramtmg4	= 0x04020304,
	.dramtmg5	= 0x03030202,
	.dramtmg8	= 0x00000803,
	.zqctl0		= 0x00800020,
	.dfitmg0	= 0x02098204,
	.dfitmg1	= 0x00030303,
	.dfiupd0	= 0x80400003,
	.dfiupd1	= 0x00100020,
	.dfiupd2	= 0x80100004,
	.addrmap0	= 0x0000001f,
	.addrmap1	= 0x00080808,
	.addrmap3	= 0x00000000,
	.addrmap4	= 0x00000f0f,
	.addrmap5	= 0x07070707,
	.addrmap6	= 0x0f070707,
	.odtcfg		= 0x06000604,
	.odtmap		= 0x00000001,
};

static struct ddrc_mp phycore_imx7_ddrc_mp_val = {
	.pctrl_0	= 0x00000001,
};

static struct ddr_phy phycore_imx7_ddr_phy_regs_val = {
	.phy_con0	= 0x17420f40,
	.phy_con1	= 0x10210100,
	.phy_con4	= 0x00060807,
	.mdll_con0	= 0x1010007e,
	.drvds_con0	= 0x00000d6e,
	.lvl_con0	= 0x06090108,
	.offset_wr_con0	= 0x06060606,
	.offset_rd_con0	= 0x08080808,
	.cmd_sdll_con0	= 0x00000010,
	.offset_lp_con0	= 0x0000000f,
};

struct mx7_calibration phycore_imx7_ddr_calib_param = {
	.num_val	= 5,
	.values		= {
		0x0e407304,
		0x0e447304,
		0x0e447306,
		0x0e447304,
		0x0e407304,
	},
};

static void phycore_imx7_dram_cfg_size(u32 ram_size)
{
	switch (ram_size) {
	case SZ_256M:
		phycore_imx7_ddrc_regs_val.rfshtmg	= 0x0040001e;
		phycore_imx7_ddrc_regs_val.addrmap6	= 0x0f0f0f07;
		break;
	case SZ_512M:
		phycore_imx7_ddrc_regs_val.rfshtmg	= 0x0040002b;
		phycore_imx7_ddrc_regs_val.addrmap6	= 0x0f0f0707;
		break;
	}

	mx7_dram_cfg(&phycore_imx7_ddrc_regs_val,
		     &phycore_imx7_ddrc_mp_val,
		     &phycore_imx7_ddr_phy_regs_val,
		     &phycore_imx7_ddr_calib_param);
}

static void phycore_imx7_dram_cfg(void)
{
	ulong ram_size_test, ram_size;

	for (ram_size = SZ_1G; ram_size >= SZ_256M; ram_size >>= 1) {
		phycore_imx7_dram_cfg_size(ram_size);
		ram_size_test = get_ram_size((long int *)PHYS_SDRAM, ram_size);
		if (ram_size_test == ram_size)
			break;
	}

	if (ram_size < SZ_256M) {
		puts("!!!ERROR!!! DRAM detection failed!!!\n");
		hang();
	}
}

static void setup_iomux_uart(void)
{
#ifndef CONFIG_CONS_INDEX
#define CONFIG_CONS_INDEX 1
#endif
	switch(CONFIG_CONS_INDEX) {
	case 1:
		imx_iomux_v3_setup_multiple_pads(uart1_pads,
						 ARRAY_SIZE(uart1_pads));
		break;
	default:
		imx_iomux_v3_setup_multiple_pads(uart5_pads,
						 ARRAY_SIZE(uart5_pads));
		break;
	}
}

void board_init_f(ulong dummy)
{
	int ret;

	/* setup AIPS and disable watchdog */
	arch_cpu_init();
	/* setup GP timer */
	timer_init();
	setup_iomux_uart();
	/* UART clocks enabled and gd valid - init serial console */
	preloader_console_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	/* init malloc pool */
	ret = spl_init();
	if (ret) {
		debug("spl_init() failed: %d\n", ret);
		hang();
	}

	/* PMIC configuration */
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
	power_init_board();

	/* DRAM detection  */
	phycore_imx7_dram_cfg();

	/* load/boot image from boot device */
	board_init_r(NULL, 0);
}

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	switch (boot_dev_spl) {
	case MMC3_BOOT:
		return BOOT_DEVICE_MMC2;
	case NAND_BOOT:
		return BOOT_DEVICE_NAND;
	case SD1_BOOT:
	case MMC1_BOOT:
	default:
		return BOOT_DEVICE_MMC1;
	}
}

void spl_board_init(void)
{
	u32 boot_device = spl_boot_device();

	if (boot_device == BOOT_DEVICE_MMC1)
		puts("Booting from MMC/SD\n");
	else if (boot_device == BOOT_DEVICE_MMC2)
		puts("Booting from eMMC\n");
	else if (boot_device == BOOT_DEVICE_NAND)
		puts("Booting from NAND\n");
	else
		puts("Unknown boot device\n");
}

void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = spl_boot_device();
	switch (spl_boot_list[0]) {
	case BOOT_DEVICE_MMC1:
		spl_boot_list[1] = BOOT_DEVICE_MMC2;
		spl_boot_list[2] = BOOT_DEVICE_NAND;
		spl_boot_list[3] = BOOT_DEVICE_SPI;
		break;
	case BOOT_DEVICE_MMC2:
		spl_boot_list[1] = BOOT_DEVICE_MMC1;
		spl_boot_list[2] = BOOT_DEVICE_SPI;
		break;
	case BOOT_DEVICE_NAND:
		spl_boot_list[1] = BOOT_DEVICE_MMC1;
		spl_boot_list[2] = BOOT_DEVICE_SPI;
		break;
	}
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	if (!strcmp(name, "phycore-imx7d")) {
		if (is_cpu_type(MXC_CPU_MX7D))
			return 0;
	} else if (!strcmp(name, "phycore-imx7s")) {
		if (is_cpu_type(MXC_CPU_MX7S) && !CONFIG_IS_ENABLED(NAND_MXS))
			return 0;
	} else if (!strcmp(name, "phycore-imx7s-nand")) {
		if (is_cpu_type(MXC_CPU_MX7S) && CONFIG_IS_ENABLED(NAND_MXS))
			return 0;
	}

	return -1;
}
#endif
