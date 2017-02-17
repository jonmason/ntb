/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Seonghee, Kim <kshblue@nexell.co.kr>
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

#ifndef __NX_VPU_GDI_H__
#define	__NX_VPU_GDI_H__

int SetTiledMapType(int mapType, int stride, int interleave);
int ConfigEncSecAXI(int codStd, struct sec_axi_info *sa, int width,
	int height, uint32_t sramAddr, uint32_t sramSize);
int ConfigDecSecAXI(int codStd, struct sec_axi_info *sa, int width,
	int height, uint32_t sramAddr, uint32_t sramSize);
unsigned int MaverickCache2Config(int decoder, int interleave, int bypass,
	int burst, int merge, int mapType, int wayshape);

#endif		/*__NX_VPU_GDI_H__ */
