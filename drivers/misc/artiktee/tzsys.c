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
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/syscalls.h>
#include <linux/atomic.h>

#include <asm/barrier.h>

#include "tzdev.h"
#include "tzdev_internal.h"
#include "tzpage.h"
#include "tzdev_smc.h"
#include "tzlog_print.h"
#include "ssdev_file.h"
#include "tzdev_plat.h"

#if defined(CONFIG_INSTANCE_DEBUG) && defined(CONFIG_USB_DUMP)
#include "usb_dump.h"
#endif

#ifdef CONFIG_INSTANCE_DEBUG
#include <linux/vmalloc.h>
#define CONFIG_TZDEV_MINIDUMP
#define ENC_DUMP_DIR_PATH  "/opt/usr/apps/save_error_log/error_log/secureos_dump/"
#endif

struct secos_syspage *tz_syspage;
#ifdef CONFIG_TZDEV_MINIDUMP
char *tz_minidump_data;
uint  tz_minidump_data_size;
#endif
static atomic_t tz_crashed;

#define MINIDUMP_PAGES			256

char *get_minidump_buf(void)
{
#ifdef CONFIG_TZDEV_MINIDUMP
	return tz_minidump_data;
#endif
	return NULL;
}

uint get_minidump_buf_size(void)
{
#ifdef CONFIG_TZDEV_MINIDUMP
	return tz_minidump_data_size;
#endif
	return 0;
}

void __init tzsys_init(void)
{
	int memid;
	int rc;
#ifdef CONFIG_TZDEV_MINIDUMP
	void *minipage;
	int minipage_size;
	int miniwsm;
#endif /* CONFIG_TZDEV_MINIDUMP */
	struct page *pg;

	pg = alloc_page(GFP_KERNEL);
	BUG_ON(pg == NULL);

	tz_syspage = (struct secos_syspage *)page_address(pg);

	memid = tzwsm_register_tzdev_memory(0, &pg, 1, GFP_KERNEL, 1);
	BUG_ON(memid < 0);

	rc = scm_syscrash_register(memid);
	BUG_ON(rc < 0);

#ifdef CONFIG_TZDEV_MINIDUMP
	tzlog_print(K_INFO, "Register MiniDump\n");

	/* 1MB for minidump */
	minipage_size = MINIDUMP_PAGES * PAGE_SIZE;
	minipage = vmalloc(minipage_size);
	BUG_ON(!minipage);

	tz_minidump_data = (char *)minipage;
	tz_minidump_data_size = minipage_size;

	memset(minipage, 0, minipage_size);

	miniwsm =
	    tzwsm_register_kernel_memory(minipage, minipage_size,
					 GFP_KERNEL);
	BUG_ON(miniwsm < 0);

	rc = scm_minidump_register(miniwsm);

	tzlog_print(K_INFO, "This tzdev has minidump system enabled !!!\n");
#endif /* CONFIG_TZDEV_MINIDUMP */
}

#ifdef CONFIG_TZDEV_MINIDUMP
int tzlog_create_dir_full_path(char *dir_full_path);
int tzlog_output_do_dump(int is_kernel)
{
	int write_size;
	struct timeval now;
	unsigned long long T;
	int ret = 0;
	char *file_full_path = NULL;

	if (!tz_syspage->minidump_size)
		return -1;

	if (is_kernel == 0) {
		if (tz_syspage->syscrash0 == SYSCRASH_VALUE_0 &&
		    tz_syspage->syscrash1 == SYSCRASH_VALUE_1) {
			tzlog_print(K_ERR, "Secure kernel crash already occurred\n");
			return -1;
		}
	}

	if (is_kernel == 1)
		tzlog_print(K_ERR, "SecureOS Crash detected\n");
	else
		tzlog_print(K_ERR, "TA Crash detected\n");

	if ((ret = tzlog_create_dir_full_path(ENC_DUMP_DIR_PATH)) != 0)	{
		tzlog_print(K_ERR, "tzlog_create_dir_full_path failed\n");
		return ret;
	}

	do_gettimeofday(&now);
	T = (now.tv_sec * 1000ULL) + (now.tv_usec / 1000LL);

	file_full_path = (char*)vmalloc(PATH_MAX);
	if (file_full_path == NULL) {
		tzlog_print(K_ERR, "vmalloc failed\n");
		ret = -1;
		goto exit_tzlog_output_do_dump;
	}
	snprintf(file_full_path, PATH_MAX, "%sminidump_%s_%010lld.elf",
		 ENC_DUMP_DIR_PATH,
		 ((is_kernel == 1) ? "os" : "app"), T);

	write_size = ss_file_create_object(file_full_path, tz_minidump_data,
			tz_syspage->minidump_size);

	if (write_size <= 0) {
		tzlog_print(K_ERR,
			"error occured while opening file %s, exiting...\n",
			file_full_path);
		ret = -1;
		goto exit_tzlog_output_do_dump;
	}

	if (is_kernel == 1)
		tzlog_print(K_ERR, "Writing SecureOS minidump to %s\n", file_full_path);
	else
		tzlog_print(K_INFO, "Writing TA minidump to %s\n", file_full_path);

	if (is_kernel == 1)
		tzlog_print(K_ERR, "Minidump stored %u bytes\n",
			    tz_syspage->minidump_size);
	else
		tzlog_print(K_INFO,
			    "Minidump TA stored %u bytespc)(uuid:%s)\n",
			    tz_syspage->minidump_size, tz_syspage->uid);

	memset(tz_minidump_data, 0, tz_syspage->minidump_size);

	tzlog_print(K_INFO, "file_full_path-dump %s \n", file_full_path);

#if defined(CONFIG_INSTANCE_DEBUG) && defined(CONFIG_USB_DUMP)
	if (ss_file_object_exist(file_full_path) == 1)
		copy_file_to_usb(file_full_path);
	else tzlog_print(K_ERR, "file(%s) not exist \n", file_full_path);
#endif

	if (is_kernel == 1)
		plat_dump_postprocess("TrustWare");
	else plat_dump_postprocess(tz_syspage->uid);

	memset(tz_syspage->uid, 0, sizeof(tz_syspage->uid));

	/* End of writing the dump file */
	tz_syspage->minidump_size = 0;

exit_tzlog_output_do_dump:

	if (file_full_path != NULL)
		vfree(file_full_path);

	return ret;
}

static int tz_crash_worker(void *unused)
{
	if (!tz_syspage->minidump_size)
		goto out;

	tzlog_output_do_dump(1);

	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout(3 * HZ);
out:
	panic("Detected crash of Secure Kernel\n");
	return 0;
}
#endif /* CONFIG_TZDEV_MINIDUMP */

void tzsys_crash_check(void)
{
	if (tz_syspage->syscrash0 == SYSCRASH_VALUE_0 &&
	    tz_syspage->syscrash1 == SYSCRASH_VALUE_1) {
#ifdef CONFIG_TZDEV_MINIDUMP
		struct task_struct *task;
#endif

		if (atomic_inc_return(&tz_crashed) != 1)
			return;
#ifdef CONFIG_TZDEV_MINIDUMP
		task = kthread_create(tz_crash_worker, NULL, "tz_minidump");

		if (!IS_ERR(task)) {
			wake_up_process(task);
		} else {
			tzlog_print(K_ERR,
				    "Can't spawn worker for minidump. Execute now\n");
			tz_crash_worker(NULL);
		}
#else
		panic("Detected crash of Secure Kernel\n");
#endif /* CONFIG_TZDEV_MINIDUMP */
	}
}
