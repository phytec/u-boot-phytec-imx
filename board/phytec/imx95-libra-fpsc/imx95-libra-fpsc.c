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
#include <i2c.h>
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

#define TUSB_PORT_POL_CRTL_REG	0xB
#define TUSB_CUSTOM_POL		BIT(7)
#define TUSB_P0_POL		BIT(0)

/*
 * WORKAROUND for PCM-937-L 1618.0, 1618.1.
 * USB HUB TUSB8042A has swapped upstream pin polarity.
 * Set i2c registers to inform the hub that the lines
 * are swapped.
 *
 * We also notice that the HUB i2c address might not be
 * as expected for unknown reasons. Test all 4 possible i2c
 * addresses to write to the device.
 *
 */
void tusb8042a_swap_lines(void)
{
	const u8 pol_swap_val = (TUSB_CUSTOM_POL | TUSB_P0_POL);
	const int addr[4] = {0x44, 0x45, 0x46, 0x47};
	struct udevice *dev;
	int i, ret;

	for (i = 0; i < 4; i++) {
		ret = i2c_get_chip_for_busnum(2, addr[i], 1, &dev);
		if (!ret) {
			dm_i2c_write(dev, TUSB_PORT_POL_CRTL_REG, &pol_swap_val, 1);
			break;
		};
	}

	if (ret)
		printf("TUSB8042A: Failed to fixup USB HUB.\n");
}

int board_init(void)
{
	tusb8042a_swap_lines();

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
