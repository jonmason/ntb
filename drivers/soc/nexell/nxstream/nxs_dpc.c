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

#define DPC_CTRL0		0x0020

/* DPC CTRL0 */
#define DIRTYFLAG_TOP_SHIFT	12
#define DIRTYFLAG_TOP_MASK	BIT(12)

struct nxs_dpc {
	struct nxs_dev nxs_dev;
	u8 *base;
};

#define nxs_to_dpc(dev)		container_of(dev, struct nxs_dpc, nxs_dev)

static void dpc_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 dpc_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 dpc_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void dpc_clear_interrupt_pending(const struct nxs_dev *pthis, int type)
{
}

static int dpc_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int dpc_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int dpc_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int dpc_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int dpc_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_dpc *dpc = nxs_to_dpc(pthis);
	u32 val;
	u8 *reg;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	reg = dpc->base + DPC_CTRL0;
	val = readl(reg);
	val |= 1 << DIRTYFLAG_TOP_SHIFT;
	writel(val, reg);

	return 0;
}

static int dpc_set_syncinfo(const struct nxs_dev *pthis,
			    const struct nxs_control *pparam)
{
	return 0;
}

static int dpc_get_syncinfo(const struct nxs_dev *pthis,
			    struct nxs_control *pparam)
{
	return 0;
}

static int nxs_dpc_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_dpc *dpc;
	struct nxs_dev *nxs_dev;
	struct resource res;

	dpc = devm_kzalloc(&pdev->dev, sizeof(*dpc), GFP_KERNEL);
	if (!dpc)
		return -ENOMEM;

	nxs_dev = &dpc->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	ret = of_address_to_resource(pdev->dev.of_node, 0, &res);
	if (ret) {
		dev_err(nxs_dev->dev, "[%s:%d] failed to get base address\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -ENXIO;
	}

	dpc->base = devm_ioremap_nocache(nxs_dev->dev, res.start,
					 resource_size(&res));
	if (!dpc->base) {
		dev_err(nxs_dev->dev, "[%s:%d] failed to ioremap\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
			return -EBUSY;
	}

	nxs_dev->set_interrupt_enable = dpc_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = dpc_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = dpc_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = dpc_clear_interrupt_pending;
	nxs_dev->open = dpc_open;
	nxs_dev->close = dpc_close;
	nxs_dev->start = dpc_start;
	nxs_dev->stop = dpc_stop;
	nxs_dev->set_dirty = dpc_set_dirty;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_SYNCINFO;
	nxs_dev->dev_services[0].set_control = dpc_set_syncinfo;
	nxs_dev->dev_services[0].get_control = dpc_get_syncinfo;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, dpc);

	return 0;
}

static int nxs_dpc_remove(struct platform_device *pdev)
{
	struct nxs_dpc *dpc = platform_get_drvdata(pdev);

	if (dpc)
		unregister_nxs_dev(&dpc->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_dpc_match[] = {
	{ .compatible = "nexell,dpc-nxs-1.0", },
	{},
};

static struct platform_driver nxs_dpc_driver = {
	.probe	= nxs_dpc_probe,
	.remove	= nxs_dpc_remove,
	.driver	= {
		.name = "nxs-dpc",
		.of_match_table = of_match_ptr(nxs_dpc_match),
	},
};

static int __init nxs_dpc_init(void)
{
	return platform_driver_register(&nxs_dpc_driver);
}
/* subsys_initcall(nxs_dpc_init); */
fs_initcall(nxs_dpc_init);

static void __exit nxs_dpc_exit(void)
{
	platform_driver_unregister(&nxs_dpc_driver);
}
module_exit(nxs_dpc_exit);

MODULE_DESCRIPTION("Nexell Stream DPC driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

