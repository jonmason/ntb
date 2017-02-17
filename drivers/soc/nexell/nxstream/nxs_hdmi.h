/*
 * Copyright (C) 2017  Nexell Co., Ltd.
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
#ifndef __NXS_HDMI_H__
#define __NXS_HDMI_H__

#include <linux/hdmi.h>
#include "hdmi_v2_reg.h"
#include "nxs_hdmi_phy.h"

/*
 * HDMI pattern type
 */
#define HDMI20_PTTN_TYPE_RND1			0
#define HDMI20_PTTN_TYPE_RND2			1
#define HDMI20_PTTN_TYPE_RGB24			2
#define HDMI20_PTTN_TYPE_RGB30			3
#define HDMI20_PTTN_TYPE_RGB36			4
#define HDMI20_PTTN_TYPE_GRAY24			5
#define HDMI20_PTTN_TYPE_GRAY30			6
#define HDMI20_PTTN_TYPE_GRAY36			7
#define HDMI20_PTTN_TYPE_BLACK			8
#define HDMI20_PTTN_TYPE_WHITE			9
#define HDMI20_PTTN_TYPE_01PTTN			0xA
#define HDMI20_PTTN_TYPE_ALT			0xC
#define HDMI20_PTTN_TYPE_USER			0xD

/*
 * HDMI video specific info
 */
enum hdmi_tmds_bitclk_ratio {
	HDMI_TMDS_BIT_CLOCK_RATIO_1_10,
	HDMI_TMDS_BIT_CLOCK_RATIO_1_40
};

enum hdmi_video_format {
	HDMI_VIDEO_FORMAT_2D	= 0x0,
	/** refer to Table 8-12 HDMI_Video_Format in HDMI specification v1.4a */
	HDMI_VIDEO_FORMAT_UD	= 0x1,
	HDMI_VIDEO_FORMAT_3D	= 0x2,
};

enum hdmi_3d_ext_data {
	/*
	 * refer to Table H-3 3D_Ext_Data - Additional video format
	 * information for Side-by-side (half) 3D structure
	 */
	/** Horizontal sub-sampling */
	HDMI_H_SUB_SAMPLE = 0x1
};

enum hdmi_pixel_repetition {
	HDMI_REPET_NONE,
	HDMI_REPET_PIXEL_SENT_2TIMES
};

enum hdmi_color_depth {
	HDMI_COLOR_DEPTH_24BPP,
	HDMI_COLOR_DEPTH_30BPP,
	HDMI_COLOR_DEPTH_36BPP,
	HDMI_COLOR_DEPTH_48BPP,
};

#define HDMI20_VIDEO_CTRL_DC_24BPP		0
#define HDMI20_VIDEO_CTRL_DC_30BPP		1
#define HDMI20_VIDEO_CTRL_DC_36BPP		2

struct hdmi_video_info {
	bool dvi_mode;
	enum hdmi_color_depth color_depth;
	enum hdmi_video_format format;
	enum hdmi_3d_structure struct_3d;
	enum hdmi_colorspace colorspace;
	enum hdmi_picture_aspect aspect_ratio;
	enum hdmi_colorimetry colorimetry;
	enum hdmi_extended_colorimetry ext_colorimetry;
	enum hdmi_quantization_range q_range;
};

enum hdmi_audio_input {
	HDMI_AUDIO_IN_I2S = 0,
	HDMI_AUDIO_IN_SPDIF,
	HDMI_AUDIO_IN_DMA,
};

enum hdmi_audio_dma {
	HDMI_AUDIO_DMA_NORMAL = 0,
	HDMI_AUDIO_DMA_PACKETED = 1,
};

enum hdmi_audio_packet {
	HDMI_AUDIO_PACKET_ASP, /** Audio Sample Packet Type */
	HDMI_AUDIO_PACKET_HBR, /** High Bit Rate Packet Type */
	HDMI_AUDIO_PACKET_MSA /** Multi-Stream Audio Sample Packet Type */
};

enum hdmi_audio_bit_length {
	HDMI_AUDIO_BIT_LEN_16 = 0,
	HDMI_AUDIO_BIT_LEN_17,
	HDMI_AUDIO_BIT_LEN_18,
	HDMI_AUDIO_BIT_LEN_19,
	HDMI_AUDIO_BIT_LEN_20,
	HDMI_AUDIO_BIT_LEN_21,
	HDMI_AUDIO_BIT_LEN_22,
	HDMI_AUDIO_BIT_LEN_23,
	HDMI_AUDIO_BIT_LEN_24
};

enum hdmi_audio_bits_per_ch {
	HDMI_AUDIO_BIT_PER_CH_16 = 0,
	HDMI_AUDIO_BIT_PER_CH_24 = 1,
};

struct hdmi_audio_info {
	bool dvi_mode;
	enum hdmi_audio_input input;
	enum hdmi_audio_dma audio_dma;
	enum hdmi_audio_packet packet_type;
	enum hdmi_audio_sample_frequency sample_rate;
	enum hdmi_audio_bit_length bit_length;
	enum hdmi_audio_bits_per_ch bits_per_ch;
	enum hdmi_audio_coding_type coding;
	int channels_num;
};

enum hdmi_hdcp_mode {
	HDMI_HDCP_NONE = 0,
	HDMI_HDCP_1P4 = 1,
	HDMI_HDCP_2P2 = 2,
};

enum hdcp14_event {
	HDCP14_EVENT_START = (1 << 0),
	HDCP14_EVENT_AN_READY = (1 << 1),
	HDCP14_EVENT_CHECK_RI = (1 << 2),
	HDCP14_EVENT_SECOND_AUTH = (1 << 3)
};

/*
 * HDMI infoframe specific
 */
struct __hdmi_infoframe {
	/* packet header */
	union {
		u8 hb[3];
		struct { u8 hb0, hb1, hb2; };
	};
	/* packet contents */
	union {
		u8 pb[28];
		struct {
		u8 pb00, pb01, pb02, pb03;
		u8 pb04, pb05, pb06, pb07;
		u8 pb08, pb09, pb10, pb11;
		u8 pb12, pb13, pb14, pb15;
		u8 pb16, pb17, pb18, pb19;
		u8 pb20, pb21, pb22, pb23;
		u8 pb24, pb25, pb26, pb27;
		};
	};
};

#define HDMI_PACKET_HEADER_LENGTH	3
#define HDMI_PACKET_CHECKSUM_LENGTH	1
#define HDMI_PACKET_LENGTH_MAX		27

#define HDMI_AVI_VERSION		0x02
#define HDMI_AUI_VERSION		0x01
#define HDMI_VSI_VERSION		0x01

#define HDMI_AVI_LENGTH			13
#define HDMI_AUI_LENGTH			10
#define HDMI_H14B_VSI_LENGTH		5
#define HDMI_HF_VSI_LENGTH		6

/* find the rightmost bit (lowest bit that is set) */
#define HDMI20_INFOFRAME_HB2_LEN_MASK		0x1F
#define HDMI20_AVI_PB1_CS_FMT_MASK		0xE0
#define HDMI20_AVI_PB1_CS_FMT_MASK		0xE0
#define HDMI20_AVI_PB2_CM_MASK			0xC0
#define HDMI20_AVI_PB3_QT_MASK			0x0C
#define HDMI20_AVI_PB3_EC_MASK			0x70
#define HDMI20_AVI_PB3_QR_MASK			0xC0
#define HDMI20_AVI_PB2_ASPECT_RATIO_MASK	0x30
#define HDMI20_AVI_PB4_VIC_MASK			0xFF
#define HDMI20_AVI_PB5_PR_MASK			0x0F
#define HDMI20_AVI_PB5_QR_MASK			0xC0

#define HDMI20_AUI_PB1_CC_MASK			0x07
#define HDMI20_AUI_PB1_CT_MASK			0xF0
#define HDMI20_AUI_PB2_SF_MASK			0x28
#define HDMI20_AUI_PB4_CA_MASK			0xFF

#define HDMI20_VSI_H14B_PB4_VIDEO_FMT_MASK	0xE0
#define HDMI20_VSI_H14B_PB5_3D_MASK		0xF0
#define HDMI20_VSI_H14B_PB5_VIC_MASK		0xFF
#define HDMI20_VSI_H14B_PB6_3D_EXT_MASK		0xF0

#define HDMI20_VSI_HF_PB4_VERSION_MASK		0xFF
#define HDMI20_VSI_HF_PB5_3D_VALID_MASK		0x01
#define HDMI20_VSI_HF_PB6_3D_F_STRUCT_MASK	0xF0
#define HDMI20_VSI_HF_PB7_3D_F_EXT_MASK		0xF0

#define RIGHTMOSTBIT(_m)	(~(_m << 1) & (_m))
#define SET_MASK(_v, _m)	((_v) * RIGHTMOSTBIT(_m) & (_m))

#endif
