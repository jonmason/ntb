/*
 * Copyright (C) 2017  Nexell Co., Ltd.
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

#ifndef __NX_MPEGTS_H__
#define __NX_MPEGTS_H__

#define NX_PARAM_TYPE_PID	0
#define NX_PARAM_TYPE_CAS	1

#define NX_NO_PID		(0x1FFF)

#define TS_CAPx_PID_MAX		(64)/* Unit : 2byte, 128 field */
#define TS_PACKET_NUM		(80)/* Maximum 80 */

#define NX_VIDEO_MPEGTS_MAX_NAME_SIZE	20
#define	NX_VIDEO_MPEGTS_MAX_BUFFERS	VB2_MAX_FRAME

#define NX_ACTION_START				0x12345678
#define NX_ACTION_END				0x87654321
#define NX_ACTION_TYPE_GPIO			0xffff0001

#define DMA_PERIPHERAL_NAME_MPEGTSI0	"mpegtsi0"
#define DMA_PERIPHERAL_NAME_MPEGTSI1	"mpegtsi1"

struct nx_video_mpegts_buffer;
typedef int (*nx_video_mpegts_buf_done)(struct nx_video_mpegts_buffer *);

struct nx_video_mpegts_buffer {
	struct list_head list;
	dma_addr_t dma_phy_addr;
	void *dma_virt_addr;
	unsigned int index;

	void *priv;
	nx_video_mpegts_buf_done cb_buf_done;
};

struct nx_video_mpegts_buffer_manager {
	struct nx_video_mpegts *video;
	struct list_head buffer_list;

	int req_buffer_count;
	atomic_t buffer_count;
	spinlock_t slock;
};

struct nx_video_mpegts {
	char name[NX_VIDEO_MPEGTS_MAX_NAME_SIZE];
	u32 type;

	struct nx_video_mpegts_buffer *sink_bufs[NX_VIDEO_MPEGTS_MAX_BUFFERS];

	struct device *t_dev;
	struct v4l2_device *v4l2_dev;
	struct vb2_queue *vbq;
	void *vb2_alloc_ctx;

	struct video_device vdev;

	struct nx_video_mpegts_buffer vbuf;
	uint32_t open_count;

	struct mutex lock;

	struct nx_video_format format;
	dma_addr_t cur_dma_phy_addr;
	void *cur_dma_virt_addr;

	unsigned int page_size;
};

struct nx_mpegts;
struct nx_dev_mpegts_ops {
	int (*s_stream)(struct nx_mpegts *, int);
};

struct ts_op_mode {
	unsigned char ch_num;
	unsigned char tx_mode;
};

struct ts_config_desc {
	unsigned int clock_pol;
	unsigned int valid_pol;
	unsigned int sync_pol;
	unsigned int err_pol;
	unsigned int serial_mode;
	unsigned int bypass;
	unsigned int xfer_mode;
	unsigned int xfer_clk_pol;
	unsigned int encrypt_on;
};

struct ts_param_info {
	unsigned int index;
	unsigned int type;
};

struct ts_param_desc {
	struct ts_param_info info;
	void *buf;
	int buf_size;
	int wait_time;
	int read_count;
	int ret_value;
};

struct ts_buf_init_info {
	int packet_size;
	int packet_num;
	int page_size;
	int page_num;
};

struct ts_ring_buf {
	unsigned int captured_addr;
	unsigned int cnt;
	unsigned int wPos;
	unsigned int rPos;
	unsigned char *ts_packet[TS_PACKET_NUM];
	unsigned char *ts_phy_packet[TS_PACKET_NUM];
};

struct ts_channel_info {
	struct dma_chan	*dma_chan;
	struct dma_async_tx_descriptor *desc;
	void		*filter_data;
	dma_addr_t	peri_addr;

	int is_running;
	int is_first;
	int do_continue;

	int wait_time;

	unsigned char *buf;
	unsigned char tx_mode;
	void *dma_virt[VB2_MAX_FRAME];
	dma_addr_t dma_phy[VB2_MAX_FRAME];

	unsigned int alloc_size;

	int cnt;
	int w_pos;
	int r_pos;
	int page_size;
	int page_num;

	wait_queue_head_t wait;
};

static int nx_video_mpegts_vbq_init(struct nx_video_mpegts *, uint32_t);
static int enable_sensor_power(struct nx_mpegts *);
#endif
