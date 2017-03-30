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
#ifndef _NXS_PLANE_FORMAT_H
#define _NXS_PLANE_FORMAT_H

enum nxs_img_type {
	NXS_IMG_YUV = 0,
	NXS_IMG_RGB,
	NXS_IMG_RAW,
	NXS_IMG_MAX
};

enum nxs_img_bit {
	NXS_IMG_BIT_RAW = 0,
	NXS_IMG_BIT_SM_10,
	NXS_IMG_BIT_10,
	NXS_IMG_BIT_BIG_10,
	NXS_IMG_BIT_MAX
};

struct nxs_p_composite {
	u32 startbit;
	u32 bitwidth;
	u8 is_2nd_pl;
	u8 use_userdef;
	u32 userdef;
};

struct nxs_plane_format {
	u32 img_type;
	u8 img_bit;
	u32 total_bitwidth_1st_pl;
	u32 total_bitwidth_2nd_pl;
	u8 half_height_1st_pl;
	u8 half_height_2nd_pl;
	/* only for DMAW */
	u8 use_average_value;
	u32 put_dummy_type;
	struct nxs_p_composite p0_comp[4];
	struct nxs_p_composite p1_comp[4];
};

void nxs_print_plane_format(struct nxs_dev *nxs_dev,
			    struct nxs_plane_format *config);
u32 nxs_get_plane_format(struct nxs_dev *nxs_dev, u32 format,
			 struct nxs_plane_format *config);
#endif
