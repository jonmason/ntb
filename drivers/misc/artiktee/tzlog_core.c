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
#include <linux/path.h>
#include <linux/vmalloc.h>

#include <asm/barrier.h>

#include "circular_buffer.h"
#include "tzdev.h"
#include "tzdev_internal.h"
#include "tzpage.h"
#include "tzdev_smc.h"
#include "ss_file.h"

#include "tzlog_process.h"
#include "tzlog_core.h"

#ifdef CONFIG_INSTANCE_DEBUG
#include <linux/vmalloc.h>
/* it should be change to removing Duplicated code(tzsys.c ) */
#define ERROR_PARENT_DIR_PATH  "/opt/usr/apps/"
#define ERROR_DIR_NAME_DEPTH1 "save_error_log"
#define ERROR_DIR_NAME_DEPTH2 "error_log"
#define ERROR_LOG_PARENT_DIR_PATH  "/opt/usr/apps/save_error_log/error_log/"
#define ERROR_LOG_DIR_NAME "secureos_log"
#define SYSLOG_ENCRYPT_MEM_SIZE (PAGE_SIZE * 8)
#endif /* CONFIG_INSTANCE_DEBUG */
#define TZLOG_TYPE_PURE 1
#define TZLOG_TYPE_ENCRYPT 2

#define TZLOG_PAGE_ORDER		0

extern void tzdev_notify_worker(void);

static s_tzlog_data log_data = {
	.ring = NULL,
	.sem = __SEMAPHORE_INITIALIZER(log_data.sem, 0),
	.lock = __MUTEX_INITIALIZER(log_data.lock),
};

#ifdef CONFIG_INSTANCE_DEBUG
static s_tzlog_data encrypt_log_data = {
	.ring = NULL,
};
#endif

/* it should be change to support creating two depth of folder */
int tzlog_create_dir(char *parnet_dir_name, char *dir_name)
{
	char path[128];
	struct dentry *dentry;
	struct path p;
	int err;
	struct inode *inode;

	snprintf(path, sizeof(path), "%s%s", parnet_dir_name, dir_name);

	dentry = kern_path_create(AT_FDCWD, path, &p, LOOKUP_DIRECTORY);

	if (IS_ERR(dentry))
		return PTR_ERR(dentry);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
	inode = p.dentry->d_inode;
#else
	inode = d_inode(p.dentry);
#endif

	err = vfs_mkdir(inode, dentry, S_IRWXU | S_IRWXG | S_IRWXO);

	done_path_create(&p, dentry);
	return err;
}

#ifdef CONFIG_INSTANCE_DEBUG
int tzlog_output_do_dump(int is_kernel);
static int tzlog_output_to_error_log(char *data, int data_size,
				     int is_kernel_error)
{
	char path[128];
	unsigned long long T;
	struct timeval now;
	tzlog_create_dir(ERROR_PARENT_DIR_PATH, ERROR_DIR_NAME_DEPTH1);
	snprintf(path, sizeof(path), "%s%s/", ERROR_PARENT_DIR_PATH,
		 ERROR_DIR_NAME_DEPTH1);
	tzlog_create_dir(path, ERROR_DIR_NAME_DEPTH2);
	tzlog_create_dir(ERROR_LOG_PARENT_DIR_PATH, ERROR_LOG_DIR_NAME);

	do_gettimeofday(&now);
	T = (now.tv_sec * 1000ULL) + (now.tv_usec / 1000LL);
	snprintf(path, sizeof(path), "%s%s/minilog_%s_%lld.log",
		 ERROR_LOG_PARENT_DIR_PATH, ERROR_LOG_DIR_NAME,
		 ((is_kernel_error == 1) ? "os" : "app"), T);

	return ss_file_create_object(path, data, data_size);
}
#endif /* CONFIG_INSTANCE_DEBUG */

static int tzlog_worker(void *arg)
{
#ifdef CONFIG_INSTANCE_DEBUG
	int is_need_enc = 0;
	int enc_ret = 0;
	int complete = 0;
	int body_size = 0;
	char *data = NULL;

	int header_offset = 0;
	int complete_offset = sizeof(int);
	int body_offset = sizeof(int) * 2;

	data = (char *)encrypt_log_data.ring;
	*(int *)((char *)data + complete_offset) = 0;
	*(int *)((char *)data + header_offset) = 0;
	is_need_enc = 1;
#endif /* CONFIG_INSTANCE_DEBUG */

	tzlog_print(TZLOG_DEBUG, "Start tzlog worker thread\n");

	mutex_lock(&log_data.lock);
	while (!kthread_should_stop()) {
		int ret;

		do {
			ret = scm_syslog(log_data.log_wsm_id, TZLOG_TYPE_PURE);

			if (chimera_ring_buffer_readable(log_data.ring)) {
				/*
				 * We may need to process tasks
				 * after reading back all
				 */
				tzdev_notify_worker();
				/* Drain syslog data further */
				tzlog_transfer_to_tzdaemon(&log_data,
							   PAGE_SIZE <<
							   TZLOG_PAGE_ORDER);
				tzlog_transfer_to_local(&log_data,
							PAGE_SIZE <<
							TZLOG_PAGE_ORDER);
				chimera_ring_buffer_clear(log_data.ring);
			}
#ifdef CONFIG_INSTANCE_DEBUG
			if (is_need_enc == 1) {
				*(int *)((char *)data + complete_offset) = 0;
				*(int *)((char *)data + header_offset) = 0;

				tzlog_print(TZLOG_DEBUG,
					    "tzlog before scm_syslog(encrypt)\n");

				enc_ret =
				    scm_syslog(encrypt_log_data.log_wsm_id,
					       TZLOG_TYPE_ENCRYPT);

				body_size =
				    *(int *)((char *)data + header_offset);
				complete =
				    *(int *)((char *)data + complete_offset);

				tzlog_print(TZLOG_DEBUG,
					    "tzlog returned %d size = %d(encrypt)\n",
					    complete, body_size);

				if (enc_ret == -1)
					is_need_enc = 0;
				else {
					if (complete & 0x01 && body_size != 0) {
						tzlog_print(TZLOG_DEBUG,
							    "tzlog process start(encrypt)\n");

						tzdev_notify_worker();
						tzlog_output_to_error_log(
							((char *)data+body_offset),
							body_size,
							((complete & 0x10) ? 1 : 0));

						if (!(complete & 0x10))
							tzlog_output_do_dump(0);

						tzlog_print(TZLOG_DEBUG,
							    "tzlog process complete(encrypt)\n");
					} else {
						tzlog_print(TZLOG_DEBUG,
							    "There's no data in tzlog yet(encrypt)\n");
					}
				}
			}
#endif /* CONFIG_INSTANCE_DEBUG */
		} while (ret > 0);

		mutex_unlock(&log_data.lock);
		ret = down_timeout(&log_data.sem, HZ * 2);

		(void)ret;

		/* Ignore other semaphore down calls */
		while (down_trylock(&log_data.sem) == 0)
			;	/* NULL */

		mutex_lock(&log_data.lock);
	}

	mutex_unlock(&log_data.lock);
	return 0;
}

void tzlog_notify(void)
{
	tz_syspage->tzlog_data_avail = 0;
	up(&log_data.sem);
}

void __init tzlog_init(void)
{
	int ret;

#ifdef CONFIG_INSTANCE_DEBUG
	void *large_page = vmalloc(SYSLOG_ENCRYPT_MEM_SIZE);
#endif

	ret = init_log_processing_resources();
	if (ret < 0)
		panic("Can't register tzlog device (%d)\n", ret);

	log_data.log_page =
	    alloc_pages(GFP_KERNEL | __GFP_ZERO, TZLOG_PAGE_ORDER);

	if (!log_data.log_page)
		panic("Can't allocate tzlog page\n");

	log_data.ring =
	    chimera_create_ring_buffer_etc(page_address(log_data.log_page),
					   PAGE_SIZE << TZLOG_PAGE_ORDER, 0,
					   GFP_KERNEL);

	tzlog_print(TZLOG_DEBUG, "TZLOG ring at %p\n", log_data.ring);
	if (log_data.ring != NULL)
		tzlog_print(TZLOG_DEBUG, "ring size %d\n", log_data.ring->size);

	log_data.log_wsm_id =
	    tzswm_register_tzdev_memory(0, &log_data.log_page,
					1U << TZLOG_PAGE_ORDER, GFP_KERNEL, 1);

	if (log_data.log_wsm_id < 0)
		panic("Can't register log WSM\n");

	tzlog_print(TZLOG_DEBUG, "Registered TZLOG WSM with id %d\n",
		    log_data.log_wsm_id);

#ifdef CONFIG_INSTANCE_DEBUG
	if (large_page == NULL)
		panic("Can't allocate tzlog page(encrypt)\n");

	encrypt_log_data.ring = large_page;
	memset(large_page, 0, SYSLOG_ENCRYPT_MEM_SIZE);

	tzlog_print(TZLOG_DEBUG, "TZLOG ring at %p(encrypt)\n",
		    encrypt_log_data.ring);
	tzlog_print(TZLOG_DEBUG, "ring size %d(encrypt)\n",
		    (int)SYSLOG_ENCRYPT_MEM_SIZE);

	encrypt_log_data.log_wsm_id =
	    tzwsm_register_kernel_memory(large_page,
					 SYSLOG_ENCRYPT_MEM_SIZE, GFP_KERNEL);

	if (encrypt_log_data.log_wsm_id < 0)
		panic("Can't register log WSM\n");

	tzlog_print(TZLOG_DEBUG,
		    "Registered TZLOG WSM with id %d(encrypt)\n",
		    encrypt_log_data.log_wsm_id);
#endif /* CONFIG_INSTANCE_DEBUG */
	log_data.task = kthread_run(tzlog_worker, NULL, "tzlogd");

	if (IS_ERR(log_data.task))
		panic("Can't create tzlog worker\n");
}
