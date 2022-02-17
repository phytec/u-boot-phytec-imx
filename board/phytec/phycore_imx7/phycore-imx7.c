// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2015-2021 PHYTEC America, LLC
 *
 * Based on board/freescale/mx7dsabresd/mx7dsabresd.c
 *
 */

#include <asm/arch/clock.h>
#include <asm/arch/mx7-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/io.h>
#include <common.h>
#include <mmc.h>
#include <miiphy.h>
#include <power/pmic.h>
#include <power/pfuze3000_pmic.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_DSE_3P3V_49OHM | \
	PAD_CTL_PUS_PU100KOHM | PAD_CTL_HYS)

#define NAND_PAD_CTRL (PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_SRE_SLOW | PAD_CTL_HYS)

#define NAND_PAD_READY0_CTRL (PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_PUS_PU5KOHM)

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

static iomux_v3_cfg_t const wdog_pads[] = {
	MX7D_PAD_GPIO1_IO00__WDOG1_WDOG_B | MUX_PAD_CTRL(NO_PAD_CTRL),
};

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

#ifdef CONFIG_NAND_MXS
static iomux_v3_cfg_t const gpmi_pads[] = {
	MX7D_PAD_SD3_DATA0__NAND_DATA00 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA1__NAND_DATA01 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA2__NAND_DATA02 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA3__NAND_DATA03 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA4__NAND_DATA04 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA5__NAND_DATA05 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA6__NAND_DATA06 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA7__NAND_DATA07 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_CLK__NAND_CLE	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_CMD__NAND_ALE	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_STROBE__NAND_RE_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_RESET_B__NAND_WE_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_MCLK__NAND_WP_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_RX_BCLK__NAND_CE3_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_RX_SYNC__NAND_CE2_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_RX_DATA__NAND_CE1_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_TX_BCLK__NAND_CE0_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_TX_SYNC__NAND_DQS	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_TX_DATA__NAND_READY_B
		| MUX_PAD_CTRL(NAND_PAD_READY0_CTRL),
};

static void setup_gpmi_nand(void)
{
	imx_iomux_v3_setup_multiple_pads(gpmi_pads, ARRAY_SIZE(gpmi_pads));

	/*
	 * NAND_USDHC_BUS_CLK is set in rom
	 */

	set_clk_nand();

	/*
	 * APBH clock root is set in init_esdhc, USDHC3_CLK.
	 * There is no clk gate for APBHDMA.
	 * No touch here.
	 */
}
#endif /* CONFIG_NAND_MXS */

static void setup_iomux_uart(void)
{
#ifndef CONFIG_CONS_INDEX
#define CONFIG_CONS_INDEX 1
#endif
	switch (CONFIG_CONS_INDEX) {
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

#ifdef CONFIG_IMX_BOOTAUX
ulong board_get_usable_ram_top(ulong total_size)
{
	/* Reserve top 1M memory used by M core vring/buffer */
	return gd->ram_top - SZ_1M;
}
#endif

#ifdef CONFIG_FEC_MXC
static int setup_fec(int fec_id)
{
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	if (fec_id == 0) {
		/* Use 125M anatop REF_CLK1 for ENET1,
		 * clear gpr1[13], gpr1[17]
		 */
		clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
				(IOMUXC_GPR_GPR1_GPR_ENET1_TX_CLK_SEL_MASK |
				 IOMUXC_GPR_GPR1_GPR_ENET1_CLK_DIR_MASK),
				0);
	} else {
		/* Use 125M anatop REF_CLK2 for ENET2,
		 * clear gpr1[14], gpr1[18]
		 */
		clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
				(IOMUXC_GPR_GPR1_GPR_ENET2_TX_CLK_SEL_MASK |
				 IOMUXC_GPR_GPR1_GPR_ENET2_CLK_DIR_MASK),
				0);
	}

	return set_clk_enet(ENET_125MHZ);
}

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}
#endif

#ifdef CONFIG_FSL_QSPI
int board_qspi_init(void)
{
	/* Set the clock */
	set_clk_qspi();

	return 0;
}
#endif

int board_early_init_f(void)
{
	setup_iomux_uart();

	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_FEC_MXC
	setup_fec(CONFIG_FEC_ENET_DEV);
#endif

#ifdef CONFIG_NAND_MXS
	setup_gpmi_nand();
#endif

#ifdef CONFIG_FSL_QSPI
	board_qspi_init();
#endif
	return 0;
}

int power_init_board(void)
{
	struct udevice *dev;
	int ret, dev_id, rev_id;

	ret = pmic_get("pfuze3000@8", &dev);
	if (ret == -ENODEV)
		return 0;
	if (ret != 0)
		return ret;

	dev_id = pmic_reg_read(dev, PFUZE3000_DEVICEID);
	rev_id = pmic_reg_read(dev, PFUZE3000_REVID);
	printf("PMIC: PFUZE3000 DEV_ID=0x%x REV_ID=0x%x\n", dev_id, rev_id);

	pmic_clrsetbits(dev, PFUZE3000_LDOGCTL, 0, 1);

	return 0;
}

#ifdef CONFIG_ENV_IS_IN_MMC
static int check_mmc_autodetect(void)
{
	char *autodetect_str = env_get("mmcautodetect");

	if (autodetect_str && (strcmp(autodetect_str, "yes") == 0))
		return 1;

	return 0;
}

int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no;
}

void board_late_mmc_env_init(void)
{
	char cmd[32];
	char mmcblk[32];
	u32 dev_no = mmc_get_env_dev();

	if (!check_mmc_autodetect())
		return;

	env_set_ulong("mmcdev", dev_no);

	/* Set mmcblk env */
	sprintf(mmcblk, "/dev/mmcblk%dp2 rootwait rw",
		mmc_map_to_kernel_blk(dev_no));
	env_set("mmcroot", mmcblk);

	sprintf(cmd, "mmc dev %d", dev_no);
	run_command(cmd, 0);
}
#endif

int board_late_init(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));
	set_wdog_reset(wdog);

	return 0;
}

u32 get_board_rev(void)
{
	return get_cpu_rev();
}

int checkboard(void)
{
	char *mode;

	if (IS_ENABLED(CONFIG_ARMV7_BOOT_SEC_DEFAULT))
		mode = "secure";
	else
		mode = "non-secure";

	printf("Board: PHYTEC phyBOARD-Zeta i.MX7%c in %s mode\n",
	       is_cpu_type(MXC_CPU_MX7D) ? 'D' : 'S', mode);

	return 0;
}
