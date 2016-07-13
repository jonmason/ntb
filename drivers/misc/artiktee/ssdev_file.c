/*********************************************************
 * Copyright (C) 2011 - 2015 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <linux/dcache.h>
#include <linux/namei.h>
#include <linux/fs.h>

#include "ssdev_file.h"
#include "tzlog_print.h"

#ifndef CONFIG_SECOS_NO_SECURE_STORAGE

int ss_file_read_object(char *path, char *buf, size_t size, off_t offset)
{
	int read_size = -1;
	struct file *file = NULL;

	file = filp_open(path, O_RDWR, 0);

	if (IS_ERR(file)) {
		tzlog_print(TZLOG_ERROR,
			    "error occured while opening file %s, exiting...\n",
			    path);
		return -1;
	}

	read_size = kernel_read(file, offset, buf, size);

	filp_close(file, NULL);

	return read_size;
}

int ss_file_create_object(char *path, char *buf, size_t size)
{
	int write_size = -1;
	struct file *file = NULL;

	file = filp_open(path, O_CREAT | O_RDWR | O_SYNC | O_TRUNC, 0600);

	if (IS_ERR(file)) {
		tzlog_print(TZLOG_ERROR,
			    "error occured while opening file %s, exiting...\n",
			    path);
		return -1;
	}

	write_size = kernel_write(file, (char *)buf, size, 0);

	filp_close(file, NULL);

	if (write_size != size)
		ss_file_delete_object(path);

	return write_size;
}

int ss_file_append_object(char *path, char *buf, size_t size)
{
	int write_size = -1;
	struct file *file = NULL;
	loff_t pos;

	file = filp_open(path, O_RDWR | O_SYNC, 0600);

	if (IS_ERR(file)) {
		tzlog_print(TZLOG_ERROR,
			    "error occured while opening file %s, exiting...\n",
			    path);
		return -1;
	}

	pos = vfs_llseek(file, 0, SEEK_END);

	if (pos < 0)
		write_size = pos;
	else
		write_size = kernel_write(file, (char *)buf, size, pos);

	filp_close(file, NULL);

	return write_size;
}

void ss_file_delete_object(char *path)
{
	struct path file_path;
	int err;
	struct dentry *dir;

	err = kern_path(path, 0, &file_path);

	if (err < 0)
		return;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
	if (0)
#else
	if (!d_is_file(file_path.dentry)
			|| d_really_is_negative(file_path.dentry))
#endif
	{
		tzlog_print(TZLOG_ERROR, "Can't evaluate path %s to dentry\n",
			    path);
		err = -EINVAL;
	} else {
		dir = dget_parent(file_path.dentry);

		if (!IS_ERR(dir)) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
			err = vfs_unlink(dir->d_inode, file_path.dentry);
#else
			err = vfs_unlink(d_inode(dir), file_path.dentry, NULL);
#endif
			dput(dir);
		} else {
			err = PTR_ERR(dir);
		}
	}

	path_put(&file_path);

	if (err < 0) {
		tzlog_print(TZLOG_ERROR,
			    "Failed to delete file %s with error %d\n", path,
			    err);
	}
}

int ss_file_object_exist(char *path)
{
	struct file *file = NULL;

	file = filp_open(path, O_RDWR, 0);
	if (file == ERR_PTR(-ENOENT))
		return 0;

	if (IS_ERR(file))
		return -1;

	filp_close(file, NULL);

	return 1;
}

int ss_file_object_size(char *path)
{
	struct file *file = NULL;
	loff_t size = 0;

	file = filp_open(path, O_RDWR, 0);
	if (IS_ERR(file)) {
		tzlog_print(TZLOG_DEBUG,
			    "error occured while opening file %s, exiting...\n",
			    path);
		return -1;
	}

	size = vfs_llseek(file, 0, SEEK_END);

	filp_close(file, NULL);
	file = NULL;

	return size;
}

#endif
