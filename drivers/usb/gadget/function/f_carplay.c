/*
 * Gadget Driver for Apple IAP
 * (for device mode of accessory device :IAP version 2)
 *
 * Copyright (C) 2016 Nexell, Inc.
 * Author: Hyunseok Jung <hsjung@nexell.co.kr>
 *
 * The driver borrows from f_ecm.c which is:
 *
 * Copyright (C) 2003-2005,2008 David Brownell
 * Copyright (C) 2008 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/etherdevice.h>
#include <linux/crc32.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/miscdevice.h>

#include <linux/usb/cdc.h>

#include "u_ether.h"
#include "u_ether_configfs.h"
#include "u_carplay.h"

#define IAP_BULK_BUFFER_SIZE           4096

/* number of tx requests to allocate */
#define TX_REQ_MAX 4

/*
 * This function is a "CDC Network Control Model" (CDC NCM) Ethernet link.
 * NCM is intended to be used with high-speed network attachments.
 *
 * Note that NCM requires the use of "alternate settings" for its data
 * interface.  This means that the set_alt() method has real work to do,
 * and also means that a get_alt() method is required.
 */

/* to trigger crc/non-crc ndp signature */

#define NCM_NDP_HDR_CRC_MASK	0x01000000
#define NCM_NDP_HDR_CRC		0x01000000
#define NCM_NDP_HDR_NOCRC	0x00000000

enum ncm_notify_state {
	NCM_NOTIFY_NONE,		/* don't notify */
	NCM_NOTIFY_CONNECT,		/* issue CONNECT next */
	NCM_NOTIFY_SPEED,		/* issue SPEED_CHANGE next */
};

static struct class *android_class;

static const char iap_shortname[] = "android_iap";

struct iap_dev {
	struct usb_function	function;
	struct usb_composite_dev *cdev;
	spinlock_t		lock;
	/*
	 * for notification, it is accessed from both
	 * callback and ethernet open/close
	 */
	spinlock_t		ncm_lock;

	struct usb_ep		*ep_in;
	struct usb_ep		*ep_out;

	int			online;
	int			error;

	atomic_t		read_excl;
	atomic_t		write_excl;
	atomic_t		open_excl;

	struct list_head	tx_idle;

	wait_queue_head_t	read_wq;
	wait_queue_head_t	write_wq;
	struct usb_request	*rx_req;
	int			rx_done;

	struct gether		port;
	u8			ctrl_id, data_id;

	char			ethaddr[14];

	struct usb_ep           *notify;
	struct usb_request      *notify_req;
	u8			notify_state;
	bool			is_open;

	struct ndp_parser_opts	*parser_opts;
	bool			is_crc;
	struct device		*dev;
};

/*-------------------------------------------------------------------------*/

/*
 * We cannot group frames so use just the minimal size which ok to put
 * one max-size ethernet frame.
 * If the host can group frames, allow it to do that, 16K is selected,
 * because it's used by default by the current linux host driver
 */
#define NTB_DEFAULT_IN_SIZE USB_CDC_NCM_NTB_MIN_IN_SIZE
#define NTB_OUT_SIZE        16384

/*
 * skbs of size less than that will not be aligned
 * to NCM's dwNtbInMaxSize to save bus bandwidth
 */

#define MAX_TX_NONFIXED     (512 * 3)

#define FORMATS_SUPPORTED   (USB_CDC_NCM_NTB16_SUPPORTED |  \
			     USB_CDC_NCM_NTB32_SUPPORTED)

static struct usb_cdc_ncm_ntb_parameters ntb_parameters = {
	.wLength = sizeof(ntb_parameters),
	.bmNtbFormatsSupported = cpu_to_le16(FORMATS_SUPPORTED),
	.dwNtbInMaxSize = cpu_to_le32(NTB_DEFAULT_IN_SIZE),
	.wNdpInDivisor = cpu_to_le16(4),
	.wNdpInPayloadRemainder = cpu_to_le16(0),
	.wNdpInAlignment = cpu_to_le16(4),

	.dwNtbOutMaxSize = cpu_to_le32(NTB_OUT_SIZE),
	.wNdpOutDivisor = cpu_to_le16(4),
	.wNdpOutPayloadRemainder = cpu_to_le16(0),
	.wNdpOutAlignment = cpu_to_le16(4),
};

/*
 * Use wMaxPacketSize big enough to fit CDC_NOTIFY_SPEED_CHANGE in one
 * packet, to simplify cancellation; and a big transfer interval, to
 * waste less bandwidth.
 */

#define LOG2_STATUS_INTERVAL_MSEC	5	/* 1 << 5 == 32 msec */
#define NCM_STATUS_BYTECOUNT		16	/* 8 byte header + data */

static struct usb_interface_descriptor iap_interface_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bNumEndpoints          = 2,
	.bInterfaceClass        = 0xff,
	.bInterfaceSubClass     = 0xf0,
	.bInterfaceProtocol     = 0,
};

static struct usb_interface_assoc_descriptor ncm_iad_desc = {
	.bLength		= sizeof(ncm_iad_desc),
	.bDescriptorType	= USB_DT_INTERFACE_ASSOCIATION,

	/* .bFirstInterface	= DYNAMIC, */
	.bInterfaceCount	= 2,  /* control + data */
	.bFunctionClass		= USB_CLASS_COMM,
	.bFunctionSubClass	= USB_CDC_SUBCLASS_NCM,
	.bFunctionProtocol	= USB_CDC_PROTO_NONE,
	/* .iFunction		= DYNAMIC */
};

/* interface descriptor: */

static struct usb_interface_descriptor ncm_control_intf = {
	.bLength		= sizeof(ncm_control_intf),
	.bDescriptorType	= USB_DT_INTERFACE,

	/* .bInterfaceNumber	= DYNAMIC */
	.bInterfaceNumber	= 1,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_COMM,
	.bInterfaceSubClass	= USB_CDC_SUBCLASS_NCM,
	.bInterfaceProtocol	= USB_CDC_PROTO_NONE,
	/* .iInterface		= DYNAMIC */
};

static struct usb_cdc_header_desc ncm_header_desc = {
	.bLength		= sizeof(ncm_header_desc),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= USB_CDC_HEADER_TYPE,

	.bcdCDC			= cpu_to_le16(0x0110),
};

static struct usb_cdc_union_desc ncm_union_desc = {
	.bLength		= sizeof(ncm_union_desc),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= USB_CDC_UNION_TYPE,
	.bMasterInterface0	= 1,
	.bSlaveInterface0	= 2,
};

static struct usb_cdc_ether_desc ecm_desc = {
	.bLength		= sizeof(ecm_desc),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= USB_CDC_ETHERNET_TYPE,

	/* this descriptor actually adds value, surprise! */
	.iMACAddress		= 8,
	.bmEthernetStatistics	= cpu_to_le32(0), /* no statistics */
	.wMaxSegmentSize	= cpu_to_le16(ETH_FRAME_LEN),
	.wNumberMCFilters	= cpu_to_le16(0),
	.bNumberPowerFilters	= 0,
};

#define NCAPS   (USB_CDC_NCM_NCAP_ETH_FILTER | USB_CDC_NCM_NCAP_CRC_MODE)

static struct usb_cdc_ncm_desc ncm_desc = {
	.bLength		= sizeof(ncm_desc),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= USB_CDC_NCM_TYPE,

	.bcdNcmVersion		= cpu_to_le16(0x0100),
	/* can process SetEthernetPacketFilter */
	.bmNetworkCapabilities	= NCAPS,
};

/* the default data interface has no endpoints ... */

static struct usb_interface_descriptor ncm_data_nop_intf = {
	.bLength		= sizeof(ncm_data_nop_intf),
	.bDescriptorType	= USB_DT_INTERFACE,

	.bInterfaceNumber	= 2,
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 0,
	.bInterfaceClass	= USB_CLASS_CDC_DATA,
	.bInterfaceSubClass	= 0,
	.bInterfaceProtocol	= USB_CDC_NCM_PROTO_NTB,
	/* .iInterface		= DYNAMIC */
};

/* ... but the "real" data interface has two bulk endpoints */

static struct usb_interface_descriptor ncm_data_intf = {
	.bLength		= sizeof(ncm_data_intf),
	.bDescriptorType	= USB_DT_INTERFACE,

	.bInterfaceNumber	= 2,
	.bAlternateSetting	= 1,
	.bNumEndpoints		= 2,
	.bInterfaceClass	= USB_CLASS_CDC_DATA,
	.bInterfaceSubClass	= 0,
	.bInterfaceProtocol	= USB_CDC_NCM_PROTO_NTB,
	/* .iInterface		= DYNAMIC */
};

/* full speed support: */

static struct usb_endpoint_descriptor iap_fullspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor iap_fullspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_ncm_notify_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize		= cpu_to_le16(NCM_STATUS_BYTECOUNT),
	.bInterval		= 1 << LOG2_STATUS_INTERVAL_MSEC,
};

static struct usb_endpoint_descriptor fs_ncm_in_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_ncm_out_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= 3,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *fs_iap_descs[] = {
	(struct usb_descriptor_header *) &iap_interface_desc,
	(struct usb_descriptor_header *) &iap_fullspeed_in_desc,
	(struct usb_descriptor_header *) &iap_fullspeed_out_desc,
	(struct usb_descriptor_header *) &ncm_iad_desc,
	/* CDC NCM control descriptors */
	(struct usb_descriptor_header *) &ncm_control_intf,
	(struct usb_descriptor_header *) &ncm_header_desc,
	(struct usb_descriptor_header *) &ncm_union_desc,
	(struct usb_descriptor_header *) &ecm_desc,
	(struct usb_descriptor_header *) &ncm_desc,
	(struct usb_descriptor_header *) &fs_ncm_notify_desc,
	/* data interface, altsettings 0 and 1 */
	(struct usb_descriptor_header *) &ncm_data_nop_intf,
	(struct usb_descriptor_header *) &ncm_data_intf,
	(struct usb_descriptor_header *) &fs_ncm_in_desc,
	(struct usb_descriptor_header *) &fs_ncm_out_desc,
	NULL,
};

/* high speed support: */

static struct usb_endpoint_descriptor iap_highspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __cpu_to_le16(512),
};

static struct usb_endpoint_descriptor iap_highspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_ncm_notify_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize		= cpu_to_le16(NCM_STATUS_BYTECOUNT),
	.bInterval		= LOG2_STATUS_INTERVAL_MSEC + 2,
};
static struct usb_endpoint_descriptor hs_ncm_in_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_ncm_out_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= 3,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= cpu_to_le16(512),
};

static struct usb_descriptor_header *hs_iap_descs[] = {
	(struct usb_descriptor_header *) &iap_interface_desc,
	(struct usb_descriptor_header *) &iap_highspeed_in_desc,
	(struct usb_descriptor_header *) &iap_highspeed_out_desc,
	(struct usb_descriptor_header *) &ncm_iad_desc,
	/* CDC NCM control descriptors */
	(struct usb_descriptor_header *) &ncm_control_intf,
	(struct usb_descriptor_header *) &ncm_header_desc,
	(struct usb_descriptor_header *) &ncm_union_desc,
	(struct usb_descriptor_header *) &ecm_desc,
	(struct usb_descriptor_header *) &ncm_desc,
	(struct usb_descriptor_header *) &hs_ncm_notify_desc,
	/* data interface, altsettings 0 and 1 */
	(struct usb_descriptor_header *) &ncm_data_nop_intf,
	(struct usb_descriptor_header *) &ncm_data_intf,
	(struct usb_descriptor_header *) &hs_ncm_in_desc,
	(struct usb_descriptor_header *) &hs_ncm_out_desc,
	NULL,
};

/* string IDs are assigned dynamically */

#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2
#define STRING_INTERFACE_IDX		3
#define STRING_INTERFACE_IDX2		4
#define STRING_INTERFACE_IDX3		5
#define STRING_MACADDRESS_IDX		6

/* string IDs are assigned dynamically */
static struct usb_string carplay_strings_defs[] = {
	[STRING_MANUFACTURER_IDX].s = "Apple",
	[STRING_PRODUCT_IDX].s = "Carplay",
	[STRING_SERIAL_IDX].s = "0123456789ABCDEF",
	[STRING_INTERFACE_IDX].s = "iAP Interface",
	[STRING_INTERFACE_IDX2].s = "CDC NCM Comm Interface",
	[STRING_INTERFACE_IDX3].s = "CDC NCM Data Interface",
	[STRING_MACADDRESS_IDX].s = "9A3BFC66E845",
	{  } /* end of list */
};

static struct usb_gadget_strings carplay_string_table = {
	.language	= 0x0409,	/* en-us */
	.strings	= carplay_strings_defs,
};

static struct usb_gadget_strings *carplay_strings[] = {
	&carplay_string_table,
	NULL,
};

/*
 * Here are options for NCM Datagram Pointer table (NDP) parser.
 * There are 2 different formats: NDP16 and NDP32 in the spec (ch. 3),
 * in NDP16 offsets and sizes fields are 1 16bit word wide,
 * in NDP32 -- 2 16bit words wide. Also signatures are different.
 * To make the parser code the same, put the differences in the structure,
 * and switch pointers to the structures when the format is changed.
 */

struct ndp_parser_opts {
	u32     nth_sign;
	u32     ndp_sign;
	unsigned    nth_size;
	unsigned    ndp_size;
	unsigned    ndplen_align;
	/* sizes in u16 units */
	unsigned    dgram_item_len; /* index or length */
	unsigned    block_length;
	unsigned    fp_index;
	unsigned    reserved1;
	unsigned    reserved2;
	unsigned    next_fp_index;
};

#define INIT_NDP16_OPTS {                   \
	.nth_sign = USB_CDC_NCM_NTH16_SIGN,     \
	.ndp_sign = USB_CDC_NCM_NDP16_NOCRC_SIGN,   \
	.nth_size = sizeof(struct usb_cdc_ncm_nth16),   \
	.ndp_size = sizeof(struct usb_cdc_ncm_ndp16),   \
	.ndplen_align = 4,              \
	.dgram_item_len = 1,            \
	.block_length = 1,              \
	.fp_index = 1,                  \
	.reserved1 = 0,                 \
	.reserved2 = 0,                 \
	.next_fp_index = 1,             \
}


#define INIT_NDP32_OPTS {                   \
	.nth_sign = USB_CDC_NCM_NTH32_SIGN,     \
	.ndp_sign = USB_CDC_NCM_NDP32_NOCRC_SIGN,   \
	.nth_size = sizeof(struct usb_cdc_ncm_nth32),   \
	.ndp_size = sizeof(struct usb_cdc_ncm_ndp32),   \
	.ndplen_align = 8,              \
	.dgram_item_len = 2,                \
	.block_length = 2,              \
	.fp_index = 2,                  \
	.reserved1 = 1,                 \
	.reserved2 = 2,                 \
	.next_fp_index = 2,             \
}

static struct ndp_parser_opts ndp16_opts = INIT_NDP16_OPTS;
static struct ndp_parser_opts ndp32_opts = INIT_NDP32_OPTS;

static inline void put_ncm(__le16 **p, unsigned size, unsigned val)
{
	switch (size) {
	case 1:
		put_unaligned_le16((u16)val, *p);
		break;
	case 2:
		put_unaligned_le32((u32)val, *p);

		break;
	default:
		pr_err("BUG!! %s %d\n", __func__, __LINE__);
	}

	*p += size;
}

static inline unsigned get_ncm(__le16 **p, unsigned size)
{
	unsigned tmp;

	switch (size) {
	case 1:
		tmp = get_unaligned_le16(*p);
		break;
	case 2:
		tmp = get_unaligned_le32(*p);
		break;
	default:
		pr_err("BUG!! %s %d\n", __func__, __LINE__);
	}

	*p += size;
	return tmp;
}

/* temporary variable used between iap_open() and iap_gadget_bind() */
static struct iap_dev *_iap_dev;

static inline struct iap_dev *func_to_iap(struct usb_function *f)
{
	return container_of(f, struct iap_dev, function);
}

/* peak (theoretical) bulk transfer rate in bits-per-second */
static inline unsigned ncm_bitrate(struct usb_gadget *g)
{
	if (gadget_is_dualspeed(g) && g->speed == USB_SPEED_HIGH)
		return 13 * 512 * 8 * 1000 * 8;
	else
		return 19 *  64 * 1 * 1000 * 8;
}

static struct usb_request *iap_request_new(struct usb_ep *ep, int buffer_size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);

	if (!req)
		return NULL;

	/* now allocate buffers for the requests */
	req->buf = kmalloc(buffer_size, GFP_KERNEL);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void iap_request_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

static inline int iap_lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1)
		return 0;

	atomic_dec(excl);
	return -1;
}

static inline void iap_unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

/* add a request to the tail of a list */
void iap_req_put(struct iap_dev *dev, struct list_head *head,
		struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* remove a request from the head of a list */
struct usb_request *iap_req_get(struct iap_dev *dev, struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(head)) {
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return req;
}

static void iap_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct iap_dev *dev = _iap_dev;

	if (req->status != 0)
		dev->error = 1;

	iap_req_put(dev, &dev->tx_idle, req);

	wake_up(&dev->write_wq);
}

static void iap_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct iap_dev *dev = _iap_dev;

	dev->rx_done = 1;
	if (req->status != 0 && req->status != -ECONNRESET)
		dev->error = 1;

	wake_up(&dev->read_wq);
}

/*-------------------------------------------------------------------------*/

static inline void ncm_reset_values(struct iap_dev *dev)
{
	dev->parser_opts = &ndp16_opts;
	dev->is_crc = false;
	dev->port.cdc_filter = DEFAULT_FILTER;

	/* doesn't make sense for ncm, fixed size used */
	dev->port.header_len = 0;

	dev->port.fixed_out_len = le32_to_cpu(ntb_parameters.dwNtbOutMaxSize);
	dev->port.fixed_in_len = NTB_DEFAULT_IN_SIZE;
}

/*
 * Context: ncm->lock held
 */
static void ncm_do_notify(struct iap_dev *dev)
{
	struct usb_request		*req = dev->notify_req;
	struct usb_cdc_notification	*event;
	struct usb_composite_dev	*cdev = dev->function.config->cdev;
	__le32				*data;
	int				status;

	/* notification already in flight? */
	if (!req)
		return;

	event = req->buf;
	switch (dev->notify_state) {
	case NCM_NOTIFY_NONE:
		return;

	case NCM_NOTIFY_CONNECT:
		event->bNotificationType = USB_CDC_NOTIFY_NETWORK_CONNECTION;
		if (dev->is_open)
			event->wValue = cpu_to_le16(1);
		else
			event->wValue = cpu_to_le16(0);
		event->wLength = 0;
		req->length = sizeof(*event);

		DBG(cdev, "notify connect %s\n",
				dev->is_open ? "true" : "false");
		dev->notify_state = NCM_NOTIFY_NONE;
		break;

	case NCM_NOTIFY_SPEED:
		event->bNotificationType = USB_CDC_NOTIFY_SPEED_CHANGE;
		event->wValue = cpu_to_le16(0);
		event->wLength = cpu_to_le16(8);
		req->length = NCM_STATUS_BYTECOUNT;

		/* SPEED_CHANGE data is up/down speeds in bits/sec */
		data = req->buf + sizeof(*event);
		data[0] = cpu_to_le32(ncm_bitrate(cdev->gadget));
		data[1] = data[0];

		DBG(cdev, "notify speed %d\n", ncm_bitrate(cdev->gadget));
		dev->notify_state = NCM_NOTIFY_CONNECT;
		break;
	}
	event->bmRequestType = 0xA1;
	event->wIndex = cpu_to_le16(dev->ctrl_id);

	dev->notify_req = NULL;
	/*
	 * In double buffering if there is a space in FIFO,
	 * completion callback can be called right after the call,
	 * so unlocking
	 */
	spin_unlock(&dev->ncm_lock);
	status = usb_ep_queue(dev->notify, req, GFP_ATOMIC);
	spin_lock(&dev->ncm_lock);
	if (status < 0) {
		dev->notify_req = req;
		DBG(cdev, "notify --> %d\n", status);
	}
}

/*
 * Context: ncm->lock held
 */
static void ncm_notify(struct iap_dev *dev)
{
	/*
	 * NOTE on most versions of Linux, host side cdc-ethernet
	 * won't listen for notifications until its netdevice opens.
	 * The first notification then sits in the FIFO for a long
	 * time, and the second one is queued.
	 *
	 * If ncm_notify() is called before the second (CONNECT)
	 * notification is sent, then it will reset to send the SPEED
	 * notificaion again (and again, and again), but it's not a problem
	 */
	dev->notify_state = NCM_NOTIFY_SPEED;
	ncm_do_notify(dev);
}

static void ncm_notify_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct usb_function *f = req->context;
	struct iap_dev  *dev = func_to_iap(f);
	struct usb_composite_dev    *cdev = dev->function.config->cdev;
	struct usb_cdc_notification	*event = req->buf;

	spin_lock(&dev->ncm_lock);
	switch (req->status) {
	case 0:
		VDBG(cdev, "Notification %02x sent\n",
		     event->bNotificationType);
		break;
	case -ECONNRESET:
	case -ESHUTDOWN:
		dev->notify_state = NCM_NOTIFY_NONE;
		break;
	default:
		DBG(cdev, "event %02x --> %d\n",
			event->bNotificationType, req->status);
		break;
	}
	dev->notify_req = req;
	ncm_do_notify(dev);
	spin_unlock(&dev->ncm_lock);
}

static void ncm_ep0out_complete(struct usb_ep *ep, struct usb_request *req)
{
	/* now for SET_NTB_INPUT_SIZE only */
	unsigned		in_size;
	struct usb_function	*f = req->context;
	struct iap_dev  *dev = func_to_iap(f);
	struct usb_composite_dev *cdev = ep->driver_data;

	req->context = NULL;
	if (req->status || req->actual != req->length) {
		DBG(cdev, "Bad control-OUT transfer\n");
		goto invalid;
	}

	in_size = get_unaligned_le32(req->buf);
	if (in_size < USB_CDC_NCM_NTB_MIN_IN_SIZE ||
	    in_size > le32_to_cpu(ntb_parameters.dwNtbInMaxSize)) {
		DBG(cdev, "Got wrong INPUT SIZE (%d) from host\n", in_size);
		goto invalid;
	}

	dev->port.fixed_in_len = in_size;
	VDBG(cdev, "Set NTB INPUT SIZE %d\n", in_size);
	return;

invalid:
	usb_ep_set_halt(ep);
}

/*-------------------------------------------------------------------------*/

/*
 * Callbacks let us notify the host about connect/disconnect when the
 * net device is opened or closed.
 *
 * For testing, note that link states on this side include both opened
 * and closed variants of:
 *
 *   - disconnected/unconfigured
 *   - configured but inactive (data alt 0)
 *   - configured and active (data alt 1)
 *
 * Each needs to be tested with unplug, rmmod, SET_CONFIGURATION, and
 * SET_INTERFACE (altsetting).  Remember also that "configured" doesn't
 * imply the host is actually polling the notification endpoint, and
 * likewise that "active" doesn't imply it's actually using the data
 * endpoints for traffic.
 */

static void ncm_open(struct gether *geth)
{
	struct iap_dev *dev = _iap_dev;

	DBG(dev->port.func.config->cdev, "%s\n", __func__);
	pr_info("%s\n", __func__);

	spin_lock(&dev->ncm_lock);
	dev->is_open = true;
	ncm_notify(dev);
	spin_unlock(&dev->ncm_lock);
}

static void ncm_close(struct gether *geth)
{
	struct iap_dev *dev = _iap_dev;

	DBG(dev->port.func.config->cdev, "%s\n", __func__);
	pr_info("%s\n", __func__);

	spin_lock(&dev->ncm_lock);
	dev->is_open = false;
	ncm_notify(dev);
	spin_unlock(&dev->ncm_lock);
}

static int iap_function_setup(struct usb_function *f,
			      const struct usb_ctrlrequest *ctrl)
{
	struct iap_dev	*dev = func_to_iap(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	pr_info("iap_function_setup\n");
	/*
	 * composite driver infrastructure handles everything except
	 * CDC class messages; interface activation uses set_alt().
	 */

	switch ((ctrl->bRequestType << 8) | ctrl->bRequest) {
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_SET_ETHERNET_PACKET_FILTER:
		/*
		 * see 6.2.30: no data, wIndex = interface,
		 * wValue = packet filter bitmap
		 */
		if (w_length != 0 || w_index != dev->ctrl_id)
			goto invalid;
		DBG(cdev, "packet filter %02x\n", w_value);
		/*
		 * REVISIT locking of cdc_filter.  This assumes the UDC
		 * driver won't have a concurrent packet TX irq running on
		 * another CPU; or that if it does, this write is atomic...
		 */
		dev->port.cdc_filter = w_value;
		value = 0;
		break;
	/*
	 * and optionally:
	 * case USB_CDC_SEND_ENCAPSULATED_COMMAND:
	 * case USB_CDC_GET_ENCAPSULATED_RESPONSE:
	 * case USB_CDC_SET_ETHERNET_MULTICAST_FILTERS:
	 * case USB_CDC_SET_ETHERNET_PM_PATTERN_FILTER:
	 * case USB_CDC_GET_ETHERNET_PM_PATTERN_FILTER:
	 * case USB_CDC_GET_ETHERNET_STATISTIC:
	 */

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_GET_NTB_PARAMETERS:
		if (w_length == 0 || w_value != 0 || w_index != dev->ctrl_id)
			goto invalid;
		value = w_length > sizeof(ntb_parameters) ?
			sizeof(ntb_parameters) : w_length;
		memcpy(req->buf, &ntb_parameters, value);
		VDBG(cdev, "Host asked NTB parameters\n");
		break;

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_GET_NTB_INPUT_SIZE:
		if (w_length < 4 || w_value != 0 || w_index != dev->ctrl_id)
			goto invalid;
		put_unaligned_le32(dev->port.fixed_in_len, req->buf);
		value = 4;
		VDBG(cdev, "Host asked INPUT SIZE, sending %d\n",
		     dev->port.fixed_in_len);
		break;

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_SET_NTB_INPUT_SIZE:
	{
		if (w_length != 4 || w_value != 0 || w_index != dev->ctrl_id)
			goto invalid;
		req->complete = ncm_ep0out_complete;
		req->length = w_length;
		req->context = f;

		value = req->length;
		break;
	}

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_GET_NTB_FORMAT:
	{
		uint16_t format;

		if (w_length < 2 || w_value != 0 || w_index != dev->ctrl_id)
			goto invalid;
		format = (dev->parser_opts == &ndp16_opts) ? 0x0000 : 0x0001;
		put_unaligned_le16(format, req->buf);
		value = 2;
		VDBG(cdev, "Host asked NTB FORMAT, sending %d\n", format);
		break;
	}

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_SET_NTB_FORMAT:
	{
		if (w_length != 0 || w_index != dev->ctrl_id)
			goto invalid;
		switch (w_value) {
		case 0x0000:
			dev->parser_opts = &ndp16_opts;
			DBG(cdev, "NCM16 selected\n");
			break;
		case 0x0001:
			dev->parser_opts = &ndp32_opts;
			DBG(cdev, "NCM32 selected\n");
			break;
		default:
			goto invalid;
		}
		value = 0;
		break;
	}

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_GET_CRC_MODE:
	{
		uint16_t is_crc;

		if (w_length < 2 || w_value != 0 || w_index != dev->ctrl_id)
			goto invalid;
		is_crc = dev->is_crc ? 0x0001 : 0x0000;
		put_unaligned_le16(is_crc, req->buf);
		value = 2;
		VDBG(cdev, "Host asked CRC MODE, sending %d\n", is_crc);
		break;
	}

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_SET_CRC_MODE:
	{
		int ndp_hdr_crc = 0;

		if (w_length != 0 || w_index != dev->ctrl_id)
			goto invalid;
		switch (w_value) {
		case 0x0000:
			dev->is_crc = false;
			ndp_hdr_crc = NCM_NDP_HDR_NOCRC;
			DBG(cdev, "non-CRC mode selected\n");
			break;
		case 0x0001:
			dev->is_crc = true;
			ndp_hdr_crc = NCM_NDP_HDR_CRC;
			DBG(cdev, "CRC mode selected\n");
			break;
		default:
			goto invalid;
		}
		dev->parser_opts->ndp_sign &= ~NCM_NDP_HDR_CRC_MASK;
		dev->parser_opts->ndp_sign |= ndp_hdr_crc;
		value = 0;
		break;
	}

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_GET_MAX_DATAGRAM_SIZE:
	{
		put_unaligned_le16(ETH_FRAME_LEN, req->buf);
		value = 2;
		break;
	}

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_CDC_SET_MAX_DATAGRAM_SIZE:
	{
		value = 0;
		break;
	}

	/* and disabled in ncm descriptor: */
	/* case USB_CDC_GET_NET_ADDRESS: */
	/* case USB_CDC_SET_NET_ADDRESS: */
	/* case USB_CDC_GET_MAX_DATAGRAM_SIZE: */
	/* case USB_CDC_SET_MAX_DATAGRAM_SIZE: */

	default:
invalid:
		DBG(cdev, "invalid control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		value = 0;
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		DBG(cdev, "ncm req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			ERROR(cdev, "ncm req %02x.%02x response err %d\n",
					ctrl->bRequestType, ctrl->bRequest,
					value);
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}

static int iap_create_bulk_endpoints(struct iap_dev *dev,
				struct usb_endpoint_descriptor *in_desc,
				struct usb_endpoint_descriptor *out_desc)
{
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req;
	struct usb_ep *ep;
	int i;

	DBG(cdev, "create_bulk_endpoints dev: %p\n", dev);

	ep = usb_ep_autoconfig(cdev->gadget, in_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_in failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for ep_in got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, out_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_out failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for iap ep_out got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_out = ep;

	/* now allocate requests for our endpoints */
	req = iap_request_new(dev->ep_out, IAP_BULK_BUFFER_SIZE);
	if (!req)
		goto fail;
	req->complete = iap_complete_out;
	dev->rx_req = req;

	for (i = 0; i < TX_REQ_MAX; i++) {
		req = iap_request_new(dev->ep_in, IAP_BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = iap_complete_in;
		iap_req_put(dev, &dev->tx_idle, req);
	}

	return 0;

fail:
	pr_err("iap_bind() could not allocate requests\n");
	return -1;
}

static ssize_t iap_read(struct file *fp, char __user *buf,
				size_t count, loff_t *pos)
{
	struct iap_dev *dev = fp->private_data;
	struct usb_request *req;
	int r = count, xfer;
	int ret;

	pr_debug("iap_read(%d)\n", count);
	if (!_iap_dev)
		return -ENODEV;

	if (count > IAP_BULK_BUFFER_SIZE)
		return -EINVAL;

	if (iap_lock(&dev->read_excl))
		return -EBUSY;

	/* we will block until we're online */
	while (!(dev->online || dev->error)) {
		pr_debug("iap_read: waiting for online state\n");
		ret = wait_event_interruptible(dev->read_wq,
				(dev->online || dev->error));
		if (ret < 0) {
			iap_unlock(&dev->read_excl);
			return ret;
		}
	}
	if (dev->error) {
		r = -EIO;
		goto done;
	}

requeue_req:
	/* queue a request */
	req = dev->rx_req;
	req->length = count;
	dev->rx_done = 0;
	ret = usb_ep_queue(dev->ep_out, req, GFP_ATOMIC);
	if (ret < 0) {
		pr_debug("iap_read: failed to queue req %p (%d)\n", req, ret);
		r = -EIO;
		dev->error = 1;
		goto done;
	} else {
		pr_debug("rx %p queue\n", req);
	}

	/* wait for a request to complete */
	ret = wait_event_interruptible(dev->read_wq, dev->rx_done);
	if (ret < 0) {
		if (ret != -ERESTARTSYS)
			dev->error = 1;
		r = ret;
		usb_ep_dequeue(dev->ep_out, req);
		goto done;
	}
	if (!dev->error) {
		/* If we got a 0-len packet, throw it back and try again. */
		if (req->actual == 0)
			goto requeue_req;

		pr_debug("rx %p %d\n", req, req->actual);
		xfer = (req->actual < count) ? req->actual : count;
		if (copy_to_user(buf, req->buf, xfer))
			r = -EFAULT;

	} else
		r = -EIO;

done:
	iap_unlock(&dev->read_excl);
	pr_debug("iap_read returning %d\n", r);
	return r;
}

static ssize_t iap_write(struct file *fp, const char __user *buf,
				 size_t count, loff_t *pos)
{
	struct iap_dev *dev = fp->private_data;
	struct usb_request *req = 0;
	int r = count, xfer;
	int ret;

	pr_debug("iap_write(%d)\n", count);
	if (!_iap_dev)
		return -ENODEV;

	if (iap_lock(&dev->write_excl))
		return -EBUSY;

	while (count > 0) {
		if (dev->error) {
			pr_debug("iap_write dev->error\n");
			r = -EIO;
			break;
		}

		/* get an idle tx request to use */
		req = 0;
		ret = wait_event_interruptible(dev->write_wq,
			(req = iap_req_get(dev, &dev->tx_idle)) || dev->error);

		if (ret < 0) {
			r = ret;
			break;
		}

		if (req != 0) {
			if (count > IAP_BULK_BUFFER_SIZE)
				xfer = IAP_BULK_BUFFER_SIZE;
			else
				xfer = count;
			if (copy_from_user(req->buf, buf, xfer)) {
				r = -EFAULT;
				break;
			}

			req->length = xfer;
			ret = usb_ep_queue(dev->ep_in, req, GFP_ATOMIC);
			if (ret < 0) {
				pr_err("iap_write: xfer error %d\n", ret);
				dev->error = 1;
				r = -EIO;
				break;
			}

			buf += xfer;
			count -= xfer;

			/* zero this so we don't try to free it on error exit */
			req = 0;
		}
	}

	if (req)
		iap_req_put(dev, &dev->tx_idle, req);

	iap_unlock(&dev->write_excl);
	pr_debug("iap_write returning %d\n", r);
	return r;
}

static char *dev_addr = "9A:3B:FC:66:E8:46";
static char *host_addr = "9A:3B:FC:66:E8:45";
static u8 hostaddr[ETH_ALEN];

static int iap_open(struct inode *ip, struct file *fp)
{
	struct iap_dev *dev = _iap_dev;
	struct usb_composite_dev *cdev = dev->cdev;
	struct eth_dev		*net_dev;

	pr_info("iap_open\n");
	if (!_iap_dev) {
		pr_err("%s %d\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (iap_lock(&_iap_dev->open_excl)) {
		if (dev->online) {
			gether_cleanup(dev->port.ioport);
			dev->port.ioport = NULL;
			usb_gadget_disconnect(cdev->gadget);
			/* Cancel pending control requests */
			usb_ep_dequeue(cdev->gadget->ep0, cdev->req);
		}
		gether_sysunreg();
		iap_unlock(&_iap_dev->open_excl);

		pr_warn("%s %d\n", __func__, __LINE__);
	}

	fp->private_data = _iap_dev;

	/* clear the error latch */
	_iap_dev->error = 0;

	/* create regnet sysfs */
	gether_sysreg();

	if (!dev->port.ioport) {
		/* set up network link layer */
		net_dev = gether_setup(cdev->gadget, dev_addr, host_addr,
				       hostaddr, QMULT_DEFAULT);
		if (!net_dev) {
			pr_err("cannot get address!!\n");
			return -ENOMEM;
		}

		ncm_reset_values(dev);
		dev->port.ioport = net_dev;
	}

	usb_gadget_connect(cdev->gadget);

	return 0;
}

static int iap_release(struct inode *ip, struct file *fp)
{
	struct iap_dev *dev = _iap_dev;
	struct usb_composite_dev *cdev = dev->cdev;

	pr_info("iap release\n");

	if (dev->online) {
		gether_cleanup(dev->port.ioport);
		dev->port.ioport = NULL;
		usb_gadget_disconnect(cdev->gadget);
		/* Cancel pending control requests */
		usb_ep_dequeue(cdev->gadget->ep0, cdev->req);
		dev->online = 0;
	}

	gether_sysunreg();

	iap_unlock(&_iap_dev->open_excl);
	return 0;
}

/* file operations for ADB device /dev/android_iap */
static const struct file_operations iap_fops = {
	.owner = THIS_MODULE,
	.read = iap_read,
	.write = iap_write,
	.open = iap_open,
	.release = iap_release,
};

static struct miscdevice iap_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = iap_shortname,
	.fops = &iap_fops,
};

static int
iap_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct iap_dev *dev = _iap_dev;
	int			id;
	int			ret;
	struct usb_ep       *ep;
	struct eth_dev		*net_dev;


	dev->cdev = cdev;
	DBG(cdev, "iap_function_bind dev: %p\n", dev);

	/* set up network link layer */
	net_dev = gether_setup(cdev->gadget, dev_addr, host_addr, hostaddr,
			       QMULT_DEFAULT);
	if (!net_dev) {
		pr_err("cannot get address!!\n");
		return -ENOMEM;
	}

	spin_lock_init(&dev->ncm_lock);
	ncm_reset_values(dev);
	dev->port.ioport = net_dev;

	usb_gstrings_attach(cdev, carplay_strings,
			    ARRAY_SIZE(carplay_strings_defs));

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	iap_interface_desc.bInterfaceNumber = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	iap_interface_desc.iInterface = 4;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	ncm_control_intf.iInterface = 5;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	ncm_data_nop_intf.iInterface = 6;
	ncm_data_intf.iInterface = 6;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	ecm_desc.iMACAddress = 7;

	/* allocate endpoints */
	ret = iap_create_bulk_endpoints(dev, &iap_fullspeed_in_desc,
			&iap_fullspeed_out_desc);
	if (ret)
		return ret;

	/* support high speed hardware */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		iap_highspeed_in_desc.bEndpointAddress =
			iap_fullspeed_in_desc.bEndpointAddress;
		iap_highspeed_out_desc.bEndpointAddress =
			iap_fullspeed_out_desc.bEndpointAddress;
	}

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	dev->ctrl_id = id;
	ncm_iad_desc.bFirstInterface = id;

	ncm_control_intf.bInterfaceNumber = id;
	ncm_union_desc.bMasterInterface0 = id;

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	dev->data_id = id;

	ncm_data_nop_intf.bInterfaceNumber = id;
	ncm_data_intf.bInterfaceNumber = id;
	ncm_union_desc.bSlaveInterface0 = id;

	/* allocate instance-specific endpoints */
	ep = usb_ep_autoconfig(dev->cdev->gadget, &fs_ncm_notify_desc);
	if (!ep)
		goto fail;
	ep->driver_data = dev;	/* claim */
	dev->notify = ep;

	/* allocate notification request and buffer */
	dev->notify_req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!dev->notify_req)
		goto fail;
	dev->notify_req->buf = kmalloc(NCM_STATUS_BYTECOUNT, GFP_KERNEL);
	if (!dev->notify_req->buf)
		goto fail;
	dev->notify_req->context = dev;
	dev->notify_req->complete = ncm_notify_complete;

	ep = usb_ep_autoconfig(dev->cdev->gadget, &fs_ncm_in_desc);
	if (!ep)
		goto fail;
	ep->driver_data = dev;	/* claim */
	dev->port.in_ep = ep;

	ep = usb_ep_autoconfig(dev->cdev->gadget, &fs_ncm_out_desc);
	if (!ep)
		goto fail;
	ep->driver_data = dev;	/* claim */
	dev->port.out_ep = ep;



	/* copy descriptors, and track endpoint copies */
	f->fs_descriptors = usb_copy_descriptors(fs_iap_descs);
	if (!f->fs_descriptors)
		goto fail;

	/*
	 * support all relevant hardware speeds... we expect that when
	 * hardware is dual speed, all bulk-capable endpoints work at
	 * both speeds
	 */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		hs_ncm_in_desc.bEndpointAddress =
				fs_ncm_in_desc.bEndpointAddress;
		hs_ncm_out_desc.bEndpointAddress =
				fs_ncm_out_desc.bEndpointAddress;
		hs_ncm_notify_desc.bEndpointAddress =
				fs_ncm_notify_desc.bEndpointAddress;

		/* copy descriptors, and track endpoint copies */
		f->hs_descriptors = usb_copy_descriptors(hs_iap_descs);
		if (!f->hs_descriptors)
			goto fail;
	}

	/*
	 * NOTE:  all that is done without knowing or caring about
	 * the network link ... which is unavailable to this code
	 * until we're activated via set_alt().
	 */

	dev->port.open = ncm_open;
	dev->port.close = ncm_close;

	DBG(cdev, "CDC Network: %s speed IN/%s OUT/%s NOTIFY/%s\n",
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			dev->port.in_ep->name, dev->port.out_ep->name,
			dev->notify->name);

	DBG(cdev, "%s speed %s: IN/%s, OUT/%s\n",
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			f->name, dev->ep_in->name, dev->ep_out->name);
	return 0;

fail:
	iap_unlock(&_iap_dev->open_excl);
	return -1;
}

static void
iap_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct iap_dev *dev = _iap_dev;
	struct usb_request *req;


	dev->online = 0;
	dev->error = 1;
	kobject_uevent_env(&dev->dev->kobj, KOBJ_CHANGE, NULL);

	wake_up(&dev->read_wq);

	iap_request_free(dev->rx_req, dev->ep_out);
	while ((req = iap_req_get(dev, &dev->tx_idle)))
		iap_request_free(req, dev->ep_in);
}

static int firstboot;
static int iap_function_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct iap_dev *dev = _iap_dev;
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;

	DBG(cdev, "iap_function_set_alt intf: %d alt: %d\n", intf, alt);

	if ((firstboot == 1) && (intf == 2) && (alt == 0))
		goto fail;
	else if ((firstboot == 1) && (intf == 2) && (alt == 1))
		goto fail;

	if (intf == 0) {
		firstboot = 0;
		ret = config_ep_by_speed(cdev->gadget, f, dev->ep_in);
		if (ret) {
			dev->ep_in->desc = NULL;
			ERROR(cdev,
			      "config_ep_by_speed failes for ep %s, result %d\n",
				dev->ep_in->name, ret);
			return ret;
		}
		ret = usb_ep_enable(dev->ep_in);
		if (ret) {
			ERROR(cdev, "failed to enable ep %s, result %d\n",
				dev->ep_in->name, ret);
			return ret;
		}

		ret = config_ep_by_speed(cdev->gadget, f, dev->ep_out);
		if (ret) {
			dev->ep_out->desc = NULL;
			ERROR(cdev,
			      "config_ep_by_speed failes for ep %s, result %d\n",
				dev->ep_out->name, ret);
			usb_ep_disable(dev->ep_in);
			return ret;
		}
		ret = usb_ep_enable(dev->ep_out);
		if (ret) {
			ERROR(cdev, "failed to enable ep %s, result %d\n",
				dev->ep_out->name, ret);
			usb_ep_disable(dev->ep_in);
			return ret;
		}
	/* Control interface has only altsetting 0 */
	} else if (intf == dev->ctrl_id) {
		if (alt != 0)
			goto fail;

		if (dev->notify->driver_data) {
			DBG(cdev, "reset ncm control %d\n", intf);
			usb_ep_disable(dev->notify);
		}


		if (!(dev->notify->desc)) {
			DBG(cdev, "init ncm ctrl %d\n", intf);
			ret = config_ep_by_speed(cdev->gadget, f, dev->notify);
		}
		usb_ep_enable(dev->notify);
		dev->notify->driver_data = dev;

	/* Data interface has two altsettings, 0 and 1 */
	} else if (intf == dev->data_id) {
#if 0
		if (alt > 1)
			goto fail;

		if (ncm->port.in_ep->enabled) {
			DBG(cdev, "reset ncm\n");
			ncm->timer_stopping = true;
			ncm->netdev = NULL;
			gether_disconnect(&ncm->port);
			ncm_reset_values(ncm);
		}
#endif
		/*
		 * CDC Network only sends data in non-default altsettings.
		 * Changing altsettings resets filters, statistics, etc.
		 */
		if (alt == 1) {
			struct net_device	*net;

			ret = config_ep_by_speed(cdev->gadget, f,
						 dev->port.in_ep);
			if (ret) {
				dev->port.in_ep->desc = NULL;
				ERROR(cdev,
				      "config_ep_by_speed failes for ep %s, result %d\n",
					dev->port.in_ep->name, ret);
				return ret;
			}
			ret = usb_ep_enable(dev->port.in_ep);
			if (ret) {
				ERROR(cdev,
				      "failed to enable ep %s, result %d\n",
					dev->port.in_ep->name, ret);
				return ret;
			}

			ret = config_ep_by_speed(cdev->gadget, f,
						 dev->port.out_ep);
			if (ret) {
				dev->port.out_ep->desc = NULL;
				ERROR(cdev,
				      "config_ep_by_speed failes for ep %s, result %d\n",
					dev->port.out_ep->name, ret);
				usb_ep_disable(dev->port.in_ep);
				return ret;
			}
			ret = usb_ep_enable(dev->port.out_ep);
			if (ret) {
				ERROR(cdev,
				      "failed to enable ep %s, result %d\n",
					dev->port.out_ep->name, ret);
				usb_ep_disable(dev->port.out_ep);
				return ret;
			}

			/* TODO */
			/* Enable zlps by default for NCM conformance;
			 * override for musb_hdrc (avoids txdma ovhead)
			 */
			dev->port.is_zlp_ok =
				gadget_is_zlp_supported(cdev->gadget);
			dev->port.cdc_filter = DEFAULT_FILTER;
			DBG(cdev, "activate ncm\n");
			net = gether_connect(&dev->port);
			if (IS_ERR(net))
				return PTR_ERR(net);

			dev->online = 1;
			firstboot = 1;
			kobject_uevent_env(&dev->dev->kobj, KOBJ_CHANGE,
					   NULL);
		}

		spin_lock(&dev->ncm_lock);
		ncm_notify(dev);
		spin_unlock(&dev->ncm_lock);
	}

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);
fail:
	return 0;
}

/*
 * Because the data interface supports multiple altsettings,
 * this NCM function *MUST* implement a get_alt() method.
 */
static int iap_function_get_alt(struct usb_function *f, unsigned intf)
{
	struct iap_dev  *dev = func_to_iap(f);

	if (intf == dev->ctrl_id)
		return 0;
	return 1;
}

static struct sk_buff *ncm_wrap_ntb(struct gether *port,
				    struct sk_buff *skb)
{
	struct iap_dev *dev = _iap_dev;
	struct sk_buff	*skb2;
	int		ncb_len = 0;
	__le16		*tmp;
	int		div = ntb_parameters.wNdpInDivisor;
	int		rem = ntb_parameters.wNdpInPayloadRemainder;
	int		pad;
	int		ndp_align = ntb_parameters.wNdpInAlignment;
	int		ndp_pad;
	unsigned	max_size = dev->port.fixed_in_len;
	struct ndp_parser_opts *opts = dev->parser_opts;
	unsigned	crc_len = dev->is_crc ? sizeof(uint32_t) : 0;

	ncb_len += opts->nth_size;
	ndp_pad = ALIGN(ncb_len, ndp_align) - ncb_len;
	ncb_len += ndp_pad;
	ncb_len += opts->ndp_size;
	ncb_len += 2 * 2 * opts->dgram_item_len; /* Datagram entry */
	ncb_len += 2 * 2 * opts->dgram_item_len; /* Zero datagram entry */
	pad = ALIGN(ncb_len, div) + rem - ncb_len;
	ncb_len += pad;

	if (ncb_len + skb->len + crc_len > max_size) {
		dev_kfree_skb_any(skb);
		return NULL;
	}

	skb2 = skb_copy_expand(skb, ncb_len,
			       max_size - skb->len - ncb_len - crc_len,
			       GFP_ATOMIC);
	dev_kfree_skb_any(skb);
	if (!skb2)
		return NULL;

	skb = skb2;

	tmp = (void *) skb_push(skb, ncb_len);
	memset(tmp, 0, ncb_len);

	put_unaligned_le32(opts->nth_sign, tmp); /* dwSignature */
	tmp += 2;
	/* wHeaderLength */
	put_unaligned_le16(opts->nth_size, tmp++);
	tmp++; /* skip wSequence */
	put_ncm(&tmp, opts->block_length, skb->len); /* (d)wBlockLength */
	/* (d)wFpIndex */
	/* the first pointer is right after the NTH + align */
	put_ncm(&tmp, opts->fp_index, opts->nth_size + ndp_pad);

	tmp = (void *)tmp + ndp_pad;

	/* NDP */
	put_unaligned_le32(opts->ndp_sign, tmp); /* dwSignature */
	tmp += 2;
	/* wLength */
	put_unaligned_le16(ncb_len - opts->nth_size - pad, tmp++);

	tmp += opts->reserved1;
	tmp += opts->next_fp_index; /* skip reserved (d)wNextFpIndex */
	tmp += opts->reserved2;

	if (dev->is_crc) {
		uint32_t crc;

		crc = ~crc32_le(~0,
				skb->data + ncb_len,
				skb->len - ncb_len);
		put_unaligned_le32(crc, skb->data + skb->len);
		skb_put(skb, crc_len);
	}

	/* (d)wDatagramIndex[0] */
	put_ncm(&tmp, opts->dgram_item_len, ncb_len);
	/* (d)wDatagramLength[0] */
	put_ncm(&tmp, opts->dgram_item_len, skb->len - ncb_len);
	/* (d)wDatagramIndex[1] and  (d)wDatagramLength[1] already zeroed */

	if (skb->len > MAX_TX_NONFIXED)
		memset(skb_put(skb, max_size - skb->len),
		       0, max_size - skb->len);

	return skb;
}

static int ncm_unwrap_ntb(struct gether *port,
			  struct sk_buff *skb,
			  struct sk_buff_head *list)
{
	struct iap_dev *dev = _iap_dev;
	__le16		*tmp = (void *) skb->data;
	unsigned	index, index2;
	unsigned	dg_len, dg_len2;
	unsigned	ndp_len;
	struct sk_buff	*skb2;
	int		ret = -EINVAL;
	unsigned	max_size = le32_to_cpu(ntb_parameters.dwNtbOutMaxSize);
	struct ndp_parser_opts *opts = dev->parser_opts;
	unsigned	crc_len = dev->is_crc ? sizeof(uint32_t) : 0;
	int		dgram_counter;

	/* dwSignature */
	if (get_unaligned_le32(tmp) != opts->nth_sign) {
		INFO(port->func.config->cdev, "Wrong NTH SIGN, skblen %d\n",
			skb->len);
		print_hex_dump(KERN_INFO, "HEAD:", DUMP_PREFIX_ADDRESS, 32, 1,
			       skb->data, 32, false);

		goto err;
	}
	tmp += 2;

	/* wHeaderLength */
	if (get_unaligned_le16(tmp++) != opts->nth_size) {
		INFO(port->func.config->cdev, "Wrong NTB headersize\n");
		goto err;
	}
	tmp++; /* skip wSequence */

	/* (d)wBlockLength */
	if (get_ncm(&tmp, opts->block_length) > max_size) {
		INFO(port->func.config->cdev, "OUT size exceeded\n");
		goto err;
	}

	index = get_ncm(&tmp, opts->fp_index);
	/* NCM 3.2 */
	if (((index % 4) != 0) && (index < opts->nth_size)) {
		INFO(port->func.config->cdev, "Bad index: %x\n",
			index);
		goto err;
	}

	/* walk through NDP */
	tmp = ((void *)skb->data) + index;
	if (get_unaligned_le32(tmp) != opts->ndp_sign) {
		INFO(port->func.config->cdev, "Wrong NDP SIGN\n");
		goto err;
	}
	tmp += 2;

	ndp_len = get_unaligned_le16(tmp++);
	/*
	 * NCM 3.3.1
	 * entry is 2 items
	 * item size is 16/32 bits, opts->dgram_item_len * 2 bytes
	 * minimal: struct usb_cdc_ncm_ndpX + normal entry + zero entry
	 */
	if ((ndp_len < opts->ndp_size + 2 * 2 * (opts->dgram_item_len * 2))
	    || (ndp_len % opts->ndplen_align != 0)) {
		INFO(port->func.config->cdev, "Bad NDP length: %x\n", ndp_len);
		goto err;
	}
	tmp += opts->reserved1;
	tmp += opts->next_fp_index; /* skip reserved (d)wNextFpIndex */
	tmp += opts->reserved2;

	ndp_len -= opts->ndp_size;
	index2 = get_ncm(&tmp, opts->dgram_item_len);
	dg_len2 = get_ncm(&tmp, opts->dgram_item_len);
	dgram_counter = 0;

	do {
		index = index2;
		dg_len = dg_len2;
		if (dg_len < 14 + crc_len) { /* ethernet header + crc */
			INFO(port->func.config->cdev, "Bad dgram length: %x\n",
			     dg_len);
			goto err;
		}
		if (dev->is_crc) {
			uint32_t crc, crc2;

			crc = get_unaligned_le32(skb->data +
						 index + dg_len - crc_len);
			crc2 = ~crc32_le(~0,
					 skb->data + index,
					 dg_len - crc_len);
			if (crc != crc2) {
				INFO(port->func.config->cdev, "Bad CRC\n");
				goto err;
			}
		}

		index2 = get_ncm(&tmp, opts->dgram_item_len);
		dg_len2 = get_ncm(&tmp, opts->dgram_item_len);

		if (index2 == 0 || dg_len2 == 0) {
			skb2 = skb;
		} else {
			skb2 = skb_clone(skb, GFP_ATOMIC);
			if (skb2 == NULL)
				goto err;
		}

		if (!skb_pull(skb2, index)) {
			ret = -EOVERFLOW;
			goto err;
		}

		skb_trim(skb2, dg_len - crc_len);
		skb_queue_tail(list, skb2);

		ndp_len -= 2 * (opts->dgram_item_len * 2);

		dgram_counter++;

		if (index2 == 0 || dg_len2 == 0)
			break;
	} while (ndp_len > 2 * (opts->dgram_item_len * 2)); /* zero entry */

	VDBG(port->func.config->cdev,
	     "Parsed NTB with %d frames\n", dgram_counter);
	return 0;
err:
	skb_queue_purge(list);
	dev_kfree_skb_any(skb);
	return ret;
}

static void iap_function_disable(struct usb_function *f)
{
	struct iap_dev	*dev = func_to_iap(f);
	struct usb_composite_dev	*cdev = dev->cdev;

	DBG(cdev, "iap_function_disable cdev %p\n", cdev);
	dev->online = 0;
	dev->error = 1;
	kobject_uevent_env(&dev->dev->kobj, KOBJ_CHANGE, NULL);
	usb_ep_disable(dev->ep_in);
	usb_ep_disable(dev->ep_out);
	usb_ep_disable(dev->notify);
	usb_ep_disable(dev->port.in_ep);
	usb_ep_disable(dev->port.out_ep);

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);

	VDBG(cdev, "%s disabled\n", dev->function.name);
}

static void iap_function_free(struct usb_function *f)
{
	struct f_carplay *carplay;
	struct f_carplay_opts *opts;
	struct iap_dev *dev = _iap_dev;

	pr_info("iap_function_free\n");

	carplay = func_to_iap(f);
	opts = container_of(f->fi, struct f_carplay_opts, func_inst);

	misc_deregister(&iap_device);
	device_destroy(android_class, dev->dev->devt);
	class_destroy(android_class);
	kfree(carplay);
	mutex_lock(&opts->lock);
	opts->refcnt--;
	mutex_unlock(&opts->lock);
}

/*-------------------------------------------------------------------------*/

/* ethernet function driver setup/binding */


int iap_bind_config(struct usb_configuration *c)
{
	struct iap_dev *dev = _iap_dev;
	struct eth_dev          *net_dev;

	pr_info("iap_bind_config\n");

	/* set up network link layer */
	net_dev = gether_setup(c->cdev->gadget, dev_addr, host_addr, hostaddr,
			       QMULT_DEFAULT);
	if (!net_dev) {
		pr_err("cannot get address!!\n");
		return -ENOMEM;
	}

	if (!can_support_ecm(c->cdev->gadget) || !hostaddr)
		return -EINVAL;

	spin_lock_init(&dev->ncm_lock);
	ncm_reset_values(dev);
	dev->port.ioport = net_dev;

	dev->port.is_fixed = true;

	dev->port.func.name = "cdc_network";

	dev->port.wrap = ncm_wrap_ntb;
	dev->port.unwrap = ncm_unwrap_ntb;

	dev->cdev = c->cdev;
	dev->function.name = "iap";
	dev->function.fs_descriptors = fs_iap_descs;
	dev->function.hs_descriptors = hs_iap_descs;
	dev->function.bind = iap_function_bind;
	dev->function.unbind = iap_function_unbind;
	dev->function.set_alt = iap_function_set_alt;
	dev->function.get_alt = iap_function_get_alt;
	dev->function.setup = iap_function_setup;
	dev->function.disable = iap_function_disable;

	return usb_add_function(c, &dev->function);
}
EXPORT_SYMBOL_GPL(iap_bind_config);

int iap_setup(void)
{
	struct iap_dev *dev;
	int ret;

	pr_info("%s\n", __func__);
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	spin_lock_init(&dev->lock);

	init_waitqueue_head(&dev->read_wq);
	init_waitqueue_head(&dev->write_wq);

	atomic_set(&dev->open_excl, 0);
	atomic_set(&dev->read_excl, 0);
	atomic_set(&dev->write_excl, 0);

	INIT_LIST_HEAD(&dev->tx_idle);

	_iap_dev = dev;

	/* create regnet sysfs */
	gether_sysreg();

	ret = misc_register(&iap_device);
	if (ret)
		goto err;

	return 0;

err:
	kfree(dev);
	pr_err("iap gadget driver failed to initialize\n");
	return ret;
}
EXPORT_SYMBOL_GPL(iap_setup);

void iap_cleanup(void)
{
	pr_info("%s\n", __func__);
	misc_deregister(&iap_device);

	kfree(_iap_dev);
	_iap_dev = NULL;
}
EXPORT_SYMBOL_GPL(iap_cleanup);

static inline struct f_carplay_opts *to_f_carplay_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_carplay_opts,
			    func_inst.group);
}

/* f_carplay_item_ops */
USB_ETHERNET_CONFIGFS_ITEM(carplay);

/* f_carplay_opts_dev_addr */
USB_ETHERNET_CONFIGFS_ITEM_ATTR_DEV_ADDR(carplay);

/* f_carplay_opts_host_addr */
USB_ETHERNET_CONFIGFS_ITEM_ATTR_HOST_ADDR(carplay);

/* f_carplay_opts_qmult */
USB_ETHERNET_CONFIGFS_ITEM_ATTR_QMULT(carplay);

/* f_carplay_opts_ifname */
USB_ETHERNET_CONFIGFS_ITEM_ATTR_IFNAME(carplay);

static struct configfs_attribute *carplay_attrs[] = {
	&carplay_opts_attr_dev_addr,
	&carplay_opts_attr_host_addr,
	&carplay_opts_attr_qmult,
	&carplay_opts_attr_ifname,
	NULL,
};

static struct config_item_type carplay_func_type = {
	.ct_item_ops	= &carplay_item_ops,
	.ct_attrs	= carplay_attrs,
	.ct_owner	= THIS_MODULE,
};

static void carplay_free_inst(struct usb_function_instance *f)
{
	struct f_carplay_opts *opts;

	opts = container_of(f, struct f_carplay_opts, func_inst);
	if (opts->bound)
		gether_cleanup(netdev_priv(opts->net));
	else
		free_netdev(opts->net);
	kfree(opts);
}

static struct usb_function_instance *carplay_alloc_inst(void)
{
	struct f_carplay_opts *opts;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);
	mutex_init(&opts->lock);
	opts->func_inst.free_func_inst = carplay_free_inst;
	opts->net = gether_setup_default();
	if (IS_ERR(opts->net)) {
		struct net_device *net = opts->net;

		kfree(opts);
		return ERR_CAST(net);
	}

	config_group_init_type_name(&opts->func_inst.group, "",
				    &carplay_func_type);

	return &opts->func_inst;
}

static ssize_t state_show(struct device *pdev, struct device_attribute *attr,
			  char *buf)
{
	struct iap_dev *dev = _iap_dev;
	char *state = "DISCONNECTED";

	if (dev->online)
		state = "CONFIGURED";
	else
		state = "DISCONNECTED";
	return sprintf(buf, "%s\n", state);
}

static DEVICE_ATTR(state, S_IRUGO, state_show, NULL);

static struct device_attribute *android_usb_attributes = &dev_attr_state;

static struct usb_function *carplay_alloc(struct usb_function_instance *fi)
{
	struct iap_dev *dev;
	struct f_carplay_opts	*opts;
	int status;
	int ret;
	struct device_attribute *attr = android_usb_attributes;

	/* allocate and initialize one new instance */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return ERR_PTR(-ENOMEM);

	spin_lock_init(&dev->lock);

	init_waitqueue_head(&dev->read_wq);
	init_waitqueue_head(&dev->write_wq);

	atomic_set(&dev->open_excl, 0);
	atomic_set(&dev->read_excl, 0);
	atomic_set(&dev->write_excl, 0);

	INIT_LIST_HEAD(&dev->tx_idle);

	_iap_dev = dev;

	ret = misc_register(&iap_device);
	if (ret)
		goto err;

	android_class = class_create(THIS_MODULE, "android_usb");
	if (IS_ERR(android_class))
		goto err;

	dev->dev = device_create(android_class, NULL,
				 MKDEV(0, 0), NULL, "android0");
	if (IS_ERR(dev->dev)) {
		class_destroy(android_class);
		goto err;
	}

	ret = device_create_file(dev->dev, attr);
	if (ret) {
		device_destroy(android_class, dev->dev->devt);
		goto err;
	}

	opts = container_of(fi, struct f_carplay_opts, func_inst);
	mutex_lock(&opts->lock);
	opts->refcnt++;

	mutex_unlock(&opts->lock);
	dev->port.is_fixed = true;
	dev->port.supports_multi_frame = true;

	dev->port.func.name = "cdc_network";
	/* descriptors are per-instance copies */
	dev->function.name = "iap";
	dev->function.fs_descriptors = fs_iap_descs;
	dev->function.hs_descriptors = hs_iap_descs;
	dev->function.bind = iap_function_bind;
	dev->function.unbind = iap_function_unbind;
	dev->function.set_alt = iap_function_set_alt;
	dev->function.get_alt = iap_function_get_alt;
	dev->function.setup = iap_function_setup;
	dev->function.disable = iap_function_disable;
	dev->function.free_func = iap_function_free;

	dev->port.wrap = ncm_wrap_ntb;
	dev->port.unwrap = ncm_unwrap_ntb;

	return &dev->function;
err:
	kfree(dev);
	pr_err("iap gadget driver failed to initialize\n");
}


DECLARE_USB_FUNCTION_INIT(carplay, carplay_alloc_inst, carplay_alloc);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hyunseok Jung");
