/*
* e522xx_driver.c -- USB Charge driver for the e522xx
*
* Copyright (C) 2017  Zhonghong.
* Author:
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

#include <linux/string.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>

#include <linux/device.h>
#include <linux/regulator/consumer.h>
#include <linux/miscdevice.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#include <linux/fs.h>

#include <linux/fb.h>
#include <linux/suspend.h>

#include "../usb_charge.h"

#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/usb.h>

#define E522XX_CTRL_CHECK_NRP	0
#define E522XX_INIT_ERROR_COUNT 5
#define E522XX_WORK_FUNC_TIME 2000
#define BUF_SIZE 1024

bool app_quick_charge;
bool app_phoneapp_charge;

enum e522xx_state {
	E_E522XX_STATE_SLEEP = 0,
	E_E522XX_STATE_STANDBY,
	E_E522XX_STATE_RUN,
	E_E522XX_STATE_ERROR
};

enum e522xx_port_conf {
	E_E522XX_PORT_CONF_RESERVED = 0,
	E_E522XX_PORT_CONF_SDP,
	E_E522XX_PORT_CONF_CDP,
	E_E522XX_PORT_CONF_DCP,
	E_E522XX_PORT_CONF_CCP_APPLE,
	E_E522XX_PORT_CONF_CCP,
	E_E522XX_PORT_CONF_AUTO,
	E_E522XX_PORT_CONF_ERROR
};

struct e522xx_debug_data {
	u8 reg;
	unsigned int en_gpio_value;
	enum e522xx_state state;
};

struct e522xx_data {
	unsigned int int_gpio_number;
	unsigned int en_gpio_number;
	int time;
	struct i2c_client *client;
	struct delayed_work check_work;
	struct workqueue_struct *workqueue;
	struct e522xx_debug_data debug;

	enum e522xx_port_conf port_config;
	int update_port_config;
	struct mutex lock;

	bool init_flag;
	int init_count;

	int vbus_current;
	int vbus_voltage;

	bool suspend_flag;
};

static struct e522xx_data *p_e522xx;

static void e522xx_state_ctrl(int state);
static int e522xx_set_port_config
	(enum e522xx_port_conf port_config);
static int e522xx_req_update_port_config
	(enum e522xx_port_conf port_config);

static int i2c_read_bytes(struct i2c_client *client,
	u8 *txbuf, int txlen, u8 *rxbuf, int rxlen)
{
	int ret = 0;
	u8 retry;

	struct i2c_msg msg[2] = {
		{
#ifdef CONFIG_MTK_I2C_EXTENSION
			.addr = ((client->addr & I2C_MASK_FLAG)
					| (I2C_ENEXT_FLAG)),
			.timing = I2C_MASTER_CLOCK,
#else
			.addr = client->addr,
#endif
			.flags = 0,
			.buf = txbuf,
			.len = txlen
		},
		{
#ifdef CONFIG_MTK_I2C_EXTENSION
			.addr = ((client->addr & I2C_MASK_FLAG)
					| (I2C_ENEXT_FLAG)),
			.timing = I2C_MASTER_CLOCK,
#else
			.addr = client->addr,
#endif
			.flags = I2C_M_RD,
			.buf = rxbuf,
			.len = rxlen
		},
	};

	if (rxbuf == NULL)
		return -1;

	retry = 0;

	while ((ret = i2c_transfer(client->adapter, &msg[0], 2)) != 2) {
		retry++;

		if (retry == 5) {
			UCD_ERR(
				"Add:0x%x I2C read 0x%X length=%d ret = %d failed\n",
				client->addr, txbuf[0], rxlen, ret);
			goto EXIT;
		}
	}

EXIT:
	return ((ret == 2) ? 0 : -1);
}

static int i2c_write_bytes(struct i2c_client *client,
	u8 *txbuf, int txlen)
{
	int ret = 0;
	int retry = 3;

	do {
		ret = i2c_master_send(client, txbuf, txlen);
		if (ret != txlen) {
			UCD_ERR(
				"I2C send failed!!, Addr = 0x%x, Data = 0x%x ret=%d\n",
				txbuf[0], txbuf[1], ret);
		} else {
			break;
		}
		udelay(50);
	} while ((retry--) > 0);

	return ((ret == txlen) ? 0 : -1);
}

static int e522xx_i2c_read(u8 addr, u8 *rxbuf, int rxlen)
{
	return i2c_read_bytes(p_e522xx->client, &addr, 1, rxbuf, rxlen);
}

static int e522xx_i2c_write(u8 addr, u8 *txbuf, int txlen)
{
	u8 buff[5];
	u8 len;

	buff[0] = addr;
	if (txlen > 4) {
		UCD_ERR("e522xx_i2c_write txlen = %d", txlen);
		return -1;
	}

	memcpy(&buff[1], txbuf, txlen);
	len = txlen + 1;

	return i2c_write_bytes(p_e522xx->client, buff, len);
}

static void e522xx_en_pin_ctrl(int value)
{
	p_e522xx->debug.en_gpio_value = value;
	/* UCD_DBG("e522xx_en_pin_ctrl=%d",value); */
	gpio_set_value(p_e522xx->en_gpio_number, value);
}

static int e522xx_gpio_init(void)
{
	int ret;

	UCD_DBG("enter\n");
	ret = gpio_request_one(p_e522xx->en_gpio_number,
		GPIOF_OUT_INIT_LOW, "usb charge_en_ctrl");
	if (ret < 0) {
		UCD_ERR("Unable to request en gpio\n");
		gpio_free(p_e522xx->en_gpio_number);
		return -1;
	}

	ret = gpio_request_one(p_e522xx->int_gpio_number,
		GPIOF_IN, "usb charge_int");
	if (ret < 0) {
		UCD_ERR("Unable to request int gpio\n");
		gpio_free(p_e522xx->int_gpio_number);
		return -1;
	}

	e522xx_state_ctrl(E_E522XX_STATE_RUN);

	return 0;
}

static void e522xx_state_ctrl(int state)
{
	p_e522xx->debug.state = state;
	switch (state) {
	case E_E522XX_STATE_SLEEP:
		e522xx_en_pin_ctrl(0);
		break;
	case E_E522XX_STATE_STANDBY:
		e522xx_en_pin_ctrl(0);
		if (p_e522xx->suspend_flag)
			mdelay(1);
		else
			msleep(100);
		e522xx_en_pin_ctrl(1);
		udelay(500);
		e522xx_en_pin_ctrl(0);
		break;
	case E_E522XX_STATE_RUN:
		e522xx_en_pin_ctrl(1);
		break;
	case E_E522XX_STATE_ERROR:
		break;
	default:
		break;
	}
}

static int e522xx_req_update_port_config(enum e522xx_port_conf port_config)
{
	if (port_config >= E_E522XX_PORT_CONF_ERROR || port_config < 0)
		return -1;
	UCD_DBG("req_update_port_config [%d]", port_config);
	mutex_lock(&p_e522xx->lock);
	p_e522xx->port_config = port_config;
	p_e522xx->update_port_config = true;
	mutex_unlock(&p_e522xx->lock);

	return 0;
}

static int e522xx_clean_update_port_config(void)
{
	/* UCD_DBG("clean update port config!!!!"); */
	mutex_lock(&p_e522xx->lock);
	p_e522xx->update_port_config = false;
	mutex_unlock(&p_e522xx->lock);

	return 0;
}

static int e522xx_set_port_config(enum e522xx_port_conf port_config)
{
	int ret = 0;
	u8 buf[2] = {0};
	u8 cmd;

	if (port_config >= E_E522XX_PORT_CONF_ERROR || port_config < 0)
		return -1;

	UCD_DBG("e522xx_set_port_config [%d]", port_config);

	e522xx_state_ctrl(E_E522XX_STATE_STANDBY);

	cmd = 0x10;/* status */
	ret = e522xx_i2c_read(cmd, buf, 1);
	UCD_DBG("read status reg 0x%x value=0x%x\n", cmd, buf[0]);
#if E522XX_CTRL_CHECK_NRP
	cmd = 0x1A;/* SMPS */
	/* (>4.0A) 0x8f--nrp off   0xff--nrp 400  bit13=1(5.25v) */
	buf[0] = 0x01;
	buf[1] = 0x8f;
#else
	cmd = 0x1A;/* SMPS */
	/* (>4.0A) 0x8f--nrp off   0xff--nrp 400  bit13=1(5.25v) */
	buf[0] = 0x0d;
	buf[1] = 0xbf;
#endif
	ret = e522xx_i2c_write(cmd, buf, 2);
	if (ret)
		UCD_DBG("set smps fail1==%d\n", ret);

	switch (port_config) {
	case E_E522XX_PORT_CONF_RESERVED:
		cmd = 0x01; /* BC1.2 */
		buf[0] = 0x00;
		ret = e522xx_i2c_write(cmd, buf, 1);
		UCD_DBG("set port conf reserved mode %s\n",
			(ret != 0) ? "fail":"success");
		/*  set sleep mode */
		cmd = 0x08;
		ret = e522xx_i2c_write(cmd, buf, 0);
		UCD_DBG("set sleep mode %s\n",
			(ret != 0) ? "fail":"success");
		break;
	case E_E522XX_PORT_CONF_SDP:
		cmd = 0x01;/* BC1.2 */
		/* 0x04(SDP) 0x0c(DCP-BC1.2)  0X10(CCP FOR APPLE) 0x08(CDP) */
		buf[0] = 0x04;
		ret = e522xx_i2c_write(cmd, buf, 1);
		UCD_DBG("set port conf sdp  %s\n",
			(ret != 0) ? "fail":"success");
		break;
	case E_E522XX_PORT_CONF_CDP:
		cmd = 0x01;/* BC1.2 */
		/* 0x0c(DCP-BC1.2)  0X10(CCP FOR APPLE) 0x08(CDP) */
		buf[0] = 0x08;
		ret = e522xx_i2c_write(cmd, buf, 1);
		UCD_DBG("set port conf cdp  %s\n",
			(ret != 0) ? "fail":"success");
		break;
	case E_E522XX_PORT_CONF_DCP:
		cmd = 0x01;/* BC1.2 */
		/* 0x0c(DCP-BC1.2)  0X10(CCP FOR APPLE) 0x08(CDP)  0x14(CCP) */
		buf[0] = 0x0c;
		UCD_DBG("DCP-BC1.2\n");
		ret = e522xx_i2c_write(cmd, buf, 1);
		UCD_DBG("set port conf dcp  %s\n",
			(ret != 0) ? "fail":"success");
		break;
	case E_E522XX_PORT_CONF_CCP_APPLE:
		cmd = 0x01;/* BC1.2 */
		/* 0x0c(DCP-BC1.2)  0X10(CCP FOR APPLE) 0x08(CDP)  0x14(CCP) */
		buf[0] = 0x10;
		UCD_DBG("CCP APPLE\n");
		ret = e522xx_i2c_write(cmd, buf, 1);
		UCD_DBG("set port conf ccp apple  %s\n",
			(ret != 0) ? "fail":"success");
		break;
	case E_E522XX_PORT_CONF_CCP:
		cmd = 0x01;/* BC1.2 */
		/* 0x0c(DCP-BC1.2)  0X10(CCP FOR APPLE) 0x08(CDP)  0x14(CCP) */
		buf[0] = 0x14;
		UCD_DBG("CCP\n");
		ret = e522xx_i2c_write(cmd, buf, 1);
		UCD_DBG("set port conf ccp  %s\n",
			(ret != 0) ? "fail":"success");
		break;
	case E_E522XX_PORT_CONF_AUTO:
		cmd = 0x01;/* BC1.2 */
		/* 0x0c(DCP-BC1.2)  0X10(CCP FOR APPLE) 0x08(CDP)  0x14(CCP) */
		buf[0] = 0x1c;
		UCD_DBG("AUTO\n");
		ret = e522xx_i2c_write(cmd, buf, 1);
		UCD_DBG("set port conf auto  %s\n",
			(ret != 0) ? "fail":"success");
		break;
	default:
		break;
	}

	if (port_config != E_E522XX_PORT_CONF_RESERVED)
		e522xx_state_ctrl(E_E522XX_STATE_RUN);

	if (ret)
		UCD_DBG("e522xx_set_port_config [%d] fail", port_config);
	else
		UCD_DBG("e522xx_set_port_config [%d] success", port_config);

	return ret;
}

static int e522xx_get_port_config(void)
{
	int ret = 0;
	u8 buf[2] = {0};
	u8 cmd;
	u8 port_config;

	cmd = 0x04;
	ret = e522xx_i2c_read(cmd, buf, 1);
	if (ret == 0)
		port_config = (u8)((buf[0] >> 2) & 0x07);
	else
		port_config = 0x0;

	switch (port_config) {
	case 0x00:
		port_config = E_E522XX_PORT_CONF_RESERVED;
		break;
	case 0x01:
		port_config = E_E522XX_PORT_CONF_SDP;
		break;
	case 0x02:
		port_config = E_E522XX_PORT_CONF_CDP;
		break;
	case 0x03:
		port_config = E_E522XX_PORT_CONF_DCP;
		break;
	case 0x04:
		port_config = E_E522XX_PORT_CONF_CCP_APPLE;
		break;
	case 0x05:
		port_config = E_E522XX_PORT_CONF_CCP;
		break;
	case 0x06:
		port_config = E_E522XX_PORT_CONF_AUTO;
		break;
	default:
		port_config = E_E522XX_PORT_CONF_ERROR;
		break;
	}

	return port_config;
}

#if 0
static int e522xx_change_port_config_ccp_apple(void)
{
	int ret = 0;

	if (p_e522xx->port_config != E_E522XX_PORT_CONF_CCP_APPLE) {
		UCD_DBG("req ccp apple mode\n");
		e522xx_req_update_port_config(E_E522XX_PORT_CONF_CCP_APPLE);
	}

	return ret;
}

static int e522xx_change_port_config_cdp(void)
{
	int ret = 0;

	if (p_e522xx->port_config != E_E522XX_PORT_CONF_CDP) {
		UCD_DBG("req CDP mode\n");
		e522xx_req_update_port_config(E_E522XX_PORT_CONF_CDP);
	}

	return ret;
}

static int e522xx_change_port_config_dcp(void)
{
	int ret = 0;
	/* u8 buf[2] = {0};
	* u8 cmd;
	*/

	if (p_e522xx->port_config != E_E522XX_PORT_CONF_DCP) {
#if 0
		/* I = (i_val*33)/1280	   0x52--(2.11A)   1--25.7MA */
		cmd = 0x24;
		ret = e522xx_i2c_read(cmd, buf, 1);
		UCD_DBG("buf[0]=0x%x\n", buf[0]);
		if (buf[0] > 15) {
			UCD_DBG("req DCP mode\n");
			e522xx_req_update_port_config(E_E522XX_PORT_CONF_DCP);
		}
#else
		UCD_DBG("req DCP mode\n");
		e522xx_req_update_port_config(E_E522XX_PORT_CONF_DCP);
#endif
	}

	return ret;
}
#endif

static int e522xx_enter_sleep(void)
{
	int ret = 0;
	u8 count = 0;

	mutex_lock(&p_e522xx->lock);
	p_e522xx->port_config = E_E522XX_PORT_CONF_RESERVED;
	mutex_unlock(&p_e522xx->lock);

sleep:
	ret = e522xx_set_port_config(p_e522xx->port_config);

	if (ret) {
		UCD_DBG("e522xx_enter_sleep fail\n");
		if (count++ < 3)
			goto sleep;
	} else {
		UCD_DBG("e522xx_enter_sleep success\n");
	}

	return ret;
}

void e522xx_update_nrp(u8 VBUS_I, u8 VBUS_V)
{
	int ret;
	u8 cmd;
	static u8 VBUS_I_PREV;
	static u8 nrp_value;
	static u8 get_valid_nrp;

	u8 i = 0;
	u16 sum = 0, VBUS_I_AVER = 0;
	static u8 buf[10] = {0};
	static u8 vbus_dec_cnt;

	for (i = 0; i < 9; i++) {
		buf[i] = buf[i+1];
		sum = sum + buf[i];
	}

	buf[9] = VBUS_I_PREV;
	sum = sum + buf[9];
	VBUS_I_AVER = sum / 10;

	if (VBUS_I == 0) {
		VBUS_I_PREV = 0;
		get_valid_nrp = 0;
		if (nrp_value == 0)
			return;
		nrp_value = 0;
	} else if (VBUS_V < 0x5d) {
		VBUS_I_PREV = VBUS_I;
		get_valid_nrp = 0;
		vbus_dec_cnt = 0;
		if (nrp_value >= 7) {
			nrp_value = 7;
			return;
		}
		nrp_value++;
	} else {
		if (get_valid_nrp == 0) {
			get_valid_nrp = 2;
			vbus_dec_cnt = 0;
			if (nrp_value >= 7) {
				nrp_value = 7;
				return;
			}
			nrp_value++;
		} else {
			if (VBUS_I_PREV < (VBUS_I - 1)) {
				VBUS_I_PREV = VBUS_I;
				get_valid_nrp = 0;
				vbus_dec_cnt = 0;
				return;
			} else if ((VBUS_I_PREV > VBUS_I)
					&& (get_valid_nrp == 2)) {
				VBUS_I_PREV = VBUS_I;
				get_valid_nrp = 1;
				vbus_dec_cnt = 0;
				if (nrp_value)
					nrp_value--;
			} else if (VBUS_I_AVER > VBUS_I) {
				if (vbus_dec_cnt >= 3) {
					UCD_DBG("VBUS_I_AVER>VBUS_I\n");
					VBUS_I_PREV = VBUS_I;
					get_valid_nrp = 1;
					vbus_dec_cnt = 0;
					for (i = 0; i < 10; i++)
						buf[i] = 0;
					if (nrp_value)
						nrp_value--;
				} else {
					VBUS_I_PREV = VBUS_I;
					get_valid_nrp = 1;
					vbus_dec_cnt++;
					return;
				}
			} else {
				VBUS_I_PREV = VBUS_I;
				get_valid_nrp = 1;
				vbus_dec_cnt = 0;
				return;
			}
		}
	}

	cmd = 0x40; /* SET_NRP */
	UCD_DBG("set nrp val=0x%x\n", nrp_value);
	ret = e522xx_i2c_write(cmd, &nrp_value, 1);

	if (ret == -1)
		UCD_DBG("usb_i2c_Write fail1==%d\n", ret);
}

static int e522xx_init(void)
{
	int ret = 0;

	ret = e522xx_set_port_config(p_e522xx->port_config);

	UCD_DBG(" %s", ret == 0 ? "success":"fail");

	return ret;
}

static int e522xx_update_port_config(void)
{
	int ret = 0;
	u8 port_config;

	port_config = e522xx_get_port_config();

	UCD_DBG("get port_config=%d", port_config);

	if (port_config == E_E522XX_PORT_CONF_ERROR)
		return -1;

	if (p_e522xx->update_port_config) {
		if (port_config != p_e522xx->port_config) {
			if (!e522xx_set_port_config(p_e522xx->port_config)) {
				e522xx_clean_update_port_config();
				ret = 1;
			} else {
				ret = -1;
			}
		} else {
			e522xx_clean_update_port_config();
		}
	}

	return ret;
}
static void e522xx_set_charge_mode(void)
{
#if 0
	if (ucd_charge_device_apple())
		e522xx_req_update_port_config(E_E522XX_PORT_CONF_CCP_APPLE);
	else if (ucd_quick_charge_mode())
		e522xx_req_update_port_config(E_E522XX_PORT_CONF_DCP);
	else
		e522xx_req_update_port_config(E_E522XX_PORT_CONF_CDP);
#else
	if (ucd_quick_charge_mode()) {
		if (p_e522xx->vbus_current > 385) {
			if (ucd_charge_device_apple()) {
				e522xx_req_update_port_config(
					E_E522XX_PORT_CONF_CCP_APPLE);
			} else {
				e522xx_req_update_port_config(
					E_E522XX_PORT_CONF_DCP);
			}
		} else if (p_e522xx->vbus_current < 205) {
			e522xx_req_update_port_config(E_E522XX_PORT_CONF_SDP);
		}
	} else {
#if 0
		e522xx_req_update_port_config(E_E522XX_PORT_CONF_SDP);
#else
		e522xx_req_update_port_config(E_E522XX_PORT_CONF_CDP);
#endif
	}
#endif
}

static int e522xx_get_vbus_current(void)
{
	int ret;
	u8 i_val = 0;
	u8 cmd;

	cmd = 0x24; /* VBUS_I */
	ret = e522xx_i2c_read(cmd, &i_val, 1);

	return i_val;
}

static int e522xx_get_vbus_voltage(void)
{
	int ret;
	u8 v_val = 0;
	u8 cmd;

	cmd = 0x2A; /* VBUS_V */
	ret = e522xx_i2c_read(cmd, &v_val, 1);

	return v_val;
}

void e522xx_check_vbus_iv(void)
{
	u8 i_val = 0, v_val = 0;

	i_val = e522xx_get_vbus_current();
	v_val = e522xx_get_vbus_voltage();

	p_e522xx->vbus_current = i_val*26;
	p_e522xx->vbus_voltage = v_val*54;

	UCD_DBG("VBUS_I=%dma,VBUS_V=%dmv\n", i_val*26, v_val*54);

	if (i_val > 15)
		e522xx_set_charge_mode();
	else if (i_val < 8)
		ucd_set_device_type(E_UCD_CHARGE_DEVICE_NONE);

#if E522XX_CTRL_CHECK_NRP
	e522xx_update_nrp(i_val, v_val);
#endif
}

static void e522xx_check(void)
{
	if (p_e522xx->init_flag) {
		if (!e522xx_update_port_config())
			e522xx_check_vbus_iv();
		queue_work(ucd->workqueue, &ucd->event_work);
	} else {
		if (p_e522xx->init_count < E522XX_INIT_ERROR_COUNT) {
			p_e522xx->init_count++;
			if (!e522xx_init())
				p_e522xx->init_flag = true;
			queue_work(ucd->workqueue, &ucd->event_work);
		}
	}
}

static void ucd_check_work_func(struct work_struct *work)
{
	e522xx_check();
	if (!p_e522xx->suspend_flag) {
		queue_delayed_work(p_e522xx->workqueue, &p_e522xx->check_work,
			msecs_to_jiffies(p_e522xx->time));
	}
}

static void e522xx_suspend(void)
{
	p_e522xx->suspend_flag = true;

	cancel_work_sync(&ucd->event_work);
	cancel_delayed_work_sync(&p_e522xx->check_work);

	e522xx_enter_sleep();

	p_e522xx->init_flag = false;
	p_e522xx->init_count = 0;
	p_e522xx->vbus_current = 0;
}

static void e522xx_resume(void)
{
	p_e522xx->suspend_flag = false;
	queue_delayed_work(p_e522xx->workqueue, &p_e522xx->check_work,
		msecs_to_jiffies(10));
}

static ssize_t e522xx_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u32 len = 0;
	int ret;
	u8 in_buff[2] = {0};
	int value = 0;
	char *kbuf;

	in_buff[0] = p_e522xx->debug.reg;

	kbuf = kzalloc(BUF_SIZE, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;

	len += snprintf(kbuf + len, BUF_SIZE - len, "client->addr:%x\n",
		p_e522xx->client->addr);
	len += snprintf(kbuf + len, BUF_SIZE - len, "debug.state:%d\n",
		p_e522xx->debug.state);
	len += snprintf(kbuf + len, BUF_SIZE - len, "debug.en_gpio_value:%d\n",
		p_e522xx->debug.en_gpio_value);
	len += snprintf(kbuf + len, BUF_SIZE - len, "ime:%d\n",
		p_e522xx->time);
	len += snprintf(kbuf + len, BUF_SIZE - len, "port_config:%d\n",
		p_e522xx->port_config);
	len += snprintf(kbuf + len, BUF_SIZE - len, "init_count:%d\n",
		p_e522xx->init_count);
	len += snprintf(kbuf + len, BUF_SIZE - len, "init_flag:%d\n",
		p_e522xx->init_flag);

	if (p_e522xx->debug.reg == 0x80) {
		ret = e522xx_i2c_read(p_e522xx->debug.reg, in_buff, 2);
		value = in_buff[0]*256 + in_buff[1];
	} else if (p_e522xx->debug.reg == 0x84) {
		ret = e522xx_i2c_read(p_e522xx->debug.reg, in_buff, 2);
		value = in_buff[0]*256 + in_buff[1];
	} else {
		ret = e522xx_i2c_read(p_e522xx->debug.reg, in_buff, 1);
		value = in_buff[0];
	}
	len += snprintf(kbuf + len, BUF_SIZE - len,
		"p_e522xx->debug.reg[0x%x] = 0x%x\n",
		p_e522xx->debug.reg, value);

	strcat(buf, kbuf);

	kfree(kbuf);

	return len;
}

static ssize_t e522xx_store(struct device *dev,
	struct device_attribute *attr, char *buf, size_t count)
{
	char *pbuf = NULL;
	int val;

	pbuf = buf;
	while (pbuf) {
		if ((*pbuf == ':') || (*pbuf == '=') || (*pbuf == ' ')) {
			pbuf++;
			break;
		}
		pbuf++;
	}

	if (!strncmp(buf, "en_gpio_ctrl", 12)) {
		p_e522xx->debug.en_gpio_value = simple_strtoul(pbuf, NULL, 10);
		e522xx_en_pin_ctrl(p_e522xx->debug.en_gpio_value);
	} else if (!strncmp(buf, "state", 4)) {
		p_e522xx->debug.state = simple_strtoul(pbuf, NULL, 10);
		e522xx_state_ctrl(p_e522xx->debug.state);
	} else if (!strncmp(buf, "addr", 4)) {
		p_e522xx->client->addr = simple_strtoul(pbuf, NULL, 16);
	} else if (!strncmp(buf, "time", 4)) {
		p_e522xx->time = simple_strtoul(pbuf, NULL, 10);
	} else if (!strncmp(buf, "count", 5)) {
		p_e522xx->init_count = simple_strtoul(pbuf, NULL, 10);
	} else if (!strncmp(buf, "init", 4)) {
		val = simple_strtoul(pbuf, NULL, 10);
		if (val)
			p_e522xx->init_flag = true;
		else
			p_e522xx->init_flag = false;
	} else if (!strncmp(buf, "port", 4)) {
		val = simple_strtoul(pbuf, NULL, 10);
		if (p_e522xx->port_config != val) {
			p_e522xx->port_config = val;
			e522xx_req_update_port_config(p_e522xx->port_config);
		}
	} else if (!strncmp(buf, "sleep", 5)) {
		e522xx_enter_sleep();
	} else if (!strncmp(buf, "reg", 3)) {
		val = simple_strtoul(pbuf, NULL, 16);

		if (val > 0xffff) {
			u8 out_buff[2];

			p_e522xx->debug.reg = (val >> 16) & 0xff;
			if (p_e522xx->debug.reg == 0x1a) {
				out_buff[0] = (val & 0xff00) >> 8;
				out_buff[1] = val & 0x00ff;
				e522xx_i2c_write(p_e522xx->debug.reg,
					out_buff, 2);
			} else if (p_e522xx->debug.reg == 0x44) {
				out_buff[0] = (val & 0xff00) >> 8;
				out_buff[1] = val & 0x00ff;
				e522xx_i2c_write(p_e522xx->debug.reg,
					out_buff, 2);
			} else {
				out_buff[0] = val & 0x00ff;
				e522xx_i2c_write(p_e522xx->debug.reg,
					out_buff, 1);
			}
		} else {
			p_e522xx->debug.reg = val & 0xff;
		}
	}

	return count;
}
static DEVICE_ATTR(e522xx, 0755, e522xx_show, e522xx_store);

/* Addresses to scan */
/* static const unsigned short ucd_i2c_addr[] = {
*	0x13, 0x11,0x12,I2C_CLIENT_END };
*/
static const struct i2c_device_id ucd_i2c_id[] = { {"e522xx", 0}, {} };
#ifdef CONFIG_OF
static const struct of_device_id ucd_of_match[] = {
	{.compatible = "mediatek,e522xx"},
	{},
};
#endif

#ifdef CONFIG_OF
static int e522xx_get_of_data(struct device *dev)
{
	const struct of_device_id *match;
	enum of_gpio_flags flags1;
	enum of_gpio_flags flags2;

	UCD_DBG("enter");

	if (dev->of_node) {
		match = of_match_device(of_match_ptr(ucd_of_match), dev);
		if (!match) {
			UCD_ERR("Error: No device match found\n");
			return -ENODEV;
		}
	}

	p_e522xx->int_gpio_number = of_get_named_gpio_flags(dev->of_node,
		"int-gpio", 0, &flags1);
	p_e522xx->en_gpio_number = of_get_named_gpio_flags(dev->of_node,
		"en-gpio", 0, &flags2);
	UCD_DBG("e522xx_int_gpio_number %d flag:%d\n",
		p_e522xx->int_gpio_number, flags1);
	UCD_DBG("e522xx_en_gpio_number %d flag: %d\n",
		p_e522xx->en_gpio_number, flags2);

	return 0;
}
#else
static int e522xx_get_of_data(struct device *dev)
{
	UCD_DBG("enter e522xx_get_of_data");
	return 0;
}
#endif

static int ucd_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	UCD_DBG("Attach I2C\n");

	p_e522xx = kmalloc(sizeof(struct e522xx_data), GFP_KERNEL);
	if (!p_e522xx)
		return -ENOMEM;

	memset(p_e522xx, 0, sizeof(struct e522xx_data));

	p_e522xx->client = client;
	p_e522xx->time = E522XX_WORK_FUNC_TIME;

	mutex_init(&p_e522xx->lock);

	e522xx_get_of_data(&client->dev);

	e522xx_gpio_init();

	INIT_DELAYED_WORK(&p_e522xx->check_work, ucd_check_work_func);
	p_e522xx->workqueue = create_workqueue("ucd_check_workqueue");
	queue_delayed_work(p_e522xx->workqueue, &p_e522xx->check_work,
		msecs_to_jiffies(p_e522xx->time * 5));

	if (device_create_file(&client->dev, &dev_attr_e522xx))
		UCD_ERR("devive:%s create file error----!\n",
			dev_name(&client->dev));
	else
		UCD_DBG("devive:%s create file ok----!\n",
			dev_name(&client->dev));

	return 0;
}

static int ucd_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static int ucd_i2c_detect(struct i2c_client *client,
	struct i2c_board_info *info)
{
	UCD_DBG("%s", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;

	strlcpy(info->type, "e522xx", I2C_NAME_SIZE);

	return 0;
}

static struct i2c_driver ucd_i2c_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = ucd_i2c_probe,
	.remove = ucd_i2c_remove,
	.driver.name = "e522xx",
	.driver = {
		.name = "e522xx",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = ucd_of_match,
#endif
	},
	.id_table = ucd_i2c_id,
	.detect = ucd_i2c_detect,
	/* .address_list	= ucd_i2c_addr, */
};


static int ucd_local_init(void)
{
	UCD_INFO("%s", __func__);
	if (i2c_add_driver(&ucd_i2c_driver) != 0) {
		UCD_INFO("unable to add i2c driver.\n");
		return -1;
	}

	return 0;
}


/* Function to manage low power suspend */
static void ucd_suspend(struct device *h)
{
	UCD_DBG("%s", __func__);
	e522xx_suspend();
}

/* Function to manage power-on resume */
static void ucd_resume(struct device *h)
{
	UCD_DBG("%s", __func__);
	e522xx_resume();
}

static void ucd_event(int event, int value)
{
	char *udev_event = (char *)value;

	switch (event) {
	case E_UCD_EVENT_CHARGE_DEVICE_TYPE:
		e522xx_set_charge_mode();
		break;
	case E_UCD_EVENT_CHARGE_NORMAL_MODE:
		e522xx_set_charge_mode();
		break;
	case E_UCD_EVENT_CHARGE_QUICK_MODE:
		e522xx_set_charge_mode();
		break;
	case E_UCD_EVENT_CHARGE_GET_MODE:
		if ((p_e522xx->vbus_current > 0)) {
			if (ucd_quick_charge_mode()) {
				if (p_e522xx->port_config >=
					E_E522XX_PORT_CONF_DCP
				&& p_e522xx->port_config <=
					E_E522XX_PORT_CONF_CCP) {
					snprintf(udev_event, 64,
						"CHARGE_MODE=QUICK_CHARGE");
				} else {
					if (ucd_charge_device_none()) {
						snprintf(udev_event, 64,
							"CHARGE_MODE=NONE");
					} else {
						snprintf(udev_event, 64,
							"CHARGE_MODE=NORMAL_CHARGE");
					}
				}
			} else {
				if (ucd_charge_device_none()) {
					snprintf(udev_event, 64,
						"CHARGE_MODE=NONE");
				} else {
					snprintf(udev_event, 64,
						"CHARGE_MODE=NORMAL_CHARGE");
				}
			}
		} else {
			snprintf(udev_event, 64, "CHARGE_MODE=NONE");
		}
		break;
	case E_UCD_EVENT_CHARGE_GET_CURRENT:
		snprintf(udev_event, 64, "vbus_current=%d",
			p_e522xx->vbus_current);
		break;
	default:
		break;
	}
}
static struct ucd_driver ucd_device_driver = {
	.device_name = "e522xx",
	.local_init = ucd_local_init,
	.suspend = ucd_suspend,
	.resume = ucd_resume,
	.event = ucd_event,
};

static int __init ucd_driver_init(void)
{
	UCD_INFO("MediaTek e522xx usb charge driver init\n");

	if (ucd_driver_add(&ucd_device_driver) < 0)
		UCD_INFO("add device driver failed\n");

	return 0;
}
static void __exit ucd_driver_exit(void)
{
	ucd_driver_remove(&ucd_device_driver);
}
module_init(ucd_driver_init);
module_exit(ucd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhonghong");
MODULE_DESCRIPTION("usb charge emlos e522xx Driver");
