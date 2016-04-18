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
#ifndef _S5PXX18_SOC_DP_H_
#define _S5PXX18_SOC_DP_H_

#include "s5pxx18_soc_mlc.h"
#include "s5pxx18_soc_dpc.h"
#include "s5pxx18_soc_disptop.h"
#include "s5pxx18_soc_disp.h"

/*
 * display sync info
 */
struct dp_sync_info {
	int h_active_len;
	int h_sync_width;
	int h_back_porch;
	int h_front_porch;
	int h_sync_invert;	/* default active low */
	int v_active_len;
	int v_sync_width;
	int v_back_porch;
	int v_front_porch;
	int v_sync_invert;	/* default active low */
	int pixel_clock_hz;	/* HZ */
	int interlace;
};

/*
 * display control info
 */
/* the data output format. */
#define	DPC_FORMAT_RGB555		0  /* RGB555 Format */
#define	DPC_FORMAT_RGB565		1  /* RGB565 Format */
#define	DPC_FORMAT_RGB666		2  /* RGB666 Format */
#define	DPC_FORMAT_RGB888		3  /* RGB888 Format */
#define	DPC_FORMAT_MRGB555A		4  /* MRGB555A Format */
#define	DPC_FORMAT_MRGB555B		5  /* MRGB555B Format */
#define	DPC_FORMAT_MRGB565		6  /* MRGB565 Format */
#define	DPC_FORMAT_MRGB666		7  /* MRGB666 Format */
#define	DPC_FORMAT_MRGB888A		8  /* MRGB888A Format */
#define	DPC_FORMAT_MRGB888B		9  /* MRGB888B Format */
#define	DPC_FORMAT_CCIR656		10 /* ITU-R BT.656 / 601(8-bit) */
#define	DPC_FORMAT_CCIR601A		12 /* ITU-R BT.601A */
#define	DPC_FORMAT_CCIR601B		13 /* ITU-R BT.601B */
#define	DPC_FORMAT_4096COLOR		1  /* 4096 Color Format */
#define	DPC_FORMAT_16GRAY		3  /* 16 Level Gray Format */

/* the data output order in case of ITU-R BT.656 / 601. */
#define	DPC_YCORDER_CbYCrY		0 /* Cb, Y, Cr, Y */
#define	DPC_YCORDER_CrYCbY		1 /* Cr, Y, Cb, Y */
#define	DPC_YCORDER_YCbYCr		2 /* Y, Cb, Y, Cr */
#define	DPC_YCORDER_YCrYCb		3 /* Y, Cr, Y, Cb */

/* the PAD output clock. */
#define	DPC_PADCLKSEL_VCLK		0 /* VCLK */
#define	DPC_PADCLKSEL_VCLK2		1 /* VCLK2 */

/* the PAD output delay. */
#define	DP_SYNC_DELAY_RGB_PVD		(1<<0)
#define	DP_SYNC_DELAY_HSYNC_CP1		(1<<1)
#define	DP_SYNC_DELAY_VSYNC_FRAM	(1<<2)
#define	DP_SYNC_DELAY_DE_CP		(1<<3)

struct dp_ctrl_info {
	/* clock generator */
	int clk_src_lv0;
	int clk_div_lv0;
	int clk_src_lv1;
	int clk_div_lv1;

	/* sync generator format */
	unsigned int out_format;
	int invert_field;	/* 0= Normal Field 1: Invert Field */
	int swap_rb;
	unsigned int yc_order;	/* for CCIR output */

	/* extern sync delay  */
	int delay_mask;		/* if not 0, set defalut delays */
	int d_rgb_pvd;		/* delay value RGB/PVD signal   , 0 ~ 16, 0 */
	int d_hsync_cp1;	/* delay value HSYNC/CP1 signal , 0 ~ 63, 12 */
	int d_vsync_fram;	/* delay value VSYNC/FRAM signal, 0 ~ 63, 12 */
	int d_de_cp2;		/* delay value DE/CP2 signal    , 0 ~ 63, 12 */

	/* extern sync delay */
	int vs_start_offset;	/* start veritcal sync offset, defatult 0 */
	int vs_end_offset;	 /* end veritcla sync offset  , defatult 0 */
	int ev_start_offset; /* start even veritcal sync offset, defatult 0 */
	int ev_end_offset;   /* end even veritcal sync offset, defatult 0 */

	/* pad clock seletor */
	int vck_select;		/* 0=vclk0, 1=vclk2 */
	int clk_inv_lv0;	/* OUTCLKINVn */
	int clk_delay_lv0;	/* OUTCLKDELAYn */
	int clk_inv_lv1;	/* OUTCLKINVn */
	int clk_delay_lv1;	/* OUTCLKDELAYn */
	int clk_sel_div1;	/* 0=clk1_inv, 1=clk1_div_2_ns */
};

/* this enumerates display type. */
enum dp_panel_type {
	do_panel_type_none,
	do_panel_type_lcd,
	do_panel_type_lvds,
	do_panel_type_mipi,
	do_panel_type_hdmi,
	do_panel_type_vidi,
};

struct dp_device_info {
	struct device *dev;
	int module;
	void *base;
	struct dp_sync_info sync;
	struct dp_ctrl_info ctrl;
	enum dp_panel_type panel_type;
	bool panel_mpu_lcd;
};

/*
 * plane'a top layer
 */
#define	PLANE_FLAG_RGB		(0<<0)
#define	PLANE_FLAG_VIDEO	(1<<0)
#define	PLANE_FLAG_UNKNOWN	(0xFFFFFFF)

struct dp_plane_top {
	struct device *dev;
	void *base;
	int module;
	int width;
	int height;
	int primary_plane;
	int video_prior;	/* 0: video>RGBn, 1: RGB0>video>RGB1,
				   2: RGB0 > RGB1 > vidoe .. */
	int num_planes;
	unsigned int plane_type[10];
	unsigned int plane_flag[10];
	struct list_head plane_list;
	unsigned int back_color;
	unsigned int color_key;
	int interlace;
	int enable;
};

/*
 * plane's each layers
 */
/*	RGB layer pixel format. */
#define	MLC_RGBFMT_R5G6B5		0x44320000 /* {R5,G6,B5 }. */
#define	MLC_RGBFMT_B5G6R5		0xC4320000 /* {B5,G6,R5 }. */
#define	MLC_RGBFMT_X1R5G5B5		0x43420000 /* {X1,R5,G5,B5}. */
#define	MLC_RGBFMT_X1B5G5R5		0xC3420000 /* {X1,B5,G5,R5}. */
#define	MLC_RGBFMT_X4R4G4B4		0x42110000 /* {X4,R4,G4,B4}. */
#define	MLC_RGBFMT_X4B4G4R4		0xC2110000 /* {X4,B4,G4,R4}. */
#define	MLC_RGBFMT_X8R3G3B2		0x41200000 /* {X8,R3,G3,B2}. */
#define	MLC_RGBFMT_X8B3G3R2		0xC1200000 /* {X8,B3,G3,R2}. */
#define	MLC_RGBFMT_A1R5G5B5		0x33420000 /* {A1,R5,G5,B5}. */
#define	MLC_RGBFMT_A1B5G5R5		0xB3420000 /* {A1,B5,G5,R5}. */
#define	MLC_RGBFMT_A4R4G4B4		0x22110000 /* {A4,R4,G4,B4}. */
#define	MLC_RGBFMT_A4B4G4R4		0xA2110000 /* {A4,B4,G4,R4}. */
#define	MLC_RGBFMT_A8R3G3B2		0x11200000 /* {A8,R3,G3,B2}. */
#define	MLC_RGBFMT_A8B3G3R2		0x91200000 /* {A8,B3,G3,R2}. */
#define	MLC_RGBFMT_R8G8B8		0x46530000 /* {R8,G8,B8 }. */
#define	MLC_RGBFMT_B8G8R8		0xC6530000 /* {B8,G8,R8 }. */
#define	MLC_RGBFMT_X8R8G8B8		0x46530000 /* {X8,R8,G8,B8}. */
#define	MLC_RGBFMT_X8B8G8R8		0xC6530000 /* {X8,B8,G8,R8}. */
#define	MLC_RGBFMT_A8R8G8B8		0x06530000 /* {A8,R8,G8,B8}. */
#define	MLC_RGBFMT_A8B8G8R8		0x86530000 /* {A8,B8,G8,R8}.  */

enum dp_plane_type {
	dp_plane_rgb,
	dp_plane_video,
};
#define	PLANE_VIDEO_NUM			(3) /* Planes = 0,1 (RGB), 3 (VIDEO) */

/* for prototype layer index */
enum dp_color_type {
	dp_color_colorkey,
	dp_color_alpha,
	dp_color_bright,
	dp_color_hue,
	dp_color_contrast,
	dp_color_saturation,
	dp_color_gamma,
	dp_color_transp,
	dp_color_invert,
};

struct dp_plane_layer {
	struct device *dev;
	struct dp_plane_top *plane_top;
	struct list_head list;
	char name[16];
	int module, num;
	enum dp_plane_type type;
	unsigned int format;

	int left;
	int top;
	int width;
	int height;
	int pixelbyte;
	int stride;
	unsigned int h_filter;
	unsigned int v_filter;
	int enable;

	/* color */
	union {
		struct {
			unsigned int transcolor;
			unsigned int invertcolor;
			unsigned int alphablend;
		};
		struct {
			int alpha;	/* def= 15, 0 <= Range <= 16 */
			int bright;	/* def= 0, -128 <= Range <= 128*/
			int contrast; /* def= 0, 0 <= Range <= 8 */
			double hue;	/* def= 0, 0 <= Range <= 360 */
			double saturation; /* def = 0, -100 <= Range <= 100 */
			int satura;
			int gamma;
		};
	} color;
};

void nx_soc_dp_plane_top_set_base(void *base[], int size);
void nx_soc_dp_plane_top_setup(struct dp_plane_top *top);
void nx_soc_dp_plane_top_set_format(struct dp_plane_top *top,
			int width, int height);
void nx_soc_dp_plane_top_set_bg_color(struct dp_plane_top *top);
int nx_soc_dp_plane_top_set_enable(struct dp_plane_top *top, bool on);

int nx_soc_dp_rgb_set_format(struct dp_plane_layer *layer,
			unsigned int format, int pixelbyte, bool adjust);
int nx_soc_dp_rgb_set_position(struct dp_plane_layer *layer,
			int src_x, int src_y, int src_w, int src_h,
			int dst_x, int dst_y, int dst_w, int dst_h,
			bool adjust);
void nx_soc_dp_rgb_set_address(struct dp_plane_layer *layer,
			unsigned int paddr, unsigned int pixelbyte,
			unsigned int stride, bool adjust);
void nx_soc_dp_rgb_set_enable(struct dp_plane_layer *layer,
			bool on, bool adjust);
void nx_soc_dp_rgb_set_color(struct dp_plane_layer *layer, unsigned int type,
			unsigned int color, bool on, bool adjust);

int nx_soc_dp_video_set_format(struct dp_plane_layer *layer,
			unsigned int format, bool adjust);
int nx_soc_dp_video_set_position(struct dp_plane_layer *layer,
			int src_x, int src_y, int src_w, int src_h,
			int dst_x, int dst_y, int dst_w, int dst_h,
			bool adjust);
void nx_soc_dp_video_set_address_1plane(struct dp_plane_layer *layer,
			unsigned int addr, unsigned int stride,
			bool adjust);
void nx_soc_dp_video_set_address_3plane(struct dp_plane_layer *layer,
			unsigned int lu_a, unsigned int lu_s,
			unsigned int cb_a, unsigned int cb_s,
			unsigned int cr_a, unsigned int cr_s,
			bool adjust);
void nx_soc_dp_video_set_enable(struct dp_plane_layer *layer,
			bool on, bool adjust);

void nx_soc_dp_device_set_base(void *base[], int size);
void nx_soc_dp_device_setup(struct dp_device_info *ddi);
int  nx_soc_dp_device_prepare(struct dp_device_info *ddi);
int  nx_soc_dp_device_power_status(struct dp_device_info *ddi);
void nx_soc_dp_device_power_on(struct dp_device_info *ddi, bool on);
void nx_soc_dp_device_irq_on(struct dp_device_info *ddi, bool on);
void nx_soc_dp_device_irq_clear(struct dp_device_info *ddi);

void nx_soc_dp_device_top_base(void *base[], int size);
void nx_soc_dp_device_top_mux(struct dp_device_info *ddi);

#endif /* __S5PXX18_SOC_DP_H__ */
