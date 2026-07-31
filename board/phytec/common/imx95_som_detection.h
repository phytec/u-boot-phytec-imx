/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 PHYTEC Messtechnik GmbH
 */

#ifndef _PHYTEC_IMX95_SOM_DETECTION_H
#define _PHYTEC_IMX95_SOM_DETECTION_H

#include "phytec_som_detection.h"

#define PHYTEC_PHYFLEX_IMX95	2

u8 __maybe_unused phytec_imx95_detect(struct phytec_eeprom_data *data);

#endif /* _PHYTEC_IMX95_SOM_DETECTION_H */
