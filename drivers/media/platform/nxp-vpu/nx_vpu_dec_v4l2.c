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

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/videodev2.h>
#include <linux/videodev2_nxp_media.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/interrupt.h>

#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>

#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/cma.h>

#include "vpu_hw_interface.h"
#include "nx_vpu_v4l2.h"


#define INFO_MSG		0

#define PS_SAVE_SIZE (320 * 1024)


static int nx_vpu_dec_ctx_ready(struct nx_vpu_ctx *ctx)
{
	struct vpu_dec_ctx *dec_ctx = &ctx->codec.dec;

	FUNC_IN();

	NX_DbgMsg(INFO_MSG, ("src = %d, dst = %d, dpb = %d\n",
		ctx->strm_queue_cnt, ctx->img_queue_cnt,
		dec_ctx->dpb_queue_cnt));

	if (ctx->vpu_cmd == DEC_RUN) {
		if (ctx->strm_queue_cnt < 1) {
			NX_DbgMsg(INFO_MSG, ("strm_queue_cnt error\n"));
			return 0;
		}
	} else if (ctx->vpu_cmd == SET_FRAME_BUF) {
		if (dec_ctx->dpb_queue_cnt < dec_ctx->frame_buffer_cnt) {
			NX_DbgMsg(INFO_MSG, ("dpb_queue_cnt error\n"));
			return 0;
		}
	}

	return 1;
}

/*-----------------------------------------------------------------------------
 *      functions for Parameter controls
 *----------------------------------------------------------------------------*/
static struct v4l2_queryctrl controls[] = {
	{
		.id = V4L2_CID_MPEG_VIDEO_THUMBNAIL_MODE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Enable thumbnail",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},
};
#define NUM_CTRLS ARRAY_SIZE(controls)

static struct v4l2_queryctrl *get_ctrl(int id)
{
	int i;

	FUNC_IN();

	for (i = 0; i < NUM_CTRLS; ++i)
		if (id == controls[i].id)
			return &controls[i];
	return NULL;
}

static int check_ctrl_val(struct nx_vpu_ctx *ctx, struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl *c;

	FUNC_IN();

	c = get_ctrl(ctrl->id);
	if (!c)
		return -EINVAL;
	if (ctrl->value < c->minimum || ctrl->value > c->maximum
	    || (c->step != 0 && ctrl->value % c->step != 0)) {
		NX_ErrMsg(("Invalid control value\n"));
		NX_ErrMsg(("value = %d, min = %d, max = %d, step = %d\n",
			ctrl->value, c->minimum, c->maximum, c->step));
		return -ERANGE;
	}

	return 0;
}
/* -------------------------------------------------------------------------- */


/*-----------------------------------------------------------------------------
 *      functions for vidioc_queryctrl
 *----------------------------------------------------------------------------*/

static int vidioc_g_fmt_vid_cap_mplane(struct file *file, void *priv,
	struct v4l2_format *f)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;

	FUNC_IN();

	if ((ctx->width == 0) || (ctx->height == 0) ||
		(ctx->codec.dec.frame_buffer_cnt == 0)) {
			NX_ErrMsg(("There is not cfg information!!"));
			return -1;
	}

	pix_mp->width = ctx->width;
	pix_mp->height = ctx->height;
	pix_mp->field = ctx->codec.dec.interlace_flg[0];
	pix_mp->num_planes = 3;
	pix_mp->pixelformat = ctx->imgFourCC;

	pix_mp->plane_fmt[0].bytesperline = ctx->buf_y_width;
	pix_mp->plane_fmt[0].sizeimage = ctx->luma_size;
	pix_mp->plane_fmt[1].bytesperline = ctx->buf_c_width;
	pix_mp->plane_fmt[1].sizeimage = ctx->chroma_size;
	pix_mp->plane_fmt[2].bytesperline = ctx->buf_c_width;
	pix_mp->plane_fmt[2].sizeimage = ctx->chroma_size;

	/* TBD. Patch for fedora */
	if (7 == sizeof(pix_mp->reserved)) {
		pix_mp->reserved[0] = (__u8)ctx->codec.dec.frame_buffer_cnt;
		pix_mp->reserved[1] = (__u8)ctx->codec.dec.frame_buffer_cnt;
	} else if (8 == sizeof(pix_mp->reserved)) {
		pix_mp->reserved[1] = (__u8)ctx->codec.dec.frame_buffer_cnt;
		pix_mp->reserved[2] = (__u8)ctx->codec.dec.frame_buffer_cnt;
	}

	NX_DbgMsg(INFO_MSG, ("vidioc_g_fmt_vid_cap_mplane : W = %d, H = %d\n",
		pix_mp->width, pix_mp->height));

	return 0;
}

static int vidioc_g_fmt_vid_out_mplane(struct file *file, void *priv,
	struct v4l2_format *f)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;

	FUNC_IN();

	NX_DbgMsg(INFO_MSG, ("f->type = %d\n", f->type));

	pix_mp->width = 0;
	pix_mp->height = 0;
	pix_mp->field = V4L2_FIELD_NONE;
	pix_mp->plane_fmt[0].bytesperline = ctx->strm_buf_size;
	pix_mp->plane_fmt[0].sizeimage = ctx->strm_buf_size;
	pix_mp->pixelformat = ctx->strm_fmt->fourcc;
	pix_mp->num_planes = ctx->strm_fmt->num_planes;

	return 0;
}

static int vidioc_try_fmt(struct file *file, void *priv,
	struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;
	struct nx_vpu_fmt *fmt;

	FUNC_IN();

	if ((f->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) &&
		(f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)) {
			NX_ErrMsg(("invalid buf type\n"));
			return -EINVAL;
	}

	fmt = find_format(f);
	if (!fmt) {
		NX_ErrMsg(("failed to try format(%x)\n",
			pix_fmt_mp->pixelformat));
		return -EINVAL;
	}

	if ((fmt->num_planes != pix_fmt_mp->num_planes) &&
		(1 != pix_fmt_mp->num_planes)) {
		NX_ErrMsg(("failed to try format(num of planes error(%d, %d)\n",
			fmt->num_planes, pix_fmt_mp->num_planes));
		return -EINVAL;
	}

	return 0;
}

static int vidioc_s_fmt_vid_cap_mplane(struct file *file, void *priv,
	struct v4l2_format *f)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;
	struct nx_vpu_fmt *img_fmt;
	int ret = 0;

	FUNC_IN();

	if (ctx->vq_img.streaming) {
		NX_ErrMsg(("%s queue busy\n", __func__));
		return -EBUSY;
	}

	ret = vidioc_try_fmt(file, priv, f);
	if (ret) {
		NX_ErrMsg(("failed to try output format\n"));
		return ret;
	}

	img_fmt = find_format(f);

	ctx->img_fmt.name = img_fmt->name;
	ctx->img_fmt.fourcc = img_fmt->fourcc;
	ctx->img_fmt.num_planes = f->fmt.pix_mp.num_planes;

	ctx->chromaInterleave = (pix_fmt_mp->num_planes != 2) ? (0) : (1);

	return 0;
}

static int vidioc_s_fmt_vid_out_mplane(struct file *file, void *priv,
	struct v4l2_format *f)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	int ret = 0;
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;

	FUNC_IN();

	if (ctx->vq_strm.streaming) {
		NX_ErrMsg(("%s queue busy\n", __func__));
		return -EBUSY;
	}

	ret = vidioc_try_fmt(file, priv, f);
	if (ret)
		return ret;

	ctx->strm_fmt = find_format(f);
	ctx->width = pix_fmt_mp->width;
	ctx->height = pix_fmt_mp->height;

	if (pix_fmt_mp->plane_fmt[0].sizeimage)
		ctx->strm_buf_size = pix_fmt_mp->plane_fmt[0].sizeimage;

	ctx->vpu_cmd = GET_DEC_INSTANCE;
	ret = nx_vpu_try_run(ctx);

	return ret;
}

static int vidioc_reqbufs(struct file *file, void *priv,
	struct v4l2_requestbuffers *reqbufs)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	int ret = 0;

	FUNC_IN();

	/* if memory is not mmp or userptr return error */
	if ((reqbufs->memory != V4L2_MEMORY_MMAP) &&
		(reqbufs->memory != V4L2_MEMORY_USERPTR) &&
		(reqbufs->memory != V4L2_MEMORY_DMABUF))
		return -EINVAL;

	if (reqbufs->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = vb2_reqbufs(&ctx->vq_strm, reqbufs);
		if (ret != 0) {
			NX_ErrMsg(("error in vb2_reqbufs() for stream\n"));
			return ret;
		}
	} else if (reqbufs->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if (reqbufs->count == 0)
			return vb2_reqbufs(&ctx->vq_img, reqbufs);

		/* TBD. Additional frame buffer count */
		if ((reqbufs->count + 2) < ctx->codec.dec.frame_buffer_cnt) {
			NX_ErrMsg(("v4l2_requestbuffers : count error\n"));
			reqbufs->count = ctx->codec.dec.frame_buffer_cnt;
			return -ENOMEM;
		}

		ret = vb2_reqbufs(&ctx->vq_img, reqbufs);
		if (ret != 0) {
			NX_ErrMsg(("error in vb2_reqbufs() for Img\n"));
			return ret;
		}

		ctx->codec.dec.frame_buffer_cnt = reqbufs->count;
	} else {
		NX_ErrMsg(("invalid buf type\n"));
		return -EINVAL;
	}

	return 0;
}

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	int ret;

	FUNC_IN();

	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if ((buf->m.planes[0].bytesused == 0)
			&& (ctx->is_initialized)) {
				ctx->codec.dec.flush = 1;
				return ctx->vq_strm.ops->start_streaming(
					&ctx->vq_strm, buf->flags);
			} else {
				ctx->codec.dec.flush = 0;
				return vb2_qbuf(&ctx->vq_strm, buf);
			}
	} else {
		if (ctx->is_initialized) {
			struct vpu_dec_clr_dsp_flag_arg clrFlagArg;

			clrFlagArg.indexFrameDisplay = buf->index;
			NX_DrvMemset(&clrFlagArg.frameBuffer, 0,
				sizeof(clrFlagArg.frameBuffer));

			ret = NX_VpuDecClrDspFlag(ctx->hInst, &clrFlagArg);
			if (ret != VPU_RET_OK) {
				NX_ErrMsg(("ClrDspFlag() failed.(Error=%d)\n",
					ret));
				return ret;
			}
		}

		return vb2_qbuf(&ctx->vq_img, buf);
	}
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	int ret;

	FUNC_IN();

	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = vb2_dqbuf(&ctx->vq_strm, buf, file->f_flags & O_NONBLOCK);
	} else {
		if (ctx->codec.dec.delay_frm == 0) {
			ret = vb2_dqbuf(&ctx->vq_img, buf, file->f_flags &
				O_NONBLOCK);
		} else if (ctx->codec.dec.delay_frm == 1) {
			buf->index = -3;
			ret = 0;
		} else {
			buf->index = -1;
			ret = 0;
		}
	}

	return ret;
}

static int vidioc_g_crop(struct file *file, void *priv, struct v4l2_crop *cr)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	struct vpu_dec_ctx *dec_ctx = &ctx->codec.dec;

	FUNC_IN();

	cr->c.left = dec_ctx->crop_left;
	cr->c.top = dec_ctx->crop_top;
	cr->c.width = dec_ctx->crop_right - dec_ctx->crop_left;
	cr->c.height = dec_ctx->crop_bot - dec_ctx->crop_top;

	return 0;
}

/* Query a ctrl */
static int vidioc_queryctrl(struct file *file, void *priv, struct v4l2_queryctrl
	*qc)
{
	FUNC_IN();
	NX_ErrMsg(("%s will be coded as needed!\n", __func__));
	return 0;
}

static int vidioc_g_ctrl(struct file *file, void *priv, struct v4l2_control
	*ctrl)
{
	FUNC_IN();
	NX_ErrMsg(("%s will be coded as needed!\n", __func__));
	return 0;
}

static int vidioc_s_ctrl(struct file *file, void *priv, struct v4l2_control
	*ctrl)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	int ret = 0;

	FUNC_IN();

	ret = check_ctrl_val(ctx, ctrl);
	if (ret != 0)
		return ret;

	if (ctrl->id == V4L2_CID_MPEG_VIDEO_THUMBNAIL_MODE) {
		ctx->codec.dec.thumbnailMode = ctrl->value;
	} else {
		NX_ErrMsg(("Invalid control(ID = %x)\n", ctrl->id));
		return -EINVAL;
	}

	return ret;
}

static int vidioc_g_ext_ctrls(struct file *file, void *priv,
	struct v4l2_ext_controls *f)
{
	FUNC_IN();
	NX_ErrMsg(("%s will be coded as needed!\n", __func__));
	return 0;
}

static const struct v4l2_ioctl_ops nx_vpu_dec_ioctl_ops = {
	.vidioc_querycap = vidioc_querycap,
	.vidioc_enum_fmt_vid_cap = vidioc_enum_fmt_vid_cap,
	.vidioc_enum_fmt_vid_cap_mplane = vidioc_enum_fmt_vid_cap_mplane,
	.vidioc_enum_fmt_vid_out = vidioc_enum_fmt_vid_out,
	.vidioc_enum_fmt_vid_out_mplane = vidioc_enum_fmt_vid_out_mplane,
	.vidioc_g_fmt_vid_cap_mplane = vidioc_g_fmt_vid_cap_mplane,
	.vidioc_g_fmt_vid_out_mplane = vidioc_g_fmt_vid_out_mplane,
	.vidioc_try_fmt_vid_cap_mplane = vidioc_try_fmt,
	.vidioc_try_fmt_vid_out_mplane = vidioc_try_fmt,
	.vidioc_s_fmt_vid_cap_mplane = vidioc_s_fmt_vid_cap_mplane,
	.vidioc_s_fmt_vid_out_mplane = vidioc_s_fmt_vid_out_mplane,
	.vidioc_reqbufs = vidioc_reqbufs,
	.vidioc_querybuf = vidioc_querybuf,
	.vidioc_qbuf = vidioc_qbuf,
	.vidioc_dqbuf = vidioc_dqbuf,
	.vidioc_streamon = vidioc_streamon,
	.vidioc_streamoff = vidioc_streamoff,
	.vidioc_queryctrl = vidioc_queryctrl,
	.vidioc_g_ctrl = vidioc_g_ctrl,
	.vidioc_s_ctrl = vidioc_s_ctrl,
	.vidioc_g_crop = vidioc_g_crop,
	.vidioc_g_ext_ctrls = vidioc_g_ext_ctrls,
};

/* -------------------------------------------------------------------------- */


static void cleanup_dpb_queue(struct nx_vpu_ctx *ctx)
{
	struct nx_vpu_buf *buf;
	struct vpu_dec_ctx *dec_ctx = &ctx->codec.dec;

	FUNC_IN();

	/* move buffers in dpb queue to img queue */
	while (!list_empty(&dec_ctx->dpb_queue)) {
		buf = list_entry((&dec_ctx->dpb_queue)->next, struct nx_vpu_buf,
			list);

		list_del(&buf->list);
		dec_ctx->dpb_queue_cnt--;

		list_add_tail(&buf->list, &ctx->img_queue);
		ctx->img_queue_cnt++;
	}

	INIT_LIST_HEAD(&dec_ctx->dpb_queue);
	dec_ctx->dpb_queue_cnt = 0;
}

static int nx_vpu_dec_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct nx_vpu_ctx *ctx = q->drv_priv;
	struct vpu_dec_ctx *dec_ctx = &ctx->codec.dec;
	int ret = 0;

	FUNC_IN();

	if ((dec_ctx->flush) || (nx_vpu_dec_ctx_ready(ctx))) {
		dec_ctx->eos_tag = count;
		ret = nx_vpu_try_run(ctx);
	}

	return ret;
}

static void nx_vpu_dec_stop_streaming(struct vb2_queue *q)
{
	unsigned long flags;
	struct nx_vpu_ctx *ctx = q->drv_priv;
	struct nx_vpu_v4l2 *dev = ctx->dev;

	FUNC_IN();

	if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ctx->vpu_cmd = DEC_BUF_FLUSH;
		nx_vpu_try_run(ctx);

		spin_lock_irqsave(&dev->irqlock, flags);

		cleanup_dpb_queue(ctx);
		nx_vpu_cleanup_queue(&ctx->img_queue, &ctx->vq_img);

		INIT_LIST_HEAD(&ctx->img_queue);
		ctx->img_queue_cnt = 0;

		INIT_LIST_HEAD(&ctx->codec.dec.dpb_queue);
		ctx->codec.dec.dpb_queue_cnt = 0;

		spin_unlock_irqrestore(&dev->irqlock, flags);
	} else if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		spin_lock_irqsave(&dev->irqlock, flags);

		nx_vpu_cleanup_queue(&ctx->strm_queue, &ctx->vq_strm);
		INIT_LIST_HEAD(&ctx->strm_queue);
		ctx->strm_queue_cnt = 0;

		spin_unlock_irqrestore(&dev->irqlock, flags);
	}

	ctx->vpu_cmd = SEQ_END;
}

static void nx_vpu_dec_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct nx_vpu_ctx *ctx = vq->drv_priv;
	struct nx_vpu_v4l2 *dev = ctx->dev;
	unsigned long flags;
	struct nx_vpu_buf *buf = vb_to_vpu_buf(vb);

	FUNC_IN();

	buf->used = 0;

	spin_lock_irqsave(&dev->irqlock, flags);

	if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		list_add_tail(&buf->list, &ctx->strm_queue);
		ctx->strm_queue_cnt++;
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		struct vpu_dec_ctx *dec_ctx = &ctx->codec.dec;
		unsigned int idx = dec_ctx->dpb_queue_cnt;

		buf->planes.raw.y = nx_vpu_mem_plane_addr(ctx, vb, 0);
		dec_ctx->frame_buf[idx].phyAddr[0] = buf->planes.raw.y;

		if (ctx->img_fmt.num_planes > 1) {
			buf->planes.raw.cb = nx_vpu_mem_plane_addr(ctx, vb, 1);
			dec_ctx->frame_buf[idx].phyAddr[1] = buf->planes.raw.cb;
		} else if (ctx->chroma_size > 0) {
			dec_ctx->frame_buf[idx].phyAddr[1] = ctx->luma_size +
				dec_ctx->frame_buf[idx].phyAddr[0];
		}

		if (ctx->img_fmt.num_planes > 2) {
			buf->planes.raw.cr = nx_vpu_mem_plane_addr(ctx, vb, 2);
			dec_ctx->frame_buf[idx].phyAddr[2] = buf->planes.raw.cr;
		} else if (ctx->chroma_size > 0 && ctx->chromaInterleave == 0) {
			dec_ctx->frame_buf[idx].phyAddr[2] = ctx->chroma_size +
				dec_ctx->frame_buf[idx].phyAddr[1];
		}

		list_add_tail(&buf->list, &ctx->codec.dec.dpb_queue);
		dec_ctx->dpb_queue_cnt++;
	} else {
		NX_ErrMsg(("unsupported buffer type(%d)\n", vq->type));
		return;
	}

	spin_unlock_irqrestore(&dev->irqlock, flags);

	if (nx_vpu_dec_ctx_ready(ctx))
		nx_vpu_try_run(ctx);
}

static struct vb2_ops nx_vpu_dec_qops = {
	.queue_setup            = nx_vpu_queue_setup,
	.wait_prepare           = nx_vpu_unlock,
	.wait_finish            = nx_vpu_lock,
	.buf_init               = nx_vpu_buf_init,
	.buf_prepare            = nx_vpu_buf_prepare,
	.start_streaming        = nx_vpu_dec_start_streaming,
	.stop_streaming         = nx_vpu_dec_stop_streaming,
	.buf_queue              = nx_vpu_dec_buf_queue,
};

/* -------------------------------------------------------------------------- */


const struct v4l2_ioctl_ops *get_dec_ioctl_ops(void)
{
	return &nx_vpu_dec_ioctl_ops;
}

int nx_vpu_dec_open(struct nx_vpu_ctx *ctx)
{
	int ret = 0;

	FUNC_IN();

	INIT_LIST_HEAD(&ctx->codec.dec.dpb_queue);
	ctx->codec.dec.dpb_queue_cnt = 0;

	/* Init videobuf2 queue for OUTPUT */
	ctx->vq_strm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	ctx->vq_strm.drv_priv = ctx;
	ctx->vq_strm.lock = &ctx->dev->dev_mutex;
	ctx->vq_strm.buf_struct_size = sizeof(struct nx_vpu_buf);
	ctx->vq_strm.io_modes = VB2_USERPTR | VB2_DMABUF;
	ctx->vq_strm.mem_ops = &vb2_dma_contig_memops;
	/*ctx->vq_strm.allow_zero_byteused = 1; */
	ctx->vq_strm.timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	ctx->vq_strm.ops = &nx_vpu_dec_qops;
	ret = vb2_queue_init(&ctx->vq_strm);
	if (ret) {
		NX_ErrMsg(("Failed to initialize videobuf2 queue(output)\n"));
		return ret;
	}

	/* Init videobuf2 queue for CAPTURE */
	ctx->vq_img.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	ctx->vq_img.drv_priv = ctx;
	ctx->vq_img.lock = &ctx->dev->dev_mutex;
	ctx->vq_img.buf_struct_size = sizeof(struct nx_vpu_buf);
	ctx->vq_img.io_modes = VB2_USERPTR | VB2_DMABUF;
	ctx->vq_img.mem_ops = &vb2_dma_contig_memops;
	ctx->vq_img.timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	ctx->vq_img.ops = &nx_vpu_dec_qops;
	ret = vb2_queue_init(&ctx->vq_img);
	if (ret) {
		NX_ErrMsg(("Failed to initialize videobuf2 queue(capture)\n"));
		return ret;
	}

	return 0;
}

static void decoder_flush_disp_info(struct vpu_dec_ctx *dec_ctx)
{
	int32_t i;

	for (i = 0 ; i < VPU_MAX_BUFFERS ; i++) {
		dec_ctx->timeStamp[i].tv_sec = -10;
		dec_ctx->timeStamp[i].tv_usec = -10;
		dec_ctx->frm_type[i] = -1;
		dec_ctx->multiResolution[i] = -0;
		dec_ctx->interlace_flg[i] = -1;
		dec_ctx->reliable_0_100[i] = 0;
		dec_ctx->upSampledWidth[i] = 0;
		dec_ctx->upSampledHeight[i] = 0;
	}

	dec_ctx->savedTimeStamp.tv_sec = -1;
	dec_ctx->savedTimeStamp.tv_usec = -1;
}

int vpu_dec_open_instance(struct nx_vpu_ctx *ctx)
{
	struct nx_vpu_v4l2 *dev = ctx->dev;
	struct nx_vpu_codec_inst *hInst = 0;
	struct vpu_open_arg openArg;
	int workBufSize = WORK_BUF_SIZE;
	int ret = 0;

	FUNC_IN();

	memset(&openArg, 0, sizeof(openArg));

	switch (ctx->strm_fmt->fourcc) {
	case V4L2_PIX_FMT_H264:
		ctx->codec_mode = CODEC_STD_AVC;
		workBufSize += PS_SAVE_SIZE;
		break;
	case V4L2_PIX_FMT_MPEG2:
		ctx->codec_mode = CODEC_STD_MPEG2;
		break;
	case V4L2_PIX_FMT_MPEG4:
		ctx->codec_mode = CODEC_STD_MPEG4;
		break;
	case V4L2_PIX_FMT_XVID:
		ctx->codec_mode = CODEC_STD_MPEG4;
		openArg.mp4Class = 2;
		break;
	case V4L2_PIX_FMT_DIV4:
	case V4L2_PIX_FMT_DIVX:
		ctx->codec_mode = CODEC_STD_MPEG4;
		openArg.mp4Class = 5;
		break;
	case V4L2_PIX_FMT_DIV5:
	case V4L2_PIX_FMT_DIV6:
		ctx->codec_mode = CODEC_STD_MPEG4;
		openArg.mp4Class = 1;
		break;
	case V4L2_PIX_FMT_H263:
		ctx->codec_mode = CODEC_STD_H263;
		break;
	case V4L2_PIX_FMT_DIV3:
		ctx->codec_mode = CODEC_STD_DIV3;
		break;
	case V4L2_PIX_FMT_WMV9:
	case V4L2_PIX_FMT_WVC1:
		ctx->codec_mode = CODEC_STD_VC1;
		break;
	case V4L2_PIX_FMT_RV8:
	case V4L2_PIX_FMT_RV9:
		ctx->codec_mode = CODEC_STD_RV;
		break;
	case V4L2_PIX_FMT_FLV1:
		/* Sorenson spark */
		ctx->codec_mode = CODEC_STD_MPEG4;
		openArg.mp4Class = 256;
		break;
	case V4L2_PIX_FMT_THEORA:
		ctx->codec_mode = CODEC_STD_THO;
		break;
	case V4L2_PIX_FMT_VP8:
		ctx->codec_mode = CODEC_STD_VP8;
		break;
	case V4L2_PIX_FMT_MJPEG:
		ctx->codec_mode = CODEC_STD_MJPG;
		break;
	default:
		NX_ErrMsg(("Invalid codec type(fourcc = %x)!!!\n",
			ctx->strm_fmt->fourcc));
		goto err_exit;
	}
	openArg.codecStd = ctx->codec_mode;

	ctx->bit_stream_buf = nx_alloc_memory(&dev->plat_dev->dev,
		STREAM_BUF_SIZE, 4096);
	if (ctx->bit_stream_buf == NULL) {
		NX_ErrMsg(("Bitstream_buf allocation failed.\n"));
		goto err_exit;
	}

	ctx->instance_buf = nx_alloc_memory(&dev->plat_dev->dev,
		workBufSize, 4096);
	if (ctx->instance_buf == NULL) {
		NX_ErrMsg(("instance_buf allocation failed.\n"));
		goto err_exit;
	}

	openArg.instIndex = ctx->idx;
	openArg.instanceBuf = *ctx->instance_buf;
	openArg.streamBuf = *ctx->bit_stream_buf;

	ret = NX_VpuDecOpen(&openArg, dev, &hInst);
	if ((VPU_RET_OK != ret) || (0 == hInst)) {
		NX_ErrMsg(("Cannot open VPU Instance!!\n"));
		NX_ErrMsg(("  codecStd=%d, is_encoder=%d, hInst=%p)\n",
			openArg.codecStd, openArg.isEncoder, hInst));
		goto err_exit;
	}

	decoder_flush_disp_info(&ctx->codec.dec);

	ctx->hInst = (void *)hInst;
	dev->cur_num_instance++;

	return ret;

err_exit:
	if (ctx->instance_buf)
		nx_free_memory(ctx->instance_buf);
	if (ctx->bit_stream_buf)
		nx_free_memory(ctx->bit_stream_buf);
	return ret;
}

int vpu_dec_parse_vid_cfg(struct nx_vpu_ctx *ctx)
{
	int ret;
	struct vpu_dec_ctx *dec_ctx = &ctx->codec.dec;
	struct vpu_dec_seq_init_arg seqArg;
	struct nx_vpu_buf *buf;
	struct vb2_v4l2_buffer *vbuf;
	unsigned long flags;

	FUNC_IN();

	if (ctx->hInst == NULL) {
		NX_ErrMsg(("Err : vpu is not opend\n"));
		return -EAGAIN;
	}

	NX_DrvMemset(&seqArg, 0, sizeof(seqArg));

	if (ctx->strm_queue_cnt > 0) {
		unsigned long phyAddr;

		/*spin_lock_irqsave(&ctx->dev->irqlock, flags);*/

		if (list_empty(&ctx->strm_queue)) {
			NX_DbgMsg(INFO_MSG, ("No src buffer.\n"));
			/* spin_unlock_irqrestore(&ctx->dev->irqlock, flags); */
			return -EAGAIN;
		}

		buf = list_entry(ctx->strm_queue.next, struct nx_vpu_buf, list);
		buf->used = 1;
		phyAddr = nx_vpu_mem_plane_addr(ctx, &buf->vb, 0);
		seqArg.seqDataSize = buf->vb.planes[0].bytesused;

#ifdef USE_ION_MEMORY
		{
			int alignSz;

			alignSz = (seqArg.seqDataSize + 4095) & (~4095);
			seqArg.seqData = (unsigned char *)cma_get_virt(phyAddr,
				alignSz, 1);
		}
#else
		seqArg.seqData = (unsigned long)vb2_plane_vaddr(&buf->vb, 0);
#endif

		/*spin_unlock_irqrestore(&ctx->dev->irqlock, flags);*/
	} else {
		return -EAGAIN;
	}

	seqArg.outWidth = ctx->width;
	seqArg.outHeight = ctx->height;

	seqArg.thumbnailMode = dec_ctx->thumbnailMode;

	ret = NX_VpuDecSetSeqInfo(ctx->hInst, &seqArg);
	if (ret != VPU_RET_OK)
		NX_ErrMsg(("NX_VpuDecSetSeqInfo() failed.(ErrorCode=%d)\n",
			ret));

	if (seqArg.minFrameBufCnt < 1 ||
		seqArg.minFrameBufCnt > VPU_MAX_BUFFERS)
		NX_ErrMsg(("Min FrameBufCnt Error(%d)!!!\n",
			seqArg.minFrameBufCnt));

	ctx->width = seqArg.cropRight;
	ctx->height = seqArg.cropBottom;
	dec_ctx->frame_buffer_cnt = seqArg.minFrameBufCnt;

	dec_ctx->interlace_flg[0] = (seqArg.interlace == 0) ?
		(V4L2_FIELD_NONE) : (V4L2_FIELD_INTERLACED);
	dec_ctx->frame_buf_delay = seqArg.frameBufDelay;
	ctx->buf_y_width = ALIGN(ctx->width, 32);
	ctx->buf_height = ALIGN(ctx->height, 16);
	ctx->luma_size = ctx->buf_y_width * ctx->buf_height;

	switch (seqArg.imgFormat) {
	case IMG_FORMAT_420:
		ctx->imgFourCC = V4L2_PIX_FMT_YUV420M;
		ctx->buf_c_width = ctx->buf_y_width >> 1;
		ctx->chroma_size = ctx->buf_c_width *
			ALIGN(ctx->buf_height/2, 16);
		break;
	case IMG_FORMAT_422:
		ctx->imgFourCC = V4L2_PIX_FMT_YUV422M;
		ctx->buf_c_width = ctx->buf_y_width >> 1;
		ctx->chroma_size = ctx->buf_c_width * ctx->buf_height;
		break;
	/* case IMG_FORMAT_224:
		ctx->imgFourCC = ;
		ctx->buf_c_width = ctx->buf_y_width;
		ctx->chroma_size = ctx->buf_c_width *
			ALIGN(ctx->buf_height/2, 16);
		break; */
	case IMG_FORMAT_444:
		ctx->imgFourCC = V4L2_PIX_FMT_YUV444M;
		ctx->buf_c_width = ctx->buf_y_width;
		ctx->chroma_size = ctx->luma_size;
		break;
	case IMG_FORMAT_400:
		ctx->imgFourCC = V4L2_PIX_FMT_GREY;
		ctx->buf_c_width = 0;
		ctx->chroma_size = 0;
		break;
	default:
		NX_ErrMsg(("Image format is not supported!!\n"));
		return -EINVAL;
	}

	dec_ctx->start_Addr = 0;
	dec_ctx->end_Addr = seqArg.strmReadPos;
	ctx->strm_size = dec_ctx->end_Addr - dec_ctx->start_Addr;

	dec_ctx->crop_left = seqArg.cropLeft;
	dec_ctx->crop_right = seqArg.cropRight;
	dec_ctx->crop_top = seqArg.cropTop;
	dec_ctx->crop_bot = seqArg.cropBottom;

	NX_DbgMsg(INFO_MSG, ("[PARSE]Min_Buff = %d\n",
		dec_ctx->frame_buffer_cnt));

	spin_lock_irqsave(&ctx->dev->irqlock, flags);

	buf = list_entry(ctx->strm_queue.next, struct nx_vpu_buf, list);
	vbuf = to_vb2_v4l2_buffer(&buf->vb);
	list_del(&buf->list);
	ctx->strm_queue_cnt--;

	buf->vb.planes[0].bytesused = ctx->strm_size;
	vbuf->field = dec_ctx->interlace_flg[0];
	vbuf->flags = ctx->codec.dec.frame_buf_delay;

	vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);

	spin_unlock_irqrestore(&ctx->dev->irqlock, flags);

	return ret;
}

int vpu_dec_init(struct nx_vpu_ctx *ctx)
{
	struct vpu_dec_ctx *dec_ctx = &ctx->codec.dec;
	struct vpu_dec_reg_frame_arg *frameArg;
	int i, ret = 0;

	FUNC_IN();

	if (ctx->hInst == NULL) {
		NX_ErrMsg(("Err : vpu is not opend\n"));
		return -EAGAIN;
	}
	frameArg = kzalloc(sizeof(struct vpu_dec_reg_frame_arg),
			GFP_KERNEL);
	if (!frameArg)
		return -ENOMEM;

	frameArg->chromaInterleave = ctx->chromaInterleave;

	if (ctx->codec_mode != CODEC_STD_MJPG) {
		ret = alloc_decoder_memory(ctx);
		if (ret) {
			NX_ErrMsg(("Failed to allocate decoder buffers.\n"));
			return -ENOMEM;
		}

		if (dec_ctx->slice_buf)
			frameArg->sliceBuffer = *dec_ctx->slice_buf;
		if (dec_ctx->col_mv_buf)
			frameArg->colMvBuffer = *dec_ctx->col_mv_buf;
		if (dec_ctx->pv_slice_buf)
			frameArg->pvbSliceBuffer = *dec_ctx->pv_slice_buf;

		frameArg->sramAddr = ctx->dev->sram_base_addr;
		frameArg->sramSize = ctx->dev->sram_size;
	}

	for (i = 0; i < dec_ctx->dpb_queue_cnt; i++) {
		frameArg->frameBuffer[i].phyAddr[0]
			= dec_ctx->frame_buf[i].phyAddr[0];
		frameArg->frameBuffer[i].phyAddr[1]
			= dec_ctx->frame_buf[i].phyAddr[1];

		if (ctx->chromaInterleave == 0)
			frameArg->frameBuffer[i].phyAddr[2]
				= dec_ctx->frame_buf[i].phyAddr[2];

		frameArg->frameBuffer[i].stride[0] = ctx->buf_y_width;
	}
	frameArg->numFrameBuffer = dec_ctx->dpb_queue_cnt;

	ret = NX_VpuDecRegFrameBuf(ctx->hInst, frameArg);
	if (ret != VPU_RET_OK)
		NX_ErrMsg(("NX_VpuDecRegFrameBuf() failed.(ErrorCode=%d)\n",
			ret));

	return ret;
}

static void put_dec_info(struct nx_vpu_ctx *ctx,
	struct vpu_dec_frame_arg *pDecArg, struct timeval *pTimestamp)
{
	struct vpu_dec_ctx *dec_ctx = &ctx->codec.dec;
	int32_t idx = pDecArg->indexFrameDecoded;

	if (idx < 0)
		return;

	if ((pDecArg->isInterace) || ((ctx->codec_mode == CODEC_STD_MPEG2) &&
		(pDecArg->picStructure != 3)))
		dec_ctx->interlace_flg[idx] = (pDecArg->topFieldFirst) ?
			(V4L2_FIELD_SEQ_TB) : (V4L2_FIELD_SEQ_BT);
	else
		dec_ctx->interlace_flg[idx] = V4L2_FIELD_NONE;

	switch (pDecArg->picType) {
	case 0:
	case 6:
		dec_ctx->frm_type[idx] = V4L2_BUF_FLAG_KEYFRAME;
		break;
	case 1:
	case 4:
	case 5:
		dec_ctx->frm_type[idx] = V4L2_BUF_FLAG_PFRAME;
		break;
	case 2:
	case 3:
		dec_ctx->frm_type[idx] = V4L2_BUF_FLAG_BFRAME;
		break;
	case 7:
		dec_ctx->frm_type[idx] = -1;
		break;
	default:
		NX_ErrMsg(("not defined frame type!!!\n"));
		dec_ctx->frm_type[idx] = -1;
	}

	if (pDecArg->numOfErrMBs == 0) {
		dec_ctx->cur_reliable = 100;
	} else {
		if (ctx->codec_mode != CODEC_STD_MJPG) {
			int totalMbNum = ((pDecArg->outWidth + 15) >> 4) *
				((pDecArg->outHeight + 15) >> 4);
			dec_ctx->cur_reliable = (totalMbNum -
				pDecArg->numOfErrMBs) * 100 / totalMbNum;
		} else {
			int32_t PosX = ((pDecArg->numOfErrMBs >> 12) & 0xFFF) *
				pDecArg->mcuWidth;
			int32_t PosY = (pDecArg->numOfErrMBs & 0xFFF) *
				pDecArg->mcuHeight;
			int32_t PosRst = ((pDecArg->numOfErrMBs >> 24) & 0xF) *
				pDecArg->mcuWidth * pDecArg->mcuHeight;
			dec_ctx->cur_reliable = (PosRst + (PosY *
				pDecArg->outWidth) + PosX) * 100 /
				(pDecArg->outWidth * pDecArg->outHeight);
		}
	}

	if ((dec_ctx->interlace_flg[idx] == V4L2_FIELD_NONE) ||
		(pDecArg->npf))
		dec_ctx->reliable_0_100[idx] = dec_ctx->cur_reliable;
	else
		dec_ctx->reliable_0_100[idx] += dec_ctx->cur_reliable >> 1;

	dec_ctx->timeStamp[idx].tv_sec = pTimestamp->tv_sec;
	dec_ctx->timeStamp[idx].tv_usec = pTimestamp->tv_usec;
}

int vpu_dec_decode_slice(struct nx_vpu_ctx *ctx)
{
	struct vpu_dec_ctx *dec_ctx = &ctx->codec.dec;
	int ret = 0;
	unsigned long flags;
	struct timeval timestamp;
	struct vpu_dec_frame_arg decArg;
	struct nx_vpu_v4l2 *dev = ctx->dev;
	struct nx_vpu_buf *buf;
	struct vb2_v4l2_buffer *vbuf;

	FUNC_IN();

	if (ctx->hInst == NULL) {
		NX_ErrMsg(("Err : vpu is not opend\n"));
		return -EAGAIN;
	}

	NX_DrvMemset(&decArg, 0, sizeof(decArg));

	if (dec_ctx->flush == 0) {
		int alignSz;
		unsigned long phyAddr;

		/* spin_lock_irqsave(&dev->irqlock, flags); */

		if (list_empty(&ctx->strm_queue)) {
			NX_DbgMsg(INFO_MSG, ("No src buffer.\n"));
			/* spin_unlock_irqrestore(&ctx->dev->irqlock, flags); */
			return -EAGAIN;
		}

		buf = list_entry(ctx->strm_queue.next, struct nx_vpu_buf, list);
		vbuf = to_vb2_v4l2_buffer(&buf->vb);
		buf->used = 1;
		phyAddr = nx_vpu_mem_plane_addr(ctx, &buf->vb, 0);
		decArg.strmDataSize = buf->vb.planes[0].bytesused;
		alignSz = (decArg.strmDataSize + 4095) & (~4095);
#ifdef USE_ION_MEMORY
		decArg.strmData = (unsigned char *)cma_get_virt(phyAddr,
			alignSz, 1);
#else
		decArg.strmData = (unsigned long)vb2_plane_vaddr(&buf->vb, 0);
#endif

		decArg.eos = vbuf->flags;
		timestamp.tv_sec = vbuf->timestamp.tv_sec;
		timestamp.tv_usec = vbuf->timestamp.tv_usec;

		/* spin_unlock_irqrestore(&dev->irqlock, flags); */
	} else {
		decArg.strmDataSize = 0;
		decArg.strmData = 0;
		decArg.eos = dec_ctx->eos_tag;

		timestamp.tv_sec = -1;
		timestamp.tv_usec = -1;
	}

	dec_ctx->start_Addr = dec_ctx->end_Addr;

	ret = NX_VpuDecRunFrame(ctx->hInst, &decArg);
	if (ret != VPU_RET_OK) {
		NX_ErrMsg(("NX_VpuDecRunFrame() failed.(ErrorCode=%d)\n", ret));
		return ret;
	}

	dec_ctx->end_Addr = decArg.strmReadPos;

	if (dec_ctx->end_Addr >= dec_ctx->start_Addr)
		ctx->strm_size = dec_ctx->end_Addr - dec_ctx->start_Addr;
	else
		ctx->strm_size = (STREAM_BUF_SIZE - dec_ctx->start_Addr)
				+ dec_ctx->end_Addr;

	put_dec_info(ctx, &decArg, &timestamp);

	if (decArg.indexFrameDisplay >= 0) {
		dec_ctx->delay_frm = 0;

		spin_lock_irqsave(&dev->irqlock, flags);

		list_for_each_entry(buf, &dec_ctx->dpb_queue, list) {
			if (decArg.indexFrameDisplay
				== buf->vb.index) {
				vbuf = to_vb2_v4l2_buffer(&buf->vb);
				list_del(&buf->list);
				dec_ctx->dpb_queue_cnt--;

				vbuf->field = decArg.isInterace;
				vbuf->flags = decArg.picType;

				list_add_tail(&buf->list, &ctx->img_queue);
				ctx->img_queue_cnt++;

				break;
			}
		}

		spin_unlock_irqrestore(&dev->irqlock, flags);
	} else if (decArg.indexFrameDisplay == -3) {
		NX_DbgMsg(INFO_MSG, ("delayed Output(%d)\n",
			decArg.indexFrameDisplay));
		dec_ctx->delay_frm = 1;
	} else if (decArg.indexFrameDisplay == -2) {
		NX_DbgMsg(INFO_MSG, ("Skip Frame\n"));
		dec_ctx->delay_frm = -1;
	} else {
		NX_ErrMsg(("There is not decoded img!!!\n"));
		dec_ctx->delay_frm = -1;
	}

	spin_lock_irqsave(&dev->irqlock, flags);

	if (ctx->strm_queue_cnt > 0) {
		int idx = decArg.indexFrameDecoded;

		buf = list_entry(ctx->strm_queue.next, struct nx_vpu_buf, list);

		if (buf->used) {
			list_del(&buf->list);
			ctx->strm_queue_cnt--;
		}

		vbuf = to_vb2_v4l2_buffer(&buf->vb);
		buf->vb.index = idx;
		vbuf->field = dec_ctx->interlace_flg[idx];
		vbuf->flags = dec_ctx->frm_type[idx];
		buf->vb.planes[0].bytesused = ctx->strm_size;
		vbuf->timestamp.tv_sec =
			dec_ctx->timeStamp[idx].tv_sec;
		vbuf->timestamp.tv_usec =
			dec_ctx->timeStamp[idx].tv_usec;

		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);
	}

	if (ctx->img_queue_cnt > 0) {
		int idx = decArg.indexFrameDisplay;

		buf = list_entry(ctx->img_queue.next, struct nx_vpu_buf, list);

		list_del(&buf->list);
		ctx->img_queue_cnt--;

		vbuf = to_vb2_v4l2_buffer(&buf->vb);
		vbuf->field = dec_ctx->interlace_flg[idx];
		vbuf->flags = dec_ctx->frm_type[idx];
		vbuf->timestamp.tv_sec =
			dec_ctx->timeStamp[idx].tv_sec;
		vbuf->timestamp.tv_usec =
			dec_ctx->timeStamp[idx].tv_usec;

		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);
	}

	spin_unlock_irqrestore(&dev->irqlock, flags);

	return ret;
}

int alloc_decoder_memory(struct nx_vpu_ctx *ctx)
{
	struct vpu_dec_ctx *dec_ctx = &ctx->codec.dec;
	int width, height, mvSize;
	void *drv = &ctx->dev->plat_dev->dev;

	FUNC_IN();

	width = ALIGN(ctx->width, 16);
	height = ALIGN(ctx->height, 16);

	mvSize = ALIGN(ctx->width, 32) * ALIGN(ctx->height, 31);
	mvSize = (mvSize * 3) / 2;
	mvSize = (mvSize + 4) / 5;
	mvSize = ((mvSize + 7) / 8) * 8;
	mvSize = ALIGN(mvSize, 4096);

	if (width == 0 || height == 0 || mvSize == 0) {
		NX_ErrMsg(("Invalid memory parameters!!!\n"));
		NX_ErrMsg(("  width=%d, height=%d, mvSize=%d\n", width, height,
			mvSize));
		return -EINVAL;
	}

	dec_ctx->col_mv_buf = nx_alloc_memory(drv, mvSize *
		dec_ctx->frame_buffer_cnt, 4096);
	if (0 == dec_ctx->col_mv_buf) {
		NX_ErrMsg(("col_mv_buf allocation failed.(size=%d,align=%d)\n",
			mvSize * dec_ctx->frame_buffer_cnt, 4096));
		goto Error_Exit;
	}

	if (ctx->codec_mode == CODEC_STD_AVC) {
		dec_ctx->slice_buf = nx_alloc_memory(drv, 2048 * 2048 * 3 / 4,
			4096);
		if (0 == dec_ctx->slice_buf) {
			NX_ErrMsg(("slice buf allocation failed(size = %d)\n",
				2048 * 2048 * 3 / 4));
			goto Error_Exit;
		}
	}

	if (ctx->codec_mode == CODEC_STD_THO || ctx->codec_mode == CODEC_STD_VP3
		|| ctx->codec_mode == CODEC_STD_VP8) {
			dec_ctx->pv_slice_buf = nx_alloc_memory(drv, 17 * 4 *
				(2048 * 2048 / 256), 4096);
		if (0 == dec_ctx->pv_slice_buf) {
			NX_ErrMsg(("slice allocation failed(size=%d)\n",
				17 * 4 * (2048 * 2048 / 256)));
			goto Error_Exit;
		}
	}

	return 0;

Error_Exit:
	free_decoder_memory(ctx);
	return -1;
}

int free_decoder_memory(struct nx_vpu_ctx *ctx)
{
	struct vpu_dec_ctx *dec_ctx = &ctx->codec.dec;

	FUNC_IN();

	if (!ctx) {
		NX_ErrMsg(("invalid decoder handle!!!\n"));
		return -1;
	}

	if (dec_ctx->col_mv_buf)
		nx_free_memory(dec_ctx->col_mv_buf);

	if (dec_ctx->slice_buf)
		nx_free_memory(dec_ctx->slice_buf);

	if (dec_ctx->pv_slice_buf)
		nx_free_memory(dec_ctx->pv_slice_buf);

	if (ctx->instance_buf)
		nx_free_memory(ctx->instance_buf);

	if (ctx->bit_stream_buf)
		nx_free_memory(ctx->bit_stream_buf);

	kfree(dec_ctx->frameArg);

	return 0;
}
