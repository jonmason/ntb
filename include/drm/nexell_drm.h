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
#ifndef _NX_DRM_H_
#define _NX_DRM_H_

/**
 * User-desired buffer creation information structure.
 *
 * @size: user-desired memory allocation size.
 *	- this size value would be page-aligned internally.
 * @flags: user request for setting memory type or cache attributes.
 * @handle: returned a handle to created gem object.
 *	- this handle will be set by gem module of kernel side.
 */
struct drm_nx_gem_create {
	uint64_t size;
	unsigned int flags;
	unsigned int handle;
};

/**
 * A structure for getting buffer offset.
 *
 * @handle: a pointer to gem object created.
 * @pad: just padding to be 64-bit aligned.
 * @offset: relatived offset value of the memory region allocated.
 *	- this value should be set by user.
 */
struct drm_nx_gem_map_off {
	unsigned int handle;
	unsigned int pad;
	uint64_t offset;
};

/**
 * A structure for mapping buffer.
 *
 * @handle: a handle to gem object created.
 * @size: memory size to be mapped.
 * @mapped: having user virtual address mmaped.
 *	- this variable would be filled by gem module
 *	of kernel side with user virtual address which is allocated
 *	by do_mmap().
 */
struct drm_nx_gem_mmap {
	unsigned int handle;
	unsigned int size;
	uint64_t mapped;
};

/**
 * A structure for user connection request of virtual display.
 *
 * @connection: indicate whether doing connetion or not by user.
 * @extensions: if this value is 1 then the vidi driver would need additional
 *	128bytes edid data.
 * @edid: the edid data pointer from user side.
 */
struct drm_nx_vidi_connection {
	unsigned int connection;
	unsigned int extensions;
	uint64_t edid;
};

struct drm_nx_lcd_connection {
	unsigned int connection;
	unsigned int extensions;
	uint64_t edid;
};

struct drm_nx_plane_set_zpos {
	__u32 plane_id;
	__s32 zpos;
};

#define DRM_NX_GEM_CREATE		0x00
#define DRM_NX_GEM_MAP_OFFSET	0x01
#define DRM_NX_GEM_MMAP		0x02
#define DRM_NX_PLANE_SET_ZPOS	0x06
#define DRM_NX_VIDI_CONNECTION	0x07

#define DRM_IOCTL_NX_GEM_CREATE		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_NX_GEM_CREATE, struct drm_nx_gem_create)

#define DRM_IOCTL_NX_GEM_MAP_OFFSET	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_NX_GEM_MAP_OFFSET, struct drm_nx_gem_map_off)

#define DRM_IOCTL_NX_GEM_MMAP	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_NX_GEM_MMAP, struct drm_nx_gem_mmap)

#define DRM_IOCTL_NX_PLANE_SET_ZPOS	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_NX_PLANE_SET_ZPOS, struct drm_nx_plane_set_zpos)

#define DRM_IOCTL_NX_VIDI_CONNECTION	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_NX_VIDI_CONNECTION, struct drm_nx_vidi_connection)

#ifdef __KERNEL__

/**
 * A structure for lcd panel information.
 *
 * @timing: default video mode for initializing
 * @width_mm: physical size of lcd width.
 * @height_mm: physical size of lcd height.
 */
struct nx_drm_panel_info {
	struct fb_videomode timing;
	u32 width_mm;
	u32 height_mm;
};

/**
 * Platform Specific Structure for DRM.
 *
 * @panel: default panel info for initializing
 * @default_win: default window layer number to be used for UI.
 * @bpp: default bit per pixel.
 */
struct nx_drm_fimd_pdata {
	struct nx_drm_panel_info panel;
	u32				vidcon0;
	u32				vidcon1;
	unsigned int			default_win;
	unsigned int			bpp;
};

/**
 * Platform Specific Structure for DRM based HDMI.
 *
 * @hdmi_dev: device point to specific hdmi driver.
 * @mixer_dev: device point to specific mixer driver.
 *
 * this structure is used for common hdmi driver and each device object
 * would be used to access specific device driver(hdmi or mixer driver)
 */
struct nx_drm_common_hdmi_pd {
	struct device *hdmi_dev;
	struct device *mixer_dev;
};

/**
 * Platform Specific Structure for DRM based HDMI core.
 *
 * @timing: default video mode for initializing
 * @default_win: default window layer number to be used for UI.
 * @bpp: default bit per pixel.
 * @is_v13: set if hdmi version 13 is.
 */
struct nx_drm_hdmi_pdata {
	struct fb_videomode		timing;
	unsigned int			default_win;
	unsigned int			bpp;
	unsigned int			is_v13:1;
};

#endif
#endif
