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

#ifndef _S5PXX18_DP_TV_H_
#define _S5PXX18_DP_TV_H_

#include <drm/drmP.h>
#include <video/videomode.h>
#include <media/v4l2-subdev.h>

#include "s5pxx18_drm_dp.h"

int nx_dp_tv_set_commit(struct nx_drm_device *display, int pipe);
extern struct media_device *nx_v4l2_get_media_device(void);
extern int nx_v4l2_register_subdev(struct v4l2_subdev *);

#endif
