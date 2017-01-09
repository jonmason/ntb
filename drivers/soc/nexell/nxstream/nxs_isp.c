/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Hyejung, Kwon <cjscld15@nexell.co.kr>
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
#include <linux/regmap.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_res_manager.h>

struct nxs_isp {
	struct nxs_dev nxs_dev;
	u8 *base;
	u8 *core_base;
};

#define nxs_to_isp(dev)		container_of(dev, struct nxs_isp, nxs_dev)

static void isp_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
	dev_info(pthis->dev, "[%s]\n", __func__);
}

static u32 isp_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	dev_info(pthis->dev, "[%s]\n", __func__);
	return 0;
}

static u32 isp_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	dev_info(pthis->dev, "[%s]\n", __func__);
	return 0;
}

static void isp_clear_interrupt_pending(const struct nxs_dev *pthis,
					     int type)
{
	dev_info(pthis->dev, "[%s]\n", __func__);
}

static int isp_open(const struct nxs_dev *pthis)
{
	dev_info(pthis->dev, "[%s]\n", __func__);
	return 0;
}

static int isp_close(const struct nxs_dev *pthis)
{
	dev_info(pthis->dev, "[%s]\n", __func__);
	return 0;
}

static int isp_start(const struct nxs_dev *pthis)
{
	dev_info(pthis->dev, "[%s]\n", __func__);
	return 0;
}

static int isp_stop(const struct nxs_dev *pthis)
{
	dev_info(pthis->dev, "[%s]\n", __func__);
	return 0;
}

static int isp_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_isp *isp = nxs_to_isp(pthis);
	u32 dirty_val;

	dev_info(pthis->dev, "[%s]\n", __func__);

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	return 0;
}

static int isp_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_isp *isp = nxs_to_isp(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	return 0;
}

/* check about whether ordering the pixel data feature is needed or not */
/* in the nxs_control_format structure					*/
/* ordering YUYV ,,, YVYU....						*/
static int isp_set_format(const struct nxs_dev *pthis,
			    const struct nxs_control *pparam)
{
	dev_info(pthis->dev, "[%s]\n", __func__);
	return 0;
}

static int isp_get_format(const struct nxs_dev *pthis,
			    struct nxs_control *pparam)
{
	dev_info(pthis->dev, "[%s]\n", __func__);
	return 0;
}

static int isp_set_crop(const struct nxs_dev *pthis,
			    const struct nxs_control *pparam)
{
	dev_info(pthis->dev, "[%s]\n", __func__);
	return 0;
}

static int isp_get_crop(const struct nxs_dev *pthis,
			    struct nxs_control *pparam)
{
	dev_info(pthis->dev, "[%s]\n", __func__);
	return 0;
}

static int nxs_isp_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_isp *isp;
	struct nxs_dev *nxs_dev;
	struct resource res;

	dev_info(&pdev->dev, "[%s] \n", __func__);
	isp = devm_kzalloc(&pdev->dev, sizeof(*isp), GFP_KERNEL);
	if (!isp)
		return -ENOMEM;

	nxs_dev = &isp->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	ret = of_address_to_resource(pdev->dev.of_node, 0, &res);
	if (ret) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to get isp base address \n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -ENXIO;
	}
	isp->base = devm_ioremap_nocache(nxs_dev->dev, res.start,
					 resource_size(&res));
	if (!isp->base) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to ioremap for isp \n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -EBUSY;
	}
	dev_info(&pdev->dev,
		 "[%s] base address = 0x%x \n",
		 nxs_function_to_str(nxs_dev->dev_function),
		 isp->base);
	ret = of_address_to_resource(pdev->dev.of_node, 1, &res);
	if (ret) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to get isp core base address \n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -ENXIO;
	}
	isp->core_base = devm_ioremap_nocache(nxs_dev->dev, res.start,
					 resource_size(&res));
	if (!isp->core_base) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to ioremap for isp \n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -EBUSY;
	}
	dev_info(&pdev->dev,
		 "[%s] core base address = 0x%x \n",
		 nxs_function_to_str(nxs_dev->dev_function),
		 isp->core_base);
	nxs_dev->set_interrupt_enable = isp_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = isp_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = isp_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = isp_clear_interrupt_pending;
	nxs_dev->open = isp_open;
	nxs_dev->close = isp_close;
	nxs_dev->start = isp_start;
	nxs_dev->stop = isp_stop;
	nxs_dev->set_dirty = isp_set_dirty;
	nxs_dev->set_tid = isp_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_FORMAT;
	nxs_dev->dev_services[0].set_control = isp_set_format;
	nxs_dev->dev_services[0].get_control = isp_get_format;
	nxs_dev->dev_services[1].type = NXS_CONTROL_CROP;
	nxs_dev->dev_services[1].set_control = isp_set_crop;
	nxs_dev->dev_services[1].get_control = isp_get_crop;
	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, isp);

	return 0;
}

static int nxs_isp_remove(struct platform_device *pdev)
{
	struct nxs_isp *isp = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "[%s] \n", __func__);
	if (isp)
		unregister_nxs_dev(&isp->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_isp_match[] = {
	{ .compatible = "nexell,isp-nxs-1.0", },
	{},
};

static struct platform_driver nxs_isp_driver = {
	.probe	= nxs_isp_probe,
	.remove	= nxs_isp_remove,
	.driver	= {
		.name = "nxs-isp",
		.of_match_table = of_match_ptr(nxs_isp_match),
	},
};

static int __init nxs_isp_init(void)
{
	return platform_driver_register(&nxs_isp_driver);
}
/* subsys_initcall(nxs_isp_init); */
fs_initcall(nxs_isp_init);

static void __exit nxs_isp_exit(void)
{
	platform_driver_unregister(&nxs_isp_driver);
}
module_exit(nxs_isp_exit);

MODULE_DESCRIPTION("Nexell Stream isp driver");
MODULE_AUTHOR("Hyejung kwon, <cjscld15@nexell.co.kr>");
MODULE_LICENSE("GPL");

