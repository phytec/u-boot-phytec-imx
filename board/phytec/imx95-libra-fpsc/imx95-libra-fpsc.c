// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 PHYTEC Messtechnik GmbH
 */
#include <env.h>
#include <init.h>
#include <asm/global_data.h>
#include <asm/arch-imx9/ccm_regs.h>
#include <asm/arch/clock.h>
#include <fdt_support.h>
#include <dwc3-uboot.h>
#include <asm/io.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <miiphy.h>
#include <asm/gpio.h>
#include <asm/arch/sys_proto.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>
#include <scmi_agent.h>
#include <scmi_protocols.h>
#include <dt-bindings/clock/fsl,imx95-clock.h>
#include <dt-bindings/power/fsl,imx95-power.h>

DECLARE_GLOBAL_DATA_PTR;

int board_early_init_f(void)
{
	/* UART7: A55, UART2: M33 */
	init_uart_clk(6);

	return 0;
}

int board_init(void)
{
	return 0;
}

int board_late_init(void)
{
	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, struct bd_info *bd)
{
	return 0;
}
#endif

int board_phys_sdram_size(phys_size_t *size)
{
	*size = PHYS_SDRAM_SIZE + PHYS_SDRAM_2_SIZE;

	return 0;
}
