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

#ifndef _UAPI__LINUX_NXS_V4L2_H
#define _UAPI__LINUX_NXS_V4L2_H

#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>

#define NXS_IOC_BASE	(BASE_VIDIOC_PRIVATE + 1)

struct nxs_control_tpgen {
	uint32_t realtime_mode;
	uint32_t nclk_per_twopix;
	uint32_t tp_gen_mode;
	uint32_t v2h_delay;
	uint32_t h2h_delay;
	uint32_t h2v_delay;
	uint32_t v2v_delay;

	uint32_t enc_header0;
	uint32_t enc_header1;
	uint32_t enc_header2;
	uint32_t enc_header3;
};

#define NXSIOC_S_TPGEN	_IOW('V', NXS_IOC_BASE + 1, struct nxs_control_tpgen)
#define NXSIOC_S_DSTFMT _IOW('V', NXS_IOC_BASE + 2, struct v4l2_subdev_format)
#define NXSIOC_G_DSTFMT _IOWR('V', NXS_IOC_BASE + 3, struct v4l2_subdev_format)
#define NXSIOC_START	_IO('V', NXS_IOC_BASE + 4)
#define NXSIOC_STOP	_IO('V', NXS_IOC_BASE + 5)

#endif
