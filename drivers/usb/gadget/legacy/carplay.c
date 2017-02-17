/*
 * carplay.c -- Apple carplay gadget driver
 *
 * Copyright (C) 2016 Nexell, Inc.
 * Author: Hyunseok Jung <hsjung@nexell.co.kr>
 *
 * The driver borrows from ether.c which is:
 *
 * Copyright (C) 2003-2005,2008 David Brownell
 * Copyright (C) 2003-2004 Robert Schwebel, Benedikt Spranger
 * Copyright (C) 2008 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/usb/composite.h>

#include "u_ether.h"
#include "u_carplay.h"

#define DRIVER_DESC		"Carplay Gadget"

/*-------------------------------------------------------------------------*/

/* DO NOT REUSE THESE IDs with a protocol-incompatible driver!!  Ever!!
 * Instead:  allocate your own, using normal USB-IF procedures.
 */

/* Thanks to NetChip Technologies for donating this product ID.
 * It's for devices with only CDC Ethernet configurations.
 */
#define CDC_VENDOR_NUM		0x0525	/* NetChip */
#define CDC_PRODUCT_NUM		0xa4a1	/* Linux-USB Ethernet Gadget */

/*-------------------------------------------------------------------------*/
USB_GADGET_COMPOSITE_OPTIONS();

USB_ETHERNET_MODULE_PARAMETERS();

static struct usb_device_descriptor device_desc = {
	.bLength =		sizeof(device_desc),
	.bDescriptorType =	USB_DT_DEVICE,

	.bcdUSB =		cpu_to_le16 (0x0200),

	.bDeviceClass =		USB_CLASS_PER_INTERFACE,
	.bDeviceSubClass =	0,
	.bDeviceProtocol =	0,
	/* .bMaxPacketSize0 = f(hardware) */

	/* Vendor and product id defaults change according to what configs
	 * we support.  (As does bNumConfigurations.)  These values can
	 * also be overridden by module parameters.
	 */
	.idVendor =		cpu_to_le16 (CDC_VENDOR_NUM),
	.idProduct =		cpu_to_le16 (CDC_PRODUCT_NUM),
	/* .bcdDevice = f(hardware) */
	/* .iManufacturer = DYNAMIC */
	/* .iProduct = DYNAMIC */
	/* NO SERIAL NUMBER */
	.bNumConfigurations =	1,
};

static struct usb_otg_descriptor otg_descriptor = {
	.bLength =		sizeof(otg_descriptor),
	.bDescriptorType =	USB_DT_OTG,

	/* REVISIT SRP-only hardware is possible, although
	 * it would not be called "OTG" ...
	 */
	.bmAttributes =		USB_OTG_SRP | USB_OTG_HNP,
};

static const struct usb_descriptor_header *otg_desc[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	NULL,
};


/* string IDs are assigned dynamically */

#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX			1
#define STRING_SERIAL_IDX           2
#define STRING_INTERFACE_IDX        3
#define STRING_INTERFACE_IDX2       4
#define STRING_INTERFACE_IDX3       5
#define STRING_MACADDRESS_IDX       6

static char manufacturer_string[256];
static char product_string[256];
static char serial_string[256];
static char interface_string[256];
static char interface_string2[256];
static char interface_string3[256];
static char macaddress_string[256];

/* string IDs are assigned dynamically */
static struct usb_string strings_dev[] = {
	[STRING_MANUFACTURER_IDX].s = manufacturer_string,
	[STRING_PRODUCT_IDX].s = product_string,
	[STRING_SERIAL_IDX].s = serial_string,
	[STRING_INTERFACE_IDX].s = interface_string,
	[STRING_INTERFACE_IDX2].s = interface_string2,
	[STRING_INTERFACE_IDX3].s = interface_string3,
	[STRING_MACADDRESS_IDX].s = macaddress_string,
	{  } /* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_function_instance *f_carplay_inst;
static struct usb_function *f_carplay;

/*-------------------------------------------------------------------------*/

static int carplay_do_config(struct usb_configuration *c)
{
	int status;

	/* FIXME alloc iConfiguration string, set it in c->strings */

	if (gadget_is_otg(c->cdev->gadget)) {
		c->descriptors = otg_desc;
		c->bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}

	f_carplay_inst = usb_get_function(f_carplay_inst);
	if (IS_ERR(f_carplay)) {
		status = PTR_ERR(f_carplay);
		return status;
	}

	status = usb_add_function(c, f_carplay);
	if (status < 0) {
		usb_put_function(f_carplay);
		return status;
	}

	return 0;
}

static struct usb_configuration carplay_config_driver = {
	/* .label = f(hardware) */
	.label			= "Carplay (IAP+NCM)",
	.bConfigurationValue	= 1,
	/* .iConfiguration	= DYNAMIC */
	.bmAttributes		= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.MaxPower		= 0, /* selfpowered */
};

/*-------------------------------------------------------------------------*/

static int gcarplay_bind(struct usb_composite_dev *cdev)
{
	struct usb_gadget	*gadget = cdev->gadget;
	struct f_carplay_opts	*carplay_opts;
	int			status;

	f_carplay_inst = usb_get_function_instance("carplay");
	if (IS_ERR(f_carplay_inst))
		return PTR_ERR(f_carplay_inst);

	carplay_opts = container_of(f_carplay_inst, struct f_carplay_opts,
				    func_inst);
	gether_set_qmult(carplay_opts->net, qmult);
	if (!gether_set_host_addr(carplay_opts->net, host_addr))
		pr_info("using host ethernet address: %s", host_addr);
	if (!gether_set_dev_addr(carplay_opts->net, dev_addr))
		pr_info("using self ethernet address: %s", dev_addr);


	iap_setup();

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */

	device_desc.bcdDevice = cpu_to_le16(0x0300 | 0x33);

	/* device descriptor strings: manufacturer, product */
	snprintf(manufacturer_string, sizeof(manufacturer_string),
		 "%s %s with %s", init_utsname()->sysname,
		 init_utsname()->release, gadget->name);
	status = usb_string_id(cdev);
	if (status < 0)
		goto fail;
	strings_dev[STRING_MANUFACTURER_IDX].id = status;
	device_desc.iManufacturer = status;

	status = usb_string_id(cdev);
	if (status < 0)
		goto fail;
	strings_dev[STRING_PRODUCT_IDX].id = status;
	device_desc.iProduct = status;

	status = usb_string_id(cdev);
	if (status < 0)
		goto fail;
	strings_dev[STRING_SERIAL_IDX].id = status;
	device_desc.iSerialNumber = status;

	strings_dev[STRING_INTERFACE_IDX].id = 4;
	strings_dev[STRING_INTERFACE_IDX2].id = 5;
	strings_dev[STRING_INTERFACE_IDX3].id = 6;
	strings_dev[STRING_MACADDRESS_IDX].id = 7;

	/* Default strings - should be updated by userspace */
	strncpy(manufacturer_string, "Apple", sizeof(manufacturer_string) - 1);
	strncpy(product_string, "Carplay", sizeof(product_string) - 1);
	strncpy(serial_string, "0123456789ABCDEF", sizeof(serial_string) - 1);
	strncpy(interface_string, "iAP Interface",
		sizeof(interface_string) - 1);
	strncpy(interface_string2, "CDC NCM Comm Interface",
		sizeof(interface_string2) - 1);
	strncpy(interface_string3, "CDC NCM Data Interface",
		sizeof(interface_string3) - 1);
	strncpy(macaddress_string, "9A3BFC66E845",
		sizeof(macaddress_string) - 1);

	status = usb_add_config(cdev, &carplay_config_driver,
				carplay_do_config);
	if (status < 0)
		goto fail;

	dev_info(&gadget->dev, "%s\n", DRIVER_DESC);

	return 0;

fail:
	usb_put_function_instance(f_carplay_inst);
	return status;
}

static int gcarplay_unbind(struct usb_composite_dev *cdev)
{
	if (!IS_ERR_OR_NULL(f_carplay_inst))
		usb_put_function_instance(f_carplay_inst);
	return 0;
}

static struct usb_composite_driver carplay_driver = {
	.name		= "g_carplay",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.max_speed	= USB_SPEED_HIGH,
	.bind		= gcarplay_bind,
	.unbind		= gcarplay_unbind,
};

module_usb_composite_driver(carplay_driver);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Hyunseok Jung");
MODULE_LICENSE("GPL");
