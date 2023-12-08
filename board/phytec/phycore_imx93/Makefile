# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (C) 2023 PHYTEC Messtechnik GmbH
# Author: Christoph Stoidner <c.stoidner@phytec.de>
#

obj-y += phycore-imx93.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX9_LPDDR4X) += lpddr4_timing.o
endif
