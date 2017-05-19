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
#include <drm/drm_panel.h>
#include <drm/drm_mipi_dsi.h>

#include <linux/backlight.h>
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

struct mipi_resource {
	struct mipi_dsi_host mipi_host;
	struct mipi_dsi_device *mipi_dev;
	unsigned long flags;
	enum mipi_dsi_pixel_format format;
	unsigned int lanes;
};

struct lcd_context {
	struct nx_drm_connector connector;
	/* lcd parameters */
	bool local_timing;
	struct mipi_resource mipi;
	struct gpio_descs *enable_gpios;
	enum of_gpio_flags gpios_active[4];
	int gpios_delay[4];
	struct backlight_device *backlight;
	int backlight_delay;
	bool enabled;
	struct work_struct lcd_power_work;
	struct nx_lcd_ops *lcd_ops;
	/* properties */
	int crtc_pipe;
	unsigned int possible_crtcs_mask;
};

#define ctx_to_display(c)	\
		((struct nx_drm_display *)(c->connector.display))

static bool panel_lcd_ops_detect(struct device *dev,
			struct drm_connector *connector)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;
	enum nx_panel_type panel_type = nx_panel_get_type(display);

	DRM_DEBUG_KMS("%s panel node %s\n",
		nx_panel_get_name(panel_type), display->panel_node ?
		"exist" : "not exist");

	if (display->panel_node) {
		struct drm_panel *drm_panel =
				of_drm_find_panel(display->panel_node);

		if (drm_panel) {
			int ret;

			display->panel = drm_panel;
			drm_panel_attach(drm_panel, connector);

			if (display->check_panel)
				return display->is_connected;

			if (ops->prepare)
				ops->prepare(display);

			ret = drm_panel_prepare(drm_panel);
			if (!ret) {
				drm_panel_unprepare(drm_panel);

				if (ops->unprepare)
					ops->unprepare(display);

				display->is_connected = true;
			} else {
				drm_panel_detach(drm_panel);
				display->is_connected = false;
			}
			display->check_panel = true;

			DRM_INFO("%s: check panel %s\n",
				nx_panel_get_name(panel_type),
				display->is_connected ?
				"connected" : "disconnected");

			return display->is_connected;
		}

		/*
		 * builded with module (.ko file).
		 */
		DRM_DEBUG_KMS("Not find panel driver for %s ...\n",
			nx_panel_get_name(panel_type));
		return false;
	}

	if (!display->panel_node && !ctx->local_timing) {
		DRM_DEBUG_DRIVER("not exist %s panel & timing %s !\n",
			nx_panel_get_name(panel_type), dev_name(dev));
		return false;
	}

	/*
	 * support DT's timing node
	 * when not use panel driver
	 */
	display->is_connected = true;

	return true;
}

static bool panel_lcd_ops_is_connected(struct device *dev)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);

	return display->is_connected;
}

static int panel_lcd_ops_get_modes(struct device *dev,
			struct drm_connector *connector)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct videomode *vm = &display->vm;
	struct drm_display_mode *mode;
	u32 hto, vto;

	DRM_DEBUG_KMS("panel %s\n",
		display->panel ? "attached" : "detached");

	if (display->panel)
		return drm_panel_get_modes(display->panel);

	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("Failed to create a new display mode !\n");
		return 0;
	}

	drm_display_mode_from_videomode(vm, mode);

	hto = vm->hactive + vm->hfront_porch + vm->hback_porch + vm->hsync_len;
	vto = vm->vactive + vm->vfront_porch + vm->vback_porch + vm->vsync_len;

	mode->width_mm = display->width_mm;
	mode->height_mm = display->height_mm;
	mode->vrefresh = vm->pixelclock / (hto * vto);

	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	DRM_DEBUG_KMS("exit, (%dx%d, flags=0x%x)\n",
		mode->hdisplay, mode->vdisplay, mode->flags);

	return 1;
}

static int panel_lcd_ops_valid_mode(struct device *dev,
			struct drm_display_mode *mode)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);

	drm_display_mode_to_videomode(mode, &display->vm);

	display->width_mm = mode->width_mm;
	display->height_mm = mode->height_mm;

	return MODE_OK;
}

static void panel_lcd_ops_set_mode(struct device *dev,
			struct drm_display_mode *mode)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;

	DRM_DEBUG_KMS("enter\n");

	if (ops->set_mode)
		ops->set_mode(display, mode, 0);
}

static void panel_lcd_work(struct work_struct *work)
{
	struct lcd_context *ctx = container_of(work,
			struct lcd_context, lcd_power_work);
	struct gpio_desc **desc;
	int i;

	if (ctx->enable_gpios) {
		desc = ctx->enable_gpios->desc;
		for (i = 0; ctx->enable_gpios->ndescs > i; i++) {
			DRM_DEBUG_KMS("LCD gpio.%d ative %s %dms\n",
				desc_to_gpio(desc[i]),
				ctx->gpios_active[i] == GPIO_ACTIVE_HIGH
				? "high" : "low ", ctx->gpios_delay[i]);

			gpiod_set_value_cansleep(desc[i],
				ctx->gpios_active[i] == GPIO_ACTIVE_HIGH ?
								1 : 0);
			if (ctx->gpios_delay[i])
				mdelay(ctx->gpios_delay[i]);
		}
	}

	if (ctx->backlight) {
		if (ctx->backlight_delay)
			mdelay(ctx->backlight_delay);

		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	}
}

static void panel_lcd_on(struct lcd_context *ctx)
{
	struct nx_drm_connector *nx_connector = &ctx->connector;
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;

	DRM_DEBUG_KMS("enter\n");

	if (nx_connector->suspended) {
		if (ops->resume)
			ops->resume(display);
	}

	if (ops->prepare)
		ops->prepare(display);

	if (display->panel)
		drm_panel_prepare(display->panel);

	if (display->panel)
		drm_panel_enable(display->panel);

	/* last enable display to prevent LCD fliker */
	if (ops->enable)
		ops->enable(display);
}

static void panel_lcd_off(struct lcd_context *ctx)
{
	struct nx_drm_connector *nx_connector = &ctx->connector;
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;

	DRM_DEBUG_KMS("enter\n");

	if (nx_connector->suspended) {
		if (ops->suspend)
			ops->suspend(display);
	}

	if (display->panel)
		drm_panel_unprepare(display->panel);

	if (display->panel)
		drm_panel_disable(display->panel);

	if (ops->unprepare)
		ops->unprepare(display);

	if (ops->disable)
		ops->disable(display);
}

static void panel_lcd_ops_enable(struct device *dev)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);
	enum nx_panel_type panel_type = nx_panel_get_type(display);

	DRM_DEBUG_KMS("%s\n", nx_panel_get_name(panel_type));
	if (ctx->enabled)
		return;

	panel_lcd_on(ctx);

	schedule_work(&ctx->lcd_power_work);
	ctx->enabled = true;
}

static void panel_lcd_ops_disable(struct device *dev)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);
	enum nx_panel_type panel_type = nx_panel_get_type(display);
	struct gpio_desc **desc;
	int i;

	DRM_DEBUG_KMS("%s\n", nx_panel_get_name(panel_type));
	if (!ctx->enabled)
		return;

	cancel_work_sync(&ctx->lcd_power_work);

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->backlight);
	}

	panel_lcd_off(ctx);

	if (ctx->enable_gpios) {
		desc = ctx->enable_gpios->desc;
		for (i = 0; ctx->enable_gpios->ndescs > i; i++)
			gpiod_set_value_cansleep(desc[i],
				ctx->gpios_active[i] ==
				GPIO_ACTIVE_HIGH ? 0 : 1);
	}
	ctx->enabled = false;
}

static int panel_mipi_attach(struct mipi_dsi_host *host,
			struct mipi_dsi_device *device)
{
	struct mipi_resource *mipi =
			container_of(host, struct mipi_resource, mipi_host);
	struct lcd_context *ctx = container_of(mipi, struct lcd_context, mipi);
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_mipi_dsi_ops *dsi_ops = display->ops->dsi;
	struct drm_connector *connector = &ctx->connector.connector;

	mipi->lanes = device->lanes;
	mipi->format = device->format;
	mipi->flags = device->mode_flags;
	mipi->mipi_dev = device;

	/* set mipi panel node */
	display->panel_node = device->dev.of_node;

	if (dsi_ops && dsi_ops->set_format)
		dsi_ops->set_format(display, device);

	if (connector->dev)
		drm_helper_hpd_irq_event(connector->dev);

	DRM_INFO("mipi: %s lanes:%d, format:%d, flags:%lx\n",
		dev_name(&device->dev), device->lanes,
		device->format, device->mode_flags);

	return 0;
}

static int panel_mipi_detach(struct mipi_dsi_host *host,
			struct mipi_dsi_device *device)
{
	struct mipi_resource *mipi =
			container_of(host, struct mipi_resource, mipi_host);
	struct lcd_context *ctx = container_of(mipi, struct lcd_context, mipi);
	struct nx_drm_display *display = ctx_to_display(ctx);

	DRM_DEBUG_KMS("enter\n");

	display->panel_node = NULL;
	display->panel = NULL;

	return 0;
}

static ssize_t panel_mipi_transfer(struct mipi_dsi_host *host,
			const struct mipi_dsi_msg *msg)
{
	struct mipi_resource *mipi =
			container_of(host, struct mipi_resource, mipi_host);
	struct lcd_context *ctx = container_of(mipi, struct lcd_context, mipi);
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;
	struct nx_drm_mipi_dsi_ops *dsi_ops = ops->dsi;
	int ret = -1;

	if (dsi_ops && dsi_ops->transfer)
		ret = dsi_ops->transfer(display, host, msg);

	return ret;
}

static struct mipi_dsi_host_ops panel_mipi_ops = {
	.attach = panel_mipi_attach,
	.detach = panel_mipi_detach,
	.transfer = panel_mipi_transfer,
};

static struct nx_drm_connect_drv_ops lcd_connector_ops = {
	.detect = panel_lcd_ops_detect,
	.is_connected = panel_lcd_ops_is_connected,
	.get_modes = panel_lcd_ops_get_modes,
	.valid_mode = panel_lcd_ops_valid_mode,
	.set_mode = panel_lcd_ops_set_mode,
	.enable = panel_lcd_ops_enable,
	.disable = panel_lcd_ops_disable,
};

static int panel_lcd_bind(struct device *dev,
			struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct drm_connector *connector = &ctx->connector.connector;
	enum nx_panel_type panel_type = nx_panel_get_type(ctx_to_display(ctx));
	struct platform_driver *drv;
	int err = 0;

	DRM_INFO("Bind %s panel\n", nx_panel_get_name(panel_type));

	err = nx_drm_connector_attach(drm, connector,
			ctx->crtc_pipe, ctx->possible_crtcs_mask, panel_type);

	if (err < 0)
		goto err_bind;

	if (IS_ENABLED(CONFIG_DRM_NX_MIPI_DSI)) {
		if (panel_type == NX_PANEL_TYPE_MIPI) {
			struct mipi_resource *mipi = &ctx->mipi;

			err = mipi_dsi_host_register(&mipi->mipi_host);
		}
	}

	if (!err) {
		struct nx_drm_private *private = drm->dev_private;

		if (panel_lcd_ops_detect(dev, connector))
			private->force_detect = true;

		return 0;
	}

err_bind:
	drv = to_platform_driver(dev->driver);
	if (drv->remove)
		drv->remove(to_platform_device(dev));

	return 0;
}

static void panel_lcd_unbind(struct device *dev,
			struct device *master, void *data)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct drm_connector *connector = &ctx->connector.connector;

	if (IS_ENABLED(CONFIG_DRM_NX_MIPI_DSI)) {
		enum nx_panel_type panel_type =
				nx_panel_get_type(ctx_to_display(ctx));

		if (panel_type == NX_PANEL_TYPE_MIPI) {
			struct mipi_resource *mipi =  &ctx->mipi;

			mipi_dsi_host_unregister(&mipi->mipi_host);
		}
	}

	if (display->panel)
		drm_panel_detach(display->panel);

	if (connector)
		nx_drm_connector_detach(connector);
}

static const struct component_ops panel_comp_ops = {
	.bind = panel_lcd_bind,
	.unbind = panel_lcd_unbind,
};

#define property_read(n, s, v)	of_property_read_u32(n, s, &v)

static const struct of_device_id panel_lcd_of_match[];

static int panel_lcd_parse_dt(struct platform_device *pdev,
			struct lcd_context *ctx)
{
	struct device *dev = &pdev->dev;
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct device_node *node = dev->of_node;
	struct device_node *np;
	struct display_timing timing;
	int err;
#ifdef CONFIG_BACKLIGHT_CLASS_DEVICE
	struct device_node *backlight;
#endif

	DRM_DEBUG_KMS("enter\n");

	/* get crtcs params */
	property_read(node, "crtc-pipe", ctx->crtc_pipe);
	property_read(node, "crtcs-possible-mask", ctx->possible_crtcs_mask);

	/* get panel timing from local. */
	np = of_graph_get_remote_port_parent(node);
	display->panel_node = np;
	if (!np) {
		struct gpio_descs *gpios;
		struct gpio_desc **desc = NULL;
		int i, ngpios = 0;

		DRM_INFO("not use remote panel node (%s) !\n",
			node->full_name);

		/* parse panel gpios */
		gpios = devm_gpiod_get_array(dev, "enable", GPIOD_ASIS);
		if (-EBUSY == (long)ERR_CAST(gpios)) {
			DRM_INFO("Failed, enable-gpios is busy : %s !!!\n",
				node->full_name);
			gpios = NULL;
		}

		if (!IS_ERR(gpios) && gpios) {
			ngpios = gpios->ndescs;
			desc = gpios->desc;
			ctx->enable_gpios = gpios;	/* set enable_gpios */
			of_property_read_u32_array(node,
				"enable-gpios-delay", ctx->gpios_delay,
				(ngpios-1));
		}

		for (i = 0; ngpios > i; i++) {
			enum of_gpio_flags flags;
			int gpio;

			gpio = of_get_named_gpio_flags(node,
						"enable-gpios", i, &flags);
			if (!gpio_is_valid(gpio)) {
				DRM_ERROR("invalid gpio #%d: %d\n", i, gpio);
				return -EINVAL;
			}

			ctx->gpios_active[i] = flags;

			/* disable at boottime */
			gpiod_direction_output(desc[i],
					flags == GPIO_ACTIVE_HIGH ? 0 : 1);

			DRM_INFO("LCD enable-gpio.%d act %s\n",
				gpio, flags == GPIO_ACTIVE_HIGH ?
				"high" : "low ");
		}

#ifdef CONFIG_BACKLIGHT_CLASS_DEVICE
		backlight = of_parse_phandle(node, "backlight", 0);
		if (backlight) {
			ctx->backlight = of_find_backlight_by_node(backlight);
			of_node_put(backlight);
			if (ctx->backlight) {
				property_read(node, "backlight-delay",
					ctx->backlight_delay);

				ctx->backlight->props.power =
					FB_BLANK_POWERDOWN;
				backlight_update_status(ctx->backlight);

				DRM_INFO("LCD backlight down, delay:%d\n",
					ctx->backlight_delay);
			}
		}
#endif

		/* parse panel lcd size */
		property_read(node, "width-mm", display->width_mm);
		property_read(node, "height-mm", display->height_mm);

		/*
		 * parse display timing (sync)
		 * refer to "drivers/video/of_display_timing.c"
		 * -> of_parse_display_timing
		 */
		err = of_get_display_timing(node, "display-timing", &timing);
		if (err == 0) {
			videomode_from_timing(&timing, &display->vm);
			ctx->local_timing = true;
		}
	}
	of_node_put(np);

	INIT_WORK(&ctx->lcd_power_work, panel_lcd_work);

	/* parse display control config */
	np = of_find_node_by_name(node, "dp_control");
	of_node_get(node);
	if (!np) {
		DRM_ERROR("Failed, not find panel's control node (%s) !\n",
			node->full_name);
		return -EINVAL;
	}
	of_node_put(np);

	return nx_drm_display_setup(display, np, display->panel_type);
}

static int panel_lcd_get_display(struct platform_device *pdev,
				struct lcd_context *ctx)
{
	struct device *dev = &pdev->dev;
	struct nx_drm_connector *nx_connector = &ctx->connector;
	struct drm_connector *connector = &nx_connector->connector;
	const struct of_device_id *id;
	enum nx_panel_type type;

	/* get panel type with of id */
	id = of_match_node(panel_lcd_of_match, dev->of_node);
	type = (enum nx_panel_type)id->data;

	DRM_INFO("Load %s panel\n", nx_panel_get_name(type));

	/* get display for RGB/LVDS/MiPi-DSI */
	nx_connector->display =
		nx_drm_display_get(dev, dev->of_node, connector, type);
	if (!nx_connector->display)
		return -ENOENT;

	return 0;
}

static int panel_lcd_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct lcd_context *ctx;
	struct nx_drm_display_ops *ops;
	enum nx_panel_type panel_type;
	size_t size;
	int err;

	DRM_DEBUG_KMS("enter (%s)\n", dev_name(dev));

	size = sizeof(*ctx);
	ctx = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->connector.dev = dev;
	ctx->connector.ops = &lcd_connector_ops;

	err = panel_lcd_get_display(pdev, ctx);
	if (err < 0)
		goto err_probe;

	ops = ctx_to_display(ctx)->ops;
	if (ops->open) {
		err = ops->open(ctx_to_display(ctx), ctx->crtc_pipe);
		if (err)
			goto err_probe;
	}

	err = panel_lcd_parse_dt(pdev, ctx);
	if (err < 0)
		goto err_probe;

	panel_type = nx_panel_get_type(ctx_to_display(ctx));

	if (IS_ENABLED(CONFIG_DRM_NX_MIPI_DSI)) {
		if (panel_type == NX_PANEL_TYPE_MIPI) {
			struct mipi_resource *mipi =  &ctx->mipi;

			mipi->mipi_host.ops = &panel_mipi_ops;
			mipi->mipi_host.dev = dev;
		}
	}

	dev_set_drvdata(dev, ctx);
	component_add(dev, &panel_comp_ops);

	DRM_DEBUG_KMS("done\n");
	return err;

err_probe:
	DRM_ERROR("Failed %s probe !!!\n", dev_name(dev));

	if (ctx && ctx->backlight)
		put_device(&ctx->backlight->dev);

	devm_kfree(dev, ctx);
	return err;
}

static int panel_lcd_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct lcd_context *ctx = dev_get_drvdata(&pdev->dev);
	struct nx_drm_display_ops *ops;
	int i = 0;

	DRM_DEBUG_KMS("enter (%s)\n", dev_name(dev));
	if (!ctx)
		return 0;

	component_del(dev, &panel_comp_ops);

	ops = ctx_to_display(ctx)->ops;
	if (ops->close)
		ops->close(ctx_to_display(ctx), ctx->crtc_pipe);

	nx_drm_display_put(dev, ctx_to_display(ctx));

	if (ctx->enable_gpios) {
		struct gpio_desc **desc = ctx->enable_gpios->desc;

		for (i = 0; ctx->enable_gpios->ndescs > i; i++)
			devm_gpiod_put(dev, desc[i]);
	}

	if (ctx->backlight)
		put_device(&ctx->backlight->dev);

	devm_kfree(dev, ctx);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int panel_lcd_suspend(struct device *dev)
{
	return 0;
}

static int panel_lcd_resume(struct device *dev)
{
	return 0;
}
#endif

static const struct dev_pm_ops panel_lcd_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(
		panel_lcd_suspend, panel_lcd_resume
	)
};

static const struct of_device_id panel_lcd_of_match[] = {
#ifdef CONFIG_DRM_NX_RGB
	{
		.compatible = "nexell,s5pxx18-drm-rgb",
		.data = (void *)NX_PANEL_TYPE_RGB
	},
#endif
#ifdef CONFIG_DRM_NX_LVDS
	{
		.compatible = "nexell,s5pxx18-drm-lvds",
		.data = (void *)NX_PANEL_TYPE_LVDS
	},
#endif
#ifdef CONFIG_DRM_NX_MIPI_DSI
	{
		.compatible = "nexell,s5pxx18-drm-mipi",
		.data = (void *)NX_PANEL_TYPE_MIPI
	},
#endif
	{}
};
MODULE_DEVICE_TABLE(of, panel_lcd_of_match);

static struct platform_driver panel_lcd_driver = {
	.probe = panel_lcd_probe,
	.remove = panel_lcd_remove,
	.driver = {
		.name = "nexell,display_drm_lcd",
		.owner = THIS_MODULE,
		.of_match_table = panel_lcd_of_match,
		.pm = &panel_lcd_pm,
	},
};

void panel_lcd_init(void)
{
	platform_driver_register(&panel_lcd_driver);
}

void panel_lcd_exit(void)
{
	platform_driver_unregister(&panel_lcd_driver);
}
