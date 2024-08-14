# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) 2024 PHYTEC Messtechnik GmbH
# Copyright (C) 2023 PHYTEC Messtechnik GmbH
# Christoph Stoidner <c.stoidner@phytec.de>
# Copyright (C) 2024 Mathieu Othacehe <m.othacehe@gmail.com>
# Copyright 2022 NXP
#

#

obj-y += phycore-imx93.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX9_LPDDR4X) += lpddr4_timing.o
endif
