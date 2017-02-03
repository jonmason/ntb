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

#include <linux/of_platform.h>
#include <linux/reboot.h>
#include <asm/mach/arch.h>
#include <linux/io.h>
#include <linux/soc/nexell/sec_reg.h>

#define PHYS_BASE_CLKPWR	0xC0010000
#define ALIVE_GATE	0x800
#define PWR_CONT	0x224
#define PWR_MODE	0x228

#define SW_RESET_EN	(1<<3)
#define SW_RESET	(1<<12)


#ifdef CONFIG_SECURE_REG_ACCESS

#define S5P4418_L2C_BASE 0xcf000000
#define SEC_L2C_ID	NEXELL_L2C_SEC_ID

void l2c_write_sec(unsigned long val, unsigned reg)
{
	write_sec_reg_by_id((void __iomem *) S5P4418_L2C_BASE + reg, val,
			NEXELL_L2C_SEC_ID);
}
#endif

void s5p4418_reboot(enum reboot_mode mode, const char *cmd)
{
	static void __iomem *base;

	base = ioremap(PHYS_BASE_CLKPWR, 0x1000);

	__raw_writel(0, base + ALIVE_GATE);
	__raw_writel(SW_RESET_EN, base + PWR_CONT);
	__raw_writel(SW_RESET, base + PWR_MODE);

}

static void __init cpu_init_machine(void)
{
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

static const char * const cpu_dt_compat[] = {
	"nexell,s5p4418",
	NULL
};
/*------------------------------------------------------------------------------
 * Maintainer: Nexell Co., Ltd.
 */

DT_MACHINE_START(S5P4418, "s5p4418")
	.l2c_aux_val	= 0x7e470000,
	.l2c_aux_mask	= 0x81b8ffff,
	.init_machine	= cpu_init_machine,
	.dt_compat	= cpu_dt_compat,
	.restart	= s5p4418_reboot,
#ifdef CONFIG_SECURE_REG_ACCESS
	.l2c_write_sec	= l2c_write_sec,
#endif
MACHINE_END

