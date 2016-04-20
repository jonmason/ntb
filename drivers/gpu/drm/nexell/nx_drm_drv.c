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
#include "nx_drm_encoder.h"
#include "nx_drm_fb.h"
#include "nx_drm_plane.h"
#include "nx_drm_gem.h"

static void nx_drm_output_poll_changed(struct drm_device *drm)
{
	struct nx_drm_priv *priv = drm->dev_private;
	struct nx_framebuffer_dev *nx_fbdev = priv->fbdev;
	struct drm_fbdev_cma *fbdev = nx_fbdev->fbdev;

	DRM_DEBUG_KMS("enter\n");

	if (fbdev)
		drm_fb_helper_hotplug_event(
			(struct drm_fb_helper *)fbdev);

	DRM_DEBUG_DRIVER("exit.\n");
}

static struct drm_mode_config_funcs nx_mode_config_funcs = {
	.fb_create = drm_fb_cma_create,
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

	DRM_DEBUG_DRIVER("drm %p, %s flags 0x%lx\n",
			drm, dev_name(drm->dev), flags);

	priv = devm_kzalloc(drm->dev,
			sizeof(struct nx_drm_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	drm->dev_private = (void *)priv;
	dev_set_drvdata(drm->dev, drm);

	nx_drm_driver_parse_dt_setup(drm, priv);

	/* drm->mode_config initialization */
	drm_mode_config_init(drm);
	nx_drm_mode_config_init(drm);

	drm_kms_helper_poll_init(drm);

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

#ifdef CONFIG_DRM_NX_FBDEV
	ret = nx_drm_framebuffer_dev_init(drm);
	if (ret) {
		DRM_ERROR("fail : initialize drm fbdev\n");
		drm_vblank_cleanup(drm);
		goto err_unbind_all;
	}
#endif

	return 0;

err_unbind_all:
	component_unbind_all(drm->dev, drm);

err_mode_config_cleanup:
	drm_mode_config_cleanup(drm);
	devm_kfree(drm->dev, priv);

	return ret;
}

static int nx_drm_unload(struct drm_device *drm)
{
	DRM_DEBUG_DRIVER("enter\n");

#ifdef CONFIG_DRM_NX_FBDEV
	nx_drm_framebuffer_dev_fini(drm);
#endif
	drm_vblank_cleanup(drm);
	drm_kms_helper_poll_fini(drm);
	drm_mode_config_cleanup(drm);
	devm_kfree(drm->dev, drm->dev_private);

	drm->dev_private = NULL;
	return 0;
}

static struct drm_ioctl_desc nx_drm_ioctls[] = {
	DRM_IOCTL_DEF_DRV(NX_GEM_CREATE,
		nx_drm_gem_create_ioctl, DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(NX_GEM_GET,
		nx_drm_gem_get_ioctl, DRM_UNLOCKED),
};

static const struct file_operations nx_drm_driver_fops = {
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
	.mmap = drm_gem_cma_mmap,
};

static struct dma_buf *nx_drm_gem_prime_export(struct drm_device *drm,
			struct drm_gem_object *obj,
			int flags)
{
	/* we want to be able to write in mmapped buffer */
	flags |= O_RDWR;
	return drm_gem_prime_export(drm, obj, flags);
}

static void nx_drm_lastclose(struct drm_device *dev)
{
#ifdef CONFIG_DRM_NX_FBDEV
	struct nx_drm_priv *priv = dev->dev_private;
	struct drm_fbdev_cma *fbdev;

	if (!priv || !priv->fbdev)
		return;

	fbdev = priv->fbdev->fbdev;
	if (fbdev)
		drm_fb_helper_restore_fbdev_mode_unlocked(
			(struct drm_fb_helper *)fbdev);
#endif
}

static struct drm_driver nx_drm_driver = {
	.driver_features = DRIVER_HAVE_IRQ | DRIVER_MODESET | DRIVER_GEM |
	    DRIVER_PRIME,
	.load = nx_drm_load,
	.unload = nx_drm_unload,
	.fops = &nx_drm_driver_fops,
	.lastclose = nx_drm_lastclose,
	.set_busid = drm_platform_set_busid,

	.get_vblank_counter = drm_vblank_count,
	.enable_vblank = nx_drm_crtc_enable_vblank,
	.disable_vblank = nx_drm_crtc_disable_vblank,

	.gem_free_object = drm_gem_cma_free_object,
	.gem_vm_ops = &drm_gem_cma_vm_ops,

	.prime_handle_to_fd = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle = drm_gem_prime_fd_to_handle,
	.gem_prime_export = nx_drm_gem_prime_export,
	.gem_prime_import = drm_gem_prime_import,
	.gem_prime_get_sg_table = nx_drm_gem_prime_get_sg_table,

	.gem_prime_import_sg_table = drm_gem_cma_prime_import_sg_table,
	.gem_prime_vmap = drm_gem_cma_prime_vmap,
	.gem_prime_vunmap = drm_gem_cma_prime_vunmap,
	.gem_prime_mmap = drm_gem_cma_prime_mmap,

	.dumb_create = drm_gem_cma_dumb_create,
	.dumb_map_offset = drm_gem_cma_dumb_map_offset,
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

static int match_drv(struct device_driver *drv, void *data)
{
	const char *t = data, *f = drv->name;

	return strstr(f, t) ? 1 : 0;
}

static int match_component(struct device *dev, void *data)
{
	const char *name = data;
	const char *t = name, *f = dev_name(dev);

	return strstr(f, t) ? 1 : 0;
}

static int nx_drm_probe(struct platform_device *pdev)
{
	struct component_match *match = NULL;
	static const char *const dev_names[] = {
		"display_drm_lcd",	/* node name (x:name) */
		"display_drm_hdmi",
	};
	bool found = false;
	int i;

	DRM_DEBUG_DRIVER("enter %s\n", dev_name(&pdev->dev));

	for (i = 0; i < ARRAY_SIZE(dev_names); i++) {
		struct device *dev;

		dev = bus_find_device(&platform_bus_type, NULL,
				      (void *)dev_names[i], match_component);
		if (!dev) {
			DRM_INFO("not found device: %s\n", dev_names[i]);
			continue;
		}
		if (!bus_for_each_drv(dev->bus, NULL,
			(void *)dev_names[i], match_drv)) {
			DRM_INFO("not found driver: %s\n", dev_names[i]);
			continue;
		}

		component_match_add(&pdev->dev, &match, match_dev, dev);
		found = true;
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
	{.compatible = "nexell,s5p6818-drm"},
	{}
};
MODULE_DEVICE_TABLE(of, dt_of_match);

static struct platform_driver nx_drm_platform_drv = {
	.probe = nx_drm_probe,
	.remove = nx_drm_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "nexell,display_drm",
		   .of_match_table = dt_of_match,
		   },
};
module_platform_driver(nx_drm_platform_drv);

MODULE_AUTHOR("jhkim <jhkim@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell DRM Driver");
MODULE_LICENSE("GPL");
