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
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/syscore_ops.h>

#include <dt-bindings/clock/nxp5540-clk.h>
#define to_clk_dev(_hw) container_of(_hw, struct clk_dev, hw)

#define AXI_ROOT	0
#define AHB_ROOT	1
#define APB_ROOT	2
#define AXI_CHILD	3
#define AHB_CHILD	4
#define APB_CHILD	5

#define CORE_ROOT	0
#define CORE_CHILD	1

#define MAX_DIV_256	((1 << 8) - 1) /* 256, align 2 */
#define MAX_DIV_16	((1 << 4) - 1) /* 16, align 2 */
#define PLL_OSC		0
#define OSC_PLL		1

struct clk_dev_peri {
	const char *parent_name;
	const char *name;
	void __iomem *base;
	int id;
	int clk_step; /* 1S or 2S */
	bool enable;
	long rate;
	int clk_type;
	int max_div;
	int input_type;
	u32 in_mask;
	u32 in_mask1;
	/* clock register */
	int div_src_0;
	int div_val_0;
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

#define DIV_OFF 0x48
#define DIV_CHILD_OFF 0x48
static inline void clk_dev_rate(struct clk_dev_peri *peri,
		int step, int src, int div)
{
	struct clk_dev_map *reg = peri->base;
	int clk_type = peri->clk_type;

	if (clk_type == CORE_ROOT)
		reg = reg + DIV_OFF;
	else
		reg = reg + DIV_CHILD_OFF;

	if (div != 0)
		div -= 1;

	writel(div, reg);
}

static inline void clk_dev_inv(void *base, int step, int inv)
{
	struct clk_dev_map *reg = base;
	unsigned int val = readl(&reg->con_gen[step << 1]) & ~(1 << 1);

	val |= (inv << 1);
	writel(val, &reg->con_gen[step << 1]);
}

#define CLK_ENB 0x1C

static inline void clk_dev_enb(struct clk_dev_peri *peri, int on)
{
	struct clk_dev_map *reg = peri->base;
	int id  = peri->id;
	unsigned int val = 0x401;

	if (on)
		val |= (1 << id);
	else
		val &= ~(1 << id);

	writel(val, reg + CLK_ENB);
}

static inline long clk_dev_divide(long rate, long request, int align,
				  int *divide, int max)
{
	int div = (rate / request);
	int adv = (div & ~(align - 1)) + align;

	max = max & ~(align - 1);

	if (!div) {
		if (divide)
			*divide = 1;
		return rate;
	}

	if (div != 1)
		div &= ~(align - 1);

	if (div != adv && abs(request - rate / div) > abs(request - rate / adv))
		div = adv;

	div = (div > max ? max : div);
	if (divide)
		*divide = div;

	return (rate / div);
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
	struct clk *clk;
	int max_div = peri->max_div;
	int step, div[2] = {
		0,
	};
	int i, n, start_src = 0, max_src = 0;
	short s1 = 0, d1 = 0;

	step =  1;
	pr_debug("clk: %s request = %ld [input=0x]\n", peri->name, rate);

	if (!peri->parent_name) {
		max_src = 8;

		for (n = start_src; n < max_src; n++) {
			rate = clk_dev_pll_rate(n);

			if (!rate)
				continue;

			clock_hz = rate;
			for (i = 0; step > i; i++)
				rate = clk_dev_divide(rate, request, 2,
						&div[i], max_div);

			if (new_rate &&
				(abs(rate - request) > abs(new_rate - request)))
				continue;

			pr_debug("clk: %s, pll.%d request[%ld] calc[%ld]\n",
					peri->name, n, request, rate);

				s1 = n, d1 = div[0];

			new_rate = rate;
			freq_hz = clock_hz;

			if (request == rate)
				break;
		}

		peri->div_src_0 = s1, peri->div_val_0 = d1;
	} else {
		clk = clk_get(NULL, peri->parent_name);
		rate = clk_get_rate(clk);
		clock_hz = rate;

		rate = clk_dev_divide(rate, request, 2, &div[i], max_div);
		new_rate = rate;
		freq_hz = clock_hz;
		peri->div_src_0 = n, peri->div_val_0 = div[0];
	}
	if (peri->input_type)
		peri->div_src_0 += 1;

	pr_debug("clk: %s,  src[%d] %ld /(div0: %d) ",
		 peri->name, peri->div_src_0,
		 freq_hz, peri->div_val_0);
	pr_debug("=  %ld, %ld diff (%ld)\n", new_rate, request,
		 (long)abs(new_rate - request));
	new_rate = rate;
	return new_rate;
}

static int dev_set_rate(struct clk_hw *hw, unsigned long rate)
{
	struct clk_dev_peri *peri = to_clk_dev(hw)->peri;

	peri->rate = dev_round_rate(hw, rate);
	return peri->rate;
}

/*
 *	clock devices interface
 */
static int nxp5540_clk_dev_enable(struct clk_hw *hw)
{
	struct clk_dev_peri *peri = to_clk_dev(hw)->peri;
	int i = 0;
	int s, d;

	pr_debug("clk: %s enable\n", peri->name);

	s = peri->div_src_0;
	d = peri->div_val_0;

	clk_dev_rate(peri, i, s, d);

	clk_dev_enb(peri, 1);
	peri->enable = true;

	return 0;
}

static void nxp5540_clk_dev_disable(struct clk_hw *hw)
{
	struct clk_dev_peri *peri = to_clk_dev(hw)->peri;

	pr_debug("clk: %s disable\n", peri->name);

	clk_dev_enb(peri, 0);

	peri->enable = false;

}

static unsigned long nxp5540_clk_dev_recalc_rate(struct clk_hw *hw,
		unsigned long rate)
{
	struct clk_dev_peri *peri = to_clk_dev(hw)->peri;
	struct clk *clk;

	pr_debug("%s: name %s, (%lu), %d\n", __func__, peri->name, peri->rate,
			peri->clk_type);
	if (peri->clk_type > APB_ROOT) {
		clk = clk_get(NULL, peri->parent_name);
		peri->rate = clk_get_rate(clk);
	}

	return peri->rate;
}

static long nxp5540_clk_dev_round_rate(struct clk_hw *hw, unsigned long drate,
			       unsigned long *prate)
{
	struct clk_dev_peri *peri = to_clk_dev(hw)->peri;
	long rate = dev_round_rate(hw, drate);

	pr_debug("%s: name %s, (%lu, %lu)\n", __func__, peri->name, drate,
		 rate);
	peri->rate = rate;
	return rate;
}

static int nxp5540_clk_dev_set_rate(struct clk_hw *hw, unsigned long drate,
			    unsigned long prate)
{
	struct clk_dev_peri *peri = to_clk_dev(hw)->peri;
	int rate = dev_set_rate(hw, drate);

	pr_debug("%s: name %s, rate %lu:%d\n", __func__, peri->name, drate,
		 rate);
	return rate;
}

static const struct clk_ops nxp5540_clk_dev_ops = {
	.recalc_rate = nxp5540_clk_dev_recalc_rate,
	.round_rate = nxp5540_clk_dev_round_rate,
	.set_rate = nxp5540_clk_dev_set_rate,
	.enable = nxp5540_clk_dev_enable,
	.disable = nxp5540_clk_dev_disable,
};

static const struct clk_ops nxp5540_clk_dev_child_ops = {
	.recalc_rate = nxp5540_clk_dev_recalc_rate,
	.round_rate = nxp5540_clk_dev_round_rate,
	.set_rate = nxp5540_clk_dev_set_rate,
	.enable = nxp5540_clk_dev_enable,
	.disable = nxp5540_clk_dev_disable,
};
static const struct clk_ops clk_empty_ops = {};

struct clk *clk_nxp5540_dev_get_core_provider(struct of_phandle_args *clkspec,
		void *data)
{
	struct clk_dev *clk_data = data;
	unsigned int idx = clkspec->args[0];

	return clk_data[idx].clk;
}

static void __init clk_dev_parse_device_data(struct device_node *np,
					     struct clk_dev *clk_data,
					     struct device *dev)
{
	struct clk_dev_peri *peri = clk_data->peri;
	unsigned int frequency = 0;

	if (of_property_read_string(np, "clock-output-names", &peri->name))
		pr_err("clock node is missing 'clock-output-names'\n");

	if (!of_property_read_u32(np, "clock-frequency", &frequency))
		clk_data->rate = frequency;

	if (of_property_read_u32(np, "cell-id", &peri->id)) {
		pr_err("clock node is missing 'cell-id'\n");
		return;
	}

	of_property_read_string(np, "clock-names", &peri->parent_name);

	peri->base = of_iomap(np, 0);
	if (!peri->base) {
		pr_err("Can't map registers for clock %s!\n", peri->name);
		return;
	}

	pr_debug("%8s: id=%2d, base=%p, m=0x%04x, m1=0x%04x\n",
		 peri->name, peri->id, peri->base,
		 peri->in_mask, peri->in_mask1);
}

struct clk *clk_nxp5540_dev_core_clock_register(const char *name,
		const char *parent_name, struct clk_hw *hw,
		const struct clk_ops *ops, unsigned long flags)
{
	struct clk *clk;
	static struct clk_init_data init;

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

#define PERI(_id, cname, pname, type, max, itype)	\
	{						\
		.id = _id,				\
		.name = cname,				\
		.parent_name = pname,			\
		.clk_type = type,			\
		.input_type = itype,			\
		.max_div = max,				\
	}						\

static struct clk_dev_peri sys_core[] __initdata = {
	PERI(PDM_APB0_CLK, PDM_APB0_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PDM_APB1_CLK, PDM_APB1_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PDM_APB2_CLK, PDM_APB2_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PDM_APB3_CLK, PDM_APB3_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(AUDIO_IO_CLK, AUDIO_IO_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PKA_CORE_CLK, PKA_CORE_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(CSSYS_SRC_CLK, CSSYS_SRC_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(MCUSTOP_CLK, MCUSTOP_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(TIMER0_TCLK, TIMER0_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(TIMER1_TCLK, TIMER1_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(TIMER2_TCLK, TIMER2_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(TIMER3_TCLK, TIMER3_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(TIMER4_TCLK, TIMER4_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(TIMER5_TCLK, TIMER5_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(TIMER6_TCLK, TIMER6_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(TIMER7_TCLK, TIMER7_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_0_TCLK, PWM_0_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_1_TCLK, PWM_1_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_2_TCLK, PWM_2_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_3_TCLK, PWM_3_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_4_TCLK, PWM_4_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_5_TCLK, PWM_5_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_6_TCLK, PWM_6_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_7_TCLK, PWM_7_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_8_TCLK, PWM_8_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_9_TCLK, PWM_9_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_10_TCLK, PWM_10_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_11_TCLK, PWM_11_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_12_TCLK, PWM_12_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_13_TCLK, PWM_13_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_14_TCLK, PWM_14_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PWM_15_TCLK, PWM_15_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PO_0_OUT_CLK, PO_0_OUT_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PO_1_OUT_CLK, PO_1_OUT_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PO_2_OUT_CLK, PO_2_OUT_NAME, NULL
			, CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PO_3_OUT_CLK, PO_3_OUT_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(PO_4_OUT_CLK, PO_4_OUT_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(DMA_BUS_AXI_CLK, DMA_BUS_AXI_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(GIC400_AXI_CLK, GIC400_AXI_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
};

static struct clk_dev_peri tbus_core[] __initdata = {
	PERI(I2S0_CLK, I2S0_NAME, NULL, CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(I2S1_CLK, I2S1_NAME, NULL, CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(I2S2_CLK, I2S2_NAME, NULL, CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(I2S3_CLK, I2S3_NAME, NULL, CORE_ROOT, MAX_DIV_256, PLL_OSC),
};

static struct clk_dev_peri lbus_core[] __initdata = {
	PERI(SDMMC0_CORE_CLK, SDMMC0_CORE_NAME,	NULL,
			CORE_ROOT, MAX_DIV_16, PLL_OSC),
	PERI(SDMMC0_AXI_CLK, SDMMC0_AXI_NAME, NULL,
			CORE_ROOT, MAX_DIV_16, PLL_OSC),
	PERI(SDMMC1_CORE_CLK, SDMMC1_CORE_NAME,	NULL,
			CORE_ROOT, MAX_DIV_16, PLL_OSC),
	PERI(SDMMC1_AXI_CLK, SDMMC1_AXI_NAME, NULL,
			CORE_ROOT, MAX_DIV_16, PLL_OSC),
	PERI(SDMMC2_CORE_CLK, SDMMC2_CORE_NAME,	NULL,
			CORE_ROOT, MAX_DIV_16, PLL_OSC),
	PERI(SDMMC2_AXI_CLK, SDMMC2_AXI_NAME, NULL,
			CORE_ROOT, MAX_DIV_16, PLL_OSC),
	PERI(GMAC_MII_CLK, GMAC_MII_NAME, NULL,
			CORE_ROOT, MAX_DIV_16, PLL_OSC),
	PERI(GMAC_TX_CLK, GMAC_TX_NAME, GMAC_MII_NAME,
			CORE_CHILD, MAX_DIV_256, PLL_OSC),
};

static struct clk_dev_peri bbus_core[] __initdata = {
	PERI(UART0_CORE_CLK, UART0_CORE_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(UART1_CORE_CLK, UART1_CORE_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(UART2_CORE_CLK, UART2_CORE_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(UART3_CORE_CLK, UART3_CORE_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(UART4_CORE_CLK, UART4_CORE_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(UART5_CORE_CLK, UART5_CORE_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(UART6_CORE_CLK, UART6_CORE_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(UART7_CORE_CLK, UART7_CORE_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(UART8_CORE_CLK, UART8_CORE_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(SPI0_APB_CLK, SPI0_APB_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(SPI0_CORE_CLK, SPI1_APB_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(SPI1_APB_CLK, SPI2_APB_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(SPI1_CORE_CLK, SPI0_CORE_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(SPI2_APB_CLK, SPI1_CORE_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(SPI2_CORE_CLK, SPI2_CORE_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(SPDIFTX0_CORE_CLK, SPDIFTX0_CORE_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
};

static struct clk_dev_peri hdmi_core[] __initdata = {
	PERI(HDMIV2_0_TMDS_10B_CLK, HDMIV2_0_TMDS_10B_NAME,
			NULL, CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(HDMIV2_0_TMDS_20B_CLK, HDMIV2_0_TMDS_20B_NAME,
			HDMIV2_0_TMDS_10B_NAME, CORE_CHILD,
			MAX_DIV_256, OSC_PLL),
	PERI(HDMIV2_0_PIXELX2_CLK, HDMIV2_0_PIXELX2_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, OSC_PLL),
	PERI(HDMIV2_0_PIXELX_CLK, HDMIV2_0_PIXELX_NAME,
			HDMIV2_0_PIXELX2_NAME, CORE_CHILD,
			MAX_DIV_256, OSC_PLL),
	PERI(HDMIV2_0_AUDIO_CLK, HDMIV2_0_AUDIO_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, OSC_PLL),
};

static struct clk_dev_peri wave_core[] __initdata = {
	PERI(WAVE_V_0_CLK, WAVE_V_0_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(WAVE_M_0_CLK, WAVE_M_0_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(WAVE_C_0_CLK, WAVE_C_0_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
	PERI(WAVE_B_0_CLK, WAVE_B_0_NAME, NULL,
			CORE_ROOT, MAX_DIV_256, PLL_OSC),
};

static void clk_dev_of_setup(struct device_node *node,
		struct clk_dev_peri *data, int num)
{
	struct device_node *np;
	struct clk_dev *clk_data = NULL;
	struct clk_dev_peri *peri = NULL;
	struct clk *clk;
	int i = 0, size = (sizeof(*clk_data) + sizeof(*peri));
	int num_clks;
	int idx = 0;
	static void __iomem *base;
#ifdef CONFIG_ARM_NEXELL_CPUFREQ
	char pll[16];

	sprintf(pll, "sys-pll%d", CONFIG_NEXELL_CPUFREQ_PLLDEV);
#endif
	base = of_iomap(node, 0);
	num_clks = num;

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

	for (i = 0; i < num_clks; i++) {
		peri[i].id = data[i].id;
		peri[i].name = data[i].name;
		peri[i].parent_name = data[i].parent_name;
		peri[i].clk_type = data[i].clk_type;
		peri[i].base = data[i].base;
		peri[i].max_div = data[i].max_div;
		clk_data[i].peri = &peri[i];
		clk_data[i].node = NULL;
	}

	for_each_child_of_node(node, np) {
		of_property_read_u32(np, "cell-id", &i);
		clk_data[i].node = np;
		clk_dev_parse_device_data(np, &clk_data[i], NULL);
	}

	of_clk_add_provider(node, clk_nxp5540_dev_get_core_provider, clk_data);

	for (i = 0; num_clks > i; i++) {
		unsigned long flags = CLK_IS_ROOT;
		const struct clk_ops *ops = &nxp5540_clk_dev_ops;

		idx  = peri[i].id;

		if (peri[idx].parent_name) {
			flags = CLK_IS_BASIC;
#ifdef CONFIG_ARM_NEXELL_CPUFREQ
			if (!strcmp(pll, peri[i].parent_name))
				flags |= CLK_SET_RATE_PARENT;
#endif
		}

		clk = clk_nxp5540_dev_core_clock_register(peri[i].name,
				peri[i].parent_name, &clk_data[i].hw,
				ops, flags);
		if (clk == NULL)
			continue;

		clk_data[i].clk = clk;
		if (clk_data[i].rate)
			pr_debug("[%s : set boot rate %u], %d\n", node->name,
				 clk_data[i].rate,
				 clk_set_rate(clk, clk_data[i].rate));

	}

#ifdef CONFIG_PM_SLEEP
	register_syscore_ops(&clk_syscore_ops);
#endif

}

static void __init hdmi_clk_init(struct device_node *node)
{
	int num = ARRAY_SIZE(hdmi_core);

	clk_dev_of_setup(node, hdmi_core, num);
}
CLK_OF_DECLARE(nxp5540_hdmi_core, "nexell,nxp5540-cmu-hdmi-core", hdmi_clk_init);

static void __init wave_clk_init(struct device_node *node)
{
	int num = ARRAY_SIZE(wave_core);

	clk_dev_of_setup(node, wave_core, num);
}
CLK_OF_DECLARE(nxp5540_wave_core, "nexell,nxp5540-cmu-wave-core", wave_clk_init);

static void __init bbus_clk_init(struct device_node *node)
{
	int num = ARRAY_SIZE(bbus_core);

	clk_dev_of_setup(node, bbus_core, num);
}
CLK_OF_DECLARE(nxp5540_bbus_core, "nexell,nxp5540-cmu-bbus-core", bbus_clk_init);

static void __init lbus_clk_init(struct device_node *node)
{
	int num = ARRAY_SIZE(lbus_core);

	clk_dev_of_setup(node, lbus_core, num);
}
CLK_OF_DECLARE(nxp5540_lbus_core, "nexell,nxp5540-cmu-lbus-core", lbus_clk_init);

static void __init tbus_clk_init(struct device_node *node)
{
	int num = ARRAY_SIZE(tbus_core);

	clk_dev_of_setup(node, tbus_core, num);
}
CLK_OF_DECLARE(nxp5540_tbus_core, "nexell,nxp5540-cmu-tbus-core", tbus_clk_init);

static void __init sys_clk_init(struct device_node *node)
{
	int num = ARRAY_SIZE(sys_core);

	clk_dev_of_setup(node, sys_core, num);
}
CLK_OF_DECLARE(nxp5440_sys_core, "nexell,nxp5540-cmu-sys-core", sys_clk_init);
