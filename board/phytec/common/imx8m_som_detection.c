// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#include <common.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <i2c.h>
#include <i2c_eeprom.h>

#include "imx8m_som_detection.h"

struct phytec_eeprom_data eeprom_data;

static struct udevice __maybe_unused *phytec_i2c_eeprom_init(char *of_path)
{
	int ret;
	struct udevice *dev;
	ofnode eeprom;

	eeprom = ofnode_path(of_path);
	if (!ofnode_valid(eeprom)) {
		pr_err("%s: Could not find i2c EEPROM in device tree.\n",
		       __func__);
		return NULL;
	}

	ret = uclass_get_device_by_ofnode(UCLASS_I2C_EEPROM, eeprom, &dev);
	if (ret) {
		pr_err("%s: i2c EEPROM not found.\n", __func__);
		return NULL;
	}

	return dev;
}

int phytec_eeprom_data_init(char *of_path, int bus_num, int addr)
{
	int ret;

#if defined(CONFIG_DM_I2C)
	struct udevice *dev;

	dev = phytec_i2c_eeprom_init(of_path);
	if (!dev)
		return -ENODEV;

	ret = i2c_eeprom_read(dev, 0, (uint8_t *)&eeprom_data,
			      sizeof(eeprom_data));
#else
	i2c_set_bus_num(bus_num);
	ret =  i2c_read(addr, 0, 2, (uint8_t *)&eeprom_data,
			sizeof(eeprom_data));
#endif
	if (ret) {
		pr_err("%s: Unable to read EEPROM data\n", __func__);
		return ret;
	}

	if (eeprom_data.api_rev > PHYTEC_API_REV1) {
		pr_err("%s: EEPROM API revision %u not supported\n",
		       __func__,
		       eeprom_data.api_rev);
		return -EINVAL;
	}

	return 0;
}

char * __maybe_unused phytec_get_imx8m_opt(void)
{
	char *opt;

	switch (eeprom_data.api_rev) {
	case PHYTEC_API_REV0:
	case PHYTEC_API_REV1:
		opt = eeprom_data.data.data_api0.opt;
		break;
	default:
		opt = NULL;
		break;
	};

	return opt;
}

/*
 * So far all PHYTEC i.MX8M boards have RAM size definition at the
 * same location.
 */
u8 __maybe_unused phytec_get_imx8m_ddr_size(void)
{
	char *opt;
	u8 ddr_id;

	opt = phytec_get_imx8m_opt();
	if (opt)
		ddr_id = opt[2] - '0';
	else
		ddr_id = 0xff;

	debug("%s: ddr id: %u\n", __func__, ddr_id);
	return ddr_id;
}
