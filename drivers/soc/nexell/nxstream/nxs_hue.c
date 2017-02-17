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

#define HUE_DIRTYSET_OFFSET	0x0004
#define HUE_DIRTYCLR_OFFSET	0x0014
#define HUE0_DIRTY		BIT(8)
#define HUE1_DIRTY		BIT(9)

#define HUE_CTRL		0x0000

/* HUE CTRL */
#define HUE_REG_CLEAR_SHIFT	31
#define HUE_REG_CLEAR_MASK	BIT(31)
#define HUE_TZPROT_SHIFT	30
#define HUE_TZPROT_MASK		BIT(30)
#define HUE_IDLE_SHIFT		29
#define HUE_IDLE_MASK		BIT(29)
#define HUE_ENABLE_SHIFT	24
#define HUE_ENABLE_MASK		BIT(24)
#define HUE_CB_COMP_INDEX_SHIFT	18
#define HUE_CB_COMP_INDEX_MASK	GENMASK(19, 18)
#define HUE_CR_COMP_INDEX_SHIFT	16
#define	HUE_CR_COMP_INDEX_MASK	GENMASK(17, 16)
#define HUE_TID_SHIFT		0
#define HUE_TID_MASK		GENMASK(13, 0)

struct nxs_hue {
	struct nxs_dev nxs_dev;
	struct regmap *reg;
	u32 offset;
};

#define nxs_to_hue(dev)       container_of(dev, struct nxs_hue, nxs_dev)

static void hue_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 hue_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 hue_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void hue_clear_interrupt_pending(const struct nxs_dev *pthis, int type)
{
}

static int hue_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int hue_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int hue_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int hue_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int hue_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_hue *hue = nxs_to_hue(pthis);
	u32 dirty_val;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	if (pthis->dev_inst_index == 0)
		dirty_val = HUE0_DIRTY;
	else if (pthis->dev_inst_index == 1)
		dirty_val = HUE1_DIRTY;
	else {
		dev_err(pthis->dev, "invalid inst %d\n", pthis->dev_inst_index);
		return -ENODEV;
	}

	return regmap_write(hue->reg, HUE_DIRTYSET_OFFSET, dirty_val);
}

static int hue_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_hue *hue = nxs_to_hue(pthis);

	return regmap_update_bits(hue->reg, hue->offset + HUE_CTRL,
				  HUE_TID_MASK, tid1 << HUE_TID_SHIFT);
}

static int nxs_hue_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_hue *hue;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	hue = devm_kzalloc(&pdev->dev, sizeof(*hue), GFP_KERNEL);
	if (!hue)
		return -ENOMEM;

	nxs_dev = &hue->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	hue->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node, "syscon");
	if (IS_ERR(hue->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(hue->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	hue->offset = res->start;

	nxs_dev->set_interrupt_enable = hue_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = hue_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = hue_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = hue_clear_interrupt_pending;
	nxs_dev->open = hue_open;
	nxs_dev->close = hue_close;
	nxs_dev->start = hue_start;
	nxs_dev->stop = hue_stop;
	nxs_dev->set_dirty = hue_set_dirty;
	nxs_dev->set_tid = hue_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, hue);

	return 0;
}

static int nxs_hue_remove(struct platform_device *pdev)
{
	struct nxs_hue *hue = platform_get_drvdata(pdev);

	if (hue)
		unregister_nxs_dev(&hue->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_hue_match[] = {
	{ .compatible = "nexell,hue-nxs-1.0", },
	{},
};

static struct platform_driver nxs_hue_driver = {
	.probe	= nxs_hue_probe,
	.remove	= nxs_hue_remove,
	.driver	= {
		.name = "nxs-hue",
		.of_match_table = of_match_ptr(nxs_hue_match),
	},
};

static int __init nxs_hue_init(void)
{
	return platform_driver_register(&nxs_hue_driver);
}
/* subsys_initcall(nxs_hue_init); */
fs_initcall(nxs_hue_init);

static void __exit nxs_hue_exit(void)
{
	platform_driver_unregister(&nxs_hue_driver);
}
module_exit(nxs_hue_exit);

MODULE_DESCRIPTION("Nexell Stream HUE driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

