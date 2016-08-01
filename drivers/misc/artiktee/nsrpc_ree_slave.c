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
#include <linux/cpu.h>
#include <linux/idr.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/cpu.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>

#include "tzdev.h"
#include "tzdev_internal.h"
#include "tzlog_print.h"
#include "ssdev_core.h"
#include "dumpdev_core.h"
#include "nsrpc_ree_slave.h"

/* If you update this, also update nsrpc_tee_master.h in Secure Kernel */

#define NSRPC_CHANNEL_SSDEV			1
#define SSTRANSACTION_CHANNEL_LIB_PROV		2
#define NSRPC_CHANNEL_DUMPDEV			3

struct nsrpc_rpc_data {
	uint64_t transaction_id;	/* 8 - filled by kernel */
	uint32_t channel;	/* 12 */
	uint32_t command;	/* 16 */
	uint32_t arg[8];	/* 48 - can be updated by NWd */
	uint32_t wsm_offset;	/* 52 */
	uint32_t wsm_size;	/* 58 */
	int32_t result;		/* 60 - updated by NWd */
	char payload[128];	/* 188 - can be updated by NWd */
};

struct NSRPCTransaction_s {
	struct list_head tsx_queue;
	struct nsrpc_rpc_data rpc_data;
};

static struct kmem_cache *s_nsrpc_cache;

static DEFINE_SEMAPHORE(s_nsrpc_sem);
static DEFINE_SPINLOCK(s_nsrpc_lock);
static LIST_HEAD(s_nsrpc_queue);
static DEFINE_SPINLOCK(s_nsrpc_completion_lock);
static LIST_HEAD(s_nsrpc_completion_queue);
static atomic_t s_completion_count = ATOMIC_INIT(0);

#define NUM_EMERGENCY_TRANSACTIONS		8
#define MAX_TRANSACTION_PAYLOAD			128

static DEFINE_SPINLOCK(s_emergency_transaction_lock);
static LIST_HEAD(s_emergency_transactions);
static NSRPCTransaction_t s_emerg_pool[NUM_EMERGENCY_TRANSACTIONS];

#define NSRPC_MAX_WORKERS		8

static struct task_struct *s_worker_task[NSRPC_MAX_WORKERS];

void nsrpc_handler(NSRPCTransaction_t *txn_object);
#ifdef CONFIG_TEE_LIBRARY_PROVISION
int libprov_handler(NSRPCTransaction_t *txn_object);
#endif

static int nsrpc_worker(void *arg)
{
	unsigned long flags;
	NSRPCTransaction_t *tsx;
	int ret;

	tzlog_print(TZLOG_DEBUG,
		    "Creating nsrpc worker thread %s. Started on CPU #%d\n",
		    current->comm, raw_smp_processor_id());

	while (!kthread_should_stop()) {
		ret = down_timeout(&s_nsrpc_sem, 10 * HZ);

		(void)ret;

		tsx = NULL;

		spin_lock_irqsave(&s_nsrpc_lock, flags);
		if (!list_empty(&s_nsrpc_queue)) {
			tsx =
			    list_entry(s_nsrpc_queue.next,
				       struct NSRPCTransaction_s, tsx_queue);
			list_del(&tsx->tsx_queue);
		}
		spin_unlock_irqrestore(&s_nsrpc_lock, flags);

		if (tsx) {
			tzlog_print(TZLOG_DEBUG,
				    "Got transaction entry %llu in worker %s on CPU #%d\n",
				    tsx->rpc_data.transaction_id, current->comm,
				    raw_smp_processor_id());

			switch (tsx->rpc_data.channel) {
#ifdef CONFIG_TEE_LIBRARY_PROVISION
			case SSTRANSACTION_CHANNEL_LIB_PROV:
				ret = libprov_handler(tsx);
				nsrpc_complete(tsx, ret);
				break;
#endif
#ifndef CONFIG_SECOS_NO_SECURE_STORAGE
			case NSRPC_CHANNEL_SSDEV:
				ssdev_handler(tsx);
				break;
#endif
			case NSRPC_CHANNEL_DUMPDEV:
				dumpdev_handler(tsx);
				break;
			default:
				tzlog_print(TZLOG_ERROR,
					    "Sending SCM transaction to unknown channel %x\n",
					    tsx->rpc_data.channel);
				nsrpc_complete(tsx, -EINVAL);
				break;
			}
		} else {
			/* tzlog_print(TZLOG_DEBUG,
				"Spurious wakeup of nsrpc worker\n"); */
		}
	}

	return 0;
}

int nsrpc_add(const void *buffer, size_t size)
{
	NSRPCTransaction_t *tsx;
	unsigned long flags;

	if (size != sizeof(struct nsrpc_rpc_data)) {
		tzlog_print(TZLOG_ERROR,
			"nsrpc structure incompatibility detected (%zd bytes, should be %zd)\n",
			size, sizeof(struct nsrpc_rpc_data));
		return -EINVAL;
	}

	tsx = kmem_cache_zalloc(s_nsrpc_cache, GFP_ATOMIC);
	if (!tsx) {
		spin_lock_irqsave(&s_emergency_transaction_lock, flags);
		if (!list_empty(&s_emergency_transactions)) {
			tzlog_print(TZLOG_ERROR,
				"Allocating memory from emergency pool for SS Transaction\n");
			tsx = list_entry(s_emergency_transactions.next,
					struct NSRPCTransaction_s, tsx_queue);
			list_del(&tsx->tsx_queue);
		}
		spin_unlock_irqrestore(&s_emergency_transaction_lock, flags);
	}

	if (!tsx) {
		tzlog_print(TZLOG_ERROR,
			    "Can't allocate memory for SS Transaction\n");
		return -ENOMEM;
	}

	memcpy(&tsx->rpc_data, buffer, sizeof(struct nsrpc_rpc_data));

	tzlog_print(TZLOG_DEBUG,
		"Added RPC transaction %llx for channel %x\n",
		tsx->rpc_data.transaction_id,
		tsx->rpc_data.channel);

	spin_lock_irqsave(&s_nsrpc_lock, flags);
	list_add_tail(&tsx->tsx_queue, &s_nsrpc_queue);
	spin_unlock_irqrestore(&s_nsrpc_lock, flags);

	tzlog_print(TZLOG_DEBUG, "Wake up RPC worker\n");

	up(&s_nsrpc_sem);

	return 0;
}

void tzdev_notify_worker(void);

void nsrpc_complete(NSRPCTransaction_t *tsx, int code)
{
	unsigned long flags;

	tzlog_print(TZLOG_DEBUG,
		"Complete transaction %p (%llx) with code %d\n", tsx,
		tsx->rpc_data.transaction_id, code);

	tsx->rpc_data.result = code;

	spin_lock_irqsave(&s_nsrpc_completion_lock, flags);
	list_add_tail(&tsx->tsx_queue, &s_nsrpc_completion_queue);
	spin_unlock_irqrestore(&s_nsrpc_completion_lock, flags);

	atomic_inc(&s_completion_count);

	tzlog_print(TZLOG_DEBUG, "Notify tz worker\n");

	tzdev_notify_worker();
}

int nsrpc_count_completions(void)
{
	return atomic_read(&s_completion_count);
}

int nsrpc_get_next_completion(void *buffer,
				      size_t avail_size,
				      size_t *remaining_size,
				      size_t *obj_size)
{
	unsigned long flags;
	NSRPCTransaction_t *tsx;

	/*
	 tzlog_print(TZLOG_DEBUG,
		"Get next nsrpc completion, buffer = %p, avail = %zd,
		remaining = %zd\n", buffer, avail_size, *remaining_size);
	 */

	if (!atomic_add_unless(&s_completion_count, -1, 0)) {
		/*tzlog_print(TZLOG_DEBUG, "No more completions available\n");*/
		return -ENOENT;
	}

	spin_lock_irqsave(&s_nsrpc_completion_lock, flags);
	if (list_empty(&s_nsrpc_completion_queue)) {
		/*tzlog_print(TZLOG_WARNING, "Completion list empty\n");*/
		spin_unlock_irqrestore(&s_nsrpc_completion_lock, flags);
		return -ENOENT;
	}

	if (avail_size < sizeof(struct nsrpc_rpc_data)) {
		/*
		 * Balance s_completion_count, because we have 1 pending item
		 * and we can't put it right now, so next retry will need this
		 * coubnter value
		 */
		atomic_inc(&s_completion_count);
		spin_unlock_irqrestore(&s_nsrpc_completion_lock, flags);
		return -ENOSPC;
	}

	tsx = list_entry(s_nsrpc_completion_queue.next,
			struct NSRPCTransaction_s, tsx_queue);
	list_del(&tsx->tsx_queue);

	spin_unlock_irqrestore(&s_nsrpc_completion_lock, flags);

	tzlog_print(TZLOG_DEBUG, "Send completion for transaction %llx\n",
		    tsx->rpc_data.transaction_id);

	memcpy(buffer, &tsx->rpc_data, sizeof(struct nsrpc_rpc_data));

	*remaining_size -= sizeof(struct nsrpc_rpc_data);
	*obj_size = sizeof(struct nsrpc_rpc_data);

	if ((unsigned long)tsx >= (unsigned long)s_emerg_pool &&
	    (unsigned long)tsx <
	    (unsigned long)(s_emerg_pool + NUM_EMERGENCY_TRANSACTIONS)) {
		tzlog_print(TZLOG_DEBUG,
			"Put back transaction %p into emergency pool\n", tsx);

		spin_lock_irqsave(&s_emergency_transaction_lock, flags);
		list_add(&tsx->tsx_queue, &s_emergency_transactions);
		spin_unlock_irqrestore(&s_emergency_transaction_lock, flags);
	} else {
		tzlog_print(TZLOG_DEBUG,
			"Free transaction %p using SLAB\n", tsx);

		kmem_cache_free(s_nsrpc_cache, tsx);
	}

	return 0;
}

uint32_t nsrpc_get_arg(NSRPCTransaction_t *tsx, int arg)
{
	return tsx->rpc_data.arg[arg];
}

uint32_t nsrpc_get_command(NSRPCTransaction_t *tsx)
{
	return tsx->rpc_data.command;
}

void nsrpc_set_arg(NSRPCTransaction_t *tsx, int arg, uint32_t value)
{
	tsx->rpc_data.arg[arg] = value;
}

void *nsrpc_payload_ptr(NSRPCTransaction_t *tsx)
{
	return tsx->rpc_data.payload;
}

unsigned int nsrpc_wsm_offset(NSRPCTransaction_t *tsx, size_t *p_size)
{
	*p_size = tsx->rpc_data.wsm_size;
	return tsx->rpc_data.wsm_offset;
}

int __init nsrpc_init_early(void)
{
	int i;

	s_nsrpc_cache = KMEM_CACHE(NSRPCTransaction_s, 0);
	if (!s_nsrpc_cache)
		return -ENOMEM;

	for (i = 0; i < NUM_EMERGENCY_TRANSACTIONS; ++i) {
		list_add_tail(&s_emerg_pool[i].tsx_queue,
			      &s_emergency_transactions);
	}

	return 0;
}

int __init nsrpc_init(void)
{
	int num_workers = 0;
	int cpu;

	for_each_online_cpu(cpu) {
		s_worker_task[num_workers] =
		    kthread_create(nsrpc_worker, NULL,
				   "nsrpc:%d", cpu);

		if (!IS_ERR(s_worker_task[num_workers])) {
			kthread_bind(s_worker_task[num_workers], cpu);
			wake_up_process(s_worker_task[num_workers]);
			++num_workers;
		}
	}

	return 0;
}
