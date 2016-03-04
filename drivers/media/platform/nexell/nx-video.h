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

#ifndef _NX_VIDEO_H
#define _NX_VIDEO_H

#define NX_VIDEO_MAX_NAME_SIZE		32
#define NX_VIDEO_MAX_PLANES		3
#define NX_VIDEO_MAX_PADS		2
#define NX_VIDEO_MAX_BUFFERS		16

#include <linux/spinlock.h>

struct nx_video;

struct nx_video_format {
	char *name;
	u32   pixelformat;
	u32   mbus_code;
	u32   num_planes;
	u32   num_sw_planes;
	bool  is_separated;
};

struct nx_video_frame {
	u16 width;
	u16 height;
	u16 stride[NX_VIDEO_MAX_PLANES];
	u32 size[NX_VIDEO_MAX_PLANES];
	struct nx_video_format format;
};

struct nx_video_buffer;
typedef int (*nx_video_buf_done)(struct nx_video_buffer *);

struct nx_video_buffer {
	struct list_head list;
	int consumer_index; /* consumer increment this field after consuming */
	dma_addr_t dma_addr[NX_VIDEO_MAX_PLANES];
	u32 stride[NX_VIDEO_MAX_PLANES];
	void *priv;   /* struct vb2_buffer */
	nx_video_buf_done cb_buf_done;
};

typedef int (*nx_queue_func)(struct nx_video_buffer *, void *);
struct nx_buffer_consumer {
	struct list_head list;
	int index;
	ulong timeout;
	void *priv; /* consumer private data */
	nx_queue_func queue;
	u32 usage_count;
};

struct nx_video_buffer_object {
	struct nx_video *video;
	struct list_head buffer_list;
	spinlock_t slock;
	struct nx_video_buffer *cur_buf;
	u32 buffer_count;
	struct nx_buffer_consumer *consumer;
};

/* video device type : exclusive */
enum nx_video_type {
	NX_VIDEO_TYPE_CAPTURE = 0,
	NX_VIDEO_TYPE_OUT,
	NX_VIDEO_TYPE_M2M,
	NX_VIDEO_TYPE_MAX,
};

enum nx_buffer_consumer_type {
	/* vq type: V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE */
	NX_BUFFER_CONSUMER_SINK = 0,
	/* vq type: V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE */
	NX_BUFFER_CONSUMER_SOURCE,
	NX_BUFFER_CONSUMER_INVALID
};

struct nx_video {
	char name[NX_VIDEO_MAX_NAME_SIZE];
	u32 type; /* enum nx_video_type */

	struct nx_video_buffer *sink_bufs[NX_VIDEO_MAX_BUFFERS];
	struct nx_video_buffer *source_bufs[NX_VIDEO_MAX_BUFFERS];

	struct v4l2_device *v4l2_dev;
	struct vb2_queue *vbq;
	void *vb2_alloc_ctx;

	struct mutex lock; /* for video_device */
	struct video_device vdev;
	/**
	 * pad 0 : sink
	 * pad 1 : source
	 */
	struct media_pad pads[NX_VIDEO_MAX_PADS];

	/* frame[0] : sink, capture
	   frame[1] : source, out */
	struct nx_video_frame frame[2];

	/* buffer consumer */
	int (*register_buffer_consumer)(struct nx_video *,
					struct nx_buffer_consumer *,
					enum nx_buffer_consumer_type);
	void (*unregister_buffer_consumer)(struct nx_video *,
					   struct nx_buffer_consumer *,
					   enum nx_buffer_consumer_type);

	/* lock for consumer list */
	spinlock_t lock_consumer;
	/* I'm source */
	struct list_head source_consumer_list;
	int source_consumer_count;
	/* I'm sink */
	struct list_head sink_consumer_list;
	int sink_consumer_count;

	uint32_t open_count;

};

/* macros */
#define vdev_to_nx_video(vdev) container_of(vdev, struct nx_video, video)
#define vbq_to_nx_video(vbq)   container_of(vbq, struct nx_video, vbq)

/* public functions */
struct nx_video *nx_video_create(char *, u32 type, struct v4l2_device *,
				 void *);
void nx_video_cleanup(struct nx_video *);
int nx_video_update_buffer(struct nx_video_buffer_object *obj);
void nx_video_done_buffer(struct nx_video_buffer_object *obj);
void nx_video_clear_buffer(struct nx_video_buffer_object *obj);
void nx_video_init_vbuf_obj(struct nx_video_buffer_object *obj);
void nx_video_add_buffer(struct nx_video_buffer_object *obj,
			 struct nx_video_buffer *buf);
int nx_video_register_buffer_consumer(struct nx_video_buffer_object *obj,
				      nx_queue_func func,
				      void *data);
void nx_video_unregister_buffer_consumer(struct nx_video_buffer_object *obj);

#endif
