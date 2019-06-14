/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2019 PHYTEC Messtechnik GmbH
 * Author: Janine Hagemann <j.hagemann@phytec.de>
 */

/*
 * pcl066_som struct represents the eeprom layout for PHYTEC Polaris
 */

struct pcl066_som {
	unsigned char api_version;	/* EEPROM layout API version */
	unsigned char som_pcb_rev;	/* SOM revision */
	unsigned char ksp;		/* 1: KSP, 2: KSM */
	unsigned char kspno;		/* Number for KSP/KSM module */
	unsigned char kit_opt[8];	/* coding for variants */
	unsigned char reserved[19];	/* not used */
	unsigned char hw8;		/* Checksum */
} __attribute__ ((__packed__));
