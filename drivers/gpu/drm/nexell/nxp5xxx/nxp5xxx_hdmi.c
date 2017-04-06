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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/regmap.h>
#include <linux/hdmi.h>
#include <linux/of.h>

#include "nxp5xxx_drv.h"

static int hdmi_ops_open(struct nx_drm_display *display, int pipe)
{
	struct nx_hdmi_dev *hdmi = display->context;
	struct nxs_dev *nxs = hdmi->control.s_dev;

	return nxs->open(nxs);
}

static int hdmi_ops_enable(struct nx_drm_display *display)
{
	struct nx_hdmi_dev *hdmi = display->context;
	struct nxs_dev *nxs = hdmi->control.s_dev;

	return nxs->start(nxs);
}

static int hdmi_ops_disable(struct nx_drm_display *display)
{
	struct nx_hdmi_dev *hdmi = display->context;
	struct nxs_dev *nxs = hdmi->control.s_dev;

	nxs->stop(nxs);

	return 0;
}

static int hdmi_ops_is_connected(struct nx_drm_display *display)
{
	struct nx_hdmi_dev *hdmi = display->context;
	struct nxs_dev *nxs = hdmi->control.s_dev;
	enum nxs_control_type type = NXS_CONTROL_STATUS;
	struct nxs_control control = { .type = NXS_DISPLAY_STATUS_CONNECT, };

	return nxs_get_control(nxs, type, &control) ? 1 : 0;
}

static u32 hdmi_ops_hpd_status(struct nx_drm_display *display)
{
	u32 ret = hdmi_ops_is_connected(display);

	return ret ? HDMI_EVENT_PLUG : HDMI_EVENT_UNPLUG;
}

static void hdmi_ops_irq_done(struct nx_drm_display *display,
			unsigned int type)
{
	struct nx_hdmi_dev *hdmi = display->context;
	struct nxs_dev *nxs = hdmi->control.s_dev;

	nxs->clear_interrupt_pending(nxs, 0);
}

static int hdmi_ops_is_valid(struct nx_drm_display *display,
			struct drm_display_mode *mode)
{
	struct nx_hdmi_dev *hdmi = display->context;
	struct nxs_dev *nxs = hdmi->control.s_dev;
	enum nxs_control_type type = NXS_CONTROL_STATUS;
	struct nxs_control control = { .type = NXS_DISPLAY_STATUS_MODE, };
	int err;

	control.u.sync_info.pixelclock = mode->clock * 1000;

	pr_debug("[%s] %4d x %4d%s, %2dHZ, %d clk\n",
		 __func__, mode->hdisplay, mode->vdisplay,
		 mode->flags & DRM_MODE_FLAG_INTERLACE ? "i" : "p",
		 mode->vrefresh, control.u.sync_info.pixelclock);

	err = nxs_get_control(nxs, type, &control);

	return err < 0 ? 0 : 1;
}

static void hdmi_mode_to_sync(struct drm_display_mode *mode,
			struct nxs_control_syncinfo *sync)
{
	struct videomode vm;

	drm_display_mode_to_videomode(mode, &vm);

	sync->pixelclock = vm.pixelclock;
	sync->width = vm.hactive;
	sync->hfrontporch = vm.hfront_porch;
	sync->hbackporch = vm.hback_porch;
	sync->hsyncwidth = vm.hsync_len;
	sync->height = vm.vactive;
	sync->vfrontporch = vm.vfront_porch;
	sync->vbackporch = vm.vback_porch;
	sync->vsyncwidth = vm.vsync_len;
	sync->fps = mode->vrefresh;

	if (vm.flags & DISPLAY_FLAGS_INTERLACED)
		sync->interlaced = 1;

	if (vm.flags & DISPLAY_FLAGS_VSYNC_LOW)
		sync->vsyncpol = 1;

	if (vm.flags & DISPLAY_FLAGS_HSYNC_LOW)
		sync->hsyncpol = 1;
}

static void hdmi_sync_to_mode(struct nxs_control_syncinfo *sync,
			struct drm_display_mode *mode)
{
	struct videomode vm;

	vm.pixelclock = sync->pixelclock;
	vm.hactive = sync->width;
	vm.hfront_porch = sync->hfrontporch;
	vm.hback_porch = sync->hbackporch;
	vm.hsync_len = sync->hsyncwidth;
	vm.vactive = sync->height;
	vm.vfront_porch = sync->vfrontporch;
	vm.vback_porch = sync->vbackporch;
	vm.vsync_len = sync->vsyncwidth;
	vm.flags = 0;

	if (sync->interlaced)
		vm.flags |= DISPLAY_FLAGS_INTERLACED;

	if (sync->vsyncpol)
		vm.flags |= DISPLAY_FLAGS_VSYNC_LOW;

	if (sync->hsyncpol)
		vm.flags |= DISPLAY_FLAGS_HSYNC_LOW;

	drm_display_mode_from_videomode(&vm, mode);
}

static int hdmi_ops_get_mode(struct nx_drm_display *display,
			struct drm_display_mode *mode)
{
	struct nx_hdmi_dev *hdmi = display->context;
	struct nxs_dev *nxs = hdmi->control.s_dev;
	enum nxs_control_type type = NXS_CONTROL_SYNCINFO;
	struct nxs_control control = { 0, };
	int err;

	pr_debug("%s: enter\n", __func__);

	hdmi_mode_to_sync(mode, &control.u.sync_info);

	err = nxs_get_control(nxs, type, &control);
	if (err < 0)
		return err;

	hdmi_sync_to_mode(&control.u.sync_info, mode);

	return 0;
}

static int hdmi_ops_set_mode(struct nx_drm_display *display,
			struct drm_display_mode *mode, unsigned int mode_flags)
{
	struct nx_drm_connector *connector = display->connector;
	struct drm_crtc *crtc = connector->encoder->encoder.crtc;
	struct nx_hdmi_dev *hdmi = display->context;
	struct nxs_dev *nxs = hdmi->control.s_dev;
	enum nxs_control_type type = NXS_CONTROL_SYNCINFO;
	struct nxs_control control;
	int ret;

	pr_debug("%s: linked to crtc.%d\n", __func__, to_nx_crtc(crtc)->pipe);

	ret = nx_stream_display_create(display);
	if (ret < 0)
		return ret;

	control.u.sync_info.flags = mode_flags;	/* for DVI mode */

	hdmi_mode_to_sync(mode, &control.u.sync_info);

	return nxs_set_control(nxs, type, (const struct nxs_control *)&control);
}

static struct nx_drm_hdmi_ops dev_ops = {
	.hpd_status = hdmi_ops_hpd_status,
	.get_mode = hdmi_ops_get_mode,
	.is_connected = hdmi_ops_is_connected,
	.is_valid = hdmi_ops_is_valid,
};

static struct nx_drm_display_ops hdmi_ops = {
	.open = hdmi_ops_open,
	.enable = hdmi_ops_enable,
	.disable = hdmi_ops_disable,
	.irq_done = hdmi_ops_irq_done,
	.set_mode = hdmi_ops_set_mode,
	.hdmi = &dev_ops,
};

void *nx_drm_display_hdmi_get(struct device *dev,
			struct device_node *node,
			struct nx_drm_display *display)
{
	struct nx_hdmi_dev *hdmi;
	struct nxs_dev *nxs;

	pr_debug("%s: pipe.%d\n", __func__, display->pipe);

	nxs = get_nxs_dev(NXS_FUNCTION_HDMI, 0, 0, false);
	if (!nxs)
		return NULL;

	hdmi = kzalloc(sizeof(*hdmi), GFP_KERNEL);
	if (!hdmi)
		return NULL;

	hdmi->control.s_dev = nxs;
	hdmi->control.pipe = -1;

	display->context = hdmi;
	display->ops = &hdmi_ops;
	display->irq = nxs->irq;

	return &hdmi->control;
}
