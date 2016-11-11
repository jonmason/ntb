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

#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/delay.h>

#include <dt-bindings/reset/nexell,nxp5540-reset.h>

#define NX_ID_TO_BANK(id)	((id >> 16) & 0xFF)
#define NX_ID_TO_OFFS(id)	(id & 0xFF)
#define NX_NUM_TO_ID(bank,offs)	((bank << 16) | offs)

#define RST_WORD(offs)		((offs) / 32)
#define RST_BIT(offs)		((offs) % 32)
#define RST_MASK(offs)		(1UL << ((offs) % 32))


/* reset bank */
struct nexell_reset_bank {
	char		*name;
	u32		nr_resets;
	void __iomem	*base;
	struct nexell_reset_ctrl *ctrl;
};

/* reset controller */
struct nexell_reset_ctrl {
	struct nexell_reset_bank	*reset_banks;
	u32				nr_banks;

	spinlock_t			lock;
	struct reset_controller_dev	rcdev;
};


static int nxp5540_reset_assert(struct reset_controller_dev *rcdev,
			       unsigned long id)
{
	struct nexell_reset_ctrl *ctrl =
		container_of(rcdev, struct nexell_reset_ctrl, rcdev);
	u32 bank = NX_ID_TO_BANK(id);
	u32 offset = NX_ID_TO_OFFS(id);
	u32 bit = RST_BIT(offset);
	void __iomem *reg =
		ctrl->reset_banks[bank].base + (RST_WORD(offset) * 4);
	u32 val;
	unsigned long flags;

	pr_debug("%s: %d.%d [%p.%d]\n", __func__, bank, offset, reg, bit);

	spin_lock_irqsave(&ctrl->lock, flags);

	val = readl(reg);
	writel(val & ~RST_MASK(offset), reg);

	spin_unlock_irqrestore(&ctrl->lock, flags);

	return 0;
}

static int nxp5540_reset_deassert(struct reset_controller_dev *rcdev,
				 unsigned long id)
{
	struct nexell_reset_ctrl *ctrl =
		container_of(rcdev, struct nexell_reset_ctrl, rcdev);
	u32 bank = NX_ID_TO_BANK(id);
	u32 offset = NX_ID_TO_OFFS(id);
	u32 bit = RST_BIT(offset);
	void __iomem *reg =
		ctrl->reset_banks[bank].base + (RST_WORD(offset) * 4);
	u32 val;
	unsigned long flags;

	pr_debug("%s: %d.%d [%p.%d]\n", __func__, bank, offset, reg, bit);

	spin_lock_irqsave(&ctrl->lock, flags);

	val = readl(reg);
	writel(val | RST_MASK(offset), reg);

	spin_unlock_irqrestore(&ctrl->lock, flags);

	return 0;
}

/* must reset deassert when clk was disabled */
static int nxp5540_reset_reset(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	nxp5540_reset_assert(rcdev, id);
	nxp5540_reset_deassert(rcdev, id);

	return 0;
}

static int nxp5540_reset_status(struct reset_controller_dev *rcdev,
			       unsigned long id)
{
	struct nexell_reset_ctrl *ctrl =
		container_of(rcdev, struct nexell_reset_ctrl, rcdev);
	u32 bank = NX_ID_TO_BANK(id);
	u32 offset = NX_ID_TO_OFFS(id);
	u32 bit = RST_BIT(offset);
	void __iomem *reg =
		ctrl->reset_banks[bank].base + (RST_WORD(offset) * 4);
	u32 val;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&ctrl->lock, flags);

	val = readl(reg);

	spin_unlock_irqrestore(&ctrl->lock, flags);

	ret = !(val & RST_MASK(offset));
	pr_debug("%s: %d.%d [%p.%d] val: %d\n",
		 __func__, bank, offset, reg, bit, ret);

	return ret;
}


static struct reset_control_ops nxp5540_reset_ops = {
	.reset		= nxp5540_reset_reset,
	.assert		= nxp5540_reset_assert,
	.deassert	= nxp5540_reset_deassert,
	.status		= nxp5540_reset_status,
};

static int nxp5540_reset_xlate(struct reset_controller_dev *rcdev,
			      const struct of_phandle_args *reset_spec)
{
	struct nexell_reset_ctrl *ctrl =
		container_of(rcdev, struct nexell_reset_ctrl, rcdev);
	u32 nr_resets;

	u32 bank, offs;

	if (WARN_ON(reset_spec->args_count != rcdev->of_reset_n_cells))
		return -EINVAL;

	bank = reset_spec->args[0];
	offs = reset_spec->args[1];

	nr_resets = ctrl->reset_banks[bank].nr_resets;

	if (offs > nr_resets)
		return -EINVAL;

	return NX_NUM_TO_ID(bank, offs);
}

static struct nexell_reset_bank nxp5540_rst_banks[] = {
	{
		.name = "reset_sys",
		.nr_resets = 53,
	}, {
		.name = "reset_tbus",
		.nr_resets = 14,
	}, {
		.name = "reset_lbus",
		.nr_resets = 18,
	}, {
		.name = "reset_bbus",
		.nr_resets = 36,
	}, {
		.name = "reset_coda",
		.nr_resets = 8,
	}, {
		.name = "reset_disp",
		.nr_resets = 103,
	}, {
		.name = "reset_usb",
		.nr_resets = 11,
	}, {
		.name = "reset_hdmi",
		.nr_resets = 10,
	}, {
		.name = "reset_wave",
		.nr_resets = 5,
	}, {
		.name = "reset_drex",
		.nr_resets = 7,
	}, {
		.name = "reset_wave420",
		.nr_resets = 5,
	}, {
		.name = "reset_cpu",
		.nr_resets = 1,
	}, {
		.name = "reset_periclk",
		.nr_resets = 3,
	}
};

const struct nexell_reset_ctrl nxp5540_rstcon[] = {
	{
		.reset_banks = nxp5540_rst_banks,
		.nr_banks = ARRAY_SIZE(nxp5540_rst_banks),
	}
};

static const struct of_device_id nxp5540_reset_dt_ids[] = {
	{ .compatible = "nexell,nxp5540-reset",
		.data = (void *)nxp5540_rstcon },
	{ },
};
MODULE_DEVICE_TABLE(of, nxp5540_reset_dt_ids);

static int nxp5540_reset_probe(struct platform_device *pdev)
{
	struct nexell_reset_ctrl *ctrl;
	struct resource *res;
	int i;
	int nr_resets = 0;

	ctrl = devm_kzalloc(&pdev->dev, sizeof(*ctrl), GFP_KERNEL);
	if (!ctrl)
		return -ENOMEM;

	for (i = 0; ; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res)
			break;
	}
	if (i != ctrl->nr_banks) {
		dev_err(&pdev->dev, "bank count mismatch\n");
		return -EINVAL;
	}

	for (i = 0; i < ctrl->nr_banks; i++) {
		struct nexell_reset_bank *bank = &ctrl->reset_banks[i];

		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		bank->base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(bank->base))
			return PTR_ERR(bank->base);
		bank->ctrl = ctrl;
		nr_resets += bank->nr_resets;
	}

	spin_lock_init(&ctrl->lock);

	ctrl->rcdev.owner = THIS_MODULE;
	ctrl->rcdev.nr_resets = nr_resets;
	ctrl->rcdev.ops = &nxp5540_reset_ops;
	ctrl->rcdev.of_node = pdev->dev.of_node;
	ctrl->rcdev.of_reset_n_cells = 2;
	ctrl->rcdev.of_xlate = nxp5540_reset_xlate;
	dev_info(&pdev->dev, "nexell reset: %s nr_resets [%d]\n",
		 dev_name(&pdev->dev), ctrl->rcdev.nr_resets);

	return reset_controller_register(&ctrl->rcdev);
}

static int nxp5540_reset_remove(struct platform_device *pdev)
{
	struct nexell_reset_ctrl *ctrl = platform_get_drvdata(pdev);

	reset_controller_unregister(&ctrl->rcdev);

	return 0;
}

static struct platform_driver nxp5540_reset_driver = {
	.probe	= nxp5540_reset_probe,
	.remove	= nxp5540_reset_remove,
	.driver = {
		.name		= "nxp5540-reset",
		.owner		= THIS_MODULE,
		.of_match_table	= nxp5540_reset_dt_ids,
	},
};

static int __init nxp5540_reset_init(void)
{
	return platform_driver_register(&nxp5540_reset_driver);
}
pure_initcall(nxp5540_reset_init);

MODULE_AUTHOR("Bon-gyu, KOO <freestyle@nexell.co.kr");
MODULE_DESCRIPTION("Reset Controller Driver for Nexell's nxp5540");
MODULE_LICENSE("GPL");
