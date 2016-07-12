#
# Copyright 2016 Nexell Co.
#
# Makefile for the Nexell Sound

obj-$(CONFIG_SND_NX_SOC)			+= nexell-snd-pcm.o
obj-$(CONFIG_SND_NX_I2S)			+= nexell-snd-i2s.o
obj-$(CONFIG_SND_NX_SPDIF_TX)			+= nexell-snd-spdiftx.o

nexell-snd-pcm-objs				:= nexell-pcm.o
nexell-snd-i2s-objs				:= nexell-i2s.o
nexell-snd-spdiftx-objs				:= nexell-spdiftx.o

obj-$(CONFIG_SND_CODEC_ES8316)			+= nexell-snd-es8316.o
obj-$(CONFIG_SND_CODEC_ALC5658)			+= nexell-snd-alc5658.o
obj-$(CONFIG_SND_CODEC_NULL)			+= nexell-snd-null.o
obj-$(CONFIG_SND_SPDIF_TRANSCEIVER)		+= nexell-snd-spdif-transceiver.o

nexell-snd-es8316-objs 				:= nexell-es8316.o
nexell-snd-alc5658-objs 			:= nexell-alc5658.o
nexell-snd-null-objs				:= nexell-null.o
nexell-snd-spdif-transceiver-objs 		:= nexell-spdif-transceiver.o
