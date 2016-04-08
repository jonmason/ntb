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
#include <uapi/drm/drm_fourcc.h>

#include "nx_drm_drv.h"
#include "nx_drm_crtc.h"
#include "nx_drm_plane.h"
#include "soc/s5pxx18_drm_dp.h"

static const uint32_t support_formats_rgb[] = {
	DRM_FORMAT_RGB565,
	DRM_FORMAT_BGR565,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
};

static const uint32_t support_formats_vid[] = {
	/* 1 buffer */
	DRM_FORMAT_YUYV,
	DRM_FORMAT_YVYU,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_VYUY,
	/* 2 buffer */
	DRM_FORMAT_NV12,
	DRM_FORMAT_NV21,
	DRM_FORMAT_NV16,
	DRM_FORMAT_NV61,
	/* 3 buffer */
	DRM_FORMAT_YUV410,
	DRM_FORMAT_YVU410,
	DRM_FORMAT_YUV411,
	DRM_FORMAT_YVU411,
	DRM_FORMAT_YUV420,
	DRM_FORMAT_YVU420,
	DRM_FORMAT_YUV422,
	DRM_FORMAT_YVU422,
	DRM_FORMAT_YUV444,
	DRM_FORMAT_YVU444,
};

static int nx_drm_update_plane(struct drm_plane *plane, struct drm_crtc *crtc,
			struct drm_framebuffer *fb, int crtc_x,
			int crtc_y, unsigned int crtc_w,
			unsigned int crtc_h, uint32_t src_x,
			uint32_t src_y, uint32_t src_w, uint32_t src_h)
{
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);

	nx_drm_dp_plane_update(plane, fb, crtc_x, crtc_y,
			      crtc_w, crtc_h, src_x >> 16, src_y >> 16,
			      src_w >> 16, src_h >> 16);

	nx_plane->enabled = true;

	return 0;
}

static int nx_drm_plane_disable(struct drm_plane *plane)
{
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);

	if (!nx_plane->enabled)
		return 0;

	nx_drm_dp_plane_disable(plane);
	nx_plane->enabled = false;

	return 0;
}

static void nx_drm_plane_destroy(struct drm_plane *plane)
{
	struct drm_device *drm = plane->dev;
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);

	DRM_DEBUG_KMS("enter\n");

	nx_drm_plane_disable(plane);
	drm_plane_cleanup(plane);
	devm_kfree(drm->dev, nx_plane);
}

static int nx_plane_set_property(struct drm_plane *plane,
			struct drm_property *property, uint64_t val)
{
	return 0;
}

static struct drm_plane_funcs nx_plane_funcs = {
	.update_plane = nx_drm_update_plane,
	.disable_plane = nx_drm_plane_disable,
	.destroy = nx_drm_plane_destroy,
	.set_property = nx_plane_set_property,
};

struct drm_plane *nx_drm_plane_init(struct drm_device *drm,
			struct drm_crtc *crtc,
			unsigned long possible_crtcs,
			enum drm_plane_type drm_type,
			int plane_num)
{
	struct drm_plane *plane;
	struct nx_drm_plane *nx_plane;
	bool video = plane_num == PLANE_VIDEO_NUM ? true : false;
	uint32_t const *formats = video ?
				support_formats_vid : support_formats_rgb;
	int format_count = video ?
				ARRAY_SIZE(support_formats_vid) :
				ARRAY_SIZE(support_formats_rgb);
	int err;

	DRM_DEBUG_KMS("plane.%d\n", plane_num);

	nx_plane =
		devm_kzalloc(drm->dev, sizeof(struct nx_drm_plane), GFP_KERNEL);
	if (IS_ERR(nx_plane))
		return ERR_PTR(-ENOMEM);

	plane = &nx_plane->plane;
	nx_drm_dp_plane_init(drm, crtc, plane, plane_num);

	err = drm_universal_plane_init(drm, plane, possible_crtcs,
			&nx_plane_funcs, formats, format_count, drm_type);
	if (err) {
		DRM_ERROR("fail : initialize plane\n");
		devm_kfree(drm->dev, nx_plane);
		return ERR_PTR(err);
	}

	DRM_DEBUG_KMS("done plane id:%d\n", plane->base.id);

	return (struct drm_plane *)plane;
}

