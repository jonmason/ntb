/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongkeun, Choi <jkchoi@nexell.co.kr>
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

#include <linux/types.h>
#include <linux/io.h>

#include "s5pxx18_soc_tvout.h"

static struct {
	struct nx_dpc_register_set *pregister;
} __g_module_variables[NUMBER_OF_DPC_MODULE] = { { NULL,},};

void nx_dpc_set_enable_with_interlace(u32 module_index, int enable, int rgbmode,
		       int use_ntscsync, int use_analog_output, int seavenable)
{
	register struct nx_dpc_register_set *pregister;
	u32 regvalue;

	pregister = __g_module_variables[module_index].pregister;
	regvalue = readl(&pregister->dpcctrl0) & 0x0efful;
	regvalue |= ((u32) enable << 15) | ((u32) use_ntscsync << 14) |
	    ((u32) seavenable << 8) | ((u32) use_analog_output << 13) |
	    ((u32) rgbmode << 12);

	regvalue |= (1<<9);

	writel((u32) regvalue, &pregister->dpcctrl0);
}
