/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Seonghee, Kim <kshblue@nexell.co.kr>
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
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/interrupt.h>

#include <linux/videodev2.h>
#include <linux/videodev2_nxp_media.h>

#include <linux/soc/nexell/nx-media-device.h>

#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>

#ifdef CONFIG_ARM_S5Pxx18_DEVFREQ
#include <linux/pm_qos.h>
#include <linux/soc/nexell/cpufreq.h>
#endif

#include "nx_vpu_v4l2.h"
#include "vpu_hw_interface.h"

#define	NX_VIDEO_NAME "nx-vpu"
#define	NX_VIDEO_ENC_NAME "nx-vpu-enc"
#define	NX_VIDEO_DEC_NAME "nx-vpu-dec"

static int num_open_inst;
static struct mutex g_vpu_mutex;

#define INFO_MSG		0

#ifdef CONFIG_ARM_S5Pxx18_DEVFREQ
static struct pm_qos_request nx_vpu_qos;

static void nx_vpu_qos_update(int val)
{
	if (!pm_qos_request_active(&nx_vpu_qos))
		pm_qos_add_request(&nx_vpu_qos, PM_QOS_BUS_THROUGHPUT, val);
	else
		pm_qos_update_request(&nx_vpu_qos, val);
}
#endif

dma_addr_t nx_vpu_mem_plane_addr(struct nx_vpu_ctx *c, struct vb2_buffer *v,
				 unsigned int n)
{
#ifdef USE_ION_MEMORY
	void *cookie = vb2_plane_cookie(v, n);
	dma_addr_t addr = 0;

	WARN_ON(vb2_ion_dma_address(cookie, &addr) != 0);
	return (unsigned long)addr;
#else
	return vb2_dma_contig_plane_dma_addr(v, n);
#endif
}

int nx_vpu_try_run(struct nx_vpu_ctx *ctx)
{
	struct nx_vpu_v4l2 *dev = ctx->dev;
	unsigned int ret = 0;
	void *err = (void *)(&dev->plat_dev->dev);

	FUNC_IN();

	NX_DbgMsg(INFO_MSG, ("cmd = %x\n", ctx->vpu_cmd));

	mutex_lock(&g_vpu_mutex);

#ifdef ENABLE_CLOCK_GATING
	NX_VPU_Clock(1);
#endif

	__set_bit(ctx->idx, &dev->ctx_work_bits);

	switch (ctx->vpu_cmd) {
	case GET_ENC_INSTANCE:
		dev->curr_ctx = ctx->idx;
		ret = vpu_enc_open_instance(ctx);
		if (ret != 0)
			dev_err(err, "Failed to create a new instance\n");
		else
			ctx->vpu_cmd = ENC_RUN;
		break;

	case ENC_RUN:
		if (ctx->is_initialized == 0) {
			ret = vpu_enc_init(ctx);
			if (ret != 0)
				dev_err(err, "enc_init is failed, ret=%d\n",
					ret);
			else
				vpu_enc_get_seq_info(ctx);
		} else {
			ret = vpu_enc_encode_frame(ctx);
			if (ret != 0) {
				dev_err(err, "encode_frame is failed, ret=%d\n",
					ret);
				break;
			}
		}
		break;

	case GET_DEC_INSTANCE:
		dev->curr_ctx = ctx->idx;
		ret = vpu_dec_open_instance(ctx);
		if (ret != 0)
			dev_err(err, "Failed to create a new instance.\n");
		else
			ctx->vpu_cmd = SEQ_INIT;
		break;

	case SEQ_INIT:
		if (ctx->is_initialized == 0) {
			ret = vpu_dec_parse_vid_cfg(ctx);
			if (ret != 0) {
				dev_err(err, "vpu_dec_parse_vfg() is ");
				dev_err(err, "failed, ret = %d\n", ret);
				break;
			}
			ctx->vpu_cmd = SET_FRAME_BUF;
		}
		break;

	case SET_FRAME_BUF:
		if (ctx->is_initialized == 0) {
			ret = vpu_dec_init(ctx);
			if (ret != 0)
				dev_err(err, "dec_init() is failed, ret=%d\n",
					ret);
			else
				ctx->is_initialized = 1;
		}
		ctx->vpu_cmd = DEC_RUN;
		break;

	case DEC_RUN:
		if (ctx->is_initialized) {
			ret = vpu_dec_decode_slice(ctx);
			if (ret != 0) {
				dev_err(err, "decode_slice() is failed, ");
				dev_err(err, "ret = %d\n", ret);
			}
		}
		break;

	case DEC_BUF_FLUSH:
		if (ctx->is_initialized) {
			ret = NX_VpuDecFlush(ctx->hInst);
			if (ret != 0)
				dev_err(err, "dec_flush() is failed\n");
		}
		ctx->vpu_cmd = DEC_RUN;
		break;

	case SEQ_END:
		if (ctx->hInst) {
			dev->curr_ctx = ctx->idx;

			if (ctx->is_encoder)
				ret = NX_VpuEncClose(ctx->hInst,
					(void *)&dev->vpu_event_present);
			else
				ret = NX_VpuDecClose(ctx->hInst,
					(void *)&dev->vpu_event_present);

			if (ret != 0)
				dev_err(err, "Failed to return an instance.\n");

			dev->cur_num_instance--;
			ctx->is_initialized = 0;
		}
		break;

	default:
		ret = -EAGAIN;
	}

	__clear_bit(ctx->idx, &dev->ctx_work_bits);

#ifdef ENABLE_CLOCK_GATING
	NX_VPU_Clock(0);
#endif

	mutex_unlock(&g_vpu_mutex);

	return ret;
}


/*-----------------------------------------------------------------------------
 *      functions for Input/Output format
 *----------------------------------------------------------------------------*/
static struct nx_vpu_fmt formats[] = {
	{
		.name = "YUV 4:2:0 3 Planes",
		.fourcc = V4L2_PIX_FMT_YUV420M,
		.num_planes = 3,
	},
	{
		.name = "YUV 4:2:2 3 Planes",
		.fourcc = V4L2_PIX_FMT_YUV422M,
		.num_planes = 3,
	},
	{
		.name = "YUV 4:4:4 3 Planes",
		.fourcc = V4L2_PIX_FMT_YUV444M,
		.num_planes = 3,
	},
	{
		.name = "Grey 1 Planes",
		.fourcc = V4L2_PIX_FMT_GREY,
		.num_planes = 1,
	},
	{
		.name = "YUV 4:2:0 2 Planes Y/CbCr",
		.fourcc = V4L2_PIX_FMT_NV12M,
		.num_planes = 2,
	},
	{
		.name = "YUV 4:2:0 2 Planes Y/CrCb",
		.fourcc = V4L2_PIX_FMT_NV21M,
		.num_planes = 2,
	},
	{
		.name = "YUV 4:2:2 2 Planes Y/CbCr",
		.fourcc = V4L2_PIX_FMT_NV16M,
		.num_planes = 2,
	},
	{
		.name = "YUV 4:2:2 2 Planes Y/CrCb",
		.fourcc = V4L2_PIX_FMT_NV61M,
		.num_planes = 2,
	},
	{
		.name = "YUV 4:4:4 2 Planes Y/CbCr",
		.fourcc = V4L2_PIX_FMT_NV24M,
		.num_planes = 2,
	},
	{
		.name = "YUV 4:4:4 2 Planes Y/CrCb",
		.fourcc = V4L2_PIX_FMT_NV42M,
		.num_planes = 2,
	},
	{
		.name = "MPEG2 Stream",
		.fourcc = V4L2_PIX_FMT_MPEG2,
		.num_planes = 1,
	},
	{
		.name = "MPEG4 Stream",
		.fourcc = V4L2_PIX_FMT_MPEG4,
		.num_planes = 1,
	},
	{
		.name = "XVID Stream",
		.fourcc = V4L2_PIX_FMT_XVID,
		.num_planes = 1,
	},
	{
		.name = "DIV3 Stream",
		.fourcc = V4L2_PIX_FMT_DIV3,
		.num_planes = 1,
	},
	{
		.name = "DIV4 Stream",
		.fourcc = V4L2_PIX_FMT_DIV4,
		.num_planes = 1,
	},
	{
		.name = "DIV5 Stream",
		.fourcc = V4L2_PIX_FMT_DIV5,
		.num_planes = 1,
	},
	{
		.name = "DIV6 Stream",
		.fourcc = V4L2_PIX_FMT_DIV6,
		.num_planes = 1,
	},
	{
		.name = "DIVX Stream",
		.fourcc = V4L2_PIX_FMT_DIVX,
		.num_planes = 1,
	},
	{
		.name = "H263 Stream",
		.fourcc = V4L2_PIX_FMT_H263,
		.num_planes = 1,
	},
	{
		.name = "H264 Stream",
		.fourcc = V4L2_PIX_FMT_H264,
		.num_planes = 1,
	},
	{
		.name = "WMV9 Stream",
		.fourcc = V4L2_PIX_FMT_WMV9,
		.num_planes = 1,
	},
	{
		.name = "VC1 Stream",
		.fourcc = V4L2_PIX_FMT_WVC1,
		.num_planes = 1,
	},
	{
		.name = "RV8 Stream",
		.fourcc = V4L2_PIX_FMT_RV8,
		.num_planes = 1,
	},
	{
		.name = "RV9/10 Stream",
		.fourcc = V4L2_PIX_FMT_RV9,
		.num_planes = 1,
	},
	{
		.name = "VP8 Stream",
		.fourcc = V4L2_PIX_FMT_VP8,
		.num_planes = 1,
	},
	{
		.name = "SORENSON SPARK Stream",
		.fourcc = V4L2_PIX_FMT_FLV1,
		.num_planes = 1,
	},
	{
		.name = "THEORA Stream",
		.fourcc = V4L2_PIX_FMT_THEORA,
		.num_planes = 1,
	},
	{
		.name = "JPEG Stream",
		.fourcc = V4L2_PIX_FMT_MJPEG,
		.num_planes = 1,
	},
};
#define NUM_FORMATS ARRAY_SIZE(formats)

struct nx_vpu_fmt *find_format(struct v4l2_format *f)
{
	unsigned int i;

	FUNC_IN();

	for (i = 0 ; i < NUM_FORMATS ; i++) {
		if (formats[i].fourcc == f->fmt.pix_mp.pixelformat)
			return &formats[i];
	}
	return NULL;
}

static int vidioc_enum_fmt(struct v4l2_fmtdesc *f, bool mplane, bool out)
{
	struct nx_vpu_fmt *fmt;
	int i, j = 0;

	FUNC_IN();

	for (i = 0; i < ARRAY_SIZE(formats); ++i) {
		if (mplane && formats[i].num_planes == 1)
			continue;
		else if (!mplane && formats[i].num_planes > 1)
			continue;

		if (j == f->index) {
			fmt = &formats[i];
			strlcpy(f->description, fmt->name,
				sizeof(f->description));
			f->pixelformat = fmt->fourcc;

			return 0;
		}

		++j;
	}

	return -EINVAL;
}
/* -------------------------------------------------------------------------- */


/*-----------------------------------------------------------------------------
 *      functions for vidioc_queryctrl
 *----------------------------------------------------------------------------*/

/* Query capabilities of the device */
int vidioc_querycap(struct file *file, void *priv, struct v4l2_capability *cap)
{
	struct nx_vpu_v4l2 *dev = video_drvdata(file);

	FUNC_IN();

	strncpy(cap->driver, dev->plat_dev->name, sizeof(cap->driver) - 1);
	strncpy(cap->card, dev->plat_dev->name, sizeof(cap->card) - 1);
	cap->bus_info[0] = 0;
	cap->version = KERNEL_VERSION(1, 0, 0);
	cap->device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

int vidioc_enum_fmt_vid_cap(struct file *file, void *pirv,
	struct v4l2_fmtdesc *f)
{
	FUNC_IN();
	return vidioc_enum_fmt(f, false, false);
}

int vidioc_enum_fmt_vid_cap_mplane(struct file *file, void *pirv,
	struct v4l2_fmtdesc *f)
{
	FUNC_IN();
	return vidioc_enum_fmt(f, true, false);
}

int vidioc_enum_fmt_vid_out(struct file *file, void *prov,
	struct v4l2_fmtdesc *f)
{
	FUNC_IN();
	return vidioc_enum_fmt(f, false, true);
}

int vidioc_enum_fmt_vid_out_mplane(struct file *file, void *priv,
	struct v4l2_fmtdesc *f)
{
	FUNC_IN();
	return vidioc_enum_fmt(f, true, true);
}

int vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	int ret = 0;

	FUNC_IN();

	/* if memory is not mmp or userptr return error */
	if ((buf->memory != V4L2_MEMORY_MMAP) &&
		(buf->memory != V4L2_MEMORY_USERPTR) &&
		(buf->memory != V4L2_MEMORY_DMABUF))
		return -EINVAL;

	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = vb2_querybuf(&ctx->vq_strm, buf);
		if (ret != 0) {
			pr_err("error in vb2_querybuf() for E(D)\n");
			return ret;
		}
	} else if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = vb2_querybuf(&ctx->vq_img, buf);
		if (ret != 0) {
			pr_err("error in vb2_querybuf() for E(S)\n");
			return ret;
		}
	} else {
		pr_err("invalid buf type\n");
		return -EINVAL;
	}

	return ret;
}

/* Stream on */
int vidioc_streamon(struct file *file, void *priv,
			   enum v4l2_buf_type type)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	int ret = -EINVAL;

	FUNC_IN();

	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		ret = vb2_streamon(&ctx->vq_img, type);
	else if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		ret = vb2_streamon(&ctx->vq_strm, type);

	return ret;
}

/* Stream off, which equals to a pause */
int vidioc_streamoff(struct file *file, void *priv,
			    enum v4l2_buf_type type)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	int ret = -EINVAL;

	FUNC_IN();

	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		ret = vb2_streamoff(&ctx->vq_img, type);
	else if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		ret = vb2_streamoff(&ctx->vq_strm, type);

	return ret;
}

/* -------------------------------------------------------------------------- */


/*-----------------------------------------------------------------------------
 *      functions for VB2 Contorls(struct "vb2_ops")
 *----------------------------------------------------------------------------*/

static int check_vb_with_fmt(struct nx_vpu_fmt *fmt, struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct nx_vpu_ctx *ctx = vq->drv_priv;
	int i;

	FUNC_IN();

	if (!fmt)
		return -EINVAL;

	if (fmt->num_planes != vb->num_planes) {
		NX_ErrMsg(("invalid plane number for the format(%d, %d)\n",
			fmt->num_planes, vb->num_planes));
		return -EINVAL;
	}

	for (i = 0; i < fmt->num_planes; i++) {
		if (!nx_vpu_mem_plane_addr(ctx, vb, i)) {
			NX_ErrMsg(("failed to get %d plane cookie\n", i));
			return -EINVAL;
		}

		NX_DbgMsg(INFO_MSG, ("index: %d, plane[%d] cookie: 0x%08lx\n",
			vb->index, i,
			(unsigned long)nx_vpu_mem_plane_addr(ctx, vb, i)));
	}

	return 0;
}

int nx_vpu_queue_setup(struct vb2_queue *vq,
			const void *parg,
			unsigned int *buf_count, unsigned int *plane_count,
			unsigned int psize[], void *allocators[])
{
	struct nx_vpu_ctx *ctx = vq->drv_priv;
	int i;

	FUNC_IN();

	if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (ctx->strm_fmt)
			*plane_count = ctx->strm_fmt->num_planes;
		else
			*plane_count = 1;

		if (*buf_count < 1)
			*buf_count = 1;
		if (*buf_count > VPU_MAX_BUFFERS)
			*buf_count = VPU_MAX_BUFFERS;

		psize[0] = ctx->strm_buf_size;
		allocators[0] = ctx->dev->alloc_ctx;
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		int cnt = (ctx->is_encoder) ? (1) :
			(ctx->codec.dec.frame_buffer_cnt);

		if (ctx->img_fmt.num_planes)
			*plane_count = ctx->img_fmt.num_planes;
		else
			*plane_count = (ctx->chromaInterleave) ? (2) : (3);

		if (*buf_count < cnt)
			*buf_count = cnt;
		if (*buf_count > VPU_MAX_BUFFERS)
			*buf_count = VPU_MAX_BUFFERS;

		psize[0] = ctx->luma_size;
		psize[1] = ctx->chroma_size;
		psize[2] = ctx->chroma_size;

		allocators[0] = ctx->dev->alloc_ctx;
		allocators[1] = ctx->dev->alloc_ctx;
		allocators[2] = ctx->dev->alloc_ctx;
	} else {
		NX_ErrMsg(("invalid queue type: %d\n", vq->type));
		return -EINVAL;
	}

	NX_DbgMsg(INFO_MSG, ("buf_count: %d, plane_count: %d\n", *buf_count,
		*plane_count));

	for (i = 0; i < *plane_count; i++)
		NX_DbgMsg(INFO_MSG, ("plane[%d] size=%d\n", i, psize[i]));

	return 0;
}

void nx_vpu_unlock(struct vb2_queue *q)
{
	FUNC_IN();
	mutex_unlock(&g_vpu_mutex);
}

void nx_vpu_lock(struct vb2_queue *q)
{
	FUNC_IN();
	mutex_lock(&g_vpu_mutex);
}

int nx_vpu_buf_init(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct nx_vpu_ctx *ctx = vq->drv_priv;
	/*struct nx_vpu_buf *buf = vb_to_vpu_buf(vb);*/
	int ret;

	FUNC_IN();

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = check_vb_with_fmt(&ctx->img_fmt, vb);
		if (ret < 0)
			return ret;

		/*buf->planes.raw.y = nx_vpu_mem_plane_addr(ctx, vb, 0);
		buf->planes.raw.cb = nx_vpu_mem_plane_addr(ctx, vb, 1);
		buf->planes.raw.cr = nx_vpu_mem_plane_addr(ctx, vb, 2);*/
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = check_vb_with_fmt(ctx->strm_fmt, vb);
		if (ret < 0)
			return ret;

		/*buf->planes.stream = nx_vpu_mem_plane_addr(ctx, vb, 0);*/
	} else {
		NX_ErrMsg(("inavlid queue type: %d\n", vq->type));
		return -EINVAL;
	}

	return 0;
}

int nx_vpu_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct nx_vpu_ctx *ctx = vq->drv_priv;
	int ret;

	FUNC_IN();

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = check_vb_with_fmt(&ctx->img_fmt, vb);
		if (ret < 0)
			return ret;

		NX_DbgMsg(INFO_MSG, ("plane size: %ld, luma size: %d\n",
			vb2_plane_size(vb, 0), ctx->luma_size));

		if (vb2_plane_size(vb, 0) < ctx->luma_size) {
			NX_ErrMsg(("plane size is too small for luma\n"));
			return -EINVAL;
		}

		if (ctx->img_fmt.num_planes > 1) {
			NX_DbgMsg(INFO_MSG, ("plane size:%ld, chroma size:%d\n",
				vb2_plane_size(vb, 1), ctx->chroma_size));

			if (vb2_plane_size(vb, 1) < ctx->chroma_size) {
				NX_ErrMsg(("plane size is small for chroma\n"));
				return -EINVAL;
			}
		}
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = check_vb_with_fmt(ctx->strm_fmt, vb);
		if (ret < 0)
			return ret;

		NX_DbgMsg(INFO_MSG, ("plane size: %ld, strm size: %d\n",
			vb2_plane_size(vb, 0), ctx->strm_buf_size));

		if (vb2_plane_size(vb, 0) < ctx->strm_buf_size) {
			pr_err("plane size is too small for stream\n");
			return -EINVAL;
		}
	} else {
		NX_ErrMsg(("inavlid queue type: %d\n", vq->type));
		return -EINVAL;
	}

	return 0;
}

void nx_vpu_cleanup_queue(struct list_head *lh, struct vb2_queue *vq)
{
	struct nx_vpu_buf *b;
	int i;

	FUNC_IN();

	while (!list_empty(lh)) {
		b = list_entry(lh->next, struct nx_vpu_buf, list);
		for (i = 0; i < b->vb.num_planes; i++)
			vb2_set_plane_payload(&b->vb, i, 0);
		vb2_buffer_done(&b->vb, VB2_BUF_STATE_ERROR);
		list_del(&b->list);
	}
}

/* -------------------------------------------------------------------------- */


/*-----------------------------------------------------------------------------
 *      Linux VPU/JPU Interrupt Handler
 *----------------------------------------------------------------------------*/

static irqreturn_t nx_vpu_irq(int irq, void *priv)
{
	struct nx_vpu_v4l2 *dev = priv;

	FUNC_IN();
	VpuWriteReg(BIT_INT_CLEAR, 0x1);

	/* Reset the timeout watchdog */
	atomic_set(&dev->vpu_event_present, 1);
	wake_up_interruptible(&dev->vpu_wait_queue);

	return IRQ_HANDLED;
}

static int VPU_WaitVpuInterrupt(struct nx_vpu_v4l2 *dev, int timeOut)
{
	int ret = wait_event_interruptible_timeout(dev->vpu_wait_queue,
		atomic_read(&dev->vpu_event_present),
		msecs_to_jiffies(timeOut));

	if (0 == atomic_read(&dev->vpu_event_present)) {
		/* Error */
		if (ret == 0) {
			NX_ErrMsg(("VPU HW Timeout!\n"));
			atomic_set(&dev->vpu_event_present, 0);
			VPU_SWReset(SW_RESET_SAFETY);
			return -1;
		}

		while (timeOut > 0) {
			if (0 != VpuReadReg(BIT_INT_REASON)) {
				atomic_set(&dev->vpu_event_present, 0);
				return 0;
			}
			DrvMSleep(1);
			timeOut--;
		}

		/* Time out */
		NX_ErrMsg(("VPU HW Error!!\n"));
		VPU_SWReset(SW_RESET_SAFETY);
		atomic_set(&dev->vpu_event_present, 0);
		return -1;
	}

	atomic_set(&dev->vpu_event_present, 0);
	return 0;
}

int VPU_WaitBitInterrupt(void *devHandle, int mSeconds)
{
	unsigned int reason = 0;

#ifdef ENABLE_INTERRUPT_MODE
	if (0 != VPU_WaitVpuInterrupt(devHandle, mSeconds)) {
		reason = VpuReadReg(BIT_INT_REASON);
		VpuWriteReg(BIT_INT_REASON, 0);
		NX_ErrMsg(("VPU_WaitVpuInterrupt() TimeOut!!!\n"));
		NX_ErrMsg(("reason = 0x%.8x, CurPC(0xBD 0xBF : %x %x %x))\n",
			reason, VpuReadReg(BIT_CUR_PC), VpuReadReg(BIT_CUR_PC),
			VpuReadReg(BIT_CUR_PC)));
		return 0;
	}

	VpuWriteReg(BIT_INT_CLEAR, 1);  /* clear HW signal */
	reason = VpuReadReg(BIT_INT_REASON);
	VpuWriteReg(BIT_INT_REASON, 0);
	return reason;
#else
	while (mSeconds > 0) {
		reason = VpuReadReg(BIT_INT_REASON);
		if (reason != 0) {
			if (reason != (unsigned int)-1)
				VpuWriteReg(BIT_INT_CLEAR, 1);
			/* tell to F/W that HOST received an interrupt. */
			VpuWriteReg(BIT_INT_REASON, 0);
			break;
		}
		DrvMSleep(1);
		mSeconds--;
	}
	return reason;
#endif
}

static irqreturn_t nx_jpu_irq(int irq, void *priv)
{
	struct nx_vpu_v4l2 *dev = priv;
	uint32_t val;

	FUNC_IN();

	val = VpuReadReg(MJPEG_PIC_STATUS_REG);
	if (val != 0)
		VpuWriteReg(MJPEG_PIC_STATUS_REG, val);
	dev->jpu_intr_reason = val;

	/* Reset the timeout watchdog */
	atomic_set(&dev->jpu_event_present, 1);
	wake_up_interruptible(&dev->jpu_wait_queue);

	return IRQ_HANDLED;
}

int JPU_WaitInterrupt(void *devHandle, int timeOut)
{
	struct nx_vpu_v4l2 *dev = (struct nx_vpu_v4l2 *)devHandle;
	uint32_t reason = 0;

#ifdef ENABLE_INTERRUPT_MODE
	if (0 == wait_event_interruptible_timeout(dev->jpu_wait_queue,
		atomic_read(&dev->jpu_event_present),
		msecs_to_jiffies(timeOut))) {
		reason = VpuReadReg(MJPEG_PIC_STATUS_REG);
		NX_ErrMsg(("JPU_WaitInterrupt() TimeOut!!!(reason = 0x%.8x)\n",
			reason));
		VPU_SWReset(SW_RESET_SAFETY);
		return 0;
	}

	atomic_set(&dev->jpu_event_present, 0);
	reason = dev->jpu_intr_reason;
#else
	while (timeOut > 0) {
		DrvMSleep(1);

		reason = VpuReadReg(MJPEG_PIC_STATUS_REG);
		if ((reason & (1<<INT_JPU_DONE)) ||
			(reason & (1<<INT_JPU_ERROR)) ||
			(reason & (1<<INT_JPU_BBC_INTERRUPT)) ||
			(reason & (1<<INT_JPU_BIT_BUF_EMPTY)))
			break;

		if (reason & (1<<INT_JPU_BIT_BUF_FULL)) {
			NX_ErrMsg(("Stream Buffer Too Small!!!"));
			VpuReadReg(MJPEG_PIC_STATUS_REG,
				(1 << INT_JPU_BIT_BUF_FULL));
			return reason;
		}

		timeOut--;
		if (timeOut == 0) {
			NX_ErrMsg(("JPU TimeOut!!!"));
			break;
		}
	}
#endif

	return reason;
}

/* -------------------------------------------------------------------------- */


static int nx_vpu_open(struct file *file)
{
	struct video_device *vdev = video_devdata(file);
	struct nx_vpu_v4l2 *dev = video_drvdata(file);
	struct nx_vpu_ctx *ctx = NULL;
	void *err = (void *)(&dev->plat_dev->dev);
	int ret = 0;

	FUNC_IN();

	if (mutex_lock_interruptible(&g_vpu_mutex))
		return -ERESTARTSYS;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		NX_ErrMsg(("Not enough memory.\n"));
		ret = -ENOMEM;
		goto err_ctx_mem;
	}

	/* get context number */
	ctx->idx = 0;
	while (dev->ctx[ctx->idx]) {
		ctx->idx++;
		if (ctx->idx >= NX_MAX_VPU_INSTANCE) {
			dev_err(err, "Can't open nx vpu driver!!\n");
			dev_err(err, "CurNumInstance = %d)\n",
				dev->cur_num_instance);
			ret = -EBUSY;
			goto err_no_ctx;
		}
	}

	v4l2_fh_init(&ctx->fh, vdev);
	file->private_data = &ctx->fh;
	v4l2_fh_add(&ctx->fh);

	ctx->dev = dev;

	INIT_LIST_HEAD(&ctx->img_queue);
	INIT_LIST_HEAD(&ctx->strm_queue);
	ctx->img_queue_cnt = 0;
	ctx->strm_queue_cnt = 0;

	/* Mark context as idle */
	__clear_bit(ctx->idx, &dev->ctx_work_bits);
	dev->ctx[ctx->idx] = ctx;

	if (vdev == dev->vfd_enc) {
		ctx->is_encoder = 1;
		ret = nx_vpu_enc_open(ctx);
	} else {
		ctx->is_encoder = 0;
		ret = nx_vpu_dec_open(ctx);
	}
	if (ret)
		goto err_ctx_init;

	/* FW Download, HW Init, Clock Set */
	if (num_open_inst == 0) {
#ifdef CONFIG_ARM_S5Pxx18_DEVFREQ
		nx_vpu_qos_update(NX_BUS_CLK_VPU_KHZ);
#endif
#ifdef ENABLE_POWER_SAVING
		dev->curr_ctx = ctx->idx;

		NX_VPU_Clock(1);
		ret = NX_VpuInit(dev, dev->regs_base,
			dev->firmware_buf->virAddr,
			(uint32_t)dev->firmware_buf->phyAddr);

#ifdef ENABLE_CLOCK_GATING
		NX_VPU_Clock(0);
#endif

		if (ret)
			goto err_hw_init;
#endif
	}

	num_open_inst++;
	mutex_unlock(&g_vpu_mutex);

	return ret;

	/* Deinit when failure occurred */
err_hw_init:
err_ctx_init:
	if (ctx->idx < NX_MAX_VPU_INSTANCE){
		dev->ctx[ctx->idx] = 0;
	}
err_no_ctx:

err_ctx_mem:
	mutex_unlock(&g_vpu_mutex);

	return ret;
}

static int nx_vpu_close(struct file *file)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	struct nx_vpu_v4l2 *dev = ctx->dev;

	FUNC_IN();

	mutex_lock(&g_vpu_mutex);
	mutex_lock(&dev->dev_mutex);

	if (ctx->is_initialized) {
		ctx->vpu_cmd = SEQ_END;
		mutex_unlock(&g_vpu_mutex);
		nx_vpu_try_run(ctx);
		mutex_lock(&g_vpu_mutex);

		if (ctx->is_encoder)
			free_encoder_memory(ctx);
		else
			free_decoder_memory(ctx);
	}

	num_open_inst--;
	if (num_open_inst == 0) {
#ifdef ENABLE_POWER_SAVING
		/* H/W Power Off */
		NX_VPU_Clock(1);
		NX_VpuDeInit(dev);
#endif
#ifdef CONFIG_ARM_S5Pxx18_DEVFREQ
		nx_vpu_qos_update(NX_BUS_CLK_IDLE_KHZ);
#endif
	}

#ifdef ENABLE_CLOCK_GATING
	NX_VPU_Clock(0);
#endif

	vb2_queue_release(&ctx->vq_img);
	vb2_queue_release(&ctx->vq_strm);

	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);

	/* Mark context as idle */
	__clear_bit(ctx->idx, &dev->ctx_work_bits);

	dev->ctx[ctx->idx] = 0;
	kfree(ctx);

	mutex_unlock(&dev->dev_mutex);
	mutex_unlock(&g_vpu_mutex);

	return 0;
}

static unsigned int nx_vpu_poll(struct file *file, struct poll_table_struct
	*wait)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	int ret;

	FUNC_IN();

	ret = vb2_poll(&ctx->vq_img, file, wait);
	if (ret == 0)
		ret = vb2_poll(&ctx->vq_strm, file, wait);

	return ret;
}

#define	DST_QUEUE_OFF_BASE	(1 << 30)
static int nx_vpu_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	struct nx_vpu_v4l2 *dev = ctx->dev;
	uint32_t offset = vma->vm_pgoff << PAGE_SHIFT;
	int ret;

	FUNC_IN();

	if (mutex_lock_interruptible(&dev->dev_mutex))
		return -ERESTARTSYS;

	if (offset < DST_QUEUE_OFF_BASE) {
		ret = vb2_mmap(&ctx->vq_strm, vma);
	} else {
		vma->vm_pgoff -= (DST_QUEUE_OFF_BASE >> PAGE_SHIFT);
		ret = vb2_mmap(&ctx->vq_img, vma);
	}

	mutex_unlock(&dev->dev_mutex);

	return ret;
}

static const struct v4l2_file_operations nx_vpu_fops = {
	.owner = THIS_MODULE,
	.open = nx_vpu_open,
	.release = nx_vpu_close,
	.poll = nx_vpu_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap = nx_vpu_mmap,
};

void vpu_soc_peri_reset_enter(void *pv)
{
	struct nx_vpu_v4l2 *dev = (struct nx_vpu_v4l2 *)pv;

	reset_control_assert(dev->coda_c);
	reset_control_assert(dev->coda_a);
	reset_control_assert(dev->coda_p);
}

void vpu_soc_peri_reset_exit(void *pv)
{
	struct nx_vpu_v4l2 *dev = (struct nx_vpu_v4l2 *)pv;

	reset_control_deassert(dev->coda_c);
	reset_control_deassert(dev->coda_a);
	reset_control_deassert(dev->coda_p);
}

static int nx_vpu_init(struct nx_vpu_v4l2 *dev)
{
	int ret = 0;

	FUNC_IN();

	dev->firmware_buf = nx_alloc_memory(&dev->plat_dev->dev,
		COMMON_BUF_SIZE, 4096);
	if (dev->firmware_buf == NULL) {
		dev_err(&dev->plat_dev->dev, "firmware allocation is failed!\n");
		return -ENOMEM;
	}

	mutex_lock(&g_vpu_mutex);

	NX_VPU_Clock(1);

	ret = NX_VpuInit(dev, dev->regs_base, dev->firmware_buf->virAddr,
		dev->firmware_buf->phyAddr);

#ifdef ENABLE_CLOCK_GATING
	NX_VPU_Clock(0);
#endif

	mutex_unlock(&g_vpu_mutex);

	dev->cur_num_instance = 0;
	dev->cur_jpg_instance = 0;

	return ret;
}

static int nx_vpu_deinit(struct nx_vpu_v4l2 *dev)
{
	int ret;

	FUNC_IN();

	NX_VPU_Clock(1);
	ret = NX_VpuDeInit(dev);
#ifdef ENABLE_CLOCK_GATING
	NX_VPU_Clock(0);
#endif

	if (dev->firmware_buf != NULL)
		nx_free_memory(dev->firmware_buf);

	return ret;
}

static int nx_vpu_probe(struct platform_device *pdev)
{
	struct nx_vpu_v4l2 *dev;
	struct video_device *vfd;
	struct resource res;
	int ret, irq;
	uint32_t info[2] = { };

	FUNC_IN();
	num_open_inst = 0;

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		NX_ErrMsg(("fail to kzalloc(size %zu) (%s)\n",
			sizeof(struct nx_vpu_v4l2), NX_VIDEO_NAME));
		return -ENOMEM;
	}

	spin_lock_init(&dev->irqlock);

	dev->plat_dev = pdev;
	if (!dev->plat_dev) {
		dev_err(&pdev->dev, "No platform data specified\n");
		return -ENODEV;
	}

	ret = of_address_to_resource(pdev->dev.of_node, 0, &res);
	if (ret) {
		dev_err(&pdev->dev, "failed to get base address\n");
		return -ENXIO;
	}

	dev->regs_base = devm_ioremap_nocache(&pdev->dev, res.start,
		resource_size(&res));
	if (!dev->regs_base) {
		dev_err(&pdev->dev, "failed to ioremap\n");
		return -EBUSY;
	}

	/* For VPU interrupt */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "failed to get vpu-irq num\n");
		return -EBUSY;
	}

	ret = devm_request_irq(&pdev->dev, irq, nx_vpu_irq, 0, pdev->name, dev);
	if (ret != 0) {
		dev_err(&pdev->dev, "Failed to install vpu-irq (%d)\n", ret);
		return ret;
	}
	init_waitqueue_head(&dev->vpu_wait_queue);

	/* For JPU interrupt */
	irq = platform_get_irq(pdev, 1);
	if (irq < 0) {
		dev_err(&pdev->dev, "failed to get jpu-irq num\n");
		return -EBUSY;
	}

	ret = devm_request_irq(&pdev->dev, irq, nx_jpu_irq, 0, pdev->name, dev);
	if (ret != 0) {
		dev_err(&pdev->dev, "Failed to install jpu-irq (%d)\n", ret);
		return ret;
	}
	init_waitqueue_head(&dev->jpu_wait_queue);

	dev->coda_c = devm_reset_control_get(&pdev->dev, "vpu-c-reset");
	if (IS_ERR(dev->coda_c)) {
		dev_err(&pdev->dev, "failed to get reset control of vpu-c\n");
		return -ENODEV;
	}

	dev->coda_a = devm_reset_control_get(&pdev->dev, "vpu-a-reset");
	if (IS_ERR(dev->coda_a)) {
		dev_err(&pdev->dev, "failed to get reset control of vpu-c\n");
		return -ENODEV;
	}

	dev->coda_p = devm_reset_control_get(&pdev->dev, "vpu-p-reset");
	if (IS_ERR(dev->coda_p)) {
		dev_err(&pdev->dev, "failed to get reset control of vpu-c\n");
		return -ENODEV;
	}

	ret = of_property_read_u32_array(pdev->dev.of_node, "sram", info, 2);
	if (!ret) {
		dev->sram_base_addr = info[0];
		dev->sram_size = info[1];
	}

	/* alloc context : use vb2 dma contig   */
	dev->alloc_ctx = vb2_dma_contig_init_ctx(&pdev->dev);
	if (!dev->alloc_ctx) {
		dev_err(&pdev->dev, "%s: failed to dma alloc context\n",
			__func__);
		return -ENOMEM;
	}

	mutex_init(&dev->dev_mutex);
	mutex_init(&g_vpu_mutex);

	ret = v4l2_device_register(&pdev->dev, &dev->v4l2_dev);
	if (ret) {
		dev_err(&pdev->dev, "%s: failed to register v4l2_device: %d\n",
			__func__, ret);
		goto err_v4l2_dev_reg;
	}

	/* encoder */
	vfd = video_device_alloc();
	if (!vfd) {
		dev_err(&pdev->dev, "Fail to allocate video device\n");
		ret = -ENOMEM;
		goto err_enc_alloc;
	}

	vfd->fops = &nx_vpu_fops;
	vfd->ioctl_ops = get_enc_ioctl_ops();
	vfd->minor = -1;
	vfd->release = video_device_release;
	vfd->lock = &dev->dev_mutex;
	vfd->v4l2_dev = &dev->v4l2_dev;
	vfd->vfl_dir = VFL_DIR_M2M;
	snprintf(vfd->name, sizeof(vfd->name), "%s", NX_VIDEO_ENC_NAME);
	dev->vfd_enc = vfd;

	v4l2_info(&dev->v4l2_dev, "encoder registered as /dev/video%d\n",
		NX_VPU_START);
	video_set_drvdata(vfd, dev);
	ret = video_register_device(vfd, VFL_TYPE_GRABBER, NX_VPU_START);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register video device\n");
		goto err_enc_reg;
	}

	/* decoder */
	vfd = video_device_alloc();
	if (!vfd) {
		dev_err(&pdev->dev, "Fail to allocate video device\n");
		ret = -ENOMEM;
		goto err_dec_alloc;
	}

	vfd->fops = &nx_vpu_fops;
	vfd->ioctl_ops = get_dec_ioctl_ops();
	vfd->minor = -1;
	vfd->release = video_device_release;
	vfd->lock = &dev->dev_mutex;
	vfd->v4l2_dev = &dev->v4l2_dev;
	vfd->vfl_dir = VFL_DIR_M2M;
	snprintf(vfd->name, sizeof(vfd->name), "%s", NX_VIDEO_DEC_NAME);
	dev->vfd_dec = vfd;

	v4l2_info(&dev->v4l2_dev, "decoder registered as /dev/video%d\n",
		NX_VPU_START + 1);
	video_set_drvdata(vfd, dev);
	ret = video_register_device(vfd, VFL_TYPE_GRABBER, NX_VPU_START + 1);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register video device\n");
		goto err_dec_reg;
	}

	platform_set_drvdata(pdev, dev);

	atomic_set(&dev->vpu_event_present, 0);
	atomic_set(&dev->jpu_event_present, 0);

	ret = NX_VpuParaInitialized(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to ioremap\n");
		goto err_vpu_init;
	}

	ret = nx_vpu_init(dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "nx_vpu_init() is Failed\n");
		goto err_vpu_init;
	}

	return 0;

err_vpu_init:
err_dec_reg:
	video_unregister_device(dev->vfd_dec);
err_dec_alloc:
	video_device_release(dev->vfd_dec);
err_enc_alloc:
	video_unregister_device(dev->vfd_enc);
err_enc_reg:
	video_device_release(dev->vfd_enc);
err_v4l2_dev_reg:
	v4l2_device_unregister(&dev->v4l2_dev);

	vb2_dma_contig_cleanup_ctx(dev->alloc_ctx);

	dev_err(&pdev->dev, "%s-- with error!!!\n", __func__);
	return ret;
}

static int nx_vpu_remove(struct platform_device *pdev)
{
	struct nx_vpu_v4l2 *dev = platform_get_drvdata(pdev);

	FUNC_IN();

	if (unlikely(!dev))
		return 0;

	if (dev->cur_num_instance > 0) {
		dev_err(&pdev->dev, "Warning Video Frimware is running.\n");
		dev_err(&pdev->dev, "(Video(%d), Jpeg(%d)\n",
			dev->cur_num_instance, dev->cur_jpg_instance);
	}

	video_unregister_device(dev->vfd_enc);
	video_unregister_device(dev->vfd_dec);
	v4l2_device_unregister(&dev->v4l2_dev);

	reset_control_assert(dev->coda_c);
	reset_control_assert(dev->coda_a);
	reset_control_assert(dev->coda_p);

	nx_vpu_deinit(dev);

	if (dev->alloc_ctx)
		vb2_dma_contig_cleanup_ctx(dev->alloc_ctx);

	mutex_destroy(&g_vpu_mutex);
	mutex_destroy(&dev->dev_mutex);

	return 0;
}

static int nx_vpu_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct nx_vpu_v4l2 *dev = platform_get_drvdata(pdev);

	FUNC_IN();

	mutex_lock(&g_vpu_mutex);
	NX_VPU_Clock(1);

	NX_VpuSuspend(dev);

#ifdef ENABLE_CLOCK_GATING
	NX_VPU_Clock(0);
#endif
	mutex_unlock(&g_vpu_mutex);

	FUNC_OUT();
	return 0;
}

static int nx_vpu_resume(struct platform_device *pdev)
{
	struct nx_vpu_v4l2 *dev = platform_get_drvdata(pdev);

	FUNC_IN();

	mutex_lock(&g_vpu_mutex);
	NX_VPU_Clock(1);

	NX_VpuResume(dev, dev->regs_base);

#ifdef ENABLE_CLOCK_GATING
	NX_VPU_Clock(0);
#endif
	mutex_unlock(&g_vpu_mutex);

	FUNC_OUT();
	return 0;
}

static struct platform_device_id nx_vpu_driver_ids[] = {
	{
		.name = NX_VIDEO_NAME, .driver_data = 0,
	},
	{},
};

static const struct of_device_id nx_vpu_dt_match[] = {
	{
	.compatible = "nexell, nx-vpu",
	},
	{},
};
MODULE_DEVICE_TABLE(of, nx_vpu_dt_match);

static struct platform_driver nx_vpu_driver = {
	.probe = nx_vpu_probe,
	.remove = nx_vpu_remove,
	.suspend = nx_vpu_suspend,
	.resume = nx_vpu_resume,
	.id_table = nx_vpu_driver_ids,
	.driver = {
		.name = NX_VIDEO_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(nx_vpu_dt_match),
	},
};

module_platform_driver(nx_vpu_driver);

MODULE_AUTHOR("Kim SeongHee <kshblue@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell S5P6818 series SoC V4L2/Codec device driver");
MODULE_LICENSE("GPL");
