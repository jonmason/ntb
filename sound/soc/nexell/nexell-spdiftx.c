/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Hyunseok, Jung <hsjung@nexell.co.kr>
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
#include <linux/version.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_address.h>
#ifdef CONFIG_RESET_CONTROLLER
#include <linux/reset.h>
#endif

#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "nexell-spdif.h"

#define	DEF_SAMPLE_RATE			48000
#define	DEF_SAMPLE_BIT			16	/* 8, 16, 24 (PCM) */

#define	SPDIF_BASEADDR			0xC0059000

#if (DEF_SAMPLE_BIT == 16)
#define	SPDIF_BUS_WIDTH			2	/* Byte */
#else
#define	SPDIF_BUS_WIDTH			4	/* Byte */
#endif

#define	SPDIF_MAX_BURST			4	/* Byte */
#define	SPDIF_MAX_CLOCK			166000000

#define	SPDIFTX_PRESETn			(43)

/*
 * SPDIF register
 */
struct spdif_register {
	unsigned int clkcon;
	unsigned int con;
	unsigned int bstas;
	unsigned int cstas;
	unsigned int data;
	unsigned int count;
	unsigned int bstas_shd;
	unsigned int cnt_shd;
	unsigned int userbit1;
	unsigned int userbit2;
	unsigned int userbit3;
	unsigned int userbit1_shd;
	unsigned int userbit2_shd;
	unsigned int userbit3_shd;
	unsigned int version_info;
};

#define	SPDIF_CLKCON_OFFSET		(0x00)
#define	SPDIF_CON_OFFSET		(0x04)
#define	SPDIF_BSTAS_OFFSET		(0x08)
#define	SPDIF_CSTAS_OFFSET		(0x0C)
#define	SPDIF_DAT_OFFSET		(0x10)
#define	SPDIF_CNT_OFFSET		(0x14)

#define	CLKCON_MCLK_SEL_POS		2
#define	CLKCON_POWER_POS		0

#define	CON_FIFO_LV_POS			22
#define	CON_FIFO_TH_POS			19
#define	CON_FIFO_TR_POS			17
#define	CON_ENDIAN_POS			13
#define CON_USERDATA_POS		12
#define	CON_SW_RESET_POS		5
#define	CON_RATIO_POS			3
#define	CON_PCM_BIT_POS			1
#define	CON_PCM_POS			0

#define	CSTAS_FREQUENCY_POS		24
#define	CSTAS_CATEGORY_POS		8
#define	CSTAS_COPYRIGHT_POS		2
#define	CSTAS_PCM_TYPE_POS		1

#define	CON_FIFO_LEVEL_0		0
#define	CON_FIFO_LEVEL_1		1
#define	CON_FIFO_LEVEL_4		2
#define	CON_FIFO_LEVEL_6		3
#define	CON_FIFO_LEVEL_10		4
#define	CON_FIFO_LEVEL_12		5
#define	CON_FIFO_LEVEL_14		6
#define	CON_FIFO_LEVEL_15		7

#define	CON_FIFO_TR_DMA			0
#define	CON_FIFO_TR_POLLING		1
#define	CON_FIFO_TR_INTR		2

#define	CON_ENDIAN_BIG			0
#define	CON_ENDIAN_4B_SWAP		1
#define	CON_ENDIAN_3B_SWAP		2
#define	CON_ENDIAN_2B_SWAP		3

#define PCM_16BIT			0
#define PCM_20BIT			1
#define PCM_24BIT			2

#define RATIO_256			0
#define RATIO_384			1

#define	CATEGORY_CD			1
#define	CATEGORY_DAT			3
#define	CATEGORY_DCC			0x43
#define	CATEGORY_MiNi_DISC		0x49

#define	NO_COPYRIGHT			1
#define	LINEAR_PCM			0
#define	NON_LINEAR_PCM			1

struct clock_ratio {
	unsigned int sample_rate;
	unsigned int ratio_256;
	unsigned int ratio_384;
	int reg_val;
};

static struct clock_ratio clk_ratio[] = {
	{  32000,  8192000, 12288000,  3, },
	{  44100, 11289600, 16934400,  0, },
	{  48000, 12288000, 18432000,  2, },
	{  96000, 24576000, 36864000, 10, },
};

/*
 * parameters
 */
struct nx_spdif_snd_param {
	int sample_rate;
	int pcm_bit;
	int status;
	long master_clock;
	int  master_ratio;
	spinlock_t lock;
	/* clock control */
	struct clk *clk;
	long clk_rate;
	/* DMA channel */
	struct nx_pcm_dma_param dma;
	/* Register */
	void __iomem *base_addr;
	struct spdif_register spdif;
};

#define	SPDIF_MASTER_CLKGEN_BASE	0xC0105000
#define	SPDIF_IN_CLKS			4
#define	MAX_DIVIDER			((1<<8) - 1)	/* 256, align 2 */
#define	DIVIDER_ALIGN			2

static void spdif_reset(struct device *dev,
			       struct nx_spdif_snd_param *par)
{
	void __iomem *base = par->base_addr;

#ifdef CONFIG_RESET_CONTROLLER
	struct reset_control *rst;

	rst = devm_reset_control_get(dev, "spdiftx-reset");

	if (!IS_ERR(rst)) {
		if (reset_control_status(rst))
			reset_control_reset(rst);
	}
#endif
	writel((1 << CON_SW_RESET_POS), (base+SPDIF_CON_OFFSET));
}

static int spdif_start(struct nx_spdif_snd_param *par, int stream)
{
	struct spdif_register *spdif = &par->spdif;
	void __iomem *base = par->base_addr;

	spdif->clkcon |= (1<<CLKCON_POWER_POS);
	writel(spdif->con, (base+SPDIF_CON_OFFSET));
	writel(spdif->clkcon, (base+SPDIF_CLKCON_OFFSET));	/* Power On */

	par->status |= SNDDEV_STATUS_PLAY;
	return 0;
}

static void spdif_stop(struct nx_spdif_snd_param *par, int stream)
{
	struct spdif_register *spdif = &par->spdif;
	void __iomem *base = par->base_addr;

	spdif->clkcon &= ~(1 << CLKCON_POWER_POS);
	writel(spdif->clkcon, (base+SPDIF_CLKCON_OFFSET));

	par->status &= ~SNDDEV_STATUS_PLAY;

}

static int nx_spdif_check_param(struct nx_spdif_snd_param *par, struct device
				*dev)
{
	struct spdif_register *spdif = &par->spdif;
	struct nx_pcm_dma_param *dmap = &par->dma;
	unsigned long request = 0, rate_hz = 0;
	int MCLK = 0; /* only support internal */
	int PCM  = 0;
	int RATIO, SAMPLE_HZ, DATA_23RDBIT = 1, PCM_OR_STREAM = 1;
	int i = 0;

	switch (par->pcm_bit) {
	case 16:
		PCM = PCM_16BIT; break;
	case 24:
		PCM = PCM_24BIT; break;
	default:
		dev_err(dev, "Fail, not support spdiftx pcm bits");
			dev_err(dev, "%d (16, 24)\n", par->pcm_bit);
			return -EINVAL;
	}

	for (i = 0; ARRAY_SIZE(clk_ratio) > i; i++) {
		if (par->sample_rate == clk_ratio[i].sample_rate) {
			SAMPLE_HZ = clk_ratio[i].reg_val;
			break;
		}
	}

	if (i >= ARRAY_SIZE(clk_ratio)) {
		dev_err(dev, "Fail, not support spdif sample rate %d\n",
			par->sample_rate);
		return -EINVAL;
	}

	/* 384 RATIO */
	RATIO = RATIO_384, request = clk_ratio[i].ratio_384;
	rate_hz = clk_round_rate(par->clk, request);

	/* 256 RATIO */
	if (rate_hz != request && PCM == PCM_16BIT) {
		unsigned int o_rate = rate_hz;

		RATIO = RATIO_256, request = clk_ratio[i].ratio_256;
		rate_hz = clk_round_rate(par->clk, request);
		if (abs(request - rate_hz) > abs(request - o_rate))
			rate_hz = o_rate, RATIO = RATIO_384;
	}

	par->master_clock = rate_hz;
	par->master_ratio = RATIO;

	dmap->real_clock = rate_hz/(RATIO_256 == RATIO?256:384);

	spdif->clkcon = (MCLK << CLKCON_MCLK_SEL_POS);
	spdif->con = (CON_FIFO_LEVEL_15 << CON_FIFO_TH_POS) |
		(CON_FIFO_TR_DMA << CON_FIFO_TR_POS) |
		(CON_ENDIAN_BIG << CON_ENDIAN_POS) |
		(DATA_23RDBIT << CON_USERDATA_POS) |
		(RATIO << CON_RATIO_POS) |
		(PCM << CON_PCM_BIT_POS) |
		(PCM_OR_STREAM << CON_PCM_POS);
	spdif->cstas = (SAMPLE_HZ << CSTAS_FREQUENCY_POS) |
		(CATEGORY_CD << CSTAS_CATEGORY_POS) |
		(LINEAR_PCM << CSTAS_PCM_TYPE_POS) |
		(NO_COPYRIGHT << CSTAS_COPYRIGHT_POS);

	return 0;
}

static struct snd_soc_dai_driver spdif_dai_driver;

static int nx_spdif_set_plat_param(struct nx_spdif_snd_param *par, void *data)
{
	struct platform_device *pdev = data;
	struct nx_pcm_dma_param *dma = &par->dma;
	unsigned int phy_base = SPDIF_BASEADDR;
	int ret = 0;
	static struct snd_soc_dai_driver *dai = &spdif_dai_driver;

	of_property_read_u32(pdev->dev.of_node, "pcm-bit",
			     &par->pcm_bit);
	if (!par->pcm_bit)
		par->pcm_bit = DEF_SAMPLE_BIT;
	if (par->pcm_bit == 16)
		dai->playback.formats = SNDRV_PCM_FMTBIT_S16_LE;
	else
		dai->playback.formats = SNDRV_PCM_FMTBIT_S24_LE;

	of_property_read_u32(pdev->dev.of_node, "sample_rate",
			     &par->sample_rate);
	if (!par->sample_rate)
		par->sample_rate = DEF_SAMPLE_RATE;
	dai->playback.rates =
		snd_pcm_rate_to_rate_bit(par->sample_rate);

	par->base_addr = of_iomap(pdev->dev.of_node, 0);
	spin_lock_init(&par->lock);

	dma->active = true;
	dma->dev = &pdev->dev;

	dma->peri_addr = phy_base + SPDIF_DAT_OFFSET;	/* SPDIF DAT */
	dma->bus_width_byte = SPDIF_BUS_WIDTH;
	dma->max_burst_byte = SPDIF_MAX_BURST;
	dev_dbg(&pdev->dev, "spdif-tx: %s dma, addr 0x%p, bus %dbyte, burst %dbyte\n",
		 STREAM_STR(0), (void *)dma->peri_addr, dma->bus_width_byte,
		 dma->max_burst_byte);

	par->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(par->clk)) {
		ret = PTR_ERR(par->clk);
		return ret;
	}

	return nx_spdif_check_param(par, &pdev->dev);
}

static int nx_spdif_setup(struct snd_soc_dai *dai)
{
	struct nx_spdif_snd_param *par = snd_soc_dai_get_drvdata(dai);
	struct spdif_register *spdif = &par->spdif;
	void __iomem *base = par->base_addr;
	long rate_hz = par->master_clock;
	int  ratio = par->master_ratio;
	unsigned int cstas = spdif->cstas;

	if (SNDDEV_STATUS_SETUP & par->status)
		return 0;

	dev_info(dai->dev, "spdif-tx: %d(%ld)Hz, MCLK=%ldhz\n",
		par->sample_rate, rate_hz/(RATIO_256 == ratio?256:384),
		rate_hz);

	/* set clock */
	par->clk_rate = clk_set_rate(par->clk, rate_hz);
	clk_prepare_enable(par->clk);

	spdif_reset(dai->dev, par);

	cstas |= readl(base+SPDIF_CSTAS_OFFSET) & 0x3fffffff;
	writel(spdif->con, (base+SPDIF_CON_OFFSET));
	writel(cstas, (base+SPDIF_CSTAS_OFFSET));

	par->status |= SNDDEV_STATUS_SETUP;
	return 0;
}

static void nx_spdif_release(struct snd_soc_dai *dai)
{
	struct nx_spdif_snd_param *par = snd_soc_dai_get_drvdata(dai);
	struct spdif_register *spdif = &par->spdif;
	void __iomem *base = par->base_addr;

	spdif->clkcon &= ~(1 << CLKCON_POWER_POS);
	writel(spdif->clkcon, (base+SPDIF_CLKCON_OFFSET));

	clk_disable_unprepare(par->clk);
	clk_put(par->clk);

	par->status = SNDDEV_STATUS_CLEAR;
}

/*
 * snd_soc_dai_ops
 */
static int  nx_spdif_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct nx_spdif_snd_param *par = snd_soc_dai_get_drvdata(dai);
	struct nx_pcm_dma_param *dmap = &par->dma;

	snd_soc_dai_set_dma_data(dai, substream, dmap);
	return 0;
}


static void nx_spdif_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct nx_spdif_snd_param *par = snd_soc_dai_get_drvdata(dai);

	spdif_stop(par, substream->stream);
}

static int nx_spdif_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct nx_spdif_snd_param *par = snd_soc_dai_get_drvdata(dai);
	int stream = substream->stream;

	dev_dbg(dai->dev, "%s: %s cmd=%d\n", __func__, STREAM_STR(stream), cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_START:
		spdif_start(par, stream);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_STOP:
		spdif_stop(par, stream);
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static int nx_spdif_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
	struct nx_spdif_snd_param *par = snd_soc_dai_get_drvdata(dai);
	struct spdif_register *spdif = &par->spdif;
	struct nx_pcm_dma_param *dmap = &par->dma;
	unsigned int format = params_format(params);
	int PCM = (spdif->con >> CON_PCM_BIT_POS) & 0x3;
	int ret = 0;

	switch (format) {
	case SNDRV_PCM_FORMAT_S16_LE:
		dev_dbg(dai->dev, "spdiftx: change sample bits %s -> S16\n",
			 PCM == PCM_16BIT?"S16":"S24");
		if (PCM != PCM_16BIT) {
			spdif->con &= ~(0x3 << CON_PCM_BIT_POS);
			spdif->con |=  (PCM_16BIT << CON_PCM_BIT_POS);
			dmap->bus_width_byte = 2; /* change dma bus width */
			dmap->max_burst_byte = SPDIF_MAX_BURST;
		}
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		dev_dbg(dai->dev, "spdiftx: change sample bits %s -> S24\n",
			 PCM == PCM_16BIT?"S16":"S24");
		if (PCM != PCM_24BIT) {
			spdif->con &= ~(0x3 << CON_PCM_BIT_POS);
			spdif->con |=  (PCM_24BIT << CON_PCM_BIT_POS);
			dmap->bus_width_byte = 4; /* change dma bus width */
			dmap->max_burst_byte = SPDIF_MAX_BURST;
		}
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int nx_spdif_set_dai_sysclk(struct snd_soc_dai *dai,
				   int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static struct snd_soc_dai_ops nx_spdif_ops = {
	.startup	= nx_spdif_startup,
	.shutdown	= nx_spdif_shutdown,
	.trigger	= nx_spdif_trigger,
	.hw_params	= nx_spdif_hw_params,
	.set_sysclk	= nx_spdif_set_dai_sysclk,
};

/*
 * snd_soc_dai_driver
 */
static int nx_spdif_dai_suspend(struct snd_soc_dai *dai)
{
	struct nx_spdif_snd_param *par = snd_soc_dai_get_drvdata(dai);
	struct spdif_register *spdif = &par->spdif;
	void __iomem *base = par->base_addr;

	dev_dbg(dai->dev, "%s\n", __func__);

	spdif->clkcon &= ~(1 << CLKCON_POWER_POS);
	writel(spdif->clkcon, (base+SPDIF_CLKCON_OFFSET));

	clk_disable_unprepare(par->clk);

	return 0;
}

static int nx_spdif_dai_resume(struct snd_soc_dai *dai)
{
	struct nx_spdif_snd_param *par = snd_soc_dai_get_drvdata(dai);
	struct spdif_register *spdif = &par->spdif;
	unsigned int cstas = spdif->cstas;
	void __iomem *base = par->base_addr;
	long rate_hz = par->master_clock;

	dev_dbg(dai->dev, "%s\n", __func__);

	par->clk_rate = clk_set_rate(par->clk, rate_hz);
	clk_prepare_enable(par->clk);

	spdif_reset(dai->dev, par);

	cstas |= readl(base+SPDIF_CSTAS_OFFSET) & 0x3fffffff;
	writel(spdif->con, (base+SPDIF_CON_OFFSET));
	writel(cstas, (base+SPDIF_CSTAS_OFFSET));

	return 0;
}

static int nx_spdif_dai_probe(struct snd_soc_dai *dai)
{
	return nx_spdif_setup(dai);
}

static int nx_spdif_dai_remove(struct snd_soc_dai *dai)
{
	nx_spdif_release(dai);
	return 0;
}

static struct snd_soc_dai_driver spdif_dai_driver = {
	.playback	= {
		.channels_min	= 2,
		.channels_max	= 2,
		.formats	= SND_SOC_SPDIF_TX_FORMATS,
		.rates		= SND_SOC_SPDIF_RATES,
		.rate_min	= 0,
		.rate_max	= 1562500,
		},
	.probe		= nx_spdif_dai_probe,
	.remove		= nx_spdif_dai_remove,
	.suspend	= nx_spdif_dai_suspend,
	.resume		= nx_spdif_dai_resume,
	.ops		= &nx_spdif_ops,
};

static const struct snd_soc_component_driver nx_spdif_component = {
	    .name       = "nx-spdif-txc",
};

static int nx_spdif_probe(struct platform_device *pdev)
{
	struct nx_spdif_snd_param *par;
	int ret = 0;

	/*  allocate i2c_port data */
	par = kzalloc(sizeof(struct nx_spdif_snd_param), GFP_KERNEL);
	if (!par)
		return -ENOMEM;

	ret = nx_spdif_set_plat_param(par, pdev);
	if (ret)
		goto err_out;

	ret = devm_snd_soc_register_component(&pdev->dev, &nx_spdif_component,
					 &spdif_dai_driver, 1);
	if (ret) {
		dev_err(&pdev->dev, "fail, %s snd_soc_register_component ...\n",
		       pdev->name);
		goto err_out;
	}

	ret = devm_snd_soc_register_platform(&pdev->dev, &nx_pcm_platform);
	if (ret) {
		dev_err(&pdev->dev, "fail, snd_soc_register_platform...\n");
		goto err_out;
	}

	dev_set_drvdata(&pdev->dev, par);
	return ret;

err_out:
	kfree(NULL);
	return ret;
}

static int nx_spdif_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);
	kfree(NULL);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id nx_spdif_match[] = {
	{ .compatible = "nexell,nexell-spdif-tx" },
	{},
};
MODULE_DEVICE_TABLE(of, nx_spdif_match);
#else
#define nx_spdif_match NULL
#endif

static struct platform_driver spdif_driver = {
	.probe  = nx_spdif_probe,
	.remove = nx_spdif_remove,
	.driver = {
		.name	= "nexell-spdif-tx",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(nx_spdif_match),
	},
};

static int __init nx_spdif_init(void)
{
	return platform_driver_register(&spdif_driver);
}

static void __exit nx_spdif_exit(void)
{
	platform_driver_unregister(&spdif_driver);
}

module_init(nx_spdif_init);
module_exit(nx_spdif_exit);

MODULE_AUTHOR("Hyunseok Jung <hsjung@nexell.co.kr>");
MODULE_DESCRIPTION("Sound S/PDIF tx driver for Nexell sound");
MODULE_LICENSE("GPL");
