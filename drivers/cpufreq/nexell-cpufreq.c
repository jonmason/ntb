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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/sysrq.h>
#include <linux/suspend.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/cpu_cooling.h>
#include <linux/pm_qos.h>

#include <linux/soc/nexell/cpufreq.h>

#define DEV_NAME_CPUFREQ	"nexell-cpufreq"

/*
 * DVFS info
 */
struct cpufreq_asv_ops {
	int  (*setup_table)(unsigned long (*tables)[2]);
	long (*get_voltage)(long freqkhz);
	int  (*modify_vol_table)(unsigned long (*tables)[2], int table_size,
			long val, bool dn, bool percent);
	int  (*current_label)(char *string);
	long (*get_vol_margin)(long uV, long val, bool dn, bool percent);
};

#if defined(CONFIG_ARCH_S5P6818)
#include "s5p6818-cpufreq.h"
#elif defined(CONFIG_ARCH_S5P4418)
#include "s5p4418-cpufreq.h"
#else
#define	FREQ_MAX_FREQ_KHZ		(1400*1000)
#define	FREQ_ARRAY_SIZE			(11)
static struct cpufreq_asv_ops	asv_ops = { };
#endif

struct cpufreq_dvfs_timestamp {
	unsigned long start;
	unsigned long duration;
};

struct cpufreq_dvfs_info {
	struct cpufreq_frequency_table *freq_table;
	unsigned long (*dvfs_table)[2];
	struct clk *clk;
	cpumask_var_t cpus;
	struct cpufreq_policy *policy;
	int cpu;
	long target_freq;
	int  freq_point;
	struct mutex lock;
	/* voltage control */
	struct regulator *volt;
	int table_size;
	long supply_delay_us;
	/* for suspend/resume */
	struct notifier_block pm_notifier;
	unsigned long resume_state;
	long boot_frequency;
	int  boot_voltage;
	/* check frequency duration */
	int  pre_freq_point;
	unsigned long check_state;
	struct cpufreq_dvfs_timestamp *time_stamp;
	/* ASV operation */
	struct cpufreq_asv_ops *asv_ops;
};

#define	FREQ_TABLE_MAX			(30)
#define	FREQ_STATE_RESUME		(0)	/* bit num */
#define	FREQ_STATE_TIME_RUN		(0)	/* bit num */

static struct thermal_cooling_device *cdev;
static struct cpufreq_dvfs_info	*ptr_current_dvfs;
static unsigned long dvfs_freq_voltage[FREQ_TABLE_MAX][2];
static struct cpufreq_dvfs_timestamp dvfs_timestamp[FREQ_TABLE_MAX] = { {0,}, };
#define	ms_to_ktime(m)	ns_to_ktime((u64)m * 1000 * 1000)

static inline void set_dvfs_ptr(void *dvfs)	{ ptr_current_dvfs = dvfs; }
static inline void *get_dvfs_ptr(void)		{ return ptr_current_dvfs; }

static inline unsigned long cpufreq_get_voltage(struct cpufreq_dvfs_info *dvfs,
						unsigned long frequency)
{
	unsigned long (*dvfs_table)[2] = (unsigned long(*)[2])dvfs->dvfs_table;
	int i = 0;

	for (i = 0; dvfs->table_size > i; i++) {
		if (frequency == dvfs_table[i][0])
			return dvfs_table[i][1];
	}

	pr_err("Fail : invalid frequency (%ld:%d) id !!!\n",
	       frequency, dvfs->table_size);
	return -EINVAL;
}

static int nxp_cpufreq_set_freq_point(struct cpufreq_dvfs_info *dvfs,
					unsigned long frequency)
{
	unsigned long (*dvfs_tables)[2] = (unsigned long(*)[2])dvfs->dvfs_table;
	int len = dvfs->table_size;
	int id = 0;

	for (id = 0; len > id; id++)
		if (frequency == dvfs_tables[id][0])
			break;

	if (id == len) {
		pr_err("Fail : invalid frequency (%ld:%d) id !!!\n",
		       frequency, len);
		return -EINVAL;
	}

	dvfs->freq_point = id;
	return 0;
}

static long nxp_cpufreq_change_voltage(struct cpufreq_dvfs_info *dvfs,
				       unsigned long frequency)
{
	long mS = 0, uS = 0;
	long uV = 0, wT = 0;

	if (!dvfs->volt)
		return 0;

	uV = cpufreq_get_voltage(dvfs, frequency);
	wT = dvfs->supply_delay_us;

	/* when rest duration */
	if (0 > uV) {
		pr_err("%s: failed invalid freq %ld uV %ld !!!\n", __func__,
		       frequency, uV);
		return -EINVAL;
	}

	if (dvfs->asv_ops->get_voltage)
		uV = dvfs->asv_ops->get_voltage(frequency);

	regulator_set_voltage(dvfs->volt, uV, uV);

	if (wT) {
		mS = wT/1000;
		uS = wT%1000;
		if (mS)
			mdelay(mS);
		if (uS)
			udelay(uS);
	}

#ifdef CONFIG_ARM_NEXELL_CPUFREQ_VOLTAGE_DEBUG
	pr_info(" volt (%lukhz %ld.%06ld v, delay %ld.%03ld us)\n",
			frequency, uV/1000000, uV%1000000, mS, uS);
#endif
	return uV;
}

static unsigned long nxp_cpufreq_change_freq(struct cpufreq_dvfs_info *dvfs,
				unsigned int new, unsigned old)
{
	struct clk *clk = dvfs->clk;
	unsigned long rate_khz = 0;
	struct cpufreq_policy policy;

	nxp_cpufreq_set_freq_point(dvfs, new);

	if (!test_bit(FREQ_STATE_RESUME, &dvfs->resume_state))
		return old;

	/* pre voltage */
	if (new >= old)
		nxp_cpufreq_change_voltage(dvfs, new);

	if (NULL == dvfs->policy) {
		cpumask_copy(policy.cpus, cpu_online_mask);
		dvfs->policy = &policy;
	}

	clk_set_rate(clk, new*1000);
	rate_khz = clk_get_rate(clk)/1000;

#ifdef CONFIG_ARM_NEXELL_CPUFREQ_DEBUG
	pr_debug(" set rate %u:%lukhz\n", new, rate_khz);
#endif

	if (test_bit(FREQ_STATE_TIME_RUN, &dvfs->check_state)) {
		int id = dvfs->freq_point;
		int prev = dvfs->pre_freq_point;
		long ms = ktime_to_ms(ktime_get());

		dvfs->time_stamp[prev].duration +=
			(ms - dvfs->time_stamp[prev].start);
		dvfs->time_stamp[id].start = ms;
		dvfs->pre_freq_point = id;
	}

	/* post voltage */
	if (old > new)
		nxp_cpufreq_change_voltage(dvfs, new);

	return rate_khz;
}

static int nxp_cpufreq_pm_notify(struct notifier_block *this,
				 unsigned long mode, void *unused)
{
	struct cpufreq_dvfs_info *dvfs = container_of(this,
						      struct cpufreq_dvfs_info,
						      pm_notifier);
	struct clk *clk = dvfs->clk;
	unsigned int old, new;
	long max_freq = cpufreq_quick_get_max(dvfs->cpu);

	switch (mode) {
	case PM_SUSPEND_PREPARE:	/* set initial frequecny */
		mutex_lock(&dvfs->lock);

		new = dvfs->boot_frequency;
		if (new > max_freq) {
			new = max_freq;
			pr_info("DVFS: max freq %ldkhz less than boot %ldkz.\n",
				max_freq, dvfs->boot_frequency);
		}
		old = clk_get_rate(clk)/1000;

		dvfs->target_freq = new;
		nxp_cpufreq_change_freq(dvfs, new, old);

		clear_bit(FREQ_STATE_RESUME, &dvfs->resume_state);
		mutex_unlock(&dvfs->lock);
		break;

	case PM_POST_SUSPEND:	/* set restore frequecny */
		mutex_lock(&dvfs->lock);
		set_bit(FREQ_STATE_RESUME, &dvfs->resume_state);

		new = dvfs->target_freq;
		old = clk_get_rate(clk)/1000;
		nxp_cpufreq_change_freq(dvfs, new, old);

		mutex_unlock(&dvfs->lock);
		break;
	}
	return 0;
}

/*
 * Attribute sys interfaces
 */
static ssize_t show_speed_duration(struct cpufreq_policy *policy, char *buf)
{
	struct cpufreq_dvfs_info *dvfs = get_dvfs_ptr();
	int id = dvfs->freq_point;
	ssize_t count = 0;
	int i = 0;

	if (test_bit(FREQ_STATE_TIME_RUN, &dvfs->check_state)) {
		long ms = ktime_to_ms(ktime_get());

		if (dvfs->time_stamp[id].start)
			dvfs->time_stamp[id].duration +=
				(ms - dvfs->time_stamp[id].start);
		dvfs->time_stamp[id].start = ms;
		dvfs->pre_freq_point = id;
	}

	for (; dvfs->table_size > i; i++)
		count += sprintf(&buf[count], "%8ld ",
				 dvfs->time_stamp[i].duration);

	count += sprintf(&buf[count], "\n");
	return count;
}

static ssize_t store_speed_duration(struct cpufreq_policy *policy,
			const char *buf, size_t count)
{
	struct cpufreq_dvfs_info *dvfs = get_dvfs_ptr();
	int id = dvfs->freq_point;
	long ms = ktime_to_ms(ktime_get());
	const char *s = buf;

	mutex_lock(&dvfs->lock);

	if (0 == strncmp(s, "run", strlen("run"))) {
		dvfs->pre_freq_point = id;
		dvfs->time_stamp[id].start = ms;
		set_bit(FREQ_STATE_TIME_RUN, &dvfs->check_state);
	} else if (0 == strncmp(s, "stop", strlen("stop"))) {
		clear_bit(FREQ_STATE_TIME_RUN, &dvfs->check_state);
	} else if (0 == strncmp(s, "clear", strlen("clear"))) {
		memset(dvfs->time_stamp, 0, sizeof(dvfs_timestamp));
		if (test_bit(FREQ_STATE_TIME_RUN, &dvfs->check_state)) {
			dvfs->time_stamp[id].start = ms;
			dvfs->pre_freq_point = id;
		}
	} else {
		count = -1;
	}

	mutex_unlock(&dvfs->lock);

	return count;
}

static ssize_t show_available_voltages(struct cpufreq_policy *policy, char *buf)
{
	struct cpufreq_dvfs_info *dvfs = get_dvfs_ptr();
	unsigned long (*dvfs_table)[2] = (unsigned long(*)[2])dvfs->dvfs_table;
	ssize_t count = 0;
	int i = 0;

	for (; dvfs->table_size > i; i++) {
		long uV = dvfs_table[i][1];

		if (dvfs->asv_ops->get_voltage)
			uV = dvfs->asv_ops->get_voltage(dvfs_table[i][0]);
		count += sprintf(&buf[count], "%ld ", uV);
	}

	count += sprintf(&buf[count], "\n");
	return count;
}

static ssize_t show_cur_voltages(struct cpufreq_policy *policy, char *buf)
{
	struct cpufreq_dvfs_info *dvfs = get_dvfs_ptr();
	unsigned long (*dvfs_table)[2] = (unsigned long(*)[2])dvfs->dvfs_table;
	ssize_t count = 0;
	int i = 0;

	for (; dvfs->table_size > i; i++)
		count += sprintf(&buf[count], "%ld ", dvfs_table[i][1]);

	count += sprintf(&buf[count], "\n");
	return count;
}

static ssize_t store_cur_voltages(struct cpufreq_policy *policy,
			const char *buf, size_t count)
{
	struct cpufreq_dvfs_info *dvfs = get_dvfs_ptr();
	unsigned long (*dvfs_tables)[2] =
		(unsigned long(*)[2])dvfs_freq_voltage;
	bool percent = false, down = false;
	const char *s = strchr(buf, '-');
	long val;

	if (s)
		down = true;
	else
		s = strchr(buf, '+');

	if (!s)
		s = buf;
	else
		s++;

	if (strchr(buf, '%'))
		percent = 1;

	val = simple_strtol(s, NULL, 10);

	mutex_lock(&dvfs->lock);

	if (dvfs->asv_ops->modify_vol_table)
		dvfs->asv_ops->modify_vol_table(dvfs_tables, dvfs->table_size,
							val, down, percent);

	nxp_cpufreq_change_voltage(dvfs, dvfs->target_freq);

	mutex_unlock(&dvfs->lock);
	return count;
}

static ssize_t show_asv_level(struct cpufreq_policy *policy, char *buf)
{
	struct cpufreq_dvfs_info *dvfs = get_dvfs_ptr();
	int ret = 0;

	if (dvfs->asv_ops->current_label)
		ret = dvfs->asv_ops->current_label(buf);

	return ret;
}

/*
 * show/store frequency duration time status
 */
static struct freq_attr cpufreq_freq_attr_scaling_speed_duration = {
	.attr = {
		.name = "scaling_speed_duration",
		.mode = 0664,
	},
	.show  = show_speed_duration,
	.store = store_speed_duration,
};

/*
 * show available voltages each frequency
 */
static struct freq_attr cpufreq_freq_attr_scaling_available_voltages = {
	.attr = {
		.name = "scaling_available_voltages",
		.mode = 0664,
	},
	.show  = show_available_voltages,
};

/*
 * show/store ASV current voltage adjust margin
 */
static struct freq_attr cpufreq_freq_attr_scaling_cur_voltages = {
	.attr = {
		.name = "scaling_cur_voltages",
		.mode = 0664,
	},
	.show  = show_cur_voltages,
	.store = store_cur_voltages,
};

/*
 * show ASV level status
 */
static struct freq_attr cpufreq_freq_attr_scaling_asv_level = {
	.attr = {
		.name = "scaling_asv_level",
		.mode = 0664,
	},
	.show  = show_asv_level,
};

static struct freq_attr *nxp_cpufreq_attr[] = {
	/* kernel attribute */
	&cpufreq_freq_attr_scaling_available_freqs,
	/* new sttribute */
	&cpufreq_freq_attr_scaling_speed_duration,
	&cpufreq_freq_attr_scaling_available_voltages,
	&cpufreq_freq_attr_scaling_cur_voltages,
	&cpufreq_freq_attr_scaling_asv_level,
	NULL,
};

static int nxp_cpufreq_verify_speed(struct cpufreq_policy *policy)
{
	struct cpufreq_dvfs_info *dvfs = get_dvfs_ptr();
	struct cpufreq_frequency_table *freq_table = dvfs->freq_table;

	if (!freq_table)
		return -EINVAL;

	return cpufreq_frequency_table_verify(policy, freq_table);
}

static unsigned int nxp_cpufreq_get_speed(unsigned int cpu)
{
	struct cpufreq_dvfs_info *dvfs = get_dvfs_ptr();
	struct clk *clk = dvfs->clk;
	long rate_khz = clk_get_rate(clk)/1000;

	return rate_khz;
}

static int nxp_cpufreq_target(struct cpufreq_policy *policy,
				unsigned int index)
{
	struct cpufreq_dvfs_info *dvfs = get_dvfs_ptr();
	struct cpufreq_frequency_table *freq_table = dvfs->freq_table;
	unsigned long rate_khz = 0;
	unsigned int old, new;
	int ret = 0;

	old = policy->cur;
	new = freq_table[index].frequency;

	new = max((unsigned int)pm_qos_request(PM_QOS_CPU_FREQ_MIN), new);
	new = min((unsigned int)pm_qos_request(PM_QOS_CPU_FREQ_MAX), new);

	mutex_lock(&dvfs->lock);

	pr_debug("cpufreq : target %u -> %u khz", old, new);

	if (old == new && policy->cur == new) {
		pr_debug("PASS\n");
		mutex_unlock(&dvfs->lock);
		return ret;
	}

	dvfs->target_freq = new;

	pr_debug("\n");

	dvfs->policy = policy;

	rate_khz = nxp_cpufreq_change_freq(dvfs, new, old);

	policy->cur = rate_khz;

	mutex_unlock(&dvfs->lock);

	return ret;
}

static int nxp_cpufreq_init(struct cpufreq_policy *policy)
{
	struct cpufreq_dvfs_info *dvfs = get_dvfs_ptr();
	struct cpufreq_frequency_table *freq_table = dvfs->freq_table;

	pr_debug("nxp-cpufreq: freq table 0x%p\n", freq_table);
	return cpufreq_generic_init(policy, freq_table, 100000);
}

static struct cpufreq_driver nxp_cpufreq_driver = {
	.flags   = CPUFREQ_STICKY | CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify  = nxp_cpufreq_verify_speed,
	.target_index  = nxp_cpufreq_target,
	.get     = nxp_cpufreq_get_speed,
	.init    = nxp_cpufreq_init,
	.name    = "nxp-cpufreq",
	.attr    = nxp_cpufreq_attr,
};

#ifdef CONFIG_OF
static unsigned long dt_dvfs_table[FREQ_TABLE_MAX][2];

struct nxp_cpufreq_plat_data dt_cpufreq_data = {
	.pll_dev = CONFIG_NEXELL_CPUFREQ_PLLDEV,
	.dvfs_table = dt_dvfs_table,
};

static const struct of_device_id dvfs_dt_match[] = {
	{
	.compatible = "nexell,s5pxx18-cpufreq",
	.data = (void *)&dt_cpufreq_data,
	}, {},
};
MODULE_DEVICE_TABLE(of, dvfs_dt_match);

#define	FN_SIZE		4
static void *nxp_cpufreq_get_dt_data(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	const struct of_device_id *match;
	struct nxp_cpufreq_plat_data *pdata;
	unsigned long (*plat_tbs)[2] = NULL;
	const __be32 *list;
	char *supply;
	int value, i, size = 0;

	match = of_match_node(dvfs_dt_match, node);
	if (!match)
		return NULL;

	pdata = (struct nxp_cpufreq_plat_data *)match->data;
	plat_tbs = (unsigned long(*)[2])pdata->dvfs_table;

	if (!of_property_read_string(node, "supply_name",
				     (const char **)&supply)) {
		pdata->supply_name = supply;
		if (!of_property_read_u32(node, "supply_delay_us", &value))
			pdata->supply_delay_us = value;
		pr_info("voltage supply : %s\n", pdata->supply_name);
	}

	list = of_get_property(node, "dvfs-tables", &size);
	size /= FN_SIZE;

	if (size) {
		for (i = 0; size/2 > i; i++) {
			plat_tbs[i][0] = be32_to_cpu(*list++);
			plat_tbs[i][1] = be32_to_cpu(*list++);
			pr_debug("DTS %2d = %8ldkhz, %8ld uV\n",
				 i, plat_tbs[i][0], plat_tbs[i][1]);
		}
		pdata->table_size = size/2;
	}

	return pdata;
}
#else
#define dvfs_dt_match NULL
#endif

static void *nxp_cpufreq_make_table(struct platform_device *pdev,
				    int *table_size,
				    unsigned long (*dvfs_tables)[2])
{
	struct nxp_cpufreq_plat_data *pdata = pdev->dev.platform_data;
	struct cpufreq_frequency_table *freq_table;
	struct cpufreq_asv_ops *ops = &asv_ops;
	unsigned long (*plat_tbs)[2] = NULL;
	int tb_size, asv_size = 0;
	int id = 0, n = 0;

	/* user defined dvfs */
	if (pdata->dvfs_table && pdata->table_size)
		plat_tbs = (unsigned long(*)[2])pdata->dvfs_table;

	/* asv dvfs tables */
	if (ops->setup_table)
		asv_size = ops->setup_table(dvfs_tables);

	if (!pdata->table_size && !asv_size) {
		dev_err(&pdev->dev, "failed no freq table !!!\n");
		return NULL;
	}

	tb_size = (pdata->table_size ? pdata->table_size : asv_size);

	/* alloc with end table */
	freq_table = kzalloc((sizeof(*freq_table) * (tb_size + 1)), GFP_KERNEL);
	if (!freq_table) {
		dev_warn(&pdev->dev, "failed cllocate freq table!!!\n");
		return NULL;
	}

	/* make frequency table with platform data */
	if (asv_size > 0) {
		for (n = 0, id = 0; tb_size > id && asv_size > n; n++) {
			if (plat_tbs) {
				for (n = 0; asv_size > n; n++) {
					if (plat_tbs[id][0] ==
					    dvfs_tables[n][0]) {
						dvfs_tables[id][0] =
							dvfs_tables[n][0];
						dvfs_tables[id][1] =
							dvfs_tables[n][1];
						break;
					}
				}
			} else {
				if (dvfs_tables[n][0] > FREQ_MAX_FREQ_KHZ)
					continue;
				dvfs_tables[id][0] = dvfs_tables[n][0];
				dvfs_tables[id][1] = dvfs_tables[n][1];
			}

			freq_table[id].frequency = dvfs_tables[id][0];
			pr_info("ASV %2d = %8ldkhz, %8ld uV\n",
			       id, dvfs_tables[id][0], dvfs_tables[id][1]);
			/* next */
			id++;
		}
	} else {
		for (id = 0; tb_size > id; id++) {
			dvfs_tables[id][0] = plat_tbs[id][0];
			dvfs_tables[id][1] = plat_tbs[id][1];
			freq_table[id].frequency = dvfs_tables[id][0];
			pr_info("DTB %2d = %8ldkhz, %8ld uV\n",
			       id, dvfs_tables[id][0], dvfs_tables[id][1]);
		}
	}

	/* End table */
	freq_table[id].frequency = CPUFREQ_TABLE_END;
	*table_size = id;

	return (void *)freq_table;
}

static int nxp_cpufreq_set_supply(struct platform_device *pdev,
				  struct cpufreq_dvfs_info *dvfs)
{
	struct nxp_cpufreq_plat_data *pdata = pdev->dev.platform_data;
	static struct notifier_block *pm_notifier;

	/* get voltage regulator */
	dvfs->volt = regulator_get(&pdev->dev, pdata->supply_name);
	if (IS_ERR(dvfs->volt)) {
		dev_err(&pdev->dev, "Cannot get regulator for DVS supply %s\n",
				pdata->supply_name);
		return -1;
	}

	pm_notifier = &dvfs->pm_notifier;
	pm_notifier->notifier_call = nxp_cpufreq_pm_notify;
	if (register_pm_notifier(pm_notifier)) {
		dev_err(&pdev->dev, "Cannot pm notifier %s\n",
			pdata->supply_name);
		return -1;
	}

	/* bootup voltage */
	nxp_cpufreq_change_voltage(dvfs, dvfs->boot_frequency);
	dvfs->boot_voltage = regulator_get_voltage(dvfs->volt);

	pr_info("DVFS: regulator %s\n", pdata->supply_name);
	return 0;
}

static int cpufreq_min_qos_handler(struct notifier_block *b,
		unsigned long val, void *v)
{
	struct cpufreq_policy *policy;
	int ret;

	policy = cpufreq_cpu_get(0);

	if (!policy)
		goto bad;

	if (policy->cur >= val) {
		cpufreq_cpu_put(policy);
		goto good;
	}

	if (!policy->governor) {
		cpufreq_cpu_put(policy);
		goto bad;
	}

	ret = __cpufreq_driver_target(policy, val, CPUFREQ_RELATION_L);

	cpufreq_cpu_put(policy);

	if (ret < 0)
	    goto bad;

good:
	return NOTIFY_OK;
bad:
	return NOTIFY_BAD;
}

static int cpufreq_max_qos_handler(struct notifier_block *b,
		unsigned long val, void *v)
{
	struct cpufreq_policy *policy;
	int ret;

	policy = cpufreq_cpu_get(0);

	if (!policy)
		goto bad;

	if (policy->cur <= val) {
		cpufreq_cpu_put(policy);
		goto good;
	}

	if (!policy->governor) {
		cpufreq_cpu_put(policy);
		goto bad;
	}

	ret = __cpufreq_driver_target(policy, val, CPUFREQ_RELATION_H);

	cpufreq_cpu_put(policy);

	if (ret < 0)
		goto bad;

good:
	return NOTIFY_OK;
bad:
	return NOTIFY_BAD;
}

static struct notifier_block cpufreq_min_qos_notifier = {
	.notifier_call = cpufreq_min_qos_handler,
};

static struct notifier_block cpufreq_max_qos_notifier = {
	.notifier_call = cpufreq_max_qos_handler,
};

static int nxp_cpufreq_probe(struct platform_device *pdev)
{
	struct nxp_cpufreq_plat_data *pdata = pdev->dev.platform_data;
	struct device_node *cpu0;
	unsigned long (*dvfs_tables)[2] =
		(unsigned long(*)[2])dvfs_freq_voltage;
	struct cpufreq_dvfs_info *dvfs = NULL;
	struct cpufreq_frequency_table *freq_table = NULL;
	int cpu = raw_smp_processor_id();
	char name[16];
	int table_size = 0, ret = 0;

	dvfs = kzalloc(sizeof(*dvfs), GFP_KERNEL);
	if (!dvfs) {
		dev_err(&pdev->dev, "failed allocate DVFS data !!!\n");
		return -ENOMEM;
	}

#ifdef CONFIG_OF
	if (pdev->dev.of_node) {
		pdata = nxp_cpufreq_get_dt_data(pdev);
		if (!pdata)
			goto err_free_table;
		pdev->dev.platform_data = pdata;
	}
#endif

	freq_table = nxp_cpufreq_make_table(pdev, &table_size, dvfs_tables);
	if (!freq_table)
		goto err_free_table;

	sprintf(name, "pll%d", pdata->pll_dev);
	dvfs->clk = clk_get(NULL, name);
	if (IS_ERR(dvfs->clk))
		goto err_free_table;

	set_dvfs_ptr(dvfs);
	mutex_init(&dvfs->lock);

	dvfs->asv_ops = &asv_ops;
	dvfs->freq_table = freq_table;
	dvfs->dvfs_table = (unsigned long(*)[2])(dvfs_tables);
	dvfs->table_size = table_size;
	dvfs->supply_delay_us = pdata->supply_delay_us;
	dvfs->boot_frequency = nxp_cpufreq_get_speed(cpu);
	dvfs->target_freq = dvfs->boot_frequency;
	dvfs->pre_freq_point = -1;
	dvfs->check_state = 0;
	dvfs->time_stamp = dvfs_timestamp;

	set_bit(FREQ_STATE_RESUME, &dvfs->resume_state);
	nxp_cpufreq_set_freq_point(dvfs, dvfs->target_freq);

	if (pdata->supply_name) {
		ret = nxp_cpufreq_set_supply(pdev, dvfs);
		if (0 > ret)
			goto err_free_table;
	}

	pr_info("DVFS: cpu %s with PLL.%d [tables=%d]\n",
		dvfs->volt?"DVFS":"DFS", pdata->pll_dev, dvfs->table_size);

	ret = cpufreq_register_driver(&nxp_cpufreq_driver);
	if (ret) {
		pr_err("Fial registet cpufreq driver!!\n");
		goto err_free_table;
	}

	cpu0 = of_get_cpu_node(0, NULL);
	if (!cpu0) {
		pr_err("failed to find cpu0 node\n");
		return 0;
	}

	if (of_find_property(cpu0, "#cooling-cells", NULL)) {
		cdev = of_cpufreq_cooling_register(cpu0,
						   cpu_present_mask);
		if (IS_ERR(cdev))
			pr_err("running cpufreq without cooling device: %ld\n",
			       PTR_ERR(cdev));
	}

	pm_qos_add_notifier(PM_QOS_CPU_FREQ_MIN, &cpufreq_min_qos_notifier);
	pm_qos_add_notifier(PM_QOS_CPU_FREQ_MAX, &cpufreq_max_qos_notifier);

	return 0;

err_free_table:
	if (dvfs)
		kfree(dvfs);

	if (freq_table)
		kfree(freq_table);

	return ret;
}

static struct platform_driver cpufreq_driver = {
	.probe	= nxp_cpufreq_probe,
	.driver	= {
		.name	= DEV_NAME_CPUFREQ,
		.owner	= THIS_MODULE,
		.of_match_table	= of_match_ptr(dvfs_dt_match),
	},
};
module_platform_driver(cpufreq_driver);
