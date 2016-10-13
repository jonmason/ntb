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

#include "nx-rearcam-vendor.h"

struct nx_vendor_context *nx_rearcam_alloc_vendor_context(void)
{
	struct nx_vendor_context *ctx;

	ctx = kmalloc(sizeof(struct nx_vendor_context), GFP_KERNEL);
	if (!ctx)
		return NULL;

	pr_debug("+++ %s ---\n", __func__);

	/*	example		*/
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
}

bool nx_rearcam_decide(void *ctx)
{
	struct nx_vendor_context *_ctx = (struct nx_vendor_context *)ctx;

	pr_debug("+++ %s ---\n", __func__);

	return true;
}

void nx_rearcam_draw_rgb_overlay(int width, int height, int pixelbyte,
				int rotation, void *ctx, void *mem)
{
	struct nx_vendor_context *_ctx = (struct nx_vendor_context *)ctx;

	pr_debug("+++ %s +++\n", __func__);

	if (mem != NULL) {
		memset(mem, 0, width * height * pixelbyte);
		/* draw redbox at (0, 0) -- (50, 50) */
		{
			u32 color = 0xFFFF0000;
			int i, j;
			u32 *pbuffer = (u32 *)mem;

			for (i = 0; i < 50; i++) {
				for (j = 0; j < 50; j++) {
					if (rotation == 0)
						pbuffer[i * 1024 + j] = color;
				}
			}
		}
	}

	pr_debug("--- %s ---\n", __func__);
}
