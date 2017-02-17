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
#ifndef __NXS_SYNCGEN_H__
#define __NXS_SYNCGEN_H__

struct nxs_syncgen_reg0 {
	u32 htotal;	/* 0x00:16: Horizontal Total Length */
	u32 hswidth;	/* 0x04:16: Horizontal Sync Width */
	u32 hastart;	/* 0x08:16: Horizontal Active Video START */
	u32 haend;	/* 0x0C:16: Horizontal Active Video End */
	u32 vtotal;	/* 0x10:16: Vertical Total Length */
	u32 vswidth;	/* 0x14:16: Vertical Sync Width */
	u32 vastart;	/* 0x18:16: Vertical Active Video START */
	u32 vaend;	/* 0x1C:16: Vertical Active Video End */

	/* 0x20: Control 0 */
	struct {
		u32 dpcenb_delay : 16;			/* bp: 16 */
		u32 dirtyflag_top_clr : 1;		/* bp: 15 */
		u32 fcs_enable : 1;			/* bp: 14 */
		u32 no_swidth_mode : 1;			/* bp: 13 */
		u32 dirtyflag_top : 1;			/* bp: 12 */
		u32 seavenb : 1;			/* bp: 8 */
		u32 fieldmode : 1;			/* bp: 6 */
		u32 fieldignore : 1;			/* bp: 5 */
		u32 intermode : 1;			/* bp: 4 */
		u32 legacy_encenb_syncenb : 1;		/* bp: 3 */
		u32 dpcenb_mode : 1;			/* bp: 2 */
		u32 nxs_dec_enb : 1;			/* bp: 1 */
		u32 dpcenb : 1;				/* bp: 0 */
	} ctrl_0;

	/* 0x24: Control 1 */
	struct {
		u32 pol_ntsc_hso : 1;			/* bp: 13 */
		u32 pol_ntsc_vso : 1;			/* bp: 12 */
		u32 pol_ntsc_fso : 1;			/* bp: 11 */
		u32 other_dpc_sel : 3;			/* bp: 8 */
		u32 use_other_dpcenb : 1;		/* bp: 7 */
		u32 use_other_vsync : 1;		/* bp: 6 */
		u32 use_vcntclear : 1;			/* bp: 5 */
		u32 use_psyncrestart : 1;		/* bp: 4 */
		u32 dec_2pixel_1clock : 1;		/* bp: 3 */
		u32 encenb : 1;				/* bp: 2 */
		u32 use_ntsc_sync : 1;			/* bp: 1 */
		u32 use_ntsc_sync_cntclr : 1;		/* bp: 0 */
	} ctrl_1;

	u32 use_even_vconfig;	/* 0x28: 1: Even Field Vertical Control */
	u32 evtotal;		/* 0x2C: 16: Even Field V Total Length */
	u32 evswidth;		/* 0x30: 16: Even Field V Sync Width */
	u32 evastart;		/* 0x34: 16: Even Field V Active Video START */
	u32 evaend;		/* 0x38: 16: Even Field V Active Video End */
	u32 use_vsetclr;	/* 0x3C: 1: Vertical Sync Set/Clr Control */
	u32 vsetpixel;		/* 0x40: 16: Vertical Sync End Offset */
	u32 vclrpixel;		/* 0x44: 16: Vertical Sync START Offset */
	u32 evenvsetpixel;	/* 0x48: 16: Even Field V Sync End Offset */
	u32 evenvclrpixel;	/* 0x4C: 16: Even Field V Sync START Offset */

	/* 0x50: Sync Polarity Control */
	struct {
		u32 polfield : 1;	/* bp: 6 */
		u32 polhsync : 1;	/* bp: 5 */
		u32 polvsync : 1;	/* bp: 4 */
		u32 padpolde : 1;	/* bp: 3 */
		u32 padpolfield : 1;	/* bp: 2 */
		u32 padpolhsync : 1;	/* bp: 1 */
		u32 padpolvsync : 1;	/* bp: 0 */
	} polctrl;

	/* 0x54: Sync Delay  */
	struct {
		u32 delayde : 6;	/* bp: 24 */
		u32 delayrgb : 4;	/* bp: 16 */
		u32 delayvs : 6;	/* bp: 8 */
		u32 delayhs : 6;	/* bp: 0 */
	} delay;

	/* 0x58: VSYNC Interrupt Control */
	struct {
		u32 int_3_intdisable : 1;	/* bp: 14 */
		u32 int_3_intenb : 1;		/* bp: 13 */
		u32 int_3_intpend : 1;		/* bp: 12 */
		u32 int_2_intdisable : 1;	/* bp: 10 */
		u32 int_2_intenb : 1;		/* bp: 9 */
		u32 int_2_intpend : 1;		/* bp: 8 */
		u32 int_1_intdisable : 1;	/* bp: 6 */
		u32 int_1_intenb : 1;		/* bp: 5 */
		u32 int_1_intpend : 1;		/* bp: 4 */
		u32 vblank_intdisable : 1;	/* bp: 2 */
		u32 vblank_intenb : 1;		/* bp: 1 */
		u32 vblank_intpend_clr : 1;	/* bp: 0 */
	} intr_0;

	/* 0x5C: Interrupt Control */
	struct {
		u32 dec_fs_intdisable : 1;		/* bp: 26 */
		u32 dec_fs_intenb : 1;			/* bp: 25 */
		u32 dec_fs_intpend : 1;			/* bp: 24 */
		u32 err_sec_intdisable : 1;		/* bp: 22 */
		u32 err_sec_intenb : 1;			/* bp: 21 */
		u32 err_sec_intpend : 1;		/* bp: 20 */
		u32 err_dec_intdisable : 1;		/* bp: 18 */
		u32 err_dec_intenb : 1;			/* bp: 17 */
		u32 err_dec_intpend : 1;		/* bp: 16 */
		u32 idle_intdisable : 1;		/* bp: 14 */
		u32 idle_intenb : 1;			/* bp: 13 */
		u32 idle_intpend : 1;			/* bp: 12 */
		u32 update_intdisable : 1;		/* bp: 10 */
		u32 update_intenb : 1;			/* bp: 9 */
		u32 update_intpend : 1;			/* bp: 8 */
		u32 err_underflow_intdisable : 1;	/* bp: 6 */
		u32 err_underflow_intenb : 1;		/* bp: 5 */
		u32 err_underflow_intpend : 1;		/* bp: 4 */
		u32 err_fcs_intdisable : 1;		/* bp: 2 */
		u32 err_fcs_intenb : 1;			/* bp: 1 */
		u32 err_fcs_intpend_clr : 1;		/* bp: 0 */
	} intr_1;

	/* 0x60: Interrupt Mode Control */
	struct {
		u32 int_3_intset_mode : 1;		/* bp: 10 */
		u32 int_2_intset_mode : 1;		/* bp: 9 */
		u32 int_1_intset_mode : 1;		/* bp: 8 */
		u32 vblank_int_intset_mode : 1;		/* bp: 7 */
		u32 dec_fs_intset_mode : 1;		/* bp: 6 */
		u32 sec_err_intset_mode : 1;		/* bp: 5 */
		u32 dec_err_intset_mode : 1;		/* bp: 4 */
		u32 idle_intset_mode : 1;		/* bp: 3 */
		u32 updateconfig_intset_mode : 1;	/* bp: 2 */
		u32 underflow_intset_mode : 1;		/* bp: 1 */
		u32 fcs_err_intset_mode : 1;		/* bp: 0 */
	} intmode;

	u32 int_1_vcnt;	/* 0x64: 16: Interrupt 1 Config  */
	u32 int_2_vcnt;	/* 0x68: 16: Interrupt 2 Config */
	u32 int_3_vcnt;	/* 0x6C: 16: Interrupt 1 Config */
	u32 cur_fcs;	/* 0x70: 32: FCS DEBUG  */
	u32 pre_1_fcs;	/* 0x74: 32: FCS DEBUG */
	u32 pre_2_fcs;	/* 0x78: 32: FCS DEBUG */
	u32 fcs_poly;	/* 0x7C: 32: FCS POLY */

	/* 0x80: NXS DECODER ERR Debug */
	struct {
		u32 dec_err_status : 7;		/* bp: 4 */
		u32 dec_err_state : 4;		/* bp: 0 */
	} decerr;

	/* 0x84: NXS DECODER DITHER Control */
	struct {
		u32 nxs_dither_enb : 1;		/* bp: 16 */
		u32 nxs_dither_table : 16;	/* bp: 0 */
	} decdither;

	/* 0x88: SyncGen Cnt Debug */
	struct {
		u32 syncgen_vcnt : 16;		/* bp: 16 */
		u32 syncgen_hcnt : 16;		/* bp: 0 */
	} debug_0;

	/* 0x8C: SyncGen Field Control */
	struct {
		u32 dec_field : 2;		/* bp: 1 */
		u32 nxs_field_ignore : 1;	/* bp: 0 */
	} debug_1;

	/* 0x90: SyncGen Debug Count */
	struct {
		u32 dbg_lcnt : 15;		/* bp: 16 */
		u32 dbg_fcnt : 9;		/* bp: 0 */
	} debug_2;

	/* 0x94:ASYNCFIFO Min Max  */
	struct {
		u32 af_data_max : 10;		/* bp: 16 */
		u32 af_data_min : 10;		/* bp: 0 */
	} valminmax;

	/* 0x98: DEBUG */
	struct {
		u32 mlcinit_period : 5;		/* bp: 20 */
		u32 idle : 1;			/* bp: 19 */
		u32 internal_field : 1;		/* bp: 18 */
		u32 af_syncgen_off : 1;		/* bp: 17 */
		u32 buffed_dpcenb : 1;		/* bp: 16 */
		u32 buffed_intermode : 1;	/* bp: 15 */
		u32 buffed_frameout : 1;	/* bp: 14 */
		u32 syncgen_field : 2;		/* bp: 12 */
		u32 af_fs : 1;			/* bp: 11 */
		u32 af_field_okay : 1;		/* bp: 10 */
		u32 af_flush : 1;		/* bp: 9 */
		u32 af_field : 2;		/* bp: 7 */
		u32 af_idle : 1;		/* bp: 6 */
		u32 is_hbp : 1;			/* bp: 5 */
		u32 is_hsw : 1;			/* bp: 4 */
		u32 is_hfp : 1;			/* bp: 3 */
		u32 is_vbp : 1;			/* bp: 2 */
		u32 is_vsw : 1;			/* bp: 1 */
		u32 is_vfp : 1;			/* bp: 0 */
	} debug_3;

	/* 0x9C: DPCENB Tearing Effect Control */
	struct {
		u32 te_mode_enable : 1;		/* bp: 24 */
		u32 te_dpcenb_hold : 8;		/* bp: 16 */
		u32 te_delay : 16;		/* bp: 0 */
	} te;

	/* 0xA0: Decoder Debug */
	struct {
		u32 dec_width : 16; /* bp: 16 */
		u32 dec_height : 1; /* bp: 0 */
	} debug4;

	/* 0xA4: NTSC Syncgen Control */
	struct {
		u32 use_delayed_ntsc_hso : 1;	/* bp: 3 */
		u32 use_delayed_ntsc_vso : 1;	/* bp: 2 */
		u32 use_delayed_ntsc_fso : 1;	/* bp: 1 */
		u32 wait_for_neg_vso : 1;	/* bp: 0 */
	} ntscctrl;
};

struct nxs_syncgen_reg1 {
	/* 0x00: DPCFORMATDEBUG0 */
	struct {
		u32 use_even_pixel : 1;		/* bp : 20 */
		u32 i80_outenable : 1;		/* bp : 19 */
		u32 tft_outenable : 1;		/* bp : 18 */
		u32 o_pad_field : 1;		/* bp : 17 */
		u32 i80_enable : 1;		/* bp : 16 */
		u32 chroma_down_sampling : 1;	/* bp :  9 */
		u32 rgbmode : 1;		/* bp :  8 */
		u32 ycorder : 1;		/* bp :  7 */
		u32 rgb2yuv : 1;		/* bp :  6 */
		u32 swaprb : 1;			/* bp :  5 */
		u32 rangergb2yc : 1;		/* bp :  4 */
		u32 format : 4;			/* bp :  0 */
	} formatdebug_0;

	/* 0x04: Dither */
	struct {
		u32 dither_enb : 1;		/* bp:  6 */
		u32 bdither : 2;		/* bp:  4 */
		u32 gdither : 2;		/* bp:  2 */
		u32 rdither : 2;		/* bp:  0 */
	} dither;

	/* 0x08 : PAD LOCATION CONTROL0*/
	struct {
		u32 use_padloc : 1;		/* bp: 16 */
		u32 padloc2 : 5;		/* bp: 10 */
		u32 padloc1 : 5;		/* bp:  5 */
		u32 padloc0 : 5;		/* bp:  0 */
	} padloc_0_2;

	/* 0x0c: PAD LOCATION CONTROL2 */
	struct {
		u32 padloc5 : 5;		/* bp: 10 */
		u32 padloc4 : 5;		/* bp:  5 */
		u32 padloc3 : 5;		/* bp:  0 */
	} padloc_3_5;

	/* 0x10: PAD LOCATION CONTROL2 */
	struct {
		u32 padloc8 : 5;		/* bp: 10 */
		u32 padloc7 : 5;		/* bp:  5 */
		u32 padloc6 : 5;		/* bp:  0 */
	} padloc_6_8;

	/* 0x14: PAD LOCATION CONTROL2 */
	struct {
		u32 padloc11 : 5;		/* bp: 10 */
		u32 padloc10 : 5;		/* bp:  5 */
		u32 padloc9 : 5;		/* bp:  0 */
	} padloc_9_11;

	/* 0x18: PAD LOCATION CONTROL2 */
	struct {
		u32 padloc14 : 5;		/* bp: 10 */
		u32 padloc13 : 5;		/* bp:  5 */
		u32 padloc12 : 5;		/* bp:  0 */
	} padloc_12_14;

	/* 0x1c: PAD LOCATION CONTROL2 */
	struct {
		u32 padloc17 : 5;		/* bp: 10 */
		u32 padloc16 : 5;		/* bp:  5 */
		u32 padloc15 : 5;		/* bp:  0 */
	} padloc_15_17;

	/* 0x20: PAD LOCATION CONTROL2 */
	struct {
		u32 padloc20 : 5;		/* bp: 10 */
		u32 padloc19 : 5;		/* bp:  5 */
		u32 padloc18 : 5;		/* bp:  0 */
	} padloc_18_20;

	/* 0x24: PAD LOCATION CONTROL2 */
	struct {
		u32 padloc23 : 5;		/* bp: 10 */
		u32 padloc22 : 5;		/* bp:  5 */
		u32 padloc21 : 5;		/* bp:  0 */
	} padloc_21_23;

	/* 0x28: PAD LOCATION MASK0 */
	struct {
		u32 rgbshift : 5;		/* bp: 24 */
		u32 rgbmask : 24;		/* bp:  0 */
	} rgbmask0;

	/* 0x2c: SyncGen YUV Min */
	struct {
		u32 min_y : 8;			/* bp: 16 */
		u32 min_cb : 8;			/* bp:  8 */
		u32 min_cr : 8;			/* bp:  0 */
	} minyuv;

	/* 0x30: SyncGen YUV Max */
	struct {
		u32 max_y : 8;			/* bp: 16 */
		u32 max_cb : 8;			/* bp:  8 */
		u32 max_cr : 8;			/* bp:  0 */
	} maxyuv;
};

#endif
