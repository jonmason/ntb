/*
 * Copyright (C) 2017  Nexell Co., Ltd.
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kernel.h>
#include <linux/module.h>

#define	HDMI_REG_BASE_CMU_SYS	(0x20100000)
#define	HDMI_REG_BASE_CMU_DISP	(0x20220000)
#define	HDMI_REG_BASE_CMU_HDMI	(0x20240000)

/*
 * Prototype/nx_base/nx_clockcontrol.c, .h
 * l4_sim/framework/Prototype/nx_base/nx_chip_sfr.c
 */
struct clock_manage {
	dma_addr_t dma_addr;
	void __iomem *cpu_addr;
	long size;
	const char *name;
	/* cmu_sys reset */
	struct clock_manage *reset;
	u32 e_offs;
	int e_bpos;
	u32 r_offs;
	int r_bpos;
};

struct clock_muxer {
	struct clock_manage *cmu;
	u32 offset;
	const char *name;
	int select;
	int gatemode;
	struct clock_divider {
		int divisor;
		u32 offset;	/* 0x40/0x80 */
	} divider[2];
};

struct clock_output {
	struct clock_muxer *muxer;
	int d_index;
	u32 e_offs;	/* reset mode */
	int e_bpos;
	u32 r_offs;	/* reset con */
	int r_bpos;
};

/*
 * @ clock manage units for display
 */
static struct clock_manage clk_manage_sys = {
	.name = "CMU_SYS",
	.dma_addr = HDMI_REG_BASE_CMU_SYS,
	.size = 0xa000,
};

static struct clock_manage clk_manage_disp = {
	.name = "CMU_DISP",
	.dma_addr = HDMI_REG_BASE_CMU_DISP,
	.size = 0xa000,
	.reset = &clk_manage_sys,
	.e_offs = 0x40c,		/* SYS_0_OSCCLK_CLK */
	.e_bpos = 2,			/* SYS_0_OSCCLK_CLK's enable cmu_disp */
	.r_offs = 0x8000,
	.r_bpos = 28,
};

static struct clock_manage clk_manage_hdmi = {
	.name = "CMU_HDMI",
	.dma_addr = HDMI_REG_BASE_CMU_HDMI,
	.size = 0xa000,
	.reset = &clk_manage_sys,
	.e_offs = 0x40c,		/* SYS_0_OSCCLK_CLK */
	.e_bpos = 6,			/* SYS_0_OSCCLK_CLK's enable cmu_hdmi */
	.r_offs = 0x8000,
	.r_bpos = 0,
};

/*
 * @ clock muxers for display
 */
static struct clock_muxer clk_mux_hdmiphy_pixel_0 = {
	.cmu = &clk_manage_sys,
	.name = "HDMIPHY_PIXEL_0_CLK",
	.offset = 0x1a00,	/* 0x20220000 + 200 */
	.select = 0,		/* 0 : OSCCLK_IN, 1: HDMIPHY_PIXEL_CLK */
};

static struct clock_muxer clk_mux_hdmiphy_tmds_0 = {
	.cmu = &clk_manage_sys,
	.name = "HDMIPHY_TMDS_0_CLK",
	.offset = 0x1c00,	/* 0x20220000 + 200 */
	.select = 0,		/* 0 : OSCCLK_IN, 1: HDMIPHY_TMDS_CLK */
};

static struct clock_muxer clk_mux_disp_0_axi = {
	.cmu = &clk_manage_disp,
	.name = "DISP_0_AXI_CLK",
	.offset = 0x0200,	/* 0x20220000 + 200 */
	.select = 0,		/* 0:S_PLL0 ~ 7:S_PLL7, 8:OSCCLK_IN */
	.divider[0] = {		/* disp_0_axi_clk */
		.offset = 0x40,
	},
	.divider[1] = {		/* disp_0_apb_clk */
		.offset = 0x80,
	},
};

static struct clock_muxer clk_mux_hdmi_0_axi  = {
	.cmu = &clk_manage_hdmi,
	.name = "HDMI_0_AXI_CLK",
	.offset = 0x0200,	/* 0x20240000 + 200 */
	.select = 0,		/* 0:S_PLL0 ~ 7:S_PLL7, 8:OSCCLK_IN */
	.divider[0] = {		/* hdmi_0_axi_clk */
		.offset = 0x40,
	},
	.divider[1] = {		/* hdmi_0_apb_clk */
		.offset = 0x80,
	},
};

static struct clock_muxer clk_mux_hdmiv2_0_tmds_10b  = {
	.cmu = &clk_manage_hdmi,
	.name = "HDMIV2_0_TMDS_10B_CLK",
	.offset = 0x0400,	/* 0x20240000 + 400 */
	.select = 0,		/*
				 * 0:OSCCLK_IN0, 1:S_PLL0 ~ 8:S_PLL7,
				 * 9:SCLK_HDMUPHY_PIXEL, 10:SCLK_HDMUPHY_TMDS
				 */
	.divider[0] = {		/* hdmiv2_0_tmds_10b_clk */
		.offset = 0x40,
	},
	.divider[1] = {		/* hdmiv2_0_tmds_20b_clk */
		.offset = 0x80,
	},
};

static struct clock_muxer clk_mux_hdmiv2_0_pixelx2  = {
	.cmu = &clk_manage_hdmi,
	.name = "HDMIV2_0_PIXELX2_CLK",
	.offset = 0x0600,	/* 0x20240000 + 600 */
	.select = 0,		/*
				 * 0:OSCCLK_IN0, 1:S_PLL0 ~ 8:S_PLL7,
				 * 9:SCLK_HDMUPHY_PIXEL, 10:SCLK_HDMUPHY_TMDS
				 */
	.divider[0] = {		/* hdmiv2_0_pixelx2_clk */
		.offset = 0x40,
	},
	.divider[1] = {		/* hdmiv2_0_pixelx_clk */
		.offset = 0x80,
	},
};

static struct clock_muxer clk_mux_hdmiv2_0_audio = {
	.cmu = &clk_manage_hdmi,
	.name = "HDMIV2_0_AUDIO_CLK",
	.offset = 0x0600,	/* 0x20240000 + 800 */
	.select = 0,		/*
				 * 0:OSCCLK_IN0, 1:S_PLL0 ~ 8:S_PLL7,
				 * 9:SCLK_HDMUPHY_PIXEL, 10:SCLK_HDMUPHY_TMDS
				 */
	.divider[0] = {		/* hdmiv2_0_audio_clk */
		.offset = 0x40,
	},
};

/*
 * @ clock output for display
 *   register : CLKENB_## name ##_clk
 */
#define	HDMICLK(_name, _mux, _dv, _eo, _eb, _ro, _rb)	\
	._name = {	\
	.muxer = &clk_mux_## _mux, .d_index = _dv,	\
	.e_offs = _eo, .e_bpos = _eb, .r_offs = _ro, .r_bpos = _rb,	\
	}

struct hdmi_clock {
	/* clock manage units */
	struct clock_manage *cmu_sys;
	struct clock_manage *cmu_disp;
	struct clock_manage *cmu_hdmi;
	int num_cmus;
	/* disp_0_axi */
	struct clock_output nxs2hdmi_0_axi;
	/* hdmi_0_axi */
	struct clock_output hdmi_0_axi;
	struct clock_output hdmiv2_0_axi;
	struct clock_output hdmi_0_apb;
	struct clock_output hdmiv2_0_apb;
	struct clock_output hdmiv2_0_apbphy;
	struct clock_output hdmiv2_0_phy;
	/* hdmiv2_0_tmds_10b */
	struct clock_output hdmiv2_0_tmds_10b;
	struct clock_output hdmiv2_0_tmds_20b;
	/* hdmiv2_0_pixelx2 */
	struct clock_output hdmiv2_0_pixelx2;
	struct clock_output hdmiv2_0_pixel;
	/* hdmiv2_0_audio */
	struct clock_output hdmiv2_0_audio;
	/* hdmiphy_pixel_0 */
	struct clock_output hdmiphy_pixel_0;
	struct clock_output bbus_hdmiphy_pixel_0;
	struct clock_output tbus_hdmiphy_pixel_0;
	struct clock_output lbus_hdmiphy_pixel_0;
	struct clock_output periclk_hdmiphy_pixel_0;
	/* hdmiphy_tmds_0 */
	struct clock_output hdmiphy_tmds_0;
	struct clock_output bbus_hdmiphy_tmds_0;
	struct clock_output tbus_hdmiphy_tmds_0;
	struct clock_output lbus_hdmiphy_tmds_0;
	struct clock_output periclk_hdmiphy_tmds_0;

} hdmi_clocks = {
	/*
	 * hdmi clock manage units
	 */
	.cmu_sys = &clk_manage_sys,
	.cmu_disp = &clk_manage_disp,
	.cmu_hdmi = &clk_manage_hdmi,
	.num_cmus = 3,

	/*
	 * clk_mux_disp_0_axi : 0x20220000 + 0x200
	 */
	HDMICLK(nxs2hdmi_0_axi, disp_0_axi, 0, 0x14, 21, 0x8, 21),

	/*
	 * clk_mux_hdmi_0_axi : 0x20240000 + 0x200
	 */
	HDMICLK(hdmi_0_axi, hdmi_0_axi, 0, 0x0c, 0, 0x0, 0),
	HDMICLK(hdmiv2_0_axi, hdmi_0_axi, 0, 0x0c, 1, 0x0, 1),
	HDMICLK(hdmi_0_apb, hdmi_0_axi, 1, 0x0c, 2, 0x0, -1),
	HDMICLK(hdmiv2_0_apb, hdmi_0_axi, 1, 0x0c, 3, 0x0, 2),
	HDMICLK(hdmiv2_0_apbphy, hdmi_0_axi, 1, 0x0c, 4, 0x0, 3),
	HDMICLK(hdmiv2_0_phy, hdmi_0_axi, 1, 0x0c, 5, 0x0, 4),

	/*
	 * clk_mux_hdmiv2_0_tmds_10b : 0x20240000 + 0x400
	 */
	HDMICLK(hdmiv2_0_tmds_10b, hdmiv2_0_tmds_10b, 0, 0x0c, 0, 0x0, 5),
	HDMICLK(hdmiv2_0_tmds_20b, hdmiv2_0_tmds_10b, 1, 0x0c, 1, 0x0, 6),

	/*
	 * clk_mux_hdmiv2_0_pixelx2 : 0x20240000 + 0x600
	 */
	HDMICLK(hdmiv2_0_pixelx2, hdmiv2_0_pixelx2, 0, 0x0c, 0, 0x0, 7),
	HDMICLK(hdmiv2_0_pixel, hdmiv2_0_pixelx2, 1, 0x0c, 1, 0x0, 8),

	/*
	 * clk_mux_hdmiv2_0_audio : 0x20240000 + 0x800
	 */
	HDMICLK(hdmiv2_0_audio, hdmiv2_0_audio, 0, 0x0c, 0, 0x0, 9),

	/*
	 * clk_mux_hdmiphy_pixel_0 : 0x20100000 + 0x1a00
	 */
	HDMICLK(hdmiphy_pixel_0, hdmiphy_pixel_0, -1, 0x0c, 0, 0x0, -1),
	HDMICLK(bbus_hdmiphy_pixel_0, hdmiphy_pixel_0, -1, 0x0c, 1, 0x0, -1),
	HDMICLK(tbus_hdmiphy_pixel_0, hdmiphy_pixel_0, -1, 0x0c, 2, 0x0, -1),
	HDMICLK(lbus_hdmiphy_pixel_0, hdmiphy_pixel_0, -1, 0x0c, 3, 0x0, -1),
	HDMICLK(periclk_hdmiphy_pixel_0, hdmiphy_pixel_0, -1, 0x0c, 4, 0x0, -1),

	/*
	 * clk_mux_hdmiphy_tmds_0 : 0x20100000 + 0x1c00
	 */
	HDMICLK(hdmiphy_tmds_0, hdmiphy_tmds_0, -1, 0x0c, 0, 0x0, -1),
	HDMICLK(bbus_hdmiphy_tmds_0, hdmiphy_tmds_0, -1, 0x0c, 1, 0x0, -1),
	HDMICLK(tbus_hdmiphy_tmds_0, hdmiphy_tmds_0, -1, 0x0c, 2, 0x0, -1),
	HDMICLK(lbus_hdmiphy_tmds_0, hdmiphy_tmds_0, -1, 0x0c, 3, 0x0, -1),
	HDMICLK(periclk_hdmiphy_tmds_0, hdmiphy_tmds_0, -1, 0x0c, 4, 0x0, -1),
};

static inline void __raw_set_bit(void __iomem *addr, int val, int bit_s)
{
	val  = readl(addr) & ~(1 << bit_s);
	val |= (val & 0x1) << bit_s;
	writel(val, addr);
}

static inline u32 __raw_get_bit(void __iomem *addr, int bit_s)
{
	u32 val = readl(addr) & (1 << bit_s);

	return (val >> bit_s);
}

static void __reset_cmu_deassert(struct clock_manage *cmu, bool release)
{
	struct clock_manage *reset = cmu->reset;
	void __iomem *e_addr;
	void __iomem *r_addr;
	int val = release ? 1 : 0;
	int eb, rb;

	if (!reset)
		return;

	/* clk enable */
	e_addr = reset->cpu_addr + cmu->e_offs;
	eb = cmu->e_bpos;
	pr_debug("cmu: enb %p, bs:%d\n", e_addr, eb);
	/* reset */
	r_addr = reset->cpu_addr + cmu->r_offs;
	rb = cmu->r_bpos;
	pr_debug("cmu: rst %p, bs:%d, %s\n",
		r_addr, rb, val ? "enter" : "exit");

	/* enter */
	if (!val) {
		__raw_set_bit(e_addr, 0, eb);
		__raw_set_bit(r_addr, 0, rb);
		__raw_set_bit(e_addr, 1, eb);
		__raw_set_bit(e_addr, 0, eb);
	/* release */
	} else {
		__raw_set_bit(e_addr, 0, eb);
		__raw_set_bit(r_addr, 1, rb);
		__raw_set_bit(e_addr, 1, eb);
	}
}

static void __reset_mode(struct clock_output *out, int mode)
{
	struct clock_muxer *muxer = out->muxer;
	void __iomem *addr;
	u32 offs = 0x8800;
	int val = mode ? 1 : 0;
	int bs;

	if (out->r_offs == -1)
		return;

	/* reset */
	addr = muxer->cmu->cpu_addr + offs + out->r_offs;
	bs = out->r_bpos;
	pr_debug("mod: rst %p, bs:%d, %s\n", addr, bs, val ? "async" : "sync");

	__raw_set_bit(addr, val, bs);
}

static void __reset_enable(struct clock_output *out, bool release)
{
	struct clock_muxer *muxer = out->muxer;
	void __iomem *addr;
	u32 offs = 0x8000;
	int val = release ? 1 : 0;
	int bs;

	if (out->r_offs == -1)
		return;

	/* reset */
	addr = muxer->cmu->cpu_addr + offs + out->r_offs;
	bs = out->r_bpos;
	pr_debug("out: rst %p, bs:%d, %s\n",
		addr, bs, val ? "assert" : "deassert");

	__raw_set_bit(addr, val, bs);
}

static void __clock_enable(struct clock_output *out, bool on)
{
	struct clock_muxer *muxer = out->muxer;
	void __iomem *addr;
	int val = on ? 1 : 0;
	int bs;

	/* enable */
	addr = muxer->cmu->cpu_addr + muxer->offset + out->e_offs;
	bs = out->e_bpos;
	pr_debug("out: enb %p, bs:%d, val:%d\n", addr, bs, val);

	__raw_set_bit(addr, val, bs);
}

static void __clock_source(struct clock_output *out, int source)
{
	struct clock_muxer *muxer = out->muxer;
	void __iomem *e_addr, *s_addr;
	u32 on;
	int bs;

	e_addr = muxer->cmu->cpu_addr + muxer->offset + out->e_offs;
	bs = out->e_bpos;
	on = __raw_get_bit(e_addr, bs);

	/* disable */
	if (on)
		__raw_set_bit(e_addr, 0, bs);

	/* mux */
	s_addr = muxer->cmu->cpu_addr + muxer->offset;
	pr_debug("mux: mux %p, val:%d\n", s_addr, source);

	__write_bitw(s_addr, source, 0, 4);

	/* enable */
	if (on)
		__raw_set_bit(e_addr, 1, bs);
}

static void __clock_divisor(struct clock_output *out, int divisor)
{
	struct clock_muxer *muxer = out->muxer;
	struct clock_divider *divider;
	void __iomem *addr;
	u32 busy, offs;

	if (out->d_index == -1)
		return;

	divider = &muxer->divider[out->d_index];
	if (!divider || !divider->offset)
		return;

	divider->divisor = divisor;
	offs = divider->offset + 0x08;	/* 0x48/0x88 */
	addr = muxer->cmu->cpu_addr + muxer->offset + offs;

	__write_bitw(addr, divisor, 0, 16);

	offs = divider->offset + 0x10;	/* 0x80/0x90 */
	addr = muxer->cmu->cpu_addr + muxer->offset + offs;

	do {
		busy = __raw_get_bit(addr, 0);
	} while (busy);
}
