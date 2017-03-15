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
#include "clk-nxp5540-pll.h"

#define PLL_LOCKING_TIME 100

struct pll_pms {
	long rate; /* unint Khz */
	int P;
	int M;
	int S;
	int K;
};

struct clk_core {
	const char *name;
	int id, div, pll;
	unsigned long rate;
	struct clk_hw hw;
	struct pll_pms pms;
};

#ifdef CONFIG_ARCH_NXP5540
/* PLL 0,1,4,5,6,7 */
static struct pll_pms pll_non_dithered_pms[] = {
	[0] = {
		.rate = 1600000, .P = 3, .M = 400, .S = 1, .K = 0
	},
	[1] = {
		.rate = 1500000, .P = 3, .M = 375, .S = 1, .K = 0
	},
	[2] = {
		.rate = 1400000, .P = 3, .M = 350, .S = 1, .K = 0
	},
	[3] = {
		.rate = 1300000, .P = 3, .M = 325, .S = 1, .K = 0
	},
	[4] = {
		.rate = 1200000, .P = 3, .M = 300, .S = 1, .K = 0
	},
	[5] = {
		.rate = 1100000, .P = 3, .M = 275, .S = 1, .K = 0
	},
	[6] = {
		.rate = 1000000, .P = 3, .M = 250, .S = 1, .K = 0
	},
	[7] = {
		.rate = 900000, .P = 3, .M = 225, .S = 1, .K = 0
	},
	[8] = {
		.rate = 800000, .P = 3, .M = 200, .S = 1, .K = 0
	},
	[9] = {
		.rate = 700000, .P = 3, .M = 175, .S = 1, .K = 0
	},
	[10] = {
		.rate = 600000, .P = 2, .M = 200, .S = 2, .K = 0
	},
	[11] = {
		.rate = 500000, .P = 3, .M = 250, .S = 2, .K = 0
	},
	[12] = {
		.rate = 400000, .P = 3, .M = 200, .S = 2, .K = 0
	},
	[13] = {
		.rate = 300000, .P = 2, .M = 200, .S = 3, .K = 0
	},
	[14] = {
		.rate = 200000, .P = 3, .M = 200, .S = 3, .K = 0
	},
	[15] = {
		.rate = 100000, .P = 3, .M = 200, .S = 4, .K = 0
	},
};

/* PLL 2,3 */
static struct pll_pms pll_dithered_pms[] = {
	[0] = {
		.rate = 1400000, .P = 3, .M = 350, .S = 1, .K = 0
	},
	[1] = {
		.rate = 1300000, .P = 3, .M = 325, .S = 1, .K = 0
	},
	[2] = {
		.rate = 1200000, .P = 3, .M = 300, .S = 1, .K = 0
	},
	[3] = {
		.rate = 1100000, .P = 3, .M = 275, .S = 1, .K = 0
	},
	[4] = {
		.rate = 1000000, .P = 3, .M = 250, .S = 1, .K = 0
	},
	[5] = {
		.rate = 900000, .P = 3, .M = 225, .S = 1, .K = 0
	},
	[6] = {
		.rate = 800000, .P = 3, .M = 200, .S = 1, .K = 0
	},
	[7] = {
		.rate = 700000, .P = 3, .M = 175, .S = 1, .K = 0
	},
	[8] = {
		.rate = 677376, .P = 3, .M = 169, .S = 1, .K = 22544
	},
	[9] = {
		.rate = 638976, .P = 2, .M = 106, .S = 1, .K = 32506
	},
	[10] = {
		.rate = 632217, .P = 3, .M = 158, .S = 1, .K = 3565
	},
	[11] = {
		.rate = 614400, .P = 2, .M = 102, .S = 1, .K = 26214
	},
	[12] = {
		.rate = 600000, .P = 3, .M = 150, .S = 1, .K = 0
	},
	[13] = {
		.rate = 589824, .P = 3, .M = 147, .S = 1, .K = 29884
	},
	[14] = {
		.rate = 500000, .P = 3, .M = 250, .S = 2, .K = 0
	},
	[15] = {
		.rate = 400000, .P = 3, .M = 200, .S = 2, .K = 0
	},
	[16] = {
		.rate = 300000, .P = 3, .M = 150, .S = 2, .K = 0
	},
	[17] = {
		.rate = 200000, .P = 3, .M = 200, .S = 3, .K = 0
	},
	[18] = {
		.rate = 100000, .P = 3, .M = 200, .S = 4, .K = 0
	},
};
#endif

#define PLL_NON_DITHERED_SIZE ARRAY_SIZE(pll_non_dithered_pms)
#define PLL_DITHERED_SIZE ARRAY_SIZE(pll_dithered_pms)

#define PMS_RATE(p, i) ((&p[i])->rate)
#define PMS_P(p, i) ((&p[i])->P)
#define PMS_M(p, i) ((&p[i])->M)
#define PMS_S(p, i) ((&p[i])->S)
#define PMS_K(p, i) ((&p[i])->K)

#define PLL_S_BITPOS 0
#define PLL_M_BITPOS 8
#define PLL_P_BITPOS 18
#define PLL_K_BITPOS 16

static void *ref_clk_base;
static spinlock_t pll_lock = __SPIN_LOCK_UNLOCKED(pll_lock);

static int bootup = 0;

static void nx_pll_delay(unsigned long delay)
{
	unsigned long i = 0;

	if (bootup)
		udelay(delay);
	else
		for (i = 0; i < delay * 1000; i++)
			nop();
}


static void nx_pll_set_rate(int PLL, int P, int M, int S, int K)
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

	if (PLL > 1)
		reg->PLLSETREG_SSCG[PLL] = (K << PLL_K_BITPOS) | (2<<0);

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


int nx_change_bus_freq(u32 pll_data)
{
	uint32_t pll_num = pll_data & 0x00000003;
	uint32_t s       = (pll_data & 0x000000fc) >> 2;
	uint32_t m       = (pll_data & 0x00ffff00) >> 8;
	uint32_t p       = (pll_data & 0xff000000) >> 24;
	uint32_t k	 = 0;

	nx_pll_set_rate(pll_num, p, m, s, k);
	return 0;
}
EXPORT_SYMBOL(nx_change_bus_freq);

#define PLL_NON_DITHERED_SIZE ARRAY_SIZE(pll_non_dithered_pms)
#define PLL_DITHERED_SIZE ARRAY_SIZE(pll_dithered_pms)

static unsigned long pll_round_rate(int pllno, unsigned long rate, int *p,
				    int *m, int *s, int *k)
{
	struct pll_pms *pms;
	int len, idx = 0, n = 0, l = 0;
	long freq = 0;

	rate /= 1000;
	pr_debug("PLL.%d, %ld", pllno, rate);

	switch (pllno) {
	case 0:
	case 1:
	case 4:
	case 5:
	case 6:
	case 7:
		pms = pll_non_dithered_pms;
		len = PLL_NON_DITHERED_SIZE;
		break;
	case 2:
	case 3:
		pms = pll_dithered_pms;
		len = PLL_DITHERED_SIZE;
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

		if (idx == 0) {
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
	if (k)
		*k = PMS_K(pms, l);

	pr_debug("(real %ld Khz, P=%d ,M=%3d, S=%d K=%d)\n", PMS_RATE(pms, l),
		 PMS_P(pms, l), PMS_M(pms, l), PMS_S(pms, l), PMS_K(pms, l));
	return (PMS_RATE(pms, l) * 1000);
}

static unsigned long ref_clk;

#define getquotient(v, d) (v / d)

#define DIV_CPUG0 0

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

#define PLLN_RATE(n) (pll_rate(n, ref_clk)) /* 0~ 3 */

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
	[ID_CPU_PLL4] =	{
		.id = ID_CPU_PLL4, .name = CLK_CPU_PLL4,
	},
	[ID_CPU_PLL5] =	{
		.id = ID_CPU_PLL5, .name = CLK_CPU_PLL5,
	},
	[ID_CPU_PLL6] =	{
		.id = ID_CPU_PLL6, .name = CLK_CPU_PLL6,
	},
	[ID_CPU_PLL7] =	{
		.id = ID_CPU_PLL7, .name = CLK_CPU_PLL7,
	},
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
	case ID_CPU_PLL4:
		rate = PLLN_RATE(4);
		break;
	case ID_CPU_PLL5:
		rate = PLLN_RATE(5);
		break;
	case ID_CPU_PLL6:
		rate = PLLN_RATE(6);
		break;
	case ID_CPU_PLL7:
		rate = PLLN_RATE(7);
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
	pms->P = 0, pms->M = 0, pms->S = 0, pms->K = 0;
	rate = pll_round_rate(id, drate, &pms->P, &pms->M, &pms->S, &pms->K);

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
		rate = pll_round_rate(id, drate, &pms->P, &pms->M, &pms->S,
				&pms->K);

	pr_debug("%s: name %s id %d (%lu, %lu) <%d,%d,%d,%d>\n", __func__,
		 clk_data->name, id, drate, rate,
		 pms->P, pms->M, pms->S, pms->K);
	nx_pll_set_rate(id, pms->P, pms->M, pms->S, pms->K);

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
	unsigned long rate[ID_END];
	int i;
	unsigned int test;
	unsigned long set_rate[ID_END] = { 0, };
	struct device_node *child;

	for_each_child_of_node(np, child) {

	if (!of_property_read_u32(child, "clock-frequency", &test))
		of_property_read_u32(child, "id", &i);
		set_rate[i] = test;
	}

	for (i = 0; i < ID_END; i++) {
		clk = clk_pll_clock_register(clk_pll_dev[i].name, NULL,
					     &clk_pll_dev[i].hw, &clk_pll_ops,
					     flags);
		if (clk == NULL)
			continue;

		if (set_rate[i])
			clk_set_rate(clk, set_rate[i]);

		rate[i] = clk_get_rate(clk);
	}
	pr_info("PLL : [0] = %10lu, [1] = %10lu, [2] = %10lu, [3] = %10lu\n",
	       rate[0], rate[1], rate[2], rate[3]);
	pr_info("PLL : [4] = %10lu, [5] = %10lu, [6] = %10lu, [7] = %10lu\n",
	       rate[4], rate[5], rate[6], rate[7]);
}

static void __init clk_pll_of_setup(struct device_node *node)
{
	struct resource regs;
	struct clk *clk;

	if (of_address_to_resource(node, 0, &regs) < 0) {
		pr_err("fail get clock pll regsister\n");
		return;
	}

	ref_clk_base = ioremap(regs.start, resource_size(&regs));
	if (ref_clk_base == NULL) {
		pr_err("fail get Clock control base address\n");
		return;
	}

	clk = __clk_lookup("fin_pll");
	ref_clk = clk_get_rate(clk);

	clk_pll_sysclk_setup(node);

	pr_info("CPU REF HZ: %lu hz (0x%08x:0x%p)\n", ref_clk, 0xc0010000,
		ref_clk_base);
	bootup = 1;
}

CLK_OF_DECLARE(s5pxx18, "nexell,s5pxx18,pll", clk_pll_of_setup);
