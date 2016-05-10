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
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/of_irq.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/err.h>

#include <linux/soc/nexell/s5pxx18-gpio.h>
#include "pinctrl-nexell.h"
#include "pinctrl-s5p6818.h"

static void irq_gpio_ack(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	writel((1 << bit), base + GPIO_INT_STATUS); /* irq pend clear */
	ARM_DMB();
}

static void irq_gpio_mask(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	/* mask:irq disable */
	writel(readl(base + GPIO_INT_ENB) & ~(1 << bit), base + GPIO_INT_ENB);
	writel(readl(base + GPIO_INT_DET) & ~(1 << bit), base + GPIO_INT_DET);
}

static void irq_gpio_unmask(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	/* unmask:irq enable */
	writel(readl(base + GPIO_INT_ENB) | (1 << bit), base + GPIO_INT_ENB);
	writel(readl(base + GPIO_INT_DET) | (1 << bit), base + GPIO_INT_DET);
	ARM_DMB();
}

static int irq_gpio_set_type(struct irq_data *irqd, unsigned int type)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;
	u32 val, alt;
	ulong reg;

	int mode = 0;

	pr_debug("%s: gpio irq=%d, %s.%d, type=0x%x\n", __func__, bank->irq,
		 bank->name, bit, type);

	switch (type) {
	case IRQ_TYPE_NONE:
		pr_warn("%s: No edge setting!\n", __func__);
		break;
	case IRQ_TYPE_EDGE_RISING:
		mode = NX_GPIO_INTMODE_RISINGEDGE;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		mode = NX_GPIO_INTMODE_FALLINGEDGE;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		mode = NX_GPIO_INTMODE_BOTHEDGE;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		mode = NX_GPIO_INTMODE_LOWLEVEL;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		mode = NX_GPIO_INTMODE_HIGHLEVEL;
		break;
	default:
		pr_err("%s: No such irq type %d", __func__, type);
		return -1;
	}

	/*
	 * must change mode to gpio to use gpio interrupt
	 */

	/* gpio out : output disable */
	writel(readl(base + GPIO_INT_OUT) & ~(1 << bit), base + GPIO_INT_OUT);

	/* gpio mode : interrupt mode */
	reg = (ulong)(base + GPIO_INT_MODE0 + (bit / 16) * 4);
	val = (readl((void *)reg) & ~(3 << ((bit & 0xf) * 2))) |
	      ((mode & 0x3) << ((bit & 0xf) * 2));
	writel(val, (void *)reg);

	reg = (ulong)(base + GPIO_INT_MODE1);
	val = (readl((void *)reg) & ~(1 << bit)) | (((mode >> 2) & 0x1) << bit);
	writel(val, (void *)reg);

	/* gpio alt : gpio mode for irq */
	reg = (ulong)(base + GPIO_INT_ALT + (bit / 16) * 4);
	val = readl((void *)reg) & ~(3 << ((bit & 0xf) * 2));
	alt = nx_soc_gpio_get_altnum(bank->grange.pin_base + bit);
	val |= alt << ((bit & 0xf) * 2);
	writel(val, (void *)reg);
	pr_debug("%s: set func to gpio. alt:%d, base:%d, bit:%d\n", __func__,
		 alt, bank->grange.pin_base, bit);

	return 0;
}

static void irq_gpio_enable(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	/* unmask:irq enable */
	writel(readl(base + GPIO_INT_ENB) | (1 << bit), base + GPIO_INT_ENB);
	writel(readl(base + GPIO_INT_DET) | (1 << bit), base + GPIO_INT_DET);
}

static void irq_gpio_disable(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	/* mask:irq disable */
	writel(readl(base + GPIO_INT_ENB) & ~(1 << bit), base + GPIO_INT_ENB);
	writel(readl(base + GPIO_INT_DET) & ~(1 << bit), base + GPIO_INT_DET);
}

/*
 * irq_chip for gpio interrupts.
 */
static struct irq_chip s5p6818_gpio_irq_chip = {
	.name = "GPIO",
	.irq_ack = irq_gpio_ack,
	.irq_mask = irq_gpio_mask,
	.irq_unmask = irq_gpio_unmask,
	.irq_set_type = irq_gpio_set_type,
	.irq_enable = irq_gpio_enable,
	.irq_disable = irq_gpio_disable,
	.flags = IRQCHIP_SKIP_SET_WAKE,
};

static int s5p6818_gpio_irq_map(struct irq_domain *h, unsigned int virq,
				irq_hw_number_t hw)
{
	struct nexell_pin_bank *b = h->host_data;

	pr_debug("%s domain map: virq %d and hw %d\n", __func__, virq, (int)hw);

	irq_set_chip_data(virq, b);
	irq_set_chip_and_handler(virq, &s5p6818_gpio_irq_chip,
				 handle_level_irq);
	return 0;
}

/*
 * irq domain callbacks for external gpio interrupt controller.
 */
static const struct irq_domain_ops s5p6818_gpio_irqd_ops = {
	.map = s5p6818_gpio_irq_map, .xlate = irq_domain_xlate_twocell,
};

static irqreturn_t s5p6818_gpio_irq_handler(int irq, void *data)
{
	struct nexell_pin_bank *bank = data;
	void __iomem *base = bank->virt_base;
	u32 stat, mask;
	unsigned int virq;
	int bit;

	mask = readl(base + GPIO_INT_ENB);
	stat = readl(base + GPIO_INT_STATUS) & mask;
	bit = ffs(stat) - 1;

	if (-1 == bit) {
		pr_err("Unknown gpio irq=%d, status=0x%08x, mask=0x%08x\r\n",
		       irq, stat, mask);
		writel(-1, (base + GPIO_INT_STATUS)); /* clear gpio status all*/
		return IRQ_NONE;
	}

	virq = irq_linear_revmap(bank->irq_domain, bit);
	if (!virq)
		return IRQ_NONE;

	pr_debug("Gpio irq=%d [%d] (hw %u), stat=0x%08x, mask=0x%08x\n", irq,
		 bit, virq, stat, mask);
	generic_handle_irq(virq);

	return IRQ_HANDLED;
}

/*
 * s5p6818_gpio_irq_init() - setup handling of external gpio interrupts.
 * @d: driver data of nexell pinctrl driver.
 */
static int s5p6818_gpio_irq_init(struct nexell_pinctrl_drv_data *d)
{
	struct nexell_pin_bank *bank;
	struct device *dev = d->dev;
	int ret;
	int i;

	bank = d->ctrl->pin_banks;
	for (i = 0; i < d->ctrl->nr_banks; ++i, ++bank) {
		if (bank->eint_type != EINT_TYPE_GPIO)
			continue;

		ret = devm_request_irq(dev, bank->irq, s5p6818_gpio_irq_handler,
				       0, dev_name(dev), bank);
		if (ret) {
			dev_err(dev, "irq request failed\n");
			ret = -ENXIO;
			goto err_domains;
		}

		bank->irq_domain = irq_domain_add_linear(
		    bank->of_node, bank->nr_pins, &s5p6818_gpio_irqd_ops, bank);
		if (!bank->irq_domain) {
			dev_err(dev, "gpio irq domain add failed\n");
			ret = -ENXIO;
			goto err_domains;
		}
	}

	return 0;

err_domains:
	for (--i, --bank; i >= 0; --i, --bank) {
		if (bank->eint_type != EINT_TYPE_GPIO)
			continue;
		irq_domain_remove(bank->irq_domain);
		devm_free_irq(dev, bank->irq, d);
	}

	return ret;
}

static void irq_alive_ack(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: irq=%d, hwirq=%d\n", __func__, irqd->irq, bit);
	/* ack: irq pend clear */
	writel(1 << bit, base + ALIVE_INT_STATUS);

	ARM_DMB();
}

static void irq_alive_mask(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: irq=%d, hwirq=%d\n", __func__, irqd->irq, bit);
	/* mask: irq reset (disable) */
	writel((1 << bit), base + ALIVE_INT_RESET);
}

static void irq_alive_unmask(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: irq=%d, hwirq=%d\n", __func__, irqd->irq, bit);
	writel((1 << bit), base + ALIVE_INT_SET);
	ARM_DMB();
}

static int irq_alive_set_type(struct irq_data *irqd, unsigned int type)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;
	int offs = 0, i = 0;
	int mode = 0;

	pr_debug("%s: irq=%d, hwirq=%d, type=0x%x\n", __func__, irqd->irq, bit,
		 type);

	switch (type) {
	case IRQ_TYPE_NONE:
		pr_warn("%s: No edge setting!\n", __func__);
		break;
	case IRQ_TYPE_EDGE_FALLING:
		mode = NX_ALIVE_DETECTMODE_SYNC_FALLINGEDGE;
		break;
	case IRQ_TYPE_EDGE_RISING:
		mode = NX_ALIVE_DETECTMODE_SYNC_RISINGEDGE;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		mode = NX_ALIVE_DETECTMODE_SYNC_FALLINGEDGE;
		break; /* and Rising Edge */
	case IRQ_TYPE_LEVEL_LOW:
		mode = NX_ALIVE_DETECTMODE_ASYNC_LOWLEVEL;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		mode = NX_ALIVE_DETECTMODE_ASYNC_HIGHLEVEL;
		break;
	default:
		pr_err("%s: No such irq type %d", __func__, type);
		return -1;
	}

	/* setting all alive detect mode set/reset register */
	for (; 6 > i; i++, offs += 0x0C) {
		u32 reg = (i == mode ? ALIVE_MOD_SET : ALIVE_MOD_RESET);

		writel(1 << bit, (base + reg + offs));
	}

	/*
	 * set risingedge mode for both edge
	 * 0x2C : Risingedge
	 */
	if (IRQ_TYPE_EDGE_BOTH == type)
		writel(1 << bit, (base + 0x2C));

	writel(1 << bit, base + ALIVE_DET_SET);
	writel(1 << bit, base + ALIVE_INT_SET);
	writel(1 << bit, base + ALIVE_OUT_RESET);

	return 0;
}

static u32 alive_wake_mask = 0xffffffff;

u32 get_wake_mask(void)
{
	return alive_wake_mask;
}

static int irq_set_alive_wake(struct irq_data *irqd, unsigned int on)
{
	int bit = (int)(irqd->hwirq);

	pr_info("alive wake bit[%d] %s for irq %d\n",
		       bit, on ? "enabled" : "disabled", irqd->irq);

	if (!on)
		alive_wake_mask |= bit;
	else
		alive_wake_mask &= ~bit;

	return 0;
}

static void irq_alive_enable(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: irq=%d, io=%s.%d\n", __func__, bank->irq, bank->name,
		 bit);
	/* unmask:irq set (enable) */
	writel((1 << bit), base + ALIVE_INT_SET);
	ARM_DMB();
}

static void irq_alive_disable(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: irq=%d, io=%s.%d\n", __func__, bank->irq, bank->name,
		 bit);
	/* mask:irq reset (disable) */
	writel((1 << bit), base + ALIVE_INT_RESET);
}

/*
 * irq_chip for wakeup interrupts
 */
static struct irq_chip s5p6818_alive_irq_chip = {
	.name = "ALIVE",
	.irq_ack = irq_alive_ack,
	.irq_mask = irq_alive_mask,
	.irq_unmask = irq_alive_unmask,
	.irq_set_type = irq_alive_set_type,
	.irq_set_wake = irq_set_alive_wake,
	.irq_enable = irq_alive_enable,
	.irq_disable = irq_alive_disable,
};

static irqreturn_t s5p6818_alive_irq_handler(int irq, void *data)
{
	struct nexell_pin_bank *bank = data;
	void __iomem *base = bank->virt_base;
	u32 stat, mask;
	unsigned int virq;
	int bit;

	mask = readl(base + ALIVE_INT_SET_READ);
	stat = readl(base + ALIVE_INT_STATUS) & mask;
	bit = ffs(stat) - 1;

	if (-1 == bit) {
		pr_err("Unknown gpio irq=%d, status=0x%08x, mask=0x%08x\r\n",
		       irq, stat, mask);
		writel(-1, (base + ALIVE_INT_STATUS)); /* clear alive status */
		return IRQ_NONE;
	}

	virq = irq_linear_revmap(bank->irq_domain, bit);
	pr_debug("alive irq=%d [%d] (hw %u), stat=0x%08x, mask=0x%08x\n", irq,
		 bit, virq, stat, mask);
	if (!virq)
		return IRQ_NONE;

	generic_handle_irq(virq);

	return IRQ_HANDLED;
}

static int s5p6818_alive_irq_map(struct irq_domain *h, unsigned int virq,
				 irq_hw_number_t hw)
{
	pr_debug("%s domain map: virq %d and hw %d\n", __func__, virq, (int)hw);

	irq_set_chip_and_handler(virq, &s5p6818_alive_irq_chip,
				 handle_level_irq);
	irq_set_chip_data(virq, h->host_data);
	return 0;
}

static const struct irq_domain_ops s5p6818_alive_irqd_ops = {
	.map = s5p6818_alive_irq_map, .xlate = irq_domain_xlate_twocell,
};

/*
 * s5p6818_alive_irq_init() - setup handling of wakeup interrupts.
 * @d: driver data of nexell pinctrl driver.
 */
static int s5p6818_alive_irq_init(struct nexell_pinctrl_drv_data *d)
{
	struct nexell_pin_bank *bank;
	struct device *dev = d->dev;
	int ret;
	int i;

	bank = d->ctrl->pin_banks;
	for (i = 0; i < d->ctrl->nr_banks; ++i, ++bank) {
		void __iomem *base = bank->virt_base;

		if (bank->eint_type != EINT_TYPE_WKUP)
			continue;

		/* clear pending, disable irq detect */
		writel(-1, base + ALIVE_INT_RESET);
		writel(-1, base + ALIVE_INT_STATUS);

		ret =
		    devm_request_irq(dev, bank->irq, s5p6818_alive_irq_handler,
				     0, dev_name(dev), bank);
		if (ret) {
			dev_err(dev, "irq request failed\n");
			ret = -ENXIO;
			goto err_domains;
		}

		bank->irq_domain =
		    irq_domain_add_linear(bank->of_node, bank->nr_pins,
					  &s5p6818_alive_irqd_ops, bank);
		if (!bank->irq_domain) {
			dev_err(dev, "gpio irq domain add failed\n");
			ret = -ENXIO;
			goto err_domains;
		}
	}

	return 0;

err_domains:
	for (--i, --bank; i >= 0; --i, --bank) {
		if (bank->eint_type != EINT_TYPE_WKUP)
			continue;
		irq_domain_remove(bank->irq_domain);
		devm_free_irq(dev, bank->irq, d);
	}

	return ret;
}

static int s5p6818_base_init(struct nexell_pinctrl_drv_data *drvdata)
{
	struct nexell_pin_ctrl *ctrl = drvdata->ctrl;
	int nr_banks = ctrl->nr_banks;
	int ret;
	int i;
	struct module_init_data *init_data, *n;
	LIST_HEAD(banks);

	for (i = 0; i < nr_banks; i++) {
		struct nexell_pin_bank *bank = &ctrl->pin_banks[i];

		init_data = kmalloc(sizeof(*init_data), GFP_KERNEL);
		if (!init_data) {
			ret = -ENOMEM;
			goto done;
		}

		INIT_LIST_HEAD(&init_data->node);
		init_data->bank_base = bank->virt_base;
		init_data->bank_type = bank->eint_type;

		list_add_tail(&init_data->node, &banks);
	}

	s5pxx18_gpio_device_init(&banks, nr_banks);

done:
	/* free */
	list_for_each_entry_safe(init_data, n, &banks, node) {
		list_del(&init_data->node);
		kfree(init_data);
	}

	return 0;
}

/* pin banks of s5p6818 pin-controller 0 */
static struct nexell_pin_bank s5p6818_pin_banks[] = {
	SOC_PIN_BANK_EINTG(32, 0xA000, "gpioa"),
	SOC_PIN_BANK_EINTG(32, 0xB000, "gpiob"),
	SOC_PIN_BANK_EINTG(32, 0xC000, "gpioc"),
	SOC_PIN_BANK_EINTG(32, 0xD000, "gpiod"),
	SOC_PIN_BANK_EINTG(32, 0xE000, "gpioe"),
	SOC_PIN_BANK_EINTW(6, 0x0800, "alive"),
};

/*
 * Nexell pinctrl driver data for SoC.
 */
const struct nexell_pin_ctrl s5p6818_pin_ctrl[] = {
	{
		.pin_banks = s5p6818_pin_banks,
		.nr_banks = ARRAY_SIZE(s5p6818_pin_banks),
		.base_init = s5p6818_base_init,
		.gpio_irq_init = s5p6818_gpio_irq_init,
		.alive_irq_init = s5p6818_alive_irq_init,
	}
};
