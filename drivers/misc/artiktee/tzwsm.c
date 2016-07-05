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
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "tzdev.h"
#include "tzpage.h"
#include "tzdev_smc.h"
#include "wsm.h"
#include "tzlog_print.h"

int tzswm_register_tzdev_memory(uint64_t ctx_id, struct page **pages,
				size_t num_pages, gfp_t gfp, int for_kernel)
{
	struct page *level0_page = NULL;
	struct page **level1_pages = NULL;
	int error;
	struct ns_level_registration *level0 = NULL;
	size_t num_level1_pages = 0;
	size_t i, j;

	if (unlikely(num_pages == 0))
		return -EINVAL;

	if (unlikely(num_pages > (NS_PAGES_PER_LEVEL * NS_PAGES_PER_LEVEL)))
		return -EINVAL;

	tzlog_print(TZLOG_DEBUG, "Register %s WSM of %zd pages\n",
		    for_kernel ? "kernel" : "user", num_pages);

	level0_page = alloc_page(gfp | __GFP_ZERO);

	if (level0_page == NULL) {
		error = -ENOMEM;
		goto err0;
	}

	level0 = (struct ns_level_registration *)page_address(level0_page);

	level0->num_pfns = num_pages;

	if (for_kernel)
		level0->flags = NS_WSM_FLAG_KERNEL;
	else
		level0->flags = 0;

	if (num_pages > NS_PAGES_PER_LEVEL) {
		num_level1_pages =
		    (num_pages + NS_PAGES_PER_LEVEL - 1) / NS_PAGES_PER_LEVEL;
		level1_pages =
		    (struct page **)kzalloc(sizeof(struct page *) *
					    num_level1_pages, gfp);

		tzlog_print(TZLOG_DEBUG, "Allocate %zd level 1 pages\n",
			    num_level1_pages);

		if (level1_pages == NULL) {
			error = -ENOMEM;
			goto err1;
		}

		for (i = 0; i < num_level1_pages; ++i) {
			level1_pages[i] = alloc_page(gfp | __GFP_ZERO);

			if (!level1_pages[i]) {
				error = -ENOMEM;
				goto err2;
			}

			level0->address[i] = page_to_phys(pages[i]) | NS_PHYS_ADDR_IS_LEVEL_1;
		}

		for (i = 0; i < num_pages;) {
			struct ns_level_registration *level1;

			level1 =
			    (struct ns_level_registration *)
			    page_address(pages[i / NS_PAGES_PER_LEVEL]);
			level1->num_pfns =
			    min(num_pages - i, (size_t)NS_PAGES_PER_LEVEL);

			tzlog_print(TZLOG_DEBUG,
				    "Level 1 Indirection #%zd Address %llx\n",
				    (size_t)(i / NS_PAGES_PER_LEVEL),
				    (uint64_t)
				    page_to_phys(pages
						 [i / NS_PAGES_PER_LEVEL]));

			for (j = 0; j < level1->num_pfns; ++j, ++i) {
				level1->address[j] = page_to_phys(pages[i]);

				tzlog_print(TZLOG_DEBUG,
					    "Page #%zu Address %llx\n", i,
					    (uint64_t) level1->address[j]);
			}
		}
	} else {
		tzlog_print(TZLOG_DEBUG, "No need for level1 pages\n");

		for (i = 0; i < num_pages; ++i) {
			level0->address[i] = page_to_phys(pages[i]);

			tzlog_print(TZLOG_DEBUG, "Page #%zu Address %llx\n", i,
				    (uint64_t) level0->address[i]);
		}
	}

	tzlog_print(TZLOG_DEBUG, "Calling register phys WSM with pfn %llx\n",
		    (uint64_t) page_to_pfn(level0_page));

	error = scm_register_phys_wsm(page_to_pfn(level0_page));

	tzlog_print(TZLOG_DEBUG, "Register phys WSM returned %d\n", error);

err2:
	for (i = 0; i < num_level1_pages; ++i) {
		if (level1_pages[i])
			__free_page(level1_pages[i]);
		else
			break;
	}

	kfree(level1_pages);
err1:
	__free_page(level0_page);
err0:
	return error;
}

int tzwsm_register_kernel_memory(const void *ptr, size_t size, gfp_t gfp)
{
	size_t i;
	int result;
	struct page **pages;

	size = (size + PAGE_SIZE - 1) >> PAGE_SHIFT;

	tzlog_print(TZLOG_DEBUG, "Register kernel memory %p of size %zd\n", ptr,
		    size);

	if (size == 0) {
		tzlog_print(TZLOG_ERROR, "Can't register zero size buffers\n");
		return -EINVAL;
	}

	if (((uintptr_t) ptr) & (PAGE_SIZE - 1)) {
		tzlog_print(TZLOG_ERROR,
			    "Trying to export misaligned kernel address\n");
		return -EINVAL;
	}

	pages = (struct page **)kmalloc(sizeof(struct page *) * size, gfp);

	if (!pages) {
		tzlog_print(TZLOG_ERROR, "Can't allocate export pages\n");
		return -ENOMEM;
	}

	for (i = 0; i < size; ++i) {
		pages[i] =
		    vmalloc_to_page((const void *)((const char *)ptr +
						   (i << PAGE_SHIFT)));

		if (!pages[i]) {
			kfree(pages);
			tzlog_print(TZLOG_ERROR,
				    "Unable to convert ptr %p to physical page\n",
				    (const void *)((const char *)ptr +
						   (i << PAGE_SHIFT)));
			return -EFAULT;
		}
	}

	result = tzswm_register_tzdev_memory(0, pages, size, gfp, 1);

	kfree(pages);

	return result;
}

void tzwsm_unregister_kernel_memory(int wsmid)
{
	scm_unregister_wsm(wsmid);
}

int tzwsm_register_user_memory(uint64_t ctx_id, const void *__user ptr,
			       size_t size, int rw, gfp_t gfp,
			       struct page ***p_pages, size_t *p_num_pages)
{
	struct page **pages;
	size_t num_pages;
	int error;
	size_t i;

	unsigned long ptr_base = ((unsigned long)ptr) & PAGE_MASK;
	unsigned long ptr_end =
	    ((unsigned long)ptr + size + PAGE_SIZE - 1) & PAGE_MASK;

	if (unlikely(!access_ok(rw, ptr, size)))
		return -EFAULT;

	if (unlikely(!p_pages || !p_num_pages))
		return -EINVAL;

	tzlog_print(TZLOG_DEBUG, "Export user mode address %p of size %zd\n",
		    ptr, size);

	num_pages = (ptr_end - ptr_base) >> PAGE_SHIFT;

	pages = (struct page **)kzalloc(sizeof(struct page *) * num_pages, gfp);

	if (!pages)
		return -ENOMEM;

	error =
	    get_user_pages_fast((unsigned long)ptr_base, num_pages, 1, pages);

	if (error == -EFAULT && rw == VERIFY_READ)
		error = get_user_pages_fast(
				(unsigned long)ptr_base, num_pages, 0, pages);

	if (error <= 0) {
		kfree(pages);
		return error == 0 ? -EFAULT : error;
	}

	if ((size_t) error != num_pages) {

		for (i = 0; i < (size_t) error; ++i)
			put_page(pages[i]);

		kfree(pages);
		return -EFAULT;
	}

	error = tzswm_register_tzdev_memory(ctx_id, pages, num_pages, gfp, 0);

	if (error < 0) {
		for (i = 0; i < num_pages; ++i)
			put_page(pages[i]);

		kfree(pages);
		return error;
	}

	*p_pages = pages;
	*p_num_pages = num_pages;

	return error;
}

void tzwsm_unregister_user_memory(int wsmid, struct page **pages,
				  size_t num_pages)
{
	size_t i;

	scm_unregister_wsm(wsmid);

	for (i = 0; i < num_pages; ++i)
		put_page(pages[i]);

	kfree(pages);
}
