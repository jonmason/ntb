/*
 * u_carplay.h
 *
 * Utility definitions for the carplay function
 *
 * Copyright (c) 2016 Nexell Co., Ltd.
 *		http://www.nexell.co.kr
 *
 * Author: Hyunseok Jung <hsjung@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef U_CARPLAY_H
#define U_CARPLAY_H

#include <linux/usb/composite.h>

struct f_carplay_opts {
	struct usb_function_instance	func_inst;
	struct net_device		*net;
	bool				bound;

	/*
	 * Read/write access to configfs attributes is handled by configfs.
	 *
	 * This is to protect the data from concurrent access by read/write
	 * and create symlink/remove symlink.
	 */
	struct mutex			lock;
	int				refcnt;
};

extern int iap_bind_config(struct usb_configuration *c);
extern int iap_setup(void);
extern void iap_cleanup(void);

#endif /* U_CARPLAY_H */
