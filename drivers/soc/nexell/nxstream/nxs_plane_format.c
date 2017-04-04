/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Hyejung, Kwon <cjscld15@nexell.co.kr>
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
#include <linux/device.h>
#include <linux/module.h>

#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_plane_format.h>

#define fourcc_code(a, b, c, d) ((__u32)(a) | ((__u32)(b) << 8) | \
				 ((__u32)(c) << 16) | ((__u32)(d) << 24))

void nxs_print_plane_format(struct nxs_dev *nxs_dev,
			    struct nxs_plane_format *config)
{
	dev_info(nxs_dev->dev,
		"==========================================\n");

	switch (config->img_type) {
	case NXS_IMG_YUV:
		dev_info(nxs_dev->dev, "the img format is YUV\n");
		break;
	case NXS_IMG_RGB:
		dev_info(nxs_dev->dev, "the img format is RGB\n");
		break;
	case NXS_IMG_RAW:
		dev_info(nxs_dev->dev, "the img format is BAYER\n");
		break;
	default:
		dev_info(nxs_dev->dev, "unsupported image format\n");
		break;
	}

	switch (config->img_bit) {
	case NXS_IMG_BIT_RAW:
		dev_info(nxs_dev->dev, "the img bit is byaer\n");
		break;
	case NXS_IMG_BIT_SM_10:
		dev_info(nxs_dev->dev, "the img bit is smaller than 10bit\n");
		break;
	case NXS_IMG_BIT_10:
		dev_info(nxs_dev->dev, "the img bit is 10 bit\n");
		break;
	case NXS_IMG_BIT_BIG_10:
		dev_info(nxs_dev->dev, "the img bit is bigger than 10 bit\n");
		break;
	default:
		dev_info(nxs_dev->dev, "unsupported image bit\n");
		break;
	}

	dev_info(nxs_dev->dev, "total bitwidth 1st plane		: %d\n",
		config->total_bitwidth_1st_pl);
	dev_info(nxs_dev->dev, "[composition for 1st plane]\n");
	dev_info(nxs_dev->dev, "[0] startbit				: %d\n",
		config->p0_comp[0].startbit);
	dev_info(nxs_dev->dev, "[0] bitwidth				: %d\n",
		config->p0_comp[0].bitwidth);
	dev_info(nxs_dev->dev, "[0] is_2nd_pland			: %s\n",
		(config->p0_comp[0].is_2nd_pl)?"true":"false");
	dev_info(nxs_dev->dev, "[0] use user definition value	: %s\n",
		(config->p0_comp[0].use_userdef)?"enable":"disable");
	dev_info(nxs_dev->dev, "[0] definition value			: %d\n",
		config->p0_comp[0].userdef);
	dev_info(nxs_dev->dev, "[1] startbit				: %d\n",
		config->p0_comp[1].startbit);
	dev_info(nxs_dev->dev, "[1] bitwidth				: %d\n",
		config->p0_comp[1].bitwidth);
	dev_info(nxs_dev->dev, "[1] is_2nd_pland			: %s\n",
		(config->p0_comp[1].is_2nd_pl)?"true":"false");
	dev_info(nxs_dev->dev, "[1] use user definition value	: %s\n",
		(config->p0_comp[1].use_userdef)?"enable":"disable");
	dev_info(nxs_dev->dev, "[1] definition value			: %d\n",
		config->p0_comp[1].userdef);
	dev_info(nxs_dev->dev, "[2] startbit				: %d\n",
		config->p0_comp[2].startbit);
	dev_info(nxs_dev->dev, "[2] bitwidth				: %d\n",
		config->p0_comp[2].bitwidth);
	dev_info(nxs_dev->dev, "[2] is_2nd_pland			: %s\n",
		(config->p0_comp[2].is_2nd_pl)?"true":"false");
	dev_info(nxs_dev->dev, "[2] use user definition value	: %s\n",
		(config->p0_comp[2].use_userdef)?"enable":"disable");
	dev_info(nxs_dev->dev, "[2] definition value			: %d\n",
		config->p0_comp[2].userdef);
	dev_info(nxs_dev->dev, "[3] startbit				: %d\n",
		config->p0_comp[3].startbit);
	dev_info(nxs_dev->dev, "[3] bitwidth				: %d\n",
		config->p0_comp[3].bitwidth);
	dev_info(nxs_dev->dev, "[3] is_2nd_pland			: %s\n",
		(config->p0_comp[3].is_2nd_pl)?"true":"false");
	dev_info(nxs_dev->dev, "[3] use user definition value	: %s\n",
		(config->p0_comp[3].use_userdef)?"enable":"disable");
	dev_info(nxs_dev->dev, "[3] definition value			: %d\n",
		config->p0_comp[3].userdef);
	dev_info(nxs_dev->dev, "[composition for 2nd plane]\n");
	dev_info(nxs_dev->dev, "total bitwidth 2nd plane		: %d\n",
		config->total_bitwidth_2nd_pl);
	dev_info(nxs_dev->dev, "[0] startbit				: %d\n",
		config->p1_comp[0].startbit);
	dev_info(nxs_dev->dev, "[0] bitwidth				: %d\n",
		config->p1_comp[0].bitwidth);
	dev_info(nxs_dev->dev, "[0] is_2nd_pland			: %s\n",
		(config->p1_comp[0].is_2nd_pl)?"true":"false");
	dev_info(nxs_dev->dev, "[0] use user definition value	: %s\n",
		(config->p1_comp[0].use_userdef)?"enable":"disable");
	dev_info(nxs_dev->dev, "[0] definition value			: %d\n",
		config->p1_comp[0].userdef);
	dev_info(nxs_dev->dev, "[1] startbit				: %d\n",
		config->p1_comp[1].startbit);
	dev_info(nxs_dev->dev, "[1] bitwidth				: %d\n",
		config->p1_comp[1].bitwidth);
	dev_info(nxs_dev->dev, "[1] is_2nd_pland			: %s\n",
		(config->p1_comp[1].is_2nd_pl)?"true":"false");
	dev_info(nxs_dev->dev, "[1] use user definition value	: %s\n",
		(config->p1_comp[1].use_userdef)?"enable":"disable");
	dev_info(nxs_dev->dev, "[1] definition value			: %d\n",
		config->p1_comp[1].userdef);
	dev_info(nxs_dev->dev, "[2] startbit				: %d\n",
		config->p1_comp[2].startbit);
	dev_info(nxs_dev->dev, "[2] bitwidth				: %d\n",
		config->p1_comp[2].bitwidth);
	dev_info(nxs_dev->dev, "[2] is_2nd_pland			: %s\n",
		(config->p1_comp[2].is_2nd_pl)?"true":"false");
	dev_info(nxs_dev->dev, "[2] use user definition value	: %s\n",
		(config->p1_comp[2].use_userdef)?"enable":"disable");
	dev_info(nxs_dev->dev, "[2] definition value			: %d\n",
		config->p1_comp[2].userdef);
	dev_info(nxs_dev->dev, "[3] startbit				: %d\n",
		config->p1_comp[3].startbit);
	dev_info(nxs_dev->dev, "[3] bitwidth				: %d\n",
		config->p1_comp[3].bitwidth);
	dev_info(nxs_dev->dev, "[3] is_2nd_pland			: %s\n",
		(config->p1_comp[3].is_2nd_pl)?"true":"false");
	dev_info(nxs_dev->dev, "[3] use user definition value	: %s\n",
		(config->p1_comp[3].use_userdef)?"enable":"disable");
	dev_info(nxs_dev->dev, "[3] definition value			: %d\n",
		config->p1_comp[3].userdef);
	dev_info(nxs_dev->dev,
		"==========================================\n");
}

u32 nxs_get_plane_format(struct nxs_dev *nxs_dev, u32 format,
			 struct nxs_plane_format *config)
{
	dev_info(nxs_dev->dev, "[%s]\n", __func__);

	config->img_bit = NXS_IMG_BIT_SM_10;
	switch (format) {
	/* RGB */
	case fourcc_code('A', 'R', '1', '5'):
		/*
		 * [15:0] ARGB-1-5-5-5
		 * BPP = 16
		 * 7	6	5	4	3	2	1	0
		 * g2	g1	g0	b4	b3	b2	b1	b0
		 * 15	14	13	12	11	10	9	8
		 * a	r4	r3	r2	r1	r0	g4	g3
		 */
		config->img_type = NXS_IMG_RGB;
		config->total_bitwidth_1st_pl = 32;
		config->p0_comp[0].startbit = 10;
		config->p0_comp[0].bitwidth = 5;
		config->p0_comp[1].startbit = 5;
		config->p0_comp[1].bitwidth = 5;
		config->p0_comp[2].startbit = 0;
		config->p0_comp[2].bitwidth = 5;
		config->p0_comp[3].startbit = 15;
		config->p0_comp[3].bitwidth = 1;
		config->p1_comp[0].startbit = 26;
		config->p1_comp[0].bitwidth = 5;
		config->p1_comp[1].startbit = 21;
		config->p1_comp[1].bitwidth = 5;
		config->p1_comp[2].startbit = 16;
		config->p1_comp[2].bitwidth = 5;
		config->p1_comp[3].startbit = 31;
		config->p1_comp[3].bitwidth = 1;
		break;
	case fourcc_code('X', 'R', '1', '5'):
		/*
		 * [15:0] XRGB-1-5-5-5(X is dummy)
		 * BPP = 16
		 * 7	6	5	4	3	2	1	0
		 * g2	g1	g0	b4	b3	b2	b1	b0
		 * 15	14	13	12	11	10	9	8
		 * -	r4	r3	r2	r1	r0	g4	g3
		 */
		config->img_type = NXS_IMG_RGB;
		config->total_bitwidth_1st_pl = 32;
		config->p0_comp[0].startbit = 10;
		config->p0_comp[0].bitwidth = 5;
		config->p0_comp[1].startbit = 5;
		config->p0_comp[1].bitwidth = 5;
		config->p0_comp[2].startbit = 0;
		config->p0_comp[2].bitwidth = 5;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 15;
		config->p1_comp[0].bitwidth = 5;
		config->p1_comp[1].startbit = 26;
		config->p1_comp[1].bitwidth = 5;
		config->p1_comp[2].startbit = 21;
		config->p1_comp[2].bitwidth = 5;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		break;
	case fourcc_code('R', 'G', 'B', 'P'): /* V4L2 RGB565 */
	case fourcc_code('R', 'G', '1', '6'): /* DRM RGB565 */
		/*
		 * [15:0] RGB-5-6-5
		 * same as NX_B5_G6_R5
		 * BPP = 16
		 * 7	6	5	4	3	2	1	0
		 * g2	g1	g0	b4	b3	b2	b1	b0
		 * 15	14	13	12	11	10	9	8
		 * r4	r3	r2	r1	r0	g5	g4	g3
		 */
		config->img_type = NXS_IMG_RGB;
		config->total_bitwidth_1st_pl = 32;
		config->p0_comp[0].startbit = 11;
		config->p0_comp[0].bitwidth = 5;
		config->p0_comp[1].startbit = 5;
		config->p0_comp[1].bitwidth = 6;
		config->p0_comp[2].startbit = 0;
		config->p0_comp[2].bitwidth = 5;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 27;
		config->p1_comp[0].bitwidth = 5;
		config->p1_comp[1].startbit = 21;
		config->p1_comp[1].bitwidth = 6;
		config->p1_comp[2].startbit = 16;
		config->p1_comp[2].bitwidth = 5;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		break;
	case fourcc_code('B', 'G', 'R', '3'): /* V4L2 BGR888 */
	case fourcc_code('B', 'G', '2', '4'): /* DRM BGR888 */
		/*
		 * [23:0] BGR-8-8-8
		 * BPP = 24
		 * 7	6	5	4	3	2	1	0
		 * r7	r6	r5	r4	r3	r2	r1	r0
		 * 15	14	13	12	11	10	9	8
		 * g7	g6	g5	g4	g3	g2	g1	g0
		 * 23	22	21	20	19	18	17	16
		 * b7	b6	b5	b4	b3	b2	b1	b0
		 */
		config->img_type = NXS_IMG_RGB;
		config->total_bitwidth_1st_pl = 48;
		config->p0_comp[0].startbit = 0;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 8;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[2].startbit = 16;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 24;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 32;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[2].startbit = 40;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		break;
	case fourcc_code('R', 'G', 'B', '3'): /* V4L2 RGB888 */
	case fourcc_code('R', 'G', '2', '4'): /* DRM RGB888 */
		/*
		 * [23:0] RGB-8-8-8
		 * bpp = 24
		 * 7	6	5	4	3	2	1	0
		 * b7	b6	b5	b4	b3	b2	b1	b0
		 * 15	14	13	12	11	10	9	8
		 * g7	g6	g5	g4	g3	g2	g1	g0
		 * 23	22	21	20	19	18	17	16
		 * r7	r6	r5	r4	r3	r2	r1	r0
		 */
		config->img_type = NXS_IMG_RGB;
		config->total_bitwidth_1st_pl = 48;
		config->p0_comp[0].startbit = 16;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 8;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[2].startbit = 0;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 40;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 32;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[2].startbit = 24;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		break;
	case fourcc_code('B', 'G', 'R', '4'):
		/* BGR-8-8-8-8
		 * as Packed RGB Formats,
		 * https://www.linuxtv.org/downloads/v4l-dvb-apis-old/
		 * packed-rgb.html
		 * thie composition is changed depends on the driver
		 * so currently handle same as the format ABGR32-8-8-8-8
		 * following the site
		 */
	case fourcc_code('A', 'R', '2', '4'):
		/*
		 * ARGB8888
		 * [32:0] ARGB
		 * BPP = 32
		 * 7	6	5	4	3	2	1	0
		 * b7	b6	b5	b4	b3	b2	b1	b0
		 * 15	14	13	12	11	10	9	8
		 * g7	g6	g5	g4	g3	g2	g1	g0
		 * 23	22	21	20	19	18	17	16
		 * r7	r6	r5	r4	r3	r2	r1	r0
		 * 31	30	29	28	27	26	25	24
		 * a7	a6	a5	a4	a3	a2	a1	a0
		 */
		config->img_type = NXS_IMG_RGB;
		config->total_bitwidth_1st_pl = 64;
		config->p0_comp[0].startbit = 16;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 8;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[2].startbit = 0;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[3].startbit = 24;
		config->p0_comp[3].bitwidth = 8;
		config->p1_comp[0].startbit = 48;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 40;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[2].startbit = 32;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[3].startbit = 56;
		config->p1_comp[3].bitwidth = 8;
		break;
	case fourcc_code('X', 'R', '2', '4'):
		/*
		 * [31:0] XRGB8888
		 * bpp = 32;
		 */
		config->img_type = NXS_IMG_RGB;
		config->p0_comp[0].startbit = 16;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 8;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[2].startbit = 0;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 48;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 40;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[2].startbit = 32;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		break;
	case fourcc_code('R', 'G', 'B', '4'):
		/* RGB-8-8-8-8
		 * as Packed RGB Formats,
		 * https://www.linuxtv.org/downloads/v4l-dvb-apis-old/
		 * packed-rgb.html
		 * thie composition is changed depends on the driver
		 * so currently handle same as the format ARGB32-8-8-8-8
		 * following the site
		 */
	case fourcc_code('B', 'A', '2', '4'):
		/*
		 * [31:0] BGRA8888 same as NX_A8_R8_G8-B8
		 * BPP = 32
		 * a7	a6	a5	a4	a3	a2	a1	a0
		 * r7	r6	r5	r4	r3	r2	r1	r0
		 * g7	g6	g5	g4	g3	g2	g1	g0
		 * b7	b6	b5	b4	b3	b2	b1	b0
		 */
		config->img_type = NXS_IMG_RGB;
		config->total_bitwidth_1st_pl = 64;
		config->p0_comp[0].startbit = 8;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 16;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[2].startbit = 24;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[3].startbit = 0;
		config->p0_comp[3].bitwidth = 8;
		config->p1_comp[0].startbit = 40;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 48;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[2].startbit = 56;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[3].startbit = 32;
		config->p1_comp[3].bitwidth = 8;
		break;
	case fourcc_code('B', 'X', '2', '4'):
		/*
		 * [31:0] BGRX8888
		 * bpp = 32
		 */
		config->img_type = NXS_IMG_RGB;
		config->total_bitwidth_1st_pl = 64;
		config->p0_comp[0].startbit = 8;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 16;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[2].startbit = 24;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 32;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 40;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[2].startbit = 48;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		break;
	/* YUV422 */
	case fourcc_code('Y', 'U', 'Y', 'V'):
		/*
		 * [31:0] Cr0:Y1:Cb0:Y0 8:8:8:8
		 * bpp = 16
		 */
		config->img_type = NXS_IMG_YUV;
		config->total_bitwidth_1st_pl = 32;
		config->p0_comp[0].startbit = 0;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 8;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[2].startbit = 24;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 16;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 8;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[2].startbit = 24;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		config->use_average_value = 1;
		break;
	case fourcc_code('Y', 'Y', 'V', 'U'):
		/*
		 * [31:0] Cb0:Cr0:Y1:Y0
		 * bpp = 16
		 */
		config->img_type = NXS_IMG_YUV;
		config->total_bitwidth_1st_pl = 32;
		config->p0_comp[0].startbit = 0;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 16;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[2].startbit = 24;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 8;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 16;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[2].startbit = 24;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		config->use_average_value = 1;
		break;
	case fourcc_code('Y', 'V', 'Y', 'U'):
		/*
		 * [31:0] Cb0:Y1:Cr0:Y0 8:8:8:8
		 * bpp = 16
		 */
		config->img_type = NXS_IMG_YUV;
		config->total_bitwidth_1st_pl = 32;
		config->p0_comp[0].startbit = 0;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 24;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[2].startbit = 8;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 16;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 24;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[2].startbit = 8;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		config->use_average_value = 1;
		break;
	case fourcc_code('U', 'Y', 'V', 'Y'):
		/*
		 * [31:0] Y1:Cr0:Y0:Cb0 8:8:8:8
		 * bpp = 16
		 */
		config->img_type = NXS_IMG_YUV;
		config->total_bitwidth_1st_pl = 32;
		config->p0_comp[0].startbit = 8;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 0;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[2].startbit = 16;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 24;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 0;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[2].startbit = 16;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		config->use_average_value = 1;
		break;
	case fourcc_code('V', 'Y', 'U', 'Y'):
		/*
		 * [31:0] Y1:Cb0:Y0:Cr0 8:8:8:8
		 * bpp = 16
		 */
		config->img_type = NXS_IMG_YUV;
		config->total_bitwidth_1st_pl = 32;
		config->p0_comp[0].startbit = 8;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 16;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[2].startbit = 0;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 24;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 16;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[2].startbit = 0;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		config->use_average_value = 1;
		break;
	case fourcc_code('N', 'V', '2', '4'):
		/*
		 * Y/CbCr 4:4:4
		 * bpp = 24
		 */
		config->img_type = NXS_IMG_YUV;
		config->total_bitwidth_1st_pl = 16;
		config->total_bitwidth_2nd_pl = 32;
		config->p0_comp[0].startbit = 0;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 0;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[1].is_2nd_pl = 1;
		config->p0_comp[2].startbit = 8;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[2].is_2nd_pl = 1;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 8;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 16;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[1].is_2nd_pl = 1;
		config->p1_comp[2].startbit = 24;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[2].is_2nd_pl = 1;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		config->use_average_value = 1;
		break;
	case fourcc_code('N', 'V', '4', '2'):
		/* bpp = 24 */
		config->img_type = NXS_IMG_YUV;
		config->total_bitwidth_1st_pl = 16;
		config->total_bitwidth_2nd_pl = 32;
		config->p0_comp[0].startbit = 0;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 8;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[1].is_2nd_pl = 1;
		config->p0_comp[2].startbit = 0;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[2].is_2nd_pl = 1;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 8;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 24;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[1].is_2nd_pl = 1;
		config->p1_comp[2].startbit = 16;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[2].is_2nd_pl = 1;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		config->use_average_value = 1;
		break;
	case fourcc_code('N', 'V', '1', '2'):
	case fourcc_code('N', 'M', '1', '2'):
		/*
		 * Y/CbCr 4:2:0 same as NX_2P_Y08_Y18_U8_V8
		 * bpp = 12
		 */
		config->img_type = NXS_IMG_YUV;
		config->total_bitwidth_1st_pl = 16;
		config->total_bitwidth_2nd_pl = 16;
		config->half_height_2nd_pl = 1;
		config->p0_comp[0].startbit = 0;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 0;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[1].is_2nd_pl = 1;
		config->p0_comp[2].startbit = 8;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[2].is_2nd_pl = 1;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 8;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 0;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[1].is_2nd_pl = 1;
		config->p1_comp[2].startbit = 8;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[2].is_2nd_pl = 1;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		config->use_average_value = 1;
		break;
	case fourcc_code('N', 'V', '2', '1'):
	case fourcc_code('N', 'M', '2', '1'):
		/*
		 * Y/CbCr 420 same as NX_2P_Y08_Y18_V8_U8
		 * bpp = 12
		 */
		config->img_type = NXS_IMG_YUV;
		config->total_bitwidth_1st_pl = 16;
		config->total_bitwidth_2nd_pl = 16;
		config->half_height_2nd_pl = 1;
		config->p0_comp[0].startbit = 0;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 8;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[1].is_2nd_pl = 1;
		config->p0_comp[2].startbit = 0;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[2].is_2nd_pl = 1;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 8;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 8;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[1].is_2nd_pl = 1;
		config->p1_comp[2].startbit = 0;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[2].is_2nd_pl = 1;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		config->use_average_value = 1;
		break;
	case fourcc_code('N', 'V', '1', '6'):
	case fourcc_code('N', 'M', '1', '6'):
		/*
		 * Y/CbCr 422
		 * bpp = 16
		 */
		config->img_type = NXS_IMG_YUV;
		config->total_bitwidth_1st_pl = 16;
		config->total_bitwidth_2nd_pl = 16;
		config->p0_comp[0].startbit = 0;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 0;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[1].is_2nd_pl = 1;
		config->p0_comp[2].startbit = 8;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[2].is_2nd_pl = 1;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 8;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 0;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[1].is_2nd_pl = 1;
		config->p1_comp[2].startbit = 8;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[2].is_2nd_pl = 1;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		config->use_average_value = 1;
		break;
	case fourcc_code('N', 'V', '6', '1'):
	case fourcc_code('N', 'M', '6', '1'):
		/*
		 * same as NX_2P_Y08_Y18_V8_U8
		 * bpp = 16
		 */
		config->img_type = NXS_IMG_YUV;
		config->total_bitwidth_1st_pl = 16;
		config->total_bitwidth_2nd_pl = 16;
		config->p0_comp[0].startbit = 0;
		config->p0_comp[0].bitwidth = 8;
		config->p0_comp[1].startbit = 8;
		config->p0_comp[1].bitwidth = 8;
		config->p0_comp[1].is_2nd_pl = 1;
		config->p0_comp[2].startbit = 0;
		config->p0_comp[2].bitwidth = 8;
		config->p0_comp[2].is_2nd_pl = 1;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 8;
		config->p1_comp[0].bitwidth = 8;
		config->p1_comp[1].startbit = 8;
		config->p1_comp[1].bitwidth = 8;
		config->p1_comp[1].is_2nd_pl = 1;
		config->p1_comp[2].startbit = 0;
		config->p1_comp[2].bitwidth = 8;
		config->p1_comp[2].is_2nd_pl = 1;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		config->use_average_value = 1;
		break;
	case fourcc_code('N', 'I', 'U', '0'):
		/*
		 * Y/CbCr 4:2:0 Interleaved 10bit
		 * bpp = 15
		 * extended 16bit
		 * LSB
		 * 15 14 13 12 11 10 9  8  7  6 5  4  3  2  1  0
		 *                  Y9 Y8 y7 Y6 Y5 Y4 Y3 y2 Y1 Y0
		 * 15 14 13 12 11 10 9  8  7  6 5  4  3  2  1  0
		 *                  U9 U8 U7 U6 U5 U4 U3 U2 U1 U0
		 * 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
		 *                   V9 V8 V7 V6 V5 V4 V3 V2 V1 V0
		 *
		 */
		config->img_type = NXS_IMG_YUV;
		config->img_bit = NXS_IMG_BIT_BIG_10;
		config->total_bitwidth_1st_pl = 32;
		config->total_bitwidth_2nd_pl = 32;
		config->half_height_2nd_pl = 1;
		config->p0_comp[0].startbit = 0;
		config->p0_comp[0].bitwidth = 10;
		config->p0_comp[1].startbit = 0;
		config->p0_comp[1].bitwidth = 10;
		config->p0_comp[1].is_2nd_pl = 1;
		config->p0_comp[2].startbit = 16;
		config->p0_comp[2].bitwidth = 10;
		config->p0_comp[2].is_2nd_pl = 1;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 16;
		config->p1_comp[0].bitwidth = 10;
		config->p1_comp[1].startbit = 0;
		config->p1_comp[1].bitwidth = 10;
		config->p1_comp[1].is_2nd_pl = 1;
		config->p1_comp[2].startbit = 16;
		config->p1_comp[2].bitwidth = 10;
		config->p1_comp[2].is_2nd_pl = 1;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		config->use_average_value = 1;
		config->put_dummy_type = 1;
		break;
	case fourcc_code('N', 'I', 'V', '0'):
		/*
		 * Y/CbCr 4:2:0 Interleaved 10bit
		 * bpp = 15
		 * extended 16bit
		 * 15 14 13 12 11 10 9  8  7  6 5  4  3  2  1  0
		 *                   V9 V8 V7 V6 V5 V4 V3 V2 V1 V0
		 * 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
		 *                   U9 U8 U7 U6 U5 U4 U3 U2 U1 U0
		 */
		config->img_type = NXS_IMG_YUV;
		config->img_bit = NXS_IMG_BIT_BIG_10;
		config->total_bitwidth_1st_pl = 32;
		config->total_bitwidth_2nd_pl = 32;
		config->half_height_2nd_pl = 1;
		config->p0_comp[0].startbit = 0;
		config->p0_comp[0].bitwidth = 10;
		config->p0_comp[1].startbit = 16;
		config->p0_comp[1].bitwidth = 10;
		config->p0_comp[1].is_2nd_pl = 1;
		config->p0_comp[2].startbit = 0;
		config->p0_comp[2].bitwidth = 10;
		config->p0_comp[2].is_2nd_pl = 1;
		config->p0_comp[3].use_userdef = 1;
		config->p0_comp[3].userdef = ~0;
		config->p1_comp[0].startbit = 16;
		config->p1_comp[0].bitwidth = 10;
		config->p1_comp[1].startbit = 16;
		config->p1_comp[1].bitwidth = 10;
		config->p1_comp[1].is_2nd_pl = 1;
		config->p1_comp[2].startbit = 0;
		config->p1_comp[2].bitwidth = 10;
		config->p1_comp[2].is_2nd_pl = 1;
		config->p1_comp[3].use_userdef = 1;
		config->p1_comp[3].userdef = ~0;
		config->use_average_value = 1;
		config->put_dummy_type = 1;
		break;
	default:
		/*
		 * Todo
		 * support for bayer format and 10bit format
		 * put_dummy_type
		 */
		dev_err(nxs_dev->dev, "unsupported format\n");
		config->img_type = NXS_IMG_MAX;
		config->img_bit = NXS_IMG_BIT_MAX;
		return -EINVAL;
	}

	dev_info(nxs_dev->dev,
		 "format is %s, multiplanes:%s\n",
		 (config->img_type) ? "RGB" : "YUV",
		 (config->total_bitwidth_2nd_pl) ? "true" : "false");
	return 0;
}
