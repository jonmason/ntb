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

#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/syscore_ops.h>

#include "clk-s5pxx18.h"

#define to_clk_dev(_hw) container_of(_hw, struct clk_dev, hw)

struct clk_dev_peri {
	const char *parent_name;
	const char *name;
	void __iomem *base;
	int id;
	int clk_step; /* 1S or 2S */
	bool enable;
	long rate;
	u32 in_mask;
	u32 in_mask1;
	/* clock register */
	int div_src_0;
	int div_val_0;
	int invert_0;
	int div_src_1;
	int div_val_1;
	int invert_1;
	int in_extclk_1;
	int in_extclk_2;
	int fix_src;
};

struct clk_dev {
	struct device_node *node;
	struct clk *clk;
	struct clk_hw hw;
	struct clk_dev_peri *peri;
	unsigned int rate;
	spinlock_t lock;
};

struct clk_dev_map {
	unsigned int con_enb;
	unsigned int con_gen[4];
};

#define MAX_DIVIDER ((1 << 8) - 1) /* 256, align 2 */

static inline void clk_dev_bclk(void *base, int on)
{
	struct clk_dev_map *reg = base;
	unsigned int val = readl(&reg->con_enb) & ~(0x3);

	val |= (on ? 3 : 0) & 0x3; /* always BCLK */
	writel(val, &reg->con_enb);
}

static inline void clk_dev_pclk(void *base, int on)
{
	struct clk_dev_map *reg = base;
	unsigned int val = 0;

	if (!on)
		return;

	val = readl(&reg->con_enb) & ~(1 << 3);
	val |= (1 << 3);
	writel(val, &reg->con_enb);
}

static inline void clk_dev_rate(void *base, int step, int src, int div)
{
	struct clk_dev_map *reg = base;
	unsigned int val = 0;

	val = readl(&reg->con_gen[step << 1]);
	val &= ~(0x07 << 2);
	val |= (src << 2); /* source */
	val &= ~(0xFF << 5);
	val |= (div - 1) << 5; /* divider */
	writel(val, &reg->con_gen[step << 1]);
}

static inline void clk_dev_inv(void *base, int step, int inv)
{
	struct clk_dev_map *reg = base;
	unsigned int val = readl(&reg->con_gen[step << 1]) & ~(1 << 1);

	val |= (inv << 1);
	writel(val, &reg->con_gen[step << 1]);
}

static inline void clk_dev_enb(void *base, int on)
{
	struct clk_dev_map *reg = base;
	unsigned int val = readl(&reg->con_enb) & ~(1 << 2);

	val |= ((on ? 1 : 0) << 2);
	writel(val, &reg->con_enb);
}

static inline long clk_dev_divide(long rate, long request, int align,
				  int *divide)
{
	int div = (rate / request);
	int max = MAX_DIVIDER & ~(align - 1);
	int adv = (div & ~(align - 1)) + align;

	if (!div) {
		if (divide)
			*divide = 1;
		return rate;
	}

	if (1 != div)
		div &= ~(align - 1);

	if (div != adv && abs(request - rate / div) > abs(request - rate / adv))
		div = adv;

	div = (div > max ? max : div);
	if (divide)
		*divide = div;

	return (rate / div);
}

static long clk_dev_bus_rate(struct clk_dev_peri *peri)
{
	struct clk *clk;
	const char *name = NULL;
	long rate = 0;

	if (I_PCLK_MASK & peri->in_mask)
		name = CLK_BUS_PCLK;
	else if (I_BCLK_MASK & peri->in_mask)
		name = CLK_BUS_PCLK;

	if (name) {
		clk = clk_get(NULL, name);
		rate = clk_get_rate(clk);
		clk_put(clk);
	}

	return rate ?: -EINVAL;
}

static long clk_dev_pll_rate(int no)
{
	struct clk *clk;
	char name[16];
	long rate = 0;

	sprintf(name, "pll%d", no);
	clk = clk_get(NULL, name);
	rate = clk_get_rate(clk);
	clk_put(clk);

	return rate;
}

static long dev_round_rate(struct clk_hw *hw, unsigned long rate)
{
	struct clk_dev_peri *peri = to_clk_dev(hw)->peri;
	unsigned long request = rate, new_rate = 0;
	unsigned long clock_hz, freq_hz = 0;
	unsigned int mask;
	int step, div[2] = {
		0,
	};
	int i, n, start_src = 0, max_src = 0, clk2 = 0;
	short s1 = 0, s2 = 0, d1 = 0, d2 = 0;

	step = peri->clk_step;
	mask = peri->in_mask;
	pr_debug("clk: %s request = %ld [input=0x%x]\n", peri->name, rate,
		 mask);

	if (!(mask & I_CLOCK_MASK))
		return clk_dev_bus_rate(peri);

next:
	if (peri->fix_src >= 0) {
		start_src = peri->fix_src;
		max_src = start_src + 1;
	} else {
		start_src = 0;
		max_src = I_CLOCK_NUM;
	}

	for (n = start_src ; n < max_src ; n++) {
		if (!(((mask & I_CLOCK_MASK) >> n) & 0x1))
			continue;

		if (I_EXT1_BIT == n)
			rate = peri->in_extclk_1;
		else if (I_EXT2_BIT == n)
			rate = peri->in_extclk_2;
		else
			rate = clk_dev_pll_rate(n);

		if (!rate)
			continue;

		clock_hz = rate;
		for (i = 0; step > i; i++)
			rate = clk_dev_divide(rate, request, 2, &div[i]);

		if (new_rate && (abs(rate - request) > abs(new_rate - request)))
			continue;

		pr_debug("clk: %s, pll.%d request[%ld] calc[%ld]\n", peri->name,
			 n, request, rate);

		if (clk2) {
			s1 = -1, d1 = -1; /* not use */
			s2 = n, d2 = div[0];
		} else {
			s1 = n, d1 = div[0];
			s2 = I_CLKn_BIT, d2 = div[1];
		}

		new_rate = rate;
		freq_hz = clock_hz;

		if (request == rate)
			break;
	}

	/* search 2th clock from input */
	if (!clk2 && abs(new_rate - request) &&
	    peri->in_mask1 & ((1 << I_CLOCK_NUM) - 1)) {
		clk2 = 1;
		mask = peri->in_mask1;
		step = 1;
		goto next;
	}

	peri->div_src_0 = s1, peri->div_val_0 = d1;
	peri->div_src_1 = s2, peri->div_val_1 = d2;

	pr_debug("clk: %s, step[%d] src[%d,%d] %ld /(div0: %d * div1: %d) ",
		 peri->name, peri->clk_step, peri->div_src_0, peri->div_src_1,
		 freq_hz, peri->div_val_0, peri->div_val_1);
	pr_debug("=  %ld, %ld diff (%ld)\n", new_rate, request,
		 abs(new_rate - request));

	return new_rate;
}

static int dev_set_rate(struct clk_hw *hw, unsigned long rate)
{
	struct clk_dev_peri *peri = to_clk_dev(hw)->peri;
	int i;

	rate = dev_round_rate(hw, rate);

	for (i = 0; peri->clk_step > i; i++) {
		int s = (0 == i ? peri->div_src_0 : peri->div_src_1);
		int d = (0 == i ? peri->div_val_0 : peri->div_val_1);

		if (-1 == s)
			continue;

		/* change rate */
#ifdef CONFIG_EARLY_PRINTK
		if (!strcmp(peri->name, "uart0"))
			break;
#endif
		clk_dev_rate((void *)peri->base, i, s, d);

		pr_debug("clk: %s (%p) set_rate [%d] src[%d] div[%d]\n",
			 peri->name, peri->base, i, s, d);
	}
	peri->rate = rate;
	return rate;
}

/*
 *	clock devices interface
 */
static int clk_dev_enable(struct clk_hw *hw)
{
	struct clk_dev_peri *peri = to_clk_dev(hw)->peri;
	int i = 0, inv = 0;

	pr_debug("clk: %s enable (BCLK=%s, PCLK=%s)\n", peri->name,
		 I_GATE_BCLK & peri->in_mask ? "ON" : "PASS",
		 I_GATE_PCLK & peri->in_mask ? "ON" : "PASS");

	if (peri->in_mask & I_GATE_BCLK)
		clk_dev_bclk((void *)peri->base, 1);

	if (peri->in_mask & I_GATE_PCLK)
		clk_dev_pclk((void *)peri->base, 1);

	if (!(peri->in_mask & I_CLOCK_MASK))
		return 0;

	for (i = 0, inv = peri->invert_0; peri->clk_step > i;
		i++, inv = peri->invert_1)
		clk_dev_inv((void *)peri->base, i, inv);

	/* restore clock rate */
	for (i = 0; peri->clk_step > i; i++) {
		if (peri->fix_src < 0) {
			int s = (0 == i ? peri->div_src_0 : peri->div_src_1);
			int d = (0 == i ? peri->div_val_0 : peri->div_val_1);

			if (s == -1)
				continue;

			clk_dev_rate((void *)peri->base, i, s, d);
		} else {
			int s = peri->fix_src;
			int d = (0 == i ? peri->div_val_0 : peri->div_val_1);

			clk_dev_rate((void *)peri->base, i, s, d);
		}
	}

	clk_dev_enb((void *)peri->base, 1);
	peri->enable = true;

	return 0;
}

static void clk_dev_disable(struct clk_hw *hw)
{
	struct clk_dev_peri *peri = to_clk_dev(hw)->peri;

	pr_debug("clk: %s disable\n", peri->name);

	if (peri->in_mask & I_GATE_BCLK)
		clk_dev_bclk((void *)peri->base, 0);

	if (peri->in_mask & I_GATE_PCLK)
		clk_dev_pclk((void *)peri->base, 0);

	if (!(peri->in_mask & I_CLOCK_MASK))
		return;

	clk_dev_rate((void *)peri->base, 0, 7, 256); /* for power save */
	clk_dev_enb((void *)peri->base, 0);

	peri->enable = false;

}

static unsigned long clk_dev_recalc_rate(struct clk_hw *hw, unsigned long rate)
{
	struct clk_dev_peri *peri = to_clk_dev(hw)->peri;

	pr_debug("%s: name %s, (%lu)\n", __func__, peri->name, peri->rate);
	return peri->rate;
}

static long clk_dev_round_rate(struct clk_hw *hw, unsigned long drate,
			       unsigned long *prate)
{
	struct clk_dev_peri *peri = to_clk_dev(hw)->peri;
	long rate = dev_round_rate(hw, drate);

	pr_debug("%s: name %s, (%lu, %lu)\n", __func__, peri->name, drate,
		 rate);
	return rate;
}

static int clk_dev_set_rate(struct clk_hw *hw, unsigned long drate,
			    unsigned long prate)
{
	struct clk_dev_peri *peri = to_clk_dev(hw)->peri;
	int rate = dev_set_rate(hw, drate);

	pr_debug("%s: name %s, rate %lu:%d\n", __func__, peri->name, drate,
		 rate);
	return rate;
}

static const struct clk_ops clk_dev_ops = {
	.recalc_rate = clk_dev_recalc_rate,
	.round_rate = clk_dev_round_rate,
	.set_rate = clk_dev_set_rate,
	.enable = clk_dev_enable,
	.disable = clk_dev_disable,
};

static const struct clk_ops clk_empty_ops = {};

struct clk *clk_dev_get_provider(struct of_phandle_args *clkspec, void *data)
{
	struct clk_dev *clk_data = data;

	pr_debug("%s: name %s\n", __func__, clk_data->peri->name);
	return clk_data->clk;
}

static void __init clk_dev_parse_device_data(struct device_node *np,
					     struct clk_dev *clk_data,
					     struct device *dev)
{
	struct clk_dev_peri *peri = clk_data->peri;
	unsigned int frequency = 0;
	u32 value;

	if (of_property_read_string(np, "clock-output-names", &peri->name)) {
		pr_err("clock node is missing 'clock-output-names'\n");
		return;
	}

	if (!of_property_read_string(np, "clock-names", &peri->parent_name))
		return;

	if (of_property_read_u32(np, "cell-id", &peri->id)) {
		pr_err("clock node is missing 'cell-id'\n");
		return;
	}

	if (of_property_read_u32(np, "clk-step", &peri->clk_step)) {
		pr_err("clock node is missing 'clk-step'\n");
		return;
	}

	if (of_property_read_u32(np, "clk-input", &peri->in_mask)) {
		pr_err("clock node is missing 'clk-input'\n");
		return;
	}

	if (2 == peri->clk_step &&
	    of_property_read_u32(np, "clk-input1", &peri->in_mask1)) {
		pr_err("clock node is missing 'clk-input1'\n");
		return;
	}

	if (!of_property_read_u32(np, "src-force", &value))
		peri->fix_src = value;
	else
		peri->fix_src = -1;

	if (!of_property_read_u32(np, "clk-input-ext1", &value))
		peri->in_extclk_1 = value;

	if (!of_property_read_u32(np, "clk-input-ext2", &value))
		peri->in_extclk_2 = value;

	if (!of_property_read_u32(np, "clock-frequency", &frequency))
		clk_data->rate = frequency;

	peri->base = of_iomap(np, 0);
	if (!peri->base) {
		pr_err("Can't map registers for clock %s!\n", peri->name);
		return;
	}

	pr_debug("%8s: id=%2d, base=%p, step=%d, m=0x%04x, m1=0x%04x\n",
		 peri->name, peri->id, peri->base, peri->clk_step,
		 peri->in_mask, peri->in_mask1);
}

struct clk *clk_dev_clock_register(const char *name, const char *parent_name,
				   struct clk_hw *hw, const struct clk_ops *ops,
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
	pr_debug("Register clk %8s: parent %s\n", name, parent_name);

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

#ifdef CONFIG_PM_SLEEP
static int clk_syscore_suspend(void) { return 0; }

static void clk_syscore_resume(void) {}

static struct syscore_ops clk_syscore_ops = {
	.suspend = clk_syscore_suspend, .resume = clk_syscore_resume,
};
#endif /* CONFIG_PM_SLEEP */

static void __init clk_dev_of_setup(struct device_node *node)
{
	struct device_node *np;
	struct clk_dev *clk_data = NULL;
	struct clk_dev_peri *peri = NULL;
	struct clk *clk;
	int i = 0, size = (sizeof(*clk_data) + sizeof(*peri));
	int num_clks;

#ifdef CONFIG_ARM_NXP_CPUFREQ
	char pll[16];

	sprintf(pll, "sys-pll%d", CONFIG_NXP_CPUFREQ_PLLDEV);
#endif

	num_clks = of_get_child_count(node);
	if (!num_clks) {
		pr_err("Failed to clocks count for clock generator!\n");
		return;
	}

	clk_data = kzalloc(size * num_clks, GFP_KERNEL);
	if (!clk_data) {
		WARN_ON(1);
		return;
	}
	peri = (struct clk_dev_peri *)(clk_data + num_clks);

	for_each_child_of_node(node, np) {
		clk_data[i].peri = &peri[i];
		clk_data[i].node = np;
		clk_dev_parse_device_data(np, &clk_data[i], NULL);
		of_clk_add_provider(np, clk_dev_get_provider, &clk_data[i++]);
	}

	for (i = 0; num_clks > i; i++, clk_data++) {
		unsigned long flags = CLK_IS_ROOT;
		const struct clk_ops *ops = &clk_dev_ops;

		if (peri[i].parent_name) {
			ops = &clk_empty_ops;
			flags = CLK_IS_BASIC;
#ifdef CONFIG_ARM_NXP_CPUFREQ
			if (!strcmp(pll, peri[i].parent_name))
				flags |= CLK_SET_RATE_PARENT;
#endif
		}

		clk = clk_dev_clock_register(peri[i].name, peri[i].parent_name,
					     &clk_data->hw, ops, flags);
		if (NULL == clk)
			continue;

		clk_data->clk = clk;
		if (clk_data->rate) {
			pr_debug("[%s set boot rate %u]\n", node->name,
				 clk_data->rate);
			clk_set_rate(clk, clk_data->rate);
		}
	}

#ifdef CONFIG_PM_SLEEP
	register_syscore_ops(&clk_syscore_ops);
#endif

	pr_debug("[%s:%d] %s (%d)\n", __func__, __LINE__, node->name, num_clks);
}
CLK_OF_DECLARE(s5pxx18, "nexell,s5pxx18,clocks", clk_dev_of_setup);
