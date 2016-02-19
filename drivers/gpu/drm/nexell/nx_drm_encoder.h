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

#include <drm/drm_encoder_slave.h>

/*
#define       DRM_ENCODER_SLAVE_SUPPORT
*/
struct nx_drm_display;

struct nx_drm_encoder {
#ifdef	DRM_ENCODER_SLAVE_SUPPORT
	struct drm_encoder_slave drm_encoder;
#else
	struct drm_encoder drm_encoder;
#endif
	struct nx_drm_display *display;
	int dpms;
	void *context;		/* device context */
};

#ifdef	DRM_ENCODER_SLAVE_SUPPORT
#define to_nx_encoder(e)	\
		container_of(to_encoder_slave(e), \
		struct nx_drm_encoder, drm_encoder)
#else
#define to_nx_encoder(e)	\
		container_of(e, struct nx_drm_encoder, drm_encoder)
#endif

extern void nx_drm_encoder_setup(struct drm_device *dev);
extern struct drm_encoder *nx_drm_encoder_create(struct drm_device *drm,
					  struct nx_drm_display *mgr,
					  int possible_crtcs, void *context);
extern struct nx_drm_display *nx_drm_get_display(struct drm_encoder *encoder);

extern void nx_drm_enable_vblank(struct drm_encoder *encoder, void *data);
extern void nx_drm_disable_vblank(struct drm_encoder *encoder, void *data);
extern void nx_drm_encoder_crtc_plane_commit(
				struct drm_encoder *encoder, void *data);
extern void nx_drm_encoder_crtc_commit(struct drm_encoder *encoder, void *data);
extern void nx_drm_encoder_dpms_from_crtc(
				struct drm_encoder *encoder, void *data);
extern void nx_drm_encoder_crtc_dpms(struct drm_encoder *encoder, void *data);
extern void nx_drm_encoder_crtc_mode_set(
				struct drm_encoder *encoder, void *data);
extern void nx_drm_encoder_crtc_disable(
				struct drm_encoder *encoder, void *data);

#endif
