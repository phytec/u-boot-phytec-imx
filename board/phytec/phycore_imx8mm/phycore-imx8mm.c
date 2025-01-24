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
#include <extension_board.h>
#include <init.h>
#include <fdt_support.h>
#include <jffs2/load_kernel.h>
#include <miiphy.h>
#include <mtd_node.h>
#include <usb.h>

#include "../common/imx8m_som_detection.h"

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_ADDR		0x51
#define EEPROM_ADDR_FALLBACK	0x59

int ft_board_setup(void *blob, struct bd_info *bd)
{
	u8 spi = phytec_get_imx8m_spi(NULL);
	/* Do nothing if no SPI is populated */
	if (!spi)
		return 0;

	static const struct node_info nodes[] = {
		{ "jedec,spi-nor", MTD_DEV_TYPE_NOR, },
	};

	fdt_fixup_mtdparts(blob, nodes, ARRAY_SIZE(nodes));

	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	imx8m_usb_power(index, true);
	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	imx8m_usb_power(index, false);
	return 0;
}

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
	int ret = phytec_eeprom_data_setup_fallback(NULL, 0,
			EEPROM_ADDR, EEPROM_ADDR_FALLBACK);
	if (ret)
		printf("%s: EEPROM data init failed\n", __func__);

	setup_fec();

	return 0;
}

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no;
}

void phytec_set_fit_extensions(void)
{
	char fit_extensions[128] = {};

	if (!phytec_get_imx8m_eth(NULL))
		sprintf(fit_extensions, "%s%s", fit_extensions,
			"#conf-imx8mm-phycore-no-eth.dtbo");

	if (!phytec_get_imx8m_spi(NULL))
		sprintf(fit_extensions, "%s%s", fit_extensions,
			"#conf-imx8mm-phycore-no-spiflash.dtbo");

	env_set("fit_extensions", fit_extensions);
}

int board_late_init(void)
{
	u8 spi = phytec_get_imx8m_spi(NULL);

	if (spi != 0 && spi != PHYTEC_EEPROM_INVAL)
		env_set("spiprobe", "sf probe");

	switch (get_boot_device()) {
	case SD2_BOOT:
		env_set_ulong("mmcdev", 1);
		env_set("boot_targets", "mmc1 mmc2 usb ethernet");
		break;
	case MMC3_BOOT:
		env_set_ulong("mmcdev", 2);
		env_set("boot_targets", "mmc2 mmc1 usb ethernet");
		break;
	default:
		break;
	}
	phytec_set_fit_extensions();

	return 0;
}

#ifdef CONFIG_CMD_EXTENSION
int extension_board_scan(struct list_head *extension_list)
{
	struct extension *extension = NULL;
	int ret = 0;
	u8 option;

	option = phytec_get_imx8m_eth(NULL);
	if (!option) {
		extension = phytec_add_extension("phyCORE-i.MX8MM no eth phy",
						 "imx8mm-phycore-no-eth.dtbo",
						 "eth phy not populated on SoM");
		list_add_tail(&extension->list, extension_list);
		ret++;
	}

	option = phytec_get_imx8m_spi(NULL);
	if (!option) {
		extension = phytec_add_extension("phyCORE-i.MX8MM no SPI flash",
						 "imx8mm-phycore-no-spiflash.dtbo",
						 "SPI flash not populated on SoM");
		list_add_tail(&extension->list, extension_list);
		ret++;
	}

	return ret;
}
#endif

enum env_location env_get_location(enum env_operation op, int prio)
{
	enum boot_device dev = get_boot_device();
	enum env_location env_loc = ENVL_UNKNOWN;

	if (prio > 0)
		return env_loc;

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

int board_phys_sdram_size(phys_size_t *size)
{
	if (!size)
		return -EINVAL;

	*size = get_ram_size((void *)PHYS_SDRAM, PHYS_SDRAM_SIZE);

	return 0;
}
