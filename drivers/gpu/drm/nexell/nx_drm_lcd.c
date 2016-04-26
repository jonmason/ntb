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
#include "soc/s5pxx18_drm_dp.h"

struct lcd_mipi_dsi {
	struct mipi_dsi_host mipi_host;
	struct mipi_dsi_device *mipi_dev;
	unsigned long flags;
	enum mipi_dsi_pixel_format format;
	unsigned int lanes;
	struct nx_drm_dp_reg r_base;
};

struct lcd_context {
	struct drm_connector *connector;
	struct drm_encoder *encoder;
	struct gpio_desc *enable_gpio;
	int encoder_port;
	struct reset_control *reset;
	void *base;
	struct nx_drm_dp_dev *dp_dev;
	struct lcd_mipi_dsi mipi_dsi;
	bool panel_timing;
	struct mutex lock;
};

#define ctx_to_ddc(c)	(struct dp_control_dev *)(&c->dp_dev->ddc)
#define ctx_to_dsi(c)	(struct lcd_mipi_dsi *)(&c->mipi_dsi)
#define host_to_dsi(h)	container_of(h, struct lcd_mipi_dsi, mipi_host)
#define dsi_to_ctx(d)	container_of(d, struct lcd_context, mipi_dsi)

static bool panel_lcd_is_connected(struct device *dev,
			struct drm_connector *connector)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_dp_panel *dpp = &ctx->dp_dev->dpp;
	struct device_node *panel_node = dpp->panel_node;

	DRM_DEBUG_KMS("enter panel node %s\n",
		panel_node ? "exist" : "not exist");

	if (panel_node) {
		struct drm_panel *panel = of_drm_find_panel(panel_node);

		if (panel) {
			dpp->panel = panel;
			drm_panel_attach(panel, connector);
		}
	}

	if (!panel_node && false == ctx->panel_timing) {
		struct dp_control_dev *ddc = ctx_to_ddc(ctx);

		DRM_ERROR("not exist %s panel node and display timing !\n",
			dp_panel_type_lcd  == ddc->panel_type ? "RGB"  :
			dp_panel_type_lvds == ddc->panel_type ? "LVDS" :
			dp_panel_type_mipi == ddc->panel_type ? "MiPi" :
			dp_panel_type_hdmi == ddc->panel_type ? "HDMI" :
			"unknown");

		return false;
	}

	/*
	 * if not attached panel,
	 * set with DT's timing node.
	 */
	return true;
}

static int panel_lcd_get_modes(struct device *dev,
			struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_dp_panel *dpp = &ctx->dp_dev->dpp;

	DRM_DEBUG_KMS("enter panel %s\n",
		dpp->panel ? "attached" : "detached");

	if (dpp->panel)
		return drm_panel_get_modes(dpp->panel);

	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("fail : create a new display mode !\n");
		return 0;
	}

	drm_display_mode_from_videomode(&dpp->vm, mode);
	mode->width_mm = dpp->width_mm;
	mode->height_mm = dpp->height_mm;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	DRM_DEBUG_KMS("exit, (%dx%d, flags=0x%x)\n",
		mode->hdisplay, mode->vdisplay, mode->flags);

	return 1;
}

static int panel_lcd_check_mode(struct device *dev,
			struct drm_display_mode *mode)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_dp_panel *dpp = &ctx->dp_dev->dpp;

	drm_display_mode_to_videomode(mode, &dpp->vm);

	DRM_DEBUG_KMS("SYNC -> LCD %d x %d mm\n",
		dpp->width_mm, dpp->height_mm);
	DRM_DEBUG_KMS("ha:%d, hs:%d, hb:%d, hf:%d\n",
		dpp->vm.hactive, dpp->vm.hsync_len,
		dpp->vm.hback_porch, dpp->vm.hfront_porch);
	DRM_DEBUG_KMS("va:%d, vs:%d, vb:%d, vf:%d\n",
		dpp->vm.vactive, dpp->vm.vsync_len,
		dpp->vm.vback_porch, dpp->vm.vfront_porch);
	DRM_DEBUG_KMS("flags:0x%x\n", dpp->vm.flags);

	return MODE_OK;
}

static void panel_lcd_commit(struct device *dev)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);

	DRM_DEBUG_KMS("enter\n");
	nx_soc_dp_device_top_mux(ctx_to_ddc(ctx));
}

static void panel_lcd_prepare(struct device *dev, struct drm_panel *panel)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_dp_dev *ddv = ctx->dp_dev;

	nx_drm_dp_lcd_prepare(ddv, panel);
	drm_panel_prepare(panel);
}

static void panel_lcd_unprepare(struct device *dev, struct drm_panel *panel)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_dp_dev *ddv = ctx->dp_dev;

	drm_panel_unprepare(panel);
	nx_drm_dp_lcd_unprepare(ddv, panel);
}

static void panel_lcd_enable(struct device *dev, struct drm_panel *panel)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_dp_dev *ddv = ctx->dp_dev;

	nx_drm_dp_lcd_prepare(ddv, panel);
	drm_panel_prepare(panel);

	drm_panel_enable(panel);
	nx_drm_dp_lcd_enable(ddv, panel);
}

static void panel_lcd_disable(struct device *dev, struct drm_panel *panel)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_dp_dev *ddv = ctx->dp_dev;

	drm_panel_unprepare(panel);
	drm_panel_disable(panel);

	nx_drm_dp_lcd_unprepare(ddv, panel);
	nx_drm_dp_lcd_disable(ddv, panel);
}

static void panel_lcd_dmps(struct device *dev, int mode)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_dp_panel *dpp = &ctx->dp_dev->dpp;
	struct drm_panel *panel = dpp->panel;

	DRM_DEBUG_KMS("dpms.%d\n", mode);
	switch (mode) {
	case DRM_MODE_DPMS_ON:
		panel_lcd_enable(dev, panel);

		if (!panel && ctx->enable_gpio)
			gpiod_set_value_cansleep(ctx->enable_gpio, 1);
		break;

	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		panel_lcd_disable(dev, panel);

		if (!panel && ctx->enable_gpio)
			gpiod_set_value_cansleep(ctx->enable_gpio, 0);
		break;
	default:
		DRM_ERROR("fail : unspecified mode %d\n", mode);
		break;
	}
}

static int panel_mipi_attach(struct mipi_dsi_host *host,
			struct mipi_dsi_device *device)
{
	struct lcd_mipi_dsi *dsi = host_to_dsi(host);
	struct lcd_context *ctx = dsi_to_ctx(dsi);
	struct nx_drm_dp_panel *dpp = &ctx->dp_dev->dpp;

	dsi->lanes = device->lanes;
	dsi->format = device->format;
	dsi->flags = device->mode_flags;
	dsi->mipi_dev = device;

	/* set panel node */
	dpp->panel_node = device->dev.of_node;

	DRM_INFO("mipi: %s lanes:%d, format:%d, flags:%lx\n",
		dev_name(&device->dev), device->lanes,
		device->format, device->mode_flags);

	return 0;
}

static int panel_mipi_detach(struct mipi_dsi_host *host,
			struct mipi_dsi_device *device)
{
	struct lcd_mipi_dsi *dsi = host_to_dsi(host);
	struct lcd_context *ctx = dsi_to_ctx(dsi);
	struct nx_drm_dp_panel *dpp = &ctx->dp_dev->dpp;

	DRM_DEBUG_KMS("enter\n");

	dpp->panel_node = NULL;

	return 0;
}

static ssize_t panel_mipi_transfer(struct mipi_dsi_host *host,
			const struct mipi_dsi_msg *msg)
{
	return nx_drm_dp_mipi_transfer(host, msg);
}

static struct mipi_dsi_host_ops panel_mipi_ops = {
	.attach = panel_mipi_attach,
	.detach = panel_mipi_detach,
	.transfer = panel_mipi_transfer,
};

static struct nx_drm_dp_ops panel_lcd_ops = {
	.is_connected = panel_lcd_is_connected,
	.get_modes = panel_lcd_get_modes,
	.check_mode = panel_lcd_check_mode,
	.commit = panel_lcd_commit,
	.dmps = panel_lcd_dmps,
};

static struct nx_drm_dp_dev lcd_dp_dev = {
	.ops = &panel_lcd_ops,
};

static int panel_lcd_bind(struct device *dev,
			struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct dp_control_dev *ddc = ctx_to_ddc(ctx);
	int port = ctx->encoder_port;
	int err;

	DRM_DEBUG_KMS("enter\n");

	err = nx_drm_connector_create_and_attach(drm, ctx->dp_dev, port, ctx);
	if (0 > err) {
		DRM_ERROR("fail : create for LCD connector !\n");
		return -EFAULT;
	}

	if (IS_ENABLED(CONFIG_DRM_NX_MIPI_DSI)) {
		if (dp_panel_type_mipi == ddc->panel_type) {
			struct lcd_mipi_dsi *dsi = ctx_to_dsi(ctx);

			err = mipi_dsi_host_register(&dsi->mipi_host);
			if (0 > err)
				DRM_ERROR("fail : register mipi host !\n");

		}
	}

	DRM_DEBUG_KMS("done %d\n", err);
	return err;
}

static void panel_lcd_unbind(struct device *dev,
			struct device *master, void *data)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct dp_control_dev *ddc = ctx_to_ddc(ctx);

	nx_drm_connector_destroy_and_detach(ctx->connector);

	if (IS_ENABLED(CONFIG_DRM_NX_MIPI_DSI)) {
		if (dp_panel_type_mipi == ddc->panel_type) {
			struct lcd_mipi_dsi *dsi = ctx_to_dsi(ctx);

			mipi_dsi_host_unregister(&dsi->mipi_host);
		}
	}
}

static const struct component_ops panel_comp_ops = {
	.bind = panel_lcd_bind,
	.unbind = panel_lcd_unbind,
};

#define parse_read_prop(n, s, v)	{ \
	u32 _v;	\
	if (!of_property_read_u32(n, s, &_v))	\
		v = _v;	\
	}

static int panel_lcd_parse_dt(struct platform_device *pdev,
			struct lcd_context *ctx)
{
	struct device *dev = &pdev->dev;
	struct nx_drm_dp_panel *dpp = &ctx->dp_dev->dpp;
	struct dp_control_dev *ddc = ctx_to_ddc(ctx);
	struct device_node *node = dev->of_node;
	struct device_node *np, *tmp;
	struct display_timing timing;
	int err;

	DRM_DEBUG_KMS("enter\n");

	np = of_graph_get_remote_port_parent(node);
	dpp->panel_node = np;
	if (!np) {
		DRM_INFO("not use remote panel node (%s) !\n",
			node->full_name);
		np = node;
	}

	DRM_DEBUG_KMS("panel node (%s)\n", np->full_name);

	/*
	 * parse panel info
	 */
	ctx->enable_gpio =
			devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);
	parse_read_prop(np, "width-mm", dpp->width_mm);
	parse_read_prop(np, "height-mm", dpp->height_mm);

	/*
	 * parse display timing (sync)
	 * refer to "drivers/video/of_display_timing.c"
	 * -> of_parse_display_timing
	 */
	err = of_get_display_timing(np, "panel-timing", &timing);
	if (0 == err) {
		videomode_from_timing(&timing, &dpp->vm);
		ctx->panel_timing = true;
	}

	/*
	 * if not exist dp-control at remote endpoint,
	 * get from lcd_panel node.
	 */
	tmp = of_find_node_by_name(np, "dp_control");
	if (!tmp) {
		np = node;
		DRM_DEBUG_KMS("panel's control node (%s)\n",
			np->full_name);
	}

	/*
	 * parse display panel info
	 */
	if (np != node)  {
		np = of_find_node_by_name(node, "port");
		if (!np) {
			DRM_ERROR("fail : not find panel's port node (%s) !\n",
				node->full_name);
			return -EINVAL;
		}
	}

	parse_read_prop(np, "encoder-port", ctx->encoder_port);

	err = nx_drm_dp_parse_dt_panel_type(np, ddc, 0);
	if (0 > err)
		return err;

	/*
	 * parse display control config
	 */
	np = of_find_node_by_name(np, "dp_control");
	if (!np) {
		DRM_ERROR("fail : not find panel's control node (%s) !\n",
			node->full_name);
		return -EINVAL;
	}

	nx_drm_dp_parse_dt_dp_control(np, ddc);
	nx_drm_dp_dump_dp_control(ctx->dp_dev);

	return 0;
}

static int panel_lcd_setup(struct platform_device *pdev,
			struct lcd_context *ctx)
{
	struct device_node *np;
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct dp_control_dev *ddc = ctx_to_ddc(ctx);
	int err;

	nx_drm_dp_lcd_base_setup(pdev, &ctx->base, &ctx->reset);

	if (dp_panel_type_mipi == ddc->panel_type) {
		struct lcd_mipi_dsi *dsi = ctx_to_dsi(ctx);

		np = of_find_node_by_name(node, "dp_mipi");
		if (!np) {
			DRM_ERROR("fail : not found 'dp_mipi' node (%s)\n",
				node->full_name);
			return -EINVAL;
		}

		err = nx_drm_dp_lcd_device_setup(pdev,
				np, &dsi->r_base, ddc->panel_type);
		if (0 > err)
			return -EINVAL;
	}

	return 0;
}

static int panel_lcd_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct lcd_context *ctx;
	struct dp_control_dev *ddc;
	int err;

	DRM_DEBUG_KMS("enter (%s)\n", dev_name(dev));

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (IS_ERR(ctx))
		return -ENOMEM;

	ctx->dp_dev = &lcd_dp_dev;
	ctx->dp_dev->dev = dev;
	ddc = ctx_to_ddc(ctx);
	ddc->dev = dev;

	mutex_init(&ctx->lock);

	err = panel_lcd_parse_dt(pdev, ctx);
	if (0 > err)
		goto err_parse;

	err = panel_lcd_setup(pdev, ctx);
	if (0 > err)
		goto err_parse;

	if (IS_ENABLED(CONFIG_DRM_NX_MIPI_DSI)) {
		if (dp_panel_type_mipi == ddc->panel_type) {
			struct lcd_mipi_dsi *dsi = ctx_to_dsi(ctx);

			dsi->mipi_host.ops = &panel_mipi_ops;
			dsi->mipi_host.dev = dev;
		}
	}

	dev_set_drvdata(dev, ctx);
	component_add(dev, &panel_comp_ops);

	DRM_DEBUG_KMS("done\n");
	return err;

err_parse:
	if (ctx)
		devm_kfree(dev, ctx);

	return err;
}

static int panel_lcd_remove(struct platform_device *pdev)
{
	struct lcd_context *ctx = dev_get_drvdata(&pdev->dev);

	if (ctx) {
		struct dp_control_dev *ddc = ctx_to_ddc(ctx);

		if (ddc->dp_out_dev)
			devm_kfree(&pdev->dev, ddc->dp_out_dev);

		devm_kfree(&pdev->dev, ctx);
	}

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
	{.compatible = "nexell,s5p6818-drm-lcd"},
	{}
};
MODULE_DEVICE_TABLE(of, panel_lcd_of_match);

struct platform_driver panel_lcd_driver = {
	.probe = panel_lcd_probe,
	.remove = panel_lcd_remove,
	.driver = {
		.name = "nexell,display_drm_lcd",
		.owner = THIS_MODULE,
		.of_match_table = panel_lcd_of_match,
		.pm = &panel_lcd_pm,
	},
};
module_platform_driver(panel_lcd_driver);

MODULE_AUTHOR("jhkim <jhkim@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell LCD DRM Driver");
MODULE_LICENSE("GPL");
