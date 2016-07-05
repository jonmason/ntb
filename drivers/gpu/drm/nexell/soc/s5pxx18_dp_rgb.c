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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>

#include "s5pxx18_dp_dev.h"

#define parse_read_prop(n, s, v)	{ \
	u32 _v;	\
	if (!of_property_read_u32(n, s, &_v))	\
		v = _v;	\
	}

int nx_dp_device_rgb_register(struct device *dev,
			struct device_node *np, struct dp_control_dev *dpc)
{
	struct dp_rgb_dev *out;
	u32 mpu_lcd = 0;

	out = devm_kzalloc(dev, sizeof(*out), GFP_KERNEL);
	if (IS_ERR(out))
		return -ENOMEM;

	parse_read_prop(np, "panel-mpu", mpu_lcd);
	out->mpu_lcd = mpu_lcd ? true : false;

	dpc->panel_type = dp_panel_type_rgb;
	dpc->dp_output = out;

	return 0;
}
