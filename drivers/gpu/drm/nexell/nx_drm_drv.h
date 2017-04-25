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
#ifndef _NX_DRM_DRV_H_
#define _NX_DRM_DRV_H_

#include <drm/drmP.h>
#include <drm/drm_mipi_dsi.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <video/display_timing.h>
#include <video/videomode.h>

#if defined CONFIG_DRM_NX_S5PXX18
#define MAX_CRTCS		2 /* Multi Layer Controller(MLC) nums (0, 1) */
#define MAX_CONNECTOR	2 /* RGB, LVDS, MiPi, HDMI */
#define MAX_PLNAES		3
#elif defined CONFIG_DRM_NX_NXP5XXX
#define MAX_CRTCS		6 /* Multi Layer Controller(MLC) nums (0, 1) */
#define MAX_CONNECTOR	6	  /* RGB, NTSC, LVDS0,1, MiPi, HDMI */
#define MAX_PLNAES		10
#endif

#define MAX_FB_MODE_WIDTH	4096
#define MAX_FB_MODE_HEIGHT	4096

#define INVALID_IRQ		((unsigned)-1)
#define	CLUSTER_LCD_MAX		2

/*
 * @ nexell drm driver private structure.
 */
struct nx_drm_private {
	struct nx_drm_fbdev *fbdev;
	bool force_detect;
	struct drm_crtc *crtcs[MAX_CRTCS];
	int num_crtcs;
	struct commit {
		struct drm_device *drm;
		struct drm_atomic_state *state;
		struct work_struct work;
		wait_queue_head_t wait;
		spinlock_t lock;
		struct mutex m_lock;
		u32 pending;
		u32 crtcs;
	} commit;
};

/*
 * @ nexell drm crtc structure.
 */
struct nx_drm_crtc_ops {
	void (*reset)(struct drm_crtc *crtc);
	int (*atomic_check)(struct drm_crtc *crtc,
			struct drm_crtc_state *state);
	int (*begin)(struct drm_crtc *crtc);
	void (*enable)(struct drm_crtc *crtc);
	void (*disable)(struct drm_crtc *crtc);
	void (*flush)(struct drm_crtc *crtc);
	void (*destroy)(struct drm_crtc *crtc);
};

enum nx_cluster_dir {
	DP_CLUSTER_HOR,
	DP_CLUSTER_VER,
	DP_CLUSTER_CLONE,
};

struct nx_drm_cluster {
	struct videomode vm[CLUSTER_LCD_MAX];
	enum nx_cluster_dir direction;
};

struct nx_drm_crtc {
	struct drm_crtc crtc;
	void *context;
	struct nx_drm_crtc_ops *ops;
	int num_planes;
	unsigned int planes_type[MAX_PLNAES];
	int pipe;		/* crtc index */
	int irq;
	struct drm_pending_vblank_event *event;
	bool suspended;
	struct nx_drm_cluster *cluster;
};

#define to_nx_crtc(x)	\
		container_of(x, struct nx_drm_crtc, crtc)

/*
 * @ nexell drm plane structure.
 */
#define	NX_PLANE_TYPE_OVERLAY	DRM_PLANE_TYPE_OVERLAY	/* 0 */
#define	NX_PLANE_TYPE_PRIMARY	DRM_PLANE_TYPE_PRIMARY	/* 1 */
#define	NX_PLANE_TYPE_CURSOR	DRM_PLANE_TYPE_CURSOR	/* 2 */
#define	NX_PLANE_TYPE_RGB	(0<<4)
#define	NX_PLANE_TYPE_VIDEO	(1<<4)
#define	NX_PLANE_TYPE_UNKNOWN	(0xFFFFFFF)

enum nx_color_type {
	NX_COLOR_COLORKEY,
	NX_COLOR_ALPHA,
	NX_COLOR_BRIGHT,
	NX_COLOR_HUE,
	NX_COLOR_CONTRAST,
	NX_COLOR_SATURATION,
	NX_COLOR_GAMMA,
	NX_COLOR_TRANS,
	NX_COLOR_INVERT,
};

struct nx_drm_plane_ops {
	int (*atomic_check)(struct drm_plane *plane,
			struct drm_plane_state *state);
	int (*update)(struct drm_plane *plane,
			struct drm_framebuffer *fb,
			int crtc_x, int crtc_y, int crtc_w, int crtc_h,
			int src_x, int src_y, int src_w, int src_h);
	void (*disable)(struct drm_plane *plane);
	void (*destroy)(struct drm_plane *plane);
	void (*create_proeprties)(struct drm_device *drm,
				struct drm_crtc *crtc, struct drm_plane *plane,
				struct drm_plane_funcs *plane_funcs);
};

struct nx_drm_plane {
	struct drm_plane plane;
	void *context;
	struct nx_drm_plane_ops *ops;
	int index;
	const uint32_t *formats;
	int format_count;
	int align;
};

#define to_nx_plane(x)	\
		container_of(x, struct nx_drm_plane, plane)

/*
 * @ nexell drm encoder structure.
 */
struct nx_drm_encoder_ops {
	int (*dpms_status)(struct drm_encoder *encoder);
	void (*enable)(struct drm_encoder *encoder);
	void (*disable)(struct drm_encoder *encoder);
	void (*destroy)(struct drm_encoder *encoder);
};

struct nx_drm_encoder {
	struct drm_encoder encoder;
	struct nx_drm_encoder_ops *ops;
	struct nx_drm_connector *connector;
	bool enabled;	/* for qos */
};

#define to_nx_encoder(e)	\
		container_of(e, struct nx_drm_encoder, encoder)

/*
 * @ nexell drm connector structure.
 */
enum nx_panel_type {
	NX_PANEL_TYPE_NONE,
	NX_PANEL_TYPE_RGB,
	NX_PANEL_TYPE_LVDS,
	NX_PANEL_TYPE_MIPI,
	NX_PANEL_TYPE_HDMI,
	NX_PANEL_TYPE_TV,
	NX_PANEL_TYPE_CLUSTER_LCD,
};

struct nx_drm_connect_drv_ops {
	bool (*detect)(struct device *dev,
			struct drm_connector *connector);
	bool (*is_connected)(struct device *dev);
	int (*get_modes)(struct device *dev, struct drm_connector *connector);
	int (*valid_mode)(struct device *dev, struct drm_display_mode *mode);
	void (*set_mode)(struct device *dev, struct drm_display_mode *mode);
	void (*set_sync)(struct device *dev, struct drm_display_mode *mode);
	void (*enable)(struct device *dev);
	void (*disable)(struct device *dev);
};

struct nx_drm_connector {
	struct drm_connector connector;
	struct nx_drm_encoder *encoder;	/* hard linked encoder */
	struct device *dev;	/* display device */
	struct nx_drm_connect_drv_ops *ops;
	struct nx_drm_display *display;	/* for display control */
	unsigned int possible_crtcs;
	bool suspended;
};

#define to_nx_connector(c)		\
		container_of(c, struct nx_drm_connector, connector)

/*
 * @ nexell drm display structure.
 */
struct nx_drm_hdmi_ops {
	u32 (*hpd_status)(struct nx_drm_display *display);
	int (*is_connected)(struct nx_drm_display *display);
	int (*is_valid)(struct nx_drm_display *display,
			struct drm_display_mode *mode);
	int (*get_mode)(struct nx_drm_display *display,
			struct drm_display_mode *mode);
	irqreturn_t (*hpd_irq_cb)(int irq, void *data);
	void *cb_data;
};

struct nx_drm_mipi_dsi_ops {
	int (*set_format)(struct nx_drm_display *display,
			struct mipi_dsi_device *device);
	int (*transfer)(struct nx_drm_display *display,
			struct mipi_dsi_host *host,
			const struct mipi_dsi_msg *msg);
};

struct nx_drm_display_ops {
	/* device control */
	int (*open)(struct nx_drm_display *display, int pipe);
	void (*close)(struct nx_drm_display *display, int pipe);
	int (*prepare)(struct nx_drm_display *display);
	int (*unprepare)(struct nx_drm_display *display);
	int (*enable)(struct nx_drm_display *display);
	int (*disable)(struct nx_drm_display *display);
	/* mode control */
	int (*set_mode)(struct nx_drm_display *display,
			struct drm_display_mode *mode, unsigned int flags);
	/* interrupt */
	void (*irq_enable)(struct nx_drm_display *display, unsigned int type);
	void (*irq_disable)(struct nx_drm_display *display, unsigned int type);
	void (*irq_done)(struct nx_drm_display *display, unsigned int type);
	/* suspend/resume */
	int (*suspend)(struct nx_drm_display *display);
	int (*resume)(struct nx_drm_display *display);
	/* specific operations */
	struct nx_drm_mipi_dsi_ops *dsi;
	struct nx_drm_hdmi_ops *hdmi;
};

struct nx_drm_display {
	struct nx_drm_connector *connector;
	struct device_node *panel_node;
	struct drm_panel *panel;
	/* for output device RGB/LVDS/MIPI/HDMI/TV */
	struct nx_drm_display_ops *ops;
	void *context;
	/* panel size, sync ... */
	enum nx_panel_type panel_type;
	int width_mm;
	int height_mm;
	struct videomode vm;	/* video sync */
	int vrefresh;
	bool check_panel;
	int pipe;
	int irq;
	bool is_connected;
	bool disable_output;
};

/* HPD events */
#define	HDMI_EVENT_PLUG		(1<<0)
#define	HDMI_EVENT_UNPLUG	(1<<1)
#define	HDMI_EVENT_HDCP		(1<<2)

/* @ specific drm driver */
int nx_drm_crtc_create(struct drm_device *drm);
int nx_drm_vblank_init(struct drm_device *drm);

int nx_drm_vblank_enable(struct drm_device *drm, unsigned int pipe);
void nx_drm_vblank_disable(struct drm_device *drm, unsigned int pipe);

int nx_drm_planes_create(struct drm_device *drm,
			struct drm_crtc *crtc, int pipe,
			struct drm_crtc_funcs *crtc_funcs);
bool nx_drm_plane_support_bgr(void);

int nx_drm_connector_attach(struct drm_device *drm,
			struct drm_connector *connector,
			int pipe, unsigned int possible_crtcs,
			enum nx_panel_type panel_type);
void nx_drm_connector_detach(struct drm_connector *connector);

struct drm_encoder *nx_drm_encoder_create(struct drm_device *drm,
			struct drm_connector *connector, int enc_type,
			int pipe, int possible_crtcs);

enum nx_panel_type nx_panel_get_type(struct nx_drm_display *display);
const char *nx_panel_get_name(enum nx_panel_type panel);

/* @ specific drm soc */
int nx_drm_crtc_init(struct drm_device *drm, struct drm_crtc *crtc, int pipe);
int nx_drm_planes_init(struct drm_device *drm, struct drm_crtc *crtc,
			struct drm_plane **planes, int num_planes,
			enum drm_plane_type *drm_types, bool bgr);
int nx_drm_encoder_init(struct drm_encoder *encoder);
struct nx_drm_display *nx_drm_display_get(struct device *dev,
			struct device_node *node,
			struct drm_connector *connector,
			enum nx_panel_type type);
void nx_drm_display_put(struct device *dev, struct nx_drm_display *display);
int  nx_drm_display_setup(struct nx_drm_display *display,
			struct device_node *node, enum nx_panel_type type);

#include <media/v4l2-subdev.h>
void *nx_drm_display_tvout_get(struct device *dev, struct device_node *node,
			struct nx_drm_display *display);

struct media_device *nx_v4l2_get_media_device(void);
int nx_v4l2_register_subdev(struct v4l2_subdev *);

/* @ specific panel driver register */
void panel_lcd_init(void);
void panel_lcd_exit(void);
void panel_hdmi_init(void);
void panel_hdmi_exit(void);
void panel_tv_init(void);
void panel_tv_exit(void);
void panel_cluster_init(void);
void panel_cluster_exit(void);

#endif

