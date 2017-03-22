/*
 * (C) Copyright 2009
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
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of_address.h>
#include <linux/io.h>
#ifdef CONFIG_RESET_CONTROLLER
#include <linux/reset.h>
#endif
#include <sound/soc.h>

#include "nexell-spi-pl022.h"
#include "nexell-pcm.h"

#define SPI_DMA_RX_OFFS		(0x8)
#define	SPI_SAMPLE_RATE		16000
#define	SPI_DATA_BIT		8
#define	SPI_RESET_MAX		2

/* smart voice spi clock */
#define SPI_CLOCK_HZ		(100 * 1000 * 1000)
#define SPI_CLOCK_PHASE		1
#define SPI_CLOCK_INVERT	1

struct pl022_snd {
	struct device *dev;
	int sample_rate;
	int status;
	spinlock_t	lock;
	/* DT */
	int master_mode;	/* 1 = master_mode, 0 = slave */
	int clk_freq;
	int clk_invert;
	int clk_phase;
	int data_bit;
	enum ssp_rx_level_trig rx_lev_trig;
	enum ssp_protocol protocol;
	int tx_out;
	/* DMA channel */
	struct nx_pcm_dma_param dma;
	/* clock and reset */
	struct clk *clk;
	struct reset_control *rst[SPI_RESET_MAX];
	/* register base */
	resource_size_t phybase;
	void __iomem *virtbase;
};

static int pl022_start(struct pl022_snd *pl022, int stream)
{
	void __iomem *base = pl022->virtbase;

	pl022_enable(base, 1);
	return 0;
}

static void pl022_stop(struct pl022_snd *pl022, int stream)
{
	void __iomem *base = pl022->virtbase;
	u32 val;

	pl022_enable(base, 0);

	/* wait for fifo flush */
	val = __raw_readl(SSP_SR(base));
	if (val & (1 << 2)) {
		while (__raw_readl(SSP_SR(base)) & (1 << 2))
			val = __raw_readl(SSP_DR(base));
	}
}

static int  pl022_ops_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct pl022_snd *pl022 = snd_soc_dai_get_drvdata(dai);
	struct nx_pcm_dma_param *dmap = &pl022->dma;

	snd_soc_dai_set_dma_data(dai, substream, dmap);
	return 0;
}

static void pl022_ops_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
}

static int pl022_ops_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct pl022_snd *pl022 = snd_soc_dai_get_drvdata(dai);
	int stream = substream->stream;

	dev_dbg(pl022->dev, "%s cmd=%d, spi:%p\n",
		STREAM_STR(stream), cmd, pl022->virtbase);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_START:
		pl022_start(pl022, stream);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_STOP:
		pl022_stop(pl022, stream);
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static struct snd_soc_dai_ops pl022_dai_ops = {
	.startup	= pl022_ops_startup,
	.shutdown	= pl022_ops_shutdown,
	.trigger	= pl022_ops_trigger,
};

static int pl022_dai_suspend(struct snd_soc_dai *dai)
{
	return 0;
}

static int pl022_dai_resume(struct snd_soc_dai *dai)
{
	return 0;
}

static struct snd_soc_dai_driver pl022_dai_driver = {
	.suspend = pl022_dai_suspend,
	.resume = pl022_dai_resume,
	.ops = &pl022_dai_ops,
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 4,
		.formats = SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S8 |
				SNDRV_PCM_FMTBIT_S16_LE,
		.rates	= SNDRV_PCM_RATE_8000_192000,
		.rate_min = 16000,
		.rate_max = 16000 * 4,	/* 4channel */
	},
};

static int pl022_dma_config(struct pl022_snd *pl022)
{
	struct nx_pcm_dma_param *dma = &pl022->dma;
	int bus_width, max_burst;

	switch (pl022->data_bit) {
	case 0:
		/* Use the same as for writing */
		bus_width = DMA_SLAVE_BUSWIDTH_UNDEFINED;
		break;
	case 8:
		bus_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		break;
	case 16:
		bus_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		break;
	case 32:
		bus_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		break;
	default:
		dev_err(pl022->dev, "not support pcm read %dbits\n",
			pl022->data_bit);
		return -EINVAL;
	}

	switch (pl022->rx_lev_trig) {
	case SSP_RX_1_OR_MORE_ELEM:
		max_burst = 1;
		break;
	case SSP_RX_4_OR_MORE_ELEM:
		max_burst = 4;
		break;
	case SSP_RX_8_OR_MORE_ELEM:
		max_burst = 8;
		break;
	case SSP_RX_16_OR_MORE_ELEM:
		max_burst = 16;
		break;
	case SSP_RX_32_OR_MORE_ELEM:
		max_burst = 32;
		break;
	default:
		max_burst = 4;
		break;
	}

	dma->active = true;
	dma->dev = pl022->dev;

	dma->peri_addr = pl022->phybase + SPI_DMA_RX_OFFS;
	dma->bus_width_byte = bus_width;
	dma->max_burst_byte = max_burst;
	dma->dma_ch_name = "spi";
	dma->real_clock = (16000 * 4);
	dma->dfs = 0;

	dev_dbg(pl022->dev, "spi-rx: %s dma, 0x%p, bus %dbyte, burst %dbyte\n",
		 STREAM_STR(1), (void *)dma->peri_addr, dma->bus_width_byte,
		 dma->max_burst_byte);

	return 0;
}

static int pl022_setup(struct pl022_snd *pl022)
{
	void __iomem *base = pl022->virtbase;
	struct reset_control *rst;
	int i, ret;

	ret = pl022_dma_config(pl022);
	if (ret)
		return ret;

	/* must be 12 x input clock */
	ret = clk_set_rate(pl022->clk, pl022->clk_freq);
	if (ret) {
		dev_err(pl022->dev,
			"failed to configure SSP/SPI bus clock\n");
		return -EINVAL;
	}

	ret = clk_prepare_enable(pl022->clk);
	if (ret) {
		dev_err(pl022->dev, "failed to enable SSP/SPI bus clock\n");
		return -EINVAL;
	}

	/* reset */
#ifdef CONFIG_RESET_CONTROLLER
	for (i = 0; i < SPI_RESET_MAX; i++) {
		rst = pl022->rst[i];
		if (!rst)
			continue;

		ret = reset_control_status(rst);
		if (ret)
			reset_control_reset(rst);
	}
#endif

	if (pl022->protocol != SSP_PROTOCOL_SPI &&
	pl022->protocol != SSP_PROTOCOL_SSP &&
	pl022->protocol != SSP_PROTOCOL_NM) {
		dev_err(pl022->dev,
			"could not support spi protocol %d\n",
			pl022->protocol);
		return -EINVAL;
	}

	pl022_enable(base, 0);
	pl022_protocol(base, pl022->protocol); /* Motorola SPI */

	pl022_clock_polarity(base, pl022->clk_invert);
	pl022_clock_phase(base, pl022->clk_phase);

	pl022_bit_width(base, pl022->data_bit);  /* 8 bit */
	pl022_mode(base, pl022->master_mode);	/* slave mode */
	pl022_irq_enable(base, 0xf, 0);
	pl022_slave_output(base, pl022->tx_out); /* NO TX */

	pl022_dma_mode(base, 1, 1);

	return 0;
}

static int pl022_dt_parse(struct platform_device *pdev,
			struct pl022_snd *pl022,
			struct snd_soc_dai_driver *dai)
{
	struct device_node *node;
	struct device *dev = &pdev->dev;
	struct resource r;
	int ret;

	pl022->dev = &pdev->dev;
	node = pdev->dev.of_node;

	/* spi clock */
	pl022->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(pl022->clk)) {
		dev_err(dev, "failed to get clock\n");
		return PTR_ERR(pl022->clk);
	}

	/* spi reset */
#ifdef CONFIG_RESET_CONTROLLER
	pl022->rst[0] = devm_reset_control_get(dev, "pre-reset");
	if (IS_ERR(pl022->rst[0])) {
		dev_err(dev, "failed to get pre-reset control\n");
		return -EINVAL;
	}

	pl022->rst[1] = devm_reset_control_get(dev, "spi-reset");
	if (IS_ERR(pl022->rst[1])) {
		dev_err(dev, "failed to get reset control\n");
		return -EINVAL;
	}
#endif

	/* register base address */
	ret = of_address_to_resource(node, 0, &r);
	if (ret) {
		dev_err(dev, "failed to get snd resource\n");
		return -EINVAL;
	}

	pl022->phybase = r.start;
	pl022->virtbase = devm_ioremap(dev, r.start, resource_size(&r));
	if (!pl022->virtbase) {
		dev_err(dev, "failed to ioremap for 0x%x,0x%x\n",
			r.start, resource_size(&r));
		return -ENOMEM;
	}

	/* parse device tree source */
	ret = of_property_read_u32(node, "clock-frequency", &pl022->clk_freq);
	if (ret < 0)
		pl022->clk_freq = SPI_CLOCK_HZ;

	ret = of_property_read_u32(node, "clock-invert", &pl022->clk_invert);
	if (ret < 0)
		pl022->clk_invert = SPI_CLOCK_INVERT;

	ret = of_property_read_u32(node, "clock-phase", &pl022->clk_phase);
	if (ret < 0)
		pl022->clk_phase = SPI_CLOCK_PHASE;

	ret = of_property_read_u32(node, "master-mode", &pl022->master_mode);
	if (ret < 0)
		pl022->master_mode = 0;

	ret = of_property_read_u32(node, "data-bit", &pl022->data_bit);
	if (ret < 0)
		pl022->data_bit = SPI_DATA_BIT;

	ret = of_property_read_u32(node, "sample_rate", &pl022->sample_rate);
	if (ret < 0)
		pl022->sample_rate = SPI_SAMPLE_RATE;

	ret = of_property_read_u32(node, "rx-level-trig", &pl022->rx_lev_trig);
	if (ret < 0)
		pl022->rx_lev_trig = SSP_RX_4_OR_MORE_ELEM;

	ret = of_property_read_u32(node, "protocol", &pl022->protocol);
	if (ret < 0)
		pl022->protocol = SSP_PROTOCOL_SPI;

	/* set snd_soc_dai_driver */
	dai->playback.rates = snd_pcm_rate_to_rate_bit(pl022->sample_rate);

	if (pl022->master_mode) {
		dev_err(dev, "not support master mode\n");
		return -EINVAL;
	}

	dev_info(dev, "spi 0x%x %d bit (%dlv) %s %d rate %dkhz clk(%d/%d)\n",
		pl022->phybase, pl022->data_bit, pl022->rx_lev_trig,
		pl022->protocol == SSP_PROTOCOL_SPI ? "Motorola" :
		pl022->protocol == SSP_PROTOCOL_SSP ? "TI" : "Microwire",
		pl022->sample_rate, pl022->clk_freq/1000,
		pl022->clk_invert, pl022->clk_phase);

	return 0;
}

static const struct snd_soc_component_driver nx_snd_spi_component = {
	    .name       = "nx-spi-rxc",
};

static int pl022_probe(struct platform_device *pdev)
{
	struct pl022_snd *pl022;
	static struct snd_soc_dai_driver *dai = &pl022_dai_driver;
	int ret = 0;

	/*  allocate driver data */
	pl022 = kzalloc(sizeof(struct pl022_snd), GFP_KERNEL);
	if (!pl022)
		return -ENOMEM;

	ret = pl022_dt_parse(pdev, pl022, dai);
	if (ret)
		goto err_out;

	ret = devm_snd_soc_register_component(
			&pdev->dev, &nx_snd_spi_component, dai, 1);
	if (ret) {
		dev_err(&pdev->dev,
			"devm_snd_soc_register_component: %s\n",
			pdev->name);
		goto err_out;
	}

	ret = devm_snd_soc_register_platform(&pdev->dev, &nx_pcm_platform);
	if (ret) {
		dev_err(&pdev->dev,
			"devm_snd_soc_register_platform: %s\n",
			pdev->name);
		goto err_out;
	}

	ret = pl022_setup(pl022);
	if (ret)
		goto err_out;

	dev_set_drvdata(&pdev->dev, pl022);

	return ret;

err_out:
	kfree(pl022);
	return ret;
}

static int pl022_remove(struct platform_device *pdev)
{
	struct pl022_snd *pl022 = dev_get_drvdata(&pdev->dev);

	snd_soc_unregister_component(&pdev->dev);
	kfree(pl022);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id pl022_match[] = {
	{ .compatible = "nexell,nexell-snd-pl022" },
	{},
};
MODULE_DEVICE_TABLE(of, nx_spdif_match);
#else
#define nx_spdif_match NULL
#endif

static struct platform_driver spi_driver = {
	.probe  = pl022_probe,
	.remove = pl022_remove,
	.driver = {
		.name	= "nexell-spi",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(pl022_match),
	},
};
module_platform_driver(spi_driver);

MODULE_AUTHOR("jhkim <jhkim@nexell.co.kr>");
MODULE_DESCRIPTION("Sound SPI driver for Nexell sound");
MODULE_LICENSE("GPL");

