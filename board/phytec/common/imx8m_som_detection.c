// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#include <common.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch/sys_proto.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <i2c.h>
#include <u-boot/crc.h>

#include "imx8m_som_detection.h"

struct phytec_eeprom_data eeprom_data;

int _phytec_eeprom_data_init(struct phytec_eeprom_data *data,
			     int bus_num, int addr)
{
	int ret, i;
	unsigned int crc;
	u8 som;
	char *opt;
	int *ptr;

	if (!data)
		data = &eeprom_data;

#if defined(CONFIG_DM_I2C)
	struct udevice *dev;

	ret = i2c_get_chip_for_busnum(bus_num, addr, 2, &dev);
	if (ret) {
		pr_err("%s: i2c EEPROM not found: %i.\n", __func__, ret);
		return ret;
	}

	ret = dm_i2c_read(dev, 0, (uint8_t *)data,
			  sizeof(struct phytec_eeprom_data));
#else
	i2c_set_bus_num(bus_num);
	ret = i2c_read(addr, 0, 2, (uint8_t *)data,
		       sizeof(struct phytec_eeprom_data));
#endif
	if (ret) {
		pr_err("%s: Unable to read EEPROM data\n", __func__);
		return ret;
	}

	if (data->api_rev == 0xff) {
		pr_err("%s: EEPROM is not flashed. Prototype?\n", __func__);
		return -EINVAL;
	}

	ptr = (int *)data;
	for (i = 0; i < sizeof(struct phytec_eeprom_data); i += sizeof(ptr))
		if (*ptr != 0x0)
			break;

	if (i == sizeof(struct phytec_eeprom_data)) {
		pr_err("%s: EEPROM data is all zero. Erased?\n", __func__);
		return -EINVAL;
	}

	if (data->api_rev > PHYTEC_API_REV2) {
		pr_err("%s: EEPROM API revision %u not supported\n",
			__func__, data->api_rev);
		return -EINVAL;
	}

	/* We are done here for early revisions */
	if (data->api_rev <= PHYTEC_API_REV1)
		return 0;

	crc = crc8(0, (const unsigned char *)data,
		   sizeof(struct phytec_eeprom_data));
	debug("%s: crc: %x\n", __func__, crc);

	if (crc) {
		pr_err("%s: CRC mismatch. EEPROM data is not usable\n",
		       __func__);
		return -EINVAL;
	}

	som = data->data.data_api2.som_no;
	debug("%s: som id: %u\n", __func__, som);
	opt = _phytec_get_imx8m_opt(data);

	if (som == PHYTEC_IMX8MP_SOM && is_imx8mp())
		return 0;

	if (som == PHYTEC_IMX8MM_SOM) {
		if (((opt[0] - '0') != 0) &&
		    ((opt[1] - '0') == 0) && is_imx8mm())
			return 0;
		else if (((opt[0] - '0') == 0) &&
			 ((opt[1] - '0') != 0) && is_imx8mn())
			return 0;
		goto err;
	}

	if (som == PHYTEC_IMX8MQ_SOM && is_imx8mq())
		return 0;
err:
	pr_err("%s: SoM ID does not match. Wrong EEPROM data?\n", __func__);
	return -EINVAL;
}

char * __maybe_unused _phytec_get_imx8m_opt(struct phytec_eeprom_data *data)
{
	char *opt;

	if (!data)
		data = &eeprom_data;

	switch (data->api_rev) {
	case PHYTEC_API_REV0:
	case PHYTEC_API_REV1:
		opt = data->data.data_api0.opt;
		break;
	case PHYTEC_API_REV2:
		opt = data->data.data_api2.opt;
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
u8 __maybe_unused _phytec_get_imx8m_ddr_size(struct phytec_eeprom_data *data)
{
	char *opt;
	u8 ddr_id;

	if (!data)
		data = &eeprom_data;

	opt = _phytec_get_imx8m_opt(data);
	if (opt)
		ddr_id = opt[2] - '0';
	else
		ddr_id = PHYTEC_EEPROM_INVAL;

	debug("%s: ddr id: %u\n", __func__, ddr_id);
	return ddr_id;
}

/*
 * Filter SPI-NOR flash information. All i.MX8M boards have this at
 * the same location.
 * returns: 0x0 if no SPI is poulated. Otherwise a board depended
 * code for the size. PHYTEC_EEPROM_INVAL when the data is invalid.
 */
u8 __maybe_unused _phytec_get_imx8m_spi(struct phytec_eeprom_data *data)
{
	char *opt;
	u8 spi;

	if (!data)
		data = &eeprom_data;

	opt = _phytec_get_imx8m_opt(data);
	if (opt)
		spi = opt[4] - '0';
	else
		spi = PHYTEC_EEPROM_INVAL;

	debug("%s: spi: %u\n", __func__, spi);
	return spi;
}

/*
 * Filter ethernet phy information. All i.MX8M boards have this at
 * the same location.
 * returns: 0x0 if no ethernet phy is poulated. 0x1 if it is populated.
 * PHYTEC_EEPROM_INVAL when the data is invalid.
 */
u8 __maybe_unused _phytec_get_imx8m_eth(struct phytec_eeprom_data *data)
{
	char *opt;
	u8 eth;

	if (!data)
		data = &eeprom_data;

	opt = _phytec_get_imx8m_opt(data);
	if (opt) {
		eth = opt[5] - '0';
		eth &= 0x1;
	} else {
		eth = PHYTEC_EEPROM_INVAL;
	}

	debug("%s: eth: %u\n", __func__, eth);
	return eth;
}

void __maybe_unused _phytec_print_som_info(struct phytec_eeprom_data *data)
{
	struct phytec_api2_data *api2;
	char pcb_sub_rev;
	unsigned int ksp_no;

	if (!data)
		data = &eeprom_data;

	if (data->api_rev < PHYTEC_API_REV2)
		return;

	api2 = &data->data.data_api2;

	/* Calculate PCB subrevision */
	pcb_sub_rev = api2->pcb_sub_opt_rev & 0x0f;
	pcb_sub_rev = pcb_sub_rev ? ((pcb_sub_rev - 1) + 'a') : 0;

	/* print standard product string */
	if (api2->som_type <= 1) {
		printf("SoM: %s-%03u-%s.%s PCB rev: %u%c\n",
		       phytec_som_type_str[api2->som_type], api2->som_no,
		       api2->opt, api2->bom_rev, api2->pcb_rev, pcb_sub_rev);
		return;
	}
	/* print KSP/KSM string */
	if (api2->som_type <= 3) {
		ksp_no = (api2->ksp_no << 8) | api2->som_no;
		printf("SoM: %s-%u ",
		       phytec_som_type_str[api2->som_type], ksp_no);
	/* print standard product based KSP/KSM strings */
	} else {
		printf("SoM: %s-%03u-%03u ",
		       phytec_som_type_str[api2->som_type],
		       api2->som_no, api2->ksp_no);
	}

	printf("Option: %s BOM rev: %s PCB rev: %u%c\n", api2->opt,
	       api2->bom_rev, api2->pcb_rev, pcb_sub_rev);
}
