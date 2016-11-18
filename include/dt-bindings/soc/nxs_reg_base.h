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

#ifndef _DT_BINDINGS_NXS_REG_BASE_H
#define _DT_BINDINGS_NXS_REG_BASE_H

/* #define NXS_BASE_ADDR		0x20c0 */
/* fpga */
// #define NXS_REGBASE(OFFSET)		0x8004 ## OFFSET
#define NXS_REGBASE(OFFSET)		0x20c0 ## OFFSET

/* DMAW */
#define NXS_REGSIZE_DMAW		0x200
#define NXS_REGBASE_DMAW(index)		(NXS_REGBASE(5000) + \
					 (NXS_REGSIZE_DMAW * index))

#define NXS_REGSIZE_DMAR		0x200
#define NXS_REGBASE_DMAR(index)		(NXS_REGBASE(1800) + \
					 (NXS_REGSIZE_DMAR * index))

#define VIP_REGBASE			0x20c40000
#define VIP_PRESCALER_REGBASE		0x20c50000

#define NXS_REGSIZE_VIP			0x200
#define NXS_REGSIZE_VIP_PRESCALER	0x100

#define NXS_REGBASE_VIP(index)		(VIP_REGBASE + \
					 (NXS_REGSIZE_VIP * index))
#define NXS_REGBASE_VIP_PRESCALER(index) (VIP_PRESCALER_REGBASE + \
					  (NXS_REGSIZE_VIP * index))

#define NXS_REGSIZE_MIPICSI		0x10000
#define NXS_REGBASE_MIPICSI(index)	(0x20e20000 + \
					 (NXS_REGSIZE_MIPICSI * index))

#define NXS_REGSIZE_MIPIDSI		0x30000
#define NXS_REGBASE_MIPIDSI(index)	(0x20e50000 + \
					 (NXS_REGSIZE_MIPIDSI * index))

/* TODO ISP2DISP */
#if 0
#define NXS_REGSIZE_ISP2DISP		0x200
#define NXS_REGBASE_ISP2DISP(index)	(NXS_REGBASE(5000) + \
					 (NXS_REGSIZE_ISP2DISP * index))
#endif

#define NXS_REGSIZE_CROPPER		0x20
#define NXS_REGBASE_CROPPER(index)	(NXS_REGBASE(0a00) + \
					 (NXS_REGSIZE_CROPPER * index))

#define NXS_REGSIZE_MULTITAP		0x20
#define NXS_REGBASE_MULTITAP(index)	(NXS_REGBASE(1500) + \
					 (NXS_REGSIZE_MULTITAP * index))

#define NXS_REGSIZE_SCALER		0x300
#define NXS_REGBASE_SCALER(index)	(NXS_REGBASE(0c00) + \
					 (NXS_REGSIZE_SCALER * index))

#define NXS_REGSIZE_MLC_BOTTOM		0x20
#define NXS_REGBASE_MLC_BOTTOM(index)	(NXS_REGBASE(4000) + \
					 (NXS_REGSIZE_MLC_BOTTOM * index))

#define NXS_REGSIZE_TPGEN		0x30
#define NXS_REGBASE_TPGEN(index)	(NXS_REGBASE(3e00) + \
					 (NXS_REGSIZE_TPGEN * index))

#define NXS_REGSIZE_MLC_BLENDER		0x80
#define NXS_REGBASE_MLC_BLENDER(index)	(NXS_REGBASE(4080) + \
					 (NXS_REGSIZE_MLC_BLENDER * index))

#define NXS_REGSIZE_HUE			0x80
#define NXS_REGBASE_HUE(index)		(NXS_REGBASE(1580) + \
					 (NXS_REGSIZE_HUE * index))

#define NXS_REGSIZE_GAMMA		0x80
#define NXS_REGBASE_GAMMA(index)	(NXS_REGBASE(1680) + \
					 (NXS_REGSIZE_GAMMA * index))

#define NXS_REGSIZE_FIFO		0x10
#define NXS_REGBASE_FIFO(index)		(NXS_REGBASE(3d00) + \
					 (NXS_REGSIZE_FIFO * index))

#define NXS_REGSIZE_CSC			0x40
#define NXS_REGBASE_CSC(index)		(NXS_REGBASE(0200) + \
					 (NXS_REGSIZE_CSC * index))

#define NXS_REGSIZE_DPC			0x20000
#define NXS_REGBASE_DPC(index)		(0x20c80000 + \
					 (NXS_REGSIZE_DPC * index))

#define NXS_REGSIZE_LVDS		0x10000
#define NXS_REGBASE_LVDS(index)		(0x20cc0000 + \
					 (NXS_REGSIZE_LVDS * index * 2))

#define NXS_REGSIZE_MAPCONV		0x10000
#define NXS_REGBASE_MAPCONV(index)	(0x20cd0000 + \
					 (NXS_REGSIZE_MAPCONV * index))

#define NXS_REGSIZE_HDMI		0x140000
#define NXS_REGBASE_HDMI(index)		(0x20ec0000 + \
					 (NXS_REGSIZE_HDMI * index))

#endif
