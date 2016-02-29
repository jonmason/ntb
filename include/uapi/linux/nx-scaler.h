/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongkeun, Choi <jkchoi@nexell.co.kr>
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
#ifndef __UAPI_NX_SCALER_H
#define __UAPI_NX_SCALER_H

#include <linux/ioctl.h>

#define	IOC_NX_MAGIC	0x6e78	/* nx */
#define	MAX_PLANE_NUM	3

struct nx_scaler_ioctl_data {
	int		src_plane_num;
	int		src_fds[MAX_PLANE_NUM];
	uint32_t	src_stride[MAX_PLANE_NUM];
	unsigned int	src_width;
	unsigned int	src_height;
	unsigned int	src_code;
	int		dst_plane_num;
	int		dst_fds[MAX_PLANE_NUM];
	int		dst_stride[MAX_PLANE_NUM];
	unsigned int	dst_width;
	unsigned int	dst_height;
	unsigned int	dst_code;
};

enum	{
	IOCTL_SCALER_SET_AND_RUN	=	_IO(IOC_NX_MAGIC, 1),
};
#endif
