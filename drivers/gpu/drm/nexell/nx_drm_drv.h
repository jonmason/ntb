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
#ifndef _NX_DRM_DRV_H_
#define _NX_DRM_DRV_H_

#include <linux/module.h>
#include <drm/nexell_drm.h>

#define MAX_CRTC		1	/* display controller 0, 1 */
#define MAX_PLANE		(3*MAX_CRTC) /* MLC Layer */
#define MAX_FB_PLANE	4
#define DEFAULT_ZPOS	-1

#define MAX_FB_MODE_WIDTH	4096
#define MAX_FB_MODE_HEIGHT	4096

struct drm_device;
struct nx_drm_plane;
struct drm_connector;

/* this enumerates display type. */
enum dsi_outdev_type {
	NX_DISPLAY_TYPE_NONE,
	/* RGB or CPU Interface. */
	NX_DISPLAY_TYPE_LCD,
	/* HDMI Interface. */
	NX_DISPLAY_TYPE_HDMI,
	/* Virtual Display Interface. */
	NX_DISPLAY_TYPE_VIDI,
	/* LVDS Interface. */
	NX_DISPLAY_TYPE_LVDS,
	/* MIPI Interface. */
	NX_DISPLAY_TYPE_MIPI,
};

/*
 * drm overlay ops structure.
 */
struct disp_overlay_ops {
	void (*mode_set)(struct device *dev, struct nx_drm_plane *overlay);
	void (*commit)(struct device *dev, int zpos);
	void (*disable)(struct device *dev, int zpos);
};

/*
 * drm device ops
 */
struct nx_drm_operation {
	enum dsi_outdev_type type;
	bool (*is_connected)(struct device *dev);

	int (*check_timing)(struct device *dev, void *timing);
	int (*power_on)(struct device *dev, int mode);

	void (*dpms)(struct device *dev, int mode);
	void (*apply)(struct device *dev);
	void (*mode_fixup)(struct device *dev,
			    struct drm_connector *connector,
			    const struct drm_display_mode *mode,
			    struct drm_display_mode *adjusted_mode);
	void (*mode_set)(struct device *dev, void *mode);
	void (*get_max_resol)(struct device *dev, unsigned int *width,
			       unsigned int *height);
	void (*commit)(struct drm_device *drm, struct device *dev);
	int (*enable_vblank)(struct device *dev);
	void (*disable_vblank)(struct device *dev);
	int (*get_edid)(struct device *dev, struct drm_connector *connector,
			 u8 *edid, int len);
	int (*get_mode)(struct drm_connector *connector);
};

struct nx_drm_private {
	struct drm_fb_helper *fb_helper;
	struct drm_crtc *crtc[MAX_CRTC];
	int num_crtcs;
};

struct nx_drm_display {
	struct device *dev;	/* display device : LVDS, HDMI, ... */
	int pipe;
	struct nx_drm_operation *ops;
};

#define	DRM_INIT_DISPLAY_DEV(c, d)	{ c->dev = dev; }

extern struct platform_driver fimd_driver;
extern struct platform_driver hdmi_driver;
extern struct platform_driver mixer_driver;
extern struct platform_driver vidi_driver;

#endif
