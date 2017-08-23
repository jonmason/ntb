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
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>

#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>

/* #define DEBUG_TW9900 */
#ifdef DEBUG_TW9900
#define vmsg(a...)  printk(a)
#else
#define vmsg(a...)
#endif

struct nx_resolution {
	uint32_t width;
	uint32_t height;
	uint32_t interval;
};

static struct nx_resolution supported_resolutions[] = {
	{
		.width	= 704,
		.height = 480,
		.interval = 30,
	}/*,
	{
		.width	= 640,
		.height = 480,
		.interval = 30,
	}*/
};

static const uint32_t avail_fps_range[] = {
	15, 30
};

/* #define BRIGHTNESS_TEST */
#define DEFAULT_BRIGHTNESS  0x1e
struct tw9900_state {
	struct media_pad pad;
	struct v4l2_subdev sd;
	bool first;

	struct i2c_client *i2c_client;

	struct v4l2_ctrl_handler handler;

	/* custom control */
	struct v4l2_ctrl *ctrl_mux;
	struct v4l2_ctrl *ctrl_status;

	/* standard control */
	struct v4l2_ctrl *ctrl_brightness;
	int brightness;
};

struct reg_val {
	uint8_t reg;
	uint8_t val;
};

#define END_MARKER {0xff, 0xff}

static struct reg_val _sensor_init_data[] = {
	{0x02, 0x40},
	{0x1c, 0x00},
	{0x03, 0xa6},
	{0x06, 0x80},
	{0x07, 0x02},
	{0x08, 0x15},
	{0x09, 0xf0},
	{0x0a, 0x09},
	{0x0b, 0xc0},
	{0x10, 0xec},
	{0x11, 0x68},
	{0x19, 0x50},
	{0x1b, 0x00},
	{0x1a, 0x0f},
	{0x2f, 0xe6},
	{0x55, 0x00},
	{0xaf, 0x40},
	{0xb1, 0x20},
	{0xb4, 0x20},

	END_MARKER
};

static struct tw9900_state _state;

static int brightness_tbl[12]
= {30, -15, -12, -9, -6, -3, 0, 10, 20, 30, 35, 40};

/**
 * util functions
 */
static inline struct tw9900_state *ctrl_to_me(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct tw9900_state, handler);
}

#define THINE_I2C_RETRY_CNT				3
static int _i2c_read_byte(struct i2c_client *client, u8 addr, u8 *data)
{
	s8 i = 0;
	s8 ret = 0;
	u8 buf = 0;
	struct i2c_msg msg[2];

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &addr;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = &buf;

	for (i = 0; i < THINE_I2C_RETRY_CNT; i++) {
		ret = i2c_transfer(client->adapter, msg, 2);
		if (likely(ret == 2))
			break;
	}

	if (unlikely(ret != 2)) {
		dev_err(&client->dev, "_i2c_read_byte failed reg:0x%02x\n",
			addr);
		return -EIO;
	}

	*data = buf;
	return 0;
}

static int _i2c_write_byte(struct i2c_client *client, u8 addr, u8 val)
{
	s8 i = 0;
	s8 ret = 0;
	u8 buf[2];
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = buf;

	buf[0] = addr;
	buf[1] = val;

	for (i = 0; i < THINE_I2C_RETRY_CNT; i++) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (likely(ret == 1))
			break;
	}

	if (ret != 1) {
		pr_err("%s: failed to write addr 0x%x, val 0x%x\n",
		__func__, addr, val);
		return -EIO;
	}

	return 0;
}

#define V4L2_CID_MUX        (V4L2_CTRL_CLASS_USER | 0x1001)
#define V4L2_CID_STATUS     (V4L2_CTRL_CLASS_USER | 0x1002)

static int tw9900_set_mux(struct v4l2_ctrl *ctrl)
{
	struct tw9900_state *me = ctrl_to_me(ctrl);

	if (ctrl->val == 0) {
		/* MUX 0 */
		if (brightness_tbl[me->brightness] !=
				DEFAULT_BRIGHTNESS)
			_i2c_write_byte(me->i2c_client, 0x10,
				DEFAULT_BRIGHTNESS);
		_i2c_write_byte(me->i2c_client, 0x02, 0x40);
	} else {
		/* MUX 1 */
		if (brightness_tbl[me->brightness] !=
			DEFAULT_BRIGHTNESS)
			_i2c_write_byte(me->i2c_client, 0x10,
				brightness_tbl[me->brightness]);
		_i2c_write_byte(me->i2c_client, 0x02, 0x44);
	}

	return 0;
}

static int tw9900_set_brightness(struct v4l2_ctrl *ctrl)
{
	struct tw9900_state *me = ctrl_to_me(ctrl);

#ifdef BRIGHTNESS_TEST
	pr_err("%s: brightness = %d\n", __func__, ctrl->val);
	_i2c_write_byte(me->i2c_client, 0x10, ctrl->val);
#else
	if (ctrl->val != me->brightness) {
		pr_err("%s: brightness = %d\n", __func__,
			brightness_tbl[ctrl->val]);
		_i2c_write_byte(me->i2c_client, 0x10,
			brightness_tbl[ctrl->val]);

		me->brightness = ctrl->val;
	}
#endif
	return 0;
}

static bool _is_camera_on(void);

static int tw9900_get_status(struct v4l2_ctrl *ctrl)
{
	struct tw9900_state *me = ctrl_to_me(ctrl);
	u8 data = 0;
	u8 mux;
	u8 val = 0;

	_i2c_read_byte(me->i2c_client, 0x02, &data);
	mux = (data & 0x0c) >> 2;

	if (mux == 0) {
		_i2c_read_byte(me->i2c_client, 0x01, &data);
		if (!(data & 0x80))
			val |= 1 << 0;
	} else {
		_i2c_read_byte(me->i2c_client, 0x01, &data);
		if (!(data & 0x80))
			val |= 1 << 1;
	}

	ctrl->val = val;

	return 0;
}

static int tw9900_s_ctrl(struct v4l2_ctrl *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_MUX:
		return tw9900_set_mux(ctrl);
	case V4L2_CID_BRIGHTNESS:
		return tw9900_set_brightness(ctrl);
	default:
		pr_err("%s: invalid control id 0x%x\n", __func__, ctrl->id);
		return -EINVAL;
	}
}

static int tw9900_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_STATUS:
		return tw9900_get_status(ctrl);
	default:
		pr_err("%s: invalid control id 0x%x\n", __func__, ctrl->id);
		return -EINVAL;
	}
}

static const struct v4l2_ctrl_ops tw9900_ctrl_ops = {
	.s_ctrl = tw9900_s_ctrl,
	.g_volatile_ctrl = tw9900_g_volatile_ctrl,
};

static const struct v4l2_ctrl_config tw9900_custom_ctrls[] = {
	{
		.ops  = &tw9900_ctrl_ops,
		.id   = V4L2_CID_MUX,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "MuxControl",
		.min  = 0,
		.max  = 1,
		.def  = 1,
		.step = 1,
	},
	{
		.ops  = &tw9900_ctrl_ops,
		.id   = V4L2_CID_STATUS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Status",
		.min  = 0,
		.max  = 1,
		.def  = 1,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}
};

#define NUM_CTRLS 3
static int tw9900_initialize_ctrls(struct tw9900_state *me)
{
	v4l2_ctrl_handler_init(&me->handler, NUM_CTRLS);

	me->ctrl_mux = v4l2_ctrl_new_custom(&me->handler,
		&tw9900_custom_ctrls[0], NULL);
	if (!me->ctrl_mux) {
		pr_err("%s: failed to v4l2_ctrl_new_custom for mux\n",
			__func__);
		return -ENOENT;
	}

	me->ctrl_status = v4l2_ctrl_new_custom(&me->handler,
		&tw9900_custom_ctrls[1], NULL);
	if (!me->ctrl_status) {
		pr_err("%s: failed to v4l2_ctrl_new_custom for status\n",
			__func__);
		return -ENOENT;
	}

	me->ctrl_brightness = v4l2_ctrl_new_std(&me->handler, &tw9900_ctrl_ops,
			V4L2_CID_BRIGHTNESS, -128, 127, 1, 0x1e);
	if (!me->ctrl_brightness) {
		pr_err("%s: failed to v4l2_ctrl_new_std for brightness\n",
			__func__);
		return -ENOENT;
	}

	me->sd.ctrl_handler = &me->handler;
	if (me->handler.error) {
		pr_err("%s: ctrl handler error(%d)\n", __func__,
			me->handler.error);
		v4l2_ctrl_handler_free(&me->handler);
		return -EINVAL;
	}

	return 0;
}

static inline bool _is_camera_on(void)
{
	/* read status */
	u8 data;

	_i2c_read_byte(_state.i2c_client, 0x01, &data);

	if (data & 0x80)
		return false;

	if ((data & 0x40) && (data & 0x08))
		return true;

	return false;
}

static int tw9900_s_stream(struct v4l2_subdev *sd, int enable)
{
	if (enable) {
		if (_state.first) {
			int  i = 0;
			struct tw9900_state *me = &_state;
			struct reg_val *reg_val = _sensor_init_data;

			while (reg_val->reg != 0xff) {
				_i2c_write_byte(me->i2c_client, reg_val->reg,
					reg_val->val);
				mdelay(10);
				i++;
				reg_val++;
			}
			_state.first = false;
		}
	}

	return 0;
}

static int tw9900_s_fmt(struct v4l2_subdev *sd,
		/* struct v4l2_subdev_fh *fh, */
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *fmt)
{
	vmsg("%s\n", __func__);
	return 0;
}

static int tw9900_s_power(struct v4l2_subdev *sd, int on)
{
	vmsg("%s: %d\n", __func__, on);
	return 0;
}

static int tw9900_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_frame_size_enum *frame)
{
	vmsg("%s, index:%d\n", __func__, frame->index);

	if (frame->index >= ARRAY_SIZE(supported_resolutions))
		return -ENODEV;

	frame->max_width = supported_resolutions[frame->index].width;
	frame->max_height = supported_resolutions[frame->index].height;

	return 0;
}

static int tw9900_enum_frame_interval(struct v4l2_subdev *sd,
				      struct v4l2_subdev_pad_config *cfg,
				      struct v4l2_subdev_frame_interval_enum *frame)
{
	vmsg("%s, index:%d\n", __func__, frame->index);
	if (frame->index >= ARRAY_SIZE(supported_resolutions))
		return -ENODEV;

	frame->width = supported_resolutions[frame->index].width;
	frame->height = supported_resolutions[frame->index].height;
	frame->interval.numerator = supported_resolutions[frame->index].interval;
	frame->interval.denominator = 1;
	vmsg("index:%d, width:%d, height:%d, interval:%d:%d\n", frame->index,
		frame->width, frame->height,
		frame->interval.numerator, frame->interval.denominator);
	return 0;
}

static const struct v4l2_subdev_core_ops tw9900_subdev_core_ops = {
	.s_power = tw9900_s_power,
};

static const struct v4l2_subdev_pad_ops tw9900_subdev_pad_ops = {
	.set_fmt = tw9900_s_fmt,
	.enum_frame_size = tw9900_enum_frame_size,
	.enum_frame_interval = tw9900_enum_frame_interval,
};

static const struct v4l2_subdev_video_ops tw9900_subdev_video_ops = {
	.s_stream = tw9900_s_stream,
};

static const struct v4l2_subdev_ops tw9900_ops = {
	.core  = &tw9900_subdev_core_ops,
	.video = &tw9900_subdev_video_ops,
	.pad   = &tw9900_subdev_pad_ops,
};

static int tw9900_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tw9900_state *state = &_state;
	int ret;

	vmsg("%s entered\n", __func__);

	sd = &state->sd;
	strcpy(sd->name, "tw9900");

	v4l2_i2c_subdev_init(sd, client, &tw9900_ops);

	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	state->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
	ret = media_entity_init(&sd->entity, 1, &state->pad, 0);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to media_entity_init()\n",
			__func__);
		return ret;
	}

	ret = tw9900_initialize_ctrls(state);
	if (ret < 0) {
		pr_err("%s: failed to initialize controls\n",
			__func__);
		return ret;
	}

	i2c_set_clientdata(client, sd);
	state->i2c_client = client;
	state->first = true;

	vmsg("%s exit\n", __func__);

	return 0;
}

static int tw9900_remove(struct i2c_client *client)
{
	struct tw9900_state *state = &_state;

	v4l2_device_unregister_subdev(&state->sd);
	return 0;
}

static const struct i2c_device_id tw9900_id[] = {
	{ "tw9900", 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, tw9900_id);

static struct i2c_driver tw9900_i2c_driver = {
	.driver = {
		.name = "tw9900",
	},
	.probe = tw9900_probe,
	.remove = tw9900_remove,
	.id_table = tw9900_id,
};

module_i2c_driver(tw9900_i2c_driver);

MODULE_DESCRIPTION("TW9900 Camera Sensor Driver");
MODULE_AUTHOR("<jkchoi@nexell.co.kr>");
MODULE_LICENSE("GPL");
