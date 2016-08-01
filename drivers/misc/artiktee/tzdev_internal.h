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


#ifndef __SOURCE_TZDEV_TZDEV_INTERNAL_H__
#define __SOURCE_TZDEV_TZDEV_INTERNAL_H__

#define SCM_RESULT_DENIED       1
#define SCM_RESULT_INTERRUPTED  4
#define SCM_RESULT_FAULT        14
#define SCM_RESULT_BUSY         16
#define SCM_RESULT_AGAIN        11
#define SCM_RESULT_INVALID      22

#define SCM_RING_SIZE       ((PAGE_SIZE / 2) - (sizeof(unsigned int) * 2))

typedef unsigned int SCM_RING_IDX;

struct scm_buffer {
	char buffer[SCM_RING_SIZE];
	unsigned int size;
	unsigned int flags;
};

struct scm_mux_link {
	struct scm_buffer out;
	struct scm_buffer in;
};

#define SCM_FLAG_LOG_AVAIL		1
#define SCM_FLAG_LOW_MEMORY		2

#define SCM_PROCESS_RX			1
#define SCM_PROCESS_TX			2
#define SCM_PROCESS_DPC			4
#define SCM_PROCESS_RESCHED		8

#define SCM_DPC_SCHEDULE		2

#define UUID_STR_LEN 37

#define WSM_FLAG_PERSIST	0x00000001

void tzsys_crash_check(void);

struct secos_syspage {
	uint32_t			syscrash0;
	uint32_t			syscrash1;
	uint32_t			tzlog_data_avail;
	uint32_t			transaction_data_avail;
	uint32_t			minidump_size;
	uint32_t			__padding_0_;
	char				uid[37];
	char				__padding_1_[3];
};

#define SYSCRASH_VALUE_0		0x3e360c54
#define SYSCRASH_VALUE_1		0xab2a9e60

extern struct secos_syspage *tz_syspage;

struct tzio_link {
	int id;
	struct scm_mux_link *mux_link;
	struct list_head list;
	struct page *pg;
};

struct secos_kern_info {
	/* Sent from SecureOS */
	uint32_t size;
	uint32_t abi;
	uint64_t shmem_base;
	uint64_t shmem_size;

	/* Check if timer management needs linux assistance */
	uint32_t soft_timer;

	/* Send to SecureOS */
	uint32_t ipi_fiq;
};

#define SECOS_KERN_INFO_V1      (sizeof(struct secos_kern_info))
#define SECOS_ABI_VERSION       11

struct secos_register_wsm_args {
	uint64_t va;
	uint32_t size;
	uint32_t flags;
	uint32_t tee_ctx_id;
};

#define SCM_LLC_CONTROL	(1)
#define SCM_LLC_DATA	(2)
#define SCM_LLC_NOTIFY  (3)
#define SCM_LLC_RPC	(4)

struct scm_msg_link {
	uint32_t		preamble;
	uint32_t		channel;
	uint32_t		length;
	char			payload[];
};

#ifndef __SecureOS__

struct tzio_link *tzio_acquire_link(gfp_t gfp);
void tzio_release_link(struct tzio_link *link);
int tzwsm_register_tzdev_memory(
		uint64_t ctx_id,
		struct page **pages,
		size_t num_pages,
		gfp_t gfp,
		int for_kernel);
int tzwsm_register_kernel_memory(const void *ptr, size_t size, gfp_t gfp);
int tzwsm_register_user_memory(
		uint64_t ctx_id,
		const void *__user ptr,
		size_t size,
		int rw,
		gfp_t gfp,
		struct page ***p_pages,
		size_t *p_num_pages);
void tzwsm_unregister_kernel_memory(int wsmid);

#endif /* __SecureOS__ */
#endif /* __SOURCE_TZDEV_TZDEV_INTERNAL_H__ */
