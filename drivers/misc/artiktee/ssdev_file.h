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

#ifndef __SECURE_FILE_H__
#define __SECURE_FILE_H__

int ss_file_read_object(char *path, char *buf, size_t size, off_t offset);
int ss_file_create_object(char *path, char *buf, size_t size);
int ss_file_append_object(char *path, char *buf, size_t size);

void ss_file_delete_object(char *path);
int ss_file_object_exist(char *path);
int ss_file_object_size(char *path);

#endif /* __SECURE_FILE_H__ */
