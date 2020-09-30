/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#ifndef _PHYTEC_IMX8M_DDR_H
#define _PHYTEC_IMX8M_DDR_H

#if !defined(CONFIG_DM_I2C)
int get_imx8m_ddr_size(int bus_num, int addr);
#else
int get_imx8m_ddr_size(char *of_path);
#endif

#endif
