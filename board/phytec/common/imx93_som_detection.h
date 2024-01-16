/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2024 PHYTEC Messtechnik GmbH
 * Author: Primoz Fiser <primoz.fiser@norik.com>
 */

#ifndef _PHYTEC_IMX93_SOM_DETECTION_H
#define _PHYTEC_IMX93_SOM_DETECTION_H

#include "phytec_som_detection.h"

#define PHYTEC_IMX93_SOM	77

enum phytec_imx93_option_index {
	PHYTEC_IMX93_OPT_DDR = 0,
	PHYTEC_IMX93_OPT_EMMC,
	PHYTEC_IMX93_OPT_CPU,
	PHYTEC_IMX93_OPT_FREQ,
	PHYTEC_IMX93_OPT_NPU,
	PHYTEC_IMX93_OPT_DISP,
	PHYTEC_IMX93_OPT_ETH,
	PHYTEC_IMX93_OPT_FEAT,
	PHYTEC_IMX93_OPT_TEMP,
	PHYTEC_IMX93_OPT_BOOT,
	PHYTEC_IMX93_OPT_LED,
	PHYTEC_IMX93_OPT_EEPROM,
};

u8 __maybe_unused phytec_imx93_detect(struct phytec_eeprom_data *data);
u8 __maybe_unused phytec_imx93_get_opt(struct phytec_eeprom_data *data,
		enum phytec_imx93_option_index idx);

#endif /* _PHYTEC_IMX93_SOM_DETECTION_H */
