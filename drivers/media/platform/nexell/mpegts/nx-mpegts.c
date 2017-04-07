/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongkeun, Choi <jkchoi@nexell.co.kr>
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
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/dmaengine.h>
#include <linux/amba/pl08x.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/atomic.h>
#include <linux/irqreturn.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/v4l2-mediabus.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/interrupt.h>
#include <linux/reset.h>
#include <linux/clk.h>

#include <media/media-device.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-subdev.h>

#include <linux/soc/nexell/nx-media-device.h>

#define TEST	0

#define TIMER_IRQ	0
#define TIMEOUT_MS	(1000)

#define EXTRACT_BITS(data, area, loc) ((data >> (loc)) & (area))

#if TIMER_IRQ
#include <linux/timer.h>
#endif

#include "../nx-v4l2.h"
#include "../nx-video.h"

#include "nx-mpegtsi.h"
#include "nx-mpegts.h"

#define	NX_MPEGTS_DEV_NAME	"nx-mpegts"

#define V4L2_CID_MPEGTS_PAGE_SIZE       (V4L2_CTRL_CLASS_USER | 0x1001)

enum {
	NX_MPEGTS_PAD_SINK,
	NX_MPEGTS_PAD_SOURCE,
	NX_MPEGTS_PAD_MAX
};

enum {
	NX_MPEGTS_CAP0,
	NX_MPEGTS_CAP1,
	NX_MPEGTS_MAX
};

static int video_device_number = NX_MPEGTS_START;

static struct nx_video_format supported_formats[] = {
	{
		.name		= "MPEGTS",
		.pixelformat	= V4L2_PIX_FMT_MPEG,
		/*	.mbus_code	= MEDIA_BUS_FMT_JPEG_1X8,	*/
		.mbus_code	= 0,
		.num_planes	= 1,
		.num_sw_planes	= 1,
		.is_separated	= false,
	}
};

struct nx_mpegts_res {
	void *base;
};

enum {
	STATE_IDLE	= 0,
	STATE_RUNNING	= (1 << 0),
	STATE_STOPPING	= (1 << 1),
};

struct gpio_action_unit {
	int value;
	int delay_ms;
};

struct nx_mpegts_enable_gpio_action {
	int gpio_num;
	int count;

	struct gpio_action_unit *units;
};

struct nx_mpegts_power_action {
	int type;
	void *action;
};

struct nx_mpegts_power_seq {
	int count;
	struct nx_mpegts_power_action *actions;
};

struct nx_mpegts {
	struct nx_mpegts_res res;
	const struct nx_dev_mpegts_ops	*ops;
	struct platform_device *pdev;
	int irq;

	spinlock_t s_lock;

	atomic_t state;
	struct completion stop_done;
	struct semaphore s_stream_sem;

	struct clk *clk;
	struct reset_control *rst;

	bool buffer_underrun;
	struct nx_video_mpegts_buffer_manager vbuf_mng;

	struct ts_channel_info ch_info;
	struct ts_config_desc config_desc;
	struct ts_param_desc param_desc;

	int channel_num;

	struct timer_list timer_irq;

	struct nx_mpegts_power_seq enable_seq;
};

void dump_mpegts_register(void)
{
	struct nx_mpegtsi_register_set *p_reg =
		(struct nx_mpegtsi_register_set *)nx_mpegtsi_get_base_address();

	pr_info("BASE ADDRESS: 0x%p\n", p_reg);
	pr_info("CAP_CTRL0       = 0x%08x\n", p_reg->cap_ctrl[0]);
	pr_info("CAP_CTRL1       = 0x%08x\n", p_reg->cap_ctrl[1]);
	pr_info("CAP_WR_PID_VAL  = 0x%08x\n", p_reg->cpu_wrdata);
	pr_info("CAP_WR_PID_ADDR = 0x%08x\n", p_reg->cpu_wraddr);
	pr_info("CAP0_CAPDATA    = 0x%08x\n", p_reg->cap_data[0]);
	pr_info("CAP1_CAPDATA    = 0x%08x\n", p_reg->cap_data[1]);
	pr_info("TSP_INDATA      = 0x%08x\n", p_reg->tsp_indata);
	pr_info("TSP_OUTDATA     = 0x%08x\n", p_reg->tsp_outdata);
	pr_info("CTRL0           = 0x%08x\n", p_reg->ctrl0);
	pr_info("IDMAEN          = 0x%08x\n", p_reg->idma_enable);
	pr_info("IDMARUN         = 0x%08x\n", p_reg->idma_run);
	pr_info("IDMAINT         = 0x%08x\n", p_reg->idma_int);
	pr_info("IDMAADDR0       = 0x%08x\n", p_reg->idma_addr[0]);
	pr_info("IDMAADDR1       = 0x%08x\n", p_reg->idma_addr[1]);
	pr_info("IDMAADDR2       = 0x%08x\n", p_reg->idma_addr[2]);
	pr_info("IDMAADDR3       = 0x%08x\n", p_reg->idma_addr[3]);
	pr_info("IDMALEN0        = 0x%08x\n", p_reg->idma_len[0]);
	pr_info("IDMALEN1        = 0x%08x\n", p_reg->idma_len[1]);
	pr_info("IDMALEN2        = 0x%08x\n", p_reg->idma_len[2]);
	pr_info("IDMALEN3        = 0x%08x\n", p_reg->idma_len[3]);
}

static struct nx_video_format *find_format(u32 pixelformat, int index)
{
	struct nx_video_format *fmt, *def_fmt = NULL;
	unsigned i;

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

static void *get_video_target_context(struct device *dev)
{
	struct platform_device *pdev =
		container_of(dev, struct platform_device, dev);

	return platform_get_drvdata(pdev);
};

static void add_buffer(struct nx_video_mpegts_buffer_manager *mng,
		      struct nx_video_mpegts_buffer *buf)
{
	unsigned long flags;

	spin_lock_irqsave(&mng->slock, flags);
	list_add_tail(&buf->list, &mng->buffer_list);
	spin_unlock_irqrestore(&mng->slock, flags);

	mng->req_buffer_count++;
	atomic_inc(&mng->buffer_count);
}

static int buffer_count(struct nx_video_mpegts_buffer_manager *vbuf_mng)
{
	return atomic_read(&vbuf_mng->buffer_count);
}

static struct nx_video_mpegts_buffer *get_next_buffer(struct
					       nx_video_mpegts_buffer_manager
					       *vbuf_mng, bool remove)
{
	unsigned long flags;
	struct nx_video_mpegts_buffer *buf = NULL;

	spin_lock_irqsave(&vbuf_mng->slock, flags);
	if (!list_empty(&vbuf_mng->buffer_list)) {
		buf = list_first_entry(&vbuf_mng->buffer_list, struct
				       nx_video_mpegts_buffer, list);
		if (remove) {
			list_del_init(&buf->list);
			atomic_dec(&vbuf_mng->buffer_count);
		}
	}
	spin_unlock_irqrestore(&vbuf_mng->slock, flags);

	return buf;
}

static int buffer_done(struct nx_video_mpegts_buffer *buf)
{
	struct vb2_buffer *vb = buf->priv;
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	/*	struct nx_video_mpegts *me = vb->vb2_queue->drv_priv;	*/

	v4l2_get_timestamp(&vbuf->timestamp);
	vb2_buffer_done(vb, VB2_BUF_STATE_DONE);

	return 0;
}

static void clear_buffer(struct nx_video_mpegts_buffer_manager *mng)
{
	struct nx_video_mpegts_buffer *buf = NULL;
	struct vb2_buffer *vb = NULL;

	if (buffer_count(mng)) {
		unsigned long flags;

		spin_lock_irqsave(&mng->slock, flags);
		while (!list_empty(&mng->buffer_list)) {
			buf = list_entry(mng->buffer_list.next,
				   struct nx_video_mpegts_buffer, list);
			if (buf) {
				vb = buf->priv;
				vb2_buffer_done(vb, VB2_BUF_STATE_ERROR);
				list_del_init(&buf->list);
			} else
				break;
		}
		INIT_LIST_HEAD(&mng->buffer_list);
		spin_unlock_irqrestore(&mng->slock, flags);
	}

	atomic_set(&mng->buffer_count, 0);

	mng->req_buffer_count	= 0;
}

static void copy_buffer_addr(dma_addr_t *dma_phy_addr,
			     struct nx_video_mpegts_buffer_manager *mng)
{
	struct nx_video_mpegts_buffer *buf = NULL;
	int i = 0;

	unsigned long flags;

	spin_lock_irqsave(&mng->slock, flags);
	if (!list_empty(&mng->buffer_list)) {
		list_for_each_entry(buf, &mng->buffer_list, list) {
			if (buf) {
				dma_phy_addr[i] = buf->dma_phy_addr;
#if 0
				pr_info("%s - dma_phy_addr[%d] : 0x%08x\n",
					__func__, i, dma_phy_addr[i]);
#endif
				i++;
			}
		}
	}
	spin_unlock_irqrestore(&mng->slock, flags);
}


static int nx_vb2_queue_setup(struct vb2_queue *q, const void *parg,
			      unsigned int *num_buffers,
			      unsigned int *num_planes,
			      unsigned int sizes[],
			      void *alloc_ctxs[])
{
	struct nx_video_mpegts *me = q->drv_priv;
	struct nx_video_format *fmt;
	int i = 0;

	me->format.pixelformat = V4L2_PIX_FMT_MPEG;

	switch (q->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		/*	fmt = find_format(V4L2_PIX_FMT_MPEG, 0);	*/
		fmt = find_format(me->format.pixelformat, 0);
		break;
	default:
		pr_err("invalid format type(0x%x)\n", q->type);
		return -EINVAL;
	}

	switch (q->memory) {
	case V4L2_MEMORY_MMAP:
		*num_planes = fmt->num_planes;
		break;
	default:
		pr_err("invalid v4l2 memory type(0x%x)\n", q->memory);
		return -EINVAL;
	}

	for (i = 0; i < *num_planes; ++i) {
		alloc_ctxs[i] = me->vb2_alloc_ctx;
		sizes[i] = me->page_size;
	}

	return 0;
}

static int nx_vb2_buf_init(struct vb2_buffer *vb)
{
	struct nx_video_mpegts_buffer **bufs;
	struct nx_video_mpegts_buffer *buf;
	struct nx_video_mpegts *me = vb->vb2_queue->drv_priv;
	int index = vb->index;
	u32 type = vb->vb2_queue->type;

	switch (type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		bufs = me->sink_bufs;
		break;
	default:
		pr_err("%s invalid buffer type(0x%x)\n", __func__, type);
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
		bufs[index] = buf;
	}

	return 0;
}

static void nx_vb2_buf_cleanup(struct vb2_buffer *vb)
{
	struct nx_video_mpegts_buffer **bufs;
	struct nx_video_mpegts_buffer *buf;
	struct nx_video_mpegts *me = vb->vb2_queue->drv_priv;
	int index = vb->index;
	u32 type = vb->vb2_queue->type;

	switch (type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		bufs = me->sink_bufs;
		break;
	default:
		pr_err("%s invalid buffer type(0x%x)\n", __func__, type);
		return;
	}

	buf = bufs[index];
	kfree(buf);
	bufs[index] = NULL;
}

static void nx_vb2_buf_queue(struct vb2_buffer *vb)
{
	struct nx_video_mpegts_buffer *buf;
	struct nx_video_mpegts *me = vb->vb2_queue->drv_priv;
	u32 type = vb->vb2_queue->type;
	struct nx_mpegts *ctx =
		(struct nx_mpegts *)get_video_target_context(me->t_dev);

	switch (type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		buf = me->sink_bufs[vb->index];
		break;
	default:
		pr_err("%s invalid buffer type(0x%x)\n", __func__, type);
		return;
	}

	if (!buf) {
		pr_err("no buf(%p)\n", buf);
		WARN_ON(1);
		return;
	}
	buf->dma_phy_addr = vb2_dma_contig_plane_dma_addr(vb, 0);
	buf->dma_virt_addr = vb2_plane_vaddr(vb, 0);
	buf->index = vb->index;
#if 0
	pr_info("%s - vb->vb2_queue->num_buffers : %d\n", __func__,
		vb->vb2_queue->num_buffers);
	pr_info("%s - vb->vb2_queue->plane_sizes : %d\n", __func__,
		vb->vb2_queue->plane_sizes[0]);

	pr_info("%s - buf index : %d\n", __func__, buf->index);
	pr_info("%s - dma phy addr : 0x%08x\n", __func__,
		buf->dma_phy_addr);
	pr_info("%s - dma virt addr : 0x%p\n", __func__,
		buf->dma_virt_addr);
	pr_info("\n");
#endif
	add_buffer(&ctx->vbuf_mng, buf);
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

	struct nx_video_mpegts *me = q->drv_priv;
	struct nx_mpegts *ctx =
		(struct nx_mpegts *)get_video_target_context(me->t_dev);

	if (!ctx)
		return -EINVAL;

	return ctx->ops->s_stream(ctx, 1);
}

static void nx_vb2_stop_streaming(struct vb2_queue *q)
{
	struct nx_video_mpegts *me = q->drv_priv;
	struct nx_mpegts *ctx =
		(struct nx_mpegts *)get_video_target_context(me->t_dev);


	if (ctx)
		ctx->ops->s_stream(ctx, 0);
}

static struct vb2_ops nx_vb2_ops = {
	.queue_setup		= nx_vb2_queue_setup,
	.buf_init		= nx_vb2_buf_init,
	.buf_cleanup		= nx_vb2_buf_cleanup,
	.buf_queue		= nx_vb2_buf_queue,
	.buf_finish		= nx_vb2_buf_finish,
	.start_streaming	= nx_vb2_start_streaming,
	.stop_streaming		= nx_vb2_stop_streaming,
};

/***
 *  set device register
 ***/
static int _power_off_device(u8 ch_num)
{
	nx_mpegtsi_set_cap_enable(ch_num, false);
	nx_mpegtsi_set_cap_sram_power_enable(ch_num, false);
	nx_mpegtsi_set_cap_sram_wakeup(ch_num, false);

	return 0;
}

static int _power_on_device(u8 ch_num)
{
	int i = 0;
#if 0
	u32 val;
	void *reg;

	reg = ioremap_nocache(0xC001D024, 0x1000);
	val = readl(reg);
	pr_info("%s - GPIO D 31 ~ 16 ALT: 0x%X\n", __func__, val);

	val = 0;
	reg = ioremap_nocache(0xC001E020, 0x1000);
	val = readl(reg);
	pr_info("%s - GPIO E 0 ~ 15 ALT: 0x%X\n", __func__, val);


	val = 0;
	reg = ioremap_nocache(0xC001D058, 0x1000);
	val = readl(reg);
	pr_info("%s - GPIO D 31 ~ 0 Pull UP: 0x%X\n", __func__, val);

	val = 0;
	reg = ioremap_nocache(0xC001E058, 0x1000);
	val = readl(reg);
	pr_info("%s - GPIO E 31 ~ 0 Pull UP: 0x%X\n", __func__, val);

	val = 0;
	reg = ioremap_nocache(0xC0012004, 0x1000);
	val = readl(reg);
	pr_info("%s - MPEGTS NO RESET Check: [0x%x], 0x%x\n", __func__, val,
		EXTRACT_BITS(val, 0x1, 2));

	val = 0;
	reg = ioremap_nocache(0xC00B7000, 0x1000);
	val = readl(reg);
	pr_info("%s - MPEGTSI CLK ENB: 0x%x\n", __func__, val);
#endif
	nx_mpegtsi_set_cap_enable(ch_num, false);
	nx_mpegtsi_set_cap_sram_power_enable(ch_num, true);
	nx_mpegtsi_set_cap_sram_wakeup(ch_num, true);

	for (i = 0; i < TS_CAPx_PID_MAX; i++)
		nx_mpegtsi_write_pid(ch_num, NX_PARAM_TYPE_PID, i, 0x1fff1fff);

	return 0;
}

static int _set_config(struct nx_mpegts *me)
{
	int channel_num;
	int clock_pol;
	int valid_pol;
	int sync_pol;
	int err_pol;
	int serial_mode;
	int bypass;
	int xfer_mode;
	int xfer_clk_pol;

	channel_num	= me->channel_num;
	clock_pol	= me->config_desc.clock_pol;
	valid_pol	= me->config_desc.valid_pol;
	sync_pol	= me->config_desc.sync_pol;
	err_pol		= me->config_desc.err_pol;
	serial_mode	= me->config_desc.serial_mode;
	bypass		= me->config_desc.bypass;
	xfer_mode	= me->config_desc.xfer_mode;
	xfer_clk_pol	= me->config_desc.xfer_clk_pol;
#if 0
	pr_info("%s channel_num : %d\n", __func__, channel_num);
	pr_info("%s clock_pol : %d\n", __func__, clock_pol);
	pr_info("%s valid_pol : %d\n", __func__, valid_pol);
	pr_info("%s sync_pol : %d\n", __func__, sync_pol);
	pr_info("%s err_pol : %d\n", __func__, err_pol);
	pr_info("%s serial_mode : %d\n", __func__, serial_mode);
	pr_info("%s bypass : %d\n", __func__, bypass);
	pr_info("%s xfer_mode : %d\n", __func__, xfer_mode);
	pr_info("%s xfer_clock_pol : %d\n", __func__, xfer_clk_pol);
#endif

	nx_mpegtsi_set_tclk_polarity_enable(channel_num, clock_pol);
	nx_mpegtsi_set_tdp_polarity_enable(channel_num, valid_pol);
	nx_mpegtsi_set_tsync_polarity_enable(channel_num, sync_pol);
	nx_mpegtsi_set_terr_polarity_enable(channel_num, err_pol);
	nx_mpegtsi_set_serial_enable(channel_num, serial_mode);
	nx_mpegtsi_set_bypass_enable(channel_num, bypass);

	switch (channel_num) {
	case NX_MPEGTS_CAP1:
		if (xfer_mode) {
			me->ch_info.tx_mode = 1;
			nx_mpegtsi_set_cap1_output_enable(true);
			nx_mpegtsi_set_cap1_out_tclk_polarity_enable
				(xfer_clk_pol);
		} else {
			me->ch_info.tx_mode = 0;
			nx_mpegtsi_set_cap1_output_enable(false);
		}
	}

	return 0;
}

static int _set_param(struct nx_mpegts *me)
{
	int max_cnt;
	u16 *PID_buf16;
	u32 temp;
	int channel_num;
	int idx;
	int i;
	void *clear_buf = kmalloc(64, GFP_KERNEL);

	memset(clear_buf, 0x00, sizeof(clear_buf));

	channel_num	= me->channel_num;

	PID_buf16 = clear_buf;

	max_cnt = sizeof(clear_buf);
	for (idx = 0, i = 0; i < (max_cnt/2); i++) {
		temp = ((PID_buf16[idx + 1] << 16) | PID_buf16[idx]);
		nx_mpegtsi_write_pid(channel_num, NX_PARAM_TYPE_PID, i, temp);

		idx += 2;
	}

	if (max_cnt & 1) {
		temp = ((NX_NO_PID << 16) | PID_buf16[idx]);
		nx_mpegtsi_write_pid(channel_num, NX_PARAM_TYPE_PID, i,
				     temp);
	}

	kfree(clear_buf);

	return 0;
}

static int _clear_param(struct nx_mpegts *me)
{
	int i;
	int channel_num;

	channel_num	= me->channel_num;

	for (i = 0; i < TS_CAPx_PID_MAX; i++)
		nx_mpegtsi_write_pid(channel_num, NX_PARAM_TYPE_PID, i,
				     0x1fff1fff);

	return 0;
}

static inline int _init_dma(struct nx_mpegts *me)
{
	dma_filter_fn	filter_fn;
	dma_cap_mask_t	mask;
	int ch_num;

	filter_fn	= pl08x_filter_id;

	ch_num = me->channel_num;

	switch (ch_num) {
	case NX_MPEGTS_CAP0:
		me->ch_info.filter_data = DMA_PERIPHERAL_NAME_MPEGTSI0;
		break;
	case NX_MPEGTS_CAP1:
		me->ch_info.filter_data = DMA_PERIPHERAL_NAME_MPEGTSI1;
		break;
	}

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_cap_set(DMA_CYCLIC, mask);

	me->ch_info.dma_chan = dma_request_channel(mask, filter_fn,
						   me->ch_info.filter_data);
	if (!me->ch_info.dma_chan) {
		pr_err("error: dma %s\n", (char *)me->ch_info.filter_data);
		goto error_request;
	}

	return 0;

error_request:
	dma_release_channel(me->ch_info.dma_chan);

	return -EINVAL;
}

static inline int _deinit_dma(struct nx_mpegts *me)
{
	if (me->ch_info.dma_chan) {
		dma_release_channel(me->ch_info.dma_chan);
		me->ch_info.dma_chan = NULL;
	}
	me->ch_info.cnt = 0;

	return 0;
}

static inline void _w_buf(struct ts_channel_info *buf)
{
	if (buf->cnt < buf->page_num) {
		buf->cnt++;
		buf->w_pos	= (buf->w_pos + 1) % buf->page_num;
	}
}

static inline void _r_buf(struct ts_channel_info *buf)
{
	if (buf->cnt) {
		buf->cnt--;
		buf->r_pos = (buf->r_pos + 1) % buf->page_num;
	}
}

static inline int _w_able(struct ts_channel_info *buf)
{
	if (buf->cnt < buf->page_num)
		return 1;

	return 0;
}

static inline int _r_able(struct ts_channel_info *buf)
{
	if (buf->cnt)
		return 1;

	return 0;
}

static inline int _rw_able(struct ts_channel_info *buf)
{
	int ret = 0;

	if (buf->is_first)
		ret = 1;
	else
		if (buf->r_pos && (buf->w_pos == (buf->r_pos-1)))
			ret = 0;
		else if (!buf->r_pos && (buf->w_pos == (buf->page_num-1)))
			ret = 0;

	return ret;
}

static void mpegts_capture_dma_irq(void *arg)
{
	struct nx_mpegts *me = arg;
	struct nx_video_mpegts_buffer_manager *mng = &me->vbuf_mng;
	struct nx_video_mpegts_buffer *done_buf = NULL;

	if (atomic_read(&me->state) & STATE_STOPPING) {
		complete(&me->stop_done);
	} else {
		if (buffer_count(mng) > 0)
			done_buf = get_next_buffer(mng, true);

		if (done_buf) {

			if (me->ch_info.tx_mode)
				_r_buf(&me->ch_info);
			else
				_w_buf(&me->ch_info);

			if (me->ch_info.wait_time)
				wake_up(&me->ch_info.wait);

			if (done_buf->cb_buf_done) {
				done_buf->cb_buf_done(done_buf);
				if (!me->ch_info.tx_mode)
					_r_buf(&me->ch_info);
			}
		}
	}
}

static int _prepare_dma_submit(struct nx_mpegts *me)
{
	struct dma_chan *dma_chan;
	struct dma_slave_config slave_config = {0, };
	size_t total_buf_len;
	int ret;
	int ch_num;
	int i = 0;

	struct scatterlist sg[VB2_MAX_FRAME];

	ch_num = me->channel_num;

	nx_mpegtsi_set_idma_enable(ch_num, false);
	nx_mpegtsi_run_idma(ch_num);

	dma_chan = me->ch_info.dma_chan;

	switch (me->ch_info.tx_mode) {
	case 0:
		slave_config.direction		= DMA_DEV_TO_MEM;
		slave_config.src_addr		=
			(nx_mpegtsi_get_physical_address() + 0x10)
			+ (ch_num * 0x04);
		slave_config.src_addr_width	= DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.src_maxburst	= 8;
		slave_config.dst_addr_width	= DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.dst_maxburst	= 8;
		slave_config.device_fc		= false;
		break;
	case 1:
		slave_config.direction		= DMA_MEM_TO_DEV;
		slave_config.dst_addr		=
			(nx_mpegtsi_get_physical_address() + 0x10)
			+ (ch_num * 0x04);
		slave_config.dst_addr_width	= DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.dst_maxburst	= 8;
		slave_config.src_addr_width	= DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.src_maxburst	= 8;
		slave_config.device_fc		= false;
		break;
	}

	ret = dmaengine_slave_config(dma_chan, &slave_config);
	if (ret < 0)
		return -EINVAL;

	me->ch_info.page_size	= me->vbuf_mng.video->page_size;
	me->ch_info.page_num	= me->vbuf_mng.req_buffer_count;

	total_buf_len = me->ch_info.page_num * me->ch_info.page_size;

#if 0
	pr_info("%s - page size : %d\n", __func__, me->ch_info.page_size);
	pr_info("%s - page num : %d\n", __func__, me->ch_info.page_num);
	pr_info("%s - total len : %d\n", __func__, total_buf_len);
#endif

	copy_buffer_addr(&me->ch_info.dma_phy[0], &me->vbuf_mng);

#if 0
	for (i = 0; i < me->ch_info.page_num; i++)
		pr_info("%s - dma phy[%d] : 0x%08x\n", __func__, i,
			me->ch_info.dma_phy[i]);
#endif

	sg_init_table(sg, me->ch_info.page_num);
	for (i = 0; i < me->ch_info.page_num; i++) {
		sg_dma_address(&sg[i]) = me->ch_info.dma_phy[i];
		sg_dma_len(&sg[i]) = me->ch_info.page_size;
	}

	me->ch_info.desc = dmaengine_prep_slave_sg(dma_chan,
						   &sg[0],
						   me->ch_info.page_num,
						   slave_config.direction,
						   DMA_PREP_INTERRUPT
						   | DMA_PREP_CONTINUE);
	if (!me->ch_info.desc) {
		pr_err("%s cannot prepare slave %s dma\n", __func__,
		       (char *)(me->ch_info.filter_data));
		goto err_prepare;
	}

	me->ch_info.desc->callback	= mpegts_capture_dma_irq;

	me->ch_info.desc->callback_param = me;
	dmaengine_submit(me->ch_info.desc);

	return 0;

err_prepare:
	dma_release_channel(me->ch_info.dma_chan);

	return -EINVAL;
}

static inline void _start_dma(struct nx_mpegts *me)
{
	if (me->ch_info.dma_chan)
		_prepare_dma_submit(me);

	if (me->ch_info.tx_mode != 1)
		dma_async_issue_pending(me->ch_info.dma_chan);
	else
		me->ch_info.do_continue	= 1;
}

static inline void _stop_dma(struct nx_mpegts *me)
{
	if (me->ch_info.dma_chan)
		dmaengine_terminate_all(me->ch_info.dma_chan);

	me->ch_info.cnt = 0;
}

static inline void _init_addr(struct nx_mpegts *me)
{
	int i;
	int page_num = 0;

	page_num = me->vbuf_mng.req_buffer_count;
	for (i = 0; i < page_num; i++) {
		me->ch_info.dma_phy[i]	= 0;
		me->ch_info.dma_virt[i] = NULL;
	}
}

static inline int _init_buf(struct nx_mpegts *me)
{
	if (me->ch_info.is_running == 0) {
		me->ch_info.cnt		= 0;
		me->ch_info.w_pos	= 0;
		me->ch_info.r_pos	= 0;

		_init_addr(me);
	}

	return 0;
}

static int _prepare_dma(struct nx_mpegts *me)
{
	int ret = 0;
	int ch_num;

	ch_num	= me->channel_num;
	ret = _init_dma(me);
	if (ret != 0)
		goto exit_cleanup;

	ret = _init_buf(me);
	if (ret != 0) {
		_deinit_dma(me);
		goto exit_cleanup;
	}

exit_cleanup:
	return ret;
}



static inline int _enable_device(struct nx_mpegts *me)
{
	int ch_num;

	ch_num	= me->channel_num;

	/*	update_buffer(me);	*/
	_start_dma(me);

	nx_mpegtsi_set_cap_int_lock_enable(ch_num, true);
	nx_mpegtsi_set_cap_int_mask_clear(ch_num, true);
	nx_mpegtsi_set_cap_enable(ch_num, true);

	return 0;
}

static inline int _disable_device(struct nx_mpegts *me)
{
	int ch_num;

	ch_num	= me->channel_num;

	nx_mpegtsi_set_cap_int_mask_clear(ch_num, false);
	nx_mpegtsi_set_cap_int_enable(ch_num, false);
	nx_mpegtsi_set_cap_int_clear(ch_num);

	nx_mpegtsi_set_cap_enable(ch_num, false);

	nx_mpegtsi_set_cap_sram_wakeup(ch_num, false);
	nx_mpegtsi_set_cap_sram_power_enable(ch_num, false);

	msleep(20);

	return 0;
}

static int nx_mpegts_clock_enable(struct nx_mpegts *me, bool enable)
{
	if (enable)
		return clk_prepare_enable(me->clk);

	clk_disable_unprepare(me->clk);

	return 0;
}

static int nx_mpegts_reset(struct nx_mpegts *me)
{
	if (reset_control_status(me->rst))
		return reset_control_reset(me->rst);

	return 0;
}


static void _init_device(struct nx_mpegts *me)
{
	nx_mpegtsi_set_base_address(me->res.base);
	nx_mpegts_clock_enable(me, true);
	nx_mpegts_reset(me);
}

static inline int ts_init(struct nx_mpegts *me)
{
	int ch_num;
	int ret;

	ch_num	= me->channel_num;

	_power_off_device(ch_num);
	_power_on_device(ch_num);
	_set_config(me);
	_set_param(me);

	if (me->ch_info.is_running == 0) {
		ret = _prepare_dma(me);
		if (ret != 0) {
			dev_err(&me->pdev->dev,
			"failed to prepare dma!!!!\n");
			return ret;
		}
	}

	return 0;
}

static inline int ts_deinit(struct nx_mpegts *me)
{
	int ch_num;

	if (me->ch_info.is_running)
		return -EBUSY;

	ch_num	= me->channel_num;

	/*	_clear_param(me);	*/
	_deinit_dma(me);

	return 0;
}

static inline int ts_start(struct nx_mpegts *me)
{
	int ret = 0;

	if (me->ch_info.is_running)
		return -2;

	ret = _enable_device(me);
	if (ret != 0) {
		me->ch_info.is_running = 0;
		_stop_dma(me);
	} else
		me->ch_info.is_running = 1;

	return ret;
}

static inline int ts_stop(struct nx_mpegts *me)
{
	if (me->ch_info.is_running) {
		me->ch_info.is_running = 0;
		_disable_device(me);
		_stop_dma(me);
	}

	return 0;
}

/* ---------------------------------------------------------------- */

static int nx_video_mpegts_open(struct file *file)
{
	struct nx_video_mpegts *me = video_drvdata(file);
	int ret = 0;

	me->open_count++;
	file->private_data = me;

	return ret;
}

static int nx_video_mpegts_release(struct file *file)
{
	struct nx_video_mpegts *me = video_drvdata(file);
	int ret = 0;

	me->open_count--;

	if (me->open_count == 0) {
		if (me->vbq)
			vb2_queue_release(me->vbq);
	}

	file->private_data = 0;

	return ret;
}

static unsigned int nx_video_mpegts_poll(struct file *file,
					 struct poll_table_struct *tbl)
{
		struct nx_video_mpegts	*me = file->private_data;

		return vb2_poll(me->vbq, file, tbl);
}

static int nx_video_mpegts_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct nx_video_mpegts *me = file->private_data;

	return vb2_mmap(me->vbq, vma);
}

static struct v4l2_file_operations nx_video_mpegts_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= video_ioctl2,
	.open		= nx_video_mpegts_open,
	.release	= nx_video_mpegts_release,
	.poll		= nx_video_mpegts_poll,
	.mmap		= nx_video_mpegts_mmap,
};

static int nx_video_mpegts_querycap(struct file *file, void *fh,
			     struct v4l2_capability *cap)
{
	struct nx_video_mpegts *me = file->private_data;

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

static int nx_video_mpegts_enum_format(struct file *file, void *fh,
				       struct v4l2_fmtdesc *f)
{
	return 0;
}

static int nx_video_mpegts_get_format(struct file *file, void *fh,
				      struct v4l2_format *f)
{
	return 0;
}

static int nx_video_mpegts_set_format(struct file *file, void *fh,
				      struct v4l2_format *f)
{
	struct nx_video_mpegts *me = file->private_data;

	/*	int ret;	*/
	/*	int i;	*/
	/*	u32 width, height, pixelformat, colorspace, field;	*/

	if (me->vbq->type != f->type) {
		vb2_queue_release(me->vbq);
		nx_video_mpegts_vbq_init(me, f->type);
	}

	return 0;
}

static int nx_video_mpegts_try_format(struct file *file, void *fh,
				      struct v4l2_format *f)
{
	return 0;
}

static int nx_video_mpegts_reqbufs(struct file *file, void *fh,
				   struct v4l2_requestbuffers *b)
{
	struct nx_video_mpegts *me = file->private_data;

	if (me->vbq)
		return vb2_reqbufs(me->vbq, b);

	return -EINVAL;
}

static int nx_video_mpegts_querybuf(struct file *file, void *fh,
				    struct v4l2_buffer *b)
{
	struct nx_video_mpegts *me = file->private_data;

	if (me->vbq)
		return vb2_querybuf(me->vbq, b);

	return -EINVAL;
}

static int nx_video_mpegts_qbuf(struct file *file, void *fh,
				struct v4l2_buffer *b)
{
	struct nx_video_mpegts *me = file->private_data;

	/*	pr_info("%s - type : %d\n", __func__, b->type);	*/

	if (me->vbq)
		return vb2_qbuf(me->vbq, b);

	return -EINVAL;
}

static int nx_video_mpegts_dqbuf(struct file *file, void *fh,
				 struct v4l2_buffer *b)
{

	struct nx_video_mpegts *me = file->private_data;

	if (me->vbq)
		return vb2_dqbuf(me->vbq, b, file->f_flags & O_NONBLOCK);

	return -EINVAL;
}

static int nx_video_mpegts_streamon(struct file *file, void *fh, enum
				    v4l2_buf_type i)
{
	struct nx_video_mpegts *me = file->private_data;

	if (me->vbq)
		return vb2_streamon(me->vbq, i);

	return -EINVAL;
}

static int nx_video_mpegts_streamoff(struct file *file, void *fh, enum
				     v4l2_buf_type i)
{
	struct nx_video_mpegts *me = file->private_data;

	if (me->vbq)
		return vb2_streamoff(me->vbq, i);

	return -EINVAL;
}

static int nx_video_mpegts_g_ctrl(struct file *file, void *fh,
				  struct v4l2_control *ctrl)
{
	return 0;
}

static int nx_video_mpegts_s_ctrl(struct file *file, void *fh,
				  struct v4l2_control *ctrl)
{
	struct nx_video_mpegts *me;

	me = file->private_data;

	switch (ctrl->id) {
	case V4L2_CID_MPEGTS_PAGE_SIZE:
		me->page_size = ctrl->value;
		break;
	default:
		pr_err("%s: invalid control id 0x%x\n", __func__,
		       ctrl->id);
		return -EINVAL;
	}

	return 0;
}

static struct v4l2_ioctl_ops nx_video_mpegts_ioctl_ops	= {
	.vidioc_querycap		= nx_video_mpegts_querycap,
	.vidioc_enum_fmt_vid_cap_mplane	= nx_video_mpegts_enum_format,
	.vidioc_enum_fmt_vid_out_mplane	= nx_video_mpegts_enum_format,
	.vidioc_g_fmt_vid_cap_mplane	= nx_video_mpegts_get_format,
	.vidioc_g_fmt_vid_out_mplane	= nx_video_mpegts_get_format,
	.vidioc_s_fmt_vid_cap_mplane	= nx_video_mpegts_set_format,
	.vidioc_s_fmt_vid_out_mplane	= nx_video_mpegts_set_format,
	.vidioc_try_fmt_vid_cap_mplane	= nx_video_mpegts_try_format,
	.vidioc_try_fmt_vid_cap		= nx_video_mpegts_try_format,
	.vidioc_enum_fmt_vid_cap	= nx_video_mpegts_enum_format,
	.vidioc_g_fmt_vid_cap		= nx_video_mpegts_get_format,
	.vidioc_s_fmt_vid_cap		= nx_video_mpegts_set_format,
	.vidioc_reqbufs			= nx_video_mpegts_reqbufs,
	.vidioc_querybuf		= nx_video_mpegts_querybuf,
	.vidioc_qbuf			= nx_video_mpegts_qbuf,
	.vidioc_dqbuf			= nx_video_mpegts_dqbuf,
	.vidioc_streamon		= nx_video_mpegts_streamon,
	.vidioc_streamoff		= nx_video_mpegts_streamoff,
	.vidioc_g_ctrl			= nx_video_mpegts_g_ctrl,
	.vidioc_s_ctrl			= nx_video_mpegts_s_ctrl,
};

static int nx_video_mpegts_vbq_init(struct nx_video_mpegts *me, uint32_t type)
{
	struct vb2_queue *vbq = me->vbq;
#if 1
	struct nx_mpegts *ctx =
		(struct nx_mpegts *)get_video_target_context(me->t_dev);
	pr_info("%s - channel number : %d\n", __func__, ctx->channel_num);
	pr_info("%s - video virt addr : 0x%p\n", __func__, me);
#endif

	vbq->type	= type;
	vbq->io_modes	= VB2_DMABUF | VB2_MMAP;
	vbq->drv_priv	= me;
	vbq->ops	= &nx_vb2_ops;
	vbq->mem_ops	= &vb2_dma_contig_memops;

	vbq->timestamp_flags	= V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;

	return vb2_queue_init(vbq);
}

static void init_me(struct nx_mpegts *me);

static int nx_mpegts_s_stream(struct nx_mpegts *me, int enable)
{
	int ret;
	char *host_name = me->vbuf_mng.video->vdev.name;
	bool is_host_video = false;

	pr_info("%s enable : %d\n", __func__, enable);

	if (!host_name)
		return -EEXIST;

	if (!strncmp(host_name, "VIDEO", 5))
		is_host_video = true;

	ret = down_interruptible(&me->s_stream_sem);

	if (enable) {
		if (atomic_read(&me->state) & STATE_STOPPING) {
			int timeout = 50;

			dev_info(&me->pdev->dev, "wait mpegts stopping..!\n");
			while (atomic_read(&me->state) & STATE_STOPPING) {
				msleep(100);
				timeout--;
				if (timeout == 0) {
					dev_err(&me->pdev->dev,
							"timeout for waiting");
					dev_err(&me->pdev->dev,
							"mpegts stop\n");
					break;
				}
			}
		}

		if ((!atomic_read(&me->state)) & STATE_RUNNING) {
			/*	TODO : start mpegts IP	*/
			ret = enable_sensor_power(me);
			if (ret) {
				WARN_ON(1);
				goto UP_AND_OUT;
			}
		}

		if (is_host_video) {
/*
*			1. register mpegts irq handle
*			2. update nexe buffer
*			3. mpegts run
*/
			ts_init(me);
			ts_start(me);
			/*	dump_mpegts_register();	*/
		}

		NX_ATOMIC_SET_MASK(STATE_RUNNING, &me->state);
#if TIMER_IRQ
		mod_timer(&me->timer_irq, jiffies +
				msecs_to_jiffies(TIMEOUT_MS));
#endif
	} else {
		if (!(atomic_read(&me->state) & STATE_RUNNING))
			goto UP_AND_OUT;

		if (is_host_video &&
			 (atomic_read(&me->state) & STATE_RUNNING)) {
			if (!me->buffer_underrun) {
				NX_ATOMIC_SET_MASK(STATE_STOPPING, &me->state);
				if (!wait_for_completion_timeout(&me->stop_done,
								 2*HZ)) {
					pr_warn("timeout for waiting mpegts");
					pr_warn("stop\n");
				}

				NX_ATOMIC_CLEAR_MASK(STATE_STOPPING,
						     &me->state);
			}

			me->buffer_underrun = false;
			ts_stop(me);
			ts_deinit(me);

			/*	TODO: unregister irq	*/

			clear_buffer(&me->vbuf_mng);
			NX_ATOMIC_CLEAR_MASK(STATE_RUNNING, &me->state);
			if (!is_host_video)
				if (atomic_read(&me->state) == STATE_IDLE)
					goto UP_AND_OUT;
		}
	}

UP_AND_OUT:
	up(&me->s_stream_sem);

	return 0;
}

static const struct nx_dev_mpegts_ops nx_mpegts_ops = {
	.s_stream	= nx_mpegts_s_stream,
};

static void nx_video_mpegts_init_vbuf_mng(struct nx_video_mpegts_buffer_manager
				      *vbuf_mng)
{
	spin_lock_init(&vbuf_mng->slock);
	INIT_LIST_HEAD(&vbuf_mng->buffer_list);
	atomic_set(&vbuf_mng->buffer_count, 0);
	vbuf_mng->req_buffer_count = 0;
}

static void init_me(struct nx_mpegts *me)
{
	spin_lock_init(&me->s_lock);

	atomic_set(&me->state, STATE_IDLE);
	init_completion(&me->stop_done);
	sema_init(&me->s_stream_sem, 1);
	nx_video_mpegts_init_vbuf_mng(&me->vbuf_mng);
	me->ops = &nx_mpegts_ops;

	me->ch_info.alloc_size	= 0;
	me->ch_info.page_size	= 0;
	me->ch_info.page_num	= 0;

	_init_addr(me);

	me->ch_info.is_running	= 0;
	me->ch_info.is_first	= 1;
	me->ch_info.do_continue	= 0;

	init_waitqueue_head(&me->ch_info.wait);
}

static int deinit_me(struct nx_mpegts *me)
{
	wake_up(&me->ch_info.wait);

	return 0;
}


#if TIMER_IRQ
static void timer_handler(unsigned long priv)
{
	struct nx_mpegts *me = (struct nx_mpegts *)priv;
	struct nx_video_mpegts_buffer_manager *mng = &me->vbuf_mng;
	struct nx_video_mpegts_buffer *done_buf = NULL;

	pr_info("+++ %s +++\n", __func__);

	if (buffer_count(mng) > 0)
		done_buf = get_next_buffer(mng, true);

	if (done_buf) {
		update_buffer(me);

		if (done_buf->cb_buf_done)
			done_buf->cb_buf_done(done_buf);
#if 0
		dev_info(&me->pdev->dev, "%s: start_dma_addr : 0x%X",
			__func__, mng->start_dma_addr);
#endif
		dev_info(&me->pdev->dev, "%s: DMA phy addr : 0x%X\n",
			__func__, done_buf->dma_phy_addr);
		dev_info(&me->pdev->dev, "%s: DMA virt addr : 0x%p\n\n",
			__func__, done_buf->dma_virt_addr);
	}

	pr_info("--- %s ---\n", __func__);
	if (atomic_read(&me->state) & STATE_RUNNING)
		mod_timer(&me->timer_irq, jiffies +
			  msecs_to_jiffies(TIMEOUT_MS));
}
#endif

struct nx_video_mpegts *nx_video_mpegts_create(char *name, u32 type,
					       struct device *dev,
					       struct v4l2_device *v4l2_dev,
					       void *vb2_alloc_ctx)
{
	int ret;
	struct vb2_queue *vbq = NULL;
	struct nx_video_mpegts *me = kzalloc(sizeof(*me), GFP_KERNEL);

	if (!me) {
		WARN_ON(1);
		return ERR_PTR(-ENOMEM);
	}

	pr_info("%s - addr : 0x%p\n", __func__, me);

	snprintf(me->name, sizeof(me->name), "%s", name);
	snprintf(me->vdev.name, sizeof(me->vdev.name), "%s", name);

	me->type		= type;
	me->t_dev		= dev;
	me->v4l2_dev		= v4l2_dev;
	me->vb2_alloc_ctx	= vb2_alloc_ctx;

	mutex_init(&me->lock);

	me->vdev.fops		= &nx_video_mpegts_fops;
	me->vdev.ioctl_ops	= &nx_video_mpegts_ioctl_ops;
	me->vdev.v4l2_dev	= v4l2_dev;
	me->vdev.minor		= -1;
	me->vdev.vfl_type	= VFL_TYPE_GRABBER;
	me->vdev.release	= video_device_release;
	me->vdev.lock		= &me->lock;

	vbq = kzalloc(sizeof(*vbq), GFP_KERNEL);
	if (!vbq) {
		WARN_ON(1);
		ret = -ENOMEM;
		goto free_me;
	}

	me->vbq = vbq;

	ret = nx_video_mpegts_vbq_init(me, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	if (ret < 0) {
		pr_err("failed to nx_video_mpegts_vbq_init()\n");
		goto free_vbq;
	}

	ret = video_register_device(&me->vdev, VFL_TYPE_GRABBER,
				    video_device_number);
	if (ret < 0) {
		pr_err("failed to video_register_device()\n");
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

void nx_video_mpegts_cleanup(struct nx_video_mpegts *me)
{
	video_unregister_device(&me->vdev);

	mutex_destroy(&me->lock);
	if (me->vbq) {
		vb2_queue_release(me->vbq);
		kfree(me->vbq);
	}
	kfree(me);
}

static int register_v4l2(struct nx_mpegts *me)
{
	char dev_name[32] = {0, };
	struct nx_video_mpegts *video;

	pr_info("%s - channel number : %d\n", __func__, me->channel_num);

	snprintf(dev_name, sizeof(dev_name), "VIDEO MPEGTS%d", me->channel_num);
	video = nx_video_mpegts_create(dev_name, NX_VIDEO_TYPE_CAPTURE,
				&me->pdev->dev,
				nx_v4l2_get_v4l2_device(),
				nx_v4l2_get_alloc_ctx());
	if (!video)
		WARN_ON(1);

	me->vbuf_mng.video = video;

	return 0;
}

static void unregister_v4l2(struct nx_mpegts *me)
{
	if (me->vbuf_mng.video) {
		nx_video_mpegts_cleanup(me->vbuf_mng.video);
		me->vbuf_mng.video = NULL;
	}
}

static int find_action_mark(u32 *p, int length, u32 mark)
{
	int i;

	for (i = 0; i < length; i++) {
		if (p[i] == mark)
			return i;
	}
	return -1;
}

static int find_action_start(u32 *p, int length)
{
	return find_action_mark(p, length, NX_ACTION_START);
}

static int find_action_end(u32 *p, int length)
{
	return find_action_mark(p, length, NX_ACTION_END);
}

static int get_num_of_enable_action(u32 *array, int count)
{
	u32 *p = array;
	int action_num = 0;
	int next_index = 0;
	int length = count;

	while (length > 0) {
		next_index = find_action_start(p, length);
		if (next_index < 0)
			break;
		p += next_index;
		length -= next_index;
		if (length <= 0)
			break;

		next_index = find_action_end(p, length);
		if (next_index <= 0) {
			pr_err("failed to find_action_end, check power node of dts\n");
			return 0;
		}
		p += next_index;
		length -= next_index;
		action_num++;
	}

	return action_num;
}

static u32 *get_next_action_unit(u32 *array, int count)
{
	u32 *p = array;
	int next_index = find_action_start(p, count);

	if (next_index >= 0)
		return p + next_index;
	return NULL;
}

static u32 get_action_type(u32 *array)
{
	return array[1];
}

static int make_enable_gpio_action(u32 *start, u32 *end,
				   struct nx_mpegts_power_action *action)
{
	struct nx_mpegts_enable_gpio_action *gpio_action;
	struct gpio_action_unit *unit;
	int i;
	u32 *p;
	/* start_marker, type, gpio num */
	int unit_count = end - start - 1 - 1 - 1;

	if ((unit_count <= 0) || (unit_count % 2)) {
		pr_err("invalid unit_count %d of gpio action\n", unit_count);
		return -EINVAL;
	}
	unit_count /= 2;

	gpio_action = kzalloc(sizeof(*gpio_action), GFP_KERNEL);
	if (!gpio_action) {
		WARN_ON(1);
		return -ENOMEM;
	}

	gpio_action->count = unit_count;
	gpio_action->units = kcalloc(unit_count, sizeof(*unit), GFP_KERNEL);
	if (!gpio_action->units) {
		WARN_ON(1);
		kfree(gpio_action);
		return -ENOMEM;
	}

	gpio_action->gpio_num = start[2];

	p = &start[3];
	for (i = 0; i < unit_count; i++) {
		unit = &gpio_action->units[i];
		unit->value = *p;
		p++;
		unit->delay_ms = *p;
		p++;
	}

	action->type = NX_ACTION_TYPE_GPIO;
	action->action = gpio_action;

	return 0;
}

static int make_enable_action(u32 *array, int count,
			      struct nx_mpegts_power_action *action)
{
	u32 *p = array;
	int end_index = find_action_end(p, count);

	if (end_index <= 0) {
		pr_err("parse dt for v4l2 mpegts error: can't find action end\n");
		return -EINVAL;
	}

	switch (get_action_type(p)) {
	case NX_ACTION_TYPE_GPIO:
		return make_enable_gpio_action(p, p + end_index, action);
	default:
		pr_err("parse dt v4l2 mpegts: invalid type 0x%x\n",
		       get_action_type(p));
		return -EINVAL;
	}
}

static void free_enable_seq_actions(struct nx_mpegts_power_seq *seq)
{
	int i;
	struct nx_mpegts_power_action *action;

	for (i = 0; i < seq->count; i++) {
		action = &seq->actions[i];
		if (action->action) {
			if (action->type == NX_ACTION_TYPE_GPIO) {
				struct nx_mpegts_enable_gpio_action *
					gpio_action = action->action;
				kfree(gpio_action->units);
			}
			kfree(action->action);
		}
	}

	kfree(seq->actions);
	seq->count = 0;
	seq->actions = NULL;
}




static int apply_gpio_action(struct device *dev, int gpio_num,
		  struct gpio_action_unit *unit)
{
	int ret;
	char label[64] = {0, };
	struct device_node *np;
	int gpio;

	np = dev->of_node;
	gpio = of_get_named_gpio(np, "gpios", gpio_num);

	sprintf(label, "v4l2-mpegts #pwr gpio %d", gpio);
	if (!gpio_is_valid(gpio)) {
		dev_err(dev, "invalid gpio %d set to %d\n", gpio, unit->value);
		return -EINVAL;
	}

	ret = devm_gpio_request_one(dev, gpio,
				    unit->value ?
				    GPIOF_OUT_INIT_HIGH : GPIOF_OUT_INIT_LOW,
				    label);
	if (ret < 0) {
		dev_err(dev, "failed to gpio %d set to %d\n",
			gpio, unit->value);
		return ret;
	}

	if (unit->delay_ms > 0)
		mdelay(unit->delay_ms);

	devm_gpio_free(dev, gpio);

	return 0;
}

static int do_gpio_action(struct device *dev,
				 struct nx_mpegts_enable_gpio_action *action)
{
	int ret;
	struct gpio_action_unit *unit;
	int i;

	for (i = 0; i < action->count; i++) {
		unit = &action->units[i];
		ret = apply_gpio_action(dev, action->gpio_num, unit);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int enable_sensor_power(struct nx_mpegts *me)
{
	struct nx_mpegts_power_seq *seq = NULL;

	seq = &me->enable_seq;

	if (seq) {
		int i;
		struct nx_mpegts_power_action *action;

		for (i = 0; i < seq->count; i++) {
			action = &seq->actions[i];
			switch (action->type) {
			case NX_ACTION_TYPE_GPIO:
				do_gpio_action(&me->pdev->dev,
					       action->action);
				break;
			default:
				pr_warn("unknown action type %d\n",
					action->type);
				break;
			}
		}
	}

	return 0;
}

static int parse_mpegts_power_seq(struct device_node *np,
				   char *node_name,
				   struct nx_mpegts_power_seq *seq)
{
	int count = of_property_count_elems_of_size(np, node_name, 4);

	if (count > 0) {
		u32 *p;
		int ret = 0;
		struct nx_mpegts_power_action *action;
		int action_nums;
		u32 *array = kcalloc(count, sizeof(u32), GFP_KERNEL);

		if (!array) {
			WARN_ON(1);
			return -ENOMEM;
		}

		of_property_read_u32_array(np, node_name, array, count);
		action_nums = get_num_of_enable_action(array, count);
		if (action_nums <= 0) {
			pr_err("parse_dt v4l2 capture: no actions in %s\n",
			       node_name);
			return -ENOENT;
		}

		seq->actions = kcalloc(count, sizeof(*action), GFP_KERNEL);
		if (!seq->actions) {
			WARN_ON(1);
			return -ENOMEM;
		}
		seq->count = action_nums;

		p = array;
		action = seq->actions;
		while (action_nums--) {
			p = get_next_action_unit(p, count - (p - array));
			if (!p) {
				pr_err("failed to get_next_action_unit(%d/%d)\n",
				       seq->count, action_nums);
				free_enable_seq_actions(seq);
				return -EINVAL;
			}

			ret = make_enable_action(p, count - (p - array),
						 action);
			if (ret != 0) {
				free_enable_seq_actions(seq);
				return ret;
			}

			p++;
			action++;
		}
	}

	return 0;
}

static int parse_power_dt(struct device_node *np, struct nx_mpegts *me)
{
	int ret = 0;

	if (of_find_property(np, "enable_seq", NULL))
		ret = parse_mpegts_power_seq(np, "enable_seq",
					      &me->enable_seq);
	return ret;
}

static int nx_mpegts_parse_dt(struct device *dev, struct nx_mpegts *me)
{
	int ret;
	struct resource res;
	struct device_node *np = dev->of_node;
	struct device_node *child_node = NULL;
	char clk_name[20] = {0, };
	char rst_name[20] = {0, };

	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		dev_err(dev, "failed to mpegts get base addr\n");
		return -ENXIO;
	}

	me->res.base = devm_ioremap_nocache(dev, res.start,
					    resource_size(&res));
	if (!me->res.base) {
		dev_err(dev, "failed to mpegts base addr ioremap\n");
		return -EBUSY;
	}

	if (of_property_read_u32(np, "channel_num",
				 &me->channel_num)) {
		dev_err(dev, "failed to get dt channel\n");
		return -EINVAL;
	}

	sprintf(clk_name, "mpegts%d-clk", me->channel_num);
	me->clk = devm_clk_get(&me->pdev->dev, clk_name);
	if (IS_ERR(me->clk)) {
		dev_err(dev, "failed to devm_clk_get\n");
		return -ENODEV;
	}

	sprintf(rst_name, "mpegts%d-rst", me->channel_num);
	me->rst = devm_reset_control_get(&me->pdev->dev, rst_name);
	if (IS_ERR(me->rst)) {
		dev_err(&me->pdev->dev, "failed to get reset control\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "clock_pol",
				 &me->config_desc.clock_pol)) {
		dev_err(dev, "failed to get dt clock pol\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "valid_pol",
				 &me->config_desc.valid_pol)) {
		dev_err(dev, "failed to get dt valid pol\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "sync_pol",
				 &me->config_desc.sync_pol)) {
		dev_err(dev, "failed to get dt syncpol\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "err_pol",
				 &me->config_desc.err_pol)) {
		dev_err(dev, "failed to get dt err pol\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "serial_mode",
				 &me->config_desc.serial_mode)) {
		dev_err(dev, "failed to get dt serial mode\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "bypass",
				 &me->config_desc.bypass)) {
		dev_err(dev, "failed to get dt bypass\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "xfer_mode",
				 &me->config_desc.xfer_mode)) {
		dev_err(dev, "failed to get dt xfer mode\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "xfer_clk_pol",
				 &me->config_desc.xfer_clk_pol)) {
		dev_err(dev, "failed to get dt xfer_clk_pol\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "encrypt_on",
				 &me->config_desc.encrypt_on)) {
		dev_err(dev, "failed to get dt encrypt on\n");
		return -EINVAL;
	}

	child_node = of_find_node_by_name(np, "power");
	if (!child_node) {
		dev_err(dev, "failed to get power node\n");
		return -EINVAL;
	}

	ret = parse_power_dt(child_node, me);
	if (ret)
		return ret;

	return 0;
}

static int nx_mpegts_probe(struct platform_device *pdev)
{
	int ret;
	struct nx_mpegts *me;
	struct device *dev = &pdev->dev;

	pr_info("+++ %s +++\n", __func__);

	me = devm_kzalloc(dev, sizeof(*me), GFP_KERNEL);
	if (!me) {
		WARN_ON(1);
		return -ENOMEM;
	}
	me->pdev = pdev;
	platform_set_drvdata(pdev, me);

	ret = nx_mpegts_parse_dt(dev, me);
	if (ret < 0) {
		dev_err(dev, "failed to parse dt\n");
		return ret;
	}

	pr_info("%s - channel number 1: %d\n", __func__, me->channel_num);

	init_me(me);
	_init_device(me);

	ret = register_v4l2(me);
	if (ret)
		return ret;

#if TIMER_IRQ
	setup_timer(&me->timer_irq, timer_handler, (long)me);
#endif

	pr_info("%s - channel number 2: %d\n", __func__, me->channel_num);

	pr_info("--- %s ---\n", __func__);

	return 0;
}

static int nx_mpegts_remove(struct platform_device *pdev)
{
	struct nx_mpegts *me = platform_get_drvdata(pdev);

	if (unlikely(!me))
		return 0;

	me->ch_info.is_running = 0;
	ts_stop(me);
	deinit_me(me);
	_deinit_dma(me);

	unregister_v4l2(me);

	return 0;
}

static struct platform_device_id nx_mpegts_id_table[] = {
	{ NX_MPEGTS_DEV_NAME, 0 },
	{},
};

static const struct of_device_id nx_mpegts_dt_match[] = {
	{ .compatible = "nexell,nx-mpegts" },
	{},
};
MODULE_DEVICE_TABLE(of, nx_mpegts_dt_match);

static struct platform_driver nx_mpegts_driver = {
	.probe		= nx_mpegts_probe,
	.remove		= nx_mpegts_remove,
	.id_table	= nx_mpegts_id_table,
	.driver		= {
		.name	= NX_MPEGTS_DEV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table	= of_match_ptr(nx_mpegts_dt_match),
	},
};

module_platform_driver(nx_mpegts_driver);

MODULE_AUTHOR("jkchoi <jkchoi@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell S5Pxx18 series SoC V4L2 mpeg-ts driver");
MODULE_LICENSE("GPL");
