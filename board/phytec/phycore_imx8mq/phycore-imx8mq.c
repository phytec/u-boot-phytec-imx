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
#include <usb.h>
#include <dwc3-uboot.h>


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

int board_qspi_init(void)
{
	set_clk_qspi();
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

#define USB_PHY_CTRL0                  0xF0040
#define USB_PHY_CTRL0_REF_SSP_EN       BIT(2)

#define USB_PHY_CTRL1                  0xF0044
#define USB_PHY_CTRL1_RESET            BIT(0)
#define USB_PHY_CTRL1_COMMONONN                BIT(1)
#define USB_PHY_CTRL1_ATERESET         BIT(3)
#define USB_PHY_CTRL1_VDATSRCENB0      BIT(19)
#define USB_PHY_CTRL1_VDATDETENB0      BIT(20)

#define USB_PHY_CTRL2                  0xF0048
#define USB_PHY_CTRL2_TXENABLEN0       BIT(8)

static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_SUPER,
	.base = USB1_BASE_ADDR,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
	.power_down_scale = 2,
};

int usb_gadget_handle_interrupts(void)
{
	dwc3_uboot_handle_interrupt(0);
	return 0;
}

static void dwc3_nxp_usb_phy_init(struct dwc3_device *dwc3)
{
	u32 RegData;

	RegData = readl(dwc3->base + USB_PHY_CTRL1);
	RegData &= ~(USB_PHY_CTRL1_VDATSRCENB0 | USB_PHY_CTRL1_VDATDETENB0 |
			USB_PHY_CTRL1_COMMONONN);
	RegData |= USB_PHY_CTRL1_RESET | USB_PHY_CTRL1_ATERESET;
	writel(RegData, dwc3->base + USB_PHY_CTRL1);

	RegData = readl(dwc3->base + USB_PHY_CTRL0);
	RegData |= USB_PHY_CTRL0_REF_SSP_EN;
	writel(RegData, dwc3->base + USB_PHY_CTRL0);

	RegData = readl(dwc3->base + USB_PHY_CTRL2);
	RegData |= USB_PHY_CTRL2_TXENABLEN0;
	writel(RegData, dwc3->base + USB_PHY_CTRL2);

	RegData = readl(dwc3->base + USB_PHY_CTRL1);
	RegData &= ~(USB_PHY_CTRL1_RESET | USB_PHY_CTRL1_ATERESET);
	writel(RegData, dwc3->base + USB_PHY_CTRL1);
}

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 0 && init == USB_INIT_DEVICE) {
		imx8m_usb_power(index, true);
		dwc3_nxp_usb_phy_init(&dwc3_device_data);
		return dwc3_uboot_init(&dwc3_device_data);
	} else if (index == 0 && init == USB_INIT_HOST)
		return ret;

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;
	if (index == 0 && init == USB_INIT_DEVICE) {
		dwc3_uboot_exit(index);
		imx8m_usb_power(index, false);
	}
	return ret;
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
	board_qspi_init();
	setup_fec();
	init_usb_clk();

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

int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no;
}
