/*
 * nxe2000.h  --  PMIC driver for the nxe2000
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

#ifndef __LINUX_MFD_NXE2000_H
#define __LINUX_MFD_NXE2000_H

#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/i2c.h>

/* Maximum number of main interrupts */
#define MAX_INTERRUPT_MASKS 13
#define MAX_MAIN_INTERRUPT 7
#define MAX_GPEDGE_REG 2

/* Power control */
#define NXE2000_REG_WATCHDOG 0x0B
#define NXE2000_REG_SLPCNT 0x0E
#define NXE2000_REG_REPCNT 0x0F

/* DCDC */
#define NXE2000_REG_DC1CTL2 0x2D
#define NXE2000_REG_DC2CTL2 0x2F

/* Interrupt */
#define NXE2000_REG_INTPOL 0x9C
#define NXE2000_REG_INTEN 0x9D  /* Enable & Mask */
#define NXE2000_REG_INTMON 0x9E /* Status */

/* Charge */
#define NXE2000_REG_TIMSET 0xB9

/* GPIO register base address */
#define NXE2000_GPIO_GPEDGE1 0x92
#define NXE2000_GPIO_GPEDGE2 0x93
#define NXE2000_REG_BANKSEL 0xFF

/* Interrupt enable register */
#define NXE2000_INT_EN_SYS 0x12
#define NXE2000_INT_EN_DCDC 0x40
#define NXE2000_INT_EN_RTC 0xAE
#define NXE2000_INT_EN_ADC1 0x88
#define NXE2000_INT_EN_ADC2 0x89
#define NXE2000_INT_EN_ADC3 0x8A
#define NXE2000_INT_EN_GPIO 0x94
#define NXE2000_INT_EN_GPIO2 0x94 /* dummy */
#define NXE2000_INT_MSK_CHGCTR 0xBE
#define NXE2000_INT_MSK_CHGSTS1 0xBF
#define NXE2000_INT_MSK_CHGSTS2 0xC0
#define NXE2000_INT_MSK_CHGERR 0xC1
#define NXE2000_INT_MSK_CHGEXTIF 0xD1

/* interrupt clearing registers */
#define NXE2000_INT_IR_SYS 0x13
#define NXE2000_INT_IR_DCDC 0x41
#define NXE2000_INT_IR_RTC 0xAF
#define NXE2000_INT_IR_ADCL 0x8C
#define NXE2000_INT_IR_ADCH 0x8D
#define NXE2000_INT_IR_ADCEND 0x8E
#define NXE2000_INT_IR_GPIOR 0x95
#define NXE2000_INT_IR_GPIOF 0x96
#define NXE2000_INT_IR_CHGCTR 0xC2
#define NXE2000_INT_IR_CHGSTS1 0xC3
#define NXE2000_INT_IR_CHGSTS2 0xC4
#define NXE2000_INT_IR_CHGERR 0xC5
#define NXE2000_INT_IR_CHGEXTIF 0xD2

/*
 * register bit position
 */

/* DC1CTL2, DC2CTL2, DC3CTL2 */
#define NXE2000_POS_DCxCTL2_DCxOSC (6)
#define NXE2000_POS_DCxCTL2_DCxSR (4)
#define NXE2000_POS_DCxCTL2_DCxLIM (1)
#define NXE2000_POS_DCxCTL2_DCxLIMSDEN (0)

/* IRQ definitions */
enum nxe2000_irq_source {
	NXE2000_IRQGRP_SYSTEM_INT = 0,
	NXE2000_IRQGRP_DCDC_INT,
	NXE2000_IRQGRP_ADC_INT1,
	NXE2000_IRQGRP_ADC_INT2,
	NXE2000_IRQGRP_ADC_INT3,
	NXE2000_IRQGRP_GPIO_INT,
	NXE2000_IRQGRP_CHG_INT1, /* Charge Control interrupt */
	NXE2000_IRQGRP_CHG_INT2, /* Charge Status1 interrupt */
	NXE2000_IRQGRP_CHG_INT3, /* Charge Status2 interrupt */
	NXE2000_IRQGRP_CHG_INT4, /* Charge Error interrupt */
	NXE2000_IRQGRP_FG_INT,
	NXE2000_IRQ_GROUP_NR,
};

enum nxe2000_irq {
	NXE2000_IRQ_DCDC_DC1LIM,
	NXE2000_IRQ_DCDC_DC2LIM,
	NXE2000_IRQ_DCDC_DC3LIM,
	NXE2000_IRQ_DCDC_DC4LIM,
	NXE2000_IRQ_DCDC_DC5LIM,
	NXE2000_IRQ_ADC1_AIN0L,
	NXE2000_IRQ_ADC1_AIN1L,
	NXE2000_IRQ_ADC1_VTHML,
	NXE2000_IRQ_ADC1_VSYSL,
	NXE2000_IRQ_ADC1_VUSBL,
	NXE2000_IRQ_ADC1_VADPL, /* 10 */
	NXE2000_IRQ_ADC1_VBATL,
	NXE2000_IRQ_ADC1_ILIML,
	NXE2000_IRQ_ADC2_AIN0H,
	NXE2000_IRQ_ADC2_AIN1H,
	NXE2000_IRQ_ADC2_VTHMH,
	NXE2000_IRQ_ADC2_VSYSH,
	NXE2000_IRQ_ADC2_VUSBH,
	NXE2000_IRQ_ADC2_VADPH,
	NXE2000_IRQ_ADC2_VBATH,
	NXE2000_IRQ_ADC2_ILIMH, /* 20 */
	NXE2000_IRQ_ADC3_END,
	NXE2000_IRQ_GPIO_GP00,
	NXE2000_IRQ_GPIO_GP01,
	NXE2000_IRQ_GPIO_GP02,
	NXE2000_IRQ_GPIO_GP03,
	NXE2000_IRQ_GPIO_GP04,
	NXE2000_IRQ_CHGCTRL_RDSTATESHIFT,
	NXE2000_IRQ_CHGCTRL_WVUSBS,
	NXE2000_IRQ_CHGCTRL_WVADPS,
	NXE2000_IRQ_CHGCTRL_VUSBLVS, /* 30 */
	NXE2000_IRQ_CHGCTRL_VADPLVS,
	NXE2000_IRQ_CHGCTRL_VUSBDETS,
	NXE2000_IRQ_CHGCTRL_VADPDETS,
	NXE2000_IRQ_CHGSTS_BTEMPJTA4,
	NXE2000_IRQ_CHGSTS_BTEMPJTA3,
	NXE2000_IRQ_CHGSTS_BTEMPJTA2,
	NXE2000_IRQ_CHGSTS_BTEMPJTA1,
	NXE2000_IRQ_CHGSTS_SLPMODE,
	NXE2000_IRQ_CHGSTS_BATOPEN,
	NXE2000_IRQ_CHGSTS_CHGCMP, /* 40 */
	NXE2000_IRQ_CHGSTS_ONCHG,
	NXE2000_IRQ_CHGSTS_OSCMDET,
	NXE2000_IRQ_CHGSTS_OSCFDET3,
	NXE2000_IRQ_CHGSTS_OSCFDET2,
	NXE2000_IRQ_CHGSTS_OSCFDET1,
	NXE2000_IRQ_CHGSTS_POOR_CHGCUR,
	NXE2000_IRQ_CHGSTS_ICRVS,
	NXE2000_IRQ_CHGSTS_VOLTERM,
	NXE2000_IRQ_CHGSTS_CURTERM,
	NXE2000_IRQ_CHGERR_VUSBOVS, /* 50 */
	NXE2000_IRQ_CHGERR_VADPOVS,
	NXE2000_IRQ_CHGERR_RTIMOV,
	NXE2000_IRQ_CHGERR_TTIMOV,
	NXE2000_IRQ_CHGERR_VBATOV,
	NXE2000_IRQ_CHGERR_BTEMPERR,
	NXE2000_IRQ_CHGERR_DIEERR,
	NXE2000_IRQ_CHGERR_DIEOFF,
	NXE2000_IRQ_NR,
};

enum nxe2000_pwr_src {
	NXE2000_PWR_SRC_BAT = 0,
	NXE2000_PWR_SRC_ADP,
	NXE2000_PWR_SRC_USB,
};

/* NXE2000 various variants */
enum {
	TYPE_NXE2000 = 0, /* Default */
};

/* NXE2000 IRQ definitions */
enum {
	NXE2000_IRQ_POWER_ON,
	NXE2000_IRQ_EXTIN,
	NXE2000_IRQ_PRE_VINDT,
	NXE2000_IRQ_PREOT,
	NXE2000_IRQ_POWER_OFF,
	NXE2000_IRQ_NOE_OFF,
	NXE2000_IRQ_WD,
	NXE2000_IRQ_CLK_STP,
	NXE2000_IRQ_DC1LIM,
	NXE2000_IRQ_DC2LIM,
	NXE2000_IRQ_DC3LIM,
	NXE2000_IRQ_DC4LIM,
	NXE2000_IRQ_DC5LIM,
	NXE2000_IRQ_ILIMLIR,
	NXE2000_IRQ_VBATLIR,
	NXE2000_IRQ_VADPLIR,
	NXE2000_IRQ_VUSBLIR,
	NXE2000_IRQ_VSYSLIR,
	NXE2000_IRQ_VTHMLIR,
	NXE2000_IRQ_AIN1LIR,
	NXE2000_IRQ_AIN0LIR,
	NXE2000_IRQ_ILIMHIR,
	NXE2000_IRQ_VBATHIR,
	NXE2000_IRQ_VADPHIR,
	NXE2000_IRQ_VUSBHIR,
	NXE2000_IRQ_VSYSHIR,
	NXE2000_IRQ_VTHMHIR,
	NXE2000_IRQ_AIN1HIR,
	NXE2000_IRQ_AIN0HIR,
	NXE2000_IRQ_ADC_ENDIR,
	NXE2000_IRQ_GPIO0,
	NXE2000_IRQ_GPIO1,
	NXE2000_IRQ_GPIO2,
	NXE2000_IRQ_GPIO3,
	NXE2000_IRQ_GPIO4,
	NXE2000_IRQ_CTC,
	NXE2000_IRQ_DALE,
	NXE2000_IRQ_FVADPDETSINT,
	NXE2000_IRQ_FVUSBDETSINT,
	NXE2000_IRQ_FVADPLVSINT,
	NXE2000_IRQ_FVUSBLVSINT,
	NXE2000_IRQ_FWVADPSINT,
	NXE2000_IRQ_FWVUSBSINT,
	NXE2000_IRQ_FONCHGINT,
	NXE2000_IRQ_FCHGCMPINT,
	NXE2000_IRQ_FBATOPENINT,
	NXE2000_IRQ_FSLPMODEINT,
	NXE2000_IRQ_FBTEMPJTA1INT,
	NXE2000_IRQ_FBTEMPJTA2INT,
	NXE2000_IRQ_FBTEMPJTA3INT,
	NXE2000_IRQ_FBTEMPJTA4INT,
	NXE2000_IRQ_FCURTERMINT,
	NXE2000_IRQ_FVOLTERMINT,
	NXE2000_IRQ_FICRVSINT,
	NXE2000_IRQ_FPOOR_CHGCURINT,
	NXE2000_IRQ_FOSCFDETINT1,
	NXE2000_IRQ_FOSCFDETINT2,
	NXE2000_IRQ_FOSCFDETINT3,
	NXE2000_IRQ_FOSCMDETINT,
	NXE2000_IRQ_FDIEOFFINT,
	NXE2000_IRQ_FDIEERRINT,
	NXE2000_IRQ_FBTEMPERRINT,
	NXE2000_IRQ_FVBATOVINT,
	NXE2000_IRQ_FTTIMOVINT,
	NXE2000_IRQ_FRTIMOVINT,
	NXE2000_IRQ_FVADPOVSINT,
	NXE2000_IRQ_FVUSBOVSINT,
	NXE2000_IRQ_FGCDET,
	NXE2000_IRQ_FPCDET,
	NXE2000_IRQ_FWARN_ADP,

	/* Should be last entry */
	NXE2000_NR_IRQS,
};

#define NXE2000_IRQ_SYSTEM_MASK (1 << 0)
#define NXE2000_IRQ_DCDC_MASK (1 << 1)
#define NXE2000_IRQ_ADC_MASK (1 << 3)
#define NXE2000_IRQ_GPIO_MASK (1 << 4)
#define NXE2000_IRQ_CHG_MASK (1 << 6)
#define NXE2000_IRQ_FG_MASK (1 << 7)

#define NXE2000_IRQ_WTSREVNT_MASK (1 << 0)
#define NXE2000_IRQ_SMPLEVNT_MASK (1 << 1)
#define NXE2000_IRQ_ALARM1_MASK (1 << 2)
#define NXE2000_IRQ_ALARM0_MASK (1 << 3)

#define NXE2000_IRQ_ONKEY1S_MASK (1 << 0)
#define NXE2000_IRQ_TOPOFFR_MASK (1 << 2)
#define NXE2000_IRQ_DCINOVPR_MASK (1 << 3)
#define NXE2000_IRQ_CHGRSTF_MASK (1 << 4)
#define NXE2000_IRQ_DONER_MASK (1 << 5)
#define NXE2000_IRQ_CHGFAULT_MASK (1 << 7)

#define NXE2000_IRQ_LOBAT1_MASK (1 << 0)
#define NXE2000_IRQ_LOBAT2_MASK (1 << 1)

#define NXE2000_ENRAMP (1 << 4)

#define NXE2000_NUM_GPIO 5
#define NXE2000_GPIO_INT_BOTH (0x3)
#define NXE2000_GPIO_INT_FALL (0x2)
#define NXE2000_GPIO_INT_RISE (0x1)
#define NXE2000_GPIO_INT_LEVEL (0x0)
#define NXE2000_GPIO_INT_MASK (0x3)

/* NXE2000 gpio definitions */
enum {
	NXE2000_GPIO0,
	NXE2000_GPIO1,
	NXE2000_GPIO2,
	NXE2000_GPIO3,
	NXE2000_GPIO4,

	NXE2000_NR_GPIO,
};


enum nxe2000_sleep_control_id {
	NXE2000_DS_DC1,
	NXE2000_DS_DC2,
	NXE2000_DS_DC3,
	NXE2000_DS_DC4,
	NXE2000_DS_DC5,
	NXE2000_DS_LDO1,
	NXE2000_DS_LDO2,
	NXE2000_DS_LDO3,
	NXE2000_DS_LDO4,
	NXE2000_DS_LDO5,
	NXE2000_DS_LDO6,
	NXE2000_DS_LDO7,
	NXE2000_DS_LDO8,
	NXE2000_DS_LDO9,
	NXE2000_DS_LDO10,
	NXE2000_DS_LDORTC1,
	NXE2000_DS_LDORTC2,
	NXE2000_DS_PSO0,
	NXE2000_DS_PSO1,
	NXE2000_DS_PSO2,
	NXE2000_DS_PSO3,
	NXE2000_DS_PSO4,
};

enum int_type {
	SYS_INT = 0x01,
	DCDC_INT = 0x02,
	RTC_INT = 0x04,
	ADC_INT = 0x08,
	GPIO_INT = 0x10,
	WDG_INT = 0x20,
	CHG_INT = 0x40,
	FG_INT = 0x80,
};

struct nxe2000_subdev_info {
	int id;
	const char *name;
	void *platform_data;
};

struct nxe2000_gpio_init_data {
	unsigned output_mode_en : 1; /* Enable output mode during init */
	unsigned output_val : 1;     /* Output value if it is in output mode */
	unsigned init_apply : 1;     /* Apply init data on configuring gpios*/
	unsigned led_mode : 1;       /* Select LED mode during init */
	unsigned led_func : 1;       /* Set LED function if LED mode is 1 */
};

struct nxe2000_platform_data {
	int num_regulators;
	int num_subdevs;
	int (*init_port)(int irq_num); /* Init GPIO for IRQ pin */
	int gpio_base;
	int irq_base;
	int irq_type;
	int num_gpioinit_data;
	bool enable_shutdown_pin;
	struct nxe2000_subdev_info *subdevs;
	struct nxe2000_gpio_init_data *gpio_init_data;
	struct nxe2000_regulator_platform_data *regulators;
};

struct nxe2000 {
	struct device *dev;
	struct nxe2000_platform_data *pdata;
	struct i2c_client *client;
	struct gpio_chip gpio_chip;
	struct gpio_chip gpio_irq_chip;
	struct mutex io_lock;
	struct mutex irq_lock;
	struct workqueue_struct *workqueue;
	struct delayed_work dcdc_int_work;

	int gpio_base;
	int irq_base;
	int chip_irq;
	int chip_irq_type;

	unsigned long group_irq_en[MAX_MAIN_INTERRUPT];

	/* For main interrupt bits in INTC */
	u8 intc_inten_cache;
	u8 intc_inten_reg;

	/* For group interrupt bits and address */
	u8 irq_en_cache[MAX_INTERRUPT_MASKS];
	u8 irq_en_reg[MAX_INTERRUPT_MASKS];

	/* For gpio edge */
	u8 gpedge_cache[MAX_GPEDGE_REG];
	u8 gpedge_reg[MAX_GPEDGE_REG];

	int bank_num;
};

/* ==================================== */
/* NXE2000 Power_Key device data	*/
/* ==================================== */
struct nxe2000_pwrkey_platform_data {
	int irq;
	unsigned long delay_ms;
};
extern int pwrkey_wakeup;
/* ==================================== */
/* NXE2000 battery device data		*/
/* ==================================== */
#if defined(CONFIG_BATTERY_NXE2000)
extern int g_soc;
extern int g_fg_on_mode;
#endif

extern int nxe2000_pm_read(uint8_t reg, uint8_t *val);
extern int nxe2000_read(struct device *dev, uint8_t reg, uint8_t *val);
extern int nxe2000_read_bank1(struct device *dev, uint8_t reg, uint8_t *val);
extern int nxe2000_bulk_reads(struct device *dev, u8 reg, u8 count,
			      uint8_t *val);
extern int nxe2000_bulk_reads_bank1(struct device *dev, u8 reg, u8 count,
				    uint8_t *val);
extern int nxe2000_pm_write(u8 reg, uint8_t val);
extern int nxe2000_write(struct device *dev, u8 reg, uint8_t val);
extern int nxe2000_write_bank1(struct device *dev, u8 reg, uint8_t val);
extern int nxe2000_bulk_writes(struct device *dev, u8 reg, u8 count,
			       uint8_t *val);
extern int nxe2000_bulk_writes_bank1(struct device *dev, u8 reg, u8 count,
				     uint8_t *val);
extern int nxe2000_set_bits(struct device *dev, u8 reg, uint8_t bit_mask);
extern int nxe2000_clr_bits(struct device *dev, u8 reg, uint8_t bit_mask);
extern int nxe2000_update(struct device *dev, u8 reg, uint8_t val,
			  uint8_t mask);
extern int nxe2000_update_bank1(struct device *dev, u8 reg, uint8_t val,
				uint8_t mask);
extern void nxe2000_power_off(void);
extern int nxe2000_irq_init(struct nxe2000 *nxe2000);
extern int nxe2000_irq_exit(struct nxe2000 *nxe2000);

#endif
