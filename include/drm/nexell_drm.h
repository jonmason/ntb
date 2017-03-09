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
#ifndef _NX_DRM_H_
#define _NX_DRM_H_

#include <uapi/drm/nexell_drm.h>

#define	MIPI_DSI_FB_FLIP	1
#define	MIPI_DSI_FB_FLUSH	2

struct nx_mipi_dsi_nb_data {
	struct mipi_dsi_device *dsi;
	struct drm_framebuffer *fb;
	void __iomem *framebuffer;
	int fifo_size;
};

int nx_drm_mipi_register_notifier(struct device *dev,
			struct notifier_block *nb, unsigned int events);
int nx_drm_mipi_unregister_notifier(struct device *dev,
			struct notifier_block *nb);

#endif
