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

#ifdef CONFIG_INSTANCE_DEBUG
#include <linux/vmalloc.h>
/* it should be change to removing Duplicated code(tzlog.c ) */
#define CONFIG_TZDEV_MINIDUMP
#define ERROR_PARENT_DIR_PATH  "/opt/usr/apps/"
#define ERROR_DIR_NAME_DEPTH1 "save_error_log"
#define ERROR_DIR_NAME_DEPTH2 "error_log"
#define ERROR_DUMP_PARENT_DIR_PATH  "/opt/usr/apps/save_error_log/error_log/"
#define ERROR_DUMP_DIR_NAME "secureos_dump"
#endif /* CONFIG_INSTANCE_DEBUG */

struct secos_syspage *tz_syspage;
#ifdef CONFIG_TZDEV_MINIDUMP
char *tz_minidump_data;
#endif
static atomic_t tz_crashed;

#ifdef CONFIG_INSTANCE_DEBUG
#ifdef CONFIG_CALL_SAVELOG
extern void set_kpi_fault(unsigned long pc, unsigned long lr, char *thread_name,
			  char *process_name, char *type);
#endif /* CONFIG_CALL_SAVELOG */
#endif /* CONFIG_INSTANCE_DEBUG */

#define MINIDUMP_PAGES			256

void __init tzsys_init(void)
{
	int memid;
	int rc;
#ifdef CONFIG_TZDEV_MINIDUMP
	void *minipage;
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
	minipage = vmalloc(MINIDUMP_PAGES * PAGE_SIZE);
	BUG_ON(!minipage);

	tz_minidump_data = (char *)minipage;

	memset(minipage, 0, MINIDUMP_PAGES * PAGE_SIZE);

	miniwsm =
	    tzwsm_register_kernel_memory(minipage, MINIDUMP_PAGES * PAGE_SIZE,
					 GFP_KERNEL);
	BUG_ON(miniwsm < 0);

	rc = scm_minidump_register(miniwsm);

	tzlog_print(K_INFO, "This tzdev has minidump system enabled !!!\n");
#endif /* CONFIG_TZDEV_MINIDUMP */
}

#ifdef CONFIG_TZDEV_MINIDUMP
int tzlog_create_dir(char *parnet_dir_name, char *dir_name);

int tzlog_output_do_dump(int is_kernel)
{
	int write_size;
	char path[128];
	struct timeval now;
	unsigned long long T;

	if (!tz_syspage->minidump_size)
		goto out;

	if (is_kernel == 0) {
		if (tz_syspage->syscrash0 == SYSCRASH_VALUE_0 &&
		    tz_syspage->syscrash1 == SYSCRASH_VALUE_1)
			return 0;
	}

	if (is_kernel == 1)
		tzlog_print(K_ERR, "SecureOS Crash detected\n");
	else
		tzlog_print(K_ERR, "TA Crash detected\n");

	tzlog_create_dir(ERROR_PARENT_DIR_PATH, ERROR_DIR_NAME_DEPTH1);
	snprintf(path, sizeof(path), "%s%s/",
			ERROR_PARENT_DIR_PATH, ERROR_DIR_NAME_DEPTH1);
	tzlog_create_dir(path, ERROR_DIR_NAME_DEPTH2);
	tzlog_create_dir(ERROR_DUMP_PARENT_DIR_PATH, ERROR_DUMP_DIR_NAME);

	do_gettimeofday(&now);

	T = (now.tv_sec * 1000ULL) + (now.tv_usec / 1000LL);

	snprintf(path, sizeof(path), "%s%s/minidump_%s_%lld.elf",
		 ERROR_DUMP_PARENT_DIR_PATH, ERROR_DUMP_DIR_NAME,
		 ((is_kernel == 1) ? "os" : "app"), T);

	write_size = ss_file_create_object(path, tz_minidump_data,
			tz_syspage->minidump_size);

	if (write_size <= 0) {
		tzlog_print(K_ERR,
			"error occured while opening file %s, exiting...\n",
			path);
		goto out;
	}

	if (is_kernel == 1)
		tzlog_print(K_ERR, "Writing SecureOS minidump to %s\n", path);
	else
		tzlog_print(K_INFO, "Writing TA minidump to %s\n", path);

	if (is_kernel == 1)
		tzlog_print(K_ERR, "Minidump stored %u bytes\n",
			    tz_syspage->minidump_size);
	else
		tzlog_print(K_INFO,
			    "Minidump TA stored %u bytespc)(uuid:%s)\n",
			    tz_syspage->minidump_size, tz_syspage->uid);

	memset(tz_minidump_data, 0, tz_syspage->minidump_size);
	memset(tz_syspage->uid, 0, sizeof(tz_syspage->uid));

#ifdef CONFIG_INSTANCE_DEBUG
#ifdef CONFIG_CALL_SAVELOG
	if (is_kernel == 1)
		set_kpi_fault(0, 0, "main", "TrustWare", "TZ");
	 else
		set_kpi_fault(0, 0, "main", tz_syspage->uid, "TZ");
#endif /* CONFIG_CALL_SAVELOG */
#endif /* CONFIG_INSTANCE_DEBUG */

	/* End of writing the dump file */
	tz_syspage->minidump_size = 0;
out:
	return 0;
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
