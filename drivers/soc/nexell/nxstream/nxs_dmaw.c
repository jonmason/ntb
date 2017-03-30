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
#include <linux/soc/nexell/nxs_plane_format.h>

#define DMAW_DIRTYSET_OFFSET	0x0008
#define DMAW_DIRTYCLR_OFFSET	0x0018
#define DMAW0_DIRTY		BIT(4)
#define DMAW1_DIRTY		BIT(5)
#define DMAW2_DIRTY		BIT(6)
#define DMAW3_DIRTY		BIT(7)
#define DMAW4_DIRTY		BIT(8)
#define DMAW5_DIRTY		BIT(9)
#define DMAW6_DIRTY		BIT(10)
#define DMAW7_DIRTY		BIT(11)
#define DMAW8_DIRTY		BIT(12)
#define DMAW9_DIRTY		BIT(13)
#define DMAW10_DIRTY		BIT(14)
#define DMAW11_DIRTY		BIT(15)


/* DMAW REGISTER OFFSET */
#define DMAW_CTRL0				0x00
#define DMAW_CTRL1				0x04
#define DMAW_CTRL2				0x08
#define DMAW_P0_COMP0				0x0C
#define DMAW_P0_COMP1				0x10
#define DMAW_P0_COMP2				0x14
#define DMAW_P0_COMP3				0x18
#define DMAW_P1_COMP0				0x1C
#define DMAW_P1_COMP1				0x20
#define DMAW_P1_COMP2				0x24
#define DMAW_P1_COMP3				0x28
#define DMAW_PL0_BASEADDR			0x34
#define DMAW_PL0_BASEADDR_H			0x38
#define DMAW_PL0_STRIDE				0x3C
#define DMAW_PL0_STRIDE_H			0x40
#define DMAW_PL0_CTRL				0x44
#define DMAW_PL1_BASEADDR			0x48
#define DMAW_PL1_BASEADDR_H			0x4C
#define DMAW_PL1_STRIDE				0x50
#define DMAW_PL1_STRIDE_H			0x54
#define DMAW_PL1_CTRL				0x58
#define DMAW_SRC_IMG_SIZE			0x5C
#define DMAW_INT				0x60

#define DMAW_PL_CONFIG_SIZE			0x14

/* DMAW CTRL0 - offset : 0x5000 */
#define DMAW_REG_CLEAR_SHIFT	3
#define DMAW_REG_CLEAR_MASK	BIT(3)
#define DMAW_TZPROT_SHIFT	2
#define DMAW_TZPROT_MASK	BIT(2)
#define DMAW_IDLE_SHIFT		1
#define DMAW_IDLE_MASK		BIT(1)
#define DMAW_EN_START_SHIFT	0
#define DMAW_EN_START_MASK	BIT(0)

/* DMAW CTRL1 - offset : 0x5004 */
#define DMAW_EN_ADDR_FITTING_SHIFT		24
#define DMAW_EN_ADDR_FITTING_MASK		BIT(24)
#define DMAW_PL1_AXI_CTRL_AWCACHE_SHIFT		20
#define DMAW_PL1_AXI_CTRL_AWCACHE_MASK		GENMASK(23, 20)
#define DMAW_PL1_AXI_CTRL_WID_SHIFT		16
#define DMAW_PL1_AXI_CTRL_WID_MASK		GENMASK(19, 16)
#define DMAW_PL0_AXI_CTRL_AWCACHE_SHIFT		12
#define DMAW_PL0_AXI_CTRL_AWCACHE_MASK		GENMASK(15, 12)
#define DMAW_PL0_AXI_CTRL_WID_SHIFT		8
#define DMAW_PL0_AXI_CTRL_WID_MASK		GENMASK(11, 8)
#define DMAW_INVERT_FIELD_SHIFT			3
#define DMAW_INVERT_FIELD_MASK			BIT(3)
#define DMAW_EN_INTERLACE_MODE_SHIFT		2
#define DMAW_EN_INTERLACE_MODE_MASK		BIT(2)
#define DMAW_EN_EXPANDER_SHIFT			1
#define DMAW_EN_EXPANDER_MASK			BIT(1)
#define DMAW_EN_DITHER_SHIFT			0
#define DMAW_EN_DITHER_MASK			BIT(0)

/* DMAW CTRL2 - offset : 0x5008 */
#define DMAW_HALF_HEIGHT_2ND_PL_SHIFT		20
#define DMAW_HALF_HEIGHT_2ND_PL_MASK		BIT(20)
#define DMAW_HALF_HEIGHT_1ST_PL_SHIFT		19
#define DMAW_HALF_HEIGHT_1ST_PL_MASK		BIT(19)
#define DMAW_USE_AVERAGE_VALUE_SHIFT		18
#define DMAW_USE_AVERAGE_VALUE_MASK		BIT(18)
#define DMAW_PUT_DUMMY_TYPE_SHIFT		16
#define DMAW_PUT_DUMMY_TYPE_MASK		GENMASK(17, 16)
#define DMAW_TOTAL_BITWIDTH_2ND_PL_SHIFT	8
#define DMAW_TOTAL_BITWIDTH_2ND_PL_MASK		GENMASK(15, 8)
#define DMAW_TOTAL_BITWIDTH_1ST_PL_SHIFT	0
#define DMAW_TOTAL_BITWIDTH_1ST_PL_MASK		GENMASK(8, 0)

/* DMAW P_COMP - offset : 0x500C */
/* P0_COMP0 - 0x500C, P0_COMP1 - 0x5010 */
/* P0_COMP2 - 0x5014, P0_COMP3 - 0x5018 */
/* P1_COMP0 - 0x501C, P1_COMP2 - 0x5020 */
/* P1_COMP2 - 0x5024, P1_COMP3 - 0x5028 */
#define DMAW_P_COMP_STARTBIT_SHIFT		17
#define DMAW_P_COMP_STARTBIT_MASK		GENMASK(23, 17)
#define DMAW_P_COMP_BITWIDTH_SHIFT		12
#define DMAW_P_COMP_BITWIDTH_MASK		GENMASK(16, 12)
#define DMAW_P_COMP_IS_2ND_PL_SHIFT		11
#define DMAW_P_COMP_IS_2ND_PL_MASK		BIT(11)
#define DMAW_P_COMP_USE_USERDEF_SHIFT		10
#define DMAW_P_COMP_USE_USERDEF_MASK		BIT(10)
#define DMAW_P_COMP_USERDEF_SHIFT		0
#define DMAW_P_COMP_USERDEF_MASK		GENMASK(10, 0)
#define DMAW_P_COMP_SIZE			0x10
#define DMAW_P_COMP_COUNT			4

/* DMAW - PL0 CTRL - offset : 0x5044 */
/* DMAW - PL1 CTRL - offset : 0x5058 */
#define DMAW_PL_NUM_OF_128X16_TR_SHIFT		12
#define DMAW_PL_NUM_OF_128X16_TR_MASK		GENMASK(22, 12)
#define DMAW_PL_LAST_FLUSH_128X16_TR_SHIFT	8
#define DMAW_PL_LAST_FLUSH_128X16_TR_MASK	GENMASK(11, 8)
#define DMAW_PL_BIT_IN_LAST_128_TR_SHIFT	0
#define DMAW_PL_BIT_IN_LAST_128_TR_MASK		GENMASK(7, 0)
#define DMAW_PL_CTRL_OFFSET			0x14
#define DMAW_PL_CTRL_MASK			GENMASK(22, 0)

/* DMAW SRC IMG SIZE - offset : 0x505C */
#define DMAW_SRC_IMG_HEIGHT_SHIFT		16
#define DMAW_SRC_IMG_HEIGHT_MASK		GENMASK(31, 16)
#define DMAW_SRC_IMG_WIDTH_SHIFT		0
#define DMAW_SRC_IMG_WIDTH_MASK			GENMASK(15, 0)

/* DMAW INTERRUPT - offset : 0x5060 */
#define DMAW_IDLE_INTDISABLE_SHIFT		30
#define DMAW_IDLE_INTDISABLE_MASK		BIT(30)
#define DMAW_IDLE_INTENB_SHIFT			29
#define DMAW_IDLE_INTENB_MASK			BIT(29)
#define DMAW_IDLE_INTPEND_CLR_SHIFT		28
#define DMAW_IDLE_INTPEND_CLR_MASK		BIT(28)
#define DMAW_RESOL_ERR_INTDISABLE_SHIFT		26
#define DMAW_RESOL_ERR_INTDISABLE_MASK		BIT(26)
#define DMAW_RESOL_ERR_INTENB_SHIFT		25
#define DMAW_RESOL_ERR_INTENB_MASK		BIT(25)
#define DMAW_RESOL_ERR_INTPEND_CLR_SHIFT	24
#define DMAW_RESOL_ERR_INTPEND_CLR_MASK		BIT(24)
#define DMAW_DEC_ERR_INTDISABLE_SHIFT		22
#define DMAW_DEC_ERR_INTDISABLE_MASK		BIT(22)
#define DMAW_DEC_ERR_INTENB_SHIFT		21
#define DMAW_DEC_ERR_INTENB_MASK		BIT(21)
#define DMAW_DEC_ERR_INTPEND_CLR_SHIFT		20
#define DMAW_DEC_ERR_INTPEND_CLR_MASK		BIT(20)
#define DMAW_AXI1_ERR_INTDISABLE_SHIFT		18
#define DMAW_AXI1_ERR_INTDISABLE_MASK		BIT(18)
#define DMAW_AXI1_ERR_INTENB_SHIFT		17
#define DMAW_AXI1_ERR_INTENB_MASK		BIT(17)
#define DMAW_AXI1_ERR_INTPEND_CLR_SHIFT		16
#define DMAW_AXI1_ERR_INTPEND_CLR_MASK		BIT(16)
#define DMAW_AXI0_ERR_INTDISABLE_SHIFT		14
#define DMAW_AXI0_ERR_INTDISABLE_MASK		BIT(14)
#define DMAW_AXI0_ERR_INTENB_SHIFT		13
#define DMAW_AXI0_ERR_INTENB_MASK		BIT(13)
#define DMAW_AXI0_ERR_INTPEND_CLR_SHIFT		12
#define DMAW_AXI0_ERR_INTPEND_CLR_MASK		BIT(12)
#define DMAW_SEC_ERR_INTDISABLE_SHIFT		10
#define DMAW_SEC_ERR_INTDISABLE_MASK		BIT(10)
#define DMAW_SEC_ERR_INTENB_SHIFT		9
#define DMAW_SEC_ERR_INTENB_MASK		BIT(9)
#define DMAW_SEC_ERR_INTPEND_CLR_SHIFT		8
#define DMAW_SEC_ERR_INTPEND_CLR_MASK		BIT(8)
#define DMAW_DONE_INTDISABLE_SHIFT		6
#define DMAW_DONE_INTDISABLE_MASK		BIT(6)
#define DMAW_DONE_INTENB_SHIFT			5
#define DMAW_DONE_INTENB_MASK			BIT(5)
#define DMAW_DONE_INTPEND_CLR_SHIFT		4
#define DMAW_DONE_INTPEND_CLR_MASK		BIT(4)
#define DMAW_UPDATE_INTDISABLE_SHIFT		2
#define DMAW_UPDATE_INTDISABLE_MASK		BIT(2)
#define DMAW_UPDATE_INTENB_SHIFT		1
#define DMAW_UPDATE_INTENB_MASK			BIT(1)
#define DMAW_UPDATE_INTPEND_CLR_SHIFT		0
#define DMAW_UPDATE_INTPEND_CLR_MASK		BIT(0)

/* AXIM_DMARW */
#define AXIM_DMARW_OFFSET			0x100000
#define AXIM_DMARW_SIZE				0x4000

#define AXIM_DMARW_AWQOS_OFFSET			0x00
#define AXIM_DMARW_AWQOS_SHIFT			4
#define AXIM_DMARW_AWQOS_MASK			GENMASK(7, 4)
#define AXIM_DMARW_AXQOS_USE_SHIFT		8
#define AXIM_DMARW_AXQOS_USE_MASK		BIT(8)
#define AXIM_DMARW_AWADDRH_OFFSET		0x24
#define AXIM_DMARW_AWADDRH_EN_OFFSET		0x2c

/* DMAW TRANSACTION CONTROL */
#define DMAW_TR_WIDTH				128
#define DMAW_TR_HEIGHT				16
#define DMAW_TR_SIZE				(DMAW_TR_WIDTH*DMAW_TR_HEIGHT)

/* #define SIMULATE_INTERRUPT */

struct nxs_dmaw {
#ifdef SIMULATE_INTERRUPT
	struct timer_list timer;
#endif
	struct nxs_dev nxs_dev;
	struct regmap *reg;
	u32 offset;
	u32 irq;
	bool is_m2m;
};

#define nxs_to_dmaw(dev)	container_of(dev, struct nxs_dmaw, nxs_dev)

static enum nxs_event_type
dmaw_get_interrupt_pending_number(struct nxs_dmaw *dmaw);
static u32 dmaw_get_interrupt_pending(const struct nxs_dev *pthis,
				      enum nxs_event_type);
static void dmaw_clear_interrupt_pending(const struct nxs_dev *pthis,
					 enum nxs_event_type type);

#ifdef SIMULATE_INTERRUPT
#include <linux/timer.h>
#define INT_TIMEOUT_MS		30

static void int_timer_func(void *priv)
{
	struct nxs_dmaw *dmaw = (struct nxs_dmaw *)priv;
	struct nxs_dev *nxs_dev = &dmaw->nxs_dev;
	struct nxs_irq_callback *callback;
	unsigned long flags;

	spin_lock_irqsave(&nxs_dev->irq_lock, flags);
	list_for_each_entry(callback, &nxs_dev->irq_callback, list) {
		if (callback)
			callback->handler(nxs_dev, callback->data);
	}
	spin_unlock_irqrestore(&nxs_dev->irq_lock, flags);

	mod_timer(&dmaw->timer, jiffies + msecs_to_jiffies(INT_TIMEOUT_MS));
}
#else
static irqreturn_t dmaw_irq_handler(void *priv)
{
	struct nxs_dmaw *dmaw = (struct nxs_dmaw *)priv;
	struct nxs_dev *nxs_dev = &dmaw->nxs_dev;
	struct nxs_irq_callback *callback;
	unsigned long flags;

	dev_info(nxs_dev->dev, "[%s] pending status = 0x%x\n", __func__,
		 dmaw_get_interrupt_pending(nxs_dev, NXS_EVENT_ALL));
	dmaw_clear_interrupt_pending(nxs_dev,
				     dmaw_get_interrupt_pending_number(dmaw));

	spin_lock_irqsave(&nxs_dev->irq_lock, flags);
	list_for_each_entry(callback, &nxs_dev->irq_callback, list) {
		if (callback)
			callback->handler(nxs_dev, callback->data);
	}
	spin_unlock_irqrestore(&nxs_dev->irq_lock, flags);
	return IRQ_HANDLED;
}
#endif

static void dmaw_reset_register(struct nxs_dmaw *dmaw)
{
	regmap_update_bits(dmaw->reg, dmaw->offset + DMAW_CTRL0,
			   DMAW_REG_CLEAR_MASK, 1 << DMAW_REG_CLEAR_SHIFT);
}

static bool dmaw_check_busy(struct nxs_dmaw *dmaw)
{
	u32 busy;

	regmap_read(dmaw->reg, dmaw->offset + DMAW_CTRL0, &busy);
	return ~(busy && DMAW_IDLE_MASK);
}

static void dmaw_set_dpc_mode(struct nxs_dmaw *dmaw, bool enable)
{
	u32 val, awqos;

	dev_info(dmaw->nxs_dev.dev, "[%s] %s\n",
		 __func__, (enable)?"enable":"disable");
	/*
	 * dpc mode - set AWADDR_H and enable AWQOS
	 * m2m mode - set AWADDR_H and disable AWQOS
	 */
	regmap_write(dmaw->reg, AXIM_DMARW_OFFSET +
		     ((dmaw->nxs_dev.dev_inst_index * AXIM_DMARW_SIZE) +
		      AXIM_DMARW_AWADDRH_OFFSET),
		     enable);
	val = 0xFFFFFFFF;
	regmap_write(dmaw->reg, AXIM_DMARW_OFFSET +
		     ((dmaw->nxs_dev.dev_inst_index * AXIM_DMARW_SIZE) +
		      AXIM_DMARW_AWADDRH_EN_OFFSET),
		     val);
	if (enable)
		awqos = 14;
	else
		awqos = 0;

	regmap_update_bits(dmaw->reg, AXIM_DMARW_OFFSET +
			   ((dmaw->nxs_dev.dev_inst_index * AXIM_DMARW_SIZE) +
			    AXIM_DMARW_AWQOS_OFFSET),
			   AXIM_DMARW_AXQOS_USE_MASK,
			   1 << AXIM_DMARW_AXQOS_USE_SHIFT);
	regmap_update_bits(dmaw->reg, AXIM_DMARW_OFFSET +
			   ((dmaw->nxs_dev.dev_inst_index * AXIM_DMARW_SIZE) +
			    AXIM_DMARW_AWQOS_OFFSET),
			   AXIM_DMARW_AWQOS_MASK,
			   awqos << AXIM_DMARW_AWQOS_SHIFT);

}

static void dmaw_set_interlaced_mode(struct nxs_dmaw *dmaw, bool enable)
{
	dev_info(dmaw->nxs_dev.dev, "[%s] %s\n", __func__,
		 (enable) ? "enable" : "disable");
	regmap_update_bits(dmaw->reg, dmaw->offset + DMAW_CTRL1,
			   DMAW_EN_INTERLACE_MODE_MASK,
			   enable << DMAW_EN_INTERLACE_MODE_SHIFT);
}

static void dmaw_set_start(struct nxs_dmaw *dmaw, bool enable)
{
	dev_info(dmaw->nxs_dev.dev, "[%s] %s\n", __func__,
		 (enable) ? "enable" : "disable");
	regmap_update_bits(dmaw->reg, dmaw->offset + DMAW_CTRL0,
			   DMAW_EN_START_MASK, enable << DMAW_EN_START_SHIFT);

}

static void dmaw_set_src_imgsize(struct nxs_dmaw *dmaw, u32 width, u32 height)
{
	dev_info(dmaw->nxs_dev.dev, "[%s] width:%d, height:%d\n",
		 __func__, width, height);
	regmap_write(dmaw->reg,
		     dmaw->offset + DMAW_SRC_IMG_SIZE,
		     ((width << DMAW_SRC_IMG_WIDTH_SHIFT) || height));
}

static u32 dmaw_get_bpp(struct nxs_dmaw *dmaw, u32 plane_num)
{
	u32 value = 0, bitwidth;

	regmap_read(dmaw->reg, dmaw->offset + DMAW_CTRL2, &value);
	if (plane_num)
		bitwidth = (value & DMAW_TOTAL_BITWIDTH_2ND_PL_MASK) >>
			DMAW_TOTAL_BITWIDTH_2ND_PL_SHIFT;
	else
		bitwidth = (value & DMAW_TOTAL_BITWIDTH_1ST_PL_MASK) >>
			DMAW_TOTAL_BITWIDTH_1ST_PL_SHIFT;

	return (bitwidth / 2);
}

static void dmaw_set_axi_control(struct nxs_dmaw *dmaw)
{
	u32 wid = 0, awcache = 3, value;

	value = ((wid << DMAW_PL0_AXI_CTRL_WID_SHIFT) |
		(wid << DMAW_PL1_AXI_CTRL_WID_SHIFT) |
		(awcache << DMAW_PL0_AXI_CTRL_AWCACHE_SHIFT) |
		(awcache << DMAW_PL1_AXI_CTRL_AWCACHE_SHIFT));
	regmap_update_bits(dmaw->reg, dmaw->offset + DMAW_CTRL1,
			   DMAW_PL0_AXI_CTRL_WID_MASK |
			   DMAW_PL1_AXI_CTRL_WID_MASK |
			   DMAW_PL0_AXI_CTRL_AWCACHE_MASK |
			   DMAW_PL1_AXI_CTRL_AWCACHE_MASK,
			   value);
}

/*
 * DMAW transfer a data by 128*16 size
 * Todo
 * currently only support for 8bit and yuv/rgb format
 * It's different to calculate each value for 10bit and bayer format
 */
static void dmaw_set_transaction_control(struct nxs_dmaw *dmaw, u32 width)
{
	u32 i, total_bit, div, mod, extra, value, bpp;
	u32 num_of_tr, last_flush_tr, bit_in_last_tr;

	for (i = 0; i < NXS_DEV_MAX_PLANES; i++) {
		/* get num of 128*16 tr and last flush 128*16 tr for plane i*/
		bpp = dmaw_get_bpp(dmaw, i);
		if (bpp % 10) {
			total_bit = width * bpp;
			div = total_bit / DMAW_TR_SIZE;
			mod = total_bit % DMAW_TR_SIZE;
			extra = (!mod) ? 0 : 1;
			num_of_tr = (bpp) ? (div + (extra-1)) : 0;
			last_flush_tr = mod;
			div = last_flush_tr / DMAW_TR_WIDTH;
			mod = last_flush_tr % DMAW_TR_WIDTH;
			extra = (!mod) ? 0 : 1;
			last_flush_tr = (div + extra) ? ((div + extra) - 1) :
				(DMAW_TR_HEIGHT - 1);
			bit_in_last_tr = mod;
		} else {
			total_bit = width;
			div = total_bit / 12;
			mod = total_bit % 12;
			extra = (!mod) ? 120 : (mod*10);
			bit_in_last_tr = extra;
			if (mod)
				num_of_tr = div + 1;
			else
				num_of_tr = div;
			div = num_of_tr / 16;
			mod = num_of_tr % 16;
			extra = (!mod) ? 0 : 1;
			num_of_tr = div + (extra - 1);
			last_flush_tr = (mod == 0) ? (16 - 1) : (mod - 1);
		}

		value = bit_in_last_tr;
		value |= ((last_flush_tr << DMAW_PL_LAST_FLUSH_128X16_TR_SHIFT)
			  | DMAW_PL_LAST_FLUSH_128X16_TR_MASK);
		value |= ((num_of_tr << DMAW_PL_NUM_OF_128X16_TR_SHIFT)
			  | DMAW_PL_NUM_OF_128X16_TR_MASK);
		regmap_update_bits(dmaw->reg, dmaw->offset + DMAW_PL0_CTRL +
				   DMAW_PL_CTRL_OFFSET * i,
				   DMAW_PL_CTRL_MASK, value);
	}
}

static void dmaw_set_plane_format(struct nxs_dmaw *dmaw,
				  struct nxs_plane_format *config)
{
	u32 value = 0, i;

	nxs_print_plane_format(&dmaw->nxs_dev, config);

	/* set color expand and color dither */
	if (config->img_bit == NXS_IMG_BIT_BIG_10)
		value |= ((1 << DMAW_EN_EXPANDER_SHIFT) |
			  DMAW_EN_EXPANDER_MASK);
	else if (config->img_bit == NXS_IMG_BIT_SM_10)
		value |= ((1 << DMAW_EN_DITHER_SHIFT) |
			DMAW_EN_DITHER_MASK);
	/*
	 * enable addr fitting if the img_type is not bayer format
	 * and the img_bit is smaller than 10bit
	 */
	if (config->img_bit == NXS_IMG_BIT_SM_10)
		value |= ((1 << DMAW_EN_ADDR_FITTING_SHIFT) |
			  DMAW_EN_ADDR_FITTING_MASK);
	regmap_update_bits(dmaw->reg, dmaw->offset + DMAW_CTRL1,
			   (DMAW_EN_EXPANDER_MASK | DMAW_EN_DITHER_MASK |
			    DMAW_EN_ADDR_FITTING_MASK),
			   value);

	/* half height setting */
	value = ((config->half_height_1st_pl <<
		  DMAW_HALF_HEIGHT_1ST_PL_SHIFT) |
		 DMAW_HALF_HEIGHT_1ST_PL_MASK);
	value |= ((config->half_height_2nd_pl <<
		DMAW_HALF_HEIGHT_2ND_PL_SHIFT) |
		 DMAW_HALF_HEIGHT_2ND_PL_MASK);
	/* setting use average value */
	value |= ((config->use_average_value << DMAW_USE_AVERAGE_VALUE_SHIFT)
		| DMAW_USE_AVERAGE_VALUE_MASK);
	/* setting put dummy type */
	if (config->put_dummy_type)
		value |= ((config->put_dummy_type << DMAW_PUT_DUMMY_TYPE_SHIFT)
			  | DMAW_PUT_DUMMY_TYPE_MASK);
	/* total bitwidth setting */
	value |= ((config->total_bitwidth_1st_pl <<
		DMAW_TOTAL_BITWIDTH_1ST_PL_SHIFT) |
		 DMAW_TOTAL_BITWIDTH_1ST_PL_MASK);
	value |= ((config->total_bitwidth_2nd_pl <<
		DMAW_TOTAL_BITWIDTH_2ND_PL_SHIFT) |
		 DMAW_TOTAL_BITWIDTH_2ND_PL_MASK);
	regmap_write(dmaw->reg, dmaw->offset + DMAW_CTRL2, value);

	/* 1st plane - p composition */
	for (i = 0; i < DMAW_P_COMP_COUNT; i++) {
		value = ((config->p0_comp[i].startbit <<
			  DMAW_P_COMP_STARTBIT_SHIFT)
			 | DMAW_P_COMP_STARTBIT_MASK);
		value |= ((config->p0_comp[i].bitwidth <<
			   DMAW_P_COMP_BITWIDTH_SHIFT)
			  | DMAW_P_COMP_BITWIDTH_MASK);
		value |= ((config->p0_comp[i].is_2nd_pl <<
			   DMAW_P_COMP_IS_2ND_PL_SHIFT) |
			  DMAW_P_COMP_IS_2ND_PL_MASK);
		value |= ((config->p0_comp[i].use_userdef <<
			   DMAW_P_COMP_USE_USERDEF_SHIFT) |
			  DMAW_P_COMP_USE_USERDEF_MASK);
		value |= ((config->p0_comp[i].userdef <<
			   DMAW_P_COMP_USERDEF_SHIFT) |
			  DMAW_P_COMP_USERDEF_MASK);
		regmap_write(dmaw->reg, dmaw->offset + DMAW_P0_COMP0 +
			     (i * DMAW_P_COMP_SIZE),
			     value);
	}
	/* 2nd plane - p composition */
	for (i = 0; i < DMAW_P_COMP_COUNT; i++) {
		value = ((config->p1_comp[i].startbit <<
			  DMAW_P_COMP_STARTBIT_SHIFT)
			 | DMAW_P_COMP_STARTBIT_MASK);
		value |= ((config->p1_comp[i].bitwidth <<
			   DMAW_P_COMP_BITWIDTH_SHIFT)
			  | DMAW_P_COMP_BITWIDTH_MASK);
		value |= ((config->p1_comp[i].is_2nd_pl <<
			   DMAW_P_COMP_IS_2ND_PL_SHIFT) |
			  DMAW_P_COMP_IS_2ND_PL_MASK);
		value |= ((config->p1_comp[i].use_userdef <<
			   DMAW_P_COMP_USE_USERDEF_SHIFT) |
			  DMAW_P_COMP_USE_USERDEF_MASK);
		value |= ((config->p1_comp[i].userdef <<
			   DMAW_P_COMP_USERDEF_SHIFT) |
			  DMAW_P_COMP_USERDEF_MASK);
		regmap_write(dmaw->reg, dmaw->offset + DMAW_P1_COMP0 +
			     (i * DMAW_P_COMP_SIZE),
			     value);
	}
}

static u32 dmaw_get_status(struct nxs_dmaw *dmaw, u32 offset, u32 mask,
			     u32 shift)
{
	u32 status;

	regmap_read(dmaw->reg, dmaw->offset + offset, &status);
	return ((status & mask) >> shift);
}

static void dmaw_dump_register(const struct nxs_dev *pthis)
{
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);
	int i, plane_num = 1;

	dev_info(pthis->dev, "============================================\n");
	dev_info(pthis->dev, "DMAW[%d] Dump Register : 0x%8x\n",
		 pthis->dev_inst_index, dmaw->offset);

	dev_info(pthis->dev,
		 "Enable:%d, is_m2m:%d\n",
		 dmaw_get_status(dmaw,
				 DMAW_CTRL0,
				 DMAW_EN_START_MASK,
				 DMAW_EN_START_SHIFT),
		 dmaw->is_m2m);

	dev_info(pthis->dev,
		 "Image width:%d, height:%d\n",
		 dmaw_get_status(dmaw,
				 DMAW_SRC_IMG_SIZE,
				 DMAW_SRC_IMG_WIDTH_MASK,
				 DMAW_SRC_IMG_WIDTH_SHIFT),
		 dmaw_get_status(dmaw,
				 DMAW_SRC_IMG_SIZE,
				 DMAW_SRC_IMG_HEIGHT_MASK,
				 DMAW_SRC_IMG_HEIGHT_SHIFT));

	dev_info(pthis->dev,
		 "Coler Expand Enable:%d, Dither Enable:%d\n",
		 dmaw_get_status(dmaw,
				 DMAW_CTRL1,
				 DMAW_EN_EXPANDER_MASK,
				 DMAW_EN_EXPANDER_SHIFT),
		 dmaw_get_status(dmaw,
				 DMAW_CTRL1,
				 DMAW_EN_DITHER_MASK,
				 DMAW_EN_DITHER_SHIFT));

	dev_info(pthis->dev,
		 "Put Dummy Type:%d, Use Average Value Enable:%d\n",
		 dmaw_get_status(dmaw,
				 DMAW_CTRL2,
				 DMAW_PUT_DUMMY_TYPE_MASK,
				 DMAW_PUT_DUMMY_TYPE_SHIFT),
		 dmaw_get_status(dmaw,
				 DMAW_CTRL2,
				 DMAW_USE_AVERAGE_VALUE_MASK,
				 DMAW_USE_AVERAGE_VALUE_SHIFT));

	dev_info(pthis->dev,
		 "[1st plane] Total Bitwidth:%d, Half Height enable:%d\n",
		 dmaw_get_status(dmaw,
				 DMAW_CTRL2,
				 DMAW_TOTAL_BITWIDTH_1ST_PL_MASK,
				 DMAW_TOTAL_BITWIDTH_1ST_PL_SHIFT),
		 dmaw_get_status(dmaw,
				 DMAW_CTRL2,
				 DMAW_HALF_HEIGHT_1ST_PL_MASK,
				 DMAW_HALF_HEIGHT_1ST_PL_SHIFT));

	for (i = 0; i < DMAW_P_COMP_COUNT; i++) {
		dev_info(pthis->dev,
			 "[%d] start:%d, width:%d, is_2nd:%d userdef:%d-%d\n",
			 i,
			 dmaw_get_status(dmaw,
					 DMAW_P0_COMP0 + (i * DMAW_P_COMP_SIZE),
					 DMAW_P_COMP_STARTBIT_MASK,
					 DMAW_P_COMP_STARTBIT_SHIFT),
			 dmaw_get_status(dmaw,
					 DMAW_P0_COMP0 + (i * DMAW_P_COMP_SIZE),
					 DMAW_P_COMP_BITWIDTH_MASK,
					 DMAW_P_COMP_BITWIDTH_SHIFT),
			 dmaw_get_status(dmaw,
					 DMAW_P0_COMP0 + (i * DMAW_P_COMP_SIZE),
					 DMAW_P_COMP_IS_2ND_PL_MASK,
					 DMAW_P_COMP_IS_2ND_PL_SHIFT),
			 dmaw_get_status(dmaw,
					 DMAW_P0_COMP0 + (i * DMAW_P_COMP_SIZE),
					 DMAW_P_COMP_USE_USERDEF_MASK,
					 DMAW_P_COMP_USE_USERDEF_SHIFT),
			 dmaw_get_status(dmaw,
					 DMAW_P0_COMP0 + (i * DMAW_P_COMP_SIZE),
					 DMAW_P_COMP_USERDEF_MASK,
					 DMAW_P_COMP_USERDEF_SHIFT));
	}

	dev_info(pthis->dev,
		 "[1st plane] Total Bitwidth:%d, Half Height enable:%d\n",
		 dmaw_get_status(dmaw,
				 DMAW_CTRL2,
				 DMAW_TOTAL_BITWIDTH_2ND_PL_MASK,
				 DMAW_TOTAL_BITWIDTH_2ND_PL_SHIFT),
		 dmaw_get_status(dmaw,
				 DMAW_CTRL2,
				 DMAW_HALF_HEIGHT_2ND_PL_MASK,
				 DMAW_HALF_HEIGHT_2ND_PL_SHIFT));

	for (i = 0; i < DMAW_P_COMP_COUNT; i++) {
		dev_info(pthis->dev,
			 "[%d] start:%d, width:%d, is_2nd:%d userdef:%d-%d\n",
			 i,
			 dmaw_get_status(dmaw,
					 DMAW_P1_COMP0 + (i * DMAW_P_COMP_SIZE),
					 DMAW_P_COMP_STARTBIT_MASK,
					 DMAW_P_COMP_STARTBIT_SHIFT),
			 dmaw_get_status(dmaw,
					 DMAW_P1_COMP0 + (i * DMAW_P_COMP_SIZE),
					 DMAW_P_COMP_BITWIDTH_MASK,
					 DMAW_P_COMP_BITWIDTH_SHIFT),
			 dmaw_get_status(dmaw,
					 DMAW_P1_COMP0 + (i * DMAW_P_COMP_SIZE),
					 DMAW_P_COMP_IS_2ND_PL_MASK,
					 DMAW_P_COMP_IS_2ND_PL_SHIFT),
			 dmaw_get_status(dmaw,
					 DMAW_P1_COMP0 + (i * DMAW_P_COMP_SIZE),
					 DMAW_P_COMP_USE_USERDEF_MASK,
					 DMAW_P_COMP_USE_USERDEF_SHIFT),
			 dmaw_get_status(dmaw,
					 DMAW_P1_COMP0 + (i * DMAW_P_COMP_SIZE),
					 DMAW_P_COMP_USERDEF_MASK,
					 DMAW_P_COMP_USERDEF_SHIFT));
	}

	if (dmaw_get_status(dmaw, DMAW_CTRL2,
			    DMAW_TOTAL_BITWIDTH_2ND_PL_MASK,
			    DMAW_TOTAL_BITWIDTH_2ND_PL_SHIFT))
		plane_num = 2;
	dev_info(pthis->dev, "Num of Planes:%d\n", plane_num);

	for (i = 0; i < plane_num; i++) {
		dev_info(pthis->dev,
			 "[PlaneNum:%d] address:0x%8x, stride:%d\n",
			 dmaw_get_status(dmaw,
					 (DMAW_PL0_BASEADDR +
					 (i * DMAW_PL_CONFIG_SIZE)),
					 0xFFFF, 0),
			 dmaw_get_status(dmaw,
					 (DMAW_PL0_STRIDE +
					 (i * DMAW_PL_CONFIG_SIZE)),
					 0xFFFF, 0));
		dev_info(pthis->dev,
			 "Last_flush of Transaction:%d, the Num of Transactions:%d\n",
			 dmaw_get_status(dmaw,
					 (DMAW_PL0_CTRL +
					  (DMAW_PL_CTRL_OFFSET * i)),
					 DMAW_PL_LAST_FLUSH_128X16_TR_MASK,
					 DMAW_PL_LAST_FLUSH_128X16_TR_SHIFT),
			 dmaw_get_status(dmaw,
					 (DMAW_PL0_CTRL +
					  (DMAW_PL_CTRL_OFFSET * i)),
					 DMAW_PL_NUM_OF_128X16_TR_MASK,
					 DMAW_PL_NUM_OF_128X16_TR_SHIFT));
	}

	dev_info(pthis->dev, "Interrupt Status:0x%x\n",
		 dmaw_get_status(dmaw, DMAW_INT, 0xFFFF, 0));

	dev_info(pthis->dev, "============================================\n");
}


static void dmaw_set_interrupt_enable(const struct nxs_dev *pthis,
				      enum nxs_event_type type,
				     bool enable)
{
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);
	u32 mask_v = 0;

	dev_info(pthis->dev, "[%s] type : %d - %s\n",
		 __func__, type, (enable)?"enable":"disable");
	if (enable) {
		if (type & NXS_EVENT_IDLE)
			mask_v |= DMAW_IDLE_INTENB_MASK;
		if (type & NXS_EVENT_DONE) {
			mask_v |= DMAW_DONE_INTENB_MASK;
			/*mask_v |= DMAW_UPDATE_INTENB_MASK;*/
		}
		if (type & NXS_EVENT_ERR) {
			mask_v |= DMAW_RESOL_ERR_INTENB_MASK;
			mask_v |= DMAW_DEC_ERR_INTENB_MASK;
			mask_v |= DMAW_AXI1_ERR_INTENB_MASK;
			mask_v |= DMAW_AXI0_ERR_INTENB_MASK;
			mask_v |= DMAW_SEC_ERR_INTENB_MASK;
		}
	} else {
		if (type & NXS_EVENT_IDLE)
			mask_v |= DMAW_IDLE_INTDISABLE_MASK;
		if (type & NXS_EVENT_DONE) {
			mask_v |= DMAW_DONE_INTDISABLE_MASK;
			mask_v |= DMAW_UPDATE_INTDISABLE_MASK;
		}
		if (type & NXS_EVENT_ERR) {
			mask_v |= DMAW_RESOL_ERR_INTDISABLE_MASK;
			mask_v |= DMAW_DEC_ERR_INTDISABLE_MASK;
			mask_v |= DMAW_AXI1_ERR_INTDISABLE_MASK;
			mask_v |= DMAW_AXI0_ERR_INTDISABLE_MASK;
			mask_v |= DMAW_SEC_ERR_INTDISABLE_MASK;
		}
	}
	regmap_update_bits(dmaw->reg, dmaw->offset + DMAW_INT,
			   mask_v, mask_v);
}

static u32 dmaw_get_interrupt_enable(const struct nxs_dev *pthis,
				     enum nxs_event_type type)
{
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);
	u32 mask_v = 0, enable = 0;

	dev_info(pthis->dev, "[%s] type:%d\n", __func__, type);
	if (type & NXS_EVENT_IDLE)
		mask_v |= DMAW_IDLE_INTENB_MASK;
	if (type & NXS_EVENT_DONE) {
		mask_v |= DMAW_UPDATE_INTENB_MASK;
		mask_v |= DMAW_DONE_INTENB_MASK;
	}
	if (type & NXS_EVENT_ERR) {
		mask_v |= DMAW_RESOL_ERR_INTENB_MASK;
		mask_v |= DMAW_DEC_ERR_INTENB_MASK;
		mask_v |= DMAW_AXI1_ERR_INTENB_MASK;
		mask_v |= DMAW_AXI0_ERR_INTENB_MASK;
		mask_v |= DMAW_SEC_ERR_INTENB_MASK;
	}
	regmap_read(dmaw->reg, dmaw->offset + DMAW_INT, &enable);
	enable &= mask_v;

	return enable;
}

static u32 dmaw_get_interrupt_pending(const struct nxs_dev *pthis,
				      enum nxs_event_type type)
{
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);
	u32 mask_v = 0, pend = 0;

	dev_info(pthis->dev, "[%s] type:%d\n", __func__, type);
	if (type & NXS_EVENT_IDLE)
		mask_v |= DMAW_IDLE_INTPEND_CLR_MASK;
	if (type & NXS_EVENT_DONE) {
		mask_v |= DMAW_DONE_INTPEND_CLR_MASK;
		mask_v |= DMAW_UPDATE_INTPEND_CLR_MASK;
	}
	if (type & NXS_EVENT_ERR) {
		mask_v |= DMAW_RESOL_ERR_INTPEND_CLR_MASK;
		mask_v |= DMAW_DEC_ERR_INTPEND_CLR_MASK;
		mask_v |= DMAW_AXI1_ERR_INTPEND_CLR_MASK;
		mask_v |= DMAW_AXI0_ERR_INTPEND_CLR_MASK;
		mask_v |= DMAW_SEC_ERR_INTPEND_CLR_MASK;
	}
	regmap_read(dmaw->reg, dmaw->offset + DMAW_INT, &pend);
	pend &= mask_v;

	return pend;
}

static enum nxs_event_type
dmaw_get_interrupt_pending_number(struct nxs_dmaw *dmaw)
{
	u32 pend = 0, type = NXS_EVENT_NONE;

	dev_info(dmaw->nxs_dev.dev, "[%s]\n", __func__);
	regmap_read(dmaw->reg, dmaw->offset + DMAW_INT, &pend);

	if (pend & DMAW_IDLE_INTPEND_CLR_MASK)
		type |= NXS_EVENT_IDLE;

	if (pend & DMAW_UPDATE_INTPEND_CLR_MASK)
		type |= NXS_EVENT_DONE;

	if (pend & DMAW_DONE_INTPEND_CLR_MASK)
		type |= NXS_EVENT_DONE;

	if ((pend & DMAW_RESOL_ERR_INTPEND_CLR_MASK) ||
	    (pend & DMAW_DEC_ERR_INTPEND_CLR_MASK) ||
	    (pend & DMAW_AXI1_ERR_INTPEND_CLR_MASK) ||
	    (pend & DMAW_AXI0_ERR_INTPEND_CLR_MASK) ||
	    (pend & DMAW_SEC_ERR_INTPEND_CLR_MASK))
		type |= NXS_EVENT_ERR;

	return type;
}

static void dmaw_clear_interrupt_pending(const struct nxs_dev *pthis,
					 enum nxs_event_type type)
{
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);
	u32 mask_v = 0;

	dev_info(pthis->dev, "[%s] type:%d\n", __func__, type);
	if (type & NXS_EVENT_IDLE)
		mask_v |= DMAW_IDLE_INTPEND_CLR_MASK;

	if (type & NXS_EVENT_DONE) {
		mask_v |= DMAW_UPDATE_INTPEND_CLR_MASK;
		mask_v |= DMAW_DONE_INTPEND_CLR_MASK;
	}

	if (type & NXS_EVENT_ERR) {
		mask_v |= DMAW_RESOL_ERR_INTPEND_CLR_MASK;
		mask_v |= DMAW_DEC_ERR_INTPEND_CLR_MASK;
		mask_v |= DMAW_AXI1_ERR_INTPEND_CLR_MASK;
		mask_v |= DMAW_AXI0_ERR_INTPEND_CLR_MASK;
		mask_v |= DMAW_SEC_ERR_INTPEND_CLR_MASK;
	}
	regmap_update_bits(dmaw->reg, dmaw->offset + DMAW_INT,
			   mask_v, mask_v);
}

static int dmaw_open(const struct nxs_dev *pthis)
{
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
	if (nxs_dev_get_open_count(&dmaw->nxs_dev) == 0) {
		int ret;
		struct clk *clk;
		char dev_name[10] = {0, };

		sprintf(dev_name, "dmaw%d", pthis->dev_inst_index);
		clk = clk_get(pthis->dev, dev_name);
		if (IS_ERR(clk)) {
			dev_err(pthis->dev, "controller clock not fount\n");
			return -ENODEV;
		}
		ret = clk_prepare_enable(clk);
		if (ret) {
			dev_err(pthis->dev,
				"clock failed to prepare enable:%d\n", ret);
			return ret;
		}
#ifndef SIMULATE_INTERRUPT
		ret = request_irq(dmaw->irq, dmaw_irq_handler,
				  IRQF_TRIGGER_NONE, "nxs-dmaw", dmaw);
		if (ret < 0) {
			dev_err(pthis->dev, "failed to request irq:%d\n", ret);
			return ret;
		}
#endif
	}
#ifdef SIMULATE_INTERRUPT
	setup_timer(&dmaw->timer, int_timer_func, (long)dmaw);
#endif
	nxs_dev_inc_open_count(&dmaw->nxs_dev);
	return 0;
}

static int dmaw_close(const struct nxs_dev *pthis)
{
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
#ifdef SIMULATE_INTERRUPT
	if (list_empty(&pthis->irq_callback))
		del_timer(&dmaw->timer);
#endif

	if (dmaw_check_busy(dmaw)) {
		dev_err(pthis->dev, "dmaw is busy status\n");
		return -EBUSY;
	}

	nxs_dev_dec_open_count(&dmaw->nxs_dev);
	if (nxs_dev_get_open_count(&dmaw->nxs_dev) == 0) {
		struct clk *clk;
		char dev_name[10] = {0, };

		dmaw_reset_register(dmaw);
#ifndef SIMULATE_INTERRUPT
		free_irq(dmaw->irq, dmaw);
#endif
		sprintf(dev_name, "dmaw%d", pthis->dev_inst_index);
		clk = clk_get(pthis->dev, dev_name);
		if (IS_ERR(clk)) {
			dev_err(pthis->dev, "controller clock not found\n");
			return -ENODEV;
		}
		clk_disable_unprepare(clk);
	}
	return 0;
}

static int dmaw_set_dirty(const struct nxs_dev *pthis, u32 type)
{
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);
	u32 dirty_val;

	dev_info(pthis->dev, "[%s]\n", __func__);

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;
	switch (pthis->dev_inst_index) {
	case 0:
		dirty_val = DMAW0_DIRTY;
		break;
	case 1:
		dirty_val = DMAW1_DIRTY;
		break;
	case 2:
		dirty_val = DMAW2_DIRTY;
		break;
	case 3:
		dirty_val = DMAW3_DIRTY;
		break;
	case 4:
		dirty_val = DMAW4_DIRTY;
		break;
	case 5:
		dirty_val = DMAW5_DIRTY;
		break;
	case 6:
		dirty_val = DMAW6_DIRTY;
		break;
	case 7:
		dirty_val = DMAW7_DIRTY;
		break;
	case 8:
		dirty_val = DMAW8_DIRTY;
		break;
	case 9:
		dirty_val = DMAW9_DIRTY;
		break;
	case 10:
		dirty_val = DMAW10_DIRTY;
		break;
	case 11:
		dirty_val = DMAW11_DIRTY;
		break;
	default:
		dev_err(pthis->dev, "invalid inst %d\n", pthis->dev_inst_index);
		return -ENODEV;
	}

	return regmap_write(dmaw->reg, DMAW_DIRTYSET_OFFSET, dirty_val);
}

static int dmaw_start(const struct nxs_dev *pthis)
{
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);
#ifdef SIMULATE_INTERRUPT
	mod_timer(&dmaw->timer, jiffies +
		  msecs_to_jiffies(INT_TIMEOUT_MS));
#endif
	dmaw_set_dpc_mode(dmaw, ~(dmaw->is_m2m));
	dmaw_set_start(dmaw, true);

	return 0;
}

static int dmaw_stop(const struct nxs_dev *pthis)
{
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);

	dev_info(pthis->dev, "[%s]\n", __func__);

#ifdef SIMULATE_INTERRUPT
	del_timer(&dmaw->timer);
#endif
	dmaw_set_dpc_mode(dmaw, false);
	dmaw_set_start(dmaw, false);

	return 0;
}

static int dmaw_set_dstformat(const struct nxs_dev *pthis,
			   const struct nxs_control *pparam)
{
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);
	struct nxs_plane_format config;

	dev_info(pthis->dev, "[%s] widht = %d, height = %d, format = 0x%x\n",
		 __func__, pparam->u.format.width, pparam->u.format.height,
		 pparam->u.format.pixelformat);
	memset(&config, 0x0, sizeof(struct nxs_plane_format));
	nxs_get_plane_format(&dmaw->nxs_dev, pparam->u.format.pixelformat,
			     &config);
	if (config.img_type == NXS_IMG_MAX) {
		dev_err(pthis->dev,
			"failed to get image information from the format\n");
		return -EINVAL;
	}
	dmaw_set_src_imgsize(dmaw, pparam->u.format.width,
			     pparam->u.format.height);
	dmaw_set_plane_format(dmaw, &config);
	dmaw_set_axi_control(dmaw);
	dmaw_set_transaction_control(dmaw, pparam->u.format.width);
	return 0;
}

static int dmaw_get_dstformat(const struct nxs_dev *pthis,
			   struct nxs_control *pparam)
{
	return 0;
}

static int dmaw_set_buffer(const struct nxs_dev *pthis,
			   const struct nxs_control *pparam)
{
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);
	u32 i, buffer_num;

	dev_info(pthis->dev, "[%s]\n", __func__);
	buffer_num = pparam->u.buffer.num_planes;
	if (buffer_num > NXS_DEV_MAX_PLANES)
		return -EINVAL;

	for (i = 0; i < buffer_num; i++) {
		dev_info(pthis->dev, "[%d] address = %p, stride = %d\n",
			 i, pparam->u.buffer.address[i],
			 pparam->u.buffer.strides[i]);
		regmap_write(dmaw->reg,
			     dmaw->offset + DMAW_PL0_BASEADDR +
			     (i * DMAW_PL_CONFIG_SIZE),
			     pparam->u.buffer.address[i]);
		regmap_write(dmaw->reg,
			     dmaw->offset + DMAW_PL0_STRIDE +
			     (i * DMAW_PL_CONFIG_SIZE),
			     pparam->u.buffer.strides[i]);
	}
	return 0;
}

static int dmaw_get_buffer(const struct nxs_dev *pthis,
			   struct nxs_control *pparam)
{
	dev_info(pthis->dev, "[%s]\n", __func__);
	return 0;
}

static int dmaw_set_video(const struct nxs_dev *pthis,
			  struct nxs_control *pparam)
{
	struct nxs_dmaw *dmaw = nxs_to_dmaw(pthis);
	u32 type = pparam->u.video.type;
	u32 field = pparam->u.video.field;

	dev_info(pthis->dev, "[%s] type:%d, field:%d\n",
		 __func__, type, field);

	if (type == NXS_VIDEO_TYPE_M2M)
		dmaw->is_m2m = true;
	else
		dmaw->is_m2m = false;
	if (field == NXS_FIELD_INTERLACED)
		dmaw_set_interlaced_mode(dmaw, true);
	return 0;
}

static int dmaw_get_video(const struct nxs_dev *pthis,
			  struct nxs_control *pparam)
{
	return 0;
}

static int nxs_dmaw_probe(struct platform_device *pdev)
{
	int ret;
	struct nxs_dmaw *dmaw;
	struct nxs_dev *nxs_dev;
	struct resource *res;

	dmaw = devm_kzalloc(&pdev->dev, sizeof(*dmaw), GFP_KERNEL);
	if (!dmaw)
		return -ENOMEM;

	nxs_dev = &dmaw->nxs_dev;
	ret = nxs_dev_parse_dt(pdev, nxs_dev);
	if (ret)
		return ret;

	dmaw->reg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						    "syscon");
	if (IS_ERR(dmaw->reg)) {
		dev_err(&pdev->dev, "unable to get syscon\n");
		return PTR_ERR(dmaw->reg);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing IO resource\n");
		return -ENODEV;
	}
	dmaw->offset = res->start;
	dmaw->irq = platform_get_irq(pdev, 0);
	dmaw->is_m2m = false;

	nxs_dev->set_interrupt_enable = dmaw_set_interrupt_enable;
	nxs_dev->get_interrupt_enable = dmaw_get_interrupt_enable;
	nxs_dev->get_interrupt_pending = dmaw_get_interrupt_pending;
	nxs_dev->clear_interrupt_pending = dmaw_clear_interrupt_pending;
	nxs_dev->open = dmaw_open;
	nxs_dev->close = dmaw_close;
	nxs_dev->start = dmaw_start;
	nxs_dev->stop = dmaw_stop;
	nxs_dev->set_dirty = dmaw_set_dirty;
	nxs_dev->dump_register = dmaw_dump_register;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;
	nxs_dev->dev_services[0].type = NXS_CONTROL_DST_FORMAT;
	nxs_dev->dev_services[0].set_control = dmaw_set_dstformat;
	nxs_dev->dev_services[0].get_control = dmaw_get_dstformat;
	nxs_dev->dev_services[1].type = NXS_CONTROL_BUFFER;
	nxs_dev->dev_services[1].set_control = dmaw_set_buffer;
	nxs_dev->dev_services[1].get_control = dmaw_get_buffer;
	nxs_dev->dev_services[2].type = NXS_CONTROL_VIDEO;
	nxs_dev->dev_services[2].set_control = dmaw_set_video;
	nxs_dev->dev_services[2].get_control = dmaw_get_video;

	nxs_dev->dev = &pdev->dev;

	ret = register_nxs_dev(&dmaw->nxs_dev);
	if (ret)
		return ret;

	dev_info(dmaw->nxs_dev.dev, "%s: success\n", __func__);
	platform_set_drvdata(pdev, dmaw);

	return 0;
}

static int nxs_dmaw_remove(struct platform_device *pdev)
{
	struct nxs_dmaw *dmaw = platform_get_drvdata(pdev);

	if (dmaw)
		unregister_nxs_dev(&dmaw->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_dmaw_match[] = {
	{ .compatible = "nexell,dmaw-nxs-1.0", },
	{},
};

static struct platform_driver nxs_dmaw_driver = {
	.probe	= nxs_dmaw_probe,
	.remove	= nxs_dmaw_remove,
	.driver	= {
		.name = "nxs-dmaw",
		.of_match_table = of_match_ptr(nxs_dmaw_match),
	},
};

static int __init nxs_dmaw_init(void)
{
	return platform_driver_register(&nxs_dmaw_driver);
}
/* subsys_initcall(nxs_dmaw_init); */
fs_initcall(nxs_dmaw_init);

static void __exit nxs_dmaw_exit(void)
{
	platform_driver_unregister(&nxs_dmaw_driver);
}
module_exit(nxs_dmaw_exit);

MODULE_DESCRIPTION("Nexell Stream DMAW driver");
MODULE_AUTHOR("Hyejung Kwon, <cjscld15@nexell.co.kr>");
MODULE_LICENSE("GPL");
