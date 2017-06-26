#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>

#include "nvp6114a.h"
#include "nvp6114a-preset.h"

#include "video.h"
#include "coax_protocol.h"
#include "motion.h"
#include "common.h"
#include "audio.h"

#define AHD_DEV_NAME	"nvp6114a"

#define NVP6124_R0_ID   0x84
#define NVP6114A_R0_ID  0x85
#define NVP6124B_R0_ID  0x86

#define PID 0xF4

static struct dev_state nvp6114a;

int chip_id[4];
unsigned int nvp6124_mode = 0;
unsigned int nvp6124_cnt = 1;
unsigned int nvp6124_slave_addr[4] = {0x64, 0x00, 0x00, 0x00};
unsigned int vloss=0xFFFF;


int nvp6114a_initialization(struct i2c_client *client);

/**
 *util functions
 */
static inline struct dev_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct dev_state, sd);
}

bool check_id(struct i2c_client *client)
{
    u8 pid = 0;
    i2c_smbus_write_byte_data(client, 0XFF, 0x00);
    pid = i2c_smbus_read_byte_data(client, PID);
    printk(KERN_ERR "NVP chip id: 0x%02X\n", pid);
    if( pid == NVP6114A_R0_ID ) {
    	chip_id[0]  = NVP6114A_R0_ID;
        return true;
	}
	else if( pid == NVP6124B_R0_ID ) {
    	chip_id[0]  = NVP6124B_R0_ID;
        return true;
	}


    printk(KERN_ERR "fail to check id: 0x%02X\n", pid);
    return false;
}

int nvp_i2c_write(unsigned int slave, u8 addr, u8 data)
{
	struct dev_state *state = &nvp6114a;
    struct i2c_client *client= state->i2c_client;

    int ret = 0;

	if ((ret = i2c_smbus_write_byte_data(client, addr, data)) != 0) {
        printk(KERN_ERR "I2C write Error! ret : %d\n", ret);
    }

    return ret;
}

unsigned char nvp_i2c_read(unsigned char slave, unsigned char addr)
{
	struct dev_state *state = &nvp6114a;
    struct i2c_client *client= state->i2c_client;

    unsigned char ret;

    ret = i2c_smbus_read_byte_data(client, addr);

    return ret;
}

static int _i2c_read_byte(struct i2c_client *client, u8 addr, u8 *data)
{
	s8 i = 0;
	s8 ret = 0;
	u8 buf = 0;

	struct i2c_msg msg[2];

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &addr;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = &buf;

	for(i=0; i<I2C_RETRY_CNT; i++) {
		ret = i2c_transfer(client->adapter, msg, 2);
		if(likely(ret ==  2))
			break;
	}

	if (unlikely(ret != 2)) {
		dev_err(&client->dev, "%s failed reg:0x%02x\n", __func__, addr);
		return -EIO;
	}

	*data = buf;

	return 0;
}


static int _i2c_write_byte(struct i2c_client *client, u8 addr, u8 val)
{
	s8 i = 0;
	s8 ret = 0;
	u8 buf[2];
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = buf;

	buf[0] = addr;
	buf[1] = val;

	for(i=0; i<I2C_RETRY_CNT; i++) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if(likely(ret == 1))
			break;
	}

	if (ret !=  1) {
		printk(KERN_ERR "%s: failed to write addr 0x%x, val 0x%x\n", __func__, addr, val);
		return -EIO;
	}

	return 0;
}


static int nvp6114a_initialize_ctrls(struct dev_state *me)
{
	return 0;
}

static int nvp6114a_write_table(struct i2c_client *client,
								unsigned char addr,
								unsigned char *tbl_ptr, unsigned char tbl_cnt)
{
	unsigned char i=0;

	for(i=0; i<tbl_cnt; i++)
	{
		printk("%s : reg_addr = 0x%02x, reg_val = 0x%02x\n", __func__, (addr+i), *(tbl_ptr+i));

		if(_i2c_write_byte(client, (addr+i), *(tbl_ptr+i)) != 0)
		{
			printk(KERN_ERR "%s : fail to write addr 0x%02X, val 0x%02X\n", __func__, (addr+i), *(tbl_ptr+i));
			//break;
		}
	}

	return 0;
}

static int nvp6114a_write_data(struct i2c_client *client, u8 write_addr, unsigned char reg_val[])
{
	if(_i2c_write_byte(client, 0xFF, write_addr) == 0)
	{
		printk("%s - write_addr : 0x%02X\n", __func__, write_addr);

		nvp6114a_write_table(client, 0x00, reg_val, 254);
	}

	return 0;
}

int nvp6114a_initialization(struct i2c_client *client)
{
	struct dev_state *state = NULL;

	state = &nvp6114a;
	state->i2c_client = client;
#if 0
	unsigned char data;
#endif
	int i=0;
	int width, height;

	nvp6124_cnt = 1;
	/* chip_id[0]  = NVP6114A_R0_ID; */
	nvp6124_mode = NTSC;

	if( !state->first )
	{
		if(!check_id(client))
			return -EINVAL;
#if 0
		if (_i2c_read_byte(client, PID, &data) == 0)
			pr_err("nvp6114a reg =0xF4, data = 0x%02X\n", data);
#endif

		if(chip_id[0] == NVP6114A_R0_ID) {
			nvp6124_ntsc_common_init();
		}
		else if(chip_id[0] == NVP6124B_R0_ID) {
			nvp6124B_ntsc_common_init();
		}

		for (i=0 ; i<nvp6124_cnt ; i++)
			audio_init(nvp6124_slave_addr[i],16,0,0);

		state->first = true;
	}

	if(chip_id[0] == NVP6114A_R0_ID)
		nvp6114a_outport_1mux(nvp6124_mode%2, 0x10|NVP6124_VI_720P_2530, 0x00|NVP6124_VI_720P_2530);
	else if(chip_id[0] == NVP6124B_R0_ID)
		nvp6124B_outport_1mux(nvp6124_mode%2, 0x10|NVP6124_VI_720P_2530, 0x00|NVP6124_VI_720P_2530);

	return 0;
}

EXPORT_SYMBOL(nvp6114a_initialization);

static int nvp6114a_s_stream(struct v4l2_subdev *sd, int enable)
{
    struct dev_state *state = to_state(sd);
    struct i2c_client *client = v4l2_get_subdevdata(sd);
#if 0
    unsigned char data;
#endif
    int i=0;
    int width, height;

    nvp6124_cnt = 1;
    /* chip_id[0]  = NVP6114A_R0_ID; */
    nvp6124_mode = NTSC;

    if( enable )
    {
        if( !state->first )
        {
            if(!check_id(client))
                return -EINVAL;
#if 0
            if (_i2c_read_byte(client, PID, &data) == 0)
            	printk(KERN_ERR "nvp6114a reg =0xF4, data = 0x%02X\n", data);
#endif
		if(chip_id[0] == NVP6114A_R0_ID) {
	        nvp6124_ntsc_common_init();
		}
		else if(chip_id[0] == NVP6124B_R0_ID) {
	        nvp6124B_ntsc_common_init();
		}

            for (i=0 ; i<nvp6124_cnt ; i++)
                audio_init(nvp6124_slave_addr[i],16,0,0);

            state->first = true;
        }

	width = state->width;
	height = state->height;

#if defined(CONFIG_ARCH_S5P4418)
	width -= 32;
	height -= 1;
#endif
		if(chip_id[0] == NVP6114A_R0_ID) {
	        if (width == 1920 && height == 1080)
    	        nvp6114a_outport_1mux(nvp6124_mode%2,
					0x10|NVP6124_VI_1080P_2530, 0x00|NVP6124_VI_1080P_2530);
        	else if (width == 1280 && height == 720)
            	nvp6114a_outport_1mux(nvp6124_mode%2,
					0x10|NVP6124_VI_720P_2530, 0x00|NVP6124_VI_720P_2530);
	    }
		#if 1
		else if(chip_id[0] == NVP6124B_R0_ID) {
	        if (width == 1920 && height == 1080)
    	        nvp6124B_outport_1mux(nvp6124_mode%2,
					0x10|NVP6124_VI_1080P_2530, 0x00|NVP6124_VI_1080P_2530);
        	else if (width == 1280 && height == 720)
            	nvp6124B_outport_1mux(nvp6124_mode%2,
					0x10|NVP6124_VI_720P_2530, 0x00|NVP6124_VI_720P_2530);
		}
		#endif
	}

	return 0;
}

static int nvp6114a_g_fmt(struct v4l2_subdev *sd,
		/*	struct v4l2_subdev_fh *fh,	*/
		struct v4l2_subdev_pad_config *cfg,
    struct v4l2_subdev_format *fmt)
{

   return 0;
}

static int nvp6114a_s_fmt(struct v4l2_subdev *sd,
		/*	struct v4l2_subdev_fh *fh,	*/
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *fmt)
{
	int err=0;
	struct v4l2_mbus_framefmt *_fmt = &fmt->format;
	struct dev_state *state = to_state(sd);

	vmsg("%s\n", __func__);

	state->width = _fmt->width;
	state->height = _fmt->height;

	printk("%s : mode %d, %dx%d\n", __func__, state->mode, state->width, state->height);

	return err;
}

static int nvp6114a_s_power(struct v4l2_subdev *sd, int on)
{
	vmsg("%s: %d\n", __func__, on);
	return 0;
}

static const struct v4l2_subdev_core_ops nvp6114a_core_ops = {
	.s_power = nvp6114a_s_power,
};

static const struct v4l2_subdev_video_ops nvp6114a_video_ops = {
	.s_stream	=	nvp6114a_s_stream,
};

static const struct v4l2_subdev_pad_ops nvp6114a_pad_ops = {
	.set_fmt = nvp6114a_s_fmt,
	.get_fmt = nvp6114a_g_fmt,
};

static const struct v4l2_subdev_ops nvp6114a_ops = {
	.core = &nvp6114a_core_ops,
	.video = &nvp6114a_video_ops,
	.pad = &nvp6114a_pad_ops,
};

static int nvp6214_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct dev_state *state = &nvp6114a;
	struct v4l2_subdev *sd;
	int ret;

	sd = &state->sd;
	strcpy(sd->name, id->name);

	v4l2_i2c_subdev_init(sd, client, &nvp6114a_ops);

	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	state->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
	ret = media_entity_init(&sd->entity, 1, &state->pad, 0);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to media_entiti_init()\n", __func__);
		return -ENOENT;
	};

	i2c_set_clientdata(client, sd);
	state->i2c_client = client;
	state->first = false;

	dev_info(&client->dev, "nvp6114a has been probed\n");

	pr_info("nvp6114a has been probed\n");

	return 0;
}

static int nvp6114a_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	v4l2_device_unregister_subdev(sd);
	v4l2_ctrl_handler_free(sd->ctrl_handler);
	media_entity_cleanup(&sd->entity);
	return 0;
}

static const struct i2c_device_id nvp6114a_id[] = {
	{ AHD_DEV_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, nvp6114a_id);

static struct i2c_driver nvp6114a_i2c_driver = {
	.driver = {
		.name = AHD_DEV_NAME,
	},
	.probe = nvp6214_probe,
	.remove = nvp6114a_remove,
	.id_table = nvp6114a_id,
};

module_i2c_driver(nvp6114a_i2c_driver);

MODULE_DESCRIPTION("NVP6114A AHD Sensor Driver");
MODULE_AUTHOR("<jkchoi@nexell.co.kr>");
MODULE_LICENSE("GPL");
