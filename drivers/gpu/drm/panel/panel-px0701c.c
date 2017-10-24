/*
 * MIPI-DSI based px0701c1881099a LCD panel driver.
 *
 * Copyright (c) 2017 Nexell Co., Ltd
 *
 * Sungwoo Park <swpark@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <drm/drmP.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>
#include <linux/backlight.h>

#define PX0701C_WIDTH_MM		94  /* real: 94.2 */
#define PX0701C_HEIGHT_MM		150 /* real: 150.72 */

struct px0701c {
	struct device *dev;
	struct drm_panel panel;

	struct regulator_bulk_data supplies[2];
	int reset_gpio;
	int enable_gpio;
	u32 power_on_delay;
	u32 reset_delay;
	u32 init_delay;
	struct videomode vm;
	u32 width_mm;
	u32 height_mm;
	bool is_power_on;

	struct backlight_device *bl_dev;
};

static inline struct px0701c *panel_to_px0701c(struct drm_panel *panel)
{
	return container_of(panel, struct px0701c, panel);
}

static int px0701c_power_on(struct px0701c *ctx)
{
	int ret;

	if (ctx->is_power_on)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0)
		return ret;

	msleep(ctx->power_on_delay);

	gpio_direction_output(ctx->reset_gpio, 1);
	gpio_set_value(ctx->reset_gpio, 0);
	msleep(ctx->reset_delay);
	gpio_set_value(ctx->reset_gpio, 1);
	msleep(ctx->init_delay);

	ctx->is_power_on = true;

	return 0;
}

static int px0701c_power_off(struct px0701c *ctx)
{
	if (!ctx->is_power_on)
		return 0;

	gpio_set_value(ctx->reset_gpio, 0);
	usleep_range(5000, 6000);

	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	ctx->is_power_on = false;

	return 0;
}

static int px0701c_enable(struct drm_panel *panel)
{
	struct px0701c *ctx = panel_to_px0701c(panel);

	if (ctx->bl_dev) {
		ctx->bl_dev->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->bl_dev);
	}

	if (ctx->enable_gpio > 0) {
		gpio_direction_output(ctx->enable_gpio, 1);
		gpio_set_value(ctx->enable_gpio, 1);
	}

	return 0;
}

static int px0701c_disable(struct drm_panel *panel)
{
	struct px0701c *ctx = panel_to_px0701c(panel);

	if (ctx->bl_dev) {
		ctx->bl_dev->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->bl_dev);
	}

	if (ctx->enable_gpio > 0)
		gpio_set_value(ctx->enable_gpio, 0);

	return 0;
}

static int px0701c_prepare(struct drm_panel *panel)
{
	struct px0701c *ctx = panel_to_px0701c(panel);

	return px0701c_power_on(ctx);
}

static int px0701c_unprepare(struct drm_panel *panel)
{
	struct px0701c *ctx = panel_to_px0701c(panel);

	return px0701c_power_off(ctx);
}

static const struct drm_display_mode default_mode = {
	.clock = 67161,			/* KHz */
	.hdisplay = 800,
	.hsync_start = 800 + 40,	/* hactive + hbackporch */
	.hsync_end = 800 + 40 + 4,	/* hsync_start + hsyncwidth */
	.htotal = 800 + 40 + 4 + 40,	/* hsync_end + hfrontporch */
	.vdisplay = 1280,
	.vsync_start = 1280 + 22,	/* vactive + vbackporch */
	.vsync_end = 1280 + 22 + 2,	/* vsync_start + vsyncwidth */
	.vtotal = 1280 + 22 + 2 + 16,	/* vsync_end + vfrontporch */
	.vrefresh = 60,			/* Hz */
};

/**
 * HACK
 * return value
 * 1: success
 * 0: failure
 */
static int px0701c_get_modes(struct drm_panel *panel)
{
	struct drm_connector *connector = panel->connector;
	struct px0701c *ctx = panel_to_px0701c(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("failed to create a new display mode\n");
		return 0;
	}

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	drm_mode_set_name(mode);
	mode->width_mm = ctx->width_mm;
	mode->height_mm = ctx->height_mm;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs px0701c_drm_funcs = {
	.enable = px0701c_enable,
	.disable = px0701c_disable,
	.prepare = px0701c_prepare,
	.unprepare = px0701c_unprepare,
	.get_modes = px0701c_get_modes,
};

static int px0701c_parse_dt(struct px0701c *ctx)
{
	struct device *dev = ctx->dev;
	struct device_node *np = dev->of_node;

	of_property_read_u32(np, "power-on-delay", &ctx->power_on_delay);
	of_property_read_u32(np, "reset-delay", &ctx->reset_delay);
	of_property_read_u32(np, "init-delay", &ctx->init_delay);

	return 0;
}

static int px0701c_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *backlight;
	struct px0701c *ctx;
	int ret;

	if (!drm_panel_connected("px0701c"))
		return -ENODEV;

	ctx = devm_kzalloc(dev, sizeof(struct px0701c), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;

	ctx->is_power_on = false;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO
		| MIPI_DSI_MODE_VIDEO_HFP | MIPI_DSI_MODE_VIDEO_HBP
		| MIPI_DSI_MODE_VIDEO_HSA | MIPI_DSI_MODE_VSYNC_FLUSH;

	ret = px0701c_parse_dt(ctx);
	if (ret)
		return ret;

	/* TODO: mapping real power name */
	ctx->supplies[0].supply = "vdd3";
	ctx->supplies[1].supply = "vci";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0)
		dev_warn(dev, "failed to get regulators: %d\n", ret);

	ctx->reset_gpio = of_get_named_gpio(dev->of_node, "reset-gpio", 0);
	if (ctx->reset_gpio < 0) {
		dev_err(dev, "cannot get reset-gpio %d\n", ctx->reset_gpio);
		return -EINVAL;
	}

	ret = devm_gpio_request(dev, ctx->reset_gpio, "reset-gpio");
	if (ret) {
		dev_err(dev, "failed to request reset-gpio\n");
		return ret;
	}

	ctx->enable_gpio = of_get_named_gpio(dev->of_node, "enable-gpio", 0);
	if (ctx->enable_gpio < 0)
		dev_warn(dev, "cannot get enable-gpio %d\n", ctx->enable_gpio);
	else {
		ret = devm_gpio_request(dev, ctx->enable_gpio, "enable-gpio");
		if (ret) {
			dev_err(dev, "failed to request enable-gpio\n");
			return ret;
		}
	}

	backlight = of_parse_phandle(dev->of_node, "backlight", 0);
	if (backlight) {
		ctx->bl_dev = of_find_backlight_by_node(backlight);
		of_node_put(backlight);

		if (!ctx->bl_dev)
			return -EPROBE_DEFER;
	}

	ctx->width_mm = PX0701C_WIDTH_MM;
	ctx->height_mm = PX0701C_HEIGHT_MM;

	drm_panel_init(&ctx->panel);
	ctx->panel.dev = dev;
	ctx->panel.funcs = &px0701c_drm_funcs;

	ret = drm_panel_add(&ctx->panel);
	if (ret < 0) {
		backlight_device_unregister(ctx->bl_dev);
		return ret;
	}

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		backlight_device_unregister(ctx->bl_dev);
		drm_panel_remove(&ctx->panel);
	}

	return ret;
}

static int px0701c_remove(struct mipi_dsi_device *dsi)
{
	struct px0701c *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
	backlight_device_unregister(ctx->bl_dev);
	px0701c_power_off(ctx);

	return 0;
}

static void px0701c_shutdown(struct mipi_dsi_device *dsi)
{
	struct px0701c *ctx = mipi_dsi_get_drvdata(dsi);

	px0701c_power_off(ctx);
}

static const struct of_device_id px0701c_of_match[] = {
	{ .compatible = "px0701c" },
	{ }
};
MODULE_DEVICE_TABLE(of, px0701c_of_match);

static struct mipi_dsi_driver px0701c_driver = {
	.probe = px0701c_probe,
	.remove = px0701c_remove,
	.shutdown = px0701c_shutdown,
	.driver = {
		.name = "panel-px0701c",
		.of_match_table = px0701c_of_match,
	},
};

module_mipi_dsi_driver(px0701c_driver);

MODULE_AUTHOR("Sungwoo Park <swpark@nexell.co.kr>");
MODULE_DESCRIPTION("MIPI-SDI based px0701c series LCD Panel Driver");
MODULE_LICENSE("GPL v2");
