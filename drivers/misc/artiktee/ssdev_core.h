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

#ifndef __SECURE_CORE_H__
#define __SECURE_CORE_H__

#include "ssdev_rpmb.h"
#include "nsrpc_ree_slave.h"

typedef enum {
	TEE_SUCCESS = 0x00000000,
	TEE_EXTERNAL_REQUEST = 0xEEEEEEEE,
	TEE_ERROR_GENERIC = 0xFFFF0000,
	TEE_ERROR_ACCESS_DENIED = 0xFFFF0001,
	TEE_ERROR_CANCEL = 0xFFFF0002,
	TEE_ERROR_ACCESS_CONFLICT = 0xFFFF0003,
	TEE_ERROR_EXCESS_DATA = 0xFFFF0004,
	TEE_ERROR_BAD_FORMAT = 0xFFFF0005,
	TEE_ERROR_BAD_PARAMETERS = 0xFFFF0006,
	TEE_ERROR_BAD_STATE = 0xFFFF0007,
	TEE_ERROR_ITEM_NOT_FOUND = 0xFFFF0008,
	TEE_ERROR_NOT_IMPLEMENTED = 0xFFFF0009,
	TEE_ERROR_NOT_SUPPORTED = 0xFFFF000A,
	TEE_ERROR_NO_DATA = 0xFFFF000B,
	TEE_ERROR_OUT_OF_MEMORY = 0xFFFF000C,
	TEE_ERROR_BUSY = 0xFFFF000D,
	TEE_ERROR_COMMUNICATION = 0xFFFF000E,
	TEE_ERROR_SECURITY = 0xFFFF000F,
	TEE_ERROR_SHORT_BUFFER = 0xFFFF0010,
	TEE_ERROR_TASK_NOT_FOUND = 0xFFFF0011,
	TEE_PENDING = 0xFFFF2000,
	TEE_ERROR_TIMEOUT = 0xFFFF3001,
	TEE_ERROR_OVERFLOW = 0xFFFF300F,
	TEE_ERROR_TARGET_DEAD = 0xFFFF3024,
	TEE_ERROR_STORAGE_NO_SPACE = 0xFFFF3041,
	TEE_ERROR_MAC_INVALID = 0xFFFF3071,
	TEE_ERROR_SIGNATURE_INVALID = 0xFFFF3072,
	TEE_ERROR_TIME_NOT_SET = 0xFFFF5000,
	TEE_ERROR_TIME_NEEDS_RESET = 0xFFFF5001,
	TEE_ERROR_CERT_PARSING = 0xFFFF6000,
	TEE_ERROR_CRL_PARSING = 0xFFFF6001,
	TEE_ERROR_CERT_EXPIRED = 0xFFFF6002,
	TEE_ERROR_CERT_VERIFICATION = 0xFFFF6003,

	TEE_RESULT_NOT_READY = 0xFFFF0FFF
} TEE_Error_Codes;

enum {
	SS_CMD_NONE,
	SS_CMD_REQUEST,
	SS_CMD_RESPONSE
};

enum PKG_OPCODE_TYPE {
	SS_LOADIMG_PREPARE,
	SS_LOADIMG_PREPARE_OK,
	SS_LOADIMG_PREPARE_FAIL,

	SS_LOADIMG_NEXTPKG,
	SS_LOADIMG_RELOAD,
	SS_LOADIMG_OK,
	SS_LOADIMG_FAIL,
	SS_LOADIMG_DONE,
	SS_LOADIMG_ABORT,

	SS_UPDIMG_PREPARE,
	SS_UPDIMG_PREPARE_OK,
	SS_UPDIMG_PREPARE_FAIL,

	SS_UPDIMG_NEXTPKG,
	SS_UPDIMG_RELOAD,
	SS_UPDIMG_OK,
	SS_UPDIMG_FAIL,
	SS_UPDIMG_DONE,
	SS_UPDIMG_ABORT,
	SS_GET_COUNTER,
};

typedef struct {
	int event;
	int sub_cmd;
	int obj_type;		/*1.TEE_STORAGE_PRIVATE   2.TEE_STORAGE_RPMB */
	int obj_dirty;		/*1.update  2.delete. */
	int result;		/*check result. */
	int obj_size;		/*The whole object size */
	int pkg_size;		/*The size of this packet */
	int start_blk;		/*The start block of object */
	int blk_nums;		/*The block number of object */
	unsigned char hash_name[32];
	struct rpmb_frame rpmb_frame;
} ss_cmd_header;

int storage_register_wsm(void);
void storage_unregister_wsm(void);
int storage_init_rpmb_key(void);
int storage_get_rpmb_status(void);
void storage_free_rpmb_buddy(void);
int storage_bottom_handler(void);

void ssdev_handler(NSRPCTransaction_t *txn_object);
#endif /* __SECURE_CORE_H__ */
