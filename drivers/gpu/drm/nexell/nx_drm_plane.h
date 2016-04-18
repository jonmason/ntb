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

#include "soc/s5pxx18_drm_dp.h"

/* properties */
union color_property {
	struct {
		unsigned int transcolor;
		unsigned int alphablend;
		struct {
			struct drm_property *transcolor;
			struct drm_property *alphablend;
		} rgb;
	};
	struct {
		unsigned int colorkey;
		struct {
			struct drm_property *colorkey;
		} yuv;
	};
};

struct nx_drm_plane {
	struct drm_plane plane;
	struct dp_plane_layer layer;
	bool enabled;
	union color_property color;
	bool is_yuv_plane;
};

#define to_nx_plane(x)	\
		container_of(x, struct nx_drm_plane, plane)

struct drm_plane *nx_drm_plane_init(struct drm_device *drm,
			struct drm_crtc *crtc,
			unsigned long possible_crtcs,
			enum drm_plane_type type,
			int plane_num);

#endif
