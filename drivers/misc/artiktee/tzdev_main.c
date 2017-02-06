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
#include <linux/cpu.h>
#include <linux/idr.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/percpu.h>
#include <linux/sysfs.h>
#include <linux/syscore_ops.h>
#include <linux/cpu.h>
#include <linux/cpu_pm.h>
#include <linux/kthread.h>
#include <linux/poll.h>
#include <linux/semaphore.h>
#include <linux/freezer.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/dcache.h>
#include <linux/vmalloc.h>
#ifdef CONFIG_ARTIK_USE_TZCMA_FOR_CRYPTO
#include <linux/dma-contiguous.h>
#include <linux/dma-mapping.h>
#include <linux/sizes.h>
#endif /* CONFIG_ARTIK_USE_TZCMA_FOR_CRYPTO */
#ifdef CONFIG_ARTIK_USE_DMA
#include <linux/dmaengine.h>
#include <linux/property.h>
#endif /* CONFIG_ARTIK_USE_DMA */

#include "tzdev.h"
#include "tzdev_init.h"
#include "tzdev_internal.h"
#include "nsrpc_ree_slave.h"
#include "tzdev_smc.h"
#include "tzdev_plat.h"

#include "ssdev_init.h"

#include "tzlog_core.h"
#include "tzlog_print.h"
#include "tzrsrc_msg.h"

#define TZDEV_MAJOR_VERSION  "007"
#define TZDEV_MINOR_VERSION  "0"
#define TZDEV_DEFAULT_TZPATH "/opt/usr/apps/"

MODULE_PARM_DESC(default_tzdev_local_log_level,
		 "Loglevel of tz driver (7 - debug, 6 - info, 5 - notice, 4 - warning, 3 - error, ...)");

/*#define CONFIG_TZDEV_CPU_IDLE*/
#ifdef CONFIG_TZDEV_CPU_IDLE
void sdp_cpuidle_enable_c1(void);
#endif

char *tzpath_buf;

static int debugging_started;

struct tzio_context {
	struct kref kref;
	int id;
	unsigned int remote_id;
	unsigned int status;
	void *payload;
	size_t payload_size;
	int softlock_timer;
	unsigned int timeout_seconds;
	struct completion comp;
	struct file *owner;
	int state;
};

#define CONTEXT_STATE_INITIALIZING		0
#define CONTEXT_STATE_SUBMIT			1
#define CONTEXT_STATE_COMPLETING		2
#define CONTEXT_STATE_COMPLETE			3

struct tzdev_ipc_data {
	struct semaphore sem;
	struct task_struct *kthread;
};

struct tzdev_notify_target {
	struct list_head list;
	uint32_t target_id;
	tzdev_notify_handler_t handler;
	void *user_data;
};

struct tzdevext_function {
	void *ptr;
	const char *name;
	const char *arg_fmt;
};

/*
 * IPC data for tzdev worker thread
 */
static struct tzdev_ipc_data tzdev_ipc;

static DEFINE_SPINLOCK(tzio_context_slock);
static DEFINE_IDR(tzio_context_idr);
static LIST_HEAD(tzio_context_delete_q);

static DECLARE_WAIT_QUEUE_HEAD(scm_waitq);

static DECLARE_WAIT_QUEUE_HEAD(oom_waitq);

static DECLARE_RWSEM(tzdev_notify_lock);
static LIST_HEAD(tzdev_notifiers);

static struct secos_kern_info secos_kernel_info;

static DECLARE_WAIT_QUEUE_HEAD(tzcpu_wait);
static atomic_t tzcpu_ev_count;

static atomic_t wait_completion_count;

static cpumask_t slab_flush_mask = CPU_MASK_NONE;

static struct kmem_cache *tzio_context_slab;

struct tzio_context *tzio_context_alloc(gfp_t gfp)
{
	struct tzio_context *ctx = kmem_cache_alloc(tzio_context_slab, gfp);

	if (ctx) {
		kref_init(&ctx->kref);
		ctx->id = -1;
		ctx->state = CONTEXT_STATE_INITIALIZING;

		tzlog_print(TZLOG_DEBUG, "Allocate context %p\n", ctx);
	}

	return ctx;
}

void __tzio_context_free(struct kref *kref)
{
	struct tzio_context *ctx =
	    container_of(kref, struct tzio_context, kref);

	tzlog_print(TZLOG_DEBUG, "Free context %p\n", ctx);

	kmem_cache_free(tzio_context_slab, ctx);
}

void tzio_context_put(struct tzio_context *ctx)
{
	kref_put(&ctx->kref, __tzio_context_free);
}

int tzio_context_idr_insert(struct tzio_context *ctx, gfp_t gfp)
{
	unsigned long flags;
	int id;

	idr_preload(gfp);

	spin_lock_irqsave(&tzio_context_slock, flags);

	id = idr_alloc_cyclic(&tzio_context_idr, ctx, 1, 0x1FFFF, GFP_ATOMIC);

	if (id > 0) {
		ctx->id = id;
		tzlog_print(TZLOG_DEBUG,
			    "Insert to IDR context %p with ID %d\n", ctx,
			    ctx->id);
	}

	spin_unlock_irqrestore(&tzio_context_slock, flags);

	idr_preload_end();

	return id;
}

void tzio_context_idr_remove_locked(struct tzio_context *ctx)
{
	if (ctx->id > 0) {
		tzlog_print(TZLOG_DEBUG,
			    "Remove context %p with id %d from IDR\n", ctx,
			    ctx->id);
		idr_remove(&tzio_context_idr, ctx->id);
		ctx->id = -1;
	}
}

void tzio_context_idr_remove(struct tzio_context *ctx)
{
	unsigned long flags;

	spin_lock_irqsave(&tzio_context_slock, flags);
	tzio_context_idr_remove_locked(ctx);
	spin_unlock_irqrestore(&tzio_context_slock, flags);
}

struct tzio_context *tzio_context_get(int id)
{
	struct tzio_context *ctx;

	ctx = idr_find(&tzio_context_idr, id);

	if (ctx && !kref_get_unless_zero(&ctx->kref))
		ctx = NULL;

	return ctx;
}

#ifndef CONFIG_ARM_PSCI
static DEFINE_PER_CPU(unsigned int, cpu_offline);
#define CPU_OFF_LINE  0x1
#define CPU_IDLE_LINE 0x2
#endif /* !CONFIG_ARM_PSCI */

/* tzdev scm_watch
*/
#define DEV_MASK                0xFFFF0000
#define FNCT_MASK               0xFFFF
#define DEVFNCT(dev, fnct)      (((dev << 16) & DEV_MASK) | (fnct & FNCT_MASK))

int tzdev_scm_watch(unsigned long dev_id, unsigned long func_id,
		    unsigned long param1, unsigned long param2,
		    unsigned long param3)
{
	int ret = 0;
	unsigned int cpu = 0;

	unsigned long dev_id_func_id = DEVFNCT(dev_id, func_id);

	cpu = get_cpu();

#ifndef CONFIG_ARM_PSCI
	if (per_cpu(cpu_offline, cpu) & CPU_OFF_LINE
	    || per_cpu(cpu_offline, cpu) & CPU_IDLE_LINE) {
		put_cpu();

		tzlog_print(TZLOG_ERROR, "CPU[%d]  SCM_WATCH FORBIDDEN!\n",
			    cpu);
		tzlog_print(TZLOG_ERROR, "dev_id = %d func_id = %d.\n",
			    (unsigned int)dev_id, (unsigned int)func_id);
		return -EPERM;
	}
#endif /* !CONFIG_ARM_PSCI */

	put_cpu();

	tzlog_print(TZLOG_DEBUG, "SCM_WATCH() %ld, %ld\n", dev_id, func_id);

	ret = scm_watch(dev_id_func_id, param1, param2, param3);

	return ret;
}
EXPORT_SYMBOL(tzdev_scm_watch);

static int tzio_flushd(void *unused)
{
	int ret;
	unsigned long for_cpu = (unsigned long)unused;

	set_freezable();
	tzlog_print(TZLOG_DEBUG, "Start OOM flushed on CPU #%d\n",
		    raw_smp_processor_id());

	while (!kthread_should_stop()) {
		wait_event_freezable(oom_waitq, cpu_online(for_cpu)
				     && cpumask_test_cpu(for_cpu,
							 &slab_flush_mask));
		ret = scm_slab_flush();
		cpumask_clear_cpu(for_cpu, &slab_flush_mask);
		tzlog_print(TZLOG_INFO,
			    "Flushing SLAB on CPU #%d. Got to %d free pages\n",
			    raw_smp_processor_id(), ret);
		(void)ret;
	}

	return 0;
}

static inline void tzio_flushd_notify(void)
{
	cpumask_setall(&slab_flush_mask);
	wake_up_all(&oom_waitq);
}

#ifndef CONFIG_ARM_PSCI
static void tzio_reset_softlock_timer(int cpu)
{
	unsigned long flags;
	struct tzio_context *tmp_node;
	int id;

	spin_lock_irqsave(&tzio_context_slock, flags);
	idr_for_each_entry(&tzio_context_idr, tmp_node, id) {
		tmp_node->softlock_timer = 0;
	}
	spin_unlock_irqrestore(&tzio_context_slock, flags);
}
#endif /* !CONFIG_ARM_PSCI */

void tzio_init_context(struct tzio_context *ctx, struct file *filp,
		       void *payload, uint32_t seconds)
{
	init_completion(&ctx->comp);

	ctx->payload = payload;
	ctx->status = 0;
	ctx->payload_size = 0;
	ctx->softlock_timer = 0;
	ctx->owner = filp;
	ctx->timeout_seconds = seconds;
}

void tzio_rbtree_list_context_node(void)
{
	unsigned long flags;
	struct tzio_context *tmp_node;
	int id, idx = 0;

	tzlog_print(TZLOG_DEBUG, "Start list all context node in rbtree\n");
	spin_lock_irqsave(&tzio_context_slock, flags);
	idr_for_each_entry(&tzio_context_idr, tmp_node, id) {
		tzlog_print(TZLOG_DEBUG, "[%d] local[%d] remote[%d]\n", idx++,
			    tmp_node->id, tmp_node->remote_id);
	}
	spin_unlock_irqrestore(&tzio_context_slock, flags);
	tzlog_print(TZLOG_DEBUG, "End list all context node in rbtree\n");
}

void tzio_connection_closed(int epid)
{
	struct tzio_context *tmp_node;
	int id;
	unsigned long flags;

	tzlog_print(TZLOG_DEBUG, "TA of channel ID %d died\n", epid);

	spin_lock_irqsave(&tzio_context_slock, flags);
	idr_for_each_entry(&tzio_context_idr, tmp_node, id) {
		if (tmp_node->remote_id == epid
		    && tmp_node->state == CONTEXT_STATE_SUBMIT
		    && kref_get_unless_zero(&tmp_node->kref)) {
			tmp_node->status = -EPIPE;
			mb();
			tmp_node->state = CONTEXT_STATE_COMPLETE;
			mb();
			/* Make sure this context is removed
				so nothing else can access it */
			tzio_context_idr_remove_locked(tmp_node);
			complete(&tmp_node->comp);
			tzio_context_put(tmp_node);
		}
	}
	spin_unlock_irqrestore(&tzio_context_slock, flags);
}

void tzio_file_closed(struct file *filp)
{
	struct tzio_context *tmp_node;
	unsigned long flags;
	int id;

	tzlog_print(TZLOG_DEBUG, "File %p died. Kill all contexts\n", filp);

	spin_lock_irqsave(&tzio_context_slock, flags);
	idr_for_each_entry(&tzio_context_idr, tmp_node, id) {
		if (tmp_node->owner == filp
		    && kref_get_unless_zero(&tmp_node->kref)) {
			tzlog_print(TZLOG_DEBUG,
				    "Deleting context %p: local=%d remote=%u because file died\n",
				    tmp_node, tmp_node->id,
				    tmp_node->remote_id);

			/* Make sure this context is removed
				so nothing else can access it */
			tzio_context_idr_remove_locked(tmp_node);
			/* Put reference owned by file */
			tzio_context_put(tmp_node);
			/* Put reference acquired by tzio_file_closed */
			tzio_context_put(tmp_node);
		}
	}
	spin_unlock_irqrestore(&tzio_context_slock, flags);
}

static int tzio_push_message(struct scm_buffer *ring, struct tzio_context *ctx,
			     struct tzio_message *msg, void *payload)
{
	unsigned long chid;
	struct scm_msg_link scm_msg;

	tzlog_print(TZLOG_DEBUG,
		    "CPU #%u: Push message size %u to ep %u. RX size = %u, avail = %u\n",
		    raw_smp_processor_id(), msg->length, msg->endpoint,
		    ring->size, (unsigned int)(SCM_RING_SIZE - ring->size));

	if ((SCM_RING_SIZE - ring->size) <
	    (sizeof(struct scm_msg_link) + msg->length)) {
		tzlog_print(TZLOG_ERROR, "Wait for request channel\n");
		return -EINVAL;
	}

	ctx->remote_id = msg->endpoint;

	chid = ((unsigned long)ctx->id) << PAGE_SHIFT;
	chid = (chid & PAGE_MASK) | ctx->remote_id;

	memset(&scm_msg, 0, sizeof(struct scm_msg_link));
	scm_msg.preamble = SCM_LLC_DATA;
	scm_msg.channel = chid;
	scm_msg.length = msg->length;

	memcpy(ring->buffer + ring->size, &scm_msg,
	       sizeof(struct scm_msg_link));

	if (msg->length > 0) {
		if (copy_from_user
		    (ring->buffer + ring->size + sizeof(struct scm_msg_link),
		     payload, scm_msg.length)) {
			return -EFAULT;
		}
	}

	ring->size += sizeof(struct scm_msg_link) + scm_msg.length;

	mb();

	ctx->state = CONTEXT_STATE_SUBMIT;

	return 0;
}

#define scm_ring_tx_avail(ring)		(SCM_RING_SIZE - (ring)->size)

static int tzio_push_completions(struct scm_buffer *ring)
{
	size_t remaining;
	size_t obj_size;
	int error;
	struct scm_msg_link scm_msg;

	scm_msg.preamble = SCM_LLC_RPC;
	scm_msg.channel = 0;
	scm_msg.length = 0;

	for (;;) {
		if (scm_ring_tx_avail(ring) < sizeof(scm_msg) * 2)
			return -ENOSPC;

		remaining = scm_ring_tx_avail(ring) - sizeof(scm_msg);

		error =
		    nsrpc_get_next_completion(ring->buffer +
						      ring->size +
						      sizeof(scm_msg),
						      remaining, &remaining,
						      &obj_size);

		if (error < 0)
			break;

		scm_msg.length = obj_size;

		memcpy(ring->buffer + ring->size, &scm_msg, sizeof(scm_msg));

		ring->size += sizeof(scm_msg) + obj_size;
	}

	return error == -ENOENT ? 0 : error;
}

static int tzio_pop_message(struct scm_buffer *ring)
{
	struct scm_msg_link mtcp;
	struct tzdev_notify_target *tgt;
	size_t rx_avail = 0;
	size_t rx_taken = 0;
	unsigned long flags;

	rx_avail = ring->size;
	rx_taken = 0;
	ring->size = 0;

	if (ring->flags & SCM_FLAG_LOG_AVAIL)
		tzlog_notify();

	while (rx_avail >= sizeof(struct scm_msg_link)) {
		void *payload = NULL;
		struct tzio_context *ctx;

		memcpy(&mtcp, ring->buffer + rx_taken,
		       sizeof(struct scm_msg_link));

		tzlog_print(TZLOG_DEBUG,
			    "CPU #%d: Received message of %d bytes, type %d, channel %x. RX Avail %zd, RX taken %zd\n",
			    raw_smp_processor_id(), mtcp.length, mtcp.preamble,
			    mtcp.channel, rx_avail, rx_taken);

		if (rx_avail < (sizeof(struct scm_msg_link) + mtcp.length)
		    || mtcp.length > rx_avail) {
			tzlog_print(TZLOG_ERROR,
				    "TX queue for CPU #%d corrupted\n",
				    raw_smp_processor_id());
			ring->size = 0;
			return -EFAULT;
		}

		switch (mtcp.preamble) {
		case SCM_LLC_CONTROL:
			rx_taken += sizeof(struct scm_msg_link) + mtcp.length;
			rx_avail -= sizeof(struct scm_msg_link) + mtcp.length;

			tzio_connection_closed(mtcp.channel & ~PAGE_MASK);
			break;
		case SCM_LLC_DATA:
			spin_lock_irqsave(&tzio_context_slock, flags);

			ctx = tzio_context_get(mtcp.channel >> PAGE_SHIFT);

			if (ctx) {
				payload = ctx->payload;

				/*
				 * Make sure this context is completing
				 * so nothing else can access it
				 */
				if (ctx->state == CONTEXT_STATE_SUBMIT)
					ctx->state = CONTEXT_STATE_COMPLETING;
			} else {
				tzlog_print(TZLOG_ERROR,
					    "can not find ctx mtcp.channel = 0x%x\n",
					    mtcp.channel);
			}

			spin_unlock_irqrestore(&tzio_context_slock, flags);

			/* This doesn't need spinlock */
			if (ctx) {
				if (ctx->state == CONTEXT_STATE_COMPLETING) {
					if (payload) {
						memcpy(payload,
						       ring->buffer + rx_taken +
						       sizeof(struct
							      scm_msg_link),
						       mtcp.length);
						ctx->payload_size = mtcp.length;
					}

					/*
					 * mb() : Ensure status and sleep are
					 * written before completing
					 */
					ctx->status = 0;
					mb();
					ctx->state = CONTEXT_STATE_COMPLETE;
					mb();
					complete(&ctx->comp);
				} else {
					tzlog_print(TZLOG_ERROR,
						    "Completing already finished ctx mtcp.channel = 0x%x\n",
						    mtcp.channel);
				}

				tzio_context_put(ctx);
			}

			rx_taken += sizeof(struct scm_msg_link) + mtcp.length;
			rx_avail -= sizeof(struct scm_msg_link) + mtcp.length;

			break;
		case SCM_LLC_NOTIFY:
			down_read(&tzdev_notify_lock);
			list_for_each_entry(tgt, &tzdev_notifiers, list) {
				if (tgt->target_id == mtcp.channel) {
					(*tgt->handler) (mtcp.channel,
							 ring->buffer +
							 rx_taken +
							 sizeof(struct
								scm_msg_link),
							 mtcp.length,
							 tgt->user_data);
				}
			}
			up_read(&tzdev_notify_lock);

			rx_taken += sizeof(struct scm_msg_link) + mtcp.length;
			rx_avail -= sizeof(struct scm_msg_link) + mtcp.length;
			break;
		case SCM_LLC_RPC:
			nsrpc_add(ring->buffer + rx_taken +
					  sizeof(struct scm_msg_link),
					  mtcp.length);
			rx_taken += sizeof(struct scm_msg_link) + mtcp.length;
			rx_avail -= sizeof(struct scm_msg_link) + mtcp.length;
			break;
		default:
			rx_taken += sizeof(struct scm_msg_link) + mtcp.length;
			rx_avail -= sizeof(struct scm_msg_link) + mtcp.length;
			tzlog_print(TZLOG_ERROR, "Unknown LLC Packet: 0x%x\n",
				    mtcp.preamble);
			break;
		}
	}

	if (rx_avail > 0) {
		tzlog_print(TZLOG_ERROR,
			    "Stray data (%zd bytes) on queue for CPU #%d\n",
			    rx_avail, raw_smp_processor_id());
	}

	return 0;
}

#ifdef CONFIG_TZDEV_EVENT_FORWARD
static irqreturn_t tzdev_ipc_notify(int irq, void *unused)
{
	(void)irq;
	(void)unused;

	tzlog_print(TZLOG_DEBUG, "Got IPI from SWd. Wake up thread\n");

	tzsys_crash_check();

	if (tz_syspage->tzlog_data_avail)
		tzlog_notify();

	up(&tzdev_ipc.sem);
	return IRQ_HANDLED;
}
#endif /* CONFIG_TZDEV_EVENT_FORWARD */

void tzdev_notify_worker(void)
{
	up(&tzdev_ipc.sem);
}

static int tzdev_ipc_thread(void *__arg)
{
	int ret = 0;
	unsigned int svc_flags = 0;
	struct scm_mux_link *ch = NULL;
	struct tzio_link *link = NULL;
	struct tzdev_ipc_data *data = (struct tzdev_ipc_data *)__arg;
	int had_more_completions = 1;
	int chan_wsm_id;

	tzlog_print(TZLOG_DEBUG, "Starting tzdev event thread\n");

	link = tzio_acquire_link(GFP_KERNEL);

	if (link == NULL)
		panic("Can't create tzio link for worker\n");

	ch = link->mux_link;
	chan_wsm_id = link->id;

	for (;;) {
		mb();

		if (!had_more_completions &&
		    !tz_syspage->transaction_data_avail &&
		    !nsrpc_count_completions()) {
			/* wakeup every 1s if no command received */
			ret = down_timeout(&data->sem, HZ * 10);

			/* when software assisted timer is used,
				always call SecOS on worker */
			if ((ret == -ETIME) &&
			    !atomic_read(&wait_completion_count) &&
			    !secos_kernel_info.soft_timer &&
			    !tz_syspage->transaction_data_avail &&
			    !nsrpc_count_completions()) {
				continue;
			}
		}

		if (kthread_should_stop())
			break;

		/* Ignore other semaphore down calls */
		while (down_trylock(&data->sem) == 0)
			;	/* NULL */

		mb();
		tz_syspage->transaction_data_avail = 0;
		mb();

		svc_flags =
		    SCM_PROCESS_RX | SCM_PROCESS_TX | SCM_PROCESS_DPC |
		    SCM_PROCESS_RESCHED;

		do {
			if (tzio_push_completions(&ch->in) == -ENOSPC)
				had_more_completions = 1;
			else
				had_more_completions = 0;

			/* Invoke secure environment */
			ret = scm_invoke_svc(chan_wsm_id, svc_flags);

			tzsys_crash_check();

			if (ret == -EINTR) {
				/* No need to do anything */
			} else if (ret == -EFAULT) {
				tzlog_notify();
				tzlog_print(TZLOG_ERROR,
					    "TZIO Link buffer corrupted\n");
			} else if (ret == -ENODEV) {
				tzlog_notify();
				tzlog_print(TZLOG_ERROR, "TRM died !!!\n");
			} else if (ret == -ENOMEM
				   || (ch->out.flags & SCM_FLAG_LOW_MEMORY)) {
				tzlog_notify();
				tzlog_print(TZLOG_ERROR,
					    "SWD has No memory to process messages or OOM detected\n");
				tzio_flushd_notify();

				if (ret == -ENOMEM) {
					set_current_state(TASK_INTERRUPTIBLE);
					schedule_timeout(HZ / 10);
				}
			} else if (ret == -ENOSPC) {
				tzlog_notify();
				tzlog_print(TZLOG_ERROR,
					    "No memory rx messages from swd\n");
			} else if (ret < 0) {
				tzlog_notify();
				tzlog_print(TZLOG_ERROR,
					    "Invalid SCM response: 0x%x\n",
					    ret);
			}

			tzio_pop_message(&ch->out);
		} while (ret == -ENOSPC || ret == -ENOMEM || ret == -EINTR);
	}

	tzlog_print(TZLOG_DEBUG, "Exiting tzdev event thread\n");

	return 0;
}

#define TZIO_PAYLOAD_MAX	256

int tzio_message_wait(struct tzio_message *__user msg,
		      struct tzio_context *context)
{
	int ret;

	mb();

	while (context->state != CONTEXT_STATE_COMPLETE) {
		atomic_inc(&wait_completion_count);
		up(&tzdev_ipc.sem);
		ret =
		    wait_for_completion_interruptible_timeout(&context->comp,
							      HZ);
		atomic_dec(&wait_completion_count);

		mb();

		/* Check if we can complete anyway */
		if (context->state == CONTEXT_STATE_COMPLETE)
			break;

		if (ret < 0) {
			tzlog_print(TZLOG_DEBUG,
				    "Syscall trigger for context %d\n", ret);
			/* We may get syscall */
			if (unlikely(msg->boost_flag))
				plat_postprocess();

			return -EINTR;
		}

		if (ret == 0) {
			if (secos_kernel_info.soft_timer)
				scm_soft_timer();

			/*
			* For debbuging purpose disable soft
			* lockup detection.
			* soft lockup checking start
			*/
			if (!debugging_started) {
				if (context->softlock_timer++ > 20) {
					tzlog_notify();

					tzlog_print(TZLOG_INFO,
						    "tzdaemon thread has been waiting %d > 20 s for PID %u on context %d, state = %d\n",
						    context->softlock_timer,
						    context->remote_id,
						    context->id,
						    context->state);

					if (!(context->softlock_timer % 5))
						scm_softlockup();
				}

				if ((context->timeout_seconds > 0) &&
					(context->softlock_timer >
						context->timeout_seconds)) {
					tzlog_print(TZLOG_ERROR,
						"TA %u timed out. Killing all requests\n",
						context->remote_id);
					tzio_connection_closed(context->remote_id);
					break;
				}
			}
			/* soft lockup checking end */
		}

		mb();
	}

	ret = context->status;

	/* Clear context ID restart */
	put_user(0, &msg->context_id);

	if (context->payload_size) {
		if (copy_to_user(
		    msg->payload, context->payload, context->payload_size)) {
			tzlog_print(TZLOG_ERROR,
				    "Can't copy data back to userspace\n");
			ret = -EFAULT;
		}
	}

	tzio_context_idr_remove(context);
	tzio_context_put(context);

	if (unlikely(msg->boost_flag))
		plat_postprocess();

	return ret;
}

int tzio_message_restart(struct tzio_message *__user msg, struct file *filp,
			 int context_id)
{
	struct tzio_context *context;
	unsigned long flags;
	int ret;

	tzlog_print(TZLOG_DEBUG, "Restart waiting for context %d\n",
		    context_id);

	spin_lock_irqsave(&tzio_context_slock, flags);

	context = tzio_context_get(context_id);

	if (context == NULL) {
		spin_unlock_irqrestore(&tzio_context_slock, flags);
		tzlog_print(TZLOG_DEBUG,
			    "Unable to finish waiting for context %d - doesn't exist\n",
			    context_id);
		return -EINVAL;
	}

	if (context->owner != filp) {
		spin_unlock_irqrestore(&tzio_context_slock, flags);
		tzlog_print(TZLOG_DEBUG,
			    "Unable to finish waiting for context %d - file mismatch\n",
			    context_id);
		return -EINVAL;
	}

	spin_unlock_irqrestore(&tzio_context_slock, flags);

	ret = tzio_message_wait(msg, context);

	tzio_context_put(context);

	return ret;
}

int tzio_exchange_message(struct file *filp, struct tzio_message *__user msg)
{
	int ret = 0;
	struct tzio_context *context;
	struct tzio_link *link;
	struct scm_mux_link *ch;
	struct tzio_message tzio_msg;
	unsigned int svc_flags;
	char __payload[TZIO_PAYLOAD_MAX];
	int32_t restart_context_id;
	uint32_t timeout_seconds;

	ret = get_user(restart_context_id, &msg->context_id);

	if (ret < 0) {
		tzlog_print(TZLOG_ERROR, "get_user error\n");
		return ret;
	}

	ret = get_user(timeout_seconds, &msg->timeout_seconds);

	if (ret < 0) {
		tzlog_print(TZLOG_ERROR, "get_user error\n");
		return ret;
	}

	if (restart_context_id)
		return tzio_message_restart(msg, filp, restart_context_id);

	if (copy_from_user(&tzio_msg, msg, sizeof(struct tzio_message))) {
		tzlog_print(TZLOG_ERROR, "copy_from_user error\n");
		return -EFAULT;
	}

	/*Just in order to clear prevent warning. */
	if (tzio_msg.length == 0 || tzio_msg.length > 0x10000000) {
		tzlog_print(TZLOG_ERROR, "tzio_msg.length is illegal\n");
		return -EFAULT;
	}

	if (tzio_msg.length > 0
	    && !access_ok(VERIFY_WRITE, msg->payload, tzio_msg.length)) {
		tzlog_print(TZLOG_ERROR, "tzio_msg.length is illegal\n");
		return -EFAULT;
	}

	if (tzio_msg.length > TZIO_PAYLOAD_MAX)
		return -EFAULT;

	context = tzio_context_alloc(GFP_KERNEL);

	if (context == NULL) {
		tzlog_print(TZLOG_WARNING,
			    "No memory to allocate tzio context\n");
		return -ENOMEM;
	}

	tzio_init_context(context, filp, __payload, timeout_seconds);

	link = tzio_acquire_link(GFP_KERNEL);

	if (link == NULL) {
		tzlog_print(TZLOG_ERROR,
			    "Can't obtain tzio link for message exchange. System out of memory\n");
		tzio_context_put(context);
		return -ENOMEM;
	}

	ch = link->mux_link;

	/*
	 * Queue must be empty here
	 */
	BUG_ON(ch->out.size != 0);
	BUG_ON(ch->in.size != 0);

	ret = tzio_context_idr_insert(context, GFP_KERNEL);

	if (ret < 0) {
		tzlog_print(TZLOG_ERROR,
			    "Unable to insert tzio context into IDR (%d)\n",
			    ret);
		tzio_release_link(link);
		tzio_context_put(context);
		return ret;
	}

	ret = put_user(context->id, &msg->context_id);

	if (ret < 0) {
		tzlog_print(TZLOG_ERROR, "Can't update context_id\n");
		tzio_release_link(link);
		tzio_context_idr_remove(context);
		tzio_context_put(context);
		return ret;
	}

	ret = tzio_push_message(&ch->in, context, &tzio_msg, msg->payload);

	if (ret != 0) {
		tzio_release_link(link);
		tzio_context_idr_remove(context);
		tzio_context_put(context);
		return ret;
	}

	tzio_push_completions(&ch->in);

	svc_flags = SCM_PROCESS_RX | SCM_PROCESS_TX | SCM_PROCESS_DPC;

	if (unlikely(msg->boost_flag))
		plat_preprocess();

	do {
		/* Invoke secure environment */
		ret = scm_invoke_svc(link->id, svc_flags);

		/*
		 * Ring buffer might be corrupted
		 */
		BUG_ON(ch->out.size > SCM_RING_SIZE);
		BUG_ON(ch->in.size > SCM_RING_SIZE);

		tzsys_crash_check();

		if (ret == -EFAULT) {
			tzlog_print(TZLOG_ERROR,
				    "TZIO Link buffer %p corrupted\n",
				    link->mux_link);

			ch->in.size = 0;
			ch->out.size = 0;

			tzlog_notify();

			goto out;
		}

		if (ret == -ENODEV) {
			tzlog_notify();
			tzlog_print(TZLOG_ERROR, "TRM died !!!\n");

			goto out;
		}

		if (ret == -ENOMEM || (ch->out.flags & SCM_FLAG_LOW_MEMORY)) {
			tzlog_notify();
			tzio_flushd_notify();

			tzlog_print(TZLOG_ERROR,
				    "No memory to process TA messages or system is OOM\n");

			if (ret == -ENOMEM) {
				ret = -ENOSPC;

				tzio_pop_message(&ch->out);

				set_current_state(TASK_INTERRUPTIBLE);
				schedule_timeout(HZ / 10);
			}
		}

		if (ret < 0 && ret != -ENOSPC && ret != -EAGAIN
		    && ret != -EINTR) {
			tzlog_notify();
			tzlog_print(TZLOG_ERROR, "Invalid SCM response: 0x%x\n",
				    ret);
			goto out;
		}

		tzio_pop_message(&ch->out);

		BUG_ON(ch->out.size != 0);
	} while (ret == -ENOSPC || ret == -EAGAIN
	       || (ret == -EINTR && context->state < CONTEXT_STATE_COMPLETING));

out:

	/*
	 * Mame sure all messages were popped
	 */
	BUG_ON(ch->out.size != 0);
	BUG_ON(ch->in.size != 0);

	tzio_release_link(link);

	if (context->state != CONTEXT_STATE_COMPLETE)
		tzdev_notify_worker();

	if (signal_pending(current)) {
		if (unlikely(msg->boost_flag))
			plat_postprocess();
		return -EINTR;
	}

	return tzio_message_wait(msg, context);
}

int tzdev_register_notify_handler(uint32_t target_id,
				  tzdev_notify_handler_t handler,
				  void *user_data)
{
	struct tzdev_notify_target *tgt = NULL;

	tgt =
	    (struct tzdev_notify_target *)
	    kmalloc(sizeof(struct tzdev_notify_target), GFP_KERNEL);
	if (tgt == NULL)
		return -ENOMEM;

	if (!try_module_get(THIS_MODULE)) {
		kfree(tgt);
		return -EBUSY;
	}

	tgt->target_id = target_id;
	tgt->handler = handler;
	tgt->user_data = user_data;

	down_write(&tzdev_notify_lock);
	list_add_tail(&tgt->list, &tzdev_notifiers);
	up_write(&tzdev_notify_lock);

	return 0;
}
EXPORT_SYMBOL(tzdev_register_notify_handler);

int tzdev_unregister_notify_handler(uint32_t target_id,
				    tzdev_notify_handler_t handler,
				    void *user_data)
{
	struct tzdev_notify_target *tgt = NULL;
	int error = -EINVAL;

	down_write(&tzdev_notify_lock);

	list_for_each_entry(tgt, &tzdev_notifiers, list) {
		if (tgt->target_id == target_id && tgt->handler == handler
		    && tgt->user_data == user_data) {
			list_del(&tgt->list);
			kfree(tgt);
			error = 0;
			break;
		}
	}

	up_write(&tzdev_notify_lock);

	if (error == 0)
		module_put(THIS_MODULE);

	return error;
}
EXPORT_SYMBOL(tzdev_unregister_notify_handler);

static long tzio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case TZIO_SMC:
		{
			struct tzio_message __user *msg;

			msg = (struct tzio_message __user *)arg;
			ret = tzio_exchange_message(file, msg);
			break;
		}
	case TZIO_DBG_START:
		debugging_started = arg;
		break;

	case TZIO_GET_INFO:
		{
			struct secos_kern_info kinfo = {};
			struct tzio_info tz_info = {};
			kinfo.size = sizeof(kinfo);
			kinfo.abi = SECOS_ABI_VERSION;
			ret = scm_query_kernel_info(&kinfo);
			strncpy(tz_info.secos_build_id, kinfo.build_id,
				sizeof(tz_info.secos_build_id) - 1);
#ifdef CONFIG_MODULE_BUILD_VCS_ID
			strncpy(tz_info.linux_module_build_id,
				CONFIG_MODULE_BUILD_VCS_ID,
				sizeof(tz_info.linux_module_build_id) - 1);
#endif
			strncpy(tz_info.machine_name, kinfo.machine_name,
				sizeof(tz_info.machine_name) - 1);
			strncpy(tz_info.secos_build_type, kinfo.build_type,
				sizeof(tz_info.secos_build_type) - 1);
#ifdef CONFIG_MODULE_BUILD_TYPE
			strncpy(tz_info.linux_module_build_type,
				CONFIG_MODULE_BUILD_TYPE,
				sizeof(tz_info.linux_module_build_type) - 1);
#endif

			ret = copy_to_user((void *)arg, &tz_info,
					sizeof(tz_info));
			break;
		}

	case TZIO_SYNC_TIME:
		{
			scm_sync_kernel_time();
			break;
		}

	default:
		tzlog_print(TZLOG_ERROR, "Unknown TZIO Command: %d\n", cmd);
		ret = -EINVAL;
	}

	return ret;
}

static ssize_t tzio_read(struct file *filp, char *buf, size_t count,
			 loff_t *f_pos)
{
	if (!count)
		return 0;

	if (!access_ok(VERIFY_WRITE, buf, count))
		return -EFAULT;

	atomic_set(&tzcpu_ev_count, 0);

	return count;
}

static int tzio_mmap(struct file *file, struct vm_area_struct *vma)
{
	return -EINVAL;
}

static int tzio_release(struct inode *inode, struct file *filp)
{
	tzlog_print(TZLOG_INFO,
		    "tzdaemon has been crashed,start notify secure os\n");
	tzio_file_closed(filp);
	scm_tzdaemon_dead(0);
	tzlog_notify();
	return 0;
}

static int tzio_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static unsigned int tzio_poll(struct file *file, struct poll_table_struct *wait)
{
	poll_wait(file, &tzcpu_wait, wait);
	return atomic_read(&tzcpu_ev_count) > 0 ? POLLIN | POLLRDNORM : 0;
}

static int tzlog_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int tzlog_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long tzlog_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static ssize_t tzlog_read(struct file *filp, char *buf, size_t count,
			  loff_t *f_pos)
{
	return 0;
}

static ssize_t tzlog_write(struct file *filp, const char *buf, size_t count,
			   loff_t *f_pos)
{
	return 0;
}

#ifndef CONFIG_ARM_PSCI
static int scm_cpu_shutdown(unsigned long cpu)
{
	int error = 0;

	error = scm_cpu_suspend();
	if (error != 0) {
		tzlog_print(TZLOG_ERROR,
			    "SecOS can't shut down CPU %d - error %d\n",
			    (int)cpu, error);
	}

	/*Record cpu satus after suspend/resume/hotplug */
	per_cpu(cpu_offline, cpu) |= CPU_OFF_LINE;

	return error;
}

#ifdef CONFIG_TZDEV_CPU_IDLE
static int scm_cpu_shutdown_for_idle(unsigned long cpu, unsigned long flags)
{
	int error = 0;

	error = scm_cpu_notify(CPU_OP_SUSPEND);
	if (error != 0) {
		tzlog_print(TZLOG_EMERG,
			    "SecOS can't shut down CPU %d - error %d\n",
			    (int)cpu, error);
	}

	/*Record cpu satus after suspend/resume/hotplug */
	per_cpu(cpu_offline, cpu) |= CPU_IDLE_LINE;

	return error;
}
#endif

static int tzdev_pm_suspend(void)
{
	int ret = 0;
	unsigned long cpu = 0;

	cpu = get_cpu();
	tzlog_print(TZLOG_DEBUG, "tzdev_pm_suspend cpu\n");

	ret = scm_sys_suspend();

	if (ret != 0) {
		put_cpu();
		return ret;
	}

	scm_cpu_shutdown(cpu);
	put_cpu();

	return 0;
}

static void tzdev_pm_resume(void)
{
	unsigned int cpu = 0;

	tzlog_print(TZLOG_DEBUG, "tzdev_pm_resume cpu\n");
	scm_sys_resume();

	/*Record cpu satus after suspend/resume/hotplug */
	cpu = get_cpu();
	tzio_reset_softlock_timer(cpu);
	per_cpu(cpu_offline, cpu) &= ~CPU_OFF_LINE;
	put_cpu();
}

static int tzdev_cpu_notify(struct notifier_block *self, unsigned long action,
			    void *hcpu)
{
	int ret = NOTIFY_OK;
	unsigned long cpu = (unsigned long)hcpu;

	switch (action & ~CPU_TASKS_FROZEN) {
	case CPU_STARTING:
		tzlog_print(TZLOG_INFO,
			    "tzdev_cpu_notify cpu(CPU_STARTING): %d, action=%ld\n",
			    (int)cpu, action);
		scm_cpu_resume();
		break;
	case CPU_ONLINE:
		tzlog_print(TZLOG_INFO,
			    "tzdev_cpu_notify cpu(CPU_ONLINE): %d\n", (int)cpu);

		/*Record cpu satus after suspend/resume/hotplug */
		per_cpu(cpu_offline, cpu) &= ~CPU_OFF_LINE;

		atomic_inc(&tzcpu_ev_count);
		wake_up_interruptible(&tzcpu_wait);
		break;
	case CPU_DYING:
		tzlog_print(TZLOG_INFO, "tzdev_cpu_notify cpu(CPU_DYING): %d\n",
			    (int)cpu);

		ret = notifier_from_errno(scm_cpu_shutdown(cpu));
		break;
	default:
		break;
	}

	return ret;
}

#ifdef CONFIG_TZDEV_CPU_IDLE
static int tzdev_cpu_idle_notify(struct notifier_block *self,
				 unsigned long action, void *hcpu)
{
	int ret = NOTIFY_OK;
	unsigned long flags = 0;
	unsigned long cpu = (unsigned long)get_cpu();

	put_cpu();

	switch (action) {
	case CPU_PM_EXIT:
		tzlog_print(TZLOG_DEBUG,
			    "tzdev_cpu_idle_notify(CPU_PM_EXIT) cpu: %d\n",
			    (int)cpu);
		if (!(per_cpu(cpu_offline, cpu) & CPU_OFF_LINE)) {
			/*Record cpu satus after suspend/resume/hotplug */
			per_cpu(cpu_offline, cpu) &= ~CPU_IDLE_LINE;

			atomic_inc(&tzcpu_ev_count);
			wake_up_interruptible(&tzcpu_wait);
		} else {
			tzlog_print(TZLOG_DEBUG,
				    "tzdev_cpu_idle_notify(CPU_PM_EXIT) not run / cpu: %d\n",
				    (int)cpu);
		}
		break;

	case CPU_PM_ENTER:
		tzlog_print(TZLOG_DEBUG,
			    "tzdev_cpu_idle_notify(CPU_PM_ENTER) cpu: %d\n",
			    (int)cpu);
		if (!(per_cpu(cpu_offline, cpu) & CPU_OFF_LINE)) {
			ret = scm_cpu_shutdown_for_idle(cpu, flags);
		} else {
			tzlog_print(TZLOG_DEBUG,
				    "tzdev_cpu_idle_notify(CPU_PM_ENTER) not run / cpu: %d\n",
				    (int)cpu);
		}
		break;

	default:
		break;
	}

	return ret;
}
#endif

static struct syscore_ops tzdev_pm_syscore_ops = {
	.suspend = tzdev_pm_suspend,
	.resume = tzdev_pm_resume
};

static struct notifier_block tzdev_cpu_block = {
	.notifier_call = tzdev_cpu_notify
};

#ifdef CONFIG_TZDEV_CPU_IDLE
static struct notifier_block tzdev_cpu_idle_block = {
	.notifier_call = tzdev_cpu_idle_notify
};
#endif
#endif /* !CONFIG_ARM_PSCI */

static const struct file_operations tzdev_fops = {
	.owner = THIS_MODULE,
	.open = tzio_open,
	.release = tzio_release,
	.mmap = tzio_mmap,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tzio_ioctl,
#endif
	.unlocked_ioctl = tzio_ioctl,
	.poll = tzio_poll,
	.read = tzio_read
};

static const struct file_operations tzlog_fops = {
	.owner = THIS_MODULE,
	.open = tzlog_open,
	.read = tzlog_read,
	.write = tzlog_write,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tzlog_ioctl,
#endif
	.unlocked_ioctl = tzlog_ioctl,
	.release = tzlog_release,
};

static struct miscdevice tzdev = {
	.name = "tzdev",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &tzdev_fops,
};

#ifndef CONFIG_ARM_PSCI
static ssize_t tzpm_show(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	return 0;
}

static ssize_t tzpm_store(struct kobject *kobj, struct kobj_attribute *attr,
			  const char *buf, size_t count)
{
	int var;

	sscanf(buf, "%d", &var);
	switch (var) {
	case 0:
		tzdev_pm_suspend();
		break;
	case 1:
		tzdev_pm_resume();
		break;
	default:
		break;
	}

	return count;
}
#endif /* !CONFIG_ARM_PSCI */

static int tzpath_subpath_create(const char *dir_full_path)
{
	struct dentry *dentry;
	struct path p;
	int err;
	struct inode *inode;

	dentry =
	    kern_path_create(AT_FDCWD, dir_full_path, &p, LOOKUP_DIRECTORY);

	if (IS_ERR(dentry))
		return PTR_ERR(dentry);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
	inode = p.dentry->d_inode;
#else
	inode = d_inode(p.dentry);
#endif

	err = vfs_mkdir(inode, dentry, S_IRWXU | S_IRWXG | S_IRWXO);

	done_path_create(&p, dentry);
	return err;
}

int tzpath_fullpath_create(const char *dir_path)
{
	int ret = 0;
	int index = 0;
	int all_len = 0;
	char *full_path = NULL;
	char *parent_dir = NULL;

	if (tzpath_buf == NULL) {
		tzlog_print(K_ERR,
			"Failed to create tzpath, tz rootpath is NULL\n");
		return -EINVAL;
	}

	if (dir_path == NULL) {
		tzlog_print(K_ERR, "dir_path param is NULL\n");
		return -EINVAL;
	}

	parent_dir = kmalloc(PATH_MAX, GFP_KERNEL);
	if (parent_dir == NULL) {
		tzlog_print(K_ERR, "vmalloc failed\n");
		ret = -ENOMEM;
		goto error_exit;
	}

	full_path = kmalloc(PATH_MAX, GFP_KERNEL);
	if (full_path == NULL) {
		tzlog_print(K_ERR, "vmalloc failed\n");
		ret = -ENOMEM;
		goto error_exit;
	}

	snprintf(full_path, PATH_MAX, "%s/%s", tzpath_buf, dir_path);
	all_len = strlen(full_path);

	for (index = 0; index < all_len; index++) {
		if (full_path[index] == '/' && index != 0) {
			memset(parent_dir, 0, PATH_MAX);
			memcpy(parent_dir, full_path, index);
			ret = tzpath_subpath_create(parent_dir);
		}
	}

	if (ret != 0 && ret != -EEXIST)
		tzlog_print(K_ERR,
				"Failed to mkdir fullpath, path : %s, err: %d",
				full_path, ret);
	else
		ret = 0;

error_exit:
	kfree(full_path);
	kfree(parent_dir);

	return ret;
}


static int tzpath_alloc(const char *path, ssize_t sz)
{
	int ret;
	char *old_tzpath = tzpath_buf;

	if (path == NULL)
		return -EINVAL;

	tzpath_buf = kstrndup(path, sz, GFP_KERNEL);

	if (tzpath_buf == NULL) {
		ret = -ENOMEM;
		goto error;
		return -ENOMEM;
	}

	kfree(old_tzpath);

	return 0;

error:
	tzpath_buf = old_tzpath;
	return ret;
}

static ssize_t tzdev_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	return snprintf(buf, 256, "TZDEV object test\n");
}

static ssize_t tzdev_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t count)
{
#ifdef CONFIG_TZDEV_DEBUG

	int var;

	sscanf(buf, "%d", &var);

	tzlog_print(TZLOG_DEBUG, "Fastcall ret = 0x%x\n",
		    tzdev_scm_watch(0, var, 1, 2, 3));
	return count;
#else
	return 0;
#endif
}

static int check_privilege(void)
{
	if (!uid_eq(current_euid(), GLOBAL_ROOT_UID))
		return -1;

	if (strncmp(current->comm, "tzdaemon", 9) != 0)
		return -1;

	return 0;
}

static ssize_t tzpath_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	int len;

	len = strlen(tzpath_buf);

	return sprintf(buf, "%s\n", tzpath_buf);
}

static ssize_t tzpath_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t count)
{
	int rc;
	struct path file_path;

	if (buf == NULL || count > 128)
		return -EINVAL;

	if (check_privilege() != 0) {
		tzlog_print(TZLOG_ERROR, "Permission denied\n");
		return -EPERM;
	}

	rc = kern_path(buf, LOOKUP_FOLLOW, &file_path);

	if (rc < 0) {
		tzlog_print(TZLOG_ERROR, "Failed to open path : %s err:%d\n",
			buf, rc);
		return rc;
	}
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 15, 0))
	if (!d_is_dir(file_path.dentry)) {
		tzlog_print(TZLOG_ERROR, "Failed to access path : %s\n", buf);
		path_put(&file_path);
		return -ENOENT;
	}
#endif

	path_put(&file_path);

	rc = tzpath_alloc(buf, count);
	if (rc != 0) {
		tzlog_print(TZLOG_ERROR, "Failed to setup tzpath : %s\n", buf);
		return -EINVAL;
	}

	if (storage_path_init() != 0)
		return -EIO;

	return count;
}



static ssize_t tzlog_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	return snprintf(buf, 256, "current loglevel = %d\n",
			default_tzdev_local_log_level);
}

static ssize_t tzlog_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t count)
{
	int var;

	sscanf(buf, "%d", &var);
	tzlog_print(K_DEBUG, "Change Loglevel = %d\n", var);
	if (var >= K_EMERG && var <= K_DEBUG)
		default_tzdev_local_log_level = var;

	return count;
}

static ssize_t tzmem_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	return snprintf(buf, 256, "TZMEM Show Test\n");
}


static ssize_t tzmem_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t count)
{
	int var;

	sscanf(buf, "%d", &var);
	tzlog_print(TZLOG_DEBUG, "TZMEM store test : %d\n", var);

	return count;
}

#ifdef CONFIG_TZDEV_L2X0_SEC_DISABLE
typedef void (*l2x0_disable_sec_fn) (void);
extern l2x0_disable_sec_fn l2x0_disable_sec;
static void tzdev_l2x0_disable_sec(void)
{

	int ret = 0;

	scm_cache_notify(0);
}

static void tzdev_set_l2x0_ctrl(void)
{
	l2x0_disable_sec = tzdev_l2x0_disable_sec;
	tzlog_print(TZLOG_DEBUG, "Set 1 l2x0_disable_sec to 0x%x\n",
		    l2x0_disable_sec);
}
#endif

#define tzdev_attr(_name)			\
static struct kobj_attribute _name##_attr =	\
	_ATTR(_name, 0644. _name##_show, _name##_store)

static struct kobj_attribute tzdev_attr =
__ATTR(tzdev, 0660, tzdev_show, tzdev_store);

static struct kobj_attribute tzlog_attr =
__ATTR(tzlog, 0660, tzlog_show, tzlog_store);

static struct kobj_attribute tzmem_attr =
__ATTR(tzmem, 0660, tzmem_show, tzmem_store);

static struct kobj_attribute tzpath_attr =
__ATTR(tzpath, 0600, tzpath_show, tzpath_store);

#ifndef CONFIG_ARM_PSCI
static struct kobj_attribute tzpm_attr =
__ATTR(tzpm, 0660, tzpm_show, tzpm_store);
#endif /* !CONFIG_ARM_PSCI */

static struct attribute *tzdev_attrs[] = {
	&tzdev_attr.attr,
	&tzlog_attr.attr,
	&tzmem_attr.attr,
	&tzpath_attr.attr,
#ifndef CONFIG_ARM_PSCI
	&tzpm_attr.attr,
#endif /* !CONFIG_ARM_PSCI */
	NULL,
};

static struct attribute_group tzdev_attr_group = {
	.attrs = tzdev_attrs,
};

static struct kobject *tzdev_kobj;

#ifdef CONFIG_TZDEV_EVENT_FORWARD
extern irqreturn_t(*secos_hook) (int irq, void *unused);
#endif
static int fetch_kernel_info(void)
{
	struct secos_kern_info kinfo;
	int rc;

	memset(&kinfo, 0, sizeof(kinfo));

	kinfo.size = sizeof(kinfo);
	kinfo.abi = SECOS_ABI_VERSION;
#ifdef CONFIG_TZDEV_EVENT_FORWARD
	kinfo.ipi_fiq = 0xb;
#else
	kinfo.ipi_fiq = ~0;
#endif

	rc = scm_query_kernel_info(&kinfo);
	if (rc)
		return rc;

	if (kinfo.size != sizeof(kinfo)) {
		tzlog_print(TZLOG_INFO,
			    "Kernel info size mismatch detected\n");
		return -EINVAL;
	}

	if (kinfo.abi != SECOS_ABI_VERSION) {
		tzlog_print(TZLOG_INFO,
			    "Unsupported ABI version. Outdated tzdev\n");
		return -EINVAL;
	}

	memcpy(&secos_kernel_info, &kinfo, sizeof(kinfo));

	return rc;
}

extern struct miscdevice tzmem;
extern struct miscdevice tzrsrc;

#ifdef CONFIG_ARTIK_USE_TZCMA_FOR_CRYPTO
#define TZCMA_MEMORY_ALLOC	0
#define TZCMA_MEMORY_FREE	1
#define TZCMA_LLI_TBL_PAGE	4
#define TZCMA_ALLOC_MAX_COUNT	4
#define TZCMA_MAX_SUPPORT_SIZE	SZ_4M
#define TZCMA_UNMMAPED	0
#define TZCMA_MMAPED	1

static struct device *tzcma_dev;
struct tzcma_info {
	int chan_id;
	u64 size;
	u64 virtAddr;
};
struct tzcma_info g_tzcmas[TZCMA_ALLOC_MAX_COUNT];
struct dma_chan *g_tzcma_channels[TZCMA_ALLOC_MAX_COUNT];

struct tzcma_info_internal {
	int is_mmaped;
	dma_addr_t phyAddr;
	void *cpuAddr;
};
static struct tzcma_info_internal g_tzcmas_internal[TZCMA_ALLOC_MAX_COUNT];

int g_tzcma_alloc_cnt;
bool g_tzcma_state_opened;

#ifdef CONFIG_ARTIK_USE_DMA
static bool tzcma_filter(struct dma_chan *chan, void *param)
{
	if (strcmp(dev_name(chan->device->dev), "c0001000.pl08xdma") == 0
			&& (strcmp(dma_chan_name(chan), "dma2chan0") == 0
				|| strcmp(dma_chan_name(chan), "dma2chan1") == 0
				|| strcmp(dma_chan_name(chan), "dma2chan2") == 0
				|| strcmp(dma_chan_name(chan), "dma2chan3") == 0
				|| strcmp(dma_chan_name(chan), "dma2chan4") == 0
				|| strcmp(dma_chan_name(chan), "dma2chan5") == 0
				|| strcmp(dma_chan_name(chan), "dma2chan6") == 0
				|| strcmp(dma_chan_name(chan), "dma2chan7") == 0
			   ))
		return true;
	return false;
}
#endif /* CONFIG_ARTIK_USE_DMA*/

static int tzcma_get_alloc_idx(void)
{
	int i;

	for (i = 0; i < TZCMA_ALLOC_MAX_COUNT; i++) {
		if (g_tzcmas_internal[i].phyAddr == 0)
			return i;
	}
	return -1;
}

static int tzcma_get_free_idx(struct tzcma_info mem)
{
	int i;

	for (i = 0; i < TZCMA_ALLOC_MAX_COUNT; i++) {
		if (g_tzcmas[i].chan_id == mem.chan_id
			&& g_tzcmas[i].size == mem.size)
			return i;
	}
	return -1;
}

static void tzcma_free(int idx)
{
	int dma_free_size;

	if (idx < 0 || idx >= TZCMA_ALLOC_MAX_COUNT) {
		tzlog_print(TZLOG_ERROR,
				"tzcma_free invaild index\n");
		return;
	}

	dma_free_size = round_up(g_tzcmas[idx].size, PAGE_SIZE) +
				TZCMA_LLI_TBL_PAGE * PAGE_SIZE;
	dma_free_writecombine(tzcma_dev, dma_free_size,
		g_tzcmas_internal[idx].cpuAddr, g_tzcmas_internal[idx].phyAddr);

#ifdef CONFIG_ARTIK_USE_DMA
	if (g_tzcma_channels[idx] != NULL) {
		dma_release_channel(g_tzcma_channels[idx]);
		g_tzcma_channels[idx] = NULL;
	}
#endif /* CONFIG_ARTIK_USE_DMA*/

	memset(&g_tzcmas[idx], 0, sizeof(struct tzcma_info));
	memset(&g_tzcmas_internal[idx], 0, sizeof(struct tzcma_info_internal));
}

static int tzcma_alloc(int idx, size_t size)
{
	int ret = 0;
	int dma_alloc_size;
	void *cpuAddr;
	dma_addr_t phyAddr;

#ifdef CONFIG_ARTIK_USE_DMA
	dma_cap_mask_t mask;
#endif /* CONFIG_ARTIK_USE_DMA*/

	if (idx < 0 || idx >= TZCMA_ALLOC_MAX_COUNT) {
		tzlog_print(TZLOG_ERROR,
				"tzcma_alloc invaild index\n");
		return -EINVAL;
	}

	dma_alloc_size = round_up(size, PAGE_SIZE) +
				TZCMA_LLI_TBL_PAGE * PAGE_SIZE;

	cpuAddr = dma_alloc_writecombine(tzcma_dev, dma_alloc_size,
			&phyAddr, GFP_KERNEL | GFP_DMA);

	if (cpuAddr == NULL) {
		tzlog_print(TZLOG_ERROR, "dma alloc failed\n");
		return -ENOMEM;
	}
	g_tzcmas[idx].size = size;
	g_tzcmas_internal[idx].is_mmaped = TZCMA_UNMMAPED;
	g_tzcmas_internal[idx].phyAddr = phyAddr;
	g_tzcmas_internal[idx].cpuAddr = cpuAddr;

#ifdef CONFIG_ARTIK_USE_DMA
	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);
	g_tzcma_channels[idx] = dma_request_channel(mask, tzcma_filter, NULL);
	if (g_tzcma_channels[idx] == NULL) {
		tzlog_print(TZLOG_ERROR, "dma request channel failed\n");
		tzcma_free(idx);
		return -ENODEV;
	}
	g_tzcmas[idx].chan_id = g_tzcma_channels[idx]->chan_id;
#endif /* CONFIG_ARTIK_USE_DMA*/

	return ret;
}

static int tzcma_open(struct inode *inode, struct file *file)
{
	int i;
	int ret = 0;

	if (g_tzcma_state_opened) {
		tzlog_print(TZLOG_ERROR, "tzcma already opened\n");
		return -EMFILE;
	}

	g_tzcma_state_opened = true;
	for (i = 0; i < TZCMA_ALLOC_MAX_COUNT; i++) {
		memset(&g_tzcmas[i], 0, sizeof(struct tzcma_info));
		memset(&g_tzcmas_internal[i], 0,
				sizeof(struct tzcma_info_internal));
#ifdef CONFIG_ARTIK_USE_DMA
		g_tzcma_channels[i] = NULL;
#endif /* CONFIG_ARTIK_USE_DMA*/
	}
	g_tzcma_alloc_cnt = 0;
	return ret;
}

static int tzcma_mmap(struct file *file, struct vm_area_struct *vma)
{
	int i;
	int idx = -1;
	int ret = 0;
	void *cpu_addr;
	dma_addr_t dma_addr;
	size_t size;

	size = vma->vm_end - vma->vm_start;

	for (i = 0; i < TZCMA_ALLOC_MAX_COUNT; i++) {
		if ((g_tzcmas_internal[i].is_mmaped == TZCMA_UNMMAPED)
			&& (size == round_up(g_tzcmas[i].size, PAGE_SIZE))) {
			g_tzcmas_internal[i].is_mmaped = TZCMA_MMAPED;
			idx = i;
			break;
		}
	}
	if (idx < 0 || idx >= TZCMA_ALLOC_MAX_COUNT) {
		tzlog_print(TZLOG_ERROR, "address is not allocated\n");
		return -ENXIO;
	}

	cpu_addr = g_tzcmas_internal[idx].cpuAddr;
	dma_addr = g_tzcmas_internal[idx].phyAddr;
	ret = dma_mmap_writecombine(tzcma_dev, vma, cpu_addr, dma_addr, size);

	return ret;
}

static long tzcma_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int idx, ret = 0;
	struct tzcma_info mem, *argp;

	argp = (struct tzcma_info *)arg;
	if (copy_from_user(&mem, argp, sizeof(struct tzcma_info))) {
		tzlog_print(TZLOG_ERROR,
			"copy_from_user error\n");
		return -EFAULT;
	}

	switch (cmd) {
	case TZCMA_MEMORY_ALLOC:
		if (g_tzcma_alloc_cnt >= TZCMA_ALLOC_MAX_COUNT) {
			tzlog_print(TZLOG_ERROR,
				"can't cma memory allocated\n");
			return -ENOMEM;
		}
		if (mem.size > TZCMA_MAX_SUPPORT_SIZE) {
			tzlog_print(TZLOG_ERROR,
				"can't support 4MB over size\n");
			return -EINVAL;
		}
		idx = tzcma_get_alloc_idx();
		ret = tzcma_alloc(idx, mem.size);
		if (ret >= 0) {
			if (copy_to_user(argp, &g_tzcmas[idx],
					sizeof(struct tzcma_info))) {
				tzlog_print(TZLOG_ERROR,
					"copy_to_user error\n");
				return -EFAULT;
			}
		}
		g_tzcma_alloc_cnt++;
		break;
	case TZCMA_MEMORY_FREE:
		if (g_tzcma_alloc_cnt < 0) {
			tzlog_print(TZLOG_ERROR,
				"no cma memory allocated\n");
			return -EFAULT;
		}
		tzcma_free(tzcma_get_free_idx(mem));
		g_tzcma_alloc_cnt--;
		break;
	default:
		return ret;
	}
	return ret;
}

static int tzcma_release(struct inode *inode, struct file *file)
{
	int i;
	int ret = 0;

	for (i = 0; i < TZCMA_ALLOC_MAX_COUNT; i++) {
		if (g_tzcmas_internal[i].phyAddr != 0)
			tzcma_free(i);
	}
	g_tzcma_alloc_cnt = 0;
	g_tzcma_state_opened = false;
	return ret;
}

static const struct file_operations tzcma_fops = {
	.owner = THIS_MODULE,
	.open = tzcma_open,
	.mmap = tzcma_mmap,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tzcma_ioctl,
#endif
	.unlocked_ioctl = tzcma_ioctl,
	.release = tzcma_release,
};

static struct miscdevice tzcma = {
	.name = "tzcma",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &tzcma_fops,
};

static int tzcma_init(void)
{
	int ret = -1;

	g_tzcma_state_opened = false;

	ret = misc_register(&tzcma);
	if (unlikely(ret)) {
		pr_err("failed to register tzcma device!\n");
		goto tzcma_out;
	}

	tzcma_dev = tzcma.this_device;
	tzcma_dev->coherent_dma_mask = ~0;

#ifdef CONFIG_ARTIK_USE_DMA
	/*
	 * if you want using tzdev module,
	 * It should be modified as follows.
	 * adding EXPORT_SYMBOL_GPL(arch_setup_dma_ops);
	 * in linux-artik7/arch/arm64/mm/dma-mapping.c
	*/
	arch_setup_dma_ops(tzcma_dev, 0, 0, NULL, 0);
#endif /* CONFIG_ARTIK_USE_DMA*/

	return ret;

tzcma_out:
	misc_deregister(&tzcma);

	return ret;
}
#endif /* CONFIG_ARTIK_USE_TZCMA_FOR_CRYPTO */

static int __init init_tzdev(void)
{
	int rc;
	int cpu;

#ifdef CONFIG_TZDEV_DEBUG
	init_smc_status();
#endif

	rc = smc_init_monitor();
	if (rc < 0) {
		tzlog_print(TZLOG_INFO,
		       "Unable to initialize monitor connection\n");
		return rc;
	}

	rc = fetch_kernel_info();
	if (unlikely(rc)) {
		tzlog_print(TZLOG_INFO,
			    "Failed to fetch kernel info. Probably incorrect SwD version\n");
		return rc;
	}

	rc = misc_register(&tzdev);
	if (unlikely(rc))
		goto err1;

	rc = tzpath_alloc(TZDEV_DEFAULT_TZPATH, strlen(TZDEV_DEFAULT_TZPATH));
	if (unlikely(rc))
		goto tzpath_out;

	if (storage_path_init() != 0) {
		rc = -EIO;
		goto tzpath_out;
	}

	rc = misc_register(&tzmem);
	if (unlikely(rc))
		goto tzdev_out;

#ifdef CONFIG_RESOURCE_MONITOR
	rc = misc_register(&tzrsrc);
	if (unlikely(rc)) {
		goto tzdev_out;
	}
#endif
#ifdef CONFIG_ARTIK_USE_TZCMA_FOR_CRYPTO
	tzcma_init();
#endif

	tzlog_print(TZLOG_INFO, "tzdev version [%s.%s]\n",
			TZDEV_MAJOR_VERSION, TZDEV_MINOR_VERSION);
	tzdev_kobj = kobject_create_and_add("tzdev", NULL);
	if (!tzdev_kobj) {
		rc = -EINVAL;
		goto tzmem_out;
	}

	rc = sysfs_create_group(tzdev_kobj, &tzdev_attr_group);
	if (rc) {
		rc = -EINVAL;
		goto sysfs_out;
	}

	sema_init(&tzdev_ipc.sem, 0);
	atomic_set(&wait_completion_count, 0);

	tzio_context_slab = KMEM_CACHE(tzio_context, SLAB_PANIC);

	tzmem_init();

	tzio_link_init();
	/*
	 * SS transaction must be initialized before event threads
	 * as it will handle incoming requests from boot time of SecOS
	 */
	nsrpc_init_early();

	tzsys_init();

	tzdev_ipc.kthread =
	    kthread_run(tzdev_ipc_thread, &tzdev_ipc, "tzdev-event");
	if (IS_ERR(tzdev_ipc.kthread)) {
		tzlog_print(TZLOG_ERROR, "Can't spawn kernel worker thread\n");
		rc = PTR_ERR(tzdev_ipc.kthread);
		goto sysfs_out;
	}
#ifndef CONFIG_ARM_PSCI
	register_syscore_ops(&tzdev_pm_syscore_ops);

	register_cpu_notifier(&tzdev_cpu_block);

#ifdef CONFIG_TZDEV_CPU_IDLE
	cpu_pm_register_notifier(&tzdev_cpu_idle_block);
#endif
#endif /* !CONFIG_ARM_PSCI */

#ifdef CONFIG_TZDEV_EVENT_FORWARD
	secos_hook = tzdev_ipc_notify;
#endif

#ifdef CONFIG_TZDEV_L2X0_SEC_DISABLE
	tzdev_set_l2x0_ctrl();
#endif

#ifndef CONFIG_SECOS_NO_SECURE_STORAGE
	init_storage();
#endif

	nsrpc_init();

	for_each_possible_cpu(cpu) {
		struct task_struct *thr = kthread_create(tzio_flushd,
							 (void *)(unsigned long)
							 cpu,
							 "tzioflush:%d",
							 cpu);
		if (IS_ERR(thr))
			panic("Can't create tzioflushd\n");

		kthread_bind(thr, cpu);
		wake_up_process(thr);
	}

	tzlog_init(secos_kernel_info.build_info);

	plat_init();


#ifdef CONFIG_TZDEV_CPU_IDLE
	sdp_cpuidle_enable_c1();
	tzlog_print(TZLOG_DEBUG, "sdp_cpuidle_enable_c1 called\n");
#endif

	return 0;

sysfs_out:
	kobject_put(tzdev_kobj);

tzmem_out:
	misc_deregister(&tzmem);

tzdev_out:
	misc_deregister(&tzdev);

tzpath_out:
	kfree(tzpath_buf);
err1:

	return rc;
}

static void __exit exit_tzdev(void)
{
	tzlog_print(TZLOG_INFO, "tzdev is non removable\n");
}

module_init(init_tzdev);
module_exit(exit_tzdev);

MODULE_AUTHOR("Jaemin Ryu <jm77.ryu@samsung.com>");
MODULE_DESCRIPTION("TZDEV driver");
MODULE_LICENSE("GPL");
