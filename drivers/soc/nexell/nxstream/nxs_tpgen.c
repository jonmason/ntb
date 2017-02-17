/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
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

#include <linux/videodev2.h>

#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_res_manager.h>

#define TPGEN_DIRTYSET_OFFSET	0x0004
#define TPGEN_DIRTYCLR_OFFSET	0x0014
#define TPGEN_DIRTY		BIT(31)

/* regmap: base 0x20c03e00 */
#define TPGEN_BASIC_CTRL	0x0000
#define TPGEN_INTERRUPT		0x0004
#define TPGEN_USERDEF_HEADER0	0x0008
#define TPGEN_USERDEF_HEADER1	0x000c
#define TPGEN_USERDEF_HEADER2	0x0010
#define TPGEN_USERDEF_HEADER3	0x0014
#define TPGEN_IMG_SIZE		0x0018
#define TPGEN_CTRL		0x001c
#define TPGEN_V2H_DLY_CTRL	0x0020
#define TPGEN_H2H_DLY_CTRL	0x0024
#define TPGEN_H2V_DLY_CTRL	0x0028
#define TPGEN_V2V_DLY_CTRL	0x002c

/* TPGEN BASIC CTRL */
#define TPGEN_REG_CLEAR_SHIFT		17
#define TPGEN_REG_CLEAR_MASK		BIT(17)
#define TPGEN_TZPROT_SHIFT		16
#define TPGEN_TZPROT_MASK		BIT(16)
#define TPGEN_NCLK_PER_TWOPIX_SHIFT	8
#define TPGEN_NCLK_PER_TWOPIX_MASK	GENMASK(15, 8)
#define TPGEN_REALTIME_MODE_SHIFT	7
#define TPGEN_REALTIME_MODE_MASK	BIT(7)
#define TPGEN_TPGEN_MODE_SHIFT		4
#define TPGEN_TPGEN_MODE_MASK		GENMASK(6, 4)
#define TPGEN_IDLE_SHIFT		1
#define TPGEN_IDLE_MASK			BIT(1)
#define TPGEN_TPGEN_ENABLE_SHIFT	0
#define TPGEN_TPGEN_ENABLE_MASK		BIT(0)

/* TPGEN INTERRUPT */
#define TPGEN_UPDATE_INTDISABLE_SHIFT	14
#define TPGEN_UPDATE_INTDISABLE_MASK	BIT(14)
#define TPGEN_UPDATE_INTENB_SHIFT	13
#define TPGEN_UPDATE_INTENB_MASK	BIT(13)
#define TPGEN_UPDATE_INTPEND_SHIFT	12
#define TPGEN_UPDATE_INTPEND_MASK	BIT(12)
#define TPGEN_IRQ_OVF_INTDISABLE_SHIFT	10
#define TPGEN_IRQ_OVF_INTDISABLE_MASK	BIT(10)
#define TPGEN_IRQ_OVF_INTENB_SHIFT	9
#define TPGEN_IRQ_OVF_INTENB_MASK	BIT(9)
#define TPGEN_IRQ_OVF_INTPEND_SHIFT	8
#define TPGEN_IRQ_OVF_INTPEND_MASK	BIT(8)
#define TPGEN_IRQ_IDLE_INTDISABLE_SHIFT 6
#define TPGEN_IRQ_IDLE_INTDISABLE_MASK	BIT(6)
#define TPGEN_IRQ_IDLE_INTENB_SHIFT	5
#define TPGEN_IRQ_IDLE_INTENB_MASK	BIT(5)
#define TPGEN_IRQ_IDLE_INTPEND_SHIFT	4
#define TPGEN_IRQ_IDLE_INTPEND_MASK	BIT(4)
#define TPGEN_IRQ_DONE_INTDISABLE_SHIFT 2
#define TPGEN_IRQ_DONE_INTDISABLE_MASK	BIT(2)
#define TPGEN_IRQ_DONE_INTENB_SHIFT	1
#define TPGEN_IRQ_DONE_INTENB_MASK	BIT(1)
#define TPGEN_IRQ_DONE_INTPEND_SHIFT	0
#define TPGEN_IRQ_DONE_INTPEND_MASK	BIT(0)

/* TPGEN USER DEF HEADER 0 */
#define TPGEN_ENC_HEADER_0_SHIFT	0
#define TPGEN_ENC_HEADER_0_MASK		GENMASK(31, 0)

/* TPGEN USER DEF HEADER 1 */
#define TPGEN_ENC_HEADER_1_SHIFT	0
#define TPGEN_ENC_HEADER_1_MASK		GENMASK(31, 0)

/* TPGEN USER DEF HEADER 2 */
#define TPGEN_ENC_HEADER_2_SHIFT	0
#define TPGEN_ENC_HEADER_2_MASK		GENMASK(31, 0)

/* TPGEN USER DEF HEADER 3 */
#define TPGEN_ENC_HEADER_3_SHIFT	0
#define TPGEN_ENC_HEADER_3_MASK		GENMASK(31, 0)

/* TPGEN IMG SIZE */
#define TPGEN_ENC_WIDTH_SHIFT		16
#define TPGEN_ENC_WIDTH_MASK		GENMASK(31, 16)
#define TPGEN_ENC_HEIGHT_SHIFT		0
#define TPGEN_ENC_HEIGHT_MASK		GENMASK(15, 0)

/* TPGEN CTRL */
#define TPGEN_ENC_IMG_TYPE_SHIFT	18
#define TPGEN_ENC_IMG_TYPE_MASK		GENMASK(19, 18)
#define TPGEN_ENC_FIELD_SHIFT		16
#define TPGEN_ENC_FIELD_MASK		GENMASK(17, 16)
#define TPGEN_ENC_SECURE_SHIFT		15
#define TPGEN_ENC_SECURE_MASK		BIT(15)
#define TPGEN_ENC_REG_FIRE_SHIFT	14
#define TPGEN_ENC_REG_FIRE_MASK		BIT(14)
#define TPGEN_ENC_TID_SHIFT		0
#define TPGEN_ENC_TID_MASK		GENMASK(13, 0)

struct nxs_tpgen {
	struct nxs_dev nxs_dev;
	struct regmap *reg;
	u32 offset;
	u32 irq;
	atomic_t open_count;
};

enum tpgen_format_type {
	TPGEN_FORMAT_YUV = 0,
	TPGEN_FORMAT_RGB = 1,
	TPGEN_FORMAT_BAYER = 3,
};

#define nxs_to_tpgen(dev)	container_of(dev, struct nxs_tpgen, nxs_dev)

static int tpgen_get_interrupt_pending_number(struct nxs_tpgen *tpgen);
static void tpgen_clear_interrupt_pending(const struct nxs_dev *pthis,
					  int type);

static irqreturn_t tpgen_irq_handler(void *priv)
{
	struct nxs_tpgen *tpgen = (struct nxs_tpgen *)priv;
	struct nxs_dev *nxs_dev = &tpgen->nxs_dev;
	struct nxs_irq_callback *callback;
	unsigned long flags;

	spin_lock_irqsave(&nxs_dev->irq_lock, flags);
	list_for_each_entry(callback, &nxs_dev->irq_callback, list) {
		if (callback)
			callback->handler(nxs_dev, callback->data);
		else {
			int int_pending_num =
				tpgen_get_interrupt_pending_number(tpgen);

			if (int_pending_num) {
				dev_info(tpgen->nxs_dev.dev, "[%s] int_num = 0x%x\n",
					 __func__, int_pending_num);
				tpgen_clear_interrupt_pending(&tpgen->nxs_dev,
							      int_pending_num);
			}
		}
	}
	return IRQ_HANDLED;
}

static bool tpgen_check_busy(struct nxs_tpgen *tpgen)
{
	u32 busy;

	regmap_read(tpgen->reg, tpgen->offset + TPGEN_BASIC_CTRL, &busy);
	return ~(busy & TPGEN_IDLE_MASK);
}

static void tpgen_set_realtime_mode(struct nxs_tpgen *tpgen, u32 mode)
{
	dev_info(tpgen->nxs_dev.dev, "[%s] mode = %d\n",
		 __func__, mode);
	regmap_update_bits(tpgen->reg, tpgen->offset + TPGEN_BASIC_CTRL,
			   TPGEN_REALTIME_MODE_MASK,
			   mode << TPGEN_REALTIME_MODE_SHIFT);
}

static int tpgen_get_realtime_mode(struct nxs_tpgen *tpgen)
{
	u32 mode;

	dev_info(tpgen->nxs_dev.dev, "[%s]\n", __func__);
	regmap_read(tpgen->reg, tpgen->offset + TPGEN_BASIC_CTRL, &mode);
	return ((mode & TPGEN_REALTIME_MODE_MASK) >> TPGEN_REALTIME_MODE_SHIFT);
}

static void tpgen_config_stream(struct nxs_tpgen *tpgen)
{
	u32 delay, nclk_per_pixel, enc, mode;

	dev_info(tpgen->nxs_dev.dev, "[%s]\n", __func__);
	/* p_tpgen_parameter->realtime_mode   = 1; */
	/* set the realtime mode as '0' just for test at the first */
	tpgen_set_realtime_mode(tpgen, false);
	/* p_tpgen_parameter->nclk_per_twopix = 1; */
	/* 0 : 1 clk per two pixel data is genterate */
	/* 1 : 2 clk per two pixel data is genterate */
	/* 2 : 3 clk per two pixel data is genterate */
	nclk_per_pixel = 1;
	regmap_update_bits(tpgen->reg, tpgen->offset + TPGEN_BASIC_CTRL,
			   TPGEN_NCLK_PER_TWOPIX_MASK,
			   nclk_per_pixel << TPGEN_NCLK_PER_TWOPIX_SHIFT);
	/* p_tpgen_parameter->tp_gen_mode     = 0 ;*/
	/* tp_gen_mode0: horizental pixel counter */
	/* tp_gen_mode1: color bar */
	/* tp_gen_mode2: bayer */
	mode = 0;
	/* explanation in the spec said '0' means color bar mode.*/
	regmap_update_bits(tpgen->reg, tpgen->offset + TPGEN_BASIC_CTRL,
			   TPGEN_TPGEN_MODE_MASK,
			   mode << TPGEN_TPGEN_MODE_SHIFT);
	if (tpgen_get_realtime_mode(tpgen)) {
		/* p_tpgen_parameter->v2h_delay     = 10 ;*/
		/* p_tpgen_parameter->h2h_delay     = 10 ;*/
		/* p_tpgen_parameter->h2v_delay     = 10 ;*/
		/* p_tpgen_parameter->v2v_delay     = 10 ;*/
		delay = 10;
		regmap_write(tpgen->reg, tpgen->offset + TPGEN_V2H_DLY_CTRL,
			     delay);
		regmap_write(tpgen->reg, tpgen->offset + TPGEN_H2H_DLY_CTRL,
			     delay);
		regmap_write(tpgen->reg, tpgen->offset + TPGEN_H2V_DLY_CTRL,
			     delay);
		regmap_write(tpgen->reg, tpgen->offset + TPGEN_V2V_DLY_CTRL,
			     delay);
	}
	/* p_tpgen_parameter->enc_header_0  = 0 ; */
	/* p_tpgen_parameter->enc_header_1  = 0 ; */
	/* p_tpgen_parameter->enc_header_2  = 0 ; */
	/* p_tpgen_parameter->enc_header_3  = 0 ; */
	regmap_write(tpgen->reg, tpgen->offset + TPGEN_USERDEF_HEADER0, 0);
	regmap_write(tpgen->reg, tpgen->offset + TPGEN_USERDEF_HEADER1, 0);
	regmap_write(tpgen->reg, tpgen->offset + TPGEN_USERDEF_HEADER2, 0);
	regmap_write(tpgen->reg, tpgen->offset + TPGEN_USERDEF_HEADER3, 0);
	/* p_tpgen_parameter->enc_field     = 0 ; */
	enc = 0;
	regmap_update_bits(tpgen->reg, tpgen->offset + TPGEN_CTRL,
			   TPGEN_ENC_FIELD_MASK,
			   enc << TPGEN_ENC_FIELD_SHIFT);
	/* p_tpgen_parameter->enc_secure    = 0 ; */
	enc = 0;
	regmap_update_bits(tpgen->reg, tpgen->offset + TPGEN_CTRL,
			   TPGEN_ENC_SECURE_MASK,
			   enc << TPGEN_ENC_SECURE_SHIFT);
	/* p_tpgen_parameter->enc_reg_fire  = 1 ; */
	enc = 1;
	regmap_update_bits(tpgen->reg, tpgen->offset + TPGEN_CTRL,
			   TPGEN_ENC_REG_FIRE_MASK,
			   enc << TPGEN_ENC_REG_FIRE_SHIFT);
}

static int tpgen_set_enable(struct nxs_tpgen *tpgen, bool enable)
{
	dev_info(tpgen->nxs_dev.dev, "[%s] - %s\n",
		 __func__, (enable)?"enable":"disable");
	return regmap_update_bits(tpgen->reg, tpgen->offset + TPGEN_BASIC_CTRL,
				  TPGEN_TPGEN_ENABLE_MASK,
				  enable << TPGEN_TPGEN_ENABLE_SHIFT);
}

static int tpgen_reset_register(struct nxs_tpgen *tpgen)
{
	dev_info(tpgen->nxs_dev.dev, "[%s]\n", __func__);
	return regmap_update_bits(tpgen->reg, tpgen->offset + TPGEN_BASIC_CTRL,
				  TPGEN_REG_CLEAR_MASK,
				  1 << TPGEN_REG_CLEAR_SHIFT);
}

static void tpgen_set_interrupt_enable(const struct nxs_dev *pthis, int type,
				       bool enable)
{
	struct nxs_tpgen *tpgen = nxs_to_tpgen(pthis);
	u32 mask_v = NXS_EVENT_NONE;

	dev_info(pthis->dev, "[%s] type - %d, %s\n",
		 __func__, type, (enable)?"enable":"disable");
	if (enable) {
		if (type && NXS_EVENT_IDLE)
			mask_v |= TPGEN_IRQ_IDLE_INTENB_MASK;
		if (type && NXS_EVENT_UPDATE)
			mask_v |= TPGEN_UPDATE_INTENB_MASK;
		if (type && NXS_EVENT_DONE)
			mask_v |= TPGEN_IRQ_DONE_INTENB_MASK;
		if (type && NXS_EVENT_ERR)
			mask_v |= TPGEN_IRQ_OVF_INTENB_MASK;
	} else {
		if (type && NXS_EVENT_IDLE)
			mask_v |= TPGEN_IRQ_IDLE_INTDISABLE_MASK;
		if (type && NXS_EVENT_UPDATE)
			mask_v |= TPGEN_UPDATE_INTDISABLE_MASK;
		if (type && NXS_EVENT_DONE)
			mask_v |= TPGEN_IRQ_DONE_INTDISABLE_MASK;
		if (type && NXS_EVENT_ERR)
			mask_v |= TPGEN_IRQ_OVF_INTDISABLE_MASK;
	}
	regmap_update_bits(tpgen->reg, tpgen->offset + TPGEN_INTERRUPT,
			   mask_v, mask_v);
}

static bool tpgen_get_interrupt_enable(const struct nxs_dev *pthis, int type)
{
	struct nxs_tpgen *tpgen = nxs_to_tpgen(pthis);
	u32 status = 0, mask_v = 0;

	dev_info(pthis->dev, "[%s] type - %d\n",
		 __func__, type);
	regmap_read(tpgen->reg, tpgen->offset + TPGEN_INTERRUPT, &status);
	if (type && NXS_EVENT_IDLE)
		mask_v |= TPGEN_IRQ_IDLE_INTENB_MASK;
	if (type && NXS_EVENT_UPDATE)
		mask_v |= TPGEN_UPDATE_INTENB_MASK;
	if (type && NXS_EVENT_DONE)
		mask_v |= TPGEN_IRQ_DONE_INTENB_MASK;
	if (type && NXS_EVENT_ERR)
		mask_v |= TPGEN_IRQ_OVF_INTENB_MASK;
	return (status & mask_v);
}

static bool tpgen_get_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	struct nxs_tpgen *tpgen = nxs_to_tpgen(pthis);
	u32 status = 0, mask_v = 0;

	dev_info(pthis->dev, "[%s] type - %d\n",
		 __func__, type);
	regmap_read(tpgen->reg, tpgen->offset + TPGEN_INTERRUPT, &status);
	if (type && NXS_EVENT_IDLE)
		mask_v |= TPGEN_IRQ_IDLE_INTPEND_MASK;
	if (type && NXS_EVENT_UPDATE)
		mask_v |= TPGEN_UPDATE_INTPEND_MASK;
	if (type && NXS_EVENT_DONE)
		mask_v |= TPGEN_IRQ_DONE_INTPEND_MASK;
	if (type && NXS_EVENT_ERR)
		mask_v |= TPGEN_IRQ_OVF_INTPEND_MASK;
	return (status & mask_v);
}

static int tpgen_get_interrupt_pending_number(struct nxs_tpgen *tpgen)
{
	u32 status = 0, ret = NXS_EVENT_NONE;

	dev_info(tpgen->nxs_dev.dev, "[%s]\n", __func__);
	regmap_read(tpgen->reg, tpgen->offset + TPGEN_INTERRUPT, &status);
	if (status && TPGEN_IRQ_IDLE_INTPEND_MASK)
		ret |= NXS_EVENT_IDLE;
	if (status && TPGEN_UPDATE_INTPEND_MASK)
		ret |= NXS_EVENT_UPDATE;
	if (status && TPGEN_IRQ_DONE_INTPEND_MASK)
		ret |= NXS_EVENT_DONE;
	if (status && TPGEN_IRQ_OVF_INTPEND_MASK)
		ret |= NXS_EVENT_ERR;
	return ret;
}

static void tpgen_clear_interrupt_pending(const struct nxs_dev *pthis, int type)
{
	struct nxs_tpgen *tpgen = nxs_to_tpgen(pthis);
	u32 mask_v = NXS_EVENT_NONE;

	dev_info(pthis->dev, "[%s] type - %d\n", __func__, type);
	if (type && NXS_EVENT_IDLE)
		mask_v |= TPGEN_IRQ_IDLE_INTPEND_MASK;
	if (type && NXS_EVENT_UPDATE)
		mask_v |= TPGEN_UPDATE_INTPEND_MASK;
	if (type && NXS_EVENT_DONE)
		mask_v |= TPGEN_IRQ_DONE_INTPEND_MASK;
	if (type && NXS_EVENT_ERR)
		mask_v |= TPGEN_IRQ_OVF_INTPEND_MASK;
	regmap_update_bits(tpgen->reg, tpgen->offset + TPGEN_INTERRUPT,
			   mask_v, mask_v);
}

static int tpgen_open(const struct nxs_dev *pthis)
{
	struct nxs_tpgen *tpgen = nxs_to_tpgen(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	if (atomic_read(&tpgen->open_count) == 0) {
		int ret;
		/* clock enable */
		/* reset */
		ret = request_irq(tpgen->irq, tpgen_irq_handler,
				  IRQF_TRIGGER_NONE,
				  "nxs-tpgen", tpgen);
		if (ret < 0) {
			dev_err(pthis->dev, "failed to request irq : %d\n",
				ret);
			return ret;
		}
	}
	atomic_inc(&tpgen->open_count);
	return 0;
}

static int tpgen_close(const struct nxs_dev *pthis)
{
	struct nxs_tpgen *tpgen = nxs_to_tpgen(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	if (tpgen_check_busy(tpgen)) {
		dev_err(pthis->dev, "tpgen is busy\n");
		return -EBUSY;
	}
	atomic_dec(&tpgen->open_count);
	if (atomic_read(&tpgen->open_count) == 0) {
		tpgen_reset_register(tpgen);
		free_irq(tpgen->irq, tpgen);
		/* clock disable */
	}
	return 0;
}

static int tpgen_start(const struct nxs_dev *pthis)
{
	struct nxs_tpgen *tpgen = nxs_to_tpgen(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	return tpgen_set_enable(tpgen, true);
}

static int tpgen_stop(const struct nxs_dev *pthis)
{
	struct nxs_tpgen *tpgen = nxs_to_tpgen(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	return tpgen_set_enable(tpgen, false);
}

/* write - clear dirtyflag for TestPatternGenerator */
static int tpgen_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_tpgen *tpgen = nxs_to_tpgen(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;
	return regmap_write(tpgen->reg, TPGEN_DIRTYSET_OFFSET,
			    TPGEN_DIRTY);
}

static int tpgen_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_tpgen *tpgen = nxs_to_tpgen(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	return regmap_update_bits(tpgen->reg, tpgen->offset + TPGEN_CTRL,
				  TPGEN_ENC_TID_MASK,
				  tid1 << TPGEN_ENC_TID_SHIFT);
}

static int tpgen_set_format(const struct nxs_dev *pthis,
			    const struct nxs_control *pparam)
{
	struct nxs_tpgen *tpgen = nxs_to_tpgen(pthis);
	u32 size, type;

	dev_info(pthis->dev, "[%s]\n", __func__);
	tpgen_config_stream(tpgen);
	/* p_tpgen_parameter->enc_width     = 64 ; */
	/* p_tpgen_parameter->enc_height    = 1 ; */
	size = ((pparam->u.format.width << TPGEN_ENC_WIDTH_SHIFT) |
		(pparam->u.format.height));
	regmap_write(tpgen->reg, TPGEN_IMG_SIZE, size);
	/* p_tpgen_parameter->enc_img_type  = 0 ; 0: YUV, 1: RGB, 3: BAYER */
	switch (pparam->u.format.pixelformat) {
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YYUV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_NV24:
	case V4L2_PIX_FMT_NV42:
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV21M:
	case V4L2_PIX_FMT_NV16M:
	case V4L2_PIX_FMT_NV61M:
		type = TPGEN_FORMAT_YUV;
		break;
	case V4L2_PIX_FMT_ARGB555:
	case V4L2_PIX_FMT_XRGB555:
	case V4L2_PIX_FMT_RGB565:
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_RGB24:
	case V4L2_PIX_FMT_BGR32:
	case V4L2_PIX_FMT_ABGR32:
	case V4L2_PIX_FMT_XBGR32:
	case V4L2_PIX_FMT_RGB32:
	case V4L2_PIX_FMT_ARGB32:
	case V4L2_PIX_FMT_XRGB32:
		type = TPGEN_FORMAT_RGB;
		break;
	default:
		/*
		 * Todo
		 * Currently not support bayer format
		 *
		 */
		type = TPGEN_FORMAT_BAYER;
		break;
	}
	return regmap_update_bits(tpgen->reg, tpgen->offset + TPGEN_CTRL,
				  TPGEN_ENC_IMG_TYPE_MASK,
				  type << TPGEN_ENC_IMG_TYPE_SHIFT);
}

static int tpgen_get_format(const struct nxs_dev *pthis,
			    struct nxs_control *pparam)
{
	return 0;
}

static int nxs_tpgen_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_tpgen *tpgen;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	tpgen = devm_kzalloc(&pdev->dev, sizeof(*tpgen), GFP_KERNEL);
	if (!tpgen)
		return -ENOMEM;

	nxs_dev = &tpgen->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	tpgen->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						     "syscon");
	if (IS_ERR(tpgen->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(tpgen->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	tpgen->offset = res->start;
	tpgen->irq = platform_get_irq(pdev, 0);
	atomic_set(&tpgen->open_count, 0);

	nxs_dev->set_interrupt_enable = tpgen_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = tpgen_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = tpgen_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = tpgen_clear_interrupt_pending;
	nxs_dev->open = tpgen_open;
	nxs_dev->close = tpgen_close;
	nxs_dev->start = tpgen_start;
	nxs_dev->stop = tpgen_stop;
	nxs_dev->set_dirty = tpgen_set_dirty;
	nxs_dev->set_tid = tpgen_set_tid;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_FORMAT;
	nxs_dev->dev_services[0].set_control = tpgen_set_format;
	nxs_dev->dev_services[0].get_control = tpgen_get_format;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, tpgen);

	return 0;
}

static int nxs_tpgen_remove(struct platform_device *pdev)
{
	struct nxs_tpgen *tpgen = platform_get_drvdata(pdev);

	if (tpgen)
		unregister_nxs_dev(&tpgen->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_tpgen_match[] = {
	{ .compatible = "nexell,tpgen-nxs-1.0", },
	{},
};

static struct platform_driver nxs_tpgen_driver = {
	.probe	= nxs_tpgen_probe,
	.remove	= nxs_tpgen_remove,
	.driver	= {
		.name = "nxs-tpgen",
		.of_match_table = of_match_ptr(nxs_tpgen_match),
	},
};

static int __init nxs_tpgen_init(void)
{
	return platform_driver_register(&nxs_tpgen_driver);
}
/* subsys_initcall(nxs_tpgen_init); */
fs_initcall(nxs_tpgen_init);

static void __exit nxs_tpgen_exit(void)
{
	platform_driver_unregister(&nxs_tpgen_driver);
}
module_exit(nxs_tpgen_exit);

MODULE_DESCRIPTION("Nexell Stream TPGEN driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");

