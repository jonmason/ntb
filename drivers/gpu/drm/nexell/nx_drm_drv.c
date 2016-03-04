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
#include <drm/drm_gem_cma_helper.h>
#include <linux/of_platform.h>
#include <linux/component.h>

#include <drm/nexell_drm.h>

#include "nx_drm_drv.h"
#include "nx_drm_crtc.h"
#include "nx_drm_encoder.h"
#include "nx_drm_fbdev.h"
#include "nx_drm_fb.h"
#include "nx_drm_plane.h"

static int nx_drm_load(struct drm_device *drm, unsigned long flags)
{
	struct nx_drm_private *private;
	int layer = 1;
	int ret, nr;

	DRM_DEBUG_DRIVER("drm %p, flags 0x%lx\n", drm, flags);

	private = kzalloc(sizeof(struct nx_drm_private), GFP_KERNEL);
	if (!private)
		return -ENOMEM;

	drm->dev_private = (void *)private;
	private->num_crtcs = MAX_CRTC;

	dev_set_drvdata(drm->dev, drm);

	/* drm->mode_config initialization */
	drm_mode_config_init(drm);
	nx_drm_mode_config_init(drm);

	drm_kms_helper_poll_init(drm);

	for (nr = 0; nr < private->num_crtcs; nr++) {
		ret = nx_drm_crtc_init(drm, nr, layer);
		if (ret)
			goto err_mode_config_cleanup;
	}

	ret = drm_vblank_init(drm, private->num_crtcs);
	if (ret)
		goto err_unbind_all;

	/* Try to bind all sub drivers. */
	ret = component_bind_all(drm->dev, drm);
	if (ret)
		goto err_mode_config_cleanup;

	/* setup possible_clones.
	 * nx_drm_encoder_setup(drm);
	 */

	/*
	 * create and configure fb helper and also nxp specific
	 * fbdev object.
	 */
	ret = nx_drm_fbdev_init(drm);
	if (ret) {
		DRM_ERROR("failed to initialize drm fbdev\n");
		goto err_cleanup_vblank;
	}

	return 0;

err_cleanup_vblank:
	drm_vblank_cleanup(drm);
err_unbind_all:
	component_unbind_all(drm->dev, drm);
err_mode_config_cleanup:
	drm_mode_config_cleanup(drm);
	kfree(private);

	return ret;
}

static int nx_drm_unload(struct drm_device *drm)
{
	DRM_DEBUG_DRIVER("enter\n");

	nx_drm_fbdev_fini(drm);

	drm_vblank_cleanup(drm);
	drm_kms_helper_poll_fini(drm);
	drm_mode_config_cleanup(drm);
	kfree(drm->dev_private);

	drm->dev_private = NULL;
	return 0;
}

static struct drm_ioctl_desc nx_drm_ioctls[] = {
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

static struct drm_driver nx_drm_driver = {
	.driver_features = DRIVER_HAVE_IRQ | DRIVER_MODESET | DRIVER_GEM |
	    DRIVER_PRIME,
	.load = nx_drm_load,
	.unload = nx_drm_unload,
	.fops = &nx_drm_driver_fops,

	.get_vblank_counter = drm_vblank_count,
	.enable_vblank = nx_drm_crtc_enable_vblank,
	.disable_vblank = nx_drm_crtc_disable_vblank,

	.gem_free_object = drm_gem_cma_free_object,
	.gem_vm_ops = &drm_gem_cma_vm_ops,

	/* added */
	.prime_handle_to_fd = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle = drm_gem_prime_fd_to_handle,
	.gem_prime_export = nx_drm_gem_prime_export,
	.gem_prime_import = drm_gem_prime_import,
	.gem_prime_get_sg_table = drm_gem_cma_prime_get_sg_table,
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
	.major = 1,
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

/* -----------------------------------------------------------------------------
 * Platform driver
 */
static int compare_dev(struct device *dev, void *data)
{
	return dev == (struct device *)data;
}

static int match_name(struct device *dev, void *data)
{
	const char *name = data;

	DRM_DEBUG_DRIVER("[match %s vs %s]\n", dev_name(dev), name);

	return strstr(dev_name(dev), name) ? 1 : 0;
}

static int nx_drm_probe(struct platform_device *pdev)
{
	struct component_match *match = NULL;
	static const char * const dev_names[] = {
		"drm-sub-lcd",	/* node name */
		/* "drm-hdmi", */
	};
	int i;

	DRM_DEBUG_DRIVER("enter\n");

	for (i = 0; i < ARRAY_SIZE(dev_names); i++) {
		struct device *dev;

		dev = bus_find_device(&platform_bus_type, NULL,
				      (void *)dev_names[i], match_name);
		if (!dev) {
			DRM_INFO("not found device: %s.\n", dev_names[i]);
			continue;
		}
		component_match_add(&pdev->dev, &match, compare_dev, dev);
	}

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	/* call master bind */
	return component_master_add_with_match(&pdev->dev, &nx_drm_ops, match);
}

static int nx_drm_remove(struct platform_device *pdev)
{
	component_master_del(&pdev->dev, &nx_drm_ops);
	return 0;
}

static const struct of_device_id drm_drv_of_match[] = {
	{.compatible = "nexell,drm-s5p6818"},
	{}
};
MODULE_DEVICE_TABLE(of, drm_drv_of_match);

static struct platform_driver nx_drm_platform_driver = {
	.probe = nx_drm_probe,
	.remove = nx_drm_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "nexell-drm",
		   .of_match_table = drm_drv_of_match,
		   },
};
module_platform_driver(nx_drm_platform_driver);

MODULE_AUTHOR("jhkim <jhkim@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell DRM Driver");
MODULE_LICENSE("GPL");
