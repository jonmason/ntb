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

#include "nx_drm_drv.h"
#include "nx_drm_fb.h"

#define PREFERRED_BPP		32

void nx_drm_framebuffer_dev_fini(struct drm_device *drm)
{
	struct nx_drm_priv *priv = drm->dev_private;
	struct nx_framebuffer_dev *nx_fbdev = priv->fbdev;
	struct drm_fbdev_cma *fbdev = nx_fbdev->fbdev;

	drm_fbdev_cma_fini(fbdev);
	devm_kfree(drm->dev, nx_fbdev);
	priv->fbdev = NULL;
}

int nx_drm_framebuffer_dev_init(struct drm_device *drm)
{
	struct drm_fbdev_cma *fbdev;
	struct nx_drm_priv *priv = drm->dev_private;
	struct nx_framebuffer_dev *nx_fbdev;
	unsigned int num_crtc;
	int bpp;
	int ret = 0;

	DRM_DEBUG_KMS("enter crtc num:%d, connector num:%d\n",
		      drm->mode_config.num_crtc,
		      drm->mode_config.num_connector);

	nx_fbdev = kzalloc(sizeof(*nx_fbdev), GFP_KERNEL);
	if (IS_ERR(nx_fbdev))
		return PTR_ERR(nx_fbdev);

	priv->fbdev = nx_fbdev;
	num_crtc = drm->mode_config.num_crtc;
	bpp = PREFERRED_BPP;

	fbdev = drm_fbdev_cma_init(drm, bpp, num_crtc, MAX_CONNECTOR);
	if (IS_ERR(fbdev)) {
		ret = PTR_ERR(fbdev);
		goto err_drm_fb_dev_free;
	}

	nx_fbdev->fbdev = fbdev;
	return 0;

err_drm_fb_dev_free:
	devm_kfree(drm->dev, nx_fbdev);
	return ret;
}



