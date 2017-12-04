/*
 * (C) Copyright 2015
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/pwm.h>
#include <sound/soc.h>

#include "nexell-pcm.h"
#include "nexell-spi-pl022.h"

#define	SVI_IOBASE_MAX		5
#define	DEF_SYNC_DEV_NUM	2 /* spi, i2sn */
#define	SVI_PDM_PLL		8000000	/* 8Mhz */

enum svoice_dev_type {
	SVI_DEV_I2S,
	SVI_DEV_SPI,
};

enum svoice_pin_type {
	SVI_PIN_SPI_CS,
	SVI_PIN_PDM_LCRCK,
	SVI_PIN_PDM_ISRUN,
	SVI_PIN_PDM_NRST,
	SVI_PIN_I2S_LRCK,
};

#define IO_GROUP(io)	((io >> 0x5) & 0x07)
#define IO_OFFSET(io)  	((io & 0x1F) >> 0x0)

struct svoice_pin {
	const char *property;
	int output;
	int nr;
	void __iomem *base;
	int group;
	int offset;
};

static struct svoice_pin svoice_pins[] = {
	[SVI_PIN_SPI_CS] = { "spi-cs-gpio", 1, -1, },		/* output */
	[SVI_PIN_PDM_LCRCK] = { "pdm-lrck-gpio", 1, -1, },	/* output */
	[SVI_PIN_PDM_ISRUN] = { "pdm-isrun-gpio", 1, -1, },	/* output */
	[SVI_PIN_PDM_NRST] = { "pdm-nrst-gpio", 1, -1, },	/* output */
	[SVI_PIN_I2S_LRCK] = { "i2s-lrck-gpio", 0, -1, },	/* status */
};
#define	SVI_PIN_NUM	ARRAY_SIZE(svoice_pins)

struct svoice_dev {
	struct snd_soc_dai *cpu_dai;
	unsigned long phybase;
	void __iomem *virtbase;
	enum svoice_dev_type type;
	bool start;
	const struct snd_soc_dai_ops *save_ops;
	struct snd_soc_dai_ops hook_ops;
	struct list_head list;
};

struct svoice_snd {
	struct device *dev;
#ifdef CONFIG_PWM
	struct pwm_device *pwm;
#endif
	struct list_head list;
	spinlock_t lock;
	int num_require_dev;
	bool detect;
	struct nx_pcm_rate_detector *rate_detector;
	void __iomem *io_bases[SVI_IOBASE_MAX];
};
#define us_to_ktime(u)  ns_to_ktime((u64)u * 1000)

#define	DETECT_LRCK_WAIT 2000

#define	find_list_dev(d, s, t)	{ \
	list_for_each_entry(d, &s->list, list)	\
		if (d->cpu_dai == t)	\
			break;	\
	}

/* SPI CS: L ON */
static inline void __spi_cs_set(void)
{
	struct svoice_pin *pin = &svoice_pins[SVI_PIN_SPI_CS];

	pr_debug("%s [gpio_%c.%02d] %s\n", __func__,
		('A' + pin->group), pin->offset, pin->property);

	gpio_set_value(pin->nr, 0);
}

/* SPI CS: H OFF */
static inline void __spi_cs_free(void)
{
	struct svoice_pin *pin = &svoice_pins[SVI_PIN_SPI_CS];

	pr_debug("%s [gpio_%c.%02d] %s\n", __func__,
		('A' + pin->group), pin->offset, pin->property);

	gpio_set_value(pin->nr, 1);
}

/* SSPSR */
static inline void __spi_clear(struct svoice_dev *sv)
{
	void __iomem *base = sv->virtbase;
	u32 val;

	/* wait for fifo flush */
	val = __raw_readl(SSP_SR(base));
	if (val & (1 << 2)) {
		pr_info("SPI: spi.%p rx not empty (0x%08x)!!!\n", base, val);
		while (__raw_readl(SSP_SR(base)) & (1 << 2))
			val = __raw_readl(SSP_DR(base));
	}
}

/* SSPCR1 = 0x04 */
static inline void __spi_start(struct svoice_dev *sv)
{
	void __iomem *base = sv->virtbase;

	pl022_enable(base, 1);
}

static inline void __i2s_clear(struct svoice_dev *sv)
{
	void __iomem *base = sv->virtbase;
	u32 val;

	writel(1<<7, (base + 0x08));	/* Clear the Rx Flush bit */
	writel(0, (base + 0x08));	/* Clear the Flush bit */

	val = readl(base + 0x08);
	if (val & (0x1f))
		pr_info("I2S: i2s.%p rx not empty (0x%08x)\n", base, val);
}

static inline void __i2s_start(struct svoice_dev *sv)
{
	void __iomem *base = sv->virtbase;
	u32 con, csr;

	con = (readl(base + 0x00) | ((1 << 1)) | (1 << 0));
	csr = (readl(base + 0x04) & ~(3 << 8)) | (1 << 8) | 0;
	writel(csr, (base+0x04));
	writel(con, (base+0x00));
}

static inline void __i2s_stop(struct svoice_dev *sv)
{
}

static inline bool __i2s_frame_wait(int wait)
{
	struct svoice_pin *pin = &svoice_pins[SVI_PIN_I2S_LRCK];
	int count, val;

	if (!gpio_is_valid(pin->nr))
		return false;

	/* wait Low */
	count = wait;
	do {
		val = readl(pin->base + 0x18) & (1 << pin->offset);
	} while (val && --count > 0);

	/* no Low -> no lrck */
	if (count == 0)
		return false;

	/* wait High */
	count = wait;
	do {
		val = readl(pin->base + 0x18) & (1 << pin->offset);
	} while (!val && --count > 0);

	/* no High : no lrck */
	if (count == 0)
		return false;

	/* wait Low */
	count = wait;
	do {
		val = readl(pin->base + 0x18) & (1 << pin->offset);
	} while (val && --count > 0);

	return true;
}

static inline bool __pin_start(struct svoice_snd *snd)
{
	struct svoice_pin *pin;
	bool lrck;
	u32 val;

	__spi_cs_set();

	lrck = __i2s_frame_wait(DETECT_LRCK_WAIT);

	/* pin: ISRUN */
	pin = &svoice_pins[SVI_PIN_PDM_ISRUN];
	val = readl(pin->base) | (1 << pin->offset);
	writel(val, pin->base);

	/* pin: LRCK */
	pin = &svoice_pins[SVI_PIN_PDM_LCRCK];
	val = readl(pin->base);
	if (lrck)
		val &= ~(1 << pin->offset);
	else
		val |= (1 << pin->offset);	/* no lrck */

	writel(val, pin->base);

	return lrck;
}

static inline void __pin_stop(void)
{
	struct svoice_pin *pin;
	u32 val;

	__spi_cs_free();

	/* pin: LRCK */
	pin = &svoice_pins[SVI_PIN_PDM_LCRCK];
	val = readl(pin->base) | (1 << pin->offset); /* no lrck */
	writel(val, pin->base);

	/* pin: ISRUN */
	pin = &svoice_pins[SVI_PIN_PDM_ISRUN];
	val = readl(pin->base) & ~(1 << pin->offset);
	writel(val, pin->base);

	pr_debug("%s [gpio_%c.%02d] %s\n", __func__,
		('A' + pin->group), pin->offset, pin->property);
}

static void __pin_nolrck(void *data)
{
	struct svoice_pin *pin;
	u32 val;

	/* pin: LRCK */
	pin = &svoice_pins[SVI_PIN_PDM_LCRCK];
	val = readl(pin->base) | (1 << pin->offset); /* no lrck */
	writel(val, pin->base);
}

static void __pin_prepare(void)
{
	struct svoice_pin *pin = &svoice_pins[SVI_PIN_PDM_NRST];
	u32 val;

	/*
	 * set nreset after pll insert
	 * to run with external pll
	 */
	if (gpio_is_valid(pin->nr)) {
		pr_debug("%s [gpio_%c.%02d] %s\n", __func__,
			('A' + pin->group), pin->offset, pin->property);

		val = readl(pin->base) & ~(1 << pin->offset);
		writel(val, pin->base);

		msleep(100);

		val = readl(pin->base) | (1 << pin->offset);
		writel(val, pin->base);
	}

	__pin_stop();
}

static int svoice_start(struct svoice_snd *snd)
{
	struct svoice_dev *sv;
	struct svoice_pin *pin = &svoice_pins[SVI_PIN_PDM_LCRCK];
	bool lrck;
	u32 val;

	dev_dbg(snd->dev, "run smart-voice ...\n");

	/* clear mask */
	list_for_each_entry(sv, &snd->list, list)
		sv->start = false;

	/* clear fifo */
	list_for_each_entry(sv, &snd->list, list)
		sv->type == SVI_DEV_SPI ?
			__spi_clear(sv) : __i2s_clear(sv);

	/* start */
	list_for_each_entry(sv, &snd->list, list)
		sv->type == SVI_DEV_SPI ?
			__spi_start(sv) : __i2s_start(sv);

	/* last set 'is run' gpio */
	lrck = __pin_start(snd);

	/* read LRCK status */
	val = readl(pin->base + 0x18) & (1 << pin->offset);

	if (snd->rate_detector) {
		/* wait multiple time */
		long duration = snd->rate_detector->duration_us * 2;

		hrtimer_start(&snd->rate_detector->timer,
			us_to_ktime(duration), HRTIMER_MODE_REL_PINNED);
	}

	pr_info("Smart Voice LRCK: %s (%s)\n",
		lrck ? "exist" : "no exist", val ? "H" : "L");

	return 0;
}

static void svoice_stop(struct svoice_dev *sv)
{
	if (sv->type != SVI_DEV_SPI)
		return;

	__pin_stop();
}

/* cpu dai trigger */
static int svoice_trigger_start(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct svoice_snd *snd = snd_soc_codec_get_drvdata(codec_dai->codec);
	struct svoice_dev *sv;
	unsigned long flags;
	int nr = 0, ret;

	spin_lock(&snd->lock);
	find_list_dev(sv, snd, rtd->cpu_dai);
	if (!sv) {
		spin_unlock(&snd->lock);
		return -1;
	}

	sv->start = true;

	dev_dbg(rtd->dev, "smart voice hook trigger\n");
	dev_dbg(rtd->dev, "%s <-> %s trigger\n", dai->name, codec_dai->name);

	list_for_each_entry(sv, &snd->list, list) {
		dev_dbg(rtd->dev, "%s.%p, %s %d - %dEA\n",
			sv->type == SVI_DEV_I2S ? "i2s" : "spi",
			sv->virtbase, sv->start ? "run":"stopped",
			nr + 1, snd->num_require_dev);
		if (!sv->start)
			break;
		nr++;
	}
	spin_unlock(&snd->lock);

	if (nr >= snd->num_require_dev) {
		spin_lock_irqsave(&snd->lock, flags);
		ret = svoice_start(snd);
		spin_unlock_irqrestore(&snd->lock, flags);
	}
	return ret;
}

/*
 * start : codec trigger -> cpu trigger
 * start : codec trigger -> cpu trigger
 */
static void hook_cpu_ops(struct snd_soc_pcm_runtime *runtime,
			struct snd_soc_dai *dai, bool hooking)
{
	struct snd_soc_dai *cpu_dai = runtime->cpu_dai;
	struct svoice_snd *snd = snd_soc_codec_get_drvdata(dai->codec);
	struct svoice_dev *sv;

	spin_lock(&snd->lock);
	find_list_dev(sv, snd, cpu_dai);
	if (!sv) {
		spin_unlock(&snd->lock);
		return;
	}
	spin_unlock(&snd->lock);

	dev_dbg(dai->dev, "%s %s %p, type:%s\n",
		cpu_dai->name, hooking ? "hook" : "unhook",
		sv->virtbase, sv->type == SVI_DEV_I2S ? "i2s" : "spi");

	spin_lock(&snd->lock);

	/* hooking to start synchronization */
	if (hooking) {
		sv->save_ops = cpu_dai->driver->ops;
		memcpy(&sv->hook_ops,
			cpu_dai->driver->ops,
			sizeof(struct snd_soc_dai_ops));
		sv->hook_ops.trigger = svoice_trigger_start;
		cpu_dai->driver->ops = &sv->hook_ops;
	} else {
		cpu_dai->driver->ops = sv->save_ops;
		svoice_stop(sv);
	}

	spin_unlock(&snd->lock);
}

static int svoice_trigger(struct snd_pcm_substream *substream, int cmd,
			     struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *runtime = substream->private_data;

	dev_dbg(dai->dev, "cmd=%d\n", cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_START:
		hook_cpu_ops(runtime, dai, true);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_STOP:
		hook_cpu_ops(runtime, dai, false);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int svoice_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct nx_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct device_node *cpu_node = rtd->dai_link->cpu_of_node;
	struct svoice_snd *snd = snd_soc_codec_get_drvdata(dai->codec);
	struct svoice_dev *sv;
	struct resource r;
	int ret = -ENODEV;

	sv = kzalloc(sizeof(*sv), GFP_KERNEL);
	if (!sv)
		return -ENOMEM;

	if (strstr(cpu_node->name, "i2s")) {
		sv->type = SVI_DEV_I2S;
		if (!snd->detect) {
			snd->detect = true;
			prtd->run_detector = true;
		}
	} else if (strstr(cpu_node->name, "spi")) {
		sv->type = SVI_DEV_SPI;
	} else {
		dev_err(dai->dev, "not support device\n");
		goto err_start;
	}

	ret = of_address_to_resource(cpu_node, 0, &r);
	if (ret) {
		pr_err("failed to get resource base address\n");
		goto err_start;
	}

	sv->phybase = r.start;
	sv->virtbase = ioremap(sv->phybase, resource_size(&r));
	if (!sv->virtbase) {
		dev_err(dai->dev, "failed to ioremap 0x%lx(%x)\n",
			sv->phybase, resource_size(&r));
		ret = -ENOMEM;
		goto err_start;
	}

	sv->cpu_dai = rtd->cpu_dai;

	spin_lock(&snd->lock);
	list_add(&sv->list, &snd->list);
	spin_unlock(&snd->lock);

	dev_dbg(dai->dev, "%s: 0x%lx -> %p, type:%s\n",
		rtd->cpu_dai->name, sv->phybase, sv->virtbase,
		sv->type == SVI_DEV_I2S ? "i2s" : "spi");

	return 0;

err_start:
	kfree(sv);
	return ret;
}

static void svoice_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct svoice_snd *snd = snd_soc_codec_get_drvdata(dai->codec);
	struct svoice_dev *sv = NULL;

	spin_lock(&snd->lock);
	find_list_dev(sv, snd, rtd->cpu_dai);
	if (!sv) {
		spin_unlock(&snd->lock);
		return;
	}

	list_del(&sv->list);
	spin_unlock(&snd->lock);

	if (snd->detect) {
		snd->detect = false;
		snd->rate_detector = NULL;
	}

	dev_dbg(dai->dev, "%s: %p type:%s\n",
		rtd->cpu_dai->name, sv->virtbase,
		sv->type == SVI_DEV_I2S ? "i2s" : "spi");

	if (sv->virtbase)
		iounmap(sv->virtbase);

	kfree(sv);
}

static int svoice_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct nx_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct svoice_snd *snd = snd_soc_codec_get_drvdata(dai->codec);

	if (prtd->rate_detector) {
		prtd->rate_detector->cb = __pin_nolrck;
		snd->rate_detector = prtd->rate_detector;
	}

	return 0;
}

static const struct snd_soc_dai_ops snd_smart_voice_ops = {
	.startup = svoice_startup,
	.prepare = svoice_prepare,
	.trigger = svoice_trigger,
	.shutdown = svoice_shutdown,
};

static struct snd_soc_dai_driver snd_smart_voice_dai = {
	.name = "snd-smart-voice",
	.capture = {
		.stream_name = "Smart Voice Capture",
		.channels_min = 1,
		.channels_max = 4,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S8 |
				SNDRV_PCM_FMTBIT_S16_LE,
	},
	.ops = &snd_smart_voice_ops,
};

static struct snd_soc_codec_driver snd_smart_voice_codec;

static int svoice_setup(struct platform_device *pdev,
			struct svoice_snd *snd)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = pdev->dev.of_node;
	struct device_node *np;
	struct svoice_pin *pin;
	const __be32 *list;
	u32 addr, length;
	int size, i;
	int ret;

	snd->dev = &pdev->dev;

	/* for PDM PLL */
#ifdef CONFIG_PWM
	snd->pwm = devm_of_pwm_get(dev, node, NULL);
	if (!IS_ERR(snd->pwm)) {
		unsigned int duty_ns = snd->pwm->period / 2;
		unsigned int period_ns = 1000000000 / SVI_PDM_PLL;

		ret = pwm_config(snd->pwm, duty_ns, period_ns);
		if (ret) {
			dev_err(dev, "can't configure PWM\n");
			return ret;
		}

		ret = pwm_enable(snd->pwm);
		if (ret) {
			dev_err(dev, "can't enable PWM\n");
			return ret;
		}

		dev_info(dev, "Smart Voice - pwm.%d %d (%d)\n",
			snd->pwm->pwm, snd->pwm->period, snd->pwm->duty_cycle);
	}
#endif

	list = of_get_property(node, "gpio_regs", &size);

	for (i = 0; i < size/8; i++) {
		addr = be32_to_cpu(*list++);
		length = PAGE_ALIGN(be32_to_cpu(*list++));

		snd->io_bases[i] = ioremap(addr, length);
		if (!snd->io_bases[i]) {
			dev_err(dev, "can't map gpio_%c (0x%x,0x%x)\n",
				('A' + i), addr, length);
			return -EINVAL;
		}
		dev_dbg(dev, "[gpio_%c] 0x%x -> %p\n",
			('A' + i), addr, snd->io_bases[i]);
	}

	/* pin property */
	np = of_find_node_by_name(node, "pin");
	if (!np) {
		dev_err(dev, "can't smart-voice 'pin' node\n");
		return -EINVAL;
	}

	for (i = 0, pin = &svoice_pins[0]; i < SVI_PIN_NUM; i++, pin++) {
		pin->nr = of_get_named_gpio(np, pin->property, 0);
		if (!gpio_is_valid(pin->nr)) {
			if (i == SVI_PIN_PDM_NRST || i == SVI_PIN_I2S_LRCK)
				continue;

			dev_err(dev, "can't smart-voice %s property\n",
				pin->property);
			return -EINVAL;
		}

		pin->group = IO_GROUP(pin->nr);
		pin->offset = IO_OFFSET(pin->nr);
		pin->base = snd->io_bases[pin->group];

		if (devm_gpio_request(dev, pin->nr, pin->property)) {
			dev_err(dev, "can't request gpio_%c.%02d\n",
				('A' + pin->group), pin->offset);
			return -EINVAL;
		}

		if (pin->output && gpio_direction_output(pin->nr, 1)) {
			dev_err(dev, "can't set gpio_%c.%02d output\n",
				('A' + pin->group), pin->offset);
			return -EINVAL;
		}

		dev_info(dev, "[gpio_%c.%02d] %s, %s\n",
			('A' + pin->group), pin->offset, pin->property,
			pin->output ? "out" : "status");
	}

	ret = of_property_read_u32(node,
			"cpu-dais", &snd->num_require_dev);
	if (ret < 0)
		snd->num_require_dev = DEF_SYNC_DEV_NUM;

	INIT_LIST_HEAD(&snd->list);
	spin_lock_init(&snd->lock);

	/* pdm prepare */
	__pin_prepare();

	dev_set_drvdata(&pdev->dev, snd);

	dev_info(dev, "Smart Voice - %dEA\n", snd->num_require_dev);

	return 0;
}

static int svoice_probe(struct platform_device *pdev)
{
	struct svoice_snd *snd;
	int ret;

	snd = kzalloc(sizeof(*snd), GFP_KERNEL);
	if (!snd)
		return -ENOMEM;

	ret = svoice_setup(pdev, snd);
	if (ret < 0)
		goto err_probe;

	ret = snd_soc_register_codec(&pdev->dev,
			&snd_smart_voice_codec, &snd_smart_voice_dai, 1);
	if (ret < 0)
		goto err_probe;


	return ret;

err_probe:
	kfree(snd);
	return ret;
}

static int svoice_remove(struct platform_device *pdev)
{
	struct svoice_snd *snd = dev_get_drvdata(&pdev->dev);
	struct device *dev = &pdev->dev;
	struct svoice_pin *pin;
	int i;

	snd_soc_unregister_codec(dev);

	for (i = 0; i < SVI_PIN_NUM; i++) {
		pin = &svoice_pins[i];
		if (gpio_is_valid(pin->nr))
			devm_gpio_free(dev, pin->nr);
	}

	for (i = 0; i < ARRAY_SIZE(snd->io_bases); i++) {
		if (snd->io_bases[i])
			iounmap(snd->io_bases[i]);
		snd->io_bases[i] = NULL;
	}

#ifdef CONFIG_PWM
	if (!IS_ERR(snd->pwm)) {
		pwm_disable(snd->pwm);
		devm_pwm_put(dev, snd->pwm);
	}
#endif

	kfree(snd);
	return 0;
}

/*
 * SPI smart-voice codec
 */
#ifdef CONFIG_OF
static const struct of_device_id snd_smart_voice_match[] = {
	{ .compatible = "nexell,snd-smart-voice,i2s,pl022" },
	{},
};
MODULE_DEVICE_TABLE(of, snd_smart_voice_match);
#else
#define snd_smart_voice_match NULL
#endif

static struct platform_driver svoice_codec = {
	.probe		= svoice_probe,
	.remove		= svoice_remove,
	.driver		= {
		.name	= "snd-smart-voice",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(snd_smart_voice_match),
	},
};
module_platform_driver(svoice_codec);

MODULE_AUTHOR("jhkim <jhkim@nexell.co.kr>");
MODULE_DESCRIPTION("Sound SPI smart-voice codec driver");
MODULE_LICENSE("GPL");

