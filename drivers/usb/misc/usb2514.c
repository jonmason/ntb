/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Hyunseok Jung <hsjung@nexell.co.kr>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/uaccess.h>

#define USB2514_RESET		0xff

enum usb2514_mode {
	USB2514_MODE_UNKNOWN,
	USB2514_MODE_HUB,
	USB2514_MODE_STANDBY,
};

struct usb2514_platform_data {
	enum usb2514_mode	initial_mode;
	int			gpio_reset;
};

struct usb2514 {
	enum usb2514_mode	mode;
	struct regmap		*regmap;
	struct device		*dev;
	int			gpio_reset;
	struct i2c_client	*client;
	u16			model;
};

struct i2cparam {
	u8 reg;
	u8 value;
};

struct i2cparam usb2514_dft_reg[] = {
	{0x00, 0x24}, {0x01, 0x04}, {0x02, 0x12}, {0x03, 0x25}, {0x04, 0xB3},
	{0x05, 0x0B}, {0x06, 0x9B}, {0x07, 0x20}, {0x08, 0x02},	{0x09, 0x00},
	{0x0A, 0x00}, {0x0B, 0x00}, {0x0C, 0x01}, {0x0D, 0x32},	{0x0E, 0x01},
	{0x0F, 0x32}, {0x10, 0x32}, {0xF6, 0x00}, {0xF8, 0x00},	{0xFB, 0x21},
	{0xFC, 0x43}, {0xFF, 0x01},
};

static int usb2514_reset(struct usb2514 *hub, int state)
{
	if (gpio_is_valid(hub->gpio_reset)) {
		if (state) {
			gpio_set_value(hub->gpio_reset, 1);
			udelay(10);
			gpio_set_value(hub->gpio_reset, 0);
			udelay(10);
			gpio_set_value(hub->gpio_reset, 1);
		} else
			gpio_set_value(hub->gpio_reset, 0);
	}

	return 0;
}

static int usb2514_connect(struct usb2514 *hub)
{
	struct device *dev = hub->dev;
	int err;
	int i;

	if (!i2c_check_functionality(hub->client->adapter,
					I2C_FUNC_SMBUS_READ_WORD_DATA))
		return -EIO;

	usb2514_reset(hub, 1);

	for (i = 0; i < (sizeof(usb2514_dft_reg)/sizeof(struct i2cparam));
	     i++) {
		err = i2c_smbus_write_block_data(hub->client,
						 usb2514_dft_reg[i].reg,
						 1,
						 &(usb2514_dft_reg[i].value));
	}

	hub->mode = USB2514_MODE_HUB;
	dev_info(dev, "switched to HUB mode\n");

	return 0;
}

static int usb2514_switch_mode(struct usb2514 *hub, enum usb2514_mode mode)
{
	struct device *dev = hub->dev;
	int err = 0;

	switch (mode) {
	case USB2514_MODE_HUB:
		err = usb2514_connect(hub);
		break;

	case USB2514_MODE_STANDBY:
		usb2514_reset(hub, 0);
		dev_info(dev, "switched to STANDBY mode\n");
		break;

	default:
		dev_err(dev, "unknown mode is requested\n");
		err = -EINVAL;
		break;
	}

	return err;
}

static const struct regmap_config usb2514_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = USB2514_RESET,
};

static int usb2514_probe(struct usb2514 *hub)
{
	struct device *dev = hub->dev;
	struct usb2514_platform_data *pdata = dev_get_platdata(dev);
	struct device_node *np = dev->of_node;
	int err;

	hub->mode = USB2514_MODE_HUB;

	if (pdata) {
		hub->gpio_reset		= pdata->gpio_reset;
	} else if (np) {
		hub->gpio_reset = of_get_named_gpio(np, "reset-gpios", 0);
		if (hub->gpio_reset == -EPROBE_DEFER)
			return -EPROBE_DEFER;
	}

	if (!hub->regmap)
		dev_err(dev, "Ports disabled with no control interface\n");

	if (gpio_is_valid(hub->gpio_reset)) {
		err = devm_gpio_request_one(dev, hub->gpio_reset,
				GPIOF_OUT_INIT_HIGH, "usb2514 reset");
		if (err) {
			dev_err(dev,
				"unable to request GPIO %d as reset pin (%d)\n",
				hub->gpio_reset, err);
			return err;
		}
	}

	usb2514_switch_mode(hub, hub->mode);

	dev_info(dev, "%s: probed in %s mode\n", __func__,
			(hub->mode == USB2514_MODE_HUB) ? "hub" : "standby");

	return 0;
}

static int usb2514_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct usb2514 *hub;
	int err;

	hub = devm_kzalloc(&i2c->dev, sizeof(struct usb2514), GFP_KERNEL);
	if (!hub)
		return -ENOMEM;

	i2c_set_clientdata(i2c, hub);

	hub->client = i2c;

	hub->regmap = devm_regmap_init_i2c(i2c, &usb2514_regmap_config);
	if (IS_ERR(hub->regmap)) {
		err = PTR_ERR(hub->regmap);
		dev_err(&i2c->dev, "Failed to initialise regmap: %d\n", err);
		return err;
	}
	hub->dev = &i2c->dev;

	return usb2514_probe(hub);
}

static int usb2514_platform_probe(struct platform_device *pdev)
{
	struct usb2514 *hub;

	hub = devm_kzalloc(&pdev->dev, sizeof(struct usb2514), GFP_KERNEL);
	if (!hub)
		return -ENOMEM;
	hub->dev = &pdev->dev;

	return usb2514_probe(hub);
}

#ifdef CONFIG_PM_SLEEP
static int usb2514_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct usb2514 *hub = i2c_get_clientdata(client);

	usb2514_switch_mode(hub, USB2514_MODE_STANDBY);

	return 0;
}

static int usb2514_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct usb2514 *hub = i2c_get_clientdata(client);

	usb2514_switch_mode(hub, hub->mode);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(usb2514_i2c_pm_ops, usb2514_i2c_suspend,
		usb2514_i2c_resume);

static const struct i2c_device_id usb2514_id[] = {
	{ "usb2514", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, usb2514_id);

#ifdef CONFIG_OF
static const struct of_device_id usb2514_of_match[] = {
	{ .compatible = "smsc,usb2514", },
	{},
};
MODULE_DEVICE_TABLE(of, usb2514_of_match);
#endif

static struct i2c_driver usb2514_i2c_driver = {
	.driver = {
		.name	= "usb2514",
		.pm = &usb2514_i2c_pm_ops,
		.of_match_table = of_match_ptr(usb2514_of_match),
	},
	.probe		= usb2514_i2c_probe,
	.id_table	= usb2514_id,
};

static struct platform_driver usb2514_platform_driver = {
	.driver = {
		.name = "usb2514",
		.of_match_table = of_match_ptr(usb2514_of_match),
	},
	.probe		= usb2514_platform_probe,
};

static int __init usb2514_init(void)
{
	int err;

	err = i2c_register_driver(THIS_MODULE, &usb2514_i2c_driver);
	if (err != 0)
		pr_err("usb2514: Failed to register I2C driver: %d\n", err);

	err = platform_driver_register(&usb2514_platform_driver);
	if (err != 0)
		pr_err("usb2514: Failed to register platform driver: %d\n",
		       err);

	return 0;
}
module_init(usb2514_init);

static void __exit usb2514_exit(void)
{
	platform_driver_unregister(&usb2514_platform_driver);
	i2c_del_driver(&usb2514_i2c_driver);
}
module_exit(usb2514_exit);

MODULE_AUTHOR("Hyunseok Jung <hsjung@nexell.co.kr>");
MODULE_DESCRIPTION("USB2514 USB HUB driver");
MODULE_LICENSE("GPL");
