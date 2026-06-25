// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 PHYTEC Messtechnik GmbH
 */
#include <env.h>
#include <init.h>
#include <asm/arch-imx9/ccm_regs.h>
#include <asm/arch/clock.h>
#include <fdt_support.h>
#include <usb.h>
#include <dwc3-uboot.h>
#include <asm/io.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <i2c.h>
#include <miiphy.h>
#include <asm/gpio.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>
#include <scmi_agent.h>
#include <dt-bindings/clock/nxp,imx95-clock.h>
#include <../dts/upstream/src/arm64/freescale/imx95-power.h>

int board_early_init_f(void)
{
	/* UART7: A55, UART2: M33 */
	init_uart_clk(6);

	return 0;
}

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
	if (index == 0 && init == USB_INIT_DEVICE)
		return dwc3_uboot_init(&dwc3_device_data);

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 0 && init == USB_INIT_DEVICE)
		dwc3_uboot_exit(index);

	return ret;
}

static int imx9_scmi_power_domain_enable(u32 domain, bool enable)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device_by_name(UCLASS_CLK, "protocol@14", &dev);
	if (ret)
		return ret;

	return scmi_pwd_state_set(dev, 0, domain, enable ? 0 : BIT(30));
}

void netc_init(void)
{
	int ret;

	ret = imx9_scmi_power_domain_enable(IMX95_PD_NETC, false);
	udelay(10000);

	/* Power up the NETC MIX. */
	ret = imx9_scmi_power_domain_enable(IMX95_PD_NETC, true);
	if (ret) {
		printf("SCMI_POWWER_STATE_SET Failed for NETC MIX\n");
		return;
	}

	pci_init();
}

int board_init(void)
{
	netc_init();

	return 0;
}

int board_late_init(void)
{
	switch (get_boot_device()) {
	case SD2_BOOT:
		env_set_ulong("mmcdev", 1);
		if (!env_get("boot_targets"))
			env_set("boot_targets", "mmc1 mmc0 ethernet");
		break;
	case MMC1_BOOT:
		env_set_ulong("mmcdev", 0);
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
	*size = PHYS_SDRAM_SIZE + PHYS_SDRAM_2_SIZE;

	return 0;
}
