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
#include <linux/soc/nexell/nxs_res_manager.h>

#define SCALER_DIRTYSET_OFFSET		0x0004
#define SCALER_DIRTYCLR_OFFSET		0x0014
#define SCALER0_DIRTY			BIT(30)
#define SCALER1_DIRTY			BIT(29)
#define SCALER2_DIRTY			BIT(28)

/* SCALER REGISTER OFFSET */
#define SCALER_SC_ENABLE		0x0000
#define SCALER_SRC_IMG_SIZE		0x0004
#define SCALER_SC_FILTER_EN		0x0008
#define SCALER_SC_DELTA_X		0x000C
#define SCALER_SC_DELTA_Y		0x0010
#define	SCALER_SC_SOFT			0x0014
#define	SCALER_DST_IMG_SIZE		0x0018
#define SCALER_DBG_LCNT			0x001C
#define SCALER_INT			0x0018
#define SCALER_ENC_TID			0x0020
/* SCALER_SC_EN */
#define SCALER_REG_CLEAR_SHIFT		4
#define SCALER_REG_CLEAR_MASK		BIT(4)
#define SCALER_CFG_SC_EN_SHIFT		0
#define SCALER_CFG_SC_EN_MASK		BIT(0)
/* SCALER SRC IMG SIZE */
#define SCALER_SRC_IMG_HEIGHT_SHIFT	16
#define SCALER_SRC_IMG_HEIGHT_MASK	GENMASK(31, 16)
#define SCALER_SRC_IMG_WIDTH_SHIFT	0
#define SCALER_SRC_IMG_WIDTH_MASK	GENMASK(15, 0)
/* SCALER SC FILTER ENABLE */
#define SCALER_HFILTER_EN_SHIFT		1
#define SCALER_HFILTER_EN_MASK		BIT(1)
#define SCALER_VFILTER_EN_SHIFT		0
#define SCALER_VFILTER_EN_MASK		BIT(1)
/* SCALER SC DELTA X */
#define SCALER_SC_DELTA_X_SHIFT		0
#define SCALER_SC_DELTA_X_MASK		GENMASK(31, 0)
#define SCALER_SC_DELTA_Y_SHIFT		0
#define SCALER_SC_DELTA_Y_MASK		GENMASK(31, 0)
/* SCALER SC SOFT */
#define SCALER_SC_V_SOFT_SHIFT		16
#define SCALER_SC_V_SOFT_MASK		GENMASK(20, 16)
#define SCALER_SC_H_SOFT_SHIFT		0
#define SCALER_SC_H_SOFT_MASK		GENMASK(5, 0)
/* SCALER DST IMG SIZE */
#define SCALER_DST_IMG_HEIGHT_SHIFT	16
#define SCALER_DST_IMG_HEIGHT_MASK	GENMASK(31, 16)
#define SCALER_DST_IMG_WIDTH_SHIFT	0
#define SCALER_DST_IMG_WIDTH_MASK	GENMASK(15, 0)
/* SCALER DBG LCNT */
#define SCALER_ERR_STATUS_SHIFT		20
#define SCALER_ERR_STATUS_MASK		GENMASK(26, 20)
#define SCALER_ERR_STATE_SHIFT		16
#define SCALER_ERR_STATE_MASK		GENMASK(19, 16)
#define SCALER_DBG_FCNT_SHIFT		8
#define SCALER_DBG_FCNT_MASK		GENMASK(15, 8)
#define SCALER_DBG_LCNT_SHIFT		0
#define SCALER_DBG_LCNT_MASK		GENMASK(7, 0)
/* SCALER ENC TID */
#define SCALER_ENC_TID_SHIFT		0
#define SCALER_ENC_TID_MASK		GENMASK(13, 0)
/* SCALER INTERRUPT */
#define SCALER_IDLE_SHIFT		23
#define SCALER_IDLE_MASK		BIT(23)
#define SCALER_IDLE_INTDIS_SHIFT	22
#define SCALER_IDLE_INTDIS_MASK		BIT(22)
#define SCALER_IDLE_INTEN_SHIFT		21
#define SCALER_IDLE_INTEN_MASK		BIT(21)
#define SCALER_IDLE_INTPEND_CLR_SHIFT	20
#define SCALER_IDLE_INTPEND_CLR_MASK	BIT(20)
#define SCALER_RESOL_INTDIS_SHIFT	18
#define SCALER_RESOL_INTDIS_MASK	BIT(18)
#define SCALER_RESOL_INTEN_SHIFT	17
#define SCALER_RESOL_INTEN_MASK		BIT(17)
#define SCALER_RESOL_INTPEND_CLR_SHIFT	16
#define SCALER_RESOL_INTPEND_CLR_MASK	BIT(16)
#define SCALER_SEC_INTDIS_SHIFT		14
#define SCALER_SEC_INTDIS_MASK		BIT(14)
#define SCALER_SEC_INTEN_SHIFT		13
#define SCALER_SEC_INTEN_MASK		BIT(13)
#define SCALER_SEC_INTPEND_CLR_SHIFT	12
#define SCALER_SEC_INTPEND_CLR_MASK	BIT(12)
#define SCALER_ERR_INTDIS_SHIFT		10
#define SCALER_ERR_INTDIS_MASK		BIT(10)
#define SCALER_ERR_INTEN_SHIFT		9
#define SCALER_ERR_INTEN_MASK		BIT(9)
#define SCALER_ERR_INTPEND_CLR_SHIFT	8
#define SCALER_ERR_INTPEND_CLR_MASK	BIT(8)
#define SCALER_UPDATE_INTDIS_SHIFT	2
#define SCALER_UPDATE_INTDIS_MASK	BIT(2)
#define SCALER_UPDATE_INTEN_SHIFT	1
#define SCALER_UPDATE_INTEN_MASK	BIT(1)
#define SCALER_UPDATE_INTPEND_CLR_SHIFT	0
#define SCALER_UPDATE_INTPEND_CLR_MASK	BIT(0)

#define SCALER_H_SOFT_RESET_VALUE	(0x8)
#define SCALER_V_SOFT_RESET_VALUE	(0x10)


struct nxs_scaler {
	struct nxs_dev nxs_dev;
	u32 line_buffer_size;
	struct regmap *reg;
	u32 offset;
	u32 irq;
};

#define nxs_to_scaler(dev)	container_of(dev, struct nxs_scaler, nxs_dev)

static u32 scaler_get_interrupt_pending(const struct nxs_dev *pthis,
					enum nxs_event_type type);
static void scaler_clear_interrupt_pending(const struct nxs_dev *pthis,
					   enum nxs_event_type type);
static enum nxs_event_type scaler_get_interrupt_pending_number
(struct nxs_scaler *scaler);

static irqreturn_t scaler_irq_handler(void *priv)
{
	struct nxs_scaler *scaler = (struct nxs_scaler *)priv;
	struct nxs_dev *nxs_dev = &scaler->nxs_dev;
	struct nxs_irq_callback *callback;
	unsigned long flags;

	dev_info(nxs_dev->dev, "[%s] pending status = 0x%x\n", __func__,
		 scaler_get_interrupt_pending(nxs_dev, NXS_EVENT_ALL));
	scaler_clear_interrupt_pending
		(nxs_dev, scaler_get_interrupt_pending_number(scaler));
	spin_lock_irqsave(&nxs_dev->irq_lock, flags);
	list_for_each_entry(callback, &nxs_dev->irq_callback, list) {
		if (callback)
			callback->handler(nxs_dev, callback->data);
	}
	spin_unlock_irqrestore(&nxs_dev->irq_lock, flags);
	return IRQ_HANDLED;
}

static u32 scaler_get_status(struct nxs_scaler *scaler, u32 offset, u32 mask,
			     u32 shift)
{
	u32 status;

	regmap_read(scaler->reg, scaler->offset + offset, &status);
	return ((status & mask) >> shift);
}

static void scaler_reset_register(struct nxs_scaler *scaler)
{
	dev_info(scaler->nxs_dev.dev, "[%s]\n", __func__);
	regmap_update_bits(scaler->reg, scaler->offset + SCALER_SC_ENABLE,
			   SCALER_REG_CLEAR_MASK,
			   1 << SCALER_REG_CLEAR_SHIFT);
}

static void scaler_dump_register(const struct nxs_dev *pthis)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);

	dev_info(pthis->dev, "============================================\n");

	dev_info(pthis->dev, "Scaler[%d] Dump Register : 0x%8x\n",
		 pthis->dev_inst_index, scaler->offset);

	dev_info(pthis->dev,
		 "TID:%d, Enable:%d\n",
		 scaler_get_status(scaler,
				   SCALER_ENC_TID,
				   SCALER_ENC_TID_MASK,
				   SCALER_ENC_TID_SHIFT),
		 scaler_get_status(scaler,
				   SCALER_SC_ENABLE,
				   SCALER_CFG_SC_EN_MASK,
				   SCALER_CFG_SC_EN_SHIFT));
	dev_info(pthis->dev,
		 "Src image width:%d, height:%d\n",
		 scaler_get_status(scaler,
				   SCALER_SRC_IMG_SIZE,
				   SCALER_SRC_IMG_WIDTH_MASK,
				   SCALER_SRC_IMG_WIDTH_SHIFT),
		 scaler_get_status(scaler,
				   SCALER_SRC_IMG_SIZE,
				   SCALER_SRC_IMG_HEIGHT_MASK,
				   SCALER_SRC_IMG_HEIGHT_SHIFT));
	dev_info(pthis->dev,
		 "Dst image width:%d, height:%d\n",
		 scaler_get_status(scaler,
				   SCALER_DST_IMG_SIZE,
				   SCALER_DST_IMG_WIDTH_MASK,
				   SCALER_DST_IMG_WIDTH_SHIFT),
		 scaler_get_status(scaler,
				   SCALER_DST_IMG_SIZE,
				   SCALER_DST_IMG_HEIGHT_MASK,
				   SCALER_DST_IMG_HEIGHT_SHIFT));
	dev_info(pthis->dev,
		 "HFilter enable:%d, VFilter enable:%d\n",
		 scaler_get_status(scaler,
				   SCALER_SC_FILTER_EN,
				   SCALER_HFILTER_EN_MASK,
				   SCALER_HFILTER_EN_SHIFT),
		 scaler_get_status(scaler,
				   SCALER_SC_FILTER_EN,
				   SCALER_VFILTER_EN_MASK,
				   SCALER_VFILTER_EN_SHIFT));
	dev_info(pthis->dev,
		 "Delta X:%d, Y:%d\n",
		 scaler_get_status(scaler,
				   SCALER_SC_DELTA_X,
				   SCALER_SC_DELTA_X_MASK,
				   SCALER_SC_DELTA_X_SHIFT),
		 scaler_get_status(scaler,
				   SCALER_SC_DELTA_Y,
				   SCALER_SC_DELTA_Y_MASK,
				   SCALER_SC_DELTA_Y_SHIFT));
	dev_info(pthis->dev,
		 "V Soft:%d, H Soft:%d\n",
		 scaler_get_status(scaler,
				   SCALER_SC_SOFT,
				   SCALER_SC_V_SOFT_MASK,
				   SCALER_SC_V_SOFT_SHIFT),
		 scaler_get_status(scaler,
				   SCALER_SC_SOFT,
				   SCALER_SC_H_SOFT_MASK,
				   SCALER_SC_H_SOFT_SHIFT));
	dev_info(pthis->dev, "Interrupt Status:0x%x\n",
		 scaler_get_status(scaler, SCALER_INT, 0xFFFF, 0));

	dev_info(pthis->dev, "============================================\n");
}

/* SCALER DST IMG SIZE */
static void scaler_set_interrupt_enable(const struct nxs_dev *pthis,
					enum nxs_event_type type,
					bool enable)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);
	u32	mask_v = 0;

	dev_info(pthis->dev, "[%s] %s\n", __func__,
		 (enable)?"enable":"disable");
	if (enable) {
		if (type & NXS_EVENT_IDLE)
			mask_v |= SCALER_IDLE_INTEN_MASK;
		if (type & NXS_EVENT_DONE)
			mask_v |= SCALER_UPDATE_INTEN_MASK;
		if (type & NXS_EVENT_ERR) {
			mask_v |= SCALER_RESOL_INTEN_MASK;
			mask_v |= SCALER_SEC_INTEN_MASK;
			mask_v |= SCALER_ERR_INTEN_MASK;
		}
	} else {
		if (type & NXS_EVENT_IDLE)
			mask_v |= SCALER_IDLE_INTDIS_MASK;
		if (type & NXS_EVENT_DONE)
			mask_v |= SCALER_UPDATE_INTDIS_MASK;
		if (type & NXS_EVENT_ERR) {
			mask_v |= SCALER_RESOL_INTDIS_MASK;
			mask_v |= SCALER_SEC_INTDIS_MASK;
			mask_v |= SCALER_ERR_INTDIS_MASK;
		}
	}
	regmap_update_bits(scaler->reg, scaler->offset + SCALER_INT,
			   mask_v, mask_v);
}

static u32 scaler_get_interrupt_enable(const struct nxs_dev *pthis,
				      enum nxs_event_type type)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);
	u32	mask_v = 0, status = 0;

	dev_info(pthis->dev, "[%s] type:%d\n", __func__, type);
	if (type & NXS_EVENT_IDLE)
		mask_v |= SCALER_IDLE_INTEN_MASK;
	if (type & NXS_EVENT_DONE)
		mask_v |= SCALER_UPDATE_INTEN_MASK;
	if (type & NXS_EVENT_ERR) {
		mask_v |= SCALER_RESOL_INTEN_MASK;
		mask_v |= SCALER_SEC_INTEN_MASK;
		mask_v |= SCALER_ERR_INTEN_MASK;
	}
	regmap_read(scaler->reg, scaler->offset + SCALER_INT, &status);
	status &= mask_v;
	return status;
}

static u32 scaler_get_interrupt_pending(const struct nxs_dev *pthis,
					enum nxs_event_type type)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);
	u32	mask_v, pend_status;

	dev_info(pthis->dev, "[%s] type:%d\n", __func__, type);
	if (type & NXS_EVENT_IDLE)
		mask_v |= SCALER_IDLE_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_DONE)
		mask_v |= SCALER_UPDATE_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_ERR) {
		mask_v |= SCALER_RESOL_INTPEND_CLR_MASK;
		mask_v |= SCALER_SEC_INTPEND_CLR_MASK;
		mask_v |= SCALER_ERR_INTPEND_CLR_MASK;
	}
	regmap_read(scaler->reg, scaler->offset + SCALER_INT, &pend_status);
	pend_status &= mask_v;
	return pend_status;
}

static enum nxs_event_type scaler_get_interrupt_pending_number
(struct nxs_scaler *scaler)
{
	enum nxs_event_type	ret = NXS_EVENT_NONE;
	u32	pend_status = 0;

	dev_info(scaler->nxs_dev.dev, "[%s]\n", __func__);
	regmap_read(scaler->reg, scaler->offset + SCALER_INT, &pend_status);
	if (pend_status & SCALER_IDLE_INTPEND_CLR_MASK)
		ret |= NXS_EVENT_IDLE;
	if (pend_status & SCALER_UPDATE_INTPEND_CLR_MASK)
		ret |= NXS_EVENT_DONE;
	if (pend_status & SCALER_RESOL_INTPEND_CLR_MASK)
		ret |= NXS_EVENT_ERR;
	if (pend_status & SCALER_SEC_INTPEND_CLR_MASK)
		ret |= NXS_EVENT_ERR;
	if (pend_status & SCALER_ERR_INTPEND_CLR_MASK)
		ret |= NXS_EVENT_ERR;
	return ret;
}

static void scaler_clear_interrupt_pending(const struct nxs_dev *pthis,
					   enum nxs_event_type type)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);
	u32	mask_v = 0;

	dev_info(pthis->dev, "[%s] type:%d\n", __func__, type);
	if (type & NXS_EVENT_IDLE)
		mask_v |= SCALER_IDLE_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_DONE)
		mask_v |= SCALER_UPDATE_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_ERR) {
		mask_v |= SCALER_RESOL_INTPEND_CLR_MASK;
		mask_v |= SCALER_SEC_INTPEND_CLR_MASK;
		mask_v |= SCALER_ERR_INTPEND_CLR_MASK;
	}
	regmap_update_bits(scaler->reg, scaler->offset + SCALER_INT,
			   mask_v, mask_v);
}

static bool scaler_check_busy(struct nxs_scaler *scaler)
{
	u32 idle = 0;

	regmap_read(scaler->reg, scaler->offset + SCALER_INT, &idle);
	return ~(idle & SCALER_IDLE_MASK);
}

static int scaler_open(const struct nxs_dev *pthis)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	if (nxs_dev_get_open_count(&scaler->nxs_dev) == 0) {
		struct clk *clk;
		int ret;
		char dev_name[10] = {0, };

		sprintf(dev_name, "scaler%d", pthis->dev_inst_index);
		/* clock enable */
		clk = clk_get(pthis->dev, dev_name);
		if (IS_ERR(clk)) {
			dev_err(pthis->dev, "controller clock not found\n");
			return -ENODEV;
		}
		ret = clk_prepare_enable(clk);
		if (ret) {
			dev_err(pthis->dev, "clock failed to prepare enable : %d\n",
				ret);
			return ret;
		}
		/* reset */
		/* register irq handler */
		ret = request_irq(scaler->irq, scaler_irq_handler,
				  IRQF_TRIGGER_NONE,
				  "nxs-scaler", scaler);
		if (ret < 0) {
			dev_err(pthis->dev, "failed to request irq : %d\n",
				ret);
			return ret;
		}
	}
	nxs_dev_inc_open_count(&scaler->nxs_dev);
	return 0;
}

static int scaler_close(const struct nxs_dev *pthis)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	/* check the busy status */
	if (scaler_check_busy(scaler)) {
		dev_err(pthis->dev, "scaler is busy\n");
		return -EBUSY;
	}

	nxs_dev_dec_open_count(&scaler->nxs_dev);
	if (nxs_dev_get_open_count(&scaler->nxs_dev) == 0) {
		struct clk *clk;
		char dev_name[10] = {0, };

		scaler_reset_register(scaler);
		/* unregister irq handler */
		free_irq(scaler->irq, scaler);
		/* clock disable */
		sprintf(dev_name, "scaler%d", pthis->dev_inst_index);
		clk = clk_get(pthis->dev, dev_name);
		if (IS_ERR(clk)) {
			dev_err(pthis->dev, "controller clock not found\n");
			return -ENODEV;
		}
		clk_disable_unprepare(clk);
	}
	return 0;
}

static int scaler_set_filter_enable(struct nxs_scaler *scaler,
				    bool enable)
{
	return regmap_update_bits(scaler->reg,
				  scaler->offset + SCALER_SC_FILTER_EN,
				  (SCALER_VFILTER_EN_MASK |
				   SCALER_HFILTER_EN_MASK),
				  ((enable << SCALER_VFILTER_EN_SHIFT) |
				   (enable << SCALER_HFILTER_EN_SHIFT)));
}

static int scaler_set_enable(struct nxs_scaler *scaler,
			      bool enable)
{
	return regmap_update_bits(scaler->reg,
				  scaler->offset + SCALER_SC_ENABLE,
				  SCALER_CFG_SC_EN_MASK,
				  enable << SCALER_CFG_SC_EN_SHIFT);
}

static int scaler_set_soft_register(struct nxs_scaler *scaler,
				     u8 v_soft, u8 h_soft)
{
	u32 reg_val;

	regmap_read(scaler->reg, scaler->offset + SCALER_SC_SOFT, &reg_val);
	reg_val |= ((h_soft << SCALER_SC_H_SOFT_SHIFT) |
		    (v_soft << SCALER_SC_V_SOFT_SHIFT));
	return regmap_write(scaler->reg, scaler->offset + SCALER_SC_SOFT,
			    reg_val);
}

static int scaler_set_delta_register(struct nxs_scaler *scaler)
{
	u16 src_width, src_height, dst_width, dst_height;
	u32 value, delta_x, delta_y;

	regmap_read(scaler->reg, scaler->offset + SCALER_SRC_IMG_SIZE, &value);
	src_width = ((value & SCALER_SRC_IMG_WIDTH_MASK)
		     >> SCALER_SRC_IMG_WIDTH_SHIFT);
	src_height = (value & SCALER_SRC_IMG_HEIGHT_MASK);
	if ((!src_width) || (!src_height)) {
		dev_err(scaler->nxs_dev.dev,
			"source width and height are invalid\n");
		return -EINVAL;
	}
	regmap_read(scaler->reg, scaler->offset + SCALER_DST_IMG_SIZE, &value);
	dst_width = ((value & SCALER_DST_IMG_WIDTH_MASK)
		     >> SCALER_DST_IMG_WIDTH_SHIFT);
	dst_height = (value & SCALER_DST_IMG_HEIGHT_MASK);
	if ((!dst_height) || (!dst_width)) {
		dev_err(scaler->nxs_dev.dev,
			"destination width and height are invalid\n");
		return -EINVAL;
	}
	/* UP scaling */
	if (dst_width > src_width)
		delta_x = ((src_width * 0x10000) / dst_width);
	else /* DOWN scaling */
		delta_x = ((src_width * 0x10000) / (dst_width-1));
	regmap_write(scaler->reg, scaler->offset + SCALER_SC_DELTA_X, delta_x);
	if (dst_height > src_height) /* UP scaling */
		delta_y = ((src_height * 0x10000) / dst_height);
	else /* DOWN scaling */
		delta_y = ((src_height * 0x10000) / (dst_height-1));
	regmap_write(scaler->reg, scaler->offset + SCALER_SC_DELTA_Y, delta_y);

	return 0;
}

static int scaler_start(const struct nxs_dev *pthis)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	/* set src addr and src stride  -> dmar */
	/* set src width and src height -> set_format */
	/* set dst addr and stride -> dmaw */
	/* set dst width and dst height -> set_dst_format */
	/* set delta x and y register */
	if (scaler_set_delta_register(scaler)) {
		dev_err(scaler->nxs_dev.dev,
			"invalid width and height\n");
		return -EINVAL;
	}

	/* set horizontal/vertical soft register */
	scaler_set_soft_register(scaler,
				 SCALER_V_SOFT_RESET_VALUE,
				 SCALER_H_SOFT_RESET_VALUE);
	/* set filter enable */
	scaler_set_filter_enable(scaler, true);
	/* set scaler enable : sc_enable as true */
	scaler_set_enable(scaler, true);
	return 0;
}

static int scaler_stop(const struct nxs_dev *pthis)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	/* set scaler disable : sc_enable as false */
	return scaler_set_enable(scaler, false);
}

static int scaler_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);
	u32 dirty_val;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	switch (pthis->dev_inst_index) {
	case 0:
		dirty_val = SCALER0_DIRTY;
		break;
	case 1:
		dirty_val = SCALER1_DIRTY;
		break;
	case 2:
		dirty_val = SCALER2_DIRTY;
		break;
	default:
		dev_err(pthis->dev, "invalid inst %d\n", pthis->dev_inst_index);
		return -ENODEV;
	}

	return regmap_write(scaler->reg, SCALER_DIRTYSET_OFFSET, dirty_val);
}

static int scaler_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);

	return regmap_update_bits(scaler->reg, scaler->offset + SCALER_ENC_TID,
				  SCALER_ENC_TID_MASK,
				  tid1 << SCALER_ENC_TID_SHIFT);
}

static int scaler_set_format(const struct nxs_dev *pthis,
			     const struct nxs_control *pparam)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);

	dev_info(pthis->dev, "[%s] width:%d, height:%d\n",
		 __func__, pparam->u.format.width, pparam->u.format.height);

	return regmap_write(scaler->reg, scaler->offset + SCALER_SRC_IMG_SIZE,
			    ((pparam->u.format.width <<
			      SCALER_SRC_IMG_WIDTH_SHIFT) |
			     pparam->u.format.height));
}

static int scaler_get_format(const struct nxs_dev *pthis,
			     struct nxs_control *pparam)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);
	u32 size;

	dev_info(pthis->dev, "[%s]\n", __func__);
	regmap_read(scaler->reg, scaler->offset + SCALER_SRC_IMG_SIZE, &size);
	pparam->u.format.width = ((size & SCALER_SRC_IMG_WIDTH_MASK) >>
				  SCALER_SRC_IMG_WIDTH_SHIFT);
	pparam->u.format.height = (size & SCALER_SRC_IMG_HEIGHT_MASK);
	return 0;
}

static int scaler_set_dstformat(const struct nxs_dev *pthis,
				const struct nxs_control *pparam)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);

	dev_info(pthis->dev, "[%s] width:%d, height:%d\n",
		 __func__, pparam->u.format.width, pparam->u.format.height);

	return regmap_write(scaler->reg, scaler->offset + SCALER_DST_IMG_SIZE,
			    ((pparam->u.format.width <<
			      SCALER_DST_IMG_WIDTH_SHIFT) |
			     pparam->u.format.height));
}

static int scaler_get_dstformat(const struct nxs_dev *pthis,
				struct nxs_control *pparam)
{
	struct nxs_scaler *scaler = nxs_to_scaler(pthis);
	u32 size;

	dev_info(pthis->dev, "[%s]\n", __func__);
	regmap_read(scaler->reg, scaler->offset + SCALER_DST_IMG_SIZE, &size);
	pparam->u.format.width = ((size & SCALER_DST_IMG_WIDTH_MASK) >>
				  SCALER_DST_IMG_WIDTH_SHIFT);
	pparam->u.format.height = (size & SCALER_DST_IMG_HEIGHT_MASK);
	return 0;
}

static int nxs_scaler_parse_dt(struct platform_device *pdev,
			       struct nxs_scaler *scaler)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;

	if (of_property_read_u32(np, "line_buffer_size",
				&scaler->line_buffer_size)) {
		dev_err(dev, "failed to get line_buffer_size\n");
		return -EINVAL;
	}

	return 0;
}

static int nxs_scaler_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_scaler *scaler;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	scaler = devm_kzalloc(&pdev->dev, sizeof(*scaler), GFP_KERNEL);
	if (!scaler)
		return -ENOMEM;

	nxs_dev = &scaler->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	ret = nxs_scaler_parse_dt(pdev, scaler);
	if (ret)
		return ret;

	scaler->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						      "syscon");
	if (IS_ERR(scaler->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(scaler->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	scaler->offset = res->start;
	scaler->irq = platform_get_irq(pdev, 0);

	dev_info(&pdev->dev, "offset=0x%x, linebuffer size=0x%x, irq = %d\n",
		 scaler->offset, scaler->line_buffer_size, scaler->irq);

	nxs_dev->set_interrupt_enable = scaler_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = scaler_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = scaler_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = scaler_clear_interrupt_pending;
	nxs_dev->open = scaler_open;
	nxs_dev->close = scaler_close;
	nxs_dev->start = scaler_start;
	nxs_dev->stop = scaler_stop;
	nxs_dev->set_dirty = scaler_set_dirty;
	nxs_dev->set_tid = scaler_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dump_register = scaler_dump_register;
	nxs_dev->dev_services[0].type = NXS_CONTROL_FORMAT;
	nxs_dev->dev_services[0].set_control = scaler_set_format;
	nxs_dev->dev_services[0].get_control = scaler_get_format;
	nxs_dev->dev_services[1].type = NXS_CONTROL_DST_FORMAT;
	nxs_dev->dev_services[1].set_control = scaler_set_dstformat;
	nxs_dev->dev_services[1].get_control = scaler_get_dstformat;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, scaler);

	return 0;
}

static int nxs_scaler_remove(struct platform_device *pdev)
{
	struct nxs_scaler *scaler = platform_get_drvdata(pdev);

	if (scaler)
		unregister_nxs_dev(&scaler->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_scaler_match[] = {
	{ .compatible = "nexell,scaler-nxs-1.0", },
	{},
};

static struct platform_driver nxs_scaler_driver = {
	.probe	= nxs_scaler_probe,
	.remove	= nxs_scaler_remove,
	.driver	= {
		.name = "nxs-scaler",
		.of_match_table = of_match_ptr(nxs_scaler_match),
	},
};

static int __init nxs_scaler_init(void)
{
	return platform_driver_register(&nxs_scaler_driver);
}
/* subsys_initcall(nxs_scaler_init); */
fs_initcall(nxs_scaler_init);

static void __exit nxs_scaler_exit(void)
{
	platform_driver_unregister(&nxs_scaler_driver);
}
module_exit(nxs_scaler_exit);

MODULE_DESCRIPTION("Nexell Stream scaler driver");
MODULE_AUTHOR("Hyejung kwon, <cjscld15@nexell.co.kr>");
MODULE_LICENSE("GPL");

