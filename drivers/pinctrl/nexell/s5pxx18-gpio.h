/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Bon-gyu, KOO <freestyle@nexell.co.kr>
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

#ifndef __S5Pxx18_GPIO_H
#define __S5Pxx18_GPIO_H

#define PAD_MD_POS 0
#define PAD_MD_MASK 0xF

#define PAD_FN_POS 4
#define PAD_FN_MASK 0xF

#define PAD_LV_POS 8
#define PAD_LV_MASK 0xF

#define PAD_PU_POS 12
#define PAD_PU_MASK 0xF

#define PAD_ST_POS 16
#define PAD_ST_MASK 0xF

#define PIN_MODE_ALT (0)
#define PIN_MODE_IN (1)
#define PIN_MODE_OUT (2)
#define PIN_MODE_INT (3)

#define PIN_FUNC_ALT0 (0)
#define PIN_FUNC_ALT1 (1)
#define PIN_FUNC_ALT2 (2)
#define PIN_FUNC_ALT3 (3)

#define PIN_LEVEL_LOW (0)		/* if alive, async lowlevel */
#define PIN_LEVEL_HIGH (1)		/* if alive, async highlevel */
#define PIN_LEVEL_FALLINGEDGE (2)	/* if alive, async fallingedge */
#define PIN_LEVEL_RISINGEDGE (3)	/* if alive, async eisingedge */
#define PIN_LEVEL_LOW_SYNC (4)		/* if gpio , not support */
#define PIN_LEVEL_HIGH_SYNC (5)		/* if gpio , not support */
#define PIN_LEVEL_BOTHEDGE (4)		/* if alive, not support */
#define PIN_LEVEL_ALT (6)		/* if pad function is alt, not set */

#define PIN_PULL_DN (0)			/* Do not support Alive-GPIO */
#define PIN_PULL_UP (1)
#define PIN_PULL_OFF (2)

#define PIN_STRENGTH_0 (0)
#define PIN_STRENGTH_1 (1)
#define PIN_STRENGTH_2 (2)
#define PIN_STRENGTH_3 (3)

#define PAD_MODE_ALT ((PIN_MODE_ALT & PAD_MD_MASK) << PAD_MD_POS)
#define PAD_MODE_IN ((PIN_MODE_IN & PAD_MD_MASK) << PAD_MD_POS)
#define PAD_MODE_OUT ((PIN_MODE_OUT & PAD_MD_MASK) << PAD_MD_POS)
#define PAD_MODE_INT ((PIN_MODE_INT & PAD_MD_MASK) << PAD_MD_POS)

#define PAD_FUNC_ALT0 ((PIN_FUNC_ALT0 & PAD_FN_MASK) << PAD_FN_POS)
#define PAD_FUNC_ALT1 ((PIN_FUNC_ALT1 & PAD_FN_MASK) << PAD_FN_POS)
#define PAD_FUNC_ALT2 ((PIN_FUNC_ALT2 & PAD_FN_MASK) << PAD_FN_POS)
#define PAD_FUNC_ALT3 ((PIN_FUNC_ALT3 & PAD_FN_MASK) << PAD_FN_POS)

#define PAD_LEVEL_LOW                                                          \
	((PIN_LEVEL_LOW & PAD_LV_MASK)                                         \
	 << PAD_LV_POS) /* if alive, async lowlevel */
#define PAD_LEVEL_HIGH                                                         \
	((PIN_LEVEL_HIGH & PAD_LV_MASK)                                        \
	 << PAD_LV_POS) /* if alive, async highlevel */
#define PAD_LEVEL_FALLINGEDGE                                                  \
	((PIN_LEVEL_FALLINGEDGE & PAD_LV_MASK)                                 \
	 << PAD_LV_POS) /* if alive, async fallingedge */
#define PAD_LEVEL_RISINGEDGE                                                   \
	((PIN_LEVEL_RISINGEDGE & PAD_LV_MASK)                                  \
	 << PAD_LV_POS) /* if alive, async eisingedge */
#define PAD_LEVEL_LOW_SYNC                                                     \
	((PIN_LEVEL_LOW_SYNC & PAD_LV_MASK)                                    \
	 << PAD_LV_POS) /* if gpio , not support */
#define PAD_LEVEL_HIGH_SYNC                                                    \
	((PIN_LEVEL_HIGH_SYNC & PAD_LV_MASK)                                   \
	 << PAD_LV_POS) /* if gpio , not support */
#define PAD_LEVEL_BOTHEDGE                                                     \
	((PIN_LEVEL_BOTHEDGE & PAD_LV_MASK)                                    \
	 << PAD_LV_POS) /* if alive, not support */
#define PAD_LEVEL_ALT                                                          \
	((PIN_LEVEL_ALT & PAD_LV_MASK)                                         \
	 << PAD_LV_POS) /* if pad function is alt, not set */

#define PAD_PULL_DN                                                            \
	((PIN_PULL_DN & PAD_PU_MASK)                                           \
	 << PAD_PU_POS) /* Do not support Alive-GPIO */
#define PAD_PULL_UP ((PIN_PULL_UP & PAD_PU_MASK) << PAD_PU_POS)
#define PAD_PULL_OFF ((PIN_PULL_OFF & PAD_PU_MASK) << PAD_PU_POS)

#define PAD_STRENGTH_0 ((PIN_STRENGTH_0 & PAD_ST_MASK) << PAD_ST_POS)
#define PAD_STRENGTH_1 ((PIN_STRENGTH_1 & PAD_ST_MASK) << PAD_ST_POS)
#define PAD_STRENGTH_2 ((PIN_STRENGTH_2 & PAD_ST_MASK) << PAD_ST_POS)
#define PAD_STRENGTH_3 ((PIN_STRENGTH_3 & PAD_ST_MASK) << PAD_ST_POS)

#define PAD_GET_GROUP(pin) ((pin >> 0x5) & 0x07) /* Divide 32 */
#define PAD_GET_BITNO(pin) (pin & 0x1F)
#define PAD_GET_FUNC(pin) ((pin >> PAD_FN_POS) & PAD_FN_MASK)
#define PAD_GET_MODE(pin) ((pin >> PAD_MD_POS) & PAD_MD_MASK)
#define PAD_GET_LEVEL(pin) ((pin >> PAD_LV_POS) & PAD_LV_MASK)
#define PAD_GET_PULLUP(pin) ((pin >> PAD_PU_POS) & PAD_PU_MASK)
#define PAD_GET_STRENGTH(pin) ((pin >> PAD_ST_POS) & PAD_ST_MASK)

#define PAD_GPIO_A (0 * 32)
#define PAD_GPIO_B (1 * 32)
#define PAD_GPIO_C (2 * 32)
#define PAD_GPIO_D (3 * 32)
#define PAD_GPIO_E (4 * 32)
#define PAD_GPIO_ALV (5 * 32)

/*
 * gpio descriptor
 */
#define IO_ALT_0 (0)
#define IO_ALT_1 (1)
#define IO_ALT_2 (2)
#define IO_ALT_3 (3)

/* s5pxx18 GPIO function number */

#define ALT_NO_GPIO_A                                                          \
	{                                                                      \
		IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,    \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0,                                                  \
	}

#define ALT_NO_GPIO_B                                                          \
	{                                                                      \
		IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,    \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_2, IO_ALT_2, IO_ALT_1, IO_ALT_2, IO_ALT_1,          \
		    IO_ALT_2, IO_ALT_1, IO_ALT_2, IO_ALT_1, IO_ALT_1,          \
		    IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1,          \
		    IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1,          \
		    IO_ALT_1,                                                  \
	}

#define ALT_NO_GPIO_C                                                          \
	{                                                                      \
		IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1,    \
		    IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1,          \
		    IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1,          \
		    IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1,          \
		    IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1,          \
		    IO_ALT_1, IO_ALT_1, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0,                                                  \
	}

#define ALT_NO_GPIO_D                                                          \
	{                                                                      \
		IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,    \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0,                                                  \
	}

#define ALT_NO_GPIO_E                                                          \
	{                                                                      \
		IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,    \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_1,          \
		    IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1, IO_ALT_1,          \
		    IO_ALT_1,                                                  \
	}

#define ALT_NO_ALIVE                                                           \
	{                                                                      \
		IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,    \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0, IO_ALT_0,          \
		    IO_ALT_0,                                                  \
	}

#define GPIO_NUM_PER_BANK (32)

#define NUMBER_OF_GPIO_MODULE 5

#define nx_gpio_intmode_lowlevel 1
#define nx_gpio_intmode_highlevel 2
#define nx_gpio_intmode_fallingedge 2
#define nx_gpio_intmode_risingedge 3
#define nx_gpio_intmode_bothedge 4

#define nx_gpio_padfunc_0 0
#define nx_gpio_padfunc_1 1
#define nx_gpio_padfunc_2 2
#define nx_gpio_padfunc_3 3

#define nx_gpio_drvstrength_0 0
#define nx_gpio_drvstrength_1 1
#define nx_gpio_drvstrength_2 2
#define nx_gpio_drvstrength_3 3

#define nx_gpio_pull_down 0
#define nx_gpio_pull_up 1
#define nx_gpio_pull_off 2

/* GPIO Module's Register List */
struct nx_gpio_reg_set {
	/* 0x00	: Output Register */
	u32 GPIOxOUT;
	/* 0x04	: Output Enable Register */
	u32 GPIOxOUTENB;
	/* 0x08	: Event Detect Mode Register */
	u32 GPIOxDETMODE[2];
	/* 0x10	: Interrupt Enable Register */
	u32 GPIOxINTENB;
	/* 0x14	: Event Detect Register */
	u32 GPIOxDET;
	/* 0x18	: PAD Status Register */
	u32 GPIOxPAD;
	/* 0x1C	: Pull Up Enable Register */
	u32 GPIOxPUENB;
	/* 0x20	: Alternate Function Select Register */
	u32 GPIOxALTFN[2];
	/* 0x28	: Event Detect Mode extended Register */
	u32 GPIOxDETMODEEX;
	/* 0x2B	: */
	u32 __Reserved[4];
	/* 0x3C	: IntPend Detect Enable Register */
	u32 GPIOxDETENB;

	/* 0x40	: Slew Register */
	u32 GPIOx_SLEW;
	/* 0x44	: Slew set On/Off Register */
	u32 GPIOx_SLEW_DISABLE_DEFAULT;
	/* 0x48	: drive strength LSB Register */
	u32 GPIOx_DRV1;
	/* 0x4C	: drive strength LSB set On/Off Register */
	u32 GPIOx_DRV1_DISABLE_DEFAULT;
	/* 0x50	: drive strength MSB Register */
	u32 GPIOx_DRV0;
	/* 0x54	: drive strength MSB set On/Off Register */
	u32 GPIOx_DRV0_DISABLE_DEFAULT;
	/* 0x58	: Pull UP/DOWN Selection Register */
	u32 GPIOx_PULLSEL;
	/* 0x5C	: Pull UP/DOWN Selection On/Off Register */
	u32 GPIOx_PULLSEL_DISABLE_DEFAULT;
	/* 0x60	: Pull Enable/Disable Register */
	u32 GPIOx_PULLENB;
	/* 0x64	: Pull Enable/Disable selection On/Off Register */
	u32 GPIOx_PULLENB_DISABLE_DEFAULT;
	/* 0x68 */
	u32 GPIOx_InputMuxSelect0;
	/* 0x6C */
	u32 GPIOx_InputMuxSelect1;
};

struct module_init_data {
	struct list_head node;
	void *bank_base;
	int bank_type;		/* 0: none, 1: gpio, 2: alive */
};

/*
 * nx_alive
 */

/* ALIVE Interrupts for interrupt interface */
#define nx_alive_int_alivegpio0 0
#define nx_alive_int_alivegpio1 1
#define nx_alive_int_alivegpio2 2
#define nx_alive_int_alivegpio3 3
#define nx_alive_int_alivegpio4 4
#define nx_alive_int_alivegpio5 5
#define nx_alive_int_alivegpio6 6
#define nx_alive_int_alivegpio7 7

/* ALIVE GPIO Detect Mode */
#define nx_alive_detect_mode_async_lowlevel 0
#define nx_alive_detect_mode_async_highlevel 1
#define nx_alive_detect_mode_sync_fallingedge 2
#define nx_alive_detect_mode_sync_risingedge 3
#define nx_alive_detect_mode_sync_lowlevel 4
#define nx_alive_detect_mode_sync_highlevel 5

#define nx_alive_number_of_gpio 6

struct nx_alive_reg_set {
	/* 0x00 : Alive Power Gating Register */
	u32 ALIVEPWRGATEREG;
	/* 0x04 : Alive GPIO ASync Detect Mode Reset Register0 */
	u32 ALIVEGPIOASYNCDETECTMODERSTREG0;
	/* 0x08 : Alive GPIO gASync Detect Mode Set gRegister0 */
	u32 ALIVEGPIOASYNCDETECTMODESETREG0;
	/* 0x0C : Alive GPIO gLow ASync Detect gMode Read Register */
	u32 ALIVEGPIOLOWASYNCDETECTMODEREADREG;

	/* 0x10 : Alive GPIO gASync Detect Mode Reset gRegister1 */
	u32 ALIVEGPIOASYNCDETECTMODERSTREG1;
	/* 0x14 : Alive GPIO gASync Detect Mode Set gRegister1 */
	u32 ALIVEGPIOASYNCDETECTMODESETREG1;
	/* 0x18 : Alive GPIO gHigh ASync Detect gMode Read Register */
	u32 ALIVEGPIOHIGHASYNCDETECTMODEREADREG;

	/* 0x1C : Alive GPIO Detect gMode Reset Register0 */
	u32 ALIVEGPIODETECTMODERSTREG0;
	/* 0x20 : Alive GPIO Detect gMode Reset Register0 */
	u32 ALIVEGPIODETECTMODESETREG0;
	/* 0x24 : Alive GPIO gFalling Edge Detect Mode gRead Register */
	u32 ALIVEGPIOFALLDETECTMODEREADREG;

	/* 0x28 : Alive GPIO Detect Mode Reset Register1 */
	u32 ALIVEGPIODETECTMODERSTREG1;
	/* 0x2C : Alive GPIO Detect Mode Reset Register1 */
	u32 ALIVEGPIODETECTMODESETREG1;
	/* 0x30 : Alive GPIO Rising Edge Detect Mode Read Register */
	u32 ALIVEGPIORISEDETECTMODEREADREG;

	/* 0x34 : Alive GPIO Detect Mode Reset Register2 */
	u32 ALIVEGPIODETECTMODERSTREG2;
	/* 0x38 : Alive GPIO Detect Mode Reset Register2 */
	u32 ALIVEGPIODETECTMODESETREG2;
	/* 0x3C : Alive GPIO Low Level Detect Mode Read gRegister */
	u32 ALIVEGPIOLOWDETECTMODEREADREG;

	/* 0x40 : Alive GPIO Detect Mode Reset Register3 */
	u32 ALIVEGPIODETECTMODERSTREG3;
	/* 0x44 : Alive GPIO Detect Mode Reset Register3 */
	u32 ALIVEGPIODETECTMODESETREG3;
	/* 0x48 : Alive GPIO High Level Detect Mode Read gRegister */
	u32 ALIVEGPIOHIGHDETECTMODEREADREG;

	/* 0x4C : Alive GPIO Detect Enable Reset Register */
	u32 ALIVEGPIODETECTENBRSTREG;
	/* 0x50 : Alive GPIO Detect Enable Set Register */
	u32 ALIVEGPIODETECTENBSETREG;
	/* 0x54 : Alive GPIO Detect Enable Read Register */
	u32 ALIVEGPIODETECTENBREADREG;

	/* 0x58 : Alive GPIO Interrupt Enable Reset Register */
	u32 ALIVEGPIOINTENBRSTREG;
	/* 0x5C : Alive GPIO Interrupt Enable Set Register */
	u32 ALIVEGPIOINTENBSETREG;
	/* 0x60 : Alive GPIO Interrupt Enable Read Register */
	u32 ALIVEGPIOINTENBREADREG;

	/* 0x64 : Alive GPIO Detect Pending Register */
	u32 ALIVEGPIODETECTPENDREG;

	/* 0x68 : Alive Scratch Reset Register */
	u32 ALIVESCRATCHRSTREG;
	/* 0x6C : Alive Scratch Set Register */
	u32 ALIVESCRATCHSETREG;
	/* 0x70 : Alive Scratch Read Register */
	u32 ALIVESCRATCHREADREG;

	/* 0x74 : Alive GPIO PAD Out Enable Reset Register */
	u32 ALIVEGPIOPADOUTENBRSTREG;
	/* 0x78 : Alive GPIO PAD Out Enable Set Register */
	u32 ALIVEGPIOPADOUTENBSETREG;
	/* 0x7C : Alive GPIO PAD Out Enable Read Register */
	u32 ALIVEGPIOPADOUTENBREADREG;

	/* 0x80 : Alive GPIO PAD Pullup Reset Register */
	u32 ALIVEGPIOPADPULLUPRSTREG;
	/* 0x84 : Alive GPIO PAD Pullup Set Register */
	u32 ALIVEGPIOPADPULLUPSETREG;
	/* 0x88 : Alive GPIO PAD Pullup Read Register */
	u32 ALIVEGPIOPADPULLUPREADREG;

	/* 0x8C : Alive GPIO PAD Out Reset Register */
	u32 ALIVEGPIOPADOUTRSTREG;
	/* 0x90 : Alive GPIO PAD Out Set Register */
	u32 ALIVEGPIOPADOUTSETREG;
	/* 0x94 : Alive GPIO PAD Out Read Register */
	u32 ALIVEGPIOPADOUTREADREG;

	/*  0x98 : VDD Control Reset Register */
	u32 VDDCTRLRSTREG;
	/*  0x9C : VDD Control Set Register */
	u32 VDDCTRLSETREG;
	/* 0xA0 : VDD Control Read Register */
	u32 VDDCTRLREADREG;

	/* 0x0A4 :  wCS[41] */
	u32 CLEARWAKEUPSTATUS;
	/* 0x0A8 :  wCS[42] */
	u32 WAKEUPSTATUS;

	/* 0x0AC :  wCS[43] */
	u32 ALIVESCRATCHRST1;
	/* 0x0B0 :  wCS[44] */
	u32 ALIVESCRATCHSET1;
	/* 0x0B4 :  wCS[45] */
	u32 ALIVESCRATCHVALUE1;

	/* 0x0B8 :  wCS[46] */
	u32 ALIVESCRATCHRST2;
	/* 0x0BC :  wCS[47] */
	u32 ALIVESCRATCHSET2;
	/* 0x0C0 :  wCS[48] */
	u32 ALIVESCRATCHVALUE2;

	/* 0x0C4 :  wCS[49] */
	u32 ALIVESCRATCHRST3;
	/* 0x0C8 :  wCS[50] */
	u32 ALIVESCRATCHSET3;
	/* 0x0CC :  wCS[51] */
	u32 ALIVESCRATCHVALUE3;

	/* 0x0D0 :  wCS[52] */
	u32 ALIVESCRATCHRST4;
	/* 0x0D4 :  wCS[53] */
	u32 ALIVESCRATCHSET4;
	/* 0x0D8 :  wCS[54] */
	u32 ALIVESCRATCHVALUE4;

	/* 0x0DC :  wCS[55] */
	u32 ALIVESCRATCHRST5;
	/* 0x0E0 :  wCS[56] */
	u32 ALIVESCRATCHSET5;
	/* 0x0E4 :  wCS[57] */
	u32 ALIVESCRATCHVALUE5;

	/* 0x0E8 :  wCS[58] */
	u32 ALIVESCRATCHRST6;
	/* 0x0EC :  wCS[59] */
	u32 ALIVESCRATCHSET6;
	/* 0x0F0 :  wCS[60] */
	u32 ALIVESCRATCHVALUE6;

	/* 0x0F4 :  wCS[61] */
	u32 ALIVESCRATCHRST7;
	/* 0x0F8 :  wCS[62] */
	u32 ALIVESCRATCHSET7;
	/* 0x0FC :  wCS[63] */
	u32 ALIVESCRATCHVALUE7;

	/* 0x100 :  wCS[64] */
	u32 ALIVESCRATCHRST8;
	/* 0x104 :  wCS[65] */
	u32 ALIVESCRATCHSET8;
	/* 0x108 :  wCS[66] */
	u32 ALIVESCRATCHVALUE8;

	/* 0x10C :  wCS[67] */
	u32 VDDOFFCNTVALUERST;
	/* 0x110 :  wCS[68] */
	u32 VDDOFFCNTVALUESET;

	/* 0x114 :  wCS[69] */
	u32 VDDOFFCNTVALUE0;
	/* 0x118 :  wCS[70] */
	u32 VDDOFFCNTVALUE1;

	/* 0x11C :  wCS[71] */
	u32 ALIVEGPIOINPUTVALUE;
};

/*
 * nx_soc functions
 */
extern void nx_soc_gpio_set_io_func(unsigned int io, unsigned int func);
extern int nx_soc_gpio_get_altnum(unsigned int io);
extern unsigned int nx_soc_gpio_get_io_func(unsigned int io);
extern void nx_soc_gpio_set_io_dir(unsigned int io, int out);
extern int nx_soc_gpio_get_io_dir(unsigned int io);
extern void nx_soc_gpio_set_io_pull(unsigned int io, int val);
extern int nx_soc_gpio_get_io_pull(unsigned int io);
extern void nx_soc_gpio_set_io_drv(int gpio, int mode);
extern int nx_soc_gpio_get_io_drv(int gpio);
extern void nx_soc_gpio_set_out_value(unsigned int io, int high);
extern int nx_soc_gpio_get_out_value(unsigned int io);
extern int nx_soc_gpio_get_in_value(unsigned int io);
extern void nx_soc_gpio_set_int_enable(unsigned int io, int on);
extern int nx_soc_gpio_get_int_enable(unsigned int io);
extern void nx_soc_gpio_set_int_mode(unsigned int io, unsigned int mode);
extern int nx_soc_gpio_get_int_mode(unsigned int io);
extern int nx_soc_gpio_get_int_pend(unsigned int io);
extern void nx_soc_gpio_clr_int_pend(unsigned int io);

extern void nx_soc_alive_set_det_enable(unsigned int io, int on);
extern int nx_soc_alive_get_det_enable(unsigned int io);
extern void nx_soc_alive_set_det_mode(unsigned int io, unsigned int mode,
				      int on);
extern int nx_soc_alive_get_det_mode(unsigned int io, unsigned int mode);
extern int nx_soc_alive_get_int_pend(unsigned int io);
extern void nx_soc_alive_clr_int_pend(unsigned int io);

extern int s5pxx18_gpio_device_init(struct list_head *banks, int nr_banks);

extern int s5pxx18_gpio_suspend(int idx);
extern int s5pxx18_gpio_resume(int idx);

extern int s5pxx18_alive_suspend(void);
extern int s5pxx18_alive_resume(void);

extern u32 nx_alive_get_wakeup_status(void);
extern void nx_alive_clear_wakeup_status(void);

#endif /* __S5Pxx18_GPIO_H */
