// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <asm/io.h>
#include <linux/bitfield.h>
#include <asm/arch/sys_proto.h>

#if defined(CONFIG_MX53)
#define MEMCTL_BASE	ESDCTL_BASE_ADDR
#elif defined(CONFIG_MX6)
#define MEMCTL_BASE	MMDC_P0_BASE_ADDR
#elif defined(CONFIG_MX7ULP)
#define MEMCTL_BASE	MMDC0_RBASE
#endif
static const unsigned char col_lookup[] = {9, 10, 11, 8, 12, 9, 9, 9};
static const unsigned char bank_lookup[] = {3, 2};

#if defined(CONFIG_IMX8M)
#define DDRC_ADDRMAP(n)                                (0x200 + 4 * (n))
#define DDRC_ADDRMAP6_LPDDR4_6GB_12GB_24GB     GENMASK(30, 29)
#define DDRC_ADDRMAP6_LPDDR3_6GB_12GB          BIT(31)
#define DDRC_ADDRMAP0_CS_BIT0                  GENMASK(4, 0)

#define DDRC_MSTR                              0x0000
#define DDRC_MSTR_LPDDR4                       BIT(5)
#define DDRC_MSTR_DATA_BUS_WIDTH               GENMASK(13, 12)
#define DDRC_MSTR_ACTIVE_RANKS                 GENMASK(27, 24)

#define DDRC_ADDRMAP0_CS_BIT1                  GENMASK(12,  8)

#define DDRC_ADDRMAP1_BANK_B2                  GENMASK(20, 16)

#define DDRC_ADDRMAP2_COL_B5                   GENMASK(27, 24)
#define DDRC_ADDRMAP2_COL_B4                   GENMASK(19, 16)

#define DDRC_ADDRMAP3_COL_B9                   GENMASK(27, 24)
#define DDRC_ADDRMAP3_COL_B8                   GENMASK(19, 16)
#define DDRC_ADDRMAP3_COL_B7                   GENMASK(11,  8)
#define DDRC_ADDRMAP3_COL_B6                   GENMASK(3,  0)
#define DDRC_ADDRMAP4_COL_B10                  GENMASK(3, 0)
#define DDRC_ADDRMAP4_COL_B11                  GENMASK(11, 8)

#define DDRC_ADDRMAP5_ROW_B11                  GENMASK(27, 24)

#define DDRC_ADDRMAP6_ROW_B15                  GENMASK(27, 24)
#define DDRC_ADDRMAP6_ROW_B14                  GENMASK(19, 16)
#define DDRC_ADDRMAP6_ROW_B13                  GENMASK(11,  8)
#define DDRC_ADDRMAP6_ROW_B12                  GENMASK(3,  0)

#define DDRC_ADDRMAP7_ROW_B17                  GENMASK(11,  8)
#define DDRC_ADDRMAP7_ROW_B16                  GENMASK(3,  0)

#define MEMCTL_BASE    0x3d400000
#endif

/* these MMDC registers are common to the IMX53 and IMX6 */
struct esd_mmdc_regs {
	u32 ctl;
	u32 pdc;
	u32 otc;
	u32 cfg0;
	u32 cfg1;
	u32 cfg2;
	u32 misc;
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
unsigned int imx_ddr_size(void)
{
	struct esd_mmdc_regs *mem = (struct esd_mmdc_regs *)MEMCTL_BASE;
	unsigned int ctl = readl(&mem->ctl);
	unsigned int misc = readl(&mem->misc);
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

unsigned int imx_ddrc_count_bits(unsigned int bits, const u8 config[],
				 unsigned int config_num)
{
	for (int i = 0; i < config_num && config[i] == 0b1111; i++)
		bits--;

	return bits;
}

resource_size_t imx_ddrc_sdram_size(void *ddrc, const u32 addrmap[],
				    u8 col_max, const u8 col_b[],
				    unsigned int col_b_num,
				    u8 row_max, const u8 row_b[],
				    unsigned int row_b_num,
				    bool reduced_address_space)
{
	const u32 mstr = readl(ddrc + DDRC_MSTR);
	unsigned int banks, ranks, columns, rows, active_ranks, width;
	resource_size_t size;

	if (is_imx8mn())
		banks = 1;
	else
		banks = 2;
	ranks = 0;

	switch (FIELD_GET(DDRC_MSTR_ACTIVE_RANKS, mstr)) {
	case 0b0001:
		active_ranks = 1;
		break;
	case 0b0011:
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
	if (active_ranks > 1 &&
	    (FIELD_GET(DDRC_ADDRMAP0_CS_BIT0, addrmap[0]) != 0b11111))
			ranks++;

	if (FIELD_GET(DDRC_ADDRMAP1_BANK_B2, addrmap[1]) != 0b11111)
		banks++;

	columns	= imx_ddrc_count_bits(col_max, col_b, col_b_num);
	rows	= imx_ddrc_count_bits(row_max, row_b, row_b_num);

	size = ((u64)(1 << banks) * width << (rows + columns)) << ranks;

	return reduced_address_space ? size * 3 / 4 : size;
}

resource_size_t imx8m_ddrc_sdram_size(void)
{
	void __iomem *mem_base = (void __iomem *)MEMCTL_BASE;

	const u32 addrmap[] = {
		readl(mem_base + DDRC_ADDRMAP(0)),
		readl(mem_base + DDRC_ADDRMAP(1)),
		readl(mem_base + DDRC_ADDRMAP(2)),
		readl(mem_base + DDRC_ADDRMAP(3)),
		readl(mem_base + DDRC_ADDRMAP(4)),
		readl(mem_base + DDRC_ADDRMAP(5)),
		readl(mem_base + DDRC_ADDRMAP(6)),
		readl(mem_base + DDRC_ADDRMAP(7))
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

	return imx_ddrc_sdram_size(mem_base, addrmap,
					12, col_b, sizeof(col_b),
					16, row_b, sizeof(row_b),
					reduced_address_space);
}
