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

#include "nx_drm_drv.h"
#include "nx_drm_crtc.h"
#include "nx_drm_encoder.h"
#include "nx_drm_connector.h"
#include "soc/s5pxx18_drm_dp.h"

static void nx_drm_encoder_dpms(struct drm_encoder *encoder, int mode)
{
	struct nx_drm_dp_dev *dp_dev = to_nx_encoder(encoder)->dp_dev;
	struct nx_drm_dev_ops *ops = dp_dev->ops;

	DRM_DEBUG_KMS("enter, encoder dpms: %d\n", mode);

	if (to_nx_encoder(encoder)->dpms == mode) {
		DRM_DEBUG_KMS("desired dpms mode is same as previous one.\n");
		return;
	}

	switch (mode) {
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
		if (ops && ops->dmps)
			ops->dmps(dp_dev->dev, mode);
		break;
	case DRM_MODE_DPMS_ON:
		nx_drm_dp_encoder_dpms(encoder, true);
		if (ops && ops->dmps)
			ops->dmps(dp_dev->dev, mode);
		break;
	case DRM_MODE_DPMS_OFF:
		if (ops && ops->dmps)
				ops->dmps(dp_dev->dev, mode);
		nx_drm_dp_encoder_dpms(encoder, false);
		break;

	default:
		DRM_ERROR("fail : unspecified mode %d\n", mode);
		break;
	}
	to_nx_encoder(encoder)->dpms = mode;
}

static bool nx_drm_encoder_mode_fixup(struct drm_encoder *encoder,
			const struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode)
{
	struct drm_device *drm = encoder->dev;
	struct drm_connector *connector;
	struct nx_drm_dp_dev *dp_dev = to_nx_encoder(encoder)->dp_dev;
	struct nx_drm_dev_ops *ops = dp_dev->ops;

	DRM_DEBUG_KMS("enter, encoder id:%d\n", encoder->base.id);

	list_for_each_entry(connector, &drm->mode_config.connector_list, head) {
		if (connector->encoder == encoder) {
			if (ops && ops->mode_fixup)
				return ops->mode_fixup(drm->dev,
						connector, mode, adjusted_mode);
		}
	}

	return true;
}

static void nx_drm_encoder_mode_set(struct drm_encoder *encoder,
			struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode)
{
	struct drm_device *drm = encoder->dev;
	struct drm_connector *connector;
	struct nx_drm_dp_dev *dp_dev = to_nx_encoder(encoder)->dp_dev;
	struct nx_drm_dev_ops *ops = dp_dev->ops;

	DRM_DEBUG_KMS("enter\n");

	list_for_each_entry(connector, &drm->mode_config.connector_list, head) {
		if (connector->encoder == encoder) {
			if (ops && ops->mode_set)
				ops->mode_set(drm->dev, adjusted_mode);
		}
	}

	nx_drm_dp_display_mode_to_sync(adjusted_mode, &dp_dev->ddi.sync);
}

static void nx_drm_encoder_prepare(struct drm_encoder *encoder)
{
	DRM_DEBUG_KMS("enter\n");
	/* drm framework doesn't check NULL. */
}

static void nx_drm_encoder_commit(struct drm_encoder *encoder)
{
	struct nx_drm_dp_dev *dp_dev = to_nx_encoder(encoder)->dp_dev;
	struct nx_drm_dev_ops *ops = dp_dev->ops;

	DRM_DEBUG_KMS("enter\n");

	if (ops && ops->commit)
		ops->commit(dp_dev->dev);

	nx_drm_dp_encoder_commit(encoder);
}

static struct drm_encoder_helper_funcs nx_encoder_helper_funcs = {
	.dpms = nx_drm_encoder_dpms,
	.mode_fixup = nx_drm_encoder_mode_fixup,
	.mode_set = nx_drm_encoder_mode_set,
	.prepare = nx_drm_encoder_prepare,
	.commit = nx_drm_encoder_commit,
};

static void nx_drm_encoder_destroy(struct drm_encoder *encoder)
{
	struct drm_device *drm = encoder->dev;
	struct nx_drm_encoder *nx_encoder = to_nx_encoder(encoder);
	struct nx_drm_dp_dev *dp_dev = to_nx_encoder(encoder)->dp_dev;

	if (nx_encoder->irq_install) {
		struct dp_device_info *ddi = &dp_dev->ddi;

		drm_irq_uninstall(drm);
		nx_soc_dp_device_irq_on(ddi, false);
	}

	drm_encoder_cleanup(encoder);
	devm_kfree(drm->dev, nx_encoder);
}

static struct drm_encoder_funcs nx_encoder_funcs = {
	.destroy = nx_drm_encoder_destroy,
};

static irqreturn_t nx_drm_irq(int irq, void *arg)
{
	struct drm_device *drm = arg;
	struct nx_drm_priv *priv;
	int pipe;

	priv = drm->dev_private;

	for (pipe = 0; pipe < priv->num_crtcs; pipe++) {
		struct drm_crtc *crtc = priv->crtcs[pipe];

		if (crtc) {
			if (irq == to_nx_crtc(crtc)->pipe_irq) {
				struct dp_device_info ddi = { .module = pipe };

				drm_handle_vblank(drm, pipe);

				/* clear hw pend */
				nx_soc_dp_device_irq_clear(&ddi);
				break;
			}
		}
	}

	return IRQ_HANDLED;
}

static int nx_drm_encoder_irq_install(struct drm_device *drm,
			struct drm_encoder *encoder)
{
	struct drm_driver *drv;
	struct nx_drm_priv *priv;
	struct nx_drm_dp_dev *dp_dev;
	int ret = 0;

	drv = drm->driver;
	priv = drm->dev_private;
	dp_dev = to_nx_encoder(encoder)->dp_dev;

	if (priv->num_irqs) {
		struct nx_drm_encoder *nx_encoder = to_nx_encoder(encoder);
		struct dp_device_info *ddi = &dp_dev->ddi;
		int pipe = ddi->module;
		int irq = priv->hw_irq_no[pipe];

		if (NULL == drv->irq_handler)
			drv->irq_handler = nx_drm_irq;

		ret = drm_irq_install(drm, irq);
		if (0 > ret)
			DRM_ERROR("fail : %d interruput %d !!!\n", pipe, irq);

		nx_encoder->irq_install = true;
		nx_soc_dp_device_irq_on(ddi, true);
		DRM_INFO("irq %d install for crtc.%d\n", irq, pipe);
	}

	return ret;
}

struct drm_encoder *nx_drm_encoder_create(struct drm_device *drm,
			struct nx_drm_dp_dev *dp_dev, int type,
			int pipe, int possible_crtcs, void *context)
{
	struct nx_drm_encoder *nx_encoder;
	struct drm_encoder *encoder;

	DRM_DEBUG_KMS("enter, encoder.%d, possible crtcs:%d\n",
		pipe, possible_crtcs);

	BUG_ON(!dp_dev || 0 == possible_crtcs);

	nx_encoder =
		devm_kzalloc(drm->dev, sizeof(*nx_encoder), GFP_KERNEL);
	if (!nx_encoder)
		return ERR_PTR(-ENOMEM);

	nx_encoder->dpms = DRM_MODE_DPMS_OFF;
	nx_encoder->pipe = pipe;
	nx_encoder->dp_dev = dp_dev;
	nx_encoder->context = context;

	encoder = &nx_encoder->encoder;
	encoder->possible_crtcs = possible_crtcs;

	nx_drm_dp_encoder_init(encoder, pipe, true);
	nx_drm_encoder_irq_install(drm, encoder);

	drm_encoder_init(drm, encoder, &nx_encoder_funcs, type);
	drm_encoder_helper_add(encoder, &nx_encoder_helper_funcs);

	DRM_DEBUG_KMS("exit, encoder id:%d\n", encoder->base.id);

	return encoder;
}
EXPORT_SYMBOL(nx_drm_encoder_create);

