/*
 * Bluetooth Broadcom GPIO and Low Power Mode control
 *
 * Copyright (C) Guangzhou FriendlyARM Computer Tech. Co., Ltd.
 * (http://www.friendlyarm.com)
 *
 * Copyright (C) 2011 Samsung Electronics Co., Ltd.
 * Copyright (C) 2011 Google, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/serial_core.h>

struct rfkill_bcm4343_data {
	const char *name;
	enum rfkill_type type;
	struct rfkill *rfkill_dev;
	bool is_running;

	struct gpio_desc *bt_reset;
	struct gpio_desc *bt_wake;
	struct gpio_desc *bt_hostwake;
	int irq;
};

static int bcm4343_rfkill_set_block(void *data, bool blocked)
{
	struct rfkill_bcm4343_data *priv = data;

	gpiod_set_value_cansleep(priv->bt_reset, !blocked);
	msleep(25);

	priv->is_running = !blocked;
	pr_info("bcm4343: rfkill set_block %d\n", blocked);

	return 0;
}

static const struct rfkill_ops bcm4343_rfkill_ops = {
	.set_block = bcm4343_rfkill_set_block,
};

static int bcm4343_rfkill_probe(struct platform_device *pdev)
{
	struct rfkill_bcm4343_data *priv;
	struct rfkill *rfkill;
	int ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->name = "bcm4343";
	priv->type = RFKILL_TYPE_BLUETOOTH;

	priv->bt_reset = devm_gpiod_get_optional(&pdev->dev, "reset",
			GPIOD_OUT_LOW);
	if (!priv->bt_reset) {
		dev_err(&pdev->dev, "failed to get GPIO for bt_reset\n");
		return -ENODEV;
	}

	priv->bt_wake = devm_gpiod_get_optional(&pdev->dev, "wake",
			GPIOD_OUT_LOW);
	priv->bt_hostwake = devm_gpiod_get_optional(&pdev->dev, "hostwake",
			GPIOD_IN);

	if (priv->bt_hostwake)
		priv->irq = gpiod_to_irq(priv->bt_hostwake);
	else
		priv->irq = -1;

	rfkill = rfkill_alloc("BCM4343 Bluetooth", &pdev->dev, priv->type,
			&bcm4343_rfkill_ops, priv);
	if (unlikely(!rfkill)) {
		return -ENOMEM;
	}

	ret = rfkill_register(rfkill);
	if (unlikely(ret)) {
		dev_err(&pdev->dev, "Cannot register rfkill device\n");
		rfkill_destroy(rfkill);
		return -ENODEV;
	}

	priv->rfkill_dev = rfkill;
	platform_set_drvdata(pdev, priv);

	dev_info(&pdev->dev, "%s device registered.\n", priv->name);

	return 0;
}

static int bcm4343_rfkill_remove(struct platform_device *pdev)
{
	struct rfkill_bcm4343_data *priv = platform_get_drvdata(pdev);

	rfkill_unregister(priv->rfkill_dev);
	rfkill_destroy(priv->rfkill_dev);

	return 0;
}

static const struct of_device_id bcm4343_of_match[] = {
	{ .compatible = "broadcom,bcm4343", },
	{},
};
MODULE_DEVICE_TABLE(of, bcm4343_of_match);

static struct platform_driver bcm4343_rfkill_driver = {
	.probe = bcm4343_rfkill_probe,
	.remove = bcm4343_rfkill_remove,
	.driver = {
		.name = "rfkill-bcm4343",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(bcm4343_of_match),
	},
};

module_platform_driver(bcm4343_rfkill_driver);

MODULE_ALIAS("platform:bcm4343");
MODULE_AUTHOR("support@friendlyarm.com");
MODULE_DESCRIPTION("BCM4343 rfkill driver for Broadcom BCM4343X chipset");
MODULE_LICENSE("GPL");
