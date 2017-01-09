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

/* NXS2DSI */
#define DSI_DPC_CTRL0		0x0020

/* DSI DPC CTRL0 */
#define DIRTYFLAG_TOP_SHIFT	12
#define DIRTYFLAG_TOP_MASK	BIT(12)

struct nxs_dsi {
	struct nxs_dev nxs_dev;
	u8 *nxs2dsi_base;
	u8 *nxs2dsi0_i80_base;
	u8 *nxs2dsi1_i80_base;
	u8 *base;
};

#define nxs_to_dsi(dev)		container_of(dev, struct nxs_dsi, nxs_dev)

static void mipi_dsi_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 mipi_dsi_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 mipi_dsi_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void mipi_dsi_clear_interrupt_pending(const struct nxs_dev *pthis,
					     int type)
{
}

static int mipi_dsi_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int mipi_dsi_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int mipi_dsi_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int mipi_dsi_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int mipi_dsi_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_dsi *dsi = nxs_to_dsi(pthis);
	u32 val;
	u8 *reg;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	reg = dsi->nxs2dsi_base + DSI_DPC_CTRL0;
	val = readl(reg);
	val |= 1 << DIRTYFLAG_TOP_SHIFT;
	writel(val, reg);

	return 0;
}

static int mipi_dsi_set_syncinfo(const struct nxs_dev *pthis,
				 const struct nxs_control *pparam)
{
	return 0;
}

static int mipi_dsi_get_syncinfo(const struct nxs_dev *pthis,
				 struct nxs_control *pparam)
{
	return 0;
}

static int nxs_mipi_dsi_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_dsi *dsi;
	struct nxs_dev *nxs_dev;
	struct resource res;

	dsi = devm_kzalloc(&pdev->dev, sizeof(*dsi), GFP_KERNEL);
	if (!dsi)
		return -ENOMEM;

	nxs_dev = &dsi->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	/* NXS2DSI base */
	ret = of_address_to_resource(pdev->dev.of_node, 0, &res);
	if (ret) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to get nxs2dsi base address\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -ENXIO;
	}

	dsi->nxs2dsi_base = devm_ioremap_nocache(nxs_dev->dev, res.start,
						 resource_size(&res));
	if (!dsi->nxs2dsi_base) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to ioremap for nxs2dsi\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -EBUSY;
	}

	/* NXS2DSI0_I80 base */
	ret = of_address_to_resource(pdev->dev.of_node, 1, &res);
	if (ret) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to get nxs2dsi0_i80 base address\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -ENXIO;
	}

	dsi->nxs2dsi0_i80_base = devm_ioremap_nocache(nxs_dev->dev, res.start,
						      resource_size(&res));
	if (!dsi->nxs2dsi0_i80_base) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to ioremap for nxs2dsi0_i80\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -EBUSY;
	}

	/* NXS2DSI1_I80 base */
	ret = of_address_to_resource(pdev->dev.of_node, 2, &res);
	if (ret) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to get nxs2dsi1_i80 base address\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -ENXIO;
	}

	dsi->nxs2dsi1_i80_base = devm_ioremap_nocache(nxs_dev->dev, res.start,
						      resource_size(&res));
	if (!dsi->nxs2dsi1_i80_base) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to ioremap for nxs2dsi1_i80\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -EBUSY;
	}

	nxs_dev->set_interrupt_enable = mipi_dsi_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = mipi_dsi_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = mipi_dsi_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = mipi_dsi_clear_interrupt_pending;
	nxs_dev->open = mipi_dsi_open;
	nxs_dev->close = mipi_dsi_close;
	nxs_dev->start = mipi_dsi_start;
	nxs_dev->stop = mipi_dsi_stop;
	nxs_dev->set_dirty = mipi_dsi_set_dirty;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_SYNCINFO;
	nxs_dev->dev_services[0].set_control = mipi_dsi_set_syncinfo;
	nxs_dev->dev_services[0].get_control = mipi_dsi_get_syncinfo;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, dsi);

	return 0;
}

static int nxs_mipi_dsi_remove(struct platform_device *pdev)
{
	struct nxs_dsi *dsi = platform_get_drvdata(pdev);

	if (dsi)
		unregister_nxs_dev(&dsi->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_mipi_dsi_match[] = {
	{ .compatible = "nexell,mipi_dsi-nxs-1.0", },
	{},
};

static struct platform_driver nxs_mipi_dsi_driver = {
	.probe	= nxs_mipi_dsi_probe,
	.remove	= nxs_mipi_dsi_remove,
	.driver	= {
		.name = "nxs-mipi_dsi",
		.of_match_table = of_match_ptr(nxs_mipi_dsi_match),
	},
};

static int __init nxs_mipi_dsi_init(void)
{
	return platform_driver_register(&nxs_mipi_dsi_driver);
}
/* subsys_initcall(nxs_mipi_dsi_init); */
fs_initcall(nxs_mipi_dsi_init);

static void __exit nxs_mipi_dsi_exit(void)
{
	platform_driver_unregister(&nxs_mipi_dsi_driver);
}
module_exit(nxs_mipi_dsi_exit);

MODULE_DESCRIPTION("Nexell Stream MIPI_DSI driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

