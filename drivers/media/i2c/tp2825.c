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

/*#define DEBUG_TP2825*/
#ifdef DEBUG_TP2825
#define vmsg(a...)  printk(a)
#else
#define vmsg(a...)
#endif

struct tp2825_state {
	struct media_pad pad;
	struct v4l2_subdev sd;
	bool first;

	struct i2c_client *i2c_client;
};

struct reg_val {
	uint8_t reg;
	uint8_t val;
};

#define END_MARKER {0xff, 0xff}

static struct reg_val _sensor_init_data[] = {
	/* video */
	{0x40, 0x00},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x39, 0x8c},
	{0x4d, 0x03},
	{0x4e, 0x17},
	/* PTZ */
	{0xc8, 0x21},
	{0x7e, 0x01},
	{0xb9, 0x01},
	/* Data set */
	{0x02, 0xcf},
	{0x15, 0x13},
	{0x16, 0x4e},
	{0x17, 0xd0},
	{0x18, 0x15},
	{0x19, 0xf0},
	{0x1a, 0x02},
	{0x1c, 0x09},
	{0x1d, 0x38},

	{0x0c, 0x53},
	{0x0d, 0x10},
	{0x20, 0xa0},
	{0x26, 0x12},
	{0x2b, 0x70},
	{0x2d, 0x68},
	{0x2e, 0x5e},

	{0x30, 0x62},
	{0x31, 0xbb},
	{0x32, 0x96},
	{0x33, 0xc0},
	{0x35, 0x25},
	{0x39, 0x84},

	END_MARKER
};

static struct tp2825_state _state;

/**
 * util functions
 */
#define THINE_I2C_RETRY_CNT				3

#ifdef DEBUG_TP2825
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
#endif

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

static int tp2825_s_stream(struct v4l2_subdev *sd, int enable)
{
	if (enable) {
		if (_state.first) {
			int  i = 0;
			struct tp2825_state *me = &_state;
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

static int tp2825_s_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *fmt)
{
	vmsg("%s\n", __func__);
	return 0;
}

static const struct v4l2_subdev_pad_ops tp2825_subdev_pad_ops = {
	.set_fmt = tp2825_s_fmt,
};

static const struct v4l2_subdev_video_ops tp2825_subdev_video_ops = {
	.s_stream = tp2825_s_stream,
};

static const struct v4l2_subdev_ops tp2825_ops = {
	.video = &tp2825_subdev_video_ops,
	.pad   = &tp2825_subdev_pad_ops,
};

static int tp2825_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tp2825_state *state = &_state;
	int ret;

	sd = &state->sd;
	strcpy(sd->name, "tp2825");

	v4l2_i2c_subdev_init(sd, client, &tp2825_ops);

	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	state->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
	ret = media_entity_init(&sd->entity, 1, &state->pad, 0);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to media_entity_init()\n",
			__func__);
		return ret;
	}

	i2c_set_clientdata(client, sd);
	state->i2c_client = client;
	state->first = true;

	return 0;
}

static int tp2825_remove(struct i2c_client *client)
{
	struct tp2825_state *state = &_state;

	v4l2_device_unregister_subdev(&state->sd);
	return 0;
}

static const struct i2c_device_id tp2825_id[] = {
	{ "tp2825", 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, tp2825_id);

static struct i2c_driver tp2825_i2c_driver = {
	.driver = {
		.name = "tp2825",
	},
	.probe = tp2825_probe,
	.remove = tp2825_remove,
	.id_table = tp2825_id,
};

module_i2c_driver(tp2825_i2c_driver);

MODULE_DESCRIPTION("TP2825 Camera Sensor Driver");
MODULE_AUTHOR("<jkchoi@nexell.co.kr>");
MODULE_LICENSE("GPL");
