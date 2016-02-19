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
#ifndef __NX_DISPLAY_H__
#define __NX_DISPLAY_H__

#include "nx_display_dev.h"
#include "nx_fourcc.h"

/* rgb layer control on multi layer */
extern int nx_soc_disp_rgb_set_fblayer(int module, int layer);
extern int nx_soc_disp_rgb_get_fblayer(int module);
extern int nx_soc_disp_rgb_set_format(int module, int layer,
					unsigned int format, int image_w,
				    int image_h, int pixelbyte);
extern void nx_soc_disp_rgb_get_format(int module, int layer,
					unsigned int *format, int *image_w,
					int *image_h, int *pixelbyte);
extern int nx_soc_disp_rgb_set_position(int module, int layer,
					int x, int y, int waitvsync);
extern int nx_soc_disp_rgb_get_position(int module, int layer,
					int *x, int *y);
extern int nx_soc_disp_rgb_set_clipping(int module, int layer, int left,
					int top, int width, int height);
extern void nx_soc_disp_rgb_get_clipping(int module, int layer, int *left,
					int *top, int *width, int *height);
extern void nx_soc_disp_rgb_set_address(int module, int layer,
					unsigned int phyaddr,
					unsigned int pixelbyte,
					unsigned int stride, int waitvsync);
extern void nx_soc_disp_rgb_get_address(int module, int layer,
					unsigned int *phyaddr,
					unsigned int *pixelbyte,
					unsigned int *stride);
extern void nx_soc_disp_rgb_set_color(int module, int layer,
					unsigned int type, unsigned int color,
					int	enable);
extern unsigned int nx_soc_disp_rgb_get_color(int module, int layer,
					unsigned int type);
extern void nx_soc_disp_rgb_set_enable(int module, int layer, int enable);
extern int nx_soc_disp_rgb_stat_enable(int module, int layer);

/* video layer control on multi layer */
extern int nx_soc_disp_video_set_format(int module, unsigned int fourcc,
					int image_w, int image_h);
extern void nx_soc_disp_video_get_format(int module, unsigned int *fourcc,
					int *image_w, int *image_h);
extern int nx_soc_disp_video_set_position(int module, int left, int top,
					int right, int bottom,
					int waitvsync);
extern int nx_soc_disp_video_get_position(int module, int *left, int *top,
					int *right, int *bottom);

extern void nx_soc_disp_video_set_address(int module, unsigned int lu_a,
					unsigned int lu_s, unsigned int cb_a,
					unsigned int cb_s, unsigned int cr_a,
					unsigned int cr_s, int waitvsync);
extern void nx_soc_disp_video_get_address(int module, unsigned int *lu_a,
					unsigned int *lu_s,
					unsigned int *cb_a,
					unsigned int *cb_s,
					unsigned int *cr_a,
					unsigned int *cr_s);
extern void nx_soc_disp_video_set_priority(int module, int prior);
extern int nx_soc_disp_video_get_priority(int module);
extern void nx_soc_disp_video_set_colorkey(int module, unsigned int color,
					int enable);
extern unsigned int nx_soc_disp_video_get_colorkey(int module);
extern void nx_soc_disp_video_set_color(int module, unsigned int type,
					unsigned int color, int enable);
extern unsigned int nx_soc_disp_video_get_color(int module,
					unsigned int type);
extern void nx_soc_disp_video_set_hfilter(int module, int enable);
extern void nx_soc_disp_video_set_vfilter(int module, int enable);
extern unsigned int nx_soc_disp_video_stat_hfilter(int module);
extern unsigned int nx_soc_disp_video_stat_vfilter(int module);

extern void nx_soc_disp_video_set_enable(int module, int enable);
extern int nx_soc_disp_video_stat_enable(int module);

extern void nx_soc_disp_video_set_crop(int module,
					bool enable, int left, int top,
					int width, int height, int waitvsync);

/* top layer control on multi layer */
extern void nx_soc_disp_get_resolution(int module, int *w, int *h);
extern void nx_soc_disp_set_bg_color(int module, unsigned int color);
extern unsigned int nx_soc_disp_get_bg_color(int module);
extern void nx_soc_disp_layer_set_enable(int module, int layer,
					int enable);
extern int nx_soc_disp_layer_stat_enable(int module, int layer);
extern int nx_soc_disp_wait_vertical_sync(int module);
extern struct disp_irq_callback *nx_soc_disp_register_irq_callback(
					int module,
					void (*callback)(void *), void *data);
extern void nx_soc_disp_unregister_irq_callback(int module,
					struct disp_irq_callback *);

/*
 * display device control
 * device 0=resconv, 1=lcd, 2=hdmi, 3=mipi, 4=lvds, 5=syncgen0, 6=syncgen1
 */
extern void nx_soc_disp_register_proc_ops(enum disp_dev_type device,
					struct disp_process_ops *ops);
extern void nx_soc_disp_register_priv(enum disp_dev_type device,
					void *priv);
extern int nx_soc_disp_device_connect_to(enum disp_dev_type device,
					enum disp_dev_type to,
					struct disp_vsync_info *vsync);
extern void nx_soc_disp_device_disconnect(enum disp_dev_type device,
					enum disp_dev_type to);
extern int nx_soc_disp_device_set_sync_param(enum disp_dev_type device,
					struct disp_sync_par *spar);
extern int nx_soc_disp_device_get_sync_param(enum disp_dev_type device,
					struct disp_sync_par *spar);
extern int nx_soc_disp_device_set_vsync_info(enum disp_dev_type device,
					struct disp_vsync_info *vsync);
extern int nx_soc_disp_device_get_vsync_info(enum disp_dev_type device,
					struct disp_vsync_info *vsync);
extern int nx_soc_disp_device_set_dev_param(enum disp_dev_type device,
					void *param);
extern int nx_soc_disp_device_enable(enum disp_dev_type device,
					int enable);
extern int nx_soc_disp_device_stat_enable(enum disp_dev_type device);
extern int nx_soc_disp_device_suspend(enum disp_dev_type device);
extern void nx_soc_disp_device_resume(enum disp_dev_type device);
extern void nx_soc_disp_device_framebuffer(int module, int fb);
extern int nx_soc_disp_device_enable_all(int module, int enable);
extern int nx_soc_disp_device_suspend_all(int module);
extern void nx_soc_disp_device_resume_all(int module);
extern void nx_soc_disp_device_reset_top(void);
extern int nx_soc_disp_device_enable_all_saved(int module, int enable);
extern void nx_soc_disp_register_lcd_ops(int module,
					struct lcd_operation *pops);

#endif
