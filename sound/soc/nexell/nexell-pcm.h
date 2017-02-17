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

#ifndef __NX_PCM_H__
#define __NX_PCM_H__

#include <linux/amba/pl08x.h>

#include "nexell-i2s.h"

#define SND_SOC_PCM_FORMATS	(SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S8 | \
				SNDRV_PCM_FMTBIT_S16_LE | \
				SNDRV_PCM_FMTBIT_U16_LE | \
				SNDRV_PCM_FMTBIT_S24_LE | \
				SNDRV_PCM_FMTBIT_U24_LE | \
				SNDRV_PCM_FMTBIT_S32_LE | \
				SNDRV_PCM_FMTBIT_U32_LE)

struct nx_pcm_dma_param {
	bool active;
	bool (*dma_filter)(struct dma_chan *chan, void *filter_param);
	char *dma_ch_name;
	dma_addr_t peri_addr;
	int bus_width_byte;
	int max_burst_byte;
	unsigned int real_clock;
	bool dfs;
	struct device *dev;
};

struct nx_pcm_dma_area {
	dma_addr_t physical;		/* dma virtual addr */
	unsigned char *virtual;		/* dma physical addr */
};

#ifdef CONFIG_SND_CODEC_SMARTVOICE
struct nx_pcm_rate_detector {
	struct snd_pcm_substream *substream;
	s64 *us;
	long duration_us;
	int t_count;
	int d_count;
	int i_rate;
	int changed_rate;
	int table[10];
	int table_size;
	struct hrtimer timer;
	struct work_struct work;
	char message[256];
	bool exist;
	bool dynamic_rate;
	spinlock_t lock;
	void (*cb)(void *);
};
#endif

struct nx_pcm_runtime_data {
	struct device *dev;
	/* hw params */
	int period_bytes;
	int periods;
	int buffer_bytes;
	unsigned int dma_area;	/* virtual addr */
	unsigned int offset;
	/* DMA param */
	struct dma_chan  *dma_chan;
	struct nx_pcm_dma_param *dma_param;
	/* dbg dma */
	void *mem_area;
	long mem_len;
	unsigned int mem_offs;
	long long period_time_us;
	/* sample rate detector for smart-voice */
#ifdef CONFIG_SND_CODEC_SMARTVOICE
	bool run_detector;
	struct nx_pcm_rate_detector *rate_detector;
#endif
};

#define	STREAM_STR(dir)	\
	(SNDRV_PCM_STREAM_PLAYBACK == dir ? "playback" : "capture ")

#define	SNDDEV_STATUS_CLEAR	(0)
#define	SNDDEV_STATUS_SETUP	(1<<0)
#define	SNDDEV_STATUS_POWER	(1<<1)
#define	SNDDEV_STATUS_PLAY	(1<<2)
#define	SNDDEV_STATUS_CAPT	(1<<3)
#define	SNDDEV_STATUS_RUNNING	(SNDDEV_STATUS_PLAY | SNDDEV_STATUS_CAPT)

extern struct snd_soc_platform_driver nx_pcm_platform;

#endif
