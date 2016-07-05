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
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/vmalloc.h>

#include "tzdev_smc.h"
#include "tzwsm.h"
#include "tzdev_internal.h"
#include "log_level_ree.h"
#include "tzlog_print.h"

#define TZINFO_CMD_REGISTER	0
#define TZINFO_CMD_FETCH	1

static char *tee_info_data;
static const char *info_msg = "Not Supported";

int tzinfo_fetch_info(char **buf)
{
	int rc;

	if (buf == NULL)
		return 0;

	rc = scm_fetch_tzinfo(TZINFO_CMD_FETCH, 0);

	*buf = tee_info_data;
	if (rc < 0)
		return sizeof(info_msg);

	return rc;
}

int tzinfo_init(void)
{
	void *tzinfo_page;
	int tzinfo_wsm;
	int rc;

	/* Single page for tz_info */
	tzinfo_page = vmalloc(PAGE_SIZE);
	if (tzinfo_page == NULL) {
		tzlog_print(K_ERR,
				"Failed to allocate memory for tzinfo\n");
		tee_info_data = (char *)info_msg;
		return -ENOMEM;
	}

	tee_info_data = (char *)tzinfo_page;

	memset(tee_info_data, 0, PAGE_SIZE);

	tzinfo_wsm =
		tzwsm_register_kernel_memory(tzinfo_page, PAGE_SIZE,
					 GFP_KERNEL);

	if (tzinfo_wsm < 0) {
		tzlog_print(K_ERR,
				"Failed to register WSM for tzinfo\n");
		vfree(tzinfo_page);
		tee_info_data = (char *)info_msg;
		return -EFAULT;
	}

	rc = scm_fetch_tzinfo(TZINFO_CMD_REGISTER, tzinfo_wsm);
	if (rc < 0) {
		tzlog_print(K_WARNING,
				"Failed to register tzinfo\n");
		tzwsm_unregister_kernel_memory(tzinfo_wsm);
		vfree(tzinfo_page);
		tee_info_data = (char *)info_msg;
		return -EFAULT;
	}

	return 0;
}
