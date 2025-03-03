/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2025 PHYTEC Messtechnik GmbH
 */

#ifndef __IMX95_LIBRA_FPSC_H
#define __IMX95_LIBRA_FPSC_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>

#define CFG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#define CFG_SYS_INIT_RAM_ADDR	0x90000000
#define CFG_SYS_INIT_RAM_SIZE	0x200000

#define CFG_SYS_SDRAM_BASE	0x90000000
#define PHYS_SDRAM		0x90000000
/* 8GiB SDRAM */
#define PHYS_SDRAM_SIZE		0x70000000	/* 2GiB  - 256MiB DDR */
#define PHYS_SDRAM_2_SIZE	0x180000000	/* 6GiB */

#define CFG_SYS_FSL_USDHC_NUM	2

/* Using ULP WDOG for reset */
#define WDOG_BASE_ADDR          WDG3_BASE_ADDR

#endif /* __IMX95_LIBRA_FPSC_H */
