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
#include <uapi/drm/drm_fourcc.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include "../nx_drm_drv.h"
#include "../nx_drm_crtc.h"
#include "../nx_drm_plane.h"
#include "../nx_drm_fb.h"
#include "../nx_drm_gem.h"
#include "../nx_drm_encoder.h"
#include "../nx_drm_connector.h"

#include "s5pxx18_drm_dp.h"

/*
 * refer to drm_fourcc.h
 */
static int convert_dp_rgb_format(uint32_t pixel_format,
			uint32_t bpp, uint32_t depth, uint32_t *format)
{
	uint32_t fmt;

	switch (pixel_format) {
	/* 16 bpp RGB */
	case DRM_FORMAT_RGB565:
		fmt = MLC_RGBFMT_R5G6B5;
		break;
	case DRM_FORMAT_BGR565:
		fmt = MLC_RGBFMT_B5G6R5;
		break;
	/* 24 bpp RGB */
	case DRM_FORMAT_RGB888:
		fmt = MLC_RGBFMT_R8G8B8;
		break;
	case DRM_FORMAT_BGR888:
		fmt = MLC_RGBFMT_B8G8R8;
		break;
	/* 32 bpp RGB */
	case DRM_FORMAT_XRGB8888:
		fmt = MLC_RGBFMT_X8R8G8B8;
		break;
	case DRM_FORMAT_XBGR8888:
		fmt = MLC_RGBFMT_X8B8G8R8;
		break;
	case DRM_FORMAT_ARGB8888:
		fmt = MLC_RGBFMT_A8R8G8B8;
		break;
	case DRM_FORMAT_ABGR8888:
		fmt = MLC_RGBFMT_A8B8G8R8;
		break;
	default:
		DRM_ERROR("fail : not support %s pixel format\n",
			drm_get_format_name(pixel_format));
		return -EINVAL;
	}

	*format = fmt;
	return 0;
}

static uint32_t convert_dp_vid_format(uint32_t fourcc,
			uint32_t *format)
{
	uint32_t fmt;

	switch (fourcc) {
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU411:
		fmt = nx_mlc_yuvfmt_420;
		break;
	case DRM_FORMAT_YUV422:
		fmt = nx_mlc_yuvfmt_422;
		break;
	case DRM_FORMAT_YUV444:
		fmt = nx_mlc_yuvfmt_444;
		break;
	case DRM_FORMAT_YUYV:
		fmt = nx_mlc_yuvfmt_yuyv;
		break;
	default:
		DRM_ERROR("fail : fail, not support fourcc %s\n",
		       drm_get_format_name(fourcc));
		return -EINVAL;
	}

	*format = fmt;
	return 0;
}

int nx_drm_driver_parse_dt_setup(struct drm_device *drm,
			struct nx_drm_priv *priv)
{
	struct device *dev;
	struct device_node *np;
	struct reset_control *rsc;
	const char *strings[4];
	void *base[5];
	int i, counts;
	int ret = 0;

	BUG_ON(!drm);
	dev = &drm->platformdev->dev;
	np = dev->of_node;

	/* parse base address */
	for (i = 0; ARRAY_SIZE(base) > i; i++) {
		base[i] = of_iomap(np, i);
		if (!base[i])
			return -EINVAL;
		DRM_DEBUG_KMS("vbase[%d]: %p\n", i, base[i]);
	}

	nx_soc_dp_device_set_base(&base[0], 2);
	nx_soc_dp_plane_top_set_base(&base[2], 2);
	nx_soc_dp_device_top_base(&base[4], 1);

	/* parse soc reset */
	counts = of_property_read_string_array(np, "reset-names", strings, 4);
	for (i = 0; counts > i; i++) {
		rsc = devm_reset_control_get(dev, strings[i]);
		priv->reset_ctrl[i] = rsc;
		DRM_DEBUG_KMS("reset[%d]: %s\n", i, strings[i]);
	}

	priv->num_resets = counts;
	nx_drm_dp_device_reset(drm, priv);

	/* parse interrupts */
	for (i = 0; 2 > i; i++) {
		ret = platform_get_irq(drm->platformdev, i);
		if (0 > ret)
			return -EINVAL;
		priv->hw_irq_no[i] = ret;
		priv->num_irqs++;
		DRM_DEBUG_KMS("irqno[%d]: %d\n", i, ret);
	}

	return 0;
}

#define parse_read_prop(n, s, v)	{ \
	u32 _v;	\
	if (!of_property_read_u32(n, s, &_v))	\
		v = _v;	\
	}

int nx_drm_dp_parse_dt_panel(struct device_node *np,
			struct dp_device_info *ddi)
{
	const char *name;

	BUG_ON(!ddi);

	if (of_property_read_string(np, "panel-type", &name)) {
		DRM_ERROR("not defined 'panel-type' !!!\n");
		DRM_ERROR("'panel-type' = 'rgb, lvds, mipi, hdmi, vivi'\n");
		return -EINVAL;
	}

	if (!strcmp("rgb", name)) {
		ddi->panel_type = do_panel_type_lcd;
	} else if (!strcmp("lvds", name)) {
		ddi->panel_type = do_panel_type_lvds;
	} else if (!strcmp("mipi", name)) {
		ddi->panel_type = do_panel_type_mipi;
	} else if (!strcmp("hdmi", name)) {
		ddi->panel_type = do_panel_type_hdmi;
	} else if (!strcmp("vivi", name)) {
		ddi->panel_type = do_panel_type_vidi;
	} else {
		DRM_ERROR("%s not support 'panel-type' !!!\n", name);
		DRM_ERROR("'panel-type' = 'rgb, lvds, mipi, hdmi, vivi'\n");
		return -EINVAL;
	}
	return 0;
}

int nx_drm_dp_parse_dt_ctrl(struct device_node *np,
			struct dp_device_info *ddi)
{
	struct dp_ctrl_info *ctrl;

	BUG_ON(!np || !ddi);

	ctrl = &ddi->ctrl;

	parse_read_prop(np, "lcd-rgb", ddi->panel_type);
	parse_read_prop(np, "clk_src_lv0", ctrl->clk_src_lv0);
	parse_read_prop(np, "clk_src_lv0", ctrl->clk_src_lv0);
	parse_read_prop(np, "clk_src_lv0", ctrl->clk_src_lv0);
	parse_read_prop(np, "clk_div_lv0", ctrl->clk_div_lv0);
	parse_read_prop(np, "clk_src_lv1", ctrl->clk_src_lv1);
	parse_read_prop(np, "clk_div_lv1", ctrl->clk_div_lv1);
	parse_read_prop(np, "out_format", ctrl->out_format);
	parse_read_prop(np, "invert_field", ctrl->invert_field);
	parse_read_prop(np, "swap_rb", ctrl->swap_rb);
	parse_read_prop(np, "yc_order", ctrl->yc_order);
	parse_read_prop(np, "delay_mask", ctrl->delay_mask);
	parse_read_prop(np, "d_rgb_pvd", ctrl->d_rgb_pvd);
	parse_read_prop(np, "d_hsync_cp1", ctrl->d_hsync_cp1);
	parse_read_prop(np, "d_vsync_fram", ctrl->d_vsync_fram);
	parse_read_prop(np, "d_de_cp2", ctrl->d_de_cp2);
	parse_read_prop(np, "vs_start_offset", ctrl->vs_start_offset);
	parse_read_prop(np, "vs_end_offset", ctrl->vs_end_offset);
	parse_read_prop(np, "ev_start_offset", ctrl->ev_start_offset);
	parse_read_prop(np, "ev_end_offset", ctrl->ev_end_offset);
	parse_read_prop(np, "vck_select", ctrl->vck_select);
	parse_read_prop(np, "clk_inv_lv0", ctrl->clk_inv_lv0);
	parse_read_prop(np, "clk_delay_lv0", ctrl->clk_delay_lv0);
	parse_read_prop(np, "clk_inv_lv1", ctrl->clk_inv_lv1);
	parse_read_prop(np, "clk_delay_lv1", ctrl->clk_delay_lv1);
	parse_read_prop(np, "clk_sel_div1", ctrl->clk_sel_div1);

	return 0;
}

void nx_drm_dp_parse_dp_dump(struct nx_drm_dp_dev *dp_dev)
{
	struct nx_drm_panel *dpp = &dp_dev->dpp;
	struct dp_device_info *ddi = &dp_dev->ddi;
	struct dp_ctrl_info *ctrl = &ddi->ctrl;

	DRM_DEBUG_KMS("SYNC -> LCD %d x %d mm\n",
		dpp->width_mm, dpp->height_mm);
	DRM_DEBUG_KMS("ha:%d, hs:%d, hb:%d, hf:%d\n",
	    dpp->vm.hactive, dpp->vm.hsync_len,
	    dpp->vm.hback_porch, dpp->vm.hfront_porch);
	DRM_DEBUG_KMS("va:%d, vs:%d, vb:%d, vf:%d\n",
		dpp->vm.vactive, dpp->vm.vsync_len,
	    dpp->vm.vback_porch, dpp->vm.vfront_porch);
	DRM_DEBUG_KMS("flags:0x%x\n", dpp->vm.flags);

	DRM_DEBUG_KMS("CTRL (%s)\n",
		ddi->panel_type ? "RGB" : "");
	DRM_DEBUG_KMS("cs0:%d, cd0:%d, cs1:%d, cd1:%d\n",
	    ctrl->clk_src_lv0, ctrl->clk_div_lv0,
	    ctrl->clk_src_lv1, ctrl->clk_div_lv1);
	DRM_DEBUG_KMS("fmt:0x%x, inv:%d, swap:%d, yb:0x%x\n",
	    ctrl->out_format, ctrl->invert_field,
	    ctrl->swap_rb, ctrl->yc_order);
	DRM_DEBUG_KMS("dm:0x%x, drp:%d, dhs:%d, dvs:%d, dde:0x%x\n",
	    ctrl->delay_mask, ctrl->d_rgb_pvd,
	    ctrl->d_hsync_cp1, ctrl->d_vsync_fram, ctrl->d_de_cp2);
	DRM_DEBUG_KMS("vss:%d, vse:%d, evs:%d, eve:%d\n",
	    ctrl->vs_start_offset, ctrl->vs_end_offset,
	    ctrl->ev_start_offset, ctrl->ev_end_offset);
	DRM_DEBUG_KMS("sel:%d, i0:%d, d0:%d, i1:%d, d1:%d, s1:%d\n",
	    ctrl->vck_select, ctrl->clk_inv_lv0, ctrl->clk_delay_lv0,
	    ctrl->clk_inv_lv1, ctrl->clk_delay_lv1, ctrl->clk_sel_div1);
}

void nx_drm_dp_crtc_dpms(struct drm_crtc *crtc, int mode)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct dp_plane_top *top = &nx_crtc->top;
	int on = (mode == DRM_MODE_DPMS_ON ? 1 : 0);

	DRM_DEBUG_KMS("crtc.%d mode: %s\n",
		top->module, mode == DRM_MODE_DPMS_ON ? "on" :
		mode == DRM_MODE_DPMS_OFF ? "off" :
		mode == DRM_MODE_DPMS_STANDBY ? "standby" :
		mode == DRM_MODE_DPMS_SUSPEND ? "suspend" : "unknown");

	nx_soc_dp_plane_top_set_enable(top, on);
}

int nx_drm_dp_crtc_mode_set(struct drm_crtc *crtc,
			struct drm_plane *plane,
			struct drm_framebuffer *fb,
			int crtc_x, int crtc_y,
			unsigned int crtc_w, unsigned int crtc_h,
			uint32_t src_x, uint32_t src_y,
			uint32_t src_w, uint32_t src_h)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);
	struct dp_plane_top *top = &nx_crtc->top;
	struct dp_plane_layer *layer = &nx_plane->layer;
	int pixel = fb->bits_per_pixel >> 3;
	unsigned int format;
	int ret;

	ret = convert_dp_rgb_format(fb->pixel_format,
			fb->bits_per_pixel, fb->depth, &format);
	if (0 > ret)
		return ret;

	DRM_DEBUG_KMS("crtc.%d plane.%d :\n",
			top->module, layer->num);
	DRM_DEBUG_KMS("crtc[%d x %d] src[%d,%d,%d,%d]\n",
			crtc_w, crtc_h, src_x, src_y, src_w, src_h);
	DRM_DEBUG_KMS("%s -> 0x%x, pixel:%d, back:0x%x\n",
			drm_get_format_name(fb->pixel_format), format,
			pixel, top->back_color);

	nx_soc_dp_plane_top_set_bg_color(top);
	nx_soc_dp_plane_top_set_format(top, crtc_w, crtc_h);
	nx_soc_dp_rgb_set_format(layer, format, pixel, true);
	nx_soc_dp_rgb_set_position(layer, src_x, src_y, src_w, src_h,
			crtc_x, crtc_y, crtc_w, crtc_h, false);

	return 0;
}

void nx_drm_dp_crtc_commit(struct drm_crtc *crtc)
{
	struct drm_framebuffer *fb = crtc->primary->fb;
	struct drm_gem_cma_object *cma_obj;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct nx_drm_plane *nx_plane = to_nx_plane(crtc->primary);
	struct dp_plane_top *top = &nx_crtc->top;
	struct dp_plane_layer *layer = &nx_plane->layer;
	dma_addr_t paddr;

	int module = top->module;
	int num = layer->num;
	int pixel = fb->bits_per_pixel >> 3;
	int width = fb->width;
	int height = fb->height;
	int hstride = width * pixel;

	cma_obj = drm_fb_cma_get_gem_obj(fb, 0);
	paddr = cma_obj->paddr;

	DRM_DEBUG_KMS("crtc.%d plane.%d (%s) :\n",
			module, num, nx_plane->layer.name);
	DRM_DEBUG_KMS("crtc[%d x %d] addr:0x%x\n",
			width, height, (unsigned int)paddr);

	/* set video color key */
	nx_soc_dp_rgb_set_color(layer,
		dp_color_transp, top->color_key, true, false);

	nx_soc_dp_rgb_set_address(layer, paddr, pixel, hstride, false);
	nx_soc_dp_rgb_set_enable(layer, true, true);
}

void nx_drm_dp_crtc_init(struct drm_device *drm,
			struct drm_crtc *crtc, int index)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct dp_plane_top *top = &nx_crtc->top;

	top->module = index;
	top->dev = drm->dev;
	INIT_LIST_HEAD(&top->plane_list);

	nx_soc_dp_plane_top_setup(top);
}

void nx_drm_dp_plane_set_color(struct drm_plane *plane,
			enum dp_color_type type, unsigned int color)
{
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);
	struct dp_plane_layer *layer = &nx_plane->layer;
	int i = 0;

	if (dp_color_colorkey == type) {
		struct dp_plane_top *top = layer->plane_top;

		list_for_each_entry(layer, &top->plane_list, list) {
			if (i == top->primary_plane)
				break;
			i++;
		}
		nx_soc_dp_rgb_set_color(layer,
			dp_color_transp, color, true, true);
	} else {
		if (dp_plane_video != layer->type)
			nx_soc_dp_rgb_set_color(layer, type, color, true, true);
	}
}

int nx_drm_dp_plane_update(struct drm_plane *plane,
			struct drm_framebuffer *fb,
			int crtc_x, int crtc_y,
			unsigned int crtc_w, unsigned int crtc_h,
			uint32_t src_x, uint32_t src_y,
			uint32_t src_w, uint32_t src_h)
{
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);
	struct dp_plane_layer *layer = &nx_plane->layer;
	struct drm_gem_cma_object *cma_obj[4];
	dma_addr_t paddrs[4];
	unsigned int pitches[4], offsets[4];
	enum dp_plane_type type;
	int num_planes = 0;
	unsigned int format;
	int i = 0;
	int ret;

	type = layer->type;
	num_planes = drm_format_num_planes(fb->pixel_format);

	DRM_DEBUG_KMS("crtc.%d plane.%d (%s) : planes %d\n",
		layer->module, layer->num, layer->name, num_planes);
	DRM_DEBUG_KMS("%s crtc[%d,%d,%d,%d] src[%d,%d,%d,%d]\n",
		drm_get_format_name(fb->pixel_format),
		crtc_x, crtc_y, crtc_w, crtc_h, src_x, src_y, src_w, src_h);

	for (i = 0; num_planes > i; i++) {
		cma_obj[i] = drm_fb_cma_get_gem_obj(fb, i);
		paddrs[i] = cma_obj[i]->paddr;
		offsets[i] = fb->offsets[i];
		pitches[i] = fb->pitches[i];
	}

	/* update rgb plane */
	if (dp_plane_rgb == type) {
		int pixel = (fb->bits_per_pixel >> 3);

		ret = convert_dp_rgb_format(fb->pixel_format,
				fb->bits_per_pixel, fb->depth, &format);
		if (0 > ret)
			return ret;

		nx_soc_dp_rgb_set_format(layer, format, pixel, false);
		nx_soc_dp_rgb_set_position(layer,
				src_x, src_y, src_w, src_h,
				crtc_x, crtc_y, crtc_w, crtc_h, false);
		nx_soc_dp_rgb_set_address(layer,
				paddrs[0], pixel, crtc_w * pixel, false);
		nx_soc_dp_rgb_set_enable(layer, true, true);

	/* update video plane */
	} else {
		dma_addr_t lua, cba, cra;
		int lus, cbs, crs;

		ret = convert_dp_vid_format(fb->pixel_format,
					&format);
		if (0 > ret)
			return ret;

		nx_soc_dp_video_set_format(layer, format, false);
		nx_soc_dp_video_set_position(layer,
				src_x, src_y, src_w, src_h,
				crtc_x, crtc_y, crtc_w, crtc_h, false);

		switch (num_planes) {
		case 1:
			lua = paddrs[0], lus = pitches[0];
			nx_soc_dp_video_set_address_1plane(layer,
				lua, lus, false);
			break;

		case 2:
		case 3:
			lua = paddrs[0];
			cba = offsets[1] ? lua + offsets[1] : paddrs[1];
			cra = offsets[2] ? lua + offsets[2] : paddrs[2];
			lus = pitches[0], cbs = pitches[1], crs = pitches[2];

			nx_soc_dp_video_set_address_3plane(layer, lua, lus,
				cba, cbs, cra, crs, false);
			break;
		default:
			ret = -EINVAL;
			break;
		}

		if (0 > ret)
			return ret;

		nx_soc_dp_video_set_enable(layer, true, true);
	}
	return 0;
}

void nx_drm_dp_plane_disable(struct drm_plane *plane)
{
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);
	struct dp_plane_layer *layer = &nx_plane->layer;

	if (dp_plane_rgb == layer->type)
		nx_soc_dp_rgb_set_enable(layer, false, true);
	else
		nx_soc_dp_video_set_enable(layer, false, true);
}

void nx_drm_dp_plane_init(struct drm_device *drm,
			struct drm_crtc *crtc,
			struct drm_plane *plane, int plane_num)
{
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);
	struct dp_plane_layer *layer = &nx_plane->layer;
	struct dp_plane_top *top;
	enum dp_plane_type type;
	int module;

	top = &(to_nx_crtc(crtc))->top;
	module = top->module;
	type = plane_num == PLANE_VIDEO_NUM ?
				dp_plane_video : dp_plane_rgb;

	layer->dev = drm->dev;
	layer->plane_top = top;
	layer->module = top->module;
	layer->num = plane_num;
	layer->type = type;

	sprintf(layer->name, "%d-%s%d",
		module, dp_plane_video == type ? "vid" : "rgb", plane_num);

	if (dp_plane_video == type) {
		layer->color.alpha = 15;
		layer->color.bright = 0;
		layer->color.contrast = 0;
		layer->color.satura = 0;
	}

	list_add_tail(&layer->list, &top->plane_list);

	DRM_DEBUG_KMS("crtc.%d plane.%d (%s)\n",
		layer->module, layer->num, layer->name);
}

void nx_drm_dp_display_mode_to_sync(struct drm_display_mode *mode,
			struct dp_sync_info *sync)
{
	struct videomode vm;

	drm_display_mode_to_videomode(mode, &vm);

	sync->interlace =
		vm.flags | DISPLAY_FLAGS_INTERLACED ? 1 : 0;

	sync->h_active_len = vm.hactive;
	sync->h_sync_width = vm.hsync_len;
	sync->h_back_porch = vm.hback_porch;
	sync->h_front_porch = vm.hfront_porch;
	sync->h_sync_invert =
		vm.flags | DISPLAY_FLAGS_HSYNC_HIGH ? 1 : 0;

	sync->v_active_len = vm.vactive;
	sync->v_sync_width = vm.vsync_len;
	sync->v_back_porch = vm.vback_porch;
	sync->v_front_porch = vm.vfront_porch;
	sync->v_sync_invert =
		vm.flags | DISPLAY_FLAGS_VSYNC_HIGH ? 1 : 0;
	sync->pixel_clock_hz = vm.pixelclock;
}

void nx_drm_dp_encoder_commit(struct drm_encoder *encoder)
{
	struct nx_drm_dp_dev *dp_dev = to_nx_encoder(encoder)->dp_dev;
	struct dp_device_info *ddi = &dp_dev->ddi;

	nx_soc_dp_device_prepare(ddi);
}

int nx_drm_dp_encoder_get_dpms(struct drm_encoder *encoder)
{
	struct nx_drm_dp_dev *dp_dev = to_nx_encoder(encoder)->dp_dev;
	struct dp_device_info *ddi = &dp_dev->ddi;
	bool poweron;

	poweron = nx_soc_dp_device_power_status(ddi);

	return poweron ? DRM_MODE_DPMS_ON : DRM_MODE_DPMS_OFF;
}

void nx_drm_dp_encoder_dpms(struct drm_encoder *encoder, bool poweron)
{
	struct nx_drm_dp_dev *dp_dev = to_nx_encoder(encoder)->dp_dev;
	struct dp_device_info *ddi = &dp_dev->ddi;

	nx_soc_dp_device_power_on(ddi, poweron);
}

void nx_drm_dp_encoder_init(struct drm_encoder *encoder,
			int index, bool irqon)
{
	struct nx_drm_dp_dev *dp_dev = to_nx_encoder(encoder)->dp_dev;
	struct dp_device_info *ddi = &dp_dev->ddi;

	/*
	 * set display module index
	 */
	ddi->module = index;

	nx_soc_dp_device_setup(ddi);
	nx_soc_dp_device_irq_on(ddi, irqon);
}

int nx_drm_dp_device_reset(struct drm_device *drm,
			struct nx_drm_priv *priv)
{
	int nums = priv->num_resets;
	int i;

	DRM_DEBUG_KMS("enter\n");

	for (i = 0; nums > i; i++) {
		struct reset_control *rsc = priv->reset_ctrl[i];

		if (rsc) {
			bool reset = reset_control_status(rsc);

			if (reset)
				reset_control_reset(rsc);
		}
	}

	DRM_DEBUG_KMS("done\n");
	return 0;
}

