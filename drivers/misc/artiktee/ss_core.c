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
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/percpu.h>
#include <linux/sysfs.h>
#include <linux/vmalloc.h>
#include <linux/crc32.h>

#include "ss_rpmb.h"
#include "ss_core.h"
#include "ss_dev.h"
#include "ss_file.h"

#include "tzdev.h"
#include "tzpage.h"
#include "tzdev_internal.h"
#include "tzdev_smc.h"

#include "sstransaction.h"
#include "tzlog_print.h"

#ifndef CONFIG_SECOS_NO_SECURE_STORAGE

#define SS_WSM_SIZE         (0x00010000)

enum scm_watch_target {
	TARGET_SS_DEV = 0x1001,
};

struct ss_wsm {
	struct mutex wsm_lock;
	int wsm_id;
	unsigned int max_size;
	char *payload;
};

enum ss_dirty_type {
	SS_OBJ_DIRTY_UPDATE,
	SS_OBJ_DIRTY_DELETE,
};

enum ss_event_type {
	SS_EVENT_QUERY_OBJECT = 0x00000001,
	SS_EVENT_FILE_LOAD_OBJECT = 0x00000010,
	SS_EVENT_FILE_UPDATE_OBJECT = 0x00000020,
	SS_EVENT_RPMB_LOAD_OBJECT = 0x00000100,
	SS_EVENT_RPMB_UPDATE_OBJECT = 0x00000200,
};

enum ss_cmd_type {
	/*common */
	SS_CMD_REGISTER_WSM = 0,
	SS_CMD_QUERY_OBJECT,
	SS_CMD_FILE_LOAD_OBJECT,
	SS_CMD_FILE_READ_DATA,
	SS_CMD_FILE_DELETE_DATA,
	SS_CMD_FILE_CREATE_DATA,
	SS_CMD_FILE_APPEND_DATA,
	SS_CMD_FILE_DELETE_FILE,
	SS_CMD_RPMB_GET_WRITE_COUNTER,
	SS_CMD_RPMB_LOAD_FRAMES,
	SS_CMD_RPMB_WRITE_FRAMES,
};

enum ss_sub_cmd_type {
	/*for object query */
	SS_SUB_CMD_QUERY_OBJECT_PREPARE,
	SS_SUB_CMD_FILE_QUERY_OBJECT,	/*unused */
	SS_SUB_CMD_RPMB_QUERY_OBJECT,
	SS_SUB_CMD_QUERY_OBJECT_POST,

	/*for file load */
	SS_SUB_CMD_FILE_LOAD_OBJECT_PREPARE,
	SS_SUB_CMD_FILE_LOAD_OBJECT,
	SS_SUB_CMD_FILE_LOAD_OBJECT_PARSE,
	SS_SUB_CMD_FILE_LOAD_OBJECT_POST,

	/*for file update */
	SS_SUB_CMD_FILE_UPDATE_OBJECT_PREPARE,
	SS_SUB_CMD_FILE_UDPATE_OBJECT_START,
	SS_SUB_CMD_FILE_UDPATE_OBJECT,
	SS_SUB_CMD_FILE_UDPATE_OBJECT_END,
	SS_SUB_CMD_FILE_UDPATE_OBJECT_POST,

	/*for rpmb load */
	SS_SUB_CMD_RPMB_LOAD_OBJECT_PREPARE,
	SS_SUB_CMD_RPMB_LOAD_OBJECT_GET_NONCE,
	SS_SUB_CMD_RPMB_LOAD_OBJECT,
	SS_SUB_CMD_RPMB_LOAD_OBJECT_PARSE,
	SS_SUB_CMD_RPMB_LOAD_OBJECT_POST,

	/*for rpmb update */
	SS_SUB_CMD_RPMB_UPDATE_OBJECT_PREPARE,

	SS_SUB_CMD_RPMB_UDPATE_OBJECT_START,
	SS_SUB_CMD_RPMB_UDPATE_OBJECT_DATA,
	SS_SUB_CMD_RPMB_UDPATE_OBJECT_VERIFY,
	SS_SUB_CMD_RPMB_UDPATE_OBJECT_END,

	SS_SUB_CMD_RPMB_UPDATE_OBJECT_ENTRY_START,
	SS_SUB_CMD_RPMB_UPDATE_OBJECT_ENTRY_END,

	SS_SUB_CMD_RPMB_UDPATE_OBJECT_POST,

	/*for rpmb header */
	SS_SUB_CMD_RPMB_HEADER_QUERY,
	SS_SUB_CMD_RPMB_HEADER_WRITE,
	SS_SUB_CMD_RPMB_HEADER_VERIFY,
	SS_SUB_CMD_RPMB_HEADER_DONE,
};

enum ss_file_main_back {
	SS_MAIN_OBJECT_FILE,
	SS_BAKE_OBJECT_FILE,
	SS_INVALIDE_OBJECT_FILE,
};

#define TEE_STORAGE_PRIVATE         0x00000001
#define TEE_STORAGE_RPMB            0x00000002
#define MAX_FILE_OBJECT_SIZE    (64*1024)
#define MAX_RPMB_OBJECT_SIZE    (4*1024)
#define OBJ_HEADER_SIZE					(256)

#define RPMB_HEADER_STATE_ERROR         (-1)
#define RPMB_HEADER_STATE_OK            (0)
#define RPMB_HEADER_STATE_UPDATE        (1)

#define OBJECT_FILE_PATH     "/opt/usr/apps/tee/data/."
#define OBJECT_FILE_MAIN_POSTFIX  ".dat"
#define OBJECT_FILE_BACK_POSTFIX  ".bak"

/* 1217 defined in tzdev */
/* 1220 rollbacked tzdev */
#define ssdev_scm_watch(devid, cmdid, p0, p1)   \
	tzdev_scm_watch((0), (devid), (cmdid), (p0), (p1))

static struct ss_wsm ss_wsm_channel;

static void ssdev_query_object(SSTransaction_t *tsx)
{
	tzlog_print(TZLOG_DEBUG,
		    "Query object of requested storage %d, RPMB object exists = %d\n",
		    sstransaction_get_arg(tsx, 0), sstransaction_get_arg(tsx,
									 1));
	/*
	 Transaction arguments:
	 0 - requested type
	 1 - true if RPMB object of this type exists
	*/

	if (sstransaction_get_arg(tsx, 0) ==
			TEE_STORAGE_PRIVATE) {
		/*PATH + 64 Bytes UUID hash name + ".dat" + NULL */
		char file_main[128] = { 0 };
		char file_back[128] = { 0 };
		int *p = NULL;
		int file_size;

		p = (int *)sstransaction_payload_ptr(tsx);

		snprintf(file_main, sizeof(file_main),
			 OBJECT_FILE_PATH "%08x%08x%08x%08x%08x%08x%08x%08x"
			 OBJECT_FILE_MAIN_POSTFIX, *p, *(p + 1), *(p + 2),
			 *(p + 3), *(p + 4), *(p + 5), *(p + 6), *(p + 7));
		snprintf(file_back, sizeof(file_back),
			 OBJECT_FILE_PATH "%08x%08x%08x%08x%08x%08x%08x%08x"
			 OBJECT_FILE_BACK_POSTFIX, *p, *(p + 1), *(p + 2),
			 *(p + 3), *(p + 4), *(p + 5), *(p + 6), *(p + 7));

		tzlog_print(TZLOG_DEBUG, "file_main:%s\n", file_main);
		tzlog_print(TZLOG_DEBUG, "file_back:%s\n", file_back);

		file_size = ss_file_object_size(file_main);
		if (file_size > 0) {
			sstransaction_set_arg(tsx, 0, TEE_STORAGE_PRIVATE);
			tzlog_print(TZLOG_DEBUG,
					" Same object id in main File.\n");
		} else {
			file_size = ss_file_object_size(file_back);
			if (file_size > 0) {
				sstransaction_set_arg(tsx, 0, TEE_STORAGE_PRIVATE);
				tzlog_print(TZLOG_DEBUG,
						" Same object id in back File.\n");
			}
			else {
				sstransaction_set_arg(tsx, 0, 0);
				tzlog_print(TZLOG_DEBUG,
						" object id does not found in normal world.\n");
			}
		}
	} else if (sstransaction_get_arg(tsx, 0) == TEE_STORAGE_RPMB) {
		if (sstransaction_get_arg(tsx, 1)) {
			sstransaction_set_arg(tsx, 0, TEE_STORAGE_RPMB);
			tzlog_print(TZLOG_DEBUG, " Same object id in RPMB.\n");
		} else {
			sstransaction_set_arg(tsx, 0, 0);
			tzlog_print(TZLOG_DEBUG,
				    " object id does not found in normal world.\n");
		}
	} else {
		tzlog_print(TZLOG_DEBUG, " Query incorrect object type.\n");
		sstransaction_set_arg(tsx, 0, 0);
	}

	sstransaction_complete(tsx, 0);
}

static int ssdev_file_copy_object(char *dest, char *src)
{
	int size = 0;
	int read_size = 0;
	int write_size = 0;
	char *buf = NULL;

	ss_file_delete_object(dest);

	size = ss_file_object_size(src);
	if (size <= 0 || size > (MAX_FILE_OBJECT_SIZE + OBJ_HEADER_SIZE)) {
		tzlog_print(TZLOG_ERROR,
			    "Failed to get object size: size =  %d.\n", size);
		return -1;
	}

	buf = (char *)vmalloc(size);
	if (NULL == buf) {
		tzlog_print(TZLOG_ERROR,
			    "Failed to malloc %d Bytes memory with vmalloc.\n",
			    size);
		return -ENOMEM;
	}

	read_size = ss_file_read_object(src, buf, size, 0);
	if (read_size != size) {
		vfree(buf);
		tzlog_print(TZLOG_ERROR, "Failed to get object data from %s.\n",
			    src);
		return -1;
	}

	write_size = ss_file_create_object(dest, buf, size);
	if (write_size != size) {
		vfree(buf);
		tzlog_print(TZLOG_ERROR, "Failed to write object data to %s.\n",
			    dest);
		return -1;
	}

	vfree(buf);
	buf = NULL;

	return 0;
}

static void ssdev_file_load_object(SSTransaction_t *tsx)
{
	char file_main[128] = { 0 };
	char file_back[128] = { 0 };
	int index;
	ssize_t obj_size;

	char *paths[] = { file_main, file_back };
	int *p = (int *)sstransaction_payload_ptr(tsx);

	snprintf(file_main, sizeof(file_main),
		 OBJECT_FILE_PATH "%08x%08x%08x%08x%08x%08x%08x%08x"
		 OBJECT_FILE_MAIN_POSTFIX, *p, *(p + 1), *(p + 2), *(p + 3),
		 *(p + 4), *(p + 5), *(p + 6), *(p + 7));
	snprintf(file_back, sizeof(file_back),
		 OBJECT_FILE_PATH "%08x%08x%08x%08x%08x%08x%08x%08x"
		 OBJECT_FILE_BACK_POSTFIX, *p, *(p + 1), *(p + 2), *(p + 3),
		 *(p + 4), *(p + 5), *(p + 6), *(p + 7));

	for (index = 0; index < 2; ++index) {
		tzlog_print(TZLOG_DEBUG, "Check if file exists '%s'\n",
			    paths[index]);

		if (ss_file_object_exist(paths[index]) != 1) {
			tzlog_print(TZLOG_DEBUG, "File '%s' doesn't exist\n",
				    paths[index]);
			continue;
		}

		obj_size = ss_file_object_size(paths[index]);
		if (obj_size <= 0 || obj_size > (MAX_FILE_OBJECT_SIZE + OBJ_HEADER_SIZE)) {
			tzlog_print(TZLOG_DEBUG,
				    "Failed to obtain proper object size. obj_size = %zd\n",
				    obj_size);
			continue;
		}

		tzlog_print(TZLOG_DEBUG, "Size for %s id %zd bytes\n",
			    paths[index], obj_size);

		/* Attach pointer to secure transaction object */
		sstransaction_set_arg(tsx, 0, obj_size);
		memcpy(sstransaction_payload_ptr(tsx), paths[index],
		       strlen(paths[index]) + 1);

		sstransaction_complete(tsx, 0);
		return;
	}

	tzlog_print(TZLOG_ERROR, "File '%s' doesn't exist\n", paths[0]);
	sstransaction_complete(tsx, -ENOENT);
}

static void ssdev_file_read_data(SSTransaction_t *tsx)
{
	char *fileName = sstransaction_payload_ptr(tsx);
	size_t wsm_size = 0;
	void *wsm_buffer =
	    (char *)ss_wsm_channel.payload + sstransaction_wsm_offset(tsx,
								      &wsm_size);
	int read_size;

	if (wsm_size < sstransaction_get_arg(tsx, 0)) {
		sstransaction_complete(tsx, -E2BIG);
		return;
	}

	tzlog_print(TZLOG_DEBUG,
		    "Read file data '%s' at offset %d and length %d\n",
		    fileName, sstransaction_get_arg(tsx, 0),
		    sstransaction_get_arg(tsx, 1));

	read_size = ss_file_read_object(fileName,
					wsm_buffer,
					sstransaction_get_arg(tsx, 0),
					sstransaction_get_arg(tsx, 1));

	if (read_size != sstransaction_get_arg(tsx, 0)) {
		tzlog_print(TZLOG_WARNING, "Unable to read file data (%d)\n",
			    read_size);

		sstransaction_complete(tsx, -EIO);
	} else {
		tzlog_print(TZLOG_DEBUG, "File data read complete\n");
#ifdef _STORAGE_DEBUG_
		print_hex_dump(KERN_DEBUG, "file data: ", DUMP_PREFIX_ADDRESS,
			       16, 4, wsm_buffer, wsm_size, 1);
#endif
		sstransaction_complete(tsx, 0);
	}
}

static void ssdev_file_delete_file(SSTransaction_t *tsx)
{
	ss_file_delete_object((char *)sstransaction_payload_ptr(tsx));
	sstransaction_complete(tsx, 0);
}

static void ssdev_file_create_data(SSTransaction_t *tsx)
{
	char file_main[128] = { 0 };	/*PATH + 32 Bytes UUID hash + ".dat" + NULL */
	char file_back[128] = { 0 };
	int *p = (int *)sstransaction_payload_ptr(tsx);
	size_t wsm_size = 0;
	int ret;
	int file_size;
	void *wsm_buffer =
	    (char *)ss_wsm_channel.payload + sstransaction_wsm_offset(tsx,
								      &wsm_size);

	snprintf(file_main, sizeof(file_main),
		 OBJECT_FILE_PATH "%08x%08x%08x%08x%08x%08x%08x%08x"
		 OBJECT_FILE_MAIN_POSTFIX, *p, *(p + 1), *(p + 2), *(p + 3),
		 *(p + 4), *(p + 5), *(p + 6), *(p + 7));
	snprintf(file_back, sizeof(file_back),
		 OBJECT_FILE_PATH "%08x%08x%08x%08x%08x%08x%08x%08x"
		 OBJECT_FILE_BACK_POSTFIX, *p, *(p + 1), *(p + 2), *(p + 3),
		 *(p + 4), *(p + 5), *(p + 6), *(p + 7));

	tzlog_print(TZLOG_DEBUG,
		    "Create data, file_main '%s', file back '%s'\n", file_main,
		    file_back);

	file_size = ss_file_object_size(file_main);
	if (file_size > 0) {
		tzlog_print(TZLOG_DEBUG, "Copy main to bak file\n");

		ret = ssdev_file_copy_object(file_back, file_main);
		if(ret < 0)
		{
			tzlog_print(TZLOG_WARNING, "Failed to make bak file. result : %d\n", ret);
			sstransaction_complete(tsx, -EIO);
			return;
		}
	}

	tzlog_print(TZLOG_DEBUG,
		    "create file object '%s' with buffer %p and size %zd\n",
		    file_main, wsm_buffer, wsm_size);
#ifdef _STORAGE_DEBUG_
	print_hex_dump(KERN_DEBUG, "file data: ", DUMP_PREFIX_ADDRESS, 16, 4,
		       wsm_buffer, wsm_size, 1);
#endif
	ret = ss_file_create_object(file_main, wsm_buffer, wsm_size);

	if (ret != wsm_size) {
		tzlog_print(TZLOG_WARNING,
			    "Can't create file '%s' - error = %d\n", file_main,
			    ret);

		sstransaction_complete(tsx, -EIO);
	} else {
		tzlog_print(TZLOG_DEBUG, "Written %d bytes to '%s'\n", ret,
			    file_main);

		sstransaction_complete(tsx, 0);
	}
}

static void ssdev_file_append_data(SSTransaction_t *tsx)
{
	char file_main[128] = { 0 };	/*PATH + 32 Bytes UUID hash + ".dat" + NULL */
	int *p = (int *)sstransaction_payload_ptr(tsx);
	size_t wsm_size = 0;
	int ret;
	void *wsm_buffer =
	    (char *)ss_wsm_channel.payload + sstransaction_wsm_offset(tsx,
								      &wsm_size);

	snprintf(file_main, sizeof(file_main),
		 OBJECT_FILE_PATH "%08x%08x%08x%08x%08x%08x%08x%08x"
		 OBJECT_FILE_MAIN_POSTFIX, *p, *(p + 1), *(p + 2), *(p + 3),
		 *(p + 4), *(p + 5), *(p + 6), *(p + 7));

	tzlog_print(TZLOG_DEBUG,
		    "append file object '%s' with buffer %p and size %zd\n",
		    file_main, wsm_buffer, wsm_size);
#ifdef _STORAGE_DEBUG_
	print_hex_dump(KERN_DEBUG, "file data: ", DUMP_PREFIX_ADDRESS, 16, 4,
		       wsm_buffer, wsm_size, 1);
#endif
	ret = ss_file_append_object(file_main, wsm_buffer, wsm_size);

	if (ret != wsm_size) {
		tzlog_print(TZLOG_WARNING,
			    "Can't append to file '%s' - error = %d\n",
			    file_main, ret);

		sstransaction_complete(tsx, -EIO);
	} else {
		tzlog_print(TZLOG_DEBUG, "Appended %d bytes to '%s'\n", ret,
			    file_main);

		sstransaction_complete(tsx, 0);
	}
}

static void ssdev_file_delete_data(SSTransaction_t *tsx)
{
	char file_main[128] = { 0 };	/*PATH + 32 Bytes UUID hash + ".dat" + NULL */
	char file_back[128] = { 0 };
	int *p = (int *)sstransaction_payload_ptr(tsx);

	snprintf(file_main, sizeof(file_main),
		 OBJECT_FILE_PATH "%08x%08x%08x%08x%08x%08x%08x%08x"
		 OBJECT_FILE_MAIN_POSTFIX, *p, *(p + 1), *(p + 2), *(p + 3),
		 *(p + 4), *(p + 5), *(p + 6), *(p + 7));
	snprintf(file_back, sizeof(file_back),
		 OBJECT_FILE_PATH "%08x%08x%08x%08x%08x%08x%08x%08x"
		 OBJECT_FILE_BACK_POSTFIX, *p, *(p + 1), *(p + 2), *(p + 3),
		 *(p + 4), *(p + 5), *(p + 6), *(p + 7));

	tzlog_print(TZLOG_DEBUG, "file_main:%s\n", file_main);
	tzlog_print(TZLOG_DEBUG, "file_back:%s\n", file_back);

	ss_file_delete_object(file_main);
	ss_file_delete_object(file_back);
	sstransaction_complete(tsx, 0);
}

#ifndef CONFIG_SECOS_NO_RPMB
static void ssdev_rpmb_get_write_counter(SSTransaction_t *tsx)
{
	u32 write_counter = 0;

#if defined(CONFIG_MMC) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
	int ret = ss_rpmb_get_wctr(&write_counter);
#else
	int ret = -EIO;
#endif

	if (ret < 0) {
		tzlog_print(TZLOG_ERROR,
			    "Can't get RPMB write counter value - error %d\n",
			    ret);

		sstransaction_complete(tsx, -EIO);
	} else {
		tzlog_print(TZLOG_DEBUG, "Got RPMB write counter %u\n",
			    write_counter);

		sstransaction_set_arg(tsx, 0, write_counter);
		sstransaction_complete(tsx, 0);
	}
}

static void ssdev_rpmb_load_frames(SSTransaction_t *tsx)
{
#if defined(CONFIG_MMC) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
	const size_t blk_nums = sstransaction_get_arg(tsx, 0);
	const size_t object_size = blk_nums * RPMB_SECOTR;
	const size_t piece_size = RPMB_SECOTR * RPMB_READ_BLOCKS_UNIT;
	const size_t count = (object_size + piece_size - 1) / piece_size;
	const size_t extra_size = object_size - piece_size * (count - 1);
	size_t piece;
	size_t pkg_size;
	size_t start_blk;
	size_t total_frames_bytes;

	uint8_t *nonce_ptr;
	uint8_t *frame_ptr;
	size_t wsm_size = 0;
	void *wsm_buffer =
	    (char *)ss_wsm_channel.payload + sstransaction_wsm_offset(tsx,
								      &wsm_size);

	start_blk = sstransaction_get_arg(tsx, 1);

	tzlog_print(TZLOG_DEBUG,
		    "Load RPMB %zd blocks. Object size = %zd, piece size = %zd, piece count = %zd, extra = %zd\n",
		    blk_nums, object_size, piece_size, count, extra_size);

	total_frames_bytes = sizeof(struct rpmb_frame) * count + object_size;

	tzlog_print(TZLOG_DEBUG,
		    "Total WSM frame bytes should be %zd, nonce should be %d bytes\n",
		    total_frames_bytes, 16 * count);

	if (wsm_size != ((16 * count) + total_frames_bytes)) {
		tzlog_print(TZLOG_ERROR,
			    "WSM size mismatch for RPMB frames - %zd, should be %zd\n",
			    wsm_size,
			    (size_t) ((16 * count) + total_frames_bytes));

		sstransaction_complete(tsx, -EINVAL);
		return;
	}

	nonce_ptr = wsm_buffer;
	frame_ptr = nonce_ptr + 16 * count;

	for (piece = 0; piece < count; piece++) {
		struct rpmb_frame *frame = (struct rpmb_frame *)frame_ptr;

		if (piece == (count - 1))
			pkg_size = extra_size;
		else
			pkg_size = piece_size;

		tzlog_print(TZLOG_DEBUG,
			    "Load piece %zd of %zd, with pkg_size = %zd\n",
			    piece, count, pkg_size);

		frame_ptr += sizeof(struct rpmb_frame) + pkg_size;

		memset(frame, 0, sizeof(struct rpmb_frame));
		memcpy(frame->nonce, nonce_ptr, 16);
		frame->addr =
		    cpu_to_be16(start_blk + RPMB_READ_BLOCKS_UNIT * piece);

		if (ss_rpmb_read_block(frame, (u8 *) (frame + 1), pkg_size) !=
		    pkg_size) {
			tzlog_print(TZLOG_ERROR,
				    "Failed to read %zd Bytes data from rpmb.\n",
				    pkg_size);
			sstransaction_complete(tsx, -EIO);
			return;
		}
	}

	tzlog_print(TZLOG_DEBUG,
		    "Complete transaction after reading RPMB blocks\n");

	sstransaction_complete(tsx, 0);
#else
	sstransaction_complete(tsx, -EIO);
#endif /* defined(CONFIG_MMC) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)) */
}

static void ssdev_rpmb_store_frames(SSTransaction_t *tsx)
{
#if defined(CONFIG_MMC) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
	const size_t blk_nums = sstransaction_get_arg(tsx, 0);
	size_t wsm_size = 0;
	void *wsm_buffer =
	    (char *)ss_wsm_channel.payload + sstransaction_wsm_offset(tsx,
								      &wsm_size);
	u32 write_counter;
	int ret;
	size_t piece;
	struct rpmb_frame *frame;

	const size_t total_frames_bytes = sizeof(struct rpmb_frame) * blk_nums;

	if (wsm_size != total_frames_bytes) {
		tzlog_print(TZLOG_ERROR,
			    "WSM size mismatch for RPMB frames - %zd, should be %zd\n",
			    wsm_size, total_frames_bytes);

		sstransaction_complete(tsx, -EINVAL);
		return;
	}

	ret = ss_rpmb_get_wctr(&write_counter);
	if (ret < 0) {
		tzlog_print(TZLOG_ERROR, "Can't get write counter - %d\n", ret);
		sstransaction_complete(tsx, -EIO);
		return;
	}

	sstransaction_set_arg(tsx, 0, write_counter);

	frame = wsm_buffer;

	for (piece = 0; piece < blk_nums; piece++) {
		if (ss_rpmb_write_block(frame, frame->data, RPMB_SECOTR) !=
		    RPMB_SECOTR) {
			tzlog_print(TZLOG_ERROR,
				    "Failed to write %d Bytes data to rpmb.\n",
				    RPMB_SECOTR);
			sstransaction_complete(tsx, -EIO);
			return;
		}
	}

	ret = ss_rpmb_get_wctr(&write_counter);
	if (ret < 0) {
		tzlog_print(TZLOG_ERROR, "Can't get write counter - %d\n", ret);
		sstransaction_complete(tsx, -EIO);
		return;
	}

	sstransaction_set_arg(tsx, 1, write_counter);

	sstransaction_complete(tsx, 0);
#else
	sstransaction_complete(tsx, -EIO);
#endif /* defined(CONFIG_MMC) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)) */
}
#endif /* CONFIG_SECOS_NO_RPMB */

int storage_register_wsm(void)
{
	int ret;
	struct ss_wsm *wsm_page = &ss_wsm_channel;
	void *vmem;

	mutex_init(&wsm_page->wsm_lock);
	mutex_lock(&wsm_page->wsm_lock);

	wsm_page->wsm_id = -1;

	vmem = vmalloc(SS_WSM_SIZE);

	if (!vmem)
		panic("Unable to allocate storage driver WSM");

	memset(vmem, 0, SS_WSM_SIZE);

	wsm_page->payload = vmem;

	ret =
	    tzwsm_register_kernel_memory(wsm_page->payload, SS_WSM_SIZE,
					 GFP_KERNEL);

	if (ret < 0)
		panic("Unable to register WSM buffer in Secure Kernel\n");

	wsm_page->wsm_id = ret;
	wsm_page->max_size = SS_WSM_SIZE;

	mutex_unlock(&wsm_page->wsm_lock);
	return 0;
}

void sstransaction_handler(SSTransaction_t *txn_object)
{
	uint32_t command = sstransaction_get_command(txn_object);

	tzlog_print(TZLOG_DEBUG, "Received command %d\n", command);

	switch (command) {
	case SS_CMD_REGISTER_WSM:
		sstransaction_set_arg(txn_object, 0, ss_wsm_channel.wsm_id);
		sstransaction_set_arg(txn_object, 1, ss_wsm_channel.max_size);
		sstransaction_complete(txn_object, 0);
		break;
	case SS_CMD_QUERY_OBJECT:
		ssdev_query_object(txn_object);
		break;
	case SS_CMD_FILE_LOAD_OBJECT:
		ssdev_file_load_object(txn_object);
		break;
	case SS_CMD_FILE_READ_DATA:
		ssdev_file_read_data(txn_object);
		break;
	case SS_CMD_FILE_DELETE_DATA:
		ssdev_file_delete_data(txn_object);
		break;
	case SS_CMD_FILE_CREATE_DATA:
		ssdev_file_create_data(txn_object);
		break;
	case SS_CMD_FILE_APPEND_DATA:
		ssdev_file_append_data(txn_object);
		break;
	case SS_CMD_FILE_DELETE_FILE:
		ssdev_file_delete_file(txn_object);
		break;
#ifndef CONFIG_SECOS_NO_RPMB
	case SS_CMD_RPMB_GET_WRITE_COUNTER:
		ssdev_rpmb_get_write_counter(txn_object);
		break;
	case SS_CMD_RPMB_LOAD_FRAMES:
		ssdev_rpmb_load_frames(txn_object);
		break;
	case SS_CMD_RPMB_WRITE_FRAMES:
		ssdev_rpmb_store_frames(txn_object);
		break;
#endif /* CONFIG_SECOS_NO_RPMB */
	default:
		tzlog_print(TZLOG_WARNING, "Received unsupported command %x\n",
			    command);
		sstransaction_complete(txn_object, -EINVAL);
		break;
	}

	tzlog_print(TZLOG_DEBUG, "Finished handling command %d\n", command);
}

#endif /* CONFIG_SECOS_NO_SECURE_STORAGE */
