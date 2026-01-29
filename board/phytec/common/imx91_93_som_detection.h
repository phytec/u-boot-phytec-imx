/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2025 PHYTEC Messtechnik GmbH
 * Copyright (C) 2024 PHYTEC Messtechnik GmbH
 * Author: Primoz Fiser <primoz.fiser@norik.com>
 */

#ifndef _PHYTEC_IMX91_93_SOM_DETECTION_H
#define _PHYTEC_IMX91_93_SOM_DETECTION_H

#include "phytec_som_detection.h"

#define PHYCORE_IMX91_93_SOM	77
#define PHYFLEX_IMX91_93_SOM	 5

enum phytec_imx91_93_phycore_option_index {
	PHYTEC_IMX91_93_PHYCORE_OPT_DDR = 0,
	PHYTEC_IMX91_93_PHYCORE_OPT_EMMC = 1,
	PHYTEC_IMX91_93_PHYCORE_OPT_CPU = 2,
	PHYTEC_IMX91_93_PHYCORE_OPT_FREQ = 3,
	PHYTEC_IMX91_93_PHYCORE_OPT_NPU = 4,
	PHYTEC_IMX91_93_PHYCORE_OPT_DISP = 5,
	PHYTEC_IMX91_93_PHYCORE_OPT_ETH = 6,
	PHYTEC_IMX91_93_PHYCORE_OPT_FEAT = 7,
	PHYTEC_IMX91_93_PHYCORE_OPT_TEMP = 8,
	PHYTEC_IMX91_93_PHYCORE_OPT_BOOT = 9,
	PHYTEC_IMX91_93_PHYCORE_OPT_LED = 10,
	PHYTEC_IMX91_93_PHYCORE_OPT_EEPROM = 11,
};

enum phytec_imx91_93_phyflex_option_index {
	PHYTEC_IMX91_93_PHYFLEX_OPT_DDR = 0,
	PHYTEC_IMX91_93_PHYFLEX_OPT_EMMC,
	PHYTEC_IMX91_93_PHYFLEX_OPT_CPU,
	PHYTEC_IMX91_93_PHYFLEX_OPT_FREQ,
	PHYTEC_IMX91_93_PHYFLEX_OPT_NPU,
	PHYTEC_IMX91_93_PHYFLEX_OPT_ETH,
	PHYTEC_IMX91_93_PHYFLEX_OPT_RTC_CLKSRC_TEMPSENS,
	PHYTEC_IMX91_93_PHYFLEX_OPT_TEMP,
	PHYTEC_IMX91_93_PHYFLEX_OPT_BOOTMODE,
	PHYTEC_IMX91_93_PHYFLEX_OPT_IOVOLTAGE,
	PHYTEC_IMX91_93_PHYFLEX_OPT_LED_EEPROMFACTORY_FLASHHIGHSPEED,
	PHYTEC_IMX91_93_PHYFLEX_OPT_EEPROMUSER
};

enum phytec_imx91_93_voltage {
	PHYTEC_IMX91_93_VOLTAGE_INVALID = PHYTEC_EEPROM_INVAL,
	PHYTEC_IMX91_93_VOLTAGE_3V3 = 0,
	PHYTEC_IMX91_93_VOLTAGE_1V8 = 1,
};

enum phytec_imx91_93_ddr_eeprom_code {
	PHYTEC_IMX91_93_DDR_INVALID = PHYTEC_EEPROM_INVAL,
	PHYTEC_IMX91_93_LPDDR4X_512MB = 0,
	PHYTEC_IMX91_93_LPDDR4X_1GB = 1,
	PHYTEC_IMX91_93_LPDDR4X_2GB = 2,
	PHYTEC_IMX91_93_LPDDR4_512MB = 3,
	PHYTEC_IMX91_93_LPDDR4_1GB = 4,
	PHYTEC_IMX91_93_LPDDR4_2GB = 5,
};

u8 __maybe_unused phytec_imx91_93_detect(struct phytec_eeprom_data *data, u8 som_no);
u8 __maybe_unused phytec_imx91_93_get_opt(struct phytec_eeprom_data *data, int idx);
enum phytec_imx91_93_voltage __maybe_unused phytec_imx91_93_get_voltage
	(struct phytec_eeprom_data *data);

static inline u8 __maybe_unused phytec_imx91_93_phycore_get_opt(struct phytec_eeprom_data *data,
		enum phytec_imx91_93_phycore_option_index idx)
{
	return phytec_imx91_93_get_opt(data, (int)idx);
}

static inline u8 __maybe_unused phytec_imx91_93_phyflex_get_opt(struct phytec_eeprom_data *data,
		enum phytec_imx91_93_phyflex_option_index idx)
{
	return phytec_imx91_93_get_opt(data, (int)idx);
}
#endif /* _PHYTEC_IMX91_93_SOM_DETECTION_H */
