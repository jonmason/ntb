/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <drm/drmP.h>
#include <drm/drm_vma_manager.h>
#include <linux/dma-buf.h>
#include <linux/shmem_fs.h>
#include <drm/nexell_drm.h>

#include "nx_drm_gem.h"

static const char * const gem_type_name[] = {
	[NEXELL_BO_DMA] = "dma, non-cachable",
	[NEXELL_BO_DMA_CACHEABLE] = "dma, cachable",
	[NEXELL_BO_SYSTEM] = "system, non-cachable",
	[NEXELL_BO_SYSTEM_CACHEABLE] = "system, cachable",
	[NEXELL_BO_SYSTEM_NONCONTIG] = "system non-contig, non-cachable",
	[NEXELL_BO_SYSTEM_NONCONTIG_CACHEABLE] = "system non-contig, cachable",
};

#define	LATE_CREATE_MMAP_OFFSET

static int nx_drm_gem_handle_create(struct drm_gem_object *obj,
			struct drm_file *file_priv,
			unsigned int *handle)
{
	int ret;

	/*
	 * allocate a id of idr table where the obj is registered
	 * and handle has the id what user can see.
	 */
	ret = drm_gem_handle_create(file_priv, obj, handle);
	if (ret)
		return ret;

	/* drop reference from allocate - handle holds it now. */
	drm_gem_object_unreference_unlocked(obj);

	return 0;
}

static struct nx_gem_object *nx_drm_gem_object_new(struct drm_device *drm,
			size_t size)
{
	struct nx_gem_object *nx_obj;
	struct drm_gem_object *obj;
	int ret;

	DRM_DEBUG_DRIVER("size:%zu\n", size);

	nx_obj = kzalloc(sizeof(*nx_obj), GFP_KERNEL);
	if (!nx_obj)
		return ERR_PTR(-ENOMEM);

	obj = &nx_obj->base;

	ret = drm_gem_object_init(drm, obj, size);
	if (ret)
		goto error;

#ifndef LATE_CREATE_MMAP_OFFSET
	ret = drm_gem_create_mmap_offset(obj);
	if (ret) {
		drm_gem_object_release(obj);
		goto error;
	}
#endif

	return nx_obj;

error:
	kfree(nx_obj);

	return ERR_PTR(ret);
}

static void nx_drm_gem_object_delete(struct nx_gem_object *nx_obj)
{
	struct drm_gem_object *obj;

	DRM_DEBUG_DRIVER("enter\n");

	obj = &nx_obj->base;

	drm_gem_object_release(obj);

	kfree(nx_obj);
}

static inline bool __gem_is_cacheable(uint32_t flags)
{
	if (flags == NEXELL_BO_DMA_CACHEABLE ||
		flags == NEXELL_BO_SYSTEM_CACHEABLE ||
		flags == NEXELL_BO_SYSTEM_NONCONTIG_CACHEABLE)
		return true;

	return false;
}

static inline bool __gem_is_system(uint32_t flags)
{
	if (flags == NEXELL_BO_SYSTEM ||
		flags == NEXELL_BO_SYSTEM_CACHEABLE ||
		flags == NEXELL_BO_SYSTEM_NONCONTIG ||
		flags == NEXELL_BO_SYSTEM_NONCONTIG_CACHEABLE)
		return true;

	return false;
}

static inline bool __gem_is_system_noncontig(uint32_t flags)
{
	if (flags == NEXELL_BO_SYSTEM_NONCONTIG ||
		flags == NEXELL_BO_SYSTEM_NONCONTIG_CACHEABLE)
		return true;

	return false;
}

static const unsigned int sys_contig_orders[] = {8, 4, 0};
static const int num_orders = ARRAY_SIZE(sys_contig_orders);

static inline unsigned int order_to_size(int order)
{
	return PAGE_SIZE << order;
}

static struct page *__alloc_order_pages(size_t size, unsigned int max_order)
{
	gfp_t high_gfp, low_gfp;
	struct page *page;
	unsigned int order;
	int i;

	high_gfp = (GFP_HIGHUSER | __GFP_ZERO | __GFP_NOWARN |
			     __GFP_NORETRY) & ~__GFP_WAIT;
	low_gfp = (GFP_HIGHUSER | __GFP_ZERO | __GFP_NOWARN);

	for (i = 0; i < num_orders; i++) {
		gfp_t gfp_flags = low_gfp;

		if (order_to_size(sys_contig_orders[i]) > size)
			continue;

		if (sys_contig_orders[i] > max_order)
			continue;

		order = sys_contig_orders[i];
		if (order > 4)
			gfp_flags = high_gfp;

		page = alloc_pages(gfp_flags | __GFP_COMP, order);

	/*
	 * For debug status :
	 * DRM_DEBUG_DRIVER("va:%p, i:%d order:%d,%d, size:%u\n",
	 *		page_address(page), i, max_order, order,
	 *		order_to_size(order));
	 */
		if (!page)
			continue;

		return page;
	}

	return NULL;
}

static void *__drm_gem_sys_remap(struct nx_gem_object *nx_obj, size_t size)
{
	struct scatterlist *sg;
	struct sg_table *sgt = nx_obj->sgt;
	int npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	pgprot_t pgprot;
	void *vaddr = NULL;
	int i, j;

	if (!pages)
		return NULL;

	if (!sgt)
		goto out;

	if (__gem_is_cacheable(nx_obj->flags))
		pgprot = PAGE_KERNEL;
	else
		pgprot = pgprot_writecombine(PAGE_KERNEL);

	for_each_sg(sgt->sgl, sg, sgt->nents, i) {
		int npages_this_entry = PAGE_ALIGN(sg->length) / PAGE_SIZE;
		struct page *page = sg_page(sg);

		BUG_ON(i >= npages);
		for (j = 0; j < npages_this_entry; j++)
			*(tmp++) = page++;
	}

	vaddr = vmap(pages, npages, VM_MAP, pgprot);

out:
	vfree(pages);

	if (!vaddr)
		return ERR_PTR(-ENOMEM);

	DRM_DEBUG_DRIVER("map va:%p, size:%zu\n", vaddr, size);

	memset(vaddr, 0, size);

	return vaddr;
}

static int nx_drm_gem_sys_alloc(struct nx_gem_object *nx_obj,
			size_t size)
{
	struct drm_device *drm;
	dma_addr_t dma_addr;
	struct sg_table *sgt;
	struct scatterlist *sg;
	struct list_head pages;
	struct page *page, *tmp_page;
	size_t remaining = PAGE_ALIGN(size), length;
	unsigned int order = sys_contig_orders[0];
	int i = 0;

	drm = nx_obj->base.dev;
	INIT_LIST_HEAD(&pages);

	while (remaining > 0) {
		page = __alloc_order_pages(remaining, order);
		if (!page)
			goto free_pages;

		list_add_tail(&page->lru, &pages);

		order = compound_order(page);
		length = PAGE_SIZE << order;
		remaining -= length;
		dma_addr = phys_to_dma(drm->dev, page_to_phys(page));

		if (!nx_obj->paddr)
			nx_obj->paddr = dma_addr;

		DRM_DEBUG_DRIVER("[%3d] va:%p, pa:%pad, size:%zu, remain:%zu\n",
			i, page_address(page), &dma_addr, length, remaining);

		i++;
	}

	sgt = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!sgt)
		goto free_pages;

	if (sg_alloc_table(sgt, i, GFP_KERNEL))
		goto free_sgtable;

	sg = sgt->sgl;
	list_for_each_entry_safe(page, tmp_page, &pages, lru) {
		sg_set_page(sg, page, PAGE_SIZE << compound_order(page), 0);
		sg_dma_address(sg) = sg_phys(sg);
		sg = sg_next(sg);
		list_del(&page->lru);
	}

	nx_obj->sgt = sgt;
	nx_obj->vaddr = __drm_gem_sys_remap(nx_obj, size);
	nx_obj->size = size;

	DRM_DEBUG_DRIVER("va:%p, sgt:%p, nents:%d\n",
		nx_obj->vaddr, sgt,  sgt->nents);

	return 0;

free_sgtable:
	kfree(sgt);

free_pages:
	list_for_each_entry_safe(page, tmp_page, &pages, lru)
		__free_pages(page, compound_order(page));

	return -ENOMEM;
}

static void nx_drm_gem_sys_free(struct nx_gem_object *nx_obj)
{
	struct scatterlist *sg;
	struct sg_table *sgt = nx_obj->sgt;
	int i;

	vunmap(nx_obj->vaddr);

	for_each_sg(sgt->sgl, sg, sgt->nents, i) {
		struct page *page = sg_page(sg);

		DRM_DEBUG_DRIVER("[%3d] %p, va:%p, pa:0x%lx, size:%ld\n",
			i, page, page_address(page),
			(unsigned long)phys_to_dma(nx_obj->base.dev->dev,
			virt_to_phys(page_address(page))),
			PAGE_SIZE << compound_order(page));

		__free_pages(page, compound_order(page));
	}

	sg_free_table(sgt);
	kfree(sgt);
}

static int nx_drm_gem_sys_mmap(struct nx_gem_object *nx_obj,
			struct vm_area_struct *vma)
{
	struct sg_table *sgt = nx_obj->sgt;
	unsigned long addr = vma->vm_start;
	unsigned long offset = vma->vm_pgoff * PAGE_SIZE;
	struct scatterlist *sg;
	int i;
	int ret;

	for_each_sg(sgt->sgl, sg, sgt->nents, i) {
		struct page *page = sg_page(sg);
		unsigned long remainder = vma->vm_end - addr;
		unsigned long len = sg->length;

		DRM_DEBUG_DRIVER("vma:0x%lx~0x%lx, va:%p, size:%ld\n",
			addr, vma->vm_end, page_address(page), len);

		if (offset >= sg->length) {
			offset -= sg->length;
			continue;
		} else if (offset) {
			page += offset / PAGE_SIZE;
			len = sg->length - offset;
			offset = 0;
		}

		len = min(len, remainder);
		ret = remap_pfn_range(vma, addr,
				page_to_pfn(page), len, vma->vm_page_prot);
		if (ret)
			return ret;

		addr += len;
		if (addr >= vma->vm_end)
			return 0;
	}

	return 0;
}

static int nx_drm_gem_sys_contig_alloc(
			struct nx_gem_object *nx_obj, size_t size)
{
	struct drm_device *drm;
	struct sg_table *sgt;
	void *cpu_addr;
	dma_addr_t dma_addr;
	struct page *page;
	unsigned long i;
	int order = get_order(size);
	int ret = -ENOMEM;

	drm = nx_obj->base.dev;

	if (order >= MAX_ORDER) {
		dev_err(drm->dev,
			"failed allocate buffer %zu over max order:%d (%d)\n",
			size, order_to_size(MAX_ORDER - 1), MAX_ORDER);
		return ret;
	}

	page = alloc_pages(GFP_HIGHUSER | __GFP_ZERO | __GFP_NOWARN, order);
	if (!page) {
		dev_err(drm->dev,
			"failed allocate buffer %zu, ret:%d\n", size, ret);
		return ret;
	}

	split_page(page, order);

	size = PAGE_ALIGN(size);
	for (i = size >> PAGE_SHIFT; i < (1 << order); i++)
		__free_page(page + i);

	sgt = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!sgt)
		goto free_pages;

	ret = sg_alloc_table(sgt, 1, GFP_KERNEL);
	if (ret)
		goto free_sgtable;

	sg_set_page(sgt->sgl, page, size, 0);
	sg_dma_address(sgt->sgl) = sg_phys(sgt->sgl);

	cpu_addr = page_address(page);
	dma_addr = phys_to_dma(drm->dev, virt_to_phys(cpu_addr));

	/* set gem object params */
	nx_obj->sgt = sgt;
	nx_obj->vaddr = __drm_gem_sys_remap(nx_obj, size);
	nx_obj->paddr = dma_addr;
	nx_obj->size = size;

	DRM_DEBUG_DRIVER("va:%p->%p, pa:%pad, size:%zu\n",
		page_address(page), nx_obj->vaddr, &nx_obj->paddr, size);

	return 0;

free_sgtable:
	kfree(sgt);

free_pages:
	for (i = 0; i < size >> PAGE_SHIFT; i++)
		__free_page(page + i);

	return ret;
}

static void nx_drm_gem_sys_contig_free(struct nx_gem_object *nx_obj)
{
	struct sg_table *sgt = nx_obj->sgt;
	struct page *page = sg_page(sgt->sgl);
	unsigned long pages = PAGE_ALIGN(nx_obj->size) >> PAGE_SHIFT;
	int i;

	DRM_DEBUG_DRIVER("va:%p, pa:%pad, size:%zu\n",
		page_address(page), &nx_obj->paddr, nx_obj->size);

	vunmap(nx_obj->vaddr);

	for (i = 0; i < pages; i++)
		__free_page(page + i);

	sg_free_table(sgt);
	kfree(sgt);
}

static int nx_drm_gem_sys_contig_mmap(struct nx_gem_object *nx_obj,
			struct vm_area_struct *vma)
{
	struct drm_device *drm;
	unsigned long nr_vma_pages, nr_pages;
	unsigned long pfn, off;
	dma_addr_t dma_addr;
	size_t size;
	int ret = -ENXIO;

	drm = nx_obj->base.dev;
	dma_addr = nx_obj->paddr;
	size = vma->vm_end - vma->vm_start;

	DRM_DEBUG_DRIVER("va:%p, pa:%pad, vma:0x%lx~0x%lx, size:%zu\n",
		nx_obj->vaddr, &dma_addr, vma->vm_start, vma->vm_end, size);

	nr_vma_pages = (size) >> PAGE_SHIFT;
	nr_pages = PAGE_ALIGN(size) >> PAGE_SHIFT;
	pfn = dma_to_phys(drm->dev, dma_addr) >> PAGE_SHIFT;
	off = vma->vm_pgoff;

	if (off < nr_pages && nr_vma_pages <= (nr_pages - off)) {
		ret = remap_pfn_range(vma, vma->vm_start,
				      pfn + off, size, vma->vm_page_prot);
	}

	return 0;
}

static int nx_drm_gem_dma_alloc(
			struct nx_gem_object *nx_obj, size_t size)
{
	struct drm_device *drm;
	void *cpu_addr;
	dma_addr_t dma_addr;

	drm = nx_obj->base.dev;

	size = PAGE_ALIGN(size);

	cpu_addr = dma_alloc_writecombine(drm->dev, size, &dma_addr,
				GFP_KERNEL | __GFP_NOWARN);
	if (!cpu_addr) {
		dev_err(drm->dev, "failed to allocate buffer with size %zu\n",
			size);
		return -ENOMEM;
	}

	/* set gem object params */
	nx_obj->vaddr = cpu_addr;
	nx_obj->paddr = dma_addr;
	nx_obj->size = size;

	DRM_DEBUG_DRIVER("va:%p, pa:%pad, size:%zu\n",
			cpu_addr, &dma_addr, size);

	return 0;
}

static void nx_drm_gem_dma_free(struct nx_gem_object *nx_obj)
{
	struct drm_device *drm = nx_obj->base.dev;
	void *cpu_addr;
	dma_addr_t dma_addr;
	size_t size;

	cpu_addr = nx_obj->vaddr;
	dma_addr = nx_obj->paddr;
	size = nx_obj->size;

	DRM_DEBUG_DRIVER("va:%p, pa:%pad, size:%zu\n",
			cpu_addr, &dma_addr, nx_obj->size);

	if (cpu_addr)
		dma_free_writecombine(drm->dev, size, cpu_addr, dma_addr);
}

static int nx_drm_gem_dma_mmap(struct nx_gem_object *nx_obj,
			struct vm_area_struct *vma)
{
	struct drm_device *drm;
	void *cpu_addr;
	dma_addr_t dma_addr;
	size_t size;
	int ret;

	drm = nx_obj->base.dev;
	cpu_addr = nx_obj->vaddr;
	dma_addr = nx_obj->paddr;
	size = vma->vm_end - vma->vm_start;

	DRM_DEBUG_DRIVER("va:%p, pa:%pad, vma:0x%lx~0x%lx, size:%zu\n",
		cpu_addr, &dma_addr, vma->vm_start, vma->vm_end, size);

	ret = dma_mmap_writecombine(drm->dev, vma, cpu_addr, dma_addr, size);
	if (ret)
		drm_gem_vm_close(vma);

	return ret;
}

static int nx_drm_gem_buf_alloc(struct nx_gem_object *nx_obj, size_t size)
{
	uint32_t flags = nx_obj->flags;
	int ret;

	switch (flags) {
	case NEXELL_BO_SYSTEM:
	case NEXELL_BO_SYSTEM_CACHEABLE:
		ret = nx_drm_gem_sys_contig_alloc(nx_obj, size);
		break;

	case NEXELL_BO_SYSTEM_NONCONTIG:
	case NEXELL_BO_SYSTEM_NONCONTIG_CACHEABLE:
		ret = nx_drm_gem_sys_alloc(nx_obj, size);
		break;

	case NEXELL_BO_DMA:
	case NEXELL_BO_DMA_CACHEABLE:
	default:
		ret = nx_drm_gem_dma_alloc(nx_obj, size);
		break;
	}

	return ret;
}

static void nx_drm_gem_buf_free(struct nx_gem_object *nx_obj)
{
	struct drm_gem_object *obj = &nx_obj->base;
	uint32_t flags = nx_obj->flags;

	DRM_DEBUG_DRIVER("va:%p, pa:%pad, attach:%s\n",
		nx_obj->vaddr, &nx_obj->paddr, obj->import_attach ? "o" : "x");

	if (nx_obj->vaddr) {
		switch (flags) {
		case NEXELL_BO_SYSTEM:
		case NEXELL_BO_SYSTEM_CACHEABLE:
			nx_drm_gem_sys_contig_free(nx_obj);
			break;

		case NEXELL_BO_SYSTEM_NONCONTIG:
		case NEXELL_BO_SYSTEM_NONCONTIG_CACHEABLE:
			nx_drm_gem_sys_free(nx_obj);
			break;

		case NEXELL_BO_DMA:
		case NEXELL_BO_DMA_CACHEABLE:
		default:
			nx_drm_gem_dma_free(nx_obj);
			break;
		}
	} else if (obj->import_attach) {
		drm_prime_gem_destroy(obj, nx_obj->import_sgt);
	}
}

static void __vm_set_cache_attr(struct vm_area_struct *vma, uint32_t flags)
{
	bool cached = __gem_is_cacheable(flags);

	if (cached) {
		vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
	} else {
		bool system = __gem_is_system(flags);

		if (system)
			vma->vm_page_prot =
			pgprot_noncached(vm_get_page_prot(vma->vm_flags));
	}

	DRM_DEBUG_DRIVER("flags: 0x%x [%s]\n", flags, gem_type_name[flags]);
}

static int nx_drm_gem_buf_mmap(struct nx_gem_object *nx_obj,
			struct vm_area_struct *vma)
{
	uint32_t flags = nx_obj->flags;
	int ret;

	DRM_DEBUG_DRIVER("va:%p, pa:%pad, s:0x%lx, e:0x%lx, size:%zu\n",
		nx_obj->vaddr, &nx_obj->paddr, vma->vm_start, vma->vm_end,
		nx_obj->size);

	/*
	 * update cache vm prot
	 */
	__vm_set_cache_attr(vma, flags);

	/*
	 * Clear the VM_PFNMAP flag that was set by drm_gem_mmap(), and set the
	 * vm_pgoff (used as a fake buffer offset by DRM) to 0 as we want
	 * to map the whole buffer.
	 */
	vma->vm_flags &= ~VM_PFNMAP;
	vma->vm_pgoff = 0;

	switch (flags) {
	case NEXELL_BO_SYSTEM:
	case NEXELL_BO_SYSTEM_CACHEABLE:
		ret = nx_drm_gem_sys_contig_mmap(nx_obj, vma);
		break;

	case NEXELL_BO_SYSTEM_NONCONTIG:
	case NEXELL_BO_SYSTEM_NONCONTIG_CACHEABLE:
		ret = nx_drm_gem_sys_mmap(nx_obj, vma);
		break;

	case NEXELL_BO_DMA:
	case NEXELL_BO_DMA_CACHEABLE:
	default:
		if (__gem_is_cacheable(flags))
			ret = nx_drm_gem_sys_contig_mmap(nx_obj, vma);
		else
			ret = nx_drm_gem_dma_mmap(nx_obj, vma);
		break;
	}

	return ret;
}

/*
 * struct nx_gem_object elements
 */
struct nx_gem_object *nx_drm_gem_create(struct drm_device *drm,
			size_t size, uint32_t flags)
{
	struct nx_gem_object *nx_obj;
	int ret;

	if (flags > NEXELL_BO_MAX - 1)
		flags = 0;

	DRM_DEBUG_DRIVER("size:%zu, flags:0x%x[%s]\n",
		size, flags, gem_type_name[flags]);

	/*
	 * must be routn up of PAGE_SIZE
	 */
	size = round_up(size, PAGE_SIZE);

	nx_obj = nx_drm_gem_object_new(drm, size);
	if (IS_ERR(nx_obj))
		return nx_obj;

	/*
	 * set memory type and
	 * cache attribute from user side.
	 */
	nx_obj->flags = flags;

	ret = nx_drm_gem_buf_alloc(nx_obj, size);
	if (ret)
		goto error;

	return nx_obj;

error:
	nx_drm_gem_object_delete(nx_obj);

	return ERR_PTR(ret);
}

void nx_drm_gem_destroy(struct nx_gem_object *nx_obj)
{
	DRM_DEBUG_DRIVER("enter\n");

	nx_drm_gem_buf_free(nx_obj);
	nx_drm_gem_object_delete(nx_obj);
}

/*
 * struct drm_driver
 */
int nx_drm_gem_dumb_create(struct drm_file *file_priv,
			struct drm_device *drm,
			struct drm_mode_create_dumb *args)
{
	struct nx_gem_object *nx_obj;
	struct drm_gem_object *obj;
	uint32_t *handle = &args->handle;
	uint32_t flags = args->flags;
	int ret;

	args->pitch = DIV_ROUND_UP(args->width * args->bpp, 8);

	/*
	 * The pitch should be aligned by 8
	 * due to restriction of mali driver
	 */
	args->pitch = ALIGN(args->pitch, 8);
	args->size = args->pitch * args->height;

	DRM_DEBUG_DRIVER("widht:%d, bpp:%d, pitch:%d, flags:0x%x\n",
		args->width, args->bpp, args->pitch, flags);

	/* create gem buffer */
	nx_obj = nx_drm_gem_create(drm, args->size, flags);
	if (IS_ERR(nx_obj))
		return PTR_ERR(nx_obj);

	obj = &nx_obj->base;

	/* create gem handle */
	ret = nx_drm_gem_handle_create(obj, file_priv, handle);
	if (ret)
		goto err_handle_create;

	return 0;

err_handle_create:
	nx_drm_gem_destroy(nx_obj);

	return ret;
}

int nx_drm_gem_dumb_map_offset(struct drm_file *file_priv,
			struct drm_device *drm, uint32_t handle,
			uint64_t *offset)
{
	struct drm_gem_object *obj;
	int ret = 0;

	DRM_DEBUG_DRIVER("enter\n");

	mutex_lock(&drm->struct_mutex);

	obj = drm_gem_object_lookup(drm, file_priv, handle);
	if (!obj) {
		dev_err(drm->dev, "failed to lookup GEM object\n");
		mutex_unlock(&drm->struct_mutex);
		return -EINVAL;
	}

#ifdef LATE_CREATE_MMAP_OFFSET
	/*
	 * create a fake mmap offset for an object
	 */
	ret = drm_gem_create_mmap_offset(obj);
	if (ret)
		goto out;

	*offset = drm_vma_node_offset_addr(&obj->vma_node);

out:
#else
	*offset = drm_vma_node_offset_addr(&obj->vma_node);
#endif

	drm_gem_object_unreference(obj);

	mutex_unlock(&drm->struct_mutex);

	DRM_DEBUG_DRIVER("offset:0x%llx\n", *offset);

	return ret;
}

void nx_drm_gem_free_object(struct drm_gem_object *obj)
{
	DRM_DEBUG_DRIVER("enter\n");
	nx_drm_gem_destroy(to_nx_gem_obj(obj));
}

struct dma_buf *nx_drm_gem_prime_export(struct drm_device *drm,
			struct drm_gem_object *obj,
			int flags)
{
	DRM_DEBUG_DRIVER("enter\n");
	/* we want to be able to write in mmapped buffer */
	flags |= O_RDWR;

	return drm_gem_prime_export(drm, obj, flags);
}

/*
 * provide a new scatter/gather table of pinned pages
 */
struct sg_table *nx_drm_gem_prime_get_sg_table(struct drm_gem_object *obj)
{
	struct list_head pages;
	struct page *page, *tmp_page;
	struct scatterlist *sg;
	struct sg_table *sgt;
	struct nx_gem_object *nx_obj;
	bool noncontig;
	int nents = 1;
	int i = 0;

	nx_obj = to_nx_gem_obj(obj);
	sgt = nx_obj->sgt;

	noncontig = __gem_is_system_noncontig(nx_obj->flags);

	DRM_DEBUG_DRIVER("enter sgt:%p %s\n",
			sgt, noncontig ? "non-contig" : "contig");

	/* Non-Contiguous memory */
	if (noncontig) {
		INIT_LIST_HEAD(&pages);
		nents = sgt->nents;

		for_each_sg(sgt->sgl, sg, nents, i) {
			page = sg_page(sg);
			list_add_tail(&page->lru, &pages);
		}
	}

	/* new scatter/gather table */
	sgt = kzalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt)
		return NULL;

	if (unlikely(sg_alloc_table(sgt, nents, GFP_KERNEL)))
		goto out;

	if (noncontig) {
		sg = sgt->sgl;

		list_for_each_entry_safe(page, tmp_page, &pages, lru) {
			sg_set_page(sg, page,
					PAGE_SIZE << compound_order(page), 0);
			sg_dma_address(sg) = sg_phys(sg);
			sg = sg_next(sg);
			list_del(&page->lru);
			DRM_DEBUG_DRIVER("[%d] sg pa:0x%lx\n",
				i++, (unsigned long)page_to_phys(page));
		}
	} else {
		struct drm_device *drm;
		phys_addr_t phys;

		drm = nx_obj->base.dev;
		phys = dma_to_phys(drm->dev, nx_obj->paddr);

		sg_set_page(sgt->sgl,
			phys_to_page(phys), PAGE_ALIGN(obj->size), 0);

		DRM_DEBUG_DRIVER("sg pa:%pad\n", &nx_obj->paddr);
	}

	/*
	 * will be delete in drm_gem_map_detach
	 */
	return sgt;

out:
	kfree(sgt);
	return NULL;
}

struct drm_gem_object *nx_drm_gem_prime_import_sg_table(
			struct drm_device *dev,
			struct dma_buf_attachment *attach,
			struct sg_table *sgt)
{
	struct nx_gem_object *nx_obj;

	DRM_DEBUG_DRIVER("enter\n");

	/* Create a CMA GEM buffer. */
	nx_obj = nx_drm_gem_object_new(dev, attach->dmabuf->size);
	if (IS_ERR(nx_obj))
		return ERR_CAST(nx_obj);

	nx_obj->paddr = sg_dma_address(sgt->sgl);
	nx_obj->import_sgt = sgt;

	DRM_DEBUG_DRIVER("dma_addr:%pad, size:%zu\n",
			&nx_obj->paddr, attach->dmabuf->size);

	return &nx_obj->base;
}

void *nx_drm_gem_prime_vmap(struct drm_gem_object *obj)
{
	struct nx_gem_object *nx_obj = to_nx_gem_obj(obj);

	DRM_DEBUG_DRIVER("vaddr:%p\n", nx_obj->vaddr);

	return nx_obj->vaddr;
}

void nx_drm_gem_prime_vunmap(struct drm_gem_object *obj, void *vaddr)
{
}

int nx_drm_gem_prime_mmap(struct drm_gem_object *obj,
			struct vm_area_struct *vma)
{
	struct nx_gem_object *nx_obj;
	struct drm_device *drm = obj->dev;
	int ret;

	DRM_DEBUG_DRIVER("enter\n");

	mutex_lock(&drm->struct_mutex);
	ret = drm_gem_mmap_obj(obj, obj->size, vma);
	mutex_unlock(&drm->struct_mutex);
	if (ret < 0)
		return ret;

	nx_obj = to_nx_gem_obj(obj);

	return nx_drm_gem_buf_mmap(nx_obj, vma);
}

/*
 * struct file_operations
 */
int nx_drm_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct nx_gem_object *nx_obj;
	struct drm_gem_object *obj;
	int ret;

	DRM_DEBUG_DRIVER("enter 0x%lx~0x%lx 0x%lx, pgoff 0x%lx\n",
		vma->vm_start, vma->vm_end, vma->vm_end - vma->vm_start,
		vma->vm_pgoff);

	/*
	 * search vma with offset in drm vma manager.
	 */
	ret = drm_gem_mmap(filp, vma);
	if (ret)
		return ret;

	obj = vma->vm_private_data;
	nx_obj = to_nx_gem_obj(obj);

	return nx_drm_gem_buf_mmap(nx_obj, vma);
}

/*
 * struct vm_operations_struct
 */
int nx_drm_gem_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	DRM_DEBUG_DRIVER("enter\n");
	return 0;
}

/*
 * struct drm_ioctl_desc
 */
int nx_drm_gem_create_ioctl(struct drm_device *drm, void *data,
			struct drm_file *file_priv)
{
	struct nx_gem_create *args = data;
	struct nx_gem_object *nx_obj;
	struct drm_gem_object *obj;
	uint32_t *handle = &args->handle;
	uint32_t flags = args->flags;
	size_t size = args->size;
	int ret;

	DRM_DEBUG_DRIVER("size:%lld, flags:0x%x\n", args->size, flags);

	nx_obj = nx_drm_gem_create(drm, size, flags);
	if (IS_ERR(nx_obj))
		return PTR_ERR(nx_obj);

	obj = &nx_obj->base;

	/* create gem handle */
	ret = nx_drm_gem_handle_create(obj, file_priv, handle);
	if (ret)
		goto err_handle_create;

	return 0;

err_handle_create:
	nx_drm_gem_destroy(nx_obj);

	return ret;
}

int nx_drm_gem_sync_ioctl(struct drm_device *drm, void *data,
			struct drm_file *file_priv)
{
	struct nx_gem_object *nx_obj;
	struct drm_gem_object *obj;
	struct nx_gem_create *args = data;
	bool system;

	mutex_lock(&drm->struct_mutex);

	obj = drm_gem_object_lookup(drm, file_priv, args->handle);
	if (!obj) {
		dev_err(drm->dev, "failed to lookup GEM object\n");
		mutex_unlock(&drm->struct_mutex);
		return -EINVAL;
	}

	nx_obj = to_nx_gem_obj(obj);
	system = __gem_is_system(nx_obj->flags);

	DRM_DEBUG_DRIVER("enter flags: 0x%x [%s]\n",
			nx_obj->flags, gem_type_name[nx_obj->flags]);

	/* non-cachable */
	if (!__gem_is_cacheable(nx_obj->flags))
		goto out;

	if (system) {
		struct sg_table *sgt = nx_obj->sgt;

		dma_sync_sg_for_device(NULL,
				sgt->sgl, sgt->nents, DMA_BIDIRECTIONAL);
	} else {
		dma_sync_single_for_device(NULL,
				nx_obj->paddr, nx_obj->size, DMA_BIDIRECTIONAL);
	}

out:
	drm_gem_object_unreference(obj);

	mutex_unlock(&drm->struct_mutex);

	DRM_DEBUG_DRIVER("sync va:%p pa:%pad\n", nx_obj->vaddr, &nx_obj->paddr);

	return 0;
}

int nx_drm_gem_get_ioctl(struct drm_device *drm, void *data,
			struct drm_file *file_priv)
{
	struct nx_gem_info *args = data;
	struct nx_gem_object *nx_obj;
	struct drm_gem_object *obj;

	DRM_DEBUG_DRIVER("enter\n");

	mutex_lock(&drm->struct_mutex);

	obj = drm_gem_object_lookup(drm, file_priv, args->handle);
	if (!obj) {
		dev_err(drm->dev, "failed to lookup GEM object\n");
		mutex_unlock(&drm->struct_mutex);
		return -EINVAL;
	}

	nx_obj = to_nx_gem_obj(obj);
	args->size = obj->size;

	drm_gem_object_unreference(obj);
	mutex_unlock(&drm->struct_mutex);

	DRM_DEBUG_DRIVER("get dma pa:%pad, va:%p\n",
			&nx_obj->paddr, nx_obj->vaddr);

	return 0;
}
