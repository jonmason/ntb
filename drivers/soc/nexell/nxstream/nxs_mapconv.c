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

struct nxs_mapconv {
	struct nxs_dev nxs_dev;
};

static void mapconv_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 mapconv_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 mapconv_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void mapconv_clear_interrupt_pending(const struct nxs_dev *pthis,
					    int type)
{
}

static int mapconv_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int mapconv_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int mapconv_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int mapconv_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int mapconv_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	return 0;
}

static int mapconv_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	return 0;
}

static int nxs_mapconv_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_mapconv *mapconv;
	struct nxs_dev *nxs_dev;

	mapconv = devm_kzalloc(&pdev->dev, sizeof(*mapconv), GFP_KERNEL);
	if (!mapconv)
		return -ENOMEM;

	nxs_dev = &mapconv->nxs_dev;

	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	nxs_dev->set_interrupt_enable = mapconv_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = mapconv_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = mapconv_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = mapconv_clear_interrupt_pending;
	nxs_dev->open = mapconv_open;
	nxs_dev->close = mapconv_close;
	nxs_dev->start = mapconv_start;
	nxs_dev->stop = mapconv_stop;
	nxs_dev->set_dirty = mapconv_set_dirty;
	nxs_dev->set_tid = mapconv_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, mapconv);

	return 0;
}

static int nxs_mapconv_remove(struct platform_device *pdev)
{
	struct nxs_mapconv *mapconv = platform_get_drvdata(pdev);

	if (mapconv)
		unregister_nxs_dev(&mapconv->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_mapconv_match[] = {
	{ .compatible = "nexell,mapconv-nxs-1.0", },
	{},
};

static struct platform_driver nxs_mapconv_driver = {
	.probe	= nxs_mapconv_probe,
	.remove	= nxs_mapconv_remove,
	.driver	= {
		.name = "nxs-mapconv",
		.of_match_table = of_match_ptr(nxs_mapconv_match),
	},
};

static int __init nxs_mapconv_init(void)
{
	return platform_driver_register(&nxs_mapconv_driver);
}
/* subsys_initcall(nxs_mapconv_init); */
fs_initcall(nxs_mapconv_init);

static void __exit nxs_mapconv_exit(void)
{
	platform_driver_unregister(&nxs_mapconv_driver);
}
module_exit(nxs_mapconv_exit);

MODULE_DESCRIPTION("Nexell Stream MAPCONV driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

