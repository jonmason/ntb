/*
 *  S3T9MF - Samsung Secure Element driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/pwm.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/stat.h>

struct s3t9mf_chip {
	struct device *se_dev;
	struct pwm_device *pwm;
	struct gpio_desc *reset_gpio;
	int status;
};

static struct class *sedev_class;

static ssize_t s3t9mf_sysfs_se_enable(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	int value;
	int pwm_period;
	struct s3t9mf_chip *chip = dev_get_drvdata(dev);

	ret = kstrtoint(buf, 10, &value);
	if (ret)
		return -EINVAL;

	if (value == chip->status)
		return 0;

	if (value) {
		pwm_period = pwm_get_period(chip->pwm);
		ret = pwm_config(chip->pwm, pwm_period/2, pwm_period);
		if (ret) {
			dev_err(chip->se_dev, "pwm config fail %d\n", ret);
			return -EIO;
		}

		ret = pwm_enable(chip->pwm);
		if (ret) {
			dev_err(chip->se_dev, "pwm enable fail %d\n", ret);
			return -EIO;
		}

		gpiod_set_value(chip->reset_gpio, 1);
	} else {
		pwm_disable(chip->pwm);
		gpiod_set_value(chip->reset_gpio, 0);
	}

	chip->status = value;

	return count;
}

static ssize_t s3t9mf_sysfs_show_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct s3t9mf_chip *chip = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", chip->status);
}

static DEVICE_ATTR(se_enable, S_IRUGO | S_IWUSR,
		s3t9mf_sysfs_show_status, s3t9mf_sysfs_se_enable);

static const struct attribute *s3t9mf_attrs[] = {
	&dev_attr_se_enable.attr,
	NULL,
};

static const struct of_device_id s3t9mf_dt_ids[] = {
	{ .compatible = "samsung,s3t9mf" },
	{},
};
MODULE_DEVICE_TABLE(of, s3t9mf_dt_ids);

static int s3t9mf_gpio_init(struct device *dev,
		struct s3t9mf_chip *pdata)
{
	pdata->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(pdata->reset_gpio)) {
		dev_err(dev, "Cannot get reset-gpios %ld\n",
			PTR_ERR(pdata->reset_gpio));
		return PTR_ERR(pdata->reset_gpio);
	}

	return 0;
}

static int s3t9mf_probe(struct platform_device *pdev)
{
	int ret;
	struct s3t9mf_chip *chip;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	ret = s3t9mf_gpio_init(&pdev->dev, chip);
	if (ret) {
		dev_err(&pdev->dev, "Cannot initialize gpio\n");
		return -ENODEV;
	}

	chip->pwm = devm_pwm_get(&pdev->dev, "pwm_t9mf");
	if (IS_ERR(chip->pwm)) {
		dev_err(&pdev->dev, "unable to request PWM\n");
		return -ENODEV;
	}

	chip->se_dev = device_create(sedev_class,
		NULL, 0, "%s", "ssafelite");
	if (IS_ERR(chip->se_dev)) {
		dev_err(&pdev->dev, "failed to create se_dev\n");
		ret = ENODEV;
	}

	ret = sysfs_create_files(&chip->se_dev->kobj, s3t9mf_attrs);
	if (ret) {
		dev_err(&pdev->dev, "failed to create sysfs files\n");
		ret = -ENODEV;
		goto err_sysfs;
	}

	dev_set_drvdata(chip->se_dev, chip);

	dev_info(&pdev->dev, "platform driver %s registered\n", pdev->name);

	return 0;

err_sysfs:
	device_destroy(sedev_class, 0);

	return ret;
}

static int s3t9mf_remove(struct platform_device *pdev)
{
	struct s3t9mf_chip *chip = platform_get_drvdata(pdev);

	sysfs_remove_files(&chip->se_dev->kobj, s3t9mf_attrs);
	device_destroy(sedev_class, 0);

	return 0;
}

static struct platform_driver s3t9mf_driver = {
	.probe		= s3t9mf_probe,
	.remove		= s3t9mf_remove,
	.driver = {
		.name		= "samsung,s3t9mf",
		.owner		= THIS_MODULE,
		.of_match_table	= of_match_ptr(s3t9mf_dt_ids),
	},
};

static int __init s3t9mf_init(void)
{
	int ret;

	sedev_class = class_create(THIS_MODULE, "se");
	if (IS_ERR(sedev_class))
		return PTR_ERR(sedev_class);

	ret = platform_driver_register(&s3t9mf_driver);
	if (ret < 0)
		class_destroy(sedev_class);

	return ret;
}

static void __exit s3t9mf_exit(void)
{
	platform_driver_unregister(&s3t9mf_driver);
	class_destroy(sedev_class);
}

module_init(s3t9mf_init);
module_exit(s3t9mf_exit);

MODULE_AUTHOR("Minho Kim <minho715.kim@samsung.com>");
MODULE_DESCRIPTION("s3t9mf driver");
MODULE_LICENSE("GPL v2");
