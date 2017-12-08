/*
* usb_charge.h -- USB Charge driver
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

#ifndef _USB_CHARGE_H_
#define _USB_CHARGE_H_

#define UCD_DRV_MAX_COUNT 5
#define UCD_USB_BUS_NUM 1

enum ucd_event_type {
	E_UCD_EVENT_CHARGE_DEVICE_TYPE = 0,
	E_UCD_EVENT_CHARGE_NORMAL_MODE,
	E_UCD_EVENT_CHARGE_QUICK_MODE,
	E_UCD_EVENT_CHARGE_GET_MODE,
	E_UCD_EVENT_CHARGE_GET_CURRENT
};

enum ucd_charge_device_type {
	E_UCD_CHARGE_DEVICE_NONE = 0,
	E_UCD_CHARGE_DEVICE_ANDROID,
	E_UCD_CHARGE_DEVICE_APPLE
};

struct ucd_device {
	struct device *dev;
	struct regulator *reg;
	struct work_struct event_work;
	struct workqueue_struct *workqueue;

	enum ucd_charge_device_type device_type;

	struct mutex lock;

	int suspend_flag;
	int quick_charge_mode;

	void *data;

	struct usb_device_id *id;
};

struct ucd_attrs {
	struct device_attribute **attr;
	int num;
};

struct ucd_driver {
	char *device_name;
	int (*local_init)(void);
	void (*suspend)(struct device *h);
	void (*resume)(struct device *h);
	void (*event)(int event, int value);
	struct ucd_attrs attrs;
};

extern struct ucd_device *ucd;
extern bool ucd_debug_on;

#define TAG "<<<<<<usb charge>>>>>>"
#define UCD_DBG_FUNC(fmt, arg...)    do {\
		if (ucd_debug_on)\
			pr_notice(TAG "[%s] " fmt "\n", __func__, ##arg);\
		} while (0)

#define UCD_INFO(fmt, arg...) pr_info(TAG " [%s] " fmt "\n", __func__, ##arg)
/* #define UCD_DEBUG */

#ifdef UCD_DEBUG
#define UCD_DBG  UCD_DBG_FUNC
#define UCD_ERR(fmt, arg...) pr_err(TAG "[%s] " fmt "\n", __func__, ##arg)
#else
#define UCD_DBG
#define UCD_ERR(fmt, arg...) pr_err(TAG "[%s] " fmt "\n", __func__, ##arg)
#endif

extern int ucd_driver_add(struct ucd_driver *ucd_drv);
extern int ucd_driver_remove(struct ucd_driver *ucd_drv);

extern bool ucd_charge_device_android(void);
extern bool ucd_charge_device_apple(void);
extern bool ucd_quick_charge_mode(void);
extern void ucd_set_device_type(enum ucd_charge_device_type type);
extern bool ucd_charge_device_none(void);

#endif

