/*
 * axp228-mfd.h  --  PMIC driver for the X-Powers AXP228
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

#ifndef __LINUX_AXP_MFD_H_
#define __LINUX_AXP_MFD_H_

/*For AXP22*/
#define AXP22 (22)
#define AXP22_STATUS (0x00)
#define AXP22_MODE_CHGSTATUS (0x01)
#define AXP22_IC_TYPE (0x03)
#define AXP22_BUFFER1 (0x04)
#define AXP22_BUFFER2 (0x05)
#define AXP22_BUFFER3 (0x06)
#define AXP22_BUFFER4 (0x07)
#define AXP22_BUFFER5 (0x08)
#define AXP22_BUFFER6 (0x09)
#define AXP22_BUFFER7 (0x0A)
#define AXP22_BUFFER8 (0x0B)
#define AXP22_BUFFER9 (0x0C)
#define AXP22_BUFFERA (0x0D)
#define AXP22_BUFFERB (0x0E)
#define AXP22_BUFFERC (0x0F)
#define AXP22_IPS_SET (0x30)
#define AXP22_VOFF_SET (0x31)
#define AXP22_OFF_CTL (0x32)
#define AXP22_CHARGE1 (0x33)
#define AXP22_CHARGE2 (0x34)
#define AXP22_CHARGE3 (0x35)
#define AXP22_POK_SET (0x36)
#define AXP22_INTEN1 (0x40)
#define AXP22_INTEN2 (0x41)
#define AXP22_INTEN3 (0x42)
#define AXP22_INTEN4 (0x43)
#define AXP22_INTEN5 (0x44)
#define AXP22_INTSTS1 (0x48)
#define AXP22_INTSTS2 (0x49)
#define AXP22_INTSTS3 (0x4A)
#define AXP22_INTSTS4 (0x4B)
#define AXP22_INTSTS5 (0x4C)

#define AXP22_LDO_DC_EN1 (0X10)
#define AXP22_LDO_DC_EN2 (0X12)
#define AXP22_LDO_DC_EN3 (0X13)
#define AXP22_DLDO1OUT_VOL (0x15)
#define AXP22_DLDO2OUT_VOL (0x16)
#define AXP22_DLDO3OUT_VOL (0x17)
#define AXP22_DLDO4OUT_VOL (0x18)
#define AXP22_ELDO1OUT_VOL (0x19)
#define AXP22_ELDO2OUT_VOL (0x1A)
#define AXP22_ELDO3OUT_VOL (0x1B)
#define AXP22_DC5LDOOUT_VOL (0x1C)
#define AXP22_DC1OUT_VOL (0x21)
#define AXP22_DC2OUT_VOL (0x22)
#define AXP22_DC3OUT_VOL (0x23)
#define AXP22_DC4OUT_VOL (0x24)
#define AXP22_DC5OUT_VOL (0x25)
#define AXP22_GPIO0LDOOUT_VOL (0x91)
#define AXP22_GPIO1LDOOUT_VOL (0x93)
#define AXP22_ALDO1OUT_VOL (0x28)
#define AXP22_ALDO2OUT_VOL (0x29)
#define AXP22_ALDO3OUT_VOL (0x2A)

#define AXP22_DCDC_MODESET (0x80)
#define AXP22_DCDC_FREQSET (0x37)
#define AXP22_ADC_EN (0x82)
#define AXP22_PWREN_CTL1 (0x8C)
#define AXP22_PWREN_CTL2 (0x8D)
#define AXP22_HOTOVER_CTL (0x8F)

#define AXP22_GPIO0_CTL (0x90)
#define AXP22_GPIO1_CTL (0x92)
#define AXP22_GPIO01_SIGNAL (0x94)
#define AXP22_BAT_CHGCOULOMB3 (0xB0)
#define AXP22_BAT_CHGCOULOMB2 (0xB1)
#define AXP22_BAT_CHGCOULOMB1 (0xB2)
#define AXP22_BAT_CHGCOULOMB0 (0xB3)
#define AXP22_BAT_DISCHGCOULOMB3 (0xB4)
#define AXP22_BAT_DISCHGCOULOMB2 (0xB5)
#define AXP22_BAT_DISCHGCOULOMB1 (0xB6)
#define AXP22_BAT_DISCHGCOULOMB0 (0xB7)
#define AXP22_COULOMB_CTL (0xB8)

/* bit definitions for AXP events ,irq event */
/*  AXP22  */
#define AXP22_IRQ_USBLO (1 << 1)
#define AXP22_IRQ_USBRE (1 << 2)
#define AXP22_IRQ_USBIN (1 << 3)
#define AXP22_IRQ_USBOV (1 << 4)
#define AXP22_IRQ_ACRE (1 << 5)
#define AXP22_IRQ_ACIN (1 << 6)
#define AXP22_IRQ_ACOV (1 << 7)
#define AXP22_IRQ_TEMLO (1 << 8)
#define AXP22_IRQ_TEMOV (1 << 9)
#define AXP22_IRQ_CHAOV (1 << 10)
#define AXP22_IRQ_CHAST (1 << 11)
#define AXP22_IRQ_BATATOU (1 << 12)
#define AXP22_IRQ_BATATIN (1 << 13)
#define AXP22_IRQ_BATRE (1 << 14)
#define AXP22_IRQ_BATIN (1 << 15)
#define AXP22_IRQ_POKLO (1 << 16)
#define AXP22_IRQ_POKSH (1 << 17)
#define AXP22_IRQ_CHACURLO (1 << 22)
#define AXP22_IRQ_ICTEMOV (1 << 23)
#define AXP22_IRQ_EXTLOWARN2 (1 << 24)
#define AXP22_IRQ_EXTLOWARN1 (1 << 25)
#define AXP22_IRQ_GPIO0TG ((uint64_t)1 << 32)
#define AXP22_IRQ_GPIO1TG ((uint64_t)1 << 33)
#define AXP22_IRQ_GPIO2TG ((uint64_t)1 << 34)
#define AXP22_IRQ_GPIO3TG ((uint64_t)1 << 35)

#define AXP22_IRQ_PEKFE ((uint64_t)1 << 37)
#define AXP22_IRQ_PEKRE ((uint64_t)1 << 38)
#define AXP22_IRQ_TIMER ((uint64_t)1 << 39)

/* Status Query Interface */
/*  AXP22  */
#define AXP22_STATUS_SOURCE (1 << 0)
#define AXP22_STATUS_ACUSBSH (1 << 1)
#define AXP22_STATUS_BATCURDIR (1 << 2)
#define AXP22_STATUS_USBLAVHO (1 << 3)
#define AXP22_STATUS_USBVA (1 << 4)
#define AXP22_STATUS_USBEN (1 << 5)
#define AXP22_STATUS_ACVA (1 << 6)
#define AXP22_STATUS_ACEN (1 << 7)

#define AXP22_STATUS_BATINACT (1 << 11)

#define AXP22_STATUS_BATEN (1 << 13)
#define AXP22_STATUS_INCHAR (1 << 14)
#define AXP22_STATUS_ICTEMOV (1 << 15)

#define AXP22_DCDC1_MIN 1600000
#define AXP22_DCDC1_MAX 3400000
#define AXP22_DCDC2_MIN 600000
#define AXP22_DCDC2_MAX 1540000
#define AXP22_DCDC3_MIN 600000
#define AXP22_DCDC3_MAX 1860000
#define AXP22_DCDC4_MIN 600000
#define AXP22_DCDC4_MAX 1540000
#define AXP22_DCDC5_MIN 1000000
#define AXP22_DCDC5_MAX 2550000

#define AXP22_LDO1_MIN 3000000
#define AXP22_LDO1_MAX 3000000
#define AXP22_LDO2_MIN 700000
#define AXP22_LDO2_MAX 3300000
#define AXP22_LDO3_MIN 700000
#define AXP22_LDO3_MAX 3300000
#define AXP22_LDO4_MIN 700000
#define AXP22_LDO4_MAX 3300000
#define AXP22_LDO5_MIN 700000
#define AXP22_LDO5_MAX 3300000
#define AXP22_LDO6_MIN 700000
#define AXP22_LDO6_MAX 3300000
#define AXP22_LDO7_MIN 700000
#define AXP22_LDO7_MAX 3300000
#define AXP22_LDO8_MIN 700000
#define AXP22_LDO8_MAX 3300000
#define AXP22_LDO9_MIN 700000
#define AXP22_LDO9_MAX 3300000
#define AXP22_LDO10_MIN 700000
#define AXP22_LDO10_MAX 3300000
#define AXP22_LDO11_MIN 700000
#define AXP22_LDO11_MAX 3300000
#define AXP22_LDO12_MIN 700000
#define AXP22_LDO12_MAX 1400000

#define AXP22_LDOIO0_MIN 700000
#define AXP22_LDOIO0_MAX 3300000
#define AXP22_LDOIO1_MIN 700000
#define AXP22_LDOIO1_MAX 3300000

/* Unified sub device IDs for AXP */
/* LDO0 For RTCLDO ,LDO1-3 for ALDO,LDO*/
enum {
	AXP22_ID_LDO1,  /* RTCLDO */
	AXP22_ID_LDO2,  /* ALDO1 */
	AXP22_ID_LDO3,  /* ALDO2 */
	AXP22_ID_LDO4,  /* ALDO3 */
	AXP22_ID_LDO5,  /* DLDO1 */
	AXP22_ID_LDO6,  /* DLDO2 */
	AXP22_ID_LDO7,  /* DLDO3 */
	AXP22_ID_LDO8,  /* DLDO4 */
	AXP22_ID_LDO9,  /* ELDO1 */
	AXP22_ID_LDO10, /* ELDO2 */
	AXP22_ID_LDO11, /* ELDO3 */
	AXP22_ID_LDO12, /* DC5LDO */
	AXP22_ID_DCDC1,
	AXP22_ID_DCDC2,
	AXP22_ID_DCDC3,
	AXP22_ID_DCDC4,
	AXP22_ID_DCDC5,
	AXP22_ID_LDOIO0,
	AXP22_ID_LDOIO1,
	AXP22_ID_SUPPLY,
	AXP22_ID_GPIO,
};

#define AXP_MFD_ATTR(_name)			\
{									\
	.attr = { .name = #_name, .mode = 0644 },	\
	.show = _name##_show,			\
	.store = _name##_store,			\
}

/* AXP battery charger data */
struct power_supply_info;

struct axp_supply_init_data {
	/* battery parameters */
	struct power_supply_info *battery_info;

	/* current and voltage to use for battery charging */
	unsigned int chgcur;
	unsigned int chgvol;
	unsigned int chgend;
	/*charger control*/
	bool chgen;
	bool limit_on;
	/*charger time */
	int chgpretime;
	int chgcsttime;

	/*adc sample time */
	unsigned int sample_time;

	/* platform callbacks for battery low and critical IRQs */
	void (*battery_low)(void);
	void (*battery_critical)(void);
};

struct axp_funcdev_info {
	struct device_node *reg_node;
	int id;
	const char *name;
	void *platform_data;
};

struct axp_platform_data {
	int id;
	int num_regl_devs;
	int num_sply_devs;
	int num_gpio_devs;
	int gpio_base;
	struct axp_funcdev_info *regl_devs;
	struct axp_funcdev_info *sply_devs;
	struct axp_funcdev_info *gpio_devs;
};

struct axp_mfd_chip {
	struct device *dev;
	struct i2c_client *client;
	struct mutex lock;
	struct work_struct irq_work;
	struct blocking_notifier_head notifier_list;

	struct axp_mfd_chip_ops *ops;
	struct axp_platform_data *pdata;

	int irq;
	int type;
	uint64_t irqs_enabled;
};

struct axp_mfd_chip_ops {
	int (*init_chip)(struct axp_mfd_chip *);
	int (*enable_irqs)(struct axp_mfd_chip *, uint64_t irqs);
	int (*disable_irqs)(struct axp_mfd_chip *, uint64_t irqs);
	int (*read_irqs)(struct axp_mfd_chip *, uint64_t *irqs);
};

extern struct device *axp_get_dev(void);
extern int axp_register_notifier(struct device *dev, struct notifier_block *nb,
				 uint64_t irqs);
extern int axp_unregister_notifier(struct device *dev,
				   struct notifier_block *nb, uint64_t irqs);

/* NOTE: the functions below are not intended for use outside
 * of the AXP sub-device drivers
 */
extern int axp_write(struct device *dev, int reg, uint8_t val);
extern int axp_writes(struct device *dev, int reg, int len, uint8_t *val);
extern int axp_read(struct device *dev, int reg, uint8_t *val);
extern int axp_reads(struct device *dev, int reg, int len, uint8_t *val);
extern int axp_update(struct device *dev, int reg, uint8_t val, uint8_t mask);
extern int axp_set_bits(struct device *dev, int reg, uint8_t bit_mask);
extern int axp_clr_bits(struct device *dev, int reg, uint8_t bit_mask);
extern struct i2c_client *axp;
#endif /* __LINUX_PMIC_AXP_H */
