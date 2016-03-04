/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/atomic.h>
#include <linux/irqreturn.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/semaphore.h>
#include <linux/v4l2-mediabus.h>
#include <linux/delay.h>

#include <media/media-device.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <dt-bindings/media/nexell-vip.h>

#include "../nx-v4l2.h"
#include "../nx-video.h"
#include "nx-vip-primitive.h"
#include "nx-vip.h"

#define NX_DECIMATOR_DEV_NAME	"nx-decimator"

enum {
	NX_DECIMATOR_PAD_SINK,
	NX_DECIMATOR_PAD_SOURCE_MEM,
	NX_DECIMATOR_PAD_MAX
};

enum {
	STATE_IDLE = 0,
	STATE_RUNNING  = (1 << 0),
	STATE_STOPPING = (1 << 1),
};

struct nx_decimator {
	u32 module;

	struct v4l2_subdev subdev;
	struct media_pad pads[NX_DECIMATOR_PAD_MAX];

	u32 width;
	u32 height;
	u32 clip_width;
	u32 clip_height;

	atomic_t state;
	struct completion stop_done;
	struct semaphore s_stream_sem;

	struct platform_device *pdev;

	struct nx_video_buffer_object vbuf_obj;
	struct nx_v4l2_irq_entry *irq_entry;
	u32 mem_fmt;
};

static int update_buffer(struct nx_decimator *me)
{
	int ret;
	struct nx_video_buffer *buf;

	ret = nx_video_update_buffer(&me->vbuf_obj);
	if (ret)
		return ret;

	buf = me->vbuf_obj.cur_buf;
	nx_vip_set_decimator_addr(me->module, me->mem_fmt,
				me->width, me->height,
				buf->dma_addr[0], buf->dma_addr[1],
				buf->dma_addr[2], buf->stride[0],
				buf->stride[1]);

	return 0;
}

static void unregister_irq_handler(struct nx_decimator *me)
{
	if (me->irq_entry) {
		nx_vip_unregister_irq_entry(me->module, me->irq_entry);
		kfree(me->irq_entry);
		me->irq_entry = NULL;
	}
}

static irqreturn_t nx_decimator_irq_handler(void *data)
{
	struct nx_decimator *me = data;

	nx_video_done_buffer(&me->vbuf_obj);
	if (NX_ATOMIC_READ(&me->state) & STATE_STOPPING) {
		nx_vip_stop(me->module, VIP_DECIMATOR);
		unregister_irq_handler(me);
		nx_video_clear_buffer(&me->vbuf_obj);
		complete(&me->stop_done);
	} else {
		update_buffer(me);
	}

	return IRQ_HANDLED;
}

static int register_irq_handler(struct nx_decimator *me)
{
	struct nx_v4l2_irq_entry *irq_entry = me->irq_entry;

	if (!irq_entry) {
		irq_entry = devm_kzalloc(&me->pdev->dev, sizeof(*irq_entry),
					 GFP_KERNEL);
		if (!irq_entry) {
			WARN_ON(1);
			return -ENOMEM;
		}
		me->irq_entry = irq_entry;
	}
	irq_entry->irqs = VIP_OD_INT;
	irq_entry->priv = me;
	irq_entry->handler = nx_decimator_irq_handler;

	return nx_vip_register_irq_entry(me->module, irq_entry);
}

static int decimator_buffer_queue(struct nx_video_buffer *buf, void *data)
{
	struct nx_decimator *me = data;

	nx_video_add_buffer(&me->vbuf_obj, buf);
	return 0;
}

static int handle_video_connection(struct nx_decimator *me, bool connected)
{
	int ret = 0;

	if (connected)
		ret = nx_video_register_buffer_consumer(&me->vbuf_obj,
							decimator_buffer_queue,
							me);
	else
		nx_video_unregister_buffer_consumer(&me->vbuf_obj);

	return ret;
}

static struct v4l2_subdev *get_remote_source_subdev(struct nx_decimator *me)
{
	struct media_pad *pad =
		media_entity_remote_pad(&me->pads[NX_DECIMATOR_PAD_SINK]);
	if (!pad) {
		dev_err(&me->pdev->dev, "can't find remote source device\n");
		return NULL;
	}
	return media_entity_to_v4l2_subdev(pad->entity);
}

static void set_vip(struct nx_decimator *me, u32 clip_width, u32 clip_height)
{
	nx_vip_set_decimation(me->module, clip_width, clip_height,
			      me->width, me->height);
	nx_vip_set_decimator_format(me->module, me->mem_fmt);
}

static int get_decimator_crop(struct v4l2_subdev *remote,
			    struct v4l2_crop *crop)
{
	return v4l2_subdev_call(remote, video, g_crop, crop);
}
/**
 * v4l2 subdev ops
 */
static int nx_decimator_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret;
	struct nx_decimator *me = v4l2_get_subdevdata(sd);
	u32 module = me->module;
	struct v4l2_subdev *remote;
	void *hostdata_back;

	remote = get_remote_source_subdev(me);
	if (!remote) {
		WARN_ON(1);
		return -ENODEV;
	}

	ret = down_interruptible(&me->s_stream_sem);

	if (enable) {
		if (NX_ATOMIC_READ(&me->state) & STATE_STOPPING) {
			int timeout = 50; /* 5 second */

			dev_info(&me->pdev->dev,  "wait decimator stopping\n");
			while (NX_ATOMIC_READ(&me->state) &
			       STATE_STOPPING) {
				msleep(100);
				timeout--;
				if (timeout == 0) {
					dev_err(&me->pdev->dev, "timeout for waiting decimator stop\n");
					break;
				}
			}
		}
		if (!(NX_ATOMIC_READ(&me->state) & STATE_RUNNING)) {
			struct v4l2_crop crop;

			hostdata_back = v4l2_get_subdev_hostdata(remote);
			v4l2_set_subdev_hostdata(remote, NX_DECIMATOR_DEV_NAME);
			ret = v4l2_subdev_call(remote, video, s_stream, 1);
			v4l2_set_subdev_hostdata(remote, hostdata_back);
			if (ret) {
				WARN_ON(1);
				goto UP_AND_OUT;
			}

			ret = get_decimator_crop(remote, &crop);
			if (ret) {
				WARN_ON(1);
				goto UP_AND_OUT;
			}

			set_vip(me, crop.c.width, crop.c.height);

			ret = register_irq_handler(me);
			if (ret) {
				WARN_ON(1);
				goto UP_AND_OUT;
			}

			update_buffer(me);
			nx_vip_run(me->module, VIP_DECIMATOR);
			NX_ATOMIC_SET_MASK(STATE_RUNNING, &me->state);
		}
	} else {
		if (NX_ATOMIC_READ(&me->state) & STATE_RUNNING) {
			NX_ATOMIC_SET_MASK(STATE_STOPPING, &me->state);
			if (!wait_for_completion_timeout(&me->stop_done,
							 2*HZ)) {
				pr_warn("timeout for waiting decimator stop\n");
				nx_vip_stop(module, VIP_DECIMATOR);
				unregister_irq_handler(me);
				nx_video_clear_buffer(&me->vbuf_obj);
				NX_ATOMIC_CLEAR_MASK(STATE_STOPPING,
						     &me->state);
			}

			nx_vip_stop(me->module, VIP_DECIMATOR);

			hostdata_back = v4l2_get_subdev_hostdata(remote);
			v4l2_set_subdev_hostdata(remote, NX_DECIMATOR_DEV_NAME);
			v4l2_subdev_call(remote, video, s_stream, 0);
			v4l2_set_subdev_hostdata(remote, hostdata_back);

			NX_ATOMIC_CLEAR_MASK(STATE_RUNNING, &me->state);
		}
	}

UP_AND_OUT:
	up(&me->s_stream_sem);

	return 0;
}

static int nx_decimator_get_fmt(struct v4l2_subdev *sd,
			      struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_format *format)
{
	struct nx_decimator *me = v4l2_get_subdevdata(sd);

	/* get mem format */
	u32 mem_fmt;
	int ret = nx_vip_find_mbus_mem_format(me->mem_fmt, &mem_fmt);

	if (ret) {
		dev_err(&me->pdev->dev, "can't get mbus_fmt for mem\n");
		return ret;
	}
	format->format.code = mem_fmt;
	format->format.width = me->width;
	format->format.height = me->height;

	return 0;
}

static int nx_decimator_set_fmt(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_format *format)
{
	struct nx_decimator *me = v4l2_get_subdevdata(sd);
	/* set memory format */
	u32 nx_mem_fmt;
	int ret = nx_vip_find_nx_mem_format(format->format.code,
					    &nx_mem_fmt);
	if (ret) {
		dev_err(&me->pdev->dev, "Unsupported mem format %d\n",
		       format->format.code);
		return ret;
	}
	me->mem_fmt = nx_mem_fmt;
	me->width = format->format.width;
	me->height = format->format.height;

	return 0;
}

static const struct v4l2_subdev_video_ops nx_decimator_video_ops = {
	.s_stream = nx_decimator_s_stream,
};

static const struct v4l2_subdev_pad_ops nx_decimator_pad_ops = {
	.get_fmt = nx_decimator_get_fmt,
	.set_fmt = nx_decimator_set_fmt,
};

static const struct v4l2_subdev_ops nx_decimator_subdev_ops = {
	.video = &nx_decimator_video_ops,
	.pad = &nx_decimator_pad_ops,
};
/**
 * media_entity_operations
 */
static int nx_decimator_link_setup(struct media_entity *entity,
				 const struct media_pad *local,
				 const struct media_pad *remote,
				 u32 flags)
{
	struct v4l2_subdev *sd = media_entity_to_v4l2_subdev(entity);
	struct nx_decimator *me = v4l2_get_subdevdata(sd);

	switch (local->index | media_entity_type(remote->entity)) {
	case NX_DECIMATOR_PAD_SINK | MEDIA_ENT_T_V4L2_SUBDEV:
		pr_debug("decimator sink %s\n",
			 flags & MEDIA_LNK_FL_ENABLED ?
			 "connected" : "disconnected");
		break;
	case NX_DECIMATOR_PAD_SOURCE_MEM | MEDIA_ENT_T_DEVNODE:
		pr_debug("decimator source mem %s\n",
			 flags & MEDIA_LNK_FL_ENABLED ?
			 "connected" : "disconnected");
		handle_video_connection(me, flags & MEDIA_LNK_FL_ENABLED ?
					true : false);
		break;
	}

	return 0;
}

static const struct media_entity_operations nx_decimator_media_ops = {
	.link_setup = nx_decimator_link_setup,
};

/**
 * initialization
 */
static void init_me(struct nx_decimator *me)
{
	NX_ATOMIC_SET(&me->state, STATE_IDLE);
	init_completion(&me->stop_done);
	sema_init(&me->s_stream_sem, 1);
	nx_video_init_vbuf_obj(&me->vbuf_obj);
}

static int init_v4l2_subdev(struct nx_decimator *me)
{
	int ret;
	struct v4l2_subdev *sd = &me->subdev;
	struct media_pad *pads = me->pads;
	struct media_entity *entity = &sd->entity;

	v4l2_subdev_init(sd, &nx_decimator_subdev_ops);
	snprintf(sd->name, sizeof(sd->name), "%s%d", NX_DECIMATOR_DEV_NAME,
		 me->module);
	v4l2_set_subdevdata(sd, me);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	pads[NX_DECIMATOR_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	pads[NX_DECIMATOR_PAD_SOURCE_MEM].flags = MEDIA_PAD_FL_SOURCE;

	entity->ops = &nx_decimator_media_ops;
	ret = media_entity_init(entity, NX_DECIMATOR_PAD_MAX, pads, 0);
	if (ret < 0) {
		dev_err(&me->pdev->dev, "failed to media_entity_init\n");
		return ret;
	}

	return 0;
}

static int register_v4l2(struct nx_decimator *me)
{
	int ret;
	char dev_name[64] = {0, };
	struct media_entity *entity = &me->subdev.entity;
	struct nx_video *video;

	ret = nx_v4l2_register_subdev(&me->subdev);
	if (ret)
		BUG();

	snprintf(dev_name, sizeof(dev_name), "VIDEO DECIMATOR%d", me->module);
	video = nx_video_create(dev_name, NX_VIDEO_TYPE_CAPTURE,
				    nx_v4l2_get_v4l2_device(),
				    nx_v4l2_get_alloc_ctx());
	if (!video)
		BUG();


	ret = media_entity_create_link(entity, NX_DECIMATOR_PAD_SOURCE_MEM,
				       &video->vdev.entity, 0, 0);
	if (ret < 0)
		BUG();

	me->vbuf_obj.video = video;
	return 0;
}

static void unregister_v4l2(struct nx_decimator *me)
{
	if (me->vbuf_obj.video) {
		nx_video_cleanup(me->vbuf_obj.video);
		me->vbuf_obj.video = NULL;
	}
	v4l2_device_unregister_subdev(&me->subdev);
}

static int nx_decimator_parse_dt(struct platform_device *pdev,
				 struct nx_decimator *me)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;

	if (of_property_read_u32(np, "module", &me->module)) {
		dev_err(dev, "failed to get dt module\n");
		return -EINVAL;
	}

	return 0;
}

/**
 * platform driver
 */
static int nx_decimator_probe(struct platform_device *pdev)
{
	int ret;
	struct nx_decimator *me;
	struct device *dev = &pdev->dev;

	me = devm_kzalloc(dev, sizeof(*me), GFP_KERNEL);
	if (!me) {
		WARN_ON(1);
		return -ENOMEM;
	}

	if (!nx_vip_is_valid(me->module)) {
		dev_err(dev, "NX VIP %d is not valid\n", me->module);
		return -ENODEV;
	}

	ret = nx_decimator_parse_dt(pdev, me);
	if (ret) {
		kfree(me);
		return ret;
	}

	init_me(me);

	ret = init_v4l2_subdev(me);
	if (ret) {
		kfree(me);
		return ret;
	}

	ret = register_v4l2(me);
	if (ret) {
		kfree(me);
		return ret;
	}

	me->pdev = pdev;
	platform_set_drvdata(pdev, me);

	return 0;
}

static int nx_decimator_remove(struct platform_device *pdev)
{
	struct nx_decimator *me = platform_get_drvdata(pdev);

	if (unlikely(!me))
		return 0;

	unregister_v4l2(me);
	kfree(me);

	return 0;
}

static struct platform_device_id nx_decimator_id_table[] = {
	{ NX_DECIMATOR_DEV_NAME, 0 },
	{},
};

static const struct of_device_id nx_decimator_dt_match[] = {
	{ .compatible = "nexell,nx-decimator" },
	{},
};
MODULE_DEVICE_TABLE(of, nx_decimator_dt_match);

static struct platform_driver nx_decimator_driver = {
	.probe		= nx_decimator_probe,
	.remove		= nx_decimator_remove,
	.id_table	= nx_decimator_id_table,
	.driver		= {
		.name	= NX_DECIMATOR_DEV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(nx_decimator_dt_match),
	},
};

module_platform_driver(nx_decimator_driver);

MODULE_AUTHOR("swpark <swpark@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell S5Pxx18 series SoC V4L2 capture decimator driver");
MODULE_LICENSE("GPL");
