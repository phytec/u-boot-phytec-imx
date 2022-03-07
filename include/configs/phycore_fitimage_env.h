/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2022 PHYTEC Messtechnik GmbH
 * Author: Albert Schwarzkopf <a.schwarzkopf@phytec.de>, Maik Otto <m.otto@phytec.de>
 */

#ifndef __PHYCORE_FITIMAGE_ENV_H
#define __PHYCORE_FITIMAGE_ENV_H

#define PHYCORE_FITIMAGE_ENV_BOOTLOGIC \
	"fit_create_overlay_conf_spec=fdt address ${loadaddr}; " \
		"fdt get value default_fit_conf /configurations/ default; " \
		"overlay_specs=\"${loadaddr}:#${default_fit_conf}\"; " \
		"for overlay in ${overlays}; do " \
			"overlay_specs=${overlay_specs}#${overlay}; " \
		"done; " \
		"env set bootm_fit_conf_spec ${overlay_specs};\0" \
	"fit_test_and_run_boot=if test ${dofitboot} = 1; then " \
			"if test ${no_overlays} = 0; then " \
				"run fit_create_overlay_conf_spec; " \
				"bootm ${bootm_fit_conf_spec};" \
			"else " \
				"bootm; " \
			"fi; " \
		"fi;\0" \

#endif /* __PHYCORE_FITIMAGE_ENV_H */
