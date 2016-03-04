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

#include <linux/of_platform.h>
#include <linux/component.h>
#include <drm/nexell_drm.h>

#include "nx_drm_drv.h"
#include "nx_drm_plane.h"
#include "nx_drm_crtc.h"
#include "nx_drm_encoder.h"
#include "nx_drm_connector.h"

/* lcd has totally three virtual windows. */
#define WINDOWS_NR		3

struct lcd_rgb_context {
	struct drm_connector *connector;
	struct drm_encoder *encoder;
	struct drm_crtc *crtc;

	unsigned int clkdiv;
	unsigned int default_win;
	unsigned long irq_flags;
	unsigned int connected;
	bool vblank_on;
	bool suspended;
	struct work_struct work;
	struct mutex lock;
	int bit_per_pixel;
};

#define lcd_connector(c)		\
			container_of(c, struct lcd_rgb_context, connector)

#define	LCD_BYTE_PER_PIXEL			4
#define	LCD_FRAMEBUFFER_FORMAT		MLC_RGBFMT_X8R8G8B8

static bool lcd_rgb_drm_is_connected(struct device *dev)
{
	DRM_DEBUG_KMS("enter\n");
	return true;
}

static int lcd_rgb_drm_check_timing(struct device *dev, void *timing)
{
	DRM_DEBUG_KMS("todo\n");
	return 0;
}

static int lcd_rgb_drm_display_power_on(struct device *dev, int mode)
{
	DRM_DEBUG_KMS("todo\n");
	return 0;
}

static void lcd_rgb_drm_dpms(struct device *dev, int mode)
{
	struct lcd_rgb_context *lcd;

	DRM_DEBUG_KMS("enter mode:%d\n", mode);
	if (!dev)
		return;

	lcd = dev_get_drvdata(dev);
	mutex_lock(&lcd->lock);

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		/* TODO. */
		break;
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		/* TODO. */
		break;
	default:
		DRM_DEBUG_KMS("unspecified mode %d\n", mode);
		break;
	}

	mutex_unlock(&lcd->lock);
}

static void lcd_rgb_drm_apply(struct device *dev)
{
	DRM_DEBUG_KMS("enter\n");
}

static int lcd_rgb_drm_get_modes(struct drm_connector *connector)
{
	struct nx_drm_connector *nx_connector = to_nx_connector(connector);
	struct lcd_rgb_context *lcd = nx_connector->context;
	struct drm_display_mode *mode;

	DRM_DEBUG_KMS("enter\n");
	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("failed to create a new display mode\n");
		return 0;
	}

	/* set BPP */
	connector->cmdline_mode.bpp_specified = true;
	connector->cmdline_mode.bpp = (LCD_BYTE_PER_PIXEL * 8);

	mode->vrefresh = 60;
	mode->clock = 75000;
	mode->hdisplay = 1024;
	mode->hsync_start = 1500;
	mode->hsync_end = 1540;
	mode->htotal = 1650;
	mode->vdisplay = 600;
	mode->vsync_start = 740;
	mode->vsync_end = 745;
	mode->vtotal = 750;
	mode->type = 0x40;
	mode->flags = 0xa;

	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	DRM_DEBUG_KMS("exit, (%dx%d)\n", mode->hdisplay, mode->vdisplay);
	return 1;
}

static void lcd_rgb_drm_commit(struct drm_device *drm, struct device *dev)
{
	DRM_DEBUG_KMS("enter paddr=0x%x\n",
		      (uint32_t) drm->mode_config.fb_base);
}

static struct nx_drm_operation lcd_rgb_drm_ops = {
	.type = NX_DISPLAY_TYPE_LCD,
	.is_connected = lcd_rgb_drm_is_connected,
	.check_timing = lcd_rgb_drm_check_timing,
	.power_on = lcd_rgb_drm_display_power_on,
	.dpms = lcd_rgb_drm_dpms,
	.apply = lcd_rgb_drm_apply,
	.get_mode = lcd_rgb_drm_get_modes,
	.commit = lcd_rgb_drm_commit,
};

static struct nx_drm_display lcd_rgb_drm_display = {
	.pipe = -1,
	.ops = &lcd_rgb_drm_ops,
};

static int lcd_rgb_drm_power_on(struct lcd_rgb_context *ctx, bool enable)
{
	DRM_DEBUG_KMS("enter\n");
	return 0;
}

static int lcd_rgb_drm_bind(struct device *dev,
				struct device *master, void *data)
{
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	struct drm_device *drm = data;
	struct nx_drm_private *private = drm->dev_private;
	struct nx_drm_display *display = &lcd_rgb_drm_display;
	struct lcd_rgb_context *lcd = dev_get_drvdata(dev);
	int crtc_num = (1 << private->num_crtcs) - 1;

	DRM_DEBUG_KMS("enter\n");

	encoder = nx_drm_encoder_create(drm, display, crtc_num, lcd);
	if (!encoder) {
		DRM_ERROR("failed to create LCD encoder\n");
		return -EFAULT;
	}

	connector = nx_drm_connector_create(drm, encoder, lcd);
	if (!connector) {
		DRM_ERROR("failed to create LCD connector\n");
		encoder->funcs->destroy(encoder);
		return -EFAULT;
	}

	lcd->encoder = encoder;
	lcd->connector = connector;
	DRM_DEBUG_KMS("exit, LCD encoder:0x%p, connector:0x%p\n", encoder,
		      connector);
	return 0;
}

static void lcd_rgb_drm_unbind(struct device *dev,
				struct device *master, void *data)
{
	struct lcd_rgb_context *lcd = dev_get_drvdata(dev);
	struct drm_encoder *encoder = lcd->encoder;
	struct drm_connector *connector = lcd->connector;

	DRM_DEBUG_KMS("enter\n");

	if (encoder)
		encoder->funcs->destroy(encoder);

	if (connector)
		connector->funcs->destroy(connector);
}

static const struct component_ops lcd_ops = {
	.bind = lcd_rgb_drm_bind,
	.unbind = lcd_rgb_drm_unbind,
};

static int lcd_rgb_drm_probe(struct platform_device *pdev)
{
	struct lcd_rgb_context *lcd;
	struct device *dev = &pdev->dev;
	struct nx_drm_display *display = &lcd_rgb_drm_display;

	DRM_DEBUG_KMS("enter\n");

	lcd = kzalloc(sizeof(*lcd), GFP_KERNEL);
	if (!lcd)
		return -ENOMEM;

	DRM_INIT_DISPLAY_DEV(display, dev);
	mutex_init(&lcd->lock);

	dev_set_drvdata(dev, lcd);
	component_add(dev, &lcd_ops);

	DRM_DEBUG_KMS("exit\n");

	return 0;
}

static int lcd_rgb_drm_remove(struct platform_device *pdev)
{
	kfree(dev_get_drvdata(&pdev->dev));
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int lcd_rgb_drm_suspend(struct device *dev)
{
	struct lcd_rgb_context *lcd = dev_get_drvdata(dev);

	return lcd_rgb_drm_power_on(lcd, false);
}

static int lcd_rgb_drm_resume(struct device *dev)
{
	struct lcd_rgb_context *lcd = dev_get_drvdata(dev);

	return lcd_rgb_drm_power_on(lcd, true);
}
#endif

static const struct dev_pm_ops lcd_rgb_drm_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(lcd_rgb_drm_suspend, lcd_rgb_drm_resume)
};

static const struct of_device_id lcd_rgb_drm_of_match[] = {
	{.compatible = "nexell,drm-s5p6818-lcd"},
	{}
};
MODULE_DEVICE_TABLE(of, lcd_rgb_drm_of_match);

struct platform_driver lcd_rgb_drm_driver = {
	.probe = lcd_rgb_drm_probe,
	.remove = lcd_rgb_drm_remove,
	.driver = {
		   .name = "nexell-drm-lcd",
		   .owner = THIS_MODULE,
		   .of_match_table = lcd_rgb_drm_of_match,
		   .pm = &lcd_rgb_drm_pm,
	},
};
module_platform_driver(lcd_rgb_drm_driver);

MODULE_AUTHOR("jhkim <jhkim@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell LCD DRM Driver");
MODULE_LICENSE("GPL");
