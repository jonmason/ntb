/*
 * axp228-mfd.c  --  PMIC driver for the X-Powers AXP228
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/pm.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>

#include <linux/mfd/core.h>
#include <linux/mfd/axp228-mfd.h>
#include <linux/mfd/axp228-cfg.h>

#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>

#ifdef ENABLE_DEBUG
#define DBG_MSG(format, args...) pr_err(format, ##args)
#else
#define DBG_MSG(format, args...) do {} while (0)
#endif

#define AXP_I2C_RETRY_CNT 3

static uint8_t axp_reg_addr;
struct axp_mfd_chip *g_chip;
struct i2c_client *axp;
EXPORT_SYMBOL_GPL(axp);

static inline int __axp_read(struct i2c_client *client, int reg, uint8_t *val)
{
	int ret = 0;
	int i = 0;

	for (i = 0; i < AXP_I2C_RETRY_CNT; i++) {
		ret = i2c_smbus_read_byte_data(client, reg);
		if (ret >= 0)
			break;

		mdelay(10);
	}

	if (ret < 0) {
		dev_err(&client->dev, "failed reading at 0x%02x\n",
			reg);
		return ret;
	}

	*val = (uint8_t)ret;
	return 0;
}

static inline int __axp_reads(struct i2c_client *client, int reg, int len,
			      uint8_t *val)
{
	int ret = 0;
	int i = 0;

	for (i = 0; i < AXP_I2C_RETRY_CNT; i++) {
		ret = i2c_smbus_read_i2c_block_data(client, reg, len, val);
		if (ret >= 0)
			break;
		mdelay(10);
	}

	if (ret < 0) {
		dev_err(&client->dev, "failed reading from 0x%02x\n",
			reg);
		return ret;
	}
	return 0;
}

static inline int __axp_write(struct i2c_client *client, int reg, uint8_t val)
{
	int ret = 0;
	int i = 0;

	for (i = 0; i < AXP_I2C_RETRY_CNT; i++) {
		ret = i2c_smbus_write_byte_data(client, reg, val);
		if (ret >= 0)
			break;
		mdelay(10);
	}

	if (ret < 0) {
		dev_err(&client->dev,
			"failed writing 0x%02x to 0x%02x\n", val,
			reg);
		return ret;
	}
	return 0;
}

static inline int __axp_writes(struct i2c_client *client, int reg, int len,
			       uint8_t *val)
{
	int ret = 0;
	int i = 0;

	for (i = 0; i < AXP_I2C_RETRY_CNT; i++) {
		ret = i2c_smbus_write_i2c_block_data(client, reg, len, val);
		if (ret >= 0)
			break;
		mdelay(10);
	}

	if (ret < 0) {
		dev_err(&client->dev, "failed writings to 0x%02x\n",
			reg);
		return ret;
	}
	return 0;
}

int axp_register_notifier(struct device *dev, struct notifier_block *nb,
			  uint64_t irqs)
{
	struct axp_mfd_chip *chip = dev_get_drvdata(dev);

	chip->ops->enable_irqs(chip, irqs);
	if (NULL != nb) {
		return blocking_notifier_chain_register(&chip->notifier_list,
							nb);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(axp_register_notifier);

int axp_unregister_notifier(struct device *dev, struct notifier_block *nb,
			    uint64_t irqs)
{
	struct axp_mfd_chip *chip = dev_get_drvdata(dev);

	chip->ops->disable_irqs(chip, irqs);
	if (NULL != nb) {
		return blocking_notifier_chain_unregister(&chip->notifier_list,
							  nb);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(axp_unregister_notifier);

int axp_write(struct device *dev, int reg, uint8_t val)
{
	struct axp_mfd_chip *chip = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&chip->lock);
	ret = __axp_write(to_i2c_client(dev), reg, val);
	mutex_unlock(&chip->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_write);

int axp_writes(struct device *dev, int reg, int len, uint8_t *val)
{
	struct axp_mfd_chip *chip = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&chip->lock);
	ret = __axp_writes(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&chip->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_writes);

int axp_read(struct device *dev, int reg, uint8_t *val)
{
	struct axp_mfd_chip *chip = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&chip->lock);
	ret = __axp_read(to_i2c_client(dev), reg, val);
	mutex_unlock(&chip->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_read);

int axp_reads(struct device *dev, int reg, int len, uint8_t *val)
{
	struct axp_mfd_chip *chip = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&chip->lock);
	ret = __axp_reads(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&chip->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_reads);

int axp_set_bits(struct device *dev, int reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	int ret = 0;
	struct axp_mfd_chip *chip;

	chip = dev_get_drvdata(dev);
	mutex_lock(&chip->lock);
	ret = __axp_read(chip->client, reg, &reg_val);
	if (ret)
		goto out;

	if ((reg_val & bit_mask) != bit_mask) {
		reg_val |= bit_mask;
		ret = __axp_write(chip->client, reg, reg_val);
	}
out:
	mutex_unlock(&chip->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(axp_set_bits);

int axp_clr_bits(struct device *dev, int reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	int ret = 0;
	struct axp_mfd_chip *chip;

	chip = dev_get_drvdata(dev);

	mutex_lock(&chip->lock);

	ret = __axp_read(chip->client, reg, &reg_val);
	if (ret)
		goto out;

	if (reg_val & bit_mask) {
		reg_val &= ~bit_mask;
		ret = __axp_write(chip->client, reg, reg_val);
	}
out:
	mutex_unlock(&chip->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(axp_clr_bits);

int axp_update(struct device *dev, int reg, uint8_t val, uint8_t mask)
{
	struct axp_mfd_chip *chip = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&chip->lock);

	ret = __axp_read(chip->client, reg, &reg_val);
	if (ret)
		goto out;

	if ((reg_val & mask) != val) {
		reg_val = (reg_val & ~mask) | val;
		ret = __axp_write(chip->client, reg, reg_val);
	}
out:
	mutex_unlock(&chip->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(axp_update);

struct device *axp_get_dev(void) { return &axp->dev; }
EXPORT_SYMBOL_GPL(axp_get_dev);

static int axp22_init_chip(struct axp_mfd_chip *chip)
{
	uint8_t chip_id = 0;
	uint8_t v[19] = {
		0xd8,
		AXP22_INTEN2, 0xff, AXP22_INTEN3, 0x00,
		AXP22_INTEN4, 0x01, AXP22_INTEN5, 0x00,
		AXP22_INTSTS1, 0xff, AXP22_INTSTS2, 0xff,
		AXP22_INTSTS3, 0xff, AXP22_INTSTS4, 0xff,
		AXP22_INTSTS5, 0xff
		};
	int err;

	/*read chip id*/
	err = __axp_read(chip->client, AXP22_IC_TYPE, &chip_id);
	if (err) {
		pr_err("[AXP22-MFD] try to read chip id failed!\n");
		return err;
	}

	/*enable irqs and clear*/
	err = __axp_writes(chip->client, AXP22_INTEN1, 19, v);
	if (err) {
		pr_err("[AXP22-MFD] try to clear irq failed!\n");
		return err;
	}

	dev_info(chip->dev, "AXP (CHIP ID: 0x%02x) detected\n", chip_id);
	chip->type = AXP22;

	/* mask and clear all IRQs */
	chip->irqs_enabled = 0xffffffff | (uint64_t)0xff << 32;
	chip->ops->disable_irqs(chip, chip->irqs_enabled);

	return 0;
}

static int axp22_disable_irqs(struct axp_mfd_chip *chip, uint64_t irqs)
{
	uint8_t v[9];
	int ret;

	chip->irqs_enabled &= ~irqs;

	v[0] = ((chip->irqs_enabled) & 0xff);
	v[1] = AXP22_INTEN2;
	v[2] = ((chip->irqs_enabled) >> 8) & 0xff;
	v[3] = AXP22_INTEN3;
	v[4] = ((chip->irqs_enabled) >> 16) & 0xff;
	v[5] = AXP22_INTEN4;
	v[6] = ((chip->irqs_enabled) >> 24) & 0xff;
	v[7] = AXP22_INTEN5;
	v[8] = ((chip->irqs_enabled) >> 32) & 0xff;
	ret = __axp_writes(chip->client, AXP22_INTEN1, 9, v);

	return ret;
}

static int axp22_enable_irqs(struct axp_mfd_chip *chip, uint64_t irqs)
{
	uint8_t v[9];
	int ret;

	chip->irqs_enabled |= irqs;

	v[0] = ((chip->irqs_enabled) & 0xff);
	v[1] = AXP22_INTEN2;
	v[2] = ((chip->irqs_enabled) >> 8) & 0xff;
	v[3] = AXP22_INTEN3;
	v[4] = ((chip->irqs_enabled) >> 16) & 0xff;
	v[5] = AXP22_INTEN4;
	v[6] = ((chip->irqs_enabled) >> 24) & 0xff;
	v[7] = AXP22_INTEN5;
	v[8] = ((chip->irqs_enabled) >> 32) & 0xff;
	ret = __axp_writes(chip->client, AXP22_INTEN1, 9, v);

	return ret;
}

static int axp22_read_irqs(struct axp_mfd_chip *chip, uint64_t *irqs)
{
	uint8_t v[5] = { 0, 0, 0, 0, 0 };
	int ret;

	ret = __axp_reads(chip->client, AXP22_INTSTS1, 5, v);
	if (ret < 0)
		return ret;

	*irqs = (((uint64_t)v[4]) << 32) | (((uint64_t)v[3]) << 24) |
		(((uint64_t)v[2]) << 16) | (((uint64_t)v[1]) << 8) |
		((uint64_t)v[0]);
	return 0;
}

static ssize_t axp22_offvol_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	uint8_t val = 0;

	axp_read(dev, AXP22_VOFF_SET, &val);
	return sprintf(buf, "%d\n", (val & 0x07) * 100 + 2600);
}

static ssize_t axp22_offvol_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned long tmp;
	uint8_t val;
	int err;

	err = kstrtoul(buf, 10, &tmp);
	if (err)
		return err;

	if (tmp < 2600)
		tmp = 2600;
	if (tmp > 3300)
		tmp = 3300;

	axp_read(dev, AXP22_VOFF_SET, &val);
	val &= 0xf8;
	val |= ((tmp - 2600) / 100);
	axp_write(dev, AXP22_VOFF_SET, val);
	return count;
}

static ssize_t axp22_noedelay_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	uint8_t val;

	axp_read(dev, AXP22_OFF_CTL, &val);
	if ((val & 0x03) == 0)
		return sprintf(buf, "%d\n", 128);
	else
		return sprintf(buf, "%d\n", (val & 0x03) * 1000);
}

static ssize_t axp22_noedelay_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	unsigned long tmp;
	uint8_t val;
	int err;

	err = kstrtoul(buf, 10, &tmp);
	if (err)
		return err;

	if (tmp < 1000)
		tmp = 128;
	if (tmp > 3000)
		tmp = 3000;
	axp_read(dev, AXP22_OFF_CTL, &val);
	val &= 0xfc;
	val |= ((tmp) / 1000);
	axp_write(dev, AXP22_OFF_CTL, val);
	return count;
}

static ssize_t axp22_pekopen_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	uint8_t val;
	int tmp = 0;

	axp_read(dev, AXP22_POK_SET, &val);
	switch (val >> 6) {
	case 0:
		tmp = 128;
		break;
	case 1:
		tmp = 3000;
		break;
	case 2:
		tmp = 1000;
		break;
	case 3:
		tmp = 2000;
		break;
	default:
		tmp = 0;
		break;
	}
	return sprintf(buf, "%d\n", tmp);
}

static ssize_t axp22_pekopen_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	unsigned long tmp;
	uint8_t val;
	int err;

	err = kstrtoul(buf, 10, &tmp);
	if (err)
		return err;

	axp_read(dev, AXP22_POK_SET, &val);
	if (tmp < 1000)
		val &= 0x3f;
	else if (tmp < 2000) {
		val &= 0x3f;
		val |= 0x80;
	} else if (tmp < 3000) {
		val &= 0x3f;
		val |= 0xc0;
	} else {
		val &= 0x3f;
		val |= 0x40;
	}
	axp_write(dev, AXP22_POK_SET, val);
	return count;
}

static ssize_t axp22_peklong_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	uint8_t val = 0;

	axp_read(dev, AXP22_POK_SET, &val);
	return sprintf(buf, "%d\n", ((val >> 4) & 0x03) * 500 + 1000);
}

static ssize_t axp22_peklong_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	unsigned long tmp;
	uint8_t val;
	int err;

	err = kstrtoul(buf, 10, &tmp);
	if (err)
		return err;

	if (tmp < 1000)
		tmp = 1000;
	if (tmp > 2500)
		tmp = 2500;
	axp_read(dev, AXP22_POK_SET, &val);
	val &= 0xcf;
	val |= (((tmp - 1000) / 500) << 4);
	axp_write(dev, AXP22_POK_SET, val);
	return count;
}

static ssize_t axp22_peken_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	uint8_t val;

	axp_read(dev, AXP22_POK_SET, &val);
	return sprintf(buf, "%d\n", ((val >> 3) & 0x01));
}

static ssize_t axp22_peken_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	unsigned long tmp;
	uint8_t val;
	int err;

	err = kstrtoul(buf, 10, &tmp);
	if (err)
		return err;

	if (tmp)
		tmp = 1;
	axp_read(dev, AXP22_POK_SET, &val);
	val &= 0xf7;
	val |= (tmp << 3);
	axp_write(dev, AXP22_POK_SET, val);
	return count;
}

static ssize_t axp22_pekdelay_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	uint8_t val;

	axp_read(dev, AXP22_POK_SET, &val);
	return sprintf(buf, "%d\n", ((val >> 2) & 0x01) ? 64 : 8);
}

static ssize_t axp22_pekdelay_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	unsigned long tmp;
	uint8_t val;
	int err;

	err = kstrtoul(buf, 10, &tmp);
	if (err)
		return err;

	if (tmp <= 8)
		tmp = 0;
	else
		tmp = 1;
	axp_read(dev, AXP22_POK_SET, &val);
	val &= 0xfb;
	val |= tmp << 2;
	axp_write(dev, AXP22_POK_SET, val);
	return count;
}

static ssize_t axp22_pekclose_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	uint8_t val;

	axp_read(dev, AXP22_POK_SET, &val);
	return sprintf(buf, "%d\n", ((val & 0x03) * 2000) + 4000);
}

static ssize_t axp22_pekclose_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	unsigned long tmp;
	uint8_t val;
	int err;

	err = kstrtoul(buf, 10, &tmp);
	if (err)
		return err;

	if (tmp < 4000)
		tmp = 4000;
	if (tmp > 10000)
		tmp = 10000;
	tmp = (tmp - 4000) / 2000;
	axp_read(dev, AXP22_POK_SET, &val);
	val &= 0xfc;
	val |= tmp;
	axp_write(dev, AXP22_POK_SET, val);
	return count;
}

static ssize_t axp22_ovtemclsen_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	uint8_t val;

	axp_read(dev, AXP22_HOTOVER_CTL, &val);
	return sprintf(buf, "%d\n", ((val >> 2) & 0x01));
}

static ssize_t axp22_ovtemclsen_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	unsigned long tmp;
	uint8_t val;
	int err;

	err = kstrtoul(buf, 10, &tmp);
	if (err)
		return err;

	if (tmp)
		tmp = 1;
	axp_read(dev, AXP22_HOTOVER_CTL, &val);
	val &= 0xfb;
	val |= tmp << 2;
	axp_write(dev, AXP22_HOTOVER_CTL, val);
	return count;
}

static ssize_t axp22_reg_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	uint8_t val;

	axp_read(dev, axp_reg_addr, &val);
	return sprintf(buf, "REG[%x]=%x\n", axp_reg_addr, val);
}

static ssize_t axp22_reg_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	unsigned long tmp;
	uint8_t val;
	int err;

	err = kstrtoul(buf, 16, &tmp);
	if (err)
		return err;

	if (tmp < 256)
		axp_reg_addr = tmp;
	else {
		val = tmp & 0x00FF;
		axp_reg_addr = (tmp >> 8) & 0x00FF;
		axp_write(dev, axp_reg_addr, val);
	}
	return count;
}

static ssize_t axp22_regs_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	uint8_t val[2];

	axp_reads(dev, axp_reg_addr, 2, val);
	return sprintf(buf, "REG[0x%x]=0x%x,REG[0x%x]=0x%x\n", axp_reg_addr,
		       val[0], axp_reg_addr + 1, val[1]);
}

static ssize_t axp22_regs_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	unsigned long tmp;
	uint8_t val[3];
	int err;

	err = kstrtoul(buf, 16, &tmp);
	if (err)
		return err;

	if (tmp < 256)
		axp_reg_addr = tmp;
	else {
		axp_reg_addr = (tmp >> 16) & 0xFF;
		val[0] = (tmp >> 8) & 0xFF;
		val[1] = axp_reg_addr + 1;
		val[2] = tmp & 0xFF;
		axp_writes(dev, axp_reg_addr, 3, val);
	}
	return count;
}

static struct device_attribute axp22_mfd_attrs[] = {
	AXP_MFD_ATTR(axp22_offvol),
	AXP_MFD_ATTR(axp22_noedelay),
	AXP_MFD_ATTR(axp22_pekopen),
	AXP_MFD_ATTR(axp22_peklong),
	AXP_MFD_ATTR(axp22_peken),
	AXP_MFD_ATTR(axp22_pekdelay),
	AXP_MFD_ATTR(axp22_pekclose),
	AXP_MFD_ATTR(axp22_ovtemclsen),
	AXP_MFD_ATTR(axp22_reg),
	AXP_MFD_ATTR(axp22_regs),
};

void axp_run_irq_handler(void)
{
	DBG_MSG("## [%s():%d]\n", __func__, __LINE__);
	(void)schedule_work(&g_chip->irq_work);
}
EXPORT_SYMBOL_GPL(axp_run_irq_handler);

static void axp_mfd_register_dump(struct device *dev)
{
	int ret = 0;
	u16 i = 0;
	u8 value = 0;

	dev_info(dev,
		"##########################################################\n");
	dev_info(dev,
		"## %s()                               #\n", __func__);
	dev_info(dev,
		"##########################################################\n");
	dev_info(dev,
		"##      0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F\n");

	for (i = 0; i <= 0xff; i++) {
		if (i % 16 == 0)
			dev_info(dev, "## %02X:", i);

		if (i % 4 == 0)
			dev_info(dev, " ");

		ret = axp_read(dev, i, &value);
		if (!ret)
			dev_info(dev, "%02x ", value);
		else
			dev_info(dev, "xx ");

		if ((i + 1) % 16 == 0)
			dev_info(dev, "\n");
	}
	dev_info(dev,
		"##########################################################\n");
}

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>

static int axp_dbg_show(struct seq_file *s, void *unused)
{
	struct axp_mfd_chip *chip = s->private;
	struct device *dev = chip->dev;

	axp_mfd_register_dump(dev);
	return 0;
}

static int axp_dbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, axp_dbg_show, inode->i_private);
}

static const struct file_operations debug_fops = {
	.open = axp_dbg_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
static void axp_debuginit(struct axp_mfd_chip *chip)
{
	(void)debugfs_create_file("axp228", S_IRUGO, NULL, chip, &debug_fops);
}
#else
static void axp_debuginit(struct axp_mfd_chip *chip)
{
	struct device *dev = chip->dev;

	axp_mfd_register_dump(dev);
	return 0;
}
#endif

static void axp_mfd_irq_work(struct work_struct *work)
{
	struct axp_mfd_chip *chip =
	    container_of(work, struct axp_mfd_chip, irq_work);
	uint64_t irqs = 0;

	DBG_MSG("## [%s():%d]\n", __func__, __LINE__);

	while (1) {
		if (chip->ops->read_irqs(chip, &irqs)) {
			pr_err("read irq fail\n");
			break;
		}
		irqs &= chip->irqs_enabled;
		if (irqs == 0)
			break;

		if (irqs > 0xffffffff) {
			blocking_notifier_call_chain(&chip->notifier_list,
						     (uint32_t)(irqs >> 32),
						     (void *)1);
		} else {
			blocking_notifier_call_chain(&chip->notifier_list,
						     (uint32_t)irqs, (void *)0);
		}
	}
	/* enable_irq(chip->client->irq); */
}

static irqreturn_t axp_mfd_irq_handler(int irq, void *data)
{
	struct axp_mfd_chip *chip = data;

	DBG_MSG("## [%s():%d]\n", __func__, __LINE__);

	/* disable_irq_nosync(irq); */
	(void)schedule_work(&chip->irq_work);

	return IRQ_HANDLED;
}

static struct axp_mfd_chip_ops axp_mfd_ops[] = {
	[0] = {
		.init_chip = axp22_init_chip,
		.enable_irqs = axp22_enable_irqs,
		.disable_irqs = axp22_disable_irqs,
		.read_irqs = axp22_read_irqs,
	},
};

static const struct i2c_device_id axp_mfd_id_table[] = {
	{ "axp22_mfd", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, axp_mfd_id_table);

int axp_mfd_create_attrs(struct axp_mfd_chip *chip)
{
	int j, ret;

	if (chip->type == AXP22) {
		for (j = 0; j < ARRAY_SIZE(axp22_mfd_attrs); j++) {
			ret = device_create_file(chip->dev,
				&axp22_mfd_attrs[j]);
			if (ret)
				goto sysfs_failed;
		}
	} else {
		ret = 0;
	}
	goto succeed;

sysfs_failed:
	while (j--)
		device_remove_file(chip->dev, &axp22_mfd_attrs[j]);
succeed:
	return ret;
}

static int __remove_subdev(struct device *dev, void *unused)
{
	platform_device_unregister(to_platform_device(dev));
	return 0;
}

static int axp_mfd_remove_subdevs(struct axp_mfd_chip *chip)
{
	return device_for_each_child(chip->dev, NULL, __remove_subdev);
}

static void axp_power_off(void)
{
	uint8_t val;
	int ret = 0;

	if (SHUTDOWNVOL >= 2600 && SHUTDOWNVOL <= 3300) {
		if (SHUTDOWNVOL > 3200)
			val = 0x7;
		else if (SHUTDOWNVOL > 3100)
			val = 0x6;
		else if (SHUTDOWNVOL > 3000)
			val = 0x5;
		else if (SHUTDOWNVOL > 2900)
			val = 0x4;
		else if (SHUTDOWNVOL > 2800)
			val = 0x3;
		else if (SHUTDOWNVOL > 2700)
			val = 0x2;
		else if (SHUTDOWNVOL > 2600)
			val = 0x1;
		else
			val = 0x0;

		axp_update(&axp->dev, AXP22_VOFF_SET, val, 0x7);
	}

	val = 0xff;

	pr_debug("[axp] send power-off command!\n");

	mdelay(20);

	if (POWER_START != 1) {
		axp_read(&axp->dev, AXP22_STATUS, &val);
		if (val & 0xF0) {
			axp_read(&axp->dev, AXP22_MODE_CHGSTATUS, &val);
			if (val & 0x20) {
				/* pr_err("[axp] set flag!\n"); */
				axp_write(&axp->dev, AXP22_BUFFERC, 0x0f);
				mdelay(20);
			}
		}
	}
	axp_write(&axp->dev, AXP22_BUFFERC, 0x00);
	mdelay(20);
	ret = axp_set_bits(&axp->dev, AXP22_OFF_CTL, 0x80);
	if (ret < 0) {
		pr_err("[axp] power-off cmd error!, retry!");
		ret = axp_set_bits(&axp->dev, AXP22_OFF_CTL, 0x80);
	}
}

static struct mfd_cell axp_devs[] = {
	{ .name = "axp228-regulator", },
	/* { .name = "axp228-supplyer", }, */
};

#ifdef CONFIG_OF
static const struct of_device_id axp228_pmic_dt_match[] = {
	{ .compatible = "x-powers,axp228", .data = NULL },
	{},
};

static struct axp_platform_data *
	axp228_i2c_parse_dt_pdata(struct device *dev)
{
	struct axp_platform_data *pd;
	struct device_node *np;
	u32 val;

	pd = devm_kzalloc(dev, sizeof(*pd), GFP_KERNEL);
	if (!pd)
		return NULL;

	np = of_node_get(dev->of_node);

	if (!of_property_read_u32(np, "nx,id", &val))
		pd->id = val;
	else
		dev_err(dev, "%s() Error : id\n", __func__);


	return pd;
}
#else
static struct axp_platform_data *
axp228_i2c_parse_dt_pdata(struct device *dev)
{
	return 0;
}
#endif

static int axp_mfd_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct axp_mfd_chip *axp228 = NULL;
	struct axp_platform_data *pdata = NULL;
	int ret = 0;

	DBG_MSG("## [%s():%d]\n", __func__, __LINE__);

	axp228 = devm_kzalloc(&client->dev,
		sizeof(struct axp_mfd_chip), GFP_KERNEL);
	if (!axp228)
		return -ENOMEM;

	axp = client;

	axp228->dev = &client->dev;
	axp228->client = client;
	axp228->irq = client->irq;
	axp228->ops = &axp_mfd_ops[0];

	i2c_set_clientdata(client, axp228);

	if (client->dev.of_node) {
		const struct of_device_id *match;

		match = of_match_device(of_match_ptr(axp228_pmic_dt_match),
					&client->dev);
		if (!match) {
			dev_err(
			    &client->dev,
			    "%s() Error: No device match found\n",
			    __func__);
			goto out_free_chip;
		}
	} else {
		dev_err(&client->dev,
			"%s() Error: No device match found\n",
			__func__);
		goto out_free_chip;
	}

	if (axp228->dev->of_node) {
		pdata = axp228_i2c_parse_dt_pdata(axp228->dev);
		if (!pdata)
			goto out_free_chip;
	}

	axp228->pdata = pdata;

	mutex_init(&axp228->lock);
	INIT_WORK(&axp228->irq_work, axp_mfd_irq_work);
	BLOCKING_INIT_NOTIFIER_HEAD(&axp228->notifier_list);

#ifdef ENABLE_DEBUG
	axp_mfd_register_dump(axp228->dev);
#endif

	axp_debuginit(axp228);

	ret = axp228->ops->init_chip(axp228);
	if (ret) {
		dev_err(&client->dev, "%s() Error: init_chip()\n",
			__func__);
		goto out_free_chip;
	}

	g_chip = axp228;

	if (client->irq > 0) {
		ret = devm_request_threaded_irq(&client->dev,
						axp228->irq,
						NULL,
						axp_mfd_irq_handler,
						IRQF_TRIGGER_FALLING |
						IRQF_ONESHOT,
						client->name,
						axp228);
		if (ret) {
			dev_err(&client->dev, "failed to request irq %d\n",
				client->irq);
			goto out_free_chip;
		}
	}

	ret = mfd_add_devices(axp228->dev, -1, axp_devs, ARRAY_SIZE(axp_devs),
			      NULL, 0, NULL);
	if (ret)
		goto out_free_irq;

	pm_power_off = axp_power_off;

	ret = axp_mfd_create_attrs(axp228);
	if (ret)
		return ret;

	return 0;

out_free_irq:
	free_irq(client->irq, axp228);

out_free_chip:
	i2c_set_clientdata(client, NULL);
	kfree(axp228);

	return ret;
}

static int axp_mfd_remove(struct i2c_client *client)
{
	struct axp_mfd_chip *chip = i2c_get_clientdata(client);

	axp_mfd_remove_subdevs(chip);
	kfree(chip);
	return 0;
}

static struct i2c_driver axp_mfd_driver = {
	.driver	= {
		.name = "axp_mfd",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(axp228_pmic_dt_match),
	},
	.probe = axp_mfd_probe,
	.remove = axp_mfd_remove,
	.id_table = axp_mfd_id_table,
};

static int __init axp_mfd_init(void) { return i2c_add_driver(&axp_mfd_driver); }
subsys_initcall(axp_mfd_init);

static void __exit axp_mfd_exit(void) { i2c_del_driver(&axp_mfd_driver); }
module_exit(axp_mfd_exit);

MODULE_DESCRIPTION("MFD Driver for X-Powers AXP228 PMIC");
MODULE_AUTHOR("Jongsin Park <pjsin865@nexell.co.kr>");
MODULE_LICENSE("GPL");
