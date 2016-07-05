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

#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/semaphore.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/namei.h>
#include <linux/path.h>

#include <asm/barrier.h>

#include "circular_buffer.h"
#include "tzdev.h"
#include "tzdev_internal.h"
#include "tzpage.h"
#include "tzdev_smc.h"

/*#define CONFIG_INSTANCE_DEBUG*/
#ifdef CONFIG_INSTANCE_DEBUG
#define ERROR_PARENT_DIR_PATH  "/opt/usr/apps/save_error_log/"
#define ERROR_DIR_NAME "error_log"
#define ERROR_LOG_PARENT_DIR_PATH  "/opt/usr/apps/save_error_log/error_log/"
#define ERROR_LOG_DIR_NAME "secureos_log"
#define SYSLOG_ENCRYPT_MEM_SIZE (PAGE_SIZE * 8)
#endif /* CONFIG_INSTANCE_DEBUG */
#define TZLOG_TYPE_PURE 1
#define TZLOG_TYPE_ENCRYPT 2

#define TZLOG_PAGE_ORDER		0

/*#define WRITE_TZLOG_TO_PRINTK 1*/

#define TZLOG_LINE_MAX		256

void tzdev_notify_worker(void);

static ssize_t tzlog_read(struct file *filp, char __user *buffer, size_t size,
			  loff_t *off);
static unsigned int tzlog_poll(struct file *file, poll_table *wait);
static int tzlog_fasync(int fd, struct file *filp, int on);

static struct fasync_struct *fasync;

struct tzlog_data {
	int log_wsm_id;
	struct chimera_ring_buffer *ring;
	struct semaphore sem;
	struct task_struct *task;
	struct mutex lock;
#ifndef WRITE_TZLOG_TO_PRINTK
	wait_queue_head_t full_wait;
	wait_queue_head_t empty_wait;
#endif
	struct page *log_page;
};

static struct tzlog_data log_data = {
	.sem = __SEMAPHORE_INITIALIZER(log_data.sem, 0),
	.lock = __MUTEX_INITIALIZER(log_data.lock),
#ifndef WRITE_TZLOG_TO_PRINTK
	.full_wait = __WAIT_QUEUE_HEAD_INITIALIZER(log_data.full_wait),
	.empty_wait = __WAIT_QUEUE_HEAD_INITIALIZER(log_data.empty_wait)
#endif
};

#ifdef CONFIG_INSTANCE_DEBUG
static struct tzlog_data encrypt_log_data;
#endif

static const struct file_operations tzlog_file_operations = {
	.owner = THIS_MODULE,
	.read = tzlog_read,
	.poll = tzlog_poll,
	.fasync = tzlog_fasync,
	.llseek = noop_llseek
};

static struct miscdevice tzlog_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "tzlog",
	.fops = &tzlog_file_operations,
	.mode = 0400
};

#ifdef WRITE_TZLOG_TO_PRINTK
#define TZLOG_LINE_MAX	256

struct tzlog_per_cpu {
	size_t tzlog_line_pos;
	char tzlog_line_loglevel;
	int state;
	char tzlog_line[TZLOG_LINE_MAX + 1];
};

static struct tzlog_per_cpu __pcpu_tzlog_data = {
	.tzlog_line_loglevel = 'd'
};

static inline void tzlog_line_accumulate(struct tzlog_per_cpu *data, char c)
{
	data->tzlog_line[data->tzlog_line_pos++] = c;
}

static inline void tzlog_line_flush(struct tzlog_per_cpu *data)
{
	tzlog_line_accumulate(data, 0);
	tzlog_print(TZLOG_DEBUG, "%c" "[SECOS] %s\n", data->tzlog_line_loglevel,
		    data->tzlog_line);
	data->tzlog_line_pos = 0;
	data->state = 0;
}

static void tzlog_line_putc(struct tzlog_per_cpu *data, char c)
{
	if (data->state == 1) {
		data->state = 0;
		/* Validate loglevel */
		if (c >= '0' && c <= '7')
			data->tzlog_line_loglevel = c;
		else
			data->tzlog_line_loglevel = 'd';

		return;
	}

	if (c == '\1') {
		if (data->tzlog_line_pos > 0)
			tzlog_line_flush(data);

		data->state = 1;
		return;
	}

	if (c == '\n') {
		tzlog_line_flush(data);
		data->state = 0;
		return;
		/* Rewind cursor */
	}

	tzlog_line_accumulate(data, c);
	if (data->tzlog_line_pos >= (TZLOG_LINE_MAX - 8))
		tzlog_line_flush(data);
}

static void __tzlog_drain_syslog(struct tzlog_per_cpu *data, const char *buf,
				 size_t length)
{
	size_t i;

	for (i = 0; i < length; ++i)
		tzlog_line_putc(data, buf[i]);

	data->state = 0;

	if (data->tzlog_line_pos > 0)
		tzlog_line_putc(data, '\n');
}
#endif /* WRITE_TZLOG_TO_PRINTK */

static ssize_t tzlog_read(struct file *filp, char __user *buffer, size_t size,
			  loff_t *off)
{
#ifndef WRITE_TZLOG_TO_PRINTK
	int error = mutex_lock_interruptible(&log_data.lock);
	ssize_t result;

	if (error < 0)
		return error;

	tzlog_print(TZLOG_DEBUG, "TZLOG read %d size = %d, in = %d, out = %d\n",
		    (int)size, log_data.ring->size, log_data.ring->in,
		    log_data.ring->first);

	result =
	    chimera_ring_buffer_user_read(log_data.ring, (uint8_t *) buffer,
					  size, PAGE_SIZE << TZLOG_PAGE_ORDER);

	tzlog_print(TZLOG_DEBUG,
		    "TZLOG read result %d size = %d, in = %d, out = %d\n",
		    (int)result, log_data.ring->size, log_data.ring->in,
		    log_data.ring->first);

	if (result > 0)
		wake_up(&log_data.full_wait);

	mutex_unlock(&log_data.lock);

	return result;
#else
	return 0;
#endif /* WRITE_TZLOG_TO_PRINTK */
}

static unsigned int tzlog_poll(struct file *file, poll_table *wait)
{
#ifndef WRITE_TZLOG_TO_PRINTK
	unsigned int mask;

	poll_wait(file, &log_data.empty_wait, wait);

	mask = 0;

	if (chimera_ring_buffer_readable(log_data.ring))
		mask |= POLLIN | POLLRDNORM;

	return mask;
#else
	return 0;
#endif /* WRITE_TZLOG_TO_PRINTK */
}

static int tzlog_fasync(int fd, struct file *filp, int on)
{
	return fasync_helper(fd, filp, on, &fasync);
}

static void tzlog_drain_syslog(struct tzlog_data *data)
{
#ifndef WRITE_TZLOG_TO_PRINTK
	wake_up_interruptible(&data->empty_wait);
	kill_fasync(&fasync, SIGIO, POLL_IN);

	if (chimera_ring_buffer_writable(data->ring) == 0) {
		DEFINE_WAIT(__wait);

		tzlog_print(TZLOG_DEBUG, "tzlog full. Wait for drain\n");

		while (chimera_ring_buffer_writable(data->ring) == 0) {
			prepare_to_wait(&log_data.full_wait, &__wait,
					TASK_UNINTERRUPTIBLE);

			if (chimera_ring_buffer_writable(data->ring))
				break;

			mutex_unlock(&log_data.lock);
			schedule();
			mutex_lock(&log_data.lock);
		}

		finish_wait(&log_data.full_wait, &__wait);
	}
#else
	struct iovec iov[2];
	int i;
	int nvec =
	    chimera_ring_buffer_get_vecs(data->ring, iov,
					 PAGE_SIZE << TZLOG_PAGE_ORDER);

	for (i = 0; i < nvec; ++i)
		__tzlog_drain_syslog(&__pcpu_tzlog_data,
				     (const char *)iov[i].iov_base,
				     iov[i].iov_len);

	chimera_ring_buffer_clear(data->ring);
#endif /* WRITE_TZLOG_TO_PRINTK */
}

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
	mm_segment_t old_fs;
	int write_size = 0;
	struct file *file = NULL;

	char path[128];
	unsigned long long T;
	struct timeval now;

	tzlog_create_dir(ERROR_PARENT_DIR_PATH, ERROR_DIR_NAME);
	tzlog_create_dir(ERROR_LOG_PARENT_DIR_PATH, ERROR_LOG_DIR_NAME);

	do_gettimeofday(&now);
	T = (now.tv_sec * 1000ULL) + (now.tv_usec / 1000LL);
	snprintf(path, sizeof(path), "%s%s/minilog_%s_%lld.log",
		 ERROR_LOG_PARENT_DIR_PATH, ERROR_LOG_DIR_NAME,
		 ((is_kernel_error == 1) ? "os" : "app"), T);
	file = filp_open(path, O_CREAT | O_RDWR | O_SYNC, 0600);
	if (IS_ERR(file)) {
		tzlog_print(TZLOG_ERROR,
			    "tzlog error occured while opening file %s, exiting...\n",
			    path);
		return -1;
	}
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	file->f_op->llseek(file, 0, SEEK_END);

	write_size += file->f_op->write(file, data, data_size, &file->f_pos);

	set_fs(old_fs);
	filp_close(file, NULL);
	file = NULL;

	return write_size;
}
#endif /* CONFIG_INSTANCE_DEBUG */

static int tzlog_worker(void *arg)
{
#ifdef CONFIG_INSTANCE_DEBUG
	int is_need_enc = 0;
	int enc_ret = 0;
	int complete = 0;
	int bodySize = 0;
	char *data = NULL;

	int headerOffset = 0;
	int completeOffset = sizeof(int);
	int bodyOffset = sizeof(int) * 2;

	data = (char *)encrypt_log_data.ring;
	*(int *)((char *)data + completeOffset) = 0;
	*(int *)((char *)data + headerOffset) = 0;
	is_need_enc = 1;
#endif /* CONFIG_INSTANCE_DEBUG */

	tzlog_print(TZLOG_DEBUG, "Start tzlog worker thread\n");

	mutex_lock(&log_data.lock);
	while (!kthread_should_stop()) {
		int ret;

		do {
			ret = scm_syslog(log_data.log_wsm_id, TZLOG_TYPE_PURE);

			if (chimera_ring_buffer_readable(log_data.ring)) {
				/* We may need to process tasks after reading back all */
				tzdev_notify_worker();
				/* Drain syslog data further */
				tzlog_drain_syslog(&log_data);

			}

#ifdef CONFIG_INSTANCE_DEBUG
			if (is_need_enc == 1) {
				*(int *)((char *)data + completeOffset) = 0;
				*(int *)((char *)data + headerOffset) = 0;

				tzlog_print(TZLOG_DEBUG,
					    "tzlog before scm_syslog(encrypt)\n");

				enc_ret =
				    scm_syslog(encrypt_log_data.log_wsm_id,
					       TZLOG_TYPE_ENCRYPT);

				bodySize =
				    *(int *)((char *)data + headerOffset);
				complete =
				    *(int *)((char *)data + completeOffset);

				tzlog_print(TZLOG_DEBUG,
					    "tzlog returned %d size = %d(encrypt)\n",
					    complete, bodySize);

				if (enc_ret == -1)
					is_need_enc = 0;
				else {
					if (complete & 0x01 && bodySize != 0) {
						tzlog_print(TZLOG_DEBUG,
							    "tzlog process start(encrypt)\n");

						tzdev_notify_worker();
						tzlog_output_to_error_log(
								((char *) data + bodyOffset), bodySize,
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

	ret = misc_register(&tzlog_device);

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
				SYSLOG_ENCRYPT_MEM_SIZE,
				GFP_KERNEL);

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
