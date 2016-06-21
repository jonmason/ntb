/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Bon-gyu, KOO <freestyle@nexell.co.kr>
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

#ifndef __PINCTRL_NEXELL_H
#define __PINCTRL_NEXELL_H

#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>

#include <linux/gpio.h>

/**
 * enum pincfg_type - possible pin configuration types supported.
 * @PINCFG_TYPE_FUNC: Function configuration.
 * @PINCFG_TYPE_DAT: Pin value configuration.
 * @PINCFG_TYPE_PULL: Pull up/down configuration.
 * @PINCFG_TYPE_DRV: Drive strength configuration.
 * @PINCFG_TYPE_DIR: pin direction for GPIO mode.
 */
enum pincfg_type {
	PINCFG_TYPE_FUNC,
	PINCFG_TYPE_DAT,
	PINCFG_TYPE_PULL,
	PINCFG_TYPE_DRV,
	PINCFG_TYPE_DIR,

	PINCFG_TYPE_NUM
};

/*
 * pin configuration (pull up/down and drive strength) type and its value are
 * packed together into a 16-bits. The upper 8-bits represent the configuration
 * type and the lower 8-bits hold the value of the configuration type.
 */
#define PINCFG_TYPE_MASK		0xFF
#define PINCFG_VALUE_SHIFT		8
#define PINCFG_VALUE_MASK		(0xFF << PINCFG_VALUE_SHIFT)
#define PINCFG_PACK(type, value)	(((value) << PINCFG_VALUE_SHIFT) | type)
#define PINCFG_UNPACK_TYPE(cfg)		((cfg) & PINCFG_TYPE_MASK)
#define PINCFG_UNPACK_VALUE(cfg)	(((cfg) & PINCFG_VALUE_MASK) >> \
						PINCFG_VALUE_SHIFT)
/**
 * enum eint_type - possible external interrupt types.
 * @EINT_TYPE_NONE: bank does not support external interrupts
 * @EINT_TYPE_GPIO: bank supportes external gpio interrupts
 * @EINT_TYPE_WKUP: bank supportes external wakeup interrupts
 *
 * GPIO controller groups all the available pins into banks. The pins
 * in a pin bank can support external gpio interrupts or external wakeup
 * interrupts or no interrupts at all. From a software perspective, the only
 * difference between external gpio and external wakeup interrupts is that
 * the wakeup interrupts can additionally wakeup the system if it is in
 * suspended state.
 */
enum eint_type {
	EINT_TYPE_NONE,
	EINT_TYPE_GPIO,
	EINT_TYPE_WKUP,
};

/* maximum length of a pin in pin descriptor (example: "gpioa-30") */
#define PIN_NAME_LENGTH	10

#define PIN_GROUP(n, p, f)				\
	{						\
		.name		= n,			\
		.pins		= p,			\
		.num_pins	= ARRAY_SIZE(p),	\
		.func		= f			\
	}

#define PMX_FUNC(n, g)					\
	{						\
		.name		= n,			\
		.groups		= g,			\
		.num_groups	= ARRAY_SIZE(g),	\
	}

struct nexell_pinctrl_drv_data;

/**
 * struct nexell_pin_bank: represent a controller pin-bank.
 * @pctl_offset: starting offset of the pin-bank registers.
 * @pin_base: starting pin number of the bank.
 * @nr_pins: number of pins included in this bank.
 * @eint_func: function to set in CON register to configure pin as EINT.
 * @eint_type: type of the external interrupt supported by the bank.
 * @eint_mask: bit mask of pins which support EINT function.
 * @name: name to be prefixed for each pin in this pin bank.
 * @of_node: OF node of the bank.
 * @drvdata: link to controller driver data
 * @irq_domain: IRQ domain of the bank.
 * @gpio_chip: GPIO chip of the bank.
 * @grange: linux gpio pin range supported by this bank.
 * @slock: spinlock protecting bank registers
 */
struct nexell_pin_bank {
	u32		pctl_offset;
	u32		pin_base;
	u8		nr_pins;
	u8		eint_func;
	enum eint_type	eint_type;
	u32		eint_mask;
	void __iomem	*virt_base;
	char		*name;
	void		*soc_priv;
	int		irq;
	struct device_node *of_node;
	struct nexell_pinctrl_drv_data *drvdata;
	struct irq_domain *irq_domain;
	struct gpio_chip gpio_chip;
	struct pinctrl_gpio_range grange;
	struct nexell_irq_chip *irq_chip;
	spinlock_t slock;
};

/**
 * struct nexell_pin_ctrl: represent a pin controller.
 * @pin_banks: list of pin banks included in this controller.
 * @nr_banks: number of pin banks.
 * @base: starting system wide pin number.
 * @nr_pins: number of pins supported by the controller.
 * @eint_gpio_init: platform specific callback to setup the external gpio
 *	interrupts for the controller.
 * @eint_alive_init: platform specific callback to setup the external wakeup
 *	interrupts for the controller.
 * @label: for debug information.
 */
struct nexell_pin_ctrl {
	struct nexell_pin_bank	*pin_banks;
	u32		nr_banks;

	u32		base;
	u32		nr_pins;

	int		(*base_init)(struct nexell_pinctrl_drv_data *);
	int		(*gpio_irq_init)(struct nexell_pinctrl_drv_data *);
	int		(*alive_irq_init)(struct nexell_pinctrl_drv_data *);
	void		(*suspend)(struct nexell_pinctrl_drv_data *);
	void		(*resume)(struct nexell_pinctrl_drv_data *);
};

/**
 * struct nexell_pinctrl_drv_data: wrapper for holding driver data together.
 * @node: global list node
 * @virt_base: register base address of the controller.
 * @dev: device instance representing the controller.
 * @irq: interrpt number used by the controller to notify gpio interrupts.
 * @ctrl: pin controller instance managed by the driver.
 * @pctl: pin controller descriptor registered with the pinctrl subsystem.
 * @pctl_dev: cookie representing pinctrl device instance.
 * @pin_groups: list of pin groups available to the driver.
 * @nr_groups: number of such pin groups.
 * @pmx_functions: list of pin functions available to the driver.
 * @nr_function: number of such pin functions.
 */
struct nexell_pinctrl_drv_data {
	struct list_head		node;
	void __iomem			*virt_base;
	struct device			*dev;
	int				irq;

	struct nexell_pin_ctrl		*ctrl;
	struct pinctrl_desc		pctl;
	struct pinctrl_dev		*pctl_dev;

	const struct nexell_pin_group	*pin_groups;
	unsigned int			nr_groups;
	const struct nexell_pmx_func	*pmx_functions;
	unsigned int			nr_functions;
};

/**
 * struct nexell_pin_group: represent group of pins of a pinmux function.
 * @name: name of the pin group, used to lookup the group.
 * @pins: the pins included in this group.
 * @num_pins: number of pins included in this group.
 * @func: the function number to be programmed when selected.
 */
struct nexell_pin_group {
	const char		*name;
	const unsigned int	*pins;
	u8			num_pins;
	u8			func;
};

/**
 * struct nexell_pmx_func: represent a pin function.
 * @name: name of the pin function, used to lookup the function.
 * @groups: one or more names of pin groups that provide this function.
 * @num_groups: number of groups included in @groups.
 */
struct nexell_pmx_func {
	const char		*name;
	const char		**groups;
	u8			num_groups;
	u32			val;
};

/* list of all exported SoC specific data */
extern const struct nexell_pin_ctrl s5pxx18_pin_ctrl[];

#endif	/* __PINCTRL_NEXELL_H */
