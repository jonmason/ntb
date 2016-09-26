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
#include <drm/drm_crtc_helper.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/of_address.h>

#include "nx_drm_drv.h"
#include "nx_drm_crtc.h"
#include "nx_drm_plane.h"
#include "nx_drm_gem.h"
#include "nx_drm_fb.h"
#include "soc/s5pxx18_drm_dp.h"

/*
 * for multiple framebuffers.
 */
static int  fb_align_rgb = 1;
static bool fb_vblank_wait;
MODULE_PARM_DESC(fb_align, "frame buffer's align (0~4096)");
MODULE_PARM_DESC(fb_vblank, "frame buffer wait vblank for pan display");

module_param_named(fb_align, fb_align_rgb, int, 0600);
module_param_named(fb_vblank, fb_vblank_wait, bool, 0600);

static void nx_drm_crtc_dpms(struct drm_crtc *crtc, int mode)
{
	struct drm_device *drm = crtc->dev;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);

	DRM_DEBUG_KMS("enter [CRTC:%d] dpms:%d\n", crtc->base.id, mode);

	if (nx_crtc->dpms_mode == mode) {
		DRM_DEBUG_KMS("dpms %d same as previous one.\n", mode);
		return;
	}

	mutex_lock(&drm->struct_mutex);

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		nx_crtc->dpms_mode = mode;
		break;
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		nx_crtc->dpms_mode = mode;
		break;
	default:
		DRM_ERROR("fail : unspecified mode %d\n", mode);
		goto err_dpms;
	}

	nx_drm_dp_crtc_dpms(crtc, mode);

err_dpms:
	mutex_unlock(&drm->struct_mutex);
}

static void nx_drm_crtc_prepare(struct drm_crtc *crtc)
{
	DRM_DEBUG_KMS("enter\n");
}

static void nx_drm_crtc_commit(struct drm_crtc *crtc)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);

	DRM_DEBUG_KMS("enter current [CRTC:%d] dpms:%d\n",
		crtc->base.id, nx_crtc->dpms_mode);

	/*
	 * when set_crtc is requested from user or at booting time,
	 * crtc->commit would be called without dpms call so if dpms is
	 * no power on then crtc->dpms should be called
	 * with DRM_MODE_DPMS_ON for the hardware power to be on.
	 */
	if (nx_crtc->dpms_mode != DRM_MODE_DPMS_ON)
		nx_drm_crtc_dpms(crtc, DRM_MODE_DPMS_ON);

	nx_drm_dp_crtc_commit(crtc);
}

static bool nx_drm_crtc_mode_fixup(struct drm_crtc *crtc,
			const struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode)
{
	return true;
}

static int nx_drm_crtc_mode_set(struct drm_crtc *crtc,
			struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode,
			int x, int y,
			struct drm_framebuffer *old_fb)
{
	struct drm_framebuffer *fb = crtc->primary->fb;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct drm_plane *plane = crtc->primary;
	struct videomode vm;
	unsigned int crtc_w, crtc_h, src_w, src_h;
	int ret = 0;

	DRM_DEBUG_KMS("enter\n");

	drm_display_mode_to_videomode(mode, &vm);
	drm_mode_copy(&nx_crtc->current_mode, mode);

	/*
	 * copy the mode data adjusted by mode_fixup() into crtc->mode
	 * so that hardware can be seet to proper mode.
	 */
	memcpy(&crtc->mode, adjusted_mode, sizeof(*adjusted_mode));

	crtc_w = vm.hactive;
	crtc_h = vm.vactive;
	src_w = fb->width - x;
	src_h = fb->height - y;

	ret = nx_drm_dp_plane_mode_set(crtc,
				crtc->primary, fb,
				0, 0, crtc_w, crtc_h, x, y, src_w, src_h);
	if (0 > ret)
		return ret;

	plane->crtc = crtc;
	to_nx_plane(plane)->enabled = true;

	return ret;
}

static int nx_drm_crtc_mode_set_base(struct drm_crtc *crtc, int x, int y,
			struct drm_framebuffer *old_fb)
{
	struct drm_device *drm = crtc->dev;
	struct drm_framebuffer *fb = crtc->primary->fb;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct nx_drm_priv *priv = drm->dev_private;
	struct nx_drm_fbdev *fbdev = priv->framebuffer_dev->fbdev;
	struct videomode vm;
	unsigned int crtc_w, crtc_h, src_w, src_h;
	int align = fb_align_rgb;
	bool doublefb = fbdev->fb_buffers > 1 ? true : false;
	bool vblank = false;
	int ret;

	drm_display_mode_to_videomode(&nx_crtc->current_mode, &vm);

	/* when framebuffer changing is requested, crtc's dpms should be on */
	if (nx_crtc->dpms_mode > DRM_MODE_DPMS_ON) {
		DRM_ERROR("fail : framebuffer changing request.\n");
		return -EPERM;
	}

	crtc_w = fb->width;  /* vm.hactive; */
	crtc_h = fb->height; /* vm.vactive; */
	src_w = fb->width - x;
	src_h = fb->height - y;

	DRM_DEBUG_KMS("crtc.%d [%d:%d] pos[%d:%d] src[%d:%d] fb[%d:%d]\n",
		nx_crtc->pipe, crtc_w, crtc_h, x, y, src_w, src_h,
		fb->width, fb->height);

	/* for multiple buffers */
	if (doublefb) {
		if (y >= fb->height)
			src_h = fb->height;

		if (fb_vblank_wait &&
			drm->driver->enable_vblank) {
			drm->driver->enable_vblank(drm, nx_crtc->pipe);
			vblank = true;
		}
	}

	ret = nx_drm_dp_plane_update(crtc->primary, fb, 0, 0,
				   crtc_w, crtc_h, x, y, src_w, src_h, align);

	if (!ret && vblank)
		drm_wait_one_vblank(drm, nx_crtc->pipe);

	return ret;
}

static void nx_drm_crtc_disable(struct drm_crtc *crtc)
{
	struct drm_plane *plane;
	int ret;

	DRM_DEBUG_KMS("enter\n");

	nx_drm_crtc_dpms(crtc, DRM_MODE_DPMS_OFF);

	drm_for_each_plane(plane, crtc->dev) {
		if (plane->crtc != crtc)
			continue;
		ret = plane->funcs->disable_plane(plane);
		if (ret)
			DRM_ERROR("fail : disable plane %d\n", ret);
	}
}

static struct drm_crtc_helper_funcs nx_crtc_helper_funcs = {
	.dpms = nx_drm_crtc_dpms,
	.prepare = nx_drm_crtc_prepare,
	.commit = nx_drm_crtc_commit,
	.mode_fixup = nx_drm_crtc_mode_fixup,
	.mode_set = nx_drm_crtc_mode_set,
	.mode_set_base = nx_drm_crtc_mode_set_base,
	.disable = nx_drm_crtc_disable,
};

static int nx_drm_crtc_page_flip(struct drm_crtc *crtc,
			struct drm_framebuffer *fb,
			struct drm_pending_vblank_event *event,
			uint32_t flags)
{
	struct drm_device *drm = crtc->dev;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct drm_framebuffer *old_fb = crtc->primary->fb;
	unsigned int crtc_w, crtc_h;
	int pipe = nx_crtc->pipe;
	int ret;

	DRM_DEBUG_KMS("page flip crtc.%d\n", nx_crtc->pipe);

	/* when the page flip is requested, crtc's dpms should be on */
	if (nx_crtc->dpms_mode > DRM_MODE_DPMS_ON) {
		DRM_ERROR("fail : page flip request.\n");
		return -EINVAL;
	}

	if (!event)
		return -EINVAL;

	spin_lock_irq(&drm->event_lock);

	if (nx_crtc->event) {
		ret = -EBUSY;
		goto out;
	}

	ret = drm_vblank_get(drm, pipe);
	if (ret) {
		DRM_ERROR("fail : to acquire vblank counter\n");
		goto out;
	}

	nx_crtc->event = event;
	spin_unlock_irq(&drm->event_lock);

	/*
	 * the pipe from user always is 0 so we can set pipe number
	 * of current owner to event.
	 */
	event->pipe = pipe;

	crtc->primary->fb = fb;
	crtc_w = fb->width - crtc->x;
	crtc_h = fb->height - crtc->y;

	ret = nx_drm_dp_plane_update(crtc->primary, fb, 0, 0,
			crtc_w, crtc_h, crtc->x, crtc->y, crtc_w, crtc_h, 0);

	if (ret) {
		DRM_DEBUG("fail : plane update for page flip %d\n", ret);
		crtc->primary->fb = old_fb;
		spin_lock_irq(&drm->event_lock);
		nx_crtc->event = NULL;
		drm_vblank_put(drm, pipe);
		spin_unlock_irq(&drm->event_lock);
		return ret;
	}

	return 0;

out:
	spin_unlock_irq(&drm->event_lock);
	return ret;
}

static void nx_drm_crtc_destroy(struct drm_crtc *crtc)
{
	struct nx_drm_priv *priv = crtc->dev->dev_private;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	int pipe = nx_crtc->pipe;

	DRM_DEBUG_KMS("enter crtc.%d\n", nx_crtc->pipe);

	priv->crtcs[pipe] = NULL;

	drm_crtc_cleanup(crtc);
	kfree(nx_crtc);
}

static struct drm_crtc_funcs nx_crtc_funcs = {
	.set_config = drm_crtc_helper_set_config,
	.page_flip = nx_drm_crtc_page_flip,
	.destroy = nx_drm_crtc_destroy,
};

int nx_drm_crtc_enable_vblank(struct drm_device *drm, unsigned int pipe)
{
	struct nx_drm_priv *priv = drm->dev_private;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(priv->crtcs[pipe]);

	DRM_DEBUG_KMS("enter pipe.%d\n", pipe);

	if (nx_crtc->dpms_mode != DRM_MODE_DPMS_ON)
		return -EPERM;

	nx_drm_dp_crtc_irq_on(&nx_crtc->crtc, pipe);

	return 0;
}

void nx_drm_crtc_disable_vblank(struct drm_device *drm, unsigned int pipe)
{
	struct nx_drm_priv *priv = drm->dev_private;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(priv->crtcs[pipe]);

	DRM_DEBUG_KMS("enter pipe.%d\n", pipe);

	nx_drm_dp_crtc_irq_off(&nx_crtc->crtc, pipe);
}

#ifdef DEBUG_FPS_TIME
#define	SHOW_PERIOD_SEC		1
#define	FPS_HZ	(60)
#define	DUMP_FPS_TIME(p) {	\
	static long ts[2] = { 0, }, vb_count;	\
	long new = ktime_to_ms(ktime_get());	\
	if (0 == (vb_count++ % (FPS_HZ * SHOW_PERIOD_SEC)))	\
		pr_info("[dp.%d] %ld ms\n", p, new - ts[p]);	\
	ts[p] = new;	\
	}
#else
#define	DUMP_FPS_TIME(p)
#endif

static irqreturn_t nx_drm_crtc_interrupt(int irq, void *arg)
{
	struct drm_device *drm = arg;
	struct nx_drm_priv *priv = drm->dev_private;
	int i;

	for (i = 0; i < priv->num_crtcs; i++) {
		struct drm_crtc *crtc = priv->crtcs[i];
		struct nx_drm_crtc *nx_crtc;

		if (!crtc)
			continue;

		nx_crtc = to_nx_crtc(crtc);

		if (irq == nx_crtc->pipe_irq) {
			struct drm_pending_vblank_event *event = NULL;
			int pipe = nx_crtc->pipe;

			drm_handle_vblank(drm, i);

			spin_lock(&drm->event_lock);

			event = nx_crtc->event;
			if (event) {
				if (nx_crtc->post_closed) {
					drm_vblank_put(drm, i);
					event->base.destroy(&event->base);
				} else {
					drm_send_vblank_event(drm, i, event);
					drm_vblank_put(drm, i);
				}
				nx_crtc->event = NULL;
				nx_crtc->post_closed = false;
			}

			spin_unlock(&drm->event_lock);

			DUMP_FPS_TIME(pipe);

			/* clear irq */
			nx_drm_dp_crtc_irq_done(crtc, pipe);
			break;
		}
	}

	return IRQ_HANDLED;
}

static int nx_drm_crtc_irq_install(struct drm_device *drm,
			struct drm_crtc *crtc)
{
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct drm_driver *drv = drm->driver;
	int irq = nx_crtc->pipe_irq;
	int ret = 0;

	if (NULL == drv->irq_handler)
		drv->irq_handler = nx_drm_crtc_interrupt;

	if (drm->irq_enabled)
		drm->irq_enabled = false;

	ret = drm_irq_install(drm, irq);
	if (0 > ret)
		DRM_ERROR("fail : crtc.%d irq %d !!!\n", nx_crtc->pipe, irq);

	DRM_INFO("irq %d install for crtc.%d\n", irq, nx_crtc->pipe);

	return ret;
}

static int __of_graph_get_port_num_index(struct drm_device *drm,
			int *pipe, int pipe_size)
{
	struct device *dev = &drm->platformdev->dev;
	struct device_node *parent = dev->of_node;
	struct device_node *node, *port;
	int num = 0;

	node = of_get_child_by_name(parent, "ports");
	if (node)
		parent = node;

	for_each_child_of_node(parent, port) {
		u32 port_id = 0;

		if (of_node_cmp(port->name, "port") != 0)
			continue;
		if (of_property_read_u32(port, "reg", &port_id))
			continue;

		pipe[num] = port_id;
		num++;

		if (num > (pipe_size - 1))
			break;
	}
	of_node_put(node);

	return num;
}

#define parse_read_prop(n, s, v)	{ \
	u32 _v;	\
	if (!of_property_read_u32(n, s, &_v))	\
		v = _v;	\
	}

static int nx_drm_crtc_parse_dt_setup(struct drm_device *drm,
			struct drm_crtc *crtc, int pipe)
{
	struct device_node *np;
	struct device *dev = &drm->platformdev->dev;
	struct device_node *node = dev->of_node;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct dp_plane_top *top = &nx_crtc->top;
	const char *strings[10];
	int i, size = 0, err;
	int irq = INVALID_IRQ;

	DRM_DEBUG_KMS("crtc.%d for %s\n", pipe, dev_name(dev));

	/*
	 * parse base address
	 */
	err = nx_drm_dp_crtc_res_parse(drm->platformdev, pipe, &irq,
				nx_crtc->resets, &nx_crtc->num_resets);
	if (0 > err)
		return -EINVAL;

	nx_crtc->pipe_irq = irq;

	if (INVALID_IRQ != nx_crtc->pipe_irq) {
		err = nx_drm_crtc_irq_install(drm, crtc);
		if (0 > err)
			return -EINVAL;
	}

	/*
	 * parse port properties.
	 */
	np = of_graph_get_port_by_id(node, pipe);
	if (!np)
		return -EINVAL;

	parse_read_prop(np, "back_color", top->back_color);
	parse_read_prop(np, "color_key", top->color_key);

	size = of_property_read_string_array(np, "plane-names", strings, 10);
	top->num_planes = size;

	for (i = 0; size > i; i++) {
		if (!strcmp("primary", strings[i])) {
			top->plane_type[i] = DRM_PLANE_TYPE_PRIMARY;
			top->plane_flag[i] = PLANE_FLAG_RGB;
		} else if (!strcmp("cursor", strings[i])) {
			top->plane_type[i] = DRM_PLANE_TYPE_CURSOR;
			top->plane_flag[i] = PLANE_FLAG_RGB;
		} else if (!strcmp("rgb", strings[i])) {
			top->plane_type[i] = DRM_PLANE_TYPE_OVERLAY;
			top->plane_flag[i] = PLANE_FLAG_RGB;
		} else if (!strcmp("video", strings[i])) {
			top->plane_type[i] = DRM_PLANE_TYPE_OVERLAY;
			top->plane_flag[i] = PLANE_FLAG_VIDEO;	/* video */
			top->video_prior = i;	/* priority */
		} else {
			top->plane_flag[i] = PLANE_FLAG_UNKNOWN;
			DRM_ERROR("fail : unknown plane name [%d] %s\n",
				i, strings[i]);
		}
		DRM_DEBUG_KMS("crtc.%d planes[%d]: %s, bg:0x%08x, key:0x%08x\n",
			pipe, i, strings[i], top->back_color, top->color_key);
	}

	return 0;
}

static int nx_drm_crtc_create_planes(struct drm_device *drm,
			struct drm_crtc *crtc, int pipe)
{
	struct drm_plane **planes;
	struct drm_plane *plane;
	struct nx_drm_crtc *nx_crtc = to_nx_crtc(crtc);
	struct dp_plane_top *top = &nx_crtc->top;
	int i = 0, ret = 0;
	int num = 0, plane_num = 0;

	/* setup crtc's planes */
	planes = kzalloc(sizeof(struct drm_plane *) * top->num_planes,
				GFP_KERNEL);
	if (!planes)
		return -ENOMEM;

	for (i = 0; top->num_planes > i; i++) {
		enum drm_plane_type drm_type = top->plane_type[i];
		bool video = top->plane_flag[i] == PLANE_FLAG_VIDEO ?
						true : false;

		if (PLANE_FLAG_UNKNOWN == top->plane_flag[i])
			continue;

		plane_num = video ? PLANE_VIDEO_NUM : num++;

		plane = nx_drm_plane_init(
				drm, crtc, (1 << pipe), drm_type, plane_num);
		if (IS_ERR(plane)) {
			ret = PTR_ERR(plane);
			goto err_plane;
		}

		if (DRM_PLANE_TYPE_PRIMARY == drm_type) {
			top->primary_plane = num - 1;
			ret = drm_crtc_init_with_planes(drm,
					crtc, plane, NULL, &nx_crtc_funcs);
			if (0 > ret)
				goto err_plane;
		}
		planes[i] = plane;
	}

	drm_crtc_helper_add(crtc, &nx_crtc_helper_funcs);
	kfree(planes);

	return 0;

err_plane:
	for (i = 0; top->num_planes > i; i++) {
		plane = planes[i];
		if (plane)
			plane->funcs->destroy(plane);
	}

	kfree(planes);

	return ret;
}

int nx_drm_crtc_init(struct drm_device *drm)
{
	struct nx_drm_crtc **nx_crtcs;
	int pipes[10], num_crtcs = 0;
	int size = ARRAY_SIZE(pipes);
	int i = 0, ret = 0;
	int align = fb_align_rgb;

	/* get ports 'reg' property value */
	num_crtcs = __of_graph_get_port_num_index(drm, pipes, size);

	if (PAGE_SIZE >= align && align > 0)
		fb_align_rgb = align;
	else
		fb_align_rgb = 1;

	DRM_INFO("num of crtcs %d, FB %d align, FB vblank %s\n",
		num_crtcs, fb_align_rgb, fb_vblank_wait ? "Wait" : "Pass");

	/* setup crtc's planes */
	nx_crtcs = kzalloc(sizeof(struct nx_drm_crtc *) * num_crtcs,
				GFP_KERNEL);
	if (!nx_crtcs)
		return -ENOMEM;

	for (i = 0; num_crtcs > i; i++) {
		struct nx_drm_priv *priv;
		struct nx_drm_crtc *nx_crtc;
		int pipe = pipes[i];	/* reg property */

		nx_crtc = kzalloc(sizeof(struct nx_drm_crtc), GFP_KERNEL);
		if (!nx_crtc)
			goto err_crtc;

		priv = drm->dev_private;
		priv->crtcs[i] = &nx_crtc->crtc;	/* sequentially link */
		priv->num_crtcs++;
		priv->possible_pipes |= (1 << pipe);

		nx_crtc->pipe = pipe;
		nx_crtc->dpms_mode = DRM_MODE_DPMS_OFF;
		nx_crtc->pipe_irq = INVALID_IRQ;

		ret = nx_drm_crtc_parse_dt_setup(drm, &nx_crtc->crtc, pipe);
		if (0 > ret)
			return ret;

		nx_drm_dp_crtc_init(drm, &nx_crtc->crtc, pipe);
		ret = nx_drm_crtc_create_planes(drm, &nx_crtc->crtc, pipe);
		if (0 > ret)
			goto err_crtc;

		nx_crtcs[i]	= nx_crtc;
		DRM_INFO("crtc[%d]: pipe.%d (irq.%d)\n",
			i, pipe, nx_crtc->pipe_irq);
	}

	kfree(nx_crtcs);

	DRM_DEBUG_KMS("done\n");
	return 0;

err_crtc:
	for (i = 0; num_crtcs > i; i++)
		kfree(nx_crtcs[i]);

	kfree(nx_crtcs);

	return ret;
}

