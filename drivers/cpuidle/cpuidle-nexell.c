/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Youngbok, Park <ybpark@nexell.co.kr>
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

#include <linux/init.h>
#include <linux/cpuidle.h>
#include <asm/cpuidle.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/cpu_pm.h>

#include "dt_idle_states.h"

#define NEXELL_MAX_STATES		2

static int nexell_enter_idle(struct cpuidle_device *dev,
			   struct cpuidle_driver *drv, int index)
{
	cpu_do_idle();

	return index;
}

static struct cpuidle_driver nexell_idle_driver = {
	.name = "nexell_idle",
	.owner = THIS_MODULE,
	.states = {
		ARM_CPUIDLE_WFI_STATE,
		{
			.enter			= nexell_enter_idle,
			.exit_latency		= 10,
			.target_residency	= 10000,
			.name			= "Nexell Idle",
			.desc			= "Nexell cpu Idle",
		},
	},
	.safe_state_index = 0,
	.state_count = NEXELL_MAX_STATES,
};

static const struct of_device_id nexell_idle_state_match[] __initconst = {
	{ .compatible = "nexell,idle-state",
	  .data = nexell_enter_idle },
	{ },
};

static int __init nexell_idle_init(void)
{
	int cpu, ret;
	struct cpuidle_driver *drv = &nexell_idle_driver;
	struct cpuidle_device *dev;

	ret = dt_init_idle_driver(drv, nexell_idle_state_match, 1);
	if (ret <= 0)
		return ret ? : -ENODEV;

	ret = cpuidle_register_driver(drv);
	if (ret) {
		pr_err("Failed to register cpuidle driver\n");
		return ret;
	}
	for_each_possible_cpu(cpu) {
		dev = kzalloc(sizeof(*dev), GFP_KERNEL);
		if (!dev) {
			pr_err("Failed to allocate cpuidle device\n");
			goto out_fail;
		}
		dev->cpu = cpu;

		ret = cpuidle_register_device(dev);
		if (ret) {
			pr_err("Failed to register cpuidle device for CPU %d\n"
			       , cpu);
			kfree(dev);
			goto out_fail;
		}
	}

	return 0;
out_fail:
	while (--cpu >= 0) {
		dev = per_cpu(cpuidle_devices, cpu);
		cpuidle_unregister_device(dev);
		kfree(dev);
	}

	cpuidle_unregister_driver(drv);

	return ret;
}
device_initcall(nexell_idle_init);
