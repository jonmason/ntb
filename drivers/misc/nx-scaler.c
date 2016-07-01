/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongkeun, Choi <jkchoi@nexell.co.kr>
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
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/reset.h>

#include <uapi/linux/media-bus-format.h>
#include <uapi/linux/nx-scaler.h>

#include <asm/irq.h>

#include "nx-scaler.h"

#define COMMAND_BUFFER_SIZE	PAGE_SIZE

#define PHY_BASEADDR_SCALER_MODULE	0xC0066000
#define	NUMBER_OF_SCALER_MODULE		1
#define WAIT_TIMEOUT_HZ			(HZ/5) /* 200ms */

static const char drv_name[] = "scaler";

struct nx_scaler_register_set {
	u32 scrunreg;
	u32 sccfgreg;
	u32 scintreg;
	u32 scsrcaddreg;
	u32 scsrcstride;
	u32 scsrcsizereg;
	u32 scdestaddreg0;
	u32 scdeststride0;
	u32 scdestaddreg1;
	u32 scdeststride1;
	u32 scdestsizereg;
	u32 deltaxreg;
	u32 deltayreg;
	u32 hvsoftreg;
	u32 cmdbufaddr;
	u32 cmdbufcon;
	u32 yvfilter[3][8];
	u32 __reserved00[24];
	int32_t yhfilter[5][32];
};

enum {
	NX_SCALER_INT_DONE = 0,
	NX_SCALER_INT_CMD_PROC = 1,
};

/**
 * filter sets
 */
struct filter_table {
	int yvfilter[3][8][4];
	int yhfilter[5][32][2];
};

static struct filter_table _default_filter_table = {
	.yvfilter = {
		{
			{
				61, 58, 55, 52
			},
			{
				50, 48, 45, 42
			},
			{
				40, 38, 35, 32
			},
			{
				30, 28, 25, 22
			},
			{
				18, 16, 14, 13
			},
			{
				12, 11, 10, 9
			},
			{
				8, 7, 6, 5
			},
			{
				4, 3, 2, 1
			},
		},
		{
			{
				66, 68, 70, 72
			},
			{
				73, 74, 76, 78
			},
			{
				79, 80, 82, 84
			},
			{
				85, 86, 87, 88
			},
			{
				88, 87, 86, 85
			},
			{
				84, 82, 80, 79
			},
			{
				78, 76, 74, 73
			},
			{
				72, 70, 68, 66
			},
		},
		{
			{
				1,  2,  3,  4
			},
			{
				5,  6,  7,  8
			},
			{
				9, 10, 11, 12
			},
			{
				13, 14, 16, 18
			},
			{
				22, 25, 28, 30
			},
			{
				32, 35, 38, 40
			},
			{
				42, 45, 48, 50
			},
			{
				52, 55, 58, 61
			},
		}
	},
	.yhfilter = {
		{
			{
				-2, -5
			},
			{
				-8, -8
			},
			{
				-8, -10
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -11
			},
			{
				-10, -9
			},
			{
				-8, -8
			},
			{
				-8, -8
			},
			{
				-8, -8
			},
			{
				-8, -8
			},
			{
				-8, -8
			},
			{
				-7, -6
			},
			{
				-5, -4
			},
			{
				-4, -4
			},
			{
				-4, -4
			},
			{
				-4, -4
			},
			{
				-4, -4
			},
			{
				-4, -4
			},
			{
				-4, -4
			},
			{
				-4, -4
			},
			{
				-3, -2
			},
			{
				-2, -2
			},
			{
				-1, 0
			},
		},
		{
			{
				252, 248
			},
			{
				244, 238
			},
			{
				232, 227
			},
			{
				222, 216
			},
			{
				210, 204
			},
			{
				198, 192
			},
			{
				186, 181
			},
			{
				176, 170
			},
			{
				164, 158
			},
			{
				152, 145
			},
			{
				138, 133
			},
			{
				128, 123
			},
			{
				118, 113
			},
			{
				108, 102
			},
			{
				96, 92
			},
			{
				88, 88
			},
			{
				86, 80
			},
			{
				75, 70
			},
			{
				67, 64
			},
			{
				61, 58
			},
			{
				54, 50
			},
			{
				44, 38
			},
			{
				35, 32
			},
			{
				30, 28
			},
			{
				26, 24
			},
			{
				22, 20
			},
			{
				18, 16
			},
			{
				14, 12
			},
			{
				10, 8
			},
			{
				6, 4
			},
			{
				2, 0
			},
			{
				-1, -2
			},
		},
		{
			{
				264, 271
			},
			{
				278, 282
			},
			{
				286, 292
			},
			{
				298, 302
			},
			{
				306, 310
			},
			{
				314, 318
			},
			{
				322, 325
			},
			{
				328, 332
			},
			{
				336, 340
			},
			{
				344, 348
			},
			{
				352, 352
			},
			{
				352, 354
			},
			{
				356, 358
			},
			{
				360, 362
			},
			{
				364, 362
			},
			{
				360, 354
			},
			{
				354, 360
			},
			{
				362, 364
			},
			{
				362, 360
			},
			{
				358, 356
			},
			{
				354, 352
			},
			{
				352, 352
			},
			{
				348, 344
			},
			{
				340, 336
			},
			{
				332, 328
			},
			{
				325, 322
			},
			{
				318, 314
			},
			{
				310, 306
			},
			{
				302, 298
			},
			{
				292, 286
			},
			{
				282, 278
			},
			{
				271, 264
			},
		},
		{
			{
				-2, -1
			},
			{
				0, 2
			},
			{
				4, 6
			},
			{
				8, 10
			},
			{
				12, 14
			},
			{
				16, 18
			},
			{
				20, 22
			},
			{
				24, 26
			},
			{
				28, 30
			},
			{
				32, 35
			},
			{
				38, 44
			},
			{
				50, 54
			},
			{
				58, 61
			},
			{
				64, 67
			},
			{
				70, 75
			},
			{
				80, 86
			},
			{
				88, 88
			},
			{
				92, 96
			},
			{
				102, 108
			},
			{
				113, 118
			},
			{
				123, 128
			},
			{
				133, 138
			},
			{
				145, 152
			},
			{
				158, 164
			},
			{
				170, 176
			},
			{
				181, 186
			},
			{
				192, 198
			},
			{
				204, 210
			},
			{
				216, 222
			},
			{
				227, 232
			},
			{
				238, 244
			},
			{
				248, 252
			},
		},
		{
			{
				0, -1
			},
			{
				-2, -2
			},
			{
				-2, -3
			},
			{
				-4, -4
			},
			{
				-4, -4
			},
			{
				-4, -4
			},
			{
				-4, -4
			},
			{
				-4, -4
			},
			{
				-4, -4
			},
			{
				-4, -4
			},
			{
				-4, -5
			},
			{
				-6, -7
			},
			{
				-8, -8
			},
			{
				-8, -8
			},
			{
				-8, -8
			},
			{
				-8, -8
			},
			{
				-8, -8
			},
			{
				-9, -10
			},
			{
				-11, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-12, -12
			},
			{
				-10, -8
			},
			{
				-8, -8
			},
			{
				-5, -2
			},
		},
	}
};

#define MAKE_YVFILTER(val0, val1, val2, val3) (\
					       ((val3 & 0x000000ff) << 24) | \
					       ((val2 & 0x000000ff) << 16) | \
					       ((val1 & 0x000000ff) << 8)  | \
					       ((val0 & 0x000000ff) << 0))

#define MAKE_YHFILTER(val0, val1) (\
				   ((val1 & 0x0000ffff) << 16) | \
				   ((val0 & 0x0000ffff) << 0)    \
				  )
/**
 * hw specific functions
 */
static struct {
	struct nx_scaler_register_set *pregister;
} __g_module_variables[NUMBER_OF_SCALER_MODULE] = {
	{
		NULL,
	},
};

int nx_scaler_initialize(void)
{
	static bool scaler_binit;
	u32 i;

	scaler_binit = false;

	if (false == scaler_binit) {
		for (i = 0; i < NUMBER_OF_SCALER_MODULE; i++)
			__g_module_variables[i].pregister = NULL;

		scaler_binit = true;
	}
	return true;
}

void nx_scaler_set_yvfilter(u32 module_index, u32 filter_sel, u32 filter_index,
			    u32 filter_val)
{
	writel((u32)filter_val,
	       &__g_module_variables[module_index]
	       .pregister->yvfilter[filter_sel][filter_index]);
}

void nx_scaler_set_yhfilter(u32 module_index, u32 filter_sel, u32 filter_index,
			    u32 filter_val)
{
	writel((u32)filter_val,
	       &__g_module_variables[module_index]
	       .pregister->yhfilter[filter_sel][filter_index]);
}

void nx_scaler_set_base_address(u32 module_index, void *base_address)
{
	__g_module_variables[module_index].pregister =
	    (struct nx_scaler_register_set *)base_address;
}

void nx_scaler_set_interrupt_enable_all(u32 module_index, int enable)
{
	const u32 sc_int_enb_mask = 0x03;
	const u32 sc_int_enb_bitpos = 16;

	if (enable)
		writel((sc_int_enb_mask << sc_int_enb_bitpos),
		&__g_module_variables[module_index].pregister->scintreg);
	else
		writel(0x00,
		    &__g_module_variables[module_index].pregister->scintreg);
}

void nx_scaler_clear_interrupt_pending(u32 module_index, int32_t int_num)
{
	register u32 regval;
	const u32 sc_int_clr_bitpos = 8;

	regval = __g_module_variables[module_index].pregister->scintreg |
		 (0x01 << (sc_int_clr_bitpos + int_num));
	writel(regval, &__g_module_variables[module_index].pregister->scintreg);
}

void nx_scaler_clear_interrupt_pending_all(u32 module_index)
{
	const u32 sc_int_clr_bitpos = 8;
	register u32 regval;

	regval = __g_module_variables[module_index].pregister->scintreg |
		 (0x03 << sc_int_clr_bitpos);
	writel(regval, &__g_module_variables[module_index].pregister->scintreg);
}

void nx_scaler_set_filter_enable(u32 module_index, int enable)
{
	const u32 fenb_mask = (0x03 << 0);
	register u32 temp;

	temp = __g_module_variables[module_index].pregister->sccfgreg;

	if (true == enable)
		temp |= fenb_mask;
	else
		temp &= ~fenb_mask;

	writel(temp, &__g_module_variables[module_index].pregister->sccfgreg);
}

void nx_scaler_set_mode(u32 module_index, int mode)
{
	const u32 mod_mask = (0x07 << 24);
	const u32 mod_bitpos = 24;
	register u32 temp;

	temp = __g_module_variables[module_index].pregister->sccfgreg;
	temp = (temp & ~mod_mask) | ((u32)mode << mod_bitpos);

	writel(temp, &__g_module_variables[module_index].pregister->sccfgreg);
}

void nx_scaler_stop(u32 module_index)
{
	writel(0x00, &__g_module_variables[module_index].pregister->scrunreg);
}

void nx_scaler_set_cmd_buf_addr(u32 module_index, u32 addr)
{
	writel(addr, &__g_module_variables[module_index].pregister->cmdbufaddr);
}

void nx_scaler_run_cmd_buf(u32 module_index)
{
	writel(0x01, &__g_module_variables[module_index].pregister->cmdbufcon);
}

void nx_scaler_set_interrupt_enable(u32 module_index, int32_t int_num,
				    int enable)
{
	const u32 sc_int_enb_bitpos = 16;

	writel(((u32)enable << (sc_int_enb_bitpos + int_num)),
	       &__g_module_variables[module_index].pregister->scintreg);
}

static inline void _hw_set_filter_table(struct nx_scaler *me,
					struct filter_table *table)
{
	int i, j;

	for (i = 0; i < 3; i++)
		for (j = 0; j < 8; j++)
			nx_scaler_set_yvfilter(0, i, j,
					       MAKE_YVFILTER(
						table->yvfilter[i][j][0],
						table->yvfilter[i][j][1],
						table->yvfilter[i][j][2],
						table->yvfilter[i][j][3]));

	for (i = 0; i < 5; i++)
		for (j = 0; j < 32; j++)
			nx_scaler_set_yhfilter(0, i, j,
					       MAKE_YHFILTER(
						table->yhfilter[i][j][0],
						table->yhfilter[i][j][1]));
}

static int get_phy_addr_from_fd(struct device *dev, int fd, bool is_src,
				dma_addr_t *phy_addr)
{
	struct dma_buf	*dmabuf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	u32 direction;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		pr_err("%s: can't get dambuf : fd{%d}\n", __func__, fd);
		return -EINVAL;
	}

	attach = dma_buf_attach(dmabuf, dev);
	if (IS_ERR(attach)) {
		pr_err("fail to attach dmabuf\n");
		return -EINVAL;
	}

	if (is_src)
		direction = DMA_TO_DEVICE;
	else
		direction = DMA_FROM_DEVICE;

	sgt = dma_buf_map_attachment(attach, direction);
	if (IS_ERR(sgt)) {
		pr_err("Error getting dmabuf scatterlist\n");
		return -EINVAL;
	}

	*phy_addr = sg_dma_address(sgt->sgl);

	dma_buf_unmap_attachment(attach, sgt, direction);
	dma_buf_detach(dmabuf, attach);
	dma_buf_put(dmabuf);

	return 0;
}

static int _hw_init(struct nx_scaler *me)
{
	int ret = 0;
	struct reset_control *rst;
	unsigned long rate;

	if (me->pdev == NULL)
		return 0;

	nx_scaler_initialize();
	nx_scaler_set_base_address(0, me->base);
	me->irq = platform_get_irq(me->pdev, 0);

	me->clk = clk_get(&me->pdev->dev, "scaler");
	if (IS_ERR(me->clk)) {
		pr_err("%s: controller clock not found\n",
			dev_name(&me->pdev->dev));

		return PTR_ERR(me->clk);
	}

	ret = clk_prepare_enable(me->clk);
	if (ret) {
		pr_err("scaler: clock failed to prepare & enable: %d\n", ret);
		clk_put(me->clk);
		return ret;
	}

	rate = clk_get_rate(me->clk);
	rst = devm_reset_control_get(&me->pdev->dev, "scaler-reset");
	if (!rst) {
		dev_err(&me->pdev->dev, "failied to get reset control\n");
		return -EINVAL;
	}

	if (reset_control_status(rst))
		reset_control_reset(rst);

	nx_scaler_set_interrupt_enable_all(0, false);
	nx_scaler_clear_interrupt_pending_all(0);
	nx_scaler_set_filter_enable(0, true);
	nx_scaler_set_mode(0, 0);

	return ret;
}

static void _hw_cleanup(struct nx_scaler *me)
{
	free_irq(me->irq, me);

	nx_scaler_stop(0);
	nx_scaler_set_interrupt_enable_all(0, false);
	nx_scaler_clear_interrupt_pending_all(0);

	clk_disable_unprepare(me->clk);
}

static int _hw_set_format(struct nx_scaler *me)
{
	nx_scaler_set_filter_enable(0, true);

	return 0;
}

static inline void _make_cmd(u32 *c)
{
	u32 command = 0;

	command |= (11-1) << 0;
	command |= 3 << (10+0);
	*c = command;
}

static bool _is_running(struct nx_scaler *me)
{
	int val = atomic_read(&me->running);

	return val == 1;
}

static void _set_running(struct nx_scaler *me)
{
	atomic_set(&me->running, 1);
}

static void _clear_running(struct nx_scaler *me)
{
	atomic_set(&me->running, 0);
}

static irqreturn_t _scaler_irq_handler(int irq, void *param)
{
	struct nx_scaler *me = (struct nx_scaler *)param;

	nx_scaler_clear_interrupt_pending(0, NX_SCALER_INT_CMD_PROC);

	_clear_running(me);
	wake_up_interruptible(&me->wq_end);

	return IRQ_HANDLED;
}

static int _make_command_buffer(struct nx_scaler *me,
				     struct nx_scaler_ioctl_data *data)
{
	u32 *cmd_buffer;
	u32 src_width, src_height, src_code;
	u32 dst_width, dst_height, dst_code;
	u32 cb_src_width, cb_src_height, cb_dst_width, cb_dst_height;
	dma_addr_t src_phy_addr, dst_phy_addr;

	dma_addr_t src_addr, dst_addr;
	u32 src_y_pos = 0, src_c_pos = 0;

	src_width = data->src_width;
	src_height = data->src_height;
	src_code = data->src_code;
	dst_width = data->dst_width;
	dst_height = data->dst_height;
	dst_code = data->dst_code;

	if ((data->crop.x > 0) || (data->crop.y > 0)) {
		src_y_pos = ((data->crop.y) * data->src_stride[0])
			+ data->crop.x;
		src_c_pos = (((data->crop.y/2)) * data->src_stride[1])
			+ (data->crop.x/2);
	}

	if (data->crop.width > 0)
		src_width = data->crop.width;
	if (data->crop.height > 0)
		src_height = data->crop.height;

	cmd_buffer = me->command_buffer_vir;

	/* Y command buffer */
	/* header */
	_make_cmd(cmd_buffer); cmd_buffer++;
	/* Source Address Register */
	get_phy_addr_from_fd(&me->pdev->dev, data->src_fds[0], true,
				&src_phy_addr);
	src_addr = src_phy_addr;
	*cmd_buffer = src_addr+src_y_pos;
	cmd_buffer++;
	/* Source Stride Register */
	*cmd_buffer = data->src_stride[0]; cmd_buffer++;
	/* Source Size Register */
	*cmd_buffer = ((src_height - 1) << 16) | (src_width - 1);
	cmd_buffer++;

	/* Destination Address */
	get_phy_addr_from_fd(&me->pdev->dev, data->dst_fds[0], false,
				&dst_phy_addr);
	dst_addr = dst_phy_addr;
	*cmd_buffer = dst_addr;
	cmd_buffer++;
	/* Destination Stride Register */
	*cmd_buffer = data->dst_stride[0];
	cmd_buffer++;
	/* not use Destination Address1, Destination Stride Register1 */
	cmd_buffer++;
	cmd_buffer++;
	/* Destination Size Register */
	*cmd_buffer = ((dst_height - 1) << 16) | (dst_width - 1);
	cmd_buffer++;
	/* Horizontal Delta Register */
	*cmd_buffer = (src_width << 16) / (dst_width - 1);
	cmd_buffer++;
	/* Vertical Delta Register */
	*cmd_buffer = (src_height << 16) / (dst_height - 1);
	cmd_buffer++;
	/* Ratio Reset Value Register : TODO fixed ??? */
	*cmd_buffer = 0x00080010;
	cmd_buffer++;

	/* workaround */
	*cmd_buffer = 0; cmd_buffer++;
	*cmd_buffer = 0x00000001; cmd_buffer++;
	*cmd_buffer = (1 << 29) | (1 << 10); cmd_buffer++;
	*cmd_buffer = 0x00000003; cmd_buffer++;

	/* CB command buffer */
	if (src_code == MEDIA_BUS_FMT_YUYV8_2X8) {
		/* 420 */
		cb_src_width = src_width >> 1;
		cb_src_height = src_height >> 1;
	} else if (src_code == MEDIA_BUS_FMT_YUYV8_1_5X8) {
		/* 420 */
		cb_src_width = src_width >> 1;
		cb_src_height = src_height >> 1;
	} else if (src_code == MEDIA_BUS_FMT_YUYV8_1X16) {
		/* 422 */
		cb_src_width = src_width >> 1;
		cb_src_height = src_height;
	} else {
		cb_src_width = src_width;
		cb_src_height = src_height;
	}

	if (dst_code == MEDIA_BUS_FMT_YUYV8_2X8) {
		/* 420 */
		cb_dst_width = dst_width >> 1;
		cb_dst_height = dst_height >> 1;
	} else if (dst_code == MEDIA_BUS_FMT_YUYV8_1_5X8) {
		/* 420 */
		cb_dst_width = dst_width >> 1;
		cb_dst_height = dst_height >> 1;
	} else if (dst_code == MEDIA_BUS_FMT_YUYV8_1X16) {
		/* 422 */
		cb_dst_width = dst_width >> 1;
		cb_dst_height = dst_height;
	} else {
		cb_dst_width = dst_width;
		cb_dst_height = dst_height;
	}

	_make_cmd(cmd_buffer);
	cmd_buffer++;
	src_addr += (data->src_stride[0] * ALIGN(data->src_height, 16));
	*cmd_buffer = src_addr + src_c_pos;
	cmd_buffer++;
	*cmd_buffer = data->src_stride[1];
	cmd_buffer++;
	*cmd_buffer = ((cb_src_height - 1) << 16) | (cb_src_width - 1);
	cmd_buffer++;

	dst_addr += (data->dst_stride[0] * ALIGN(data->dst_height, 16));
	*cmd_buffer = dst_addr;
	cmd_buffer++;
	*cmd_buffer = data->dst_stride[1];
	cmd_buffer++;
	cmd_buffer++;
	cmd_buffer++;
	*cmd_buffer = ((cb_dst_height - 1) << 16) | (cb_dst_width - 1);
	cmd_buffer++;
	*cmd_buffer = (cb_src_width << 16) / (cb_dst_width - 1);
	cmd_buffer++;
	*cmd_buffer = (cb_src_height << 16) / (cb_dst_height - 1);
	cmd_buffer++;
	*cmd_buffer = 0x00080010; cmd_buffer++;

	*cmd_buffer = 0;
	cmd_buffer++;
	*cmd_buffer = 0x00000001;
	cmd_buffer++;
	*cmd_buffer = (1 << 29) | (1 << 10);
	cmd_buffer++;
	*cmd_buffer = 0x00000003;
	cmd_buffer++;

	_make_cmd(cmd_buffer);
	cmd_buffer++;

	src_addr += (data->src_stride[1] * ALIGN(data->src_height >> 1, 16));
	*cmd_buffer = src_addr + src_c_pos;
	cmd_buffer++;
	*cmd_buffer = data->src_stride[2];
	cmd_buffer++;
	*cmd_buffer = ((cb_src_height - 1) << 16) | (cb_src_width - 1);
	cmd_buffer++;

	dst_addr += (data->dst_stride[1] * ALIGN(data->dst_height >> 1, 16));
	*cmd_buffer = dst_addr;
	cmd_buffer++;
	*cmd_buffer = data->dst_stride[2];
	cmd_buffer++;
	cmd_buffer++;
	cmd_buffer++;
	*cmd_buffer = ((cb_dst_height - 1) << 16) | (cb_dst_width - 1);
	cmd_buffer++;
	*cmd_buffer = (cb_src_width << 16) / (cb_dst_width - 1);
	cmd_buffer++;
	*cmd_buffer = (cb_src_height << 16) / (cb_dst_height - 1);
	cmd_buffer++;
	*cmd_buffer = 0x00080010;
	cmd_buffer++;

	*cmd_buffer = 0;
	cmd_buffer++;
	*cmd_buffer = 0x00000001;
	cmd_buffer++;
	*cmd_buffer = (1 << 29) | (1 << 27) | (1 << 10);
	cmd_buffer++;
	*cmd_buffer = 0x00000003;
	cmd_buffer++;

	return 0;
}


static int _set_and_run(struct nx_scaler *me,
	struct nx_scaler_ioctl_data *data)
{
	_set_running(me);

	_make_command_buffer(me, data);
	nx_scaler_set_cmd_buf_addr(0, me->command_buffer_phy);
	nx_scaler_set_interrupt_enable(0, NX_SCALER_INT_CMD_PROC, true);
	nx_scaler_set_mode(0, 0);
	nx_scaler_run_cmd_buf(0);

	if (!wait_event_interruptible_timeout(me->wq_end,
		      atomic_read(&me->running) == 0, WAIT_TIMEOUT_HZ)) {
		_clear_running(me);

		nx_scaler_stop(0);
		nx_scaler_set_interrupt_enable_all(0, false);
		nx_scaler_clear_interrupt_pending_all(0);

		clk_disable_unprepare(me->clk);

		_hw_init(me);

		_hw_set_filter_table(me, &_default_filter_table);
		_hw_set_format(me);
	}

	return 0;
}

static int nx_scaler_open(struct inode *inode, struct file *filp)
{
	int ret;
	struct nx_scaler *me = container_of(filp->private_data,
				struct nx_scaler, miscdev);

	if (atomic_read(&me->open_count) > 0) {
		atomic_inc(&me->open_count);
		atomic_read(&me->open_count);

		return 0;
	}
	_hw_init(me);
	_hw_set_filter_table(me, &_default_filter_table);
	_hw_set_format(me);
	ret = request_irq(me->irq, _scaler_irq_handler, IRQF_TRIGGER_NONE,
			  "nx-scaler", me);
	if (ret < 0) {
		pr_err("%s: failed to request_irq()\n", __func__);

		return ret;
	}
	atomic_inc(&me->open_count);

	return 0;
}

static int nx_scaler_release(struct inode *inode,
				  struct file *file)
{
	struct nx_scaler *me = (struct nx_scaler *)file->private_data;

	atomic_dec(&me->open_count);

	if (atomic_read(&me->open_count) == 0)
		_hw_cleanup(me);

	_clear_running(me);

	return 0;
}

static long nx_scaler_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg)
{
	long ret = 0;
	struct nx_scaler *me = (struct nx_scaler *)file->private_data;

	mutex_lock(&me->mutex);

	switch (cmd) {
	case IOCTL_SCALER_SET_AND_RUN:
		{
			struct nx_scaler_ioctl_data data;

			if (copy_from_user(&data, (void __user *)arg,
				sizeof(struct nx_scaler_ioctl_data))) {
				pr_info("%s: failed to copy_from_user()\n",
					__func__);
				ret = -EFAULT;

				goto END;
			}

			ret = _set_and_run(me, &data);
		}
		break;
	default:
		ret = -EFAULT;
	}

END:
	mutex_unlock(&me->mutex);

	return ret;
}

static const struct file_operations nx_scaler_ops = {
	.owner          = THIS_MODULE,
	.open           = nx_scaler_open,
	.release        = nx_scaler_release,
	.unlocked_ioctl = nx_scaler_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= nx_scaler_ioctl,
#endif
};

static struct miscdevice nx_scaler_misc_device = {
	.minor          = MISC_DYNAMIC_MINOR,
	.name           = "scaler",
	.fops           = &nx_scaler_ops,
};

static int nx_scaler_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct nx_scaler *me = NULL;
	struct resource *res = NULL;

	me = devm_kzalloc(&pdev->dev, sizeof(struct nx_scaler), GFP_KERNEL);
	if (!me)
		return -ENOMEM;

	me->miscdev = nx_scaler_misc_device;

	platform_set_drvdata(pdev, me);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	me->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(me->base))
		return PTR_ERR(me->base);

	pr_debug("%s - base : 0x%p, phy addr : 0x%lX\n", __func__,
		me->base, (unsigned long)res->start);

	me->command_buffer_vir = NULL;
	me->command_buffer_phy = 0;

	ret = misc_register(&me->miscdev);
	if (ret) {
		pr_err("%s: failed to misc_register()\n",
		       __func__);
		kfree(me);

		return -1;
	}

	dev_set_drvdata(&pdev->dev, me);

	mutex_init(&me->mutex);

	atomic_set(&me->open_count, 0);
	atomic_set(&me->running, 0);

	init_waitqueue_head(&me->wq_end);

	if (me->miscdev.this_device) {
		struct device *p_dev =
			me->miscdev.this_device;
		p_dev->dma_mask = &p_dev->coherent_dma_mask;
		p_dev->coherent_dma_mask = DMA_BIT_MASK(32);
		me->command_buffer_vir = dma_alloc_coherent(
					    p_dev,
					    COMMAND_BUFFER_SIZE,
					    &me->command_buffer_phy,
					    GFP_KERNEL);
		if (!me->command_buffer_vir) {
			pr_err("%s: failed to alloc", __func__);
			pr_err("command buffer!!\n");

			goto misc_deregister;
		}
	}
	me->pdev = pdev;

	return 0;

misc_deregister:
	misc_deregister(&me->miscdev);
	kfree(me);

	return ret;
}

static int nx_scaler_remove(struct platform_device *pdev)
{
	struct nx_scaler *me = platform_get_drvdata(pdev);
	struct device *this_device = me->miscdev.this_device;

	if (this_device) {
		struct device *pdev = this_device;

		dma_free_coherent(pdev, COMMAND_BUFFER_SIZE,
				  me->command_buffer_vir,
				  me->command_buffer_phy);
		me->command_buffer_vir = NULL;
		me->command_buffer_phy = 0;
	}

	misc_deregister(&me->miscdev);
	kfree(me);

	return 0;
}

static const struct of_device_id nx_scaler_match[] = {
	{ .compatible = "nexell,scaler", },
	{},
};
MODULE_DEVICE_TABLE(of, nx_scaler_match);

static struct platform_driver nx_scaler_driver = {
	.probe		= nx_scaler_probe,
	.remove		= nx_scaler_remove,
	.driver		= {
		.name	= drv_name,
		.of_match_table	= nx_scaler_match,
	},
};

module_platform_driver(nx_scaler_driver);

MODULE_AUTHOR("jkchoi <jkchoi@nexell.co.kr>");
MODULE_DESCRIPTION("Scaler driver for Nexell");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:nx-scaler");
