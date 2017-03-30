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
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/mfd/syscon.h>
#include <linux/platform_device.h>

#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_res_manager.h>
#include <linux/soc/nexell/nxs_plane_format.h>

#define DMAR_DIRTYSET_OFFSET	0x0000
#define DMAR_DIRTYCLR_OFFSET	0x0010
#define DMAR0_DIRTY		BIT(0)
#define DMAR1_DIRTY		BIT(1)
#define DMAR2_DIRTY		BIT(2)
#define DMAR3_DIRTY		BIT(3)
#define DMAR4_DIRTY		BIT(4)
#define DMAR5_DIRTY		BIT(5)
#define DMAR6_DIRTY		BIT(6)
#define DMAR7_DIRTY		BIT(7)
#define DMAR8_DIRTY		BIT(8)
#define DMAR9_DIRTY		BIT(9)

/* DMAR Register Offset */
#define DMAR_CTRL0			0x00
#define DMAR_CTRL1			0x04
#define DMAR_CTRL2			0x08
#define DMAR_CTRL3			0x0C
#define DMAR_CTRL4			0x10
#define DMAR_CTRL5			0x14
#define DMAR_CTRL6			0x18
#define DMAR_CTRL7			0x1c
#define DMAR_CTRL8			0x20
#define DMAR_CTRL9			0x24
#define DMAR_CTRL10			0x28
#define DMAR_CTRL11			0x2c
#define DMAR_CTRL12			0x30
#define DMAR_CTRL13			0x34
#define DMAR_CTRL14			0x38
#define DMAR_CTRL15			0x3c
#define DMAR_CTRL16			0x40
#define DMAR_CTRL17			0x44
#define DMAR_CTRL18			0x48
#define DMAR_CTRL19			0x4c
#define DMAR_CTRL20			0x50
#define DMAR_CTRL21			0x54
#define DMAR_CTRL22			0x58
#define DMAR_CTRL23			0x5C
#define DMAR_CTRL24			0x60
#define DMAR_CTRL25			0x64
#define DMAR_CTRL26			0x68
#define DMAR_CTRL27			0x6c
#define DMAR_CTRL28			0x70
#define DMAR_CTRL29			0x74
#define DMAR_CTRL30			0x78
#define DMAR_CTRL31			0x7c
#define DMAR_CTRL32			0x80
#define DMAR_CTRL33			0x84
#define DMAR_CTRL34			0x88
#define DMAR_CTRL35			0x8c
#define DMAR_CTRL36			0x90
#define DMAR_CTRL37			0x94
#define DMAR_CTRL38			0x98
#define DMAR_CTRL39			0x9c
#define DMAR_CTRL40			0xa0
#define DMAR_CTRL41			0xa4
#define DMAR_CTRL42			0xa8
#define DMAR_CTRL43			0xaC
#define DMAR_CTRL44			0xb0
#define DMAR_CTRL45			0xb4
#define DMAR_CTRL46			0xb8
#define DMAR_CTRL47			0xbc
#define DMAR_CTRL48			0xc0
#define DMAR_CTRL49			0xc4
#define DMAR_CTRL50			0xc8
#define DMAR_CTRL51			0xcc
#define DMAR_CTRL52			0xd0
#define DMAR_CTRL53			0xd4
#define DMAR_CTRL54			0xd8
#define DMAR_CTRL55			0xdc
#define DMAR_CTRL56			0xe0
#define DMAR_CTRL57			0xe4
#define DMAR_CTRL58			0xe8
#define DMAR_CTRL59			0xec
#define DMAR_FORMAT0			0xf0
#define DMAR_P0_COMP0			0xf4
#define DMAR_P0_COMP1			0xf8
#define DMAR_P0_COMP2			0xfc
#define DMAR_P0_COMP3			0x100
#define DMAR_P1_COMP0			0x104
#define DMAR_P1_COMP1			0x108
#define DMAR_P1_COMP2			0x10c
#define DMAR_P1_COMP3			0x110
#define DMAR_DIT			0x114
#define DMAR_DITVAL0			0x118
#define DMAR_DITVAL1			0x11c
#define DMAR_DEBUG			0x120
#define DMAR_INT			0x124
/* DMAR_CTRL0 */
#define DMAR_REG_CLEAR_SHIFT			16
#define DMAR_REG_CLEAR_MASK			BIT(16)
#define DMAR_TZPROT_SHIFT			15
#define DMAR_TZPROT_MASK			BIT(15)
#define DMAR_UPDATEFIELD_SHIFT			13
#define DMAR_UPDATEFIELD_MASK			GENMASK(14, 13)
#define DMAR_MAXFIELD_SHIFT			11
#define DMAR_MAXFIELD_MASK			GENMASK(12, 11)
#define DMAR_TMODE_SHIFT			7
#define DMAR_TMODE_MASK				GENMASK(10, 7)
#define DMAR_IMG_TYPE_SHIFT			5
#define DMAR_IMG_TYPE_MASK			GENMASK(6, 5)
#define DMAR_CPU_REG_FIRE_SHIFT			4
#define DMAR_CPU_REG_FIRE_MASK			BIT(4)
#define DMAR_DPC_ENABLE_SHIFT			3
#define DMAR_DPC_ENABLE_MASK			BIT(3)
#define DMAR_IDLE_SHIFT				2
#define DMAR_IDLE_MASK				BIT(2)
#define DMAR_IDLE_FLUSH_SHIFT			1
#define DMAR_IDLE_FLUSH_MASK			BIT(1)
#define DMAR_CPU_START_SHIFT			0
#define DMAR_CPU_START_MASK			BIT(0)
/* DMAR_CTRL1 */
#define DMAR_FIELD_SHIFT			14
#define DMAR_FIELD_MASK				GENMASK(15, 14)
#define DMAR_TID_SHIFT				0
#define DMAR_TID_MASK				GENMASK(13, 0)
/* DMAR_CTRL2 */
#define DMAR_DMAR_WIDTH_SHIFT			16
#define DMAR_DMAR_WIDTH_MASK			GENMASK(31, 16)
#define DMAR_DMAR_HEIGHT_SHIFT			0
#define DMAR_DMAR_HEIGHT_MASK			GENMASK(15, 0)
/* DMAR_CTRL59 */
#define DMAR_1P_EVEN_HALF_HEIGHT_R_SHIFT	7 /* 4thframe */
#define DMAR_1P_EVEN_HALF_HEIGHT_R_MASK		BIT(7)
#define DMAR_1P_HALF_HEIGHT_R_SHIFT		6 /* 3thframe */
#define DMAR_1P_HALF_HEIGHT_R_MASK		BIT(6)
#define DMAR_1P_EVEN_HALF_HEIGHT_SHIFT		5 /* 2thframe */
#define DMAR_1P_EVEN_HALF_HEIGHT_MASK		BIT(5)
#define DMAR_1P_HALF_HEIGHT_SHIFT		4 /* 1stframe */
#define DMAR_1P_HALF_HEIGHT_MASK		BIT(4)
#define DMAR_2P_EVEN_HALF_HEIGHT_R_SHIFT	3 /* 4thframe */
#define DMAR_2P_EVEN_HALF_HEIGHT_R_MASK		BIT(3)
#define DMAR_2P_HALF_HEIGHT_R_SHIFT		2 /* 3thframe */
#define DMAR_2P_HALF_HEIGHT_R_MASK		BIT(2)
#define DMAR_2P_EVEN_HALF_HEIGHT_SHIFT		1 /* 2thframe */
#define DMAR_2P_EVEN_HALF_HEIGHT_MASK		BIT(1)
#define DMAR_2P_HALF_HEIGHT_SHIFT		0 /* 1stframe */
#define DMAR_2P_HALF_HEIGHT_MASK		BIT(0)

/* DMAR BLOCK READBIT & COUNT */
#define DMAR_BLOCK_READBIT_SHIFT		16
#define DMAR_BLOCK_READBIT_MASK			GENMASK(18, 16)
#define DMAR_BLOCK_COUNT_SHIFT			0
#define DMAR_BLOCK_COUNT_MASK			GENMASK(15, 0)

/* DMAR_FORMAT0 */
#define DMAR_USE_AVERAGE_VALLUE_SHIFT		18
#define DMAR_USE_AVERAGE_VALLUE_MASK		BIT(18)
#define DMAR_PUT_DUMMY_TYPE_SHIFT		16
#define DMAR_PUT_DUMMY_TYPE_MASK		GENMASK(17, 16)
#define DMAR_TOTAL_BITWIDTH_2ND_PL_SHIFT	8
#define DMAR_TOTAL_BITWIDTH_2ND_PL_MASK		GENMASK(15, 8)
#define DMAR_TOTAL_BITWIDTH_1ST_PL_SHIFT	0
#define DMAR_TOTAL_BITWIDTH_1ST_PL_MASK		GENMASK(8, 0)
/* DMAR_P_COMP */
#define DMAR_P_COMP_SIZE			0x4
#define DMAR_P_COMP_COUNT			4
#define DMAR_P_COMP_STARTBIT_SHIFT		17
#define DMAR_P_COMP_STARTBIT_MASK		GENMASK(23, 17)
#define DMAR_P_COMP_BITWIDTH_SHIFT		12
#define DMAR_P_COMP_BITWIDTH_MASK		GENMASK(16, 12)
#define DMAR_P_COMP_IS_2ND_PL_SHIFT		11
#define DMAR_P_COMP_IS_2ND_PL_MASK		BIT(11)
#define DMAR_P_COMP_USE_USERDEF_SHIFT		10
#define DMAR_P_COMP_USE_USERDEF_MASK		BIT(10)
#define DMAR_P_COMP_USERDEF_SHIFT		0
#define DMAR_P_COMP_USERDEF_MASK		GENMASK(9, 0)
/* DMAR_DIT */
#define DMAR_COLOR_EXPAND_SHIFT			1
#define DMAR_COLOR_EXPAND_MASK			BIT(1)
#define DMAR_COLOR_DITHER_SHIFT			0
#define DMAR_COLOR_DITHER_MASK			BIT(0)
/* DMAR_DITVAL0 */
#define DMAR_DMAR_DITHER_X3Y1_SHIFT		28
#define DMAR_DMAR_DITHER_X3Y1_MASK		GENMASK(31, 28)
#define DMAR_DMAR_DITHER_X2Y1_SHIFT		24
#define DMAR_DMAR_DITHER_X2Y1_MASK		GENMASK(27, 24)
#define DMAR_DMAR_DITHER_X1Y1_SHIFT		20
#define DMAR_DMAR_DITHER_X1Y1_MASK		GENMASK(23, 20)
#define DMAR_DMAR_DITHER_X0Y1_SHIFT		16
#define DMAR_DMAR_DITHER_X0Y1_MASK		GENMASK(19, 16)
#define DMAR_DMAR_DITHER_X3Y0_SHIFT		12
#define DMAR_DMAR_DITHER_X3Y0_MASK		GENMASK(15, 12)
#define DMAR_DMAR_DITHER_X2Y0_SHIFT		8
#define DMAR_DMAR_DITHER_X2Y0_MASK		GENMASK(11, 8)
#define DMAR_DMAR_DITHER_X1Y0_SHIFT		4
#define DMAR_DMAR_DITHER_X1Y0_MASK		GENMASK(7, 4)
#define DMAR_DMAR_DITHER_X0Y0_SHIFT		0
#define DMAR_DMAR_DITHER_X0Y0_MASK		GENMASK(3, 0)
/* DMAR_INT */
#define DMAR_ERR_PRESP_INTDISABLE_SHIFT		10
#define DMAR_ERR_PRESP_INTDISABLE_MASK		BIT(10)
#define DMAR_ERR_PRESP_INTENB_SHIFT		9
#define DMAR_ERR_PRESP_INTENB_MASK		BIT(9)
#define DMAR_ERR_PRESP_INTPEND_CLR_SHIFT	8
#define DMAR_ERR_PRESP_INTPEND_CLR_MASK		BIT(8)
#define DMAR_IDLE_PRESP_INTDISABLE_SHIFT	6
#define DMAR_IDLE_PRESP_INTDISABLE_MASK		BIT(6)
#define DMAR_IDLE_PRESP_INTENB_SHIFT		5
#define DMAR_IDLE_PRESP_INTENB_MASK		BIT(5)
#define DMAR_IDLE_PRESP_INTPEND_CLR_SHIFT	4
#define DMAR_IDLE_PRESP_INTPEND_CLR_MASK	BIT(4)
#define DMAR_UPDATE_PRESP_INTDISABLE_SHIFT	2
#define DMAR_UPDATE_PRESP_INTDISABLE_MASK	BIT(2)
#define DMAR_UPDATE_PRESP_INTENB_SHIFT		1
#define DMAR_UPDATE_PRESP_INTENB_MASK		BIT(1)
#define DMAR_UPDATE_PRESP_INTPEND_CLR_SHIFT	0
#define DMAR_UPDATE_PRESP_INTPEND_CLR_MASK	BIT(0)

#define DMAR_PLANE_CONFIG_SIZE			0x7c

/* AXIM_DMARW_BASEADDR				0x20D00000*/
#define AXIM_DMARW_OFFSET			0x100000
#define AXIM_DMARW_SIZE				0x4000

#define AXIM_DMARW_ARQOS_OFFSET			0x00
#define AXIM_DMARW_ARQOS_SHIFT			0
#define AXIM_DMARW_ARQOS_MASK			GENMASK(3, 0)
#define AXIM_DMARW_AXQOS_USE_SHIFT		8
#define AXIM_DMARW_AXQOS_USE_MASK		BIT(8)
#define AXIM_DMARW_ARADDRH_OFFSET		0x20
#define AXIM_DMARW_ARADDRH_EN_OFFSET		0x28

/*#define SIMULATE_INTERRUPT*/

#ifdef SIMULATE_INTERRUPT
#include <linux/timer.h>
#endif

struct nxs_dmar {
#ifdef SIMULATE_INTERRUPT
	struct timer_list timer;
#endif
	struct nxs_dev nxs_dev;
	struct regmap *reg;
	u32 offset;
	u32 irq;
	bool is_m2m;
};

#define nxs_to_dmar(dev)	container_of(dev, struct nxs_dmar, nxs_dev)

static void dmar_clear_interrupt_pending(const struct nxs_dev *pthis,
					 enum nxs_event_type type);
static enum nxs_event_type
dmar_get_interrupt_pending_number(struct nxs_dmar *dmar);
static u32 dmar_get_interrupt_pending(const struct nxs_dev *pthis,
				      enum nxs_event_type type);

#ifdef SIMULATE_INTERRUPT
#define INT_TIMEOUT_MS				30

static void int_timer_func(void *priv)
{
	struct nxs_dmar *dmar = (struct nxs_dmar *)priv;
	struct nxs_dev *nxs_dev = &dmar->nxs_dev;
	struct nxs_irq_callback *callback;
	unsigned long flags;

	spin_lock_irqsave(&nxs_dev->irq_lock, flags);
	list_for_each_entry(callback, &nxs_dev->irq_callback, list) {
		if (callback)
			callback->handler(nxs_dev, callback->data);
	}
	spin_unlock_irqrestore(&nxs_dev->irq_lock, flags);
	mod_timer(&dmar->timer, jiffies + msecs_to_jiffies(INT_TIMEOUT_MS));
}
#else
static irqreturn_t dmar_irq_handler(void *priv)
{
	struct nxs_dmar *dmar = (struct nxs_dmar *)priv;
	struct nxs_dev *nxs_dev = &dmar->nxs_dev;
	struct nxs_irq_callback *callback;
	unsigned long flags;

	dev_info(nxs_dev->dev, "[%s] pend_status = 0x%x\n", __func__,
		 dmar_get_interrupt_pending(nxs_dev, NXS_EVENT_ALL));
	dmar_clear_interrupt_pending(nxs_dev,
				     dmar_get_interrupt_pending_number(dmar));
	spin_lock_irqsave(&nxs_dev->irq_lock, flags);
	list_for_each_entry(callback, &nxs_dev->irq_callback, list) {
		if (callback)
			callback->handler(nxs_dev, callback->data);
	}
	spin_unlock_irqrestore(&nxs_dev->irq_lock, flags);
	return IRQ_HANDLED;
}
#endif

static bool dmar_check_busy(struct nxs_dmar *dmar)
{
	u32 busy;

	regmap_read(dmar->reg, dmar->offset + DMAR_CTRL0, &busy);
	return ~(busy & DMAR_IDLE_MASK);
}

static void dmar_reset_register(struct nxs_dmar *dmar)
{
	dev_info(dmar->nxs_dev.dev, "[%s]\n", __func__);
	regmap_update_bits(dmar->reg, dmar->offset + DMAR_CTRL0,
			   DMAR_REG_CLEAR_MASK, 1 << DMAR_REG_CLEAR_SHIFT);
}

static void dmar_set_cpu_start(struct nxs_dmar *dmar, bool start)
{
	dev_info(dmar->nxs_dev.dev, "[%s] %s\n",
		__func__, (start)?"start":"stop");
	regmap_update_bits(dmar->reg, dmar->offset + DMAR_CTRL0,
			   DMAR_CPU_START_MASK, start << DMAR_CPU_START_SHIFT);
}

/*
 * dpc mode is needed to be set for the case when it's connected to
 * display gadget such as DPC, HDMI and LVDS to start automatically
 * except for the case is cpu mode
 */
static void dmar_set_dpc_enable(struct nxs_dmar *dmar, bool enable)
{
	u32 val, arqos;

	/*
	 * dpc mode - enable ARADDR_H and ARQOS
	 * m2m mode - disable ARADDR_H and ARQOS
	 */
	dev_info(dmar->nxs_dev.dev, "[%s] %s\n",
		__func__, (enable)?"enable":"disable");
	/* set ARADDR_H */
	regmap_write(dmar->reg, AXIM_DMARW_OFFSET +
		     ((dmar->nxs_dev.dev_inst_index * AXIM_DMARW_SIZE) +
		      AXIM_DMARW_ARADDRH_OFFSET),
		     enable);
	val = 0xFFFFFFFF;
	regmap_write(dmar->reg, AXIM_DMARW_OFFSET +
		     ((dmar->nxs_dev.dev_inst_index * AXIM_DMARW_SIZE) +
		      AXIM_DMARW_ARADDRH_EN_OFFSET),
		     val);
	/* set ARQOS */
	if (enable)
		arqos = 15;
	else
		arqos = 0;
	regmap_update_bits(dmar->reg, AXIM_DMARW_OFFSET +
			   ((dmar->nxs_dev.dev_inst_index * AXIM_DMARW_SIZE) +
			   AXIM_DMARW_ARQOS_OFFSET),
			   AXIM_DMARW_AXQOS_USE_MASK,
			   (1 << AXIM_DMARW_AXQOS_USE_SHIFT));
	regmap_update_bits(dmar->reg, AXIM_DMARW_OFFSET +
			   ((dmar->nxs_dev.dev_inst_index * AXIM_DMARW_SIZE) +
			   AXIM_DMARW_ARQOS_OFFSET),
			   AXIM_DMARW_ARQOS_MASK,
			   (arqos << AXIM_DMARW_ARQOS_SHIFT));
	/* dpc enable/disable */
	regmap_update_bits(dmar->reg, dmar->offset + DMAR_CTRL0,
			   DMAR_DPC_ENABLE_MASK,
			   enable << DMAR_DPC_ENABLE_SHIFT);
}

static void dmar_set_field(struct nxs_dmar *dmar, u32 field)
{
	dev_info(dmar->nxs_dev.dev, "[%s] field:%d\n",
		 __func__, field);
	/* 0: Progressive, 1: Interlace 3: 3D */
	regmap_update_bits(dmar->reg, dmar->offset + DMAR_CTRL0,
			   DMAR_MAXFIELD_MASK,
			   field << DMAR_MAXFIELD_SHIFT);
}

static u32 dmar_get_field(struct nxs_dmar *dmar)
{
	u32 field;

	dev_info(dmar->nxs_dev.dev, "[%s]\n", __func__);
	regmap_read(dmar->reg, dmar->offset + DMAR_CTRL0, &field);
	return ((field & DMAR_MAXFIELD_MASK) >> DMAR_MAXFIELD_SHIFT);
}

static int dmar_get_img_type(struct nxs_dmar *dmar)
{
	int type;

	dev_info(dmar->nxs_dev.dev, "[%s]\n", __func__);
	/* img type - 0: YUV, 1: RGB, 2: RAW */
	regmap_read(dmar->reg, dmar->offset + DMAR_CTRL0, &type);
	return ((type & DMAR_IMG_TYPE_MASK) >> DMAR_IMG_TYPE_SHIFT);
}

static void dmar_set_img_type(struct nxs_dmar *dmar, u32 type)
{
	dev_info(dmar->nxs_dev.dev, "[%s]\n", __func__);
	/* img type - 0: YUV, 1: RGB, 2: RAW */
	regmap_update_bits(dmar->reg, dmar->offset + DMAR_CTRL0,
			   DMAR_IMG_TYPE_MASK,
			   type << DMAR_IMG_TYPE_SHIFT);
}

static void dmar_set_size(struct nxs_dmar *dmar, u32 width, u32 height)
{
	dev_info(dmar->nxs_dev.dev, "[%s]\n", __func__);
	regmap_write(dmar->reg, dmar->offset + DMAR_CTRL2,
		     ((width << DMAR_DMAR_WIDTH_SHIFT) | height));
}

static u32 dmar_get_size(struct nxs_dmar *dmar)
{
	u32 size;

	dev_info(dmar->nxs_dev.dev, "[%s]\n", __func__);
	regmap_read(dmar->reg, dmar->offset + DMAR_CTRL2, &size);
	return size;
}

static u32 dmar_get_bpp(struct nxs_dmar *dmar, u32 plane_num)
{
	u32 value = 0, bitwidth;

	regmap_read(dmar->reg, dmar->offset + DMAR_FORMAT0, &value);
	if (plane_num)
		bitwidth = ((value & DMAR_TOTAL_BITWIDTH_2ND_PL_MASK) >>
			    DMAR_TOTAL_BITWIDTH_2ND_PL_SHIFT);
	else
		bitwidth = ((value & DMAR_TOTAL_BITWIDTH_1ST_PL_MASK) >>
			    DMAR_TOTAL_BITWIDTH_1ST_PL_SHIFT);

	return (bitwidth / 2);
}

static void dmar_set_block_config(struct nxs_dmar *dmar, u32 plane_num,
				  u32 buffer_index, u32 stride,
				  u32 byte_offset, u32 bit_offset)
{
	u32 readbyte, readbit, count, width, height, value;
	u32 bpp = dmar_get_bpp(dmar, buffer_index);

	dev_info(dmar->nxs_dev.dev,
		"[%s] plane_num:%d, buffer_index:%d, stride:%d\n",
		__func__, plane_num, buffer_index, stride);
	value = dmar_get_size(dmar);
	width = (value & DMAR_DMAR_WIDTH_MASK) >> DMAR_DMAR_WIDTH_SHIFT;
	height = value & DMAR_DMAR_HEIGHT_MASK;

	/*
	 * Todo
	 * currently not support 10 bit format
	 * for the case, it can't be calculated by bpp
	 * so it need a recalculation for 10 bit as each format
	 */
	if (bpp % 10) {
		if ((((width * bpp) / 8) == stride) && (!plane_num)) {
			readbyte = stride*height;
			count = 1;
			readbit = 7;
		} else {
			/* 2planes mode can't use block count '1' */
			readbyte = ((width*bpp) / 8);
			count = height;
			readbit = 7;
		}
	} else {
		/* 10bit mode */
		readbyte = ((width / 3) * 4);
		count = height;
		readbit = 7;
		if ((width % 3) == 1) {
			readbyte = readbyte + 2;
			readbit = 1;
		} else if ((width % 3) == 2) {
			readbyte = readbyte + 3;
			readbit = 3;
		}

	}
	dev_info(dmar->nxs_dev.dev,
		"[%s] blockreadbyte:%d, blockreadbit:%d, blockreadcount:%d\n",
		__func__, readbyte, readbit, count);
	regmap_write(dmar->reg,
		     dmar->offset + byte_offset +
		     (buffer_index * DMAR_PLANE_CONFIG_SIZE),
		     readbyte);
	value = readbit << DMAR_BLOCK_READBIT_SHIFT;
	value |= count << DMAR_BLOCK_COUNT_SHIFT;
	regmap_write(dmar->reg,
		     dmar->offset + bit_offset +
		     (buffer_index * DMAR_PLANE_CONFIG_SIZE),
		     value);
}

static u32 dmar_get_status(struct nxs_dmar *dmar, u32 offset, u32 mask,
			     u32 shift)
{
	u32 status;

	regmap_read(dmar->reg, dmar->offset + offset, &status);
	return ((status & mask) >> shift);
}

static void dmar_dump_register(const struct nxs_dev *pthis)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);
	int i, plane_num = 1;

	dev_info(pthis->dev, "============================================\n");
	dev_info(pthis->dev, "DMAR[%d] Dump Register : 0x%8x\n",
		 pthis->dev_inst_index, dmar->offset);

	dev_info(pthis->dev,
		 "TID:%d, Enable:%d, is_m2m:%d\n",
		 dmar_get_status(dmar,
				 DMAR_CTRL1,
				 DMAR_TID_MASK,
				 DMAR_TID_SHIFT),
		 dmar_get_status(dmar,
				 DMAR_CTRL0,
				 DMAR_CPU_START_MASK,
				 DMAR_CPU_START_SHIFT),
		 dmar->is_m2m);

	dev_info(pthis->dev,
		 "Image type:%s, width:%d, height:%d\n",
		 (dmar_get_img_type(dmar)) ? "RGB" : "YUV",
		 dmar_get_status(dmar,
				 DMAR_CTRL2,
				 DMAR_DMAR_WIDTH_MASK,
				 DMAR_DMAR_WIDTH_SHIFT),
		 dmar_get_status(dmar,
				 DMAR_CTRL2,
				 DMAR_DMAR_HEIGHT_MASK,
				 DMAR_DMAR_HEIGHT_SHIFT));

	dev_info(pthis->dev,
		 "Color Expand Enable:%d, Dither Enable:%d, Put Dummy:%d\n",
		 dmar_get_status(dmar,
				 DMAR_DIT,
				 DMAR_COLOR_EXPAND_MASK,
				 DMAR_COLOR_EXPAND_SHIFT),
		 dmar_get_status(dmar,
				 DMAR_DIT,
				 DMAR_COLOR_DITHER_MASK,
				 DMAR_COLOR_DITHER_SHIFT),
		 dmar_get_status(dmar,
				 DMAR_FORMAT0,
				 DMAR_PUT_DUMMY_TYPE_MASK,
				 DMAR_PUT_DUMMY_TYPE_SHIFT));

	dev_info(pthis->dev,
		 "[1st plane] Total Bitwidth:%d, Half Height enable:%d\n",
		 dmar_get_status(dmar,
				 DMAR_FORMAT0,
				 DMAR_TOTAL_BITWIDTH_1ST_PL_MASK,
				 DMAR_TOTAL_BITWIDTH_1ST_PL_SHIFT),
		 dmar_get_status(dmar,
				 DMAR_CTRL59,
				 DMAR_1P_HALF_HEIGHT_MASK,
				 DMAR_1P_HALF_HEIGHT_SHIFT));

	for (i = 0; i < DMAR_P_COMP_COUNT; i++) {
		dev_info(pthis->dev,
			 "[%d] start:%d, width:%d, is_2nd:%d userdef:%d-%d\n",
			 i,
			 dmar_get_status(dmar,
					 DMAR_P0_COMP0 + (i * DMAR_P_COMP_SIZE),
					 DMAR_P_COMP_STARTBIT_MASK,
					 DMAR_P_COMP_STARTBIT_SHIFT),
			 dmar_get_status(dmar,
					 DMAR_P0_COMP0 + (i * DMAR_P_COMP_SIZE),
					 DMAR_P_COMP_BITWIDTH_MASK,
					 DMAR_P_COMP_BITWIDTH_SHIFT),
			 dmar_get_status(dmar,
					 DMAR_P0_COMP0 + (i * DMAR_P_COMP_SIZE),
					 DMAR_P_COMP_IS_2ND_PL_MASK,
					 DMAR_P_COMP_IS_2ND_PL_SHIFT),
			 dmar_get_status(dmar,
					 DMAR_P0_COMP0 + (i * DMAR_P_COMP_SIZE),
					 DMAR_P_COMP_USE_USERDEF_MASK,
					 DMAR_P_COMP_USE_USERDEF_SHIFT),
			 dmar_get_status(dmar,
					 DMAR_P0_COMP0 + (i * DMAR_P_COMP_SIZE),
					 DMAR_P_COMP_USERDEF_MASK,
					 DMAR_P_COMP_USERDEF_SHIFT));
	}

	dev_info(pthis->dev,
		 "[2nd Plane] Total Bitwidth :%d, Half Height Enable:%d\n",
		 dmar_get_status(dmar,
				 DMAR_FORMAT0,
				 DMAR_TOTAL_BITWIDTH_2ND_PL_MASK,
				 DMAR_TOTAL_BITWIDTH_2ND_PL_SHIFT),
		 dmar_get_status(dmar,
				 DMAR_CTRL59,
				 DMAR_2P_HALF_HEIGHT_MASK,
				 DMAR_2P_HALF_HEIGHT_SHIFT));

	for (i = 0; i < DMAR_P_COMP_COUNT; i++) {
		dev_info(pthis->dev,
			 "[%d] start:%d, width:%d, is_2nd:%d userdef:%d-%d\n",
			 i,
			 dmar_get_status(dmar,
					 DMAR_P1_COMP0 + (i * DMAR_P_COMP_SIZE),
					 DMAR_P_COMP_STARTBIT_MASK,
					 DMAR_P_COMP_STARTBIT_SHIFT),
			 dmar_get_status(dmar,
					 DMAR_P1_COMP0 + (i * DMAR_P_COMP_SIZE),
					 DMAR_P_COMP_BITWIDTH_MASK,
					 DMAR_P_COMP_BITWIDTH_SHIFT),
			 dmar_get_status(dmar,
					 DMAR_P1_COMP0 + (i * DMAR_P_COMP_SIZE),
					 DMAR_P_COMP_IS_2ND_PL_MASK,
					 DMAR_P_COMP_IS_2ND_PL_SHIFT),
			 dmar_get_status(dmar,
					 DMAR_P1_COMP0 + (i * DMAR_P_COMP_SIZE),
					 DMAR_P_COMP_USE_USERDEF_MASK,
					 DMAR_P_COMP_USE_USERDEF_SHIFT),
			 dmar_get_status(dmar,
					 DMAR_P1_COMP0 + (i * DMAR_P_COMP_SIZE),
					 DMAR_P_COMP_USERDEF_MASK,
					 DMAR_P_COMP_USERDEF_SHIFT));
	}
	if (dmar_get_status(dmar, DMAR_FORMAT0,
			    DMAR_TOTAL_BITWIDTH_2ND_PL_MASK,
			    DMAR_TOTAL_BITWIDTH_2ND_PL_SHIFT))
		plane_num = 2;

	dev_info(pthis->dev,
		 "Field:%s, Num of Planes:%d\n",
		 (dmar_get_field(dmar)) ? "Interlaced" : "Progressive",
		 plane_num);

	for (i = 0; i < plane_num; i++) {
		dev_info(pthis->dev,
			 "[PlaneNum:%d] address:0x%8x, stride:%d\n",
			 i,
			 dmar_get_status(dmar,
					 (DMAR_CTRL3 +
					 (i * DMAR_PLANE_CONFIG_SIZE)),
					 0xFFFF, 0),
			 dmar_get_status(dmar,
					 (DMAR_CTRL11 +
					 (i * DMAR_PLANE_CONFIG_SIZE)),
					 0xFFFF, 0));
		dev_info(pthis->dev,
			 "Block Read Byte:%d, Bit:%d, Count:%d\n",
			 dmar_get_status(dmar,
					 (DMAR_CTRL19 +
					  (i * DMAR_PLANE_CONFIG_SIZE)),
					 0xFFFF, 0),
			 dmar_get_status(dmar,
					 (DMAR_CTRL27 +
					 (i * DMAR_PLANE_CONFIG_SIZE)),
					 DMAR_BLOCK_READBIT_MASK,
					 DMAR_BLOCK_READBIT_SHIFT),
			 dmar_get_status(dmar,
					 (DMAR_CTRL27 +
					 (i * DMAR_PLANE_CONFIG_SIZE)),
					 DMAR_BLOCK_COUNT_MASK,
					 DMAR_BLOCK_COUNT_SHIFT));
	}

	dev_info(pthis->dev, "Interrupt Status:0x%x\n",
		 dmar_get_status(dmar, DMAR_INT, 0xFFFF, 0));

	dev_info(pthis->dev, "============================================\n");
}

static void dmar_set_interrupt_enable(const struct nxs_dev *pthis,
				      enum nxs_event_type type,
				      bool enable)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);
	u32 mask_v = 0;

	dev_info(pthis->dev, "[%s] type  %d, enable - %d\n",
		 __func__, type, enable);
	if (enable) {
		if (type & NXS_EVENT_IDLE)
			mask_v |= DMAR_IDLE_PRESP_INTENB_MASK;
		if (type & NXS_EVENT_DONE)
			mask_v |= DMAR_UPDATE_PRESP_INTENB_MASK;
		if (type & NXS_EVENT_ERR)
			mask_v |= DMAR_ERR_PRESP_INTENB_MASK;
	} else {
		if (type & NXS_EVENT_IDLE)
			mask_v |= DMAR_IDLE_PRESP_INTDISABLE_MASK;
		if (type & NXS_EVENT_DONE)
			mask_v |= DMAR_UPDATE_PRESP_INTDISABLE_MASK;
		if (type & NXS_EVENT_ERR)
			mask_v = DMAR_ERR_PRESP_INTDISABLE_MASK;
	}
	regmap_update_bits(dmar->reg, dmar->offset + DMAR_INT,
			   mask_v, mask_v);
}

static u32 dmar_get_interrupt_enable(const struct nxs_dev *pthis,
				     enum nxs_event_type type)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);
	u32 status = 0, mask_v = 0;

	dev_info(pthis->dev, "[%s] type  %d\n", __func__, type);
	if (type & NXS_EVENT_IDLE)
		mask_v |= DMAR_IDLE_PRESP_INTENB_MASK;
	if (type & NXS_EVENT_DONE)
		mask_v |= DMAR_UPDATE_PRESP_INTENB_MASK;
	if (type & NXS_EVENT_ERR)
		mask_v |= DMAR_ERR_PRESP_INTENB_MASK;
	regmap_read(dmar->reg, dmar->offset + DMAR_INT, &status);
	status &= mask_v;
	return status;
}

static u32 dmar_get_interrupt_pending(const struct nxs_dev *pthis,
				      enum nxs_event_type type)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);
	u32 status = 0, mask_v = 0;

	dev_info(pthis->dev, "[%s] type  %d\n", __func__, type);
	if (type & NXS_EVENT_IDLE)
		mask_v |= DMAR_IDLE_PRESP_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_DONE)
		mask_v |= DMAR_UPDATE_PRESP_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_ERR)
		mask_v |= DMAR_ERR_PRESP_INTPEND_CLR_MASK;
	regmap_read(dmar->reg, dmar->offset + DMAR_INT, &status);
	status &= mask_v;

	return status;
}

static enum nxs_event_type
dmar_get_interrupt_pending_number(struct nxs_dmar *dmar)
{
	u32 status = 0, ret = NXS_EVENT_NONE;

	dev_info(dmar->nxs_dev.dev, "[%s]\n", __func__);
	regmap_read(dmar->reg, dmar->offset + DMAR_INT, &status);
	if (status & DMAR_IDLE_PRESP_INTPEND_CLR_MASK)
		ret |= NXS_EVENT_IDLE;
	if (status &  DMAR_UPDATE_PRESP_INTPEND_CLR_MASK)
		ret |= NXS_EVENT_DONE;
	if (status &  DMAR_ERR_PRESP_INTPEND_CLR_MASK)
		ret |= NXS_EVENT_ERR;
	return ret;
}

static void dmar_clear_interrupt_pending(const struct nxs_dev *pthis,
					 enum nxs_event_type type)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);
	u32 mask_v = 0;

	dev_info(pthis->dev, "[%s] type  %d\n", __func__, type);
	if (type & NXS_EVENT_IDLE)
		mask_v |= DMAR_IDLE_PRESP_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_DONE)
		mask_v |= DMAR_UPDATE_PRESP_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_ERR)
		mask_v |= DMAR_ERR_PRESP_INTPEND_CLR_MASK;

	regmap_update_bits(dmar->reg, dmar->offset + DMAR_INT,
			   mask_v, mask_v);
}

static int dmar_open(const struct nxs_dev *pthis)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	if (nxs_dev_get_open_count(&dmar->nxs_dev) == 0) {
		int ret;
		struct clk *clk;
		char dev_name[10] = {0, };

		sprintf(dev_name, "dmar%d", pthis->dev_inst_index);
		clk = clk_get(pthis->dev, dev_name);
		if (IS_ERR(clk)) {
			dev_err(pthis->dev, "controller clock not found\n");
			return -ENODEV;
		}
		ret = clk_prepare_enable(clk);
		if (ret) {
			dev_err(pthis->dev, "clock failed to prepare enable: %d\n",
				ret);
			return ret;
		}
		/* reset */
#ifndef SIMULATE_INTERRUPT
		ret = request_irq(dmar->irq, dmar_irq_handler,
				  IRQF_TRIGGER_NONE, "nxs-dmar", dmar);
		if (ret < 0) {
			dev_err(pthis->dev, "failed to request irq :%d\n", ret);
			return ret;
		}
#endif
	}
#ifdef SIMULATE_INTERRUPT
	setup_timer(&dmar->timer, int_timer_func, (long)dmar);
#endif
	nxs_dev_inc_open_count(&dmar->nxs_dev);
	return 0;
}

static int dmar_close(const struct nxs_dev *pthis)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);

#ifdef SIMULATE_INTERRUPT
	del_timer(&dmar->timer);
#endif
	if (dmar_check_busy(dmar)) {
		dev_err(pthis->dev, "dmar status is busy\n");
		return -EBUSY;
	}

	nxs_dev_dec_open_count(&dmar->nxs_dev);
	if (nxs_dev_get_open_count(&dmar->nxs_dev) == 0) {
		struct clk *clk;
		char dev_name[10] = {0, };

		dmar_reset_register(dmar);
#ifndef SIMULATE_INTERRUPT
		free_irq(dmar->irq, dmar);
#endif
		sprintf(dev_name, "dmar%d", pthis->dev_inst_index);
		clk = clk_get(pthis->dev, dev_name);
		if (IS_ERR(clk)) {
			dev_err(pthis->dev, "controller clock not found\n");
			return -ENODEV;
		}
		clk_disable_unprepare(clk);
	}
	return 0;
}

static int dmar_start(const struct nxs_dev *pthis)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	/*
	 * m2m path - dpc disable
	 * real stream path - dpc enable
	 * how to check whether this function list is m2m or real
	 * in driver side
	 */
	/* just for m2m test */
	dmar_set_dpc_enable(dmar, ~(dmar->is_m2m));
	dmar_set_cpu_start(dmar, true);
#ifdef SIMULATE_INTERRUPT
	mod_timer(&dmar->timer, jiffies +
		  msecs_to_jiffies(INT_TIMEOUT_MS));
#endif
	return 0;
}

static int dmar_stop(const struct nxs_dev *pthis)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
#ifdef SIMULATE_INTERRUPT
	del_timer(&dmar->timer);
#endif
	dmar_set_cpu_start(dmar, false);
	dmar_set_dpc_enable(dmar, false);
	return 0;
}

static int dmar_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);
	u32 dirty_val;

	dev_info(pthis->dev, "[%s] type:%d\n", __func__, type);
	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;
	switch (pthis->dev_inst_index) {
	case 0:
		dirty_val = DMAR0_DIRTY;
		break;
	case 1:
		dirty_val = DMAR1_DIRTY;
		break;
	case 2:
		dirty_val = DMAR2_DIRTY;
		break;
	case 3:
		dirty_val = DMAR3_DIRTY;
		break;
	case 4:
		dirty_val = DMAR4_DIRTY;
		break;
	case 5:
		dirty_val = DMAR5_DIRTY;
		break;
	case 6:
		dirty_val = DMAR6_DIRTY;
		break;
	case 7:
		dirty_val = DMAR7_DIRTY;
		break;
	case 8:
		dirty_val = DMAR8_DIRTY;
		break;
	case 9:
		dirty_val = DMAR9_DIRTY;
		break;
	default:
		dev_err(pthis->dev, "invalid inst %d\n", pthis->dev_inst_index);
		return -ENODEV;
	}

	dev_info(pthis->dev, "[%s] dmar index:%d, dirty_val:0x%x\n",
		__func__, pthis->dev_inst_index, dirty_val);

	return regmap_write(dmar->reg, DMAR_DIRTYSET_OFFSET, dirty_val);
}

static int dmar_set_tid(const struct nxs_dev *pthis, u32 tid1, u32 tid2)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	return regmap_update_bits(dmar->reg, dmar->offset + DMAR_CTRL1,
				  DMAR_TID_MASK, tid1 << DMAR_TID_SHIFT);
}

static void dmar_set_plane_format_config(struct nxs_dmar *dmar,
					 struct nxs_plane_format *config)
{
	u32 value = 0, i;

	nxs_print_plane_format(&dmar->nxs_dev, config);

	/* set color expand and color dither */
	if (config->img_bit == NXS_IMG_BIT_SM_10)
		value |= ((1 << DMAR_COLOR_EXPAND_SHIFT) |
			DMAR_COLOR_EXPAND_MASK);
	else if (config->img_bit == NXS_IMG_BIT_BIG_10)
		value |= ((1 << DMAR_COLOR_DITHER_SHIFT) |
			DMAR_COLOR_DITHER_MASK);
	regmap_update_bits(dmar->reg, dmar->offset + DMAR_DIT,
			   DMAR_COLOR_EXPAND_MASK | DMAR_COLOR_DITHER_MASK,
			   value);

	/* dmar total bitwidth and half height configuration */
	value = ((config->total_bitwidth_1st_pl <<
		DMAR_TOTAL_BITWIDTH_1ST_PL_SHIFT) |
		 DMAR_TOTAL_BITWIDTH_1ST_PL_MASK);
	value |= ((config->total_bitwidth_2nd_pl <<
		DMAR_TOTAL_BITWIDTH_2ND_PL_SHIFT) |
		 DMAR_TOTAL_BITWIDTH_2ND_PL_MASK);
	/* setting put dummy type */
	if (config->put_dummy_type)
		value |= ((config->put_dummy_type << DMAR_PUT_DUMMY_TYPE_SHIFT)
			  | DMAR_PUT_DUMMY_TYPE_MASK);
	regmap_update_bits(dmar->reg, dmar->offset + DMAR_FORMAT0,
			   DMAR_TOTAL_BITWIDTH_1ST_PL_MASK |
			   DMAR_TOTAL_BITWIDTH_2ND_PL_MASK,
			   value);
	/*
	 * Todo
	 * currently support only for progressive mode
	 * so it needs a setting about half height for odd/even/r_odd/r_even as
	 * each mode
	 */
	value = ((config->half_height_1st_pl <<
		DMAR_1P_HALF_HEIGHT_SHIFT) |
		 DMAR_1P_HALF_HEIGHT_MASK);
	value |= ((config->half_height_2nd_pl <<
		DMAR_2P_HALF_HEIGHT_SHIFT) |
		 DMAR_2P_HALF_HEIGHT_MASK);
	regmap_update_bits(dmar->reg, dmar->offset + DMAR_CTRL59,
			   DMAR_1P_HALF_HEIGHT_MASK |
			   DMAR_2P_HALF_HEIGHT_MASK,
			   value);

	/* 1st plane - p composition */
	for (i = 0; i < DMAR_P_COMP_COUNT; i++) {
		value = ((config->p0_comp[i].startbit <<
			  DMAR_P_COMP_STARTBIT_SHIFT)
			 | DMAR_P_COMP_STARTBIT_MASK);
		value |= ((config->p0_comp[i].bitwidth <<
			   DMAR_P_COMP_BITWIDTH_SHIFT)
			  | DMAR_P_COMP_BITWIDTH_MASK);
		value |= ((config->p0_comp[i].is_2nd_pl <<
			   DMAR_P_COMP_IS_2ND_PL_SHIFT) |
			  DMAR_P_COMP_IS_2ND_PL_MASK);
		value |= ((config->p0_comp[i].use_userdef <<
			   DMAR_P_COMP_USE_USERDEF_SHIFT) |
			  DMAR_P_COMP_USE_USERDEF_MASK);
		value |= ((config->p0_comp[i].userdef <<
			   DMAR_P_COMP_USERDEF_SHIFT) |
			  DMAR_P_COMP_USERDEF_MASK);
		regmap_write(dmar->reg, dmar->offset + DMAR_P0_COMP0 +
			     (i * DMAR_P_COMP_SIZE),
			     value);
	}

	/* 2nd plane - p composition */
	for (i = 0; i < DMAR_P_COMP_COUNT; i++) {
		value = ((config->p1_comp[i].startbit <<
			  DMAR_P_COMP_STARTBIT_SHIFT)
			 | DMAR_P_COMP_STARTBIT_MASK);
		value |= ((config->p1_comp[i].bitwidth <<
			   DMAR_P_COMP_BITWIDTH_SHIFT)
			  | DMAR_P_COMP_BITWIDTH_MASK);
		value |= ((config->p1_comp[i].is_2nd_pl <<
			   DMAR_P_COMP_IS_2ND_PL_SHIFT) |
			  DMAR_P_COMP_IS_2ND_PL_MASK);
		value |= ((config->p1_comp[i].use_userdef <<
			   DMAR_P_COMP_USE_USERDEF_SHIFT) |
			  DMAR_P_COMP_USE_USERDEF_MASK);
		value |= ((config->p1_comp[i].userdef <<
			   DMAR_P_COMP_USERDEF_SHIFT) |
			  DMAR_P_COMP_USERDEF_MASK);
		regmap_write(dmar->reg, dmar->offset + DMAR_P1_COMP0 +
			     (i * DMAR_P_COMP_SIZE),
			     value);
	}
}
/*
 * Nexell Stream Format
 * RGBA / VYUY
 */
static int dmar_set_format(const struct nxs_dev *pthis,
			   const struct nxs_control *pparam)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);
	u32 format = pparam->u.format.pixelformat;
	u32 width, height;
	struct nxs_plane_format config;

	dev_info(pthis->dev, "[%s] widht = %d, height = %d, format = 0x%x\n",
		 __func__, pparam->u.format.width, pparam->u.format.height,
		 pparam->u.format.pixelformat);
	dev_info(pthis->dev, "[%s]\n", __func__);
	width = pparam->u.format.width;
	height = pparam->u.format.height;

	memset(&config, 0x0, sizeof(struct nxs_plane_format));

	nxs_get_plane_format(&dmar->nxs_dev,
			     format, &config);
	if (config.img_type == NXS_IMG_MAX) {
		dev_err(pthis->dev, " failed to get disp img info\n");
		return -EINVAL;
	}
	dev_info(pthis->dev,
		 "format is %s, width:%d, height:%d, multi planes:%s\n",
		 (config.img_type)?"RGB" : "YUV", width, height,
		 (config.total_bitwidth_2nd_pl) ? "true" : "false");
	dmar_set_img_type(dmar, config.img_type);
	dmar_set_size(dmar, width, height);
	dmar_set_plane_format_config(dmar, &config);

	return 0;
}

static int dmar_get_format(const struct nxs_dev *pthis,
			   struct nxs_control *pparam)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);
	u32 img_type;

	dev_info(pthis->dev, "[%s]\n", __func__);
	img_type = dmar_get_img_type(dmar);
	dev_info(pthis->dev, "image type = %s\n",
		 (img_type)?"RGB":"YUV");
	/* pparam->u.format.pixelformat; */
	return 0;
}

static int dmar_set_buffer(const struct nxs_dev *pthis,
			   const struct nxs_control *pparam)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);
	u32 i, plane_num, mode;
	u32 addr_offset, stride_offset;
	u32 blockbyte_offset, blockbit_offset;

	dev_info(pthis->dev, "[%s]\n", __func__);
	plane_num = pparam->u.buffer.num_planes;
	if (plane_num > NXS_DEV_MAX_PLANES)
		return -EINVAL;

	mode = dmar_get_field(dmar);
	for (i = 0; i < plane_num; i++) {
		switch (mode) {
		case NXS_FIELD_PROGRESSIVE:
			addr_offset = DMAR_CTRL3;
			stride_offset = DMAR_CTRL11;
			blockbyte_offset = DMAR_CTRL19;
			blockbit_offset = DMAR_CTRL27;
			break;
		/*
		 * Todo
		 * support interlaced and hdmi 3d filed mode
		 */
		case NXS_FIELD_INTERLACED:
			/*
			 * even
			 * addr_offset = DMAR_CTRL5
			 * stride_offset = DMAR_CTRL13;
			 * blockbyte_offset = DMAR_CTRL21;
			 * blockbit_offset = DMAR_CTRL29;
			 */
			break;
		case NXS_FIELD_3D:
			/*
			 * R_odd
			 * addr_offset = DMAR_CTRL7
			 * stride_offset = DMAR_CTRL15;
			 * blockbyte_offset = DMAR_CTRL23;
			 * blockbit_offset = DMAR_CTRL31;
			 * R_even
			 * addr_offset = DMAR_CTRL9
			 * stride_offset = DMAR_CTRL17;
			 * blockbyte_offset = DMAR_CTRL25;
			 * blockbit_offset = DMAR_CTRL33;
			 */
			break;
		default:
			dev_err(pthis->dev, "invalid field mode\n");
			break;
		}
		regmap_write(dmar->reg,
			     dmar->offset + addr_offset +
			     (i * DMAR_PLANE_CONFIG_SIZE),
			     pparam->u.buffer.address[i]);
		regmap_write(dmar->reg,
			     dmar->offset + stride_offset +
			     (i * DMAR_PLANE_CONFIG_SIZE),
			     pparam->u.buffer.strides[i]);
		dmar_set_block_config(dmar, plane_num, i,
				      pparam->u.buffer.strides[i],
				      blockbyte_offset, blockbit_offset);
	}

	return 0;
}

static int dmar_get_buffer(const struct nxs_dev *pthis,
			   struct nxs_control *pparam)
{
	/*struct nxs_dmar *dmar = nxs_to_dmar(pthis);*/
	return 0;
}

static int dmar_set_video(const struct nxs_dev *pthis,
			  const struct nxs_control *pparam)
{
	struct nxs_dmar *dmar = nxs_to_dmar(pthis);
	u32 field, type = pparam->u.video.type;

	dev_info(pthis->dev, "[%s] type:%d, field:%d\n",
		 __func__, type, pparam->u.video.field);
	if (type == NXS_VIDEO_TYPE_M2M)
		dmar->is_m2m = true;
	else
		dmar->is_m2m = false;

	field = pparam->u.video.field;
	if (field > NXS_FIELD_3D) {
		dev_err(dmar->nxs_dev.dev, "Invalid field mode\n");
		return -EINVAL;
	}
	dmar_set_field(dmar, field);
	return 0;
}

static int dmar_get_video(const struct nxs_dev *pthis,
			  const struct nxs_control *pparam)
{
	return 0;
}

static int nxs_dmar_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_dmar *dmar;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	dmar = devm_kzalloc(&pdev->dev, sizeof(*dmar), GFP_KERNEL);
	if (!dmar)
		return -ENOMEM;

	nxs_dev = &dmar->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	dmar->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						    "syscon");
	if (IS_ERR(dmar->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(dmar->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	dmar->offset = res->start;
	dmar->irq = platform_get_irq(pdev, 0);
	dmar->is_m2m = false;

	nxs_dev->set_interrupt_enable = dmar_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = dmar_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = dmar_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = dmar_clear_interrupt_pending;
	nxs_dev->open = dmar_open;
	nxs_dev->close = dmar_close;
	nxs_dev->start = dmar_start;
	nxs_dev->stop = dmar_stop;
	nxs_dev->set_dirty = dmar_set_dirty;
	nxs_dev->set_tid = dmar_set_tid;
	nxs_dev->dump_register = dmar_dump_register;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_FORMAT;
	nxs_dev->dev_services[0].set_control = dmar_set_format;
	nxs_dev->dev_services[0].get_control = dmar_get_format;
	nxs_dev->dev_services[1].type = NXS_CONTROL_BUFFER;
	nxs_dev->dev_services[1].set_control = dmar_set_buffer;
	nxs_dev->dev_services[1].get_control = dmar_get_buffer;
	nxs_dev->dev_services[2].type = NXS_CONTROL_VIDEO;
	nxs_dev->dev_services[2].set_control = dmar_set_video;
	nxs_dev->dev_services[2].get_control = dmar_get_video;

	nxs_dev->dev = &pdev->dev;
	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	dev_info(nxs_dev->dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, dmar);

	return 0;
}

static int nxs_dmar_remove(struct platform_device *pdev)
{
	struct nxs_dmar *dmar = platform_get_drvdata(pdev);

	if (dmar)
		unregister_nxs_dev(&dmar->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_dmar_match[] = {
	{ .compatible = "nexell,dmar-nxs-1.0", },
	{},
};

static struct platform_driver nxs_dmar_driver = {
	.probe	= nxs_dmar_probe,
	.remove	= nxs_dmar_remove,
	.driver	= {
		.name = "nxs-dmar",
		.of_match_table = of_match_ptr(nxs_dmar_match),
	},
};

static int __init nxs_dmar_init(void)
{
	return platform_driver_register(&nxs_dmar_driver);
}
fs_initcall(nxs_dmar_init);

static void __exit nxs_dmar_exit(void)
{
	platform_driver_unregister(&nxs_dmar_driver);
}
module_exit(nxs_dmar_exit);

MODULE_DESCRIPTION("Nexell Stream DMAR driver");
MODULE_AUTHOR("Hyejung Kwon, <cjscld15@nexell.co.kr>");
MODULE_LICENSE("GPL");
