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

#ifdef DEBUG_FPS_TIME
#define	SHOW_PERIOD_SEC		3
#define	FPS_HZ	(60)
#define	DUMP_FPS_TIME(p) {	\
	static long ts[MAX_CRTCS] = { 0, }, vb_count;	\
	long new = ktime_to_ms(ktime_get());	\
	if (0 == (vb_count++ % (FPS_HZ * SHOW_PERIOD_SEC)))	\
		pr_info("[dp.%d] %ld ms\n", p, new - ts[p]);	\
	ts[p] = new;	\
	}
#else
#define	DUMP_FPS_TIME(p)
#endif

#define for_each_connector_in_drm(connector, dev) \
	for (connector =	\
		list_first_entry(&(dev)->mode_config.connector_list,	\
				struct drm_connector, head);	\
		&connector->head != (&(dev)->mode_config.connector_list); \
		connector = list_next_entry(connector, head))

static inline struct nx_drm_display *get_drm_display(struct drm_crtc *crtc)
{
	struct drm_device *drm = crtc->dev;
	struct nx_drm_display *display = NULL;
	struct drm_connector *connector;

	for_each_connector_in_drm(connector, drm) {
		if (!connector->state)
			continue;

		if (connector->state->crtc == crtc) {
			if (to_nx_connector(connector)->display != display)
				display = to_nx_connector(connector)->display;
			break;
		}
	}

	return display;
}

int nx_drm_vblank_enable(struct drm_device *drm, unsigned int pipe)
{
	struct nx_drm_private *private = drm->dev_private;
	struct nx_drm_display *display = get_drm_display(private->crtcs[pipe]);

	DRM_DEBUG_KMS("pipe.%d\n", pipe);

	if (!display)
		return 0;

	if (display->ops && display->ops->irq_enable)
		display->ops->irq_enable(display, pipe);

	return 0;
}

void nx_drm_vblank_disable(struct drm_device *drm, unsigned int pipe)
{
	struct nx_drm_private *private = drm->dev_private;
	struct nx_drm_display *display = get_drm_display(private->crtcs[pipe]);

	DRM_DEBUG_KMS("pipe.%d\n", pipe);

	if (!display)
		return;

	if (display->ops && display->ops->irq_disable)
		display->ops->irq_disable(display, pipe);
}

static irqreturn_t nx_drm_vblank_irq_handler(int irq, void *arg)
{
	struct drm_encoder *encoder = arg;
	struct nx_drm_connector *nx_connector =
				to_nx_encoder(encoder)->connector;
	struct drm_connector *connector = &nx_connector->connector;
	/* must be get from state */
	struct drm_crtc *crtc = connector->state->crtc;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct nx_drm_display *display = nx_connector->display;
	struct drm_pending_vblank_event *event = NULL;

	drm_crtc_handle_vblank(crtc);
	spin_lock(&crtc->dev->event_lock);

	event = nx_crtc->event;
	if (event) {
		drm_crtc_send_vblank_event(crtc, event);
		drm_crtc_vblank_put(crtc);
		nx_crtc->event = NULL;
	}

	spin_unlock(&crtc->dev->event_lock);

	DUMP_FPS_TIME(nx_crtc->pipe);

	if (WARN_ON(!display))
		return IRQ_NONE;

	/* done irq */
	if (display->ops && display->ops->irq_done)
		display->ops->irq_done(display, nx_crtc->pipe);

	return IRQ_HANDLED;
}

static int nx_drm_vblank_irq_request(struct drm_device *drm,
			struct drm_encoder *encoder)
{
	struct drm_crtc *crtc = encoder->crtc;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	unsigned long flags = IRQF_SHARED;
	int err;

	if (nx_crtc->irq == INVALID_IRQ || nx_crtc->irq_install)
		return 0;

	err = devm_request_irq(drm->dev, nx_crtc->irq,
				nx_drm_vblank_irq_handler,
				flags, dev_name(drm->dev), encoder);
	if (err < 0) {
		DRM_ERROR("Failed to request IRQ#%u: %d\n", nx_crtc->irq, err);
		nx_crtc->irq = INVALID_IRQ;
		return -EINVAL;
	}

	/*
	 * enable drm irq mode.
	 * - with irq_enabled = true, we can use the vblank feature.
	 */
	drm->irq_enabled = true;
	nx_crtc->irq_install = true;

	DRM_INFO("request CRTC.%d IRQ %d\n", nx_crtc->irq, nx_crtc->pipe);
	return err;
}

static void nx_drm_vblank_irq_free(struct drm_device *drm,
			struct drm_encoder *encoder)
{
	struct drm_crtc *crtc = encoder->crtc;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);

	if (!nx_crtc->irq_install)
		return;

	devm_free_irq(drm->dev, nx_crtc->irq, encoder);

	nx_crtc->irq_install = false;
	drm->irq_enabled = false;
	DRM_INFO("free CRTC.%d IRQ %d\n", nx_crtc->irq, nx_crtc->pipe);
}

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

	/*
	 * Link Encoder to CRTC
	 */
	DRM_DEBUG_KMS("encoder id:%d crtc.%d:%p\n",
		encoder->base.id, to_nx_crtc(encoder->crtc)->pipe,
		encoder->crtc);

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

		nx_drm_vblank_irq_request(encoder->dev, encoder);
		nx_encoder->enabled = true;
	}
}

static void nx_drm_encoder_disable(struct drm_encoder *encoder)
{
	struct drm_device *drm = encoder->dev;
	struct nx_drm_encoder *nx_encoder = to_nx_encoder(encoder);
	struct nx_drm_connector *nx_connector = nx_encoder->connector;

	DRM_DEBUG_KMS("encoder id:%d power %s\n",
		encoder->base.id, nx_encoder->enabled ? "on" : "off");

	if (nx_encoder->enabled) {
		nx_drm_vblank_irq_free(drm, encoder);

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
