/*
 * Copyright (C) 2017  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
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
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "nexell-i2s.h"

/* #define TRACE */
#ifdef TRACE
#define trace(a...)		pr_err(a)
#else
#define trace(a...)
#endif

#define	PERIOD_BYTES_MAX	8192
#define SND_SOC_PCM_FORMATS	(SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S8 | \
				 SNDRV_PCM_FMTBIT_S16_LE | \
				 SNDRV_PCM_FMTBIT_U16_LE | \
				 SNDRV_PCM_FMTBIT_S24_LE | \
				 SNDRV_PCM_FMTBIT_U24_LE | \
				 SNDRV_PCM_FMTBIT_S32_LE | \
				 SNDRV_PCM_FMTBIT_U32_LE)
#define SND_PCM_INFO		(SNDRV_PCM_INFO_MMAP | \
				 SNDRV_PCM_INFO_MMAP_VALID  | \
				 SNDRV_PCM_INFO_INTERLEAVED | \
				 SNDRV_PCM_INFO_PAUSE | \
				 SNDRV_PCM_INFO_RESUME)
#define	substream_to_prtd(s)	(s->runtime->private_data)
#define BUFFER_SIZE		(PAGE_SIZE * 32)

static struct snd_pcm_hardware nx_feedback_pcm_hardware = {
	.info			= SND_PCM_INFO,
	.formats		= SND_SOC_PCM_FORMATS,
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 4,
	.buffer_bytes_max	= BUFFER_SIZE,
	.period_bytes_min	= 32,
	.period_bytes_max	= PERIOD_BYTES_MAX,
	.periods_min		= 2,
	.periods_max		= 64,
	.fifo_size		= 32,
};

struct nx_feedback_context {
	bool play_running;
	bool capture_running;

	void *buffer; /* play buffer */
	void *capture_buffer; /* capture buffer */
	unsigned long buffer_size;
	unsigned long buffer_bytes; /* set by user params: real used total buffer size */
	unsigned long period_bytes; /* play/capture unit size */
	unsigned long offset; /* play offset */
	/* offset of play buffer, copied from this offset
	 * to capture_offset of capture_buffer */
	unsigned long read_offset;
	unsigned long capture_offset; /*capture offset */
	long play_interval_ns; /* period_bytes consume interval */
	ktime_t play_start_time;
	ktime_t play_update_time;
	ktime_t capture_start_time;
	ktime_t capture_update_time;

	struct snd_pcm_substream *capture_substream;
	struct snd_pcm_substream *play_substream;

	struct hrtimer play_hrtimer;
	struct hrtimer capture_hrtimer;
};

struct nx_feedback_pcm_runtime_data {
	struct device *dev;
	int period_bytes; /* unit of transfer(user <-> kernel) */
	int periods;	  /* count of unit */
	int buffer_bytes;

	struct nx_feedback_context *context;
};

struct nx_feedback {
	struct snd_soc_dai_driver dai_drv;
	struct nx_feedback_context context;
};

static int nx_feedback_startup(struct snd_pcm_substream *ss,
			       struct snd_soc_dai *dai)
{
	struct nx_feedback *nx_feedback = snd_soc_dai_get_drvdata(dai);

	trace("[%s:%s]\n", __func__, STREAM_STR(ss->stream));
	snd_soc_dai_set_dma_data(dai, ss, &nx_feedback->context);
	return 0;
}

static void nx_feedback_shutdown(struct snd_pcm_substream *ss,
				 struct snd_soc_dai *dai)
{
	trace("[%s:%s]\n", __func__, STREAM_STR(ss->stream));
}

static int nx_feedback_trigger(struct snd_pcm_substream *ss,
			       int cmd, struct snd_soc_dai *dai)
{
	trace("[%s:%s]\n", __func__, STREAM_STR(ss->stream));
	return 0;
}

static int nx_feedback_hw_params(struct snd_pcm_substream *ss,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
	trace("[%s:%s]\n", __func__, STREAM_STR(ss->stream));
	return 0;
}

static struct snd_soc_dai_ops nx_feedback_dai_ops = {
	.startup	= nx_feedback_startup,
	.shutdown	= nx_feedback_shutdown,
	.trigger	= nx_feedback_trigger,
	.hw_params	= nx_feedback_hw_params,
};

static void nx_feedback_init_dai(struct snd_soc_dai_driver *dai_drv)
{
	dai_drv->ops = &nx_feedback_dai_ops;
	dai_drv->symmetric_rates = 1;

	dai_drv->playback.stream_name = "Playback";
	dai_drv->playback.channels_min = 1;
	dai_drv->playback.channels_max = 4;
	dai_drv->playback.formats = SND_SOC_I2S_FORMATS;
	dai_drv->playback.rates = SND_SOC_I2S_RATES;
	dai_drv->playback.rate_min = 0;
	dai_drv->playback.rate_max = 192000;

	dai_drv->capture.stream_name = "Capture";
	dai_drv->capture.channels_min = 1;
	dai_drv->capture.channels_max = 4;
	dai_drv->capture.formats = SND_SOC_I2S_FORMATS;
	dai_drv->capture.rates = SND_SOC_I2S_RATES;
	dai_drv->capture.rate_min = 0;
	dai_drv->capture.rate_max = 192000;
}

static const struct snd_soc_component_driver nx_feedback_component = {
	.name = "nx-feedbackc",
};

static int nx_feedback_pcm_setup_buffer(struct snd_pcm_substream *ss)
{
	struct nx_feedback_pcm_runtime_data *prtd = substream_to_prtd(ss);
	struct snd_dma_buffer *buf = &ss->dma_buffer;

	buf->dev.type = SNDRV_DMA_TYPE_CONTINUOUS;
	buf->dev.dev = ss->pcm->card->dev;
	buf->private_data = prtd->context;
	buf->bytes = prtd->context->buffer_bytes;
	buf->area = prtd->context->buffer;

	return 0;
}

static enum hrtimer_restart play_callback(struct hrtimer *hrtimer)
{
	struct nx_feedback_context *context =
		container_of(hrtimer, struct nx_feedback_context, play_hrtimer);
	enum hrtimer_restart ret = HRTIMER_NORESTART;

	if (context->play_running) {
		ktime_t interval;
		long elapsed, diff;

		context->offset += context->period_bytes;
		context->offset %= context->buffer_bytes;;
		snd_pcm_period_elapsed(context->play_substream);

		context->play_update_time = ktime_get();
		elapsed = ktime_to_ns(ktime_sub(context->play_update_time,
						context->play_start_time));
		diff = elapsed - context->play_interval_ns;
		trace("%s: elapsed %ld, diff %ld\n", __func__, elapsed, diff);

		if (diff > 0)
			interval = ktime_set(0, context->play_interval_ns - diff);
		else
			interval = ktime_set(0, context->play_interval_ns);

		hrtimer_forward(hrtimer, context->play_update_time, interval);
		context->play_start_time = context->play_update_time;
		ret = HRTIMER_RESTART;
	}

	return ret;
}

static enum hrtimer_restart capture_callback(struct hrtimer *hrtimer)
{
	struct nx_feedback_context *context =
		container_of(hrtimer, struct nx_feedback_context,
			     capture_hrtimer);
	enum hrtimer_restart ret = HRTIMER_NORESTART;

	if (context->capture_running) {
		ktime_t interval;
		long elapsed, diff;

		if (context->read_offset != context->offset) {
			memcpy(context->capture_buffer + context->capture_offset,
			       context->buffer + context->read_offset,
			       context->period_bytes);

			context->read_offset += context->period_bytes;
			context->read_offset %= context->buffer_bytes;
			context->capture_offset += context->period_bytes;
			context->capture_offset %= context->buffer_bytes;
			snd_pcm_period_elapsed(context->capture_substream);
		}

		context->capture_update_time = ktime_get();
		elapsed = ktime_to_ns(ktime_sub(context->capture_update_time,
						context->capture_start_time));
		diff = elapsed - context->play_interval_ns;
		trace("%s: elapsed %ld, diff %ld\n", __func__, elapsed, diff);

		if (diff > 0)
			interval = ktime_set(0, context->play_interval_ns - diff);
		else
			interval = ktime_set(0, context->play_interval_ns);

		hrtimer_forward(hrtimer, context->capture_update_time,
				interval);
		context->capture_start_time = context->capture_update_time;
		ret = HRTIMER_RESTART;
	}

	return ret;
}

static int nx_feedback_pcm_open(struct snd_pcm_substream *ss)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct snd_soc_pcm_runtime *rtd = ss->private_data;
	struct snd_pcm_hardware *hw = &nx_feedback_pcm_hardware;
	struct nx_feedback_pcm_runtime_data *prtd;
	int ret = 0;

	trace("[%s:%s]\n", __func__, STREAM_STR(ss->stream));
	prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
	if (!prtd) {
		pcm_err(ss->pcm,
			"failed to allocate nx_feedback_pcm_runtime_data\n");
		return -ENOMEM;
	}

	prtd->dev = ss->pcm->card->dev;
	prtd->context = snd_soc_dai_get_dma_data(rtd->cpu_dai, ss);

	runtime->private_data = prtd;

	nx_feedback_pcm_setup_buffer(ss);

	ret = snd_pcm_hw_constraint_integer(runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		pcm_err(ss->pcm,
			"failed to snd_pcm_hw_constraint_integer(%d)\n", ret);
		return ret;
	}

	hw->period_bytes_max = PERIOD_BYTES_MAX;
	return snd_soc_set_runtime_hwparams(ss, &nx_feedback_pcm_hardware);
}

static int nx_feedback_pcm_close(struct snd_pcm_substream *ss)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct nx_feedback_pcm_runtime_data *prtd = runtime->private_data;

	trace("[%s:%s]\n", __func__, STREAM_STR(ss->stream));
	if (prtd) {
		if (ss->stream == SNDRV_PCM_STREAM_CAPTURE) {
			hrtimer_cancel(&prtd->context->capture_hrtimer);
			prtd->context->capture_running = false;
			prtd->context->capture_substream = NULL;
		} else {
			hrtimer_cancel(&prtd->context->play_hrtimer);
			prtd->context->play_running = false;
			prtd->context->offset = 0;
			prtd->context->play_substream = NULL;
		}

		kfree(prtd);
		runtime->private_data = NULL;
	}

	return 0;
}

static int nx_feedback_pcm_hw_params(struct snd_pcm_substream *ss,
				     struct snd_pcm_hw_params *params)
{
	struct nx_feedback_pcm_runtime_data *prtd = substream_to_prtd(ss);

	prtd->periods = params_periods(params);
	prtd->period_bytes = params_period_bytes(params);
	prtd->buffer_bytes = params_buffer_bytes(params);

	if (ss->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		prtd->context->buffer_bytes = prtd->buffer_bytes;
		prtd->context->period_bytes = prtd->period_bytes;
		prtd->context->play_interval_ns =
			((1000 * 1000 * 1000) / params_rate(params)) *
			params_period_size(params);
	}

	trace("[%s:%s] periods %d, period_bytes %d, buffer_bytes %d, "
	      "rate=%6d, play_interval_ns=%lu\n",
	      __func__, STREAM_STR(ss->stream), prtd->periods,
	      prtd->period_bytes, prtd->buffer_bytes,
	      params_rate(params), prtd->context->play_interval_ns);

	snd_pcm_set_runtime_buffer(ss, &ss->dma_buffer);
	return 0;
}

static int nx_feedback_pcm_hw_free(struct snd_pcm_substream *ss)
{
	trace("[%s:%s]\n", __func__, STREAM_STR(ss->stream));
	snd_pcm_set_runtime_buffer(ss, NULL);
	return 0;
}

static int nx_feedback_pcm_trigger(struct snd_pcm_substream *ss, int cmd)
{
	struct nx_feedback_pcm_runtime_data *prtd = substream_to_prtd(ss);
	struct nx_feedback_context *context = prtd->context;

	trace("[%s:%s] cmd=%d\n", __func__, STREAM_STR(ss->stream), cmd);

	if (ss->stream == SNDRV_PCM_STREAM_PLAYBACK && !context->play_running) {
		ktime_t time = ktime_set(0, context->play_interval_ns);

		context->play_running = true;
		context->offset = 0;
		context->play_substream = ss;
		context->play_start_time = ktime_get();

		hrtimer_start(&context->play_hrtimer, time, HRTIMER_MODE_REL);
	} else if (ss->stream == SNDRV_PCM_STREAM_CAPTURE &&
		   !context->capture_running) {
		ktime_t time = ktime_set(0, context->play_interval_ns);

		context->capture_running = true;
		context->capture_offset = 0;
		context->read_offset = context->offset;
		context->capture_substream = ss;

		context->capture_start_time = ktime_get();
		hrtimer_start(&context->capture_hrtimer, time,
			      HRTIMER_MODE_REL);
	}
	return 0;
}

static snd_pcm_uframes_t nx_feedback_pcm_pointer(struct snd_pcm_substream *ss)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct nx_feedback_pcm_runtime_data *prtd = substream_to_prtd(ss);
	struct nx_feedback_context *ctx = prtd->context;
	snd_pcm_uframes_t frames = 0;

	if (ss->stream == SNDRV_PCM_STREAM_CAPTURE)
		frames = bytes_to_frames(runtime, ctx->capture_offset);
	else
		frames = bytes_to_frames(runtime, ctx->offset);

	trace("[%s:%s] play offset %lu, capture offset %lu, frames %lu\n",
	      __func__, STREAM_STR(ss->stream), ctx->offset,
	      ctx->capture_offset, frames);

	return frames;
}

static int nx_feedback_pcm_mmap(struct snd_pcm_substream *ss,
				struct vm_area_struct *vma)
{
	struct snd_dma_buffer *buf = &ss->dma_buffer;
	unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);

	trace("[%s:%s] request size %lu, buffer bytes %d\n", __func__,
	      STREAM_STR(ss->stream), size, buf->bytes);
	if (size > buf->bytes) {
		pcm_err(ss->pcm,
			"%s: size(%lu) is over buffer_bytes(%d)\n",
			__func__, size, buf->bytes);
		return -EINVAL;
	}

	return remap_pfn_range(vma, vma->vm_start,
			       (unsigned long)buf->area >> PAGE_SHIFT,
			       size, vma->vm_page_prot);
}

static int nx_feedback_pcm_prepare(struct snd_pcm_substream *ss)
{
	struct nx_feedback_pcm_runtime_data *prtd = substream_to_prtd(ss);
	struct nx_feedback_context *context = prtd->context;

	trace("[%s:%s]\n", __func__, STREAM_STR(ss->stream));

	if (ss->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (!context->play_running) {
			dev_err(prtd->dev, "play must be running\n");
			return -EINVAL;
		}
	}

	return 0;
}

static struct snd_pcm_ops nx_feedback_pcm_ops = {
	.open		= nx_feedback_pcm_open,
	.close		= nx_feedback_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= nx_feedback_pcm_hw_params,
	.hw_free	= nx_feedback_pcm_hw_free,
	.trigger	= nx_feedback_pcm_trigger,
	.pointer	= nx_feedback_pcm_pointer,
	.mmap		= nx_feedback_pcm_mmap,
	.prepare	= nx_feedback_pcm_prepare,
};

static int nx_feedback_pcm_new(struct snd_soc_pcm_runtime *runtime)
{
	return 0;
}

static void nx_feedback_pcm_free(struct snd_pcm *pcm)
{
}

static struct snd_soc_platform_driver nx_feedback_pcm_platform = {
	.ops		= &nx_feedback_pcm_ops,
	.pcm_new	= nx_feedback_pcm_new,
	.pcm_free	= nx_feedback_pcm_free,
};

static int nx_feedback_probe(struct platform_device *pdev)
{
	struct nx_feedback *nx_feedback;
	int ret = 0;
	struct nx_feedback_context *context;

	nx_feedback = devm_kzalloc(&pdev->dev, sizeof(*nx_feedback),
				   GFP_KERNEL);
	if (!nx_feedback) {
		dev_err(&pdev->dev, "failed to alloc nx_feedback\n");
		return -ENOMEM;
	}

	nx_feedback_init_dai(&nx_feedback->dai_drv);

	context = &nx_feedback->context;
	context->buffer = devm_kzalloc(&pdev->dev, BUFFER_SIZE, GFP_KERNEL);
	if (!context->buffer) {
		dev_err(&pdev->dev, "failed to allocate play buffer\n");
		return -ENOMEM;
	}
	context->capture_buffer = devm_kzalloc(&pdev->dev, BUFFER_SIZE,
					       GFP_KERNEL);
	if (!context->capture_buffer) {
		dev_err(&pdev->dev, "failed to allocate capture buffer\n");
		return -ENOMEM;
	}
	context->buffer_size = BUFFER_SIZE;
	context->play_running = false;
	context->capture_running = false;
	hrtimer_init(&context->play_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	context->play_hrtimer.function = play_callback;
	hrtimer_init(&context->capture_hrtimer, CLOCK_MONOTONIC,
		     HRTIMER_MODE_REL);
	context->capture_hrtimer.function = capture_callback;

	ret = devm_snd_soc_register_component(&pdev->dev,
					      &nx_feedback_component,
					      &nx_feedback->dai_drv, 1);
	if (ret) {
		dev_err(&pdev->dev, "fail to snd_soc_register_component %s\n",
			pdev->name);
		return ret;
	}

	ret = devm_snd_soc_register_platform(&pdev->dev,
					     &nx_feedback_pcm_platform);
	if (ret) {
		dev_err(&pdev->dev, "fail to snd_soc_register_platform %s\n",
			pdev->name);
		snd_soc_unregister_component(&pdev->dev);
		return ret;
	}

	dev_set_drvdata(&pdev->dev, nx_feedback);
	return 0;
}

static int nx_feedback_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id nx_feedback_match[] = {
	{ .compatible = "nexell,snd-feedback" },
	{},
};
#else
#define nx_feedback_match	NULL
#endif

static struct platform_driver feedback_driver = {
	.probe	= nx_feedback_probe,
	.remove	= nx_feedback_remove,
	.driver = {
		.name	= "nexell-feedback",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(nx_feedback_match),
	},
};

static int __init nx_feedback_init(void)
{
	return platform_driver_register(&feedback_driver);
}

static void __exit nx_feedback_exit(void)
{
	platform_driver_unregister(&feedback_driver);
}

module_init(nx_feedback_init);
module_exit(nx_feedback_exit);

MODULE_AUTHOR("Sungwoo Park <swpark@nexell.co.kr>");
MODULE_DESCRIPTION("Sound Feedback driver for Nexell SoC");
MODULE_LICENSE("GPL");
