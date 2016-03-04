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
#include "nx_drm_crtc.h"
#include "nx_drm_encoder.h"

static void nx_drm_display_power(struct drm_encoder *encoder, int mode)
{
	struct drm_device *drm = encoder->dev;
	struct drm_connector *connector;
	struct nx_drm_display *display = nx_drm_get_display(encoder);

	list_for_each_entry(connector, &drm->mode_config.connector_list, head) {
		if (connector->encoder == encoder) {
			struct nx_drm_operation *ops = display->ops;

			DRM_DEBUG_KMS("connector[%d] dpms[%d]\n",
				      connector->base.id, mode);
			if (ops && ops->power_on)
				ops->power_on(display->dev, mode);
		}
	}
}

static void nx_drm_encoder_dpms(struct drm_encoder *encoder, int mode)
{
	struct drm_device *drm = encoder->dev;
	struct nx_drm_display *display = nx_drm_get_display(encoder);
	struct nx_drm_operation *ops = display->ops;
	struct nx_drm_encoder *nx_encoder = to_nx_encoder(encoder);

	DRM_DEBUG_KMS("enter, encoder dpms: %d\n", mode);

	if (nx_encoder->dpms == mode) {
		DRM_DEBUG_KMS("desired dpms mode is same as previous one.\n");
		return;
	}

	mutex_lock(&drm->struct_mutex);

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		if (ops && ops->apply)
			ops->apply(display->dev);
		nx_drm_display_power(encoder, mode);
		nx_encoder->dpms = mode;
		break;
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		nx_drm_display_power(encoder, mode);
		nx_encoder->dpms = mode;
		break;
	default:
		DRM_ERROR("unspecified mode %d\n", mode);
		break;
	}

	mutex_unlock(&drm->struct_mutex);
}

static bool nx_drm_encoder_mode_fixup(struct drm_encoder *encoder,
				const struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
	struct drm_device *drm = encoder->dev;
	struct drm_connector *connector;
	struct nx_drm_display *display = nx_drm_get_display(encoder);
	struct nx_drm_operation *ops = display->ops;

	DRM_DEBUG_KMS("enter\n");

	list_for_each_entry(connector, &drm->mode_config.connector_list, head) {
		if (connector->encoder == encoder)
			if (ops && ops->mode_fixup)
				ops->mode_fixup(display->dev, connector, mode,
						adjusted_mode);
	}

	return true;
}

static void nx_drm_encoder_mode_set(struct drm_encoder *encoder,
				struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
	struct drm_device *drm = encoder->dev;
	struct drm_connector *connector;
	struct nx_drm_display *display = nx_drm_get_display(encoder);
	struct nx_drm_operation *ops = display->ops;

	DRM_DEBUG_KMS("enter\n");

	list_for_each_entry(connector, &drm->mode_config.connector_list, head) {
		if (connector->encoder == encoder) {
			if (ops && ops->mode_set)
				ops->mode_set(display->dev, adjusted_mode);
		}
	}
}

static void nx_drm_encoder_prepare(struct drm_encoder *encoder)
{
	DRM_DEBUG_KMS("enter\n");

	/* drm framework doesn't check NULL. */
}

static void nx_drm_encoder_commit(struct drm_encoder *encoder)
{
	struct drm_device *drm = encoder->dev;
	struct nx_drm_display *display = nx_drm_get_display(encoder);
	struct nx_drm_operation *ops = display->ops;

	DRM_DEBUG_KMS("enter\n");

	if (ops && ops->commit)
		ops->commit(drm, display->dev);
}

static struct drm_crtc *nx_drm_encoder_get_crtc(struct drm_encoder *encoder)
{
	DRM_DEBUG_KMS("enter\n");
	return encoder->crtc;
}

static struct drm_encoder_helper_funcs nx_encoder_helper_funcs = {
	.dpms = nx_drm_encoder_dpms,
	.mode_fixup = nx_drm_encoder_mode_fixup,
	.mode_set = nx_drm_encoder_mode_set,
	.prepare = nx_drm_encoder_prepare,
	.commit = nx_drm_encoder_commit,
	.get_crtc = nx_drm_encoder_get_crtc,
};

static void nx_drm_encoder_destroy(struct drm_encoder *encoder)
{
	struct nx_drm_encoder *nx_encoder = to_nx_encoder(encoder);

	DRM_DEBUG_KMS("enter\n");

	nx_encoder->display->pipe = -1;

	drm_encoder_cleanup(encoder);
	kfree(nx_encoder);
}

static struct drm_encoder_funcs nx_encoder_funcs = {
	.destroy = nx_drm_encoder_destroy,
};

/* DRM encoder slave functions */
#ifdef	DRM_ENCODER_SLAVE_SUPPORT
static void nx_encoder_set_config(struct drm_encoder *encoder, void *params)
{
	DRM_DEBUG_KMS("enter\n");
}

static void nx_encoder_dpms(struct drm_encoder *encoder, int mode)
{
	DRM_DEBUG_KMS("enter\n");
}

static void nx_encoder_save(struct drm_encoder *encoder)
{
	DRM_DEBUG_KMS("enter\n");
}

static void nx_encoder_restore(struct drm_encoder *encoder)
{
	DRM_DEBUG_KMS("enter\n");
}

static bool nx_encoder_mode_fixup(struct drm_encoder *encoder,
				const struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
	DRM_DEBUG_KMS("enter\n");
	return true;
}

static int nx_encoder_mode_valid(struct drm_encoder *encoder,
				struct drm_display_mode *mode)
{
	DRM_DEBUG_KMS("enter\n");
	return MODE_OK;
}

static void nx_encoder_mode_set(struct drm_encoder *encoder,
				struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
	DRM_DEBUG_KMS("enter\n");
	nx_encoder_dpms(encoder, DRM_MODE_DPMS_ON);
}

static enum drm_connector_status nx_encoder_detect(struct drm_encoder *encoder,
				struct drm_connector *connector)
{
	DRM_DEBUG_KMS("enter\n");
	return connector_status_connected;
}

static int nx_encoder_get_modes(struct drm_encoder *encoder,
				struct drm_connector *connector)
{
	DRM_DEBUG_KMS("enter\n");
	return 0;
}

static int nx_encoder_create_resources(struct drm_encoder *encoder,
				struct drm_connector *connector)
{
	DRM_DEBUG_KMS("enter\n");
	return 0;
}

static int nx_encoder_set_property(struct drm_encoder *encoder,
					struct drm_connector *connector,
					struct drm_property *property,
					uint64_t val)
{
	DRM_DEBUG_KMS("enter\n");
	return 0;
}

static void nx_encoder_destroy(struct drm_encoder *encoder)
{
	DRM_DEBUG_KMS("enter\n");
}

static struct drm_encoder_slave_funcs nx_encoder_slave_funcs = {
	.set_config = nx_encoder_set_config,
	.destroy = nx_encoder_destroy,
	.dpms = nx_encoder_dpms,
	.save = nx_encoder_save,
	.restore = nx_encoder_restore,
	.mode_fixup = nx_encoder_mode_fixup,
	.mode_valid = nx_encoder_mode_valid,
	.mode_set = nx_encoder_mode_set,
	.detect = nx_encoder_detect,
	.get_modes = nx_encoder_get_modes,
	.create_resources = nx_encoder_create_resources,
	.set_property = nx_encoder_set_property,
};
#endif

static unsigned int nx_drm_encoder_clones(struct drm_encoder *encoder)
{
	struct drm_encoder *clone;
	struct drm_device *drm = encoder->dev;
	struct nx_drm_encoder *nx_encoder = to_nx_encoder(encoder);
	struct nx_drm_operation *ops = nx_encoder->display->ops;
	unsigned int clone_mask = 0;
	int cnt = 0;

	list_for_each_entry(clone, &drm->mode_config.encoder_list, head) {
		switch (ops->type) {
		case NX_DISPLAY_TYPE_LCD:
		case NX_DISPLAY_TYPE_HDMI:
		case NX_DISPLAY_TYPE_VIDI:
			clone_mask |= (1 << (cnt++));
			break;
		default:
			continue;
		}
	}
	return clone_mask;
}

void nx_drm_encoder_setup(struct drm_device *drm)
{
	struct drm_encoder *encoder;

	DRM_DEBUG_KMS("enter\n");

	list_for_each_entry(encoder, &drm->mode_config.encoder_list, head)
		encoder->possible_clones = nx_drm_encoder_clones(encoder);
}

struct nx_drm_display *nx_drm_get_display(struct drm_encoder *encoder)
{
	return to_nx_encoder(encoder)->display;
}

void nx_drm_enable_vblank(struct drm_encoder *encoder, void *data)
{
	struct nx_drm_display *display = to_nx_encoder(encoder)->display;
	struct nx_drm_operation *ops = display->ops;
	int crtc = *(int *)data;

	if (display->pipe == -1)
		display->pipe = crtc;

	if (ops->enable_vblank)
		ops->enable_vblank(display->dev);
}

void nx_drm_disable_vblank(struct drm_encoder *encoder, void *data)
{
	struct nx_drm_display *display = to_nx_encoder(encoder)->display;
	struct nx_drm_operation *ops = display->ops;
	int crtc = *(int *)data;

	if (display->pipe == -1)
		display->pipe = crtc;

	if (ops->disable_vblank)
		ops->disable_vblank(display->dev);
}

void nx_drm_encoder_crtc_plane_commit(struct drm_encoder *encoder, void *data)
{
	DRM_DEBUG_KMS("enter\n");
}

void nx_drm_encoder_crtc_commit(struct drm_encoder *encoder, void *data)
{
	struct nx_drm_display *display = to_nx_encoder(encoder)->display;
	int crtc = *(int *)data;
	int zpos = DEFAULT_ZPOS;

	DRM_DEBUG_KMS("enter\n");

	/*
	 * when crtc is detached from encoder, this pipe is used
	 * to select display operation
	 */
	display->pipe = crtc;

	nx_drm_encoder_crtc_plane_commit(encoder, &zpos);
}

void nx_drm_encoder_dpms_from_crtc(struct drm_encoder *encoder, void *data)
{
	struct nx_drm_encoder *nx_encoder = to_nx_encoder(encoder);
	int mode = *(int *)data;

	DRM_DEBUG_KMS("enter\n");

	nx_drm_encoder_dpms(encoder, mode);

	nx_encoder->dpms = mode;
}

void nx_drm_encoder_crtc_dpms(struct drm_encoder *encoder, void *data)
{
	struct drm_device *drm = encoder->dev;
	struct nx_drm_encoder *nx_encoder = to_nx_encoder(encoder);
	struct nx_drm_display *display = nx_encoder->display;
	struct nx_drm_operation *ops = display->ops;
	struct drm_connector *connector;
	int mode = *(int *)data;

	DRM_DEBUG_KMS("enter\n");

	if (ops && ops->dpms)
		ops->dpms(display->dev, mode);

	/*
	 * set current dpms mode to the connector connected to
	 * current encoder. connector->dpms would be checked
	 * at drm_helper_connector_dpms()
	 */
	list_for_each_entry(connector, &drm->mode_config.connector_list, head)
		if (connector->encoder == encoder)
			connector->dpms = mode;

	/*
	 * if this condition is ok then it means that the crtc is already
	 * detached from encoder and last function for detaching is properly
	 * done, so clear pipe from display to prevent repeated call.
	 */
	if (mode > DRM_MODE_DPMS_ON) {
		if (!encoder->crtc)
			display->pipe = -1;
	}
}

void nx_drm_encoder_crtc_mode_set(struct drm_encoder *encoder, void *data)
{
	DRM_DEBUG_KMS("enter\n");
}

void nx_drm_encoder_crtc_disable(struct drm_encoder *encoder, void *data)
{
	DRM_DEBUG_KMS("enter\n");
}

struct drm_encoder *nx_drm_encoder_create(struct drm_device *drm,
					  struct nx_drm_display *display,
					  int possible_crtcs, void *context)
{
	struct nx_drm_encoder *nx_encoder;
	struct drm_encoder *encoder;

	DRM_DEBUG_KMS("enter, possible CRTCs:%d\n", possible_crtcs);

	if (!display || !possible_crtcs)
		return NULL;

	nx_encoder = kzalloc(sizeof(*nx_encoder), GFP_KERNEL);
	if (!nx_encoder) {
		DRM_ERROR("failed to allocate encoder\n");
		return NULL;
	}

	nx_encoder->dpms = DRM_MODE_DPMS_OFF;
	nx_encoder->display = display;
	nx_encoder->context = context;

#ifdef	DRM_ENCODER_SLAVE_SUPPORT
	nx_encoder->drm_encoder.slave_funcs = &nx_encoder_slave_funcs;
	encoder = &nx_encoder->drm_encoder.base;
#else
	encoder = &nx_encoder->drm_encoder;
#endif
	encoder->possible_crtcs = possible_crtcs;

	drm_encoder_init(drm, encoder, &nx_encoder_funcs,
			 DRM_MODE_ENCODER_TMDS);

	drm_encoder_helper_add(encoder, &nx_encoder_helper_funcs);

	DRM_DEBUG_KMS("exit, encoder ID:%d\n", encoder->base.id);

	return encoder;
}
EXPORT_SYMBOL(nx_drm_encoder_create);


