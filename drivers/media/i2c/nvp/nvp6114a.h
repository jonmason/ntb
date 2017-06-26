#ifndef __NVP6124A_H__
#define	__NVP6124A_H__

#include <linux/regulator/machine.h>

#define I2C_RETRY_CNT	5

struct reg_val {
	uint8_t	reg;
	uint8_t val;
};

#define END_MARKER	{0xff, 0xff};

#define DEBUG_NVP6114A

#ifdef DEBUG_NVP6114A
	#define vmsg(a...)	printk(KERN_ERR a)
#else
	#define vmsg(a...)
#endif

struct dev_state {
	struct media_pad pad;
	struct v4l2_subdev sd;
	bool first;

	int mode;
	int width;
	int height;

	struct i2c_client *i2c_client;

	struct v4l2_ctrl_handler handler;

	/* common control */
	struct v4l2_ctrl *ctrl_mux;
	struct v4l2_ctrl *ctrl_status;

	/* standard control */
	struct v4l2_ctrl *ctrl_brightness;
	char brightness;

	/* worker */
	struct work_struct work;
};

#endif
