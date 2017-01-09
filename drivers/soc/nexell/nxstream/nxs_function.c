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
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/v4l2-mediabus.h>
#include <linux/regulator/consumer.h>

#include <media/v4l2-async.h>
#include <media/v4l2-of.h>

#include <dt-bindings/media/nexell-vip.h>
#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_res_manager.h>

struct nxs_display {
	struct list_head list; /* connect to nxs_disp_list */
	struct list_head func_list; /* head of nxs_function->disp_list */
	struct list_head vert_list; /* head of nxs_dev->disp_list */
	struct nxs_dev *vert_bottom; /* bottom dev for screen */
	struct nxs_function *hori_top; /* top horizontal instance */
};

static LIST_HEAD(nxs_func_list);
static DEFINE_MUTEX(nxs_func_lock);
static int nxs_func_seq;

static LIST_HEAD(nxs_disp_list);
static DEFINE_MUTEX(nxs_disp_lock);

static const char *nxs_function_str[] = {
	"NONE",
	"DMAR",
	"DMAW",
	"VIP_CLIPPER",
	"VIP_DECIMATOR",
	"MIPI_CSI",
	"ISP2DISP",
	"CROPPER",
	"MULTI_TAP",
	"SCALER_4096",
	"SCALER_5376",
	"MLC_BOTTOM",
	"MLC_BLENDER",
	"HUE",
	"GAMMA",
	"FIFO",
	"MAPCONV",
	"TPGEN",
	"DISP2ISP",
	"CSC",
	"DPC",
	"LVDS",
	"MIPI_DSI",
	"HDMI",
	"ISP",
	"INVALID",
};

inline const char *nxs_function_to_str(int function)
{
	if (function < NXS_FUNCTION_MAX)
		return nxs_function_str[function];
	return NULL;
}

static void nxs_function_print(struct nxs_function *f)
{
	struct nxs_dev *dev;

	pr_info("Function %p -> id %d, type 0x%x, disp %p\n", f, f->id, f->type,
		f->disp);
	list_for_each_entry(dev, &f->dev_list, func_list)
		nxs_dev_print(dev, NULL);
	pr_info("\n");
}

static void nxs_display_print(struct nxs_display *d)
{
	struct nxs_function *f;
	struct nxs_dev *dev;

	pr_info("============================\n");
	pr_info("Display %p\n", d);
	pr_info("============================\n");
	pr_info("Horizontal Path\n");
	list_for_each_entry(f, &d->func_list, disp_list)
		nxs_function_print(f);
	pr_info("Vertical Path\n");
	list_for_each_entry(dev, &d->vert_list, disp_list)
		nxs_dev_print(dev, NULL);
	pr_info("\n");
}

static int apply_ep_to_capture(struct device *dev, struct v4l2_of_endpoint *sep,
			       struct nxs_capture_ctx *capture)
{
	capture->bus_type = sep->bus_type;

	if (sep->bus_type == V4L2_MBUS_PARALLEL ||
	    sep->bus_type == V4L2_MBUS_BT656) {
		struct nxs_capture_hw_vip_param *param = &capture->bus.parallel;
		struct v4l2_of_bus_parallel *sep_param = &sep->bus.parallel;

		param->bus_width = sep_param->bus_width;
		param->interlace = sep_param->flags &
			(V4L2_MBUS_FIELD_EVEN_HIGH | V4L2_MBUS_FIELD_EVEN_LOW) ?
			1 : 0;
		param->pclk_polarity = sep_param->flags &
			V4L2_MBUS_PCLK_SAMPLE_RISING ? 1 : 0;
		param->hsync_polarity = sep_param->flags &
			V4L2_MBUS_HSYNC_ACTIVE_HIGH ? 1 : 0;
		param->vsync_polarity = sep_param->flags &
			V4L2_MBUS_VSYNC_ACTIVE_HIGH ? 1 : 0;
		param->field_polarity = sep_param->flags &
			V4L2_MBUS_FIELD_EVEN_HIGH ? 1 : 0;
	} else if (sep->bus_type == V4L2_MBUS_CSI2) {
		struct nxs_capture_hw_csi_param *param = &capture->bus.csi;
		struct v4l2_of_bus_mipi_csi2 *sep_param = &sep->bus.mipi_csi2;
		int i;

		param->num_data_lanes = sep_param->num_data_lanes;
		for (i = 0; i < param->num_data_lanes; i++)
			param->data_lanes[i] = sep_param->data_lanes[i];
		param->clock_lane = sep_param->clock_lane;
		for (i = 0; i < 5; i++)
			param->lane_polarities[i] =
				sep_param->lane_polarities[i];
		if (sep->nr_of_link_frequencies)
			param->link_frequency = sep->link_frequencies[0];
	} else {
		dev_err(dev, "%s: invalid bus type %d\n", __func__,
			sep->bus_type);
		return -EINVAL;
	}

	return 0;
}

static int find_action_mark(u32 *p, int length, u32 mark)
{
	int i;

	for (i = 0; i < length; i++) {
		if (p[i] == mark)
			return i;
	}
	return -1;
}

static int find_action_start(u32 *p, int length)
{
	return find_action_mark(p, length, NX_ACTION_START);
}

static int find_action_end(u32 *p, int length)
{
	return find_action_mark(p, length, NX_ACTION_END);
}

static int get_num_of_action(struct device *dev, u32 *array, int count)
{
	u32 *p = array;
	int action_num = 0;
	int next_index = 0;
	int length = count;

	while (length > 0) {
		next_index = find_action_start(p, length);
		if (next_index < 0)
			break;
		p += next_index;
		length -= next_index;
		if (length <= 0)
			break;

		next_index = find_action_end(p, length);
		if (next_index <= 0) {
			dev_err(dev, "failed to find_action_end\n");
			return 0;
		}

		p += next_index;
		length -= next_index;
		action_num++;
	}

	return action_num;
}

static u32 *get_next_action(u32 *array, int count)
{
	u32 *p = array;
	int next_index = find_action_start(p, count);

	if (next_index >= 0)
		return p + next_index;
	return NULL;
}

static u32 get_action_type(u32 *array)
{
	return array[1];
}

static void free_action_seq(struct nxs_action_seq *seq)
{
	int i;
	struct nxs_action *action;

	for (i = 0; i < seq->count; i++) {
		action = &seq->actions[i];
		if (action->action) {
			if (action->type == NX_ACTION_TYPE_GPIO) {
				struct nxs_gpio_action *gpio_action =
					action->action;

				if (gpio_action->units)
					kfree(gpio_action->units);
			}
			kfree(action->action);
		}
	}

	kfree(seq->actions);
	seq->count = 0;
	seq->actions = NULL;
}

static int make_gpio_action(struct device *dev, u32 *start, u32 *end,
			    struct nxs_action *action)
{
	struct nxs_gpio_action *gpio_action;
	struct nxs_action_unit *unit;
	int i;
	u32 *p;
	/* start_marker, type, gpio num */
	int unit_count = end - start - 1 - 1 - 1;

	if ((unit_count <= 0) || (unit_count % 2)) {
		dev_err(dev, "%s: invalid unit_count %d\n", __func__,
			unit_count);
		return -EINVAL;
	}
	unit_count /= 2;

	gpio_action = kzalloc(sizeof(*gpio_action), GFP_KERNEL);
	if (!gpio_action) {
		WARN_ON(1);
		return -ENOMEM;
	}

	gpio_action->count = unit_count;
	gpio_action->units = kcalloc(unit_count, sizeof(*unit), GFP_KERNEL);
	if (!gpio_action->units) {
		WARN_ON(1);
		kfree(gpio_action);
		return -ENOMEM;
	}

	gpio_action->gpio_num = start[2];
	p = &start[3];
	for (i = 0; i < unit_count; i++) {
		unit = &gpio_action->units[i];
		unit->value = *p;
		p++;
		unit->delay_ms = *p;
		p++;
	}

	action->type = start[1];
	action->action = gpio_action;

	return 0;
}

static int make_generic_action(struct device *dev, u32 *start, u32 *end,
			       struct nxs_action *action)
{
	struct nxs_action_unit *unit;
	/* start_marker, type */
	int unit_count = end - start - 1 - 1;

	if ((unit_count <= 0) || (unit_count % 2)) {
		dev_err(dev, "%s: invalid unit_count %d\n", __func__,
			unit_count);
		return -EINVAL;
	}

	unit = kzalloc(sizeof(*unit), GFP_KERNEL);
	if (!unit) {
		WARN_ON(1);
		return -ENOMEM;
	}

	unit->value = start[2];
	unit->delay_ms = start[3];

	action->type = start[1];
	action->action = unit;

	return 0;
}

static int make_nxs_action(struct device *dev, u32 *array, int count,
			   struct nxs_action *action)
{
	u32 *p = array;
	int end_index = find_action_end(p, count);

	if (end_index <= 0) {
		dev_err(dev, "%s: can't find action end\n", __func__);
		return -EINVAL;
	}

	switch (get_action_type(p)) {
	case NX_ACTION_TYPE_GPIO:
		return make_gpio_action(dev, p, p + end_index, action);
	case NX_ACTION_TYPE_PMIC:
	case NX_ACTION_TYPE_CLOCK:
		return make_generic_action(dev, p, p + end_index, action);
	default:
		dev_err(dev, "%s: incalid type 0x%x\n", __func__,
			get_action_type(p));
		break;
	}

	return -EINVAL;
}

static int parse_power_seq(struct device *dev, struct device_node *node,
			   char *node_name, struct nxs_action_seq *seq)
{
	int count = of_property_count_elems_of_size(node, node_name, 4);

	if (count > 0) {
		u32 *p;
		int ret = 0;
		struct nxs_action *action;
		int action_nums;
		u32 *array = kcalloc(count, sizeof(u32), GFP_KERNEL);

		if (!array) {
			WARN_ON(1);
			return -ENOMEM;
		}

		of_property_read_u32_array(node, node_name, array, count);
		action_nums = get_num_of_action(dev, array, count);
		if (action_nums <= 0) {
			dev_err(dev, "%s: no actions in %s\n", __func__,
				node_name);
			return -ENOENT;
		}

		seq->actions = kcalloc(count, sizeof(*action), GFP_KERNEL);
		if (!seq->actions) {
			WARN_ON(1);
			return -ENOMEM;
		}
		seq->count = action_nums;

		p = array;
		action = seq->actions;
		while (action_nums--) {
			p = get_next_action(p, count - (p - array));
			if (!p) {
				dev_err(dev, "failed to get_next_action(%d/%d)\n",
					seq->count, action_nums);
				free_action_seq(seq);
				return -EINVAL;
			}

			ret = make_nxs_action(dev, p, count - (p - array),
					      action);
			if (ret != 0) {
				free_action_seq(seq);
				return ret;
			}

			p++;
			action++;
		}
	}

	return 0;
}

static int parse_capture_dts(struct device *dev,
			     struct nxs_capture_ctx *capture,
			     struct device_node *node)
{
	int ret;

	if (capture->bus_type == V4L2_MBUS_PARALLEL ||
	    capture->bus_type == V4L2_MBUS_BT656) {
		struct nxs_capture_hw_vip_param *param = &capture->bus.parallel;

		if (of_property_read_u32(node, "data_order",
					 &param->data_order)) {
			dev_err(dev, "failed to get dt data_order\n");
			return -EINVAL;
		}

		if (param->bus_width > 8) {
			/* bus_align must be exist: LSB 1, MSB 0 */
			if (of_property_read_u32(node, "bus_align",
						 &param->bus_align)) {
				dev_err(dev, "failed to get dt bus_align\n");
				return -EINVAL;
			}
		}

		if (capture->bus_type == V4L2_MBUS_PARALLEL) {
			if (of_property_read_u32(node, "h_backporch",
						 &param->h_backporch)) {
				dev_err(dev, "failed to get dt h_backporch\n");
				return -EINVAL;
			}

			if (of_property_read_u32(node, "v_backporch",
						 &param->v_backporch)) {
				dev_err(dev, "failed to get dt v_backporch\n");
				return -EINVAL;
			}
		}
	}

	/* regulator */
	capture->regulator_nr = of_property_count_strings(node, "regulator_names");
	if (capture->regulator_nr > 0) {
		int i;
		const char *name;

		capture->regulator_names = kcalloc(capture->regulator_nr,
						   sizeof(char *),
						   GFP_KERNEL);
		if (!capture->regulator_names) {
			WARN_ON(1);
			return -ENOMEM;
		}

		capture->regulator_voltages = kcalloc(capture->regulator_nr,
						      sizeof(u32),
						      GFP_KERNEL);
		if (!capture->regulator_voltages) {
			WARN_ON(1);
			return -ENOMEM;
		}

		for (i = 0; i < capture->regulator_nr; i++) {
			if (of_property_read_string_index(node, "regulator_names",
							  i, &name)) {
				dev_err(dev, "failed to read regulator %d name\n", i);
				return -EINVAL;
			}
			capture->regulator_names[i] = (char *)name;
		}

		of_property_read_u32_array(node, "regulator_voltages",
					   capture->regulator_voltages,
					   capture->regulator_nr);
	}

	/* clock */
	capture->clk = clk_get(dev, "vclkout");
	if (!IS_ERR(capture->clk)) {
		if (of_property_read_u32(node, "clock-frequency",
					 &capture->clock_freq)) {
			dev_err(dev, "failed to get clock-frequency\n");
			return -EINVAL;
		}
	}

	/* enable seq */
	if (of_find_property(node, "enable_seq", NULL)) {
		ret = parse_power_seq(dev, node, "enable_seq",
				      &capture->enable_seq);
		if (ret)
			return ret;
	}

	/* disable seq */
	if (of_find_property(node, "disable_seq", NULL)) {
		ret = parse_power_seq(dev, node, "disable_seq",
				      &capture->enable_seq);
		if (ret)
			return ret;
	}

	return 0;
}

static int apply_gpio_action(struct device *dev, int gpio_num,
			     struct nxs_action_unit *unit)
{
	int ret;
	char label[64] = {0, };
	struct device_node *node;
	int gpio;

	node = dev->of_node;
	gpio = of_get_named_gpio(node, "gpios", gpio_num);

	sprintf(label, "nxs-v4l2 camera #pwr gpio %d", gpio);
	if (!gpio_is_valid(gpio)) {
		dev_err(dev, "%s: invalid gpio %d\n", __func__, gpio);
		return -EINVAL;
	}

	ret = devm_gpio_request_one(dev, gpio, unit->value ?
				    GPIOF_OUT_INIT_HIGH : GPIOF_OUT_INIT_LOW,
				    label);
	if (ret < 0) {
		dev_err(dev, "%s: failed to set gpio %d to %d\n",
			__func__, gpio, unit->value);
		return ret;
	}

	if (unit->delay_ms > 0)
		mdelay(unit->delay_ms);

	devm_gpio_free(dev, gpio);

	return 0;
}

static int do_gpio_action(struct device *dev, struct nxs_capture_ctx *capture,
			  struct nxs_gpio_action *action)
{
	int ret;
	struct nxs_action_unit *unit;
	int i;

	for (i = 0; i < action->count; i++) {
		unit = &action->units[i];
		ret = apply_gpio_action(dev, action->gpio_num, unit);
		/* test comment out */
		/* if (ret) */
		/* 	return ret; */
	}

	return 0;
}

static int do_pmic_action(struct device *dev, struct nxs_capture_ctx *capture,
			  struct nxs_action_unit *unit)
{
	int ret;
	int i;
	struct regulator *power;

	for (i = 0; i < capture->regulator_nr; i++) {
		power = devm_regulator_get(dev, capture->regulator_names[i]);
		if (IS_ERR(power)) {
			dev_err(dev, "%s: failed to get power %s\n",
				__func__, capture->regulator_names[i]);
			return -ENODEV;
		}

		if (regulator_can_change_voltage(power)) {
			ret = regulator_set_voltage(power,
						capture->regulator_voltages[i],
						capture->regulator_voltages[i]);
			if (ret) {
				devm_regulator_put(power);
				dev_err(dev, "%s: failed to set voltages for %s\n",
					__func__, capture->regulator_names[i]);
			}
		}

		ret = 0;
		if (unit->value && !regulator_is_enabled(power))
			ret = regulator_enable(power);
		else if (!unit->value && regulator_is_enabled(power))
			ret = regulator_disable(power);

		devm_regulator_put(power);
		if (ret) {
			dev_err(dev, "%s: failed to power %s to %s\n",
				__func__, capture->regulator_names[i],
				unit->value ? "enable" : "disable");
			return ret;
		}

		if (unit->delay_ms > 0)
			mdelay(unit->delay_ms);
	}

	return 0;
}

static int do_clock_action(struct device *dev, struct nxs_capture_ctx *capture,
			   struct nxs_action_unit *unit)
{
	int ret = 0;

	if (capture->clk) {
		if (unit->value)
			ret = clk_prepare_enable(capture->clk);
		else
			clk_disable_unprepare(capture->clk);

		if (ret) {
			dev_err(dev, "%s: failed to clk control\n", __func__);
			return ret;
		}

		if (unit->delay_ms > 0)
			mdelay(unit->delay_ms);
	}

	return 0;
}

static bool is_display_channel(struct nxs_function *f)
{
	struct nxs_dev *nxs_dev;

	nxs_dev = list_last_entry(&f->dev_list, struct nxs_dev,
				  func_list);
	/* pr_info("%s: function %p, nxs_dev [%s:%d]\n", __func__, f, */
	/* 	nxs_function_to_str(nxs_dev->dev_function), */
	/* 	nxs_dev->dev_inst_index); */
	switch (nxs_dev->dev_function) {
	case NXS_FUNCTION_DPC:
	case NXS_FUNCTION_LVDS:
	case NXS_FUNCTION_MIPI_DSI:
	case NXS_FUNCTION_HDMI:
		return true;
	}

	return false;
}

static struct nxs_dev *get_bottom_dev(struct nxs_function *f)
{
	struct nxs_dev *cur, *prev;
	bool found = false;

	prev = NULL;
	list_for_each_entry(cur, &f->dev_list, func_list) {
		if (cur->dev_function == NXS_FUNCTION_FIFO) {
			found = true;
			break;
		}
		prev = cur;
	}

	if (found)
		return prev;

	return NULL;
}

static struct nxs_display *find_display_by_function(struct nxs_function *f)
{
	struct nxs_display *disp;
	struct nxs_function *func;

	list_for_each_entry(disp, &nxs_disp_list, list) {
		list_for_each_entry(func, &disp->func_list, disp_list) {
			if (f == func)
				return disp;
		}
	}

	return NULL;
}

static int function_connect(struct nxs_function *f)
{
	struct nxs_dev *prev, *cur;
	int ret;

	prev = NULL;
	list_for_each_entry(cur, &f->dev_list, func_list) {
		if (prev) {
			if (WARN(!prev->set_tid,
				 "no set_tid func for [%s:%d]\n",
				 nxs_function_to_str(prev->dev_function),
				 prev->dev_inst_index))
				return -EINVAL;

			/* TODO: handle multitap */
			ret = prev->set_tid(prev, cur->tid, 0);
			if (ret)
				return ret;
		}

		prev = cur;
	}

	return 0;
}

static int vertical_connect(struct nxs_display *d)
{
	struct nxs_dev *prev, *cur;
	int ret;

	prev = NULL;
	list_for_each_entry(cur, &d->vert_list, disp_list) {
		if (prev) {
			if (WARN(!prev->set_tid,
				 "no set_tid func for [%s:%d]\n",
				 nxs_function_to_str(prev->dev_function),
				 prev->dev_inst_index))
				return -EINVAL;

			/* TODO: handle multitap */
			ret = prev->set_tid(prev, cur->tid2, 0);
			if (ret)
				return ret;
		}

		prev = cur;
	}

	return 0;
}

static int nxs_display_connect(void *disp, struct nxs_function *f)
{
	int ret;
	struct nxs_display *d = disp;

	/* horizontal */
	if (f == d->hori_top) {
		ret = function_connect(f);
		if (ret)
			return ret;
	}

	/* vertical */
	return vertical_connect(d);
}

static int function_ready(struct nxs_function *f)
{
	struct nxs_dev *nxs_dev;
	int ret;

	list_for_each_entry_reverse(nxs_dev, &f->dev_list, func_list) {
		if (nxs_dev->set_dirty) {
			ret = nxs_dev->set_dirty(nxs_dev, NXS_DEV_DIRTY_NORMAL);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static int vertical_ready(struct nxs_display *d)
{
	struct nxs_dev *nxs_dev;
	int ret;

	list_for_each_entry_reverse(nxs_dev, &d->vert_list, disp_list) {
		if (nxs_dev->set_dirty) {
			ret = nxs_dev->set_dirty(nxs_dev, NXS_DEV_DIRTY_TID);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static int nxs_display_ready(void *disp, struct nxs_function *f)
{
	int ret;
	struct nxs_display *d = disp;

	/* vertical first */
	ret = vertical_ready(d);
	if (ret)
		return ret;

	/* horizontal */
	if (f == d->hori_top)
		return function_ready(f);

	return 0;
}

static struct nxs_display *find_display_by_index(int disp)
{
	struct nxs_display *d;
	int index = 0;
	bool found = false;

	list_for_each_entry(d, &nxs_disp_list, list) {
		if (disp == index) {
			found = true;
			break;
		}
		index++;
	}

	if (found)
		return d;

	return NULL;
}

static struct nxs_dev *find_joint_dev(struct nxs_function *f)
{
	struct nxs_dev *dev;

	list_for_each_entry(dev, &f->dev_list, func_list)
		if (dev->dev_function == NXS_FUNCTION_MLC_BLENDER)
			return dev;

	/* if can't find blender, dmar is joint dev */
	list_for_each_entry(dev, &f->dev_list, func_list)
		if (dev->dev_function == NXS_FUNCTION_DMAR)
			return dev;

	return NULL;
}

static struct nxs_dev *find_blender(struct nxs_function *f)
{
	struct nxs_dev *dev;

	list_for_each_entry(dev, &f->dev_list, func_list)
		if (dev->dev_function == NXS_FUNCTION_MLC_BLENDER)
			return dev;

	return NULL;
}

static int add_function_to_display(struct nxs_display *disp,
				   struct nxs_function *f)
{
	struct nxs_dev *joint, *joint_next;
	struct nxs_dev *last;
	struct nxs_dev *last_of_top;
	struct list_head cut;

	mutex_lock(&nxs_disp_lock);

	pr_info("%s: disp %p, function %d\n", __func__, disp, f->id);
	joint = find_joint_dev(disp->hori_top);
	joint_next = list_next_entry(joint, func_list);
	last_of_top = list_last_entry(&disp->hori_top->dev_list, struct nxs_dev,
				      func_list);
	last = list_last_entry(&f->dev_list, struct nxs_dev, func_list);

	/* nxs_dev_print(joint, "joint"); */
	/* nxs_dev_print(joint_next, "joint_next"); */
	/* nxs_dev_print(last_of_top, "last_of_top"); */
	/* nxs_dev_print(last, "last"); */

	/* connect last of f to next of joint */
	INIT_LIST_HEAD(&cut);
	list_cut_position(&cut, &joint->func_list,
			  &last_of_top->func_list);
	list_splice_tail(&cut, &f->dev_list);

	/* connect last_dev to vert_list */
	list_add_tail(&last->disp_list, &disp->vert_list);

	/* connect f to func_list */
	list_add_tail(&f->disp_list, &disp->func_list);

	/* update hori_top */
	disp->hori_top = f;

	f->disp = disp;

	nxs_display_print(disp);

	mutex_unlock(&nxs_disp_lock);

	return 0;
}

static int unregister_display(struct nxs_display *d)
{
	pr_info("%s: display %p\n", __func__, d);

	/* put bottom dev */
	if (d->vert_bottom &&
	    d->vert_bottom->dev_function == NXS_FUNCTION_MLC_BOTTOM) {
		put_nxs_dev(d->vert_bottom);
		pr_info("%s: put [%s:%d]\n", __func__,
			nxs_function_to_str(d->vert_bottom->dev_function),
			d->vert_bottom->dev_inst_index);
		d->vert_bottom = NULL;
	}

	/* disconnect from nxs_disp_list) */
	mutex_lock(&nxs_disp_lock);
	list_del_init(&d->list);
	mutex_unlock(&nxs_disp_lock);

	/* free */
	kfree(d);

	return 0;
}

static int remove_function_from_display(struct nxs_display *disp,
					struct nxs_function *f)
{
	bool is_f_top = false;
	struct nxs_function *prev;

	mutex_lock(&nxs_disp_lock);

	if (f == disp->hori_top)
		is_f_top = true;

	pr_info("%s: disp %p, function %d\n", __func__, disp, f->id);
	if (is_f_top) {
		struct nxs_dev *joint, *joint_next;
		struct nxs_dev *last;
		struct nxs_dev *last_of_top;
		struct list_head cut;
		struct nxs_function *first_function;

		first_function = list_first_entry(&disp->func_list,
						  struct nxs_function,
						  disp_list);
		if (first_function != f) {
			joint = find_joint_dev(f);
			joint_next = list_next_entry(joint, func_list);
			last_of_top = list_last_entry(&f->dev_list,
						      struct nxs_dev,
						      func_list);

			prev = list_prev_entry(f, disp_list);
			last = list_last_entry(&prev->dev_list, struct nxs_dev,
					       func_list);
			/* nxs_dev_print(last, "last"); */

			/* connect joint_next to last */
			INIT_LIST_HEAD(&cut);
			list_cut_position(&cut, &joint->func_list,
					  &last_of_top->func_list);
			list_splice(&cut, &last->func_list);

			/* disconnect joint from vert_list */
			list_del_init(&joint->disp_list);

			/* disconnect f from func_list */
			list_del_init(&f->disp_list);

			/* update hori_top */
			disp->hori_top = prev;
		} else {
			list_del_init(&f->disp_list);
		}
	} else {
		struct nxs_dev *last;

		last = list_last_entry(&f->dev_list, struct nxs_dev, func_list);

		/* disconnect last from vert_list */
		list_del_init(&last->disp_list);

		/* disconnect f from func_list */
		list_del_init(&f->disp_list);
	}

	nxs_display_print(disp);

	mutex_unlock(&nxs_disp_lock);

	if (list_empty(&disp->func_list))
		unregister_display(disp);

	return 0;
}

int nxs_capture_power(struct nxs_capture_ctx *capture, bool enable)
{
	struct nxs_action_seq *seq;
	int i;
	struct nxs_action *act;
	struct device *dev = capture->dev;
	int ret = 0;

	if (enable)
		seq = &capture->enable_seq;
	else
		seq = &capture->disable_seq;

	for (i = 0; i < seq->count; i++) {
		act = &seq->actions[i];
		switch (act->type) {
		case NX_ACTION_TYPE_GPIO:
			ret = do_gpio_action(dev, capture, act->action);
			if (ret)
				return ret;
			break;
		case NX_ACTION_TYPE_PMIC:
			ret = do_pmic_action(dev, capture, act->action);
			if (ret)
				return ret;
			break;
		case NX_ACTION_TYPE_CLOCK:
			ret = do_clock_action(dev, capture, act->action);
			if (ret)
				return ret;
			break;
		default:
			dev_err(dev, "unknown action type 0x%x\n", act->type);
			break;
		}
	}

	return ret;
}
EXPORT_SYMBOL_GPL(nxs_capture_power);

int nxs_capture_bind_sensor(struct device *dev, struct nxs_dev *nxs_dev,
			    struct v4l2_async_subdev *asd,
			    struct nxs_capture_ctx *capture)
{
	struct device_node *node = NULL;
	struct device_node *remote_parent_node = NULL, *remote_node = NULL;
	struct v4l2_of_endpoint vep, sep;
	int ret;

	node = of_graph_get_next_endpoint(nxs_dev->dev->of_node, NULL);
	if (!node) {
		dev_err(dev, "%s: can't find endpoint node\n", __func__);
		ret = -ENOENT;
		goto err_out;
	}

	ret = v4l2_of_parse_endpoint(node, &vep);
	if (ret) {
		dev_err(dev, "%s: failed to v4l2_of_parse_endpoint\n",
			__func__);
		goto err_out;
	}

	remote_parent_node = of_graph_get_remote_port_parent(node);
	if (!remote_parent_node) {
		dev_err(dev, "%s: can't find remote parent node\n", __func__);
		ret = -ENOENT;
		goto err_out;
	}

	remote_node = of_graph_get_next_endpoint(remote_parent_node, NULL);
	if (!remote_node) {
		dev_err(dev, "%s: can't find remote node\n", __func__);
		ret = -ENOENT;
		goto err_out;
	}

	ret = v4l2_of_parse_endpoint(remote_node, &sep);
	if (ret) {
		dev_err(dev,
			"%s: failed to v4l2_of_parse_endpoint for remote\n",
			__func__);
		goto err_out;
	}

	ret = apply_ep_to_capture(dev, &sep, capture);
	if (ret) {
		dev_err(dev, "%s: failed to apply_ep_to_capture\n", __func__);
		goto err_out;
	}

	ret = parse_capture_dts(dev, capture, node);
	if (ret)
		dev_err(dev, "%s: failed to parse_capture_dts\n", __func__);

	capture->dev = nxs_dev->dev;

err_out:
	if (remote_node)
		of_node_put(remote_node);

	if (remote_parent_node)
		of_node_put(remote_parent_node);

	if (node)
		of_node_put(node);

	return ret;
}
EXPORT_SYMBOL_GPL(nxs_capture_bind_sensor);

void nxs_capture_free(struct nxs_capture_ctx *capture)
{
	free_action_seq(&capture->enable_seq);
	free_action_seq(&capture->disable_seq);
	kfree(capture);
}
EXPORT_SYMBOL_GPL(nxs_capture_free);

void nxs_free_function_request(struct nxs_function_request *req)
{
	if (req) {
		struct nxs_function_elem *elem;

		while (!list_empty(&req->head)) {
			elem = list_first_entry(&req->head,
						struct nxs_function_elem,
						list);
			list_del_init(&elem->list);
			kfree(elem);
		}
		kfree(req);
	}
}
EXPORT_SYMBOL_GPL(nxs_free_function_request);

struct nxs_function *nxs_function_build(struct nxs_function_request *req)
{
	struct nxs_function_elem *elem;
	struct nxs_dev *nxs_dev;
	struct nxs_function *f = NULL;

	f = kzalloc(sizeof(*f), GFP_KERNEL);
	if (!f) {
		WARN_ON(1);
		return NULL;
	}

	INIT_LIST_HEAD(&f->dev_list);
	f->req = req;

	list_for_each_entry(elem, &req->head, list) {
		nxs_dev = get_nxs_dev(elem->function, elem->index, elem->user,
				      elem->multitap_follow);
		if (!nxs_dev) {
			pr_err("can't get nxs_dev for func %s, index 0x%x\n",
				nxs_function_to_str(elem->function),
				elem->index);
			goto error_out;
		}

		list_add_tail(&nxs_dev->func_list, &f->dev_list);
	}

	if (is_display_channel(f) && nxs_function_register_display(f)) {
		pr_err("%s: failed to nxs_function_register_display\n",
		       __func__);
		goto error_out;
	}

	mutex_lock(&nxs_func_lock);
	nxs_func_seq++;
	f->id = nxs_func_seq;
	mutex_unlock(&nxs_func_lock);

	if (req->flags & BLENDING_TO_OTHER) {
		int ret;

		ret = nxs_function_add_to_display(req->option.display_id, f);
		if (ret) {
			pr_err("%s: failed to nxs_function_add_to_display\n",
			       __func__);
			goto error_out;
		}
	}

	mutex_lock(&nxs_func_lock);
	list_add_tail(&f->list, &nxs_func_list);
	mutex_unlock(&nxs_func_lock);

	return f;

error_out:
	if (f)
		nxs_function_destroy(f);

	return NULL;
}
EXPORT_SYMBOL_GPL(nxs_function_build);

struct nxs_function *nxs_function_make(int dev_num, ...)
{
	int i;
	va_list arg;
	struct nxs_function_elem *elem;
	struct nxs_function_request *req;
	u32 func_num, func_index;

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	INIT_LIST_HEAD(&req->head);

	va_start(arg, dev_num);
	for (i = 0; i < dev_num; i++) {
		func_num = va_arg(arg, u32);
		func_index = va_arg(arg, u32);

		elem = kzalloc(sizeof(*elem), GFP_KERNEL);
		if (WARN(!elem, "failed to alloc nxs_function_elem\n"))
			return NULL;

		elem->function = func_num;
		elem->index = func_index;
		elem->user = NXS_FUNCTION_USER_KERNEL;
		list_add_tail(&elem->list, &req->head);
		req->nums_of_function++;
	}
	va_end(arg);

	return nxs_function_build(req);
}
EXPORT_SYMBOL_GPL(nxs_function_make);

void nxs_function_destroy(struct nxs_function *f)
{
	struct nxs_dev *nxs_dev;

	mutex_lock(&nxs_func_lock);
	list_del_init(&f->list);
	mutex_unlock(&nxs_func_lock);

	pr_info("%s: function %p\n", __func__, f);

	while (!list_empty(&f->dev_list)) {
		nxs_dev = list_first_entry(&f->dev_list,
					   struct nxs_dev, func_list);
		list_del_init(&nxs_dev->func_list);
		/* nxs_dev_print(nxs_dev, "PUT"); */
		put_nxs_dev(nxs_dev);
	}

	nxs_free_function_request(f->req);
	kfree(f);
}
EXPORT_SYMBOL_GPL(nxs_function_destroy);

struct nxs_function *nxs_function_get(int handle)
{
	struct nxs_function *f;
	bool found = false;

	mutex_lock(&nxs_func_lock);
	list_for_each_entry(f, &nxs_func_list, list) {
		if (f->id == handle) {
			found = true;
			break;
		}
	}
	if (!found)
		f = NULL;
	mutex_unlock(&nxs_func_lock);

	return f;
}
EXPORT_SYMBOL_GPL(nxs_function_get);

struct nxs_irq_callback *
nxs_function_register_irqcallback(struct nxs_function *f,
				  void *data,
				  void (*handler)(struct nxs_dev *,
						  void*)
				 )
{
	struct nxs_dev *last;
	struct nxs_irq_callback *callback;
	int ret;

	last = list_last_entry(&f->dev_list, struct nxs_dev, func_list);
	if (!last)
		BUG();

	callback = kzalloc(sizeof(*callback), GFP_KERNEL);
	if (!callback)
		return NULL;

	callback->handler = handler;
	callback->data = data;

	ret = nxs_dev_register_irq_callback(last, NXS_DEV_IRQCALLBACK_TYPE_IRQ,
					    callback);
	if (ret) {
		kfree(callback);
		return NULL;
	}

	return callback;
}
EXPORT_SYMBOL_GPL(nxs_function_register_irqcallback);

int nxs_function_unregister_irqcallback(struct nxs_function *f,
					struct nxs_irq_callback *callback)
{
	struct nxs_dev *last;

	last = list_last_entry(&f->dev_list, struct nxs_dev, func_list);
	if (!last)
		BUG();

	nxs_dev_unregister_irq_callback(last,
					NXS_DEV_IRQCALLBACK_TYPE_IRQ,
					callback);
	kfree(callback);

	return 0;
}
EXPORT_SYMBOL_GPL(nxs_function_unregister_irqcallback);

int nxs_function_config(struct nxs_function *f, bool dirty,
			int count, ...)
{
	struct nxs_dev *nxs_dev;
	va_list arg;
	int ret;
	int i;
	struct nxs_control *c;

	va_start(arg, count);
	for (i = 0; i < count; i++) {
		c = va_arg(arg, struct nxs_control *);

		list_for_each_entry_reverse(nxs_dev, &f->dev_list, func_list) {
			ret = nxs_set_control(nxs_dev, c->type, c);
			if (ret)
				return ret;
		}
	}
	va_end(arg);

	if (dirty) {
		list_for_each_entry_reverse(nxs_dev, &f->dev_list, func_list) {
			if (nxs_dev->set_dirty) {
				ret = nxs_dev->set_dirty(nxs_dev,
							 NXS_DEV_DIRTY_NORMAL);
				if (ret)
					return ret;
			}
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(nxs_function_config);

int nxs_function_connect(struct nxs_function *f)
{
	if (f->disp)
		return nxs_display_connect(f->disp, f);

	return function_connect(f);
}
EXPORT_SYMBOL_GPL(nxs_function_connect);

int nxs_function_ready(struct nxs_function *f)
{
	if (f->disp)
		return nxs_display_ready(f->disp, f);

	return function_ready(f);
}
EXPORT_SYMBOL_GPL(nxs_function_ready);

int nxs_function_start(struct nxs_function *f)
{
	struct nxs_dev *nxs_dev;
	int ret;

	list_for_each_entry_reverse(nxs_dev, &f->dev_list, func_list) {
		if (nxs_dev->start) {
			ret = nxs_dev->start(nxs_dev);
			if (ret)
				return ret;
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(nxs_function_start);

void nxs_function_stop(struct nxs_function *f)
{
	struct nxs_dev *nxs_dev;

	list_for_each_entry_reverse(nxs_dev, &f->dev_list, func_list) {
		if (nxs_dev->stop)
			nxs_dev->stop(nxs_dev);
	}
}
EXPORT_SYMBOL_GPL(nxs_function_stop);

void nxs_function_disconnect(struct nxs_function *f)
{
	pr_info("%s: function %p\n", __func__, f);
	if (f->disp) {
		int ret;
		struct nxs_display *d = f->disp;

		ret = remove_function_from_display(d, f);
		if (WARN(ret, "%s: failed to remove_function_from_display\n",
		     __func__))
			return;

		ret = nxs_display_connect(d, d->hori_top);
		if (WARN(ret, "%s: failed to nxs_display_connect\n", __func__))
			return;
	}
}
EXPORT_SYMBOL_GPL(nxs_function_disconnect);

struct nxs_dev *nxs_function_find_dev(struct nxs_function *f, u32 dev_function)
{
	struct nxs_dev *nxs_dev;

	list_for_each_entry(nxs_dev, &f->dev_list, func_list)
		if (nxs_dev->dev_function == dev_function)
			return nxs_dev;

	return NULL;
}
EXPORT_SYMBOL_GPL(nxs_function_find_dev);

int nxs_function_register_display(struct nxs_function *f)
{
	struct nxs_display *disp = NULL;
	struct nxs_dev *bottom;
	struct nxs_dev *joint_dev;

	pr_info("/////////////////////////////////////////////////\n");
	pr_info("/////////////////////////////////////////////////\n");
	pr_info("/////////////////////////////////////////////////\n");

	disp = kzalloc(sizeof(*disp), GFP_KERNEL);
	if (WARN(!disp, "%s: failed to alloc nxs_display\n", __func__))
		return -ENOMEM;

	INIT_LIST_HEAD(&disp->list);
	INIT_LIST_HEAD(&disp->func_list);
	INIT_LIST_HEAD(&disp->vert_list);

	if (f->req->flags & BLENDING_TO_BOTTOM)
		bottom = get_nxs_dev(NXS_FUNCTION_MLC_BOTTOM,
				     f->req->option.bottom_id,
				     NXS_FUNCTION_USER_APP,
				     0);
	else
		bottom = get_bottom_dev(f);

	if (WARN(!bottom, "failed to get bottom dev %d\n",
		 f->req->option.bottom_id))
		goto error_out;

	list_add_tail(&bottom->disp_list, &disp->vert_list);
	joint_dev = find_blender(f);
	if (joint_dev)
		list_add_tail(&joint_dev->disp_list, &disp->vert_list);

	list_add_tail(&f->disp_list, &disp->func_list);
	disp->hori_top = f;
	disp->vert_bottom = bottom;

	mutex_lock(&nxs_disp_lock);
	list_add_tail(&disp->list, &nxs_disp_list);
	mutex_unlock(&nxs_disp_lock);

	f->disp = disp;

	pr_info("%s: success disp %p, func %p, bottom %p[%s, %d]\n", __func__,
		disp, f, bottom, nxs_function_to_str(bottom->dev_function),
		bottom->dev_inst_index);
	nxs_display_print(disp);
	/* nxs_function_print(f); */
	return 0;

error_out:
	if (disp)
		kfree(disp);

	return -ENODEV;
}
EXPORT_SYMBOL_GPL(nxs_function_register_display);

int nxs_function_unregister_display(struct nxs_function *f)
{
	struct nxs_dev *nxs_dev;
	struct nxs_display *disp;
	struct nxs_function *func;

	disp = find_display_by_function(f);

	pr_info("%s: function %p, display %p\n", __func__, f, disp);

	/* unlink all function list */
	while (!list_empty(&disp->func_list)) {
		func = list_first_entry(&disp->func_list,
					struct nxs_function, disp_list);
		pr_info("func %p\n", func);
		list_del_init(&func->disp_list);
	}

	/* unlink all vert_list */
	while (!list_empty(&disp->vert_list)) {
		nxs_dev = list_first_entry(&disp->vert_list,
					   struct nxs_dev, disp_list);
		list_del_init(&nxs_dev->disp_list);
	}

	return unregister_display(disp);
}
EXPORT_SYMBOL_GPL(nxs_function_unregister_display);

/* this is blending model */
int nxs_function_add_to_display(int disp, struct nxs_function *add)
{
	struct nxs_display *d;

	/* pr_info("%s: disp %d, function %d\n", __func__, disp, add->id); */
	/* nxs_function_print(add); */
	d = find_display_by_index(disp);
	if (WARN(!d, "%s: failed to find display for disp %d\n", __func__,
		 disp))
		return -ENODEV;

	return add_function_to_display(d, add);
}
EXPORT_SYMBOL_GPL(nxs_function_add_to_display);

int nxs_function_remove_from_display(int disp, struct nxs_function *remove)
{
	struct nxs_display *d;

	d = find_display_by_index(disp);
	if (WARN(!d, "%s: failed to find display for disp %d\n", __func__,
		 disp))
		return -ENODEV;

	return remove_function_from_display(d, remove);
}
EXPORT_SYMBOL_GPL(nxs_function_remove_from_display);
