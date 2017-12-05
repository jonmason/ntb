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

#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/component.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/of_gpio.h>
#include <video/of_display_timing.h>
#include <dt-bindings/gpio/gpio.h>
#include <drm/nexell_drm.h>

#include "nx_drm_drv.h"
#include "nx_drm_fb.h"
#include <linux/switch.h>

struct hdmi_resource {
	struct i2c_adapter *ddc_adpt;
	const struct edid *edid;
	bool dvi_mode;
	int hpd_gpio;
	int hpd_irq;
	struct switch_dev swdev;
};

struct hdmi_context {
	struct nx_drm_connector *connector;
	/* lcd parameters */
	struct delayed_work work;
	struct gpio_desc *enable_gpio;
	struct hdmi_resource hdmi;
	bool plug;
	/* properties */
	int crtc_pipe;
	unsigned int possible_crtcs_mask;
	bool skip_boot_connect;
};

#define ctx_to_display(c)	\
		((struct nx_drm_display *)(c->connector->display))

#define HOTPLUG_DEBOUNCE_MS		1000

static bool panel_hdmi_ops_detect(struct device *dev,
			struct drm_connector *connector)
{
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);

	display->is_connected = ctx->plug;

	DRM_INFO("HDMI: %s\n",
		ctx->plug ? "connect" : "disconnect");

	return ctx->plug;
}

static bool panel_hdmi_ops_is_connected(struct device *dev)
{
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);

	return display->is_connected;
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
	struct nx_drm_connector *nx_connector = to_nx_connector(connector);
	struct nx_drm_display *display = nx_connector->display;
	struct nx_drm_display_ops *ops = display->ops;
	struct nx_drm_hdmi_ops *hdmi_ops = ops->hdmi;
	struct videomode *vm = &display->vm;
	struct drm_display_mode *t;
	bool err = false;

	DRM_DEBUG_KMS("vm %d:%d:%d modes %d\n",
		vm->hactive, vm->vactive, display->vrefresh, num_modes);

	/*
	 * if not support EDID, use default resolution
	 */
	if (!num_modes) {
		if (!hdmi_ops || !hdmi_ops->get_mode)
			return 0;

		mode = drm_mode_create(connector->dev);
		if (!mode) {
			DRM_ERROR("Failed to create a new display mode !\n");
			return 0;
		}

		drm_display_mode_from_videomode(vm, mode);
		mode->vrefresh = display->vrefresh;

		err = hdmi_ops->get_mode(display, mode);
		if (err)
			return err;

		mode->vrefresh = display->vrefresh;
		mode->width_mm = display->width_mm;
		mode->height_mm = display->height_mm;
		connector->display_info.width_mm = mode->width_mm;
		connector->display_info.height_mm = mode->height_mm;

		mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;

		drm_mode_set_name(mode);
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
			mode->vrefresh == display->vrefresh) {
			mode->type |= DRM_MODE_TYPE_PREFERRED;
			break;
		}
	}
	return num_modes;
}

static int panel_hdmi_ops_get_modes(struct device *dev,
			struct drm_connector *connector)
{
	struct edid *edid;
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct hdmi_resource *hdmi = &ctx->hdmi;
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
		kfree(edid);
	}

	return panel_hdmi_preferred_modes(dev, connector, num_modes);
}

static int panel_hdmi_ops_valid_mode(struct device *dev,
			struct drm_display_mode *mode)
{
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;
	struct nx_drm_hdmi_ops *hdmi = ops->hdmi;
	int ret;

	if (!hdmi || !hdmi->is_valid)
		return MODE_BAD;

	ret = hdmi->is_valid(display, mode);
	if (!ret)
		return MODE_BAD;

	return MODE_OK;
}

static void panel_hdmi_ops_set_mode(struct device *dev,
			struct drm_display_mode *mode)
{
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct hdmi_resource *hdmi = &ctx->hdmi;
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;

	DRM_DEBUG_KMS("enter\n");

	if (ops->set_mode)
		ops->set_mode(display, mode, hdmi->dvi_mode ? 1 : 0);
}

static void panel_hdmi_on(struct hdmi_context *ctx)
{
	struct nx_drm_connector *nx_connector = ctx->connector;
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;
	struct hdmi_resource *hdmi = &ctx->hdmi;

	DRM_DEBUG_KMS("enter\n");

	if (nx_connector->suspended)
		return;

	if (ops->enable)
		ops->enable(display);
	switch_set_state(&hdmi->swdev, 1);
}

static void panel_hdmi_off(struct hdmi_context *ctx)
{
	struct nx_drm_connector *nx_connector = ctx->connector;
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;
	struct hdmi_resource *hdmi = &ctx->hdmi;

	DRM_DEBUG_KMS("enter\n");

	if (nx_connector->suspended)
		return;

	if (ops->disable)
		ops->disable(display);
	switch_set_state(&hdmi->swdev, 0);
}

static void panel_hdmi_ops_enable(struct device *dev)
{
	DRM_DEBUG_KMS("enter\n");
	panel_hdmi_on(dev_get_drvdata(dev));
}

static void panel_hdmi_ops_disable(struct device *dev)
{
	DRM_DEBUG_KMS("enter\n");
	panel_hdmi_off(dev_get_drvdata(dev));
}

static struct nx_drm_connect_drv_ops hdmi_connector_ops = {
	.detect = panel_hdmi_ops_detect,
	.is_connected = panel_hdmi_ops_is_connected,
	.get_modes = panel_hdmi_ops_get_modes,
	.valid_mode = panel_hdmi_ops_valid_mode,
	.set_mode = panel_hdmi_ops_set_mode,
	.enable = panel_hdmi_ops_enable,
	.disable = panel_hdmi_ops_disable,
};

static struct nx_drm_connector hdmi_connector_dev = {
	.ops = &hdmi_connector_ops,
};

static int panel_hdmi_bind(struct device *dev,
			struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct hdmi_resource *hdmi = &ctx->hdmi;
	struct drm_connector *connector = &ctx->connector->connector;
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;
	struct nx_drm_hdmi_ops *hdmi_ops = ops->hdmi;
	int pipe = ctx->crtc_pipe;
	int err = 0;

	DRM_INFO("Bind %s panel\n", nx_panel_get_name(NX_PANEL_TYPE_HDMI));

	err = nx_drm_connector_attach(drm, connector,
			pipe, ctx->possible_crtcs_mask, NX_PANEL_TYPE_HDMI);
	if (err < 0) {
		struct platform_driver *drv = to_platform_driver(dev->driver);

		if (drv->remove)
			drv->remove(to_platform_device(dev));
		return 0;
	}

	/* check connect status at boot time */
	if (hdmi_ops->is_connected) {
		if (hdmi_ops->is_connected(display)) {
			struct nx_drm_private *private = drm->dev_private;

			DRM_INFO("HDMI %s connected, LCD %s\n",
				ctx->skip_boot_connect ? "Skip" : "Check",
				private->force_detect ? "connected" :
				"not connected");

			if (!ctx->skip_boot_connect || !private->force_detect) {
				ctx->plug = hdmi_ops->is_connected(display);
				private->force_detect = true;
			}
		}
	}

	/*
	 * Enable the interrupt after the connector has been
	 * connected.
	 */
	if (hdmi->hpd_irq != INVALID_IRQ)
		enable_irq(hdmi->hpd_irq);

	return 0;
}

static void panel_hdmi_unbind(struct device *dev,
			struct device *master, void *data)
{
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct hdmi_resource *hdmi = &ctx->hdmi;
	struct drm_connector *connector = &ctx->connector->connector;

	if (hdmi->hpd_irq != INVALID_IRQ)
		disable_irq(hdmi->hpd_irq);

	cancel_delayed_work_sync(&ctx->work);

	if (connector)
		nx_drm_connector_detach(connector);
}

static const struct component_ops panel_comp_ops = {
	.bind = panel_hdmi_bind,
	.unbind = panel_hdmi_unbind,
};

static bool __drm_fb_bound(struct drm_fb_helper *fb_helper)
{
	struct drm_device *dev = fb_helper->dev;
	struct drm_crtc *crtc;
	int bound = 0, crtcs_bound = 0;

	/*
	 * Sometimes user space wants everything disabled, so don't steal the
	 * display if there's a master.
	 */
	if (dev->primary->master)
		return true;

	drm_for_each_crtc(crtc, dev) {
		if (crtc->primary->fb)
			crtcs_bound++;
		if (crtc->primary->fb == fb_helper->fb)
			bound++;
	}

	if (bound < crtcs_bound)
		return true;

	return false;
}

static int panel_hdmi_wait_fb_bound(struct hdmi_context *ctx)
{
	struct drm_connector *connector;
	struct drm_fb_helper *fb_helper;
	struct nx_drm_private *private;
	struct nx_drm_display_ops *ops;
	int count = 500;	/* wait for 10 sec */

	if (!ctx->connector)
		return 0;

	connector = &ctx->connector->connector;
	private = connector->dev->dev_private;
	fb_helper = &private->fbdev->fb_helper;
	ops = ctx_to_display(ctx)->ops;

	if (!ctx->skip_boot_connect) {
		/*
		 * check clone mode, refer to drm_target_cloned
		 */
		if (fb_helper->crtc_count < 2 &&
			fb_helper->crtc_count != fb_helper->connector_count &&
			!__drm_fb_bound(fb_helper)) {
			dev_warn(connector->dev->dev,
				"can't cloning framebuffer ....\n");
			dev_warn(connector->dev->dev,
				"framebuffer crtcs %d connecots %d\n",
				fb_helper->crtc_count,
				fb_helper->connector_count);
			dev_warn(connector->dev->dev,
				"check property 'skip-boot-connect'\n");
		}
		return 0;
	}

	do {
		if (__drm_fb_bound(fb_helper))
			return 0;

		msleep(20);

		if (!ops->hdmi->is_connected(ctx_to_display(ctx)))
			return -ENOENT;

	} while (count-- > 0);

	if (count < 0) {
		DRM_ERROR("check other framebuffer binded !!!\n");
		return -ENOENT;
	}

	return 0;
}

static void panel_hdmi_hpd_work(struct work_struct *work)
{
	struct hdmi_context *ctx;
	struct drm_connector *connector;
	struct nx_drm_display *display;
	struct nx_drm_display_ops *ops;
	bool plug;

	ctx = container_of(work, struct hdmi_context, work.work);
	if (!ctx->connector)
		return;

	display = ctx_to_display(ctx);
	ops = display->ops;
	if (!ops->hdmi->is_connected) {
		DRM_INFO("HDMI not implement connected API\n");
		return;
	}

#ifdef CONFIG_DRM_FBDEV_EMULATION
	if (panel_hdmi_wait_fb_bound(ctx))
		return;
#endif

	plug = ops->hdmi->is_connected(display);
	if (plug == ctx->plug)
		return;

	DRM_INFO("HDMI: %s\n", plug ? "plug" : "unplug");

	ctx->plug = plug;
	connector = &ctx->connector->connector;
	drm_helper_hpd_irq_event(connector->dev);
}

static irqreturn_t panel_hdmi_hpd_irq(int irq, void *data)
{
	struct hdmi_context *ctx = data;
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;
	struct nx_drm_hdmi_ops *hdmi_ops = ops->hdmi;
	u32 event;

	if (!hdmi_ops->hpd_status)
		return IRQ_HANDLED;

	event = hdmi_ops->hpd_status(display);

	if (event & (HDMI_EVENT_PLUG | HDMI_EVENT_UNPLUG))
		mod_delayed_work(system_wq, &ctx->work,
				msecs_to_jiffies(HOTPLUG_DEBOUNCE_MS));

	return IRQ_HANDLED;
}

#define property_read(n, s, v)	of_property_read_u32(n, s, &v)

static int panel_hdmi_parse_dt_hdmi(struct platform_device *pdev,
			struct hdmi_context *ctx)
{
	struct device *dev = &pdev->dev;
	struct hdmi_resource *hdmi = &ctx->hdmi;
	struct device_node *node = dev->of_node;
	struct device_node *np;
	unsigned long flags = IRQF_ONESHOT;
	int hpd_gpio = 0, hpd_irq = INVALID_IRQ;
	int err;

	/* parse hdmi default resolution */
	np = of_find_node_by_name(node, "mode");
	of_node_get(node);
	if (np) {
		struct nx_drm_display *display = ctx_to_display(ctx);
		struct videomode *vm = &display->vm;

		property_read(np, "width", vm->hactive);
		property_read(np, "height", vm->vactive);
		property_read(np, "flags", vm->flags);
		property_read(np, "refresh", display->vrefresh);
	}

	/* EDID ddc */
	np = of_parse_phandle(node, "ddc-i2c-bus", 0);
	if (!np) {
		DRM_ERROR("Failed to find ddc adapter node for HPD !\n");
		return -ENODEV;
	}

	hdmi->ddc_adpt = of_find_i2c_adapter_by_node(np);
	if (!hdmi->ddc_adpt) {
		DRM_ERROR("Failed to get ddc adapter !\n");
		return -EPROBE_DEFER;
	}

	/* HPD gpio */
	hpd_gpio = of_get_named_gpio(node, "hpd-gpio", 0);
	if (gpio_is_valid(hpd_gpio)) {
		err = gpio_request_one(hpd_gpio, GPIOF_DIR_IN,
						"HDMI hotplug detect");
		if (err < 0) {
			DRM_ERROR("Failed to gpio_request_one(): %d, err %d\n",
				hpd_gpio, err);
			return err;
		}

		err = gpio_to_irq(hpd_gpio);
		if (err < 0) {
			DRM_ERROR("Failed to gpio_to_irq(): %d -> %d\n",
				hpd_gpio, err);
			gpio_free(hpd_gpio);
			return err;
		}

		flags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING |
				IRQF_ONESHOT;
		DRM_INFO("hdp gpio %d\n", hpd_gpio);
	} else { /* HPD hardwired */
		err = platform_get_irq(pdev, 0);
	}

	if (err >= 0)
		hpd_irq = err;

	INIT_DELAYED_WORK(&ctx->work, panel_hdmi_hpd_work);

	if (hpd_irq != INVALID_IRQ) {
		err = devm_request_threaded_irq(dev, hpd_irq, NULL,
				panel_hdmi_hpd_irq, flags, "hdmi-hpd", ctx);
		if (err < 0) {
			DRM_ERROR("Failed to request IRQ#%u: %d\n", hpd_irq, err);
			gpio_free(hpd_irq);
			return err;
		}

		/*
		 * Disable the interrupt until the connector has been
	 	* initialized to avoid a race in the hotplug interrupt
	 	* handler.
	 	*/
		disable_irq(hpd_irq);
		DRM_INFO("irq %d install for hdp\n", hpd_irq);
	} else {
		struct nx_drm_display *display = ctx_to_display(ctx);
		struct nx_drm_hdmi_ops *hdmi_ops = display->ops->hdmi;

		hdmi_ops->hpd_irq_cb = panel_hdmi_hpd_irq;
		hdmi_ops->cb_data = ctx;
	}

	ctx->skip_boot_connect =
		of_property_read_bool(node, "skip-boot-connect");

	hdmi->hpd_gpio = hpd_gpio;
	hdmi->hpd_irq = hpd_irq;

	return 0;
}

static int panel_hdmi_parse_dt(struct platform_device *pdev,
			struct hdmi_context *ctx)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct gpio_desc *desc;
	int err;

	DRM_INFO("Load HDMI panel\n");

	property_read(node, "crtc-pipe", ctx->crtc_pipe);

	/* get possible crtcs */
	property_read(node, "crtcs-possible-mask", ctx->possible_crtcs_mask);

	/* parse HDMI configs */
	err = panel_hdmi_parse_dt_hdmi(pdev, ctx);
	if (err < 0)
		return err;

	desc = devm_gpiod_get_optional(dev, "enable", GPIOD_ASIS);
	if (-EBUSY == (long)ERR_CAST(desc)) {
		DRM_INFO("Failed, enable-gpios is busy : %s !!!\n",
			node->full_name);
		desc = NULL;
	}

	if (!IS_ERR(desc) && desc) {
		enum of_gpio_flags flags;
		int gpio;

		gpio = of_get_named_gpio_flags(node, "enable-gpios", 0, &flags);
		if (!gpio_is_valid(gpio)) {
			DRM_ERROR("invalid gpio.%d\n", gpio);
			return -EINVAL;
		}

		/* enable at boottime */
		gpiod_direction_output(desc,
					flags == GPIO_ACTIVE_HIGH ? 1 : 0);
		ctx->enable_gpio = desc;

		DRM_INFO("HDMI enable-gpio.%d act %s\n", gpio,
				flags == GPIO_ACTIVE_HIGH ? "high" : "low ");
	}

	property_read(node, "width-mm", display->width_mm);
	property_read(node, "height-mm", display->height_mm);

	return 0;
}

static int panel_hdmi_get_display(struct platform_device *pdev,
			struct hdmi_context *ctx)
{
	struct device *dev = &pdev->dev;
	struct nx_drm_connector *nx_connector = ctx->connector;
	struct drm_connector *connector = &nx_connector->connector;

	/* get HDMI */
	nx_connector->display =
		nx_drm_display_get(dev,
			dev->of_node, connector, NX_PANEL_TYPE_HDMI);
	if (!nx_connector->display)
		return -ENOENT;

	return 0;
}

static int panel_hdmi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hdmi_resource *hdmi;
	struct hdmi_context *ctx;
	struct nx_drm_display_ops *ops;
	int err;

	DRM_DEBUG_KMS("enter (%s)\n", dev_name(dev));

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->connector = &hdmi_connector_dev;
	ctx->connector->dev = dev;

	hdmi = &ctx->hdmi;
	hdmi->hpd_gpio = -1;
	hdmi->hpd_irq = INVALID_IRQ;

	hdmi->swdev.name = "hdmi";
	err = switch_dev_register(&hdmi->swdev);
	if (err) {
		dev_err(&pdev->dev, "failed to register hdmi switch\n");
		return -ENOMEM;
	}

	err = panel_hdmi_get_display(pdev, ctx);
	if (err < 0)
		goto err_probe;

	ops = ctx_to_display(ctx)->ops;
	if (ops->open) {
		err = ops->open(ctx_to_display(ctx), ctx->crtc_pipe);
		if (err)
			goto err_probe;
	}

	err = panel_hdmi_parse_dt(pdev, ctx);
	if (err < 0)
		goto err_probe;

	dev_set_drvdata(dev, ctx);

	component_add(dev, &panel_comp_ops);

	DRM_DEBUG_KMS("done\n");
	return err;

err_probe:
	DRM_ERROR("Failed %s probe !!!\n", dev_name(dev));
	switch_dev_unregister(&hdmi->swdev);
	devm_kfree(dev, ctx);
	return err;
}

static int panel_hdmi_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hdmi_context *ctx = dev_get_drvdata(&pdev->dev);
	struct hdmi_resource *hdmi;
	struct nx_drm_display_ops *ops;

	if (!ctx)
		return 0;

	component_del(dev, &panel_comp_ops);

	switch_dev_unregister(&hdmi->swdev);

	hdmi = &ctx->hdmi;
	if (hdmi->hpd_irq != INVALID_IRQ)
		devm_free_irq(dev, hdmi->hpd_irq, ctx);

	if (hdmi->ddc_adpt)
		put_device(&hdmi->ddc_adpt->dev);

	ops = ctx_to_display(ctx)->ops;
	if (ops->close)
		ops->close(ctx_to_display(ctx), ctx->crtc_pipe);

	if (ctx->enable_gpio)
		devm_gpiod_put(dev, ctx->enable_gpio);

	nx_drm_display_put(dev, ctx_to_display(ctx));
	devm_kfree(dev, ctx);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int panel_hdmi_suspend(struct device *dev)
{
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct drm_connector *connector;
	struct nx_drm_display *display;
	int ret = 0;

	if (!ctx || !ctx->connector)
		return 0;

	connector = &ctx->connector->connector;
	display = ctx_to_display(ctx);

	/*
	 * if hdmi is connected, prevent to go suspend mode.
	 * to protect current leakage from HDMI port
	 */
	if (ctx->plug) {
		dev_warn(dev, "HDMI is connected -> prevent suspend !!!\n");
		return -EIO;
	}

	ctx->plug = false;

	cancel_delayed_work_sync(&ctx->work);
	drm_helper_hpd_irq_event(connector->dev);

	if (display->ops && display->ops->suspend)
		ret = display->ops->suspend(display);

	return ret;
}

static int panel_hdmi_resume(struct device *dev)
{
	struct hdmi_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display;

	if (!ctx || !ctx->connector)
		return 0;

	display = ctx_to_display(ctx);

	if (display->ops && display->ops->resume)
		display->ops->resume(display);

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

static struct platform_driver panel_hdmi_driver = {
	.probe = panel_hdmi_probe,
	.remove = panel_hdmi_remove,
	.driver = {
		.name = "nexell,display_drm_hdmi",
		.owner = THIS_MODULE,
		.of_match_table = panel_hdmi_of_match,
		.pm = &panel_hdmi_pm,
	},
};

void panel_hdmi_init(void)
{
	platform_driver_register(&panel_hdmi_driver);
}

void panel_hdmi_exit(void)
{
	platform_driver_unregister(&panel_hdmi_driver);
}
