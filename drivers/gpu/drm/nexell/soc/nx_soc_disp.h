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
#ifndef _NX_SOC_DISP_H_
#define _NX_SOC_DISP_H_

/*
 *	Display hardware configs
 */
#define	RESET_ID_DISP_TOP	9
#define RESET_ID_DISPLAY	10

#define NUMBER_OF_DISPTOP_MODULE	1
#define PHY_BASEADDR_DISPLAYTOP_MODULE 0xC0101000
#define	PHY_BASEADDR_DISPTOP_LIST	\
		{ PHY_BASEADDR_DISPLAYTOP_MODULE }

#define	IRQ_OFFSET	32
#define IRQ_DPC_P    (IRQ_OFFSET + 33)
#define NUMBER_OF_MLC_MODULE 2
#define PHY_BASEADDR_MLC0	0xC0102000
#define PHY_BASEADDR_MLC1	0xC0102400

#define IRQ_DPC_S   (IRQ_OFFSET + 34)
#define NUMBER_OF_DPC_MODULE	2
#define PHY_BASEADDR_DPC0	0xC0102800
#define PHY_BASEADDR_DPC1	0xC0102C70

#define	PHY_BASEADDR_MLC_LIST	\
		{ PHY_BASEADDR_MLC0, PHY_BASEADDR_MLC1 }
#define	PHY_BASEADDR_DPC_LIST	\
		{ PHY_BASEADDR_DPC0, PHY_BASEADDR_DPC1 }

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

/*	the data output format. */
#define	DPC_FORMAT_RGB555		0  /* RGB555 Format */
#define	DPC_FORMAT_RGB565		1  /* RGB565 Format */
#define	DPC_FORMAT_RGB666		2  /* RGB666 Format */
#define	DPC_FORMAT_RGB888		3  /* RGB888 Format */
#define	DPC_FORMAT_MRGB555A		4  /* MRGB555A Format */
#define	DPC_FORMAT_MRGB555B		5  /* MRGB555B Format */
#define	DPC_FORMAT_MRGB565		6  /* MRGB565 Format */
#define	DPC_FORMAT_MRGB666		7  /* MRGB666 Format */
#define	DPC_FORMAT_MRGB888A		8  /* MRGB888A Format */
#define	DPC_FORMAT_MRGB888B		9  /* MRGB888B Format */
#define	DPC_FORMAT_CCIR656		10 /* ITU-R BT.656 / 601(8-bit) */
#define	DPC_FORMAT_CCIR601A		12 /* ITU-R BT.601A */
#define	DPC_FORMAT_CCIR601B		13 /* ITU-R BT.601B */
#define	DPC_FORMAT_4096COLOR	1  /* 4096 Color Format */
#define	DPC_FORMAT_16GRAY		3  /* 16 Level Gray Format */

/*	RGB layer pixel format. */
#define	MLC_RGBFMT_R5G6B5		0x44320000	/* {R5,G6,B5 }. */
#define	MLC_RGBFMT_B5G6R5		0xC4320000  /* {B5,G6,R5 }. */
#define	MLC_RGBFMT_X1R5G5B5		0x43420000  /* {X1,R5,G5,B5}. */
#define	MLC_RGBFMT_X1B5G5R5		0xC3420000  /* {X1,B5,G5,R5}. */
#define	MLC_RGBFMT_X4R4G4B4		0x42110000  /* {X4,R4,G4,B4}. */
#define	MLC_RGBFMT_X4B4G4R4		0xC2110000	/* {X4,B4,G4,R4}. */
#define	MLC_RGBFMT_X8R3G3B2		0x41200000	/* {X8,R3,G3,B2}. */
#define	MLC_RGBFMT_X8B3G3R2		0xC1200000	/* {X8,B3,G3,R2}. */
#define	MLC_RGBFMT_A1R5G5B5		0x33420000	/* {A1,R5,G5,B5}. */
#define	MLC_RGBFMT_A1B5G5R5		0xB3420000	/* {A1,B5,G5,R5}. */
#define	MLC_RGBFMT_A4R4G4B4		0x22110000	/* {A4,R4,G4,B4}. */
#define	MLC_RGBFMT_A4B4G4R4		0xA2110000	/* {A4,B4,G4,R4}. */
#define	MLC_RGBFMT_A8R3G3B2		0x11200000	/* {A8,R3,G3,B2}. */
#define	MLC_RGBFMT_A8B3G3R2		0x91200000	/* {A8,B3,G3,R2}. */
#define	MLC_RGBFMT_R8G8B8		0x46530000	/* {R8,G8,B8 }. */
#define	MLC_RGBFMT_B8G8R8		0xC6530000	/* {B8,G8,R8 }. */
#define	MLC_RGBFMT_X8R8G8B8		0x46530000	/* {X8,R8,G8,B8}. */
#define	MLC_RGBFMT_X8B8G8R8		0xC6530000	/* {X8,B8,G8,R8}. */
#define	MLC_RGBFMT_A8R8G8B8		0x06530000	/* {A8,R8,G8,B8}. */
#define	MLC_RGBFMT_A8B8G8R8		0x86530000	/* {A8,B8,G8,R8}.  */

/*	the data output order in case of ITU-R BT.656 / 601. */
#define	DPC_YCORDER_CbYCrY		0	/* Cb, Y, Cr, Y */
#define	DPC_YCORDER_CrYCbY		1	/* Cr, Y, Cb, Y */
#define	DPC_YCORDER_YCbYCr		2	/* Y, Cb, Y, Cr */
#define	DPC_YCORDER_YCrYCb		3	/* Y, Cr, Y, Cb */

/* the PAD output clock. */
#define	DPC_PADCLKSEL_VCLK		0	/* VCLK */
#define	DPC_PADCLKSEL_VCLK2		1	/* VCLK2 */

#endif
