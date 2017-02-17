/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Hyunseok Jung <hsjung@nexell.co.kr>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <sound/jack.h>
#include "nexell-i2s.h"
#ifdef CONFIG_GPIOLIB
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
int alc5623_amp_io;
#endif
#define I2S_BASEADDR 0xC0055000
#define I2S_CH_OFFSET 0x1000

#define DEBUG

static char str_dai_name[16] = "c0055000.i2s";
static int (*cpu_resume_fn)(struct snd_soc_dai *dai);
static struct snd_soc_codec *alc5623;
static int codec_bias_level;

/* sound DAI (I2S/SPDIF and codec interface) */
struct nx_snd_jack_pin {
	int support;
	int detect_level;
	int detect_io;
	int debounce_time;
};

static int alc5623_jack_status_check(void *data);
/* Headphones jack detection GPIO */
static struct snd_soc_jack_gpio jack_gpio = {
	.invert	= false,	/* High detect : invert = false */
	.name = "hp-gpio",
	.report = SND_JACK_HEADPHONE,
	.debounce_time = 200,
	.jack_status_check = alc5623_jack_status_check,
};

static struct snd_soc_jack hp_jack;

static int alc5623_jack_status_check(void *data)
{
	struct snd_soc_codec *codec = alc5623;
	int jack = jack_gpio.gpio;
	int invert = jack_gpio.invert;
	int level = gpio_get_value_cansleep(jack);

	if (!codec)
		return -1;

	if (invert)
		level = !level;

	dev_dbg(codec->dev, "%s: hp jack %s\n", __func__, level?"IN":"OUT");

	if (!level) {
		/* TODO: you want */
		dev_dbg(codec->dev, "hp jack ejected\n");
	} else {
		/* TODO: you want */
		dev_dbg(codec->dev, "hp jack inserted\n");
	}
	return level;
}

static int alc5623_hw_params(struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	/* 48K * 256 = 12.288 Mhz */
	unsigned int freq = params_rate(params) * 256;
	unsigned int fmt = SND_SOC_DAIFMT_I2S |
			SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS;
	int ret = 0;

	ret = snd_soc_dai_set_sysclk(codec_dai, 0, freq,
					SND_SOC_CLOCK_IN);
	if (0 > ret)
		return ret;

	ret = snd_soc_dai_set_fmt(codec_dai, fmt);

	if (0 > ret)
		return ret;
	return ret;
}

static int alc5623_suspend_pre(struct snd_soc_card *card)
{
	dev_dbg(card->dev, "+%s\n", __func__);
#ifdef CONFIG_GPIOLIB
	if (alc5623_amp_io > 0)
		gpio_set_value(alc5623_amp_io, 0);
#endif
	return 0;
}

static int alc5623_resume_pre(struct snd_soc_card *card)
{
	struct snd_soc_dai *cpu_dai = card->rtd->cpu_dai;
	struct snd_soc_codec *codec = alc5623;
	struct snd_soc_dapm_context *dapm = snd_soc_codec_get_dapm(codec);
	int ret = 0;

	dev_dbg(card->dev, "+%s\n", __func__);

	/*
	 * first execute cpu(i2s) resume and execute codec resume.
	 */
	if (cpu_dai->driver->resume && !cpu_resume_fn) {
		cpu_resume_fn = cpu_dai->driver->resume;
		cpu_dai->driver->resume = NULL;
	}

	if (cpu_resume_fn)
		ret = cpu_resume_fn(cpu_dai);
	dev_dbg(card->dev, "-%s\n", __func__);
	codec_bias_level = dapm->bias_level;

	return ret;
}

static int alc5623_resume_post(struct snd_soc_card *card)
{
	struct snd_soc_codec *codec = alc5623;
	struct snd_soc_dapm_context *dapm = snd_soc_codec_get_dapm(codec);
	int jack = jack_gpio.gpio;
	int invert = jack_gpio.invert;
	int level = gpio_get_value_cansleep(jack);

	dev_dbg(card->dev, "%s BAIAS=%d, PRE=%d\n", __func__,
		dapm->bias_level, codec_bias_level);

	if (!codec)
		return -1;

	if (SND_SOC_BIAS_OFF != codec_bias_level)
		codec->driver->resume(codec);

	if (NULL == jack_gpio.name)
		return 0;

	if (invert)
		level = !level;

	dev_dbg(card->dev, "%s: hp jack %s\n", __func__, level?"IN":"OUT");

	if (!level) {
		/* TODO: you want */
		dev_dbg(codec->dev, "hp jack ejected\n");
	} else {
		/* TODO: you want */
		dev_dbg(codec->dev, "hp jack inserted\n");
	}
	snd_soc_jack_report(&hp_jack, level, jack_gpio.report);
	return 0;
}

static struct snd_soc_ops alc5623_ops = {
	.hw_params = alc5623_hw_params,
};

static int alc5623_spk_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
#ifdef CONFIG_GPIOLIB
	if (alc5623_amp_io > 0) {
		if (SND_SOC_DAPM_EVENT_ON(event))
			gpio_set_value(alc5623_amp_io, 1);
		else
			gpio_set_value(alc5623_amp_io, 0);
	}
#endif
	return 0;
}

/* alc5623 machine dapm widgets */
static const struct snd_soc_dapm_widget alc5623_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("Ext Spk", alc5623_spk_event),
};

/* Corgi machine audio map (connections to the codec pins) */
static const struct snd_soc_dapm_route alc5623_audio_map[] = {
	/* speaker connected to HPOL, HPOR */ /* TODO: change initial path */
	{"Ext Spk", NULL, "SPKOUT"},
	{"Ext Spk", NULL, "SPKOUTN"},
};

/* Headphones jack detection DAPM pin */
static struct snd_soc_jack_pin jack_pins[] = {
	{
		.pin = "Ext Spk", /* TODO: change initial path */
		.mask = SND_JACK_HEADPHONE,
	},
};

static int alc5623_dai_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = snd_soc_codec_get_dapm(codec);
	struct snd_soc_jack_gpio *jack = &jack_gpio;
	int ret;

	alc5623 = codec;

	if (NULL == jack->name)
		return 0;

	/* Headset jack detection */
	ret = snd_soc_card_jack_new(rtd->card, "Ext Spk",
					SND_JACK_HEADPHONE, &hp_jack, jack_pins,
					ARRAY_SIZE(jack_pins));
	if (ret)
		return ret;
	/* to power up alc5623 (HP Depop: hp_event) */
	snd_soc_dapm_enable_pin(dapm, "Ext Spk");
	/* TODO: change initial path */
	snd_soc_dapm_sync(dapm);

	ret = snd_soc_jack_add_gpios(&hp_jack, 1, jack);
	if (ret)
		dev_err(codec->dev, "Fail, register audio jack detect, io [%d]...\n",
				jack->gpio);
	return 0;
}

static struct snd_soc_dai_link alc5623_dai_link = {
	.name = "ASOC-ALC5623",
	.stream_name = "ALC5623 HiFi",
	.cpu_dai_name = str_dai_name,
	.platform_name = "nexell-pcm",
	.codec_dai_name = "alc5623-hifi",
	.ops = &alc5623_ops,
	.symmetric_rates = 1,
	.init = alc5623_dai_init,
};

static struct snd_soc_card alc5623_card = {
	.name = "I2S-ALC5623", /* proc/asound/cards */
	.owner = THIS_MODULE,
	.dai_link = &alc5623_dai_link,
	.num_links = 1,
	.suspend_pre = &alc5623_suspend_pre,
	.resume_pre = &alc5623_resume_pre,
	.resume_post = &alc5623_resume_post,
	.dapm_widgets = alc5623_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(alc5623_dapm_widgets),
	.dapm_routes = alc5623_audio_map,
	.num_dapm_routes = ARRAY_SIZE(alc5623_audio_map),
};

/*
 * codec driver
 */
static int alc5623_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &alc5623_card;
	struct snd_soc_jack_gpio *jack = &jack_gpio;
	struct snd_soc_dai_driver *i2s_dai = NULL;
	struct nx_snd_jack_pin hpin = {0.};
	unsigned int rates = 0, format = 0;
	int ret;
	int ch;
	const char *format_name;

	/* set I2S name */
	of_property_read_u32(pdev->dev.of_node, "ch", &ch);
	sprintf(str_dai_name, "%x%s", (I2S_BASEADDR + (ch * I2S_CH_OFFSET)),
		".i2s");
	of_property_read_u32(pdev->dev.of_node, "sample-rate", &rates);
	format_name = of_get_property(pdev->dev.of_node, "format", NULL);
	if (format_name != NULL) {
		if (strcmp(format_name, "S16") == 0)
			format = SNDRV_PCM_FMTBIT_S16_LE;
		else if (strcmp(format_name, "S24") == 0) {
			format = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE;
		}
	}
	of_property_read_u32(pdev->dev.of_node, "hpin-support", &hpin.support);
	if (hpin.support) {
		hpin.detect_io = of_get_named_gpio(pdev->dev.of_node,
		  "hpin-gpio", 0);
		of_property_read_u32(pdev->dev.of_node, "hpin-level",
		  &hpin.detect_level);
		jack->gpio = hpin.detect_io;
		jack->invert = hpin.detect_level ? false : true;
		jack->debounce_time = hpin.debounce_time ?
		   hpin.debounce_time : 200;
	} else {
		jack->name = NULL;
	}
#ifdef CONFIG_GPIOLIB
	alc5623_amp_io = of_get_named_gpio(pdev->dev.of_node, "amp-gpio", 0);
	if (gpio_is_valid(alc5623_amp_io)) {
		ret = devm_gpio_request(&pdev->dev, alc5623_amp_io,
					"alc5623_amp_en");
		if (ret < 0)
			dev_err(&pdev->dev,
			  "can't request amp gpio %d\n", alc5623_amp_io);
		ret = gpio_direction_output(alc5623_amp_io, 0);
		if (ret < 0) {
			dev_err(&pdev->dev, "can't request output direction");
			dev_err(&pdev->dev, "amp gpio %d\n", alc5623_amp_io);
		}
	} else {
		dev_err(&pdev->dev, "alc5623_amp_io is invalid pin(alc5623_amp_io #%d)\n",
				alc5623_amp_io);
	}
#endif
	card->dev = &pdev->dev;
	if (!alc5623_dai_link.codec_name) {
		alc5623_dai_link.codec_name = NULL;
		alc5623_dai_link.codec_of_node = of_parse_phandle(
		pdev->dev.of_node, "audio-codec", 0);
		if (!alc5623_dai_link.codec_of_node) {
			dev_err(&pdev->dev,
			    "Property 'audio-codec' missing or invalid\n");
			ret = -EINVAL;
		}
	}

	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev,
			"snd_soc_register_card() failed: %d\n", ret);
		return ret;
	}

	if (card->rtd) {
		struct snd_soc_dai *cpu_dai = card->rtd->cpu_dai;

		if (cpu_dai)
			i2s_dai = cpu_dai->driver;
	}
	dev_dbg(&pdev->dev, "alc5623-dai: register card %s -> %s\n",
			card->dai_link->codec_dai_name,
			card->dai_link->cpu_dai_name);
	if (NULL == i2s_dai)
		return 0;
	/*
	 * Reset i2s sample rates
	 */
	if (rates) {
		rates = snd_pcm_rate_to_rate_bit(rates);
		if (SNDRV_PCM_RATE_KNOT == rates)
			dev_err(&pdev->dev, "%s, invalid sample rates=%d\n",
					__func__, rates);
		else {
			i2s_dai->playback.rates = rates;
			i2s_dai->capture.rates = rates;
		}
	}
	/*
	 * Reset i2s format
	 */
	if (format) {
		i2s_dai->playback.formats = format;
		i2s_dai->capture.formats = format;
	}

	return ret;
}

static int alc5623_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	snd_soc_unregister_card(card);
#ifdef CONFIG_GPIOLIB
	if (alc5623_amp_io > 0) {
		gpio_set_value(alc5623_amp_io, 0);
		devm_gpio_free(&pdev->dev, alc5623_amp_io);
	}
#endif
	return 0;
}
#ifdef CONFIG_OF
static const struct of_device_id nx_alc5623_match[] = {
	{ .compatible = "nexell,nexell-alc5623" },
	{},
};
MODULE_DEVICE_TABLE(of, nx_alc5623_match);
#else
#define nx_alc5623_match NULL
#endif

static struct platform_driver alc5623_driver = {
	.driver = {
		.name = "alc5623-audio",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops, /* for suspend */
		.of_match_table = of_match_ptr(nx_alc5623_match),
	},
	.probe = alc5623_probe,
	.remove = alc5623_remove,
};
module_platform_driver(alc5623_driver);

MODULE_AUTHOR("hsjung <hsjung@nexell.co.kr>");
MODULE_DESCRIPTION("Sound codec-alc5623 driver for Nexell sound");
MODULE_LICENSE("GPL");
