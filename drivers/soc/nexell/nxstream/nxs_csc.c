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
#include <linux/io.h>
#include <linux/regmap.h>
#include <linux/interrupt.h>
#include <linux/mfd/syscon.h>
#include <linux/platform_device.h>

#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_plane_format.h>
#include <linux/soc/nexell/nxs_res_manager.h>

#define CSC_DIRTYSET_OFFSET		0x0000
#define CSC_DIRTYCLR_OFFSET		0x0010
#define CSC0_DIRTY			BIT(14)
#define CSC1_DIRTY			BIT(13)
#define CSC2_DIRTY			BIT(12)
#define CSC3_DIRTY			BIT(11)
/* offset */
#define CSC_CFG_CTRL			0x0000
#define CSC_TID_CTRL			0x0004
#define CSC_ALPHA_CTRL			0x0008
#define CSC_INT_CTRL			0x0038
/* CSC CFG CTRL */
#define CSC_IDLE_SHIFT			3
#define CSC_IDLE_MASK			BIT(3)
#define CSC_REG_CLEAR_SHIFT		5
#define CSC_REG_CLEAR_MASK		BIT(5)
#define CSC_DEC_EN_SHIFT		0
#define CSC_DEC_EN_MASK			BIT(0)

/* CSC TID CTRL */
#define CSC_IMGTYPE_SHIFT		14
#define CSC_IMGTYPE_MASK		GENMASK(15, 14)
#define CSC_TID_SHIFT			0
#define CSC_TID_MASK			GENMASK(13, 0)

/* CSC ALPHA CTRL */
#define CSC_BYPASS_ALPHA_SHIFT		16
#define CSC_BYPASS_ALPHA_MASK		BIT(16)
#define CSC_USER_ALPHA_SHIFT		0
#define CSC_USER_ALPHA_MASK		GENMASK(11, 0)

/* CSC INT CTRL */
#define CSC_DEC_ERR_INTDISABLE_SHIFT	14
#define CSC_DEC_ERR_INTDISABLE_MASK	BIT(14)
#define CSC_DEC_ERR_INTENB_SHIFT	13
#define CSC_DEC_ERR_INTENB_MASK		BIT(13)
#define CSC_DEC_ERR_INTPEND_CLR_SHIFT	12
#define CSC_DEC_ERR_INTPEND_CLR_MASK	BIT(12)

#define CSC_DEC_SEC_INTDISABLE_SHIFT	10
#define CSC_DEC_SEC_INTDISABLE_MASK	BIT(10)
#define CSC_DEC_SEC_INTENB_SHIFT	9
#define CSC_DEC_SEC_INTENB_MASK		BIT(9)
#define CSC_DEC_SEC_INTPEND_CLR_SHIFT	8
#define CSC_DEC_SEC_INTPEND_CLR_MASK	BIT(8)

#define CSC_UPDATE_INTDISABLE_SHIFT	6
#define CSC_UPDATE_INTDISABLE_MASK	BIT(6)
#define CSC_UPDATE_INTENB_SHIFT		5
#define CSC_UPDATE_INTENB_MASK		BIT(5)
#define CSC_UPDATE_INTPEND_CLR_SHIFT	4
#define CSC_UPDATE_INTPEND_CLR_MASK	BIT(4)

#define CSC_IDLE_INTDISABLE_SHIFT	2
#define CSC_IDLE_INTDISABLE_MASK	BIT(2)
#define CSC_IDLE_INTENB_SHIFT		1
#define CSC_IDLE_INTENB_MASK		BIT(1)
#define CSC_IDLE_INTPEND_CLR_SHIFT	0
#define CSC_IDLE_INTPEND_CLR_MASK	BIT(0)

#define CSC_USER_DEF_ALPHA_VALUE	0x3FF
#define CSC_BYPASS_ALPHA_ENABLE		0

struct nxs_csc {
	struct nxs_dev nxs_dev;
	struct regmap *reg;
	u32 offset;
	u32 irq;
};

#define nxs_to_csc(dev)		container_of(dev, struct nxs_csc, nxs_dev)

static enum nxs_event_type
csc_get_interrupt_pending_number(struct nxs_csc *csc);
static u32 csc_get_interrupt_pending(const struct nxs_dev *pthis,
				     enum nxs_event_type type);
static void csc_clear_interrupt_pending(const struct nxs_dev *pthis,
					enum nxs_event_type type);

static irqreturn_t csc_irq_handler(void *priv)
{
	struct nxs_csc *csc = (struct nxs_csc *)priv;
	struct nxs_dev *nxs_dev = &csc->nxs_dev;
	struct nxs_irq_callback *callback;
	unsigned long flags;

	dev_info(nxs_dev->dev, "[%s] pending status = 0x%x\n", __func__,
		 csc_get_interrupt_pending(nxs_dev, NXS_EVENT_ALL));
	csc_clear_interrupt_pending(nxs_dev,
				    csc_get_interrupt_pending_number(csc));
	spin_lock_irqsave(&nxs_dev->irq_lock, flags);
	list_for_each_entry(callback, &nxs_dev->irq_callback, list) {
		if (callback)
			callback->handler(nxs_dev, callback->data);
	}
	spin_unlock_irqrestore(&nxs_dev->irq_lock, flags);
	return IRQ_HANDLED;
}

static void csc_reset_register(struct nxs_csc *csc)
{
	dev_info(csc->nxs_dev.dev, "[%s]\n", __func__);
	regmap_update_bits(csc->reg, csc->offset + CSC_CFG_CTRL,
			   CSC_REG_CLEAR_MASK,
			   1 << CSC_REG_CLEAR_SHIFT);
}

static bool csc_check_busy(struct nxs_csc *csc)
{
	u32 idle = 0;

	regmap_read(csc->reg, csc->offset + CSC_CFG_CTRL, &idle);
	return ~(idle & CSC_IDLE_MASK);
}

static void csc_set_enc_format(struct nxs_csc *csc, u32 type)
{
	dev_info(csc->nxs_dev.dev, "[%s] type = %s\n",
		 __func__, (type) ? "RGB" : "YUV");
	regmap_update_bits(csc->reg, csc->offset + CSC_TID_CTRL,
			   CSC_IMGTYPE_MASK,
			   type << CSC_IMGTYPE_SHIFT);
}

static void csc_set_bypass_alpha_enable(struct nxs_csc *csc, bool enable)
{
	dev_info(csc->nxs_dev.dev, "[%s] %s\n",
		 __func__, (enable) ? "enable" : "disable");
	regmap_update_bits(csc->reg, csc->offset + CSC_ALPHA_CTRL,
			   CSC_BYPASS_ALPHA_MASK,
			   enable << CSC_BYPASS_ALPHA_SHIFT);
}

static void csc_set_alpha_value(struct nxs_csc *csc, u32 value)
{
	dev_info(csc->nxs_dev.dev, "[%s] 0x%x\n", __func__, value);
	regmap_update_bits(csc->reg, csc->offset + CSC_ALPHA_CTRL,
			   CSC_USER_ALPHA_MASK,
			   value << CSC_USER_ALPHA_SHIFT);
}

static void csc_set_start(struct nxs_csc *csc, bool enable)
{
	dev_info(csc->nxs_dev.dev, "[%s] %s\n", __func__,
		 (enable) ? "start" : "stop");
	regmap_update_bits(csc->reg, csc->offset + CSC_CFG_CTRL,
			   CSC_DEC_EN_MASK,
			   enable << CSC_DEC_EN_SHIFT);
}

static u32 csc_get_status(struct nxs_csc *csc, u32 offset, u32 mask,
			  u32 shift)
{
	u32 status;

	regmap_read(csc->reg, csc->offset + offset, &status);
	return ((status & mask) >> shift);
}

static void csc_dump_register(const struct nxs_dev *pthis)
{
	struct nxs_csc *csc = nxs_to_csc(pthis);

	dev_info(pthis->dev, "============================================\n");

	dev_info(pthis->dev,
		 "TID:%d, Enable:%d\n",
		 csc_get_status(csc,
				CSC_TID_CTRL,
				CSC_TID_MASK,
				CSC_TID_SHIFT),
		 csc_get_status(csc,
				CSC_CFG_CTRL,
				CSC_DEC_EN_MASK,
				CSC_DEC_EN_SHIFT));

	dev_info(pthis->dev,
		 "Encode Format:%d(0:RGB->YUV, 1:YUV->RGB)\n",
		 csc_get_status(csc,
				CSC_TID_CTRL,
				CSC_IMGTYPE_MASK,
				CSC_IMGTYPE_SHIFT));

	dev_info(pthis->dev,
		 "BypassMode:%d, Alpha Value:%d\n",
		 csc_get_status(csc,
				CSC_ALPHA_CTRL,
				CSC_BYPASS_ALPHA_MASK,
				CSC_BYPASS_ALPHA_SHIFT),
		 csc_get_status(csc,
				CSC_ALPHA_CTRL,
				CSC_USER_ALPHA_MASK,
				CSC_USER_ALPHA_SHIFT));

	dev_info(pthis->dev, "Interrupt Status:0x%x\n",
		 csc_get_status(csc, CSC_INT_CTRL, 0xFFFF, 0));

	dev_info(pthis->dev, "============================================\n");
}

static enum nxs_event_type
csc_get_interrupt_pending_number(struct nxs_csc *csc)
{
	enum nxs_event_type ret = NXS_EVENT_NONE;
	u32 pend_status = 0;

	dev_info(csc->nxs_dev.dev, "[%s]\n", __func__);
	regmap_read(csc->reg, csc->offset + CSC_INT_CTRL, &pend_status);
	if (pend_status & CSC_IDLE_INTPEND_CLR_MASK)
		ret |= NXS_EVENT_IDLE;
	if (pend_status & CSC_UPDATE_INTPEND_CLR_MASK)
		ret |= NXS_EVENT_DONE;
	if (pend_status & CSC_DEC_ERR_INTPEND_CLR_MASK)
		ret |= NXS_EVENT_ERR;
	if (pend_status & CSC_DEC_SEC_INTPEND_CLR_MASK)
		ret |= NXS_EVENT_ERR;
	return ret;
}

static void csc_set_interrupt_enable(const struct nxs_dev *pthis,
				     enum nxs_event_type type,
				     bool enable)
{
	struct nxs_csc *csc = (struct nxs_csc *)pthis;
	u32 mask_v = 0;

	dev_info(pthis->dev, "[%s] type = %d %s\n", __func__, type,
		 (enable) ? "enable" : "disable");
	if (enable) {
		if (type & NXS_EVENT_IDLE)
			mask_v |= CSC_IDLE_INTENB_MASK;
		if (type & NXS_EVENT_DONE)
			mask_v |= CSC_UPDATE_INTENB_MASK;
		if (type & NXS_EVENT_ERR) {
			mask_v |= CSC_DEC_ERR_INTENB_MASK;
			mask_v |= CSC_DEC_SEC_INTENB_MASK;
		}
	} else {
		if (type & NXS_EVENT_IDLE)
			mask_v |= CSC_IDLE_INTDISABLE_MASK;
		if (type & NXS_EVENT_DONE)
			mask_v |= CSC_UPDATE_INTDISABLE_MASK;
		if (type & NXS_EVENT_ERR) {
			mask_v |= CSC_DEC_ERR_INTDISABLE_MASK;
			mask_v |= CSC_DEC_SEC_INTDISABLE_MASK;
		}
	}
	regmap_update_bits(csc->reg, csc->offset + CSC_INT_CTRL,
			   mask_v, mask_v);
}

static u32 csc_get_interrupt_enable(const struct nxs_dev *pthis,
				    enum nxs_event_type type)
{
	struct nxs_csc *csc = (struct nxs_csc *)pthis;
	u32 mask_v = 0, status = 0;

	dev_info(pthis->dev, "[%s] type : %d\n", __func__, type);
	if (type & NXS_EVENT_IDLE)
		mask_v |= CSC_IDLE_INTENB_MASK;
	if (type & NXS_EVENT_DONE)
		mask_v |= CSC_UPDATE_INTENB_MASK;
	if (type & NXS_EVENT_ERR) {
		mask_v |= CSC_DEC_ERR_INTENB_MASK;
		mask_v |= CSC_DEC_SEC_INTENB_MASK;
	}
	regmap_read(csc->reg, csc->offset + CSC_INT_CTRL, &status);
	status &= mask_v;
	return status;
}

static u32 csc_get_interrupt_pending(const struct nxs_dev *pthis,
				     enum nxs_event_type type)
{
	struct nxs_csc *csc = (struct nxs_csc *)pthis;
	u32 mask_v = 0, status = 0;

	dev_info(pthis->dev, "[%s] type : %d\n", __func__, type);
	if (type & NXS_EVENT_IDLE)
		mask_v |= CSC_IDLE_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_DONE)
		mask_v |= CSC_UPDATE_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_ERR) {
		mask_v |= CSC_DEC_ERR_INTPEND_CLR_MASK;
		mask_v |= CSC_DEC_SEC_INTPEND_CLR_MASK;
	}
	regmap_read(csc->reg, csc->offset + CSC_INT_CTRL, &status);
	status &= mask_v;

	return status;
}

static void csc_clear_interrupt_pending(const struct nxs_dev *pthis,
					enum nxs_event_type type)
{
	struct nxs_csc *csc = (struct nxs_csc *)pthis;
	u32 mask_v = 0;

	dev_info(pthis->dev, "[%s] type : %d\n", __func__, type);
	if (type & NXS_EVENT_IDLE)
		mask_v |= CSC_IDLE_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_DONE)
		mask_v |= CSC_UPDATE_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_ERR) {
		mask_v |= CSC_DEC_ERR_INTPEND_CLR_MASK;
		mask_v |= CSC_DEC_SEC_INTPEND_CLR_MASK;
	}
	regmap_update_bits(csc->reg, csc->offset + CSC_INT_CTRL, mask_v,
			   mask_v);
}

static int csc_open(const struct nxs_dev *pthis)
{
	struct nxs_csc *csc = (struct nxs_csc *)pthis;

	dev_info(pthis->dev, "[%s]\n", __func__);
	if (nxs_dev_get_open_count(&csc->nxs_dev) == 0) {
		struct clk *clk;
		int ret;
		char dev_name[10] = {0, };

		sprintf(dev_name, "csc%d", pthis->dev_inst_index);
		clk = clk_get(pthis->dev, dev_name);
		if (IS_ERR(clk)) {
			dev_err(pthis->dev, "controller clock not found\n");
			return -ENODEV;
		}
		ret = clk_prepare_enable(clk);
		if (ret) {
			dev_err(pthis->dev, "clock failed to prepare enable:%d\n",
				ret);
			return ret;
		}
		ret = request_irq(csc->irq, csc_irq_handler,
				  IRQF_TRIGGER_NONE,
				  "nxs-csc", csc);
		if (ret < 0) {
			dev_err(pthis->dev, "failed to request irq: %d\n",
				ret);
			return ret;
		}
	}
	nxs_dev_inc_open_count(&csc->nxs_dev);
	return 0;
}

static int csc_close(const struct nxs_dev *pthis)
{
	struct nxs_csc *csc = (struct nxs_csc *)pthis;

	dev_info(pthis->dev, "[%s]\n", __func__);
	if (csc_check_busy(csc)) {
		dev_err(pthis->dev, "csc is busy\n");
		return -EBUSY;
	}

	nxs_dev_dec_open_count(&csc->nxs_dev);
	if (nxs_dev_get_open_count(&csc->nxs_dev) == 0) {
		struct clk *clk;
		char dev_name[10] = {0, };

		csc_reset_register(csc);
		free_irq(csc->irq, csc);
		sprintf(dev_name, "csc%d", pthis->dev_inst_index);
		clk = clk_get(pthis->dev, dev_name);
		if (IS_ERR(clk)) {
			dev_err(pthis->dev, "controller clock not found\n");
			return -ENODEV;
		}
		clk_disable_unprepare(clk);
	}
	return 0;
}

static int csc_start(const struct nxs_dev *pthis)
{
	struct nxs_csc *csc = (struct nxs_csc *)pthis;

	dev_info(pthis->dev, "[%s]\n", __func__);
	csc_set_start(csc, true);
	return 0;
}

static int csc_stop(const struct nxs_dev *pthis)
{
	struct nxs_csc *csc = (struct nxs_csc *)pthis;

	dev_info(pthis->dev, "[%s]\n", __func__);
	csc_set_start(csc, false);
	return 0;
}

static int csc_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_csc *csc = nxs_to_csc(pthis);
	u32 dirty_val;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	switch (pthis->dev_inst_index) {
	case 0:
		dirty_val = CSC0_DIRTY;
		break;
	case 1:
		dirty_val = CSC1_DIRTY;
		break;
	case 2:
		dirty_val = CSC2_DIRTY;
		break;
	case 3:
		dirty_val = CSC3_DIRTY;
		break;
	default:
		dev_err(pthis->dev, "invalid inst %d\n", pthis->dev_inst_index);
		return -ENODEV;
	}

	return regmap_write(csc->reg, CSC_DIRTYSET_OFFSET, dirty_val);
}

static int csc_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_csc *csc = nxs_to_csc(pthis);

	return regmap_update_bits(csc->reg, csc->offset + CSC_TID_CTRL,
				  CSC_TID_MASK, tid1 << CSC_TID_SHIFT);
}

static int csc_set_format(const struct nxs_dev *pthis,
			  const struct nxs_control *pparam)
{
	struct nxs_csc *csc = nxs_to_csc(pthis);
	u32 format = pparam->u.format.pixelformat;
	struct nxs_plane_format config;
	bool	bypass_mode = false;

	dev_info(pthis->dev, "[%s]\n", __func__);
	nxs_get_plane_format(&csc->nxs_dev, format, &config);
	if (config.img_type > NXS_IMG_RGB) {
		dev_err(pthis->dev, "not supported format\n");
		return -EINVAL;
	}
	if (config.img_type == NXS_IMG_RGB) {
		/*
		 * if config.p0_comp[3].use_userdef is false,
		 * it means the format has alpha value
		 */
		if ((!config.p0_comp[3].use_userdef) &&
		    (CSC_BYPASS_ALPHA_ENABLE))
			bypass_mode = true;
	}
	csc_set_bypass_alpha_enable(csc, bypass_mode);
	return 0;
}

static int csc_get_format(const struct nxs_dev *pthis,
			  const struct nxs_control *pparam)
{
	return 0;
}

static int csc_set_dstformat(const struct nxs_dev *pthis,
			     const struct nxs_control *pparam)
{
	struct nxs_csc *csc = nxs_to_csc(pthis);
	u32 format = pparam->u.format.pixelformat;
	struct nxs_plane_format config;

	dev_info(pthis->dev, "[%s]\n", __func__);
	nxs_get_plane_format(&csc->nxs_dev, format, &config);
	if (config.img_type > NXS_IMG_RGB) {
		dev_err(pthis->dev, "not supported format\n");
		return -EINVAL;
	}
	if (config.img_type == NXS_IMG_RGB)
		csc_set_alpha_value(csc, CSC_USER_DEF_ALPHA_VALUE);
	csc_set_enc_format(csc, config.img_type);
	return 0;
}

static int csc_get_dstformat(const struct nxs_dev *pthis,
			     const struct nxs_control *pparam)
{
	return 0;
}

static int nxs_csc_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_csc *csc;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	csc = devm_kzalloc(&pdev->dev, sizeof(*csc), GFP_KERNEL);
	if (!csc)
		return -ENOMEM;

	nxs_dev = &csc->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	csc->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						   "syscon");
	if (IS_ERR(csc->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(csc->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	csc->offset = res->start;

	nxs_dev->set_interrupt_enable = csc_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = csc_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = csc_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = csc_clear_interrupt_pending;
	nxs_dev->open = csc_open;
	nxs_dev->close = csc_close;
	nxs_dev->start = csc_start;
	nxs_dev->stop = csc_stop;
	nxs_dev->set_dirty = csc_set_dirty;
	nxs_dev->set_tid = csc_set_tid;
	nxs_dev->dump_register = csc_dump_register;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_FORMAT;
	nxs_dev->dev_services[0].set_control = csc_set_format;
	nxs_dev->dev_services[0].get_control = csc_get_format;
	nxs_dev->dev_services[1].type = NXS_CONTROL_DST_FORMAT;
	nxs_dev->dev_services[1].set_control = csc_set_dstformat;
	nxs_dev->dev_services[1].get_control = csc_get_dstformat;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, csc);

	return 0;
}

static int nxs_csc_remove(struct platform_device *pdev)
{
	struct nxs_csc *csc = platform_get_drvdata(pdev);

	if (csc)
		unregister_nxs_dev(&csc->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_csc_match[] = {
	{ .compatible = "nexell,csc-nxs-1.0", },
	{},
};

static struct platform_driver nxs_csc_driver = {
	.probe	= nxs_csc_probe,
	.remove	= nxs_csc_remove,
	.driver	= {
		.name = "nxs-csc",
		.of_match_table = of_match_ptr(nxs_csc_match),
	},
};

static int __init nxs_csc_init(void)
{
	return platform_driver_register(&nxs_csc_driver);
}
/* subsys_initcall(nxs_csc_init); */
fs_initcall(nxs_csc_init);

static void __exit nxs_csc_exit(void)
{
	platform_driver_unregister(&nxs_csc_driver);
}
module_exit(nxs_csc_exit);

MODULE_DESCRIPTION("Nexell Stream CSC driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

