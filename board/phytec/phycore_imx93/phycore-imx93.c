// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (C) 2023 PHYTEC Messtechnik GmbH
 * Author: Christoph Stoidner <c.stoidner@phytec.de>
 */

#include <common.h>
#include <asm/arch-imx9/ccm_regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch-imx9/imx93_pins.h>
#include <asm/arch/clock.h>
#include <asm/global_data.h>
#include <asm/mach-imx/boot_mode.h>
#include <env.h>
#include <extension_board.h>

#include "../common/imx93_som_detection.h"

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_ADDR            0x50

int board_init(void)
{
	int ret = phytec_eeprom_data_setup(NULL, 2, EEPROM_ADDR);

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
		env_set_ulong("mmcdev", 1);
		break;
	case MMC1_BOOT:
		env_set_ulong("mmcdev", 0);
		break;
	default:
		break;
	}

	return 0;
}

#ifdef CONFIG_CMD_EXTENSION
int extension_board_scan(struct list_head *extension_list)
{
	struct extension *extension = NULL;
	int ret = 0;
	u8 option;

	option = phytec_imx93_get_opt(NULL, PHYTEC_IMX93_OPT_EMMC);
	if (!option) {
		extension = phytec_add_extension("phyCORE-i.MX93 no eMMC",
						 "imx93-phycore-no-emmc.dtbo",
						 "eMMC not populated on the SoM");
		list_add_tail(&extension->list, extension_list);
		ret++;
	}

	option = phytec_imx93_get_opt(NULL, PHYTEC_IMX93_OPT_ETH);
	if (!option) {
		extension = phytec_add_extension("phyCORE-i.MX93 no eth phy",
						 "imx93-phycore-no-eth.dtbo",
						 "eth phy not populated on the SoM");
		list_add_tail(&extension->list, extension_list);
		ret++;
	}

	return ret;
}
#endif
