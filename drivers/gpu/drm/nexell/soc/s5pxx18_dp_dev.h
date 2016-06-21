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
#ifndef _S5PXX18_DP_DEV_H_
#define _S5PXX18_DP_DEV_H_

#include <drm/drm_mipi_dsi.h>

#include "s5pxx18_soc_mlc.h"
#include "s5pxx18_soc_dpc.h"
#include "s5pxx18_soc_disptop.h"
#include "s5pxx18_soc_disp.h"
#include "s5pxx18_soc_mipi.h"
#include "s5pxx18_soc_lvds.h"
#include "s5pxx18_soc_disptop_clk.h"

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
	dp_panel_type_none,
	dp_panel_type_rgb,
	dp_panel_type_lvds,
	dp_panel_type_mipi,
	dp_panel_type_hdmi,
	dp_panel_type_vidi,
};

struct dp_control_ops;

struct dp_control_dev {
	int module;
	void *base;
	struct dp_sync_info sync;
	struct dp_ctrl_info ctrl;
	enum dp_panel_type panel_type;
	void *dp_output;
	struct dp_control_ops *ops;
};

struct dp_control_ops {
	void (*set_base)(struct dp_control_dev *dpc, void *base);
	int  (*prepare)(struct dp_control_dev *dpc, unsigned int flags);
	int  (*unprepare)(struct dp_control_dev *dpc);
	int  (*enable)(struct dp_control_dev *dpc, unsigned int flags);
	int  (*disable)(struct dp_control_dev *dpc);
};

enum {
	dp_clock_rconv = 0,
	dp_clock_lcd = 1,
	dp_clock_mipi = 2,
	dp_clock_lvds = 3,
	dp_clock_hdmi = 4,
	dp_clock_end,
};

struct dp_rgb_dev {
	bool mpu_lcd;
};

struct dp_mipi_dev {
	int lp_bitrate;	/* to lcd setup, low power bitrate (150, 100, 80 Mhz) */
	int hs_bitrate; /* to lcd data, high speed bitrate (1000, ... Mhz) */
	unsigned int hs_pllpms;
	unsigned int hs_bandctl;
	unsigned int lp_pllpms;
	unsigned int lp_bandctl;
};

enum dp_lvds_format {
	dp_lvds_format_vesa = 0,
	dp_lvds_format_jeida = 1,
	dp_lvds_format_loc = 2,
};

struct dp_lvds_dev {
	unsigned int lvds_format;	/* 0:VESA, 1:JEIDA, 2: Location */
	int pol_inv_hs;		/* hsync polarity invert for VESA, JEIDA */
	int pol_inv_vs;		/* bsync polarity invert for VESA, JEIDA */
	int pol_inv_de;		/* de polarity invert for VESA, JEIDA */
	int pol_inv_ck;		/* input clock(pixel clock) polarity invert */
	void *reset_control;
	int num_resets;
};

struct dp_hdmi_dev {
	int preset_num;
	void *preset_data;
};

struct dp_mipi_xfer {
	u8  id;
	u8  data[2];
	u16 flags;
	const u8 *tx_buf;
	u16 tx_len;
	u8 *rx_buf;
	u16 rx_len;
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

	/* source */
	int left;
	int top;
	int width;
	int height;
	int pixelbyte;
	int stride;
	unsigned int h_filter;
	unsigned int v_filter;
	int enable;

	/* target */
	int dst_left;
	int dst_top;
	int dst_width;
	int dst_height;

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

const char *dp_panel_type_name(enum dp_panel_type panel);

void nx_soc_dp_device_dpc_base(int module, void __iomem *base);
void nx_soc_dp_device_mlc_base(int module, void __iomem *base);
void nx_soc_dp_device_top_base(int module, void __iomem *base);
void nx_soc_dp_device_clk_base(int id, void __iomem *base);
void nx_soc_dp_device_lvds_base(void __iomem *base);

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

void nx_soc_dp_device_setup(struct dp_control_dev *dpc);
int  nx_soc_dp_device_prepare(struct dp_control_dev *dpc);
int  nx_soc_dp_device_power_status(struct dp_control_dev *dpc);
void nx_soc_dp_device_power_on(struct dp_control_dev *dpc, bool on);
void nx_soc_dp_device_irq_on(struct dp_control_dev *dpc, bool on);
void nx_soc_dp_device_irq_done(struct dp_control_dev *dpc);

#ifdef CONFIG_DRM_NX_RGB
int nx_soc_dp_rgb_register(struct device *dev,
			struct device_node *np, struct dp_control_dev *dpc);
#endif

#ifdef CONFIG_DRM_NX_LVDS
int nx_soc_dp_lvds_register(struct device *dev,
			struct device_node *np, struct dp_control_dev *dpc,
			void *resets, int num_resets);
#endif

#ifdef CONFIG_DRM_NX_MIPI_DSI
int nx_soc_dp_mipi_register(struct device *dev,
			struct device_node *np, struct dp_control_dev *dpc);
int nx_soc_dp_mipi_tx_transfer(struct dp_mipi_xfer *xfer);
int nx_soc_dp_mipi_rx_transfer(struct dp_mipi_xfer *xfer);
#endif

#endif /* __S5PXX18_DP_DEV_H__ */
