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

#include "nx_drm_drv.h"
#include "nx_drm_connector.h"
#include "nx_drm_encoder.h"
#include "soc/s5pxx18_drm_dp.h"

static int nx_drm_connector_get_modes(struct drm_connector *connector)
{
	struct nx_drm_dp_dev *dp_dev = to_nx_connector(connector)->dp_dev;
	struct nx_drm_dp_ops *ops = dp_dev->ops;

	DRM_DEBUG_KMS("enter\n");

	if (ops && ops->get_modes)
		return ops->get_modes(dp_dev->dev, connector);

	DRM_ERROR("fail : create a new display mode.\n");
	return 0;
}

static int nx_drm_connector_mode_valid(struct drm_connector *connector,
			struct drm_display_mode *mode)
{
	struct nx_drm_dp_dev *dp_dev = to_nx_connector(connector)->dp_dev;
	struct nx_drm_dp_ops *ops = dp_dev->ops;

	DRM_DEBUG_KMS("enter\n");
	DRM_DEBUG_KMS("bpp specified : %s, %d\n",
		connector->cmdline_mode.bpp_specified ? "yes" : "no",
		connector->cmdline_mode.bpp);

	if (ops && ops->check_mode)
		return ops->check_mode(dp_dev->dev, mode);

	return MODE_BAD;
}

struct drm_encoder *nx_drm_best_encoder(struct drm_connector *connector)
{
	/*
	struct drm_device *drm = connector->dev;
	*/
	struct nx_drm_connector *nx_connector = to_nx_connector(connector);
	struct drm_encoder *encoder = nx_connector->connector.encoder;

	if (encoder) {
		DRM_DEBUG_KMS("encoodr id:%d (enc.%d)\n",
			encoder->base.id, to_nx_encoder(encoder)->pipe);
	}

	DRM_DEBUG_KMS("connector id:%d\n", connector->base.id);
	return encoder;
}

static struct drm_connector_helper_funcs nx_drm_connector_helper_funcs = {
	.get_modes = nx_drm_connector_get_modes,
	.mode_valid = nx_drm_connector_mode_valid,
	.best_encoder = nx_drm_best_encoder,
};

static enum drm_connector_status nx_drm_connector_detect(
			struct drm_connector *connector, bool force)
{
	struct nx_drm_connector *nx_connector = to_nx_connector(connector);
	struct nx_drm_dp_dev *dp_dev = nx_connector->dp_dev;
	struct nx_drm_dp_ops *ops = dp_dev->ops;
	enum drm_connector_status status = connector_status_disconnected;

	if (ops && ops->is_connected) {
		if (ops->is_connected(dp_dev->dev, connector))
			status = connector_status_connected;
		else
			status = connector_status_disconnected;
	}

	DRM_DEBUG_KMS("status: %s\n",
		status == connector_status_connected ? "connected" :
	    "disconnected");

	return status;
}

static void nx_drm_connector_destroy(struct drm_connector *connector)
{
	struct nx_drm_connector *nx_connector = to_nx_connector(connector);
	struct drm_device *drm = connector->dev;

	DRM_DEBUG_KMS("enter\n");

	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
	devm_kfree(drm->dev, nx_connector);
}

static int nx_drm_connector_dpms(struct drm_connector *connector, int mode)
{
	DRM_DEBUG_KMS("enter\n");
	return drm_helper_connector_dpms(connector, mode);
}

static struct drm_connector_funcs nx_drm_connector_funcs = {
	.dpms = nx_drm_connector_dpms,
	.detect = nx_drm_connector_detect,
	.destroy = nx_drm_connector_destroy,
	.fill_modes = drm_helper_probe_single_connector_modes,
};

void nx_drm_connector_destroy_and_detach(struct drm_connector *connector)
{
	struct drm_encoder *encoder;

	BUG_ON(!connector);
	encoder = connector->encoder;

	if (encoder)
		encoder->funcs->destroy(encoder);

	if (connector)
		connector->funcs->destroy(connector);
}
EXPORT_SYMBOL(nx_drm_connector_destroy_and_detach);

int nx_drm_connector_create_and_attach(struct drm_device *drm,
			struct nx_drm_dp_dev *dp_dev, int to_encoder,
			void *context)
{
	struct nx_drm_priv *priv = drm->dev_private;
	struct nx_drm_connector *nx_connector;
	struct drm_connector *connector;
	struct drm_encoder *encoder;

	/* bitmask of potential CRTC bindings */
	int crtc_mask = (1 << priv->num_crtcs) - 1;
	int panel_type, con_type = 0, enc_type = 0;
	bool interlace_allowed = false;
	uint8_t polled = 0;
	int err;

	DRM_DEBUG_KMS("enter\n");

	BUG_ON(!dp_dev);
	panel_type = dp_dev->ddc.panel_type;

	switch (panel_type) {
	case dp_panel_type_lcd:
		con_type = DRM_MODE_CONNECTOR_VGA;
		enc_type = DRM_MODE_ENCODER_TMDS;
		break;
	case dp_panel_type_lvds:
		con_type = DRM_MODE_CONNECTOR_LVDS;
		enc_type = DRM_MODE_ENCODER_LVDS;
		break;
	case dp_panel_type_mipi:	/* MiPi DSI */
		con_type = DRM_MODE_CONNECTOR_DSI;
		enc_type = DRM_MODE_ENCODER_DSI;
		break;
	case dp_panel_type_hdmi:
		con_type = DRM_MODE_CONNECTOR_HDMIA;
		interlace_allowed = true;
		polled = DRM_CONNECTOR_POLL_HPD;
		break;
	case dp_panel_type_vidi:
		con_type = DRM_MODE_CONNECTOR_VIRTUAL;
		enc_type = DRM_MODE_ENCODER_VIRTUAL;
		polled = DRM_CONNECTOR_POLL_HPD;
		break;
	default:
		con_type = DRM_MODE_CONNECTOR_Unknown;
		DRM_ERROR("fail : unknown drm connector type(%d)\n",
			panel_type);
		return -EINVAL;
	}

	nx_connector =
		devm_kzalloc(drm->dev, sizeof(*nx_connector), GFP_KERNEL);
	if (IS_ERR(nx_connector))
		return -ENOMEM;

	connector = &nx_connector->connector;
	connector->polled = polled;
	connector->interlace_allowed = interlace_allowed;

	/* create encoder */
	encoder = nx_drm_encoder_create(drm, dp_dev, enc_type,
					to_encoder, crtc_mask, context);
	if (IS_ERR(encoder))
		goto err_alloc;

	/* create connector and attach */
	drm_connector_helper_add(connector, &nx_drm_connector_helper_funcs);
	drm_connector_init(drm, connector, &nx_drm_connector_funcs, con_type);
	err = drm_connector_register(connector);
	if (err)
		goto err_encoder;

	connector->encoder = encoder;
	err = drm_mode_connector_attach_encoder(connector, encoder);
	if (err) {
		DRM_ERROR("fail : attach a connector to a encoder\n");
		goto err_connector;
	}

	nx_connector->dp_dev = dp_dev;
	nx_connector->context = context;

	/* inititalize dpms status */
	connector->dpms = nx_drm_dp_encoder_get_dpms(encoder);

	DRM_DEBUG_KMS("done, encoder id:%d , connector id:%d, dpms %s\n",
		encoder->base.id, connector->base.id,
		connector->dpms == DRM_MODE_DPMS_ON ? "on" : "off");

	return 0;

err_connector:
	drm_connector_unregister(connector);
err_encoder:
	drm_connector_cleanup(connector);
	if (encoder)
		encoder->funcs->destroy(encoder);
err_alloc:
	devm_kfree(drm->dev, nx_connector);

	return -EINVAL;
}
EXPORT_SYMBOL(nx_drm_connector_create_and_attach);

