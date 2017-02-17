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

#include <linux/mm.h>
#include <linux/io.h>

#include "tzdev.h"
#include "tzdev_internal.h"
#include "tzpage.h"

/*
 * Emergency memory pool for allocating memory
 * when Linux kernel fails to provide non-movable
 * page. Must be provided by SecOS
 */
static struct resource *emerg_pool_resource;
static void *emerg_pool_base;
static phys_addr_t emerg_pool_phys;

void tzdev_free_watch_page(tzdev_page_handle h)
{
	__free_page((struct page *)h);
}
EXPORT_SYMBOL(tzdev_free_watch_page);

tzdev_page_handle tzdev_alloc_watch_page(void)
{
	return (tzdev_page_handle) alloc_page(GFP_ATOMIC);
}
EXPORT_SYMBOL(tzdev_alloc_watch_page);

void *tzdev_get_virt_addr(tzdev_page_handle h)
{
	return page_address((struct page *)h);
}
EXPORT_SYMBOL(tzdev_get_virt_addr);

phys_addr_t tzdev_get_phys_addr(tzdev_page_handle h)
{
	return page_to_phys((struct page *)h);
}
EXPORT_SYMBOL(tzdev_get_phys_addr);

void tzpage_init(phys_addr_t phys_addr, size_t size)
{
	emerg_pool_phys = phys_addr;
	emerg_pool_resource = request_mem_region(phys_addr, size, "NwD_Shared");

	if (!emerg_pool_resource)
		panic("Can't obtain shared tz resource\n");
	else {
#if defined(ioremap_cached)
		emerg_pool_base =
		    (void *)ioremap_cached(emerg_pool_resource->start, size);
#else
		/* For kernels 4.x */
		emerg_pool_base =
		    (void *)ioremap_cache(emerg_pool_resource->start, size);
#endif

		if (!emerg_pool_base)
			panic("Can't ioremap shared tz resource\n");
	}
}
