/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: junghyun, kim <jhkim@nexell.co.kr>
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
#include <linux/delay.h>
#include <linux/of.h>
#include <video/mipi_display.h>

#include "s5pxx18_dp_dev.h"

#define	PLLPMS_1000MHZ		0x33E8
#define	BANDCTL_1000MHZ		0xF
#define PLLPMS_960MHZ       0x2280
#define BANDCTL_960MHZ      0xF
#define	PLLPMS_900MHZ		0x2258
#define	BANDCTL_900MHZ		0xE
#define	PLLPMS_840MHZ		0x2230
#define	BANDCTL_840MHZ		0xD
#define	PLLPMS_750MHZ		0x43E8
#define	BANDCTL_750MHZ		0xC
#define	PLLPMS_660MHZ		0x21B8
#define	BANDCTL_660MHZ		0xB
#define	PLLPMS_600MHZ		0x2190
#define	BANDCTL_600MHZ		0xA
#define	PLLPMS_540MHZ		0x2168
#define	BANDCTL_540MHZ		0x9
#define	PLLPMS_512MHZ		0x03200
#define	BANDCTL_512MHZ		0x9
#define	PLLPMS_480MHZ		0x2281
#define	BANDCTL_480MHZ		0x8
#define	PLLPMS_420MHZ		0x2231
#define	BANDCTL_420MHZ		0x7
#define	PLLPMS_402MHZ		0x2219
#define	BANDCTL_402MHZ		0x7
#define	PLLPMS_330MHZ		0x21B9
#define	BANDCTL_330MHZ		0x6
#define	PLLPMS_300MHZ		0x2191
#define	BANDCTL_300MHZ		0x5
#define	PLLPMS_210MHZ		0x2232
#define	BANDCTL_210MHZ		0x4
#define	PLLPMS_180MHZ		0x21E2
#define	BANDCTL_180MHZ		0x3
#define	PLLPMS_150MHZ		0x2192
#define	BANDCTL_150MHZ		0x2
#define	PLLPMS_100MHZ		0x3323
#define	BANDCTL_100MHZ		0x1
#define	PLLPMS_80MHZ		0x3283
#define	BANDCTL_80MHZ		0x0

#define	MIPI_INDEX			0
#define	MIPI_EXC_PRE_VALUE	1

static int dp_mipi_phy_pll(int bitrate, unsigned int *pllpms,
			unsigned int *bandctl)
{
	unsigned int pms, ctl;

	switch (bitrate) {
	case 1000:
		pms = PLLPMS_1000MHZ;
		ctl = BANDCTL_1000MHZ;
		break;
	case 960:
		pms = PLLPMS_960MHZ;
		ctl = BANDCTL_960MHZ;
		break;
	case 900:
		pms = PLLPMS_900MHZ;
		ctl = BANDCTL_900MHZ;
		break;
	case 840:
		pms = PLLPMS_840MHZ;
		ctl = BANDCTL_840MHZ;
		break;
	case 750:
		pms = PLLPMS_750MHZ;
		ctl = BANDCTL_750MHZ;
		break;
	case 660:
		pms = PLLPMS_660MHZ;
		ctl = BANDCTL_660MHZ;
		break;
	case 600:
		pms = PLLPMS_600MHZ;
		ctl = BANDCTL_600MHZ;
		break;
	case 540:
		pms = PLLPMS_540MHZ;
		ctl = BANDCTL_540MHZ;
		break;
	case 512:
		pms = PLLPMS_512MHZ;
		ctl = BANDCTL_512MHZ;
		break;
	case 480:
		pms = PLLPMS_480MHZ;
		ctl = BANDCTL_480MHZ;
		break;
	case 420:
		pms = PLLPMS_420MHZ;
		ctl = BANDCTL_420MHZ;
		break;
	case 402:
		pms = PLLPMS_402MHZ;
		ctl = BANDCTL_402MHZ;
		break;
	case 330:
		pms = PLLPMS_330MHZ;
		ctl = BANDCTL_330MHZ;
		break;
	case 300:
		pms = PLLPMS_300MHZ;
		ctl = BANDCTL_300MHZ;
		break;
	case 210:
		pms = PLLPMS_210MHZ;
		ctl = BANDCTL_210MHZ;
		break;
	case 180:
		pms = PLLPMS_180MHZ;
		ctl = BANDCTL_180MHZ;
		break;
	case 150:
		pms = PLLPMS_150MHZ;
		ctl = BANDCTL_150MHZ;
		break;
	case 100:
		pms = PLLPMS_100MHZ;
		ctl = BANDCTL_100MHZ;
		break;
	case 80:
		pms = PLLPMS_80MHZ;
		ctl = BANDCTL_80MHZ;
		break;
	default:
		return -EINVAL;
	}

	*pllpms = pms;
	*bandctl = ctl;

	return 0;
}

static void nx_soc_dp_mipi_set_base(struct dp_control_dev *dpc,
			void __iomem *base)
{
	BUG_ON(!base);
	pr_debug("%s: dev mipi\n", __func__);

	nx_mipi_set_base_address(0, base);
}

static int nx_soc_dp_mipi_set_prepare(struct dp_control_dev *dpc,
			unsigned int flags)
{
	struct dp_mipi_dev *dev = dpc->dp_output;
	int index = MIPI_INDEX;
	u32 esc_pre_value = MIPI_EXC_PRE_VALUE;
	int ret = 0;

	ret = dp_mipi_phy_pll(dev->hs_bitrate,
			&dev->hs_pllpms, &dev->hs_bandctl);
	if (0 > ret)
		return ret;

	ret = dp_mipi_phy_pll(dev->lp_bitrate,
			&dev->lp_pllpms, &dev->lp_bandctl);
	if (0 > ret)
		return ret;

	pr_debug("%s: mipi lp:%dmhz:0x%x:0x%x, hs:%dmhz:0x%x:0x%x\n",
	      __func__, dev->lp_bitrate, dev->lp_pllpms, dev->lp_bandctl,
	      dev->hs_bitrate, dev->hs_pllpms, dev->hs_bandctl);

	if (flags) {
		nx_mipi_dsi_set_pll(index, 1, 0xFFFFFFFF,
				    dev->lp_pllpms, dev->lp_bandctl, 0, 0);
		msleep(20);

		nx_mipi_dsi_software_reset(index);
		nx_mipi_dsi_set_clock(index, 0, 0, 1, 1, 1, 0, 0, 0, 1,
				      esc_pre_value);
		nx_mipi_dsi_set_phy(index, 0, 1, 1, 0, 0, 0, 0, 0);

		nx_mipi_dsi_set_escape_lp(index, nx_mipi_dsi_lpmode_lp,
					  nx_mipi_dsi_lpmode_lp);
		msleep(20);
	}

	return 0;
}

static int nx_soc_dp_mipi_set_enable(struct dp_control_dev *dpc,
			unsigned int flags)
{
	struct dp_mipi_dev *dev = dpc->dp_output;
	struct dp_sync_info *sync = &dpc->sync;
	struct dp_ctrl_info *ctrl = &dpc->ctrl;
	int clkid = dp_clock_mipi;
	int index = MIPI_INDEX;
	u32 esc_pre_value = MIPI_EXC_PRE_VALUE;
	int module = dpc->module;
	int HFP = sync->h_front_porch;
	int HBP = sync->h_back_porch;
	int HS = sync->h_sync_width;
	int VFP = sync->v_front_porch;
	int VBP = sync->v_back_porch;
	int VS = sync->v_sync_width;
	int width = sync->h_active_len;
	int height = sync->v_active_len;

	pr_debug("%s: flags=0x%x\n", __func__, flags);

	if (flags)
		nx_mipi_dsi_set_escape_lp(index, nx_mipi_dsi_lpmode_hs,
						  nx_mipi_dsi_lpmode_hs);

	nx_mipi_dsi_set_pll(index, 1, 0xFFFFFFFF,
			    dev->hs_pllpms, dev->hs_bandctl, 0, 0);
	mdelay(1);

	nx_mipi_dsi_set_clock(index, 0, 0, 1, 1, 1, 0, 0, 0, 1, 10);
	mdelay(1);

	nx_mipi_dsi_software_reset(index);
	nx_mipi_dsi_set_clock(index, 1, 0, 1, 1, 1, 1, 1, 1, 1, esc_pre_value);
	nx_mipi_dsi_set_phy(index, 3, 1, 1, 1, 1, 1, 0, 0);
	nx_mipi_dsi_set_config_video_mode(index, 1, 0, 1,
					  nx_mipi_dsi_syncmode_event, 1, 1, 1,
					  1, 1, 0, nx_mipi_dsi_format_rgb888,
					  HFP, HBP, HS, VFP, VBP, VS, 0);

	nx_mipi_dsi_set_size(index, width, height);

	/* set mux */
	nx_disp_top_set_mipimux(1, module);

	/*  0 is spdif, 1 is mipi vclk */
	nx_disp_top_clkgen_set_clock_source(clkid, 1, ctrl->clk_src_lv0);
	nx_disp_top_clkgen_set_clock_divisor(clkid, 1,
					     ctrl->clk_div_lv1 *
					     ctrl->clk_div_lv0);

	/* SPDIF and MIPI */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, 1);

	/* START: CLKGEN, MIPI is started in setup function */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, true);
	nx_mipi_dsi_set_enable(0, true);

	return 0;
}

static int nx_soc_dp_mipi_set_unprepare(struct dp_control_dev *dpc)
{
	return 0;
}

static int nx_soc_dp_mipi_set_disable(struct dp_control_dev *dpc)
{
	int clkid = dp_clock_mipi;

	pr_debug("%s\n", __func__);

	/* SPDIF and MIPI */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, 0);

	/* START: CLKGEN, MIPI is started in setup function */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, false);
	nx_mipi_dsi_set_enable(0, false);

	return 0;
}

int nx_soc_dp_mipi_tx_transfer(struct dp_mipi_xfer *xfer)
{
	const u8 *txb;
	u16 size;
	u32 data;

	if (xfer->tx_len > DSI_TX_FIFO_SIZE)
		pr_warn("warn: tx %d size over fifo %d\n", (int)xfer->tx_len,
			DSI_TX_FIFO_SIZE);

	/*
	 * write payload
	 */
	size = xfer->tx_len;
	txb  = xfer->tx_buf;

	while (size >= 4) {
		data = (txb[3] << 24) | (txb[2] << 16) |
			(txb[1] << 8) | (txb[0]);
		nx_mipi_dsi_write_payload(0, data);
		txb += 4, size -= 4;
		data = 0;
	}

	switch (size) {
	case 3:
		data |= txb[2] << 16;
	case 2:
		data |= txb[1] << 8;
	case 1:
		data |= txb[0];
		nx_mipi_dsi_write_payload(0, data);
		break;
	case 0:
		break;	/* no payload */
	}

	/*
	 * write paket hdr
	 */
	data = (xfer->data[1] << 16) |
		   (xfer->data[0] <<  8) | xfer->id;

	nx_mipi_dsi_write_pkheader(0, data);

	return 0;
}

int nx_soc_dp_mipi_rx_transfer(struct dp_mipi_xfer *xfer)
{
	int module = 0;
	u8 *rxb = xfer->rx_buf;
	int rx_len = 0;
	u16 size;
	u32 data;
	u32 count = 0;
	int err = -EINVAL;

	while (1) {
		/* Completes receiving data. */
		if (nx_mipi_get_interrupt_pending(module, 50))
			break;

		mdelay(1);

		if (count > 500) {
			pr_err("%s: DSI Error : recevice data\n", __func__);
			err = -EINVAL;
			goto clear_fifo;
		} else {
			count++;
		}
	}

	data = nx_mipi_dsi_read_fifo(module);

	switch (data & 0x3f) {
	case MIPI_DSI_RX_GENERIC_SHORT_READ_RESPONSE_2BYTE:
	case MIPI_DSI_RX_DCS_SHORT_READ_RESPONSE_2BYTE:
		if (xfer->rx_len >= 2) {
			rxb[1] = data >> 16;
			rx_len++;
		}

		/* Fall through */
	case MIPI_DSI_RX_GENERIC_SHORT_READ_RESPONSE_1BYTE:
	case MIPI_DSI_RX_DCS_SHORT_READ_RESPONSE_1BYTE:
		rxb[0] = data >> 8;
		rx_len++;
		xfer->rx_len = rx_len;
		err = rx_len;
		goto clear_fifo;

	case MIPI_DSI_RX_ACKNOWLEDGE_AND_ERROR_REPORT:
		pr_debug("DSI Error Report: 0x%04x\n",
			(data >> 8) & 0xffff);
		err = rx_len;
		goto clear_fifo;
	}

	size = (data >> 8) & 0xffff;

	if (size > xfer->rx_len)
		size = xfer->rx_len;
	else if (size < xfer->rx_len)
		xfer->rx_len = size;

	size = xfer->rx_len - rx_len;
	rx_len += size;


	/* Receive payload */
	while (size >= 4) {
		data = nx_mipi_dsi_read_fifo(module);
		rxb[0] = (data >>  0) & 0xff;
		rxb[1] = (data >>  8) & 0xff;
		rxb[2] = (data >> 16) & 0xff;
		rxb[3] = (data >> 24) & 0xff;
		rxb += 4, size -= 4;
	}

	if (size) {
		data = nx_mipi_dsi_read_fifo(module);
		switch (size) {
		case 3:
			rxb[2] = (data >> 16) & 0xff;
		case 2:
			rxb[1] = (data >> 8) & 0xff;
		case 1:
			rxb[0] = data & 0xff;
		}
	}

	if (rx_len == xfer->rx_len)
		err = rx_len;

clear_fifo:
	size = DSI_RX_FIFO_SIZE / 4;
	do {
		data = nx_mipi_dsi_read_fifo(module);
		if (data == DSI_RX_FIFO_EMPTY)
			break;
	} while (--size);

	return err;
}

static struct dp_control_ops mipi_dp_ops = {
	.set_base = nx_soc_dp_mipi_set_base,
	.prepare = nx_soc_dp_mipi_set_prepare,
	.unprepare = nx_soc_dp_mipi_set_unprepare,
	.enable = nx_soc_dp_mipi_set_enable,
	.disable = nx_soc_dp_mipi_set_disable,
};

#define parse_read_prop(n, s, v)	{ \
	u32 _v;	\
	if (!of_property_read_u32(n, s, &_v))	\
		v = _v;	\
	}

int nx_soc_dp_mipi_register(struct device *dev,
			struct device_node *np, struct dp_control_dev *dpc)
{
	struct dp_mipi_dev *out;
	u32 lp_rate = 0, hs_rate = 0;

	out = devm_kzalloc(dev, sizeof(*out), GFP_KERNEL);
	if (IS_ERR(out))
		return -ENOMEM;

	parse_read_prop(np, "lp_bitrate", lp_rate);
	parse_read_prop(np, "hs_bitrate", hs_rate);

	out->lp_bitrate = lp_rate;
	out->hs_bitrate = hs_rate;

	dpc->panel_type = dp_panel_type_mipi;
	dpc->dp_output = out;
	dpc->ops = &mipi_dp_ops;

	return 0;
}
