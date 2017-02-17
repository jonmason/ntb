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

#ifndef _UAPI_NX_DRM_H_
#define _UAPI_NX_DRM_H_

#include <drm/drm.h>

/**
 * User-desired buffer creation information structure.
 *
 * @size: user-desired memory allocation size.
 *	- this size value would be page-aligned internally.
 * @flags: user request for setting memory type or cache attributes.
 * @handle: returned a handle to created gem object.
 *	- this handle will be set by gem module of kernel side.
 */
struct nx_gem_create {
	uint64_t size;
	unsigned int flags;
	unsigned int handle;
	void *priv_data;
};

/**
 * A structure to gem information.
 *
 * @handle: a handle to gem object created.
 * @flags: flag value including memory type and cache attribute and
 *	this value would be set by driver.
 * @size: size to memory region allocated by gem and this size would
 *	be set by driver.
 */
struct nx_gem_info {
	unsigned int handle;
	unsigned int flags;
	uint64_t size;
};

/*
 * nexell gem memory type
 */
enum nx_gem_type {
	/*
	 * DMA continuous memory
	 * user   : non-cacheable
	 * kernel : non-cacheable
	 */
	NEXELL_BO_DMA,

	/*
	 * DMA continuous memory, allocate from DMA,
	 * user   : cacheable
	 * kernel : non-cacheable
	 */
	NEXELL_BO_DMA_CACHEABLE,

	/*
	 * System continuous memory, allocate from system
	 * user   : non-cacheable
	 * kernel : non-cacheable
	 */
	NEXELL_BO_SYSTEM,

	/*
	 * System continuous memory, allocate from system
	 * user   : cacheable
	 * kernel : cacheable
	 */
	NEXELL_BO_SYSTEM_CACHEABLE,

	/*
	 * System non-continuous memory, allocate from system
	 * user   : non-cacheable
	 * kernel : non-cacheable
	 */
	NEXELL_BO_SYSTEM_NONCONTIG,

	/*
	 * System non-continuous memory, allocate from system
	 * user   : cacheable
	 * kernel : cacheable
	 */
	NEXELL_BO_SYSTEM_NONCONTIG_CACHEABLE,

	NEXELL_BO_MAX,
};

/*
 * nexell private ioctl
 */
#define DRM_NX_GEM_CREATE		0x00
/* Reserved 0x03 ~ 0x05 for nx specific gem ioctl */
#define DRM_NX_GEM_GET			0x04
#define DRM_NX_GEM_SYNC			0x05

#define DRM_IOCTL_NX_GEM_CREATE	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_NX_GEM_CREATE, struct nx_gem_create)

#define DRM_IOCTL_NX_GEM_SYNC	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_NX_GEM_SYNC, struct nx_gem_create)

#define DRM_IOCTL_NX_GEM_GET	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_NX_GEM_GET,	struct nx_gem_info)

#endif
