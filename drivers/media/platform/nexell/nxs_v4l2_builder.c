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

#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>
#include <linux/v4l2-mediabus.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>

#include <media/media-device.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ctrls.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-mem2mem.h>

#include <linux/nxs_ioctl.h>
#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_res_manager.h>

#define NXS_VIDEO_MAX_NAME_SIZE	32
#define NXS_VIDEO_CAPTURE_DEF_FRAMERATE	30
#define NXS_VIDEO_RENDER_DEF_FRAMERATE	60
#define NXS_VIDEO_MAX_PLANES	3

enum nxs_video_type {
	NXS_VIDEO_TYPE_NONE = 0,
	NXS_VIDEO_TYPE_CAPTURE,
	NXS_VIDEO_TYPE_RENDER,
	NXS_VIDEO_TYPE_M2M,
	NXS_VIDEO_TYPE_SUBDEV,
	NXS_VIDEO_TYPE_INVALID
};

struct nxs_video {
	char name[NXS_VIDEO_MAX_NAME_SIZE];
	u32 type; /* enum nxs_video_type */
	int open_count;

	struct mutex queue_lock;
	struct mutex stream_lock;

	struct v4l2_device *v4l2_dev;
	struct media_device *media_dev;
	struct vb2_queue *vbq;
	void *vb2_alloc_ctx;

	struct video_device vdev;

	struct nxs_function_instance *nxs_function;
};

#define vdev_to_nxs_video(vdev) container_of(vdev, struct nxs_video, video)
#define vbq_to_nxs_video(vbq)	container_of(vbq, struct nxs_video, vbq)

struct nxs_video_fh {
	struct v4l2_fh vfh;
	struct nxs_video *video;
	struct vb2_queue queue;
	struct v4l2_format format;
	struct v4l2_crop crop;
	struct v4l2_selection selection;
	struct v4l2_streamparm parm;
	/* m2m */
	struct v4l2_m2m_ctx *m2m_ctx;
	struct v4l2_m2m_dev *m2m_dev;
	unsigned int num_planes;
	unsigned int strides[NXS_VIDEO_MAX_PLANES];
	unsigned int sizes[NXS_VIDEO_MAX_PLANES];
	unsigned int width;
	unsigned int height;
	unsigned int pixelformat;
	struct list_head bufq;
	spinlock_t bufq_lock;
	atomic_t underflow;
	u32 type; /* enum v4l2_buf_type */
};

#define to_nxs_video_fh(fh)	container_of(fh, struct nxs_video_fh, vfh)
#define queue_to_nxs_video_fh(q) \
				container_of(q, struct nxs_video_fh, queue)

struct nxs_video_buffer {
	struct list_head list;
	struct vb2_v4l2_buffer vb;
	dma_addr_t dma_addr[NXS_VIDEO_MAX_PLANES];
	unsigned int strides[NXS_VIDEO_MAX_PLANES];
};

#define to_nxs_video_buffer(buf) container_of(buf, struct nxs_video_buffer, vb)

struct nxs_v4l2_builder {
	struct nxs_function_builder *builder;

	struct device *dev;

	struct mutex lock; /* protect function_list */
	struct list_head function_list;

	struct media_device media_dev;
	struct v4l2_device v4l2_dev;
	void  *vb2_alloc_ctx;

	int seq;
};

struct nxs_video_format {
	u32   format;
	u32   bpp;
	char *name;
};

/* name field is same to videodev2.h format comments */
static struct nxs_video_format supported_formats[] = {
	{
		.format		= V4L2_PIX_FMT_ARGB555,
		.bpp		= 16,
		.name		= "ARGB-1-5-5-5 16bit",
	},
	{
		.format		= V4L2_PIX_FMT_XRGB555,
		.bpp		= 16,
		.name		= "XRGB-1-5-5-5",
	},
	{
		.format		= V4L2_PIX_FMT_RGB565,
		.bpp		= 16,
		.name		= "RGB-5-6-5",
	},
	{
		.format		= V4L2_PIX_FMT_BGR24,
		.bpp		= 24,
		.name		= "BGR-8-8-8",
	},
	{
		.format		= V4L2_PIX_FMT_RGB24,
		.bpp		= 24,
		.name		= "RGB-8-8-8",
	},
	{
		.format		= V4L2_PIX_FMT_BGR32,
		.bpp		= 32,
		.name		= "BGR-8-8-8-8",
	},
	{
		.format		= V4L2_PIX_FMT_ABGR32,
		.bpp		= 32,
		.name		= "BGRA-8-8-8-8",
	},
	{
		.format		= V4L2_PIX_FMT_XBGR32,
		.bpp		= 32,
		.name		= "BGRX-8-8-8-8",
	},
	{
		.format		= V4L2_PIX_FMT_RGB32,
		.bpp		= 32,
		.name		= "RGB-8-8-8-8",
	},
	{
		.format		= V4L2_PIX_FMT_ARGB32,
		.bpp		= 32,
		.name		= "ARGB-8-8-8-8",
	},
	{
		.format		= V4L2_PIX_FMT_XRGB32,
		.bpp		= 32,
		.name		= "XRGB-8-8-8-8",
	},
	{
		.format		= V4L2_PIX_FMT_YUYV,
		.bpp		= 16,
		.name		= "YUV 4:2:2",
	},
	{
		.format		= V4L2_PIX_FMT_YYUV,
		.bpp		= 16,
		.name		= "YUV 4:2:2",
	},
	{
		.format		= V4L2_PIX_FMT_YVYU,
		.bpp		= 16,
		.name		= "YVU 4:2:2",
	},
	{
		.format		= V4L2_PIX_FMT_UYVY,
		.bpp		= 16,
		.name		= "YUV 4:2:2",
	},
	{
		.format		= V4L2_PIX_FMT_VYUY,
		.bpp		= 16,
		.name		= "YUV 4:2:2",
	},
};

static struct nxs_video_format supported_mplane_formats[] = {
	{
		.format		= V4L2_PIX_FMT_YVU420,
		.bpp		= 12,
		.name		= "YVU 4:2:0",
	},
	{
		.format		= V4L2_PIX_FMT_YUV422P,
		.bpp		= 16,
		.name		= "YVU422 planar",
	},
	{
		.format		= V4L2_PIX_FMT_YUV420,
		.bpp		= 12,
		.name		= "YUV 4:2:0",
	},
	{
		.format		= V4L2_PIX_FMT_NV12,
		.bpp		= 12,
		.name		= "Y/CbCr 4:2:0",
	},
	{
		.format		= V4L2_PIX_FMT_NV21,
		.bpp		= 12,
		.name		= "Y/CrCb 4:2:0",
	},
	{
		.format		= V4L2_PIX_FMT_NV16,
		.bpp		= 16,
		.name		= "Y/CbCr 4:2:2",
	},
	{
		.format		= V4L2_PIX_FMT_NV61,
		.bpp		= 16,
		.name		= "Y/CrCb 4:2:2",
	},
	{
		.format		= V4L2_PIX_FMT_NV24,
		.bpp		= 24,
		.name		= "Y/CbCr 4:4:4",
	},
	{
		.format		= V4L2_PIX_FMT_NV42,
		.bpp		= 24,
		.name		= "Y/CrCb 4:4:4",
	},
	{
		.format		= V4L2_PIX_FMT_NV12M,
		.bpp		= 12,
		.name		= "Y/CbCr 4:2:0",
	},
	{
		.format		= V4L2_PIX_FMT_NV21M,
		.bpp		= 12,
		.name		= "Y/CrCb 4:2:0",
	},
	{
		.format		= V4L2_PIX_FMT_NV16M,
		.bpp		= 16,
		.name		= "Y/CbCr 4:2:2",
	},
	{
		.format		= V4L2_PIX_FMT_NV61M,
		.bpp		= 16,
		.name		= "Y/CrCb 4:2:2",
	},
	{
		.format		= V4L2_PIX_FMT_YUV420M,
		.bpp		= 12,
		.name		= "YUV420 planar",
	},
	{
		.format		= V4L2_PIX_FMT_YVU420M,
		.bpp		= 12,
		.name		= "YVU420 planar",
	},
};

/*
 * util functions
 */
static int nxs_vbq_init(struct nxs_video_fh *vfh, u32 type);

static inline bool is_multiplanar(u32 buf_type)
{
	switch (buf_type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		return false;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		return true;
	}

	return false;
}

static void get_info_of_format(struct v4l2_format *f,
			       unsigned int *num_planes,
			       unsigned int strides[],
			       unsigned int sizes[],
			       unsigned int *width,
			       unsigned int *height,
			       unsigned int *pixelformat)
{
	/* TODO: check app source set_format routine
	 * bytesperline, sizeimage, num_planes, v4l2_plane_pix_format
	 */
	if (is_multiplanar(f->type)) {
		int i;

		*num_planes = f->fmt.pix_mp.num_planes;
		*width = f->fmt.pix_mp.width;
		*height = f->fmt.pix_mp.height;
		*pixelformat = f->fmt.pix_mp.pixelformat;

		for (i = 0; i < *num_planes; i++) {
			strides[i] = f->fmt.pix_mp.plane_fmt[i].bytesperline;
			sizes[i] = f->fmt.pix_mp.plane_fmt[i].sizeimage;
		}
	} else {
		*num_planes = 1;
		*width = f->fmt.pix.width;
		*height = f->fmt.pix.height;
		*pixelformat = f->fmt.pix.pixelformat;

		strides[0] = f->fmt.pix.bytesperline;
		sizes[0] = f->fmt.pix.sizeimage;
	}
}

static struct nxs_video_buffer *get_next_video_buffer(struct nxs_video_fh *vfh,
						      bool remove)
{
	struct nxs_video_buffer *buffer;
	unsigned long flags;

	spin_lock_irqsave(&vfh->bufq_lock, flags);
	if (list_empty(&vfh->bufq)) {
		spin_unlock_irqrestore(&vfh->bufq_lock, flags);
		return NULL;
	}

	buffer = list_first_entry(&vfh->bufq, struct nxs_video_buffer, list);
	if (remove)
		list_del(&buffer->list);
	spin_unlock_irqrestore(&vfh->bufq_lock, flags);

	return buffer;
}

/* chain functions */
static void nxs_video_irqcallback(struct nxs_dev *pthis, void *data);

static int nxs_chain_register_irqcallback(struct nxs_function_instance *inst,
					  struct nxs_video_fh *vfh)
{
	struct nxs_dev *last;
	struct nxs_irq_callback *callback;

	last = list_last_entry(&inst->dev_list, struct nxs_dev, func_list);
	if (!last)
		BUG();

	callback = kzalloc(sizeof(*callback), GFP_KERNEL);
	if (!callback)
		return -ENOMEM;

	callback->handler = nxs_video_irqcallback;
	callback->data = vfh;

	return nxs_dev_register_irq_callback(last, NXS_DEV_IRQCALLBACK_TYPE_IRQ,
					     callback);
}

static int nxs_chain_unregister_irqcallback(struct nxs_function_instance *inst)
{
	struct nxs_dev *last;

	last = list_last_entry(&inst->dev_list, struct nxs_dev, func_list);
	if (!last)
		BUG();

	return nxs_dev_unregister_irq_callback(last,
					       NXS_DEV_IRQCALLBACK_TYPE_IRQ);
}

static int nxs_chain_config(struct nxs_function_instance *inst,
			    struct nxs_video_fh *vfh)
{
	struct nxs_dev *nxs_dev;
	union nxs_control f;
	union nxs_control c;
	union nxs_control s;
	/* union nxs_control t; */
	int ret;

	/* format */
	f.format.width = vfh->width;
	f.format.height = vfh->height;
	f.format.pixelformat = vfh->pixelformat;

	/* crop */
	c.crop.l = vfh->crop.c.left;
	c.crop.t = vfh->crop.c.top;
	c.crop.w = vfh->crop.c.width;
	c.crop.h = vfh->crop.c.height;

	/* selection */
	s.sel.l = vfh->selection.r.left;
	s.sel.t = vfh->selection.r.top;
	s.sel.w = vfh->selection.r.width;
	s.sel.h = vfh->selection.r.height;

	/* timeperframe */
	/* TODO */
	/* if (vfh->video->type == NXS_VIDEO_TYPE_CAPTURE) { */
	/* } else if (vfh->video->type == NXS_VIDEO_TYPE_RENDER) { */
	/* } */

	list_for_each_entry_reverse(nxs_dev, &inst->dev_list, func_list) {
		ret = nxs_set_control(nxs_dev, NXS_CONTROL_FORMAT, &f);
		if (ret)
			return ret;

		ret = nxs_set_control(nxs_dev, NXS_CONTROL_CROP, &c);
		if (ret)
			return ret;

		ret = nxs_set_control(nxs_dev, NXS_CONTROL_SELECTION, &s);
		if (ret)
			return ret;
	}

	return 0;
}

static int nxs_dev_set_buffer(struct nxs_dev *nxs_dev,
			      struct nxs_video_fh *vfh,
			      struct nxs_video_buffer *buffer)
{
	union nxs_control control;
	int i;

	control.buffer.num_planes = vfh->num_planes;
	for (i = 0; i < vfh->num_planes; i++) {
		control.buffer.address[i] = buffer->dma_addr[i];
		control.buffer.strides[i] = buffer->strides[i];
	}

	return nxs_set_control(nxs_dev, NXS_CONTROL_BUFFER, &control);
}

static int nxs_chain_set_buffer(struct nxs_function_instance *inst,
				struct nxs_video_fh *vfh,
				struct nxs_video_buffer *buffer)
{
	struct nxs_dev *nxs_dev = NULL;
	struct nxs_video *video = vfh->video;

	switch (video->type) {
	case NXS_VIDEO_TYPE_CAPTURE:
		nxs_dev = list_last_entry(&inst->dev_list, struct nxs_dev,
					  func_list);
		break;
	case NXS_VIDEO_TYPE_RENDER:
		nxs_dev = list_first_entry(&inst->dev_list, struct nxs_dev,
					   func_list);
		break;
	case NXS_VIDEO_TYPE_M2M:
		/* TODO */
		break;
	default:
		dev_err(&video->vdev.dev, "%s: Not supported type(0x%x)\n",
			__func__, video->type);
		return -EINVAL;
	}

	if (nxs_dev)
		return nxs_dev_set_buffer(nxs_dev, vfh, buffer);

	return 0;
}

static int nxs_chain_run(struct nxs_function_instance *inst)
{
	struct nxs_dev *nxs_dev;
	int ret;

	list_for_each_entry_reverse(nxs_dev, &inst->dev_list, func_list) {
		if (nxs_dev->start) {
			ret = nxs_dev->start(nxs_dev);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static void nxs_chain_stop(struct nxs_function_instance *inst)
{
	struct nxs_dev *nxs_dev;

	list_for_each_entry_reverse(nxs_dev, &inst->dev_list, func_list) {
		if (nxs_dev->stop)
			nxs_dev->stop(nxs_dev);
	}
}

/* irq callback */
static void nxs_video_irqcallback(struct nxs_dev *nxs_dev, void *data)
{
	struct nxs_video_fh *vfh;
	struct nxs_video *video;
	struct nxs_video_buffer *done_buffer;
	struct nxs_video_buffer *next_buffer;

	/* TODO: M2M */

	vfh = data;
	video = vfh->video;

	done_buffer = get_next_video_buffer(vfh, true);
	if (!done_buffer)
		BUG();

	next_buffer = get_next_video_buffer(vfh, false);
	if (!next_buffer) {
		atomic_set(&vfh->underflow, 1);
		nxs_chain_stop(video->nxs_function);
	} else {
		nxs_dev_set_buffer(nxs_dev, vfh, next_buffer);
	}

	vb2_buffer_done(&done_buffer->vb.vb2_buf, VB2_BUF_STATE_DONE);
}

/*
 * v4l2_file_operations
 */
static int nxs_video_open(struct file *file)
{
	struct nxs_video *nxs_video = video_drvdata(file);

	struct nxs_video_fh *handle;

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	v4l2_fh_init(&handle->vfh, &nxs_video->vdev);
	v4l2_fh_add(&handle->vfh);
	INIT_LIST_HEAD(&handle->bufq);
	spin_lock_init(&handle->bufq_lock);
	atomic_set(&handle->underflow, 0);

	memset(&handle->format, 0, sizeof(handle->format));

	handle->video = nxs_video;
	file->private_data = &handle->vfh;

	nxs_video->open_count++;

	return 0;
}

static int nxs_video_release(struct file *file)
{
	struct nxs_video *nxs_video = video_drvdata(file);
	struct v4l2_fh *vfh = file->private_data;
	struct nxs_video_fh *handle = to_nxs_video_fh(vfh);

	mutex_lock(&nxs_video->queue_lock);
	vb2_queue_release(&handle->queue);
	mutex_unlock(&nxs_video->queue_lock);

	v4l2_fh_del(vfh);
	kfree(handle);
	file->private_data = NULL;

	nxs_video->open_count--;
	/* TODO: if (nxs_video->open_count == 0) */

	return 0;
}

static int nxs_video_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(file->private_data);

	return vb2_mmap(&vfh->queue, vma);
}

static unsigned int nxs_video_poll(struct file *file,
				   struct poll_table_struct *tbl)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(file->private_data);
	struct nxs_video *video = video_drvdata(file);
	int ret;

	mutex_lock(&video->queue_lock);
	ret = vb2_poll(&vfh->queue, file, tbl);
	mutex_unlock(&video->queue_lock);

	return ret;
}

static struct v4l2_file_operations nxs_video_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = video_ioctl2,
	.open		= nxs_video_open,
	.release	= nxs_video_release,
	.poll		= nxs_video_poll,
	.mmap		= nxs_video_mmap,
};

/* v4l2_m2m_ops */
static void nxs_m2m_device_run(void *priv)
{
	/* TODO */
	pr_info("%s: entered\n", __func__);
}

static int nxs_m2m_job_ready(void *priv)
{
	/* TODO */
	pr_info("%s: entered\n", __func__);
	return 0;
}

static void nxs_m2m_job_abort(void *priv)
{
	/* TODO */
	pr_info("%s: entered\n", __func__);
}

static struct v4l2_m2m_ops nxs_m2m_ops = {
	.device_run = nxs_m2m_device_run,
	.job_ready  = nxs_m2m_job_ready,
	.job_abort  = nxs_m2m_job_abort,
};

/* v4l2_ioctl_ops */

/* VIDIOC_QUERYCAP handler */
static int nxs_vidioc_querycap(struct file *file, void *fh,
			       struct v4l2_capability *cap)
{
	struct nxs_video *nxs_video = video_drvdata(file);

	strlcpy(cap->driver, nxs_video->name, sizeof(cap->driver));
	strlcpy(cap->card, nxs_video->vdev.name, sizeof(cap->card));
	strlcpy(cap->bus_info, "media", sizeof(cap->bus_info));
	cap->version = KERNEL_VERSION(1, 0, 0);

	switch (nxs_video->type) {
	case NXS_VIDEO_TYPE_CAPTURE:
		cap->device_caps = V4L2_CAP_VIDEO_CAPTURE |
			V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_STREAMING;
		break;
	case NXS_VIDEO_TYPE_RENDER:
		cap->device_caps = V4L2_CAP_VIDEO_OUTPUT |
			V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_STREAMING;
		break;
	case NXS_VIDEO_TYPE_M2M:
		cap->device_caps = V4L2_CAP_VIDEO_M2M |
			V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
		break;
	}

	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

/* VIDIOC_ENUM_FMT handlers */
static int generic_enum_fmt(struct v4l2_fmtdesc *f)
{
	struct nxs_video_format *array;
	int array_size;

	if (is_multiplanar(f->type)) {
		array = supported_mplane_formats;
		array_size = ARRAY_SIZE(supported_mplane_formats);
	} else {
		array = supported_formats;
		array_size = ARRAY_SIZE(supported_formats);
	}

	if (f->index >= array_size)
		return -EINVAL;

	f->flags = 0;
	strlcpy(f->description, array[f->index].name, sizeof(f->description));
	f->pixelformat = array[f->index].format;

	return 0;
}

static int nxs_vidioc_enum_fmt_vid_cap(struct file *file, void *fh,
				       struct v4l2_fmtdesc *f)
{
	return generic_enum_fmt(f);
}

static int nxs_vidioc_enum_fmt_vid_out(struct file *file, void *fh,
				       struct v4l2_fmtdesc *f)
{
	return generic_enum_fmt(f);
}

static int nxs_vidioc_enum_fmt_vid_cap_mplane(struct file *file, void *fh,
					      struct v4l2_fmtdesc *f)
{
	return generic_enum_fmt(f);
}

static int nxs_vidioc_enum_fmt_vid_out_mplane(struct file *file, void *fh,
					      struct v4l2_fmtdesc *f)
{
	return generic_enum_fmt(f);
}

/* VIDIOC_G_FMT handlers */
static int generic_g_fmt(void *fh, struct v4l2_format *f)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);
	struct v4l2_format *cur_format = &vfh->format;

	if (cur_format->type == 0) {
		struct nxs_video *video = vfh->video;
		dev_err(&video->vdev.dev, "G_FMT: S_FMT is not called\n");
		return -EINVAL;
	}

	memcpy(f, cur_format, sizeof(*f));
	return 0;
}

static int nxs_vidioc_g_fmt_vid_cap(struct file *file, void *fh,
				    struct v4l2_format *f)
{
	return generic_g_fmt(fh, f);
}

static int nxs_vidioc_g_fmt_vid_out(struct file *file, void *fh,
				    struct v4l2_format *f)
{
	return generic_g_fmt(fh, f);
}

static int nxs_vidioc_g_fmt_vid_cap_mplane(struct file *file, void *fh,
					   struct v4l2_format *f)
{
	return generic_g_fmt(fh, f);
}

static int nxs_vidioc_g_fmt_vid_out_mplane(struct file *file, void *fh,
					   struct v4l2_format *f)
{
	return generic_g_fmt(fh, f);
}

/* VIDIOC_S_FMT handlers */
static bool is_supported_format(struct nxs_video_fh *vfh, struct v4l2_format *f)
{
	struct nxs_video_format *array;
	int array_size;
	int i;
	bool supported;
	u32 pixelformat;

	if (is_multiplanar(f->type)) {
		array = supported_mplane_formats;
		array_size = ARRAY_SIZE(supported_mplane_formats);
		pixelformat = f->fmt.pix_mp.pixelformat;
	} else {
		array = supported_formats;
		array_size = ARRAY_SIZE(supported_formats);
		pixelformat = f->fmt.pix.pixelformat;
	}

	supported = false;
	for (i = 0; i < array_size; i++) {
		if (pixelformat == array[i].format) {
			supported = true;
			break;
		}
	}

	return supported;
}

static int generic_s_fmt(void *fh, struct v4l2_format *f)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);
	struct nxs_video *video = vfh->video;

	if (is_supported_format(vfh, f)) {
		memcpy(&vfh->format, f, sizeof(*f));
		vfh->type = f->type;
		/* init crop, selection by format */
		if (vfh->crop.type == 0) {
			vfh->crop.type = f->type;
			vfh->crop.c.left = 0;
			vfh->crop.c.top = 0;
			vfh->crop.c.width = is_multiplanar(f->type) ?
				f->fmt.pix_mp.width : f->fmt.pix.width;
			vfh->crop.c.height = is_multiplanar(f->type) ?
				f->fmt.pix_mp.height : f->fmt.pix.height;
		}

		if (vfh->selection.type == 0) {
			vfh->selection.type = f->type;
			vfh->selection.r.left = 0;
			vfh->selection.r.top = 0;
			vfh->selection.r.width = is_multiplanar(f->type) ?
				f->fmt.pix_mp.width : f->fmt.pix.width;
			vfh->selection.r.height = is_multiplanar(f->type) ?
				f->fmt.pix_mp.height : f->fmt.pix.height;
		}

		if (vfh->parm.type == 0) {
			vfh->parm.type = f->type;
			if (video->type == NXS_VIDEO_TYPE_CAPTURE) {
				vfh->parm.parm.capture.capability =
					V4L2_CAP_TIMEPERFRAME;
				vfh->parm.parm.capture.timeperframe.denominator
					= 1;
				vfh->parm.parm.capture.timeperframe.numerator =
					NXS_VIDEO_CAPTURE_DEF_FRAMERATE;
				vfh->parm.parm.capture.readbuffers = 1;
			} else if (video->type == NXS_VIDEO_TYPE_RENDER) {
				vfh->parm.parm.output.capability =
					V4L2_CAP_TIMEPERFRAME;
				vfh->parm.parm.output.timeperframe.denominator =
					1;
				vfh->parm.parm.output.timeperframe.numerator =
					NXS_VIDEO_RENDER_DEF_FRAMERATE;
				vfh->parm.parm.output.writebuffers = 1;
			}
		}

		get_info_of_format(f, &vfh->num_planes, vfh->strides,
				   vfh->sizes, &vfh->width, &vfh->height,
				   &vfh->pixelformat);

		if (video->type == NXS_VIDEO_TYPE_CAPTURE ||
		    video->type == NXS_VIDEO_TYPE_RENDER)
			return nxs_vbq_init(vfh, f->type);
		else if (video->type == NXS_VIDEO_TYPE_M2M &&
			 !vfh->m2m_ctx) {
			vfh->m2m_dev = v4l2_m2m_init(&nxs_m2m_ops);
			if (IS_ERR(vfh->m2m_dev)) {
				dev_err(&video->vdev.dev,
					"%s: failed to initialize v4l2-m2m device\n",
					__func__);
				return PTR_ERR(vfh->m2m_dev);
			}
		}
	}

	return -EINVAL;
}

static int nxs_vidioc_s_fmt_vid_cap(struct file *file, void *fh,
				    struct v4l2_format *f)
{
	return generic_s_fmt(fh, f);
}

static int nxs_vidioc_s_fmt_vid_out(struct file *file, void *fh,
				    struct v4l2_format *f)
{
	return generic_s_fmt(fh, f);
}

static int nxs_vidioc_s_fmt_vid_cap_mplane(struct file *file, void *fh,
					   struct v4l2_format *f)
{
	return generic_s_fmt(fh, f);
}

static int nxs_vidioc_s_fmt_vid_out_mplane(struct file *file, void *fh,
					   struct v4l2_format *f)
{
	return generic_s_fmt(fh, f);
}

/* VIDIOC_TRY_FMT handlers */
static int generic_try_fmt(void *fh, struct v4l2_format *f)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);

	if (is_supported_format(vfh, f))
		return 0;

	return -EINVAL;
}

static int nxs_vidioc_try_fmt_vid_cap(struct file *file, void *fh,
				      struct v4l2_format *f)
{
	return generic_try_fmt(fh, f);
}

static int nxs_vidioc_try_fmt_vid_out(struct file *file, void *fh,
				      struct v4l2_format *f)
{
	return generic_try_fmt(fh, f);
}

static int nxs_vidioc_try_fmt_vid_cap_mplane(struct file *file, void *fh,
					     struct v4l2_format *f)
{
	return generic_try_fmt(fh, f);
}

static int nxs_vidioc_try_fmt_vid_out_mplane(struct file *file, void *fh,
					     struct v4l2_format *f)
{
	return generic_try_fmt(fh, f);
}

/* Buffer handlers */
static int nxs_vidioc_reqbufs(struct file *file, void *fh,
			      struct v4l2_requestbuffers *b)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);
	struct nxs_video *video = vfh->video;
	int ret;

	mutex_lock(&video->queue_lock);
	ret = vb2_reqbufs(&vfh->queue, b);
	mutex_unlock(&video->queue_lock);

	return ret;
}

static int nxs_vidioc_querybuf(struct file *file, void *fh,
			       struct v4l2_buffer *b)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);
	struct nxs_video *video = vfh->video;
	int ret;

	mutex_lock(&video->queue_lock);
	ret = vb2_querybuf(&vfh->queue, b);
	mutex_unlock(&video->queue_lock);

	return ret;
}

static int nxs_vidioc_qbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);
	struct nxs_video *video = vfh->video;
	int ret;

	mutex_lock(&video->queue_lock);
	ret = vb2_qbuf(&vfh->queue, b);
	mutex_unlock(&video->queue_lock);

	return ret;
}

static int nxs_vidioc_expbuf(struct file *file, void *fh,
			     struct v4l2_exportbuffer *e)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);
	struct nxs_video *video = vfh->video;
	int ret;

	mutex_lock(&video->queue_lock);
	ret = vb2_expbuf(&vfh->queue, e);
	mutex_unlock(&video->queue_lock);

	return ret;
}

static int nxs_vidioc_dqbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);
	struct nxs_video *video = vfh->video;
	int ret;

	mutex_lock(&video->queue_lock);
	ret = vb2_dqbuf(&vfh->queue, b, file->f_flags & O_NONBLOCK);
	mutex_unlock(&video->queue_lock);

	return ret;
}

static int nxs_vidioc_create_bufs(struct file *file, void *fh,
				  struct v4l2_create_buffers *b)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);
	struct nxs_video *video = vfh->video;
	int ret;

	mutex_lock(&video->queue_lock);
	ret = vb2_create_bufs(&vfh->queue, b);
	mutex_unlock(&video->queue_lock);

	return ret;
}

static int nxs_vidioc_prepare_buf(struct file *file, void *fh,
				  struct v4l2_buffer *b)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);
	struct nxs_video *video = vfh->video;
	int ret;

	mutex_lock(&video->queue_lock);
	ret = vb2_prepare_buf(&vfh->queue, b);
	mutex_unlock(&video->queue_lock);

	return ret;
}

/* Stream on/off */
static int nxs_vidioc_streamon(struct file *file, void *fh,
			       enum v4l2_buf_type type)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);
	struct nxs_video *video = vfh->video;
	int ret;

	if (type != vfh->queue.type)
		return -EINVAL;

	mutex_lock(&video->stream_lock);
	mutex_lock(&video->queue_lock);
	ret = vb2_streamon(&vfh->queue, type);
	mutex_unlock(&video->queue_lock);
	mutex_unlock(&video->stream_lock);

	return ret;
}

static int nxs_vidioc_streamoff(struct file *file, void *fh,
				enum v4l2_buf_type type)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);
	struct nxs_video *video = vfh->video;
	int ret;

	if (type != vfh->queue.type)
		return -EINVAL;

	mutex_lock(&video->stream_lock);
	mutex_lock(&video->queue_lock);
	ret = vb2_streamoff(&vfh->queue, type);
	mutex_unlock(&video->queue_lock);
	mutex_unlock(&video->stream_lock);

	return ret;
}

/* Control handling */
static int nxs_vidioc_queryctrl(struct file *file, void *fh,
				struct v4l2_queryctrl *ctrl)
{
	/* struct nxs_video_fh *vfh = to_nxs_video_fh(fh); */
	/* struct nxs_video *video = vfh->video; */
	/* int ret; */

	/* TODO: see CIDS include/uapi/linux/v4l2-controls.h */
	/* use like below */
	/* switch (ctrl->id) { */
	/* case V4L2_CID_ROTATE: */
	/* 	ret = v4l2_ctrl_query_fill(ctrl, min, max, step, def); */
	/* 	break; */
	/* } */

	return 0;
}

static int nxs_vidioc_g_ctrl(struct file *file, void *fh,
			     struct v4l2_control *ctrl)
{
	/* struct nxs_video_fh *vfh = to_nxs_video_fh(fh); */
	/* struct nxs_video *video = vfh->video; */
	/* int ret; */

	/* TODO: use like below */
	/* switch (ctrl->id) { */
	/* case V4L2_CID_ROTATE: */
	/* 	ctrl->value = xxx; */
	/* 	break; */
	/* } */

	return 0;
}

static int nxs_vidioc_s_ctrl(struct file *file, void *fh,
			     struct v4l2_control *ctrl)
{
	/* struct nxs_video_fh *vfh = to_nxs_video_fh(fh); */
	/* struct nxs_video *video = vfh->video; */
	/* int ret; */

	/* TODO: use like below */
	/* switch (ctrl->id) { */
	/* case V4L2_CID_ROTATE: */
	/* 	vfh->xx = ctrl->value; */
	/* 	break; */
	/* } */

	return 0;
}

/* Ext control handling */
static int nxs_vidioc_query_ext_ctrl(struct file *file, void *fh,
				     struct v4l2_query_ext_ctrl *ctrl)
{
	/* struct nxs_video_fh *vfh = to_nxs_video_fh(fh); */
	/* struct nxs_video *video = vfh->video; */
	/* int ret; */

	/* TODO */
	return 0;
}

static int nxs_vidioc_g_ext_ctrls(struct file *file, void *fh,
				  struct v4l2_ext_controls *ctrls)
{
	/* struct nxs_video_fh *vfh = to_nxs_video_fh(fh); */
	/* struct nxs_video *video = vfh->video; */
	/* int ret; */

	/* TODO */
	return 0;
}

static int nxs_vidioc_s_ext_ctrls(struct file *file, void *fh,
				  struct v4l2_ext_controls *ctrls)
{
	/* struct nxs_video_fh *vfh = to_nxs_video_fh(fh); */
	/* struct nxs_video *video = vfh->video; */
	/* int ret; */

	/* TODO */
	return 0;
}

static int nxs_vidioc_try_ext_ctrls(struct file *file, void *fh,
				    struct v4l2_ext_controls *ctrls)
{
	/* struct nxs_video_fh *vfh = to_nxs_video_fh(fh); */
	/* struct nxs_video *video = vfh->video; */
	/* int ret; */

	/* TODO */
	return 0;
}

/* Crop ioctls */
/* crop is for input image cropping */
/* generally cropping will be used in vip clipper */
static int nxs_vidioc_cropcap(struct file *file, void *fh,
			      struct v4l2_cropcap *a)
{
	/* struct nxs_video_fh *vfh = to_nxs_video_fh(fh); */
	/* struct nxs_video *video = vfh->video; */
	/* int ret; */

	/* TODO */
	return 0;
}

static int nxs_vidioc_g_crop(struct file *file, void *fh, struct v4l2_crop *a)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);

	if (vfh->crop.type == 0)
		return -EINVAL;

	memcpy(a, &vfh->crop, sizeof(*a));

	return 0;
}

static int nxs_vidioc_s_crop(struct file *file, void *fh,
			     const struct v4l2_crop *a)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);

	/* TODO: check crop area */
	memcpy(&vfh->crop, a, sizeof(*a));

	return 0;
}

static int nxs_vidioc_g_selection(struct file *file, void *fh,
				  struct v4l2_selection *s)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);

	if (vfh->selection.type == 0)
		return -EINVAL;

	memcpy(s, &vfh->selection, sizeof(*s));

	return 0;
}

static int nxs_vidioc_s_selection(struct file *file, void *fh,
				  struct v4l2_selection *s)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);

	/* TODO: check selection area */
	memcpy(&vfh->selection, s, sizeof(*s));

	return 0;
}

/* Stream type-dependent parameter ioctls */
static int nxs_vidioc_g_parm(struct file *file, void *fh,
			     struct v4l2_streamparm *a)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);

	if (vfh->parm.type == 0)
		return -EINVAL;

	memcpy(a, &vfh->parm, sizeof(*a));

	return 0;
}

static int nxs_vidioc_s_parm(struct file *file, void *fh,
			     struct v4l2_streamparm *a)
{
	struct nxs_video_fh *vfh = to_nxs_video_fh(fh);

	/* TODO: check parm parameter */
	memcpy(&vfh->parm, a, sizeof(*a));

	return 0;
}

/* Debugging ioctls */
#ifdef CONFIG_VIDEO_ADV_DEBUG
static int nxs_vidioc_g_register(struct file *file, void *fh,
				 struct v4l2_dbg_register *reg)
{
	/* struct nxs_video_fh *vfh = to_nxs_video_fh(fh); */
	/* struct nxs_video *video = vfh->video; */
	/* int ret; */

	/* TODO */
	return 0;
}

static int nxs_vidioc_s_register(struct file *file, void *fh,
				 struct v4l2_dbg_register *reg)
{
	/* struct nxs_video_fh *vfh = to_nxs_video_fh(fh); */
	/* struct nxs_video *video = vfh->video; */
	/* int ret; */

	/* TODO */
	return 0;
}
#endif

static int nxs_vidioc_enum_framesizes(struct file *file, void *fh,
				      struct v4l2_frmsizeenum *fsize)
{
	/* struct nxs_video_fh *vfh = to_nxs_video_fh(fh); */
	/* struct nxs_video *video = vfh->video; */
	/* int ret; */

	/* TODO: implement for only capture type */
	/* ask for sensor subdev */

	return 0;
}

static int nxs_vidioc_enum_frameintervals(struct file *file, void *fh,
					  struct v4l2_frmivalenum *fival)
{
	/* struct nxs_video_fh *vfh = to_nxs_video_fh(fh); */
	/* struct nxs_video *video = vfh->video; */
	/* int ret; */

	/* TODO: implement for only capture type */
	/* ask for sensor subdev */

	return 0;
}

/* DV Timings IOCTLs */
/* TODO this handler is only for render device */
static int nxs_vidioc_s_dv_timings(struct file *file, void *fh,
				   struct v4l2_dv_timings *timings)
{
	return 0;
}

static int nxs_vidioc_g_dv_timings(struct file *file, void *fh,
				   struct v4l2_dv_timings *timings)
{
	return 0;
}

static int nxs_vidioc_query_dv_timings(struct file *file, void *fh,
				       struct v4l2_dv_timings *timings)
{
	return 0;
}

static int nxs_vidioc_enum_dv_timings(struct file *file, void *fh,
				      struct v4l2_enum_dv_timings *timings)
{
	return 0;
}

static int nxs_vidioc_dv_timings_cap(struct file *file, void *fh,
				     struct v4l2_dv_timings_cap *cap)
{
	return 0;
}

/* EDID IOCTLs */
/* TODO this handler is only for render device */
static int nxs_vidioc_g_edid(struct file *file, void *fh,
			     struct v4l2_edid *edid)
{
	return 0;
}

static int nxs_vidioc_s_edid(struct file *file, void *fh,
			     struct v4l2_edid *edid)
{
	return 0;
}

/* v4l2_event IOCTLs */
/* App can subscribe event by VIDIOC_SUBSCRIBE_EVENT cmd and
 * can receive by VIDIOC_DQEVENT
 * Driver can emit event by v4l2_event_queue()
 * When driver calls v4l2_event_queue, process that called VIDIOC_DQEVENT
 * is awaken.
 * See videodev2.h, struct v4l2_event
 * event type: vsync, eos, ctrl, frame_sync, source_change, motion_det, private
 */
static int nxs_vidioc_subscribe_event(struct v4l2_fh *fh,
				      const struct v4l2_event_subscription *sub)
{
	if (sub->type == V4L2_EVENT_CTRL)
		return v4l2_ctrl_subscribe_event(fh, sub);
	/* TODO */
	/* else */

	return 0;
}

/* For other private ioctls */
static long nxs_vidioc_default(struct file *file, void *fh, bool valid_prio,
			       unsigned int cmd, void *arg)
{
	/* struct nxs_video_fh *vfh = to_nxs_video_fh(fh); */
	/* struct nxs_video *video = vfh->video; */
	/* int ret; */

	/* TODO */
	return 0;
}

static struct v4l2_ioctl_ops nxs_video_ioctl_ops = {
	.vidioc_querycap		= nxs_vidioc_querycap,
	.vidioc_enum_fmt_vid_cap	= nxs_vidioc_enum_fmt_vid_cap,
	.vidioc_enum_fmt_vid_out	= nxs_vidioc_enum_fmt_vid_out,
	.vidioc_enum_fmt_vid_cap_mplane	= nxs_vidioc_enum_fmt_vid_cap_mplane,
	.vidioc_enum_fmt_vid_out_mplane	= nxs_vidioc_enum_fmt_vid_out_mplane,
	.vidioc_g_fmt_vid_cap		= nxs_vidioc_g_fmt_vid_cap,
	.vidioc_g_fmt_vid_out		= nxs_vidioc_g_fmt_vid_out,
	.vidioc_g_fmt_vid_cap_mplane	= nxs_vidioc_g_fmt_vid_cap_mplane,
	.vidioc_g_fmt_vid_out_mplane	= nxs_vidioc_g_fmt_vid_out_mplane,
	.vidioc_s_fmt_vid_cap		= nxs_vidioc_s_fmt_vid_cap,
	.vidioc_s_fmt_vid_out		= nxs_vidioc_s_fmt_vid_out,
	.vidioc_s_fmt_vid_cap_mplane	= nxs_vidioc_s_fmt_vid_cap_mplane,
	.vidioc_s_fmt_vid_out_mplane	= nxs_vidioc_s_fmt_vid_out_mplane,
	.vidioc_try_fmt_vid_cap		= nxs_vidioc_try_fmt_vid_cap,
	.vidioc_try_fmt_vid_out		= nxs_vidioc_try_fmt_vid_out,
	.vidioc_try_fmt_vid_cap_mplane	= nxs_vidioc_try_fmt_vid_cap_mplane,
	.vidioc_try_fmt_vid_out_mplane	= nxs_vidioc_try_fmt_vid_out_mplane,
	.vidioc_reqbufs			= nxs_vidioc_reqbufs,
	.vidioc_querybuf		= nxs_vidioc_querybuf,
	.vidioc_qbuf			= nxs_vidioc_qbuf,
	.vidioc_expbuf			= nxs_vidioc_expbuf,
	.vidioc_dqbuf			= nxs_vidioc_dqbuf,
	.vidioc_create_bufs		= nxs_vidioc_create_bufs,
	.vidioc_prepare_buf		= nxs_vidioc_prepare_buf,
	.vidioc_streamon		= nxs_vidioc_streamon,
	.vidioc_streamoff		= nxs_vidioc_streamoff,
	.vidioc_queryctrl		= nxs_vidioc_queryctrl,
	.vidioc_g_ctrl			= nxs_vidioc_g_ctrl,
	.vidioc_s_ctrl			= nxs_vidioc_s_ctrl,
	.vidioc_query_ext_ctrl		= nxs_vidioc_query_ext_ctrl,
	.vidioc_g_ext_ctrls		= nxs_vidioc_g_ext_ctrls,
	.vidioc_s_ext_ctrls		= nxs_vidioc_s_ext_ctrls,
	.vidioc_try_ext_ctrls		= nxs_vidioc_try_ext_ctrls,
	.vidioc_cropcap			= nxs_vidioc_cropcap,
	.vidioc_g_crop			= nxs_vidioc_g_crop,
	.vidioc_s_crop			= nxs_vidioc_s_crop,
	.vidioc_g_selection		= nxs_vidioc_g_selection,
	.vidioc_s_selection		= nxs_vidioc_s_selection,
	.vidioc_g_parm			= nxs_vidioc_g_parm,
	.vidioc_s_parm			= nxs_vidioc_s_parm,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.vidioc_g_register		= nxs_vidioc_g_register,
	.vidioc_s_register		= nxs_vidioc_s_register,
#endif
	.vidioc_enum_framesizes		= nxs_vidioc_enum_framesizes,
	.vidioc_enum_frameintervals	= nxs_vidioc_enum_frameintervals,
	.vidioc_s_dv_timings		= nxs_vidioc_s_dv_timings,
	.vidioc_g_dv_timings		= nxs_vidioc_g_dv_timings,
	.vidioc_query_dv_timings	= nxs_vidioc_query_dv_timings,
	.vidioc_enum_dv_timings		= nxs_vidioc_enum_dv_timings,
	.vidioc_dv_timings_cap		= nxs_vidioc_dv_timings_cap,
	.vidioc_g_edid			= nxs_vidioc_g_edid,
	.vidioc_s_edid			= nxs_vidioc_s_edid,
	.vidioc_subscribe_event		= nxs_vidioc_subscribe_event,
	.vidioc_unsubscribe_event	= v4l2_event_unsubscribe,
	.vidioc_default			= nxs_vidioc_default,
};

/* struct vb2_ops */

/* reqbuf call this callback */
static int nxs_vb2_queue_setup(struct vb2_queue *q,
			       const void *parg,
			       unsigned int *num_buffers,
			       unsigned int *num_planes,
			       unsigned int sizes[],
			       void *alloc_ctxs[])
{
	struct nxs_video_fh *vfh = vb2_get_drv_priv(q);
	struct nxs_video *video = vfh->video;
	int i;

	if (vfh->format.type == 0) {
		dev_err(&video->vdev.dev, "%s: format is not set\n", __func__);
		return -EINVAL;
	}

	*num_planes = vfh->num_planes;
	memcpy(sizes, vfh->sizes, vfh->num_planes * sizeof(unsigned int));

	for (i = 0; i < *num_planes; i++)
		alloc_ctxs[i] = video->vb2_alloc_ctx;

	/* do not need to set num_buffers
	 * use user value
	 */

	return 0;
}

/* TODO: check wait_prepare, wait_finish overriding */
/* static void nxs_vb2_wait_prepare(struct vb2_queue *q) */
/* { */
/* } */
/*  */
/* static void nxs_vb2_wait_finish(struct vb2_queue *q) */
/* { */
/* } */

/* TODO: check vb2_buf_init, vb2_buf_finish, vb2_buf_cleanup overriding */
/* static int nxs_vb2_buf_init(struct vb2_buffer *vb) */
/* { */
/* 	return 0; */
/* } */
/*  */
/* static void nxs_vb2_buf_finish(struct vb2_buffer *vb) */
/* { */
/* } */
/*  */
/* static void nxs_vb2_buf_cleanup(struct vb2_buffer *vb) */
/* { */
/* } */

static int nxs_vb2_buf_prepare(struct vb2_buffer *buf)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buf);
	struct nxs_video_fh *vfh = vb2_get_drv_priv(buf->vb2_queue);
	struct nxs_video_buffer *buffer = to_nxs_video_buffer(vbuf);
	struct nxs_video *video = vfh->video;
	int i;

	memcpy(buffer->strides, vfh->strides,
	       buf->num_planes * sizeof(unsigned int));

	for (i = 0; i < buf->num_planes; i++) {
		buffer->dma_addr[i] = vb2_dma_contig_plane_dma_addr(buf, i);
		if (!buffer->dma_addr[i]) {
			dev_err(&video->vdev.dev,
				"failed to get dma address for index %d\n", i);
			return -ENOMEM;
		}
		vb2_set_plane_payload(&buffer->vb.vb2_buf, 0, vfh->sizes[i]);
	}

	return 0;
}

static void nxs_vb2_buf_queue(struct vb2_buffer *buf)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buf);
	struct nxs_video_fh *vfh = vb2_get_drv_priv(buf->vb2_queue);
	struct nxs_video_buffer *buffer = to_nxs_video_buffer(vbuf);
	unsigned long flags;

	spin_lock_irqsave(&vfh->bufq_lock, flags);
	list_add_tail(&buffer->list, &vfh->bufq);
	spin_unlock_irqrestore(&vfh->bufq_lock, flags);
}

static int nxs_vb2_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct nxs_video_fh *vfh = vb2_get_drv_priv(q);
	struct nxs_video *video = vfh->video;
	struct nxs_video_buffer *buffer;
	int ret;

	buffer = get_next_video_buffer(vfh, false);
	if (!buffer) {
		dev_err(&video->vdev.dev, "%s: can't get buffer\n", __func__);
		return -ENOMEM;
	}

	ret = nxs_chain_register_irqcallback(video->nxs_function, vfh);
	if (ret)
		return ret;

	ret = nxs_chain_config(video->nxs_function, vfh);
	if (ret)
		return ret;

	ret = nxs_chain_set_buffer(video->nxs_function, vfh, buffer);
	if (ret)
		return ret;

	ret = nxs_chain_run(video->nxs_function);
	if (ret)
		return ret;

	/* TODO */
	/* when using camera sensor, subdev ops needed here */

	return 0;
}

static void nxs_vb2_stop_streaming(struct vb2_queue *q)
{
	struct nxs_video_fh *vfh = vb2_get_drv_priv(q);
	struct nxs_video *video = vfh->video;

	nxs_chain_stop(video->nxs_function);
	nxs_chain_unregister_irqcallback(video->nxs_function);

	/* TODO */
	/* when using camera sensor, subdev ops needed here */
}

static struct vb2_ops nxs_vb2_ops = {
	.queue_setup		= nxs_vb2_queue_setup,
	/* .wait_prepare		= vb2_ops_wait_prepare, */
	/* .wait_finish		= vb2_ops_wait_finish, */
	/* .buf_init		= nxs_vb2_buf_init, */
	/* .buf_finish		= nxs_vb2_buf_finish, */
	/* .buf_cleanup		= nxs_vb2_buf_cleanup, */
	.buf_prepare		= nxs_vb2_buf_prepare,
	.buf_queue		= nxs_vb2_buf_queue,
	.start_streaming	= nxs_vb2_start_streaming,
	.stop_streaming		= nxs_vb2_stop_streaming,
};

static int nxs_vbq_init(struct nxs_video_fh *vfh, u32 type)
{
	struct vb2_queue *vbq;

	vbq = &vfh->queue;

	vbq->type	= type;
	vbq->io_modes	= VB2_MMAP
			| VB2_USERPTR
			| VB2_READ
			| VB2_WRITE
			| VB2_DMABUF;
	vbq->drv_priv	= vfh;
	vbq->ops	= &nxs_vb2_ops;
	vbq->mem_ops	= &vb2_dma_contig_memops;
	vbq->buf_struct_size = sizeof(struct nxs_video_buffer);
	vbq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;

	return vb2_queue_init(vbq);
}

/* m2m vb2 ops */
static int nxs_m2m_vb2_queue_setup(struct vb2_queue *q,
				   const void *parg,
				   unsigned int *num_buffers,
				   unsigned int *num_planes,
				   unsigned int sizes[],
				   void *alloc_ctxs[])
{
	/* TODO */
	return 0;
}

static int nxs_m2m_vb2_buf_prepare(struct vb2_buffer *buf)
{
	/* TODO */
	return 0;
}

static void nxs_m2m_vb2_buf_queue(struct vb2_buffer *buf)
{
	/* TODO */
}

static int nxs_m2m_vb2_start_streaming(struct vb2_queue *q, unsigned int count)
{
	/* TODO */
	return 0;
}

static void nxs_m2m_vb2_stop_streaming(struct vb2_queue *q)
{
	/* TODO */
}

static struct vb2_ops nxs_m2m_vb2_ops = {
	.queue_setup		= nxs_m2m_vb2_queue_setup,
	.buf_prepare		= nxs_m2m_vb2_buf_prepare,
	.buf_queue		= nxs_m2m_vb2_buf_queue,
	.start_streaming	= nxs_m2m_vb2_start_streaming,
	.stop_streaming		= nxs_m2m_vb2_stop_streaming,
};

static int nxs_m2m_queue_init(void *priv, struct vb2_queue *src_q,
			      struct vb2_queue *dst_q)
{
	/* struct nxs_video_fh *vfh = priv; */
	/* int ret; */

	memset(src_q, 0, sizeof(*src_q));

	/* TODO */
	return 0;
}

static u32 get_nxs_video_type(struct nxs_function_instance *inst)
{
	struct nxs_dev *first, *last;

	first = list_first_entry(&inst->dev_list, struct nxs_dev, func_list);
	last = list_last_entry(&inst->dev_list, struct nxs_dev, func_list);

	if (!first || !last)
		return NXS_VIDEO_TYPE_INVALID;

	if (first->dev_function == NXS_FUNCTION_DMAR) {
		if (last->dev_function == NXS_FUNCTION_DMAW)
			return NXS_VIDEO_TYPE_M2M;
		else
			return NXS_VIDEO_TYPE_RENDER;
	} else {
		if (last->dev_function == NXS_FUNCTION_DMAW)
			return NXS_VIDEO_TYPE_CAPTURE;
		else
			return NXS_VIDEO_TYPE_SUBDEV;
	}
}

static void dump_nxs_dev(struct nxs_dev *dev)
{
	pr_info("\tdev %p: [%s:%d] user %d, multitap_connected %d, connect_count %d\n",
		dev, nxs_function_to_str(dev->dev_function),
		dev->dev_inst_index,
		dev->user,
		dev->multitap_connected,
		atomic_read(&dev->connect_count));
	pr_info("\t\tcan_multitap_follow %d, refcount %d, max_refcount %d\n",
		dev->can_multitap_follow, atomic_read(&dev->refcount),
		dev->max_refcount);
}

static void dump_nxs_function_inst(struct nxs_function_instance *inst)
{
	struct nxs_dev *nxs_dev;

	pr_info("dump inst %p ==================> \n", inst);
	pr_info("\tsibling list\n");
	list_for_each_entry(nxs_dev, &inst->dev_sibling_list, sibling_list) {
		dump_nxs_dev(nxs_dev);
	}
	pr_info("\tdev list\n");
	list_for_each_entry(nxs_dev, &inst->dev_list, func_list) {
		dump_nxs_dev(nxs_dev);
	}
}

static struct nxs_video *build_nxs_video(struct nxs_v4l2_builder *builder,
					 const char *name,
					 struct nxs_function_instance *inst)
{
	int ret;
	struct vb2_queue *vbq;
	struct nxs_video *nxs_video;
	u32 type;

	nxs_video = kzalloc(sizeof(*nxs_video), GFP_KERNEL);
	if (!nxs_video) {
		WARN_ON(1);
		return ERR_PTR(-ENOMEM);
	}

	pr_info("%s: builder %p, name %s, inst %p\n",
		__func__, builder, name, inst);

	/* dump_nxs_function_inst(inst); */
	snprintf(nxs_video->name, sizeof(nxs_video->name), "%s", name);
	/* snprintf(nxs_video->vdev.name, sizeof(nxs_video->vdev.name), */
	/* 	 "%s", name); */

	type = get_nxs_video_type(inst);
	if (type == NXS_VIDEO_TYPE_INVALID) {
		dev_err(builder->dev, "invalid type 0x%x\n", type);
		kfree(nxs_video);
		return NULL;
	}

	mutex_init(&nxs_video->queue_lock);
	mutex_init(&nxs_video->stream_lock);

	nxs_video->type			= type;
	nxs_video->vdev.fops		= &nxs_video_fops;
	nxs_video->vdev.ioctl_ops	= &nxs_video_ioctl_ops;
	nxs_video->vdev.v4l2_dev	= &builder->v4l2_dev;
	nxs_video->vdev.minor		= -1;
	nxs_video->vdev.vfl_type	= VFL_TYPE_GRABBER;
	nxs_video->vdev.release		= video_device_release;
	/* nxs_video->vdev.lock		= &nxs_video->queue_lock; */

	switch (type) {
	case NXS_VIDEO_TYPE_CAPTURE:
		nxs_video->vdev.vfl_dir = VFL_DIR_RX;
		break;
	case NXS_VIDEO_TYPE_RENDER:
		nxs_video->vdev.vfl_dir = VFL_DIR_TX;
		break;
	case NXS_VIDEO_TYPE_M2M:
		nxs_video->vdev.vfl_dir = VFL_DIR_M2M;
		break;
	}

	nxs_video->vb2_alloc_ctx	= builder->vb2_alloc_ctx;

	ret = video_register_device(&nxs_video->vdev, VFL_TYPE_GRABBER, -1);
	if (ret < 0) {
		dev_err(builder->dev, "failed to video_register_device()\n");
		goto free_vbq;
	}

	video_set_drvdata(&nxs_video->vdev, nxs_video);

	nxs_video->nxs_function = inst;

	pr_info("%s exit\n", __func__);

	dump_nxs_function_inst(inst);

	return nxs_video;

free_vbq:
	kfree(vbq);

/* free_nxs_video: */
	kfree(nxs_video);

	return ERR_PTR(ret);
}

static int cleanup_nxs_video(struct nxs_video *video)
{
	video_unregister_device(&video->vdev);
	/* vb2_queue_release(video->vbq); */
	/* kfree(video->vbq); */
	mutex_destroy(&video->queue_lock);
	mutex_destroy(&video->stream_lock);
	/* below code is source of kobject_uevent_env kernel panic */
	/* kfree(video); */

	return 0;
}

static void free_function_instance(struct nxs_function_instance *inst)
{
	struct nxs_dev *nxs_dev;

	pr_info("%s: inst %p\n", __func__, inst);
	pr_info("%s: free sibling_list start\n", __func__);
	while (!list_empty(&inst->dev_sibling_list)) {
		nxs_dev = list_first_entry(&inst->dev_sibling_list,
					   struct nxs_dev, sibling_list);
		pr_info("sibling: put %p, function %s, index %d\n",
			nxs_dev, nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		list_del_init(&nxs_dev->sibling_list);
		put_nxs_dev(nxs_dev);
	}
	/* list_for_each_entry(nxs_dev, &inst->dev_sibling_list, sibling_list) { */
	/* 	pr_info("sibling: put %p, function %s, index %d\n", __func__, */
	/* 		nxs_dev, nxs_function_to_str(nxs_dev->dev_function), */
	/* 		nxs_dev->dev_inst_index); */
	/* 	put_nxs_dev(nxs_dev); */
	/* 	#<{(| list_del_init(&nxs_dev->sibling_list); |)}># */
	/* } */
	pr_info("%s: free sibling_list end\n", __func__);

	pr_info("%s: free dev_list start\n", __func__);
	while (!list_empty(&inst->dev_list)) {
		nxs_dev = list_first_entry(&inst->dev_list,
					   struct nxs_dev, func_list);
		pr_info("dev: put %p, function %s, index %d\n",
			nxs_dev, nxs_function_to_str(nxs_dev->dev_function),
			nxs_dev->dev_inst_index);
		list_del_init(&nxs_dev->func_list);
		put_nxs_dev(nxs_dev);
	}
	/* list_for_each_entry(nxs_dev, &inst->dev_list, func_list) { */
	/* 	pr_info("dev: put %p, function %s, index %d\n", __func__, */
	/* 		nxs_dev, nxs_function_to_str(nxs_dev->dev_function), */
	/* 		nxs_dev->dev_inst_index); */
	/* 	put_nxs_dev(nxs_dev); */
	/* 	#<{(| list_del_init(&nxs_dev->func_list); |)}># */
	/* } */
	pr_info("%s: free dev_list end\n", __func__);

	free_function_request(inst->req);

	kfree(inst);
}

static struct nxs_function_instance *
nxs_v4l2_build(struct nxs_function_builder *pthis,
	       const char *name,
	       struct nxs_function_request *req)
{
	struct nxs_function *func;
	struct nxs_dev *nxs_dev;
	struct nxs_function_instance *inst = NULL;
	struct nxs_v4l2_builder *me;
	struct nxs_video *nxs_video;

	me = pthis->priv;

	inst = kzalloc(sizeof(*inst), GFP_KERNEL);
	if (!inst) {
		WARN_ON(1);
		return NULL;
	}

	INIT_LIST_HEAD(&inst->dev_list);
	INIT_LIST_HEAD(&inst->dev_sibling_list);

	list_for_each_entry(func, &req->head, list) {
		if (func)
			dev_info(me->dev, "REQ: function %s, index %d, user %d\n",
				nxs_function_to_str(func->function),
				func->index, func->user);
		nxs_dev = get_nxs_dev(func->function, func->index, func->user,
				      func->multitap_follow);
		if (!nxs_dev) {
			dev_err(me->dev, "can't get nxs_dev for func %s, index 0x%x\n",
				nxs_function_to_str(func->function),
				func->index);
			goto error_out;
		}

		dev_info(me->dev, "GET: fuction %s, index %d, user %d, multitap_connected %d\n",
			 nxs_function_to_str(nxs_dev->dev_function),
			 nxs_dev->dev_inst_index,
			 nxs_dev->user,
			 nxs_dev->multitap_connected);

		if ((nxs_dev->dev_function == NXS_FUNCTION_MULTITAP &&
		     atomic_read(&nxs_dev->refcount) > 1) ||
		    (nxs_dev->multitap_connected &&
		     atomic_read(&nxs_dev->connect_count) > 1)) {
			pr_info("function %s, index %d added to sibling_list\n",
				nxs_function_to_str(nxs_dev->dev_function),
				nxs_dev->dev_inst_index);
			list_add_tail(&nxs_dev->sibling_list,
				      &inst->dev_sibling_list);
		}
		else
			list_add_tail(&nxs_dev->func_list, &inst->dev_list);
	}

	inst->req = req;

	nxs_video = build_nxs_video(me, name, inst);
	if (!nxs_video) {
		dev_err(me->dev, "failed to build nxs v4l2 driver\n");
		goto error_out;
	}

	inst->priv = nxs_video;

	mutex_lock(&me->lock);
	me->seq++;
	inst->id = me->seq;
	list_add_tail(&inst->sibling_list, &me->function_list);
	mutex_unlock(&me->lock);

	return inst;

error_out:
	if (inst)
		free_function_instance(inst);

	return NULL;
}

static int nxs_v4l2_free(struct nxs_function_builder *pthis,
			 struct nxs_function_instance *inst)
{
	struct nxs_v4l2_builder *me;
	struct nxs_video *nxs_video;
	struct nxs_function_instance *entry;
	int ret;

	me = pthis->priv;
	nxs_video = inst->priv;

	ret = cleanup_nxs_video(nxs_video);
	if (ret) {
		dev_err(me->dev,
			"%s: failed to cleanup_nxs_video\n", __func__);
		return ret;
	}

	mutex_lock(&me->lock);
	list_for_each_entry(entry, &me->function_list, sibling_list) {
		if (entry == inst) {
			list_del_init(&inst->sibling_list);
			break;
		}
	}
	mutex_unlock(&me->lock);

	free_function_instance(inst);

	return 0;
}

static struct nxs_function_instance *
nxs_v4l2_get(struct nxs_function_builder *pthis, int handle)
{
	struct nxs_v4l2_builder *me;
	struct nxs_function_instance *entry;
	bool found = false;

	me = pthis->priv;

	mutex_lock(&me->lock);
	list_for_each_entry(entry, &me->function_list, sibling_list) {
		if (entry->id == handle) {
			found = true;
			break;
		}
	}

	if (!found)
		entry = NULL;

	mutex_unlock(&me->lock);

	return entry;
}

static inline char *get_vdev_name_base(struct video_device *vdev)
{
	switch (vdev->vfl_type) {
	case VFL_TYPE_GRABBER:
		return "video";
	case VFL_TYPE_SUBDEV:
		return "v4l-subdev";
	default:
		BUG();
	}

	return NULL;
}

static inline int get_vdev_nr(struct video_device *vdev)
{
	return vdev->num;
}

static int nxs_v4l2_query(struct nxs_function_builder *pthis,
			  struct nxs_query_function *query)
{
	struct nxs_v4l2_builder *me;
	struct nxs_function_instance *inst;
	struct nxs_video *nxs_video;

	me = pthis->priv;
	inst = nxs_v4l2_get(pthis, query->handle);

	if (!inst) {
		dev_err(me->dev,
			"%s: can't find function instance for handle %d\n",
			__func__, query->handle);
		return -ENOENT;
	}

	nxs_video = (struct nxs_video *)inst->priv;

	switch (query->query) {
	case NXS_FUNCTION_QUERY_DEVINFO:
		sprintf(query->devinfo.dev_name, "%s%d",
			get_vdev_name_base(&nxs_video->vdev),
			get_vdev_nr(&nxs_video->vdev));
		break;
	default:
		dev_err(me->dev,
			"%s: unknown query 0x%x\n", __func__, query->query);
		return -EINVAL;
	}

	return 0;
}

static struct nxs_function_builder v4l2_builder = {
	.build = nxs_v4l2_build,
	.free  = nxs_v4l2_free,
	.get   = nxs_v4l2_get,
	.query = nxs_v4l2_query,
};

static int init_v4l2_context(struct nxs_v4l2_builder *builder)
{
	int ret;
	struct v4l2_device *v4l2_dev;

	builder->media_dev.dev = builder->dev;
	snprintf(builder->media_dev.model, sizeof(builder->media_dev.model),
		 "%s", dev_name(builder->dev));

	v4l2_dev	= &builder->v4l2_dev;
	v4l2_dev->mdev	= &builder->media_dev;
	snprintf(v4l2_dev->name, sizeof(v4l2_dev->name),
		 "%s", dev_name(builder->dev));

	builder->vb2_alloc_ctx = vb2_dma_contig_init_ctx(builder->dev);
	if (!builder->vb2_alloc_ctx) {
		WARN_ON(1);
		return -ENOMEM;
	}

	ret = v4l2_device_register(builder->dev, &builder->v4l2_dev);
	if (ret < 0) {
		dev_err(builder->dev, "failed to register nxs v4l2_device\n");
		goto cleanup_alloc_ctx;
	}

	ret = media_device_register(&builder->media_dev);
	if (ret < 0) {
		dev_err(builder->dev, "failed to register nxs media_device\n");
		goto unregister_v4l2;
	}

	dev_info(builder->dev, "%s success\n", __func__);

	return 0;

unregister_v4l2:
	v4l2_device_unregister(&builder->v4l2_dev);

cleanup_alloc_ctx:
	vb2_dma_contig_cleanup_ctx(&builder->vb2_alloc_ctx);

	return ret;
}

static int nxs_v4l2_builder_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_v4l2_builder *builder;

	builder = devm_kzalloc(&pdev->dev, sizeof(*builder), GFP_KERNEL);
	if (!builder) {
		WARN_ON(1);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&builder->function_list);
	mutex_init(&builder->lock);

	builder->builder = &v4l2_builder;
	v4l2_builder.priv = builder;
	builder->dev = &pdev->dev;

	ret = register_nxs_function_builder(builder->builder);
	if (ret) {
		dev_err(builder->dev,
			"failed to register_nxs_function_builder\n");
		return ret;
	}

	ret = init_v4l2_context(builder);
	if (ret) {
		unregister_nxs_function_builder(builder->builder);
		return ret;
	}

	platform_set_drvdata(pdev, builder);

	dev_info(builder->dev, "%s success\n", __func__);

	return 0;
}

static int nxs_v4l2_builder_remove(struct platform_device *pdev)
{
	struct nxs_v4l2_builder *builder;

	builder = platform_get_drvdata(pdev);
	/* TODO */
	/* returns all functions */
	unregister_nxs_function_builder(builder->builder);

	return 0;
}

static const struct of_device_id nxs_v4l2_builder_match[] = {
	{ .compatible = "nexell,nxs-v4l2-builder-1.0", },
	{},
};

static struct platform_driver nxs_v4l2_builder_driver = {
	.probe  = nxs_v4l2_builder_probe,
	.remove = nxs_v4l2_builder_remove,
	.driver = {
		.name = "nxs-v4l2-builer",
		.of_match_table = of_match_ptr(nxs_v4l2_builder_match),
	},
};

module_platform_driver(nxs_v4l2_builder_driver);
