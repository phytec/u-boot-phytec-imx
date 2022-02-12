// SPDX-License-Identifier: GPL-2.0+
/*
 * Analog Devices PHY drivers
 *
 * Copyright 2021 Konsulko Group
 * Author: Matt Ranostay <matt.ranostay@konsulko.com>
 */

#include <phy.h>

static struct phy_driver ADIN1300_driver = {
	.name		= "ADIN1300",
	.uid		= 0x0283bc30,
	.mask 		= 0xffffff0,
	.features 	= PHY_BASIC_FEATURES,
	.config 	= genphy_config,
	.startup	= genphy_startup,
	.shutdown	= genphy_shutdown,
};

int phy_adin_init(void)
{
	phy_register(&ADIN1300_driver);

	return 0;
}
