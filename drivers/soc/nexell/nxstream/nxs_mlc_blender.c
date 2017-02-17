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

#define BLENDER_DIRTYSET_OFFSET	0x0004
#define BLENDER_DIRTYCLR_OFFSET	0x0014
#define BLENDER0_DIRTY		BIT(12)
#define BLENDER1_DIRTY		BIT(13)
#define BLENDER2_DIRTY		BIT(14)
#define BLENDER3_DIRTY		BIT(15)
#define BLENDER4_DIRTY		BIT(16)
#define BLENDER5_DIRTY		BIT(17)
#define BLENDER6_DIRTY		BIT(18)
#define BLENDER7_DIRTY		BIT(19)
#define BLENDER0_TID_DIRTY	BIT(20)
#define BLENDER1_TID_DIRTY	BIT(21)
#define BLENDER2_TID_DIRTY	BIT(22)
#define BLENDER3_TID_DIRTY	BIT(23)
#define BLENDER4_TID_DIRTY	BIT(24)
#define BLENDER5_TID_DIRTY	BIT(25)
#define BLENDER6_TID_DIRTY	BIT(26)
#define BLENDER7_TID_DIRTY	BIT(27)

#define BLENDER_CTRL0		0x0000

/* BLENDER CTRL0 */
#define BLENDER_REG_CLEAR_SHIFT	15
#define BLENDER_REG_CLEAR_MASK	BIT(15)
#define BLENDER_TZPROT_SHIFT	14
#define BLENDER_TZPROT_MASK	BIT(14)
#define BLENDER_TID_SHIFT	0
#define BLENDER_TID_MASK	GENMASK(13, 0)

struct nxs_blender {
	struct nxs_dev nxs_dev;
	struct regmap *reg;
	u32 offset;
};

#define nxs_to_blender(dev)       container_of(dev, struct nxs_blender, nxs_dev)

static void mlc_blender_set_interrupt_enable(const struct nxs_dev *pthis,
					     int type, bool enable)
{
}

static u32 mlc_blender_get_interrupt_enable(const struct nxs_dev *pthis,
					    int type)
{
	return 0;
}

static u32 mlc_blender_get_interrupt_pending(const struct nxs_dev *pthis,
					     int type)
{
	return 0;
}

static void mlc_blender_clear_interrupt_pending(const struct nxs_dev *pthis,
						int type)
{
}

static int mlc_blender_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int mlc_blender_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int mlc_blender_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int mlc_blender_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int mlc_blender_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_blender *blender = nxs_to_blender(pthis);
	u32 dirty_val;

	if (type == NXS_DEV_DIRTY_TID) {
		switch (pthis->dev_inst_index) {
		case 0:
			dirty_val = BLENDER0_TID_DIRTY;
			break;
		case 1:
			dirty_val = BLENDER1_TID_DIRTY;
			break;
		case 2:
			dirty_val = BLENDER2_TID_DIRTY;
			break;
		case 3:
			dirty_val = BLENDER3_TID_DIRTY;
			break;
		case 4:
			dirty_val = BLENDER4_TID_DIRTY;
			break;
		case 5:
			dirty_val = BLENDER5_TID_DIRTY;
			break;
		case 6:
			dirty_val = BLENDER6_TID_DIRTY;
			break;
		case 7:
			dirty_val = BLENDER7_TID_DIRTY;
			break;
		default:
			dev_err(pthis->dev, "invalid inst %d\n",
				pthis->dev_inst_index);
			return -ENODEV;
		}

		return regmap_write(blender->reg, BLENDER_DIRTYSET_OFFSET,
				    dirty_val);
	} else {
		switch (pthis->dev_inst_index) {
		case 0:
			dirty_val = BLENDER0_DIRTY;
			break;
		case 1:
			dirty_val = BLENDER1_DIRTY;
			break;
		case 2:
			dirty_val = BLENDER2_DIRTY;
			break;
		case 3:
			dirty_val = BLENDER3_DIRTY;
			break;
		case 4:
			dirty_val = BLENDER4_DIRTY;
			break;
		case 5:
			dirty_val = BLENDER5_DIRTY;
			break;
		case 6:
			dirty_val = BLENDER6_DIRTY;
			break;
		case 7:
			dirty_val = BLENDER7_DIRTY;
			break;
		default:
			dev_err(pthis->dev, "invalid inst %d\n",
				pthis->dev_inst_index);
			return -ENODEV;
		}

		return regmap_write(blender->reg, BLENDER_DIRTYSET_OFFSET,
				    dirty_val);
	}
}

static int mlc_blender_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_blender *blender = nxs_to_blender(pthis);
	u32 dirty_val;

	return regmap_update_bits(blender->reg, blender->offset + BLENDER_CTRL0,
				  BLENDER_TID_MASK, tid1 << BLENDER_TID_SHIFT);
}

static int mlc_blender_set_format(const struct nxs_dev *pthis,
				  const struct nxs_control *pparam)
{
	return 0;
}

static int mlc_blender_get_format(const struct nxs_dev *pthis,
				  struct nxs_control *pparam)
{
	return 0;
}

static int nxs_mlc_blender_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_blender *blender;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	blender = devm_kzalloc(&pdev->dev, sizeof(*blender), GFP_KERNEL);
	if (!blender)
		return -ENOMEM;

	nxs_dev = &blender->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	blender->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						       "syscon");
	if (IS_ERR(blender->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(blender->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	blender->offset = res->start;

	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	nxs_dev->set_interrupt_enable = mlc_blender_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = mlc_blender_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = mlc_blender_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = mlc_blender_clear_interrupt_pending;
	nxs_dev->open = mlc_blender_open;
	nxs_dev->close = mlc_blender_close;
	nxs_dev->start = mlc_blender_start;
	nxs_dev->stop = mlc_blender_stop;
	nxs_dev->set_dirty = mlc_blender_set_dirty;
	nxs_dev->set_tid = mlc_blender_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_FORMAT;
	nxs_dev->dev_services[0].set_control = mlc_blender_set_format;
	nxs_dev->dev_services[0].get_control = mlc_blender_get_format;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, blender);

	return 0;
}

static int nxs_mlc_blender_remove(struct platform_device *pdev)
{
	struct nxs_blender *blender = platform_get_drvdata(pdev);

	if (blender)
		unregister_nxs_dev(&blender->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_mlc_blender_match[] = {
	{ .compatible = "nexell,mlc_blender-nxs-1.0", },
	{},
};

static struct platform_driver nxs_mlc_blender_driver = {
	.probe	= nxs_mlc_blender_probe,
	.remove	= nxs_mlc_blender_remove,
	.driver	= {
		.name = "nxs-mlc_blender",
		.of_match_table = of_match_ptr(nxs_mlc_blender_match),
	},
};

static int __init nxs_mlc_blender_init(void)
{
	return platform_driver_register(&nxs_mlc_blender_driver);
}
/* subsys_initcall(nxs_mlc_blender_init); */
fs_initcall(nxs_mlc_blender_init);

static void __exit nxs_mlc_blender_exit(void)
{
	platform_driver_unregister(&nxs_mlc_blender_driver);
}
module_exit(nxs_mlc_blender_exit);

MODULE_DESCRIPTION("Nexell Stream MLC_BLENDER driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

