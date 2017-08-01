/*****************************************************************************
	Copyright(c) 2013 FCI Inc. All Rights Reserved

	File name : fc8300_i2c.c

	Description : source of I2C interface

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	History :
	----------------------------------------------------------------------
*******************************************************************************/
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/delay.h>

#include "fc8300.h"
#include "fci_types.h"
#include "fc8300_regs.h"
#include "fci_oal.h"
#include "fc8300_spi.h"

#define I2C_M_FCIRD 1
#define I2C_M_FCIWR 0
#define I2C_MAX_SEND_LENGTH 256

struct i2c_ts_driver {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work;
};

struct i2c_client *fc8300_i2c;

struct i2c_driver fc8300_i2c_driver;
static DEFINE_MUTEX(fci_i2c_lock);

static int fc8300_i2c_probe(struct i2c_client *i2c_client,
	const struct i2c_device_id *id)
{
	fc8300_i2c = i2c_client;
	i2c_set_clientdata(i2c_client, NULL);
	return 0;
}

static int fc8300_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id fc8300_id[] = {
	{ "fc8300_i2c", 0 },
	{ },
};

static const struct of_device_id fc8300_i2c_dt_match[] = {
	{ .compatible = "fci,fc8300_i2c" },
	{ },
};
MODULE_DEVICE_TABLE(of, fc8300_i2c_dt_match);

struct i2c_driver fc8300_i2c_driver = {
	.driver = {
			.name = "fc8300_i2c",
			.owner = THIS_MODULE,
			.of_match_table = of_match_ptr(fc8300_i2c_dt_match),
		},
	.probe    = fc8300_i2c_probe,
	.remove   = fc8300_remove,
	.id_table = fc8300_id,
};

static s32 i2c_bulkread(HANDLE handle, u8 chip, u16 addr, u8 *data, u16 length)
{
	int res;
	struct i2c_msg rmsg[2];
	unsigned char i2c_data[2];

	rmsg[0].addr = chip;
	rmsg[0].flags = I2C_M_FCIWR;
	rmsg[0].len = 2;
	rmsg[0].buf = i2c_data;
	i2c_data[0] = (addr >> 8) & 0xff;
	i2c_data[1] = addr & 0xff;

	rmsg[1].addr = chip;
	rmsg[1].flags = I2C_M_FCIRD;
	rmsg[1].len = length;
	rmsg[1].buf = data;
	res = i2c_transfer(fc8300_i2c->adapter, &rmsg[0], 2);

	return 0;
}


static s32 i2c_bulkwrite(HANDLE handle, u8 chip, u16 addr, u8 *data, u16 length)
{
	int res;
	struct i2c_msg wmsg;
	unsigned char i2c_data[I2C_MAX_SEND_LENGTH];

	if ((length + 1) > I2C_MAX_SEND_LENGTH)
		return -ENODEV;

	wmsg.addr = chip;
	wmsg.flags = I2C_M_FCIWR;
	wmsg.len = length + 2;
	wmsg.buf = i2c_data;

	i2c_data[0] = (addr >> 8) & 0xff;
	i2c_data[1] = addr & 0xff;
	memcpy(&i2c_data[2], data, length);

	res = i2c_transfer(fc8300_i2c->adapter, &wmsg, 1);

	return 0;
}

s32 fc8300_i2c_init(HANDLE handle, u16 param1, u16 param2)
{
	s32 res;

	fc8300_i2c = kzalloc(sizeof(struct i2c_ts_driver), GFP_KERNEL);

	if (fc8300_i2c == NULL)
	{
		return -ENOMEM;
	}

	res = i2c_add_driver(&fc8300_i2c_driver);
	((struct ISDBT_INIT_INFO_T *)handle)->client = fc8300_i2c;

#ifdef BBM_I2C_SPI
#else
	/* ts_initialize(); */
#ifdef CONFIG_NXP_MP2TS_IF_FCI
	tsif_init(tsif_get_channel_num());
#endif
#endif

	return res;
}

s32 fc8300_i2c_byteread(HANDLE handle, DEVICEID devid, u16 addr, u8 *data)
{
	s32 res;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkread(handle, (u8) (devid >> 8) & 0xff, addr, data, 1);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8300_i2c_wordread(HANDLE handle, DEVICEID devid, u16 addr, u16 *data)
{
	s32 res;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkread(handle, (u8) (devid >> 8) & 0xff,
			addr, (u8 *) data, 2);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8300_i2c_longread(HANDLE handle, DEVICEID devid, u16 addr, u32 *data)
{
	s32 res;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkread(handle, (u8) (devid >> 8) & 0xff,
			addr, (u8 *) data, 4);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8300_i2c_bulkread(HANDLE handle, DEVICEID devid,
		u16 addr, u8 *data, u16 length)
{
	s32 res;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkread(handle, (u8) (devid >> 8) & 0xff,
			addr, data, length);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8300_i2c_bytewrite(HANDLE handle, DEVICEID devid, u16 addr, u8 data)
{
	s32 res;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, (u8) (devid >> 8) & 0xff,
			addr, (u8 *)&data, 1);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8300_i2c_wordwrite(HANDLE handle, DEVICEID devid, u16 addr, u16 data)
{
	s32 res;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, (u8) (devid >> 8) & 0xff,
			addr, (u8 *)&data, 2);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8300_i2c_longwrite(HANDLE handle, DEVICEID devid, u16 addr, u32 data)
{
	s32 res;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, (u8) (devid >> 8) & 0xff,
			addr, (u8 *)&data, 4);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8300_i2c_bulkwrite(HANDLE handle, DEVICEID devid,
		u16 addr, u8 *data, u16 length)
{
	s32 res;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, (u8) (devid >> 8) & 0xff,
			addr, data, length);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8300_i2c_dataread(HANDLE handle, DEVICEID devid,
		u16 addr, u8 *data, u32 length)
{
	s32 res;

#ifdef BBM_I2C_SPI
	res = fc8300_spi_dataread(handle, devid,
		addr, data, length);
#else
	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkread(handle, (u8) (devid >> 8) & 0xff,
			addr, data, length);
	mutex_unlock(&fci_i2c_lock);
#endif

	return res;
}

s32 fc8300_i2c_deinit(HANDLE handle)
{
	i2c_del_driver(&fc8300_i2c_driver);
#ifdef BBM_I2C_SPI
	fc8300_spi_deinit(handle);
#else
	/* ts_receiver_disable(); */
#endif

	return BBM_OK;
}

