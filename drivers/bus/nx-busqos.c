/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Bon-gyu, KOO <freestyle@nexell.co.kr>
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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/suspend.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/pm_qos.h>

#include <soc/nexell/tieoff.h>

/* DREX Registers */
#define BRBRSVCONTROL		0x100
#define BRBRSVCONFIG		0x104

#define QOSCONTROL0		0x60
#define QOSCONTROL1		0x68

#define QOSCONTROL0_DEFAULT	0x100
#define QOSCONTROL1_DEFAULT	0xFFF

/* TIEOFF Register */
#define TIEOFFREG24		24

struct nx_busqos {
	void __iomem *drex_base;
	u32 drex_s0_qos_enable;
	u32 drex_s1_qos_enable;
	u32 drex_s0_qos_thres;
	u32 drex_s1_qos_thres;
	u32 drex_s0_qos_tout;
	u32 drex_s1_qos_tout;

	struct device *dev;
	int busqos_class;
};


static int nx_busqos_parse_dt(struct device *dev,
			       struct nx_busqos *nx_busqos)
{
	struct device_node *np = dev->of_node;
	u32 s0_enb, s1_enb, s0_thr, s1_thr, s0_tout, s1_tout;

	if (of_property_read_u32(np, "drex_s0_qos_enable", &s0_enb)) {
		dev_err(dev, "failed to get s0 qos enable\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "drex_s1_qos_enable", &s1_enb)) {
		dev_err(dev, "failed to get s0 qos enable\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "drex_s0_qos_thres", &s0_thr)) {
		dev_err(dev, "failed to get s0 qos threshold\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "drex_s1_qos_thres", &s1_thr)) {
		dev_err(dev, "failed to get s1 qos threshold\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "drex_s0_qos_tout", &s0_tout)) {
		dev_err(dev, "failed to get s0 qos timeout\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "drex_s1_qos_tout", &s1_tout)) {
		dev_err(dev, "failed to get s1 qos timeout\n");
		return -EINVAL;
	}


	/* value check */
	if (s0_enb != 0 && s0_enb != 1) {
		dev_err(dev, "wrong drex s0 qos enable\n");
		return -EINVAL;
	}

	if (s1_enb != 0 && s1_enb != 1) {
		dev_err(dev, "wrong drex s1 qos enable\n");
		return -EINVAL;
	}

	if (!(s0_thr > 0 && s0_thr <= 0xf)) {
		dev_err(dev, "wrong drex s0 qos threshold\n");
		return -EINVAL;
	}

	if (!(s1_thr > 0 && s1_thr <= 0xf)) {
		dev_err(dev, "wrong drex s1 qos threshold\n");
		return -EINVAL;
	}

	if (!(s0_tout >= 0 && s0_tout <= 0xfff)) {
		dev_err(dev, "wrong drex s0 qos timeout\n");
		return -EINVAL;
	}

	if (!(s1_tout >= 0 && s1_tout <= 0xfff)) {
		dev_err(dev, "wrong drex s1 qos timeout\n");
		return -EINVAL;
	}


	nx_busqos->drex_s0_qos_enable = s0_enb;
	nx_busqos->drex_s1_qos_enable = s1_enb;
	nx_busqos->drex_s0_qos_thres = s0_thr;
	nx_busqos->drex_s1_qos_thres = s1_thr;
	nx_busqos->drex_s0_qos_tout = s0_tout;
	nx_busqos->drex_s1_qos_tout = s1_tout;

	dev_dbg(dev, "DREX QoS s0 enb:0x%x, thres:0x%x, tout:0x%x\n",
	       s0_enb, s0_thr, s0_tout);
	dev_dbg(dev, "DREX QoS s1 enb:0x%x, thres:0x%x, tout:0x%x\n",
	       s1_enb, s1_thr, s1_tout);

	return 0;
}

static void busqos_setup(struct nx_busqos *nx_busqos)
{
	u32 drex_brb_config = 0, drex_brb_control = 0;
	u32 drex_qos_bits = 0;
	u32 drex_s0_qos_tout = QOSCONTROL0_DEFAULT;
	u32 drex_s1_qos_tout = QOSCONTROL1_DEFAULT;

	/* step 1. config drex brbrsvconfig */
	drex_brb_config = (0xFF00FF00
		| ((nx_busqos->drex_s1_qos_thres & 0xf) << 20)
		| ((nx_busqos->drex_s0_qos_thres & 0xf) << 16)
		| ((nx_busqos->drex_s1_qos_thres & 0xf) << 4)
		| ((nx_busqos->drex_s0_qos_thres & 0xf) << 0));

	writel(drex_brb_config, nx_busqos->drex_base + BRBRSVCONFIG);

	/* step 2. config drex brbrsvcontrol */
	drex_brb_control = 0x00000033;
	writel(drex_brb_control, nx_busqos->drex_base + BRBRSVCONTROL);

	/* step 3. config tieoff offset */
	if (nx_busqos->drex_s0_qos_enable)
		drex_qos_bits |= ((1 << 4) | (1 << 0));
	if (nx_busqos->drex_s1_qos_enable)
		drex_qos_bits |= ((1 << 12) | (1 << 8));

	nx_tieoff_set((32 << 16) | (TIEOFFREG24 * 32), drex_qos_bits);

	/* step 4. config drex qoscontrol - qos cycle */
	if (nx_busqos->drex_s0_qos_enable)
		drex_s0_qos_tout = nx_busqos->drex_s0_qos_tout;
	if (nx_busqos->drex_s1_qos_enable)
		drex_s1_qos_tout = nx_busqos->drex_s1_qos_tout;

	writel(drex_s0_qos_tout, nx_busqos->drex_base + QOSCONTROL0);
	writel(drex_s1_qos_tout, nx_busqos->drex_base + QOSCONTROL1);
}

/* platform driver interface */
static int nx_busqos_probe(struct platform_device *pdev)
{
	struct nx_busqos *nx_busqos;
	struct resource	*mem;

	nx_busqos = devm_kzalloc(&pdev->dev, sizeof(*nx_busqos), GFP_KERNEL);
	if (!nx_busqos)
		return -ENOMEM;

	if (nx_busqos_parse_dt(&pdev->dev, nx_busqos))
		return -EINVAL;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	nx_busqos->drex_base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(nx_busqos->drex_base))
		return PTR_ERR(nx_busqos->drex_base);

	busqos_setup(nx_busqos);

	platform_set_drvdata(pdev, nx_busqos);
	nx_busqos->dev = &pdev->dev;

	return 0;
}

static int nx_busqos_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int nx_busqos_suspend(struct device *dev)
{
	return 0;
}

static int nx_busqos_resume(struct device *dev)
{
	struct nx_busqos *nx_busqos;

	nx_busqos = dev_get_drvdata(dev);
	if (nx_busqos)
		busqos_setup(nx_busqos);

	return 0;
}

static const struct dev_pm_ops nx_busqos_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(nx_busqos_suspend, nx_busqos_resume)
};
#endif

static const struct of_device_id nx_busqos_match[] = {
	{ .compatible = "nexell,s5p4418-busqos" },
	{ },
};

static struct platform_driver nx_bus_qos_driver = {
	.probe		= nx_busqos_probe,
	.remove		= nx_busqos_remove,
	.driver		= {
		.name	= "nx-busqos",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM_SLEEP
		.pm	= &nx_busqos_pm_ops,
#endif
		.of_match_table = nx_busqos_match,
	},
};

static int __init nx_busqos_init(void)
{
	return platform_driver_register(&nx_bus_qos_driver);
}
arch_initcall(nx_busqos_init);

static void __exit nx_busqos_exit(void)
{
	platform_driver_unregister(&nx_bus_qos_driver);
}
module_exit(nx_busqos_exit);

MODULE_AUTHOR("Bon-gyu, KOO <freestyle@nexell.co.kr>");
MODULE_DESCRIPTION("BUS QoS driver for the Nexell s5p4418");
MODULE_LICENSE("GPL v2");
