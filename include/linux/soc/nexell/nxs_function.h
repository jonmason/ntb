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

#endif
