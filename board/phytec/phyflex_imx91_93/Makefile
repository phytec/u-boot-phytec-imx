# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) 2026 PHYTEC Messtechnik GmbH
# Christoph Stoidner <c.stoidner@phytec.de>

obj-y += phyflex-imx91_93.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
ifdef CONFIG_IMX91
# error: no RAM config yet generated for imx91
exit 1
else
obj-$(CONFIG_IMX9_LPDDR4X) += ./lpddr4_timing_imx93.o
endif
endif
