// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#include <common.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/clock.h>
#include <asm/io.h>
#include <asm/mach-imx/boot_mode.h>
#include <env.h>
#include <fdt_support.h>
#include <jffs2/load_kernel.h>
#include <miiphy.h>
#include <mtd_node.h>

#include "../common/imx8m_som_detection.h"

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_I2C_PATH	"/soc@0/bus@30800000/i2c@30a20000/eeprom@59"

int ft_board_setup(void *blob, bd_t *bd)
{
	u8 spi = phytec_get_imx8m_spi();
	/* Do nothing if no SPI is poulated or data invalid */
	if (spi == 0 && spi == 0xff)
		return 0;

	static const struct node_info nodes[] = {
		{ "jedec,spi-nor", MTD_DEV_TYPE_NOR, },
	};

	fdt_fixup_mtdparts(blob, nodes, ARRAY_SIZE(nodes));

	return 0;
}

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Enable RGMII TX clk output */
	setbits_le32(&gpr->gpr[1], BIT(22));

	return 0;
}

int board_init(void)
{
	int ret;

	ret = phytec_eeprom_data_init(EEPROM_I2C_PATH, 0, 0);
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
	u8 spi = phytec_get_imx8m_spi();

	if (spi != 0 && spi != 0xff)
		env_set("spiprobe", "sf probe");

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
