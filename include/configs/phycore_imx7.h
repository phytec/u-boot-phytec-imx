/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2017-2021 PHYTEC America, LLC
 */

#ifndef __PHYCORE_IMX7_H
#define __PHYCORE_IMX7_H

#include "mx7_common.h"
#include "phycore_rauc_env.h"

#ifdef CONFIG_PCM_061_MT41K64M16TW107IT
#define PHYS_SDRAM_SIZE			SZ_256M
#elif defined CONFIG_PCM_061_MT41K128M16JT125IT
#define PHYS_SDRAM_SIZE			SZ_512M
#elif defined CONFIG_PCM_061_MT41K256M16TW107IT
#define PHYS_SDRAM_SIZE			SZ_1G
#else
#define PHYS_SDRAM_SIZE			SZ_1G
#endif

#define CONFIG_MXC_UART_BASE		UART1_IPS_BASE_ADDR

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(32 * SZ_1M)

/* ATAGs */
#define CONFIG_CMDLINE_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_INITRD_TAG
#define CONFIG_REVISION_TAG

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE

/* Network */
#define CONFIG_FEC_XCV_TYPE             RGMII
#define CONFIG_FEC_ENET_DEV             0

/* ENET1 */
#if (CONFIG_FEC_ENET_DEV == 0)
#define IMX_FEC_BASE			ENET_IPS_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR		1
#define CONFIG_ETHPRIME			"eth0"
#elif (CONFIG_FEC_ENET_DEV == 1)
#define IMX_FEC_BASE			ENET2_IPS_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR		2
#define CONFIG_ETHPRIME			"eth1"
#endif

#define CONFIG_FEC_MXC_MDIO_BASE	ENET_IPS_BASE_ADDR

/* I2C configs */
#define CONFIG_SYS_I2C_MXC
#define CONFIG_SYS_I2C_SPEED		100000

#ifdef CONFIG_FSL_QSPI
/* Set to QSPI1 A flash at default */
#define CONFIG_SYS_AUXCORE_BOOTDATA	0x60100000
#else
/* Set to TCML address */
#define CONFIG_SYS_AUXCORE_BOOTDATA	0x7F8000
#endif

#ifdef CONFIG_IMX_BOOTAUX
#define UPDATE_M4_ENV \
	"m4image=m4_qspi.bin\0" \
	"loadm4image=" \
		"fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${m4image}\0" \
	"update_m4_from_sd=" \
		"if sf probe 0:0; then " \
			"if run loadm4image; then " \
				"setexpr fw_sz ${filesize} + 0xffff; " \
				"setexpr fw_sz ${fw_sz} / 0x10000; " \
				"setexpr fw_sz ${fw_sz} * 0x10000; " \
				"sf erase 0x0 ${fw_sz}; " \
				"sf write ${loadaddr} 0x0 ${filesize}; " \
			"fi; " \
		"fi\0" \
	"m4boot=sf probe 0:0; " \
		"bootaux " __stringify(CONFIG_SYS_AUXCORE_BOOTDATA) "\0"
#else
#define UPDATE_M4_ENV ""
#endif

#define CONFIG_SYS_MMC_IMG_LOAD_PART	1

#ifdef CONFIG_NAND_BOOT
#define MFG_NAND_PARTITION "mtdparts=gpmi-nand:64m(NAND.boot),16m(NAND.kernel),16m(NAND.dtb),-(NAND.rootfs)"
#else
#define MFG_NAND_PARTITION ""
#endif

#define CONFIG_MFG_ENV_SETTINGS \
	"mfgtool_args=setenv bootargs console=${console},${baudrate} " \
		"rdinit=/linuxrc " \
		"g_mass_storage.stall=0 g_mass_storage.removable=1 " \
		"g_mass_storage.idVendor=0x066F " \
		"g_mass_storage.idProduct=0x37FF " \
		"g_mass_storage.iSerialNumber=\"\" " \
		MFG_NAND_PARTITION \
		"clk_ignore_unused " \
		"\0" \
	"initrd_addr=0x83800000\0" \
	"initrd_high=0xffffffff\0" \
	"bootcmd_mfg=run mfgtool_args;" \
		"bootz ${loadaddr} ${initrd_addr} ${fdt_addr};\0" \

#define CONFIG_EXTRA_ENV_SETTINGS \
	CONFIG_MFG_ENV_SETTINGS \
	UPDATE_M4_ENV \
	"script=boot.scr\0" \
	"image=zImage\0" \
	"console=ttymxc0\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"fdt_file=imx7d-phyboard-zeta-kit.dtb\0" \
	"fdt_addr=0x83000000\0" \
	"boot_fdt=try\0" \
	"ip_dyn=yes\0" \
	"mmcdev=" __stringify(CONFIG_SYS_MMC_ENV_DEV) "\0" \
	"mmcpart=" __stringify(CONFIG_SYS_MMC_IMG_LOAD_PART) "\0" \
	"mmcroot=" CONFIG_MMCROOT " rootwait rw\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} " \
		"root=${mmcroot} " \
		MFG_NAND_PARTITION \
		"\0" \
	"loadbootscript=" \
		"fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"mmc dev ${mmcdev}; if mmc rescan; then " \
			"env exists doraucboot || setenv doraucboot 0 && saveenv;" \
			"if test ${doraucboot} = 1; then " \
				"run raucboot; " \
			"elif run loadbootscript; then " \
				"run bootscript; " \
			"else " \
				"run mmcbootcmd; " \
			"fi; " \
		"fi;\0" \
	"mmcbootcmd=run loadimage; "\
		"run mmcargs; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if run loadfdt; then " \
				"bootz ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"if test ${boot_fdt} = try; then " \
					"bootz; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi; " \
		"else " \
			"bootz; " \
		"fi;\0" \
	"nandroot=ubi0:rootfs rootfstype=ubifs \0" \
	"nandargs=setenv bootargs console=${console},${baudrate} " \
		"ubi.mtd=rootfs " \
		"root=${nandroot} " \
		MFG_NAND_PARTITION \
		"\0" \
	"nandboot=echo Booting from nand ...; " \
		"run nandargs; " \
		"nand read ${loadaddr} 0x4000000 0x1000000;"\
		"nand read ${fdt_addr} 0x5000000 0x100000;"\
		"bootz ${loadaddr} - ${fdt_addr}\0" \
	"netargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/nfs " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"run netargs; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${image}; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if ${get_cmd} ${fdt_addr} ${fdt_file}; then " \
				"bootz ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"if test ${boot_fdt} = try; then " \
					"bootz; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi; " \
		"else " \
			"bootz; " \
		"fi;\0" \
	"raucdev=0\0" \
	PHYCORE_RAUC_ENV_BOOTLOGIC

#ifdef CONFIG_ENV_WRITEABLE_LIST
/* Set environment flag validation to RAUC's list of env vars that must writable */
#define CONFIG_ENV_FLAGS_LIST_STATIC	RAUC_REQUIRED_WRITABLE_ENV_FLAGS
#endif

#ifdef CONFIG_NAND_BOOT
#define CONFIG_BOOTCOMMAND \
	"run nandboot"
#else
#define CONFIG_BOOTCOMMAND \
	"run mmcboot"
#endif

#define CONFIG_SYS_MEMTEST_START	0x80000000
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_MEMTEST_START + 0x20000000)

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR
#define CONFIG_SYS_HZ			1000

/* Physical Memory Map */
#define PHYS_SDRAM			MMDC0_ARB_BASE_ADDR

#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM
#define CONFIG_SYS_INIT_RAM_ADDR	IRAM_BASE_ADDR
#define CONFIG_SYS_INIT_RAM_SIZE	IRAM_SIZE

#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

#ifdef CONFIG_NAND_MXS

/* NAND stuff */
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_BASE		0x40000000
#define CONFIG_SYS_NAND_5_ADDR_CYCLE
#define CONFIG_SYS_NAND_ONFI_DETECTION
#define CONFIG_SYS_NAND_USE_FLASH_BBT
#endif

#ifdef CONFIG_FSL_QSPI
#define CONFIG_SYS_FSL_QSPI_AHB
#define QSPI0_BASE_ADDR			QSPI1_IPS_BASE_ADDR
#define QSPI0_AMBA_BASE			QSPI0_ARB_BASE_ADDR

#define FSL_QSPI_FLASH_NUM		1
#define FSL_QSPI_FLASH_SIZE		SZ_16M
#endif

#ifdef CONFIG_NAND_MXS
#define CONFIG_SYS_FSL_USDHC_NUM	1
#else
#define CONFIG_SYS_FSL_USDHC_NUM	2
#endif

/* MMC Configs */
#define CONFIG_SYS_FSL_ESDHC_ADDR	0
#define CONFIG_SYS_MMC_ENV_DEV		0			/* USDHC1 */
#define CONFIG_SYS_MMC_ENV_PART		0			/* user area */
#define CONFIG_MMCROOT			"/dev/mmcblk0p2"	/* USDHC1 */

/* USB Configs */
#define CONFIG_MXC_USB_PORTSC		(PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_USB_MAX_CONTROLLER_COUNT	2

#define CONFIG_IMX_THERMAL

#endif /* __PHYCORE_IMX7_H */
