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

#ifndef _nx_vpu_v4l2_H
#define _nx_vpu_v4l2_H

#include <linux/platform_device.h>
#include <linux/irqreturn.h>
#include <media/media-device.h>
#include <media/v4l2-device.h>

#include "nx_port_func.h"
#include "nx_vpu_config.h"
#include "nx_vpu_api.h"


#define VPU_MAX_BUFFERS                 32
#define STREAM_BUF_SIZE                 (4*1024*1024)
#define ENABLE_INTERRUPT_MODE

#define fh_to_ctx(__fh) container_of(__fh, struct nx_vpu_ctx, fh)
#define vb_to_vpu_buf(x) container_of(x, struct nx_vpu_buf, vb)


struct nx_vpu_v4l2 {
	struct v4l2_device v4l2_dev;
	struct video_device *vfd_dec;
	struct video_device *vfd_enc;
	struct platform_device *plat_dev;

	void *regs_base;
	struct reset_control *coda_c;
	struct reset_control *coda_a;
	struct reset_control *coda_p;

	uint32_t sram_base_addr;
	uint32_t sram_size;

	spinlock_t irqlock;	/* lock when operating on videobuf2 queues */
	struct mutex dev_mutex;

	wait_queue_head_t vpu_wait_queue;
	wait_queue_head_t jpu_wait_queue;

	atomic_t vpu_event_present;
	atomic_t jpu_event_present;

	uint32_t jpu_intr_reason;

	struct nx_vpu_ctx *ctx[NX_MAX_VPU_INSTANCE];
	int curr_ctx;
	unsigned long ctx_work_bits;

	void *alloc_ctx;

	/* instance management */
	int cur_num_instance;
	int cur_jpg_instance;

	struct nx_memory_info *firmware_buf;
};

struct vpu_enc_ctx {
	int gop_frm_cnt;		/* gop frame counter */

	int userIQP;
	int userPQP;

	struct nx_vid_memory_info *ref_recon_buf[2];
	struct nx_memory_info *sub_sample_buf[2];

	union vpu_enc_get_header_arg seq_info;
	struct vpu_enc_seq_arg seq_para;
	struct vpu_enc_run_frame_arg run_info;
	struct vpu_enc_chg_para_arg chg_para;

	int reconChromaInterleave;
};

struct vpu_dec_ctx {
	int flush;
	int eos_tag;
	int delay_frm;
	int frame_buf_delay;
	int cur_reliable;

	int frm_type[VPU_MAX_BUFFERS];
	int interlace_flg[VPU_MAX_BUFFERS];
	int reliable_0_100[VPU_MAX_BUFFERS];
	struct timeval timeStamp[VPU_MAX_BUFFERS];
	int multiResolution[VPU_MAX_BUFFERS];
	int upSampledWidth[VPU_MAX_BUFFERS];
	int upSampledHeight[VPU_MAX_BUFFERS];

	struct timeval savedTimeStamp;

	unsigned int start_Addr;
	unsigned int end_Addr;

	int frame_buffer_cnt;
	struct nx_vid_memory_info frame_buf[VPU_MAX_BUFFERS-2];

	struct nx_memory_info *col_mv_buf;
	struct nx_memory_info *slice_buf;
	struct nx_memory_info *pv_slice_buf;

	struct list_head dpb_queue;
	unsigned int dpb_queue_cnt;

	int crop_left, crop_right, crop_top, crop_bot;

	/* for Jpeg */
	int32_t thumbnailMode;

	struct vpu_dec_reg_frame_arg *frameArg;
};

struct nx_vpu_fmt {
	char *name;
	unsigned int fourcc;
	unsigned int num_planes;
};

struct nx_vpu_ctx {
	struct nx_vpu_v4l2 *dev;
	struct v4l2_fh fh;

	int idx;

	enum nx_vpu_cmd vpu_cmd;

	void *hInst;				/* VPU handle */
	int is_initialized;
	int is_encoder;
	int codec_mode;

	int width;
	int height;

	int buf_y_width;
	int buf_c_width;
	int buf_height;

	int luma_size;
	int chroma_size;

	int chromaInterleave;

	uint32_t imgFourCC;

	unsigned int strm_size;
	unsigned int strm_buf_size;

	struct nx_memory_info *instance_buf;
	struct nx_memory_info *bit_stream_buf;

#if 0
	/* TBD. */
	struct list_head ctrls;
	struct list_head src_ctrls[VPU_MAX_BUFFERS];
	struct list_head dst_ctrls[VPU_MAX_BUFFERS];
	unsigned long src_ctrls_avail;
	unsigned long dst_ctrls_avail;
#endif

	struct nx_vpu_fmt img_fmt;
	struct nx_vpu_fmt *strm_fmt;

	struct vb2_queue vq_img;
	struct vb2_queue vq_strm;
	struct list_head img_queue;
	struct list_head strm_queue;
	unsigned int img_queue_cnt;
	unsigned int strm_queue_cnt;

	struct nx_vpu_codec_ops *c_ops;

	union {
		struct vpu_enc_ctx enc;
		struct vpu_dec_ctx dec;
	} codec;
};

struct nx_vpu_buf {
	struct vb2_buffer vb;
	struct list_head list;
	union {
		struct {
			dma_addr_t y;
			dma_addr_t cb;
			dma_addr_t cr;
		} raw;
		dma_addr_t stream;
	} planes;
	int used;
};


dma_addr_t nx_vpu_mem_plane_addr(struct nx_vpu_ctx *c, struct vb2_buffer *v,
	unsigned int n);
int nx_vpu_try_run(struct nx_vpu_ctx *ctx);

struct nx_vpu_fmt *find_format(struct v4l2_format *f);

int vidioc_querycap(struct file *file, void *priv, struct v4l2_capability *cap);
int vidioc_enum_fmt_vid_cap(struct file *file, void *pirv,
	struct v4l2_fmtdesc *f);
int vidioc_enum_fmt_vid_cap_mplane(struct file *file, void *pirv,
	struct v4l2_fmtdesc *f);
int vidioc_enum_fmt_vid_out(struct file *file, void *prov,
	struct v4l2_fmtdesc *f);
int vidioc_enum_fmt_vid_out_mplane(struct file *file, void *priv,
	struct v4l2_fmtdesc *f);
int vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *buf);
int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type type);
int vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type type);
int nx_vpu_queue_setup(struct vb2_queue *vq, const void *parg,
	unsigned int *buf_count, unsigned int *plane_count,
	unsigned int psize[], void *allocators[]);

void nx_vpu_unlock(struct vb2_queue *q);
void nx_vpu_lock(struct vb2_queue *q);
int nx_vpu_buf_init(struct vb2_buffer *vb);
int nx_vpu_buf_prepare(struct vb2_buffer *vb);
void nx_vpu_cleanup_queue(struct list_head *lh, struct vb2_queue *vq);

/* For Encoder V4L2 */
const struct v4l2_ioctl_ops *get_enc_ioctl_ops(void);

int nx_vpu_enc_open(struct nx_vpu_ctx *ctx);
int vpu_enc_open_instance(struct nx_vpu_ctx *ctx);
int vpu_enc_init(struct nx_vpu_ctx *ctx);
void vpu_enc_get_seq_info(struct nx_vpu_ctx *ctx);
int vpu_enc_encode_frame(struct nx_vpu_ctx *ctx);

int alloc_encoder_memory(struct nx_vpu_ctx *ctx);
int free_encoder_memory(struct nx_vpu_ctx *ctx);

/* For Decoder V4L2 */
const struct v4l2_ioctl_ops *get_dec_ioctl_ops(void);

int nx_vpu_dec_open(struct nx_vpu_ctx *ctx);
int vpu_dec_open_instance(struct nx_vpu_ctx *ctx);
int vpu_dec_parse_vid_cfg(struct nx_vpu_ctx *ctx);
int vpu_dec_init(struct nx_vpu_ctx *ctx);
int vpu_dec_decode_slice(struct nx_vpu_ctx *ctx);

int alloc_decoder_memory(struct nx_vpu_ctx *ctx);
int free_decoder_memory(struct nx_vpu_ctx *ctx);


#endif          /* #define _nx_vpu_v4l2_H */
