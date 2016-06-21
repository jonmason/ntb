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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/reset.h>

#include "s5pxx18_dp_dev.h"

#define	WAIT_VBLANK(m, n, o)
#define	LAYER_VIDEO		PLANE_VIDEO_NUM

/* 12345'678'[8] -> 12345 [5], 123456'78'[8] -> 123456[6] */
static inline u_short R8G8B8toR5G6B5(unsigned int RGB)
{
	u8 R = (u8) ((RGB >> 16) & 0xff);
	u8 G = (u8) ((RGB >> 8) & 0xff);
	u8 B = (u8) ((RGB >> 0) & 0xff);
	u_short R5G6B5 =
		((R & 0xF8) << 8) | ((G & 0xFC) << 3) | ((B & 0xF8) >> 3);

	return R5G6B5;
}

/* 12345 [5] -> 12345'123'[8], 123456[6] -> 123456'12'[8] */
static inline unsigned int R5G6B5toR8G8B8(u_short RGB)
{
	u8 R5 = (RGB >> 11) & 0x1f;
	u8 G6 = (RGB >> 5) & 0x3f;
	u8 B5 = (RGB >> 0) & 0x1f;
	u8 R8 = ((R5 << 3) & 0xf8) | ((R5 >> 2) & 0x7);
	u8 G8 = ((G6 << 2) & 0xfc) | ((G6 >> 4) & 0x3);
	u8 B8 = ((B5 << 3) & 0xf8) | ((B5 >> 2) & 0x7);
	unsigned int R8B8G8 = (R8 << 16) | (G8 << 8) | (B8);

	return R8B8G8;
}

/* 123'45678'[8] -> 123[3], 12'345678'[8] -> 12 [2] */
static inline u8 R8G8B8toR3G3B2(unsigned int RGB)
{
	u8 R = (u8) ((RGB >> 16) & 0xff);
	u8 G = (u8) ((RGB >> 8) & 0xff);
	u8 B = (u8) ((RGB >> 0) & 0xff);
	u8 R3G3B2 = ((R & 0xE0) | ((G & 0xE0) >> 3) | ((B & 0xC0) >> 6));

	return R3G3B2;
}

/* 123[3] -> 123'123'12' [8], 12 [2] -> 12'12'12'12'[8] */
static inline unsigned int R3G3B2toR8G8B8(u8 RGB)
{
	u8 R3 = (RGB >> 5) & 0x7;
	u8 G3 = (RGB >> 2) & 0x7;
	u8 B2 = (RGB >> 0) & 0x3;
	u8 R8 = ((R3 << 5) | (R3 << 2) | (R3 >> 1));
	u8 G8 = ((G3 << 5) | (G3 << 2) | (G3 >> 1));
	u8 B8 = ((B2 << 6) | (B2 << 4) | (B2 << 2) | B2);
	unsigned int R8B8G8 = (R8 << 16) | (G8 << 8) | (B8);

	return R8B8G8;
}

static inline void dp_wait_vblank_done(int module, int layer)
{
	bool on = nx_mlc_get_layer_enable(module, layer);
	int count = 20000;

	while (on) {
		bool dflag = nx_mlc_get_dirty_flag(module, layer);

		if (0 > --count || !dflag)
			break;
	}
}

static inline void dp_plane_adjust(int module, int layer, bool now)
{
	if (now)
		nx_mlc_set_dirty_flag(module, layer);
}

void nx_soc_dp_device_dpc_base(int module, void __iomem *base)
{
	BUG_ON(!base);
	pr_debug("%s: dev.%d\n", __func__, module);

	nx_dpc_set_base_address(module, base);
}

void nx_soc_dp_device_mlc_base(int module, void __iomem *base)
{
	BUG_ON(!base);
	pr_debug("%s: crtc.%d\n", __func__, module);

	nx_mlc_set_base_address(module, base);
}

void nx_soc_dp_device_top_base(int module, void __iomem *base)
{
	BUG_ON(!base);
	pr_debug("%s: dev top\n", __func__);

	nx_disp_top_set_base_address(base);
}

void nx_soc_dp_device_clk_base(int id, void __iomem *base)
{
	BUG_ON(!base);
	pr_debug("%s: dev clk %d\n", __func__, id);

	nx_disp_top_clkgen_set_base_address(id, base);
	nx_disp_top_clkgen_set_clock_pclk_mode(id, nx_pclkmode_always);
}

void nx_soc_dp_plane_top_setup(struct dp_plane_top *top)
{
	int module = top->module;

	pr_debug("%s: crtc.%d\n", __func__, module);
	nx_mlc_set_clock_pclk_mode(module, nx_pclkmode_always);
	nx_mlc_set_clock_bclk_mode(module, nx_bclkmode_always);
}

void nx_soc_dp_plane_top_set_format(struct dp_plane_top *top,
			int width, int height)
{
	int module = top->module;
	enum nx_mlc_priority priority;
	int prior = top->video_prior;
	unsigned int bgcolor = top->back_color;

	switch (prior) {
	case 0:
		priority = nx_mlc_priority_videofirst;
		break;	/* PRIORITY-video>0>1>2 */
	case 1:
		priority = nx_mlc_priority_videosecond;
		break;	/* PRIORITY-0>video>1>2 */
	case 2:
		priority = nx_mlc_priority_videothird;
		break;	/* PRIORITY-0>1>video>2 */
	case 3:
		priority = nx_mlc_priority_videofourth;
		break;	/* PRIORITY-0>1>2>video */
	default:
		pr_err(
			"fail : not support video priority num(0~3),(%d)\n",
		    prior);
		return;
	}

	top->width = width;
	top->height = height;

	pr_debug("%s: crtc.%d, %d by %d, prior %d, bg 0x%x\n",
		__func__, module, width, height, prior, bgcolor);

	nx_mlc_set_screen_size(module, top->width, top->height);
	nx_mlc_set_background(module, bgcolor & 0x00FFFFFF);
	nx_mlc_set_layer_priority(module, priority);
	nx_mlc_set_top_dirty_flag(module);
}

void nx_soc_dp_plane_top_set_bg_color(struct dp_plane_top *top)
{
	int module = top->module;
	unsigned int bgcolor = top->back_color;

	pr_debug("%s: crtc.%d, bg 0x%x\n",
		__func__, module, bgcolor);

	nx_mlc_set_background(module, bgcolor & 0x00FFFFFF);
	nx_mlc_set_top_dirty_flag(module);
}

int nx_soc_dp_plane_top_set_enable(struct dp_plane_top *top, bool on)
{
	struct dp_plane_layer *layer;
	int module = top->module;

	pr_debug("%s: crtc.%d, %s %dx%d\n",
		__func__, module, on ? "on" : "off", top->width, top->height);

	if (on) {
		int m_lock_size = 16;

		nx_mlc_set_field_enable(module, top->interlace);
		nx_mlc_set_rgblayer_gama_table_power_mode(module, 0, 0, 0);
		nx_mlc_set_rgblayer_gama_table_sleep_mode(module, 1, 1, 1);
		nx_mlc_set_rgblayer_gamma_enable(module, 0);
		nx_mlc_set_dither_enable_when_using_gamma(module, 0);
		nx_mlc_set_gamma_priority(module, 0);
		nx_mlc_set_top_power_mode(module, 1);
		nx_mlc_set_top_sleep_mode(module, 0);
		nx_mlc_set_mlc_enable(module, 1);

		list_for_each_entry(layer, &top->plane_list, list) {
			nx_mlc_set_lock_size(module, layer->num, m_lock_size);
			if (layer->enable) {
				nx_mlc_set_layer_enable(module, layer->num, 1);
				dp_plane_adjust(module, layer->num, true);
				pr_debug("%s: %s on\n", __func__, layer->name);
			}
		}

	} else {
		list_for_each_entry(layer, &top->plane_list, list) {
			if (layer->enable) {
				nx_mlc_set_layer_enable(module, layer->num, 1);
				dp_plane_adjust(module, layer->num, true);
			}
		}

		nx_mlc_set_top_power_mode(module, 0);
		nx_mlc_set_top_sleep_mode(module, 1);
		nx_mlc_set_mlc_enable(module, 0);
	}

	nx_mlc_set_top_dirty_flag(module);
	top->enable = on;

	return 0;
}

int nx_soc_dp_rgb_set_format(struct dp_plane_layer *layer,
			unsigned int format, int pixelbyte, bool adjust)
{
	int module = layer->module;
	int num = layer->num;
	int en_alpha = 0;

	pr_debug("%s: %s, fmt:0x%x, pixel=%d\n",
		 __func__, layer->name, format, pixelbyte);

	if (layer->format == format &&
		layer->pixelbyte == pixelbyte)
		return 0;

	layer->format = format;
	layer->pixelbyte = pixelbyte;

	/* set alphablend */
	if (format == nx_mlc_rgbfmt_a1r5g5b5 ||
	    format == nx_mlc_rgbfmt_a1b5g5r5 ||
	    format == nx_mlc_rgbfmt_a4r4g4b4 ||
	    format == nx_mlc_rgbfmt_a4b4g4r4 ||
	    format == nx_mlc_rgbfmt_a8r3g3b2 ||
	    format == nx_mlc_rgbfmt_a8b3g3r2 ||
	    format == nx_mlc_rgbfmt_a8r8g8b8 ||
	    format == nx_mlc_rgbfmt_a8b8g8r8)
		en_alpha = 1;

	/* nx_mlc_set_transparency(module,layer,0,layer->color.transcolor); */
	nx_mlc_set_color_inversion(module, num, 0, layer->color.invertcolor);
	nx_mlc_set_alpha_blending(module, num, en_alpha,
			layer->color.alphablend);
	nx_mlc_set_format_rgb(module, num, (enum nx_mlc_rgbfmt)format);
	nx_mlc_set_rgblayer_invalid_position(module, num, 0, 0, 0, 0, 0, 0);
	nx_mlc_set_rgblayer_invalid_position(module, num, 1, 0, 0, 0, 0, 0);
	dp_plane_adjust(module, num, adjust);

	return 0;
}

int nx_soc_dp_rgb_set_position(struct dp_plane_layer *layer,
			int src_x, int src_y, int src_w, int src_h,
			int dst_x, int dst_y, int dst_w, int dst_h,
			bool adjust)

{
	int module = layer->module;
	int num = layer->num;

	int sx = src_x;
	int sy = src_y;
	int sw = src_w;
	int sh = src_h;

	if (layer->left == sx && layer->top == sy &&
		layer->width == sw && layer->height == sh)
		return 0;

	if (layer->dst_left == dst_x && layer->dst_top == dst_y &&
		layer->dst_width == dst_w && layer->dst_height == dst_h)
		return 0;

	layer->left = sx;
	layer->top = sy;
	layer->width = sw;
	layer->height = sh;

	layer->dst_left = dst_x;
	layer->dst_top = dst_y;
	layer->dst_width = dst_w;
	layer->dst_height = dst_h;

	pr_debug("%s: %s, (%d, %d, %d, %d) to (%d, %d, %d, %d) adjust=%d\n",
		 __func__, layer->name, sx, sy, sw, sh,
		 dst_x, dst_y, sw, sh, adjust);

	nx_mlc_set_position(module, num, dst_x, dst_y,
		dst_x + sw - 1, dst_y + sh - 1);
	dp_plane_adjust(module, num, adjust);

	return 0;
}

void nx_soc_dp_rgb_set_address(struct dp_plane_layer *layer,
			unsigned int addr, unsigned int pixelbyte,
			unsigned int stride, bool adjust)
{
	int module = layer->module;
	int num = layer->num;
	int cl = layer->left, ct = layer->top;

	unsigned int phys = addr + (cl*pixelbyte) + (ct * stride);

	pr_debug("%s: %s, pa=0x%x, hs=%d, vs=%d, adjust=%d\n",
		__func__, layer->name, phys, pixelbyte, stride, adjust);

	if (adjust)
		dp_wait_vblank_done(module, num);

	nx_mlc_set_rgblayer_stride(module, num, pixelbyte, stride);
	nx_mlc_set_rgblayer_address(module, num, phys);
	dp_plane_adjust(module, num, adjust);
}

void nx_soc_dp_rgb_set_enable(struct dp_plane_layer *layer,
			bool on, bool adjust)
{
	int module = layer->module;
	int num = layer->num;

	pr_debug("%s: %s, %s\n",
		__func__, layer->name, on ? "on" : "off");

	if (on != layer->enable) {
		nx_mlc_set_layer_enable(module, num, (on ? 1 : 0));
		dp_plane_adjust(module, num, adjust);
	}

	layer->enable = on;
}

void nx_soc_dp_rgb_set_color(struct dp_plane_layer *layer,
			unsigned int type, unsigned int color,
			bool on, bool adjust)
{
	int module = layer->module;
	int num = layer->num;

	pr_debug("%s: %s, type:%d color:0x%x, pixel %d, %s\n",
		__func__, layer->name, type, color, layer->pixelbyte,
		on ? "on" : "off");

	switch (type) {
	case dp_color_alpha:
		if (color <= 0)
			color = 0;
		if (color >= 15)
			color = 15;

		layer->color.alpha = (on ? color : 15);

		nx_mlc_set_alpha_blending(module, num,
			 (on ? 1 : 0), (u32)color);

		dp_plane_adjust(module, num, adjust);
		break;

	case dp_color_transp:
		if (1 == layer->pixelbyte) {
			color = R8G8B8toR3G3B2((unsigned int)color);
			color = R3G3B2toR8G8B8((u8) color);
		}

		if (2 == layer->pixelbyte) {
			color = R8G8B8toR5G6B5((unsigned int)color);
			color = R5G6B5toR8G8B8((u_short) color);
		}

		layer->color.transcolor = (on ? color : 0);

		nx_mlc_set_transparency(module, num,
			(on ? 1 : 0), (u32)(color & 0x00FFFFFF));

		dp_plane_adjust(module, num, adjust);
		break;

	case dp_color_invert:
		if (1 == layer->pixelbyte) {
			color = R8G8B8toR3G3B2((unsigned int)color);
			color = R3G3B2toR8G8B8((u8) color);
		}

		if (2 == layer->pixelbyte) {
			color = R8G8B8toR5G6B5((unsigned int)color);
			color = R5G6B5toR8G8B8((u_short) color);
		}

		layer->color.invertcolor = (on ? color : 0);

		nx_mlc_set_color_inversion(module, num,
			(on ? 1 : 0),
			(u32)(color & 0x00FFFFFF));

		dp_plane_adjust(module, num, adjust);
		break;
	default:
		break;
	}
}

int nx_soc_dp_video_set_format(struct dp_plane_layer *layer,
			unsigned int format, bool adjust)
{
	int module = layer->module;

	if (layer->format == format)
		return 0;

	layer->format = format;

	pr_debug("%s: %s, format=0x%x\n",
		__func__, layer->name, format);

	nx_mlc_set_format_yuv(module, (enum nx_mlc_yuvfmt)format);
	dp_plane_adjust(module, LAYER_VIDEO, adjust);

	return 0;
}

int nx_soc_dp_video_set_position(struct dp_plane_layer *layer,
			int src_x, int src_y, int src_w, int src_h,
			int dst_x, int dst_y, int dst_w, int dst_h,
			bool adjust)
{
	int module = layer->module;
	int sx = src_x, sy = src_y;
	int sw = src_w, sh = src_h;
	int dx = dst_x, dy = dst_y;
	int dw = dst_w, dh = dst_h;
	int hf = 1, vf = 1;

	if (layer->left == sx && layer->top == sy &&
		layer->width == sw && layer->height == sh)
		return 0;

	if (layer->dst_left == dst_x && layer->dst_top == dst_y &&
		layer->dst_width == dst_w && layer->dst_height == dst_h)
		return 0;

	layer->left = sx;
	layer->top = sy;
	layer->width = sw;
	layer->height = sh;

	layer->dst_left = dst_x;
	layer->dst_top = dst_y;
	layer->dst_width = dst_w;
	layer->dst_height = dst_h;

	pr_debug("%s: %s, (%d, %d, %d, %d) to (%d, %d, %d, %d) adjust=%d\n",
		 __func__, layer->name, sx, sy, sw, sh,
		 dx, dy, dw, dh, adjust);

	if (dw > 2048)
		dw = 2048;

	if (dh > 2048)
		dh = 2048;

	if (sw == dw && sh == dh)
		hf = 0, vf = 0;

	layer->h_filter = hf;
	layer->v_filter = vf;

	/* set scale and position */
	nx_mlc_set_video_layer_scale(module, sw, sh, dw, dh, hf, hf, vf, vf);
	nx_mlc_set_position(module, LAYER_VIDEO, dx, dy,
			dx + dw - 1, dy + dh - 1);
	dp_plane_adjust(module, LAYER_VIDEO, adjust);

	return 0;
}

void nx_soc_dp_video_set_address_1plane(struct dp_plane_layer *layer,
			unsigned int addr, unsigned int stride,
			bool adjust)
{
	int module = layer->module;
	int cl = layer->left, ct = layer->top;
	unsigned int phys = addr + (cl/2) + (ct * stride);

	pr_debug("%s: %s, lu:0x%x->0x%x,%d\n",
		__func__, layer->name, addr, phys, stride);

	nx_mlc_set_video_layer_address_yuyv(module, phys, stride);
	dp_plane_adjust(module, LAYER_VIDEO, adjust);
}

void nx_soc_dp_video_set_address_3plane(struct dp_plane_layer *layer,
			unsigned int lu_a, unsigned int lu_s,
			unsigned int cb_a, unsigned int cb_s,
			unsigned int cr_a, unsigned int cr_s,
			bool adjust)
{
	int module = layer->module;
	int cl = layer->left;
	int ct = layer->top;
	int ls = 1, us = 1;
	int lh = 1, uh = 1;

	switch (layer->format) {
	case nx_mlc_yuvfmt_420:
			us = 2, uh = 2;
			break;
	case nx_mlc_yuvfmt_422:
			us = 2, uh = 1;
			break;
	case nx_mlc_yuvfmt_444:
			us = 1, uh = 1;
			break;
	}

	lu_a = lu_a + (cl/ls) + (ct/lh * lu_s);
	cb_a = cb_a + (cl/us) + (ct/uh * cb_s);
	cr_a = cr_a + (cl/us) + (ct/uh * cr_s);

	pr_debug("%s: %s, lu:0x%x,%d, cb:0x%x,%d, cr:0x%x,%d\n",
		__func__, layer->name, lu_a, lu_s, cb_a, cb_s, cr_a, cr_s);

	if (adjust)
		dp_wait_vblank_done(module, LAYER_VIDEO);

	nx_mlc_set_video_layer_stride(module, lu_s, cb_s, cr_s);
	nx_mlc_set_video_layer_address(module, lu_a, cb_a, cr_a);
	dp_plane_adjust(module, LAYER_VIDEO, adjust);
}

void nx_soc_dp_video_set_enable(struct dp_plane_layer *layer,
			bool on, bool adjust)
{
	int module = layer->module;
	int hl, hc, vl, vc;

	pr_debug("%s: %s, %s\n", __func__, layer->name, on ? "on" : "off");

	layer->enable = on;

	if (adjust)
		dp_wait_vblank_done(module, LAYER_VIDEO);

	if (on) {
		nx_mlc_set_video_layer_line_buffer_power_mode(module, 1);
		nx_mlc_set_video_layer_line_buffer_sleep_mode(module, 0);
		nx_mlc_set_layer_enable(module, LAYER_VIDEO, 1);
		dp_plane_adjust(module, LAYER_VIDEO, adjust);
	} else {
		nx_mlc_set_layer_enable(module, LAYER_VIDEO, 0);
		dp_plane_adjust(module, LAYER_VIDEO, adjust);
		WAIT_VBLANK(module, LAYER_VIDEO, 1);

		nx_mlc_get_video_layer_scale_filter(module, &hl, &hc, &vl, &vc);
		if (hl | hc | vl | vc)
			nx_mlc_set_video_layer_scale_filter(module, 0, 0, 0, 0);
		nx_mlc_set_video_layer_line_buffer_power_mode(module, 0);
		nx_mlc_set_video_layer_line_buffer_sleep_mode(module, 1);
		dp_plane_adjust(module, LAYER_VIDEO, adjust);
	}
}

void nx_soc_dp_device_setup(struct dp_control_dev *dpc)
{
	int module = dpc->module;

	pr_debug("%s: dev.%d\n", __func__, module);
	nx_dpc_set_clock_pclk_mode(module, nx_pclkmode_always);
}

int nx_soc_dp_device_prepare(struct dp_control_dev *dpc)
{
	struct dp_sync_info *sync = &dpc->sync;
	struct dp_ctrl_info *ctl = &dpc->ctrl;
	int module = dpc->module;
	unsigned int out_format = ctl->out_format;
	unsigned int delay_mask = ctl->delay_mask;
	int rgb_pvd = 0, hsync_cp1 = 7, vsync_fram = 7, de_cp2 = 7;
	int v_vso = 1, v_veo = 1, e_vso = 1, e_veo = 1;

	int interlace = sync->interlace;
	int invert_field = ctl->invert_field;
	int swap_rb = ctl->swap_rb;
	unsigned int yc_order = ctl->yc_order;
	int vck_select = ctl->vck_select;
	int vclk_invert = ctl->clk_inv_lv0 | ctl->clk_inv_lv1;
	int emb_sync = (out_format == DPC_FORMAT_CCIR656 ? 1 : 0);

	enum nx_dpc_dither r_dither, g_dither, b_dither;
	int rgb_mode = 0;
	bool lcd_rgb = dpc->panel_type == dp_panel_type_rgb ?
			true : false;

	/* set delay mask */
	if (delay_mask & DP_SYNC_DELAY_RGB_PVD)
		rgb_pvd = ctl->d_rgb_pvd;
	if (delay_mask & DP_SYNC_DELAY_HSYNC_CP1)
		hsync_cp1 = ctl->d_hsync_cp1;
	if (delay_mask & DP_SYNC_DELAY_VSYNC_FRAM)
		vsync_fram = ctl->d_vsync_fram;
	if (delay_mask & DP_SYNC_DELAY_DE_CP)
		de_cp2 = ctl->d_de_cp2;

	if (ctl->vs_start_offset != 0 ||
	    ctl->vs_end_offset != 0 ||
	    ctl->ev_start_offset != 0 || ctl->ev_end_offset != 0) {
		v_vso = ctl->vs_start_offset;
		v_veo = ctl->vs_end_offset;
		e_vso = ctl->ev_start_offset;
		e_veo = ctl->ev_end_offset;
	}

	if ((nx_dpc_format_rgb555 == out_format) ||
	    (nx_dpc_format_mrgb555a == out_format) ||
	    (nx_dpc_format_mrgb555b == out_format)) {
		r_dither = g_dither = b_dither = nx_dpc_dither_5bit;
		rgb_mode = 1;
	} else if ((nx_dpc_format_rgb565 == out_format) ||
		   (nx_dpc_format_mrgb565 == out_format)) {
		r_dither = b_dither = nx_dpc_dither_5bit;
		g_dither = nx_dpc_dither_6bit, rgb_mode = 1;
	} else if ((nx_dpc_format_rgb666 == out_format) ||
		   (nx_dpc_format_mrgb666 == out_format)) {
		r_dither = g_dither = b_dither = nx_dpc_dither_6bit;
		rgb_mode = 1;
	} else {
		r_dither = g_dither = b_dither = nx_dpc_dither_bypass;
		rgb_mode = 1;
	}

	/* CLKGEN0/1 */
	nx_dpc_set_clock_source(module, 0, ctl->clk_src_lv0);
	nx_dpc_set_clock_divisor(module, 0, ctl->clk_div_lv0);
	nx_dpc_set_clock_out_delay(module, 0, ctl->clk_delay_lv0);
	nx_dpc_set_clock_source(module, 1, ctl->clk_src_lv1);
	nx_dpc_set_clock_divisor(module, 1, ctl->clk_div_lv1);
	nx_dpc_set_clock_out_delay(module, 1, ctl->clk_delay_lv1);

	/* LCD out */
	if (lcd_rgb) {
		nx_dpc_set_mode(module, out_format, interlace, invert_field,
				rgb_mode, swap_rb, yc_order, emb_sync, emb_sync,
				vck_select, vclk_invert, 0);
		nx_dpc_set_hsync(module, sync->h_active_len,
				 sync->h_sync_width, sync->h_front_porch,
				 sync->h_back_porch, sync->h_sync_invert);
		nx_dpc_set_vsync(module, sync->v_active_len,
				 sync->v_sync_width, sync->v_front_porch,
				 sync->v_back_porch, sync->v_sync_invert,
				 sync->v_active_len, sync->v_sync_width,
				 sync->v_front_porch, sync->v_back_porch);
		nx_dpc_set_vsync_offset(module, v_vso, v_veo, e_vso, e_veo);
		nx_dpc_set_delay(module, rgb_pvd, hsync_cp1, vsync_fram,
				 de_cp2);
		nx_dpc_set_dither(module, r_dither, g_dither, b_dither);
	} else {
		enum polarity fd_polarity = polarity_activehigh;
		enum polarity hs_polarity = polarity_activehigh;
		enum polarity vs_polarity = polarity_activehigh;

		nx_dpc_set_sync(module,
				progressive,
				sync->h_active_len,
				sync->v_active_len,
				sync->h_sync_width,
				sync->h_front_porch,
				sync->h_back_porch,
				sync->v_sync_width,
				sync->v_front_porch,
				sync->v_back_porch,
				fd_polarity, hs_polarity,
				vs_polarity, 0, 0, 0, 0, 0, 0, 0);

		/* EvenVSW, EvenVFP, EvenVBP, VSP, VCP, EvenVSP, EvenVCP */
		nx_dpc_set_delay(module, rgb_pvd, hsync_cp1,
				vsync_fram, de_cp2);
		nx_dpc_set_output_format(module, out_format, 0);
		nx_dpc_set_dither(module, r_dither, g_dither, b_dither);
		nx_dpc_set_quantization_mode(module, qmode_256, qmode_256);
	}

	pr_debug("%s: %s dev.%d (x=%4d, hfp=%3d, hbp=%3d, hsw=%3d)\n",
		 __func__, dp_panel_type_name(dpc->panel_type), module,
		 sync->h_active_len, sync->h_front_porch,
		 sync->h_back_porch, sync->h_sync_width);
	pr_debug("%s: dev.%d (y=%4d, vfp=%3d, vbp=%3d, vsw=%3d)\n",
		 __func__, module, sync->v_active_len, sync->v_front_porch,
		 sync->v_back_porch, sync->v_sync_width);
	pr_debug
	    ("%s: dev.%d clk 0[s=%d, d=%3d], 1[s=%d, d=%3d], inv[%d:%d]\n",
	     __func__, module, ctl->clk_src_lv0, ctl->clk_div_lv0,
	     ctl->clk_src_lv1, ctl->clk_div_lv1, ctl->clk_inv_lv0,
	     ctl->clk_inv_lv1);
	pr_debug("%s: dev.%d v_vso=%d, v_veo=%d, e_vso=%d, e_veo=%d\n",
		 __func__, module, v_vso, v_veo, e_vso, e_veo);
	pr_debug("%s: dev.%d delay RGB=%d, HS=%d, VS=%d, DE=%d\n", __func__,
		 module, rgb_pvd, hsync_cp1, vsync_fram, de_cp2);

	return 0;
}

int nx_soc_dp_device_power_status(struct dp_control_dev *dpc)
{
	return 0;
}

void nx_soc_dp_device_power_on(struct dp_control_dev *dpc, bool on)
{
	int module = dpc->module;
	int count = 200;
	int vbl;

	pr_debug("%s: %s dev.%d power %s\n",
		__func__, dp_panel_type_name(dpc->panel_type),
		module, on ? "on" : "off");

	nx_dpc_clear_interrupt_pending_all(module);

	if (on) {
		nx_dpc_set_reg_flush(module);	/* for HDMI */
		nx_dpc_set_dpc_enable(module, 1);
		nx_dpc_set_clock_divisor_enable(module, 1);
	} else {
		nx_dpc_set_dpc_enable(module, 0);
		nx_dpc_set_clock_divisor_enable(module, 0);
	}

	/*
	 * wait for video sync
	 * for hdmi and output devices.
	 */
	vbl = nx_dpc_get_interrupt_enable_all(module);
	while (on) {
		if (!vbl) {
			msleep(20);
			break;
		}

		vbl = nx_dpc_get_interrupt_pending_all(module);
		mdelay(1);
		if (0 > --count || vbl)
			break;
	}
}

void nx_soc_dp_device_irq_on(struct dp_control_dev *dpc, bool on)
{
	int module = dpc->module;

	pr_debug("%s: dev.%d, %s\n", __func__, module, on ? "on" : "off");

	nx_dpc_clear_interrupt_pending_all(module);
	nx_dpc_set_interrupt_enable_all(module, on ? 1 : 0);
}

void nx_soc_dp_device_irq_done(struct dp_control_dev *dpc)
{
	int module = dpc->module;

	nx_dpc_clear_interrupt_pending_all(module);
}

