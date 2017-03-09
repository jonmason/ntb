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
#include <linux/of_address.h>

#ifdef CONFIG_ARM_S5Pxx18_DEVFREQ
#include <linux/pm_qos.h>
#include <linux/soc/nexell/cpufreq.h>
#endif

#include "nx_drm_drv.h"

#ifdef CONFIG_ARM_S5Pxx18_DEVFREQ
static struct pm_qos_request pm_qos_req;

static inline void dev_qos_update(int khz)
{
	struct pm_qos_request *qos = &pm_qos_req;
	int active = pm_qos_request_active(qos);

	if (!active)
		pm_qos_add_request(qos, PM_QOS_BUS_THROUGHPUT, khz);
	else
		pm_qos_update_request(qos, khz);
}

static inline void dev_qos_remove(void)
{
	struct pm_qos_request *qos = &pm_qos_req;
	int active = pm_qos_request_active(qos);

	if (active)
		pm_qos_remove_request(qos);
}

static void nx_drm_qos_up(struct drm_encoder *encoder)
{
	int khz = NX_BUS_CLK_DISP_KHZ;

	DRM_DEBUG_KMS("[ENCODER:%d] qos up to %d khz\n",
		encoder->base.id, khz);
	dev_qos_update(khz);
}

static void nx_drm_qos_down(struct drm_encoder *encoder)
{
	struct drm_encoder *enc;
	struct drm_device *drm = encoder->dev;
	int khz = NX_BUS_CLK_IDLE_KHZ;
	bool power = false;

	list_for_each_entry(enc, &drm->mode_config.encoder_list, head) {
		power = to_nx_encoder(enc)->enabled;
		if (power)
			break;
	}

	if (!power) {
		DRM_DEBUG_KMS("[ENCODER:%d] qos idle to %d khz\n",
			encoder->base.id, khz);
		dev_qos_update(khz);
		dev_qos_remove();
	}
}
#else
#define nx_drm_qos_up(encoder)
#define nx_drm_qos_down(encoder)
#endif

static bool nx_drm_encoder_mode_fixup(struct drm_encoder *encoder,
			const struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode)
{
	DRM_DEBUG_KMS("encoder id:%d\n", encoder->base.id);
	return true;
}

static void nx_drm_encoder_mode_set(struct drm_encoder *encoder,
			struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode)
{
	struct drm_device *drm = encoder->dev;
	struct nx_drm_connector *nx_connector =
				to_nx_encoder(encoder)->connector;
	struct drm_connector *connector;

	DRM_DEBUG_KMS("encoder id:%d crtc:%p\n",
		encoder->base.id, encoder->crtc);

	list_for_each_entry(connector,
		&drm->mode_config.connector_list, head) {
		if (connector->encoder == encoder) {
			if (nx_connector->ops && nx_connector->ops->set_mode)
				nx_connector->ops->set_mode(
					nx_connector->dev, adjusted_mode);
		}
	}
}

static void nx_drm_encoder_enable(struct drm_encoder *encoder)
{
	struct nx_drm_encoder *nx_encoder = to_nx_encoder(encoder);
	struct nx_drm_connector *nx_connector = nx_encoder->connector;
	bool is_connected = false;

	if (nx_connector->ops && nx_connector->ops->is_connected)
		is_connected =
			nx_connector->ops->is_connected(nx_connector->dev);

	DRM_DEBUG_KMS("encoder id:%d %s, power %s\n",
		encoder->base.id, is_connected ? "connected" : "disconnected",
		nx_encoder->enabled ? "on" : "off");

	if (is_connected) {
		nx_drm_qos_up(encoder);
		/*
		 * enable : encoder -> connector driver
		 */
		if (nx_encoder->ops && nx_encoder->ops->enable)
			nx_encoder->ops->enable(encoder);

		if (nx_connector->ops && nx_connector->ops->enable)
			nx_connector->ops->enable(nx_connector->dev);

		nx_encoder->enabled = true;
	}
}

static void nx_drm_encoder_disable(struct drm_encoder *encoder)
{
	struct nx_drm_encoder *nx_encoder = to_nx_encoder(encoder);
	struct nx_drm_connector *nx_connector = nx_encoder->connector;

	DRM_DEBUG_KMS("encoder id:%d power %s\n",
		encoder->base.id, nx_encoder->enabled ? "on" : "off");

	if (nx_encoder->enabled) {
		/*
		 * disable : connector driver -> encoder
		 */
		if (nx_connector->ops && nx_connector->ops->disable)
			nx_connector->ops->disable(nx_connector->dev);

		if (nx_encoder->ops && nx_encoder->ops->disable)
			nx_encoder->ops->disable(encoder);

		nx_encoder->enabled = false;
		nx_drm_qos_down(encoder);
	}
}

static struct drm_encoder_helper_funcs nx_encoder_helper_funcs = {
	.mode_fixup = nx_drm_encoder_mode_fixup,
	.mode_set = nx_drm_encoder_mode_set,
	.enable = nx_drm_encoder_enable,
	.disable = nx_drm_encoder_disable,
};

static void nx_drm_encoder_destroy(struct drm_encoder *encoder)
{
	struct nx_drm_encoder *nx_encoder = to_nx_encoder(encoder);

	DRM_DEBUG_KMS("enter\n");

	if (nx_encoder->ops && nx_encoder->ops->destroy)
		nx_encoder->ops->destroy(encoder);

	drm_encoder_cleanup(encoder);
	kfree(nx_encoder);
}

static struct drm_encoder_funcs nx_encoder_funcs = {
	.destroy = nx_drm_encoder_destroy,
};

struct drm_encoder *nx_drm_encoder_create(struct drm_device *drm,
			struct drm_connector *connector, int enc_type,
			int pipe, int possible_crtcs)
{
	struct nx_drm_encoder *nx_encoder;
	struct drm_encoder *encoder;

	DRM_DEBUG_KMS("pipe.%d crtc mask:0x%x\n", pipe, possible_crtcs);

	if (WARN_ON(!connector || 0 == possible_crtcs))
		return NULL;

	nx_encoder = kzalloc(sizeof(*nx_encoder), GFP_KERNEL);
	if (!nx_encoder)
		return ERR_PTR(-ENOMEM);

	nx_encoder->connector = to_nx_connector(connector);
	encoder = &nx_encoder->encoder;
	encoder->possible_crtcs = possible_crtcs;

	nx_drm_encoder_init(encoder);
	drm_encoder_init(drm, encoder, &nx_encoder_funcs, enc_type);
	drm_encoder_helper_add(encoder, &nx_encoder_helper_funcs);

	return encoder;
}
