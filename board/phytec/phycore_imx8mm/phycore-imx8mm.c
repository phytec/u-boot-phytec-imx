// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2019-2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#include <common.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/mach-imx/boot_mode.h>
#include <env.h>
#include <env_internal.h>
#include <miiphy.h>

DECLARE_GLOBAL_DATA_PTR;

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

int board_phys_sdram_size(phys_size_t *size)
{
	if (!size)
		return -EINVAL;

	*size = imx8m_ddrc_sdram_size();

	return 0;
}

enum env_location env_get_location(enum env_operation op, int prio)
{
	enum boot_device dev = get_boot_device();
	enum env_location env_loc = ENVL_UNKNOWN;

	if (prio > 1)
		return env_loc;

	if (prio == 1) {
		if (IS_ENABLED(CONFIG_ENV_APPEND))
			return ENVL_NOWHERE;
		else
			return env_loc;
	}

	switch (dev) {
		case QSPI_BOOT:
			env_loc = ENVL_SPI_FLASH;
			break;
		case SD1_BOOT:
		case SD2_BOOT:
		case SD3_BOOT:
		case MMC2_BOOT:
		case MMC3_BOOT:
		case USB_BOOT:
			env_loc =  ENVL_MMC;
			break;
		default:
			env_loc = ENVL_NOWHERE;
			break;
	}

	return env_loc;
}
