/*
 * multi touch screen driver for it7260
 * base on multi-touch protocol A
 *
 * Copyright (C) Guangzhou FriendlyARM Computer Tech. Co., Ltd.
 * (http://www.friendlyarm.com)
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/i2c.h>
#include <linux/timer.h>
#include <linux/platform_data/ctouch.h>

/* buffer address */
#define CMD_BUF		0x20	/* command buffer (write only) */
#define SYS_CMD_BUF	0x40	/* systerm command buffer (write only) */
#define QUERY_BUF	0x80	/* query buffer (read only) */
#define CMD_RSP_BUF	0xA0	/* command response buffer (read only) */
#define SYS_CMD_RSP_BUF	0xC0	/* systerm command response buffer (read only) */
#define POINT_INFO_BUF	0xE0	/* point information buffer (read only) */

struct it7260_ts_priv {
	struct i2c_client *client;
	struct input_dev *input;
	struct delayed_work work;
	struct mutex mutex;
	int irq;
};

/**
* i2c_master_read_it7260 - issue two I2C message in master receive mode
* @client: handler to slave device
* @buf_index: buffer address
* @buf_data: where to store data read from slave
* @len_data: the bytes of buf_data to read
*
* returns negative errno, or else the number of bytes read
*/
static int i2c_master_read_it7260(struct i2c_client *client,
		unsigned char buf_index, unsigned char *buf_data,
		unsigned short len_data)
{
	int ret;
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,
			.flags = I2C_M_NOSTART,
			.len = 1,
			.buf = &buf_index,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = len_data,
			.buf = buf_data,
		}
	};

	ret = i2c_transfer(client->adapter, msgs, 2);

	return (ret == 2) ? len_data : ret;
}

/**
* i2c_master_write_it7260 - issue a single I2C message in master transmit mode
* @client: handler to slave device
* @buf_index: buffer address
* @buf_data: data that wile be write to the slave
* @len_data: the bytes of buf_data to write
*
* returns negative errno, or else the number of bytes written
*/
static int i2c_master_write_it7260(struct i2c_client *client,
		unsigned char buf_index, unsigned char const *buf_data,
		unsigned short len_data)
{
	unsigned char buf[2];
	int ret;

	struct i2c_msg msgs[1] = {
		{
			.addr = client->addr,
			.flags = 0, /* default write flag */
			.len = len_data + 1,
			.buf = buf,
		}
	};

	buf[0] = buf_index;
	memcpy(&buf[1], buf_data, len_data);

	ret = i2c_transfer(client->adapter, msgs, 1);

	return (ret == 1) ? sizeof(buf) : ret;
}

/**
* it7260_ts_poscheck - delayed work
*
* get the informations of contacts from slave and report it
*/
#define PT_MAX		3
static void it7260_ts_poscheck(struct work_struct *work)
{
	struct it7260_ts_priv *priv = container_of(work,
							struct it7260_ts_priv, work.work);
	unsigned char buf[14];
	unsigned short xpos[PT_MAX] = {0}, ypos[PT_MAX] = {0};
	unsigned char event[PT_MAX] = {0};
	unsigned char query = 0;
	int touch_point = 0;
	int ret, i;

	mutex_lock(&priv->mutex);

	i2c_master_read_it7260(priv->client, QUERY_BUF, &query, 1);
	if (!(query & 0x80)) {
		goto up;
	}

	memset(buf, 0, sizeof(buf));

	ret = i2c_master_read_it7260(priv->client, POINT_INFO_BUF, buf, 14);
	if (ret != 14) {
		printk("failed to read point info buffer\n");
		goto out;
	}

	/* touch key */
	if (buf[0] == 0x41) {
		printk(KERN_DEBUG "it7260: key number %d\n", buf[1]);
		if (buf[1] == 0x04)
			input_report_key(priv->input, KEY_MENU, !!buf[2]);
		else if (buf[1] == 0x03)
			input_report_key(priv->input, KEY_HOMEPAGE, !!buf[2]);
		else if (buf[1] == 0x02)
			input_report_key(priv->input, KEY_BACK, !!buf[2]);
		else if (buf[1] == 0x01)
			input_report_key(priv->input, KEY_SEARCH, !!buf[2]);
		else
			goto out;

		goto sync;
	}

	/* finger 0 */
	if (buf[0] & 0x01) {
		xpos[0] = ((buf[3] & 0x0F) << 8) | buf[2];
		ypos[0] = ((buf[3] & 0xF0) << 4) | buf[4];
		event[0] = buf[5] & 0x0F;
	}

	/* finger 1 */
	if (buf[0] & 0x02) {
		xpos[1] = ((buf[7] & 0x0F) << 8) | buf[6];
		ypos[1] = ((buf[7] & 0xF0) << 4) | buf[8];
		event[1] = buf[9] & 0x0F;
	}

	/* finger 2 */
	if (buf[0] & 0x04) {
		xpos[2] = ((buf[11] & 0x0F) << 8) | buf[10];
		ypos[2] = ((buf[11] & 0xF0) << 4) | buf[12];
		event[2] = buf[13] & 0x0F;
	}

	for (i = 0; i < PT_MAX; i++) {
		if (xpos[i] || ypos[i] || event[i]) {
			touch_point++;
#ifdef CONFIG_TOUCHSCREEN_PROT_SINGLE
			input_report_abs(priv->input, ABS_X, xpos[i]);
			input_report_abs(priv->input, ABS_Y, ypos[i]);
			input_report_abs(priv->input, ABS_PRESSURE, (event[i] << 4));
			input_report_key(priv->input, BTN_TOUCH, 1);
			break;
#else
			input_report_abs(priv->input, ABS_MT_POSITION_X, xpos[i]);
			input_report_abs(priv->input, ABS_MT_POSITION_Y, ypos[i]);
			input_report_abs(priv->input, ABS_MT_PRESSURE,   (event[i] << 4));
			input_report_abs(priv->input, ABS_MT_TOUCH_MAJOR, event[i]);
			input_report_abs(priv->input, ABS_MT_TRACKING_ID, i);
			input_mt_sync(priv->input);
#endif
#if 0
			printk("finger %d >  (%4d, %4d),  event = %d\n",
					i, ypos[i], xpos[i], event[i]);
#endif
		}
	}

up:
	if (!touch_point) {
		/* All fingers are removed */
#ifdef CONFIG_TOUCHSCREEN_PROT_SINGLE
		input_report_abs(priv->input, ABS_PRESSURE, 0);
		input_report_key(priv->input, BTN_TOUCH, 0);
#else
		input_mt_sync(priv->input);
#endif
	}

sync:
	input_sync(priv->input);

out:
	mutex_unlock(&priv->mutex);
	enable_irq(priv->irq);
}

static irqreturn_t it7260_ts_isr(int irq, void *dev_id)
{
	struct it7260_ts_priv *priv = dev_id;

	disable_irq_nosync(irq);
	schedule_delayed_work(&priv->work, HZ / 50);

	return IRQ_HANDLED;
}

/**
* it7260_identify_capsensor - identify capacitance sensor model
*
* returns error -1, or else suc 0
*/
static int it7260_identify_capsensor(struct i2c_client *client)
{
	unsigned char buf[16] = {0};
	unsigned char query = 0;

	do {
		i2c_master_read_it7260(client, QUERY_BUF, &query, 1);
	} while (query & 0x01);

	/* 0x00: the command of identify cap sensor */
	buf[0] = 0x00;
	i2c_master_write_it7260(client, CMD_BUF, buf, 1);

	do {
		i2c_master_read_it7260(client, QUERY_BUF, &query, 1);
	} while (query & 0x01);

	memset(&buf, 0, sizeof(buf));

	i2c_master_read_it7260(client, CMD_RSP_BUF, buf, 10);
	dev_info(&client->dev, "Found chip %s\n", &buf[1]);

	if (buf[1] != 'I' || buf[2] != 'T' || buf[3] != 'E')
		return -1;

	return 0;
}

static int it7260_ts_probe(struct i2c_client *client,
			const struct i2c_device_id *idp)
{
	struct it7260_ts_priv *priv;
	struct input_dev *input;
	unsigned int ctp_id;
	int error;

	ctp_id = panel_get_touch_id();
	if (ctp_id != CTP_ITE7260 && ctp_id != CTP_AUTO)
		return -ENODEV;

	if (it7260_identify_capsensor(client)) {
		dev_err(&client->dev, "cannot identify the touch screen\n");
		return -ENODEV;
	}

	priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&client->dev, "failed to allocate driver data\n");
		return -ENOMEM;
	}

	input = devm_input_allocate_device(&client->dev);
	if (!input) {
		dev_err(&client->dev, "failed to allocate input device\n");
		return -ENOMEM;
	}

	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

	input_set_capability(input, EV_KEY, KEY_MENU);
	input_set_capability(input, EV_KEY, KEY_BACK);
	input_set_capability(input, EV_KEY, KEY_HOMEPAGE);
	input_set_capability(input, EV_KEY, KEY_SEARCH);

#ifdef CONFIG_TOUCHSCREEN_PROT_SINGLE
	set_bit(ABS_X, input->absbit);
	set_bit(ABS_Y, input->absbit);
	set_bit(ABS_PRESSURE, input->absbit);
	set_bit(BTN_TOUCH, input->keybit);

	input_set_abs_params(input, ABS_X, 0, 1024, 0, 0);
	input_set_abs_params(input, ABS_Y, 0,  600, 0, 0);
	input_set_abs_params(input, ABS_PRESSURE, 0, 255, 0, 0);
#else
	set_bit(INPUT_PROP_DIRECT, input->propbit);

	input_set_abs_params(input, ABS_MT_POSITION_X, 0, 1024, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0,  600, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0,  16, 0, 0);
	input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 2, 0, 0);
	input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, 5, 0, 0);
#endif

	input->name = "it7260_ts";
	input->phys = "I2C";
	input->id.bustype = BUS_I2C;

	input_set_drvdata(input, priv);

	mutex_init(&priv->mutex);
	priv->client = client;
	priv->input = input;
	priv->irq = client->irq;
	INIT_DELAYED_WORK(&priv->work, it7260_ts_poscheck);

	error = devm_request_irq(&client->dev, priv->irq, it7260_ts_isr,
				 IRQF_TRIGGER_LOW, client->name, priv);
	if (error) {
		dev_err(&client->dev, "unable to request touchscreen IRQ\n");
		return error;
	}

	error = input_register_device(input);
	if (error) {
		dev_err(&client->dev, "failed to register input device\n");
		return error;
	}

	dev_set_drvdata(&client->dev, priv);
	panel_set_touch_id(CTP_ITE7260);

	device_init_wakeup(&client->dev, 1);
	return 0;
}

static int it7260_ts_remove(struct i2c_client *client)
{
	struct it7260_ts_priv *priv = dev_get_drvdata(&client->dev);

	input_unregister_device(priv->input);
	dev_set_drvdata(&client->dev, NULL);

	return 0;
}

static int __maybe_unused it7260_ts_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct it7260_ts_priv *priv = i2c_get_clientdata(client);
	u8 suspend_cmd[] = {0x04, 0x00, 0x02};
	int ret = -1;

	if (device_may_wakeup(&client->dev)) {
		enable_irq_wake(priv->irq);
		if (sizeof(suspend_cmd) == i2c_master_write_it7260(client,
					CMD_BUF, suspend_cmd, 3))
			ret = 0;
	}

	return ret;
}

static int __maybe_unused it7260_ts_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct it7260_ts_priv *priv = i2c_get_clientdata(client);
	unsigned char query;
	int ret = -1;

	if (device_may_wakeup(&client->dev)) {
		i2c_master_read_it7260(client, QUERY_BUF, &query, 1);
		disable_irq_wake(priv->irq);
		ret = 0;
	}

	return ret;
}

static SIMPLE_DEV_PM_OPS(it7260_dev_pm_ops,
			 it7260_ts_suspend, it7260_ts_resume);

static const struct i2c_device_id it7260_ts_id[] = {
	{"IT7260", 0},
	{}			/* should not omitted */
};
MODULE_DEVICE_TABLE(i2c, it7260_ts_id);

static const struct of_device_id it7260_of_match[] = {
	{ .compatible = "ite,it7260", },
	{ }
};
MODULE_DEVICE_TABLE(of, it7260_of_match);

static struct i2c_driver it7260_ts_driver = {
	.driver = {
		.name = "IT7260-ts",
		.of_match_table = of_match_ptr(it7260_of_match),
		.pm = &it7260_dev_pm_ops,
	},
	.probe = it7260_ts_probe,
	.remove = it7260_ts_remove,
	.id_table = it7260_ts_id,
};

static int __init it7260_ts_init(void)
{
	return i2c_add_driver(&it7260_ts_driver);
}

static void __exit it7260_ts_exit(void)
{
	i2c_del_driver(&it7260_ts_driver);
}

module_init(it7260_ts_init);
module_exit(it7260_ts_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CJOK <cjok.liao@gmail.com>");
MODULE_DESCRIPTION("it7260 touchscreen driver");

