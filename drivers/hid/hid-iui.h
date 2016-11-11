/*
 *   Apple "IUI" driver header
 *
 *   Copyright (c) 2015 Hyunseok Jung <hsjung@nexell.co.kr>
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef HID_IUI_H
#define HID_IUI_H

/*function proto types*/
long iuihid_ioctl(struct file *file,  unsigned int cmd, unsigned long arg);
int iuihid_open(struct inode *inode, struct file *file);
int iuihid_release(struct inode *inode, struct file *file);

#endif

