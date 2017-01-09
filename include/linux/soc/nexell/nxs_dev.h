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

#define NXS_DEV_MAX_PLANES	3

enum nxs_event_type {
	NXS_EVENT_IDLE		= 1,
	NXS_EVENT_UPDATE	= 2,
	NXS_EVENT_DONE		= 3,
	NXS_EVENT_MAX
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
	/* NXS_CONTROL_TPGEN	= NXS_SET_TPGEN, */
	NXS_CONTROL_MAX
};

struct nxs_control_format {
	u32 width;
	u32 height;
	u32 pixelformat;
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
	u32 hfrontporch;
	u32 hbackporch;
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
		/* struct nxs_control_tpgen tpgen; */
		u32 gamma;
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
	void *data;
};

struct reset_control;
struct clk;
struct device;
struct list_head;

#define NXS_MAX_SERVICES 8

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

	void *priv;

	int irq;
	struct reset_control *resets;
	int reset_num;

	struct clk *clks;
	int clk_num;

	struct device *dev;

	spinlock_t irq_lock;
	struct list_head irq_callback;

	void (*set_interrupt_enable)(const struct nxs_dev *pthis, int type,
				     bool enable);
	u32  (*get_interrupt_enable)(const struct nxs_dev *pthis, int type);
	u32  (*get_interrupt_pending)(const struct nxs_dev *pthis, int type);
	void (*clear_interrupt_pending)(const struct nxs_dev *pthis, int type);

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

#endif
