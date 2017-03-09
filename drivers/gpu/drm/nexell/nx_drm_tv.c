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

#include <linux/sched.h>
#include <linux/component.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/of_gpio.h>
#include <video/of_display_timing.h>

#include <drm/nexell_drm.h>

#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include "nx_drm_drv.h"
#include "nx_drm_fb.h"

struct tv_resource {
	struct i2c_adapter *adpt;
};

struct nx_v4l2_i2c_board_info {
	int	i2c_adapter_id;
	struct	i2c_board_info board_info;
};

struct tv_context {
	struct nx_drm_connector connector;
	/* lcd parameters */
	struct reset_control *reset;
	void *base;
	struct delayed_work	work;
	bool local_timing;
	struct gpio_desc *enable_gpio;
	struct tv_resource tv;
	bool plug;
	int connection_cnt;
	bool is_run;
	bool is_enable;
	struct platform_device *pdev;
	struct nx_v4l2_i2c_board_info sensor_info;
	struct v4l2_dbg_register *sensor_reg;
	struct media_pad sensor_pad;
	struct media_pad pad;
	struct media_entity entity;
	/* properties */
	int crtc_pipe;
	unsigned int possible_crtcs_mask;
};

static int register_v4l2(struct tv_context *);
#define CONNECTION_COUNT	10 /* 500 * CONNECTION_COUNT */

#define ctx_to_display(c)	\
		((struct nx_drm_display *)(c->connector.display))

static struct v4l2_subdev *get_remote_source_subdev(struct tv_context *me)
{
	struct media_pad *pad = media_entity_remote_pad(&me->pad);

	if (!pad) {
		dev_err(&me->pdev->dev, "can't find remote source device\n");
		return NULL;
	}

	return media_entity_to_v4l2_subdev(pad->entity);
}

static int call_set_register(struct tv_context *ctx)
{
	struct v4l2_subdev *remote;
	int err;

	remote = get_remote_source_subdev(ctx);

	if (!remote) {
		WARN_ON(1);
		return -ENODEV;
	}

	err = v4l2_subdev_call(remote, core, s_register, ctx->sensor_reg);
	if (err) {
		dev_info(&ctx->pdev->dev, "can't call s_register!!\n");
		return err;
	}

	return 0;
}

static bool panel_tv_ops_detect(struct device *dev,
			struct drm_connector *connector)
{
	struct tv_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;
	enum nx_panel_type panel_type = nx_panel_get_type(display);

	DRM_DEBUG_KMS("panel %s\n",
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
				nx_panel_get_name(panel_type),
				display->is_connected ?
					"connected" : "disconnected");

			return display->is_connected;
		}
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

static bool panel_tv_ops_is_connected(struct device *dev)
{
	struct tv_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);

	return display->is_connected;
}

static int panel_tv_ops_get_modes(struct device *dev,
			struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct tv_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);

	DRM_DEBUG_KMS("panel %s\n",
		display->panel ? "attached" : "detached");

	if (display->panel)
		return drm_panel_get_modes(display->panel);

	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("Failed to create a new display mode !\n");
		return 0;
	}

	drm_display_mode_from_videomode(&display->vm, mode);
	mode->width_mm = display->width_mm;
	mode->height_mm = display->height_mm;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	DRM_DEBUG_KMS("exit, (%dx%d, flags=0x%x)\n",
		mode->hdisplay, mode->vdisplay, mode->flags);

	return 1;
}

static int panel_tv_ops_valid_mode(struct device *dev,
			struct drm_display_mode *mode)
{
	struct tv_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);

	drm_display_mode_to_videomode(mode, &display->vm);

	display->width_mm = mode->width_mm;
	display->height_mm = mode->height_mm;

	DRM_DEBUG_KMS("SYNC -> LCD %d x %d mm\n",
		display->width_mm, display->height_mm);
	DRM_DEBUG_KMS("ha:%d, hf:%d, hb:%d, hs:%d\n",
		display->vm.hactive, display->vm.hfront_porch,
		display->vm.hback_porch, display->vm.hsync_len);
	DRM_DEBUG_KMS("va:%d, vf:%d, vb:%d, vs:%d\n",
		display->vm.vactive, display->vm.vfront_porch,
		display->vm.vback_porch, display->vm.vsync_len);
	DRM_DEBUG_KMS("flags:0x%x\n", display->vm.flags);

	return MODE_OK;
}

static void panel_tv_ops_set_mode(struct device *dev,
			struct drm_display_mode *mode)
{
	struct tv_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;

	DRM_DEBUG_KMS("enter\n");

	if (ops->set_mode)
		ops->set_mode(display, mode, 0);
}

static void panel_tv_on(struct tv_context *ctx, struct drm_panel *panel)
{
	struct nx_drm_connector *nx_connector = &ctx->connector;
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;
	int ret = 0;

	if (nx_connector->suspended) {
		if (ops->resume)
			ops->resume(display);
	}

	if (ops->prepare)
		ops->prepare(display);

	drm_panel_prepare(panel);
	drm_panel_enable(panel);

	if (ops->enable)
		ret = ops->enable(display);

	if (!ret && !ctx->is_enable) {
		if (ctx->is_run)
			call_set_register(ctx);

		ctx->is_enable = true;
	}
}

static void panel_tv_off(struct tv_context *ctx, struct drm_panel *panel)
{
	struct nx_drm_connector *nx_connector = &ctx->connector;
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct nx_drm_display_ops *ops = display->ops;

	if (nx_connector->suspended) {
		if (ops->suspend)
			ops->suspend(display);
	}

	drm_panel_unprepare(panel);
	drm_panel_disable(panel);

	if (ops->unprepare)
		ops->unprepare(display);

	if (ops->disable)
		ops->disable(display);
}

static void panel_tv_ops_enable(struct device *dev)
{
	struct tv_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);

	DRM_DEBUG_KMS("enter\n");

	panel_tv_on(ctx, display->panel);

	if (ctx->enable_gpio)
		gpiod_set_value_cansleep(ctx->enable_gpio, 1);
}

static void panel_tv_ops_disable(struct device *dev)
{
	struct tv_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_display *display = ctx_to_display(ctx);

	DRM_DEBUG_KMS("enter\n");

	panel_tv_off(ctx, display->panel);

	if (ctx->enable_gpio)
		gpiod_set_value_cansleep(ctx->enable_gpio, 0);
}

static struct nx_drm_connect_drv_ops tvout_connector_ops = {
	.detect = panel_tv_ops_detect,
	.is_connected = panel_tv_ops_is_connected,
	.get_modes = panel_tv_ops_get_modes,
	.valid_mode = panel_tv_ops_valid_mode,
	.set_mode = panel_tv_ops_set_mode,
	.enable = panel_tv_ops_enable,
	.disable = panel_tv_ops_disable,
};

static int panel_tv_bind(struct device *dev,
			struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct tv_context *ctx = dev_get_drvdata(dev);
	struct drm_connector *connector = &ctx->connector.connector;
	enum nx_panel_type panel_type = nx_panel_get_type(ctx_to_display(ctx));
	struct platform_driver *drv;
	int err = 0;

	DRM_INFO("Bind %s panel\n", nx_panel_get_name(panel_type));

	err = nx_drm_connector_attach(drm, connector,
			ctx->crtc_pipe, ctx->possible_crtcs_mask, panel_type);
	if (err < 0)
		goto err_bind;

	if (!err) {
		struct nx_drm_private *private = drm->dev_private;

		if (panel_tv_ops_detect(dev, connector))
			private->force_detect = true;

		return 0;
	}

err_bind:
	drv = to_platform_driver(dev->driver);
	if (drv->remove)
		drv->remove(to_platform_device(dev));

	return 0;
}

static void panel_tv_unbind(struct device *dev,
			struct device *master, void *data)
{
	struct tv_context *ctx = dev_get_drvdata(dev);
	struct drm_connector *connector = &ctx->connector.connector;

	if (connector)
		nx_drm_connector_detach(connector);
}

static const struct component_ops panel_comp_ops = {
	.bind = panel_tv_bind,
	.unbind = panel_tv_unbind,
};

static void panel_tv_work(struct work_struct *work)
{
	struct tv_context *ctx;
	struct drm_connector *connector;
	int err;

	ctx = container_of(work, struct tv_context, work.work);
	connector = &ctx->connector.connector;
	err = register_v4l2(ctx);
	if (err) {
		if (ctx->connection_cnt++ < CONNECTION_COUNT)
			mod_delayed_work(system_wq, &ctx->work,
				msecs_to_jiffies(500));
		else
			cancel_delayed_work_sync(&ctx->work);
	} else {
		drm_helper_hpd_irq_event(connector->dev);

		if (ctx->is_enable && !ctx->is_run) {
			call_set_register(ctx);
			ctx->is_run = true;
		}

		cancel_delayed_work_sync(&ctx->work);
	}
}

#define property_read(n, s, v)	of_property_read_u32(n, s, &v)
static const struct of_device_id panel_tv_of_match[];

static int parse_sensor_i2c_board_info_dt(struct device_node *np,
			struct device *dev, struct tv_context *me)
{
	const char *name;
	u32 adapter;
	u32 addr;
	struct nx_v4l2_i2c_board_info *info = &me->sensor_info;

	if (of_property_read_string(np, "i2c_name", &name)) {
		dev_err(dev, "Failed to get sensor i2c name\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "i2c_adapter", &adapter)) {
		dev_err(dev, "Failed to get sensor i2c adapter\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "addr", &addr)) {
		dev_err(dev, "Failed to get sensor i2c addr\n");
		return -EINVAL;
	}

	strcpy(info->board_info.type, name);
	info->board_info.addr = addr;
	info->i2c_adapter_id = adapter;

	return 0;

}

static int parse_sensor_register_dt(struct device_node *np,
			struct device *dev, struct tv_context *me)
{
	struct v4l2_dbg_register **reg = &me->sensor_reg;
	struct property *prop;
	const __be32 *cur;
	u32 val;
	int index = 0;
	int data_pos = 0;
	int alloc_size;
	int count = of_property_count_elems_of_size(np, "register", 4);

	if ((count%2) != 0) {
		dev_err(dev, "Failed to get dt sensor register\n");
		return -EINVAL;
	}

	alloc_size = (count / 2) + 1;
	if (count > 0 && *reg == NULL) {
		*reg = kcalloc(alloc_size,
			 sizeof(struct v4l2_dbg_register), GFP_KERNEL);
		if (!*reg) {
			WARN_ON(1);
			return -ENOMEM;
		}

		of_property_for_each_u32(np, "register", prop, cur, val) {
			if (index % 2) {
				(*reg + data_pos)->val = val;
				(*reg + data_pos)->size = 8;
				data_pos++;
			} else
				(*reg + data_pos)->reg = val;
			index++;
		}

		(*reg + data_pos)->reg = 0xFF;
		(*reg + data_pos)->val = 0xFF;
		(*reg + data_pos)->size = 0;
	}

	return 0;
}

static int parse_sensor_dt(struct device_node *np, struct device *dev,
			struct tv_context *me)
{
	int ret = 0;

	ret = parse_sensor_i2c_board_info_dt(np, dev, me);
	if (ret)
		return ret;

	if (of_find_property(np, "register", NULL))
		parse_sensor_register_dt(np, dev, me);

	return 0;
}

static int panel_tv_parse_dt(struct platform_device *pdev,
			struct tv_context *ctx)
{
	struct device *dev = &pdev->dev;
	struct nx_drm_display *display = ctx_to_display(ctx);
	struct device_node *node = dev->of_node;
	struct device_node *np;
	struct display_timing timing;
	int err;

	DRM_INFO("Load TV-OUT panel\n");

	property_read(node, "crtc-pipe", ctx->crtc_pipe);

	/* get possible crtcs */
	property_read(node, "crtcs-possible-mask", ctx->possible_crtcs_mask);

	/* get panel timing from local. */
	np = of_graph_get_remote_port_parent(node);
	display->panel_node = np;
	if (!np) {
		DRM_INFO("not use remote panel node (%s) !\n",
			node->full_name);

		/* parse panel info */
		ctx->enable_gpio =
			devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);

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

	/* parse display control config	 */
	np = of_find_node_by_name(node, "dp_control");
	of_node_get(node);
	if (!np) {
		DRM_ERROR("Failed, not find panel's control node (%s) !\n",
			node->full_name);
		return -EINVAL;
	}

	nx_drm_display_setup(display, np, display->panel_type);

	/* parse sensor	*/
	np = of_find_node_by_name(node, "sensor");
	of_node_get(node);
	if (!np) {
		DRM_ERROR("Failed, not find sensor node (%s) !\n",
			node->full_name);
		return -EINVAL;
	}
	of_node_put(np);

	err = parse_sensor_dt(np, dev, ctx);
	if (err)
		return err;

	return 0;
}

static int panel_tv_get_display(struct platform_device *pdev,
			struct tv_context *ctx)
{
	struct device *dev = &pdev->dev;
	struct nx_drm_connector *nx_connector = &ctx->connector;
	struct drm_connector *connector = &nx_connector->connector;

	/* get TV-OUT */
	nx_connector->display =
		nx_drm_display_get(dev,
			dev->of_node, connector, NX_PANEL_TYPE_TV);
	if (!nx_connector->display)
		return -ENOENT;

	return 0;
}

static int panel_tv_link_setup(struct media_entity *entity,
			const struct media_pad *local,
			const struct media_pad *remote,
			u32 flag)
{
	return 0;
}

static const struct media_entity_operations nx_tv_media_ops = {
	.link_setup = panel_tv_link_setup,
};

static int setup_link(struct media_pad *src, struct media_pad *dst)
{
	struct media_link *link;

	link = media_entity_find_link(src, dst);
	if (link == NULL)
		return -ENODEV;

	return __media_entity_setup_link(link, MEDIA_LNK_FL_ENABLED);
}

static int init_sensor_media_entity(struct tv_context *me,
				struct v4l2_subdev *sd)
{
	if (!sd->entity.links) {
		struct media_pad *pad = &me->sensor_pad;

		pad->flags = MEDIA_PAD_FL_SINK;
		sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;

		return media_entity_init(&sd->entity, 1, pad, 0);
	}

	return 0;
}

static int init_media_entity(struct tv_context *me)
{
	struct media_pad *pad;
	struct media_entity *entity;
	int err;

	pad = &me->pad;
	entity = &me->entity;

	pad->flags = MEDIA_PAD_FL_SOURCE;
	entity->ops = &nx_tv_media_ops;

	err = media_entity_init(entity, 1, pad, 0);
	if (err < 0) {
		dev_err(&me->pdev->dev, "%s: Failed to media_entity_init()\n",
			__func__);
		return err;
	}

	return 0;
}

static int register_sensor_subdev(struct tv_context *me)
{
	int ret = 0;
	struct i2c_adapter *adapter;
	struct v4l2_subdev *sensor;
	struct i2c_client *client;
	struct media_entity *input;
	u32 pad;
	struct nx_v4l2_i2c_board_info *info = &me->sensor_info;

	adapter = i2c_get_adapter(info->i2c_adapter_id);
	if (!adapter) {
		dev_err(&me->pdev->dev, "unable to get sensor i2c adapter\n");
		return -ENODEV;
	}

	request_module(I2C_MODULE_PREFIX "%s", info->board_info.type);
	client = i2c_new_device(adapter, &info->board_info);

	if (client == NULL || client->dev.driver == NULL) {
		ret = -ENODEV;
		goto error;
	}

	if (!try_module_get(client->dev.driver->owner)) {
		ret = -ENODEV;
		goto error;
	}
	sensor = i2c_get_clientdata(client);
	sensor->host_priv = info;

	ret = init_sensor_media_entity(me, sensor);
	if (ret) {
		dev_err(&me->pdev->dev, "Failed to init sensor media entity\n");
		goto error;
	}

	ret  = nx_v4l2_register_subdev(sensor);
	if (ret) {
		dev_err(&me->pdev->dev, "Failed to register subdev sensor\n");
		goto error;
	}

	input = &me->entity;
	pad = 0;

	input->parent = nx_v4l2_get_media_device();
	ret = media_entity_create_link(input, pad, &sensor->entity, 0, 0);
	if (ret < 0)
		dev_err(&me->pdev->dev,
			"Failed to create link from sensor\n");

	ret = setup_link(&input->pads[pad], &sensor->entity.pads[0]);
	BUG_ON(ret);
error:
	if (client && ret < 0)
		i2c_unregister_device(client);

	return ret;
}

static int register_v4l2(struct tv_context *me)
{
	int ret;

	ret = register_sensor_subdev(me);
	if (ret) {
		dev_err(&me->pdev->dev, "can't register sensor subdev\n");
		return ret;
	}

	return 0;
}

static int panel_tv_probe(struct platform_device *pdev)
{
	struct tv_context *ctx;
	struct device *dev = &pdev->dev;
	struct nx_drm_display_ops *ops;
	size_t size;
	int err;

	DRM_DEBUG_KMS("enter (%s)\n", dev_name(dev));

	size = sizeof(*ctx);
	ctx = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->pdev = pdev;
	ctx->connector.dev = dev;
	ctx->connector.ops = &tvout_connector_ops;
	ctx->connection_cnt = 0;
	ctx->is_run = false;
	ctx->is_enable = false;
	ctx->sensor_reg = NULL;

	err = panel_tv_get_display(pdev, ctx);
	if (err < 0)
		goto err_probe;

	ops = ctx_to_display(ctx)->ops;
	if (ops->open) {
		err = ops->open(ctx_to_display(ctx), ctx->crtc_pipe);
		if (err)
			goto err_probe;
	}

	err = panel_tv_parse_dt(pdev, ctx);
	if (err < 0)
		goto err_probe;

	err = init_media_entity(ctx);
	if (err)
		return err;

	INIT_DELAYED_WORK(&ctx->work, panel_tv_work);
	mod_delayed_work(system_wq, &ctx->work, msecs_to_jiffies(500));

	dev_set_drvdata(dev, ctx);
	component_add(dev, &panel_comp_ops);

	DRM_DEBUG_KMS("done\n");
	return err;

err_probe:
	DRM_ERROR("Failed %s probe !!!\n", dev_name(dev));
	devm_kfree(dev, ctx);
	return err;
}

static int panel_tv_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct tv_context *ctx = dev_get_drvdata(&pdev->dev);
	struct nx_drm_display_ops *ops;

	if (!ctx)
		return 0;

	component_del(dev, &panel_comp_ops);

	ops = ctx_to_display(ctx)->ops;
	if (ops->close)
		ops->close(ctx_to_display(ctx), ctx->crtc_pipe);

	nx_drm_display_put(dev, ctx_to_display(ctx));
	devm_kfree(dev, ctx);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int panel_tv_suspend(struct device *dev)
{
	return 0;
}

static int panel_tv_resume(struct device *dev)
{
	return 0;
}
#endif

static const struct dev_pm_ops panel_tv_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(
		panel_tv_suspend, panel_tv_resume
	)
};

static const struct of_device_id panel_tv_of_match[] = {
	{.
		compatible = "nexell,s5pxx18-drm-tv",
		.data = (void *)NX_PANEL_TYPE_TV
	},
	{}
};
MODULE_DEVICE_TABLE(of, panel_tv_of_match);

static struct platform_driver panel_tv_driver = {
	.probe = panel_tv_probe,
	.remove = panel_tv_remove,
	.driver = {
		.name = "nexell,display_drm_tv",
		.owner = THIS_MODULE,
		.of_match_table = panel_tv_of_match,
		.pm = &panel_tv_pm,
	},
};

void panel_tv_init(void)
{
	platform_driver_register(&panel_tv_driver);
}

void panel_tv_exit(void)
{
	platform_driver_unregister(&panel_tv_driver);
}
