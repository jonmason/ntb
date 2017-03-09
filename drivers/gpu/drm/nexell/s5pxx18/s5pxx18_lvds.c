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
#include <linux/slab.h>
#include <linux/of.h>

#include "s5pxx18_drv.h"

#define	DEF_VOLTAGE_LEVEL	(0x20)

static void lvds_phy_reset(struct reset_control *rsc[], int num)
{
	int count = num;
	int i;

	pr_debug("%s: resets %d\n", __func__, num);

	for (i = 0; count > i; i++)
		reset_control_assert(rsc[i]);

	mdelay(1);

	for (i = 0; count > i; i++)
		reset_control_deassert(rsc[i]);
}

static int lvds_ops_open(struct nx_drm_display *display, int pipe)
{
	struct nx_lvds_dev *lvds = display->context;
	struct nx_control_dev *control = &lvds->control;
	struct nx_control_res *res = &control->res;

	pr_debug("%s: pipe.%d\n", __func__, pipe);

	control->module = pipe;
	nx_lvds_set_base_address(0, res->sub_bases[0]);

	return 0;
}

static int lvds_ops_prepare(struct nx_drm_display *display)
{
	struct nx_lvds_dev *lvds = display->context;
	struct nx_control_res *res = &lvds->control.res;
	struct nx_control_info *ctrl = &lvds->control.ctrl;
	enum nx_lvds_format format = NX_LVDS_FORMAT_JEIDA;
	int clkid = NX_CLOCK_LVDS;
	u32 voltage = DEF_VOLTAGE_LEVEL;
	u32 val;

	/*
	 *-------- predefined type.
	 * only change iTA to iTE in VESA mode
	 * wire [34:0] loc_VideoIn =
	 * {4'hf, 4'h0, i_VDEN, i_VSYNC, i_HSYNC, i_VD[23:0] };
	 */
	u32 VSYNC = 25;
	u32 HSYNC = 24;
	u32 VDEN  = 26; /* bit position */
	u32 ONE   = 34;
	u32 ZERO  = 27;

	/*
	 * ====================================================
	 * current not use location mode
	 * ===================================================
	*/
	u32 LOC_A[7] = {ONE, ONE, ONE, ONE, ONE, ONE, ONE};
	u32 LOC_B[7] = {ONE, ONE, ONE, ONE, ONE, ONE, ONE};
	u32 LOC_C[7] = {VDEN, VSYNC, HSYNC, ONE, HSYNC, VSYNC, VDEN};
	u32 LOC_D[7] = {ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO};
	u32 LOC_E[7] = {ZERO, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO};

#ifdef CONFIG_DRM_CHECK_PRE_INIT
	if (nx_disp_top_clkgen_get_clock_divisor_enable(clkid)) {
		pr_debug("%s: already enabled ...\n", __func__);
		return 0;
	}
#endif
	if (lvds) {
		format = lvds->lvds_format;
		voltage = lvds->voltage_level;
	}

	pr_debug("%s: format: %d\n", __func__, format);

	/*
	 * select TOP MUX
	 */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, 0);
	nx_disp_top_clkgen_set_clock_source(clkid, 0, ctrl->clk_src_lv0);
	nx_disp_top_clkgen_set_clock_divisor(clkid, 0, ctrl->clk_div_lv0);
	nx_disp_top_clkgen_set_clock_source(clkid, 1, ctrl->clk_src_lv1);
	nx_disp_top_clkgen_set_clock_divisor(clkid, 1, ctrl->clk_div_lv1);

	/*
	 * LVDS Control Pin Setting
	 */
	val = (0<<30) /* CPU_I_VBLK_FLAG_SEL */
		| (0<<29)  /* CPU_I_BVLK_FLAG */
		| (1<<28)  /* SKINI_BST  */
		| (1<<27)  /* DLYS_BST  */
		| (0<<26)  /* I_AUTO_SEL */
		| (format<<19)  /* JEiDA data packing */
		| (0x1B<<13)  /* I_LOCK_PPM_SET, PPM setting for PLL lock */
		| (0x638<<1)  /* I_DESKEW_CNT_SEL, period of de-skew region */
		;
	nx_lvds_set_lvdsctrl0(0, val);

	val = (0<<28) /* I_ATE_MODE, funtion mode */
		| (0<<27) /* I_TEST_CON_MODE, DA (test ctrl mode) */
		| (0<<24) /* I_TX4010X_DUMMY */
		| (0<<15) /* SKCCK 0 */
		| (0<<12) /* SKC4 (TX output skew control pin at ODD ch4) */
		| (0<<9)  /* SKC3 (TX output skew control pin at ODD ch3) */
		| (0<<6)  /* SKC2 (TX output skew control pin at ODD ch2) */
		| (0<<3)  /* SKC1 (TX output skew control pin at ODD ch1) */
		| (0<<0)  /* SKC0 (TX output skew control pin at ODD ch0) */
		;
	nx_lvds_set_lvdsctrl1(0, val);

	val = (0<<15) /* CK_POL_SEL, Input clock, bypass */
		| (0<<14)	/*
				 * VSEL, VCO Freq. range. 0: Low(40MHz~90MHz),
				 * 1:High(90MHz~160MHz)
				 */
		| (0x1<<12) /* S (Post-scaler) */
		| (0xA<<6) /* M (Main divider) */
		| (0xA<<0) /* P (Pre-divider) */
		;
	nx_lvds_set_lvdsctrl2(0, val);

	val = (0x03<<6) /* SK_BIAS, Bias current ctrl pin */
		| (0<<5) /* SKEWINI, skew pin, 0 : bypass, 1 : skew enable */
		| (0<<4) /* SKEW_EN_H, skew power, 0 : down, 1 : operating */
		| (1<<3) /* CNTB_TDLY, delay control pin */
		| (0<<2) /* SEL_DATABF, input clock 1/2 division control pin */
		| (0x3<<0) /* SKEW_REG_CUR, regulator bias current selection */
		;
	nx_lvds_set_lvdsctrl3(0, val);

	val =	(0<<28) /* FLT_CNT, filter control pin for PLL */
		| (0<<27)	/*
				 * VOD_ONLY_CNT, the pre-emphasis's pre-diriver
				 * control pin (VOD only)
				 */
		| (0<<26)	/*
				 * CNNCT_MODE_SEL, connectivity mode selection,
				 * 0:TX operating, 1:con check
				 */
		| (0<<24)	/*
				 * CNNCT_CNT, connectivity ctrl pin,
				 * 0:tx operating, 1: con check
				 */
		| (0<<23) /* VOD_HIGH_S, VOD control pin, 1 : Vod only */
		| (0<<22) /* SRC_TRH, source termination resistor select pin */
		| (voltage<<14)
		| (0x01<<6) /* CNT_PEN_H, TX pre-emphasis level control */
		| (0x4<<3) /* FC_CODE, vos control pin */
		| (0<<2) /* OUTCON, TX Driver state pin, 0:Hi-z, 1:Low */
		| (0<<1) /* LOCK_CNT, Lock signal selection pin, enable */
		| (0<<0) /* AUTO_DSK_SEL, auto deskew selection pin, normal */
		;
	nx_lvds_set_lvdsctrl4(0, val);

	val = (0<<24)	/* I_BIST_RESETB */
		| (0<<23) /* I_BIST_EN */
		| (0<<21) /* I_BIST_PAT_SEL */
		| (0<<14) /* I_BIST_USER_PATTERN */
		| (0<<13) /* I_BIST_FORCE_ERROR */
		| (0<<7) /* I_BIST_SKEW_CTRL */
		| (0<<5) /* I_BIST_CLK_INV */
		| (0<<3) /* I_BIST_DATA_INV */
		| (0<<0) /* I_BIST_CH_SEL */
		;
	nx_lvds_set_lvdstmode0(0, val);

	/* user do not need to modify this codes. */
	val = (LOC_A[4] << 24) | (LOC_A[3] << 18) |
		(LOC_A[2] << 12) | (LOC_A[1] << 6) | (LOC_A[0] << 0);

	nx_lvds_set_lvdsloc0(0, val);

	val = (LOC_B[2] << 24) | (LOC_B[1] << 18) |
		(LOC_B[0] << 12) | (LOC_A[6] << 6) | (LOC_A[5] << 0);

	nx_lvds_set_lvdsloc1(0, val);

	val = (LOC_C[0] << 24) | (LOC_B[6] << 18) |
		(LOC_B[5] << 12) | (LOC_B[4] << 6) | (LOC_B[3] << 0);

	nx_lvds_set_lvdsloc2(0, val);

	val = (LOC_C[5] << 24) | (LOC_C[4] << 18) |
		(LOC_C[3] << 12) | (LOC_C[2] << 6) | (LOC_C[1] << 0);

	nx_lvds_set_lvdsloc3(0, val);

	val = (LOC_D[3] << 24) | (LOC_D[2] << 18) |
		(LOC_D[1] << 12) | (LOC_D[0] << 6) | (LOC_C[6] << 0);

	nx_lvds_set_lvdsloc4(0, val);

	val = (LOC_E[1] << 24) | (LOC_E[0] << 18) |
		(LOC_D[6] << 12) | (LOC_D[5] << 6) | (LOC_D[4] << 0);

	nx_lvds_set_lvdsloc5(0, val);

	val = (LOC_E[6] << 24) | (LOC_E[5] << 18) |
		(LOC_E[4] << 12) | (LOC_E[3] << 6) | (LOC_E[2] << 0);

	nx_lvds_set_lvdsloc6(0, val);

	nx_lvds_set_lvdslocmask0(0, 0xffffffff);
	nx_lvds_set_lvdslocmask1(0, 0xffffffff);

	nx_lvds_set_lvdslocpol0(0, (0<<19) | (0<<18));

	/*
	 * LVDS PHY Reset, make sure last.
	 */
	lvds_phy_reset(res->sub_resets, res->num_sub_resets);

	return 0;
}

static int lvds_ops_enable(struct nx_drm_display *display)
{
	struct nx_lvds_dev *lvds = display->context;
	int module = lvds->control.module;
	int clkid = NX_CLOCK_LVDS;

	pr_debug("%s lvds.%d\n", __func__, module);

	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, 1);
	nx_disp_top_set_lvdsmux(1, module);

	return 0;
}

static int lvds_ops_disable(struct nx_drm_display *display)
{
	int clkid = NX_CLOCK_LVDS;

	pr_debug("%s\n", __func__);

	/* SPDIF and MIPI */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, 0);

	/* START: CLKGEN, MIPI is started in setup function */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, false);

	return 0;
}

static int lvds_ops_set_mode(struct nx_drm_display *display,
			struct drm_display_mode *mode, unsigned int flags)
{
	nx_display_mode_to_sync(mode, display);
	return 0;
}

static int lvds_ops_resume(struct nx_drm_display *display)
{
	nx_display_resume_resource(display);
	return 0;
}

static struct nx_drm_display_ops lvds_ops = {
	.open = lvds_ops_open,
	.prepare = lvds_ops_prepare,
	.enable = lvds_ops_enable,
	.disable = lvds_ops_disable,
	.set_mode = lvds_ops_set_mode,
	.resume = lvds_ops_resume,
};

void *nx_drm_display_lvds_get(struct device *dev,
			struct device_node *node,
			struct nx_drm_display *display)
{
	struct nx_lvds_dev *lvds;
	u32 format;
	u32 voltage;

	lvds = kzalloc(sizeof(*lvds), GFP_KERNEL);
	if (!lvds)
		return NULL;

	if (!of_property_read_u32(node, "format", &format))
		lvds->lvds_format = format;

	if (!of_property_read_u32(node, "voltage_level", &voltage))
		lvds->voltage_level = voltage;

	display->context = lvds;
	display->ops = &lvds_ops;

	return &lvds->control;
}
