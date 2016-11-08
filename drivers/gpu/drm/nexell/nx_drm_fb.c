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
#include <linux/module.h>

#include "nx_drm_drv.h"
#include "nx_drm_fb.h"
#include "nx_drm_gem.h"

#define PREFERRED_BPP		32

static int fb_buffer_count = 1;
static bool fb_format_bgr;

MODULE_PARM_DESC(fb_buffers, "frame buffer count");
module_param_named(fb_buffers, fb_buffer_count, int, 0600);

MODULE_PARM_DESC(fb_bgr, "frame buffer BGR pixel format");
module_param_named(fb_bgr, fb_format_bgr, bool, 0600);

static void nx_drm_fb_destroy(struct drm_framebuffer *fb)
{
	struct nx_drm_fb *nx_fb = to_nx_drm_fb(fb);
	int i;

	for (i = 0; i < 4; i++) {
		if (nx_fb->obj[i])
			drm_gem_object_unreference_unlocked(
					&nx_fb->obj[i]->base);
	}

	drm_framebuffer_cleanup(fb);
	kfree(nx_fb);
}

static int nx_drm_fb_create_handle(struct drm_framebuffer *fb,
			struct drm_file *file_priv, unsigned int *handle)
{
	struct nx_drm_fb *nx_fb = to_nx_drm_fb(fb);

	return drm_gem_handle_create(file_priv, &nx_fb->obj[0]->base, handle);
}

static int nx_drm_fb_dirty(struct drm_framebuffer *fb,
			struct drm_file *file_priv, unsigned flags,
			unsigned color, struct drm_clip_rect *clips,
			unsigned num_clips)
{
	/* TODO */
	return 0;
}

static struct drm_framebuffer_funcs nx_drm_framebuffer_funcs = {
	.destroy = nx_drm_fb_destroy,
	.create_handle = nx_drm_fb_create_handle,
	.dirty = nx_drm_fb_dirty,
};

static struct fb_ops nx_fb_ops = {
	.owner		= THIS_MODULE,
	.fb_fillrect	= sys_fillrect,
	.fb_copyarea	= sys_copyarea,
	.fb_imageblit	= sys_imageblit,
	.fb_check_var	= drm_fb_helper_check_var,
	.fb_set_par	= drm_fb_helper_set_par,
	.fb_blank	= drm_fb_helper_blank,
	.fb_pan_display	= drm_fb_helper_pan_display,
	.fb_setcmap	= drm_fb_helper_setcmap,
};

static struct nx_drm_fb *nx_drm_fb_alloc(struct drm_device *drm,
			struct drm_mode_fb_cmd2 *mode_cmd,
			struct nx_gem_object **nx_obj,
			unsigned int num_planes)
{
	struct nx_drm_fb *nx_fb;
	int ret;
	int i;

	nx_fb = kzalloc(sizeof(*nx_fb), GFP_KERNEL);
	if (!nx_fb)
		return ERR_PTR(-ENOMEM);

	drm_helper_mode_fill_fb_struct(&nx_fb->fb, mode_cmd);

	for (i = 0; i < num_planes; i++)
		nx_fb->obj[i] = nx_obj[i];

	ret = drm_framebuffer_init(drm, &nx_fb->fb, &nx_drm_framebuffer_funcs);
	if (ret) {
		dev_err(drm->dev, "failed to initialize framebuffer:%d\n", ret);
		kfree(nx_fb);
		return ERR_PTR(ret);
	}

	return nx_fb;
}

static struct drm_framebuffer *nx_drm_fb_create(struct drm_device *drm,
			struct drm_file *file_priv,
			struct drm_mode_fb_cmd2 *mode_cmd)
{
	struct nx_drm_fb *nx_fb;
	struct nx_gem_object *nx_objs[4];
	struct drm_gem_object *obj;
	unsigned int hsub;
	unsigned int vsub;
	int ret;
	int i;

	hsub = drm_format_horz_chroma_subsampling(mode_cmd->pixel_format);
	vsub = drm_format_vert_chroma_subsampling(mode_cmd->pixel_format);

	for (i = 0; i < drm_format_num_planes(mode_cmd->pixel_format); i++) {
		unsigned int width = mode_cmd->width / (i ? hsub : 1);
		unsigned int height = mode_cmd->height / (i ? vsub : 1);
		unsigned int min_size;

		obj = drm_gem_object_lookup(drm, file_priv,
				mode_cmd->handles[i]);
		if (!obj) {
			dev_err(drm->dev, "Failed to lookup GEM object\n");
			ret = -ENXIO;
			goto err_gem_object_unreference;
		}

		min_size = (height - 1) * mode_cmd->pitches[i]
			+ width
			* drm_format_plane_cpp(mode_cmd->pixel_format, i)
			+ mode_cmd->offsets[i];

		if (obj->size < min_size) {
			drm_gem_object_unreference_unlocked(obj);
			ret = -EINVAL;
			goto err_gem_object_unreference;
		}
		nx_objs[i] = to_nx_gem_obj(obj);
	}

	nx_fb = nx_drm_fb_alloc(drm, mode_cmd, nx_objs, i);
	if (IS_ERR(nx_fb)) {
		ret = PTR_ERR(nx_fb);
		goto err_gem_object_unreference;
	}

	return &nx_fb->fb;

err_gem_object_unreference:
	for (i--; i >= 0; i--)
		drm_gem_object_unreference_unlocked(&nx_objs[i]->base);

	return ERR_PTR(ret);
}

struct drm_framebuffer *nx_drm_fb_mode_create(struct drm_device *drm,
			struct drm_file *file_priv,
			struct drm_mode_fb_cmd2 *mode_cmd)
{
	DRM_DEBUG_KMS("enter\n");

	return nx_drm_fb_create(drm, file_priv, mode_cmd);
}

static uint32_t nx_drm_mode_fb_format(uint32_t bpp, uint32_t depth, bool bgr)
{
	uint32_t fmt;

	switch (bpp) {
	case 8:
		fmt = DRM_FORMAT_C8;
		break;
	case 16:
		if (depth == 15)
			fmt = bgr ? DRM_FORMAT_XBGR1555 : DRM_FORMAT_XRGB1555;
		else
			fmt = bgr ? DRM_FORMAT_BGR565 : DRM_FORMAT_RGB565;
		break;
	case 24:
		fmt = bgr ? DRM_FORMAT_BGR888 : DRM_FORMAT_RGB888;
		break;
	case 32:
		if (depth == 24)
			fmt = bgr ? DRM_FORMAT_XBGR8888 : DRM_FORMAT_XRGB8888;
		else
			fmt = bgr ? DRM_FORMAT_ABGR8888 : DRM_FORMAT_ARGB8888;
		break;
	default:
		DRM_ERROR("bad bpp, assuming x8r8g8b8 pixel format\n");
		fmt = DRM_FORMAT_XRGB8888;
		break;
	}

	return fmt;
}

static int nx_drm_fb_helper_probe(struct drm_fb_helper *fb_helper,
			struct drm_fb_helper_surface_size *sizes)
{
	struct nx_drm_fbdev *fbdev = to_nx_drm_fbdev(fb_helper);
	struct drm_mode_fb_cmd2 mode_cmd = { 0 };
	struct drm_device *drm = fb_helper->dev;
	struct nx_gem_object *nx_obj;
	struct drm_framebuffer *fb;
	unsigned int bytes_per_pixel;
	unsigned long offset;
	struct fb_info *info;
	size_t size;
	unsigned int flags = 0;
	int buffers = fbdev->fb_buffers;
	int ret;

	DRM_DEBUG_KMS("surface width(%d), height(%d) and bpp(%d) buffers(%d)\n",
			sizes->surface_width, sizes->surface_height,
			sizes->surface_bpp, buffers);

	bytes_per_pixel = DIV_ROUND_UP(sizes->surface_bpp, 8);

	mode_cmd.width = sizes->surface_width;
	mode_cmd.height = sizes->surface_height;
	mode_cmd.pitches[0] = sizes->surface_width * bytes_per_pixel;
	mode_cmd.pixel_format = nx_drm_mode_fb_format(sizes->surface_bpp,
		sizes->surface_depth, fb_format_bgr);

	/* for double buffer */
	size = mode_cmd.pitches[0] * (mode_cmd.height * buffers);
	nx_obj = nx_drm_gem_create(drm, size, flags);
	if (IS_ERR(nx_obj))
		return -ENOMEM;

	info = framebuffer_alloc(0, drm->dev);
	if (!info) {
		dev_err(drm->dev, "Failed to allocate framebuffer info.\n");
		ret = -ENOMEM;
		goto err_drm_gem_free_object;
	}

	fbdev->fb = nx_drm_fb_alloc(drm, &mode_cmd, &nx_obj, 1);
	if (IS_ERR(fbdev->fb)) {
		dev_err(drm->dev, "Failed to allocate DRM framebuffer.\n");
		ret = PTR_ERR(fbdev->fb);
		goto err_framebuffer_release;
	}

	fb = &fbdev->fb->fb;
	fb_helper->fb = fb;
	fb_helper->fbdev = info;

	info->par = fb_helper;
	info->flags = FBINFO_FLAG_DEFAULT;
	info->fbops = &nx_fb_ops;

	ret = fb_alloc_cmap(&info->cmap, 256, 0);
	if (ret) {
		dev_err(drm->dev, "Failed to allocate color map.\n");
		goto err_drm_fb_destroy;
	}

	drm_fb_helper_fill_fix(info, fb->pitches[0], fb->depth);
	drm_fb_helper_fill_var(info, fb_helper,
			sizes->fb_width, sizes->fb_height);

	/* for double buffer */
	info->var.yres_virtual = fb->height * buffers;

	offset = info->var.xoffset * bytes_per_pixel;
	offset += info->var.yoffset * fb->pitches[0];

	drm->mode_config.fb_base = (resource_size_t)nx_obj->dma_addr;
	info->screen_base = nx_obj->cpu_addr + offset;
	info->fix.smem_start = (unsigned long)(nx_obj->dma_addr + offset);
	info->screen_size = size;
	info->fix.smem_len = size;

	if (fb_helper->crtc_info &&
		fb_helper->crtc_info->desired_mode) {
		struct videomode vm;
		struct drm_display_mode *mode =
				fb_helper->crtc_info->desired_mode;

		drm_display_mode_to_videomode(mode, &vm);
		info->var.left_margin = vm.hsync_len + vm.hback_porch;
		info->var.right_margin = vm.hfront_porch;
		info->var.upper_margin = vm.vsync_len + vm.vback_porch;
		info->var.lower_margin = vm.vfront_porch;
		/* pico second */
		info->var.pixclock = KHZ2PICOS(vm.pixelclock/1000);
	}

	return 0;

err_drm_fb_destroy:
	drm_framebuffer_unregister_private(fb);
	nx_drm_fb_destroy(fb);

err_framebuffer_release:
	framebuffer_release(info);

err_drm_gem_free_object:
	nx_drm_gem_destroy(nx_obj);

	return ret;
}

static const struct drm_fb_helper_funcs nx_drm_fb_helper = {
	.fb_probe = nx_drm_fb_helper_probe,
};

static struct nx_drm_fbdev *nx_drm_fbdev_init(struct drm_device *drm,
			unsigned int preferred_bpp, unsigned int num_crtc,
			unsigned int max_conn_count)
{
	struct nx_drm_fbdev *fbdev;
	struct drm_fb_helper *fb_helper;
	int ret;

	fbdev = kzalloc(sizeof(*fbdev), GFP_KERNEL);
	if (!fbdev)
		return ERR_PTR(-ENOMEM);

	fb_helper = &fbdev->fb_helper;
	fbdev->fb_buffers = 1;

	if (fb_buffer_count > 0)
		fbdev->fb_buffers = fb_buffer_count;

	DRM_INFO("FB counts = %d\n", fbdev->fb_buffers);

	drm_fb_helper_prepare(drm, fb_helper, &nx_drm_fb_helper);

	ret = drm_fb_helper_init(drm, fb_helper, num_crtc, max_conn_count);
	if (ret < 0) {
		dev_err(drm->dev, "Failed to initialize drm fb fb_helper.\n");
		goto err_free;
	}

	ret = drm_fb_helper_single_add_all_connectors(fb_helper);
	if (ret < 0) {
		dev_err(drm->dev, "Failed to add connectors.\n");
		goto err_drm_fb_helper_fini;

	}

	/* disable all the possible outputs/crtcs before entering KMS mode */
	drm_helper_disable_unused_functions(drm);

	ret = drm_fb_helper_initial_config(fb_helper, preferred_bpp);
	if (ret < 0) {
		dev_err(drm->dev, "Failed to set initial hw configuration.\n");
		goto err_drm_fb_helper_fini;
	}

	return fbdev;

err_drm_fb_helper_fini:
	drm_fb_helper_fini(fb_helper);
err_free:
	kfree(fbdev);

	return ERR_PTR(ret);
}

static void nx_drm_fbdev_fini(struct nx_drm_fbdev *fbdev)
{
	if (fbdev->fb_helper.fbdev) {
		struct fb_info *info;
		int ret;

		info = fbdev->fb_helper.fbdev;
		ret = unregister_framebuffer(info);
		if (ret < 0)
			DRM_DEBUG_KMS("failed unregister_framebuffer()\n");

		if (info->cmap.len)
			fb_dealloc_cmap(&info->cmap);

		framebuffer_release(info);
	}

	if (fbdev->fb) {
		drm_framebuffer_unregister_private(&fbdev->fb->fb);
		nx_drm_fb_destroy(&fbdev->fb->fb);
	}

	drm_fb_helper_fini(&fbdev->fb_helper);
	kfree(fbdev);
}

int nx_drm_framebuffer_init(struct drm_device *drm)
{
	struct nx_drm_fbdev *fbdev;
	struct nx_drm_priv *priv = drm->dev_private;
	struct nx_framebuffer_dev *nx_framebuffer;
	unsigned int num_crtc;
	int bpp;
	int ret = 0;

	if (!drm->mode_config.num_crtc ||
		!drm->mode_config.num_connector)
		return 0;

	DRM_DEBUG_KMS("enter crtc num:%d, connector num:%d\n",
		      drm->mode_config.num_crtc,
		      drm->mode_config.num_connector);

	nx_framebuffer = kzalloc(sizeof(*nx_framebuffer), GFP_KERNEL);
	if (!nx_framebuffer)
		return -ENOMEM;

	priv->framebuffer_dev = nx_framebuffer;
	num_crtc = drm->mode_config.num_crtc;
	bpp = PREFERRED_BPP;

	fbdev = nx_drm_fbdev_init(drm, bpp, num_crtc, MAX_CONNECTOR);
	if (IS_ERR(fbdev)) {
		ret = PTR_ERR(fbdev);
		goto err_drm_fb_dev_free;
	}

	nx_framebuffer->fbdev = fbdev;

	return 0;

err_drm_fb_dev_free:
	kfree(nx_framebuffer);
	return ret;
}

void nx_drm_framebuffer_fini(struct drm_device *drm)
{
	struct nx_drm_priv *priv = drm->dev_private;
	struct nx_framebuffer_dev *nx_framebuffer = priv->framebuffer_dev;
	struct nx_drm_fbdev *fbdev = nx_framebuffer->fbdev;

	nx_drm_fbdev_fini(fbdev);
	kfree(nx_framebuffer);
	priv->framebuffer_dev = NULL;
}

/*
 * fb with gem
 */
struct nx_gem_object *nx_drm_fb_get_gem_obj(struct drm_framebuffer *fb,
			unsigned int plane)
{
	struct nx_drm_fb *nx_fb = to_nx_drm_fb(fb);

	if (plane >= 4)
		return NULL;

	return nx_fb->obj[plane];
}
EXPORT_SYMBOL_GPL(nx_drm_fb_get_gem_obj);
