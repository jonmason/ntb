/*********************************************************
 * Copyright (C) 2011 - 2016 Samsung Electronics Co., Ltd All Rights Reserved
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

#ifndef __TZLOG_PROCESS_H__
#define __TZLOG_PROCESS_H__

#include "tzlog_core.h"

int init_log_processing_resources(void);
void tzlog_transfer_to_tzdaemon(s_tzlog_data *src_data, int src_buffer_size);
void tzlog_transfer_to_local(s_tzlog_data *src_data, int src_buffer_size);

#endif /* __TZLOG_PROCESS_H__ */
