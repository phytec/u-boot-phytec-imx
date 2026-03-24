// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/arch/clock.h>
#include <env.h>
#include <init.h>
#include <fdt_support.h>
#include <jffs2/load_kernel.h>
#include <miiphy.h>
#include <mtd_node.h>
#include <usb.h>
#include <dwc3-uboot.h>

#include "../common/imx8m_som_detection.h"

DECLARE_GLOBAL_DATA_PTR;

static struct dwc3_device dwc3_device_data = {
#ifdef CONFIG_SPL_BUILD
	.maximum_speed = USB_SPEED_HIGH,
#else
	.maximum_speed = USB_SPEED_SUPER,
#endif
	.base = USB1_BASE_ADDR,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
	.power_down_scale = 2,
};

int board_usb_init(int index, enum usb_init_type init)
{
	if (index == 0 && init == USB_INIT_DEVICE) {
		imx8m_usb_power(index, true);
		return dwc3_uboot_init(&dwc3_device_data);
	}
	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	if (index == 0 && init == USB_INIT_DEVICE) {
		dwc3_uboot_exit(index);
		imx8m_usb_power(index, false);
	}
	return 0;
}

#define EEPROM_ADDR		0x51
#define EEPROM_ADDR_FALLBACK	0x59

int ft_board_setup(void *blob, struct bd_info *bd)
{
	enum phytec_imx8mp_ddr_eeprom_code size;

	u8 spi = phytec_get_imx8m_spi(NULL);
	/* Add partitions when SPI flash is available */
	if (spi) {
		static const struct node_info nodes[] = {
			{ "jedec,spi-nor", MTD_DEV_TYPE_NOR, },
		};

		fdt_fixup_mtdparts(blob, nodes, ARRAY_SIZE(nodes));
	};

	size = phytec_get_imx8m_ddr_size(NULL);
	if (size == PHYTEC_IMX8MP_DDR_1GB) {
		u32 *prop;
		u32 phandle;
		int prop_offset;
		int node_offset;

		prop_offset = fdt_node_offset_by_compatible(blob, -1, "fsl,imx8mp-gpu");
		if (prop_offset < 0) {
			printf("%s: Could not find \"fsl,imx8mp-gpu\" node in oftree\n", __func__);
			return 0;
		}

		prop = (uint32_t *)fdt_getprop(blob, prop_offset, "memory-region", NULL);
		if (prop < 0) {
			printf("%s: Could not find \"memory-region\" property in GPU node\n",
			       __func__);
			return 0;
		}

		/* Look up in memory-region referenced reserved gpu mem node */
		phandle = fdt32_to_cpu(*prop);
		node_offset = fdt_node_offset_by_phandle(blob, phandle);
		if (node_offset < 0) {
			printf("%s: Could not find referenced node in \"memory-region\" property",
			       __func__);
			return 0;
		}

		/* Delete memory-region property and reserved gpu mem node */
		fdt_del_node(blob, node_offset);
		fdt_delprop(blob, prop_offset, "memory-region");
	}

	return 0;
}

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&gpr->gpr[1], 0x2000, 0);

	return 0;
}

int board_init(void)
{
	int ret = phytec_eeprom_data_setup_fallback(NULL, 0,
			EEPROM_ADDR, EEPROM_ADDR_FALLBACK);
	if (ret)
		printf("%s: EEPROM data init failed\n", __func__);

	setup_fec();

	init_usb_clk();

	return 0;
}

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no;
}

int board_late_init(void)
{
	u8 spi = phytec_get_imx8m_spi(NULL);

	if (spi != 0 && spi != PHYTEC_EEPROM_INVAL)
		env_set("spiprobe", "sf probe");

	switch (get_boot_device()) {
	case SD2_BOOT:
		env_set_ulong("mmcdev", 1);
		if (!env_get("boot_targets"))
			env_set("boot_targets", "mmc1 mmc2 usb ethernet");
		break;
	case MMC3_BOOT:
		env_set_ulong("mmcdev", 2);
		break;
	case USB_BOOT:
		printf("Detect USB boot. Will enter fastboot mode!\n");
		if (!strcmp(env_get("bootcmd"), env_get_default("bootcmd")))
			env_set("bootcmd", "fastboot 0; bootflow scan -lb;");
		break;
	default:
		break;
	}

	return 0;
}

int board_phys_sdram_size(phys_size_t *size)
{
	if (!size)
		return -EINVAL;

	*size = get_ram_size((void *)PHYS_SDRAM, PHYS_SDRAM_SIZE + PHYS_SDRAM_2_SIZE);

	return 0;
}
