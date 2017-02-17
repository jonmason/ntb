/*
 * axp228-regu.h  --  PMIC driver for the X-Powers AXP228
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

#ifndef _LINUX_AXP_REGU_H_
#define _LINUX_AXP_REGU_H_

/* AXP22 Regulator Registers */
#define AXP22_LDO1 AXP22_STATUS
#define AXP22_LDO5 AXP22_DLDO1OUT_VOL
#define AXP22_LDO6 AXP22_DLDO2OUT_VOL
#define AXP22_LDO7 AXP22_DLDO3OUT_VOL
#define AXP22_LDO8 AXP22_DLDO4OUT_VOL
#define AXP22_LDO9 AXP22_ELDO1OUT_VOL
#define AXP22_LDO10 AXP22_ELDO2OUT_VOL
#define AXP22_LDO11 AXP22_ELDO3OUT_VOL
#define AXP22_LDO12 AXP22_DC5LDOOUT_VOL
#define AXP22_DCDC1 AXP22_DC1OUT_VOL
#define AXP22_DCDC2 AXP22_DC2OUT_VOL
#define AXP22_DCDC3 AXP22_DC3OUT_VOL
#define AXP22_DCDC4 AXP22_DC4OUT_VOL
#define AXP22_DCDC5 AXP22_DC5OUT_VOL

#define AXP22_LDOIO0 AXP22_GPIO0LDOOUT_VOL
#define AXP22_LDOIO1 AXP22_GPIO1LDOOUT_VOL
#define AXP22_LDO2 AXP22_ALDO1OUT_VOL
#define AXP22_LDO3 AXP22_ALDO2OUT_VOL
#define AXP22_LDO4 AXP22_ALDO3OUT_VOL

#define AXP22_LDO1EN AXP22_STATUS
#define AXP22_LDO2EN AXP22_LDO_DC_EN1
#define AXP22_LDO3EN AXP22_LDO_DC_EN1
#define AXP22_LDO4EN AXP22_LDO_DC_EN3
#define AXP22_LDO5EN AXP22_LDO_DC_EN2
#define AXP22_LDO6EN AXP22_LDO_DC_EN2
#define AXP22_LDO7EN AXP22_LDO_DC_EN2
#define AXP22_LDO8EN AXP22_LDO_DC_EN2
#define AXP22_LDO9EN AXP22_LDO_DC_EN2
#define AXP22_LDO10EN AXP22_LDO_DC_EN2
#define AXP22_LDO11EN AXP22_LDO_DC_EN2
#define AXP22_LDO12EN AXP22_LDO_DC_EN1
#define AXP22_DCDC1EN AXP22_LDO_DC_EN1
#define AXP22_DCDC2EN AXP22_LDO_DC_EN1
#define AXP22_DCDC3EN AXP22_LDO_DC_EN1
#define AXP22_DCDC4EN AXP22_LDO_DC_EN1
#define AXP22_DCDC5EN AXP22_LDO_DC_EN1
#define AXP22_LDOIO0EN AXP22_GPIO0_CTL
#define AXP22_LDOIO1EN AXP22_GPIO1_CTL
#define AXP22_DC1SW1EN AXP22_LDO_DC_EN2

#define AXP22_BUCKMODE AXP22_DCDC_MODESET
#define AXP22_BUCKFREQ AXP22_DCDC_FREQSET

#define AXP22_PWREN_CONTROL1 AXP22_PWREN_CTL1
#define AXP_DCDC1_BIT (7)
#define AXP_DCDC2_BIT (6)
#define AXP_DCDC3_BIT (5)
#define AXP_DCDC4_BIT (4)
#define AXP_DCDC5_BIT (3)
#define AXP_ALDO1_BIT (2)
#define AXP_ALDO2_BIT (1)
#define AXP_ALDO3_BIT (0)

#define AXP22_PWREN_CONTROL2 AXP22_PWREN_CTL2
#define AXP_DLDO1_BIT (7)
#define AXP_DLDO2_BIT (6)
#define AXP_DLDO3_BIT (5)
#define AXP_DLDO4_BIT (4)
#define AXP_ELDO1_BIT (3)
#define AXP_ELDO2_BIT (2)
#define AXP_ELDO3_BIT (1)
#define AXP_DC5LDO_BIT (0)

#define AXP_LDO(_pmic, _id, min, max, step, vreg, shift,\
			nbits, ereg, ebit, vrc_ramp)	\
{	\
	.desc = {	\
		.name = #_pmic"_LDO" #_id,	\
		.type = REGULATOR_VOLTAGE,	\
		.id = _pmic##_ID_LDO##_id,	\
		.n_voltages = (step) ? ((max - min) / step + 1) : 1,\
		.owner = THIS_MODULE,	\
	},	\
	.min_uV = (min) * 1000,	\
	.max_uV = (max) * 1000,	\
	.step_uV = (step) * 1000,	\
	.vol_reg = _pmic##_##vreg,	\
	.vol_shift = (shift),	\
	.vol_nbits = (nbits),	\
	.enable_reg = _pmic##_##ereg,	\
	.enable_bit = (ebit),	\
	.vrc_ramp_delay = (vrc_ramp),	\
}

#define AXP_BUCK(_pmic, _id, min, max, step, vreg, shift,\
			nbits, ereg, ebit, vrc_ramp)	\
{	\
	.desc = {	\
		.name = #_pmic"_BUCK" #_id,	\
		.type = REGULATOR_VOLTAGE,	\
		.id = _pmic##_ID_BUCK##_id,	\
		.n_voltages = (step) ? ((max - min) / step + 1) : 1,\
		.owner	= THIS_MODULE,	\
	},	\
	.min_uV = (min) * 1000,	\
	.max_uV = (max) * 1000,	\
	.step_uV = (step) * 1000,	\
	.vol_reg = _pmic##_##vreg,	\
	.vol_shift = (shift),	\
	.vol_nbits = (nbits),	\
	.enable_reg = _pmic##_##ereg,	\
	.enable_bit = (ebit),	\
	.vrc_ramp_delay = (vrc_ramp),	\
}

#define AXP_DCDC(_pmic, _id, min, max, step, vreg, shift,\
			nbits, ereg, ebit, vrc_ramp)	\
{	\
	.desc = {	\
		.name = #_pmic"_DCDC" #_id,	\
		.type = REGULATOR_VOLTAGE,	\
		.id = _pmic##_ID_DCDC##_id,	\
		.n_voltages = (step) ? ((max - min) / step + 1) : 1,\
		.owner = THIS_MODULE,	\
	},	\
	.min_uV = (min) * 1000,	\
	.max_uV = (max) * 1000,	\
	.step_uV = (step) * 1000,	\
	.vol_reg = _pmic##_##vreg,	\
	.vol_shift = (shift),	\
	.vol_nbits = (nbits),	\
	.enable_reg = _pmic##_##ereg,	\
	.enable_bit = (ebit),	\
	.vrc_ramp_delay = (vrc_ramp),	\
}

#define AXP_SW(_pmic, _id, min, max, step, vreg, shift,\
			nbits, ereg, ebit, vrc_ramp)	\
{	\
	.desc = {	\
		.name = #_pmic"_SW" #_id,	\
		.type = REGULATOR_VOLTAGE,	\
		.id = _pmic##_ID_SW##_id,	\
		.n_voltages = (step) ? ((max - min) / step + 1) : 1,\
		.owner = THIS_MODULE,	\
	},	\
	.min_uV = (min) * 1000,	\
	.max_uV = (max) * 1000,	\
	.step_uV = (step) * 1000,	\
	.vol_reg = _pmic##_##vreg,	\
	.vol_shift = (shift),	\
	.vol_nbits = (nbits),	\
	.enable_reg = _pmic##_##ereg,	\
	.enable_bit = (ebit),	\
	.vrc_ramp_delay = (vrc_ramp),	\
}

#define AXP_REGU_ATTR(_name)	\
{	\
	.attr = { .name = #_name, .mode = 0644 },\
	.show = _name##_show,	\
	.store = _name##_store,	\
}

struct axp_regulator_info {
	struct regulator_dev *rdev;
	struct regulator_desc desc;

	int min_uV;
	int max_uV;
	int step_uV;
	int vol_reg;
	int vol_shift;
	int vol_nbits;
	int enable_reg;
	int enable_bit;
	u16 vrc_ramp_delay;
	u8 vout_reg_cache;
};

#endif
