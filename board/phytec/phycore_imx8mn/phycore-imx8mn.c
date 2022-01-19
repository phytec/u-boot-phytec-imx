// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#include <common.h>
#include <asm/arch/sys_proto.h>
#include <asm/io.h>
#include <asm/mach-imx/boot_mode.h>
#include <env.h>
#include <extension_board.h>
#include <fdt_support.h>
#include <jffs2/load_kernel.h>
#include <malloc.h>
#include <miiphy.h>
#include <mtd_node.h>

#include "../common/imx8m_som_detection.h"

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_ADDR		0x51
#define EEPROM_ADDR_FALLBACK	0x59

#ifdef CONFIG_OF_BOARD_FIXUP
static int fdt_disable_node(void *blob, const char *compatible)
{
	int node, ret;

	node = fdt_node_offset_by_compatible(blob, -1, compatible);
	if (node < 0) {
		printf("%s: Can not find \"%s\" comaptible.\n",
		       __func__, compatible);
		return node;
	}

	ret = fdt_increase_size(blob, 5);
	if (ret) {
		printf("%s: Could not increase fdt size. Leave default.\n",
		       __func__);
		return ret;
	}

	ret = fdt_status_disabled(blob, node);

	return ret;
}

int board_fix_fdt(void *blob)
{
	int ret;
	struct phytec_eeprom_data data;
	u8 opt;

	/* We have no access to global variables here */
	ret = _phytec_eeprom_data_init(&data, 0, EEPROM_ADDR);
	if (ret) {
		_phytec_eeprom_data_init(&data, 0, EEPROM_ADDR_FALLBACK);
		if (ret)
			printf("%s: EEPROM data init failed\n", __func__);
	}

	/* Disable fspi node if SPI flash if not populated */
	opt = _phytec_get_imx8m_spi(&data);
	if (!opt) {
		ret = fdt_disable_node(blob, "nxp,imx8mm-fspi");
		if (ret)
			printf("%s: Could not disable SPI-NOR flash %i\n",
			       __func__, ret);
	}

	/* Disable fec node if eth phy is not populated */
	opt = _phytec_get_imx8m_eth(&data);
	if (!opt) {
		ret = fdt_disable_node(blob, "fsl,imx8mn-fec");
		if (ret)
			printf("%s: Could not disable fec node %i\n",
			       __func__, ret);
	}

	return 0;
}
#endif

int ft_board_setup(void *blob, struct bd_info *bd)
{
	u8 spi = phytec_get_imx8m_spi();
	/* Do nothing if no SPI is poulated or data invalid */
	if (spi == 0 || spi == PHYTEC_EEPROM_INVAL)
		return 0;

	static const struct node_info nodes[] = {
		{ "jedec,spi-nor", MTD_DEV_TYPE_NOR, },
	};

	fdt_fixup_mtdparts(blob, nodes, ARRAY_SIZE(nodes));

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
	int ret;

	ret = phytec_eeprom_data_init(0, EEPROM_ADDR);
	if (ret) {
		ret = phytec_eeprom_data_init(0, EEPROM_ADDR_FALLBACK);
		if (ret)
			printf("%s: EEPROM data init failed\n", __func__);
	}

	setup_fec();

	return 0;
}

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int board_late_init(void)
{
	u8 spi;

	spi = phytec_get_imx8m_spi();

	if (spi != 0 && spi != PHYTEC_EEPROM_INVAL)
		env_set("spiprobe", "sf probe");

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

#ifdef CONFIG_CMD_EXTENSION
static struct extension *add_extension(const char *name, const char *overlay,
				       const char *other)
{
	struct extension *extension;

	extension = calloc(1, sizeof(struct extension));
	snprintf(extension->name, sizeof(extension->name), name);
	snprintf(extension->overlay, sizeof(extension->overlay), overlay);
	snprintf(extension->other, sizeof(extension->other), other);
	snprintf(extension->owner, sizeof(extension->owner), "PHYTEC");

	return extension;
}

int extension_board_scan(struct list_head *extension_list)
{
	struct extension *extension = NULL;
	int ret = 0;
	u8 option;

	option = phytec_get_imx8m_eth();
	if (!option) {
		extension = add_extension("phyCORE-i.MX8MN no eth phy",
					  "imx8mn-phycore-no-eth.dtbo",
					  "eth phy not populated on SoM");
		list_add_tail(&extension->list, extension_list);
		ret++;
	}

	option = phytec_get_imx8m_spi();
	if (!option) {
		extension = add_extension("phyCORE-i.MX8MN no SPI flash",
					  "imx8mn-phycore-no-spiflash.dtbo",
					  "SPI flash not populated on SoM");
		list_add_tail(&extension->list, extension_list);
		ret++;
	}

	return ret;
}
#endif
