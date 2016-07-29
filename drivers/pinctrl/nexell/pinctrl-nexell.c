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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/irqdomain.h>
#include <linux/spinlock.h>
#include <linux/syscore_ops.h>
#include <linux/of_irq.h>

#include "../core.h"
#include "s5pxx18-gpio.h"
#include "pinctrl-nexell.h"

/* list of all possible config options supported */
static struct pin_config {
	const char *property;
	enum pincfg_type param;
} cfg_params[] = {
	{"nexell,pin-pull", PINCFG_TYPE_PULL},
	{"nexell,pin-strength", PINCFG_TYPE_DRV},
};

/* Global list of devices (struct nexell_pinctrl_drv_data) */
static LIST_HEAD(drvdata_list);

static unsigned int pin_base;

static inline struct nexell_pin_bank *gc_to_pin_bank(struct gpio_chip *gc)
{
	return container_of(gc, struct nexell_pin_bank, gpio_chip);
}

static int nexell_get_group_count(struct pinctrl_dev *pctldev)
{
	struct nexell_pinctrl_drv_data *pmx = pinctrl_dev_get_drvdata(pctldev);

	return pmx->nr_groups;
}

static const char *nexell_get_group_name(struct pinctrl_dev *pctldev,
					 unsigned group)
{
	struct nexell_pinctrl_drv_data *pmx = pinctrl_dev_get_drvdata(pctldev);

	return pmx->pin_groups[group].name;
}

static int nexell_get_group_pins(struct pinctrl_dev *pctldev, unsigned group,
				 const unsigned **pins, unsigned *num_pins)
{
	struct nexell_pinctrl_drv_data *pmx = pinctrl_dev_get_drvdata(pctldev);

	*pins = pmx->pin_groups[group].pins;
	*num_pins = pmx->pin_groups[group].num_pins;

	return 0;
}

static int reserve_map(struct device *dev, struct pinctrl_map **map,
		       unsigned *reserved_maps, unsigned *num_maps,
		       unsigned reserve)
{
	unsigned old_num = *reserved_maps;
	unsigned new_num = *num_maps + reserve;
	struct pinctrl_map *new_map;

	if (old_num >= new_num)
		return 0;

	new_map = krealloc(*map, sizeof(*new_map) * new_num, GFP_KERNEL);
	if (!new_map) {
		dev_err(dev, "krealloc(map) failed\n");
		return -ENOMEM;
	}

	memset(new_map + old_num, 0, (new_num - old_num) * sizeof(*new_map));

	*map = new_map;
	*reserved_maps = new_num;

	return 0;
}

static int add_map_mux(struct pinctrl_map **map, unsigned *reserved_maps,
		       unsigned *num_maps, const char *group,
		       const char *function)
{
	if (WARN_ON(*num_maps == *reserved_maps))
		return -ENOSPC;

	(*map)[*num_maps].type = PIN_MAP_TYPE_MUX_GROUP;
	(*map)[*num_maps].data.mux.group = group;
	(*map)[*num_maps].data.mux.function = function;
	(*num_maps)++;

	return 0;
}

static int add_map_configs(struct device *dev, struct pinctrl_map **map,
			   unsigned *reserved_maps, unsigned *num_maps,
			   const char *group, unsigned long *configs,
			   unsigned num_configs)
{
	unsigned long *dup_configs;

	if (WARN_ON(*num_maps == *reserved_maps))
		return -ENOSPC;

	dup_configs =
	    kmemdup(configs, num_configs * sizeof(*dup_configs), GFP_KERNEL);
	if (!dup_configs) {
		dev_err(dev, "kmemdup(configs) failed\n");
		return -ENOMEM;
	}

	(*map)[*num_maps].type = PIN_MAP_TYPE_CONFIGS_GROUP;
	(*map)[*num_maps].data.configs.group_or_pin = group;
	(*map)[*num_maps].data.configs.configs = dup_configs;
	(*map)[*num_maps].data.configs.num_configs = num_configs;
	(*num_maps)++;

	return 0;
}

static int add_config(struct device *dev, unsigned long **configs,
		      unsigned *num_configs, unsigned long config)
{
	unsigned old_num = *num_configs;
	unsigned new_num = old_num + 1;
	unsigned long *new_configs;

	new_configs =
	    krealloc(*configs, sizeof(*new_configs) * new_num, GFP_KERNEL);
	if (!new_configs) {
		dev_err(dev, "krealloc(configs) failed\n");
		return -ENOMEM;
	}

	new_configs[old_num] = config;

	*configs = new_configs;
	*num_configs = new_num;

	return 0;
}

static void nexell_dt_free_map(struct pinctrl_dev *pctldev,
			       struct pinctrl_map *map, unsigned num_maps)
{
	int i;

	for (i = 0; i < num_maps; i++)
		if (map[i].type == PIN_MAP_TYPE_CONFIGS_GROUP)
			kfree(map[i].data.configs.configs);

	kfree(map);
}

static int nexell_dt_subnode_to_map(struct nexell_pinctrl_drv_data *drvdata,
				    struct device *dev, struct device_node *np,
				    struct pinctrl_map **map,
				    unsigned *reserved_maps, unsigned *num_maps)
{
	int ret, i;
	u32 val;
	unsigned long config;
	unsigned long *configs = NULL;
	unsigned num_configs = 0;
	unsigned reserve;
	struct property *prop;
	const char *group;
	bool has_func = false;

	ret = of_property_read_u32(np, "nexell,pin-function", &val);
	if (!ret)
		has_func = true;

	for (i = 0; i < ARRAY_SIZE(cfg_params); i++) {
		ret = of_property_read_u32(np, cfg_params[i].property, &val);
		if (!ret) {
			config = PINCFG_PACK(cfg_params[i].param, val);
			ret = add_config(dev, &configs, &num_configs, config);
			if (ret < 0)
				goto exit;
			/* EINVAL=missing, which is fine since it's optional */
		} else if (ret != -EINVAL) {
			dev_err(dev, "could not parse property %s\n",
				cfg_params[i].property);
		}
	}

	reserve = 0;
	if (has_func)
		reserve++;
	if (num_configs)
		reserve++;
	ret = of_property_count_strings(np, "nexell,pins");
	if (ret < 0) {
		dev_err(dev, "could not parse property nexell,pins\n");
		goto exit;
	}
	reserve *= ret;

	ret = reserve_map(dev, map, reserved_maps, num_maps, reserve);
	if (ret < 0)
		goto exit;

	of_property_for_each_string(np, "nexell,pins", prop, group) {
		if (has_func) {
			ret = add_map_mux(map, reserved_maps, num_maps, group,
					  np->full_name);
			if (ret < 0)
				goto exit;
		}

		if (num_configs) {
			ret = add_map_configs(dev, map, reserved_maps, num_maps,
					      group, configs, num_configs);
			if (ret < 0)
				goto exit;
		}
	}

	ret = 0;

exit:
	kfree(configs);
	return ret;
}

static int nexell_dt_node_to_map(struct pinctrl_dev *pctldev,
				 struct device_node *np_config,
				 struct pinctrl_map **map, unsigned *num_maps)
{
	struct nexell_pinctrl_drv_data *drvdata;
	unsigned reserved_maps;
	struct device_node *np;
	int ret;

	drvdata = pinctrl_dev_get_drvdata(pctldev);

	reserved_maps = 0;
	*map = NULL;
	*num_maps = 0;

	if (!of_get_child_count(np_config))
		return nexell_dt_subnode_to_map(drvdata, pctldev->dev,
						np_config, map, &reserved_maps,
						num_maps);

	for_each_child_of_node(np_config, np) {
		ret = nexell_dt_subnode_to_map(drvdata, pctldev->dev, np, map,
					       &reserved_maps, num_maps);
		if (ret < 0) {
			nexell_dt_free_map(pctldev, *map, *num_maps);
			return ret;
		}
	}

	return 0;
}

/* list of pinctrl callbacks for the pinctrl core */
static const struct pinctrl_ops nexell_pctrl_ops = {
	.get_groups_count = nexell_get_group_count,
	.get_group_name = nexell_get_group_name,
	.get_group_pins = nexell_get_group_pins,
	.dt_node_to_map = nexell_dt_node_to_map,
	.dt_free_map = nexell_dt_free_map,
};

/* check if the selector is a valid pin function selector */
static int nexell_get_functions_count(struct pinctrl_dev *pctldev)
{
	struct nexell_pinctrl_drv_data *drvdata;

	drvdata = pinctrl_dev_get_drvdata(pctldev);
	return drvdata->nr_functions;
}

/* return the name of the pin function specified */
static const char *nexell_pinmux_get_fname(struct pinctrl_dev *pctldev,
					   unsigned selector)
{
	struct nexell_pinctrl_drv_data *drvdata;

	drvdata = pinctrl_dev_get_drvdata(pctldev);
	return drvdata->pmx_functions[selector].name;
}

/* return the groups associated for the specified function selector */
static int nexell_pinmux_get_groups(struct pinctrl_dev *pctldev,
				    unsigned selector,
				    const char *const **groups,
				    unsigned *const num_groups)
{
	struct nexell_pinctrl_drv_data *drvdata;

	drvdata = pinctrl_dev_get_drvdata(pctldev);
	*groups = drvdata->pmx_functions[selector].groups;
	*num_groups = drvdata->pmx_functions[selector].num_groups;
	return 0;
}

/*
 * given a pin number that is local to a pin controller, find out the pin bank
 * and the register base of the pin bank.
 */
static void pin_to_reg_bank(struct nexell_pinctrl_drv_data *drvdata,
			    unsigned pin, void __iomem **reg, u32 *offset,
			    struct nexell_pin_bank **bank)
{
	struct nexell_pin_bank *b;

	b = drvdata->ctrl->pin_banks;

	while ((pin >= b->pin_base) && ((b->pin_base + b->nr_pins - 1) < pin))
		b++;

	*reg = b->virt_base;
	*offset = pin - b->pin_base;
	if (bank)
		*bank = b;
}

/* enable or disable a pinmux function */
static void nexell_pinmux_setup(struct pinctrl_dev *pctldev, unsigned selector,
				unsigned group, bool enable)
{
	struct nexell_pinctrl_drv_data *drvdata;
	const struct nexell_pmx_func *func;
	const struct nexell_pin_group *grp;
	int io;

	drvdata = pinctrl_dev_get_drvdata(pctldev);
	func = &drvdata->pmx_functions[selector];
	grp = &drvdata->pin_groups[group];

	io = grp->pins[0];
	nx_soc_gpio_set_io_func(io, func->val);
}

/* enable a specified pinmux by writing to registers */
static int nexell_pinmux_set_mux(struct pinctrl_dev *pctldev, unsigned selector,
				 unsigned group)
{
	nexell_pinmux_setup(pctldev, selector, group, true);
	return 0;
}

/* list of pinmux callbacks for the pinmux vertical in pinctrl core */
static const struct pinmux_ops nexell_pinmux_ops = {
	.get_functions_count = nexell_get_functions_count,
	.get_function_name = nexell_pinmux_get_fname,
	.get_function_groups = nexell_pinmux_get_groups,
	.set_mux = nexell_pinmux_set_mux,
};

/* set or get the pin config settings for a specified pin */
static int nexell_soc_write_pin(unsigned int io,
				enum pincfg_type cfg_type,
				u32 data)
{
	if (cfg_type >= PINCFG_TYPE_NUM)
		return -EINVAL;

	switch (cfg_type) {
	case PINCFG_TYPE_DAT:
		nx_soc_gpio_set_out_value(io, data);
		break;
	case PINCFG_TYPE_PULL:
		nx_soc_gpio_set_io_pull(io, data);
		break;
	case PINCFG_TYPE_DRV:
		nx_soc_gpio_set_io_drv(io, data);
		break;
	case PINCFG_TYPE_FUNC:
		nx_soc_gpio_set_io_func(io, data);
		break;
	case PINCFG_TYPE_DIR:
		nx_soc_gpio_set_io_dir(io, data);
		break;
	default:
		break;
	}

	return 0;
}

static int nexell_soc_read_pin(unsigned int io,
			       enum pincfg_type cfg_type,
			       u32 *data)
{
	if (cfg_type >= PINCFG_TYPE_NUM)
		return -EINVAL;

	switch (cfg_type) {
	case PINCFG_TYPE_DAT:
		*data = nx_soc_gpio_get_in_value(io);
		break;
	case PINCFG_TYPE_PULL:
		*data = nx_soc_gpio_get_io_pull(io);
		break;
	case PINCFG_TYPE_DRV:
		*data = nx_soc_gpio_get_io_drv(io);
		break;
	case PINCFG_TYPE_DIR:
		*data = nx_soc_gpio_get_io_dir(io);
		break;
	case PINCFG_TYPE_FUNC:
		*data = nx_soc_gpio_get_io_func(io);
		break;
	default:
		break;
	}

	return 0;
}

static int nexell_pinconf_rw(struct pinctrl_dev *pctldev, unsigned int pin,
			     unsigned long *config, bool set)
{
	struct nexell_pinctrl_drv_data *drvdata;
	struct nexell_pin_bank *bank;
	void __iomem *reg_base;
	enum pincfg_type cfg_type = PINCFG_UNPACK_TYPE(*config);
	u32 pin_offset;
	u32 cfg_value;
	int io;

	drvdata = pinctrl_dev_get_drvdata(pctldev);
	pin_to_reg_bank(drvdata, pin - drvdata->ctrl->base, &reg_base,
			&pin_offset, &bank);

	io = bank->grange.pin_base + pin_offset;

	if (cfg_type >= PINCFG_TYPE_NUM)
		return -EINVAL;

	if (set) {
		cfg_value = PINCFG_UNPACK_VALUE(*config);
		nexell_soc_write_pin(io, cfg_type, cfg_value);
	} else {
		nexell_soc_read_pin(io, cfg_type, &cfg_value);
		*config = PINCFG_PACK(cfg_type, cfg_value);
	}

	return 0;
}

/* set the pin config settings for a specified pin */
static int nexell_pinconf_set(struct pinctrl_dev *pctldev, unsigned int pin,
			      unsigned long *configs, unsigned num_configs)
{
	int i, ret;

	for (i = 0; i < num_configs; i++) {
		ret = nexell_pinconf_rw(pctldev, pin, &configs[i], true);
		if (ret < 0)
			return ret;
	} /* for each config */

	return 0;
}

/* get the pin config settings for a specified pin */
static int nexell_pinconf_get(struct pinctrl_dev *pctldev, unsigned int pin,
			      unsigned long *config)
{
	return nexell_pinconf_rw(pctldev, pin, config, false);
}

/* set the pin config settings for a specified pin group */
static int nexell_pinconf_group_set(struct pinctrl_dev *pctldev, unsigned group,
				    unsigned long *configs,
				    unsigned num_configs)
{
	struct nexell_pinctrl_drv_data *drvdata;
	const unsigned int *pins;
	unsigned int cnt;

	drvdata = pinctrl_dev_get_drvdata(pctldev);
	pins = drvdata->pin_groups[group].pins;

	for (cnt = 0; cnt < drvdata->pin_groups[group].num_pins; cnt++)
		nexell_pinconf_set(pctldev, pins[cnt], configs, num_configs);

	return 0;
}

/* get the pin config settings for a specified pin group */
static int nexell_pinconf_group_get(struct pinctrl_dev *pctldev,
				    unsigned int group, unsigned long *config)
{
	struct nexell_pinctrl_drv_data *drvdata;
	const unsigned int *pins;

	drvdata = pinctrl_dev_get_drvdata(pctldev);
	pins = drvdata->pin_groups[group].pins;
	nexell_pinconf_get(pctldev, pins[0], config);
	return 0;
}

/* list of pinconfig callbacks for pinconfig vertical in the pinctrl code */
static const struct pinconf_ops nexell_pinconf_ops = {
	.pin_config_get = nexell_pinconf_get,
	.pin_config_set = nexell_pinconf_set,
	.pin_config_group_get = nexell_pinconf_group_get,
	.pin_config_group_set = nexell_pinconf_group_set,
};

/* gpiolib gpio_set callback function */
static void nx_gpio_set(struct gpio_chip *gc, unsigned offset, int value)
{
	struct nexell_pin_bank *bank = gc_to_pin_bank(gc);
	int io;

	io = bank->grange.pin_base + offset;
	nx_soc_gpio_set_out_value(io, value);
}

/* gpiolib gpio_get callback function */
static int nx_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	u32 data;
	struct nexell_pin_bank *bank = gc_to_pin_bank(gc);
	int io;

	io = bank->grange.pin_base + offset;
	data = nx_soc_gpio_get_in_value(io);

	return data;
}

/*
 * The calls to gpio_direction_output() and gpio_direction_input()
 * leads to this function call.
 */
static int nx_gpio_set_direction(struct gpio_chip *gc, unsigned offset,
				 bool input)
{
	struct nexell_pin_bank *bank;
	struct nexell_pinctrl_drv_data *drvdata;
	int io;
	int fn;

	bank = gc_to_pin_bank(gc);
	drvdata = bank->drvdata;

	io = bank->grange.pin_base + offset;
	fn = nx_soc_gpio_get_altnum(io);

	/*nx_soc_gpio_set_io_func(io, fn);*/
	nx_soc_gpio_set_io_dir(io, input ? 0 : 1);

	return 0;
}

/* gpiolib gpio_direction_input callback function. */
static int nx_gpio_direction_input(struct gpio_chip *gc, unsigned offset)
{
	return nx_gpio_set_direction(gc, offset, true);
}

/* gpiolib gpio_direction_output callback function. */
static int nx_gpio_direction_output(struct gpio_chip *gc, unsigned offset,
				    int value)
{
	nx_gpio_set(gc, offset, value);
	return nx_gpio_set_direction(gc, offset, false);
}

/*
 * gpiolib gpio_to_irq callback function. Creates a mapping between a GPIO pin
 * and a virtual IRQ, if not already present.
 */
static int nx_gpio_to_irq(struct gpio_chip *gc, unsigned offset)
{
	struct nexell_pin_bank *bank = gc_to_pin_bank(gc);
	unsigned int virq;

	if (!bank->irq_domain)
		return -ENXIO;

	virq = irq_create_mapping(bank->irq_domain, offset);

	return (virq) ?: -ENXIO;
}

static struct nexell_pin_group *
nexell_pinctrl_create_groups(struct device *dev,
			     struct nexell_pinctrl_drv_data *drvdata,
			     unsigned int *cnt)
{
	struct pinctrl_desc *ctrldesc = &drvdata->pctl;
	struct nexell_pin_group *groups, *grp;
	const struct pinctrl_pin_desc *pdesc;
	int i;

	groups =
	    devm_kzalloc(dev, ctrldesc->npins * sizeof(*groups), GFP_KERNEL);
	if (!groups)
		return ERR_PTR(-EINVAL);
	grp = groups;

	pdesc = ctrldesc->pins;
	for (i = 0; i < ctrldesc->npins; ++i, ++pdesc, ++grp) {
		grp->name = pdesc->name;
		grp->pins = &pdesc->number;
		grp->num_pins = 1;
	}

	*cnt = ctrldesc->npins;
	return groups;
}

static int nexell_pinctrl_create_function(struct device *dev,
				struct nexell_pinctrl_drv_data *drvdata,
				struct device_node *func_np,
				struct nexell_pmx_func *func)
{
	int npins;
	int ret;
	int i;

	if (of_property_read_u32(func_np, "nexell,pin-function", &func->val))
		return 0;

	npins = of_property_count_strings(func_np, "nexell,pins");
	if (npins < 1) {
		dev_err(dev, "invalid pin list in %s node", func_np->name);
		return -EINVAL;
	}

	func->name = func_np->full_name;

	func->groups = devm_kzalloc(dev, npins * sizeof(char *), GFP_KERNEL);
	if (!func->groups)
		return -ENOMEM;

	for (i = 0; i < npins; ++i) {
		const char *gname;

		ret = of_property_read_string_index(func_np, "nexell,pins", i,
						    &gname);
		if (ret) {
			dev_err(dev,
				"failed to read pin name %d from %s node\n", i,
				func_np->name);
			return ret;
		}

		func->groups[i] = gname;
	}

	func->num_groups = npins;
	return 1;
}

static struct nexell_pmx_func *
nexell_pinctrl_create_functions(struct device *dev,
				struct nexell_pinctrl_drv_data *drvdata,
				unsigned int *cnt)
{
	struct nexell_pmx_func *functions, *func;
	struct device_node *dev_np = dev->of_node;
	struct device_node *cfg_np;
	unsigned int func_cnt = 0;
	int ret;

	/*
	 * Iterate over all the child nodes of the pin controller node
	 * and create pin groups and pin function lists.
	 */
	for_each_child_of_node(dev_np, cfg_np) {
		struct device_node *func_np;

		if (!of_get_child_count(cfg_np)) {
			if (!of_find_property(cfg_np, "nexell,pin-function",
					      NULL))
				continue;
			++func_cnt;
			continue;
		}

		for_each_child_of_node(cfg_np, func_np) {
			if (!of_find_property(func_np, "nexell,pin-function",
					      NULL))
				continue;
			++func_cnt;
		}
	}

	functions =
	    devm_kzalloc(dev, func_cnt * sizeof(*functions), GFP_KERNEL);
	if (!functions) {
		dev_err(dev, "failed to allocate memory for function list\n");
		return ERR_PTR(-EINVAL);
	}
	func = functions;

	/*
	 * Iterate over all the child nodes of the pin controller node
	 * and create pin groups and pin function lists.
	 */
	func_cnt = 0;
	for_each_child_of_node(dev_np, cfg_np) {
		struct device_node *func_np;

		if (!of_get_child_count(cfg_np)) {
			ret = nexell_pinctrl_create_function(dev, drvdata,
							     cfg_np, func);
			if (ret < 0)
				return ERR_PTR(ret);
			if (ret > 0) {
				++func;
				++func_cnt;
			}
			continue;
		}

		for_each_child_of_node(cfg_np, func_np) {
			ret = nexell_pinctrl_create_function(dev, drvdata,
							     func_np, func);
			if (ret < 0)
				return ERR_PTR(ret);
			if (ret > 0) {
				++func;
				++func_cnt;
			}
		}
	}

	*cnt = func_cnt;
	return functions;
}

/*
 * Parse the information about all the available pin groups and pin functions
 * from device node of the pin-controller. A pin group is formed with all
 * the pins listed in the "nexell,pins" property.
 */
static int nexell_pinctrl_parse_dt(struct platform_device *pdev,
				   struct nexell_pinctrl_drv_data *drvdata)
{
	struct device *dev = &pdev->dev;
	struct nexell_pin_group *groups;
	struct nexell_pmx_func *functions;
	unsigned int grp_cnt = 0, func_cnt = 0;

	groups = nexell_pinctrl_create_groups(dev, drvdata, &grp_cnt);
	if (IS_ERR(groups)) {
		dev_err(dev, "failed to parse pin groups\n");
		return PTR_ERR(groups);
	}

	functions = nexell_pinctrl_create_functions(dev, drvdata, &func_cnt);
	if (IS_ERR(functions)) {
		dev_err(dev, "failed to parse pin functions\n");
		return PTR_ERR(groups);
	}

	drvdata->pin_groups = groups;
	drvdata->nr_groups = grp_cnt;
	drvdata->pmx_functions = functions;
	drvdata->nr_functions = func_cnt;

	return 0;
}

/* register the pinctrl interface with the pinctrl subsystem */
static int nexell_pinctrl_register(struct platform_device *pdev,
				   struct nexell_pinctrl_drv_data *drvdata)
{
	struct pinctrl_desc *ctrldesc = &drvdata->pctl;
	struct pinctrl_pin_desc *pindesc, *pdesc;
	struct nexell_pin_bank *pin_bank;
	char *pin_names;
	int pin, bank, ret;

	ctrldesc->name = "nexell-pinctrl";
	ctrldesc->owner = THIS_MODULE;
	ctrldesc->pctlops = &nexell_pctrl_ops;
	ctrldesc->pmxops = &nexell_pinmux_ops;
	ctrldesc->confops = &nexell_pinconf_ops;

	pindesc = devm_kzalloc(
	    &pdev->dev, sizeof(*pindesc) * drvdata->ctrl->nr_pins, GFP_KERNEL);
	if (!pindesc)
		return -ENOMEM;
	ctrldesc->pins = pindesc;
	ctrldesc->npins = drvdata->ctrl->nr_pins;

	/* dynamically populate the pin number and pin name for pindesc */
	for (pin = 0, pdesc = pindesc; pin < ctrldesc->npins; pin++, pdesc++)
		pdesc->number = pin + drvdata->ctrl->base;

	/*
	 * allocate space for storing the dynamically generated names for all
	 * the pins which belong to this pin-controller.
	 */
	pin_names = devm_kzalloc(&pdev->dev, sizeof(char) * PIN_NAME_LENGTH *
						 drvdata->ctrl->nr_pins,
				 GFP_KERNEL);
	if (!pin_names) {
		dev_err(&pdev->dev, "mem alloc for pin names failed\n");
		return -ENOMEM;
	}

	/* for each pin, the name of the pin is pin-bank name + pin number */
	for (bank = 0; bank < drvdata->ctrl->nr_banks; bank++) {
		pin_bank = &drvdata->ctrl->pin_banks[bank];
		for (pin = 0; pin < pin_bank->nr_pins; pin++) {
			sprintf(pin_names, "%s-%d", pin_bank->name, pin);
			pdesc = pindesc + pin_bank->pin_base + pin;
			pdesc->name = pin_names;
			pin_names += PIN_NAME_LENGTH;
		}
	}

	ret = nexell_pinctrl_parse_dt(pdev, drvdata);
	if (ret)
		return ret;

	drvdata->pctl_dev = pinctrl_register(ctrldesc, &pdev->dev, drvdata);
	if (!drvdata->pctl_dev) {
		dev_err(&pdev->dev, "could not register pinctrl driver\n");
		return -EINVAL;
	}

	for (bank = 0; bank < drvdata->ctrl->nr_banks; ++bank) {
		pin_bank = &drvdata->ctrl->pin_banks[bank];
		pin_bank->grange.name = pin_bank->name;
		pin_bank->grange.id = bank;
		pin_bank->grange.pin_base =
		    drvdata->ctrl->base + pin_bank->pin_base;
		pin_bank->grange.base = pin_bank->gpio_chip.base;
		pin_bank->grange.npins = pin_bank->gpio_chip.ngpio;
		pin_bank->grange.gc = &pin_bank->gpio_chip;
		pinctrl_add_gpio_range(drvdata->pctl_dev, &pin_bank->grange);
	}

	return 0;
}

static int nx_gpio_request(struct gpio_chip *gc, unsigned offset)
{
	struct nexell_pin_bank *bank;
	struct nexell_pinctrl_drv_data *drvdata;

	int io;
	int fn;

	bank = gc_to_pin_bank(gc);
	drvdata = bank->drvdata;

	io = bank->grange.pin_base + offset;
	fn = nx_soc_gpio_get_altnum(io);

	nx_soc_gpio_set_io_func(io, fn);

	return pinctrl_request_gpio(gc->base + offset);
}

static void nx_gpio_free(struct gpio_chip *gc, unsigned offset)
{
	pinctrl_free_gpio(gc->base + offset);
}

static const struct gpio_chip nexell_gpiolib_chip = {
	.request = nx_gpio_request,
	.free = nx_gpio_free,
	.set = nx_gpio_set,
	.get = nx_gpio_get,
	.direction_input = nx_gpio_direction_input,
	.direction_output = nx_gpio_direction_output,
	.to_irq = nx_gpio_to_irq,
	.owner = THIS_MODULE,
};

/* register the gpiolib interface with the gpiolib subsystem */
static int nexell_gpiolib_register(struct platform_device *pdev,
				   struct nexell_pinctrl_drv_data *drvdata)
{
	struct nexell_pin_ctrl *ctrl = drvdata->ctrl;
	struct nexell_pin_bank *bank = ctrl->pin_banks;
	struct gpio_chip *gc;
	int ret;
	int i;

	pr_debug("gpiolib register ctr: %p, nr banks: %d\n", ctrl,
		 ctrl->nr_banks);

	for (i = 0; i < ctrl->nr_banks; ++i, ++bank) {
		bank->gpio_chip = nexell_gpiolib_chip;

		gc = &bank->gpio_chip;
		gc->base = ctrl->base + bank->pin_base;
		gc->ngpio = bank->nr_pins;
		gc->dev = &pdev->dev;
		gc->of_node = bank->of_node;
		gc->label = bank->name;

		ret = gpiochip_add(gc);
		if (ret) {
			dev_err(
			    &pdev->dev,
			    "failed to register gpio_chip %s, error code: %d\n",
			    gc->label, ret);
			goto fail;
		}
	}

	return 0;

fail:
	for (--i, --bank; i >= 0; --i, --bank)
		gpiochip_remove(&bank->gpio_chip);
	return ret;
}

/* unregister the gpiolib interface with the gpiolib subsystem */
static int nexell_gpiolib_unregister(struct platform_device *pdev,
				     struct nexell_pinctrl_drv_data *drvdata)
{
	struct nexell_pin_ctrl *ctrl = drvdata->ctrl;
	struct nexell_pin_bank *bank = ctrl->pin_banks;
	int i;

	for (i = 0; i < ctrl->nr_banks; ++i, ++bank)
		gpiochip_remove(&bank->gpio_chip);
	return 0;
}

static const struct of_device_id nexell_pinctrl_dt_match[];

/* retrieve the soc specific data */
static struct nexell_pin_ctrl *
nexell_pinctrl_get_soc_data(struct nexell_pinctrl_drv_data *d,
			    struct platform_device *pdev)
{
	int id;
	const struct of_device_id *match;
	struct device_node *node = pdev->dev.of_node;
	struct device_node *np;
	struct nexell_pin_ctrl *ctrl;
	struct nexell_pin_bank *bank;
	int i;

	id = of_alias_get_id(node, "pinctrl");
	if (id < 0) {
		dev_err(&pdev->dev, "failed to get alias id\n");
		return NULL;
	}
	match = of_match_node(nexell_pinctrl_dt_match, node);
	ctrl = (struct nexell_pin_ctrl *)match->data + id;

	bank = ctrl->pin_banks;
	for (i = 0; i < ctrl->nr_banks; ++i, ++bank) {
		spin_lock_init(&bank->slock);
		bank->drvdata = d;
		bank->pin_base = ctrl->nr_pins;
		ctrl->nr_pins += bank->nr_pins;
	}

	for_each_child_of_node(node, np) {
		if (!of_find_property(np, "gpio-controller", NULL))
			continue;
		bank = ctrl->pin_banks;
		for (i = 0; i < ctrl->nr_banks; ++i, ++bank) {
			if (!strcmp(bank->name, np->name)) {
				bank->of_node = np;
				break;
			}
		}
	}

	ctrl->base = pin_base;
	pin_base += ctrl->nr_pins;

	return ctrl;
}

static int nexell_pinctrl_probe(struct platform_device *pdev)
{
	struct nexell_pinctrl_drv_data *drvdata;
	struct device *dev = &pdev->dev;
	struct nexell_pin_ctrl *ctrl;
	struct resource *res;
	int irq;
	int ret;
	int i;

	if (!dev->of_node) {
		dev_err(dev, "device tree node not found\n");
		return -ENODEV;
	}

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	ctrl = nexell_pinctrl_get_soc_data(drvdata, pdev);
	if (!ctrl) {
		dev_err(&pdev->dev, "driver data not available\n");
		return -EINVAL;
	}
	drvdata->ctrl = ctrl;
	drvdata->dev = dev;

	for (i = 0;; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res)
			break;
	}
	if (i != ctrl->nr_banks) {
		dev_err(&pdev->dev, "bank count mismatch\n");
		return -EINVAL;
	}

	for (i = 0; i < ctrl->nr_banks; i++) {
		struct nexell_pin_bank *bank = &ctrl->pin_banks[i];

		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		bank->virt_base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(bank->virt_base))
			return PTR_ERR(bank->virt_base);
	}

	for (i = 0; i < ctrl->nr_banks; i++) {
		struct nexell_pin_bank *bank = &ctrl->pin_banks[i];

		if (bank->eint_type == EINT_TYPE_NONE)
			continue;

		irq = irq_of_parse_and_map(dev->of_node, i);
		if (irq <= 0) {
			dev_err(dev, "irq parsing failed\n");
			return -EINVAL;
		}

		bank->irq = irq;
	}

	ret = nexell_gpiolib_register(pdev, drvdata);
	if (ret)
		return ret;

	ret = nexell_pinctrl_register(pdev, drvdata);
	if (ret) {
		nexell_gpiolib_unregister(pdev, drvdata);
		return ret;
	}

	if (ctrl->base_init)
		ctrl->base_init(drvdata);
	if (ctrl->gpio_irq_init)
		ctrl->gpio_irq_init(drvdata);
	if (ctrl->alive_irq_init)
		ctrl->alive_irq_init(drvdata);

	platform_set_drvdata(pdev, drvdata);

	/* Add to the global list */
	list_add_tail(&drvdata->node, &drvdata_list);

	return 0;
}

#ifdef CONFIG_PM

/**
 * nexell_pinctrl_suspend_dev - save pinctrl state for suspend for a device
 *
 * Save data for all banks handled by this device.
 */
static void nexell_pinctrl_suspend_dev(
	struct nexell_pinctrl_drv_data *drvdata)
{
	struct nexell_pin_ctrl *ctrl = drvdata->ctrl;

	if (ctrl->suspend)
		ctrl->suspend(drvdata);
}

/**
 * nexell_pinctrl_resume_dev - restore pinctrl state from suspend for a device
 *
 * Restore one of the banks that was saved during suspend.
 *
 * We don't bother doing anything complicated to avoid glitching lines since
 * we're called before pad retention is turned off.
 */
static void nexell_pinctrl_resume_dev(struct nexell_pinctrl_drv_data *drvdata)
{
	struct nexell_pin_ctrl *ctrl = drvdata->ctrl;

	if (ctrl->resume)
		ctrl->resume(drvdata);
}

/**
 * nexell_pinctrl_suspend - save pinctrl state for suspend
 *
 * Save data for all banks across all devices.
 */
static int nexell_pinctrl_suspend(void)
{
	struct nexell_pinctrl_drv_data *drvdata;

	list_for_each_entry(drvdata, &drvdata_list, node) {
		nexell_pinctrl_suspend_dev(drvdata);
	}

	return 0;
}

/**
 * nexell_pinctrl_resume - restore pinctrl state for suspend
 *
 * Restore data for all banks across all devices.
 */
static void nexell_pinctrl_resume(void)
{
	struct nexell_pinctrl_drv_data *drvdata;

	list_for_each_entry_reverse(drvdata, &drvdata_list, node) {
		nexell_pinctrl_resume_dev(drvdata);
	}
}

#else
#define nexell_pinctrl_suspend		NULL
#define nexell_pinctrl_resume		NULL
#endif

static struct syscore_ops nexell_pinctrl_syscore_ops = {
	.suspend	= nexell_pinctrl_suspend,
	.resume		= nexell_pinctrl_resume,
};

static const struct of_device_id nexell_pinctrl_dt_match[] = {
	{ .compatible = "nexell,s5p6818-pinctrl",
		.data = (void *)s5pxx18_pin_ctrl },
	{ .compatible = "nexell,s5pxx18-pinctrl",
		.data = (void *)s5pxx18_pin_ctrl },
	{},
};
MODULE_DEVICE_TABLE(of, nexell_pinctrl_dt_match);

static struct platform_driver nexell_pinctrl_driver = {
	.probe = nexell_pinctrl_probe,
	.driver = {
		.name = "nexell-pinctrl",
		.owner = THIS_MODULE,
		.of_match_table = nexell_pinctrl_dt_match,
	},
};

static int __init nexell_pinctrl_drv_register(void)
{
	register_syscore_ops(&nexell_pinctrl_syscore_ops);

	return platform_driver_register(&nexell_pinctrl_driver);
}
postcore_initcall(nexell_pinctrl_drv_register);

static void __exit nexell_pinctrl_drv_unregister(void)
{
	platform_driver_unregister(&nexell_pinctrl_driver);
}
module_exit(nexell_pinctrl_drv_unregister);

MODULE_AUTHOR("Bon-gyu, KOO <freestyle@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell pinctrl driver");
MODULE_LICENSE("GPL v2");
