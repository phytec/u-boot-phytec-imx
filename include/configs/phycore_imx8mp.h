/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#ifndef __PHYCORE_IMX8MP_H
#define __PHYCORE_IMX8MP_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>

#include "phycore_rauc_env.h"
#include "phycore_fitimage_env.h"

#define CONFIG_SYS_BOOTM_LEN		SZ_64M

#define CONFIG_SPL_MAX_SIZE		(152 * SZ_1K)
#define CONFIG_SYS_MONITOR_LEN		SZ_512K
#define CONFIG_SYS_UBOOT_BASE \
		(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#ifdef CONFIG_SPL_BUILD
#define CONFIG_SPL_STACK		0x960000
#define CONFIG_SPL_BSS_START_ADDR	0x98FC00
#define CONFIG_SPL_BSS_MAX_SIZE		SZ_1K
#define CONFIG_SYS_SPL_MALLOC_START	0x42200000
#define CONFIG_SYS_SPL_MALLOC_SIZE	SZ_512K

#define CONFIG_SPL_ABORT_ON_RAW_IMAGE

#define CONFIG_POWER_PCA9450

#endif

#define CONFIG_EXTRA_ENV_SETTINGS \
	"image=Image\0" \
	"console=ttymxc0\0" \
	"bootenv=bootenv.txt\0" \
	"bootenv_addr=0x49100000\0" \
	"fdt_addr=0x48000000\0" \
	"fdt_high=0xffffffffffffffff\0" \
	"fdto_addr=0x49000000\0" \
	"fdt_file=" CONFIG_DEFAULT_FDT_FILE "\0" \
	"ipaddr=192.168.3.11\0" \
	"serverip=192.168.3.10\0" \
	"netmask=255.255.255.0\0" \
	"ip_dyn=no\0" \
	"prepare_mcore=setenv mcore_clk clk-imx8mp.mcore_booted;\0" \
	"mtdparts=30bb0000.spi:3840k(u-boot),128k(env),128k(env_redund),-(none)\0" \
	"mtdids=nor0=30bb0000.spi\0" \
	"spiprobe=true\0" \
	"emmc_dev=2\0" \
	"sd_dev=1\0" \
	"mmcdev=" __stringify(CONFIG_SYS_MMC_ENV_DEV) "\0" \
	"mmcpart=1\0" \
	"mmcroot=2\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs ${mcore_clk} console=${console},${baudrate} " \
		"root=/dev/mmcblk${mmcdev}p${mmcroot} fsck.repair=yes rootwait rw\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"mmc_load_bootenv=fatload mmc ${mmcdev}:${mmcpart} ${bootenv_addr} ${bootenv}\0" \
	"mmc_load_overlay=fatload mmc ${mmcdev}:${mmcpart} ${fdto_addr} ${overlay}\0" \
	"mmc_apply_overlays=fdt address ${fdt_addr}; "  \
		"if test ${no_extensions} = 0; then " \
			"setenv extension_overlay_addr ${fdto_addr}; " \
			"setenv extension_overlay_cmd \'fatload mmc ${mmcdev}:${mmcpart} " \
				"${fdto_addr} ${extension_overlay_name}\'; " \
			"extension scan; " \
			"extension apply all; " \
		"fi; " \
		"if test ${no_overlays} = 0; then " \
			"for overlay in ${overlays}; " \
			"do; " \
				"if run mmc_load_overlay; then " \
					"fdt resize ${filesize}; " \
					"fdt apply ${fdto_addr}; " \
				"fi; " \
			"done;" \
		"fi;\0 " \
	"mmcboot=echo Booting from mmc ...; " \
		"run spiprobe; " \
		"if test ${no_bootenv} = 0; then " \
			"if run mmc_load_bootenv; then " \
				"env import -t ${bootenv_addr} ${filesize}; " \
			"fi; " \
		"fi; " \
		"run mmcargs; " \
		"run fit_test_and_run_boot; " \
		"if run loadfdt; then " \
			"run mmc_apply_overlays; " \
			"booti ${loadaddr} - ${fdt_addr}; " \
		"else " \
			"echo WARN: Cannot load the DT; " \
		"fi;\0 " \
	"nfsroot=/nfs\0" \
	"netargs=setenv bootargs ${mcore_clk} console=${console},${baudrate} root=/dev/nfs ip=${nfsip} " \
		"nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"net_load_bootenv=${get_cmd} ${bootenv_addr} ${bootenv}\0" \
	"net_load_overlay=${get_cmd} ${fdto_addr} ${overlay}\0" \
	"net_apply_overlays=fdt address ${fdt_addr}; " \
		"if test ${no_extensions} = 0; then " \
			"setenv extension_overlay_addr ${fdto_addr}; " \
			"setenv extension_overlay_cmd \'${get_cmd} ${fdto_addr} " \
				"${extension_overlay_name}\'; " \
			"extension scan; " \
			"extension apply all; " \
		"fi; " \
		"if test ${no_overlays} = 0; then " \
			"for overlay in ${overlays}; " \
			"do; " \
				"if run net_load_overlay; then " \
					"fdt resize ${filesize}; " \
					"fdt apply ${fdto_addr}; " \
				"fi; " \
			"done;" \
		"fi;\0 " \
	"netboot=echo Booting from net ...; " \
		"run spiprobe; " \
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
	"raucdev=2\0" \
	PHYCORE_RAUC_ENV_BOOTLOGIC \
	PHYCORE_FITIMAGE_ENV_BOOTLOGIC

#ifdef CONFIG_ENV_WRITEABLE_LIST
/* Set environment flag validation to a list of env vars that must be writable */
#define CONFIG_ENV_FLAGS_LIST_STATIC RAUC_REQUIRED_WRITABLE_ENV_FLAGS",dofitboot:dw,mcore_clk:sw"
#endif

/* Link Definitions */

#define CONFIG_SYS_INIT_RAM_ADDR	0x40000000
#define CONFIG_SYS_INIT_RAM_SIZE	SZ_512K
#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

#define CONFIG_MMCROOT			"/dev/mmcblk2p2"  /* USDHC3 */

#define CONFIG_SYS_SDRAM_BASE		0x40000000

#define PHYS_SDRAM			0x40000000
#define PHYS_SDRAM_SIZE			0xC0000000 /* 3GB */
#define PHYS_SDRAM_2			0x100000000
#define PHYS_SDRAM_2_SIZE		SZ_1G

/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE		SZ_2K
#define CONFIG_SYS_MAXARGS		64
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

/* USDHC */
#define CONFIG_SYS_FSL_USDHC_NUM	2
#define CONFIG_SYS_FSL_ESDHC_ADDR       0

/* USB configs */
#define CONFIG_USB_MAX_CONTROLLER_COUNT         2
#define CONFIG_SERIAL_TAG

#endif /* __PHYCORE_IMX8MP_H */
