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
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/regmap.h>
#include <linux/interrupt.h>
#include <linux/mfd/syscon.h>
#include <linux/platform_device.h>

#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_res_manager.h>

#define FIFO_CTRL		0x0000
#define FIFO_INT		0x0004

/* FIFO CTRL */
#define FIFO_REG_CLEAR_SHIFT	21
#define FIFO_REG_CLEAR_MASK	BIT(21)
#define FIFO_DIRTY_SHIFT	20
#define FIFO_DIRTY_MASK		BIT(20)
#define FIFO_DIRTY_CLR_SHIFT	19
#define FIFO_DIRTY_CLR_MASK	BIT(19)
#define FIFO_TZPROT_SHIFT	18
#define FIFO_TZPROT_MASK	BIT(18)
#define FIFO_BUFFERED_ENABLE_SHIFT	17
#define FIFO_BUFFERED_ENABLE_MASK	BIT(17)
#define FIFO_ENABLE_SHIFT	16
#define FIFO_ENABLE_MASK	BIT(16)
#define FIFO_IDLE_SHIFT		15
#define FIFO_IDLE_MASK		BIT(15)
#define FIFO_INIT_SHIFT		14
#define FIFO_INIT_MASK		BIT(14)
#define FIFO_TID_SHIFT		0
#define FIFO_TID_MASK		GENMASK(13, 0)

/* FIFO INT */
#define FIFO_IDLE_INTDISABLE_SHIFT	6
#define FIFO_IDLE_INTDISABLE_MASK	BIT(6)
#define FIFO_IDLE_INTENB_SHIFT		5
#define FIFO_IDLE_INTENB_MASK		BIT(5)
#define FIFO_IDLE_INTPEND_CLR_SHIFT	4
#define FIFO_IDLE_INTPEND_CLR_MASK	BIT(4)
#define FIFO_UPDATE_INTDISABLE_SHIFT	2
#define FIFO_UPDATE_INTDISABLE_MASK	BIT(2)
#define FIFO_UPDATE_INTENB_SHIFT	1
#define FIFO_UPDATE_INTENB_MASK		BIT(1)
#define FIFO_UPDATE_INTPEND_CLR_SHIFT	0
#define FIFO_UPDATE_INTPEND_CLR_MASK	BIT(0)

struct nxs_fifo {
	struct nxs_dev nxs_dev;
	struct regmap *reg;
	u32 offset;
};

#define nxs_to_fifo(dev)	container_of(dev, struct nxs_fifo, nxs_dev)

static void fifo_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 fifo_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 fifo_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void fifo_clear_interrupt_pending(const struct nxs_dev *pthis, int type)
{
}

static int fifo_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int fifo_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int fifo_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int fifo_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int fifo_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_fifo *fifo = nxs_to_fifo(pthis);

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	return regmap_update_bits(fifo->reg, fifo->offset + FIFO_CTRL,
				  FIFO_DIRTY_MASK,
				  1 << FIFO_DIRTY_SHIFT);
}

static int fifo_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_fifo *fifo = nxs_to_fifo(pthis);

	return regmap_update_bits(fifo->reg, fifo->offset + FIFO_CTRL,
				  FIFO_TID_MASK,
				  tid1 << FIFO_TID_SHIFT);
}

static int nxs_fifo_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_fifo *fifo;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	fifo = devm_kzalloc(&pdev->dev, sizeof(*fifo), GFP_KERNEL);
	if (!fifo)
		return -ENOMEM;

	nxs_dev = &fifo->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	fifo->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						    "syscon");
	if (IS_ERR(fifo->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(fifo->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	fifo->offset = res->start;

	nxs_dev->set_interrupt_enable = fifo_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = fifo_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = fifo_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = fifo_clear_interrupt_pending;
	nxs_dev->open = fifo_open;
	nxs_dev->close = fifo_close;
	nxs_dev->start = fifo_start;
	nxs_dev->stop = fifo_stop;
	nxs_dev->set_dirty = fifo_set_dirty;
	nxs_dev->set_tid = fifo_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, fifo);

	return 0;
}

static int nxs_fifo_remove(struct platform_device *pdev)
{
	struct nxs_fifo *fifo = platform_get_drvdata(pdev);

	if (fifo)
		unregister_nxs_dev(&fifo->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_fifo_match[] = {
	{ .compatible = "nexell,fifo-nxs-1.0", },
	{},
};

static struct platform_driver nxs_fifo_driver = {
	.probe	= nxs_fifo_probe,
	.remove	= nxs_fifo_remove,
	.driver	= {
		.name = "nxs-fifo",
		.of_match_table = of_match_ptr(nxs_fifo_match),
	},
};

static int __init nxs_fifo_init(void)
{
	return platform_driver_register(&nxs_fifo_driver);
}
/* subsys_initcall(nxs_fifo_init); */
fs_initcall(nxs_fifo_init);

static void __exit nxs_fifo_exit(void)
{
	platform_driver_unregister(&nxs_fifo_driver);
}
module_exit(nxs_fifo_exit);

MODULE_DESCRIPTION("Nexell Stream FIFO driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

