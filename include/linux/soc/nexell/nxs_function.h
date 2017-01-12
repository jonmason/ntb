/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
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

#ifndef _NXS_FUNCTION_H
#define _NXS_FUNCTION_H

#include <dt-bindings/soc/nxs_function.h>

struct v4l2_async_subdev;
struct nxs_dev;
struct nxs_irq_callback;

enum {
	NXS_FUNCTION_USER_NONE	 = 0,
	NXS_FUNCTION_USER_KERNEL = 1,
	NXS_FUNCTION_USER_APP,
};

struct nxs_function_elem {
	struct list_head list;
	u32 function;
	u32 index;
	u32 user;
	bool multitap_follow;
};

/* below code must be synced with include/uapi/linux/nxs_ioctl.h */
#define BLENDING_TO_OTHER       (1 << 0)
#define BLENDING_TO_BOTTOM      (1 << 1)
#define MULTI_PATH              (1 << 2)

struct nxs_function_request {
	int nums_of_function;
	struct list_head head;
	uint32_t flags;
	struct {
		/* for multitap: MULTI_PATH */
		int sibling_handle;
		/* for blending: BLENDING_TO_OTHER */
		uint32_t display_id;
		/* for complete display: BLENDING_TO_BOTTOM */
		uint32_t bottom_id;
	} option;
};

struct nxs_function {
	struct list_head list; /* connected to nxs_func_list of nxs_function.c */
	struct list_head dev_list; /* head of nxs_dev->func_list */
	struct list_head disp_list; /* connected to nxs_display->func_list */

	struct nxs_function_request *req;
	int type;
	void *priv;
	u32 id;
	void *disp;
};

struct nxs_query_function;
struct nxs_function_builder {
	void *priv;
	struct nxs_function *(*build)(struct nxs_function_builder *pthis,
				      const char *name,
				      struct nxs_function_request *);
	int (*free)(struct nxs_function_builder *pthis,
		    struct nxs_function *);
	struct nxs_function *(*get)(struct nxs_function_builder *pthis,
				    int handle);
	int (*query)(struct nxs_function_builder *pthis,
		     struct nxs_query_function *query);
};

struct nxs_action_unit {
	u32 value;
	u32 delay_ms;
};

struct nxs_gpio_action {
	u32 gpio_num;
	u32 count;
	struct nxs_action_unit *units;
};

struct nxs_action {
	int type;
	void *action;
};

struct nxs_action_seq {
	int count;
	struct nxs_action *actions;
};

struct nxs_capture_hw_vip_param {
	u32 bus_width;
	u32 bus_align;
	u32 interlace;
	u32 h_backporch;
	u32 v_backporch;
	u32 pclk_polarity;
	u32 hsync_polarity;
	u32 vsync_polarity;
	u32 field_polarity;
	u32 data_order;
};

struct nxs_capture_hw_csi_param {
	u8 data_lanes[4];
	u8 clock_lane;
	u8 num_data_lanes;
	u8 lane_polarities[5];
	u64 link_frequency;
};

struct nxs_capture_ctx {
	u32 bus_type;
	union {
		struct nxs_capture_hw_vip_param parallel;
		struct nxs_capture_hw_csi_param csi;
	} bus;
	struct nxs_action_seq enable_seq;
	struct nxs_action_seq disable_seq;
	struct clk *clk;
	u32 clock_freq;
	u32 regulator_nr;
	char **regulator_names;
	u32 *regulator_voltages;
	struct v4l2_subdev *sensor;
	struct device *dev;
};

const char *nxs_function_to_str(int function);
int nxs_capture_bind_sensor(struct device *dev, struct nxs_dev *nxs_dev,
			    struct v4l2_async_subdev *asd,
			    struct nxs_capture_ctx *capture);
int nxs_capture_power(struct nxs_capture_ctx *capture, bool enable);
void nxs_capture_free(struct nxs_capture_ctx *capture);
void nxs_free_function_request(struct nxs_function_request *req);
struct nxs_function *nxs_function_build(struct nxs_function_request *req);
struct nxs_function *nxs_function_make(int dev_num, bool blending,
				       u32 disp_num, ...);
void nxs_function_destroy(struct nxs_function *f);
struct nxs_function *nxs_function_get(int handle);
struct nxs_irq_callback *
nxs_function_register_irqcallback(struct nxs_function *f,
				  void *data,
				  void (*handler)(struct nxs_dev *,
						  void*)
				 );
int nxs_function_unregister_irqcallback(struct nxs_function *f,
					struct nxs_irq_callback *callback);
int nxs_function_config(struct nxs_function *f, bool dirty, int count, ...);
int nxs_function_connect(struct nxs_function *f);
int nxs_function_ready(struct nxs_function *f);
int nxs_function_start(struct nxs_function *f);
void nxs_function_stop(struct nxs_function *f);
void nxs_function_disconnect(struct nxs_function *f);
struct nxs_dev *nxs_function_find_dev(struct nxs_function *f,
				      u32 dev_function);
int nxs_function_register_display(struct nxs_function *f);
int nxs_function_unregister_display(struct nxs_function *f);
int nxs_function_add_to_display(int disp, struct nxs_function *f);
int nxs_function_remove_from_display(int disp, struct nxs_function *f);

#endif
