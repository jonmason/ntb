/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
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

#include <linux/of.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_res_manager.h>

/* NXS2LVDS */
#define LVDS_DPC_CTRL0		0x0020

/* LVDS DPC CTRL0 */
#define DIRTYFLAG_TOP_SHIFT	12
#define DIRTYFLAG_TOP_MASK	BIT(12)

#define SIMULATE_INTERRUPT

struct nxs_lvds {
#ifdef SIMULATE_INTERRUPT
	struct timer_list timer;
#endif
	struct nxs_dev nxs_dev;
	u8 *base;
};

#define nxs_to_lvds(dev)	container_of(dev, struct nxs_lvds, nxs_dev)

#ifdef SIMULATE_INTERRUPT
#include <linux/timer.h>
#define INT_TIMEOUT_MS		30

static void int_timer_func(unsigned long priv)
{
	struct nxs_lvds *lvds = (struct nxs_lvds *)priv;
	struct nxs_dev *nxs_dev = &lvds->nxs_dev;

	if (nxs_dev->irq_callback)
		nxs_dev->irq_callback->handler(nxs_dev,
					       nxs_dev->irq_callback->data);

	mod_timer(&lvds->timer, jiffies + msecs_to_jiffies(INT_TIMEOUT_MS));
}
#endif

static void lvds_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 lvds_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 lvds_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void lvds_clear_interrupt_pending(const struct nxs_dev *pthis, int type)
{
}

static int lvds_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int lvds_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int lvds_start(const struct nxs_dev *pthis)
{
#ifdef SIMULATE_INTERRUPT
	struct nxs_lvds *lvds = nxs_to_lvds(pthis);

	mod_timer(&lvds->timer, jiffies + msecs_to_jiffies(INT_TIMEOUT_MS));
#endif

	return 0;
}

static int lvds_stop(const struct nxs_dev *pthis)
{
#ifdef SIMULATE_INTERRUPT
	struct nxs_lvds *lvds = nxs_to_lvds(pthis);

	del_timer(&lvds->timer);
#endif

	return 0;
}

static int lvds_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_lvds *lvds = nxs_to_lvds(pthis);
	u32 val;
	u8 *reg;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	reg = lvds->base + LVDS_DPC_CTRL0;
	val = readl(reg);
	val &= ~DIRTYFLAG_TOP_MASK;
	val |= 1 << DIRTYFLAG_TOP_SHIFT;
	writel(val, reg);

	return 0;
}

static int lvds_set_syncinfo(const struct nxs_dev *pthis,
			     const struct nxs_control *pparam)
{
	return 0;
}

static int lvds_get_syncinfo(const struct nxs_dev *pthis,
			     struct nxs_control *pparam)
{
	return 0;
}

static int nxs_lvds_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_lvds *lvds;
	struct nxs_dev *nxs_dev;
	struct resource res;

	lvds = devm_kzalloc(&pdev->dev, sizeof(*lvds), GFP_KERNEL);
	if (!lvds)
		return -ENOMEM;

	nxs_dev = &lvds->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	ret = of_address_to_resource(pdev->dev.of_node, 0, &res);
	if (ret) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to get lvds base address\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -ENXIO;
	}

	lvds->base = devm_ioremap_nocache(nxs_dev->dev, res.start,
					  resource_size(&res));
	if (!lvds->base) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to ioremap for lvds\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
			return -EBUSY;
	}

	nxs_dev->set_interrupt_enable = lvds_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = lvds_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = lvds_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = lvds_clear_interrupt_pending;
	nxs_dev->open = lvds_open;
	nxs_dev->close = lvds_close;
	nxs_dev->start = lvds_start;
	nxs_dev->stop = lvds_stop;
	nxs_dev->set_dirty = lvds_set_dirty;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_SYNCINFO;
	nxs_dev->dev_services[0].set_control = lvds_set_syncinfo;
	nxs_dev->dev_services[0].get_control = lvds_get_syncinfo;

	nxs_dev->dev = &pdev->dev;

#ifdef SIMULATE_INTERRUPT
	setup_timer(&lvds->timer, int_timer_func, (long)lvds);
#endif

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, lvds);

	return 0;
}

static int nxs_lvds_remove(struct platform_device *pdev)
{
	struct nxs_lvds *lvds = platform_get_drvdata(pdev);

	if (lvds)
		unregister_nxs_dev(&lvds->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_lvds_match[] = {
	{ .compatible = "nexell,lvds-nxs-1.0", },
	{},
};

static struct platform_driver nxs_lvds_driver = {
	.probe	= nxs_lvds_probe,
	.remove	= nxs_lvds_remove,
	.driver	= {
		.name = "nxs-lvds",
		.of_match_table = of_match_ptr(nxs_lvds_match),
	},
};

static int __init nxs_lvds_init(void)
{
	return platform_driver_register(&nxs_lvds_driver);
}
/* subsys_initcall(nxs_lvds_init); */
fs_initcall(nxs_lvds_init);

static void __exit nxs_lvds_exit(void)
{
	platform_driver_unregister(&nxs_lvds_driver);
}
module_exit(nxs_lvds_exit);

MODULE_DESCRIPTION("Nexell Stream LVDS driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

