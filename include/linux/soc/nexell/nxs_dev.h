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
#ifndef _NXS_DEV_H
#define _NXS_DEV_H

#include <linux/atomic.h>
#include <linux/spinlock.h>

#include "nxs_display.h"

#define NXS_DEV_MAX_PLANES	2
#define NXS_DEV_MAX_FIELDS	4

enum nxs_event_type {
	NXS_EVENT_NONE		= 0,
	NXS_EVENT_IDLE		= 1<<0,
	NXS_EVENT_DONE		= 1<<1,
	NXS_EVENT_CHANGED	= 1<<2,
	NXS_EVENT_ERR		= 1<<3,
	NXS_EVENT_ALL		= (NXS_EVENT_IDLE |
				   NXS_EVENT_DONE |
				   NXS_EVENT_CHANGED |
				   NXS_EVENT_ERR),
	NXS_EVENT_MAX		= 1<<4,
};

enum nxs_control_type {
	NXS_CONTROL_NONE	= 0,
	NXS_CONTROL_FORMAT, /* width, height, pixelformat */
	NXS_CONTROL_DST_FORMAT, /* m2m: width, height, pixelformat */
	NXS_CONTROL_CROP, /* input source cropping */
	NXS_CONTROL_SELECTION, /* output target position */
	NXS_CONTROL_TPF, /* timeperframe */
	NXS_CONTROL_SYNCINFO,
	NXS_CONTROL_BUFFER,
	NXS_CONTROL_GAMMA,
	NXS_CONTROL_STATUS, /* device specific status ex> hdmi connected? */
	NXS_CONTROL_VIDEO,
	NXS_CONTROL_MAX
};

enum nxs_video_type {
	NXS_VIDEO_TYPE_NONE = 0,
	NXS_VIDEO_TYPE_CAPTURE,
	NXS_VIDEO_TYPE_RENDER,
	NXS_VIDEO_TYPE_M2M,
	NXS_VIDEO_TYPE_SUBDEV,
	NXS_VIDEO_TYPE_INVALID
};

enum nxs_field_type {
	NXS_FIELD_PROGRESSIVE = 0,
	NXS_FIELD_INTERLACED = 1,
	NXS_FIELD_3D = 3,
};

struct nxs_control_format {
	u32 width;
	u32 height;
	u32 pixelformat;
	u32 field;
};

struct nxs_control_video {
	u32 type;
	u32 field;
};

struct nxs_control_crop {
	u32 l;
	u32 t;
	u32 w;
	u32 h;
};

struct nxs_control_selection {
	u32 l;
	u32 t;
	u32 w;
	u32 h;
};

struct nxs_control_tpf {
	u32 numerator;
	u32 denominator;
};

struct nxs_control_syncinfo {
	u32 width;
	u32 height;
	u32 vfrontporch;
	u32 vbackporch;
	u32 vsyncwidth;
	u32 vsyncpol;
	u32 hfrontporch;
	u32 hbackporch;
	u32 hsyncwidth;
	u32 hsyncpol;
	u32 pixelclock;
	u32 fps;
	u32 interlaced;
	u32 flags;	/* private flags EX> hdmi DVI mode */
};

struct nxs_control_buffer {
	u32 num_planes;
	u32 address[NXS_DEV_MAX_PLANES];
	u32 strides[NXS_DEV_MAX_PLANES];
};

struct nxs_control {
	u32 type;
	union {
		struct nxs_control_format format;
		struct nxs_control_crop crop;
		struct nxs_control_selection sel;
		struct nxs_control_tpf tpf;
		struct nxs_control_syncinfo sync_info;
		struct nxs_control_buffer buffer;
		struct nxs_control_display display;
		struct nxs_control_video video;
		/* struct nxs_control_tpgen tpgen; */
		u32 gamma;
		u32 status;
	} u;
};

struct nxs_dev;
struct nxs_dev_service {
	enum nxs_control_type type;
	int (*set_control)(const struct nxs_dev *pthis,
			   const struct nxs_control *pparam);
	int (*get_control)(const struct nxs_dev *pthis,
			   struct nxs_control *pparam);
};

enum {
	NXS_DEV_IRQCALLBACK_TYPE_NONE,
	NXS_DEV_IRQCALLBACK_TYPE_IRQ = 1,
	NXS_DEV_IRQCALLBACK_TYPE_BOTTOM_HALF, /* currently not used */
	NXS_DEV_IRQCALLBACK_TYPE_INVALID
};

enum {
	NXS_DEV_DIRTY_NORMAL = 1,
	NXS_DEV_DIRTY_TID,
};

struct nxs_irq_callback {
	struct list_head list;
	void (*handler)(struct nxs_dev *, void *);
	u32 type;
	void *data;
};

struct reset_control;
struct clk;
struct device;
struct list_head;

#define NXS_MAX_SERVICES NXS_CONTROL_MAX /*8*/

struct nxs_dev {
	struct list_head list; /* connected to nxs_res_manager dev_list */
	struct list_head func_list; /* connected to nxs_function->dev_list */
	struct list_head disp_list; /* connected to nxs_display->vert_list */
	/* for multitap */
	struct list_head sibling_list;

	/* same input other output */
	struct nxs_dev *sibling;

	u32 user;
	/* for multitap usage */
	u32 can_multitap_follow;
	bool multitap_connected;
	atomic_t connect_count;

	u32 dev_ver;
	u32 dev_function;
	u32 dev_inst_index;
	u16 tid;
	u16 tid2; /* for blender */

	atomic_t refcount;
	u32 max_refcount;

	atomic_t open_count;

	void *priv;

	int irq;
	struct reset_control *resets;
	int reset_num;

	struct clk *clks;
	int clk_num;

	struct device *dev;

	spinlock_t irq_lock;
	struct list_head irq_callback;

	void (*set_interrupt_enable)(const struct nxs_dev *pthis,
				    enum nxs_event_type type,
				     bool enable);
	u32  (*get_interrupt_enable)(const struct nxs_dev *pthis,
				     enum nxs_event_type type);
	u32  (*get_interrupt_pending)(const struct nxs_dev *pthis,
				      enum nxs_event_type type);
	void (*clear_interrupt_pending)(const struct nxs_dev *pthis,
					enum nxs_event_type type);

	int (*open)(const struct nxs_dev *pthis);
	int (*close)(const struct nxs_dev *pthis);
	int (*start)(const struct nxs_dev *pthis);
	int (*stop)(const struct nxs_dev *pthis);
	int (*set_control)(const struct nxs_dev *pthis, int type,
			   const struct nxs_control *pparam);
	int (*get_control)(const struct nxs_dev *pthis, int type,
			   struct nxs_control *pparam);
	int (*set_dirty)(const struct nxs_dev *pthis, u32 type);
	int (*set_tid)(const struct nxs_dev *pthis, u32 tid1, u32 tid2);
	void (*dump_register)(const struct nxs_dev *pthis);

	struct nxs_dev_service dev_services[NXS_MAX_SERVICES];
};

int nxs_set_control(const struct nxs_dev *pthis, int type,
		    const struct nxs_control *pparam);
int nxs_get_control(const struct nxs_dev *pthis, int type,
		    struct nxs_control *pparam);

struct platform_device;
int nxs_dev_parse_dt(struct platform_device *pdev, struct nxs_dev *pthis);
int nxs_dev_register_irq_callback(struct nxs_dev *pthis, u32 type,
				  struct nxs_irq_callback *callback);
int nxs_dev_unregister_irq_callback(struct nxs_dev *pthis, u32 type,
				    struct nxs_irq_callback *callback);
void nxs_dev_print(const struct nxs_dev *pthis, char *prefix);
void nxs_dev_dump_register(struct nxs_dev *nxs_dev,
		       struct regmap *reg,
		       u32 offset, u32 size);
u32 nxs_dev_get_open_count(const struct nxs_dev *pthis);
void nxs_dev_dec_open_count(const struct nxs_dev *pthis);
void nxs_dev_inc_open_count(const struct nxs_dev *pthis);
#endif
