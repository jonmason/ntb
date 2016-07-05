/*********************************************************
 * Copyright (C) 2011 - 2016 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 *********************************************************/

#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/semaphore.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/namei.h>
#include <linux/dcache.h>
#include <linux/path.h>

#ifndef __TZLOG_CORE_H__
#define __TZLOG_CORE_H__

typedef struct tzlog_data {
	int log_wsm_id;
	struct chimera_ring_buffer *ring;
	struct semaphore sem;
	struct task_struct *task;
	struct mutex lock;
	struct page *log_page;
} s_tzlog_data;

void tzlog_notify(void);
void tzlog_init(void);

#endif
