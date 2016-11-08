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
#include <linux/of_platform.h>
#include <linux/component.h>

#include <drm/nexell_drm.h>

#include "nx_drm_drv.h"
#include "nx_drm_crtc.h"
#include "nx_drm_connector.h"
#include "nx_drm_encoder.h"
#include "nx_drm_fb.h"
#include "nx_drm_plane.h"
#include "nx_drm_gem.h"

/*
 * DRM Configuration
 *
 * CRTC		    : MLC top control (and display interrupt, reset, clock, ...)
 * Plane	    : MLC layer control
 * Encoder	    : DPC control
 * Connector	: DRM connetcor for LCD, LVDS, MiPi, HDMI,...
 * Panel	    : Display device control (LCD, LVDS, MiPi, HDMI,...)
 *
 */

static void nx_drm_output_poll_changed(struct drm_device *drm)
{
	struct nx_drm_priv *priv = drm->dev_private;
	struct nx_framebuffer_dev *nx_framebuffer = priv->framebuffer_dev;

	DRM_DEBUG_KMS("enter : fbdev %s\n",
		nx_framebuffer ? "exist" : "non exist");

	mutex_lock(&priv->lock);

	if (nx_framebuffer && nx_framebuffer->fbdev)
		drm_fb_helper_hotplug_event(
			(struct drm_fb_helper *)nx_framebuffer->fbdev);
	else
		nx_drm_framebuffer_init(drm);

	mutex_unlock(&priv->lock);
	DRM_DEBUG_DRIVER("exit.\n");
}

static struct drm_mode_config_funcs nx_mode_config_funcs = {
	.fb_create = nx_drm_fb_mode_create,
	.output_poll_changed = nx_drm_output_poll_changed,
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

static int nx_drm_load(struct drm_device *drm, unsigned long flags)
{
	struct nx_drm_priv *priv;
	int ret;

	DRM_DEBUG_DRIVER("drm %s flags 0x%lx\n", dev_name(drm->dev), flags);

	priv = kzalloc(sizeof(struct nx_drm_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	mutex_init(&priv->lock);
	drm->dev_private = (void *)priv;
	dev_set_drvdata(drm->dev, drm);

	/* drm->mode_config initialization */
	drm_mode_config_init(drm);
	nx_drm_mode_config_init(drm);

	/* Try to nexell crtcs. */
	ret = nx_drm_crtc_init(drm);
	if (ret)
		goto err_mode_config_cleanup;

	ret = drm_vblank_init(drm, priv->num_crtcs);
	if (ret)
		goto err_unbind_all;

	/* Try to bind all sub drivers. */
	ret = component_bind_all(drm->dev, drm);
	if (ret)
		goto err_mode_config_cleanup;

	drm->vblank_disable_allowed = true;

	/* init kms poll for handling hpd */
	drm_kms_helper_poll_init(drm);

	/* force connectors detection for LCD */
	if (priv->force_detect)
		drm_helper_hpd_irq_event(drm);

	return 0;

err_unbind_all:
	component_unbind_all(drm->dev, drm);

err_mode_config_cleanup:
	drm_mode_config_cleanup(drm);
	kfree(priv);

	return ret;
}

static int nx_drm_unload(struct drm_device *drm)
{
	DRM_DEBUG_DRIVER("enter\n");

	nx_drm_framebuffer_fini(drm);

	drm_vblank_cleanup(drm);
	drm_kms_helper_poll_fini(drm);
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
	.llseek = no_llseek,
	.mmap = nx_drm_gem_fops_mmap,
};

static void nx_drm_lastclose(struct drm_device *drm)
{
	struct nx_drm_priv *priv = drm->dev_private;
	struct nx_drm_fbdev *fbdev;

	if (!priv || !priv->framebuffer_dev)
		return;

	fbdev = priv->framebuffer_dev->fbdev;
	if (fbdev)
		drm_fb_helper_restore_fbdev_mode_unlocked(
				(struct drm_fb_helper *)fbdev);
}

static void nx_drm_postclose(struct drm_device *drm, struct drm_file *file)
{
	struct drm_pending_vblank_event *event;
	struct nx_drm_crtc *nx_crtc;
	struct nx_drm_priv *priv = drm->dev_private;
	unsigned long flags;
	int i;

	for (i = 0; i < priv->num_crtcs; i++) {
		nx_crtc = to_nx_crtc(priv->crtcs[i]);
		event = nx_crtc->event;
		if (event && event->base.file_priv == file) {
			spin_lock_irqsave(&drm->event_lock, flags);
			nx_crtc->post_closed = true;
			spin_unlock_irqrestore(&drm->event_lock, flags);
		}
	}
}

static struct drm_driver nx_drm_driver = {
	.driver_features = DRIVER_HAVE_IRQ | DRIVER_MODESET |
		DRIVER_GEM | DRIVER_PRIME | DRIVER_IRQ_SHARED,
	.load = nx_drm_load,
	.unload = nx_drm_unload,
	.fops = &nx_drm_fops,	/* replace fops */
	.lastclose = nx_drm_lastclose,
	.postclose = nx_drm_postclose,
	.set_busid = drm_platform_set_busid,

	.get_vblank_counter = drm_vblank_no_hw_counter,
	.enable_vblank = nx_drm_crtc_enable_vblank,
	.disable_vblank = nx_drm_crtc_disable_vblank,

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
	.major = 2,
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

static const struct component_master_ops nx_drm_ops = {
	.bind = nx_drm_bind,
	.unbind = nx_drm_unbind,
};

static int match_dev(struct device *dev, void *data)
{
	return dev == (struct device *)data;
}

#ifdef CHECK_DRIVER_NAME
static int match_drv(struct device_driver *drv, void *data)
{
	const char *t = data, *f = drv->name;

	return strstr(f, t) ? 1 : 0;
}
#endif

static int match_component(struct device *dev, void *data)
{
	const char *name = data;
	const char *t = name, *f = dev_name(dev);

	return strstr(f, t) ? 1 : 0;
}

static int nx_drm_probe(struct platform_device *pdev)
{
	struct component_match *match = NULL;
	const char *const dev_names[] = {
		/* node name (x:name) */
#ifdef CONFIG_DRM_NX_RGB
		"display_drm_rgb",
#endif
#ifdef CONFIG_DRM_NX_LVDS
		"display_drm_lvds",
#endif
#ifdef CONFIG_DRM_NX_MIPI_DSI
		"display_drm_mipi",
#endif
#ifdef CONFIG_DRM_NX_HDMI
		"display_drm_hdmi",
#endif
	};
	int found = 0;
	int i;

	DRM_DEBUG_DRIVER("enter %s\n", dev_name(&pdev->dev));

	for (i = 0; i < ARRAY_SIZE(dev_names); i++) {
		struct device *dev;

		dev = bus_find_device(&platform_bus_type, NULL,
				      (void *)dev_names[i], match_component);
		if (!dev) {
			DRM_INFO("not found device name: %s\n", dev_names[i]);
			continue;
		}

		#ifdef CHECK_DRIVER_NAME
		if (!bus_for_each_drv(dev->bus, NULL,
			(void *)dev_names[i], match_drv)) {
			DRM_INFO("not found driver: %s\n", dev_names[i]);
			continue;
		}
		#endif

		component_match_add(&pdev->dev, &match, match_dev, dev);
		found++;
	}

	if (!found)
		return -EINVAL;

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	/* call master bind */
	return component_master_add_with_match(&pdev->dev, &nx_drm_ops, match);
}

static int nx_drm_remove(struct platform_device *pdev)
{
	component_master_del(&pdev->dev, &nx_drm_ops);
	return 0;
}

static const struct of_device_id dt_of_match[] = {
	{.compatible = "nexell,s5pxx18-drm"},
	{}
};
MODULE_DEVICE_TABLE(of, dt_of_match);


static int nx_drm_pm_suspend(struct device *dev)
{
	struct drm_connector *connector;
	struct drm_device *drm = dev_get_drvdata(dev);
	struct nx_drm_priv *priv = drm->dev_private;
	int i;

	DRM_DEBUG_DRIVER("enter %s\n", dev_name(dev));

	drm_modeset_lock_all(drm);

	for (i = 0; i < priv->num_crtcs; i++)
		to_nx_crtc(priv->crtcs[i])->suspended = true;

	list_for_each_entry(connector, &drm->mode_config.connector_list, head) {
		int old_dpms = connector->dpms;
		struct nx_drm_device *display =
				to_nx_connector(connector)->display;

		if (display)
			display->suspended = true;

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
	struct drm_connector *connector;
	struct drm_device *drm = dev_get_drvdata(dev);
	struct nx_drm_priv *priv = drm->dev_private;
	int i;

	DRM_DEBUG_DRIVER("enter %s\n", dev_name(dev));

	drm_modeset_lock_all(drm);

	for (i = 0; i < priv->num_crtcs; i++)
		nx_drm_dp_crtc_reset(priv->crtcs[i]);

	list_for_each_entry(connector, &drm->mode_config.connector_list, head) {
		if (connector->funcs->dpms) {
			int dpms = connector->dpms;
			struct nx_drm_device *display =
					to_nx_connector(connector)->display;

			connector->dpms = DRM_MODE_DPMS_OFF;
			connector->funcs->dpms(connector, dpms);
			if (display)
				display->suspended = false;
		}
	}

	for (i = 0; i < priv->num_crtcs; i++)
		to_nx_crtc(priv->crtcs[i])->suspended = false;

	drm_modeset_unlock_all(drm);

	return 0;
}

static const struct dev_pm_ops nx_drm_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(nx_drm_pm_suspend, nx_drm_pm_resume)
};

static struct platform_driver nx_drm_platform_drv = {
	.probe = nx_drm_probe,
	.remove = nx_drm_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "nexell,display_drm",
		   .of_match_table = dt_of_match,
		   .pm	= &nx_drm_pm_ops,
		   },
};
module_platform_driver(nx_drm_platform_drv);

MODULE_AUTHOR("jhkim <jhkim@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell DRM Driver");
MODULE_LICENSE("GPL");
