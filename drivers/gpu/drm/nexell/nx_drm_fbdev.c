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
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_cma_helper.h>

#include <drm/nexell_drm.h>

#include "nx_drm_drv.h"
#include "nx_drm_fb.h"
#include "soc/nx_display.h"

#define MAX_CONNECTOR		4
#define PREFERRED_BPP		32

#define to_nx_fbdev(x)		\
		container_of(x, struct nx_drm_fbdev, fb_helper)

struct nx_drm_fbdev {
	struct drm_fb_helper fb_helper;
	struct drm_gem_cma_object *obj;
};

uint32_t nx_drm_mode_fb_format(uint32_t bpp, uint32_t depth)
{
	uint32_t fmt;

	DRM_DEBUG_KMS("enter (bpp:%d, depth:%d)\n", bpp, depth);

	switch (bpp) {
	case 16:
		fmt = MLC_RGBFMT_R5G6B5;
		break;
	case 24:
		fmt = MLC_RGBFMT_R8G8B8;
		break;
	case 32:
		if (depth == 24)
			fmt = MLC_RGBFMT_X8R8G8B8;
		else
			fmt = MLC_RGBFMT_A8R8G8B8;
		break;
	default:
		DRM_ERROR("bad bpp, assuming x8r8g8b8 pixel format\n");
		fmt = MLC_RGBFMT_X8R8G8B8;
		break;
	}
	return fmt;
}

static int nx_drm_fb_helper_check_var(struct fb_var_screeninfo *var,
				      struct fb_info *info)
{
	struct drm_fb_helper *fb_helper = info->par;
	struct drm_framebuffer *fb = fb_helper->fb;
	int depth;

	DRM_DEBUG_KMS("enter\n");

	if (var->pixclock != 0 || in_dbg_master())
		return -EINVAL;

	/* Need to resize the fb object !!! */
	if (var->bits_per_pixel > fb->bits_per_pixel ||
	    var->xres > fb->width || var->yres > fb->height ||
	    var->xres_virtual > fb->width || var->yres_virtual > fb->height) {

		DRM_ERROR("fb userspace request width/height/bpp is greater\n");
		DRM_ERROR("current fb request %dx%d-%d (%dx%d) > %dx%d-%d\n",
			  var->xres, var->yres, var->bits_per_pixel,
			  var->xres_virtual, var->yres_virtual,
			  fb->width, fb->height, fb->bits_per_pixel);

		return -EINVAL;
	}

	switch (var->bits_per_pixel) {
	case 16:
		depth = (var->green.length == 6) ? 16 : 15;
		break;
	case 32:
		depth = (var->transp.length > 0) ? 32 : 24;
		break;
	default:
		depth = var->bits_per_pixel;
		break;
	}

	switch (depth) {
	case 8:
		var->red.offset = 0;
		var->green.offset = 0;
		var->blue.offset = 0;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->transp.length = 0;
		var->transp.offset = 0;
		break;
	case 15:
		var->red.offset = 10;
		var->green.offset = 5;
		var->blue.offset = 0;
		var->red.length = 5;
		var->green.length = 5;
		var->blue.length = 5;
		var->transp.length = 1;
		var->transp.offset = 15;
		break;
	case 16:
		var->red.offset = 11;
		var->green.offset = 5;
		var->blue.offset = 0;
		var->red.length = 5;
		var->green.length = 6;
		var->blue.length = 5;
		var->transp.length = 0;
		var->transp.offset = 0;
		break;
	case 24:
		var->red.offset = 16;
		var->green.offset = 8;
		var->blue.offset = 0;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->transp.length = 0;
		var->transp.offset = 0;
		break;
	case 32:		/* RGBA8888 */
		var->red.offset = 0;
		var->green.offset = 8;
		var->blue.offset = 16;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->transp.length = 8;
		var->transp.offset = 24;
		break;
	default:
		return -EINVAL;
	}
	DRM_DEBUG_KMS("out.\n");
	return 0;
}

static struct fb_ops nx_fb_ops = {
	.owner = THIS_MODULE,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,

	.fb_check_var = nx_drm_fb_helper_check_var,

	.fb_set_par = drm_fb_helper_set_par,
	.fb_blank = drm_fb_helper_blank,
	.fb_pan_display = drm_fb_helper_pan_display,
	.fb_setcmap = drm_fb_helper_setcmap,
};

static int nx_drm_fbdev_probe(struct drm_fb_helper *helper,
			      struct drm_fb_helper_surface_size *sizes)
{
	struct nx_drm_fbdev *nx_fbdev = to_nx_fbdev(helper);
	struct drm_gem_cma_object *cma_obj;
	struct drm_device *drm = helper->dev;
	struct drm_framebuffer *fb;
	struct fb_info *fbi;
	struct drm_mode_fb_cmd2 mode_cmd = { 0 };
	struct platform_device *pdev = drm->platformdev;
	unsigned long offset;
	unsigned long size;
	int ret;

	DRM_DEBUG_KMS("enter\n");
	DRM_DEBUG_KMS("surface width(%d), height(%d) and bpp(%d)\n",
		      sizes->surface_width, sizes->surface_height,
		      sizes->surface_bpp);

	mode_cmd.width = sizes->surface_width;
	mode_cmd.height = sizes->surface_height;
	mode_cmd.pitches[0] = sizes->surface_width * (sizes->surface_bpp >> 3);
	mode_cmd.pixel_format = drm_mode_legacy_fb_format(sizes->surface_bpp,
							  sizes->surface_depth);

	size = roundup(mode_cmd.pitches[0] * mode_cmd.height, PAGE_SIZE);
	DRM_DEBUG_KMS("size:%lu pitches:%d, height:%d\n",
		      size, mode_cmd.pitches[0], mode_cmd.height);

	/* allocate frame buffer */
	cma_obj = drm_gem_cma_create(drm, size);
	if (IS_ERR(cma_obj))
		return -ENOMEM;

	nx_fbdev->obj = cma_obj;

	fbi = framebuffer_alloc(0, &pdev->dev);
	if (!fbi) {
		DRM_ERROR("failed to allocate framebuffer info.\n");
		ret = -ENOMEM;
		goto err_drm_gem_cma_free_object;
	}

	fb = nx_drm_fb_allocate(drm, &mode_cmd, &cma_obj, 1);
	if (IS_ERR(helper->fb)) {
		DRM_ERROR("failed to allocate DRM framebuffer.\n");
		ret = PTR_ERR(helper->fb);
		goto err_framebuffer_release;
	}

	/*
	 * reset pixel_format
	 */
	fb->pixel_format = nx_drm_mode_fb_format(sizes->surface_bpp,
						 sizes->surface_depth);

	helper->fb = fb;
	helper->fbdev = fbi;

	fbi->par = helper;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	fbi->fbops = &nx_fb_ops;

	ret = fb_alloc_cmap(&fbi->cmap, 256, 0);
	if (ret) {
		DRM_ERROR("failed to allocate cmap.\n");
		goto err_drm_fb_destroy;
	}

	drm_fb_helper_fill_fix(fbi, fb->pitches[0], fb->depth);
	drm_fb_helper_fill_var(fbi, helper, fb->width, fb->height);

	offset = fbi->var.xoffset * (fb->bits_per_pixel >> 3);
	offset += fbi->var.yoffset * fb->pitches[0];

	drm->mode_config.fb_base = (resource_size_t) cma_obj->paddr;
	fbi->screen_base = cma_obj->vaddr + offset;
	fbi->fix.smem_start = (unsigned long)(cma_obj->paddr + offset);
	fbi->screen_size = size;
	fbi->fix.smem_len = size;

	DRM_DEBUG_KMS("v:0x%p, p:0x%lx, offs:0x%lx, size:%lu\n",
		      cma_obj->vaddr, (unsigned long)cma_obj->paddr,
		      offset, size);
	DRM_DEBUG_KMS("exit.\n");

	return 0;

err_drm_fb_destroy:
	drm_framebuffer_unregister_private(fb);
	nx_drm_fb_destroy(fb);
err_framebuffer_release:
	framebuffer_release(fbi);
err_drm_gem_cma_free_object:
	drm_gem_cma_free_object(&cma_obj->base);

	return ret;
}

static void nx_drm_fbdev_destroy(struct drm_device *drm,
				 struct drm_fb_helper *fb_helper)
{
	struct drm_framebuffer *fb;

	/* release drm framebuffer and real buffer */
	if (fb_helper->fb && fb_helper->fb->funcs) {
		fb = fb_helper->fb;
		if (fb && fb->funcs->destroy)
			fb->funcs->destroy(fb);
	}

	/* release linux framebuffer */
	if (fb_helper->fbdev) {
		struct fb_info *info;
		int ret;

		info = fb_helper->fbdev;
		ret = unregister_framebuffer(info);
		if (0 > ret)
			DRM_DEBUG_KMS("failed unregister_framebuffer()\n");

		if (info->cmap.len)
			fb_dealloc_cmap(&info->cmap);

		framebuffer_release(info);
	}

	drm_fb_helper_fini(fb_helper);
}

static struct drm_fb_helper_funcs nx_fb_helper_funcs = {
	.fb_probe = nx_drm_fbdev_probe,
};

void nx_drm_fbdev_fini(struct drm_device *drm)
{
	struct nx_drm_private *private = drm->dev_private;
	struct nx_drm_fbdev *nx_fbdev;

	if (!private || !private->fb_helper)
		return;

	nx_fbdev = to_nx_fbdev(private->fb_helper);

	nx_drm_fbdev_destroy(drm, private->fb_helper);
	kfree(nx_fbdev);
	private->fb_helper = NULL;
}

int nx_drm_fbdev_init(struct drm_device *drm)
{
	struct nx_drm_fbdev *nx_fbdev;
	struct nx_drm_private *private = drm->dev_private;
	struct drm_fb_helper *helper;
	unsigned int num_crtc;
	int ret;

	DRM_DEBUG_KMS("enter crtc num:%d, connector num:%d\n",
		      drm->mode_config.num_crtc,
		      drm->mode_config.num_connector);

	if (!drm->mode_config.num_crtc || !drm->mode_config.num_connector)
		return 0;

	nx_fbdev = kzalloc(sizeof(*nx_fbdev), GFP_KERNEL);
	if (!nx_fbdev) {
		DRM_ERROR("failed to allocate drm fbdev.\n");
		return -ENOMEM;
	}

	private->fb_helper = helper = &nx_fbdev->fb_helper;
	drm_fb_helper_prepare(drm, helper, &nx_fb_helper_funcs);

	num_crtc = drm->mode_config.num_crtc;
	ret = drm_fb_helper_init(drm, helper, num_crtc, MAX_CONNECTOR);
	if (0 > ret) {
		DRM_ERROR("failed to initialize drm fb helper.\n");
		goto err_drm_fb_dev_free;
	}

	ret = drm_fb_helper_single_add_all_connectors(helper);
	if (0 > ret) {
		DRM_ERROR("failed to register drm_fb_helper_connector.\n");
		goto err_drm_fb_helper_fini;

	}
	DRM_DEBUG_KMS("fb helper's connector:%d\n", helper->connector_count);

	/* disable all the possible outputs/crtcs before entering KMS mode */
	drm_helper_disable_unused_functions(drm);

	/* call fb_probe */
	ret = drm_fb_helper_initial_config(helper, PREFERRED_BPP);
	if (0 > ret) {
		DRM_ERROR("failed to set up hw configuration.\n");
		goto err_drm_fb_helper_fini;
	}

	DRM_DEBUG_KMS("exit.\n");
	return 0;

err_drm_fb_helper_fini:
	drm_fb_helper_fini(helper);

err_drm_fb_dev_free:
	kfree(nx_fbdev);
	private->fb_helper = NULL;

	return ret;
}
