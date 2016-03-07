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
struct nx_drm_gem_create {
	uint64_t size;
	unsigned int flags;
	unsigned int handle;
	struct drm_gem_cma_object *obj;
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
struct nx_drm_gem_info {
	unsigned int handle;
	unsigned int flags;
	uint64_t size;
};

/**
 * A structure for user connection request of virtual display.
 *
 * @connection: indicate whether doing connetion or not by user.
 * @extensions: if this value is 1 then the vidi driver would need additional
 *	128bytes edid data.
 * @edid: the edid data pointer from user side.
 */
struct nx_drm_vidi_connection {
	unsigned int connection;
	unsigned int extensions;
	uint64_t edid;
};

/* memory type definitions. */
enum e_nx_drm_gem_mem_type {
	/* Physically Continuous memory and used as default. */
	NX_BO_CONTIG	= 0 << 0,
	/* Physically Non-Continuous memory. */
	NX_BO_NONCONTIG	= 1 << 0,
	/* non-cachable mapping and used as default. */
	NX_BO_NONCACHABLE	= 0 << 1,
	/* cachable mapping. */
	NX_BO_CACHABLE	= 1 << 1,
	/* write-combine mapping. */
	NX_BO_WC		= 1 << 2,
	NX_BO_MASK		= NX_BO_NONCONTIG | NX_BO_CACHABLE |
					NX_BO_WC
};

enum nx_drm_ops_id {
	NX_DRM_OPS_SRC,
	NX_DRM_OPS_DST,
	NX_DRM_OPS_MAX,
};

struct nx_drm_sz {
	__u32	hsize;
	__u32	vsize;
};

struct nx_drm_pos {
	__u32	x;
	__u32	y;
	__u32	w;
	__u32	h;
};

enum nx_drm_flip {
	NX_DRM_FLIP_NONE = (0 << 0),
	NX_DRM_FLIP_VERTICAL = (1 << 0),
	NX_DRM_FLIP_HORIZONTAL = (1 << 1),
	NX_DRM_FLIP_BOTH = NX_DRM_FLIP_VERTICAL |
			NX_DRM_FLIP_HORIZONTAL,
};

enum nx_drm_degree {
	NX_DRM_DEGREE_0,
	NX_DRM_DEGREE_90,
	NX_DRM_DEGREE_180,
	NX_DRM_DEGREE_270,
};

enum nx_drm_planer {
	NX_DRM_PLANAR_Y,
	NX_DRM_PLANAR_CB,
	NX_DRM_PLANAR_CR,
	NX_DRM_PLANAR_MAX,
};

#define DRM_NX_GEM_CREATE		0x00
/* Reserved 0x03 ~ 0x05 for nx specific gem ioctl */
#define DRM_NX_GEM_GET			0x04
#define DRM_NX_VIDI_CONNECTION	0x07

#define DRM_IOCTL_NX_GEM_CREATE		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_NX_GEM_CREATE, struct nx_drm_gem_create)

#define DRM_IOCTL_NX_GEM_GET	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_NX_GEM_GET,	struct nx_drm_gem_info)

#define DRM_IOCTL_NX_VIDI_CONNECTION	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_NX_VIDI_CONNECTION, struct nx_drm_vidi_connection)

#endif
