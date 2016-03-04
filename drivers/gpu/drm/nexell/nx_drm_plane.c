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
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <uapi/drm/drm_fourcc.h>

#include "nx_drm_drv.h"
#include "nx_drm_crtc.h"
#include "nx_drm_plane.h"
#include "nx_drm_fb.h"
#include "nx_drm_encoder.h"

static const uint32_t formats[] = {
	DRM_FORMAT_RGB565,
	DRM_FORMAT_BGR565,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_YUYV,
#if 0
	DRM_FORMAT_NV12,
	DRM_FORMAT_NV12MT,
#endif
};

int nx_drm_plane_update(struct nx_drm_plane *nx_plane,
				struct drm_framebuffer *fb,
				struct drm_display_mode *mode,
				struct nx_drm_crtc_pos *pos)
{
#if 1
	DRM_DEBUG_KMS("TODO\n");
	return 0;
#else
	struct nx_gem_buffer *buffer;
	unsigned int actual_w;
	unsigned int actual_h;
	int nr = nx_drm_format_num_buffers(fb->pixel_format);
	int i;

	DRM_DEBUG_KMS("enter\n");
	for (i = 0; i < nr; i++) {
		buffer = nx_drm_fb_buffer(fb, i);
		if (!buffer) {
			DRM_DEBUG_KMS("buffer is null\n");
			return -EFAULT;
		}
		nx_plane->dma_addr[i] = buffer->dma_addr;
		nx_plane->vaddr[i] = buffer->kvaddr;

		DRM_DEBUG_KMS("buffer: %d, vaddr = 0x%lx, dma_addr = 0x%lx\n",
			      i, (unsigned long)nx_plane->vaddr[i],
			      (unsigned long)nx_plane->dma_addr[i]);
	}

	actual_w = min((mode->hdisplay - pos->crtc_x), pos->crtc_w);
	actual_h = min((mode->vdisplay - pos->crtc_y), pos->crtc_h);

	/* set drm framebuffer data. */
	nx_plane->fb_x = pos->fb_x;
	nx_plane->fb_y = pos->fb_y;
	nx_plane->fb_width = fb->width;
	nx_plane->fb_height = fb->height;
	nx_plane->bpp = fb->bits_per_pixel;
	nx_plane->pitch = fb->pitches[0];
	nx_plane->pixel_format = fb->pixel_format;

	/* set nx_plane range to be displayed. */
	nx_plane->crtc_x = pos->crtc_x;
	nx_plane->crtc_y = pos->crtc_y;
	nx_plane->crtc_width = actual_w;
	nx_plane->crtc_height = actual_h;

	/* set drm mode data. */
	nx_plane->mode_width = mode->hdisplay;
	nx_plane->mode_height = mode->vdisplay;
	nx_plane->refresh = mode->vrefresh;
	nx_plane->scan_flag = mode->flags;

	DRM_DEBUG_KMS("plane : offset_x/y(%d,%d), width/height(%d,%d)",
		      nx_plane->crtc_x, nx_plane->crtc_y,
		      nx_plane->crtc_width, nx_plane->crtc_height);
#endif
	return 0;
}

static int nx_drm_update_plane(struct drm_plane *plane, struct drm_crtc *crtc,
				struct drm_framebuffer *fb, int crtc_x,
			    int crtc_y, unsigned int crtc_w,
			    unsigned int crtc_h, uint32_t src_x,
			    uint32_t src_y, uint32_t src_w, uint32_t src_h)
{
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);
	struct nx_drm_crtc_pos pos = { 0, };
	unsigned int x = src_x >> 16;
	unsigned int y = src_y >> 16;
	int ret;

	DRM_DEBUG_KMS(
	    "+[crtc X:%d, Y:%d, W:%d, H:%d] [src X:%d, Y:%d, W:%d, H:%d]\n",
		crtc_x, crtc_y, crtc_w, crtc_h, src_x, src_y, src_w, src_h);

	pos.crtc_x = crtc_x;
	pos.crtc_y = crtc_y;
	pos.crtc_w = crtc_w;
	pos.crtc_h = crtc_h;

	pos.fb_x = x;
	pos.fb_y = y;

	/* TODO: scale feature */
	ret = nx_drm_plane_update(nx_plane, fb, &crtc->mode, &pos);
	if (ret < 0)
		return ret;

#if 0
	nx_drm_fn_encoder(crtc, nx_plane, nx_drm_encoder_crtc_mode_set);
	nx_drm_fn_encoder(crtc, &nx_plane->zpos,
					nx_drm_encoder_crtc_plane_commit);
#endif

	nx_plane->enabled = true;

	return 0;
}

static int nx_drm_disable_plane(struct drm_plane *plane)
{
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);

	DRM_DEBUG_KMS("enter\n");

	if (!nx_plane->enabled)
		return 0;

#if 0
	nx_drm_fn_encoder(plane->crtc,
				&nx_plane->zpos, nx_drm_encoder_crtc_disable);
#endif

	nx_plane->enabled = false;
	nx_plane->zpos = DEFAULT_ZPOS;

	return 0;
}

static void nx_drm_plane_destroy(struct drm_plane *plane)
{
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);

	DRM_DEBUG_KMS("enter\n");

	nx_drm_disable_plane(plane);
	drm_plane_cleanup(plane);
	kfree(nx_plane);
}

static int nx_plane_set_property(struct drm_plane *plane,
				struct drm_property *property, uint64_t val)
{
	DRM_DEBUG_KMS("TO DO\n");
	return 0;
}

static struct drm_plane_funcs nx_plane_funcs = {
	.update_plane = nx_drm_update_plane,
	.disable_plane = nx_drm_disable_plane,
	.destroy = nx_drm_plane_destroy,
	.set_property = nx_plane_set_property,
};

int nx_drm_plane_set_zpos_ioctl(struct drm_device *drm, void *data,
				struct drm_file *file_priv)
{
	struct nx_drm_plane_set_zpos *zpos_req = data;
	struct drm_mode_object *obj;
	struct drm_plane *plane;
	struct nx_drm_plane *nx_plane;
	int ret = 0;

	DRM_DEBUG_KMS("enter\n");

	if (!drm_core_check_feature(drm, DRIVER_MODESET))
		return -EINVAL;

	if (zpos_req->zpos < 0 || zpos_req->zpos >= MAX_PLANE) {
		if (zpos_req->zpos != DEFAULT_ZPOS) {
			DRM_ERROR("zpos not within limits\n");
			return -EINVAL;
		}
	}

	mutex_lock(&drm->mode_config.mutex);

	obj = drm_mode_object_find(drm, zpos_req->plane_id,
				   DRM_MODE_OBJECT_PLANE);
	if (!obj) {
		DRM_DEBUG_KMS("unknown plane ID %d\n", zpos_req->plane_id);
		ret = -EINVAL;
		goto out;
	}

	plane = obj_to_plane(obj);
	nx_plane = to_nx_plane(plane);

	nx_plane->zpos = zpos_req->zpos;

out:
	mutex_unlock(&drm->mode_config.mutex);
	return ret;
}

struct drm_plane *nx_drm_plane_init(struct drm_device *drm,
				unsigned long possible_crtcs,
				enum drm_plane_type type, int layer)
{
	struct nx_drm_plane *nx_plane;
	struct drm_plane *plane;
	const char * const plane_type[] = {
		[DRM_PLANE_TYPE_OVERLAY] = "Overlay",
		[DRM_PLANE_TYPE_PRIMARY] = "Primary",
		[DRM_PLANE_TYPE_CURSOR] = "Cursor",
	};
	int err;

	DRM_DEBUG_KMS("enter Type:%s\n",
		      ARRAY_SIZE(plane_type) >
		      type ? plane_type[type] : "Unknown");

	nx_plane = kzalloc(sizeof(struct nx_drm_plane), GFP_KERNEL);
	if (!nx_plane)
		return ERR_PTR(-ENOMEM);

	nx_plane->layer = layer;
	plane = &nx_plane->plane;

	err = drm_universal_plane_init(drm, plane, possible_crtcs,
				&nx_plane_funcs, formats,
				ARRAY_SIZE(formats), type);
	if (err) {
		DRM_ERROR("failed to initialize plane\n");
		kfree(nx_plane);
		return ERR_PTR(err);
	}

	DRM_DEBUG_KMS("exit, plane ID:%d\n", plane->base.id);
	return (struct drm_plane *)plane;
}
