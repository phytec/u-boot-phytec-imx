// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#include <common.h>
#include <asm/arch/sys_proto.h>
#include <asm/io.h>
#include <asm/mach-imx/boot_mode.h>
#include <env.h>
#include <extension_board.h>
#include <miiphy.h>

#include "../common/imx8m_som_detection.h"

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_ADDR		0x51
#define EEPROM_ADDR_FALLBACK	0x59

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&gpr->gpr[1], 0x2000, 0);

	return 0;
}

int board_init(void)
{
	int ret = phytec_eeprom_data_setup_fallback(NULL, 0,
			EEPROM_ADDR, EEPROM_ADDR_FALLBACK);
	if (ret)
		printf("%s: EEPROM data init failed\n", __func__);

	setup_fec();

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
	case MMC3_BOOT:
		env_set_ulong("mmcdev", 2);
		break;
	default:
		break;
	}

	return 0;
}

#if (IS_ENABLED(CONFIG_CMD_EXTENSION))
int extension_board_scan(struct list_head *extension_list)
{
	int cnt = 0;

	if (!phytec_get_imx8m_eth(NULL)) {
		list_add_tail(&phytec_add_extension(
					"phyCORE-i.MX8MN no eth phy",
					"imx8mn-phycore-no-eth.dtbo",
					"eth phy not populated on SoM"
					)->list, extension_list);
		cnt += 1;
	}
	if (!phytec_get_imx8m_spi(NULL)) {
		list_add_tail(&phytec_add_extension(
					"phyCORE-i.MX8MN no SPI flash",
					"imx8mn-phycore-no-spiflash.dtbo",
					"SPI flash not populated on SoM"
					)->list, extension_list);
		cnt += 1;
	}
	return cnt;
}
#endif /* CONFIG_CMD_EXTENSION */

int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no;
}
