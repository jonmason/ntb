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
#include <linux/fb.h>
#include <video/videomode.h>

struct nx_drm_fb {
	struct drm_framebuffer	fb;
	struct nx_gem_object *obj[4];
};

struct nx_drm_fbdev {
	struct drm_fb_helper fb_helper;
	struct nx_drm_fb *fb;
	int fb_buffers;
};

struct nx_framebuffer_dev {
	struct nx_drm_fbdev *fbdev;
};

static inline struct nx_drm_fbdev *to_nx_drm_fbdev(struct drm_fb_helper *helper)
{
	return container_of(helper, struct nx_drm_fbdev, fb_helper);
}

static inline struct nx_drm_fb *to_nx_drm_fb(struct drm_framebuffer *fb)
{
	return container_of(fb, struct nx_drm_fb, fb);
}

int nx_drm_framebuffer_init(struct drm_device *dev);
void nx_drm_framebuffer_fini(struct drm_device *dev);

struct drm_framebuffer *nx_drm_fb_mode_create(struct drm_device *dev,
			struct drm_file *file_priv,
			struct drm_mode_fb_cmd2 *mode_cmd);

/*
 * nexell framebuffer with gem
 */
struct nx_gem_object *nx_drm_fb_get_gem_obj(struct drm_framebuffer *fb,
			unsigned int plane);
#endif
