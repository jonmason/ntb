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

/* VIP Register */
#define VIP_CONFIG		0x0000

/* VIP CONFIG */
#define VIP_DIRTY_SHIFT		31
#define VIP_DIRTY_MASK		BIT(31)
#define VIP_DIRTY_CLR_SHIFT	30
#define VIP_DIRTY_CLR_MASK	BIT(30)
#define VIP_TZPROT_SHIFT	29
#define VIP_TZPROT_MASK		BIT(29)
#define VIP_REG_CLEAR_SHIFT	28
#define VIP_REG_CLEAR_MASK	BIT(28)
#define VIP_MXL_SEL_ENB_SHIFT	12
#define VIP_MXL_SEL_ENB_MASK	BIT(12)
#define VIP_MXL_SEL_SHIFT	11
#define VIP_MXL_SEL_MASK	BIT(11)
#define VIP_MXL_ENB_SHIFT	10
#define VIP_MXL_ENB_MASK	BIT(10)
#define VIP_MXS_ENB_SHIFT	9
#define VIP_MXS_ENB_MASK	BIT(9)
#define VIP_EXTSYNCENB_SHIFT	8
#define VIP_EXTSYNCENB_MASK	BIT(8)
#define VIP_RAW_WIDTH_SHIFT	5
#define VIP_RAW_WIDTH_MASK	GENMASK(6, 5)
#define VIP_RAW_ALIGN_LSB_SHIFT	4
#define VIP_RAW_ALIGN_LSB_MASK	BIT(4)
#define VIP_YUYVSEL_SHIFT	2
#define VIP_YUYVSEL_MASK	GENMASK(3, 2)
#define VIP_WIDTH8_SHIFT	1
#define VIP_WIDTH8_MASK		BIT(1)
#define VIP_ENB_SHIFT		0
#define VIP_ENB_MASK		BIT(0)

/* VIP PRESCALER Register */
#define PRESCALER_CDENB		0x0000
#define PRESCALER_DECIMATOR_TID	0x004c
#define PRESCALER_CLIPPER_TID	0x00a8

/* VIP PRE CDENB */
#define	PRESCALER_REG_CLEAR_SHIFT	31
#define PRESCALER_REG_CLEAR_MASK	BIT(31)
#define PRESCALER_TZPROT_SHIFT		12
#define PRESCALER_TZPROT_MASK		BIT(12)
#define PRESCALER_DIRTY_CLR_SHIFT	11
#define PRESCALER_DIRTY_CLR_MASK	BIT(11)
#define PRESCALER_DIRTY_SHIFT		10
#define PRESCALER_DIRTY_MASK		BIT(10)
#define PRESCALER_REGFIRE_SHIFT		9
#define PRESCALER_REGFIRE_MASK		BIT(9)
#define PRESCALER_TOP_EN_SHIFT		8
#define PRESCALER_TOP_EN_MASK		BIT(8)
#define PRESCALER_SIPENB_SHIFT		5
#define PRESCALER_SIPENB_MASK		BIT(5)
#define PRESCALER_FORMAT_SHIFT		4
#define PRESCALER_FORMAT_MASK		BIT(4)
#define PRESCALER_RAW_SEP_SHIFT		3
#define PRESCALER_RAW_SEP_MASK		BIT(3)
#define PRESCALER_FIELD_INV_SHIFT	2
#define PRESCALER_FIELD_INV_MASK	BIT(2)
#define PRESCALER_CLIPPER_EN_SHIFT	1
#define PRESCALER_CLIPPER_EN_MASK	BIT(1)
#define PRESCALER_DECIMATOR_EN_SHIFT	0
#define PRESCALER_DECIMATOR_EN_MASK	BIT(0)

/* TID */
#define PRESCALER_TID_SHIFT		0
#define PRESCALER_TID_MASK		GENMASK(13, 0)

struct nxs_vip {
	struct nxs_dev nxs_dev_clipper;
	struct nxs_dev nxs_dev_decimator;
	u8 *base; /* vip register */
	u8 *prescaler_base; /* prescaler register */
};

#define clipper_to_vip(dev)	container_of(dev, struct nxs_vip, \
					     nxs_dev_clipper)
#define decimator_to_vip(dev)	container_of(dev, struct nxs_vip, \
					     nxs_dev_decimator)

static void vip_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 vip_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 vip_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void vip_clear_interrupt_pending(const struct nxs_dev *pthis, int type)
{
}

static int vip_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int vip_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int vip_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int vip_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int vip_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_vip *vip;
	u32 val;
	u8 *reg;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	if (pthis->dev_function == NXS_FUNCTION_VIP_CLIPPER)
		vip = clipper_to_vip(pthis);
	else
		vip = decimator_to_vip(pthis);

	/* VIP DIRTY */
	reg = vip->base + VIP_CONFIG;
	val = readl(reg);
	val &= ~VIP_DIRTY_MASK;
	val |= 1 << VIP_DIRTY_SHIFT;
	writel(val, reg);

	/* PRESCALER DIRTY & REG FIRE */
	reg = vip->prescaler_base + PRESCALER_CDENB;
	val = readl(reg);
	val &= ~PRESCALER_DIRTY_MASK;
	val |= 1 << PRESCALER_DIRTY_SHIFT;
	val &= ~PRESCALER_REGFIRE_MASK;
	val |= 1 << PRESCALER_REGFIRE_SHIFT;
	writel(val, reg);

	return 0;
}

static int vip_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_vip *vip;
	u32 val;
	u8 *reg;

	if (pthis->dev_function == NXS_FUNCTION_VIP_CLIPPER) {
		vip = clipper_to_vip(pthis);
		reg = vip->prescaler_base + PRESCALER_CLIPPER_TID;
	} else {
		vip = decimator_to_vip(pthis);
		reg = vip->prescaler_base + PRESCALER_DECIMATOR_TID;
	}

	val = readl(reg);
	val &= ~PRESCALER_TID_MASK;
	val |= tid1 << PRESCALER_TID_SHIFT;
	writel(val, reg);

	return 0;
}

static int vip_set_syncinfo(const struct nxs_dev *pthis,
			    const struct nxs_control *pparam)
{
	return 0;
}

static int vip_get_syncinfo(const struct nxs_dev *pthis,
			    struct nxs_control *pparam)
{
	return 0;
}

static int nxs_vip_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_vip *vip;
	struct nxs_dev *nxs_dev_clipper;
	struct nxs_dev *nxs_dev_decimator;
	struct resource res;

	vip = devm_kzalloc(&pdev->dev, sizeof(*vip), GFP_KERNEL);
	if (!vip)
		return -ENOMEM;

	nxs_dev_clipper = &vip->nxs_dev_clipper;
	ret = nxs_dev_parse_dt(pdev, nxs_dev_clipper);
	if (ret)
		return ret;

	ret = of_address_to_resource(pdev->dev.of_node, 0, &res);
	if (ret) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to get vip base address\n",
			nxs_function_to_str(nxs_dev_clipper->dev_function),
			nxs_dev_clipper->dev_inst_index);
		return -ENXIO;
	}

	vip->base = devm_ioremap_nocache(nxs_dev_clipper->dev, res.start,
					 resource_size(&res));
	if (!vip->base) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to ioremap for vip\n",
			nxs_function_to_str(nxs_dev_clipper->dev_function),
			nxs_dev_clipper->dev_inst_index);
			return -EBUSY;
	}

	ret = of_address_to_resource(pdev->dev.of_node, 1, &res);
	if (ret) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to get vip prescaler base address\n",
			nxs_function_to_str(nxs_dev_clipper->dev_function),
			nxs_dev_clipper->dev_inst_index);
		return -ENXIO;
	}

	vip->prescaler_base = devm_ioremap_nocache(nxs_dev_clipper->dev,
						   res.start,
						   resource_size(&res));
	if (!vip->prescaler_base) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to ioremap for vip prescaler\n",
			nxs_function_to_str(nxs_dev_clipper->dev_function),
			nxs_dev_clipper->dev_inst_index);
			return -EBUSY;
	}

	INIT_LIST_HEAD(&nxs_dev_clipper->list);
	nxs_dev_clipper->set_interrupt_enable = vip_set_interrupt_enable;
	nxs_dev_clipper->get_interrupt_enable = vip_get_interrupt_enable;
	nxs_dev_clipper->get_interrupt_pending = vip_get_interrupt_pending;
	nxs_dev_clipper->clear_interrupt_pending = vip_clear_interrupt_pending;
	nxs_dev_clipper->open = vip_open;
	nxs_dev_clipper->close = vip_close;
	nxs_dev_clipper->start = vip_start;
	nxs_dev_clipper->stop = vip_stop;
	nxs_dev_clipper->set_dirty = vip_set_dirty;
	nxs_dev_clipper->set_tid = vip_set_tid;
	nxs_dev_clipper->set_control = nxs_set_control;
	nxs_dev_clipper->get_control = nxs_get_control;
	nxs_dev_clipper->dev_services[0].type = NXS_CONTROL_SYNCINFO;
	nxs_dev_clipper->dev_services[0].set_control = vip_set_syncinfo;
	nxs_dev_clipper->dev_services[0].get_control = vip_get_syncinfo;

	nxs_dev_clipper->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev_clipper);
	if (ret)
		return ret;

	nxs_dev_decimator = &vip->nxs_dev_decimator;

	INIT_LIST_HEAD(&nxs_dev_decimator->list);
	nxs_dev_decimator->dev_ver = nxs_dev_clipper->dev_ver;
	nxs_dev_decimator->dev_function = NXS_FUNCTION_VIP_DECIMATOR;
	nxs_dev_decimator->dev_inst_index = nxs_dev_clipper->dev_inst_index;
	/* nxs_dev_decimator->base = nxs_dev_clipper->base; */
	nxs_dev_decimator->irq = nxs_dev_clipper->irq;
	nxs_dev_decimator->resets = nxs_dev_clipper->resets;
	nxs_dev_decimator->reset_num = nxs_dev_clipper->reset_num;
	nxs_dev_decimator->max_refcount = nxs_dev_clipper->max_refcount;

	nxs_dev_decimator->set_interrupt_enable = vip_set_interrupt_enable;
	nxs_dev_decimator->get_interrupt_enable = vip_get_interrupt_enable;
	nxs_dev_decimator->get_interrupt_pending = vip_get_interrupt_pending;
	nxs_dev_decimator->clear_interrupt_pending =
		vip_clear_interrupt_pending;
	nxs_dev_decimator->open = vip_open;
	nxs_dev_decimator->close = vip_close;
	nxs_dev_decimator->start = vip_start;
	nxs_dev_decimator->stop = vip_stop;
	nxs_dev_decimator->set_dirty = vip_set_dirty;
	nxs_dev_decimator->set_tid = vip_set_tid;
	nxs_dev_decimator->set_control = nxs_set_control;
	nxs_dev_decimator->get_control = nxs_get_control;
	nxs_dev_decimator->dev_services[0].type = NXS_CONTROL_SYNCINFO;
	nxs_dev_decimator->dev_services[0].set_control = vip_set_syncinfo;
	nxs_dev_decimator->dev_services[0].get_control = vip_get_syncinfo;

	nxs_dev_decimator->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev_decimator);
	if (ret) {
		unregister_nxs_dev(nxs_dev_clipper);
		return ret;
	}

	nxs_dev_clipper->sibling = nxs_dev_decimator;
	nxs_dev_decimator->sibling = nxs_dev_clipper;

	dev_info(&pdev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, vip);

	return 0;
}

static int nxs_vip_remove(struct platform_device *pdev)
{
	struct nxs_vip *vip = platform_get_drvdata(pdev);

	if (vip) {
		unregister_nxs_dev(&vip->nxs_dev_clipper);
		unregister_nxs_dev(&vip->nxs_dev_decimator);
	}

	return 0;
}

static const struct of_device_id nxs_vip_match[] = {
	{ .compatible = "nexell,vip-nxs-1.0", },
	{},
};

static struct platform_driver nxs_vip_driver = {
	.probe	= nxs_vip_probe,
	.remove	= nxs_vip_remove,
	.driver	= {
		.name = "nxs-vip",
		.of_match_table = of_match_ptr(nxs_vip_match),
	},
};

static int __init nxs_vip_init(void)
{
	return platform_driver_register(&nxs_vip_driver);
}
/* subsys_initcall(nxs_vip_init); */
fs_initcall(nxs_vip_init);

static void __exit nxs_vip_exit(void)
{
	platform_driver_unregister(&nxs_vip_driver);
}
module_exit(nxs_vip_exit);

MODULE_DESCRIPTION("Nexell Stream VIP driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");
