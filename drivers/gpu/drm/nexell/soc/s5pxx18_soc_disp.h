/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: junghyun, kim <jhkim@nexell.co.kr>
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
#ifndef _S5PXX18_SOC_DISP_H_
#define _S5PXX18_SOC_DISP_H_

/*
 *	Display Clock Control types
 */
enum nx_pclkmode {
	nx_pclkmode_dynamic = 0UL,
	nx_pclkmode_always	= 1UL
};

enum nx_bclkmode {
	nx_bclkmode_disable	= 0UL,
	nx_bclkmode_dynamic	= 2UL,
	nx_bclkmode_always	= 3UL
};

#endif
