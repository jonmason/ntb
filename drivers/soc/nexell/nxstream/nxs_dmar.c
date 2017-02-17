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

#define DMAR_DIRTYSET_OFFSET	0x0000
#define DMAR_DIRTYCLR_OFFSET	0x0010
#define DMAR0_DIRTY		BIT(0)
#define DMAR1_DIRTY		BIT(1)
#define DMAR2_DIRTY		BIT(2)
#define DMAR3_DIRTY		BIT(3)
#define DMAR4_DIRTY		BIT(4)
#define DMAR5_DIRTY		BIT(5)
#define DMAR6_DIRTY		BIT(6)
#define DMAR7_DIRTY		BIT(7)
#define DMAR8_DIRTY		BIT(8)
#define DMAR9_DIRTY		BIT(9)

struct nxs_dmar {
	struct nxs_dev nxs_dev;
	struct regmap *reg;
	u32 offset;
};

#define nxs_to_dmar(dev)	container_of(dev, struct nxs_dmar, nxs_dev)

static void dmar_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 dmar_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 dmar_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void dmar_clear_interrupt_pending(const struct nxs_dev *pthis, int type)
{
}

static int dmar_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int dmar_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int dmar_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int dmar_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int dmar_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);
	u32 dirty_val;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	switch (pthis->dev_inst_index) {
	case 0:
		dirty_val = DMAR0_DIRTY;
		break;
	case 1:
		dirty_val = DMAR1_DIRTY;
		break;
	case 2:
		dirty_val = DMAR2_DIRTY;
		break;
	case 3:
		dirty_val = DMAR3_DIRTY;
		break;
	case 4:
		dirty_val = DMAR4_DIRTY;
		break;
	case 5:
		dirty_val = DMAR5_DIRTY;
		break;
	case 6:
		dirty_val = DMAR6_DIRTY;
		break;
	case 7:
		dirty_val = DMAR7_DIRTY;
		break;
	case 8:
		dirty_val = DMAR8_DIRTY;
		break;
	case 9:
		dirty_val = DMAR9_DIRTY;
		break;
	default:
		dev_err(pthis->dev, "invalid inst %d\n", pthis->dev_inst_index);
		return -ENODEV;
	}

	return regmap_write(dmar->reg, DMAR_DIRTYSET_OFFSET, dirty_val);
}

static int dmar_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	return 0;
}

static int dmar_set_format(const struct nxs_dev *pthis,
			   const struct nxs_control *pparam)
{
	return 0;
}

static int dmar_get_format(const struct nxs_dev *pthis,
			   struct nxs_control *pparam)
{
	return 0;
}

static int dmar_set_buffer(const struct nxs_dev *pthis,
			   const struct nxs_control *pparam)
{
	return 0;
}

static int dmar_get_buffer(const struct nxs_dev *pthis,
			   struct nxs_control *pparam)
{
	return 0;
}

static int nxs_dmar_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_dmar *dmar;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	dmar = devm_kzalloc(&pdev->dev, sizeof(*dmar), GFP_KERNEL);
	if (!dmar)
		return -ENOMEM;

	nxs_dev = &dmar->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	dmar->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						    "syscon");
	if (IS_ERR(dmar->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(dmar->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	dmar->offset = res->start;

	nxs_dev->set_interrupt_enable = dmar_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = dmar_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = dmar_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = dmar_clear_interrupt_pending;
	nxs_dev->open = dmar_open;
	nxs_dev->close = dmar_close;
	nxs_dev->start = dmar_start;
	nxs_dev->stop = dmar_stop;
	nxs_dev->set_dirty = dmar_set_dirty;
	nxs_dev->set_tid = dmar_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_FORMAT;
	nxs_dev->dev_services[0].set_control = dmar_set_format;
	nxs_dev->dev_services[0].get_control = dmar_get_format;
	nxs_dev->dev_services[1].type = NXS_CONTROL_BUFFER;
	nxs_dev->dev_services[1].set_control = dmar_set_buffer;
	nxs_dev->dev_services[1].get_control = dmar_get_buffer;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, dmar);

	return 0;
}

static int nxs_dmar_remove(struct platform_device *pdev)
{
	struct nxs_dmar *dmar = platform_get_drvdata(pdev);

	if (dmar)
		unregister_nxs_dev(&dmar->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_dmar_match[] = {
	{ .compatible = "nexell,dmar-nxs-1.0", },
	{},
};

static struct platform_driver nxs_dmar_driver = {
	.probe	= nxs_dmar_probe,
	.remove	= nxs_dmar_remove,
	.driver	= {
		.name = "nxs-dmar",
		.of_match_table = of_match_ptr(nxs_dmar_match),
	},
};

static int __init nxs_dmar_init(void)
{
	return platform_driver_register(&nxs_dmar_driver);
}
fs_initcall(nxs_dmar_init);

static void __exit nxs_dmar_exit(void)
{
	platform_driver_unregister(&nxs_dmar_driver);
}
module_exit(nxs_dmar_exit);

MODULE_DESCRIPTION("Nexell Stream DMAR driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");
