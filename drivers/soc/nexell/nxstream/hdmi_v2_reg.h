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
#ifndef __HDMI_V2_REG_H__
#define __HDMI_V2_REG_H__

#include "nxs_syncgen_reg.h"

/*
 * @ NXS HDMI VPS
 * - BASE : 20C30000
 *
 * @ refer to __nx_cpuif_nx_nxs_to_hdmi_regmap_struct__
 */
struct nxs_hdmi_syncgen_reg {
	struct nxs_syncgen_reg0 syncgen;

	/* 0x100 ~ 0xA8 : Reserved */
	u32 reserved_0[(0x100 - 0xA4)/4 - 1];

	/* 0x100: HDMI RGB Control Register 1 */
	struct {
		u32 fieldmask : 4;			/* bp: 28 */
		u32 rgb1_b_cb_comp_sel : 4;		/* bp: 24 */
		u32 rgb1_g_y_comp_sel : 4;		/* bp: 20 */
		u32 rgb1_r_cr_comp_sel : 4;		/* bp: 16 */
		u32 rgb0_b_cb_comp_sel : 4;		/* bp: 12 */
		u32 rgb0_g_y_comp_sel : 4;		/* bp:  8 */
		u32 rgb0_r_cr_comp_sel : 4;		/* bp:  4 */
		u32 pre_enb_autoclear : 1;		/* bp:  3 */
		u32 outfield : 2;			/* bp:  0 */
	} compsel0;

	/* 0x104 : HDMI RGB Control Register 2 */
	struct {
		u32 rgb1_b_cb_comp_sel_line1 : 4;	/* bp: 24 */
		u32 rgb1_g_y_comp_sel_line1 : 4;	/* bp: 20 */
		u32 rgb1_r_cr_comp_sel_line1 : 4;	/* bp: 16 */
		u32 rgb0_b_cb_comp_sel_line1 : 4;	/* bp: 12 */
		u32 rgb0_g_y_comp_sel_line1 : 4;	/* bp:  8 */
		u32 rgb0_r_cr_comp_sel_line1 : 4;	/* bp:  4 */
		u32 rgb_expand : 1;			/* bp:  1 */
		u32 rgb_comp_line1_enb : 1;		/* bp:  0 */
	} compsel1;

	/* 0x108: HDMI PACKET_EN Control Register 0 */
	struct {
		u32 packet_vcnt_offset : 2;	/* bp: 16 */
		u32 packet_clr_hcnt : 16;	/* bp:  0 */
	} packeten0;

	/* 0x10c: HDMI PACKET_EN Control Register 1 */
	struct {
		u32 packet_1_vcnt_offset : 2;	/* bp: 16 */
		u32 packet_1_clr_hcnt : 16;	/* bp:  0 */
	} packeten1;
};

/*
 * @ HDMI-LINK
 * - BASE : 20F00000
 *
 * @ refer to __nx_cpuif_nx_hdmi_regmap_struct__
 */
#define HDMI_VERSION				0x0000	/* BS: 0 BW: 32	*/
#define HDMI_DBG_VIDEO_UNALIGN			0x0008	/* BS: 7 BW: 1	*/
#define HDMI_HPD				0x0008	/* BS: 6 BW: 1	*/
#define HDMI_HPD_FILTERED			0x0008	/* BS: 5 BW: 1	*/
#define HDMI_AESKEY_VALID			0x0008	/* BS: 4 BW: 1	*/
#define HDMI_PLL_LOCK				0x0008	/* BS: 3 BW: 1	*/
#define HDMI_PHY_CLK_RDY			0x0008	/* BS: 2 BW: 1	*/
#define HDMI_PHY_READY				0x0008	/* BS: 1 BW: 1	*/
#define HDMI_PHY_READY_FILTERED			0x0008	/* BS: 0 BW: 1	*/
#define HDMI_DEFAULT_PHASE			0x000C	/* BS: 17 BW: 1	*/
#define HDMI_INITIAL_PHASE			0x000C	/* BS: 16 BW: 1	*/
#define HDMI_GCP_SEPARATE_F			0x000C	/* BS: 15 BW: 1	*/
#define HDMI_GCP_SEPARATE_B			0x000C	/* BS: 14 BW: 1	*/
#define HDMI_AUD_SEPARATE_F			0x000C	/* BS: 13 BW: 1	*/
#define HDMI_AUD_SEPARATE_B			0x000C	/* BS: 12 BW: 1	*/
#define HDMI_DI_EVEN_EN				0x000C	/* BS: 9 BW: 1	*/
#define HDMI_DI_EVEN				0x000C	/* BS: 8 BW: 1	*/
#define HDMI_PHY_READY_GATEN			0x000C	/* BS: 7 BW: 1	*/
#define HDMI_CLKREQ_FORCEN			0x000C	/* BS: 6 BW: 1	*/
#define HDMI_HPD_FORCE				0x000C	/* BS: 5 BW: 1	*/
#define HDMI_INTERNAL_PTTN			0x000C	/* BS: 4 BW: 1	*/
#define HDMI_SSCP_COND				0x000C	/* BS: 2 BW: 2	*/
#define HDMI_SCRAMBLER_EN			0x000C	/* BS: 1 BW: 1	*/
#define HDMI_HDMI_EN				0x000C	/* BS: 0 BW: 1	*/
#define HDMI_PIXEL_LIMIT_FORMAT			0x0010	/* BS: 12 BW: 2	*/
#define HDMI_VSYNC_POL				0x0010	/* BS: 9 BW: 1	*/
#define HDMI_HSYNC_POL				0x0010	/* BS: 8 BW: 1	*/
#define HDMI_OESS_MODE				0x0010	/* BS: 7 BW: 1	*/
#define HDMI_NO_VPREAMBLE			0x0010	/* BS: 6 BW: 1	*/
#define HDMI_NO_VGB				0x0010	/* BS: 5 BW: 1	*/
#define HDMI_DVI_MODE				0x0010	/* BS: 4 BW: 1	*/
#define HDMI_DEEP_COLOR_MODE			0x0010	/* BS: 0 BW: 2	*/
#define HDMI_AUDIO_FIFO_RESETN			0x0014	/* BS: 4 BW: 1	*/
#define HDMI_AUDIO_FIFO_FLUSH			0x0014	/* BS: 0 BW: 1	*/
#define HDMI_PKT_CTRL				0x0018
#define HDMI_DUMMY_3_EN				0x0018	/* BS: 30 BW: 1	*/
#define HDMI_DUMMY_2_EN				0x0018	/* BS: 29 BW: 1	*/
#define HDMI_DUMMY_1_EN				0x0018	/* BS: 28 BW: 1	*/
#define HDMI_NUM_PKT				0x0018	/* BS: 20 BW: 5	*/
#define HDMI_NULL_EN				0x0018	/* BS: 19 BW: 1	*/
#define HDMI_VBI_EN				0x0018	/* BS: 18 BW: 1	*/
#define HDMI_AMPM_EN				0x0018	/* BS: 17 BW: 1	*/
#define HDMI_AMP3D_EN				0x0018	/* BS: 16 BW: 1	*/
#define HDMI_MPG_EN				0x0018	/* BS: 15 BW: 1	*/
#define HDMI_AUI_EN				0x0018	/* BS: 14 BW: 1	*/
#define HDMI_SPD_EN				0x0018	/* BS: 13 BW: 1	*/
#define HDMI_AVI_EN				0x0018	/* BS: 12 BW: 1	*/
#define HDMI_VSI_EN				0x0018	/* BS: 11 BW: 1	*/
#define HDMI_GMP_EN				0x0018	/* BS: 10 BW: 1	*/
#define HDMI_ISRC2_EN				0x0018	/* BS: 9 BW: 1	*/
#define HDMI_ISRC1_EN				0x0018	/* BS: 8 BW: 1	*/
#define HDMI_ACP_EN				0x0018	/* BS: 7 BW: 1	*/
#define HDMI_GCP_EN				0x0018	/* BS: 6 BW: 1	*/
#define HDMI_AUD_EN				0x0018	/* BS: 5 BW: 1	*/
#define HDMI_ACR_EN				0x0018	/* BS: 4 BW: 1	*/
#define HDMI_GCP_PRIORITY			0x0018	/* BS: 0 BW: 1	*/
#define HDMI_HDCP_SELECT			0x001C	/* BS: 0 BW: 1	*/
#define HDMI_ACR_CTS_HALF_EN			0x0020	/* BS: 8 BW: 1	*/
#define HDMI_ACR_CTS_MODE			0x0020	/* BS: 4 BW: 2	*/
#define HDMI_ACR_CTS_DIV4			0x0024	/* BS: 20 BW: 1	*/
#define HDMI_ACR_CTS				0x0024	/* BS: 0 BW: 20	*/
#define HDMI_ACR_N				0x0028	/* BS: 0 BW: 20	*/
#define HDMI_AUD_REQ_MAX_CNT			0x002C	/* BS: 20 BW: 5	*/
#define HDMI_AUD_CH_NUM				0x002C	/* BS: 16 BW: 4	*/
#define HDMI_AUD_PACKED_MODE			0x002C	/* BS: 13 BW: 1	*/
#define HDMI_AUD_LAYOUT				0x002C	/* BS: 12 BW: 1	*/
#define HDMI_AUD_FLAT				0x002C	/* BS: 8 BW: 4	*/
#define HDMI_AUD_TYPE				0x002C	/* BS: 4 BW: 2	*/
#define HDMI_AUD_FS_EN_STOP			0x002C	/* BS: 1 BW: 1	*/
#define HDMI_AUD_REQ_STOP			0x002C	/* BS: 0 BW: 1	*/
#define HDMI_GCP_PKT_TYPE			0x0030	/* BS: 0 BW: 8	*/
#define HDMI_GCP_DEFAULT_PHASE			0x0034	/* BS: 16 BW: 1	*/
#define HDMI_GCP_CLEAR_AVMUTE			0x0034	/* BS: 4 BW: 1	*/
#define HDMI_GCP_SET_AVMUTE			0x0034	/* BS: 0 BW: 1	*/
#define HDMI_GCP_PP				0x0034	/* BS: 12 BW: 4	*/
#define HDMI_GCP_CD				0x0034	/* BS: 8 BW: 4	*/
#define HDMI_ACP_TYPE				0x0040	/* BS: 8 BW: 8	*/
#define HDMI_ACP_PKT_TYPE			0x0040	/* BS: 0 BW: 8	*/
#define HDMI_ACP_TYPE_DEPENDENT_3_0		0x0044	/* BS: 0 BW: 32	*/
#define HDMI_ACP_TYPE_DEPENDENT_7_4		0x0048	/* BS: 0 BW: 32	*/
#define HDMI_ACP_TYPE_DEPENDENT_11_8		0x004C	/* BS: 0 BW: 32	*/
#define HDMI_ACP_TYPE_DEPENDENT_15_12		0x0050	/* BS: 0 BW: 32	*/
#define HDMI_ACP_TYPE_DEPENDENT_19_16		0x0054	/* BS: 0 BW: 32	*/
#define HDMI_ACP_TYPE_DEPENDENT_23_20		0x0058	/* BS: 0 BW: 32	*/
#define HDMI_ACP_TYPE_DEPENDENT_27_24		0x005C	/* BS: 0 BW: 32	*/
#define HDMI_ISRC_CONT				0x0060	/* BS: 15 BW: 1	*/
#define HDMI_ISRC_VALID				0x0060	/* BS: 14 BW: 1	*/
#define HDMI_ISRC_STATUS			0x0060	/* BS: 8 BW: 3	*/
#define HDMI_ISRC1_PKT_TYPE			0x0060	/* BS: 0 BW: 8	*/
#define HDMI_ISRC_UPC_EAN_ISRC_3_0		0x0064	/* BS: 0 BW: 32	*/
#define HDMI_ISRC_UPC_EAN_ISRC_7_4		0x0068	/* BS: 0 BW: 32	*/
#define HDMI_ISRC_UPC_EAN_ISRC_11_8		0x006C	/* BS: 0 BW: 32	*/
#define HDMI_ISRC_UPC_EAN_ISRC_15_12		0x0070	/* BS: 0 BW: 32	*/
#define HDMI_ISRC2_PKT_TYPE			0x0080	/* BS: 0 BW: 8	*/
#define HDMI_ISRC_UPC_EAN_ISRC_19_16		0x0084	/* BS: 0 BW: 32	*/
#define HDMI_ISRC_UPC_EAN_ISRC_23_20		0x0088	/* BS: 0 BW: 32	*/
#define HDMI_ISRC_UPC_EAN_ISRC_27_24		0x008C	/* BS: 0 BW: 32	*/
#define HDMI_ISRC_UPC_EAN_ISRC_31_28		0x0090	/* BS: 0 BW: 32	*/
#define HDMI_GMP_NO_CRNT_GBD			0x00A0	/* BS: 23 BW: 1	*/
#define HDMI_GMP_PACKET_SEQ			0x00A0	/* BS: 20 BW: 2	*/
#define HDMI_GMP_CURRENT_GAMUT_SEQ_NUM		0x00A0	/* BS: 16 BW: 4	*/
#define HDMI_GMP_NEXT_FIELD			0x00A0	/* BS: 15 BW: 1	*/
#define HDMI_GMP_GBD_PROFILE			0x00A0	/* BS: 12 BW: 3	*/
#define HDMI_GMP_AFFECTED_GAMUT_SEQ_NUM		0x00A0	/* BS: 8 BW: 4	*/
#define HDMI_GMP_PKT_TYPE			0x00A0	/* BS: 0 BW: 8	*/
#define HDMI_P0_GMP_METADATA_3_0		0x00A4	/* BS: 0 BW: 32	*/
#define HDMI_P0_GMP_METADATA_7_4		0x00A8	/* BS: 0 BW: 32	*/
#define HDMI_P0_GMP_METADATA_11_8		0x00AC	/* BS: 0 BW: 32	*/
#define HDMI_P0_GMP_METADATA_15_12		0x00B0	/* BS: 0 BW: 32	*/
#define HDMI_P0_GMP_METADATA_19_16		0x00B4	/* BS: 0 BW: 32	*/
#define HDMI_P0_GMP_METADATA_23_20		0x00B8	/* BS: 0 BW: 32	*/
#define HDMI_P0_GMP_METADATA_27_24		0x00BC	/* BS: 0 BW: 32	*/
#define HDMI_P1_GMP_METADATA_3_0		0x00C4	/* BS: 24 BW: 8	*/
#define HDMI_GMP_CHECKSUM			0x00C4	/* BS: 16 BW: 8	*/
#define HDMI_GMP_GBD_LENGTH_L			0x00C4	/* BS: 8 BW: 8	*/
#define HDMI_GMP_GBD_LENGTH_H			0x00C4	/* BS: 0 BW: 8	*/
#define HDMI_P1_1_GMP_METADATA_7_4		0x00C8	/* BS: 0 BW: 32	*/
#define HDMI_P1_1_GMP_METADATA_11_8		0x00CC	/* BS: 0 BW: 32	*/
#define HDMI_P1_1_GMP_METADATA_15_12		0x00D0	/* BS: 0 BW: 32	*/
#define HDMI_P1_1_GMP_METADATA_19_16		0x00D4	/* BS: 0 BW: 32	*/
#define HDMI_P1_1_GMP_METADATA_23_20		0x00D8	/* BS: 0 BW: 32	*/
#define HDMI_P1_1_GMP_METADATA_27_24		0x00DC	/* BS: 0 BW: 32	*/
#define HDMI_P1_2_GMP_METADATA_3_0		0x00E4	/* BS: 0 BW: 32	*/
#define HDMI_P1_2_GMP_METADATA_7_4		0x00E8	/* BS: 0 BW: 32	*/
#define HDMI_P1_2_GMP_METADATA_11_8		0x00EC	/* BS: 0 BW: 32	*/
#define HDMI_P1_2_GMP_METADATA_15_12		0x00F0	/* BS: 0 BW: 32	*/
#define HDMI_P1_2_GMP_METADATA_19_16		0x00F4	/* BS: 0 BW: 32	*/
#define HDMI_P1_2_GMP_METADATA_23_20		0x00F8	/* BS: 0 BW: 32	*/
#define HDMI_P1_2_GMP_METADATA_27_24		0x00FC	/* BS: 0 BW: 32	*/
#define HDMI_VSI_CTRL				0x0100
#define HDMI_VSI_INFOFRAME_HB2			0x0100	/* BS: 16 BW: 8	*/
#define HDMI_VSI_INFOFRAME_HB1			0x0100	/* BS: 8 BW: 8	*/
#define HDMI_VSI_PKT_TYPE			0x0100	/* BS: 0 BW: 8	*/
#define HDMI_VSI_DATA_BYTE_3_1			0x0104	/* BS: 8 BW: 24	*/
#define HDMI_VSI_DATA_BYTE_CHECKSUM		0x0104	/* BS: 0 BW: 8	*/
#define HDMI_VSI_DATA_BYTE_7_4			0x0108	/* BS: 0 BW: 32	*/
#define HDMI_VSI_DATA_BYTE_11_8			0x010C	/* BS: 0 BW: 32	*/
#define HDMI_VSI_DATA_BYTE_15_12		0x0110	/* BS: 0 BW: 32	*/
#define HDMI_VSI_DATA_BYTE_19_16		0x0114	/* BS: 0 BW: 32	*/
#define HDMI_VSI_DATA_BYTE_23_20		0x0118	/* BS: 0 BW: 32	*/
#define HDMI_VSI_DATA_BYTE_27_24		0x011C	/* BS: 0 BW: 32	*/
#define HDMI_AVI_CTRL				0x0120
#define HDMI_AVI_INFOFRAME_HB2			0x0120	/* BS: 16 BW: 8	*/
#define HDMI_AVI_INFOFRAME_HB1			0x0120	/* BS: 8 BW: 8	*/
#define HDMI_AVI_PKT_TYPE			0x0120	/* BS: 0 BW: 8	*/
#define HDMI_AVI_DATA_BYTE_3_1			0x0124	/* BS: 8 BW: 24	*/
#define HDMI_AVI_DATA_BYTE_CHECKSUM		0x0124	/* BS: 0 BW: 8	*/
#define HDMI_AVI_DATA_BYTE_7_4			0x0128	/* BS: 0 BW: 32	*/
#define HDMI_AVI_DATA_BYTE_11_8			0x012C	/* BS: 0 BW: 32	*/
#define HDMI_AVI_DATA_BYTE_15_12		0x0130	/* BS: 0 BW: 32	*/
#define HDMI_AVI_DATA_BYTE_19_16		0x0134	/* BS: 0 BW: 32	*/
#define HDMI_AVI_DATA_BYTE_23_20		0x0138	/* BS: 0 BW: 32	*/
#define HDMI_AVI_DATA_BYTE_27_24		0x013C	/* BS: 0 BW: 32	*/
#define HDMI_SPD_INFOFRAME_HB2			0x0140	/* BS: 16 BW: 8	*/
#define HDMI_SPD_INFOFRAME_HB1			0x0140	/* BS: 8 BW: 8	*/
#define HDMI_SPD_PKT_TYPE			0x0140	/* BS: 0 BW: 8	*/
#define HDMI_SPD_DATA_BYTE_3_1			0x0144	/* BS: 8 BW: 24	*/
#define HDMI_SPD_DATA_BYTE_CHECKSUM		0x0144	/* BS: 0 BW: 8	*/
#define HDMI_SPD_DATA_BYTE_7_4			0x0148	/* BS: 0 BW: 32	*/
#define HDMI_SPD_DATA_BYTE_11_8			0x014C	/* BS: 0 BW: 32	*/
#define HDMI_SPD_DATA_BYTE_15_12		0x0150	/* BS: 0 BW: 32	*/
#define HDMI_SPD_DATA_BYTE_19_16		0x0154	/* BS: 0 BW: 32	*/
#define HDMI_SPD_DATA_BYTE_23_20		0x0158	/* BS: 0 BW: 32	*/
#define HDMI_SPD_DATA_BYTE_27_24		0x015C	/* BS: 0 BW: 32	*/
#define HDMI_AUI_CTRL				0x0160
#define HDMI_AUI_INFOFRAME_HB2			0x0160	/* BS: 16 BW: 8	*/
#define HDMI_AUI_INFOFRAME_HB1			0x0160	/* BS: 8 BW: 8	*/
#define HDMI_AUI_PKT_TYPE			0x0160	/* BS: 0 BW: 8	*/
#define HDMI_AUI_DATA_BYTE_3_1			0x0164	/* BS: 8 BW: 24	*/
#define HDMI_AUI_DATA_BYTE_CHECKSUM		0x0164	/* BS: 0 BW: 8	*/
#define HDMI_AUI_DATA_BYTE_7_4			0x0168	/* BS: 0 BW: 32	*/
#define HDMI_AUI_DATA_BYTE_11_8			0x016C	/* BS: 0 BW: 32	*/
#define HDMI_AUI_DATA_BYTE_15_12		0x0170	/* BS: 0 BW: 32	*/
#define HDMI_AUI_DATA_BYTE_19_16		0x0174	/* BS: 0 BW: 32	*/
#define HDMI_AUI_DATA_BYTE_23_20		0x0178	/* BS: 0 BW: 32	*/
#define HDMI_AUI_DATA_BYTE_27_24		0x017C	/* BS: 0 BW: 32	*/
#define HDMI_MPG_INFOFRAME_HB2			0x0180	/* BS: 16 BW: 8	*/
#define HDMI_MPG_INFOFRAME_HB1			0x0180	/* BS: 8 BW: 8	*/
#define HDMI_MPG_PKT_TYPE			0x0180	/* BS: 0 BW: 8	*/
#define HDMI_MPG_DATA_BYTE_3_1			0x0184	/* BS: 8 BW: 24	*/
#define HDMI_MPG_DATA_BYTE_CHECKSUM		0x0184	/* BS: 0 BW: 8	*/
#define HDMI_MPG_DATA_BYTE_7_4			0x0188	/* BS: 0 BW: 32	*/
#define HDMI_MPG_DATA_BYTE_11_8			0x018C	/* BS: 0 BW: 32	*/
#define HDMI_MPG_DATA_BYTE_15_12		0x0190	/* BS: 0 BW: 32	*/
#define HDMI_MPG_DATA_BYTE_19_16		0x0194	/* BS: 0 BW: 32	*/
#define HDMI_MPG_DATA_BYTE_23_20		0x0198	/* BS: 0 BW: 32	*/
#define HDMI_MPG_DATA_BYTE_27_24		0x019C	/* BS: 0 BW: 32	*/
#define HDMI_AMP3D_INFOFRAME_HB2		0x01A0	/* BS: 16 BW: 8	*/
#define HDMI_AMP3D_INFOFRAME_HB1		0x01A0	/* BS: 8 BW: 8	*/
#define HDMI_AMP3D_PKT_TYPE			0x01A0	/* BS: 0 BW: 8	*/
#define HDMI_AMP3D_DATA_BYTE_3_1		0x01A4	/* BS: 8 BW: 24	*/
#define HDMI_AMP3D_DATA_BYTE_CHECKSUM		0x01A4	/* BS: 0 BW: 8	*/
#define HDMI_AMP3D_DATA_BYTE_7_4		0x01A8	/* BS: 0 BW: 32	*/
#define HDMI_AMP3D_DATA_BYTE_11_8		0x01AC	/* BS: 0 BW: 32	*/
#define HDMI_AMP3D_DATA_BYTE_15_12		0x01B0	/* BS: 0 BW: 32	*/
#define HDMI_AMP3D_DATA_BYTE_19_16		0x01B4	/* BS: 0 BW: 32	*/
#define HDMI_AMP3D_DATA_BYTE_23_20		0x01B8	/* BS: 0 BW: 32	*/
#define HDMI_AMP3D_DATA_BYTE_27_24		0x01BC	/* BS: 0 BW: 32	*/
#define HDMI_AMPM_INFOFRAME_HB2			0x01C0	/* BS: 16 BW: 8	*/
#define HDMI_AMPM_INFOFRAME_HB1			0x01C0	/* BS: 8 BW: 8	*/
#define HDMI_AMPM_PKT_TYPE			0x01C0	/* BS: 0 BW: 8	*/
#define HDMI_AMPM_DATA_BYTE_3_1			0x01C4	/* BS: 8 BW: 24	*/
#define HDMI_AMPM_DATA_BYTE_CHECKSUM		0x01C4	/* BS: 0 BW: 8	*/
#define HDMI_AMPM_DATA_BYTE_7_4			0x01C8	/* BS: 0 BW: 32	*/
#define HDMI_AMPM_DATA_BYTE_11_8		0x01CC	/* BS: 0 BW: 32	*/
#define HDMI_AMPM_DATA_BYTE_15_12		0x01D0	/* BS: 0 BW: 32	*/
#define HDMI_AMPM_DATA_BYTE_19_16		0x01D4	/* BS: 0 BW: 32	*/
#define HDMI_AMPM_DATA_BYTE_23_20		0x01D8	/* BS: 0 BW: 32	*/
#define HDMI_AMPM_DATA_BYTE_27_24		0x01DC	/* BS: 0 BW: 32	*/
#define HDMI_VBI_INFOFRAME_HB2			0x01E0	/* BS: 16 BW: 8	*/
#define HDMI_VBI_INFOFRAME_HB1			0x01E0	/* BS: 8 BW: 8	*/
#define HDMI_VBI_PKT_TYPE			0x01E0	/* BS: 0 BW: 8	*/
#define HDMI_VBI_DATA_BYTE_3_1			0x01E4	/* BS: 8 BW: 24	*/
#define HDMI_VBI_DATA_BYTE_CHECKSUM		0x01E4	/* BS: 0 BW: 8	*/
#define HDMI_VBI_DATA_BYTE_7_4			0x01E8	/* BS: 0 BW: 32	*/
#define HDMI_VBI_DATA_BYTE_11_8			0x01EC	/* BS: 0 BW: 32	*/
#define HDMI_VBI_DATA_BYTE_15_12		0x01F0	/* BS: 0 BW: 32	*/
#define HDMI_VBI_DATA_BYTE_19_16		0x01F4	/* BS: 0 BW: 32	*/
#define HDMI_VBI_DATA_BYTE_23_20		0x01F8	/* BS: 0 BW: 32	*/
#define HDMI_VBI_DATA_BYTE_27_24		0x01FC	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_1_HB2			0x0200	/* BS: 16 BW: 8	*/
#define HDMI_DUMMY_1_HB1			0x0200	/* BS: 8 BW: 8	*/
#define HDMI_DUMMY_1_PKT_TYPE			0x0200	/* BS: 0 BW: 8	*/
#define HDMI_DUMMY_1_DATA_BYTE_3_0		0x0204	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_1_DATA_BYTE_7_4		0x0208	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_1_DATA_BYTE_11_8		0x020C	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_1_DATA_BYTE_15_12		0x0210	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_1_DATA_BYTE_19_16		0x0214	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_1_DATA_BYTE_23_20		0x0218	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_1_DATA_BYTE_27_24		0x021C	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_2_HB2			0x0220	/* BS: 16 BW: 8	*/
#define HDMI_DUMMY_2_HB1			0x0220	/* BS: 8 BW: 8	*/
#define HDMI_DUMMY_2_PKT_TYPE			0x0220	/* BS: 0 BW: 8	*/
#define HDMI_DUMMY_2_DATA_BYTE_3_0		0x0224	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_2_DATA_BYTE_7_4		0x0228	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_2_DATA_BYTE_11_8		0x022C	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_2_DATA_BYTE_15_12		0x0230	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_2_DATA_BYTE_19_16		0x0234	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_2_DATA_BYTE_23_20		0x0238	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_2_DATA_BYTE_27_24		0x023C	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_3_HB2			0x0240	/* BS: 16 BW: 8	*/
#define HDMI_DUMMY_3_HB1			0x0240	/* BS: 8 BW: 8	*/
#define HDMI_DUMMY_3_PKT_TYPE			0x0240	/* BS: 0 BW: 8	*/
#define HDMI_DUMMY_3_DATA_BYTE_3_0		0x0244	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_3_DATA_BYTE_7_4		0x0248	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_3_DATA_BYTE_11_8		0x024C	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_3_DATA_BYTE_15_12		0x0250	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_3_DATA_BYTE_19_16		0x0254	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_3_DATA_BYTE_23_20		0x0258	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_3_DATA_BYTE_27_24		0x025C	/* BS: 0 BW: 32	*/
#define HDMI_KEEPOUT_START_CNT			0x0300	/* BS: 0 BW: 16	*/
#define HDMI_WINOP_START_CNT			0x0304	/* BS: 0 BW: 16	*/
#define HDMI_WINOP_END_CNT			0x0308	/* BS: 0 BW: 16	*/
#define HDMI_KEEPOUT_END_CNT			0x030C	/* BS: 0 BW: 16	*/
#define HDMI_INT_AUDIOFIFO_UNDERFLOW_EN		0x0310	/* BS: 5 BW: 1	*/
#define HDMI_INT_AUDIOFIFO_OVERFLOW_EN		0x0310	/* BS: 4 BW: 1	*/
#define HDMI_INT_DEEPCOLOR_UNDERFLOW_EN		0x0310	/* BS: 3 BW: 1	*/
#define HDMI_INT_DEEPCOLOR_OVERFLOW_EN		0x0310	/* BS: 2 BW: 1	*/
#define HDMI_PHY_READY_EN			0x0310	/* BS: 1 BW: 1	*/
#define HDMI_HPD_EN				0x0310	/* BS: 0 BW: 1	*/
#define HDMI_INT_AUDIOFIFO_UNDERFLOW		0x0314	/* BS: 5 BW: 1	*/
#define HDMI_INT_AUDIOFIFO_OVERFLOW		0x0314	/* BS: 4 BW: 1	*/
#define HDMI_INT_DEEPCOLOR_UNDERFLOW		0x0314	/* BS: 3 BW: 1	*/
#define HDMI_INT_DEEPCOLOR_OVERFLOW		0x0314	/* BS: 2 BW: 1	*/
#define HDMI_CORE_INT_PHY_READY			0x0314	/* BS: 1 BW: 1	*/
#define HDMI_HPD_INTPEND			0x0314	/* BS: 0 BW: 1	*/
#define HDMI_HDCP22_INT				0x0318	/* BS: 5 BW: 1	*/
#define HDMI_HDCP14_INT				0x0318	/* BS: 4 BW: 1	*/
#define HDMI_AUDIO_INPUT_INT			0x0318	/* BS: 3 BW: 1	*/
#define HDMI_SPDIF_INT				0x0318	/* BS: 2 BW: 1	*/
#define HDMI_I2S_INT				0x0318	/* BS: 1 BW: 1	*/
#define HDMI_HDMI_CORE_INT			0x0318	/* BS: 0 BW: 1	*/
#define HDMI_HPD_GLITCH_THRESHOLD_R		0x0330	/* BS: 0 BW: 32	*/
#define HDMI_HPD_GLITCH_THRESHOLD_F		0x0334	/* BS: 0 BW: 32	*/
#define HDMI_PHY_READY_GLITCH_THRESHOLD_R	0x0338	/* BS: 0 BW: 32	*/
#define HDMI_PHY_READY_GLITCH_THRESHOLD_F	0x033C	/* BS: 0 BW: 32	*/
#define HDMI_DUMMY_3_OCCUR			0x0340	/* BS: 28 BW: 4	*/
#define HDMI_DUMMY_2_OCCUR			0x0340	/* BS: 24 BW: 4	*/
#define HDMI_DUMMY_1_OCCUR			0x0340	/* BS: 20 BW: 4	*/
#define HDMI_VBI_OCCUR				0x0340	/* BS: 16 BW: 4	*/
#define HDMI_AMPM_OCCUR				0x0340	/* BS: 12 BW: 4	*/
#define HDMI_AMP3D_OCCUR			0x0340	/* BS: 8 BW: 4	*/
#define HDMI_MPG_OCCUR				0x0340	/* BS: 4 BW: 4	*/
#define HDMI_AUI_OCCUR				0x0340	/* BS: 0 BW: 4	*/
#define HDMI_SPD_OCCUR				0x0344	/* BS: 28 BW: 4	*/
#define HDMI_AVI_OCCUR				0x0344	/* BS: 24 BW: 4	*/
#define HDMI_VSI_OCCUR				0x0344	/* BS: 20 BW: 4	*/
#define HDMI_GMP_OCCUR				0x0344	/* BS: 16 BW: 4	*/
#define HDMI_ISRC2_OCCUR			0x0344	/* BS: 12 BW: 4	*/
#define HDMI_ISRC1_OCCUR			0x0344	/* BS: 8 BW: 4	*/
#define HDMI_ACP_OCCUR				0x0344	/* BS: 4 BW: 4	*/
#define HDMI_GCP_OCCUR				0x0344	/* BS: 0 BW: 4	*/
#define HDMI_RGB_Y_LIMIT_MAX			0x0350	/* BS: 16 BW: 12 */
#define HDMI_RGB_Y_LIMIT_MIN			0x0350	/* BS: 0 BW: 12	*/
#define HDMI_CB_CR_LIMIT_MAX			0x0354	/* BS: 16 BW: 12 */
#define HDMI_CB_CR_LIMIT_MIN			0x0354	/* BS: 0 BW: 12	*/
#define HDMI_B_CB_SEL				0x0360	/* BS: 8 BW: 2	*/
#define HDMI_G_Y_SEL				0x0360	/* BS: 4 BW: 2	*/
#define HDMI_R_CR_SEL				0x0360	/* BS: 0 BW: 2	*/
#define HDMI_BIT_SWAP_SEL_B_CB_3		0x0364	/* BS: 24 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_B_CB_2		0x0364	/* BS: 16 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_B_CB_1		0x0364	/* BS: 8 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_B_CB_0		0x0364	/* BS: 0 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_B_CB_7		0x0368	/* BS: 24 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_B_CB_6		0x0368	/* BS: 16 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_B_CB_5		0x0368	/* BS: 8 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_B_CB_4		0x0368	/* BS: 0 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_B_CB_11		0x036C	/* BS: 24 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_B_CB_10		0x036C	/* BS: 16 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_B_CB_9		0x036C	/* BS: 8 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_B_CB_8		0x036C	/* BS: 0 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_G_Y_3			0x0370	/* BS: 24 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_G_Y_2			0x0370	/* BS: 16 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_G_Y_1			0x0370	/* BS: 8 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_G_Y_0			0x0370	/* BS: 0 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_G_Y_7			0x0374	/* BS: 24 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_G_Y_6			0x0374	/* BS: 16 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_G_Y_5			0x0374	/* BS: 8 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_G_Y_4			0x0374	/* BS: 0 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_G_Y_11		0x0378	/* BS: 24 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_G_Y_10		0x0378	/* BS: 16 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_G_Y_9			0x0378	/* BS: 8 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_G_Y_8			0x0378	/* BS: 0 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_R_CR_3		0x037C	/* BS: 24 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_R_CR_2		0x037C	/* BS: 16 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_R_CR_1		0x037C	/* BS: 8 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_R_CR_0		0x037C	/* BS: 0 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_R_CR_7		0x0380	/* BS: 24 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_R_CR_6		0x0380	/* BS: 16 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_R_CR_5		0x0380	/* BS: 8 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_R_CR_4		0x0380	/* BS: 0 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_R_CR_11		0x0384	/* BS: 24 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_R_CR_10		0x0384	/* BS: 16 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_R_CR_9		0x0384	/* BS: 8 BW: 5	*/
#define HDMI_BIT_SWAP_SEL_R_CR_8		0x0384	/* BS: 0 BW: 5	*/
#define HDMI_TEST_PTTN_RUN			0x0400	/* BS: 0 BW: 1	*/
#define HDMI_TEST_PTTN_RUNNING			0x0404	/* BS: 0 BW: 1	*/
#define HDMI_TEST_PTTN_PORT			0x0408	/* BS: 12 BW: 4	*/
#define HDMI_TEST_PTTN_INTERLACED		0x0408	/* BS: 10 BW: 1	*/
#define HDMI_TEST_PTTN_POL_HS			0x0408	/* BS: 9 BW: 1	*/
#define HDMI_TEST_PTTN_POL_VS			0x0408	/* BS: 8 BW: 1	*/
#define HDMI_TEST_PTTN_TYPE			0x0408	/* BS: 0 BW: 4	*/
#define HDMI_TEST_PTTN_VOFF			0x040C	/* BS: 0 BW: 16	*/
#define HDMI_TEST_PTTN_VFP			0x0410	/* BS: 0 BW: 16	*/
#define HDMI_TEST_PTTN_VS			0x0414	/* BS: 0 BW: 16	*/
#define HDMI_TEST_PTTN_VBP			0x0418	/* BS: 0 BW: 16	*/
#define HDMI_TEST_PTTN_VLINE			0x041C	/* BS: 0 BW: 16	*/
#define HDMI_TEST_PTTN_DE			0x0420	/* BS: 0 BW: 16	*/
#define HDMI_TEST_PTTN_HBP			0x0424	/* BS: 0 BW: 16	*/
#define HDMI_TEST_PTTN_HS			0x0428	/* BS: 0 BW: 16	*/
#define HDMI_TEST_PTTN_HFP			0x042C	/* BS: 0 BW: 16	*/
#define HDMI_TEST_PTTN_USER0_B			0x0430	/* BS: 0 BW: 12	*/
#define HDMI_TEST_PTTN_USER0_G			0x0434	/* BS: 0 BW: 12	*/
#define HDMI_TEST_PTTN_USER0_R			0x0438	/* BS: 0 BW: 12	*/
#define HDMI_TEST_PTTN_USER1_B			0x0440	/* BS: 0 BW: 12	*/
#define HDMI_TEST_PTTN_USER1_G			0x0444	/* BS: 0 BW: 12	*/
#define HDMI_TEST_PTTN_USER1_R			0x0448	/* BS: 0 BW: 12	*/
#define HDMI_TEST_PTTN_PACKET_EN		0x044C	/* BS: 0 BW: 16	*/
#define HDMI_CH0_SWAP				0x0500	/* BS: 8 BW: 2	*/
#define HDMI_CH1_SWAP				0x0500	/* BS: 6 BW: 2	*/
#define HDMI_CH2_SWAP				0x0500	/* BS: 4 BW: 2	*/
#define HDMI_OUTPUT_SWAP_1B			0x0500	/* BS: 1 BW: 1	*/
#define HDMI_OUTPUT_SWAP_10B			0x0500	/* BS: 0 BW: 1	*/
#define HDMI_SSCP_POSITION_EN			0x0504	/* BS: 28 BW: 1	*/
#define HDMI_SSCP_LINE_CNT			0x0504	/* BS: 16 BW: 12 */
#define HDMI_SSCP_PIXEL_CNT			0x0504	/* BS: 0 BW: 16	*/
#define HDMI_BLUESCREEN_EN			0x0510	/* BS: 16 BW: 1	*/
#define HDMI_RGB_B				0x0510	/* BS: 0 BW: 12	*/
#define HDMI_RGB_G				0x0514	/* BS: 0 BW: 12	*/
#define HDMI_RGB_R				0x0518	/* BS: 0 BW: 12	*/
#define HDMI_PKT_DISABLE_EN			0x0550	/* BS: 28 BW: 1	*/
#define HDMI_PKT_DISABLE_START_1		0x0550	/* BS: 16 BW: 12 */
#define HDMI_PKT_DISABLE_END_1			0x0550	/* BS: 0 BW: 12	*/
#define HDMI_PKT_DISABLE_START_2		0x0554	/* BS: 16 BW: 12 */
#define HDMI_PKT_DISABLE_END_2			0x0554	/* BS: 0 BW: 12	*/
#define HDMI_PKT_DISABLE_START_3		0x0558	/* BS: 16 BW: 12 */
#define HDMI_PKT_DISABLE_END_3			0x0558	/* BS: 0 BW: 12	*/
#define HDMI_DBG_CTS_CNT			0x0560	/* BS: 0 BW: 20	*/
#define HDMI_DBG_B_CNT				0x0564	/* BS: 0 BW: 32	*/
#define HDMI_PERIOD_1				0x0570	/* BS: 16 BW: 10 */
#define HDMI_PERIOD_0				0x0570	/* BS: 0 BW: 10	*/
#define HDMI_PKT_EN_MASK_EN_R			0x0574	/* BS: 29 BW: 1	*/
#define HDMI_PKT_EN_MASK_EN_F			0x0574	/* BS: 28 BW: 1	*/
#define HDMI_PKT_EN_CNT_R			0x0574	/* BS: 8 BW: 8	*/
#define HDMI_PKT_EN_CNT_F			0x0574	/* BS: 0 BW: 8	*/
#define HDMI_PKT_EN_MOD_EN			0x0578	/* BS: 28 BW: 1	*/
#define HDMI_PKT_EN_MOD_START			0x0578	/* BS: 16 BW: 12 */
#define HDMI_PKT_EN_MOD_END			0x0578	/* BS: 0 BW: 12	*/
#define HDMI_MON_PACKET_EN_1_F_WIDTH_PIXEL	0x0580	/* BS: 16 BW: 16 */
#define HDMI_MON_PACKET_EN_0_F_WIDTH_PIXEL	0x0580	/* BS: 0 BW: 16	*/
#define HDMI_MON_PACKET_EN_1_B_WIDTH_PIXEL	0x0584	/* BS: 16 BW: 16 */
#define HDMI_MON_PACKET_EN_0_B_WIDTH_PIXEL	0x0584	/* BS: 0 BW: 16	*/
#define HDMI_MON_VFP_1_WIDTH_PIXEL		0x0588	/* BS: 16 BW: 16 */
#define HDMI_MON_VFP_0_WIDTH_PIXEL		0x0588	/* BS: 0 BW: 16	*/
#define HDMI_MON_VSYNC_1_WIDTH_PIXEL		0x058C	/* BS: 16 BW: 16 */
#define HDMI_MON_VSYNC_0_WIDTH_PIXEL		0x058C	/* BS: 0 BW: 16	*/
#define HDMI_MON_VBP_1_WIDTH_PIXEL		0x0590	/* BS: 16 BW: 16 */
#define HDMI_MON_VBP_0_WIDTH_PIXEL		0x0590	/* BS: 0 BW: 16	*/
#define HDMI_MON_HFP_1_WIDTH_PIXEL		0x0594	/* BS: 16 BW: 16 */
#define HDMI_MON_HFP_0_WIDTH_PIXEL		0x0594	/* BS: 0 BW: 16	*/
#define HDMI_MON_HSYNC_1_WIDTH_PIXEL		0x0598	/* BS: 16 BW: 16 */
#define HDMI_MON_HSYNC_0_WIDTH_PIXEL		0x0598	/* BS: 0 BW: 16	*/
#define HDMI_MON_HBP_1_WIDTH_PIXEL		0x059C	/* BS: 16 BW: 16 */
#define HDMI_MON_HBP_0_WIDTH_PIXEL		0x059C	/* BS: 0 BW: 16	*/
#define HDMI_MON_DE_1_WIDTH_PIXEL		0x05A0	/* BS: 16 BW: 16 */
#define HDMI_MON_DE_0_WIDTH_PIXEL		0x05A0	/* BS: 0 BW: 16	*/
#define HDMI_MON_PACKET_EN_1_F_WIDTH_TMDS	0x05B0	/* BS: 16 BW: 16 */
#define HDMI_MON_PACKET_EN_0_F_WIDTH_TMDS	0x05B0	/* BS: 0 BW: 16	*/
#define HDMI_MON_PACKET_EN_1_B_WIDTH_TMDS	0x05B4	/* BS: 16 BW: 16 */
#define HDMI_MON_PACKET_EN_0_B_WIDTH_TMDS	0x05B4	/* BS: 0 BW: 16	*/
#define HDMI_MON_VFP_1_WIDTH_TMDS		0x05B8	/* BS: 16 BW: 16 */
#define HDMI_MON_VFP_0_WIDTH_TMDS		0x05B8	/* BS: 0 BW: 16	*/
#define HDMI_MON_VSYNC_1_WIDTH_TMDS		0x05BC	/* BS: 16 BW: 16 */
#define HDMI_MON_VSYNC_0_WIDTH_TMDS		0x05BC	/* BS: 0 BW: 16	*/
#define HDMI_MON_VBP_1_WIDTH_TMDS		0x05C0	/* BS: 16 BW: 16 */
#define HDMI_MON_VBP_0_WIDTH_TMDS		0x05C0	/* BS: 0 BW: 16	*/
#define HDMI_MON_HFP_1_WIDTH_TMDS		0x05C4	/* BS: 16 BW: 16 */
#define HDMI_MON_HFP_0_WIDTH_TMDS		0x05C4	/* BS: 0 BW: 16	*/
#define HDMI_MON_HSYNC_1_WIDTH_TMDS		0x05C8	/* BS: 16 BW: 16 */
#define HDMI_MON_HSYNC_0_WIDTH_TMDS		0x05C8	/* BS: 0 BW: 16	*/
#define HDMI_MON_HBP_1_WIDTH_TMDS		0x05CC	/* BS: 16 BW: 16 */
#define HDMI_MON_HBP_0_WIDTH_TMDS		0x05CC	/* BS: 0 BW: 16	*/
#define HDMI_MON_DE_1_WIDTH_TMDS		0x05D0	/* BS: 16 BW: 16 */
#define HDMI_MON_DE_0_WIDTH_TMDS		0x05D0	/* BS: 0 BW: 16	*/
#define HDMI_AUDIO_FIFO_HIT_MAX_CLEAR		0x0650	/* BS: 17 BW: 1	*/
#define HDMI_AUDIO_FIFO_HIT_MIN_CLEAR		0x0650	/* BS: 16 BW: 1	*/
#define HDMI_AUDIO_FIFO_HIT_MAX_SET		0x0650	/* BS: 8 BW: 5	*/
#define HDMI_AUDIO_FIFO_HIT_MIN_SET		0x0650	/* BS: 0 BW: 5	*/
#define HDMI_AUDIO_FIFO_FULL			0x0654	/* BS: 21 BW: 1	*/
#define HDMI_AUDIO_FIFO_EMPTY			0x0654	/* BS: 20 BW: 1	*/
#define HDMI_AUDIO_FIFO_MAX			0x0654	/* BS: 13 BW: 5	*/
#define HDMI_AUDIO_FIFO_MIN			0x0654	/* BS: 8 BW: 5	*/
#define HDMI_AUDIO_FIFO_HIT_MAX			0x0654	/* BS: 4 BW: 1	*/
#define HDMI_AUDIO_FIFO_HIT_MIN			0x0654	/* BS: 0 BW: 1	*/
#define HDMI_AUDIO_PACKET_REMAIN_UNDERFLOW_CLR	0x0658	/* BS: 1 BW: 1	*/
#define HDMI_AUDIO_PACKET_REMAIN_OVERFLOW_CLR	0x0658	/* BS: 0 BW: 1	*/
#define HDMI_AUDIO_PACKET_REMAIN_UNDERFLOW	0x0658	/* BS: 21 BW: 1	*/
#define HDMI_AUDIO_PACKET_REMAIN_OVERFLOW	0x0658	/* BS: 20 BW: 1	*/
#define HDMI_AUDIO_REMAIN_SAMPLE		0x0658	/* BS: 12 BW: 5	*/
#define HDMI_AUDIO_PACKET_REMAIN		0x0658	/* BS: 4 BW: 5	*/
#define HDMI_ACR_REMINDER_IN_TMDS		0x0660	/* BS: 0 BW: 20	*/
#define HDMI_CTS_DITHERING_THRESHOLD		0x0664	/* BS: 8 BW: 8	*/
#define HDMI_CTS_DITHERING_LOGIC_SYSTEM_ENABLE	0x0664	/* BS: 1 BW: 1	*/
#define HDMI_CTS_DITHERING_MUX_ENABLE		0x0664	/* BS: 0 BW: 1	*/
#define HDMI_STROBE_GEN_CNT			0x0668	/* BS: 4 BW: 12	*/
#define HDMI_STROBE_GEN_MUX			0x0668	/* BS: 0 BW: 1	*/
#define HDMI_SYNC_MULTI_CYCLE_CNT		0x066C	/* BS: 8 BW: 8	*/
#define HDMI_STROBE_MULTI_CYCLE_SYNC_MUX_EN	0x066C	/* BS: 0 BW: 1	*/
#define HDMI_PCM_STROBE_PULSE_MONITOR_SYS_EN	0x0670	/* BS: 0 BW: 1	*/
#define HDMI_PCM_STROBE_CUR_PULSE_CNT		0x0674	/* BS: 0 BW: 20	*/
#define HDMI_PCM_STROBE_MAX_PULSE_CNT		0x0678	/* BS: 0 BW: 20	*/
#define HDMI_PCM_STROBE_MIN_PULSE_CNT		0x067C	/* BS: 0 BW: 20	*/
#define HDMI_CTS_MONITOR_SYS_EN			0x0680	/* BS: 0 BW: 1	*/
#define HDMI_CTS_CURRENT			0x0684	/* BS: 0 BW: 20	*/
#define HDMI_CTS_MAX				0x0688	/* BS: 0 BW: 20	*/
#define HDMI_CTS_MIN				0x068C	/* BS: 0 BW: 20	*/
#define HDMI_DITHERED_CTS_MONITOR_SYS_EN	0x0690	/* BS: 0 BW: 1	*/
#define HDMI_DITHERED_CTS_CURRENT		0x0694	/* BS: 0 BW: 20	*/
#define HDMI_DITHERED_CTS_MAX			0x0698	/* BS: 0 BW: 20	*/
#define HDMI_DITHERED_CTS_MIN			0x069C	/* BS: 0 BW: 20	*/
#define HDMI_DBG_RSTN_DISABLE			0x06A0	/* BS: 8 BW: 1	*/
#define HDMI_DBG_RSTN_DISABLE_CNT		0x06A0	/* BS: 0 BW: 8	*/
#define HDMI_DBG_AUD_FS_EN_ALARM_EN		0x06A4	/* BS: 0 BW: 1	*/
#define HDMI_DBG_AUD_FS_EN_ALARM_CUR		0x06A4	/* BS: 16 BW: 16 */
#define HDMI_DBG_AUD_FS_EN_ALARM_MAX		0x06A8	/* BS: 16 BW: 16 */
#define HDMI_DBG_AUD_FS_EN_ALARM_MIN		0x06A8	/* BS: 0 BW: 16	*/
#define HDMI_DBG_AUD_FS_EN_MON			0x06AC	/* BS: 2 BW: 1	*/
#define HDMI_DBG_PDMA_REQ_MON			0x06AC	/* BS: 1 BW: 1	*/
#define HDMI_DBG_PDMA_ACK_MON			0x06AC	/* BS: 0 BW: 1	*/
/* I2S */
#define HDMI_I2S_CLK_CON			0x1000
#define HDMI_I2S_CON_1				0x1004
#define HDMI_I2S_CON_2				0x1008
#define HDMI_I2S_PIN_SEL_0			0x100c
#define HDMI_I2S_PIN_SEL_1			0x1010
#define HDMI_I2S_PIN_SEL_2			0x1014
#define HDMI_I2S_PIN_SEL_3			0x1018
#define HDMI_I2S_DSD_CON			0x101c
#define HDMI_I2S_CH_ST_CON			0x1024
#define HDMI_I2S_CH_ST_0			0x1028
#define HDMI_I2S_CH_ST_1			0x102c
#define HDMI_I2S_CH_ST_2			0x1030
#define HDMI_I2S_CH_ST_3			0x1034
#define HDMI_I2S_CH_ST_4			0x1038
#define HDMI_I2S_CH_ST_SH_0			0x103c
#define HDMI_I2S_CH_ST_SH_1			0x1040
#define HDMI_I2S_CH_ST_SH_2			0x1044
#define HDMI_I2S_CH_ST_SH_3			0x1048
#define HDMI_I2S_CH_ST_SH_4			0x104c
#define HDMI_I2S_VD_DATA			0x1050
#define HDMI_I2S_CH0_L_0			0x1064
#define HDMI_I2S_CH0_L_1			0x1068
#define HDMI_I2S_CH0_L_2			0x106C
#define HDMI_I2S_CH0_L_3			0x1070
#define HDMI_I2S_CH0_R_0			0x1074
#define HDMI_I2S_CH0_R_1			0x1078
#define HDMI_I2S_CH0_R_2			0x107C
#define HDMI_I2S_CH0_R_3			0x1080
#define HDMI_I2S_CH1_L_0			0x1084
#define HDMI_I2S_CH1_L_1			0x1088
#define HDMI_I2S_CH1_L_2			0x108C
#define HDMI_I2S_CH1_L_3			0x1090
#define HDMI_I2S_CH1_R_0			0x1094
#define HDMI_I2S_CH1_R_1			0x1098
#define HDMI_I2S_CH1_R_2			0x109C
#define HDMI_I2S_CH1_R_3			0x10A0
#define HDMI_I2S_CH2_L_0			0x10A4
#define HDMI_I2S_CH2_L_1			0x10A8
#define HDMI_I2S_CH2_L_2			0x10AC
#define HDMI_I2S_CH2_L_3			0x10B0
#define HDMI_I2S_CH2_R_0			0x10B4
#define HDMI_I2S_CH2_R_1			0x10B8
#define HDMI_I2S_CH2_R_2			0x10BC
#define HDMI_I2S_Ch2_R_3			0x10C0
#define HDMI_I2S_CH3_L_0			0x10C4
#define HDMI_I2S_CH3_L_1			0x10C8
#define HDMI_I2S_CH3_L_2			0x10CC
#define HDMI_I2S_CH3_R_0			0x10D0
#define HDMI_I2S_CH3_R_1			0x10D4
#define HDMI_I2S_CH3_R_2			0x10D8
#define HDMI_I2S_CUV_L_R			0x10DC

/* SPDIF */
#define HDMI_SPDIFIN_CLK_CTRL			0x2000
#define HDMI_SPDIFIN_OP_CTRL			0x2004
#define HDMI_SPDIFIN_IRQ_MASK			0x2008
#define HDMI_SPDIFIN_IRQ_STATUS			0x200c
#define HDMI_SPDIFIN_CONFIG_1			0x2010
#define HDMI_SPDIFIN_CONFIG_2			0x2014
#define HDMI_SPDIFIN_USER_VALUE_1		0x2020
#define HDMI_SPDIFIN_USER_VALUE_2		0x2024
#define HDMI_SPDIFIN_USER_VALUE_3		0x2028
#define HDMI_SPDIFIN_USER_VALUE_4		0x202c
#define HDMI_SPDIFIN_CH_STATUS_0_1		0x2030
#define HDMI_SPDIFIN_CH_STATUS_0_2		0x2034
#define HDMI_SPDIFIN_CH_STATUS_0_3		0x2038
#define HDMI_SPDIFIN_CH_STATUS_0_4		0x203c
#define HDMI_SPDIFIN_CH_STATUS_1		0x2040
#define HDMI_SPDIFIN_FRAME_PERIOD_1		0x2048
#define HDMI_SPDIFIN_FRAME_PERIOD_2		0x204c
#define HDMI_SPDIFIN_PC_INFO_1			0x2050
#define HDMI_SPDIFIN_PC_INFO_2			0x2054
#define HDMI_SPDIFIN_PD_INFO_1			0x2058
#define HDMI_SPDIFIN_PD_INFO_2			0x205c
#define HDMI_SPDIFIN_DATA_BUF_0_1		0x2060
#define HDMI_SPDIFIN_DATA_BUF_0_2		0x2064
#define HDMI_SPDIFIN_DATA_BUF_0_3		0x2068
#define HDMI_SPDIFIN_USER_BUF_0			0x206c
#define HDMI_SPDIFIN_DATA_BUF_1_1		0x2070
#define HDMI_SPDIFIN_DATA_BUF_1_2		0x2074
#define HDMI_SPDIFIN_DATA_BUF_1_3		0x2078
#define HDMI_SPDIFIN_USER_BUF_1			0x207c

#define HDMI_AUDIO_INPUT_SYSTEM_ENABLE		0x3000	/* BS: 0 BW: 1	*/
#define HDMI_CHANEL_NUMBER			0x3004	/* BS: 0 BW: 4	*/
#define HDMI_PCM_SELECT				0x3008	/* BS: 0 BW: 1	*/
#define HDMI_CUV_SELECT				0x300C	/* BS: 0 BW: 3	*/
#define HDMI_PSEL				0x3010	/* BS: 0 BW: 1	*/
#define HDMI_HBR				0x3014	/* BS: 0 BW: 1	*/
#define HDMI_PACKED_MODE			0x3018	/* BS: 0 BW: 1	*/
#define HDMI_PCUV_MODE				0x301C	/* BS: 0 BW: 1	*/
#define HDMI_CHANNEL_BIT			0x3020	/* BS: 0 BW: 2	*/
#define HDMI_AUDIO_TIMEOUT_FLAG			0x3024	/* BS: 1 BW: 1	*/
#define HDMI_DATA_ERROR_FLAG			0x3024	/* BS: 0 BW: 1	*/
#define HDMI_AUDIO_TIMEOUT_INTERRUPT_EN		0x3028	/* BS: 1 BW: 1	*/
#define HDMI_DATA_ERROR_INTERRUPT_EN		0x3028	/* BS: 0 BW: 1	*/
#define HDMI_TIMEOUT_INTERRUPT_CLEAR		0x302C	/* BS: 1 BW: 1	*/
#define HDMI_DATA_ERROR_INTERRUPT_CLEAR		0x302C	/* BS: 0 BW: 1	*/
#define HDMI_MAX_AUDIO_TIME			0x3030	/* BS: 0 BW: 16	*/
#define HDMI_AUDIO_TIMEOUT_BYPASS		0x3034	/* BS: 0 BW: 1	*/
#define HDMI_DATA_ERROR_BYPASS			0x3038	/* BS: 0 BW: 1	*/
#define HDMI_CH0_ST_SH_0			0x3050	/* BS: 0 BW: 32	*/
#define HDMI_CH0_ST_SH_1			0x3054	/* BS: 0 BW: 8	*/
#define HDMI_CH1_ST_SH_0			0x3058	/* BS: 0 BW: 32	*/
#define HDMI_CH1_ST_SH_1			0x305C	/* BS: 0 BW: 8	*/
#define HDMI_CH2_ST_SH_0			0x3060	/* BS: 0 BW: 32	*/
#define HDMI_CH2_ST_SH_1			0x3064	/* BS: 0 BW: 8	*/
#define HDMI_CH3_ST_SH_0			0x3068	/* BS: 0 BW: 32	*/
#define HDMI_CH3_ST_SH_1			0x306C	/* BS: 0 BW: 8	*/
#define HDMI_CH4_ST_SH_0			0x3070	/* BS: 0 BW: 32	*/
#define HDMI_CH4_ST_SH_1			0x3074	/* BS: 0 BW: 8	*/
#define HDMI_CH5_ST_SH_0			0x3078	/* BS: 0 BW: 32	*/
#define HDMI_CH5_ST_SH_1			0x307C	/* BS: 0 BW: 8	*/
#define HDMI_CH6_ST_SH_0			0x3080	/* BS: 0 BW: 32	*/
#define HDMI_CH6_ST_SH_1			0x3084	/* BS: 0 BW: 8	*/
#define HDMI_CH7_ST_SH_0			0x3088	/* BS: 0 BW: 32	*/
#define HDMI_CH7_ST_SH_1			0x308C	/* BS: 0 BW: 8	*/
#define HDMI_CHANNEL_STATUS_BIT_RELOAD		0x3090	/* BS: 0 BW: 1	*/
#define HDMI_CH0_ST_0				0x30A0	/* BS: 0 BW: 32	*/
#define HDMI_CH0_ST_1				0x30A4	/* BS: 0 BW: 8	*/
#define HDMI_CH1_ST_0				0x30A8	/* BS: 0 BW: 32	*/
#define HDMI_CH1_ST_1				0x30AC	/* BS: 0 BW: 8	*/
#define HDMI_CH2_ST_0				0x30B0	/* BS: 0 BW: 32	*/
#define HDMI_CH2_ST_1				0x30B4	/* BS: 0 BW: 8	*/
#define HDMI_CH3_ST_0				0x30B8	/* BS: 0 BW: 32	*/
#define HDMI_CH3_ST_1				0x30BC	/* BS: 0 BW: 8	*/
#define HDMI_CH4_ST_0				0x30C0	/* BS: 0 BW: 32	*/
#define HDMI_CH4_ST_1				0x30C4	/* BS: 0 BW: 8	*/
#define HDMI_CH5_ST_0				0x30C8	/* BS: 0 BW: 32	*/
#define HDMI_CH5_ST_1				0x30CC	/* BS: 0 BW: 8	*/
#define HDMI_CH6_ST_0				0x30D0	/* BS: 0 BW: 32	*/
#define HDMI_CH6_ST_1				0x30D4	/* BS: 0 BW: 8	*/
#define HDMI_CH7_ST_0				0x30D8	/* BS: 0 BW: 32	*/
#define HDMI_CH7_ST_1				0x30DC	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_0			0x4000	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_1			0x4004	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_2			0x4008	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_3			0x400C	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_4			0x4010	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_5			0x4014	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_6			0x4018	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_7			0x401C	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_8			0x4020	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_9			0x4024	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_10			0x4028	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_11			0x402C	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_12			0x4030	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_13			0x4034	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_14			0x4038	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_15			0x403C	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_16			0x4040	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_17			0x4044	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_18			0x4048	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_SHA1_19			0x404C	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_KSV_LIST_0			0x4050	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_KSV_LIST_1			0x4054	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_KSV_LIST_2			0x4058	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_KSV_LIST_3			0x405C	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_KSV_LIST_4			0x4060	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_KSV_WRITE_DONE		0x4064	/* BS: 3 BW: 1	*/
#define HDMI_HDCP14_KSV_LIST_EMPTY		0x4064	/* BS: 2 BW: 1	*/
#define HDMI_HDCP14_KSV_END			0x4064	/* BS: 1 BW: 1	*/
#define HDMI_HDCP14_KSV_READ			0x4064	/* BS: 0 BW: 1	*/
#define HDMI_HDCP14_SHA_VALID_READY		0x4070	/* BS: 1 BW: 1	*/
#define HDMI_HDCP14_SHA_VALID			0x4070	/* BS: 0 BW: 1	*/
#define HDMI_TIMEOUT				0x4080	/* BS: 2 BW: 1	*/
#define HDMI_CP_DESIRED				0x4080	/* BS: 1 BW: 1	*/
#define HDMI_AN_INT_MOD				0x4084	/* BS: 4 BW: 1	*/
#define HDMI_REVOCATION_SET			0x4084	/* BS: 0 BW: 1	*/
#define HDMI_UPDATE_RI_INT			0x4088	/* BS: 4 BW: 1	*/
#define HDMI_UPDATE_PJ_INT			0x4088	/* BS: 3 BW: 1	*/
#define HDMI_AN_WRITE_INT			0x4088	/* BS: 2 BW: 1	*/
#define HDMI_WATCHDOG_INT			0x4088	/* BS: 1 BW: 1	*/
#define HDMI_I2C_INIT_INT			0x4088	/* BS: 0 BW: 1	*/
#define HDMI_AUTHEN_ACK				0x4088	/* BS: 5 BW: 1	*/
#define HDMI_UPDATE_RI_INT_EN			0x408C	/* BS: 4 BW: 1	*/
#define HDMI_UPDATE_PJ_INT_EN			0x408C	/* BS: 3 BW: 1	*/
#define HDMI_AN_WRITE_INT_EN			0x408C	/* BS: 2 BW: 1	*/
#define HDMI_WATCHDOG_INT_EN			0x408C	/* BS: 1 BW: 1	*/
#define HDMI_I2C_INIT_INT_EN			0x408C	/* BS: 0 BW: 1	*/
#define HDMI_RI_MATCH_RESULT			0x4090	/* BS: 0 BW: 2	*/
#define HDMI_HDCP14_BKSV_0			0x40A0	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_BKSV_1			0x40A4	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_BKSV_2			0x40A8	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_BKSV_3			0x40AC	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_BKSV_4			0x40B0	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_AKSV_0			0x40C0	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_AKSV_1			0x40C4	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_AKSV_2			0x40C8	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_AKSV_3			0x40CC	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_AN_0			0x40E0	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_AN_1			0x40E4	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_AN_2			0x40E8	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_AN_3			0x40EC	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_AN_4			0x40F0	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_AN_5			0x40F4	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_AN_6			0x40F8	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_AN_7			0x40FC	/* BS: 0 BW: 8	*/
#define HDMI_REPEATER				0x4100	/* BS: 6 BW: 1	*/
#define HDMI_READY				0x4100	/* BS: 5 BW: 1	*/
#define HDMI_FAST				0x4100	/* BS: 4 BW: 1	*/
#define HDMI_V1P1_FEATURES			0x4100	/* BS: 1 BW: 1	*/
#define HDMI_FAST_REAUTHENTICATION		0x4100	/* BS: 0 BW: 1	*/
#define HDMI_MAX_DEVS_EXCEEDED			0x4110	/* BS: 7 BW: 1	*/
#define HDMI_DEVICE_COUNT			0x4110	/* BS: 0 BW: 7	*/
#define HDMI_HDMI_MODE				0x4114	/* BS: 4 BW: 1	*/
#define HDMI_MAX_CASCADE_EXCEEDED		0x4114	/* BS: 3 BW: 1	*/
#define HDMI_DEPTH				0x4114	/* BS: 0 BW: 3	*/
#define HDMI_HDCP14_RI_0			0x4140	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_RI_1			0x4144	/* BS: 0 BW: 8	*/
#define HDMI_HDCP14_I2C_INT			0x4180	/* BS: 0 BW: 1	*/
#define HDMI_HDCP14_AN_INT			0x4190	/* BS: 0 BW: 1	*/
#define HDMI_HDCP14_WATCHDOG_INT		0x41A0	/* BS: 0 BW: 1	*/
#define HDMI_HDCP14_RI_INT			0x41B0	/* BS: 0 BW: 1	*/
#define HDMI_HDCP14_RI_COMPARE_0_COMPARE_EN	0x41D0	/* BS: 7 BW: 1	*/
#define HDMI_HDCP14_RI_COMPARE_0_FRAME_NUMBER	0x41D0	/* BS: 0 BW: 7	*/
#define HDMI_HDCP14_RI_COMPARE_1_COMPARE_EN	0x41D4	/* BS: 7 BW: 1	*/
#define HDMI_HDCP14_RI_COMPARE_1_FRAME_NUMBER	0x41D4	/* BS: 0 BW: 7	*/
#define HDMI_FRAME_COUNT			0x41E0	/* BS: 0 BW: 7	*/
#define HDMI_HDCP14_SYS_EN			0x41E4	/* BS: 0 BW: 1	*/
#define HDMI_HDCP14_ENCRYPTION_ENABLE		0x41E8	/* BS: 0 BW: 1	*/
#define HDMI_AN_SEED_SEL			0x41EC	/* BS: 0 BW: 1	*/
#define HDMI_AN_RANDOM_SEED_0			0x41F0	/* BS: 0 BW: 8	*/
#define HDMI_AN_RANDOM_SEED_1			0x41F4	/* BS: 0 BW: 8	*/
#define HDMI_AN_RANDOM_SEED_2			0x41F8	/* BS: 0 BW: 8	*/
#define HDMI_SHA1_FORCE_CLKGATE_ON		0x41FC	/* BS: 1 BW: 1	*/
#define HDMI_SHA1_AUTO_CLKGATE_MODE		0x41FC	/* BS: 0 BW: 1	*/
#define HDMI_SYS_EN				0x5000	/* BS: 0 BW: 1	*/
#define HDMI_LC128_LOAD_DONE			0x5004	/* BS: 1 BW: 1	*/
#define HDMI_HDCP22_KEY_LOAD_DONE		0x5004	/* BS: 0 BW: 1	*/
#define HDMI_HDCP22_KEY_LOAD_DONE_INT		0x5008	/* BS: 0 BW: 1	*/
#define HDMI_HDCP22_KEY_LOAD_DONE_INT_CLEAR	0x500C	/* BS: 0 BW: 1	*/
#define HDMI_KEY_LOAD_DONE_INT_EN		0x5010	/* BS: 0 BW: 1	*/
#define HDMI_SRAM_LOAD_CYCLE			0x5014	/* BS: 0 BW: 8	*/
#define HDMI_PIXEL_TO_PIXEL			0x5018	/* BS: 3 BW: 1	*/
#define HDMI_PIXEL_TO_PACKET			0x5018	/* BS: 2 BW: 1	*/
#define HDMI_PACKET_TO_PIXEL			0x5018	/* BS: 1 BW: 1	*/
#define HDMI_PACKET_TO_PACKET			0x5018	/* BS: 0 BW: 1	*/
#define HDMI_INPUT_CTR_RESET			0x501C	/* BS: 0 BW: 1	*/
#define HDMI_AES_ADDRESS			0x5020	/* BS: 0 BW: 13	*/
#define HDMI_LC28_ADDRESS			0x5024	/* BS: 0 BW: 13	*/
#define HDMI_AES_AUTO_CLOCK_GATING		0x5028	/* BS: 0 BW: 1	*/
#define HDMI_HDCP22_ENCRYPTION_ENABLE		0x5064	/* BS: 0 BW: 1	*/
#define HDMI_RIV_KEY_0_REGISTER			0x5104	/* BS: 0 BW: 32	*/
#define HDMI_RIV_KEY_1_REGISTER			0x5108	/* BS: 0 BW: 32	*/
#define HDMI_AES_KEY_0_REGISTER			0x5200	/* BS: 0 BW: 32	*/
#define HDMI_AES_KEY_1_REGISTER			0x5204	/* BS: 0 BW: 32	*/
#define HDMI_AES_KEY_2_REGISTER			0x5208	/* BS: 0 BW: 32	*/
#define HDMI_AES_KEY_3_REGISTER			0x520C	/* BS: 0 BW: 32	*/
#define HDMI_KS_KEY_0_REGISTER			0x5210	/* BS: 0 BW: 32	*/
#define HDMI_KS_KEY_1_REGISTER			0x5214	/* BS: 0 BW: 32	*/
#define HDMI_KS_KEY_2_REGISTER			0x5218	/* BS: 0 BW: 32	*/
#define HDMI_KS_KEY_3_REGISTER			0x521C	/* BS: 0 BW: 32	*/
#define HDMI_BLANK_TO_IDLE			0x5300	/* BS: 7 BW: 1	*/
#define HDMI_NEW_FRAME				0x5300	/* BS: 6 BW: 1	*/
#define HDMI_VIDEO_TO_BLANK			0x5300	/* BS: 5 BW: 1	*/
#define HDMI_AUX_TO_BLANK			0x5300	/* BS: 4 BW: 1	*/
#define HDMI_VIDEO_ENC				0x5300	/* BS: 3 BW: 1	*/
#define HDMI_AUX_ENC				0x5300	/* BS: 2 BW: 1	*/
#define HDMI_ENC_STANDBY			0x5300	/* BS: 1 BW: 1	*/
#define HDMI_FIRST_FRAME			0x5300	/* BS: 0 BW: 1	*/
#define HDMI_FRAME_NUMBER			0x5304	/* BS: 0 BW: 12	*/
#define HDMI_AES_START				0x6000	/* BS: 0 BW: 1	*/
#define HDMI_AES_DATA_SIZE_L			0x6020	/* BS: 0 BW: 8	*/
#define HDMI_AES_DATA_SIZE_H			0x6024	/* BS: 0 BW: 1	*/
#define HDMI_AES_DATA				0x6040	/* BS: 0 BW: 8	*/
#define HDMI_HDCP_OFFSET_TX_0			0x4160	/* BS: 0 BW: 8	*/
#define HDMI_HDCP_OFFSET_TX_1			0x4164	/* BS: 0 BW: 8	*/
#define HDMI_HDCP_OFFSET_TX_2			0x4168	/* BS: 0 BW: 8	*/
#define HDMI_HDCP_OFFSET_TX_3			0x416C	/* BS: 0 BW: 8	*/
#define HDMI_HDCP_CYCLE_AA			0x4170	/* BS: 0 BW: 8	*/

/*
 * @ HDMI-CEC
 * - BASE : 20011000
 *
 * @ refer to __nx_cpuif_nx_cec_regmap_struct__
 */
#define HDMI_CEC_TX_ERROR			0x000	/* BS: 3 BW: 1 */
#define HDMI_CEC_TX_DONE			0x000	/* BS: 2 BW: 1 */
#define HDMI_CEC_TX_TRANSFERRING		0x000	/* BS: 1 BW: 1 */
#define HDMI_CEC_TX_RUNNING			0x000	/* BS: 0 BW: 1 */
#define HDMI_CEC_TX_BYTES_TRANSFERRED		0x004	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BCAST			0x008	/* BS: 4 BW: 1 */
#define HDMI_CEC_RX_ERROR			0x008	/* BS: 3 BW: 1 */
#define HDMI_CEC_RX_DONE			0x008	/* BS: 2 BW: 1 */
#define HDMI_CEC_RX_RECEIVING			0x008	/* BS: 1 BW: 1 */
#define HDMI_CEC_RX_RUNNING			0x008	/* BS: 0 BW: 1 */
#define HDMI_CEC_RX_BYTES_RECEIVED		0x00C	/* BS: 0 BW: 8 */
#define HDMI_CEC_MASK_INTR_RX_ERROR		0x010	/* BS: 5 BW: 1 */
#define HDMI_CEC_MASK_INTR_RX_DONE		0x010	/* BS: 4 BW: 1 */
#define HDMI_CEC_MASK_INTR_TX_ERROR		0x010	/* BS: 1 BW: 1 */
#define HDMI_CEC_MASK_INTR_TX_DONE		0x010	/* BS: 0 BW: 1 */
#define HDMI_CEC_CLEAR_INTR_RX_ERROR		0x014	/* BS: 5 BW: 1 */
#define HDMI_CEC_CLEAR_INTR_RX_DONE		0x014	/* BS: 4 BW: 1 */
#define HDMI_CEC_CLEAR_INTR_TX_ERROR		0x014	/* BS: 1 BW: 1 */
#define HDMI_CEC_CLEAR_INTR_TX_DONE		0x014	/* BS: 0 BW: 1 */
#define HDMI_CEC_LOGIC_ADDR			0x020	/* BS: 0 BW: 4 */
#define HDMI_CEC_DIVISOR_7_0_			0x030	/* BS: 0 BW: 8 */
#define HDMI_CEC_DIVISOR_15_8_			0x034	/* BS: 0 BW: 8 */
#define HDMI_CEC_DIVISOR_23_16_			0x038	/* BS: 0 BW: 8 */
#define HDMI_CEC_DIVISOR_31_24_			0x03C	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_RETRANS_NUM			0x040	/* BS: 4 BW: 3 */
#define HDMI_CEC_TX_BCAST			0x040	/* BS: 1 BW: 1 */
#define HDMI_CEC_TX_RESET			0x040	/* BS: 7 BW: 1 */
#define HDMI_CEC_TX_START			0x040	/* BS: 0 BW: 1 */
#define HDMI_CEC_TX_BYTE_NUM			0x044	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_WAIT			0x060	/* BS: 7 BW: 1 */
#define HDMI_CEC_TX_SENDING_START_BIT		0x060	/* BS: 6 BW: 1 */
#define HDMI_CEC_TX_SENDING_HDR_BLK		0x060	/* BS: 5 BW: 1 */
#define HDMI_CEC_TX_SENDING_DATA_BLK		0x060	/* BS: 4 BW: 1 */
#define HDMI_CEC_TX_LASTEST_INITIATOR		0x060	/* BS: 3 BW: 1 */
#define HDMI_CEC_TX_WAIT_SFT_SUCC		0x064	/* BS: 6 BW: 1 */
#define HDMI_CEC_TX_WAIT_SFT_NEW		0x064	/* BS: 5 BW: 1 */
#define HDMI_CEC_TX_WAIT_SFT_RETRAN		0x064	/* BS: 4 BW: 1 */
#define HDMI_CEC_TX_RETRANS_CNT			0x064	/* BS: 1 BW: 3 */
#define HDMI_CEC_TX_ACK_FAILED			0x064	/* BS: 0 BW: 1 */
#define HDMI_CEC_TX_BLOCK_0			0x080	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_1			0x084	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_2			0x088	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_3			0x08C	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_4			0x090	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_5			0x094	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_6			0x098	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_7			0x09C	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_8			0x0A0	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_9			0x0A4	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_10			0x0A8	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_11			0x0AC	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_12			0x0B0	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_13			0x0B4	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_14			0x0B8	/* BS: 0 BW: 8 */
#define HDMI_CEC_TX_BLOCK_15			0x0BC	/* BS: 0 BW: 8 */
#define HDMI_CEC_CHECK_SAMPLING_ERROR		0x0C0	/* BS: 6 BW: 1 */
#define HDMI_CEC_CHECK_LOW_TIME_ERROR		0x0C0	/* BS: 5 BW: 1 */
#define HDMI_CEC_CHECK_START_BIT_ERROR		0x0C0	/* BS: 4 BW: 1 */
#define HDMI_CEC_LB_BIT_ERROR_INT_EN		0x0C0	/* BS: 3 BW: 1 */
#define HDMI_CEC_RX_HOST_BUSY			0x0C0	/* BS: 1 BW: 1 */
#define HDMI_CEC_RX_ENABLE			0x0C0	/* BS: 0 BW: 1 */
#define HDMI_CEC_RX_RESET			0x0C0	/* BS: 7 BW: 1 */
#define HDMI_CEC_LB_TIME_LIMIT			0x0C4	/* BS: 1 BW: 3 */
#define HDMI_CEC_LB_TIME_LIMIT_EN		0x0C4	/* BS: 0 BW: 1 */
#define HDMI_CEC_RX_WAITING			0x0E0	/* BS: 7 BW: 1 */
#define HDMI_CEC_RX_RECEIVING_START_BIT		0x0E0	/* BS: 6 BW: 1 */
#define HDMI_CEC_RX_RECEIVING_HDR_BLK		0x0E0	/* BS: 5 BW: 1 */
#define HDMI_CEC_RX_RECEIVING_DATA_BLK		0x0E0	/* BS: 4 BW: 1 */
#define HDMI_CEC_SAMPLING_ERROR			0x0E4	/* BS: 6 BW: 1 */
#define HDMI_CEC_LOW_TIME_ERROR			0x0E4	/* BS: 5 BW: 1 */
#define HDMI_CEC_START_BIT_ERROR		0x0E4	/* BS: 4 BW: 1 */
#define HDMI_CEC_LB_TIME_ERROR			0x0E4	/* BS: 3 BW: 1 */
#define HDMI_CEC_LINE_ERROR			0x0E4	/* BS: 0 BW: 1 */
#define HDMI_CEC_RX_BLOCK_0			0x100	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_1			0x104	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_2			0x108	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_3			0x10C	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_4			0x110	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_5			0x114	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_6			0x118	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_7			0x11C	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_8			0x120	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_9			0x124	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_10			0x128	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_11			0x12C	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_12			0x130	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_13			0x134	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_14			0x138	/* BS: 0 BW: 8 */
#define HDMI_CEC_RX_BLOCK_15			0x13C	/* BS: 0 BW: 8 */
#define HDMI_CEC_FILTER_CUR_VAL			0x180	/* BS: 7 BW: 1 */
#define HDMI_CEC_FILTER_ENABLE			0x180	/* BS: 0 BW: 1 */
#define HDMI_CEC_FILTER_TH			0x184	/* BS: 0 BW: 8 */

/*
 * @ HDMI-PHY
 * - BASE : 20ED0000
 *
 * @ refer to __nx_cpuif_nx_hdmiphy_regmap_struct__
 */
#define HDMI_PHY_REG04		0x04
#define HDMI_PHY_REG6C		0x6C
#define HDMI_PHY_REG70		0x70
#define HDMI_PHY_REG74		0x74
#define HDMI_PHY_REG78		0x78
#define HDMI_PHY_REG84		0x84
#define HDMI_PHY_REG88		0x88

#define HDMI_PHY_MODE_SET	HDMI_PHY_REG84	/* BS:7 BW:1 */
#define HDMI_PHY_PD_EN		HDMI_PHY_REG74	/* BS:1 BW:1 */
#define HDMI_PHY_DRV_CLK	HDMI_PHY_REG70	/* BS:6 BW:1 */
#define HDMI_PHY_DRV_DATA	HDMI_PHY_REG70	/* BS:7 BW:1 */


#define HDMI_PHY_TXCK_AMP	0xb0	/* BS:3 BW:5 */
#define HDMI_PHY_TXCK_EMP	0xb4	/* BS:0 BW:4 */
#define HDMI_PHY_TXD0_AMP	0xc0	/* BS:3 BW:5 */
#define HDMI_PHY_TXD0_EMP	0xc4	/* BS:0 BW:4 */
#define HDMI_PHY_TXD1_AMP	0xd0	/* BS:3 BW:5 */
#define HDMI_PHY_TXD1_EMP	0xd4	/* BS:0 BW:4 */
#define HDMI_PHY_TXD2_AMP	0xe0	/* BS:3 BW:5 */
#define HDMI_PHY_TXD2_EMP	0xe4	/* BS:0 BW:4 */


/*
 * @ HDMI-TIEOFF
 * - BASE : 20EC8000
 *
 * @ refer to __nx_cpuif_hdmi_tieoff_regmap_struct__
 */
#define	HDMI_TIEOFF_HBR_ENB			28	/* BW: 1 */
#define	HDMI_TIEOFF_USE_INTERNAL		27	/* BW: 1 */
#define	HDMI_TIEOFF_NXS2HDMI_PWDN_REQ		26	/* BW: 1 */
#define	HDMI_TIEOFF_SPSRAM_EMAW			23	/* BW: 2 */
#define	HDMI_TIEOFF_SPSRAM_EMA			20	/* BW: 3 */
#define	HDMI_TIEOFF_SRAM_NPOWERDOWN		15	/* BW: 3 */
#define	HDMI_TIEOFF_SRAM_NSLEEP			12	/* BW: 3 */
#define	HDMI_TIEOFF_PHY_EN			1	/* BW: 1 */
#define	HDMI_TIEOFF_PWDN_REQ			0	/* BW: 1 */
#define	HDMI_TIEOFF_NXS2HDMI_PWDN_ACK		25	/* BW: 1 */
#define	HDMI_TIEOFF_PHY_CLK_RDY			11	/* BW: 1 */
#define	HDMI_TIEOFF_PHY_PLL_LOCK		10	/* BW: 1 */
#define	HDMI_TIEOFF_PHY_READY			9	/* BW: 1 */
#define	HDMI_TIEOFF_PHY_AFC_CODE		4	/* BW: 5 */
#define	HDMI_TIEOFF_PHY_SINK_DET		3	/* BW: 1 */
#define	HDMI_TIEOFF_PWDN_ACK			2	/* BW: 1 */

/* audio packet */
#define HDMI_AUI_DATA_CC_HEADER			(0 << 0)
#define HDMI_AUI_DATA_CC_2CH			(0x1 << 0)
#define HDMI_AUI_DATA_CC_3CH			(0x2 << 0)
#define HDMI_AUI_DATA_CC_4CH			(0x3 << 0)
#define HDMI_AUI_DATA_CC_5CH			(0x4 << 0)
#define HDMI_AUI_DATA_CC_6CH			(0x5 << 0)
#define HDMI_AUI_DATA_CC_7CH			(0x6 << 0)
#define HDMI_AUI_DATA_CC_8CH			(0x7 << 0)
#define HDMI_AUI_DATA_SS_16BIT			(0x1 << 0)
#define HDMI_AUI_DATA_SS_20BIT			(0x2 << 0)
#define HDMI_AUI_DATA_SS_24BIT			(0x3 << 0)
#define HDMI_AUI_DATA_FS_32			(0x1 << 2)
#define HDMI_AUI_DATA_FS_44_1			(0x2 << 2)
#define HDMI_AUI_DATA_FS_48			(0x3 << 2)
#define HDMI_AUI_DATA_FS_96			(0x5 << 2)
#define HDMI_AUI_DATA_CA_2CH			0x00
#define HDMI_AUI_DATA_CA_3CH			0x01
#define HDMI_AUI_DATA_CA_4CH			0x03
#define HDMI_AUI_DATA_CA_5CH			0x07
#define HDMI_AUI_DATA_CA_6CH			0x0B
#define HDMI_AUI_DATA_CA_7CH			0x0F
#define HDMI_AUI_DATA_CA_8CH			0x13

/* HDMI_AUDIO_CH0_ST_SH_0 ... HDMI_AUDIO_CH7_ST_SH_0 */
#define HDMI_AUDIO_CHST_NON_LINEAR		(1<<1)
#define HDMI_AUDIO_CHST_32KHZ		(1<<24 | 1<<25 | 0<<26 | 0<<27)
#define HDMI_AUDIO_CHST_44KHZ		(0<<24 | 0<<25 | 0<<26 | 0<<27)
#define HDMI_AUDIO_CHST_48KHZ		(0<<24 | 1<<25 | 0<<26 | 0<<27)
#define HDMI_AUDIO_CHST_88KHZ		(0<<24 | 0<<25 | 0<<26 | 1<<27)
#define HDMI_AUDIO_CHST_96KHZ		(0<<24 | 1<<25 | 0<<26 | 1<<27)
#define HDMI_AUDIO_CHST_176KHZ		(0<<24 | 0<<25 | 1<<26 | 1<<27)
#define HDMI_AUDIO_CHST_192KHZ		(0<<24 | 1<<25 | 1<<26 | 1<<27)
#define HDMI_AUDIO_CHST_768KHZ		(1<<24 | 0<<25 | 0<<26 | 1<<27)

/* HDMI_AUDIO_CH0_ST_SH_1 ... HDMI_AUDIO_CH7_ST_SH_1 */
#define HDMI_AUDIO_CHST_MAX_24BITS		(1<<0)

#define HDMI_AUDIO_CHST_MAX24_20BITS		(0x1 << 1)
#define HDMI_AUDIO_CHST_MAX24_22BITS		(0x2 << 1)
#define HDMI_AUDIO_CHST_MAX24_23BITS		(0x4 << 1)
#define HDMI_AUDIO_CHST_MAX24_24BITS		(0x5 << 1)
#define HDMI_AUDIO_CHST_MAX24_21BITS		(0x6 << 1)
#define HDMI_AUDIO_CHST_MAX20_16BITS		(0x1 << 1)
#define HDMI_AUDIO_CHST_MAX20_18BITS		(0x2 << 1)
#define HDMI_AUDIO_CHST_MAX20_19BITS		(0x4 << 1)
#define HDMI_AUDIO_CHST_MAX20_20BITS		(0x5 << 1)
#define HDMI_AUDIO_CHST_MAX20_17BITS		(0x6 << 1)

/* HDMI_AUDIO_CH0_ST_SH_1 ... HDMI_AUDIO_CH7_ST_SH_1  at SPDIF*/
#define HDMI_AUDIO_CHST_SPDIF_MAX20_16BITS	0x2
#define HDMI_AUDIO_CHST_SPDIF_MAX20_17BITS	0xC
#define HDMI_AUDIO_CHST_SPDIF_MAX20_18BITS	0x4
#define HDMI_AUDIO_CHST_SPDIF_MAX20_19BITS	0x8
#define HDMI_AUDIO_CHST_SPDIF_MAX20_20BITS	0xA
#define HDMI_AUDIO_CHST_SPDIF_MAX24_20BITS	0x3
#define HDMI_AUDIO_CHST_SPDIF_MAX24_21BITS	0xD
#define HDMI_AUDIO_CHST_SPDIF_MAX24_22BITS	0x5
#define HDMI_AUDIO_CHST_SPDIF_MAX24_23BITS	0x9
#define HDMI_AUDIO_CHST_SPDIF_MAX24_24BITS	0xB

/* SPDIFIN_CLK_CTRL */
#define HDMI_SPDIFIN_CFG_NOISE_FILTER_2_SAMPLE			(1 << 6)
#define HDMI_SPDIFIN_CFG_PCPD_MANUAL				(1 << 4)
#define HDMI_SPDIFIN_CFG_HDMI_AUDIO_BIT_LEN_MANUAL			(1 << 3)
#define HDMI_SPDIFIN_CFG_UVCP_REPORT				(1 << 2)
#define HDMI_SPDIFIN_CFG_HDMI_2_BURST				(1 << 1)
#define HDMI_SPDIFIN_CFG_DATA_ALIGN_32				(1 << 0)

#endif
