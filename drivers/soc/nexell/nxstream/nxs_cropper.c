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

#define CROPPER_DIRTYSET_OFFSET	0x0004
#define CROPPER_DIRTYCLR_OFFSET	0x0014
#define CROPPER0_DIRTY		BIT(1)
#define CROPPER1_DIRTY		BIT(0)

#define CROP_CTRL		0x0000

/* CROPPER CTRL */
#define CROP_REG_CLEAR_SHIFT	31
#define CROP_REG_CLEAR_MASK	BIT(31)
#define CROP_TZPROT_SHIFT	30
#define CROP_TZPROT_MASK	BIT(30)
#define CROP_TID_SHIFT		16
#define CROP_TID_MASK		GENMASK(29, 16)
#define CROP_BYPASS_SHIFT	1
#define CROP_BYPASS_MASK	BIT(1)
#define CROP_ENABLE_SHIFT	0
#define CROP_ENABLE_MASK	BIT(0)

struct nxs_cropper {
	struct nxs_dev nxs_dev;
	struct regmap *reg;
	u32 offset;
};

#define nxs_to_cropper(dev)       container_of(dev, struct nxs_cropper, nxs_dev)

static void cropper_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 cropper_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 cropper_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void cropper_clear_interrupt_pending(const struct nxs_dev *pthis,
					     int type)
{
}

static int cropper_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int cropper_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int cropper_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int cropper_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int cropper_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_cropper *cropper = nxs_to_cropper(pthis);
	u32 dirty_val;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	if (pthis->dev_inst_index == 0)
		dirty_val = CROPPER0_DIRTY;
	else if (pthis->dev_inst_index == 1)
		dirty_val = CROPPER1_DIRTY;
	else {
		dev_err(pthis->dev, "invalid inst %d\n", pthis->dev_inst_index);
		return -ENODEV;
	}

	return regmap_write(cropper->reg, CROPPER_DIRTYSET_OFFSET, dirty_val);
}

static int cropper_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_cropper *cropper = nxs_to_cropper(pthis);

	return regmap_update_bits(cropper->reg, cropper->offset + CROP_CTRL,
				  CROP_TID_MASK, tid1 << CROP_TID_SHIFT);
}

static int cropper_set_format(const struct nxs_dev *pthis,
			      const struct nxs_control *pparam)
{
	return 0;
}

static int cropper_get_format(const struct nxs_dev *pthis,
			      struct nxs_control *pparam)
{
	return 0;
}

static int cropper_set_crop(const struct nxs_dev *pthis,
			    const struct nxs_control *pparam)
{
	return 0;
}

static int cropper_get_crop(const struct nxs_dev *pthis,
			    struct nxs_control *pparam)
{
	return 0;
}

static int nxs_cropper_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_cropper *cropper;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	cropper = devm_kzalloc(&pdev->dev, sizeof(*cropper), GFP_KERNEL);
	if (!cropper)
		return -ENOMEM;

	nxs_dev = &cropper->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	cropper->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						       "syscon");
	if (IS_ERR(cropper->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(cropper->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	cropper->offset = res->start;

	nxs_dev->set_interrupt_enable = cropper_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = cropper_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = cropper_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = cropper_clear_interrupt_pending;
	nxs_dev->open = cropper_open;
	nxs_dev->close = cropper_close;
	nxs_dev->start = cropper_start;
	nxs_dev->stop = cropper_stop;
	nxs_dev->set_dirty = cropper_set_dirty;
	nxs_dev->set_tid = cropper_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_FORMAT;
	nxs_dev->dev_services[0].set_control = cropper_set_format;
	nxs_dev->dev_services[0].get_control = cropper_get_format;
	nxs_dev->dev_services[1].type = NXS_CONTROL_CROP;
	nxs_dev->dev_services[1].set_control = cropper_set_crop;
	nxs_dev->dev_services[1].get_control = cropper_get_crop;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, cropper);

	return 0;
}

static int nxs_cropper_remove(struct platform_device *pdev)
{
	struct nxs_cropper *cropper = platform_get_drvdata(pdev);

	if (cropper)
		unregister_nxs_dev(&cropper->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_cropper_match[] = {
	{ .compatible = "nexell,cropper-nxs-1.0", },
	{},
};

static struct platform_driver nxs_cropper_driver = {
	.probe	= nxs_cropper_probe,
	.remove	= nxs_cropper_remove,
	.driver	= {
		.name = "nxs-cropper",
		.of_match_table = of_match_ptr(nxs_cropper_match),
	},
};

static int __init nxs_cropper_init(void)
{
	return platform_driver_register(&nxs_cropper_driver);
}
/* subsys_initcall(nxs_cropper_init); */
fs_initcall(nxs_cropper_init);

static void __exit nxs_cropper_exit(void)
{
	platform_driver_unregister(&nxs_cropper_driver);
}
module_exit(nxs_cropper_exit);

MODULE_DESCRIPTION("Nexell Stream CROPPER driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

