/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Bon-gyu, KOO <freestyle@nexell.co.kr>
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
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/reset.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/driver.h>
#include <linux/iio/machine.h>

/*
 * ADC definitions
 */
#define	ADC_MAX_SAMPLE_RATE		(1*1000*1000)	/* with 6bit */
#define	ADC_MAX_SAMPLE_BITS		6
#define	ADC_MAX_PRESCALE		256		/* 8bit */
#define	ADC_MIN_PRESCALE		20

#define ADC_TIMEOUT		(msecs_to_jiffies(100))


/* PRESCALERCON */
#define	APEN_BITP		(15)	/* 15  */
#define	PRES_BITP		(0)	/* 9:0 */

/* ADCCON */
#define DATA_SEL_VAL		(0)	/* 0:5clk, 1:4clk, 2:3clk, 3:2clk */
					/* 4:1clk: 5:not delayed, else: 4clk */
#define CLK_CNT_VAL		(6)	/* 28nm ADC */

#define DATA_SEL_BITP		(10)	/* 13:10 */
#define CLK_CNT_BITP		(6)	/* 9:6 */
#define	ASEL_BITP		(3)
#define	ADCON_STBY		(2)
#define	ADEN_BITP		(0)

/* ADCINTENB */
#define	AIEN_BITP		(0)

/* ADCINTCLR */
#define	AICL_BITP		(0)


/*
 * ADC register
 */
struct adc_register {
	u32 adccon;
	u32 adcdat;
	u32 adcintenb;
	u32 adcintclr;	/* R: Interrupt Pendded, W: Pending Clear */
	u32 adcprescon;
};

/*
 * ADC data
 */
struct nexell_adc_info {
	void __iomem *adc_base;
	ulong clk_rate;
	ulong sample_rate;
	ulong max_sampele_rate;
	ulong min_sampele_rate;
	int		value;
	int		prescale;
	spinlock_t	lock;
	struct completion completion;
	int irq;
	struct clk *clk;
	struct iio_map *map;
	struct reset_control *rst;
};

static const char * const str_adc_label[] = {
	"ADC0", "ADC1", "ADC2", "ADC3",
	"ADC4", "ADC5", "ADC6", "ADC7",
};

#define ADC_CHANNEL(_index, _id) {			\
	.type = IIO_VOLTAGE,				\
	.indexed = 1,					\
	.channel = _index,				\
	.address = _index,				\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),	\
	.datasheet_name = _id,				\
}

static struct iio_chan_spec nexell_adc_iio_channels[] = {
	ADC_CHANNEL(0, "adc0"),
	ADC_CHANNEL(1, "adc1"),
	ADC_CHANNEL(2, "adc2"),
	ADC_CHANNEL(3, "adc3"),
	ADC_CHANNEL(4, "adc4"),
	ADC_CHANNEL(5, "adc5"),
	ADC_CHANNEL(6, "adc6"),
	ADC_CHANNEL(7, "adc7"),
};


static int nexell_adc_remove_devices(struct device *dev, void *c)
{
	struct platform_device *pdev = to_platform_device(dev);

	platform_device_unregister(pdev);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id nexell_adc_match[] = {
	{ .compatible = "nexell,s5p6818-adc" },
	{},
};
MODULE_DEVICE_TABLE(of, nexell_adc_match);
#endif

/*
 * ADC functions
 */
static irqreturn_t nexell_adc_isr(int irq, void *dev_id)
{
	struct nexell_adc_info *adc = (struct nexell_adc_info *)dev_id;
	struct adc_register *reg = adc->adc_base;

	writel(1, &reg->adcintclr);

	adc->value = readl(&reg->adcdat);
	complete(&adc->completion);

	return IRQ_HANDLED;
}

static void nx_adc_ch_start(struct adc_register *reg, int ch)
{
	unsigned int adcon = 0;

	adcon  = readl(&reg->adccon) & ~(0x07 << ASEL_BITP);
	adcon  = adcon & ~(0x01 << ADEN_BITP);
	adcon |= ch << ASEL_BITP;	/* channel */
	writel(adcon, &reg->adccon);
	adcon  = readl(&reg->adccon);

	adcon |=  1 << ADEN_BITP;	/* start */
	writel(adcon, &reg->adccon);
}

static int __turn_around_invalid_first_read(struct nexell_adc_info *adc)
{
	struct adc_register *reg = adc->adc_base;
	int value = 0;
	unsigned long wait = loops_per_jiffy * (HZ/10);

	nx_adc_ch_start(reg, 0);

	while (wait > 0) {
		if (readl(&reg->adcintclr) & (1 << AICL_BITP)) {
			writel(0x1, &reg->adcintclr); /* pending clear */
			value = readl(&reg->adcdat); /* get value */
			break;
		}
		wait--;
	}
	return 0;
}

static int setup_adc_con(struct nexell_adc_info *adc)
{
	unsigned int adcon = 0;
	unsigned int pres = 0;
	struct adc_register *reg = adc->adc_base;

	adcon = ((DATA_SEL_VAL & 0xf) << DATA_SEL_BITP) |
		((CLK_CNT_VAL & 0xf)  << CLK_CNT_BITP) |
		(0 << ADCON_STBY);
	writel(adcon, &reg->adccon);

	pres = ((adc->prescale & 0x3FF) << PRES_BITP);
	writel(pres, &reg->adcprescon);
	pres |= (1 << APEN_BITP);
	writel(pres, &reg->adcprescon);

	/* *****************************************************
	 * Turn-around invalid value after Power On
	 * *****************************************************/
	__turn_around_invalid_first_read(adc);

	writel(1, &reg->adcintclr);
	writel(1, &reg->adcintenb);
	init_completion(&adc->completion);

	return 0;
}

static int nexell_adc_setup(struct nexell_adc_info *adc,
		struct platform_device *pdev)
{
	ulong min_rate;
	uint32_t sample_rate;
	int prescale = 0;
	int num_ch;

	of_property_read_u32(pdev->dev.of_node, "sample_rate", &sample_rate);

	prescale = (adc->clk_rate) / (sample_rate * ADC_MAX_SAMPLE_BITS);
	min_rate = (adc->clk_rate) / (ADC_MAX_PRESCALE * ADC_MAX_SAMPLE_BITS);

	if (sample_rate > ADC_MAX_SAMPLE_RATE ||
			min_rate > sample_rate) {
		dev_err(&pdev->dev, "not support %u(%d ~ %lu) sample rate\n",
			sample_rate, ADC_MAX_SAMPLE_RATE, min_rate);
		return -EINVAL;
	}

	adc->sample_rate = sample_rate;
	adc->max_sampele_rate = ADC_MAX_SAMPLE_RATE;
	adc->min_sampele_rate = min_rate;
	adc->prescale = prescale;

	setup_adc_con(adc);

	num_ch = ARRAY_SIZE(nexell_adc_iio_channels);
	dev_info(&pdev->dev, "CHs %d, %ld(%ld ~ %ld) sample rate, scale=%d(bit %d)\n",
		num_ch,
		adc->sample_rate,
		adc->max_sampele_rate, adc->min_sampele_rate,
		prescale, ADC_MAX_SAMPLE_BITS);

	return 0;
}

static int nexell_read_raw(struct iio_dev *indio_dev,
		struct iio_chan_spec const *chan,
		int *val,
		int *val2,
		long mask)
{
	struct nexell_adc_info *adc = iio_priv(indio_dev);
	struct adc_register *reg = adc->adc_base;
	int ch = chan->channel;
	unsigned long timeout;
	int ret;

	mutex_lock(&indio_dev->mlock);
	reinit_completion(&adc->completion);

	nx_adc_ch_start(reg, ch);

	timeout = wait_for_completion_timeout(&adc->completion, ADC_TIMEOUT);
	if (timeout == 0) {
		dev_warn(&indio_dev->dev,
				"Conversion timed out! resetting...\n");
		reset_control_reset(adc->rst);
		setup_adc_con(adc);
		ret = -ETIMEDOUT;
	} else {
		*val = adc->value;
		*val2 = 0;
		ret = IIO_VAL_INT;
	}

	mutex_unlock(&indio_dev->mlock);

	dev_dbg(&indio_dev->dev, "%s, ch=%d, val=0x%x\n", __func__, ch, *val);

	return ret;
}

static const struct iio_info nexell_adc_iio_info = {
	.read_raw = &nexell_read_raw,
	.driver_module = THIS_MODULE,
};

static int nexell_adc_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int nexell_adc_resume(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);
	struct nexell_adc_info *adc = iio_priv(indio_dev);

	reset_control_reset(adc->rst);

	setup_adc_con(adc);

	return 0;
}


static int nexell_adc_probe(struct platform_device *pdev)
{
	struct iio_dev *iio = NULL;
	struct nexell_adc_info *adc = NULL;
	struct iio_chan_spec *spec;
	struct resource	*mem;
	struct device_node *np = pdev->dev.of_node;
	int i = 0, irq;
	int ret = -ENODEV;

	if (!np)
		return ret;

	iio = devm_iio_device_alloc(&pdev->dev, sizeof(struct nexell_adc_info));
	if (!iio) {
		dev_err(&pdev->dev, "failed allocating iio ADC device\n");
		return -ENOMEM;
	}

	adc = iio_priv(iio);

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	adc->adc_base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(adc->adc_base))
		return PTR_ERR(adc->adc_base);

	/* setup: clock */
	adc->clk = devm_clk_get(&pdev->dev, "adc");
	if (IS_ERR(adc->clk)) {
		dev_err(&pdev->dev, "failed getting clock for ADC\n");
		return PTR_ERR(adc->clk);
	}
	adc->clk_rate = clk_get_rate(adc->clk);
	clk_prepare_enable(adc->clk);


	/* setup: reset */
	adc->rst = devm_reset_control_get(&pdev->dev, "adc-reset");
	if (IS_ERR(adc->rst)) {
		dev_err(&pdev->dev, "failed to get reset\n");
		return PTR_ERR(adc->rst);
	}

	reset_control_reset(adc->rst);


	/* setup: irq */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "failed get irq resource\n");
		goto err_unprepare_clk;
	}

	ret = devm_request_irq(&pdev->dev, irq, nexell_adc_isr,
			0, dev_name(&pdev->dev), adc);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed get irq (%d)\n", irq);
		goto err_unprepare_clk;
	}

	adc->irq = irq;


	/* setup: adc */
	ret = nexell_adc_setup(adc, pdev);
	if (0 > ret) {
		dev_err(&pdev->dev, "failed setup iio ADC device\n");
		goto err_unprepare_clk;
	}

	platform_set_drvdata(pdev, iio);

	iio->name = dev_name(&pdev->dev);
	iio->dev.parent = &pdev->dev;
	iio->info = &nexell_adc_iio_info;
	iio->modes = INDIO_DIRECT_MODE;
	iio->channels = nexell_adc_iio_channels;
	iio->num_channels = ARRAY_SIZE(nexell_adc_iio_channels);
	iio->dev.of_node = pdev->dev.of_node;

	/*
	 * sys interface : user interface
	 */
	spec = nexell_adc_iio_channels;
	for (i = 0; iio->num_channels > i; i++)
		spec[i].datasheet_name = str_adc_label[i];

	ret = devm_iio_device_register(&pdev->dev, iio);
	if (ret)
		goto err_unprepare_clk;


	ret = of_platform_populate(np, nexell_adc_match, NULL, &pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed adding child nodes\n");
		goto err_of_populate;
	}

	dev_dbg(&pdev->dev, "ADC init success\n");

	return 0;

err_of_populate:
	device_for_each_child(&pdev->dev, NULL,
			nexell_adc_remove_devices);
err_unprepare_clk:
	clk_disable_unprepare(adc->clk);

	return ret;
}

static int nexell_adc_remove(struct platform_device *pdev)
{
	struct iio_dev *iio = platform_get_drvdata(pdev);
	struct nexell_adc_info *adc = iio_priv(iio);

	device_for_each_child(&pdev->dev, NULL,
			nexell_adc_remove_devices);
	clk_disable_unprepare(adc->clk);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver nexell_adc_driver = {
	.probe		= nexell_adc_probe,
	.remove		= nexell_adc_remove,
	.suspend	= nexell_adc_suspend,
	.resume		= nexell_adc_resume,
	.driver		= {
		.name	= "nexell-adc",
		.owner	= THIS_MODULE,
		.of_match_table = nexell_adc_match,
	},
};

module_platform_driver(nexell_adc_driver);

MODULE_AUTHOR("Bon-gyu, KOO <freestyle@nexell.co.kr>");
MODULE_DESCRIPTION("ADC driver for the Nexell s5pxx18");
MODULE_LICENSE("GPL v2");
