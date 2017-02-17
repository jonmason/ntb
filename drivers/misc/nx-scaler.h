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
#ifndef _NX_SCALER_H
#define _NX_SCALER_H

#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>

struct list_head;
struct nx_scaler_ioctl_data;

struct nx_scaler {
	struct miscdevice		miscdev;
	struct platform_device		*pdev;

	void __iomem			*base;
	struct clk			*clk;
	int				irq;

	unsigned int			*command_buffer_vir;
	dma_addr_t			command_buffer_phy;

	atomic_t			open_count;
	wait_queue_head_t		wq_end;
	struct nx_scaler_ioctl_data	*ioctl_data;
	atomic_t			running;

	struct mutex mutex;
};
#endif
