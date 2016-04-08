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
#ifndef _S5PXX18_DRM_DP_H_
#define _S5PXX18_DRM_DP_H_

#include <video/videomode.h>

#include "s5pxx18_soc_dp.h"

struct nx_drm_dev_ops {
	bool (*is_connected)(struct device *dev,
			struct drm_connector *connector);
	int (*get_modes)(struct device *dev, struct drm_connector *connector);
	int (*check_mode)(struct device *dev, struct drm_display_mode *mode);

	void (*commit)(struct device *dev);
	void (*dmps)(struct device *dev, int mode);

	bool (*mode_fixup)(struct device *dev,
			struct drm_connector *connector,
			const struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode);
	void (*mode_set)(struct device *dev, void *mode);
	int (*enable_vblank)(struct device *dev);
	void (*disable_vblank)(struct device *dev);
};

struct nx_drm_panel {
	struct device_node *panel_node;
	struct drm_panel *panel;
	int width_mm;
	int height_mm;
	struct videomode vm;
};

struct nx_drm_dp_dev {
	struct device *dev;
	struct nx_drm_dev_ops *ops;
	struct nx_drm_panel dpp;
	struct dp_device_info ddi;
};

void nx_drm_dp_device_set_base(struct platform_device *pdev);
void nx_drm_dp_crtc_init(struct drm_device *drm, struct drm_crtc *crtc,
			int index);
int nx_drm_dp_crtc_mode_set(struct drm_crtc *crtc,
			struct drm_plane *plane, struct drm_framebuffer *fb,
			int crtc_x, int crtc_y,
			unsigned int crtc_w, unsigned int crtc_h,
			uint32_t src_x, uint32_t src_y,
			uint32_t src_w, uint32_t src_h);

void nx_drm_dp_crtc_commit(struct drm_crtc *crtc);
void nx_drm_dp_crtc_dpms(struct drm_crtc *crtc, int mode);

void nx_drm_dp_plane_init(struct drm_device *drm, struct drm_crtc *crtc,
			struct drm_plane *plane, int plane_num);
int nx_drm_dp_plane_update(struct drm_plane *plane,
			struct drm_framebuffer *fb,
			int crtc_x, int crtc_y,
			unsigned int crtc_w, unsigned int crtc_h,
			uint32_t src_x, uint32_t src_y,
			uint32_t src_w, uint32_t src_h);
void nx_drm_dp_plane_disable(struct drm_plane *plane);

void nx_drm_dp_display_mode_to_sync(struct drm_display_mode *mode,
			struct dp_sync_info *sync);

void nx_drm_dp_encoder_init(struct drm_encoder *encoder,
			int index, bool irqon);
void nx_drm_dp_encoder_commit(struct drm_encoder *encoder);

void nx_drm_dp_encoder_dpms(struct drm_encoder *encoder, bool poweron);
int nx_drm_dp_encoder_get_dpms(struct drm_encoder *encoder);

int nx_drm_dp_device_reset(struct drm_device *drm,
			struct nx_drm_priv *priv);

int nx_drm_dp_parse_dt_panel(struct device_node *np,
			struct dp_device_info *ddi);

void nx_drm_dp_parse_dp_dump(struct nx_drm_dp_dev *dp_dev);

int nx_drm_driver_parse_dt_setup(struct drm_device *drm,
			struct nx_drm_priv *priv);
int  nx_drm_dp_parse_dt_ctrl(struct device_node *np,
			struct dp_device_info *ddi);

#endif
