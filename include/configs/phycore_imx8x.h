/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2023 PHYTEC America, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PHYCORE_IMX8X_H
#define __PHYCORE_IMX8X_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>

#include "imx_env.h"

#ifdef CONFIG_SPL_BUILD
#define CFG_MALLOC_F_ADDR		0x00138000

/*
 * 0x08081000 - 0x08180FFF is for m4_0 xip image,
 * So 3rd container image may start from 0x8181000
 */
#define CFG_SYS_UBOOT_BASE 0x08181000
#endif

#ifdef CONFIG_AHAB_BOOT
#define AHAB_ENV "sec_boot=yes\0"
#else
#define AHAB_ENV "sec_boot=no\0"
#endif

/* Boot M4 */
#define M4_BOOT_ENV \
	"m4_0_image=m4_0.bin\0" \
	"loadm4image_0=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${m4_0_image}\0" \
	"m4boot_0=run loadm4image_0; dcache flush; bootaux ${loadaddr} 0\0" \

#define CFG_MFG_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x83100000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=0\0" \
	"sd_dev=1\0" \

/* Initial environment variables */
#define CFG_EXTRA_ENV_SETTINGS		\
	CFG_MFG_ENV_SETTINGS \
	M4_BOOT_ENV \
	AHAB_ENV \
	"script=boot.scr\0" \
	"image=Image\0" \
	"splashimage=0x9e000000\0" \
	"console=ttyLP0\0" \
	"fdt_addr=0x83000000\0"			\
	"fdto_addr=0x83100000\0" \
	"bootenv_addr=0x83200000\0" \
	"fdt_high=0xffffffffffffffff\0"		\
	"cntr_addr=0x98000000\0"			\
	"cntr_file=os_cntr_signed.bin\0" \
	"boot_fdt=try\0" \
	"fdt_file=imx8qxp-phytec-pcm-942.dtb\0" \
	"bootenv=bootenv.txt\0" \
	"mmc_load_bootenv=fatload mmc ${mmcdev}:${mmcpart} ${bootenv_addr} ${bootenv}\0" \
	"ipaddr=192.168.3.11\0" \
	"serverip=192.168.3.10\0" \
	"netmask=255.255.255.0\0" \
	"ip_dyn=no\0" \
	"mmcpart=1\0" \
	"mmcroot=2\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/mmcblk${mmcdev}p${mmcroot} fsck.repair=yes rootwait rw\0" \
	"loadbootscript=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"mmc_load_overlay=fatload mmc ${mmcdev}:${mmcpart} ${fdto_addr} ${overlay}\0" \
	"mmc_apply_overlays=fdt address ${fdt_addr}; "  \
		"if test ${no_overlays} = 0; then " \
			"for overlay in $overlays; " \
			"do; " \
				"if run mmc_load_overlay; then " \
					"fdt resize ${filesize}; " \
					"fdt apply ${fdto_addr}; " \
				"fi; " \
			"done;" \
		"fi;\0 " \
	"loadcntr=fatload mmc ${mmcdev}:${mmcpart} ${cntr_addr} ${cntr_file}\0" \
	"auth_os=auth_cntr ${cntr_addr}\0" \
	"boot_os=booti ${loadaddr} - ${fdt_addr};\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"if test ${no_bootenv} = 0; then " \
			"if run mmc_load_bootenv; then " \
				"env import -t ${bootenv_addr} ${filesize}; " \
			"fi; " \
		"fi; " \
		"run mmcargs; " \
		"if run loadfdt; then " \
			"run mmc_apply_overlays; " \
			"booti ${loadaddr} - ${fdt_addr}; " \
		"else " \
			"echo WARN: Cannot load the DT; " \
		"fi;\0 " \
	"nfsroot=/nfsroot\0" \
	"netargs=setenv bootargs console=${console},${baudrate} root=/dev/nfs ip=${nfsip} " \
		"nfsroot=${serverip}:${nfsroot},v4,tcp\0" \
	"net_load_bootenv=${get_cmd} ${bootenv_addr} ${bootenv}\0" \
	"net_load_overlay=${get_cmd} ${fdto_addr} ${overlay}\0" \
	"net_apply_overlays=fdt address ${fdt_addr}; " \
		"if test ${no_overlays} = 0; then " \
			"for overlay in $overlays; " \
			"do; " \
				"if run net_load_overlay; then " \
					"fdt resize ${filesize}; " \
					"fdt apply ${fdto_addr}; " \
				"fi; " \
			"done;" \
		"fi;\0 " \
	"netboot=echo Booting from net ...; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv nfsip dhcp; " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv nfsip ${ipaddr}:${serverip}::${netmask}::eth0:on; " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"if test ${no_bootenv} = 0; then " \
			"if run net_load_bootenv; then " \
				"env import -t ${bootenv_addr} ${filesize}; " \
			"fi; " \
		"fi; " \
		"run netargs; " \
		"${get_cmd} ${loadaddr} ${image}; " \
		"if ${get_cmd} ${fdt_addr} ${fdt_file}; then " \
			"run net_apply_overlays; " \
			"booti ${loadaddr} - ${fdt_addr}; " \
		"else " \
			"echo WARN: Cannot load the DT; " \
		"fi;\0" \

#define CONFIG_BOOTCOMMAND \
	   "mmc dev ${mmcdev}; if mmc rescan; then " \
		   "if run loadbootscript; then " \
			   "run bootscript; " \
		   "else " \
			   "if test ${sec_boot} = yes; then " \
				   "if run loadcntr; then " \
					   "run mmcboot; " \
				   "else run netboot; " \
				   "fi; " \
		   "else " \
			   "if run loadimage; then " \
				   "run mmcboot; " \
			   "else run netboot; " \
			   "fi; " \
		   "fi; " \
		   "fi; " \
	   "else booti ${loadaddr} - ${fdt_addr}; fi"

/* Link Definitions */

/* On LPDDR4 board, USDHC1 is for eMMC, USDHC2 is for SD on CPU board */

#define CFG_SYS_SDRAM_BASE		0x80000000
#define PHYS_SDRAM_1			0x80000000
#define PHYS_SDRAM_2			0x880000000
#define PHYS_SDRAM_1_SIZE		0x80000000	/* 2 GB */
#define PHYS_SDRAM_2_SIZE		0x00000000	/* 0 GB */

/* Misc configuration */
#define PHY_ANEG_TIMEOUT 20000

#endif /* __PHYCORE_IMX8X_H */
