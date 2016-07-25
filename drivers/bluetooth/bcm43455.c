/*
 * Bluetooth Broadcom GPIO and Low Power Mode control
 *
 *  Copyright (C) 2011 Samsung Electronics Co., Ltd.
 *  Copyright (C) 2011 Google, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/serial_core.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif
#include <linux/of_gpio.h>
#include <linux/of.h>
#ifdef CONFIG_SOC_EXYNOS5422
#include <linux/serial_s3c.h>
#endif
#include <linux/gpio.h>

#define BT_LPM_ENABLE
#define FORCE_ENABLE_BT_WAKE

#ifdef CONFIG_SOC_EXYNOS5422
#define BT_UPORT 2
#else
#define BT_UPORT 3
#endif

#ifdef CONFIG_SOC_EXYNOS5422
extern s3c_wake_peer_t s3c2410_serial_wake_peer[CONFIG_SERIAL_SAMSUNG_UARTS];
#endif

static struct rfkill *bt_rfkill;
#ifdef BT_LPM_ENABLE
static int bt_wake_state = -1;
#endif

struct bcm_bt_lpm {
	int host_wake;
	int dev_wake;

	struct hrtimer enter_lpm_timer;
	ktime_t enter_lpm_delay;

	struct uart_port *uport;

#ifdef CONFIG_HAS_WAKELOCK
	struct wake_lock host_wake_lock;
	struct wake_lock bt_wake_lock;
#endif
} bt_lpm;

struct bcm_bt_gpio {
	int bt_en;
	int bt_wake;
	int bt_hostwake;
	int irq;
} bt_gpio;

int bt_is_running;
unsigned int is_inverted_power;

int check_bt_op(void)
{
	return bt_is_running;
}
EXPORT_SYMBOL(check_bt_op);

static int bcm43455_bt_rfkill_set_power(void *data, bool blocked)
{
	/* rfkill_ops callback. Turn transmitter on when blocked is false */
	if (!blocked) {
		pr_info("[BT] Bluetooth Power On.\n");

#ifdef BT_LPM_ENABLE
		if (irq_set_irq_wake(bt_gpio.irq, 1)) {
			pr_err("[BT] Set_irq_wake failed.\n");
			return -1;
		}
#endif
		if (is_inverted_power) {
			pr_info("%s: run inverted power control (onoff=0)\n",
				__func__);
			gpio_set_value(bt_gpio.bt_en, 0);
		} else {
			pr_info("%s: run normal power control (onoff=1)\n",
				__func__);
			gpio_set_value(bt_gpio.bt_en, 1);
		}
		bt_is_running = 1;
		msleep(100);

	} else {
		pr_info("[BT] Bluetooth Power Off.\n");

#ifdef BT_LPM_ENABLE
		if (is_inverted_power) {
			pr_info("%s: check inverted power control\n",
				__func__);
			if (!gpio_get_value(bt_gpio.bt_en) &&
					irq_set_irq_wake(bt_gpio.irq, 0)) {
				pr_err("[BT] Release_irq_wake failed.\n");
				return -1;
			}
		} else {
			pr_info("%s: check normal power control\n",
				__func__);
			if (gpio_get_value(bt_gpio.bt_en) &&
					irq_set_irq_wake(bt_gpio.irq, 0)) {
				pr_err("[BT] Release_irq_wake failed.\n");
				return -1;
			}
		}
#endif
		bt_is_running = 0;
		if (is_inverted_power) {
			pr_info("%s: run inverted power control (onoff=1)\n",
				__func__);
			gpio_set_value(bt_gpio.bt_en, 1);
		} else {
			pr_info("%s: run normal power control (onoff=0)\n",
				__func__);
			gpio_set_value(bt_gpio.bt_en, 0);
		}
	}
	return 0;
}

static const struct rfkill_ops bcm43455_bt_rfkill_ops = {
	.set_block = bcm43455_bt_rfkill_set_power,
};

#ifdef BT_LPM_ENABLE
static void set_wake_locked(int wake)
{
#ifdef CONFIG_BT_UART_IN_AUDIO
	struct uart_port *port = bt_lpm.uport;
#endif
#ifdef CONFIG_HAS_WAKELOCK
	if (wake)
		wake_lock(&bt_lpm.bt_wake_lock);
#endif
	gpio_set_value(bt_gpio.bt_wake, wake);
	bt_lpm.dev_wake = wake;

	if (bt_wake_state != wake) {
#ifdef CONFIG_BT_UART_IN_AUDIO
		if (bt_lpm.host_wake)
			if (wake)
				port->ops->set_wake(port, wake);
		else
			port->ops->set_wake(port, wake);
#endif
		pr_debug("[BT] set_wake_locked value = %d\n", wake);
		bt_wake_state = wake;
	}
}

static enum hrtimer_restart enter_lpm(struct hrtimer *timer)
{
	if (bt_lpm.uport != NULL)
		set_wake_locked(0);

	bt_is_running = 0;

#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_timeout(&bt_lpm.bt_wake_lock, HZ/2);
#endif

	return HRTIMER_NORESTART;
}

void bcm_bt_lpm_exit_lpm_locked(struct uart_port *uport)
{
	bt_lpm.uport = uport;

	hrtimer_try_to_cancel(&bt_lpm.enter_lpm_timer);
	bt_is_running = 1;
	set_wake_locked(1);

	hrtimer_start(&bt_lpm.enter_lpm_timer, bt_lpm.enter_lpm_delay,
		HRTIMER_MODE_REL);
}

static void update_host_wake_locked(int host_wake)
{
	if (host_wake == bt_lpm.host_wake)
		return;

	bt_lpm.host_wake = host_wake;

	bt_is_running = 1;

	if (host_wake) {
#ifdef CONFIG_HAS_WAKELOCK
		wake_lock(&bt_lpm.host_wake_lock);
#endif
	} else  {
		/* Take a timed wakelock, so that upper layers can take it.
		 * The chipset deasserts the hostwake lock, when there is no
		 * more data to send.
		 */
#ifdef CONFIG_HAS_WAKELOCK
		pr_err("[BT]update_host_wake_locked: deasserted host_wake\n");
		pr_err("release wakelock in 1s\n");
		wake_lock_timeout(&bt_lpm.host_wake_lock, HZ/2);
#endif
	}
}

static irqreturn_t host_wake_isr(int irq, void *dev)
{
#ifdef CONFIG_BT_UART_IN_AUDIO
	struct uart_port *port = bt_lpm.uport;
#endif
	int host_wake;

	host_wake = gpio_get_value(bt_gpio.bt_hostwake);
	irq_set_irq_type(irq, host_wake ? IRQF_TRIGGER_FALLING :
					  IRQF_TRIGGER_RISING);

	if (!bt_lpm.uport) {
		bt_lpm.host_wake = host_wake;
		return IRQ_HANDLED;
	}

#ifdef CONFIG_BT_UART_IN_AUDIO
	if (bt_lpm.dev_wake)
		if (host_wake)
			port->ops->set_wake(port, host_wake);
	else
		port->ops->set_wake(port, host_wake);
#endif
	update_host_wake_locked(host_wake);

	return IRQ_HANDLED;
}

static int bcm_bt_lpm_init(struct platform_device *pdev)
{
	int ret;

	hrtimer_init(&bt_lpm.enter_lpm_timer, CLOCK_MONOTONIC,
			HRTIMER_MODE_REL);
	bt_lpm.enter_lpm_delay = ktime_set(5, 0);  /* 1 sec */ /*1->3*//*3->4*/
	bt_lpm.enter_lpm_timer.function = enter_lpm;

	bt_lpm.host_wake = 0;

#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_init(&bt_lpm.host_wake_lock, WAKE_LOCK_SUSPEND,
			 "BT_host_wake");
	wake_lock_init(&bt_lpm.bt_wake_lock, WAKE_LOCK_SUSPEND,
			 "BT_bt_wake");
#endif
#ifdef CONFIG_SOC_EXYNOS5422
	s3c2410_serial_wake_peer[BT_UPORT] =
		(s3c_wake_peer_t)bcm_bt_lpm_exit_lpm_locked;
#endif
	bt_gpio.irq = gpio_to_irq(bt_gpio.bt_hostwake);
	ret = devm_request_irq(&pdev->dev, bt_gpio.irq, host_wake_isr,
			IRQF_TRIGGER_RISING, "bt_host_wake", NULL);
	if (ret) {
		pr_err("[BT] Request_host wake irq failed.\n");
		return ret;
	}

	return 0;
}
#endif

static int bcm43455_bluetooth_probe(struct platform_device *pdev)
{
	int rc = 0;

	bt_gpio.bt_en = of_get_gpio(pdev->dev.of_node, 0);
	if (bt_gpio.bt_en < 0) {
		dev_err(&pdev->dev, "cannot parse bt_en gpio\n");
		return -ENODEV;
	}

	if (of_property_read_u32(pdev->dev.of_node, "inverted-power-control",
				&is_inverted_power)) {
		pr_info("%s: inverted-power-control property not found",
				  __func__);
		is_inverted_power = 0;
	}

	rc = devm_gpio_request(&pdev->dev, bt_gpio.bt_en, "bten_gpio");
	if (rc) {
		dev_err(&pdev->dev, "bt_gpio.bt_en request failed.\n");
		return rc;
	}

	bt_gpio.bt_wake = of_get_gpio(pdev->dev.of_node, 1);
	if (bt_gpio.bt_wake < 0) {
		dev_err(&pdev->dev, "cannot parse bt_wake gpio\n");
		return -ENODEV;
	}

	rc = devm_gpio_request(&pdev->dev, bt_gpio.bt_wake, "btwake_gpio");
	if (rc) {
		dev_err(&pdev->dev, "bt_gpio.bt_wake request failed.\n");
		return rc;
	}

	bt_gpio.bt_hostwake = of_get_gpio(pdev->dev.of_node, 2);
	if (bt_gpio.bt_hostwake < 0) {
		dev_err(&pdev->dev, "cannot parse bt_hostwake gpio\n");
		return -ENODEV;
	}

	rc = devm_gpio_request(&pdev->dev, bt_gpio.bt_hostwake,
				"bthostwake_gpio");

	if (rc) {
		dev_err(&pdev->dev, "bt_gpio.bt_hostwake request failed.\n");
		return rc;
	}

	gpio_direction_input(bt_gpio.bt_hostwake);
#ifdef FORCE_ENABLE_BT_WAKE
	gpio_direction_output(bt_gpio.bt_wake, 1);
#else
	gpio_direction_output(bt_gpio.bt_wake, 0);
#endif
	if (is_inverted_power) {
		 pr_info("%s: run inverted power control (onoff=1)\n",
				   __func__);
		 gpio_direction_output(bt_gpio.bt_en, 1);
	} else {
		 pr_info("%s: run normal power control (onoff=0)\n",
				   __func__);
		 gpio_direction_output(bt_gpio.bt_en, 0);
	}

#ifdef BT_LPM_ENABLE
	rc = bcm_bt_lpm_init(pdev);
	if (rc) {
		dev_err(&pdev->dev, "lpm init failed\n");
		return rc;
	}
#endif
	bt_rfkill = rfkill_alloc("bcm43455 Bluetooth", &pdev->dev,
				RFKILL_TYPE_BLUETOOTH, &bcm43455_bt_rfkill_ops,
				NULL);
	if (unlikely(!bt_rfkill)) {
		dev_err(&pdev->dev, "bt_rfkill alloc failed.\n");
		return -ENOMEM;
	}

	rfkill_init_sw_state(bt_rfkill, 0);

	rc = rfkill_register(bt_rfkill);
	if (unlikely(rc)) {
		dev_err(&pdev->dev, "bt_rfkill register failed.\n");
		rfkill_destroy(bt_rfkill);
		return -ENODEV;
	}

	rfkill_set_sw_state(bt_rfkill, true);

	dev_info(&pdev->dev, "bcm43455_bluetooth_probe End\n");
	return 0;
}

static int bcm43455_bluetooth_remove(struct platform_device *pdev)
{
	rfkill_unregister(bt_rfkill);
	rfkill_destroy(bt_rfkill);

#ifdef BT_LPM_ENABLE
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_destroy(&bt_lpm.host_wake_lock);
	wake_lock_destroy(&bt_lpm.bt_wake_lock);
#endif
#endif
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id artik_bluetooth_match[] = {
	{
		.compatible = "broadcom,bcm43455bt",
	},
	{},
};
MODULE_DEVICE_TABLE(of, artik_bluetooth_match);

static struct platform_driver bcm43455_bluetooth_platform_driver = {
	.probe = bcm43455_bluetooth_probe,
	.remove = bcm43455_bluetooth_remove,
	.driver = {
		   .name = "bcm43455_bluetooth",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(artik_bluetooth_match),
		   },
};

module_platform_driver(bcm43455_bluetooth_platform_driver);
#endif
MODULE_ALIAS("platform:bcm43455");
MODULE_DESCRIPTION("bcm43455_bluetooth");
MODULE_LICENSE("GPL");
