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
#include <linux/regmap.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/mfd/syscon.h>
#include <linux/platform_device.h>

#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_res_manager.h>

#define CSI_DIRTYSET_OFFSET	0x0000
#define CSI_DIRTYCLR_OFFSET	0x0008
#define CSI_CH0_DIRTY		BIT(19)
#define CSI_CH1_DIRTY		BIT(20)
#define CSI_CH2_DIRTY		BIT(21)
#define CSI_CH3_DIRTY		BIT(22)

#define CSI2NXS0_CTRL1		0x0040
#define CSI2NXS1_CTRL1		0x0060
#define CSI2NXS2_CTRL1		0x0080
#define CSI2NXS3_CTRL1		0x00a0

/* CSI2NXS CTRL1 */
#define CSI2NXS_TID_SHIFT	0
#define CSI2NXS_TID_MASK	GENMASK(13, 0)

struct nxs_csi {
	struct nxs_dev nxs_dev;
	u8 *base;
	struct regmap *reg_csi2nxs;
	u32 offset_csi2nxs;
	u32 channel; /* TODO: how to set channel */
};

#define nxs_to_csi(dev)		container_of(dev, struct nxs_csi, nxs_dev)

static void mipi_csi_set_interrupt_enable(const struct nxs_dev *pthis, int type,
					  bool enable)
{
}

static u32 mipi_csi_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 mipi_csi_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void mipi_csi_clear_interrupt_pending(const struct nxs_dev *pthis,
					     int type)
{
}

static int mipi_csi_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int mipi_csi_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int mipi_csi_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int mipi_csi_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int mipi_csi_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_csi *csi = nxs_to_csi(pthis);
	u32 dirty_val = 0;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	switch (csi->channel) {
	case 4:
		dirty_val |= CSI_CH3_DIRTY;
	case 3:
		dirty_val |= CSI_CH2_DIRTY;
	case 2:
		dirty_val |= CSI_CH1_DIRTY;
	case 1:
		dirty_val |= CSI_CH0_DIRTY;
		break;
	default:
		return -ENODEV;
	}

	return regmap_write(csi->reg_csi2nxs, CSI_DIRTYSET_OFFSET, dirty_val);
}

static int mipi_csi_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_csi *csi = nxs_to_csi(pthis);

	switch (csi->channel) {
	case 4:
		regmap_update_bits(csi->reg_csi2nxs,
				   csi->offset_csi2nxs + CSI2NXS3_CTRL1,
				   CSI2NXS_TID_MASK, tid1 << CSI2NXS_TID_SHIFT);
	case 3:
		regmap_update_bits(csi->reg_csi2nxs,
				   csi->offset_csi2nxs + CSI2NXS2_CTRL1,
				   CSI2NXS_TID_MASK, tid1 << CSI2NXS_TID_SHIFT);
	case 2:
		regmap_update_bits(csi->reg_csi2nxs,
				   csi->offset_csi2nxs + CSI2NXS1_CTRL1,
				   CSI2NXS_TID_MASK, tid1 << CSI2NXS_TID_SHIFT);
	case 1:
		regmap_update_bits(csi->reg_csi2nxs,
				   csi->offset_csi2nxs + CSI2NXS0_CTRL1,
				   CSI2NXS_TID_MASK, tid1 << CSI2NXS_TID_SHIFT);
		break;
	default:
		return -ENODEV;
	}

	return 0;
}

static int nxs_mipi_csi_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_csi *csi;
	struct nxs_dev *nxs_dev;
	struct resource res;

	csi = devm_kzalloc(&pdev->dev, sizeof(*csi), GFP_KERNEL);
	if (!csi)
		return -ENOMEM;

	nxs_dev = &csi->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	csi->reg_csi2nxs = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
							   "syscon");
	if (IS_ERR(csi->reg_csi2nxs)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(csi->reg_csi2nxs);
	}

	ret = of_address_to_resource(pdev->dev.of_node, 0, &res);
	if (ret) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	csi->offset_csi2nxs = res.start;

	ret = of_address_to_resource(pdev->dev.of_node, 1, &res);
	if (ret) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to get csi base address\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		return -ENXIO;
	}

	csi->base = devm_ioremap_nocache(nxs_dev->dev, res.start,
					 resource_size(&res));
	if (!csi->base) {
		dev_err(&pdev->dev,
			"[%s:%d] failed to ioremap for csi\n",
			nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
			return -EBUSY;
	}

	nxs_dev->set_interrupt_enable = mipi_csi_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = mipi_csi_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = mipi_csi_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = mipi_csi_clear_interrupt_pending;
	nxs_dev->open = mipi_csi_open;
	nxs_dev->close = mipi_csi_close;
	nxs_dev->start = mipi_csi_start;
	nxs_dev->stop = mipi_csi_stop;
	nxs_dev->set_dirty = mipi_csi_set_dirty;
	nxs_dev->set_tid = mipi_csi_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, csi);

	return 0;
}

static int nxs_mipi_csi_remove(struct platform_device *pdev)
{
	struct nxs_csi *csi = platform_get_drvdata(pdev);

	if (csi)
		unregister_nxs_dev(&csi->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_mipi_csi_match[] = {
	{ .compatible = "nexell,mipi_csi-nxs-1.0", },
	{},
};

static struct platform_driver nxs_mipi_csi_driver = {
	.probe	= nxs_mipi_csi_probe,
	.remove	= nxs_mipi_csi_remove,
	.driver	= {
		.name = "nxs-mipi_csi",
		.of_match_table = of_match_ptr(nxs_mipi_csi_match),
	},
};

static int __init nxs_mipi_csi_init(void)
{
	return platform_driver_register(&nxs_mipi_csi_driver);
}
/* subsys_initcall(nxs_mipi_csi_init); */
fs_initcall(nxs_mipi_csi_init);

static void __exit nxs_mipi_csi_exit(void)
{
	platform_driver_unregister(&nxs_mipi_csi_driver);
}
module_exit(nxs_mipi_csi_exit);

MODULE_DESCRIPTION("Nexell Stream MIPI_CSI driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");
