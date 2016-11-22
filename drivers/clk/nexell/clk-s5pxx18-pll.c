/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Youngbok, Park <ybpark@nexell.co.kr>
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

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <asm/tlbflush.h>
#include "clk-s5pxx18.h"
#include "clk-s5pxx18-pll.h"

#define PLL_LOCKING_TIME 100

struct pll_pms {
	long rate; /* unint Khz */
	int P;
	int M;
	int S;
};

struct clk_core {
	const char *name;
	int id, div, pll;
	unsigned long rate;
	struct clk_hw hw;
	struct pll_pms pms;
};

#ifdef CONFIG_ARCH_S5P4418
/* PLL 0,1 */
static struct pll_pms pll0_1_pms[] = {
	[0] = {
		.rate = 1200000, .P = 3, .M = 300, .S = 1,
	},
	[1] = {
		.rate = 1100000, .P = 3, .M = 275, .S = 1,
	},
	[2] = {
		.rate = 1000000, .P = 3, .M = 250, .S = 1,
	},
	[3] = {
		.rate = 900000, .P = 3, .M = 225, .S = 1,
	},
	[4] = {
		.rate = 800000, .P = 3, .M = 200, .S = 1,
	},
	[5] = {
		.rate = 700000, .P = 3, .M = 175, .S = 1,
	},
	[6] = {
		.rate = 600000, .P = 2, .M = 200, .S = 2,
	},
	[7] = {
		.rate = 500000, .P = 3, .M = 250, .S = 2,
	},
	[8] = {
		.rate = 400000, .P = 3, .M = 200, .S = 2,
	},
	[9] = {
		.rate = 300000, .P = 2, .M = 200, .S = 3,
	},
	[10] = {
		.rate = 200000, .P = 3, .M = 200, .S = 3,
	},
	[11] = {
		.rate = 100000, .P = 3, .M = 200, .S = 4,
	},
};

/* PLL 2,3 */
static struct pll_pms pll2_3_pms[] = {
	[0] = {
		.rate = 1200000, .P = 3, .M = 300, .S = 1,
	},
	[1] = {
		.rate = 1100000, .P = 3, .M = 275, .S = 1,
	},
	[2] = {
		.rate = 1000000, .P = 3, .M = 250, .S = 1,
	},
	[3] = {
		.rate = 900000, .P = 3, .M = 225, .S = 1,
	},
	[4] = {
		.rate = 800000, .P = 3, .M = 200, .S = 1,
	},
	[5] = {
		.rate = 700000, .P = 3, .M = 175, .S = 1,
	},
	[6] = {
		.rate = 600000, .P = 3, .M = 150, .S = 1,
	},
	[7] = {
		.rate = 500000, .P = 3, .M = 250, .S = 2,
	},
	[8] = {
		.rate = 400000, .P = 3, .M = 200, .S = 2,
	},
	[9] = {
		.rate = 300000, .P = 3, .M = 150, .S = 2,
	},
	[10] = {
		.rate = 200000, .P = 3, .M = 200, .S = 3,
	},
	[11] = {
		.rate = 100000, .P = 3, .M = 200, .S = 4,
	},
};
#else	/* S5P6818 */
/* PLL 0,1 */
static struct pll_pms pll0_1_pms[] = {
	[0] = {
		.rate = 1600000, .P = 6, .M = 400, .S = 0,
	},
	[1] = {
		.rate = 1500000, .P = 6, .M = 375, .S = 0,
	},
	[2] = {
		.rate = 1400000, .P = 6, .M = 350, .S = 0,
	},
	[3] = {
		.rate = 1300000, .P = 6, .M = 325, .S = 0,
	},
	[4] = {
		.rate = 1200000, .P = 3, .M = 300, .S = 1,
	},
	[5] = {
		.rate = 1100000, .P = 3, .M = 275, .S = 1,
	},
	[6] = {
		.rate = 1000000, .P = 3, .M = 250, .S = 1,
	},
	[7] = {
		.rate = 900000, .P = 3, .M = 225, .S = 1,
	},
	[8] = {
		.rate = 800000, .P = 3, .M = 200, .S = 1,
	},
	[9] = {
		.rate = 700000, .P = 3, .M = 175, .S = 1,
	},
	[10] = {
		.rate = 600000, .P = 2, .M = 200, .S = 2,
	},
	[11] = {
		.rate = 500000, .P = 3, .M = 250, .S = 2,
	},
	[12] = {
		.rate = 400000, .P = 3, .M = 200, .S = 2,
	},
	[13] = {
		.rate = 300000, .P = 2, .M = 200, .S = 3,
	},
	[14] = {
		.rate = 200000, .P = 3, .M = 200, .S = 3,
	},
	[15] = {
		.rate = 100000, .P = 3, .M = 200, .S = 4,
	},
};

/* PLL 2,3 */
static struct pll_pms pll2_3_pms[] = {
	[0] = {
		.rate = 1600000, .P = 3, .M = 400, .S = 1,
	},
	[1] = {
		.rate = 1500000, .P = 3, .M = 375, .S = 1,
	},
	[2] = {
		.rate = 1400000, .P = 3, .M = 350, .S = 1,
	},
	[3] = {
		.rate = 1300000, .P = 3, .M = 325, .S = 1,
	},
	[4] = {
		.rate = 1200000, .P = 3, .M = 300, .S = 1,
	},
	[5] = {
		.rate = 1100000, .P = 3, .M = 275, .S = 1,
	},
	[6] = {
		.rate = 1000000, .P = 3, .M = 250, .S = 1,
	},
	[7] = {
		.rate = 900000, .P = 3, .M = 225, .S = 1,
	},
	[8] = {
		.rate = 800000, .P = 3, .M = 200, .S = 1,
	},
	[9] = {
		.rate = 700000, .P = 3, .M = 175, .S = 1,
	},
	[10] = {
		.rate = 600000, .P = 3, .M = 150, .S = 1,
	},
	[11] = {
		.rate = 500000, .P = 3, .M = 250, .S = 2,
	},
	[12] = {
		.rate = 400000, .P = 3, .M = 200, .S = 2,
	},
	[13] = {
		.rate = 300000, .P = 3, .M = 150, .S = 2,
	},
	[14] = {
		.rate = 200000, .P = 3, .M = 200, .S = 3,
	},
	[15] = {
		.rate = 100000, .P = 3, .M = 200, .S = 4,
	},
};
#endif

#define PLL0_1_SIZE ARRAY_SIZE(pll0_1_pms)
#define PLL2_3_SIZE ARRAY_SIZE(pll2_3_pms)

#define PMS_RATE(p, i) ((&p[i])->rate)
#define PMS_P(p, i) ((&p[i])->P)
#define PMS_M(p, i) ((&p[i])->M)
#define PMS_S(p, i) ((&p[i])->S)

#define PLL_S_BITPOS 0
#define PLL_M_BITPOS 8
#define PLL_P_BITPOS 18

static void *ref_clk_base;
static spinlock_t pll_lock = __SPIN_LOCK_UNLOCKED(pll_lock);

static void nx_pll_set_rate(int PLL, int P, int M, int S)
{
	struct reg_clkpwr *reg = ref_clk_base;
	unsigned long flags;

	spin_lock_irqsave(&pll_lock, flags);

	/*
	 * 1. change PLL0 clock to Oscillator Clock
	 */
	reg->PLLSETREG[PLL] &= ~(1 << 28); /* pll bypass on, xtal clock use */
	reg->CLKMODEREG0 = (1 << PLL);     /* update pll */

	while (readl(&reg->CLKMODEREG0) & (1 << 31))
		;/* wait for change update pll*/

	/*
	 * 2. PLL Power Down & PMS value setting
	 */
	reg->PLLSETREG[PLL] =
		((1UL << 29 |   /* power down */
		  (0UL << 28) | /* clock bypass on, xtal clock use */
		  (S << PLL_S_BITPOS) |
		  (M << PLL_M_BITPOS) |
		  (P << PLL_P_BITPOS)));
	reg->CLKMODEREG0 = (1 << PLL); /* update pll */

	while (readl(&reg->CLKMODEREG0) & (1 << 31))
		; /* wait for change update pll */

	udelay(10);

	/*
	 * 3. Update PLL & wait PLL locking
	 */
	reg->PLLSETREG[PLL] &= ~((u32)(1UL << 29));/* pll power up */
	reg->CLKMODEREG0 = (1 << PLL);		   /* update pll */

	while (readl(&reg->CLKMODEREG0) & (1 << 31))
		;/* wait for change update pll */

	udelay(PLL_LOCKING_TIME);/* 1000us */

	/*
	 * 4. Change to PLL clock
	 */
	reg->PLLSETREG[PLL] |= (1 << 28);/* pll bypass off, pll clock use */
	reg->CLKMODEREG0 = (1 << PLL);   /* update pll */

	while (readl(&reg->CLKMODEREG0) & (1 << 31))
		;/* wait for change update pll */

	spin_unlock_irqrestore(&pll_lock, flags);
}

#ifdef CONFIG_ARM_S5Pxx18_DEVFREQ
#if defined(CONFIG_ARCH_S5P4418)
asmlinkage int __invoke_nexell_fn_smc(u32, u32, u32, u32);
#endif

int nx_change_bus_freq(u32 pll_data)
{
#if defined(CONFIG_ARCH_S5P6818)
	uint32_t pll_num = pll_data & 0x00000003;
	uint32_t s       = (pll_data & 0x000000fc) >> 2;
	uint32_t m       = (pll_data & 0x00ffff00) >> 8;
	uint32_t p       = (pll_data & 0xff000000) >> 24;

	nx_pll_set_rate(pll_num, p, m, s);
	return 0;
#else
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&pll_lock, flags);
	ret = __invoke_nexell_fn_smc(0x82000009, pll_data, 0, 0);
	spin_unlock_irqrestore(&pll_lock, flags);

	return ret;
#endif
}
EXPORT_SYMBOL(nx_change_bus_freq);
#endif	/* End of CONFIG_ARM_S5Pxx18_DEVFREQ */

static unsigned long pll_round_rate(int pllno, unsigned long rate, int *p,
				    int *m, int *s)
{
	struct pll_pms *pms;
	int len, idx = 0, n = 0, l = 0;
	long freq = 0;

	rate /= 1000;
	pr_debug("PLL.%d, %ld", pllno, rate);

	switch (pllno) {
	case 0:
	case 1:
		pms = pll0_1_pms;
		len = PLL0_1_SIZE;
		break;
	case 2:
	case 3:
		pms = pll2_3_pms;
		len = PLL2_3_SIZE;
		break;
	default:
		pr_info("Not support pll.%d (0~3)\n", pllno);
		return 0;
	}

	/* array index so -1 */
	idx = (len / 2) - 1;

	while (1) {
		l = n + idx;
		freq = PMS_RATE(pms, l);
		if (freq == rate)
			break;

		if (rate > freq)
			len -= idx, idx >>= 1;
		else
			n += idx, idx = (len - n - 1) >> 1;

		if (0 == idx) {
			int k = l;

			if (abs(rate - freq) > abs(rate - PMS_RATE(pms, k + 1)))
				k += 1;

			if (abs(rate - PMS_RATE(pms, k)) >=
			    abs(rate - PMS_RATE(pms, k - 1)))
				k -= 1;

			l = k;
			break;
		}
	}

	if (p)
		*p = PMS_P(pms, l);
	if (m)
		*m = PMS_M(pms, l);
	if (s)
		*s = PMS_S(pms, l);

	pr_debug("(real %ld Khz, P=%d ,M=%3d, S=%d)\n", PMS_RATE(pms, l),
		 PMS_P(pms, l), PMS_M(pms, l), PMS_S(pms, l));

	return (PMS_RATE(pms, l) * 1000);
}

static unsigned long ref_clk = 24000000UL;

#define getquotient(v, d) (v / d)

#define DIV_CPUG0 0
#define DIV_BUS 1
#define DIV_MEM 2
#define DIV_G3D 3
#define DIV_VPU 4
#define DIV_DISP 5
#define DIV_HDMI 6
#define DIV_CPUG1 7
#define DIV_CCI4 8

#define DVO0 3
#define DVO1 9
#define DVO2 15
#define DVO3 21

static inline unsigned int pll_rate(unsigned int pllN, unsigned int xtal)
{
	struct reg_clkpwr *reg = ref_clk_base;
	unsigned int val, val1, nP, nM, nS, nK;
	unsigned int temp = 0;

	val = reg->PLLSETREG[pllN];
	val1 = reg->PLLSETREG_SSCG[pllN];
	xtal /= 1000; /* Unit Khz */

	nP = (val >> 18) & 0x03F;
	nM = (val >> 8) & 0x3FF;
	nS = (val >> 0) & 0x0FF;
	nK = (val1 >> 16) & 0xFFFF;

	if ((pllN > 1) && nK)
		temp =	(unsigned int)(
			getquotient((getquotient((nK * 1000), 65536) * xtal),
			nP) >>  nS);

	return (unsigned int)((getquotient((nM * xtal), nP) >> nS) * 1000) +
		temp;
}

static inline unsigned int pll_dvo(int dvo)
{
	struct reg_clkpwr *reg = ref_clk_base;

	return (reg->DVOREG[dvo] & 0x7);
}

static inline unsigned int pll_div(int dvo)
{
	struct reg_clkpwr *reg = ref_clk_base;
	unsigned int val = reg->DVOREG[dvo];

	return ((((val >> DVO3) & 0x3F) + 1) << 24) |
		((((val >> DVO2) & 0x3F) + 1) << 16) |
		((((val >> DVO1) & 0x3F) + 1) << 8) |
		((((val >> DVO0) & 0x3F) + 1) << 0);
}

#define PLLN_RATE(n) (pll_rate(n, ref_clk)) /* 0~ 3 */
#define CPU_FCLK_RATE(n)                                                       \
	(pll_rate(pll_dvo(n), ref_clk) / ((pll_div(n) >> 0) & 0x3F))
#define CPU_HCLK_RATE(n)                                                       \
	(pll_rate(pll_dvo(n), ref_clk) / ((pll_div(n) >> 0) & 0x3F) /          \
	 ((pll_div(n) >> 8) & 0x3F))
#define MEM_FCLK_RATE()                                                        \
	(pll_rate(pll_dvo(DIV_MEM), ref_clk) /                                 \
	 ((pll_div(DIV_MEM) >> 0) & 0x3F) / ((pll_div(DIV_MEM) >> 8) & 0x3F))
#define MEM_DCLK_RATE()                                                        \
	(pll_rate(pll_dvo(DIV_MEM), ref_clk) / ((pll_div(DIV_MEM) >> 0) & 0x3F))
#define MEM_BCLK_RATE()                                                        \
	(pll_rate(pll_dvo(DIV_MEM), ref_clk) /                                 \
	 ((pll_div(DIV_MEM) >> 0) & 0x3F) / ((pll_div(DIV_MEM) >> 8) & 0x3F) / \
	 ((pll_div(DIV_MEM) >> 16) & 0x3F))
#define MEM_PCLK_RATE()                                                        \
	(pll_rate(pll_dvo(DIV_MEM), ref_clk) /                                 \
	 ((pll_div(DIV_MEM) >> 0) & 0x3F) / ((pll_div(DIV_MEM) >> 8) & 0x3F) / \
	 ((pll_div(DIV_MEM) >> 16) & 0x3F) /                                   \
	 ((pll_div(DIV_MEM) >> 24) & 0x3F))
#define BUS_BCLK_RATE()                                                        \
	(pll_rate(pll_dvo(DIV_BUS), ref_clk) / ((pll_div(DIV_BUS) >> 0) & 0x3F))
#define BUS_PCLK_RATE()                                                        \
	(pll_rate(pll_dvo(DIV_BUS), ref_clk) /                                 \
	 ((pll_div(DIV_BUS) >> 0) & 0x3F) / ((pll_div(DIV_BUS) >> 8) & 0x3F))
#define G3D_BCLK_RATE()                                                        \
	(pll_rate(pll_dvo(DIV_G3D), ref_clk) / ((pll_div(DIV_G3D) >> 0) & 0x3F))
#define VPU_BCLK_RATE()                                                        \
	(pll_rate(pll_dvo(DIV_VPU), ref_clk) / ((pll_div(DIV_VPU) >> 0) & 0x3F))
#define VPU_PCLK_RATE()                                                        \
	(pll_rate(pll_dvo(DIV_VPU), ref_clk) /                                 \
	 ((pll_div(DIV_VPU) >> 0) & 0x3F) / ((pll_div(DIV_VPU) >> 8) & 0x3F))
#define DIS_BCLK_RATE()                                                        \
	(pll_rate(pll_dvo(DIV_DISP), ref_clk) /                                \
	 ((pll_div(DIV_DISP) >> 0) & 0x3F))
#define DIS_PCLK_RATE()                                                        \
	(pll_rate(pll_dvo(DIV_DISP), ref_clk) /                                \
	 ((pll_div(DIV_DISP) >> 0) & 0x3F) /                                   \
	 ((pll_div(DIV_DISP) >> 8) & 0x3F))
#define HDMI_PCLK_RATE()                                                       \
	(pll_rate(pll_dvo(DIV_HDMI), ref_clk) /                                \
	 ((pll_div(DIV_HDMI) >> 0) & 0x3F))
#define CCI_BCLK_RATE()                                                        \
	(pll_rate(pll_dvo(DIV_CCI4), ref_clk) /                                \
	 ((pll_div(DIV_CCI4) >> 0) & 0x3F))
#define CCI_PCLK_RATE()                                                        \
	(pll_rate(pll_dvo(DIV_CCI4), ref_clk) /                                \
	 ((pll_div(DIV_CCI4) >> 0) & 0x3F) /                                   \
	 ((pll_div(DIV_CCI4) >> 8) & 0x3F))

/*
 *	core frequency clk interface
 */
static struct clk_core clk_pll_dev[] = {
	[ID_CPU_PLL0] =	{
		.id = ID_CPU_PLL0, .name = CLK_CPU_PLL0,
	},
	[ID_CPU_PLL1] =	{
		.id = ID_CPU_PLL1, .name = CLK_CPU_PLL1,
	},
	[ID_CPU_PLL2] =	{
		.id = ID_CPU_PLL2, .name = CLK_CPU_PLL2,
	},
	[ID_CPU_PLL3] =	{
		.id = ID_CPU_PLL3, .name = CLK_CPU_PLL3,
	},
	[ID_CPU_FCLK] = {.id = ID_CPU_FCLK,
		.name = CLK_CPU_FCLK,
		.div = DIV_CPUG0},
	[ID_CPU_HCLK] = {.id = ID_CPU_HCLK,
		.name = CLK_CPU_HCLK,
		.div = DIV_CPUG0},
	[ID_MEM_FCLK] = {.id = ID_MEM_FCLK,
		.name = CLK_MEM_FCLK,
		.div = DIV_MEM},
	[ID_MEM_DCLK] = {.id = ID_MEM_DCLK,
		.name = CLK_MEM_DCLK,
		.div = DIV_MEM},
	[ID_MEM_BCLK] = {.id = ID_MEM_BCLK,
		.name = CLK_MEM_BCLK,
		.div = DIV_MEM},
	[ID_MEM_PCLK] = {.id = ID_MEM_PCLK,
		.name = CLK_MEM_PCLK,
		.div = DIV_MEM},
	[ID_BUS_BCLK] = {.id = ID_BUS_BCLK,
		.name = CLK_BUS_BCLK,
		.div = DIV_BUS},
	[ID_BUS_PCLK] = {.id = ID_BUS_PCLK,
		.name = CLK_BUS_PCLK,
		.div = DIV_BUS},
	[ID_VPU_BCLK] = {.id = ID_VPU_BCLK,
		.name = CLK_VPU_BCLK,
		.div = DIV_VPU},
	[ID_VPU_PCLK] = {.id = ID_VPU_PCLK,
		.name = CLK_VPU_PCLK,
		.div = DIV_VPU},
	[ID_DIS_BCLK] = {.id = ID_DIS_BCLK,
		.name = CLK_DIS_BCLK,
		.div = DIV_DISP},
	[ID_DIS_PCLK] = {.id = ID_DIS_PCLK,
		.name = CLK_DIS_PCLK,
		.div = DIV_DISP},
	[ID_CCI_BCLK] = {.id = ID_CCI_BCLK,
		.name = CLK_CCI_BCLK,
		.div = DIV_CCI4},
	[ID_CCI_PCLK] = {.id = ID_CCI_PCLK,
		.name = CLK_CCI_PCLK,
		.div = DIV_CCI4},
	[ID_G3D_BCLK] = {.id = ID_G3D_BCLK,
		.name = CLK_G3D_BCLK,
		.div = DIV_G3D},
};

static unsigned long clk_pll_recalc_rate(struct clk_hw *hw, unsigned long rate)
{
	struct clk_core *clk_data = to_clk_core(hw);
	int id = clk_data->id;

	switch (id) {
	case ID_CPU_PLL0:
		rate = PLLN_RATE(0);
		break;
	case ID_CPU_PLL1:
		rate = PLLN_RATE(1);
		break;
	case ID_CPU_PLL2:
		rate = PLLN_RATE(2);
		break;
	case ID_CPU_PLL3:
		rate = PLLN_RATE(3);
		break;
	case ID_CPU_FCLK:
		rate = CPU_FCLK_RATE(DIV_CPUG0);
		break;
	case ID_CPU_HCLK:
		rate = CPU_HCLK_RATE(DIV_CPUG0);
		break;
	case ID_MEM_FCLK:
		rate = MEM_FCLK_RATE();
		break;
	case ID_BUS_BCLK:
		rate = BUS_BCLK_RATE();
		break;
	case ID_BUS_PCLK:
		rate = BUS_PCLK_RATE();
		break;
	case ID_MEM_DCLK:
		rate = MEM_DCLK_RATE();
		break;
	case ID_MEM_BCLK:
		rate = MEM_BCLK_RATE();
		break;
	case ID_MEM_PCLK:
		rate = MEM_PCLK_RATE();
		break;
	case ID_G3D_BCLK:
		rate = G3D_BCLK_RATE();
		break;
	case ID_VPU_BCLK:
		rate = VPU_BCLK_RATE();
		break;
	case ID_VPU_PCLK:
		rate = VPU_PCLK_RATE();
		break;
	case ID_DIS_BCLK:
		rate = DIS_BCLK_RATE();
		break;
	case ID_DIS_PCLK:
		rate = DIS_PCLK_RATE();
		break;
	case ID_CCI_BCLK:
		rate = CCI_BCLK_RATE();
		break;
	case ID_CCI_PCLK:
		rate = CCI_PCLK_RATE();
		break;
	default:
		pr_info("Unknown clock ID [%d] ...\n", id);
		break;
	}

	pr_debug("%s: name %s id %d rate %ld\n", __func__, clk_data->name,
		 clk_data->id, rate);
	return rate;
}

static long clk_pll_round_rate(struct clk_hw *hw, unsigned long drate,
			       unsigned long *prate)
{
	struct clk_core *clk_data = to_clk_core(hw);
	struct pll_pms *pms = &clk_data->pms;
	int id = clk_data->id;
	long rate = 0;

	/* clear P,M,S */
	pms->P = 0, pms->M = 0, pms->S = 0;
	rate = pll_round_rate(id, drate, &pms->P, &pms->M, &pms->S);

	pr_debug("%s: name %s id %d (%lu, %lu) <%d,%d,%d>\n", __func__,
		 clk_data->name, id, drate, rate, pms->P, pms->M, pms->S);
	return rate;
}

static int clk_pll_set_rate(struct clk_hw *hw, unsigned long drate,
			    unsigned long prate)
{
	struct clk_core *clk_data = to_clk_core(hw);
	struct pll_pms *pms = &clk_data->pms;
	int id = clk_data->id;
	long rate = drate;

	if (!pms->P && !pms->M && !pms->S)
		rate = pll_round_rate(id, drate, &pms->P, &pms->M, &pms->S);

	pr_debug("%s: name %s id %d (%lu, %lu) <%d,%d,%d>\n", __func__,
		 clk_data->name, id, drate, rate, pms->P, pms->M, pms->S);
	nx_pll_set_rate(id, pms->P, pms->M, pms->S);

	/* clear P,M,S */
	pms->P = 0, pms->M = 0, pms->S = 0;

	return 0;
}

static const struct clk_ops clk_pll_ops = {
	.recalc_rate = clk_pll_recalc_rate,
	.round_rate = clk_pll_round_rate,
	.set_rate = clk_pll_set_rate,
};

static const struct clk_ops clk_dev_ops = {
	.recalc_rate = clk_pll_recalc_rate,
};

static struct clk *clk_pll_clock_register(const char *name,
					  const char *parent_name,
					  struct clk_hw *hw,
					  const struct clk_ops *ops,
					  unsigned long flags)
{
	struct clk *clk;
	struct clk_init_data init;

	init.name = name;
	init.ops = ops;
	init.flags = flags;
	init.parent_names = (parent_name ? &parent_name : NULL);
	init.num_parents = parent_name ? 1 : 0;
	hw->init = &init;

	clk = clk_register(NULL, hw);
	if (IS_ERR(clk)) {
		pr_err("%s: failed to register pll clock %s\n", __func__,
		       init.name);
		return NULL;
	}

	if (clk_register_clkdev(clk, init.name, NULL))
		pr_err("%s: failed to register lookup for %s", __func__,
		       init.name);

	return clk;
}

static void __init clk_pll_sysclk_setup(struct device_node *np)
{
	struct clk *clk;
	unsigned long flags = CLK_IS_ROOT;/* | CLK_GET_RATE_NOCACHE; */
	unsigned long rate[ID_CPU_FCLK];
	int i;

	for (i = 0; i < ID_CPU_FCLK; i++) {
		clk = clk_pll_clock_register(clk_pll_dev[i].name, NULL,
					     &clk_pll_dev[i].hw, &clk_pll_ops,
					     flags);
		if (NULL == clk)
			continue;
		rate[i] = clk_get_rate(clk);
	}
	pr_info("PLL : [0] = %10lu, [1] = %10lu, [2] = %10lu, [3] = %10lu\n",
	       rate[0], rate[1], rate[2], rate[3]);
}

static void __init clk_pll_of_clocks_setup(struct device_node *node)
{
	struct clk_core *clk_data = NULL;
	struct clk *clk;
	unsigned long flags = CLK_IS_BASIC;
	const char *parent_name = NULL;
	int i, div, pll;

	for (i = ID_CPU_FCLK; ID_END > i; i++) {
		clk_data = &clk_pll_dev[i];
		div = clk_data->div;
		pll = pll_dvo(div);
		clk_data->pll = pll;
		parent_name = clk_pll_dev[pll].name;

		clk = clk_pll_clock_register(clk_data->name, parent_name,
				       &clk_data->hw, &clk_dev_ops, flags);
		if (clk)
			clk_data->rate = clk_get_rate(clk);
	}
}

static void __init clk_pll_of_clocks_dump(struct device_node *np)
{
	struct clk_core *clk_data = clk_pll_dev;
	int pll = pll_dvo(DIV_CPUG1);

	/* cpu 0, 1  : div 0, 7 */
	pr_info("(%d) PLL%d: CPU  FCLK = %10lu, HCLK = %9lu (G0)\n",
	       clk_data[ID_CPU_FCLK].div, clk_data[ID_CPU_FCLK].pll,
	       clk_data[ID_CPU_FCLK].rate, clk_data[ID_CPU_HCLK].rate);

	pr_info("(%d) PLL%d: CPU  FCLK = %10lu, HCLK = %9lu (G1)\n", DIV_CPUG1,
	       pll, (ulong)CPU_FCLK_RATE(DIV_CPUG1),
	       (ulong)CPU_HCLK_RATE(DIV_CPUG1));

	/* memory */
	pr_info("(%d) PLL%d: MEM  FCLK = %10lu, DCLK = %9lu, BCLK = %9lu,",
	       clk_data[ID_MEM_FCLK].div, clk_data[ID_MEM_FCLK].pll,
	       clk_data[ID_MEM_FCLK].rate, clk_data[ID_MEM_DCLK].rate,
	       clk_data[ID_MEM_BCLK].rate);

	pr_info("PCLK = %9lu\n", clk_data[ID_MEM_PCLK].rate);

	/* bus */
	pr_info("(%d) PLL%d: BUS  BCLK = %10lu, PCLK = %9lu\n",
	       clk_data[ID_BUS_BCLK].div, clk_data[ID_BUS_BCLK].pll,
	       clk_data[ID_BUS_BCLK].rate, clk_data[ID_BUS_PCLK].rate);

	/* cci400 */
	pr_info("(%d) PLL%d: CCI4 BCLK = %10lu, PCLK = %9lu\n",
	       clk_data[ID_CCI_BCLK].div, clk_data[ID_CCI_BCLK].pll,
	       clk_data[ID_CCI_BCLK].rate, clk_data[ID_CCI_PCLK].rate);

	/* 3d graphic */
	pr_info("(%d) PLL%d: G3D  BCLK = %10lu\n", clk_data[ID_G3D_BCLK].div,
	       clk_data[ID_G3D_BCLK].pll, clk_data[ID_G3D_BCLK].rate);

	/* coda (vpu) */
	pr_info("(%d) PLL%d: VPU  BCLK = %10lu, PCLK = %9lu\n",
	       clk_data[ID_VPU_BCLK].div, clk_data[ID_VPU_BCLK].pll,
	       clk_data[ID_VPU_BCLK].rate, clk_data[ID_VPU_PCLK].rate);

	/* display */
	pr_info("(%d) PLL%d: DISP BCLK = %10lu, PCLK = %9lu\n",
	       clk_data[ID_DIS_BCLK].div, clk_data[ID_DIS_BCLK].pll,
	       clk_data[ID_DIS_BCLK].rate, clk_data[ID_DIS_PCLK].rate);
}

static void __init clk_pll_of_setup(struct device_node *node)
{
	unsigned int pllin;
	struct resource regs;

	if (of_address_to_resource(node, 0, &regs) < 0) {
		pr_err("fail get clock pll regsister\n");
		return;
	}

	ref_clk_base = ioremap(regs.start, resource_size(&regs));
	if (ref_clk_base == NULL) {
		pr_err("fail get Clock control base address\n");
		return;
	}
	if (0 == of_property_read_u32(node, "ref-freuecny", &pllin))
		ref_clk = pllin;

	clk_pll_sysclk_setup(node);
	clk_pll_of_clocks_setup(node);
	clk_pll_of_clocks_dump(node);

	pr_info("CPU REF HZ: %lu hz (0x%08x:0x%p)\n", ref_clk, 0xc0010000,
		ref_clk_base);
}

CLK_OF_DECLARE(s5pxx18, "nexell,s5pxx18,pll", clk_pll_of_setup);
