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

#ifndef __SOURCE_TZDEV_SSTRANSACTION_H__
#define __SOURCE_TZDEV_SSTRANSACTION_H__

#include <linux/kernel.h>

typedef struct SSTransaction_s SSTransaction_t;

#define SSTRANSACTION_NUM_ARGS		8

void sstransaction_complete(SSTransaction_t *tsx, int code);
int sstransaction_add(const void *buffer, size_t size);

int sstransaction_get_next_completion(void *buffer,
				      size_t avail_size,
				      size_t *remaining_size,
				      size_t *obj_size);

uint32_t sstransaction_get_arg(SSTransaction_t *tsx, int arg);
uint32_t sstransaction_get_command(SSTransaction_t *tsx);
void sstransaction_set_arg(SSTransaction_t *tsx, int arg, uint32_t value);
void *sstransaction_payload_ptr(SSTransaction_t *tsx);
unsigned int sstransaction_wsm_offset(SSTransaction_t *tsx, size_t *p_size);
int sstransaction_count_completions(void);

#endif /* __SOURCE_TZDEV_SSTRANSACTION_H__ */
