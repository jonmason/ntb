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
#include <drm/drm_gem_cma_helper.h>

#include "nx_drm_drv.h"
#include "nx_drm_plane.h"
#include "nx_drm_crtc.h"
#include "nx_drm_fb.h"
#include "nx_drm_encoder.h"
#include "soc/nx_display.h"

#define to_nx_crtc(x)	\
			container_of(x, struct nx_drm_crtc, drm_crtc)

static int nx_drm_crtc_update(struct drm_crtc *crtc)
{
	struct nx_drm_crtc *nx_crtc;
	struct nx_drm_crtc_pos pos;
	struct drm_display_mode *mode = &crtc->mode;
	struct drm_framebuffer *fb = crtc->primary->fb;

	DRM_DEBUG_KMS("Enter mode:%p, fb:%p\n", mode, fb);

	if (!mode || !fb)
		return -EINVAL;

	nx_crtc = to_nx_crtc(crtc);

	memset(&pos, 0, sizeof(struct nx_drm_crtc_pos));

	/* it means the offset of framebuffer to be displayed. */
	pos.fb_x = crtc->x;
	pos.fb_y = crtc->y;

	/* OSD position to be displayed. */
	pos.crtc_x = 0;
	pos.crtc_y = 0;
	pos.crtc_w = fb->width - crtc->x;
	pos.crtc_h = fb->height - crtc->y;

	return 0;
}

static void nx_drm_crtc_dpms(struct drm_crtc *crtc, int mode)
{
	struct drm_device *drm = crtc->dev;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);

	DRM_DEBUG_KMS("Enter CRTC.%d ID:%d mode:%d (%s)\n",
		      nx_crtc->nr, crtc->base.id, mode,
		      mode == DRM_MODE_DPMS_ON ? "ON" :
		      mode == DRM_MODE_DPMS_OFF ? "OFF" :
		      mode == DRM_MODE_DPMS_STANDBY ? "STANDBY" :
		      mode == DRM_MODE_DPMS_SUSPEND ? "SUSPEND" : "Unknown");

	if (nx_crtc->dpms_mode == mode) {
		DRM_DEBUG_KMS("desired dpms mode %d is same as previous one.\n",
			      mode);
		return;
	}

	mutex_lock(&drm->struct_mutex);

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		nx_crtc->dpms_mode = mode;
		break;
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		nx_crtc->dpms_mode = mode;
		break;
	default:
		DRM_ERROR("Unspecified mode %d\n", mode);
		break;
	}

	mutex_unlock(&drm->struct_mutex);
}

static void nx_drm_crtc_prepare(struct drm_crtc *crtc)
{
	DRM_DEBUG_KMS("Enter\n");
	/* drm framework doesn't check NULL. */
}

static void nx_drm_crtc_commit(struct drm_crtc *crtc)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct drm_framebuffer *fb = crtc->primary->fb;
	struct drm_gem_cma_object *obj = nx_drm_fb_get_gem_obj(fb, 0);
	struct nx_drm_plane *nx_plane = to_nx_plane(crtc->primary);

	int module, layer;
	int width, height, pixel;
	uint32_t pixel_format;
	dma_addr_t paddr;
	u32 bgcolor = 0x0000FF;

	module = nx_crtc->nr;
	layer = nx_plane->layer;
	width = fb->width;
	height = fb->height;
	pixel = fb->bits_per_pixel >> 3;
	pixel_format = fb->pixel_format;

	paddr = (unsigned long)obj->paddr;

	DRM_DEBUG_KMS("+[crtc.%d:%d %4dx%4d, %d pix: format:0x%x, addr:0x%x]\n",
		module, layer, width, height, pixel,
		pixel_format, (u32)paddr);

	/*
	 * when set_crtc is requested from user or at booting time,
	 * crtc->commit would be called without dpms call so if dpms is
	 * no power on then crtc->dpms should be called
	 * with DRM_MODE_DPMS_ON for the hardware power to be on.
	 */
	if (nx_crtc->dpms_mode != DRM_MODE_DPMS_ON) {
		int mode = DRM_MODE_DPMS_ON;

		/*
		 * enable hardware(power on) to all encoders hdmi connected
		 * to current crtc.
		 */
		nx_drm_crtc_dpms(crtc, mode);
	}

	nx_soc_disp_set_bg_color(module, bgcolor);
	nx_soc_disp_rgb_set_format(module, layer, pixel_format, width, height,
				    pixel);
	nx_soc_disp_rgb_set_address(module, layer, paddr, pixel,
					width * pixel, 1);
	nx_soc_disp_rgb_set_enable(module, layer, 1);
	nx_soc_disp_device_enable_all(module, 1);

	DRM_DEBUG_KMS("Exit.\n");
}

static bool nx_drm_crtc_mode_fixup(struct drm_crtc *crtc,
				const struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
	DRM_DEBUG_KMS("Enter\n");

	/* drm framework doesn't check NULL */
	return true;
}

static int nx_drm_crtc_mode_set(struct drm_crtc *crtc,
				struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode,
				int x, int y,
				struct drm_framebuffer *old_fb)
{
	DRM_DEBUG_KMS("Enter\n");

	/*
	 * copy the mode data adjusted by mode_fixup() into crtc->mode
	 * so that hardware can be seet to proper mode.
	 */
	memcpy(&crtc->mode, adjusted_mode, sizeof(*adjusted_mode));

	return nx_drm_crtc_update(crtc);
}

static int nx_drm_crtc_mode_set_base(struct drm_crtc *crtc, int x, int y,
				struct drm_framebuffer *old_fb)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct drm_framebuffer *fb = crtc->primary->fb;
	struct drm_gem_cma_object *obj = nx_drm_fb_get_gem_obj(fb, 0);
	struct nx_drm_plane *nx_plane = to_nx_plane(crtc->primary);

	int module, layer = 0;
	int width, pixel;
	uint32_t offset;
	dma_addr_t paddr;
	int ret;

	module = nx_crtc->nr;
	layer = nx_plane->layer;
	width = fb->width;
	pixel = fb->bits_per_pixel >> 3;

	offset = y * fb->pitches[0] + x * (fb->bits_per_pixel >> 3);
	paddr = (unsigned long)obj->paddr + offset;

	DRM_DEBUG_KMS("+[crtc.%d:%d %4dx%4d, %d Pix, addr:0x%x]\n",
		module, layer, fb->width, fb->height,
		pixel, (u32)paddr);

	nx_soc_disp_rgb_set_address(module, layer, paddr, pixel,
					width * pixel, 1);

	ret = nx_drm_crtc_update(crtc);
	if (ret)
		return ret;

	DRM_DEBUG_DRIVER("Exit.\n");
	return ret;
}

static void nx_drm_crtc_disable(struct drm_crtc *crtc)
{
	struct drm_plane *plane;
	int ret;

	DRM_DEBUG_KMS("Enter\n");
	nx_drm_crtc_dpms(crtc, DRM_MODE_DPMS_OFF);

	drm_for_each_plane(plane, crtc->dev) {
		if (plane->crtc != crtc)
			continue;
		ret = plane->funcs->disable_plane(plane);
		if (ret)
			DRM_ERROR("Failed to disable plane %d\n", ret);
	}
}

static struct drm_crtc_helper_funcs nx_crtc_helper_funcs = {
	.dpms = nx_drm_crtc_dpms,
	.prepare = nx_drm_crtc_prepare,
	.commit = nx_drm_crtc_commit,
	.mode_fixup = nx_drm_crtc_mode_fixup,
	.mode_set = nx_drm_crtc_mode_set,
	.mode_set_base = nx_drm_crtc_mode_set_base,
	.disable = nx_drm_crtc_disable,
};

static int nx_drm_crtc_page_flip(struct drm_crtc *crtc,
				struct drm_framebuffer *fb,
				struct drm_pending_vblank_event *event,
				uint32_t flags)
{
	struct drm_device *drm = crtc->dev;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct drm_framebuffer *old_fb = crtc->primary->fb;
	int nr = nx_crtc->nr;
	int ret = -EINVAL;

	DRM_DEBUG_KMS("Enter\n");

	mutex_lock(&drm->struct_mutex);

	if (event) {
		event->pipe = nr;
		ret = drm_vblank_get(drm, nr);
		if (ret) {
			DRM_DEBUG("Failed to acquire vblank counter\n");
			list_del(&event->base.link);
			goto out;
		}

		crtc->primary->fb = fb;
		ret = nx_drm_crtc_update(crtc);
		if (ret) {
			crtc->primary->fb = old_fb;
			drm_vblank_put(drm, nr);
			list_del(&event->base.link);
			goto out;
		}
	}

	DRM_DEBUG_DRIVER("Exit.\n");
out:
	mutex_unlock(&drm->struct_mutex);
	return ret;
}

static void nx_drm_crtc_destroy(struct drm_crtc *crtc)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct nx_drm_private *private = crtc->dev->dev_private;
	int nr = nx_crtc->nr;

	DRM_DEBUG_KMS("Enter\n");

	private->crtc[nr] = NULL;

	drm_crtc_cleanup(crtc);
	kfree(nx_crtc);
}

static int nx_drm_crtc_set_config(struct drm_mode_set *set)
{
	DRM_DEBUG_KMS("Enter\n");
	return drm_crtc_helper_set_config(set);
}

static struct drm_crtc_funcs nx_crtc_funcs = {
	.set_config = nx_drm_crtc_set_config,
	.page_flip = nx_drm_crtc_page_flip,
	.destroy = nx_drm_crtc_destroy,
};

int nx_drm_crtc_enable_vblank(struct drm_device *drm, int crtc)
{
	struct nx_drm_private *private = drm->dev_private;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(private->crtc[crtc]);

	DRM_DEBUG_KMS("Enter\n");

	if (nx_crtc->dpms_mode != DRM_MODE_DPMS_ON)
		return -EPERM;

	DRM_DEBUG_DRIVER("Exit.\n");
	return 0;
}

void nx_drm_crtc_disable_vblank(struct drm_device *drm, int crtc)
{
	struct nx_drm_private *private = drm->dev_private;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(private->crtc[crtc]);

	DRM_DEBUG_KMS("Enter\n");

	if (nx_crtc->dpms_mode != DRM_MODE_DPMS_ON)
		return;
}

int nx_drm_crtc_init(struct drm_device *drm, int nr, int layer)
{
	struct nx_drm_crtc *nx_crtc;
	struct nx_drm_private *private = drm->dev_private;
	struct drm_crtc *crtc;
	struct drm_plane *primary;
	int ret = 0;

	DRM_DEBUG_KMS("Enter CRTC.%d\n", nr);

	nx_crtc = kzalloc(sizeof(*nx_crtc), GFP_KERNEL);
	if (!nx_crtc) {
		DRM_ERROR("Failed to allocate drm crtc.\n");
		return -ENOMEM;
	}

	nx_crtc->nr = nr;
	nx_crtc->dpms_mode = DRM_MODE_DPMS_OFF;
	crtc = &nx_crtc->drm_crtc;

	primary = nx_drm_plane_init(drm, (1 << nr),
						DRM_PLANE_TYPE_PRIMARY, layer);
	if (IS_ERR(primary)) {
		ret = PTR_ERR(primary);
		goto err_plane;
	}

	ret = drm_crtc_init_with_planes(drm,
				crtc, primary, NULL, &nx_crtc_funcs);

	if (0 > ret)
		goto err_crtc;

	private->crtc[nr] = crtc;
	drm_crtc_helper_add(crtc, &nx_crtc_helper_funcs);

	DRM_DEBUG_KMS("Exit, CRTC.%d ID:%d\n", nr, crtc->base.id);
	return 0;

err_crtc:
	primary->funcs->destroy(primary);
err_plane:
	kfree(nx_crtc);
	return ret;
}
