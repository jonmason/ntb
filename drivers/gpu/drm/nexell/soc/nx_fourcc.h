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
#ifndef _NX_FOURCC_H_
#define _NX_FOURCC_H_

/*
 * Macro: FOURCC for video
 */
#ifndef MAKEFOURCC
#define MAKEFOURCC(a, b, c, d) \
		(((u_int32_t)a) |  \
		(((u_int32_t)b) <<  8) | \
		(((u_int32_t)c) << 16) | \
		(((u_int32_t)d) << 24))
#endif

/*
 * Specific Pixel Format FourCC for NX series
 */

/* YCbCr 420 */
#define FOURCC_MVS0		MAKEFOURCC('M', 'V', 'S', '0')

/* YCbCr 422 */
#define FOURCC_MVS2		MAKEFOURCC('M', 'V', 'S', '2')

/* YCbCr 444 */
#define FOURCC_MVS4		MAKEFOURCC('M', 'V', 'S', '4')

/* Non separated YCbCr 422 */
#define FOURCC_MVN2		MAKEFOURCC('M', 'V', 'N', '2')

/*
 * General Pixel Format FourCC
 */

/* WMV media format   (nonseperated 420 format - Y,Cr,Cb)	 */
#define FOURCC_YV12		MAKEFOURCC('Y', 'V', '1', '2')

/* MPEG1 media format (nonseperated 422 format - Y,Cb,Y,Cr)*/
#define FOURCC_YUY2		MAKEFOURCC('Y', 'U', 'Y', '2')

/* Duplicated of YUY2 (nonseperated 422 format - Y,Cb,Y,Cr) */
#define FOURCC_YUYV		MAKEFOURCC('Y', 'U', 'Y', 'V')

#endif
