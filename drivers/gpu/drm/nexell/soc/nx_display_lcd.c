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
#include <linux/of.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include "nx_display.h"

/*
#define pr_debug(msg...)		printk(msg);
*/

#define	DEV_NAME_LCD	"display-lcd"

static int lcd_set_vsync(struct disp_process_dev *pdev,
				struct disp_vsync_info *psync)
{
	RET_ASSERT_VAL(pdev && psync, -EINVAL);
	pr_debug("%s: %s\n", __func__, dev_to_str(pdev->dev_id));

	pdev->status |= PROC_STATUS_READY;
	memcpy(&pdev->vsync, psync, sizeof(*psync));

	return 0;
}

static int lcd_get_vsync(struct disp_process_dev *pdev,
				struct disp_vsync_info *psync)
{
	pr_debug("%s: %s\n", __func__, dev_to_str(pdev->dev_id));
	RET_ASSERT_VAL(pdev, -EINVAL);

	if (psync)
		memcpy(psync, &pdev->vsync, sizeof(*psync));

	return 0;
}

static int lcd_prepare(struct disp_process_dev *pdev)
{
	struct disp_lcd_param *plcd = pdev->dev_param;
	int input = pdev->dev_in;
	int mpu = plcd->lcd_mpu_type;
	int rsc = 0, sel = 0;

	switch (input) {
	case DISP_DEVICE_SYNCGEN0:
		input = 0;
		break;
	case DISP_DEVICE_SYNCGEN1:
		input = 1;
		break;
	case DISP_DEVICE_RESCONV:
		input = 2;
		break;
	default:
		return -EINVAL;
	}

	switch (input) {
	case 0:
		sel = mpu ? 1 : 0;
		break;
	case 1:
		sel = rsc ? 3 : 2;
		break;
	default:
		pr_err("fail, %s nuknown module %d\n", __func__, input);
		return -1;
	}

	nx_disp_top_set_primary_mux(sel);
	return 0;
}

static int lcd_enable(struct disp_process_dev *pdev, int enable)
{
	pr_debug("%s %s, %s\n", __func__, dev_to_str(pdev->dev_id),
		 enable ? "ON" : "OFF");

	if (!enable) {
		pdev->status &= ~PROC_STATUS_ENABLE;
	} else {
		lcd_prepare(pdev);
		pdev->status |= PROC_STATUS_ENABLE;
	}
	return 0;
}

static int lcd_stat_enable(struct disp_process_dev *pdev)
{
	int ret = 0;

	switch (pdev->dev_in) {
	case DISP_DEVICE_SYNCGEN0:
		ret = nx_dpc_get_dpc_enable(0);
		break;
	case DISP_DEVICE_SYNCGEN1:
		ret = nx_dpc_get_dpc_enable(1);
		break;
	case DISP_DEVICE_RESCONV:
		break;
	default:
		break;
	}

	if (ret)
		pdev->status |= PROC_STATUS_ENABLE;
	else
		pdev->status &= ~PROC_STATUS_ENABLE;

	return pdev->status & PROC_STATUS_ENABLE ? 1 : 0;
}

static int lcd_suspend(struct disp_process_dev *pdev)
{
	pr_debug("%s\n", __func__);
	return lcd_enable(pdev, 0);
}

static void lcd_resume(struct disp_process_dev *pdev)
{
	pr_debug("%s\n", __func__);
	lcd_enable(pdev, 1);
}

static struct disp_process_ops lcd_ops = {
	.set_vsync = lcd_set_vsync,
	.get_vsync = lcd_get_vsync,
	.enable = lcd_enable,
	.stat_enable = lcd_stat_enable,
	.suspend = lcd_suspend,
	.resume = lcd_resume,
};

#ifdef CONFIG_OF
static const struct of_device_id lcd_dt_match[];

static int lcd_get_dt_data(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct device_node *np = node;
	const struct of_device_id *match;
	struct nx_lcd_plat_data *plat;
	struct disp_vsync_info *psync;

	match = of_match_node(lcd_dt_match, pdev->dev.of_node);
	RET_ASSERT_VAL(match, -EINVAL);

	plat = (struct nx_lcd_plat_data *)match->data;
	psync = plat->vsync;

	of_property_read_u32(np, "display_in", &plat->display_in);

	np = of_get_child_by_name(node, "vsync");
	RET_ASSERT_VAL(np, -EINVAL);

	of_property_read_u32(np, "interlace", &psync->interlace);
	of_property_read_u32(np, "hor_active_size", &psync->h_active_len);
	of_property_read_u32(np, "hor_sync_width", &psync->h_sync_width);
	of_property_read_u32(np, "hor_back_porch", &psync->h_back_porch);
	of_property_read_u32(np, "hor_front_porch", &psync->h_front_porch);
	of_property_read_u32(np, "hor_sync_invert", &psync->h_sync_invert);
	of_property_read_u32(np, "ver_active_size", &psync->v_active_len);
	of_property_read_u32(np, "ver_sync_width", &psync->v_sync_width);
	of_property_read_u32(np, "ver_back_porch", &psync->v_back_porch);
	of_property_read_u32(np, "ver_front_porch", &psync->v_front_porch);
	of_property_read_u32(np, "ver_sync_invert", &psync->v_sync_invert);
	of_property_read_u32(np, "pixel_clock_hz", &psync->pixel_clock_hz);

	np = of_get_child_by_name(node, "clock");
	RET_ASSERT_VAL(np, -EINVAL);

	of_property_read_u32(np, "clock,src_div0", &psync->clk_src_lv0);
	of_property_read_u32(np, "clock,div_val0", &psync->clk_div_lv0);
	of_property_read_u32(np, "clock,src_div1", &psync->clk_src_lv1);
	of_property_read_u32(np, "clock,div_val1", &psync->clk_div_lv1);

	/* TO DO for LCD parameters
	   np = of_get_child_by_name(node, "lcd");
	 */

	pdev->dev.platform_data = plat;

	return 0;
}
#endif

static int lcd_probe(struct platform_device *pdev)
{
	struct nx_lcd_plat_data *plat = pdev->dev.platform_data;
	struct disp_lcd_param *plcd = NULL;
	void *base = NULL;
	int device = DISP_DEVICE_LCD;
	int ret = 0, input;

#ifdef CONFIG_OF
	if (pdev->dev.of_node) {
		ret = lcd_get_dt_data(pdev);
		RET_ASSERT_VAL(0 == ret, -EINVAL);
	}
	plat = pdev->dev.platform_data;
#endif

	RET_ASSERT_VAL(plat, -EINVAL);
	RET_ASSERT_VAL(plat->display_in == DISP_DEVICE_SYNCGEN0 ||
		       plat->display_in == DISP_DEVICE_SYNCGEN1 ||
		       plat->display_dev == DISP_DEVICE_LCD ||
		       plat->display_in == DISP_DEVICE_RESCONV, -EINVAL);
	RET_ASSERT_VAL(plat->vsync, -EINVAL);

	if (plat->dev_param) {
		plcd = kzalloc(sizeof(*plcd), GFP_KERNEL);
		RET_ASSERT_VAL(plcd, -EINVAL);
		memcpy(plcd, plat->dev_param, sizeof(*plcd));
	}

	input = plat->display_in;

	nx_soc_disp_register_proc_ops(device, &lcd_ops);
	nx_soc_disp_device_connect_to(device, input, plat->vsync);

	if (plcd)
		nx_soc_disp_device_set_dev_param(device, plcd);

	if (plat->sync_par &&
	    (input == DISP_DEVICE_SYNCGEN0 || input == DISP_DEVICE_SYNCGEN1))
		nx_soc_disp_device_set_sync_param(input, plat->sync_par);

	base = ioremap(nx_disp_top_get_physical_address(), PAGE_SIZE);
	nx_disp_top_set_base_address(base);

	pr_info("LCD : [%d]=%s connect to [%d]=%s\n",
	       device, dev_to_str(device), input, dev_to_str(input));

	return ret;
}

#ifdef CONFIG_OF
static struct disp_vsync_info lcd_vsync;
static struct disp_lcd_param lcd_param;

static struct nx_lcd_plat_data lcd_dt_data = {
	.display_dev = DISP_DEVICE_LCD,
	.vsync = &lcd_vsync,
	.dev_param = (union disp_dev_param *)&lcd_param,
};

static const struct of_device_id lcd_dt_match[] = {
	{
	 .compatible = "nexell,dp-s5p6818-lcd",
	 .data = (void *)&lcd_dt_data,
	 }, {},
};
MODULE_DEVICE_TABLE(of, lcd_dt_match);

#else
#define lcd_dt_match NULL
#endif

static struct platform_driver lcd_driver = {
	.probe = lcd_probe,
	.driver = {
		   .name = DEV_NAME_LCD,
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(lcd_dt_match),
		   },
};

static int __init lcd_initcall(void)
{
	return platform_driver_register(&lcd_driver);
}

subsys_initcall(lcd_initcall);

MODULE_AUTHOR("jhkim <jhkim@nexell.co.kr>");
MODULE_DESCRIPTION("Display RGB driver for the Nexell");
MODULE_LICENSE("GPL");
