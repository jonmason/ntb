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

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/videodev2.h>
#include <linux/v4l2-mediabus.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-subdev.h>

#include <linux/soc/nexell/nx-media-device.h>

#include "nx-v4l2.h"
#include "nx-video.h"

#define YUV_STRIDE_ALIGN_FACTOR     64
#define YUV_VSTRIDE_ALIGN_FACTOR    16

#define YUV_STRIDE(w)    ALIGN(w, YUV_STRIDE_ALIGN_FACTOR)
#define YUV_YSTRIDE(w)   (ALIGN(w/2, YUV_STRIDE_ALIGN_FACTOR) * 2)
#define YUV_VSTRIDE(h)   ALIGN(h, YUV_VSTRIDE_ALIGN_FACTOR)

static int video_device_number = NX_CAPTURE_START;
/*
 * supported formats
 * V4L2_PIX_FMT_XXX : reference to include/linux/videodev2.h
 * mbus_code : this field is only meaningful in NX V4L2, not same to real format
 */
static struct nx_video_format supported_formats[] = {
	{
		.name		= "YUV 4:2:2 packed, YCbYCr",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
		.mbus_code	= MEDIA_BUS_FMT_YUYV8_1X16,
		.num_planes	= 1,
		.num_sw_planes	= 1,
		.is_separated	= false,
	}, {
		.name		= "YUV 4:2:0 separated 1-planar, YCbYCr",
		.pixelformat	= V4L2_PIX_FMT_YUV420,
		.mbus_code	= MEDIA_BUS_FMT_YUYV12_1X24,
		.num_planes	= 1,
		.num_sw_planes	= 3,
		.is_separated	= false,
	}, {
		.name		= "YUV 4:2:0 separated 1-planar, YCrYCb",
		.pixelformat	= V4L2_PIX_FMT_YVU420,
		.mbus_code	= MEDIA_BUS_FMT_YVYU12_1X24,
		.num_planes	= 1,
		.num_sw_planes	= 3,
		.is_separated	= false,
	}, {
		.name		= "YUV 4:2:2 separated 1-planar, YCbCr",
		.pixelformat	= V4L2_PIX_FMT_YUV422P,
		.mbus_code	= MEDIA_BUS_FMT_YDYUYDYV8_1X16,
		.num_planes	= 1,
		.num_sw_planes	= 3,
		.is_separated	= false,
	}, {
		.name		= "YUV 4:4:4 separated 1-planar, YCbCr",
		.pixelformat	= V4L2_PIX_FMT_YUV444,
		.mbus_code	= MEDIA_BUS_FMT_AYUV8_1X32,
		.num_planes	= 1,
		.num_sw_planes	= 3,
		.is_separated	= false,
	}, {
		.name		= "YUV 4:2:0 separated 1-planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV12,
		.mbus_code	= MEDIA_BUS_FMT_YUYV12_2X12,
		.num_planes	= 1,
		.num_sw_planes	= 2,
		.is_separated	= false,
	}, {
		.name		= "YUV 4:2:2 separated 1-planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV16,
		.mbus_code	= MEDIA_BUS_FMT_YUYV8_2X8,
		.num_planes	= 1,
		.num_sw_planes	= 2,
		.is_separated	= false,
	}, {
		.name		= "YUV 4:4:4 separated 1-planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV24,
		.mbus_code	= MEDIA_BUS_FMT_YUYV8_1_5X8,
		.num_planes	= 1,
		.num_sw_planes	= 2,
		.is_separated	= false,
	}, {
		.name		= "YUV 4:2:0 separated 1-planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV21,
		.mbus_code	= MEDIA_BUS_FMT_YVYU12_2X12,
		.num_planes	= 1,
		.num_sw_planes	= 2,
		.is_separated	= false,
	}, {
		.name		= "YUV 4:2:2 separated 1-planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV61,
		.mbus_code	= MEDIA_BUS_FMT_YVYU8_2X8,
		.num_planes	= 1,
		.num_sw_planes	= 2,
		.is_separated	= false,
	}, {
		.name		= "YUV 4:4:4 separated 1-planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV42,
		.mbus_code	= MEDIA_BUS_FMT_YVYU8_1_5X8,
		.num_planes	= 1,
		.num_sw_planes	= 2,
		.is_separated	= false,
	}
};

static struct nx_video_format *find_format(u32 pixelformat, int index)
{
	struct nx_video_format *fmt, *def_fmt = NULL;
	unsigned int i;

	if (index >= ARRAY_SIZE(supported_formats))
		return NULL;

	for (i = 0; i < ARRAY_SIZE(supported_formats); ++i) {
		fmt = &supported_formats[i];
		if (pixelformat == fmt->pixelformat)
			return fmt;
		if (index == i)
			def_fmt = fmt;
	}
	return def_fmt;
}

static int set_plane_size(struct nx_video_frame *frame, unsigned int sizes[])
{
	u32 y_stride = ALIGN(frame->width, 32);
	u32 y_size = y_stride * ALIGN(frame->height, 16);
	int i, j;

	pr_debug("[%s] format = 0x%x\n", __func__, frame->format.pixelformat);

	switch (frame->format.pixelformat) {
	case V4L2_PIX_FMT_YUYV:
		frame->size[0] = sizes[0] = y_size << 1;
		frame->stride[0] = y_stride << 1;
		break;

	case V4L2_PIX_FMT_YUV420M:
		frame->size[0] = sizes[0] =
			YUV_YSTRIDE(frame->width) * YUV_VSTRIDE(frame->height);
		frame->size[1] = sizes[1] =
			frame->size[2] = sizes[2] =
			YUV_STRIDE(frame->width/2)
			* YUV_VSTRIDE(frame->height/2);

		frame->stride[0] = YUV_YSTRIDE(frame->width);
		frame->stride[1] = YUV_STRIDE(frame->width/2);
		frame->stride[2] = YUV_STRIDE(frame->width/2);
		break;

	case V4L2_PIX_FMT_YUV420:
		if (frame->format.field == V4L2_FIELD_INTERLACED) {
			y_stride = ALIGN(frame->width, 128);
			y_size = y_stride * ALIGN(frame->height, 16);

			frame->size[0] = sizes[0] = y_size;
			frame->size[1] = sizes[1] =
				frame->size[2] = sizes[2] =
				ALIGN(y_stride >> 1, 64)
				* ALIGN(frame->height >> 1, 16);

			frame->stride[0] = y_stride;
			frame->stride[1] = frame->stride[2] =
			ALIGN(frame->width >> 1, 64);
		} else {
			frame->size[0] = sizes[0] = y_size;
			frame->size[1] = sizes[1] =
				frame->size[2] = sizes[2] =
				ALIGN(y_stride >> 1, 16)
				* ALIGN(frame->height >> 1, 16);

			frame->stride[0] = y_stride;
			frame->stride[1] = frame->stride[2] =
			ALIGN(y_stride >> 1, 16);
		}
		break;
	case V4L2_PIX_FMT_YVU420:
		for (j = 0; j < frame->format.num_planes; j++) {
			sizes[j] = 0;
			for (i = 0; i < frame->format.num_sw_planes; i++)
				sizes[j] += frame->size[i];
			pr_debug("[%s] %d: size[%d]=%d\n", __func__,
				j, j, sizes[j]);
		}
		break;
	case V4L2_PIX_FMT_NV16:
		frame->size[0] = sizes[0] =
			frame->size[1] = sizes[1] = y_size;

		frame->stride[0] = frame->stride[1] = y_stride;
		break;

	case V4L2_PIX_FMT_NV21:
		frame->size[0] = sizes[0] = y_size;
		frame->size[1] = y_stride * ALIGN(frame->height >> 1, 16);

		frame->stride[0] = y_stride;
		frame->stride[1] = y_stride;
		break;

	case V4L2_PIX_FMT_YUV422P:
		frame->size[0] = sizes[0] = y_size;
		frame->size[1] = frame->size[2] = sizes[1] = sizes[2] =
			y_size >> 1;

		frame->stride[0] = y_stride;
		frame->stride[1] = frame->stride[2] = ALIGN(y_stride >> 1, 16);
		break;

	case V4L2_PIX_FMT_YUV444:
		frame->size[0] = frame->size[1] = frame->size[2] = sizes[0] =
			sizes[1] = sizes[2] = y_size;
		frame->stride[0] = frame->stride[1] = frame->stride[2] =
			y_stride;
		break;

	case V4L2_PIX_FMT_RGB565:
		frame->size[0] = sizes[0] = (frame->width * frame->height) << 1;
		frame->stride[0] = frame->width << 1;
		break;

	case V4L2_PIX_FMT_RGB32:
		frame->size[0] = sizes[0] = (frame->width * frame->height) << 2;
		frame->stride[0] = (frame->width) << 2;
		break;

	default:
		pr_err("[nx video] unknown format(%d)\n",
		       frame->format.pixelformat);
		return -EINVAL;
	}

	return 0;
}

static int set_plane_size_mmap(struct nx_video_frame *frame,
			       unsigned int sizes[])
{
	u32 y_stride = ALIGN(frame->width, 32);
	u32 y_size = y_stride * frame->height;

	switch (frame->format.pixelformat) {
	case V4L2_PIX_FMT_YUYV:
		frame->size[0] = y_size << 1;
		frame->stride[0] = y_stride << 1;

		sizes[0] = frame->size[0];
		break;

	case V4L2_PIX_FMT_YUV420:
		frame->size[0] = y_size;
		frame->size[1] = frame->size[2] =
			ALIGN(y_stride >> 1, 16) * (frame->height >> 1);
		frame->stride[0] = y_stride;
		frame->stride[1] = frame->stride[2] = ALIGN(y_stride >> 1, 16);

		sizes[0] = frame->size[0];
		sizes[0] += frame->size[1] * 2;
		break;

	case V4L2_PIX_FMT_NV16:
		frame->size[0] = frame->size[1] = y_size;
		frame->stride[0] = frame->stride[1] = y_stride;

		sizes[0] = frame->size[0] * 2;
		break;

	case V4L2_PIX_FMT_NV21:
		frame->size[0] = y_size;
		frame->size[1] = y_stride * (frame->height >> 1);
		frame->stride[0] = y_stride;
		frame->stride[1] = y_stride;

		sizes[0] = frame->size[0];
		sizes[0] += frame->size[1];
		break;

	case V4L2_PIX_FMT_YUV422P:
		frame->size[0] = y_size;
		frame->size[1] = frame->size[2] = y_size >> 1;
		frame->stride[0] = y_stride;
		frame->stride[1] = frame->stride[2] = ALIGN(y_stride >> 1, 16);

		sizes[0] = frame->size[0];
		sizes[0] += frame->size[1] * 2;
		break;

	case V4L2_PIX_FMT_YUV444:
		frame->size[0] = frame->size[1] = frame->size[2] = y_size;
		frame->stride[0] = frame->stride[1] = frame->stride[2] =
			y_stride;

		sizes[0] = frame->size[0] * 3;
		break;

	default:
		pr_err("[nx video] unknown format(%d)\n",
		       frame->format.pixelformat);
		return -EINVAL;
	}

	return 0;
}

static struct v4l2_subdev *get_remote_subdev(struct nx_video *me, u32 type,
					     u32 *pad)
{
	struct media_pad *remote;

	if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE ||
	    type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
		remote = media_entity_remote_pad(&me->pads[1]);
	else
		remote = media_entity_remote_pad(&me->pads[0]);

	if (!remote ||
	    media_entity_type(remote->entity) != MEDIA_ENT_T_V4L2_SUBDEV)
		return NULL;

	if (pad)
		*pad = remote->index;

	return media_entity_to_v4l2_subdev(remote->entity);
}

static void fill_nx_video_buffer(struct nx_video_buffer *buf,
				 struct vb2_buffer *vb)
{
	int i;
	u32 type = vb->vb2_queue->type;
	struct nx_video *me = vb->vb2_queue->drv_priv;
	struct nx_video_frame *frame;
	bool is_separated;

	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		frame = &me->frame[0];
	else
		frame = &me->frame[1];

	is_separated = frame->format.is_separated;
	for (i = 0; i < frame->format.num_sw_planes; i++) {
		if (i == 0 || is_separated) {
			buf->dma_addr[i] = vb2_dma_contig_plane_dma_addr(vb, i);
			if ((frame->format.pixelformat == V4L2_PIX_FMT_YVU420)
			    && (!is_separated)) {
				buf->dma_addr[i+1] =
					buf->dma_addr[i] + frame->size[i]
					+ frame->size[i+1];
				buf->dma_addr[i+2] =
					buf->dma_addr[i] + frame->size[i];
			}
		} else {
			if (frame->format.pixelformat != V4L2_PIX_FMT_YVU420)
				buf->dma_addr[i] =
					buf->dma_addr[i-1] + frame->size[i-1];
		}
		if (!buf->dma_addr[i])
			BUG();
		buf->stride[i] = frame->stride[i];
		pr_debug("[BUF plane %d] addr(0x%x), s(%d)\n",
			i, (int)buf->dma_addr[i], buf->stride[i]);
	}

	buf->consumer_index = 0;
}

static struct nx_buffer_consumer *find_consumer(struct nx_video *me,
						struct list_head *head,
						int index)
{
	int i;
	unsigned long flags;
	struct nx_buffer_consumer *c = NULL;
	struct list_head *cur = head->next;

	spin_lock_irqsave(&me->lock_consumer, flags);
	if (list_empty(head)) {
		spin_unlock_irqrestore(&me->lock_consumer, flags);
		WARN_ON(1);
		return NULL;
	}
	for (i = 0; i <= index; i++) {
		c = list_entry(cur, struct nx_buffer_consumer, list);
		cur = cur->next;
	}
	spin_unlock_irqrestore(&me->lock_consumer, flags);

	return c;
}

/*
 * consumer call this
 */
static int buffer_done(struct nx_video_buffer *buf)
{
	struct vb2_buffer *vb = buf->priv;
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct nx_video *me = vb->vb2_queue->drv_priv;
	int consumer_count;
	struct list_head *cl;
	u32 ci = buf->consumer_index;
	u32 type = vb->vb2_queue->type;

	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		consumer_count = me->sink_consumer_count;
		cl = &me->sink_consumer_list;
	} else {
		consumer_count = me->source_consumer_count;
		cl = &me->source_consumer_list;
	}

	pr_debug("%s: type(0x%x), ci(%d), count(%d)\n",
		 __func__, type, ci, consumer_count);

	if (ci >= consumer_count) {
		v4l2_get_timestamp(&vbuf->timestamp);
		vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
	} else {
		struct nx_buffer_consumer *nc = find_consumer(me, cl, ci);

		nc->queue(buf, nc->priv);
	}

	return 0;
}

/*
 * vb2_ops
 */

/*
 * queue_setup() called from vb2_reqbufs()
 * setup plane number, plane size
 */
static int nx_vb2_queue_setup(struct vb2_queue *q,
			      const void *parg,
			      unsigned int *num_buffers,
			      unsigned int *num_planes,
			      unsigned int sizes[], void *alloc_ctxs[])
{
	int ret;
	int i;
	struct nx_video *me = q->drv_priv;
	struct nx_video_frame *frame = NULL;

	if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		frame = &me->frame[0];
	else
		frame = &me->frame[1];

	if (!frame) {
		pr_err("[nx video] can't find frame for q type(0x%x)\n",
		       q->type);
		return -EINVAL;
	}

	if (q->memory == V4L2_MEMORY_MMAP) {
		ret = set_plane_size_mmap(frame, sizes);
		if (ret < 0) {
			pr_err("[nx video] failed to set_plane_size_mmap()\n");
			return ret;
		}
	} else {
		ret = set_plane_size(frame, sizes);
		if (ret < 0) {
			pr_err("[nx video] failed to set_plane_size()\n");
			return ret;
		}
	}

	*num_planes = (unsigned int)(frame->format.num_planes);

	for (i = 0; i < *num_planes; ++i)
		alloc_ctxs[i] = me->vb2_alloc_ctx;

	return 0;
}

static int nx_vb2_buf_init(struct vb2_buffer *vb)
{
	struct nx_video_buffer **bufs;
	struct nx_video_buffer *buf;
	struct nx_video *me = vb->vb2_queue->drv_priv;
	int index = vb->index;
	u32 type = vb->vb2_queue->type;

	pr_debug("%s: type(0x%x)\n", __func__, type);

	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		bufs = me->sink_bufs;
	} else if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE ||
		   type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		bufs = me->source_bufs;
	} else {
		pr_err("[nx video] invalid buffer type(0x%x)\n", type);
		return -EINVAL;
	}

	if (!bufs[index]) {
		buf = kzalloc(sizeof(*buf), GFP_KERNEL);
		if (!buf) {
			WARN_ON(1);
			return -ENOMEM;
		}
		buf->priv = vb;
		buf->cb_buf_done = buffer_done;
		bufs[index]  = buf;
	}

	return 0;
}

static void nx_vb2_buf_cleanup(struct vb2_buffer *vb)
{
	struct nx_video_buffer **bufs;
	struct nx_video_buffer *buf;
	struct nx_video *me = vb->vb2_queue->drv_priv;
	int index = vb->index;
	u32 type = vb->vb2_queue->type;

	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		bufs = me->sink_bufs;
	} else if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE ||
		   type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		bufs = me->source_bufs;
	} else {
		pr_err("[nx video] invalid buffer type(0x%x)\n", type);
		return;
	}

	buf = bufs[index];
	kfree(buf);
	bufs[index] = NULL;
}

/* real queue */
static void nx_vb2_buf_queue(struct vb2_buffer *vb)
{
	struct nx_video *me = vb->vb2_queue->drv_priv;
	struct nx_video_buffer *buf;
	struct nx_buffer_consumer *c;
	u32 type = vb->vb2_queue->type;

	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		buf = me->sink_bufs[vb->index];
		c = find_consumer(me, &me->sink_consumer_list, 0);
	} else if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE ||
		   type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		buf = me->source_bufs[vb->index];
		c = find_consumer(me, &me->source_consumer_list, 0);
	} else {
		pr_err("[nx video] invalid buffer type(0x%x)\n", type);
		return;
	}

	if (!buf || !c) {
		pr_err("[nx video] no consumer or no buf(%p,%p)\n", buf, c);
		WARN_ON(1);
		return;
	}

	fill_nx_video_buffer(buf, vb);

	pr_debug("%s buf(%p)\n", __func__, buf);
	c->queue(buf, c->priv);
}

static void nx_vb2_buf_finish(struct vb2_buffer *vb)
{
	struct vb2_queue *q;

	q = vb->vb2_queue;
	if (q->memory == V4L2_MEMORY_MMAP)
		vb->planes[0].bytesused = vb->planes[0].length;
}

static int nx_vb2_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct nx_video *me = q->drv_priv;
	struct v4l2_subdev *sd;
	u32 pad;

	sd = get_remote_subdev(me,
			       me->type == NX_VIDEO_TYPE_OUT ?
			       V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE :
			       V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
			       &pad);

	if (sd)
		return v4l2_subdev_call(sd, video, s_stream, 1);
	else
		return -EINVAL;
}

static void nx_vb2_stop_streaming(struct vb2_queue *q)
{
	struct nx_video *me = q->drv_priv;
	struct v4l2_subdev *sd;
	u32 pad;

	sd = get_remote_subdev(me,
			       me->type == NX_VIDEO_TYPE_OUT ?
			       V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE :
			       V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
			       &pad);
	if (sd)
		v4l2_subdev_call(sd, video, s_stream, 0);
}

static struct vb2_ops nx_vb2_ops = {
	.queue_setup    = nx_vb2_queue_setup,
	.buf_init       = nx_vb2_buf_init,
	.buf_cleanup    = nx_vb2_buf_cleanup,
	.buf_queue      = nx_vb2_buf_queue,
	.buf_finish	= nx_vb2_buf_finish,
	.start_streaming = nx_vb2_start_streaming,
	.stop_streaming	= nx_vb2_stop_streaming,
};

static int nx_video_vbq_init(struct nx_video *me, uint32_t type)
{
	struct vb2_queue *vbq = me->vbq;

	vbq->type = type;
	vbq->io_modes = VB2_DMABUF | VB2_MMAP;
	vbq->drv_priv = me;
	vbq->ops      = &nx_vb2_ops;
	vbq->mem_ops  = &vb2_dma_contig_memops;

	vbq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;

	return vb2_queue_init(vbq);
}

/*
 * v4l2_ioctl_ops
 */

/* querycap: check capture, out, m2m */
static int nx_video_querycap(struct file *file, void *fh,
			     struct v4l2_capability *cap)
{
	struct nx_video *me = file->private_data;

	strlcpy(cap->driver, me->name, sizeof(cap->driver));
	strlcpy(cap->card, me->vdev.name, sizeof(cap->card));
	strlcpy(cap->bus_info, "media", sizeof(cap->bus_info));
	cap->version = KERNEL_VERSION(1, 0, 0);

	switch (me->type) {
	case NX_VIDEO_TYPE_CAPTURE:
		cap->device_caps = V4L2_CAP_VIDEO_CAPTURE |
			V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_STREAMING;
		break;
	case NX_VIDEO_TYPE_OUT:
		cap->device_caps =
			V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_STREAMING;
		break;
	case NX_VIDEO_TYPE_M2M:
		cap->device_caps = V4L2_CAP_VIDEO_CAPTURE_MPLANE |
			V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_STREAMING;
		break;
	default:
		pr_err("[nx video] querycap: invalid type(%d)\n", me->type);
		return -EINVAL;
	}

	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static int nx_video_enum_format(struct file *file, void *fh,
				struct v4l2_fmtdesc *f)
{
	if (f->index >= ARRAY_SIZE(supported_formats))
		return -EINVAL;

	strlcpy(f->description, supported_formats[f->index].name,
		sizeof(f->description));
	f->pixelformat = supported_formats[f->index].pixelformat;
	return 0;
}

static int nx_video_get_format(struct file *file, void *fh,
			       struct v4l2_format *f)
{
	/* TODO */
	pr_debug("%s\n", __func__);
	return 0;
}

static int nx_video_set_format(struct file *file, void *fh,
			       struct v4l2_format *f)
{
	struct nx_video *me = file->private_data;
	struct nx_video_format *format;
	struct v4l2_subdev_format mbus_fmt;
	struct v4l2_subdev *subdev;
	struct nx_video_frame *frame;
	u32 pad;
	int ret;
	int i, j;
	u32 width, height, pixelformat, colorspace, field;

	if (me->vbq->type != f->type) {
		vb2_queue_release(me->vbq);
		nx_video_vbq_init(me, f->type);
	}

	if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		frame = &me->frame[0];
	} else if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE ||
		   f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		frame = &me->frame[1];
	} else {
		pr_err("[nx video] set format: invalid type(0x%x)\n", f->type);
		return -EINVAL;
	}

	if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		width = f->fmt.pix_mp.width;
		height = f->fmt.pix_mp.height;
		pixelformat = f->fmt.pix_mp.pixelformat;
		colorspace = f->fmt.pix_mp.colorspace;
		field = f->fmt.pix_mp.field;
	} else {
		width = f->fmt.pix.width;
		height = f->fmt.pix.height;
		pixelformat = f->fmt.pix.pixelformat;
		colorspace = f->fmt.pix.colorspace;
		field = f->fmt.pix.field;
	}

	format = find_format(pixelformat, 0);
	if (!format) {
		pr_err("[nx video] unsupported format 0x%x\n", pixelformat);
		return -EINVAL;
	}

	subdev = get_remote_subdev(me, f->type, &pad);
	if (!subdev) {
		WARN_ON(1);
		pr_err("[nx video] can't get remote source subdev\n");
		return -EINVAL;
	}

	memset(&mbus_fmt, 0, sizeof(mbus_fmt));
	/* pad must be 1 : set_fmt from nx video device */
	/* mbus_fmt.pad = pad; */
	mbus_fmt.pad = 1;
	mbus_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	mbus_fmt.format.width  = width;
	mbus_fmt.format.height = height;
	mbus_fmt.format.code   = format->mbus_code;
	mbus_fmt.format.colorspace = colorspace;
	mbus_fmt.format.field  = field;

	/* call to subdev */
	ret = v4l2_subdev_call(subdev, pad, set_fmt, NULL, &mbus_fmt);
	if (ret < 0) {
		pr_err("[nx video] failed to subdev set_fmt()\n");
		return ret;
	}

	frame->format.name = format->name;
	frame->format.pixelformat = format->pixelformat;
	frame->format.mbus_code   = format->mbus_code;
	frame->format.num_planes  = format->num_planes;
	frame->format.num_sw_planes  = format->num_sw_planes;
	frame->format.is_separated  = format->is_separated;
	frame->format.field  = field;
	frame->width  = width;
	frame->height = height;

	if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		struct v4l2_pix_format_mplane *pix = &f->fmt.pix_mp;

		for (i = 0; i < format->num_planes; ++i) {
			if (format->pixelformat == V4L2_PIX_FMT_YVU420) {
				for (j = 0; j < format->num_sw_planes; j++) {
					frame->stride[j] =
						pix->plane_fmt[j].bytesperline;
					frame->size[j] =
						pix->plane_fmt[j].sizeimage;
					pr_debug("stride[%d]=%d, size[%d]=%d\n",
						 j, frame->stride[j], j,
						 frame->size[j]);
				}
			} else {
				frame->stride[i] =
					pix->plane_fmt[i].bytesperline;
				frame->size[i] =
					pix->plane_fmt[i].sizeimage;
			}
		}
	}

	if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		set_plane_size_mmap(frame, &f->fmt.pix.sizeimage);
		f->fmt.pix.bytesperline = frame->stride[0];
	}

	return 0;
}

static int nx_video_try_format(struct file *file, void *fh,
			       struct v4l2_format *f)
{
	struct nx_video_format *format;
	u32 pixelformat;

	if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		pixelformat = f->fmt.pix_mp.pixelformat;
	else
		pixelformat = f->fmt.pix.pixelformat;

	format = find_format(pixelformat, 0);

	if (format->pixelformat == pixelformat)
		return 0;
	else
		return -EINVAL;
}

static int nx_video_reqbufs(struct file *file, void *fh,
			    struct v4l2_requestbuffers *b)
{
	struct nx_video *me = file->private_data;

	if (me->vbq)
		return vb2_reqbufs(me->vbq, b);
	return -EINVAL;
}

static int nx_video_querybuf(struct file *file, void *fh,
			     struct v4l2_buffer *b)
{
	struct nx_video *me = file->private_data;

	if (me->vbq)
		return vb2_querybuf(me->vbq, b);
	return -EINVAL;
}

static int nx_video_qbuf(struct file *file, void *fh,
			 struct v4l2_buffer *b)
{
	struct nx_video *me = file->private_data;

	if (me->vbq)
		return vb2_qbuf(me->vbq, b);
	return -EINVAL;
}

static int nx_video_dqbuf(struct file *file, void *fh,
			  struct v4l2_buffer *b)
{
	struct nx_video *me = file->private_data;

	if (me->vbq)
		return vb2_dqbuf(me->vbq, b, file->f_flags & O_NONBLOCK);

	return -EINVAL;
}

static int nx_video_streamon(struct file *file, void *fh, enum v4l2_buf_type i)
{
	u32 pad;
	struct nx_video *me = file->private_data;
	struct v4l2_subdev *subdev = get_remote_subdev(me, i, &pad);

	if (!subdev) {
		WARN_ON(1);
		return -ENODEV;
	}

	v4l2_set_subdev_hostdata(subdev, me->name);

	if (me->vbq)
		return vb2_streamon(me->vbq, i);

	return -EINVAL;
}

static int nx_video_streamoff(struct file *file, void *fh, enum v4l2_buf_type i)
{
	u32 pad;
	struct nx_video *me = file->private_data;
	struct v4l2_subdev *subdev = get_remote_subdev(me, i, &pad);

	if (!subdev) {
		pr_err("[nx video] no subdev device linked\n");
		return -ENODEV;
	}

	v4l2_set_subdev_hostdata(subdev, me->name);

	if (me->vbq)
		return vb2_streamoff(me->vbq, i);

	return -EINVAL;
}

static int nx_video_cropcap(struct file *file, void *fh, struct v4l2_cropcap *a)
{
	u32 pad;
	struct nx_video *me = file->private_data;
	struct v4l2_subdev *subdev = get_remote_subdev(me, a->type, &pad);

	if (!subdev) {
		WARN_ON(1);
		return -ENODEV;
	}

	return v4l2_subdev_call(subdev, video, cropcap, a);
}

static int nx_video_get_crop(struct file *file, void *fh, struct v4l2_crop *a)
{
	u32 pad;
	struct nx_video *me = file->private_data;
	struct v4l2_subdev *subdev = get_remote_subdev(me, a->type, &pad);

	if (!subdev) {
		WARN_ON(1);
		return -ENODEV;
	}

	return v4l2_subdev_call(subdev, video, g_crop, a);
}

static int nx_video_set_crop(struct file *file, void *fh,
			     const struct v4l2_crop *a)
{
	u32 pad;
	struct nx_video *me = file->private_data;
	struct v4l2_subdev *subdev = get_remote_subdev(me, a->type, &pad);

	if (!subdev) {
		WARN_ON(1);
		return -ENODEV;
	}

	return v4l2_subdev_call(subdev, video, s_crop, a);
}

static int nx_video_g_ctrl(struct file *file, void *fh,
			   struct v4l2_control *ctrl)
{
	u32 pad;
	u32 type;
	struct nx_video *me;
	struct v4l2_subdev *subdev;

	me = file->private_data;
	type = (me->type == NX_VIDEO_TYPE_CAPTURE) ?
		V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE :
		V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	subdev = get_remote_subdev(me, type, &pad);

	if (!subdev) {
		WARN_ON(1);
		return -ENODEV;
	}

	return v4l2_subdev_call(subdev, core, g_ctrl, ctrl);
}

static int nx_video_s_ctrl(struct file *file, void *fh,
			   struct v4l2_control *ctrl)
{
	u32 pad;
	u32 type;
	struct nx_video *me;
	struct v4l2_subdev *subdev;

	me = file->private_data;
	type = (me->type == NX_VIDEO_TYPE_CAPTURE) ?
		V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE :
		V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	subdev = get_remote_subdev(me, type, &pad);

	if (!subdev) {
		WARN_ON(1);
		return -ENODEV;
	}

	return v4l2_subdev_call(subdev, core, s_ctrl, ctrl);
}

static int nx_video_g_input(struct file *file, void *fh, unsigned int *i)
{
	*i = 0;
	return 0;
}

static int nx_video_s_input(struct file *file, void *fh, unsigned int i)
{
	return 0;
}

static int nx_video_enum_input(struct file *file, void *fh,
			       struct v4l2_input *inp)
{
	struct nx_video *me;

	me = file->private_data;

	if (inp->index != 0)
		return -EINVAL;

	if (me->type == NX_VIDEO_TYPE_CAPTURE) {
		inp->type = V4L2_INPUT_TYPE_CAMERA;
		strcpy(inp->name, "Camera");
		return 0;
	}

	return -EINVAL;
}

static int nx_video_g_parm(struct file *file, void *fh,
			   struct v4l2_streamparm *a)
{
	struct nx_video *me;

	me = file->private_data;

	if (me->type == NX_VIDEO_TYPE_CAPTURE &&
	    a->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		struct v4l2_subdev *subdev;

		subdev = get_remote_subdev(me, a->type, NULL);
		return v4l2_subdev_call(subdev, video, g_parm, a);
	}

	return -EINVAL;
}

static int nx_video_s_parm(struct file *file, void *fh,
			   struct v4l2_streamparm *a)
{
	struct nx_video *me;

	me = file->private_data;

	if (me->type == NX_VIDEO_TYPE_CAPTURE &&
	    ((a->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
	     || (a->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE))) {
		struct v4l2_subdev *subdev;

		subdev = get_remote_subdev(me, a->type, NULL);
		return v4l2_subdev_call(subdev, video, s_parm, a);
	}

	return -EINVAL;
}

static struct v4l2_ioctl_ops nx_video_ioctl_ops = {
	.vidioc_querycap                = nx_video_querycap,
	.vidioc_enum_fmt_vid_cap_mplane = nx_video_enum_format,
	.vidioc_enum_fmt_vid_out_mplane = nx_video_enum_format,
	.vidioc_g_fmt_vid_cap_mplane    = nx_video_get_format,
	.vidioc_g_fmt_vid_out_mplane    = nx_video_get_format,
	.vidioc_s_fmt_vid_cap_mplane    = nx_video_set_format,
	.vidioc_s_fmt_vid_out_mplane    = nx_video_set_format,
	.vidioc_try_fmt_vid_cap_mplane  = nx_video_try_format,
	.vidioc_enum_fmt_vid_cap	= nx_video_enum_format,
	.vidioc_g_fmt_vid_cap		= nx_video_get_format,
	.vidioc_s_fmt_vid_cap		= nx_video_set_format,
	.vidioc_try_fmt_vid_cap		= nx_video_try_format,
	.vidioc_reqbufs                 = nx_video_reqbufs,
	.vidioc_querybuf                = nx_video_querybuf,
	.vidioc_qbuf                    = nx_video_qbuf,
	.vidioc_dqbuf                   = nx_video_dqbuf,
	.vidioc_streamon                = nx_video_streamon,
	.vidioc_streamoff               = nx_video_streamoff,
	.vidioc_cropcap                 = nx_video_cropcap,
	.vidioc_g_crop                  = nx_video_get_crop,
	.vidioc_s_crop                  = nx_video_set_crop,
	.vidioc_g_ctrl                  = nx_video_g_ctrl,
	.vidioc_s_ctrl                  = nx_video_s_ctrl,
	.vidioc_g_input			= nx_video_g_input,
	.vidioc_s_input			= nx_video_s_input,
	.vidioc_enum_input		= nx_video_enum_input,
	.vidioc_g_parm			= nx_video_g_parm,
	.vidioc_s_parm			= nx_video_s_parm,
};

/*
 * v4l2_file_operations
 */
static int nx_video_open(struct file *file)
{
	struct nx_video *me = video_drvdata(file);
	u32 pad;
	struct v4l2_subdev *sd;
	int ret = 0;

	if (me->open_count == 0) {
		memset(me->frame, 0, sizeof(struct nx_video_frame)*2);
		sd = get_remote_subdev(me,
				       me->type == NX_VIDEO_TYPE_OUT ?
				       V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE :
				       V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
				       &pad);
		if (sd)
			v4l2_subdev_call(sd, core, s_power, 1);
	}

	me->open_count++;
	file->private_data = me;

	return ret;
}

static int nx_video_release(struct file *file)
{
	struct nx_video *me = video_drvdata(file);
	u32 pad;
	struct v4l2_subdev *sd;
	int ret = 0;

	me->open_count--;
	if (me->open_count == 0) {
		sd = get_remote_subdev(me,
				       me->type == NX_VIDEO_TYPE_OUT ?
				       V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE :
				       V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
				       &pad);
		if (sd) {
			v4l2_subdev_call(sd, video, s_stream, 0);
			v4l2_subdev_call(sd, core, s_power, 0);
		}

		if (me->vbq)
			vb2_queue_release(me->vbq);

	}

	file->private_data = 0;

	return ret;
}

static int nx_video_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct nx_video *me = file->private_data;

	return vb2_mmap(me->vbq, vma);
}

static unsigned int nx_video_poll(struct file *file,
				  struct poll_table_struct *tbl)
{
	struct nx_video *me = file->private_data;

	return vb2_poll(me->vbq, file, tbl);
}

static struct v4l2_file_operations nx_video_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = video_ioctl2,
	.open           = nx_video_open,
	.release        = nx_video_release,
	.poll           = nx_video_poll,
	.mmap           = nx_video_mmap,
};

/*
 * media_entity_operations
 */
static int nx_video_link_setup(struct media_entity *entity,
			       const struct media_pad *local,
			       const struct media_pad *remote, u32 flags)
{
	switch (local->index | media_entity_type(remote->entity)) {
	case 0 | MEDIA_ENT_T_V4L2_SUBDEV:
		/* capture, m2m : sink
		 * video : source
		 */
		pr_debug("local %d, link subdev\n", local->index);
		break;

	case 0 | MEDIA_ENT_T_DEVNODE:
		pr_debug("local %d, link videodev\n", local->index);
		break;

	case 1 | MEDIA_ENT_T_V4L2_SUBDEV:
		pr_debug("local %d, link subdev\n", local->index);
		break;

	case 1 | MEDIA_ENT_T_DEVNODE:
		pr_debug("local %d, link videodev\n", local->index);
		break;
	}

	if (flags & MEDIA_LNK_FL_ENABLED)
		pr_debug("linked\n");
	else
		pr_debug("unlinked\n");

	return 0;
}

static struct media_entity_operations nx_media_entity_operations = {
	.link_setup = nx_video_link_setup,
};

/**
 * callback functions
 */
static int register_buffer_consumer(struct nx_video *me,
				    struct nx_buffer_consumer *consumer,
				    enum nx_buffer_consumer_type type)
{
	unsigned long flags;

	if (type >= NX_BUFFER_CONSUMER_INVALID) {
		pr_err("[nx video] invalid buffer consumer type: %d\n", type);
		return -EINVAL;
	}

	pr_debug("%s: name(%s) type(0x%x), consumer(%p)\n",
		 __func__, me->name, type, consumer);

	spin_lock_irqsave(&me->lock_consumer, flags);
	if (type == NX_BUFFER_CONSUMER_SINK) {
		list_add_tail(&consumer->list, &me->sink_consumer_list);
		me->sink_consumer_count++;
	} else {
		list_add_tail(&consumer->list, &me->source_consumer_list);
		me->source_consumer_count++;
	}
	spin_unlock_irqrestore(&me->lock_consumer, flags);

	return 0;
}

static void unregister_buffer_consumer(struct nx_video *me,
				       struct nx_buffer_consumer *consumer,
				       enum nx_buffer_consumer_type type)
{
	unsigned long flags;

	if (type >= NX_BUFFER_CONSUMER_INVALID) {
		pr_err("%s: invalid type(%d)\n", __func__, type);
		return;
	}

	pr_debug("%s: type(0x%x), consumer(%p)\n",
		 __func__, type, consumer);

	spin_lock_irqsave(&me->lock_consumer, flags);
	list_del(&consumer->list);
	if (type == NX_BUFFER_CONSUMER_SINK)
		me->sink_consumer_count--;
	else
		me->source_consumer_count--;
	spin_unlock_irqrestore(&me->lock_consumer, flags);
}

/*
 * public functions
 */
struct nx_video *nx_video_create(char *name, u32 type,
				 struct v4l2_device *v4l2_dev,
				 void *vb2_alloc_ctx)
{
	int ret;
	struct vb2_queue *vbq = NULL;
	int pad_num = 0;
	struct nx_video *me = kzalloc(sizeof(*me), GFP_KERNEL);

	if (!me) {
		WARN_ON(1);
		return ERR_PTR(-ENOMEM);
	}

	snprintf(me->name, sizeof(me->name), "%s", name);
	snprintf(me->vdev.name, sizeof(me->vdev.name), "%s", name);

	me->type          = type;
	me->v4l2_dev      = v4l2_dev;
	me->vb2_alloc_ctx = vb2_alloc_ctx;
	me->pads[0].flags = MEDIA_PAD_FL_SINK;
	me->pads[1].flags = MEDIA_PAD_FL_SOURCE;
	pad_num = 2;

	me->vdev.entity.ops = &nx_media_entity_operations;
	ret = media_entity_init(&me->vdev.entity, pad_num, me->pads, 0);
	if (ret < 0) {
		kfree(me);
		pr_err("[nx video] failed to media_entity_init()\n");
		return NULL;
	}

	mutex_init(&me->lock);

	me->register_buffer_consumer = register_buffer_consumer;
	me->unregister_buffer_consumer = unregister_buffer_consumer;

	me->vdev.fops      = &nx_video_fops;
	me->vdev.ioctl_ops = &nx_video_ioctl_ops;
	me->vdev.v4l2_dev  = v4l2_dev;
	me->vdev.minor     = -1;
	me->vdev.vfl_type  = VFL_TYPE_GRABBER;
	me->vdev.release   = video_device_release;
	me->vdev.lock      = &me->lock;

	vbq = kzalloc(sizeof(*vbq), GFP_KERNEL);
	if (!vbq) {
		WARN_ON(1);
		ret = -ENOMEM;
		goto free_me;
	}

	me->vbq = vbq;
	ret = nx_video_vbq_init(me, type == NX_VIDEO_TYPE_CAPTURE ?
				V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE :
				V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	if (ret < 0) {
		pr_err("[nx video] failed to nx_video_vbq_init()\n");
		goto free_vbq;
	}

	INIT_LIST_HEAD(&me->source_consumer_list);
	INIT_LIST_HEAD(&me->sink_consumer_list);
	spin_lock_init(&me->lock_consumer);

	me->vdev.v4l2_dev = me->v4l2_dev;
	ret = video_register_device(&me->vdev, VFL_TYPE_GRABBER,
				    video_device_number);
	if (ret < 0) {
		pr_err("[nx video] failed to video_register_device()\n");
		goto free_vbq;
	}

	vb2_dma_contig_enable_cached_mmap(me->vb2_alloc_ctx, true);

	video_set_drvdata(&me->vdev, me);

	video_device_number++;

	return me;

free_vbq:
	kfree(vbq);
free_me:
	kfree(me);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(nx_video_create);

void nx_video_cleanup(struct nx_video *me)
{
	video_unregister_device(&me->vdev);

	mutex_destroy(&me->lock);
	media_entity_cleanup(&me->vdev.entity);
	if (me->vbq) {
		vb2_queue_release(me->vbq);
		kfree(me->vbq);
	}
	kfree(me);
}
EXPORT_SYMBOL_GPL(nx_video_cleanup);

int nx_video_get_buffer_count(struct nx_video_buffer_object *obj)
{
	return atomic_read(&obj->buffer_count);
}
EXPORT_SYMBOL_GPL(nx_video_get_buffer_count);

struct nx_video_buffer *
nx_video_get_next_buffer(struct nx_video_buffer_object *obj, bool remove)
{
	unsigned long flags;
	struct nx_video_buffer *buf = NULL;

	spin_lock_irqsave(&obj->slock, flags);
	if (!list_empty(&obj->buffer_list)) {
		buf = list_first_entry(&obj->buffer_list,
				       struct nx_video_buffer, list);
		if (remove) {
			list_del_init(&buf->list);
			atomic_dec(&obj->buffer_count);
		}
	}
	spin_unlock_irqrestore(&obj->slock, flags);

	return buf;
}
EXPORT_SYMBOL_GPL(nx_video_get_next_buffer);

bool nx_video_done_buffer(struct nx_video_buffer_object *obj)
{
	struct nx_video_buffer *done_buf;

	if (nx_video_get_buffer_count(obj) == 1) {
		pr_debug("%s: only 1 buffer\n", __func__);
		return false;
	}

	done_buf = nx_video_get_next_buffer(obj, true);
	if (!done_buf)
		return false;

	if (done_buf->cb_buf_done) {
		done_buf->consumer_index++;
		done_buf->cb_buf_done(done_buf);
	}

	return true;
}
EXPORT_SYMBOL_GPL(nx_video_done_buffer);

void nx_video_clear_buffer(struct nx_video_buffer_object *obj)
{
	struct nx_video_buffer *buf = NULL;

	if (nx_video_get_buffer_count(obj)) {
		unsigned long flags;

		spin_lock_irqsave(&obj->slock, flags);
		while (!list_empty(&obj->buffer_list)) {
			buf = list_entry(obj->buffer_list.next,
					 struct nx_video_buffer, list);
			if (buf) {
				struct vb2_buffer *vb = buf->priv;
				vb2_buffer_done(vb, VB2_BUF_STATE_ERROR);
				list_del_init(&buf->list);
			} else
				break;
		}
		INIT_LIST_HEAD(&obj->buffer_list);
		spin_unlock_irqrestore(&obj->slock, flags);
	}

	atomic_set(&obj->buffer_count, 0);

}
EXPORT_SYMBOL_GPL(nx_video_clear_buffer);

void nx_video_init_vbuf_obj(struct nx_video_buffer_object *obj)
{
	spin_lock_init(&obj->slock);
	INIT_LIST_HEAD(&obj->buffer_list);
	atomic_set(&obj->buffer_count, 0);
}
EXPORT_SYMBOL_GPL(nx_video_init_vbuf_obj);

void nx_video_add_buffer(struct nx_video_buffer_object *obj,
			 struct nx_video_buffer *buf)
{
	unsigned long flags;


	spin_lock_irqsave(&obj->slock, flags);
	list_add_tail(&buf->list, &obj->buffer_list);
	spin_unlock_irqrestore(&obj->slock, flags);
	atomic_inc(&obj->buffer_count);
}
EXPORT_SYMBOL_GPL(nx_video_add_buffer);

int nx_video_register_buffer_consumer(struct nx_video_buffer_object *obj,
				      nx_queue_func func,
				      void *data)
{
	struct nx_buffer_consumer *consumer = obj->consumer;

	if (!consumer) {
		consumer = kzalloc(sizeof(*consumer), GFP_KERNEL);
		if (!consumer) {
			WARN_ON(1);
			return -ENOMEM;
		}
		obj->consumer = consumer;
	}

	consumer->priv = data;
	consumer->queue = func;
	return obj->video->register_buffer_consumer(obj->video, consumer,
						   NX_BUFFER_CONSUMER_SINK);
}
EXPORT_SYMBOL_GPL(nx_video_register_buffer_consumer);

void nx_video_unregister_buffer_consumer(struct nx_video_buffer_object *obj)
{
	if (obj->consumer) {
		obj->video->unregister_buffer_consumer(obj->video,
						       obj->consumer,
						       NX_BUFFER_CONSUMER_SINK);
		kfree(obj->consumer);
		obj->consumer = NULL;
	}
}
EXPORT_SYMBOL_GPL(nx_video_unregister_buffer_consumer);
