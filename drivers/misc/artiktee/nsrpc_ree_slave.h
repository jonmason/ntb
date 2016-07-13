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

#ifndef __SOURCE_TZDEV_NSRPC_H__
#define __SOURCE_TZDEV_NSRPC_H__

#include <linux/kernel.h>

typedef struct NSRPCTransaction_s NSRPCTransaction_t;

#define NSRPC_NUM_ARGS		8

void nsrpc_complete(NSRPCTransaction_t *tsx, int code);
int nsrpc_add(const void *buffer, size_t size);

int nsrpc_get_next_completion(void *buffer,
				      size_t avail_size,
				      size_t *remaining_size,
				      size_t *obj_size);

uint32_t nsrpc_get_arg(NSRPCTransaction_t *tsx, int arg);
uint32_t nsrpc_get_command(NSRPCTransaction_t *tsx);
void nsrpc_set_arg(NSRPCTransaction_t *tsx, int arg, uint32_t value);
void *nsrpc_payload_ptr(NSRPCTransaction_t *tsx);
unsigned int nsrpc_wsm_offset(NSRPCTransaction_t *tsx, size_t *p_size);
int nsrpc_count_completions(void);

#endif /* __SOURCE_TZDEV_NSRPC_H__ */
