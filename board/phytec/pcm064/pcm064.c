
/*
 * Copyright 2017 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <netdev.h>
#include <fsl_ifc.h>
#include <fdt_support.h>
#include <libfdt.h>
#include <environment.h>
#include <fsl_esdhc.h>
#include <i2c.h>
#include "pca953x.h"

#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/imx-common/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <dm.h>
#include <imx8_hsio.h>
#include <usb.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <asm/imx-common/video.h>
#include <asm/arch/video_common.h>
#include <power-domain.h>

DECLARE_GLOBAL_DATA_PTR;

#define ESDHC_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ESDHC_CLK_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define UART_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_LOW << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PD << PADRING_PULL_SHIFT))

static iomux_cfg_t uart0_pads[] = {
	SC_P_UART0_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	SC_P_UART0_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx8_iomux_setup_multiple_pads(uart0_pads, ARRAY_SIZE(uart0_pads));
}

int board_early_init_f(void)
{
	sc_ipc_t ipcHndl = 0;
	sc_err_t sciErr = 0;

	ipcHndl = gd->arch.ipc_channel_handle;

	/* Power up UART0, this is very early while power domain is not working */
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_UART_0, SC_PM_PW_MODE_ON);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Set UART0 clock root to 80 MHz */
	sc_pm_clock_rate_t rate = 80000000;
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_UART_0, 2, &rate);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Enable UART0 clock root */
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_UART_0, 2, true, false);
	if (sciErr != SC_ERR_NONE)
		return 0;

	setup_iomux_uart();

	return 0;
}

#define USDHC2_CD_GPIO	IMX_GPIO_NR(4, 12)

static struct fsl_esdhc_cfg usdhc_cfg[CONFIG_SYS_FSL_USDHC_NUM] = {
	{USDHC1_BASE_ADDR, 0, 8},
	{USDHC3_BASE_ADDR, 0, 4},
};

static iomux_cfg_t emmc0[] = {
	SC_P_EMMC0_CLK | MUX_PAD_CTRL(ESDHC_CLK_PAD_CTRL),
	SC_P_EMMC0_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA4 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA5 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA6 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA7 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_RESET_B | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_STROBE | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
};

static iomux_cfg_t usdhc2_sd[] = {
	SC_P_USDHC2_CLK | MUX_PAD_CTRL(ESDHC_CLK_PAD_CTRL),
	SC_P_USDHC2_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC2_DATA0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC2_DATA1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC2_DATA2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC2_DATA3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC2_RESET_B | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC2_CD_B | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ESDHC_PAD_CTRL),	/* Mux to GPIO4 IO12 */
};

int board_mmc_init(bd_t *bis)
{
	int i, ret;
	struct power_domain pd;

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    USDHC1
	 * mmc1	                   USDHC3
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			if (!power_domain_lookup_name("conn_sdhc0", &pd))
				power_domain_on(&pd);

			imx8_iomux_setup_multiple_pads(emmc0, ARRAY_SIZE(emmc0));
			init_clk_usdhc(0);
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			break;
		case 1:
			if (!power_domain_lookup_name("conn_sdhc2", &pd))
				power_domain_on(&pd);

			imx8_iomux_setup_multiple_pads(usdhc2_sd, ARRAY_SIZE(usdhc2_sd));
			init_clk_usdhc(2);
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
			gpio_request(USDHC2_CD_GPIO, "sd2_cd");
			gpio_direction_input(USDHC2_CD_GPIO);
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return -EINVAL;
		}
		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
		if (ret) {
			printf("Warning: failed to initialize mmc dev %d\n", i);
			return ret;
		}
	}

	return 0;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC1_BASE_ADDR:
		ret = 1; /* eMMC */
		break;
	case USDHC3_BASE_ADDR:
		ret = !gpio_get_value(USDHC2_CD_GPIO);
		break;
	}

	return ret;
}

/*
 * Board specific reset that is system reset.
 */
void reset_cpu(ulong addr)
{
	puts("SCI reboot request");
	sc_pm_reboot(SC_IPC_CH, SC_PM_RESET_TYPE_COLD);
	while (1)
		putc('.');
}

int board_late_init(void)
{
	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}
#endif

int board_init(void)
{
	return 0;
}
