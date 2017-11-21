/*
 * TI BQ25895M charger driver
 *
 * Copyright (C) 2015 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/types.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/usb/phy.h>

#include <linux/acpi.h>
#include <linux/of.h>

#define BQ25895M_MANUFACTURER		"Texas Instruments"
#define BQ25895M_IRQ_PIN			"bq25895m_irq"

#define BQ25895M_ID			7

enum bq25895m_fields {
	F_EN_HIZ, F_EN_ILIM, F_IILIM,				     /* Reg00 */
	F_BHOT, F_BCOLD, F_VINDPM_OFS,				     /* Reg01 */
	F_CONV_START, F_CONV_RATE, F_BOOSTF, F_ICO_EN,
	F_HVDCP_EN, F_MAXC_EN, F_FORCE_DPM, F_AUTO_DPDM_EN,	     /* Reg02 */
	F_BAT_LOAD_EN, F_WD_RST, F_OTG_CFG, F_CHG_CFG, F_SYSVMIN,
	F_MIN_VBAT_SEL, /* Reg03 */
	F_PUMPX_EN, F_ICHG,					     /* Reg04 */
	F_IPRECHG, F_ITERM,					     /* Reg05 */
	F_VREG, F_BATLOWV, F_VRECHG,				     /* Reg06 */
	F_TERM_EN, F_STAT_DIS, F_WD, F_TMR_EN, F_CHG_TMR, /* Reg07 */
	F_BATCMP, F_VCLAMP, F_TREG,				     /* Reg08 */
	F_FORCE_ICO, F_TMR2X_EN, F_BATFET_DIS,
	F_BATFET_DLY, F_BATFET_RST_EN, F_PUMPX_UP, F_PUMPX_DN,	     /* Reg09 */
	F_BOOSTV, F_PFM_OTG_DIS, F_BOOSTI,					     /* Reg0A */
	F_VBUS_STAT, F_CHG_STAT, F_PG_STAT, F_SDP_STAT, F_VSYS_STAT, /* Reg0B */
	F_WD_FAULT, F_BOOST_FAULT, F_CHG_FAULT, F_BAT_FAULT,
	F_NTC_FAULT,						     /* Reg0C */
	F_FORCE_VINDPM, F_VINDPM,				     /* Reg0D */
	F_THERM_STAT, F_BATV,					     /* Reg0E */
	F_SYSV,							     /* Reg0F */
	F_TSPCT,						     /* Reg10 */
	F_VBUS_GD, F_VBUSV,					     /* Reg11 */
	F_ICHGR,						     /* Reg12 */
	F_VDPM_STAT, F_IDPM_STAT, F_IDPM_LIM,			     /* Reg13 */
	F_REG_RST, F_ICO_OPTIMIZED, F_PN, F_TS_PROFILE, F_DEV_REV,   /* Reg14 */

	F_MAX_FIELDS
};

/* initial field values, converted to register values */
struct bq25895m_init_data {
	u8 ichg;	/* charge current		*/
	u8 vreg;	/* regulation voltage		*/
	u8 iterm;	/* termination current		*/
	u8 iprechg;	/* precharge current		*/
	u8 sysvmin;	/* minimum system voltage limit */
	u8 boostv;	/* boost regulation voltage	*/
	u8 boosti;	/* boost current limit		*/
	u8 boostf;	/* boost frequency		*/
	u8 ilim_en;	/* enable ILIM pin		*/
	u8 treg;	/* thermal regulation threshold */
};

struct bq25895m_state {
	u8 online;
	u8 chrg_status;
	u8 chrg_fault;
	u8 vsys_status;
	u8 boost_fault;
	u8 bat_fault;
};

struct bq25895m_device {
	struct i2c_client *client;
	struct device *dev;
	struct power_supply *charger;

	struct usb_phy *usb_phy;
	struct notifier_block usb_nb;
	struct work_struct usb_work;
	unsigned long usb_event;

	struct regmap *rmap;
	struct regmap_field *rmap_fields[F_MAX_FIELDS];

	int chip_id;
	struct bq25895m_init_data init_data;
	struct bq25895m_state state;

	struct mutex lock; /* protect state data */
};
static int bq25895m_reg_read(void *context, unsigned int reg,
		unsigned int *value)
{
	struct i2c_client *client = context;
	uint8_t send_buf, recv_buf[4];
	struct i2c_msg msgs[2];
	unsigned int size=1;
	unsigned int i;
	int ret;

	send_buf = reg;

	printk("%s: reg=0x%x \n", __FUNCTION__, reg);

	msgs[0].addr = client->addr;
	msgs[0].len = sizeof(send_buf);
	msgs[0].buf = &send_buf;
	msgs[0].flags = 0;

	msgs[1].addr = client->addr;
	msgs[1].len = size;
	msgs[1].buf = recv_buf;
	msgs[1].flags = I2C_M_RD;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0)
		return ret;
	else if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*value = 0;

	for (i = 0; i < size; i++) {
		*value <<= 8;
		*value |= recv_buf[i];
	}

	return 0;
}
static int bq25895m_reg_write(void *context, unsigned int reg,
		unsigned int value)
{
	struct i2c_client *client = context;
	struct tas5782_private *priv = i2c_get_clientdata(client);
	unsigned int i, size=1;
	uint8_t buf[5];
	int ret;

	buf[0] = reg;

	for (i = size; i >= 1; --i) {
		buf[i] = value;
		value >>= 8;
	}

	ret = i2c_master_send(client, buf, size + 1);
	if (ret == size + 1)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}

static const struct regmap_range bq25895m_readonly_reg_ranges[] = {
	regmap_reg_range(0x0b, 0x0c),
	regmap_reg_range(0x0e, 0x13),
};

static const struct regmap_access_table bq25895m_writeable_regs = {
	.no_ranges = bq25895m_readonly_reg_ranges,
	.n_no_ranges = ARRAY_SIZE(bq25895m_readonly_reg_ranges),
};

static const struct regmap_range bq25895m_volatile_reg_ranges[] = {
	regmap_reg_range(0x00, 0x00),
	regmap_reg_range(0x09, 0x09),
	regmap_reg_range(0x0b, 0x0c),
	regmap_reg_range(0x0e, 0x14),
};

static const struct regmap_access_table bq25895m_volatile_regs = {
	.yes_ranges = bq25895m_volatile_reg_ranges,
	.n_yes_ranges = ARRAY_SIZE(bq25895m_volatile_reg_ranges),
};

static const struct regmap_config bq25895m_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = 0x14,
	.cache_type = REGCACHE_RBTREE,
	.wr_table = &bq25895m_writeable_regs,
	.volatile_table = &bq25895m_volatile_regs,
};

static const struct reg_field bq25895m_reg_fields[] = {
	/* REG00 */
	[F_EN_HIZ]		= REG_FIELD(0x00, 7, 7),
	[F_EN_ILIM]		= REG_FIELD(0x00, 6, 6),
	[F_IILIM]		= REG_FIELD(0x00, 0, 5),
	/* REG01 */
	[F_BHOT]		= REG_FIELD(0x01, 6, 7),
	[F_BCOLD]		= REG_FIELD(0x01, 5, 5),
	[F_VINDPM_OFS]		= REG_FIELD(0x01, 0, 4),
	/* REG02 */
	[F_CONV_START]		= REG_FIELD(0x02, 7, 7),
	[F_CONV_RATE]		= REG_FIELD(0x02, 6, 6),
	[F_BOOSTF]		= REG_FIELD(0x02, 5, 5),
	[F_ICO_EN]		= REG_FIELD(0x02, 4, 4),
	[F_HVDCP_EN]		= REG_FIELD(0x02, 3, 3),
	[F_MAXC_EN]		= REG_FIELD(0x02, 2, 2),
	[F_FORCE_DPM]		= REG_FIELD(0x02, 1, 1),
	[F_AUTO_DPDM_EN]	= REG_FIELD(0x02, 0, 0),
	/* REG03 */
	[F_BAT_LOAD_EN]		= REG_FIELD(0x03, 7, 7),
	[F_WD_RST]		= REG_FIELD(0x03, 6, 6),
	[F_OTG_CFG]		= REG_FIELD(0x03, 5, 5),
	[F_CHG_CFG]		= REG_FIELD(0x03, 4, 4),
	[F_SYSVMIN]		= REG_FIELD(0x03, 1, 3),
	[F_MIN_VBAT_SEL]= REG_FIELD(0x03, 0, 0),
	/* REG04 */
	[F_PUMPX_EN]		= REG_FIELD(0x04, 7, 7),
	[F_ICHG]		= REG_FIELD(0x04, 0, 6),
	/* REG05 */
	[F_IPRECHG]		= REG_FIELD(0x05, 4, 7),
	[F_ITERM]		= REG_FIELD(0x05, 0, 3),
	/* REG06 */
	[F_VREG]		= REG_FIELD(0x06, 2, 7),
	[F_BATLOWV]		= REG_FIELD(0x06, 1, 1),
	[F_VRECHG]		= REG_FIELD(0x06, 0, 0),
	/* REG07 */
	[F_TERM_EN]		= REG_FIELD(0x07, 7, 7),
	[F_STAT_DIS]		= REG_FIELD(0x07, 6, 6),
	[F_WD]			= REG_FIELD(0x07, 4, 5),
	[F_TMR_EN]		= REG_FIELD(0x07, 3, 3),
	[F_CHG_TMR]		= REG_FIELD(0x07, 1, 2),
	/* REG08 */
	[F_BATCMP]		= REG_FIELD(0x08, 6, 7),
	[F_VCLAMP]		= REG_FIELD(0x08, 2, 4),
	[F_TREG]		= REG_FIELD(0x08, 0, 1),
	/* REG09 */
	[F_FORCE_ICO]		= REG_FIELD(0x09, 7, 7),
	[F_TMR2X_EN]		= REG_FIELD(0x09, 6, 6),
	[F_BATFET_DIS]		= REG_FIELD(0x09, 5, 5),
	[F_BATFET_DLY]		= REG_FIELD(0x09, 3, 3),
	[F_BATFET_RST_EN]	= REG_FIELD(0x09, 2, 2),
	[F_PUMPX_UP]		= REG_FIELD(0x09, 1, 1),
	[F_PUMPX_DN]		= REG_FIELD(0x09, 0, 0),
	/* REG0A */
	[F_BOOSTV]		= REG_FIELD(0x0A, 4, 7),
	[F_PFM_OTG_DIS]		= REG_FIELD(0x0A, 1, 3),
	[F_BOOSTI]		= REG_FIELD(0x0A, 0, 2),
	/* REG0B */
	[F_VBUS_STAT]		= REG_FIELD(0x0B, 5, 7),
	[F_CHG_STAT]		= REG_FIELD(0x0B, 3, 4),
	[F_PG_STAT]		= REG_FIELD(0x0B, 2, 2),
	[F_SDP_STAT]		= REG_FIELD(0x0B, 1, 1),
	[F_VSYS_STAT]		= REG_FIELD(0x0B, 0, 0),
	/* REG0C */
	[F_WD_FAULT]		= REG_FIELD(0x0C, 7, 7),
	[F_BOOST_FAULT]		= REG_FIELD(0x0C, 6, 6),
	[F_CHG_FAULT]		= REG_FIELD(0x0C, 4, 5),
	[F_BAT_FAULT]		= REG_FIELD(0x0C, 3, 3),
	[F_NTC_FAULT]		= REG_FIELD(0x0C, 0, 2),
	/* REG0D */
	[F_FORCE_VINDPM]	= REG_FIELD(0x0D, 7, 7),
	[F_VINDPM]		= REG_FIELD(0x0D, 0, 6),
	/* REG0E */
	[F_THERM_STAT]		= REG_FIELD(0x0E, 7, 7),
	[F_BATV]		= REG_FIELD(0x0E, 0, 6),
	/* REG0F */
	[F_SYSV]		= REG_FIELD(0x0F, 0, 6),
	/* REG10 */
	[F_TSPCT]		= REG_FIELD(0x10, 0, 6),
	/* REG11 */
	[F_VBUS_GD]		= REG_FIELD(0x11, 7, 7),
	[F_VBUSV]		= REG_FIELD(0x11, 0, 6),
	/* REG12 */
	[F_ICHGR]		= REG_FIELD(0x12, 0, 6),
	/* REG13 */
	[F_VDPM_STAT]		= REG_FIELD(0x13, 7, 7),
	[F_IDPM_STAT]		= REG_FIELD(0x13, 6, 6),
	[F_IDPM_LIM]		= REG_FIELD(0x13, 0, 5),
	/* REG14 */
	[F_REG_RST]		= REG_FIELD(0x14, 7, 7),
	[F_ICO_OPTIMIZED]	= REG_FIELD(0x14, 6, 6),
	[F_PN]			= REG_FIELD(0x14, 3, 5),
	[F_TS_PROFILE]		= REG_FIELD(0x14, 2, 2),
	[F_DEV_REV]		= REG_FIELD(0x14, 0, 1)
};

/*
 * Most of the val -> idx conversions can be computed, given the minimum,
 * maximum and the step between values. For the rest of conversions, we use
 * lookup tables.
 */
enum bq25895m_table_ids {
	/* range tables */
	TBL_ICHG,
	TBL_ITERM,
	TBL_IPRECHG,
	TBL_VREG,
	TBL_BATCMP,
	TBL_VCLAMP,
	TBL_BOOSTV,
	TBL_SYSVMIN,

	/* lookup tables */
	TBL_TREG,
	TBL_BOOSTI,
};

/* Thermal Regulation Threshold lookup table, in degrees Celsius */
static const u32 bq25895m_treg_tbl[] = { 60, 80, 100, 120 };

#define BQ25895M_TREG_TBL_SIZE		ARRAY_SIZE(bq25895m_treg_tbl)

/* Boost mode current limit lookup table, in uA */
static const u32 bq25895m_boosti_tbl[] = {
	500000, 700000, 1100000, 1300000, 1600000, 1800000, 2100000, 2400000
};

#define BQ25895M_BOOSTI_TBL_SIZE		ARRAY_SIZE(bq25895m_boosti_tbl)

struct bq25895m_range {
	u32 min;
	u32 max;
	u32 step;
};

struct bq25895m_lookup {
	const u32 *tbl;
	u32 size;
};

static const union {
	struct bq25895m_range  rt;
	struct bq25895m_lookup lt;
} bq25895m_tables[] = {
	/* range tables */
	[TBL_ICHG] =	{ .rt = {0,	  5056000, 64000} },	 /* uA */
	[TBL_ITERM] =	{ .rt = {64000,   1024000, 64000} },	 /* uA */
	[TBL_VREG] =	{ .rt = {3840000, 4608000, 16000} },	 /* uV */
	[TBL_BATCMP] =	{ .rt = {0,	  140,     20} },	 /* mOhm */
	[TBL_VCLAMP] =	{ .rt = {0,	  224000,  32000} },	 /* uV */
	[TBL_BOOSTV] =	{ .rt = {4550000, 5510000, 64000} },	 /* uV */
	[TBL_SYSVMIN] = { .rt = {3000000, 3700000, 100000} },	 /* uV */

	/* lookup tables */
	[TBL_TREG] =	{ .lt = {bq25895m_treg_tbl, BQ25895M_TREG_TBL_SIZE} },
	[TBL_BOOSTI] =	{ .lt = {bq25895m_boosti_tbl, BQ25895M_BOOSTI_TBL_SIZE} }
};

static int bq25895m_field_read(struct bq25895m_device *bq,
			      enum bq25895m_fields field_id)
{
	int ret;
	int val;

	ret = regmap_field_read(bq->rmap_fields[field_id], &val);
	if (ret < 0)
		return ret;

	return val;
}

static int bq25895m_field_write(struct bq25895m_device *bq,
			       enum bq25895m_fields field_id, u8 val)
{
	return regmap_field_write(bq->rmap_fields[field_id], val);
}

static u8 bq25895m_find_idx(u32 value, enum bq25895m_table_ids id)
{
	u8 idx;

	if (id >= TBL_TREG) {
		const u32 *tbl = bq25895m_tables[id].lt.tbl;
		u32 tbl_size = bq25895m_tables[id].lt.size;

		for (idx = 1; idx < tbl_size && tbl[idx] <= value; idx++)
			;
	} else {
		const struct bq25895m_range *rtbl = &bq25895m_tables[id].rt;
		u8 rtbl_size;

		rtbl_size = (rtbl->max - rtbl->min) / rtbl->step + 1;

		for (idx = 1;
		     idx < rtbl_size && (idx * rtbl->step + rtbl->min <= value);
		     idx++)
			;
	}

	return idx - 1;
}

static u32 bq25895m_find_val(u8 idx, enum bq25895m_table_ids id)
{
	const struct bq25895m_range *rtbl;

	/* lookup table? */
	if (id >= TBL_TREG)
		return bq25895m_tables[id].lt.tbl[idx];

	/* range table */
	rtbl = &bq25895m_tables[id].rt;

	return (rtbl->min + idx * rtbl->step);
}

enum bq25895m_status {
	STATUS_NOT_CHARGING,
	STATUS_PRE_CHARGING,
	STATUS_FAST_CHARGING,
	STATUS_TERMINATION_DONE,
};

enum bq25895m_chrg_fault {
	CHRG_FAULT_NORMAL,
	CHRG_FAULT_INPUT,
	CHRG_FAULT_THERMAL_SHUTDOWN,
	CHRG_FAULT_TIMER_EXPIRED,
};

static int bq25895m_get_voltage(struct power_supply *psy)
{
	int ret;
	struct bq25895m_device *bq = power_supply_get_drvdata(psy);

	ret = bq25895m_field_read(bq, F_BATV); /* read measured value */
	if (ret < 0)
		return ret;
	/* converted_val = 2.304V + ADC_val * 20mV (table 10.3.15) */
	return 2304000 + ret * 20000;
}

static int bq25895m_get_capacity(struct power_supply *psy)
{
	int ret, voltage;
	struct bq25895m_device *bq = power_supply_get_drvdata(psy);

	ret = bq25895m_field_read(bq, F_BATV); /* read measured value */
	if (ret < 0)
		return ret;
	/* converted_val = 2.304V + ADC_val * 20mV (table 10.3.15) */
	voltage = 2304000 + ret * 20000;

	if (voltage>3900000) {
		ret = 100;
	}
	else if(voltage > 3800000) {
		ret = 75;
	}
	else if(voltage > 3700000) {
		ret = 50;
	}
	else if(voltage > 3600000) {
		ret = 25;
	}
	else {
		ret = 10;
	}

	return ret;
}
static int bq25895m_power_supply_get_property(struct power_supply *psy,
					     enum power_supply_property psp,
					     union power_supply_propval *val)
{
	int ret;
	struct bq25895m_device *bq = power_supply_get_drvdata(psy);
	struct bq25895m_state state;

	mutex_lock(&bq->lock);
	state = bq->state;
	mutex_unlock(&bq->lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		/*
		if (!state.online)
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else if (state.chrg_status == STATUS_NOT_CHARGING)
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else if (state.chrg_status == STATUS_PRE_CHARGING ||
			 state.chrg_status == STATUS_FAST_CHARGING)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else if (state.chrg_status == STATUS_TERMINATION_DONE)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
			*/
		if (!state.online)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		break;

	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = BQ25895M_MANUFACTURER;
		break;

	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = state.online;
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		if (!state.chrg_fault && !state.bat_fault && !state.boost_fault)
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		else if (state.bat_fault)
			val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		else if (state.chrg_fault == CHRG_FAULT_TIMER_EXPIRED)
			val->intval = POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE;
		else if (state.chrg_fault == CHRG_FAULT_THERMAL_SHUTDOWN)
			val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
		else
			val->intval = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		break;

	case POWER_SUPPLY_PROP_CURRENT_NOW: /* battery current */
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = bq25895m_field_read(bq, F_ICHGR); /* read measured value */
		if (ret < 0)
			return ret;
		/* converted_val = ADC_val * 50mA (table 10.3.19) */
		val->intval = ret * 50000;
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
		val->intval = bq25895m_tables[TBL_ICHG].rt.max;
		break;

	case POWER_SUPPLY_PROP_VOLTAGE_NOW: /* battery voltage */
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		val->intval = bq25895m_get_voltage(psy);
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
		val->intval = bq25895m_tables[TBL_VREG].rt.max;
		break;

	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
		val->intval = bq25895m_find_val(bq->init_data.iterm, TBL_ITERM);
		break;
/* add the battery feature */
	case POWER_SUPPLY_PROP_PRESENT:  /* charging status */
		val->intval = 1;  /* always present */
		break;
	case POWER_SUPPLY_PROP_CAPACITY: /* remaining capacity */
		val->intval = bq25895m_get_capacity(psy);
		break;
	case POWER_SUPPLY_PROP_TEMP:  /* battery temperature */
		val->intval = 100;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY: /* battery technology */
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:  /* ???? */
		val->intval = 10;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL: /* ???? */
		val->intval = 100;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int bq25895m_get_chip_state(struct bq25895m_device *bq,
				  struct bq25895m_state *state)
{
	int i, ret;

	struct {
		enum bq25895m_fields id;
		u8 *data;
	} state_fields[] = {
		{F_CHG_STAT,	&state->chrg_status},
		{F_PG_STAT,	&state->online},
		{F_VSYS_STAT,	&state->vsys_status},
		{F_BOOST_FAULT, &state->boost_fault},
		{F_BAT_FAULT,	&state->bat_fault},
		{F_CHG_FAULT,	&state->chrg_fault}
	};

	for (i = 0; i < ARRAY_SIZE(state_fields); i++) {
		ret = bq25895m_field_read(bq, state_fields[i].id);
		if (ret < 0)
			return ret;

		*state_fields[i].data = ret;
	}

	dev_dbg(bq->dev, "S:CHG/PG/VSYS=%d/%d/%d, F:CHG/BOOST/BAT=%d/%d/%d\n",
		state->chrg_status, state->online, state->vsys_status,
		state->chrg_fault, state->boost_fault, state->bat_fault);

	return 0;
}

static bool bq25895m_state_changed(struct bq25895m_device *bq,
				  struct bq25895m_state *new_state)
{
	struct bq25895m_state old_state;

	mutex_lock(&bq->lock);
	old_state = bq->state;
	mutex_unlock(&bq->lock);

	return (old_state.chrg_status != new_state->chrg_status ||
		old_state.chrg_fault != new_state->chrg_fault	||
		old_state.online != new_state->online		||
		old_state.bat_fault != new_state->bat_fault	||
		old_state.boost_fault != new_state->boost_fault ||
		old_state.vsys_status != new_state->vsys_status);
}

static void bq25895m_handle_state_change(struct bq25895m_device *bq,
					struct bq25895m_state *new_state)
{
	int ret;
	struct bq25895m_state old_state;

	mutex_lock(&bq->lock);
	old_state = bq->state;
	mutex_unlock(&bq->lock);

	if (!new_state->online) {			     /* power removed */
		/* disable ADC */
		ret = bq25895m_field_write(bq, F_CONV_START, 0);
		if (ret < 0)
			goto error;
	} else if (!old_state.online) {			    /* power inserted */
		/* enable ADC, to have control of charge current/voltage */
		ret = bq25895m_field_write(bq, F_CONV_START, 1);
		if (ret < 0)
			goto error;
	}

	return;

error:
	dev_err(bq->dev, "Error communicating with the chip.\n");
}

static irqreturn_t bq25895m_irq_handler_thread(int irq, void *private)
{
	struct bq25895m_device *bq = private;
	int ret;
	struct bq25895m_state state;

	ret = bq25895m_get_chip_state(bq, &state);
	if (ret < 0)
		goto handled;

	if (!bq25895m_state_changed(bq, &state))
		goto handled;

	bq25895m_handle_state_change(bq, &state);

	mutex_lock(&bq->lock);
	bq->state = state;
	mutex_unlock(&bq->lock);

	power_supply_changed(bq->charger);

handled:
	return IRQ_HANDLED;
}

static int bq25895m_chip_reset(struct bq25895m_device *bq)
{
	int ret;
	int rst_check_counter = 10;

	ret = bq25895m_field_write(bq, F_REG_RST, 1);
	if (ret < 0)
		return ret;

	do {
		ret = bq25895m_field_read(bq, F_REG_RST);
		if (ret < 0)
			return ret;

		usleep_range(5, 10);
	} while (ret == 1 && --rst_check_counter);

	if (!rst_check_counter)
		return -ETIMEDOUT;

	return 0;
}

static int bq25895m_hw_init(struct bq25895m_device *bq)
{
	int ret;
	int i;
	struct bq25895m_state state;

	const struct {
		enum bq25895m_fields id;
		u32 value;
	} init_data[] = {
		{F_ICHG,	 bq->init_data.ichg},
		{F_VREG,	 bq->init_data.vreg},
		{F_ITERM,	 bq->init_data.iterm},
		{F_IPRECHG,	 bq->init_data.iprechg},
		{F_SYSVMIN,	 bq->init_data.sysvmin},
		{F_BOOSTV,	 bq->init_data.boostv},
		{F_BOOSTI,	 bq->init_data.boosti},
		{F_BOOSTF,	 bq->init_data.boostf},
		{F_EN_ILIM,	 bq->init_data.ilim_en},
		{F_TREG,	 bq->init_data.treg}
	};

	ret = bq25895m_chip_reset(bq);
	if (ret < 0)
		return ret;

	/* disable watchdog */
	ret = bq25895m_field_write(bq, F_WD, 0);
	if (ret < 0)
		return ret;

	/* initialize currents/voltages and other parameters */
	for (i = 0; i < ARRAY_SIZE(init_data); i++) {
		ret = bq25895m_field_write(bq, init_data[i].id,
					  init_data[i].value);
		if (ret < 0)
			return ret;
	}

	/* Configure ADC for continuous conversions. This does not enable it. */
	ret = bq25895m_field_write(bq, F_CONV_RATE, 1);
	if (ret < 0)
		return ret;

	ret = bq25895m_get_chip_state(bq, &state);
	if (ret < 0)
		return ret;

	mutex_lock(&bq->lock);
	bq->state = state;
	mutex_unlock(&bq->lock);

	return 0;
}

static enum power_supply_property bq25895m_power_supply_props[] = {
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH, /* battery health */
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT,
	/* add the battery feature */
	POWER_SUPPLY_PROP_PRESENT, /* charging status */
	POWER_SUPPLY_PROP_CAPACITY, /* remaining capacity */
	POWER_SUPPLY_PROP_VOLTAGE_NOW, /* battery voltage */
	POWER_SUPPLY_PROP_TEMP,  /* battery temperature */
	POWER_SUPPLY_PROP_TECHNOLOGY, /* battery technology */
	POWER_SUPPLY_PROP_CURRENT_NOW, /* battery current */
	POWER_SUPPLY_PROP_CYCLE_COUNT,  /* ???? */
	POWER_SUPPLY_PROP_CHARGE_FULL, /* ???? */


};

static char *bq25895m_charger_supplied_to[] = {
	"main-battery",
};

static const struct power_supply_desc bq25895m_power_supply_desc = {
	.name = "bq25895m-charger",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = bq25895m_power_supply_props,
	.num_properties = ARRAY_SIZE(bq25895m_power_supply_props),
	.get_property = bq25895m_power_supply_get_property,
};

static int bq25895m_power_supply_init(struct bq25895m_device *bq)
{
	struct power_supply_config psy_cfg = { .drv_data = bq, };

	psy_cfg.supplied_to = bq25895m_charger_supplied_to;
	psy_cfg.num_supplicants = ARRAY_SIZE(bq25895m_charger_supplied_to);

	bq->charger = power_supply_register(bq->dev, &bq25895m_power_supply_desc,
					    &psy_cfg);

	return PTR_ERR_OR_ZERO(bq->charger);
}

static void bq25895m_usb_work(struct work_struct *data)
{
	int ret;
	struct bq25895m_device *bq =
			container_of(data, struct bq25895m_device, usb_work);

	switch (bq->usb_event) {
	case USB_EVENT_ID:
		/* Enable boost mode */
		ret = bq25895m_field_write(bq, F_OTG_CFG, 1);
		if (ret < 0)
			goto error;
		break;

	case USB_EVENT_NONE:
		/* Disable boost mode */
		ret = bq25895m_field_write(bq, F_OTG_CFG, 0);
		if (ret < 0)
			goto error;

		power_supply_changed(bq->charger);
		break;
	}

	return;

error:
	dev_err(bq->dev, "Error switching to boost/charger mode.\n");
}

static int bq25895m_usb_notifier(struct notifier_block *nb, unsigned long val,
				void *priv)
{
	struct bq25895m_device *bq =
			container_of(nb, struct bq25895m_device, usb_nb);

	bq->usb_event = val;
	queue_work(system_power_efficient_wq, &bq->usb_work);

	return NOTIFY_OK;
}

static int bq25895m_irq_probe(struct bq25895m_device *bq)
{
	struct gpio_desc *irq;

	irq = devm_gpiod_get_index(bq->dev, BQ25895M_IRQ_PIN, 0, GPIOD_IN);
	if (IS_ERR(irq)) {
		dev_err(bq->dev, "Could not probe irq pin.\n");
		return PTR_ERR(irq);
	}

	return gpiod_to_irq(irq);
}

static int bq25895m_fw_read_u32_props(struct bq25895m_device *bq)
{
	int ret;
	u32 property;
	int i;
	struct bq25895m_init_data *init = &bq->init_data;
	struct {
		char *name;
		bool optional;
		enum bq25895m_table_ids tbl_id;
		u8 *conv_data; /* holds converted value from given property */
	} props[] = {
		/* required properties */
		{"ti,charge-current", false, TBL_ICHG, &init->ichg},
		{"ti,battery-regulation-voltage", false, TBL_VREG, &init->vreg},
		{"ti,termination-current", false, TBL_ITERM, &init->iterm},
		{"ti,precharge-current", false, TBL_ITERM, &init->iprechg},
		{"ti,minimum-sys-voltage", false, TBL_SYSVMIN, &init->sysvmin},
		{"ti,boost-voltage", false, TBL_BOOSTV, &init->boostv},
		{"ti,boost-max-current", false, TBL_BOOSTI, &init->boosti},

		/* optional properties */
		{"ti,thermal-regulation-threshold", true, TBL_TREG, &init->treg}
	};

	/* initialize data for optional properties */
	init->treg = 3; /* 120 degrees Celsius */

	for (i = 0; i < ARRAY_SIZE(props); i++) {
		ret = device_property_read_u32(bq->dev, props[i].name,
					       &property);
		if (ret < 0) {
			if (props[i].optional)
				continue;

			return ret;
		}

		*props[i].conv_data = bq25895m_find_idx(property,
						       props[i].tbl_id);
	}

	return 0;
}

static int bq25895m_fw_probe(struct bq25895m_device *bq)
{
	int ret;
	struct bq25895m_init_data *init = &bq->init_data;

	ret = bq25895m_fw_read_u32_props(bq);
	if (ret < 0)
		return ret;

	init->ilim_en = device_property_read_bool(bq->dev, "ti,use-ilim-pin");
	init->boostf = device_property_read_bool(bq->dev, "ti,boost-low-freq");

	return 0;
}

static int bq25895m_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct device *dev = &client->dev;
	struct bq25895m_device *bq;
	int ret;
	int i;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(dev, "No support for SMBUS_BYTE_DATA\n");
		return -ENODEV;
	}

	bq = devm_kzalloc(dev, sizeof(*bq), GFP_KERNEL);
	if (!bq)
		return -ENOMEM;

	bq->client = client;
	bq->dev = dev;

	mutex_init(&bq->lock);

	bq->rmap = devm_regmap_init_i2c(client, &bq25895m_regmap_config);
	if (IS_ERR(bq->rmap)) {
		dev_err(dev, "failed to allocate register map\n");
		return PTR_ERR(bq->rmap);
	}

	for (i = 0; i < ARRAY_SIZE(bq25895m_reg_fields); i++) {
		const struct reg_field *reg_fields = bq25895m_reg_fields;

		bq->rmap_fields[i] = devm_regmap_field_alloc(dev, bq->rmap,
							     reg_fields[i]);
		if (IS_ERR(bq->rmap_fields[i])) {
			dev_err(dev, "cannot allocate regmap field\n");
			return PTR_ERR(bq->rmap_fields[i]);
		}
	}

	i2c_set_clientdata(client, bq);

	bq->chip_id = bq25895m_field_read(bq, F_PN);
	if (bq->chip_id < 0) {
		dev_err(dev, "Cannot read chip ID.\n");
		return bq->chip_id;
	}

	if (bq->chip_id != BQ25895M_ID) {
		dev_err(dev, "Chip with ID=%d, not supported!\n", bq->chip_id);
		return -ENODEV;
	}
	else {
		dev_info(dev, "%s: Chip with ID=%d, \n", __FUNCTION__, bq->chip_id);
	}

	if (!dev->platform_data) {
		ret = bq25895m_fw_probe(bq);
		if (ret < 0) {
			dev_err(dev, "Cannot read device properties.\n");
			return ret;
		}
	} else {
		return -ENODEV;
	}

	ret = bq25895m_hw_init(bq);
	if (ret < 0) {
		dev_err(dev, "Cannot initialize the chip.\n");
		return ret;
	}

	if (client->irq <= 0)
		client->irq = bq25895m_irq_probe(bq);

	if (client->irq < 0) {
		dev_err(dev, "No irq resource found.\n");
		return client->irq;
	}

	/* OTG reporting */
	bq->usb_phy = devm_usb_get_phy(dev, USB_PHY_TYPE_USB2);
	if (!IS_ERR_OR_NULL(bq->usb_phy)) {
		INIT_WORK(&bq->usb_work, bq25895m_usb_work);
		bq->usb_nb.notifier_call = bq25895m_usb_notifier;
		usb_register_notifier(bq->usb_phy, &bq->usb_nb);
	}

	ret = devm_request_threaded_irq(dev, client->irq, NULL,
					bq25895m_irq_handler_thread,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					BQ25895M_IRQ_PIN, bq);
	if (ret)
		goto irq_fail;

	ret = bq25895m_power_supply_init(bq);
	if (ret < 0) {
		dev_err(dev, "Failed to register power supply\n");
		goto irq_fail;
	}

	return 0;

irq_fail:
	if (!IS_ERR_OR_NULL(bq->usb_phy))
		usb_unregister_notifier(bq->usb_phy, &bq->usb_nb);

	return ret;
}

static int bq25895m_remove(struct i2c_client *client)
{
	struct bq25895m_device *bq = i2c_get_clientdata(client);

	power_supply_unregister(bq->charger);

	if (!IS_ERR_OR_NULL(bq->usb_phy))
		usb_unregister_notifier(bq->usb_phy, &bq->usb_nb);

	/* reset all registers to default values */
	bq25895m_chip_reset(bq);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int bq25895m_suspend(struct device *dev)
{
	struct bq25895m_device *bq = dev_get_drvdata(dev);

	/*
	 * If charger is removed, while in suspend, make sure ADC is diabled
	 * since it consumes slightly more power.
	 */
	return bq25895m_field_write(bq, F_CONV_START, 0);
}

static int bq25895m_resume(struct device *dev)
{
	int ret;
	struct bq25895m_state state;
	struct bq25895m_device *bq = dev_get_drvdata(dev);

	ret = bq25895m_get_chip_state(bq, &state);
	if (ret < 0)
		return ret;

	mutex_lock(&bq->lock);
	bq->state = state;
	mutex_unlock(&bq->lock);

	/* Re-enable ADC only if charger is plugged in. */
	if (state.online) {
		ret = bq25895m_field_write(bq, F_CONV_START, 1);
		if (ret < 0)
			return ret;
	}

	/* signal userspace, maybe state changed while suspended */
	power_supply_changed(bq->charger);

	return 0;
}
#endif

static const struct dev_pm_ops bq25895m_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(bq25895m_suspend, bq25895m_resume)
};

static const struct i2c_device_id bq25895m_i2c_ids[] = {
	{ "bq25895m", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, bq25895m_i2c_ids);

static const struct of_device_id bq25895m_of_match[] = {
	{ .compatible = "ti,bq25895m", },
	{ },
};
MODULE_DEVICE_TABLE(of, bq25895m_of_match);

static const struct acpi_device_id bq25895m_acpi_match[] = {
	{"BQ25895M", 0},
	{},
};
MODULE_DEVICE_TABLE(acpi, bq25895m_acpi_match);

static struct i2c_driver bq25895m_driver = {
	.driver = {
		.name = "bq25895m-charger",
		.of_match_table = of_match_ptr(bq25895m_of_match),
		.acpi_match_table = ACPI_PTR(bq25895m_acpi_match),
		.pm = &bq25895m_pm,
	},
	.probe = bq25895m_probe,
	.remove = bq25895m_remove,
	.id_table = bq25895m_i2c_ids,
};
module_i2c_driver(bq25895m_driver);

MODULE_AUTHOR("Allan Park <allan.park@nexell.co.kr>");
MODULE_DESCRIPTION("bq25895m charger driver");
MODULE_LICENSE("GPL");
