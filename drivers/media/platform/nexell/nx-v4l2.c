/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
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

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/of.h>

#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>

#include "nx-v4l2.h"

#define NX_V4L2_DEV_NAME	"nx-v4l2"

static struct nx_v4l2 {
	struct media_device media_dev;
	struct v4l2_device  v4l2_dev;
	struct platform_device *pdev;
	void *alloc_ctx;
} *__me;

struct media_device *nx_v4l2_get_media_device(void)
{
	if (likely(__me))
		return &__me->media_dev;
	return NULL;
}
EXPORT_SYMBOL_GPL(nx_v4l2_get_media_device);

struct v4l2_device *nx_v4l2_get_v4l2_device(void)
{
	if (likely(__me))
		return &__me->v4l2_dev;
	return NULL;
}
EXPORT_SYMBOL_GPL(nx_v4l2_get_v4l2_device);

void *nx_v4l2_get_alloc_ctx(void)
{
	if (likely(__me))
		return &__me->alloc_ctx;
	return NULL;
}
EXPORT_SYMBOL_GPL(nx_v4l2_get_alloc_ctx);

static void nx_v4l2_release_subdev_node(struct video_device *vdev)
{
	struct v4l2_subdev *sd = video_get_drvdata(vdev);

	sd->devnode = NULL;
	kfree(vdev);
}

int nx_v4l2_register_subdev(struct v4l2_subdev *sd)
{
	int ret;
	struct v4l2_device *me = nx_v4l2_get_v4l2_device();

	if (!me) {
		WARN_ON(1);
		return -ENODEV;
	}

	ret = v4l2_device_register_subdev(me, sd);
	if (ret != 0) {
		pr_err("failed to register v4l2 subdev(name: %s)\n",
		       sd->name);
		return ret;
	}

	if (sd->flags & V4L2_SUBDEV_FL_HAS_DEVNODE) {
		struct video_device *vdev = devm_kzalloc(me->dev,
							 sizeof(*vdev),
							 GFP_KERNEL);

		if (!vdev) {
			WARN_ON(1);
			return -ENOMEM;
		}

		video_set_drvdata(vdev, sd);
		strlcpy(vdev->name, sd->name, sizeof(vdev->name));
		vdev->v4l2_dev = me;
		vdev->fops = &v4l2_subdev_fops;
		vdev->release = nx_v4l2_release_subdev_node;
		vdev->ctrl_handler = sd->ctrl_handler;
		ret = __video_register_device(vdev, VFL_TYPE_SUBDEV, -1, 1,
					      sd->owner);
		if (ret < 0) {
			pr_err("failed to __video_register_device for subdev %s\n",
			       sd->name);
			kfree(vdev);
			return ret;
		}
#if defined(CONFIG_MEDIA_CONTROLLER)
		sd->entity.info.dev.major = VIDEO_MAJOR;
		sd->entity.info.dev.minor = vdev->minor;
#endif
		sd->devnode = vdev;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(nx_v4l2_register_subdev);

static int nx_v4l2_probe(struct platform_device *pdev)
{
	int ret;
	struct v4l2_device *v4l2_dev;
	struct nx_v4l2 *nx_v4l2;

	nx_v4l2 = devm_kzalloc(&pdev->dev, sizeof(*nx_v4l2), GFP_KERNEL);
	if (!nx_v4l2) {
		WARN_ON(1);
		return -ENOMEM;
	}

	nx_v4l2->pdev = pdev;

	snprintf(nx_v4l2->media_dev.model, sizeof(nx_v4l2->media_dev.model),
		 "%s", dev_name(&pdev->dev));

	nx_v4l2->media_dev.dev = &pdev->dev;

	v4l2_dev       = &nx_v4l2->v4l2_dev;
	v4l2_dev->mdev = &nx_v4l2->media_dev;
	snprintf(v4l2_dev->name, sizeof(v4l2_dev->name), "%s",
		 dev_name(&pdev->dev));

	/* alloc context: use vb2 dma contig */
	nx_v4l2->alloc_ctx = vb2_dma_contig_init_ctx(&pdev->dev);
	if (!nx_v4l2->alloc_ctx) {
		WARN_ON(1);
		ret = -ENOMEM;
		goto free_me;
	}

	ret = v4l2_device_register(&pdev->dev, &nx_v4l2->v4l2_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register nx v4l2_device\n");
		goto cleanup_alloc_ctx;
	}

	ret = media_device_register(&nx_v4l2->media_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register nx media_device\n");
		goto unregister_v4l2;
	}

	__me = nx_v4l2;

	nx_v4l2->pdev = pdev;
	platform_set_drvdata(pdev, nx_v4l2);

	dev_info(&pdev->dev, "%s success\n", __func__);

	return 0;

unregister_v4l2:
	v4l2_device_unregister(&nx_v4l2->v4l2_dev);

cleanup_alloc_ctx:
	vb2_dma_contig_cleanup_ctx(nx_v4l2->alloc_ctx);

free_me:
	kfree(nx_v4l2);
	__me = NULL;
	return ret;
}

static int nx_v4l2_remove(struct platform_device *pdev)
{
	struct nx_v4l2 *nx_v4l2 = platform_get_drvdata(pdev);

	if (unlikely(!nx_v4l2))
		return 0;

	media_device_unregister(&nx_v4l2->media_dev);
	v4l2_device_unregister(&nx_v4l2->v4l2_dev);
	vb2_dma_contig_cleanup_ctx(nx_v4l2->alloc_ctx);
	kfree(nx_v4l2);

	__me = NULL;
	return 0;
}

static struct platform_device_id nx_v4l2_id_table[] = {
	{ NX_V4L2_DEV_NAME, 0 },
	{ },
};

static const struct of_device_id nx_v4l2_dt_match[] = {
	{ .compatible = "nexell,nx-v4l2" },
	{},
};
MODULE_DEVICE_TABLE(of, nx_v4l2_dt_match);

static struct platform_driver nx_v4l2_driver = {
	.probe      = nx_v4l2_probe,
	.remove     = nx_v4l2_remove,
	.id_table   = nx_v4l2_id_table,
	.driver     = {
		.name   = NX_V4L2_DEV_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(nx_v4l2_dt_match),
	},
};

module_platform_driver(nx_v4l2_driver);

MODULE_AUTHOR("swpark <swpark@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell S5Pxx18 series SoC V4L2/MEDIA top device driver");
MODULE_LICENSE("GPL");
