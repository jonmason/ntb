/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Youngbok, Park <ybpark@nexell.co.kr>
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/ctype.h>
#include <linux/io.h>

#define PHY_BASEADDR_ECID_MODULE	(0xC0067000)

struct nx_ecid_register_set {
	u32 ecid[4];
	u8 chipname[48];
	u32 reserved;
	u32 guid0;
	u16 guid1;
	u16 guid2;
	u8 guid3[8];
	u32 ec[3];
};

struct nx_guid {
	u32 guid0;
	u16 guid1;
	u16 guid2;
	u8 guid3[8];
};

static struct {
	struct nx_ecid_register_set *pregister;
} __g_module_variables = {
	NULL,
};


static unsigned int convertmsblsb(uint32_t data, int bits)
{
	uint32_t result = 0;
	uint32_t mask = 1;
	int i = 0;

	for (i = 0; i < bits ; i++) {
		if (data & (1<<i))
			result |= mask<<(bits-i-1);
	}
	return result;
}

static const char gst36Strtable[36] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
	'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

static void lotid_num2string(uint32_t lotId, char str[6])
{
	uint32_t value[3];
	uint32_t mad[3];

	value[0] = lotId / 36;
	mad[0] = lotId % 36;

	value[1] = value[0] / 36;
	mad[1] = value[0] % 36;

	value[2] = value[1] / 36;
	mad[2] = value[1]  % 36;

	str[0] = 'N';
	str[1] = gst36Strtable[value[2]];
	str[2] = gst36Strtable[mad[2]];
	str[3] = gst36Strtable[mad[1]];
	str[4] = gst36Strtable[mad[0]];
	str[5] = '\0';
}

void nx_ecid_set_base_address(void *base_address)
{
	__g_module_variables.pregister =
	    (struct nx_ecid_register_set *)base_address;
}

int nx_ecid_get_key_ready(void)
{
	const u32 ready_pos = 15;
	const u32 ready_mask = 1ul << ready_pos;

	register u32 regval;

	regval = __g_module_variables.pregister->ec[2];

	return (int)((regval & ready_mask) >> ready_pos);
}

void nx_ecid_get_chip_name(u8 chip_name[49])
{
	u32 i;

	for (i = 0; i < 48; i++)
		chip_name[i] = __g_module_variables.pregister->chipname[i];

	for (i = 0; i < 48; i++) {
		if ((chip_name[i] == '-') && (chip_name[i + 1] == '-')) {
			chip_name[i] = 0;
			chip_name[i + 1] = 0;
		}
	}
}

void nx_ecid_get_ecid(u32 ecid[4])
{
	ecid[0] = __g_module_variables.pregister->ecid[0];
	ecid[1] = __g_module_variables.pregister->ecid[1];
	ecid[2] = __g_module_variables.pregister->ecid[2];
	ecid[3] = __g_module_variables.pregister->ecid[3];
}

void nx_ecid_get_guid(struct nx_guid *guid)
{
	guid->guid0 = __g_module_variables.pregister->guid0;
	guid->guid1 = __g_module_variables.pregister->guid1;
	guid->guid2 = __g_module_variables.pregister->guid2;
	guid->guid3[0] = __g_module_variables.pregister->guid3[0];
	guid->guid3[1] = __g_module_variables.pregister->guid3[1];
	guid->guid3[2] = __g_module_variables.pregister->guid3[2];
	guid->guid3[3] = __g_module_variables.pregister->guid3[3];
	guid->guid3[4] = __g_module_variables.pregister->guid3[4];
	guid->guid3[5] = __g_module_variables.pregister->guid3[5];
	guid->guid3[6] = __g_module_variables.pregister->guid3[6];
	guid->guid3[7] = __g_module_variables.pregister->guid3[7];
}

static inline int wait_key_ready(void)
{
	while (!nx_ecid_get_key_ready()) {
		if (time_after(jiffies, jiffies + 1)) {
			if (nx_ecid_get_key_ready())
				break;
			pr_err("Error: id not key ready\n");
			return -EINVAL;
		}
		cpu_relax();
	}
	return 0;
}

int nxp_cpu_id_guid(u32 guid[4])
{
	if (0 > wait_key_ready())
		return -EINVAL;
	nx_ecid_get_guid((struct nx_guid *)guid);
	return 0;
}

int nxp_cpu_id_ecid(u32 ecid[4])
{
	if (0 > wait_key_ready())
		return -EINVAL;
	nx_ecid_get_ecid(ecid);
	return 0;
}

int nxp_cpu_id_string(u32 *string)
{
	if (0 > wait_key_ready())
		return -EINVAL;
	nx_ecid_get_chip_name((char *)string);
	return 0;
}

/* Notify cpu GUID: /sys/devices/platform/cpu,  guid, uuid,  name  */
static ssize_t sys_id_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct attribute *at = &attr->attr;
	char *s = buf;
	u32 uid[4] = {0, };
	u8  name[12*4] = {0,};
	int string = -EINVAL;

	pr_debug("[%s : name =%s ]\n", __func__, at->name);

	if (!strcmp(at->name, "uuid"))
		nxp_cpu_id_ecid(uid);
	else if (!strcmp(at->name, "guid"))
		nxp_cpu_id_guid(uid);
	else if (!strcmp(at->name, "name"))
		string = nxp_cpu_id_string((u32 *)name);
	else
		return -EINVAL;

	if (!string) {
		if (isprint(name[0])) {
			s += sprintf(s, "%s\n", name);
		} else {
			#define _W	(12)	/* width */
			int i;

			for (i = 0; i < sizeof(name); i++) {
				s += sprintf(s, "%02x", name[i]);
				if ((i+1) % _W == 0)
					s += sprintf(s, " ");
			}
			s += sprintf(s, "\n");
		}
	} else {
		s += sprintf(s, "%08x:%08x:%08x:%08x\n",
			     uid[0], uid[1], uid[2], uid[3]);
	}

	if (s != buf)
		*(s-1) = '\n';

	return (s - buf);
}

#define	ATTR_MODE	0644
static struct device_attribute __guid__ =
			__ATTR(guid, ATTR_MODE, sys_id_show, NULL);
static struct device_attribute __uuid__ =
			__ATTR(uuid, ATTR_MODE, sys_id_show, NULL);
static struct device_attribute __name__ =
			__ATTR(name, ATTR_MODE, sys_id_show, NULL);

static struct attribute *sys_attrs[] = {
	&__guid__.attr,
	&__uuid__.attr,
	&__name__.attr,
	NULL,
};

static struct attribute_group sys_attr_group = {
	.attrs = (struct attribute **)sys_attrs,
};

static int __init cpu_sys_id_setup(void)
{
	struct kobject *kobj;
	u32 uid[4] = {0, };
	int ret = 0;
	u32 lotid;
	char strlotid[6];

	nx_ecid_set_base_address(ioremap_nocache(PHY_BASEADDR_ECID_MODULE,
						 0x1000));

	kobj = kobject_create_and_add("cpu", &platform_bus.kobj);
	if (!kobj) {
		pr_err("Failed create cpu kernel object ....\n");
		return -ret;
	}

	ret = sysfs_create_group(kobj, &sys_attr_group);
	if (ret) {
		pr_err("Failed create cpu sysfs group ...\n");
		kobject_del(kobj);
		return -ret;
	}

	if (0 > nxp_cpu_id_ecid(uid))
		pr_err("FAIL: ecid !!!\n");

	lotid =  convertmsblsb(uid[0] & 0x1FFFFF, 21);
	lotid_num2string(lotid, strlotid);

	pr_info("ECID: %08x:%08x:%08x:%08x\n", uid[0], uid[1], uid[2], uid[3]);
	pr_info("LOT ID : %s\n", strlotid);

	return ret;
}

static int __init cpu_sys_init_setup(void)
{
	cpu_sys_id_setup();
	return 0;
}
core_initcall(cpu_sys_init_setup);
