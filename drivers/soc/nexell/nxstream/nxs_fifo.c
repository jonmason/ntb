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

static void fifo_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 fifo_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 fifo_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void fifo_clear_interrupt_pending(const struct nxs_dev *pthis, int type)
{
}

static int fifo_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int fifo_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int fifo_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int fifo_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int fifo_set_syncinfo(const struct nxs_dev *pthis,
			    const union nxs_control *pparam)
{
	return 0;
}

static int fifo_get_syncinfo(const struct nxs_dev *pthis,
			    union nxs_control *pparam)
{
	return 0;
}

static int nxs_fifo_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_dev *nxs_dev;

	nxs_dev = devm_kzalloc(&pdev->dev, sizeof(*nxs_dev), GFP_KERNEL);
	if (!nxs_dev)
		return -ENOMEM;


	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	nxs_dev->set_interrupt_enable = fifo_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = fifo_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = fifo_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = fifo_clear_interrupt_pending;
	nxs_dev->open = fifo_open;
	nxs_dev->close = fifo_close;
	nxs_dev->start = fifo_start;
	nxs_dev->stop = fifo_stop;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_SYNCINFO;
	nxs_dev->dev_services[0].set_control = fifo_set_syncinfo;
	nxs_dev->dev_services[0].get_control = fifo_get_syncinfo;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, nxs_dev);

	return 0;
}

static int nxs_fifo_remove(struct platform_device *pdev)
{
	struct nxs_dev *nxs_dev = platform_get_drvdata(pdev);

	if (nxs_dev)
		unregister_nxs_dev(nxs_dev);

	return 0;
}

static const struct of_device_id nxs_fifo_match[] = {
	{ .compatible = "nexell,fifo-nxs-1.0", },
	{},
};

static struct platform_driver nxs_fifo_driver = {
	.probe	= nxs_fifo_probe,
	.remove	= nxs_fifo_remove,
	.driver	= {
		.name = "nxs-fifo",
		.of_match_table = of_match_ptr(nxs_fifo_match),
	},
};

static int __init nxs_fifo_init(void)
{
	return platform_driver_register(&nxs_fifo_driver);
}
/* subsys_initcall(nxs_fifo_init); */
fs_initcall(nxs_fifo_init);

static void __exit nxs_fifo_exit(void)
{
	platform_driver_unregister(&nxs_fifo_driver);
}
module_exit(nxs_fifo_exit);

MODULE_DESCRIPTION("Nexell Stream FIFO driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

