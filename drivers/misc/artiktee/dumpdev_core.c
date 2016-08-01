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

#include "dumpdev_core.h"

#include "circular_buffer.h"
#include "ssdev_file.h"
#include "tzdev_plat.h"

#include <linux/limits.h>
#include <linux/vmalloc.h>

#if defined(CONFIG_INSTANCE_DEBUG) && defined(CONFIG_USB_DUMP)
#include "usb_dump.h"
#endif

extern char *get_minidump_buf(void);
extern uint get_minidump_buf_size(void);
extern int tzlog_create_dir_full_path(char *dir_full_path);

/* TODO : will be remove this declare. and should be share tzsys.c's dump path */
#define ERROR_DUMP_DIR_PATH  "/opt/usr/apps/save_error_log/error_log/secureos_dump/"

static char* get_file_full_path(char *file_name, char *dst_buf, int dst_buf_len)
{
	if (file_name == NULL)
		return NULL;

	snprintf(dst_buf, dst_buf_len,
		"%s%s.elf",
		ERROR_DUMP_DIR_PATH, file_name);

	return dst_buf;
}

static int copy_to_file_from_ring(
		struct chimera_ring_buffer *src_ring,
		int src_ring_size,
		char *file_full_path,
		int data_order)
{
	struct iovec iov[2];
	int i;
	size_t n_written = 0;
	int nvec = chimera_ring_buffer_get_vecs(src_ring, iov, src_ring_size);

	for (i = 0; i < nvec; ++i) {
		if (iov[i].iov_len <= 0)
			continue;

		if (data_order == 1 && i == 0) {
			if (ss_file_create_object(file_full_path,
					iov[i].iov_base,
					iov[i].iov_len) != iov[i].iov_len) {
				tzlog_print(K_ERR, "Fail - ss_file_create_object(%s)(req:%d)\n",
						file_full_path,
						iov[i].iov_len);
				break;
			}
		}
		else {
			if (ss_file_append_object(file_full_path,
					iov[i].iov_base,
					iov[i].iov_len) != iov[i].iov_len) {
				tzlog_print(K_ERR, "Fail - ss_file_append_object(req:%d)\n",
						iov[i].iov_len);
				break;
			}
		}
		n_written += iov[i].iov_len;
	}
	chimera_ring_buffer_flush(src_ring, src_ring_size);
	return n_written;
}


void dumpdev_handler(NSRPCTransaction_t *txn_object)
{
	int ret = 0;
	uint ring_size = 0;
	struct chimera_ring_buffer *ring = NULL;
	pss_dump_ctrl dumpdev_ctrl = NULL;

	uint32_t command = nsrpc_get_command(txn_object);
	tzlog_print(K_DEBUG, "NSRPC: Received dumpdev command %d\n", command);

	dumpdev_ctrl = (pss_dump_ctrl)nsrpc_payload_ptr(txn_object);
	ring_size = get_minidump_buf_size();
	ring = (struct chimera_ring_buffer*)get_minidump_buf();
	if (ring == NULL || ring_size == 0) {
		tzlog_print(K_ERR, "NSRPC: can not process : ring : %p ring_size : %d\n",
			    ring, ring_size);
		ret = -1;
		goto exit_dumpdev_handler;
	}

	if (tzlog_create_dir_full_path(ERROR_DUMP_DIR_PATH) != 0) {
		tzlog_print(K_ERR, "NSRPC: tzlog_create_dir_full_path\n");
		ret = -1;
		goto exit_dumpdev_handler;
	}

	switch (command) {
	case NSRPC_CMD_DUMP_START:
		{
			tzlog_print(K_INFO, "NSRPC: (%s,%s)\n",
						dumpdev_ctrl->dump_file_name,
						dumpdev_ctrl->dump_ta_uuid);
		}
		break;
	case NSRPC_CMD_DUMP_TRANSFER:
	case NSRPC_CMD_DUMP_END:
		{
			size_t src_size = 0;
			char full_full_path_buf[256] = {0,};
			char *file_full_path = get_file_full_path(dumpdev_ctrl->dump_file_name,
								full_full_path_buf,
								sizeof(full_full_path_buf));
			if(command == NSRPC_CMD_DUMP_TRANSFER)
				tzlog_print(K_DEBUG, "NSRPC_CMD_DUMP_TRANSFER Received\n");
			else tzlog_print(K_DEBUG, "NSRPC_CMD_DUMP_END Received\n");

			src_size = chimera_ring_buffer_readable(ring);
			if (src_size > 0) {
				int read_size = 0;
				read_size = copy_to_file_from_ring(
						ring,
						ring_size,
						file_full_path,
						dumpdev_ctrl->data_order);
				if (src_size != read_size) {
					tzlog_print(K_ERR, "Fail - copy_to_file_from_ring\n");
					ret = -1;
					break;

				}

				tzlog_print(K_INFO, "NSRPC: file name %s order %d (src_size : %d)\n",
					    file_full_path, dumpdev_ctrl->data_order, src_size);
			}
			if (command == NSRPC_CMD_DUMP_END) {
#if defined(CONFIG_INSTANCE_DEBUG) && defined(CONFIG_USB_DUMP)
				if (ss_file_object_exist(file_full_path) == 1)
					copy_file_to_usb(file_full_path);
#endif
				plat_dump_postprocess(dumpdev_ctrl->dump_ta_uuid);
			}
		}
		break;
	default:
		tzlog_print(K_WARNING, "NSRPC: Received unsupported command %x\n", command);
		ret = -1;
		break;
	}

exit_dumpdev_handler:
	nsrpc_complete(txn_object, ret);

	tzlog_print(K_DEBUG, "NSRPC: Finished handling dumpdev command %d(ret:%d)\n", command, ret);
}
