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
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>

#include <linux/dma-buf.h>

#include "nx_drm_drv.h"
#include "nx_drm_fb.h"
#include "nx_drm_gem.h"

#define PREFERRED_BPP		32

static bool fb_format_bgr;

MODULE_PARM_DESC(fb_bgr, "frame buffer BGR pixel format");
module_param_named(fb_bgr, fb_format_bgr, bool, 0600);

static uint fb_pan_crtcs = 0xff;

#ifdef CONFIG_DRM_NX_FB_PAN_DISPLAY
#if defined(CONFIG_DRM_PANEL_FRIENDLYELEC)
/* recommended for no flickering */
static int fb_buffer_count = 3;
#else
static int fb_buffer_count = 1;
#endif
static bool fb_vblank_wait;

MODULE_PARM_DESC(fb_buffers, "frame buffer count");
module_param_named(fb_buffers, fb_buffer_count, int, 0600);

MODULE_PARM_DESC(fb_vblank, "frame buffer wait vblank for pan display");
module_param_named(fb_vblank, fb_vblank_wait, bool, 0600);

MODULE_PARM_DESC(fb_pan_crtcs, "frame buffer pan display possible crtcs");
module_param_named(fb_pan_crtcs, fb_pan_crtcs, uint, 0600);
#endif

#ifdef CONFIG_FB_PRE_INIT_FB
#include <linux/memblock.h>
#include <linux/of_address.h>

static int fb_splash_image_copy(struct device *dev,
			phys_addr_t logo_phys, size_t logo_size,
			void *screen_virt, size_t screen_size)
{
	int npages, i;
	struct page **pages;
	void *logo_virt;

	npages = PAGE_ALIGN(logo_size) / PAGE_SIZE;
	pages = kcalloc(npages, sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return -ENOMEM;

	for (i = 0; i < npages; i++)
		pages[i] = pfn_to_page((logo_phys / PAGE_SIZE) + i);

	logo_virt = vmap(pages, npages, VM_MAP, PAGE_KERNEL);
	if (!logo_virt) {
		dev_err(dev, "Failed to map for splash image:0x%lx:%u\n",
			(unsigned long)logo_phys, (unsigned)logo_size);
		kfree(pages);
		return -ENOMEM;
	}

	DRM_INFO("Copy splash image from 0x%lx(0x%p) to 0x%p size %u\n",
		(unsigned long)logo_phys, logo_virt, screen_virt,
		(unsigned)screen_size);

	memcpy(screen_virt, logo_virt, screen_size);

	kfree(pages);
	vunmap(logo_virt);

	return 0;
}

/*
 * get previous FB base address from hw register
 */
static dma_addr_t fb_get_splash_base(struct drm_fb_helper *fb_helper)
{
	struct drm_crtc *crtc;
	dma_addr_t dma_addr = 0;
	int i;

	for (i = 0; fb_helper->crtc_count > i; i++) {
		crtc = fb_helper->crtc_info[i].mode_set.crtc;
		if (crtc->primary) {
			dma_addr =
				nx_drm_framebuffer_get_dma_addr(crtc->primary);
			if (dma_addr)
				return dma_addr;
		}
	}
	return 0;
}

void fb_set_splash_base(struct drm_fb_helper *fb_helper,
			dma_addr_t screen_phys)
{
	struct drm_crtc *crtc;
	int i;

	for (i = 0; fb_helper->crtc_count > i; i++) {
		crtc = fb_helper->crtc_info[i].mode_set.crtc;
		if (crtc->primary) {
			nx_drm_framebuffer_set_dma_addr(
					crtc->primary, screen_phys);
			return;
		}
	}
}

static int nx_drm_fb_pre_boot_logo(struct drm_fb_helper *fb_helper)
{
	struct device_node *node;
	struct drm_device *drm = fb_helper->dev;
	struct fb_info *info = fb_helper->fbdev;
	void *screen_base = info->screen_base;
	int size = info->var.xres * info->var.yres *
			(info->var.bits_per_pixel / 8);
	dma_addr_t screen_phys = drm->mode_config.fb_base;
	dma_addr_t logo_phys = 0;
	int reserved = 0;
	struct resource r;
	int ret;

	node = of_find_node_by_name(NULL, "display_reserved");
	if (node) {
		ret = of_address_to_resource(node, 0, &r);
		if (!ret) {
			of_node_put(node);

			reserved = memblock_is_region_reserved(
					r.start, resource_size(&r));
			if (!reserved)
				DRM_INFO("can't reserved splash:0x%pa:%d\n",
					&r.start, (int)resource_size(&r));

			if (reserved) {
				if (size > resource_size(&r)) {
					DRM_INFO(
					"WARN: reserved splash size %d less %d",
					(int)resource_size(&r), size);
				}
			}
			logo_phys = r.start;
		}
	}

	if (!logo_phys) {
		DRM_INFO("splash image: no reserved memory-region\n");
		DRM_INFO("splash image: memroy from HW\n");
		logo_phys = fb_get_splash_base(fb_helper);
		if (!logo_phys) {
			DRM_ERROR("splash image: not setted memory !!!\n");
			return -EFAULT;
		}
	}

	fb_splash_image_copy(drm->dev, logo_phys, size, screen_base, size);
	fb_set_splash_base(fb_helper, screen_phys);

	if (reserved) {
		memblock_free(r.start, resource_size(&r));
		memblock_remove(r.start, resource_size(&r));
		free_reserved_area(__va(r.start),
				__va(r.start + resource_size(&r)), -1, NULL);
	}

	return 0;
}
#endif

static inline int possible_crtc_count(unsigned int n)
{
	int counter = 0;

	while (n) {
		counter++;
		n &= (n - 1);
	}
	return counter;
}

#ifdef CONFIG_DRM_NX_FB_PAN_DISPLAY
static void nx_drm_fb_pan_wait_for_vblanks(struct drm_device *dev,
		struct drm_atomic_state *old_state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *old_crtc_state;
	unsigned int possible_crtcs = fb_pan_crtcs;
	int i, ret;

	for_each_crtc_in_state(old_state, crtc, old_crtc_state, i) {
		if (!(possible_crtcs & 1<<i))
			continue;

		/*
		 * No one cares about the old state, so abuse it for tracking
		 * and store whether we hold a vblank reference (and should
		 * do a vblank wait) in the ->enable boolean.
		 */
		old_crtc_state->enable = false;

		if (!crtc->state->enable)
			continue;

		/*
		 * Legacy cursor ioctls are completely unsynced, and userspace
		 * relies on that (by doing tons of cursor updates).
		 */
		if (old_state->legacy_cursor_update)
			continue;

		ret = drm_crtc_vblank_get(crtc);
		if (ret != 0)
			continue;

		old_crtc_state->enable = true;
		old_crtc_state->last_vblank_count = drm_crtc_vblank_count(crtc);
	}

	for_each_crtc_in_state(old_state, crtc, old_crtc_state, i) {
		if (!(possible_crtcs & 1<<i))
			continue;

		if (!old_crtc_state->enable)
			continue;

		ret = wait_event_timeout(dev->vblank[i].queue,
				old_crtc_state->last_vblank_count !=
					drm_crtc_vblank_count(crtc),
				msecs_to_jiffies(50));

		drm_crtc_vblank_put(crtc);
	}
}

static int nx_drm_fb_atomic_crtc_check(struct drm_crtc *crtc,
		struct drm_crtc_state *state)
{
	/* NOTE: we explicitly don't enforce constraints such as primary
	 * layer covering entire screen, since that is something we want
	 * to allow (on hw that supports it).  For hw that does not, it
	 * should be checked in driver's crtc->atomic_check() vfunc.
	 *
	 * TODO: Add generic modeset state checks once we support those.
	 */

	if (state->active && !state->enable) {
		DRM_DEBUG_ATOMIC("[CRTC:%d] active without enabled\n",
				 crtc->base.id);
		return -EINVAL;
	}

	/*
	 * The state->enable vs. state->mode_blob checks can be WARN_ON,
	 * as this is a kernel-internal detail that userspace should never
	 * be able to trigger.
	 */
	if (drm_core_check_feature(crtc->dev, DRIVER_ATOMIC) &&
	    WARN_ON(state->enable && !state->mode_blob)) {
		DRM_DEBUG_ATOMIC("[CRTC:%d] enabled without mode blob\n",
				 crtc->base.id);
		return -EINVAL;
	}

	if (drm_core_check_feature(crtc->dev, DRIVER_ATOMIC) &&
	    WARN_ON(!state->enable && state->mode_blob)) {
		DRM_DEBUG_ATOMIC("[CRTC:%d] disabled with mode blob\n",
				 crtc->base.id);
		return -EINVAL;
	}

	return 0;
}

static int nx_drm_fb_atomic_check_only(struct drm_atomic_state *state)
{
	struct drm_device *dev = state->dev;
	struct drm_mode_config *config = &dev->mode_config;
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	int i, ret = 0;

	DRM_DEBUG_ATOMIC("checking %p\n", state);

	for_each_crtc_in_state(state, crtc, crtc_state, i) {
		ret = nx_drm_fb_atomic_crtc_check(crtc, crtc_state);
		if (ret) {
			DRM_DEBUG_ATOMIC("[CRTC:%d] atomic core check failed\n",
					 crtc->base.id);
			return ret;
		}
	}

	if (config->funcs->atomic_check)
		ret = config->funcs->atomic_check(state->dev, state);

	if (!state->allow_modeset) {
		for_each_crtc_in_state(state, crtc, crtc_state, i) {
			if (drm_atomic_crtc_needs_modeset(crtc_state)) {
				DRM_DEBUG_ATOMIC(
					"[CRTC:%d] requires full modeset\n",
					 crtc->base.id);
				return -EINVAL;
			}
		}
	}

	return ret;
}

static int nx_drm_fb_atomic_commit(struct drm_device *drm,
			struct drm_atomic_state *state)
{
	int ret;

	ret = nx_drm_fb_atomic_check_only(state);
	if (ret)
		return ret;

	ret = drm_atomic_helper_prepare_planes(drm, state);
	if (ret)
		return ret;

	drm_atomic_helper_swap_state(drm, state);

	drm_atomic_helper_commit_modeset_disables(drm, state);
	drm_atomic_helper_commit_planes(drm, state, false);
	drm_atomic_helper_commit_modeset_enables(drm, state);

	/*
	 * wait hw vblank
	 * skip 1st frame, vblank is not enabled at 1st,
	 * and not changed framebuffer
	 */
	if (fb_vblank_wait)
		nx_drm_fb_pan_wait_for_vblanks(drm, state);

	drm_atomic_helper_cleanup_planes(drm, state);
	drm_atomic_state_free(state);

	return 0;
}

static inline int nx_drm_fb_mode_lock(struct drm_fb_helper *fb_helper,
			unsigned int possible_crtcs, bool *lock_all)
{
	struct drm_device *drm = fb_helper->dev;
	struct drm_crtc *crtc;
	struct drm_mode_set *mode_set;
	unsigned int mask = (fb_helper->crtc_count << 1) - 1;
	unsigned int possible = possible_crtcs & mask;
	int i;

	if (possible == mask || possible_crtc_count(possible) > 1) {
		drm_modeset_lock_all(drm);
		*lock_all = true;
		return 0;
	}

	for (i = 0; i < fb_helper->crtc_count; i++) {

		if (!(possible_crtcs & 1<<i))
			continue;

		mode_set = &fb_helper->crtc_info[i].mode_set;
		crtc = mode_set->crtc;
		drm_modeset_lock_crtc(crtc, crtc->primary);
		if (crtc->primary->fb == NULL) {
			/* The framebuffer is currently unbound, presumably
			 * due to a hotplug event, that userspace has not
			 * yet discovered.
			 */
			drm_modeset_unlock_crtc(crtc);
			return -EBUSY;
		}
		*lock_all = false;
	}

	return 0;
}

static inline void nx_drm_fb_mode_unlock(struct drm_fb_helper *fb_helper,
			unsigned int possible_crtcs, bool lock_all)
{
	struct drm_device *drm = fb_helper->dev;
	struct drm_crtc *crtc;
	struct drm_mode_set *mode_set;
	int i;

	if (lock_all) {
		drm_modeset_unlock_all(drm);
		return;
	}

	for (i = 0; i < fb_helper->crtc_count; i++) {

		if (!(possible_crtcs & 1<<i))
			continue;

		mode_set = &fb_helper->crtc_info[i].mode_set;
		crtc = mode_set->crtc;
		drm_modeset_unlock_crtc(crtc);
	}
}

static int nx_drm_fb_pan_display_atomic(struct fb_var_screeninfo *var,
			      struct fb_info *info)
{
	struct drm_fb_helper *fb_helper = info->par;
	struct drm_device *drm = fb_helper->dev;
	struct drm_atomic_state *state;
	struct drm_plane *plane;
	unsigned plane_mask;
	unsigned int possible_crtcs = fb_pan_crtcs;
	bool lock_all = true;
	int i, ret = 0;

	ret = nx_drm_fb_mode_lock(fb_helper, possible_crtcs, &lock_all);
	if (ret)
		return ret;

	state = drm_atomic_state_alloc(drm);
	if (!state) {
		nx_drm_fb_mode_unlock(fb_helper, possible_crtcs, lock_all);
		return -ENOMEM;
	}

	if (lock_all)
		state->acquire_ctx = drm->mode_config.acquire_ctx;

retry:
	plane_mask = 0;

	for (i = 0; i < fb_helper->crtc_count; i++) {
		struct drm_mode_set *mode_set;
		struct drm_crtc *crtc;

		if (!(possible_crtcs & 1<<i))
			continue;

		mode_set = &fb_helper->crtc_info[i].mode_set;
		/*
		 * set pan buffer pointer
		 */
		mode_set->x = var->xoffset;
		mode_set->y = var->yoffset;
		crtc = mode_set->crtc;

		if (!lock_all)
			state->acquire_ctx =
				drm_modeset_legacy_acquire_ctx(crtc);

		ret = __drm_atomic_helper_set_config(mode_set, state);
		if (ret != 0)
			goto fail;

		plane = mode_set->crtc->primary;
		plane_mask |= (1 << drm_plane_index(plane));
		plane->old_fb = plane->fb;
	}

	ret = nx_drm_fb_atomic_commit(drm, state);
	if (ret != 0)
		goto fail;

	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;

fail:
	drm_atomic_clean_old_fb(drm, plane_mask, ret);

	if (ret == -EDEADLK)
		goto backoff;

	if (ret != 0)
		drm_atomic_state_free(state);

	nx_drm_fb_mode_unlock(fb_helper, possible_crtcs, lock_all);

	return ret;

backoff:
	drm_atomic_state_clear(state);
	drm_atomic_legacy_backoff(state);

	goto retry;
}

#ifdef CONFIG_DRM_CHECK_IOCTL_AUTH
static bool nx_drm_fb_is_bound(struct drm_fb_helper *fb_helper)
{
	struct drm_device *drm = fb_helper->dev;
	struct drm_crtc *crtc;
	int bound = 0, crtcs_bound = 0;

	/*
	 * Sometimes user space wants everything disabled, so don't steal the
	 * display if there's a master.
	 */
	if (drm->primary->master)
		return false;

	drm_for_each_crtc(crtc, drm) {
		if (crtc->primary->fb)
			crtcs_bound++;
		if (crtc->primary->fb == fb_helper->fb)
			bound++;
	}

	if (bound < crtcs_bound)
		return false;

	return true;
}
#endif

static int nx_drm_fb_pan_display(struct fb_var_screeninfo *var,
			      struct fb_info *info)
{
	struct drm_fb_helper *fb_helper = info->par;
	struct drm_device *drm = fb_helper->dev;

	if (oops_in_progress)
		return -EBUSY;

	drm_modeset_lock_all(drm);

#ifdef CONFIG_DRM_CHECK_IOCTL_AUTH
	if (!nx_drm_fb_is_bound(fb_helper)) {
		drm_modeset_unlock_all(drm);
		return -EBUSY;
	}
#endif
	if (!fb_helper->atomic) {
		DRM_ERROR("Error: no atomic is not support pan display!\n");
		drm_modeset_unlock_all(drm);
		return -EINVAL;
	}
	drm_modeset_unlock_all(drm);

	return nx_drm_fb_pan_display_atomic(var, info);
}
#endif

static void nx_drm_fb_destroy(struct drm_framebuffer *fb)
{
	struct nx_drm_fb *nx_fb = to_nx_drm_fb(fb);
	int i;

	drm_framebuffer_cleanup(fb);

	for (i = 0; i < ARRAY_SIZE(nx_fb->obj); i++) {
		struct drm_gem_object *obj;

		if (!nx_fb->obj[i])
			continue;

		obj = &nx_fb->obj[i]->base;
		drm_gem_object_unreference_unlocked(obj);
	}
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

static int nx_drm_fb_check_var(struct fb_var_screeninfo *var,
			       struct fb_info *info)
{
	struct drm_fb_helper *fb_helper = info->par;
	struct drm_framebuffer *fb = fb_helper->fb;
	int depth;

	if (var->pixclock != 0 || in_dbg_master())
		return -EINVAL;

	/* Need to resize the fb object !!! */
	if (var->bits_per_pixel > fb->bits_per_pixel ||
	    var->xres > fb->width || var->yres > fb->height ||
	    var->xres_virtual > info->var.xres_virtual ||
	    var->yres_virtual > info->var.yres_virtual) {
		pr_err("fb userspace requested width/height/bpp is greater than current fb "
			  "request %dx%d-%d (virtual %dx%d) > %dx%d-%d (virtual %dx%d)\n",
			  var->xres, var->yres, var->bits_per_pixel,
			  var->xres_virtual, var->yres_virtual,
			  fb->width, fb->height, fb->bits_per_pixel,
			  info->var.xres_virtual, info->var.yres_virtual);
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
	case 32:
		var->red.offset = 16;
		var->green.offset = 8;
		var->blue.offset = 0;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->transp.length = 8;
		var->transp.offset = 24;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct dma_buf *nx_drm_fb_dmabuf_export(struct fb_info *info)
{
	struct drm_fb_helper *fb_helper = info->par;
	struct drm_device *dev = fb_helper->dev;
	struct nx_drm_fbdev *nx_fbdev = to_nx_drm_fbdev(fb_helper);
	struct nx_gem_object *nx_gem_obj = nx_fbdev->fb->obj[0];
	struct dma_buf *buf = NULL;

	if (dev->driver->gem_prime_export) {
		buf = dev->driver->gem_prime_export(dev, &nx_gem_obj->base, O_RDWR);
		if (buf)
			drm_gem_object_reference(&nx_gem_obj->base);
	}

	return buf;
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
	.fb_check_var	= nx_drm_fb_check_var,
	.fb_set_par	= drm_fb_helper_set_par,
	.fb_blank	= drm_fb_helper_blank,
	.fb_setcmap	= drm_fb_helper_setcmap,
	.fb_pan_display	= drm_fb_helper_pan_display,
	.fb_dmabuf_export = nx_drm_fb_dmabuf_export,
};

static struct nx_drm_fb *nx_drm_fb_alloc(struct drm_device *drm,
			struct drm_mode_fb_cmd2 *mode_cmd,
			struct nx_gem_object **nx_obj,
			unsigned int num_planes)
{
	struct nx_drm_fb *nx_fb;
	int i, ret;

	nx_fb = kzalloc(sizeof(*nx_fb), GFP_KERNEL);
	if (!nx_fb)
		return ERR_PTR(-ENOMEM);

	drm_helper_mode_fill_fb_struct(&nx_fb->fb, mode_cmd);

	for (i = 0; i < num_planes; i++)
		nx_fb->obj[i] = nx_obj[i];

	ret = drm_framebuffer_init(drm, &nx_fb->fb, &nx_drm_framebuffer_funcs);
	if (ret) {
		DRM_ERROR("Failed to initialize framebuffer:%d\n", ret);
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
			DRM_ERROR("Failed to lookup GEM object\n");
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
	struct nx_drm_fbdev *nx_fbdev = to_nx_drm_fbdev(fb_helper);
	struct drm_device *drm = fb_helper->dev;
	struct drm_mode_fb_cmd2 mode_cmd = { 0 };
	struct nx_gem_object *nx_obj;
	struct drm_framebuffer *fb;
	unsigned int bytes_per_pixel;
	unsigned long offset;
	struct fb_info *info;
	size_t size;
	unsigned int flags = 0;
	int buffers = nx_fbdev->fb_buffers;
	int ret;

	DRM_INFO("framebuffer width(%d), height(%d) and bpp(%d) buffers(%d)\n",
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

	nx_fbdev->fb = nx_drm_fb_alloc(drm, &mode_cmd, &nx_obj, 1);
	if (IS_ERR(fb_helper->fb)) {
		DRM_ERROR("Failed to allocate DRM framebuffer.\n");
		ret = PTR_ERR(fb_helper->fb);
		goto err_framebuffer_release;
	}
	fb_helper->fb = &nx_fbdev->fb->fb;
	fb = fb_helper->fb;

	info = drm_fb_helper_alloc_fbi(fb_helper);
	if (IS_ERR(info)) {
		DRM_ERROR("Failed to allocate framebuffer info.\n");
		ret = PTR_ERR(info);
		goto err_drm_fb_destroy;
	}
	info->par = fb_helper;
	info->flags = FBINFO_FLAG_DEFAULT;
	info->fbops = &nx_fb_ops;

	drm_fb_helper_fill_fix(info, fb->pitches[0], fb->depth);
	drm_fb_helper_fill_var(info, fb_helper,
			sizes->fb_width, sizes->fb_height);

#ifdef CONFIG_DRM_NX_FB_PAN_DISPLAY
	/* for double buffer */
	info->var.yres_virtual = fb->height * buffers;
	if (buffers > 0)
		info->fbops->fb_pan_display = nx_drm_fb_pan_display;
#endif

	offset = info->var.xoffset * bytes_per_pixel;
	offset += info->var.yoffset * fb->pitches[0];

	drm->mode_config.fb_base = (resource_size_t)nx_obj->dma_addr;
	info->screen_base = (void __iomem *)nx_obj->cpu_addr + offset;
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

#ifdef CONFIG_FB_PRE_INIT_FB
	nx_drm_fb_pre_boot_logo(fb_helper);
#endif
	return 0;

err_drm_fb_destroy:
	nx_drm_fb_destroy(fb);

err_framebuffer_release:
	nx_drm_gem_destroy(nx_obj);

	return ret;
}

static const struct drm_fb_helper_funcs nx_drm_fb_helper = {
	.fb_probe = nx_drm_fb_helper_probe,
};

static int nx_drm_framebuffer_dev_init(struct drm_device *drm,
			unsigned int preferred_bpp, unsigned int num_crtc,
			unsigned int max_conn_count)
{
	struct nx_drm_private *private = drm->dev_private;
	struct nx_drm_fbdev *nx_fbdev;
	struct drm_fb_helper *fb_helper;
	int ret;

	nx_fbdev = kzalloc(sizeof(*nx_fbdev), GFP_KERNEL);
	if (!nx_fbdev)
		return -ENOMEM;

	private->fbdev = nx_fbdev;
	fb_helper = &nx_fbdev->fb_helper;

	nx_fbdev->fb_buffers = 1;

#ifdef CONFIG_DRM_NX_FB_PAN_DISPLAY
	if (fb_buffer_count > 0)
		nx_fbdev->fb_buffers = fb_buffer_count;

	DRM_INFO("FB counts = %d, FB vblank %s fb crtcs %d [0x%x]\n",
		nx_fbdev->fb_buffers, fb_vblank_wait ? "Wait" : "Pass",
		num_crtc, fb_pan_crtcs);
#endif

	drm_fb_helper_prepare(drm, fb_helper, &nx_drm_fb_helper);

	ret = drm_fb_helper_init(drm, fb_helper, num_crtc, max_conn_count);
	if (ret < 0) {
		DRM_ERROR("Failed to initialize drm fb fb_helper.\n");
		goto err_free;
	}

	ret = drm_fb_helper_single_add_all_connectors(fb_helper);
	if (ret < 0) {
		DRM_ERROR("Failed to add connectors.\n");
		goto err_drm_fb_helper_fini;

	}

	ret = drm_fb_helper_initial_config(fb_helper, preferred_bpp);
	if (ret < 0) {
		DRM_ERROR("Failed to set initial hw configuration.\n");
		goto err_drm_fb_helper_fini;
	}

	return 0;

err_drm_fb_helper_fini:
	drm_fb_helper_fini(fb_helper);
err_free:
	kfree(nx_fbdev);
	private->fbdev = NULL;

	return ret;
}

static void nx_drm_framebuffer_dev_fini(struct drm_device *drm)
{
	struct nx_drm_private *private = drm->dev_private;
	struct drm_fb_helper *fb_helper;
	struct drm_framebuffer *fb;

	if (!private->fbdev)
		return;

	fb_helper = &private->fbdev->fb_helper;

	/* release drm framebuffer and real buffer */
	if (fb_helper->fb && fb_helper->fb->funcs) {
		fb = fb_helper->fb;
		if (fb) {
			drm_framebuffer_unregister_private(fb);
			drm_framebuffer_remove(fb);
		}
	}

	drm_fb_helper_unregister_fbi(fb_helper);
	drm_fb_helper_release_fbi(fb_helper);
	drm_fb_helper_fini(fb_helper);

	kfree(private->fbdev);
	private->fbdev = NULL;
}

int nx_drm_framebuffer_init(struct drm_device *drm)
{
	int bpp = PREFERRED_BPP;
	int num_crtc = possible_crtc_count(fb_pan_crtcs);

	if (!drm->mode_config.num_crtc ||
		!drm->mode_config.num_connector || !num_crtc)
		return 0;

	if (num_crtc > drm->mode_config.num_crtc)
		num_crtc = drm->mode_config.num_crtc;

	DRM_DEBUG_KMS("crtc num:%d (possible %d), connector num:%d\n",
		      drm->mode_config.num_crtc, num_crtc,
		      drm->mode_config.num_connector);

	return nx_drm_framebuffer_dev_init(drm, bpp, num_crtc, MAX_CONNECTOR);
}

void nx_drm_framebuffer_fini(struct drm_device *drm)
{
	nx_drm_framebuffer_dev_fini(drm);
}

/*
 * fb with gem
 */
struct nx_gem_object *nx_drm_fb_gem(struct drm_framebuffer *fb,
			unsigned int plane)
{
	struct nx_drm_fb *nx_fb = to_nx_drm_fb(fb);

	if (plane >= 4)
		return NULL;

	return nx_fb->obj[plane];
}
EXPORT_SYMBOL_GPL(nx_drm_fb_gem);

