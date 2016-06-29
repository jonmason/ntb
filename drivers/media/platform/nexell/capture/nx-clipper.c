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
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/atomic.h>
#include <linux/irqreturn.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/semaphore.h>
#include <linux/v4l2-mediabus.h>
#include <linux/i2c.h>
#include <linux/pwm.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>

#include <media/media-device.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <dt-bindings/media/nexell-vip.h>

#ifdef CONFIG_ARM_S5Pxx18_DEVFREQ
#include <linux/soc/nexell/cpufreq.h>
#endif

#include "../nx-v4l2.h"
#include "../nx-video.h"
#include "nx-vip-primitive.h"
#include "nx-vip.h"

#define NX_CLIPPER_DEV_NAME	"nx-clipper"

enum {
	NX_CLIPPER_PAD_SINK,
	NX_CLIPPER_PAD_SOURCE_MEM,
	NX_CLIPPER_PAD_SOURCE_DECIMATOR,
	NX_CLIPPER_PAD_MAX
};

struct gpio_action_unit {
	int value;
	int delay_ms;
};

struct nx_capture_enable_gpio_action {
	int gpio_num;
	int count;
	/* alloc dynamically by count */
	struct gpio_action_unit *units;
};

struct nx_capture_enable_pmic_action {
	int enable;
	int delay_ms;
};

struct nx_capture_enable_clock_action {
	int enable;
	int delay_ms;
};

struct nx_capture_power_action {
	int type;
	/**
	 * nx_capture_enable_gpio_action or
	 * nx_capture_enable_pmic_action or
	 * nx_capture_enable_clock_action
	 */
	void *action;
};

struct nx_capture_power_seq {
	int count;
	/* alloc dynamically by count */
	struct nx_capture_power_action *actions;
};

struct nx_v4l2_i2c_board_info {
	int     i2c_adapter_id;
	struct i2c_board_info board_info;
};

enum {
	STATE_IDLE = 0,
	STATE_MEM_RUNNING  = (1 << 0),
	STATE_CLIP_RUNNING = (1 << 1),
	STATE_MEM_STOPPING = (1 << 2),
};

struct nx_clipper {
	u32 module;
	u32 interface_type;
	u32 external_sync;
	u32 h_frontporch;
	u32 h_syncwidth;
	u32 h_backporch;
	u32 v_frontporch;
	u32 v_backporch;
	u32 v_syncwidth;
	u32 clock_invert;
	u32 port;
	u32 interlace;
	int regulator_nr;
	char **regulator_names;
	u32 *regulator_voltages;
	struct pwm_device *pwm;
	u32 clock_rate;
	struct nx_capture_power_seq enable_seq;
	struct nx_capture_power_seq disable_seq;
	struct nx_v4l2_i2c_board_info sensor_info;

	struct v4l2_subdev subdev;
	struct media_pad pads[NX_CLIPPER_PAD_MAX];

	struct v4l2_rect crop;
	u32 bus_fmt; /* data_order */
	u32 width;
	u32 height;

	atomic_t state;
	struct completion stop_done;
	struct semaphore s_stream_sem;
	bool sensor_enabled;
	struct media_pad sensor_pad;


	struct platform_device *pdev;

#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
	struct nx_video_buffer_object vbuf_obj;
	struct nx_v4l2_irq_entry *irq_entry;
	u32 mem_fmt;
#endif
};


/**
 * parse device tree
 */
static int parse_sensor_i2c_board_info_dt(struct device_node *np,
				      struct device *dev, struct nx_clipper *me)
{
	const char *name;
	u32 adapter;
	u32 addr;
	struct nx_v4l2_i2c_board_info *info = &me->sensor_info;

	if (of_property_read_string(np, "i2c_name", &name)) {
		dev_err(dev, "failed to get sensor i2c name\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "i2c_adapter", &adapter)) {
		dev_err(dev, "failed to get sensor i2c adapter\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "addr", &addr)) {
		dev_err(dev, "failed to get sensor i2c addr\n");
		return -EINVAL;
	}

	strcpy(info->board_info.type, name);
	info->board_info.addr = addr;
	info->i2c_adapter_id = adapter;

	return 0;
}

static int parse_sensor_dt(struct device_node *np, struct device *dev,
			   struct nx_clipper *me)
{
	int ret;
	u32 type;

	if (of_property_read_u32(np, "type", &type)) {
		dev_err(dev, "failed to get dt sensor type\n");
		return -EINVAL;
	}

	if (type == NX_CAPTURE_SENSOR_I2C) {
		ret = parse_sensor_i2c_board_info_dt(np, dev, me);
		if (ret)
			return ret;
	} else {
		dev_err(dev, "sensor type %d is not supported\n",
			type);
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

static int get_num_of_enable_action(u32 *array, int count)
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
			pr_err("failed to find_action_end, check power node of dts\n");
			return 0;
		}
		p += next_index;
		length -= next_index;
		action_num++;
	}

	return action_num;
}

static u32 *get_next_action_unit(u32 *array, int count)
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

static int make_enable_gpio_action(u32 *start, u32 *end,
				   struct nx_capture_power_action *action)
{
	struct nx_capture_enable_gpio_action *gpio_action;
	struct gpio_action_unit *unit;
	int i;
	u32 *p;
	/* start_marker, type, gpio num */
	int unit_count = end - start - 1 - 1 - 1;

	if ((unit_count <= 0) || (unit_count % 2)) {
		pr_err("invalid unit_count %d of gpio action\n", unit_count);
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

	action->type = NX_ACTION_TYPE_GPIO;
	action->action = gpio_action;

	return 0;
}

static int make_enable_pmic_action(u32 *start, u32 *end,
				   struct nx_capture_power_action *action)
{
	struct nx_capture_enable_pmic_action *pmic_action;
	int entry_count = end - start - 1 - 1; /* start_marker, type */

	if ((entry_count <= 0) || (entry_count % 2)) {
		pr_err("invalid entry_count %d of pmic action\n", entry_count);
		return -EINVAL;
	}

	pmic_action = kzalloc(sizeof(*pmic_action), GFP_KERNEL);
	if (!pmic_action) {
		WARN_ON(1);
		return -ENOMEM;
	}

	pmic_action->enable = start[2];
	pmic_action->delay_ms = start[3];

	action->type = NX_ACTION_TYPE_PMIC;
	action->action = pmic_action;

	return 0;
}

static int make_enable_clock_action(u32 *start, u32 *end,
				    struct nx_capture_power_action *action)
{
	struct nx_capture_enable_clock_action *clock_action;
	int entry_count = end - start - 1 - 1; /* start_marker, type */

	if ((entry_count <= 0) || (entry_count % 2)) {
		pr_err("invalid entry_count %d of clock action\n", entry_count);
		return -EINVAL;
	}

	clock_action = kzalloc(sizeof(*clock_action), GFP_KERNEL);
	if (!clock_action) {
		WARN_ON(1);
		return -ENOMEM;
	}

	clock_action->enable = start[2];
	clock_action->delay_ms = start[3];

	action->type = NX_ACTION_TYPE_CLOCK;
	action->action = clock_action;

	return 0;
}

static int make_enable_action(u32 *array, int count,
			      struct nx_capture_power_action *action)
{
	u32 *p = array;
	int end_index = find_action_end(p, count);

	if (end_index <= 0) {
		pr_err("parse dt for v4l2 capture error: can't find action end\n");
		return -EINVAL;
	}

	switch (get_action_type(p)) {
	case NX_ACTION_TYPE_GPIO:
		return make_enable_gpio_action(p, p + end_index, action);
	case NX_ACTION_TYPE_PMIC:
		return make_enable_pmic_action(p, p + end_index, action);
	case NX_ACTION_TYPE_CLOCK:
		return make_enable_clock_action(p, p + end_index, action);
	default:
		pr_err("parse dt v4l2 capture: invalid type 0x%x\n",
		       get_action_type(p));
		return -EINVAL;
	}
}

static void free_enable_seq_actions(struct nx_capture_power_seq *seq)
{
	int i;
	struct nx_capture_power_action *action;

	for (i = 0; i < seq->count; i++) {
		action = &seq->actions[i];
		if (action->action) {
			if (action->type == NX_ACTION_TYPE_GPIO) {
				struct nx_capture_enable_gpio_action *
					gpio_action = action->action;
				kfree(gpio_action->units);
			}
			kfree(action->action);
		}
	}

	kfree(seq->actions);
	seq->count = 0;
	seq->actions = NULL;
}

static int parse_capture_power_seq(struct device_node *np,
				   char *node_name,
				   struct nx_capture_power_seq *seq)
{
	int count = of_property_count_elems_of_size(np, node_name, 4);

	if (count > 0) {
		u32 *p;
		int ret = 0;
		struct nx_capture_power_action *action;
		int action_nums;
		u32 *array = kcalloc(count, sizeof(u32), GFP_KERNEL);

		if (!array) {
			WARN_ON(1);
			return -ENOMEM;
		}

		of_property_read_u32_array(np, node_name, array, count);
		action_nums = get_num_of_enable_action(array, count);
		if (action_nums <= 0) {
			pr_err("parse_dt v4l2 capture: no actions in %s\n",
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
			p = get_next_action_unit(p, count - (p - array));
			if (!p) {
				pr_err("failed to get_next_action_unit(%d/%d)\n",
				       seq->count, action_nums);
				free_enable_seq_actions(seq);
				return -EINVAL;
			}

			ret = make_enable_action(p, count - (p - array),
						 action);
			if (ret != 0) {
				free_enable_seq_actions(seq);
				return ret;
			}

			p++;
			action++;
		}
	}

	return 0;
}

static int parse_power_dt(struct device_node *np, struct device *dev,
			  struct nx_clipper *me)
{
	int ret = 0;

	if (of_find_property(np, "enable_seq", NULL))
		ret = parse_capture_power_seq(np, "enable_seq",
					      &me->enable_seq);

	if (ret)
		return ret;

	if (of_find_property(np, "disable_seq", NULL))
		ret = parse_capture_power_seq(np, "disable_seq",
					      &me->disable_seq);

	return ret;
}

static int parse_clock_dt(struct device_node *np, struct device *dev,
			  struct nx_clipper *me)
{
	me->pwm = devm_pwm_get(dev, NULL);
	if (!IS_ERR(me->pwm)) {
		unsigned int period = pwm_get_period(me->pwm);
		pwm_config(me->pwm, period/2, period);
	} else {
		me->pwm = NULL;
	}

	return 0;
}

static int nx_clipper_parse_dt(struct device *dev, struct nx_clipper *me)
{
	int ret;
	struct device_node *np = dev->of_node;
	struct device_node *child_node = NULL;

	if (of_property_read_u32(np, "module", &me->module)) {
		dev_err(dev, "failed to get dt module\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "interface_type", &me->interface_type)) {
		dev_err(dev, "failed to get dt interface_type\n");
		return -EINVAL;
	}

	if (me->interface_type == NX_CAPTURE_INTERFACE_MIPI_CSI) {
		/* mipi use always same config, so ignore user config */
		if (me->module != 0) {
			dev_err(dev, "module of mipi-csi must be 0 but, current %d\n",
				me->module);
			return -EINVAL;
		}
		me->port = 1;
#ifdef CONFIG_ARCH_S5P4418
		me->h_frontporch = 8;
		me->h_syncwidth = 7;
		me->h_backporch = 7;
		me->v_frontporch = 1;
		me->v_syncwidth = 8;
		me->v_backporch = 1;
		me->clock_invert = 0;
#else
		me->h_frontporch = 4;
		me->h_syncwidth = 4;
		me->h_backporch = 4;
		me->v_frontporch = 1;
		me->v_syncwidth = 1;
		me->v_backporch = 1;
		me->clock_invert = 0;
#endif
		me->bus_fmt = NX_VIN_CBY0CRY1;
		me->interlace = 0;
	} else {
		if (of_property_read_u32(np, "port", &me->port)) {
			dev_err(dev, "failed to get dt port\n");
			return -EINVAL;
		}

		if (of_property_read_u32(np, "external_sync",
					 &me->external_sync)) {
			dev_err(dev, "failed to get dt external_sync\n");
			return -EINVAL;
		}

		if (me->external_sync == 0) {
			/* when 656, porch value is always same, so ignore user
			 * config
			 */
			me->h_frontporch = 7;
			me->h_syncwidth = 1;
			me->h_backporch = 10;
			me->v_frontporch = 0;
			me->v_syncwidth = 2;
			me->v_backporch = 3;
		} else {
			if (of_property_read_u32(np, "h_frontporch",
						 &me->h_frontporch)) {
				dev_err(dev, "failed to get dt h_frontporch\n");
				return -EINVAL;
			}
			if (of_property_read_u32(np, "h_syncwidth",
						 &me->h_syncwidth)) {
				dev_err(dev, "failed to get dt h_syncwidth\n");
				return -EINVAL;
			}
			if (of_property_read_u32(np, "h_backporch",
						 &me->h_backporch)) {
				dev_err(dev, "failed to get dt h_backporch\n");
				return -EINVAL;
			}
			if (of_property_read_u32(np, "v_frontporch",
						 &me->v_frontporch)) {
				dev_err(dev, "failed to get dt v_frontporch\n");
				return -EINVAL;
			}
			if (of_property_read_u32(np, "v_syncwidth",
						 &me->v_syncwidth)) {
				dev_err(dev, "failed to get dt v_syncwidth\n");
				return -EINVAL;
			}
			if (of_property_read_u32(np, "v_backporch",
						 &me->v_backporch)) {
				dev_err(dev, "failed to get dt v_backporch\n");
				return -EINVAL;
			}
		}

		/* optional */
		of_property_read_u32(np, "interlace", &me->interlace);
	}

	/* common property */
	of_property_read_u32(np, "data_order", &me->bus_fmt);

	me->regulator_nr = of_property_count_strings(np, "regulator_names");
	if (me->regulator_nr > 0) {
		int i;
		const char *name;
		me->regulator_names = devm_kcalloc(dev,
						   me->regulator_nr,
						   sizeof(char *),
						   GFP_KERNEL);
		if (!me->regulator_names) {
			WARN_ON(1);
			return -ENOMEM;
		}

		me->regulator_voltages = devm_kcalloc(dev,
						      me->regulator_nr,
						      sizeof(u32),
						      GFP_KERNEL);
		if (!me->regulator_voltages) {
			WARN_ON(1);
			return -ENOMEM;
		}


		for (i = 0; i < me->regulator_nr; i++) {
			if (of_property_read_string_index(np, "regulator_names",
							  i, &name)) {
				dev_err(&me->pdev->dev, "failed to read regulator %d name\n", i);
				return -EINVAL;
			}
			me->regulator_names[i] = (char *)name;
		}

		of_property_read_u32_array(np, "regulator_voltages",
					   me->regulator_voltages,
					   me->regulator_nr);
	}

	child_node = of_find_node_by_name(np, "sensor");
	if (!child_node) {
		dev_err(dev, "failed to get sensor node\n");
		return -EINVAL;
	}
	ret = parse_sensor_dt(child_node, dev, me);
	if (ret)
		return ret;

	child_node = of_find_node_by_name(np, "power");
	if (!child_node) {
		dev_err(dev, "failed to get power node\n");
		return -EINVAL;
	}
	ret = parse_power_dt(child_node, dev, me);
	if (ret)
		return ret;

	ret = parse_clock_dt(child_node, dev, me);
	if (ret)
		return ret;

	return 0;
}

/**
 * sensor power enable
 */
static int apply_gpio_action(struct device *dev, int gpio_num,
		  struct gpio_action_unit *unit)
{
	int ret;
	char label[64] = {0, };
	struct device_node *np;
	int gpio;

	np = dev->of_node;
	gpio = of_get_named_gpio(np, "gpios", gpio_num);

	sprintf(label, "v4l2-cam #pwr gpio %d", gpio);
	if (!gpio_is_valid(gpio)) {
		dev_err(dev, "invalid gpio %d set to %d\n", gpio, unit->value);
		return -EINVAL;
	}

	ret = devm_gpio_request_one(dev, gpio,
				    unit->value ?
				    GPIOF_OUT_INIT_HIGH : GPIOF_OUT_INIT_LOW,
				    label);
	if (ret < 0) {
		dev_err(dev, "failed to gpio %d set to %d\n",
			gpio, unit->value);
		return ret;
	}

	if (unit->delay_ms > 0)
		mdelay(unit->delay_ms);

	devm_gpio_free(dev, gpio);

	return 0;
}

static int do_gpio_action(struct nx_clipper *me,
				 struct nx_capture_enable_gpio_action *action)
{
	int ret;
	struct gpio_action_unit *unit;
	int i;

	for (i = 0; i < action->count; i++) {
		unit = &action->units[i];
		ret = apply_gpio_action(&me->pdev->dev, action->gpio_num, unit);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int do_pmic_action(struct nx_clipper *me,
				 struct nx_capture_enable_pmic_action *action)
{
	int ret;
	int i;
	struct regulator *power;

	for (i = 0; i < me->regulator_nr; i++) {
		power = devm_regulator_get(&me->pdev->dev,
					   me->regulator_names[i]);
		if (IS_ERR(power)) {
			dev_err(&me->pdev->dev, "failed to get power %s\n",
				me->regulator_names[i]);
			return -ENODEV;
		}

		if (regulator_can_change_voltage(power)) {
			ret = regulator_set_voltage(power,
						    me->regulator_voltages[i],
						    me->regulator_voltages[i]);
			if (ret) {
				devm_regulator_put(power);
				dev_err(&me->pdev->dev,
					"can't set voltages(index: %d)\n", i);
				return ret;
			}
		}

		ret = 0;
		if (action->enable && !regulator_is_enabled(power)) {
			ret = regulator_enable(power);
		} else if (!action->enable && regulator_is_enabled(power)) {
			ret = regulator_disable(power);
		}

		devm_regulator_put(power);

		if (ret) {
			dev_err(&me->pdev->dev, "failed to power %s %s\n",
				me->regulator_names[i],
				action->enable ? "enable" : "disable");
			return ret;
		}
	}

	return 0;
}

static int do_clock_action(struct nx_clipper *me,
				  struct nx_capture_enable_clock_action *action)
{
	if (me->pwm)
		pwm_enable(me->pwm);

	if (action->delay_ms > 0)
		mdelay(action->delay_ms);

	return 0;
}

static int enable_sensor_power(struct nx_clipper *me, bool enable)
{
	struct nx_capture_power_seq *seq = NULL;

	if (enable && !me->sensor_enabled)
		seq = &me->enable_seq;
	else if (!enable && me->sensor_enabled)
		seq = &me->disable_seq;

	if (seq) {
		int i;
		struct nx_capture_power_action *action;

		for (i = 0; i < seq->count; i++) {
			action = &seq->actions[i];
			switch (action->type) {
			case NX_ACTION_TYPE_GPIO:
				do_gpio_action(me, action->action);
				break;
			case NX_ACTION_TYPE_PMIC:
				do_pmic_action(me, action->action);
				break;
			case NX_ACTION_TYPE_CLOCK:
				do_clock_action(me, action->action);
				break;
			default:
				pr_warn("unknown action type %d\n",
					action->type);
				break;
			}
		}
	}

	me->sensor_enabled = enable;
	return 0;
}

/**
 * buffer operations
 */
#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
static int update_buffer(struct nx_clipper *me)
{
	int ret;
	struct nx_video_buffer *buf;

	ret = nx_video_update_buffer(&me->vbuf_obj);
	if (ret)
		return ret;

	buf = me->vbuf_obj.cur_buf;
	nx_vip_set_clipper_addr(me->module, me->mem_fmt,
				me->crop.width, me->crop.height,
				buf->dma_addr[0], buf->dma_addr[1],
				buf->dma_addr[2], buf->stride[0],
				buf->stride[1]);

	return 0;
}

static void unregister_irq_handler(struct nx_clipper *me)
{
	if (me->irq_entry) {
		nx_vip_unregister_irq_entry(me->module, me->irq_entry);
		kfree(me->irq_entry);
		me->irq_entry = NULL;
	}
}

static irqreturn_t nx_clipper_irq_handler(void *data)
{
	struct nx_clipper *me = data;

	nx_video_done_buffer(&me->vbuf_obj);
	if (NX_ATOMIC_READ(&me->state) & STATE_MEM_STOPPING) {
		nx_vip_stop(me->module, VIP_CLIPPER);
		complete(&me->stop_done);
	} else {
		update_buffer(me);
	}

	return IRQ_HANDLED;
}

static int register_irq_handler(struct nx_clipper *me)
{
	struct nx_v4l2_irq_entry *irq_entry = me->irq_entry;

	if (!irq_entry) {
		irq_entry = kzalloc(sizeof(*irq_entry), GFP_KERNEL);
		if (!irq_entry) {
			WARN_ON(1);
			return -ENOMEM;
		}
		me->irq_entry = irq_entry;
	}
	irq_entry->irqs = VIP_OD_INT;
	irq_entry->priv = me;
	irq_entry->handler = nx_clipper_irq_handler;

	return nx_vip_register_irq_entry(me->module, irq_entry);
}

static int clipper_buffer_queue(struct nx_video_buffer *buf, void *data)
{
	struct nx_clipper *me = data;

	nx_video_add_buffer(&me->vbuf_obj, buf);
	return 0;
}

static int handle_video_connection(struct nx_clipper *me, bool connected)
{
	int ret = 0;

	if (connected)
		ret = nx_video_register_buffer_consumer(&me->vbuf_obj,
							clipper_buffer_queue,
							me);
	else
		nx_video_unregister_buffer_consumer(&me->vbuf_obj);

	return ret;
}
#endif

static struct v4l2_subdev *get_remote_source_subdev(struct nx_clipper *me)
{
	struct media_pad *pad =
		media_entity_remote_pad(&me->pads[NX_CLIPPER_PAD_SINK]);
	if (!pad) {
		dev_err(&me->pdev->dev, "can't find remote source device\n");
		return NULL;
	}
	return media_entity_to_v4l2_subdev(pad->entity);
}

static void set_vip(struct nx_clipper *me)
{
	u32 module = me->module;
	bool is_mipi = me->interface_type == NX_CAPTURE_INTERFACE_MIPI_CSI;

	nx_vip_set_input_port(module, me->port);
	nx_vip_set_field_mode(module, false, nx_vip_fieldsel_bypass,
			      me->interlace, false);

	if (is_mipi) {
		nx_vip_set_data_mode(module, me->bus_fmt, 16);
		nx_vip_set_dvalid_mode(module, true, true, true);
		nx_vip_set_hvsync_for_mipi(module,
					   me->width * 2,
					   me->height,
					   me->h_syncwidth,
					   me->h_frontporch,
					   me->h_backporch,
					   me->v_syncwidth,
					   me->v_frontporch,
					   me->v_backporch);
	} else {
		nx_vip_set_data_mode(module, me->bus_fmt, 8);
		nx_vip_set_dvalid_mode(module, false, false, false);
		nx_vip_set_hvsync(module,
				  me->external_sync,
				  me->width * 2,
				  me->interlace ?
				  me->height >> 1 : me->height,
				  me->h_syncwidth,
				  me->h_frontporch,
				  me->h_backporch,
				  me->v_syncwidth,
				  me->v_frontporch,
				  me->v_backporch);
	}

	nx_vip_set_fiforeset_mode(module, nx_vip_fiforeset_all);

	nx_vip_set_clip_region(module,
			       me->crop.left,
			       me->crop.top,
			       me->crop.left + me->crop.width,
			       me->interlace ?
			       (me->crop.top + me->crop.height) >> 1 :
			       (me->crop.top + me->crop.height));
}

/**
 * v4l2 subdev ops
 */
static int nx_clipper_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret;
	struct nx_clipper *me = v4l2_get_subdevdata(sd);
	struct v4l2_subdev *remote;
#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
	u32 module = me->module;
	char *hostname = (char *)v4l2_get_subdev_hostdata(sd);
	bool is_host_video = false;
#endif

	remote = get_remote_source_subdev(me);
	if (!remote) {
		WARN_ON(1);
		return -ENODEV;
	}

#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
	if (!hostname)
		return -EEXIST;

	if (!strncmp(hostname, "VIDEO", 5))
		is_host_video = true;

#endif

	ret = down_interruptible(&me->s_stream_sem);

	if (enable) {
#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
		if (NX_ATOMIC_READ(&me->state) & STATE_MEM_STOPPING) {
			int timeout = 50; /* 5 second */

			dev_info(&me->pdev->dev, "wait clipper stopping\n");
			while (NX_ATOMIC_READ(&me->state) &
			       STATE_MEM_STOPPING) {
				msleep(100);
				timeout--;
				if (timeout == 0) {
					dev_err(&me->pdev->dev, "timeout for waiting clipper stop\n");
					break;
				}
			}
		}
#endif
		if (!(NX_ATOMIC_READ(&me->state) &
		      (STATE_MEM_RUNNING | STATE_CLIP_RUNNING))) {
			if (me->crop.width == 0 || me->crop.height == 0) {
				me->crop.left = 0;
				me->crop.top = 0;
				me->crop.width = me->width;
				me->crop.height = me->height;
			}

#ifdef CONFIG_ARM_S5Pxx18_DEVFREQ
			nx_bus_qos_update(NX_BUS_CLK_HIGH_KHZ);
#endif

			set_vip(me);
			ret = enable_sensor_power(me, true);
			if (ret) {
				WARN_ON(1);
				goto UP_AND_OUT;
			}
			ret = v4l2_subdev_call(remote, video, s_stream, 1);
			if (ret) {
				dev_err(&me->pdev->dev,
					"failed to s_stream %d\n", enable);
				goto UP_AND_OUT;
			}
		}

#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
		if (is_host_video) {
			nx_vip_set_clipper_format(module, me->mem_fmt);
			ret = register_irq_handler(me);
			if (ret) {
				WARN_ON(1);
				goto UP_AND_OUT;
			}

			update_buffer(me);
			nx_vip_run(me->module, VIP_CLIPPER);
			NX_ATOMIC_SET_MASK(STATE_MEM_RUNNING, &me->state);
		} else
#endif
		NX_ATOMIC_SET_MASK(STATE_CLIP_RUNNING, &me->state);
	} else {
		if (!(NX_ATOMIC_READ(&me->state) &
		      (STATE_MEM_RUNNING | STATE_CLIP_RUNNING)))
			goto UP_AND_OUT;

#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
		if (is_host_video &&
		    (NX_ATOMIC_READ(&me->state) & STATE_MEM_RUNNING)) {
			NX_ATOMIC_SET_MASK(STATE_MEM_STOPPING, &me->state);
			if (!wait_for_completion_timeout(&me->stop_done,
							 2*HZ)) {
				pr_warn("timeout for waiting clipper stop\n");
				nx_vip_stop(module, VIP_CLIPPER);
			}

			NX_ATOMIC_CLEAR_MASK(STATE_MEM_STOPPING, &me->state);
			unregister_irq_handler(me);
			nx_video_clear_buffer(&me->vbuf_obj);
			NX_ATOMIC_CLEAR_MASK(STATE_MEM_RUNNING, &me->state);

			memset(&me->crop, 0, sizeof(me->crop));
		}

		if (!is_host_video)
			if (NX_ATOMIC_READ(&me->state) == STATE_IDLE)
				goto UP_AND_OUT;

#else
		if (NX_ATOMIC_READ(&me->state) & STATE_CLIP_RUNNING) {
#endif
			v4l2_subdev_call(remote, video, s_stream, 0);
			enable_sensor_power(me, false);
			NX_ATOMIC_CLEAR_MASK(STATE_CLIP_RUNNING, &me->state);

#ifdef CONFIG_ARM_S5Pxx18_DEVFREQ
			nx_bus_qos_update(NX_BUS_CLK_LOW_KHZ);
#endif

#ifndef CONFIG_VIDEO_NEXELL_CLIPPER
		}
#endif
	}

UP_AND_OUT:
	up(&me->s_stream_sem);

	return ret;
}

static int nx_clipper_g_crop(struct v4l2_subdev *sd,
			     struct v4l2_crop *crop)
{
	struct nx_clipper *me = v4l2_get_subdevdata(sd);
	/* crop->c = me->crop; */
	memcpy(&crop->c, &me->crop, sizeof(struct v4l2_rect));
	return 0;
}

static int nx_clipper_s_crop(struct v4l2_subdev *sd,
			     const struct v4l2_crop *crop)
{
	struct nx_clipper *me = v4l2_get_subdevdata(sd);
	/* me->crop = crop->c; */
	memcpy(&me->crop, &crop->c, sizeof(struct v4l2_rect));
	return 0;
}

static int nx_clipper_g_parm(struct v4l2_subdev *sd,
			     struct v4l2_streamparm *param)
{
	int err;
	struct nx_clipper *me = v4l2_get_subdevdata(sd);
	struct v4l2_subdev *remote = get_remote_source_subdev(me);

	if (!remote) {
		WARN_ON(1);
		return -ENODEV;
	}

	err = v4l2_subdev_call(remote, video, g_parm, param);

	if (err) {
		param->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
		param->parm.capture.timeperframe.numerator = 1001;
		param->parm.capture.timeperframe.denominator = 30000;
		param->parm.capture.readbuffers = 1;
	}

	return 0;
}

static int nx_clipper_s_parm(struct v4l2_subdev *sd,
			     struct v4l2_streamparm *param)
{
	struct nx_clipper *me = v4l2_get_subdevdata(sd);
	struct v4l2_subdev *remote = get_remote_source_subdev(me);

	if (!remote) {
		WARN_ON(1);
		return -ENODEV;
	}

	return v4l2_subdev_call(remote, video, s_parm, param);
}

/**
 * called by VIDIOC_SUBDEV_S_CROP
 */
static int nx_clipper_get_selection(struct v4l2_subdev *sd,
				    struct v4l2_subdev_pad_config *cfg,
				    struct v4l2_subdev_selection *sel)
{
	struct nx_clipper *me = v4l2_get_subdevdata(sd);

	memcpy(&sel->r, &me->crop, sizeof(struct v4l2_rect));
	return 0;
}

static int nx_clipper_set_selection(struct v4l2_subdev *sd,
				    struct v4l2_subdev_pad_config *cfg,
				    struct v4l2_subdev_selection *sel)
{
	struct nx_clipper *me = v4l2_get_subdevdata(sd);

	memcpy(&me->crop, &sel->r, sizeof(struct v4l2_rect));
	return 0;
}

static int nx_clipper_get_fmt(struct v4l2_subdev *sd,
			      struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_format *format)
{
	struct nx_clipper *me = v4l2_get_subdevdata(sd);
	u32 pad = format->pad;

	if (pad == 0) {
		/* get bus format */
		u32 mbus_fmt;
		int ret = nx_vip_find_mbus_format(me->bus_fmt, &mbus_fmt);

		if (ret) {
			dev_err(&me->pdev->dev, "can't get mbus_fmt for bus\n");
			return ret;
		}
		format->format.code = mbus_fmt;
		format->format.width = me->width;
		format->format.height = me->height;
	}
#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
	else if (pad == 1) {
		/* get mem format */
		u32 mem_fmt;
		int ret = nx_vip_find_mbus_mem_format(me->mem_fmt, &mem_fmt);

		if (ret) {
			dev_err(&me->pdev->dev, "can't get mbus_fmt for mem\n");
			return ret;
		}
		format->format.code = mem_fmt;
		format->format.width = me->width;
		format->format.height = me->height;
	}
#endif
	else {
		dev_err(&me->pdev->dev, "%d is invalid pad value for get_fmt\n",
			pad);
		return -EINVAL;
	}

	return 0;
}

static int nx_clipper_set_fmt(struct v4l2_subdev *sd,
			      struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_format *format)
{
	struct nx_clipper *me = v4l2_get_subdevdata(sd);
	u32 pad = format->pad;
	struct v4l2_subdev *remote = get_remote_source_subdev(me);

	if (!remote) {
		WARN_ON(1);
		return -ENODEV;
	}

	if (pad == 0) {
		/* set bus format */
		u32 nx_bus_fmt;
		int ret = nx_vip_find_nx_bus_format(format->format.code,
						    &nx_bus_fmt);
		if (ret) {
			dev_err(&me->pdev->dev, "Unsupported bus format %d\n",
			       format->format.code);
			return ret;
		}
		me->bus_fmt = nx_bus_fmt;
		me->width = format->format.width;
		me->height = format->format.height;
	}
#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
	else if (pad == 1) {
		struct v4l2_subdev_format fmt;
		/* set memory format */
		u32 nx_mem_fmt;
		int ret = nx_vip_find_nx_mem_format(format->format.code,
						    &nx_mem_fmt);
		if (ret) {
			dev_err(&me->pdev->dev, "Unsupported mem format %d\n",
			       format->format.code);
			return ret;
		}
		me->mem_fmt = nx_mem_fmt;
		me->width = format->format.width;
		me->height = format->format.height;

		memset(&fmt, 0, sizeof(fmt));
		fmt.format.width = me->width;
		fmt.format.height = me->height;

		return v4l2_subdev_call(remote, pad, set_fmt, NULL, &fmt);
	}
#endif
	else {
		dev_err(&me->pdev->dev, "%d is invalid pad value for set_fmt\n",
			pad);
		return -EINVAL;
	}

	return 0;
}

static const struct v4l2_subdev_video_ops nx_clipper_video_ops = {
	.s_stream = nx_clipper_s_stream,
	.g_crop = nx_clipper_g_crop,
	.s_crop = nx_clipper_s_crop,
	.g_parm = nx_clipper_g_parm,
	.s_parm = nx_clipper_s_parm,
};

static const struct v4l2_subdev_pad_ops nx_clipper_pad_ops = {
	.get_selection = nx_clipper_get_selection,
	.set_selection = nx_clipper_set_selection,
	.get_fmt = nx_clipper_get_fmt,
	.set_fmt = nx_clipper_set_fmt,
};

static const struct v4l2_subdev_ops nx_clipper_subdev_ops = {
	.video = &nx_clipper_video_ops,
	.pad = &nx_clipper_pad_ops,
};

/**
 * media_entity_operations
 */
static int nx_clipper_link_setup(struct media_entity *entity,
				 const struct media_pad *local,
				 const struct media_pad *remote,
				 u32 flags)
{
#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
	struct v4l2_subdev *sd = media_entity_to_v4l2_subdev(entity);
	struct nx_clipper *me = v4l2_get_subdevdata(sd);
#endif

	switch (local->index | media_entity_type(remote->entity)) {
	case NX_CLIPPER_PAD_SINK | MEDIA_ENT_T_V4L2_SUBDEV:
		pr_debug("clipper sink %s\n",
			 flags & MEDIA_LNK_FL_ENABLED ?
			 "connected" : "disconnected");
		break;
	case NX_CLIPPER_PAD_SOURCE_DECIMATOR | MEDIA_ENT_T_V4L2_SUBDEV:
		pr_debug("clipper source decimator %s\n",
			 flags & MEDIA_LNK_FL_ENABLED ?
			 "connected" : "disconnected");
		break;
#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
	case NX_CLIPPER_PAD_SOURCE_MEM | MEDIA_ENT_T_DEVNODE:
		pr_debug("clipper source mem %s\n",
			 flags & MEDIA_LNK_FL_ENABLED ?
			 "connected" : "disconnected");
		handle_video_connection(me, flags & MEDIA_LNK_FL_ENABLED ?
					true : false);
		break;
#endif
	}

	return 0;
}

static const struct media_entity_operations nx_clipper_media_ops = {
	.link_setup = nx_clipper_link_setup,
};

/**
 * initialization
 */
static void init_me(struct nx_clipper *me)
{
	NX_ATOMIC_SET(&me->state, STATE_IDLE);
	init_completion(&me->stop_done);
	sema_init(&me->s_stream_sem, 1);
#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
	nx_video_init_vbuf_obj(&me->vbuf_obj);
#endif
}

static int init_v4l2_subdev(struct nx_clipper *me)
{
	int ret;
	struct v4l2_subdev *sd = &me->subdev;
	struct media_pad *pads = me->pads;
	struct media_entity *entity = &sd->entity;

	v4l2_subdev_init(sd, &nx_clipper_subdev_ops);
	snprintf(sd->name, sizeof(sd->name), "%s%d", NX_CLIPPER_DEV_NAME,
		 me->module);
	v4l2_set_subdevdata(sd, me);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	pads[NX_CLIPPER_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	pads[NX_CLIPPER_PAD_SOURCE_MEM].flags = MEDIA_PAD_FL_SOURCE;
	pads[NX_CLIPPER_PAD_SOURCE_DECIMATOR].flags = MEDIA_PAD_FL_SOURCE;

	entity->ops = &nx_clipper_media_ops;
	ret = media_entity_init(entity, NX_CLIPPER_PAD_MAX, pads, 0);
	if (ret < 0) {
		dev_err(&me->pdev->dev, "failed to media_entity_init\n");
		return ret;
	}

	return 0;
}

static struct camera_sensor_info {
	int is_mipi;
	char name[V4L2_SUBDEV_NAME_SIZE];
} camera_sensor_info[NUMBER_OF_VIP_MODULE];

static ssize_t camera_sensor_show_common(struct device *dev,
	struct device_attribute *attr, char **buf, u32 module)
{
	struct attribute *at;

	at = &attr->attr;

	if (!strlen(camera_sensor_info[module].name))
		return scnprintf(*buf, PAGE_SIZE, "no exist");
	else
		return scnprintf(*buf, PAGE_SIZE, "is_mipi:%d,name:%s",
				 camera_sensor_info[module].is_mipi,
				 camera_sensor_info[module].name);
}

static ssize_t camera_sensor_show0(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return camera_sensor_show_common(dev, attr, &buf, 0);
}

static ssize_t camera_sensor_show1(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return camera_sensor_show_common(dev, attr, &buf, 1);
}

static ssize_t camera_sensor_show2(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return camera_sensor_show_common(dev, attr, &buf, 2);
}

static struct device_attribute camera_sensor0_attr =
__ATTR(info, 0644, camera_sensor_show0, NULL);
static struct device_attribute camera_sensor1_attr =
__ATTR(info, 0644, camera_sensor_show1, NULL);
static struct device_attribute camera_sensor2_attr =
__ATTR(info, 0644, camera_sensor_show2, NULL);

static struct attribute *camera_sensor_attrs[] = {
	&camera_sensor0_attr.attr,
	&camera_sensor1_attr.attr,
	&camera_sensor2_attr.attr,
};

static int create_sysfs_for_camera_sensor(struct nx_clipper *me,
					  struct nx_v4l2_i2c_board_info *info)
{
	int ret;
	struct kobject *kobj;
	char kobject_name[16] = {0, };
	char sensor_name[V4L2_SUBDEV_NAME_SIZE];

	memset(sensor_name, 0, V4L2_SUBDEV_NAME_SIZE);
	sprintf(sensor_name, "%s %d-%04x",
		info->board_info.type,
		info->i2c_adapter_id,
		info->board_info.addr);

	strcpy(camera_sensor_info[me->module].name, sensor_name);
	camera_sensor_info[me->module].is_mipi =
		me->interface_type == NX_CAPTURE_INTERFACE_MIPI_CSI;

	sprintf(kobject_name, "camerasensor%d", me->module);
	kobj = kobject_create_and_add(kobject_name, &platform_bus.kobj);
	if (!kobj) {
		dev_err(&me->pdev->dev, "failed to kobject_create for module %d\n",
			me->module);
		return -EINVAL;
	}

	ret = sysfs_create_file(kobj, camera_sensor_attrs[me->module]);
	if (ret) {
		dev_err(&me->pdev->dev, "failed to sysfs_create_file for module %d\n",
			me->module);
		kobject_put(kobj);
	}

	return 0;
}

static int init_sensor_media_entity(struct nx_clipper *me,
				    struct v4l2_subdev *sd)
{
	if (!sd->entity.links) {
		struct media_pad *pad = &me->sensor_pad;

		pad->flags = MEDIA_PAD_FL_SOURCE;
		sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
		return media_entity_init(&sd->entity, 1, pad, 0);
	}

	return 0;
}

static int setup_link(struct media_pad *src, struct media_pad *dst)
{
	struct media_link *link;

	link = media_entity_find_link(src, dst);
	if (link == NULL)
		return -ENODEV;

	return __media_entity_setup_link(link, MEDIA_LNK_FL_ENABLED);
}

static int register_sensor_subdev(struct nx_clipper *me)
{
	int ret;
	struct i2c_adapter *adapter;
	struct v4l2_subdev *sensor;
	struct i2c_client *client;
	struct media_entity *input;
	u32 pad;
	struct nx_v4l2_i2c_board_info *info = &me->sensor_info;

	adapter = i2c_get_adapter(info->i2c_adapter_id);
	if (!adapter) {
		dev_err(&me->pdev->dev, "unable to get sensor i2c adapter\n");
		return -ENODEV;
	}

	request_module(I2C_MODULE_PREFIX "%s", info->board_info.type);
	client = i2c_new_device(adapter, &info->board_info);
	if (client == NULL || client->dev.driver == NULL) {
		ret = -ENODEV;
		goto error;
	}

	if (!try_module_get(client->dev.driver->owner)) {
		ret = -ENODEV;
		goto error;
	}
	sensor = i2c_get_clientdata(client);
	sensor->host_priv = info;

	ret = create_sysfs_for_camera_sensor(me, info);
	if (ret)
		goto error;

	ret = init_sensor_media_entity(me, sensor);
	if (ret) {
		dev_err(&me->pdev->dev, "failed to init sensor media entity\n");
		goto error;
	}

	ret  = nx_v4l2_register_subdev(sensor);
	if (ret) {
		dev_err(&me->pdev->dev, "failed to register subdev sensor\n");
		goto error;
	}

	if (me->interface_type == NX_CAPTURE_INTERFACE_MIPI_CSI) {
		struct v4l2_subdev *mipi_csi;

		mipi_csi = nx_v4l2_get_subdev("nx-csi");
		if (!mipi_csi) {
			dev_err(&me->pdev->dev, "can't get mipi_csi subdev\n");
			goto error;
		}

		ret = media_entity_create_link(&mipi_csi->entity, 1,
					       &me->subdev.entity, 0, 0);
		if (ret < 0) {
			dev_err(&me->pdev->dev,
				"failed to create link from csi to clipper\n");
			goto error;
		}

		ret = setup_link(&mipi_csi->entity.pads[1],
				 &me->subdev.entity.pads[0]);
		if (ret)
			BUG();

		input = &mipi_csi->entity;
		pad = 0;
	} else {
		input = &me->subdev.entity;
		pad = NX_CLIPPER_PAD_SINK;
	}

	ret = media_entity_create_link(&sensor->entity, 0, input, pad, 0);
	if (ret < 0)
		dev_err(&me->pdev->dev,
			"failed to create link from sensor\n");

	ret = setup_link(&sensor->entity.pads[0], &input->pads[pad]);
	if (ret)
		BUG();

error:
	if (client && ret < 0)
		i2c_unregister_device(client);

	return ret;
}

static int register_v4l2(struct nx_clipper *me)
{
	int ret;
#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
	char dev_name[64] = {0, };
	struct media_entity *entity = &me->subdev.entity;
	struct nx_video *video;
#endif

	ret = nx_v4l2_register_subdev(&me->subdev);
	if (ret)
		BUG();

	ret = register_sensor_subdev(me);
	if (ret) {
		dev_err(&me->pdev->dev, "can't register sensor subdev\n");
		return ret;
	}

#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
	snprintf(dev_name, sizeof(dev_name), "VIDEO CLIPPER%d", me->module);
	video = nx_video_create(dev_name, NX_VIDEO_TYPE_CAPTURE,
				    nx_v4l2_get_v4l2_device(),
				    nx_v4l2_get_alloc_ctx());
	if (!video)
		BUG();


	ret = media_entity_create_link(entity, NX_CLIPPER_PAD_SOURCE_MEM,
				       &video->vdev.entity, 0, 0);
	if (ret < 0)
		BUG();

	me->vbuf_obj.video = video;

	ret = setup_link(&entity->pads[NX_CLIPPER_PAD_SOURCE_MEM],
			 &video->vdev.entity.pads[0]);
	if (ret)
		BUG();
#endif

	return 0;
}

static void unregister_v4l2(struct nx_clipper *me)
{
#ifdef CONFIG_VIDEO_NEXELL_CLIPPER
	if (me->vbuf_obj.video) {
		nx_video_cleanup(me->vbuf_obj.video);
		me->vbuf_obj.video = NULL;
	}
#endif
	v4l2_device_unregister_subdev(&me->subdev);
}

/**
 * platform driver
 */
static int nx_clipper_probe(struct platform_device *pdev)
{
	int ret;
	struct nx_clipper *me;
	struct device *dev = &pdev->dev;

	me = devm_kzalloc(dev, sizeof(*me), GFP_KERNEL);
	if (!me) {
		WARN_ON(1);
		return -ENOMEM;
	}
	me->pdev = pdev;

	ret = nx_clipper_parse_dt(dev, me);
	if (ret) {
		dev_err(dev, "failed to parse dt\n");
		return ret;
	}

	if (!nx_vip_is_valid(me->module)) {
		dev_err(dev, "NX VIP %d is not valid\n", me->module);
		return -ENODEV;
	}

	init_me(me);

	ret = init_v4l2_subdev(me);
	if (ret)
		return ret;

	ret = register_v4l2(me);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, me);

	return 0;
}

static int nx_clipper_remove(struct platform_device *pdev)
{
	struct nx_clipper *me = platform_get_drvdata(pdev);

	if (unlikely(!me))
		return 0;

	unregister_v4l2(me);

	return 0;
}

static struct platform_device_id nx_clipper_id_table[] = {
	{ NX_CLIPPER_DEV_NAME, 0 },
	{},
};

static const struct of_device_id nx_clipper_dt_match[] = {
	{ .compatible = "nexell,nx-clipper" },
	{},
};
MODULE_DEVICE_TABLE(of, nx_clipper_dt_match);

static struct platform_driver nx_clipper_driver = {
	.probe		= nx_clipper_probe,
	.remove		= nx_clipper_remove,
	.id_table	= nx_clipper_id_table,
	.driver		= {
		.name	= NX_CLIPPER_DEV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(nx_clipper_dt_match),
	},
};

module_platform_driver(nx_clipper_driver);

MODULE_AUTHOR("swpark <swpark@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell S5Pxx18 series SoC V4L2 capture clipper driver");
MODULE_LICENSE("GPL");
