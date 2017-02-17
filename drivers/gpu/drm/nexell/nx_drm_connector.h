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
#ifndef _NX_DRM_CONNECTOR_H_
#define _NX_DRM_CONNECTOR_H_

#include "soc/s5pxx18_drm_dp.h"

struct nx_drm_connector {
	struct drm_connector connector;
	struct drm_encoder *encoder;
	struct nx_drm_device *display;
	void *context;		/* device context */
};

#define to_nx_connector(c)		\
		container_of(c, struct nx_drm_connector, connector)

struct drm_connector *nx_drm_connector_create_and_attach(
			struct drm_device *drm,
			struct nx_drm_device *display,
			int pipe, unsigned int possible_crtcs,
			enum dp_panel_type panel_type, void *context);

void nx_drm_connector_destroy_and_detach(struct drm_connector *connector);

#endif
