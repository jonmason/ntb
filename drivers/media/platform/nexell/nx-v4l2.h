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

#ifndef _NX_V4L2_H
#define _NX_V4L2_H

#include <linux/atomic.h>
#include <linux/irqreturn.h>

#include <media/media-device.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

/**
 * structure for irq sharing
 */
struct nx_v4l2_irq_entry {
	struct list_head entry;
	u32 irqs;
	void *priv;
	irqreturn_t (*handler)(void *);
};

/* macro functions for atomic operations */
#ifdef CONFIG_ARM64
#define NX_ATOMIC_SET(V, I) atomic_set(V, I)
#define NX_ATOMIC_SET_MASK(MASK, PTR)  \
	do { \
		int oldval = atomic_read(PTR); \
		int newval = oldval | MASK; \
		atomic_cmpxchg(PTR, oldval, newval); \
	} while (0)
#define NX_ATOMIC_CLEAR_MASK(MASK, PTR) \
	do { \
		int oldval = atomic_read(PTR); \
		int newval = oldval & (~MASK); \
		atomic_cmpxchg(PTR, oldval, newval); \
	} while (0)
#define NX_ATOMIC_READ(PTR)    atomic_read(PTR)
#define NX_ATOMIC_INC(PTR)     atomic_inc(PTR)
#define NX_ATOMIC_DEC(PTR)     atomic_dec(PTR)
#else
#define NX_ATOMIC_SET(V, I) atomic_set(V, I)
#define NX_ATOMIC_SET_MASK(MASK, PTR)  \
	do { \
		unsigned long oldval = atomic_read(PTR); \
		unsigned long newval = oldval | MASK; \
		atomic_cmpxchg(PTR, oldval, newval); \
	} while (0)
#define NX_ATOMIC_CLEAR_MASK(MASK, PTR) \
	atomic_clear_mask(MASK, (unsigned long *)&((PTR)->counter))
#define NX_ATOMIC_READ(PTR)    atomic_read(PTR)
#define NX_ATOMIC_INC(PTR)     atomic_inc(PTR)
#define NX_ATOMIC_DEC(PTR)     atomic_dec(PTR)
#endif

struct media_device *nx_v4l2_get_media_device(void);
struct v4l2_device  *nx_v4l2_get_v4l2_device(void);
void *nx_v4l2_get_alloc_ctx(void);
int nx_v4l2_register_subdev(struct v4l2_subdev *sd);

#endif
