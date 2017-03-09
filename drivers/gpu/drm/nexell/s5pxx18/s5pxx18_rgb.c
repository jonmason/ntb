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

#include "s5pxx18_drv.h"

static int rgb_ops_open(struct nx_drm_display *display, int pipe)
{
	struct nx_rgb_dev *rgb = display->context;
	struct nx_control_dev *control = &rgb->control;

	pr_debug("%s: pipe.%d\n", __func__, pipe);

	control->module = pipe;

	return 0;
}

static int rgb_ops_enable(struct nx_drm_display *display)
{
	struct nx_rgb_dev *rgb = display->context;
	struct nx_control_dev *control = &rgb->control;
	int module = control->module;
	int pin = 0;

	/*
	 *  0 : Primary MLC  , 1 : Primary MPU,
	 *  2 : Secondary MLC, 3 : ResConv(LCDIF)
	 */
	pr_debug("%s dev.%d\n", __func__, module);

	switch (module) {
	case 0:
		pin = rgb->mpu_lcd ? 1 : 0;
		break;
	case 1:
		pin = rgb->mpu_lcd ? 3 : 2;
		break;
	default:
		pr_err("Failed, %s not support module %d\n",
			__func__, module);
		return -EINVAL;
	}

	nx_disp_top_set_primary_mux(pin);

	return 0;
}

static int rgb_ops_set_mode(struct nx_drm_display *display,
			struct drm_display_mode *mode, unsigned int flags)
{
	nx_display_mode_to_sync(mode, display);
	return 0;
}

static int rgb_ops_resume(struct nx_drm_display *display)
{
	nx_display_resume_resource(display);
	return 0;
}

static struct nx_drm_display_ops rgb_ops = {
	.open = rgb_ops_open,
	.enable = rgb_ops_enable,
	.set_mode = rgb_ops_set_mode,
	.resume = rgb_ops_resume,
};

void *nx_drm_display_rgb_get(struct device *dev,
			struct device_node *node,
			struct nx_drm_display *display)
{
	struct nx_rgb_dev *rgb;
	u32 mpu_lcd = 0;

	rgb = kzalloc(sizeof(*rgb), GFP_KERNEL);
	if (!rgb)
		return NULL;

	of_property_read_u32(node, "panel-mpu", &mpu_lcd);
	rgb->mpu_lcd = mpu_lcd ? true : false;

	display->context = rgb;
	display->ops = &rgb_ops;

	return &rgb->control;
}
