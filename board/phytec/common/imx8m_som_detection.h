/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2023 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#ifndef _PHYTEC_IMX8M_SOM_DETECTION_H
#define _PHYTEC_IMX8M_SOM_DETECTION_H

#include "phytec_som_detection.h"

#define PHYTEC_IMX8MQ_SOM       66
#define PHYTEC_IMX8MM_SOM       69
#define PHYTEC_IMX8MP_SOM       70

enum phytec_imx8mp_ddr_eeprom_code {
	PHYTEC_IMX8MP_DDR_1GB = 2,
	PHYTEC_IMX8MP_DDR_2GB = 3,
	PHYTEC_IMX8MP_DDR_4GB = 5,
	PHYTEC_IMX8MP_DDR_8GB = 7,
	PHYTEC_IMX8MP_DDR_4GB_2GHZ = 8,
};

enum phytec_imx8mm_ddr_eeprom_code {
	INVALID = PHYTEC_EEPROM_INVAL,
	PHYTEC_IMX8MM_DDR_1GB = 1,
	PHYTEC_IMX8MM_DDR_2GB = 3,
	PHYTEC_IMX8MM_DDR_4GB = 5,
};

int __maybe_unused phytec_imx8m_detect(struct phytec_eeprom_data *data);
u8 __maybe_unused phytec_get_imx8m_ddr_size(struct phytec_eeprom_data *data);
u8 __maybe_unused phytec_get_imx8mp_rtc(struct phytec_eeprom_data *data);
u8 __maybe_unused phytec_get_imx8m_spi(struct phytec_eeprom_data *data);
u8 __maybe_unused phytec_get_imx8m_eth(struct phytec_eeprom_data *data);

#endif /* _PHYTEC_IMX8M_SOM_DETECTION_H */
