//SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Janine Hagemann <j.hagemann@phytec.de>
 */

#include <common.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx8mq_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/io.h>
#include <asm/mach-imx/boot_mode.h>
#include <env.h>
#include <miiphy.h>
#include <power/pfuze100_pmic.h>
#include <power/pmic.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL   (PAD_CTL_DSE6 | PAD_CTL_FSEL1)

#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE)

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MQ_PAD_GPIO1_IO02__WDOG1_WDOG_B | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

static iomux_v3_cfg_t const uart_pads[] = {
	IMX8MQ_PAD_UART1_RXD__UART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MQ_PAD_UART1_TXD__UART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

int board_early_init_f(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));
	set_wdog_reset(wdog);

	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	return 0;
}

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&gpr->gpr[1],
			IOMUXC_GPR_GPR1_GPR_ENET1_TX_CLK_SEL_MASK, 0);

	return set_clk_enet(ENET_125MHZ);
}

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}

void imx8mq_set_bootsource(void)
{
	if (get_boot_device() == MMC1_BOOT) {
		env_set_ulong("mmcdev", 0);
	} else if (get_boot_device() == SD2_BOOT) {
		env_set_ulong("mmcdev", 1);
	} else {
		pr_err("No available bootsource selected!\n");
	}
}

int board_init(void)
{
	setup_fec();
	return 0;
}

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int board_late_init(void)
{
	imx8mq_set_bootsource();
	env_set("board_name", "phyCORE-i.MX8MQ");
	env_set("board_rev", "iMX8MQ");

	return 0;
}
