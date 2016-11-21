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
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_res_manager.h>

#define SIMULATE_INTERRUPT

struct nxs_dmaw {
#ifdef SIMULATE_INTERRUPT
	struct timer_list timer;
#endif

	struct nxs_dev nxs_dev;
};

#ifdef SIMULATE_INTERRUPT
#include <linux/timer.h>
#define INT_TIMEOUT_MS		30

static void int_timer_func(unsigned long priv)
{
	struct nxs_dmaw *dmaw = (struct nxs_dmaw *)priv;
	struct nxs_dev *nxs_dev = &dmaw->nxs_dev;

	if (nxs_dev->irq_callback)
		nxs_dev->irq_callback->handler(nxs_dev,
					       nxs_dev->irq_callback->data);

	mod_timer(&dmaw->timer, jiffies + msecs_to_jiffies(INT_TIMEOUT_MS));
}
#endif

#define nxs_dev_to_dmaw(dev)	container_of(dev, struct nxs_dmaw, nxs_dev)

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

static int dmaw_start(const struct nxs_dev *pthis)
{
#ifdef SIMULATE_INTERRUPT
	struct nxs_dmaw *dmaw = nxs_dev_to_dmaw(pthis);

	mod_timer(&dmaw->timer, jiffies + msecs_to_jiffies(INT_TIMEOUT_MS));
#endif

	return 0;
}

static int dmaw_stop(const struct nxs_dev *pthis)
{
#ifdef SIMULATE_INTERRUPT
	struct nxs_dmaw *dmaw = nxs_dev_to_dmaw(pthis);

	del_timer(&dmaw->timer);
#endif

	return 0;
}

static int dmaw_set_syncinfo(const struct nxs_dev *pthis,
			    const union nxs_control *pparam)
{
	return 0;
}

static int dmaw_get_syncinfo(const struct nxs_dev *pthis,
			    union nxs_control *pparam)
{
	return 0;
}

static int nxs_dmaw_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_dmaw *dmaw;

	dmaw = devm_kzalloc(&pdev->dev, sizeof(*dmaw), GFP_KERNEL);
	if (!dmaw)
		return -ENOMEM;

	ret = nxs_dev_parse_dt(pdev, &dmaw->nxs_dev);
	if (ret)
		return ret;

	dmaw->nxs_dev.set_interrupt_enable = dmaw_set_interrupt_enable;
	dmaw->nxs_dev.get_interrupt_enable = dmaw_get_interrupt_enable;
	dmaw->nxs_dev.get_interrupt_pending = dmaw_get_interrupt_pending;
	dmaw->nxs_dev.clear_interrupt_pending = dmaw_clear_interrupt_pending;
	dmaw->nxs_dev.open = dmaw_open;
	dmaw->nxs_dev.close = dmaw_close;
	dmaw->nxs_dev.start = dmaw_start;
	dmaw->nxs_dev.stop = dmaw_stop;
	dmaw->nxs_dev.set_control = nxs_set_control;
	dmaw->nxs_dev.get_control = nxs_get_control;
	dmaw->nxs_dev.dev_services[0].type = NXS_CONTROL_SYNCINFO;
	dmaw->nxs_dev.dev_services[0].set_control = dmaw_set_syncinfo;
	dmaw->nxs_dev.dev_services[0].get_control = dmaw_get_syncinfo;

	dmaw->nxs_dev.dev = &pdev->dev;

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
