#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/sec_sysfs.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>

static bool artik_zb_boot_enable = true;
module_param_named(zb_boot_enable, artik_zb_boot_enable, bool, 0);
MODULE_PARM_DESC(zb_boot_enable,
		 "Enable artik zigbee power during boot (default=enabled)");

static const char * const zb_supply_names[] = {
	"vdd_zb",
	NULL,
};

static inline int get_array_size(const char **arrays)
{
	int i = 0;

	while (arrays[i])
		i++;
	return i;
}

struct artik_zb_power_platform_data {
	const char **supply_names;
	struct regulator_bulk_data *supplies;
	struct device *sec_sysfs;
	int on;
	int recovery_mode;
	int num_supplies;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *bootloader_gpio;
};

static int artik_zb_power_control(struct artik_zb_power_platform_data *pdata,
				  int onoff)
{
	int ret;

	if (onoff == pdata->on)
		return 0;

	if (onoff) {
		ret = regulator_bulk_enable(pdata->num_supplies,
				pdata->supplies);
		gpiod_set_value(pdata->reset_gpio, 1);
		if (!IS_ERR_OR_NULL(pdata->bootloader_gpio))
			gpiod_set_value(pdata->bootloader_gpio, 1);
	} else {
		if (!IS_ERR_OR_NULL(pdata->bootloader_gpio))
			gpiod_set_value(pdata->bootloader_gpio, 0);
		gpiod_set_value(pdata->reset_gpio, 0);
		ret = regulator_bulk_disable(pdata->num_supplies,
				pdata->supplies);
	}

	pdata->on = onoff;

	return ret;
}

static int artik_zb_recovery_control(struct artik_zb_power_platform_data *pdata,
				  int onoff)
{
	int ret;

	if (onoff == pdata->recovery_mode)
		return 0;

	/* Turn off */
	gpiod_set_value(pdata->reset_gpio, 0);
	if (!IS_ERR_OR_NULL(pdata->bootloader_gpio))
		gpiod_set_value(pdata->bootloader_gpio, 0);

	/* EM358x chip needs 26usec hold time to reset device */
	udelay(30);

	if (onoff) {
		/* Go to recovery mode */
		gpiod_set_value(pdata->reset_gpio, 1);
	} else {
		/* Go to normal mode */
		if (!IS_ERR_OR_NULL(pdata->bootloader_gpio))
			gpiod_set_value(pdata->bootloader_gpio, 1);
		gpiod_set_value(pdata->reset_gpio, 1);
	}
	pdata->recovery_mode = onoff;

	return ret;
}

static ssize_t show_zb_power_status(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct artik_zb_power_platform_data *pdata = dev_get_drvdata(dev);
	int ret;

	ret = sprintf(buf, "%d\n", pdata->on);

	return ret;
}

static ssize_t set_zb_power_status(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct artik_zb_power_platform_data *pdata = dev_get_drvdata(dev);
	int val = 0;
	int ret;

	ret = kstrtoint(buf, 10, &val);
	if (ret)
		return -EINVAL;

	ret = artik_zb_power_control(pdata, !!val);
	if (ret)
		return -EINVAL;

	return count;	/* if success returns count, if failed returns - */
}

static ssize_t show_zb_recovery_status(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct artik_zb_power_platform_data *pdata = dev_get_drvdata(dev);
	int ret;

	ret = sprintf(buf, "%d\n", pdata->recovery_mode);

	return ret;
}

static ssize_t set_zb_recovery_status(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct artik_zb_power_platform_data *pdata = dev_get_drvdata(dev);
	int val = 0;
	int ret;

	ret = kstrtoint(buf, 10, &val);
	if (ret)
		return -EINVAL;

	ret = artik_zb_recovery_control(pdata, !!val);
	if (ret)
		return -EINVAL;

	return count;	/* if success returns count, if failed returns - */
}

static DEVICE_ATTR(recovery, S_IRUGO | S_IWUSR,
		show_zb_recovery_status,
		set_zb_recovery_status);

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR,
		show_zb_power_status,
		set_zb_power_status);

static struct attribute *artik_zb_power_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_recovery.attr,
	NULL
};

static struct attribute_group artik_zb_power_attr_group = {
	.attrs = artik_zb_power_attributes,
};

#ifdef CONFIG_OF
static const struct of_device_id artik_zb_power_match[] = {
	{
		.compatible = "samsung,artik_zb_power",
		.data = zb_supply_names,
	},
	{
		.compatible = "samsung,artik_zb_power_efr32",
		.data = zb_supply_names,
	},

	{ },
};
MODULE_DEVICE_TABLE(of, artik_zb_power_match);
#endif

static int artik_zb_power_gpio_init(struct device *dev,
		struct artik_zb_power_platform_data *pdata)
{
	struct device_node *np;
	pdata->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(pdata->reset_gpio)) {
		dev_err(dev, "cannot get reset-gpios %ld\n",
			PTR_ERR(pdata->reset_gpio));
		return PTR_ERR(pdata->reset_gpio);
	}

	np = of_find_compatible_node(NULL, NULL,
			  "samsung,artik_zb_power_efr32");
	if (np) {
		pdata->bootloader_gpio = NULL;
		return 0;
	}

	pdata->bootloader_gpio = devm_gpiod_get(dev, "bootloader",
			GPIOD_OUT_HIGH);
	if (IS_ERR(pdata->bootloader_gpio)) {
		dev_err(dev, "cannot get bootloader-gpios %ld\n",
			PTR_ERR(pdata->bootloader_gpio));
		return PTR_ERR(pdata->bootloader_gpio);
	}

	return 0;
}

static int artik_zb_power_probe(struct platform_device *pdev)
{
	struct artik_zb_power_platform_data *pdata;
	const struct of_device_id *match;
	int ret, i;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "device tree node not found\n");
		return -ENODEV;
	}

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	match = of_match_node(artik_zb_power_match, pdev->dev.of_node);
	if (!match) {
		dev_err(&pdev->dev, "Cannot match dt node\n");
		return -ENODEV;
	}

	pdata->supply_names = (const char **)match->data;
	pdata->num_supplies = get_array_size(pdata->supply_names);

	pdata->supplies = devm_kzalloc(&pdev->dev,
			sizeof(struct regulator_bulk_data) *
			pdata->num_supplies, GFP_KERNEL);
	if (!pdata->supplies) {
		dev_err(&pdev->dev, "Cannot allocate memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < pdata->num_supplies; i++)
		pdata->supplies[i].supply = pdata->supply_names[i];

	ret = devm_regulator_bulk_get(&pdev->dev, pdata->num_supplies,
			pdata->supplies);
	if (ret) {
		dev_err(&pdev->dev, "Failed to get regulators\n");
		return -ENODEV;
	}

	ret = artik_zb_power_gpio_init(&pdev->dev, pdata);
	if (ret)
		return -ENODEV;

	platform_set_drvdata(pdev, pdata);

	pdata->sec_sysfs = sec_device_create(pdata, "artik_zb_power");
	if (IS_ERR(pdata->sec_sysfs)) {
		dev_err(&pdev->dev, "Failed to create sec_sysfs device");
		return -ENODEV;
	}

	/* create sysfs */
	ret = sysfs_create_group(&pdata->sec_sysfs->kobj,
			&artik_zb_power_attr_group);
	if (ret) {
		dev_err(&pdev->dev, "failed to create sysfs group\n");
		ret = -ENODEV;
		goto err_sysfs;
	}

	if (artik_zb_boot_enable) {
		ret = artik_zb_power_control(pdata, 1);
		if (ret) {
			dev_err(&pdev->dev, "Failed to turn power on\n");
			ret = -ENODEV;
			goto err_control;
		}
	}

	pdata->recovery_mode = 0;
	dev_info(&pdev->dev, "platform driver %s registered\n", pdev->name);

	return 0;

err_control:
	sysfs_remove_group(&pdata->sec_sysfs->kobj,
			&artik_zb_power_attr_group);

err_sysfs:
	sec_device_destroy(pdata->sec_sysfs->devt);

	return ret;
}

static int artik_zb_power_remove(struct platform_device *pdev)
{
	struct artik_zb_power_platform_data *pdata =
				platform_get_drvdata(pdev);

	artik_zb_power_control(pdata, 0);

	sysfs_remove_group(&pdata->sec_sysfs->kobj,
			&artik_zb_power_attr_group);
	sec_device_destroy(pdata->sec_sysfs->devt);

	return 0;
}

static struct platform_driver artik_zb_power_driver = {
	.probe	= artik_zb_power_probe,
	.remove = artik_zb_power_remove,
	.driver = {
		.name	= "artik_zb_power",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(artik_zb_power_match),
	},
};

static int __init artik_zb_power_init(void)
{
	return platform_driver_register(&artik_zb_power_driver);
}

static void __exit artik_zb_power_exit(void)
{
	platform_driver_unregister(&artik_zb_power_driver);
}

module_init(artik_zb_power_init);
module_exit(artik_zb_power_exit);

MODULE_ALIAS("platform:artik_zb_power");
MODULE_DESCRIPTION("ARTIK Zigbee Power Control Driver");
MODULE_LICENSE("GPL");
