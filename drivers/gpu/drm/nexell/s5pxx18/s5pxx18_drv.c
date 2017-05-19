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
#include <drm/drmP.h>
#include <drm/drm_fb_helper.h>
#include <uapi/drm/drm_fourcc.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/dma-buf.h>

#include "../nx_drm_fb.h"
#include "../nx_drm_gem.h"

#include "s5pxx18_drv.h"
#include "s5pxx18_hdmi.h"

/* for cluster lcd */
static struct list_head crtc_top_list = LIST_HEAD_INIT(crtc_top_list);
static struct list_head dp_dpc_list = LIST_HEAD_INIT(dp_dpc_list);

#define property_read(n, s, v)	of_property_read_u32(n, s, &v)
#define	dp_to_control(d)	\
		((struct nx_control_dev *)d->context)

static const uint32_t support_formats_rgb[] = {
	DRM_FORMAT_RGB565,
	DRM_FORMAT_BGR565,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
};

static const uint32_t support_formats_vid[] = {
	/* 1 buffer */
	DRM_FORMAT_YUYV,
	DRM_FORMAT_YVYU,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_VYUY,
	/* 2 buffer */
	DRM_FORMAT_NV12,
	DRM_FORMAT_NV21,
	DRM_FORMAT_NV16,
	DRM_FORMAT_NV61,
	/* 3 buffer */
	DRM_FORMAT_YUV410,
	DRM_FORMAT_YVU410,
	DRM_FORMAT_YUV411,
	DRM_FORMAT_YVU411,
	DRM_FORMAT_YUV420,
	DRM_FORMAT_YVU420,
	DRM_FORMAT_YUV422,
	DRM_FORMAT_YVU422,
	DRM_FORMAT_YUV444,
	DRM_FORMAT_YVU444,
};

static struct plane_format_type {
	const void *formats;
	int format_count;
} plane_formats[] = {
	{
		.formats = support_formats_rgb,
		.format_count = ARRAY_SIZE(support_formats_rgb),
	},
	{
		.formats = support_formats_vid,
		.format_count = ARRAY_SIZE(support_formats_vid),
	},
};

static int convert_format_rgb(uint32_t pixel_format,
			uint32_t bpp, uint32_t depth, uint32_t *format)
{
	uint32_t fmt;

	switch (pixel_format) {
	/* 16 bpp RGB */
	case DRM_FORMAT_XRGB1555:
		fmt = nx_mlc_rgbfmt_x1r5g5b5;
		break;
	case DRM_FORMAT_XBGR1555:
		fmt = nx_mlc_rgbfmt_x1b5g5r5;
		break;
	case DRM_FORMAT_RGB565:
		fmt = nx_mlc_rgbfmt_r5g6b5;
		break;
	case DRM_FORMAT_BGR565:
		fmt = nx_mlc_rgbfmt_b5g6r5;
		break;
	/* 24 bpp RGB */
	case DRM_FORMAT_RGB888:
		fmt = nx_mlc_rgbfmt_r8g8b8;
		break;
	case DRM_FORMAT_BGR888:
		fmt = nx_mlc_rgbfmt_b8g8r8;
		break;
	/* 32 bpp RGB */
	case DRM_FORMAT_XRGB8888:
		fmt = nx_mlc_rgbfmt_x8r8g8b8;
		break;
	case DRM_FORMAT_XBGR8888:
		fmt = nx_mlc_rgbfmt_x8b8g8r8;
		break;
	case DRM_FORMAT_ARGB8888:
		fmt = nx_mlc_rgbfmt_a8r8g8b8;
		break;
	case DRM_FORMAT_ABGR8888:
		fmt = nx_mlc_rgbfmt_a8b8g8r8;
		break;
	default:
		DRM_ERROR("Failed, not support %s pixel format\n",
			drm_get_format_name(pixel_format));
		return -EINVAL;
	}

	*format = fmt;
	return 0;
}

static uint32_t convert_format_video(uint32_t fourcc,
			uint32_t *format)
{
	uint32_t fmt;

	switch (fourcc) {
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU411:
		fmt = nx_mlc_yuvfmt_420 | 0x1<<31;
		break;
	case DRM_FORMAT_YUV422:
		fmt = nx_mlc_yuvfmt_422 | 0x1<<31;
		break;
	case DRM_FORMAT_YUV444:
		fmt = nx_mlc_yuvfmt_444 | 0x1<<31;
		break;
	case DRM_FORMAT_YUYV:
		fmt = nx_mlc_yuvfmt_yuyv | 0x1<<31;
		break;
	default:
		DRM_ERROR("Failed, not support fourcc %s\n",
		       drm_get_format_name(fourcc));
		return -EINVAL;
	}

	*format = fmt;
	return 0;
}

#define LIST_LAYER_ENTRY(_l, _cl, _n)	{ \
		list_for_each_entry(_l, &_cl->plane_list, list) {	\
			if (_n == _l->num)	\
				break;	\
		}	\
		if (!_l)	\
			continue;	\
	}

static inline void rect_plane_rgb(struct nx_plane_rect *r,
			int cx, int cy, int cw, int ch,
			int aw, int ah, enum nx_cluster_dir dir)
{
	r->left = cx;
	r->right = cx + cw;
	cw = r->right - r->left;

	if (r->left < 0)
		r->left = 0;

	if (r->left >= aw)
		goto invalid;

	if (r->right > aw)
		r->right = aw;

	if (cw <= 0)
		goto invalid;

	r->top = cy;
	r->bottom = cy + ch;
	ch = r->bottom - r->top;

	if (r->top >= ah)
		goto invalid;

	if (r->bottom <= 0)
		goto invalid;

	if (ch <= 0)
		goto invalid;

	return;

invalid:
	r->left = 0, r->right = 0;
	r->top = 0, r->bottom = 0;
}

static inline void rect_plane_video(struct nx_plane_rect *r,
			int cx, int cy, int cw, int ch,
			int aw, int ah, enum nx_cluster_dir dir)
{
	r->left = cx;
	r->right = cx + cw;

	if (r->right <= 0)
		goto invalid;

	if (r->left >= aw)
		goto invalid;

	r->top = cy;
	r->bottom = cy + ch;

	if (r->top >= ah)
		goto invalid;

	if (r->bottom <= 0)
		goto invalid;

	return;

invalid:
	r->left = 0, r->right = 0;
	r->top = 0, r->bottom = 0;
}

static void crtc_mlc_resume(struct drm_crtc *crtc)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct nx_top_plane *top = nx_crtc->context;
	void __iomem *base;

	if (!nx_crtc->cluster) {
		base = nx_mlc_get_base_address(top->module);
		memcpy(base, top->regs, sizeof(top->regs));
		nx_soc_dp_plane_top_prepare(top);
		return;
	}

	list_for_each_entry(top, &crtc_top_list, list) {
		base = nx_mlc_get_base_address(top->module);
		memcpy(base, top->regs, sizeof(top->regs));
		nx_soc_dp_plane_top_prepare(top);
	}
}

static void crtc_mlc_suspend(struct drm_crtc *crtc)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct nx_top_plane *top = nx_crtc->context;
	void __iomem *base;

	if (!nx_crtc->cluster) {
		base = nx_mlc_get_base_address(top->module);
		memcpy(top->regs, base, sizeof(top->regs));
		return;
	}

	list_for_each_entry(top, &crtc_top_list, list) {
		base = nx_mlc_get_base_address(top->module);
		memcpy(top->regs, base, sizeof(top->regs));
	}
}

static void crtc_dpc_resume(struct nx_control_dev *control)
{
	void __iomem *base;

	if (!control->cluster) {
		base = nx_dpc_get_base_address(control->module);
		memcpy(base, control->regs, sizeof(control->regs));
		nx_soc_dp_cont_dpc_clk_on(control);
		return;
	}

	list_for_each_entry(control, &dp_dpc_list, list) {
		base = nx_dpc_get_base_address(control->module);
		memcpy(base, control->regs, sizeof(control->regs));
		nx_soc_dp_cont_dpc_clk_on(control);
	}
}

static void crtc_dpc_suspend(struct nx_control_dev *control)
{
	void __iomem *base;

	if (!control->cluster) {
		base = nx_dpc_get_base_address(control->module);
		memcpy(control->regs, base, sizeof(control->regs));
		return;
	}

	list_for_each_entry(control, &dp_dpc_list, list) {
		base = nx_dpc_get_base_address(control->module);
		memcpy(control->regs, base, sizeof(control->regs));
	}
}

static int crtc_ops_begin(struct drm_crtc *crtc)
{
	struct drm_framebuffer *fb = crtc->primary->state->fb;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct nx_top_plane *top = nx_crtc->context;
	struct nx_plane_layer *primary = to_nx_plane(crtc->primary)->context;
	int crtc_w, crtc_h;
	int i = 0;

	crtc_w = crtc->state->mode.hdisplay;
	crtc_h = crtc->state->mode.vdisplay;

	DRM_DEBUG_KMS("crtc.%d: [%d x %d] -> [%d,%d]\n",
		top->module, fb->width, fb->height, crtc_w, crtc_h);
	DRM_DEBUG_KMS("%s pixel:%d, back:0x%x, colorkey:0x%x\n",
		drm_get_format_name(fb->pixel_format),
		fb->bits_per_pixel >> 3,
		top->back_color, top->color_key);

	if (!nx_crtc->cluster) {
		nx_soc_dp_plane_top_prepare(top);
		nx_soc_dp_plane_top_set_bg_color(top);
		nx_soc_dp_plane_top_set_format(top, crtc_w, crtc_h);
		/* color key */
		nx_soc_dp_plane_rgb_set_color(primary,
			NX_COLOR_TRANS, top->color_key, true, false);
		return 0;
	}

	list_for_each_entry(top, &crtc_top_list, list) {
		crtc_w = nx_crtc->cluster->vm[i].hactive;
		crtc_h = nx_crtc->cluster->vm[i].vactive;
		top->cluster = true;
		i++;

		nx_soc_dp_plane_top_prepare(top);
		nx_soc_dp_plane_top_set_bg_color(top);
		nx_soc_dp_plane_top_set_format(top, crtc_w, crtc_h);
		/* color key */
		nx_soc_dp_plane_rgb_set_color(primary,
			NX_COLOR_TRANS, top->color_key, true, false);
	}

	return 0;
}

static void crtc_ops_enable(struct drm_crtc *crtc)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct nx_top_plane *top = nx_crtc->context;

	DRM_DEBUG_KMS("crtc.%d\n", top->module);

	if (nx_crtc->suspended)
		crtc_mlc_resume(crtc);

	if (!nx_crtc->cluster) {
		nx_soc_dp_plane_top_set_enable(top, 1);
		return;
	}

	list_for_each_entry(top, &crtc_top_list, list)
		nx_soc_dp_plane_top_set_enable(top, 1);
}

static void crtc_ops_disable(struct drm_crtc *crtc)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct nx_top_plane *top = nx_crtc->context;

	DRM_DEBUG_KMS("crtc.%d\n", top->module);

	if (nx_crtc->suspended)
		crtc_mlc_suspend(crtc);

	if (!nx_crtc->cluster) {
		nx_soc_dp_plane_top_set_enable(top, 0);
		return;
	}

	list_for_each_entry(top, &crtc_top_list, list)
		nx_soc_dp_plane_top_set_enable(top, 0);
}

static void crtc_ops_reset(struct drm_crtc *crtc)
{
	struct nx_top_plane *top = to_nx_crtc(crtc)->context;
	struct nx_control_res *res = &top->res;
	bool stat;
	int i = 0;

	DRM_DEBUG_KMS("crtc.%d\n", top->module);

	for (i = 0; res->num_resets > i; i++) {
		stat = reset_control_status(res->resets[i]);
		if (stat)
			reset_control_reset(res->resets[i]);
	}
}

static void crtc_ops_destory(struct drm_crtc *crtc)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct nx_top_plane *top = nx_crtc->context;
	int module = top->module;
	void __iomem *base;

	base = nx_dpc_get_base_address(module);
	iounmap(base);

	base = nx_mlc_get_base_address(module);
	iounmap(base);

	kfree(nx_crtc->context);
}

static struct nx_drm_crtc_ops nx_crtc_ops = {
	.reset = crtc_ops_reset,
	.destroy = crtc_ops_destory,
	.begin = crtc_ops_begin,
	.enable = crtc_ops_enable,
	.disable = crtc_ops_disable,
};

static int crtc_planes_parse(struct device *dev, struct nx_drm_crtc *nx_crtc,
			struct nx_top_plane *top, int pipe)
{
	struct device_node *np;
	const char *strings[10];
	int i, size = 0;

	/* port node */
	np = of_graph_get_port_by_id(dev->of_node, pipe);
	if (!np)
		return -ENOENT;
	of_node_put(np);

	size = of_property_read_string_array(np, "plane-names", strings, 10);
	for (i = 0; size > i; i++) {
		if (!strcmp("primary", strings[i])) {
			top->planes_type[i] =
				DRM_PLANE_TYPE_PRIMARY | NX_PLANE_TYPE_RGB;
		} else if (!strcmp("cursor", strings[i])) {
			top->planes_type[i] =
				DRM_PLANE_TYPE_CURSOR | NX_PLANE_TYPE_RGB;
		} else if (!strcmp("rgb", strings[i])) {
			top->planes_type[i] =
				DRM_PLANE_TYPE_OVERLAY | NX_PLANE_TYPE_RGB;
		} else if (!strcmp("video", strings[i])) {
			top->planes_type[i] =
				DRM_PLANE_TYPE_OVERLAY | NX_PLANE_TYPE_VIDEO;
			top->video_priority = i;	/* priority */
		} else {
			top->planes_type[i] = NX_PLANE_TYPE_UNKNOWN;
			DRM_ERROR("Failed, unknown plane name [%d] %s\n",
				i, strings[i]);
		}
		DRM_DEBUG_KMS("crtc.%d planes[%d]: %s, bg:0x%08x, key:0x%08x\n",
			pipe, i, strings[i], top->back_color,
			top->color_key);
	}

	nx_crtc->num_planes = size;
	property_read(np, "back_color", top->back_color);
	property_read(np, "color_key", top->color_key);

	return 0;
}

static int crtc_dt_parse(struct drm_device *drm, struct nx_drm_crtc *nx_crtc,
			struct nx_top_plane *top, int pipe)
{
	struct platform_device *pdev = drm->platformdev;
	struct device *dev = &pdev->dev;
	struct reset_control **resets = top->res.resets;
	int *num_resets = &top->res.num_resets;
	const char *strings[10];
	void __iomem *base[8];
	int i, n, size = 0;
	int offset = 4;	/* mlc or control */
	int err;

	DRM_DEBUG_KMS("crtc.%d for %s\n", pipe, dev_name(dev));

	/* parse base address */
	size = of_property_read_string_array(dev->of_node,
			"reg-names", strings, ARRAY_SIZE(strings));
	if (size > ARRAY_SIZE(strings))
		return -EINVAL;

	for (n = 0, i = 0; size > i; i++) {
		const char *c = strings[i] + offset;
		unsigned long no;

		if (kstrtoul(c, 0, &no) < 0)
			continue;

		if (pipe != no)
			continue;

		base[n] = of_iomap(dev->of_node, i);
		if (!base[n]) {
			DRM_DEBUG_KMS("Failed, %s iomap\n", strings[i]);
			return -EINVAL;
		}
		n++;
	}

	/* parse interrupts */
	size = of_property_read_string_array(dev->of_node,
			"interrupts-names", strings, ARRAY_SIZE(strings));
	if (size > ARRAY_SIZE(strings))
		return -EINVAL;

	for (n = 0, i = 0; size > i; i++) {
		const char *c = strings[i] + offset;
		unsigned long no;

		if (kstrtoul(c, 0, &no) < 0)
			continue;

		if (pipe != no)
			continue;

		err = platform_get_irq(pdev, i);
		if (err < 0)
			return -EINVAL;

		nx_crtc->irq = err;
	}

	/* parse reset address */
	size = of_property_read_string_array(dev->of_node,
				"reset-names", strings, ARRAY_SIZE(strings));

	for (i = 0; size > i; i++, resets++) {
		*resets = devm_reset_control_get(dev, strings[i]);
		if (*resets) {
			bool stat = reset_control_status(*resets);

			if (stat)
				reset_control_reset(*resets);
		}
		DRM_DEBUG_KMS("reset[%d]: %s:%p\n", i, strings[i], *resets);
	}
	*num_resets = size;

	nx_soc_dp_cont_dpc_base(pipe, base[0]);
	nx_soc_dp_cont_mlc_base(pipe, base[1]);

	return crtc_planes_parse(dev, nx_crtc, top, pipe);
}

int nx_drm_crtc_init(struct drm_device *drm,
			struct drm_crtc *crtc, int pipe)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct nx_top_plane *top;
	int err;

	top = kzalloc(sizeof(*top), GFP_KERNEL);
	if (!top)
		return -ENOMEM;

	top->module = pipe;
	top->crtc = crtc;

	err = crtc_dt_parse(drm, nx_crtc, top, pipe);
	if (err < 0) {
		kfree(top);
		return err;
	}

	INIT_LIST_HEAD(&top->plane_list);
	INIT_LIST_HEAD(&top->list);
	list_add_tail(&top->list, &crtc_top_list);

	nx_crtc->context = top;
	nx_crtc->ops = &nx_crtc_ops;

	return 0;
}

static int plane_wait_sync(struct drm_plane *plane,
			struct drm_framebuffer *fb, bool sync)
{
	struct drm_gem_object *obj;
	struct nx_plane_layer *layer = to_nx_plane(plane)->context;
	int plane_num = 0;
	long ts;
	int ret;

	obj = to_gem_obj(nx_drm_fb_gem(fb, plane_num));

	ts = ktime_to_ms(ktime_get());
	ret = nx_drm_gem_wait_fence(obj);
	ts = ktime_to_ms(ktime_get()) - ts;

	DRM_DEBUG_KMS("crtc.%d plane.%d (%s) : wait:%3ldms, ret:%d\n",
		layer->module, layer->num, layer->name, ts, ret);

	return ret;
}

static int plane_rgb_update(struct drm_framebuffer *fb,
			struct nx_plane_layer *layer,
			int crtc_x, int crtc_y, int crtc_w, int crtc_h,
			int src_x, int src_y, int src_w, int src_h, int align)
{
	struct nx_gem_object *nx_obj = nx_drm_fb_gem(fb, 0);
	struct nx_top_plane *top = layer->top_plane;
	dma_addr_t dma_addr = nx_obj->dma_addr;
	int pixel = (fb->bits_per_pixel >> 3);
	int num = layer->num;
	int pitch = fb->pitches[0];
	unsigned int format;
	struct nx_plane_rect rect;
	int ret;

	ret = convert_format_rgb(fb->pixel_format,
			fb->bits_per_pixel, fb->depth, &format);
	if (ret < 0)
		return ret;

	if (!top->cluster) {
		rect_plane_rgb(&rect, crtc_x, crtc_y, crtc_w, crtc_h,
				top->width, top->height, 0);
		nx_soc_dp_plane_rgb_set_format(layer, format, pixel, true);
		nx_soc_dp_plane_rgb_set_position(layer,
				src_x, src_y, src_w, src_h, rect.left, rect.top,
				rect.right - rect.left,
				rect.bottom - rect.top, true);
		nx_soc_dp_plane_rgb_set_address(layer,
				dma_addr, pixel, pitch, align, true);
		nx_soc_dp_plane_rgb_set_enable(layer, true, true);
	} else {
		int cx, cy, cw, ch;
		int aw, ah;

		cx = crtc_x, cy = crtc_y;
		cw = crtc_w, ch = crtc_h;

		list_for_each_entry(top, &crtc_top_list, list) {
			aw = top->width, ah = top->height;
			rect_plane_rgb(&rect, cx, cy, cw, ch, aw, ah,
					top->direction);
			cx = cx - aw;

			LIST_LAYER_ENTRY(layer, top, num);

			nx_soc_dp_plane_rgb_set_format(
						layer, format, pixel, true);
			nx_soc_dp_plane_rgb_set_position(layer,	src_x, src_y,
					src_w, src_h, rect.left, rect.top,
					rect.right - rect.left,
					rect.bottom - rect.top, true);
			nx_soc_dp_plane_rgb_set_address(layer,
					dma_addr, pixel, pitch, align, true);
			nx_soc_dp_plane_rgb_set_enable(layer, true, true);

			/* next fb base */
			dma_addr += layer->dst_width * pixel;
		}
	}
	return 0;
}

static int plane_video_base(struct nx_plane_layer *layer,
			dma_addr_t dma_addrs[4], unsigned int pitches[4],
			unsigned int offsets[4], int planes)
{
	dma_addr_t lua, cba, cra;
	int lus, cbs, crs;
	int ret = 0;

	switch (planes) {
	case 1:
		lua = dma_addrs[0], lus = pitches[0];
		nx_soc_dp_plane_video_set_address_1p(layer,
				lua, lus, true);
		break;
	case 2:
	case 3:
		lua = dma_addrs[0];
		cba = offsets[1] ? lua + offsets[1] : dma_addrs[1];
		cra = offsets[2] ? lua + offsets[2] : dma_addrs[2];
		lus = pitches[0], cbs = pitches[1], crs = pitches[2];
		nx_soc_dp_plane_video_set_address_3p(layer, lua, lus,
				cba, cbs, cra, crs, true);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int plane_video_update(struct drm_framebuffer *fb,
			struct nx_plane_layer *layer,
			int crtc_x, int crtc_y, int crtc_w, int crtc_h,
			int src_x, int src_y, int src_w, int src_h)
{
	struct nx_top_plane *top = layer->top_plane;
	struct nx_gem_object *nx_obj[4];
	dma_addr_t dma_addrs[4];
	unsigned int pitches[4], offsets[4];
	unsigned int format;
	int num = layer->num;
	int num_planes;
	struct nx_plane_rect rect;
	int i, ret;

	num_planes = drm_format_num_planes(fb->pixel_format);
	for (i = 0; num_planes > i; i++) {
		nx_obj[i] = nx_drm_fb_gem(fb, i);
		dma_addrs[i] = nx_obj[i]->dma_addr;
		offsets[i] = fb->offsets[i];
		pitches[i] = fb->pitches[i];
	}

	ret = convert_format_video(fb->pixel_format, &format);
	if (ret < 0)
		return ret;

	if (!top->cluster) {
		rect_plane_video(&rect, crtc_x, crtc_y, crtc_w, crtc_h,
				top->width, top->height, 0);
		nx_soc_dp_plane_video_set_format(layer, format, true);
		nx_soc_dp_plane_video_set_position(layer,
				src_x, src_y, src_w, src_h,
				rect.left, rect.top, rect.right - rect.left,
				rect.bottom - rect.top, true);

		ret = plane_video_base(layer,
				dma_addrs, pitches, offsets, num_planes);
		if (ret < 0)
			return ret;

		nx_soc_dp_plane_video_set_enable(layer, true, true);
	} else {
		int cx, cy, cw, ch;
		int aw, ah;

		cx = crtc_x, cy = crtc_y;
		cw = crtc_w, ch = crtc_h;

		list_for_each_entry(top, &crtc_top_list, list) {
			aw = top->width, ah = top->height;
			rect_plane_video(&rect, cx, cy, cw, ch,
						aw, ah, top->direction);
			cx = cx - aw;

			LIST_LAYER_ENTRY(layer, top, num);

			nx_soc_dp_plane_video_set_format(layer, format, true);
			nx_soc_dp_plane_video_set_position(layer,
					src_x, src_y, src_w, src_h,
					rect.left, rect.top,
					rect.right - rect.left,
					rect.bottom - rect.top, true);

			ret = plane_video_base(layer, dma_addrs,
					pitches, offsets, num_planes);
			if (ret < 0)
				continue;

			nx_soc_dp_plane_video_set_enable(layer, true, true);
		}
	}
	return 0;
}

static int plane_ops_update(struct drm_plane *plane,
			struct drm_framebuffer *fb,
			int crtc_x, int crtc_y, int crtc_w, int crtc_h,
			int src_x, int src_y, int src_w, int src_h)
{
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);
	struct nx_plane_layer *layer = nx_plane->context;
	int ret;

	DRM_DEBUG_KMS("crtc.%d plane.%d (%s)\n",
		layer->module, layer->num, layer->name);
	DRM_DEBUG_KMS("%s crtc[%d,%d,%d,%d] src[%d,%d,%d,%d]\n",
		drm_get_format_name(fb->pixel_format),
		crtc_x, crtc_y, crtc_w, crtc_h, src_x, src_y, src_w, src_h);

	if (!(layer->type & NX_PLANE_TYPE_VIDEO)) {
		ret = plane_wait_sync(plane, fb, false);
		if (!ret)
			ret = plane_rgb_update(fb, layer,
				crtc_x, crtc_y, crtc_w, crtc_h,
				src_x, src_y, src_w, src_h, nx_plane->align);
	} else {
		ret = plane_video_update(fb, layer,
				crtc_x, crtc_y, crtc_w, crtc_h,
				src_x, src_y, src_w, src_h);
	}
	return ret;
}

static void plane_ops_disable(struct drm_plane *plane)
{
	struct nx_plane_layer *layer = to_nx_plane(plane)->context;
	struct nx_top_plane *top = layer->top_plane;
	int num = layer->num;

	DRM_DEBUG_KMS("enter (%s)\n",
		layer->type & NX_PLANE_TYPE_VIDEO ?  "VIDEO" : "RGB");

	if (!top->cluster) {
		if (layer->type & NX_PLANE_TYPE_VIDEO)
			nx_soc_dp_plane_video_set_enable(layer, false, true);
		else
			nx_soc_dp_plane_rgb_set_enable(layer, false, true);
		return;
	}

	list_for_each_entry(top, &crtc_top_list, list) {
		LIST_LAYER_ENTRY(layer, top, num);
		if (layer->type & NX_PLANE_TYPE_VIDEO)
			nx_soc_dp_plane_video_set_enable(layer, false, true);
		else
			nx_soc_dp_plane_rgb_set_enable(layer, false, true);

	}
}

static void plane_set_color(struct drm_plane *plane,
			enum nx_color_type type, unsigned int color)
{
	struct nx_plane_layer *layer = to_nx_plane(plane)->context;
	struct nx_top_plane *top = layer->top_plane;
	struct drm_crtc *crtc = top->crtc;
	struct nx_plane_layer *primary = to_nx_plane(crtc->primary)->context;
	int num = layer->num;

	if (!top->cluster) {
		if (type == NX_COLOR_COLORKEY) {
			layer = primary;
			type = NX_COLOR_TRANS;
		}
		nx_soc_dp_plane_rgb_set_color(layer, type, color, true, true);
		return;
	}

	list_for_each_entry(top, &crtc_top_list, list) {
		if (type == NX_COLOR_COLORKEY) {
			layer = primary;
			type = NX_COLOR_TRANS;
			nx_soc_dp_plane_rgb_set_color(
				layer, type, color, true, true);
		} else {
			LIST_LAYER_ENTRY(layer, top, num);
			nx_soc_dp_plane_rgb_set_color(
				layer, type, color, true, true);
		}
	}
}

static void plane_set_priority(struct drm_plane *plane, int priority)
{
	struct nx_plane_layer *layer = to_nx_plane(plane)->context;
	struct nx_top_plane *top = layer->top_plane;

	if (!top->cluster) {
		nx_soc_dp_plane_video_set_priority(layer, priority);
		return;
	}

	list_for_each_entry(top, &crtc_top_list, list) {
		list_for_each_entry(layer, &top->plane_list, list) {
			if (layer->type & NX_PLANE_TYPE_VIDEO) {
				nx_soc_dp_plane_video_set_priority(
					layer, priority);
				break;
			}
		}
	}
}

static int plane_set_property(struct drm_plane *plane,
			struct drm_plane_state *state,
			struct drm_property *property, uint64_t val)
{
	struct nx_plane_layer *layer = to_nx_plane(plane)->context;
	struct plane_property *prop = &layer->property;
	union color *color = &prop->color;

	DRM_DEBUG_KMS("%s : %s 0x%llx\n", layer->name, property->name, val);

	if (property == color->yuv.colorkey) {
		layer->color.colorkey = val;
		plane_set_color(plane, NX_COLOR_COLORKEY, val);
	}

	if (property == color->rgb.transcolor) {
		layer->color.transcolor = val;
		plane_set_color(plane, NX_COLOR_TRANS, val);
	}

	if (property == color->rgb.alphablend) {
		layer->color.alphablend = val;
		plane_set_color(plane, NX_COLOR_ALPHA, val);
	}

	if (property == prop->priority) {
		struct nx_top_plane *top = layer->top_plane;

		top->video_priority = val;
		plane_set_priority(plane, val);
	}

	return 0;
}

static int plane_get_property(struct drm_plane *plane,
			const struct drm_plane_state *state,
			struct drm_property *property, uint64_t *val)
{
	struct nx_plane_layer *layer = to_nx_plane(plane)->context;
	struct plane_property *prop = &layer->property;
	union color *color = &prop->color;

	DRM_DEBUG_KMS("%s : %s\n", layer->name, property->name);

	if (property == color->yuv.colorkey)
		*val = layer->color.colorkey;

	if (property == color->rgb.transcolor)
		*val = layer->color.transcolor;

	if (property == color->rgb.alphablend)
		*val = layer->color.alphablend;

	if (property == prop->priority) {
		struct nx_top_plane *top = layer->top_plane;

		*val = top->video_priority;
	}

	return 0;
}

static void plane_ops_create_proeprties(struct drm_device *drm,
			struct drm_crtc *crtc, struct drm_plane *plane,
			struct drm_plane_funcs *plane_funcs)
{
	struct nx_plane_layer *layer = to_nx_plane(plane)->context;
	struct nx_top_plane *top = layer->top_plane;
	struct plane_property *prop = &layer->property;
	union color *color = &prop->color;

	DRM_DEBUG_KMS("crtc.%d plane.%d (%s)\n",
		layer->module, layer->num, layer->name);

	if (layer->type & NX_PLANE_TYPE_VIDEO) {
		layer->color.colorkey = top->color_key;
		/* YUV color */
		color->yuv.colorkey =
		drm_property_create_range(drm, 0, "colorkey", 0, 0xffffffff);
		drm_object_attach_property(&plane->base,
			color->yuv.colorkey, layer->color.colorkey);
	} else {
		/* RGB color */
		color->rgb.transcolor =
		drm_property_create_range(drm, 0, "transcolor", 0, 0xffffffff);
		drm_object_attach_property(&plane->base,
			color->rgb.transcolor, layer->color.transcolor);

		color->rgb.alphablend =
		drm_property_create_range(drm, 0, "alphablend", 0, 0xffffffff);
		drm_object_attach_property(&plane->base,
			color->rgb.alphablend, layer->color.alphablend);
	}

	/* video prority */
	prop->priority =
	drm_property_create_range(drm, 0, "video-priority", 0, 2);
	drm_object_attach_property(&plane->base,
			prop->priority, top->video_priority);

	/* plane property functions */
	plane_funcs->atomic_set_property = plane_set_property;
	plane_funcs->atomic_get_property = plane_get_property;
}

static void plane_ops_destory(struct drm_plane *plane)
{
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);

	kfree(nx_plane->context);
}

static struct nx_drm_plane_ops nx_plane_ops = {
	.update = plane_ops_update,
	.disable = plane_ops_disable,
	.create_proeprties = plane_ops_create_proeprties,
	.destroy = plane_ops_destory,
};

static int plane_create(struct drm_device *drm,
			struct drm_crtc *crtc, struct nx_drm_plane *nx_plane,
			bool bgr, int plane_num)
{
	struct nx_top_plane *top;
	struct nx_plane_layer *layer;
	struct plane_format_type *format;

	layer = kzalloc(sizeof(*layer), GFP_KERNEL);
	if (!layer)
		return -ENOMEM;

	top = to_nx_crtc(crtc)->context;
	layer->top_plane = top;
	layer->module = top->module;
	layer->num = plane_num;
	layer->is_bgr = bgr;
	layer->type |= plane_num == PLANE_VIDEO_NUM ?
				NX_PLANE_TYPE_VIDEO : 0;
	layer->color.alpha = layer->type & NX_PLANE_TYPE_VIDEO ? 15 : 0;

	sprintf(layer->name, "%d-%s%d", top->module,
		layer->type & NX_PLANE_TYPE_VIDEO ? "vid" : "rgb", plane_num);

	list_add_tail(&layer->list, &top->plane_list);
	format = &plane_formats[layer->type & NX_PLANE_TYPE_VIDEO ? 1 : 0];

	nx_plane->context = layer;
	nx_plane->ops = &nx_plane_ops;
	nx_plane->formats = format->formats;
	nx_plane->format_count = format->format_count;

	DRM_DEBUG_KMS("crtc.%d plane.%d (%s)\n",
		layer->module, layer->num, layer->name);

	return 0;
}

int nx_drm_planes_init(struct drm_device *drm, struct drm_crtc *crtc,
			struct drm_plane **planes, int num_planes,
			enum drm_plane_type *drm_types, bool bgr)
{
	struct nx_top_plane *top = to_nx_crtc(crtc)->context;
	int num = 0, plane_num = 0;
	int i = 0;

	for (i = 0; num_planes > i; i++) {
		struct nx_drm_plane *nx_plane = to_nx_plane(planes[i]);
		enum drm_plane_type drm_type;

		drm_types[i] = top->planes_type[i];
		drm_type = drm_types[i] & 0xff;

		if (drm_type == NX_PLANE_TYPE_UNKNOWN)
			continue;

		plane_num = drm_types[i] & NX_PLANE_TYPE_VIDEO ?
				PLANE_VIDEO_NUM : num++;

		plane_create(drm, crtc, nx_plane, bgr, plane_num);
	}

	return num_planes;
}

#ifdef CONFIG_FB_PRE_INIT_FB
dma_addr_t nx_drm_plane_get_dma_addr(struct drm_plane *plane)
{
	struct nx_plane_layer *layer;
	dma_addr_t dma_addr;

	if (!plane)
		return 0;

	layer = to_nx_plane(plane)->context;
	nx_mlc_get_rgblayer_address(
		layer->module, layer->num, (u32 *)&dma_addr);

	return dma_addr;
}
#endif

static int encoder_ops_dpms_status(struct drm_encoder *encoder)
{
	struct nx_drm_connector *nx_connector =
			to_nx_encoder(encoder)->connector;
	struct nx_drm_display *display = nx_connector->display;
	struct nx_control_dev *control = dp_to_control(display);
	bool poweron;

	poweron = nx_soc_dp_cont_power_status(control);

	return poweron ? DRM_MODE_DPMS_ON : DRM_MODE_DPMS_OFF;
}

static void encoder_ops_enable(struct drm_encoder *encoder)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(encoder->crtc);
	struct nx_drm_connector *nx_connector =
				to_nx_encoder(encoder)->connector;
	struct nx_drm_display *display = nx_connector->display;
	struct nx_control_dev *control = dp_to_control(display);

	DRM_DEBUG_KMS("%s\n", nx_panel_get_name(display->panel_type));

	/*
	 * set output module index
	 */
	if (!nx_crtc->cluster)
		control->module = nx_crtc->pipe;
	else
		control->cluster = true;

	if (display->disable_output)
		return;

	if (nx_connector->suspended)
		crtc_dpc_resume(control);

	if (!control->cluster) {
		nx_soc_dp_cont_dpc_clk_on(control);
		nx_soc_dp_cont_prepare(control);
		nx_soc_dp_cont_power_on(control, true);
		return;
	}

	list_for_each_entry(control, &dp_dpc_list, list) {
		nx_soc_dp_cont_dpc_clk_on(control);
		nx_soc_dp_cont_prepare(control);
		nx_soc_dp_cont_power_on(control, true);
	}
}

static void encoder_ops_disable(struct drm_encoder *encoder)
{
	struct nx_drm_connector *nx_connector =
				to_nx_encoder(encoder)->connector;
	struct nx_drm_display *display = nx_connector->display;
	struct nx_control_dev *control = dp_to_control(display);

	DRM_DEBUG_KMS("%s\n", nx_panel_get_name(display->panel_type));

	if (display->disable_output)
		return;

	nx_soc_dp_cont_irq_on(control->module, false);

	if (nx_connector->suspended)
		crtc_dpc_suspend(control);

	if (!control->cluster) {
		nx_soc_dp_cont_power_on(control, false);
		return;
	}

	list_for_each_entry(control, &dp_dpc_list, list)
		nx_soc_dp_cont_power_on(control, false);
}

static struct nx_drm_encoder_ops nx_encoder_ops = {
	.dpms_status = encoder_ops_dpms_status,
	.enable = encoder_ops_enable,
	.disable = encoder_ops_disable,
};

int nx_drm_encoder_init(struct drm_encoder *encoder)
{
	struct nx_drm_encoder *nx_encoder = to_nx_encoder(encoder);

	nx_encoder->ops = &nx_encoder_ops;
	return 0;
}

static int display_resource_parse(struct device *dev,
			struct nx_control_dev *control,
			enum nx_panel_type panel_type)
{
	struct device_node *np;
	struct device_node *node = dev->of_node;
	struct nx_control_res *res;
	const __be32 *list;
	bool reset;
	const char *strings[8];
	u32 addr, length;
	int size, i, err;

	res = &control->res;

	/* set base resources */
	res->base = of_iomap(node, 0);
	if (!res->base) {
		DRM_ERROR("Failed, %s of_iomap\n", dev_name(dev));
		return -EINVAL;
	}
	DRM_DEBUG_KMS("top base  : 0x%lx\n", (unsigned long)res->base);

	err = of_property_read_string(node, "reset-names", &strings[0]);
	if (!err) {
		res->resets[0] = devm_reset_control_get(dev, strings[0]);
		if (res->resets[0]) {
			bool stat = reset_control_status(res->resets[0]);

			if (stat)
				reset_control_reset(res->resets[0]);
		}
		DRM_DEBUG_KMS("top reset : %s\n", strings[0]);
	}
	nx_soc_dp_cont_top_base(0, res->base);

	/* set sub device resources */
	np = of_find_node_by_name(node, "dp-resource");
	of_node_get(node);
	if (!np)
		return 0;

	of_node_put(np);

	/* register base */
	list = of_get_property(np, "reg_base", &size);
	size /= 8;
	if (size > MAX_RESOURCE_NUM) {
		DRM_ERROR("error: over devs dt size %d (max %d)\n",
			size, MAX_RESOURCE_NUM);
		return -EINVAL;
	}

	for (i = 0; size > i; i++) {
		addr = be32_to_cpu(*list++);
		length = PAGE_ALIGN(be32_to_cpu(*list++));

		res->sub_bases[i] = ioremap(addr, length);
		if (!res->sub_bases[i])
			return -EINVAL;

		DRM_DEBUG_KMS("dev base  :  0x%x (0x%x) %p\n",
			addr, size, res->sub_bases[i]);
	}
	res->num_sud_devs = size;

	/* clock gen base : 2 contents */
	list = of_get_property(np, "clk_base", &size);
	size /= 8;
	if (size > MAX_RESOURCE_NUM) {
		DRM_ERROR("error: over clks dt size %d (max %d)\n",
			size, MAX_RESOURCE_NUM);
		return -EINVAL;
	}

	for (i = 0; size > i; i++) {
		addr = be32_to_cpu(*list++);
		res->sub_clk_bases[i] = ioremap(addr, PAGE_SIZE);
		res->sub_clk_ids[i] = be32_to_cpu(*list++);
		DRM_DEBUG_KMS("dev clock : [%d] clk 0x%x, %d\n",
			i, addr, res->sub_clk_ids[i]);
	}
	res->num_sub_clks = size;

	/* tieoffs : 2 contents */
	list = of_get_property(np, "soc,tieoff", &size);
	size /= 8;
	if (size > MAX_RESOURCE_NUM) {
		DRM_ERROR("error: over tieoff dt size %d (max %d)\n",
			size, MAX_RESOURCE_NUM);
		return -EINVAL;
	}
	res->num_tieoffs = size;

	for (i = 0; size > i; i++) {
		res->tieoffs[i][0] = be32_to_cpu(*list++);
		res->tieoffs[i][1] = be32_to_cpu(*list++);
		DRM_DEBUG_KMS("dev tieoff: [%d] res->tieoffs <0x%x %d>\n",
			i, res->tieoffs[i][0], res->tieoffs[i][1]);
	}

	for (i = 0; size > i; i++)
		nx_tieoff_set(res->tieoffs[i][0], res->tieoffs[i][1]);

	/* resets */
	size = of_property_read_string_array(np,
			"reset-names", strings, MAX_RESOURCE_NUM);
	for (i = 0; size > i; i++) {
		res->sub_resets[i] = of_reset_control_get(np, strings[i]);
		DRM_DEBUG_KMS("dev reset : [%d] %s\n", i, strings[i]);
	}
	res->num_sub_resets = size;

	for (i = 0; size > i; i++) {
		reset = reset_control_status(res->sub_resets[i]);
		if (reset)
			reset_control_assert(res->sub_resets[i]);
	}

	for (i = 0; size > i; i++)
		reset_control_deassert(res->sub_resets[i]);

	/* set pclk */
	for (i = 0; res->num_sub_clks > i; i++) {
		nx_soc_dp_cont_top_clk_base(
				res->sub_clk_ids[i], res->sub_clk_bases[i]);
		nx_soc_dp_cont_top_clk_on(res->sub_clk_ids[i]);
	}
	return 0;
}

static void display_resource_free(struct device *dev,
			struct nx_control_dev *control)
{
	struct nx_control_res *res = &control->res;
	int i;

	for (i = 0; res->num_sud_devs > i; i++) {
		if (res->sub_bases[i])
			iounmap(res->sub_bases[i]);
	}

	for (i = 0; res->num_sub_clks > i; i++) {
		if (res->sub_clk_bases[i])
			iounmap(res->sub_clk_bases[i]);
	}

	if (res->base)
		iounmap(res->base);

	for (i = 0; res->num_sub_resets > i; i++)
		reset_control_put(res->sub_resets[i]);

	for (i = 0; res->num_resets > i; i++)
		reset_control_put(res->resets[i]);
}

static void display_param_dump(struct nx_drm_display *display)
{
	struct nx_control_dev *control = dp_to_control(display);
	struct nx_control_info *ctrl = &control->ctrl;

	DRM_DEBUG_KMS("%s:\n", nx_panel_get_name(display->panel_type));
	DRM_DEBUG_KMS("SYNC -> LCD %d x %d mm\n",
		display->width_mm, display->height_mm);
	DRM_DEBUG_KMS("ha:%d, hs:%d, hb:%d, hf:%d\n",
	    display->vm.hactive, display->vm.hsync_len,
	    display->vm.hback_porch, display->vm.hfront_porch);
	DRM_DEBUG_KMS("va:%d, vs:%d, vb:%d, vf:%d\n",
		display->vm.vactive, display->vm.vsync_len,
	    display->vm.vback_porch, display->vm.vfront_porch);
	DRM_DEBUG_KMS("flags:0x%x\n", display->vm.flags);
	DRM_DEBUG_KMS("cs0:%d, cd0:%d, cs1:%d, cd1:%d\n",
	    ctrl->clk_src_lv0, ctrl->clk_div_lv0,
	    ctrl->clk_src_lv1, ctrl->clk_div_lv1);
	DRM_DEBUG_KMS("fmt:0x%x, inv:%d, swap:%d, yb:0x%x\n",
	    ctrl->out_format, ctrl->invert_field,
	    ctrl->swap_rb, ctrl->yc_order);
	DRM_DEBUG_KMS("dm:0x%x, drp:%d, dhs:%d, dvs:%d, dde:0x%x\n",
	    ctrl->delay_mask, ctrl->d_rgb_pvd,
	    ctrl->d_hsync_cp1, ctrl->d_vsync_fram, ctrl->d_de_cp2);
	DRM_DEBUG_KMS("vss:%d, vse:%d, evs:%d, eve:%d\n",
	    ctrl->vs_start_offset, ctrl->vs_end_offset,
	    ctrl->ev_start_offset, ctrl->ev_end_offset);
	DRM_DEBUG_KMS("sel:%d, i0:%d, d0:%d, i1:%d, d1:%d, s1:%d\n",
	    ctrl->vck_select, ctrl->clk_inv_lv0, ctrl->clk_delay_lv0,
	    ctrl->clk_inv_lv1, ctrl->clk_delay_lv1, ctrl->clk_sel_div1);
}

static void display_irq_enable(struct nx_drm_display *display,
			unsigned int pipe)
{
	nx_soc_dp_cont_irq_on((int)pipe, true);
}

static void display_irq_disable(struct nx_drm_display *display,
			unsigned int pipe)
{
	nx_soc_dp_cont_irq_on((int)pipe, false);
}

static void display_irq_done(struct nx_drm_display *display,
			unsigned int pipe)
{
	nx_soc_dp_cont_irq_done((int)pipe);
}

struct nx_drm_display *nx_drm_display_get(struct device *dev,
			struct device_node *node,
			struct drm_connector *connector,
			enum nx_panel_type type)
{
	struct nx_drm_display *display;
	struct nx_control_dev *control = NULL;
	int err = -EINVAL;

	display = kzalloc(sizeof(*display), GFP_KERNEL);
	if (!display)
		return NULL;

	switch (type) {
	#ifdef CONFIG_DRM_NX_RGB
	case NX_PANEL_TYPE_RGB:
		control = nx_drm_display_rgb_get(dev, node, display);
		break;
	#endif
	#ifdef CONFIG_DRM_NX_LVDS
	case NX_PANEL_TYPE_LVDS:
		control = nx_drm_display_lvds_get(dev, node, display);
		break;
	#endif
	#ifdef CONFIG_DRM_NX_MIPI_DSI
	case NX_PANEL_TYPE_MIPI:
		control = nx_drm_display_mipi_get(dev, node, display);
		break;
	#endif
	#ifdef CONFIG_DRM_NX_HDMI
	case NX_PANEL_TYPE_HDMI:
		control = nx_drm_display_hdmi_get(dev, node, display);
		break;
	#endif
	#ifdef CONFIG_DRM_NX_TVOUT
	case NX_PANEL_TYPE_TV:
		control = nx_drm_display_tvout_get(dev, node, display);
		break;
	#endif
	default:
		DRM_ERROR("not support panel type [%d] !!!\n", type);
		break;
	}

	if (!control) {
		DRM_ERROR("Failed [%s] display !!!\n", nx_panel_get_name(type));
		kfree(display);
		return NULL;
	}

	BUG_ON(!display->context);
	err = display_resource_parse(dev, control, type);
	if (err < 0)
		return NULL;

	INIT_LIST_HEAD(&control->list);
	list_add_tail(&control->list, &dp_dpc_list);
	control->panel_type = type;

	display->panel_type = type;
	display->irq = INVALID_IRQ;
	display->connector = to_nx_connector(connector);

	display->ops->irq_enable = display_irq_enable;
	display->ops->irq_disable = display_irq_disable;
	display->ops->irq_done = display_irq_done;

	DRM_DEBUG_KMS("get:%s [%d]\n", nx_panel_get_name(type), type);

	return display;
}

void nx_drm_display_put(struct device *dev, struct nx_drm_display *display)
{
	struct nx_control_dev *control = dp_to_control(display);

	display_resource_free(dev, control);
	kfree(display->context);
	kfree(display);
}

int nx_drm_display_setup(struct nx_drm_display *display,
			struct device_node *node, enum nx_panel_type type)
{
	struct clk *clk;
	struct nx_control_dev *control;
	struct nx_control_info *ctrl;
	char pll[256];

	control = dp_to_control(display);
	ctrl = &control->ctrl;

	DRM_DEBUG_KMS("display :%s, %p\n",
		nx_panel_get_name(nx_panel_get_type(display)), display);

	property_read(node, "clk_src_lv0", ctrl->clk_src_lv0);
	property_read(node, "clk_div_lv0", ctrl->clk_div_lv0);
	property_read(node, "clk_src_lv1", ctrl->clk_src_lv1);
	property_read(node, "clk_div_lv1", ctrl->clk_div_lv1);
	property_read(node, "out_format", ctrl->out_format);
	property_read(node, "invert_field", ctrl->invert_field);
	property_read(node, "swap_rb", ctrl->swap_rb);
	property_read(node, "yc_order", ctrl->yc_order);
	property_read(node, "delay_mask", ctrl->delay_mask);
	property_read(node, "d_rgb_pvd", ctrl->d_rgb_pvd);
	property_read(node, "d_hsync_cp1", ctrl->d_hsync_cp1);
	property_read(node, "d_vsync_fram", ctrl->d_vsync_fram);
	property_read(node, "d_de_cp2", ctrl->d_de_cp2);
	property_read(node, "vs_start_offset", ctrl->vs_start_offset);
	property_read(node, "vs_end_offset", ctrl->vs_end_offset);
	property_read(node, "ev_start_offset", ctrl->ev_start_offset);
	property_read(node, "ev_end_offset", ctrl->ev_end_offset);
	property_read(node, "vck_select", ctrl->vck_select);
	property_read(node, "clk_inv_lv0", ctrl->clk_inv_lv0);
	property_read(node, "clk_delay_lv0", ctrl->clk_delay_lv0);
	property_read(node, "clk_inv_lv1", ctrl->clk_inv_lv1);
	property_read(node, "clk_delay_lv1", ctrl->clk_delay_lv1);
	property_read(node, "clk_sel_div1", ctrl->clk_sel_div1);

	sprintf(pll, "sys-pll%d", ctrl->clk_src_lv0);

	clk = clk_get(NULL, pll);
	if (clk) {
		long rate, pixclock;

		rate = clk_get_rate(clk);
		pixclock = (rate / ctrl->clk_div_lv0) / ctrl->clk_div_lv1;
		display->vm.pixelclock = pixclock;
		clk_put(clk);

		DRM_DEBUG_KMS("SYNC -> PLL.%d, pixelclock %ld (%d,%d)\n",
			ctrl->clk_src_lv0, pixclock,
			ctrl->clk_div_lv0, ctrl->clk_div_lv1);
	}

	display_param_dump(display);

	return 0;
}

void nx_display_mode_to_sync(struct drm_display_mode *mode,
			struct nx_drm_display *display)
{
	struct videomode vm;
	struct nx_control_dev *control = dp_to_control(display);
	struct nx_sync_info *sync = &control->sync;

	drm_display_mode_to_videomode(mode, &vm);

	sync->interlace =
		vm.flags & DISPLAY_FLAGS_INTERLACED ? 1 : 0;

	sync->h_active_len = vm.hactive;
	sync->h_sync_width = vm.hsync_len;
	sync->h_back_porch = vm.hback_porch;
	sync->h_front_porch = vm.hfront_porch;
	sync->h_sync_invert =
		vm.flags & DISPLAY_FLAGS_HSYNC_HIGH ? 1 : 0;

	sync->v_active_len = vm.vactive;
	sync->v_sync_width = vm.vsync_len;
	sync->v_back_porch = vm.vback_porch;
	sync->v_front_porch = vm.vfront_porch;
	sync->v_sync_invert =
		vm.flags & DISPLAY_FLAGS_VSYNC_HIGH ? 1 : 0;
	sync->pixel_clock_hz = vm.pixelclock;

	/* refresh */
	display->vrefresh = mode->vrefresh;
}

void nx_display_resume_resource(struct nx_drm_display *display)
{
	struct nx_control_dev *control = dp_to_control(display);
	struct nx_control_res *res = &control->res;
	enum nx_panel_type panel_type = control->panel_type;
	bool stat;
	int i;

	DRM_DEBUG_KMS("%s\n", nx_panel_get_name(panel_type));

	if (res->resets[0]) {
		stat = reset_control_status(res->resets[0]);
		if (stat)
			reset_control_reset(res->resets[0]);
	}

	for (i = 0; res->num_tieoffs > i; i += 2)
		nx_tieoff_set(res->tieoffs[i][0], res->tieoffs[i][1]);

	for (i = 0; res->num_sub_resets > i; i++) {
		stat = reset_control_status(res->sub_resets[i]);
		if (stat)
			reset_control_assert(res->sub_resets[i]);
	}

	for (i = 0; res->num_sub_resets > i; i++)
		reset_control_deassert(res->sub_resets[i]);

	for (i = 0; res->num_sub_clks > i; i++)
		nx_soc_dp_cont_top_clk_on(res->sub_clk_ids[i]);
}
