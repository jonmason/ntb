/*
 * MELFAS MIP4 Touchscreen
 *
 * Copyright (C) 2000-2017 MELFAS Inc.
 *
 *
 * mip4_ts_mod.c : Model dependent functions
 *
 * Version : 2017.11.13
 */

#include "mip4_ts.h"


/*
* Pre-run config
*/
int mip4_ts_startup_config(struct mip4_ts_info *info)
{
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (info->disable_esd) {
		dev_dbg(&info->client->dev, "%s - disable_esd\n", __func__);
		mip4_ts_disable_esd_alert(info);
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;
}

/*
* Config regulator
*/
int mip4_ts_config_regulator(struct mip4_ts_info *info)
{
	int ret = 0;

#ifdef CONFIG_REGULATOR
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	info->regulator_vd33 = regulator_get(&info->client->dev, "vd33");

	if (IS_ERR_OR_NULL(info->regulator_vd33)) {
		dev_err(&info->client->dev,
			"%s [ERROR] regulator_get : vd33\n", __func__);
		ret = PTR_ERR(info->regulator_vd33);
	} else {
		dev_dbg(&info->client->dev, "%s - regulator_get : vd33\n", __func__);

		ret = regulator_set_voltage(info->regulator_vd33, 3300000, 3300000);
		if (ret) {
			dev_err(&info->client->dev,
					"%s [ERROR] regulator_set_voltage : vd33\n", __func__);
		} else {
			dev_dbg(&info->client->dev,
					"%s - regulator_set_voltage : 3300000\n", __func__);
		}
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
#endif

	return ret;
}

/*
* Control regulator
*/
int mip4_ts_control_regulator(struct mip4_ts_info *info, int enable)
{
#ifdef CONFIG_REGULATOR
	int ret = 0;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);
	dev_dbg(&info->client->dev, "%s - switch : %d\n", __func__, enable);

	if (info->power == enable) {
		dev_dbg(&info->client->dev, "%s - skip\n", __func__);
		goto exit;
	}

	if (IS_ERR_OR_NULL(info->regulator_vd33)) {
		dev_err(&info->client->dev,
				"%s [ERROR] regulator_vd33 not found\n", __func__);
		goto exit;
	}

	if (enable) {
		ret = regulator_enable(info->regulator_vd33);
		if (ret) {
			dev_err(&info->client->dev,
					"%s [ERROR] regulator_enable : vd33\n", __func__);
			goto error;
		} else {
			dev_dbg(&info->client->dev, "%s - regulator_enable\n", __func__);
		}

#ifdef CONFIG_OF
		if (!IS_ERR_OR_NULL(info->pinctrl)) {
			ret = pinctrl_select_state(info->pinctrl, info->pins_enable);
			if (ret < 0) {
				dev_err(&info->client->dev,
				"%s [ERROR] pinctrl_select_state : pins_enable\n", __func__);
			}
		} else {
			dev_err(&info->client->dev,
				"%s [ERROR] pinctrl not found\n", __func__);
		}
#endif /* CONFIG_OF */
	} else {
		if (regulator_is_enabled(info->regulator_vd33)) {
			regulator_disable(info->regulator_vd33);
			dev_dbg(&info->client->dev, "%s - regulator_disable\n", __func__);
		}

#ifdef CONFIG_OF
		if (!IS_ERR_OR_NULL(info->pinctrl)) {
			ret = pinctrl_select_state(info->pinctrl, info->pins_disable);
			if (ret < 0) {
				dev_err(&info->client->dev,
				"%s [ERROR] pinctrl_select_state : pins_disable\n", __func__);
			}
		} else {
			dev_err(&info->client->dev,
				"%s [ERROR] pinctrl not found\n", __func__);
		}
#endif /* CONFIG_OF */
	}
	info->power = enable;

exit:
	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return ret;
#else
	return 0;
#endif /* CONFIG_REGULATOR */
}

/*
* Turn off power supply
*/
int mip4_ts_power_off(struct mip4_ts_info *info)
{
	int __maybe_unused ret = 0;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	/* Use CE pin */
#if 1
	if (info->gpio_ce) {
		gpio_direction_output(info->gpio_ce, 0);
		dev_dbg(&info->client->dev, "%s - gpio_ce : 0\n", __func__);
	}
#endif

	/* Use VD33 regulator */
#if 0
	mip4_ts_control_regulator(info, 0);
#endif

	/* Use VD33_EN pin */
#if 0
	if (info->gpio_vd33_en) {
		gpio_direction_output(info->gpio_vd33_en, 0);
		dev_dbg(&info->client->dev, "%s - gpio_vd33_en : 0\n", __func__);
	}
#endif

#ifdef CONFIG_OF
	/* Use pinctrl */
#if 0
	if (!IS_ERR_OR_NULL(info->pinctrl)) {
		ret = pinctrl_select_state(info->pinctrl, info->pins_disable);
		if (ret < 0) {
			dev_err(&info->client->dev,
				"%s [ERROR] pinctrl_select_state : pins_disable\n", __func__);
		} else {
			dev_dbg(&info->client->dev,
				"%s - pinctrl_select_state : disable\n", __func__);
		}
	}
#endif
#endif /* CONFIG_OF */

	usleep_range(1 * 1000, 2 * 1000);
	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;
}

/*
* Turn on power supply
*/
int mip4_ts_power_on(struct mip4_ts_info *info)
{
	int __maybe_unused ret = 0;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	/* Use VD33 regulator */
#if 0
	mip4_ts_control_regulator(info, 1);
#endif

	/* Use VD33_EN pin */
#if 0
	if (info->gpio_vd33_en) {
		gpio_direction_output(info->gpio_vd33_en, 1);
		dev_dbg(&info->client->dev, "%s - gpio_vd33_en : 1\n", __func__);
	}
#endif

	/* Use CE pin */
#if 1
	if (info->gpio_ce) {
		gpio_direction_output(info->gpio_ce, 1);
		dev_dbg(&info->client->dev, "%s - gpio_ce : 1\n", __func__);
	}
#endif

#ifdef CONFIG_OF
	/* Use pinctrl */
#if 0
	if (!IS_ERR_OR_NULL(info->pinctrl)) {
		ret = pinctrl_select_state(info->pinctrl, info->pins_enable);
		if (ret < 0) {
			dev_err(&info->client->dev, "%s [ERROR] pinctrl_select_state : pins_enable\n", __func__);
		} else {
			dev_dbg(&info->client->dev, "%s - pinctrl_select_state : enable\n", __func__);
		}
	}
#endif
#endif /* CONFIG_OF */

#if !USE_STARTUP_WAITING
	msleep(200);
#endif

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;
}

/*
* Clear touch input event status
*/
void mip4_ts_clear_input(struct mip4_ts_info *info)
{
	int i;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	/* Screen */
	for (i = 0; i < MAX_FINGER_NUM; i++) {

		input_mt_slot(info->input_dev, i);
		input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, 0);

		info->touch_state[i] = 0;

	}
	input_sync(info->input_dev);

	/* Key */
	if (info->key_enable == true) {
		for (i = 0; i < info->key_num; i++) {
			input_report_key(info->input_dev, info->key_code[i], 0);
		}
	}
	input_sync(info->input_dev);

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
}

/*
* Input event handler - Report input event
*/
void mip4_ts_input_event_handler(struct mip4_ts_info *info, u8 sz, u8 *buf)
{
	int i,j;
	int type;
	int id;
	int direction = 0;
	int key_value=0;
	int x = 0;
	int x_delta = 0;
	int gesture = 0;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	/*
	 print_hex_dump(KERN_ERR, MIP4_TS_DEVICE_NAME
		" Event Packet : ", DUMP_PREFIX_OFFSET, 16, 1, buf, sz, false);
	 */

	for (i = 0; i < sz; i += info->event_size) {
		u8 *packet = &buf[i];

		/* Event format & type */
		if (info->event_format == 0x00) {
			type = (packet[0] & 0x80) >> 7;
		} else {
			dev_err(&info->client->dev,
				"%s [ERROR] Unknown event format [%d]\n",
					__func__, info->event_format);
			goto exit;
		}
		dev_dbg(&info->client->dev, "%s - Type[%d]\n", __func__, type);

		/* Report input event */
		if (type == MIP4_EVENT_INPUT_TYPE_SCREEN) {
			/* Slider event */
			if (info->event_format == 0x00) {
				id = 0;
				direction = (packet[0] & 0x40) >> 6;
				x_delta = ((packet[1] & 0x0F) << 8) | packet[2];
				x = ((packet[1] & 0xF0) << 4) | packet[3];
			} else {
				dev_err(&info->client->dev,
					"%s [ERROR] Unknown event format [%d]\n",
					__func__, info->event_format);
				goto exit;
			}

			if (!((id >= 0) && (id < MAX_FINGER_NUM))) {
				dev_err(&info->client->dev,
					"%s [ERROR] Unknown finger id [%d]\n", __func__, id);
				continue;
			}

			dev_err(&info->client->dev,
					"%s - Slider : direction[%d] x_delta[%d] x[%d]\n",
						__func__, direction, x_delta, x);

			key_value = (direction) ? KEY_VOLUMEUP : KEY_VOLUMEDOWN;

			for(j=0;j<x_delta;j++) {
				input_report_key(info->input_dev, key_value, 1);
				input_sync(info->input_dev);
				input_report_key(info->input_dev, key_value, 0);
				input_sync(info->input_dev);
			}
			//
			/////////////////////////////////
		} else if (type == MIP4_EVENT_INPUT_TYPE_KEY) {
			/* Key event */
			if (info->event_format == 0x00) {
				id = (packet[0] & 0x0F) - 1;
				gesture = (packet[0] & 0x70) >> 4;
			} else {
				dev_err(&info->client->dev, "%s [ERROR] Unknown event format [%d]\n", __func__, info->event_format);
				goto exit;
			}

			if ((id >= 0) && (id < info->key_num)) {

				int keycode = info->key_code[id];

				dev_err(&info->client->dev,
					"%s - Key : id[%d] gesture[%d] keycode[%d]\n",
						__func__, id, gesture, keycode);

				input_report_key(info->input_dev, keycode, 1);
				input_sync(info->input_dev);

				input_report_key(info->input_dev, keycode, 0);
				input_sync(info->input_dev);

			} else {
				dev_err(&info->client->dev,
					"%s [ERROR] Unknown key id [%d]\n", __func__, id);
				continue;
			}
		} else {
			dev_err(&info->client->dev,
					"%s [ERROR] Unknown event type [%d]\n", __func__, type);
			goto exit;
		}
	}

exit:
	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
}

/*
* Wake-up gesture event handler
*/
int mip4_ts_gesture_wakeup_event_handler(struct mip4_ts_info *info,
		int gesture_code)
{

	int i;
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	/* Report gesture event */
	dev_err(&info->client->dev, "%s - gesture[%d]\n", __func__, gesture_code);

	info->wakeup_gesture_code = gesture_code;

	switch (gesture_code) {
	case 24:
		/* Center - Double tap */
		input_report_key(info->input_dev, KEY_PLAYPAUSE, 1);
		input_sync(info->input_dev);
		input_report_key(info->input_dev, KEY_PLAYPAUSE, 0);
		input_sync(info->input_dev);
		break;
	case 25:
		/* Right - Single tap */
		for(i=0;i<7;i++) {
			input_report_key(info->input_dev, KEY_VOLUMEUP, 1);
			input_sync(info->input_dev);
			input_report_key(info->input_dev, KEY_VOLUMEUP, 0);
			input_sync(info->input_dev);
		}
		break;
	case 26:
		/* Left - Single tap */
		for(i=0;i<7;i++) {
			input_report_key(info->input_dev, KEY_VOLUMEDOWN, 1);
			input_sync(info->input_dev);
			input_report_key(info->input_dev, KEY_VOLUMEDOWN, 0);
			input_sync(info->input_dev);
		}
		break;
	default:
		dev_err(&info->client->dev,
			"%s [ERROR] Unknown gesture code[%d]\n", __func__, gesture_code);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	return 1;
}

/*
* Config GPIO
*/
int mip4_ts_config_gpio(struct mip4_ts_info *info)
{
	int ret = 0;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	/* Interrupt */
	dev_dbg(&info->client->dev, "%s - gpio_intr[%d]\n", __func__,
		info->gpio_intr);
	if (info->gpio_intr) {
		ret = gpio_request(info->gpio_intr, "irq-gpio");
		if (ret < 0) {
			dev_err(&info->client->dev,
				"%s [ERROR] gpio_request : irq-gpio\n", __func__);
			goto error;
		} else {
			gpio_direction_input(info->gpio_intr);

			/* Set IRQ */
			info->client->irq = gpio_to_irq(info->gpio_intr);
			info->irq = info->client->irq;
			dev_dbg(&info->client->dev,
				"%s - gpio_to_irq : irq[%d]\n", __func__, info->irq);
		}
	}

	/* CE (Optional) */
	dev_dbg(&info->client->dev, "%s - gpio_ce[%d]\n", __func__, info->gpio_ce);
	if (info->gpio_ce) {
		ret = gpio_request(info->gpio_ce, "ce-gpio");
		if (ret < 0) {
			dev_err(&info->client->dev,
				"%s [ERROR] gpio_request : ce-gpio\n", __func__);
		} else {
			gpio_direction_output(info->gpio_ce, 0);
		}
	}

	/* VD33_EN (Optional) */
	dev_dbg(&info->client->dev,
			"%s - gpio_vd33_en[%d]\n", __func__, info->gpio_vd33_en);
	if (info->gpio_vd33_en) {
		ret = gpio_request(info->gpio_vd33_en, "vd33_en-gpio");
		if (ret < 0) {
			dev_err(&info->client->dev,
			"%s [ERROR] gpio_request : vd33_en-gpio\n", __func__);
		} else {
			gpio_direction_output(info->gpio_vd33_en, 0);
		}
	}

#ifdef CONFIG_OF
	/* Pinctrl (Optional) */
#if 0
	if (!IS_ERR_OR_NULL(info->pinctrl)) {
		ret = pinctrl_select_state(info->pinctrl, info->pins_enable);
		if (ret < 0) {
			dev_err(&info->client->dev,
				"%s [ERROR] pinctrl_select_state : pins_enable\n", __func__);
		} else {
			dev_dbg(&info->client->dev,
				"%s - pinctrl_select_state : enable\n", __func__);
		}
	}
#endif
#endif

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 0;
}

#ifdef CONFIG_OF
/*
* Parse device tree
*/
int mip4_ts_parse_devicetree(struct device *dev, struct mip4_ts_info *info)
{
	struct device_node *np = dev->of_node;
	int ret;

	dev_dbg(dev, "%s [START]\n", __func__);

	/* Get Interrupt GPIO */
	ret = of_get_named_gpio(np, "irq-gpio", 0);
	if (!gpio_is_valid(ret)) {
		dev_err(&info->client->dev,
			"%s [ERROR] of_get_named_gpio : irq-gpio\n", __func__);
		info->gpio_intr = 0;
	} else {
		info->gpio_intr = ret;
	}
	dev_dbg(dev, "%s - gpio_intr[%d]\n", __func__, info->gpio_intr);

	/* Get CE GPIO (Optional) */
	ret = of_get_named_gpio(np, "ce-gpio", 0);
	if (!gpio_is_valid(ret)) {
		dev_err(&info->client->dev,
			"%s [ERROR] of_get_named_gpio : ce-gpio\n", __func__);
		info->gpio_ce = 0;
	} else {
		info->gpio_ce = ret;
	}
	dev_dbg(dev, "%s - gpio_ce[%d]\n", __func__, info->gpio_ce);

	/* Get VD33_EN GPIO (Optional) */
#if 0
	ret = of_get_named_gpio(np, "vd33_en-gpio", 0);
	if (!gpio_is_valid(ret)) {
		dev_err(&info->client->dev,
			"%s [ERROR] of_get_named_gpio : vd33_en-gpio\n", __func__);
		info->gpio_vd33_en = 0;
	} else {
		info->gpio_vd33_en = ret;
	}
	dev_dbg(dev, "%s - gpio_vd33_en[%d]\n", __func__, info->gpio_vd33_en);

	/* Get Pinctrl (Optional) */
	info->pinctrl = devm_pinctrl_get(&info->client->dev);
	if (IS_ERR(info->pinctrl)) {
		dev_err(&info->client->dev, "%s [ERROR] devm_pinctrl_get\n", __func__);
	} else {
		info->pins_enable = pinctrl_lookup_state(info->pinctrl, "enable");
		if (IS_ERR(info->pins_enable)) {
			dev_err(&info->client->dev,
				"%s [ERROR] pinctrl_lookup_state : enable\n", __func__);
		}

		info->pins_disable = pinctrl_lookup_state(info->pinctrl, "disable");
		if (IS_ERR(info->pins_disable)) {
			dev_err(&info->client->dev,
				"%s [ERROR] pinctrl_lookup_state : disable\n", __func__);
		}
	}
#endif

	/* Config GPIO */
	ret = mip4_ts_config_gpio(info);
	if (ret) {
		dev_err(&info->client->dev,
				"%s [ERROR] mip4_ts_config_gpio\n", __func__);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}
#endif

/*
* Config input interface
*/
void mip4_ts_config_input(struct mip4_ts_info *info)
{
	struct input_dev *input_dev = info->input_dev;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	set_bit(EV_SYN, input_dev->evbit);

	/* Slider */
	set_bit(EV_ABS, input_dev->evbit);
	input_set_abs_params(input_dev, ABS_X, 0, info->max_x, 0, 0);

	set_bit(EV_REL, input_dev->evbit);
	set_bit(REL_X, input_dev->relbit);

	/* Key */
	set_bit(EV_KEY, input_dev->evbit);

	set_bit(KEY_PLAYPAUSE, input_dev->keybit);
	set_bit(KEY_VOLUMEDOWN, input_dev->keybit);
	set_bit(KEY_VOLUMEUP, input_dev->keybit);

	info->key_code[0] = KEY_PLAYPAUSE;
	info->key_code[1] = KEY_VOLUMEDOWN;
	info->key_code[2] = KEY_VOLUMEUP;

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
}

