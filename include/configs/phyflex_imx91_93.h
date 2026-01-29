/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2026 PHYTEC Messtechnik GmbH
 * Christoph Stoidner <c.stoidner@phytec.de>
 */

#ifndef __PHYFLEX_IMX91_93_H
#define __PHYFLEX_IMX91_93_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>

#define CFG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#define CFG_SYS_INIT_RAM_ADDR        0x80000000
#define CFG_SYS_INIT_RAM_SIZE        0x200000

#define CFG_SYS_SDRAM_BASE           0x80000000
#define PHYS_SDRAM                   0x80000000
#define PHYS_SDRAM_SIZE              0x80000000

/* Using ULP WDOG for reset */
#define WDOG_BASE_ADDR          WDG3_BASE_ADDR

#ifdef CONFIG_ENV_WRITEABLE_LIST
/* Set environment flag validation to a list of env vars that must be writable */
#define CFG_ENV_FLAGS_LIST_STATIC "BOOT_ORDER:sw,BOOT_system0_LEFT:dw,BOOT_system1_LEFT:dw"
#endif

#endif /* __PHYFLEX_IMX91_93_H */
