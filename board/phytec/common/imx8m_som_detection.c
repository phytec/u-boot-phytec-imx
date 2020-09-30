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

#if !defined(CONFIG_DM_I2C)
int get_imx8m_ddr_size(int bus_num, int addr)
{
	uint8_t var;
	int ret;

	i2c_set_bus_num(bus_num);
	ret = i2c_read(addr, 6, 2, &var, sizeof(var));
	if (ret < 0) {
		printf("Unable to read from i2c EEPROM\n");
		return ret;
	}

	return (int)var - 48;
};

#else

static struct udevice *phytec_i2c_eeprom_init(char *of_path)
{
	struct udevice *dev;
	ofnode eeprom;
	int ret;

	eeprom = ofnode_path(of_path);
	if (!ofnode_valid(eeprom)) {
		printf("Could not find i2c EEPROM in device tree.\n");
		return NULL;
	}

	ret = uclass_get_device_by_ofnode(UCLASS_I2C_EEPROM, eeprom, &dev);
	if (ret) {
		printf("i2c EEPROM not found.\n");
		return NULL;
	}

	return dev;
}

/* Get ddr size setting from EEPROM */
int get_imx8m_ddr_size(char *of_path)
{
	struct udevice *dev;
	uint8_t var;
	int ret;

	dev = phytec_i2c_eeprom_init(of_path);
	if (!dev)
		return -ENODEV;

	ret = i2c_eeprom_read(dev, 6, &var, sizeof(var));
	if (ret) {
		printf("Unable to read from i2c EEPROM\n");
		return ret;
	}

	return ((int)var - 48);
}

#endif
