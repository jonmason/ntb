/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Hyejung, Kwon <cjscld15@nexell.co.kr>
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

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/regmap.h>
#include <linux/interrupt.h>
#include <linux/mfd/syscon.h>
#include <linux/platform_device.h>

#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_res_manager.h>

#define FIFO_CTRL			0x0000
#define FIFO_INT			0x0004

/* FIFO CTRL */
#define FIFO_REG_CLEAR_SHIFT		21
#define FIFO_REG_CLEAR_MASK		BIT(21)
#define FIFO_DIRTY_SHIFT		20
#define FIFO_DIRTY_MASK			BIT(20)
#define FIFO_DIRTY_CLR_SHIFT		19
#define FIFO_DIRTY_CLR_MASK		BIT(19)
#define FIFO_TZPROT_SHIFT		18
#define FIFO_TZPROT_MASK		BIT(18)
#define FIFO_BUFFERED_ENABLE_SHIFT	17
#define FIFO_BUFFERED_ENABLE_MASK	BIT(17)
#define FIFO_ENABLE_SHIFT		16
#define FIFO_ENABLE_MASK		BIT(16)
#define FIFO_IDLE_SHIFT			15
#define FIFO_IDLE_MASK			BIT(15)
#define FIFO_INIT_SHIFT			14
#define FIFO_INIT_MASK			BIT(14)
#define FIFO_TID_SHIFT			0
#define FIFO_TID_MASK			GENMASK(13, 0)

/* FIFO INT */
#define FIFO_IDLE_INTDISABLE_SHIFT	6
#define FIFO_IDLE_INTDISABLE_MASK	BIT(6)
#define FIFO_IDLE_INTENB_SHIFT		5
#define FIFO_IDLE_INTENB_MASK		BIT(5)
#define FIFO_IDLE_INTPEND_CLR_SHIFT	4
#define FIFO_IDLE_INTPEND_CLR_MASK	BIT(4)
#define FIFO_UPDATE_INTDISABLE_SHIFT	2
#define FIFO_UPDATE_INTDISABLE_MASK	BIT(2)
#define FIFO_UPDATE_INTENB_SHIFT	1
#define FIFO_UPDATE_INTENB_MASK		BIT(1)
#define FIFO_UPDATE_INTPEND_CLR_SHIFT	0
#define FIFO_UPDATE_INTPEND_CLR_MASK	BIT(0)

struct nxs_fifo {
	struct nxs_dev nxs_dev;
	struct regmap *reg;
	u32 offset;
	u32 irq;
};

#define nxs_to_fifo(dev)	container_of(dev, struct nxs_fifo, nxs_dev)

static enum nxs_event_type
fifo_get_interrupt_pending_number(struct nxs_fifo *fifo);
static u32 fifo_get_interrupt_pending(const struct nxs_dev *pthis,
				     enum nxs_event_type type);
static void fifo_clear_interrupt_pending(const struct nxs_dev *pthis,
					enum nxs_event_type type);

static irqreturn_t fifo_irq_handler(void *priv)
{
	struct nxs_fifo *fifo = (struct nxs_fifo *)priv;
	struct nxs_dev *nxs_dev = &fifo->nxs_dev;
	struct nxs_irq_callback *callback;
	unsigned long flags;

	dev_info(nxs_dev->dev, "[%s] pending status = 0x%x\n", __func__,
		 fifo_get_interrupt_pending(nxs_dev, NXS_EVENT_ALL));

	fifo_clear_interrupt_pending(nxs_dev,
				    fifo_get_interrupt_pending_number(fifo));

	spin_lock_irqsave(&nxs_dev->irq_lock, flags);
	list_for_each_entry(callback, &nxs_dev->irq_callback, list) {
		if (callback)
			callback->handler(nxs_dev, callback->data);
	}
	spin_unlock_irqrestore(&nxs_dev->irq_lock, flags);
	return IRQ_HANDLED;
}

static bool fifo_check_busy(struct nxs_fifo *fifo)
{
	u32 idle = 0;

	regmap_read(fifo->reg, fifo->offset + FIFO_CTRL, &idle);
	return ~(idle & FIFO_IDLE_MASK);
}

static void fifo_reset_register(struct nxs_fifo *fifo)
{
	dev_info(fifo->nxs_dev.dev, "[%s]\n", __func__);
	regmap_update_bits(fifo->reg, fifo->offset + FIFO_CTRL,
			   FIFO_REG_CLEAR_MASK,
			   1 << FIFO_REG_CLEAR_SHIFT);
}

static void fifo_set_enable(struct nxs_fifo *fifo, bool enable)
{
	dev_info(fifo->nxs_dev.dev, "[%s] - %s\n",
		 __func__, (enable) ? "enable" : "disable");

	regmap_update_bits(fifo->reg, fifo->offset + FIFO_CTRL,
			   FIFO_ENABLE_MASK,
			   enable << FIFO_ENABLE_SHIFT);

}

static enum nxs_event_type
fifo_get_interrupt_pending_number(struct nxs_fifo *fifo)
{
	enum nxs_event_type ret = NXS_EVENT_NONE;
	u32 pend_status = 0;

	dev_info(fifo->nxs_dev.dev, "[%s]\n", __func__);
	regmap_read(fifo->reg, fifo->offset + FIFO_INT, &pend_status);
	if (pend_status & FIFO_IDLE_INTPEND_CLR_MASK)
		ret |= NXS_EVENT_IDLE;
	if (pend_status & FIFO_UPDATE_INTPEND_CLR_MASK)
		ret |= NXS_EVENT_DONE;

	return ret;
}

static u32 fifo_get_status(struct nxs_fifo *fifo, u32 offset, u32 mask,
			   u32 shift)
{
	u32 status;

	regmap_read(fifo->reg, fifo->offset + offset, &status);
	return ((status & mask) >> shift);
}

static void fifo_dump_register(const struct nxs_dev *pthis)
{
	 struct nxs_fifo *fifo = nxs_to_fifo(pthis);

	 dev_info(pthis->dev, "============================================\n");
	 dev_info(pthis->dev, "Scaler[%d] Dump Register : 0x%8x\n",
		  pthis->dev_inst_index, fifo->offset);

	 dev_info(pthis->dev, "TID:%d, Enable:%d\n",
		 fifo_get_status(fifo,
				 FIFO_CTRL,
				 FIFO_TID_MASK,
				 FIFO_TID_SHIFT),
		 fifo_get_status(fifo,
				 FIFO_CTRL,
				 FIFO_ENABLE_MASK,
				 FIFO_ENABLE_SHIFT));

	 dev_info(pthis->dev, "Interrupt Status:0x%x\n",
		 fifo_get_status(fifo, FIFO_INT, 0xFFFF, 0));

	 dev_info(pthis->dev, "============================================\n");
}

static void fifo_set_interrupt_enable(const struct nxs_dev *pthis,
				      enum nxs_event_type type,
				      bool enable)
{
	struct nxs_fifo *fifo = nxs_to_fifo(pthis);
	u32 mask_v = 0;

	dev_info(pthis->dev, "[%s] %s\n", __func__,
		 (enable)?"enable":"disable");

	if (enable) {
		if (type & NXS_EVENT_IDLE)
			mask_v |= FIFO_IDLE_INTENB_MASK;
		if (type & NXS_EVENT_DONE)
			mask_v |= FIFO_UPDATE_INTENB_MASK;
	} else {
		if (type & NXS_EVENT_IDLE)
			mask_v |= FIFO_IDLE_INTDISABLE_MASK;
		if (type & NXS_EVENT_DONE)
			mask_v |= FIFO_UPDATE_INTDISABLE_MASK;
	}

	regmap_update_bits(fifo->reg, fifo->offset + FIFO_INT,
			   mask_v, mask_v);

}

static u32 fifo_get_interrupt_enable(const struct nxs_dev *pthis,
				     enum nxs_event_type type)
{
	struct nxs_fifo *fifo = nxs_to_fifo(pthis);
	u32     mask_v = 0, status = 0;

	dev_info(pthis->dev, "[%s] type:%d\n", __func__, type);

	if (type & NXS_EVENT_IDLE)
		mask_v |= FIFO_IDLE_INTENB_MASK;
	if (type & NXS_EVENT_DONE)
		mask_v |= FIFO_UPDATE_INTENB_MASK;

	regmap_read(fifo->reg, fifo->offset + FIFO_INT, &status);
	status &= mask_v;

	return status;
}

static u32 fifo_get_interrupt_pending(const struct nxs_dev *pthis,
				      enum nxs_event_type type)
{
	struct nxs_fifo *fifo = nxs_to_fifo(pthis);
	u32     mask_v = 0, pend_status = 0;

	dev_info(pthis->dev, "[%s] type:%d\n", __func__, type);
	if (type & NXS_EVENT_IDLE)
		mask_v |= FIFO_IDLE_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_DONE)
		mask_v |= FIFO_UPDATE_INTPEND_CLR_MASK;

	regmap_read(fifo->reg, fifo->offset + FIFO_INT, &pend_status);
	pend_status &= mask_v;

	return pend_status;
}

static void fifo_clear_interrupt_pending(const struct nxs_dev *pthis,
					 enum nxs_event_type type)
{
	struct nxs_fifo *fifo = nxs_to_fifo(pthis);
	u32     mask_v = 0, status = 0;

	dev_info(pthis->dev, "[%s] type:%d\n", __func__, type);
	if (type & NXS_EVENT_IDLE)
		mask_v |= FIFO_IDLE_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_DONE)
		mask_v |= FIFO_UPDATE_INTPEND_CLR_MASK;

	regmap_update_bits(fifo->reg, fifo->offset + FIFO_INT,
			   mask_v, mask_v);
}

static int fifo_open(const struct nxs_dev *pthis)
{
	struct nxs_fifo *fifo = nxs_to_fifo(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	if (nxs_dev_get_open_count(&fifo->nxs_dev) == 0) {
		struct clk *clk;
		int ret;
		char dev_name[10] = {0, };

		sprintf(dev_name, "fifo%d", pthis->dev_inst_index);
		clk = clk_get(pthis->dev, dev_name);
		if (IS_ERR(clk)) {
			dev_err(pthis->dev, "controller clock not found\n");
			return -ENODEV;
		}
		ret = clk_prepare_enable(clk);
		if (ret) {
			dev_err(pthis->dev,
				"clock failed to prepare enable :%d\n", ret);
				return ret;
		}
		ret = request_irq(fifo->irq, fifo_irq_handler,
				  IRQF_TRIGGER_NONE,
				  "nxs-fifo", fifo);
		if (ret < 0) {
			dev_err(pthis->dev, "failed to request irq : %d\n",
				ret);
			return ret;
		}
	}
	nxs_dev_inc_open_count(&fifo->nxs_dev);

	return 0;
}

static int fifo_close(const struct nxs_dev *pthis)
{
	struct nxs_fifo *fifo = nxs_to_fifo(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);

	if (fifo_check_busy(fifo)) {
		dev_err(pthis->dev, "fifo is busy\n");
		return -EBUSY;
	}

	nxs_dev_dec_open_count(&fifo->nxs_dev);
	if (nxs_dev_get_open_count(&fifo->nxs_dev) == 0) {
		struct clk *clk;
		char dev_name[10] = {0, };

		fifo_reset_register(fifo);
		free_irq(fifo->irq, fifo);

		/* clock disable */
		sprintf(dev_name, "fifo%d", pthis->dev_inst_index);
		clk = clk_get(pthis->dev, dev_name);
		if (IS_ERR(clk)) {
			dev_err(pthis->dev, "controller clock not found\n");
			return -ENODEV;
		}

		clk_disable_unprepare(clk);
	}

	return 0;
}

static int fifo_start(const struct nxs_dev *pthis)
{
	return 0;
}

static int fifo_stop(const struct nxs_dev *pthis)
{
	return 0;
}

static int fifo_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_fifo *fifo = nxs_to_fifo(pthis);

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	return regmap_update_bits(fifo->reg, fifo->offset + FIFO_CTRL,
				  FIFO_DIRTY_MASK,
				  1 << FIFO_DIRTY_SHIFT);
}

static int fifo_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_fifo *fifo = nxs_to_fifo(pthis);

	return regmap_update_bits(fifo->reg, fifo->offset + FIFO_CTRL,
				  FIFO_TID_MASK,
				  tid1 << FIFO_TID_SHIFT);
}

static int nxs_fifo_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_fifo *fifo;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	fifo = devm_kzalloc(&pdev->dev, sizeof(*fifo), GFP_KERNEL);
	if (!fifo)
		return -ENOMEM;

	nxs_dev = &fifo->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	fifo->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						    "syscon");
	if (IS_ERR(fifo->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(fifo->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	fifo->offset = res->start;
	fifo->irq = platform_get_irq(pdev, 0);

	dev_info(&pdev->dev, "offset=0x%x, irq = %d\n",
		 fifo->offset, fifo->irq);

	nxs_dev->set_interrupt_enable = fifo_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = fifo_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = fifo_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = fifo_clear_interrupt_pending;
	nxs_dev->open = fifo_open;
	nxs_dev->close = fifo_close;
	nxs_dev->start = fifo_start;
	nxs_dev->stop = fifo_stop;
	nxs_dev->set_dirty = fifo_set_dirty;
	nxs_dev->set_tid = fifo_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dump_register = fifo_dump_register;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, fifo);

	return 0;
}

static int nxs_fifo_remove(struct platform_device *pdev)
{
	struct nxs_fifo *fifo = platform_get_drvdata(pdev);

	if (fifo)
		unregister_nxs_dev(&fifo->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_fifo_match[] = {
	{ .compatible = "nexell,fifo-nxs-1.0", },
	{},
};

static struct platform_driver nxs_fifo_driver = {
	.probe	= nxs_fifo_probe,
	.remove	= nxs_fifo_remove,
	.driver	= {
		.name = "nxs-fifo",
		.of_match_table = of_match_ptr(nxs_fifo_match),
	},
};

static int __init nxs_fifo_init(void)
{
	return platform_driver_register(&nxs_fifo_driver);
}
/* subsys_initcall(nxs_fifo_init); */
fs_initcall(nxs_fifo_init);

static void __exit nxs_fifo_exit(void)
{
	platform_driver_unregister(&nxs_fifo_driver);
}
module_exit(nxs_fifo_exit);

MODULE_DESCRIPTION("Nexell Stream FIFO driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

