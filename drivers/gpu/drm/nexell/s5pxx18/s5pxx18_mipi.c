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
#include <linux/of_irq.h>
#include <video/mipi_display.h>
#include <drm/nexell_drm.h>

#include "s5pxx18_drv.h"
#include "../nx_drm_gem.h"

#define	PLLPMS_1000MHZ		0x33E8
#define	BANDCTL_1000MHZ		0xF
#define PLLPMS_960MHZ		0x2280
#define BANDCTL_960MHZ		0xF
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

#define	MIPI_INDEX		0
#define	MIPI_EXC_PRE_VALUE	1
#define MIPI_DSI_IRQ_MASK	29	/* Empties SFR payload FIFO */

int nx_drm_mipi_register_notifier(struct device *dev,
			struct notifier_block *nb, unsigned int events)
{
	struct nx_drm_connector *nx_connector = dev_get_drvdata(dev);
	struct nx_mipi_dev *mipi = nx_connector->display->context;

	return blocking_notifier_chain_register(&mipi->notifier, nb);
}
EXPORT_SYMBOL_GPL(nx_drm_mipi_register_notifier);

int nx_drm_mipi_unregister_notifier(struct device *dev,
			struct notifier_block *nb)
{
	struct nx_drm_connector *nx_connector = dev_get_drvdata(dev);
	struct nx_mipi_dev *mipi = nx_connector->display->context;

	return blocking_notifier_chain_unregister(&mipi->notifier, nb);
}
EXPORT_SYMBOL_GPL(nx_drm_mipi_unregister_notifier);

static void mipi_fifo_tm_enable(struct nx_drm_display *display,
			unsigned int pipe)
{
	struct nx_mipi_dev *mipi = display->context;

	if (hrtimer_is_queued(&mipi->timer))
		return;

	hrtimer_start(&mipi->timer,
			ms_to_ktime(1000/display->vrefresh),
			HRTIMER_MODE_REL_PINNED);
}

static void mipi_fifo_tm_disable(struct nx_drm_display *display,
			unsigned int pipe)
{
	struct nx_mipi_dev *mipi = display->context;

	hrtimer_cancel(&mipi->timer);
}

static enum hrtimer_restart mipi_fifo_tm_fn(struct hrtimer *hrtimer)
{
	struct nx_mipi_dev *mipi =
		container_of(hrtimer, struct nx_mipi_dev, timer);
	struct nx_drm_display *display = mipi->display;

	hrtimer_start(&mipi->timer, ms_to_ktime(1000/display->vrefresh),
			HRTIMER_MODE_REL_PINNED);

	schedule_work(&mipi->work);

	return HRTIMER_NORESTART;
}

static void mipi_fifo_tm_work(struct work_struct *work)
{
	struct nx_mipi_dev *mipi =
			container_of(work, struct nx_mipi_dev, work);
	struct nx_drm_display *display = mipi->display;
	struct nx_drm_connector *connector = display->connector;
	struct drm_encoder *encoder;
	struct drm_crtc *crtc;
	struct drm_plane *primary;
	struct drm_pending_vblank_event *event;

	encoder = &connector->encoder->encoder;
	if (!encoder || !encoder->crtc)
		return;

	crtc = encoder->crtc;
	primary = crtc->primary;

	/*
	 * update dsi-fifo
	 */
	mutex_lock(&mipi->lock);
	if (primary && primary->state && primary->state->fb) {
		struct drm_framebuffer *fb = primary->state->fb;
		struct nx_mipi_dsi_nb_data nd = {
			.dsi = mipi->dsi,
			.fb = fb,
			.framebuffer = nx_drm_fb_gem(fb, 0)->cpu_addr,
			.fifo_size = DSI_TX_FIFO_SIZE,
		};
		unsigned int events = MIPI_DSI_FB_FLIP;

		blocking_notifier_call_chain(&mipi->notifier, events, &nd);
	}
	mutex_unlock(&mipi->lock);

	/*
	 * update crtc event
	 */
	drm_crtc_handle_vblank(crtc);

	spin_lock(&crtc->dev->event_lock);

	event = to_nx_crtc(crtc)->event;

	if (event) {
		drm_crtc_send_vblank_event(crtc, event);
		drm_crtc_vblank_put(crtc);
		to_nx_crtc(crtc)->event = NULL;
	}
	spin_unlock(&crtc->dev->event_lock);
}

static ssize_t mipi_fifo_update(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t n)
{
	struct nx_drm_connector *nx_connector = dev_get_drvdata(dev);
	struct nx_drm_display *display = nx_connector->display;
	struct nx_mipi_dev *mipi = display->context;

	mutex_lock(&mipi->lock);

	if (mipi->framebuffer && mipi->fb) {
		struct drm_framebuffer *fb = mipi->fb;
		struct nx_mipi_dsi_nb_data nd = {
			.dsi = mipi->dsi,
			.fb = fb,
			.framebuffer = mipi->framebuffer,
			.fifo_size = DSI_TX_FIFO_SIZE,
		};
		unsigned int events = MIPI_DSI_FB_FLUSH;

		blocking_notifier_call_chain(&mipi->notifier, events, &nd);
	}
	mutex_unlock(&mipi->lock);

	return n;
}

static DEVICE_ATTR(fifo, 0664, NULL, mipi_fifo_update);
static struct attribute *attrs[] = {
	&dev_attr_fifo.attr,
	NULL,
};

static struct attribute_group dsi_attr_group = {
	.attrs = (struct attribute **)attrs,
};

static irqreturn_t mipi_fifo_irq_handler(int irq, void *arg)
{
	struct nx_mipi_dev *mipi = arg;
	int index = MIPI_INDEX;

	mipi->cond = 1;
	wake_up_interruptible(&mipi->waitq);
	nx_mipi_dsi_clear_interrupt_pending(index, MIPI_DSI_IRQ_MASK);

	return IRQ_HANDLED;
}

static int mipi_fifo_irq_register(struct nx_drm_display *display)
{
	struct nx_mipi_dev *mipi = display->context;
	unsigned long flags = IRQF_SHARED;
	int ret;

	if (mipi->fifo_irq == INVALID_IRQ)
		return -EINVAL;

	init_waitqueue_head(&mipi->waitq);

	ret = request_irq(mipi->fifo_irq,
		mipi_fifo_irq_handler, flags, "mipi-dsi-fifo", mipi);
	if (ret < 0) {
		pr_err("Failed, mipi request IRQ#%u: %d\n",
			mipi->fifo_irq, ret);
		return -EINVAL;
	}
	mipi->irq_installed = true;

	pr_info("irq %d install for mipi fifo transfer\n", mipi->fifo_irq);
	return 0;
}

static int mipi_fifo_mode_setup(struct nx_drm_display *display)
{
	struct nx_mipi_dev *mipi = display->context;
	struct nx_drm_display_ops *ops = display->ops;
	int ret;

	ops->irq_enable = mipi_fifo_tm_enable;
	ops->irq_disable = mipi_fifo_tm_disable;

	/*
	 * disable display controller's output
	 */
	display->disable_output = true;

	hrtimer_init(&mipi->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mipi->timer.function = mipi_fifo_tm_fn;
	mipi->display = display;
	mipi->dev = mipi->dsi->host->dev;

	INIT_WORK(&mipi->work, mipi_fifo_tm_work);

	if (!mipi->sys_grouped) {
		ret = sysfs_create_group(&mipi->dev->kobj, &dsi_attr_group);
		if (ret)
			pr_err("%s: Failed, sysfs group for mipi-dsi\n",
				__func__);
		else
			mipi->sys_grouped = true;
	}

	if (!mipi->irq_installed)
		mipi_fifo_irq_register(display);

	return ret;
}

static int mipi_get_phy_pll(int bitrate, unsigned int *pllpms,
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

static int mipi_ops_open(struct nx_drm_display *display, int pipe)
{
	struct nx_mipi_dev *mipi = display->context;
	struct nx_control_dev *control = &mipi->control;
	struct nx_control_res *res = &control->res;
	int index = MIPI_INDEX;

	pr_debug("%s: pipe.%d\n", __func__, pipe);

	control->module = pipe;

	nx_mipi_set_base_address(index, res->sub_bases[0]);
	nx_mipi_dsi_clear_interrupt_pending_all(index);
	nx_mipi_dsi_set_interrupt_enable_all(index, 0);

	return 0;
}

static void mipi_ops_close(struct nx_drm_display *display, int pipe)
{
	struct nx_mipi_dev *mipi = display->context;

	pr_debug("%s: mode:%s\n", __func__,
		mipi->command_mode ? "command" : "video");

	if (!mipi->command_mode)
		return;

	hrtimer_cancel(&mipi->timer);
	cancel_work_sync(&mipi->work);

	if (mipi->sys_grouped)
		sysfs_remove_group(&mipi->dev->kobj, &dsi_attr_group);

	mipi->sys_grouped = false;

	if (mipi->irq_installed)
		free_irq(mipi->fifo_irq, mipi);

	mipi->irq_installed = false;
}

static int mipi_ops_prepare(struct nx_drm_display *display)
{
	struct nx_mipi_dev *mipi = display->context;
	int index = MIPI_INDEX;
	u32 esc_pre_value = MIPI_EXC_PRE_VALUE;
	int lpm = mipi->lpm_trans;
	bool command_mode = mipi->command_mode;
	int ret = 0;

#ifdef CONFIG_DRM_CHECK_PRE_INIT
	int clkid = NX_CLOCK_MIPI;

	if (nx_disp_top_clkgen_get_clock_divisor_enable(clkid)) {
		pr_debug("%s: already enabled ...\n", __func__);
		return 0;
	}
#endif

	ret = mipi_get_phy_pll(mipi->hs_bitrate,
			&mipi->hs_pllpms, &mipi->hs_bandctl);
	if (ret < 0)
		return ret;

	ret = mipi_get_phy_pll(mipi->lp_bitrate,
			&mipi->lp_pllpms, &mipi->lp_bandctl);
	if (ret < 0)
		return ret;

	pr_debug("%s: mipi lp:%dmhz:0x%x:0x%x, hs:%dmhz:0x%x:0x%x, %s trans\n",
	      __func__, mipi->lp_bitrate, mipi->lp_pllpms, mipi->lp_bandctl,
	      mipi->hs_bitrate, mipi->hs_pllpms, mipi->hs_bandctl,
	      lpm ? "low" : "high");

	if (lpm)
		nx_mipi_dsi_set_pll(index, 1, 0xFFFFFFFF,
			    mipi->lp_pllpms, mipi->lp_bandctl, 0, 0);
	else
		nx_mipi_dsi_set_pll(index, 1, 0xFFFFFFFF,
			    mipi->hs_pllpms, mipi->hs_bandctl, 0, 0);

	msleep(20);

#ifdef CONFIG_ARCH_S5P4418
	/*
	 * disable the escape clock generating prescaler
	 * before soft reset.
	 */
	nx_mipi_dsi_set_clock(index, 0, 0, 1, 1, 1, 0, 0, 0, 0, 10);
	mdelay(1);
#endif
	nx_mipi_dsi_software_reset(index);
	nx_mipi_dsi_set_clock(index, 0, 0, 1, 1, 1, 0, 0, 0, 1,
				      esc_pre_value);
	nx_mipi_dsi_set_phy(index, 0, 1, 1, 0, 0, 0, 0, 0);

	if (lpm)
		nx_mipi_dsi_set_escape_lp(index, nx_mipi_dsi_lpmode_lp,
					  nx_mipi_dsi_lpmode_lp);
	else
		nx_mipi_dsi_set_escape_lp(index, nx_mipi_dsi_lpmode_hs,
					  nx_mipi_dsi_lpmode_hs);
	msleep(20);

	if (command_mode) {
		nx_mipi_dsi_set_i80_clock(0x91180001);
		nx_mipi_dsi_set_i80_config(0x30047003);
		nx_mipi_dsi_fifo_reset();
		nx_mipi_dsi_set_escape_lp(index, nx_mipi_dsi_lpmode_hs,
				nx_mipi_dsi_lpmode_hs);
	}

	return 0;
}

static int mipi_ops_enable(struct nx_drm_display *display)
{
	struct nx_mipi_dev *mipi = display->context;
	struct mipi_dsi_device *dsi = mipi->dsi;
	struct nx_sync_info *sync = &mipi->control.sync;
	struct nx_control_info *ctrl = &mipi->control.ctrl;
	int module = mipi->control.module;
	int clkid = NX_CLOCK_MIPI;
	int index = MIPI_INDEX;
	u32 esc_pre_value = MIPI_EXC_PRE_VALUE;
	int HFP = sync->h_front_porch;
	int HBP = sync->h_back_porch;
	int HS = sync->h_sync_width;
	int VFP = sync->v_front_porch;
	int VBP = sync->v_back_porch;
	int VS = sync->v_sync_width;
	int width = sync->h_active_len;
	int height = sync->v_active_len;
	int en_prescaler = 1;
	int lpm = mipi->lpm_trans;

	int txhsclock = 1;
	bool command_mode = mipi->command_mode;
	int data_len = dsi->lanes - 1;
	enum nx_mipi_dsi_format dsi_format;

	if (command_mode)
		data_len = 0;

	/*
	 * disable the escape clock generating prescaler
	 * before soft reset.
	 */
#ifdef CONFIG_ARCH_S5P4418
	en_prescaler = 0;
#endif

#ifdef CONFIG_DRM_CHECK_PRE_INIT
	if (nx_disp_top_clkgen_get_clock_divisor_enable(clkid)) {
		pr_debug("%s: already enabled ...\n", __func__);
		return 0;
	}
#endif

	pr_debug("%s: mode:%s, lanes.%d, %dfps\n", __func__,
		command_mode ? "command" : "video",
		data_len + 1, display->vrefresh);

	if (lpm)
		nx_mipi_dsi_set_escape_lp(index,
			nx_mipi_dsi_lpmode_hs, nx_mipi_dsi_lpmode_hs);

	nx_mipi_dsi_set_pll(index, 1, 0xFFFFFFFF,
			    mipi->hs_pllpms, mipi->hs_bandctl, 0, 0);
	mdelay(1);

	nx_mipi_dsi_set_clock(index, 0, 0, 1, 1, 1, 0, 0, 0, en_prescaler, 10);
	mdelay(1);

	nx_mipi_dsi_software_reset(index);
	nx_mipi_dsi_set_clock(index, txhsclock, 0, 1,
			1, 1, 0, 0, 0, 1, esc_pre_value);

	switch (data_len) {
	case 0:	/* 1 lane */
		nx_mipi_dsi_set_phy(index, data_len, 1, 1, 0, 0, 0, 0, 0);
		break;
	case 1: /* 2 lane */
		nx_mipi_dsi_set_phy(index, data_len, 1, 1, 1, 0, 0, 0, 0);
		break;
	case 2: /* 3 lane */
		nx_mipi_dsi_set_phy(index, data_len, 1, 1, 1, 1, 0, 0, 0);
		break;
	case 3: /* 3 lane */
		nx_mipi_dsi_set_phy(index, data_len, 1, 1, 1, 1, 1, 0, 0);
		break;
	default:
		pr_err("%s: not support data lanes %d\n",
			__func__, data_len + 1);
		return -EINVAL;
	}

	switch (dsi->format) {
	case MIPI_DSI_FMT_RGB565:
		dsi_format = nx_mipi_dsi_format_rgb565;
		break;
	case MIPI_DSI_FMT_RGB666:
		dsi_format = nx_mipi_dsi_format_rgb666;
		break;
	case MIPI_DSI_FMT_RGB666_PACKED:
		dsi_format = nx_mipi_dsi_format_rgb666_packed;
		break;
	case MIPI_DSI_FMT_RGB888:
		dsi_format = nx_mipi_dsi_format_rgb888;
		break;
	default:
		pr_err("%s: not support format %d\n", __func__, dsi->format);
		return -EINVAL;
	}

	if (command_mode) {
		nx_mipi_dsi_set_config_command_mode(index, 1, 1, 1, dsi_format);
		return 0;
	}

	nx_mipi_dsi_set_config_video_mode(index, 1, 0, 1,
				nx_mipi_dsi_syncmode_event, 1, 1, 1,
				1, 1, 0, dsi_format,
				HFP, HBP, HS, VFP, VBP, VS, 0);

	nx_mipi_dsi_set_size(index, width, height);

	/* set mux */
	nx_disp_top_set_mipimux(1, module);

	/*  0 is spdif, 1 is mipi vclk */
	nx_disp_top_clkgen_set_clock_source(clkid, 1, ctrl->clk_src_lv0);
	nx_disp_top_clkgen_set_clock_divisor(clkid, 1,
				ctrl->clk_div_lv1 * ctrl->clk_div_lv0);

	/* SPDIF and MIPI */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, 1);

	/* START: CLKGEN, MIPI is started in setup function */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, true);
	nx_mipi_dsi_set_enable(index, true);

	return 0;
}

static int mipi_ops_unprepare(struct nx_drm_display *display)
{
	struct nx_mipi_dev *mipi = display->context;
	int index = MIPI_INDEX;

	pr_debug("%s\n", __func__);

	/* wait for end transfer */
	mutex_lock(&mipi->lock);
	nx_mipi_dsi_set_clock(index, 0, 0, 1, 1, 1, 0, 0, 0, 0, 10);
	mutex_unlock(&mipi->lock);
	return 0;
}

static int mipi_ops_disable(struct nx_drm_display *display)
{
	struct nx_mipi_dev *mipi = display->context;
	int clkid = NX_CLOCK_MIPI;
	int index = MIPI_INDEX;

	pr_debug("%s\n", __func__);

	/* wait for end transfer */
	mutex_lock(&mipi->lock);

	/* SPDIF and MIPI */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, 0);

	/* START: CLKGEN, MIPI is started in setup function */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, false);
	nx_mipi_dsi_set_enable(index, false);

	mutex_unlock(&mipi->lock);
	return 0;
}

static int mipi_ops_set_mode(struct nx_drm_display *display,
			struct drm_display_mode *mode, unsigned int flags)
{
	nx_display_mode_to_sync(mode, display);
	return 0;
}

static int mipi_ops_set_format(struct nx_drm_display *display,
			struct mipi_dsi_device *dsi)
{
	struct nx_mipi_dev *mipi = display->context;

	mipi->dsi = dsi;
	mipi->lpm_trans = 1;
	mipi->command_mode = dsi->mode_flags & MIPI_DSI_MODE_VIDEO ? 0 : 1;

	if (mipi->command_mode)
		mipi_fifo_mode_setup(display);

	pr_debug("%s: lanes.%d, %s, %s, %s\n", __func__,
		dsi->lanes, dsi->format == MIPI_DSI_FMT_RGB888 ? "RGB888" :
		dsi->format == MIPI_DSI_FMT_RGB666 ? "RGB666" :
		dsi->format == MIPI_DSI_FMT_RGB666_PACKED ? "RGB666-PACKED" :
		dsi->format == MIPI_DSI_FMT_RGB565 ? "RGB565" :	"UNKNOWN",
		dsi->mode_flags & MIPI_DSI_MODE_LPM ? "low" : "high",
		dsi->mode_flags & MIPI_DSI_MODE_VIDEO ? "video" : "command");

	return 0;
}

static void mipi_dump_msg(const struct mipi_dsi_msg *msg,
			bool dump)
{
	const char *txb = msg->tx_buf;
	const char *rxb = msg->rx_buf;
	int i = 0;

	if (!dump)
		return;

	pr_info("%s\n", __func__);
	pr_info("ch   :%d\n", msg->channel);
	pr_info("type :0x%x\n", msg->type);
	pr_info("flags:0x%x\n", msg->flags);

	for (i = 0; msg->tx_len > i; i++)
		pr_info("T[%2d]: 0x%02x\n", i, txb[i]);

	for (i = 0; msg->rx_len > i; i++)
		pr_info("R[%2d]: 0x%02x\n", i, rxb[i]);
}

static int mipi_transfer_done(struct nx_mipi_dev *mipi, bool wait_irq)
{
	if (!mipi->command_mode) {
		int index = 0, count = 100;
		u32 value;

		do {
			mdelay(1);
			value = nx_mipi_dsi_read_fifo_status(index);
			if (((1<<22) & value))
				break;
		} while (count-- > 0);

		if (count < 0)
			return -EINVAL;
	} else {
		if (wait_irq) {
			mipi->cond = 0;
			wait_event_interruptible_timeout(
						mipi->waitq, mipi->cond, HZ);
		} else {
			while ((nx_mipi_dsi_read_fifo_status(0) & (1<<20))
				!= (1<<20))
				;
		}
	}

	return 0;
}

static int mipi_transfer_tx(struct nx_mipi_dev *mipi,
			struct nx_mipi_xfer *xfer)
{
	const u8 *txb;
	int size, index = 0;
	u32 data;

	if (xfer->tx_len > DSI_TX_FIFO_SIZE)
		pr_warn("warn: tx %d size over fifo %d\n",
			(int)xfer->tx_len, DSI_TX_FIFO_SIZE);

	/* write payload */
	size = xfer->tx_len;
	txb = xfer->tx_buf;

	if (!mipi->command_mode) {
		while (size >= 4) {
			data = (txb[3] << 24) | (txb[2] << 16) |
				(txb[1] << 8) | (txb[0]);
			nx_mipi_dsi_write_payload(index, data);
			txb += 4, size -= 4;
			data = 0;
		}
	} else {
		const u32 *ptr = (const u32 *)txb;

		while (size >= 4) {
			nx_mipi_dsi_write_payload(index, *ptr);
			size -= 4;
			ptr++;
		}
		txb = (u8 *)ptr;
		data = 0;
	}

	switch (size) {
	case 3:
		data |= txb[2] << 16;
	case 2:
		data |= txb[1] << 8;
	case 1:
		data |= txb[0];
		nx_mipi_dsi_write_payload(index, data);
		break;
	case 0:
		break;	/* no payload */
	}

	/* write packet hdr */
	data = (xfer->data[1] << 16) |
		   (xfer->data[0] <<  8) | xfer->id;

	nx_mipi_dsi_write_pkheader(index, data);

	return 0;
}

static int mipi_transfer_rx(struct nx_mipi_dev *mipi, struct nx_mipi_xfer *xfer)
{
	int index = 0;
	u8 *rxb = xfer->rx_buf;
	int rx_len = 0;
	u16 size;
	u32 data;
	u32 count = 0;
	int err = -EINVAL;

	nx_mipi_dsi_clear_interrupt_pending(index, 18);

	while (1) {
		/* Completes receiving data. */
		if (nx_mipi_dsi_get_interrupt_pending(index, 18))
			break;

		mdelay(1);

		if (count > 500) {
			pr_err("%s: error recevice data\n", __func__);
			err = -EINVAL;
			goto clear_fifo;
		} else {
			count++;
		}
	}

	data = nx_mipi_dsi_read_fifo(index);

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
		data = nx_mipi_dsi_read_fifo(index);
		rxb[0] = (data >>  0) & 0xff;
		rxb[1] = (data >>  8) & 0xff;
		rxb[2] = (data >> 16) & 0xff;
		rxb[3] = (data >> 24) & 0xff;
		rxb += 4, size -= 4;
	}

	if (size) {
		data = nx_mipi_dsi_read_fifo(index);
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
		data = nx_mipi_dsi_read_fifo(index);
		if (data == DSI_RX_FIFO_EMPTY)
			break;
	} while (--size);

	return err;
}

#define	IS_SHORT(t)	(9 > (t & 0x0f))

static int mipi_ops_transfer(struct nx_drm_display *display,
			struct mipi_dsi_host *host,
			const struct mipi_dsi_msg *msg)
{
	struct nx_mipi_dev *mipi = display->context;
	struct nx_mipi_xfer xfer;
	bool wait_irq = false;
	int index = MIPI_INDEX;
	int err;

	mipi_dump_msg(msg, false);

	if (!msg->tx_len)
		return -EINVAL;

	/* set id */
	xfer.id = msg->type | (msg->channel << 6);

	/* short type msg */
	if (IS_SHORT(msg->type)) {
		const char *txb = msg->tx_buf;

		if (msg->tx_len > 2)
			return -EINVAL;

		xfer.tx_len  = 0;	/* no payload */
		xfer.data[0] = txb[0];
		xfer.data[1] = (msg->tx_len == 2) ? txb[1] : 0;
		xfer.tx_buf = NULL;
	} else {
		xfer.tx_len  = msg->tx_len;
		xfer.data[0] = msg->tx_len & 0xff;
		xfer.data[1] = msg->tx_len >> 8;
		xfer.tx_buf = msg->tx_buf;
	}

	xfer.rx_len = msg->rx_len;
	xfer.rx_buf = msg->rx_buf;
	xfer.flags = msg->flags;

	if (mipi->irq_installed && xfer.tx_len) {
		nx_mipi_dsi_set_interrupt_enable(index, MIPI_DSI_IRQ_MASK, 1);
		wait_irq = true;
	}

	err = mipi_transfer_tx(mipi, &xfer);
	if (xfer.rx_len)
		err = mipi_transfer_rx(mipi, &xfer);

	mipi_transfer_done(mipi, wait_irq);

	nx_mipi_dsi_set_interrupt_enable(index, MIPI_DSI_IRQ_MASK, 0);

	return err;
}

static int mipi_ops_resume(struct nx_drm_display *display)
{
	nx_display_resume_resource(display);
	return 0;
}

static struct nx_drm_mipi_dsi_ops dev_ops = {
	.set_format = mipi_ops_set_format,
	.transfer = mipi_ops_transfer,
};

static struct nx_drm_display_ops mipi_ops = {
	.open = mipi_ops_open,
	.close = mipi_ops_close,
	.prepare = mipi_ops_prepare,
	.unprepare = mipi_ops_unprepare,
	.enable = mipi_ops_enable,
	.disable = mipi_ops_disable,
	.set_mode = mipi_ops_set_mode,
	.resume = mipi_ops_resume,
	.dsi = &dev_ops,
};

void *nx_drm_display_mipi_get(struct device *dev,
			struct device_node *node,
			struct nx_drm_display *display)
{
	struct nx_mipi_dev *mipi;
	int set_fifo_irq = 0;

	mipi = kzalloc(sizeof(*mipi), GFP_KERNEL);
	if (!mipi)
		return NULL;

	of_property_read_u32(node, "lp_bitrate", &mipi->lp_bitrate);
	of_property_read_u32(node, "hs_bitrate", &mipi->hs_bitrate);
	of_property_read_u32(node, "enable-fifo-irq", &set_fifo_irq);
	display->context = mipi;
	display->ops = &mipi_ops;

	mutex_init(&mipi->lock);
	BLOCKING_INIT_NOTIFIER_HEAD(&mipi->notifier);
	mipi->fifo_irq = INVALID_IRQ;

	if (set_fifo_irq) {
		mipi->fifo_irq = of_irq_get(dev->of_node, 0);
		if (mipi->fifo_irq < 0)
			mipi->fifo_irq = INVALID_IRQ;
	}

	return &mipi->control;
}
