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
#include "s5pxx18_dp_hdmi.h"

#define	display_to_dpc(d)	(&d->ctrl.dpc)

static const char *const panel_type_name[] = {
	[dp_panel_type_none] = "unknown",
	[dp_panel_type_rgb]  = "RGB",
	[dp_panel_type_lvds] = "LVDS",
	[dp_panel_type_mipi] = "MIPI",
	[dp_panel_type_hdmi] = "HDMI",
};

enum dp_panel_type dp_panel_get_type(struct nx_drm_device *display)
{
	struct nx_drm_ctrl *ctrl = &display->ctrl;

	return ctrl->dpc.panel_type;
}

const char *dp_panel_type_name(enum dp_panel_type panel)
{
	return panel_type_name[panel];
}

static int convert_dp_rgb_format(uint32_t pixel_format,
			uint32_t bpp, uint32_t depth, uint32_t *format)
{
	uint32_t fmt;

	switch (pixel_format) {
	/* 16 bpp RGB */
	case DRM_FORMAT_RGB565:
		fmt = nx_mlc_rgbfmt_r5g6b5;
		break;
	case DRM_FORMAT_BGR565:
		fmt = nx_mlc_rgbfmt_b5g6r5;
		break;
	/* 24 bpp RGB */
	case DRM_FORMAT_RGB888:
		fmt = nx_mlc_rgbfmt_r8g8b8;
		break;
	case DRM_FORMAT_BGR888:
		fmt = nx_mlc_rgbfmt_b8g8r8;
		break;
	/* 32 bpp RGB */
	case DRM_FORMAT_XRGB8888:
		fmt = nx_mlc_rgbfmt_x8r8g8b8;
		break;
	case DRM_FORMAT_XBGR8888:
		fmt = nx_mlc_rgbfmt_x8b8g8r8;
		break;
	case DRM_FORMAT_ARGB8888:
		fmt = nx_mlc_rgbfmt_a8r8g8b8;
		break;
	case DRM_FORMAT_ABGR8888:
		fmt = nx_mlc_rgbfmt_a8b8g8r8;
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

#define parse_read_prop(n, s, v)	{ \
	u32 _v;	\
	if (!of_property_read_u32(n, s, &_v))	\
		v = _v;	\
	}

int nx_drm_dp_panel_drv_res_parse(struct device *dev,
			void **base, struct reset_control **reset)
{
	struct device_node *node = dev->of_node;
	const char *string;
	int err;

	*base = of_iomap(node, 0);
	if (!*base) {
		DRM_ERROR("fail : %s of_iomap\n", dev_name(dev));
		return -EINVAL;
	}

	DRM_DEBUG_KMS("base  : 0x%lx\n", (unsigned long)*base);

	err = of_property_read_string(node, "reset-names", &string);
	if (!err) {
		*reset = devm_reset_control_get(dev, string);
		if (*reset) {
			bool stat = reset_control_status(*reset);

			if (stat)
				reset_control_reset(*reset);
		}
		DRM_DEBUG_KMS("reset : %s\n", string);
	}

	nx_soc_dp_device_top_base(0, *base);

	return 0;
}

void nx_drm_dp_panel_drv_res_free(struct device *dev,
			void *base, struct reset_control *reset)
{
	if (base)
		iounmap(base);
}

int nx_drm_dp_panel_dev_res_parse(struct device *dev,
			struct device_node *node, struct nx_drm_res *res,
			enum dp_panel_type panel_type)
{
	struct device_node *np;
	const __be32 *list;
	const char *strings[8];
	u32 addr;
	int size, i = 0;
	bool reset;
	int clks_len = ARRAY_SIZE(res->clk_ids);
	int tieoffs_len = ARRAY_SIZE(res->tieoffs);
	int resets_len = ARRAY_SIZE(res->resets);

	np = of_find_node_by_name(node, "dp-resource");
	if (!np) {
		DRM_INFO("%s no output resource[%s] ...\n",
			dp_panel_type_name(panel_type), node->full_name);
		return 0;
	}

	/*
	 * register base
	 */
	if (!of_property_read_u32(np, "reg_base", &addr)) {
		list = of_get_property(np, "reg_base", &size);
		addr = be32_to_cpu(*list++);
		size = PAGE_ALIGN(be32_to_cpu(*list++));

		res->vir_base = ioremap(addr, size);
		if (!res->vir_base)
			return -EINVAL;

		DRM_DEBUG_KMS("base  :  0x%x (0x%x) %p\n",
			addr, size, res->vir_base);
	}

	/*
	 * clock gen base : 2 contents
	 */
	list = of_get_property(np, "clk_base", &size);
	size /= 8;
	if (size > clks_len) {
		DRM_ERROR("error: over clks dt size %d (%d)\n",
			size, clks_len);
		return -EINVAL;
	}

	for (i = 0; size > i; i++) {
		addr = be32_to_cpu(*list++);
		res->clk_bases[i] = ioremap(addr, PAGE_SIZE);
		res->clk_ids[i] = be32_to_cpu(*list++);
		DRM_DEBUG_KMS("clock : [%d] clk 0x%x, %d\n",
			i, addr, res->clk_ids[i]);
	}
	res->num_clks = size;

	/*
	 * tieoffs : 2 contents
	 */
	list = of_get_property(np, "soc,tieoff", &size);
	size /= 8;
	if (size > tieoffs_len) {
		DRM_ERROR("error: over tieoff dt size %d (%d)\n",
			size, tieoffs_len);
		return -EINVAL;
	}
	res->num_tieoffs = size;

	for (i = 0; size > i; i++) {
		res->tieoffs[i][0] = be32_to_cpu(*list++);
		res->tieoffs[i][1] = be32_to_cpu(*list++);
		DRM_DEBUG_KMS("tieoff: [%d] res->tieoffs <0x%x %d>\n",
			i, res->tieoffs[i][0], res->tieoffs[i][1]);
	}

	for (i = 0; size > i; i += 2)
		nx_tieoff_set(res->tieoffs[i][0], res->tieoffs[i][1]);

	/*
	 * resets
	 */
	size = of_property_read_string_array(np,
			"reset-names", strings, resets_len);
	for (i = 0; size > i; i++) {
		res->resets[i] = of_reset_control_get(np, strings[i]);
		DRM_DEBUG_KMS("reset : [%d] %s\n", i, strings[i]);
	}
	res->num_resets = size;

	for (i = 0; size > i; i++) {
		reset = reset_control_status(res->resets[i]);
		if (reset)
			reset_control_assert(res->resets[i]);
	}

	for (i = 0; size > i; i++)
		reset_control_deassert(res->resets[i]);

	/*
	 * set base
	 */
	for (i = 0; res->num_clks > i; i++)
		nx_soc_dp_device_clk_base(
				res->clk_ids[i], res->clk_bases[i]);

	return 0;
}

void nx_drm_dp_panel_dev_res_free(struct device *dev,
			struct nx_drm_res *res)
{
	int i, size;

	if (res->vir_base)
		iounmap(res->vir_base);

	size = res->num_clks;

	for (i = 0; size > i; i++) {
		if (res->clk_bases[i])
			iounmap(res->clk_bases[i]);
	}
}

int nx_drm_dp_panel_dev_register(struct device *dev,
			struct device_node *np, enum dp_panel_type type,
			struct nx_drm_device *display)
{
	struct dp_control_dev *dpc = display_to_dpc(display);
	struct nx_drm_res *res = &display->res;
	int err = -EINVAL;

	if (dp_panel_type_rgb == type) {

		#ifdef CONFIG_DRM_NX_RGB
		err = nx_soc_dp_rgb_register(dev, np, dpc);
		#endif

	} else if (dp_panel_type_lvds == type) {

		#ifdef CONFIG_DRM_NX_LVDS
		err = nx_soc_dp_lvds_register(dev, np, dpc,
					(void *)res->resets, res->num_resets);
		#endif

	} else if (dp_panel_type_mipi == type) {

		#ifdef CONFIG_DRM_NX_MIPI_DSI
		err = nx_soc_dp_mipi_register(dev, np, dpc);
		#endif

	} else if (dp_panel_type_hdmi == type) {

		#ifdef CONFIG_DRM_NX_HDMI
		err = nx_soc_dp_hdmi_register(dev, np, dpc);
		#endif

	} else {
		DRM_ERROR("not support panel type [%d] !!!\n", type);
		return -EINVAL;
	}

	if (0 > err) {
		DRM_ERROR("not selected panel [%s] !!!\n",
			dp_panel_type_name(type));
		return err;
	}

	if (dpc->ops && dpc->ops->set_base)
		dpc->ops->set_base(dpc, res->vir_base);

	return 0;
}

void nx_drm_dp_panel_dev_release(struct device *dev,
			struct nx_drm_device *display)
{
	struct dp_control_dev *dpc = display_to_dpc(display);

	if (dpc->dp_output)
		devm_kfree(dev, dpc->dp_output);
}

int nx_drm_dp_panel_ctrl_parse(struct device_node *np,
			struct nx_drm_device *display)
{
	struct dp_control_dev *dpc = display_to_dpc(display);
	struct dp_ctrl_info *ctl = &dpc->ctrl;

	parse_read_prop(np, "clk_src_lv0", ctl->clk_src_lv0);
	parse_read_prop(np, "clk_src_lv0", ctl->clk_src_lv0);
	parse_read_prop(np, "clk_src_lv0", ctl->clk_src_lv0);
	parse_read_prop(np, "clk_div_lv0", ctl->clk_div_lv0);
	parse_read_prop(np, "clk_src_lv1", ctl->clk_src_lv1);
	parse_read_prop(np, "clk_div_lv1", ctl->clk_div_lv1);
	parse_read_prop(np, "out_format", ctl->out_format);
	parse_read_prop(np, "invert_field", ctl->invert_field);
	parse_read_prop(np, "swap_rb", ctl->swap_rb);
	parse_read_prop(np, "yc_order", ctl->yc_order);
	parse_read_prop(np, "delay_mask", ctl->delay_mask);
	parse_read_prop(np, "d_rgb_pvd", ctl->d_rgb_pvd);
	parse_read_prop(np, "d_hsync_cp1", ctl->d_hsync_cp1);
	parse_read_prop(np, "d_vsync_fram", ctl->d_vsync_fram);
	parse_read_prop(np, "d_de_cp2", ctl->d_de_cp2);
	parse_read_prop(np, "vs_start_offset", ctl->vs_start_offset);
	parse_read_prop(np, "vs_end_offset", ctl->vs_end_offset);
	parse_read_prop(np, "ev_start_offset", ctl->ev_start_offset);
	parse_read_prop(np, "ev_end_offset", ctl->ev_end_offset);
	parse_read_prop(np, "vck_select", ctl->vck_select);
	parse_read_prop(np, "clk_inv_lv0", ctl->clk_inv_lv0);
	parse_read_prop(np, "clk_delay_lv0", ctl->clk_delay_lv0);
	parse_read_prop(np, "clk_inv_lv1", ctl->clk_inv_lv1);
	parse_read_prop(np, "clk_delay_lv1", ctl->clk_delay_lv1);
	parse_read_prop(np, "clk_sel_div1", ctl->clk_sel_div1);

	return 0;
}

void nx_drm_dp_panel_ctrl_dump(struct nx_drm_device *display)
{
	struct nx_drm_panel *panel = &display->panel;
	struct dp_control_dev *dpc = display_to_dpc(display);
	struct dp_ctrl_info *ctrl = &dpc->ctrl;

	DRM_DEBUG_KMS("SYNC -> LCD %d x %d mm\n",
		panel->width_mm, panel->height_mm);
	DRM_DEBUG_KMS("ha:%d, hs:%d, hb:%d, hf:%d\n",
	    panel->vm.hactive, panel->vm.hsync_len,
	    panel->vm.hback_porch, panel->vm.hfront_porch);
	DRM_DEBUG_KMS("va:%d, vs:%d, vb:%d, vf:%d\n",
		panel->vm.vactive, panel->vm.vsync_len,
	    panel->vm.vback_porch, panel->vm.vfront_porch);
	DRM_DEBUG_KMS("flags:0x%x\n", panel->vm.flags);

	DRM_DEBUG_KMS("CTRL (%s)\n", dp_panel_type_name(dpc->panel_type));
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

int nx_drm_dp_crtc_drv_parse(struct platform_device *pdev, int pipe,
			int *irqno, struct reset_control **reset)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	const char *strings[10];
	void *base[8];
	int i, n, size = 0;
	int offset = 4;	/* mlc. or dpc. */
	int err;

	DRM_DEBUG_KMS("crtc.%d for %s\n", pipe, dev_name(dev));

	/*
	 * parse base address
	 */
	size = of_property_read_string_array(node,
			"reg-names", strings, ARRAY_SIZE(strings));
	if (size > ARRAY_SIZE(strings))
		return -EINVAL;

	for (n = 0, i = 0; size > i; i++) {
		const char *c = strings[i] + offset;
		unsigned long no;

		if (0 > kstrtoul(c, 0, &no))
			continue;

		if (pipe != no)
			continue;

		base[n] = of_iomap(node, i);
		if (!base[n]) {
			DRM_DEBUG_KMS("fail : %s iomap\n", strings[i]);
			return -EINVAL;
		}
		n++;
	}

	/*
	 * parse interrupts.
	 */
	size = of_property_read_string_array(node,
			"interrupts-names", strings, ARRAY_SIZE(strings));
	if (size > ARRAY_SIZE(strings))
		return -EINVAL;

	for (n = 0, i = 0; size > i; i++) {
		const char *c = strings[i] + offset;
		unsigned long no;

		if (0 > kstrtoul(c, 0, &no))
			continue;

		if (pipe != no)
			continue;

		err = platform_get_irq(pdev, i);
		if (0 > err)
			return -EINVAL;

		*irqno = err;
	}

	/*
	 * parse reset address
	 */
	size = of_property_read_string_array(node,
				"reset-names", strings, ARRAY_SIZE(strings));
	for (i = 0; size > i; i++) {
		*reset = devm_reset_control_get(dev, strings[i]);
		if (*reset) {
			bool stat = reset_control_status(*reset);

			if (stat)
				reset_control_reset(*reset);
		}
		DRM_DEBUG_KMS("reset[%d]: %s\n", i, strings[i]);
	}

	nx_soc_dp_device_dpc_base(pipe, base[0]);
	nx_soc_dp_device_mlc_base(pipe, base[1]);

	return 0;
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

	/* set top layer */
	nx_mlc_set_screen_size(module, top->width, top->height);

	/* set video color key */
	nx_soc_dp_rgb_set_color(layer,
		dp_color_transp, top->color_key, true, false);

	nx_soc_dp_rgb_set_address(layer, paddr, pixel, hstride, true);
	nx_soc_dp_rgb_set_enable(layer, true, true);
}

void nx_drm_dp_crtc_irq_on(struct drm_crtc *crtc, int pipe)
{
	struct dp_control_dev dpc = { .module = pipe };

	nx_soc_dp_device_irq_on(&dpc, true);
}

void nx_drm_dp_crtc_irq_off(struct drm_crtc *crtc, int pipe)
{
	struct dp_control_dev dpc = { .module = pipe };

	nx_soc_dp_device_irq_on(&dpc, false);
}

void nx_drm_dp_crtc_irq_done(struct drm_crtc *crtc, int pipe)
{
	struct dp_control_dev dpc = { .module = pipe };

	nx_soc_dp_device_irq_done(&dpc);
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

		nx_soc_dp_rgb_set_format(layer, format, pixel, true);
		nx_soc_dp_rgb_set_position(layer,
				src_x, src_y, src_w, src_h,
				crtc_x, crtc_y, crtc_w, crtc_h, true);
		nx_soc_dp_rgb_set_address(layer,
				paddrs[0], pixel, crtc_w * pixel, true);
		nx_soc_dp_rgb_set_enable(layer, true, true);

	/* update video plane */
	} else {
		dma_addr_t lua, cba, cra;
		int lus, cbs, crs;

		ret = convert_dp_vid_format(fb->pixel_format,
					&format);
		if (0 > ret)
			return ret;

		nx_soc_dp_video_set_format(layer, format, true);
		nx_soc_dp_video_set_position(layer,
				src_x, src_y, src_w, src_h,
				crtc_x, crtc_y, crtc_w, crtc_h, true);

		switch (num_planes) {
		case 1:
			lua = paddrs[0], lus = pitches[0];
			nx_soc_dp_video_set_address_1plane(layer,
				lua, lus, true);
			break;

		case 2:
		case 3:
			lua = paddrs[0];
			cba = offsets[1] ? lua + offsets[1] : paddrs[1];
			cra = offsets[2] ? lua + offsets[2] : paddrs[2];
			lus = pitches[0], cbs = pitches[1], crs = pitches[2];

			nx_soc_dp_video_set_address_3plane(layer, lua, lus,
				cba, cbs, cra, crs, true);
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
			struct nx_drm_device *display)
{
	struct videomode vm;
	struct dp_control_dev *dpc = display_to_dpc(display);
	struct dp_sync_info *sync = &dpc->sync;

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
	struct nx_drm_device *display = to_nx_encoder(encoder)->display;
	struct dp_control_dev *dpc = display_to_dpc(display);

	DRM_DEBUG_KMS("%s\n", dp_panel_type_name(dpc->panel_type));

	/*
	 * when set_crtc is requested from user or at booting time,
	 * encoder->commit would be called without dpms call so if dpms is
	 * no power on then encoder->dpms should be called
	 * with DRM_MODE_DPMS_ON for the hardware power to be on.
	 */
	nx_soc_dp_device_prepare(dpc);
	nx_soc_dp_device_power_on(dpc, true);
}

int nx_drm_dp_encoder_get_dpms(struct drm_encoder *encoder)
{
	struct nx_drm_device *display = to_nx_encoder(encoder)->display;
	struct dp_control_dev *dpc = display_to_dpc(display);
	bool poweron;

	poweron = nx_soc_dp_device_power_status(dpc);

	return poweron ? DRM_MODE_DPMS_ON : DRM_MODE_DPMS_OFF;
}

void nx_drm_dp_encoder_dpms(struct drm_encoder *encoder, bool poweron)
{
	struct nx_drm_device *display = to_nx_encoder(encoder)->display;
	struct dp_control_dev *dpc = display_to_dpc(display);

	DRM_DEBUG_KMS("%s power %s\n",
		dp_panel_type_name(dpc->panel_type), poweron ? "on" : "off");

	if (poweron)
		nx_soc_dp_device_prepare(dpc);

	nx_soc_dp_device_power_on(dpc, poweron);
}

void nx_drm_dp_encoder_prepare(struct drm_encoder *encoder,
			int index, bool irqon)
{
	struct nx_drm_device *display = to_nx_encoder(encoder)->display;
	struct dp_control_dev *dpc = display_to_dpc(display);

	/*
	 * set display module index
	 */
	dpc->module = index;

	nx_soc_dp_device_setup(dpc);
}

void nx_drm_dp_encoder_unprepare(struct drm_encoder *encoder)
{
	struct nx_drm_device *display = to_nx_encoder(encoder)->display;
	struct dp_control_dev *dpc = display_to_dpc(display);

	nx_soc_dp_device_irq_on(dpc, false);
}

int nx_drm_dp_lcd_prepare(struct nx_drm_device *display,
			struct drm_panel *panel)
{
	struct dp_control_dev *dpc = display_to_dpc(display);
	struct dp_control_ops *ops = dpc->ops;

	DRM_DEBUG_KMS("%s\n", dp_panel_type_name(dpc->panel_type));

	if (ops && ops->prepare)
		ops->prepare(dpc, panel ? 1 : 0);

	return 0;
}

int nx_drm_dp_lcd_enable(struct nx_drm_device *display,
				struct drm_panel *panel)
{
	struct dp_control_dev *dpc = display_to_dpc(display);
	struct dp_control_ops *ops = dpc->ops;
	int module = dpc->module;

	DRM_DEBUG_KMS("%s\n", dp_panel_type_name(dpc->panel_type));

	if (dp_panel_type_rgb == dpc->panel_type) {
		/*
		 *  0 : Primary MLC  , 1 : Primary MPU,
		 *  2 : Secondary MLC, 3 : ResConv(LCDIF)
		 */
		struct dp_rgb_dev *rgb = dpc->dp_output;
		int pin = 0;

		BUG_ON(!rgb);

		switch (module) {
		case 0:
			pin = rgb->mpu_lcd ? 1 : 0;
			break;
		case 1:
			pin = rgb->mpu_lcd ? 3 : 2;
			break;
		default:
			pr_err("fail : %s not support module %d\n",
				__func__, module);
			return -EINVAL;
		}

		nx_disp_top_set_primary_mux(pin);
		return 0;
	}

	if (ops && ops->prepare)
		ops->enable(dpc, panel ? 1 : 0);

	return 0;
}

int nx_drm_dp_lcd_unprepare(struct nx_drm_device *display,
				struct drm_panel *panel)
{
	struct dp_control_dev *dpc = display_to_dpc(display);
	struct dp_control_ops *ops = dpc->ops;

	DRM_DEBUG_KMS("%s\n", dp_panel_type_name(dpc->panel_type));

	if (ops && ops->unprepare)
		ops->unprepare(dpc);

	return 0;
}

int nx_drm_dp_lcd_disable(struct nx_drm_device *display,
				struct drm_panel *panel)
{
	struct dp_control_dev *dpc = display_to_dpc(display);
	struct dp_control_ops *ops = dpc->ops;

	DRM_DEBUG_KMS("%s\n", dp_panel_type_name(dpc->panel_type));

	if (ops && ops->disable)
		ops->disable(dpc);

	return 0;
}

#ifdef CONFIG_DRM_NX_MIPI_DSI
static void dp_mipi_dsi_dump_messages(const struct mipi_dsi_msg *msg, bool dump)
{
	const char *txb = msg->tx_buf;
	const char *rxb = msg->rx_buf;
	int i = 0;

	if (!dump)
		return;

	pr_info("%s\n", __func__);
	pr_info("ch   :%d\n", msg->channel);
	pr_info("type :0x%x\n", msg->type);
	pr_info("flags:0x%x\n", msg->flags);

	for (i = 0; msg->tx_len > i; i++)
		pr_info("T[%2d]: 0x%02x\n", i, txb[i]);

	for (i = 0; msg->rx_len > i; i++)
		pr_info("R[%2d]: 0x%02x\n", i, rxb[i]);
}

#define	IS_SHORT(t)	(9 > (t & 0x0f))

int nx_drm_dp_mipi_transfer(struct mipi_dsi_host *host,
			const struct mipi_dsi_msg *msg)
{
	struct dp_mipi_xfer xfer;
	int err;

	dp_mipi_dsi_dump_messages(msg, false);

	if (!msg->tx_len)
		return -EINVAL;

	/* set id */
	xfer.id = msg->type | (msg->channel << 6);

	/* short type msg */
	if (IS_SHORT(msg->type)) {
		const char *txb = msg->tx_buf;

		if (msg->tx_len > 2)
			return -EINVAL;

		xfer.tx_len  = 0;	/* no payload */
		xfer.data[0] = txb[0];
		xfer.data[1] = (msg->tx_len == 2) ? txb[1] : 0;

	} else {
		xfer.tx_len  = msg->tx_len;
		xfer.data[0] = msg->tx_len & 0xff;
		xfer.data[1] = msg->tx_len >> 8;
		xfer.tx_buf = msg->tx_buf;
	}

	xfer.rx_len = msg->rx_len;
	xfer.rx_buf = msg->rx_buf;
	xfer.flags = msg->flags;

	err = nx_soc_dp_mipi_tx_transfer(&xfer);

	if (xfer.rx_len)
		err = nx_soc_dp_mipi_rx_transfer(&xfer);

	return err;
}
#else
int nx_drm_dp_mipi_transfer(struct mipi_dsi_host *host,
			const struct mipi_dsi_msg *msg)
{
	return 0;
}
#endif
