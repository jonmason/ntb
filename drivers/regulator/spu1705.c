/*
 * Regulator driver for STM32 based PMIC chip
 *
 * Copyright (C) Guangzhou FriendlyElec Computer Tech. Co., Ltd.
 * (http://www.friendlyarm.com)
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
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/spu1705.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/cpufreq.h>
#include <linux/rtc.h>
#include <linux/reboot.h>

struct spu1705 {
	struct device *dev;
	struct mutex io_lock;
	struct i2c_client *i2c;
	int chip_id, chip_rev;
	int pwm_en;
	int num_regulators;
	struct regulator_dev *rdev[SPU1705_NUM_REGULATORS];
	void (*pm_power_off)(void);
#if defined(CONFIG_REGULATOR_SPU1705_REBOOT)
	struct notifier_block reboot_handler;
	int blocked_uV;
#endif
};

static int spu1705_i2c_read(struct i2c_client *client,
		unsigned char req, unsigned char *buf, int count)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			.addr   = client->addr,
			.flags  = 0,
			.len    = 1,
			.buf    = &req,
		}, {
			.addr   = client->addr,
			.flags  = I2C_M_RD,
			.len    = count,
			.buf    = buf,
		},
	};

	ret = i2c_transfer(client->adapter, &msgs[0], 2);
	if (ret < 0) {
		pr_err("spu1705: REQ 0x%02x: i2c xfer error %d\n", req, ret);
		return ret;
	}

	pr_debug("spu1705: resp %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);
	return 0;
}

static int spu1705_i2c_write(struct i2c_client *client,
		unsigned char req, unsigned char *buf, int count)
{
	int ret;
	struct i2c_msg msgs[] = {
		{
			.addr   = client->addr,
			.flags  = 0,
			.len    = count,
			.buf    = buf,
		},
	};

	ret = i2c_transfer(client->adapter, &msgs[0], 1);
	if (ret < 0) {
		pr_err("spu1705: REQ 0x%02x: i2c write error %d\n", req, ret);
		return ret;
	}

	return 0;
}

/* Supported commands */
#define SPU1705_GET_INFO	0x21
#define SPU1705_GET_TIME	0x22
#define SPU1705_GET_PWM		0x23
#define SPU1705_SET_TIME	0x11
#define SPU1705_SET_PWR		0x12
#define SPU1705_SET_PWM		0x13

/* Supported voltages */
#define SPU1705_MIN_uV		905000
#define SPU1705_MAX_uV		1265000
#define SPU1705_INIT_uV		1200000
#define SPU1705_N_VOLTAGES	96

#define VOLT_STEP	((SPU1705_MAX_uV - SPU1705_MIN_uV) / SPU1705_N_VOLTAGES)

static int spu1705_dcdc_list_voltage(struct regulator_dev *dev, unsigned index)
{
	return (SPU1705_MAX_uV - (VOLT_STEP * index));
}

/* DCDC is always enabled */
static int spu1705_dcdc_is_enabled(struct regulator_dev *dev)
{
	return 1;
}

static int spu1705_dcdc_enable(struct regulator_dev *dev)
{
	return 0;
}

static int spu1705_dcdc_disable(struct regulator_dev *dev)
{
	return 0;
}

static int spu1705_dcdc_get_voltage(struct regulator_dev *dev)
{
	struct spu1705 *priv = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - SPU1705_DCDC1;
	unsigned char pwm[4] = { 0 };

	if (!priv->pwm_en) {
		dev_dbg(priv->dev, "get_voltage: %d (default)\n", SPU1705_INIT_uV);
		return SPU1705_INIT_uV;
	}

	mutex_lock(&priv->io_lock);
	spu1705_i2c_read(priv->i2c, SPU1705_GET_PWM, pwm, 2);
	mutex_unlock(&priv->io_lock);

	dev_dbg(priv->dev, "get_voltage: buck = %d, pwm = %d, vol = %d\n",
			buck, pwm[buck], (SPU1705_MAX_uV - (VOLT_STEP * pwm[buck])));

	return (SPU1705_MAX_uV - (VOLT_STEP * pwm[buck]));
}

static int spu1705_dcdc_set_voltage(struct regulator_dev *dev,
		int min_uV, int max_uV,
		unsigned int *selector)
{
	struct spu1705 *priv = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - SPU1705_DCDC1;
	int index;
	u8 buf[4];
	int ret;

	index = (SPU1705_MAX_uV - min_uV) / VOLT_STEP;
	*selector = index;

#if defined(CONFIG_REGULATOR_SPU1705_REBOOT)
	if (unlikely(min_uV < priv->blocked_uV)) {
		dev_dbg(priv->dev, "voltage blocked to %d mV\n", priv->blocked_uV/1000);
		return 0;
	}
#endif

	mutex_lock(&priv->io_lock);

	buf[0] = SPU1705_SET_PWM;
	buf[1] = (buck + 1) & 0xff;
	buf[2] = index & 0xff;
	spu1705_i2c_write(priv->i2c, SPU1705_SET_PWM, buf, 3);
	priv->pwm_en = 1;

	/* verify write */
	buf[0] = 0;
	buf[1] = 0;
	ret = spu1705_i2c_read(priv->i2c, SPU1705_GET_PWM, buf, 2);

	mutex_unlock(&priv->io_lock);

	if (ret < 0 || buf[buck] != index)
		return -EIO;

	dev_dbg(priv->dev, "set DCDC%d (%d, %d) mV --> sel %d\n", buck,
			min_uV/1000, max_uV/1000, buf[buck]);
	return 0;
}

static struct regulator_ops spu1705_dcdc_ops = {
	.list_voltage = spu1705_dcdc_list_voltage,
	.is_enabled = spu1705_dcdc_is_enabled,
	.enable = spu1705_dcdc_enable,
	.disable = spu1705_dcdc_disable,
	.get_voltage = spu1705_dcdc_get_voltage,
	.set_voltage = spu1705_dcdc_set_voltage,
};

static struct regulator_desc regulators[] = {
	{
		.name = "DCDC1",
		.id = SPU1705_DCDC1,
		.ops = &spu1705_dcdc_ops,
		.n_voltages = SPU1705_N_VOLTAGES,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "DCDC2",
		.id = SPU1705_DCDC2,
		.ops = &spu1705_dcdc_ops,
		.n_voltages = SPU1705_N_VOLTAGES,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
};

static int spu1705_dt_parse_pdata(struct spu1705 *priv,
		struct spu1705_platform_data *pdata)
{
	struct device *dev = priv->dev;
	struct device_node *regulators_np, *reg_np;
	struct spu1705_regulator_subdev *rdata;
	int i, ret;

	regulators_np = of_get_child_by_name(dev->of_node, "regulators");
	if (!regulators_np) {
		dev_err(dev, "could not find regulators sub-node\n");
		return -EINVAL;
	}

	/* count the number of regulators to be supported in pmic */
	ret = of_get_child_count(regulators_np);
	if (ret <= 0) {
		dev_err(dev, "Error parsing regulator init data, %d\n", ret);
		return -EINVAL;
	}

	rdata = devm_kzalloc(dev, sizeof(*rdata) * ret, GFP_KERNEL);
	if (!rdata) {
		of_node_put(regulators_np);
		return -ENOMEM;
	}

	pdata->num_regulators = ret;
	pdata->regulators = rdata;

	for_each_child_of_node(regulators_np, reg_np) {
		for (i = 0; i < ARRAY_SIZE(regulators); i++)
			if (!of_node_cmp(reg_np->name, regulators[i].name))
				break;

		if (i == ARRAY_SIZE(regulators)) {
			dev_warn(dev, "don't know how to configure regulator %s\n",
					reg_np->name);
			continue;
		}

		rdata->id = i;
		rdata->initdata = of_get_regulator_init_data(dev, reg_np, &regulators[i]);
		rdata->reg_node = reg_np;
		rdata++;
	}

	of_node_put(regulators_np);

	return 0;
}

static int setup_regulators(struct spu1705 *priv,
		struct spu1705_platform_data *pdata)
{
	struct regulator_config config = { };
	int i, err;

	priv->num_regulators = pdata->num_regulators;

	for (i = 0; i < pdata->num_regulators; i++) {
		int id = pdata->regulators[i].id;

		config.dev = priv->dev;
		config.driver_data = priv;
		config.init_data = pdata->regulators[i].initdata;
		config.of_node = pdata->regulators[i].reg_node;

		priv->rdev[i] = devm_regulator_register(priv->dev, &regulators[id],
				&config);
		if (IS_ERR(priv->rdev[i])) {
			err = PTR_ERR(priv->rdev[i]);
			dev_err(priv->dev, "failed to register regulator %d, err = %d\n",
					i, err);
			return err;

		}
	}

	return 0;
}

/* Power On/Off support */
static struct i2c_client *pm_i2c;

static void spu1705_power_off(void)
{
	struct spu1705 *priv = i2c_get_clientdata(pm_i2c);
	u8 buf[4];

	buf[0] = SPU1705_SET_PWR;
	buf[1] = 0x01;
	spu1705_i2c_write(pm_i2c, SPU1705_SET_PWR, buf, 2);

	pr_info("spu1705: power off\n");

	if (priv->pm_power_off)
		priv->pm_power_off();
}

#if defined(CONFIG_REGULATOR_SPU1705_REBOOT)
static int spu1705_restart_handle(struct notifier_block *this,
		unsigned long mode, void *cmd)
{
	struct spu1705 *priv =
		container_of(this, struct spu1705, reboot_handler);
	unsigned int sel;
	int i;

	priv->blocked_uV = SPU1705_INIT_uV;

	for (i = 0; i < priv->num_regulators; i++)
		spu1705_dcdc_set_voltage(priv->rdev[i],
				SPU1705_INIT_uV, SPU1705_INIT_uV, &sel);

	return NOTIFY_DONE;
}
#endif

#define to_i2c_client(d) container_of(d, struct i2c_client, dev)

static inline void spu1705_tm_to_data(struct rtc_time *tm, u8 *data)
{
	data[0] = tm->tm_hour;
	data[1] = tm->tm_min;
	data[2] = tm->tm_sec;
}

static ssize_t spu1705_sysfs_show_wakealarm(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct spu1705 *priv = dev_get_drvdata(dev);
	u8 tm[8];
	ssize_t n;
	int ret;

	mutex_lock(&priv->io_lock);
	ret = spu1705_i2c_read(priv->i2c, SPU1705_GET_TIME, tm, 7);
	mutex_unlock(&priv->io_lock);

	if (ret < 0)
		return -EIO;

	n = sprintf(buf, "%02d:%02d:%02d", tm[1], tm[2], tm[3]);
	if (tm[0])
		n += sprintf(buf + n, " %02d:%02d:%02d\n", tm[4], tm[5], tm[6]);
	else
		n += sprintf(buf + n, " disabled\n");

	return n;
}

#define SPU1705_ALARM_MIN	60

static ssize_t spu1705_sysfs_set_wakealarm(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t n)
{
	struct spu1705 *priv = dev_get_drvdata(dev);
	struct rtc_time tm;
	struct timeval tv;
	unsigned long alarm;
	u8 data[8];
	int count = 8;

	do_gettimeofday(&tv);
	rtc_time_to_tm(tv.tv_sec, &tm);
	spu1705_tm_to_data(&tm, &data[2]);

	alarm = simple_strtoul(buf, NULL, 0);
	if (alarm > SPU1705_ALARM_MIN) {
		data[1] = 1;
		tv.tv_sec += alarm;
		rtc_time_to_tm(tv.tv_sec, &tm);
		spu1705_tm_to_data(&tm, &data[5]);
		dev_info(dev, "wake alarm: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);

	} else if (alarm == 0) {
		data[1] = 0;
		count = 2;
		dev_info(dev, "wake alarm: disabled\n");

	} else {
		dev_err(dev, "invalid alarm %lu (0: disable, >%d: enable)\n",
				alarm, SPU1705_ALARM_MIN);
		return -EINVAL;
	}

	mutex_lock(&priv->io_lock);

	data[0] = SPU1705_SET_TIME;
	spu1705_i2c_write(priv->i2c, SPU1705_SET_TIME, data, count);

	mutex_unlock(&priv->io_lock);

	return n;
}

static DEVICE_ATTR(wakealarm, S_IRUGO | S_IWUSR,
		spu1705_sysfs_show_wakealarm, spu1705_sysfs_set_wakealarm);

static void spu1705_sysfs_add_device(struct spu1705 *priv)
{
	int err;

	err = device_create_file(priv->dev, &dev_attr_wakealarm);
	if (err)
		dev_err(priv->dev, "failed to create alarm attribute, %d\n", err);
}

static int sp1705_identify_chip(struct spu1705 *priv)
{
	unsigned char id[4] = { 0 };

	if (spu1705_i2c_read(priv->i2c, SPU1705_GET_INFO, id, 4) < 0)
		return -1;

	if (!id[0] || !id[1])
		return -1;

	priv->chip_id = id[0];
	priv->chip_rev = id[1] * 100 + id[2];
	priv->pwm_en = id[3];

	return 0;
}

static int spu1705_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct spu1705_platform_data *pdata = client->dev.platform_data;
	struct spu1705 *priv;
	int ret;

	priv = devm_kzalloc(&client->dev, sizeof(struct spu1705),
			GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;

	mutex_init(&priv->io_lock);
	priv->i2c = client;
	priv->dev = &client->dev;

	if (IS_ENABLED(CONFIG_OF) && priv->dev->of_node) {
		pdata = devm_kzalloc(&client->dev,
				sizeof(struct spu1705_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "could not allocate memory for pdata\n");
			return -ENOMEM;
		}

		ret = spu1705_dt_parse_pdata(priv, pdata);
		if (ret < 0)
			return ret;
	}

	if (!pdata) {
		dev_err(&client->dev, "No platform init data supplied\n");
		return -ENODEV;
	}

	if (sp1705_identify_chip(priv) < 0) {
		dev_err(&client->dev, "failed to detect chip\n");
		ret = -ENODEV;
		goto err_detect;
	}

	ret = setup_regulators(priv, pdata);
	if (ret < 0)
		goto err_detect;

	i2c_set_clientdata(client, priv);

	/* PM hookup */
	if (pm_power_off)
		priv->pm_power_off = pm_power_off;
	pm_i2c = client;
	pm_power_off = spu1705_power_off;

#if defined(CONFIG_REGULATOR_SPU1705_REBOOT)
	priv->reboot_handler.notifier_call = spu1705_restart_handle;
	priv->reboot_handler.priority = 192;
	ret = register_reboot_notifier(&priv->reboot_handler);
	if (ret) {
		dev_err(&client->dev, "can't register restart notifier, %d\n", ret);
		return ret;
	}
#endif

	spu1705_sysfs_add_device(priv);

	dev_info(&client->dev, "found chip 0x%02x, rev %04d\n",
			priv->chip_id, priv->chip_rev);
	return 0;

err_detect:
	return ret;
}

static int spu1705_i2c_remove(struct i2c_client *i2c)
{
	struct spu1705 *priv = i2c_get_clientdata(i2c);
	unsigned int sel;
	int i;

#if defined(CONFIG_REGULATOR_SPU1705_REBOOT)
	if (unregister_reboot_notifier(&priv->reboot_handler))
		dev_err(priv->dev, "can't unregister restart handler\n");
		return -ENODEV;
	}
#endif

	pm_power_off = priv->pm_power_off;

	for (i = 0; i < priv->num_regulators; i++)
		spu1705_dcdc_set_voltage(priv->rdev[i],
				SPU1705_INIT_uV, SPU1705_INIT_uV, &sel);

	return 0;
}

static const struct i2c_device_id spu1705_i2c_id[] = {
	{ "fe-pmu", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, spu1705_i2c_id);

#if defined(CONFIG_OF)
static const struct of_device_id spu1705_of_match[] = {
	{ .compatible = "spu1705", },
	{},
};
MODULE_DEVICE_TABLE(of, spu1705_of_match);
#endif

static struct i2c_driver spu1705_i2c_driver = {
	.driver = {
		.name = "spu1705",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(spu1705_of_match),
	},
	.probe    = spu1705_i2c_probe,
	.remove   = spu1705_i2c_remove,
	.id_table = spu1705_i2c_id,
};

static int __init spu1705_module_init(void)
{
	int ret;

	ret = i2c_add_driver(&spu1705_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register I2C driver: %d\n", ret);

	return ret;
}
module_init(spu1705_module_init);

static void __exit spu1705_module_exit(void)
{
	i2c_del_driver(&spu1705_i2c_driver);
}
module_exit(spu1705_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Guangzhou FriendlyElec Computer Tech. Co., Ltd.");
MODULE_DESCRIPTION("SPU1705 PMIC driver");
