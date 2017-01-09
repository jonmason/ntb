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
#include <dt-bindings/soc/nxs_tid.h>

#define MULTITAP_DIRTYSET_OFFSET	0x0000
#define MULTITAP_DIRTYCLR_OFFSET	0x0010
#define MULTITAP0_DIRTY			BIT(15)
#define MULTITAP1_DIRTY			BIT(16)
#define MULTITAP2_DIRTY			BIT(17)
#define MULTITAP3_DIRTY			BIT(18)

#define MULTITAP_CTRL1			0x0004

/* MULTITAP CTRL1 */
#define MULTITAP_TID1_SHIFT		16
#define MULTITAP_TID1_MASK		GENMASK(29, 16)
#define MULTITAP_TID0_SHIFT		0
#define MULTITAP_TID0_MASK		GENMASK(13, 0)

struct nxs_multitap {
	struct nxs_dev nxs_dev;
	struct regmap *reg;
	u32 offset;
};

#define nxs_to_multitap(dev)    container_of(dev, struct nxs_multitap, nxs_dev)

static void multitap_set_interrupt_enable(const struct nxs_dev *pthis, int type,
					  bool enable)
{
}

static u32 multitap_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 multitap_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void multitap_clear_interrupt_pending(const struct nxs_dev *pthis,
					     int type)
{
}

static int multitap_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int multitap_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int multitap_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int multitap_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int multitap_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_multitap *multitap = nxs_to_multitap(pthis);
	u32 dirty_val;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	switch (pthis->dev_inst_index) {
	case 0:
		dirty_val = MULTITAP0_DIRTY;
		break;
	case 1:
		dirty_val = MULTITAP1_DIRTY;
		break;
	case 2:
		dirty_val = MULTITAP2_DIRTY;
		break;
	case 3:
		dirty_val = MULTITAP3_DIRTY;
		break;
	default:
		dev_err(pthis->dev, "invalid inst %d\n", pthis->dev_inst_index);
		return -ENODEV;
	}

	return regmap_write(multitap->reg, MULTITAP_DIRTYSET_OFFSET, dirty_val);
}

static int multitap_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_multitap *multitap = nxs_to_multitap(pthis);
	int ret = 0;

	if (tid1 != NXS_TID_DEFAULT) {
		ret = regmap_update_bits(multitap->reg,
					 multitap->offset + MULTITAP_CTRL1,
					 MULTITAP_TID0_MASK,
					 tid1 << MULTITAP_TID0_SHIFT);
		if (ret)
			return ret;
	}

	if (tid2 != NXS_TID_DEFAULT)
		ret = regmap_update_bits(multitap->reg,
					 multitap->offset + MULTITAP_CTRL1,
					 MULTITAP_TID1_MASK,
					 tid2 << MULTITAP_TID1_SHIFT);

	return ret;
}

static int multitap_set_syncinfo(const struct nxs_dev *pthis,
			    const struct nxs_control *pparam)
{
	return 0;
}

static int multitap_get_syncinfo(const struct nxs_dev *pthis,
			    struct nxs_control *pparam)
{
	return 0;
}

static int nxs_multitap_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_multitap *multitap;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	multitap = devm_kzalloc(&pdev->dev, sizeof(*multitap), GFP_KERNEL);
	if (!multitap)
		return -ENOMEM;

	nxs_dev = &multitap->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	multitap->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
							"syscon");
	if (IS_ERR(multitap->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(multitap->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	multitap->offset = res->start;

	nxs_dev->set_interrupt_enable = multitap_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = multitap_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = multitap_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = multitap_clear_interrupt_pending;
	nxs_dev->open = multitap_open;
	nxs_dev->close = multitap_close;
	nxs_dev->start = multitap_start;
	nxs_dev->stop = multitap_stop;
	nxs_dev->set_dirty = multitap_set_dirty;
	nxs_dev->set_tid = multitap_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_SYNCINFO;
	nxs_dev->dev_services[0].set_control = multitap_set_syncinfo;
	nxs_dev->dev_services[0].get_control = multitap_get_syncinfo;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, multitap);

	return 0;
}

static int nxs_multitap_remove(struct platform_device *pdev)
{
	struct nxs_multitap *multitap = platform_get_drvdata(pdev);

	if (multitap)
		unregister_nxs_dev(&multitap->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_multitap_match[] = {
	{ .compatible = "nexell,multitap-nxs-1.0", },
	{},
};

static struct platform_driver nxs_multitap_driver = {
	.probe	= nxs_multitap_probe,
	.remove	= nxs_multitap_remove,
	.driver	= {
		.name = "nxs-multitap",
		.of_match_table = of_match_ptr(nxs_multitap_match),
	},
};

static int __init nxs_multitap_init(void)
{
	return platform_driver_register(&nxs_multitap_driver);
}
/* subsys_initcall(nxs_multitap_init); */
fs_initcall(nxs_multitap_init);

static void __exit nxs_multitap_exit(void)
{
	platform_driver_unregister(&nxs_multitap_driver);
}
module_exit(nxs_multitap_exit);

MODULE_DESCRIPTION("Nexell Stream MULTITAP driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");
