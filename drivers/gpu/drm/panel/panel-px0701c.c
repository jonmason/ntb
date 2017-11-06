/*
 * MIPI-DSI based px0701c1881099a LCD panel driver.
 *
 * Copyright (c) 2017 Nexell Co., Ltd
 *
 * Sungwoo Park <swpark@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <drm/drmP.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>
#include <linux/backlight.h>

#define PX0701C_WIDTH_MM		94  /* real: 94.2 */
#define PX0701C_HEIGHT_MM		150 /* real: 150.72 */

struct px0701c {
	struct device *dev;
	struct drm_panel panel;

	struct regulator_bulk_data supplies[2];
	int reset_gpio;
	int enable_gpio;
	u32 power_on_delay;
	u32 reset_delay;
	u32 init_delay;
	struct videomode vm;
	u32 width_mm;
	u32 height_mm;
	bool is_power_on;

	struct backlight_device *bl_dev;
	int error;
};

static inline struct px0701c *panel_to_px0701c(struct drm_panel *panel)
{
	return container_of(panel, struct px0701c, panel);
}

static int px0701c_clear_error(struct px0701c *ctx)
{
	int ret = ctx->error;

	ctx->error = 0;
	return ret;
}

static void _dcs_write(struct px0701c *ctx, const void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;

	if (ctx->error < 0)
		return;

	ret = mipi_dsi_dcs_write_buffer(dsi, data, len);
	if (ret < 0) {
		dev_err(ctx->dev, "error %zd writing dcs seq: %*ph\n", ret,
			(int)len, data);
		ctx->error = ret;
	}
}

static int _dcs_read(struct px0701c *ctx, u8 cmd, void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	int ret;

	if (ctx->error < 0)
		return ctx->error;

	ret = mipi_dsi_dcs_read(dsi, cmd, data, len);
	if (ret < 0) {
		dev_err(ctx->dev, "error %d reading dcs seq(%#x)\n", ret, cmd);
		ctx->error = ret;
	}

	return ret;
}

#define _dcs_write_seq_static(ctx, seq...) \
({\
	static const u8 d[] = { seq };\
	_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

static void _set_sequence(struct px0701c *ctx)
{
	//Page0
	_dcs_write_seq_static(ctx,0xE0,0x00);

	//--- PASSWORD  ----//
	_dcs_write_seq_static(ctx,0xE1,0x93);
	_dcs_write_seq_static(ctx,0xE2,0x65);
	_dcs_write_seq_static(ctx,0xE3,0xF8);
	_dcs_write_seq_static(ctx,0x80,0x03);

	//--- Sequence Ctrl  ----//
	//--- Page1  ----//
	_dcs_write_seq_static(ctx,0xE0,0x01);
	//Set VCOM
	_dcs_write_seq_static(ctx,0x00,0x00);
	_dcs_write_seq_static(ctx,0x01,0x8E);//A0
	_dcs_write_seq_static(ctx,0x03,0x00);
	_dcs_write_seq_static(ctx,0x04,0x8E);//A0

	//Set Gamma Power,1, VGMP,1,VGMN,1,VGSP,1,VGSN
	//_dcs_write_seq_static(ctx,0x0A,1,0x07);
	_dcs_write_seq_static(ctx,0x0C,0x74);

	//Set Gamma Power,1, VGMP,1,VGM1,VGSP,1,VGSN
	_dcs_write_seq_static(ctx,0x17,0x00);
	_dcs_write_seq_static(ctx,0x18,0xD8);//D7
	_dcs_write_seq_static(ctx,0x19,0x01);//00
	_dcs_write_seq_static(ctx,0x1A,0x00);
	_dcs_write_seq_static(ctx,0x1B,0xD8);
	_dcs_write_seq_static(ctx,0x1C,0x01);//00

	//Set Gate Power
	_dcs_write_seq_static(ctx,0x1F,0x74); //VGH_REG=17V   6A VGH_REG=15V
	_dcs_write_seq_static(ctx,0x20,0x19); //VGL_REG=-8V   23 VGL_REG=-10V
	_dcs_write_seq_static(ctx,0x21,0x19); //VGL_REG2=-8V  23 VGL_REG2=-10V
	_dcs_write_seq_static(ctx,0x22,0x0E);

	//SET RGBCYC
	_dcs_write_seq_static(ctx,0x37,0x29); //SS=1,1, BGR=1
	_dcs_write_seq_static(ctx,0x38,0x05); //JDT=101 Zig-zag
	_dcs_write_seq_static(ctx,0x39,0x08); //RGB_N_EQ1,1, modify 20140806
	_dcs_write_seq_static(ctx,0x3A,0x18); //12    modify 15/05/06 RGB_N_EQ2,1, modify 20140806
	_dcs_write_seq_static(ctx,0x3B,0x18); //modify 15/05/06
	_dcs_write_seq_static(ctx,0x3C,0x72); //78    modify 15/05/06 SET EQ3 for TE_H
	_dcs_write_seq_static(ctx,0x3D,0xFF); //SET CHGEN_ON,1, modify 20140827
	_dcs_write_seq_static(ctx,0x3E,0xFF); //SET CHGEN_OFF,1, modify 20140827
	_dcs_write_seq_static(ctx,0x3F,0xFF); //SET CHGEN_OFF2,1, modify 20140827

	//Set TCON
	_dcs_write_seq_static(ctx,0x40,0x06); //RSO 04h=720,1, 05h=768,1, 06h=800
	_dcs_write_seq_static(ctx,0x41,0xA0); //LN=640->1280 line
	_dcs_write_seq_static(ctx,0x43,0x10); //VFP
	_dcs_write_seq_static(ctx,0x44,0x0E); //VBP
	_dcs_write_seq_static(ctx,0x45,0x3C); //HBP

	//--- power voltage  ----//
	_dcs_write_seq_static(ctx,0x55,0x01); //DCDCM=0011,1, HX PWR_IC
	_dcs_write_seq_static(ctx,0x56,0x01);
	_dcs_write_seq_static(ctx,0x57,0x64); //65    modify 15/05/06
	_dcs_write_seq_static(ctx,0x58,0x0A); //AVDD
	_dcs_write_seq_static(ctx,0x59,0x0A); //VCL &AVEE
	_dcs_write_seq_static(ctx,0x5A,0x28); //VGH ,1,15V
	_dcs_write_seq_static(ctx,0x5B,0x10); //VGL,1,-10V

	//--- Gamma  ----//
	_dcs_write_seq_static(ctx,0x5D,0x7C); //70
	_dcs_write_seq_static(ctx,0x5E,0x60);  //56
	_dcs_write_seq_static(ctx,0x5F,0x4E); //45
	_dcs_write_seq_static(ctx,0x60,0x40); //39
	_dcs_write_seq_static(ctx,0x61,0x39);  //35
	_dcs_write_seq_static(ctx,0x62,0x28);  //26
	_dcs_write_seq_static(ctx,0x63,0x2A);  //2A
	_dcs_write_seq_static(ctx,0x64,0x11);  //14
	_dcs_write_seq_static(ctx,0x65,0x27);  //2E
	_dcs_write_seq_static(ctx,0x66,0x23);  //2D
	_dcs_write_seq_static(ctx,0x67,0x21);  //2D
	_dcs_write_seq_static(ctx,0x68,0x3D);  //4C
	_dcs_write_seq_static(ctx,0x69,0x2B);  //3A
	_dcs_write_seq_static(ctx,0x6A,0x33);  //45
	_dcs_write_seq_static(ctx,0x6B,0x26);  //38
	_dcs_write_seq_static(ctx,0x6C,0x24);  //36
	_dcs_write_seq_static(ctx,0x6D,0x18);  //2B
	_dcs_write_seq_static(ctx,0x6E,0x0A);  //1C
	_dcs_write_seq_static(ctx,0x6F,0x00);  //00

	_dcs_write_seq_static(ctx,0x70,0x7C);  //70
	_dcs_write_seq_static(ctx,0x71,0x60);  //56
	_dcs_write_seq_static(ctx,0x72,0x4E);  //45
	_dcs_write_seq_static(ctx,0x73,0x40);  //39
	_dcs_write_seq_static(ctx,0x74,0x39);  //35
	_dcs_write_seq_static(ctx,0x75,0x29);  //26
	_dcs_write_seq_static(ctx,0x76,0x2B);  //2A
	_dcs_write_seq_static(ctx,0x77,0x12);  //14
	_dcs_write_seq_static(ctx,0x78,0x27);  //2E
	_dcs_write_seq_static(ctx,0x79,0x24);  //2D
	_dcs_write_seq_static(ctx,0x7A,0x21);  //2D
	_dcs_write_seq_static(ctx,0x7B,0x3E);  //4C
	_dcs_write_seq_static(ctx,0x7C,0x2B);  //3A
	_dcs_write_seq_static(ctx,0x7D,0x33);  //45
	_dcs_write_seq_static(ctx,0x7E,0x26);  //38
	_dcs_write_seq_static(ctx,0x7F,0x24);  //36
	_dcs_write_seq_static(ctx,0x80,0x19);  //2B
	_dcs_write_seq_static(ctx,0x81,0x0A);  //1C
	_dcs_write_seq_static(ctx,0x82,0x00);  //00

	//Page2,1, for GIP
	_dcs_write_seq_static(ctx,0xE0,0x02);
	//GIP_L Pin mapping
	_dcs_write_seq_static(ctx,0x00,0x0E); //L1/CK2BO/CKV10
	_dcs_write_seq_static(ctx,0x01,0x06);  //L2/CK2O/CKV2
	_dcs_write_seq_static(ctx,0x02,0x0C);  //L3/CK1BO/CKV8
	_dcs_write_seq_static(ctx,0x03,0x04);  //L4/CK1O/CKV0
	_dcs_write_seq_static(ctx,0x04,0x08);  //L5/CK3O/CKV4
	_dcs_write_seq_static(ctx,0x05,0x19);  //L6/CK3BO/CKV2
	_dcs_write_seq_static(ctx,0x06,0x0A);  //L7/CK4O/CKV6
	_dcs_write_seq_static(ctx,0x07,0x1B);  //L8/CK4BO/CKV14
	_dcs_write_seq_static(ctx,0x08,0x00);  //L9/STVO/STV0
	_dcs_write_seq_static(ctx,0x09,0x1D);  //L10
	_dcs_write_seq_static(ctx,0x0A,0x1F);  //L11/VGL
	_dcs_write_seq_static(ctx,0x0B,0x1F);  //L12/VGL
	_dcs_write_seq_static(ctx,0x0C,0x1D);  //L13
	_dcs_write_seq_static(ctx,0x0D,0x1D);  //L14
	_dcs_write_seq_static(ctx,0x0E,0x1D);  //L15
	_dcs_write_seq_static(ctx,0x0F,0x17);  //L16/V1_O/FLM1
	_dcs_write_seq_static(ctx,0x10,0x37);  //L17/V2_O/FLM1_INV
	_dcs_write_seq_static(ctx,0x11,0x1D);  //L18
	_dcs_write_seq_static(ctx,0x12,0x1F);  //L19/BW/VGL
	_dcs_write_seq_static(ctx,0x13,0x1E);  //L20/FW/VGH
	_dcs_write_seq_static(ctx,0x14,0x10);  //L21/RST_O/ETV0
	_dcs_write_seq_static(ctx,0x15,0x1D);  //L22

	//GIP_R Pin mapping
	_dcs_write_seq_static(ctx,0x16,0x0F);  //R1/CK2BE/CKV11
	_dcs_write_seq_static(ctx,0x17,0x07);  //R2/CK2E/CKV3
	_dcs_write_seq_static(ctx,0x18,0x0D);  //R3/CK1BE/CKV9
	_dcs_write_seq_static(ctx,0x19,0x05);  //R4/CK1E/CKV1
	_dcs_write_seq_static(ctx,0x1A,0x09);  //R5/CK3E/CKV5
	_dcs_write_seq_static(ctx,0x1B,0x1A);  //R6/CK3BE/CKV13
	_dcs_write_seq_static(ctx,0x1C,0x0B);  //R7/CK4E/CKV7
	_dcs_write_seq_static(ctx,0x1D,0x1C);  //R8/CK4BE/CKV15
	_dcs_write_seq_static(ctx,0x1E,0x01);  //R9/STVE/STV1
	_dcs_write_seq_static(ctx,0x1F,0x1D);  //R10
	_dcs_write_seq_static(ctx,0x20,0x1F);  //R11/VGL
	_dcs_write_seq_static(ctx,0x21,0x1F);  //R12/VGL
	_dcs_write_seq_static(ctx,0x22,0x1D);  //R13
	_dcs_write_seq_static(ctx,0x23,0x1D);  //R14
	_dcs_write_seq_static(ctx,0x24,0x1D);  //R15
	_dcs_write_seq_static(ctx,0x25,0x18);  //R16/V1_E/FLM2
	_dcs_write_seq_static(ctx,0x26,0x38);  //R17/V2_E/FLM2_INV
	_dcs_write_seq_static(ctx,0x27,0x1D);  //R18
	_dcs_write_seq_static(ctx,0x28,0x1F);  //R19/BW/VGL
	_dcs_write_seq_static(ctx,0x29,0x1E);  //R20/FW/VGH
	_dcs_write_seq_static(ctx,0x2A,0x11);  //R21/RST_E/ETV1
	_dcs_write_seq_static(ctx,0x2B,0x1D);  //R22

	//GIP_L_GS Pin mapping
	_dcs_write_seq_static(ctx,0x2C,0x09);  //L1/CK2BO/CKV5
	_dcs_write_seq_static(ctx,0x2D,0x1A);  //L2/CK2O/CKV13
	_dcs_write_seq_static(ctx,0x2E,0x0B);  //L3/CK1BO/CKV7
	_dcs_write_seq_static(ctx,0x2F,0x1C);  //L4/CK1O/CKV15
	_dcs_write_seq_static(ctx,0x30,0x0F);  //L5/CK3O/CKV11
	_dcs_write_seq_static(ctx,0x31,0x07);  //L6/CK3BO/CKV3
	_dcs_write_seq_static(ctx,0x32,0x0D);  //L7/CK4O/CKV9
	_dcs_write_seq_static(ctx,0x33,0x05);  //L8/CK4BO/CKV1
	_dcs_write_seq_static(ctx,0x34,0x11);  //L9/STVO/ETV1
	_dcs_write_seq_static(ctx,0x35,0x1D);  //L10
	_dcs_write_seq_static(ctx,0x36,0x1F);  //L11/VGL
	_dcs_write_seq_static(ctx,0x37,0x1F);  //L12/VGL
	_dcs_write_seq_static(ctx,0x38,0x1D);  //L13
	_dcs_write_seq_static(ctx,0x39,0x1D);  //L14
	_dcs_write_seq_static(ctx,0x3A,0x1D);  //L15
	_dcs_write_seq_static(ctx,0x3B,0x18);  //L16/V1_O/FLM2
	_dcs_write_seq_static(ctx,0x3C,0x38);  //L17/V2_O/?
	_dcs_write_seq_static(ctx,0x3D,0x1D);  //L18
	_dcs_write_seq_static(ctx,0x3E,0x1E);  //L19/BW/VGH
	_dcs_write_seq_static(ctx,0x3F,0x1F);  //L20/FW/VGL
	_dcs_write_seq_static(ctx,0x40,0x01);  //L21/RST_O/STV1
	_dcs_write_seq_static(ctx,0x41,0x1D);  //L22

	//GIP_R_GS Pin mapping
	_dcs_write_seq_static(ctx,0x42,0x08);  //R1/CK2BE/CKV4
	_dcs_write_seq_static(ctx,0x43,0x19);  //R2/CK2E/CKV12
	_dcs_write_seq_static(ctx,0x44,0x0A);  //R3/CK1BE/CKV6
	_dcs_write_seq_static(ctx,0x45,0x1B);  //R4/CK1E/CKV14
	_dcs_write_seq_static(ctx,0x46,0x0E);  //R5/CK3E/CKV10
	_dcs_write_seq_static(ctx,0x47,0x06);  //R6/CK3BE/CKV2
	_dcs_write_seq_static(ctx,0x48,0x0C);  //R7/CK4E/CKV8
	_dcs_write_seq_static(ctx,0x49,0x04);  //R8/CK4BE/CKV0
	_dcs_write_seq_static(ctx,0x4A,0x10);  //R9/STVE/ETV0
	_dcs_write_seq_static(ctx,0x4B,0x1D);  //R10
	_dcs_write_seq_static(ctx,0x4C,0x1F);  //R11/VGL
	_dcs_write_seq_static(ctx,0x4D,0x1F);  //R12/VGL
	_dcs_write_seq_static(ctx,0x4E,0x1D);  //R13
	_dcs_write_seq_static(ctx,0x4F,0x1D);  //R14
	_dcs_write_seq_static(ctx,0x50,0x1D);  //R15
	_dcs_write_seq_static(ctx,0x51,0x17);  //R16/V1_E/FLM1
	_dcs_write_seq_static(ctx,0x52,0x37);  //R17/V2_E/?
	_dcs_write_seq_static(ctx,0x53,0x1D);  //R18
	_dcs_write_seq_static(ctx,0x54,0x1E);  //R19/BW/VGH
	_dcs_write_seq_static(ctx,0x55,0x1F);  //R20/FW/VGL
	_dcs_write_seq_static(ctx,0x56,0x00);  //R21/RST_E/STV0
	_dcs_write_seq_static(ctx,0x57,0x1D);  //R22

	//GIP Timing
	_dcs_write_seq_static(ctx,0x58,0x10);
	_dcs_write_seq_static(ctx,0x59,0x00);
	_dcs_write_seq_static(ctx,0x5A,0x00);
	_dcs_write_seq_static(ctx,0x5B,0x10);
	_dcs_write_seq_static(ctx,0x5C,0x00);  //01
	_dcs_write_seq_static(ctx,0x5D,0xD0);  //50
	_dcs_write_seq_static(ctx,0x5E,0x01);
	_dcs_write_seq_static(ctx,0x5F,0x02);
	_dcs_write_seq_static(ctx,0x60,0x60);
	_dcs_write_seq_static(ctx,0x61,0x01);
	_dcs_write_seq_static(ctx,0x62,0x02);
	_dcs_write_seq_static(ctx,0x63,0x06);
	_dcs_write_seq_static(ctx,0x64,0x6A);
	_dcs_write_seq_static(ctx,0x65,0x55);
	_dcs_write_seq_static(ctx,0x66,0x0F);   //2C
	_dcs_write_seq_static(ctx,0x67,0xF7);  //73
	_dcs_write_seq_static(ctx,0x68,0x08);  //05
	_dcs_write_seq_static(ctx,0x69,0x08);
	_dcs_write_seq_static(ctx,0x6A,0x6A);  //66_by Max_20151029
	_dcs_write_seq_static(ctx,0x6B,0x10); //dummy clk
	_dcs_write_seq_static(ctx,0x6C,0x00);
	_dcs_write_seq_static(ctx,0x6D,0x00);
	_dcs_write_seq_static(ctx,0x6E,0x00);
	_dcs_write_seq_static(ctx,0x6F,0x88);
	_dcs_write_seq_static(ctx,0x70,0x00);  //00
	_dcs_write_seq_static(ctx,0x71,0x17);  //00
	_dcs_write_seq_static(ctx,0x72,0x06);
	_dcs_write_seq_static(ctx,0x73,0x7B);
	_dcs_write_seq_static(ctx,0x74,0x00);
	_dcs_write_seq_static(ctx,0x75,0x80);  //80
	_dcs_write_seq_static(ctx,0x76,0x01);
	_dcs_write_seq_static(ctx,0x77,0x5D);  //0D
	_dcs_write_seq_static(ctx,0x78,0x18);  //18
	_dcs_write_seq_static(ctx,0x79,0x00);
	_dcs_write_seq_static(ctx,0x7A,0x00);
	_dcs_write_seq_static(ctx,0x7B,0x00);
	_dcs_write_seq_static(ctx,0x7C,0x00);
	_dcs_write_seq_static(ctx,0x7D,0x03);
	_dcs_write_seq_static(ctx,0x7E,0x7B);

	//Page4
	_dcs_write_seq_static(ctx,0xE0,0x04);
	//_dcs_write_seq_static(ctx,0x04,0x01);   //00 modify 15/05/06
	_dcs_write_seq_static(ctx,0x09,0x10); // modify 15/05/06
	_dcs_write_seq_static(ctx,0x0E,0x38); // modify 15/05/06

	//ESD Check & lane number
	_dcs_write_seq_static(ctx,0x2B,0x2B);
	_dcs_write_seq_static(ctx,0x2D,0x03);
	_dcs_write_seq_static(ctx,0x2E,0x44);

	//Page0
	_dcs_write_seq_static(ctx,0xE0,0x00);

	//Watch dog
	_dcs_write_seq_static(ctx,0xE6,0x02);
	_dcs_write_seq_static(ctx,0xE7,0x06);
	//_dcs_write_seq_static(ctx,0x11,0x0);
	_dcs_write_seq_static(ctx,0x11);
	mdelay(200);

	//_dcs_write_seq_static(ctx,0x29,0x0);
	_dcs_write_seq_static(ctx,0x29);
	mdelay(20);
	/* all pixel on */
	//_dcs_write_seq_static(ctx, 0x23);
}

static int px0701c_power_on(struct px0701c *ctx)
{
	int ret;

#ifndef CONFIG_DRM_CHECK_PRE_INIT

	if (ctx->is_power_on)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0)
		return ret;

	msleep(ctx->power_on_delay);

	gpio_direction_output(ctx->reset_gpio, 1);
	gpio_set_value(ctx->reset_gpio, 0);
	msleep(ctx->reset_delay);
	gpio_set_value(ctx->reset_gpio, 1);
	msleep(ctx->init_delay);
#endif

	ctx->is_power_on = true;

	return 0;
}

static int px0701c_power_off(struct px0701c *ctx)
{
	if (!ctx->is_power_on)
		return 0;

	/* gpio_set_value(ctx->reset_gpio, 0); */
	/* usleep_range(5000, 6000); */
        /*  */
	/* regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies); */
	/* ctx->is_power_on = false; */

	return 0;
}

static int px0701c_enable(struct drm_panel *panel)
{

	struct px0701c *ctx = panel_to_px0701c(panel);

	if (ctx->bl_dev) {
		ctx->bl_dev->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->bl_dev);
	}

	if (ctx->enable_gpio > 0) {
		gpio_direction_output(ctx->enable_gpio, 1);
		gpio_set_value(ctx->enable_gpio, 1);
	}

	return 0;
}

static int px0701c_disable(struct drm_panel *panel)
{
	struct px0701c *ctx = panel_to_px0701c(panel);

	if (ctx->bl_dev) {
		ctx->bl_dev->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->bl_dev);
	}

	if (ctx->enable_gpio > 0)
		gpio_set_value(ctx->enable_gpio, 0);

	return 0;
}

static int px0701c_prepare(struct drm_panel *panel)
{
	struct px0701c *ctx = panel_to_px0701c(panel);
	int ret;

	ret = px0701c_power_on(ctx);
	if (ret < 0) {
		dev_err(ctx->dev, "failed to power on\n");
		return ret;
	}

#ifndef CONFIG_DRM_CHECK_PRE_INIT
	_set_sequence(ctx);
	ret = ctx->error;
	if (ret < 0) {
		dev_err(ctx->dev, "failed to set_sequence\n");
		return ret;
	}
#endif

	return 0;
}

static int px0701c_unprepare(struct drm_panel *panel)
{
	struct px0701c *ctx = panel_to_px0701c(panel);

	return px0701c_power_off(ctx);
}

static const struct drm_display_mode default_mode = {
	.clock = 67161,			/* KHz */
	.hdisplay = 800,
	.hsync_start = 800 + 40,	/* hactive + hbackporch */
	.hsync_end = 800 + 40 + 20,	/* hsync_start + hsyncwidth */
	.htotal = 800 + 40 + 20 + 40,	/* hsync_end + hfrontporch */
	.vdisplay = 1280,
	.vsync_start = 1280 + 6,	/* vactive + vbackporch */
	.vsync_end = 1280 + 6 + 9,	/* vsync_start + vsyncwidth */
	.vtotal = 1280 + 6 + 9 + 16,	/* vsync_end + vfrontporch */
	.vrefresh = 60,			/* Hz */
};

/**
 * HACK
 * return value
 * 1: success
 * 0: failure
 */
static int px0701c_get_modes(struct drm_panel *panel)
{
	struct drm_connector *connector = panel->connector;
	struct px0701c *ctx = panel_to_px0701c(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("failed to create a new display mode\n");
		return 0;
	}

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	drm_mode_set_name(mode);
	mode->width_mm = ctx->width_mm;
	mode->height_mm = ctx->height_mm;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs px0701c_drm_funcs = {
	.enable = px0701c_enable,
	.disable = px0701c_disable,
	.prepare = px0701c_prepare,
	.unprepare = px0701c_unprepare,
	.get_modes = px0701c_get_modes,
};

static int px0701c_parse_dt(struct px0701c *ctx)
{
	struct device *dev = ctx->dev;
	struct device_node *np = dev->of_node;

	of_property_read_u32(np, "power-on-delay", &ctx->power_on_delay);
	of_property_read_u32(np, "reset-delay", &ctx->reset_delay);
	of_property_read_u32(np, "init-delay", &ctx->init_delay);

	return 0;
}

static int px0701c_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *backlight;
	struct px0701c *ctx;
	int ret;

	if (!drm_panel_connected("px0701c"))
		return -ENODEV;

	ctx = devm_kzalloc(dev, sizeof(struct px0701c), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;

	ctx->is_power_on = false;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO
		| MIPI_DSI_MODE_VIDEO_HFP | MIPI_DSI_MODE_VIDEO_HBP
		| MIPI_DSI_MODE_VIDEO_HSA | MIPI_DSI_MODE_VSYNC_FLUSH;
//	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = px0701c_parse_dt(ctx);
	if (ret)
		return ret;

	/* TODO: mapping real power name */
	ctx->supplies[0].supply = "vdd3";
	ctx->supplies[1].supply = "vci";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0)
		dev_warn(dev, "failed to get regulators: %d\n", ret);

	ctx->reset_gpio = of_get_named_gpio(dev->of_node, "reset-gpio", 0);
	if (ctx->reset_gpio < 0) {
		dev_err(dev, "cannot get reset-gpio %d\n", ctx->reset_gpio);
		return -EINVAL;
	}

	ret = devm_gpio_request(dev, ctx->reset_gpio, "reset-gpio");
	if (ret) {
		dev_err(dev, "failed to request reset-gpio\n");
		return ret;
	}

	ctx->enable_gpio = of_get_named_gpio(dev->of_node, "enable-gpio", 0);
	if (ctx->enable_gpio < 0)
		dev_warn(dev, "cannot get enable-gpio %d\n", ctx->enable_gpio);
	else {
		ret = devm_gpio_request(dev, ctx->enable_gpio, "enable-gpio");
		if (ret) {
			dev_err(dev, "failed to request enable-gpio\n");
			return ret;
		}
	}

	backlight = of_parse_phandle(dev->of_node, "backlight", 0);
	if (backlight) {
		ctx->bl_dev = of_find_backlight_by_node(backlight);
		of_node_put(backlight);

		if (!ctx->bl_dev)
			return -EPROBE_DEFER;
	}

	ctx->width_mm = PX0701C_WIDTH_MM;
	ctx->height_mm = PX0701C_HEIGHT_MM;

	drm_panel_init(&ctx->panel);
	ctx->panel.dev = dev;
	ctx->panel.funcs = &px0701c_drm_funcs;

	ret = drm_panel_add(&ctx->panel);
	if (ret < 0) {
		backlight_device_unregister(ctx->bl_dev);
		return ret;
	}

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		backlight_device_unregister(ctx->bl_dev);
		drm_panel_remove(&ctx->panel);
	}

	return ret;
}

static int px0701c_remove(struct mipi_dsi_device *dsi)
{
	struct px0701c *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
	backlight_device_unregister(ctx->bl_dev);
	px0701c_power_off(ctx);

	return 0;
}

static void px0701c_shutdown(struct mipi_dsi_device *dsi)
{
	struct px0701c *ctx = mipi_dsi_get_drvdata(dsi);

	px0701c_power_off(ctx);
}

static const struct of_device_id px0701c_of_match[] = {
	{ .compatible = "px0701c" },
	{ }
};
MODULE_DEVICE_TABLE(of, px0701c_of_match);

static struct mipi_dsi_driver px0701c_driver = {
	.probe = px0701c_probe,
	.remove = px0701c_remove,
	.shutdown = px0701c_shutdown,
	.driver = {
		.name = "panel-px0701c",
		.of_match_table = px0701c_of_match,
	},
};

module_mipi_dsi_driver(px0701c_driver);

MODULE_AUTHOR("Sungwoo Park <swpark@nexell.co.kr>");
MODULE_DESCRIPTION("MIPI-SDI based px0701c series LCD Panel Driver");
MODULE_LICENSE("GPL v2");
