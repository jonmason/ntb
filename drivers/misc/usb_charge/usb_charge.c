/*
* usb_charge.c -- USB Charge driver
*
* Copyright (C) 2017 ZhongHong.
* Author:
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

#include <linux/string.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>

#include <linux/device.h>
#include <linux/regulator/consumer.h>
#include <linux/miscdevice.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#include <linux/fs.h>

#include <linux/fb.h>
#include <linux/suspend.h>

#include "usb_charge.h"

#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/usb.h>

bool ucd_debug_on = true;
module_param(ucd_debug_on, bool, 0664);

#define UCD_IOC_MAGIC 'U'
#define UCD_IOC_QUICK_CHARGE _IO(UCD_IOC_MAGIC, 0)
#define UCD_IOC_NORMAL_CHARGE _IO(UCD_IOC_MAGIC, 1)

struct ucd_device *ucd;
static struct ucd_driver *g_ucd_drv;
static struct ucd_driver ucd_driver_list[UCD_DRV_MAX_COUNT];
static struct notifier_block ucd_fb_notifier;
static struct work_struct ucd_resume_work;
static struct workqueue_struct *ucd_resume_workqueue;

#ifdef CONFIG_OF
static  struct of_device_id usb_charge_of_match[] = {
	{.compatible = "mediatek,usb_charge", },
	{ }
};
MODULE_DEVICE_TABLE(of, usb_charge_of_match);
#endif

#define USB_APPLE_VENDOR_ID 0x05AC
#define USB_APPLE_PRODUCT_ID 0x12A8
#define APPLE_DEVICE(prod) \
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE | \
					USB_DEVICE_ID_MATCH_INT_CLASS | \
					USB_DEVICE_ID_MATCH_INT_PROTOCOL, \
	.idVendor = USB_APPLE_VENDOR_ID, \
	.idProduct = (prod), \
	.bInterfaceClass = USB_CLASS_STILL_IMAGE, \
	.bInterfaceProtocol = 0x01

#define ANDROID_DEVICE \
	.match_flags = USB_DEVICE_ID_MATCH_INT_CLASS, \
	.bInterfaceClass = USB_CLASS_MASS_STORAGE \

static const struct usb_device_id ucd_device_ids[] = {
	{ APPLE_DEVICE(USB_APPLE_PRODUCT_ID) },
	/* {ANDROID_DEVICE}, */
	/* Terminating entry */
	{ }
};
MODULE_DEVICE_TABLE(usb, ucd_device_ids);

void ucd_set_device_type(enum ucd_charge_device_type type)
{
	mutex_lock(&ucd->lock);
	switch (type) {
	case E_UCD_CHARGE_DEVICE_APPLE:
		ucd->device_type = E_UCD_CHARGE_DEVICE_APPLE;
		break;
	case E_UCD_CHARGE_DEVICE_ANDROID:
		ucd->device_type = E_UCD_CHARGE_DEVICE_ANDROID;
		break;
	default:
		ucd->device_type = E_UCD_CHARGE_DEVICE_NONE;
		break;
	}
	mutex_unlock(&ucd->lock);

	if (g_ucd_drv && g_ucd_drv->event)
		g_ucd_drv->event(E_UCD_EVENT_CHARGE_DEVICE_TYPE,
			ucd->device_type);
	/* queue_work(ucd->workqueue,&ucd->event_work); */
}

bool ucd_charge_device_apple(void)
{
	bool ret = false;

	mutex_lock(&ucd->lock);
	if (ucd->device_type == E_UCD_CHARGE_DEVICE_APPLE)
		ret = true;
	mutex_unlock(&ucd->lock);

	return ret;
}
bool ucd_charge_device_android(void)
{
	bool ret = false;

	mutex_lock(&ucd->lock);
	if (ucd->device_type == E_UCD_CHARGE_DEVICE_ANDROID)
		ret = true;
	mutex_unlock(&ucd->lock);

	return ret;
}
bool ucd_charge_device_none(void)
{
	bool ret = false;

	mutex_lock(&ucd->lock);
	if (ucd->id) {
		if (ucd->id->bInterfaceClass == USB_CLASS_MASS_STORAGE)
			ret = true;
	}
	mutex_unlock(&ucd->lock);

	return ret;
}
bool ucd_quick_charge_mode(void)
{
	bool ret = false;

	mutex_lock(&ucd->lock);
	if (ucd->quick_charge_mode)
		ret = true;
	mutex_unlock(&ucd->lock);

	return ret;
}

void ucd_set_quick_charge_mode(int value)
{
	UCD_DBG("set quick mode:%d", value);
	if (value) {
		mutex_lock(&ucd->lock);
		ucd->quick_charge_mode = true;
		mutex_unlock(&ucd->lock);
		if (g_ucd_drv && g_ucd_drv->event)
			g_ucd_drv->event(E_UCD_EVENT_CHARGE_QUICK_MODE, 1);
	} else {
		mutex_lock(&ucd->lock);
		ucd->quick_charge_mode = false;
		mutex_unlock(&ucd->lock);
		if (g_ucd_drv && g_ucd_drv->event)
			g_ucd_drv->event(E_UCD_EVENT_CHARGE_NORMAL_MODE, 1);
	}
}

static void ucd_event_work_func(struct work_struct *work)
{
	int ret;
	int len = 0;
	char udev_event1[64];
	char udev_event2[64];
	char udev_event3[64];
	char udev_event4[64];
	char *envp[] = {udev_event1, udev_event2,
					udev_event3, udev_event4, NULL};

	if (ucd_charge_device_apple())
		snprintf(udev_event1, 64, "DEVTYPE=%s", "APPLE");
	else if (ucd_charge_device_android())
		snprintf(udev_event1, 64, "DEVTYPE=%s", "ANDROID");
	else
		snprintf(udev_event1, 64, "DEVTYPE=%s", "NONE");

	snprintf(udev_event2, 64, "INTERFACE_PATH=/dev/ucd_interface");

	if (g_ucd_drv && g_ucd_drv->event)
		g_ucd_drv->event(E_UCD_EVENT_CHARGE_GET_MODE, udev_event3);

	if (g_ucd_drv && g_ucd_drv->event)
		g_ucd_drv->event(E_UCD_EVENT_CHARGE_GET_CURRENT, udev_event4);

	ret = kobject_uevent_env(&ucd->dev->kobj, KOBJ_CHANGE, envp);
	if (ret < 0)
		UCD_DBG("uevent sending failed with ret = %d\n", ret);
}


static void ucd_set_device_data(void *data)
{
	if (ucd)
		ucd->data = data;
}

static int ucd_usb_probe(struct usb_interface *intf,
	const struct usb_device_id *id)
{
	struct usb_device *usb_dev = interface_to_usbdev(intf);

	if (!usb_dev || !ucd)
		return -1;

	UCD_DBG("usb_dev busnum:%d", usb_dev->bus->busnum);

	if (ucd->suspend_flag)
		return -1;

	if (usb_dev->bus->busnum == UCD_USB_BUS_NUM) {
		ucd->id = id;
		if (id->idVendor == USB_APPLE_VENDOR_ID) {
			UCD_DBG("apple device connect");
			ucd_set_device_data(intf);
			ucd_set_device_type(E_UCD_CHARGE_DEVICE_APPLE);
		} else if (id->bInterfaceClass == USB_CLASS_VENDOR_SPEC) {
			UCD_DBG("android device connect");
			ucd_set_device_data(intf);
			ucd_set_device_type(E_UCD_CHARGE_DEVICE_ANDROID);
			return -1;
		} else if (id->bInterfaceClass == USB_CLASS_MASS_STORAGE) {
			UCD_DBG("disk device connect");
			ucd_set_device_data(intf);
			ucd_set_device_type(E_UCD_CHARGE_DEVICE_NONE);
			return -1;
		}
	} else {
		return -1;
	}

	return 0;
}
static void ucd_usb_disconnect(struct usb_interface *intf)
{
	struct usb_device *usb_dev = interface_to_usbdev(intf);

	if (!usb_dev || !ucd)
		return;

	UCD_DBG("usb_dev busnum:%d", usb_dev->bus->busnum);

	if (usb_dev->bus->busnum == UCD_USB_BUS_NUM) {
		if (ucd->data && intf == ucd->data) {
			ucd->id = NULL;
			if (ucd_charge_device_apple())
				UCD_DBG("apple device disconnect");
			else if (ucd_charge_device_android())
				UCD_DBG("android device disconnect");
			else if (ucd_charge_device_none())
				UCD_DBG("disk device disconnect");

			ucd_set_device_data(NULL);
			/* ucd_set_device_type(E_UCD_CHARGE_DEVICE_NONE); */
		}
	}
}
static int ucd_usb_suspend(struct usb_interface *intf, pm_message_t message)
{
	return 0;
}
static int ucd_usb_resume(struct usb_interface *intf)
{
	return 0;
}
static int ucd_usb_reset_resume(struct usb_interface *intf)
{
	return 0;
}

static struct usb_driver ucd_usb_driver  = {
	.name		= "ucd_usb",
	.probe		= ucd_usb_probe,
	.disconnect	= ucd_usb_disconnect,
	.suspend		= ucd_usb_suspend,
	.resume		= ucd_usb_resume,
	.reset_resume	= ucd_usb_reset_resume,
	.id_table	= ucd_device_ids,
	.supports_autosuspend = 1,
};

static int ucd_usb_init(void)
{
	int ret;

	UCD_DBG("************ usb init***************");

	ret = usb_register(&ucd_usb_driver);
	if (ret < 0)
		return ret;

	return 0;
}

static int ucd_usb_exit(void)
{
	usb_deregister(&ucd_usb_driver);
	return 0;
}

static long ucd_unlocked_ioctl(struct file *file,
	unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	int tmp = 0;

	UCD_DBG("ucd_unlocked_ioctl:cmd=%d\n", cmd);
	switch (cmd) {
	case UCD_IOC_QUICK_CHARGE:
		if (copy_from_user(&tmp, (int __user *)arg,
			sizeof(int)))
			return -EFAULT;
		if (tmp)
			ucd_set_quick_charge_mode(1);
		break;
	case UCD_IOC_NORMAL_CHARGE:
		if (copy_from_user(&tmp, (int __user *)arg,
			sizeof(int)))
			return -EFAULT;
		if (tmp)
			ucd_set_quick_charge_mode(0);
		break;
	default:
		break;
	}

	return ret;
}

static int ucd_open(struct inode *inode, struct file *filp)
{
	UCD_DBG("ucd_open\n");
	return 0;
}

static int ucd_release(struct inode *inode, struct file *filp)
{
	UCD_DBG("ucd_Release\n");
	return 0;
}

static struct file_operations ucd_fops = {
	.owner = THIS_MODULE,
	.open = ucd_open,
	.release = ucd_release,
	.unlocked_ioctl = ucd_unlocked_ioctl,
};

static struct miscdevice ucd_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ucd_interface",
	.fops = &ucd_fops,
};

static void ucd_resume_work_func(struct work_struct *work)
{
	UCD_DBG("%s", __func__);

	ucd_set_device_type(ucd->device_type);
	g_ucd_drv->resume(NULL);
	ucd->suspend_flag = 0;
}

void ucd_core_suspend(void)
{
	int err = 0;

	UCD_DBG("----");
	ucd_set_device_type(E_UCD_CHARGE_DEVICE_NONE);
	if (g_ucd_drv && !ucd->suspend_flag) {
		err = cancel_work_sync(&ucd_resume_work);
		if (!err)
			UCD_DBG("cancel usb charge resume_work err = %d\n",
					err);
		g_ucd_drv->suspend(NULL);
	}
	ucd->suspend_flag = 1;
}
EXPORT_SYMBOL(ucd_core_suspend);

void ucd_core_resume(void)
{
	int err = 0;

	UCD_DBG("----");
	if (g_ucd_drv && ucd->suspend_flag) {
		err = queue_work(ucd_resume_workqueue, &ucd_resume_work);
		if (!err) {
			UCD_DBG("start touch_resume_workqueue failed\n");
			return;
		}
	}
}
EXPORT_SYMBOL(ucd_core_resume);

static int ucd_fb_notifier_callback(struct notifier_block *self,
	unsigned long event, void *data)
{
	struct fb_event *evdata = NULL;
	int blank;

	evdata = data;
	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;
	UCD_DBG("fb_notify(blank=%d)\n", blank);

	switch (blank) {
	case FB_BLANK_UNBLANK:
		UCD_DBG("LCD ON Notify\n");
		ucd_core_resume();
		break;

	case FB_BLANK_POWERDOWN:
		UCD_DBG("LCD OFF Notify\n");
		ucd_core_suspend();
		break;

	default:
		break;
	}
	return 0;
}


int ucd_driver_add(struct ucd_driver *ucd_drv)
{
	int i;

	if (g_ucd_drv != NULL) {
		UCD_DBG("usb charge driver exist\n");
		return -1;
	}

	/* check parameter */
	if (ucd_drv == NULL)
		return -1;

	for (i = 0; i < UCD_DRV_MAX_COUNT; i++) {
		/* add tpd driver into list */
		if (ucd_driver_list[i].device_name == NULL) {
			ucd_driver_list[i].device_name = ucd_drv->device_name;
			ucd_driver_list[i].local_init = ucd_drv->local_init;
			ucd_driver_list[i].suspend = ucd_drv->suspend;
			ucd_driver_list[i].resume = ucd_drv->resume;
			ucd_driver_list[i].event = ucd_drv->event;
			ucd_driver_list[i].attrs = ucd_drv->attrs;
			break;
		}

		if (strcmp(ucd_driver_list[i].device_name,
			ucd_drv->device_name) == 0)
			return 1;	/* driver exist */
	}

	return 0;
}

int ucd_driver_remove(struct ucd_driver *ucd_drv)
{
	int i = 0;

	/* check parameter */
	if (ucd_drv == NULL)
		return -1;

	for (i = 0; i < UCD_DRV_MAX_COUNT; i++) {
		/* find it */
		if (strcmp(ucd_driver_list[i].device_name,
			ucd_drv->device_name) == 0) {
			memset(&ucd_driver_list[i], 0,
				sizeof(struct ucd_driver));
			break;
		}
	}

	return 0;
}

static void ucd_create_attributes(struct device *dev, struct ucd_attrs *attrs)
{
	int num = attrs->num;

	for (; num > 0;)
		device_create_file(dev, attrs->attr[--num]);
}

static int ucd_local_init(void)
{
	int i;

	for (i = 0; i < UCD_DRV_MAX_COUNT; i++) {
		/* add tpd driver into list */
		if (ucd_driver_list[i].device_name != NULL) {
			if (!ucd_driver_list[i].local_init()) {
				UCD_DBG("ucd_local_init, ucd_driver_name=%s\n",
					ucd_driver_list[i].device_name);
				g_ucd_drv = &ucd_driver_list[i];
				break;
			}
		}
	}

	return (g_ucd_drv == NULL) ? -1:0;
}

static int ucd_probe(struct platform_device *pdev)
{
	ucd = kmalloc(sizeof(struct ucd_device), GFP_KERNEL);
	if (ucd == NULL)
		return -ENOMEM;
	memset(ucd, 0, sizeof(struct ucd_device));

	ucd->dev = &pdev->dev;

	mutex_init(&ucd->lock);

	ucd->workqueue = create_singlethread_workqueue("ucd_workqueue");
	INIT_WORK(&ucd->event_work, ucd_event_work_func);

	ucd_set_device_type(E_UCD_CHARGE_DEVICE_NONE);

	if (ucd_local_init())
		return -1;

	if (g_ucd_drv->attrs.num)
		ucd_create_attributes(&pdev->dev, &g_ucd_drv->attrs);

	ucd_resume_workqueue = create_singlethread_workqueue("ucd_resume");
	INIT_WORK(&ucd_resume_work, ucd_resume_work_func);

	/* use fb_notifier */
	ucd_fb_notifier.notifier_call = ucd_fb_notifier_callback;
	if (fb_register_client(&ucd_fb_notifier))
		UCD_DBG("register fb_notifier fail!\n");

	ucd_usb_init();

	return 0;
}

static int ucd_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver ucd_driver = {
	.probe = ucd_probe,
	.remove = ucd_remove,
	.driver = {
		.name = "usb_charge",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(usb_charge_of_match),
#endif
	}
};

static int __init ucd_device_init(void)
{
	int ret = 0;

	/*platform driver register*/
	ret = platform_driver_register(&ucd_driver);
	if (ret) {
		UCD_DBG("[%s:%d]:usb charge platform driver register error!\n",
			__func__, __LINE__);
	}
	misc_register(&ucd_miscdev);

	return ret;
}

static void __exit ucd_device_exit(void)
{
	ucd_usb_exit();

	misc_deregister(&ucd_miscdev);

	/*platform driver remove*/
	platform_driver_unregister(&ucd_driver);
}

late_initcall(ucd_device_init);
module_exit(ucd_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhonghong");
MODULE_DESCRIPTION("usb charge  Driver");
