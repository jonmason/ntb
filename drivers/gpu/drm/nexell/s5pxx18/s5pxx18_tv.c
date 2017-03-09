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
#include <linux/delay.h>
#include <linux/reset.h>

#include <linux/io.h>

#include "s5pxx18_drv.h"
#include "s5pxx18_hdmi.h"
#include "s5pxx18_reg_hdmi.h"

static void __iomem *hdmi_base;
#define	hdmi_write(r, v)	writel(v, hdmi_base + r)
#define	hdmi_read(r)		readl(hdmi_base + r)

static int hdmi_clock_on_27MHz(struct nx_control_res *res);

static int tvout_ops_open(struct nx_drm_display *display, int pipe)
{
	struct nx_tvout_dev *tvout = display->context;
	struct nx_control_res *res = &tvout->control.res;

	pr_debug("%s: dev TV\n", __func__);
	hdmi_base = res->sub_bases[0];
	return 0;
}

static bool wait_for_hdmi_phy_ready(void)
{
	bool is_hdmi_phy_ready = false;

	while (!is_hdmi_phy_ready) {
		if (hdmi_read(HDMI_PHY_STATUS_0) & 0x01)
			is_hdmi_phy_ready = true;
	}

	return is_hdmi_phy_ready;
}

static int hdmi_clock_on_27MHz(struct nx_control_res *res)
{
	int err = 0;

	nx_tieoff_set(res->tieoffs[0][0], res->tieoffs[0][1]);
	nx_disp_top_clkgen_set_clock_pclk_mode(hdmi_clkgen, nx_pclkmode_always);

	reset_control_assert(res->sub_resets[4]);
	reset_control_assert(res->sub_resets[3]);

	reset_control_deassert(res->sub_resets[4]);
	reset_control_deassert(res->sub_resets[3]);

	nx_disp_top_clkgen_set_clock_pclk_mode(hdmi_clkgen, nx_pclkmode_always);

	hdmi_write(HDMI_PHY_REG7C, (0<<7));
	hdmi_write(HDMI_PHY_REG7C, (0<<7));
	hdmi_write(HDMI_PHY_REG04, (0<<4));
	hdmi_write(HDMI_PHY_REG04, (0<<4));
	hdmi_write(HDMI_PHY_REG24, (1<<7));
	hdmi_write(HDMI_PHY_REG24, (1<<7));
	hdmi_write(HDMI_PHY_REG04, 0xD1);
	hdmi_write(HDMI_PHY_REG04, 0xD1);
	hdmi_write(HDMI_PHY_REG08, 0x22);
	hdmi_write(HDMI_PHY_REG08, 0x22);
	hdmi_write(HDMI_PHY_REG0C, 0x51);
	hdmi_write(HDMI_PHY_REG0C, 0x51);
	hdmi_write(HDMI_PHY_REG10, 0x40);
	hdmi_write(HDMI_PHY_REG10, 0x40);
	hdmi_write(HDMI_PHY_REG14, 0x8);
	hdmi_write(HDMI_PHY_REG14, 0x8);
	hdmi_write(HDMI_PHY_REG18, 0xFC);
	hdmi_write(HDMI_PHY_REG18, 0xFC);
	hdmi_write(HDMI_PHY_REG1C, 0xE0);
	hdmi_write(HDMI_PHY_REG1C, 0xE0);
	hdmi_write(HDMI_PHY_REG20, 0x98);
	hdmi_write(HDMI_PHY_REG20, 0x98);
	hdmi_write(HDMI_PHY_REG24, 0xE8);
	hdmi_write(HDMI_PHY_REG24, 0xE8);
	hdmi_write(HDMI_PHY_REG28, 0xCB);
	hdmi_write(HDMI_PHY_REG28, 0xCB);
	hdmi_write(HDMI_PHY_REG2C, 0xD8);
	hdmi_write(HDMI_PHY_REG2C, 0xD8);
	hdmi_write(HDMI_PHY_REG30, 0x45);
	hdmi_write(HDMI_PHY_REG30, 0x45);
	hdmi_write(HDMI_PHY_REG34, 0xA0);
	hdmi_write(HDMI_PHY_REG34, 0xA0);
	hdmi_write(HDMI_PHY_REG38, 0xAC);
	hdmi_write(HDMI_PHY_REG38, 0xAC);
	hdmi_write(HDMI_PHY_REG3C, 0x80);
	hdmi_write(HDMI_PHY_REG3C, 0x80);
	hdmi_write(HDMI_PHY_REG40, 0x6);
	hdmi_write(HDMI_PHY_REG40, 0x6);
	hdmi_write(HDMI_PHY_REG44, 0x80);
	hdmi_write(HDMI_PHY_REG44, 0x80);
	hdmi_write(HDMI_PHY_REG48, 0x9);
	hdmi_write(HDMI_PHY_REG48, 0x9);
	hdmi_write(HDMI_PHY_REG4C, 0x84);
	hdmi_write(HDMI_PHY_REG4C, 0x84);
	hdmi_write(HDMI_PHY_REG50, 0x5);
	hdmi_write(HDMI_PHY_REG50, 0x5);
	hdmi_write(HDMI_PHY_REG54, 0x22);
	hdmi_write(HDMI_PHY_REG54, 0x22);
	hdmi_write(HDMI_PHY_REG58, 0x24);
	hdmi_write(HDMI_PHY_REG58, 0x24);
	hdmi_write(HDMI_PHY_REG5C, 0x86);
	hdmi_write(HDMI_PHY_REG5C, 0x86);
	hdmi_write(HDMI_PHY_REG60, 0x54);
	hdmi_write(HDMI_PHY_REG60, 0x54);
	hdmi_write(HDMI_PHY_REG64, 0xE4);
	hdmi_write(HDMI_PHY_REG64, 0xE4);
	hdmi_write(HDMI_PHY_REG68, 0x24);
	hdmi_write(HDMI_PHY_REG68, 0x24);
	hdmi_write(HDMI_PHY_REG6C, 0x0);
	hdmi_write(HDMI_PHY_REG6C, 0x0);
	hdmi_write(HDMI_PHY_REG70, 0x0);
	hdmi_write(HDMI_PHY_REG70, 0x0);
	hdmi_write(HDMI_PHY_REG74, 0x0);
	hdmi_write(HDMI_PHY_REG74, 0x0);
	hdmi_write(HDMI_PHY_REG78, 0x1);
	hdmi_write(HDMI_PHY_REG78, 0x1);
	hdmi_write(HDMI_PHY_REG7C, 0x80);
	hdmi_write(HDMI_PHY_REG7C, 0x80);
	hdmi_write(HDMI_PHY_REG7C, (1<<7));
	hdmi_write(HDMI_PHY_REG7C, (1<<7));/* MODE_SET_DONE : APB Set Done */

	if (!wait_for_hdmi_phy_ready())
		err = -1;

	return err;
}

static int tvout_commit(struct nx_drm_display *display)
{
	struct nx_tvout_dev *tvout = display->context;
	struct nx_control_res *res = &tvout->control.res;
	struct nx_control_info *ctrl = &tvout->control.ctrl;
	int ret = 0;

	if (ctrl->clk_src_lv0 == 4) {
		ret = hdmi_clock_on_27MHz(res);
		if (ret != 0) {
			pr_err("hdmi 27MHz clock set error!\n");
			return -1;
		}
	}
	return 0;
}

static int tvout_ops_prepare(struct nx_drm_display *display)
{
	struct nx_tvout_dev *tvout = display->context;
	struct nx_sync_info *sync = &tvout->control.sync;
	struct nx_control_info *ctrl = &tvout->control.ctrl;
	int module = tvout->control.module;
	unsigned int out_format = ctrl->out_format;
	int interlace = sync->interlace;
	int invert_field = ctrl->invert_field;
	int swap_rb = ctrl->swap_rb;
	unsigned int yc_order = ctrl->yc_order;
	int vclk_select = ctrl->vck_select;
	int vclk_invert = ctrl->clk_inv_lv0 | ctrl->clk_inv_lv1;
	int emb_sync = (out_format == DPC_FORMAT_CCIR656 ? 1 : 0);

	enum nx_dpc_dither r_dither, g_dither, b_dither;
	int rgb_mode = 0;

	u32 vsp = 0, vcp = 0, even_vsp = 0, even_vcp = 0;

	if (out_format != nx_dpc_format_ccir656) {
		pr_err("tvout just support CCIR656 now!\n");
		return -1;
	}

	tvout_commit(display);

	r_dither = g_dither = b_dither = nx_dpc_dither_bypass;

	/* CLKGEN0/1 */
	nx_dpc_set_clock_source(module, 0, ctrl->clk_src_lv0);
	nx_dpc_set_clock_divisor(module, 0, ctrl->clk_div_lv0);
	nx_dpc_set_clock_out_delay(module, 0, ctrl->clk_delay_lv0);
	nx_dpc_set_clock_out_inv(module, 0, ctrl->clk_inv_lv0);
	nx_dpc_set_clock_source(module, 1, ctrl->clk_src_lv1);
	nx_dpc_set_clock_divisor(module, 1, ctrl->clk_div_lv1);
	nx_dpc_set_clock_out_delay(module, 1, ctrl->clk_delay_lv1);
	nx_dpc_set_clock_out_inv(module, 1, ctrl->clk_inv_lv1);

	vsp	= 0;
	vcp	= (u32)((sync->h_sync_width + sync->h_front_porch +
		sync->h_back_porch + sync->h_active_len) / 2);
	even_vsp = (u32)((sync->h_sync_width + sync->h_front_porch +
		sync->h_back_porch + sync->h_active_len) / 2);
	even_vcp = 0;

	nx_dpc_set_mode(module, out_format, interlace, invert_field,
			rgb_mode, swap_rb, yc_order, emb_sync, emb_sync,
			vclk_select, vclk_invert, 0);
	nx_dpc_set_hsync(module, sync->h_active_len,
			 sync->h_sync_width, sync->h_front_porch,
			 sync->h_back_porch, sync->h_sync_invert);
	nx_dpc_set_vsync(module, sync->v_active_len,
			 sync->v_sync_width, sync->v_front_porch,
			 sync->v_back_porch, sync->v_sync_invert,
			 sync->v_active_len, sync->v_sync_width,
			 sync->v_front_porch, sync->v_back_porch+1);

	nx_dpc_set_vsync_offset(module, vsp, vcp, even_vsp, even_vcp);
	nx_dpc_set_delay(module, 12, 12, 12, 12);
	nx_dpc_set_dither(module, r_dither, g_dither, b_dither);

	pr_debug("%s: %s dev.%d (x=%4d, hfp=%3d, hbp=%3d, hsw=%3d)\n",
		 __func__, nx_panel_get_name(display->panel_type), module,
		 sync->h_active_len, sync->h_front_porch,
		 sync->h_back_porch, sync->h_sync_width);
	pr_debug("%s: dev.%d (y=%4d, vfp=%3d, vbp=%3d, vsw=%3d)\n",
		 __func__, module, sync->v_active_len, sync->v_front_porch,
		 sync->v_back_porch, sync->v_sync_width);
	pr_debug
	    ("%s: dev.%d clk 0[s=%d, d=%3d], 1[s=%d, d=%3d], inv[%d:%d]\n",
	     __func__, module, ctrl->clk_src_lv0, ctrl->clk_div_lv0,
	     ctrl->clk_src_lv1, ctrl->clk_div_lv1, ctrl->clk_inv_lv0,
	     ctrl->clk_inv_lv1);
	pr_debug("%s: dev.%d vsp=%d, vcp=%d, even_vsp=%d, even_vcp=%d\n",
		 __func__, module, vsp, vcp, even_vsp, even_vcp);

	return 0;
}

static int tvout_ops_enable(struct nx_drm_display *display)
{
	struct nx_tvout_dev *tvout = display->context;
	struct nx_sync_info *sync = &tvout->control.sync;
	int module = tvout->control.module;

	nx_dpc_set_reg_flush(module);

	if (sync->interlace > 0)
		nx_dpc_set_enable_with_interlace(module, 1, 0, 0, 0, 1);

	nx_dpc_set_dpc_enable(module, 1);
	nx_dpc_set_clock_divisor_enable(module, 1);

	return 0;
}

static int tvout_ops_disable(struct nx_drm_display *display)
{
	struct nx_tvout_dev *tvout = display->context;
	int module = tvout->control.module;

	nx_dpc_set_dpc_enable(module, 0);
	nx_dpc_set_clock_divisor_enable(module, 0);
	return 0;
}

static int tvout_ops_set_mode(struct nx_drm_display *display,
			struct drm_display_mode *mode, unsigned int flags)
{
	nx_display_mode_to_sync(mode, display);
	return 0;
}

static int tvout_ops_resume(struct nx_drm_display *display)
{
	nx_display_resume_resource(display);
	return 0;
}

static struct nx_drm_display_ops tvout_ops = {
	.open = tvout_ops_open,
	.prepare = tvout_ops_prepare,
	.enable = tvout_ops_enable,
	.disable = tvout_ops_disable,
	.set_mode = tvout_ops_set_mode,
	.resume = tvout_ops_resume,
};

void *nx_drm_display_tvout_get(struct device *dev,
			struct device_node *node,
			struct nx_drm_display *display)
{
	struct nx_tvout_dev *tvout;

	tvout = kzalloc(sizeof(*tvout), GFP_KERNEL);
	if (!tvout)
		return NULL;

	display->context = tvout;
	display->ops = &tvout_ops;

	return &tvout->control;
}
