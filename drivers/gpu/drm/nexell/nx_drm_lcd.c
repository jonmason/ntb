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
#include "nx_drm_fb.h"
#include "soc/s5pxx18_drm_dp.h"

struct mipi_resource {
	struct mipi_dsi_host mipi_host;
	struct mipi_dsi_device *mipi_dev;
	unsigned long flags;
	enum mipi_dsi_pixel_format format;
	unsigned int lanes;
};

struct lcd_context {
	struct drm_connector *connector;
	int crtc_pipe;
	struct reset_control *reset;
	void *base;
	struct nx_drm_device *display;
	struct mutex lock;
	bool local_timing;
	struct gpio_desc *enable_gpio;
	struct mipi_resource mipi_res;
};

#define ctx_to_mipi(c)	(struct mipi_resource *)(&c->mipi_res)
#define host_to_mipi(h)	container_of(h, struct mipi_resource, mipi_host)
#define mipi_to_ctx(d)	container_of(d, struct lcd_context, mipi_res)

static bool panel_lcd_is_connected(struct device *dev,
			struct drm_connector *connector)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_panel *panel = &ctx->display->panel;
	struct device_node *panel_node = panel->panel_node;
		enum dp_panel_type panel_type = dp_panel_get_type(ctx->display);

	DRM_DEBUG_KMS("%s panel node %s\n",
		dp_panel_type_name(panel_type), panel_node ?
		"exist" : "not exist");

	if (panel_node) {
		struct drm_panel *drm_panel = of_drm_find_panel(panel_node);

		if (drm_panel) {
			int ret;

			panel->panel = drm_panel;
			drm_panel_attach(drm_panel, connector);

			if (panel->check_panel)
				return panel->is_connected;

			nx_drm_dp_lcd_prepare(ctx->display, drm_panel);
			ret = drm_panel_prepare(drm_panel);
			if (!ret) {
				drm_panel_unprepare(drm_panel);
				nx_drm_dp_lcd_unprepare(ctx->display,
						drm_panel);
				panel->is_connected = true;
			} else {
				drm_panel_detach(drm_panel);
				panel->is_connected = false;
			}
			panel->check_panel = true;

			DRM_INFO("%s: check panel %s\n",
				dp_panel_type_name(panel_type),
				panel->is_connected ?
				"connected" : "disconnected");

			return panel->is_connected;
		}

		/*
		 * builded with module (.ko file).
		 */
		DRM_DEBUG_KMS("Not find panel driver for %s ...\n",
			dp_panel_type_name(panel_type));
		return true;
	}

	if (!panel_node && false == ctx->local_timing) {
		DRM_ERROR("not exist %s panel & timing %s !\n",
			dp_panel_type_name(panel_type), dev_name(dev));
		return false;
	}

	/*
	 * support DT's timing node
	 * when not use panel driver
	 */
	panel->is_connected = true;

	return true;
}

static int panel_lcd_get_modes(struct device *dev,
			struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_panel *panel = &ctx->display->panel;

	DRM_DEBUG_KMS("enter panel %s\n",
		panel->panel ? "attached" : "detached");

	if (panel->panel)
		return drm_panel_get_modes(panel->panel);

	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("fail : create a new display mode !\n");
		return 0;
	}

	drm_display_mode_from_videomode(&panel->vm, mode);
	mode->width_mm = panel->width_mm;
	mode->height_mm = panel->height_mm;
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
	struct nx_drm_panel *panel = &ctx->display->panel;

	drm_display_mode_to_videomode(mode, &panel->vm);

	panel->width_mm = mode->width_mm;
	panel->height_mm = mode->height_mm;

	DRM_DEBUG_KMS("SYNC -> LCD %d x %d mm\n",
		panel->width_mm, panel->height_mm);
	DRM_DEBUG_KMS("ha:%d, hf:%d, hb:%d, hs:%d\n",
		panel->vm.hactive, panel->vm.hfront_porch,
		panel->vm.hback_porch, panel->vm.hsync_len);
	DRM_DEBUG_KMS("va:%d, vf:%d, vb:%d, vs:%d\n",
		panel->vm.vactive, panel->vm.vfront_porch,
		panel->vm.vback_porch, panel->vm.vsync_len);
	DRM_DEBUG_KMS("flags:0x%x\n", panel->vm.flags);

	return MODE_OK;
}

bool panel_lcd_mode_fixup(struct device *dev,
			struct drm_connector *connector,
			const struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void panel_lcd_commit(struct device *dev)
{
}

static void panel_lcd_enable(struct device *dev, struct drm_panel *panel)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_device *display = ctx->display;

	nx_drm_dp_lcd_prepare(display, panel);
	drm_panel_prepare(panel);

	drm_panel_enable(panel);
	nx_drm_dp_lcd_enable(display, panel);
}

static void panel_lcd_disable(struct device *dev, struct drm_panel *panel)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_device *display = ctx->display;

	drm_panel_unprepare(panel);
	drm_panel_disable(panel);

	nx_drm_dp_lcd_unprepare(display, panel);
	nx_drm_dp_lcd_disable(display, panel);
}

static void panel_lcd_dmps(struct device *dev, int mode)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_panel *panel = &ctx->display->panel;
	struct drm_panel *drm_panel = panel->panel;

	DRM_DEBUG_KMS("dpms.%d\n", mode);
	switch (mode) {
	case DRM_MODE_DPMS_ON:
		panel_lcd_enable(dev, drm_panel);

		if (!panel && ctx->enable_gpio)
			gpiod_set_value_cansleep(ctx->enable_gpio, 1);
		break;

	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		panel_lcd_disable(dev, drm_panel);

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
	struct mipi_resource *mipi = host_to_mipi(host);
	struct lcd_context *ctx = mipi_to_ctx(mipi);
	struct nx_drm_panel *panel = &ctx->display->panel;

	mipi->lanes = device->lanes;
	mipi->format = device->format;
	mipi->flags = device->mode_flags;
	mipi->mipi_dev = device;

	/* set panel node */
	panel->panel_node = device->dev.of_node;

	DRM_INFO("mipi: %s lanes:%d, format:%d, flags:%lx\n",
		dev_name(&device->dev), device->lanes,
		device->format, device->mode_flags);

	return 0;
}

static int panel_mipi_detach(struct mipi_dsi_host *host,
			struct mipi_dsi_device *device)
{
	struct mipi_resource *mipi = host_to_mipi(host);
	struct lcd_context *ctx = mipi_to_ctx(mipi);
	struct nx_drm_panel *panel = &ctx->display->panel;

	DRM_DEBUG_KMS("enter\n");

	panel->panel_node = NULL;

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

static struct nx_drm_ops panel_lcd_ops = {
	.is_connected = panel_lcd_is_connected,
	.get_modes = panel_lcd_get_modes,
	.check_mode = panel_lcd_check_mode,
	.mode_fixup = panel_lcd_mode_fixup,
	.commit = panel_lcd_commit,
	.dpms = panel_lcd_dmps,
};

static int panel_lcd_bind(struct device *dev,
			struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct platform_driver *pdrv = to_platform_driver(dev->driver);
	enum dp_panel_type panel_type = dp_panel_get_type(ctx->display);
	int pipe = ctx->crtc_pipe;
	int err = 0;

	DRM_INFO("Bind %s panel\n", dp_panel_type_name(panel_type));

	ctx->connector = nx_drm_connector_create_and_attach(drm,
			ctx->display, pipe, panel_type, ctx);
	if (IS_ERR(ctx->connector))
		goto err_bind;

	if (IS_ENABLED(CONFIG_DRM_NX_MIPI_DSI)) {
		if (dp_panel_type_mipi == panel_type) {
			struct mipi_resource *mipi = ctx_to_mipi(ctx);

			err = mipi_dsi_host_register(&mipi->mipi_host);
		}
	}

	if (!err) {
		struct nx_drm_priv *priv = drm->dev_private;

		if (panel_lcd_is_connected(dev, ctx->connector))
			priv->force_detect = true;

		return 0;
	}

err_bind:
	if (pdrv->remove)
		pdrv->remove(to_platform_device(dev));

	return 0;
}

static void panel_lcd_unbind(struct device *dev,
			struct device *master, void *data)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	enum dp_panel_type panel_type = dp_panel_get_type(ctx->display);

	if (ctx->connector)
		nx_drm_connector_destroy_and_detach(ctx->connector);

	if (IS_ENABLED(CONFIG_DRM_NX_MIPI_DSI)) {
		if (dp_panel_type_mipi == panel_type) {
			struct mipi_resource *mipi = ctx_to_mipi(ctx);

			mipi_dsi_host_unregister(&mipi->mipi_host);
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

static const struct of_device_id panel_lcd_of_match[];

static int panel_lcd_parse_dt(struct platform_device *pdev,
			struct lcd_context *ctx, enum dp_panel_type panel_type)
{
	struct device *dev = &pdev->dev;
	struct nx_drm_panel *panel = &ctx->display->panel;
	struct nx_drm_device *display = ctx->display;
	struct device_node *node = dev->of_node;
	struct device_node *np;
	struct display_timing timing;
	int err;

	DRM_DEBUG_KMS("enter\n");

	parse_read_prop(node, "crtc-pipe", ctx->crtc_pipe);

	/*
	 * parse panel output for RGB/LVDS/MiPi-DSI
	 */
	err = nx_drm_dp_panel_dev_register(dev, node, panel_type, display);
	if (0 > err)
		return err;

	/*
	 * get panel timing from local.
	 */
	np = of_graph_get_remote_port_parent(node);
	panel->panel_node = np;
	if (!np) {
		DRM_INFO("not use remote panel node (%s) !\n",
			node->full_name);

		/*
		 * parse panel info
		 */
		ctx->enable_gpio =
			devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);

		parse_read_prop(node, "width-mm", panel->width_mm);
		parse_read_prop(node, "height-mm", panel->height_mm);

		/*
		 * parse display timing (sync)
		 * refer to "drivers/video/of_display_timing.c"
		 * -> of_parse_display_timing
		 */
		err = of_get_display_timing(node, "display-timing", &timing);
		if (0 == err) {
			videomode_from_timing(&timing, &panel->vm);
			ctx->local_timing = true;
		}
	}

	/*
	 * parse display control config
	 */
	np = of_find_node_by_name(node, "dp_control");
	if (!np) {
		DRM_ERROR("fail : not find panel's control node (%s) !\n",
			node->full_name);
		return -EINVAL;
	}

	nx_drm_dp_panel_ctrl_parse(np, display);
	nx_drm_dp_panel_ctrl_dump(ctx->display);

	return 0;
}

static int panel_lcd_driver_setup(struct platform_device *pdev,
			struct lcd_context *ctx, enum dp_panel_type *panel_type)
{
	const struct of_device_id *id;
	enum dp_panel_type type;
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct nx_drm_res *res = &ctx->display->res;
	int err;

	/*
	 * get panel type with of id
	 */
	id = of_match_node(panel_lcd_of_match, pdev->dev.of_node);
	type = (enum dp_panel_type)id->data;

	DRM_INFO("Load %s panel\n", dp_panel_type_name(type));

	err = nx_drm_dp_panel_drv_res_parse(dev, &ctx->base, &ctx->reset);
	if (0 > err)
		return -EINVAL;

	err = nx_drm_dp_panel_dev_res_parse(dev, node, res, type);
	if (0 > err)
		return -EINVAL;

	*panel_type = type;

	return 0;
}

static int panel_lcd_probe(struct platform_device *pdev)
{
	struct lcd_context *ctx;
	enum dp_panel_type panel_type;
	struct device *dev = &pdev->dev;
	size_t size;
	int err;

	DRM_DEBUG_KMS("enter (%s)\n", dev_name(dev));

	size = sizeof(*ctx) + sizeof(struct nx_drm_device);
	ctx = devm_kzalloc(dev, size, GFP_KERNEL);
	if (IS_ERR(ctx))
		return -ENOMEM;

	ctx->display = (void *)ctx + sizeof(*ctx);
	ctx->display->dev = dev;
	ctx->display->ops = &panel_lcd_ops;

	mutex_init(&ctx->lock);

	err = panel_lcd_driver_setup(pdev, ctx, &panel_type);
	if (0 > err)
		goto err_parse;

	err = panel_lcd_parse_dt(pdev, ctx, panel_type);
	if (0 > err)
		goto err_parse;

	panel_type = dp_panel_get_type(ctx->display);

	if (IS_ENABLED(CONFIG_DRM_NX_MIPI_DSI)) {
		if (dp_panel_type_mipi == panel_type) {
			struct mipi_resource *mipi = ctx_to_mipi(ctx);

			mipi->mipi_host.ops = &panel_mipi_ops;
			mipi->mipi_host.dev = dev;
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
	struct device *dev = &pdev->dev;

	if (!ctx)
		return 0;

	nx_drm_dp_panel_dev_res_free(dev, &ctx->display->res);
	nx_drm_dp_panel_drv_res_free(dev, ctx->base, ctx->reset);
	nx_drm_dp_panel_dev_release(dev, ctx->display);

	devm_kfree(&pdev->dev, ctx);

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
		.data = (void *)dp_panel_type_rgb
	},
#endif
#ifdef CONFIG_DRM_NX_LVDS
	{
		.compatible = "nexell,s5pxx18-drm-lvds",
		.data = (void *)dp_panel_type_lvds
	},
#endif
#ifdef CONFIG_DRM_NX_MIPI_DSI
	{
		.compatible = "nexell,s5pxx18-drm-mipi",
		.data = (void *)dp_panel_type_mipi
	},
#endif
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
