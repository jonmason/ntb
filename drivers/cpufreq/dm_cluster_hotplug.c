/*
 * Cluster hotplug driver
 *
 * Copyright (C) 2016 Samsung Ltd.
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

#include <linux/atomic.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/suspend.h>
#include <linux/pm_qos.h>

/* Booting time 90s */
#define BOOTING_TIME 900

static struct delayed_work hotplug_work;
static struct delayed_work start_hotplug;
static struct workqueue_struct *khotplug_wq;

enum hstate {
	H0,
	H1,
	H2,
	H3,
	MAX_HSTATE,
};

enum action {
	DOWN,
	UP,
	STAY,
};

struct hotplug_hstates_usage {
	unsigned long time;
};

struct hotplug_ctrl {
	ktime_t last_time;
	ktime_t last_check_time;
	unsigned int sampling_rate;
	unsigned int down_freq;
	unsigned int up_freq;
	unsigned int up_threshold;
	unsigned int down_threshold;
	unsigned int up_tasks;
	unsigned int down_tasks;
	int max_lock;
	int min_lock;
	int force_hstate;
	int cur_hstate;
	enum hstate old_state;
	bool suspended;
	struct hotplug_hstates_usage usage[MAX_HSTATE];
};

struct hotplug_hstate {
	char *name;
	unsigned int core_count;
	enum hstate state;
};

static struct hotplug_hstate hstate_set[] = {
	[H0] = {
		.name		= "H0",
		.core_count	= 8,
		.state		= H0,
	},
	[H1] = {
		.name		= "H1",
		.core_count	= 4,
		.state		= H1,
	},
	[H2] = {
		.name		= "H2",
		.core_count	= 2,
		.state		= H2,
	},
	[H3] = {
		.name		= "H3",
		.core_count	= 1,
		.state		= H3,
	},
};

static struct hotplug_ctrl ctrl_hotplug = {
	.sampling_rate = 100,		/* ms */
	.down_freq = 800000,		/* MHz */
	.up_freq = 1300000,			/* MHz */
	.up_threshold = 3,
	.down_threshold = 3,
	.up_tasks = 4,
	.down_tasks = 6,
	.force_hstate = -1,
	.min_lock = -1,
	.max_lock = -1,
	.cur_hstate = H0,
	.old_state = H0,
};

static DEFINE_MUTEX(hotplug_lock);
static DEFINE_SPINLOCK(hstate_status_lock);

static atomic_t freq_history[STAY] =  {ATOMIC_INIT(0), ATOMIC_INIT(0)};
static bool lcd_on = false;

/* check the core count */
static int get_core_count(enum hstate state)
{
	int old = ctrl_hotplug.old_state;

	/* CPU UP */
	if (ctrl_hotplug.old_state < state)
		return ((hstate_set[old].core_count) -
				(hstate_set[state].core_count));
	else
		return ((hstate_set[state].core_count) -
				(hstate_set[old].core_count));
}

static void __ref cluster_down(enum hstate state)
{
	struct device *cdev;
	int i, count, cpu;

	count = get_core_count(state);

	for (i = 0; i < count; i++) {
		cpu = num_online_cpus() - 1;
		if (cpu > 0 && cpu_online(cpu)) {
			cpu_down(cpu);
			cdev = get_cpu_device(cpu);
			cdev->offline = true;
		}
	}
}

static void __ref cluster_up(enum hstate state)
{
	struct device *cdev;
	int i, count, cpu;

	count = get_core_count(state);

	for (i = 0; i < count; i++) {
		cpu = num_online_cpus();
		if (cpu < num_possible_cpus() && !cpu_online(cpu)) {
			cpu_up(cpu);
			cdev = get_cpu_device(cpu);
			cdev->offline = false;
		}
	}
}

static s64 hotplug_update_time_status(void)
{
	ktime_t curr_time, last_time;
	s64 diff;

	curr_time = ktime_get();
	last_time = ctrl_hotplug.last_time;

	diff = ktime_to_ms(ktime_sub(curr_time, last_time));

	if (diff > INT_MAX)
		diff = INT_MAX;

	ctrl_hotplug.usage[ctrl_hotplug.old_state].time += diff;
	ctrl_hotplug.last_time = curr_time;

	return diff;
}

static void hotplug_enter_hstate(bool force, enum hstate state)
{
	int min_state, max_state;
	bool up = false;

	if (ctrl_hotplug.suspended)
		return;

	if (!force) {
		min_state = ctrl_hotplug.min_lock;
		max_state = ctrl_hotplug.max_lock;

		if (lcd_on && (state > H1))
			state = H1;

		if (min_state >= 0 && state >= min_state)
			state = min_state;

		if (max_state > 0 && state <= max_state)
			state = max_state;
	}

	if (ctrl_hotplug.old_state == state)
		return;

	if (ctrl_hotplug.old_state > state)
		up = true;

	spin_lock(&hstate_status_lock);
	hotplug_update_time_status();
	spin_unlock(&hstate_status_lock);

	if (up)
		cluster_up(state);
	else
		cluster_down(state);

	atomic_set(&freq_history[UP], 0);
	atomic_set(&freq_history[DOWN], 0);

	spin_lock(&hstate_status_lock);
	hotplug_update_time_status();
	spin_unlock(&hstate_status_lock);

	ctrl_hotplug.old_state = state;
	ctrl_hotplug.cur_hstate = state;
}

void dm_hotplug_disable(void)
{
	/* Reserved Function */
}

void dm_hotplug_enable(void)
{
	/* Reserved Function */
}

void dc_hotplug_control(int state)
{
	if (delayed_work_pending(&hotplug_work))
		cancel_delayed_work_sync(&hotplug_work);

	mutex_lock(&hotplug_lock);

	if (state == -1) {
		ctrl_hotplug.force_hstate = state;

		if (!delayed_work_pending(&hotplug_work))
			queue_delayed_work_on(0, khotplug_wq, &hotplug_work,
				 msecs_to_jiffies(ctrl_hotplug.sampling_rate));

	} else {
		if (delayed_work_pending(&hotplug_work))
			cancel_delayed_work_sync(&hotplug_work);

		if (ctrl_hotplug.old_state > state)
			hotplug_enter_hstate(true, state);
		else
			hotplug_enter_hstate(false, state);
	}

	mutex_unlock(&hotplug_lock);
}

static enum action select_up_down(void)
{
	int up_threshold, down_threshold;
	unsigned int down_freq, up_freq;
	unsigned int c0_freq, c1_freq;
	int nr;

	nr = nr_running();

	c0_freq = cpufreq_quick_get(0);	/* 0 : first cpu number for Cluster 0 */

	/* 4 : first cpu number for Cluster 1 */
	if (cpu_online(4))
		c1_freq = cpufreq_quick_get(4);
	else
		c1_freq = c0_freq;

	up_threshold = ctrl_hotplug.up_threshold;
	down_threshold = ctrl_hotplug.down_threshold;
	down_freq = ctrl_hotplug.down_freq;
	up_freq = ctrl_hotplug.up_freq;

	if (((c0_freq < up_freq) && (c0_freq > down_freq)) ||
			((c1_freq < up_freq && c1_freq > down_freq))) {
		atomic_set(&freq_history[UP], 0);
		atomic_set(&freq_history[DOWN], 0);

		return STAY;
	}

	if (((c1_freq <= down_freq) && (c0_freq <= down_freq)) &&
			(ctrl_hotplug.down_tasks >= nr)) {
		atomic_inc(&freq_history[DOWN]);
		atomic_set(&freq_history[UP], 0);
	} else if ((c0_freq >= up_freq) || (c1_freq >= up_freq)) {
		atomic_inc(&freq_history[UP]);
		atomic_set(&freq_history[DOWN], 0);
	}

	if (atomic_read(&freq_history[UP]) > up_threshold)
		return UP;
	else if (atomic_read(&freq_history[DOWN]) > down_threshold)
		return DOWN;

	return STAY;
}

static enum hstate hotplug_adjust_state(enum action move)
{
	int state, nr;

	nr = nr_running();

	if (move == DOWN) {
		state = ctrl_hotplug.old_state + 1;
		if (state >= MAX_HSTATE)
			state = MAX_HSTATE - 1;
	} else {
		if (ctrl_hotplug.old_state == H1 &&
		    ctrl_hotplug.up_tasks >= nr)
			return ctrl_hotplug.old_state;

		state = ctrl_hotplug.old_state - 1;
		if (state <= 0)
			state = H0;
	}

	return state;
}

static void start_work(struct work_struct *dwork)
{
	mutex_lock(&hotplug_lock);
	ctrl_hotplug.suspended = false;
	mutex_unlock(&hotplug_lock);

	if (ctrl_hotplug.force_hstate == -1)
		queue_delayed_work_on(0, khotplug_wq, &hotplug_work,
			msecs_to_jiffies(ctrl_hotplug.sampling_rate));
}

static void dm_hotplug_work(struct work_struct *dwork)
{
	enum action move = select_up_down();
	enum hstate target_state;

	mutex_lock(&hotplug_lock);

	target_state = hotplug_adjust_state(move);
	if (move != STAY)
		hotplug_enter_hstate(false, target_state);

	queue_delayed_work_on(0, khotplug_wq, &hotplug_work,
		msecs_to_jiffies(ctrl_hotplug.sampling_rate));
	mutex_unlock(&hotplug_lock);
}

static int fb_state_change(struct notifier_block *nb,
		unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank = evdata->data;

	if (event == FB_EVENT_BLANK) {
		switch (*blank) {
		case FB_BLANK_NORMAL:
			lcd_on = false;
			break;
		case FB_BLANK_UNBLANK:
			lcd_on = true;
			break;
		}
	}

	return NOTIFY_OK;
}

static struct notifier_block fb_block = {
	.notifier_call = fb_state_change,
};

#define define_show_state_function(_name) \
static ssize_t show_##_name(struct device *dev, struct device_attribute *attr, \
			char *buf) \
{ \
	return sprintf(buf, "%d\n", ctrl_hotplug._name); \
}

#define define_store_state_function(_name) \
static ssize_t store_##_name(struct device *dev, struct device_attribute \
			*attr, const char *buf, size_t count) \
{ \
	unsigned long value; \
	int ret; \
	ret = kstrtoul(buf, 10, &value); \
	if (ret) \
		return ret; \
	ctrl_hotplug._name = value; \
	return ret ? ret : count; \
}

define_show_state_function(up_threshold)
define_store_state_function(up_threshold)

define_show_state_function(down_threshold)
define_store_state_function(down_threshold)

define_show_state_function(sampling_rate)
define_store_state_function(sampling_rate)

define_show_state_function(down_freq)
define_store_state_function(down_freq)

define_show_state_function(up_freq)
define_store_state_function(up_freq)

define_show_state_function(up_tasks)
define_store_state_function(up_tasks)

define_show_state_function(down_tasks)
define_store_state_function(down_tasks)

define_show_state_function(min_lock)
define_show_state_function(max_lock)
define_show_state_function(cur_hstate)
define_show_state_function(force_hstate)

void __set_force_hstate(int target_state)
{
	if (target_state < 0) {
		mutex_lock(&hotplug_lock);
		ctrl_hotplug.force_hstate = -1;
		queue_delayed_work_on(0, khotplug_wq, &hotplug_work,
			msecs_to_jiffies(ctrl_hotplug.sampling_rate));
	} else {
		cancel_delayed_work_sync(&hotplug_work);

		mutex_lock(&hotplug_lock);
		hotplug_enter_hstate(true, target_state);
		ctrl_hotplug.force_hstate = target_state;
	}

	mutex_unlock(&hotplug_lock);
}

static ssize_t store_force_hstate(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret, target_state;

	ret = kstrtoint(buf, 0, &target_state);
	if (ret < 0 || target_state >= MAX_HSTATE)
		return -EINVAL;

	__set_force_hstate(target_state);

	return ret ? ret : count;
}

static void __force_hstate(int target_state, int *value)
{
	if (target_state < 0) {
		mutex_lock(&hotplug_lock);
		*value = -1;
	} else {
		cancel_delayed_work_sync(&hotplug_work);

		mutex_lock(&hotplug_lock);
		hotplug_enter_hstate(true, target_state);
		*value = target_state;
	}

	queue_delayed_work_on(0, khotplug_wq, &hotplug_work,
		msecs_to_jiffies(ctrl_hotplug.sampling_rate));

	mutex_unlock(&hotplug_lock);
}

static ssize_t store_max_lock(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int max_state;
	int state;

	int ret, target_state;

	ret = kstrtoint(buf, 0, &target_state);
	if (ret < 0 || target_state >= MAX_HSTATE)
		return -EINVAL;

	max_state = target_state;
	state = target_state;

	mutex_lock(&hotplug_lock);

	if (ctrl_hotplug.force_hstate != -1) {
		mutex_unlock(&hotplug_lock);
		return count;
	}

	if (state < 0) {
		mutex_unlock(&hotplug_lock);
		goto out;
	}

	if (ctrl_hotplug.min_lock >= 0)
		state = ctrl_hotplug.min_lock;

	if (max_state >= 0 && state <= max_state)
		state = max_state;

	if ((int)ctrl_hotplug.old_state > state) {
		ctrl_hotplug.max_lock = state;
		mutex_unlock(&hotplug_lock);
		return count;
	}

	mutex_unlock(&hotplug_lock);

out:
	__force_hstate(state, &ctrl_hotplug.max_lock);

	return ret ? ret : count;
}

static ssize_t store_min_lock(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int max_state = -1;
	int state;

	int ret, target_state;

	ret = kstrtoint(buf, 0, &target_state);
	if (ret < 0 || target_state >= MAX_HSTATE)
		return -EINVAL;

	state = target_state;

	mutex_lock(&hotplug_lock);

	if (ctrl_hotplug.force_hstate != -1) {
		mutex_unlock(&hotplug_lock);
		return count;
	}

	if (state < 0) {
		mutex_unlock(&hotplug_lock);
		goto out;
	}

	if (ctrl_hotplug.max_lock >= 0)
		max_state = ctrl_hotplug.max_lock;

	if (max_state >= 0 && state <= max_state)
		state = max_state;

	if ((int)ctrl_hotplug.old_state < state) {
		ctrl_hotplug.min_lock = state;
		mutex_unlock(&hotplug_lock);
		return count;
	}

	mutex_unlock(&hotplug_lock);

out:
	__force_hstate(state, &ctrl_hotplug.min_lock);

	return ret ? ret : count;
}

static ssize_t show_time_in_state(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	int i;

	spin_lock(&hstate_status_lock);
	hotplug_update_time_status();
	spin_unlock(&hstate_status_lock);

	for (i = 0; i < MAX_HSTATE; i++) {
		len += sprintf(buf + len, "%s %llu\n", hstate_set[i].name,
			(unsigned long long)ctrl_hotplug.usage[i].time);
	}
	return len;
}

static DEVICE_ATTR(up_threshold, S_IRUGO | S_IWUSR, show_up_threshold,
		store_up_threshold);
static DEVICE_ATTR(down_threshold, S_IRUGO | S_IWUSR, show_down_threshold,
		store_down_threshold);
static DEVICE_ATTR(sampling_rate, S_IRUGO | S_IWUSR, show_sampling_rate,
		store_sampling_rate);
static DEVICE_ATTR(down_freq, S_IRUGO | S_IWUSR, show_down_freq,
		store_down_freq);
static DEVICE_ATTR(up_freq, S_IRUGO | S_IWUSR, show_up_freq, store_up_freq);
static DEVICE_ATTR(up_tasks, S_IRUGO | S_IWUSR, show_up_tasks, store_up_tasks);
static DEVICE_ATTR(down_tasks, S_IRUGO | S_IWUSR, show_down_tasks,
		store_down_tasks);
static DEVICE_ATTR(force_hstate, S_IRUGO | S_IWUSR, show_force_hstate,
		store_force_hstate);
static DEVICE_ATTR(cur_hstate, S_IRUGO, show_cur_hstate, NULL);
static DEVICE_ATTR(min_lock, S_IRUGO | S_IWUSR, show_min_lock, store_min_lock);
static DEVICE_ATTR(max_lock, S_IRUGO | S_IWUSR, show_max_lock, store_max_lock);

static DEVICE_ATTR(time_in_state, S_IRUGO, show_time_in_state, NULL);

static struct attribute *hotplug_default_attrs[] = {
	&dev_attr_up_threshold.attr,
	&dev_attr_down_threshold.attr,
	&dev_attr_sampling_rate.attr,
	&dev_attr_down_freq.attr,
	&dev_attr_up_freq.attr,
	&dev_attr_up_tasks.attr,
	&dev_attr_down_tasks.attr,
	&dev_attr_force_hstate.attr,
	&dev_attr_cur_hstate.attr,
	&dev_attr_time_in_state.attr,
	&dev_attr_min_lock.attr,
	&dev_attr_max_lock.attr,
	NULL
};

static struct attribute_group hotplug_attr_group = {
	.attrs = hotplug_default_attrs,
	.name = "hotplug",
};

static int hotplug_pm_notify(struct notifier_block *nb, unsigned long event,
	void *dummy)
{
	mutex_lock(&hotplug_lock);
	if (event == PM_SUSPEND_PREPARE) {
		ctrl_hotplug.suspended = true;

		atomic_set(&freq_history[UP], 0);
		atomic_set(&freq_history[DOWN], 0);

		mutex_unlock(&hotplug_lock);

		cancel_delayed_work_sync(&hotplug_work);
	} else if (event == PM_POST_SUSPEND) {
		ctrl_hotplug.suspended = false;

		if (ctrl_hotplug.force_hstate == -1)
			queue_delayed_work_on(0, khotplug_wq, &hotplug_work,
				msecs_to_jiffies(ctrl_hotplug.sampling_rate));

		mutex_unlock(&hotplug_lock);
	}

	return NOTIFY_OK;
}

static struct notifier_block hotplug_cpu_pm_notifier = {
	.notifier_call = hotplug_pm_notify,
};

static int __ref cpu_online_min_qos_handler(struct notifier_block *b, unsigned
		long val, void *v)
{
	int max_state = -1;
	int state = 0;

	state = val;

	mutex_lock(&hotplug_lock);

	if (ctrl_hotplug.force_hstate != -1) {
		mutex_unlock(&hotplug_lock);
		return NOTIFY_OK;
	}

	if (state < 0) {
		mutex_unlock(&hotplug_lock);
		goto out;
	}

	if (ctrl_hotplug.max_lock >= 0)
		max_state = ctrl_hotplug.max_lock;

	if (max_state >= 0 && state <= max_state)
		state = max_state;

	if ((int)ctrl_hotplug.old_state < state) {
		ctrl_hotplug.min_lock = state;
		mutex_unlock(&hotplug_lock);
		return NOTIFY_OK;
	}

	mutex_unlock(&hotplug_lock);

out:
	__force_hstate(state, &ctrl_hotplug.min_lock);

	return NOTIFY_OK;
}

static int __ref cpu_online_max_qos_handler(struct notifier_block *b, unsigned
		long val, void *v)
{
	int max_state = -1;
	int state = 0;

	max_state = val;
	state = val;

	mutex_lock(&hotplug_lock);

	if (ctrl_hotplug.force_hstate != -1) {
		mutex_unlock(&hotplug_lock);
		return NOTIFY_OK;
	}

	if (state < 0) {
		mutex_unlock(&hotplug_lock);
		goto out;
	}

	if (ctrl_hotplug.min_lock >= 0)
		state = ctrl_hotplug.min_lock;

	if (max_state >= 0 && state <= max_state)
		state = max_state;

	if ((int)ctrl_hotplug.old_state > state) {
		ctrl_hotplug.max_lock = state;
		mutex_unlock(&hotplug_lock);
		return NOTIFY_OK;
	}

	mutex_unlock(&hotplug_lock);

out:
	__force_hstate(state, &ctrl_hotplug.max_lock);

	return NOTIFY_OK;
}

static struct notifier_block cpu_online_min_qos_notifier= {
	.notifier_call = cpu_online_min_qos_handler,
};

static struct notifier_block cpu_online_max_qos_notifier= {
	.notifier_call = cpu_online_max_qos_handler,
};

static int __init dm_cluster_hotplug_init(void)
{
	int ret = 0;

	INIT_DEFERRABLE_WORK(&hotplug_work, dm_hotplug_work);
	INIT_DEFERRABLE_WORK(&start_hotplug, start_work);

	mutex_lock(&hotplug_lock);
	ctrl_hotplug.suspended = true;
	mutex_unlock(&hotplug_lock);

	khotplug_wq = alloc_workqueue("khotplug", WQ_FREEZABLE, 0);
	if (!khotplug_wq) {
		pr_err("Failed to create khotplug workqueue\n");
		ret = -EFAULT;
		goto err_wq;
	}

	ret = sysfs_create_group(&cpu_subsys.dev_root->kobj,
			&hotplug_attr_group);
	if (ret) {
		pr_err("Failed to create sysfs for hotplug\n");
		goto err_sys;
	}

	ret = fb_register_client(&fb_block);
	if (ret) {
		pr_err("Failed to register fb notifier\n");
		goto err_fb;
	}

	ret = register_pm_notifier(&hotplug_cpu_pm_notifier);
	if (ret) {
		pr_err("Faile to register pm notifier\n");
		goto err_pm;
	}

	pm_qos_add_notifier(PM_QOS_CPU_ONLINE_MIN, &cpu_online_min_qos_notifier);
	pm_qos_add_notifier(PM_QOS_CPU_ONLINE_MAX, &cpu_online_max_qos_notifier);

	queue_delayed_work_on(0, khotplug_wq, &start_hotplug,
		msecs_to_jiffies(ctrl_hotplug.sampling_rate) * BOOTING_TIME);

	return 0;

err_pm:
	fb_unregister_client(&fb_block);
err_fb:
	sysfs_remove_group(&cpu_subsys.dev_root->kobj,
		&hotplug_attr_group);
err_sys:
	destroy_workqueue(khotplug_wq);
err_wq:

	pr_err("[HOTPLUG] %s error\n", __func__);
	return ret;
}
late_initcall(dm_cluster_hotplug_init);
