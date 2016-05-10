/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Bon-gyu, KOO <freestyle@nexell.co.kr>
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

#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/delay.h>

#define NX_ID_TO_BANK(id)	((id >> 0x5) & 0x07)       /* Divide 32 */
#define NX_ID_TO_BIT(id)	(id & 0x1F)


struct nexell_reset_data {
	spinlock_t			lock;
	void __iomem			*base;	/* mapped address */
	struct reset_controller_dev	rcdev;
};


static int nexell_reset_assert(struct reset_controller_dev *rcdev,
			       unsigned long id)
{
	struct nexell_reset_data *data = container_of(rcdev,
						      struct nexell_reset_data,
						      rcdev);
	int bank = NX_ID_TO_BANK(id);
	int offset = NX_ID_TO_BIT(id);
	unsigned long flags;
	u32 reg;

	pr_debug("%s: id=%ld [0x%p]\n", __func__, id, data->base + (bank * 4));

	spin_lock_irqsave(&data->lock, flags);

	reg = readl(data->base + (bank * 4));
	writel(reg & ~BIT(offset), data->base + (bank * 4));
	spin_unlock_irqrestore(&data->lock, flags);

	return 0;
}

static int nexell_reset_deassert(struct reset_controller_dev *rcdev,
				 unsigned long id)
{
	struct nexell_reset_data *data = container_of(rcdev,
						      struct nexell_reset_data,
						      rcdev);
	int bank = NX_ID_TO_BANK(id);
	int offset = NX_ID_TO_BIT(id);
	unsigned long flags;
	u32 reg;

	pr_debug("%s: id=%ld [0x%p]\n", __func__, id, data->base + (bank * 4));

	spin_lock_irqsave(&data->lock, flags);

	reg = readl(data->base + (bank * 4));
	writel(reg | BIT(offset), data->base + (bank * 4));

	spin_unlock_irqrestore(&data->lock, flags);

	return 0;
}

static int nexell_reset_reset(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	nexell_reset_assert(rcdev, id);

	mdelay(1);

	nexell_reset_deassert(rcdev, id);

	return 0;
}

static int nexell_reset_status(struct reset_controller_dev *rcdev,
			       unsigned long id)
{
	struct nexell_reset_data *data = container_of(rcdev,
						      struct nexell_reset_data,
						      rcdev);
	int bank = NX_ID_TO_BANK(id);
	int offset = NX_ID_TO_BIT(id);
	unsigned long flags;
	u32 reg;
	int rst;

	spin_lock_irqsave(&data->lock, flags);

	reg = readl(data->base + (bank * 4));

	spin_unlock_irqrestore(&data->lock, flags);

	rst = !(reg & BIT(offset));
	pr_debug("%s: id=%ld [0x%p] val: %d\n", __func__,
		id, data->base + (bank * 4), rst);

	return rst;
}


static struct reset_control_ops nexell_reset_ops = {
	.reset		= nexell_reset_reset,
	.assert		= nexell_reset_assert,
	.deassert	= nexell_reset_deassert,
	.status		= nexell_reset_status,
};


static const struct of_device_id nexell_reset_dt_ids[] = {
	{ .compatible = "nexell,s5pxx18-reset", },
	{ },
};
MODULE_DEVICE_TABLE(of, nexell_reset_dt_ids);

static int nexell_reset_probe(struct platform_device *pdev)
{
	struct nexell_reset_data *data;
	struct resource *res;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	data->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(data->base))
		return PTR_ERR(data->base);

	spin_lock_init(&data->lock);

	data->rcdev.owner = THIS_MODULE;
	data->rcdev.nr_resets = resource_size(res) * 32;
	data->rcdev.ops = &nexell_reset_ops;
	data->rcdev.of_node = pdev->dev.of_node;
	dev_info(&pdev->dev, "nexell reset: nr_resets [%d], base [%p]\n",
		 data->rcdev.nr_resets, data->base);

	return reset_controller_register(&data->rcdev);
}

static int nexell_reset_remove(struct platform_device *pdev)
{
	struct nexell_reset_data *data = platform_get_drvdata(pdev);

	reset_controller_unregister(&data->rcdev);

	return 0;
}

static struct platform_driver nexell_reset_driver = {
	.probe	= nexell_reset_probe,
	.remove	= nexell_reset_remove,
	.driver = {
		.name		= "nexell-reset",
		.owner		= THIS_MODULE,
		.of_match_table	= nexell_reset_dt_ids,
	},
};

static int __init nexell_reset_init(void)
{
	return platform_driver_register(&nexell_reset_driver);
}
pure_initcall(nexell_reset_init);

MODULE_AUTHOR("Bon-gyu, KOO <freestyle@nexell.co.kr");
MODULE_DESCRIPTION("Reset Controller Driver for Nexell");
MODULE_LICENSE("GPL");
