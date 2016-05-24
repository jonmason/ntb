/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
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
#include <linux/devfreq.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/pm_qos.h>
#include <linux/clk.h>
#include <linux/atomic.h>
#include <linux/of.h>
#include <linux/devfreq.h>
#include <linux/soc/nexell/cpufreq.h>

#include "governor.h"

#define KHZ			1000

struct nx_devfreq {
	struct devfreq *devfreq;
	struct devfreq_notifier_block nb;
	int pm_qos_class;
	struct clk *bclk;
	atomic_t req_freq;
	atomic_t cur_freq;
	u32 pll;
	struct device *dev;
	atomic_t usage_cnt;
};

struct bus_opp_table {
	int index;
	unsigned long clk;
};

static struct nx_devfreq *_nx_devfreq;

static struct bus_opp_table bus_opp_table[] = {
	{0, NX_BUS_CLK_HIGH_KHZ},
	{1, NX_BUS_CLK_MID_KHZ},
	{2, NX_BUS_CLK_LOW_KHZ},
	{0, 0},
};

static struct pm_qos_request nx_bus_qos;

/* interface function for update qos */
void nx_bus_qos_update(int val)
{
	pm_qos_update_request(&nx_bus_qos, val);
}
EXPORT_SYMBOL(nx_bus_qos_update);

/* soc specific */
struct pll_pms {
	unsigned long rate;
	u32 P;
	u32 M;
	u32 S;
};

static struct pll_pms pll0_1_pms[] = {
	[0] = { .rate =  NX_BUS_CLK_HIGH_KHZ, .P = 3, .M = 200, .S = 2, },
	[1] = { .rate =  NX_BUS_CLK_MID_KHZ,  .P = 3, .M = 260, .S = 3, },
	[2] = { .rate =  NX_BUS_CLK_LOW_KHZ,  .P = 3, .M = 210, .S = 3, },
};

static struct pll_pms pll2_3_pms[] = {
	[0] = { .rate =  NX_BUS_CLK_HIGH_KHZ, .P = 3, .M = 200, .S = 2, },
	[1] = { .rate =  NX_BUS_CLK_MID_KHZ,  .P = 3, .M = 200, .S = 3, },
	[2] = { .rate =  NX_BUS_CLK_LOW_KHZ,  .P = 3, .M = 250, .S = 4, },
};

static int get_pll_data(u32 pll, unsigned long rate, u32 *pll_data)
{
	struct pll_pms *p;
	int len;
	int i;
	unsigned long freq = 0;

	switch (pll) {
	case 0:
	case 1:
		p = &pll0_1_pms[0];
		len = ARRAY_SIZE(pll0_1_pms);
		break;
	case 2:
	case 3:
		p = &pll2_3_pms[0];
		len = ARRAY_SIZE(pll2_3_pms);
		break;
	}

	for (i = 0; i < len; i++) {
		freq = p->rate;
		if (freq == rate)
			break;
		p++;
	}

	if (freq) {
		*pll_data = (p->P << 24) | (p->M << 8) | (p->S << 2) | pll;
		return 0;
	}

	return -EINVAL;
}

static int change_bus_freq(u32 pll_data)
{
	uint32_t pll_num = pll_data & 0x00000003;
	uint32_t s       = (pll_data & 0x000000fc) >> 2;
	uint32_t m       = (pll_data & 0x00ffff00) >> 8;
	uint32_t p       = (pll_data & 0xff000000) >> 24;

	nx_pll_set_rate(pll_num, p, m, s);

	return 0;
}

/* profile */
static int nx_devfreq_target(struct device *dev, unsigned long *freq, u32 flags)
{
	int err;
	u32 pll_data;
	struct nx_devfreq *nx_devfreq = dev_get_drvdata(dev);
	struct dev_pm_opp *opp;
	unsigned long rate = *freq * KHZ;

	rcu_read_lock();
	opp = devfreq_recommended_opp(dev, freq, flags);
	if (IS_ERR(opp)) {
		rcu_read_unlock();
		dev_err(dev, "failed to find opp for %lu KHz\n", *freq);
		return PTR_ERR(opp);
	}
	rate = dev_pm_opp_get_freq(opp);
	rcu_read_unlock();

	dev_info(dev, "freq: %lu KHz, rate: %lu\n", *freq, rate);

	if (atomic_read(&nx_devfreq->cur_freq) == *freq)
		return 0;

	err = get_pll_data(nx_devfreq->pll, *freq, &pll_data);
	if (err) {
		dev_err(dev, "failed to get pll data of freq %lu KHz\n", *freq);
		return err;
	}

	err = change_bus_freq(pll_data);
	if (err) {
		dev_err(dev, "failed to change bus clock for %lu KHz\n", *freq);
		return err;
	}

	atomic_set(&nx_devfreq->cur_freq, *freq);
	return 0;
}

static int nx_devfreq_get_dev_status(struct device *dev,
				     struct devfreq_dev_status *stat)
{
	struct nx_devfreq *nx_devfreq = dev_get_drvdata(dev);

	stat->current_frequency = atomic_read(&nx_devfreq->cur_freq);
	stat->private_data = nx_devfreq;

	return 0;
}

static struct devfreq_dev_profile nx_devfreq_profile = {
	.target = nx_devfreq_target,
	.get_dev_status = nx_devfreq_get_dev_status,
};

/* notifier */
static int nx_devfreq_pm_qos_notifier(struct notifier_block *nb,
				      unsigned long val,
				      void *v)
{
	struct devfreq_notifier_block *devfreq_nb;
	struct nx_devfreq *nx_devfreq;
	u32 cur_freq;
	bool changed = false;

	devfreq_nb = container_of(nb, struct devfreq_notifier_block, nb);
	nx_devfreq = devfreq_nb->df->data;

	dev_info(nx_devfreq->dev, "%s: val --> %ld\n", __func__, val);
	cur_freq = atomic_read(&nx_devfreq->cur_freq);

	if (val > cur_freq) {
		cur_freq = val;
		atomic_inc(&nx_devfreq->usage_cnt);
		changed = true;
	} else if (val < cur_freq) {
		if (atomic_read(&nx_devfreq->usage_cnt) > 0)
			atomic_dec(&nx_devfreq->usage_cnt);
		if (atomic_read(&nx_devfreq->usage_cnt)) {
			cur_freq = val;
			changed = true;
		}
	}

	if (changed) {
		atomic_set(&nx_devfreq->req_freq, cur_freq);
		mutex_lock(&devfreq_nb->df->lock);
		update_devfreq(devfreq_nb->df);
		mutex_unlock(&devfreq_nb->df->lock);
	}

	return NOTIFY_OK;
}

static int nx_devfreq_register_notifier(struct devfreq *devfreq)
{
	struct nx_devfreq *nx_devfreq;

	nx_devfreq = devfreq->data;

	dev_dbg(nx_devfreq->dev, "%s: E\n", __func__);
	nx_devfreq->nb.df = devfreq;
	nx_devfreq->nb.nb.notifier_call = nx_devfreq_pm_qos_notifier;

	return pm_qos_add_notifier(nx_devfreq->pm_qos_class,
				   &nx_devfreq->nb.nb);
}

static int nx_devfreq_unregister_notifier(struct devfreq *devfreq)
{
	struct nx_devfreq *nx_devfreq = devfreq->data;

	dev_dbg(nx_devfreq->dev, "%s: E\n", __func__);
	return pm_qos_remove_notifier(nx_devfreq->pm_qos_class,
				      &nx_devfreq->nb.nb);
}

/* governor */
static int nx_governor_get_target(struct devfreq *devfreq, unsigned long *freq)
{
	struct devfreq_dev_status stat;
	struct nx_devfreq *nx_devfreq;
	int err;

	err = devfreq->profile->get_dev_status(devfreq->dev.parent, &stat);
	if (err)
		return err;

	nx_devfreq = stat.private_data;
	*freq = atomic_read(&nx_devfreq->req_freq);

	if (*freq == atomic_read(&nx_devfreq->cur_freq)) {
		if (devfreq->min_freq && *freq != devfreq->min_freq)
			*freq = devfreq->min_freq;
		else if (devfreq->max_freq && *freq != devfreq->max_freq)
			*freq = devfreq->max_freq;
	}

	return 0;
}

static int nx_governor_event_handler(struct devfreq *devfreq,
				     unsigned int event, void *data)
{
	int ret;

	switch (event) {
	case DEVFREQ_GOV_START:
		ret = nx_devfreq_register_notifier(devfreq);
		if (ret)
			return ret;
		devfreq_monitor_start(devfreq);
		break;

	case DEVFREQ_GOV_STOP:
		devfreq_monitor_stop(devfreq);
		ret = nx_devfreq_unregister_notifier(devfreq);
		if (ret)
			return ret;
		break;

	case DEVFREQ_GOV_SUSPEND:
		devfreq_monitor_suspend(devfreq);
		break;

	case DEVFREQ_GOV_RESUME:
		devfreq_monitor_resume(devfreq);
		break;
	}

	return 0;
}

static struct devfreq_governor nx_devfreq_governor = {
	.name = "nx_devfreq_gov",
	.get_target_freq = nx_governor_get_target,
	.event_handler = nx_governor_event_handler,
};

/* util function */
static int nx_devfreq_parse_dt(struct device *dev,
			       struct nx_devfreq *nx_devfreq)
{
	struct device_node *np = dev->of_node;

	if (of_property_read_u32(np, "pll", &nx_devfreq->pll)) {
		dev_err(dev, "failed to get dt pll number\n");
		return -EINVAL;
	}

	return 0;
}

/* platform driver interface */
static int nx_devfreq_probe(struct platform_device *pdev)
{
	struct nx_devfreq *nx_devfreq;
	struct bus_opp_table *entry;

	nx_devfreq = devm_kzalloc(&pdev->dev, sizeof(*nx_devfreq), GFP_KERNEL);
	if (!nx_devfreq)
		return -ENOMEM;

	if (nx_devfreq_parse_dt(&pdev->dev, nx_devfreq))
		return -EINVAL;

	nx_devfreq->bclk = devm_clk_get(&pdev->dev, "bclk");
	if (IS_ERR(nx_devfreq->bclk)) {
		dev_err(&pdev->dev, "failed to get bus clock\n");
		return PTR_ERR(nx_devfreq->bclk);
	}

	atomic_set(&nx_devfreq->cur_freq, clk_get_rate(nx_devfreq->bclk) / KHZ);
	dev_info(&pdev->dev, "Current bus clock rate: %dKHz\n",
		 atomic_read(&nx_devfreq->cur_freq));

	atomic_set(&nx_devfreq->usage_cnt, 0);

	entry = &bus_opp_table[0];
	while (entry->clk != 0) {
		dev_pm_opp_add(&pdev->dev, entry->clk, 0);
		entry++;
	}

	nx_devfreq->pm_qos_class = PM_QOS_BUS_THROUGHPUT;
	nx_devfreq_profile.initial_freq = NX_BUS_CLK_LOW_KHZ;
	nx_devfreq->devfreq = devm_devfreq_add_device(&pdev->dev,
						      &nx_devfreq_profile,
						      "nx_devfreq_gov",
						      nx_devfreq);

	platform_set_drvdata(pdev, nx_devfreq);
	nx_devfreq->dev = &pdev->dev;
	_nx_devfreq = nx_devfreq;

	pm_qos_add_request(&nx_bus_qos, PM_QOS_BUS_THROUGHPUT,
			   nx_devfreq_profile.initial_freq);
	/* FIXME: after testing end, default bus clock must set to
	 * initial_freq(210000KHz)
	 */
#if 0
	pm_qos_update_request_timeout(&nx_bus_qos, NX_BUS_CLK_HIGH_KHZ,
				      60 * 1000 * 1000);
#else
	pm_qos_update_request(&nx_bus_qos, NX_BUS_CLK_HIGH_KHZ);
#endif

	return 0;
}

static int nx_devfreq_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id nx_devfreq_of_match[] = {
	{ .compatible = "nexell,s5pxx18-devfreq" },
	{ },
};

MODULE_DEVICE_TABLE(of, nx_devfreq_of_match);

static struct platform_driver nx_devfreq_driver = {
	.probe = nx_devfreq_probe,
	.remove = nx_devfreq_remove,
	.driver = {
		.name = "nx-devfreq",
		.of_match_table = nx_devfreq_of_match,
	},
};

static int __init nx_devfreq_init(void)
{
	int ret;

	ret = devfreq_add_governor(&nx_devfreq_governor);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&nx_devfreq_driver);
	if (ret)
		devfreq_remove_governor(&nx_devfreq_governor);

	return ret;
}
module_init(nx_devfreq_init);

static void __exit nx_devfreq_exit(void)
{
	int ret;

	platform_driver_unregister(&nx_devfreq_driver);

	ret = devfreq_remove_governor(&nx_devfreq_governor);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);
}
module_exit(nx_devfreq_exit);

MODULE_AUTHOR("SungwooPark <swpark@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell S5Pxx18 series SoC devfreq driver");
MODULE_LICENSE("GPL v2");
