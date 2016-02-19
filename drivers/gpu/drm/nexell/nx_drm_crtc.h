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
#ifndef _NX_DRM_CRTC_H_
#define _NX_DRM_CRTC_H_

extern int nx_drm_crtc_init(struct drm_device *dev, int nr, int layer);
extern int nx_drm_crtc_enable_vblank(struct drm_device *dev, int crtc);
extern void nx_drm_crtc_disable_vblank(struct drm_device *dev, int crtc);

struct nx_drm_crtc_pos {
	unsigned int fb_x;
	unsigned int fb_y;
	unsigned int crtc_x;
	unsigned int crtc_y;
	unsigned int crtc_w;
	unsigned int crtc_h;
};

struct nx_drm_crtc {
	struct drm_crtc drm_crtc;
	int nr;
	unsigned int dpms_mode;
};

#endif
