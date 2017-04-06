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
#ifndef _NXP5XXX_DRV_H_
#define _NXP5XXX_DRV_H_

#include <drm/drmP.h>
#include <drm/drm_panel.h>
#include <drm/drm_mipi_dsi.h>

#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_res_manager.h>

#include "../nx_drm_drv.h"

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
};

#define	NXS_MASK_DMAR		(1<<0)
#define	NXS_MASK_BLENDER	(1<<1)
#define	NXS_MASK_FIFO		(1<<2)
#define	PLANE_MAX_LINE		4096

struct nx_crtc_frame {
	struct nx_plane_layer *top;
	unsigned int s_mask;
	struct nxs_dev *bottom;
	struct list_head plane_list;	/* add nx_plane_layer->plane */
	int pipe;
	int width;
	int height;
	unsigned int back_color;
	unsigned int color_key;
	int enable;
};

struct nx_plane_rect {
	int left, right;
	int top, bottom;
};

struct nx_plane_layer {
	struct nx_crtc_frame *frame;
	struct nxs_function *s_func;	/* nexell function base */
	unsigned int s_mask;
	struct list_head plane; /* adde to bottom layer_list */
	char name[16];
	int index;
	int pipe;
	enum drm_plane_type drm_type;
	unsigned int format;
	bool is_bgr;
	/* source */
	int left;
	int top;
	int width;
	int height;
	int pixelbyte;
	int stride;
	int enable;
	/* target */
	int d_left;
	int d_top;
	int d_width;
	int d_height;
	/* color */
	union {
		struct {
			unsigned int transcolor;
			unsigned int invertcolor;
			unsigned int alphablend;
		};
		struct {
			int alpha;
			int bright;
			int contrast;
			double hue;
			double saturation;
			int satura;
			int gamma;
		};
	} color;

	/* property */
	struct plane_property property;
};

struct nx_control_dev {
	struct nxs_dev *s_dev;
	int s_display;
	int pipe;
	bool connected;
	struct videomode vm;
	enum nx_panel_type panel_type;
};

struct nx_lvds_dev {
	struct nx_control_dev control;
	/* properties */
	unsigned int lvds_format;	/* 0:VESA, 1:JEIDA, 2: Location */
	int voltage_level;
};

struct nx_rgb_dev {
	struct nx_control_dev control;
	/* properties */
	bool mpu_lcd;
};

struct nx_mipi_dev {
	struct nx_control_dev control;
	/* properties */
	int lp_bitrate;	/* to lcd setup, low power bitrate (150, 100, 80 Mhz) */
	int hs_bitrate; /* to lcd data, high speed bitrate (1000, ... Mhz) */
	unsigned int hs_pllpms;
	unsigned int hs_bandctl;
	unsigned int lp_pllpms;
	unsigned int lp_bandctl;
};

struct nx_hdmi_dev {
	struct nx_control_dev control;
	/* properties */
	int preset_num;
	int q_range;
	void *preset_data;
};

struct nx_ntsc_dev {
	struct nx_control_dev control;
};

#define	get_sdev(c)	(c->control.s_dev.device)

/*
 * @ nxp5xxx drm device tree parse
 */
#define property_read(n, s, v) of_property_read_u32(n, s, &v)


/*
 * @ nxp5xxx specific display util interfaces.
 */
int nx_stream_display_create(struct nx_drm_display *display);
void nx_stream_display_free(struct nx_drm_display *display);

void *nx_drm_display_hdmi_get(struct device *dev,
			struct device_node *np, struct nx_drm_display *display);

#endif
