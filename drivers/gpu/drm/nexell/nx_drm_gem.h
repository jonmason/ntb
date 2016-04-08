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

#ifndef _NX_DRM_GEM_H_
#define _NX_DRM_GEM_H_

#include <drm/drm_gem.h>
#include <drm/drm_gem_cma_helper.h>

/*
 * request gem object creation and buffer allocation as the size
 * that it is calculated with framebuffer information such as width,
 * height and bpp.
 */
int nx_drm_gem_create_ioctl(struct drm_device *drm, void *data,
			struct drm_file *file_priv);

/* get buffer information to memory region allocated by gem. */
int nx_drm_gem_get_ioctl(struct drm_device *drm, void *data,
			struct drm_file *file_priv);

struct sg_table *nx_drm_gem_prime_get_sg_table(struct drm_gem_object *obj);


#endif
