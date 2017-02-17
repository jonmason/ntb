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
#ifndef _NX_DRM_DRV_H_
#define _NX_DRM_DRV_H_

#include <linux/module.h>
#include <linux/reset.h>

#define MAX_CRTCS	2	/* Multi Layer Controller(MLC) nums (0, 1) */
#define MAX_CONNECTOR	4	/* RGB, LVDS, MiPi, HDMI */

#define MAX_FB_MODE_WIDTH	4096
#define MAX_FB_MODE_HEIGHT	4096

struct nx_drm_priv {
	struct nx_framebuffer_dev *framebuffer_dev;
	unsigned int possible_pipes;
	bool force_detect;
	struct drm_crtc *crtcs[MAX_CRTCS];
	int num_crtcs;
	struct mutex lock;
};

#endif
