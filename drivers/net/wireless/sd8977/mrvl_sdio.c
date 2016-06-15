/** @file mrvl_sdio.c
 *
 *  @brief This file contains SDIO interrupt related functions.
 *
 * Copyright (C) 2008-2015, Marvell International Ltd.
 *
 * This software file (the "File") is distributed by Marvell International
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991
 * (the "License").  You may use, redistribute and/or modify this File in
 * accordance with the terms and conditions of the License, a copy of which
 * is available by writing to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
 * worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
 * this warranty disclaimer.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/card.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif

int oob_gpio = -1;
static unsigned int oob_irq;
static unsigned int num_sdio_irqs = 0;
static struct task_struct *sdio_irq_thread = NULL;
static int irq_enabled = 0;
static int irq_registered = 0;

#define MRVL_MAX_SDIO_FUNC 2
static int suspended[MRVL_MAX_SDIO_FUNC] = {0, };

static int
mrvl_sdio_is_suspended(void)
{
	int i;

	for (i = 0; i < MRVL_MAX_SDIO_FUNC; i++) {
		if (!suspended[i])
			return 0;
	}

	return 1;
}

#ifdef CONFIG_OF
static void
mrvl_get_gpio_from_dev_tree(void)
{
	struct device_node *dt_node = NULL;

	dt_node = of_find_node_by_name(NULL, "sd8x-rfkill");
	if (!dt_node) return;

	oob_gpio = of_get_named_gpio(dt_node, "gpios", 0);
}
#endif

static irqreturn_t
mrvl_sdio_irq(int irq, void *dev_id)
{
	disable_irq_nosync(oob_irq);
	irq_enabled = 0;

	wake_up_process(sdio_irq_thread);

	return IRQ_HANDLED;
}

static int
mrvl_sdio_irq_register(struct mmc_card *card)
{
	int ret = 0;

	ret = request_irq(oob_irq, mrvl_sdio_irq,
			IRQF_TRIGGER_LOW | IRQF_SHARED,
			"mrvl_sdio_irq", card);

	if (!ret) {
		irq_registered = 1;
		irq_enabled = 1;
		enable_irq_wake(oob_irq);
	}

	return ret;
}

static void
mrvl_sdio_irq_unregister(struct mmc_card *card)
{
	if (irq_registered) {
		irq_registered = 0;
		disable_irq_wake(oob_irq);
		if (irq_enabled) {
			disable_irq(oob_irq);
			irq_enabled = 0;
		}
		free_irq(oob_irq, card);
	}
}

static int
mrvl_sdio_irq_thread(void *_card)
{
	struct mmc_card *card = _card;
	struct sdio_func *func;
	unsigned char pending;
	int i;
	int ret;
	struct sched_param param = { .sched_priority = 1 };

	sched_setscheduler(current, SCHED_FIFO, &param);

	do {
		func = NULL;
		for (i = 0; i < card->sdio_funcs; i++) {
			if (card->sdio_func[i]) {
				func = card->sdio_func[i];
				break;
			}
		}
		if (!func)
			break;

		sdio_claim_host(func);
		pending = sdio_f0_readb(func, SDIO_CCCR_INTx, &ret);

		if (!ret && pending) {
			for (i = 1; i <=2; i++) {
				if (pending & (1 << i)) {
					func = card->sdio_func[i-1];
					if (func && func->irq_handler)
						func->irq_handler(func);
				}
			}
		}
		sdio_release_host(func);

		if (ret) {
			set_current_state(TASK_INTERRUPTIBLE);
			if (!kthread_should_stop())
				schedule_timeout(HZ);
			set_current_state(TASK_RUNNING);
		}

		set_current_state(TASK_INTERRUPTIBLE);
		if (irq_registered && !irq_enabled && !mrvl_sdio_is_suspended())
		{
			irq_enabled = 1;
			enable_irq(oob_irq);
		}
		if (!kthread_should_stop())
			schedule();
		set_current_state(TASK_RUNNING);

	} while(!kthread_should_stop());

	return 0;
}

static int
mvrl_sdio_func_intr_enable(struct sdio_func *func, sdio_irq_handler_t *handler)
{
	int ret;
	unsigned char reg;

#ifdef MMC_QUIRK_LENIENT_FN0
	func->card->quirks |= MMC_QUIRK_LENIENT_FN0;
#endif
	reg = sdio_f0_readb(func, SDIO_CCCR_IENx, &ret);
	if (ret)
		return ret;
	reg |= 1 << func->num;
	reg |= 1;
	sdio_f0_writeb(func, reg, SDIO_CCCR_IENx, &ret);
	if (ret)
		return ret;

	func->irq_handler = handler;
	suspended[func->num-1] = 0;

	return ret;
}

static int
mrvl_sdio_func_intr_disable(struct sdio_func *func)
{
	int ret;
	unsigned char reg;

#ifdef MMC_QUIRK_LENIENT_FN0
	func->card->quirks |= MMC_QUIRK_LENIENT_FN0;
#endif
	if (func->irq_handler)
		func->irq_handler = NULL;

	reg = sdio_f0_readb(func, SDIO_CCCR_IENx, &ret);
	if (ret)
		return ret;
	reg &= ~(1 << func->num);
	if (!(reg & 0xFE))
		reg = 0;
	sdio_f0_writeb(func, reg, SDIO_CCCR_IENx, &ret);

	return ret;
}

int
mrvl_sdio_claim_irq(struct sdio_func *func, sdio_irq_handler_t *handler)
{
	int ret;

	BUG_ON(!func);
	BUG_ON(!func->card);

	if (!num_sdio_irqs++) {
		sdio_irq_thread = kthread_run(mrvl_sdio_irq_thread, func->card,
				              "mrvl_sdio_irq");
		if (IS_ERR(sdio_irq_thread)) {
			num_sdio_irqs--;
			return -1;
		}
		ret = mrvl_sdio_irq_register(func->card);
		if (ret) {
			num_sdio_irqs--;
			kthread_stop(sdio_irq_thread);
			return ret;
		}
	}

	ret = mvrl_sdio_func_intr_enable(func, handler);
	if (ret && !--num_sdio_irqs) {
		mrvl_sdio_irq_unregister(func->card);
		kthread_stop(sdio_irq_thread);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(mrvl_sdio_claim_irq);

int
mrvl_sdio_release_irq(struct sdio_func *func)
{
	BUG_ON(!func);
	BUG_ON(!func->card);

	if (!num_sdio_irqs)
		return 0;

	if (!--num_sdio_irqs) {
		mrvl_sdio_irq_unregister(func->card);
		kthread_stop(sdio_irq_thread);
	}

	mrvl_sdio_func_intr_disable(func);

	return 0;
}
EXPORT_SYMBOL_GPL(mrvl_sdio_release_irq);

int
mrvl_sdio_suspend(struct sdio_func *func)
{
	suspended[func->num-1] = 1;

	if (mrvl_sdio_is_suspended() && irq_enabled) {
		disable_irq_nosync(oob_irq);
		irq_enabled = 0;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(mrvl_sdio_suspend);

int
mrvl_sdio_resume(struct sdio_func *func)
{
	if (mrvl_sdio_is_suspended()) {
		suspended[func->num-1] = 0;
		wake_up_process(sdio_irq_thread);
	} else {
		suspended[func->num-1] = 0;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(mrvl_sdio_resume);

static int
mrvl_sdio_init_module(void)
{
#ifdef CONFIG_OF
	if (oob_gpio < 0)
		mrvl_get_gpio_from_dev_tree();
#endif
	if (!gpio_is_valid(oob_gpio)) {
		return -1;
	}
	if (gpio_request(oob_gpio, "mrvl_oob_gpio")) {
		return -1;
	}
	gpio_direction_input(oob_gpio);
	oob_irq = gpio_to_irq(oob_gpio);

	return 0;
}

static void
mrvl_sdio_cleanup_module(void)
{
	if (gpio_is_valid(oob_gpio))
		gpio_free(oob_gpio);
}

module_init(mrvl_sdio_init_module);
module_exit(mrvl_sdio_cleanup_module);

module_param(oob_gpio, int, 0660);
MODULE_PARM_DESC(oob_gpio, "SDIO OOB interrupt gpio");

MODULE_DESCRIPTION("MRVL SDIO Driver");
MODULE_AUTHOR("Marvell International Ltd.");
MODULE_VERSION("0.3");
MODULE_LICENSE("GPL");
