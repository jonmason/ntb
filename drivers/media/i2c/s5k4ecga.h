/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef S5K4ECGA_H
#define S5K4ECGA_H

/* #define CONFIG_LOAD_FILE */

#if defined(CONFIG_LOAD_FILE)
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define LOAD_FILE_PATH "/etc/s5k4ecga_reg.h"
#endif

#define SENSOR_S5K4EC_INSTANCE	1
#define SENSOR_S5K4EC_NAME	SENSOR_NAME_S5K4EC
#define SENSOR_S5K4EC_DRIVING

#define TAG_NAME	"[4EC] "
#define cam_err(fmt, ...)	\
	pr_err(TAG_NAME "[ERR]%s:%d: " fmt, __func__, __LINE__, \
	##__VA_ARGS__)
#define cam_warn(fmt, ...)	\
	pr_warn(TAG_NAME "%s: " fmt, __func__, \
	##__VA_ARGS__)
#define cam_info(fmt, ...)	\
	pr_info(TAG_NAME "%s: " fmt, __func__, \
	##__VA_ARGS__)
#if defined(CONFIG_CAM_DEBUG)
#define cam_dbg(fmt, ...)	\
	pr_debug(TAG_NAME fmt, ##__VA_ARGS__)
#else
#define cam_dbg(fmt, ...)
#endif

enum v4l2_focus_mode {
	V4L2_FOCUS_MODE_AUTO		= 0,
	V4L2_FOCUS_MODE_MACRO		= 1,
	V4L2_FOCUS_MODE_MANUAL		= 2,
	V4L2_FOCUS_MODE_LASTP		= 2,
};

#define V4L2_CID_CAMERA_CAPTURE			(V4L2_CID_PRIVATE_BASE+50)
#define V4L2_CID_CAMERA_SCENE_MODE		(V4L2_CID_PRIVATE_BASE+70)

#define V4L2_CID_CAMERA_FLASH_MODE		(V4L2_CID_PRIVATE_BASE+71)
enum v4l2_flash_mode {
	FLASH_MODE_BASE,
	FLASH_MODE_OFF,
	FLASH_MODE_AUTO,
	FLASH_MODE_ON,
	FLASH_MODE_TORCH,
	FLASH_MODE_MAX,
};

#define V4L2_CID_CAMERA_BRIGHTNESS		(V4L2_CID_PRIVATE_BASE+72)
enum v4l2_ev_mode {
	EV_MINUS_4	= -4,
	EV_MINUS_3	= -3,
	EV_MINUS_2	= -2,
	EV_MINUS_1	= -1,
	EV_DEFAULT	= 0,
	EV_PLUS_1	= 1,
	EV_PLUS_2	= 2,
	EV_PLUS_3	= 3,
	EV_PLUS_4	= 4,
	EV_MAX,
};

#define V4L2_CID_CAMERA_WHITE_BALANCE		(V4L2_CID_PRIVATE_BASE+73)
enum v4l2_wb_mode {
	WHITE_BALANCE_BASE = 0,
	WHITE_BALANCE_AUTO,
	WHITE_BALANCE_SUNNY,
	WHITE_BALANCE_CLOUDY,
	WHITE_BALANCE_TUNGSTEN,
	WHITE_BALANCE_FLUORESCENT,
	WHITE_BALANCE_MAX,
};

#define V4L2_CID_CAMERA_EFFECT			(V4L2_CID_PRIVATE_BASE+74)
enum v4l2_effect_mode {
	IMAGE_EFFECT_BASE = 0,
	IMAGE_EFFECT_NONE,
	IMAGE_EFFECT_BNW,
	IMAGE_EFFECT_SEPIA,
	IMAGE_EFFECT_AQUA,
	IMAGE_EFFECT_ANTIQUE,
	IMAGE_EFFECT_NEGATIVE,
	IMAGE_EFFECT_SHARPEN,
	IMAGE_EFFECT_MAX,
};

#define V4L2_CID_CAMERA_ISO			(V4L2_CID_PRIVATE_BASE+75)
enum v4l2_iso_mode {
	ISO_AUTO = 0,
	ISO_50,
	ISO_100,
	ISO_200,
	ISO_400,
	ISO_800,
	ISO_1600,
	ISO_SPORTS,
	ISO_NIGHT,
	ISO_MOVIE,
	ISO_MAX,
};

#define V4L2_CID_CAMERA_METERING		(V4L2_CID_PRIVATE_BASE+76)
enum v4l2_metering_mode {
	METERING_BASE = 0,
	METERING_MATRIX,
	METERING_CENTER,
	METERING_SPOT,
	METERING_MAX,
};

#define V4L2_CID_CAMERA_CONTRAST		(V4L2_CID_PRIVATE_BASE+77)
enum v4l2_contrast_mode {
	CONTRAST_MINUS_2 = 0,
	CONTRAST_MINUS_1,
	CONTRAST_DEFAULT,
	CONTRAST_PLUS_1,
	CONTRAST_PLUS_2,
	CONTRAST_MAX,
};

#define V4L2_CID_CAMERA_SATURATION		(V4L2_CID_PRIVATE_BASE+78)
enum v4l2_saturation_mode {
	SATURATION_MINUS_2 = 0,
	SATURATION_MINUS_1,
	SATURATION_DEFAULT,
	SATURATION_PLUS_1,
	SATURATION_PLUS_2,
	SATURATION_MAX,
};

#define V4L2_CID_CAMERA_SHARPNESS		(V4L2_CID_PRIVATE_BASE+79)
enum v4l2_sharpness_mode {
	SHARPNESS_MINUS_2 = 0,
	SHARPNESS_MINUS_1,
	SHARPNESS_DEFAULT,
	SHARPNESS_PLUS_1,
	SHARPNESS_PLUS_2,
	SHARPNESS_MAX,
};

#define V4L2_CID_CAMERA_TOUCH_AF_START_STOP	(V4L2_CID_PRIVATE_BASE+82)
enum v4l2_touch_af {
	TOUCH_AF_STOP = 0,
	TOUCH_AF_START,
	TOUCH_AF_MAX,
};

#define V4L2_CID_CAMERA_OBJECT_POSITION_X	(V4L2_CID_PRIVATE_BASE+97)
#define V4L2_CID_CAMERA_OBJECT_POSITION_Y	(V4L2_CID_PRIVATE_BASE+98)
#define V4L2_CID_CAMERA_AEAWB_LOCK_UNLOCK	(V4L2_CID_PRIVATE_BASE+95)
enum v4l2_ae_awb_lockunlock {
	AE_UNLOCK_AWB_UNLOCK = 0,
	AE_LOCK_AWB_UNLOCK,
	AE_UNLOCK_AWB_LOCK,
	AE_LOCK_AWB_LOCK,
	AE_AWB_MAX
};

#define V4L2_CID_CAMERA_FOCUS_MODE		(V4L2_CID_PRIVATE_BASE+99)
enum v4l2_focusmode {
	FOCUS_MODE_AUTO = 0,
	FOCUS_MODE_MACRO,
	FOCUS_MODE_FACEDETECT,
	FOCUS_MODE_AUTO_DEFAULT,
	FOCUS_MODE_MACRO_DEFAULT,
	FOCUS_MODE_FACEDETECT_DEFAULT,
	FOCUS_MODE_INFINITY,
	FOCUS_MODE_FIXED,
	FOCUS_MODE_CONTINUOUS,
	FOCUS_MODE_CONTINUOUS_PICTURE,
	FOCUS_MODE_CONTINUOUS_PICTURE_MACRO,
	FOCUS_MODE_CONTINUOUS_VIDEO,
	FOCUS_MODE_TOUCH,
	FOCUS_MODE_MAX,
	FOCUS_MODE_DEFAULT = (1 << 8),
};

#define V4L2_CID_CAMERA_FRAME_RATE		(V4L2_CID_PRIVATE_BASE+104)
enum v4l2_frame_rate {
	FRAME_RATE_AUTO = 0,
	FRAME_RATE_7 = 7,
	FRAME_RATE_15 = 15,
	FRAME_RATE_20 = 20,
	FRAME_RATE_30 = 30,
	FRAME_RATE_60 = 60,
	FRAME_RATE_120 = 120,
	FRAME_RATE_MAX
};

#define V4L2_CID_CAMERA_ANTI_BANDING		(V4L2_CID_PRIVATE_BASE+105)
enum v4l2_anti_banding {
	ANTI_BANDING_AUTO = 0,
	ANTI_BANDING_50HZ = 1,
	ANTI_BANDING_60HZ = 2,
	ANTI_BANDING_OFF = 3,
};

#define V4L2_CID_CAMERA_SET_GAMMA		(V4L2_CID_PRIVATE_BASE+106)
enum v4l2_gamma_mode {
	GAMMA_OFF = 0,
	GAMMA_ON = 1,
	GAMMA_MAX,
};

#define V4L2_CID_CAMERA_SET_SLOW_AE		(V4L2_CID_PRIVATE_BASE+107)
enum v4l2_slow_ae_mode {
	SLOW_AE_OFF,
	SLOW_AE_ON,
	SLOW_AE_MAX,
};

#define V4L2_CID_CAMERA_SENSOR_MODE		(V4L2_CID_PRIVATE_BASE+116)
enum v4l2_sensor_mode {
	SENSOR_CAMERA,
	SENSOR_MOVIE,
};

#define V4L2_CID_CAMERA_EXIF_EXPTIME		(V4L2_CID_PRIVATE_BASE+117)
#define V4L2_CID_CAMERA_EXIF_FLASH		(V4L2_CID_PRIVATE_BASE+118)
#define V4L2_CID_CAMERA_EXIF_ISO		(V4L2_CID_PRIVATE_BASE+119)
#define V4L2_CID_CAMERA_EXIF_TV			(V4L2_CID_PRIVATE_BASE+120)
#define V4L2_CID_CAMERA_EXIF_BV			(V4L2_CID_PRIVATE_BASE+121)
#define V4L2_CID_CAMERA_EXIF_EBV		(V4L2_CID_PRIVATE_BASE+122)
#define V4L2_CID_CAMERA_CHECK_ESD		(V4L2_CID_PRIVATE_BASE+123)
#define V4L2_CID_CAMERA_APP_CHECK		(V4L2_CID_PRIVATE_BASE+124)

enum cam_scene_mode {
	SCENE_MODE_BASE,
	SCENE_MODE_NONE,
	SCENE_MODE_PORTRAIT,
	SCENE_MODE_NIGHTSHOT,
	SCENE_MODE_BACK_LIGHT,
	SCENE_MODE_LANDSCAPE,
	SCENE_MODE_SPORTS,
	SCENE_MODE_PARTY_INDOOR,
	SCENE_MODE_BEACH_SNOW,
	SCENE_MODE_SUNSET,
	SCENE_MODE_DUSK_DAWN,
	SCENE_MODE_FALL_COLOR,
	SCENE_MODE_FIREWORKS,
	SCENE_MODE_TEXT,
	SCENE_MODE_CANDLE_LIGHT,
	SCENE_MODE_LOW_LIGHT,
	SCENE_MODE_MAX,
};

#define V4L2_CID_CAMERA_AE_LOCK_UNLOCK	(V4L2_CID_PRIVATE_BASE + 144)
enum v4l2_ae_lockunlock {
	AE_UNLOCK = 0,
	AE_LOCK,
	AE_LOCK_MAX
};

#define V4L2_CID_CAMERA_AWB_LOCK_UNLOCK	(V4L2_CID_PRIVATE_BASE + 145)
enum v4l2_awb_lockunlock {
	AWB_UNLOCK = 0,
	AWB_LOCK,
	AWB_LOCK_MAX
};

#define V4L2_FOCUS_MODE_DEFAULT (1 << 8)
#define V4L2_CID_CAMERA_POWER_OFF	(V4L2_CID_PRIVATE_BASE + 330)
#define V4L2_CID_CAM_SINGLE_AUTO_FOCUS	(V4L2_CID_CAMERA_CLASS_BASE + 63)
#define V4L2_CID_EXIF_SHUTTER_SPEED_NUM (V4L2_CID_CAMERA_CLASS_BASE + 78)
#define V4L2_CID_EXIF_SHUTTER_SPEED_DEN (V4L2_CID_CAMERA_CLASS_BASE + 79)
#define V4L2_CID_EXIF_EXPOSURE_TIME_NUM (V4L2_CID_CAMERA_CLASS_BASE + 80)
#define V4L2_CID_EXIF_EXPOSURE_TIME_DEN (V4L2_CID_CAMERA_CLASS_BASE + 81)
#define V4L2_CID_CAMERA_AUTO_FOCUS_DONE	(V4L2_CID_CAMERA_CLASS_BASE+69)

enum af_operation_status {
	AF_NONE = 0,
	AF_START,
	AF_CANCEL,
};

enum capture_flash_status {
	FLASH_STATUS_OFF = 0,
	FLASH_STATUS_PRE_ON,
	FLASH_STATUS_MAIN_ON,
};

enum s5k4ec_runmode {
	S5K4EC_RUNMODE_NOTREADY,
	S5K4EC_RUNMODE_IDLE,
	S5K4EC_RUNMODE_RUNNING,
	S5K4EC_RUNMODE_CAPTURE,
};

/* result values returned to HAL */
enum af_result_status {
	AF_RESULT_NONE = 0x00,
	AF_RESULT_FAILED = 0x01,
	AF_RESULT_SUCCESS = 0x02,
	AF_RESULT_CANCELLED = 0x04,
	AF_RESULT_DOING = 0x08
};

struct sensor4ec_exif {
	u32 exp_time_num;
	u32 exp_time_den;
	u16 iso;
	u16 flash;
};

/* EXIF - flash filed */
#define EXIF_FLASH_OFF		(0x00)
#define EXIF_FLASH_FIRED		(0x01)
#define EXIF_FLASH_MODE_FIRING		(0x01 << 3)
#define EXIF_FLASH_MODE_SUPPRESSION	(0x02 << 3)
#define EXIF_FLASH_MODE_AUTO		(0x03 << 3)

/* Sensor AF first,second window size.
 * we use constant values instead of reading sensor register */
#define FIRST_WINSIZE_X			512
#define FIRST_WINSIZE_Y			568
#define SCND_WINSIZE_X			116	/* 230 -> 116 */
#define SCND_WINSIZE_Y			306

struct s5k4ec_rect {
	s32 x;
	s32 y;
	u32 width;
	u32 height;
};

struct s5k4ec_focus {
	u32 pos_x;
	u32 pos_y;
	u32 touch:1;
	u32 reset_done:1;
	u32 window_verified:1;	/* touch window verified */
};

struct s5k4ec_framesize {
	u32 index;
	u32 width;
	u32 height;
};

struct s5k4ec_state {
	struct v4l2_subdev	subdev;
	struct v4l2_mbus_framefmt fmt;
	struct media_pad	pad;	/* for media device pad */
	struct mutex		ctrl_lock;
	struct mutex		af_lock;
	struct mutex		i2c_lock;
	struct completion	af_complete;

	u16			sensor_version;
	u32			setfile_index;
	u32			mode;
	u32			contrast;
	u32			effect;
	u32			ev;
	u32			flash_mode;
	u32			focus_mode;
	u32			iso;
	u32			metering;
	u32			saturation;
	u32			scene_mode;
	u32			sharpness;
	u32			white_balance;
	u32			anti_banding;
	u32			fps;
	bool			ae_lock;
	bool			awb_lock;
	bool			user_ae_lock;
	bool			user_awb_lock;
	bool			sensor_af_in_low_light_mode;
	bool			flash_fire;
	u32			sensor_mode;
	u32			light_level;

	enum af_operation_status	af_status;
	enum capture_flash_status flash_status;
	u32 af_result;
	struct work_struct set_focus_mode_work;
	struct work_struct af_work;
	struct work_struct af_stop_work;
	struct sensor4ec_exif	exif;
	struct s5k4ec_focus focus;
	struct s5k4ec_framesize preview;
	struct i2c_client	*client;
	const struct s5k4ec_framesize *stream_size;
};

static int sensor_4ec_set_framerate(struct v4l2_subdev *subdev);
static int sensor_4ec_s_duration(struct v4l2_subdev *subdev, u64 duration);
static int sensor_4ec_stream_on(struct v4l2_subdev *subdev);
static int sensor_4ec_stream_off(struct v4l2_subdev *subdev);
static int sensor_4ec_pre_flash_start(struct v4l2_subdev *subdev);
static int sensor_4ec_pre_flash_stop(struct v4l2_subdev *subdev);
static int sensor_4ec_main_flash_start(struct v4l2_subdev *subdev);
static int sensor_4ec_main_flash_stop(struct v4l2_subdev *subdev);
static int sensor_4ec_auto_focus_proc(struct v4l2_subdev *subdev);
static int sensor_4ec_get_exif(struct v4l2_subdev *subdev);
int sensor_4ec_probe(struct i2c_client *client,
	const struct i2c_device_id *id);

#endif
