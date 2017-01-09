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

/* NXS2HDMI */
#define HDMI_DPC_CTRL0		0x0020

/* HDMI DPC CTRL0 */
#define DIRTYFLAG_TOP_SHIFT	12
#define DIRTYFLAG_TOP_MASK	BIT(12)

struct nxs_hdmi {
	struct nxs_dev nxs_dev;
	u8 *nxs2hdmi_base;
	u8 *cec_base;
	u8 *phy_base;
	u8 *link_base;
};

#define nxs_to_hdmi(dev)	container_of(dev, struct nxs_hdmi, nxs_dev)

static void hdmi_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 hdmi_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 hdmi_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void hdmi_clear_interrupt_pending(const struct nxs_dev *pthis, int type)
{
}

static int hdmi_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int hdmi_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int hdmi_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int hdmi_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int hdmi_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_hdmi *hdmi = nxs_to_hdmi(pthis);
	u32 val;
	u8 *reg;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	reg = hdmi->nxs2hdmi_base + HDMI_DPC_CTRL0;
	val = readl(reg);
	val &= ~DIRTYFLAG_TOP_MASK;
	val |= 1 << DIRTYFLAG_TOP_SHIFT;
	writel(val, reg);

	return 0;
}

static int hdmi_set_syncinfo(const struct nxs_dev *pthis,
			     const struct nxs_control *pparam)
{
	return 0;
}

static int hdmi_get_syncinfo(const struct nxs_dev *pthis,
			     struct nxs_control *pparam)
{
	return 0;
}

static int nxs_hdmi_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_hdmi *hdmi;
	struct nxs_dev *nxs_dev;
	struct resource res;

	hdmi = devm_kzalloc(&pdev->dev, sizeof(*hdmi), GFP_KERNEL);
	if (!hdmi)
		return -ENOMEM;

	nxs_dev = &hdmi->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	ret = of_address_to_resource(pdev->dev.of_node, 0, &res);
	if (ret) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to get nxs2hdmi base address\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -ENXIO;
	}

	hdmi->nxs2hdmi_base = devm_ioremap_nocache(nxs_dev->dev, res.start,
						   resource_size(&res));
	if (!hdmi->nxs2hdmi_base) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to ioremap for nxs2hdmi\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
			return -EBUSY;
	}

	ret = of_address_to_resource(pdev->dev.of_node, 1, &res);
	if (ret) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to get cec base address\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -ENXIO;
	}

	hdmi->cec_base = devm_ioremap_nocache(nxs_dev->dev, res.start,
					      resource_size(&res));
	if (!hdmi->cec_base) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to ioremap for cec\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
			return -EBUSY;
	}

	ret = of_address_to_resource(pdev->dev.of_node, 2, &res);
	if (ret) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to get link base address\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -ENXIO;
	}

	hdmi->link_base = devm_ioremap_nocache(nxs_dev->dev, res.start,
					       resource_size(&res));
	if (!hdmi->link_base) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to ioremap for link\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
			return -EBUSY;
	}

	nxs_dev->set_interrupt_enable = hdmi_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = hdmi_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = hdmi_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = hdmi_clear_interrupt_pending;
	nxs_dev->open = hdmi_open;
	nxs_dev->close = hdmi_close;
	nxs_dev->start = hdmi_start;
	nxs_dev->stop = hdmi_stop;
	nxs_dev->set_dirty = hdmi_set_dirty;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_SYNCINFO;
	nxs_dev->dev_services[0].set_control = hdmi_set_syncinfo;
	nxs_dev->dev_services[0].get_control = hdmi_get_syncinfo;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, hdmi);

	return 0;
}

static int nxs_hdmi_remove(struct platform_device *pdev)
{
	struct nxs_hdmi *hdmi = platform_get_drvdata(pdev);

	if (hdmi)
		unregister_nxs_dev(&hdmi->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_hdmi_match[] = {
	{ .compatible = "nexell,hdmi-nxs-1.0", },
	{},
};

static struct platform_driver nxs_hdmi_driver = {
	.probe	= nxs_hdmi_probe,
	.remove	= nxs_hdmi_remove,
	.driver	= {
		.name = "nxs-hdmi",
		.of_match_table = of_match_ptr(nxs_hdmi_match),
	},
};

static int __init nxs_hdmi_init(void)
{
	return platform_driver_register(&nxs_hdmi_driver);
}
/* subsys_initcall(nxs_hdmi_init); */
fs_initcall(nxs_hdmi_init);

static void __exit nxs_hdmi_exit(void)
{
	platform_driver_unregister(&nxs_hdmi_driver);
}
module_exit(nxs_hdmi_exit);

MODULE_DESCRIPTION("Nexell Stream HDMI driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

