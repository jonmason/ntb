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
#include <drm/drm_fb_helper.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>

#include <linux/of_platform.h>
#include <linux/component.h>

#include <drm/nexell_drm.h>

#include "nx_drm_drv.h"
#include "nx_drm_fb.h"
#include "nx_drm_gem.h"

static void nx_drm_output_poll_changed(struct drm_device *drm)
{
	struct nx_drm_private *private = drm->dev_private;
	struct nx_drm_fbdev *nx_fbdev = private->fbdev;

	DRM_DEBUG_KMS("enter : framebuffer dev %s\n",
		nx_fbdev ? "exist" : "non exist");

	if (nx_fbdev)
		drm_fb_helper_hotplug_event(&nx_fbdev->fb_helper);
	else
		nx_drm_framebuffer_init(drm);
}

static void nx_atomic_commit_complete(struct commit *commit,
			struct drm_atomic_state *state)
{
	struct drm_device *drm = commit->drm;

	/*
	 * Everything below can be run asynchronously without the need to grab
	 * any modeset locks at all under one condition: It must be guaranteed
	 * that the asynchronous work has either been cancelled (if the driver
	 * supports it, which at least requires that the framebuffers get
	 * cleaned up with drm_atomic_helper_cleanup_planes()) or completed
	 * before the new state gets committed on the software side with
	 * drm_atomic_helper_swap_state().
	 *
	 * This scheme allows new atomic state updates to be prepared and
	 * checked in parallel to the asynchronous completion of the previous
	 * update. Which is important since compositors need to figure out the
	 * composition of the next frame right after having submitted the
	 * current layout.
	 */
	drm_atomic_helper_commit_modeset_disables(drm, state);
	drm_atomic_helper_commit_planes(drm, state, false);
	drm_atomic_helper_commit_modeset_enables(drm, state);

	/*
	 * wait hw vblank
	 * skip 1st frame, vblank is not enabled at 1st,
	 * and not changed framebuffer
	 */
	drm_atomic_helper_wait_for_vblanks(drm, state);
	drm_atomic_helper_cleanup_planes(drm, state);
	drm_atomic_state_free(state);

	spin_lock(&commit->lock);
	commit->pending &= ~commit->crtcs;
	spin_unlock(&commit->lock);

	wake_up_all(&commit->wait);
}

static void nx_atomic_schedule(struct commit *commit,
				  struct drm_atomic_state *state)
{
	commit->state = state;
	schedule_work(&commit->work);
}

static void nx_drm_atomic_work(struct work_struct *work)
{
	struct commit *commit = container_of(work, struct commit, work);

	nx_atomic_commit_complete(commit, commit->state);
}

static int commit_is_pending(struct commit *commit)
{
	u32 crtcs = commit->crtcs;
	bool pending;

	spin_lock(&commit->lock);
	pending = commit->pending & crtcs;
	spin_unlock(&commit->lock);

	return pending;
}

static int nx_drm_atomic_commit(struct drm_device *drm,
			struct drm_atomic_state *state, bool async)
{
	struct nx_drm_private *private = drm->dev_private;
	struct commit *commit = &private->commit;
	int i, ret;

	DRM_DEBUG_KMS("enter : %s\n", async ? "async" : "sync");

	ret = drm_atomic_helper_prepare_planes(drm, state);
	if (ret)
		return ret;

	mutex_lock(&commit->m_lock);

	/* Wait until all affected CRTCs have completed previous commits and
	 * mark them as pending.
	 */
	for (i = 0; i < drm->mode_config.num_crtc; ++i) {
		if (state->crtcs[i])
			commit->crtcs |= 1 << drm_crtc_index(state->crtcs[i]);
	}

	wait_event(commit->wait, !commit_is_pending(commit));

	spin_lock(&commit->lock);
	commit->pending |= commit->crtcs;
	spin_unlock(&commit->lock);

	/* Swap the state, this is the point of no return. */
	drm_atomic_helper_swap_state(drm, state);

	if (async)
		nx_atomic_schedule(commit, state);
	else
		nx_atomic_commit_complete(commit, state);

	mutex_unlock(&commit->m_lock);
	return 0;
}

static struct drm_mode_config_funcs nx_mode_config_funcs = {
	.fb_create = nx_drm_fb_mode_create,
	.output_poll_changed = nx_drm_output_poll_changed,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = nx_drm_atomic_commit,
};

static void nx_drm_mode_config_init(struct drm_device *drm)
{
	drm->mode_config.min_width = 0;
	drm->mode_config.min_height = 0;

	/*
	 * set max width and height as default value
	 * this value would be used to check framebuffer size limitation
	 * at drm_mode_addfb().
	 */
	drm->mode_config.max_width = MAX_FB_MODE_WIDTH;
	drm->mode_config.max_height = MAX_FB_MODE_HEIGHT;
	drm->mode_config.funcs = &nx_mode_config_funcs;

	DRM_DEBUG_KMS("min %d*%d, max %d*%d\n",
		 drm->mode_config.min_width, drm->mode_config.min_height,
		 drm->mode_config.max_width, drm->mode_config.max_height);
}

static struct nx_drm_private *nx_drm_private_init(struct drm_device *drm)
{
	struct nx_drm_private *private;
	struct commit *commit;

	private = kzalloc(sizeof(struct nx_drm_private), GFP_KERNEL);
	if (!private)
		return NULL;

	drm->dev_private = (void *)private;
	dev_set_drvdata(drm->dev, drm);

	commit = &private->commit;
	commit->drm = drm;

	mutex_init(&commit->m_lock);
	spin_lock_init(&commit->lock);
	init_waitqueue_head(&commit->wait);
	INIT_WORK(&commit->work, nx_drm_atomic_work);

	return private;
}

static int nx_drm_load(struct drm_device *drm, unsigned long flags)
{
	struct nx_drm_private *private;
	int ret;

	DRM_DEBUG_DRIVER("drm %s\n", dev_name(drm->dev));

	private = nx_drm_private_init(drm);
	if (!private)
		return -ENOMEM;

	drm_mode_config_init(drm);
	nx_drm_mode_config_init(drm);

	ret = nx_drm_crtc_create(drm);
	if (ret)
		goto err_mode_config_cleanup;

	/* Try to bind all sub drivers. */
	ret = component_bind_all(drm->dev, drm);
	if (ret)
		goto err_unbind_all;

	ret = drm_vblank_init(drm, private->num_crtcs);
	if (ret)
		goto err_mode_config_cleanup;

	ret = nx_drm_vblank_init(drm);
	if (ret)
		goto err_mode_config_cleanup;

	drm_mode_config_reset(drm);

	/* disable irq turn off  */
#ifdef CONFIG_VIDEO_NEXELL_REARCAM
	drm->vblank_disable_allowed = false;
#else
	drm->vblank_disable_allowed = true;
#endif

	/* init kms poll for handling hpd */
	drm_kms_helper_poll_init(drm);

	/* force connectors detection for LCD */
	if (private->force_detect)
		drm_helper_hpd_irq_event(drm);

	return 0;

err_unbind_all:
	component_unbind_all(drm->dev, drm);

err_mode_config_cleanup:
	drm_mode_config_cleanup(drm);
	kfree(private);
	return ret;
}

static int nx_drm_unload(struct drm_device *drm)
{
	nx_drm_framebuffer_fini(drm);
	drm_kms_helper_poll_fini(drm);

	drm_vblank_cleanup(drm);
	component_unbind_all(drm->dev, drm);
	drm_mode_config_cleanup(drm);

	kfree(drm->dev_private);
	drm->dev_private = NULL;

	return 0;
}

static struct drm_ioctl_desc nx_drm_ioctls[] = {
	DRM_IOCTL_DEF_DRV(NX_GEM_CREATE, nx_drm_gem_create_ioctl,
			DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(NX_GEM_SYNC, nx_drm_gem_sync_ioctl,
			DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(NX_GEM_GET, nx_drm_gem_get_ioctl,
			DRM_UNLOCKED),
};

static const struct file_operations nx_drm_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = drm_release,
	.unlocked_ioctl = drm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = drm_compat_ioctl,
#endif
	.poll = drm_poll,
	.read = drm_read,
	.mmap = nx_drm_gem_fops_mmap,
};

static void nx_drm_lastclose(struct drm_device *drm)
{
	struct nx_drm_private *private = drm->dev_private;
	struct drm_fb_helper *fb_helper;

	if (!private || !private->fbdev)
		return;

	fb_helper = &private->fbdev->fb_helper;
	if (fb_helper)
		drm_fb_helper_restore_fbdev_mode_unlocked(fb_helper);
}

static void nx_drm_postclose(struct drm_device *drm, struct drm_file *file)
{
	struct drm_pending_event *e, *et;
	unsigned long flags;

	spin_lock_irqsave(&drm->event_lock, flags);
	/* Release all events handled by page flip handler but not freed. */
	list_for_each_entry_safe(e, et, &file->event_list, link) {
		list_del(&e->link);
		e->destroy(e);
	}
	spin_unlock_irqrestore(&drm->event_lock, flags);
}

static struct drm_driver nx_drm_driver = {
	.driver_features = DRIVER_MODESET |
		DRIVER_GEM | DRIVER_PRIME | DRIVER_ATOMIC,

	.load = nx_drm_load,
	.unload = nx_drm_unload,
	.fops = &nx_drm_fops,	/* replace fops */
	.lastclose = nx_drm_lastclose,
	.postclose = nx_drm_postclose,
	.set_busid = drm_platform_set_busid,
	.get_vblank_counter = drm_vblank_no_hw_counter,
	.enable_vblank = nx_drm_vblank_enable,
	.disable_vblank = nx_drm_vblank_disable,
	.prime_handle_to_fd = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle = drm_gem_prime_fd_to_handle,
	.gem_free_object = nx_drm_gem_free_object,
	.gem_prime_export = nx_drm_gem_prime_export,
	.gem_prime_import = drm_gem_prime_import,
	.gem_prime_get_sg_table = nx_drm_gem_prime_get_sg_table,
	.gem_prime_import_sg_table = nx_drm_gem_prime_import_sg_table,
	.dumb_create = nx_drm_gem_dumb_create,
	.dumb_map_offset = nx_drm_gem_dumb_map_offset,
	.dumb_destroy = drm_gem_dumb_destroy,
	.ioctls = nx_drm_ioctls,
	.num_ioctls = ARRAY_SIZE(nx_drm_ioctls),

	.name = "nexell",
	.desc = "nexell SoC DRM",
	.date  = "20160219",
	.major = 3,
	.minor = 0,
};

static int nx_drm_bind(struct device *dev)
{
	return drm_platform_init(&nx_drm_driver, to_platform_device(dev));
}

static void nx_drm_unbind(struct device *dev)
{
	drm_put_dev(platform_get_drvdata(to_platform_device(dev)));
}

static const struct component_master_ops nx_drm_component_ops = {
	.bind = nx_drm_bind,
	.unbind = nx_drm_unbind,
};

static int match_dev(struct device *dev, void *data)
{
	return dev == (struct device *)data;
}

static int match_ddrv(struct device *dev, void *data)
{
	return 1;
}

static int match_component(struct device *dev, void *data)
{
	const char *t = data;
	const char *f = dev->of_node ?
			dev->of_node->full_name : dev_name(dev);

	return strstr(f, t) ? 1 : 0;
}

/*
 * @ drm driver node name (x:name)
 */
static const char *const drm_panel_names[] = {
	"display_drm_rgb",
	"display_drm_lvds",
	"display_drm_mipi",
	"display_drm_hdmi",
	"display_drm_tv",
	"display_drm_cluster_lcd",
};

static struct drm_panel_driver {
	const char *name;
	void (*init)(void);
	void (*exit)(void);
} drm_panel_drivers[] = {
#if defined(CONFIG_DRM_NX_RGB) || defined(CONFIG_DRM_NX_LVDS) || \
	defined(CONFIG_DRM_NX_MIPI_DSI)
	{
		.name = "rgb,lvds,mipi",
		.init = panel_lcd_init,
		.exit = panel_lcd_exit,
	},
#endif
#if defined(CONFIG_DRM_NX_HDMI)
	{
		.name = "hdmi",
		.init = panel_hdmi_init,
		.exit = panel_hdmi_exit,
	},
#endif
#if defined(CONFIG_DRM_NX_TVOUT)
	{
		.name = "tv out",
		.init = panel_tv_init,
		.exit = panel_tv_exit,
	},
#endif
#if defined(CONFIG_DRM_NX_CLUSTER_LCD)
	{
		.name = "display_drm_cluster_lcd",
		.init = panel_cluster_init,
		.exit = panel_cluster_exit,
	},
#endif
};

static int nx_drm_probe(struct platform_device *pdev)
{
	struct component_match *match = NULL;
	struct device *dev = &pdev->dev;
	int found = 0;
	int i;

	DRM_DEBUG_DRIVER("enter %s\n", dev_name(dev));

	for (i = 0; i < ARRAY_SIZE(drm_panel_names); i++) {
		struct device *d;
		const char *panel_name = drm_panel_names[i];

		d = bus_find_device(&platform_bus_type, NULL,
			      (void *)panel_name, match_component);
		if (!d)
			continue;

		if (!driver_find_device(d->driver, NULL,
				(void *)panel_name, match_ddrv)) {
			DRM_INFO("not found driver for %s\n", panel_name);
			continue;
		}
		put_device(d);

		component_match_add(dev, &match, match_dev, d);
		found++;
	}

	if (!found)
		return -EINVAL;

	dev->coherent_dma_mask = DMA_BIT_MASK(32);

	/* call master bind */
	return component_master_add_with_match(dev,
				&nx_drm_component_ops, match);
}

static int nx_drm_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	component_master_del(dev, &nx_drm_component_ops);
	return 0;
}

static const struct of_device_id dt_of_match[] = {
	{.compatible = "nexell,s5pxx18-drm"},
	{}
};
MODULE_DEVICE_TABLE(of, dt_of_match);

static int nx_drm_pm_suspend(struct device *dev)
{
	struct drm_device *drm = dev_get_drvdata(dev);
	struct drm_crtc *crtc;
	struct drm_connector *connector;
	struct nx_drm_crtc *nx_crtc;
	struct nx_drm_connector *nx_connector;

	DRM_DEBUG_DRIVER("enter %s\n", dev_name(dev));

	drm_modeset_lock_all(drm);

	list_for_each_entry(crtc, &drm->mode_config.crtc_list, head) {
		nx_crtc = to_nx_crtc(crtc);
		nx_crtc->suspended = true;
	}

	list_for_each_entry(connector, &drm->mode_config.connector_list, head) {
		int old_dpms = connector->dpms;

		nx_connector = to_nx_connector(connector);
		if (nx_connector)
			nx_connector->suspended = true;

		if (connector->funcs->dpms)
			connector->funcs->dpms(connector, DRM_MODE_DPMS_OFF);

		/* Set the old mode back to the connector for resume */
		connector->dpms = old_dpms;
	}

	drm_modeset_unlock_all(drm);

	return 0;
}

static int nx_drm_pm_resume(struct device *dev)
{
	struct drm_device *drm = dev_get_drvdata(dev);
	struct drm_crtc *crtc;
	struct drm_connector *connector;
	struct nx_drm_crtc *nx_crtc;
	struct nx_drm_connector *nx_connector;

	DRM_DEBUG_DRIVER("enter %s\n", dev_name(dev));

	drm_modeset_lock_all(drm);

	list_for_each_entry(crtc, &drm->mode_config.crtc_list, head) {
		nx_crtc = to_nx_crtc(crtc);
		if (nx_crtc->ops && nx_crtc->ops->reset)
			nx_crtc->ops->reset(crtc);
	}

	list_for_each_entry(connector,
		&drm->mode_config.connector_list, head) {
		if (connector->funcs->dpms) {
			int dpms = connector->dpms;

			connector->dpms = DRM_MODE_DPMS_OFF;
			connector->funcs->dpms(connector, dpms);
			nx_connector = to_nx_connector(connector);
			if (nx_connector)
				nx_connector->suspended = false;
		}
	}

	list_for_each_entry(crtc, &drm->mode_config.crtc_list, head) {
		nx_crtc = to_nx_crtc(crtc);
		nx_crtc->suspended = false;
	}

	drm_modeset_unlock_all(drm);

	return 0;
}

static const struct dev_pm_ops nx_drm_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(nx_drm_pm_suspend, nx_drm_pm_resume)
};

static struct platform_driver nx_drm_drviver = {
	.probe = nx_drm_probe,
	.remove = nx_drm_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "nexell,display_drm",
		   .of_match_table = dt_of_match,
		   .pm	= &nx_drm_pm_ops,
		   },
};

static int __init nx_drm_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(drm_panel_drivers); i++) {
		struct drm_panel_driver *pn = &drm_panel_drivers[i];

		DRM_DEBUG_DRIVER("Load [%d] %s\n", i, pn->name);
		if (pn->init)
			pn->init();
	}

	return platform_driver_register(&nx_drm_drviver);
}

static void __exit nx_drm_exit(void)
{
	int i;

	platform_driver_unregister(&nx_drm_drviver);

	for (i = 0; i < ARRAY_SIZE(drm_panel_drivers); i++) {
		struct drm_panel_driver *pn = &drm_panel_drivers[i];

		DRM_DEBUG_DRIVER("UnLoad[%d] %s\n", i, pn->name);
		if (pn->exit)
			pn->exit();
	}
}

module_init(nx_drm_init);
module_exit(nx_drm_exit);

MODULE_AUTHOR("jhkim <jhkim@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell DRM Driver");
MODULE_LICENSE("GPL");
