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
#include <linux/soc/nexell/nxs_function.h>

static const char *nxs_function_str[] = {
	"NONE",
	"DMAR",
	"DMAW",
	"VIP_CLIPPER",
	"VIP_DECIMATOR",
	"MIPI_CSI",
	"ISP2DISP",
	"CROPPER",
	"MULTI_TAP",
	"SCALER_4096",
	"SCALER_5376",
	"MLC_BOTTOM",
	"MLC_BLENDER",
	"HUE",
	"GAMMA",
	"FIFO",
	"MAPCONV",
	"TPGEN",
	"DISP2ISP",
	"CSC",
	"DPC",
	"LVDS",
	"MIPI_DSI",
	"HDMI",
	"INVALID",
};

inline const char *nxs_function_to_str(int function)
{
	if (function < NXS_FUNCTION_MAX)
		return nxs_function_str[function];
	return NULL;
}

