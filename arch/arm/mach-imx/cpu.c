/*
 * (C) Copyright 2007
 * Sascha Hauer, Pengutronix
 *
 * (C) Copyright 2009-2016 Freescale Semiconductor, Inc.
 * Copyright 2017-2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <bootm.h>
#include <common.h>
#include <netdev.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/crm_regs.h>
#include <asm/mach-imx/boot_mode.h>
#if defined(CONFIG_VIDEO_IMXDCSS)
#include <asm/arch/video_common.h>
#endif
#include <imx_thermal.h>
#include <ipu_pixfmt.h>
#include <thermal.h>
#include <sata.h>
#include <linux/bitfield.h>
#include <linux/io.h>
#ifdef CONFIG_VIDEO_GIS
#include <gis.h>
#endif

#ifdef CONFIG_FSL_ESDHC
#include <fsl_esdhc.h>
#endif

#if defined(CONFIG_DISPLAY_CPUINFO) && !defined(CONFIG_SPL_BUILD)
static u32 reset_cause = -1;

static char *get_reset_cause(void)
{
	u32 cause;
	struct src *src_regs = (struct src *)SRC_BASE_ADDR;

	cause = readl(&src_regs->srsr);
#ifndef CONFIG_ANDROID_BOOT_IMAGE
	/* We will read the ssrs states later for android so we don't
	 * clear the states here.
	 */
	writel(cause, &src_regs->srsr);
#endif
	reset_cause = cause;

	switch (cause) {
	case 0x00001:
	case 0x00011:
		return "POR";
	case 0x00004:
		return "CSU";
	case 0x00008:
		return "IPP USER";
	case 0x00010:
#ifdef	CONFIG_MX7
		return "WDOG1";
#else
		return "WDOG";
#endif
	case 0x00020:
		return "JTAG HIGH-Z";
	case 0x00040:
		return "JTAG SW";
	case 0x00080:
		return "WDOG3";
#ifdef CONFIG_MX7
	case 0x00100:
		return "WDOG4";
	case 0x00200:
		return "TEMPSENSE";
#elif defined(CONFIG_IMX8M)
	case 0x00100:
		return "WDOG2";
	case 0x00200:
		return "TEMPSENSE";
#else
	case 0x00100:
		return "TEMPSENSE";
	case 0x10000:
		return "WARM BOOT";
#endif
	default:
		return "unknown reset";
	}
}

#ifdef CONFIG_ANDROID_BOOT_IMAGE
void get_reboot_reason(char *ret)
{
	struct src *src_regs = (struct src *)SRC_BASE_ADDR;

	strcpy(ret, (char *)get_reset_cause());
	/* clear the srsr here, its state has been recorded in reset_cause */
	writel(reset_cause, &src_regs->srsr);
}
#endif

u32 get_imx_reset_cause(void)
{
	return reset_cause;
}
#endif
#ifdef CONFIG_IMX8M

#define DDRC_ADDRMAP(n)				(0x200 + 4 * (n))
#define DDRC_ADDRMAP6_LPDDR4_6GB_12GB_24GB	GENMASK(30, 29)
#define DDRC_ADDRMAP6_LPDDR3_6GB_12GB		BIT(31)
#define DDRC_ADDRMAP0_CS_BIT0			GENMASK(4, 0)

#define DDRC_MSTR				0x0000
#define DDRC_MSTR_LPDDR4			BIT(5)
#define DDRC_MSTR_DATA_BUS_WIDTH		GENMASK(13, 12)
#define DDRC_MSTR_ACTIVE_RANKS			GENMASK(27, 24)

#define DDRC_ADDRMAP0_CS_BIT1			GENMASK(12,  8)

#define DDRC_ADDRMAP1_BANK_B2			GENMASK(20, 16)

#define DDRC_ADDRMAP2_COL_B5			GENMASK(27, 24)
#define DDRC_ADDRMAP2_COL_B4			GENMASK(19, 16)

#define DDRC_ADDRMAP3_COL_B9			GENMASK(27, 24)
#define DDRC_ADDRMAP3_COL_B8			GENMASK(19, 16)
#define DDRC_ADDRMAP3_COL_B7			GENMASK(11,  8)
#define DDRC_ADDRMAP3_COL_B6			GENMASK(3,  0)

#define DDRC_ADDRMAP4_COL_B10			GENMASK(3, 0)
#define DDRC_ADDRMAP4_COL_B11			GENMASK(11, 8)

#define DDRC_ADDRMAP5_ROW_B11			GENMASK(27, 24)

#define DDRC_ADDRMAP6_ROW_B15			GENMASK(27, 24)
#define DDRC_ADDRMAP6_ROW_B14			GENMASK(19, 16)
#define DDRC_ADDRMAP6_ROW_B13			GENMASK(11,  8)
#define DDRC_ADDRMAP6_ROW_B12			GENMASK(3,  0)

#define DDRC_ADDRMAP7_ROW_B17			GENMASK(11,  8)
#define DDRC_ADDRMAP7_ROW_B16			GENMASK(3,  0)

#define MEMCTL_BASE	0x3d400000

unsigned int imx_ddrc_count_bits(unsigned int bits, const u8 config[],
					unsigned int config_num)
{
	unsigned int i;

	for (i = 0; i < config_num && config[i] == 0b1111; i++)
		bits--;

	return bits;
}

resource_size_t imx_ddrc_sdram_size(unsigned char ddrc, const u32 addrmap[],
			u8 col_max, const u8 col_b[], unsigned int col_b_num,
			u8 row_max, const u8 row_b[], unsigned int row_b_num,
			bool reduced_address_space)
{
	const u32 mstr = readl(ddrc + DDRC_MSTR);
	unsigned int banks, ranks, columns, rows, active_ranks, width;
	resource_size_t size;

	banks = 2;
	ranks = 0;

	switch (FIELD_GET(DDRC_MSTR_ACTIVE_RANKS, mstr)) {
	case 0b0001:
	case 0b0100:
		active_ranks = 1;
		break;
	case 0b0011:
	case 0b1100:
		active_ranks = 2;
		break;
	case 0b1111:
		active_ranks = 4;
		break;
	default:
		BUG();
	}

	switch (FIELD_GET(DDRC_MSTR_DATA_BUS_WIDTH, mstr)) {
	case 0b00:	/* Full DQ bus  */
		width = 4;
		break;
	case 0b01:	/* Half DQ bus  */
		width = 2;
		break;
	case 0b10:	/* Quarter DQ bus  */
		width = 1;
		break;
	default:
		BUG();
	}

	if (active_ranks == 4 &&
		(FIELD_GET(DDRC_ADDRMAP0_CS_BIT1, addrmap[0]) != 0b11111))
			ranks++;

	if (active_ranks > 1  &&
		(FIELD_GET(DDRC_ADDRMAP0_CS_BIT0, addrmap[0]) != 0b11111))
			ranks++;

	if (FIELD_GET(DDRC_ADDRMAP1_BANK_B2, addrmap[1]) != 0b11111)
		banks++;

	columns = imx_ddrc_count_bits(col_max, col_b, col_b_num);
	rows    = imx_ddrc_count_bits(row_max, row_b, row_b_num);

	size = ((u64)(1 << banks) * width << (rows + columns)) << ranks;

	return reduced_address_space ? size * 3 / 4 : size;
}

resource_size_t imx8mq_ddrc_sdram_size(void)
{
	const u32 addrmap[] = {
		readl(MEMCTL_BASE + DDRC_ADDRMAP(0)),
		readl(MEMCTL_BASE + DDRC_ADDRMAP(1)),
		readl(MEMCTL_BASE + DDRC_ADDRMAP(2)),
		readl(MEMCTL_BASE + DDRC_ADDRMAP(3)),
		readl(MEMCTL_BASE + DDRC_ADDRMAP(4)),
		readl(MEMCTL_BASE + DDRC_ADDRMAP(5)),
		readl(MEMCTL_BASE + DDRC_ADDRMAP(6)),
		readl(MEMCTL_BASE + DDRC_ADDRMAP(7))
	};

	const u8 col_b[] = {
		FIELD_GET(DDRC_ADDRMAP4_COL_B11, addrmap[4]),
		FIELD_GET(DDRC_ADDRMAP4_COL_B10, addrmap[4]),
		FIELD_GET(DDRC_ADDRMAP3_COL_B9,  addrmap[3]),
		FIELD_GET(DDRC_ADDRMAP3_COL_B8,  addrmap[3]),
		FIELD_GET(DDRC_ADDRMAP3_COL_B7,  addrmap[3]),
		FIELD_GET(DDRC_ADDRMAP3_COL_B6,  addrmap[3]),
		FIELD_GET(DDRC_ADDRMAP2_COL_B5,  addrmap[2]),
		FIELD_GET(DDRC_ADDRMAP2_COL_B4,  addrmap[2]),
	};

	const u8 row_b[] = {
		FIELD_GET(DDRC_ADDRMAP6_ROW_B15, addrmap[6]),
		FIELD_GET(DDRC_ADDRMAP6_ROW_B14, addrmap[6]),
		FIELD_GET(DDRC_ADDRMAP6_ROW_B13, addrmap[6]),
		FIELD_GET(DDRC_ADDRMAP6_ROW_B12, addrmap[6]),
		FIELD_GET(DDRC_ADDRMAP5_ROW_B11, addrmap[5]),
	};

	const bool reduced_address_space =
		FIELD_GET(DDRC_ADDRMAP6_LPDDR4_6GB_12GB_24GB, addrmap[6]);

	return imx_ddrc_sdram_size(MEMCTL_BASE, addrmap,
					12, col_b, sizeof(col_b),
					16, row_b, sizeof(row_b),
					reduced_address_space);
}
#endif
#if defined(CONFIG_MX53) || defined(CONFIG_MX6)
#if defined(CONFIG_MX53)
#define MEMCTL_BASE	ESDCTL_BASE_ADDR
#else
#define MEMCTL_BASE	MMDC_P0_BASE_ADDR
#endif
static const unsigned char col_lookup[] = {9, 10, 11, 8, 12, 9, 9, 9};
static const unsigned char bank_lookup[] = {3, 2};

/* these MMDC registers are common to the IMX53 and IMX6 */
struct esd_mmdc_regs {
	uint32_t	ctl;
	uint32_t	pdc;
	uint32_t	otc;
	uint32_t	cfg0;
	uint32_t	cfg1;
	uint32_t	cfg2;
	uint32_t	misc;
};

#define ESD_MMDC_CTL_GET_ROW(mdctl)	((ctl >> 24) & 7)
#define ESD_MMDC_CTL_GET_COLUMN(mdctl)	((ctl >> 20) & 7)
#define ESD_MMDC_CTL_GET_WIDTH(mdctl)	((ctl >> 16) & 3)
#define ESD_MMDC_CTL_GET_CS1(mdctl)	((ctl >> 30) & 1)
#define ESD_MMDC_MISC_GET_BANK(mdmisc)	((misc >> 5) & 1)

/*
 * imx_ddr_size - return size in bytes of DRAM according MMDC config
 * The MMDC MDCTL register holds the number of bits for row, col, and data
 * width and the MMDC MDMISC register holds the number of banks. Combine
 * all these bits to determine the meme size the MMDC has been configured for
 */
unsigned imx_ddr_size(void)
{
	struct esd_mmdc_regs *mem = (struct esd_mmdc_regs *)MEMCTL_BASE;
	unsigned ctl = readl(&mem->ctl);
	unsigned misc = readl(&mem->misc);
	int bits = 11 + 0 + 0 + 1;      /* row + col + bank + width */

	bits += ESD_MMDC_CTL_GET_ROW(ctl);
	bits += col_lookup[ESD_MMDC_CTL_GET_COLUMN(ctl)];
	bits += bank_lookup[ESD_MMDC_MISC_GET_BANK(misc)];
	bits += ESD_MMDC_CTL_GET_WIDTH(ctl);
	bits += ESD_MMDC_CTL_GET_CS1(ctl);

	/* The MX6 can do only 3840 MiB of DRAM */
	if (bits == 32)
		return 0xf0000000;

	return 1 << bits;
}
#endif

#if defined(CONFIG_DISPLAY_CPUINFO) && !defined(CONFIG_SPL_BUILD)

const char *get_imx_type(u32 imxtype)
{
	switch (imxtype) {
	case MXC_CPU_IMX8MM:
		return "8MMQ";	/* Quad-core version of the imx8mm */
	case MXC_CPU_IMX8MML:
		return "8MMQL";	/* Quad-core Lite version of the imx8mm */
	case MXC_CPU_IMX8MMD:
		return "8MMD";	/* Dual-core version of the imx8mm */
	case MXC_CPU_IMX8MMDL:
		return "8MMDL";	/* Dual-core Lite version of the imx8mm */
	case MXC_CPU_IMX8MMS:
		return "8MMS";	/* Single-core version of the imx8mm */
	case MXC_CPU_IMX8MMSL:
		return "8MMSL";	/* Single-core Lite version of the imx8mm */
	case MXC_CPU_IMX8MQ:
		return "8MQ";	/* Quad-core version of the imx8mq */
	case MXC_CPU_IMX8MQL:
		return "8MQLite";	/* Quad-core Lite version of the imx8mq */
	case MXC_CPU_IMX8MD:
		return "8MD";	/* Dual-core version of the imx8mq */
	case MXC_CPU_MX7S:
		return "7S";	/* Single-core version of the mx7 */
	case MXC_CPU_MX7D:
		return "7D";	/* Dual-core version of the mx7 */
	case MXC_CPU_MX6QP:
		return "6QP";	/* Quad-Plus version of the mx6 */
	case MXC_CPU_MX6DP:
		return "6DP";	/* Dual-Plus version of the mx6 */
	case MXC_CPU_MX6Q:
		return "6Q";	/* Quad-core version of the mx6 */
	case MXC_CPU_MX6D:
		return "6D";	/* Dual-core version of the mx6 */
	case MXC_CPU_MX6DL:
		return "6DL";	/* Dual Lite version of the mx6 */
	case MXC_CPU_MX6SOLO:
		return "6SOLO";	/* Solo version of the mx6 */
	case MXC_CPU_MX6SL:
		return "6SL";	/* Solo-Lite version of the mx6 */
	case MXC_CPU_MX6SLL:
		return "6SLL";	/* SLL version of the mx6 */
	case MXC_CPU_MX6SX:
		return "6SX";   /* SoloX version of the mx6 */
	case MXC_CPU_MX6UL:
		return "6UL";   /* Ultra-Lite version of the mx6 */
	case MXC_CPU_MX6ULL:
		return "6ULL";	/* ULL version of the mx6 */
	case MXC_CPU_MX6ULZ:
		return "6ULZ";	/* ULL version of the mx6 */
	case MXC_CPU_MX51:
		return "51";
	case MXC_CPU_MX53:
		return "53";
	default:
		return "??";
	}
}

int print_cpuinfo(void)
{
	u32 cpurev;
	__maybe_unused u32 max_freq;
#if defined(CONFIG_DBG_MONITOR)
	struct dbg_monitor_regs *dbg =
		(struct dbg_monitor_regs *)DEBUG_MONITOR_BASE_ADDR;
#endif

	cpurev = get_cpu_rev();

#if defined(CONFIG_IMX_THERMAL) || defined(CONFIG_NXP_TMU)
	struct udevice *thermal_dev;
	int cpu_tmp, minc, maxc, ret;

	printf("CPU:   Freescale i.MX%s rev%d.%d",
	       get_imx_type((cpurev & 0xFF000) >> 12),
	       (cpurev & 0x000F0) >> 4,
	       (cpurev & 0x0000F) >> 0);
	max_freq = get_cpu_speed_grade_hz();
	if (!max_freq || max_freq == mxc_get_clock(MXC_ARM_CLK)) {
		printf(" at %dMHz\n", mxc_get_clock(MXC_ARM_CLK) / 1000000);
	} else {
		printf(" %d MHz (running at %d MHz)\n", max_freq / 1000000,
		       mxc_get_clock(MXC_ARM_CLK) / 1000000);
	}
#else
	printf("CPU:   Freescale i.MX%s rev%d.%d at %d MHz\n",
		get_imx_type((cpurev & 0xFF000) >> 12),
		(cpurev & 0x000F0) >> 4,
		(cpurev & 0x0000F) >> 0,
		mxc_get_clock(MXC_ARM_CLK) / 1000000);
#endif

#if defined(CONFIG_IMX_THERMAL) || defined(CONFIG_NXP_TMU)
	puts("CPU:   ");
	switch (get_cpu_temp_grade(&minc, &maxc)) {
	case TEMP_AUTOMOTIVE:
		puts("Automotive temperature grade ");
		break;
	case TEMP_INDUSTRIAL:
		puts("Industrial temperature grade ");
		break;
	case TEMP_EXTCOMMERCIAL:
		puts("Extended Commercial temperature grade ");
		break;
	default:
		puts("Commercial temperature grade ");
		break;
	}
	printf("(%dC to %dC)", minc, maxc);
#if	defined(CONFIG_NXP_TMU)
	ret = uclass_get_device_by_name(UCLASS_THERMAL, "cpu-thermal", &thermal_dev);
#else
	ret = uclass_get_device(UCLASS_THERMAL, 0, &thermal_dev);
#endif
	if (!ret) {
		ret = thermal_get_temp(thermal_dev, &cpu_tmp);

		if (!ret)
			printf(" at %dC\n", cpu_tmp);
		else
			debug(" - invalid sensor data\n");
	} else {
		debug(" - invalid sensor device\n");
	}
#endif

#if defined(CONFIG_DBG_MONITOR)
	if (readl(&dbg->snvs_addr))
		printf("DBG snvs regs addr 0x%x, data 0x%x, info 0x%x\n",
		       readl(&dbg->snvs_addr),
		       readl(&dbg->snvs_data),
		       readl(&dbg->snvs_info));
#endif

	printf("Reset cause: %s\n", get_reset_cause());
	return 0;
}
#endif

int cpu_eth_init(bd_t *bis)
{
	int rc = -ENODEV;

#if defined(CONFIG_FEC_MXC)
	rc = fecmxc_initialize(bis);
#endif

	return rc;
}

#ifdef CONFIG_FSL_ESDHC
/*
 * Initializes on-chip MMC controllers.
 * to override, implement board_mmc_init()
 */
int cpu_mmc_init(bd_t *bis)
{
	return fsl_esdhc_mmc_init(bis);
}
#endif

#if !(defined(CONFIG_MX7) || defined(CONFIG_IMX8M))
u32 get_ahb_clk(void)
{
	struct mxc_ccm_reg *imx_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	u32 reg, ahb_podf;

	reg = __raw_readl(&imx_ccm->cbcdr);
	reg &= MXC_CCM_CBCDR_AHB_PODF_MASK;
	ahb_podf = reg >> MXC_CCM_CBCDR_AHB_PODF_OFFSET;

	return get_periph_clk() / (ahb_podf + 1);
}
#endif

void arch_preboot_os(void)
{
#if defined(CONFIG_PCIE_IMX)
	imx_pcie_remove();
#endif
#if defined(CONFIG_SATA)
	sata_remove(0);
#if defined(CONFIG_MX6)
	disable_sata_clock();
#endif
#endif
#if defined(CONFIG_LDO_BYPASS_CHECK)
	ldo_mode_set(check_ldo_bypass());
#endif
#if defined(CONFIG_VIDEO_IPUV3)
	/* disable video before launching O/S */
	ipuv3_fb_shutdown();
#endif
#ifdef CONFIG_VIDEO_GIS
	/* Entry for GIS */
	mxc_disable_gis();
#endif
#if defined(CONFIG_VIDEO_MXS)
	lcdif_power_down();
#endif
#if defined(CONFIG_VIDEO_IMXDCSS)
	imx8m_fb_disable();
#endif
}

#ifndef CONFIG_IMX8M
void set_chipselect_size(int const cs_size)
{
	unsigned int reg;
	struct iomuxc *iomuxc_regs = (struct iomuxc *)IOMUXC_BASE_ADDR;
	reg = readl(&iomuxc_regs->gpr[1]);

	switch (cs_size) {
	case CS0_128:
		reg &= ~0x7;	/* CS0=128MB, CS1=0, CS2=0, CS3=0 */
		reg |= 0x5;
		break;
	case CS0_64M_CS1_64M:
		reg &= ~0x3F;	/* CS0=64MB, CS1=64MB, CS2=0, CS3=0 */
		reg |= 0x1B;
		break;
	case CS0_64M_CS1_32M_CS2_32M:
		reg &= ~0x1FF;	/* CS0=64MB, CS1=32MB, CS2=32MB, CS3=0 */
		reg |= 0x4B;
		break;
	case CS0_32M_CS1_32M_CS2_32M_CS3_32M:
		reg &= ~0xFFF;  /* CS0=32MB, CS1=32MB, CS2=32MB, CS3=32MB */
		reg |= 0x249;
		break;
	default:
		printf("Unknown chip select size: %d\n", cs_size);
		break;
	}

	writel(reg, &iomuxc_regs->gpr[1]);
}
#endif

#if defined(CONFIG_MX7) || defined(CONFIG_IMX8M)
/*
 * OCOTP_TESTER3[9:8] (see Fusemap Description Table offset 0x440)
 * defines a 2-bit SPEED_GRADING
 */
#define OCOTP_TESTER3_SPEED_SHIFT	8
enum cpu_speed {
	OCOTP_TESTER3_SPEED_GRADE0,
	OCOTP_TESTER3_SPEED_GRADE1,
	OCOTP_TESTER3_SPEED_GRADE2,
	OCOTP_TESTER3_SPEED_GRADE3,
	OCOTP_TESTER3_SPEED_GRADE4,
};

u32 get_cpu_speed_grade_hz(void)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[1];
	struct fuse_bank1_regs *fuse =
		(struct fuse_bank1_regs *)bank->fuse_regs;
	uint32_t val;

	val = readl(&fuse->tester3);
	val >>= OCOTP_TESTER3_SPEED_SHIFT;
	if (is_imx8mm())
		val &= 0x7;
	else
		val &= 0x3;

	switch(val) {
	case OCOTP_TESTER3_SPEED_GRADE0:
		return 800000000;
	case OCOTP_TESTER3_SPEED_GRADE1:
		return (is_mx7() ? 500000000 : (is_imx8mq() ? 1000000000 : 1200000000));
	case OCOTP_TESTER3_SPEED_GRADE2:
		return (is_mx7() ? 1000000000 : (is_imx8mq() ? 1300000000 : 1600000000));
	case OCOTP_TESTER3_SPEED_GRADE3:
		return (is_mx7() ? 1200000000 : (is_imx8mq() ? 1500000000 : 1800000000));
	case OCOTP_TESTER3_SPEED_GRADE4:
		return 2000000000;
	}

	return 0;
}

/*
 * OCOTP_TESTER3[7:6] (see Fusemap Description Table offset 0x440)
 * defines a 2-bit SPEED_GRADING
 */
#define OCOTP_TESTER3_TEMP_SHIFT	6

u32 get_cpu_temp_grade(int *minc, int *maxc)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[1];
	struct fuse_bank1_regs *fuse =
		(struct fuse_bank1_regs *)bank->fuse_regs;
	uint32_t val;

	val = readl(&fuse->tester3);
	val >>= OCOTP_TESTER3_TEMP_SHIFT;
	val &= 0x3;

	if (minc && maxc) {
		if (val == TEMP_AUTOMOTIVE) {
			*minc = -40;
			*maxc = 125;
		} else if (val == TEMP_INDUSTRIAL) {
			*minc = -40;
			*maxc = 105;
		} else if (val == TEMP_EXTCOMMERCIAL) {
			*minc = -20;
			*maxc = 105;
		} else {
			*minc = 0;
			*maxc = 95;
		}
	}
	return val;
}
#endif

#if defined(CONFIG_MX7) || defined(CONFIG_IMX8M)
enum boot_device get_boot_device(void)
{
	struct bootrom_sw_info **p =
		(struct bootrom_sw_info **)(ulong)ROM_SW_INFO_ADDR;

	enum boot_device boot_dev = SD1_BOOT;
	u8 boot_type = (*p)->boot_dev_type;
	u8 boot_instance = (*p)->boot_dev_instance;

	switch (boot_type) {
	case BOOT_TYPE_SD:
		boot_dev = boot_instance + SD1_BOOT;
		break;
	case BOOT_TYPE_MMC:
		boot_dev = boot_instance + MMC1_BOOT;
		break;
	case BOOT_TYPE_NAND:
		boot_dev = NAND_BOOT;
		break;
	case BOOT_TYPE_QSPI:
		boot_dev = QSPI_BOOT;
		break;
	case BOOT_TYPE_WEIM:
		boot_dev = WEIM_NOR_BOOT;
		break;
	case BOOT_TYPE_SPINOR:
		boot_dev = SPI_NOR_BOOT;
		break;
	case BOOT_TYPE_USB:
		boot_dev = USB_BOOT;
		break;
	default:
#ifdef CONFIG_IMX8M
		if (((readl(SRC_BASE_ADDR + 0x58) & 0x00007FFF) >> 12) == 0x4)
			boot_dev = QSPI_BOOT;
#endif
		break;
	}

	return boot_dev;
}
#endif

#ifdef CONFIG_NXP_BOARD_REVISION
int nxp_board_rev(void)
{
	/*
	 * Get Board ID information from OCOTP_GP1[15:8]
	 * RevA: 0x1
	 * RevB: 0x2
	 * RevC: 0x3
	 */
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[4];
	struct fuse_bank4_regs *fuse =
			(struct fuse_bank4_regs *)bank->fuse_regs;

	return (readl(&fuse->gp1) >> 8 & 0x0F);
}

char nxp_board_rev_string(void)
{
	const char *rev = "A";

	return (*rev + nxp_board_rev() - 1);
}
#endif
