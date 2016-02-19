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
#ifndef _NX_DRM_PLANE_H_
#define _NX_DRM_PLANE_H_

#include "nx_drm_crtc.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>

/*
 * drm common pnane structure.
 */
struct nx_drm_plane {
	struct drm_plane plane;
	int layer;
	unsigned int fb_x;
	unsigned int fb_y;
	unsigned int fb_width;
	unsigned int fb_height;
	unsigned int crtc_x;
	unsigned int crtc_y;
	unsigned int crtc_width;
	unsigned int crtc_height;
	unsigned int mode_width;
	unsigned int mode_height;
	unsigned int refresh;
	unsigned int scan_flag;
	unsigned int bpp;
	unsigned int pitch;
	uint32_t pixel_format;
	dma_addr_t dma_addr[MAX_FB_PLANE];
	void __iomem *vaddr[MAX_FB_PLANE];
	int zpos;
	bool default_win;
	bool color_key;
	unsigned int index_color;
	bool local_path;
	bool transparency;
	bool activated;
	bool enabled;
};

#define to_nx_plane(x)	\
			container_of(x, struct nx_drm_plane, plane)

extern struct drm_plane *nx_drm_plane_init(struct drm_device *dev,
				unsigned long possible_crtcs,
				enum drm_plane_type type, int layer);

extern int nx_drm_plane_set_zpos_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv);
extern int nx_drm_plane_update(struct nx_drm_plane *dev_plane,
				struct drm_framebuffer *fb,
				struct drm_display_mode *mode,
				struct nx_drm_crtc_pos *pos);

#endif
