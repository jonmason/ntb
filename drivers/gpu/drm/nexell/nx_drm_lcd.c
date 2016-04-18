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

#include <linux/component.h>
#include <linux/of_platform.h>
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

struct lcd_context {
	struct drm_connector *connector;
	struct drm_encoder *encoder;
	struct nx_drm_dp_dev *dp_dev;
	struct gpio_desc *enable_gpio;
	int encoder_port;
	int panel_mpu_lcd;
	struct mutex lock;
};

#define lcd_connector(c)	\
		container_of(c, struct lcd_context, connector)

static bool panel_lcd_is_connected(struct device *dev,
			struct drm_connector *connector)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_panel *dpp = &ctx->dp_dev->dpp;
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
	struct nx_drm_dp_dev *dp_dev = ctx->dp_dev;
	struct nx_drm_panel *dpp = &dp_dev->dpp;

	DRM_DEBUG_KMS("enter panel %s\n",
		dpp->panel ? "attached" : "detached");

	if (dpp->panel)
		return drm_panel_get_modes(dpp->panel);

	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("fail : create a new display mode !!!\n");
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
	struct nx_drm_dp_dev *dp_dev = ctx->dp_dev;
	struct nx_drm_panel *dpp = &dp_dev->dpp;

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
	struct nx_drm_dp_dev *dp_dev = ctx->dp_dev;

	dp_dev->ddi.panel_mpu_lcd =
		ctx->panel_mpu_lcd ? true : false;

	DRM_DEBUG_KMS("enter\n");
	nx_soc_dp_device_top_mux(&dp_dev->ddi);
}

static void panel_lcd_dmps(struct device *dev, int mode)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_dp_dev *dp_dev = ctx->dp_dev;
	struct drm_panel *panel = dp_dev->dpp.panel;

	DRM_DEBUG_KMS("dpms.%d\n", mode);
	switch (mode) {
	case DRM_MODE_DPMS_STANDBY:
		drm_panel_prepare(panel);
		break;
	case DRM_MODE_DPMS_SUSPEND:
		drm_panel_unprepare(panel);
		break;
	case DRM_MODE_DPMS_ON:
		drm_panel_prepare(panel);
		drm_panel_enable(panel);
		if (!panel && ctx->enable_gpio)
			gpiod_set_value_cansleep(ctx->enable_gpio, 1);
		break;
	case DRM_MODE_DPMS_OFF:
		drm_panel_disable(panel);
		drm_panel_unprepare(panel);
		if (!panel && ctx->enable_gpio)
			gpiod_set_value_cansleep(ctx->enable_gpio, 0);
		break;
	default:
		DRM_ERROR("fail : unspecified mode %d\n", mode);
		break;
	}
}

static struct nx_drm_dev_ops panel_lcd_ops = {
	.is_connected = panel_lcd_is_connected,
	.get_modes = panel_lcd_get_modes,
	.check_mode = panel_lcd_check_mode,
	.commit = panel_lcd_commit,
	.dmps = panel_lcd_dmps,
};

static struct nx_drm_dp_dev lcd_display = {
	.ops = &panel_lcd_ops,
};

static int panel_lcd_bind(struct device *dev,
			struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct lcd_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_dp_dev *dp_dev = ctx->dp_dev;
	int to_enc = ctx->encoder_port;
	int ret;

	ret = nx_drm_connector_create_and_attach(drm, dp_dev, to_enc, ctx);
	if (0 > ret) {
		DRM_ERROR("fail : create for LCD connector !!!\n");
		return -EFAULT;
	}
	return 0;
}

static void panel_lcd_unbind(struct device *dev,
			struct device *master, void *data)
{
	struct lcd_context *ctx = dev_get_drvdata(dev);

	nx_drm_connector_destroy_and_detach(ctx->connector);
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
	struct nx_drm_dp_dev *dp_dev = ctx->dp_dev;
	struct nx_drm_panel *dpp = &dp_dev->dpp;
	struct device_node *node = dev->of_node;
	struct device_node *np, *tmp;
	struct display_timing timing;
	int ret;

	np = of_graph_get_remote_port_parent(node);
	if (!np) {
		DRM_ERROR("fail : not find panel node (%s:%s) !!!\n",
			node->name, node->full_name);
		return -EINVAL;
	}

	dpp->panel_node = np;

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
	ret = of_get_display_timing(np, "panel-timing", &timing);
	if (0 == ret)
		videomode_from_timing(&timing, &dpp->vm);

	/*
	 * if not exist dp-control at remote endpoint,
	 * get from lcd_panel node.
	 */
	tmp = of_find_node_by_name(np, "dp-control");
	if (!tmp)
		np = node;

	/*
	 * parse display panel info
	 */
	np = of_find_node_by_name(node, "port");
	if (!np) {
		DRM_ERROR("fail : not find panel's port node (%s:%s) !!!\n",
			node->name, node->full_name);
		return -EINVAL;
	}

	parse_read_prop(np, "encoder-port", ctx->encoder_port);
	parse_read_prop(np, "panel-mpu", ctx->panel_mpu_lcd);

	ret = nx_drm_dp_parse_dt_panel(np, &dp_dev->ddi);
	if (0 > ret)
		return ret;

	/*
	 * parse display control config
	 */
	np = of_find_node_by_name(np, "dp-control");
	if (!np) {
		DRM_ERROR("fail : not find panel's control node (%s:%s) !!!\n",
			node->name, node->full_name);
		return -EINVAL;
	}

	nx_drm_dp_parse_dt_ctrl(np, &dp_dev->ddi);
	nx_drm_dp_parse_dp_dump(dp_dev);

	return 0;
}

static int panel_lcd_probe(struct platform_device *pdev)
{
	struct lcd_context *ctx;
	int ret;

	DRM_DEBUG_KMS("enter\n");

	ctx = devm_kzalloc(&pdev->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->dp_dev = &lcd_display;
	ctx->dp_dev->dev = &pdev->dev;
	mutex_init(&ctx->lock);

	ret = panel_lcd_parse_dt(pdev, ctx);
	if (0 > ret)
		goto err_parse;

	dev_set_drvdata(&pdev->dev, ctx);
	component_add(&pdev->dev, &panel_comp_ops);

	DRM_DEBUG_KMS("done\n");
	return ret;

err_parse:
	if (ctx)
		devm_kfree(&pdev->dev, ctx);

	return ret;
}

static int panel_lcd_remove(struct platform_device *pdev)
{
	struct lcd_context *ctx = dev_get_drvdata(&pdev->dev);

	if (ctx)
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
	{.compatible = "nexell,s5p6818-lcd-panel"},
	{}
};
MODULE_DEVICE_TABLE(of, panel_lcd_of_match);

struct platform_driver panel_lcd_driver = {
	.probe = panel_lcd_probe,
	.remove = panel_lcd_remove,
	.driver = {
		.name = "nexell,lcd_panel",
		.owner = THIS_MODULE,
		.of_match_table = panel_lcd_of_match,
		.pm = &panel_lcd_pm,
	},
};
module_platform_driver(panel_lcd_driver);

MODULE_AUTHOR("jhkim <jhkim@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell LCD DRM Driver");
MODULE_LICENSE("GPL");
