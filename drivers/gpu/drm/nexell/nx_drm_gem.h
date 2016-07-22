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

#ifndef _NX_DRM_GEM_H_
#define _NX_DRM_GEM_H_

#include <drm/drm_gem.h>

/*
 * nexell drm gem object
 */
struct nx_gem_object {
	struct drm_gem_object base;
	dma_addr_t paddr;
	void *vaddr;
	size_t size;
	uint32_t flags;
	struct sg_table *sgt;		 /* for system memory */
	struct sg_table *import_sgt; /* for prime import */
};

static inline struct nx_gem_object *to_nx_gem_obj(struct drm_gem_object *obj)
{
	return container_of(obj, struct nx_gem_object, base);
}

/*
 * struct nx_gem_object elements
 */
struct nx_gem_object *nx_drm_gem_create(struct drm_device *drm,
			size_t size, unsigned int flags);
void nx_drm_gem_destroy(struct nx_gem_object *nx_obj);

/*
 * struct drm_driver elements
 */
int nx_drm_gem_dumb_create(struct drm_file *file_priv,
			struct drm_device *dev,
			struct drm_mode_create_dumb *args);
int nx_drm_gem_dumb_map_offset(struct drm_file *file_priv,
			struct drm_device *dev, uint32_t handle,
			uint64_t *offset);
void nx_drm_gem_free_object(struct drm_gem_object *obj);

struct dma_buf *nx_drm_gem_prime_export(struct drm_device *drm,
			struct drm_gem_object *obj,
			int flags);
struct sg_table *nx_drm_gem_prime_get_sg_table(struct drm_gem_object *obj);
struct drm_gem_object *nx_drm_gem_prime_import_sg_table(
			struct drm_device *dev,
			struct dma_buf_attachment *attach,
			struct sg_table *sgt);

void *nx_drm_gem_prime_vmap(struct drm_gem_object *obj);
void nx_drm_gem_prime_vunmap(struct drm_gem_object *obj, void *vaddr);
int nx_drm_gem_prime_mmap(struct drm_gem_object *obj,
			struct vm_area_struct *vma);
/*
 * struct file_operations elements
 */
int nx_drm_gem_mmap(struct file *filp, struct vm_area_struct *vma);

/*
 * struct vm_operations_struct elements
 */
int nx_drm_gem_fault(struct vm_area_struct *vma, struct vm_fault *vmf);


/*
 * struct drm_ioctl_desc
 */
int nx_drm_gem_create_ioctl(struct drm_device *drm, void *data,
			struct drm_file *file_priv);
int nx_drm_gem_sync_ioctl(struct drm_device *drm, void *data,
			struct drm_file *file_priv);
int nx_drm_gem_get_ioctl(struct drm_device *drm, void *data,
			struct drm_file *file_priv);

#endif
