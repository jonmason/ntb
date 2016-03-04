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
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fb_cma_helper.h>

#include "nx_drm_drv.h"
#include "nx_drm_fb.h"
#include "soc/nx_display.h"

struct drm_gem_cma_object *nx_drm_fb_get_gem_obj(struct drm_framebuffer *fb,
				unsigned int plane)
{
	struct nx_drm_fb *nx_fb = to_nx_drm_fb(fb);

	if (plane >= MAX_FB_PLANE)
		return NULL;

	return nx_fb->obj[plane];
}

static int nx_drm_fb_create_handle(struct drm_framebuffer *fb,
				struct drm_file *file_priv,
				unsigned int *handle)
{
	struct nx_drm_fb *nx_fb = to_nx_drm_fb(fb);

	DRM_DEBUG_KMS("enter\n");

	return drm_gem_handle_create(file_priv, &nx_fb->obj[0]->base, handle);
}

static int nx_drm_fb_dirty(struct drm_framebuffer *fb,
				struct drm_file *file_priv, unsigned flags,
				unsigned color, struct drm_clip_rect *clips,
				unsigned num_clips)
{
	DRM_DEBUG_KMS("TODO\n");
	return 0;
}

static struct drm_framebuffer_funcs nx_fb_funcs = {
	.destroy = nx_drm_fb_destroy,
	.create_handle = nx_drm_fb_create_handle,
	.dirty = nx_drm_fb_dirty,
};

void nx_drm_fb_destroy(struct drm_framebuffer *fb)
{
	struct nx_drm_fb *nx_fb = to_nx_drm_fb(fb);
	int i;

	DRM_DEBUG_KMS("enter\n");

	for (i = 0; MAX_FB_PLANE > i; i++) {
		if (nx_fb->obj[i])
			drm_gem_object_unreference_unlocked(&nx_fb->obj[i]->
							    base);
	}

	drm_framebuffer_cleanup(fb);
	kfree(nx_fb);
}

struct drm_framebuffer *nx_drm_fb_allocate(struct drm_device *drm,
				struct drm_mode_fb_cmd2 *mode_cmd,
				struct drm_gem_cma_object **obj,
				int num_planes)
{
	struct nx_drm_fb *nx_fb;
	struct drm_framebuffer *fb;
	int i, ret;

	DRM_DEBUG_KMS("enter\n");

	nx_fb = kzalloc(sizeof(*nx_fb), GFP_KERNEL);
	if (!nx_fb) {
		DRM_ERROR("failed to allocate drm framebuffer\n");
		return ERR_PTR(-ENOMEM);
	}
	fb = &nx_fb->fb;

	for (i = 0; i < num_planes; i++)
		nx_fb->obj[i] = obj[i];

	ret = drm_framebuffer_init(drm, fb, &nx_fb_funcs);
	if (ret) {
		DRM_ERROR("failed to initialize framebuffer\n");
		return ERR_PTR(ret);
	}

	drm_helper_mode_fill_fb_struct(fb, mode_cmd);

	DRM_DEBUG_KMS("exit, fb ID:%d\n", fb->base.id);

	return fb;
}

static struct drm_framebuffer *nx_drm_fb_create(struct drm_device *drm,
				struct drm_file *file_priv,
				struct drm_mode_fb_cmd2
				*mode_cmd)
{
	struct drm_framebuffer *fb;
	struct drm_gem_cma_object *objs[4];
	struct drm_gem_object *obj;
	unsigned int hsub;
	unsigned int vsub;
	int ret;
	int i;

	DRM_DEBUG_KMS("enter\n");

	/* TODO: Need to use ion heaps to create frame buffer?? */
	hsub = drm_format_horz_chroma_subsampling(mode_cmd->pixel_format);
	vsub = drm_format_vert_chroma_subsampling(mode_cmd->pixel_format);

	for (i = 0; i < drm_format_num_planes(mode_cmd->pixel_format); i++) {
		unsigned int width = mode_cmd->width / (i ? hsub : 1);
		unsigned int height = mode_cmd->height / (i ? vsub : 1);
		unsigned int min_size;

		obj = drm_gem_object_lookup(drm, file_priv,
					    mode_cmd->handles[i]);
		if (!obj) {
			DRM_ERROR("failed to lookup GEM object\n");
			ret = -ENXIO;
			goto err_gem_object_unreference;
		}

		min_size = (height - 1) * mode_cmd->pitches[i]
		    + width * drm_format_plane_cpp(mode_cmd->pixel_format, i)
		    + mode_cmd->offsets[i];

		if (obj->size < min_size) {
			drm_gem_object_unreference_unlocked(obj);
			ret = -EINVAL;
			goto err_gem_object_unreference;
		}
		objs[i] = to_drm_gem_cma_obj(obj);
	}

	fb = nx_drm_fb_allocate(drm, mode_cmd, objs, 1);
	if (IS_ERR(fb)) {
		ret = PTR_ERR(fb);
		goto err_gem_object_unreference;
	}

	DRM_DEBUG_KMS("exit.\n");
	return fb;

err_gem_object_unreference:
	for (i--; i >= 0; i--)
		drm_gem_object_unreference_unlocked(&objs[i]->base);

	return ERR_PTR(ret);
}

#if 0
static void nx_drm_output_poll_changed(struct drm_device *drm)
{
	struct nx_drm_private *private = drm->dev_private;
	struct drm_fb_helper *fb_helper = private->fb_helper;

	DRM_DEBUG_KMS("enter\n");

	if (fb_helper)
		drm_fb_helper_hotplug_event(fb_helper);

	DRM_DEBUG_DRIVER("exit.\n");
}
#endif

static struct drm_mode_config_funcs nx_mode_config_funcs = {
	.fb_create = nx_drm_fb_create,
	/*
	.output_poll_changed = nx_drm_output_poll_changed,
	*/
};

void nx_drm_mode_config_init(struct drm_device *drm)
{
	drm->mode_config.min_width = 0;
	drm->mode_config.min_height = 0;

	/*
	 * set max width and height as default value
	 * this value would be used to check framebuffer size limitation
	 * at drm_mode_addfb().
	 */
	drm->mode_config.max_width = MAX_FB_MODE_WIDTH;
	drm->mode_config.max_height = MAX_FB_MODE_HEIGHT;
	drm->mode_config.funcs = &nx_mode_config_funcs;

	DRM_DEBUG_KMS("min %d*%d, max %d*%d\n",
		 drm->mode_config.min_width, drm->mode_config.min_height,
		 drm->mode_config.max_width, drm->mode_config.max_height);
}
