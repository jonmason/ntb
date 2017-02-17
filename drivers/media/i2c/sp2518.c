/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongkeun, Choi <jkchoi@nexell.co.kr>
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
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <linux/v4l2-mediabus.h>

#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <uapi/linux/media-bus-format.h>
#include <linux/version.h>

#define MODULE_NAME "SP2518"

#define PID 0x02 /* Product ID Number  */
#define SP2518 0x53
#define OUTTO_SENSO_CLOCK 24000000
#define NUM_CTRLS 11
#define V4L2_IDENT_SP2518 64112

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};

/****************************************************************************
 * predefined reg values
 */
#define ENDMARKER	\
{			\
	0xff, 0xff	\
}

static struct regval_list sp2518_fmt_yuv422_yuyv[] = {
	/* YCbYCr */
	{0xfd, 0x00},
	{0x35, 0x40},
	ENDMARKER,
};

static struct regval_list sp2518_fmt_yuv422_yvyu[] = {
	/* YCrYCb */
	{0xfd, 0x00},
	{0x35, 0x41},
	ENDMARKER,
};

static struct regval_list sp2518_fmt_yuv422_vyuy[] = {
	/* CrYCbY */
	{0xfd, 0x00},
	{0x35, 0x01},
	ENDMARKER,
};

static struct regval_list sp2518_fmt_yuv422_uyvy[] = {
	/* CbYCrY */
	{0xfd, 0x00},
	{0x35, 0x00},
	ENDMARKER,
};

/*
 * register setting for window size
 */
static const struct regval_list sp2518_svga_init_regs[] = {
	{0xfd, 0x00},
	{0x1b, 0x1a},
	{0x0e, 0x01},
	{0x0f, 0x2f},
	{0x10, 0x2e},
	{0x11, 0x00},
	{0x12, 0x4f},
	{0x14, 0x40},
	{0x16, 0x02},
	{0x17, 0x10},
	{0x1a, 0x1f},
	{0x1e, 0x81},
	{0x21, 0x00},
	{0x22, 0x1b},
	{0x25, 0x10},
	{0x26, 0x25},
	{0x27, 0x6d},
	{0x2c, 0x23},
	{0x2d, 0x75},
	{0x2e, 0x38},
	{0x31, 0x18},
	{0x44, 0x03},
	{0x6f, 0x00},
	{0xa0, 0x04},
	{0x5f, 0x01},
	{0x32, 0x00},
	{0xfd, 0x01},
	{0x2c, 0x00},
	{0x2d, 0x00},
	{0xfd, 0x00},
	{0xfb, 0x83},
	{0xf4, 0x09},
	{0xfd, 0x01},
	{0xc6, 0x90},
	{0xc7, 0x90},
	{0xc8, 0x90},
	{0xc9, 0x90},
	{0xfd, 0x00},
	{0x65, 0x08},
	{0x66, 0x08},
	{0x67, 0x08},
	{0x68, 0x08},
	{0x46, 0xff},
	{0xfd, 0x00},
	{0xe0, 0x6c},
	{0xe1, 0x54},
	{0xe2, 0x48},
	{0xe3, 0x40},
	{0xe4, 0x40},
	{0xe5, 0x3e},
	{0xe6, 0x3e},
	{0xe8, 0x3a},
	{0xe9, 0x3a},
	{0xea, 0x3a},
	{0xeb, 0x38},
	{0xf5, 0x38},
	{0xf6, 0x38},
	{0xfd, 0x01},
	{0x94, 0xcC},
	{0x95, 0x38},
	{0x9c, 0x74},
	{0x9d, 0x38},
	{0xfd, 0x00},
	{0x03, 0x03},
	{0x04, 0x0C},
	{0x05, 0x00},
	{0x06, 0x00},
	{0x07, 0x00},
	{0x08, 0x00},
	{0x09, 0x00},
	{0x0a, 0xE4},
	{0x2f, 0x00},
	{0x30, 0x11},
	{0xf0, 0x82},
	{0xf1, 0x00},
	{0xfd, 0x01},
	{0x90, 0x0A},
	{0x92, 0x01},
	{0x98, 0x82},
	{0x99, 0x01},
	{0x9a, 0x01},
	{0x9b, 0x00},
	{0xfd, 0x01},
	{0xce, 0x14},
	{0xcf, 0x05},
	{0xd0, 0x14},
	{0xd1, 0x05},
	{0xd7, 0x7E},
	{0xd8, 0x00},
	{0xd9, 0x82},
	{0xda, 0x00},
	{0xfd, 0x00},
	{0xfd, 0x01},
	{0xca, 0x30},
	{0xcb, 0x50},
	{0xcc, 0xc0},
	{0xcd, 0xc0},
	{0xd5, 0x80},
	{0xd6, 0x90},
	{0xfd, 0x00},
	{0xfd, 0x00},
	{0xa1, 0x20},
	{0xa2, 0x20},
	{0xa3, 0x20},
	{0xa4, 0xff},
	{0xa5, 0x80},
	{0xa6, 0x80},
	{0xfd, 0x01},
	{0x64, 0x1e},
	{0x65, 0x1c},
	{0x66, 0x1c},
	{0x67, 0x16},
	{0x68, 0x1c},
	{0x69, 0x1c},
	{0x6a, 0x1a},
	{0x6b, 0x16},
	{0x6c, 0x1a},
	{0x6d, 0x1a},
	{0x6e, 0x1a},
	{0x6f, 0x16},
	{0xb8, 0x04},
	{0xb9, 0x13},
	{0xba, 0x00},
	{0xbb, 0x03},
	{0xbc, 0x03},
	{0xbd, 0x11},
	{0xbe, 0x00},
	{0xbf, 0x02},
	{0xc0, 0x04},
	{0xc1, 0x0e},
	{0xc2, 0x00},
	{0xc3, 0x05},
	{0xfd, 0x01},
	{0xde, 0x0f},
	{0xfd, 0x00},
	{0x57, 0x08},
	{0x58, 0x08},
	{0x56, 0x08},
	{0x59, 0x10},
	{0x5a, 0xa0},
	{0xc4, 0xa0},
	{0x43, 0xa0},
	{0xad, 0x40},
	{0x4f, 0xa0},
	{0xc3, 0xa0},
	{0x3f, 0xa0},
	{0x42, 0x40},
	{0xc2, 0x15},
	{0xb6, 0x80},
	{0xb7, 0x80},
	{0xb8, 0x40},
	{0xb9, 0x20},
	{0xfd, 0x01},
	{0x50, 0x0c},
	{0x51, 0x0c},
	{0x52, 0x10},
	{0x53, 0x10},
	{0xfd, 0x00},
	{0xfd, 0x01},
	{0x11, 0x10},
	{0x12, 0x1f},
	{0x16, 0x1c},
	{0x18, 0x00},
	{0x19, 0x00},
	{0x1b, 0x96},
	{0x1a, 0x9a},
	{0x1e, 0x2f},
	{0x1f, 0x29},
	{0x20, 0xff},
	{0x22, 0xff},
	{0x28, 0xce},
	{0x29, 0x8a},
	{0xfd, 0x00},
	{0xe7, 0x03},
	{0xe7, 0x00},
	{0xfd, 0x01},
	{0x2a, 0xf0},
	{0x2b, 0x10},
	{0x2e, 0x04},
	{0x2f, 0x18},
	{0x21, 0x60},
	{0x23, 0x60},
	{0x8b, 0xab},
	{0x8f, 0x12},
	{0xfd, 0x01},
	{0x1a, 0x80},
	{0x1b, 0x80},
	{0x43, 0x80},
	{0x00, 0xd4},
	{0x01, 0xb0},
	{0x02, 0x90},
	{0x03, 0x78},
	{0x35, 0xd6},
	{0x36, 0xf0},
	{0x37, 0x7a},
	{0x38, 0x9a},
	{0x39, 0xab},
	{0x3a, 0xca},
	{0x3b, 0xa3},
	{0x3c, 0xc1},
	{0x31, 0x82},
	{0x32, 0xa5},
	{0x33, 0xd6},
	{0x34, 0xec},
	{0x3d, 0xa5},
	{0x3e, 0xc2},
	{0x3f, 0xa7},
	{0x40, 0xc5},
	{0xfd, 0x01},
	{0x1c, 0xc0},
	{0x1d, 0x95},
	{0xa0, 0xa6},
	{0xa1, 0xda},
	{0xa2, 0x00},
	{0xa3, 0x06},
	{0xa4, 0xb2},
	{0xa5, 0xc7},
	{0xa6, 0x00},
	{0xa7, 0xce},
	{0xa8, 0xb2},
	{0xa9, 0x0c},
	{0xaa, 0x30},
	{0xab, 0x0c},
	{0xac, 0xc0},
	{0xad, 0xc0},
	{0xae, 0x00},
	{0xaf, 0xf2},
	{0xb0, 0xa6},
	{0xb1, 0xe8},
	{0xb2, 0x00},
	{0xb3, 0xe7},
	{0xb4, 0x99},
	{0xb5, 0x0c},
	{0xb6, 0x33},
	{0xb7, 0x0c},
	{0xfd, 0x00},
	{0xbf, 0x01},
	{0xbe, 0xbb},
	{0xc0, 0xb0},
	{0xc1, 0xf0},
	{0xd3, 0x77},
	{0xd4, 0x77},
	{0xd6, 0x77},
	{0xd7, 0x77},
	{0xd8, 0x77},
	{0xd9, 0x77},
	{0xda, 0x77},
	{0xdb, 0x77},
	{0xfd, 0x00},
	{0xf3, 0x03},
	{0xb0, 0x00},
	{0xb1, 0x23},
	{0xfd, 0x00},
	{0x8b, 0x00},
	{0x8c, 0x0A},
	{0x8d, 0x13},
	{0x8e, 0x25},
	{0x8f, 0x43},
	{0x90, 0x5D},
	{0x91, 0x74},
	{0x92, 0x88},
	{0x93, 0x9A},
	{0x94, 0xA9},
	{0x95, 0xB5},
	{0x96, 0xC0},
	{0x97, 0xCA},
	{0x98, 0xD4},
	{0x99, 0xDD},
	{0x9a, 0xE6},
	{0x9b, 0xEF},
	{0xfd, 0x01},
	{0x8d, 0xF7},
	{0x8e, 0xFF},
	{0xfd, 0x00},
	{0x78, 0x00},
	{0x79, 0x0A},
	{0x7a, 0x13},
	{0x7b, 0x25},
	{0x7c, 0x43},
	{0x7d, 0x5D},
	{0x7e, 0x74},
	{0x7f, 0x88},
	{0x80, 0x9A},
	{0x81, 0xA9},
	{0x82, 0xB5},
	{0x83, 0xC0},
	{0x84, 0xCA},
	{0x85, 0xD4},
	{0x86, 0xDD},
	{0x87, 0xE6},
	{0x88, 0xEF},
	{0x89, 0xF7},
	{0x8a, 0xFF},
	{0xfd, 0x01},
	{0x96, 0x46},
	{0x97, 0x14},
	{0x9f, 0x06},
	{0xfd, 0x00},
	{0xdd, 0x80},
	{0xde, 0x95},
	{0xdf, 0x80},
	{0xfd, 0x00},
	{0xec, 0x70},
	{0xed, 0x86},
	{0xee, 0x70},
	{0xef, 0x86},
	{0xf7, 0x80},
	{0xf8, 0x74},
	{0xf9, 0x80},
	{0xfa, 0x74},
	{0xfd, 0x01},
	{0xdf, 0x0f},
	{0xe5, 0x10},
	{0xe7, 0x10},
	{0xe8, 0x20},
	{0xec, 0x20},
	{0xe9, 0x20},
	{0xed, 0x20},
	{0xea, 0x10},
	{0xef, 0x10},
	{0xeb, 0x10},
	{0xf0, 0x10},
	{0xfd, 0x01},
	{0x70, 0x76},
	{0x7b, 0x40},
	{0x81, 0x30},
	{0xfd, 0x00},
	{0xb2, 0x10},
	{0xb3, 0x1f},
	{0xb4, 0x30},
	{0xb5, 0x50},
	{0xfd, 0x00},
	{0x5b, 0x20},
	{0x61, 0x80},
	{0x77, 0x80},
	{0xca, 0x80},
	{0xab, 0x00},
	{0xac, 0x02},
	{0xae, 0x08},
	{0xaf, 0x20},
	{0xfd, 0x00},
	{0x31, 0x10},
	{0x32, 0x0d},
	{0x33, 0xcf},
	{0x34, 0x7f},
	{0xe7, 0x03},
	{0xe7, 0x00},
	ENDMARKER
};

/**
 * 1280x720
 */
static const struct regval_list sp2518_720P_regs[] = {
	{0xfd, 0x00},
	{0x36, 0x1f},
	{0x47, 0x00},
	{0x48, 0xf0},
	{0x49, 0x02},
	{0x4a, 0xd0},
	{0x4b, 0x00},
	{0x4c, 0xa0},
	{0x4d, 0x05},
	{0x4e, 0x00},
	{0xfd, 0x01},
	{0x0e, 0x00},
	{0xfd, 0x00},
	ENDMARKER
};

/**
 * 720x480
 */
static const struct regval_list sp2518_480P_regs[] = {
	{0xfd, 0x00},
	{0x36, 0x1f},
	{0x47, 0x01},
	{0x48, 0x68},
	{0x49, 0x01},
	{0x4a, 0xe0},
	{0x4b, 0x01},
	{0x4c, 0xb8},
	{0x4d, 0x02},
	{0x4e, 0xd0},
	{0xfd, 0x01},
	{0x06, 0x00},
	{0x07, 0x25},
	{0x08, 0x00},
	{0x09, 0x28},
	{0x0a, 0x04},
	{0x0b, 0x00},
	{0x0c, 0x05},
	{0x0d, 0x00},
	{0x0e, 0x00},
	{0xfd, 0x00},
	ENDMARKER
};

/**
 * 704x480
 */
static const struct regval_list sp2518_704X480_regs[] = {
	{0xfd, 0x00},
	{0x36, 0x1f},
	{0x47, 0x01},
	{0x48, 0x68},
	{0x49, 0x01},
	{0x4a, 0xe0},
	{0x4b, 0x01},
	{0x4c, 0xc0},
	{0x4d, 0x02},
	{0x4e, 0xc0},
	{0xfd, 0x01},
	{0x06, 0x00},
	{0x07, 0x25},
	{0x08, 0x00},
	{0x09, 0x28},
	{0x0a, 0x04},
	{0x0b, 0x00},
	{0x0c, 0x05},
	{0x0d, 0x00},
	{0x0e, 0x00},
	ENDMARKER
};

/**
 * 640x480
 */
static const struct regval_list sp2518_640X480_regs[] = {
	{0xfd, 0x00},
	{0x36, 0x1f},
	{0x47, 0x00},
	{0x48, 0x00},
	{0x49, 0x04},
	{0x4a, 0xb0},
	{0x4b, 0x00},
	{0x4c, 0x00},
	{0x4d, 0x06},
	{0x4e, 0x40},
	{0xfd, 0x01},
	{0x06, 0x00},
	{0x07, 0x50},
	{0x08, 0x00},
	{0x09, 0x50},
	{0x0a, 0x01},
	{0x0b, 0xe0},
	{0x0c, 0x02},
	{0x0d, 0x80},
	{0x0e, 0x01},
	ENDMARKER
};

/**
 * 320x240
 */
static const struct regval_list sp2518_320X240_regs[] = {
	{0xfd, 0x00},
	{0x36, 0x1f},
	{0x47, 0x01},
	{0x48, 0xe0},
	{0x49, 0x00},
	{0x4a, 0xf0},
	{0x4b, 0x02},
	{0x4c, 0x80},
	{0x4d, 0x01},
	{0x4e, 0x40},
	{0xfd, 0x01},
	{0x06, 0x00},
	{0x07, 0x25},
	{0x08, 0x00},
	{0x09, 0x28},
	{0x0a, 0x04},
	{0x0b, 0x00},
	{0x0c, 0x05},
	{0x0d, 0x00},
	{0x0e, 0x00},
	ENDMARKER
};

/**
 * 352x288
 */
static const struct regval_list sp2518_CIF_regs[] = {
	{0xfd, 0x00},
	{0x36, 0x1f},
	{0x47, 0x00},
	{0x48, 0x00},
	{0x49, 0x04},
	{0x4a, 0xb0},
	{0x4b, 0x00},
	{0x4c, 0x00},
	{0x4d, 0x06},
	{0x4e, 0x40},
	{0xfd, 0x01},
	{0x06, 0x00},
	{0x07, 0x85},
	{0x08, 0x00},
	{0x09, 0x91},
	{0x0a, 0x01},
	{0x0b, 0x20},
	{0x0c, 0x01},
	{0x0d, 0x60},
	{0x0e, 0x01},
	ENDMARKER
};

/**
 * 176x144
 */
static const struct regval_list sp2518_QCIF_regs[] = {
	{0xfd, 0x00},
	{0x36, 0x1f},
	{0x47, 0x00},
	{0x48, 0x00},
	{0x49, 0x04},
	{0x4a, 0xb0},
	{0x4b, 0x00},
	{0x4c, 0x00},
	{0x4d, 0x06},
	{0x4e, 0x40},
	{0xfd, 0x01},
	{0x06, 0x01},
	{0x07, 0x0a},
	{0x08, 0x01},
	{0x09, 0x22},
	{0x0a, 0x00},
	{0x0b, 0x90},
	{0x0c, 0x00},
	{0x0d, 0xb0},
	{0x0e, 0x01},
	ENDMARKER
};

/**
 * 800x600
 */
static const struct regval_list sp2518_svga_regs[] = {
	{0xfd, 0x00},
	{0x36, 0x1f},
	{0x47, 0x00},
	{0x48, 0x00},
	{0x49, 0x04},
	{0x4a, 0xb0},
	{0x4b, 0x00},
	{0x4c, 0x00},
	{0x4d, 0x06},
	{0x4e, 0x40},
	{0xfd, 0x01},
	{0x06, 0x00},
	{0x07, 0x40},
	{0x08, 0x00},
	{0x09, 0x40},
	{0x0a, 0x02},
	{0x0b, 0x58},
	{0x0c, 0x03},
	{0x0d, 0x20},
	{0x0e, 0x21},
	{0xfd, 0x00},
	ENDMARKER
};

/**
 * 1600x1200
 */
static const struct regval_list sp2518_uxga_regs[] = {
	{0xfd, 0x00},
	{0x36, 0x1f},
	{0x47, 0x00},
	{0x48, 0x00},
	{0x49, 0x04},
	{0x4a, 0xb0},
	{0x4b, 0x00},
	{0x4c, 0x00},
	{0x4d, 0x06},
	{0x4e, 0x40},
	{0xfd, 0x01},
	{0x0e, 0x00},
	{0x0f, 0x00},
	{0xfd, 0x00},
	ENDMARKER,
};

static const struct regval_list sp2518_disable_regs[] = {
	{0xfd, 0x00},
	{0x36, 0x00},
	{0xfd, 0x00},
	ENDMARKER,
};

/***************************************************************************
 * structures
 */
struct sp2518_win_size {
	char *name;
	__u32 width;
	__u32 height;
	__u32 exposure_line_width;
	__u32 capture_maximum_shutter;
	const struct regval_list *win_regs;
	const struct regval_list *lsc_regs;
	unsigned int *frame_rate_array;
};

struct sp2518_priv {
	struct v4l2_subdev subdev;
	struct media_pad pad;
	const struct sp2518_color_format *cfmt;
	const struct sp2518_win_size *win;
	int model;
	bool initialized;

	struct v4l2_rect rect;
	struct v4l2_fract timeperframe;
};

struct sp2518_color_format {
	u32 code;
	enum v4l2_colorspace colorspace;
};

/*****************************************************************************
 * tables
 */
static const struct sp2518_color_format sp2518_cfmts[] = {
	{
		.code = MEDIA_BUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
	},
	{
		.code = MEDIA_BUS_FMT_UYVY8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
	},
	{
		.code = MEDIA_BUS_FMT_YVYU8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
	},
	{
		.code = MEDIA_BUS_FMT_VYUY8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
	},
};

/*
 * window size list
 */
#define QCIF_WIDTH 176
#define QCIF_HEIGHT 144
#define CIF_WIDTH 352
#define CIF_HEIGHT 288
#define X480P_WIDTH 720
#define X480P_HEIGHT 480
#define VGA_WIDTH 640
#define VGA_HEIGHT 480
#define X720P_WIDTH 1280
#define X720P_HEIGHT 720
#define UXGA_WIDTH 1600
#define UXGA_HEIGHT 1200
#define SVGA_WIDTH 800
#define SVGA_HEIGHT 600
#define SP2518_MAX_WIDTH UXGA_WIDTH
#define SP2518_MAX_HEIGHT UXGA_HEIGHT
#define AHEAD_LINE_NUM 15
#define DROP_NUM_CAPTURE 0
#define DROP_NUM_PREVIEW 0

static unsigned int frame_rate_svga[] = {
	12,
};
static unsigned int frame_rate_uxga[] = {
	12,
};

/* 1280X720 */
static const struct sp2518_win_size sp2518_win_720P = {
	.name = "720P",
	.width = X720P_WIDTH,
	.height = X720P_HEIGHT,
	.win_regs = sp2518_720P_regs,
	.frame_rate_array = frame_rate_svga,
};

/* 720X480 */
static const struct sp2518_win_size sp2518_win_480P = {
	.name = "480P",
	.width = X480P_WIDTH,
	.height = X480P_HEIGHT,
	.win_regs = sp2518_480P_regs,
	.frame_rate_array = frame_rate_svga,
};

/* 704X480 */
static const struct sp2518_win_size sp2518_win_704X480 = {
	.name = "704X480",
	.width = 704,
	.height = 480,
	.win_regs = sp2518_704X480_regs,
	.frame_rate_array = frame_rate_svga,
};

/* 640X480 */
static const struct sp2518_win_size sp2518_win_640X480 = {
	.name = "640X480",
	.width = VGA_WIDTH,
	.height = VGA_HEIGHT,
	.win_regs = sp2518_640X480_regs,
	.frame_rate_array = frame_rate_svga,
};

/* 320X240 */
static const struct sp2518_win_size sp2518_win_320X240 = {
	.name = "CIF",
	.width = 320,
	.height = 240,
	.win_regs = sp2518_320X240_regs,
	.frame_rate_array = frame_rate_svga,
};

/* 352X288 */
static const struct sp2518_win_size sp2518_win_CIF = {
	.name = "CIF",
	.width = CIF_WIDTH,
	.height = CIF_HEIGHT,
	.win_regs = sp2518_CIF_regs,
	.frame_rate_array = frame_rate_svga,
};

/* 176X144 */
static const struct sp2518_win_size sp2518_win_QCIF = {
	.name = "QCIF",
	.width = QCIF_WIDTH,
	.height = QCIF_HEIGHT,
	.win_regs = sp2518_QCIF_regs,
	.frame_rate_array = frame_rate_svga,
};

/* 800X600 */
static const struct sp2518_win_size sp2518_win_svga = {
	.name = "SVGA",
	.width = SVGA_WIDTH,
	.height = SVGA_HEIGHT,
	.win_regs = sp2518_svga_regs,
	.frame_rate_array = frame_rate_svga,
};

/* 1600X1200 */
static const struct sp2518_win_size sp2518_win_uxga = {
	.name = "UXGA",
	.width = UXGA_WIDTH,
	.height = UXGA_HEIGHT,
	.win_regs = sp2518_uxga_regs,
	.frame_rate_array = frame_rate_uxga,
};

static const struct sp2518_win_size *sp2518_win[] = {
	&sp2518_win_720P,
	&sp2518_win_480P,
	&sp2518_win_704X480,
	&sp2518_win_640X480,
	&sp2518_win_CIF,
	&sp2518_win_320X240,
	&sp2518_win_QCIF,
	&sp2518_win_svga, &sp2518_win_uxga,
};

/****************************************************************************
 * general functions
 */
static inline struct sp2518_priv *to_priv(struct v4l2_subdev *subdev)
{
	return container_of(subdev, struct sp2518_priv, subdev);
}

static bool check_id(struct i2c_client *client)
{
	u8 pid = i2c_smbus_read_byte_data(client, PID);

	if (pid == SP2518)
		return true;

	return false;
}

static int sp2518_write_array(struct i2c_client *client,
			      const struct regval_list *vals)
{
	int ret;

	while (vals->reg_num != 0xff) {
		ret = i2c_smbus_write_byte_data(client, vals->reg_num,
						vals->value);
		if (ret < 0)
			return ret;

		vals++;
	}
	return 0;
}

static const struct sp2518_win_size *sp2518_select_win(struct v4l2_subdev *sd,
						u32 width, u32 height)
{
	const struct sp2518_win_size *win;
	int i;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	for (i = 0; i < ARRAY_SIZE(sp2518_win); i++) {
		win = sp2518_win[i];
		if (width == win->width && height == win->height)
			return win;
	}
	dev_err(&client->dev, "%s: unsupported width, height (%dx%d)\n",
		__func__, width, height);
	return NULL;
}

static int sp2518_set_mbusformat(struct i2c_client *client,
				 const struct sp2518_color_format *cfmt)
{
	u32 code;
	int ret = -1;

	code = cfmt->code;

	switch (code) {
	case MEDIA_BUS_FMT_YUYV8_2X8:
		ret = sp2518_write_array(client, sp2518_fmt_yuv422_yuyv);
		break;
	case MEDIA_BUS_FMT_UYVY8_2X8:
		ret = sp2518_write_array(client, sp2518_fmt_yuv422_uyvy);
		break;
	case MEDIA_BUS_FMT_YVYU8_2X8:
		ret = sp2518_write_array(client, sp2518_fmt_yuv422_yvyu);
		break;
	case MEDIA_BUS_FMT_VYUY8_2X8:
		ret = sp2518_write_array(client, sp2518_fmt_yuv422_vyuy);
		break;
	default:
		dev_err(&client->dev, "mbus code error in %s() line %d\n",
		       __func__, __LINE__);
	}
	return ret;
}

static int sp2518_set_params(struct v4l2_subdev *sd, u32 *width, u32 *height,
			     u32 code)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct sp2518_priv *priv = to_priv(sd);
	const struct sp2518_win_size *old_win, *new_win;
	int i;

	/*
	 * select format
	 */
	priv->cfmt = NULL;
	for (i = 0; i < ARRAY_SIZE(sp2518_cfmts); i++) {
		if (code == sp2518_cfmts[i].code) {
			priv->cfmt = sp2518_cfmts + i;
			break;
		}
	}
	if (!priv->cfmt) {
		dev_err(&client->dev, "Unsupported sensor format.(0x%x)\n", code);
		return -EINVAL;
	}

	/*
	 * select win
	 */
	old_win = priv->win;
	new_win = sp2518_select_win(sd, *width, *height);
	if (!new_win) {
		dev_err(&client->dev, "Unsupported win size\n");
		return -EINVAL;
	}
	priv->win = new_win;

	priv->rect.left = 0;
	priv->rect.top = 0;
	priv->rect.width = priv->win->width;
	priv->rect.height = priv->win->height;

	*width = priv->win->width;
	*height = priv->win->height;

	return 0;
}

/****************************************************************************
 * v4l2 subdev ops
 */
static int sp2518_s_power(struct v4l2_subdev *sd, int on)
{
	/* used when suspending */
	if (!on) {
		struct sp2518_priv *priv = to_priv(sd);

		priv->initialized = false;
	}
	return 0;
}

static const struct v4l2_subdev_core_ops sp2518_subdev_core_ops = {
	.s_power = sp2518_s_power,
};

/**
 * video ops
 */
static int sp2518_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct sp2518_priv *priv = to_priv(sd);
	int ret = 0;

	priv->initialized = false;

	dev_info(&client->dev, "%s: enable %d, initialized %d\n",
		__func__, enable, priv->initialized);

	if (enable) {
		if (!priv->win || !priv->cfmt) {
			dev_err(&client->dev, "norm or win select error\n");
			return -EPERM;
		}
		/* write init regs */
		if (!priv->initialized) {
			if (!check_id(client))
				return -EINVAL;

			ret = sp2518_write_array(client,
						sp2518_svga_init_regs);
			if (ret < 0) {
				dev_err(&client->dev, "%s: ", __func__);
				dev_err(&client->dev, "failed to reg_val ");
				dev_err(&client->dev, "init regs\n");
				return -EIO;
			}
			priv->initialized = true;
		}

		ret = sp2518_write_array(client, priv->win->win_regs);
		if (ret < 0) {
			dev_err(&client->dev, "%s: ", __func__);
			dev_err(&client->dev, "failed to reg_val ");
			dev_err(&client->dev, "init regs\n");

			return -EIO;
		}

		ret = sp2518_set_mbusformat(client, priv->cfmt);
		if (ret < 0) {
			dev_err(&client->dev,
			"%s: failed to sp2518_set_mbusformat()\n", __func__);
			return -EIO;
		}
	} else {
		sp2518_write_array(client, sp2518_disable_regs);
	}

	return ret;
}

static int sp2518_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned int index,
				u32 *code)
{
	if (index >= ARRAY_SIZE(sp2518_cfmts))
		return -EINVAL;

	*code = sp2518_cfmts[index].code;
	return 0;
}

static int sp2518_g_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *mf)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct sp2518_priv *priv = to_priv(sd);

	if (!priv->win || !priv->cfmt) {
		u32 width = SVGA_WIDTH;
		u32 height = SVGA_HEIGHT;
		int ret = sp2518_set_params(sd, &width, &height,
					    MEDIA_BUS_FMT_UYVY8_2X8);
		if (ret < 0) {
			dev_info(&client->dev, "%s, %d\n", __func__, __LINE__);
			return ret;
		}
	}

	mf->width = priv->win->width;
	mf->height = priv->win->height;
	mf->code = priv->cfmt->code;
	mf->colorspace = priv->cfmt->colorspace;
	mf->field = V4L2_FIELD_NONE;
	dev_info(&client->dev, "%s, %d\n", __func__, __LINE__);
	return 0;
}

static int sp2518_try_mbus_fmt(struct v4l2_subdev *sd,
			       struct v4l2_mbus_framefmt *mf)
{
	struct sp2518_priv *priv = to_priv(sd);
	const struct sp2518_win_size *win;
	int i;

	/*
	 * select suitable win
	 */
	win = sp2518_select_win(sd, mf->width, mf->height);
	if (!win)
		return -EINVAL;

	mf->width = win->width;
	mf->height = win->height;
	mf->field = V4L2_FIELD_NONE;

	for (i = 0; i < ARRAY_SIZE(sp2518_cfmts); i++)
		if (mf->code == sp2518_cfmts[i].code)
			break;

	if (i == ARRAY_SIZE(sp2518_cfmts)) {
		/* Unsupported format requested. Propose either */
		if (priv->cfmt) {
			/* the current one or */
			mf->colorspace = priv->cfmt->colorspace;
			mf->code = priv->cfmt->code;
		} else {
			/* the default one */
			mf->colorspace = sp2518_cfmts[0].colorspace;
			mf->code = sp2518_cfmts[0].code;
		}
	} else {
		/* Also return the colorspace */
		mf->colorspace = sp2518_cfmts[i].colorspace;
	}

	return 0;
}

static int sp2518_s_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *mf)
{
	struct sp2518_priv *priv = to_priv(sd);
	int ret = sp2518_set_params(sd, &mf->width, &mf->height, mf->code);

	if (!ret)
		mf->colorspace = priv->cfmt->colorspace;

	return ret;
}

static const struct v4l2_subdev_video_ops sp2518_subdev_video_ops = {
	.s_stream = sp2518_s_stream,
	.enum_mbus_fmt = sp2518_enum_mbus_fmt,
	.g_mbus_fmt = sp2518_g_mbus_fmt,
	.try_mbus_fmt = sp2518_try_mbus_fmt,
	.s_mbus_fmt = sp2518_s_mbus_fmt,
};

/**
 * pad ops
 */
static int sp2518_s_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
{
	struct v4l2_mbus_framefmt *mf = &fmt->format;
	return sp2518_s_mbus_fmt(sd, mf);
}

static const struct v4l2_subdev_pad_ops sp2518_subdev_pad_ops = {
	.set_fmt = sp2518_s_fmt,
};

/**
 * subdev ops
 */
static const struct v4l2_subdev_ops sp2518_subdev_ops = {
	.core = &sp2518_subdev_core_ops,
	.video = &sp2518_subdev_video_ops,
	.pad = &sp2518_subdev_pad_ops,
};

/**
 * media_entity_operations
 */
static int sp2518_link_setup(struct media_entity *entity,
			     const struct media_pad *local,
			     const struct media_pad *remote, u32 flags)
{
	return 0;
}

static const struct media_entity_operations sp2518_media_ops = {
	.link_setup = sp2518_link_setup,
};

/****************************************************************************
 * initialize
 */
static void sp2518_priv_init(struct sp2518_priv *priv)
{
	priv->model = V4L2_IDENT_SP2518;
	priv->timeperframe.denominator = 12; /* 30; */
	priv->timeperframe.numerator = 1;
	priv->win = &sp2518_win_svga;
}

static int sp2518_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct sp2518_priv *priv;
	struct v4l2_subdev *sd;

	priv = kzalloc(sizeof(struct sp2518_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	sp2518_priv_init(priv);

	sd = &priv->subdev;
	strcpy(sd->name, MODULE_NAME);

	/* register subdev */
	v4l2_i2c_subdev_init(sd, client, &sp2518_subdev_ops);

	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	priv->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
	sd->entity.ops = &sp2518_media_ops;
	if (media_entity_init(&sd->entity, 1, &priv->pad, 0)) {
		dev_err(&client->dev, "%s : failed to ", __func__);
		dev_err(&client->dev, "media_entity_init()\n");
		kfree(priv);
		return -ENOENT;
	}

	return 0;
}

static int sp2518_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	kfree(to_priv(sd));
	return 0;
}

static const struct i2c_device_id sp2518_id[] = { {MODULE_NAME, 0}, {} };

MODULE_DEVICE_TABLE(i2c, sp2518_id);

static struct i2c_driver sp2518_i2c_driver = {
	.driver = {
		.name = MODULE_NAME,
	},
	.probe = sp2518_probe,
	.remove = sp2518_remove,
	.id_table = sp2518_id,
};

module_i2c_driver(sp2518_i2c_driver);

MODULE_DESCRIPTION("SoC Camera driver for sp2518");
MODULE_AUTHOR("JongKeun Choi(jkchoi@nexell.co.kr)");
MODULE_LICENSE("GPL v2");
