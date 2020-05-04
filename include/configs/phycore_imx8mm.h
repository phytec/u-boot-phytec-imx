/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2019 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#ifndef __PCL069_H
#define __PCL069_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>

#include "imx_env.h"

#ifdef CONFIG_SECURE_BOOT
#define CONFIG_CSF_SIZE			SZ_8K
#endif

#define CONFIG_SYS_BOOTM_LEN		SZ_64M

#define CONFIG_SPL_MAX_SIZE				(148 * SZ_1K)
#define CONFIG_SYS_MONITOR_LEN				SZ_512K
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_USE_SECTOR
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR		0x300
#define CONFIG_SYS_MMCSD_FS_BOOT_PARTITION		1
#define CONFIG_SYS_UBOOT_BASE \
		(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#ifdef CONFIG_SPL_BUILD
/*#define CONFIG_ENABLE_DDR_TRAINING_DEBUG*/
#define CONFIG_SPL_WATCHDOG_SUPPORT
#define CONFIG_SPL_POWER_SUPPORT
#define CONFIG_SPL_DRIVERS_MISC_SUPPORT
#define CONFIG_SPL_I2C_SUPPORT
#define CONFIG_SPL_LDSCRIPT		"arch/arm/cpu/armv8/u-boot-spl.lds"
#define CONFIG_SPL_STACK		0x91fff0
#define CONFIG_SPL_LIBCOMMON_SUPPORT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_GPIO_SUPPORT
#define CONFIG_SPL_BSS_START_ADDR	0x00910000
#define CONFIG_SPL_BSS_MAX_SIZE		SZ_8K
#define CONFIG_SYS_SPL_MALLOC_START	0x42200000
#define CONFIG_SYS_SPL_MALLOC_SIZE	SZ_512K
#define CONFIG_SYS_ICACHE_OFF
#define CONFIG_SYS_DCACHE_OFF

#define CONFIG_MALLOC_F_ADDR		0x912000 /* malloc f used before GD_FLG_FULL_MALLOC_INIT set */

#define CONFIG_SPL_ABORT_ON_RAW_IMAGE /* For RAW image gives a error info not panic */

#undef CONFIG_DM_MMC

#define CONFIG_POWER
#define CONFIG_POWER_I2C

#define CONFIG_SYS_I2C
#define CONFIG_SYS_I2C_MXC_I2C1		/* enable I2C bus 1 */
#define CONFIG_SYS_I2C_MXC_I2C2		/* enable I2C bus 2 */
#define CONFIG_SYS_I2C_MXC_I2C3		/* enable I2C bus 3 */

#define CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG

#endif

#define CONFIG_CMD_READ
#define CONFIG_SERIAL_TAG
#define CONFIG_FASTBOOT_USB_DEV		0

#define CONFIG_REMAKE_ELF

#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_BOARD_LATE_INIT

#undef CONFIG_CMD_EXPORTENV
#undef CONFIG_CMD_IMPORTENV
#undef CONFIG_CMD_IMLS

#undef CONFIG_CMD_CRC32
#undef CONFIG_BOOTM_NETBSD

/* ENET Config */
/* ENET1 */
#if defined(CONFIG_CMD_NET)
#define CONFIG_CMD_PING
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_MII
#define CONFIG_MII
#define CONFIG_ETHPRIME			"FEC"

#define FEC_QUIRK_ENET_MAC

#define CONFIG_PHY_GIGE
#define IMX_FEC_BASE			0x30BE0000
#endif

#define CONFIG_WATCHDOG_TIMEOUT_MSECS	60000

#define CONFIG_MFG_ENV_SETTINGS \
	CONFIG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x43800000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=2\0" \
	"sd_dev=1\0" \

#define CONFIG_EXTRA_ENV_SETTINGS \
	CONFIG_MFG_ENV_SETTINGS \
	"script=boot.scr\0" \
	"image=Image\0" \
	"console=ttymxc2,115200\0" \
	"fdt_addr=0x43000000\0" \
	"fdt_high=0xffffffffffffffff\0" \
	"boot_fdt=try\0" \
	"fdt_file=" CONFIG_DEFAULT_FDT_FILE "\0" \
	"initrd_addr=0x43800000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"ipaddr=192.168.3.11\0" \
	"serverip=192.168.3.10\0" \
	"netmask=255.225.255.0\0" \
	"ip_dyn=no\0" \
	"mtdparts=30bb0000.flexspi:4M(u-boot),1M(env),40M(kernel),1M(oftree)\0" \
	"mtdids=nor0=30bb0000.flexspi\0" \
	"mmcdev=" __stringify(CONFIG_SYS_MMC_ENV_DEV) "\0" \
	"mmcpart=" __stringify(CONFIG_SYS_MMC_IMG_LOAD_PART) "\0" \
	"mmcroot=2\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs console=${console} " \
		"root=/dev/mmcblk${mmcdev}p${mmcroot} rootwait rw\0" \
	"mmcargsreset=" \
		"setenv mmcdev " __stringify(CONFIG_SYS_MMC_ENV_DEV) "; " \
		"setenv mmcpart " __stringify(CONFIG_SYS_MMC_IMG_LOAD_PART) "; " \
		"setexpr mmcroot " __stringify(CONFIG_SYS_MMC_IMG_LOAD_PART) " + 1; " \
		"setenv mmcargs setenv bootargs console=${console} " \
			"root=/dev/mmcblk${mmcdev}p${mmcroot} rootwait rw;\0" \
	"loadbootscript=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} " \
		"${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if run loadfdt; then " \
				"booti ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"echo WARN: Cannot load the DT; " \
			"fi; " \
		"else " \
			"echo wait for boot; " \
		"fi;\0" \
	"nfsroot=/nfs\0" \
	"netargs=setenv bootargs console=${console} root=/dev/nfs ip=dhcp " \
		"nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"sf probe; " \
		"run netargs; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${loadaddr} ${image}; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if ${get_cmd} ${fdt_addr} ${fdt_file}; then " \
				"booti ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"echo WARN: Cannot load the DT; " \
			"fi; " \
		"else " \
			"booti; " \
		"fi;\0" \
	"doraucboot=0\0" \
	"raucslot=system0\0" \
	"raucdev=2\0" \
	"raucrootpart=2\0" \
	"raucpart=1\0" \
	"raucargs=setenv bootargs console=${console} " \
		"root=/dev/mmcblk${raucdev}p${raucrootpart} rauc.slot=${raucslot} " \
		"rootwait rw\";\0" \
	"loadraucimage=fatload mmc ${raucdev}:${raucpart} ${loadaddr} ${image}\0" \
	"loadraucfdt=fatload mmc ${raucdev}:${raucpart} ${fdt_addr} ${fdt_file}\0" \
	"raucemmcboot=echo Booting rauc from emmc ...; " \
		"run raucargs; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if run loadraucfdt; then " \
				"booti ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"echo WARN: Cannot load the DT; " \
			"fi; " \
		"else " \
			"echo wait for boot; " \
		"fi;\0" \
	"raucboot=echo Booting A/B system ...; " \
		"test -n \"${BOOT_ORDER}\" || setenv BOOT_ORDER \"system0 system1\"; " \
		"test -n \"${BOOT_system0_LEFT}\" || setenv BOOT_system0_LEFT 3; " \
		"test -n \"${BOOT_system1_LEFT}\" || setenv BOOT_system1_LEFT 3; " \
		"setenv raucstatus; " \
		"for BOOT_SLOT in \"${BOOT_ORDER}\"; do " \
			"if test \"x${raucstatus}\" != \"x\"; then " \
				"echo Skipping remaing slots!; " \
			"elif test \"x${BOOT_SLOT}\" = \"xsystem0\"; then " \
				"if test ${BOOT_system0_LEFT} -gt 0; then " \
					"echo \"Found valid slot A, " \
						"${BOOT_system0_LEFT} attempts remaining\"; " \
					"setexpr BOOT_system0_LEFT ${BOOT_system0_LEFT} - 1; " \
					"setenv raucpart 1; " \
					"setenv raucrootpart 2; " \
					"setenv raucslot system0; " \
					"run raucargs; " \
					"setenv raucstatus success; " \
				"fi; " \
			"elif test \"x${BOOT_SLOT}\" = \"xsystem1\"; then " \
				"if test ${BOOT_system1_LEFT} -gt 0; then " \
					"echo \"Found valid slot B, " \
						"${BOOT_system1_LEFT} attempts remaining\"; " \
					"setexpr BOOT_system1_LEFT ${BOOT_system1_LEFT} - 1; " \
					"setenv raucpart 3; " \
					"setenv raucrootpart 4; " \
					"setenv raucslot system1; " \
					"run raucargs; " \
					"setenv raucstatus success; " \
				"fi; " \
			"fi; " \
		"done; " \
		"if test -n \"${raucstatus}\"; then " \
			"env delete raucstatus; " \
			"saveenv; " \
			"if run loadraucimage; then " \
				"run raucemmcboot; " \
			"fi; " \
		"else " \
			"echo \"WARN: No valid slot found\"; " \
			"setenv BOOT_system0_LEFT 3; " \
			"setenv BOOT_system1_LEFT 3; " \
			"run mmcargsreset; " \
			"env delete raucstatus; " \
			"saveenv; " \
			"reset; " \
		"fi;\0" \
	"dospiboot=0\0" \
	"spiboot=echo Booting from SPI NOR...; " \
		"setenv mmcdev 2; " \
		"run mmcargs; " \
		"sf read ${loadaddr} 0x500000 0x2800000; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if sf read ${fdt_addr} 0x2D00000 0x100000; then " \
				"booti ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"echo WARN: Cannot load the DT; " \
			"fi; " \
		"fi;\0" \

#define CONFIG_BOOTCOMMAND \
	"mmc dev ${mmcdev}; if mmc rescan; then " \
		"sf probe; " \
		"if run loadbootscript; then " \
			"run bootscript; " \
		"else " \
			"if test ${doraucboot} = 1; then " \
				"run raucboot; " \
			"elif test ${dospiboot} = 1; then " \
				"run spiboot; " \
			"elif run loadimage; then " \
				"env delete BOOT_ORDER; " \
				"env delete BOOT_system0_LEFT; " \
				"env delete BOOT_system1_LEFT; " \
				"saveenv; " \
				"run mmcboot; " \
			"else run netboot; " \
			"fi; " \
		"fi; " \
	"else booti ${loadaddr} - ${fdt_addr}; fi"

/* Link Definitions */
#define CONFIG_LOADADDR			0x40480000

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR

#define CONFIG_SYS_INIT_RAM_ADDR	0x40000000
#define CONFIG_SYS_INIT_RAM_SIZE	SZ_512K
#define CONFIG_SYS_INIT_SP_OFFSET \
		(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
		(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

#define CONFIG_ENV_OVERWRITE
#if defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_ENV_OFFSET		(SZ_4M - SZ_256K)
#define CONFIG_ENV_OFFSET_REDUND	(SZ_4M - SZ_128K)
#define CONFIG_ENV_SIZE			SZ_64K
#elif defined(CONFIG_ENV_IS_IN_SPI_FLASH)
#define CONFIG_ENV_SIZE			SZ_64K
#define CONFIG_ENV_OFFSET		SZ_4M
#define CONFIG_ENV_SECT_SIZE		SZ_1M
#define CONFIG_ENV_SPI_BUS		CONFIG_SF_DEFAULT_BUS
#define CONFIG_ENV_SPI_CS		CONFIG_SF_DEFAULT_CS
#define CONFIG_ENV_SPI_MODE		CONFIG_SF_DEFAULT_MODE
#define CONFIG_ENV_SPI_MAX_HZ		CONFIG_SF_DEFAULT_SPEED
#endif
#define CONFIG_SYS_MMC_ENV_DEV		2   /* USDHC3 */
#define CONFIG_MMCROOT			"/dev/mmcblk2p2"  /* USDHC3 */

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		((CONFIG_ENV_SIZE + SZ_2K + SZ_16K) * SZ_1K)

#define CONFIG_SYS_SDRAM_BASE		0x40000000
#define PHYS_SDRAM			SZ_1G

#define CONFIG_BAUDRATE			115200

#define CONFIG_MXC_UART
#define CONFIG_MXC_UART_BASE		UART3_BASE_ADDR

/* Monitor Command Prompt */
#undef CONFIG_SYS_PROMPT
#define CONFIG_SYS_PROMPT		"u-boot=> "
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_SYS_CBSIZE		SZ_2K
#define CONFIG_SYS_MAXARGS		64
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					 sizeof(CONFIG_SYS_PROMPT) + 16)

#define CONFIG_IMX_BOOTAUX

/* USDHC */
#define CONFIG_CMD_MMC
#define CONFIG_FSL_ESDHC
#define CONFIG_FSL_USDHC

#define CONFIG_SYS_FSL_USDHC_NUM	2
#define CONFIG_SYS_FSL_ESDHC_ADDR       0

#define CONFIG_SUPPORT_EMMC_BOOT	/* eMMC specific */
#define CONFIG_SYS_MMC_IMG_LOAD_PART	1

#ifdef CONFIG_FSL_FSPI
#define FSL_FSPI_FLASH_SIZE		SZ_64M
#define FSL_FSPI_FLASH_NUM		1
#define FSPI0_BASE_ADDR			0x30bb0000
#define FSPI0_AMBA_BASE			0x0
#define CONFIG_FSPI_QUAD_SUPPORT

#define CONFIG_SYS_FSL_FSPI_AHB
#endif

#define CONFIG_MXC_GPIO

#define CONFIG_MXC_OCOTP
#define CONFIG_CMD_FUSE

#ifndef CONFIG_DM_I2C
#define CONFIG_SYS_I2C
#endif
#define CONFIG_SYS_I2C_MXC_I2C1		/* enable I2C bus 1 */
#define CONFIG_SYS_I2C_MXC_I2C2		/* enable I2C bus 2 */
#define CONFIG_SYS_I2C_MXC_I2C3		/* enable I2C bus 3 */
#define CONFIG_SYS_I2C_SPEED		100000

/* USB configs */
#ifndef CONFIG_SPL_BUILD
#define CONFIG_USBD_HS
#endif

#define CONFIG_USB_GADGET_VBUS_DRAW	2

#define CONFIG_MXC_USB_PORTSC  (PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_USB_MAX_CONTROLLER_COUNT	2

#define CONFIG_OF_SYSTEM_SETUP

#endif
