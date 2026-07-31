// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 PHYTEC Messtechnik GmbH
 */

#include <asm/arch/sys_proto.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <i2c.h>
#include <u-boot/crc.h>

#include "imx95_som_detection.h"

extern struct phytec_eeprom_data eeprom_data;

#if IS_ENABLED(CONFIG_PHYTEC_IMX95_SOM_DETECTION)

/* Check if the SoM is actually one of the following products:
 * - i.MX95
 *
 * Returns 0 in case it's a known SoM. Otherwise, returns 1.
 */
u8 __maybe_unused phytec_imx95_detect(struct phytec_eeprom_data *data)
{
	u8 som;

	if (!data)
		data = &eeprom_data;

	/* Early API revisions are not supported */
	if (!data->valid || data->payload.api_rev < PHYTEC_API_REV2)
		return 1;

	som = data->payload.data.data_api2.som_no;
	debug("%s: som id: %u\n", __func__, som);

	if (som == PHYTEC_PHYFLEX_IMX95 && is_imx95())
		return 0;

	pr_err("%s: SoM ID does not match. Wrong EEPROM data?\n", __func__);
	return 1;
}

#else

inline u8 __maybe_unused phytec_imx95_detect(struct phytec_eeprom_data *data)
{
	return 1;
}

#endif /* IS_ENABLED(CONFIG_PHYTEC_IMX95_SOM_DETECTION) */
