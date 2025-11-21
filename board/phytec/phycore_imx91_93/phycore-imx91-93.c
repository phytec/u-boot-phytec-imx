// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (C) 2025 PHYTEC Messtechnik GmbH
 * Copyright (C) 2023 PHYTEC Messtechnik GmbH
 * Author: Christoph Stoidner <c.stoidner@phytec.de>
 * Copyright (C) 2024 Mathieu Othacehe <m.othacehe@gmail.com>
 * Copyright (C) 2024 PHYTEC Messtechnik GmbH
 */

#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/mach-imx/boot_mode.h>
#include <env.h>
#include <fdt_support.h>
#include <phy.h>

#include "../common/imx91_93_som_detection.h"

DECLARE_GLOBAL_DATA_PTR;

#define EEPROM_ADDR            0x50

int board_init(void)
{
	int ret = phytec_eeprom_data_setup(NULL, 2, EEPROM_ADDR);

	if (ret)
		printf("%s: EEPROM data init failed\n", __func__);

	return 0;
}

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int board_late_init(void)
{
	switch (get_boot_device()) {
	case SD2_BOOT:
		env_set_ulong("mmcdev", 1);
		env_set("boot_targets", "mmc1 mmc0 usb ethernet");
		break;
	case MMC1_BOOT:
		env_set_ulong("mmcdev", 0);
		env_set("boot_targets", "mmc0 mmc1 usb ethernet");
		break;
	default:
		break;
	}

	return 0;
}

#define DP83822_DEVADDR		0x1f

static void dp8382x_phy_fixup(struct phy_device *phydev)
{
	int pid;

	/* Read PHYIDR2 for model number */
	pid = phy_read_mmd(phydev, DP83822_DEVADDR, 0x3);

	/* DP83825I fixup */
	if (((pid >> 4) & 0x3F) == 0x14) {
		/* AutoNeg enable */
		phy_set_bits_mmd(phydev, DP83822_DEVADDR, 0x0, BIT(12));
		/* CRS_DV: enable */
		phy_clear_bits_mmd(phydev, DP83822_DEVADDR, 0x302, BIT(8));
		/* LED_2_Polarity: active low/high */
		if (IS_ENABLED(CONFIG_PHYCORE_IMX91_93_ETHPHYLED_ACTIVELOW))
			phy_clear_bits_mmd(phydev, DP83822_DEVADDR, 0x469, BIT(6));
		else if (IS_ENABLED(CONFIG_PHYCORE_IMX91_93_ETHPHYLED_ACTIVEHIGH))
			phy_set_bits_mmd(phydev, DP83822_DEVADDR, 0x469, BIT(6));
		/* LED_2_Control: RX/TX act */
		phy_clear_bits_mmd(phydev, DP83822_DEVADDR, 0x460, GENMASK(7, 4));
		phy_set_bits_mmd(phydev, DP83822_DEVADDR, 0x460, BIT(4));
		/* LED_0_Configuration: Link OK, cfg_mled_en: LED_0 */
		phy_clear_bits_mmd(phydev, DP83822_DEVADDR, 0x25, GENMASK(6, 3));
		phy_set_bits_mmd(phydev, DP83822_DEVADDR, 0x25, BIT(0));
		/* LED_Link_Polarity : active low/high */
		if (IS_ENABLED(CONFIG_PHYCORE_IMX91_93_ETHPHYLED_ACTIVELOW))
			phy_clear_bits_mmd(phydev, DP83822_DEVADDR, 0x18, BIT(7));
		else if (IS_ENABLED(CONFIG_PHYCORE_IMX91_93_ETHPHYLED_ACTIVEHIGH))
			phy_set_bits_mmd(phydev, DP83822_DEVADDR, 0x18, BIT(7));
	}

	/* DP83826I fixup */
	if (((pid >> 4) & 0x3F) == 0x11) {
		/* Swap LEDs configuration */
		phy_clear_bits_mmd(phydev, DP83822_DEVADDR, 0x460, GENMASK(3, 0));
		phy_set_bits_mmd(phydev, DP83822_DEVADDR, 0x460, BIT(0));
		phy_clear_bits_mmd(phydev, DP83822_DEVADDR, 0x25, GENMASK(6, 3));
	}
}

int board_phy_config(struct phy_device *phydev)
{
	u8 option = phytec_imx91_93_get_opt(NULL, PHYTEC_IMX91_93_OPT_ETH);

	if (!option)
		return 0;

	dp8382x_phy_fixup(phydev);

	if (phydev->drv->config)
		return phydev->drv->config(phydev);

	return 0;
}

static void emmc_fixup(void *blob, struct phytec_eeprom_data *data)
{
	enum phytec_imx91_93_voltage voltage = phytec_imx91_93_get_voltage(data);
	u8 option = phytec_imx91_93_get_opt(data, PHYTEC_IMX91_93_OPT_EMMC);
	int ret, offset;

	offset = fdt_node_offset_by_compat_reg(blob, "fsl,imx93-usdhc", 0x42850000);
	if (offset < 0) {
		printf("%s: failed to find eMMC node offset\n", __func__);
		return;
	}

	if (!option) {
		ret = fdt_status_disabled(blob, offset);
		if (ret < 0)
			printf("%s: failed to disable eMMC node\n", __func__);
		else
			return;
	}

	if (voltage == PHYTEC_IMX91_93_VOLTAGE_INVALID) {
		printf("%s: invalid voltage, skipping\n", __func__);
		return;
	}

	if (voltage == PHYTEC_IMX91_93_VOLTAGE_1V8) {
		ret = fdt_delprop(blob, offset, "no-1-8-v");
		if (ret < 0)
			printf("%s: failed to delete \"no-1-8-v\" property\n", __func__);
	}
}

static void ethphy_fixup(void *blob, struct phytec_eeprom_data *data)
{
	const char *path = "/soc@0/bus@42800000/ethernet@42890000/mdio/ethernet-phy@1";
	u8 option = phytec_imx91_93_get_opt(data, PHYTEC_IMX91_93_OPT_ETH);
	u8 pcb_rev = phytec_get_rev(data);
	int ret, offset;

	offset = fdt_path_offset(blob, path);
	if (offset < 0) {
		printf("%s: failed to find eth phy offset %s\n", __func__, path);
		return;
	}

	if (!option) {
		ret = fdt_status_disabled(blob, offset);
		if (ret < 0)
			printf("%s: failed to disable eth phy %s\n", __func__, path);
		else
			return;
	}

	if (pcb_rev == PHYTEC_EEPROM_INVAL) {
		printf("%s: invalid PCB revision, skipping\n", __func__);
		return;
	}

	if (pcb_rev < 4) {
		ret = fdt_delprop(blob, offset, "reset-gpios");
		if (ret < 0)
			printf("%s: failed to delete \"reset-gpios\" property\n", __func__);
	}
}

int board_fix_fdt(void *blob)
{
	struct phytec_eeprom_data data;

	phytec_eeprom_data_setup(&data, 2, EEPROM_ADDR);

	emmc_fixup(blob, &data);

	ethphy_fixup(blob, &data);

	/* Update dtb clocks for low drive mode */
	if (is_voltage_mode(VOLT_LOW_DRIVE))
		low_drive_freq_update(blob, LOW_DRIVE_TABLE_UBOOT);

	return 0;
}

int ft_board_setup(void *blob, struct bd_info *bd)
{
	emmc_fixup(blob, NULL);

	ethphy_fixup(blob, NULL);

	/**
	 * NOTE: VOLT_LOW_DRIVE fixup is done by the ft_system_setup()
	 * in arch/arm/mach-imx/imx9/soc.c for Linux device-tree.
	 */

	return 0;
}
