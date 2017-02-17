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
#ifndef _NX_DRM_ENCODER_H_
#define _NX_DRM_ENCODER_H_

#include "soc/s5pxx18_drm_dp.h"

struct nx_drm_encoder {
	struct drm_encoder encoder;
	int pipe;
	struct nx_drm_device *display;
	int dpms;
	bool enabled;
	void *context;		/* device context */
};

#define to_nx_encoder(e)	\
		container_of(e, struct nx_drm_encoder, encoder)

struct drm_encoder *nx_drm_encoder_create(struct drm_device *drm,
			struct nx_drm_device *display, int enc_type,
			int pipe, int possible_crtcs, void *context);

#endif
