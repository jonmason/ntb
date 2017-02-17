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

#ifndef __SOURCE_TZDEV_DUMPDEV_CORE_H__
#define __SOURCE_TZDEV_DUMPDEV_CORE_H__

#define MAX_DUMP_FILE_NAME 32 * 2

#include "nsrpc_ree_slave.h"

typedef enum {
	NSRPC_CMD_DUMP_START = 1,
	NSRPC_CMD_DUMP_TRANSFER,
	NSRPC_CMD_DUMP_END,
} nsrpc_dumpdev_cmd;

typedef struct nsrpc_dump_ctrl {
	char 		 	dump_file_name[MAX_DUMP_FILE_NAME];
	char 			dump_ta_uuid[37]; /* Can not change by caller */
	uint32_t		data_order; /* Can not change by caller */
} nsrpc_dumpdev_ctrl;

typedef nsrpc_dumpdev_ctrl *pss_dump_ctrl;

void dumpdev_handler(NSRPCTransaction_t *txn_object);

#endif /* __SOURCE_TZDEV_DUMPDEV_CORE_H__ */
