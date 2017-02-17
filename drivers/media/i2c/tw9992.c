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
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/completion.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/pm.h>

#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>
#include <linux/v4l2-controls.h>

#define TW9992_DRIVER_NAME			"tw9992"

#define POLL_TIME_MS				10
#define THINE_I2C_RETRY_CNT			3

/* #define TW9900_DEBUG */

#ifdef TW9900_DEBUG
#define tw9992_debug(x...) pr_info(x)
#else
#define tw9992_debug(x...)
#endif

enum tw9992_runmode {
	TW9992_RUNMODE_NOTREADY,
	TW9992_RUNMODE_IDLE,
	TW9992_RUNMODE_RUNNING,
	TW9992_RUNMODE_CAPTURE,
};

struct tw9992_regset {
	u32 size;
	u8 *data;
};

struct tw9992_state {
	struct media_pad		pad; /* for media device pad */
	struct v4l2_subdev		sd;

	enum tw9992_runmode		runmode;

	struct i2c_client *i2c_client;
	struct v4l2_ctrl_handler handler;
	struct v4l2_ctrl *ctrl_mux;
	struct v4l2_ctrl *ctrl_status;

	/* standard control */
	struct v4l2_ctrl *ctrl_brightness;
	char brightness;
};

struct reg_val {
	u8 reg;
	u8 val;
};

static u8 tw9992_init_data[] =  {
	0x02, 0x44,
	0x03, 0x78,
	0x08, 0x11,
	0x0A, 0x09,
	0x10, 0xF0,
	0x11, 0x60,
	0x12, 0x03,
	0x17, 0x31,
	0x1A, 0x10,
	0x1B, 0x00,
	0x1C, 0x08,
	0x20, 0x30,
	0x21, 0x22,
	0x31, 0x10,
	0x32, 0xFF,
	0x33, 0x05,
	0x34, 0x1A,
	0x36, 0x7A,
	0x37, 0x18,
	0x38, 0xDD,
	0x3A, 0x71,
	0x3B, 0x3C,
	0x3C, 0x00,
	0x3F, 0x1A,
	0x40, 0x80,
	0x48, 0x02,
	0x4A, 0x81,
	0x4B, 0x0A,
	0x4D, 0x01,
	0x4E, 0x01,
	0x54, 0x06,
	0x71, 0xA5,
	0xA2, 0x30,
	0x70, 0x01,
	0x08, 0x15,
	0x0a, 0x14,
	0x06, 0x80,

	0xff, 0xff
};

struct tw9992_state *_state;

static inline struct tw9992_state *ctrl_to_me(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct tw9992_state, handler);
}

static int tw9992_i2c_read_byte(struct i2c_client *client, u8 addr, u8 *data)
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
		/* mdelay(POLL_TIME_MS); */
	}

	if (unlikely(ret != 2)) {
		dev_err(&client->dev, "\e[31mtw9992_i2c_read_byte \e[0m");
		dev_err(&client->dev, "\e[31mfailed reg:0x%02x \e[0m\n", addr);
		return -EIO;
	}

	*data = buf;
	return 0;
}

static int tw9992_i2c_write_byte(struct i2c_client *client, u8 addr, u8 val)
{
	s8 i = 0;
	s8 ret = 0;
	u8 buf[2];
	u8 read_val = 0;
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
		/* mdelay(POLL_TIME_MS); */
	}

	if (ret != 1) {
		tw9992_i2c_read_byte(client, addr, &read_val);
		dev_err(&client->dev, "\e[31mtw9992_i2c_write_byte \e[0m");
		dev_err(&client->dev, "\e[31mfailed reg:0x%02x, \e[0m", addr);
		dev_err(&client->dev, "\e[31mwrite:0x%04x, \e[0m", val);
		dev_err(&client->dev, "\e[31mread:0x%04x, retry:%d\e[0m\n",
			read_val, i);
		return -EIO;
	}

	return 0;
}

static int tw9992_reg_set_write(struct i2c_client *client, u8 *reg_set)
{
	u8 index, val;
	int ret = 0;

	/* 0xff, 0xff is end of data */
	while ((reg_set[0] != 0xFF) || (reg_set[1] != 0xFF)) {
		index = *reg_set;
		val = *(reg_set+1);

		ret = tw9992_i2c_write_byte(client, index, val);
		mdelay(5);

		if (ret < 0)
			return ret;

		reg_set += 2;
	}

	return 0;
}

/**
 * private controls
 */
#define V4L2_CID_MUX        (V4L2_CTRL_CLASS_USER | 0x1001)
#define V4L2_CID_STATUS     (V4L2_CTRL_CLASS_USER | 0x1002)

static int tw9992_set_mux(struct v4l2_ctrl *ctrl)
{
	struct tw9992_state *me = ctrl_to_me(ctrl);

	if (ctrl->val == 0) {
		tw9992_i2c_write_byte(me->i2c_client, 0x02, 0x44);
		tw9992_i2c_write_byte(me->i2c_client, 0x3b, 0x30);
	} else {
		tw9992_i2c_write_byte(me->i2c_client, 0x02, 0x46);
		tw9992_i2c_write_byte(me->i2c_client, 0x3b, 0x0c);
	}

	return 0;
}

static int tw9992_get_status(struct v4l2_ctrl *ctrl)
{
	struct tw9992_state *me = ctrl_to_me(ctrl);
	u8 data = 0;
	u8 mux;
	u8 val = 0;

	tw9992_i2c_read_byte(me->i2c_client, 0x02, &data);
	data = data & 0x0f;
	if (data == 0x4)
		mux = 0;
	else
		mux = 1;

	if (mux == 0) {
		tw9992_i2c_read_byte(me->i2c_client, 0x03, &data);
		if (!(data & 0x80))
			val |= 1 << 0;

		tw9992_i2c_write_byte(me->i2c_client, 0x52, 0x03);
		tw9992_i2c_read_byte(me->i2c_client, 0x52, &data);
		if (data == 0x03)
			val |= 1 << 1;
	} else {
		tw9992_i2c_read_byte(me->i2c_client, 0x03, &data);
		if (!(data & 0x80))
			val |= 1 << 1;

		tw9992_i2c_write_byte(me->i2c_client, 0x52, 0x03);
		tw9992_i2c_read_byte(me->i2c_client, 0x52, &data);
		if (data == 0x03)
			val |= 1 << 0;
	}

	ctrl->val = val;
	return 0;
}

static int tw9992_set_brightness(struct v4l2_ctrl *ctrl)
{
	struct tw9992_state *me = ctrl_to_me(ctrl);

	if (me->runmode != TW9992_RUNMODE_RUNNING) {
		me->brightness = ctrl->val;
	} else {
		if (ctrl->val != me->brightness) {
			pr_info("%s: set brightness %d\n", __func__, ctrl->val);
			tw9992_i2c_write_byte(me->i2c_client, 0x10, ctrl->val);
			me->brightness = ctrl->val;
		}
	}
	return 0;
}

static int tw9992_s_ctrl(struct v4l2_ctrl *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_MUX:
		return tw9992_set_mux(ctrl);
	case V4L2_CID_BRIGHTNESS:
		pr_info("%s: brightness\n", __func__);
		return tw9992_set_brightness(ctrl);
	default:
		pr_err("%s: invalid control id 0x%x\n",
			__func__, ctrl->id);
		return -EINVAL;
	}
}

static int tw9992_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_STATUS:
		return tw9992_get_status(ctrl);
	default:
		pr_err("%s: invalid control id 0x%x\n",
			__func__, ctrl->id);
		return -EINVAL;
	}
}

static const struct v4l2_ctrl_ops tw9992_ctrl_ops = {
	.s_ctrl = tw9992_s_ctrl,
	.g_volatile_ctrl = tw9992_g_volatile_ctrl,
};

static const struct v4l2_ctrl_config tw9992_custom_ctrls[] = {
	{
		.ops  = &tw9992_ctrl_ops,
		.id   = V4L2_CID_MUX,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "MuxControl",
		.min  = 0,
		.max  = 1,
		.def  = 1,
		.step = 1,
	},
	{
		.ops  = &tw9992_ctrl_ops,
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
static int tw9992_initialize_ctrls(struct tw9992_state *me)
{
	v4l2_ctrl_handler_init(&me->handler, NUM_CTRLS);

	me->ctrl_mux = v4l2_ctrl_new_custom(&me->handler,
		&tw9992_custom_ctrls[0], NULL);
	if (!me->ctrl_mux) {
		pr_err("%s: failed to v4l2_ctrl_new_custom for mux\n",
			__func__);
		return -ENOENT;
	}

	me->ctrl_status = v4l2_ctrl_new_custom(&me->handler,
		&tw9992_custom_ctrls[1], NULL);
	if (!me->ctrl_status) {
		pr_err("%s: failed to v4l2_ctrl_new_custom for status\n",
			__func__);
		return -ENOENT;
	}

	me->ctrl_brightness = v4l2_ctrl_new_std(&me->handler, &tw9992_ctrl_ops,
			V4L2_CID_BRIGHTNESS, -128, 127, 1, -112);
	if (!me->ctrl_brightness) {
		pr_err("%s: failed to v4l2_ctrl_new_std for brightness\n",
			__func__);
		return -ENOENT;
	}

	me->sd.ctrl_handler = &me->handler;
	if (me->handler.error) {
		pr_err("%s: ctrl handler error(%d)\n",
			__func__, me->handler.error);
		v4l2_ctrl_handler_free(&me->handler);
		return -EINVAL;
	}

	return 0;
}

static void tw9992_dump_regset(struct tw9992_regset *regset)
{
	if ((regset->data[0] == 0x00) && (regset->data[1] == 0x2A)) {
		if (regset->size <= 6)
			pr_err("odd regset size %d\n", regset->size);

		pr_info("regset: addr = 0x%02X%02X, data[0,1] = 0x%02X%02X,",
		regset->data[2], regset->data[3], regset->data[6],
		regset->data[7]);
		pr_info(" total data size = %d\n", regset->size-6);
	} else {
		pr_info("regset: 0x%02X%02X%02X%02X\n",
				regset->data[0], regset->data[1],
				regset->data[2], regset->data[3]);
		if (regset->size != 4)
			pr_err("odd regset size %d\n", regset->size);
	}
}

static int tw9992_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	dev_err(&client->dev, "%s: start\n", __func__);

	ret = tw9992_reg_set_write(client, tw9992_init_data);
	if (ret < 0) {
		dev_err(&client->dev, "\e[31mtw9992_init_data0 ");
		dev_err(&client->dev, "error\e[0m, ret = %d\n", ret);

		return ret;
	}

	if (ret < 0) {
		dev_err(&client->dev, "\e[31mcolor_system() error\e[0m, ");
		dev_err(&client->dev, "ret = %d\n", ret);

		return ret;
	}
	mdelay(10);

#ifdef TW9900_DEBUG
	u8 data = 0;

	tw9992_i2c_read_byte(client, 0x02, &data);
	data = data & 0x0f;

	pr_info("%s - data : %02X\n", __func__, data);
	if (data == 0x4)
		pr_info("%s - mux == 0\n", __func__);
	else
		pr_info("%s - mux == 1\n", __func__);

	tw9992_i2c_read_byte(client, 0x07, &data);
	data = data & 0x03;
	pr_info("%s - HACTIVE_HI : %02X\n", __func__, data);

	tw9992_i2c_read_byte(client, 0x0B, &data);
	data = data & 0xff;
	pr_info("%s - HACTIVE_LO : %02X\n", __func__, data);
#endif
	return ret;
}

static int tw9992_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct tw9992_state *state = container_of(sd, struct tw9992_state, sd);
	int ret = 0;

	dev_err(&client->dev, "%s(enable : %d)\n", __func__, enable);

	if (enable) {
		tw9992_init(sd, enable);
		tw9992_i2c_write_byte(state->i2c_client, 0x10,
			state->brightness);
		state->runmode = TW9992_RUNMODE_RUNNING;
	}

	return ret;
}

static const struct v4l2_subdev_video_ops tw9992_video_ops = {
	.s_stream		= tw9992_s_stream,
};

static int tw9992_set_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *fmt)
{
	return 0;
}

static struct v4l2_subdev_pad_ops tw9992_pad_ops = {
	.set_fmt		= tw9992_set_fmt,
};

static const struct v4l2_subdev_ops tw9992_ops = {
	.video = &tw9992_video_ops,
	.pad	= &tw9992_pad_ops,
};

static int tw9992_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tw9992_state *state;
	int ret = 0;

	state = kzalloc(sizeof(struct tw9992_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	state->runmode = TW9992_RUNMODE_NOTREADY;
	sd = &state->sd;
	strcpy(sd->name, TW9992_DRIVER_NAME);

	v4l2_i2c_subdev_init(sd, client, &tw9992_ops);

	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
	sd->flags = V4L2_SUBDEV_FL_HAS_DEVNODE;

	ret = tw9992_initialize_ctrls(state);
	if (ret < 0) {
		pr_err("%s: failed to initialize controls\n", __func__);
		return ret;
	}
	state->i2c_client = client;
	_state = state;

	return 0;
}

static int tw9992_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct tw9992_state *state =
		container_of(sd, struct tw9992_state, sd);

	v4l2_device_unregister_subdev(sd);
	kfree(state);
	dev_dbg(&client->dev, "Unloaded camera sensor TW9992.\n");

	return 0;
}

static int tw9992_suspend(struct device *dev)
{
	int ret = 0;

	return ret;
}

static int tw9992_resume(struct device *dev)
{
	int ret = 0;

	return ret;
}

static const struct i2c_device_id tw9992_id[] = {
	{ TW9992_DRIVER_NAME, 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, tw9992_id);

static const struct dev_pm_ops tw9992_pm_ops = {
	.suspend	= tw9992_suspend,
	.resume		= tw9992_resume,
};

static struct i2c_driver tw9992_i2c_driver = {
	.probe = tw9992_probe,
	.remove = tw9992_remove,
	.driver = {
		.name	= TW9992_DRIVER_NAME,
		.owner	= THIS_MODULE,
		.pm	= &tw9992_pm_ops,
	},
	.id_table = tw9992_id,
};

module_i2c_driver(tw9992_i2c_driver);

MODULE_DESCRIPTION("TW9992 Video driver");
MODULE_AUTHOR("<pjsin865@nexell.co.kr>");
MODULE_LICENSE("GPL");
