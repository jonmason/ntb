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
#include <drm/drm_edid.h>
#include <drm/drm_fb_cma_helper.h>

#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/component.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/of_gpio.h>
#include <video/of_display_timing.h>

#include <drm/nexell_drm.h>

#include "nx_drm_drv.h"
#include "nx_drm_plane.h"
#include "nx_drm_crtc.h"
#include "nx_drm_encoder.h"
#include "nx_drm_connector.h"
#include "nx_drm_fb.h"
#include "soc/s5pxx18_dp_hdmi.h"

struct hdmi_resource {
	struct i2c_adapter *ddc_adpt;
	const struct edid *edid;
	bool dvi_mode;
	int hpd_gpio;
	int hpd_irq;
};

struct hdmi_context {
	struct drm_connector *connector;
	int crtc_pipe;
	struct reset_control *reset;
	void *base;
	struct nx_drm_device *display;
	struct delayed_work	 work;
	struct gpio_desc *enable_gpio;
	struct hdmi_resource hdmi_res;
	spinlock_t lock;
	bool plug;
};

#define ctx_to_hdmi(c)	(struct hdmi_resource *)(&c->hdmi_res)

static bool panel_hdmi_is_connected(struct device *dev,
			struct drm_connector *connector)
{
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_panel *panel = &ctx->display->panel;

	panel->is_connected = ctx->plug;

	DRM_INFO("HDMI: %s\n",
		ctx->plug ? "connect" : "disconnect");

	return ctx->plug;
}

static void panel_hdmi_dump_edid_modes(struct drm_connector *connector,
			int num_modes, bool dump)
{
	struct drm_display_mode *mode, *t;

	if (!num_modes || !dump)
		return;

	list_for_each_entry_safe(mode, t, &connector->probed_modes, head) {
		DRM_DEBUG_KMS("EDID [%4d x %4d %3d fps, 0x%08x(%s), %d] %s\n",
			mode->hdisplay, mode->vdisplay, mode->vrefresh,
			mode->flags, mode->flags & DRM_MODE_FLAG_3D_MASK ?
			"3D" : "2D",
			mode->clock*1000, mode->name);
	}
}

static int panel_hdmi_preferred_modes(struct device *dev,
			struct drm_connector *connector, int num_modes)
{
	struct drm_display_mode *mode;
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_panel *panel = &ctx->display->panel;
	struct videomode *vm = &panel->vm;
	struct drm_display_mode *t;
	bool err = false;

	DRM_DEBUG_KMS("enter %d:%d:%d\n",
		vm->hactive, vm->vactive, panel->vrefresh);

	err = nx_dp_hdmi_mode_get(vm->hactive,
				vm->vactive, panel->vrefresh, vm);
	if (false == err)
		return 0;

	/*
	 * if not support EDID, use default resolution
	 */
	if (!num_modes) {
		mode = drm_mode_create(connector->dev);
		if (!mode) {
			DRM_ERROR("fail : create a new display mode !\n");
			return 0;
		}
		drm_display_mode_from_videomode(vm, mode);

		mode->vrefresh = panel->vrefresh;
		mode->width_mm = panel->width_mm;
		mode->height_mm = panel->height_mm;
		connector->display_info.width_mm = mode->width_mm;
		connector->display_info.height_mm = mode->height_mm;

		mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
		drm_mode_probed_add(connector, mode);
		return 1;
	}

	/*
	 * set preferred mode form EDID modes
	 */
	list_for_each_entry_safe(mode, t, &connector->probed_modes, head) {
		mode->type &= ~DRM_MODE_TYPE_PREFERRED;
		if (mode->hdisplay == vm->hactive &&
			mode->vdisplay == vm->vactive &&
			mode->vrefresh == panel->vrefresh) {
			mode->type |= DRM_MODE_TYPE_PREFERRED;
			break;
		}
	}
	return num_modes;
}

static int panel_hdmi_get_modes(struct device *dev,
			struct drm_connector *connector)
{
	struct edid *edid;
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct hdmi_resource *hdmi = &ctx->hdmi_res;
	int num_modes = 0;

	if (!hdmi->ddc_adpt)
		return -ENODEV;

	edid = drm_get_edid(connector, hdmi->ddc_adpt);
	if (edid) {
		hdmi->dvi_mode = !drm_detect_hdmi_monitor(edid);
		DRM_DEBUG_KMS("%s : width[%d] x height[%d]\n",
			(hdmi->dvi_mode ? "dvi monitor" : "hdmi monitor"),
			edid->width_cm, edid->height_cm);

		drm_mode_connector_update_edid_property(connector, edid);
		num_modes = drm_add_edid_modes(connector, edid);
		panel_hdmi_dump_edid_modes(connector, num_modes, false);
	}

	return panel_hdmi_preferred_modes(dev, connector, num_modes);
}

static int panel_hdmi_check_mode(struct device *dev,
			struct drm_display_mode *mode)
{
	int pixelclock = mode->clock * 1000;
	struct videomode vm;
	bool ret;

	drm_display_mode_to_videomode(mode, &vm);

	ret = nx_dp_hdmi_mode_valid(&vm, mode->vrefresh, pixelclock);
	if (!ret)
		return MODE_BAD;

	DRM_DEBUG_KMS("OK MODE %d x %d mm, %s, %d hz %d fps\n",
		mode->width_mm, mode->height_mm,
		mode->flags & DRM_MODE_FLAG_INTERLACE ?
		"interlace" : "progressive", pixelclock,
		mode->vrefresh);
	DRM_DEBUG_KMS("ha:%d, hf:%d, hb:%d, hs:%d\n",
		vm.hactive, vm.hfront_porch,
		vm.hback_porch, vm.hsync_len);
	DRM_DEBUG_KMS("va:%d, vf:%d, vb:%d, vs:%d\n",
		vm.vactive, vm.vfront_porch,
		vm.vback_porch, vm.vsync_len);
	DRM_DEBUG_KMS("flags:0x%x\n", vm.flags);

	return MODE_OK;
}

bool panel_hdmi_mode_fixup(struct device *dev,
			struct drm_connector *connector,
			const struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode)
{
	return true;
}

void panel_hdmi_mode_set(struct device *dev,
			struct drm_display_mode *mode)
{
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_device *display = ctx->display;
	struct hdmi_resource *hdmi = ctx_to_hdmi(ctx);
	struct videomode *vm = &display->panel.vm;

	DRM_DEBUG_KMS("enter\n");

	nx_dp_hdmi_mode_set(display, mode, vm, hdmi->dvi_mode);
}

static void panel_hdmi_commit(struct device *dev)
{
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	int pipe = drm_dev_get_dpc(ctx->display)->module;

	nx_dp_hdmi_mode_commit(ctx->display, pipe);
}

static void panel_hdmi_enable(struct device *dev)
{
	struct hdmi_context *ctx = dev_get_drvdata(dev);

	nx_dp_hdmi_power(ctx->display, true);
}

static void panel_hdmi_disable(struct device *dev)
{
	struct hdmi_context *ctx = dev_get_drvdata(dev);

	nx_dp_hdmi_power(ctx->display, false);
}

static void panel_hdmi_dmps(struct device *dev, int mode)
{
	DRM_DEBUG_KMS("dpms.%d\n", mode);

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		panel_hdmi_enable(dev);
		break;

	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		panel_hdmi_disable(dev);
		break;
	default:
		DRM_ERROR("fail : unspecified mode %d\n", mode);
		break;
	}
}

static struct nx_drm_ops panel_hdmi_ops = {
	.is_connected = panel_hdmi_is_connected,
	.get_modes = panel_hdmi_get_modes,
	.check_mode = panel_hdmi_check_mode,
	.mode_fixup = panel_hdmi_mode_fixup,
	.mode_set = panel_hdmi_mode_set,
	.commit = panel_hdmi_commit,
	.dpms = panel_hdmi_dmps,
};

static struct nx_drm_device hdmi_dp_dev = {
	.ops = &panel_hdmi_ops,
};

static int panel_hdmi_bind(struct device *dev,
			struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct hdmi_resource *hdmi = &ctx->hdmi_res;
	struct platform_driver *pdrv = to_platform_driver(dev->driver);
	int pipe = ctx->crtc_pipe;

	DRM_DEBUG_KMS("enter\n");

	ctx->connector = nx_drm_connector_create_and_attach(drm,
				ctx->display, pipe, dp_panel_type_hdmi, ctx);
	if (IS_ERR(ctx->connector)) {
		if (pdrv->remove)
			pdrv->remove(to_platform_device(dev));
		return 0;
	}

	/*
	 * check connect boot status at boot time
	 */
	if (nx_dp_hdmi_is_connected()) {
		struct nx_drm_priv *priv = drm->dev_private;

		ctx->plug = nx_dp_hdmi_is_connected();
		priv->force_detect = true;
	}

	/*
	 * Enable the interrupt after the connector has been
	 * connected.
	 */
	enable_irq(hdmi->hpd_irq);

	DRM_DEBUG_KMS("done\n");
	return 0;
}

static void panel_hdmi_unbind(struct device *dev,
			struct device *master, void *data)
{
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct hdmi_resource *hdmi = &ctx->hdmi_res;

	if (ctx->connector)
		nx_drm_connector_destroy_and_detach(ctx->connector);

	if (INVALID_IRQ != hdmi->hpd_irq)
		disable_irq(hdmi->hpd_irq);
}

static const struct component_ops panel_comp_ops = {
	.bind = panel_hdmi_bind,
	.unbind = panel_hdmi_unbind,
};

static void panel_hdmi_hpd_work(struct work_struct *work)
{
	struct hdmi_context *ctx;
	bool plug;

	ctx = container_of(work, struct hdmi_context, work.work);
	if (!ctx->connector)
		return;

	plug = nx_dp_hdmi_is_connected();
	if (plug == ctx->plug)
		return;

	ctx->plug = plug;

	DRM_INFO("HDMI %s\n", plug ? "plug" : "unplug");

	drm_helper_hpd_irq_event(ctx->connector->dev);
}

#define HOTPLUG_DEBOUNCE_MS		1000

static irqreturn_t panel_hdmi_hpd_irq(int irq, void *data)
{
	struct hdmi_context *ctx = data;
	u32 event;

	event = nx_dp_hdmi_hpd_event(irq);

	if (event & (HDMI_EVENT_PLUG | HDMI_EVENT_UNPLUG))
		mod_delayed_work(system_wq, &ctx->work,
				msecs_to_jiffies(HOTPLUG_DEBOUNCE_MS));

	return IRQ_HANDLED;
}

#define parse_read_prop(n, s, v)	{ \
	u32 _v;	\
	if (!of_property_read_u32(n, s, &_v))	\
		v = _v;	\
	}

static int panel_hdmi_parse_dt_hdmi(struct platform_device *pdev,
			struct hdmi_context *ctx)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct device_node *np;
	struct hdmi_resource *hdmi = &ctx->hdmi_res;
	unsigned long flags = IRQF_ONESHOT;
	int hpd_gpio = 0, hpd_irq;
	int err;

	/*
	 * parse hdmi default resolution
	 */
	np = of_find_node_by_name(node, "mode");
	if (np) {
		struct nx_drm_panel *panel = &ctx->display->panel;
		struct videomode *vm = &panel->vm;

		parse_read_prop(np, "width", vm->hactive);
		parse_read_prop(np, "height", vm->vactive);
		parse_read_prop(np, "flags", vm->flags);
		parse_read_prop(np, "refresh", panel->vrefresh);
	}

	/*
	 * EDID ddc
	 */
	np = of_parse_phandle(node, "ddc-i2c-bus", 0);
	if (!np) {
		DRM_ERROR("fail : to find ddc adapter node for HPD !\n");
		return -ENODEV;
	}

	hdmi->ddc_adpt = of_find_i2c_adapter_by_node(np);
	if (!hdmi->ddc_adpt) {
		DRM_ERROR("fail : get ddc adapter !\n");
		return -EPROBE_DEFER;
	}

	/*
	 * HPD
	 */
	hpd_gpio = of_get_named_gpio(node, "hpd-gpio", 0);
	if (gpio_is_valid(hpd_gpio)) {
		err = gpio_request_one(hpd_gpio, GPIOF_DIR_IN,
						"HDMI hotplug detect");
		if (0 > err) {
			DRM_ERROR("fail : gpio_request_one(): %d, err %d\n",
				hpd_gpio, err);
			return err;
		}

		err = gpio_to_irq(hpd_gpio);
		if (0 > err) {
			DRM_ERROR("fail : gpio_to_irq(): %d -> %d\n",
				hpd_gpio, err);
			gpio_free(hpd_gpio);
			return err;
		}

		flags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING |
				IRQF_ONESHOT;
		DRM_INFO("hdp gpio %d\n", hpd_gpio);
	} else {
		err = platform_get_irq(pdev, 0);
		if (0 > err) {
			DRM_ERROR("fail : hdmi platform_get_irq !\n");
			return -EINVAL;
		}
	}

	hpd_irq = err;
	INIT_DELAYED_WORK(&ctx->work, panel_hdmi_hpd_work);

	err = devm_request_threaded_irq(dev, hpd_irq, NULL,
				panel_hdmi_hpd_irq, flags, "hdmi-hpd", ctx);
	if (0 > err) {
		DRM_ERROR("fail : to request IRQ#%u: %d\n", hpd_irq, err);
		gpio_free(hpd_irq);
		return err;
	}

	hdmi->hpd_gpio = hpd_gpio;
	hdmi->hpd_irq = hpd_irq;

	DRM_INFO("irq %d install for hdp\n", hpd_irq);

	/*
	 * Disable the interrupt until the connector has been
	 * initialized to avoid a race in the hotplug interrupt
	 * handler.
	 */
	disable_irq(hpd_irq);

	return 0;
}

static int panel_hdmi_parse_dt(struct platform_device *pdev,
			struct hdmi_context *ctx)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct nx_drm_device *display = ctx->display;
	struct nx_drm_panel *panel = &ctx->display->panel;
	int err;

	DRM_INFO("Load HDMI panel\n");

	parse_read_prop(node, "crtc-pipe", ctx->crtc_pipe);

	/*
	 * parse panel output for HDMI
	 */
	err = nx_drm_dp_panel_dev_register(dev,
				node, dp_panel_type_hdmi, display);
	if (0 > err)
		return err;

	/*
	 * parse HDMI configs
	 */
	err = panel_hdmi_parse_dt_hdmi(pdev, ctx);
	if (0 > err)
		return err;

	ctx->enable_gpio =
			devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);

	if (ctx->enable_gpio)
		gpiod_set_value_cansleep(ctx->enable_gpio, 1);

	parse_read_prop(node, "width-mm", panel->width_mm);
	parse_read_prop(node, "height-mm", panel->height_mm);

	return 0;
}

static int panel_hdmi_driver_setup(struct platform_device *pdev,
			struct hdmi_context *ctx)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct nx_drm_res *res = &ctx->display->res;
	int err;

	err = nx_drm_dp_panel_drv_res_parse(dev, &ctx->base, &ctx->reset);
	if (0 > err)
		return -EINVAL;

	err = nx_drm_dp_panel_dev_res_parse(dev, node, res, dp_panel_type_hdmi);
	if (0 > err)
		return -EINVAL;

	return 0;
}

static int panel_hdmi_probe(struct platform_device *pdev)
{
	struct hdmi_resource *hdmi;
	struct hdmi_context *ctx;
	struct device *dev = &pdev->dev;
	int err;

	DRM_DEBUG_KMS("enter (%s)\n", dev_name(dev));

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (IS_ERR(ctx))
		return -ENOMEM;

	ctx->display = &hdmi_dp_dev;
	ctx->display->dev = dev;

	spin_lock_init(&ctx->lock);

	hdmi = &ctx->hdmi_res;
	hdmi->hpd_gpio = -1;
	hdmi->hpd_irq = INVALID_IRQ;

	err = panel_hdmi_driver_setup(pdev, ctx);
	if (0 > err)
		goto err_parse;

	err = panel_hdmi_parse_dt(pdev, ctx);
	if (0 > err)
		goto err_parse;

	dev_set_drvdata(dev, ctx);

	component_add(dev, &panel_comp_ops);

	DRM_DEBUG_KMS("done\n");
	return err;

err_parse:
	if (ctx)
		devm_kfree(dev, ctx);

	return err;
}

static int panel_hdmi_remove(struct platform_device *pdev)
{
	struct hdmi_resource *hdmi;
	struct hdmi_context *ctx = dev_get_drvdata(&pdev->dev);
	struct device *dev = &pdev->dev;

	if (!ctx)
		return 0;

	cancel_delayed_work_sync(&ctx->work);

	hdmi = &ctx->hdmi_res;
	if (INVALID_IRQ != hdmi->hpd_irq)
		devm_free_irq(&pdev->dev, hdmi->hpd_irq, ctx);

	if (hdmi->ddc_adpt)
		put_device(&hdmi->ddc_adpt->dev);

	nx_drm_dp_panel_dev_res_free(dev, &ctx->display->res);
	nx_drm_dp_panel_drv_res_free(dev, ctx->base, ctx->reset);
	nx_drm_dp_panel_dev_release(dev, ctx->display);

	devm_kfree(&pdev->dev, ctx);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int panel_hdmi_suspend(struct device *dev)
{
	return 0;
}

static int panel_hdmi_resume(struct device *dev)
{
	return 0;
}
#endif

static const struct dev_pm_ops panel_hdmi_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(
		panel_hdmi_suspend, panel_hdmi_resume
	)
};

static const struct of_device_id panel_hdmi_of_match[] = {
	{.compatible = "nexell,s5pxx18-drm-hdmi"},
	{}
};
MODULE_DEVICE_TABLE(of, panel_hdmi_of_match);

struct platform_driver panel_hdmi_driver = {
	.probe = panel_hdmi_probe,
	.remove = panel_hdmi_remove,
	.driver = {
		.name = "nexell,display_drm_hdmi",
		.owner = THIS_MODULE,
		.of_match_table = panel_hdmi_of_match,
		.pm = &panel_hdmi_pm,
	},
};
module_platform_driver(panel_hdmi_driver);

MODULE_AUTHOR("jhkim <jhkim@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell HDMI DRM Driver");
MODULE_LICENSE("GPL");
