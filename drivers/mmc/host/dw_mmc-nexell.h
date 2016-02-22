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

#ifndef _DW_MMC_NEXELL_H_
#define _DW_MMC_NEXELL_H_

#define SDMMC_CLKCTRL			0x114

#define NX_SDMMC_CLK_DIV		2

#define NX_MMC_CLK_DELAY(x, y, a, b)	(((x & 0xFF) << 0) |\
					((y & 0x03) << 16) |\
					((a & 0xFF) << 8)  |\
					((b & 0x03) << 24))
#endif /* _DW_MMC_NEXELL_H_ */
