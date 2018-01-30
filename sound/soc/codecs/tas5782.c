/*
 * TAS5782 amplifier audio driver
 *
 * Copyright (C) 2017 Allan Park <allan.park@nexell.co.kr>
 * Copyright (C) 2015 Google, Inc.
  * Copyright (c) 2013 Daniel Mack <zonque@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/stddef.h>
#include <linux/device.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>

#include "tas5782.h"

#define TAS5782_MAX_SUPPLIES		6

struct tas5782_chip {
	const char			*const *supply_names;
	int				num_supply_names;
	const struct snd_kcontrol_new	*controls;
	int				num_controls;
	const struct regmap_config	*regmap_config;
	int				vol_reg_size;
};

struct tas5782_private {
	const struct tas5782_chip	*chip;
	struct regmap			*regmap;
	struct regulator_bulk_data	supplies[TAS5782_MAX_SUPPLIES];
	struct clk			*mclk;
	unsigned int			format;
	struct gpio_desc		*reset_gpio;
	struct gpio_desc		*mute_gpio;
	struct gpio_desc		*fault_gpio;
	struct snd_soc_codec_driver	codec_driver;
};

/*
 * error code
 */
enum eq_update_errno {
	eq_err_file_read = -4,
	eq_err_file_open = -3,
	eq_err_file_type = -2,
	eq_err_download = -1,
	eq_err_none = 0,
	eq_err_uptodate = 1,
};

#define TAS5782_EQ_MAX 2048
#define EQ_PATH_EXTERNAL "/sdcard/tas5782.eq"

static cfg_reg eq_regs[TAS5782_EQ_MAX];

static int tas5782_register_size(struct tas5782_private *priv, unsigned int reg)
{
	switch (reg) {
	case TAS5782_DIGITAL_VOL_REG:
	case TAS5782_DIGITAL_LEFT_VOL_REG:
	case TAS5782_DIGITAL_RIGHT_VOL_REG:
		return priv->chip->vol_reg_size;
	default:
		return 1;
	}
}

static int tas5782_reg_write(void *context, unsigned int reg,
			     unsigned int value)
{
	struct i2c_client *client = context;
	struct tas5782_private *priv = i2c_get_clientdata(client);
	unsigned int i, size;
	uint8_t buf[5];
	int ret;

	size = tas5782_register_size(priv, reg);
	buf[0] = reg;

	for (i = size; i >= 1; --i) {
		buf[i] = value;
		value >>= 8;
	}

	ret = i2c_master_send(client, buf, size + 1);
	if (ret == size + 1)
		return 0;
	else if (ret < 0) {
		dev_err(&client->dev, "tas 5782m ret < 0 \n");
		return ret;
	}
	else {
		dev_err(&client->dev, "tas 5782m ret -EIO \n");
		return -EIO;
	}
}

static int tas5782_reg_read(void *context, unsigned int reg,
			    unsigned int *value)
{
	struct i2c_client *client = context;
	struct tas5782_private *priv = i2c_get_clientdata(client);
	uint8_t send_buf, recv_buf[4];
	struct i2c_msg msgs[2];
	unsigned int size;
	unsigned int i;
	int ret;

	size = tas5782_register_size(priv, reg);
	send_buf = reg;

	msgs[0].addr = client->addr;
	msgs[0].len = sizeof(send_buf);
	msgs[0].buf = &send_buf;
	msgs[0].flags = 0;

	msgs[1].addr = client->addr;
	msgs[1].len = size;
	msgs[1].buf = recv_buf;
	msgs[1].flags = I2C_M_RD;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0)
		return ret;
	else if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*value = 0;

	for (i = 0; i < size; i++) {
		*value <<= 8;
		*value |= recv_buf[i];
	}

	return 0;
}

static int _init_sequence(struct i2c_client *client,
							cfg_reg *r, int n)
{
	int i = 0;
	int ret;

	while (i < n) {
		switch (r[i].command) {
			case CFG_META_SWITCH:
				/*  Used in legacy applications.  Ignored here. */
				break;
			case CFG_META_DELAY:
				udelay(r[i].param);
				break;
			case CFG_META_BURST:
				ret = i2c_master_send(client, (unsigned char *)&r[i+1],
						r[i].param);
				dev_err(&client->dev, "tas 5782m ret =%d", ret);
				i +=  (r[i].param + 1)/2;
				break;
			default:
				tas5782_reg_write(client, r[i].offset, r[i].value);
				break;
		}
		i++;
	}
	return 0;
}

static int  tas5782_eq_adjust(struct i2c_client *client, const u8 *eq_data,
								size_t eq_size)
{
	char *tok;
	long lval;
	int  ret, cnt = 0;

	while(true) {
		/* parse command */
		tok = strsep(&eq_data, " ");
		if(!tok) {
			break;
		}
		ret = kstrtol(tok, 0, &lval);
        dev_err(&client->dev, "command:%x ", lval);
		eq_regs[cnt].command = lval;

		/*  parse param */
		tok = strsep(&eq_data, " ");
		if(!tok)
			break;

		ret = kstrtol(tok, 0, &lval);
		dev_err(&client->dev, "param:%x \n", lval);
		eq_regs[cnt].param = lval;

		cnt++;
		dev_err(&client->dev, "command cnt=%d\n", cnt);
	}
	ret = _init_sequence(client, eq_regs, cnt);

	return ret;
}

/*
 * Update eq from external storage
 */
static int tas5782_eq_update_from_storage(struct i2c_client *client, char *path)
{
	struct file *fp;
	mm_segment_t old_fs;
	unsigned char *eq_data;
	size_t eq_size, nread;
	   int ret;

	/* Get eq */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(path, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		dev_err(&client->dev,"%s [ERROR] file_open - path[%s]  err value:%d \n",
				__func__, path, PTR_ERR(fp));
		ret = eq_err_file_open;
		goto error;
	}

	eq_size = fp->f_path.dentry->d_inode->i_size;

	if (eq_size > 0) {
		/* Read eq */
		eq_data = kzalloc(eq_size, GFP_KERNEL);
		nread = vfs_read(fp, (char __user *)eq_data, eq_size, &fp->f_pos);
		dev_dbg(&client->dev, "%s - path[%s] size[%zu]\n", __func__,
				path, eq_size);

		if (nread != eq_size) {
			dev_err(&client->dev, "%s [ERROR] vfs_read - \
					size[%zu] read[%zu]\n", __func__, eq_size, nread);
			ret = eq_err_file_read;
		} else {
			/* Update eq */
			ret = tas5782_eq_adjust(client, eq_data, eq_size);
		}

		kfree(eq_data);
	} else {
		dev_err(&client->dev, "%s [ERROR] eq_size[%zu]\n", __func__, eq_size);
		ret = eq_err_file_read;
	}

	filp_close(fp, current->files);

error:
	set_fs(old_fs);

	if (ret < eq_err_none) {
		dev_err(&client->dev, "%s [ERROR]\n", __func__);
	} else {
		dev_dbg(&client->dev, "%s [DONE]\n", __func__);
	}

	return ret;
}

static ssize_t tas5782_sys_eq_update_from_storage(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	int result;
	u8 data[255];
	int ret = 0;
	struct i2c_client *client = to_i2c_client(dev);

	ret = tas5782_eq_update_from_storage(client, EQ_PATH_EXTERNAL);

	switch (ret) {
		case eq_err_none:
			snprintf(data, sizeof(data),"EQ update success.\n");
			break;
		case eq_err_uptodate:
			snprintf(data, sizeof(data),"EQ is already up-to-date.\n");
			break;
		case eq_err_download:
			snprintf(data, sizeof(data),"EQ update failed : Download error\n");
			break;
		case eq_err_file_type:
			snprintf(data, sizeof(data),"EQ update failed : File type error\n");
			break;
		case eq_err_file_open:
			snprintf(data, sizeof(data),
					"EQ update failed : File open error[%s]\n",
					EQ_PATH_EXTERNAL);
			break;
		case eq_err_file_read:
			snprintf(data, sizeof(data),
					"EQ update failed : File read error\n");
			break;
		default:
			snprintf(data, sizeof(data), "EQ update failed.\n");
			break;
	}

	dev_dbg(&client->dev, "%s [DONE]\n", __func__);

	result = snprintf(buf, 255, "%s\n", data);
    return count;

}

static int tas5782_mute(struct tas5782_private *priv, int mute)
{
    if (gpio_is_valid(priv->mute_gpio)) {
        /* Reset codec - minimum assertion time is 400ns */
        gpio_set_value(priv->mute_gpio,  !mute);
        /* Codec needs ~15ms to wake up */
        msleep(15);
	}
}

static void tas5782_reset(struct tas5782_private *priv)
{
    if (gpio_is_valid(priv->reset_gpio)) {
        /* Reset codec - minimum assertion time is 400ns */
        gpio_direction_output(priv->reset_gpio, 0);
        udelay(1);
        gpio_set_value(priv->reset_gpio, 1);

        /* Codec needs ~15ms to wake up */
        msleep(15);
    }
}

static int tas5782_set_dai_fmt(struct snd_soc_dai *dai, unsigned int format)
{
	struct tas5782_private *priv = snd_soc_codec_get_drvdata(dai->codec);

	priv->format = format;

	return 0;
}

static int tas5782_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params,
			     struct snd_soc_dai *dai)
{
	struct tas5782_private *priv = snd_soc_codec_get_drvdata(dai->codec);
	u32 val;

	switch (priv->format & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_RIGHT_J:
		val = 0x00;
		break;
	case SND_SOC_DAIFMT_I2S:
		val = 0x03;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		val = 0x06;
		break;
	default:
		return -EINVAL;
	}

	if (params_width(params) >= 24)
		val += 2;
	else if (params_width(params) >= 20)
		val += 1;

	/* return regmap_update_bits(priv->regmap, TAS5782_SDI_REG,
					  TAS5782_SDI_FMT_MASK, val); */
	return 1;
}

static int tas5782_set_bias_level(struct snd_soc_codec *codec,
				  enum snd_soc_bias_level level)
{
	struct tas5782_private *priv = snd_soc_codec_get_drvdata(codec);
	int ret;

	switch (level) {
	case SND_SOC_BIAS_ON:
		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		if (snd_soc_codec_get_bias_level(codec) == SND_SOC_BIAS_OFF) {
			if (!IS_ERR(priv->mclk)) {
				ret = clk_prepare_enable(priv->mclk);
				if (ret) {
					dev_err(codec->dev,
						"Failed to enable master clock: %d\n",
						ret);
					return ret;
				}
			}

			/* gpiod_set_value(priv->reset_gpio, 0);
			usleep_range(5000, 6000); */

			regcache_cache_only(priv->regmap, false);
			ret = regcache_sync(priv->regmap);
			if (ret)
				return ret;
		}
		break;
	case SND_SOC_BIAS_OFF:
		regcache_cache_only(priv->regmap, true);
		/* gpiod_set_value(priv->reset_gpio, 1); */

		if (!IS_ERR(priv->mclk))
			clk_disable_unprepare(priv->mclk);
		break;
	}

	return 0;
}

static const struct snd_soc_dai_ops tas5782_dai_ops = {
	.set_fmt	= tas5782_set_dai_fmt,
	.hw_params	= tas5782_hw_params,
};

static const char *const tas5782_supply_names[] = {
	"AVDD",
	"DVDD",
	"PVDD_A",
	"PVDD_B",
	"PVDD_C",
	"PVDD_D",
};

static const DECLARE_TLV_DB_SCALE(tas5782_volume_tlv, -10350, 50, 1);

static const struct snd_kcontrol_new tas5782_controls[] = {
	SOC_DOUBLE_R_TLV("Master Volume",
			 TAS5782_DIGITAL_LEFT_VOL_REG,
			 TAS5782_DIGITAL_RIGHT_VOL_REG,
			 0, 0xff, 1, tas5782_volume_tlv),
	SOC_DOUBLE("Speaker Switch",
		   TAS5782_SOFT_MUTE_REG,
		   TAS5782_SOFT_MUTE_RIGHT_SHIFT, TAS5782_SOFT_MUTE_LEFT_SHIFT,
		   1, 1),
};
/*
static const struct reg_default tas5782_reg_defaults[] = {
	{ 0x04, 0x05 },
	{ 0x05, 0x40 },
	{ 0x06, 0x00 },
};
*/

static const struct regmap_config tas5782_regmap_config = {
	.reg_bits			= 8,
	.val_bits			= 8,
	.max_register			= 0xff,
	.reg_read			= tas5782_reg_read,
	.reg_write			= tas5782_reg_write,
	/* .reg_defaults			= tas5782_reg_defaults,
	 .num_reg_defaults		= ARRAY_SIZE(tas5782_reg_defaults), */
	.cache_type			= REGCACHE_RBTREE,
};

static const struct tas5782_chip tas5782_chip = {
	.supply_names			= tas5782_supply_names,
	.num_supply_names		= ARRAY_SIZE(tas5782_supply_names),
	.controls			= tas5782_controls,
	.num_controls			= ARRAY_SIZE(tas5782_controls),
	.regmap_config			= &tas5782_regmap_config,
	.vol_reg_size			= 1,
};


static const struct snd_soc_dapm_widget tas5782_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DACL", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("DACR", NULL, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_OUTPUT("OUT_A"),
	SND_SOC_DAPM_OUTPUT("OUT_B"),
	SND_SOC_DAPM_OUTPUT("OUT_C"),
	SND_SOC_DAPM_OUTPUT("OUT_D"),
};

static const struct snd_soc_dapm_route tas5782_dapm_routes[] = {
	{ "DACL",  NULL, "Playback" },
	{ "DACR",  NULL, "Playback" },

	{ "OUT_A", NULL, "DACL" },
	{ "OUT_B", NULL, "DACL" },
	{ "OUT_C", NULL, "DACR" },
	{ "OUT_D", NULL, "DACR" },
};

static const struct snd_soc_codec_driver tas5782_codec = {
	.set_bias_level = tas5782_set_bias_level,
	.idle_bias_off = true,

	.dapm_widgets = tas5782_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(tas5782_dapm_widgets),
	.dapm_routes = tas5782_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(tas5782_dapm_routes),
};

static struct snd_soc_dai_driver tas5782_dai = {
	.name = "tas5782-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = SNDRV_PCM_FMTBIT_S32_LE |
			   SNDRV_PCM_FMTBIT_S24_LE |
			   SNDRV_PCM_FMTBIT_S16_LE,
	},
	.ops = &tas5782_dai_ops,
};

static DEVICE_ATTR(eq_update_storage, S_IWUSR, NULL,
		tas5782_sys_eq_update_from_storage);

static const struct of_device_id tas5782_of_match[];

static int tas5782_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	struct tas5782_private *priv;
	struct device *dev = &client->dev;
	int gpio_nreset = -EINVAL;
	int gpio_nmute = -EINVAL;
	int gpio_nfault = -EINVAL;
	const struct of_device_id *of_id;
	struct device_node *of_node;
	int i, ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	i2c_set_clientdata(client, priv);

	of_id = of_match_device(tas5782_of_match, dev);
	if (!of_id) {
		dev_err(dev, "Unknown device type\n");
		return -EINVAL;
	}
	of_node = dev->of_node;
	priv->chip = of_id->data;

	BUG_ON(priv->chip->num_supply_names > TAS5782_MAX_SUPPLIES);
	for (i = 0; i < priv->chip->num_supply_names; i++)
		priv->supplies[i].supply = priv->chip->supply_names[i];

	ret = devm_regulator_bulk_get(dev, priv->chip->num_supply_names,
				      priv->supplies);
	if (ret) {
		dev_err(dev, "Failed to get supplies: %d\n", ret);
		return ret;
	}
	ret = regulator_bulk_enable(priv->chip->num_supply_names,
				    priv->supplies);
	if (ret) {
		dev_err(dev, "Failed to enable supplies: %d\n", ret);
		return ret;
	}

	priv->regmap = devm_regmap_init(dev, NULL, client,
					priv->chip->regmap_config);
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	/* reset */

	gpio_nreset = of_get_named_gpio(of_node, "reset-gpios", 0);
	if (gpio_is_valid(gpio_nreset)) {
		if (devm_gpio_request(dev, gpio_nreset, "TAS5086 Reset"))
			 gpio_nreset = -EINVAL;

	priv->reset_gpio = gpio_nreset;
	gpio_direction_output(priv->reset_gpio, 0);
	}


	gpio_nmute = of_get_named_gpio(of_node, "mute-gpios", 0);
	if (gpio_is_valid(gpio_nmute)) {
		if (devm_gpio_request(dev, gpio_nmute, "TAS5086 Mute"))
			gpio_nmute = -EINVAL;

		priv->mute_gpio = gpio_nmute;
		gpio_direction_output(priv->mute_gpio, 1);
	}


	gpio_nfault = of_get_named_gpio(of_node, "fault-gpios", 0);
	if (gpio_is_valid(gpio_nfault)) {
		if (devm_gpio_request(dev, gpio_nfault, "TAS5086 Fault"))
			gpio_nfault = -EINVAL;

		priv->fault_gpio = gpio_nfault;
		gpio_direction_input(priv->fault_gpio);
	}

	tas5782_reset(priv);

/* The TAS5086 always returns 0x84 in its TAS5782_DEV_ID register */
	ret = regmap_read(priv->regmap, TAS5782_DEV_ID, &i);
	dev_info(dev, "TAS5782 codec ID = %02x\n", i);

	/* init tas5782 */
	_init_sequence(client, registers, sizeof(registers)/sizeof(registers[0]));

	memcpy(&priv->codec_driver, &tas5782_codec, sizeof(priv->codec_driver));
	priv->codec_driver.controls = priv->chip->controls;
	priv->codec_driver.num_controls = priv->chip->num_controls;

	regcache_cache_only(priv->regmap, true);

	ret = device_create_file(dev, &dev_attr_eq_update_storage);
	if (ret != 0) {
		dev_err(dev,
		 "Failed to create eq_update_storage sysfs files: %d\n", ret);
		return ret;
	}

	return snd_soc_register_codec(&client->dev, &priv->codec_driver,
				      &tas5782_dai, 1);
}

static int tas5782_i2c_remove(struct i2c_client *client)
{
	struct tas5782_private *priv = i2c_get_clientdata(client);

	snd_soc_unregister_codec(&client->dev);
	regulator_bulk_disable(priv->chip->num_supply_names, priv->supplies);

	device_remove_file(&client->dev, &dev_attr_eq_update_storage);

	return 0;
}

static const struct of_device_id tas5782_of_match[] = {
	{ .compatible = "ti,tas5782", .data = &tas5782_chip, },
	{ }
};
MODULE_DEVICE_TABLE(of, tas5782_of_match);

static const struct i2c_device_id tas5782_i2c_id[] = {
	{ "tas5782", 0x48 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tas5782_i2c_id);

static struct i2c_driver tas5782_i2c_driver = {
	.driver = {
		.name = "tas5782",
		.of_match_table = of_match_ptr(tas5782_of_match),
	},
	.probe = tas5782_i2c_probe,
	.remove = tas5782_i2c_remove,
	.id_table = tas5782_i2c_id,
};
module_i2c_driver(tas5782_i2c_driver);

MODULE_DESCRIPTION("ASoC TAS5782 driver");
MODULE_AUTHOR("Allan park <allan.park@nexell.co.kr>");
MODULE_LICENSE("GPL");
