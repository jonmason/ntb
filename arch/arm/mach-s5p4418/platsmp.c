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
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <asm/cacheflush.h>
#include <linux/irqchip/arm-gic.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>

#define SCR_SEC_BOOT	0xc0010c1c
#define SCR_SIGNATURE	0xc0010868

static void __iomem *scu_base;
static void __iomem *scr_second_boot;
static void __iomem *scr_signature_reset;

extern void __secondary_startup(void);

/*
 * Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably.
 */
static void write_pen_release(int val)
{
	pen_release = val;
	smp_wmb();
	sync_cache_w(&pen_release);
}

static inline int boot_second_core(int cpu)
{
	__raw_writel(virt_to_phys(__secondary_startup), scr_second_boot);
	return 0;
}

static DEFINE_SPINLOCK(boot_lock);

static void platform_secondary_init(unsigned int cpu)
{
	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	write_pen_release(-1);

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

static int boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;
	u32 mpidr = cpu_logical_map(cpu);
	u32 core_id = MPIDR_AFFINITY_LEVEL(mpidr, 0);

	/*
	 * Set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	boot_second_core(cpu);

	/*
	 * This is really belt and braces; we hold unintended secondary
	 * CPUs in the holding pen until we're ready for them.  However,
	 * since we haven't sent them a soft interrupt, they shouldn't
	 * be there.
	 */
	write_pen_release(core_id);

	/*
	 * Send the secondary CPU a soft interrupt, thereby causing
	 * the boot monitor to read the system wide flags register,
	 * and branch to the address found there.
	 */

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if (pen_release == -1)
			break;

		udelay(10);
	}

	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);

	flush_cache_all();

	return pen_release != -1 ? -ENOSYS : 0;
}

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */

static void __init platform_smp_prepare_cpus(unsigned int max_cpus)
{
	unsigned int i, ncores;
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "arm,cortex-a9-scu");
	if (!node) {
		pr_err("%s: Missing scu\n", __func__);
		return;
	}
	scu_base = of_iomap(node, 0);
	scr_second_boot = ioremap(SCR_SEC_BOOT, 0x100);
	scr_signature_reset = ioremap(SCR_SIGNATURE, 0x100);

	scu_enable(scu_base);
	__raw_writel(virt_to_phys(__secondary_startup), scr_second_boot);
	__raw_writel((-1UL), scr_signature_reset);
	ncores = scu_get_core_count(scu_base);

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);
}

static struct smp_operations s5p4418_smp_ops __initdata = {
	.smp_prepare_cpus       = platform_smp_prepare_cpus,
	.smp_boot_secondary     = boot_secondary,
	.smp_secondary_init	= platform_secondary_init,
};
CPU_METHOD_OF_DECLARE(s5p4418_smp, "nexell,s5p4418-smp", &s5p4418_smp_ops);
