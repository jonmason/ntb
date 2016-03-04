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
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/reset.h>

#include "nx_display.h"

/*
#define pr_debug(msg...)		printk(msg)
*/

#define	DEV_NAME_DISP	"dp-controller"

/* 12345'678'[8] -> 12345 [5], 123456'78'[8] -> 123456[6] */
static inline u_short R8G8B8toR5G6B5(unsigned int RGB)
{
	u_char R = (u_char) ((RGB >> 16) & 0xff);
	u_char G = (u_char) ((RGB >> 8) & 0xff);
	u_char B = (u_char) ((RGB >> 0) & 0xff);
	u_short R5G6B5 =
	    ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | ((B & 0xF8) >> 3);
	pr_debug(" RGB888:0x%08x -> RGB565:0x%08x\n", RGB, R5G6B5);
	return R5G6B5;
}

/* 12345 [5] -> 12345'123'[8], 123456[6] -> 123456'12'[8] */
static inline unsigned int R5G6B5toR8G8B8(u_short RGB)
{
	u_char R5 = (RGB >> 11) & 0x1f;
	u_char G6 = (RGB >> 5) & 0x3f;
	u_char B5 = (RGB >> 0) & 0x1f;
	u_char R8 = ((R5 << 3) & 0xf8) | ((R5 >> 2) & 0x7);
	u_char G8 = ((G6 << 2) & 0xfc) | ((G6 >> 4) & 0x3);
	u_char B8 = ((B5 << 3) & 0xf8) | ((B5 >> 2) & 0x7);
	unsigned int R8B8G8 = (R8 << 16) | (G8 << 8) | (B8);

	pr_debug(" RGB565:0x%08x -> RGB888:0x%08x\n", RGB, R8B8G8);
	return R8B8G8;
}

/* 123'45678'[8] -> 123[3], 12'345678'[8] -> 12 [2] */
static inline u_char R8G8B8toR3G3B2(unsigned int RGB)
{
	u_char R = (u_char) ((RGB >> 16) & 0xff);
	u_char G = (u_char) ((RGB >> 8) & 0xff);
	u_char B = (u_char) ((RGB >> 0) & 0xff);
	u_char R3G3B2 = ((R & 0xE0) | ((G & 0xE0) >> 3) | ((B & 0xC0) >> 6));

	pr_debug(" RGB888:0x%08x -> RGB332:0x%08x\n", RGB, R3G3B2);
	return R3G3B2;
}

/* 123[3] -> 123'123'12' [8], 12 [2] -> 12'12'12'12'[8] */
static inline unsigned int R3G3B2toR8G8B8(u_char RGB)
{
	u_char R3 = (RGB >> 5) & 0x7;
	u_char G3 = (RGB >> 2) & 0x7;
	u_char B2 = (RGB >> 0) & 0x3;
	u_char R8 = ((R3 << 5) | (R3 << 2) | (R3 >> 1));
	u_char G8 = ((G3 << 5) | (G3 << 2) | (G3 >> 1));
	u_char B8 = ((B2 << 6) | (B2 << 4) | (B2 << 2) | B2);
	unsigned int R8B8G8 = (R8 << 16) | (G8 << 8) | (B8);

	pr_debug(" RGB332:0x%08x -> RGB888:0x%08x\n", RGB, R8B8G8);
	return R8B8G8;
}

/* For math func */
#define PI						(3.141592)
#define	DEGREE_RADIAN(dg)		(dg*PI/180)

static inline double ksin(double radian)
{
	int i = 1;
	double d = 0.0;
	double s = radian;

	while (1) {
		d += s;
		s *= -radian * radian / (2 * i * (2 * i + 1));
		i++;
		if (s < 0.000000000000001 && s > -0.000000000000001)
			break;
	}
	return d;
}

static inline double kcos(double radian)
{
	int i = 1;
	double d = 0.0;
	double s = 1.0;

	while (1) {
		d += s;
		s *= -radian * radian / (2 * i * (2 * i - 1));
		i++;
		if (s < 0.000000000000001 && s > -0.000000000000001)
			break;
	}
	return d;
}

/*
 */
#define DEF_MLC_INTERLACE		0
#define DEF_OUT_FORMAT			DPC_FORMAT_RGB888
#define DEF_OUT_INVERT_FIELD	0
#define DEF_OUT_SWAPRB			0
#define DEF_OUT_YCORDER			DPC_YCORDER_CbYCrY
#define DEF_PADCLKSEL			DPC_PADCLKSEL_VCLK
#define DEF_CLKGEN0_DELAY		0
#define DEF_CLKGEN0_INVERT		0
#define DEF_CLKGEN1_DELAY		0
#define DEF_CLKGEN1_INVERT		0
#define DEF_CLKSEL1_SELECT		0

struct disp_control_info {
	int module;		/* 0: primary, 1: secondary */
	int irqno;
	int active_notify;
	int cond_notify;
	unsigned int condition;
	struct kobject *kobj;
	struct work_struct work;
	unsigned int wait_time;
	wait_queue_head_t wait_queue;
	ktime_t time_stamp;
	long fps;		/* ms */
	unsigned int status;
	struct list_head link;
	struct lcd_operation *lcd_ops;	/* LCD and Backlight */
	struct disp_multily_dev multilayer;
	struct disp_process_dev *proc_dev;
	spinlock_t lock_callback;
	struct list_head callback_list;
};

static LIST_HEAD(disp_resconv_link);
static struct disp_control_info *display_info[NUMBER_OF_DPC_MODULE];

static struct nx_mlc_register_set save_multily[NUMBER_OF_DPC_MODULE];
static struct nx_dpc_register_set save_syncgen[NUMBER_OF_DPC_MODULE];

/*
 * display device array
 */
#define	LIST_INIT(x)	LIST_HEAD_INIT(device_dev[x].list)
#define	LOCK_INIT(x)	(__SPIN_LOCK_UNLOCKED(device_dev[x].lock))

static struct disp_process_dev device_dev[] = {
	[0] = {.dev_id = DISP_DEVICE_RESCONV,
		   .name = "RESCONV",
		   .list = LIST_INIT(0),
		   .lock = LOCK_INIT(0)
		   },
	[1] = {.dev_id = DISP_DEVICE_LCD,
		   .name = "LCD",
		   .list = LIST_INIT(1),
		   .lock = LOCK_INIT(1)
		   },
	[2] = {.dev_id = DISP_DEVICE_HDMI,
		   .name = "HDMI",
		   .list = LIST_INIT(2),
		   .lock = LOCK_INIT(2)
		   },
	[3] = {.dev_id = DISP_DEVICE_MIPI,
		   .name = "MiPi",
		   .list = LIST_INIT(3),
		   .lock = LOCK_INIT(3)
		   },
	[4] = {.dev_id = DISP_DEVICE_LVDS,
		   .name = "LVDS",
		   .list = LIST_INIT(4),
		   .lock = LOCK_INIT(4)
		   },
	[5] = {.dev_id = DISP_DEVICE_TVOUT,
		   .name = "TVOUT",
		   .list = LIST_INIT(5),
		   .lock = LOCK_INIT(5)
		   },
	[6] = {.dev_id = DISP_DEVICE_SYNCGEN0,
		   .name = "SYNCGEN0",
		   .list = LIST_INIT(6),
		   .lock = LOCK_INIT(6)
		   },
	[7] = {.dev_id = DISP_DEVICE_SYNCGEN1,
		   .name = "SYNCGEN1",
		   .list = LIST_INIT(7),
		   .lock = LOCK_INIT(7)
		   },
};

#define DEVICE_SIZE	 ARRAY_SIZE(device_dev)

static struct kobject *kobj_syncgen;

static inline void *get_display_ptr(enum disp_dev_type device)
{
	return (void *)(&device_dev[device]);
}

const char *dev_to_str(enum disp_dev_type device)
{
	struct disp_process_dev *pdev = get_display_ptr(device);

	return pdev->name;
}

#define	get_display_ops(d)	(device_dev[d].disp_ops)
#define	set_display_ops(d, op) { \
	if (op) { \
		device_dev[d].disp_ops  = op; op->dev = &device_dev[d]; \
	} }

static inline void *get_module_to_info(int module)
{
	struct disp_control_info *info = display_info[module];

	RET_ASSERT_VAL(module == 0 || module == 1, NULL);
	RET_ASSERT_VAL(info, NULL);
	return info;
}

static inline void *get_device_to_info(struct disp_process_dev *pdev)
{
	return (void *)pdev->dev_info;
}

#define	DISP_CONTROL_INFO(m, in)		\
	struct disp_control_info *in = get_module_to_info(m)

#define	DISP_MULTILY_DEV(m, d) \
	DISP_CONTROL_INFO(m, info); \
	struct disp_multily_dev *d = &info->multilayer

#define	DISP_MULTILY_RGB(m, r, l) \
	DISP_CONTROL_INFO(m, info); \
	struct disp_multily_dev *pmly = &info->multilayer;	\
	struct mlc_layer_info *r = &pmly->layer[l]

#define	DISP_MULTILY_VID(m, v) \
	DISP_CONTROL_INFO(m, info); \
	struct disp_multily_dev *pmly = &info->multilayer; \
	struct mlc_layer_info *v = &pmly->layer[LAYER_VIDEO_IDX]

#define	DISP_WAIT_POLL_VSYNC(m, w) do { \
		mdelay(1); \
	} while (w-- > 0 && !nx_dpc_get_interrupt_pending_all(m))

static inline void set_display_kobj(struct kobject *kobj)
{
	kobj_syncgen = kobj;
}

static inline struct kobject *get_display_kobj(enum disp_dev_type device)
{
	return kobj_syncgen;
}

static inline void video_alpha_blend(int module, int depth, bool on)
{
	int enb = on ? 1 : 0;

	if (depth <= 0)
		depth = 0;
	if (depth >= 15)
		depth = 15;

	nx_mlc_set_alpha_blending(module, MLC_LAYER_VIDEO, enb, depth);
	nx_mlc_set_dirty_flag(module, MLC_LAYER_VIDEO);
}

static inline void video_lu_color(int module, int bright, int contra)
{
	if (contra >= 7)
		contra = 7;
	if (contra <= 0)
		contra = 0;
	if (bright >= 127)
		bright = 127;
	if (bright <= -128)
		bright = -128;

	nx_mlc_set_video_layer_luma_enhance(module, (unsigned int)contra,
					    (int32_t) bright);
	nx_mlc_set_dirty_flag(module, MLC_LAYER_VIDEO);
}

static inline void video_cbr_color(int module, int cba,
				int cbb, int cra, int crb)
{
	int i;

	if (cba > 127)
		cba = 127;
	if (cba <= -128)
		cba = -128;
	if (cbb > 127)
		cbb = 127;
	if (cbb <= -128)
		cbb = -128;
	if (cra > 127)
		cra = 127;
	if (cra <= -128)
		cra = -128;
	if (crb > 127)
		crb = 127;
	if (crb <= -128)
		crb = -128;

	for (i = 1; 5 > i; i++)
		nx_mlc_set_video_layer_chroma_enhance(module,
				i, cba, cbb, cra, crb);

	nx_mlc_set_dirty_flag(module, MLC_LAYER_VIDEO);
}

#define	SET_PARAM(s, d, member)	do { \
	if (s->member && s->member != d->member)	\
		d->member = s->member; \
	} while (0)

static inline void set_syncgen_param(struct disp_sync_par *src,
				struct disp_sync_par *dst)
{
	if (!src || !dst)
		return;

	SET_PARAM(src, dst, interlace);
	SET_PARAM(src, dst, out_format);
	SET_PARAM(src, dst, invert_field);
	SET_PARAM(src, dst, swap_RB);
	SET_PARAM(src, dst, yc_order);
	SET_PARAM(src, dst, delay_mask);
	SET_PARAM(src, dst, d_rgb_pvd);
	SET_PARAM(src, dst, d_hsync_cp1);
	SET_PARAM(src, dst, d_vsync_fram);
	SET_PARAM(src, dst, d_de_cp2);
	SET_PARAM(src, dst, vs_start_offset);
	SET_PARAM(src, dst, vs_end_offset);
	SET_PARAM(src, dst, ev_start_offset);
	SET_PARAM(src, dst, ev_end_offset);
	SET_PARAM(src, dst, vclk_select);
	SET_PARAM(src, dst, clk_inv_lv0);
	SET_PARAM(src, dst, clk_delay_lv0);
	SET_PARAM(src, dst, clk_inv_lv1);
	SET_PARAM(src, dst, clk_delay_lv1);
	SET_PARAM(src, dst, clk_sel_div1);
}

static int display_framerate_jiffies(int module,
				struct disp_vsync_info *psync)
{
	unsigned long hpix, vpix, pixclk;
	long rate_jiffies;
	long fps, rate;
	struct clk *clk = NULL;
	char name[32] = { 0, };

	sprintf(name, "pll%d", psync->clk_src_lv0);
	clk = clk_get(NULL, name);

	/* clock divider align */
	if (1 != psync->clk_div_lv0 && (psync->clk_div_lv0 & 0x1))
		psync->clk_div_lv0 += 1;

	if (1 != psync->clk_div_lv1 && (psync->clk_div_lv1 & 0x1))
		psync->clk_div_lv1 += 1;

	/* get pixel clock */
	psync->pixel_clock_hz =
	    ((clk_get_rate(clk) / psync->clk_div_lv0) / psync->clk_div_lv1);

	hpix = psync->h_active_len + psync->h_front_porch +
	    psync->h_back_porch + psync->h_sync_width;
	vpix = psync->v_active_len + psync->v_front_porch +
	    psync->v_back_porch + psync->v_sync_width;
	pixclk = psync->pixel_clock_hz;

	pr_debug(" pixel clock   =%ld\n", pixclk);
	pr_debug(" pixel horizon =%4ld (x=%4d, hfp=%3d, hbp=%3d, hsw=%3d)\n",
		 hpix, psync->h_active_len, psync->h_front_porch,
		 psync->h_back_porch, psync->h_sync_width);
	pr_debug(" pixel vertical=%4ld (y=%4d, vfp=%3d, vbp=%3d, vsw=%3d)\n",
		 vpix, psync->v_active_len, psync->v_front_porch,
		 psync->v_back_porch, psync->v_sync_width);

	fps = pixclk ? ((pixclk / hpix) / vpix) : 60;
	fps = fps ? fps : 60;
	rate = 1000 / fps;
	rate_jiffies = msecs_to_jiffies(rate) + 2;	/* +1 jiffies */

	pr_info("Display.%d fps=%2ld (%ldms), wait=%2ld, Pclk=%ldhz\n",
	     module, fps, rate, rate_jiffies, pixclk);

	return rate_jiffies;
}

static int disp_multily_enable(struct disp_control_info *info, int enable)
{
	struct disp_process_dev *pdev = info->proc_dev;
	struct disp_vsync_info *psync = &pdev->vsync;
	struct disp_multily_dev *pmly = &info->multilayer;
	int module = info->module, i = 0;

	if (enable) {
		struct mlc_layer_info *layer = pmly->layer;
		int xresol, yresol;
		int interlace, lock_size;
		int i = 0;

		pmly->x_resol = psync->h_active_len;
		pmly->y_resol = psync->v_active_len;
		pmly->interlace = psync->interlace;

		xresol = pmly->x_resol;
		yresol = pmly->y_resol;
		interlace = pmly->interlace;
		lock_size = pmly->mem_lock_len;

		nx_mlc_set_field_enable(module, interlace);
		nx_mlc_set_screen_size(module, xresol, yresol);
		nx_mlc_set_rgblayer_gama_table_power_mode(module, 0, 0, 0);
		nx_mlc_set_rgblayer_gama_table_sleep_mode(module, 1, 1, 1);
		nx_mlc_set_rgblayer_gamma_enable(module, 0);
		nx_mlc_set_dither_enable_when_using_gamma(module, 0);
		nx_mlc_set_gamma_priority(module, 0);
		nx_mlc_set_top_power_mode(module, 1);
		nx_mlc_set_top_sleep_mode(module, 0);
		nx_mlc_set_mlc_enable(module, 1);

		/* restore layer enable status */
		for (i = 0; MULTI_LAYER_NUM > i; i++, layer++) {
			nx_mlc_set_lock_size(module, i, lock_size);
			nx_mlc_set_layer_enable(module, i,
						layer->enable ? 1 : 0);
			nx_mlc_set_dirty_flag(module, i);
			pr_debug("%s: %s, %s\n", __func__, layer->name,
				 layer->enable ? "ON" : "OFF");
		}
		nx_mlc_set_top_dirty_flag(module);
		pmly->enable = enable;

	} else {
		for (i = 0; MULTI_LAYER_NUM > i; i++) {
			nx_mlc_set_layer_enable(module, i, 0);
			nx_mlc_set_dirty_flag(module, i);
		}
		nx_mlc_set_top_power_mode(module, 0);
		nx_mlc_set_top_sleep_mode(module, 1);
		nx_mlc_set_mlc_enable(module, 0);
		nx_mlc_set_top_dirty_flag(module);
		pmly->enable = enable;
	}
	return 0;
}

static int disp_syncgen_waitsync(int module, int layer, int waitvsync)
{
	DISP_CONTROL_INFO(module, info);
	int ret = -1;

	if (waitvsync && nx_dpc_get_dpc_enable(module)) {
		info->condition = 0;
		/*
		 * wait_event_timeout may be loss sync evnet, so use
		 * wait_event_interruptible_timeout
		 */
		ret = wait_event_interruptible_timeout(info->wait_queue,
						       info->condition,
						       info->wait_time);
		if (0 == info->condition)
			pr_err("fail, wait vsync %d, time:%s, condition:%d\n",
			       module, !ret ? "out" : "remain",
			       info->condition);
	}

	return ret;		/* 0: success, -1: fail */
}

static inline void disp_syncgen_irqenable(int module, int enable)
{
	pr_debug("%s: display.%d, %s\n", __func__, module,
		 enable ? "ON" : "OFF");
	/* enable source irq */
	nx_dpc_clear_interrupt_pending_all(module);
	nx_dpc_set_interrupt_enable_all(module, enable ? 1 : 0);
}

static void disp_syncgen_irq_work(struct work_struct *work)
{
	struct disp_control_info *info;
	struct disp_process_dev *pdev;
	int module;
	char path[32] = { 0, };

	info = container_of(work, struct disp_control_info, work);
	RET_ASSERT(info);

	if (!info->cond_notify)
		return;

	pdev = info->proc_dev;
	module = info->module;
	sprintf(path, "vsync.%d", module);

	/* check select with exceptfds */
	sysfs_notify(info->kobj, NULL, path);

	info->cond_notify = 0;
}

static irqreturn_t disp_syncgen_irqhandler(int irq, void *desc)
{
	struct disp_control_info *info = desc;
	struct disp_irq_callback *callback;
	int module = info->module;
	ktime_t ts;

	info->condition = 1;
	wake_up_interruptible(&info->wait_queue);

	if (info->active_notify) {
		info->cond_notify = 1;
		ts = ktime_get();
		info->fps = ktime_to_us(ts) - ktime_to_us(info->time_stamp);
		info->time_stamp = ts;

		schedule_work(&info->work);
	}
	nx_dpc_clear_interrupt_pending_all(module);

	spin_lock(&info->lock_callback);
	if (!list_empty(&info->callback_list)) {
		list_for_each_entry(callback, &info->callback_list, list)
			callback->handler(callback->data);
	}
	spin_unlock(&info->lock_callback);

#if (0)
	{
		static long ts[2] = { 0, };
		long new = ktime_to_ms(ktime_get());
		/* if (ts) printk("[%dms]\n", new-ts); */
		if ((new - ts[module]) > 18) {
			pr_info("[DPC %d]INTR OVERRUN %ld ms\n", module,
			       new - ts[module]);
		}
		ts[module] = new;
	}
#endif

	return IRQ_HANDLED;
}

static void disp_syncgen_initialize(struct disp_process_dev *pdev)
{
	void *base;
	int i = 0;

	pr_info("Disply Reset IF : %p,%p\n", pdev->rstc, pdev->rstc2);

	/* reset */
	if (pdev->rstc)
		reset_control_reset(pdev->rstc);

	if (pdev->rstc2)
		reset_control_reset(pdev->rstc2);

	for (; DISPLAY_SYNCGEN_NUM > i; i++) {
		/* BASE : MLC, PCLK/BCLK */
		base = ioremap(nx_mlc_get_physical_address(i), PAGE_SIZE);
		nx_mlc_set_base_address(i, base);
		nx_mlc_set_clock_pclk_mode(i, nx_pclkmode_always);
		nx_mlc_set_clock_bclk_mode(i, nx_bclkmode_always);

		/* BASE : DPC, PCLK */
		base = ioremap(nx_dpc_get_physical_address(i), PAGE_SIZE);
		nx_dpc_set_base_address(i, base);
		nx_dpc_set_clock_pclk_mode(i, nx_pclkmode_always);
	}
}

static int disp_syncgen_set_vsync(struct disp_process_dev *pdev,
				  struct disp_vsync_info *psync)
{
	struct disp_control_info *info = get_device_to_info(pdev);

	RET_ASSERT_VAL(psync, -EINVAL);

	pr_debug("%s\n", __func__);

	memcpy(&pdev->vsync, psync, sizeof(struct disp_vsync_info));

	pdev->status |= PROC_STATUS_READY;
	info->wait_time = display_framerate_jiffies(info->module, &pdev->vsync);

	return 0;
}

static int disp_syncgen_get_vsync(struct disp_process_dev *pdev,
				  struct disp_vsync_info *psync)
{
	struct disp_control_info *info = get_device_to_info(pdev);
	struct list_head *head = &info->link;

	if (list_empty(head)) {
		pr_info("display:%9s not connected display out ...\n",
		       dev_to_str(pdev->dev_id));
		return -1;
	}

	if (psync)
		memcpy(psync, &pdev->vsync, sizeof(struct disp_vsync_info));

	return 0;
}

static int disp_syncgen_prepare(struct disp_control_info *info)
{
	struct disp_process_dev *pdev = info->proc_dev;
	struct disp_sync_par *pspar = &pdev->sync_par;
	struct disp_vsync_info *psync = &pdev->vsync;
	int module = info->module;
	unsigned int out_format = pspar->out_format;
	unsigned int delay_mask = pspar->delay_mask;
	int rgb_pvd = 0, hsync_cp1 = 7, vsync_fram = 7, de_cp2 = 7;
	int v_vso = 1, v_veo = 1, e_vso = 1, e_veo = 1;

	int clk_src_lv0 = psync->clk_src_lv0;
	int clk_div_lv0 = psync->clk_div_lv0;
	int clk_src_lv1 = psync->clk_src_lv1;
	int clk_div_lv1 = psync->clk_div_lv1;
	int clk_dly_lv0 = pspar->clk_delay_lv0;
	int clk_dly_lv1 = pspar->clk_delay_lv1;

	int invert_field = pspar->invert_field;
	int swap_RB = pspar->swap_RB;
	unsigned int yc_order = pspar->yc_order;
	int interlace = pspar->interlace;
	int vclk_select = pspar->vclk_select;
	int vclk_invert = pspar->clk_inv_lv0 | pspar->clk_inv_lv1;
	int emb_sync = (out_format == DPC_FORMAT_CCIR656 ? 1 : 0);

	enum nx_dpc_dither r_dither, g_dither, b_dither;
	int rgb_mode = 0;

	bool LCD_out = pdev->dev_out == DISP_DEVICE_LCD ? true : false;

	/* set delay mask */
	if (delay_mask & DISP_SYNCGEN_DELAY_RGB_PVD)
		rgb_pvd = pspar->d_rgb_pvd;
	if (delay_mask & DISP_SYNCGEN_DELAY_HSYNC_CP1)
		hsync_cp1 = pspar->d_hsync_cp1;
	if (delay_mask & DISP_SYNCGEN_DELAY_VSYNC_FRAM)
		vsync_fram = pspar->d_vsync_fram;
	if (delay_mask & DISP_SYNCGEN_DELAY_DE_CP)
		de_cp2 = pspar->d_de_cp2;

	if (pspar->vs_start_offset != 0 ||
	    pspar->vs_end_offset != 0 ||
	    pspar->ev_start_offset != 0 || pspar->ev_end_offset != 0) {
		v_vso = pspar->vs_start_offset;
		v_veo = pspar->vs_end_offset;
		e_vso = pspar->ev_start_offset;
		e_veo = pspar->ev_end_offset;
	}

	if ((nx_dpc_format_rgb555 == out_format) ||
	    (nx_dpc_format_mrgb555a == out_format) ||
	    (nx_dpc_format_mrgb555b == out_format)) {
		r_dither = g_dither = b_dither = nx_dpc_dither_5bit;
		rgb_mode = 1;
	} else if ((nx_dpc_format_rgb565 == out_format) ||
		   (nx_dpc_format_mrgb565 == out_format)) {
		r_dither = b_dither = nx_dpc_dither_5bit;
		g_dither = nx_dpc_dither_6bit, rgb_mode = 1;
	} else if ((nx_dpc_format_rgb666 == out_format) ||
		   (nx_dpc_format_mrgb666 == out_format)) {
		r_dither = g_dither = b_dither = nx_dpc_dither_6bit;
		rgb_mode = 1;
	} else {
		r_dither = g_dither = b_dither = nx_dpc_dither_bypass;
		rgb_mode = 1;
	}

	/* CLKGEN0/1 */
	nx_dpc_set_clock_source(module, 0, clk_src_lv0);
	nx_dpc_set_clock_divisor(module, 0, clk_div_lv0);
	nx_dpc_set_clock_out_delay(module, 0, clk_dly_lv0);
	nx_dpc_set_clock_source(module, 1, clk_src_lv1);
	nx_dpc_set_clock_divisor(module, 1, clk_div_lv1);
	nx_dpc_set_clock_out_delay(module, 1, clk_dly_lv1);

	/* LCD out */
	if (LCD_out) {
		nx_dpc_set_mode(module, out_format, interlace, invert_field,
				rgb_mode, swap_RB, yc_order, emb_sync, emb_sync,
				vclk_select, vclk_invert, 0);
		nx_dpc_set_hsync(module, psync->h_active_len,
				 psync->h_sync_width, psync->h_front_porch,
				 psync->h_back_porch, psync->h_sync_invert);
		nx_dpc_set_vsync(module, psync->v_active_len,
				 psync->v_sync_width, psync->v_front_porch,
				 psync->v_back_porch, psync->v_sync_invert,
				 psync->v_active_len, psync->v_sync_width,
				 psync->v_front_porch, psync->v_back_porch);
		nx_dpc_set_vsync_offset(module, v_vso, v_veo, e_vso, e_veo);
		nx_dpc_set_delay(module, rgb_pvd, hsync_cp1, vsync_fram,
				 de_cp2);
		nx_dpc_set_dither(module, r_dither, g_dither, b_dither);
	} else {
		enum polarity fd_polarity = polarity_activehigh;
		enum polarity hs_polarity = polarity_activehigh;
		enum polarity vs_polarity = polarity_activehigh;

		nx_dpc_set_sync(module,
			progressive,
			psync->h_active_len,
			psync->v_active_len,
			psync->h_sync_width,
			psync->h_front_porch,
			psync->h_back_porch,
			psync->v_sync_width,
			psync->v_front_porch,
			psync->v_back_porch,
			fd_polarity, hs_polarity,
			vs_polarity, 0, 0, 0, 0, 0, 0, 0);
		/* EvenVSW, EvenVFP, EvenVBP, VSP, VCP, EvenVSP, EvenVCP */

		nx_dpc_set_delay(module, rgb_pvd, hsync_cp1,
				vsync_fram, de_cp2);
		nx_dpc_set_output_format(module, out_format, 0);
		nx_dpc_set_dither(module, r_dither, g_dither, b_dither);
		nx_dpc_set_quantization_mode(module, qmode_256, qmode_256);
	}

	pr_debug("%s: display.%d (x=%4d, hfp=%3d, hbp=%3d, hsw=%3d)\n",
		 __func__, module, psync->h_active_len, psync->h_front_porch,
		 psync->h_back_porch, psync->h_sync_width);
	pr_debug("%s: display.%d (y=%4d, vfp=%3d, vbp=%3d, vsw=%3d)\n",
		 __func__, module, psync->v_active_len, psync->v_front_porch,
		 psync->v_back_porch, psync->v_sync_width);
	pr_debug
	    ("%s: display.%d clk 0[s=%d, d=%3d], 1[s=%d, d=%3d], inv[%d:%d]\n",
	     __func__, module, clk_src_lv0, clk_div_lv0, clk_src_lv1,
	     clk_div_lv1, pspar->clk_inv_lv0, pspar->clk_inv_lv1);
	pr_debug("%s: display.%d v_vso=%d, v_veo=%d, e_vso=%d, e_veo=%d\n",
		 __func__, module, v_vso, v_veo, e_vso, e_veo);
	pr_debug("%s: display.%d delay RGB=%d, HS=%d, VS=%d, DE=%d\n", __func__,
		 module, rgb_pvd, hsync_cp1, vsync_fram, de_cp2);

	return 0;
}

static int disp_syncgen_enable(struct disp_process_dev *pdev, int enable)
{
	struct disp_control_info *info = get_device_to_info(pdev);
	int wait = info->wait_time * 10;
	int module = info->module;

	pr_debug("%s : display.%d, %s, wait=%d, status=0x%x\n",
		 __func__, module, enable ? "ON" : "OFF", wait, pdev->status);

	if (!enable) {
		/* multilayer top */
		disp_multily_enable(info, enable);

		if (nx_dpc_get_dpc_enable(module)) {
			disp_syncgen_irqenable(module, 1);
			DISP_WAIT_POLL_VSYNC(module, wait);
		}

		nx_dpc_set_dpc_enable(module, 0);
		nx_dpc_set_clock_divisor_enable(module, 0);

		disp_syncgen_irqenable(info->module, 0);
		pdev->status &= ~PROC_STATUS_ENABLE;
	} else {
		/* set irq wait time */
		if (!(PROC_STATUS_READY & pdev->status)) {
			if (pdev->dev_id == DISP_DEVICE_TVOUT) {
				disp_multily_enable(info, enable);
				pdev->status |= PROC_STATUS_ENABLE;
				disp_syncgen_irqenable(info->module, 1);
				return 0;
			}
			pr_err("fail, %s not set sync ...\n",
				dev_to_str(pdev->dev_id));
			return -EINVAL;
		}

		disp_multily_enable(info, enable);
		disp_syncgen_prepare(info);
		disp_syncgen_irqenable(info->module, 1);

		if (module == 0)
			nx_dpc_set_reg_flush(module);

		nx_dpc_set_dpc_enable(module, 1);
		nx_dpc_set_clock_divisor_enable(module, 1);

		pdev->status |= PROC_STATUS_ENABLE;
	}

	return 0;
}

static int disp_syncgen_stat_enable(struct disp_process_dev *pdev)
{
	struct disp_control_info *info = get_device_to_info(pdev);
	struct disp_process_ops *ops;
	struct list_head *pos, *head;
	int ret = pdev->status & PROC_STATUS_ENABLE ? 1 : 0;

	pr_debug("%s %s -> ", __func__, dev_to_str(pdev->dev_id));

	head = &info->link;
	if (list_empty(head)) {
		pr_info("display:%9s not connected display out ...\n",
		       dev_to_str(pdev->dev_id));
		return -1;
	}

	/* from last */
	list_for_each_prev(pos, head) {
		if (list_is_last(pos, head)) {
			pdev = container_of(pos, struct disp_process_dev, list);
			if (pdev) {
				ops = pdev->disp_ops;
				if (ops && ops->stat_enable)
					ret = ops->stat_enable(pdev);
			}
			break;
		}
	}

	pr_debug("(%s status = %s)\n", dev_to_str(pdev->dev_id),
		 pdev->status & PROC_STATUS_ENABLE ? "ON" : "OFF");

	return ret;
}

static int disp_multily_suspend(struct disp_process_dev *pdev)
{
	struct disp_control_info *info = get_device_to_info(pdev);
	struct disp_multily_dev *pmly = &info->multilayer;
	int mlc_len = sizeof(struct nx_mlc_register_set);
	int module = info->module;

	pr_debug("%s display.%d (MLC:%s, DPC:%s)\n",
		 __func__, module, pmly->enable ? "ON" : "OFF",
		 pdev->status & PROC_STATUS_ENABLE ? "ON" : "OFF");

	nx_mlc_set_mlc_enable(module, 0);
	nx_mlc_set_top_dirty_flag(module);

	memcpy((void *)pmly->save_addr, (const void *)pmly->base_addr, mlc_len);

	return 0;
}

static void disp_multily_resume(struct disp_process_dev *pdev)
{
	struct disp_control_info *info = get_device_to_info(pdev);
	struct disp_multily_dev *pmly = &info->multilayer;
	int mlc_len = sizeof(struct nx_mlc_register_set);
	int module = info->module;
	int i = 0;

	pr_debug("%s display.%d (MLC:%s, DPC:%s)\n",
		 __func__, module, pmly->enable ? "ON" : "OFF",
		 pdev->status & PROC_STATUS_ENABLE ? "ON" : "OFF");

	/* restore */
	nx_mlc_set_clock_pclk_mode(module, nx_pclkmode_always);
	nx_mlc_set_clock_bclk_mode(module, nx_bclkmode_always);

	memcpy((void *)pmly->base_addr, (const void *)pmly->save_addr, mlc_len);

	if (pmly->enable) {
		nx_mlc_set_top_power_mode(module, 1);
		nx_mlc_set_top_sleep_mode(module, 0);
		nx_mlc_set_mlc_enable(module, 1);

		for (i = 0; LAYER_RGB_NUM > i; i++)
			nx_mlc_set_dirty_flag(module, i);

		nx_mlc_set_dirty_flag(module, LAYER_VID_NUM);
		nx_mlc_set_top_dirty_flag(module);
	}
}

static int disp_syncgen_suspend(struct disp_process_dev *pdev)
{
	struct disp_control_info *info = get_device_to_info(pdev);
	struct disp_multily_dev *pmly = &info->multilayer;
	int mlc_len = sizeof(struct nx_mlc_register_set);
	int dpc_len = sizeof(struct nx_dpc_register_set);
	int module = info->module;

	pr_debug("%s display.%d (MLC:%s, DPC:%s)\n",
		 __func__, module, pmly->enable ? "ON" : "OFF",
		 pdev->status & PROC_STATUS_ENABLE ? "ON" : "OFF");

	nx_mlc_set_mlc_enable(module, 0);
	nx_mlc_set_top_dirty_flag(module);

	nx_dpc_set_dpc_enable(module, 0);
	nx_dpc_set_clock_divisor_enable(module, 0);

	memcpy((void *)pmly->save_addr, (const void *)pmly->base_addr, mlc_len);
	memcpy((void *)pdev->save_addr, (const void *)pdev->base_addr, dpc_len);

	disp_syncgen_irqenable(module, 0);
	info->condition = 1;

	return 0;
}

static void disp_syncgen_resume(struct disp_process_dev *pdev)
{
	struct disp_control_info *info = get_device_to_info(pdev);
	struct disp_multily_dev *pmly = &info->multilayer;
	int mlc_len = sizeof(struct nx_mlc_register_set);
	int dpc_len = sizeof(struct nx_dpc_register_set);
	int module = info->module;
	int i = 0;

	pr_debug("%s display.%d (MLC:%s, DPC:%s)\n",
		 __func__, module, pmly->enable ? "ON" : "OFF",
		 pdev->status & PROC_STATUS_ENABLE ? "ON" : "OFF");

	/* restore */
	nx_mlc_set_clock_pclk_mode(module, nx_pclkmode_always);
	nx_mlc_set_clock_bclk_mode(module, nx_bclkmode_always);

	memcpy((void *)pmly->base_addr, (const void *)pmly->save_addr, mlc_len);

	if (pmly->enable) {
		nx_mlc_set_top_power_mode(module, 1);
		nx_mlc_set_top_sleep_mode(module, 0);
		nx_mlc_set_mlc_enable(module, 1);

		for (i = 0; LAYER_RGB_NUM > i; i++)
			nx_mlc_set_dirty_flag(module, i);

		nx_mlc_set_dirty_flag(module, LAYER_VID_NUM);
		nx_mlc_set_top_dirty_flag(module);
	}

	/* restore */
	nx_dpc_set_clock_pclk_mode(module, nx_pclkmode_always);

	memcpy((void *)pdev->base_addr, (const void *)pdev->save_addr, dpc_len);

	if (pdev->status & PROC_STATUS_ENABLE) {
		disp_syncgen_irqenable(module, 0);	/* disable interrupt */

		nx_dpc_set_dpc_enable(module, 1);
		nx_dpc_set_clock_divisor_enable(module, 1);

		/* wait sync */
		disp_syncgen_irqenable(module, 1);	/* enable interrupt */
	}
}

static inline int disp_ops_prepare_devs(struct list_head *head)
{
	struct disp_process_ops *ops;
	struct disp_process_dev *pdev;
	struct list_head *pos;
	int ret = 0;

	list_for_each_prev(pos, head) {
		pdev = container_of(pos, struct disp_process_dev, list);
		ops = pdev->disp_ops;
		pr_debug("PRE: %s, ops=0x%p\n", pdev->name, ops);
		if (ops && ops->prepare) {
			ret = ops->prepare(ops->dev);
			if (ret) {
				pr_err("fail, display prepare [%s]...\n",
					dev_to_str(pdev->dev_id));
				return -EINVAL;
			}
		}
	}
	return 0;
}

static inline void disp_ops_enable_devs(struct list_head *head, int on)
{
	struct disp_process_ops *ops;
	struct disp_process_dev *pdev;
	struct list_head *pos;

	list_for_each(pos, head) {
		pdev = container_of(pos, struct disp_process_dev, list);
		ops = pdev->disp_ops;
		pr_debug("%s: %s, ops=0x%p\n", on ? "ON " : "OFF", pdev->name,
			 ops);
		if (ops && ops->enable)
			ops->enable(ops->dev, on);
	}
}

static inline void disp_ops_pre_resume_devs(struct list_head *head)
{
	struct disp_process_ops *ops;
	struct disp_process_dev *pdev;
	struct list_head *pos;

	list_for_each_prev(pos, head) {
		pdev = container_of(pos, struct disp_process_dev, list);
		ops = pdev->disp_ops;
		pr_debug("PRE RESUME: %s, ops=0x%p\n", pdev->name, ops);
		if (ops && ops->pre_resume)
			ops->pre_resume(ops->dev);
	}
}

static inline int disp_ops_suspend_devs(struct list_head *head,
				int suspend)
{
	struct disp_process_ops *ops;
	struct disp_process_dev *pdev;
	struct list_head *pos;
	int ret = 0;

	if (suspend) {		/* last  -> first */
		list_for_each_prev(pos, head) {
			pdev = container_of(pos, struct disp_process_dev, list);
			ops = pdev->disp_ops;
			pr_debug("SUSPEND: %s, ops=0x%p\n", pdev->name, ops);
			if (ops && ops->suspend) {
				ret = ops->suspend(ops->dev);
				if (ret)
					return -EINVAL;
			}
		}
	} else {
		list_for_each(pos, head) {
			pdev = container_of(pos, struct disp_process_dev, list);
			ops = pdev->disp_ops;
			pr_debug("RESUME : %s, ops=0x%p\n", pdev->name, ops);
			if (ops && ops->resume)
				ops->resume(ops->dev);
		}
	}

	return 0;
}

static struct disp_process_ops syncgen_ops[DISPLAY_SYNCGEN_NUM] = {
	/* primary */
	{
	 .set_vsync = disp_syncgen_set_vsync,
	 .get_vsync = disp_syncgen_get_vsync,
	 .enable = disp_syncgen_enable,
	 .stat_enable = disp_syncgen_stat_enable,
	 .suspend = disp_syncgen_suspend,
	 .resume = disp_syncgen_resume,
	 },
#if (DISPLAY_SYNCGEN_NUM > 1)
	/* secondary */
	{
	 .set_vsync = disp_syncgen_set_vsync,
	 .get_vsync = disp_syncgen_get_vsync,
	 .enable = disp_syncgen_enable,
	 .stat_enable = disp_syncgen_stat_enable,
	 .suspend = disp_syncgen_suspend,
	 .resume = disp_syncgen_resume,
	 },
#endif
};

/*
 * RGB Layer on MultiLayer
 */
int nx_soc_disp_rgb_set_fblayer(int module, int layer)
{
	DISP_MULTILY_DEV(module, pmly);
	RET_ASSERT_VAL(module > -1 && 2 > module, -EINVAL);
	RET_ASSERT_VAL(layer > -1 && LAYER_RGB_NUM > layer, -EINVAL);
	pr_debug("%s: framebuffer layer = %d.%d\n", __func__, module, layer);

	pmly->fb_layer = layer;

	return 0;
}

int nx_soc_disp_rgb_get_fblayer(int module)
{
	DISP_MULTILY_DEV(module, pmly);
	return pmly->fb_layer;
}

int nx_soc_disp_rgb_set_format(int module, int layer, unsigned int format,
				int image_w, int image_h, int pixelbyte)
{
	DISP_MULTILY_RGB(module, prgb, layer);
	int EnAlpha = 0;

	pr_debug("%s: %s, fmt:0x%x, w=%d, h=%d, bpp/8=%d\n",
		 __func__, prgb->name, format, image_w, image_h, pixelbyte);

	prgb->pos_x = 0;
	prgb->pos_y = 0;
	prgb->left = 0;
	prgb->top = 0;
	prgb->right = image_w;
	prgb->bottom = image_h;
	prgb->format = format;
	prgb->width = image_w;
	prgb->height = image_h;
	prgb->pixelbyte = pixelbyte;
	prgb->clipped = 0;

	/* set alphablend */
	if (format == MLC_RGBFMT_A1R5G5B5 ||
	    format == MLC_RGBFMT_A1B5G5R5 ||
	    format == MLC_RGBFMT_A4R4G4B4 ||
	    format == MLC_RGBFMT_A4B4G4R4 ||
	    format == MLC_RGBFMT_A8R3G3B2 ||
	    format == MLC_RGBFMT_A8B3G3R2 ||
	    format == MLC_RGBFMT_A8R8G8B8 || format == MLC_RGBFMT_A8B8G8R8)
		EnAlpha = 1;

	/* psw0523 fix for video -> prgb setting ordering */
	/* nx_mlc_set_transparency(module, layer, 0, prgb->color.transcolor); */
	nx_mlc_set_color_inversion(module, layer, 0, prgb->color.invertcolor);
	nx_mlc_set_alpha_blending(module, layer, EnAlpha,
				  prgb->color.alphablend);
	nx_mlc_set_format_rgb(module, layer, (enum nx_mlc_rgbfmt)format);
	nx_mlc_set_rgblayer_invalid_position(module, layer, 0, 0, 0, 0, 0, 0);
	nx_mlc_set_rgblayer_invalid_position(module, layer, 1, 0, 0, 0, 0, 0);

	if (image_w && image_h)
		nx_mlc_set_position(module, layer, 0, 0, image_w - 1,
				    image_h - 1);

	return 0;
}

void nx_soc_disp_rgb_get_format(int module, int layer,
				unsigned int *format, int *image_w,
				int *image_h, int *pixelbyte)
{
	DISP_MULTILY_RGB(module, prgb, layer);

	if (format)
		*format = prgb->format;
	if (image_w)
		*image_w = prgb->width;
	if (image_h)
		*image_h = prgb->height;
	if (pixelbyte)
		*pixelbyte = prgb->pixelbyte;
}

int nx_soc_disp_rgb_set_position(int module, int layer, int x, int y,
				int waitvsync)
{
	DISP_MULTILY_RGB(module, prgb, layer);
	int left = prgb->pos_x = x;
	int top = prgb->pos_y = y;
	int right = x + prgb->right;
	int bottom = y + prgb->bottom;

	RET_ASSERT_VAL(prgb->format, -EINVAL);

	pr_debug("%s: %s, wait=%d - L=%d, T=%d, R=%d, B=%d\n",
		 __func__, prgb->name, waitvsync, left, top, right, bottom);

	nx_mlc_set_position(module, layer, left, top, right - 1, bottom - 1);
	nx_mlc_set_dirty_flag(module, layer);
	disp_syncgen_waitsync(module, layer, waitvsync);

	return 0;
}

int nx_soc_disp_rgb_get_position(int module, int layer, int *x, int *y)
{
	int left, top, right, bottom;

	nx_mlc_get_position(module, layer, &left, &top, &right, &bottom);
	if (x)
		*x = left;
	if (y)
		*y = top;
	return 0;
}

int nx_soc_disp_rgb_set_clipping(int module, int layer,
				int left, int top, int width, int height)
{
	DISP_MULTILY_RGB(module, prgb, layer);

	RET_ASSERT_VAL(left >= 0 && top >= 0, -EINVAL);
	RET_ASSERT_VAL(width > 0 && height > 0, -EINVAL);

	pr_debug("%s: %s, x=%d, y=%d, w=%d, h=%d\n",
		 __func__, prgb->name, left, top, width, height);

	prgb->clipped = 1;
	prgb->left = left;
	prgb->top = top;
	prgb->right = left + width;
	prgb->bottom = top + height;

	return 0;
}

void nx_soc_disp_rgb_get_clipping(int module, int layer,
				int *left, int *top, int *width, int *height)
{
	DISP_MULTILY_RGB(module, prgb, layer);

	if (left)
		*left = prgb->left;
	if (top)
		*top = prgb->top;
	if (width)
		*width = prgb->right - prgb->left;
	if (height)
		*height = prgb->bottom - prgb->top;
}

void nx_soc_disp_rgb_set_address(int module, int layer,
				unsigned int phyaddr, unsigned int pixelbyte,
				unsigned int stride, int waitvsync)
{
	DISP_MULTILY_RGB(module, prgb, layer);

	pr_debug("%s: %s, pa=0x%x, hs=%d, vs=%d, wait=%d\n",
		 __func__, prgb->name, phyaddr, pixelbyte, stride, waitvsync);

	if (prgb->clipped) {
		int xoff = prgb->left * pixelbyte;
		int yoff = prgb->top * (prgb->width * prgb->pixelbyte);

		phyaddr += (xoff + yoff);
		stride = (prgb->width - prgb->left) * prgb->pixelbyte;
		nx_mlc_set_position(module, layer,
				    prgb->pos_x, prgb->pos_x, prgb->right - 1,
				    prgb->bottom - 1);
	}

	prgb->address = phyaddr;
	prgb->pixelbyte = pixelbyte;
	prgb->stride = stride;

	nx_mlc_set_rgblayer_stride(module, layer, pixelbyte, stride);
	nx_mlc_set_rgblayer_address(module, layer, phyaddr);
	nx_mlc_set_dirty_flag(module, layer);
	disp_syncgen_waitsync(module, layer, waitvsync);
}

void nx_soc_disp_rgb_get_address(int module, int layer,
				unsigned int *phyaddr,
				unsigned int *pixelbyte, unsigned int *stride)
{
	DISP_MULTILY_RGB(module, prgb, layer);

	if (!prgb->pixelbyte || !prgb->stride || !prgb->address) {
		unsigned int phy_addr, pixelbyte, stride;

		nx_mlc_get_rgblayer_stride(module, layer, &pixelbyte, &stride);
		nx_mlc_get_rgblayer_address(module, layer, &phy_addr);
		prgb->pixelbyte = pixelbyte;
		prgb->stride = stride;
		prgb->address = phy_addr;
	}

	if (phyaddr)
		*phyaddr = prgb->address;
	if (pixelbyte)
		*pixelbyte = prgb->pixelbyte;
	if (stride)
		*stride = prgb->stride;

	pr_debug("%s: %s, pa=0x%x, hs=%d, vs=%d\n",
		 __func__, prgb->name, prgb->address, prgb->pixelbyte,
		 prgb->stride);
}

void nx_soc_disp_rgb_set_color(int module, int layer, unsigned int type,
				unsigned int color, int enable)
{
	DISP_MULTILY_RGB(module, prgb, layer);

	switch (type) {
	case RGB_COLOR_ALPHA:
		if (color <= 0)
			color = 0;
		if (color >= 15)
			color = 15;
		prgb->color.alpha = (enable ? color : 15);
		nx_mlc_set_alpha_blending(module, layer, (enable ? 1 : 0),
					  color);
		nx_mlc_set_dirty_flag(module, layer);
		break;
	case RGB_COLOR_TRANSP:
		if (1 == prgb->pixelbyte) {
			color = R8G8B8toR3G3B2((unsigned int)color);
			color = R3G3B2toR8G8B8((u_char) color);
		}

		if (2 == prgb->pixelbyte) {
			color = R8G8B8toR5G6B5((unsigned int)color);
			color = R5G6B5toR8G8B8((u_short) color);
		}

		prgb->color.transcolor = (enable ? color : 0);
		nx_mlc_set_transparency(module, layer,
					(enable ? 1 : 0), (color & 0x00FFFFFF));
		nx_mlc_set_dirty_flag(module, layer);
		break;
	case RGB_COLOR_INVERT:
		if (1 == prgb->pixelbyte) {
			color = R8G8B8toR3G3B2((unsigned int)color);
			color = R3G3B2toR8G8B8((u_char) color);
		}

		if (2 == prgb->pixelbyte) {
			color = R8G8B8toR5G6B5((unsigned int)color);
			color = R5G6B5toR8G8B8((u_short) color);
		}
		prgb->color.invertcolor = (enable ? color : 0);
		nx_mlc_set_color_inversion(module, layer,
					   (enable ? 1 : 0),
					   (color & 0x00FFFFFF));
		nx_mlc_set_dirty_flag(module, layer);
		break;
	default:
		break;
	}
}

unsigned int nx_soc_disp_rgb_get_color(int module, int layer,
				unsigned int type)
{
	DISP_MULTILY_RGB(module, prgb, layer);

	switch (type) {
	case RGB_COLOR_ALPHA:
		return (unsigned int)prgb->color.alpha;
	case RGB_COLOR_TRANSP:
		return (unsigned int)prgb->color.transcolor;
	case RGB_COLOR_INVERT:
		return (unsigned int)prgb->color.invertcolor;
	default:
		break;
	}
	return 0;
}

void nx_soc_disp_rgb_set_enable(int module, int layer, int enable)
{
	DISP_MULTILY_RGB(module, prgb, layer);
	pr_debug("%s: %s, %s\n", __func__, prgb->name, enable ? "ON" : "OFF");

	prgb->enable = enable;
	nx_mlc_set_layer_enable(module, layer, (enable ? 1 : 0));
	nx_mlc_set_dirty_flag(module, layer);
}

int nx_soc_disp_rgb_stat_enable(int module, int layer)
{
	int enable = nx_mlc_get_layer_enable(module, layer) ? 1 : 0;

	DISP_MULTILY_RGB(module, prgb, layer);

	prgb->enable = enable;
	return enable;
}

/*
 * RGB Layer on MultiLayer
 */
int nx_soc_disp_video_set_format(int module, unsigned int fourcc,
				int image_w, int image_h)
{
	DISP_MULTILY_VID(module, pvid);
	unsigned int format;

	RET_ASSERT_VAL(image_w > 0 && image_h > 0, -EINVAL);

	switch (fourcc) {
	case FOURCC_MVS0:
	case FOURCC_YV12:
		format = nx_mlc_yuvfmt_420;
		break;
	case FOURCC_MVS2:
	case FOURCC_MVN2:
		format = nx_mlc_yuvfmt_422;
		break;
	case FOURCC_MVS4:
		format = nx_mlc_yuvfmt_444;
		break;
	case FOURCC_YUY2:
	case FOURCC_YUYV:
		format = nx_mlc_yuvfmt_yuyv;
		break;
	default:
		pr_err("fail, not support video fourcc=%c%c%c%c\n",
		       (fourcc >> 0) & 0xFF, (fourcc >> 8) & 0xFF,
		       (fourcc >> 16) & 0xFF, (fourcc >> 24) & 0xFF);
		return -EINVAL;
	}

	pvid->format = fourcc;
	pvid->width = image_w;
	pvid->height = image_h;

	nx_mlc_set_format_yuv(module, (enum nx_mlc_yuvfmt)format);
	nx_mlc_set_dirty_flag(module, MLC_LAYER_VIDEO);

	return 0;
}

void nx_soc_disp_video_get_format(int module, unsigned int *fourcc,
				int *image_w, int *image_h)
{
	DISP_MULTILY_VID(module, pvid);

	*fourcc = pvid->format;
	*image_w = pvid->width;
	*image_h = pvid->height;
}

void nx_soc_disp_video_set_address(int module,
				unsigned int lu_a, unsigned int lu_s,
				unsigned int cb_a, unsigned int cb_s,
				unsigned int cr_a, unsigned int cr_s,
				int waitvsync)
{
	DISP_MULTILY_VID(module, pvid);

	pr_debug(
		"%s: %s, lua=0x%x, cba=0x%x, cra=0x%x (lus=%d, cbs=%d, crs=%d), w=%d\n",
	    __func__, pvid->name, lu_a, cb_a, cr_a, lu_s, cb_s, cr_s,
	    waitvsync);

	if (FOURCC_YUYV == pvid->format) {
		nx_mlc_set_video_layer_address_yuyv(module, lu_a, lu_s);
	} else {
		nx_mlc_set_video_layer_stride(module, lu_s, cb_s, cr_s);
		nx_mlc_set_video_layer_address(module, lu_a, cb_a, cr_a);
	}

	nx_mlc_set_dirty_flag(module, MLC_LAYER_VIDEO);
	disp_syncgen_waitsync(module, MLC_LAYER_VIDEO, waitvsync);
}

void nx_soc_disp_video_get_address(int module,
				unsigned int *lu_a, unsigned int *lu_s,
				unsigned int *cb_a, unsigned int *cb_s,
				unsigned int *cr_a, unsigned int *cr_s)
{
	DISP_MULTILY_VID(module, pvid);

	if (FOURCC_YUYV == pvid->format) {
		RET_ASSERT(lu_a && lu_s);
		nx_mlc_get_video_layer_address_yuyv(module, lu_a, lu_s);
	} else {
		RET_ASSERT(lu_a && cb_a && cr_a);
		RET_ASSERT(lu_s && cb_s && cr_s);
		nx_mlc_get_video_layer_address(module, lu_a, cb_a, cr_a);
		nx_mlc_get_video_layer_stride(module, lu_s, cb_s, cr_s);
	}
}

int nx_soc_disp_video_set_position(int module, int left, int top,
				int right, int bottom, int waitvsync)
{
	DISP_MULTILY_VID(module, pvid);
	int srcw = pvid->width;
	int srch = pvid->height;
	int dstw = right - left;
	int dsth = bottom - top;
	int hf = 1, vf = 1;

	RET_ASSERT_VAL(pvid->format, -EINVAL);

	pr_debug("%s: %s, L=%d, T=%d, R=%d, B=%d, wait=%d\n",
		 __func__, pvid->name, left, top, right, bottom, waitvsync);

	if (left >= right)
		right = left + 1;
	if (top >= bottom)
		bottom = top + 1;
	if (0 >= right)
		left -= (right - 1), right = 1;
	if (0 >= bottom)
		top -= (bottom - 1), bottom = 1;
	if (dstw >= 2048)
		dstw = 2048;
	if (dsth >= 2048)
		dsth = 2048;

	if (srcw == dstw && srch == dsth)
		hf = 0, vf = 0;

	pvid->hFilter = hf;
	pvid->vFilter = vf;

	/* set scale */
	nx_mlc_set_video_layer_scale(module, srcw, srch, dstw, dsth,
				     hf, hf, vf, vf);
	nx_mlc_set_position(module, MLC_LAYER_VIDEO, left, top, right - 1,
			    bottom - 1);
	nx_mlc_set_dirty_flag(module, MLC_LAYER_VIDEO);
	disp_syncgen_waitsync(module, MLC_LAYER_VIDEO, waitvsync);

	return 0;
}

void nx_soc_disp_video_set_crop(int module, bool enable,
				int left, int top, int width, int height,
				int waitvsync)
{
	DISP_MULTILY_VID(module, pvid);

	pvid->en_source_crop = enable;

	if (enable) {
		int srcw = width;
		int srch = height;
		int dstw = pvid->right - pvid->left;
		int dsth = pvid->bottom - pvid->top;
		int hf = 1, vf = 1;

		if (dstw == 0)
			dstw = pvid->width;
		if (dsth == 0)
			dsth = pvid->height;

		pr_info(
			"%s: %s, L=%d, T=%d, W=%d, H=%d, dstw=%d, dsth=%d, wait=%d\n",
		    __func__, pvid->name, left, top, width, height, dstw, dsth,
		    waitvsync);

		if (srcw == dstw && srch == dsth)
			hf = 0, vf = 0;

		pvid->hFilter = hf;
		pvid->vFilter = vf;

		/* set scale */
		nx_mlc_set_video_layer_scale(module, srcw, srch, dstw, dsth,
					     hf, hf, vf, vf);
		nx_mlc_set_dirty_flag(module, MLC_LAYER_VIDEO);
		disp_syncgen_waitsync(module, MLC_LAYER_VIDEO, waitvsync);
	} else {
		nx_soc_disp_video_set_position(module, pvid->left, pvid->top,
						pvid->right, pvid->bottom,
						waitvsync);
	}
}

int nx_soc_disp_video_get_position(int module, int *left, int *top,
				int *right, int *bottom)
{
	nx_mlc_get_video_position(module, left, top, right, bottom);
	return 0;
}

void nx_soc_disp_video_set_enable(int module, int enable)
{
	DISP_MULTILY_VID(module, pvid);
	int hl, hc, vl, vc;

	if (enable) {
		nx_mlc_set_video_layer_line_buffer_power_mode(module, 1);
		nx_mlc_set_video_layer_line_buffer_sleep_mode(module, 0);
		nx_mlc_set_layer_enable(module, MLC_LAYER_VIDEO, 1);
		nx_mlc_set_dirty_flag(module, MLC_LAYER_VIDEO);
	} else {
		nx_mlc_set_layer_enable(module, MLC_LAYER_VIDEO, 0);
		nx_mlc_set_dirty_flag(module, MLC_LAYER_VIDEO);
		disp_syncgen_waitsync(module, MLC_LAYER_VIDEO, 1);

		nx_mlc_get_video_layer_scale_filter(module, &hl, &hc, &vl, &vc);
		if (hl | hc | vl | vc)
			nx_mlc_set_video_layer_scale_filter(module, 0, 0, 0, 0);
		nx_mlc_set_video_layer_line_buffer_power_mode(module, 0);
		nx_mlc_set_video_layer_line_buffer_sleep_mode(module, 1);
		nx_mlc_set_dirty_flag(module, MLC_LAYER_VIDEO);
	}
	pvid->enable = enable;
}

int nx_soc_disp_video_stat_enable(int module)
{
	int enable = nx_mlc_get_layer_enable(module, MLC_LAYER_VIDEO) ? 1 : 0;

	DISP_MULTILY_VID(module, pvid);

	pvid->enable = enable;

	return enable;
}

void nx_soc_disp_video_set_priority(int module, int prior)
{
	DISP_MULTILY_DEV(module, pmly);
	enum nx_mlc_priority priority = nx_mlc_priority_videofirst;

	pr_debug("%s: multilayer.%d, video prior=%d\n", __func__, module,
		 prior);

	switch (prior) {
	case 0:
		priority = nx_mlc_priority_videofirst;
		break;		/* PRIORITY-video>0>1>2 */
	case 1:
		priority = nx_mlc_priority_videosecond;
		break;		/* PRIORITY-0>video>1>2 */
	case 2:
		priority = nx_mlc_priority_videothird;
		break;		/* PRIORITY-0>1>video>2 */
	case 3:
		priority = nx_mlc_priority_videofourth;
		break;		/* PRIORITY-0>1>2>video */
	default:
		pr_err(
			"fail, Not support video priority num(0~3),(%d)\n",
		    prior);
		return;
	}

	nx_mlc_set_layer_priority(module, priority);
	nx_mlc_set_top_dirty_flag(module);
	pmly->video_prior = prior;
}

int nx_soc_disp_video_get_priority(int module)
{
	DISP_MULTILY_DEV(module, pmly);
	return (int)pmly->video_prior;
}

void nx_soc_disp_video_set_colorkey(int module,
				unsigned int color, int enable)
{
	DISP_MULTILY_DEV(module, pmly);
	nx_soc_disp_rgb_set_color(module,
				   pmly->fb_layer, RGB_COLOR_TRANSP, color,
				   enable);
}

unsigned int nx_soc_disp_video_get_colorkey(int module)
{
	DISP_MULTILY_DEV(module, pmly);
	return nx_soc_disp_rgb_get_color(module,
					  pmly->fb_layer, RGB_COLOR_TRANSP);
}

void nx_soc_disp_video_set_color(int module, unsigned int type,
				unsigned int color, int enable)
{
	DISP_MULTILY_VID(module, pvid);

	pr_debug("%s: %s, type=0x%x, col=0x%x, %s\n",
		 __func__, pvid->name, type, color, enable ? "ON" : "OFF");

	switch (type) {
	case VIDEO_COLOR_ALPHA:
		pvid->color.alpha = (enable ? color : 15);	/* Default 15 */
		video_alpha_blend(module, pvid->color.alpha, enable);
		break;
	case VIDEO_COLOR_BRIGHT:
		pvid->color.bright = (enable ? color : 0);
		pvid->color.contrast = (enable ? pvid->color.contrast : 0);
		video_lu_color(module, (int)pvid->color.bright,
			       (int)pvid->color.contrast);
		break;
	case VIDEO_COLOR_CONTRAST:
		pvid->color.contrast = (enable ? color : 0);
		pvid->color.bright = (enable ? pvid->color.bright : 0);
		video_lu_color(module, (int)pvid->color.bright,
			       (int)pvid->color.contrast);
		break;
	case VIDEO_COLOR_GAMMA:
		pvid->color.gamma = color;
		break;
	default:
		break;
	}
}

unsigned int nx_soc_disp_video_get_color(int module, unsigned int type)
{
	DISP_MULTILY_VID(module, pvid);
	unsigned int color;

	switch (type) {
	case VIDEO_COLOR_ALPHA:
		color = (unsigned int)pvid->color.alpha;
		break;
	case VIDEO_COLOR_BRIGHT:
		color = (unsigned int)pvid->color.bright;
		break;
	case VIDEO_COLOR_CONTRAST:
		color = (unsigned int)pvid->color.contrast;
		break;
#if 0
	case VIDEO_COLOR_HUE:
		color = (unsigned int)pvid->color.hue;
		break;
#endif
	case VIDEO_COLOR_SATURATION:
		color = (unsigned int)pvid->color.satura;
		break;
	case VIDEO_COLOR_GAMMA:
		color = (unsigned int)pvid->color.gamma;
		break;
	default:
		return -EINVAL;
	}
	return color;
}

void nx_soc_disp_video_set_hfilter(int module, int enable)
{
	DISP_MULTILY_VID(module, pvid);
	int hl, hc, vl, vc;

	hl = hc = pvid->hFilter = enable;
	vl = vc = pvid->vFilter;
	nx_mlc_set_video_layer_scale_filter(module, hl, hc, vl, vc);
	nx_mlc_set_dirty_flag(module, MLC_LAYER_VIDEO);
}

unsigned int nx_soc_disp_video_get_hfilter(int module)
{
	int hl, hc, vl, vc;

	nx_mlc_get_video_layer_scale_filter(module, &hl, &hc, &vl, &vc);
	if (hl != hc) {
		pr_info("%s: WARN %d horizontal filter Lu=%s, Ch=%s \r\n",
			__func__, module, hl ? "On" : "Off", hc ? "On" : "Off");
	}

	return (unsigned int)hl;
}

void nx_soc_disp_video_set_vfilter(int module, int enable)
{
	DISP_MULTILY_VID(module, pvid);
	int hl, hc, vl, vc;

	hl = hc = pvid->hFilter;
	vl = vc = pvid->vFilter = enable;
	nx_mlc_set_video_layer_scale_filter(module, hl, hc, vl, vc);
	nx_mlc_set_dirty_flag(module, MLC_LAYER_VIDEO);
}

unsigned int nx_soc_disp_video_get_vfilter(int module)
{
	int hl, hc, vl, vc;

	nx_mlc_get_video_layer_scale_filter(module, &hl, &hc, &vl, &vc);
	if (hl != hc) {
		pr_info("%s: WARN %d vertical filter Lu=%s, Ch=%s \r\n",
			__func__, module, vl ? "On" : "Off", vc ? "On" : "Off");
	}

	return (unsigned int)vl;
}

/*
 * TOP Layer on MultiLayer
 */
void nx_soc_disp_get_resolution(int module, int *w, int *h)
{
	DISP_MULTILY_DEV(module, pmly);
	*w = pmly->x_resol;
	*h = pmly->y_resol;
}

void nx_soc_disp_set_bg_color(int module, unsigned int color)
{
	DISP_MULTILY_DEV(module, pmly);
	pmly->bg_color = color;
	nx_mlc_set_background(module, color & 0x00FFFFFF);
	nx_mlc_set_top_dirty_flag(module);
}

unsigned int nx_soc_disp_get_bg_color(int module)
{
	DISP_MULTILY_DEV(module, pmly);
	return pmly->bg_color;
}

int nx_soc_disp_wait_vertical_sync(int module)
{
	return disp_syncgen_waitsync(module, 0, 1);
}

void nx_soc_disp_layer_set_enable(int module, int layer, int enable)
{
	if (MLC_LAYER_VIDEO == layer)
		nx_soc_disp_video_set_enable(module, enable);
	else
		nx_soc_disp_rgb_set_enable(module, layer, enable);
}

int nx_soc_disp_layer_stat_enable(int module, int layer)
{
	if (MLC_LAYER_VIDEO == layer)
		return nx_soc_disp_video_stat_enable(module);
	else
		return nx_soc_disp_rgb_stat_enable(module, layer);
}

/*
 *	Display Devices
 */
int nx_soc_disp_device_connect_to(enum disp_dev_type device,
				enum disp_dev_type to,
				struct disp_vsync_info *psync)
{
	struct disp_process_dev *pdev, *sdev;
	struct disp_process_ops *ops;
	struct disp_control_info *info;
	struct list_head *head, *new, *obj;
	int ret = 0;

	RET_ASSERT_VAL(device != to, -EINVAL);
	RET_ASSERT_VAL(DEVICE_SIZE > device, -EINVAL);
	RET_ASSERT_VAL(to == DISP_DEVICE_SYNCGEN0 || to == DISP_DEVICE_SYNCGEN1
		       || to == DISP_DEVICE_RESCONV, -EINVAL);
	pr_debug("%s: %s, in[%s]\n", __func__, dev_to_str(device),
		 dev_to_str(to));

	sdev = get_display_ptr((int)to);
	pdev = get_display_ptr((int)device);
	sdev->dev_out = device;
	pdev->dev_in = to;

	/* list add */
	if (DISP_DEVICE_SYNCGEN0 == sdev->dev_id ||
	    DISP_DEVICE_SYNCGEN1 == sdev->dev_id) {
		info = get_device_to_info(sdev);
		head = &info->link;
	} else {
		head = &disp_resconv_link;
	}

	spin_lock(&sdev->lock);

	/* check connect status */
	list_for_each(obj, head) {
		struct disp_process_dev *dev = container_of(obj,
							    struct
							    disp_process_dev,
							    list);
		if (dev == pdev) {
			pr_err("fail, %s is already connected to %s ...\n",
			       dev_to_str(dev->dev_id),
			       dev_to_str(sdev->dev_id));
			ret = -EINVAL;
			break;
		}
	}

	spin_unlock(&sdev->lock);

	if (0 > ret)
		return ret;

	if (psync) {
		ops = sdev->disp_ops;	/* source sync */
		if (ops && ops->set_vsync) {
			ret = ops->set_vsync(sdev, psync);
			if (0 > ret)
				return ret;
		}

		ops = pdev->disp_ops;	/* device operation */
		if (ops && ops->set_vsync) {
			ret = ops->set_vsync(pdev, psync);
			if (0 > ret)
				return ret;
		}
	}

	spin_lock(&sdev->lock);
	new = &pdev->list;
	list_add_tail(new, head);
	spin_unlock(&sdev->lock);

	return ret;
}

void nx_soc_disp_device_disconnect(enum disp_dev_type device,
				enum disp_dev_type to)
{
	struct disp_process_dev *pdev, *sdev;
	struct list_head *entry;

	RET_ASSERT(device != to);
	RET_ASSERT(DEVICE_SIZE > device);
	RET_ASSERT(to == DISP_DEVICE_SYNCGEN0 || to == DISP_DEVICE_SYNCGEN1 ||
		   to == DISP_DEVICE_RESCONV);
	pr_debug("%s: %s, in[%s]\n", __func__, dev_to_str(device),
		 dev_to_str(to));

	/* list delete */
	sdev = get_display_ptr((int)to);
	pdev = get_display_ptr((int)device);

	spin_lock(&sdev->lock);

	/* list add */
	entry = &pdev->list;
	list_del(entry);

	spin_unlock(&sdev->lock);
}

int nx_soc_disp_device_set_vsync_info(enum disp_dev_type device,
				struct disp_vsync_info *psync)
{
	struct disp_process_ops *ops = NULL;

	RET_ASSERT_VAL(DEVICE_SIZE > device, -EINVAL);
	pr_debug("%s: %s\n", __func__, dev_to_str(device));

	ops = get_display_ops(device);
	if (ops && ops->set_vsync)
		return ops->set_vsync(ops->dev, psync);

	return -1;
}

int nx_soc_disp_device_get_vsync_info(enum disp_dev_type device,
				struct disp_vsync_info *psync)
{
	struct disp_process_ops *ops = NULL;

	RET_ASSERT_VAL(DEVICE_SIZE > device, -EINVAL);
	pr_debug("%s: %s\n", __func__, dev_to_str(device));

	ops = get_display_ops(device);
	if (ops && ops->get_vsync)
		return ops->get_vsync(ops->dev, psync);

	return -1;
}

int nx_soc_disp_device_set_sync_param(enum disp_dev_type device,
				struct disp_sync_par *spar)
{
	struct disp_process_dev *pdev = get_display_ptr(device);
	struct disp_sync_par *pdst, *psrc;

	RET_ASSERT_VAL(spar, -EINVAL);
	RET_ASSERT_VAL(DEVICE_SIZE > device, -EINVAL);
	pr_debug("%s: %s\n", __func__, dev_to_str(device));

	pdst = &pdev->sync_par;
	psrc = spar;
	set_syncgen_param(psrc, pdst);

	return 0;
}

int nx_soc_disp_device_get_sync_param(enum disp_dev_type device,
				struct disp_sync_par *spar)
{
	struct disp_process_dev *pdev = get_display_ptr(device);
	int size = sizeof(struct disp_sync_par);

	RET_ASSERT_VAL(spar, -EINVAL);
	RET_ASSERT_VAL(DEVICE_SIZE > device, -EINVAL);
	pr_debug("%s: %s\n", __func__, dev_to_str(device));

	memcpy(spar, &pdev->sync_par, size);
	return 0;
}

int nx_soc_disp_device_set_dev_param(enum disp_dev_type device,
				void *param)
{
	struct disp_process_dev *pdev = get_display_ptr(device);

	RET_ASSERT_VAL(param, -EINVAL);
	RET_ASSERT_VAL(DEVICE_SIZE > device, -EINVAL);

	pr_debug("%s: %s, param = %p\n",
		__func__, dev_to_str(device), pdev->dev_param);

	pdev->dev_param = param;

	return 0;
}

#define	no_prev(dev)	(NULL == &dev->list.prev ? 1 : 0)
#define	no_next(dev)	(NULL == &dev->list.next ? 1 : 0)

int nx_soc_disp_device_suspend(enum disp_dev_type device)
{
	struct disp_process_ops *ops = NULL;
	struct disp_control_info *info;
	struct lcd_operation *lcd;
	int ret = 0;

	RET_ASSERT_VAL(DEVICE_SIZE > device, -EINVAL);
	pr_debug("%s: %s\n", __func__, dev_to_str(device));

	ops = get_display_ops(device);

	/* LCD control: first */
	if (DISP_DEVICE_SYNCGEN0 == device || DISP_DEVICE_SYNCGEN1 == device) {
		RET_ASSERT_VAL(ops, -EINVAL);
		info = get_device_to_info(ops->dev);
		lcd = info->lcd_ops;

		if (no_prev(ops->dev) && lcd && lcd->backlight_suspend)
			lcd->backlight_suspend(info->module, lcd->data);
	}

	if (ops && ops->suspend)
		ret = ops->suspend(ops->dev);

	/* LCD control: last */
	if (DISP_DEVICE_SYNCGEN0 == device || DISP_DEVICE_SYNCGEN1 == device) {
		RET_ASSERT_VAL(ops, -EINVAL);
		info = get_device_to_info(ops->dev);
		lcd = info->lcd_ops;

		if (no_next(ops->dev) && lcd && lcd->lcd_suspend)
			lcd->lcd_suspend(info->module, lcd->data);
	}
	return ret;
}

void nx_soc_disp_device_resume(enum disp_dev_type device)
{
	struct disp_process_ops *ops = NULL;
	struct disp_control_info *info;
	struct disp_process_dev *pdev;
	struct lcd_operation *lcd;

	RET_ASSERT(DEVICE_SIZE > device);
	pr_debug("%s: %s\n", __func__, dev_to_str(device));

	ops = get_display_ops(device);

	/* LCD control: first */
	if (DISP_DEVICE_SYNCGEN0 == device || DISP_DEVICE_SYNCGEN1 == device) {
		RET_ASSERT(ops);
		info = get_device_to_info(ops->dev);
		pdev = info->proc_dev;
		lcd = info->lcd_ops;

		if (pdev->rstc)
			reset_control_reset(pdev->rstc);

		if (pdev->rstc2)
			reset_control_reset(pdev->rstc2);

		if (no_prev(ops->dev) && lcd && lcd->backlight_resume)
			lcd->backlight_resume(info->module, lcd->data);
	}

	if (ops && ops->resume)
		ops->resume(ops->dev);

	/* LCD control: last */
	if (DISP_DEVICE_SYNCGEN0 == device || DISP_DEVICE_SYNCGEN1 == device) {
		RET_ASSERT(ops);
		info = get_device_to_info(ops->dev);
		lcd = info->lcd_ops;

		if (no_next(ops->dev) && lcd && lcd->lcd_resume)
			lcd->lcd_resume(info->module, lcd->data);
	}
}

int nx_soc_disp_device_suspend_all(int module)
{
	struct disp_control_info *info = get_module_to_info(module);
	struct lcd_operation *lcd = info->lcd_ops;
	int ret = 0;

	RET_ASSERT_VAL(0 == module || 1 == module, -EINVAL);
	pr_debug("%s: display.%d\n", __func__, module);

	/* LCD control */
	if (lcd && lcd->backlight_suspend)
		lcd->backlight_suspend(module, lcd->data);

	if (list_empty(&info->link)) {
		pr_debug("display:%9s not connected display out ...\n",
			 dev_to_str(((struct disp_process_dev *)
				     get_display_ptr((0 ==
						      module ?
						      DISP_DEVICE_SYNCGEN0 :
						      DISP_DEVICE_SYNCGEN1)))->
				    dev_id));
		return 0;
	}

	ret = disp_ops_suspend_devs(&disp_resconv_link, 1);
	if (ret)
		return ret;

	ret = disp_ops_suspend_devs(&info->link, 1);
	if (ret)
		return ret;

	/* LCD control */
	if (lcd && lcd->lcd_suspend)
		lcd->lcd_suspend(module, lcd->data);

	return 0;
}

void nx_soc_disp_device_resume_all(int module)
{
	struct disp_control_info *info = get_module_to_info(module);
	struct disp_process_dev *pdev = info->proc_dev;
	struct lcd_operation *lcd = info->lcd_ops;

	RET_ASSERT(0 == module || 1 == module);
	pr_debug("%s: display.%d\n", __func__, module);

	if (pdev->rstc)
		reset_control_reset(pdev->rstc);

	if (pdev->rstc2)
		reset_control_reset(pdev->rstc2);

	/* LCD control */
	if (lcd && lcd->lcd_resume)
		lcd->lcd_resume(module, lcd->data);

	/* device control */
	if (list_empty(&info->link)) {
		pr_debug("display:%9s not connected display out ...\n",
			dev_to_str(((struct disp_process_dev *)
			get_display_ptr((0 == module ? DISP_DEVICE_SYNCGEN0 :
			DISP_DEVICE_SYNCGEN1)))->dev_id));
		return;
	}

	/* pre_resume */
	disp_ops_pre_resume_devs(&disp_resconv_link);
	disp_ops_pre_resume_devs(&info->link);

	/* resume */
	disp_ops_suspend_devs(&info->link, 0);
	disp_ops_suspend_devs(&disp_resconv_link, 0);

	/* LCD control */
	if (lcd && lcd->backlight_resume)
		lcd->backlight_resume(module, lcd->data);
}

int nx_soc_disp_device_enable(enum disp_dev_type device, int enable)
{
	struct disp_process_ops *ops = NULL;
	struct disp_control_info *info;
	struct lcd_operation *lcd;
	int ret = 0;

	RET_ASSERT_VAL(DEVICE_SIZE > device, -EINVAL);
	pr_debug("%s: %s, %s\n", __func__, dev_to_str(device),
		 enable ? "ON" : "OFF");

	ops = get_display_ops(device);

	/* LCD control: first */
	if (DISP_DEVICE_SYNCGEN0 == device || DISP_DEVICE_SYNCGEN1 == device) {
		RET_ASSERT_VAL(ops, -EINVAL);
		info = get_device_to_info(ops->dev);
		lcd = info->lcd_ops;

		if (enable) {
			if (no_prev(ops->dev) && lcd && lcd->lcd_poweron)
				lcd->lcd_poweron(info->module, lcd->data);
		} else {
			if (no_prev(ops->dev) && lcd && lcd->backlight_off)
				lcd->backlight_off(info->module, lcd->data);
		}
	}

	if (ops && ops->enable)
		ret = ops->enable(ops->dev, enable);

	/* LCD control: last */
	if (DISP_DEVICE_SYNCGEN0 == device || DISP_DEVICE_SYNCGEN1 == device) {
		RET_ASSERT_VAL(ops, -EINVAL);
		info = get_device_to_info(ops->dev);
		lcd = info->lcd_ops;

		if (enable) {
			if (no_next(ops->dev) && lcd && lcd->backlight_on)
				lcd->backlight_on(info->module, lcd->data);
		} else {
			if (no_next(ops->dev) && lcd && lcd->lcd_poweroff)
				lcd->lcd_poweroff(info->module, lcd->data);
		}
	}
	return ret;
}

int nx_soc_disp_device_stat_enable(enum disp_dev_type device)
{
	struct disp_process_ops *ops = NULL;

	RET_ASSERT_VAL(DEVICE_SIZE > device, -EINVAL);

	ops = get_display_ops(device);
	if (ops && ops->stat_enable)
		return ops->stat_enable(ops->dev);

	return 0;
}

int nx_soc_disp_device_enable_all(int module, int enable)
{
	struct disp_control_info *info = get_module_to_info(module);
	struct lcd_operation *lcd = info->lcd_ops;
	enum disp_dev_type device = (0 == module ?
				     DISP_DEVICE_SYNCGEN0 :
				     DISP_DEVICE_SYNCGEN1);
	int ret = 0;

	RET_ASSERT_VAL(0 == module || 1 == module, -EINVAL);
	pr_debug("%s: display.%d, %s\n", __func__, module,
		 enable ? "ON" : "OFF");

	/* LCD control */
	if (enable) {
		if (lcd && lcd->lcd_poweron)
			lcd->lcd_poweron(module, lcd->data);
	} else {
		if (lcd && lcd->backlight_off)
			lcd->backlight_off(module, lcd->data);
	}

	/* device control */
	if (list_empty(&info->link)) {
		device =
		    (0 == module ? DISP_DEVICE_SYNCGEN0 : DISP_DEVICE_SYNCGEN1);
		pr_info("display:%9s not connected display out ...\n",
		       dev_to_str(((struct disp_process_dev *)
				   get_display_ptr(device))->dev_id));
		nx_soc_disp_device_enable(device, 0);
		return 0;
	}

	if (enable) {
		ret = disp_ops_prepare_devs(&disp_resconv_link);
		if (ret)
			return -EINVAL;

		ret = disp_ops_prepare_devs(&info->link);
		if (ret)
			return -EINVAL;

		disp_ops_enable_devs(&info->link, 1);
		disp_ops_enable_devs(&disp_resconv_link, 1);

	} else {
		disp_ops_enable_devs(&disp_resconv_link, 0);
		disp_ops_enable_devs(&info->link, 0);
	}

	/* LCD control */
	if (enable) {
		if (lcd && lcd->backlight_on)
			lcd->backlight_on(module, lcd->data);
	} else {
		if (lcd && lcd->lcd_poweroff)
			lcd->lcd_poweroff(module, lcd->data);
	}

	return 0;
}

int nx_soc_disp_device_enable_all_saved(int module, int enable)
{
	struct disp_control_info *info = get_module_to_info(module);
	struct lcd_operation *lcd = info->lcd_ops;
	enum disp_dev_type device = (0 == module ? DISP_DEVICE_SYNCGEN0 :
				     DISP_DEVICE_SYNCGEN1);
	struct disp_process_dev *pdev = get_display_ptr(device);
	int ret = 0;

	RET_ASSERT_VAL(0 == module || 1 == module, -EINVAL);
	pr_debug("%s: display.%d, %s\n", __func__, module,
		 enable ? "ON" : "OFF");

	if (pdev->status & PROC_STATUS_ENABLE)
		return 0;

	/* LCD control */
	if (enable) {
		if (lcd && lcd->lcd_poweron)
			lcd->lcd_poweron(module, lcd->data);
	} else {
		if (lcd && lcd->backlight_off)
			lcd->backlight_off(module, lcd->data);
	}

	/* device control */
	if (list_empty(&info->link)) {
		device =
		    (0 == module ? DISP_DEVICE_SYNCGEN0 : DISP_DEVICE_SYNCGEN1);
		pr_info("display:%9s not connected display out ...\n",
		       dev_to_str(((struct disp_process_dev *)
				   get_display_ptr(device))->dev_id));
		nx_soc_disp_device_enable(device, 0);
		return 0;
	}

	if (enable) {
		ret = disp_ops_prepare_devs(&disp_resconv_link);
		if (ret)
			return -EINVAL;

		ret = disp_ops_prepare_devs(&info->link);
		if (ret)
			return -EINVAL;

		disp_multily_resume(pdev);	/* restore multiple layer */
		disp_ops_enable_devs(&info->link, 1);
		disp_ops_enable_devs(&disp_resconv_link, 1);

	} else {
		disp_multily_suspend(pdev);	/* save multiple layer */
		disp_ops_enable_devs(&disp_resconv_link, 0);
		disp_ops_enable_devs(&info->link, 0);
	}

	/* LCD control */
	if (enable) {
		if (lcd && lcd->backlight_on)
			lcd->backlight_on(module, lcd->data);
	} else {
		if (lcd && lcd->lcd_poweroff)
			lcd->lcd_poweroff(module, lcd->data);
	}

	return 0;
}

void nx_soc_disp_register_proc_ops(enum disp_dev_type device,
				struct disp_process_ops *ops)
{
	struct disp_process_dev *pdev = get_display_ptr(device);

	RET_ASSERT(DEVICE_SIZE > device);
	RET_ASSERT(device == pdev->dev_id);

	if (get_display_ops(device))
		pr_warn("Warn , %s operation will be replaced\n",
		       dev_to_str(device));

	spin_lock(&pdev->lock);

	/* set device info */
	set_display_ops(device, ops);

	spin_unlock(&pdev->lock);
	pr_info("Display %s register operation\n",
	       dev_to_str(device));
}

void nx_soc_disp_register_priv(enum disp_dev_type device, void *priv)
{
	struct disp_process_dev *pdev = get_display_ptr(device);

	RET_ASSERT(DEVICE_SIZE > device);
	RET_ASSERT(device == pdev->dev_id);

	pdev->priv = priv;
	pr_debug("%s: %p\n", __func__, priv);
}

void nx_soc_disp_register_lcd_ops(int module, struct lcd_operation *ops)
{
	DISP_CONTROL_INFO(module, info);
	info->lcd_ops = ops;
}

struct disp_irq_callback *nx_soc_disp_register_irq_callback(int module,
				void (*callback)(void *), void *data)
{
	unsigned long flags;
	struct disp_irq_callback *entry = NULL;
	struct disp_control_info *info = get_module_to_info(module);

	RET_ASSERT_NULL(0 == module || 1 == module);
	RET_ASSERT_NULL(callback);

	pr_debug("%s: display.%d\n", __func__, module);

	entry = (struct disp_irq_callback *)
	    kmalloc(sizeof(struct disp_irq_callback), GFP_KERNEL);
	if (!entry) {
		pr_err("%s: failed to allocate disp_irq_callback entry\n",
		       __func__);
		return NULL;
	}

	entry->handler = callback;
	entry->data = data;
	spin_lock_irqsave(&info->lock_callback, flags);
	list_add_tail(&entry->list, &info->callback_list);
	spin_unlock_irqrestore(&info->lock_callback, flags);

	return entry;
}

void nx_soc_disp_unregister_irq_callback(int module,
				struct disp_irq_callback *callback)
{
	unsigned long flags;
	struct disp_control_info *info = get_module_to_info(module);

	RET_ASSERT(0 == module || 1 == module);

	pr_debug("%s: display.%d\n", __func__, module);

	spin_lock_irqsave(&info->lock_callback, flags);
	list_del(&callback->list);
	spin_unlock_irqrestore(&info->lock_callback, flags);
	kfree(callback);
}

void nx_soc_disp_device_framebuffer(int module, int fb)
{
	struct disp_control_info *info = get_module_to_info(module);
	struct disp_process_dev *pdev = info->proc_dev;

	RET_ASSERT(0 == module || 1 == module);
	pdev->dev_in = fb;
	pr_info("display.%d connected to fb.%d  ...\n", module, fb);
}

/*
 * Notify vertical sync en/disable
 *
 * /sys/devices/platform/display/active.N
 */
static ssize_t active_show(struct device *pdev,
				struct device_attribute *attr, char *buf)
{
	struct attribute *at = &attr->attr;
	struct disp_control_info *info;
	struct disp_process_dev *dev;
	const char *c;
	char *s = buf;
	int a, i, d[2], m = 0;

	c = &at->name[strlen("active.")];
	a = kstrtoul(c, 10, NULL);

	/* for (i = 0; 2 > i; i++) { */
	for (i = 0; 1 > i; i++) {
		info = get_module_to_info(i);
		dev = info->proc_dev;
		d[i] = dev->dev_in;
	}

	/* not fb */
	if (d[0] != DISP_DEVICE_END && d[1] != DISP_DEVICE_END)
		m = a;
	else			/* 1 fb */
		m = (d[0] == a) ? 0 : 1;

	info = get_module_to_info(m);

	s += sprintf(s, "%d\n", info->active_notify);
	if (s != buf)
		*(s - 1) = '\n';

	return (s - buf);
}

static ssize_t active_store(struct device *pdev,
				struct device_attribute *attr, const char *buf,
			    size_t n)
{
	struct attribute *at = &attr->attr;
	struct disp_control_info *info;
	struct disp_process_dev *dev;
	const char *c;
	int a, i, d[2], m = 0;
	int active = 0;
	int ret;

	c = &at->name[strlen("active.")];
	a = kstrtoul(c, 10, NULL);

	/* for (i = 0; 2 > i; i++) { */
	for (i = 0; 1 > i; i++) {
		info = get_module_to_info(i);
		dev = info->proc_dev;
		d[i] = dev->dev_in;
	}

	/* not fb */
	if (d[0] != DISP_DEVICE_END && d[1] != DISP_DEVICE_END)
		m = a;
	else			/* 1 fb */
		m = (d[0] == a) ? 0 : 1;

	info = get_module_to_info(m);

	/* sscanf(buf, "%d", &active); */
	ret = kstrtoint(buf, 0, &active);

	info->active_notify = active ? 1 : 0;

	return n;
}

static struct device_attribute active0_attr =
__ATTR(active .0, 0664, active_show, active_store);
static struct device_attribute active1_attr =
__ATTR(active .1, 0664, active_show, active_store);

/*
 * Notify vertical sync timestamp
 *
 * /sys/devices/platform/display/vsync.N
 */
static ssize_t vsync_show(struct device *pdev,
				struct device_attribute *attr, char *buf)
{
	struct attribute *at = &attr->attr;
	struct disp_control_info *info;
	struct disp_process_dev *dev;
	const char *c;
	int a, i, d[2], m = 0;

	c = &at->name[strlen("vsync.")];
	a = kstrtoul(c, 10, NULL);

	/* for (i = 0; 2 > i; i++) { */
	for (i = 0; 1 > i; i++) {
		info = get_module_to_info(i);
		dev = info->proc_dev;
		d[i] = dev->dev_in;
	}

	/* not fb */
	if (d[0] != DISP_DEVICE_END && d[1] != DISP_DEVICE_END)
		m = a;
	else			/* 1 fb */
		m = (d[0] == a) ? 0 : 1;

	info = get_module_to_info(m);

	return scnprintf(buf, PAGE_SIZE, "%llu\n",
			 ktime_to_ns(info->time_stamp));
}

static struct device_attribute vblank0_attr =
		__ATTR(vsync .0, S_IRUGO | S_IWUSR, vsync_show, NULL);
static struct device_attribute vblank1_attr =
		__ATTR(vsync .1, S_IRUGO | S_IWUSR, vsync_show, NULL);

/*
 * Notify vertical sync timestamp
 *
 * /sys/devices/platform/display/fps.N
 */
static ssize_t fps_show(struct device *pdev,
				struct device_attribute *attr, char *buf)
{
	struct attribute *at = &attr->attr;
	struct disp_control_info *info;
	struct disp_process_dev *dev;
	const char *c;
	int a, i, d[2], m = 0;

	c = &at->name[strlen("vsync.")];
	a = kstrtoul(c, 10, NULL);

	for (i = 0; 2 > i; i++) {
		info = get_module_to_info(i);
		dev = info->proc_dev;
		d[i] = dev->dev_in;
	}

	/* not fb */
	if (d[0] != DISP_DEVICE_END && d[1] != DISP_DEVICE_END)
		m = a;
	else			/* 1 fb */
		m = (d[0] == a) ? 0 : 1;

	info = get_module_to_info(m);

	return scnprintf(buf, PAGE_SIZE, "%ld.%3ld\n", info->fps / 1000,
			 info->fps % 1000);
}

static struct device_attribute fps0_attr =
		__ATTR(fps .0, S_IRUGO | S_IWUSR, fps_show, NULL);
static struct device_attribute fps1_attr =
		__ATTR(fps .1, S_IRUGO | S_IWUSR, fps_show, NULL);

/* sys attribte group */
static struct attribute *attrs[] = {
	&active0_attr.attr, &active1_attr.attr,
	&vblank0_attr.attr, &vblank1_attr.attr,
	&fps0_attr.attr, &fps1_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = (struct attribute **)attrs,
};

static int display_soc_setup(int module, struct disp_process_dev *pdev,
				struct platform_device *pldev)
{
	struct disp_control_info *info = pdev->dev_info;
	struct disp_multily_dev *pmly = &info->multilayer;
	struct mlc_layer_info *layer = pmly->layer;
	int i = 0, ret;

	RET_ASSERT_VAL(info, -EINVAL)

	for (i = 0; MULTI_LAYER_NUM > i; i++, layer++) {
		if (LAYER_RGB_NUM > i) {
			sprintf(layer->name, "rgb.%d.%d", module, i);
		} else {
			/* video layers color */
			sprintf(layer->name, "vid.%d.%d", module, i);
			layer->color.alpha = 15;
			layer->color.bright = 0;
			layer->color.contrast = 0;
			layer->color.satura = 0;
			/* NOTE>
			 * hue and saturation type is double (float point)
			 * so set after enable VFP
			 */
		}
	}

	info->kobj = get_display_kobj(pdev->dev_id);
	info->module = module;
	info->wait_time = 4;
	info->condition = 0;
	info->irqno = platform_get_irq(pldev, 0);
	INIT_WORK(&info->work, disp_syncgen_irq_work);

	init_waitqueue_head(&info->wait_queue);
	INIT_LIST_HEAD(&info->callback_list);
	spin_lock_init(&info->lock_callback);

	ret = request_irq(info->irqno, &disp_syncgen_irqhandler,
			  0, DEV_NAME_DISP, info);
	if (ret) {
		pr_err("fail, display.%d request interrupt %d ...\n",
		       info->module, info->irqno);
		return ret;
	}

	/* irq enable */
	disp_syncgen_irqenable(info->module, 1);

	/* init status */
	pmly->enable = nx_mlc_get_mlc_enable(module) ? 1 : 0;
	pdev->status = nx_dpc_get_dpc_enable(module) ? PROC_STATUS_ENABLE : 0;

	return ret;
}

static int display_soc_resume(struct platform_device *pldev)
{
	struct disp_process_dev *pdev = platform_get_drvdata(pldev);
	struct disp_control_info *info = pdev->dev_info;
	int module = info->module;

	pr_debug("%s [%d]\n", __func__, module);

	if (pdev->rstc)
		reset_control_reset(pdev->rstc);

	if (pdev->rstc2)
		reset_control_reset(pdev->rstc2);

	nx_mlc_set_clock_pclk_mode(module, nx_pclkmode_always);
	nx_mlc_set_clock_bclk_mode(module, nx_bclkmode_always);
	nx_dpc_set_clock_pclk_mode(module, nx_pclkmode_always);

	return 0;
}

/* suspend save register */
static int display_soc_probe(struct platform_device *pldev)
{
	struct disp_control_info *info;
	struct disp_process_dev *pdev = NULL;
	struct disp_multily_dev *pmly = NULL;
	struct disp_sync_par *pspar = NULL;
	int module = pldev->id;
	int size = sizeof(*info);
	int ret = 0;

#ifdef CONFIG_OF
	if (pldev->dev.of_node) {
		ret = of_property_read_u32(pldev->dev.of_node,
				"module", &module);
		RET_ASSERT_VAL(0 == ret, -EINVAL);
	}
#endif
	RET_ASSERT_VAL(0 == module || 1 == module, -EINVAL);

	size += sizeof(struct disp_sync_par);
	info = kzalloc(size, GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	INIT_LIST_HEAD(&info->link);

	/* set syncgen device */
	pdev = get_display_ptr(DISP_DEVICE_SYNCGEN0 + module);
	list_add(&pdev->list, &info->link);

	pdev->rstc = devm_reset_control_get(&pldev->dev, "rsc-display");
	pdev->rstc2 = devm_reset_control_get(&pldev->dev, "rsc-display-top");

	/* reset and clock */
	disp_syncgen_initialize(pdev);

	pdev->dev_in = DISP_DEVICE_END;
	pdev->dev_out = DISP_DEVICE_END;
	pdev->dev_info = (void *)info;
	pdev->dev_id = DISP_DEVICE_SYNCGEN0 + module;
	pdev->disp_ops = &syncgen_ops[module];
	pdev->base_addr = (unsigned long)nx_dpc_get_base_address(module);
	pdev->save_addr = (unsigned long)&save_syncgen[module];
	pdev->disp_ops->dev = pdev;

	pspar = &pdev->sync_par;
	pspar->interlace = DEF_MLC_INTERLACE;
	pspar->out_format = DEF_OUT_FORMAT;
	pspar->invert_field = DEF_OUT_INVERT_FIELD;
	pspar->swap_RB = DEF_OUT_SWAPRB;
	pspar->yc_order = DEF_OUT_YCORDER;
	pspar->delay_mask = 0;
	pspar->vclk_select = DEF_PADCLKSEL;
	pspar->clk_delay_lv0 = DEF_CLKGEN0_DELAY;
	pspar->clk_inv_lv0 = DEF_CLKGEN0_INVERT;
	pspar->clk_delay_lv1 = DEF_CLKGEN1_DELAY;
	pspar->clk_inv_lv1 = DEF_CLKGEN1_INVERT;
	pspar->clk_sel_div1 = DEF_CLKSEL1_SELECT;

    /* set multilayer device */
	pmly = &info->multilayer;

	pmly->base_addr = (unsigned long)nx_mlc_get_base_address(module);
	pmly->save_addr = (unsigned long)&save_multily[module];
	pmly->mem_lock_len = 16;

	/* set control info */
	ret = display_soc_setup(module, pdev, pldev);
	if (0 > ret) {
		kfree(info);
		return ret;
	}

	info->proc_dev = pdev;
	display_info[module] = info;

	/* set operation */
	set_display_ops(pdev->dev_id, pdev->disp_ops);

	platform_set_drvdata(pldev, pdev);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id display_dt_match[] = {
	{.compatible = "nexell,dp-s5p6818",},
	{},
};

MODULE_DEVICE_TABLE(of, display_dt_match);
#else
#define display_dt_match NULL
#endif

static struct platform_driver disp_driver = {
	.probe = display_soc_probe,
	.resume = display_soc_resume,
	.driver = {
		   .name = DEV_NAME_DISP,
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(display_dt_match),
		   },
};

static int __init display_soc_initcall(void)
{
	struct kobject *kobj = NULL;
	int ret = 0;

	/* create attribute interface */
	kobj = kobject_create_and_add("display", &platform_bus.kobj);
	if (!kobj) {
		pr_err("fail, create kobject for display\n");
		return -ret;
	}

	ret = sysfs_create_group(kobj, &attr_group);
	if (ret) {
		pr_err("fail, create sysfs group for display\n");
		kobject_del(kobj);
		return -ret;
	}
	set_display_kobj(kobj);

	return platform_driver_register(&disp_driver);
}
subsys_initcall(display_soc_initcall);

MODULE_AUTHOR("jhkim <jhkim@nexell.co.kr>");
MODULE_DESCRIPTION("Display core driver for the Nexell");
MODULE_LICENSE("GPL");
