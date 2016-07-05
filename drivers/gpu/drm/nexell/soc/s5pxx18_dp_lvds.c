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

#include "s5pxx18_dp_dev.h"

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

static void nx_soc_dp_lvds_set_base(struct dp_control_dev *dpc,
			void __iomem **base, int num)
{
	BUG_ON(!base);
	pr_debug("%s: dev mipi\n", __func__);

	nx_lvds_set_base_address(0, base[0]);
}

static int nx_soc_dp_lvds_set_prepare(struct dp_control_dev *dpc,
			unsigned int flags)
{
	unsigned int val;
	int clkid = dp_clock_lvds;
	enum dp_lvds_format format = dp_lvds_format_jeida;
	struct dp_lvds_dev *dev = dpc->dp_output;
	struct dp_ctrl_info *ctrl = &dpc->ctrl;
	struct reset_control **rsc = dev->reset_control;

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

	/*====================================================
	 * current not use location mode
	 ===================================================*/
	u32 LOC_A[7] = {ONE,ONE,ONE,ONE,ONE,ONE,ONE};
	u32 LOC_B[7] = {ONE,ONE,ONE,ONE,ONE,ONE,ONE};
	u32 LOC_C[7] = {VDEN,VSYNC,HSYNC,ONE, HSYNC, VSYNC, VDEN};
	u32 LOC_D[7] = {ZERO,ZERO,ZERO,ZERO,ZERO,ZERO,ZERO};
	u32 LOC_E[7] = {ZERO,ZERO,ZERO,ZERO,ZERO,ZERO,ZERO};

	if (dev)
		format = dev->lvds_format;

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
	val =	(0<<30)  	/* CPU_I_VBLK_FLAG_SEL */
			|	(0<<29)  /* CPU_I_BVLK_FLAG */
			|	(1<<28)  /* SKINI_BST  */
			|	(1<<27)  /* DLYS_BST  */
			|	(0<<26)  /* I_AUTO_SEL */
			|	(format<<19)  /* JEiDA data packing */
			|	(0x1B<<13)  /* I_LOCK_PPM_SET, PPM setting for PLL lock */
			|	(0x638<<1)  /* I_DESKEW_CNT_SEL, period of de-skew region */
			;
	nx_lvds_set_lvdsctrl0(0, val);

	val =	(0<<28) /* I_ATE_MODE, funtion mode */
			|	(0<<27) /* I_TEST_CON_MODE, DA (test ctrl mode) */
			|	(0<<24) /* I_TX4010X_DUMMY */
			|	(0<<15) /* SKCCK 0 */
			|	(0<<12) /* SKC4 (TX output skew control pin at ODD ch4) */
			|	(0<<9)  /* SKC3 (TX output skew control pin at ODD ch3) */
			|	(0<<6)  /* SKC2 (TX output skew control pin at ODD ch2) */
			|	(0<<3)  /* SKC1 (TX output skew control pin at ODD ch1) */
			|	(0<<0)  /* SKC0 (TX output skew control pin at ODD ch0) */
				;
	nx_lvds_set_lvdsctrl1(0, val);

	val =	(0<<15) /* CK_POL_SEL, Input clock, bypass */
			|	(0<<14) /* VSEL, VCO Freq. range. 0: Low(40MHz~90MHz), 1:High(90MHz~160MHz) */
			|	(0x1<<12) /* S (Post-scaler) */
			|	(0xA<<6) /* M (Main divider) */
			|	(0xA<<0) /* P (Pre-divider) */
			;
	nx_lvds_set_lvdsctrl2(0, val);

	val =	(0x03<<6) /* SK_BIAS, Bias current ctrl pin */
			|	(0<<5) /* SKEWINI, skew selection pin, 0 : bypass, 1 : skew enable */
			|	(0<<4) /* SKEW_EN_H, skew block power down, 0 : power down, 1 : operating */
			|	(1<<3) /* CNTB_TDLY, delay control pin */
			|	(0<<2) /* SEL_DATABF, input clock 1/2 division control pin */
			|	(0x3<<0) /* SKEW_REG_CUR, regulator bias current selection in in SKEW block */
			;
	nx_lvds_set_lvdsctrl3(0, val);

	val =	(0<<28) /* FLT_CNT, filter control pin for PLL */
			|	(0<<27) /* VOD_ONLY_CNT, the pre-emphasis's pre-diriver control pin (VOD only) */
			|	(0<<26) /* CNNCT_MODE_SEL, connectivity mode selection, 0:TX operating, 1:con check */
			|	(0<<24) /* CNNCT_CNT, connectivity ctrl pin, 0:tx operating, 1: con check */
			|	(0<<23) /* VOD_HIGH_S, VOD control pin, 1 : Vod only */
			|	(0<<22) /* SRC_TRH, source termination resistor select pin */
			|	(0x20/*0x3F*/<<14) /* CNT_VOD_H, TX driver output differential volatge level control pin */
			|	(0x01<<6) /* CNT_PEN_H, TX driver pre-emphasis level control */
			|	(0x4<<3) /* FC_CODE, vos control pin */
			|	(0<<2) /* OUTCON, TX Driver state selectioin pin, 0:Hi-z, 1:Low */
			|	(0<<1) /* LOCK_CNT, Lock signal selection pin, enable */
			|	(0<<0) /* AUTO_DSK_SEL, auto deskew selection pin, normal */
			;
	nx_lvds_set_lvdsctrl4(0, val);

	val =	(0<<24)	/* I_BIST_RESETB */
			|	(0<<23)	/* I_BIST_EN */
			|	(0<<21)	/* I_BIST_PAT_SEL */
			|	(0<<14) /* I_BIST_USER_PATTERN */
			|	(0<<13)	/* I_BIST_FORCE_ERROR */
			|	(0<<7)	/* I_BIST_SKEW_CTRL */
			|	(0<<5)	/* I_BIST_CLK_INV */
			|	(0<<3)	/* I_BIST_DATA_INV */
			|	(0<<0)	/* I_BIST_CH_SEL */
			;
	nx_lvds_set_lvdstmode0(0, val);

	/* user do not need to modify this codes. */
	val = (LOC_A[4]  <<24) | (LOC_A[3]  <<18) | (LOC_A[2]  <<12) | (LOC_A[1]  <<6) | (LOC_A[0]  <<0);
	nx_lvds_set_lvdsloc0(0, val);

	val = (LOC_B[2]  <<24) | (LOC_B[1]  <<18) | (LOC_B[0]  <<12) | (LOC_A[6]  <<6) | (LOC_A[5]  <<0);
	nx_lvds_set_lvdsloc1(0, val);

	val = (LOC_C[0]  <<24) | (LOC_B[6]  <<18) | (LOC_B[5]  <<12) | (LOC_B[4]  <<6) | (LOC_B[3]  <<0);
	nx_lvds_set_lvdsloc2(0, val);

	val = (LOC_C[5]  <<24) | (LOC_C[4]  <<18) | (LOC_C[3]  <<12) | (LOC_C[2]  <<6) | (LOC_C[1]  <<0);
	nx_lvds_set_lvdsloc3(0, val);

	val = (LOC_D[3]  <<24) | (LOC_D[2]  <<18) | (LOC_D[1]  <<12) | (LOC_D[0]  <<6) | (LOC_C[6]  <<0);
	nx_lvds_set_lvdsloc4(0, val);

	val = (LOC_E[1]  <<24) | (LOC_E[0]  <<18) | (LOC_D[6]  <<12) | (LOC_D[5]  <<6) | (LOC_D[4]  <<0);
	nx_lvds_set_lvdsloc5(0, val);

	val = (LOC_E[6]  <<24) | (LOC_E[5]  <<18) | (LOC_E[4]  <<12) | (LOC_E[3]  <<6) | (LOC_E[2]  <<0);
	nx_lvds_set_lvdsloc6(0, val);

	nx_lvds_set_lvdslocmask0(0, 0xffffffff);
	nx_lvds_set_lvdslocmask1(0, 0xffffffff);

	nx_lvds_set_lvdslocpol0(0, (0<<19) | (0<<18));

	/*
	 * LVDS PHY Reset, make sure last.
	 */
	lvds_phy_reset(rsc, dev->num_resets);

	return 0;
}

static int nx_soc_dp_lvds_set_unprepare(struct dp_control_dev *dpc)
{
	return 0;
}

static int nx_soc_dp_lvds_set_enable(struct dp_control_dev *dpc,
			unsigned int flags)
{
	int clkid = dp_clock_lvds;
	int module = dpc->module;

	pr_debug("%s dev.%d\n", __func__, module);

  	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, 1);
	nx_disp_top_set_lvdsmux(1, module);

	return 0;
}

static int nx_soc_dp_lvds_set_disable(struct dp_control_dev *dpc)
{
	int clkid = dp_clock_lvds;

	pr_debug("%s\n", __func__);

	/* SPDIF and MIPI */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, 0);

	/* START: CLKGEN, MIPI is started in setup function */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, false);

	return 0;
}

static struct dp_control_ops lvds_dp_ops = {
	.set_base = nx_soc_dp_lvds_set_base,
	.prepare = nx_soc_dp_lvds_set_prepare,
	.unprepare = nx_soc_dp_lvds_set_unprepare,
	.enable = nx_soc_dp_lvds_set_enable,
	.disable = nx_soc_dp_lvds_set_disable,
};

int nx_dp_device_lvds_register(struct device *dev,
			struct device_node *np, struct dp_control_dev *dpc,
			void *resets, int num_resets)
{
	struct dp_lvds_dev *out;

	out = devm_kzalloc(dev, sizeof(*out), GFP_KERNEL);
	if (IS_ERR(out))
		return -ENOMEM;

	out->reset_control = (void *)resets;
	out->num_resets = num_resets;
	dpc->panel_type = dp_panel_type_lvds;
	dpc->dp_output = out;
	dpc->ops = &lvds_dp_ops;

	return 0;
}
