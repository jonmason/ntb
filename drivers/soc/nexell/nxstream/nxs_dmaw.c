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

#define DMAW_DIRTYSET_OFFSET	0x0008
#define DMAW_DIRTYCLR_OFFSET	0x0018
#define DMAW0_DIRTY		BIT(4)
#define DMAW1_DIRTY		BIT(5)
#define DMAW2_DIRTY		BIT(6)
#define DMAW3_DIRTY		BIT(7)
#define DMAW4_DIRTY		BIT(8)
#define DMAW5_DIRTY		BIT(9)
#define DMAW6_DIRTY		BIT(10)
#define DMAW7_DIRTY		BIT(11)
#define DMAW8_DIRTY		BIT(12)
#define DMAW9_DIRTY		BIT(13)
#define DMAW10_DIRTY		BIT(14)
#define DMAW11_DIRTY		BIT(15)

#define SIMULATE_INTERRUPT

struct nxs_dmaw {
#ifdef SIMULATE_INTERRUPT
	struct timer_list timer;
#endif
	struct nxs_dev nxs_dev;
	struct regmap *reg;
	u32 offset;
};

#ifdef SIMULATE_INTERRUPT
#include <linux/timer.h>
#define INT_TIMEOUT_MS		30

static void int_timer_func(unsigned long priv)
{
	struct nxs_dmaw *dmaw = (struct nxs_dmaw *)priv;
	struct nxs_dev *nxs_dev = &dmaw->nxs_dev;
	struct nxs_irq_callback *callback;
	unsigned long flags;

	spin_lock_irqsave(&nxs_dev->irq_lock, flags);
	list_for_each_entry(callback, &nxs_dev->irq_callback, list)
		callback->handler(nxs_dev, callback->data);
	spin_unlock_irqrestore(&nxs_dev->irq_lock, flags);

	mod_timer(&dmaw->timer, jiffies + msecs_to_jiffies(INT_TIMEOUT_MS));
}
#endif

#define nxs_to_dmaw(dev)	container_of(dev, struct nxs_dmaw, nxs_dev)

static void dmaw_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 dmaw_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 dmaw_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void dmaw_clear_interrupt_pending(const struct nxs_dev *pthis, int type)
{
}

static int dmaw_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int dmaw_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int dmaw_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);
	u32 dirty_val;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	switch (pthis->dev_inst_index) {
	case 0:
		dirty_val = DMAW0_DIRTY;
		break;
	case 1:
		dirty_val = DMAW1_DIRTY;
		break;
	case 2:
		dirty_val = DMAW2_DIRTY;
		break;
	case 3:
		dirty_val = DMAW3_DIRTY;
		break;
	case 4:
		dirty_val = DMAW4_DIRTY;
		break;
	case 5:
		dirty_val = DMAW5_DIRTY;
		break;
	case 6:
		dirty_val = DMAW6_DIRTY;
		break;
	case 7:
		dirty_val = DMAW7_DIRTY;
		break;
	case 8:
		dirty_val = DMAW8_DIRTY;
		break;
	case 9:
		dirty_val = DMAW9_DIRTY;
		break;
	case 10:
		dirty_val = DMAW10_DIRTY;
		break;
	case 11:
		dirty_val = DMAW11_DIRTY;
		break;
	default:
		dev_err(pthis->dev, "invalid inst %d\n", pthis->dev_inst_index);
		return -ENODEV;
	}

	return regmap_write(dmaw->reg, DMAW_DIRTYSET_OFFSET, dirty_val);
}

static int dmaw_start(const struct nxs_dev *pthis)
{
#ifdef SIMULATE_INTERRUPT
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);

	mod_timer(&dmaw->timer, jiffies + msecs_to_jiffies(INT_TIMEOUT_MS));
#endif

	return 0;
}

static int dmaw_stop(const struct nxs_dev *pthis)
{
	if (list_empty(&pthis->irq_callback)) {
#ifdef SIMULATE_INTERRUPT
		struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);

		del_timer(&dmaw->timer);
#endif
	}

	return 0;
}

static int dmaw_set_format(const struct nxs_dev *pthis,
			   const struct nxs_control *pparam)
{
	return 0;
}

static int dmaw_get_format(const struct nxs_dev *pthis,
			   struct nxs_control *pparam)
{
	return 0;
}

static int dmaw_set_buffer(const struct nxs_dev *pthis,
			   const struct nxs_control *pparam)
{
	return 0;
}

static int dmaw_get_buffer(const struct nxs_dev *pthis,
			   struct nxs_control *pparam)
{
	return 0;
}

static int nxs_dmaw_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_dmaw *dmaw;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	dmaw = devm_kzalloc(&pdev->dev, sizeof(*dmaw), GFP_KERNEL);
	if (!dmaw)
		return -ENOMEM;

	nxs_dev = &dmaw->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	dmaw->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						    "syscon");
	if (IS_ERR(dmaw->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(dmaw->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	dmaw->offset = res->start;

	nxs_dev->set_interrupt_enable = dmaw_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = dmaw_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = dmaw_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = dmaw_clear_interrupt_pending;
	nxs_dev->open = dmaw_open;
	nxs_dev->close = dmaw_close;
	nxs_dev->start = dmaw_start;
	nxs_dev->stop = dmaw_stop;
	nxs_dev->set_dirty = dmaw_set_dirty;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_FORMAT;
	nxs_dev->dev_services[0].set_control = dmaw_set_format;
	nxs_dev->dev_services[0].get_control = dmaw_get_format;
	nxs_dev->dev_services[1].type = NXS_CONTROL_BUFFER;
	nxs_dev->dev_services[1].set_control = dmaw_set_buffer;
	nxs_dev->dev_services[1].get_control = dmaw_get_buffer;

	nxs_dev->dev = &pdev->dev;

#ifdef SIMULATE_INTERRUPT
	setup_timer(&dmaw->timer, int_timer_func, (long)dmaw);
#endif

	ret = register_nxs_dev(&dmaw->nxs_dev);
	if (ret)
		return ret;

	dev_info(dmaw->nxs_dev.dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, dmaw);

	return 0;
}

static int nxs_dmaw_remove(struct platform_device *pdev)
{
	struct nxs_dmaw *dmaw = platform_get_drvdata(pdev);

	if (dmaw)
		unregister_nxs_dev(&dmaw->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_dmaw_match[] = {
	{ .compatible = "nexell,dmaw-nxs-1.0", },
	{},
};

static struct platform_driver nxs_dmaw_driver = {
	.probe	= nxs_dmaw_probe,
	.remove	= nxs_dmaw_remove,
	.driver	= {
		.name = "nxs-dmaw",
		.of_match_table = of_match_ptr(nxs_dmaw_match),
	},
};

static int __init nxs_dmaw_init(void)
{
	return platform_driver_register(&nxs_dmaw_driver);
}
/* subsys_initcall(nxs_dmaw_init); */
fs_initcall(nxs_dmaw_init);

static void __exit nxs_dmaw_exit(void)
{
	platform_driver_unregister(&nxs_dmaw_driver);
}
module_exit(nxs_dmaw_exit);

MODULE_DESCRIPTION("Nexell Stream DMAW driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");
