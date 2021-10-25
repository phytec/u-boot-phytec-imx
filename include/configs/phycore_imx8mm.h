/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2019-2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#ifndef __PHYCORE_IMX8MM_H
#define __PHYCORE_IMX8MM_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>

#include "phycore_rauc_env.h"

#define CONFIG_SYS_BOOTM_LEN		SZ_64M
#define CONFIG_SPL_MAX_SIZE		(148 * SZ_1K)
#define CONFIG_SYS_MONITOR_LEN		SZ_512K
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_USE_SECTOR
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR	0x300
#define CONFIG_SYS_UBOOT_BASE \
		(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#ifdef CONFIG_SPL_BUILD
#define CONFIG_SPL_STACK		0x920000
#define CONFIG_SPL_BSS_START_ADDR	0x910000
#define CONFIG_SPL_BSS_MAX_SIZE		SZ_8K
#define CONFIG_SYS_SPL_MALLOC_START	0x42200000
#define CONFIG_SYS_SPL_MALLOC_SIZE	SZ_512K

/* malloc f used before GD_FLG_FULL_MALLOC_INIT set */
#define CONFIG_MALLOC_F_ADDR		0x912000
/* For RAW image gives a error info not panic */
#define CONFIG_SPL_ABORT_ON_RAW_IMAGE

#define CONFIG_SYS_I2C

#endif

#define CONFIG_EXTRA_ENV_SETTINGS \
	"image=Image\0" \
	"console=ttymxc2,115200\0" \
	"fdt_addr=0x48000000\0" \
	"fdt_file=" CONFIG_DEFAULT_FDT_FILE "\0" \
	"ipaddr=192.168.3.11\0" \
	"serverip=192.168.3.10\0" \
	"netmask=255.225.255.0\0" \
	"ip_dyn=no\0" \
	"mtdparts=30bb0000.spi:3840k(u-boot),128k(env),128k(env_redund),-(none)\0" \
	"mtdids=nor0=30bb0000.spi\0" \
	"spiprobe=true\0" \
	"mmcdev=" __stringify(CONFIG_SYS_MMC_ENV_DEV) "\0" \
	"mmcpart=" __stringify(CONFIG_SYS_MMC_IMG_LOAD_PART) "\0" \
	"mmcroot=2\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs console=${console} " \
		"root=/dev/mmcblk${mmcdev}p${mmcroot} fsck.repair=yes rootwait rw \0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"if run loadfdt; then " \
			"booti ${loadaddr} - ${fdt_addr}; " \
		"else " \
			"echo WARN: Cannot load the DT; " \
		"fi;\0 " \
	"nfsroot=/nfs\0" \
	"netargs=setenv bootargs console=${console} root=/dev/nfs ip=${nfsip} " \
		"nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv nfsip dhcp; " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv nfsip ${ipaddr}:${serverip}::${netmask}::eth0:on; " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"run netargs; " \
		"${get_cmd} ${loadaddr} ${image}; " \
		"if ${get_cmd} ${fdt_addr} ${fdt_file}; then " \
			"booti ${loadaddr} - ${fdt_addr}; " \
		"else " \
			"echo WARN: Cannot load the DT; " \
		"fi;\0" \
	"doraucboot=0\0" \
	"raucdev=2\0" \
	PHYCORE_RAUC_ENV_BOOTLOGIC

#define CONFIG_BOOTCOMMAND \
	"mmc dev ${mmcdev}; if mmc rescan; then " \
		"run spiprobe; " \
		"if test ${doraucboot} = 1; then " \
			"run raucboot; " \
		"elif run loadimage; then " \
			"run mmcboot; " \
		"else run netboot; " \
		"fi; " \
	"fi;"

/* Link Definitions */
#define CONFIG_LOADADDR			0x40480000
#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR

#define CONFIG_SYS_INIT_RAM_ADDR	0x40000000
#define CONFIG_SYS_INIT_RAM_SIZE	SZ_512K
#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

#define CONFIG_MMCROOT			"/dev/mmcblk2p2"  /* USDHC3 */

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		SZ_32M
#define CONFIG_SYS_SDRAM_BASE		0x40000000

#define PHYS_SDRAM			0x40000000
#define PHYS_SDRAM_SIZE                 SZ_2G /* 2GB DDR */

/* UART */
#define CONFIG_MXC_UART_BASE		UART3_BASE_ADDR

/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE		SZ_2K
#define CONFIG_SYS_MAXARGS		64
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

/* USDHC */
#define CONFIG_FSL_USDHC
#define CONFIG_SYS_FSL_USDHC_NUM	2
#define CONFIG_SYS_FSL_ESDHC_ADDR       0
#define CONFIG_SYS_MMC_IMG_LOAD_PART	1

/* I2C */
#define CONFIG_SYS_I2C_SPEED		100000

#endif /* __PHYCORE_IMX8MM_H */
