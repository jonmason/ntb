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

#define TPGEN_DIRTYSET_OFFSET	0x0004
#define TPGEN_DIRTYCLR_OFFSET	0x0014
#define TPGEN_DIRTY		BIT(31)

/* regmap: base 0x20c03e00 */
#define TPGEN_BASIC_CTRL	0x0000
#define TPGEN_INTERRUPT		0x0004
#define TPGEN_USERDEF_HEADER0	0x0008
#define TPGEN_USERDEF_HEADER1	0x000c
#define TPGEN_USERDEF_HEADER2	0x0010
#define TPGEN_USERDEF_HEADER3	0x0014
#define TPGEN_IMG_SIZE		0x0018
#define TPGEN_CTRL		0x001c
#define TPGEN_V2H_DLY_CTRL	0x0020
#define TPGEN_H2H_DLY_CTRL	0x0024
#define TPGEN_H2V_DLY_CTRL	0x0028
#define TPGEN_V2V_DLY_CTRL	0x002c

/* TPGEN BASIC CTRL */
#define TPGEN_REG_CLEAR_SHIFT		17
#define TPGEN_REG_CLEAR_MASK		BIT(17)
#define TPGEN_TZPROT_SHIFT		16
#define TPGEN_TZPROT_MASK		BIT(16)
#define TPGEN_NCLK_PER_TWOPIX_SHIFT	8
#define TPGEN_NCLK_PER_TWOPIX_MASK	GENMASK(15, 8)
#define TPGEN_REALTIME_MODE_SHIFT	7
#define TPGEN_REALTIME_MODE_MASK	BIT(7)
#define TPGEN_TPGEN_MODE_SHIFT		4
#define TPGEN_TPGEN_MODE_MASK		GENMASK(6, 4)
#define TPGEN_IDLE_SHIFT		1
#define TPGEN_IDLE_MASK			BIT(1)
#define TPGEN_TPGEN_ENABLE_SHIFT	0
#define TPGEN_TPGEN_ENABLE_MASK		BIT(0)

/* TPGEN INTERRUPT */
#define TPGEN_UPDATE_INTDISABLE_SHIFT	14
#define TPGEN_UPDATE_INTDISABLE_MASK	BIT(14)
#define TPGEN_UPDATE_INTENB_SHIFT	13
#define TPGEN_UPDATE_INTENB_MASK	BIT(13)
#define TPGEN_UPDATE_INTPEND_CLR_SHIFT	12
#define TPGEN_UPDATE_INTPEND_CLR_MASK	BIT(12)
#define TPGEN_IRQ_OVF_INTDISABLE_SHIFT	10
#define TPGEN_IRQ_OVF_INTDISABLE_MASK	BIT(10)
#define TPGEN_IRQ_OVF_INTENB_SHIFT	9
#define TPGEN_IRQ_OVF_INTENB_MASK	BIT(9)
#define TPGEN_IRQ_OVF_INTPEND_SHIFT	8
#define TPGEN_IRQ_OVF_INTPEND_MASK	BIT(8)
#define TPGEN_IRQ_IDLE_INTDISABLE_SHIFT 6
#define TPGEN_IRQ_IDLE_INTDISABLE_MASK	BIT(6)
#define TPGEN_IRQ_IDLE_INTENB_SHIFT	5
#define TPGEN_IRQ_IDLE_INTENB_MASK	BIT(5)
#define TPGEN_IRQ_IDLE_INTPEND_SHIFT	4
#define TPGEN_IRQ_IDLE_INTPEND_MASK	BIT(4)
#define TPGEN_IRQ_DONE_INTDISABLE_SHIFT 2
#define TPGEN_IRQ_DONE_INTDISABLE_MASK	BIT(2)
#define TPGEN_IRQ_DONE_INTENB_SHIFT	1
#define TPGEN_IRQ_DONE_INTENB_MASK	BIT(1)
#define TPGEN_IRQ_DONE_INTPEND_SHIFT	0
#define TPGEN_IRQ_DONE_INTPEND_MASK	BIT(0)

/* TPGEN USER DEF HEADER 0 */
#define TPGEN_ENC_HEADER_0_SHIFT	0
#define TPGEN_ENC_HEADER_0_MASK		GENMASK(31, 0)

/* TPGEN USER DEF HEADER 1 */
#define TPGEN_ENC_HEADER_1_SHIFT	0
#define TPGEN_ENC_HEADER_1_MASK		GENMASK(31, 0)

/* TPGEN USER DEF HEADER 2 */
#define TPGEN_ENC_HEADER_2_SHIFT	0
#define TPGEN_ENC_HEADER_2_MASK		GENMASK(31, 0)

/* TPGEN USER DEF HEADER 3 */
#define TPGEN_ENC_HEADER_3_SHIFT	0
#define TPGEN_ENC_HEADER_3_MASK		GENMASK(31, 0)

/* TPGEN IMG SIZE */
#define TPGEN_ENC_WIDTH_SHIFT		16
#define TPGEN_ENC_WIDTH_MASK		GENMASK(31, 16)
#define TPGEN_ENC_HEIGHT_SHIFT		0
#define TPGEN_ENC_HEIGHT_MASK		GENMASK(15, 0)

/* TPGEN CTRL */
#define TPGEN_ENC_IMG_TYPE_SHIFT	18
#define TPGEN_ENC_IMG_TYPE_MASK		GENMASK(19, 18)
#define TPGEN_ENC_FIELD_SHIFT		16
#define TPGEN_ENC_FIELD_MASK		GENMASK(17, 16)
#define TPGEN_ENC_SECURE_SHIFT		15
#define TPGEN_ENC_SECURE_MASK		BIT(15)
#define TPGEN_ENC_REG_FIRE_SHIFT	14
#define TPGEN_ENC_REG_FIRE_MASK		BIT(14)
#define TPGEN_ENC_TID_SHIFT		0
#define TPGEN_ENC_TID_MASK		GENMASK(13, 0)

struct nxs_tpgen {
	struct nxs_dev nxs_dev;
	struct regmap *reg;
	u32 offset;
};

#define nxs_to_tpgen(dev)	container_of(dev, struct nxs_tpgen, nxs_dev)

static void tpgen_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 tpgen_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 tpgen_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void tpgen_clear_interrupt_pending(const struct nxs_dev *pthis, int type)
{
}

static int tpgen_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int tpgen_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int tpgen_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int tpgen_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int tpgen_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_tpgen *tpgen = nxs_to_tpgen(pthis);

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	return regmap_write(tpgen->reg, TPGEN_DIRTYSET_OFFSET,
			    TPGEN_DIRTY);
}

static int tpgen_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_tpgen *tpgen = nxs_to_tpgen(pthis);

	return regmap_update_bits(tpgen->reg, tpgen->offset + TPGEN_CTRL,
				  TPGEN_ENC_TID_MASK,
				  tid1 << TPGEN_ENC_TID_SHIFT);
}

static int tpgen_set_format(const struct nxs_dev *pthis,
			    const struct nxs_control *pparam)
{
	return 0;
}

static int tpgen_get_format(const struct nxs_dev *pthis,
			    struct nxs_control *pparam)
{
	return 0;
}

static int nxs_tpgen_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_tpgen *tpgen;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	tpgen = devm_kzalloc(&pdev->dev, sizeof(*tpgen), GFP_KERNEL);
	if (!tpgen)
		return -ENOMEM;

	nxs_dev = &tpgen->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	tpgen->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						     "syscon");
	if (IS_ERR(tpgen->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(tpgen->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	tpgen->offset = res->start;

	nxs_dev->set_interrupt_enable = tpgen_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = tpgen_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = tpgen_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = tpgen_clear_interrupt_pending;
	nxs_dev->open = tpgen_open;
	nxs_dev->close = tpgen_close;
	nxs_dev->start = tpgen_start;
	nxs_dev->stop = tpgen_stop;
	nxs_dev->set_dirty = tpgen_set_dirty;
	nxs_dev->set_tid = tpgen_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_FORMAT;
	nxs_dev->dev_services[0].set_control = tpgen_set_format;
	nxs_dev->dev_services[0].get_control = tpgen_get_format;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, tpgen);

	return 0;
}

static int nxs_tpgen_remove(struct platform_device *pdev)
{
	struct nxs_tpgen *tpgen = platform_get_drvdata(pdev);

	if (tpgen)
		unregister_nxs_dev(&tpgen->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_tpgen_match[] = {
	{ .compatible = "nexell,tpgen-nxs-1.0", },
	{},
};

static struct platform_driver nxs_tpgen_driver = {
	.probe	= nxs_tpgen_probe,
	.remove	= nxs_tpgen_remove,
	.driver	= {
		.name = "nxs-tpgen",
		.of_match_table = of_match_ptr(nxs_tpgen_match),
	},
};

static int __init nxs_tpgen_init(void)
{
	return platform_driver_register(&nxs_tpgen_driver);
}
/* subsys_initcall(nxs_tpgen_init); */
fs_initcall(nxs_tpgen_init);

static void __exit nxs_tpgen_exit(void)
{
	platform_driver_unregister(&nxs_tpgen_driver);
}
module_exit(nxs_tpgen_exit);

MODULE_DESCRIPTION("Nexell Stream TPGEN driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

