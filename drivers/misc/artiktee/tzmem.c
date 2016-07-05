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
#include <linux/slab.h>
#include <linux/highmem.h>
#include <linux/spinlock.h>
#include <linux/radix-tree.h>
#include <linux/rbtree.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/sysfs.h>
#include <linux/idr.h>

#include "tzdev.h"
#include "tzdev_internal.h"
#include "tzlog_print.h"
#include "sstransaction.h"
#include "tzpage.h"
#include "tzdev_smc.h"
#include "tzwsm.h"

static int register_user_memory(struct file *owner,
				unsigned long base,
				size_t size, unsigned long tee_ctx_id, int rw);

static int unregister_user_memory(struct file *owner, int id);

static int verify_client_memory(int id, pid_t tgid, unsigned long tee_ctx_id);

static long tzmem_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case TZMEM_EXPORT_MEMORY:
		{
			struct tzmem_region __user *argp, s;

			argp = (struct tzmem_region __user *)arg;
			if (copy_from_user
			    (&s, argp, sizeof(struct tzmem_region))) {
				ret = -EFAULT;
				break;
			}

			ret = register_user_memory(file,
						   (unsigned long)s.ptr,
						   s.size,
						   s.tee_ctx_id, s.writable);

			if (ret < 0)
				break;

			s.pid = current->tgid;
			s.id = ret;
			ret = 0;

			if (copy_to_user(argp, &s,
						sizeof(struct tzmem_region))) {
				unregister_user_memory(file, s.id);
				ret = -EFAULT;
				break;
			}

			break;
		}
	case TZMEM_RELEASE_MEMORY:
		{
			int id;

			if (copy_from_user(&id,
					(int __user *)arg, sizeof(int))) {
				ret = -EFAULT;
				break;
			}

			ret = unregister_user_memory(file, id);
			break;
		}
	case TZMEM_CHECK_MEMORY:
		{
			struct tzmem_region __user *argp, s;

			argp = (struct tzmem_region __user *)arg;
			if (copy_from_user
			    (&s, argp, sizeof(struct tzmem_region))) {
				ret = -EFAULT;
				break;
			}

			ret = verify_client_memory(s.id, s.pid, s.tee_ctx_id);
			break;
		}
	default:
		tzlog_print(TZLOG_ERROR, "Unknown TZMEM Command: %d\n", cmd);
		ret = -EINVAL;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long tzmem_compat_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case TZMEM_COMPAT_EXPORT_MEMORY:
		{
			struct tzmem_region32 __user *argp, s;

			argp = (struct tzmem_region32 __user *)arg;
			if (copy_from_user
			    (&s, argp, sizeof(struct tzmem_region32))) {
				ret = -EFAULT;
				break;
			}

			ret = register_user_memory(file,
						   (unsigned long)s.ptr,
						   s.size,
						   s.tee_ctx_id, s.writable);

			if (ret < 0)
				break;

			s.pid = current->tgid;
			s.id = ret;
			ret = 0;

			if (copy_to_user(argp, &s,
					sizeof(struct tzmem_region32))) {
				unregister_user_memory(file, s.id);
				ret = -EFAULT;
				break;
			}

			break;
		}
	case TZMEM_RELEASE_MEMORY:
		{
			int id;

			if (copy_from_user(&id,
					(int __user *)arg, sizeof(int))) {
				ret = -EFAULT;
				break;
			}

			ret = unregister_user_memory(file, id);
			break;
		}
	case TZMEM_COMPAT_CHECK_MEMORY:
		{
			struct tzmem_region32 __user *argp, s;

			argp = (struct tzmem_region32 __user *)arg;
			if (copy_from_user
			    (&s, argp, sizeof(struct tzmem_region32))) {
				ret = -EFAULT;
				break;
			}

			ret = verify_client_memory(s.id, s.pid, s.tee_ctx_id);
			break;
		}
	default:
		tzlog_print(TZLOG_ERROR, "Unknown TZMEM Command: %d\n", cmd);
		ret = -EINVAL;
	}

	return ret;
}
#endif

struct tzmem_file {
	struct list_head nodes;
	spinlock_t lock;
};

struct tzmem_registration {
	struct kref kref;
	struct list_head owner_list;	/* Protected by fd lock */
	pid_t tgid;		/* TGID of owner */
	int id;			/* WSM id */
	size_t num_pages;	/* Number of pages */
	struct page **pages;	/* From get_user_pages() */
	unsigned long tee_ctx_id;	/* TEE context ID */
	struct file *owner;	/* File which owns this WSM */
};

static DEFINE_IDR(tzmem_idr);
static DEFINE_RWLOCK(tzmem_idr_lock);
static struct kmem_cache *tzmem_registration_slab;

static void tzmem_registration_free(struct kref *kref)
{
	struct tzmem_registration *node;

	node = container_of(kref, struct tzmem_registration, kref);
	tzlog_print(TZLOG_DEBUG, "Free WSM %d with ptr %p\n", node->id, node);
	tzwsm_unregister_user_memory(node->id, node->pages, node->num_pages);
	kmem_cache_free(tzmem_registration_slab, node);
}

static int tzmem_open(struct inode *inode, struct file *filp)
{
	struct tzmem_file *file;

	file = kmalloc(sizeof(struct tzmem_file), GFP_KERNEL);

	if (!file)
		return -ENOMEM;

	spin_lock_init(&file->lock);
	INIT_LIST_HEAD(&file->nodes);

	tzlog_print(TZLOG_DEBUG, "Open tzmem with %p for TGID %d\n", file,
		    current->tgid);

	filp->private_data = file;
	return 0;
}

static int tzmem_release(struct inode *inode, struct file *owner)
{
	struct tzmem_registration *node, *next_node;
	struct tzmem_file *file = owner->private_data;
	unsigned long flags;

	tzlog_print(TZLOG_DEBUG, "Close tzmem handle %p\n", file);

	list_for_each_entry_safe(node, next_node, &file->nodes, owner_list) {
		list_del(&node->owner_list);
		node->owner = NULL;

		tzlog_print(TZLOG_DEBUG,
			    "Kill mapping of %zd pages in context %lx (WSM %d)\n",
			    node->num_pages, node->tee_ctx_id, node->id);

		/* Remove from IDR */
		write_lock_irqsave(&tzmem_idr_lock, flags);
		idr_remove(&tzmem_idr, node->id);
		write_unlock_irqrestore(&tzmem_idr_lock, flags);

		/* Kill reference of owner */
		kref_put(&node->kref, tzmem_registration_free);
	}

	kfree(file);
	return 0;
}

static int register_user_memory(struct file *owner,
				unsigned long base,
				size_t size, unsigned long tee_ctx_id, int rw)
{
	struct tzmem_registration *node;
	struct tzmem_file *file = owner->private_data;
	unsigned long uaddr = base & PAGE_MASK;
	unsigned long uend = (base + size + PAGE_SIZE - 1) & PAGE_MASK;
	unsigned long flags;
	int error;

	if (uend <= uaddr)
		return -EFAULT;

	if (!access_ok
	    (rw ? VERIFY_WRITE : VERIFY_READ, (const void *)uaddr,
	     uend - uaddr)) {
		return -EFAULT;
	}

	tzlog_print(TZLOG_DEBUG,
		    "Register user memory %p-%p for TEE CTX ID %lx in TGID %d\n",
		    (char *)base, (char *)base + size, tee_ctx_id,
		    current->tgid);

	node = kmem_cache_alloc(tzmem_registration_slab, GFP_KERNEL);

	if (!node) {
		tzlog_print(TZLOG_ERROR,
			    "No memory to allocate tzmem_registration node\n");
		return -ENOMEM;
	}

	kref_init(&node->kref);
	node->tgid = current->tgid;
	node->tee_ctx_id = tee_ctx_id;
	node->owner = owner;

	error = tzwsm_register_user_memory(node->tee_ctx_id,
					   (const void *__user)base,
					   size,
					   rw ? VERIFY_WRITE : VERIFY_READ,
					   GFP_KERNEL,
					   &node->pages, &node->num_pages);

	if (error < 0) {
		tzlog_print(TZLOG_ERROR,
			    "Unable to register user memory to SecureOS\n");
		kmem_cache_free(tzmem_registration_slab, node);
		return error;
	}

	node->id = error;

	idr_preload(GFP_KERNEL);
	write_lock_irqsave(&tzmem_idr_lock, flags);
	error = idr_alloc(&tzmem_idr, node, node->id, node->id + 1, GFP_ATOMIC);
	BUG_ON(error != node->id);
	write_unlock_irqrestore(&tzmem_idr_lock, flags);
	idr_preload_end();

	spin_lock_irqsave(&file->lock, flags);
	list_add_tail(&node->owner_list, &file->nodes);
	spin_unlock_irqrestore(&file->lock, flags);

	tzlog_print(TZLOG_DEBUG,
		    "Node %p registered with ID %d in TEE CTX %lx for TGID %d\n",
		    node, node->id, node->tee_ctx_id, (int)current->tgid);

	return node->id;
}

static int unregister_user_memory(struct file *owner, int id)
{
	unsigned long flags;
	struct tzmem_registration *node;
	struct tzmem_file *file = owner->private_data;

	tzlog_print(TZLOG_DEBUG, "Unregister user memory %d for TGID %d\n", id,
		    (int)current->tgid);

	read_lock_irqsave(&tzmem_idr_lock, flags);
	node = idr_find(&tzmem_idr, id);
	if (node && !kref_get_unless_zero(&node->kref))
		node = NULL;

	read_unlock_irqrestore(&tzmem_idr_lock, flags);

	if (!node) {
		tzlog_print(TZLOG_ERROR, "Unable to find WSM node ID %d\n", id);
		return -EINVAL;
	}

	/* Remove from owner */
	spin_lock_irqsave(&file->lock, flags);
	if (node->owner != owner) {
		tzlog_print(TZLOG_ERROR,
			    "Trying to unregister node for another owner\n");
		spin_unlock_irqrestore(&file->lock, flags);
		kref_put(&node->kref, tzmem_registration_free);
		return -EINVAL;
	}

	node->owner = NULL;
	list_del(&node->owner_list);
	spin_unlock_irqrestore(&file->lock, flags);

	/* Remove from IDR */
	write_lock_irqsave(&tzmem_idr_lock, flags);
	idr_remove(&tzmem_idr, id);
	write_unlock_irqrestore(&tzmem_idr_lock, flags);

	/* Kill reference of owner */
	kref_put(&node->kref, tzmem_registration_free);
	/* Kill reference of unregister_user_memory */
	kref_put(&node->kref, tzmem_registration_free);

	return 0;
}

static int verify_client_memory(int id, pid_t tgid, unsigned long tee_ctx_id)
{
	unsigned long flags;
	struct tzmem_registration *node;
	int ok;

	read_lock_irqsave(&tzmem_idr_lock, flags);
	node = idr_find(&tzmem_idr, id);

	if (node && !kref_get_unless_zero(&node->kref))
		node = NULL;

	read_unlock_irqrestore(&tzmem_idr_lock, flags);

	if (!node) {
		tzlog_print(TZLOG_ERROR, "Unable to find WSM node ID %d\n", id);
		return -EINVAL;
	}

	ok = (node->tee_ctx_id == tee_ctx_id && node->tgid == tgid);

	kref_put(&node->kref, tzmem_registration_free);

	return ok ? 0 : -EINVAL;
}

static const struct file_operations tzmem_fops = {
	.owner = THIS_MODULE,
	.open = tzmem_open,
	.release = tzmem_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tzmem_compat_ioctl,
#endif
	.unlocked_ioctl = tzmem_ioctl,
};

struct miscdevice tzmem = {
	.name = "tzmem",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &tzmem_fops,
	.mode = 0666,
};

__init void tzmem_init(void)
{
	tzmem_registration_slab = KMEM_CACHE(tzmem_registration, SLAB_PANIC);
}
