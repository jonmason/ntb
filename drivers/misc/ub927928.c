/*
* ub927928.c-- Serdes driver for the ds90ub927928
*
* Copyright (C) 2017 Freescale Semiconductor, Inc.
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

#include <linux/slab.h>
#include <linux/module.h>
/*#define VERBOSE_DEBUG*/
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/kthread.h>

#define DEBUG 0
#undef pr_debug
#ifdef DEBUG
#define pr_debug(format, args...) printk(format, ##args)
#else
#define pr_debug(format, args...) do {} while (0)
#endif

#define LVDS_TEST 0
#define MCU_UARTID 1
/* #define MCU_BARDRATE  B38400 */
#define LENGTH 6

const unsigned char cmd_ti927_ok[] = {0xFF, 0xAA, 0xAB, 0x01, 0x00, 0x0A};
struct i2c_client *zh_iic_dev;
static int is_927928_lock = 1;

unsigned int ti_928_regs[] = {
	53,
	57,
	58,
	59,
	68,
	69,
	73,
	86,
	100,
	101,
	102,
	103,
	110,
	111,
};

#define TI928_REGS_NUM ((sizeof(ti_928_regs))/(sizeof(ti_928_regs[0])))

static int fast_init927_write_data(struct i2c_client *client,
		u8 slave_addr, u8 register_addr, u8 data)
{
	struct i2c_msg msgs[1];
	s32 retries = 0;
	u8	test[2];
	s32 ret;

	test[0] = register_addr;
	test[1] = data;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = slave_addr;
	msgs[0].len   = 2;
	msgs[0].buf   = &test[0];

	while (retries < 2) {
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret == 1)
			return ret;
		retries++;
	}

	return -1;
}

static int fast_init927_read_data(struct i2c_client *client,
	u8 slave_addr, u8 register_addr)
{
	struct i2c_msg msgs[2];
	s32 retries = 0;
	u8	test[2];
	s32 ret;

	test[0] = register_addr;
	test[1] = 0x00;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = slave_addr;
	msgs[0].len   = 1;
	msgs[0].buf   = &test[0];

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = slave_addr;
	msgs[1].len   = 1;
	msgs[1].buf   = &test[1];

	while (retries < 2) {
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret == 2)
			return test[1];
		retries++;
		mdelay(1);
	}
	return -1;
}

#ifdef USE_926
#define i2c_send_slave_addr 0x0c /* 925 */
#define i2c_rev_slave_addr 0x2c /* 926 */
#else
#define i2c_send_slave_addr 0x0c /* 927 */
#define i2c_rev_slave_addr 0x2c /* 928 */
#endif

void ti927928_backlight(void)
{
	u16 reg[2] = {0x0d, 0x1d};
	u16 val[2] = {0x03, 0x05}; /* 927 928 transparen */
	int ret;

	msleep(200);
	ret = fast_init927_write_data(zh_iic_dev,
		i2c_send_slave_addr, reg[0], val[0]);
	pr_debug("===ti927928_gpio2_outputd 0x0e%d===\n", ret);
	ret = fast_init927_write_data(zh_iic_dev,
		i2c_rev_slave_addr, reg[1], val[1]);
	pr_debug("===ti927928_gpio2_outputd 0x1e%d===\n", ret);
}

#define i2c_touch_addr 0x14
void ti927_Init(struct i2c_client *client)
{
	int i;
	int data;
	/*0x0D,0x1D backlight control,if mcu syn success open it */
	u16 reg[] = {0x17, 0x0D, 0x0E, 0x0F, 0x18, 0x19, 0x11, 0x10};
	u16 val[] = {0x9e, 0x03, 0x53, 0x03, 0x21, 0x25, 0x03, 0x03};

	u16 reg1[] = {0x1D, 0x1E, 0x1F, 0x26, 0x27/*add*/, 0x20, 0x21};
	u16 val1[] = {0x05, 0x35, 0x05, 0x21, 0x25/*add*/, 0x90, 0x90};

	pr_debug("---start xxx ti927_Init--\r\n");

	for (i = 0; i < 10; i++) {
		data = fast_init927_read_data(client, i2c_rev_slave_addr, 0x1c);
		pr_debug("ti927_Init :0x1c: 0x%x\n", data);
		if (data == 0x03) {
			pr_debug("927,928 already lock\n");
			break;
		} else if (data == -1) {
			pr_debug("read 928 error\n");
			if (i == 9)
				return;
			break;
		}
		mdelay(1);
	}

	data = fast_init927_read_data(client, i2c_rev_slave_addr, 0x21);
	if ((data & 0xf0) == 0x90) {
		pr_debug("927,928 already init\n");
		return;
	}

	/* 927 write */
	for (i = 0; i < ARRAY_SIZE(reg); i++) {
		int ret;

		ret = fast_init927_write_data(client,
			i2c_send_slave_addr, reg[i], val[i]);
		pr_debug("===927 write reg:0x%x val:0x%x ret:%d===\n",
			reg[i], val[i], ret);
		mdelay(1);
	}

#if 0
	/* 927 read */
	for (i = 0; i < ARRAY_SIZE(reg); i++) {
		data = fast_init927_read_data(client,
			i2c_send_slave_addr, reg[i]);
		pr_debug("===927 read reg :0x%x: val:0x%x===\n", reg[i], data);
		msleep(100);
	}
#endif

	/* 928 write */
	for (i = 0; i < ARRAY_SIZE(reg1); i++) {
		int ret;

		ret = fast_init927_write_data(client,
			i2c_rev_slave_addr, reg1[i], val1[i]);
		pr_debug("===928 write reg:0x%x val:0x%x ret:%d===\n",
			reg1[i], val1[i], ret);
	}

#if 0
	/* 928 read */
	/* for(i = 0;i < ARRAY_SIZE(reg1); i++){
	*	data = fast_init927_read_data(client,
	*		i2c_rev_slave_addr, reg1[i]);
	*	pr_debug("===928 read reg :0x%x: val:0x%x===\n",reg1[i],data);
	*	msleep(100);
	*}
	*/

	pr_debug("\n---end ti927_Init---\r\n");

	ti927928_backlight();
#endif

#if 0
	struct device_node *np = NULL;

	np = of_find_compatible_node(NULL, NULL, "ti,ub927928");
	if (np == NULL) {
		pr_debug("===== read node failed=====\n");
		return;
	}

	int stb_gpio, rst_gpio;

	rst_gpio = of_get_named_gpio(np, "lcd_rst", 0);
	if (!gpio_is_valid(rst_gpio))
		return;
	pr_debug("===== lcd rst_gpio is valid=====\n");

	stb_gpio = of_get_named_gpio(np, "lcd_stb", 0);
	if (!gpio_is_valid(stb_gpio))
		return;
	pr_debug("===== lcd stb_gpio is valid=====\n");

	gpio_direction_output(rst_gpio, 1);
	gpio_direction_output(stb_gpio, 1);
#endif

}

void ti927_reInit(struct i2c_client *client)
{
	int i;
	int data;

	/* 0x0D, 0x1D backlight control, if mcu syn success open it */
	u16 reg[] = {0x17/*,0x0D*/, 0x0E, 0x0F, 0x18, 0x19, 0x11, 0x10};
	u16 val[] = {0x9e/*,0x03*/, 0x53, 0x03, 0x21, 0x25, 0x03, 0x03};

	u16 reg1[] = {/*0x1D,*/0x1E, 0x1F, 0x26, 0x27/*add*/, 0x20, 0x21};
	u16 val1[] = {/*0x05,*/0x35, 0x05, 0x21, 0x25/*add*/, 0x90, 0x90};

	pr_debug("---start xxx ti927_Init--\r\n");

	for (i = 0; i < 10; i++) {
		data = fast_init927_read_data(client,
			i2c_rev_slave_addr, 0x1c);
		pr_debug("ti927_Init :0x1c: 0x%x\n", data);
		if (data == 0x03) {
			pr_debug("927,928 already lock\n");
			break;
		} else if (data == -1) {
			pr_debug("read 928 error\n");
			if (i == 9)
				return;
			break;
		}
		mdelay(1);
	}

	/*
	* data = fast_init927_read_data(client, i2c_rev_slave_addr, 0x21);
	* if ((data & 0xf0) == 0x90) {
	*	pr_debug("927,928 already init\n");
	*	return;
	*}
	*/

	/* 927 write */
	for (i = 0; i < ARRAY_SIZE(reg); i++) {
		int ret;

		ret = fast_init927_write_data(client,
			i2c_send_slave_addr, reg[i], val[i]);
		pr_debug("===927 write reg:0x%x val:0x%x ret:%d===\n",
			reg[i], val[i], ret);
		mdelay(1);
	}

	/* 928 write */
	for (i = 0; i < ARRAY_SIZE(reg1); i++) {
		int ret;

		ret = fast_init927_write_data(client,
			i2c_rev_slave_addr, reg1[i], val1[i]);
		pr_debug("===928 write reg:0x%x val:0x%x ret:%d===\n",
			reg1[i], val1[i], ret);
	}

	/*avoid white screen*/
	msleep(500);
	fast_init927_write_data(client, i2c_send_slave_addr, 0x0D, 0x03);
	fast_init927_write_data(client, i2c_rev_slave_addr, 0x1D, 0x05);

#if 0
	struct device_node *np = NULL;
	int stb_gpio, rst_gpio;

	np = of_find_compatible_node(NULL, NULL, "ti,ub927928");
	if (np == NULL) {
		pr_debug("===== read node failed=====\n");
		return;
	}

	rst_gpio = of_get_named_gpio(np, "lcd_rst", 0);
	if (!gpio_is_valid(rst_gpio))
		return;
	pr_debug("===== lcd rst_gpio is valid=====\n");

	stb_gpio = of_get_named_gpio(np, "lcd_stb", 0);
	if (!gpio_is_valid(stb_gpio))
		return;
	pr_debug("===== lcd stb_gpio is valid=====\n");

	gpio_direction_output(rst_gpio, 1);
	gpio_direction_output(stb_gpio, 1);
#endif
}

void ti927928_gpio2_output(void)
{
	u16 reg[2] = {0x0e, 0x1e};
	u16 val[2] = {0x35, 0x53};
	int ret;

	ret = fast_init927_write_data(zh_iic_dev,
		i2c_send_slave_addr, reg[0], val[0]);
	pr_debug("===ti927928_gpio2_outputd 0x0e%d===\n", ret);
	ret = fast_init927_write_data(zh_iic_dev,
		i2c_rev_slave_addr, reg[1], val[1]);
	pr_debug("===ti927928_gpio2_outputd 0x1e%d===\n", ret);

}
/* EXPORT_SYMBOL(ti927928_gpio2_output); */

void ti927928_gpio2_input(void)
{
	u16 reg[2] = {0x0e, 0x1e};
	u16 val[2] = {0x53, 0x35};
	int ret;

	ret = fast_init927_write_data(zh_iic_dev,
		i2c_send_slave_addr, reg[0], val[0]);
	pr_debug("===ti927928_gpio2_input 0x0e %d===\n", ret);

	fast_init927_write_data(zh_iic_dev,
		i2c_rev_slave_addr, reg[1], val[1]);
	pr_debug("===ti927928_gpio2_input 0x1e %d===\n", ret);
}
/* EXPORT_SYMBOL(ti927928_gpio2_output); */

int thread_func_init(void *arg)
{
	int status_928 = 0;
	int status_928_1D = 0;

	ti927_Init(zh_iic_dev);

	while (1) {
		status_928 = fast_init927_read_data(zh_iic_dev,
			i2c_rev_slave_addr, 0x1c);
		status_928_1D = fast_init927_read_data(zh_iic_dev,
			i2c_rev_slave_addr, 0x1d);
		if ((status_928 == 0x03)
			&& (!is_927928_lock)
			&& (status_928_1D != 0x05)) {
			ti927_reInit(zh_iic_dev);
			is_927928_lock = 1;
			pr_debug(" %s:927,928 already lock\n", __func__);
		} else if ((status_928 != 0x03) ||
				(status_928 == -1) ||
				(status_928_1D != 0x25)) {
			pr_debug(" %s: 927, 928 not lock, status_928:%d, status_928_1D:%d\n",
				__func__, status_928, status_928_1D);
			is_927928_lock = 0;
		} else {
			pr_debug("aron 927928 status_928:%d\n", status_928);
		}
		msleep(1000);
	}

	return 0;
}

static int fast_ti927_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
#if 0
	ti927_Init(client);
#endif
	struct task_struct *my_thread = NULL;
	int rc = 0;

	zh_iic_dev = client;
	my_thread = kthread_run(thread_func_init, NULL, "927928init_thread");
	if (IS_ERR(my_thread)) {
		rc = PTR_ERR(my_thread);
		pr_debug("error %d create thread_name\n", rc);
	}

	return 0;
}

static int fast_ti927_remove(struct i2c_client *client)
{
	return 0;
}

static const struct  of_device_id ub927928_of_match[] = {
	{ .compatible = "ti,ub927928", },
	{},
};

MODULE_DEVICE_TABLE(of, ub927928_of_match);

static const struct i2c_device_id ub927928_id[] = {
	{ "ds90_ub927928", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ub927928_id);

static struct i2c_driver fast_ti927_driver = {
	/* .id_table = fast_ti927_device_id, */
	.driver = {
		.name = "ds90_ub927928",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ub927928_of_match),
	},
	.probe = fast_ti927_probe,
	.remove = fast_ti927_remove,
	.id_table = ub927928_id,
};

static int __init fast_ti927_init(void)
{
	return i2c_add_driver(&fast_ti927_driver);
}

/* subsys_initcall(fast_ti927_init); */
postcore_initcall_sync(fast_ti927_init);

static void __exit fast_ti927_exit(void)
{
	i2c_del_driver(&fast_ti927_driver);
}

module_exit(fast_ti927_exit);

MODULE_DESCRIPTION("Core driver for Freescale PFUZE PMIC");
MODULE_AUTHOR("Freescale Semiconductor, Inc");
MODULE_LICENSE("GPL v2");

