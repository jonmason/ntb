/*
 * nxe2000.c  --  PMIC driver for the nxe2000
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

/*#define DEBUG			1*/
/*#define VERBOSE_DEBUG		1*/
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/mfd/core.h>
#include <linux/mfd/nxe2000.h>

static struct i2c_client *nxe2000_i2c_client;

static inline int __nxe2000_read(struct i2c_client *client, u8 reg,
				 uint8_t *val)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading at 0x%02x\n", reg);
		return ret;
	}

	*val = (uint8_t)ret;
	dev_dbg(&client->dev, "nxe2000: reg read  reg=%x, val=%x\n", reg, *val);
	return 0;
}

static inline int __nxe2000_bulk_reads(struct i2c_client *client, u8 reg,
				       int len, uint8_t *val)
{
	int ret;
	int i;

	ret = i2c_smbus_read_i2c_block_data(client, reg, len, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading from 0x%02x\n", reg);
		return ret;
	}
	for (i = 0; i < len; ++i) {
		dev_dbg(&client->dev, "nxe2000: reg read  reg=%x, val=%x\n",
			reg + i, *(val + i));
	}
	return 0;
}

static inline int __nxe2000_write(struct i2c_client *client, u8 reg,
				  uint8_t val)
{
	int ret;

	dev_dbg(&client->dev, "nxe2000: reg write  reg=%x, val=%x\n", reg, val);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writing 0x%02x to 0x%02x\n", val,
			reg);
		return ret;
	}

	return 0;
}

static inline int __nxe2000_bulk_writes(struct i2c_client *client, u8 reg,
					int len, uint8_t *val)
{
	int ret;
	int i;

	for (i = 0; i < len; ++i) {
		dev_dbg(&client->dev, "nxe2000: reg write  reg=%x, val=%x\n",
			reg + i, *(val + i));
	}

	ret = i2c_smbus_write_i2c_block_data(client, reg, len, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writings to 0x%02x\n", reg);
		return ret;
	}

	return 0;
}

static inline int set_bank_nxe2000(struct device *dev, int bank)
{
	struct nxe2000 *nxe2000 = dev_get_drvdata(dev);
	int ret;

	if (bank != (bank & 1))
		return -EINVAL;
	if (bank == nxe2000->bank_num)
		return 0;
	ret = __nxe2000_write(to_i2c_client(dev), NXE2000_REG_BANKSEL, bank);
	if (!ret)
		nxe2000->bank_num = bank;

	return ret;
}

int nxe2000_pm_write(u8 reg, uint8_t val)
{
	int ret = 0;

	ret = nxe2000_write(&nxe2000_i2c_client->dev, reg, val);

	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_pm_write);

int nxe2000_write(struct device *dev, u8 reg, uint8_t val)
{
	struct nxe2000 *nxe2000 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&nxe2000->io_lock);
	ret = set_bank_nxe2000(dev, 0);
	if (!ret)
		ret = __nxe2000_write(to_i2c_client(dev), reg, val);
	mutex_unlock(&nxe2000->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_write);

int nxe2000_write_bank1(struct device *dev, u8 reg, uint8_t val)
{
	struct nxe2000 *nxe2000 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&nxe2000->io_lock);
	ret = set_bank_nxe2000(dev, 1);
	if (!ret)
		ret = __nxe2000_write(to_i2c_client(dev), reg, val);
	mutex_unlock(&nxe2000->io_lock);

	ret = set_bank_nxe2000(dev, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_write_bank1);

int nxe2000_bulk_writes(struct device *dev, u8 reg, u8 len, uint8_t *val)
{
	struct nxe2000 *nxe2000 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&nxe2000->io_lock);
	ret = set_bank_nxe2000(dev, 0);
	if (!ret)
		ret = __nxe2000_bulk_writes(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&nxe2000->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_bulk_writes);

int nxe2000_bulk_writes_bank1(struct device *dev, u8 reg, u8 len, uint8_t *val)
{
	struct nxe2000 *nxe2000 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&nxe2000->io_lock);
	ret = set_bank_nxe2000(dev, 1);
	if (!ret)
		ret = __nxe2000_bulk_writes(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&nxe2000->io_lock);

	ret = set_bank_nxe2000(dev, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_bulk_writes_bank1);

int nxe2000_pm_read(u8 reg, uint8_t *val)
{
	int ret = 0;

	ret = nxe2000_read(&nxe2000_i2c_client->dev, reg, val);

	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_pm_read);

int nxe2000_read(struct device *dev, u8 reg, uint8_t *val)
{
	struct nxe2000 *nxe2000 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&nxe2000->io_lock);
	ret = set_bank_nxe2000(dev, 0);
	if (!ret)
		ret = __nxe2000_read(to_i2c_client(dev), reg, val);
	mutex_unlock(&nxe2000->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_read);

int nxe2000_read_bank1(struct device *dev, u8 reg, uint8_t *val)
{
	struct nxe2000 *nxe2000 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&nxe2000->io_lock);
	ret = set_bank_nxe2000(dev, 1);
	if (!ret)
		ret = __nxe2000_read(to_i2c_client(dev), reg, val);
	mutex_unlock(&nxe2000->io_lock);

	ret = set_bank_nxe2000(dev, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_read_bank1);

int nxe2000_bulk_reads(struct device *dev, u8 reg, u8 len, uint8_t *val)
{
	struct nxe2000 *nxe2000 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&nxe2000->io_lock);
	ret = set_bank_nxe2000(dev, 0);
	if (!ret)
		ret = __nxe2000_bulk_reads(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&nxe2000->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_bulk_reads);

int nxe2000_bulk_reads_bank1(struct device *dev, u8 reg, u8 len, uint8_t *val)
{
	struct nxe2000 *nxe2000 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&nxe2000->io_lock);
	ret = set_bank_nxe2000(dev, 1);
	if (!ret)
		ret = __nxe2000_bulk_reads(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&nxe2000->io_lock);

	ret = set_bank_nxe2000(dev, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_bulk_reads_bank1);

int nxe2000_set_bits(struct device *dev, u8 reg, uint8_t bit_mask)
{
	struct nxe2000 *nxe2000 = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&nxe2000->io_lock);
	ret = set_bank_nxe2000(dev, 0);
	if (!ret) {
		ret = __nxe2000_read(to_i2c_client(dev), reg, &reg_val);
		if (ret)
			goto out;

		if ((reg_val & bit_mask) != bit_mask) {
			reg_val |= bit_mask;
			ret = __nxe2000_write(to_i2c_client(dev), reg, reg_val);
		}
	}
out:
	mutex_unlock(&nxe2000->io_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_set_bits);

int nxe2000_clr_bits(struct device *dev, u8 reg, uint8_t bit_mask)
{
	struct nxe2000 *nxe2000 = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&nxe2000->io_lock);
	ret = set_bank_nxe2000(dev, 0);
	if (!ret) {
		ret = __nxe2000_read(to_i2c_client(dev), reg, &reg_val);
		if (ret)
			goto out;

		if (reg_val & bit_mask) {
			reg_val &= ~bit_mask;
			ret = __nxe2000_write(to_i2c_client(dev), reg, reg_val);
		}
	}
out:
	mutex_unlock(&nxe2000->io_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_clr_bits);

int nxe2000_update(struct device *dev, u8 reg, uint8_t val, uint8_t mask)
{
	struct nxe2000 *nxe2000 = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&nxe2000->io_lock);
	ret = set_bank_nxe2000(dev, 0);
	if (!ret) {
		ret = __nxe2000_read(nxe2000->client, reg, &reg_val);
		if (ret)
			goto out;

		if ((reg_val & mask) != val) {
			reg_val = (reg_val & ~mask) | (val & mask);
			ret = __nxe2000_write(nxe2000->client, reg, reg_val);
		}
	}
out:
	mutex_unlock(&nxe2000->io_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(nxe2000_update);

int nxe2000_update_bank1(struct device *dev, u8 reg, uint8_t val, uint8_t mask)
{
	struct nxe2000 *nxe2000 = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&nxe2000->io_lock);
	ret = set_bank_nxe2000(dev, 1);
	if (!ret) {
		ret = __nxe2000_read(nxe2000->client, reg, &reg_val);
		if (ret)
			goto out;

		if ((reg_val & mask) != val) {
			reg_val = (reg_val & ~mask) | (val & mask);
			ret = __nxe2000_write(nxe2000->client, reg, reg_val);
		}
	}
out:
	ret = set_bank_nxe2000(dev, 0);
	mutex_unlock(&nxe2000->io_lock);
	return ret;
}

void nxe2000_power_off(void)
{
	int ret;
	uint8_t reg_val = 0;

	if (!nxe2000_i2c_client)
		return;

#if defined(CONFIG_BATTERY_NXE2000)
	reg_val = g_soc;
	reg_val &= 0x7f;

	ret = nxe2000_write(&nxe2000_i2c_client->dev, NXE2000_PSWR, reg_val);
	if (ret < 0)
		dev_err(&nxe2000_i2c_client->dev,
			"Error in writing PSWR_REG\n");

	if (g_fg_on_mode == 0) {
		/* Clear NXE2000_FG_CTRL 0x01 bit */
		ret = nxe2000_read(&nxe2000_i2c_client->dev, NXE2000_FG_CTRL,
				   &reg_val);
		if (reg_val & 0x01) {
			reg_val &= ~0x01;
			ret = nxe2000_write(&nxe2000_i2c_client->dev,
					    NXE2000_FG_CTRL, reg_val);
		}
		if (ret < 0)
			dev_err(&nxe2000_i2c_client->dev,
				"Error in writing FG_CTRL\n");
	}
#endif

	/* set rapid timer 300 min */
	ret = nxe2000_read(&nxe2000_i2c_client->dev, NXE2000_REG_TIMSET,
			   &reg_val);
	if (ret < 0)
		ret = nxe2000_read(&nxe2000_i2c_client->dev, NXE2000_REG_TIMSET,
				   &reg_val);

	reg_val |= 0x03;

	ret = nxe2000_write(&nxe2000_i2c_client->dev, NXE2000_REG_TIMSET,
			    reg_val);
	if (ret < 0)
		ret = nxe2000_write(&nxe2000_i2c_client->dev,
				    NXE2000_REG_TIMSET, reg_val);
	if (ret < 0)
		dev_err(&nxe2000_i2c_client->dev,
			"Error in writing the NXE2000_REG_TIMSET\n");

	/* Disable all Interrupt */
	ret = nxe2000_write(&nxe2000_i2c_client->dev, NXE2000_REG_INTEN, 0);
	if (ret < 0)
		ret = nxe2000_write(&nxe2000_i2c_client->dev, NXE2000_REG_INTEN,
				    0);

	/* Not repeat power ON after power off(Power Off/N_OE) */
	ret = nxe2000_write(&nxe2000_i2c_client->dev, NXE2000_REG_REPCNT, 0x0);
	if (ret < 0)
		ret = nxe2000_write(&nxe2000_i2c_client->dev,
				    NXE2000_REG_REPCNT, 0x0);

	/* Power OFF */
	ret = nxe2000_write(&nxe2000_i2c_client->dev, NXE2000_REG_SLPCNT, 0x1);
	if (ret < 0)
		ret = nxe2000_write(&nxe2000_i2c_client->dev,
				    NXE2000_REG_SLPCNT, 0x1);
}

#ifdef CONFIG_NXE2000_WDG_TEST
static irqreturn_t nxe2000_watchdog_isr(int irq, void *data)
{
	struct nxe2000 *info = data;

	pr_debug("## %s()\n", __func__);
	nxe2000_clr_bits(info->dev, NXE2000_INT_IR_SYS, 0x40);

	return IRQ_HANDLED;
}

static void nxe2000_watchdog_init(struct nxe2000 *nxe2000)
{
	int ret;

	pr_debug("## %s()\n", __func__);
	ret = request_threaded_irq((IRQ_SYSTEM_END + NXE2000_IRQ_WD), NULL,
				   nxe2000_watchdog_isr, IRQF_ONESHOT,
				   "nxe2000_watchdog_isr", nxe2000);

	nxe2000_set_bits(nxe2000->dev, NXE2000_REG_REPCNT, 0x01);
	nxe2000_write(nxe2000->dev, NXE2000_REG_WATCHDOG, 0x05);
}
#endif

static int nxe2000_remove_subdev(struct device *dev, void *unused)
{
	platform_device_unregister(to_platform_device(dev));
	return 0;
}

static int nxe2000_remove_subdevs(struct nxe2000 *nxe2000)
{
	return device_for_each_child(nxe2000->dev, NULL, nxe2000_remove_subdev);
}

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
static void print_regs(const char *header, struct seq_file *s,
		       struct i2c_client *client, int start_offset,
		       int end_offset)
{
	uint8_t reg_val;
	int i;
	int ret;

	seq_printf(s, "%s\n", header);
	for (i = start_offset; i <= end_offset; ++i) {
		ret = __nxe2000_read(client, i, &reg_val);
		if (ret >= 0)
			seq_printf(s, "Reg 0x%02x Value 0x%02x\n", i, reg_val);
	}
	seq_puts(s, "------------------\n");
}

static int dbg_nxe2000_show(struct seq_file *s, void *unused)
{
	struct nxe2000 *nxe2000 = s->private;
	struct i2c_client *client = nxe2000->client;

	seq_puts(s, "NXE2000 register values:\n");
	seq_puts(s, "------------------\n");

	print_regs("System Regs", s, client, 0x0, 0x05);
	print_regs("Power Control Regs", s, client, 0x07, 0x2B);
	print_regs("DCDC  Regs", s, client, 0x2C, 0x43);
	print_regs("LDO   Regs", s, client, 0x44, 0x61);
	print_regs("ADC   Regs", s, client, 0x64, 0x8F);
	print_regs("GPIO  Regs", s, client, 0x90, 0x98);
	print_regs("INTC  Regs", s, client, 0x9C, 0x9E);
	print_regs("RTC   Regs", s, client, 0xA0, 0xAF);
	print_regs("OPT   Regs", s, client, 0xB0, 0xB1);
	print_regs("CHG   Regs", s, client, 0xB2, 0xDF);
	print_regs("FUEL  Regs", s, client, 0xE0, 0xFC);
	return 0;
}

static int dbg_nxe2000_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_nxe2000_show, inode->i_private);
}

static const struct file_operations debug_fops = {
	.open = dbg_nxe2000_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void nxe2000_debuginit(struct nxe2000 *nxe2000)
{
	(void)debugfs_create_file("nxe2000", S_IRUGO, NULL, nxe2000,
				  &debug_fops);
}
#endif

static struct mfd_cell nxe2000_devs[] = {
	{ .name = "nxe2000-regulator", },
	/* { .name = "nxe2000-battery", }, */
};

#ifdef CONFIG_OF
static const struct of_device_id nxe2000_pmic_dt_match[] = {
	{ .compatible = "nexell,nxe2000", .data = NULL }, {},
};

static struct nxe2000_platform_data *
nxe2000_i2c_parse_dt_pdata(struct nxe2000 *pdev)
{
	struct device *dev = pdev->dev;
	struct i2c_client *client = pdev->client;
	struct nxe2000_platform_data *pd;
	struct device_node *nxe2000_np;
	u32 val;

	pd = devm_kzalloc(dev, sizeof(*pd), GFP_KERNEL);
	if (!pd)
		return NULL;

	nxe2000_np = of_node_get(dev->of_node);
	if (!nxe2000_np) {
		dev_err(dev, "could not find nxe2000 sub-node\n");
		return pd;
	}

	return pd;
}
#else
static struct nxe2000_platform_data *
nxe2000_i2c_parse_dt_pdata(struct nxe2000 *pdev)
{
	return 0;
}
#endif

static int nxe2000_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	struct nxe2000 *nxe2000 = NULL;
	struct nxe2000_platform_data *pdata = NULL;
	int ret;

	nxe2000 = devm_kzalloc(&client->dev, sizeof(struct nxe2000), GFP_KERNEL);
	if (!nxe2000)
		return -ENOMEM;

	nxe2000->dev = &client->dev;
	nxe2000->client = client;

	i2c_set_clientdata(client, nxe2000);

	if (client->dev.of_node) {
		const struct of_device_id *match;

		match = of_match_device(of_match_ptr(nxe2000_pmic_dt_match),
			&client->dev);
		if (!match) {
			dev_err(&client->dev,
				"%s() Error: No device match found\n",
				__func__);
			goto err_irq_init;
		}
	} else {
		dev_err(&client->dev,
			"%s() Error: No device match found\n", __func__);
	}

	if (nxe2000->dev->of_node) {
		pdata = nxe2000_i2c_parse_dt_pdata(nxe2000);
		if (!pdata)
			goto err_irq_init;
	}

	nxe2000->pdata = pdata;

	mutex_init(&nxe2000->io_lock);

	nxe2000->bank_num = 0;

	/* For init PMIC_IRQ port */
	/* ret = pdata->init_port(client->irq); */

	if (client->irq > 0) {
		nxe2000->chip_irq = client->irq;

		ret = nxe2000_irq_init(nxe2000);
		if (ret) {
			dev_err(&client->dev, "IRQ init failed: %d\n", ret);
			goto err_irq_init;
		}
	}

	ret = mfd_add_devices(nxe2000->dev, -1, nxe2000_devs,
			      ARRAY_SIZE(nxe2000_devs), NULL, 0, NULL);

	if (ret) {
		dev_err(&client->dev, "add devices failed: %d\n", ret);
		goto err_add_devs;
	}

#ifdef CONFIG_DEBUG_FS
	nxe2000_debuginit(nxe2000);
#endif

#ifdef CONFIG_NXE2000_WDG_TEST
	nxe2000_watchdog_init(nxe2000);
#endif

	nxe2000_i2c_client = client;

#if 1
	pm_power_off = nxe2000_power_off;
#else
	nxp_board_shutdown = nxe2000_power_off;
#endif

	return 0;

err_add_devs:
	if (client->irq)
		nxe2000_irq_exit(nxe2000);

err_irq_init:
	kfree(nxe2000);
	return ret;
}

static int nxe2000_i2c_remove(struct i2c_client *client)
{
	struct nxe2000 *nxe2000 = i2c_get_clientdata(client);

#ifdef CONFIG_NXE2000_WDG_TEST
	free_irq((IRQ_SYSTEM_END + NXE2000_IRQ_WD), nxe2000);
#endif

	if (client->irq)
		nxe2000_irq_exit(nxe2000);

	nxe2000_remove_subdevs(nxe2000);

	cancel_delayed_work(&nxe2000->dcdc_int_work);
	flush_workqueue(nxe2000->workqueue);
	destroy_workqueue(nxe2000->workqueue);

	kfree(nxe2000);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int nxe2000_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	if (client->irq)
		disable_irq(client->irq);

	return 0;
}

int pwrkey_wakeup;
static int nxe2000_i2c_resume(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	uint8_t reg_val = 0;
	int ret;

	/* Disable all Interrupt */
	__nxe2000_write(client, NXE2000_REG_INTEN, 0x0);

	ret = __nxe2000_read(client, NXE2000_INT_IR_SYS, &reg_val);
	if (reg_val & 0x01) { /* If PWR_KEY wakeup */
		pwrkey_wakeup = 1;
		/* Clear PWR_KEY IRQ */
		__nxe2000_write(client, NXE2000_INT_IR_SYS, 0x0);
	}
	enable_irq(client->irq);

	/* Enable all Interrupt */
	__nxe2000_write(client, NXE2000_REG_INTEN, 0xff);

	return 0;
}

#endif

static SIMPLE_DEV_PM_OPS(nxe2000_pm, nxe2000_i2c_suspend, nxe2000_i2c_resume);

static const struct i2c_device_id nxe2000_i2c_id[] = {
	{"nxe2000", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, nxe2000_i2c_id);


static struct i2c_driver nxe2000_i2c_driver = {
	.driver = {
		.name = "nxe2000",
		.owner = THIS_MODULE,
		.pm	= &nxe2000_pm,
		.of_match_table = of_match_ptr(nxe2000_pmic_dt_match),
	},
	.probe = nxe2000_i2c_probe,
	.remove = nxe2000_i2c_remove,
	.id_table = nxe2000_i2c_id,
};

static int __init nxe2000_i2c_init(void)
{
	int ret = -ENODEV;

	ret = i2c_add_driver(&nxe2000_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register I2C driver: %d\n", ret);

	return ret;
}
subsys_initcall_sync(nxe2000_i2c_init);

static void __exit nxe2000_i2c_exit(void)
{
	i2c_del_driver(&nxe2000_i2c_driver);
}
module_exit(nxe2000_i2c_exit);

MODULE_DESCRIPTION("NEXELL NXE2000 PMU multi-function core driver");
MODULE_LICENSE("GPL");
