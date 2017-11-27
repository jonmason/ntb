/*
 * Platform Dependent file for Nexell
 *
 * Copyright (C) 1999-2016, Broadcom Corporation
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: dhd_custom_nexell.c 577036 2015-08-05 13:15:51Z $
 */
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/workqueue.h>
#include <linux/unistd.h>
#include <linux/bug.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/wlan_plat.h>

#include <linux/sec_sysfs.h>

#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
extern int dhd_init_wlan_mem(void);
extern void *dhd_wlan_mem_prealloc(int section, unsigned long size);
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */

#define WIFI_TURNON_DELAY	200
static int wlan_pwr_on = -1;
static int wlan_host_wake_irq = 0;
static unsigned int wlan_host_wake_up = -1;
EXPORT_SYMBOL(wlan_host_wake_irq);

unsigned int is_inverted_wl_power;

static int
dhd_wlan_power(int onoff)
{
	struct pinctrl *pinctrl = NULL;
	printk(KERN_INFO"------------------------------------------------");
	printk(KERN_INFO"------------------------------------------------\n");
	printk(KERN_INFO"%s Enter: power %s\n", __FUNCTION__, onoff ? "on" : "off");

	if (is_inverted_wl_power) {
		 onoff = onoff ? 0: 1;
		 pr_info("%s: run inverted power control (onoff=%d)\n",
			__func__, onoff);
	} else {
		 pr_info("%s: run normal power control (onoff=%d)\n",
			__func__, onoff);
	}
#ifdef CONFIG_MACH_A7LTE
	if (onoff) {
		pinctrl = devm_pinctrl_get_select(mmc_dev_for_wlan, "sdio_wifi_on");
		if (IS_ERR(pinctrl))
			printk(KERN_INFO "%s WLAN SDIO GPIO control error\n", __FUNCTION__);
		msleep(PINCTL_DELAY);
	}
#endif /* CONFIG_MACH_A7LTE */

	if (gpio_direction_output(wlan_pwr_on, onoff)) {
		printk(KERN_ERR "%s failed to control WLAN_REG_ON to %s\n",
			__FUNCTION__, onoff ? "HIGH" : "LOW");
		return -EIO;
	}

#ifdef CONFIG_MACH_A7LTE
	if (!onoff) {
		pinctrl = devm_pinctrl_get_select(mmc_dev_for_wlan, "sdio_wifi_off");
		if (IS_ERR(pinctrl))
			printk(KERN_INFO "%s WLAN SDIO GPIO control error\n", __FUNCTION__);
	}
#endif /* CONFIG_MACH_A7LTE */
	return 0;
}

static int
dhd_wlan_reset(int onoff)
{
	return 0;
}

extern void (*notify_func_callback)(void *dev_id, int state);
extern void *mmc_host_dev;

static int
dhd_wlan_set_carddetect(int val)
{
	pr_err("%s: notify_func=%p, mmc_host_dev=%p, val=%d\n",
		__FUNCTION__, notify_func_callback, mmc_host_dev, val);

	if (notify_func_callback) {
		notify_func_callback(mmc_host_dev, val);
	} else {
		pr_warning("%s: Nobody to notify\n", __FUNCTION__);
	}

	return 0;
}
int __init
dhd_wlan_init_gpio(void)
{
	const char *wlan_node = "nexell,brcm-wlan";
	struct device_node *root_node = NULL;
	int ret;

	root_node = of_find_compatible_node(NULL, NULL, wlan_node);
	if (!root_node) {
		WARN(1, "failed to get device node of bcm4354\n");
		return -ENODEV;
	}

	if (of_property_read_u32(root_node, "inverted-power-control",
				&is_inverted_wl_power)) {
		pr_info("%s: inverted-power-control property not found",
				  __func__);
		is_inverted_wl_power = 0;
	}

	if (!is_inverted_wl_power) {
		pr_info("%s: Run normal power control\n", __func__);
	} else {
		pr_info("%s: Run inverted power control\n", __func__);
	}

	/* ========== WLAN_PWR_EN ============ */
	wlan_pwr_on = of_get_named_gpio(root_node,"wlan_reg_on", 0);
	if (!gpio_is_valid(wlan_pwr_on)) {
		printk("Invalied gpio pin : %d\n", wlan_pwr_on);
		return -ENODEV;
	}

	ret = gpio_request(wlan_pwr_on, "WLAN_REG_ON");
	if (ret < 0) {
		printk("fail to request gpio(WLAN_REG_ON) ret=%d \n", ret);
		return -ENODEV;
	}

	if (is_inverted_wl_power)
		gpio_direction_output(wlan_pwr_on, 1);
	else
		gpio_direction_output(wlan_pwr_on, 0);
	gpio_export(wlan_pwr_on, 1);

	msleep(WIFI_TURNON_DELAY);

	/* ========== WLAN_HOST_WAKE ============ */
	wlan_host_wake_up = of_get_named_gpio(root_node, "wlan_host_wake", 0);
	if (!gpio_is_valid(wlan_host_wake_up)) {
		printk("Invalied gpio pin : %d\n", wlan_host_wake_up);
		gpio_free(wlan_pwr_on);
		return -ENODEV;
	}

	ret = gpio_request(wlan_host_wake_up, "WLAN_HOST_WAKE");
	if (ret < 0) {
		printk("fail to request gpio(WLAN_REG_ON)\n", ret);
		return -ENODEV;
	}

	gpio_direction_input(wlan_host_wake_up);
	gpio_export(wlan_host_wake_up, 1);

	wlan_host_wake_irq = gpio_to_irq(wlan_host_wake_up);

	return 0;
}


void
interrupt_set_cpucore(int set)
{
	printk(KERN_INFO "%s: set: %d\n", __FUNCTION__, set);
	if (set) {

	}
}

struct resource dhd_wlan_resources = {
	.name	= "bcmdhd_wlan_irq",
	.start	= 0,
	.end	= 0,
	.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_SHAREABLE |
#ifdef CONFIG_BCMDHD_PCIE
	IORESOURCE_IRQ_HIGHEDGE,
#else
	IORESOURCE_IRQ_HIGHLEVEL,
#endif /* CONFIG_BCMDHD_PCIE */
};
EXPORT_SYMBOL(dhd_wlan_resources);

struct wifi_platform_data dhd_wlan_control = {
	.set_power	= dhd_wlan_power,
	.set_reset	= dhd_wlan_reset,
#ifndef CONFIG_BCMDHD_PCIE
	.set_carddetect	= dhd_wlan_set_carddetect,
#endif /* !CONFIG_BCMDHD_PCIE */
#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	.mem_prealloc	= dhd_wlan_mem_prealloc,
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */
};
EXPORT_SYMBOL(dhd_wlan_control);

int __init
dhd_wlan_init(void)
{
	int ret;

	printk(KERN_INFO "%s: START.......\n", __FUNCTION__);
	ret = dhd_wlan_init_gpio();
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to initiate GPIO, ret=%d\n",
			__FUNCTION__, ret);
		goto fail;
	}

	dhd_wlan_resources.start = wlan_host_wake_irq;
	dhd_wlan_resources.end = wlan_host_wake_irq;

#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	ret = dhd_init_wlan_mem();
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to alloc reserved memory,"
			" ret=%d\n", __FUNCTION__, ret);
	}
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */

fail:
	return ret;
}

void __exit dhd_wlan_exit(void)
{
	gpio_free(wlan_pwr_on);
	gpio_free(wlan_host_wake_up);
	printk(KERN_INFO "%s: exit\n", __FUNCTION__);
}


