/*
UEI_IRRC_DRIVER_FOR_NX4418
*/

#include <linux/err.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/types.h>
#include <linux/time.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>

#include "kp_uei_irrc.h"

struct uei_irrc_pdata_type irrc_data;

#ifdef CONFIG_OF
static int irrc_parse_dt(struct device *dev, struct uei_irrc_pdata_type *pdata)
{
	struct device_node *np = dev->of_node;
	pdata->reset_gpio = of_get_named_gpio(np, "uei,reset-gpio", 0);
	dev_err(dev, "IrRC: uei,reset-gpio: %d\n", pdata->reset_gpio);

	return 0;
}
#endif

static int uei_irrc_probe(struct platform_device *pdev)
{
	int rc = 0;

    dev_err(&pdev->dev, "%s begin\n", __func__);
	if (pdev->dev.of_node) {
		irrc_parse_dt(&pdev->dev, &irrc_data);
	}

	if ((!gpio_is_valid(irrc_data.reset_gpio))) {
		dev_err(&pdev->dev "IrRC: IrRC reset_gpio is invalid\n");
		return 1;
	}

	rc = gpio_request(irrc_data.reset_gpio, "irrc_reset_n");
	if (rc) {
		dev_err(&pdev->dev, "%s: irrc_reset_n %d request failed\n",
				__func__, irrc_data.reset_gpio);
		return rc;
	}

	rc = gpio_direction_output(irrc_data.reset_gpio, 0);
	if (rc) {
		dev_err(&pdev->dev, "%s: gpio_direction %d config is failed\n",
				__func__, irrc_data.reset_gpio);
		return rc;
	}

	gpio_set_value(irrc_data.reset_gpio, 1);

    dev_err(&pdev->dev, "%s end\n", __func__);
	return rc;
}

static int uei_irrc_remove(struct platform_device *pdev)
{
	struct uei_irrc_pdata_type *pdata = platform_get_drvdata(pdev);
	pdata = NULL;

	return 0;
}

static void uei_irrc_shutdown(struct platform_device *pdev)
{
	return;
}

static int uei_irrc_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int uei_irrc_resume(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id irrc_match_table[] = {
	{ .compatible = "uei,irrc",},
	{ },
};
#endif

static struct platform_driver uei_irrc_driver = {
	.probe = uei_irrc_probe,
	.remove = uei_irrc_remove,
	.shutdown = uei_irrc_shutdown,
	.suspend = uei_irrc_suspend,
	.resume = uei_irrc_resume,
	.driver = {
		.name = UEI_IRRC_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = irrc_match_table,
#endif
	},
};

static int __init uei_irrc_init(void)
{
	int ret = 0;
	ret = platform_driver_register(&uei_irrc_driver);

	if (ret) {
		printk( "%s: init fail\n", __func__);
	}

	return ret;
}

static void __exit uei_irrc_exit(void)
{
	platform_driver_unregister(&uei_irrc_driver);
}

module_init(uei_irrc_init);
module_exit(uei_irrc_exit);

MODULE_AUTHOR("KPVoice");
MODULE_DESCRIPTION("UEI IrRC Driver");
MODULE_LICENSE("GPL");
