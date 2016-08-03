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
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/of_gpio.h>
#include <video/of_display_timing.h>

#include <drm/nexell_drm.h>

#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include "nx_drm_drv.h"
#include "nx_drm_plane.h"
#include "nx_drm_crtc.h"
#include "nx_drm_encoder.h"
#include "nx_drm_connector.h"
#include "nx_drm_fb.h"
#include "soc/s5pxx18_dp_tv.h"

struct tv_resource {
	struct i2c_adapter *adpt;
};

struct nx_v4l2_i2c_board_info {
	int	i2c_adapter_id;
	struct	i2c_board_info board_info;
};

struct tv_context {
	struct drm_connector *connector;
	int crtc_pipe;
	unsigned int possible_crtcs_mask;
	struct reset_control *reset;
	void *base;
	struct nx_drm_device *display;
	struct delayed_work	work;
	struct mutex lock;
	bool local_timing;
	struct gpio_desc *enable_gpio;
	struct tv_resource tv_res;
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
};

static int register_v4l2(struct tv_context *);

#define ctx_to_tv(c)	(struct tv_resource *)(&c->tv_res)
#define CONNECTION_COUNT	10 /* 500 * CONNECTION_COUNT */

static struct v4l2_subdev *get_remote_source_subdev(struct tv_context *me)
{
	struct media_pad *pad =
		media_entity_remote_pad(&me->pad);
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


static bool panel_tv_is_connected(struct device *dev,
			struct drm_connector *connector)
{
	struct tv_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_panel *panel = &ctx->display->panel;
	struct device_node *panel_node = panel->panel_node;
		enum dp_panel_type panel_type = dp_panel_get_type(ctx->display);

	DRM_DEBUG_KMS("enter panel %s\n",
		panel_node ? "exist" : "not exist");

	if (panel_node) {
		struct drm_panel *drm_panel = of_drm_find_panel(panel_node);

		if (drm_panel) {
			int ret;

			panel->panel = drm_panel;
			drm_panel_attach(drm_panel, connector);

			if (panel->check_panel)
				return panel->is_connected;

			nx_drm_dp_tv_prepare(ctx->display, drm_panel);
			ret = drm_panel_prepare(drm_panel);
			if (!ret) {
				drm_panel_unprepare(drm_panel);
				nx_drm_dp_tv_unprepare(ctx->display,
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
	}

	if (!panel_node && false == ctx->local_timing) {
		DRM_DEBUG_DRIVER("not exist %s panel & timing %s !\n",
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

static int panel_tv_get_modes(struct device *dev,
			struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct tv_context *ctx = dev_get_drvdata(dev);
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

static int panel_tv_check_mode(struct device *dev,
			struct drm_display_mode *mode)
{
	struct tv_context *ctx = dev_get_drvdata(dev);
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

bool panel_tv_mode_fixup(struct device *dev,
			struct drm_connector *connector,
			const struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void panel_tv_commit(struct device *dev)
{
	struct tv_context *ctx = dev_get_drvdata(dev);
	int pipe = drm_dev_get_dpc(ctx->display)->module;

	nx_dp_tv_set_commit(ctx->display, pipe);
}

static void panel_tv_enable(struct device *dev, struct drm_panel *panel)
{
	struct tv_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_device *display = ctx->display;
	int ret = 0;

	nx_drm_dp_tv_prepare(display, panel);
	drm_panel_prepare(panel);

	drm_panel_enable(panel);
	ret = nx_drm_dp_tv_enable(display, panel);
	if (!ret && !ctx->is_enable) {
		if (ctx->is_run)
			call_set_register(ctx);

		ctx->is_enable = true;
	}
}

static void panel_tv_disable(struct device *dev, struct drm_panel *panel)
{
	struct tv_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_device *display = ctx->display;

	drm_panel_unprepare(panel);
	drm_panel_disable(panel);

	nx_drm_dp_tv_unprepare(display, panel);
	nx_drm_dp_tv_disable(display, panel);
}

static void panel_tv_dmps(struct device *dev, int mode)
{
	struct tv_context *ctx = dev_get_drvdata(dev);
	struct nx_drm_panel *panel = &ctx->display->panel;
	struct drm_panel *drm_panel = panel->panel;

	DRM_DEBUG_KMS("dpms.%d\n", mode);

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		panel_tv_enable(dev, drm_panel);

		if (!panel && ctx->enable_gpio)
			gpiod_set_value_cansleep(ctx->enable_gpio, 1);
		break;

	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		panel_tv_disable(dev, drm_panel);

		if (!panel && ctx->enable_gpio)
			gpiod_set_value_cansleep(ctx->enable_gpio, 0);
		break;
	default:
		DRM_ERROR("fail : unspecified mode %d\n", mode);
		break;
	}
}

static struct nx_drm_ops panel_tv_ops = {
	.is_connected = panel_tv_is_connected,
	.get_modes = panel_tv_get_modes,
	.check_mode = panel_tv_check_mode,
	.mode_fixup = panel_tv_mode_fixup,
	.commit = panel_tv_commit,
	.dpms = panel_tv_dmps,
};

static int panel_tv_bind(struct device *dev,
			struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct tv_context *ctx = dev_get_drvdata(dev);
	struct platform_driver *pdrv = to_platform_driver(dev->driver);
	enum dp_panel_type panel_type = dp_panel_get_type(ctx->display);
	int pipe = ctx->crtc_pipe;
	unsigned int possible_crtcs = ctx->possible_crtcs_mask;
	int err = 0;

	DRM_INFO("Bind %s panel\n", dp_panel_type_name(panel_type));
	ctx->connector = nx_drm_connector_create_and_attach(drm,
			ctx->display, pipe, possible_crtcs,
			panel_type, ctx);

	if (IS_ERR(ctx->connector))
		goto err_bind;

	if (!err) {
		struct nx_drm_priv *priv = drm->dev_private;

		if (panel_tv_is_connected(dev, ctx->connector))
			priv->force_detect = true;

		return 0;
	}
err_bind:
	if (pdrv->remove)
		pdrv->remove(to_platform_device(dev));

	return 0;
}

static void panel_tv_unbind(struct device *dev,
			struct device *master, void *data)
{
	struct tv_context *ctx = dev_get_drvdata(dev);
	/* enum dp_panel_type panel_type = dp_panel_get_type(ctx->display); */

	if (ctx->connector)
		nx_drm_connector_destroy_and_detach(ctx->connector);
}

static const struct component_ops panel_comp_ops = {
	.bind = panel_tv_bind,
	.unbind = panel_tv_unbind,
};

static void panel_tv_work(struct work_struct *work)
{
	struct tv_context *ctx;
	int err;

	ctx = container_of(work, struct tv_context, work.work);
	if (!ctx->connector)
		return;

	err = register_v4l2(ctx);
	if (err) {
		if (ctx->connection_cnt++ < CONNECTION_COUNT)
			mod_delayed_work(system_wq, &ctx->work,
				msecs_to_jiffies(500));
		else
			cancel_delayed_work_sync(&ctx->work);
	} else {
		drm_helper_hpd_irq_event(ctx->connector->dev);

		if (ctx->is_enable && !ctx->is_run) {
			call_set_register(ctx);
			ctx->is_run = true;
		}

		cancel_delayed_work_sync(&ctx->work);
	}
}

#define parse_read_prop(n, s, v)	{ \
	u32 _v;	\
	if (!of_property_read_u32(n, s, &_v))	\
		v = _v;	\
	}

static const struct of_device_id panel_tv_of_match[];

static int parse_sensor_i2c_board_info_dt(struct device_node *np,
			struct device *dev, struct tv_context *me)
{
	const char *name;
	u32 adapter;
	u32 addr;
	struct nx_v4l2_i2c_board_info *info = &me->sensor_info;

	if (of_property_read_string(np, "i2c_name", &name)) {
		dev_err(dev, "failed to get sensor i2c name\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "i2c_adapter", &adapter)) {
		dev_err(dev, "failed to get sensor i2c adapter\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "addr", &addr)) {
		dev_err(dev, "failed to get sensor i2c addr\n");
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
		dev_err(dev, "failed to get dt sensor register\n");
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
			struct tv_context *ctx, enum dp_panel_type panel_type)
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
	 * get possible crtcs
	 */
	parse_read_prop(node, "crtcs-possible-mask", ctx->possible_crtcs_mask);

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

	/*
		parse sensor
	*/
	np = of_find_node_by_name(node, "sensor");
	if (!np) {
		DRM_ERROR("fail : not find sensor node (%s) !\n",
			node->full_name);
		return -EINVAL;
	}

	err = parse_sensor_dt(np, dev, ctx);
	if (err)
		return err;

	return 0;
}

static int panel_tv_driver_setup(struct platform_device *pdev,
			struct tv_context *ctx, enum dp_panel_type *panel_type)
{
	const struct of_device_id *id;
	enum dp_panel_type type;
	struct device *dev = &pdev->dev;
	struct nx_drm_res *res = &ctx->display->res;
	int err;

	/*
	 * get panel type with of id
	 */
	id = of_match_node(panel_tv_of_match, pdev->dev.of_node);
	type = (enum dp_panel_type)id->data;

	DRM_INFO("Load %s panel\n", dp_panel_type_name(type));

	err = nx_drm_dp_panel_res_parse(dev, res, type);
	if (0 > err)
		return -EINVAL;

	*panel_type = type;

	return 0;
}

static int nx_tv_link_setup(struct media_entity *entity,
			const struct media_pad *local,
			const struct media_pad *remote,
			u32 flag)
{

	return 0;
}

static const struct media_entity_operations nx_tv_media_ops = {
	.link_setup = nx_tv_link_setup,
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
		dev_err(&me->pdev->dev, "%s: failed to media_entity_init()\n",
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
		dev_err(&me->pdev->dev, "failed to init sensor media entity\n");
		goto error;
	}

	ret  = nx_v4l2_register_subdev(sensor);
	if (ret) {
		dev_err(&me->pdev->dev, "failed to register subdev sensor\n");
		goto error;
	}

	input = &me->entity;
	pad = 0;

	input->parent = nx_v4l2_get_media_device();
	ret = media_entity_create_link(input, pad, &sensor->entity, 0, 0);
	if (ret < 0)
		dev_err(&me->pdev->dev,
			"failed to create link from sensor\n");

	ret = setup_link(&input->pads[pad], &sensor->entity.pads[0]);
	if (ret)
		BUG();
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
	enum dp_panel_type panel_type;
	struct device *dev = &pdev->dev;
	size_t size;
	int err;

	DRM_DEBUG_KMS("enter (%s)\n", dev_name(dev));

	size = sizeof(*ctx) + sizeof(struct nx_drm_device);
	ctx = devm_kzalloc(dev, size, GFP_KERNEL);
	if (IS_ERR(ctx))
		return -ENOMEM;

	ctx->pdev = pdev;

	ctx->display = (void *)ctx + sizeof(*ctx);
	ctx->display->dev = dev;
	ctx->display->ops = &panel_tv_ops;
	ctx->connection_cnt = 0;
	ctx->is_run = false;
	ctx->is_enable = false;
	ctx->sensor_reg = NULL;

	mutex_init(&ctx->lock);

	err = panel_tv_driver_setup(pdev, ctx, &panel_type);
	if (0 > err)
		goto err_parse;

	err = panel_tv_parse_dt(pdev, ctx, panel_type);
	if (0 > err)
		goto err_parse;

	panel_type = dp_panel_get_type(ctx->display);

	err = init_media_entity(ctx);
	if (err)
		return err;

	INIT_DELAYED_WORK(&ctx->work, panel_tv_work);
	mod_delayed_work(system_wq, &ctx->work, msecs_to_jiffies(500));

	dev_set_drvdata(dev, ctx);
	component_add(dev, &panel_comp_ops);

	DRM_DEBUG_KMS("done\n");
	return err;

err_parse:
	if (ctx)
		devm_kfree(dev, ctx);

	return err;
}

static int panel_tv_remove(struct platform_device *pdev)
{
	struct tv_context *ctx = dev_get_drvdata(&pdev->dev);
	struct device *dev = &pdev->dev;

	if (!ctx)
		return 0;

	nx_drm_dp_panel_res_free(dev, &ctx->display->res);
	nx_drm_dp_panel_dev_release(dev, ctx->display);

	devm_kfree(&pdev->dev, ctx);

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
		.data = (void *)dp_panel_type_tv
	},
	{}
};
MODULE_DEVICE_TABLE(of, panel_tv_of_match);

struct platform_driver panel_tv_driver = {
	.probe = panel_tv_probe,
	.remove = panel_tv_remove,
	.driver = {
		.name = "nexell,display_drm_tv",
		.owner = THIS_MODULE,
		.of_match_table = panel_tv_of_match,
		.pm = &panel_tv_pm,
	},
};
module_platform_driver(panel_tv_driver);

MODULE_AUTHOR("jkchoi <jkchoi@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell tv DRM Driver");
MODULE_LICENSE("GPL");
