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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_address.h>
#include <linux/io.h>

#define	NX_PIN_FN_SIZE	4
#define TIEOFF_REG_NUM 33

struct	nx_tieoff_registerset {
	u32	tieoffreg[TIEOFF_REG_NUM];
};

static struct nx_tieoff_registerset *nx_tieoff;


void nx_tieoff_set(u32 tieoff_index, u32 tieoff_value)
{
	u32 regindex, mask;
	u32 lsb, msb;
	u32 regval;

	u32 position;
	u32 BitWidth;

	position = tieoff_index & 0xffff;
	BitWidth = (tieoff_index>>16) & 0xffff;

	regindex	= position>>5;

	lsb = position & 0x1F;
	msb = lsb+BitWidth;

	if (msb > 32) {
		msb &= 0x1F;
		mask   = ~(0xffffffff<<lsb);
		regval = readl(&nx_tieoff->tieoffreg[regindex]) & mask;
		regval |= ((tieoff_value & ((1UL<<BitWidth)-1))<<lsb);
		writel(regval, &nx_tieoff->tieoffreg[regindex]);

		mask   = (0xffffffff<<msb);
		regval = readl(&nx_tieoff->tieoffreg[regindex+1]) & mask;
		regval |= ((tieoff_value & ((1UL<<BitWidth)-1))>>msb);
		writel(regval, &nx_tieoff->tieoffreg[regindex+1]);
	} else	{
		mask	= (0xffffffff<<msb) | (~(0xffffffff<<lsb));
		regval	= readl(&nx_tieoff->tieoffreg[regindex]) & mask;
		regval	|= ((tieoff_value & ((1UL<<BitWidth)-1))<<lsb);
		writel(regval, &nx_tieoff->tieoffreg[regindex]);
	}
}
EXPORT_SYMBOL_GPL(nx_tieoff_set);

u32 nx_tieoff_get(u32 tieoff_index)
{
	u32 regindex, mask;
	u32 lsb, msb;
	u32 regval;

	u32 position;
	u32 BitWidth;

	position = tieoff_index & 0xffff;
	BitWidth = (tieoff_index>>16) & 0xffff;

	regindex = position/32;
	lsb = position % 32;
	msb = lsb+BitWidth;

	if (msb > 32) {
		msb &= 0x1F;
		mask   = 0xffffffff<<lsb;
		regval = readl(&nx_tieoff->tieoffreg[regindex]) & mask;
		regval >>= lsb;

		mask   = ~(0xffffffff<<msb);
		regval |= ((readl(&nx_tieoff->tieoffreg[regindex+1]) & mask)
			  << (32-lsb));
	} else	{
		mask   = ~(0xffffffff<<msb) & (0xffffffff<<lsb);
		regval = readl(&nx_tieoff->tieoffreg[regindex]) & mask;
		regval >>= lsb;
	}
	return regval;
}
EXPORT_SYMBOL_GPL(nx_tieoff_get);

static const struct of_device_id nexell_tieoff_match[] = {
	{ .compatible = "nexell,tieoff", .data = NULL },
};

static int __init cpu_early_initcall_setup(void)
{
	const struct of_device_id *match;
	struct device_node *np;
	struct resource regs;
	struct device_node *child;
	const __be32 *list;
	u32 pins[32];
	int index = 0, size = 0;

	np = of_find_matching_node_and_match(NULL, nexell_tieoff_match, &match);
	if (!np) {
		regs.start = 0xc0011000;
		regs.end = 0xc0012000;
		regs.flags = IORESOURCE_MEM;
	} else {
		if (of_address_to_resource(np, 0, &regs) < 0) {
			pr_err("failed to get tieoff registers\n");
			return -ENXIO;
		}
	};

	nx_tieoff = ioremap_nocache(regs.start, resource_size(&regs));
	if (!nx_tieoff) {
		pr_err("failed to map tieoff registers\n");
		return -ENXIO;
	}

	np = of_find_node_by_name(NULL, "soc");
	if (!np) {
		pr_err("** WARNING: Not exist tieoff DTS for init Tieoff.**\n");
		return -EINVAL;
	}

	for_each_child_of_node(np, child) {
		list = of_get_property(child, "soc,tieoff", &size);
		size /= NX_PIN_FN_SIZE;
		if (!list)
			continue;

		for (index = 0; index < size; index++)
			pins[index] = be32_to_cpu(*list++);

		for (index = 0; size > index; index += 2)
			nx_tieoff_set(pins[index], pins[index+1]);

	}
	return 0;
}
early_initcall(cpu_early_initcall_setup);
