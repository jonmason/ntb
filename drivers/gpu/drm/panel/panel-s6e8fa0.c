/*
 * MIPI-DSI based s6e8fa0 AMOLED LCD 4.99 inch panel driver.
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * Chanho Park <chanho61.park@samsung.com>
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

#define MIPI_DCS_CUSTOM_HBM_MODE		0x53
#define MIPI_DCS_CUSTOM_HBM_MODE_NONE		0x28
#define MIPI_DCS_CUSTOM_HBM_MODE_MEDIUM		0x68
#define MIPI_DCS_CUSTOM_HBM_MODE_MAX		0xe8

#define MIPI_DCS_CUSTOM_DIMMING_SET		0x51

#define MIN_BRIGHTNESS				0
#define MAX_BRIGHTNESS				255
#define DEFAULT_BRIGHTNESS			160

struct s6e8fa0 {
	struct device *dev;
	struct drm_panel panel;

	struct regulator_bulk_data supplies[2];
	int reset_gpio;
	u32 power_on_delay;
	u32 reset_delay;
	u32 init_delay;
	bool flip_horizontal;
	bool flip_vertical;
	struct videomode vm;
	u32 width_mm;
	u32 height_mm;
	bool is_power_on;

	struct backlight_device *bl_dev;
	u8 id[3];
	/* This field is tested by functions directly accessing DSI bus before
	 * transfer, transfer is skipped if it is set. In case of transfer
	 * failure or unexpected response the field is set to error value.
	 * Such construct allows to eliminate many checks in higher level
	 * functions.
	 */
	int error;
};

static inline struct s6e8fa0 *panel_to_s6e8fa0(struct drm_panel *panel)
{
	return container_of(panel, struct s6e8fa0, panel);
}

static int s6e8fa0_clear_error(struct s6e8fa0 *ctx)
{
	int ret = ctx->error;

	ctx->error = 0;
	return ret;
}

static void s6e8fa0_dcs_write(struct s6e8fa0 *ctx, const void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;

	if (ctx->error < 0)
		return;

	ret = mipi_dsi_dcs_write_buffer(dsi, data, len);
	if (ret < 0) {
		dev_err(ctx->dev, "error %zd writing dcs seq: %*ph\n", ret,
			(int)len, data);
		ctx->error = ret;
	}
}

static int s6e8fa0_dcs_read(struct s6e8fa0 *ctx, u8 cmd, void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	int ret;

	if (ctx->error < 0)
		return ctx->error;

	ret = mipi_dsi_dcs_read(dsi, cmd, data, len);
	if (ret < 0) {
		dev_err(ctx->dev, "error %d reading dcs seq(%#x)\n", ret, cmd);
		ctx->error = ret;
	}

	return ret;
}

#define s6e8fa0_dcs_write_seq(ctx, seq...) \
({\
	const u8 d[] = { seq };\
	BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64, "DCS sequence too big for stack");\
	s6e8fa0_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

#define s6e8fa0_dcs_write_seq_static(ctx, seq...) \
({\
	static const u8 d[] = { seq };\
	s6e8fa0_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

static void s6e8fa0_apply_level_2_key(struct s6e8fa0 *ctx)
{
	s6e8fa0_dcs_write_seq_static(ctx, 0xf0, 0x5a, 0x5a);
}

static void s6e8fa0_seq_test_key_on_f1(struct s6e8fa0 *ctx)
{
	s6e8fa0_dcs_write_seq_static(ctx, 0xf1, 0x5a, 0x5a);
}

static void s6e8fa0_seq_test_key_on_fc(struct s6e8fa0 *ctx)
{
	s6e8fa0_dcs_write_seq_static(ctx, 0xfc, 0x5a, 0x5a);
}

static void s6e8fa0_set_maximum_return_packet_size(struct s6e8fa0 *ctx,
						   u16 size)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	int ret;

	if (ctx->error < 0)
		return;

	ret = mipi_dsi_set_maximum_return_packet_size(dsi, size);
	if (ret < 0) {
		dev_err(ctx->dev,
			"error %d setting maximum return packet size to %d\n",
			ret, size);
		ctx->error = ret;
	}
}

static void s6e8fa0_read_mtp_id(struct s6e8fa0 *ctx)
{
	int ret;
	int id_len = ARRAY_SIZE(ctx->id);

	ret = s6e8fa0_dcs_read(ctx, 0x04, ctx->id, id_len);
	if (ret < id_len || ctx->error < 0) {
		dev_err(ctx->dev, "read id failed\n");
		ctx->error = -EIO;
		return;
	}
}

static void s6e8fa0_set_sequence(struct s6e8fa0 *ctx)
{
	s6e8fa0_set_maximum_return_packet_size(ctx, 3);
	s6e8fa0_read_mtp_id(ctx);

	if (ctx->error != 0)
		return;

	usleep_range(17000, 18000);

	s6e8fa0_apply_level_2_key(ctx);

	s6e8fa0_seq_test_key_on_f1(ctx);
	s6e8fa0_seq_test_key_on_fc(ctx);
	/* Scan timing1 b0 */
	s6e8fa0_dcs_write_seq_static(ctx, 0xb0, 0x29);
	/* Scan timing1 fe */
	s6e8fa0_dcs_write_seq_static(ctx, 0xfe, 0x01, 0x12, 0x22, 0x8c, 0xa2,
				     0x00, 0x80, 0x0A, 0x01);
	s6e8fa0_dcs_write_seq_static(ctx, MIPI_DCS_EXIT_SLEEP_MODE);

	/* Error flags on */
	s6e8fa0_dcs_write_seq_static(ctx, 0xe7, 0xed, 0xc7, 0x23, 0x63);

	msleep(25);

	/* LTPS f2 */
	s6e8fa0_dcs_write_seq_static(ctx, 0xf2, 0x00, 0x04, 0x0c);
	/* LTPS b0 */
	s6e8fa0_dcs_write_seq_static(ctx, 0xb0, 0x20);
	/* LTPS cb */
	s6e8fa0_dcs_write_seq_static(ctx, 0xcb, 0x02);
	/* LTPS f7 */
	s6e8fa0_dcs_write_seq_static(ctx, 0xf7, 0x03);

	msleep(100);

	/* Scan timing2 fd */
	s6e8fa0_dcs_write_seq_static(ctx, 0xfd, 0x14, 0x09);
	/* Scan timing2 c0 */
	s6e8fa0_dcs_write_seq_static(ctx, 0xc0, 0x00, 0x02, 0x03, 0x32, 0xc0,
				     0x44, 0x44, 0xc0, 0x00, 0x08, 0x20, 0xc0);

	/* ETC condition set */
	s6e8fa0_dcs_write_seq_static(ctx, 0x55, 0x00);

	usleep_range(17000, 18000);
	s6e8fa0_dcs_write_seq_static(ctx, MIPI_DCS_CUSTOM_DIMMING_SET, 0xff);
	s6e8fa0_dcs_write_seq_static(ctx, MIPI_DCS_CUSTOM_HBM_MODE,
			MIPI_DCS_CUSTOM_HBM_MODE_NONE);
}

static int s6e8fa0_get_brightness(struct backlight_device *bl_dev)
{
	return bl_dev->props.brightness;
}

static int s6e8fa0_set_brightness(struct backlight_device *bl_dev)
{
	struct s6e8fa0 *ctx = (struct s6e8fa0 *)bl_get_data(bl_dev);
	unsigned int brightness = bl_dev->props.brightness;
	/* FIXME: MIPI_DSI_DCS_SHORT_WRITE_PARAM is not working properly, the
	 * panel is turned on from Power key source.
	 */
	u8 gamma_update[3] = { 0x51, };

	if (brightness < MIN_BRIGHTNESS ||
		brightness > bl_dev->props.max_brightness) {
		dev_err(ctx->dev, "Invalid brightness: %u\n", brightness);
		return -EINVAL;
	}

	if (!ctx->is_power_on) {
		return -ENODEV;
	}

	/* TODO: support only over 60nit */
	gamma_update[1] = brightness;
	s6e8fa0_dcs_write(ctx, gamma_update, ARRAY_SIZE(gamma_update));

	return 0;
}

static const struct backlight_ops s6e8fa0_bl_ops = {
	.get_brightness = s6e8fa0_get_brightness,
	.update_status = s6e8fa0_set_brightness,
};

static int s6e8fa0_power_on(struct s6e8fa0 *ctx)
{
	int ret;

	if (ctx->is_power_on)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0)
		return ret;

	msleep(ctx->power_on_delay);

	gpio_direction_output(ctx->reset_gpio, 1);
	usleep_range(5000, 6000);
	gpio_set_value(ctx->reset_gpio, 0);
	usleep_range(5000, 6000);
	gpio_set_value(ctx->reset_gpio, 1);

	msleep(ctx->reset_delay);

	ctx->is_power_on = true;

	return 0;
}

static int s6e8fa0_power_off(struct s6e8fa0 *ctx)
{
	if (!ctx->is_power_on)
		return 0;

	gpio_set_value(ctx->reset_gpio, 0);
	usleep_range(5000, 6000);

	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	ctx->is_power_on = false;

	return 0;
}

static int s6e8fa0_disable(struct drm_panel *panel)
{
	struct s6e8fa0 *ctx = panel_to_s6e8fa0(panel);

	s6e8fa0_dcs_write_seq_static(ctx, MIPI_DCS_SET_DISPLAY_OFF);
	if (ctx->error != 0)
		return ctx->error;

	msleep(35);

	s6e8fa0_dcs_write_seq_static(ctx, MIPI_DCS_ENTER_SLEEP_MODE);
	if (ctx->error != 0)
		return ctx->error;

	msleep(125);

	return 0;
}

static int s6e8fa0_unprepare(struct drm_panel *panel)
{
	struct s6e8fa0 *ctx = panel_to_s6e8fa0(panel);
	int ret;

	ret = s6e8fa0_power_off(ctx);
	if (ret)
		return ret;

	s6e8fa0_clear_error(ctx);

	return 0;
}

static int s6e8fa0_prepare(struct drm_panel *panel)
{
	struct s6e8fa0 *ctx = panel_to_s6e8fa0(panel);
	int ret;

	ret = s6e8fa0_power_on(ctx);
	if (ret < 0)
		return ret;

	s6e8fa0_set_sequence(ctx);
	ret = ctx->error;

	if (ret < 0)
		s6e8fa0_unprepare(panel);

	return ret;
}

static int s6e8fa0_enable(struct drm_panel *panel)
{
	struct s6e8fa0 *ctx = panel_to_s6e8fa0(panel);

	s6e8fa0_dcs_write_seq_static(ctx, MIPI_DCS_SET_DISPLAY_ON);
	if (ctx->error != 0)
		return ctx->error;

	msleep(20);

	/* Brightness control rev_a2 */
	s6e8fa0_dcs_write_seq_static(ctx, 0xb2, 0x01, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x06);

	/* Gamma update */
	s6e8fa0_dcs_write_seq_static(ctx, 0xf7, 0x03);

	return 0;
}

static int s6e8fa0_get_modes(struct drm_panel *panel)
{
	struct drm_connector *connector = panel->connector;
	struct s6e8fa0 *ctx = panel_to_s6e8fa0(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("failed to create a new display mode\n");
		return 0;
	}

	drm_display_mode_from_videomode(&ctx->vm, mode);
	mode->width_mm = ctx->width_mm;
	mode->height_mm = ctx->height_mm;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs s6e8fa0_drm_funcs = {
	.disable = s6e8fa0_disable,
	.unprepare = s6e8fa0_unprepare,
	.prepare = s6e8fa0_prepare,
	.enable = s6e8fa0_enable,
	.get_modes = s6e8fa0_get_modes,
};

static int s6e8fa0_parse_dt(struct s6e8fa0 *ctx)
{
	struct device *dev = ctx->dev;
	struct device_node *np = dev->of_node;
	int ret;

	ret = of_get_videomode(np, &ctx->vm, 0);
	if (ret < 0)
		return ret;

	of_property_read_u32(np, "power-on-delay", &ctx->power_on_delay);
	of_property_read_u32(np, "reset-delay", &ctx->reset_delay);
	of_property_read_u32(np, "init-delay", &ctx->init_delay);
	of_property_read_u32(np, "panel-width-mm", &ctx->width_mm);
	of_property_read_u32(np, "panel-height-mm", &ctx->height_mm);

	ctx->flip_horizontal = of_property_read_bool(np, "flip-horizontal");
	ctx->flip_vertical = of_property_read_bool(np, "flip-vertical");

	return 0;
}

static int s6e8fa0_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct s6e8fa0 *ctx;
	int ret;

	if (!drm_panel_connected("s6e8fa0"))
		return -ENODEV;

	ctx = devm_kzalloc(dev, sizeof(struct s6e8fa0), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;

	ctx->is_power_on = false;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST
		| MIPI_DSI_MODE_VIDEO_HFP | MIPI_DSI_MODE_VIDEO_HBP
		| MIPI_DSI_MODE_VIDEO_HSA | MIPI_DSI_MODE_VSYNC_FLUSH;

	ret = s6e8fa0_parse_dt(ctx);
	if (ret < 0)
		return ret;

	ctx->supplies[0].supply = "vdd3";
	ctx->supplies[1].supply = "vci";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies),
				      ctx->supplies);
	if (ret < 0)
		dev_warn(dev, "failed to get regulators: %d\n", ret);

	ctx->reset_gpio = of_get_named_gpio(dev->of_node, "reset-gpio", 0);
	if (ctx->reset_gpio < 0) {
		dev_err(dev, "cannot get reset-gpios %d\n",
			ctx->reset_gpio);
		return ctx->reset_gpio;
	}

	ret = devm_gpio_request(dev, ctx->reset_gpio, "reset-gpio");
	if (ret) {
		dev_err(dev, "failed to request reset-gpio\n");
		return ret;
	}

	ctx->bl_dev = backlight_device_register("s6e8fa0", dev, ctx,
						&s6e8fa0_bl_ops, NULL);
	if (IS_ERR(ctx->bl_dev)) {
		dev_err(dev, "failed to register backlight device\n");
		return PTR_ERR(ctx->bl_dev);
	}

	ctx->bl_dev->props.max_brightness = MAX_BRIGHTNESS;
	ctx->bl_dev->props.brightness = DEFAULT_BRIGHTNESS;
	ctx->bl_dev->props.power = FB_BLANK_POWERDOWN;

	drm_panel_init(&ctx->panel);
	ctx->panel.dev = dev;
	ctx->panel.funcs = &s6e8fa0_drm_funcs;

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

static int s6e8fa0_remove(struct mipi_dsi_device *dsi)
{
	struct s6e8fa0 *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
	backlight_device_unregister(ctx->bl_dev);
	s6e8fa0_power_off(ctx);

	return 0;
}

static void s6e8fa0_shutdown(struct mipi_dsi_device *dsi)
{
	struct s6e8fa0 *ctx = mipi_dsi_get_drvdata(dsi);

	s6e8fa0_power_off(ctx);
}

static const struct of_device_id s6e8fa0_of_match[] = {
	{ .compatible = "samsung,s6e8fa0" },
	{ }
};
MODULE_DEVICE_TABLE(of, s6e8fa0_of_match);

static struct mipi_dsi_driver s6e8fa0_driver = {
	.probe = s6e8fa0_probe,
	.remove = s6e8fa0_remove,
	.shutdown = s6e8fa0_shutdown,
	.driver = {
		.name = "panel-samsung-s6e8fa0",
		.of_match_table = s6e8fa0_of_match,
	},
};
module_mipi_dsi_driver(s6e8fa0_driver);

MODULE_AUTHOR("Chanho Park <chanho61.park@samsung.com>");
MODULE_DESCRIPTION("MIPI-DSI based s6e8fa0 AMOLED LCD Panel Driver");
MODULE_LICENSE("GPL v2");
