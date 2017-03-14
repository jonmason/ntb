/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Hyejung, Kwon <cjscld15@nexell.co.kr>
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
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_sysctl.h>

#define ATTR_MODE 0644
static int nxs_trace_mode;
static struct kobject *kobj;

u32 nxs_get_trace_mode(void)
{
	pr_info("[%s] trace mode:%s\n", __func__,
		(nxs_trace_mode) ? "enable" : "disable");
	return nxs_trace_mode;
}

static ssize_t sys_trace_mode_show(struct kobject *kobj,
				   struct kobj_attribute *attr,
				   char *buf)
{
	return sprintf(buf, "%d\n", nxs_trace_mode);
}

static ssize_t sys_trace_mode_store(struct kobject *kobj,
				    struct kobj_attribute *attr,
				    const char *buf, size_t count)
{
	int ret;

	ret = kstrtoint(buf, 10, &nxs_trace_mode);
	if (ret < 0)
		return ret;
	return count;
}

static struct device_attribute __tracemode__ =
			__ATTR(tracemode, ATTR_MODE, sys_trace_mode_show,
			       sys_trace_mode_store);

static struct attribute *sys_attrs[] = {
	&__tracemode__.attr,
	NULL,
	NULL,
	NULL,
};

static struct attribute_group sys_attr_group = {
	.attrs = (struct attribute **)sys_attrs,
};

int nxs_sysctl_init(void)
{
	int ret = 0;

	pr_info("[%s] enter\n", __func__);
	kobj = kobject_create_and_add("nxstream", &platform_bus.kobj);
	if (!kobj) {
		pr_err("Failed to create nxstream object\n");
		return -ret;
	}

	ret = sysfs_create_group(kobj, &sys_attr_group);
	if (ret) {
		pr_err("Failed to create cpu sysfs group\n");
		kobject_del(kobj);
		return -ret;
	}
	pr_info("[%s] exit\n", __func__);
	return ret;
}

void nxs_sysctl_exit(void)
{
	if (kobj)
		kobject_del(kobj);
}

