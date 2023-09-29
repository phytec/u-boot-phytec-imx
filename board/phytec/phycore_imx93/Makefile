#
# Copyright 2022 NXP
# Copyright (C) 2023 PHYTEC Messtechnik GmbH
# Christoph Stoidner <c.stoidner@phytec.de>
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += phycore_imx93.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX93_PHYCORE_LPDDR4X_1GB) += lpddr4_timing_1GB.o
endif
