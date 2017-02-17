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

#define GAMMA_DIRTYSET_OFFSET	0x0004
#define GAMMA_DIRTYCLR_OFFSET	0x0014
#define GAMMA0_DIRTY		BIT(10)
#define GAMMA1_DIRTY		BIT(11)

#define GAMMA_CTRL		0x0000
#define GAMMA_INT		0x0004
#define GAMMA_COEFF0		0x0008
#define GAMMA_COEFF1		0x000c
#define GAMMA_COEFF2		0x0010
#define GAMMA_COEFF3		0x0014

/* GAMMA CTRL */
#define GAMMA_REG_CLEAR_SHIFT	19
#define GAMMA_REG_CLEAR_MASK	BIT(19)
#define GAMMA_TZPROT_SHIFT	18
#define GAMMA_TZPROT_MASK	BIT(18)
#define GAMMA_IDLE_SHIFT	17
#define GAMMA_IDLE_MASK		BIT(17)
#define GAMMA_ENABLE_SHIFT	16
#define GAMMA_ENABLE_MASK	BIT(16)
#define GAMMA_TID_SHIFT		0
#define GAMMA_TID_MASK		GENMASK(13, 0)

/* GAMMA INT */
#define GAMMA_IDLE_INT_DISABLE_SHIFT	6
#define GAMMA_IDLE_INT_DISABLE_MASK	BIT(6)
#define GAMMA_IDLE_INTENB_SHIFT		5
#define GAMMA_IDLE_INTENB_MASK		BIT(5)
#define GAMMA_IDLE_INTPEND_CLR_SHIFT	4
#define GAMMA_IDLE_INTPEND_CLR_MASK	BIT(4)
#define GAMMA_UPDATE_INT_DISABLE_SHIFT	2
#define GAMMA_UPDATE_INT_DISABLE_MASK	BIT(2)
#define GAMMA_UPDATE_INTENB_SHIFT	1
#define GAMMA_UPDATE_INTENB_MASK	BIT(1)
#define GAMMA_UPDATE_INTPEND_CLR_SHIFT	0
#define GAMMA_UPDATE_INTPEND_CLR_MASK	BIT(0)

struct nxs_gamma {
	struct nxs_dev nxs_dev;
	struct regmap *reg;
	u32 offset;
};

#define nxs_to_gamma(dev)	container_of(dev, struct nxs_gamma, nxs_dev)

static void gamma_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 gamma_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 gamma_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void gamma_clear_interrupt_pending(const struct nxs_dev *pthis, int type)
{
}

static int gamma_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int gamma_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int gamma_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int gamma_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int gamma_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_gamma *gamma = nxs_to_gamma(pthis);
	u32 dirty_val;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	switch (pthis->dev_inst_index) {
	case 0:
		dirty_val = GAMMA0_DIRTY;
		break;
	case 1:
		dirty_val = GAMMA1_DIRTY;
		break;
	default:
		dev_err(pthis->dev, "invalid inst %d\n", pthis->dev_inst_index);
		return -ENODEV;
	}

	return regmap_write(gamma->reg, GAMMA_DIRTYSET_OFFSET, dirty_val);
}

static int gamma_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_gamma *gamma = nxs_to_gamma(pthis);

	return regmap_update_bits(gamma->reg, gamma->offset + GAMMA_CTRL,
				  GAMMA_TID_MASK,
				  tid1 << GAMMA_TID_SHIFT);
}

static int gamma_set_gamma(const struct nxs_dev *pthis,
			   const struct nxs_control *pparam)
{
	return 0;
}

static int gamma_get_gamma(const struct nxs_dev *pthis,
			   struct nxs_control *pparam)
{
	return 0;
}

static int nxs_gamma_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_gamma *gamma;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	gamma = devm_kzalloc(&pdev->dev, sizeof(*gamma), GFP_KERNEL);
	if (!gamma)
		return -ENOMEM;

	nxs_dev = &gamma->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	gamma->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						     "syscon");
	if (IS_ERR(gamma->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(gamma->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	gamma->offset = res->start;

	nxs_dev->set_interrupt_enable = gamma_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = gamma_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = gamma_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = gamma_clear_interrupt_pending;
	nxs_dev->open = gamma_open;
	nxs_dev->close = gamma_close;
	nxs_dev->start = gamma_start;
	nxs_dev->stop = gamma_stop;
	nxs_dev->set_dirty = gamma_set_dirty;
	nxs_dev->set_tid = gamma_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_GAMMA;
	nxs_dev->dev_services[0].set_control = gamma_set_gamma;
	nxs_dev->dev_services[0].get_control = gamma_get_gamma;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, gamma);

	return 0;
}

static int nxs_gamma_remove(struct platform_device *pdev)
{
	struct nxs_gamma *gamma = platform_get_drvdata(pdev);

	if (gamma)
		unregister_nxs_dev(&gamma->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_gamma_match[] = {
	{ .compatible = "nexell,gamma-nxs-1.0", },
	{},
};

static struct platform_driver nxs_gamma_driver = {
	.probe	= nxs_gamma_probe,
	.remove	= nxs_gamma_remove,
	.driver	= {
		.name = "nxs-gamma",
		.of_match_table = of_match_ptr(nxs_gamma_match),
	},
};

static int __init nxs_gamma_init(void)
{
	return platform_driver_register(&nxs_gamma_driver);
}
/* subsys_initcall(nxs_gamma_init); */
fs_initcall(nxs_gamma_init);

static void __exit nxs_gamma_exit(void)
{
	platform_driver_unregister(&nxs_gamma_driver);
}
module_exit(nxs_gamma_exit);

MODULE_DESCRIPTION("Nexell Stream GAMMA driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

