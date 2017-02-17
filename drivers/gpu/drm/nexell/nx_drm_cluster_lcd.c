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
#include <video/display_timing.h>
#include <video/videomode.h>

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

struct dp_context {
	int crtc_pipe;
	unsigned int possible_crtcs_mask;
	struct nx_drm_device *display;
	struct mutex lock;
	bool local_timing;
	struct mipi_resource mipi_res;
};

struct cluster_context {
	struct drm_connector *connector;
	struct mutex lock;
	struct dp_context context[CLISTER_LCD_MAX];

	/* cluster's screen */
	struct videomode vm;
	u32 hactive, vactive;	/* "cluster-hactive", "cluster-vactive" */
	int width_mm;			/* "width-mm" */
	int height_mm;			/* "height-mm" */
	int ref_lcd;			/* "cluster-reference" */
	enum dp_cluster_dir cluster_dir;

	/* back light */
	struct gpio_descs *enable_gpios;
	enum of_gpio_flags gpios_active[4];
	int gpios_delay[4];
	struct backlight_device *backlight;
	int backlight_delay;
	bool enabled;
	struct work_struct lcd_power_work;
};

#define display_to_dpc(d)	(&d->ctrl.dpc)
#define ctx_to_mipi(c)	(struct mipi_resource *)(&c->mipi_res)
#define host_to_mipi(h)	container_of(h, struct mipi_resource, mipi_host)
#define mipi_to_ctx(d)	container_of(d, struct dp_context, mipi_res)

static bool panel_cluster_lcd_is_connected(struct device *dev,
			struct drm_connector *connector)
{
	struct dp_context *context;
	struct nx_drm_panel *panel;
	struct cluster_context *ctx = dev_get_drvdata(dev);
	enum dp_panel_type panel_type;
	int i;


	DRM_DEBUG_KMS("CLUSTER-LCD panel\n");

	for (i = 0; CLISTER_LCD_MAX > i; i++) {

		context = &ctx->context[i];
		panel = &context->display->panel;
		panel_type = dp_panel_get_type(context->display);

		DRM_DEBUG_KMS("cluster %s panel node %s\n",
			dp_panel_type_name(panel_type),
			panel->panel_node ? "exist" : "not exist");

		if (panel->panel_node) {
			struct drm_panel *drm_panel =
					of_drm_find_panel(panel->panel_node);

			if (drm_panel) {
				int ret;

				panel->panel = drm_panel;
				drm_panel_attach(drm_panel, connector);

				if (panel->check_panel)
					return panel->is_connected;

				nx_drm_dp_lcd_prepare(
					context->display, drm_panel);
				ret = drm_panel_prepare(drm_panel);
				if (!ret) {
					drm_panel_unprepare(drm_panel);
					nx_drm_dp_lcd_unprepare(
						context->display, drm_panel);
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
			return false;
		}

		if (!panel->panel_node && false == context->local_timing) {
			DRM_DEBUG_DRIVER("not exist %s panel & timing %s !\n",
				dp_panel_type_name(panel_type), dev_name(dev));
			return false;
		}

		/*
		 * support DT's timing node
		 * when not use panel driver
		 */
		panel->is_connected = true;
	}

	return true;
}

static int panel_cluster_lcd_get_modes(struct device *dev,
			struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct drm_device *drm = connector->dev;
	struct nx_drm_priv *priv = drm->dev_private;
	struct cluster_context *ctx = dev_get_drvdata(dev);
	struct videomode *vm = &ctx->vm;
	struct nx_drm_panel *panel;
	struct nx_drm_crtc *nx_crtc;
	struct nx_cluster_crtc *cluster;
	struct dp_context *context;
	int ref_sync = ctx->ref_lcd;
	int i, ret;

	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("fail : create a new display mode !\n");
		return 0;
	}

	/* get cluster's sync */
	for (i = 0; CLISTER_LCD_MAX > i; i++) {
		context = &ctx->context[i];
		panel = &context->display->panel;

		DRM_DEBUG_KMS("enter panel.%d %s\n",
			ctx->ref_lcd, panel->panel ? "attached" : "detached");

		if (panel->panel) {
			struct drm_panel *p = panel->panel;
			struct display_timing dt;

			if (p->funcs && p->funcs->get_timings) {
				ret = p->funcs->get_timings(p, 1, &dt);
				if (!ret) {
					DRM_ERROR("fail : display timing %s\n",
						dp_panel_type_name(
						dp_panel_get_type(
							context->display)));
					return 0;
				}
				videomode_from_timing(&dt, &panel->vm);
			}
		}
	}

	/*
	 * copy other sync info from reference context
	 */
	context = &ctx->context[ref_sync];
	panel = &context->display->panel;

	memcpy(vm, &panel->vm, sizeof(struct videomode));
   	vm->hactive = ctx->hactive;
   	vm->vactive = ctx->vactive;

	drm_display_mode_from_videomode(vm, mode);

	mode->width_mm = ctx->width_mm;
	mode->height_mm = ctx->height_mm;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	/*
	 * set cluster crtc resolution
	 */
	nx_crtc = to_nx_crtc(priv->crtcs[0]);
	cluster = nx_crtc->cluster;
	cluster->cluster_dir = ctx->cluster_dir;

	for (i = 0; CLISTER_LCD_MAX > i; i++) {
		context = &ctx->context[i];
		panel = &context->display->panel;

		memcpy(&cluster->vm[i], &panel->vm, sizeof(struct videomode));
	}

	DRM_DEBUG_KMS("exit\n");

	return 1;
}

static int panel_cluster_lcd_check_mode(struct device *dev,
			struct drm_display_mode *mode)
{
	struct cluster_context *ctx = dev_get_drvdata(dev);
	struct videomode *vm = &ctx->vm;

	drm_display_mode_to_videomode(mode, &ctx->vm);

	ctx->width_mm = mode->width_mm;
	ctx->height_mm = mode->height_mm;

	DRM_DEBUG_KMS("SYNC -> LCD %d x %d mm\n",
			ctx->width_mm, ctx->height_mm);
	DRM_DEBUG_KMS("ha:%d, hf:%d, hb:%d, hs:%d\n",
			vm->hactive, vm->hfront_porch,
			vm->hback_porch, vm->hsync_len);
	DRM_DEBUG_KMS("va:%d, vf:%d, vb:%d, vs:%d\n",
			vm->vactive, vm->vfront_porch,
			vm->vback_porch, vm->vsync_len);
	DRM_DEBUG_KMS("flags:0x%x\n", vm->flags);

	return MODE_OK;
}

static void panel_cluster_lcd_set_mode(struct device *dev,
			struct drm_display_mode *mode)
{
	struct cluster_context *ctx = dev_get_drvdata(dev);
	int i;

	for (i = 0; CLISTER_LCD_MAX > i; i++) {
		struct dp_context *context = &ctx->context[i];
		struct nx_drm_panel *panel = &context->display->panel;
		struct drm_display_mode mode;

		drm_display_mode_from_videomode(&panel->vm, &mode);
		nx_drm_dp_display_mode_to_sync(&mode, context->display);
	}
}

bool panel_cluster_lcd_mode_fixup(struct device *dev,
			struct drm_connector *connector,
			const struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void panel_cluster_lcd_commit(struct device *dev)
{
}

static void panel_cluster_lcd_work(struct work_struct *work)
{
	struct cluster_context *ctx = container_of(work,
					struct cluster_context, lcd_power_work);
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

static void panel_cluster_lcd_enable(struct device *dev)
{
	struct cluster_context *ctx = dev_get_drvdata(dev);
	struct dp_context *context;
	struct nx_drm_device *display;
	struct dp_control_dev *dpc;
	struct drm_panel *panel;
	bool suspended;
	int i;

	for (i = 0; CLISTER_LCD_MAX > i; i++) {
		context = &ctx->context[i];
		display = context->display;
		suspended = display->suspended;
		panel = (&display->panel)->panel;
		dpc = display_to_dpc(display);

		dpc->module = i;

		if (suspended)
			nx_drm_dp_panel_res_resume(dev, display);

		nx_drm_dp_lcd_prepare(display, panel);

		drm_panel_prepare(panel);
		drm_panel_enable(panel);

		nx_drm_dp_lcd_enable(display, panel);
	}
}

static void panel_cluster_lcd_disable(struct device *dev)
{
	struct cluster_context *ctx = dev_get_drvdata(dev);
	struct dp_context *context;
	struct nx_drm_device *display;
	struct dp_control_dev *dpc;
	struct drm_panel *panel;
	bool suspend;
	int i;

	for (i = 0; CLISTER_LCD_MAX > i; i++) {
		context = &ctx->context[i];
		display = context->display;
		suspend = display->suspended;
		panel = (&display->panel)->panel;
		dpc = display_to_dpc(display);

		dpc->module = i;

		if (suspend)
			nx_drm_dp_panel_res_suspend(dev, display);

		drm_panel_unprepare(panel);
		drm_panel_disable(panel);

		nx_drm_dp_lcd_unprepare(display, panel);
		nx_drm_dp_lcd_disable(display, panel);
	}
}

static void panel_cluster_lcd_dmps(struct device *dev, int mode)
{
	struct gpio_desc **desc;
	struct cluster_context *ctx = dev_get_drvdata(dev);
	int i;

	DRM_DEBUG_KMS("CLUSTER-LCD dpms.%d\n", mode);

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		if (ctx->enabled)
			break;

		panel_cluster_lcd_enable(dev);
		schedule_work(&ctx->lcd_power_work);
		ctx->enabled = true;
		break;

	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		if (!ctx->enabled)
			break;

		cancel_work_sync(&ctx->lcd_power_work);

		if (ctx->backlight) {
			ctx->backlight->props.power = FB_BLANK_POWERDOWN;
			backlight_update_status(ctx->backlight);
		}

		panel_cluster_lcd_disable(dev);

		if (ctx->enable_gpios) {
			desc = ctx->enable_gpios->desc;
			for (i = 0; ctx->enable_gpios->ndescs > i; i++)
				gpiod_set_value_cansleep(desc[i],
						ctx->gpios_active[i] ==
						GPIO_ACTIVE_HIGH ? 0 : 1);
		}
		ctx->enabled = false;
		break;
	default:
		DRM_ERROR("fail : unspecified mode %d\n", mode);
		break;
	}
}

static int panel_cluster_mipi_attach(struct mipi_dsi_host *host,
			struct mipi_dsi_device *device)
{
	struct mipi_resource *mipi = host_to_mipi(host);
	struct dp_context *context = mipi_to_ctx(mipi);
	struct nx_drm_panel *panel = &context->display->panel;

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

static int panel_cluster_mipi_detach(struct mipi_dsi_host *host,
			struct mipi_dsi_device *device)
{
	struct mipi_resource *mipi = host_to_mipi(host);
	struct dp_context *context = mipi_to_ctx(mipi);
	struct nx_drm_panel *panel = &context->display->panel;

	DRM_DEBUG_KMS("enter\n");

	panel->panel_node = NULL;
	return 0;
}

static ssize_t panel_cluster_mipi_transfer(struct mipi_dsi_host *host,
			const struct mipi_dsi_msg *msg)
{
	return nx_drm_dp_mipi_transfer(host, msg);
}

static struct mipi_dsi_host_ops panel_cluster_mipi_ops = {
	.attach = panel_cluster_mipi_attach,
	.detach = panel_cluster_mipi_detach,
	.transfer = panel_cluster_mipi_transfer,
};

static struct nx_drm_ops panel_cluster_lcd_ops = {
	.is_connected = panel_cluster_lcd_is_connected,
	.get_modes = panel_cluster_lcd_get_modes,
	.set_mode = panel_cluster_lcd_set_mode,
	.check_mode = panel_cluster_lcd_check_mode,
	.mode_fixup = panel_cluster_lcd_mode_fixup,
	.commit = panel_cluster_lcd_commit,
	.dpms = panel_cluster_lcd_dmps,
};

static int panel_cluster_lcd_bind(struct device *dev,
			struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct cluster_context *ctx = dev_get_drvdata(dev);
	struct platform_driver *pdrv = to_platform_driver(dev->driver);
	enum dp_panel_type panel_type = dp_panel_type_cluster_lcd;
	struct nx_drm_crtc *nx_crtc;
	struct dp_context *context;
	unsigned int possible_crtcs;
	int pipe;
	int i, err = 0;

	DRM_INFO("Bind CLUSTER-LCD panel\n");

	context = &ctx->context[0];
	pipe = context->crtc_pipe;
	possible_crtcs = context->possible_crtcs_mask;

	ctx->connector = nx_drm_connector_create_and_attach(drm,
					context->display, 0, possible_crtcs,
					panel_type, context);
	if (IS_ERR(ctx->connector))
		goto err_bind;

	for (i = 0; CLISTER_LCD_MAX > i; i++) {
		context = &ctx->context[i];
		panel_type = dp_panel_get_type(context->display);

		if (IS_ENABLED(CONFIG_DRM_NX_MIPI_DSI)) {
			if (dp_panel_type_mipi == panel_type) {
				struct mipi_resource *mipi =
						ctx_to_mipi(context);

				err = mipi_dsi_host_register(&mipi->mipi_host);
			}
		}
	}

	if (!err) {
		struct nx_drm_priv *priv = drm->dev_private;
		struct nx_cluster_crtc *cluster =
				devm_kzalloc(dev,
					sizeof(struct nx_cluster_crtc),
					GFP_KERNEL);
		if (IS_ERR(cluster))
			goto err_bind;

		if (panel_cluster_lcd_is_connected(dev, ctx->connector))
			priv->force_detect = true;

		nx_crtc = to_nx_crtc(priv->crtcs[0]);
		nx_crtc->cluster = cluster;

		DRM_INFO("DONE BIND\n");
		return 0;
	}

err_bind:
	if (pdrv->remove)
		pdrv->remove(to_platform_device(dev));

	return 0;
}

static void panel_cluster_lcd_unbind(struct device *dev,
			struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct nx_drm_priv *priv = drm->dev_private;
	struct cluster_context *ctx = dev_get_drvdata(dev);
	struct dp_context *context;
	struct nx_drm_crtc *nx_crtc;
	enum dp_panel_type panel_type;
	int i;

	if (ctx->connector)
		nx_drm_connector_destroy_and_detach(ctx->connector);

	for (i = 0; CLISTER_LCD_MAX > i; i++) {
		context = &ctx->context[i];
		panel_type = dp_panel_get_type(context->display);

		if (IS_ENABLED(CONFIG_DRM_NX_MIPI_DSI)) {
			if (dp_panel_type_mipi == panel_type) {
				struct mipi_resource *mipi =
						ctx_to_mipi(context);

				mipi_dsi_host_unregister(&mipi->mipi_host);
			}
		}
	}

	nx_crtc = to_nx_crtc(priv->crtcs[0]);
	if (nx_crtc->cluster) {
		devm_kfree(dev, ctx);
		nx_crtc->cluster = NULL;
	}
}

static const struct component_ops panel_comp_ops = {
	.bind = panel_cluster_lcd_bind,
	.unbind = panel_cluster_lcd_unbind,
};

#define parse_read_prop(n, s, v)	{ \
	u32 _v;	\
	if (!of_property_read_u32(n, s, &_v))	\
		v = _v;	\
	}

static const struct of_device_id panel_cluster_lcd_of_match[];

static int panel_cluster_lcd_parse_bl(struct platform_device *pdev,
			struct cluster_context *ctx)
{
	struct device *dev = &pdev->dev;
	struct device_node *node;
	struct gpio_descs *gpios;
	struct gpio_desc **desc = NULL;
#ifdef CONFIG_BACKLIGHT_CLASS_DEVICE
	struct device_node *backlight;
#endif
	int i, ngpios = 0;

	node = dev->of_node;

	/* parse panel gpios */
	gpios = devm_gpiod_get_array(dev, "enable", GPIOD_ASIS);
	if (-EBUSY == (long)ERR_CAST(gpios)) {
		DRM_INFO("fail : enable-gpios is busy : %s !!!\n",
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

		gpio = of_get_named_gpio_flags(node, "enable-gpios", i, &flags);
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
			parse_read_prop(node, "backlight-delay",
				ctx->backlight_delay);

			ctx->backlight->props.power =
				FB_BLANK_POWERDOWN;
			backlight_update_status(ctx->backlight);

			DRM_INFO("LCD backlight down, delay:%d\n",
				ctx->backlight_delay);
		}
	}
#endif

	INIT_WORK(&ctx->lcd_power_work, panel_cluster_lcd_work);

	return 0;
}

static int panel_cluster_lcd_parse_dt(struct platform_device *pdev,
			struct device_node *node, struct cluster_context *ctx,
			struct dp_context *context,
			enum dp_panel_type panel_type)
{
	struct device *dev = &pdev->dev;
	struct nx_drm_panel *panel = &context->display->panel;
	struct nx_drm_device *display = context->display;
	struct device_node *np;
	struct display_timing timing;
	int err;

	DRM_DEBUG_KMS("enter\n");

	/*
	 * get crtcs params
	 */
	parse_read_prop(node, "crtc-pipe", context->crtc_pipe);
	parse_read_prop(node, "crtcs-possible-mask",
		context->possible_crtcs_mask);

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
		/* parse panel context size */
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
			context->local_timing = true;
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
	nx_drm_dp_panel_ctrl_dump(context->display);

	return 0;
}

static int panel_cluster_lcd_driver_setup(struct platform_device *pdev,
			struct device_node *parent, struct device_node *node,
			struct cluster_context *ctx, struct dp_context *context,
			enum dp_panel_type *panel_type)
{
	const struct of_device_id *id;
	enum dp_panel_type type;
	struct device *dev = &pdev->dev;
	struct nx_drm_res *res = &context->display->res;
	u32 hactive, vactive;
	int err;

	/*
	 * get panel type with of id
	 */
	id = of_match_node(panel_cluster_lcd_of_match, parent);

	/* get cluster lcd node */
	if (!of_node_cmp(node->name, "rgb")) {
		node = of_find_node_by_name(NULL, "display_drm_rgb");
		type = dp_panel_type_rgb;
	} else if (!of_node_cmp(node->name, "lvds")) {
		node = of_find_node_by_name(NULL, "display_drm_lvds");
		type = dp_panel_type_lvds;
	} else if (!of_node_cmp(node->name, "mipi")) {
		node = of_find_node_by_name(NULL, "display_drm_mipi");
		type = dp_panel_type_mipi;
	} else {
		DRM_ERROR("fail : none cluster lcd nodes !\n");
		return -EINVAL;
	}

	if (of_property_read_u32(parent, "cluster-hactive", &hactive)) {
		DRM_ERROR("fail : none 'cluster-hactive' property !\n");
		return -EINVAL;
	}

	if (of_property_read_u32(parent, "cluster-vactive", &vactive)) {
		DRM_ERROR("fail : none 'cluster-vactive' property !\n");
		return -EINVAL;
	}

	ctx->hactive = hactive;
	ctx->vactive = vactive;
	parse_read_prop(parent, "cluster-reference", ctx->ref_lcd);
	parse_read_prop(parent, "cluster-direction", ctx->cluster_dir);

	if (ctx->cluster_dir > dp_cluster_clone ||
		0 > ctx->cluster_dir)
		ctx->cluster_dir = dp_cluster_hor;

	/* parse panel lcd size */
	parse_read_prop(parent, "width-mm", ctx->width_mm);
	parse_read_prop(parent, "height-mm", ctx->height_mm);

	/* change node */
	dev->of_node = node;
	*panel_type = type;

	DRM_INFO("Load CLUSTER-LCD panel (%s)\n",
			dp_panel_type_name(*panel_type));

	err = nx_drm_dp_panel_res_parse(dev, res, type);

	/* restore node */
	dev->of_node = parent;
	if (0 > err)
		return -EINVAL;

	return 0;
}

static int panel_cluster_lcd_probe(struct platform_device *pdev)
{
	struct cluster_context *ctx;
	struct dp_context *context;
	enum dp_panel_type panel_type;
	struct nx_drm_device *display;
	struct device *dev = &pdev->dev;
	struct dp_control_dev *dpc;
	size_t size;
	int i, err;

	struct device_node *parent = dev->of_node;
	struct device_node *node = NULL;

	DRM_DEBUG_KMS("enter (%s)\n", dev_name(dev));

	size = (sizeof(*ctx) + (sizeof(struct nx_drm_device)) * 2);
	ctx = devm_kzalloc(dev, size, GFP_KERNEL);
	if (IS_ERR(ctx))
		return -ENOMEM;

	display = (void *)ctx + sizeof(*ctx);

	for (i = 0; CLISTER_LCD_MAX > i; i++) {

		context = &ctx->context[i];
		context->display = display++;
		context->display->dev = dev;
		context->display->ops = &panel_cluster_lcd_ops;

		mutex_init(&context->lock);

		dpc = display_to_dpc(context->display);
		dpc->module = i;
		dpc->cluster = true;
		INIT_LIST_HEAD(&dpc->list);

		node = of_get_next_child(parent, node);
		err = panel_cluster_lcd_driver_setup(pdev,
				parent, node, ctx, context, &panel_type);
		if (0 > err)
				goto err_parse;

		err = panel_cluster_lcd_parse_dt(pdev, node,
						ctx, context, panel_type);
		if (0 > err)
			goto err_parse;

		if (IS_ENABLED(CONFIG_DRM_NX_MIPI_DSI)) {
			if (dp_panel_type_mipi == panel_type) {
				struct mipi_resource *mipi =
						ctx_to_mipi(context);

				mipi->mipi_host.ops = &panel_cluster_mipi_ops;
				mipi->mipi_host.dev = dev;
			}
		}
		nx_drm_dp_display_add_link(context->display);
	}
	panel_cluster_lcd_parse_bl(pdev, ctx);

	dev_set_drvdata(dev, ctx);
	component_add(dev, &panel_comp_ops);

	DRM_DEBUG_KMS("done\n");
	return err;

err_parse:
	if (ctx) {
		if (ctx->backlight)
			put_device(&ctx->backlight->dev);
		devm_kfree(dev, ctx);
	}

	return err;
}

static int panel_cluster_lcd_remove(struct platform_device *pdev)
{
	struct cluster_context *ctx = dev_get_drvdata(&pdev->dev);
	struct dp_context *context;
	struct device *dev = &pdev->dev;
	int i;

	for (i = 0; CLISTER_LCD_MAX > i; i++) {
		context = &ctx->context[i];
		if (!context)
			continue;

		nx_drm_dp_panel_res_free(dev, &context->display->res);
		nx_drm_dp_panel_dev_release(dev, context->display);
	}

	if (ctx->backlight)
		put_device(&ctx->backlight->dev);

	devm_kfree(dev, ctx);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int panel_cluster_lcd_suspend(struct device *dev)
{
	return 0;
}

static int panel_cluster_lcd_resume(struct device *dev)
{
	return 0;
}
#endif

static const struct dev_pm_ops panel_cluster_lcd_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(
		panel_cluster_lcd_suspend, panel_cluster_lcd_resume
	)
};

static const struct of_device_id panel_cluster_lcd_of_match[] = {
	{
		.compatible = "nexell,s5pxx18-drm-cluster-lcd",
		.data = (void *)dp_panel_type_cluster_lcd
	},
	{}
};
MODULE_DEVICE_TABLE(of, panel_cluster_lcd_of_match);

struct platform_driver panel_cluster_lcd_driver = {
	.probe = panel_cluster_lcd_probe,
	.remove = panel_cluster_lcd_remove,
	.driver = {
		.name = "nexell,display_drm_cluster_lcd",
		.owner = THIS_MODULE,
		.of_match_table = panel_cluster_lcd_of_match,
		.pm = &panel_cluster_lcd_pm,
	},
};

module_platform_driver(panel_cluster_lcd_driver);

MODULE_AUTHOR("jhkim <jhkim@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell LCD DRM Driver");
MODULE_LICENSE("GPL");
