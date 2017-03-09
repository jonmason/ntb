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

struct display_context {
	struct nx_drm_display *display;
	struct list_head list;
	bool local_timing;
};

struct cluster_context {
	struct nx_drm_connector *connector;
	struct display_context context[CLUSTER_LCD_MAX];
	struct list_head cluster_list;
	/* cluster lcd parameters */
	int crtc_pipe;
	unsigned int possible_crtcs_mask;
	/* cluster's screen */
	struct videomode vm;
	u32 hactive, vactive;	/* "cluster-hactive", "cluster-vactive" */
	int width_mm;			/* "width-mm" */
	int height_mm;			/* "height-mm" */
	int ref_lcd;			/* "cluster-reference" */
	enum nx_cluster_dir direction;
	/* back light */
	struct gpio_descs *enable_gpios;
	enum of_gpio_flags gpios_active[4];
	int gpios_delay[4];
	struct backlight_device *backlight;
	int backlight_delay;
	bool enabled;
	struct work_struct lcd_power_work;
};

static bool panel_cluster_ops_detect(struct device *dev,
			struct drm_connector *connector)
{
	struct cluster_context *cluster = dev_get_drvdata(dev);
	struct display_context *ctx;

	DRM_DEBUG_KMS("CLUSTER-LCD panel\n");

	list_for_each_entry(ctx, &cluster->cluster_list, list) {
		struct nx_drm_display *display = ctx->display;
		struct nx_drm_display_ops *ops = display->ops;

		DRM_DEBUG_KMS("cluster %s panel node %s\n",
			nx_panel_get_name(display->panel_type),
			display->panel_node ? "exist" : "not exist");

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
					nx_panel_get_name(display->panel_type),
					display->is_connected ?
					"connected" : "disconnected");

				return display->is_connected;
			}

			/*
			 * builded with module (.ko file).
			 */
			DRM_DEBUG_KMS("Not find panel driver for %s ...\n",
				nx_panel_get_name(display->panel_type));
			return false;
		}

		if (!display->panel_node && !ctx->local_timing) {
			DRM_DEBUG_DRIVER("not exist %s panel & timing %s !\n",
				nx_panel_get_name(display->panel_type),
				dev_name(dev));
			return false;
		}

		/*
		 * support DT's timing node
		 * when not use panel driver
		 */
		display->is_connected = true;
	}

	return true;
}

static bool panel_cluster_ops_is_connected(struct device *dev)
{
	struct cluster_context *cluster = dev_get_drvdata(dev);
	struct display_context *ctx;

	list_for_each_entry(ctx, &cluster->cluster_list, list) {
		if (!ctx->display->is_connected)
			return false;
	}
	return true;
}

static int panel_cluster_ops_get_modes(struct device *dev,
			struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct drm_device *drm = connector->dev;
	struct nx_drm_private *private = drm->dev_private;
	struct cluster_context *cluster = dev_get_drvdata(dev);
	struct nx_drm_display *display;
	struct videomode *vm = &cluster->vm;
	struct nx_drm_crtc *nx_crtc;
	struct nx_drm_cluster *cluster_crtc;
	struct display_context *ctx;
	int i = 0, ret;

	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("Failed to create a new display mode !\n");
		return 0;
	}

	/* get cluster's sync */
	list_for_each_entry(ctx, &cluster->cluster_list, list) {
		display = ctx->display;
		if (display->panel) {
			struct drm_panel *p = display->panel;
			struct display_timing dt;

			if (p->funcs && p->funcs->get_timings) {
				ret = p->funcs->get_timings(p, 1, &dt);
				if (!ret) {
					DRM_ERROR("Failed, display timing %s\n",
						nx_panel_get_name(
						nx_panel_get_type(display)));
					return 0;
				}
				videomode_from_timing(&dt, &display->vm);
			}
		}
	}

	/* copy other sync info from reference context */
	ctx = &cluster->context[cluster->ref_lcd];
	display = ctx->display;

	memcpy(vm, &display->vm, sizeof(struct videomode));
	vm->hactive = cluster->hactive;
	vm->vactive = cluster->vactive;

	drm_display_mode_from_videomode(vm, mode);

	mode->width_mm = cluster->width_mm;
	mode->height_mm = cluster->height_mm;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	/* set cluster crtc resolution */
	nx_crtc = to_nx_crtc(private->crtcs[cluster->crtc_pipe]);
	cluster_crtc = nx_crtc->cluster;
	cluster_crtc->direction = cluster->direction;

	list_for_each_entry(ctx, &cluster->cluster_list, list) {
		display = ctx->display;
		memcpy(&cluster_crtc->vm[i++], &display->vm, sizeof(*vm));
	}

	DRM_DEBUG_KMS("exit\n");

	return 1;
}

static int panel_cluster_ops_valid_mode(struct device *dev,
			struct drm_display_mode *mode)
{
	struct cluster_context *cluster = dev_get_drvdata(dev);
	struct videomode *vm = &cluster->vm;

	drm_display_mode_to_videomode(mode, &cluster->vm);

	cluster->width_mm = mode->width_mm;
	cluster->height_mm = mode->height_mm;

	DRM_DEBUG_KMS("SYNC -> LCD %d x %d mm\n",
			cluster->width_mm, cluster->height_mm);
	DRM_DEBUG_KMS("ha:%d, hf:%d, hb:%d, hs:%d\n",
			vm->hactive, vm->hfront_porch,
			vm->hback_porch, vm->hsync_len);
	DRM_DEBUG_KMS("va:%d, vf:%d, vb:%d, vs:%d\n",
			vm->vactive, vm->vfront_porch,
			vm->vback_porch, vm->vsync_len);
	DRM_DEBUG_KMS("flags:0x%x\n", vm->flags);

	return MODE_OK;
}

static void panel_cluster_ops_set_mode(struct device *dev,
			struct drm_display_mode *mode)
{
	struct cluster_context *cluster = dev_get_drvdata(dev);
	struct display_context *ctx;

	list_for_each_entry(ctx, &cluster->cluster_list, list) {
		struct nx_drm_display *display = ctx->display;
		struct nx_drm_display_ops *ops = display->ops;
		struct drm_display_mode mode;

		DRM_DEBUG_KMS("enter %s\n",
			nx_panel_get_name(nx_panel_get_type(display)));

		drm_display_mode_from_videomode(&display->vm, &mode);

		if (ops->set_mode)
			ops->set_mode(display, &mode, 0);
	}
}

static void panel_cluster_work(struct work_struct *work)
{
	struct cluster_context *cluster = container_of(work,
					struct cluster_context, lcd_power_work);
	struct gpio_desc **desc;
	int i;

	if (cluster->enable_gpios) {
		desc = cluster->enable_gpios->desc;
		for (i = 0; cluster->enable_gpios->ndescs > i; i++) {
			DRM_DEBUG_KMS("LCD gpio.%d ative %s %dms\n",
				desc_to_gpio(desc[i]),
				cluster->gpios_active[i] == GPIO_ACTIVE_HIGH
				? "high" : "low ", cluster->gpios_delay[i]);

			gpiod_set_value_cansleep(desc[i],
				cluster->gpios_active[i] == GPIO_ACTIVE_HIGH ?
								1 : 0);
			if (cluster->gpios_delay[i])
				mdelay(cluster->gpios_delay[i]);
		}
	}

	if (cluster->backlight) {
		if (cluster->backlight_delay)
			mdelay(cluster->backlight_delay);

		cluster->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(cluster->backlight);
	}
}

static void panel_cluster_on(struct cluster_context *cluster)
{
	struct nx_drm_connector *nx_connector = cluster->connector;
	struct display_context *ctx;

	list_for_each_entry(ctx, &cluster->cluster_list, list) {
		struct nx_drm_display *display = ctx->display;
		struct nx_drm_display_ops *ops = display->ops;

		DRM_DEBUG_KMS("enter %s\n",
			nx_panel_get_name(nx_panel_get_type(display)));

		if (nx_connector->suspended) {
			if (ops->resume)
				ops->resume(display);
		}

		if (ops->prepare)
			ops->prepare(display);

		drm_panel_prepare(display->panel);
		drm_panel_enable(display->panel);

		if (ops->enable)
			ops->enable(display);
	}
}

static void panel_cluster_off(struct cluster_context *cluster)
{
	struct nx_drm_connector *nx_connector = cluster->connector;
	struct display_context *ctx;

	list_for_each_entry(ctx, &cluster->cluster_list, list) {
		struct nx_drm_display *display = ctx->display;
		struct nx_drm_display_ops *ops = display->ops;

		DRM_DEBUG_KMS("enter %s\n",
			nx_panel_get_name(nx_panel_get_type(display)));

		if (nx_connector->suspended) {
			if (ops->suspend)
				ops->suspend(display);
		}

		drm_panel_unprepare(display->panel);
		drm_panel_disable(display->panel);

		if (ops->unprepare)
			ops->unprepare(display);

		if (ops->disable)
			ops->disable(display);
	}
}

static void panel_cluster_ops_enable(struct device *dev)
{
	struct cluster_context *cluster = dev_get_drvdata(dev);

	DRM_DEBUG_KMS("enter\n");

	if (cluster->enabled)
		return;

	panel_cluster_on(cluster);
	schedule_work(&cluster->lcd_power_work);
	cluster->enabled = true;
}

static void panel_cluster_ops_disable(struct device *dev)
{
	struct cluster_context *cluster = dev_get_drvdata(dev);
	struct gpio_desc **desc;
	int i;

	DRM_DEBUG_KMS("enter\n");

	if (!cluster->enabled)
		return;

	cancel_work_sync(&cluster->lcd_power_work);

	if (cluster->backlight) {
		cluster->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(cluster->backlight);
	}

	panel_cluster_off(cluster);

	if (cluster->enable_gpios) {
		desc = cluster->enable_gpios->desc;
		for (i = 0; cluster->enable_gpios->ndescs > i; i++)
			gpiod_set_value_cansleep(desc[i],
						cluster->gpios_active[i] ==
						GPIO_ACTIVE_HIGH ? 0 : 1);
	}
	cluster->enabled = false;
}

static struct nx_drm_connect_drv_ops cluster_connector_ops = {
	.detect = panel_cluster_ops_detect,
	.is_connected = panel_cluster_ops_is_connected,
	.get_modes = panel_cluster_ops_get_modes,
	.valid_mode = panel_cluster_ops_valid_mode,
	.set_mode = panel_cluster_ops_set_mode,
	.enable = panel_cluster_ops_enable,
	.disable = panel_cluster_ops_disable,
};

static struct nx_drm_connector cluster_connector_dev = {
	.ops = &cluster_connector_ops,
};

static int panel_cluster_bind(struct device *dev,
			struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct nx_drm_private *private = drm->dev_private;
	struct cluster_context *cluster = dev_get_drvdata(dev);
	struct drm_connector *connector = &cluster->connector->connector;
	struct nx_drm_crtc *nx_crtc;
	struct nx_drm_cluster *cluster_crtc;
	struct platform_driver *drv;
	int err = 0;

	DRM_INFO("Bind CLUSTER-LCD\n");

	err = nx_drm_connector_attach(drm, connector,
			cluster->crtc_pipe, cluster->possible_crtcs_mask,
			NX_PANEL_TYPE_CLUSTER_LCD);
	if (err < 0)
		goto err_bind;

	cluster_crtc = devm_kzalloc(dev, sizeof(*cluster_crtc), GFP_KERNEL);
	if (!cluster_crtc)
		goto err_bind;

	if (panel_cluster_ops_detect(dev, connector))
		private->force_detect = true;

	nx_crtc = to_nx_crtc(private->crtcs[cluster->crtc_pipe]);
	nx_crtc->cluster = cluster_crtc;

	DRM_INFO("DONE BIND\n");
	return 0;

err_bind:
	drv = to_platform_driver(dev->driver);
	if (drv->remove)
		drv->remove(to_platform_device(dev));

	return 0;
}

static void panel_cluster_unbind(struct device *dev,
			struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct nx_drm_private *private = drm->dev_private;
	struct cluster_context *cluster = dev_get_drvdata(dev);
	struct drm_connector *connector = &cluster->connector->connector;
	struct nx_drm_crtc *nx_crtc;

	if (connector)
		nx_drm_connector_detach(connector);

	nx_crtc = to_nx_crtc(private->crtcs[cluster->crtc_pipe]);
	devm_kfree(dev, nx_crtc->cluster);
	nx_crtc->cluster = NULL;
}

static const struct component_ops panel_comp_ops = {
	.bind = panel_cluster_bind,
	.unbind = panel_cluster_unbind,
};

#define property_read(n, s, v)	of_property_read_u32(n, s, &v)
static const struct of_device_id panel_cluster_of_match[];

static int panel_cluster_set_backlight(struct platform_device *pdev,
			struct cluster_context *cluster)
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
		DRM_INFO("Failed, enable-gpios is busy : %s !!!\n",
			node->full_name);
		gpios = NULL;
	}

	if (!IS_ERR(gpios) && gpios) {
		ngpios = gpios->ndescs;
		desc = gpios->desc;
		cluster->enable_gpios = gpios;	/* set enable_gpios */
		of_property_read_u32_array(node,
			"enable-gpios-delay", cluster->gpios_delay,
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
		cluster->gpios_active[i] = flags;

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
		cluster->backlight = of_find_backlight_by_node(backlight);
		of_node_put(backlight);
		if (cluster->backlight) {
			property_read(node, "backlight-delay",
				cluster->backlight_delay);

			cluster->backlight->props.power =
				FB_BLANK_POWERDOWN;
			backlight_update_status(cluster->backlight);

			DRM_INFO("LCD backlight down, delay:%d\n",
				cluster->backlight_delay);
		}
	}
#endif

	INIT_WORK(&cluster->lcd_power_work, panel_cluster_work);
	return 0;
}

static int panel_cluster_parse_dt(struct platform_device *pdev,
				struct cluster_context *cluster)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	DRM_DEBUG_KMS("node:%s\n", node->name);

	/* get crtcs params */
	property_read(node, "crtc-pipe", cluster->crtc_pipe);
	property_read(node, "crtcs-possible-mask",
		cluster->possible_crtcs_mask);

	if (of_property_read_u32(node, "cluster-hactive",
			&cluster->hactive)) {
		DRM_ERROR("Failed none 'cluster-hactive' property !\n");
		return -EINVAL;
	}

	if (of_property_read_u32(node, "cluster-vactive",
			&cluster->vactive)) {
		DRM_ERROR("Failed none 'cluster-vactive' property !\n");
		return -EINVAL;
	}

	property_read(node, "cluster-reference", cluster->ref_lcd);
	property_read(node, "cluster-direction", cluster->direction);

	if (cluster->direction > DP_CLUSTER_CLONE ||
		cluster->direction < 0)
		cluster->direction = DP_CLUSTER_HOR;

	/* parse panel lcd size */
	property_read(node, "width-mm", cluster->width_mm);
	property_read(node, "height-mm", cluster->height_mm);

	return 0;
}

static int panel_parse_display(struct device_node *node,
			struct display_context *ctx)
{
	struct nx_drm_display *display = ctx->display;
	struct device_node *np;
	struct display_timing timing;
	int err;

	DRM_DEBUG_KMS("enter\n");

	/* get panel timing from local. */
	np = of_graph_get_remote_port_parent(node);
	display->panel_node = np;
	if (!np) {
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

	/* parse control config */
	np = of_find_node_by_name(node, "dp_control");
	of_node_get(node);
	if (!np) {
		DRM_ERROR("Failed not find panel's control node (%s) !\n",
			node->full_name);
		return -EINVAL;
	}
	of_node_put(np);

	return nx_drm_display_setup(display, np, NX_PANEL_TYPE_CLUSTER_LCD);
}

static int panel_cluster_get_displays(struct platform_device *pdev,
			struct cluster_context *cluster)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct nx_drm_connector *nx_connector = cluster->connector;
	struct drm_connector *connector = &nx_connector->connector;
	struct device_node *child = NULL, *np = NULL;
	struct display_context *ctx;
	enum nx_panel_type type;
	int err;

	list_for_each_entry(ctx, &cluster->cluster_list, list) {

		/* get cluster lcd type */
		child = of_get_next_child(node, child);
		if (!of_node_cmp(child->name, "rgb")) {
			np = of_find_node_by_name(NULL, "display_drm_rgb");
			type = NX_PANEL_TYPE_RGB;
		} else if (!of_node_cmp(child->name, "lvds")) {
			np = of_find_node_by_name(NULL, "display_drm_lvds");
			type = NX_PANEL_TYPE_LVDS;
		} else {
			DRM_ERROR("Failed none cluster lcd nodes !\n");
			return -EINVAL;
		}
		of_node_get(node);
		of_node_put(child);

		/* change node */
		dev->of_node = np;

		/* get display with type */
		ctx->display = nx_drm_display_get(dev, np, connector, type);
		if (!ctx->display)
			return -ENOENT;

		err = panel_parse_display(child, ctx);
		if (err < 0)
			return err;

		/* restore node */
		dev->of_node = node;

		if (!nx_connector->display)
			nx_connector->display = ctx->display;

		DRM_INFO("Load CLUSTER panel: %s\n", nx_panel_get_name(type));
	}

	return 0;
}

static int panel_cluster_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cluster_context *cluster;
	struct display_context *ctx;
	struct nx_drm_display_ops *ops;
	int i, err;
	int pipe;

	DRM_DEBUG_KMS("enter (%s)\n", dev_name(dev));

	cluster = devm_kzalloc(dev, sizeof(*cluster), GFP_KERNEL);
	if (!cluster)
		return -ENOMEM;

	cluster->connector = &cluster_connector_dev;
	cluster->connector->dev = dev;
	INIT_LIST_HEAD(&cluster->cluster_list);

	for (i = 0; i < CLUSTER_LCD_MAX; i++) {
		ctx = &cluster->context[i];
		INIT_LIST_HEAD(&ctx->list);
		list_add_tail(&ctx->list, &cluster->cluster_list);
	}

	err = panel_cluster_get_displays(pdev, cluster);
	if (err < 0)
		goto err_probe;

	pipe = cluster->crtc_pipe;
	list_for_each_entry(ctx, &cluster->cluster_list, list) {
		ops = ctx->display->ops;
		if (ops->open) {
			err = ops->open(ctx->display, pipe++);
			if (err)
				goto err_probe;
		}
	}

	err = panel_cluster_parse_dt(pdev, cluster);
	if (err < 0)
		goto err_probe;

	panel_cluster_set_backlight(pdev, cluster);

	dev_set_drvdata(dev, cluster);
	component_add(dev, &panel_comp_ops);

	DRM_DEBUG_KMS("done\n");
	return err;

err_probe:
	DRM_ERROR("Failed %s probe !!!\n", dev_name(dev));

	if (cluster && cluster->backlight)
		put_device(&cluster->backlight->dev);

	devm_kfree(dev, cluster);
	return err;
}

static int panel_cluster_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cluster_context *cluster = dev_get_drvdata(&pdev->dev);
	struct display_context *ctx;
	int pipe;

	if (!cluster)
		return 0;

	component_del(dev, &panel_comp_ops);

	pipe = cluster->crtc_pipe;
	list_for_each_entry(ctx, &cluster->cluster_list, list) {
		struct nx_drm_display_ops *ops = ctx->display->ops;

		if (ops->close)
			ops->close(ctx->display, pipe++);

		nx_drm_display_put(dev, ctx->display);
	}

	if (cluster->backlight)
		put_device(&cluster->backlight->dev);

	devm_kfree(dev, cluster);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int panel_cluster_suspend(struct device *dev)
{
	return 0;
}

static int panel_cluster_resume(struct device *dev)
{
	return 0;
}
#endif

static const struct dev_pm_ops panel_cluster_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(
		panel_cluster_suspend, panel_cluster_resume
	)
};

static const struct of_device_id panel_cluster_of_match[] = {
	{
		.compatible = "nexell,s5pxx18-drm-cluster-lcd",
		.data = (void *)NX_PANEL_TYPE_CLUSTER_LCD
	},
	{}
};
MODULE_DEVICE_TABLE(of, panel_cluster_of_match);

static struct platform_driver panel_cluster_driver = {
	.probe = panel_cluster_probe,
	.remove = panel_cluster_remove,
	.driver = {
		.name = "nexell,display_drm_cluster_lcd",
		.owner = THIS_MODULE,
		.of_match_table = panel_cluster_of_match,
		.pm = &panel_cluster_pm,
	},
};

void panel_cluster_init(void)
{
	platform_driver_register(&panel_cluster_driver);
}

void panel_cluster_exit(void)
{
	platform_driver_unregister(&panel_cluster_driver);
}

