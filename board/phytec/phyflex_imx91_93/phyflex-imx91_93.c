// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (C) 2026 PHYTEC Messtechnik GmbH
 * Author: Christoph Stoidner <c.stoidner@phytec.de>
 */

#include <asm/arch-imx9/ccm_regs.h>
#include <asm/arch/sys_proto.h>
#if defined(CONFIG_IMX91)
#include <asm/arch-imx9/imx91_pins.h>
#elif defined(CONFIG_IMX93)
#include <asm/arch-imx9/imx93_pins.h>
#else
#error Unknown Platform
#endif
#include <asm/arch/clock.h>
#include <asm/global_data.h>
#include <asm/mach-imx/boot_mode.h>
#include <env.h>
#include <extension_board.h>
#include <fdt_support.h>
#include <phy.h>

#include "../common/imx91_93_som_detection.h"

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_ADDR            0x50

int board_init(void)
{
	int ret = phytec_eeprom_data_setup(NULL, 1, EEPROM_ADDR);

	if (ret)
		printf("%s: EEPROM data init failed\n", __func__);

	return 0;
}

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int board_late_init(void)
{
	switch (get_boot_device()) {
	case SD2_BOOT:
		if (!env_get("boot_targets"))
			env_set("boot_targets", "mmc1 mmc0 ethernet");
		break;
	case MMC1_BOOT:
		env_set_ulong("mmcdev", 0);
		break;
	default:
		break;
	}

	return 0;
}

static void emmc_fixup(void *blob, struct phytec_eeprom_data *data)
{
	u8 option = phytec_imx91_93_phyflex_get_opt(data, PHYTEC_IMX91_93_PHYFLEX_OPT_IOVOLTAGE);
	int offset;

	if (option == PHYTEC_EEPROM_INVAL)
		goto err;

	/* Check if "IO Voltage" of WAKEUP domain is on 1v8 */
	if ((option & 0x04) == 0x00) {
		/* imx93-usdhc driver is also used by imx91 */
		offset = fdt_node_offset_by_compat_reg(blob, "fsl,imx93-usdhc",
						       0x42850000);
		if (offset)
			fdt_delprop(blob, offset, "no-1-8-v");
		else
			goto err;
	}

	return;
err:
	printf("Could not detect eMMC VDD-IO. Fall back to default.\n");
}

static void usdhc_clk_fixup(void *blob, u32 reg, unsigned long freq)
{
	/* imx93-usdhc driver is also used by imx91 */
	int offset = fdt_node_offset_by_compat_reg(blob, "fsl,imx93-usdhc", reg);

	if (offset) {
		offset = fdt_setprop_cell(blob, offset, "assigned-clock-rates", freq);
		if (offset > 0)
			printf("%s error for 0x%08x: %s\n", __func__, reg, fdt_strerror(offset));
	}
}

int board_fix_fdt(void *blob)
{
	struct phytec_eeprom_data data;

	phytec_eeprom_data_setup(&data, 1, EEPROM_ADDR);

	emmc_fixup(blob, &data);

	if (is_voltage_mode(VOLT_LOW_DRIVE)) {
		usdhc_clk_fixup(blob, 0x42850000, 266666667);
		usdhc_clk_fixup(blob, 0x42860000, 266666667);
		usdhc_clk_fixup(blob, 0x428b0000, 266666667);
	}

	return 0;
}

int ft_board_setup(void *blob, struct bd_info *bd)
{
	emmc_fixup(blob, NULL);

	/**
	 * NOTE: VOLT_LOW_DRIVE fixup is already done by the ft_system_setup()
	 * in arch/arm/mach-imx/imx9/native/soc.c (see low_drive_freq_update())
	 */

	return 0;
}
