/*
 * Samsung SoC series Sensor driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/videodev2.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include "s5k4ecga.h"
#include "s5k4ecga_reg.h"

#define SENSOR_NAME "S5K4ECGA"

#define NORMAL_FRAME_DELAY_MS     100
#define FLASH_AE_DELAY_MS     200
#define MAX_SOFTLANDING_DELAY_MS     450
#define CONST_SOFTLANDING_DELAY_AUTO_MS		120
#define CONST_SOFTLANDING_DELAY_MACRO_MS	200
#define POLL_TIME_MS		10
#define CAPTURE_POLL_TIME_MS    1000

#define FIRST_AF_SEARCH_COUNT   80
#define SECOND_AF_SEARCH_COUNT  80
#define AE_STABLE_SEARCH_COUNT  4
#define LOW_LIGHT_LEVEL	0x13

#define EV_MIN  EV_MINUS_4
#define CONTRAST_MIN  CONTRAST_MINUS_2
#define SATURATION_MIN  SATURATION_MINUS_2
#define SHARPNESS_MIN  SHARPNESS_MINUS_2
#define CID_INDEX(cid, min) ((cid) - (min))

#define IS_LOWLIGHT(x)	((x <= LOW_LIGHT_LEVEL) ? 1 : 0)

#define INIT_COMPLETION(x)	((x).done = 0)

enum s5k4ec_sensor_setfile_index {
	SETFILE_INDEX_4EC_DEFAULT = 0,
	SETFILE_INDEX_MAX,
};

enum s5k4ec_preview_frame_size {
	S5K4EC_PREVIEW_144_176 = 0,	/* 144x176 */
	S5K4EC_PREVIEW_QCIF,		/* 176x144 */
	S5K4EC_PREVIEW_CIF,		/* 352x288 */
	S5K4EC_PREVIEW_QVGA,		/* 320x240 */
	S5K4EC_PREVIEW_VGA,		/* 640x480 */
	S5K4EC_PREVIEW_704_576,		/* 704x576 */
	S5K4EC_PREVIEW_D1,		/* 720x480 */
	S5K4EC_PREVIEW_720_540,		/* 720x540 */
	S5K4EC_PREVIEW_720_720,		/* 720x720 */
	S5K4EC_PREVIEW_WVGA,		/* 800x480 */
	S5K4EC_PREVIEW_SVGA,		/* 800x600 */
	S5K4EC_PREVIEW_880,		/* 880x720 */
	S5K4EC_PREVIEW_960_540,		/* 960x540 */
	S5K4EC_PREVIEW_960_720,		/* 960x720 */
	S5K4EC_PREVIEW_960,		/* 960x960 */
	S5K4EC_PREVIEW_964_964,		/* 964x964 */
	S5K4EC_PREVIEW_WSVGA,		/* 1024x600*/
	S5K4EC_PREVIEW_1024,		/* 1024x768*/
	S5K4EC_PREVIEW_W1280,		/* 1280x720*/
	S5K4EC_PREVIEW_WXGA,		/* 1280x768*/
	S5K4EC_PREVIEW_1280,		/* 1280x960*/
	S5K4EC_PREVIEW_1288_772,	/* 1288x772*/
	S5K4EC_PREVIEW_1288_966,	/* 1288x966*/
	S5K4EC_PREVIEW_1600,		/* 1600x1200*/
	S5K4EC_PREVIEW_1920,		/* 1920x1080*/
	S5K4EC_PREVIEW_2048,		/* 2048x1536*/
	S5K4EC_PREVIEW_2560,		/* 2560x1920*/
	S5K4EC_PREVIEW_MAX,
};

enum s5k4ec_capture_frame_size {
	S5K4EC_CAPTURE_QVGA = 0,	/* 640x480 */
	S5K4EC_CAPTURE_VGA,		/* 320x240 */
	S5K4EC_CAPTURE_WVGA,		/* 800x480 */
	S5K4EC_CAPTURE_SVGA,		/* 800x600 */
	S5K4EC_CAPTURE_WSVGA,		/* 1024x600 */
	S5K4EC_CAPTURE_XGA,		/* 1024x768 */
	S5K4EC_CAPTURE_1MP,		/* 1280x960 */
	S5K4EC_CAPTURE_W1MP,		/* 1600x960 */
	S5K4EC_CAPTURE_2MP,		/* UXGA  - 1600x1200 */
	S5K4EC_CAPTURE_1632_1218,	/* 1632x1218 */
	S5K4EC_CAPTURE_1920_1920,	/* 1920x1920 */
	S5K4EC_CAPTURE_1928_1928,	/* 1928x1928 */
	S5K4EC_CAPTURE_2036_1526,	/* 2036x1526 */
	S5K4EC_CAPTURE_2048_1152,	/* 2048x1152 */
	S5K4EC_CAPTURE_2048_1232,	/* 2048x1232 */
	S5K4EC_CAPTURE_W2MP,		/* 35mm Academy Offset Standard 1.66 */
					/* 2048x1232, 2.4MP */
	S5K4EC_CAPTURE_3MP,		/* QXGA  - 2048x1536 */
	S5K4EC_CAPTURE_2560_1440,	/* 2560x1440 */
	S5K4EC_CAPTURE_W4MP,		/* WQXGA - 2560x1536 */
	S5K4EC_CAPTURE_5MP,		/* 2560x1920 */
	S5K4EC_CAPTURE_2576_1544,	/* 2576x1544 */
	S5K4EC_CAPTURE_2576_1932,	/* 2576x1932 */
	S5K4EC_CAPTURE_MAX,
};

enum s5k4ec_oprmode {
	S5K4EC_OPRMODE_VIDEO = 0,
	S5K4EC_OPRMODE_IMAGE = 1,
	S5K4EC_OPRMODE_MAX,
};

enum s5k4ec_flicker_mode {
	S5K4EC_FLICKER_50HZ_AUTO = 0,
	S5K4EC_FLICKER_50HZ_FIXED,
	S5K4EC_FLICKER_60HZ_AUTO,
	S5K4EC_FLICKER_60HZ_FIXED,
	S5K4EC_FLICKER_MAX,
};

struct s5k4ec_reg_tbl {
	const u32	*reg[SETFILE_INDEX_MAX];
	const char	*setting_name[SETFILE_INDEX_MAX];
	int		array_size[SETFILE_INDEX_MAX];
};

static const struct s5k4ec_framesize preview_size_list[] = {
	{ S5K4EC_PREVIEW_144_176,	144,   176  },
	{ S5K4EC_PREVIEW_QCIF,		176,   144  },
	{ S5K4EC_PREVIEW_QVGA,		320,   240  },
	{ S5K4EC_PREVIEW_CIF,		352,   288  },
	{ S5K4EC_PREVIEW_VGA,		640,   480  },
	{ S5K4EC_PREVIEW_704_576,	704,   576  },
	{ S5K4EC_PREVIEW_D1,		720,   480  },
	{ S5K4EC_PREVIEW_720_540,	720,   540  },
	{ S5K4EC_PREVIEW_720_720,	720,   720  },
	{ S5K4EC_PREVIEW_WVGA,		800,   480  },
	{ S5K4EC_PREVIEW_960_540,	960,   540  },
	{ S5K4EC_PREVIEW_960_720,	960,   720  },
	{ S5K4EC_PREVIEW_960,		960,   960  },
	{ S5K4EC_PREVIEW_964_964,	964,   964  },
	{ S5K4EC_PREVIEW_1024,		1024,  768  },
	{ S5K4EC_PREVIEW_W1280,		1280,  720  },
	{ S5K4EC_PREVIEW_WXGA,		1280,  768  },
	{ S5K4EC_PREVIEW_1280,		1280,  960  },
	{ S5K4EC_PREVIEW_1288_772,	1288,  772  },
	{ S5K4EC_PREVIEW_1288_966,	1288,  966  },
	{ S5K4EC_PREVIEW_1920,		1920,  1080 },
	{ S5K4EC_PREVIEW_1600,		1600,  1200 },
	{ S5K4EC_PREVIEW_2048,		2048,  1536 },
	{ S5K4EC_PREVIEW_2560,		2560,  1920 },
};

static const struct s5k4ec_framesize capture_size_list[] = {
	{ S5K4EC_CAPTURE_VGA,		640,  480  },
	{ S5K4EC_CAPTURE_WVGA,		800,  480  },
	{ S5K4EC_CAPTURE_XGA,		1024, 768  },
	{ S5K4EC_CAPTURE_1MP,		1280, 960  },
	{ S5K4EC_CAPTURE_W1MP,		1600, 960  },
	{ S5K4EC_CAPTURE_2MP,		1600, 1200 },
	{ S5K4EC_CAPTURE_1632_1218,	1632, 1218 },
	{ S5K4EC_CAPTURE_1920_1920,	1920, 1920 },
	{ S5K4EC_CAPTURE_1928_1928,	1928, 1928 },
	{ S5K4EC_CAPTURE_2036_1526,	2036, 1526 },
	{ S5K4EC_CAPTURE_2048_1152,	2048, 1152 },
	{ S5K4EC_CAPTURE_2048_1232,	2048, 1232 },
	{ S5K4EC_CAPTURE_3MP,		2048, 1536 },
	{ S5K4EC_CAPTURE_2560_1440,	2560, 1440 },
	{ S5K4EC_CAPTURE_W4MP,		2560, 1536 },
	{ S5K4EC_CAPTURE_5MP,		2560, 1920 },
	{ S5K4EC_CAPTURE_2576_1544,	2576, 1544 },
	{ S5K4EC_CAPTURE_2576_1932,	2576, 1932 },
};

#define S5K4EC_REG_TBL(y)			\
	{						\
		.reg		= { s5k4ec_##y, },	\
		.setting_name	= { "s5k4ec_"#y, },	\
		.array_size	= { ARRAY_SIZE(s5k4ec_##y), },	\
	}

struct s5k4ec_regs {
	struct s5k4ec_reg_tbl ev[CID_INDEX(EV_MAX, EV_MIN)];
	struct s5k4ec_reg_tbl metering[METERING_MAX];
	struct s5k4ec_reg_tbl iso[ISO_MAX];
	struct s5k4ec_reg_tbl effect[IMAGE_EFFECT_MAX];
	struct s5k4ec_reg_tbl white_balance[WHITE_BALANCE_MAX];
	struct s5k4ec_reg_tbl preview_size[S5K4EC_PREVIEW_MAX];
	struct s5k4ec_reg_tbl capture_size[S5K4EC_CAPTURE_MAX];
	struct s5k4ec_reg_tbl scene_mode[SCENE_MODE_MAX];
	struct s5k4ec_reg_tbl saturation[SATURATION_MAX];
	struct s5k4ec_reg_tbl contrast[CONTRAST_MAX];
	struct s5k4ec_reg_tbl sharpness[SHARPNESS_MAX];
	struct s5k4ec_reg_tbl anti_banding[S5K4EC_FLICKER_MAX];
	struct s5k4ec_reg_tbl fps[FRAME_RATE_MAX];
	struct s5k4ec_reg_tbl preview_return;
	struct s5k4ec_reg_tbl jpeg_quality_high;
	struct s5k4ec_reg_tbl jpeg_quality_normal;
	struct s5k4ec_reg_tbl jpeg_quality_low;
	struct s5k4ec_reg_tbl flash_start;
	struct s5k4ec_reg_tbl flash_end;
	struct s5k4ec_reg_tbl af_assist_flash_start;
	struct s5k4ec_reg_tbl af_assist_flash_end;
	struct s5k4ec_reg_tbl af_low_light_mode_on;
	struct s5k4ec_reg_tbl af_low_light_mode_off;
	struct s5k4ec_reg_tbl aeawb_lockunlock[AE_AWB_MAX];
	struct s5k4ec_reg_tbl ae_lockunlock[AE_LOCK_MAX];
	struct s5k4ec_reg_tbl awb_lockunlock[AWB_LOCK_MAX];
	struct s5k4ec_reg_tbl wdr_on;
	struct s5k4ec_reg_tbl wdr_off;
	struct s5k4ec_reg_tbl face_detection_on;
	struct s5k4ec_reg_tbl face_detection_off;
	struct s5k4ec_reg_tbl capture_start;
	struct s5k4ec_reg_tbl normal_snapshot;
	struct s5k4ec_reg_tbl camcorder;
	struct s5k4ec_reg_tbl camcorder_disable;
	struct s5k4ec_reg_tbl af_macro_mode_1;
	struct s5k4ec_reg_tbl af_macro_mode_2;
	struct s5k4ec_reg_tbl af_macro_mode_3;
	struct s5k4ec_reg_tbl af_normal_mode_1;
	struct s5k4ec_reg_tbl af_normal_mode_2;
	struct s5k4ec_reg_tbl af_normal_mode_3;
	struct s5k4ec_reg_tbl af_return_macro_position;
	struct s5k4ec_reg_tbl single_af_start;
	struct s5k4ec_reg_tbl single_af_off_1;
	struct s5k4ec_reg_tbl single_af_off_2;
	struct s5k4ec_reg_tbl continuous_af_on;
	struct s5k4ec_reg_tbl continuous_af_off;
	struct s5k4ec_reg_tbl dtp_start;
	struct s5k4ec_reg_tbl dtp_stop;
	struct s5k4ec_reg_tbl init_reg_1;
	struct s5k4ec_reg_tbl init_reg_2;
	struct s5k4ec_reg_tbl init_reg_3;
	struct s5k4ec_reg_tbl init_reg_4;
	struct s5k4ec_reg_tbl flash_init;
	struct s5k4ec_reg_tbl reset_crop;
	struct s5k4ec_reg_tbl fast_ae_on;
	struct s5k4ec_reg_tbl fast_ae_off;
	struct s5k4ec_reg_tbl get_ae_stable_status;
	struct s5k4ec_reg_tbl get_light_level;
	struct s5k4ec_reg_tbl get_frame_duration;
	struct s5k4ec_reg_tbl get_1st_af_search_status;
	struct s5k4ec_reg_tbl get_2nd_af_search_status;
	struct s5k4ec_reg_tbl get_capture_status;
	struct s5k4ec_reg_tbl get_esd_status;
	struct s5k4ec_reg_tbl get_iso;
	struct s5k4ec_reg_tbl get_shutterspeed;
	struct s5k4ec_reg_tbl get_exptime;
	struct s5k4ec_reg_tbl get_frame_count;
	struct s5k4ec_reg_tbl get_preview_status;
	struct s5k4ec_reg_tbl get_pid;
	struct s5k4ec_reg_tbl get_revision;
	struct s5k4ec_reg_tbl get_modechange_check;
	struct s5k4ec_reg_tbl set_vendor_id_read;
	struct s5k4ec_reg_tbl get_vendor_id_read;
};

static const struct s5k4ec_regs regs_set = {
	.init_reg_1 = S5K4EC_REG_TBL(init_reg1),
	.init_reg_2 = S5K4EC_REG_TBL(init_reg2),
	.init_reg_3 = S5K4EC_REG_TBL(init_reg3),
	.init_reg_4 = S5K4EC_REG_TBL(init_reg4),

	.capture_start = S5K4EC_REG_TBL(capture_start),
	.normal_snapshot = S5K4EC_REG_TBL(normal_snapshot),
	.camcorder = S5K4EC_REG_TBL(camcorder),
	.camcorder_disable = S5K4EC_REG_TBL(camcorder_disable),

	.preview_size = {
		[S5K4EC_PREVIEW_144_176] = S5K4EC_REG_TBL(144_176_preview),
		[S5K4EC_PREVIEW_QCIF] = S5K4EC_REG_TBL(176_preview),
		[S5K4EC_PREVIEW_QVGA] = S5K4EC_REG_TBL(qvga_preview),
		[S5K4EC_PREVIEW_CIF] = S5K4EC_REG_TBL(352_preview),
		[S5K4EC_PREVIEW_VGA] = S5K4EC_REG_TBL(640_preview),
		[S5K4EC_PREVIEW_704_576] = S5K4EC_REG_TBL(704_576_preview),
		[S5K4EC_PREVIEW_D1] = S5K4EC_REG_TBL(720_preview),
		[S5K4EC_PREVIEW_720_540] = S5K4EC_REG_TBL(720_540_preview),
		[S5K4EC_PREVIEW_720_720] = S5K4EC_REG_TBL(720_720_preview),
		[S5K4EC_PREVIEW_WVGA] = S5K4EC_REG_TBL(wvga_preview),
		[S5K4EC_PREVIEW_960_540] = S5K4EC_REG_TBL(960_540_preview),
		[S5K4EC_PREVIEW_960_720] = S5K4EC_REG_TBL(960_720_preview),
		[S5K4EC_PREVIEW_960] = S5K4EC_REG_TBL(960_preview),
		[S5K4EC_PREVIEW_964_964] = S5K4EC_REG_TBL(964_964_preview),
		[S5K4EC_PREVIEW_1024] = S5K4EC_REG_TBL(1024_preview),
		[S5K4EC_PREVIEW_W1280] = S5K4EC_REG_TBL(w1280_preview),
		[S5K4EC_PREVIEW_WXGA] = S5K4EC_REG_TBL(1280_wxga_preview),
		[S5K4EC_PREVIEW_1288_772] = S5K4EC_REG_TBL(1288_772_preview),
		[S5K4EC_PREVIEW_1288_966] = S5K4EC_REG_TBL(1288_966_preview),
		[S5K4EC_PREVIEW_1280] = S5K4EC_REG_TBL(1280_preview),
		[S5K4EC_PREVIEW_1600] = S5K4EC_REG_TBL(1600_preview),
		[S5K4EC_PREVIEW_1920] = S5K4EC_REG_TBL(1920_preview),
		[S5K4EC_PREVIEW_2048] = S5K4EC_REG_TBL(2048_preview),
		[S5K4EC_PREVIEW_2560] = S5K4EC_REG_TBL(max_preview),
	},

	.capture_size = {
		[S5K4EC_CAPTURE_VGA] = S5K4EC_REG_TBL(vga_capture),
		[S5K4EC_CAPTURE_XGA] = S5K4EC_REG_TBL(xga_capture),
		[S5K4EC_CAPTURE_WVGA] = S5K4EC_REG_TBL(wvga_capture),
		[S5K4EC_CAPTURE_1MP] = S5K4EC_REG_TBL(1m_capture),
		[S5K4EC_CAPTURE_W1MP] = S5K4EC_REG_TBL(w1mp_capture),
		[S5K4EC_CAPTURE_2MP] = S5K4EC_REG_TBL(2m_capture),
		[S5K4EC_CAPTURE_1632_1218] = S5K4EC_REG_TBL(1632_1218_capture),
		[S5K4EC_CAPTURE_3MP] = S5K4EC_REG_TBL(3m_capture),
		[S5K4EC_CAPTURE_1920_1920] = S5K4EC_REG_TBL(1920_1920_capture),
		[S5K4EC_CAPTURE_1928_1928] = S5K4EC_REG_TBL(1928_1928_capture),
		[S5K4EC_CAPTURE_2036_1526] = S5K4EC_REG_TBL(2036_1526_capture),
		[S5K4EC_CAPTURE_2048_1152] = S5K4EC_REG_TBL(2048_1152_capture),
		[S5K4EC_CAPTURE_2048_1232] = S5K4EC_REG_TBL(2048_1232_capture),
		[S5K4EC_CAPTURE_W4MP] = S5K4EC_REG_TBL(w4mp_capture),
		[S5K4EC_CAPTURE_2560_1440] = S5K4EC_REG_TBL(2560_1440_capture),
		[S5K4EC_CAPTURE_5MP] = S5K4EC_REG_TBL(5m_capture),
		[S5K4EC_CAPTURE_2576_1544] = S5K4EC_REG_TBL(2576_1544_capture),
		[S5K4EC_CAPTURE_2576_1932] = S5K4EC_REG_TBL(2576_1932_capture),
	},

	.reset_crop = S5K4EC_REG_TBL(reset_crop),
	.get_capture_status =
		S5K4EC_REG_TBL(get_capture_status),
	.ev = {
		[CID_INDEX(EV_MINUS_4, EV_MIN)] = S5K4EC_REG_TBL(ev_minus_4),
		[CID_INDEX(EV_MINUS_3, EV_MIN)] = S5K4EC_REG_TBL(ev_minus_3),
		[CID_INDEX(EV_MINUS_2, EV_MIN)] = S5K4EC_REG_TBL(ev_minus_2),
		[CID_INDEX(EV_MINUS_1, EV_MIN)] = S5K4EC_REG_TBL(ev_minus_1),
		[CID_INDEX(EV_DEFAULT, EV_MIN)] = S5K4EC_REG_TBL(ev_default),
		[CID_INDEX(EV_PLUS_1, EV_MIN)] = S5K4EC_REG_TBL(ev_plus_1),
		[CID_INDEX(EV_PLUS_2, EV_MIN)] = S5K4EC_REG_TBL(ev_plus_2),
		[CID_INDEX(EV_PLUS_3, EV_MIN)] = S5K4EC_REG_TBL(ev_plus_3),
		[CID_INDEX(EV_PLUS_4, EV_MIN)] = S5K4EC_REG_TBL(ev_plus_4),
	},
	.metering = {
		[METERING_MATRIX] = S5K4EC_REG_TBL(metering_matrix),
		[METERING_CENTER] = S5K4EC_REG_TBL(metering_center),
		[METERING_SPOT] = S5K4EC_REG_TBL(metering_spot),
	},
	.iso = {
		[ISO_AUTO] = S5K4EC_REG_TBL(iso_auto),
		[ISO_50] = S5K4EC_REG_TBL(iso_100),     /* map to 100 */
		[ISO_100] = S5K4EC_REG_TBL(iso_100),
		[ISO_200] = S5K4EC_REG_TBL(iso_200),
		[ISO_400] = S5K4EC_REG_TBL(iso_400),
		[ISO_800] = S5K4EC_REG_TBL(iso_400),    /* map to 400 */
		[ISO_1600] = S5K4EC_REG_TBL(iso_400),   /* map to 400 */
		[ISO_SPORTS] = S5K4EC_REG_TBL(iso_auto),/* map to auto */
		[ISO_NIGHT] = S5K4EC_REG_TBL(iso_auto), /* map to auto */
		[ISO_MOVIE] = S5K4EC_REG_TBL(iso_auto), /* map to auto */
	},
	.effect = {
		[IMAGE_EFFECT_NONE] = S5K4EC_REG_TBL(effect_normal),
		[IMAGE_EFFECT_BNW] = S5K4EC_REG_TBL(effect_black_white),
		[IMAGE_EFFECT_SEPIA] = S5K4EC_REG_TBL(effect_sepia),
		[IMAGE_EFFECT_NEGATIVE] = S5K4EC_REG_TBL(effect_negative),
	},
	.white_balance = {
		[WHITE_BALANCE_AUTO] = S5K4EC_REG_TBL(wb_auto),
		[WHITE_BALANCE_SUNNY] = S5K4EC_REG_TBL(wb_sunny),
		[WHITE_BALANCE_CLOUDY] = S5K4EC_REG_TBL(wb_cloudy),
		[WHITE_BALANCE_TUNGSTEN] = S5K4EC_REG_TBL(wb_tungsten),
		[WHITE_BALANCE_FLUORESCENT] = S5K4EC_REG_TBL(wb_fluorescent),
	},
	.scene_mode = {
		[SCENE_MODE_NONE] = S5K4EC_REG_TBL(scene_default),
		[SCENE_MODE_PORTRAIT] = S5K4EC_REG_TBL(scene_portrait),
		[SCENE_MODE_NIGHTSHOT] = S5K4EC_REG_TBL(scene_nightshot),
		[SCENE_MODE_BACK_LIGHT] = S5K4EC_REG_TBL(scene_backlight),
		[SCENE_MODE_LANDSCAPE] = S5K4EC_REG_TBL(scene_landscape),
		[SCENE_MODE_SPORTS] = S5K4EC_REG_TBL(scene_sports),
		[SCENE_MODE_PARTY_INDOOR] = S5K4EC_REG_TBL(scene_party_indoor),
		[SCENE_MODE_BEACH_SNOW] = S5K4EC_REG_TBL(scene_beach_snow),
		[SCENE_MODE_SUNSET] = S5K4EC_REG_TBL(scene_sunset),
		[SCENE_MODE_DUSK_DAWN] = S5K4EC_REG_TBL(scene_duskdawn),
		[SCENE_MODE_FALL_COLOR] = S5K4EC_REG_TBL(scene_fall_color),
		[SCENE_MODE_FIREWORKS] = S5K4EC_REG_TBL(scene_fireworks),
		[SCENE_MODE_TEXT] = S5K4EC_REG_TBL(scene_text),
		[SCENE_MODE_CANDLE_LIGHT] = S5K4EC_REG_TBL(scene_candle_light),
	},
	.saturation = {
		[SATURATION_MINUS_2] = S5K4EC_REG_TBL(saturation_minus_2),
		[SATURATION_MINUS_1] = S5K4EC_REG_TBL(saturation_minus_1),
		[SATURATION_DEFAULT] = S5K4EC_REG_TBL(saturation_default),
		[SATURATION_PLUS_1] = S5K4EC_REG_TBL(saturation_plus_1),
		[SATURATION_PLUS_2] = S5K4EC_REG_TBL(saturation_plus_2),
	},
	.contrast = {
		[CONTRAST_MINUS_2] = S5K4EC_REG_TBL(contrast_minus_2),
		[CONTRAST_MINUS_1] = S5K4EC_REG_TBL(contrast_minus_1),
		[CONTRAST_DEFAULT] = S5K4EC_REG_TBL(contrast_default),
		[CONTRAST_PLUS_1] = S5K4EC_REG_TBL(contrast_plus_1),
		[CONTRAST_PLUS_2] = S5K4EC_REG_TBL(contrast_plus_2),
	},
	.sharpness = {
		[SHARPNESS_MINUS_2] = S5K4EC_REG_TBL(sharpness_minus_2),
		[SHARPNESS_MINUS_1] = S5K4EC_REG_TBL(sharpness_minus_1),
		[SHARPNESS_DEFAULT] = S5K4EC_REG_TBL(sharpness_default),
		[SHARPNESS_PLUS_1] = S5K4EC_REG_TBL(sharpness_plus_1),
		[SHARPNESS_PLUS_2] = S5K4EC_REG_TBL(sharpness_plus_2),
	},
	.anti_banding = {
		[S5K4EC_FLICKER_50HZ_AUTO] = S5K4EC_REG_TBL(50hz_auto),
		[S5K4EC_FLICKER_50HZ_FIXED] = S5K4EC_REG_TBL(50hz_fixed),
		[S5K4EC_FLICKER_60HZ_AUTO] = S5K4EC_REG_TBL(60hz_auto),
		[S5K4EC_FLICKER_60HZ_FIXED] = S5K4EC_REG_TBL(60hz_fixed),
	},
	.fps = {
		[FRAME_RATE_AUTO] = S5K4EC_REG_TBL(fps_auto),
		[FRAME_RATE_7] = S5K4EC_REG_TBL(fps_7),
		[10] = S5K4EC_REG_TBL(fps_10),
		[12] = S5K4EC_REG_TBL(fps_12),
		[FRAME_RATE_15] = S5K4EC_REG_TBL(fps_15),
		[FRAME_RATE_20] = S5K4EC_REG_TBL(fps_20),
		[25] = S5K4EC_REG_TBL(fps_25),
		[FRAME_RATE_30] = S5K4EC_REG_TBL(fps_30),
	},
	.preview_return = S5K4EC_REG_TBL(preview_return),
	.jpeg_quality_high = S5K4EC_REG_TBL(jpeg_quality_high),
	.jpeg_quality_normal = S5K4EC_REG_TBL(jpeg_quality_normal),
	.jpeg_quality_low = S5K4EC_REG_TBL(jpeg_quality_low),
	.flash_start = S5K4EC_REG_TBL(flash_start),
	.flash_end = S5K4EC_REG_TBL(flash_end),
	.af_assist_flash_start = S5K4EC_REG_TBL(pre_flash_start),
	.af_assist_flash_end = S5K4EC_REG_TBL(pre_flash_end),
	.af_low_light_mode_on = S5K4EC_REG_TBL(af_low_light_mode_on),
	.af_low_light_mode_off = S5K4EC_REG_TBL(af_low_light_mode_off),
	.aeawb_lockunlock = {
		[AE_UNLOCK_AWB_UNLOCK] = S5K4EC_REG_TBL(ae_awb_lock_off),
		[AE_LOCK_AWB_UNLOCK] = S5K4EC_REG_TBL(ae_lock_on_awb_lock_off),
		[AE_UNLOCK_AWB_LOCK] = S5K4EC_REG_TBL(ae_lock_off_awb_lock_on),
		[AE_LOCK_AWB_LOCK] = S5K4EC_REG_TBL(ae_awb_lock_on),
	},
	.ae_lockunlock = {
		[AE_UNLOCK] = S5K4EC_REG_TBL(ae_lock_off),
		[AE_LOCK] = S5K4EC_REG_TBL(ae_lock_on),
	},
	.awb_lockunlock = {
		[AWB_UNLOCK] = S5K4EC_REG_TBL(awb_lock_off),
		[AWB_LOCK] = S5K4EC_REG_TBL(awb_lock_on),
	},
	.wdr_on = S5K4EC_REG_TBL(wdr_on),
	.wdr_off = S5K4EC_REG_TBL(wdr_off),
	.face_detection_on = S5K4EC_REG_TBL(face_detection_on),
	.face_detection_off = S5K4EC_REG_TBL(face_detection_off),
	.af_macro_mode_1 = S5K4EC_REG_TBL(af_macro_mode_1),
	.af_macro_mode_2 = S5K4EC_REG_TBL(af_macro_mode_2),
	.af_macro_mode_3 = S5K4EC_REG_TBL(af_macro_mode_3),
	.af_normal_mode_1 = S5K4EC_REG_TBL(af_normal_mode_1),
	.af_normal_mode_2 = S5K4EC_REG_TBL(af_normal_mode_2),
	.af_normal_mode_3 = S5K4EC_REG_TBL(af_normal_mode_3),
	.af_return_macro_position =
		S5K4EC_REG_TBL(af_return_macro_pos),
	.single_af_start = S5K4EC_REG_TBL(single_af_start),
	.single_af_off_1 = S5K4EC_REG_TBL(single_af_off_1),
	.single_af_off_2 = S5K4EC_REG_TBL(single_af_off_2),
	.continuous_af_on = S5K4EC_REG_TBL(continuous_af_on),
	.continuous_af_off = S5K4EC_REG_TBL(continuous_af_off),
	.dtp_start = S5K4EC_REG_TBL(dtp_init),
	.dtp_stop = S5K4EC_REG_TBL(dtp_stop),

	.flash_init = S5K4EC_REG_TBL(flash_init),
	.reset_crop = S5K4EC_REG_TBL(reset_crop),
	.fast_ae_on =  S5K4EC_REG_TBL(fast_ae_on),
	.fast_ae_off =  S5K4EC_REG_TBL(fast_ae_off),
	.get_ae_stable_status =
		S5K4EC_REG_TBL(get_ae_stable_status),
	.get_light_level = S5K4EC_REG_TBL(get_light_level),
	.get_frame_duration = S5K4EC_REG_TBL(get_frame_duration_reg),
	.get_1st_af_search_status =
		S5K4EC_REG_TBL(get_1st_af_search_status),
	.get_2nd_af_search_status =
		S5K4EC_REG_TBL(get_2nd_af_search_status),
	.get_capture_status =
		S5K4EC_REG_TBL(get_capture_status),
	.get_esd_status = S5K4EC_REG_TBL(get_esd_status),
	.get_iso = S5K4EC_REG_TBL(get_iso_reg),
	.get_shutterspeed =
		S5K4EC_REG_TBL(get_shutterspeed_reg),
	.get_exptime =
		S5K4EC_REG_TBL(get_exptime_reg),
	.get_frame_count =
		S5K4EC_REG_TBL(get_frame_count_reg),
	.get_preview_status =
		S5K4EC_REG_TBL(get_preview_status_reg),
	.get_pid =
		S5K4EC_REG_TBL(get_pid_reg),
	.get_revision =
		S5K4EC_REG_TBL(get_revision_reg),
	.get_modechange_check =
		S5K4EC_REG_TBL(get_modechange_check_reg),
	.set_vendor_id_read =
		S5K4EC_REG_TBL(set_vendor_id_read_reg),
	.get_vendor_id_read =
		S5K4EC_REG_TBL(get_vendor_id_read_reg),
};

struct s5k4ec_regset {
	u32 size;
	u8 *data;
};

static inline struct s5k4ec_state *to_state
	(struct v4l2_subdev *subdev)
{
	struct s5k4ec_state *state =
		container_of(subdev, struct s5k4ec_state, subdev);
	return state;
}

static inline struct i2c_client *to_client
	(struct v4l2_subdev *subdev)
{
	return (struct i2c_client *)v4l2_get_subdevdata(subdev);
}

static int sensor_4ec_i2c_write(struct i2c_client *client,
	u8 *buf, u32 size)
{
	int ret = 0;
	int retry_count = 1;
	struct i2c_msg msg = {client->addr, 0, size, buf};

	do {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (likely(ret == 1))
			break;
		usleep_range(10000, 10001);
	} while (retry_count-- > 0);

	if (ret != 1) {
		dev_err(&client->dev, "%s: I2C is not working.\n", __func__);
		return -EIO;
	}

	return 0;
}

static int sensor_4ec_i2c_write16(struct i2c_client *client,
		u16 addr, u16 w_data)
{
	int retry_count = 1;
	unsigned char buf[4];
	struct i2c_msg msg = {client->addr, 0, 4, buf};
	int ret = 0;

	buf[0] = addr >> 8;
	buf[1] = addr;
	buf[2] = w_data >> 8;
	buf[3] = w_data & 0xff;

	do {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (likely(ret == 1))
			break;
		usleep_range(10000, 10001);
		dev_err(&client->dev, "%s: I2C err %d, retry %d.\n",
				__func__, ret, retry_count);
	} while (retry_count-- > 0);

	if (ret != 1) {
		dev_err(&client->dev, "%s: I2C is not working.\n", __func__);
		return -EIO;
	}

	return 0;
}

static int sensor_4ec_i2c_read16(struct i2c_client *client,
		u16 subaddr, u16 *data)
{
	int err;
	unsigned char buf[2];
	struct i2c_msg msg[2];

	cpu_to_be16s(&subaddr);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = (u8 *)&subaddr;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = buf;

	err = i2c_transfer(client->adapter, msg, 2);
	if (unlikely(err != 2)) {
		dev_err(&client->dev,
				"%s: register read fail\n", __func__);
		return -EIO;
	}

	*data = ((buf[0] << 8) | buf[1]);

	return 0;
}

#ifdef CONFIG_LOAD_FILE
static char *sensor_4ec_regs_table;
static int sensor_4ec_regs_table_size;

int sensor_4ec_regs_table_init(void)
{
	struct file *filp;
	char *dp;
	long size;
	loff_t pos;
	int ret;
	mm_segment_t fs = get_fs();

	cam_info("E\n");

	set_fs(get_ds());

	filp = filp_open(LOAD_FILE_PATH, O_RDONLY, 0);

	if (IS_ERR_OR_NULL(filp)) {
		cam_err("file open error\n");
		return PTR_ERR(filp);
	}

	size = filp->f_path.dentry->d_inode->i_size;
	cam_dbg("size = %ld\n", size);
	dp = vmalloc(size);
	if (unlikely(!dp)) {
		cam_err("Out of Memory\n");
		filp_close(filp, current->files);
		return -ENOMEM;
	}

	pos = 0;
	memset(dp, 0, size);
	ret = vfs_read(filp, (char __user *)dp, size, &pos);

	if (unlikely(ret != size)) {
		cam_err("Failed to read file ret = %d\n", ret);
		/*kfree(dp);*/
		vfree(dp);
		filp_close(filp, current->files);
		return -EINVAL;
	}

	filp_close(filp, current->files);

	set_fs(fs);

	sensor_4ec_regs_table = dp;

	sensor_4ec_regs_table_size = size;

	*((sensor_4ec_regs_table + sensor_4ec_regs_table_size) - 1) = '\0';

	cam_info("X\n");
	return 0;
}

static int sensor_4ec_write_regs_from_sd(struct v4l2_subdev *sd,
	const char *name)
{
	char *start = NULL, *end = NULL, *reg = NULL, *temp_start = NULL;
	u16 addr = 0, value = 0;
	u16 data = 0;
	char data_buf[7] = {0, };
	u32 len = 0;
	int err = 0;
	int i;
	int cnt = 0;
	struct i2c_client *client = to_client(sd);

	cam_info("E, sensor_4ec_regs_table_size - %d\n",
		sensor_4ec_regs_table_size);

	addr = value = 0;

	*(data_buf + 6) = '\0';

	start = strnstr(sensor_4ec_regs_table, name,
		sensor_4ec_regs_table_size);

	do {
		if (*(start + strlen(name)) == '[') {
			cam_info("break to search (%s)\n", name);
			break;
		}

		cam_info("searching (%s)\n", name);
		start = start + strlen(name);
		start = strnstr(start, name, strlen(start));
	} while (start != NULL);

	if (unlikely(start == NULL)) {
		cam_err("start is NULL\n");
		return -ENODATA;
	}

	end = strnstr(start, "};", sensor_4ec_regs_table_size);
	if (unlikely(end == NULL)) {
		cam_err("end is NULL\n");
		return -ENODATA;
	}

	while (1) {
		if (cnt < 1) {
			len = end - start;
			temp_start = strnstr(start, "{", len);
			if (!temp_start || (temp_start > end)) {
				cam_info("write end of %s\n", name);
				break;
			}
			start = temp_start;
		}

		len = end - start;
		/* Find Address */
		reg = strnstr(start, "0x", len);
		if (!reg || (reg > end)) {
			cam_info("write end of %s\n", name);
			break;
		}

		start = (reg + 6);

		/* Write Value to Address */
		memcpy(data_buf, reg, 6);

		err = kstrtou16(data_buf, 16, &data);
		if (unlikely(err < 0)) {
			cam_err("kstrtou16 failed\n");
			return err;
		}

		addr = data;
		len = end - start;

		/* Write Value to Address */
		memset(data_buf, 0, sizeof(char) * 7);
		memcpy(data_buf, "0x", 2);
		for (i = 0; i < 4; i++)
			data_buf[i+2] = *(reg + 6 + i);

		err = kstrtou16(data_buf, 16, &data);
		if (unlikely(err < 0)) {
			cam_err("kstrtou16 failed\n");
			return err;
		}

		value = data;

		if (addr == 0xFFFF) {
			cam_info("use delay (%d ms) in I2C Write\n",
				value);
			msleep(value);
		} else {
			err = sensor_4ec_i2c_write16(client, addr, value);
			cam_info("(%s)addr(0x%x)value(0x%x)\n",
				name, addr, value);
			if (unlikely(err < 0)) {
				cam_err("register set failed\n");
				return err;
			}
		}
		cnt++;
	}

	cam_info("X\n");

	return err;
}
#endif

static int sensor_4ec_apply_set(struct v4l2_subdev *subdev,
	const struct s5k4ec_reg_tbl *regset)
{
	int ret = 0;
#ifndef CONFIG_LOAD_FILE
	u16 addr, val;
	u32 i;
#endif
	struct i2c_client *client = to_client(subdev);
	struct s5k4ec_state *s5k4ec_state = to_state(subdev);
	u32 set_idx = s5k4ec_state->setfile_index;

	BUG_ON(!client);
	BUG_ON(!regset);
	BUG_ON(!s5k4ec_state);

	mutex_lock(&s5k4ec_state->i2c_lock);

	cam_info("E, setting_name : %s\n",
		regset->setting_name[set_idx]);

#ifdef CONFIG_LOAD_FILE
	cam_info("COFIG_LOAD_FILE feature is enabled\n");
	ret = sensor_4ec_write_regs_from_sd(subdev,
		regset->setting_name[set_idx]);
	if (unlikely(ret < 0)) {
		cam_err("regs set(%s)apply is fail(%d)\n",
			regset->setting_name[set_idx], ret);
		goto p_err;
	}
#else
	for (i = 0; i < regset->array_size[set_idx]; i++) {
		addr = (regset->reg[set_idx][i] & 0xFFFF0000) >> 16;
		val = regset->reg[set_idx][i] & 0x0000FFFF;
		if (addr == 0xFFFF) {
			cam_dbg("use delay (%d ms) in I2C Write\n",
				val);
			msleep(val);
		} else {
			ret = sensor_4ec_i2c_write16(client, addr, val);
			if (unlikely(ret < 0)) {
				cam_err("failed to set regs(0x%08lx, %d)(%d)",
					(uintptr_t)regset->reg[set_idx],
					regset->array_size[set_idx], ret);
				goto p_err;
			}
		}
	}
#endif

p_err:
	mutex_unlock(&s5k4ec_state->i2c_lock);
	return ret;
}

static int sensor_4ec_read_reg(struct v4l2_subdev *subdev,
	const struct s5k4ec_reg_tbl *regset, u16 *data, u32 count)
{
	int ret = 0;
	u16 addr, val;
	u32 i;
	struct i2c_client *client = to_client(subdev);
	struct s5k4ec_state *s5k4ec_state = to_state(subdev);
	u32 set_idx = s5k4ec_state->setfile_index;

	BUG_ON(!client);
	BUG_ON(!s5k4ec_state);

	mutex_lock(&s5k4ec_state->i2c_lock);

	/* Enter read mode */
	ret = sensor_4ec_i2c_write16(client, 0x002C, 0x7000);
	if (unlikely(ret < 0)) {
		cam_err("write reg fail(%d)", ret);
		goto write_err;
	}

	for (i = 0; i < regset->array_size[set_idx]; i++) {
		addr = (regset->reg[set_idx][i] & 0xFFFF0000) >> 16;
		val = regset->reg[set_idx][i] & 0x0000FFFF;
		ret = sensor_4ec_i2c_write16(client, addr, val);
		if (unlikely(ret < 0)) {
			cam_err("failed to set regs(0x%08lx, %d)(%d)",
				(uintptr_t)regset->reg[set_idx],
				regset->array_size[set_idx], ret);
			goto p_err;
		}
	}

	for (i = 0; i < count; i++, data++) {
		ret = sensor_4ec_i2c_read16(client, 0x0F12, data);
		if (unlikely(ret < 0)) {
			cam_err("read reg fail(%d)", ret);
			goto p_err;
		}
	}

p_err:
	/* restore write mode */
	sensor_4ec_i2c_write16(client, 0x0028, 0x7000);
write_err:
	mutex_unlock(&s5k4ec_state->i2c_lock);
	return ret;
}

static int sensor_4ec_read_addr(struct v4l2_subdev *subdev,
	u16 addr, u16 *data, u32 count)
{
	int ret = 0;
	u32 i;
	struct i2c_client *client = to_client(subdev);
	struct s5k4ec_state *s5k4ec_state = to_state(subdev);

	BUG_ON(!client);
	BUG_ON(!s5k4ec_state);

	mutex_lock(&s5k4ec_state->i2c_lock);

	/* Enter read mode */
	sensor_4ec_i2c_write16(client, 0x002C, 0x7000);
	sensor_4ec_i2c_write16(client, 0x002e, addr);

	for (i = 0; i < count; i++, data++) {
		ret = sensor_4ec_i2c_read16(client, 0x0F12, data);
		if (unlikely(ret < 0)) {
			cam_err("read reg fail(%d)", ret);
			goto p_err;
		}
	}

p_err:
	/* restore write mode */
	sensor_4ec_i2c_write16(client, 0x0028, 0x7000);
	mutex_unlock(&s5k4ec_state->i2c_lock);
	return ret;
}

static int sensor_4ec_s_flash(struct v4l2_subdev *subdev,
	int value)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	cam_info("%d\n", value);

	s5k4ec_state = to_state(subdev);

	if (s5k4ec_state->flash_mode == value) {
		cam_warn("already flash mode(%d) is set\n", value);
		goto p_err;
	}

	if ((value >= FLASH_MODE_OFF) && (value <= FLASH_MODE_TORCH)) {
		s5k4ec_state->flash_mode = value;
		if (s5k4ec_state->flash_mode == FLASH_MODE_TORCH) {
			ret = sensor_4ec_apply_set(subdev,
				&regs_set.flash_start);
		} else {
			ret = sensor_4ec_apply_set(subdev,
				&regs_set.flash_end);
		}
	}

p_err:
	return ret;
}

static int sensor_4ec_s_white_balance(struct v4l2_subdev *subdev,
	int wb)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cam_info("( %d ) : E\n", wb);

	switch (wb) {
	case WHITE_BALANCE_BASE:
	case WHITE_BALANCE_AUTO:
		s5k4ec_state->white_balance = WHITE_BALANCE_AUTO;
		break;
	case WHITE_BALANCE_SUNNY:
		s5k4ec_state->white_balance = WHITE_BALANCE_SUNNY;
		break;
	case WHITE_BALANCE_CLOUDY:
		s5k4ec_state->white_balance = WHITE_BALANCE_CLOUDY;
		break;
	case WHITE_BALANCE_TUNGSTEN:
		s5k4ec_state->white_balance = WHITE_BALANCE_TUNGSTEN;
		break;
	case WHITE_BALANCE_FLUORESCENT:
		s5k4ec_state->white_balance = WHITE_BALANCE_FLUORESCENT;
		break;
	default:
		cam_err("%s: Not support.(%d)\n", __func__, wb);
		ret = -EINVAL;
		goto p_err;
	}

	ret = sensor_4ec_apply_set(subdev,
		&regs_set.white_balance[s5k4ec_state->white_balance]);
	if (ret) {
		cam_err("%s: write cmd failed\n", __func__);
		goto p_err;
	}

p_err:

	return ret;
}

static int sensor_4ec_s_effect(struct v4l2_subdev *subdev,
	int effect)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cam_info("( %d ) : E\n", effect);

	switch (effect) {
	case IMAGE_EFFECT_BASE:
	case IMAGE_EFFECT_NONE:
		s5k4ec_state->effect = IMAGE_EFFECT_NONE;
		break;
	case IMAGE_EFFECT_BNW:
		s5k4ec_state->effect = IMAGE_EFFECT_BNW;
		break;
	case IMAGE_EFFECT_SEPIA:
		s5k4ec_state->effect = IMAGE_EFFECT_SEPIA;
		break;
	case IMAGE_EFFECT_NEGATIVE:
		s5k4ec_state->effect = IMAGE_EFFECT_NEGATIVE;
		break;
	default:
		cam_err("%s: Not support.(%d)\n", __func__, effect);
		ret = -EINVAL;
		goto p_err;
	}

	ret = sensor_4ec_apply_set(subdev,
		&regs_set.effect[s5k4ec_state->effect]);
	if (ret) {
		cam_err("%s: write cmd failed\n", __func__);
		goto p_err;
	}

p_err:

	return ret;
}

static int sensor_4ec_s_iso(struct v4l2_subdev *subdev, int iso)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cam_info("( %d ) : E\n", iso);

	switch (iso) {
	case ISO_AUTO:
		s5k4ec_state->iso = ISO_AUTO;
		break;
	case ISO_50:
		s5k4ec_state->iso = ISO_50;
		break;
	case ISO_100:
		s5k4ec_state->iso = ISO_100;
		break;
	case ISO_200:
		s5k4ec_state->iso = ISO_200;
		break;
	case ISO_400:
		s5k4ec_state->iso = ISO_400;
		break;
	default:
		cam_err("%s: Not support.(%d)\n", __func__, iso);
		ret = -EINVAL;
		goto p_err;
	}

	ret = sensor_4ec_apply_set(subdev, &regs_set.iso[s5k4ec_state->iso]);
	if (ret) {
		cam_err("%s: write cmd failed\n", __func__);
		goto p_err;
	}

p_err:

	return ret;
}

static int sensor_4ec_s_contrast(struct v4l2_subdev *subdev, int contrast)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cam_info("( %d ) : E\n", contrast);

	if (contrast < CONTRAST_MINUS_2)
		contrast = CONTRAST_MINUS_2;
	else if (contrast > CONTRAST_PLUS_2)
		contrast = CONTRAST_PLUS_2;

	s5k4ec_state->contrast = CID_INDEX(contrast, CONTRAST_MIN);

	ret = sensor_4ec_apply_set(subdev,
		&regs_set.contrast[s5k4ec_state->contrast]);
	if (ret) {
		cam_err("%s: write cmd failed\n", __func__);
		goto p_err;
	}

p_err:

	return ret;
}

static int sensor_4ec_s_saturation(struct v4l2_subdev *subdev,
	int saturation)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cam_info("( %d ) : E\n", saturation);

	if (saturation < SATURATION_MINUS_2)
		saturation = SATURATION_MINUS_2;
	else if (saturation > SATURATION_PLUS_2)
		saturation = SATURATION_PLUS_2;

	s5k4ec_state->saturation = CID_INDEX(saturation,
		SATURATION_MIN);

	ret = sensor_4ec_apply_set(subdev,
		&regs_set.saturation[s5k4ec_state->saturation]);
	if (ret) {
		cam_err("%s: write cmd failed\n", __func__);
		goto p_err;
	}

p_err:

	return ret;
}

static int sensor_4ec_s_sharpness(struct v4l2_subdev *subdev,
	int sharpness)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cam_info("( %d ) : E\n", sharpness);

	if (sharpness < SHARPNESS_MINUS_2)
		sharpness = SHARPNESS_MINUS_2;
	else if (sharpness > SHARPNESS_PLUS_2)
		sharpness = SHARPNESS_PLUS_2;

	s5k4ec_state->sharpness = CID_INDEX(sharpness, SHARPNESS_MIN);

	ret = sensor_4ec_apply_set(subdev,
		&regs_set.sharpness[s5k4ec_state->sharpness]);
	if (ret) {
		cam_err("%s: write cmd failed\n", __func__);
		goto p_err;
	}

p_err:

	return ret;
}

static int sensor_4ec_s_brightness(struct v4l2_subdev *subdev,
	int brightness)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cam_info("( %d ) : E\n", brightness);

	if (brightness < EV_MINUS_4)
		brightness = EV_MINUS_4;
	else if (brightness > EV_PLUS_4)
		brightness = EV_PLUS_4;

	s5k4ec_state->ev = CID_INDEX(brightness, EV_MIN);

	ret = sensor_4ec_apply_set(subdev, &regs_set.ev[s5k4ec_state->ev]);
	if (ret) {
		cam_err("%s: write cmd failed\n", __func__);
		goto p_err;
	}

p_err:

	return ret;
}

static int sensor_4ec_s_metering(struct v4l2_subdev *subdev,
	int metering)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cam_info("( %d ) : E\n", metering);

	switch (metering) {
	case METERING_MATRIX:
	case METERING_CENTER:
	case METERING_SPOT:
		s5k4ec_state->metering = metering;
		break;
	default:
		cam_err("%s: Not support.(%d)\n", __func__, metering);
		ret = -EINVAL;
		goto p_err;
	}

	ret = sensor_4ec_apply_set(subdev,
		&regs_set.metering[s5k4ec_state->metering]);
	if (ret) {
		cam_err("%s: write cmd failed\n", __func__);
		goto p_err;
	}

p_err:

	return ret;
}

static int sensor_4ec_s_scene_mode(struct v4l2_subdev *subdev,
	int scene_mode)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cam_info("( %d ) : E\n", scene_mode);

	switch (scene_mode) {
	case SCENE_MODE_BASE:
	case SCENE_MODE_NONE:
		s5k4ec_state->scene_mode = SCENE_MODE_NONE;
		break;
	case SCENE_MODE_PORTRAIT:
		s5k4ec_state->scene_mode = SCENE_MODE_PORTRAIT;
		break;
	case SCENE_MODE_NIGHTSHOT:
		s5k4ec_state->scene_mode = SCENE_MODE_NIGHTSHOT;
		break;
	case SCENE_MODE_BACK_LIGHT:
		s5k4ec_state->scene_mode = SCENE_MODE_BACK_LIGHT;
		break;
	case SCENE_MODE_LANDSCAPE:
		s5k4ec_state->scene_mode = SCENE_MODE_LANDSCAPE;
		break;
	case SCENE_MODE_SPORTS:
		s5k4ec_state->scene_mode = SCENE_MODE_SPORTS;
		break;
	case SCENE_MODE_PARTY_INDOOR:
		s5k4ec_state->scene_mode = SCENE_MODE_PARTY_INDOOR;
		break;
	case SCENE_MODE_BEACH_SNOW:
		s5k4ec_state->scene_mode = SCENE_MODE_BEACH_SNOW;
		break;
	case SCENE_MODE_SUNSET:
		s5k4ec_state->scene_mode = SCENE_MODE_SUNSET;
		break;
	case SCENE_MODE_DUSK_DAWN:
		s5k4ec_state->scene_mode = SCENE_MODE_DUSK_DAWN;
		break;
	case SCENE_MODE_FALL_COLOR:
		s5k4ec_state->scene_mode = SCENE_MODE_FALL_COLOR;
		break;
	case SCENE_MODE_FIREWORKS:
		s5k4ec_state->scene_mode = SCENE_MODE_FIREWORKS;
		break;
	case SCENE_MODE_TEXT:
		s5k4ec_state->scene_mode = SCENE_MODE_TEXT;
		break;
	case SCENE_MODE_CANDLE_LIGHT:
		s5k4ec_state->scene_mode = SCENE_MODE_CANDLE_LIGHT;
		break;
	default:
		cam_err("%s: Not support.(%d)\n", __func__, scene_mode);
		ret = -EINVAL;
		goto p_err;
	}

	ret = sensor_4ec_apply_set(subdev,
		&regs_set.scene_mode[s5k4ec_state->scene_mode]);
	if (ret) {
		cam_err("%s: write cmd failed\n", __func__);
		goto p_err;
	}

p_err:

	return ret;
}

static bool sensor_4ec_check_focus_mode(struct v4l2_subdev *subdev,
	s32 focus_mode)
{
	bool cancel_af = false;
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);

	cancel_af = (focus_mode & V4L2_FOCUS_MODE_DEFAULT);
	focus_mode = (focus_mode & ~V4L2_FOCUS_MODE_DEFAULT);

	cam_info("cancel_af(%d) focus_mode(%d)\n", cancel_af, focus_mode);

	if ((s5k4ec_state->focus_mode != focus_mode) || cancel_af)
		return true;
	else
		return false;
}

static int sensor_4ec_s_focus_mode(struct v4l2_subdev *subdev,
	s32 focus_mode)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);

	cam_info("( %d ) E\n", focus_mode);

	mutex_lock(&s5k4ec_state->af_lock);

	if (focus_mode == V4L2_FOCUS_MODE_AUTO) {
		ret = sensor_4ec_apply_set(subdev,
			&regs_set.af_normal_mode_1);
		msleep(100);
		ret |= sensor_4ec_apply_set(subdev,
			&regs_set.af_normal_mode_2);
		msleep(100);
		ret |= sensor_4ec_apply_set(subdev,
			&regs_set.af_normal_mode_3);
		msleep(100);
	} else if (focus_mode == V4L2_FOCUS_MODE_MACRO) {
		ret = sensor_4ec_apply_set(subdev,
			&regs_set.af_macro_mode_1);
		msleep(100);
		ret |= sensor_4ec_apply_set(subdev,
			&regs_set.af_macro_mode_2);
		msleep(100);
		ret |= sensor_4ec_apply_set(subdev,
			&regs_set.af_macro_mode_3);
		msleep(100);
	} else {
		cam_warn("Not support.(%d)\n", focus_mode);
		ret = -EINVAL;
		goto p_err;
	}

	ret |= sensor_4ec_apply_set(subdev, &regs_set.continuous_af_on);

p_err:
	mutex_unlock(&s5k4ec_state->af_lock);

	return ret;
}

static int sensor_4ec_ae_lock(struct v4l2_subdev *subdev, int ctrl)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cam_info("( %d ) : E\n", ctrl);

	if (s5k4ec_state->ae_lock == ctrl) {
		cam_info("same ae_lock mode( %d ). skip to set\n", ctrl);
		return 0;
	}

	switch (ctrl) {
	case AE_UNLOCK:
	case AE_LOCK:
		s5k4ec_state->ae_lock = ctrl;
		break;
	default:
		cam_err("%s: Not support.(%d)\n", __func__, ctrl);
		ret = -EINVAL;
		goto p_err;
	}

	ret = sensor_4ec_apply_set(subdev,
		&regs_set.ae_lockunlock[s5k4ec_state->ae_lock]);
	if (ret) {
		cam_err("%s: write cmd failed\n", __func__);
		goto p_err;
	}

p_err:
	return ret;
}

static int sensor_4ec_awb_lock(struct v4l2_subdev *subdev, int ctrl)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cam_info("( %d ) : E\n", ctrl);

	if (s5k4ec_state->awb_lock == ctrl) {
		cam_info("same awb_lock mode( %d ). skip to set\n", ctrl);
		return 0;
	}

	if (s5k4ec_state->white_balance != WHITE_BALANCE_AUTO) {
		cam_info("cannot lock/unlock awb on MWB mode\n");
		return 0;
	}

	switch (ctrl) {
	case AWB_UNLOCK:
	case AWB_LOCK:
		s5k4ec_state->awb_lock = ctrl;
		break;
	default:
		cam_err("Not support.(%d)\n", ctrl);
		ret = -EINVAL;
		goto p_err;
	}

	ret = sensor_4ec_apply_set(subdev,
		&regs_set.awb_lockunlock[s5k4ec_state->awb_lock]);
	if (ret) {
		cam_err("write cmd failed\n");
		goto p_err;
	}

p_err:
	return ret;
}

static int sensor_4ec_ae_awb_lock(struct v4l2_subdev *subdev,
	int ae_lock, int awb_lock)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cam_info("( %d, %d ) : E\n", ae_lock, awb_lock);

	ret = sensor_4ec_ae_lock(subdev, ae_lock);
	if (ret) {
		cam_err("ae_lock(%d) failed\n", ae_lock);
		goto p_err;
	}

	ret = sensor_4ec_awb_lock(subdev, awb_lock);
	if (ret) {
		cam_err("awbe_lock(%d) failed\n", awb_lock);
		goto p_err;
	}

p_err:
	return ret;
}

static int sensor_4ec_read_sensor_version(struct v4l2_subdev *subdev,
	u16 *sensor_version)
{
	int ret = 0;
	struct i2c_client *client = to_client(subdev);
	struct s5k4ec_state *s5k4ec_state = to_state(subdev);

	BUG_ON(!client);
	BUG_ON(!s5k4ec_state);

	cam_dbg("E\n");

	mutex_lock(&s5k4ec_state->i2c_lock);

	ret = sensor_4ec_i2c_write16(client, 0xFCFC, 0xD000);
	ret |= sensor_4ec_i2c_write16(client, 0x0028, 0xD000);
	ret |= sensor_4ec_i2c_write16(client, 0x002A, 0x0012);
	ret |= sensor_4ec_i2c_write16(client, 0x0F12, 0x0001);
	ret |= sensor_4ec_i2c_write16(client, 0x002A, 0x007A);
	ret |= sensor_4ec_i2c_write16(client, 0x0F12, 0x0000);
	ret |= sensor_4ec_i2c_write16(client, 0x002A, 0xA000);
	ret |= sensor_4ec_i2c_write16(client, 0x0F12, 0x0004);
	ret |= sensor_4ec_i2c_write16(client, 0x002A, 0xA002);
	ret |= sensor_4ec_i2c_write16(client, 0x0F12, 0x000F);
	ret |= sensor_4ec_i2c_write16(client, 0x002A, 0xA000);
	ret |= sensor_4ec_i2c_write16(client, 0x0F12, 0x0001);

	usleep_range(200, 201);

	ret |= sensor_4ec_i2c_write16(client, 0x002C, 0xD000);
	ret |= sensor_4ec_i2c_write16(client, 0x002E, 0xA044);
	ret |= sensor_4ec_i2c_read16(client, 0x0F12, sensor_version);

	mutex_unlock(&s5k4ec_state->i2c_lock);

	cam_dbg("X\n");
	return ret;
}

static int sensor_4ec_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	struct i2c_client *client = to_client(subdev);
	struct s5k4ec_state *s5k4ec_state = to_state(subdev);
	u16 pid = 0;
	u16 revision = 0;

	BUG_ON(!subdev);

	cam_info("start\n");

	if (s5k4ec_state->sensor_version == 0) {
		ret = sensor_4ec_read_sensor_version(subdev,
			&s5k4ec_state->sensor_version);
		if (ret < 0) {
			cam_err("read_sensor_version failed\n");
			goto p_err;
		}

		s5k4ec_state->setfile_index = SETFILE_INDEX_4EC_DEFAULT;
	}

	cam_info("Sensor Version 0x%04X\n", s5k4ec_state->sensor_version);
	cam_dbg("setfile_index %d\n", s5k4ec_state->setfile_index);

#ifdef CONFIG_LOAD_FILE
	ret = sensor_4ec_regs_table_init();
	if (ret < 0) {
		cam_err("[CONFIG_LOAD_FILE] init fail\n");
		goto p_err;
	}
#endif

	/* init member */
	s5k4ec_state->flash_mode = FLASH_MODE_OFF;
	s5k4ec_state->flash_status = FLASH_STATUS_OFF;
	s5k4ec_state->af_status = AF_NONE;
	s5k4ec_state->af_result = AF_RESULT_NONE;
	s5k4ec_state->sensor_af_in_low_light_mode = false;
	s5k4ec_state->preview.width = 0;
	s5k4ec_state->preview.height = 0;

	ret = sensor_4ec_read_reg(subdev,
		&regs_set.get_pid, &pid, 1);
	if (ret < 0) {
		cam_err("read PID failed\n");
		goto p_err;
	}

	cam_info("pid 0x%04X\n", pid);

	ret = sensor_4ec_read_reg(subdev,
		&regs_set.get_revision, &revision, 1);
	if (ret < 0) {
		cam_err("read Version failed\n");
		goto p_err;
	}

	cam_info("revision 0x%04X\n", revision);

	msleep(20);

	ret = sensor_4ec_apply_set(subdev, &regs_set.init_reg_1);
	if (ret < 0) {
		cam_err("init_reg_1 failed\n");
		goto p_err;
	}

#ifdef CONFIG_LOAD_FILE
	ret = sensor_4ec_apply_set(subdev, &regs_set.init_reg_2);
	if (ret < 0) {
		cam_err("init_reg_2 failed\n");
		goto p_err;
	}
#else
	{
		struct s5k4ec_regset *reg_tbl;
		struct s5k4ec_regset *regset;
		struct s5k4ec_regset *end_regset;
		u8 *regset_data;
		u8 *dst_ptr;
		const u32 *end_src_ptr;
		bool flag_copied;
		u32 set_idx = s5k4ec_state->setfile_index;
		int init_reg_2_array_size =
			regs_set.init_reg_2.array_size[set_idx];
		int init_reg_2_size = init_reg_2_array_size * sizeof(u32);
		const u32 *src_ptr = regs_set.init_reg_2.reg[set_idx];
		u32 src_value;
		int err;

		cam_info("start reg 2 bursts\n");

		regset_data = vmalloc(init_reg_2_size);
		if (regset_data == NULL)
			return -ENOMEM;
		reg_tbl =
			vmalloc((u32)sizeof(struct s5k4ec_regset)
				* init_reg_2_size);
		if (reg_tbl == NULL) {
			vfree(regset_data);
			return -ENOMEM;
		}

		dst_ptr = regset_data;
		regset = reg_tbl;
		end_src_ptr =
			&regs_set.init_reg_2.reg
			[set_idx][init_reg_2_array_size];

		src_value = *src_ptr++;
		while (src_ptr <= end_src_ptr) {
			/* initial value for a regset */
			regset->data = dst_ptr;
			flag_copied = false;
			*dst_ptr++ = src_value >> 24;
			*dst_ptr++ = src_value >> 16;
			*dst_ptr++ = src_value >> 8;
			*dst_ptr++ = src_value;

			/* check subsequent values for a data flag (starts with
			   0x0F12) or something else */
			do {
				src_value = *src_ptr++;
				if ((src_value & 0xFFFF0000) != 0x0F120000) {
					/* src_value is start of next regset */
					regset->size = dst_ptr - regset->data;
					regset++;
					break;
				}
				/* copy the 0x0F12 flag if not done already */
				if (!flag_copied) {
					*dst_ptr++ = src_value >> 24;
					*dst_ptr++ = src_value >> 16;
					flag_copied = true;
				}
				/* copy the data part */
				*dst_ptr++ = src_value >> 8;
				*dst_ptr++ = src_value;
			} while (src_ptr < end_src_ptr);
		}
		cam_dbg("finished creating table\n");

		end_regset = regset;
		cam_dbg("1st regset = %p, last regset = %p, count = %ld\n",
			reg_tbl,
			regset, end_regset - reg_tbl);
		cam_dbg("regset_data = %p, end = %p, dst_ptr = %p\n",
			regset_data,
			regset_data + (init_reg_2_size * sizeof(u32)),
			dst_ptr);

		regset = reg_tbl;
		cam_dbg("start writing init reg 2 bursts\n");
		do {
			if (regset->size > 4) {
				/* write the address packet */
				err = sensor_4ec_i2c_write(client,
					regset->data, 4);
				if (err)
					break;
				/* write the data in a burst */
				err = sensor_4ec_i2c_write(client,
					regset->data+4,
					regset->size-4);

			} else
				err = sensor_4ec_i2c_write(client,
					regset->data,
					regset->size);
			if (err)
				break;
			regset++;
		} while (regset < end_regset);

		cam_info("finished writing init reg 2 bursts\n");

		vfree(regset_data);
		vfree(reg_tbl);

		if (err) {
			cam_err("write cmd failed\n");
			goto p_err;
		}
	}
#endif

	s5k4ec_state->sensor_mode = SENSOR_CAMERA;
	s5k4ec_state->contrast = CONTRAST_DEFAULT;
	s5k4ec_state->effect = IMAGE_EFFECT_NONE;
	s5k4ec_state->ev = CID_INDEX(EV_DEFAULT, EV_MIN);
	s5k4ec_state->flash_mode = FLASH_MODE_OFF;
	s5k4ec_state->focus_mode = V4L2_FOCUS_MODE_AUTO;
	s5k4ec_state->iso = ISO_AUTO;
	s5k4ec_state->metering = METERING_CENTER;
	s5k4ec_state->saturation = SATURATION_DEFAULT;
	s5k4ec_state->scene_mode = SCENE_MODE_NONE;
	s5k4ec_state->sharpness = SHARPNESS_DEFAULT;
	s5k4ec_state->white_balance = WHITE_BALANCE_AUTO;
	s5k4ec_state->ae_lock = AE_UNLOCK;
	s5k4ec_state->awb_lock = AWB_UNLOCK;
	s5k4ec_state->user_ae_lock = AE_UNLOCK;
	s5k4ec_state->user_awb_lock = AWB_UNLOCK;
	s5k4ec_state->anti_banding = S5K4EC_FLICKER_50HZ_AUTO;
	s5k4ec_state->light_level = 0;

	ret = sensor_4ec_apply_set(subdev,
		&regs_set.effect[s5k4ec_state->effect]);
	ret |= sensor_4ec_apply_set(subdev,
		&regs_set.ev[s5k4ec_state->ev]);
	ret |= sensor_4ec_apply_set(subdev,
		&regs_set.iso[s5k4ec_state->iso]);
	ret |= sensor_4ec_apply_set(subdev,
		&regs_set.metering[s5k4ec_state->metering]);
	ret |= sensor_4ec_apply_set(subdev,
		&regs_set.scene_mode[s5k4ec_state->scene_mode]);
	ret |= sensor_4ec_apply_set(subdev,
		&regs_set.ae_lockunlock[s5k4ec_state->ae_lock]);
	ret |= sensor_4ec_apply_set(subdev,
		&regs_set.awb_lockunlock[s5k4ec_state->awb_lock]);
	ret |= sensor_4ec_apply_set(subdev,
		&regs_set.white_balance[s5k4ec_state->white_balance]);
	ret |= sensor_4ec_apply_set(subdev,
		&regs_set.flash_init);
	if (ret) {
		cam_err("write cmd failed\n");
		goto p_err;
	}

	ret = (schedule_work(&s5k4ec_state->set_focus_mode_work) == 1)
		? 0 : -EINVAL;
	if (ret < 0)
		cam_err("set focus mode fail.\n");

	flush_work(&s5k4ec_state->set_focus_mode_work);

	cam_info("end\n");

p_err:
	return ret;
}

static int sensor_4ec_set_size(struct v4l2_subdev *subdev)
{
	int ret = 0;
	const struct s5k4ec_framesize *size;
	struct s5k4ec_state *s5k4ec_state;

	cam_dbg("E\n");

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	size = s5k4ec_state->stream_size;

	if (s5k4ec_state->mode == S5K4EC_OPRMODE_IMAGE) {
		ret = sensor_4ec_apply_set(subdev,
			&regs_set.capture_size[size->index]);
		if (ret) {
			cam_err("write cmd failed\n");
			goto p_err;
		}
	} else {
		ret = sensor_4ec_apply_set(subdev,
			&regs_set.preview_size[size->index]);
		if (ret) {
			cam_err("write cmd failed\n");
			goto p_err;
		}
		s5k4ec_state->preview.width = size->width;
		s5k4ec_state->preview.height = size->height;
	}

	cam_info("stream size(%d) %d x %d\n",
		size->index, size->width, size->height);

p_err:
	return ret;
}

static int sensor_4ec_s_format(struct v4l2_subdev *subdev,
	struct v4l2_mbus_framefmt *fmt)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;
	const struct s5k4ec_framesize *size;
	u32 i;

	BUG_ON(!subdev);
	BUG_ON(!fmt);

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cam_info("%dx%d (0x%x)\n", fmt->width, fmt->height, fmt->code);

	s5k4ec_state->fmt.width = fmt->width;
	s5k4ec_state->fmt.height = fmt->height;
	s5k4ec_state->fmt.code = fmt->code;

	size = NULL;

	if (fmt->width > 1920 && fmt->height > 1080)
		s5k4ec_state->mode = S5K4EC_OPRMODE_IMAGE;
	else
		s5k4ec_state->mode = S5K4EC_OPRMODE_VIDEO;

	if (s5k4ec_state->mode == S5K4EC_OPRMODE_IMAGE) {
		cam_info("for capture\n");
		for (i = 0; i < ARRAY_SIZE(capture_size_list); ++i) {
			if ((capture_size_list[i].width ==
				s5k4ec_state->fmt.width) &&
				(capture_size_list[i].height ==
				s5k4ec_state->fmt.height)) {
				size = &capture_size_list[i];
				break;
			}
		}

		if (!size) {
			cam_err("the capture size(%d x %d) is not supported",
				s5k4ec_state->fmt.width,
				s5k4ec_state->fmt.height);
			ret = -EINVAL;
			goto p_err;
		}
	} else {
		cam_info("for preview\n");
		for (i = 0; i < ARRAY_SIZE(preview_size_list); ++i) {
			if ((preview_size_list[i].width ==
			s5k4ec_state->fmt.width) &&
				(preview_size_list[i].height ==
				s5k4ec_state->fmt.height)) {
				size = &preview_size_list[i];
				break;
			}
		}

		if (!size) {
			cam_err("the preview size(%d x %d) is not supported",
				s5k4ec_state->fmt.width,
				s5k4ec_state->fmt.height);
			ret = -EINVAL;
			goto p_err;
		}
	}

	s5k4ec_state->stream_size = size;

p_err:
	return ret;
}

static int sensor_4ec_s_stream(struct v4l2_subdev *subdev,
	int enable)
{
	int ret = 0;

	cam_info("%d\n", enable);

	if (enable) {
		ret = sensor_4ec_init(subdev, 1);
		if (ret) {
			cam_err("stream_on is fail(%d)", ret);
			goto p_err;
		}

		ret = sensor_4ec_set_framerate(subdev);
		if (ret) {
			cam_err("stream_on is fail(%d)", ret);
			goto p_err;
		}

		ret = sensor_4ec_set_size(subdev);
		if (ret) {
			cam_err("stream_on is fail(%d)", ret);
			goto p_err;
		}

		ret = sensor_4ec_stream_on(subdev);
		if (ret) {
			cam_err("stream_on is fail(%d)", ret);
			goto p_err;
		}
	} else {
		ret = sensor_4ec_stream_off(subdev);
		if (ret) {
			cam_err("stream_off is fail(%d)", ret);
			goto p_err;
		}
	}

p_err:
	return ret;
}

static int sensor_4ec_s_param(struct v4l2_subdev *subdev,
	struct v4l2_streamparm *param)
{
	int ret = 0;
	struct v4l2_captureparm *cp;
	struct v4l2_fract *tpf;
	u64 framerate;

	BUG_ON(!subdev);
	BUG_ON(!param);

	cp = &param->parm.capture;
	tpf = &cp->timeperframe;

	if (!tpf->numerator) {
		cam_err("numerator is 0");
		ret = -EINVAL;
		goto p_err;
	}

	framerate = tpf->denominator;

	ret = sensor_4ec_s_duration(subdev, framerate);
	if (ret) {
		cam_err("s_duration is fail(%d)", ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int sensor_4ec_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u16 read_value = 0;
	u32 poll_time_ms = 0;
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (s5k4ec_state->mode == S5K4EC_OPRMODE_IMAGE) {
		/* Do Single AF */
		sensor_4ec_auto_focus_start(&s5k4ec_state->subdev);

		/* Do Flash work */
		cam_dbg("light_level = 0x%08x\n",
			s5k4ec_state->light_level);
		s5k4ec_state->flash_fire = false;

		switch (s5k4ec_state->flash_mode) {
		case FLASH_MODE_AUTO:
			if (!IS_LOWLIGHT(s5k4ec_state->light_level)) {
				/* light level bright enough
				 * that we don't need flash
				 */
				break;
			}
			/* fall through to flash start */
		case FLASH_MODE_ON:
			ret = sensor_4ec_main_flash_start(subdev);
			if (ret) {
				cam_err("main_flash_start failed\n");
				goto p_err;
			}

			s5k4ec_state->flash_fire = true;
			break;
		default:
			break;
		}

		/* Start Capture */
		ret = sensor_4ec_apply_set(subdev, &regs_set.capture_start);
		if (ret) {
			cam_err("write cmd failed\n");
			goto p_err;
		}

		msleep(50);
		poll_time_ms = 50;
		do {
			sensor_4ec_read_reg(subdev,
				&regs_set.get_modechange_check, &read_value, 1);
			if (read_value == 0x0100)
				break;
			msleep(POLL_TIME_MS);
			poll_time_ms += POLL_TIME_MS;
		} while (poll_time_ms < CAPTURE_POLL_TIME_MS);

		cam_info("capture done check finished after %d ms\n",
			poll_time_ms);
	} else {
		ret = sensor_4ec_apply_set(subdev, &regs_set.init_reg_3);
		if (ret) {
			cam_err("write cmd failed\n");
			goto p_err;
		}

		/* Checking preveiw start (handshaking) */
		do {
			sensor_4ec_read_reg(subdev,
				&regs_set.get_preview_status, &read_value, 1);
			if (read_value == 0x00)
				break;
			cam_dbg("value = %d\n", read_value);
			msleep(POLL_TIME_MS);
			poll_time_ms += POLL_TIME_MS;
		} while (poll_time_ms < CAPTURE_POLL_TIME_MS);

		cam_info("preview on done check finished after %d ms\n",
			poll_time_ms);
	}

	cam_info("stream on end\n");

p_err:
	return ret;
}

static int sensor_4ec_stream_off(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u16 read_value = 0;
	u32 poll_time_ms = 0;
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	flush_work(&s5k4ec_state->set_focus_mode_work);
	flush_work(&s5k4ec_state->af_work);
	flush_work(&s5k4ec_state->af_stop_work);

	if (s5k4ec_state->mode == S5K4EC_OPRMODE_IMAGE) {
		u16 size[2] = {0, };

		if (s5k4ec_state->flash_status == FLASH_STATUS_MAIN_ON) {
			ret = sensor_4ec_main_flash_stop(subdev);
			if (ret) {
				cam_err("main_flash_stop failed\n");
				goto p_err;
			}
		}

		sensor_4ec_read_addr(subdev, 0x1d02, size, 2);
		cam_info("Captured image width = %d, height = %d\n",
			size[0], size[1]);

		/* get exif information */
		ret = sensor_4ec_get_exif(subdev);
		if (ret) {
			cam_err("get exif failed\n");
			goto p_err;
		}
	}

	ret = sensor_4ec_apply_set(subdev, &regs_set.init_reg_4);
	if (ret) {
		cam_err("write cmd failed\n");
		goto p_err;
	}

	/* Checking preveiw start (handshaking) */
	do {
		sensor_4ec_read_reg(subdev,
			&regs_set.get_preview_status, &read_value, 1);
		if (read_value == 0x00)
			break;
		cam_dbg("value = %d\n", read_value);
		msleep(POLL_TIME_MS);
		poll_time_ms += POLL_TIME_MS;
	} while (poll_time_ms < CAPTURE_POLL_TIME_MS);

	cam_info("preview off done check finished after %d ms\n",
		poll_time_ms);

	s5k4ec_state->fps = FRAME_RATE_30; /* default frame rate */

	cam_info("stream off\n");

p_err:
	return ret;
}

static int sensor_4ec_get_exif_exptime(struct v4l2_subdev *subdev,
	u32 *exp_time)
{
	int err = 0;
	u16 exptime[2] = {0, };
	u32 val;

	err = sensor_4ec_read_reg(subdev,
		&regs_set.get_exptime, exptime, 2);
	if (err) {
		cam_err("read exptime failed\n");
		goto out;
	}

	cam_info("exp_time_den value(LSB) = %08X, (MSB) = %08X\n",
		exptime[0], exptime[1]);

	val = ((exptime[1] << 16) | (exptime[0] & 0xffff));
	*exp_time = (val * 1000) / 400;

	cam_info("exp_time_den value = %08X\n", *exp_time);

out:
	return err;
}

static int sensor_4ec_get_exif_iso(struct v4l2_subdev *subdev,
	u16 *iso)
{
	int err = 0;
	u16 gain_value = 0;
	u16 gain[2] = {0, };

	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		goto out;
	}

	err = sensor_4ec_read_reg(subdev, &regs_set.get_iso, gain, 2);
	if (err) {
		cam_err("read iso failed\n");
		goto out;
	}

	cam_info("a_gain value = %08X, d_gain value = %08X\n",
		gain[0], gain[1]);

	gain_value = ((gain[0] * gain[1]) / 0x100) / 2;
	cam_info("iso: gain_value=%08X\n", gain_value);

	switch (s5k4ec_state->iso) {
	case ISO_AUTO:
		if (gain_value < 256) { /*0x100*/
			*iso = 50;
		} else if (gain_value < 512) { /*0x1ff*/
			*iso = 100;
		} else if (gain_value < 896) { /*0x37f*/
			*iso = 200;
		} else {
			*iso = 400;
		}
		break;
	case ISO_50:
		*iso = 100; /* map to 100 */
		break;
	case ISO_100:
		*iso = 100;
		break;
	case ISO_200:
		*iso = 200;
		break;
	case ISO_400:
		*iso = 400;
		break;
	default:
		cam_err("Not support. id(%d)\n", s5k4ec_state->iso);
		break;
	}

	cam_info("ISO=%d\n", *iso);
out:
	return err;
}

static void sensor_4ec_get_exif_flash(struct v4l2_subdev *subdev,
					u16 *flash)
{
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);

	if (s5k4ec_state->flash_fire) {
		*flash = EXIF_FLASH_FIRED;
		s5k4ec_state->flash_fire = false;
	} else {
		*flash = 0;
	}

	cam_info("flash(%d)\n", *flash);
}

static int sensor_4ec_get_exif(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u32 exp_time = 0;
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);

	/* exposure time */
	s5k4ec_state->exif.exp_time_num = 1;
	s5k4ec_state->exif.exp_time_den = 0;
	ret = sensor_4ec_get_exif_exptime(subdev, &exp_time);
	if (exp_time)
		s5k4ec_state->exif.exp_time_den = 1000 * 1000 / exp_time;
	else
		cam_err("EXIF: error, exposure time 0. %d\n", ret);

	/* iso */
	s5k4ec_state->exif.iso = 0;
	ret |= sensor_4ec_get_exif_iso(subdev, &s5k4ec_state->exif.iso);

	/* flash */
	sensor_4ec_get_exif_flash(subdev, &s5k4ec_state->exif.flash);

	return ret;
}

static u32 sensor_4ec_get_light_level(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u16 light[2] = {0, };
	u32 light_sum = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = sensor_4ec_read_reg(subdev,
		&regs_set.get_light_level, light, 2);
	if (ret) {
		cam_err("read cmd failed\n");
		goto p_err;
	}

	light_sum = (light[1] << 16) | light[0];

	cam_info("light value = 0x%08x\n", light_sum);

p_err:
	return light_sum;
}

static u32 sensor_4ec_get_frame_duration(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u16 frame_duration = 0;
	u16 frame_duration_ms = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = sensor_4ec_read_reg(subdev,
		&regs_set.get_frame_duration, &frame_duration, 1);
	if (ret) {
		cam_err("read cmd failed\n");
		goto p_err;
	}

	frame_duration_ms = frame_duration / 400;

	cam_info("frame_duration_ms = %d ms\n", frame_duration_ms);

p_err:
	return frame_duration_ms;
}

static int sensor_4ec_pre_flash_start(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;
	int count;
	u16 read_value;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/* AE / AWB Unlock */
	ret = sensor_4ec_ae_awb_lock(subdev, AE_UNLOCK, AWB_UNLOCK);
	if (ret) {
		cam_err("AE/AWB unlock failed\n");
		goto p_err;
	}

	/* Fast AE On */
	ret = sensor_4ec_apply_set(subdev, &regs_set.fast_ae_on);
	if (ret) {
		cam_err("write cmd failed\n");
		goto p_err;
	}

	/* Pre Flash Start */
	ret = sensor_4ec_apply_set(subdev, &regs_set.af_assist_flash_start);
	if (ret) {
		cam_err("write cmd failed\n");
		goto p_err;
	}

	/* Pre Flash On */
	cam_info("Preflash On\n");

	s5k4ec_state->flash_status = FLASH_STATUS_PRE_ON;

	/* delay 400ms (SLSI value) and then poll to see if AE is stable.
	 * once it is stable, lock it and then return to do AF
	 */
	msleep(400);

	/* enter read mode */
	for (count = 0; count < AE_STABLE_SEARCH_COUNT; count++) {
		if (s5k4ec_state->af_status == AF_CANCEL)
			break;
		ret = sensor_4ec_read_reg(subdev,
			&regs_set.get_ae_stable_status, &read_value, 1);
		if (ret) {
			cam_err("write cmd failed\n");
			goto p_err;
		}

		cam_info("ae stable status = 0x%04x\n", read_value);
		if (read_value == 0x1)
			break;
		msleep(NORMAL_FRAME_DELAY_MS);
	}

p_err:
	return ret;
}

static int sensor_4ec_pre_flash_stop(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/* Fast AE Off */
	ret = sensor_4ec_apply_set(subdev, &regs_set.fast_ae_off);
	if (ret) {
		cam_err("write cmd failed\n");
		goto p_err;
	}

	/* AE / AWB Unlock */
	ret = sensor_4ec_ae_lock(subdev, AE_UNLOCK);
	if (ret) {
		cam_err("AE unlock failed\n");
		goto p_err;
	}

	ret = sensor_4ec_awb_lock(subdev, AWB_UNLOCK);
	if (ret) {
		cam_err("AWB unlock failed\n");
		goto p_err;
	}

	/* Pre Flash End */
	ret = sensor_4ec_apply_set(subdev, &regs_set.af_assist_flash_end);
	if (ret) {
		cam_err("write cmd failed\n");
		goto p_err;
	}

	/* Pre Flash Off */
	cam_info("Preflash Off\n");

	s5k4ec_state->flash_status = FLASH_STATUS_OFF;

p_err:
	return ret;
}

static int sensor_4ec_main_flash_start(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/* Main Flash On */
	cam_info("Main Flash On\n");

	/* Main Flash Start */
	ret = sensor_4ec_apply_set(subdev, &regs_set.flash_start);
	if (ret) {
		cam_err("write cmd failed\n");
		goto p_err;
	}

	s5k4ec_state->flash_status = FLASH_STATUS_MAIN_ON;

p_err:
	return ret;
}

static int sensor_4ec_main_flash_stop(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/* Main Flash End */
	ret = sensor_4ec_apply_set(subdev, &regs_set.flash_end);
	if (ret) {
		cam_err("write cmd failed\n");
		goto p_err;
	}

	/* Main Flash Off */
	cam_info("Main Flash Off\n");

	s5k4ec_state->flash_status = FLASH_STATUS_OFF;

p_err:
	return ret;
}

static int sensor_4ec_auto_focus_start(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		return -EINVAL;
	}

	mutex_lock(&s5k4ec_state->af_lock);

	/* Check low light*/
	s5k4ec_state->light_level = sensor_4ec_get_light_level(subdev);

	switch (s5k4ec_state->flash_mode) {
	case FLASH_MODE_AUTO:
		if (!IS_LOWLIGHT(s5k4ec_state->light_level)) {
			/* flash not needed */
			break;
		}
		/* fall through to turn on flash for AF assist */
	case FLASH_MODE_ON:
		ret = sensor_4ec_pre_flash_start(subdev);
		if (ret) {
			cam_err("pre flash start failed\n");
			goto p_err;
		}

		if (s5k4ec_state->af_status == AF_CANCEL) {
			ret = sensor_4ec_pre_flash_stop(subdev);
			if (ret) {
				cam_err("pre flash stop failed\n");
				goto p_err;
			}
			mutex_unlock(&s5k4ec_state->af_lock);
			return 0;
		}
		break;
	case FLASH_MODE_OFF:
		break;
	default:
		cam_info("Unknown Flash mode 0x%x\n",
			s5k4ec_state->flash_mode);
		break;
	}

	/* AE / AWB Lock */
	ret = sensor_4ec_ae_awb_lock(subdev, AE_LOCK, AWB_LOCK);
	if (ret) {
		cam_err("AE/AWB lock failed\n");
		goto p_err;
	}

	if (!IS_LOWLIGHT(s5k4ec_state->light_level)) {
		if (s5k4ec_state->sensor_af_in_low_light_mode) {
			s5k4ec_state->sensor_af_in_low_light_mode = false;
			ret = sensor_4ec_apply_set(subdev,
				&regs_set.af_low_light_mode_off);
			if (ret) {
				cam_err("write cmd failed\n");
				goto p_err;
			}
		}
	} else {
		if (!s5k4ec_state->sensor_af_in_low_light_mode) {
			s5k4ec_state->sensor_af_in_low_light_mode = true;
			ret = sensor_4ec_apply_set(subdev,
				&regs_set.af_low_light_mode_on);
			if (ret) {
				cam_err("write cmd failed\n");
				goto p_err;
			}
		}
	}

	/* Start AF */
	s5k4ec_state->af_status = AF_START;
	ret = sensor_4ec_apply_set(subdev, &regs_set.single_af_start);
	if (ret) {
		cam_err("write cmd failed\n");
		goto p_err;
	}

	INIT_COMPLETION(s5k4ec_state->af_complete);

	cam_info("AF started.\n");

	ret = sensor_4ec_auto_focus_proc(subdev);
	if (ret)
		cam_err("auto_focus_proc failed\n");

p_err:
	mutex_unlock(&s5k4ec_state->af_lock);
	return ret;
}

static int sensor_4ec_auto_focus_stop(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		return -EINVAL;
	}

	if (s5k4ec_state->af_status != AF_START) {
		/* To do */
		/* restore focus mode */
		s5k4ec_state->af_status = AF_NONE;
		s5k4ec_state->af_result = AF_RESULT_CANCELLED;
		return 0;
	}

	/* Stop AF */
	s5k4ec_state->af_status = AF_CANCEL;
	s5k4ec_state->af_result = AF_RESULT_CANCELLED;

	/* AE / AWB Unock */
	ret = sensor_4ec_ae_awb_lock(subdev, AE_UNLOCK, AWB_UNLOCK);
	if (ret) {
		cam_err("AE / AWB Unock failed\n");
		goto p_err;
	}

	ret = sensor_4ec_apply_set(subdev, &regs_set.single_af_off_1);
	if (ret) {
		cam_err("write cmd failed\n");
		goto p_err;
	}

	/* wait until the other thread has completed
	 * aborting the auto focus and restored state
	 */
	cam_info("wait AF cancel done start\n");
	wait_for_completion(&s5k4ec_state->af_complete);
	cam_info("wait AF cancel done finished\n");

	cam_info("AF stop.\n");

p_err:
	return ret;
}

static int sensor_4ec_auto_focus_proc(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;
	u16 read_value;
	int count = 0;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/* Get 1st AF result*/
	msleep(NORMAL_FRAME_DELAY_MS * 2);

	for (count = 0; count < FIRST_AF_SEARCH_COUNT; count++) {
		if (s5k4ec_state->af_status == AF_CANCEL) {
			cam_info("AF is cancelled while doing\n");
			s5k4ec_state->af_result = AF_RESULT_FAILED;
			goto check_flash;
		}
		sensor_4ec_read_reg(subdev,
			&regs_set.get_1st_af_search_status, &read_value, 1);
		cam_dbg("%s: 1st i2c_read --- read_value == 0x%x\n",
			__func__, read_value);

		/* check for success and failure cases.  0x1 is
		 * auto focus still in progress.  0x2 is success.
		 * 0x0,0x3,0x4,0x6 are all failures cases
		 * 0x8 is to receive new AF command during AF execution
		 */
		if ((read_value != 0x01) && (read_value != 0x08))
			break;
		msleep(50);
	}

	if ((count >= FIRST_AF_SEARCH_COUNT) || (read_value != 0x02)) {
		cam_info("1st scan timed out or failed, read_value=%d\n",
			read_value);

		/* we need a time to move the lens to default position */
		msleep(350);

		s5k4ec_state->af_result  = AF_RESULT_FAILED;
		goto check_flash;
	}

	cam_info("2nd AF search\n");

	/* delay 1 frame time before checking for 2nd AF completion */
	msleep(NORMAL_FRAME_DELAY_MS);

	for (count = 0; count < SECOND_AF_SEARCH_COUNT; count++) {
		if (s5k4ec_state->af_status == AF_CANCEL) {
			cam_info("AF is cancelled while doing\n");
			s5k4ec_state->af_result  = AF_RESULT_FAILED;
			goto check_flash;
		}
		sensor_4ec_read_reg(subdev,
			&regs_set.get_2nd_af_search_status, &read_value, 1);
		cam_dbg("%s: 2nd i2c_read --- read_value == 0x%x\n",
			__func__, read_value);

		/* Check result value. 0x0 means finished. */
		if (read_value == 0x0)
			break;

		msleep(50);
	}

	if (count >= SECOND_AF_SEARCH_COUNT) {
		cam_info("2nd scan timed out\n");
		s5k4ec_state->af_result  = AF_RESULT_FAILED;
		goto check_flash;
	}

	cam_info("AF is success\n");
	s5k4ec_state->af_result  = AF_RESULT_SUCCESS;

check_flash:

	if (s5k4ec_state->flash_status == FLASH_STATUS_PRE_ON) {
		/* Delay 1 frame time before turning off preflash
		* to prevent black screen of preview on capturing image.
		 */
		msleep(NORMAL_FRAME_DELAY_MS);
		ret = sensor_4ec_pre_flash_stop(subdev);
		if (ret)
			cam_err("pre flash stop failed\n");
	} else {
		/* AE / AWB Unlock */
		if (s5k4ec_state->user_ae_lock == AE_UNLOCK) {
			ret = sensor_4ec_ae_lock(subdev, AE_UNLOCK);
			if (ret)
				cam_err("AE unlock failed\n");
		}

		if (s5k4ec_state->user_awb_lock == AWB_UNLOCK) {
			ret = sensor_4ec_awb_lock(subdev, AWB_UNLOCK);
			if (ret)
				cam_err("AWB unlock failed\n");
		}
	}

	cam_info("single AF finished\n");

	s5k4ec_state->af_status = AF_NONE;
	complete(&s5k4ec_state->af_complete);

p_err:
	return ret;
}

static int sensor_4ec_auto_focus_softlanding(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;
	u32 frame_delay, delay = 0;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		return -EINVAL;
	}

	/* Do soft landing */
	ret = sensor_4ec_apply_set(subdev, &regs_set.af_normal_mode_1);
	if (ret) {
		cam_err("write cmd failed\n");
		goto p_err;
	}

	ret = sensor_4ec_apply_set(subdev, &regs_set.af_normal_mode_2);
	if (ret) {
		cam_err("write cmd failed\n");
		goto p_err;
	}

	frame_delay = sensor_4ec_get_frame_duration(subdev);

	if (s5k4ec_state->focus_mode == V4L2_FOCUS_MODE_AUTO)
		delay = CONST_SOFTLANDING_DELAY_AUTO_MS + frame_delay;
	else if (s5k4ec_state->focus_mode == V4L2_FOCUS_MODE_MACRO)
		delay = CONST_SOFTLANDING_DELAY_MACRO_MS + frame_delay * 3;

	if (delay > MAX_SOFTLANDING_DELAY_MS)
		delay = MAX_SOFTLANDING_DELAY_MS;

	cam_info("softlanding delay = %d ms\n", delay);

	/* Delay for moving lens */
	msleep(delay);

p_err:
	return ret;
}

static void  sensor_4ec_set_focus_work(struct work_struct *work)
{
	struct s5k4ec_state *s5k4ec_state =
		container_of(work, struct s5k4ec_state,
		set_focus_mode_work);

	BUG_ON(!s5k4ec_state);

	sensor_4ec_s_focus_mode(&s5k4ec_state->subdev,
		s5k4ec_state->focus_mode);
}

static void  sensor_4ec_af_worker(struct work_struct *work)
{
	struct s5k4ec_state *s5k4ec_state =
		container_of(work, struct s5k4ec_state, af_work);

	BUG_ON(!s5k4ec_state);

	sensor_4ec_auto_focus_start(&s5k4ec_state->subdev);
}

static void  sensor_4ec_af_stop_worker(struct work_struct *work)
{
	struct s5k4ec_state *s5k4ec_state =
		container_of(work, struct s5k4ec_state, af_stop_work);

	BUG_ON(!s5k4ec_state);

	sensor_4ec_auto_focus_stop(&s5k4ec_state->subdev);
}

static int sensor_4ec_verify_window(struct v4l2_subdev *subdev)
{
	int err = 0;
	u16 outter_window_width = 0, outter_window_height = 0;
	u16 inner_window_width = 0, inner_window_height = 0;
	u16 read_value[2] = {0, };
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);

	/* Set first widnow x width, y height */
	err |= sensor_4ec_read_addr(subdev, 0x0298, read_value, 2);
	outter_window_width = read_value[0];
	outter_window_height = read_value[1];

	memset(read_value, 0, sizeof(read_value));

	/* Set second widnow x width, y height */
	err |= sensor_4ec_read_addr(subdev, 0x02A0, read_value, 2);
	inner_window_width = read_value[0];
	inner_window_height = read_value[1];

	if ((outter_window_width != FIRST_WINSIZE_X)
		|| (outter_window_height != FIRST_WINSIZE_Y)
		|| (inner_window_width != SCND_WINSIZE_X)
		|| (inner_window_height != SCND_WINSIZE_Y)) {
		cam_err("invalid window size(0x%X, 0x%X) (0x%X, 0x%X)\n",
			outter_window_width, outter_window_height,
			inner_window_width, inner_window_height);
	} else {
		s5k4ec_state->focus.window_verified = 1;
		cam_info("outter(0x%X, 0x%X) inner(0x%X, 0x%X)\n",
			outter_window_width, outter_window_height,
			inner_window_width, inner_window_height);
	}

	return err;
}

static int sensor_4ec_set_af_window(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct i2c_client *client = to_client(subdev);
	struct s5k4ec_state *s5k4ec_state;

	struct s5k4ec_rect inner_window, outter_window;
	struct s5k4ec_rect first_window, second_window;

	s32 mapped_x = 0, mapped_y = 0;
	u32 preview_width = 0, preview_height = 0;

	u32 inner_half_width = 0, inner_half_height = 0;
	u32 outter_half_width = 0, outter_half_height = 0;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);

	inner_window.x = 0;
	inner_window.y = 0;
	inner_window.width = 0;
	inner_window.height = 0;

	outter_window.x = 0;
	outter_window.y = 0;
	outter_window.width = 0;
	outter_window.height = 0;

	first_window.x = 0;
	first_window.y = 0;
	first_window.width = 0;
	first_window.height = 0;

	second_window.x = 0;
	second_window.y = 0;
	second_window.width = 0;
	second_window.height = 0;

	mapped_x = s5k4ec_state->focus.pos_x;
	mapped_y = s5k4ec_state->focus.pos_y;

	preview_width = s5k4ec_state->preview.width;
	preview_height = s5k4ec_state->preview.height;
	if (unlikely(preview_width <= 0 || preview_height <= 0)) {
		cam_warn("invalid preview_width(%d) or preview_height(%d)\n",
			preview_width, preview_height);
		goto p_err;
	}

	cam_info("E\n");

	mutex_lock(&s5k4ec_state->af_lock);

	/* Verify 1st, 2nd widnwo size */
	if (!s5k4ec_state->focus.window_verified)
		sensor_4ec_verify_window(subdev);

	inner_window.width = SCND_WINSIZE_X * preview_width / 1024;
	inner_window.height = SCND_WINSIZE_Y * preview_height / 1024;
	outter_window.width = FIRST_WINSIZE_X * preview_width / 1024;
	outter_window.height = FIRST_WINSIZE_Y * preview_height / 1024;
	inner_half_width = inner_window.width / 2;
	inner_half_height = inner_window.height / 2;
	outter_half_width = outter_window.width / 2;
	outter_half_height = outter_window.height / 2;

	/* Get X */
	if (mapped_x <= inner_half_width) {
		inner_window.x = outter_window.x = 0;
	} else if (mapped_x <= outter_half_width) {
		inner_window.x = mapped_x - inner_half_width;
		outter_window.x = 0;
	} else if (mapped_x >= ((preview_width - 1) - inner_half_width)) {
		inner_window.x = (preview_width - 1) - inner_window.width;
		outter_window.x = (preview_width - 1) - outter_window.width;
	} else if (mapped_x >= ((preview_width - 1) - outter_half_width)) {
		inner_window.x = mapped_x - inner_half_width;
		outter_window.x = (preview_width - 1) - outter_window.width;
	} else {
		inner_window.x = mapped_x - inner_half_width;
		outter_window.x = mapped_x - outter_half_width;
	}

	/* Get Y */
	if (mapped_y <= inner_half_height) {
		inner_window.y = outter_window.y = 0;
	} else if (mapped_y <= outter_half_height) {
		inner_window.y = mapped_y - inner_half_height;
		outter_window.y = 0;
	} else if (mapped_y >= ((preview_height - 1) - inner_half_height)) {
		inner_window.y = (preview_height - 1) - inner_window.height;
		outter_window.y = (preview_height - 1) - outter_window.height;
	} else if (mapped_y >= ((preview_height - 1) - outter_half_height)) {
		inner_window.y = mapped_y - inner_half_height;
		outter_window.y = (preview_height - 1) - outter_window.height;
	} else {
		inner_window.y = mapped_y - inner_half_height;
		outter_window.y = mapped_y - outter_half_height;
	}

	cam_info("inner_window top=(%d,%d), bottom=(%d,%d)\n",
		inner_window.x, inner_window.y,
		inner_window.x + inner_window.width,
		inner_window.y + inner_window.height);
	cam_info("outter window top=(%d,%d), bottom(%d,%d)\n",
		outter_window.x, outter_window.y,
		outter_window.x + outter_window.width,
		outter_window.y + outter_window.height);

	second_window.x = inner_window.x * 1024 / preview_width;
	second_window.y = inner_window.y * 1024 / preview_height;
	first_window.x = outter_window.x * 1024 / preview_width;
	first_window.y = outter_window.y * 1024 / preview_height;

	cam_info("second_window top=(%d,%d)\n",
		second_window.x, second_window.y);
	cam_info("first_window top=(%d,%d)\n",
		first_window.x, first_window.y);

	mutex_lock(&s5k4ec_state->i2c_lock);

	/* restore write mode */
	sensor_4ec_i2c_write16(client, 0x0028, 0x7000);

	/* Set first widnow x, y */
	sensor_4ec_i2c_write16(client, 0x002A, 0x0294);
	sensor_4ec_i2c_write16(client, 0x0F12, (u16)(first_window.x));

	sensor_4ec_i2c_write16(client, 0x002A, 0x0296);
	sensor_4ec_i2c_write16(client, 0x0F12, (u16)(first_window.y));

	/* Set second widnow x, y */
	sensor_4ec_i2c_write16(client, 0x002A, 0x029C);
	sensor_4ec_i2c_write16(client, 0x0F12, (u16)(second_window.x));

	sensor_4ec_i2c_write16(client, 0x002A, 0x029E);
	sensor_4ec_i2c_write16(client, 0x0F12, (u16)(second_window.y));

	/* Update AF window */
	sensor_4ec_i2c_write16(client, 0x002A, 0x02A4);
	sensor_4ec_i2c_write16(client, 0x0F12, 0x0001);

	mutex_unlock(&s5k4ec_state->i2c_lock);

	mutex_unlock(&s5k4ec_state->af_lock);

p_err:
	return ret;
}

static int sensor_4ec_set_touch_af(struct v4l2_subdev *subdev, s32 val)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (val) {
		if (mutex_is_locked(&s5k4ec_state->af_lock)) {
			cam_warn("AF is still operating!\n");
			return 0;
		}
		ret = sensor_4ec_set_af_window(subdev);
	} else {
		cam_info("set_touch_af: invalid value %d\n", val);
	}

p_err:
	return ret;
}

static int sensor_4ec_s_capture_mode(struct v4l2_subdev *subdev,
	int value)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	cam_info("%d\n", value);

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (value)
		s5k4ec_state->mode = S5K4EC_OPRMODE_IMAGE;
	else
		s5k4ec_state->mode = S5K4EC_OPRMODE_VIDEO;

p_err:
	return ret;
}

static int sensor_4ec_s_sensor_mode(struct v4l2_subdev *subdev,
	int value)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	cam_info("%d\n", value);

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	switch (value) {
	case SENSOR_CAMERA:
		cam_info("SENSOR_CAMERA mode\n");

		if (s5k4ec_state->sensor_mode == SENSOR_MOVIE) {
			ret = sensor_4ec_apply_set(subdev,
				&regs_set.camcorder_disable);
			if (ret) {
				cam_err("write cmd failed\n");
				goto p_err;
			}
		}

		ret = sensor_4ec_apply_set(subdev, &regs_set.preview_return);
		if (ret) {
			cam_err("write cmd failed\n");
			goto p_err;
		}

		s5k4ec_state->sensor_mode = SENSOR_CAMERA;
		break;
	case SENSOR_MOVIE:
		cam_info("SENSOR_MOVIE mode\n");

		ret = sensor_4ec_apply_set(subdev, &regs_set.camcorder);
		if (ret) {
			cam_err("write cmd failed\n");
			goto p_err;
		}

		s5k4ec_state->sensor_mode = SENSOR_MOVIE;
		break;
	default:
		cam_err("invalid mode %d\n", value);
		ret = -EINVAL;
		break;
	}

p_err:
	return ret;
}

static int sensor_4ec_s_duration(struct v4l2_subdev *subdev,
	u64 duration)
{
	int ret = 0;
	u32 framerate;
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	cam_info("%lld\n", duration);

	s5k4ec_state = to_state(subdev);

	framerate = duration;
	if (framerate > FRAME_RATE_MAX) {
		cam_err("framerate is invalid(%d)", framerate);
		ret = -EINVAL;
		goto p_err;
	}

	s5k4ec_state->fps = framerate;

p_err:
	return ret;
}

static int sensor_4ec_set_framerate(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u32 framerate;
	u32 frametime;
	struct s5k4ec_state *s5k4ec_state;
	struct i2c_client *client = to_client(subdev);

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);

	framerate = s5k4ec_state->fps;

	if (s5k4ec_state->mode == S5K4EC_OPRMODE_IMAGE)
		return 0;

	if (framerate > FRAME_RATE_30) {
		cam_info("invalid(%d) value, forced set to MAX", framerate);
		framerate = FRAME_RATE_30;
	}

	if (framerate == 0) {
		cam_info("framerate=0, auto mode\n");
		ret = sensor_4ec_apply_set(subdev,
			&regs_set.fps[FRAME_RATE_AUTO]);
		if (ret)
			cam_err("write cmd failed\n");

		return ret;
	}

	frametime = 10000 / (u32)framerate;
	cam_info("framerate=%d, frametime=%d\n", framerate, frametime);

	sensor_4ec_i2c_write16(client, 0x0028, 0x7000);

	/* REG_0TC_PCFG_usFrTimeType */
	sensor_4ec_i2c_write16(client, 0x002A, 0x02BE);
	sensor_4ec_i2c_write16(client, 0x0F12, 2); /* fixed framerate */
	sensor_4ec_i2c_write16(client, 0x0F12, 1); /* best framerate */
	sensor_4ec_i2c_write16(client, 0x0F12, frametime);
	sensor_4ec_i2c_write16(client, 0x0F12, frametime);

	return ret;
}

static int sensor_4ec_s_power_off(struct v4l2_subdev *subdev,
	u32 val)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state;

	BUG_ON(!subdev);

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	flush_work(&s5k4ec_state->set_focus_mode_work);
	flush_work(&s5k4ec_state->af_work);
	flush_work(&s5k4ec_state->af_stop_work);

	cam_info("%d\n", val);

	ret = sensor_4ec_auto_focus_softlanding(subdev);
	if (ret) {
		cam_err("write cmd failed\n");
		goto p_err;
	}

p_err:
	return ret;
}

static int sensor_4ec_set_focus_mode(struct v4l2_subdev *subdev,
	int mode)
{
	struct s5k4ec_state *s5k4ec_state = NULL;
	int busy = 0;
	int ret = 0;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	busy = work_busy(&s5k4ec_state->set_focus_mode_work);
	s5k4ec_state->focus_mode = (mode & ~V4L2_FOCUS_MODE_DEFAULT);

	if (busy) {
		cam_info("set_focus_mode_work is busy(%d)\n", busy);
		if (!(mode & V4L2_FOCUS_MODE_DEFAULT)) {
			flush_work(&s5k4ec_state->set_focus_mode_work);
			ret = (schedule_work(&s5k4ec_state->set_focus_mode_work)
				== 1) ? 0 : -EINVAL;
			if (ret < 0)
				cam_err("set focus mode fail.\n");
		}
	} else {
		ret = (schedule_work(&s5k4ec_state->set_focus_mode_work)
			== 1) ? 0 : -EINVAL;
		if (ret < 0)
			cam_err("set focus mode fail.\n");
	}

p_err:
	return ret;
}

static int sensor_4ec_s_ctrl(struct v4l2_subdev *subdev,
	struct v4l2_control *ctrl)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state = NULL;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	mutex_lock(&s5k4ec_state->ctrl_lock);

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_TOUCH_AF_START_STOP:
		ret = sensor_4ec_set_touch_af(subdev, 1);
		if (ret < 0)
			cam_err("sensor_4ec_set_touch_af fail.\n");
		break;
	case V4L2_CID_CAMERA_SCENE_MODE:
		ret = sensor_4ec_s_scene_mode(subdev, ctrl->value);
		if (ret < 0)
			cam_err("sensor_4ec_s_scene_mode fail.\n");
		break;
	case V4L2_CID_CAMERA_FOCUS_MODE:
		if (sensor_4ec_check_focus_mode(subdev, ctrl->value))
			ret = sensor_4ec_set_focus_mode(subdev, ctrl->value);
		else
			cam_info("same focus mode( %d ). skip to set.\n",
				ctrl->value);
		break;
	case V4L2_CID_CAMERA_WHITE_BALANCE:
		ret = sensor_4ec_s_white_balance(subdev, ctrl->value);
		if (ret < 0)
			cam_err("sensor_4ec_s_white_balance fail.\n");
		break;
	case V4L2_CID_CAMERA_EFFECT:
		ret = sensor_4ec_s_effect(subdev, ctrl->value);
		if (ret < 0)
			cam_err("sensor_4ec_s_effect fail.\n");
		break;
	case V4L2_CID_CAMERA_ISO:
		ret = sensor_4ec_s_iso(subdev, ctrl->value);
		if (ret < 0)
			cam_err("sensor_4ec_s_iso fail.\n");
		break;
	case V4L2_CID_CAMERA_CONTRAST:
		ret = sensor_4ec_s_contrast(subdev, ctrl->value);
		if (ret < 0)
			cam_err("sensor_4ec_s_contrast fail.\n");
		break;
	case V4L2_CID_CAMERA_SATURATION:
		ret = sensor_4ec_s_saturation(subdev, ctrl->value);
		if (ret < 0)
			cam_err("sensor_4ec_s_saturation fail.\n");
		break;
	case V4L2_CID_CAMERA_SHARPNESS:
		ret = sensor_4ec_s_sharpness(subdev, ctrl->value);
		if (ret < 0)
			cam_err("sensor_4ec_s_sharpness fail.\n");
		break;
	case V4L2_CID_CAMERA_BRIGHTNESS:
		ret = sensor_4ec_s_brightness(subdev, ctrl->value);
		if (ret < 0)
			cam_err("sensor_4ec_s_brightness fail.\n");
		break;
	case V4L2_CID_CAMERA_METERING:
		ret = sensor_4ec_s_metering(subdev, ctrl->value);
		if (ret < 0)
			cam_err("sensor_4ec_s_metering fail.\n");
		break;
	case V4L2_CID_CAMERA_POWER_OFF:
		sensor_4ec_s_power_off(subdev, ctrl->value);
		break;
	case V4L2_CID_CAM_SINGLE_AUTO_FOCUS:
		s5k4ec_state->af_result = AF_RESULT_DOING;
		ret = (schedule_work(&s5k4ec_state->af_work) == 1) ?
			0 : -EINVAL;
		if (ret < 0)
			cam_err("Start auto focus fail.\n");
		break;
	case V4L2_CID_CAMERA_OBJECT_POSITION_X:
		s5k4ec_state->focus.pos_x = ctrl->value;
		break;
	case V4L2_CID_CAMERA_OBJECT_POSITION_Y:
		s5k4ec_state->focus.pos_y = ctrl->value;
		break;
	case V4L2_CID_CAMERA_AE_LOCK_UNLOCK:
		s5k4ec_state->user_ae_lock = ctrl->value;
		ret = sensor_4ec_ae_lock(subdev, ctrl->value);
		if (ret < 0)
			cam_err("ae_lock_unlock fail.\n");
		break;
	case V4L2_CID_CAMERA_AWB_LOCK_UNLOCK:
		s5k4ec_state->user_awb_lock = ctrl->value;
		ret = sensor_4ec_awb_lock(subdev, ctrl->value);
		if (ret < 0)
			cam_err("awb_lock_unlock fail.\n");
		break;
	case V4L2_CID_CAMERA_CAPTURE:
		ret = sensor_4ec_s_capture_mode(subdev, ctrl->value);
		if (ret) {
			cam_err("sensor_4ec_s_capture_mode is fail(%d)", ret);
			goto p_err;
		}
		break;
	case V4L2_CID_CAMERA_FLASH_MODE:
		ret = sensor_4ec_s_flash(subdev, ctrl->value);
		if (ret) {
			cam_err("sensor_4ec_s_flash is fail(%d)", ret);
			goto p_err;
		}
		break;
	case V4L2_CID_CAMERA_SENSOR_MODE:
		ret = sensor_4ec_s_sensor_mode(subdev, ctrl->value);
		if (ret) {
			cam_err("sensor_4ec_s_sensor_mode is fail(%d)", ret);
			goto p_err;
		}
		break;
	default:
		cam_err("invalid ioctl(0x%08X) is requested", ctrl->id);
		/* ret = -EINVAL; */
		break;
	}

p_err:
	mutex_unlock(&s5k4ec_state->ctrl_lock);

	return ret;
}

static int sensor_4ec_g_ctrl(struct v4l2_subdev *subdev,
	struct v4l2_control *ctrl)
{
	int ret = 0;
	struct s5k4ec_state *s5k4ec_state = NULL;

	s5k4ec_state = to_state(subdev);
	if (unlikely(!s5k4ec_state)) {
		cam_err("s5k4ec_state is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	mutex_lock(&s5k4ec_state->ctrl_lock);

	switch (ctrl->id) {
	case V4L2_CID_EXIF_EXPOSURE_TIME_DEN:
		ctrl->value = s5k4ec_state->exif.exp_time_den;
		cam_dbg("V4L2_CID_CAMERA_EXIF_EXPTIME :%d\n", ctrl->value);
		break;
	case V4L2_CID_EXIF_EXPOSURE_TIME_NUM:
		ctrl->value = 1; /* s5k4ec_state->exif.exp_time_num */
		cam_dbg("V4L2_CID_EXIF_EXPOSURE_TIME_NUM :%d\n", ctrl->value);
		break;
	case V4L2_CID_EXIF_SHUTTER_SPEED_NUM:
		ctrl->value = 1;
		cam_dbg("V4L2_CID_EXIF_SHUTTER_SPEED_NUM :%d\n", ctrl->value);
		break;
	case V4L2_CID_CAMERA_EXIF_FLASH:
		ctrl->value = s5k4ec_state->exif.flash;
		cam_dbg("V4L2_CID_CAMERA_EXIF_FLASH :%d\n", ctrl->value);
		break;
	case V4L2_CID_CAMERA_EXIF_ISO:
		ctrl->value = s5k4ec_state->exif.iso;
		cam_dbg("V4L2_CID_CAMERA_EXIF_ISO :%d\n", ctrl->value);
		break;
	case V4L2_CID_CAMERA_EXIF_TV:
		ctrl->value = 0;
		break;
	case V4L2_CID_CAMERA_EXIF_BV:
		ctrl->value = 0;
		break;
	case V4L2_CID_CAMERA_AUTO_FOCUS_DONE:
		ctrl->value = s5k4ec_state->af_result;
		cam_dbg("%s: af_result = %d\n", __func__, ctrl->value);
		break;
	default:
		cam_err("invalid ioctl(0x%08X) is requested", ctrl->id);
		ret = -EINVAL;
		break;
	}

p_err:
	mutex_unlock(&s5k4ec_state->ctrl_lock);

	return ret;
}

static int sensor_4ec_subdev_open(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	cam_info("\n");

	return 0;
}

static int sensor_4ec_subdev_close(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	cam_info("\n");

	return 0;
}

static int sensor_4ec_subdev_registered(struct v4l2_subdev *sd)
{
	cam_info("\n");
	return 0;
}

static void sensor_4ec_subdev_unregistered(struct v4l2_subdev *sd)
{
	cam_info("\n");
}

static int sensor_4ec_link_setup(struct media_entity *entity,
		const struct media_pad *local,
		const struct media_pad *remote, u32 flags)
{
	cam_info("\n");
	return 0;
}

static int sensor_4ec_s_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
{
	struct v4l2_mbus_framefmt *mf = &fmt->format;

	return sensor_4ec_s_format(sd, mf);
}

static struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt		= sensor_4ec_s_fmt,
};

static const struct v4l2_subdev_core_ops core_ops = {
	.init		= sensor_4ec_init,
	.s_ctrl		= sensor_4ec_s_ctrl,
	.g_ctrl		= sensor_4ec_g_ctrl
};

static const struct v4l2_subdev_video_ops video_ops = {
	.s_stream = sensor_4ec_s_stream,
	.s_parm = sensor_4ec_s_param,
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad	= &pad_ops,
};

static const struct v4l2_subdev_internal_ops internal_ops = {
	.open = sensor_4ec_subdev_open,
	.close = sensor_4ec_subdev_close,
	.registered = sensor_4ec_subdev_registered,
	.unregistered = sensor_4ec_subdev_unregistered,
};

static const struct media_entity_operations media_ops = {
	.link_setup = sensor_4ec_link_setup,
};

int sensor_4ec_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct v4l2_subdev *subdev_module;
	struct s5k4ec_state *s5k4ec_state;

	s5k4ec_state = devm_kzalloc(&client->dev, sizeof(struct s5k4ec_state),
		GFP_KERNEL);
	if (!s5k4ec_state) {
		ret = -ENOMEM;
		goto p_err;
	}

	subdev_module = &s5k4ec_state->subdev;
	snprintf(subdev_module->name, V4L2_SUBDEV_NAME_SIZE,
		"%s", SENSOR_NAME);

	v4l2_i2c_subdev_init(subdev_module, client, &subdev_ops);

	s5k4ec_state->pad.flags = MEDIA_PAD_FL_SOURCE;

	subdev_module->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
	subdev_module->flags = V4L2_SUBDEV_FL_HAS_DEVNODE;
	subdev_module->internal_ops = &internal_ops;
	subdev_module->entity.ops = &media_ops;

	ret = media_entity_init(&subdev_module->entity, 1,
		&s5k4ec_state->pad, 0);
	if (ret < 0) {
		cam_err("failed\n");
		return ret;
	}

	s5k4ec_state->client = client;
	mutex_init(&s5k4ec_state->ctrl_lock);
	mutex_init(&s5k4ec_state->af_lock);
	mutex_init(&s5k4ec_state->i2c_lock);
	init_completion(&s5k4ec_state->af_complete);

	INIT_WORK(&s5k4ec_state->set_focus_mode_work,
		sensor_4ec_set_focus_work);
	INIT_WORK(&s5k4ec_state->af_work, sensor_4ec_af_worker);
	INIT_WORK(&s5k4ec_state->af_stop_work, sensor_4ec_af_stop_worker);

	s5k4ec_state->fps = FRAME_RATE_30; /* default frame rate */
	s5k4ec_state->mode = S5K4EC_OPRMODE_VIDEO;

p_err:
	cam_info("(%d)\n", ret);
	return ret;
}

static int sensor_4ec_remove(struct i2c_client *client)
{
	int ret = 0;
	struct v4l2_subdev *subdev = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(subdev);
	media_entity_cleanup(&subdev->entity);

	return ret;
}

static const struct i2c_device_id sensor_4ec_idt[] = {
	{ SENSOR_NAME, 0 },
	{ }
};

static struct i2c_driver sensor_4ec_driver = {
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
	},
	.probe	= sensor_4ec_probe,
	.remove	= sensor_4ec_remove,
	.id_table = sensor_4ec_idt
};

module_i2c_driver(sensor_4ec_driver);

MODULE_AUTHOR("Gilyeon lim");
MODULE_AUTHOR("Sooman Jeong <sm5.jeong@samsung.com>");
MODULE_DESCRIPTION("Sensor 4ec driver");
MODULE_LICENSE("GPL v2");
