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

#include "nexell-i2s.h"

#define	DEF_SAMPLE_RATE			48000
#define	DEF_FRAME_BIT			32	/* 32, 48(BFS) */

#define	I2S_BASEADDR			0xC0055000
#define	I2S_CH_OFFSET			0x1000
#define	I2S_BUS_WIDTH			4	/* Byte */
#define	I2S_PERI_BURST			4	/* Byte */
#define	I2S_MAX_CLOCK			166000000

#define	I2S0_PRESETn			(23)
#define	I2S1_PRESETn			(24)
#define	I2S2_PRESETn			(25)

/*
 * I2S register
 */
struct i2s_register {
	unsigned int CON;		/* 0x00 */
	unsigned int CSR;		/* 0x04 */
	unsigned int FIC;		/* 0x08 */
};

#define	I2S_CON_OFFSET			(0x00)
#define	I2S_CSR_OFFSET			(0x04)
#define	I2S_FIC_OFFSET			(0x08)
#define	I2S_TXD_OFFSET			(0x10)
#define	I2S_RXD_OFFSET			(0x14)

#define	CON_TXDMAPAUSE_POS		6
#define	CON_RXDMAPAUSE_POS		5
#define	CON_TXCHPAUSE_POS		4
#define	CON_RXCHPAUSE_POS		3
#define	CON_TXDMACTIVE_POS		2
#define	CON_RXDMACTIVE_POS		1
#define	CON_I2SACTIVE_POS		0

#define	CSR_BLC_POS			13
#define	CSR_CDCLKCON_POS		12
#define	CSR_IMS_POS			11
#define	CSR_TXR_POS			8
#define	CSR_LRP_POS			7
#define	CSR_SDF_POS			5
#define	CSR_RFS_POS			3
#define	CSR_BFS_POS			1

#define	IMS_BIT_EXTCLK			(0<<0)
#define	IMS_BIT_SLAVE			(1<<0)

#define BLC_8BIT			1
#define BLC_16BIT			0
#define BLC_24BIT			2

#define TX_MODE				0
#define RX_MODE				1
#define TXRX_MODE			2
#define TRANS_MODE_MASK			3

#define BFS_16BIT			2
#define BFS_24BIT			3
#define BFS_32BIT			0
#define BFS_48BIT			1

#define RATIO_256			0
#define RATIO_384			2

#define FIC_TFULSH_POS			15
#define FIC_RFULSH_POS			7

#define FIC_FLUSH_EN			1


struct clock_ratio {
	unsigned int sample_rate;
	unsigned int ratio_256;
	unsigned int ratio_384;
};

static struct clock_ratio clk_ratio[] = {
	{   8000,  2048000,  3072000 },
	{  11025,  2822400,  4233600 },
	{  16000,  4096000,  6144000 },
	{  22050,  5644800,  8467200 },
	{  32000,  8192000, 12288000 },
	{  44100, 11289600, 16934400 },
	{  48000, 12288000, 18432000 },
	{  64000, 16384000, 24576000 },
	{  88200, 22579200, 33868800 },
	{  96000, 24576000, 36864000 },
	{ 192000, 49152000, 73728000 },
};

/*
 * parameters
 */
struct nx_i2s_snd_param {
	int channel;
	int master_mode;	/* 1 = master_mode, 0 = slave */
	int mclk_in;
	int trans_mode;		/* 0 = I2S, 1 = Left-Justified,
				   2 = Right-Justified  */
	int sample_rate;
	int frame_bit;		/* 16, 24, 32, 48 */
	int LR_pol_inv;
	int in_clkgen;
	int pre_supply_mclk;
	bool ext_is_en;
	unsigned long (*set_ext_mclk)(unsigned long clk, int ch);
	int status;
	spinlock_t lock;
	unsigned long flags;
	/* clock control */
	struct clk *clk;
	long clk_rate;
	/* DMA channel */
	struct nx_pcm_dma_param play;
	struct nx_pcm_dma_param capt;
	/* Register */
	void __iomem *base_addr;
	struct i2s_register i2s;
};

#define	SND_I2S_LOCK_INIT(x)		spin_lock_init(x)
#define	SND_I2S_LOCK(x, f)		spin_lock_irqsave(x, f)
#define	SND_I2S_UNLOCK(x, f)		spin_unlock_irqrestore(x, f)

static int set_sample_rate_clock(struct device *dev, struct clk *clk,
				 unsigned long request,	unsigned long *rate_hz)
{
	unsigned long find, rate = 0, clock = request;
	int dio = 0, din = 0, div = 1;
	int ret = 0;

	/* form clock generator */
	find = clk_round_rate(clk, clock);
	din = abs(find - clock);
	dio = din, rate = find;
	div = 0, ret = 0;
	clk_set_rate(clk, find);

	dev_dbg(dev, "%s: req=%ld, acq=%ld, div=%2d\n",
		__func__, request, rate, div);

	if (rate_hz)
		*rate_hz = rate;
	return ret;
}

static void supply_master_clock(struct device *dev,
				struct nx_i2s_snd_param *par)
{
	struct i2s_register *i2s = &par->i2s;
	void __iomem *base = par->base_addr;

	dev_dbg(dev, "%s: %s (status=0x%x)\n",
		__func__, par->master_mode?"master":"slave", par->status);

	if (!par->master_mode ||
		(SNDDEV_STATUS_POWER & par->status))
		return;

	if (par->in_clkgen)
		clk_prepare_enable(par->clk);

	i2s->CSR &= ~(1 << CSR_CDCLKCON_POS);
	writel(i2s->CSR, (base+I2S_CSR_OFFSET));

	par->status |= SNDDEV_STATUS_POWER;
}

static void cutoff_master_clock(struct device *dev,
				struct nx_i2s_snd_param *par)
{
	struct i2s_register *i2s = &par->i2s;
	void __iomem *base = par->base_addr;

	dev_dbg(dev, "%s: %s (status=0x%x)\n",
		__func__, par->master_mode?"master":"slave", par->status);

	if (!par->master_mode ||
		!(SNDDEV_STATUS_POWER & par->status))
		return;

	if (par->in_clkgen)
		clk_disable_unprepare(par->clk);

	/* when high is MCLK OUT enable */
	i2s->CSR |= (1 << CSR_CDCLKCON_POS);
	writel(i2s->CSR, (base+I2S_CSR_OFFSET));

	par->status &= ~SNDDEV_STATUS_POWER;
}

static void i2s_reset(struct device *dev)
{
#ifdef CONFIG_RESET_CONTROLLER
	struct reset_control *rst;

	rst = devm_reset_control_get(dev, "i2s-reset");

	if (!IS_ERR(rst)) {
		if (reset_control_status(rst))
			reset_control_reset(rst);
	}
#endif
}

static int i2s_start(struct snd_soc_dai *dai, int stream)
{
	struct nx_i2s_snd_param *par = snd_soc_dai_get_drvdata(dai);
	struct i2s_register *i2s = &par->i2s;
	void __iomem *base = par->base_addr;
	unsigned int FIC = 0;

	dev_dbg(dai->dev, "%s %d\n", __func__, par->channel);

	SND_I2S_LOCK(&par->lock, par->flags);

	if (!par->pre_supply_mclk) {
		if (par->ext_is_en)
			par->set_ext_mclk(true, par->channel);
		supply_master_clock(dai->dev, par);
	}

	if (SNDRV_PCM_STREAM_PLAYBACK == stream) {
		/* flush fifo */
		FIC |= (FIC_FLUSH_EN << FIC_TFULSH_POS);
		i2s->CON |=  ((1 << CON_TXDMACTIVE_POS) |
			      (1 << CON_I2SACTIVE_POS));
		i2s->CSR &= ~(TRANS_MODE_MASK << CSR_TXR_POS);
		/* Transmit only */
		i2s->CSR |=  (TX_MODE << CSR_TXR_POS);
		par->status |= SNDDEV_STATUS_PLAY;
	} else {
		/* flush fifo */
		FIC |= (FIC_FLUSH_EN << FIC_RFULSH_POS);
		i2s->CON |=  ((1 << CON_RXDMACTIVE_POS) |
			      (1 << CON_I2SACTIVE_POS));
		i2s->CSR &= ~(TRANS_MODE_MASK << CSR_TXR_POS);
		/* Receive only */
		i2s->CSR |=  (RX_MODE << CSR_TXR_POS);
		par->status |= SNDDEV_STATUS_CAPT;
	}

	if ((par->status & SNDDEV_STATUS_PLAY) &&
		(par->status & SNDDEV_STATUS_CAPT)) {
		i2s->CSR &= ~(TRANS_MODE_MASK << CSR_TXR_POS);
		/* Transmit and Receive simultaneous mode */
		i2s->CSR |=  (TXRX_MODE << CSR_TXR_POS);
	}

	writel(FIC, (base+I2S_FIC_OFFSET));	/* Flush the current FIFO */
	writel(0x0, (base+I2S_FIC_OFFSET));	/* Clear the Flush bit */

	writel(i2s->CSR, (base+I2S_CSR_OFFSET));
	writel(i2s->CON, (base+I2S_CON_OFFSET));

	SND_I2S_UNLOCK(&par->lock, par->flags);

	return 0;
}

static void i2s_stop(struct snd_soc_dai *dai, int stream)
{
	struct nx_i2s_snd_param *par = snd_soc_dai_get_drvdata(dai);
	struct i2s_register *i2s = &par->i2s;
	void __iomem *base = par->base_addr;

	dev_dbg(dai->dev, "%s %d\n", __func__, par->channel);

	SND_I2S_LOCK(&par->lock, par->flags);

	if (SNDRV_PCM_STREAM_PLAYBACK == stream) {
		par->status &= ~SNDDEV_STATUS_PLAY;
		i2s->CON &= ~(1 << CON_TXDMACTIVE_POS);
		i2s->CSR &= ~(TRANS_MODE_MASK << CSR_TXR_POS);
		/* Receive only */
		i2s->CSR |=  (RX_MODE << CSR_TXR_POS);
	} else {
		par->status &= ~SNDDEV_STATUS_CAPT;
		i2s->CON &= ~(1 << CON_RXDMACTIVE_POS);
		i2s->CSR &= ~(TRANS_MODE_MASK << CSR_TXR_POS);
		/* Transmit only */
		i2s->CSR |=  (TX_MODE << CSR_TXR_POS);
	}

	if (!(par->status & SNDDEV_STATUS_RUNNING)) {
		if (!par->pre_supply_mclk)
			i2s->CON &= ~(1 << CON_I2SACTIVE_POS);
		/* no tx/rx */
		i2s->CSR |=  (TRANS_MODE_MASK << CSR_TXR_POS);
	}

	writel(i2s->CON, (base+I2S_CON_OFFSET));
	writel(i2s->CSR, (base+I2S_CSR_OFFSET));

	/* I2S Inactive */
	if (!(par->status & SNDDEV_STATUS_RUNNING) &&
		!par->pre_supply_mclk) {
		cutoff_master_clock(dai->dev, par);
		if (par->ext_is_en)
			par->set_ext_mclk(false, par->channel);
	}

	SND_I2S_UNLOCK(&par->lock, par->flags);
}

static struct snd_soc_dai_driver i2s_dai_driver;

static int nx_i2s_check_param(struct nx_i2s_snd_param *par, struct device *dev)
{
	static struct snd_soc_dai_driver *dai = &i2s_dai_driver;
	struct i2s_register *i2s = &par->i2s;
	struct nx_pcm_dma_param *dmap_play = &par->play;
	struct nx_pcm_dma_param *dmap_capt = &par->capt;
	void __iomem *base = par->base_addr;
	unsigned long request = 0, rate_hz = 0;
	int i = 0;

	int LRP, IMS, BLC = BLC_16BIT, BFS = 0, RFS = RATIO_256;
	int SDF = 0, OEN = 0;

	IMS = par->master_mode ? 0 : IMS_BIT_SLAVE;
	/* 0:I2S, 1:Left 2:Right justfied */
	SDF = par->trans_mode & 0x03;
	LRP = par->LR_pol_inv ? 1 : 0;
	/* Active low : MLCK out enable */
	OEN = !par->master_mode ? 1 : par->mclk_in;

	switch (par->frame_bit) {
	case 32:
		BFS = BFS_32BIT; break;
	case 48:
		BFS = BFS_48BIT; break;
	default:
		dev_err(dev, "Fail, not support i2s frame bits");
		dev_err(dev, "%d (32, 48)\n", par->frame_bit);
		return -EINVAL;
	}

	if (!par->master_mode) {
		/* 384 RATIO */
		RFS = RATIO_384, request = clk_ratio[i].ratio_384;
		/* 256 RATIO */
		if (BFS_32BIT == BFS)
			RFS = RATIO_256, request = clk_ratio[i].ratio_256;
		goto done;
	}

	for (i = 0; ARRAY_SIZE(clk_ratio) > i; i++) {
		if (par->sample_rate == clk_ratio[i].sample_rate)
			break;
	}

	if (i >= ARRAY_SIZE(clk_ratio)) {
		dev_err(dev, "Fail, not support i2s sample rate %d\n",
			par->sample_rate);
		return -EINVAL;
	}

	if (par->ext_is_en) {
		if (BFS_48BIT == BFS)
			request = clk_ratio[i].ratio_384;
		else
			request = clk_ratio[i].ratio_256;
	    par->set_ext_mclk(request, par->channel);
	}

	/* 384 RATIO */
	RFS = RATIO_384, request = clk_ratio[i].ratio_384;
	set_sample_rate_clock(dev, par->clk, request, &rate_hz);


	/* 256 RATIO */
	if (rate_hz != request && BFS_32BIT == BFS) {
		unsigned int rate = rate_hz;

		RFS = RATIO_256, request = clk_ratio[i].ratio_256;
		set_sample_rate_clock(dev, par->clk, request, &rate_hz);
		if (abs(request - rate_hz) >
			abs(request - rate)) {
			rate_hz = rate, RFS = RATIO_384;
		}
	}

	/* input clock */
	clk_set_rate(par->clk, rate_hz);
	par->clk_rate = rate_hz;
	par->in_clkgen = 1;

done:
	i2s->CSR =	(BLC << CSR_BLC_POS) |
				(OEN << CSR_CDCLKCON_POS) |
				(IMS << CSR_IMS_POS) |
				(LRP << CSR_LRP_POS) |
				(SDF << CSR_SDF_POS) |
				(RFS << CSR_RFS_POS) |
				(BFS << CSR_BFS_POS);
	i2s_reset(dev);

	if (par->pre_supply_mclk) {
		if (par->ext_is_en)
			rate_hz = par->set_ext_mclk(true, par->channel);
		supply_master_clock(dev, par);
		i2s->CON |=  1 << CON_I2SACTIVE_POS;
		writel(i2s->CON, (base+I2S_CON_OFFSET));
	} else {
		if (par->ext_is_en) {
			rate_hz = par->set_ext_mclk(true, par->channel);
			par->set_ext_mclk(false, par->channel);
		}
	}

	dmap_play->real_clock = rate_hz/(RATIO_256 == RFS ? 256:384);
	dmap_capt->real_clock = rate_hz/(RATIO_256 == RFS ? 256:384);
	dev_info(dev, "snd i2s: ch %d, %s, %s mode, %d(%ld)hz, %d FBITs,",
	       par->channel, par->master_mode ? "master":"slave",
	       par->trans_mode == 0 ? "iis" :
	       (par->trans_mode == 1 ? "left justified" : "right justified"),
	       par->sample_rate, rate_hz/(RATIO_256 == RFS ? 256:384),
	       par->frame_bit);
	dev_info(dev, "MCLK=%ldhz, RFS=%d\n", rate_hz,
		 (RATIO_256 == RFS ? 256:384));
	dev_dbg(dev, "snd i2s: BLC=%d, IMS=%d, LRP=%d", BLC, IMS, LRP);
	dev_dbg(dev, "SDF=%d, RFS=%d, BFS=%d\n", SDF, RFS, BFS);

	/* i2s support format */
	if (RFS == RATIO_256 || BFS != BFS_48BIT) {
		dai->playback.formats &= ~(SNDRV_PCM_FMTBIT_S24_LE |
					   SNDRV_PCM_FMTBIT_U24_LE);
		dai->capture.formats  &= ~(SNDRV_PCM_FMTBIT_S24_LE |
					   SNDRV_PCM_FMTBIT_U24_LE);
	}

	return 0;
}

static int nx_i2s_set_plat_param(struct nx_i2s_snd_param *par, void *data)
{
	struct platform_device *pdev = data;
	struct nx_pcm_dma_param *dma = &par->play;
	unsigned int phy_base = 0;
	int i = 0, ret = 0;
	unsigned int id = 0;

	id = of_alias_get_id(pdev->dev.of_node, "i2s");

	par->channel = id;

	phy_base = I2S_BASEADDR + (par->channel * I2S_CH_OFFSET);

	of_property_read_u32(pdev->dev.of_node, "master-mode",
			     &par->master_mode);
	of_property_read_u32(pdev->dev.of_node, "mclk-in",
			     &par->mclk_in);
	of_property_read_u32(pdev->dev.of_node, "trans-mode",
			     &par->trans_mode);
	of_property_read_u32(pdev->dev.of_node, "frame-bit",
			     &par->frame_bit);
	if (!par->frame_bit)
		par->frame_bit = DEF_FRAME_BIT;
	of_property_read_u32(pdev->dev.of_node, "sample-rate",
			     &par->sample_rate);
	if (!par->sample_rate)
		par->sample_rate = DEF_SAMPLE_RATE;
	of_property_read_u32(pdev->dev.of_node, "pre-supply-mclk",
			     &par->pre_supply_mclk);
	of_property_read_u32(pdev->dev.of_node, "LR-pol-inv",
			     &par->LR_pol_inv);
	par->base_addr = of_iomap(pdev->dev.of_node, 0);
	SND_I2S_LOCK_INIT(&par->lock);

	for (i = 0; 2 > i; i++, dma = &par->capt) {
		dma->active = true;
		dma->dev = &pdev->dev;

		/* I2S TXD/RXD */
		dma->peri_addr = phy_base + (i == 0 ? I2S_TXD_OFFSET :
					     I2S_RXD_OFFSET);
		dma->bus_width_byte = I2S_BUS_WIDTH;
		dma->max_burst_byte = I2S_PERI_BURST;
		dev_dbg(&pdev->dev, "snd i2s: %s dma (peri 0x%p, bus %dbits)\n",
			STREAM_STR(i), (void *)dma->peri_addr,
			dma->bus_width_byte*8);
	}

	par->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(par->clk)) {
		ret = PTR_ERR(par->clk);
		return ret;
	}

	return nx_i2s_check_param(par, &pdev->dev);
}

static int nx_i2s_setup(struct snd_soc_dai *dai)
{
	struct nx_i2s_snd_param *par = snd_soc_dai_get_drvdata(dai);
	struct i2s_register *i2s = &par->i2s;
	void __iomem *base = par->base_addr;

	SND_I2S_LOCK(&par->lock, par->flags);

	if (SNDDEV_STATUS_SETUP & par->status) {
		SND_I2S_UNLOCK(&par->lock, par->flags);
		return 0;
	}

	writel(i2s->CSR, (base+I2S_CSR_OFFSET));

	par->status |= SNDDEV_STATUS_SETUP;

	SND_I2S_UNLOCK(&par->lock, par->flags);

	return 0;
}

static void nx_i2s_release(struct snd_soc_dai *dai)
{
	struct nx_i2s_snd_param *par = snd_soc_dai_get_drvdata(dai);
	struct i2s_register *i2s = &par->i2s;
	void __iomem *base = par->base_addr;

	SND_I2S_LOCK(&par->lock, par->flags);

	i2s->CON = ~((1 << CON_TXDMAPAUSE_POS) | (1 << CON_RXDMAPAUSE_POS) |
		    (1 << CON_TXCHPAUSE_POS) | (1 << CON_RXCHPAUSE_POS) |
		    (1 << CON_TXDMACTIVE_POS) | (1 << CON_RXDMACTIVE_POS) |
		    (1 << CON_I2SACTIVE_POS));

	writel(i2s->CON, (base+I2S_CON_OFFSET));

	cutoff_master_clock(dai->dev, par);
	clk_put(par->clk);

	par->status = SNDDEV_STATUS_CLEAR;

	SND_I2S_UNLOCK(&par->lock, par->flags);
}

/*
 * snd_soc_dai_ops
 */
static int nx_i2s_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct nx_i2s_snd_param *par = snd_soc_dai_get_drvdata(dai);
	struct nx_pcm_dma_param *dmap
		= SNDRV_PCM_STREAM_PLAYBACK == substream->stream ?
		&par->play : &par->capt;

	snd_soc_dai_set_dma_data(dai, substream, dmap);
	return 0;
}

static void nx_i2s_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	i2s_stop(dai, substream->stream);
}

static int nx_i2s_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	int stream = substream->stream;

	dev_dbg(dai->dev, "%s: %s cmd=%d\n", __func__, STREAM_STR(stream), cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_START:
		i2s_start(dai, stream);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_STOP:
		i2s_stop(dai, stream);
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static int nx_i2s_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct nx_i2s_snd_param *par = snd_soc_dai_get_drvdata(dai);
	struct i2s_register *i2s = &par->i2s;
	unsigned int format = params_format(params);
	int RFS = (i2s->CSR >> CSR_RFS_POS) & 0x3;
	int BFS = (i2s->CSR >> CSR_BFS_POS) & 0x3;
	int BLC = (i2s->CSR >> CSR_BLC_POS) & 0x3;
	int ret = 0;

	switch (format) {
	case SNDRV_PCM_FORMAT_S8:
		dev_dbg(dai->dev, "i2s: change sample bits S08\n");
		if (BLC != BLC_8BIT) {
			i2s->CSR &= ~(0x3 << CSR_BLC_POS);
			i2s->CSR |=  (BLC_8BIT << CSR_BLC_POS);
		}
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		dev_dbg(dai->dev, "i2s: change i2s sample bits %s -> S16\n",
			 BLC == BLC_16BIT ? "S16":"S24");
		if (BLC != BLC_16BIT) {
			i2s->CSR &= ~(0x3 << CSR_BLC_POS);
			i2s->CSR |=  (BLC_16BIT << CSR_BLC_POS);
		}
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		dev_dbg(dai->dev, "i2s: change i2s sample bits %s -> S24\n",
			 BLC == BLC_16BIT ? "S16":"S24");
		if (RFS == RATIO_256 || BFS != BFS_48BIT) {
			dev_err(dai->dev, "Fail, i2s RFS 256/BFS 32 not support");
			dev_err(dai->dev, "24 sample bits\n");
			return -EINVAL;
		}
		if (BLC != BLC_24BIT) {
			i2s->CSR &= ~(0x3 << CSR_BLC_POS);
			i2s->CSR |=  (BLC_24BIT << CSR_BLC_POS);
		}
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static struct snd_soc_dai_ops nx_i2s_ops = {
	.startup	= nx_i2s_startup,
	.shutdown	= nx_i2s_shutdown,
	.trigger	= nx_i2s_trigger,
	.hw_params	= nx_i2s_hw_params,
};

/*
 * snd_soc_dai_driver
 */
static int nx_i2s_dai_suspend(struct snd_soc_dai *dai)
{
	struct nx_i2s_snd_param *par = snd_soc_dai_get_drvdata(dai);

	dev_dbg(dai->dev, "%s\n", __func__);

	if (par->in_clkgen)
		clk_disable_unprepare(par->clk);
	return 0;
}

static int nx_i2s_dai_resume(struct snd_soc_dai *dai)
{
	struct nx_i2s_snd_param *par = snd_soc_dai_get_drvdata(dai);
	struct i2s_register *i2s = &par->i2s;
	void __iomem *base = par->base_addr;
	unsigned int FIC = 0;

	dev_dbg(dai->dev, "%s\n", __func__);

	i2s_reset(dai->dev);

	if (par->pre_supply_mclk && par->ext_is_en)
		par->set_ext_mclk(true, par->channel);

	if (par->master_mode && par->in_clkgen) {
		clk_set_rate(par->clk, par->clk_rate);
		if (SNDDEV_STATUS_POWER & par->status)
			clk_prepare_enable(par->clk);
	}

	/* flush fifo */
	FIC |= (par->status & SNDDEV_STATUS_PLAY) ? (1 << 15) : 0;
	FIC |= (par->status & SNDDEV_STATUS_CAPT) ? (1 << 7) : 0;

	writel(FIC, (base+I2S_FIC_OFFSET));	/* Flush the current FIFO */
	writel(0x0, (base+I2S_FIC_OFFSET));	/* Clear the Flush bit */
	writel(i2s->CSR, (base+I2S_CSR_OFFSET));
	writel(i2s->CON, (base+I2S_CON_OFFSET));

	return 0;
}

static int nx_i2s_dai_probe(struct snd_soc_dai *dai)
{
	return nx_i2s_setup(dai);
}

static int nx_i2s_dai_remove(struct snd_soc_dai *dai)
{
	nx_i2s_release(dai);
	return 0;
}

static struct snd_soc_dai_driver i2s_dai_driver = {
	.probe		= nx_i2s_dai_probe,
	.remove		= nx_i2s_dai_remove,
	.suspend	= nx_i2s_dai_suspend,
	.resume		= nx_i2s_dai_resume,
	.playback	= {
		.channels_min	= 2,
		.channels_max	= 2,
		.formats		= SND_SOC_I2S_FORMATS,
		.rates			= SND_SOC_I2S_RATES,
		.rate_min		= 0,
		.rate_max		= 1562500,
		},
	.capture	= {
		.channels_min	= 2,
		.channels_max	= 2,
		.formats		= SND_SOC_I2S_FORMATS,
		.rates			= SND_SOC_I2S_RATES,
		.rate_min		= 0,
		.rate_max		= 1562500,
		},
	.ops = &nx_i2s_ops,
	.symmetric_rates = 1,
};

static const struct snd_soc_component_driver nx_i2s_component = {
	.name = "nx-i2sc",
};

static int nx_i2s_probe(struct platform_device *pdev)
{
	struct nx_i2s_snd_param *par;
	int ret = 0;

	/*  allocate i2c_port data */
	par = kzalloc(sizeof(struct nx_i2s_snd_param), GFP_KERNEL);
	if (!par)
		return -ENOMEM;

	ret = nx_i2s_set_plat_param(par, pdev);
	if (ret)
		goto err_out;

	ret = snd_soc_register_component(&pdev->dev, &nx_i2s_component,
					 &i2s_dai_driver, 1);
	if (ret) {
		dev_err(&pdev->dev, "fail, %s snd_soc_register_component ...\n",
		       pdev->name);
		goto err_out;
	}

	dev_set_drvdata(&pdev->dev, par);
	return ret;

err_out:
	kfree(NULL);
	return ret;
}

static int nx_i2s_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);
	kfree(NULL);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id nx_i2s_match[] = {
	{ .compatible = "nexell,nexell-i2s" },
	{},
};
MODULE_DEVICE_TABLE(of, nx_i2s_match);
#else
#define nx_i2s_match NULL
#endif

static struct platform_driver i2s_driver = {
	.probe  = nx_i2s_probe,
	.remove = nx_i2s_remove,
	.driver = {
		.name	= "nexell-i2s",
		.owner	= THIS_MODULE,
	.of_match_table = of_match_ptr(nx_i2s_match),
	},
};

static int __init nx_i2s_init(void)
{
	return platform_driver_register(&i2s_driver);
}

static void __exit nx_i2s_exit(void)
{
	platform_driver_unregister(&i2s_driver);
}

module_init(nx_i2s_init);
module_exit(nx_i2s_exit);

MODULE_AUTHOR("hsjung <hsjung@nexell.co.kr>");
MODULE_DESCRIPTION("Sound I2S driver for Nexell sound");
MODULE_LICENSE("GPL");
