/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongkeun, Choi <jkchoi@nexell.co.kr>
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
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#include "nx-rearcam.h"
#include "nx-rearcam-vendor.h"

#if defined(CONFIG_VIDEO_NEXELL_REARCAM_SAMPLEPARKINGLINE)
#include "parking_line.h"
#endif

#if defined(CONFIG_VIDEO_NEXELL_REARCAM_SAMPLEPARKINGLINE)
void nx_rearcam_draw_parking_guide_line(void *mem, void *ctx,
					int width, int height,
					int pixelbyte, int rotation)
{
	if (mem != NULL) {
		int i, j;
		int src_width, src_height;
		static int line_index;
		unsigned int *data;

		u32 *pbuffer = (u32 *)mem;

		line_index = line_index % 3;

		if (width == 1280 && height == 720)
			line_index += 3;

		src_width = parkingline_resolution[line_index][0];
		src_height = parkingline_resolution[line_index][1];

		switch(line_index) {
		case 0:
			data = parkingline_left_1024x600;
			break;
		case 1:
			data = parkingline_center_1024x600;
			break;
		case 2:
			data = parkingline_right_1024x600;
			break;
		case 3:
			data = parkingline_left_1280x720;
			break;
		case 4:
			data = parkingline_center_1280x720;
			break;
		case 5:
			data = parkingline_right_1280x720;
			break;
		}

		memset(mem, 0, width * height * pixelbyte);

		for (i = 0; i < src_height; i++) {
			for (j = 0; j < src_width; j++) {
				pbuffer[i * width + j] =
					data[i * src_width + j];
			}
		}

		line_index++;
	}
}
#endif

void nx_rearcam_draw_rgb_overlay(int width, int height, int pixelbyte,
				int rotation, void *ctx, void *mem)
{
	struct nx_vendor_context *_ctx = (struct nx_vendor_context *)ctx;

#if defined(CONFIG_VIDEO_NEXELL_REARCAM_SAMPLEPARKINGLINE)
	nx_rearcam_draw_parking_guide_line(mem, ctx, width, height, pixelbyte,
			rotation);
#else
	int i, j;
	int sample_size = 50;
	u32 color = 0xFFFF0000;
	u32 *pbuffer = (u32 *)mem;

	memset(mem, 0, width * height * pixelbyte);
	for (i = 0; i < sample_size; i++) {
		for (j = 0; j < sample_size; j++)
			pbuffer[i * width + j] = color;
	}
#endif
	pr_debug("+++ %s ---\n", __func__);
}

void nx_rearcam_sensor_init_func(struct i2c_client *client)
{
}

struct nx_vendor_context *nx_rearcam_alloc_vendor_context(void *priv,
        struct device *dev)
{
	struct nx_vendor_context *ctx;
	struct device_node *np = dev->of_node;
	struct device_node *gpio_node;

	ctx = kmalloc(sizeof(struct nx_vendor_context), GFP_KERNEL);
	if (!ctx)
		return NULL;

	pr_debug("+++ %s ---\n", __func__);
	/*	example		*/
	ctx->priv = priv;
	ctx->type = 1;

	return ctx;
}

bool nx_rearcam_pre_turn_on(void *ctx)
{
	struct nx_vendor_context *_ctx = (struct nx_vendor_context *)ctx;

	pr_debug("+++ %s ---\n", __func__);

	pr_debug("%s - type : %d\n", __func__, _ctx->type);

	return true;
}

void nx_rearcam_post_turn_off(void *ctx)
{
	struct nx_vendor_context *_ctx = (struct nx_vendor_context *)ctx;

	pr_debug("+++ %s ---\n", __func__);
}

void nx_rearcam_free_vendor_context(void *ctx)
{
	struct nx_vendor_context *_ctx = (struct nx_vendor_context *)ctx;

	pr_debug("+++ %s ---\n", __func__);
	kfree(_ctx);
}

bool nx_rearcam_decide(void *ctx)
{
	struct nx_vendor_context *_ctx = (struct nx_vendor_context *)ctx;
	bool is_on = false;

	pr_debug("+++ %s ---is_on:%d\n", __func__, is_on);

	return is_on;
}

