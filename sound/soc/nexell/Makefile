#
# Copyright 2016 Nexell Co.
#
# Makefile for the Nexell Sound

obj-$(CONFIG_SND_NX_SOC)			+= nexell-snd-pcm.o
obj-$(CONFIG_SND_NX_I2S)			+= nexell-snd-i2s.o
obj-$(CONFIG_SND_NX_SPDIF_TX)			+= nexell-snd-spdiftx.o
obj-$(CONFIG_SND_NX_SPI_PL022)			+= nexell-snd-spi-pl022.o
obj-$(CONFIG_SND_NX_FEEDBACK)			+= nexell-snd-feedback.o

nexell-snd-pcm-objs				:= nexell-pcm.o
nexell-snd-i2s-objs				:= nexell-i2s.o
nexell-snd-spdiftx-objs				:= nexell-spdiftx.o
nexell-snd-spi-pl022-objs			:= nexell-spi-pl022.o
nexell-snd-feedback-objs			:= nexell-feedback.o

obj-$(CONFIG_SND_CODEC_ES8316)			+= nexell-snd-es8316.o
obj-$(CONFIG_SND_CODEC_ALC5658)			+= nexell-snd-alc5658.o
obj-$(CONFIG_SND_CODEC_NULL)			+= nexell-snd-null.o
obj-$(CONFIG_SND_CODEC_RT5640)			+= nexell-snd-rt5640.o
obj-$(CONFIG_SND_CODEC_ALC5623)			+= nexell-snd-alc5623.o
obj-$(CONFIG_SND_CODEC_SMARTVOICE)		+= nexell-snd-smart-voice.o
obj-$(CONFIG_SND_SPDIF_TRANSCEIVER)		+= nexell-snd-spdif-transceiver.o
obj-$(CONFIG_SND_NX_SIMPLE_CARD)		+= nexell-snd-soc-simple-card.o

nexell-snd-es8316-objs 				:= nexell-es8316.o
nexell-snd-alc5658-objs 			:= nexell-alc5658.o
nexell-snd-null-objs				:= nexell-null.o
nexell-snd-rt5640-objs				:= nexell-rt5640.o
nexell-snd-alc5623-objs 			:= nexell-alc5623.o
nexell-snd-smart-voice-objs			:= nexell-smart-voice.o nexell-pcm-smart-voice.o
nexell-snd-spdif-transceiver-objs 		:= nexell-spdif-transceiver.o
nexell-snd-soc-simple-card-objs			:= nexell-simple-card.o
