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
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/iomux-v3.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <hang.h>
#include <init.h>
#include <i2c_eeprom.h>
#include <log.h>
#include <spl.h>

DECLARE_GLOBAL_DATA_PTR;

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

static int get_imx8mm_ddr_size(void)
{
	struct udevice *dev;
	ofnode eeprom;
	unsigned char var;
	int ret;

	eeprom = ofnode_path("/soc@0/bus@30800000/i2c@30a20000/eeprom@59");
	if (!ofnode_valid(eeprom)) {
		printf("Could not find i2c EEPROM in device tree.\n");
		return -ENODEV;
	}

	ret = uclass_get_device_by_ofnode(UCLASS_I2C_EEPROM, eeprom, &dev);
	if (ret) {
		printf("i2c EEPROM not found.\n");
		return ret;
	}

	ret = i2c_eeprom_read(dev, 6, &var, sizeof(var));
	if (ret) {
		printf("Unable to read from i2c EEPROM\n");
		return ret;
	}

	return ((int)var - 48);
}

void spl_dram_init(void)
{
	int ret;

	ret =  get_imx8mm_ddr_size();
	if (ret < 0)
		goto err;

	switch (ret) {
	case 3:
		ddr_init(&dram_timing);
		break;
	default:
		goto err;
	}

	return;
err:
	printf("Could not detect correct RAM size. Fallback to default.\n");
	ddr_init(&dram_timing);
}

void spl_board_init(void)
{
	/* Serial download mode */
	if (is_usb_boot()) {
		puts("Back to ROM, SDP\n");
		restore_boot_params();
	}
	puts("Normal Boot\n");
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	/* Just empty function now - can't decide what to choose */
	debug("%s: %s\n", __func__, name);

	return 0;
}
#endif

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

	return 0;
}

void board_init_f(ulong dummy)
{
	struct udevice *dev;
	int ret;

	arch_cpu_init();

	init_uart_clk(2);

	board_early_init_f();

	timer_init();

	preloader_console_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	ret = spl_early_init();
	if (ret) {
		debug("spl_early_init() failed: %d\n", ret);
		hang();
	}

	ret = uclass_get_device_by_name(UCLASS_CLK,
					"clock-controller@30380000", &dev);
	if (ret < 0) {
		printf("Failed to find clock node. Check device tree\n");
		hang();
	}

	enable_tzc380();

	/* DDR initialization */
	spl_dram_init();

	board_init_r(NULL, 0);
}
