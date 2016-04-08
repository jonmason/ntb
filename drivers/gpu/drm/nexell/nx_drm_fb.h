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
#ifndef _NX_DRM_FB_H_
#define _NX_DRM_FB_H_

#include <drm/drm_fb_helper.h>
#include <drm/drm_fb_cma_helper.h>

struct nx_framebuffer_dev {
	struct drm_fbdev_cma *fbdev;
};

int nx_drm_framebuffer_dev_init(struct drm_device *dev);
void nx_drm_framebuffer_dev_fini(struct drm_device *dev);

#endif
