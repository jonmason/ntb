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
#ifndef _S5PXX18_DRM_DP_H_
#define _S5PXX18_DRM_DP_H_

#include <drm/drmP.h>
#include <drm/drm_panel.h>
#include <drm/drm_mipi_dsi.h>

#include <linux/reset.h>
#include <soc/nexell/tieoff.h>

#include "../nx_drm_drv.h"
#include "s5pxx18_soc_mlc.h"
#include "s5pxx18_soc_dpc.h"
#include "s5pxx18_soc_disptop.h"
#include "s5pxx18_soc_disp.h"
#include "s5pxx18_soc_mipi.h"
#include "s5pxx18_soc_lvds.h"
#include "s5pxx18_soc_disptop_clk.h"

#define	MAX_RESOURCE_NUM	8

struct nx_control_res {
	void __iomem *base;
	struct reset_control *resets[MAX_RESOURCE_NUM];
	int num_resets;
	/* sub devices */
	void __iomem *sub_bases[MAX_RESOURCE_NUM];
	int num_sud_devs;
	void __iomem *sub_clk_bases[MAX_RESOURCE_NUM];
	int sub_clk_ids[MAX_RESOURCE_NUM];
	int num_sub_clks;
	struct reset_control *sub_resets[MAX_RESOURCE_NUM];
	int num_sub_resets;
	u32 tieoffs[MAX_RESOURCE_NUM][2];
	int num_tieoffs;
};

struct nx_plane_rect {
	int left, right;
	int top, bottom;
};

#define	PLANE_VIDEO_NUM		(3) /* Planes = 0,1 (RGB), 3 (VIDEO) */
#define NX_PLANE_FORMAT_SCREEN_SIZE	(1<<0)
#define NX_PLANE_FORMAT_VIDEO_PRIORITY	(1<<1)
#define NX_PLANE_FORMAT_BACK_COLOR	(1<<2)

struct plane_top_format {
	int module;
	int video_priority;
	int width;
	int height;
	unsigned int bgcolor;
	unsigned int mask;
};

struct plane_property {
	union color {
		struct {
			struct drm_property *transcolor;
			struct drm_property *alphablend;
		} rgb;
		struct {
			struct drm_property *colorkey;
		} yuv;
	} color;
	struct drm_property *priority;
};

struct nx_plane_layer {
	struct nx_top_plane *top_plane;
	struct list_head list;
	char name[16];
	int module, num;
	unsigned int type;
	unsigned int format;
	bool is_bgr;
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
			u32 transcolor;
			u32 invertcolor;
			u32 alphablend;
			u32 colorkey;
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

	/* property */
	struct plane_property property;
};

struct nx_top_plane {
	struct drm_crtc *crtc;
	struct nx_control_res res;
	unsigned int planes_type[MAX_PLNAES];
	int module;
	int width;
	int height;
	int video_priority;	/* 0: video>RGBn, 1: RGB0>video>RGB1, */
				/*   2: RGB0 > RGB1 > vidoe .. */
	struct list_head plane_list;
	unsigned int back_color;
	unsigned int color_key;
	int interlace;
	int enable;
	void *regs[sizeof(struct nx_mlc_register_set)/sizeof(void *)];
	struct list_head list; /* next top_plane */
	bool cluster;
	enum nx_cluster_dir direction;
};

struct nx_sync_info {
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
#define	DPC_SYNC_DELAY_RGB_PVD		(1<<0)
#define	DPC_SYNC_DELAY_HSYNC_CP1	(1<<1)
#define	DPC_SYNC_DELAY_VSYNC_FRAM	(1<<2)
#define	DPC_SYNC_DELAY_DE_CP		(1<<3)

struct nx_control_info {
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

struct nx_control_dev {
	int module;
	struct nx_control_res res;
	struct nx_control_info ctrl;
	struct nx_sync_info sync;
	enum nx_panel_type panel_type;
	void __iomem *regs[sizeof(struct nx_dpc_register_set)/sizeof(void *)];
	bool cluster;
	bool bootup;
	struct list_head list; /* next contol dev */
};

enum {
	NX_CLOCK_RCONV = 0,
	NX_CLOCK_LCD = 1,
	NX_CLOCK_MIPI = 2,
	NX_CLOCK_LVDS = 3,
	NX_CLOCK_HDMI = 4,
	NX_CLOCK_END,
};

struct nx_rgb_dev {
	struct nx_control_dev control;
	/* properties */
	bool mpu_lcd;
};

struct nx_mipi_xfer {
	u8  id;
	u8  data[2];
	u16 flags;
	const u8 *tx_buf;
	u16 tx_len;
	u8 *rx_buf;
	u16 rx_len;
};

struct nx_mipi_dev {
	struct nx_control_dev control;
	struct device *dev;
	/* properties */
	int lp_bitrate;	/* to lcd setup, low power bitrate (150, 100, 80 Mhz) */
	int hs_bitrate; /* to lcd data, high speed bitrate (1000, ... Mhz) */
	int lpm_trans;
	int command_mode;
	unsigned int hs_pllpms;
	unsigned int hs_bandctl;
	unsigned int lp_pllpms;
	unsigned int lp_bandctl;
	struct nx_drm_display *display;
	struct mipi_dsi_device *dsi;
	/* for command mode */
	struct hrtimer timer;
	struct work_struct work;
	struct mutex lock;
	void *framebuffer;
	struct drm_framebuffer *fb;
	struct blocking_notifier_head notifier;
	bool sys_grouped;
	/* for fifo irq */
	int fifo_irq;
	int cond;
	wait_queue_head_t waitq;
	bool irq_installed;
};

enum nx_lvds_format {
	NX_LVDS_FORMAT_VESA = 0,
	NX_LVDS_FORMAT_JEIDA = 1,
	NX_LVDS_FORMAT_LOC = 2,
};

struct nx_lvds_dev {
	struct nx_control_dev control;
	/* properties */
	unsigned int lvds_format;	/* 0:VESA, 1:JEIDA, 2: Location */
	int pol_inv_hs;		/* hsync polarity invert for VESA, JEIDA */
	int pol_inv_vs;		/* bsync polarity invert for VESA, JEIDA */
	int pol_inv_de;		/* de polarity invert for VESA, JEIDA */
	int pol_inv_ck;		/* input clock(pixel clock) polarity invert */
	int voltage_level;
};

struct nx_hdmi_dev {
	struct nx_control_dev control;
	/* properties */
	int preset_num;
	int q_range;
	void *preset_data;
};

struct nx_tvout_dev {
	struct nx_control_dev control;
};

/*
 * @ s5pxx18 specific display util interfaces.
 */
void *nx_drm_display_lvds_get(struct device *dev, struct device_node *node,
			struct nx_drm_display *display);
void *nx_drm_display_rgb_get(struct device *dev, struct device_node *node,
			struct nx_drm_display *display);
void *nx_drm_display_mipi_get(struct device *dev, struct device_node *node,
			struct nx_drm_display *display);
void *nx_drm_display_hdmi_get(struct device *dev,
			struct device_node *np, struct nx_drm_display *display);

void nx_display_mode_to_sync(struct drm_display_mode *mode,
			struct nx_drm_display *display);
void nx_display_resume_resource(struct nx_drm_display *display);

/*
 * @ s5pxx18 specific display soc interfaces.
 */
void nx_soc_dp_cont_dpc_base(int module, void __iomem *base);
void nx_soc_dp_cont_mlc_base(int module, void __iomem *base);
void nx_soc_dp_cont_top_base(int module, void __iomem *base);
void nx_soc_dp_cont_top_clk_base(int id, void __iomem *base);
void nx_soc_dp_cont_top_clk_on(int id);

void nx_soc_dp_cont_dpc_clk_on(struct nx_control_dev *control);
int  nx_soc_dp_cont_prepare(struct nx_control_dev *control);
int  nx_soc_dp_cont_power_status(struct nx_control_dev *control);
void nx_soc_dp_cont_power_on(struct nx_control_dev *control, bool on);
void nx_soc_dp_cont_irq_on(int module, bool on);
void nx_soc_dp_cont_irq_done(int module);

void nx_soc_dp_plane_top_prepare(struct nx_top_plane *top);
void nx_soc_dp_plane_top_set_format(struct nx_top_plane *top,
			int width, int height);
void nx_soc_dp_plane_top_set_bg_color(struct nx_top_plane *top);
int nx_soc_dp_plane_top_set_enable(struct nx_top_plane *top, bool on);
void nx_soc_dp_plane_top_prev_format(struct plane_top_format *format);

int nx_soc_dp_plane_rgb_set_format(struct nx_plane_layer *layer,
			unsigned int format, int pixelbyte, bool adjust);
int nx_soc_dp_plane_rgb_set_position(struct nx_plane_layer *layer,
			int src_x, int src_y, int src_w, int src_h,
			int dst_x, int dst_y, int dst_w, int dst_h,
			bool adjust);
void nx_soc_dp_plane_rgb_set_address(struct nx_plane_layer *layer,
			unsigned int paddr, unsigned int pixelbyte,
			unsigned int stride, int align, bool adjust);
void nx_soc_dp_plane_rgb_set_enable(struct nx_plane_layer *layer,
			bool on, bool adjust);
void nx_soc_dp_plane_rgb_set_color(struct nx_plane_layer *layer,
			unsigned int type, unsigned int color,
			bool on, bool adjust);

int nx_soc_dp_plane_video_set_format(struct nx_plane_layer *layer,
			unsigned int format, bool adjust);
int nx_soc_dp_plane_video_set_position(struct nx_plane_layer *layer,
			int src_x, int src_y, int src_w, int src_h,
			int dst_x, int dst_y, int dst_w, int dst_h,
			bool adjust);
void nx_soc_dp_plane_video_set_address_1p(struct nx_plane_layer *layer,
			unsigned int addr, unsigned int stride,
			bool adjust);
void nx_soc_dp_plane_video_set_address_3p(struct nx_plane_layer *layer,
			unsigned int lu_a, unsigned int lu_s,
			unsigned int cb_a, unsigned int cb_s,
			unsigned int cr_a, unsigned int cr_s,
			bool adjust);
void nx_soc_dp_plane_video_set_enable(struct nx_plane_layer *layer,
			bool on, bool adjust);
void nx_soc_dp_plane_video_set_priority(struct nx_plane_layer *layer,
			int priority);

#endif
