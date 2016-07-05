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

#ifndef __TZDEV_H__
#define __TZDEV_H__

#ifdef __KERNEL__
#include <linux/version.h>		/* for linux kernel version information */
#include <linux/types.h>
#include <linux/cpu.h>
#else
#include <linux/types.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#endif /* __KERNEL__ */

#define TZ_IOC_MAGIC		'c'

#define TZIO_SMC		_IOWR(TZ_IOC_MAGIC, 102, struct tzio_message)
#define TZIO_DBG_START		_IOR(TZ_IOC_MAGIC, 113, unsigned long)

#define TZMEM_EXPORT_MEMORY	_IOWR(TZ_IOC_MAGIC, 122, struct tzmem_region)
#define TZMEM_RELEASE_MEMORY	_IOWR(TZ_IOC_MAGIC, 123, int)
#define TZMEM_CHECK_MEMORY	_IOR(TZ_IOC_MAGIC, 124, struct tzmem_region)

struct tzio_message {
	__u32		type;
	__u32		endpoint;
	__u32		length;
	__s32		context_id;
	__u32		timeout_seconds;
	__u32		boost_flag;
	__u8		payload[];
};

struct tzmem_region {
	__s32		pid;	/* Memory region owner's PID (in) */
	const void  *ptr;	/* Memory region start (in) */
	__s32		size;	/* Memory region size (in) */
	__s32		id;	/* Memory region ID (out) */
	__u32		tee_ctx_id;	/* (in) */
	__s32		writable;
};

#ifdef CONFIG_COMPAT

#define TZMEM_COMPAT_EXPORT_MEMORY \
	_IOWR(TZ_IOC_MAGIC, 122, struct tzmem_region32)
#define TZMEM_COMPAT_CHECK_MEMORY \
	_IOR(TZ_IOC_MAGIC, 124, struct tzmem_region32)

struct tzmem_region32 {
	__s32		pid;	/* Memory region owner's PID (in) */
	__u32		ptr;	/* Memory region start (in) */
	__s32		size;	/* Memory region size (in) */
	__s32		id;	/* Memory region ID (out) */
	__u32		tee_ctx_id;	/* (in) */
	__s32		writable;
};
#endif

#ifdef __KERNEL__

typedef unsigned long tzdev_page_handle;

tzdev_page_handle tzdev_alloc_watch_page(void);
void *tzdev_get_virt_addr(tzdev_page_handle h);
phys_addr_t tzdev_get_phys_addr(tzdev_page_handle h);
void tzdev_free_watch_page(tzdev_page_handle pg);
int tzdev_scm_watch(unsigned long dev_id, unsigned long func_id,
		unsigned long param1,
		unsigned long param2,
		unsigned long param3);

typedef void (*tzdev_notify_handler_t)(uint32_t target_id,
		const void *buffer, size_t data_size, void *user_data);

/*
 *
 * Register notification handler for commands send using scm_send_notification()
 *
 * WARNING: Commands may be executed on any thread and any CPU !
 * Please keep the handler as short possible
 * (for example only notify completion) and avoid waiting on resources.
 *
 * WARNING 2: Do not send messages from notify handler to SecureOS.
 * This will cause deadlock.
 *
 */
int tzdev_register_notify_handler(uint32_t target_id, tzdev_notify_handler_t handler, void *user_data);
int tzdev_unregister_notify_handler(uint32_t target_id, tzdev_notify_handler_t handler, void *user_data);


#endif /* __KERNEL__ */
#endif /* __TZDEV_H__ */
