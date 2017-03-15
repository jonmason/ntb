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
static inline void clk_dev_rate(struct clk_dev_peri *peri,
		int step, int src, int div)
{
	struct clk_dev_map *reg = peri->base;
	int clk_type = peri->clk_type;

	reg = reg + DIV_OFF + clk_type * 0x40;
	if (div)
		div = div - 1;
	writel(div, reg);
}

static inline void clk_dev_inv(void *base, int step, int inv)
{
	struct clk_dev_map *reg = base;
	unsigned int val = readl(&reg->con_gen[step << 1]) & ~(1 << 1);

	val |= (inv << 1);
	writel(val, &reg->con_gen[step << 1]);
}

#define CLK_AXI_ENB 0x1C

static inline void clk_dev_enb(struct clk_dev_peri *peri, int on)
{
	struct clk_dev_map *reg = peri->base;
	int id  = peri->id;
	unsigned int val = 0;

	if (id >= 32) {
		id = id % 32;
		if (id >= 64)
			reg = reg + CLK_AXI_ENB + 0x08;
		else
			reg = reg + CLK_AXI_ENB + 0x04;

	} else {
		reg = reg + CLK_AXI_ENB;
	}

	val = readl(reg);
	if (on)
		val |= (1 << id);
	else
		val &= ~(1 << id);

	writel(val, reg);
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
				rate = clk_dev_divide(rate, request,
						2, &div[i]);

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

		rate = clk_dev_divide(rate, request, 2, &div[i]);
		new_rate = rate;
		freq_hz = clock_hz;
		peri->div_src_0 = n, peri->div_val_0 = div[0];
	}
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

	if (peri->clk_type <= APB_ROOT) {
		rate = dev_round_rate(hw, rate);
	} else {
		struct clk *clk;

		clk = clk_get(NULL, peri->parent_name);
		rate = clk_get_rate(clk);
	}
	peri->rate = rate;
	return rate;
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

	pr_debug("\e32m %s: name %s, rate %lu:%d \e[0m\n", __func__,
			peri->name, drate, rate);
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

struct clk *clk_nxp5540_dev_get_provider(struct of_phandle_args *clkspec,
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

	pr_debug("%8s: id=%2d, base=%p, m=0x%04x, m1=0x%04x\n",
		 peri->name, peri->id, peri->base,
		 peri->in_mask, peri->in_mask1);
}

struct clk *clk_nxp5540_dev_clock_register(const char *name,
		const char *parent_name, struct clk_hw *hw,
		const struct clk_ops *ops, unsigned long flags)
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

#define BUS(_id, cname, pname, type)		\
	{					\
		.id = _id,			\
		.name = cname,			\
		.parent_name = pname,		\
		.clk_type = type,		\
	}					\

static struct clk_dev_peri sys_pclk[] __initdata = {
	BUS(SYS_AXI_CLK, SYS_AXI_NAME, NULL, AXI_ROOT),
	BUS(CCI400_AXI_CLK, CCI400_AXI_NAME, SYS_AXI_NAME, AXI_CHILD),
	BUS(TIEOFF_CCI_AXI_CLK, TIEOFF_CCI_AXI_NAME, SYS_AXI_NAME, AXI_CHILD),
	BUS(CRYPTO_AXI_CLK, CRYPTO_AXI_NAME, SYS_AXI_NAME, AXI_CHILD),
	BUS(SYS_AHB_CLK, SYS_AHB_NAME, SYS_AXI_NAME, AHB_ROOT),
	BUS(SYS_APB_CLK, SYS_APB_NAME, SYS_AHB_NAME, APB_ROOT),
	BUS(TIMER_0_3_APBCLK, TIMER_0_3_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(TIMER_4_7_APBCLK, TIMER_4_7_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(PWM_0_3_APB_CLK, PWM_0_3_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(PWM_4_7_APB_CLK, PWM_4_7_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(PWM_8_11_APB_CLK, PWM_8_11_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(PWM_12_APB_CLK, PWM_12_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(SYSCTRL_APB_CLK, SYSCTRL_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(HPM_APB_CLK, HPM_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(Q_ENHANCER_APB_CLK, Q_ENHANCER_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(CYPTO_APB_CLK, CYPTO_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(WDT_APB0_CLK, WDT_APB0_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(WDT_POR0_CLK, WDT_POR0_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(WDT_APB1_CLK, WDT_APB1_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(WDT_POR1_CLK, WDT_POR1_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(TZPC_APB_CLK, TZPC_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(ECID_APB_CLK, ECID_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(DMAC_0_APB_CLK, DMAC_0_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(DMAC_1_APB_CLK, DMAC_1_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(SDMAC_0_APB_CLK, SDMAC_0_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(SDMAC_1_APB_CLK, SDMAC_1_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(MDMAC_0_APB_CLK, MDMAC_0_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(SYSTIEOFF_APB_CLK, SYSTIEOFF_APB_NAME, SYS_APB_NAME, APB_CHILD),
	BUS(MCUS_APB_CLK, MCUS_APB_NAME, SYS_APB_NAME, APB_CHILD),
};

static struct clk_dev_peri tbus_pclk[] __initdata = {
	BUS(TBUS_AXI_CLK, TBUS_AXI_NAME, NULL, AXI_ROOT),
	BUS(TBUS_AHB_CLK, TBUS_AHB_NAME, TBUS_AXI_NAME, AHB_ROOT),
	BUS(TBUS_APB_CLK, TBUS_APB_NAME, TBUS_AHB_NAME, APB_ROOT),
	BUS(TBUS_I2S0_APB_CLK, TBUS_I2S0_APB_NAME, TBUS_APB_NAME, APB_CHILD),
	BUS(TBUS_I2S1_APB_CLK, TBUS_I2S1_APB_NAME, TBUS_APB_NAME, APB_CHILD),
	BUS(TBUS_I2S2_APB_CLK, TBUS_I2S2_APB_NAME, TBUS_APB_NAME, APB_CHILD),
	BUS(TBUS_AC97_APB_CLK, TBUS_AC97_APB_NAME, TBUS_APB_NAME, APB_CHILD),
	BUS(TBUS_I2C0_APB_CLK, TBUS_I2C0_APB_NAME, TBUS_APB_NAME, APB_CHILD),
	BUS(TBUS_I2C1_APB_CLK, TBUS_I2C1_APB_NAME, TBUS_APB_NAME, APB_CHILD),
};

static struct clk_dev_peri lbus_pclk[] __initdata = {
	BUS(LBUS_AXI_CLK, LBUS_AXI_NAME, NULL, AXI_ROOT),
	BUS(LBUS_AHB_CLK, LBUS_AHB_NAME, LBUS_AXI_NAME, AHB_ROOT),
	BUS(LBUS_GMAC_AHB_CLK, LBUS_GMAC_AHB_NAME, LBUS_AHB_NAME, AHB_CHILD),
	BUS(LBUS_MP2TS_AHB_CLK, LBUS_MP2TS_AHB_NAME, LBUS_AHB_NAME, APB_CHILD),
	BUS(LBUS_APB_CLK, LBUS_APB_NAME, LBUS_AHB_NAME, APB_ROOT),
	BUS(LBUS_GMAC_CSR_CLK, LBUS_GMAC_CSR_NAME, LBUS_APB_NAME, APB_CHILD),
	BUS(LBUS_TIEOFF_APB_CLK, LBUS_TIEOFF_APB_NAME,
			LBUS_APB_NAME, APB_CHILD),
	BUS(LBUS_GPIO0_APB_CLK, LBUS_GPIO0_APB_NAME, LBUS_APB_NAME, APB_CHILD),
	BUS(LBUS_GPIO1_APB_CLK, LBUS_GPIO1_APB_NAME, LBUS_APB_NAME, APB_CHILD),
	BUS(LBUS_I2C2_APB_CLK, LBUS_I2C2_APB_NAME, LBUS_APB_NAME, APB_CHILD),
	BUS(LBUS_I2C3_APB_CLK, LBUS_I2C3_APB_NAME, LBUS_APB_NAME, APB_CHILD),
	BUS(LBUS_I2C4_APB_CLK, LBUS_I2C4_APB_NAME, LBUS_APB_NAME, APB_CHILD),
};

static struct clk_dev_peri bbus_pclk[] __initdata = {
	BUS(BBUS_AXI_CLK, BBUS_AXI_NAME, NULL, AXI_ROOT),
	BUS(BBUS_AHB_CLK, BBUS_AHB_NAME, BBUS_AXI_NAME, AHB_ROOT),
	BUS(BBUS_APB_CLK, BBUS_APB_NAME, BBUS_AHB_NAME, APB_ROOT),
	BUS(UART0_APB_CLK, UART0_APB_NAME, BBUS_APB_NAME, APB_CHILD),
	BUS(UART1_APB_CLK, UART1_APB_NAME, BBUS_APB_NAME, APB_CHILD),
	BUS(UART2_APB_CLK, UART2_APB_NAME, BBUS_APB_NAME, APB_CHILD),
	BUS(UART3_APB_CLK, UART3_APB_NAME, BBUS_APB_NAME, APB_CHILD),
	BUS(UART4_APB_CLK, UART4_APB_NAME, BBUS_APB_NAME, APB_CHILD),
	BUS(UART5_APB_CLK, UART5_APB_NAME, BBUS_APB_NAME, APB_CHILD),
	BUS(UART6_APB_CLK, UART6_APB_NAME, BBUS_APB_NAME, APB_CHILD),
	BUS(UART7_APB_CLK, UART7_APB_NAME, BBUS_APB_NAME, APB_CHILD),
	BUS(UART8_APB_CLK, UART8_APB_NAME, BBUS_APB_NAME, APB_CHILD),
	BUS(SPDIFTX0_APB_CLK, SPDIFTX0_APB_NAME, BBUS_APB_NAME, APB_CHILD),
	BUS(PDIFRTX0_APB_CLK, PDIFRTX0_APB_NAME, BBUS_APB_NAME, APB_CHILD),
	BUS(TMU_APB_CLK, TMU_APB_NAME,	BBUS_APB_NAME, APB_CHILD),
	BUS(BBUS_TIEOFF_APB_CLK, BBUS_TIEOFF_APB_NAME,
			BBUS_APB_NAME, APB_CHILD),
	BUS(ADC_0_APB_CLK, ADC_0_APB_NAME, BBUS_APB_NAME, APB_CHILD),
	BUS(GPIO_2_APB_CLK, GPIO_2_APB_NAME, BBUS_APB_NAME, APB_CHILD),
	BUS(GPIO_3_APB_CLK, GPIO_3_APB_NAME, BBUS_APB_NAME, APB_CHILD),
	BUS(GPIO_4_APB_CLK, GPIO_4_APB_NAME, BBUS_APB_NAME, APB_CHILD),
};

static struct clk_dev_peri isp_pclk[] __initdata = {
	BUS(ISP_0_AXI_CLK, ISP_0_AXI_NAME, NULL, AXI_ROOT),
	BUS(ISP_0_DIV4_CLK, ISP_0_DIV4_NAME, ISP_0_AXI_NAME, AHB_ROOT),
};

static struct clk_dev_peri hdmi_pclk[] __initdata = {
	BUS(HDMI_0_AXI_CLK, HDMI_0_AXI_NAME, NULL, AXI_ROOT),
	BUS(HDMIV2_0_AXI_CLK, HDMIV2_0_AXI_NAME, HDMI_0_AXI_NAME, AXI_CHILD),
	BUS(HDMI_0_APB_CLK, HDMI_0_APB_NAME, HDMI_0_AXI_NAME, APB_ROOT),
	BUS(HDMIV2_0_APB_CLK, HDMIV2_0_APB_NAME, HDMI_0_APB_NAME, APB_CHILD),
	BUS(HDMIV2_0_APBPHY_CLK, HDMIV2_0_APBPHY_NAME,
			HDMI_0_APB_NAME, APB_CHILD),
	BUS(HDMIV2_0_PHY_CLK, HDMIV2_0_PHY_NAME, HDMI_0_APB_NAME, APB_CHILD),
};

static struct clk_dev_peri wave_pclk[] __initdata = {
	BUS(WAVE_AXI_CLK, WAVE_AXI_NAME, NULL, AXI_ROOT),
	BUS(WAVE_APB_CLK, WAVE_APB_NAME, WAVE_AXI_NAME, APB_ROOT),
};

static struct clk_dev_peri disp_pclk[] __initdata = {
	BUS(DISP_0_AXI_CLK, DISP_0_AXI_NAME, NULL, AXI_ROOT),
	BUS(MIPI_0_AXI_CLK, MIPI_0_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(CSI_TO_AXI_0_AXI_CLK, CSI_TO_AXI_0_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(CSI_TO_NXS_0_AXI_CLK, CSI_TO_NXS_0_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(CSI_TO_NXS_1_AXI_CLK, CSI_TO_NXS_1_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(CSI_TO_NXS_2_AXI_CLK, CSI_TO_NXS_2_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(CSI_TO_NXS_3_AXI_CLK, CSI_TO_NXS_3_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MLC_0_BOTTOM0_CLK, MLC_0_BOTTOM0_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MLC_0_BOTTOM1_CLK, MLC_0_BOTTOM1_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MLC_0_BLENDER0_CLK, MLC_0_BLENDER0_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MLC_0_BLENDER1_CLK, MLC_0_BLENDER1_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MLC_0_BLENDER2_CLK, MLC_0_BLENDER2_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MLC_0_BLENDER3_CLK, MLC_0_BLENDER3_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MLC_0_BLENDER4_CLK, MLC_0_BLENDER4_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MLC_0_BLENDER5_CLK, MLC_0_BLENDER5_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MLC_0_BLENDER6_CLK, MLC_0_BLENDER6_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MLC_0_BLENDER7_CLK, MLC_0_BLENDER7_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(VIP_0_AXI_CLK, VIP_0_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(VIP_1_AXI_CLK, VIP_1_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(VIP_2_AXI_CLK, VIP_2_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(VIP_3_AXI_CLK, VIP_3_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MCD_CPUIF_0_AXI_CLK, MCD_CPUIF_0_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(VIP_CPUIF_0_AXI_CLK, VIP_CPUIF_0_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(VIP_PRES_CPUIF_0_AXI_CLK, VIP_PRES_CPUIF_0_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(ISS_CPUIF_0_AXI_CLK, ISS_CPUIF_0_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DISP2ISP_0_AXI_CLK, DISP2ISP_0_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(ISP2DISP_0_AXI_CLK, ISP2DISP_0_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(ISP2DISP_1_AXI_CLK, ISP2DISP_1_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(CROP_0_AXI_CLK, CROP_0_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(CROP_1_AXI_CLK, CROP_1_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(CSC_0_AXI_CLK, CSC_0_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(CSC_1_AXI_CLK, CSC_1_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(CSC_2_AXI_CLK, CSC_2_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(CSC_3_AXI_CLK, CSC_3_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(SCALER_0_AXI_CLK, SCALER_0_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(SCALER_1_AXI_CLK, SCALER_1_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(SCALER_2_AXI_CLK, SCALER_2_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MULTI_TAP_0_AXI_CLK, MULTI_TAP_0_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MULTI_TAP_1_AXI_CLK, MULTI_TAP_1_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MULTI_TAP_2_AXI_CLK, MULTI_TAP_2_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(MULTI_TAP_3_AXI_CLK, MULTI_TAP_3_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAR_0_AXI_CLK, DMAR_0_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAR_1_AXI_CLK, DMAR_1_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAR_2_AXI_CLK, DMAR_2_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAR_3_AXI_CLK, DMAR_3_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAR_4_AXI_CLK, DMAR_4_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAR_5_AXI_CLK, DMAR_5_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAR_6_AXI_CLK, DMAR_6_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAR_7_AXI_CLK, DMAR_7_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAR_8_AXI_CLK, DMAR_8_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAR_9_AXI_CLK, DMAR_9_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAW_0_AXI_CLK, DMAW_0_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAW_1_AXI_CLK, DMAW_1_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAW_2_AXI_CLK, DMAW_2_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAW_3_AXI_CLK, DMAW_3_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAW_4_AXI_CLK, DMAW_4_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAW_5_AXI_CLK, DMAW_5_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAW_6_AXI_CLK, DMAW_6_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAW_7_AXI_CLK, DMAW_7_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAW_8_AXI_CLK, DMAW_8_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAW_9_AXI_CLK, DMAW_9_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAW_10_AXI_CLK, DMAW_10_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DMAW_11_AXI_CLK, DMAW_11_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(HUE_0_AXI_CLK, HUE_0_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(HUE_1_AXI_CLK, HUE_1_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(GAMMA_0_AXI_CLK, GAMMA_0_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(GAMMA_1_AXI_CLK, GAMMA_1_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DPC_0_AXI_CLK, DPC_0_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DPC_1_AXI_CLK, DPC_1_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(LVDS_0_AXI_CLK, LVDS_0_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(LVDS_0_PHY_CLK, LVDS_0_PHY_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(LVDS_1_AXI_CLK, LVDS_1_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(LVDS_1_PHY_CLK, LVDS_1_PHY_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(NXS_FIFO_0_AXI_CLK, NXS_FIFO_0_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(NXS_FIFO_1_AXI_CLK, NXS_FIFO_1_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(NXS_FIFO_2_AXI_CLK, NXS_FIFO_2_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(NXS_FIFO_3_AXI_CLK, NXS_FIFO_3_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(NXS_FIFO_4_AXI_CLK, NXS_FIFO_4_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(NXS_FIFO_5_AXI_CLK, NXS_FIFO_5_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(NXS_FIFO_6_AXI_CLK, NXS_FIFO_6_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(NXS_FIFO_7_AXI_CLK, NXS_FIFO_7_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(NXS_FIFO_8_AXI_CLK, NXS_FIFO_8_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(NXS_FIFO_9_AXI_CLK, NXS_FIFO_9_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(NXS_FIFO_10_AXI_CLK, NXS_FIFO_10_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(NXS_FIFO_11_AXI_CLK, NXS_FIFO_11_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(NXS2HDMI_0_AXI_CLK, NXS2HDMI_0_AXI_NAME,
			DISP_0_AXI_NAME, AXI_CHILD),
	BUS(TPGEN_0_AXI_CLK, TPGEN_0_AXI_NAME, DISP_0_AXI_NAME, AXI_CHILD),
	BUS(DISP_0_APB_CLK, DISP_0_APB_NAME, DISP_0_AXI_NAME, APB_ROOT),
	BUS(DEINTERLACE_0_APB_CLK, DEINTERLACE_0_APB_NAME,
			DISP_0_APB_NAME, APB_CHILD),
	BUS(DISP_TIEOFF_0_APB_CLK, DISP_TIEOFF_0_APB_NAME,
			DISP_0_APB_NAME, APB_CHILD),
	BUS(DISP_TZPC_0_APB_CLK, DISP_TZPC_0_APB_NAME,
			DISP_0_APB_NAME, APB_CHILD),
	BUS(DISP_TZPC_1_APB_CLK, DISP_TZPC_1_APB_NAME,
			DISP_0_APB_NAME, APB_CHILD),
	BUS(MIPI_0_APBCSI_CLK, MIPI_0_APBCSI_NAME,
			DISP_0_APB_NAME, APB_CHILD),
	BUS(MIPI_0_APBDSI_CLK, MIPI_0_APBDSI_NAME, DISP_0_APB_NAME, APB_CHILD),
	BUS(MIPI_0_CSIPHY_CLK, MIPI_0_CSIPHY_NAME, DISP_0_APB_NAME, APB_CHILD),
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
		peri[i].base = base;
		clk_data[i].peri = &peri[i];
		clk_data[i].node = NULL;
	}

	for_each_child_of_node(node, np) {
		of_property_read_u32(np, "cell-id", &i);
		clk_data[i].node = np;
		clk_dev_parse_device_data(np, &clk_data[i], NULL);
	}

	of_clk_add_provider(node, clk_nxp5540_dev_get_provider, clk_data);

	for (i = 0; num_clks > i; i++) {
		unsigned long flags = CLK_IS_ROOT;
		const struct clk_ops *ops = &nxp5540_clk_dev_child_ops;

		if (peri[i].clk_type <= APB_ROOT)
			ops = &nxp5540_clk_dev_ops;

		idx = peri[i].id;

		if (peri[idx].parent_name) {
			flags = CLK_IS_BASIC;
#ifdef CONFIG_ARM_NEXELL_CPUFREQ
			if (!strcmp(pll, peri[i].parent_name))
				flags |= CLK_SET_RATE_PARENT;
#endif
		}

		clk = clk_nxp5540_dev_clock_register(peri[i].name,
				peri[i].parent_name,
				&clk_data[i].hw, ops, flags);
		if (clk == NULL)
			continue;
		clk_data[i].clk = clk;
		if (clk_data[i].rate) {
			pr_info("[%s set boot rate %u], %d\n", node->name,
				 clk_data[i].rate,
				 clk_set_rate(clk, clk_data[i].rate));
		}
	}

#ifdef CONFIG_PM_SLEEP
	register_syscore_ops(&clk_syscore_ops);
#endif
}

static void __init disp_clk_init(struct device_node *node)
{
	int num = ARRAY_SIZE(disp_pclk);

	clk_dev_of_setup(node, disp_pclk, num);
}
CLK_OF_DECLARE(s5pxx18_disp_axi, "nexell,nxp5540-cmu-disp-axi", disp_clk_init);

static void __init wave_clk_init(struct device_node *node)
{
	int num = ARRAY_SIZE(wave_pclk);

	clk_dev_of_setup(node, wave_pclk, num);
}
CLK_OF_DECLARE(s5pxx18_wave_axi, "nexell,nxp5540-cmu-wave-axi", wave_clk_init);

static void __init hdmi_clk_init(struct device_node *node)
{
	int num = ARRAY_SIZE(hdmi_pclk);

	clk_dev_of_setup(node, hdmi_pclk, num);
}
CLK_OF_DECLARE(s5pxx18_hdmi_axi, "nexell,nxp5540-cmu-hdmi-axi", hdmi_clk_init);

static void __init isp_clk_init(struct device_node *node)
{
	int num = ARRAY_SIZE(isp_pclk);

	clk_dev_of_setup(node, isp_pclk, num);
}
CLK_OF_DECLARE(s5pxx18_isp_axi, "nexell,nxp5540-cmu-isp-axi", isp_clk_init);

static void __init bbus_clk_init(struct device_node *node)
{
	int num = ARRAY_SIZE(bbus_pclk);

	clk_dev_of_setup(node, bbus_pclk, num);
}
CLK_OF_DECLARE(s5pxx18_bbus_axi, "nexell,nxp5540-cmu-bbus-axi", bbus_clk_init);

static void __init lbus_clk_init(struct device_node *node)
{
	int num = ARRAY_SIZE(lbus_pclk);

	clk_dev_of_setup(node, lbus_pclk, num);
}
CLK_OF_DECLARE(s5pxx18_lbus_axi, "nexell,nxp5540-cmu-lbus-axi", lbus_clk_init);

static void __init tbus_clk_init(struct device_node *node)
{
	int num = ARRAY_SIZE(tbus_pclk);

	clk_dev_of_setup(node, tbus_pclk, num);
}
CLK_OF_DECLARE(s5pxx18_tbus_axi, "nexell,nxp5540-cmu-tbus-axi", tbus_clk_init);

static void __init sys_clk_init(struct device_node *node)
{
	int num = ARRAY_SIZE(sys_pclk);

	clk_dev_of_setup(node, sys_pclk, num);
}
CLK_OF_DECLARE(s5pxx18_sys_axi, "nexell,nxp5540-cmu-sys-axi", sys_clk_init);

