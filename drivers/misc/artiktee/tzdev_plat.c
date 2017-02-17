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

#include <linux/pm_qos.h>
#include "tzdev_plat.h"

static struct pm_qos_request min_cpu_qos;

int plat_init(void)
{
	int ret = 0;

	pm_qos_add_request(&min_cpu_qos, PM_QOS_CPU_FREQ_MIN, 0);

	return ret;
}

int plat_preprocess(void)
{
	int ret = 0;

	if (pm_qos_request_active(&min_cpu_qos))
		pm_qos_update_request(&min_cpu_qos, 1200000);

	return ret;
}

int plat_postprocess(void)
{
	int ret = 0;

	if (pm_qos_request_active(&min_cpu_qos))
		pm_qos_update_request(&min_cpu_qos, -1);

	return ret;
}

int plat_dump_postprocess(char *name)
{
	return 0;
}
