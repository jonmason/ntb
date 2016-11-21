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

static int vip_set_syncinfo(const struct nxs_dev *pthis,
			    const union nxs_control *pparam)
{
	return 0;
}

static int vip_get_syncinfo(const struct nxs_dev *pthis,
			    union nxs_control *pparam)
{
	return 0;
}

static int nxs_vip_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_dev *nxs_dev_clipper;
	struct nxs_dev *nxs_dev_decimator;

	nxs_dev_clipper = devm_kzalloc(&pdev->dev, sizeof(*nxs_dev_clipper),
				       GFP_KERNEL);
	if (!nxs_dev_clipper)
		return -ENOMEM;


	ret = nxs_dev_parse_dt(pdev, nxs_dev_clipper);
	if (ret)
		return ret;

	INIT_LIST_HEAD(&nxs_dev_clipper->list);
	nxs_dev_clipper->set_interrupt_enable = vip_set_interrupt_enable;
	nxs_dev_clipper->get_interrupt_enable = vip_get_interrupt_enable;
	nxs_dev_clipper->get_interrupt_pending = vip_get_interrupt_pending;
	nxs_dev_clipper->clear_interrupt_pending = vip_clear_interrupt_pending;
	nxs_dev_clipper->open = vip_open;
	nxs_dev_clipper->close = vip_close;
	nxs_dev_clipper->start = vip_start;
	nxs_dev_clipper->stop = vip_stop;
	nxs_dev_clipper->set_control = nxs_set_control;
	nxs_dev_clipper->get_control = nxs_get_control;
	nxs_dev_clipper->dev_services[0].type = NXS_CONTROL_SYNCINFO;
	nxs_dev_clipper->dev_services[0].set_control = vip_set_syncinfo;
	nxs_dev_clipper->dev_services[0].get_control = vip_get_syncinfo;

	nxs_dev_clipper->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev_clipper);
	if (ret)
		return ret;

	nxs_dev_decimator = devm_kzalloc(&pdev->dev, sizeof(*nxs_dev_decimator),
					 GFP_KERNEL);
	if (!nxs_dev_decimator) {
		unregister_nxs_dev(nxs_dev_clipper);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&nxs_dev_decimator->list);
	nxs_dev_decimator->dev_ver = nxs_dev_clipper->dev_ver;
	nxs_dev_decimator->dev_function = NXS_FUNCTION_VIP_DECIMATOR;
	nxs_dev_decimator->dev_inst_index = nxs_dev_clipper->dev_inst_index;
	nxs_dev_decimator->base = nxs_dev_clipper->base;
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
	platform_set_drvdata(pdev, nxs_dev_clipper);

	return 0;
}

static int nxs_vip_remove(struct platform_device *pdev)
{
	struct nxs_dev *nxs_dev = platform_get_drvdata(pdev);

	if (nxs_dev) {
		unregister_nxs_dev(nxs_dev);
		if (nxs_dev->sibling)
			unregister_nxs_dev(nxs_dev->sibling);
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
