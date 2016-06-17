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
#ifndef _S5PXX18_DP_HDMI_H_
#define _S5PXX18_DP_HDMI_H_

#include <drm/drmP.h>
#include <video/videomode.h>

#include "s5pxx18_drm_dp.h"

struct hdmi_reg_tg {
	u8 cmd;
	u8 h_fsz_l;
	u8 h_fsz_h;
	u8 hact_st_l;
	u8 hact_st_h;
	u8 hact_sz_l;
	u8 hact_sz_h;
	u8 v_fsz_l;
	u8 v_fsz_h;
	u8 vsync_l;
	u8 vsync_h;
	u8 vsync2_l;
	u8 vsync2_h;
	u8 vact_st_l;
	u8 vact_st_h;
	u8 vact_sz_l;
	u8 vact_sz_h;
	u8 field_chg_l;
	u8 field_chg_h;
	u8 vact_st2_l;
	u8 vact_st2_h;
	u8 vact_st3_l;
	u8 vact_st3_h;
	u8 vact_st4_l;
	u8 vact_st4_h;
	u8 vsync_top_hdmi_l;
	u8 vsync_top_hdmi_h;
	u8 vsync_bot_hdmi_l;
	u8 vsync_bot_hdmi_h;
	u8 field_top_hdmi_l;
	u8 field_top_hdmi_h;
	u8 field_bot_hdmi_l;
	u8 field_bot_hdmi_h;
	u8 tg_3d;
};

struct hdmi_reg_core {
	u8 h_blank[2];
	u8 v2_blank[2];
	u8 v1_blank[2];
	u8 v_line[2];
	u8 h_line[2];
	u8 hsync_pol[1];
	u8 vsync_pol[1];
	u8 int_pro_mode[1];
	u8 v_blank_f0[2];
	u8 v_blank_f1[2];
	u8 h_sync_start[2];
	u8 h_sync_end[2];
	u8 v_sync_line_bef_2[2];
	u8 v_sync_line_bef_1[2];
	u8 v_sync_line_aft_2[2];
	u8 v_sync_line_aft_1[2];
	u8 v_sync_line_aft_pxl_2[2];
	u8 v_sync_line_aft_pxl_1[2];
	u8 v_blank_f2[2];	/* for 3D mode */
	u8 v_blank_f3[2];	/* for 3D mode */
	u8 v_blank_f4[2];	/* for 3D mode */
	u8 v_blank_f5[2];	/* for 3D mode */
	u8 v_sync_line_aft_3[2];
	u8 v_sync_line_aft_4[2];
	u8 v_sync_line_aft_5[2];
	u8 v_sync_line_aft_6[2];
	u8 v_sync_line_aft_pxl_3[2];
	u8 v_sync_line_aft_pxl_4[2];
	u8 v_sync_line_aft_pxl_5[2];
	u8 v_sync_line_aft_pxl_6[2];
	u8 vact_space_1[2];
	u8 vact_space_2[2];
	u8 vact_space_3[2];
	u8 vact_space_4[2];
	u8 vact_space_5[2];
	u8 vact_space_6[2];
};

#define	RES_FIELD_INTERLACED	(1<<0)

struct hdmi_res_mode {
	int pixelclock;
	int h_as, h_sw, h_bp, h_fp, h_si;
	int v_as, v_sw, v_bp, v_fp, v_si;
	u16 refresh;
	unsigned long flags;
	char *name;
};

struct hdmi_preset {
	struct hdmi_res_mode mode;
	enum hdmi_picture_aspect aspect_ratio;
	struct hdmi_reg_core core;
	struct hdmi_reg_tg tg;
	bool dvi_mode;
	int color_range;
	u8 vic;
};

enum hdmi_vformat {
	HDMI_VIDEO_FORMAT_2D = 0x0,
	HDMI_VIDEO_FORMAT_3D = 0x2
};

enum hdmi_3d_type {
	HDMI_3D_TYPE_FP = 0x0, /** Frame Packing */
	HDMI_3D_TYPE_TB = 0x6, /** Top-and-Bottom */
	HDMI_3D_TYPE_SB_HALF = 0x8 /** Side-by-Side Half */
};

struct hdmi_format {
	enum hdmi_vformat vformat;
	enum hdmi_3d_type type_3d;
};

struct hdmi_conf {
	const struct hdmi_preset *preset;
	const struct hdmi_format *format;
	const u8 *phy_data;
	bool support;
};

/* VENDOR header */
#define HDMI_VSI_VERSION		0x01
#define HDMI_VSI_LENGTH			0x05

/* AVI header */
#define HDMI_AVI_VERSION		0x02
#define HDMI_AVI_LENGTH			0x0d
#define AVI_UNDERSCAN			(2 << 0)
#define AVI_ACTIVE_FORMAT_VALID	(1 << 4)
#define AVI_SAME_AS_PIC_ASPECT_RATIO 0x8
#define AVI_ITU709				(2 << 6)
#define AVI_LIMITED_RANGE		(1 << 2)
#define AVI_FULL_RANGE			(2 << 2)

/* AUI header info */
#define HDMI_AUI_VERSION		0x01
#define HDMI_AUI_LENGTH			0x0a

enum HDMI_3D_EXT_DATA {
	/* refer to Table H-3 3D_Ext_Data - Additional video format
	 * information for Side-by-side(half) 3D structure */

	/** Horizontal sub-sampleing */
	HDMI_H_SUB_SAMPLE = 0x1
};

enum HDMI_OUTPUT_FMT {
	HDMI_OUTPUT_RGB888 = 0x0,
	HDMI_OUTPUT_YUV444 = 0x2
};

enum HDMI_AUDIO_CODEC {
	HDMI_AUDIO_PCM,
	HDMI_AUDIO_AC3,
	HDMI_AUDIO_MP3
};

/* HPD events */
#define	HDMI_EVENT_PLUG		(1<<0)
#define	HDMI_EVENT_UNPLUG	(1<<1)
#define	HDMI_EVENT_HDCP		(1<<2)

extern const struct hdmi_conf hdmi_conf[];
extern const int num_hdmi_presets;

int nx_soc_dp_hdmi_register(struct device *dev,
			struct device_node *np, struct dp_control_dev *dpc);

u32  nx_dp_hdmi_hpd_event(int irq);
bool nx_dp_hdmi_is_connected(void);
bool nx_dp_hdmi_mode_valid(struct videomode *vm, int refresh, int pixelclock);
bool nx_dp_hdmi_mode_get(int width, int height, int refresh,
			struct videomode *vm);
int nx_dp_hdmi_mode_set(struct nx_drm_device *display,
			struct drm_display_mode *mode, struct videomode *vm,
			bool dvi_mode);
int  nx_dp_hdmi_mode_commit(struct nx_drm_device *display, int crtc);
void nx_dp_hdmi_power(struct nx_drm_device *display, bool on);

#endif
