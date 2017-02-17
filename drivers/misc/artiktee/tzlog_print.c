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
#include "log_system_api_ext.h"

kernel_log_level default_tzdev_tee_log_level = TZDEV_TEE_LOG_LEVEL;
kernel_log_level default_tzdev_local_log_level = TZDEV_LOCAL_LOG_LEVEL;

#define DEFAULT_MEM_SIZE PAGE_SIZE

static char buf_for_tzdev[DEFAULT_MEM_SIZE];
static char buf_for_tee[DEFAULT_MEM_SIZE];
const static char buf_for_prefix[8][10] = {"[EMERG]", "[ALERT]", "[CRIT]", "[ERR]", \
					   "[WARN]", "[NOTI]", "[INFO]", "[DEBUG]"};

static int tzlog_print_common(
		uint32_t level,
		const char *label,
		va_list *ap,
		const char *fmt)
{
	int write_len = 0;
	int buf_size = 0;
	char *printk_buf = NULL;
	kernel_log_level only_level = GET_LEVEL(level);

	if(IS_SWD(level)) {
		printk_buf = buf_for_tee;
		buf_size = sizeof(buf_for_tee);
	} else {
		printk_buf = buf_for_tzdev;
		buf_size = sizeof(buf_for_tzdev);
	}

	if(IS_SWD(level)) {
		/* Prefix addition for TEE */
                if(IS_PREFIX(level)) {
                        write_len = snprintf(printk_buf, buf_size, "[TEE]");

                        if (label != NULL) {
                                write_len += snprintf(printk_buf + write_len,
                                buf_size - 1 - write_len, "[%s]", label);
                        }

                        write_len += snprintf(printk_buf + write_len,
                                              buf_size - write_len -1,
                                              buf_for_prefix[only_level]);
                }
	} else {
		/* Prefix addition for REE */
                write_len = snprintf(printk_buf, buf_size, "[REE][TZDEV]");
                write_len += snprintf(printk_buf + write_len,
                                      buf_size - write_len -1,
                                      buf_for_prefix[only_level]);
	}

	write_len += vsnprintf(printk_buf + write_len,
			       buf_size - 1 - write_len, fmt, *ap);

	if (write_len + 1 >= buf_size) {
		/* buffer is not enough */
		printk(KERN_ERR "buffer is not enough, \
		       write:%d, avail:%d\n", write_len, buf_size);
	}

	switch (only_level) {
		case K_DEBUG:
			printk(KERN_DEBUG "%s", printk_buf);
			break;
		case K_INFO:
			printk(KERN_INFO "%s", printk_buf);
			break;
		case K_NOTICE:
			printk(KERN_NOTICE "%s", printk_buf);
			break;
		case K_WARNING:
			printk(KERN_WARNING "%s", printk_buf);
			break;
		case K_ERR:
			printk(KERN_ERR "%s", printk_buf);
			break;
		case K_CRIT:
			printk(KERN_CRIT "%s", printk_buf);
			break;
		case K_ALERT:
			printk(KERN_ALERT "%s", printk_buf);
			break;
		case K_EMERG:
			printk(KERN_EMERG "%s", printk_buf);
			break;
		default:
			break;
	}
	return 0;
}

/*
 * will be use bit flag for decreasing parameter count.
 * we can change log_header_info to use bit flag
 */
void tzlog_print_for_tee(uint32_t level,
			 const char *label,
			 const char *fmt, ...)
{
	int ret;
	va_list ap;
	int only_level = GET_LEVEL((int)level);

	if (((uint32_t)default_tzdev_tee_log_level < only_level)
	    || default_tzdev_tee_log_level == K_SILENT)
		return;

	va_start(ap, fmt);
	ret = tzlog_print_common(level, label, &ap, fmt);
	va_end(ap);

	if (ret == -1)
		printk(KERN_ERR "@tzlog_print_for_tee returns err\n");
}

void tzlog_print(uint32_t level, const char *fmt, ...)
{
	int ret;
	va_list ap;

	if (((uint32_t)default_tzdev_local_log_level < level)
	    || default_tzdev_local_log_level == K_SILENT)
		return;

	va_start(ap, fmt);
	ret = tzlog_print_common(level | NWD_MODE, NULL, &ap, fmt);
	va_end(ap);

	if (ret == -1)
		printk(KERN_ERR "tzlog_print returns err\n");
}
