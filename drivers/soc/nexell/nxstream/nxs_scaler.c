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

#define SCALER_DIRTYSET_OFFSET	0x0004
#define SCALER_DIRTYCLR_OFFSET	0x0014
#define SCALER0_DIRTY		BIT(30)
#define SCALER1_DIRTY		BIT(29)
#define SCALER2_DIRTY		BIT(28)

#define SCALER_CFG_TID		0x0c20

/* SCALER CFG ENC TID */
#define SCALER_TID_SHIFT	0
#define SCALER_TID_MASK		GENMASK(13, 0)

struct nxs_scaler {
	struct nxs_dev nxs_dev;
	u32 line_buffer_size;
	struct regmap *reg;
	u32 offset;
};

#define nxs_to_scaler(dev)	container_of(dev, struct nxs_scaler, nxs_dev)

static void scaler_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 scaler_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 scaler_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void scaler_clear_interrupt_pending(const struct nxs_dev *pthis,
					     int type)
{
}

static int scaler_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int scaler_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int scaler_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int scaler_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int scaler_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);
	u32 dirty_val;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	switch (pthis->dev_inst_index) {
	case 0:
		dirty_val = SCALER0_DIRTY;
		break;
	case 1:
		dirty_val = SCALER1_DIRTY;
		break;
	case 2:
		dirty_val = SCALER2_DIRTY;
		break;
	default:
		dev_err(pthis->dev, "invalid inst %d\n", pthis->dev_inst_index);
		return -ENODEV;
	}

	return regmap_write(scaler->reg, SCALER_DIRTYSET_OFFSET, dirty_val);
}

static int scaler_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);

	return regmap_update_bits(scaler->reg, scaler->offset + SCALER_CFG_TID,
				  SCALER_TID_MASK, tid1 << SCALER_TID_SHIFT);
}

static int scaler_set_format(const struct nxs_dev *pthis,
			     const struct nxs_control *pparam)
{
	return 0;
}

static int scaler_get_format(const struct nxs_dev *pthis,
			     struct nxs_control *pparam)
{
	return 0;
}

static int scaler_set_dstformat(const struct nxs_dev *pthis,
				const struct nxs_control *pparam)
{
	return 0;
}

static int scaler_get_dstformat(const struct nxs_dev *pthis,
				struct nxs_control *pparam)
{
	return 0;
}

static int nxs_scaler_parse_dt(struct platform_device *pdev,
			       struct nxs_scaler *scaler)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;

	if (of_property_read_u32(np, "line_buffer_size",
				&scaler->line_buffer_size)) {
		dev_err(dev, "failed to get line_buffer_size\n");
		return -EINVAL;
	}

	return 0;
}

static int nxs_scaler_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_scaler *scaler;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	scaler = devm_kzalloc(&pdev->dev, sizeof(*scaler), GFP_KERNEL);
	if (!scaler)
		return -ENOMEM;

	nxs_dev = &scaler->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	ret = nxs_scaler_parse_dt(pdev, scaler);
	if (ret)
		return ret;

	scaler->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						      "syscon");
	if (IS_ERR(scaler->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(scaler->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	scaler->offset = res->start;

	nxs_dev->set_interrupt_enable = scaler_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = scaler_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = scaler_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = scaler_clear_interrupt_pending;
	nxs_dev->open = scaler_open;
	nxs_dev->close = scaler_close;
	nxs_dev->start = scaler_start;
	nxs_dev->stop = scaler_stop;
	nxs_dev->set_dirty = scaler_set_dirty;
	nxs_dev->set_tid = scaler_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_FORMAT;
	nxs_dev->dev_services[0].set_control = scaler_set_format;
	nxs_dev->dev_services[0].get_control = scaler_get_format;
	nxs_dev->dev_services[0].type = NXS_CONTROL_DST_FORMAT;
	nxs_dev->dev_services[0].set_control = scaler_set_dstformat;
	nxs_dev->dev_services[0].get_control = scaler_get_dstformat;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, scaler);

	return 0;
}

static int nxs_scaler_remove(struct platform_device *pdev)
{
	struct nxs_scaler *scaler = platform_get_drvdata(pdev);

	if (scaler)
		unregister_nxs_dev(&scaler->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_scaler_match[] = {
	{ .compatible = "nexell,scaler-nxs-1.0", },
	{},
};

static struct platform_driver nxs_scaler_driver = {
	.probe	= nxs_scaler_probe,
	.remove	= nxs_scaler_remove,
	.driver	= {
		.name = "nxs-scaler",
		.of_match_table = of_match_ptr(nxs_scaler_match),
	},
};

static int __init nxs_scaler_init(void)
{
	return platform_driver_register(&nxs_scaler_driver);
}
/* subsys_initcall(nxs_scaler_init); */
fs_initcall(nxs_scaler_init);

static void __exit nxs_scaler_exit(void)
{
	platform_driver_unregister(&nxs_scaler_driver);
}
module_exit(nxs_scaler_exit);

MODULE_DESCRIPTION("Nexell Stream scaler driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

