/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2023 PHYTEC Messtechnik GmbH
 * Author: Benjamin Hahn <B.Hahn@phytec.de>
 */

#ifndef __PHYCORE_EFI_ENV_H
#define __PHYCORE_EFI_ENV_H

/*
 * Note:
 *
 * These are environment variables and scripts that help booting EFI
 * binaries on PHYTEC's phyCORE SoMs.
 *
 * Implementation and Usage:
 *
 * 1. Include this file in your include/configs/SoM.h configuration.
 * 2. Set the following variables in the U-Boot environment:
 *    a. "imx_boot_offset" to the offset of the imx_boot partition from
 *       the SoM. For example "0x40" for the i.MX8M Plus.
 *    b. "efi_boot_targets" the list of boot targets in the order to be
 *       scanned. For example "mmc0 mmc1 usb0".
 * 3. Include the macro PHYCORE_EFI_ENV_BOOTLOGIC environment to add the
 *    bootlogic for booting EFI binaries.
 * 4. Make sure CONFIG_BOOTCOMMAND properly boots EFI binaries by executing
 *    "run efi_bootcmd".
 */

#define PHYCORE_EFI_ENV_BOOTLOGIC \
	"efi_bootcmd=" \
		"for target in ${efi_boot_targets}; do " \
			"run efi_bootcmd_${target}; " \
		"done;\0" \
	"efi_bootcmd_mmc1=" \
		"devnum=1; " \
		"run efi_mmc_boot;\0" \
	"efi_bootcmd_mmc2=" \
		"devnum=2; " \
		"run efi_mmc_boot;\0" \
	"efi_bootcmd_usb0=" \
		"devnum=0; " \
		"run efi_usb_boot;\0" \
	"efi_mmc_boot=" \
		"if mmc dev ${devnum}; then " \
			"setenv devtype mmc; " \
			"run scan_dev_for_efi_boot_part; " \
		"fi;\0" \
	"efi_usb_boot=" \
		"usb start; " \
		"if usb dev ${devnum}; then " \
			"setenv devtype usb; " \
			"run scan_dev_for_efi_boot_part; " \
		"fi;\0" \
	"scan_dev_for_efi_boot_part=" \
		"part list ${devtype} ${devnum} -bootable devplist; " \
		"env exists devplist || setenv devplist 1; " \
		"for distro_bootpart in ${devplist}; do " \
			"run scan_dev_for_efi_boot; " \
		"done; " \
		"setenv devplist;\0" \
	"scan_dev_for_efi_boot=" \
		"echo Scanning ${devtype} ${devnum}:${distro_bootpart}...; " \
		"run scan_dev_for_efi;\0" \
	"scan_dev_for_efi= " \
		"run boot_efi_bootmgr; " \
		"if test -e ${devtype} ${devnum}:${distro_bootpart} efi/boot/bootaa64.efi; then " \
			"echo Found EFI binary efi/boot/bootaa64.efi; " \
			"run boot_efi_binary; " \
			"echo EFI LOAD FAILED: continuing...; " \
		"fi;\0" \
	"boot_efi_bootmgr=" \
		"env set dfu_alt_info \"${devtype} ${mmcdev}=${mmcpart} raw ${imx_boot_offset} 0x2000\"; " \
		"efidebug boot add 0 Boot0000 ${devtype} " \
		"${devnum}:${distro_bootpart} capsule1.bin; " \
		"efidebug boot next 0; " \
		"setenv -e -nv -bs -rt -v OsIndications =0x04; " \
		"bootefi bootmgr;\0" \
	"boot_efi_binary=" \
		"env set dfu_alt_info \"${devtype} ${mmcdev}=${mmcpart} raw ${imx_boot_offset} 0x2000\"; " \
		"efidebug boot add 0 Boot0000 ${devtype} " \
		"${devnum}:${distro_bootpart} capsule1.bin; " \
		"efidebug boot next 0; " \
		"setenv -e -nv -bs -rt -v OsIndications =0x04; " \
		"fatload ${devtype} ${devnum}:${distro_bootpart} " \
		"${loadaddr} efi/boot/bootaa64.efi; " \
		"bootefi ${loadaddr} ${fdtcontroladdr};\0" \

#endif /* __PHYCORE_EFI_ENV_H */
