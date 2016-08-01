/*********************************************************
 * Copyright (C) 2011 - 2015 Samsung Electronics Co., Ltd All Rights Reserved
 * Author : Jongtak Lee <jong-tak.lee@samsung.com>
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
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
#include <linux/semaphore.h>

#include "tzdev.h"
#include "tzdev_internal.h"
#include "tzlog_print.h"
#include "log_level_ree.h"
#include "tzdev_smc.h"
#include "tzrsrc_msg.h"

#define TZRSRC_WSM_SIZE			( 0x80000 )

static DEFINE_SEMAPHORE(tzperf_sem);
static int open_count = 0;

static int wsmid = -1;
char *tzrsrc_wsm;


static void tzrsrc_register_wsm(void)
{
	tzrsrc_wsm = vmalloc(TZRSRC_WSM_SIZE);
	BUG_ON(!tzrsrc_wsm);
	memset(tzrsrc_wsm, 0, TZRSRC_WSM_SIZE);

	wsmid = tzwsm_register_kernel_memory(tzrsrc_wsm, TZRSRC_WSM_SIZE, GFP_KERNEL);

	BUG_ON(wsmid < 0);
}

static void tzrsrc_unregister_wsm(void)
{
	if(tzrsrc_wsm)
	{
		vfree(tzrsrc_wsm);
	}
}

int resource_monitor_cmd(struct rsrc_msg *__user rmsg)
{
	int ret = -1;
	struct rsrc_msg msg;

	if(copy_from_user(&msg, rmsg, sizeof(struct rsrc_msg)))
	{
		tzlog_print(TZLOG_ERROR, "copy_from_user error\n");
		return -EFAULT;
	}

	if(msg.cmd >= RSRC_LAST_CMD)
	{
		tzlog_print(TZLOG_ERROR, "Wrong command(0x%x) requested.\n", msg.cmd);
		return -EFAULT;
	}

	if(msg.cmd== RSRC_REGISTER_WSM)
		msg.arg0 = wsmid;

	ret = scm_resource_monitor_cmd(msg.cmd, msg.arg0);

	if((msg.cmd== RSRC_REQUEST_MONINFO) && msg.arg1)
	{
		if(copy_to_user((char __user *)msg.arg1, tzrsrc_wsm, TZRSRC_WSM_SIZE))
		{
			tzlog_print(TZLOG_ERROR, "copy_to_user error\n");
			return -EFAULT;
		}
	}

	return ret;

}

static int tzrsrc_open(struct inode *inode, struct file *filp)
{
	down(&tzperf_sem);
	if(open_count > 1)
	{
		up(&tzperf_sem);
		return -EBUSY;
	}
	else
	{
		open_count++;
		tzrsrc_register_wsm();
		up(&tzperf_sem);
		return 0;
	}
}

static long tzrsrc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	switch (cmd) {
		case TZIO_RSRC_CMD:
		{
			struct rsrc_msg __user * msg;
			msg = (struct rsrc_msg __user *)arg;

			ret = resource_monitor_cmd(msg);

			break;
		}
		default:
		{
			tzlog_print(TZLOG_ERROR, "Unknown TZRSRC Command : %d\n", cmd);
			ret = -EINVAL;
		}
	}
	return ret;
}

static int tzrsrc_release(struct inode *inode, struct file *filp)
{
	down(&tzperf_sem);
	open_count--;
	tzrsrc_unregister_wsm();
	up(&tzperf_sem);

	return 0;
}

static const struct file_operations tzrsrc_fops = {
	.owner = THIS_MODULE,
	.open = tzrsrc_open,
	.release = tzrsrc_release,
	.unlocked_ioctl = tzrsrc_ioctl,
};

struct miscdevice tzrsrc = {
	.name = "tzrsrc",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &tzrsrc_fops,
	.mode = 0600,
};
