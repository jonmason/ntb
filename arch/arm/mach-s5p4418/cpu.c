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
#include <asm/mach/arch.h>

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
	.init_machine	= cpu_init_machine,
	.dt_compat	= cpu_dt_compat,
MACHINE_END

