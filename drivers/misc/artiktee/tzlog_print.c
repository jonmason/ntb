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

#include <linux/kernel.h>
#include <linux/vmalloc.h>

#include "tzlog_print.h"

kernel_log_level default_tzdev_tee_log_level = TZDEV_TEE_LOG_LEVEL;
kernel_log_level default_tzdev_local_log_level = TZDEV_LOCAL_LOG_LEVEL;

#define DEFAULT_MEM_SIZE PAGE_SIZE
/* #define ENABLE_LOG_DEBUG */

static char buf_for_local[DEFAULT_MEM_SIZE];
static char buf_for_tee[DEFAULT_MEM_SIZE];

static int tzlog_print_common(log_header_info header_info,
		kernel_log_level level,
		const char *label,
		int is_ree,
		va_list *ap,
		const char *fmt)
{
	int write_len = 0;
	int buf_size = 0;
	char *printk_buf = NULL;
	if (is_ree == 1) {
		printk_buf = buf_for_local;
		buf_size = sizeof(buf_for_local);
	} else {
		printk_buf = buf_for_tee;
		buf_size = sizeof(buf_for_tee);
	}

	if (is_ree == 1) {
		switch (level) {
		case K_DEBUG:
			write_len = snprintf(printk_buf,
					buf_size, "[REE][TZDEV][DEBUG]");
			break;
		case K_INFO:
			write_len = snprintf(printk_buf,
					buf_size, "[REE][TZDEV][INFO]");
			break;
		case K_NOTICE:
			write_len = snprintf(printk_buf,
					buf_size, "[REE][TZDEV][NOTI]");
			break;
		case K_WARNING:
			write_len = snprintf(printk_buf,
					buf_size, "[REE][TZDEV][WARN]");
			break;
		case K_ERR:
			write_len = snprintf(printk_buf,
					buf_size, "[REE][TZDEV][ERR]");
			break;
		case K_CRIT:
			write_len = snprintf(printk_buf,
					buf_size, "[REE][TZDEV][CRIT]");
			break;
		case K_ALERT:
			write_len = snprintf(printk_buf,
					buf_size, "[REE][TZDEV][ALERT]");
			break;
		case K_EMERG:
			write_len = snprintf(printk_buf,
					buf_size, "[REE][TZDEV][EMERG]");
			break;
		default:
			break;
		}
	} else {
		switch (header_info) {
		case SWD_USERMODE:
			write_len = snprintf(printk_buf,
					buf_size, "[TEE][TA]");
			break;
		case SWD_KERNMODE:
			write_len = snprintf(printk_buf,
					buf_size, "[TEE][KERN]");
			break;
		default:
			write_len = snprintf(printk_buf,
					buf_size, "[TEE]");
			break;
		}

		switch (level) {
		case K_DEBUG:
			write_len += snprintf(printk_buf + write_len,
					buf_size - 1 - write_len, "[DEBUG]");
			break;
		case K_INFO:
			write_len += snprintf(printk_buf + write_len,
					buf_size - 1 - write_len, "[INFO]");
			break;
		case K_NOTICE:
			write_len += snprintf(printk_buf + write_len,
					buf_size - 1 - write_len, "[NOTI]");
			break;
		case K_WARNING:
			write_len += snprintf(printk_buf + write_len,
					buf_size - 1 - write_len, "[WARN]");
			break;
		case K_ERR:
			write_len += snprintf(printk_buf + write_len,
					buf_size - 1 - write_len, "[ERR]");
			break;
		case K_CRIT:
			write_len += snprintf(printk_buf + write_len,
					buf_size - 1 - write_len, "[CRIT]");
			break;
		case K_ALERT:
			write_len += snprintf(printk_buf + write_len,
					buf_size - 1 - write_len, "[ALERT]");
			break;
		case K_EMERG:
			write_len += snprintf(printk_buf + write_len,
					buf_size - 1 - write_len, "[EMERG]");
			break;
		default:
			break;
		}

		if (label != NULL)
			write_len += snprintf(printk_buf + write_len,
					buf_size - 1 - write_len, "[%s]",
					label);
	}

	write_len += vsnprintf(printk_buf + write_len,
			buf_size - 1 - write_len, fmt, *ap);

	if (write_len + 1 >= buf_size) /* buffer is not enough */
		pr_err("buffer is not enough(ree(%d) / cur : %d )\n",
				is_ree, buf_size);

	switch (level) {
	case K_DEBUG:
		pr_debug("%s", printk_buf);
		break;
	case K_INFO:
		pr_info("%s", printk_buf);
		break;
	case K_NOTICE:
		pr_notice("%s", printk_buf);
		break;
	case K_WARNING:
		pr_warn("%s", printk_buf);
		break;
	case K_ERR:
		pr_err("%s", printk_buf);
		break;
	case K_CRIT:
		pr_crit("%s", printk_buf);
		break;
	case K_ALERT:
		pr_alert("%s", printk_buf);
		break;
	case K_EMERG:
		pr_emerg("%s", printk_buf);
		break;
	default:
		break;
	}
	return 0;
}

/*
   will be use bit flag for decreasing parameter count.
   we can change log_header_info to use bit flag
 */
void tzlog_print_for_tee(log_header_info header_info,
		kernel_log_level level,
		const char *label,
		const char *fmt, ...)
{
	int ret;
	va_list ap;
	if (!(default_tzdev_tee_log_level >= level))
		return;

	va_start(ap, fmt);
	ret = tzlog_print_common(header_info, level, label, 0, &ap, fmt);
	va_end(ap);

#ifdef ENABLE_LOG_DEBUG
	if (ret == -1)
		pr_err("tzlog_print_common return err\n");
#endif
}

void tzlog_print(kernel_log_level level, const char *fmt, ...)
{
	int ret;
	va_list ap;
	if (!(default_tzdev_local_log_level >= level))
		return;

	va_start(ap, fmt);
	ret = tzlog_print_common(0, level, NULL, 1, &ap, fmt);
	va_end(ap);

#ifdef ENABLE_LOG_DEBUG
	if (ret == -1)
		pr_err("tzlog_print_common return err\n");
#endif
}
