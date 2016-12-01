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

struct nxs_scaler {
	struct nxs_dev nxs_dev;
	u32 line_buffer_size;
};

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

static int scaler_set_dirty(const struct nxs_dev *pthis)
{
	return 0;
}

static int scaler_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	return 0;
}

static int scaler_set_syncinfo(const struct nxs_dev *pthis,
			    const union nxs_control *pparam)
{
	return 0;
}

static int scaler_get_syncinfo(const struct nxs_dev *pthis,
			    union nxs_control *pparam)
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
	nxs_dev->dev_services[0].type = NXS_CONTROL_SYNCINFO;
	nxs_dev->dev_services[0].set_control = scaler_set_syncinfo;
	nxs_dev->dev_services[0].get_control = scaler_get_syncinfo;

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

