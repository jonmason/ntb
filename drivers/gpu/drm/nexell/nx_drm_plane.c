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
	int ret;

	ret = nx_drm_dp_plane_update(plane, fb, crtc_x, crtc_y,
			      crtc_w, crtc_h, src_x >> 16, src_y >> 16,
			      src_w >> 16, src_h >> 16, 0);

	if (!ret)
		nx_plane->enabled = true;

	/* link to plane's crtc */
	plane->crtc = crtc;

	return ret;
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

static int nx_drm_plane_set_property(struct drm_plane *plane,
			struct drm_property *property, uint64_t val)
{
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);
	union color_property *color = &nx_plane->color;

	DRM_DEBUG_KMS("enter %s:0x%llx\n", property->name, val);

	if (nx_plane->is_yuv_plane) {
		if (property == color->yuv.colorkey) {
			color->colorkey = val;
			nx_drm_dp_plane_set_color(
				plane, dp_color_colorkey, val);
		}
	} else {
		if (property == color->rgb.transcolor) {
			color->transcolor = val;
			nx_drm_dp_plane_set_color(plane, dp_color_transp, val);
		}

		if (property == color->rgb.alphablend) {
			color->alphablend = val;
			nx_drm_dp_plane_set_color(plane, dp_color_alpha, val);
		}
	}

	return 0;
}

static struct drm_plane_funcs nx_plane_funcs = {
	.update_plane = nx_drm_update_plane,
	.disable_plane = nx_drm_plane_disable,
	.destroy = nx_drm_plane_destroy,
	.set_property = nx_drm_plane_set_property,
};

static void	nx_drm_plane_create_proeprties(struct drm_device *drm,
			struct drm_plane *plane)
{
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);
	union color_property *color = &nx_plane->color;

	if (nx_plane->is_yuv_plane) {
		/* YUV color for video plane */
		color->yuv.colorkey = drm_property_create_range(
					drm, 0, "colorkey", 0, 0xffffffff);
		color->colorkey = 0x0;
		drm_object_attach_property(&plane->base,
				   color->yuv.colorkey, color->colorkey);
	} else {
		/* RGB color for RGB plane */
		color->rgb.transcolor = drm_property_create_range(
					drm, 0, "transcolor", 0, 0xffffffff);
		color->transcolor = 0;
		drm_object_attach_property(&plane->base,
				   color->rgb.transcolor, color->transcolor);

		color->rgb.alphablend = drm_property_create_range(
					drm, 0, "alphablend", 0, 0xffffffff);
		color->alphablend = 0;
		drm_object_attach_property(&plane->base,
				   color->rgb.alphablend, color->alphablend);
	}
}

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

	nx_plane->is_yuv_plane = video;
	plane = &nx_plane->plane;

	nx_drm_dp_plane_init(drm, crtc, plane, plane_num);

	err = drm_universal_plane_init(drm, plane, possible_crtcs,
			&nx_plane_funcs, formats, format_count, drm_type);
	if (err) {
		DRM_ERROR("fail : initialize plane\n");
		devm_kfree(drm->dev, nx_plane);
		return ERR_PTR(err);
	}

	nx_drm_plane_create_proeprties(drm, plane);

	DRM_DEBUG_KMS("done plane id:%d\n", plane->base.id);

	return (struct drm_plane *)plane;
}

