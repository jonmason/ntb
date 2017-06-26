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
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/dma-buf.h>

#include "../nx_drm_drv.h"
#include "../nx_drm_fb.h"
#include "../nx_drm_gem.h"

#include "nxp5xxx_drv.h"

#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_res_manager.h>

static const uint32_t support_fourcc_rgb[] = {
	DRM_FORMAT_RGB565,
	/*
	 * DRM_FORMAT_BGR565,
	 */
	DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
};

static const uint32_t support_fourcc_video[] = {
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
		.formats = support_fourcc_rgb,
		.format_count = ARRAY_SIZE(support_fourcc_rgb),
	},
	{
		.formats = support_fourcc_video,
		.format_count = ARRAY_SIZE(support_fourcc_video),
	},
};

struct of_plane_type {
	const char *name;
	u32 type;
	struct property *property;
};

static const struct of_plane_type nx_plane_of_types[] = {
	{ "plane-primary", DRM_PLANE_TYPE_PRIMARY | NX_PLANE_TYPE_RGB },
	{ "plane-cursor", DRM_PLANE_TYPE_CURSOR | NX_PLANE_TYPE_RGB },
	{ "plane-rgb", DRM_PLANE_TYPE_OVERLAY | NX_PLANE_TYPE_RGB },
	{ "plane-video", DRM_PLANE_TYPE_OVERLAY | NX_PLANE_TYPE_VIDEO },
};

#define	__display_control(d)	\
		((struct nx_control_dev *)d->context)

static void crtc_ops_reset(struct drm_crtc *crtc)
{
}

static void crtc_ops_destory(struct drm_crtc *crtc)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);

	kfree(nx_crtc->context);
}

static int crtc_ops_begin(struct drm_crtc *crtc)
{
	struct drm_framebuffer *fb = crtc->primary->state->fb;
	struct nx_crtc_frame *frame = to_nx_crtc(crtc)->context;

	frame->width = crtc->state->mode.hdisplay;
	frame->height = crtc->state->mode.vdisplay;

	DRM_DEBUG_KMS("crtc.%d: [%d x %d] -> [%d,%d]\n",
		frame->pipe, fb->width, fb->height,
		frame->width, frame->height);
	DRM_DEBUG_KMS("%s pixel:%d, back:0x%x, colorkey:0x%x\n",
		drm_get_format_name(fb->pixel_format),
		fb->bits_per_pixel >> 3, frame->back_color, frame->color_key);

	/* no blender and no bottom element */
	if (!(frame->s_mask & NXS_MASK_BLENDER))
		return 0;

	return 0;
}

static void crtc_ops_enable(struct drm_crtc *crtc)
{
	struct nx_crtc_frame *frame = to_nx_crtc(crtc)->context;

	/* no blender and no bottom element */
	if (!(frame->s_mask & NXS_MASK_BLENDER))
		return;
}

static void crtc_ops_disable(struct drm_crtc *crtc)
{
	struct nx_crtc_frame *frame = to_nx_crtc(crtc)->context;

	/* no blender and no bottom element */
	if (!(frame->s_mask & NXS_MASK_BLENDER))
		return;
}

struct nx_drm_crtc_ops nx_crtc_ops = {
	.reset = crtc_ops_reset,
	.destroy = crtc_ops_destory,
	.begin = crtc_ops_begin,
	.enable = crtc_ops_enable,
	.disable = crtc_ops_disable,
};

static int crtc_get_of_props(struct drm_device *drm,
			struct nx_drm_crtc *nx_crtc,
			struct nx_crtc_frame *frame)
{
	struct device_node *node;
	struct device *dev = &drm->platformdev->dev;
	int i, counts = 0;
	struct property *pp;

	/* port node */
	node = of_graph_get_port_by_id(dev->of_node, nx_crtc->pipe);
	if (!node)
		return -ENOENT;
	of_node_put(node);

	/* get planes num */
	for_each_property_of_node(node, pp) {
		for (i = 0; ARRAY_SIZE(nx_plane_of_types) > i ; i++) {
			if (!strncasecmp(pp->name, nx_plane_of_types[i].name,
				strlen(nx_plane_of_types[i].name))) {
				counts++;
				continue;
			}
		}
	}

	nx_crtc->num_planes = counts;
	property_read(node, "back_color", frame->back_color);
	property_read(node, "color_key", frame->color_key);

	DRM_DEBUG_KMS("crtc.%d for %s planes %dEA\n",
		nx_crtc->pipe, dev_name(dev), counts);

	return counts > 0 ? 0 : -ENOENT;
}

int nx_drm_crtc_init(struct drm_device *drm,
			struct drm_crtc *crtc, int pipe)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct nx_crtc_frame *frame;
	int err;

	frame = kzalloc(sizeof(*frame), GFP_KERNEL);
	if (!frame)
		return -ENOMEM;

	frame->pipe = pipe;
	INIT_LIST_HEAD(&frame->plane_list);

	err = crtc_get_of_props(drm, nx_crtc, frame);
	if (err < 0) {
		kfree(frame);
		return err;
	}

	/* set nx_crtc */
	nx_crtc->context = frame;
	nx_crtc->ops = &nx_crtc_ops;

	return 0;
}

static void plane_elem_debug(struct nx_drm_crtc *nx_crtc)
{
	struct nx_crtc_frame *frame = nx_crtc->context;
	struct nx_plane_layer *layer;

	DRM_DEBUG_KMS("crtc.%d plane %dEA\n",
		nx_crtc->pipe, nx_crtc->num_planes);
	DRM_DEBUG_KMS("crtc colorkey:0x%x bgcolor:0x%x\n",
		frame->back_color, frame->color_key);

	list_for_each_entry(layer, &frame->plane_list, plane) {
		struct nxs_function *fn = layer->s_func;
		struct nxs_dev *nxs;

		DRM_DEBUG_KMS("%s func %d\n", layer->name, fn->id);
		list_for_each_entry(nxs, &fn->dev_list, func_list) {
			DRM_DEBUG_KMS("\t[dev:%2d, id:%2d, irq:%2d, tid:%3d]\n",
				nxs->dev_function, nxs->dev_inst_index,
				nxs->irq, nxs->tid);
		}
	}
}

static inline int plane_elem_check(struct nx_plane_layer *layer, u32 func)
{
	int ret = 0;

	switch (func) {
	case NXS_FUNCTION_MLC_BLENDER:
		layer->s_mask |= NXS_MASK_BLENDER;
		break;
	case NXS_FUNCTION_DMAR:
		layer->s_mask |= NXS_MASK_DMAR;
		break;
	case NXS_FUNCTION_FIFO:
		layer->s_mask |= NXS_MASK_FIFO;
		break;
	default:
		DRM_ERROR("Failed layer %s, not support %d stream\n",
			layer->name, func);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static void plane_stream_put(struct nx_plane_layer *layer)
{
	struct nx_crtc_frame *frame = layer->frame;

	list_del(&layer->plane);

	if (layer->s_func)
		nxs_function_destroy(layer->s_func);

	if (list_empty(&frame->plane_list))
		frame->top = NULL;
}

static int plane_stream_get(struct nx_crtc_frame *frame,
			struct nx_plane_layer *layer,
			struct of_plane_type *of_type)
{
	struct property *property = of_type->property;
	const __be32 *val = property->value;
	size_t sz = property->length/4;
	struct nxs_function_elem *fe = NULL;

	DRM_DEBUG_KMS("pipe.%d %s elements %d\n",
			frame->pipe, property->name, sz);

	/* mask elements */
	while (sz--) {
		u32 v = be32_to_cpup(val++);
		int ret = plane_elem_check(layer, v);

		if (ret < 0)
			return ret;
	}

	if (frame->top) {
		/* blender is all or noting */
		if ((frame->s_mask & NXS_MASK_BLENDER) ^
		(layer->s_mask & NXS_MASK_BLENDER)) {
			DRM_ERROR("Failed layer element %s, check blender!!!\n",
				layer->name);
			return -EINVAL;
		}

		/* without blender, not support multiple layer */
		if (!(frame->s_mask & NXS_MASK_BLENDER)) {
			DRM_ERROR("Failed layer element %s !!!\n", layer->name);
			DRM_ERROR("No blender, not support multiple layer !\n");
			return -EINVAL;
		}
	}

	val = property->value;
	sz = property->length/4;

	while (sz--) {
		u32 v = be32_to_cpup(val++);
		int err;

		fe = kzalloc(sizeof(*fe), GFP_KERNEL);
		if (!fe)
			return -ENOMEM;

		INIT_LIST_HEAD(&fe->list);
		fe->function = v;
		fe->index = NXS_FUNCTION_ANY;

		if (!layer->s_func) {
			layer->s_func = nxs_function_create(fe);
			if (!layer->s_func)
				goto err_sdev;
			continue;
		}

		/*
		 * skip the last fifo elements.
		 * connect the last fifo elements after display
		 */
		if (!sz && fe->function == NXS_FUNCTION_FIFO)
			continue;

		err = nxs_function_add(layer->s_func, fe);
		if (err < 0)
			goto err_sdev;
	}

	if (!frame->top) {
		frame->s_mask = layer->s_mask;
		frame->top = layer;
	}
	list_add_tail(&layer->plane, &frame->plane_list);

	return 0;

err_sdev:
	layer->s_mask = 0;
	kfree(fe);

	return -ENOENT;
}

static inline void plane_set_rect(struct nx_plane_rect *r,
			int cx, int cy, int cw, int ch, int aw, int ah)
{
	if (cw <= 0 || (cx + cw) <= 0)
		return;

	if (ch <= 0 || (cy + ch) <= 0)
		return;

	r->left = cx < 0 ? 0 : cx;
	r->right = (cx + cw) > aw ? aw : cx + cw;
	r->top = cy < 0 ? 0 : cy;
	r->bottom = (cy + ch) > ah ? ah : cy + ch;
}

static int plane_set_format(struct nx_plane_layer *layer,
			unsigned int format, int pixelbyte)
{
	DRM_DEBUG_KMS("%s, format:0x%x, pixel:%d\n",
		 layer->name, format, pixelbyte);

	if (layer->format == format &&
		layer->pixelbyte == pixelbyte)
		return 0;

	layer->format = format;
	layer->pixelbyte = pixelbyte;


	return 0;
}

static int plane_set_pos(struct nx_plane_layer *layer,
			int src_x, int src_y, int src_w, int src_h,
			int dst_x, int dst_y, int dst_w, int dst_h)
{
	struct nxs_dev *nxs;
	struct nxs_control s_cont;
	int sx, sy, ex, ey;
	int ret;

	if (layer->left == src_x && layer->top == src_y &&
		layer->width == src_w && layer->height == src_h &&
		layer->d_left == dst_x && layer->d_top == dst_y &&
		layer->d_width == dst_w && layer->d_height == dst_h)
		return 0;

	layer->left = src_x;
	layer->top = src_y;
	layer->width = src_w;
	layer->height = src_h;
	layer->d_left = dst_x;
	layer->d_top = dst_y;
	layer->d_width = dst_w;
	layer->d_height = dst_h;

	/*
	 * max scale size PLANE_MAX_LINE
	 * if ove scale size, fix max
	 */
	if (dst_w > PLANE_MAX_LINE)
		dst_w = PLANE_MAX_LINE;

	if (dst_h > PLANE_MAX_LINE)
		dst_h = PLANE_MAX_LINE;

	sx = dst_x, sy = dst_y;
	ex = dst_x + dst_w;
	ey = dst_y + dst_h;

	/* max rectangle 2048 */
	if (ex > PLANE_MAX_LINE)
		ex = PLANE_MAX_LINE;

	if (ey > PLANE_MAX_LINE)
		ey = PLANE_MAX_LINE;

	DRM_DEBUG_KMS("%s, (%d, %d, %d, %d) to (%d, %d, %d, %d, %d, %d)\n",
		 layer->name, src_x, src_y, src_w, src_h,
		 sx, sy, ex, ey, dst_w, dst_h);

	nxs = nxs_function_find_dev(layer->s_func,
				NXS_FUNCTION_DMAR, NXS_FUNCTION_ANY);
	if (WARN_ON(!nxs))
		return -EINVAL;

	s_cont.type = NXS_CONTROL_FORMAT;
	s_cont.u.format.width = (ex - 1) - sx;
	s_cont.u.format.height = (ey - 1) - sy;
	s_cont.u.format.pixelformat = layer->format;

	ret = nxs_set_control(nxs, s_cont.type,
			(const struct nxs_control *)&s_cont);
	if (ret < 0)
		return ret;

	/* ToDo: set position with blender  */

	return 0;
}

static void plane_set_addr(struct nx_plane_layer *layer,
			dma_addr_t addr, unsigned int pixelbyte,
			unsigned int stride, int align)
{
	struct nxs_dev *nxs;
	struct nxs_control s_cont;

	int cl = layer->left, ct = layer->top;
	unsigned int phys_addr = addr + (cl * pixelbyte) + (ct * stride);
	int ret;

	if (align)
		phys_addr = ALIGN(phys_addr, align);

	DRM_DEBUG_KMS("%s, pa=0x%x, hs=%d, vs=%d, l:%d, t:%d\n",
		layer->name, phys_addr, pixelbyte, stride, cl, ct);
	DRM_DEBUG_KMS("%s, pa=0x%x -> 0x%x aligned %d\n",
		layer->name, (addr + (cl * pixelbyte) + (ct * stride)),
		phys_addr, align);

	nxs = nxs_function_find_dev(layer->s_func,
				NXS_FUNCTION_DMAR, NXS_FUNCTION_ANY);
	if (WARN_ON(!nxs))
		return;

	s_cont.type = NXS_CONTROL_BUFFER;
	s_cont.u.buffer.num_planes = 1;
	s_cont.u.buffer.strides[0] = stride;
	s_cont.u.buffer.address[0] = phys_addr;

	ret = nxs_set_control(nxs, s_cont.type,
			(const struct nxs_control *)&s_cont);
	if (ret < 0)
		return;

}

static int plane_ops_update(struct drm_plane *plane,
			struct drm_framebuffer *fb,
			int crtc_x, int crtc_y, int crtc_w, int crtc_h,
			int src_x, int src_y, int src_w, int src_h)
{
	struct nx_plane_layer *layer = to_nx_plane(plane)->context;
	struct nx_crtc_frame *frame = layer->frame;

	struct nx_gem_object *nx_obj = nx_drm_fb_gem(fb, 0);
	dma_addr_t dma_addr = nx_obj->dma_addr;
	int align = to_nx_plane(plane)->align;
	int pitch = fb->pitches[0];
	int pixel = (fb->bits_per_pixel >> 3);
	unsigned int format = fb->pixel_format;

	struct nx_plane_rect r = { 0, };
	int act_w = frame->width, act_h = frame->height;

	DRM_DEBUG_KMS("crtc.%d %s function %d\n",
		layer->pipe, layer->name, layer->s_func->id);
	DRM_DEBUG_KMS("%s crtc[%d,%d,%d,%d] src[%d,%d,%d,%d]\n",
		drm_get_format_name(fb->pixel_format),
		crtc_x, crtc_y, crtc_w, crtc_h, src_x, src_y, src_w, src_h);

	if (layer->drm_type & NX_PLANE_TYPE_VIDEO) {
		DRM_INFO("crtc.%d %s current not support !!!\n",
			layer->pipe, layer->name);
		return 0;
	}

	plane_set_rect(&r, crtc_x, crtc_y, crtc_w, crtc_h, act_w, act_h);
	plane_set_format(layer, format, pixel);
	plane_set_pos(layer, src_x, src_y, src_w, src_h,
			r.left, r.top, r.right - r.left, r.bottom - r.top);
	plane_set_addr(layer, dma_addr, pixel, pitch, align);

	return 0;
}

static void plane_ops_disable(struct drm_plane *plane)
{
	struct nx_plane_layer *layer = to_nx_plane(plane)->context;
	struct nxs_dev *nxs;

	DRM_DEBUG_KMS("crtc.%d %s function %d\n",
		layer->pipe, layer->name, layer->s_func->id);

	nxs = nxs_function_find_dev(layer->s_func,
				NXS_FUNCTION_DMAR, NXS_FUNCTION_ANY);
	if (!nxs)
		return;

}

static int plane_set_property(struct drm_plane *plane,
			struct drm_plane_state *state,
			struct drm_property *property, uint64_t val)
{
	return 0;
}

static int plane_get_property(struct drm_plane *plane,
			const struct drm_plane_state *state,
			struct drm_property *property, uint64_t *val)
{
	return 0;
}

static void plane_ops_create_proeprties(struct drm_device *drm,
			struct drm_crtc *crtc, struct drm_plane *plane,
			struct drm_plane_funcs *plane_funcs)
{
	/* plane property functions */
	plane_funcs->atomic_set_property = plane_set_property;
	plane_funcs->atomic_get_property = plane_get_property;
}

static void plane_ops_destory(struct drm_plane *plane)
{
	struct nx_drm_plane *nx_plane = to_nx_plane(plane);

	plane_stream_put(nx_plane->context);
	kfree(nx_plane->context);
}

struct nx_drm_plane_ops nx_plane_ops = {
	.update = plane_ops_update,
	.disable = plane_ops_disable,
	.create_proeprties = plane_ops_create_proeprties,
	.destroy = plane_ops_destory,
};

static struct of_plane_type *plane_get_of_types(
			struct device_node *node, int count)
{
	struct of_plane_type *of_types;
	struct property *pp;
	int i, n = 0;

	of_types = kcalloc(count, sizeof(*of_types), GFP_KERNEL);
	if (!of_types)
		return NULL;

	for_each_property_of_node(node, pp) {
		for (i = 0; ARRAY_SIZE(nx_plane_of_types) > i ; i++) {
			if (!strncasecmp(pp->name, nx_plane_of_types[i].name,
				strlen(nx_plane_of_types[i].name))) {
				memcpy(&of_types[n], &nx_plane_of_types[i],
					sizeof(struct of_plane_type));
				of_types[n++].property = pp;
				if (n == count)
					return of_types;
				continue;
			}
		}
	}

	kfree(of_types);
	return NULL;
}

/* add FIFO element */
static int plane_complete(struct nx_crtc_frame *frame)
{
	struct nx_plane_layer *layer = frame->top;
	struct nxs_function_elem *fe = NULL;
	int err;

	if (WARN_ON(!layer))
		return -EINVAL;

	fe = kzalloc(sizeof(*fe), GFP_KERNEL);
	if (!fe)
		return -ENOMEM;

	INIT_LIST_HEAD(&fe->list);
	fe->function = NXS_FUNCTION_FIFO;
	fe->index = NXS_FUNCTION_ANY;

	/* add fifo to top layer */
	err = nxs_function_add(layer->s_func, fe);
	if (err) {
		kfree(fe);
		return err;
	}

	return 0;
}

static int plane_create(struct drm_crtc *crtc,
			struct nx_drm_plane *nx_plane,
			int index, struct of_plane_type *of_type,
			bool bgr)
{
	struct nx_crtc_frame *frame;
	struct nx_plane_layer *layer;
	struct plane_format_type *format;
	bool is_video = of_type->type & NX_PLANE_TYPE_VIDEO ? true : false;
	int ret;

	layer = kzalloc(sizeof(*layer), GFP_KERNEL);
	if (!layer)
		return -ENOMEM;

	frame = to_nx_crtc(crtc)->context;
	layer->frame = frame;
	layer->index = index;
	layer->drm_type = of_type->type;
	layer->is_bgr = bgr;
	layer->color.alpha = is_video ? 15 : 0;
	layer->pipe = frame->pipe;

	sprintf(layer->name, "%d-%s%d", frame->pipe,
		of_type->type & NX_PLANE_TYPE_VIDEO ?
		"vid" : "rgb", index);

	ret = plane_stream_get(frame, layer, of_type);
	if (ret) {
		kfree(layer);
		return ret;
	}

	format = &plane_formats[is_video ? 1 : 0];
	nx_plane->context = layer;
	nx_plane->ops = &nx_plane_ops;
	nx_plane->formats = format->formats;
	nx_plane->format_count = format->format_count;

	DRM_DEBUG_KMS("crtc.%d %s done\n", layer->pipe, layer->name);

	return 0;
}

int nx_drm_planes_init(struct drm_device *drm, struct drm_crtc *crtc,
			struct drm_plane **planes, int num_planes,
			enum drm_plane_type *drm_types, bool bgr)
{
	struct device_node *node;
	struct device *dev = &drm->platformdev->dev;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct of_plane_type *of_types;
	int count = 0;
	int i = 0, ret = -EINVAL;

	DRM_DEBUG_KMS("planes %d\n", num_planes);

	node = of_graph_get_port_by_id(dev->of_node, to_nx_crtc(crtc)->pipe);
	if (!node)
		return -ENOENT;
	of_node_put(node);

	of_types = plane_get_of_types(node, num_planes);
	if (!of_types)
		return -ENOENT;

	for (i = 0; num_planes > i; i++) {
		struct nx_drm_plane *nx_plane = to_nx_plane(planes[i]);
		enum drm_plane_type drm_type;

		drm_types[i] = of_types[i].type;
		drm_type = drm_types[i] & 0xff;

		if (drm_type == NX_PLANE_TYPE_UNKNOWN)
			continue;

		ret = plane_create(crtc, nx_plane, count, &of_types[i], bgr);
		if (ret == -ENOENT)
			continue;

		if (ret < 0)
			goto err_plane;

		count++;
	}

	ret = plane_complete(nx_crtc->context);
	if (ret < 0)
		goto err_plane;

	/* reset crtc's plane num value */
	nx_crtc->num_planes = count;
	plane_elem_debug(nx_crtc);

err_plane:
	kfree(of_types);

	return count == 0 ? -EINVAL : count;
}

dma_addr_t nx_drm_framebuffer_get_dma_addr(struct drm_plane *plane)
{
	return 0;
}

static int encoder_ops_dpms_status(struct drm_encoder *encoder)
{
	return DRM_MODE_DPMS_OFF;
}

static void encoder_ops_enable(struct drm_encoder *encoder)
{
}

static void encoder_ops_disable(struct drm_encoder *encoder)
{
}

struct nx_drm_encoder_ops nx_encoder_ops = {
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

void nx_stream_display_free(struct nx_drm_display *display)
{
	struct nx_drm_connector *connector = display->connector;
	struct nx_drm_crtc *nx_crtc =
			to_nx_crtc(connector->encoder->encoder.crtc);
	struct nx_crtc_frame *frame = nx_crtc->context;
	struct nx_plane_layer *layer = frame->top;
	struct nx_control_dev *control = display->context;
	int id = control->s_display;

	DRM_DEBUG_KMS("destroy display for crtc.%d(%d)\n", nx_crtc->pipe, id);

	if (!layer || !layer->s_func)
		return;

	if (!control->connected)
		return;

	/* remove functions */
	list_for_each_entry(layer, &frame->plane_list, plane) {
		if (layer == frame->top)
			continue;
		nxs_function_remove_from_display(id, layer->s_func);
	}

	/* destroy display */
	nxs_function_destroy_display(layer->s_func);

	control->connected = false;
	control->s_display = -1;
	control->pipe = -1;
}

int nx_stream_display_create(struct nx_drm_display *display)
{
	struct nx_drm_connector *connector = display->connector;
	struct nx_drm_crtc *nx_crtc =
			to_nx_crtc(connector->encoder->encoder.crtc);
	struct nx_crtc_frame *frame = nx_crtc->context;
	struct nx_plane_layer *layer = frame->top;
	struct nx_control_dev *control = display->context;
	struct nxs_dev *nxs = control->s_dev;
	bool blending;
	int id, ret;

	DRM_DEBUG_KMS("create display for crtc.%d\n", nx_crtc->pipe);

	if (WARN_ON(!layer || !layer->s_func))
		return -EINVAL;

	if (control->connected) {
		if (control->pipe == nx_crtc->pipe)
			return 0;

		nx_stream_display_free(display);
	}

	/* create display and add hdmi */
	blending = layer->s_mask & NXS_MASK_BLENDER ? true : false;
	id = nxs_function_create_display(layer->s_func, nxs, blending);
	if (id)
		return -EINVAL;

	/* add functions to display */
	list_for_each_entry(layer, &frame->plane_list, plane) {
		if (layer == frame->top)
			continue;
		ret = nxs_function_add_to_display(id, layer->s_func);
		if (ret)
			return ret;
	}

	control->pipe = nx_crtc->pipe;
	control->s_display = id;
	control->connected = true;

	return 0;
}

void nx_stream_display_update(struct nx_drm_display *display)
{
	struct drm_crtc *crtc;
	struct drm_plane *primary;
	struct drm_encoder *encoder;
	struct nx_drm_connector *connector;
	struct drm_pending_vblank_event *event;

	if (WARN_ON(!display))
		return;

	connector = display->connector;
	encoder = &connector->encoder->encoder;
	if (!encoder || !encoder->crtc)
		return;

	crtc = encoder->crtc;
	primary = crtc->primary;

	/*
	 * update crtc event
	 */
	drm_crtc_handle_vblank(crtc);

	spin_lock(&crtc->dev->event_lock);

	event = to_nx_crtc(crtc)->event;
	if (event) {
		drm_crtc_send_vblank_event(crtc, event);
		drm_crtc_vblank_put(crtc);
		to_nx_crtc(crtc)->event = NULL;
	}

	spin_unlock(&crtc->dev->event_lock);
}

struct nx_drm_display *nx_drm_display_get(struct device *dev,
			struct device_node *node,
			struct drm_connector *connector,
			enum nx_panel_type type)
{
	struct nx_drm_display *display;
	struct nx_control_dev *control = NULL;

	display = kzalloc(sizeof(*display), GFP_KERNEL);
	if (!display)
		return NULL;

	switch (type) {
	#ifdef CONFIG_DRM_NX_HDMI
	case NX_PANEL_TYPE_HDMI:
		control = nx_drm_display_hdmi_get(dev, node, display);
		break;
	#endif
	default:
		DRM_ERROR("not support display type [%d] !!!\n", type);
		break;
	}

	if (!control) {
		DRM_ERROR("Failed [%s] display !!!\n", nx_panel_get_name(type));
		kfree(display);
		return NULL;
	}

	control->panel_type = type;
	display->panel_type = type;
	display->connector = to_nx_connector(connector);

	DRM_DEBUG_KMS("get device :%s, %p [%d]\n",
		nx_panel_get_name(type), display, type);

	return display;
}

void nx_drm_display_put(struct device *dev, struct nx_drm_display *display)
{
	kfree(display->context);
	kfree(display);
}

int nx_drm_display_setup(struct nx_drm_display *display,
			struct device_node *node, enum nx_panel_type type)
{
	return 0;
}
