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

#include "soc/s5pxx18_drm_dp.h"

struct nx_drm_crtc {
	struct drm_crtc crtc;
	struct drm_display_mode	current_mode;
	int pipe;		/* hw crtc index */
	int pipe_irq;
	struct dp_plane_top top;
	struct drm_pending_vblank_event *event;
	unsigned int dpms_mode;
	struct reset_control *resets[2];
	int num_resets;
	bool post_closed;
	bool suspended;
};

#define to_nx_crtc(x)	\
		container_of(x, struct nx_drm_crtc, crtc)

int nx_drm_crtc_init(struct drm_device *dev);
int nx_drm_crtc_enable_vblank(struct drm_device *dev, unsigned int pipe);
void nx_drm_crtc_disable_vblank(struct drm_device *dev, unsigned int pipe);

#endif
