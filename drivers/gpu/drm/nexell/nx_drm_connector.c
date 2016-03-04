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
#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>

#include <drm/nexell_drm.h>
#include "nx_drm_drv.h"
#include "nx_drm_connector.h"
#include "nx_drm_encoder.h"

#define MAX_EDID 256

/* convert drm_display_mode to nx_video_timings */
static inline void convert_to_video_timing(struct fb_videomode *timing,
				struct drm_display_mode *mode)
{
	DRM_DEBUG_KMS("enter\n");

	memset(timing, 0, sizeof(*timing));

	timing->pixclock = mode->clock * 1000;
	timing->refresh = drm_mode_vrefresh(mode);

	timing->xres = mode->hdisplay;
	timing->right_margin = mode->hsync_start - mode->hdisplay;
	timing->hsync_len = mode->hsync_end - mode->hsync_start;
	timing->left_margin = mode->htotal - mode->hsync_end;

	timing->yres = mode->vdisplay;
	timing->lower_margin = mode->vsync_start - mode->vdisplay;
	timing->vsync_len = mode->vsync_end - mode->vsync_start;
	timing->upper_margin = mode->vtotal - mode->vsync_end;

	if (mode->flags & DRM_MODE_FLAG_INTERLACE)
		timing->vmode = FB_VMODE_INTERLACED;
	else
		timing->vmode = FB_VMODE_NONINTERLACED;

	if (mode->flags & DRM_MODE_FLAG_DBLSCAN)
		timing->vmode |= FB_VMODE_DOUBLE;
}

static int nx_drm_connector_get_modes(struct drm_connector *connector)
{
	struct nx_drm_connector *nx_connector = to_nx_connector(connector);
	struct nx_drm_display *display = nx_connector->display;
	struct edid *edid;
	unsigned int count = 0;

	DRM_DEBUG_KMS("enter\n");

	if (!display || !display->ops)
		return 0;

	if (display->ops->get_edid) {
		edid = NULL;
		pr_info("[%s: EDID unimplemented]\n", __func__);
	} else {
		if (display->ops->get_mode)
			count = display->ops->get_mode(connector);
	}

	DRM_DEBUG_DRIVER("exit, count:%d\n", count);
	return count;
}

static int nx_drm_connector_mode_valid(struct drm_connector *connector,
				struct drm_display_mode *mode)
{
	struct nx_drm_connector *nx_connector = to_nx_connector(connector);
	struct nx_drm_display *display = nx_connector->display;
	struct nx_drm_operation *ops = display->ops;
	struct fb_videomode timing;
	int ret = MODE_BAD;

	DRM_DEBUG_KMS("enter\n");

	convert_to_video_timing(&timing, mode);

	if (ops && ops->check_timing)
		if (!ops->check_timing(display->dev, (void *)&timing))
			ret = MODE_OK;

	return ret;
}

struct drm_encoder *nx_drm_best_encoder(struct drm_connector *connector)
{
	/*
	struct drm_device *drm = connector->dev;
	*/
	struct nx_drm_connector *nx_connector = to_nx_connector(connector);
	struct drm_encoder *encoder = nx_connector->connector.encoder;

	DRM_DEBUG_KMS("enter connector ID:%d\n", connector->base.id);
	return encoder;
}

static struct drm_connector_helper_funcs nx_drm_connector_helper_funcs = {
	.get_modes = nx_drm_connector_get_modes,
	.mode_valid = nx_drm_connector_mode_valid,
	.best_encoder = nx_drm_best_encoder,
};

/* get detection status of display device. */
static enum drm_connector_status nx_drm_connector_detect(
				struct drm_connector *connector, bool force)
{
	struct nx_drm_connector *nx_connector = to_nx_connector(connector);
	struct nx_drm_display *display = nx_connector->display;
	struct nx_drm_operation *ops = display->ops;
	enum drm_connector_status status = connector_status_disconnected;

	DRM_DEBUG_KMS("enter\n");

	if (ops && ops->is_connected) {
		if (ops->is_connected(display->dev))
			status = connector_status_connected;
		else
			status = connector_status_disconnected;
	}

	DRM_DEBUG_KMS("exit, status:%d (%s)\n",
	      status, status == connector_status_connected ? "connected" :
	      "disconnected");

	return status;
}

static void nx_drm_connector_destroy(struct drm_connector *connector)
{
	struct nx_drm_connector *nx_connector = to_nx_connector(connector);

	DRM_DEBUG_KMS("enter\n");

	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
	kfree(nx_connector);
}

static void nx_drm_connector_dpms(struct drm_connector *connector, int mode)
{
	DRM_DEBUG_KMS("enter\n");
	drm_helper_connector_dpms(connector, mode);
}

static struct drm_connector_funcs nx_drm_connector_funcs = {
	.dpms = nx_drm_connector_dpms,
	.detect = nx_drm_connector_detect,
	.destroy = nx_drm_connector_destroy,
	.fill_modes = drm_helper_probe_single_connector_modes,
};

struct drm_connector *nx_drm_connector_create(struct drm_device *drm,
				struct drm_encoder *encoder,
				void *context)
{
	struct nx_drm_connector *nx_connector;
	struct nx_drm_display *display = nx_drm_get_display(encoder);
	struct drm_connector *connector;
	int dev_type = display->ops->type;
	int type;
	int err;

	DRM_DEBUG_KMS("enter\n");

	nx_connector = kzalloc(sizeof(*nx_connector), GFP_KERNEL);
	if (!nx_connector) {
		DRM_ERROR("failed to allocate connector\n");
		return NULL;
	}
	connector = &nx_connector->connector;

	switch (dev_type) {
	case NX_DISPLAY_TYPE_HDMI:
		type = DRM_MODE_CONNECTOR_HDMIA;
		connector->interlace_allowed = true;
		connector->polled = DRM_CONNECTOR_POLL_HPD;
		break;
	case NX_DISPLAY_TYPE_VIDI:
		type = DRM_MODE_CONNECTOR_VIRTUAL;
		connector->polled = DRM_CONNECTOR_POLL_HPD;
		break;
	case NX_DISPLAY_TYPE_LVDS:
		type = DRM_MODE_CONNECTOR_LVDS;
		break;
	case NX_DISPLAY_TYPE_LCD:
		type = DRM_MODE_CONNECTOR_DSI;
		break;
	default:
		type = DRM_MODE_CONNECTOR_Unknown;
		break;
	}

	drm_connector_init(drm, connector, &nx_drm_connector_funcs, type);
	drm_connector_helper_add(connector, &nx_drm_connector_helper_funcs);

	err = drm_connector_register(connector);
	if (err)
		goto err_connector;

	nx_connector->display = display;
	nx_connector->context = context;
	connector->encoder = encoder;

	err = drm_mode_connector_attach_encoder(connector, encoder);
	if (err) {
		DRM_ERROR("failed to attach a connector to a encoder\n");
		goto err_sysfs;
	}

	DRM_DEBUG_KMS("exit, connector ID:%d\n", connector->base.id);

	return connector;

err_sysfs:
	drm_connector_unregister(connector);
err_connector:
	drm_connector_cleanup(connector);
	kfree(nx_connector);

	return NULL;
}
EXPORT_SYMBOL(nx_drm_connector_create);
