/*********************************************************
 * Copyright (C) 2011 - 2015 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/completion.h>
#include <linux/sysfs.h>
#include <linux/syscore_ops.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/semaphore.h>

#include "ssdev_rpmb.h"
#include "ssdev_core.h"
#include "ssdev_init.h"
#include "tzlog_print.h"

#ifndef CONFIG_SECOS_NO_SECURE_STORAGE

#define SS_MAJOR_VERSION  "005"
#define SS_MINOR_VERSION  "0"

#define SSDEV_NOTIFY_TARGET_ID (0x1001)

struct semaphore ssdev_sem;

#ifdef SSDEV_MEASURE_TIME

#define SS_CHECK_TIME_DECLARE() \
	struct timeval before, after

#define SS_CHECK_TIME_BEGIN()   \
	do_gettimeofday(&before)

#define SS_CHECK_TIME_END()		\
	do {						\
	do_gettimeofday(&after);\
	tzlog_print(TZLOG_DEBUG, "\ndiff: %ld(us)\n", \
			((after.tv_sec-before.tv_sec)*1000*1000 \
			 + after.tv_usec-before.tv_usec));		\
			before = after;	\
	} while (0);
#else
#define SS_CHECK_TIME_DECLARE()
#define SS_CHECK_TIME_BEGIN()
#define SS_CHECK_TIME_END()
#endif /* SSDEV_MEASURE_TIME */

static long storage_ioctl(struct file *file, unsigned int cmd,
			  unsigned long arg)
{
	return 0;
}

static int storage_mmap(struct file *file, struct vm_area_struct *vma)
{
	return -EINVAL;
}

static int storage_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int storage_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations storage_fops = {
	.owner = THIS_MODULE,
	.open = storage_open,
	.release = storage_release,
	.mmap = storage_mmap,
	.unlocked_ioctl = storage_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = storage_ioctl,
#endif
};

static struct miscdevice storage = {
	.name = "ssdev",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &storage_fops,
};

int __init init_storage(void)
{
	int rc = 0;

	rc = misc_register(&storage);
	if (unlikely(rc))
		return rc;

	rc = storage_register_wsm();
	if (rc)
		panic("cannot create world share memory\n");

	sema_init(&ssdev_sem, 0);

	tzlog_print(TZLOG_INFO, "storage driver version [%s.%s]\n",
			SS_MAJOR_VERSION, SS_MINOR_VERSION);

	return 0;
}

#endif
