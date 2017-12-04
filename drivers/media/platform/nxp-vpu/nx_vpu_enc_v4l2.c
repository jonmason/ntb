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


#define INFO_MSG				0
#define RECON_CHROMA_INTERLEAVE			0


static int nx_vpu_enc_ctx_ready(struct nx_vpu_ctx *ctx)
{
	FUNC_IN();
	NX_DbgMsg(INFO_MSG, ("src = %d, dst = %d\n", ctx->img_queue_cnt,
		ctx->strm_queue_cnt));

	if (ctx->vpu_cmd == ENC_RUN) {
		if (ctx->strm_queue_cnt < 1) {
			NX_DbgMsg(INFO_MSG, ("strm_queue_cnt error\n"));
			return 0;
		}
		if (ctx->is_initialized && ctx->img_queue_cnt < 1) {
			NX_DbgMsg(INFO_MSG, ("img_queue_cnt error\n"));
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
		.id = V4L2_CID_MPEG_VIDEO_FPS_NUM,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "frame per second",
		.minimum = 0,
		.maximum = (1 << 16) - 1,
		.step = 1,
		.default_value = 30,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_FPS_DEN,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "frame per second Density",
		.minimum = 1,
		.maximum = (1 << 16) - 1,
		.step = 1,
		.default_value = 1,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_GOP_SIZE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Size of key frame interval",
		.minimum = 0,
		.maximum = (1 << 16) - 1,
		.step = 1,
		.default_value = 30,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_CYCLIC_INTRA_REFRESH_MB,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Intra MB refresh number",
		.minimum = 0,
		.maximum = (1 << 16) - 1,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_SEARCH_RANGE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Search range of ME",
		.minimum = 0,
		.maximum = (1 << 16) - 1,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Enable rate control",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_BITRATE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Target bitrate",
		.minimum = 0,
		.maximum = 50 * 1024 * 1024,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_VBV_SIZE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "VBV size in byte",
		.minimum = 0,
		.maximum = 0x7FFFFFFF,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_RC_DELAY,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Reference decoder buffer delay",
		.minimum = 0,
		.maximum = 0x7FFF,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_RC_GAMMA_FACTOR,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "User gamma factor",
		.minimum = 0,
		.maximum = (1 << 16) - 1,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_FRAME_SKIP_MODE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Enable skip frame mode",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_PROFILE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "H264 profile",
		.minimum = V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE,
		.maximum = V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE,
		.step = 1,
		.default_value = V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "QP of I-frame in H.264",
		.minimum = 0,
		.maximum = 51,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "QP of P-frame in H.264",
		.minimum = 0,
		.maximum = 51,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_MAX_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Maximum QP for H.264",
		.minimum = 0,
		.maximum = 51,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_AUD_INSERT,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Insert AUD before NAL unit",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},
	/*{
		.id = V4L2_CID_MPEG_VIDEO_MPEG4_PROFILE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "MPEG4 profile",
		.minimum = V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE,
		.maximum = V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE,
		.step = 1,
		.default_value = V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE,
	},*/
	{
		.id = V4L2_CID_MPEG_VIDEO_MPEG4_I_FRAME_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "QP of I-frame in MPEG4",
		.minimum = 0,
		.maximum = 31,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_MPEG4_P_FRAME_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "QP of P-frame in MPEG4",
		.minimum = 0,
		.maximum = 31,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Maximum QP for MPEG4",
		.minimum = 0,
		.maximum = 31,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H263_PROFILE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "H263 Profile",
		.minimum = V4L2_MPEG_VIDEO_H263_PROFILE_P0,
		.maximum = V4L2_MPEG_VIDEO_H263_PROFILE_P3,
		.step = 1,
		.default_value = V4L2_MPEG_VIDEO_H263_PROFILE_P0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H263_I_FRAME_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "QP of I-frame in H.263",
		.minimum = 0,
		.maximum = 31,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H263_P_FRAME_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "QP of P-frame in H.263",
		.minimum = 0,
		.maximum = 31,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H263_MAX_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Maximum QP for H.263",
		.minimum = 0,
		.maximum = 31,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_JPEG_COMPRESSION_QUALITY,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Parameter for Jpeg Quality",
		.minimum = 0,
		.maximum = 100,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_FORCE_I_FRAME,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Flag of forced intra frame",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_FORCE_SKIP_FRAME,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Flag of forced skip frame",
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

static int check_format_interleaved(unsigned int fourcc)
{
	switch(fourcc) {
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV24:
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV16M:
	case V4L2_PIX_FMT_NV24M:
	/* Not support CbCr ordering in interleaved format.
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_NV42:
	case V4L2_PIX_FMT_NV21M:
	case V4L2_PIX_FMT_NV61M:
	case V4L2_PIX_FMT_NV42M: */
		return 0;
	}
	return -ERANGE;
}

static int check_plane_continuous(unsigned int fourcc)
{
	switch(fourcc) {
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_YVU420:
	case V4L2_PIX_FMT_YUV422P:
	case V4L2_PIX_FMT_YUV444:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV24:
	/* Not support CbCr ordering in interleaved format.
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_NV42: */
		return 0;
	}
	return -ERANGE;
}

static int check_cb_first(unsigned int fourcc)
{
	switch(fourcc) {
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_YUV420M:
	case V4L2_PIX_FMT_YUV422P:
	case V4L2_PIX_FMT_YUV422M:
	case V4L2_PIX_FMT_YUV444:
	case V4L2_PIX_FMT_YUV444M:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV24:
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV16M:
	case V4L2_PIX_FMT_NV24M:
		return 0;
	}
	return -ERANGE;
}

/* -------------------------------------------------------------------------- */


/*-----------------------------------------------------------------------------
 *      functions for vidioc_queryctrl
 *----------------------------------------------------------------------------*/

/* Query a ctrl */
static int vidioc_queryctrl(struct file *file, void *priv, struct v4l2_queryctrl
	*qc)
{
	struct v4l2_queryctrl *c;

	FUNC_IN();

	c = get_ctrl(qc->id);
	if (!c)
		return -EINVAL;
	*qc = *c;
	return 0;
}

static int vidioc_g_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;

	FUNC_IN();

	if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		/* This is run on output (encoder dest) */
		pix_fmt_mp->width = 0;
		pix_fmt_mp->height = 0;
		pix_fmt_mp->field = V4L2_FIELD_NONE;
		pix_fmt_mp->pixelformat = ctx->strm_fmt->fourcc;
		pix_fmt_mp->num_planes = ctx->strm_fmt->num_planes;

		pix_fmt_mp->plane_fmt[0].bytesperline = ctx->strm_buf_size;
		pix_fmt_mp->plane_fmt[0].sizeimage = ctx->strm_buf_size;
	} else if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		/* This is run on capture (encoder src) */
		pix_fmt_mp->width = ctx->width;
		pix_fmt_mp->height = ctx->height;
		pix_fmt_mp->field = V4L2_FIELD_NONE;
		pix_fmt_mp->pixelformat = ctx->img_fmt.fourcc;
		pix_fmt_mp->num_planes = ctx->img_fmt.num_planes;

		pix_fmt_mp->plane_fmt[0].bytesperline = ctx->buf_y_width;
		pix_fmt_mp->plane_fmt[0].sizeimage = ctx->luma_size;
		pix_fmt_mp->plane_fmt[1].bytesperline = ctx->buf_c_width;
		pix_fmt_mp->plane_fmt[1].sizeimage = ctx->chroma_size;
		pix_fmt_mp->plane_fmt[2].bytesperline = ctx->buf_c_width;
		pix_fmt_mp->plane_fmt[2].sizeimage = ctx->chroma_size;

	} else {
		NX_ErrMsg(("invalid buf type\n"));
		return -EINVAL;
	}

	return 0;
}

static int vidioc_try_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct nx_vpu_fmt *fmt;
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;

	FUNC_IN();

	if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		fmt = find_format(f);
		if (!fmt) {
			NX_ErrMsg(("failed to try output format(ES), %x\n",
				pix_fmt_mp->pixelformat));
			return -EINVAL;
		}

		if (pix_fmt_mp->plane_fmt[0].sizeimage == 0) {
			NX_ErrMsg(("must be set encoding output size\n"));
			return -EINVAL;
		}

		pix_fmt_mp->plane_fmt[0].bytesperline =
			pix_fmt_mp->plane_fmt[0].sizeimage;
	} else if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		fmt = find_format(f);
		if (!fmt) {
			NX_ErrMsg(("failed to try input format(IMG), %x\n",
				pix_fmt_mp->pixelformat));
			return -EINVAL;
		}

		if ((fmt->num_planes != pix_fmt_mp->num_planes) &&
			(1 != pix_fmt_mp->num_planes)) {
			NX_ErrMsg(("failed to try input format(%d, %d)\n",
				fmt->num_planes, pix_fmt_mp->num_planes));
			return -EINVAL;
		}
	} else {
		NX_ErrMsg(("invalid buf type\n"));
		return -EINVAL;
	}

	return 0;
}

static int vidioc_s_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(priv);
	struct nx_vpu_fmt *fmt;
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;
	int ret = 0;

	FUNC_IN();

	ret = vidioc_try_fmt(file, priv, f);
	if (ret)
		return ret;

	if (ctx->vq_img.streaming || ctx->vq_strm.streaming) {
		NX_ErrMsg(("%s queue busy\n", __func__));
		ret = -EBUSY;
		goto ERROR_EXIT;
	}

	if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		fmt = find_format(f);
		if (!fmt) {
			NX_ErrMsg(("failed to set capture format\n"));
			return -EINVAL;
		}

		ctx->vpu_cmd = GET_ENC_INSTANCE;

		ctx->strm_fmt = fmt;

		ctx->strm_buf_size = pix_fmt_mp->plane_fmt[0].sizeimage;
		pix_fmt_mp->plane_fmt[0].bytesperline = 0;
		ret = nx_vpu_try_run(ctx);
	} else if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		struct vpu_enc_ctx *enc_ctx = &ctx->codec.enc;

		fmt = find_format(f);
		if (!fmt) {
			NX_ErrMsg(("failed to set output format\n"));
			return -EINVAL;
		}

		ctx->img_fmt.name = fmt->name;
		ctx->img_fmt.fourcc = fmt->fourcc;
		ctx->img_fmt.num_planes = f->fmt.pix_mp.num_planes;

		ctx->width = pix_fmt_mp->width;
		ctx->height = pix_fmt_mp->height;
		enc_ctx->reconChromaInterleave = RECON_CHROMA_INTERLEAVE;

		ctx->luma_size = f->fmt.pix_mp.plane_fmt[0].sizeimage;
		ctx->buf_y_width = f->fmt.pix_mp.plane_fmt[0].bytesperline;
		ctx->buf_height = ctx->luma_size / ctx->buf_y_width;

		if (1 < ctx->img_fmt.num_planes) {
			ctx->chroma_size = f->fmt.pix_mp.plane_fmt[1].sizeimage;
			ctx->buf_c_width = f->fmt.pix_mp.plane_fmt[1].bytesperline;
		}

		ctx->chromaInterleave = !check_format_interleaved(fmt->fourcc) ? 1 : 0;

		/* Maybe this code is here instead of line565 */
		/* enc_ctx->reconChromaInterleave = RECON_CHROMA_INTERLEAVE; */
		enc_ctx->seq_para.srcWidth = ctx->width;
		enc_ctx->seq_para.srcHeight = ctx->height;
		enc_ctx->seq_para.chromaInterleave = ctx->chromaInterleave;
		enc_ctx->seq_para.refChromaInterleave = enc_ctx->reconChromaInterleave;

		pix_fmt_mp->plane_fmt[0].bytesperline = ctx->buf_y_width;
		pix_fmt_mp->plane_fmt[0].sizeimage = ctx->luma_size;

		if (1 < ctx->img_fmt.num_planes) {
			pix_fmt_mp->plane_fmt[1].bytesperline = ctx->buf_c_width;
			pix_fmt_mp->plane_fmt[1].sizeimage = ctx->chroma_size;
		}

		if (2 < ctx->img_fmt.num_planes) {
			pix_fmt_mp->plane_fmt[2].bytesperline = ctx->buf_c_width;
			pix_fmt_mp->plane_fmt[2].sizeimage = ctx->chroma_size;
		}
	} else {
		NX_ErrMsg(("invalid buf type\n"));
		return -EINVAL;
	}

ERROR_EXIT:
	NX_DbgMsg(INFO_MSG, ("%s End!!(ret = %d)\n", __func__, ret));
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

	/* For Output ES */
	if (reqbufs->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = vb2_reqbufs(&ctx->vq_strm, reqbufs);
		if (ret != 0) {
			NX_ErrMsg(("error in vb2_reqbufs() for Stream\n"));
			return ret;
		}

		ret = alloc_encoder_memory(ctx);
		if (ret) {
			NX_ErrMsg(("Failed to allocate encoding buffers.\n"));
			reqbufs->count = 0;
			ret = vb2_reqbufs(&ctx->vq_strm, reqbufs);
			return -ENOMEM;
		}
	} else if (reqbufs->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = vb2_reqbufs(&ctx->vq_img, reqbufs);/* 10,  9(new) */
		if (ret != 0) {
			NX_ErrMsg(("error in vb2_reqbufs() for YUV\n"));
			return ret;
		}
	} else {
		NX_ErrMsg(("invalid buf type\n"));
		return -EINVAL;
	}

	return ret;
}

/* Queue a buffer */
static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);

	FUNC_IN();

	if (ctx->hInst == NULL) {
		NX_ErrMsg(("%s : Invalid encoder handle!!!\n", __func__));
		return -EIO;
	}

	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if (ctx->is_initialized == 0) {
			NX_ErrMsg(("%s : Not initialized!!!\n",  __func__));
			return -EIO;
		}

		return vb2_qbuf(&ctx->vq_img, buf);
	} else if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		return vb2_qbuf(&ctx->vq_strm, buf);
	}

	return -EINVAL;
}

/* Dequeue a buffer */
static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);

	FUNC_IN();

	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return vb2_dqbuf(&ctx->vq_img, buf, file->f_flags & O_NONBLOCK);

	return vb2_dqbuf(&ctx->vq_strm, buf, file->f_flags & O_NONBLOCK);
}

static int get_ctrl_val(struct vpu_enc_ctx *enc_ctx, struct v4l2_control *ctrl)
{
	struct vpu_enc_seq_arg *seq_para = &enc_ctx->seq_para;

	FUNC_IN();

	switch (ctrl->id) {
	case V4L2_CID_MPEG_VIDEO_FPS_NUM:
		ctrl->value = seq_para->frameRateNum;
		break;
	case V4L2_CID_MPEG_VIDEO_FPS_DEN:
		ctrl->value = seq_para->frameRateDen;
		break;
	case V4L2_CID_MPEG_VIDEO_GOP_SIZE:
		ctrl->value = seq_para->gopSize;
		break;
	case V4L2_CID_MPEG_VIDEO_CYCLIC_INTRA_REFRESH_MB:
		ctrl->value = seq_para->intraRefreshMbs;
		break;
	case V4L2_CID_MPEG_VIDEO_SEARCH_RANGE:
		ctrl->value = seq_para->searchRange;
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE:
		ctrl->value = (seq_para->bitrate > 0) ? (1) : (0);
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE:
		ctrl->value = seq_para->bitrate;
		break;
	case V4L2_CID_MPEG_VIDEO_VBV_SIZE:
		ctrl->value = seq_para->vbvBufferSize;
		break;
	case V4L2_CID_MPEG_VIDEO_RC_DELAY:
		ctrl->value = seq_para->initialDelay;
		break;
	case V4L2_CID_MPEG_VIDEO_RC_GAMMA_FACTOR:
		ctrl->value = seq_para->gammaFactor;
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_SKIP_MODE:
		ctrl->value = !seq_para->disableSkip;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_PROFILE:
		ctrl->value = V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_MPEG4_I_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_H263_I_FRAME_QP:
		ctrl->value = enc_ctx->userIQP;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_MPEG4_P_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_H263_P_FRAME_QP:
		ctrl->value = enc_ctx->userPQP;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_H263_MAX_QP:
		ctrl->value = seq_para->maxQP;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_AUD_INSERT:
		ctrl->value = seq_para->enableAUDelimiter;
		break;
	case V4L2_CID_MPEG_VIDEO_MPEG4_PROFILE:
		ctrl->value = V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE;
		break;
	case V4L2_CID_MPEG_VIDEO_H263_PROFILE:
		ctrl->value = (seq_para->annexFlg) ?
			(V4L2_MPEG_VIDEO_H263_PROFILE_P3) :
			(V4L2_MPEG_VIDEO_H263_PROFILE_P0);
		break;
	case V4L2_CID_JPEG_COMPRESSION_QUALITY:
		ctrl->value = seq_para->quality;
		break;
	default:
		NX_ErrMsg(("Invalid control(ID = %x)\n", ctrl->id));
		return -EINVAL;
	}

	return 0;
}

static int vidioc_g_ctrl(struct file *file, void *priv,
			 struct v4l2_control *ctrl)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	int ret = 0;

	FUNC_IN();
	ret = get_ctrl_val(&ctx->codec.enc, ctrl);

	return ret;
}

static int set_enc_param(struct vpu_enc_ctx *enc_ctx, struct v4l2_control *ctrl)
{
	int ret = 0;
	struct vpu_enc_seq_arg *seq_para = &enc_ctx->seq_para;

	FUNC_IN();

	switch (ctrl->id) {
	case V4L2_CID_MPEG_VIDEO_FPS_NUM:
		seq_para->frameRateNum = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_FPS_DEN:
		seq_para->frameRateDen = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_GOP_SIZE:
		seq_para->gopSize = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_CYCLIC_INTRA_REFRESH_MB:
		seq_para->intraRefreshMbs = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_SEARCH_RANGE:
		seq_para->searchRange = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE:
		if (ctrl->value == 0)
			seq_para->bitrate = 0;
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE:
		seq_para->bitrate = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VBV_SIZE:
		seq_para->vbvBufferSize = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_RC_DELAY:
		seq_para->initialDelay = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_RC_GAMMA_FACTOR:
		seq_para->gammaFactor = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_SKIP_MODE:
		seq_para->disableSkip = !ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_MPEG4_I_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_H263_I_FRAME_QP:
		enc_ctx->userIQP = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_MPEG4_P_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_H263_P_FRAME_QP:
		enc_ctx->userPQP = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_H263_MAX_QP:
		seq_para->maxQP = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_AUD_INSERT:
		seq_para->enableAUDelimiter = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H263_PROFILE:
		if (ctrl->value == V4L2_MPEG_VIDEO_H263_PROFILE_P3)
			seq_para->annexFlg = 1;
		break;
	case V4L2_CID_JPEG_COMPRESSION_QUALITY:
		seq_para->quality = ctrl->value;
		break;
	default:
		NX_ErrMsg(("Invalid control(ID = %x)\n", ctrl->id));
		ret = -EINVAL;
	}

	return ret;
}

static int set_enc_run_param(struct vpu_enc_ctx *enc_ctx,
	struct v4l2_control *ctrl)
{
	int ret = 0;
	struct vpu_enc_run_frame_arg *run_info = &enc_ctx->run_info;
	struct vpu_enc_chg_para_arg *chg_para = &enc_ctx->chg_para;
	struct vpu_enc_seq_arg *seq_para = &enc_ctx->seq_para;

	FUNC_IN();

	switch (ctrl->id) {
	case V4L2_CID_MPEG_VIDEO_FORCE_I_FRAME:
		run_info->forceIPicture = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_FORCE_SKIP_FRAME:
		run_info->forceSkipPicture = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_MPEG4_I_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_MPEG4_P_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_H263_I_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_H263_P_FRAME_QP:
		if (ctrl->value > 0)
			run_info->quantParam = ctrl->value;
		else
			run_info->quantParam = enc_ctx->userPQP;
		break;
	case V4L2_CID_MPEG_VIDEO_GOP_SIZE:
		chg_para->chgFlg |= VPU_BIT_CHG_GOP;
		chg_para->gopSize = ctrl->value;
		seq_para->gopSize = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE:
		chg_para->chgFlg |= VPU_BIT_CHG_BITRATE;
		chg_para->bitrate = ctrl->value;
		seq_para->bitrate = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_FPS_NUM:
		chg_para->chgFlg |= VPU_BIT_CHG_FRAMERATE;
		chg_para->frameRateNum = ctrl->value;
		seq_para->frameRateNum = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_FPS_DEN:
		chg_para->chgFlg |= VPU_BIT_CHG_FRAMERATE;
		chg_para->frameRateDen = ctrl->value;
		seq_para->frameRateDen = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_CYCLIC_INTRA_REFRESH_MB:
		chg_para->chgFlg |= VPU_BIT_CHG_INTRARF;
		chg_para->intraRefreshMbs = ctrl->value;
		seq_para->intraRefreshMbs = ctrl->value;
		break;
	case V4L2_CID_JPEG_COMPRESSION_QUALITY:
		seq_para->quality = ctrl->value;
		break;
	default:
		NX_ErrMsg(("Invalid control(ID = %x)\n", ctrl->id));
		ret = -EINVAL;
	}

	return ret;
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

	return set_enc_run_param(&ctx->codec.enc, ctrl);
}

static int vidioc_g_ext_ctrls(struct file *file, void *priv,
			      struct v4l2_ext_controls *f)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	struct v4l2_ext_control *ext_ctrl;
	struct v4l2_control ctrl;
	int i;
	int ret = 0;

	FUNC_IN();

	if ((f->ctrl_class != V4L2_CTRL_CLASS_MPEG) &&
		(f->ctrl_class != V4L2_CID_JPEG_CLASS_BASE))
		return -EINVAL;

	for (i = 0; i < f->count; i++) {
		ext_ctrl = (f->controls + i);

		ctrl.id = ext_ctrl->id;

		ret = get_ctrl_val(&ctx->codec.enc, &ctrl);
		if (ret == 0) {
			ext_ctrl->value = ctrl.value;
		} else {
			f->error_idx = i;
			break;
		}

		NX_DbgMsg(INFO_MSG, ("[%d] id: 0x%08x, value: %d", i,
			ext_ctrl->id, ext_ctrl->value));
	}

	return ret;
}

static int vidioc_s_ext_ctrls(struct file *file, void *priv,
				struct v4l2_ext_controls *f)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	struct v4l2_ext_control *ext_ctrl;
	struct v4l2_control ctrl;
	int i;
	int ret = 0;

	FUNC_IN();

	if ((f->ctrl_class != V4L2_CTRL_CLASS_MPEG) &&
		(f->ctrl_class != V4L2_CID_JPEG_CLASS_BASE))
		return -EINVAL;

	for (i = 0; i < f->count; i++) {
		ext_ctrl = (f->controls + i);

		ctrl.id = ext_ctrl->id;
		ctrl.value = ext_ctrl->value;

		ret = check_ctrl_val(ctx, &ctrl);
		if (ret != 0) {
			f->error_idx = i;
			break;
		}

		ret = set_enc_param(&ctx->codec.enc, &ctrl);
		if (ret != 0) {
			f->error_idx = i;
			break;
		}

		NX_DbgMsg(INFO_MSG, ("[%d] id: 0x%08x, value: %d\n", i,
			ext_ctrl->id, ext_ctrl->value));
	}

	return ret;
}

static int vidioc_try_ext_ctrls(struct file *file, void *priv,
	struct v4l2_ext_controls *f)
{
	struct nx_vpu_ctx *ctx = fh_to_ctx(file->private_data);
	struct v4l2_ext_control *ext_ctrl;
	struct v4l2_control ctrl;
	int i;
	int ret = 0;

	FUNC_IN();

	if ((f->ctrl_class != V4L2_CTRL_CLASS_MPEG) &&
		(f->ctrl_class != V4L2_CID_JPEG_CLASS_BASE))
		return -EINVAL;

	for (i = 0; i < f->count; i++) {
		ext_ctrl = (f->controls + i);

		ctrl.id = ext_ctrl->id;
		ctrl.value = ext_ctrl->value;

		ret = check_ctrl_val(ctx, &ctrl);
		if (ret != 0) {
			f->error_idx = i;
			break;
		}

		NX_DbgMsg(INFO_MSG, ("[%d] id: 0x%08x, value: %d", i,
			ext_ctrl->id, ext_ctrl->value));
	}

	return ret;
}

static const struct v4l2_ioctl_ops nx_vpu_enc_ioctl_ops = {
	.vidioc_querycap = vidioc_querycap,
	.vidioc_enum_fmt_vid_cap = vidioc_enum_fmt_vid_cap,
	.vidioc_enum_fmt_vid_cap_mplane = vidioc_enum_fmt_vid_cap_mplane,
	.vidioc_enum_fmt_vid_out = vidioc_enum_fmt_vid_out,
	.vidioc_enum_fmt_vid_out_mplane = vidioc_enum_fmt_vid_out_mplane,
	.vidioc_g_fmt_vid_cap_mplane = vidioc_g_fmt,
	.vidioc_g_fmt_vid_out_mplane = vidioc_g_fmt,
	.vidioc_try_fmt_vid_cap_mplane = vidioc_try_fmt,
	.vidioc_try_fmt_vid_out_mplane = vidioc_try_fmt,
	.vidioc_s_fmt_vid_cap_mplane = vidioc_s_fmt,
	.vidioc_s_fmt_vid_out_mplane = vidioc_s_fmt,
	.vidioc_reqbufs = vidioc_reqbufs,
	.vidioc_querybuf = vidioc_querybuf,
	.vidioc_qbuf = vidioc_qbuf,
	.vidioc_dqbuf = vidioc_dqbuf,
	.vidioc_streamon = vidioc_streamon,
	.vidioc_streamoff = vidioc_streamoff,
	.vidioc_queryctrl = vidioc_queryctrl,
	.vidioc_g_ctrl = vidioc_g_ctrl,
	.vidioc_s_ctrl = vidioc_s_ctrl,
	.vidioc_g_ext_ctrls = vidioc_g_ext_ctrls,
	.vidioc_s_ext_ctrls = vidioc_s_ext_ctrls,
	.vidioc_try_ext_ctrls = vidioc_try_ext_ctrls,
};

static int nx_vpu_enc_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct nx_vpu_ctx *ctx = q->drv_priv;
	int ret = 0;

	FUNC_IN();

	if (nx_vpu_enc_ctx_ready(ctx))
		ret = nx_vpu_try_run(ctx);

	return ret;
}

static void nx_vpu_enc_stop_streaming(struct vb2_queue *q)
{
	unsigned long flags;
	struct nx_vpu_ctx *ctx = q->drv_priv;
	struct nx_vpu_v4l2 *dev = ctx->dev;

	FUNC_IN();

	spin_lock_irqsave(&dev->irqlock, flags);

	if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		nx_vpu_cleanup_queue(&ctx->strm_queue, &ctx->vq_strm);
		INIT_LIST_HEAD(&ctx->strm_queue);
		ctx->strm_queue_cnt = 0;
	} else if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		nx_vpu_cleanup_queue(&ctx->img_queue, &ctx->vq_img);
		INIT_LIST_HEAD(&ctx->img_queue);
		ctx->img_queue_cnt = 0;
	}

	spin_unlock_irqrestore(&dev->irqlock, flags);
}

static void nx_vpu_enc_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct nx_vpu_ctx *ctx = vq->drv_priv;
	struct nx_vpu_v4l2 *dev = ctx->dev;
	unsigned long flags;
	struct nx_vpu_buf *buf = vb_to_vpu_buf(vb);

	FUNC_IN();

	spin_lock_irqsave(&dev->irqlock, flags);

	if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		buf->used = 0;

		NX_DbgMsg(INFO_MSG, ("adding to dst: %p (%08lx, %08lx)\n", vb,
			(unsigned long)nx_vpu_mem_plane_addr(ctx, vb, 0),
			(unsigned long)buf->planes.stream));

		list_add_tail(&buf->list, &ctx->strm_queue);
		ctx->strm_queue_cnt++;
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		buf->used = 0;
		NX_DbgMsg(INFO_MSG, ("adding to src: %p(%08lx, %08lx)\n",
			vb, (unsigned long)nx_vpu_mem_plane_addr(ctx, vb, 0),
			(unsigned long)nx_vpu_mem_plane_addr(ctx, vb, 1)));

		list_add_tail(&buf->list, &ctx->img_queue);
		ctx->img_queue_cnt++;
	} else {
		NX_ErrMsg(("unsupported buffer type (%d)\n", vq->type));
	}

	spin_unlock_irqrestore(&dev->irqlock, flags);

	if (nx_vpu_enc_ctx_ready(ctx))
		nx_vpu_try_run(ctx);
}

static struct vb2_ops nx_vpu_enc_qops = {
	.queue_setup            = nx_vpu_queue_setup,
	.wait_prepare           = nx_vpu_unlock,
	.wait_finish            = nx_vpu_lock,
	.buf_init               = nx_vpu_buf_init,
	.buf_prepare            = nx_vpu_buf_prepare,
	.start_streaming        = nx_vpu_enc_start_streaming,
	.stop_streaming         = nx_vpu_enc_stop_streaming,
	.buf_queue              = nx_vpu_enc_buf_queue,
};

/* -------------------------------------------------------------------------- */


const struct v4l2_ioctl_ops *get_enc_ioctl_ops(void)
{
	return &nx_vpu_enc_ioctl_ops;
};

int nx_vpu_enc_open(struct nx_vpu_ctx *ctx)
{
	int ret = 0;

	FUNC_IN();

	/* Init videobuf2 queue for INPUT */
	ctx->vq_img.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	ctx->vq_img.drv_priv = ctx;
	ctx->vq_img.lock = &ctx->dev->dev_mutex;
	ctx->vq_img.buf_struct_size = sizeof(struct nx_vpu_buf);
	ctx->vq_img.io_modes = VB2_USERPTR | VB2_DMABUF;
	/* ctx->vq_strm.io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF; */
	ctx->vq_img.ops = &nx_vpu_enc_qops;
	ctx->vq_img.mem_ops = &vb2_dma_contig_memops;
	/*ctx->vq_img.allow_zero_byteused = 1; */
	ctx->vq_img.timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	ret = vb2_queue_init(&ctx->vq_img);
	if (ret) {
		NX_ErrMsg(("Failed to initialize videobuf2 queue(output)\n"));
		return ret;
	}

	/* Init videobuf2 queue for OUTPUT */
	ctx->vq_strm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	ctx->vq_strm.drv_priv = ctx;
	ctx->vq_strm.lock = &ctx->dev->dev_mutex;
	ctx->vq_strm.buf_struct_size = sizeof(struct nx_vpu_buf);
	ctx->vq_strm.io_modes = VB2_USERPTR | VB2_DMABUF;
	/* ctx->vq_strm.io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF; */
	ctx->vq_strm.ops = &nx_vpu_enc_qops;
	ctx->vq_strm.mem_ops = &vb2_dma_contig_memops;
	ctx->vq_strm.timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	ret = vb2_queue_init(&ctx->vq_strm);
	if (ret) {
		NX_ErrMsg(("Failed to initialize videobuf2 queue(capture)\n"));
		return ret;
	}

	return 0;
}

int vpu_enc_open_instance(struct nx_vpu_ctx *ctx)
{
	struct nx_vpu_v4l2 *dev = ctx->dev;
	struct nx_vpu_codec_inst *hInst = 0;
	struct vpu_open_arg openArg;
	int ret;

	FUNC_IN();

	memset(&openArg, 0, sizeof(openArg));

	switch (ctx->strm_fmt->fourcc) {
	case V4L2_PIX_FMT_MPEG4:
		ctx->codec_mode = CODEC_STD_MPEG4;
		break;
	case V4L2_PIX_FMT_H264:
		ctx->codec_mode = CODEC_STD_AVC;
		break;
	case V4L2_PIX_FMT_H263:
		ctx->codec_mode = CODEC_STD_H263;
		break;
	case V4L2_PIX_FMT_MJPEG:
		ctx->codec_mode = CODEC_STD_MJPG;
		break;
	default:
		NX_ErrMsg(("Invalid codec type (%x)!!!\n",
			ctx->strm_fmt->fourcc));
		goto err_exit;
	}
	openArg.codecStd = ctx->codec_mode;

	/* Allocate Instance Memory & Stream Buffer */
	ctx->instance_buf = nx_alloc_memory(&dev->plat_dev->dev, WORK_BUF_SIZE,
		4096);
	if (ctx->instance_buf == NULL) {
		NX_ErrMsg(("hinstance_buf allocation failed.\n"));
		goto err_exit;
	}

	openArg.instanceBuf = *ctx->instance_buf;
	openArg.instIndex = ctx->idx;
	openArg.isEncoder = 1;

	ret = NX_VpuEncOpen(&openArg, dev, &hInst);
	if ((VPU_RET_OK != ret) || (0 == hInst)) {
		NX_ErrMsg(("Cannot open VPU Instance!!!\n"));
		NX_ErrMsg(("  codecStd=%d, is_encoder=%d, hInst=%p)\n",
			openArg.codecStd, openArg.isEncoder, hInst));
		ret = -1;
		goto err_exit;
	}

	ctx->hInst = (void *)hInst;
	dev->cur_num_instance++;

	return ret;

err_exit:
	if (ctx->instance_buf)
		nx_free_memory(ctx->instance_buf);

	return ret;
}

static void get_stream_buffer(struct nx_vpu_ctx *ctx,
	struct nx_memory_info *stream_buf)
{
	struct nx_vpu_buf *dst_mb;

	FUNC_IN();

	/* spin_lock_irqsave(&ctx->dev->irqlock, flags); */

	dst_mb = list_entry(ctx->strm_queue.next, struct nx_vpu_buf, list);
	dst_mb->used = 1;

	stream_buf->phyAddr = nx_vpu_mem_plane_addr(ctx, &dst_mb->vb, 0);
	stream_buf->size = vb2_plane_size(&dst_mb->vb, 0);
#ifdef USE_ION_MEMORY
	stream_buf->virAddr = (unsigned int)cma_get_virt(stream_buf->phyAddr,
		stream_buf->size, 1);
#else
	stream_buf->virAddr = vb2_plane_vaddr(&dst_mb->vb, 0);
#endif

	/* spin_unlock_irqrestore(&ctx->dev->irqlock, flags); */
}

int vpu_enc_init(struct nx_vpu_ctx *ctx)
{
	struct vpu_enc_ctx *enc_ctx = &ctx->codec.enc;
	struct vpu_enc_seq_arg *pSeqArg = &ctx->codec.enc.seq_para;
	int ret = 0;

	FUNC_IN();

	if (ctx->hInst == NULL) {
		NX_ErrMsg(("Err : vpu is not opened\n"));
		return -EAGAIN;
	}

	pSeqArg->strmBufPhyAddr = (uint64_t)ctx->bit_stream_buf->phyAddr;
	pSeqArg->strmBufVirAddr = (unsigned long)ctx->bit_stream_buf->virAddr;
	pSeqArg->strmBufSize = ctx->bit_stream_buf->size;

	if ((pSeqArg->strmBufPhyAddr == 0) || (pSeqArg->strmBufSize == 0)) {
		NX_ErrMsg(("stream buffer error(addr = %llx, size = %d)\n",
			pSeqArg->strmBufPhyAddr, pSeqArg->strmBufSize));
		return -1;
	}

	if ((pSeqArg->srcWidth == 0) || (pSeqArg->srcWidth > 1920) ||
		(pSeqArg->srcHeight == 0) || (pSeqArg->srcHeight > 1920)) {
		NX_ErrMsg(("resolution pamameter error(W = %d, H = %d)\n",
			pSeqArg->srcWidth, pSeqArg->srcHeight));
		return -1;
	}

	if (ctx->codec_mode != CODEC_STD_MJPG) {
		if (pSeqArg->bitrate) {
			if (pSeqArg->gammaFactor == 0)
				pSeqArg->gammaFactor = (int)(0.75 * 32768);
		} else {
			if (enc_ctx->userIQP == 0)
				enc_ctx->userIQP = 26;
			if (enc_ctx->userPQP == 0)
				enc_ctx->userPQP = 26;
		}

		if (ctx->codec_mode == CODEC_STD_AVC) {
			if ((pSeqArg->maxQP > 51) || (pSeqArg->maxQP == 0))
				pSeqArg->maxQP = 51;
		} else {
			if ((pSeqArg->maxQP > 31) || (pSeqArg->maxQP == 0))
				pSeqArg->maxQP = 31;

			if (ctx->codec_mode == CODEC_STD_H263)
				pSeqArg->searchRange = 3;
		}
	} else {
		pSeqArg->frameRateNum = 1;
		pSeqArg->frameRateDen = 1;
		pSeqArg->gopSize = 1;

		switch (ctx->img_fmt.fourcc) {
		case V4L2_PIX_FMT_YUV420:
		case V4L2_PIX_FMT_YVU420:
		case V4L2_PIX_FMT_YUV420M:
		case V4L2_PIX_FMT_YVU420M:
			pSeqArg->imgFormat = IMG_FORMAT_420;
			pSeqArg->chromaInterleave = 0;
			break;
		case V4L2_PIX_FMT_YUV422P:
		case V4L2_PIX_FMT_YUV422M:
			pSeqArg->imgFormat = IMG_FORMAT_422;
			pSeqArg->chromaInterleave = 0;
			break;
		case V4L2_PIX_FMT_YUV444:
		case V4L2_PIX_FMT_YUV444M:
			pSeqArg->imgFormat = IMG_FORMAT_444;
			pSeqArg->chromaInterleave = 0;
			break;
		case V4L2_PIX_FMT_GREY:
			pSeqArg->imgFormat = IMG_FORMAT_400;
			pSeqArg->chromaInterleave = 0;
			break;
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV12M:
			pSeqArg->imgFormat = IMG_FORMAT_420;
			pSeqArg->chromaInterleave = 1;
			break;
		case V4L2_PIX_FMT_NV16:
		case V4L2_PIX_FMT_NV16M:
			pSeqArg->imgFormat = IMG_FORMAT_422;
			pSeqArg->chromaInterleave = 1;
			break;
		case V4L2_PIX_FMT_NV24:
		case V4L2_PIX_FMT_NV24M:
			pSeqArg->imgFormat = IMG_FORMAT_444;
			pSeqArg->chromaInterleave = 1;
			break;
		default:
			NX_ErrMsg(("Color format is not supported!!"));
			return -EINVAL;
		}
	}

	ret = NX_VpuEncSetSeqParam(ctx->hInst, pSeqArg);
	if (ret != VPU_RET_OK) {
		NX_ErrMsg(("NX_VpuEncSetSeqParam() failed.(ErrorCode=%d)\n",
			ret));
		return ret;
	}

	if (ctx->codec_mode != CODEC_STD_MJPG) {
		struct vpu_enc_set_frame_arg  frameArg;
		union vpu_enc_get_header_arg *pHdrArg
			= &ctx->codec.enc.seq_info;

		/* We use always 2 frame */
		frameArg.numFrameBuffer = 2;

		frameArg.frameBuffer[0] = *enc_ctx->ref_recon_buf[0];
		frameArg.frameBuffer[1] = *enc_ctx->ref_recon_buf[1];
		frameArg.subSampleBuffer[0] = *enc_ctx->sub_sample_buf[0];
		frameArg.subSampleBuffer[1] = *enc_ctx->sub_sample_buf[1];

		/* data partition mode always disabled (for MPEG4) */
		frameArg.dataPartitionBuffer.phyAddr = 0;
		frameArg.dataPartitionBuffer.virAddr = 0;

		frameArg.sramAddr = ctx->dev->sram_base_addr;
		frameArg.sramSize = ctx->dev->sram_size;

		ret = NX_VpuEncSetFrame(ctx->hInst, &frameArg);
		if (ret != VPU_RET_OK) {
			NX_ErrMsg(("NX_VpuEncSetFrame() is failed!(ret = %d\n",
				ret));
			goto ERROR_EXIT;
		}

		ret = NX_VpuEncGetHeader(ctx->hInst, pHdrArg);
		if (ret != VPU_RET_OK) {
			NX_ErrMsg(("NX_VpuEncGetHeader() is failed!(ret = %d\n",
				ret));
			goto ERROR_EXIT;
		}
	}

	enc_ctx->gop_frm_cnt = 0;

	ctx->is_initialized = 1;

ERROR_EXIT:
	return ret;
}

void vpu_enc_get_seq_info(struct nx_vpu_ctx *ctx)
{
	struct nx_memory_info stream_buf;
	union vpu_enc_get_header_arg *pHdr = &ctx->codec.enc.seq_info;
	void *pvDst;

	FUNC_IN();

	if (ctx->codec_mode == CODEC_STD_AVC) {
		get_stream_buffer(ctx, &stream_buf);
		pvDst = stream_buf.virAddr;

		NX_DrvMemcpy(pvDst, (void *)pHdr->avcHeader.spsData,
			pHdr->avcHeader.spsSize);
		pvDst += pHdr->avcHeader.spsSize;
		NX_DrvMemcpy(pvDst, (void *)pHdr->avcHeader.ppsData,
			pHdr->avcHeader.ppsSize);
		ctx->strm_size = pHdr->avcHeader.spsSize +
			pHdr->avcHeader.ppsSize;
	} else if (ctx->codec_mode == CODEC_STD_MPEG4) {
		get_stream_buffer(ctx, &stream_buf);
		pvDst = stream_buf.virAddr;

		NX_DrvMemcpy((void *)stream_buf.virAddr,
			pHdr->mp4Header.vosData, pHdr->mp4Header.vosSize);
		NX_DrvMemcpy((void *)stream_buf.virAddr +
			pHdr->mp4Header.vosSize, pHdr->mp4Header.volData,
			pHdr->mp4Header.volSize);
		ctx->strm_size = pHdr->mp4Header.vosSize +
			pHdr->mp4Header.volSize;
	} else {
		ctx->strm_size = 0;
	}

	{
		struct nx_vpu_buf *dst_mb;
		unsigned long flags;

		spin_lock_irqsave(&ctx->dev->irqlock, flags);

		dst_mb = list_entry(ctx->strm_queue.next, struct nx_vpu_buf,
			list);
		list_del(&dst_mb->list);
		ctx->strm_queue_cnt--;

		vb2_set_plane_payload(&dst_mb->vb, 0, ctx->strm_size);
		vb2_buffer_done(&dst_mb->vb, VB2_BUF_STATE_DONE);

		spin_unlock_irqrestore(&ctx->dev->irqlock, flags);
	}
}

int vpu_enc_encode_frame(struct nx_vpu_ctx *ctx)
{
	struct vpu_enc_ctx *enc_ctx = &ctx->codec.enc;
	struct nx_vpu_v4l2 *dev = ctx->dev;
	struct nx_vpu_codec_inst *hInst = ctx->hInst;
	int ret = 0;
	unsigned long flags;
	struct nx_vpu_buf *mb_entry;
	struct vpu_enc_run_frame_arg *pRunArg = &enc_ctx->run_info;
	struct nx_memory_info stream_buf;

	FUNC_IN();

	if (ctx->hInst == NULL) {
		NX_ErrMsg(("Err : vpu is not opened\n"));
		return -EAGAIN;
	}

	if (enc_ctx->chg_para.chgFlg) {
		ret = NX_VpuEncChgParam(hInst, &enc_ctx->chg_para);
		if (ret != VPU_RET_OK) {
			NX_ErrMsg(("NX_VpuEncChgParam() failed.(Err=%d)\n",
				ret));
			return ret;
		}

		NX_DrvMemset(&enc_ctx->chg_para, 0, sizeof(enc_ctx->chg_para));
	}

	get_stream_buffer(ctx, &stream_buf);

	spin_lock_irqsave(&dev->irqlock, flags);

	mb_entry = list_entry(ctx->img_queue.next, struct nx_vpu_buf, list);
	mb_entry->used = 1;

	pRunArg->inImgBuffer.phyAddr[0] = nx_vpu_mem_plane_addr(ctx,
		&mb_entry->vb, 0);
	pRunArg->inImgBuffer.stride[0] = ctx->buf_y_width;

	if (1 < ctx->img_fmt.num_planes) {
		if (!check_plane_continuous(ctx->img_fmt.fourcc)) {
			pRunArg->inImgBuffer.phyAddr[1] =
			!check_cb_first(ctx->img_fmt.fourcc) ?
			pRunArg->inImgBuffer.phyAddr[0] + ctx->luma_size :
			pRunArg->inImgBuffer.phyAddr[0] + ctx->luma_size + ctx->chroma_size;
		}
		else {
			pRunArg->inImgBuffer.phyAddr[1] =
			!check_cb_first(ctx->img_fmt.fourcc) ?
			nx_vpu_mem_plane_addr(ctx, &mb_entry->vb, 1) :
			nx_vpu_mem_plane_addr(ctx, &mb_entry->vb, 2) ;
		}
		pRunArg->inImgBuffer.stride[1] = ctx->buf_c_width;
	}

	if (2 < ctx->img_fmt.num_planes) {
		if (!check_plane_continuous(ctx->img_fmt.fourcc)) {
			pRunArg->inImgBuffer.phyAddr[2] =
			!check_cb_first(ctx->img_fmt.fourcc) ?
			pRunArg->inImgBuffer.phyAddr[0] + ctx->luma_size + ctx->chroma_size:
			pRunArg->inImgBuffer.phyAddr[0] + ctx->luma_size ;
		}
		else {
			pRunArg->inImgBuffer.phyAddr[2] =
			!check_cb_first(ctx->img_fmt.fourcc) ?
			nx_vpu_mem_plane_addr(ctx, &mb_entry->vb, 2) :
			nx_vpu_mem_plane_addr(ctx, &mb_entry->vb, 1) ;
		}
		pRunArg->inImgBuffer.stride[2] = ctx->buf_c_width;
	}

	spin_unlock_irqrestore(&dev->irqlock, flags);

	dev->curr_ctx = ctx->idx;

	if (ctx->codec_mode != V4L2_PIX_FMT_MJPEG) {
		if ((enc_ctx->gop_frm_cnt >= enc_ctx->seq_para.gopSize) ||
			(enc_ctx->gop_frm_cnt == 0))
			pRunArg->forceIPicture = 1;

		enc_ctx->gop_frm_cnt = (pRunArg->forceIPicture) ?
			(1) : (enc_ctx->gop_frm_cnt + 1);
	}

	ret = NX_VpuEncRunFrame(hInst, pRunArg);
	if (ret != VPU_RET_OK) {
		NX_ErrMsg(("NX_VpuEncRunFrame() failed.(ErrorCode=%d)\n", ret));
		return ret;
	}

	memcpy(stream_buf.virAddr,
		(void *)(unsigned long)pRunArg->outStreamAddr,
		pRunArg->outStreamSize);
	ctx->strm_size = pRunArg->outStreamSize;

	spin_lock_irqsave(&dev->irqlock, flags);

	if (ctx->img_queue_cnt > 0) {
		mb_entry = list_entry(ctx->img_queue.next, struct nx_vpu_buf,
			list);
		list_del(&mb_entry->list);
		ctx->img_queue_cnt--;

		vb2_buffer_done(&mb_entry->vb, VB2_BUF_STATE_DONE);
	}

	if (ctx->strm_size > 0) {
		struct vb2_v4l2_buffer *vbuf;
		mb_entry = list_entry(ctx->strm_queue.next, struct nx_vpu_buf,
			list);

		list_del(&mb_entry->list);
		ctx->strm_queue_cnt--;
		vbuf = to_vb2_v4l2_buffer(&mb_entry->vb);

		switch (pRunArg->frameType) {
			case 0:
			case 6:
				vbuf->flags = V4L2_BUF_FLAG_KEYFRAME;
				break;
			case 1:
				vbuf->flags = V4L2_BUF_FLAG_PFRAME;
				break;
			case 2:
			case 3:
				vbuf->flags = V4L2_BUF_FLAG_BFRAME;
				break;
			default:
				vbuf->flags = V4L2_BUF_FLAG_PFRAME;
				NX_ErrMsg(("not defined frame type!!!\n"));
				break;
		}
		vb2_set_plane_payload(&mb_entry->vb, 0, ctx->strm_size);

		vb2_buffer_done(&mb_entry->vb, VB2_BUF_STATE_DONE);
	}

	spin_unlock_irqrestore(&dev->irqlock, flags);

	return ret;
}

int alloc_encoder_memory(struct nx_vpu_ctx *ctx)
{
	int width, height, i;
	struct vpu_enc_ctx *enc_ctx = &ctx->codec.enc;
	void *drv = &ctx->dev->plat_dev->dev;

	FUNC_IN();

	width = ALIGN(ctx->width, 16);
	height = ALIGN(ctx->height, 16);

	if (ctx->codec_mode != CODEC_STD_MJPG) {
		int num;
		uint32_t format;

		if (enc_ctx->reconChromaInterleave == 0) {
			num = 3;
			format = V4L2_PIX_FMT_YUV420M;
		} else {
			num = 2;
			format = V4L2_PIX_FMT_NV12M;
		}

		for (i = 0 ; i < 2 ; i++) {
			enc_ctx->ref_recon_buf[i] = nx_alloc_frame_memory(drv,
				width, height, num, format, 64);

			if (enc_ctx->ref_recon_buf[i] == 0) {
				NX_ErrMsg(("alloc(%d,%d,..) failed(recon%d)\n",
					width, height, i));
				goto Error_Exit;
			}

			enc_ctx->sub_sample_buf[i] = nx_alloc_memory(drv,
				width * height/4, 4096);
			if (enc_ctx->sub_sample_buf[i] == 0) {
				NX_ErrMsg(("sub_buf allocation failed\n"));
				NX_ErrMsg(("  size = %d, align = %d)\n",
					width * height, 16));
				goto Error_Exit;
			}
		}
	}

	ctx->bit_stream_buf = nx_alloc_memory(drv, STREAM_BUF_SIZE, 4096);
	if (0 == ctx->bit_stream_buf) {
		NX_ErrMsg(("bit_stream_buf allocation failed.\n"));
		NX_ErrMsg(("  size = %d, align = %d)\n",
			STREAM_BUF_SIZE, 4096));
		goto Error_Exit;
	}

	return 0;

Error_Exit:
	free_encoder_memory(ctx);
	return -1;
}

int free_encoder_memory(struct nx_vpu_ctx *ctx)
{
	struct vpu_enc_ctx *enc_ctx = &ctx->codec.enc;

	FUNC_IN();

	if (!ctx) {
		NX_ErrMsg(("invalid encoder handle!!!\n"));
		return -1;
	}

	/* Free Reconstruct Buffer & Reference Buffer */
	if (enc_ctx->ref_recon_buf[0])
		nx_free_frame_memory(enc_ctx->ref_recon_buf[0]);

	if (enc_ctx->ref_recon_buf[1])
		nx_free_frame_memory(enc_ctx->ref_recon_buf[1]);

	/* Free SubSampleb Buffer */
	if (enc_ctx->sub_sample_buf[0])
		nx_free_memory(enc_ctx->sub_sample_buf[0]);

	if (enc_ctx->sub_sample_buf[1])
		nx_free_memory(enc_ctx->sub_sample_buf[1]);

	/* Free Bitstream Buffer */
	if (ctx->bit_stream_buf)
		nx_free_memory(ctx->bit_stream_buf);

	if (ctx->instance_buf)
		nx_free_memory(ctx->instance_buf);

	return 0;
}
