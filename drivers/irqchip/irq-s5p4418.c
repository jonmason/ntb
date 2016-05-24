/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Youngbok, Park <ybpark@nexell.co.kr>
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

#include <linux/export.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/cpu.h>
#include <linux/cpu_pm.h>
#include <linux/cpumask.h>
#include <linux/syscore_ops.h>
#include <linux/device.h>
#include <linux/amba/bus.h>
#include <linux/irqchip/arm-vic.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/irqchip/arm-gic-acpi.h>
#include <asm/cputype.h>
#include <asm/exception.h>
#include <asm/smp_plat.h>

#include "irq-gic-common.h"
#include "irqchip.h"

#ifdef CONFIG_OF

/* VIC Definitions */

#define VIC_IRQ_STATUS			0x00
#define VIC_FIQ_STATUS			0x04
#define VIC_INT_SELECT			0x0c	/* 1 = FIQ, 0 = IRQ */
#define VIC_INT_SOFT			0x18
#define VIC_INT_SOFT_CLEAR		0x1c
#define VIC_PROTECT			0x20
#define VIC_PL190_VECT_ADDR		0x30	/* PL190 only */
#define VIC_PL190_DEF_VECT_ADDR		0x34	/* PL190 only */

#define VIC_VECT_ADDR0			0x100	/* 0 to 15 (0..31 PL192) */
#define VIC_VECT_CNTL0			0x200	/* 0 to 15 (0..31 PL192) */
#define VIC_ITCR			0x300	/* VIC test control register */

#define VIC_VECT_CNTL_ENABLE		(1 << 5)

#define VIC_PL192_VECT_ADDR		0xF00

/**
 * struct vic_device - VIC PM device
 * @parent_irq: The parent IRQ number of the VIC if cascaded, or 0.
 * @irq: The IRQ number for the base of the VIC.
 * @base: The register base for the VIC.
 * @valid_sources: A bitmask of valid interrupts
 * @resume_sources: A bitmask of interrupts for resume.
 * @resume_irqs: The IRQs enabled for resume.
 * @int_select: Save for VIC_INT_SELECT.
 * @int_enable: Save for VIC_INT_ENABLE.
 * @soft_int: Save for VIC_INT_SOFT.
 * @protect: Save for VIC_PROTECT.
 * @domain: The IRQ domain for the VIC.
 */
struct vic_device {
	void __iomem	*base;
	void __iomem	*base1;
	int		irq;
	u32		valid_sources;
	u32		resume_sources;
	u32		resume_irqs;
	u32		int_select;
	u32		int_enable;
	u32		soft_int;
	u32		protect;
	u32		int_select1;
	u32		int_enable1;
	u32		soft_int1;
	u32		protect1;
	struct irq_domain *domain;
};

/* we cannot allocate memory when VICs are initially registered */
static struct vic_device vic_devices[CONFIG_ARM_VIC_NR];

static int vic_id;

static void vic_handle_irq(struct pt_regs *regs);
static void vic_init2(void __iomem *base)
{
	int i;

	for (i = 0; i < 16; i++) {
		void __iomem *reg = base + VIC_VECT_CNTL0 + (i * 4);

		writel(VIC_VECT_CNTL_ENABLE | i, reg);
	}

	writel(32, base + VIC_PL190_DEF_VECT_ADDR);
}

#ifdef CONFIG_PM
static void resume_one_vic(struct vic_device *vic)
{
	void __iomem *base = vic->base;
	void __iomem *base1 = vic->base1;

	pr_debug("%s: resuming vic at %p\n", __func__, base);

	/* re-initialise static settings */
	vic_init2(base);

	writel(vic->int_select, base + VIC_INT_SELECT);
	writel(vic->protect, base + VIC_PROTECT);

	/* set the enabled ints and then clear the non-enabled */
	writel(vic->int_enable, base + VIC_INT_ENABLE);
	writel(~vic->int_enable, base + VIC_INT_ENABLE_CLEAR);

	/* and the same for the soft-int register */

	writel(vic->soft_int, base + VIC_INT_SOFT);
	writel(~vic->soft_int, base + VIC_INT_SOFT_CLEAR);

	vic_init2(base1);

	writel(vic->int_select1, base1 + VIC_INT_SELECT);
	writel(vic->protect1, base1 + VIC_PROTECT);

	/* set the enabled ints and then clear the non-enabled */
	writel(vic->int_enable1, base1 + VIC_INT_ENABLE);
	writel(~vic->int_enable1, base1 + VIC_INT_ENABLE_CLEAR);

	/* and the same for the soft-int register */

	writel(vic->soft_int1, base1 + VIC_INT_SOFT);
	writel(~vic->soft_int1, base1 + VIC_INT_SOFT_CLEAR);

}

static void vic_resume(void)
{
	int id;

	for (id = vic_id - 1; id >= 0; id--)
		resume_one_vic(vic_devices + id);
}

static void suspend_one_vic(struct vic_device *vic)
{
	void __iomem *base = vic->base;
	void __iomem *base1 = vic->base;

	pr_debug("%s: suspending vic at %p, %p\n", __func__, base, base1);
	vic->int_select = readl(base + VIC_INT_SELECT);
	vic->int_enable = readl(base + VIC_INT_ENABLE);
	vic->soft_int = readl(base + VIC_INT_SOFT);
	vic->protect = readl(base + VIC_PROTECT);

	/* set the interrupts (if any) that are used for
	 * resuming the system */

	writel(vic->resume_irqs, base + VIC_INT_ENABLE);
	writel(~vic->resume_irqs, base + VIC_INT_ENABLE_CLEAR);


	vic->int_select1 = readl(base1 + VIC_INT_SELECT);
	vic->int_enable1 = readl(base1 + VIC_INT_ENABLE);
	vic->soft_int1 = readl(base1 + VIC_INT_SOFT);
	vic->protect1 = readl(base1 + VIC_PROTECT);

	/* set the interrupts (if any) that are used for
	 * resuming the system */

	writel(vic->resume_irqs, base1 + VIC_INT_ENABLE);
	writel(~vic->resume_irqs, base1 + VIC_INT_ENABLE_CLEAR);

}

static int vic_suspend(void)
{
	int id;

	for (id = 0; id < vic_id; id++)
		suspend_one_vic(vic_devices + id);

	return 0;
}

struct syscore_ops s5p4418_vic_syscore_ops = {
	.suspend	= vic_suspend,
	.resume		= vic_resume,
};

/**
 * vic_pm_init - initicall to register VIC pm
 *
 * This is called via late_initcall() to register
 * the resources for the VICs due to the early
 * nature of the VIC's registration.
*/
static int __init vic_pm_init(void)
{
	if (vic_id > 0)
		register_syscore_ops(&s5p4418_vic_syscore_ops);

	return 0;
}
late_initcall(vic_pm_init);
#endif /* CONFIG_PM */

static struct irq_chip vic_chip;

static int vic_irqdomain_map(struct irq_domain *d, unsigned int irq,
			     irq_hw_number_t hwirq)
{
	struct vic_device *v = d->host_data;
	/* Skip invalid IRQs, only register handlers for the real ones */
	irq_set_chip_and_handler(irq, &vic_chip, handle_level_irq);
	if (hwirq > 31)
		irq_set_chip_data(irq, v->base1);
	else
		irq_set_chip_data(irq, v->base);

	set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
	return 0;
}

/*
 * Handle each interrupt in a single VIC.  Returns non-zero if we've
 * handled at least one interrupt.  This reads the status register
 * before handling each interrupt, which is necessary given that
 * handle_IRQ may briefly re-enable interrupts for soft IRQ handling.
 */
static int handle_one_vic(struct vic_device *vic, struct pt_regs *regs)
{
	u32 stat, irq;
	int handled = 0;

	while ((stat = readl_relaxed(vic->base + VIC_IRQ_STATUS))) {
		irq = ffs(stat) - 1;
		handle_domain_irq(vic->domain, irq, regs);
		handled = 1;
	}

	return handled;
}

static int handle_two_vic(struct vic_device *vic, struct pt_regs *regs)
{
	u32 stat, irq;
	int handled = 0;

	while ((stat = readl_relaxed(vic->base1 + VIC_IRQ_STATUS))) {
		irq = ffs(stat) - 1 + 32;
		handle_domain_irq(vic->domain, irq, regs);
		handled = 1;
	}
	return handled;
}
static void vic_handle_irq_cascaded(unsigned int irq, struct irq_desc *desc)
{
	u32 stat, hwirq;
	struct irq_chip *host_chip = irq_desc_get_chip(desc);
	struct vic_device *vic = irq_desc_get_handler_data(desc);

	chained_irq_enter(host_chip, desc);

	while ((stat = readl_relaxed(vic->base + VIC_IRQ_STATUS))) {
		hwirq = ffs(stat) - 1;
		generic_handle_irq(irq_find_mapping(vic->domain, hwirq));
	}

	chained_irq_exit(host_chip, desc);
}

/*
 * Keep iterating over all registered VIC's until there are no pending
 * interrupts.
 */


static void __exception_irq_entry vic_handle_irq(struct pt_regs *regs)
{
	int i, handled;

	do {
		for (i = 0, handled = 0; i < vic_id; ++i)
			handled |= handle_one_vic(&vic_devices[i], regs);
	} while (handled);
	do {
		for (i = 0, handled = 0; i < vic_id; ++i)
			handled |= handle_two_vic(&vic_devices[i], regs);
	} while (handled);

}

static struct irq_domain_ops s5p4418_vic_irqdomain_ops = {
	.map = vic_irqdomain_map,
	.xlate = irq_domain_xlate_onetwocell,
};

/**
 * vic_register() - Register a VIC.
 * @base: The base address of the VIC.
 * @parent_irq: The parent IRQ if cascaded, else 0.
 * @irq: The base IRQ for the VIC.
 * @valid_sources: bitmask of valid interrupts
 * @resume_sources: bitmask of interrupts allowed for resume sources.
 * @node: The device tree node associated with the VIC.
 *
 * Register the VIC with the system device tree so that it can be notified
 * of suspend and resume requests and ensure that the correct actions are
 * taken to re-instate the settings on resume.
 *
 * This also configures the IRQ domain for the VIC.
 */
static void __init vic_register(void __iomem *base, void __iomem *base1,
				unsigned int parent_irq,
				unsigned int irq,
				u32 valid_sources, u32 resume_sources,
				struct device_node *node)
{
	struct vic_device *v;
	int i;

	if (vic_id >= ARRAY_SIZE(vic_devices)) {
		pr_err("%s: too few VICs, increase CONFIG_ARM_VIC_NR\n",
		       __func__);
		return;
	}

	v = &vic_devices[vic_id];
	v->base = base;
	v->base1 = base1;
	v->valid_sources = valid_sources;
	v->resume_sources = resume_sources;
	vic_id++;

	if (parent_irq) {
		irq_set_handler_data(parent_irq, v);
		irq_set_chained_handler(parent_irq, vic_handle_irq_cascaded);
	}
	v->domain = irq_domain_add_simple(node, 60, irq,
					  &s5p4418_vic_irqdomain_ops, v);
	/* create an IRQ mapping for each valid IRQ */
	for (i = 0; i < 60; i++)
			irq_create_mapping(v->domain, i);

	/* If no base IRQ was passed, figure out our allocated base */
	if (irq)
		v->irq = irq;
	else
		v->irq = irq_find_mapping(v->domain, 0);
}

static void vic_ack_irq(struct irq_data *d)
{
	void __iomem *base = irq_data_get_irq_chip_data(d);
	unsigned int irq = d->hwirq;

	if (irq > 31)
		irq -= 32;
	writel(1 << irq, base + VIC_INT_ENABLE_CLEAR);
	/* moreover, clear the soft-triggered, in case it was the reason */
	writel(1 << irq, base + VIC_INT_SOFT_CLEAR);
}

static void vic_mask_irq(struct irq_data *d)
{
	void __iomem *base = irq_data_get_irq_chip_data(d);
	unsigned int irq = d->hwirq;

	if (irq > 31)
		irq -= 32;

	writel(1 << irq, base + VIC_INT_ENABLE_CLEAR);
}

static void vic_unmask_irq(struct irq_data *d)
{
	void __iomem *base = irq_data_get_irq_chip_data(d);
	unsigned int irq = d->hwirq;

	if (irq > 31)
		irq -= 32;
	writel(1 << irq, base + VIC_INT_ENABLE);
}

#if defined(CONFIG_PM)
static struct vic_device *vic_from_irq(unsigned int irq)
{
	struct vic_device *v = vic_devices;
	unsigned int base_irq = irq & ~31;
	int id;

	for (id = 0; id < vic_id; id++, v++) {
		if (v->irq == base_irq)
			return v;
	}

	return NULL;
}

static int vic_set_wake(struct irq_data *d, unsigned int on)
{
	struct vic_device *v = vic_from_irq(d->irq);
	unsigned int off = d->hwirq;
	u32 bit = 1 << off;

	if (!v)
		return -EINVAL;

	if (!(bit & v->resume_sources))
		return -EINVAL;

	if (on)
		v->resume_irqs |= bit;
	else
		v->resume_irqs &= ~bit;

	return 0;
}
#else
#define vic_set_wake NULL
#endif /* CONFIG_PM */

static struct irq_chip vic_chip = {
	.name		= "VIC",
	.irq_ack	= vic_ack_irq,
	.irq_mask	= vic_mask_irq,
	.irq_unmask	= vic_unmask_irq,
	.irq_set_wake	= vic_set_wake,
};

static void __init vic_disable(void __iomem *base)
{
	writel(0, base + VIC_INT_SELECT);
	writel(0, base + VIC_INT_ENABLE);
	writel(~0, base + VIC_INT_ENABLE_CLEAR);
	writel(0, base + VIC_ITCR);
	writel(~0, base + VIC_INT_SOFT_CLEAR);
}

static void __init vic_clear_interrupts(void __iomem *base)
{
	unsigned int i;

	writel(0, base + VIC_PL190_VECT_ADDR);
	for (i = 0; i < 19; i++) {
		unsigned int value;

		value = readl(base + VIC_PL190_VECT_ADDR);
		writel(value, base + VIC_PL190_VECT_ADDR);
	}
}

/*
 * The PL190 cell from ARM has been modified by ST to handle 64 interrupts.
 * The original cell has 32 interrupts, while the modified one has 64,
 * replocating two blocks 0x00..0x1f in 0x20..0x3f. In that case
 * the probe function is called twice, with base set to offset 000
 *  and 020 within the page. We call this "second block".
 */
static void __init vic_init_st(void __iomem *base, void __iomem *base1,
			       unsigned int irq_start,
			       u32 vic_sources, struct device_node *node)
{
	unsigned int i;
	int vic_2nd_block = ((unsigned long)base & ~PAGE_MASK) != 0;

	/* Disable all interrupts initially. */
	vic_disable(base);
	vic_disable(base1);

	/*
	 * Make sure we clear all existing interrupts. The vector registers
	 * in this cell are after the second block of general registers,
	 * so we can address them using standard offsets, but only from
	 * the second base address, which is 0x20 in the page
	 */
	if (vic_2nd_block) {
		vic_clear_interrupts(base);
		vic_clear_interrupts(base1);

		/* ST has 16 vectors as well, but we don't enable them by now */
		for (i = 0; i < 16; i++) {
			void __iomem *reg = base + VIC_VECT_CNTL0 + (i * 4);

			writel(0, reg);
		}

		writel(32, base + VIC_PL190_DEF_VECT_ADDR);

		/* ST has 16 vectors as well, but we don't enable them by now */
		for (i = 0; i < 16; i++) {
			void __iomem *reg = base1 + VIC_VECT_CNTL0 + (i * 4);

			writel(0, reg);
		}

		writel(32, base1 + VIC_PL190_DEF_VECT_ADDR);


	}

	vic_register(base, base1, 0, irq_start, vic_sources, 0, node);
}

void __init __s5p4418_vic_init(void __iomem *base, void __iomem *base1,
			       int parent_irq, int irq_start,
			       u32 vic_sources, u32 resume_sources,
			       struct device_node *node)
{
	unsigned int i;
	u32 cellid = 0;
	enum amba_vendor vendor;

	/* Identify which VIC cell this one is, by reading the ID */
	for (i = 0; i < 4; i++) {
		void __iomem *addr;

		addr = (void __iomem *)((u32)base & PAGE_MASK)
			+ 0xfe0 + (i * 4);
		cellid |= (readl(addr) & 0xff) << (8 * i);
	}
	vendor = (cellid >> 12) & 0xff;
	pr_info("VIC @%p: id 0x%08x, vendor 0x%02x\n",
	       base, cellid, vendor);

	switch (vendor) {
	case AMBA_VENDOR_ST:
		vic_init_st(base, base1, irq_start, vic_sources, node);
		return;
	default:
		pr_warn("VIC: unknown vendor, continuing anyways\n");
		/* fall through */
	case AMBA_VENDOR_ARM:
		break;
	}

	/* Disable all interrupts initially. */
	vic_disable(base);
	vic_disable(base1);

	/* Make sure we clear all existing interrupts */
	vic_clear_interrupts(base);
	vic_clear_interrupts(base1);

	vic_init2(base);
	vic_init2(base1);

	vic_register(base, base1, parent_irq, irq_start, vic_sources,
		     resume_sources, node);
}

/**
 * vic_init() - initialise a vectored interrupt controller
 * @base: iomem base address
 * @irq_start: starting interrupt number, must be muliple of 32
 * @vic_sources: bitmask of interrupt sources to allow
 * @resume_sources: bitmask of interrupt sources to allow for resume
 */
void __init s5p4418_vic_init(void __iomem *base, void __iomem *base1,
			     unsigned int irq_start,
			     u32 vic_sources, u32 resume_sources)
{
	__s5p4418_vic_init(base, base1, 0, irq_start, vic_sources,
			   resume_sources, NULL);
}

/* GIC Definition */

union gic_base {
	void __iomem *common_base;
	void __percpu * __iomem *percpu_base;
};

struct gic_chip_data {
	union gic_base dist_base;
	union gic_base cpu_base;
#ifdef CONFIG_CPU_PM
	u32 saved_spi_enable[DIV_ROUND_UP(1020, 32)];
	u32 saved_spi_conf[DIV_ROUND_UP(1020, 16)];
	u32 saved_spi_target[DIV_ROUND_UP(1020, 4)];
	u32 __percpu *saved_ppi_enable;
	u32 __percpu *saved_ppi_conf;
#endif
	struct irq_domain *domain;
	unsigned int gic_irqs;
#ifdef CONFIG_GIC_NON_BANKED
	void __iomem *(*get_base)(union gic_base *);
#endif
};

static DEFINE_RAW_SPINLOCK(irq_controller_lock);

/*
 * The GIC mapping of CPU interfaces does not necessarily match
 * the logical CPU numbering.  Let's use a mapping as returned
 * by the GIC itself.
 */
#define NR_GIC_CPU_IF 8
static u8 gic_cpu_map[NR_GIC_CPU_IF] __read_mostly;

#ifndef MAX_GIC_NR
#define MAX_GIC_NR	1
#endif

static struct gic_chip_data gic_data[MAX_GIC_NR] __read_mostly;


#ifdef CONFIG_GIC_NON_BANKED
static void __iomem *gic_get_percpu_base(union gic_base *base)
{
	return raw_cpu_read(*base->percpu_base);
}

static void __iomem *gic_get_common_base(union gic_base *base)
{
	return base->common_base;
}

static inline void __iomem *gic_data_dist_base(struct gic_chip_data *data)
{
	return data->get_base(&data->dist_base);
}

static inline void __iomem *gic_data_cpu_base(struct gic_chip_data *data)
{
	return data->get_base(&data->cpu_base);
}

static inline void gic_set_base_accessor(struct gic_chip_data *data,
					 void __iomem *(*f)(union gic_base *))
{
	data->get_base = f;
}
#else
#define gic_data_dist_base(d)	((d)->dist_base.common_base)
#define gic_data_cpu_base(d)	((d)->cpu_base.common_base)
#define gic_set_base_accessor(d, f)
#endif

static inline void __iomem *gic_dist_base(struct irq_data *d)
{
	struct gic_chip_data *gic_data = irq_data_get_irq_chip_data(d);

	return gic_data_dist_base(gic_data);
}

static inline void __iomem *gic_cpu_base(struct irq_data *d)
{
	struct gic_chip_data *gic_data = irq_data_get_irq_chip_data(d);

	return gic_data_cpu_base(gic_data);
}

static inline unsigned int gic_irq(struct irq_data *d)
{
	return d->hwirq;
}

/*
 * Routines to acknowledge, disable and enable interrupts
 */
static void gic_poke_irq(struct irq_data *d, u32 offset)
{
	u32 mask = 1 << (gic_irq(d) % 32);

	writel_relaxed(mask, gic_dist_base(d) + offset + (gic_irq(d) / 32) * 4);
}

static int gic_peek_irq(struct irq_data *d, u32 offset)
{
	u32 mask = 1 << (gic_irq(d) % 32);

	return !!(readl_relaxed(gic_dist_base(d) + offset +
				(gic_irq(d) / 32) * 4) & mask);
}

static void gic_mask_irq(struct irq_data *d)
{
	gic_poke_irq(d, GIC_DIST_ENABLE_CLEAR);
}

static void gic_unmask_irq(struct irq_data *d)
{
	gic_poke_irq(d, GIC_DIST_ENABLE_SET);
}

static void gic_eoi_irq(struct irq_data *d)
{
	writel_relaxed(gic_irq(d), gic_cpu_base(d) + GIC_CPU_EOI);
}

static int gic_irq_set_irqchip_state(struct irq_data *d,
				     enum irqchip_irq_state which, bool val)
{
	u32 reg;

	switch (which) {
	case IRQCHIP_STATE_PENDING:
		reg = val ? GIC_DIST_PENDING_SET : GIC_DIST_PENDING_CLEAR;
		break;

	case IRQCHIP_STATE_ACTIVE:
		reg = val ? GIC_DIST_ACTIVE_SET : GIC_DIST_ACTIVE_CLEAR;
		break;

	case IRQCHIP_STATE_MASKED:
		reg = val ? GIC_DIST_ENABLE_CLEAR : GIC_DIST_ENABLE_SET;
		break;

	default:
		return -EINVAL;
	}

	gic_poke_irq(d, reg);
	return 0;
}

static int gic_irq_get_irqchip_state(struct irq_data *d,
				      enum irqchip_irq_state which, bool *val)
{
	switch (which) {
	case IRQCHIP_STATE_PENDING:
		*val = gic_peek_irq(d, GIC_DIST_PENDING_SET);
		break;

	case IRQCHIP_STATE_ACTIVE:
		*val = gic_peek_irq(d, GIC_DIST_ACTIVE_SET);
		break;

	case IRQCHIP_STATE_MASKED:
		*val = !gic_peek_irq(d, GIC_DIST_ENABLE_SET);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int gic_set_type(struct irq_data *d, unsigned int type)
{
	void __iomem *base = gic_dist_base(d);
	unsigned int gicirq = gic_irq(d);

	/* Interrupt configuration for SGIs can't be changed */
	if (gicirq < 16)
		return -EINVAL;

	/* SPIs have restrictions on the supported types */
	if (gicirq >= 32 && type != IRQ_TYPE_LEVEL_HIGH &&
			    type != IRQ_TYPE_EDGE_RISING)
		return -EINVAL;

	return gic_configure_irq(gicirq, type, base, NULL);
}

#ifdef CONFIG_SMP
static int gic_set_affinity(struct irq_data *d, const struct cpumask *mask_val,
			    bool force)
{
	void __iomem *reg = gic_dist_base(d) +
		GIC_DIST_TARGET + (gic_irq(d) & ~3);
	unsigned int cpu, shift = (gic_irq(d) % 4) * 8;
	u32 val, mask, bit;
	unsigned long flags;

	if (!force)
		cpu = cpumask_any_and(mask_val, cpu_online_mask);
	else
		cpu = cpumask_first(mask_val);

	if (cpu >= NR_GIC_CPU_IF || cpu >= nr_cpu_ids)
		return -EINVAL;

	raw_spin_lock_irqsave(&irq_controller_lock, flags);
	mask = 0xff << shift;
	bit = gic_cpu_map[cpu] << shift;
	val = readl_relaxed(reg) & ~mask;
	writel_relaxed(val | bit, reg);
	raw_spin_unlock_irqrestore(&irq_controller_lock, flags);

	return IRQ_SET_MASK_OK;
}
#endif

static void __exception_irq_entry gic_handle_irq(struct pt_regs *regs)
{
	u32 irqstat, irqnr;
	struct gic_chip_data *gic = &gic_data[0];
	void __iomem *cpu_base = gic_data_cpu_base(gic);

	do {
		irqstat = readl_relaxed(cpu_base + GIC_CPU_INTACK);
		irqnr = irqstat & GICC_IAR_INT_ID_MASK;

		if (likely(irqnr > 15 && irqnr < 1021)) {
			writel_relaxed(irqstat, cpu_base + GIC_CPU_EOI);
			if (irqnr == 31)
				vic_handle_irq(regs);
			else
				handle_domain_irq(gic->domain, irqnr, regs);

			break;
		}
		if (irqnr < 16) {
			writel_relaxed(irqstat, cpu_base + GIC_CPU_EOI);
#ifdef CONFIG_SMP
			handle_IPI(irqnr, regs);
#endif
			continue;
		}
		break;
	} while (1);
}

static struct irq_chip gic_chip = {
	.name			= "GIC",
	.irq_mask		= gic_mask_irq,
	.irq_unmask		= gic_unmask_irq,
	.irq_eoi		= gic_eoi_irq,
	.irq_set_type		= gic_set_type,
#ifdef CONFIG_SMP
	.irq_set_affinity	= gic_set_affinity,
#endif
	.irq_get_irqchip_state	= gic_irq_get_irqchip_state,
	.irq_set_irqchip_state	= gic_irq_set_irqchip_state,
};

void s5p4418_gic_cpu_config(void __iomem *base, void (*sync_access)(void))
{
	int i;

	/*
	 * Deal with the banked PPI and SGI interrupts - disable all
	 * PPI interrupts, ensure all SGI interrupts are enabled.
	 */
	writel_relaxed(0x5fff0000, base + GIC_DIST_ENABLE_CLEAR);
	writel_relaxed(0xa000ffff, base + GIC_DIST_ENABLE_SET);

	/*
	 * Set priority on PPI and SGI interrupts
	 */
	for (i = 0; i < 32; i += 4)
		writel_relaxed(GICD_INT_DEF_PRI_X4,
					base + GIC_DIST_PRI + i * 4 / 4);

	if (sync_access)
		sync_access();
}

static u8 gic_get_cpumask(struct gic_chip_data *gic)
{
	void __iomem *base = gic_data_dist_base(gic);
	u32 mask, i;

	for (i = mask = 0; i < 32; i += 4) {
		mask = readl_relaxed(base + GIC_DIST_TARGET + i);
		mask |= mask >> 16;
		mask |= mask >> 8;
		if (mask)
			break;
	}

	if (!mask && num_possible_cpus() > 1)
		pr_crit("GIC CPU mask not found - kernel will fail to boot.\n");

	return mask;
}

static void gic_cpu_if_up(void)
{
	void __iomem *cpu_base = gic_data_cpu_base(&gic_data[0]);
	u32 bypass = 0;

	/*
	* Preserve bypass disable bits to be written back later
	*/
	bypass = readl(cpu_base + GIC_CPU_CTRL);
	bypass &= GICC_DIS_BYPASS_MASK;

	writel_relaxed(bypass | GICC_ENABLE, cpu_base + GIC_CPU_CTRL);
}


static void __init gic_dist_init(struct gic_chip_data *gic)
{
	unsigned int i;
	u32 cpumask;
	unsigned int gic_irqs = gic->gic_irqs;
	void __iomem *base = gic_data_dist_base(gic);

	writel_relaxed(GICD_DISABLE, base + GIC_DIST_CTRL);

	/*
	 * Set all global interrupts to this CPU only.
	 */
	cpumask = gic_get_cpumask(gic);
	cpumask |= cpumask << 8;
	cpumask |= cpumask << 16;
	for (i = 32; i < gic_irqs; i += 4)
		writel_relaxed(cpumask, base + GIC_DIST_TARGET + i * 4 / 4);

	gic_dist_config(base, gic_irqs, NULL);

	writel_relaxed(GICD_ENABLE, base + GIC_DIST_CTRL);
}

static void gic_cpu_init(struct gic_chip_data *gic)
{
	void __iomem *dist_base = gic_data_dist_base(gic);
	void __iomem *base = gic_data_cpu_base(gic);
	unsigned int cpu_mask, cpu = smp_processor_id();
	int i;

	/*
	 * Get what the GIC says our CPU mask is.
	 */
	BUG_ON(cpu >= NR_GIC_CPU_IF);
	cpu_mask = gic_get_cpumask(gic);
	gic_cpu_map[cpu] = cpu_mask;

	/*
	 * Clear our mask from the other map entries in case they're
	 * still undefined.
	 */
	for (i = 0; i < NR_GIC_CPU_IF; i++)
		if (i != cpu)
			gic_cpu_map[i] &= ~cpu_mask;

	s5p4418_gic_cpu_config(dist_base, NULL);

	writel_relaxed(GICC_INT_PRI_THRESHOLD, base + GIC_CPU_PRIMASK);
	gic_cpu_if_up();
}

#ifdef CONFIG_CPU_PM
/*
 * Saves the GIC distributor registers during suspend or idle.  Must be called
 * with interrupts disabled but before powering down the GIC.  After calling
 * this function, no interrupts will be delivered by the GIC, and another
 * platform-specific wakeup source must be enabled.
 */
static void gic_dist_save(unsigned int gic_nr)
{
	unsigned int gic_irqs;
	void __iomem *dist_base;
	int i;

	if (gic_nr >= MAX_GIC_NR)
		BUG();

	gic_irqs = gic_data[gic_nr].gic_irqs;
	dist_base = gic_data_dist_base(&gic_data[gic_nr]);

	if (!dist_base)
		return;

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 16); i++)
		gic_data[gic_nr].saved_spi_conf[i] =
			readl_relaxed(dist_base + GIC_DIST_CONFIG + i * 4);

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 4); i++)
		gic_data[gic_nr].saved_spi_target[i] =
			readl_relaxed(dist_base + GIC_DIST_TARGET + i * 4);

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 32); i++)
		gic_data[gic_nr].saved_spi_enable[i] =
			readl_relaxed(dist_base + GIC_DIST_ENABLE_SET + i * 4);
}

/*
 * Restores the GIC distributor registers during resume or when coming out of
 * idle.  Must be called before enabling interrupts.  If a level interrupt
 * that occurred while the GIC was suspended is still present, it will be
 * handled normally, but any edge interrupts that occurred will not be seen by
 * the GIC and need to be handled by the platform-specific wakeup source.
 */
static void gic_dist_restore(unsigned int gic_nr)
{
	unsigned int gic_irqs;
	unsigned int i;
	void __iomem *dist_base;

	if (gic_nr >= MAX_GIC_NR)
		BUG();

	gic_irqs = gic_data[gic_nr].gic_irqs;
	dist_base = gic_data_dist_base(&gic_data[gic_nr]);

	if (!dist_base)
		return;

	writel_relaxed(GICD_DISABLE, dist_base + GIC_DIST_CTRL);

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 16); i++)
		writel_relaxed(gic_data[gic_nr].saved_spi_conf[i],
			dist_base + GIC_DIST_CONFIG + i * 4);

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 4); i++)
		writel_relaxed(GICD_INT_DEF_PRI_X4,
			dist_base + GIC_DIST_PRI + i * 4);

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 4); i++)
		writel_relaxed(gic_data[gic_nr].saved_spi_target[i],
			dist_base + GIC_DIST_TARGET + i * 4);

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 32); i++)
		writel_relaxed(gic_data[gic_nr].saved_spi_enable[i],
			dist_base + GIC_DIST_ENABLE_SET + i * 4);

	writel_relaxed(GICD_ENABLE, dist_base + GIC_DIST_CTRL);
}

static void gic_cpu_save(unsigned int gic_nr)
{
	int i;
	u32 *ptr;
	void __iomem *dist_base;
	void __iomem *cpu_base;

	if (gic_nr >= MAX_GIC_NR)
		BUG();

	dist_base = gic_data_dist_base(&gic_data[gic_nr]);
	cpu_base = gic_data_cpu_base(&gic_data[gic_nr]);

	if (!dist_base || !cpu_base)
		return;

	ptr = raw_cpu_ptr(gic_data[gic_nr].saved_ppi_enable);
	for (i = 0; i < DIV_ROUND_UP(32, 32); i++)
		ptr[i] = readl_relaxed(dist_base + GIC_DIST_ENABLE_SET + i * 4);

	ptr = raw_cpu_ptr(gic_data[gic_nr].saved_ppi_conf);
	for (i = 0; i < DIV_ROUND_UP(32, 16); i++)
		ptr[i] = readl_relaxed(dist_base + GIC_DIST_CONFIG + i * 4);

}

static void gic_cpu_restore(unsigned int gic_nr)
{
	int i;
	u32 *ptr;
	void __iomem *dist_base;
	void __iomem *cpu_base;

	if (gic_nr >= MAX_GIC_NR)
		BUG();

	dist_base = gic_data_dist_base(&gic_data[gic_nr]);
	cpu_base = gic_data_cpu_base(&gic_data[gic_nr]);

	if (!dist_base || !cpu_base)
		return;

	ptr = raw_cpu_ptr(gic_data[gic_nr].saved_ppi_enable);
	for (i = 0; i < DIV_ROUND_UP(32, 32); i++)
		writel_relaxed(ptr[i], dist_base + GIC_DIST_ENABLE_SET + i * 4);

	ptr = raw_cpu_ptr(gic_data[gic_nr].saved_ppi_conf);
	for (i = 0; i < DIV_ROUND_UP(32, 16); i++)
		writel_relaxed(ptr[i], dist_base + GIC_DIST_CONFIG + i * 4);

	for (i = 0; i < DIV_ROUND_UP(32, 4); i++)
		writel_relaxed(GICD_INT_DEF_PRI_X4,
					dist_base + GIC_DIST_PRI + i * 4);

	writel_relaxed(GICC_INT_PRI_THRESHOLD, cpu_base + GIC_CPU_PRIMASK);
	gic_cpu_if_up();
}

static int gic_notifier(struct notifier_block *self, unsigned long cmd,	void *v)
{
	int i;

	for (i = 0; i < MAX_GIC_NR; i++) {
#ifdef CONFIG_GIC_NON_BANKED
		/* Skip over unused GICs */
		if (!gic_data[i].get_base)
			continue;
#endif
		switch (cmd) {
		case CPU_PM_ENTER:
			gic_cpu_save(i);
			break;
		case CPU_PM_ENTER_FAILED:
		case CPU_PM_EXIT:
			gic_cpu_restore(i);
			break;
		case CPU_CLUSTER_PM_ENTER:
			gic_dist_save(i);
			break;
		case CPU_CLUSTER_PM_ENTER_FAILED:
		case CPU_CLUSTER_PM_EXIT:
			gic_dist_restore(i);
			break;
		}
	}

	return NOTIFY_OK;
}

static struct notifier_block gic_notifier_block = {
	.notifier_call = gic_notifier,
};

static void __init gic_pm_init(struct gic_chip_data *gic)
{
	gic->saved_ppi_enable = __alloc_percpu(DIV_ROUND_UP(32, 32) * 4,
		sizeof(u32));
	BUG_ON(!gic->saved_ppi_enable);

	gic->saved_ppi_conf = __alloc_percpu(DIV_ROUND_UP(32, 16) * 4,
		sizeof(u32));
	BUG_ON(!gic->saved_ppi_conf);

	if (gic == &gic_data[0])
		cpu_pm_register_notifier(&gic_notifier_block);
}
#else
static void __init gic_pm_init(struct gic_chip_data *gic)
{
}
#endif

#ifdef CONFIG_SMP
static void gic_raise_softirq(const struct cpumask *mask, unsigned int irq)
{
	int cpu;
	unsigned long flags, map = 0;

	raw_spin_lock_irqsave(&irq_controller_lock, flags);

	/* Convert our logical CPU mask into a physical one. */
	for_each_cpu(cpu, mask)
		map |= gic_cpu_map[cpu];

	/*
	 * Ensure that stores to Normal memory are visible to the
	 * other CPUs before they observe us issuing the IPI.
	 */
	dmb(ishst);

	/* this always happens on GIC0 */
	writel_relaxed(map << 16 | irq,
		       gic_data_dist_base(&gic_data[0]) + GIC_DIST_SOFTINT);

	raw_spin_unlock_irqrestore(&irq_controller_lock, flags);
}
#endif

#ifdef CONFIG_BL_SWITCHER
/*
 * gic_send_sgi - send a SGI directly to given CPU interface number
 *
 * cpu_id: the ID for the destination CPU interface
 * irq: the IPI number to send a SGI for
 */
void gic_send_sgi(unsigned int cpu_id, unsigned int irq)
{
	BUG_ON(cpu_id >= NR_GIC_CPU_IF);
	cpu_id = 1 << cpu_id;
	/* this always happens on GIC0 */
	writel_relaxed((cpu_id << 16) | irq, gic_data_dist_base(&gic_data[0]) +
		       GIC_DIST_SOFTINT);
}

/*
 * gic_get_cpu_id - get the CPU interface ID for the specified CPU
 *
 * @cpu: the logical CPU number to get the GIC ID for.
 *
 * Return the CPU interface ID for the given logical CPU number,
 * or -1 if the CPU number is too large or the interface ID is
 * unknown (more than one bit set).
 */
int gic_get_cpu_id(unsigned int cpu)
{
	unsigned int cpu_bit;

	if (cpu >= NR_GIC_CPU_IF)
		return -1;
	cpu_bit = gic_cpu_map[cpu];
	if (cpu_bit & (cpu_bit - 1))
		return -1;
	return __ffs(cpu_bit);
}

/*
 * gic_migrate_target - migrate IRQs to another CPU interface
 *
 * @new_cpu_id: the CPU target ID to migrate IRQs to
 *
 * Migrate all peripheral interrupts with a target matching the current CPU
 * to the interface corresponding to @new_cpu_id.  The CPU interface mapping
 * is also updated.  Targets to other CPU interfaces are unchanged.
 * This must be called with IRQs locally disabled.
 */
void gic_migrate_target(unsigned int new_cpu_id)
{
	unsigned int cur_cpu_id, gic_irqs, gic_nr = 0;
	void __iomem *dist_base;
	int i, ror_val, cpu = smp_processor_id();
	u32 val, cur_target_mask, active_mask;

	if (gic_nr >= MAX_GIC_NR)
		BUG();

	dist_base = gic_data_dist_base(&gic_data[gic_nr]);
	if (!dist_base)
		return;
	gic_irqs = gic_data[gic_nr].gic_irqs;

	cur_cpu_id = __ffs(gic_cpu_map[cpu]);
	cur_target_mask = 0x01010101 << cur_cpu_id;
	ror_val = (cur_cpu_id - new_cpu_id) & 31;

	raw_spin_lock(&irq_controller_lock);

	/* Update the target interface for this logical CPU */
	gic_cpu_map[cpu] = 1 << new_cpu_id;

	/*
	 * Find all the peripheral interrupts targeting the current
	 * CPU interface and migrate them to the new CPU interface.
	 * We skip DIST_TARGET 0 to 7 as they are read-only.
	 */
	for (i = 8; i < DIV_ROUND_UP(gic_irqs, 4); i++) {
		val = readl_relaxed(dist_base + GIC_DIST_TARGET + i * 4);
		active_mask = val & cur_target_mask;
		if (active_mask) {
			val &= ~active_mask;
			val |= ror32(active_mask, ror_val);
			writel_relaxed(val, dist_base + GIC_DIST_TARGET + i*4);
		}
	}

	raw_spin_unlock(&irq_controller_lock);

	/*
	 * Now let's migrate and clear any potential SGIs that might be
	 * pending for us (cur_cpu_id).  Since GIC_DIST_SGI_PENDING_SET
	 * is a banked register, we can only forward the SGI using
	 * GIC_DIST_SOFTINT.  The original SGI source is lost but Linux
	 * doesn't use that information anyway.
	 *
	 * For the same reason we do not adjust SGI source information
	 * for previously sent SGIs by us to other CPUs either.
	 */
	for (i = 0; i < 16; i += 4) {
		int j;

		val = readl_relaxed(dist_base + GIC_DIST_SGI_PENDING_SET + i);
		if (!val)
			continue;
		writel_relaxed(val, dist_base + GIC_DIST_SGI_PENDING_CLEAR + i);
		for (j = i; j < i + 4; j++) {
			if (val & 0xff)
				writel_relaxed((1 << (new_cpu_id + 16)) | j,
						dist_base + GIC_DIST_SOFTINT);
			val >>= 8;
		}
	}
}

/*
 * gic_get_sgir_physaddr - get the physical address for the SGI register
 *
 * REturn the physical address of the SGI register to be used
 * by some early assembly code when the kernel is not yet available.
 */
static unsigned long gic_dist_physaddr;

unsigned long gic_get_sgir_physaddr(void)
{
	if (!gic_dist_physaddr)
		return 0;
	return gic_dist_physaddr + GIC_DIST_SOFTINT;
}

void __init gic_init_physaddr(struct device_node *node)
{
	struct resource res;

	if (of_address_to_resource(node, 0, &res) == 0) {
		gic_dist_physaddr = res.start;
		pr_info("GIC physical location is %#lx\n", gic_dist_physaddr);
	}
}

#else
#define gic_init_physaddr(node)  do { } while (0)
#endif

static int gic_irq_domain_map(struct irq_domain *d, unsigned int irq,
				irq_hw_number_t hw)
{
	if (hw < 32) {
		irq_set_percpu_devid(irq);
		irq_domain_set_info(d, irq, hw, &gic_chip, d->host_data,
				    handle_percpu_devid_irq, NULL, NULL);
		set_irq_flags(irq, IRQF_VALID | IRQF_NOAUTOEN);
	} else {
		irq_domain_set_info(d, irq, hw, &gic_chip, d->host_data,
				    handle_fasteoi_irq, NULL, NULL);
		set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
	}
	return 0;
}

static void gic_irq_domain_unmap(struct irq_domain *d, unsigned int irq)
{
}

static int gic_irq_domain_xlate(struct irq_domain *d,
				struct device_node *controller,
				const u32 *intspec, unsigned int intsize,
				unsigned long *out_hwirq,
				unsigned int *out_type)
{
	unsigned long ret = 0;

	if (d->of_node != controller)
		return -EINVAL;
	if (intsize < 3)
		return -EINVAL;

	/* Get the interrupt number and add 16 to skip over SGIs */
	*out_hwirq = intspec[1] + 16;

	/* For SPIs, we need to add 16 more to get the GIC irq ID number */
	if (!intspec[0])
		*out_hwirq += 16;

	*out_type = intspec[2] & IRQ_TYPE_SENSE_MASK;

	return ret;
}

#ifdef CONFIG_SMP
static int gic_secondary_init(struct notifier_block *nfb, unsigned long action,
			      void *hcpu)
{
	if (action == CPU_STARTING || action == CPU_STARTING_FROZEN)
		gic_cpu_init(&gic_data[0]);
	return NOTIFY_OK;
}

/*
 * Notifier for enabling the GIC CPU interface. Set an arbitrarily high
 * priority because the GIC needs to be up before the ARM generic timers.
 */
static struct notifier_block gic_cpu_notifier = {
	.notifier_call = gic_secondary_init,
	.priority = 100,
};
#endif

static int gic_irq_domain_alloc(struct irq_domain *domain, unsigned int virq,
				unsigned int nr_irqs, void *arg)
{
	int i, ret;
	irq_hw_number_t hwirq;
	unsigned int type = IRQ_TYPE_NONE;
	struct of_phandle_args *irq_data = arg;

	ret = gic_irq_domain_xlate(domain, irq_data->np, irq_data->args,
				   irq_data->args_count, &hwirq, &type);
	if (ret)
		return ret;

	for (i = 0; i < nr_irqs; i++)
		gic_irq_domain_map(domain, virq + i, hwirq + i);

	return 0;
}

static const struct irq_domain_ops gic_irq_domain_hierarchy_ops = {
	.xlate = gic_irq_domain_xlate,
	.alloc = gic_irq_domain_alloc,
	.free = irq_domain_free_irqs_top,
};

static const struct irq_domain_ops gic_irq_domain_ops = {
	.map = gic_irq_domain_map,
	.unmap = gic_irq_domain_unmap,
	.xlate = gic_irq_domain_xlate,
};

void s5p4418_gic_set_irqchip_flags(unsigned long flags)
{
	gic_chip.flags |= flags;
}

void __init s5p4418_gic_init_bases(unsigned int gic_nr, int irq_start,
			   void __iomem *dist_base, void __iomem *cpu_base,
			   u32 percpu_offset, struct device_node *node)
{
	irq_hw_number_t hwirq_base;
	struct gic_chip_data *gic;
	int gic_irqs, irq_base, i;

	BUG_ON(gic_nr >= MAX_GIC_NR);

	gic = &gic_data[gic_nr];
#ifdef CONFIG_GIC_NON_BANKED
	if (percpu_offset) { /* Frankein-GIC without banked registers... */
		unsigned int cpu;

		gic->dist_base.percpu_base = alloc_percpu(void __iomem *);
		gic->cpu_base.percpu_base = alloc_percpu(void __iomem *);
		if (WARN_ON(!gic->dist_base.percpu_base ||
			    !gic->cpu_base.percpu_base)) {
			free_percpu(gic->dist_base.percpu_base);
			free_percpu(gic->cpu_base.percpu_base);
			return;
		}

		for_each_possible_cpu(cpu) {
			u32 mpidr = cpu_logical_map(cpu);
			u32 core_id = MPIDR_AFFINITY_LEVEL(mpidr, 0);
			unsigned long offset = percpu_offset * core_id;
			*per_cpu_ptr(gic->dist_base.percpu_base, cpu) =
				dist_base + offset;
			*per_cpu_ptr(gic->cpu_base.percpu_base, cpu) =
				cpu_base + offset;
		}

		gic_set_base_accessor(gic, gic_get_percpu_base);
	} else
#endif
	{			/* Normal, sane GIC... */
		WARN(percpu_offset,
		     "GIC_NON_BANKED not enabled, ignoring %08x offset!",
		     percpu_offset);
		gic->dist_base.common_base = dist_base;
		gic->cpu_base.common_base = cpu_base;
		gic_set_base_accessor(gic, gic_get_common_base);
	}

	/*
	 * Initialize the CPU interface map to all CPUs.
	 * It will be refined as each CPU probes its ID.
	 */
	for (i = 0; i < NR_GIC_CPU_IF; i++)
		gic_cpu_map[i] = 0xff;

	/*
	 * Find out how many interrupts are supported.
	 * The GIC only supports up to 1020 interrupt sources.
	 */
	gic_irqs = readl_relaxed(gic_data_dist_base(gic) + GIC_DIST_CTR) & 0x1f;
	gic_irqs = (gic_irqs + 1) * 32;
	if (gic_irqs > 1020)
		gic_irqs = 1020;
	gic->gic_irqs = gic_irqs;

	if (node) {		/* DT case */
		gic->domain = irq_domain_add_linear(node, gic_irqs,
						    &gic_irq_domain_hierarchy_ops,
						    gic);
	} else {		/* Non-DT case */
		/*
		 * For primary GICs, skip over SGIs.
		 * For secondary GICs, skip over PPIs, too.
		 */
		if (gic_nr == 0 && (irq_start & 31) > 0) {
			hwirq_base = 16;
			if (irq_start != -1)
				irq_start = (irq_start & ~31) + 16;
		} else {
			hwirq_base = 32;
		}

		gic_irqs -= hwirq_base; /* calculate # of irqs to allocate */

		irq_base = irq_alloc_descs(irq_start, 16, gic_irqs,
					   numa_node_id());
		if (IS_ERR_VALUE(irq_base)) {
			WARN(1, "Cannot allocate irq_descs @ IRQ%d, assuming pre-allocated\n",
			     irq_start);
			irq_base = irq_start;
		}

		gic->domain = irq_domain_add_legacy(node, gic_irqs, irq_base,
					hwirq_base, &gic_irq_domain_ops, gic);
	}

	if (WARN_ON(!gic->domain))
		return;

	if (gic_nr == 0) {
#ifdef CONFIG_SMP
		set_smp_cross_call(gic_raise_softirq);
		register_cpu_notifier(&gic_cpu_notifier);
#endif
		set_handle_irq(gic_handle_irq);
	}

	gic_dist_init(gic);
	gic_cpu_init(gic);
	gic_pm_init(gic);
}



int __init s5p4418_vic_of_init(struct device_node *node,
			       struct device_node *parent)
{
	void __iomem *regs0;
	void __iomem *regs1;
	u32 interrupt_mask = ~0;
	u32 wakeup_mask = ~0;

	regs0 = of_iomap(node, 0);
	regs1 = of_iomap(node, 1);
	if (WARN_ON(!regs0))
		return -EIO;
	if (WARN_ON(!regs0))
		return -EIO;

	of_property_read_u32(node, "valid-mask", &interrupt_mask);
	of_property_read_u32(node, "valid-wakeup-mask", &wakeup_mask);
	/*
	 * Passing 0 as first IRQ makes the simple domain allocate descriptors
	 */
	__s5p4418_vic_init(regs0, regs1, 0, 0, interrupt_mask,
			   wakeup_mask, node);

	return 0;
}
IRQCHIP_DECLARE(arm_pl190_vic, "nexell,s5p4418-vic", s5p4418_vic_of_init);

static int gic_cnt __initdata;

static int __init
s5p4418_gic_of_init(struct device_node *node, struct device_node *parent)
{
	void __iomem *cpu_base;
	void __iomem *dist_base;
	u32 percpu_offset;

	if (WARN_ON(!node))
		return -ENODEV;

	dist_base = of_iomap(node, 0);
	WARN(!dist_base, "unable to map gic dist registers\n");

	cpu_base = of_iomap(node, 1);
	WARN(!cpu_base, "unable to map gic cpu registers\n");

	if (of_property_read_u32(node, "cpu-offset", &percpu_offset))
		percpu_offset = 0;

	s5p4418_gic_init_bases(gic_cnt, -1, dist_base, cpu_base,
			       percpu_offset, node);
	gic_cnt++;

	return 0;
}
IRQCHIP_DECLARE(s5p4418_gic_400, "nexell,s5p4418-gic", s5p4418_gic_of_init);
#endif
