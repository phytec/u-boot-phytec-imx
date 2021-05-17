/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#ifndef _PHYTEC_IMX8M_DDR_H
#define _PHYTEC_IMX8M_DDR_H

enum {
	PHYTEC_API_REV0 = 0,
	PHYTEC_API_REV1,
};

struct phytec_api0_data {
	u8 pcb_rev;		/* PCB revision of SoM */
	u8 som_type;		/* SoM type */
	u8 ksp_no;		/* KSP no */
	char opt[16];		/* SoM options */
	u8 mac[6];		/* MAC address (optional) */
	u8 pad[5];		/* padding */
	u8 cksum;		/* checksum */
} __attribute__ ((__packed__));

struct phytec_eeprom_data {
	u8 api_rev;
	union {
		struct phytec_api0_data data_api0;
	} data;
} __attribute__ ((__packed__));

int phytec_eeprom_data_init(char *of_path, int bus_num, int addr);

char * __maybe_unused phytec_get_imx8m_opt(void);
u8 __maybe_unused phytec_get_imx8m_ddr_size(void);
#endif
