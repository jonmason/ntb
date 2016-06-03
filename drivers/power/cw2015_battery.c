/*
 * RockChip ADC Battery Driver
 * Copyright (C) 2012, RockChip
 *
 * Authors: xuhuicong <xhc@rock-chips.com>
 * Authors: Dongil Park <dongil01.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/power/cw2015_battery.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/regmap.h>

#include <linux/io.h>

#define CW2015_GPIO_HIGH  1
#define CW2015_GPIO_LOW   0

#define REG_VERSION             0x0
#define REG_VCELL               0x2
#define REG_SOC                 0x4
#define REG_RRT_ALERT           0x6
#define REG_CONFIG              0x8
#define REG_MODE                0xA
#define REG_BATINFO             0x10

#define MODE_SLEEP_MASK         (0x3 << 6)
#define MODE_SLEEP              (0x3 << 6)
#define MODE_NORMAL             (0x0 << 6)
#define MODE_QUICK_START        (0x3 << 4)
#define MODE_RESTART            (0xf << 0)

#define CONFIG_UPDATE_FLG       (0x1 << 1)
#define ATHD                    (0x0 << 3)	/* ATHD = 0% */

/* default i2c speed set 100khz */
#define CW_I2C_SPEED            100000

/* the max time allow battery change quantity */
#define BATTERY_UP_MAX_CHANGE   420

/* the max time allow battery change quantity */
#define BATTERY_DOWN_CHANGE	60

/* the min time allow battery change quantity when run */
#define BATTERY_DOWN_MIN_CHANGE_RUN 30

/* the min time allow battery change quantity when run 30min */
#define BATTERY_DOWN_MIN_CHANGE_SLEEP 1800

#define BATTERY_DOWN_MAX_CHANGE_RUN_AC_ONLINE 3600

#define NO_STANDARD_AC_BIG_CHARGE_MODE 1
#define BAT_LOW_INTERRUPT	1

#define USB_CHARGER_MODE	1
#define AC_CHARGER_MODE		2
#define CW_QUICKSTART		0

struct cw_battery {
	struct device *dev;
	struct i2c_client *client;
	struct workqueue_struct *battery_workqueue;
	struct delayed_work battery_delay_work;
	struct delayed_work dc_wakeup_work;
	struct delayed_work bat_low_wakeup_work;
	struct cw_bat_platform_data plat_data;

	struct power_supply *rk_bat;
	struct power_supply_desc rk_bat_desc;
	struct power_supply *rk_ac;
	struct power_supply_desc rk_ac_desc;
	struct power_supply *rk_usb;
	struct power_supply_desc rk_usb_desc;

	/* the sleep time from capacity change to present,
	 * it will set 0 when capacity change */
	long sleep_time_capacity_change;
	long run_time_capacity_change;

	/* the sleep time from insert ac to present,
	 * it will set 0 when insert ac */
	long sleep_time_charge_start;
	long run_time_charge_start;

	int dc_online;
	int usb_online;
	int charger_mode;
	int charger_init_mode;
	int capacity;
	int voltage;
	int status;
	int time_to_empty;
	int alt;

	int bat_change;
};

static int cw_read(struct i2c_client *client, u8 reg, u8 buf[])
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		return ret;

	buf[0] = ret;

	return 0;
}

static int cw_write(struct i2c_client *client, u8 reg, u8 const buf[])
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, buf[0]);

	return ret;
}

static int cw_read_word(struct i2c_client *client, u8 reg, u8 buf[])
{
	int ret;

	ret = i2c_smbus_read_word_data(client, reg);

	if (ret < 0)
		return ret;

	buf[0] = ret & 0xff;
	buf[1] = (ret >> 8) & 0xff;

	return 0;
}

static int cw_update_config_info(struct cw_battery *cw_bat)
{
	int ret;
	u8 reg_val = 0, i, reset_val = 0;
	struct cw_bat_platform_data *plat_data = &(cw_bat->plat_data);

	/* make sure not in sleep mode */
	ret = cw_read(cw_bat->client, REG_MODE, &reg_val);
	if (ret < 0)
		return ret;

	reset_val = reg_val;
	if ((reg_val & MODE_SLEEP_MASK) == MODE_SLEEP) {
		dev_err(&cw_bat->client->dev,
				"Device in sleep mode, cannot update battery info\n");
		return -1;
	}

	/* update new battery info */
	for (i = 0; i < SIZE_BATINFO; i++) {
		dev_dbg(&cw_bat->client->dev,
			"cw_bat->plat_data.cw_bat_config_info[%d] = 0x%x\n",
			i, plat_data->cw_bat_config_info[i]);
		ret = cw_write(cw_bat->client, REG_BATINFO + i,
				(u8 *)&plat_data->cw_bat_config_info[i]);

		if (ret < 0)
			return ret;
	}

	/* readback & check */
	for (i = 0; i < SIZE_BATINFO; i++) {
		ret = cw_read(cw_bat->client, REG_BATINFO + i, &reg_val);
		if (reg_val != plat_data->cw_bat_config_info[i]) {
			dev_dbg(cw_bat->dev, "----> i(%d) [0x%x-0x%x]\n",
				i, reg_val, plat_data->cw_bat_config_info[i]);
			return -1;
		}
	}

	/* set cw2015/cw2013 to use new battery info */
	ret = cw_read(cw_bat->client, REG_CONFIG, &reg_val);
	if (ret < 0)
		return ret;

	reg_val |= CONFIG_UPDATE_FLG;   /* set UPDATE_FLAG */
	reg_val &= 0x07;                /* clear ATHD */
	reg_val |= ATHD;                /* set ATHD */
	ret = cw_write(cw_bat->client, REG_CONFIG, &reg_val);
	if (ret < 0)
		return ret;

	/* check 2015/cw2013 for ATHD & update_flag */
	ret = cw_read(cw_bat->client, REG_CONFIG, &reg_val);
	if (ret < 0)
		return ret;

	if (!(reg_val & CONFIG_UPDATE_FLG)) {
		dev_info(cw_bat->dev,
			"update flag for new battery info have not set..\n");
	}

	if ((reg_val & 0xf8) != ATHD) {
		dev_info(cw_bat->dev,
				"the new ATHD have not set..\n");
	}

	/* reset */
	reset_val &= ~(MODE_RESTART);
	reg_val = reset_val | MODE_RESTART;
	ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
	if (ret < 0)
		return ret;

	mdelay(10);
	ret = cw_write(cw_bat->client, REG_MODE, &reset_val);
	if (ret < 0)
		return ret;

	return 0;
}

static int cw_init(struct cw_battery *cw_bat)
{
	int ret;
	int i;
	u8 reg_val;

	reg_val = MODE_NORMAL;
	ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
	if (ret < 0)
		return ret;

	ret = cw_read(cw_bat->client, REG_VERSION, &reg_val);
	if (ret < 0)
		return ret;

	dev_err(cw_bat->dev, "ret=%d version(%x)\n", ret, reg_val);

	ret = cw_read(cw_bat->client, REG_CONFIG, &reg_val);
	if (ret < 0)
		return ret;

	if ((reg_val & 0xf8) != ATHD) {
		dev_info(cw_bat->dev, "the new ATHD have not set\n");
		reg_val &= 0x07;    /* clear ATHD */
		reg_val |= ATHD;    /* set ATHD */
		ret = cw_write(cw_bat->client, REG_CONFIG, &reg_val);
		if (ret < 0)
			return ret;
	}

	ret = cw_read(cw_bat->client, REG_CONFIG, &reg_val);
	if (ret < 0)
		return ret;

	dev_err(cw_bat->dev, "rkbat config %x\n", reg_val);
	if (!(reg_val & CONFIG_UPDATE_FLG)) {
		dev_err(cw_bat->dev,
			"update flag for new battery info have not set\n");
		ret = cw_update_config_info(cw_bat);
		if (ret < 0)
			return ret;
	} else {
		for (i = 0; i < SIZE_BATINFO; i++) {
			ret = cw_read(cw_bat->client, (REG_BATINFO + i),
					&reg_val);
			if (ret < 0)
				return ret;

			if (cw_bat->plat_data.cw_bat_config_info[i] != reg_val)
				break;
		}

		if (i != SIZE_BATINFO) {
			dev_info(cw_bat->dev,
				"update flag for new battery info not set\n");
			ret = cw_update_config_info(cw_bat);
			if (ret < 0)
				return ret;
		}
	}

	for (i = 0; i < 30; i++) {
		ret = cw_read(cw_bat->client, REG_SOC, &reg_val);
		dev_dbg(cw_bat->dev, "rkbat soc %d[%d]\n", reg_val, i);
		if (ret < 0)
			return ret;
		else if (reg_val <= 0x64)
			break;
		mdelay(50);
		if (i > 25)
			dev_err(cw_bat->dev,
				"cw2015/cw2013 input unvalid power error\n");
	}

	if (i >= 30) {
		reg_val = MODE_SLEEP;
		ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
		dev_err(cw_bat->dev, "report battery capacity error");
		return -1;
	}

	return 0;
}

static void cw_update_time_member_charge_start(struct cw_battery *cw_bat)
{
	struct timespec ts;
	int new_run_time;
	int new_sleep_time;

	ktime_get_ts(&ts);
	new_run_time = ts.tv_sec;

	get_monotonic_boottime(&ts);
	new_sleep_time = ts.tv_sec - new_run_time;

	cw_bat->run_time_charge_start = new_run_time;
	cw_bat->sleep_time_charge_start = new_sleep_time;
}

static void cw_update_time_member_capacity_change(struct cw_battery *cw_bat)
{
	struct timespec ts;
	int new_run_time;
	int new_sleep_time;

	ktime_get_ts(&ts);
	new_run_time = ts.tv_sec;

	get_monotonic_boottime(&ts);
	new_sleep_time = ts.tv_sec - new_run_time;

	cw_bat->run_time_capacity_change = new_run_time;
	cw_bat->sleep_time_capacity_change = new_sleep_time;
}

#ifdef SYSTEM_SHUTDOWN_VOLTAGE
static int cw_quickstart(struct cw_battery *cw_bat)
{
	int ret = 0;
	u8 reg_val = MODE_QUICK_START;

	ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
	if (ret < 0) {
		dev_err(&cw_bat->client->dev, "Error quick start1\n");
		return ret;
	}

	reg_val = MODE_NORMAL;
	ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
	if (ret < 0) {
		dev_err(&cw_bat->client->dev, "Error quick start2\n");
		return ret;
	}
	return 1;
}
#endif

static inline int cw_reset_reg_mode(struct cw_battery *cw_bat)
{
	u8 reset_val = MODE_SLEEP;
	int ret;

	ret = cw_write(cw_bat->client, REG_MODE, &reset_val);
	if (ret < 0)
		return ret;

	reset_val = MODE_NORMAL;
	mdelay(10);
	ret = cw_write(cw_bat->client, REG_MODE, &reset_val);
	if (ret < 0)
		return ret;

	return 0;
}

static int cw_get_capacity(struct cw_battery *cw_bat)
{
	int cw_capacity;
	int ret;
	u8 reg_val[2];
	struct timespec ts;
	long new_run_time;
	long new_sleep_time;
	long capacity_or_aconline_time;
	int allow_change;
	int allow_capacity;
	static int if_quickstart;
	static int jump_flag;
	static int reset_loop;
	int charge_time;

	ret = cw_read_word(cw_bat->client, REG_SOC, reg_val);
	if (ret < 0)
		return ret;

	cw_capacity = reg_val[0];
	if (cw_capacity == 0 && if_quickstart == 0) {
		dev_dbg(cw_bat->dev, "the capacity is 0");

		ret = cw_reset_reg_mode(cw_bat);
		if (ret < 0)
			return ret;

		if_quickstart = 1;
	} else
		dev_dbg(cw_bat->dev, "the cw201x capacity is %d\n",
				cw_capacity);

	if ((cw_capacity < 0) || (cw_capacity > 100)) {
		dev_err(cw_bat->dev,
				"get cw_capacity error; cw_capacity = %d\n",
				cw_capacity);
		reset_loop++;

		if (reset_loop > 5) {
			ret = cw_reset_reg_mode(cw_bat);
			if (ret < 0)
				return ret;

			dev_info(cw_bat->dev,
					"report battery capacity error");
			ret = cw_update_config_info(cw_bat);
			if (ret)
				return ret;
			reset_loop = 0;
		}
		return cw_capacity;
	}

	reset_loop = 0;

	ktime_get_ts(&ts);
	new_run_time = ts.tv_sec;

	get_monotonic_boottime(&ts);
	new_sleep_time = ts.tv_sec - new_run_time;

	if (((cw_bat->charger_mode > 0) &&
		(cw_capacity <= (cw_bat->capacity - 1)) &&
		(cw_capacity > (cw_bat->capacity - 9))) ||
			((cw_bat->charger_mode == 0) &&
			 (cw_capacity == (cw_bat->capacity + 1)))) {
		/* modify battery level swing */
		if (!(cw_capacity == 0 && cw_bat->capacity <= 2))
			cw_capacity = cw_bat->capacity;
	}

	if ((cw_bat->charger_mode > 0) && (cw_capacity >= 95) &&
			(cw_capacity <= cw_bat->capacity)) {
		/* avoid no charge full */
		capacity_or_aconline_time =
			(cw_bat->sleep_time_capacity_change >
			 cw_bat->sleep_time_charge_start) ?
			cw_bat->sleep_time_capacity_change :
			cw_bat->sleep_time_charge_start;

		capacity_or_aconline_time +=
			(cw_bat->run_time_capacity_change >
			 cw_bat->run_time_charge_start) ?
			cw_bat->run_time_capacity_change :
			cw_bat->run_time_charge_start;

		allow_change = (new_sleep_time + new_run_time -
				capacity_or_aconline_time) /
				BATTERY_UP_MAX_CHANGE;

		if (allow_change > 0) {
			allow_capacity = cw_bat->capacity + allow_change;
			cw_capacity = (allow_capacity <= 100) ?
					allow_capacity : 100;
			jump_flag = 1;
		} else if (cw_capacity <= cw_bat->capacity)
			cw_capacity = cw_bat->capacity;
	} else if (cw_bat->charger_mode == 0 &&
			cw_capacity <= cw_bat->capacity &&
			cw_capacity >= 90 && jump_flag == 1) {
		/* avoid battery level jump to CW_BAT */
		capacity_or_aconline_time =
			(cw_bat->sleep_time_capacity_change >
			 cw_bat->sleep_time_charge_start) ?
			 cw_bat->sleep_time_capacity_change :
			 cw_bat->sleep_time_charge_start;

		capacity_or_aconline_time +=
			(cw_bat->run_time_capacity_change >
			 cw_bat->run_time_charge_start) ?
			 cw_bat->run_time_capacity_change :
			 cw_bat->run_time_charge_start;

		allow_change = (new_sleep_time + new_run_time -
				capacity_or_aconline_time) /
				BATTERY_DOWN_CHANGE;

		if (allow_change > 0) {
			allow_capacity = cw_bat->capacity - allow_change;
			if (cw_capacity >= allow_capacity)
				jump_flag = 0;
			else {
				cw_capacity = (allow_capacity <= 100) ?
						allow_capacity : 100;
			}
		} else if (cw_capacity <= cw_bat->capacity)
			cw_capacity = cw_bat->capacity;
	}

	if ((cw_capacity == 0) && (cw_bat->capacity > 1)) {
		/* avoid battery level jump to 0%
		 * at a moment from more than 2% */
		allow_change = ((new_run_time -
				cw_bat->run_time_capacity_change) /
				BATTERY_DOWN_MIN_CHANGE_RUN);

		allow_change += ((new_sleep_time -
				cw_bat->sleep_time_capacity_change) /
				BATTERY_DOWN_MIN_CHANGE_SLEEP);

		allow_capacity = cw_bat->capacity - allow_change;
		cw_capacity = (allow_capacity >= cw_capacity) ?
				allow_capacity : cw_capacity;
		dev_info(cw_bat->dev, "report GGIC POR happened");

		ret = cw_reset_reg_mode(cw_bat);
		if (ret < 0)
			return ret;

		dev_info(cw_bat->dev, "report battery capacity error");
		ret = cw_update_config_info(cw_bat);
		if (ret)
			return ret;

		dev_info(cw_bat->dev, "report battery capacity jump 0 ");
	}

	if (cw_bat->charger_mode > 0 && cw_capacity == 0) {
		charge_time = new_sleep_time + new_run_time -
			cw_bat->sleep_time_charge_start -
			cw_bat->run_time_charge_start;

		if (charge_time > BATTERY_DOWN_MAX_CHANGE_RUN_AC_ONLINE &&
				if_quickstart == 0) {

			ret = cw_reset_reg_mode(cw_bat);
			if (ret < 0)
				return ret;

			dev_info(cw_bat->dev,
					"report battery capacity error");
			ret = cw_update_config_info(cw_bat);
			if (ret)
				return ret;
			dev_info(cw_bat->dev,
				"report battery capacity still 0 if in changing");

			if_quickstart = 1;
		}
	} else if (if_quickstart == 1 && cw_bat->charger_mode == 0)
		if_quickstart = 0;

	if (gpio_is_valid(cw_bat->plat_data.chg_ok_pin)) {
		if (gpio_get_value(cw_bat->plat_data.chg_ok_pin) !=
				cw_bat->plat_data.chg_ok_level) {
			if (cw_capacity == 100)
				cw_capacity = 99;
		} else {
			if (cw_bat->charger_mode > 0)
				cw_capacity = 100;
		}
	}

#ifdef SYSTEM_SHUTDOWN_VOLTAGE
	if (cw_bat->charger_mode == 0 && cw_capacity <= 10 &&
			cw_bat->voltage <= SYSTEM_SHUTDOWN_VOLTAGE) {
		if (if_quickstart == 10) {
			cw_quickstart(cw_bat);
			if_quickstart = 12;
			cw_capacity = 0;
		} else if (if_quickstart <= 10)
			if_quickstart = if_quickstart + 2;

		dev_info(cw_bat->dev,
			"voltage is less than SYSTEM_SHUTDOWN_VOLTAGE\n");
	} else if (cw_bat->charger_mode > 0 && if_quickstart <= 12)
		if_quickstart = 0;
#endif
	return cw_capacity;
}

static int cw_get_vol(struct cw_battery *cw_bat)
{
	int ret;
	u8 reg_val[2];
	u16 value16, value16_1, value16_2, value16_3;
	int voltage;

	ret = cw_read_word(cw_bat->client, REG_VCELL, reg_val);
	if (ret < 0)
		return ret;
	value16 = (reg_val[0] << 8) + reg_val[1];

	ret = cw_read_word(cw_bat->client, REG_VCELL, reg_val);
	if (ret < 0)
		return ret;
	value16_1 = (reg_val[0] << 8) + reg_val[1];

	ret = cw_read_word(cw_bat->client, REG_VCELL, reg_val);
	if (ret < 0)
		return ret;
	value16_2 = (reg_val[0] << 8) + reg_val[1];

	if (value16 > value16_1) {
		value16_3 = value16;
		value16 = value16_1;
		value16_1 = value16_3;
	}

	if (value16_1 > value16_2) {
		value16_3 = value16_1;
		value16_1 = value16_2;
		value16_2 = value16_3;
	}

	if (value16 > value16_1) {
		value16_3 = value16;
		value16 = value16_1;
		value16_1 = value16_3;
	}

	voltage = value16_1 * 305;
	dev_dbg(cw_bat->dev, "voltage=%d,reg_val=%x %x\n", voltage, reg_val[0],
			reg_val[1]);
	return voltage;
}

#ifdef BAT_LOW_INTERRUPT
static int cw_get_alt(struct cw_battery *cw_bat)
{
	int ret = 0;
	u8 reg_val = 0;
	u8 value8 = 0;
	int alrt;

	ret = cw_read(cw_bat->client, REG_RRT_ALERT, &reg_val);
	if (ret < 0)
		return ret;
	value8 = reg_val;
	alrt = value8 >> 7;

	value8 = value8 & 0x7f;
	reg_val = value8;
	ret = cw_write(cw_bat->client, REG_RRT_ALERT, &reg_val);
	if (ret < 0) {
		dev_err(cw_bat->dev, "Error clear ALRT\n");
		return ret;
	}

	return alrt;
}
#endif

static int cw_get_time_to_empty(struct cw_battery *cw_bat)
{
	int ret;
	u8 reg_val = 0;
	u16 value16 = 0;

	ret = cw_read(cw_bat->client, REG_RRT_ALERT, &reg_val);
	if (ret < 0)
		return ret;

	value16 = reg_val;

	ret = cw_read(cw_bat->client, REG_RRT_ALERT + 1, &reg_val);
	if (ret < 0)
		return ret;

	value16 = ((value16 << 8) + reg_val) & 0x1fff;
	return value16;
}

static void rk_bat_update_capacity(struct cw_battery *cw_bat)
{
	int cw_capacity;

	cw_capacity = cw_get_capacity(cw_bat);
	if (cw_capacity >= 0 && cw_capacity <= 100 &&
			cw_bat->capacity != cw_capacity) {
		cw_bat->capacity = cw_capacity;
		cw_bat->bat_change = 1;
		cw_update_time_member_capacity_change(cw_bat);

		if (cw_bat->capacity == 0)
			dev_info(cw_bat->dev,
				"report battery capacity 0 and will shutdown if no changing");
	}
}

static void rk_bat_update_vol(struct cw_battery *cw_bat)
{
	int ret;

	ret = cw_get_vol(cw_bat);
	if (ret >= 0 && cw_bat->voltage != ret)
		cw_bat->voltage = ret;
}

static void rk_bat_update_status(struct cw_battery *cw_bat)
{
	int status;

	if (cw_bat->charger_mode > 0) {
		if (cw_bat->capacity >= 100)
			status = POWER_SUPPLY_STATUS_FULL;
		else
			status = POWER_SUPPLY_STATUS_CHARGING;
	} else
		status = POWER_SUPPLY_STATUS_NOT_CHARGING;

	if (cw_bat->status != status) {
		cw_bat->status = status;
		cw_bat->bat_change = 1;
	}
}

static void rk_bat_update_time_to_empty(struct cw_battery *cw_bat)
{
	int ret;

	ret = cw_get_time_to_empty(cw_bat);
	if (ret >= 0 && cw_bat->time_to_empty != ret)
		cw_bat->time_to_empty = ret;
}

static int rk_ac_update_online(struct cw_battery *cw_bat)
{
	int ret = 0;

	/* Need to check charger status directly and update it */

	if (!gpio_is_valid(cw_bat->plat_data.dc_det_pin)) {
		cw_bat->dc_online = 0;
		dev_dbg(cw_bat->dev,
			"dc charger but without dc_det_pin,maybe error\n");
		return 0;
	}

	if (gpio_get_value(cw_bat->plat_data.dc_det_pin) ==
			cw_bat->plat_data.dc_det_level) {
		if (cw_bat->dc_online != 1) {
			cw_update_time_member_charge_start(cw_bat);
			cw_bat->dc_online = 1;
			if (cw_bat->charger_mode != AC_CHARGER_MODE)
				cw_bat->charger_mode = AC_CHARGER_MODE;

			ret = 1;
		}
	} else {
		if (cw_bat->dc_online != 0) {
			cw_update_time_member_charge_start(cw_bat);
			cw_bat->dc_online = 0;
			if (cw_bat->usb_online == 0)
				cw_bat->charger_mode = 0;
			ret = 1;
		}
	}
	return ret;
}

#ifdef SUPPORT_USB_CHARGE_MODE
static int get_usb_charge_state(struct cw_battery *cw_bat)
{
	int charge_time;
	int time_from_boot;
	struct timespec ts;
	int gadget_status, usb_status;

	if (cw_bat->charger_mode != USB_CHARGER_MODE)
		return 0;

	gadget_status = get_gadget_connect_flag();
	usb_status = dwc_vbus_status();

	get_monotonic_boottime(&ts);
	time_from_boot = ts.tv_sec;

	if (cw_bat->charger_init_mode) {
		if (usb_status == 1 || usb_status == 2)
			cw_bat->charger_init_mode = 0;
		else if (time_from_boot < 8)
			usb_status = cw_bat->charger_init_mode;
		else if (strstr(saved_command_line, "charger")) {
			cw_bat->charger_init_mode = dwc_otg_check_dpdm();
			usb_status = cw_bat->charger_init_mode;
		}
	}
#ifdef NO_STANDARD_AC_BIG_CHARGE_MODE
	if (cw_bat->usb_online == 1) {
		charge_time = time_from_boot -
			cw_bat->sleep_time_charge_start -
			cw_bat->run_time_charge_start;

		if (charge_time > 3) {
			if (gadget_status == 0 && dwc_vbus_status() == 1)
				usb_status = 2;
		}
	}
#endif

	return usb_status;
}
#else
static int get_usb_charge_state(struct cw_battery *cw_bat)
{
	return 0;
}
#endif	/* End of SUPPORT_USB_CHARGE_MODE */

static int rk_usb_update_online(struct cw_battery *cw_bat)
{
	int ret = 0;
	int usb_status = 0;
	int sel_pin = cw_bat->plat_data.chg_mode_sel_pin;
	int sel_level = cw_bat->plat_data.chg_mode_sel_level;

	usb_status = get_usb_charge_state(cw_bat);
	if (usb_status == 2) {
		if (cw_bat->charger_mode != AC_CHARGER_MODE) {
			cw_bat->charger_mode = AC_CHARGER_MODE;
			ret = 1;
		}

		if (gpio_is_valid(sel_pin)) {
			if (gpio_get_value(sel_pin) != sel_level)
				gpio_direction_output(sel_pin,
						sel_level == CW2015_GPIO_LOW ?
						CW2015_GPIO_LOW :
						CW2015_GPIO_HIGH);
		}

		if (cw_bat->usb_online != 1) {
			cw_bat->usb_online = 1;
			cw_update_time_member_charge_start(cw_bat);
		}
	} else if (usb_status == 1) {
		if (cw_bat->charger_mode != USB_CHARGER_MODE &&
				cw_bat->dc_online == 0) {
			cw_bat->charger_mode = USB_CHARGER_MODE;
			ret = 1;
		}

		if (gpio_is_valid(sel_pin)) {
			if (gpio_get_value(sel_pin) == sel_level)
				gpio_direction_output(sel_pin,
						sel_level == CW2015_GPIO_LOW ?
						CW2015_GPIO_HIGH :
						CW2015_GPIO_LOW);
		}
		if (cw_bat->usb_online != 1) {
			cw_bat->usb_online = 1;
			cw_update_time_member_charge_start(cw_bat);
		}
	} else if (usb_status == 0 && cw_bat->usb_online != 0) {
		if (gpio_is_valid(sel_pin)) {
			if (gpio_get_value(sel_pin == sel_level))
				gpio_direction_output(sel_pin,
						sel_level == CW2015_GPIO_LOW ?
						CW2015_GPIO_HIGH :
						CW2015_GPIO_LOW);
		}

		if (cw_bat->dc_online == 0)
			cw_bat->charger_mode = 0;

		cw_update_time_member_charge_start(cw_bat);
		cw_bat->usb_online = 0;
		ret = 1;
	}

	return ret;
}

static void cw_bat_work(struct work_struct *work)
{
	struct delayed_work *delay_work;
	struct cw_battery *cw_bat;
	int ret;

	delay_work = container_of(work, struct delayed_work, work);
	cw_bat = container_of(delay_work, struct cw_battery,
			battery_delay_work);
	if (cw_bat->plat_data.is_dc_charge == 1) {
		ret = rk_ac_update_online(cw_bat);
		if (ret == 1)
			power_supply_changed(cw_bat->rk_ac);
	}

	if (cw_bat->plat_data.is_usb_charge == 1) {
		ret = rk_usb_update_online(cw_bat);
		if (ret == 1) {
			power_supply_changed(cw_bat->rk_usb);
			power_supply_changed(cw_bat->rk_ac);
		}
	}

	rk_bat_update_status(cw_bat);
	rk_bat_update_capacity(cw_bat);
	rk_bat_update_vol(cw_bat);
	rk_bat_update_time_to_empty(cw_bat);

	if (cw_bat->bat_change) {
		power_supply_changed(cw_bat->rk_bat);
		cw_bat->bat_change = 0;
	}

	queue_delayed_work(cw_bat->battery_workqueue,
			&cw_bat->battery_delay_work, msecs_to_jiffies(1000));
	dev_dbg(cw_bat->dev, "bat_change = %d, time_to_empty = %d, capacity = %d\n",
			cw_bat->bat_change, cw_bat->time_to_empty,
			cw_bat->capacity);

	dev_dbg(cw_bat->dev, "voltage = %d, dc_online = %d, usb_online = %d\n",
			cw_bat->voltage, cw_bat->dc_online,
			cw_bat->usb_online);
}

#ifdef SUPPORT_USB_CHARGE_MODE
static int rk_usb_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	int ret = 0;
	struct cw_battery *cw_bat;

	cw_bat = container_of(psy, struct cw_battery, rk_usb);
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = (cw_bat->charger_mode ==
				USB_CHARGER_MODE);
		break;
	default:
		break;
	}
	return ret;
}

static enum power_supply_property rk_usb_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};
#endif

static int rk_ac_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	int ret = 0;
	struct cw_battery *cw_bat = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = (cw_bat->charger_mode ==
				AC_CHARGER_MODE);
		break;
	default:
		break;
	}
	return ret;
}

static enum power_supply_property rk_ac_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static int rk_battery_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	int ret = 0;
	struct cw_battery *cw_bat = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = cw_bat->capacity;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = cw_bat->status;
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = cw_bat->voltage <= 0 ? 0 : 1;
		break;

	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = cw_bat->voltage;
		break;

	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		val->intval = cw_bat->time_to_empty;
		break;

	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;

	default:
		break;
	}
	return ret;
}

static enum power_supply_property rk_battery_properties[] = {
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	POWER_SUPPLY_PROP_TECHNOLOGY,
};

static void cw_bat_gpio_free(struct cw_battery *cw_bat)
{
	if (gpio_is_valid(cw_bat->plat_data.chg_mode_sel_pin))
		gpio_free(cw_bat->plat_data.chg_mode_sel_pin);
	if (gpio_is_valid(cw_bat->plat_data.bat_low_pin))
		gpio_free(cw_bat->plat_data.bat_low_pin);
	if (gpio_is_valid(cw_bat->plat_data.dc_det_pin))
		gpio_free(cw_bat->plat_data.dc_det_pin);
}

static int cw_bat_gpio_init(struct cw_battery *cw_bat)
{
	int ret;

	if (gpio_is_valid(cw_bat->plat_data.dc_det_pin)) {
		ret = gpio_request(cw_bat->plat_data.dc_det_pin, NULL);
		if (ret) {
			dev_err(cw_bat->dev,
					"failed to request dc_det_pin gpio\n");
			goto request_dc_det_pin_fail;
		}
		ret = gpio_direction_input(cw_bat->plat_data.dc_det_pin);
		if (ret) {
			dev_err(cw_bat->dev,
					"failed to set dc_det_pin input\n");
			goto request_bat_low_pin_fail;
		}
	}

	if (gpio_is_valid(cw_bat->plat_data.bat_low_pin)) {
		ret = gpio_request(cw_bat->plat_data.bat_low_pin, NULL);
		if (ret) {
			dev_err(cw_bat->dev,
				"failed to request bat_low_pin gpio\n");
			goto request_bat_low_pin_fail;
		}

		ret = gpio_direction_input(cw_bat->plat_data.bat_low_pin);
		if (ret) {
			dev_err(cw_bat->dev,
					"failed to set bat_low_pin input\n");
			goto request_bat_low_pin_fail;
		}
	}
	if (gpio_is_valid(cw_bat->plat_data.chg_ok_pin)) {
		ret = gpio_request(cw_bat->plat_data.chg_ok_pin, NULL);
		if (ret) {
			dev_err(cw_bat->dev,
					"failed to request chg_ok_pin gpio\n");
			goto request_chg_ok_pin_fail;
		}

		ret = gpio_direction_input(cw_bat->plat_data.chg_ok_pin);
		if (ret) {
			dev_err(cw_bat->dev,
					"failed to set chg_ok_pin input\n");
			gpio_free(cw_bat->plat_data.chg_ok_pin);
			goto request_chg_ok_pin_fail;
		}
	}

	if (gpio_is_valid(cw_bat->plat_data.chg_mode_sel_pin)) {
		ret = gpio_request(cw_bat->plat_data.chg_mode_sel_pin, NULL);
		if (ret) {
			dev_err(cw_bat->dev,
				"failed to request chg_mode_sel_pin gpio\n");
			goto request_chg_mode_sel_pin_fail;
		}
		ret = gpio_direction_output(cw_bat->plat_data.chg_mode_sel_pin,
				(cw_bat->plat_data.chg_mode_sel_level ==
				 CW2015_GPIO_LOW) ? CW2015_GPIO_HIGH :
						    CW2015_GPIO_LOW);
		if (ret) {
			dev_err(cw_bat->dev,
				"failed to set chg_mode_sel_pin output\n");
			gpio_free(cw_bat->plat_data.chg_mode_sel_pin);
			goto request_chg_mode_sel_pin_fail;
		}
	}

	return 0;

request_chg_mode_sel_pin_fail:
	if (gpio_is_valid(cw_bat->plat_data.chg_mode_sel_pin))
		gpio_free(cw_bat->plat_data.chg_mode_sel_pin);

request_chg_ok_pin_fail:
	if (gpio_is_valid(cw_bat->plat_data.bat_low_pin))
		gpio_free(cw_bat->plat_data.bat_low_pin);

request_bat_low_pin_fail:
	if (gpio_is_valid(cw_bat->plat_data.dc_det_pin))
		gpio_free(cw_bat->plat_data.dc_det_pin);

request_dc_det_pin_fail:
	return ret;
}

static void dc_detect_do_wakeup(struct work_struct *work)
{
	int ret;
	int irq;
	unsigned int type;

	struct delayed_work *delay_work;
	struct cw_battery *cw_bat;

	delay_work = container_of(work, struct delayed_work, work);
	cw_bat = container_of(delay_work, struct cw_battery, dc_wakeup_work);

	irq = gpio_to_irq(cw_bat->plat_data.dc_det_pin);
	type = gpio_get_value(cw_bat->plat_data.dc_det_pin) ?
		IRQ_TYPE_EDGE_FALLING : IRQ_TYPE_EDGE_RISING;
	ret = irq_set_irq_type(irq, type);
	if (ret < 0)
		dev_err(cw_bat->dev, "irq_set_irq_type(%d, %d) failed\n",
				irq, type);
	enable_irq(irq);
}

static irqreturn_t dc_detect_irq_handler(int irq, void *dev_id)
{
	struct cw_battery *cw_bat = dev_id;

	disable_irq_nosync(irq);
	queue_delayed_work(cw_bat->battery_workqueue, &cw_bat->dc_wakeup_work,
			msecs_to_jiffies(20));
	return IRQ_HANDLED;
}

#ifdef BAT_LOW_INTERRUPT

#define WAKE_LOCK_TIMEOUT       (10 * HZ)

static void bat_low_detect_do_wakeup(struct work_struct *work)
{
	struct delayed_work *delay_work;
	struct cw_battery *cw_bat;

	delay_work = container_of(work, struct delayed_work, work);
	cw_bat = container_of(delay_work, struct cw_battery,
			bat_low_wakeup_work);
	cw_get_alt(cw_bat);
}

static irqreturn_t bat_low_detect_irq_handler(int irq, void *dev_id)
{
	struct cw_battery *cw_bat = dev_id;

	queue_delayed_work(cw_bat->battery_workqueue,
			&cw_bat->bat_low_wakeup_work, msecs_to_jiffies(20));
	return IRQ_HANDLED;
}
#endif

#ifdef CONFIG_OF
static int cw2015_parse_dt(struct device *dev,
		struct cw_bat_platform_data *data)
{
	struct device_node *node = dev->of_node;
	enum of_gpio_flags flags;
	struct property *prop;
	int length;
	u32 value;
	int ret;

	if (!node)
		return -ENODEV;

	memset(data, 0, sizeof(*data));

	/* determine the number of config info */
	prop = of_find_property(node, "bat_config_info", &length);
	if (!prop)
		return -EINVAL;

	length /= sizeof(u32);

	if (length > 0) {
		size_t size = sizeof(*data->cw_bat_config_info) * length;

		data->cw_bat_config_info = devm_kzalloc(dev, size, GFP_KERNEL);
		if (!data->cw_bat_config_info)
			return -ENOMEM;

		ret = of_property_read_u32_array(node, "bat_config_info",
				data->cw_bat_config_info,
				length);
		if (ret < 0)
			return ret;
	}

	data->dc_det_pin = of_get_named_gpio_flags(node, "dc_det_gpio", 0,
			&flags);
	if (data->dc_det_pin == -EPROBE_DEFER)
		dev_err(dev, "dc_det_gpio error\n");

	if (gpio_is_valid(data->dc_det_pin))
		data->dc_det_level = (flags & OF_GPIO_ACTIVE_LOW) ?
			CW2015_GPIO_LOW : CW2015_GPIO_HIGH;

	data->bat_low_pin = of_get_named_gpio_flags(node, "bat_low_gpio", 0,
			&flags);
	if (data->bat_low_pin == -EPROBE_DEFER)
		dev_err(dev, "bat_low_gpio error\n");

	if (gpio_is_valid(data->bat_low_pin))
		data->bat_low_level = (flags & OF_GPIO_ACTIVE_LOW) ?
			CW2015_GPIO_LOW : CW2015_GPIO_HIGH;

	data->chg_ok_pin = of_get_named_gpio_flags(node, "chg_ok_gpio", 0,
			&flags);
	if (data->chg_ok_pin == -EPROBE_DEFER)
		dev_err(dev, "chg_ok_gpio error\n");

	if (gpio_is_valid(data->chg_ok_pin))
		data->chg_ok_level = (flags & OF_GPIO_ACTIVE_LOW) ?
			CW2015_GPIO_LOW : CW2015_GPIO_HIGH;

	data->chg_mode_sel_pin = of_get_named_gpio_flags(node,
			"chg_mode_sel_gpio", 0, &flags);
	if (data->chg_mode_sel_pin == -EPROBE_DEFER)
		dev_err(dev, "chg_mod_sel_gpio error\n");

	if (gpio_is_valid(data->chg_mode_sel_pin))
		data->chg_mode_sel_level = (flags & OF_GPIO_ACTIVE_LOW) ?
			CW2015_GPIO_LOW : CW2015_GPIO_HIGH;

	/* Need to set is_dc_charge to 1 */
	ret = of_property_read_u32(node, "is_dc_charge", &value);
	if (ret < 0) {
		dev_info(dev, "hardware unsupport dc charge\n");
		value = 0;
	}
	data->is_dc_charge = value;

	ret = of_property_read_u32(node, "is_usb_charge", &value);
	if (ret < 0) {
		dev_info(dev, "hardware unsupport usb charge\n");
		value = 0;
	}
	data->is_usb_charge = value;

	dev_info(dev, "cw201x:support %s %s charger\n",
			data->is_dc_charge ? "DC" : "",
			data->is_usb_charge ? "USB" : "");

	return 0;
}
#else
static int cw2015_parse_dt(struct device *dev,
		struct cw_bat_platform_data *data)
{
	return -ENODEV;
}
#endif

#ifdef CONFIG_SYSFS
static ssize_t cw2015_show_charger_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cw_battery *cw_bat = i2c_get_clientdata(client);

	return scnprintf(buf, PAGE_SIZE,
			"online(%d), change(%d), vol(%d), cap(%d) tte(%d)\n",
			cw_bat->dc_online,
			cw_bat->bat_change,
			cw_bat->voltage,
			cw_bat->capacity,
			cw_bat->time_to_empty);
}

static DEVICE_ATTR(charger_status, S_IRUSR, cw2015_show_charger_status, NULL);

static struct attribute *cw2015_charger_attr[] = {
	&dev_attr_charger_status.attr,
	NULL,
};

static const struct attribute_group cw2015_attr_group = {
	.attrs = cw2015_charger_attr,
};

#endif

static int cw_power_supply_init(struct cw_battery *cw_bat)
{
	struct power_supply_config ac_cfg = { .drv_data = cw_bat, };
	struct power_supply_config bat_cfg = { .drv_data = cw_bat, };
#ifdef SUPPORT_USB_CHARGE_MODE
	struct power_supply_config usb_cfg = { .drv_data = cw_bat, };
#endif

	cw_bat->rk_bat_desc.name = "rk-bat";
	cw_bat->rk_bat_desc.type = POWER_SUPPLY_TYPE_BATTERY;
	cw_bat->rk_bat_desc.properties = rk_battery_properties;
	cw_bat->rk_bat_desc.num_properties = ARRAY_SIZE(rk_battery_properties);
	cw_bat->rk_bat_desc.get_property = rk_battery_get_property;
	cw_bat->rk_bat = power_supply_register(cw_bat->dev,
			&cw_bat->rk_bat_desc, &bat_cfg);
	if (IS_ERR(cw_bat->rk_bat)) {
		dev_err(cw_bat->dev, "power supply register rk_bat error\n");
		return PTR_ERR(cw_bat->rk_bat);
	}

	cw_bat->rk_ac_desc.name = "rk-ac";
	cw_bat->rk_ac_desc.type = POWER_SUPPLY_TYPE_MAINS;
	cw_bat->rk_ac_desc.properties = rk_ac_properties;
	cw_bat->rk_ac_desc.num_properties = ARRAY_SIZE(rk_ac_properties);
	cw_bat->rk_ac_desc.get_property = rk_ac_get_property;
	cw_bat->rk_ac = power_supply_register(cw_bat->dev, &cw_bat->rk_ac_desc,
			&ac_cfg);
	if (IS_ERR(cw_bat->rk_ac)) {
		dev_err(cw_bat->dev, "power supply register rk_ac error\n");
		return PTR_ERR(cw_bat->rk_ac);
	}

#ifdef SUPPORT_USB_CHARGE_MODE
	if (cw_bat->plat_data.is_usb_charge == 1) {
		cw_bat->rk_usb->desc.name = "rk-usb";
		cw_bat->rk_usb->desc.type = POWER_SUPPLY_TYPE_USB;
		cw_bat->rk_usb->desc.properties = rk_usb_properties;
		cw_bat->rk_usb->desc.num_properties =
			ARRAY_SIZE(rk_usb_properties);
		cw_bat->rk_usb->desc.get_property = rk_usb_get_property;
		cw_bat->rk_usb = power_supply_register(cw_bat->dev,
				&cw_bat->rk_usb->desc, &usb_cfg);
		if (IS_ERR(cw_bat->rk_usb)) {
			dev_err(cw_bat->dev, "power supply register rk_usb error\n");
			return PTR_ERR(cw_bat->rk_usb);
		}

		cw_bat->charger_init_mode = dwc_otg_check_dpdm();
		dev_info("cw2015 support charger by usb. usb_mode=%d\n",
				cw_bat->charger_init_mode);
	}
#endif

	return 0;
}

static int cw_bat_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct cw_battery *cw_bat;
	struct cw_bat_platform_data *plat_data = client->dev.platform_data;
	int ret, irq, irq_flags, level = 0;
	struct cw_bat_platform_data *data;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	cw_bat = devm_kzalloc(&client->dev, sizeof(*cw_bat), GFP_KERNEL);
	if (!cw_bat)
		return -ENOMEM;

	cw_bat->dev = &client->dev;

	/* Make sure we have at least i2c functionality on the bus */
	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "No i2c functionality available\n");
		return -EIO;
	}

	i2c_set_clientdata(client, cw_bat);

	if (!plat_data) {
		ret = cw2015_parse_dt(&client->dev, &(cw_bat->plat_data));
		if (ret < 0) {
			dev_err(&client->dev,
				"failed to find cw2015 platform data\n");
			goto pdate_fail;
		}
		data = &(cw_bat->plat_data);
	}

#ifdef CONFIG_SYSFS
	ret = sysfs_create_group(&client->dev.kobj, &cw2015_attr_group);
	if (ret) {
		dev_err(&client->dev, "Failed to create cw2015 sysfs\n");
		goto pdate_fail;
	}
#endif

	cw_bat->client = client;
	ret = cw_bat_gpio_init(cw_bat);
	if (ret) {
		dev_err(cw_bat->dev, "cw_bat_gpio_init error\n");
		goto gpio_init_fail;
	}

	ret = cw_init(cw_bat);
	if (ret) {
		dev_err(cw_bat->dev, "cw_init error\n");
		goto cw_init_fail;
	}

	ret = cw_power_supply_init(cw_bat);
	if (ret) {
		dev_err(cw_bat->dev, "failed to register power supply: %d\n",
				ret);
		goto irq_fail;
	}

	cw_bat->dc_online = 0;
	cw_bat->usb_online = 0;
	cw_bat->charger_mode = 0;
	cw_bat->capacity = 2; /* Is it reseonable? */
	cw_bat->voltage = 0;
	cw_bat->status = 0;
	cw_bat->time_to_empty = 0;
	cw_bat->bat_change = 0;

	cw_update_time_member_capacity_change(cw_bat);
	cw_update_time_member_charge_start(cw_bat);

	cw_bat->battery_workqueue = create_singlethread_workqueue("rk_battery");
	INIT_DELAYED_WORK(&cw_bat->battery_delay_work, cw_bat_work);
	INIT_DELAYED_WORK(&cw_bat->dc_wakeup_work, dc_detect_do_wakeup);
	queue_delayed_work(cw_bat->battery_workqueue,
			&cw_bat->battery_delay_work, msecs_to_jiffies(10));

	if (gpio_is_valid(cw_bat->plat_data.dc_det_pin)) {
		irq = gpio_to_irq(cw_bat->plat_data.dc_det_pin);
		level = gpio_get_value(cw_bat->plat_data.dc_det_pin);
		if (level == cw_bat->plat_data.dc_det_level) {
			dev_dbg(cw_bat->dev, "booting up with dc plug\n");
			cw_bat->status = POWER_SUPPLY_STATUS_CHARGING;
		}
		irq_flags = level ? IRQF_TRIGGER_FALLING : IRQF_TRIGGER_RISING;
		ret = request_irq(irq, dc_detect_irq_handler, irq_flags,
				"dc_detect", cw_bat);
		if (ret < 0) {
			dev_err(cw_bat->dev, "request_irq(%d) failed\n", irq);
			goto irq_fail;
		}
		enable_irq_wake(irq);
	}

	if (gpio_is_valid(cw_bat->plat_data.bat_low_pin)) {
		INIT_DELAYED_WORK(&cw_bat->bat_low_wakeup_work,
				bat_low_detect_do_wakeup);
		level = gpio_get_value(cw_bat->plat_data.bat_low_pin);
		if (level == cw_bat->plat_data.dc_det_level) {
			dev_dbg(cw_bat->dev, "booting up with lower power\n");
			cw_bat->capacity = 1;
		}
		irq = gpio_to_irq(cw_bat->plat_data.bat_low_pin);
		ret = request_irq(irq, bat_low_detect_irq_handler,
				IRQF_TRIGGER_RISING, "bat_low_detect", cw_bat);
		if (ret < 0) {
			gpio_free(cw_bat->plat_data.bat_low_pin);
			goto bat_low_pin_fail;
		}
		enable_irq_wake(irq);
	}

	dev_info(cw_bat->dev, "cw2015/cw2013 driver v1.2 probe sucess\n");

	return 0;

bat_low_pin_fail:
	if (gpio_is_valid(cw_bat->plat_data.dc_det_pin)) {
		disable_irq_wake(gpio_to_irq(cw_bat->plat_data.dc_det_pin));
		free_irq(gpio_to_irq(cw_bat->plat_data.dc_det_pin), cw_bat);
	}

irq_fail:
#ifdef SUPPORT_USB_CHARGE_MODE
	if (cw_bat->plat_data.is_usb_charge == 1)
		power_supply_unregister(cw_bat->rk_usb);
rk_usb_register_fail:
#endif
	power_supply_unregister(cw_bat->rk_ac);
	power_supply_unregister(cw_bat->rk_bat);
cw_init_fail:
	cw_bat_gpio_free(cw_bat);
gpio_init_fail:
#ifdef CONFIG_SYSFS
	sysfs_remove_group(&client->dev.kobj, &cw2015_attr_group);
#endif
pdate_fail:
	dev_err(cw_bat->dev,
			"cw2015/cw2013 driver v1.2 probe error!!!!\n");
	return ret;
}

static int cw_bat_remove(struct i2c_client *client)
{
	struct cw_battery *cw_bat = i2c_get_clientdata(client);

	cancel_delayed_work(&cw_bat->battery_delay_work);

	if (gpio_is_valid(cw_bat->plat_data.bat_low_pin)) {
		disable_irq_wake(gpio_to_irq(cw_bat->plat_data.bat_low_pin));
		free_irq(gpio_to_irq(cw_bat->plat_data.bat_low_pin), cw_bat);
	}

	if (gpio_is_valid(cw_bat->plat_data.dc_det_pin)) {
		disable_irq_wake(gpio_to_irq(cw_bat->plat_data.dc_det_pin));
		free_irq(gpio_to_irq(cw_bat->plat_data.dc_det_pin), cw_bat);
	}

#ifdef SUPPORT_USB_CHARGE_MODE
	if (cw_bat->plat_data.is_usb_charge == 1)
		power_supply_unregister(cw_bat->rk_usb);
#endif

	power_supply_unregister(cw_bat->rk_ac);
	power_supply_unregister(cw_bat->rk_bat);

	cw_bat_gpio_free(cw_bat);
#ifdef CONFIG_SYSFS
	sysfs_remove_group(&cw_bat->client->dev.kobj, &cw2015_attr_group);
#endif

	return 0;
}

#ifdef CONFIG_PM
static int cw_bat_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cw_battery *cw_bat = i2c_get_clientdata(client);

	cancel_delayed_work(&cw_bat->battery_delay_work);
	return 0;
}

static int cw_bat_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cw_battery *cw_bat = i2c_get_clientdata(client);

	queue_delayed_work(cw_bat->battery_workqueue,
			&cw_bat->battery_delay_work, msecs_to_jiffies(100));
	return 0;
}

static const struct i2c_device_id cw_id[] = {
	{ "cw201x", 0 },
	{  }
};
MODULE_DEVICE_TABLE(i2c, cw_id);

static const struct dev_pm_ops cw_bat_pm_ops = {
	.suspend  = cw_bat_suspend,
	.resume   = cw_bat_resume,
};
#endif

static struct i2c_driver cw_bat_driver = {
	.driver         = {
		.name   = "cw201x",
#ifdef CONFIG_PM
		.pm     = &cw_bat_pm_ops,
#endif
	},

	.probe          = cw_bat_probe,
	.remove         = cw_bat_remove,
	.id_table	= cw_id,
};

static int __init cw_bat_init(void)
{
	return i2c_add_driver(&cw_bat_driver);
}

static void __exit cw_bat_exit(void)
{
	i2c_del_driver(&cw_bat_driver);
}

fs_initcall(cw_bat_init);
module_exit(cw_bat_exit);

MODULE_AUTHOR("xhc<xhc@rock-chips.com>");
MODULE_DESCRIPTION("cw2015/cw2013 battery driver");
MODULE_LICENSE("GPL");
