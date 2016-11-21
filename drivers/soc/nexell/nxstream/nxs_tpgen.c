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

/* regmap: base 0x20c03e00 */
#define TPGEN_CONTROL0		0x3e00
#define TPGEN_INT_CONTROL	0x3e04
#define TPGEN_CONTROL1		0x3e08
#define TPGEN_CONTROL2		0x3e0c
#define TPGEN_CONTROL3		0x3e10
#define TPGEN_CONTROL4		0x3e14
#define TPGEN_CONTROL5		0x3e18
#define TPGEN_CONTROL6		0x3e1c
#define TPGEN_CONTROL7		0x3e20
#define TPGEN_CONTROL8		0x3e24
#define TPGEN_CONTROL9		0x3e28
#define TPGEN_CONTROL10		0x3e2c

/* TPGEN_CONTROL0 */
#define REG_CLEAR_BIT_OFFSET	17
#define REG_CLEAR_BIT_MASK	0x1
#define TZPROT_BIT_OFFSET	16
#define TZPROT_BIT_MASK		0x1
#define NCLK_PER_TWOPIX_BIT_OFFSET	8
#define NCLK_PER_TWOPIX_BIT_MASK	0xff
#define REALTIME_MODE_BIT_OFFSET	7
#define REALTIME_MODE_BIT_MASK		0x1
#define TPGEN_MODE_BIT_OFFSET		4
#define TPGEN_MODE_BIT_MASK		0x7
#define TPGEN_IDLE_BIT_OFFSET		1
#define TPGEN_IDLE_BIT_MASK		0x1
#define TPGEN_ENABLE_BIT_OFFSET		0
#define TPGEN_ENABLE_BIT_MASK		0x1

/* TPGEN_INT_CONTROL */
#define UPDATE_INTDISABLE_BIT_OFFSET	14
#define UPDATE_INTDISABLE_BIT_MASK	0x1
#define UPDATE_INTENB_BIT_OFFSET	13
#define UPDATE_INTENB_BIT_MASK		0x1
#define UPDATE_INTPEND_BIT_OFFSET	12
#define UPDATE_INTPEND_BIT_MASK		0x1
#define IRQ_OVF_INTDISABLE_BIT_OFFSET	10
#define IRQ_OVF_INTDISABLE_BIT_MASK	0x1
#define IRQ_OVF_INTENB_BIT_OFFSET	9
#define IRQ_OVF_INTENB_BIT_MASK		0x1
#define IRQ_OVF_INTPEND_BIT_OFFSET	8
#define IRQ_OVF_INTPEND_BIT_MASK	0x1
#define IDLE_INTDISABLE_BIT_OFFSET	6
#define IDLE_INTDISABLE_BIT_MASK	0x1
#define IDLE_INTENB_BIT_OFFSET		5
#define IDLE_INTENB_BIT_MASK		0x1
#define IDLE_INTPEND_BIT_OFFSET		4
#define IDLE_INTPEND_BIT_MASK		0x1
#define IRQ_DONE_INTDISABLE_BIT_OFFSET	2
#define IRQ_DONE_INTDISABLE_BIT_MASK	0x1
#define IRQ_DONE_INTENB_BIT_OFFSET	1
#define IRQ_DONE_INTENB_BIT_MASK	0x1
#define IRQ_DONE_INTPEND_BIT_OFFSET	0
#define IRQ_DONE_INTPEND_BIT_MASK	0x1

/* TPGEN_CONTROL1 */
#define ENC_HEADER_0_BIT_OFFSET		0
#define ENC_HEADER_0_BIT_MASK		0xffffffff

/* TPGEN_CONTROL2 */
#define ENC_HEADER_1_BIT_OFFSET		0
#define ENC_HEADER_1_BIT_MASK		0xffffffff

/* TPGEN_CONTROL3 */
#define ENC_HEADER_2_BIT_OFFSET		0
#define ENC_HEADER_2_BIT_MASK		0xffffffff

/* TPGEN_CONTROL4 */
#define ENC_HEADER_3_BIT_OFFSET		0
#define ENC_HEADER_3_BIT_MASK		0xffffffff

/* TPGEN_CONTROL5 */
#define ENC_WIDTH_BIT_OFFSET		16
#define ENC_WIDTH_BIT_MASK		0xffff
#define ENC_HEIGHT_BIT_OFFSET		0
#define ENC_HEIGHT_BIT_MASK		0xffff

/* TPGEN_CONTROL6 */
#define ENC_IMG_TYPE_BIT_OFFSET		18
#define ENC_IMG_TYPE_BIT_MASK		0x3
#define ENC_FIELD_BIT_OFFSET		16
#define ENC_FIELD_BIT_MASK		0x3
#define ENC_SECURE_BIT_OFFSET		15
#define ENC_SECURE_BIT_MASK		0x1
#define ENC_REG_FIRE_BIT_OFFSET		14
#define ENC_REG_FIRE_BIT_MASK		0x1
#define ENC_TID_BIT_OFFSET		0
#define ENC_TID_BIT_MASK		0x3fff

/* TPGEN_CONTROL7 */
#define V2H_DELAY_BIT_OFFSET		0
#define V2H_DELAY_BIT_MASK		0xffffffff

/* TPGEN_CONTROL8 */
#define H2H_DELAY_BIT_OFFSET		0
#define H2H_DELAY_BIT_MASK		0xffffffff

/* TPGEN_CONTROL9 */
#define H2V_DELAY_BIT_OFFSET		0
#define H2V_DELAY_BIT_MASK		0xffffffff

/* TPGEN_CONTROL10 */
#define V2V_DELAY_BIT_OFFSET		0
#define V2V_DELAY_BIT_MASK		0xffffffff

static void tpgen_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				     bool enable)
{
}

static u32 tpgen_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static u32 tpgen_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	return 0;
}

static void tpgen_clear_interrupt_pending(const struct nxs_dev *pthis, int type)
{
}

static int tpgen_open(const struct nxs_dev *pthis)
{
	return 0;
}

static int tpgen_close(const struct nxs_dev *pthis)
{
	return 0;
}

static int tpgen_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int tpgen_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int tpgen_set_syncinfo(const struct nxs_dev *pthis,
			    const union nxs_control *pparam)
{
	return 0;
}

static int tpgen_get_syncinfo(const struct nxs_dev *pthis,
			    union nxs_control *pparam)
{
	return 0;
}

static int nxs_tpgen_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_dev *nxs_dev;

	nxs_dev = devm_kzalloc(&pdev->dev, sizeof(*nxs_dev), GFP_KERNEL);
	if (!nxs_dev)
		return -ENOMEM;


	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	nxs_dev->set_interrupt_enable = tpgen_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = tpgen_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = tpgen_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = tpgen_clear_interrupt_pending;
	nxs_dev->open = tpgen_open;
	nxs_dev->close = tpgen_close;
	nxs_dev->start = tpgen_start;
	nxs_dev->stop = tpgen_stop;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_SYNCINFO;
	nxs_dev->dev_services[0].set_control = tpgen_set_syncinfo;
	nxs_dev->dev_services[0].get_control = tpgen_get_syncinfo;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, nxs_dev);

	return 0;
}

static int nxs_tpgen_remove(struct platform_device *pdev)
{
	struct nxs_dev *nxs_dev = platform_get_drvdata(pdev);

	if (nxs_dev)
		unregister_nxs_dev(nxs_dev);

	return 0;
}

static const struct of_device_id nxs_tpgen_match[] = {
	{ .compatible = "nexell,tpgen-nxs-1.0", },
	{},
};

static struct platform_driver nxs_tpgen_driver = {
	.probe	= nxs_tpgen_probe,
	.remove	= nxs_tpgen_remove,
	.driver	= {
		.name = "nxs-tpgen",
		.of_match_table = of_match_ptr(nxs_tpgen_match),
	},
};

static int __init nxs_tpgen_init(void)
{
	return platform_driver_register(&nxs_tpgen_driver);
}
/* subsys_initcall(nxs_tpgen_init); */
fs_initcall(nxs_tpgen_init);

static void __exit nxs_tpgen_exit(void)
{
	platform_driver_unregister(&nxs_tpgen_driver);
}
module_exit(nxs_tpgen_exit);

MODULE_DESCRIPTION("Nexell Stream TPGEN driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

