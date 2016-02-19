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

#include <drm/drm_gem.h>

struct nx_drm_fb {
	struct drm_framebuffer fb;
	struct drm_gem_cma_object *obj[MAX_FB_PLANE];
	bool is_fbdev_fb;
};

#define to_nx_drm_fb(x)	\
			container_of(x, struct nx_drm_fb, fb)

static inline int nx_drm_format_num_buffers(uint32_t format)
{
	DRM_DEBUG_KMS("format:0x%x\n", format);
	return 1;
}

extern void nx_drm_mode_config_init(struct drm_device *dev);
extern struct drm_framebuffer *nx_drm_fb_allocate(struct drm_device *dev,
						  struct drm_mode_fb_cmd2
						  *mode_cmd,
						  struct drm_gem_cma_object
						  **obj, int num_planes);
extern void nx_drm_fb_destroy(struct drm_framebuffer *fb);

extern struct nx_gem_buffer *nx_drm_fb_buffer(struct drm_framebuffer *fb,
					      int index);
extern struct drm_gem_cma_object *nx_drm_fb_get_gem_obj(struct drm_framebuffer
							*fb,
							unsigned int plane);

#endif
