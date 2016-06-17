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
#include <linux/hdmi.h>

#include "s5pxx18_dp_hdmi.h"
#include "s5pxx18_reg_hdmi.h"

#define DEFAULT_SAMPLE_RATE		48000
#define DEFAULT_BITS_PER_SAMPLE		16
#define DEFAULT_AUDIO_CODEC		HDMI_AUDIO_PCM
#define DEFAULT_HDMIPHY_TX_LEVEL	23
#define HDMI_PHY_TABLE_SIZE		30

#define	HDMI_CHECK_REFRESH		(1<<0)
#define	HDMI_CHECK_PIXCLOCK		(1<<1)

#define	display_to_dpc(d)	(&d->ctrl.dpc)

static int hdmi_hpd_status(void)
{
	return hdmi_read(HDMI_HPD_STATUS);
}

static void hdmi_reset(struct reset_control *rsc[], int num)
{
	int count = (num - 1);	/* skip hdmi phy reset */
	int i;

	pr_debug("%s: resets %d\n", __func__, num);

	for (i = 0; count > i; i++)
		reset_control_assert(rsc[i]);

	mdelay(1);

	for (i = 0; count > i; i++)
		reset_control_deassert(rsc[i]);
}

static bool hdmi_wait_phy_ready(void)
{
	u32 val;
	int count = 500;
	bool ret = false;

	do {
		val = hdmi_read(HDMI_PHY_STATUS_0);
		if (val & 0x01) {
			ret = true;
			break;
		}
		mdelay(10);
	} while (count--);

	pr_info("HDMI: PHY [%s][0x%x] ...\n",
	       ret ? "Ready Done" : "Fail : Not Ready", val);

	return ret;
}

#ifdef DEFAULT_HDMIPHY_TX_LEVEL
static int hdmi_tx_level_get(void)
{
	u32 val;
	int lv;

	val = hdmi_read(HDMI_PHY_REG3C);
	lv = (val & 0x80) >> 7;
	val = hdmi_read(HDMI_PHY_REG40);
	val &= 0x0f;
	val <<= 1;
	lv |= val;

	return lv;
}

static int hdmi_tx_level_set(int level, bool enable)
{
	u32 val;
	u32 dat;

	if (level > 31) {
		pr_err("%s: hdmi invalid tx level %d, 0 ~ 31\n",
		       __func__, level);
		return -EINVAL;
	}

	if (!enable)
		return 0;

	val = hdmi_tx_level_get();
	if (val == level)
		return 0;

	hdmi_write(HDMI_PHY_REG7C, (0 << 7));
	hdmi_write(HDMI_PHY_REG7C, (0 << 7));

	dat = hdmi_read(HDMI_PHY_REG3C);
	dat = hdmi_read(HDMI_PHY_REG3C);

	val = level & 0x1;
	val <<= 7;
	dat &= ~0x80;
	dat |= val;

	hdmi_write(HDMI_PHY_REG3C, dat);
	hdmi_write(HDMI_PHY_REG3C, dat);

	dat = hdmi_read(HDMI_PHY_REG40);
	dat = hdmi_read(HDMI_PHY_REG40);

	val = (level & 0x1f) >> 1;
	dat &= ~0x0f;
	dat |= val;

	hdmi_write(HDMI_PHY_REG40, dat);
	hdmi_write(HDMI_PHY_REG40, dat);
	hdmi_write(HDMI_PHY_REG7C, (1 << 7));
	hdmi_write(HDMI_PHY_REG7C, (1 << 7));

	return 0;

}
#endif

static void hdmi_phy_set(const struct hdmi_conf *conf, int size)
{
	const u8 *data = conf->phy_data;
	u32 addr = HDMI_PHY_REG04;
	int i;

	hdmi_write(HDMI_PHY_REG7C, (0 << 7));
	hdmi_write(HDMI_PHY_REG04, (0 << 4));
	hdmi_write(HDMI_PHY_REG24, (1 << 7));

	for (i = 0; size > i; i++, addr += 4)
		hdmi_write(addr, data[i]);

#ifdef DEFAULT_HDMIPHY_TX_LEVEL
	hdmi_tx_level_set(DEFAULT_HDMIPHY_TX_LEVEL, true);
#endif

	hdmi_write(HDMI_PHY_REG7C, 0x80);
	hdmi_write(HDMI_PHY_REG7C, (1 << 7));
}

static void hdmi_dp_set(struct dp_control_dev *dpc, struct videomode *vm)
{
	struct dp_ctrl_info *ctrl = &dpc->ctrl;

	/*
	 * FIX dpc clock source, from HDMI phy
	 */
	ctrl->clk_src_lv0 = 4;
	ctrl->clk_div_lv0 = 1;
	ctrl->clk_src_lv1 = 7;
	ctrl->clk_div_lv1 = 1;

	ctrl = &dpc->ctrl;
	ctrl->out_format = outputformat_rgb888;
	ctrl->delay_mask = (DP_SYNC_DELAY_RGB_PVD | DP_SYNC_DELAY_HSYNC_CP1 |
			    DP_SYNC_DELAY_VSYNC_FRAM | DP_SYNC_DELAY_DE_CP);
	ctrl->d_rgb_pvd = 0;
	ctrl->d_hsync_cp1 = 0;
	ctrl->d_vsync_fram = 0;
	ctrl->d_de_cp2 = 7;

	/* HFP + HSW + HBP + AVWidth-VSCLRPIXEL- 1; */
	ctrl->vs_start_offset = (vm->hfront_porch + vm->hsync_len +
				 vm->hback_porch + vm->hactive - 1);
	ctrl->vs_end_offset = 0;

	/* HFP + HSW + HBP + AVWidth-EVENVSCLRPIXEL- 1 */
	ctrl->ev_start_offset = (vm->hfront_porch + vm->hsync_len +
				 vm->hback_porch + vm->hactive - 1);
	ctrl->ev_end_offset = 0;
}

static void hdmi_clock_on(void)
{
	bool enabled =
		nx_disp_top_clkgen_get_clock_divisor_enable(to_mipi_clkgen);

	/* check for mipi-dsi */
	if (!enabled)
		nx_disp_top_clkgen_set_clock_divisor_enable(to_mipi_clkgen, 0);

	nx_disp_top_clkgen_set_clock_pclk_mode(to_mipi_clkgen,
					       nx_pclkmode_always);
	nx_disp_top_clkgen_set_clock_source(to_mipi_clkgen,
			HDMI_SPDIF_CLKOUT, 2);
	nx_disp_top_clkgen_set_clock_divisor(to_mipi_clkgen,
			HDMI_SPDIF_CLKOUT, 2);
	/*
	 * skip : do not change mipi source
	 * nx_disp_top_clkgen_set_clock_source(to_mipi_clkgen, 1, 7);
	 */
	nx_disp_top_clkgen_set_clock_divisor_enable(to_mipi_clkgen, 1);
}

static void hdmi_standby(void)
{
	nx_disp_top_hdmi_set_vsync_hsstart_end(0, 0);
	nx_disp_top_hdmi_set_vsync_start(0);
	nx_disp_top_hdmi_set_hactive_start(0);
	nx_disp_top_hdmi_set_hactive_end(0);
}

static void hdmi_conf_set(const struct hdmi_conf *conf)
{
	const struct hdmi_preset *preset = conf->preset;
	const struct hdmi_res_mode *mode = &preset->mode;
	u32 h_blank, h_line, h_sync_start, h_sync_end;
	u32 v_blank, v2_blank, v_line;
	u32 v_sync_line_bef_1, v_sync_line_bef_2;

	u32 fixed_ffff = 0xffff;

	/*
	 * calculate sync variables
	 */
	h_blank = mode->h_fp + mode->h_sw + mode->h_bp;
	v_blank = mode->v_fp + mode->v_sw + mode->v_bp;
	v_line = mode->v_as + v_blank;	/* total v */
	v2_blank = mode->v_as + v_blank;	/* total v */
	h_line = mode->h_as + mode->h_fp + mode->h_sw + mode->h_bp;
	h_sync_start = mode->h_fp;
	h_sync_end = mode->h_fp + mode->h_sw;
	v_sync_line_bef_1 = mode->v_fp;
	v_sync_line_bef_2 = mode->v_fp + mode->v_sw;

	pr_debug("%s : %s ha:%4d, hf:%3d, hb:%3d, hs:%3d\n",
		 __func__, mode->name,
		 mode->h_as, mode->h_fp, mode->h_bp, mode->h_sw);
	pr_debug("%s : %s va:%4d, vf:%3d, vb:%3d, vs:%3d\n",
		 __func__, mode->name,
		 mode->v_as, mode->v_fp, mode->v_bp, mode->v_sw);

	/* no blue screen mode, encoding order as it is */
	hdmi_write(HDMI_CON_0, (0 << 5) | (1 << 4));

	/* set HDMI_BLUE_SCREEN_* to 0x0 */
	hdmi_write(HDMI_BLUE_SCREEN_R_0, 0x5555);
	hdmi_write(HDMI_BLUE_SCREEN_R_1, 0x5555);
	hdmi_write(HDMI_BLUE_SCREEN_G_0, 0x5555);
	hdmi_write(HDMI_BLUE_SCREEN_G_1, 0x5555);
	hdmi_write(HDMI_BLUE_SCREEN_B_0, 0x5555);
	hdmi_write(HDMI_BLUE_SCREEN_B_1, 0x5555);

	/* set HDMI_CON_1 to 0x0 */
	hdmi_write(HDMI_CON_1, 0x0);
	hdmi_write(HDMI_CON_2, 0x0);

	/* set interrupt : enable hpd_plug, hpd_unplug */
	hdmi_write(HDMI_INTC_CON_0, (1 << 6) | (1 << 3) | (1 << 2));

	/* set STATUS_EN to 0x17 */
	hdmi_write(HDMI_STATUS_EN, 0x17);

	/* set HPD to 0x0 : later check hpd */
	hdmi_write(HDMI_HPD_STATUS, 0x0);

	/* set MODE_SEL to 0x02 */
	hdmi_write(HDMI_MODE_SEL, 0x2);

	/* set H_BLANK_*, V1_BLANK_*, V2_BLANK_*, V_LINE_*,
	 * H_LINE_*, H_SYNC_START_*, H_SYNC_END_ *
	 * V_SYNC_LINE_BEF_1_*, V_SYNC_LINE_BEF_2_*
	 */
	hdmi_write(HDMI_H_BLANK_0, h_blank % 256);
	hdmi_write(HDMI_H_BLANK_1, h_blank >> 8);
	hdmi_write(HDMI_V1_BLANK_0, v_blank % 256);
	hdmi_write(HDMI_V1_BLANK_1, v_blank >> 8);
	hdmi_write(HDMI_V2_BLANK_0, v2_blank % 256);
	hdmi_write(HDMI_V2_BLANK_1, v2_blank >> 8);
	hdmi_write(HDMI_V_LINE_0, v_line % 256);
	hdmi_write(HDMI_V_LINE_1, v_line >> 8);
	hdmi_write(HDMI_H_LINE_0, h_line % 256);
	hdmi_write(HDMI_H_LINE_1, h_line >> 8);

	/* 480p, 576p : invert VSYNC, HSYNC */
	if (mode->h_as == 720) {
		hdmi_write(HDMI_HSYNC_POL, 0x1);
		hdmi_write(HDMI_VSYNC_POL, 0x1);
	} else {
		hdmi_write(HDMI_HSYNC_POL, 0x0);
		hdmi_write(HDMI_VSYNC_POL, 0x0);
	}

	hdmi_write(HDMI_INT_PRO_MODE, 0x0);

	hdmi_write(HDMI_H_SYNC_START_0, (h_sync_start % 256) - 2);
	hdmi_write(HDMI_H_SYNC_START_1, h_sync_start >> 8);
	hdmi_write(HDMI_H_SYNC_END_0, (h_sync_end % 256) - 2);
	hdmi_write(HDMI_H_SYNC_END_1, h_sync_end >> 8);
	hdmi_write(HDMI_V_SYNC_LINE_BEF_1_0, v_sync_line_bef_1 % 256);
	hdmi_write(HDMI_V_SYNC_LINE_BEF_1_1, v_sync_line_bef_1 >> 8);
	hdmi_write(HDMI_V_SYNC_LINE_BEF_2_0, v_sync_line_bef_2 % 256);
	hdmi_write(HDMI_V_SYNC_LINE_BEF_2_1, v_sync_line_bef_2 >> 8);

	/* Set V_SYNC_LINE_AFT*, V_SYNC_LINE_AFT_PXL*, VACT_SPACE* */
	hdmi_write(HDMI_V_SYNC_LINE_AFT_1_0, fixed_ffff % 256);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_1_1, fixed_ffff >> 8);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_2_0, fixed_ffff % 256);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_2_1, fixed_ffff >> 8);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_3_0, fixed_ffff % 256);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_3_1, fixed_ffff >> 8);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_4_0, fixed_ffff % 256);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_4_1, fixed_ffff >> 8);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_5_0, fixed_ffff % 256);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_5_1, fixed_ffff >> 8);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_6_0, fixed_ffff % 256);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_6_1, fixed_ffff >> 8);

	hdmi_write(HDMI_V_SYNC_LINE_AFT_PXL_1_0, fixed_ffff % 256);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_PXL_1_1, fixed_ffff >> 8);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_PXL_2_0, fixed_ffff % 256);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_PXL_2_1, fixed_ffff >> 8);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_PXL_3_0, fixed_ffff % 256);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_PXL_3_1, fixed_ffff >> 8);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_PXL_4_0, fixed_ffff % 256);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_PXL_4_1, fixed_ffff >> 8);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_PXL_5_0, fixed_ffff % 256);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_PXL_5_1, fixed_ffff >> 8);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_PXL_6_0, fixed_ffff % 256);
	hdmi_write(HDMI_V_SYNC_LINE_AFT_PXL_6_1, fixed_ffff >> 8);

	hdmi_write(HDMI_VACT_SPACE_1_0, fixed_ffff % 256);
	hdmi_write(HDMI_VACT_SPACE_1_1, fixed_ffff >> 8);
	hdmi_write(HDMI_VACT_SPACE_2_0, fixed_ffff % 256);
	hdmi_write(HDMI_VACT_SPACE_2_1, fixed_ffff >> 8);
	hdmi_write(HDMI_VACT_SPACE_3_0, fixed_ffff % 256);
	hdmi_write(HDMI_VACT_SPACE_3_1, fixed_ffff >> 8);
	hdmi_write(HDMI_VACT_SPACE_4_0, fixed_ffff % 256);
	hdmi_write(HDMI_VACT_SPACE_4_1, fixed_ffff >> 8);
	hdmi_write(HDMI_VACT_SPACE_5_0, fixed_ffff % 256);
	hdmi_write(HDMI_VACT_SPACE_5_1, fixed_ffff >> 8);
	hdmi_write(HDMI_VACT_SPACE_6_0, fixed_ffff % 256);
	hdmi_write(HDMI_VACT_SPACE_6_1, fixed_ffff >> 8);

	hdmi_write(HDMI_CSC_MUX, 0x0);
	hdmi_write(HDMI_SYNC_GEN_MUX, 0x0);

	hdmi_write(HDMI_SEND_START_0, 0xfd);
	hdmi_write(HDMI_SEND_START_1, 0x01);
	hdmi_write(HDMI_SEND_END_0, 0x0d);
	hdmi_write(HDMI_SEND_END_1, 0x3a);
	hdmi_write(HDMI_SEND_END_2, 0x08);

	/* Set DC_CONTROL to 0x00 */
	hdmi_write(HDMI_DC_CONTROL, 0x0);

	/* Set VIDEO_PATTERN_GEN to 0x00 */
	hdmi_write(HDMI_VIDEO_PATTERN_GEN, 0x0);
	/*hdmi_write(HDMI_VIDEO_PATTERN_GEN, 0x1); */
	/*hdmi_write(HDMI_VIDEO_PATTERN_GEN, 0x3);, internal */

	hdmi_write(HDMI_GCP_CON, 0x0a);
}

static void hdmi_stop_vsi(void)
{
	hdmi_writeb(HDMI_VSI_CON, HDMI_VSI_CON_DO_NOT_TRANSMIT);
}

static u8 hdmi_chksum(u32 start, u8 len, u32 hdr_sum)
{
	int i;

	/* hdr_sum : header0 + header1 + header2
	 * start : start address of packet byte1
	 * len : packet bytes - 1 */
	for (i = 0; i < len; ++i)
		hdr_sum += hdmi_read(start + i * 4);

	return (u8) (0x100 - (hdr_sum & 0xff));
}

static inline bool hdmi_valid_ratio_4_3(const struct hdmi_res_mode *mode)
{
	if ((mode->h_as == 720 &&
	     mode->v_as == 480 &&
	     mode->pixelclock == 27000000) ||
	    (mode->h_as == 720 &&
	     mode->v_as == 480 && mode->pixelclock == 27027000))
		return true;

	return false;
}

static void hdmi_reg_infoframe(const struct hdmi_conf *conf,
			       union hdmi_infoframe *infoframe,
			       const struct hdmi_format *format)
{
	const struct hdmi_preset *preset = conf->preset;
	bool dvi_mode = conf->preset->dvi_mode;
	u32 hdr_sum;
	u8 chksum;
	u32 aspect_ratio;
	u32 vic;

	pr_debug("%s: infoframe type = 0x%x, %s\n", __func__,
		infoframe->any.type, dvi_mode ? "dvi monitor" : "hdmi monitor");

	if (dvi_mode) {
		hdmi_writeb(HDMI_VSI_CON, HDMI_VSI_CON_DO_NOT_TRANSMIT);
		hdmi_writeb(HDMI_AVI_CON, HDMI_AVI_CON_DO_NOT_TRANSMIT);
		hdmi_write(HDMI_AUI_CON, HDMI_AUI_CON_NO_TRAN);
		return;
	}

	switch (infoframe->any.type) {
	case HDMI_INFOFRAME_TYPE_VENDOR:
		hdmi_writeb(HDMI_VSI_CON, HDMI_VSI_CON_EVERY_VSYNC);
		hdmi_writeb(HDMI_VSI_HEADER0, infoframe->any.type);
		hdmi_writeb(HDMI_VSI_HEADER1, infoframe->any.version);

		/* 0x000C03 : 24bit IEEE Registration Identifier */
		hdmi_writeb(HDMI_VSI_DATA(1), 0x03);
		hdmi_writeb(HDMI_VSI_DATA(2), 0x0c);
		hdmi_writeb(HDMI_VSI_DATA(3), 0x00);
		hdmi_writeb(HDMI_VSI_DATA(4),
			    HDMI_VSI_DATA04_VIDEO_FORMAT(format->vformat));
		hdmi_writeb(HDMI_VSI_DATA(5),
			    HDMI_VSI_DATA05_3D_STRUCTURE(format->type_3d));

		if (format->type_3d == HDMI_3D_TYPE_SB_HALF) {
			infoframe->any.length += 1;
			hdmi_writeb(HDMI_VSI_DATA(6),
				    (u8)
				    HDMI_VSI_DATA06_3D_EXT_DATA
				    (HDMI_H_SUB_SAMPLE));
		}

		hdmi_writeb(HDMI_VSI_HEADER2, infoframe->any.length);
		hdr_sum =
		    infoframe->any.type + infoframe->any.version +
		    infoframe->any.length;
		chksum =
		    hdmi_chksum(HDMI_VSI_DATA(1), infoframe->any.length,
				hdr_sum);

		pr_debug("%s: VSI checksum = 0x%x\n", __func__, chksum);
		hdmi_writeb(HDMI_VSI_DATA(0), chksum);
		break;

	case HDMI_INFOFRAME_TYPE_AVI:
		hdmi_writeb(HDMI_AVI_CON, HDMI_AVI_CON_EVERY_VSYNC);
		hdmi_writeb(HDMI_AVI_HEADER0, infoframe->any.type);
		hdmi_writeb(HDMI_AVI_HEADER1, infoframe->any.version);
		hdmi_writeb(HDMI_AVI_HEADER2, infoframe->any.length);
		hdr_sum = infoframe->any.type +
		    infoframe->any.version + infoframe->any.length;

		hdmi_writeb(HDMI_AVI_BYTE(1), HDMI_OUTPUT_RGB888 << 5 |
			    AVI_ACTIVE_FORMAT_VALID |
			    AVI_UNDERSCANNED_DISPLAY_VALID);

		if (preset->aspect_ratio == HDMI_PICTURE_ASPECT_4_3 &&
		    hdmi_valid_ratio_4_3(&preset->mode)) {
			aspect_ratio = HDMI_AVI_PICTURE_ASPECT_4_3;
			/* 17 : 576P50Hz 4:3 aspect ratio */
			vic = 17;
		} else {
			aspect_ratio = HDMI_AVI_PICTURE_ASPECT_16_9;
			vic = preset->vic;
		}

		hdmi_writeb(HDMI_AVI_BYTE(2), preset->aspect_ratio |
			    AVI_SAME_AS_PIC_ASPECT_RATIO | AVI_ITU709);

		if (preset->color_range == 0 || preset->color_range == 2)
			hdmi_writeb(HDMI_AVI_BYTE(3), AVI_FULL_RANGE);
		else
			hdmi_writeb(HDMI_AVI_BYTE(3), AVI_LIMITED_RANGE);

		hdmi_writeb(HDMI_AVI_BYTE(4), vic);
		chksum = hdmi_chksum(HDMI_AVI_BYTE(1),
				     infoframe->any.length, hdr_sum);

		pr_debug("%s: AVI checksum = 0x%x\n", __func__, chksum);
		hdmi_writeb(HDMI_AVI_CHECK_SUM, chksum);
		break;

	case HDMI_INFOFRAME_TYPE_AUDIO:
		hdmi_write(HDMI_AUI_CON, HDMI_AUI_CON_TRANS_EVERY_VSYNC);
		hdmi_writeb(HDMI_AUI_HEADER0, infoframe->any.type);
		hdmi_writeb(HDMI_AUI_HEADER1, infoframe->any.version);
		hdmi_writeb(HDMI_AUI_HEADER2, infoframe->any.length);

#ifdef SPEAKER_PLACEMENT
		/* speaker placement */
		if (audio_channel_count == 6)
			hdmi_writeb(HDMI_AUI_BYTE(4), 0x0b);
		else if (audio_channel_count == 8)
			hdmi_writeb(HDMI_AUI_BYTE(4), 0x13);
		else
			hdmi_writeb(HDMI_AUI_BYTE(4), 0x00);
#endif
		hdr_sum = infoframe->any.type + infoframe->any.version +
		    infoframe->any.length;
		chksum = hdmi_chksum(HDMI_AUI_BYTE(1),
				     infoframe->any.length, hdr_sum);
		pr_debug("%s: AUI checksum = 0x%x\n", __func__, chksum);
		hdmi_writeb(HDMI_AUI_CHECK_SUM, chksum);
		break;

	default:
		pr_err("%s: unknown type(0x%x)\n",
		       __func__, infoframe->any.type);
		break;
	}
}

static void hdmi_infoframe_set(const struct hdmi_conf *conf)
{
	union hdmi_infoframe infoframe;
	const struct hdmi_format *format;

	format = conf->format;

	pr_debug("%s: format [%s]\n", __func__,
		 format->vformat == HDMI_VIDEO_FORMAT_3D ? "3D" : "2D");

	/* vendor infoframe */
	if (format->vformat != HDMI_VIDEO_FORMAT_3D) {
		hdmi_stop_vsi();
	} else {
		infoframe.any.type = HDMI_INFOFRAME_TYPE_VENDOR;
		infoframe.any.version = HDMI_VSI_VERSION;
		infoframe.any.length = HDMI_VSI_LENGTH;
		hdmi_reg_infoframe(conf, &infoframe, format);
	}

	/* avi infoframe */
	infoframe.any.type = HDMI_INFOFRAME_TYPE_AVI;
	infoframe.any.version = HDMI_AVI_VERSION;
	infoframe.any.length = HDMI_AVI_LENGTH;
	hdmi_reg_infoframe(conf, &infoframe, format);

	/* audio infoframe */
	infoframe.any.type = HDMI_INFOFRAME_TYPE_AUDIO;
	infoframe.any.version = HDMI_AUI_VERSION;
	infoframe.any.length = HDMI_AUI_LENGTH;
	hdmi_reg_infoframe(conf, &infoframe, format);
}

static void hdmi_audio_enable(bool on)
{
	if (on)
		hdmi_write_mask(HDMI_CON_0, ~0, HDMI_ASP_ENABLE);
	else
		hdmi_write_mask(HDMI_CON_0, 0, HDMI_ASP_ENABLE);
}

static void hdmi_set_acr(int sample_rate, bool dvi_mode)
{
	u32 n, cts;

	pr_debug("%s %s\n",
		 __func__, dvi_mode ? "dvi monitor" : "hdmi monitor");

	if (dvi_mode) {
		hdmi_write(HDMI_ACR_CON, HDMI_ACR_CON_TX_MODE_NO_TX);
		return;
	}

	if (sample_rate == 32000) {
		n = 4096;
		cts = 27000;
	} else if (sample_rate == 44100) {
		n = 6272;
		cts = 30000;
	} else if (sample_rate == 48000) {
		n = 6144;
		cts = 27000;
	} else if (sample_rate == 88200) {
		n = 12544;
		cts = 30000;
	} else if (sample_rate == 96000) {
		n = 12288;
		cts = 27000;
	} else if (sample_rate == 176400) {
		n = 25088;
		cts = 30000;
	} else if (sample_rate == 192000) {
		n = 24576;
		cts = 27000;
	} else {
		n = 0;
		cts = 0;
	}

	hdmi_write(HDMI_ACR_N0, HDMI_ACR_N0_VAL(n));
	hdmi_write(HDMI_ACR_N1, HDMI_ACR_N1_VAL(n));
	hdmi_write(HDMI_ACR_N2, HDMI_ACR_N2_VAL(n));

	/* transfer ACR packet */
	hdmi_write(HDMI_ACR_CON, HDMI_ACR_CON_TX_MODE_MESURED_CTS);
}

void hdmi_spdif_init(int audio_codec, int bits_per_sample)
{
	u32 val;
	int bps, rep_time;

	hdmi_write(HDMI_I2S_CLK_CON, HDMI_I2S_CLK_ENABLE);

	val = HDMI_SPDIFIN_CFG_NOISE_FILTER_2_SAMPLE |
	    HDMI_SPDIFIN_CFG_PCPD_MANUAL |
	    HDMI_SPDIFIN_CFG_WORD_LENGTH_MANUAL |
	    HDMI_SPDIFIN_CFG_UVCP_REPORT |
	    HDMI_SPDIFIN_CFG_HDMI_2_BURST | HDMI_SPDIFIN_CFG_DATA_ALIGN_32;

	hdmi_write(HDMI_SPDIFIN_CONFIG_1, val);
	hdmi_write(HDMI_SPDIFIN_CONFIG_2, 0);

	bps = audio_codec == HDMI_AUDIO_PCM ? bits_per_sample : 16;
	rep_time = audio_codec == HDMI_AUDIO_AC3 ? 1536 * 2 - 1 : 0;
	val = HDMI_SPDIFIN_USER_VAL_REPETITION_TIME_LOW(rep_time) |
	    HDMI_SPDIFIN_USER_VAL_WORD_LENGTH_24;
	hdmi_write(HDMI_SPDIFIN_USER_VALUE_1, val);
	val = HDMI_SPDIFIN_USER_VAL_REPETITION_TIME_HIGH(rep_time);
	hdmi_write(HDMI_SPDIFIN_USER_VALUE_2, val);
	hdmi_write(HDMI_SPDIFIN_USER_VALUE_3, 0);
	hdmi_write(HDMI_SPDIFIN_USER_VALUE_4, 0);

	val = HDMI_I2S_IN_ENABLE | HDMI_I2S_AUD_SPDIF | HDMI_I2S_MUX_ENABLE;
	hdmi_write(HDMI_I2S_IN_MUX_CON, val);

	hdmi_write(HDMI_I2S_MUX_CH, HDMI_I2S_CH_ALL_EN);
	hdmi_write(HDMI_I2S_MUX_CUV, HDMI_I2S_CUV_RL_EN);

	hdmi_write_mask(HDMI_SPDIFIN_CLK_CTRL, 0, HDMI_SPDIFIN_CLK_ON);
	hdmi_write_mask(HDMI_SPDIFIN_CLK_CTRL, ~0, HDMI_SPDIFIN_CLK_ON);

	hdmi_write(HDMI_SPDIFIN_OP_CTRL, HDMI_SPDIFIN_STATUS_CHECK_MODE);
	hdmi_write(HDMI_SPDIFIN_OP_CTRL, HDMI_SPDIFIN_STATUS_CHECK_MODE_HDMI);
}

static void hdmi_audio_init(const struct hdmi_conf *conf)
{
	u32 sample_rate, bits_per_sample;
	u32 audio_codec;

	sample_rate = DEFAULT_SAMPLE_RATE;
	bits_per_sample = DEFAULT_BITS_PER_SAMPLE;
	audio_codec = DEFAULT_AUDIO_CODEC;

	hdmi_set_acr(sample_rate, conf->preset->dvi_mode);
	hdmi_spdif_init(audio_codec, bits_per_sample);
}

void hdmi_dvi_mode_set(bool dvi_mode)
{
	u32 val;

	pr_debug("%s %s\n",
		 __func__, dvi_mode ? "dvi monitor" : "hdmi monitor");

	hdmi_write_mask(HDMI_MODE_SEL, dvi_mode ? HDMI_MODE_DVI_EN :
			HDMI_MODE_HDMI_EN, HDMI_MODE_MASK);

	if (dvi_mode)
		val = HDMI_VID_PREAMBLE_DIS | HDMI_GUARD_BAND_DIS;
	else
		val = HDMI_VID_PREAMBLE_EN | HDMI_GUARD_BAND_EN;

	hdmi_write(HDMI_CON_2, val);
}

static inline void hdmi_enable(const struct hdmi_conf *conf, bool on)
{
	const struct hdmi_preset *preset = conf->preset;
	const struct hdmi_res_mode *mode = &preset->mode;

	int h_as = mode->h_as;
	int h_sw = mode->h_sw;
	int h_bp = mode->h_bp;
	int v_as = mode->v_as;
	int v_sw = mode->v_sw;
	int v_bp = mode->v_bp;
	int h_s_offs = 0;

	int v_sync_s = v_sw + v_bp + v_as - 1;
	int h_active_s = h_sw + h_bp + h_s_offs;
	int h_active_e = h_as + h_sw + h_bp + h_s_offs;
	int v_sync_hs_se0 = h_sw + h_bp + 1 + h_s_offs;
	int v_sync_hs_se1 = v_sync_hs_se0 + 1;

	pr_debug("%s : %s %s\n",
		__func__, mode->name, on ? "on" : "off");

	if (!on) {
		hdmi_write(HDMI_CON_0, hdmi_read(HDMI_CON_0) & ~0x01);
		return;
	}

	hdmi_write(HDMI_CON_0, hdmi_read(HDMI_CON_0) | 0x01);
	msleep(20);

	nx_disp_top_hdmi_set_vsync_start(v_sync_s);
	nx_disp_top_hdmi_set_hactive_start(h_active_s);
	nx_disp_top_hdmi_set_hactive_end(h_active_e);
	nx_disp_top_hdmi_set_vsync_hsstart_end(v_sync_hs_se0, v_sync_hs_se1);
}

int hdmi_find_mode(struct videomode *vm, int refresh,
		   int pixelclock, unsigned int flags)
{
	const struct hdmi_conf *conf;
	int size = num_hdmi_presets;
	int i;

	pr_debug("[%s] Search hac=%4d, vac=%4d, %2d fps, %dhz [array:%d]\n",
		 __func__, vm->hactive, vm->vactive, refresh, pixelclock, size);

	conf = hdmi_conf;

	for (i = 0; size > i; i++) {
		const struct hdmi_preset *preset = conf[i].preset;
		const struct hdmi_res_mode *mode = &preset->mode;

		if (false == conf[i].support)
			continue;

		if (mode->h_as != vm->hactive || mode->v_as != vm->vactive)
			continue;

		if ((flags & HDMI_CHECK_REFRESH) && (mode->refresh != refresh))
			continue;

		if ((flags & HDMI_CHECK_PIXCLOCK) &&
		    (mode->pixelclock != pixelclock))
			continue;

		if (0 == mode->pixelclock)
			continue;

		pr_debug("[%s] Ok Find %2d %s ha=%4d, va=%4d, %2d fps, %dhz\n",
			 __func__, i, mode->name, mode->h_as, mode->v_as,
			 mode->refresh, mode->pixelclock);
		return i;
	}

	pr_debug("[%s] Not Find !\n", __func__);
	return -EINVAL;
}

bool nx_dp_hdmi_is_connected(void)
{
	int state = hdmi_hpd_status();

	pr_debug("%s: %s\n", __func__, state ? "connected" : "disconnected");

	return state ? true : false;
}

static void nx_dp_hdmi_set_base(struct dp_control_dev *dpc,
				void __iomem *base)
{
	u32 mask = (1 << 6) | (1 << 3) | (1 << 2);

	hdmi_set_base(base);

	/* HPD interrupt control: INTC_CON */
	hdmi_write(HDMI_INTC_CON_0, mask);
}

u32 nx_dp_hdmi_hpd_event(int irq)
{
	u32 flags, event = 0;

	flags = hdmi_read(HDMI_INTC_FLAG_0);

	pr_debug("%s: flags 0x%x\n", __func__, flags);

	if (flags & HDMI_INTC_FLAG_HPD_UNPLUG) {
		hdmi_write_mask(HDMI_INTC_FLAG_0, ~0,
				HDMI_INTC_FLAG_HPD_UNPLUG);
		event |= HDMI_EVENT_UNPLUG;
		pr_debug("%s: UNPLUG\n", __func__);
	}

	if (flags & HDMI_INTC_FLAG_HPD_PLUG) {
		hdmi_write_mask(HDMI_INTC_FLAG_0, ~0, HDMI_INTC_FLAG_HPD_PLUG);
		event |= HDMI_EVENT_PLUG;
		pr_debug("%s: PLUG\n", __func__);
	}

	if (flags & HDMI_INTC_FLAG_HDCP) {
		event |= HDMI_EVENT_HDCP;
		pr_debug("%s: hdcp not implenent !\n", __func__);
	}

	return event;
}

bool nx_dp_hdmi_mode_valid(struct videomode *vm, int refresh, int pixelclock)
{
	unsigned int flags = HDMI_CHECK_REFRESH | HDMI_CHECK_PIXCLOCK;
	int err;

	err = hdmi_find_mode(vm, refresh, pixelclock, flags);

	return 0 > err ? false : true;
}

bool nx_dp_hdmi_mode_get(int width, int height, int refresh,
			 struct videomode *vm)
{
	const struct hdmi_conf *conf;
	const struct hdmi_res_mode *mode;
	unsigned int flags = HDMI_CHECK_REFRESH;
	int err;

	vm->hactive = width;
	vm->vactive = height;

	if (!width || !height || !refresh)
		return false;

	err = hdmi_find_mode(vm, refresh, 0, flags);
	if (0 > err)
		return false;

	conf = &hdmi_conf[err];
	mode = &conf->preset->mode;

	vm->pixelclock = mode->pixelclock;
	vm->hactive = mode->h_as;
	vm->hfront_porch = mode->h_fp;
	vm->hback_porch = mode->h_bp;
	vm->hsync_len = mode->h_sw;
	vm->vactive = mode->v_as;
	vm->vfront_porch = mode->v_fp;
	vm->vback_porch = mode->v_bp;
	vm->vsync_len = mode->v_sw;

	return true;
}

static void hdmi_mode_to_display_mode(struct hdmi_res_mode *hm,
			struct drm_display_mode *dmode)
{
	dmode->hdisplay = hm->h_as;
	dmode->hsync_start = dmode->hdisplay + hm->h_fp;
	dmode->hsync_end = dmode->hsync_start + hm->h_sw;
	dmode->htotal = dmode->hsync_end + hm->h_bp;

	dmode->vdisplay = hm->v_as;
	dmode->vsync_start = dmode->vdisplay + hm->v_fp;
	dmode->vsync_end = dmode->vsync_start + hm->v_sw;
	dmode->vtotal = dmode->vsync_end + hm->v_bp;

}

int nx_dp_hdmi_mode_set(struct nx_drm_device *display,
			struct drm_display_mode *mode, struct videomode *vm,
			bool dvi_mode)
{
	struct dp_hdmi_dev *out;
	const struct hdmi_conf *conf;
	struct hdmi_preset *preset;
	struct dp_control_dev *dpc = display_to_dpc(display);
	unsigned int flags = HDMI_CHECK_REFRESH | HDMI_CHECK_PIXCLOCK;
	int refresh = mode->vrefresh;
	int pixelclock = mode->clock * 1000;
	int err;

	BUG_ON(!dpc);

	drm_display_mode_to_videomode(mode, vm);

	pr_debug("%s %s\n",
		 __func__, dvi_mode ? "dvi monitor" : "hdmi monitor");

	err = hdmi_find_mode(vm, refresh, pixelclock, flags);
	if (0 > err) {
		pr_err("%s: not found vm mode !\n", __func__);
		return -ENODEV;
	}

	conf = &hdmi_conf[err];

	preset = (struct hdmi_preset *)conf->preset;
	preset->aspect_ratio = HDMI_PICTURE_ASPECT_16_9;
	preset->dvi_mode = dvi_mode;

	out = dpc->dp_output;
	out->preset_data = (void *)conf;

	/*
	 * set display control config
	 */
	hdmi_dp_set(dpc, vm);

	/* set display mode values */
	hdmi_mode_to_display_mode(&preset->mode, mode);

	pr_debug("%s %s done\n", __func__, preset->mode.name);
	return 0;
}

int nx_dp_hdmi_mode_commit(struct nx_drm_device *display, int pipe)
{
	struct dp_hdmi_dev *out;
	const struct hdmi_conf *conf;
	const struct hdmi_preset *preset;
	struct dp_control_dev *dpc = display_to_dpc(display);
	struct nx_drm_res *res = &display->res;
	u32 input = 0;

	pr_debug("%s pipe.%d\n", __func__, pipe);

	out = dpc->dp_output;
	conf = out->preset_data;

	if (!conf)
		return -EINVAL;

	preset = conf->preset;
	pr_debug("%s %s\n", __func__, preset->mode.name);

	/* HDMI setup */
	hdmi_reset(res->resets, res->num_resets);

	hdmi_phy_set(conf, HDMI_PHY_TABLE_SIZE);

	if (false == hdmi_wait_phy_ready())
		return -EIO;

	switch (pipe) {
	case 0:
		input = primary_mlc;
		break;
	case 1:
		input = secondary_mlc;
		break;
	case 3:
		input = resolution_conv;
		break;
	default:
		pr_err("%s: not support input mux %d\n", __func__, input);
		return -EINVAL;
	}

	nx_disp_top_set_hdmimux(1, input);

	hdmi_clock_on();
	hdmi_standby();

	hdmi_conf_set(conf);
	hdmi_infoframe_set(conf);

	hdmi_audio_init(conf);
	hdmi_audio_enable(true);
	hdmi_dvi_mode_set(preset->dvi_mode);

	pr_debug("%s done\n", __func__);

	return 0;
}

void nx_dp_hdmi_power(struct nx_drm_device *display, bool on)
{
	struct dp_hdmi_dev *out;
	const struct hdmi_conf *conf;
	struct dp_control_dev *dpc = display_to_dpc(display);
	bool dvi_mode;

	pr_debug("%s %s\n", __func__, on ? "on" : "off");

	out = dpc->dp_output;
	conf = out->preset_data;

	if (!conf || !conf->preset)
		return;

	dvi_mode = conf->preset->dvi_mode;

	if (on)
		hdmi_enable(conf, true);
	else
		hdmi_enable(conf, false);

	if (on && dvi_mode)
		hdmi_write(HDMI_GCP_CON, HDMI_GCP_CON_NO_TRAN);
}

static struct dp_control_ops hdmi_dp_ops = {
	.set_base = nx_dp_hdmi_set_base,
};

int nx_soc_dp_hdmi_register(struct device *dev,
			struct device_node *np, struct dp_control_dev *dpc)
{
	struct dp_hdmi_dev *out;

	out = devm_kzalloc(dev, sizeof(*out), GFP_KERNEL);
	if (IS_ERR(out))
		return -ENOMEM;

	dpc->panel_type = dp_panel_type_hdmi;
	dpc->dp_output = out;
	dpc->ops = &hdmi_dp_ops;

	return 0;
}

