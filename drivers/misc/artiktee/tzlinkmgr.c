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
#include <linux/smp.h>

#include "circular_buffer.h"
#include "tzdev.h"
#include "tzdev_internal.h"
#include "tzpage.h"
#include "tzdev_smc.h"
#include "tzlog_print.h"

static DEFINE_SPINLOCK(tzio_free_lock);
static LIST_HEAD(tzio_free_links);

static struct tzio_link *tzio_alloc_link(gfp_t gfp)
{
	struct tzio_link *link;

	link = kmalloc(sizeof(struct tzio_link), gfp);
	if (link == NULL) {
		tzlog_print(TZLOG_WARNING,
				"Can't allocate memory for tzio link\n");
		return NULL;
	}

	link->pg = alloc_page(gfp | __GFP_ZERO);

	if (link->pg == NULL) {
		tzlog_print(TZLOG_WARNING,
				"Can't allocate page for tzio link\n");
		kfree(link);
		return NULL;
	}

	link->mux_link = page_address(link->pg);
	link->id = tzswm_register_tzdev_memory(0, &link->pg, 1, gfp, 1);

	if (link->id < 0) {
		tzlog_print(TZLOG_ERROR,
				"Can't register WSM for tzio link (%d)\n",
				link->id);
		__free_page(link->pg);
		kfree(link);
		return NULL;
	}

	return link;
}

struct tzio_link *tzio_acquire_link(gfp_t gfp)
{
	unsigned long flags;
	struct tzio_link *link;
	int try;

	for (try = 0; try < 2; ++try) {
		spin_lock_irqsave(&tzio_free_lock, flags);

		link = list_first_entry_or_null(
				&tzio_free_links,
				struct tzio_link,
				list);

		if (link) {
			list_del(&link->list);
			spin_unlock_irqrestore(&tzio_free_lock, flags);
			return link;
		}

		spin_unlock_irqrestore(&tzio_free_lock, flags);

		link = tzio_alloc_link(try == 0 ? gfp : GFP_ATOMIC);

		if (link != NULL)
			return link;
	}
	return NULL;
}

void tzio_release_link(struct tzio_link *link)
{
	unsigned long flags;

	spin_lock_irqsave(&tzio_free_lock, flags);
	list_add(&link->list, &tzio_free_links);
	spin_unlock_irqrestore(&tzio_free_lock, flags);
}

__init void tzio_link_init(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		struct tzio_link *link = tzio_alloc_link(GFP_KERNEL);

		if (link == NULL)
			link = tzio_alloc_link(GFP_ATOMIC);

		if (link == NULL)
			panic("Failed to allocate initial tzio link\n");
	}
}
