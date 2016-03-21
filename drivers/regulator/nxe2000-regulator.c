/*
 * nxe2000-regulator.c  --  PMIC driver for the nxe2000
 *
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongshin, Park <pjsin865@nexell.co.kr>
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

/*#define DEBUG			1*/
/*#define VERBOSE_DEBUG		1*/
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/nxe2000-regulator.h>
#include <linux/mfd/nxe2000.h>

struct nxe2000_regulator {
	const char *name;
	int id;
	int sleep_id;

	/* Regulator register address.*/
	u8 reg_en_reg;
	u8 en_bit;
	u8 reg_disc_reg;
	u8 disc_bit;
	u8 vout_reg;
	u8 vout_mask;
	u8 vout_reg_cache;
	u8 sleep_slot_reg;
	u8 sleep_volt_reg;
	u8 eco_reg;
	u8 eco_bit;
	u8 eco_slp_reg;
	u8 eco_slp_bit;

	/* chip constraints on regulator behavior */
	int min_uV;
	int max_uV;
	int step_uV;
	int nsteps;

	/* regulator specific turn-on delay */
	u16 delay;
	u16 en_delay;
	u16 cmd_delay;

	struct regulator_desc desc;
	struct regulator_dev *rdev;
	struct device *dev;
};

static unsigned int nxe2000_suspend_status;

static inline struct device *to_nxe2000_dev(struct regulator_dev *rdev)
{
	return rdev_get_dev(rdev)->parent->parent;
}

static int nxe2000_regulator_enable_time(struct regulator_dev *rdev)
{
	struct nxe2000_regulator *ri = rdev_get_drvdata(rdev);

	return ri->en_delay;
}

static int nxe2000_reg_is_enabled(struct regulator_dev *rdev)
{
	struct nxe2000_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_nxe2000_dev(rdev);
	uint8_t control;
	int ret;

	ret = nxe2000_read(parent, ri->reg_en_reg, &control);
	if (ret < 0) {
		dev_err(&rdev->dev, "Error in reading the control register\n");
		return ret;
	}
	return (((control >> ri->en_bit) & 1) == 1);
}

static int nxe2000_reg_enable(struct regulator_dev *rdev)
{
	struct nxe2000_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_nxe2000_dev(rdev);
	int ret;

	ret = nxe2000_set_bits(parent, ri->reg_en_reg, (1 << ri->en_bit));
	if (ret < 0) {
		dev_err(&rdev->dev, "Error in updating the STATE register\n");
		return ret;
	}
	udelay(ri->delay);
	return ret;
}

static int nxe2000_reg_disable(struct regulator_dev *rdev)
{
	struct nxe2000_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_nxe2000_dev(rdev);
	int ret;

	ret = nxe2000_clr_bits(parent, ri->reg_en_reg, (1 << ri->en_bit));
	if (ret < 0)
		dev_err(&rdev->dev, "Error in updating the STATE register\n");

	return ret;
}

static int nxe2000_list_voltage(struct regulator_dev *rdev, unsigned index)
{
	struct nxe2000_regulator *ri = rdev_get_drvdata(rdev);

	return ri->min_uV + (ri->step_uV * index);
}

static int __nxe2000_set_voltage(struct device *parent,
				 struct nxe2000_regulator *ri, int min_uV,
				 int max_uV, unsigned *selector, int sleep_mode)
{
	int vsel;
	int ret;
	uint8_t vout_val;

	if ((min_uV < ri->min_uV) || (max_uV > ri->max_uV))
		return -EDOM;

	vsel = (min_uV - ri->min_uV + ri->step_uV - 1)/ri->step_uV;
	if (vsel > ri->nsteps)
		return -EDOM;

	if (selector)
		*selector = vsel;

	vout_val =
	    (ri->vout_reg_cache & ~ri->vout_mask) | (vsel & ri->vout_mask);
	if (sleep_mode)
		ret = nxe2000_write(parent, ri->sleep_volt_reg, vout_val);
	else
		ret = nxe2000_write(parent, ri->vout_reg, vout_val);
	if (ret < 0)
		dev_err(ri->dev, "Error in writing the Voltage register\n");
	else
		ri->vout_reg_cache = vout_val;

#ifdef CONFIG_PM_DBGOUT
	vout_val = 0x00;
	nxe2000_read(parent, ri->vout_reg, &vout_val);
	if (ri->vout_reg_cache != vout_val)
		pr_err("## %s() Data is different! set:0x%02x, read:0x%02x\n",
		       __func__, ri->vout_reg_cache, vout_val);
#endif

	return ret;
}

static int nxe2000_set_voltage_time_sel(struct regulator_dev *rdev,
					unsigned int old_sel,
					unsigned int new_sel)
{
	struct nxe2000_regulator *ri = rdev_get_drvdata(rdev);

	if (old_sel < new_sel)
		return ((new_sel - old_sel) * ri->delay) + ri->cmd_delay;

	return 0;
}

static int nxe2000_set_voltage_sel(struct regulator_dev *rdev,
				   unsigned selector)
{
	struct nxe2000_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_nxe2000_dev(rdev);

	int uV;

	uV = ri->min_uV + (ri->step_uV * selector);

	if (nxe2000_suspend_status)
		return -EBUSY;

	return __nxe2000_set_voltage(parent, ri, uV, uV, NULL, 0);
}

static int nxe2000_set_voltage(struct regulator_dev *rdev, int min_uV,
			       int max_uV, unsigned *selector)
{
	struct nxe2000_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_nxe2000_dev(rdev);

	if (nxe2000_suspend_status)
		return -EBUSY;

	return __nxe2000_set_voltage(parent, ri, min_uV, max_uV, selector, 0);
}

static int nxe2000_get_voltage_sel(struct regulator_dev *rdev)
{
	struct nxe2000_regulator *ri = rdev_get_drvdata(rdev);
	uint8_t vsel;

	vsel = ri->vout_reg_cache & ri->vout_mask;
	return vsel;
}

static int nxe2000_get_voltage(struct regulator_dev *rdev)
{
	struct nxe2000_regulator *ri = rdev_get_drvdata(rdev);
	uint8_t vsel;

	vsel = ri->vout_reg_cache & ri->vout_mask;
	return ri->min_uV + vsel * ri->step_uV;
}

int nxe2000_regulator_enable_eco_mode(struct regulator_dev *rdev)
{
	struct nxe2000_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_nxe2000_dev(rdev);
	int ret;

	ret = nxe2000_set_bits(parent, ri->eco_reg, (1 << ri->eco_bit));
	if (ret < 0)
		dev_err(&rdev->dev, "Error Enable LDO eco mode\n");

	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_regulator_enable_eco_mode);

int nxe2000_regulator_disable_eco_mode(struct regulator_dev *rdev)
{
	struct nxe2000_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_nxe2000_dev(rdev);
	int ret;

	ret = nxe2000_clr_bits(parent, ri->eco_reg, (1 << ri->eco_bit));
	if (ret < 0)
		dev_err(&rdev->dev, "Error Disable LDO eco mode\n");

	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_regulator_disable_eco_mode);

int nxe2000_regulator_enable_eco_slp_mode(struct regulator_dev *rdev)
{
	struct nxe2000_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_nxe2000_dev(rdev);
	int ret;

	ret = nxe2000_set_bits(parent, ri->eco_slp_reg, (1 << ri->eco_slp_bit));
	if (ret < 0)
		dev_err(&rdev->dev,
		"Error Enable LDO eco mode in d during sleep\n");

	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_regulator_enable_eco_slp_mode);

int nxe2000_regulator_disable_eco_slp_mode(struct regulator_dev *rdev)
{
	struct nxe2000_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_nxe2000_dev(rdev);
	int ret;

	ret = nxe2000_clr_bits(parent, ri->eco_slp_reg, (1 << ri->eco_slp_bit));
	if (ret < 0)
		dev_err(&rdev->dev,
		"Error Enable LDO eco mode in d during sleep\n");

	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_regulator_disable_eco_slp_mode);

static struct regulator_ops nxe2000_dcdc1_ops = {
	.list_voltage = nxe2000_list_voltage,
	.set_voltage_sel = nxe2000_set_voltage_sel,
	.set_voltage_time_sel = nxe2000_set_voltage_time_sel,
	.get_voltage_sel = nxe2000_get_voltage_sel,
	.enable = nxe2000_reg_enable,
	.disable = nxe2000_reg_disable,
	.is_enabled = nxe2000_reg_is_enabled,
	.enable_time = nxe2000_regulator_enable_time,
};

static struct regulator_ops nxe2000_ops = {
	.list_voltage = nxe2000_list_voltage,
	.set_voltage = nxe2000_set_voltage,
	.get_voltage = nxe2000_get_voltage,
	.enable = nxe2000_reg_enable,
	.disable = nxe2000_reg_disable,
	.is_enabled = nxe2000_reg_is_enabled,
	.enable_time = nxe2000_regulator_enable_time,
};

#define NXE2000_REG(_name, _id, _en_reg, _en_bit, _disc_reg, _disc_bit,	\
	_vout_reg, _vout_mask, _slp_volt_reg, _slp_slot_reg, _min_mv,	\
	_max_mv, _step_uV, _nsteps, _ops, _delay, _en_delay, _eco_reg,	\
	_eco_bit, _eco_slp_reg, _eco_slp_bit)	\
{							\
	.name = #_name,			\
	.id = NXE2000_ID_##_id,		\
	.sleep_id = NXE2000_DS_##_id,		\
	.reg_en_reg = _en_reg,			\
	.en_bit = _en_bit,			\
	.reg_disc_reg = _disc_reg,			\
	.disc_bit = _disc_bit,			\
	.vout_reg = _vout_reg,			\
	.vout_mask = _vout_mask,			\
	.sleep_volt_reg = _slp_volt_reg,		\
	.sleep_slot_reg = _slp_slot_reg,		\
	.min_uV = _min_mv * 1000,		\
	.max_uV = _max_mv * 1000,		\
	.step_uV = _step_uV,			\
	.nsteps = _nsteps,			\
	.delay = _delay,			\
	.en_delay = _en_delay,			\
	.cmd_delay = 50,				\
	.eco_reg = _eco_reg,			\
	.eco_bit = _eco_bit,			\
	.eco_slp_reg = _eco_slp_reg,			\
	.eco_slp_bit = _eco_slp_bit,			\
	.desc = {					\
		.name = nxe2000_rails(_id),		\
		.id = NXE2000_ID_##_id,		\
		.n_voltages = _nsteps,			\
		.ops = &_ops,			\
		.type = REGULATOR_VOLTAGE,		\
		.owner = THIS_MODULE,			\
	},						\
}

/* DCDC Enable Rising Time
*  DCDC1 = 300us/1.3V
*  DCDC2 = 300us/1.2V
*  DCDC3 = 700us/3.3V
*  DCDC4 = 400us/1.5V
*  DCDC5 = 400us/1.5V
*/
static struct nxe2000_regulator nxe2000_regulators[] = {
	NXE2000_REG(dcdc1, DC1, 0x2C, 0, 0x2C, 1, 0x36, 0xFF, 0x3B, 0x16,
			600, 3500, 12500, 0xE8, nxe2000_dcdc1_ops, 2, 300,
			0x00, 0, 0x00, 0),

	NXE2000_REG(dcdc2, DC2, 0x2E, 0, 0x2E, 1, 0x37, 0xFF, 0x3C, 0x17,
			600, 3500, 12500, 0xE8, nxe2000_ops, 2, 300,
			0x00, 0, 0x00, 0),

	NXE2000_REG(dcdc3, DC3, 0x30, 0, 0x30, 1, 0x38, 0xFF, 0x3D, 0x18,
			600, 3500, 12500, 0xE8, nxe2000_ops, 2, 700,
			0x00, 0, 0x00, 0),

	NXE2000_REG(dcdc4, DC4, 0x32, 0, 0x32, 1, 0x39, 0xFF, 0x3E, 0x19,
			600, 3500, 12500, 0xE8, nxe2000_ops, 2, 400,
			0x00, 0, 0x00, 0),

	NXE2000_REG(dcdc5, DC5, 0x34, 0, 0x34, 1, 0x3A, 0xFF, 0x3F, 0x1A,
			600, 3500, 12500, 0xE8, nxe2000_ops, 2, 400,
			0x00, 0, 0x00, 0),

	NXE2000_REG(ldo1, LDO1, 0x44, 0, 0x46, 0, 0x4C, 0x7F, 0x58, 0x1B,
			900, 3500, 25000, 0x68, nxe2000_ops, 0, 500,
			0x48, 0, 0x4A, 0),

	NXE2000_REG(ldo2, LDO2, 0x44, 1, 0x46, 1, 0x4D, 0x7F, 0x59, 0x1C,
			900, 3500, 25000, 0x68, nxe2000_ops, 0, 500,
			0x48, 1, 0x4A, 1),

	NXE2000_REG(ldo3, LDO3, 0x44, 2, 0x46, 2, 0x4E, 0x7F, 0x5A, 0x1D,
			900, 3500, 25000, 0x68, nxe2000_ops, 0, 500,
			0x48, 2, 0x4A, 2),

	NXE2000_REG(ldo4, LDO4, 0x44, 3, 0x46, 3, 0x4F, 0x7F, 0x5B, 0x1E,
			900, 3500, 25000, 0x68, nxe2000_ops, 0, 500,
			0x48, 3, 0x4A, 3),

	NXE2000_REG(ldo5, LDO5, 0x44, 4, 0x46, 4, 0x50, 0x7F, 0x5C, 0x1F,
			600, 3500, 25000, 0x74, nxe2000_ops, 0, 500,
			0x48, 4, 0x4A, 4),

	NXE2000_REG(ldo6, LDO6, 0x44, 5, 0x46, 5, 0x51, 0x7F, 0x5D, 0x20,
			600, 3500, 25000, 0x74, nxe2000_ops, 0, 500,
			0x48, 5, 0x4A, 5),

	NXE2000_REG(ldo7, LDO7, 0x44, 6, 0x46, 6, 0x52, 0x7F, 0x5E, 0x21,
			900, 3500, 25000, 0x68, nxe2000_ops, 0, 500,
			0x00, 0, 0x00, 0),

	NXE2000_REG(ldo8, LDO8, 0x44, 7, 0x46, 7, 0x53, 0x7F, 0x5F, 0x22,
			900, 3500, 25000, 0x68, nxe2000_ops, 0, 500,
			0x00, 0, 0x00, 0),

	NXE2000_REG(ldo9, LDO9, 0x45, 0, 0x47, 0, 0x54, 0x7F, 0x60, 0x23,
			900, 3500, 25000, 0x68, nxe2000_ops, 0, 500,
			0x00, 0, 0x00, 0),

	NXE2000_REG(ldo10, LDO10, 0x45, 1, 0x47, 1, 0x55, 0x7F, 0x61, 0x24,
			900, 3500, 25000, 0x68, nxe2000_ops, 0, 500,
			0x00, 0, 0x00, 0),

	NXE2000_REG(ldortc1, LDORTC1, 0x45, 4, 0x00, 0, 0x56, 0x7F, 0x00, 0x00,
			1700, 3500, 25000, 0x48, nxe2000_ops, 0, 500,
			0x00, 0, 0x00, 0),

	NXE2000_REG(ldortc2, LDORTC2, 0x45, 5, 0x00, 0, 0x57, 0x7F, 0x00, 0x00,
			900, 3500, 25000, 0x68, nxe2000_ops, 0, 500,
			0x00, 0, 0x00, 0),
};
static inline struct nxe2000_regulator *find_regulator_info(int id)
{
	struct nxe2000_regulator *ri;
	int i;

	for (i = 0; i < ARRAY_SIZE(nxe2000_regulators); i++) {
		ri = &nxe2000_regulators[i];
		if (ri->desc.id == id)
			return ri;
	}
	return NULL;
}

static int
nxe2000_regulator_preinit(struct device *parent, struct nxe2000_regulator *ri,
			  struct nxe2000_regulator_platform_data *nxe2000_pdata)
{
	int ret = 0;

	if ((nxe2000_pdata->init_uV > -1) && nxe2000_pdata->set_init_uV) {
		ret = __nxe2000_set_voltage(parent, ri, nxe2000_pdata->init_uV,
					    nxe2000_pdata->init_uV, 0, 0);
		if (ret < 0) {
			dev_err(ri->dev,
				"Not able to initialize voltage %d for rail %d err %d\n",
				nxe2000_pdata->init_uV, ri->desc.id, ret);
			return ret;
		}
	}

	if (nxe2000_pdata->sleep_slots == -1) {
		ret = __nxe2000_set_voltage(parent, ri, nxe2000_pdata->init_uV,
					    nxe2000_pdata->init_uV, 0, 1);
		if (ret < 0) {
			dev_err(ri->dev,
				"Not able to sleep voltage %d for rail %d err %d\n",
				nxe2000_pdata->init_uV, ri->desc.id, ret);
			return ret;
		}
	}

	if (nxe2000_pdata->init_enable)
		ret =
		    nxe2000_set_bits(parent, ri->reg_en_reg, (1 << ri->en_bit));
	else
		ret =
		    nxe2000_clr_bits(parent, ri->reg_en_reg, (1 << ri->en_bit));
	if (ret < 0)
		dev_err(ri->dev, "Not able to %s rail %d err %d\n",
			(nxe2000_pdata->init_enable) ? "enable" : "disable",
			ri->desc.id, ret);

	ret = nxe2000_update(parent, ri->sleep_slot_reg,
			     nxe2000_pdata->sleep_slots, 0xF);

	if (ret < 0)
		dev_err(ri->dev, "Not able to 0x%02X rail %d err %d\n",
			nxe2000_pdata->sleep_slots, ri->desc.id, ret);

	return ret;
}

static inline int nxe2000_cache_regulator_register(struct device *parent,
						   struct nxe2000_regulator *ri)
{
	ri->vout_reg_cache = 0;
	return nxe2000_read(parent, ri->vout_reg, &ri->vout_reg_cache);
}

#ifdef CONFIG_OF
static int nxe2000_regulator_dt_parse_pdata(struct platform_device *pdev,
					    struct nxe2000_platform_data *pdata)
{
	struct nxe2000 *iodev = dev_get_drvdata(pdev->dev.parent);
	struct device_node *pmic_np, *regulators_np, *reg_np;
	struct regulator_init_data *regu_initdata;
	struct nxe2000_regulator_platform_data *rdata;
	unsigned int i;
	u32 val;

	pmic_np = of_node_get(iodev->dev->of_node);
	if (!pmic_np) {
		dev_err(&pdev->dev, "could not find pmic sub-node\n");
		return -ENODEV;
	}

	regulators_np = of_find_node_by_name(pmic_np, "regulators");
	if (!regulators_np) {
		dev_err(&pdev->dev, "could not find regulators sub-node\n");
		return -EINVAL;
	}

	/* count the number of regulators to be supported in pmic */
	pdata->num_regulators = of_get_child_count(regulators_np);

	rdata = devm_kzalloc(&pdev->dev, sizeof(*rdata) * pdata->num_regulators,
			     GFP_KERNEL);
	if (!rdata) {
		of_node_put(regulators_np);
		dev_err(&pdev->dev,
			"could not allocate memory for regulator data\n");
		return -ENOMEM;
	}

	pdata->regulators = rdata;

	for_each_child_of_node(regulators_np, reg_np) {
		for (i = 0; i < ARRAY_SIZE(nxe2000_regulators); i++)
			if (!of_node_cmp(reg_np->name,
					 nxe2000_regulators[i].name))
				break;
		if (i == ARRAY_SIZE(nxe2000_regulators)) {
			dev_warn(&pdev->dev,
				 "Error : don't know how to configure regulator %s\n",
				 reg_np->name);
			continue;
		}
		rdata->id = i;
		regu_initdata = of_get_regulator_init_data(
		    &pdev->dev, reg_np, &nxe2000_regulators[i].desc);
		rdata->reg_node = reg_np;

		if (!of_property_read_u32(reg_np, "nx,id", &val))
			rdata->id = val;
		else
			dev_err(&pdev->dev, "%s() Error : id\n", __func__);

		regu_initdata->consumer_supplies = devm_kzalloc(
		    &pdev->dev, sizeof(struct regulator_consumer_supply),
		    GFP_KERNEL);

		if (of_property_read_string(
			reg_np, "regulator-name",
			&regu_initdata->consumer_supplies->supply))
			dev_err(&pdev->dev,
				"%s() Error : regulator-name\n",
				__func__);

		regu_initdata->num_consumer_supplies = 1;
		regu_initdata->constraints.valid_ops_mask =
		    REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS;

		if (!of_property_read_u32(reg_np, "nx,init_enable", &val))
			rdata->init_enable = val;
		else
			dev_err(&pdev->dev, "%s() Error : init_enable\n",
				__func__);

		if (!of_property_read_u32(reg_np, "nx,init_uV", &val))
			rdata->init_uV = (int)val;
		else
			dev_err(&pdev->dev, "%s() Error : init_uV\n",
				__func__);

		if (!of_property_read_u32(reg_np, "nx,set_init_uV", &val))
			rdata->set_init_uV = (int)val;
		else
			dev_err(&pdev->dev, "%s() Error : set_init_uV\n",
				__func__);

		if (!of_property_read_u32(reg_np, "nx,sleep_slots", &val))
			rdata->sleep_slots = (int)val;
		else
			dev_err(&pdev->dev, "%s() Error : sleep_slots\n",
				__func__);

		rdata->initdata = regu_initdata;
		rdata++;
	}
	of_node_put(regulators_np);

	return 0;
}
#else
static int nxe2000_regulator_dt_parse_pdata(struct platform_device *pdev,
					    struct nxe2000_platform_data *pdata)
{
	return 0;
}
#endif /* CONFIG_OF */

static int nxe2000_regulator_probe(struct platform_device *pdev)
{
	struct nxe2000 *iodev = dev_get_drvdata(pdev->dev.parent);
	struct nxe2000_platform_data *pdata = iodev->pdata;

	struct nxe2000_regulator *nxe2000 = NULL;
	struct nxe2000_regulator_platform_data *tps_pdata;

	struct regulator_config config = {};
	int i, ret = -1, err;

	if (iodev->dev->of_node) {
		err = nxe2000_regulator_dt_parse_pdata(pdev, pdata);
		if (err)
			return err;
	} else {
		dev_err(&pdev->dev, "%s() Error: No parse pdata\n",
			__func__);
		goto err_out;
	}

	nxe2000_suspend_status = 0;
	tps_pdata = pdata->regulators;

	for (i = 0; i < pdata->num_regulators; i++) {
		nxe2000 = find_regulator_info(tps_pdata->id);
		if (nxe2000 == NULL) {
			dev_err(&pdev->dev,
				"%s() Error: invalid regulator ID specified\n",
				__func__);
			return -EINVAL;
		}
		nxe2000->dev = &pdev->dev;
		/* nxe2000->desc.supply_name = tps_pdata->supply_name; */

		err =
		    nxe2000_cache_regulator_register(pdev->dev.parent, nxe2000);
		if (err) {
			dev_err(&pdev->dev,
				"%s() Error: Fail in caching register\n",
				__func__);
			return err;
		}

		err = nxe2000_regulator_preinit(pdev->dev.parent, nxe2000,
						tps_pdata);
		if (err) {
			dev_err(&pdev->dev,
				"%s() Error: Fail in pre-initialisation\n",
				__func__);
			return err;
		}

		/* Register the regulators */
		config.dev = &pdev->dev;
		config.init_data = tps_pdata->initdata;
		config.driver_data = nxe2000;
		config.of_node = tps_pdata->reg_node;

		nxe2000_regulators[i].rdev = devm_regulator_register(&pdev->dev,
			&nxe2000->desc,
			&config);

		if (IS_ERR(nxe2000_regulators[i].rdev)) {
			dev_err(&pdev->dev,
				"%s() Error: regulator register failed\n",
				__func__);
			ret = PTR_ERR(nxe2000_regulators[i].rdev);
		}
		tps_pdata++;
	}

	return 0;

err_out:
	dev_err(&pdev->dev, "%s() Error\n", __func__);
	return ret;
}

static int nxe2000_regulator_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
static int nxe2000_regulator_suspend(struct device *dev)
{
	/* struct nxe2000_regulator *info = dev_get_drvdata(dev); */

	nxe2000_suspend_status = 1;

	return 0;
}

static int nxe2000_regulator_resume(struct device *dev)
{
	/* struct nxe2000_regulator *info = dev_get_drvdata(dev); */

	nxe2000_suspend_status = 0;

	return 0;
}

static const struct dev_pm_ops nxe2000_regulator_pm_ops = {
	.suspend = nxe2000_regulator_suspend,
	.resume = nxe2000_regulator_resume,
};
#endif

static struct platform_driver nxe2000_regulator_driver = {
	.driver = {
		.name = "nxe2000-regulator",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &nxe2000_regulator_pm_ops,
#endif
	},
	.probe = nxe2000_regulator_probe,
	.remove = nxe2000_regulator_remove,
};

static int __init nxe2000_regulator_init(void)
{
	return platform_driver_register(&nxe2000_regulator_driver);
}
subsys_initcall(nxe2000_regulator_init);

static void __exit nxe2000_regulator_exit(void)
{
	platform_driver_unregister(&nxe2000_regulator_driver);
}
module_exit(nxe2000_regulator_exit);

MODULE_DESCRIPTION("NEXELL NXE2000 regulator driver");
MODULE_ALIAS("platform:nxe2000-regulator");
MODULE_LICENSE("GPL");
