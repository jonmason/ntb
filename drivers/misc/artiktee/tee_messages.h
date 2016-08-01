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

#ifndef __TEE_MESSAGES_H__
#define __TEE_MESSAGES_H__

enum trm_cmd_id
{
	MSG_CONNECT,			/* 0x00 */
	MSG_DISCONNECT,			/* 0x01 */
	MSG_INITIALIZE_CONTEXT,		/* 0x02 */
	MSG_FINALIZE_CONTEXT,		/* 0x03 */
	MSG_REGISTER_SHARED_MEMORY,	/* 0x04 */
	MSG_UNREGISTER_SHARED_MEMORY,	/* 0x05 */
	MSG_OPEN_SESSION,		/* 0x06 */
	MSG_CLOSE_SESSION,		/* 0x07 */
	MSG_INVOKE_CMD,			/* 0x08 */
	MSG_PROVISION,			/* 0x09 */
	MSG_DESTROY,			/* 0x09 */
	MSG_CLEANUP,			/* 0x0A */
	MSG_INIT_INTERNAL_SESSION,
	MSG_DESTROY_INTERNAL_SESSION,
	MSG_CHECK_SESSION,
	MSG_TRAN_VERSION_INFO,
	MSG_NUM,
};

struct trm_message_context
{
	unsigned int tee_ctx_id;
	char name[];
};

struct trm_message_version_infos
{
	unsigned int shared_mem_id;
	unsigned int data_size;
};

struct trm_message_object
{
	unsigned int tee_ctx_id;
	unsigned int image;
	unsigned int size;
	unsigned int session;
	unsigned int endpoint;
	unsigned char uuid[16];
	unsigned int launch_flags[2];
};

struct trm_message_internal_session
{
	unsigned int src_id;
	unsigned int session;
	unsigned int endpoint;
	unsigned char uuid[16];
};

struct value_params
{
	uint32_t a,b;
};

typedef union
{
	struct
	{
		int	 memid;
		uint32_t offset;
		uint64_t size;
		char shm_name[6];
	} memref;
	struct value_params value;
} trm_param;

struct trm_message_session
{
	unsigned int	id;
	unsigned int	param_types;
	trm_param	params[4];
};

struct trm_message_command
{
	unsigned int	id;
	unsigned int	session;
	unsigned int	param_types;
	trm_param	params[4];
};


struct trm_message
{
	enum trm_cmd_id	type;
	uint64_t	code;
	uint64_t	origin;
	union
	{
		struct trm_message_version_infos ver;
		struct trm_message_object  obj;
		struct trm_message_context ctx;
		struct trm_message_session session;
		struct trm_message_command command;
		struct trm_message_internal_session internal;
	};
};

struct trm_ta_image
{
	unsigned int type;
	char buffer[];
};

int get_entrypoint_socket(void);

#endif /*!__TEE_MESSAGES_H__*/
