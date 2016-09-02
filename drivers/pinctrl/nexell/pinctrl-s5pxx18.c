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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/of_irq.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/err.h>

#include "pinctrl-nexell.h"
#include "pinctrl-s5pxx18.h"
#include "s5pxx18-gpio.h"

static struct {
	struct nx_gpio_reg_set *gpio_regs;
	struct nx_gpio_reg_set gpio_save;
} gpio_modules[NUMBER_OF_GPIO_MODULE];

static struct nx_alive_reg_set *alive_regs;
static struct nx_alive_reg_set alive_saves;

/*
 * gpio functions
 */

void nx_gpio_setbit(u32 *p, u32 bit, bool enable)
{
	u32 newvalue = readl(p);

	newvalue &= ~(1UL << bit);
	newvalue |= (u32)enable << bit;

	writel(newvalue, p);
}

bool nx_gpio_getbit(u32 value, u32 bit)
{
	return (bool)((value >> bit) & (1UL));
}

void nx_gpio_setbit2(u32 *p, u32 bit, u32 value)
{
	u32 newvalue = readl(p);

	newvalue = (u32)(newvalue & ~(3UL << (bit * 2)));
	newvalue = (u32)(newvalue | (value << (bit * 2)));

	writel(newvalue, p);
}

u32 nx_gpio_getbit2(u32 value, u32 bit)
{
	return (u32)((u32)(value >> (bit * 2)) & 3UL);
}

bool nx_gpio_open_module(u32 idx)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	writel(0xFFFFFFFF, &p_register->GPIOx_SLEW_DISABLE_DEFAULT);
	writel(0xFFFFFFFF, &p_register->GPIOx_DRV1_DISABLE_DEFAULT);
	writel(0xFFFFFFFF, &p_register->GPIOx_DRV0_DISABLE_DEFAULT);
	writel(0xFFFFFFFF, &p_register->GPIOx_PULLSEL_DISABLE_DEFAULT);
	writel(0xFFFFFFFF, &p_register->GPIOx_PULLENB_DISABLE_DEFAULT);

	return true;
}

void nx_gpio_set_output_enable(u32 idx, u32 bitnum, bool enable)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	nx_gpio_setbit(&p_register->GPIOxOUTENB, bitnum, enable);
}

bool nx_gpio_get_detect_enable(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	return nx_gpio_getbit(readl(&p_register->GPIOxDETENB), bitnum);
}

void nx_gpio_set_detect_enable(u32 idx, u32 bitnum, bool enable)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	nx_gpio_setbit(&p_register->GPIOxDETENB, bitnum, enable);
}

bool nx_gpio_get_output_enable(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	return nx_gpio_getbit(readl(&p_register->GPIOxOUTENB), bitnum);
}

void nx_gpio_set_output_value(u32 idx, u32 bitnum, bool value)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	nx_gpio_setbit(&p_register->GPIOxOUT, bitnum, value);
}

bool nx_gpio_get_output_value(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	return nx_gpio_getbit(readl(&p_register->GPIOxOUT), bitnum);
}

bool nx_gpio_get_input_value(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	return nx_gpio_getbit(readl(&p_register->GPIOxPAD), bitnum);
}

void nx_gpio_set_pull_select(u32 idx, u32 bitnum, bool enable)
{
	nx_gpio_setbit(
	    &gpio_modules[idx].gpio_regs->GPIOx_PULLSEL_DISABLE_DEFAULT,
	    bitnum, true);
	nx_gpio_setbit(&gpio_modules[idx].gpio_regs->GPIOx_PULLSEL, bitnum,
		       enable);
}

bool nx_gpio_get_pull_select(u32 idx, u32 bitnum)
{
	return nx_gpio_getbit(gpio_modules[idx].gpio_regs->GPIOx_PULLSEL,
			      bitnum);
}

void nx_gpio_set_pull_mode(u32 idx, u32 bitnum, int mode)
{
	nx_gpio_setbit(
	    &gpio_modules[idx].gpio_regs->GPIOx_PULLSEL_DISABLE_DEFAULT,
	    bitnum, true);
	nx_gpio_setbit(
	    &gpio_modules[idx].gpio_regs->GPIOx_PULLENB_DISABLE_DEFAULT,
	    bitnum, true);

	if (mode == nx_gpio_pull_off) {
		nx_gpio_setbit(&gpio_modules[idx].gpio_regs->GPIOx_PULLENB,
			       bitnum, false);
		nx_gpio_setbit(&gpio_modules[idx].gpio_regs->GPIOx_PULLSEL,
			       bitnum, false);
	} else {
		nx_gpio_setbit(&gpio_modules[idx].gpio_regs->GPIOx_PULLSEL,
			       bitnum, (mode & 1 ? true : false));
		nx_gpio_setbit(&gpio_modules[idx].gpio_regs->GPIOx_PULLENB,
			       bitnum, true);
	}
}

void nx_gpio_set_pad_function(u32 idx, u32 bitnum, int padfunc)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	nx_gpio_setbit2(&p_register->GPIOxALTFN[bitnum / 16], bitnum % 16,
			(u32)padfunc);
}

int nx_gpio_get_pad_function(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	return nx_gpio_getbit2(
	    readl(&p_register->GPIOxALTFN[bitnum / 16]), bitnum % 16);
}

void nx_gpio_set_slew(u32 idx, u32 bitnum, bool enable)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	nx_gpio_setbit(&p_register->GPIOx_SLEW, bitnum, enable);
}

void nx_gpio_set_slew_disable_default(u32 idx, u32 bitnum, bool enable)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	nx_gpio_setbit(&p_register->GPIOx_SLEW_DISABLE_DEFAULT, bitnum,
		       enable);
}

bool nx_gpio_get_slew(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	return (bool)nx_gpio_getbit(readl(&p_register->GPIOx_SLEW), bitnum);
}

void nx_gpio_set_drive_strength(u32 idx, u32 bitnum, int drvstrength)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	nx_gpio_setbit(&p_register->GPIOx_DRV1, bitnum,
		       (bool)(((u32)drvstrength >> 0) & 0x1));
	nx_gpio_setbit(&p_register->GPIOx_DRV0, bitnum,
		       (bool)(((u32)drvstrength >> 1) & 0x1));
}

void nx_gpio_set_drive_strength_disable_default(u32 idx, u32 bitnum,
						bool enable)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	nx_gpio_setbit(&p_register->GPIOx_DRV1_DISABLE_DEFAULT, bitnum,
		       (bool)(enable));
	nx_gpio_setbit(&p_register->GPIOx_DRV0_DISABLE_DEFAULT, bitnum,
		       (bool)(enable));
}

int nx_gpio_get_drive_strength(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *p_register;
	u32 retvalue;

	p_register = gpio_modules[idx].gpio_regs;

	retvalue = nx_gpio_getbit(readl(&p_register->GPIOx_DRV0), bitnum)
		   << 1;
	retvalue |= nx_gpio_getbit(readl(&p_register->GPIOx_DRV1), bitnum)
		    << 0;

	return (int)retvalue;
}

void nx_gpio_set_pull_enable(u32 idx, u32 bitnum, int pullsel)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	if (pullsel == nx_gpio_pull_down || pullsel == nx_gpio_pull_up) {
		nx_gpio_setbit(&p_register->GPIOx_PULLSEL, bitnum,
			       (bool)pullsel);
		nx_gpio_setbit(&p_register->GPIOx_PULLENB, bitnum, true);
	} else
		nx_gpio_setbit(&p_register->GPIOx_PULLENB, bitnum, false);
}

int nx_gpio_get_pull_enable(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *p_register;
	bool enable;

	p_register = gpio_modules[idx].gpio_regs;
	enable = nx_gpio_getbit(readl(&p_register->GPIOx_PULLENB), bitnum);

	if (enable == true)
		return (int)nx_gpio_getbit(
		    readl(&p_register->GPIOx_PULLSEL), bitnum);
	else
		return (int)(nx_gpio_pull_off);
}

void nx_gpio_set_input_mux_select0(u32 idx, u32 value)
{
	writel(value, &gpio_modules[idx].gpio_regs->GPIOx_InputMuxSelect0);
}

void nx_gpio_set_input_mux_select1(u32 idx, u32 value)
{
	writel(value, &gpio_modules[idx].gpio_regs->GPIOx_InputMuxSelect1);
}

void nx_gpio_set_interrupt_enable(u32 idx, s32 irqnum, bool enable)
{
	struct nx_gpio_reg_set *p_register;
	u32 ReadValue;

	p_register = gpio_modules[idx].gpio_regs;

	ReadValue = readl(&p_register->GPIOxINTENB);

	ReadValue &= ~((u32)1 << irqnum);
	ReadValue |= ((u32)enable << irqnum);

	writel(ReadValue, &p_register->GPIOxINTENB);

	nx_gpio_set_detect_enable(idx, irqnum, enable);
}

bool nx_gpio_get_interrupt_enable(u32 idx, s32 irqnum)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	if (readl(&p_register->GPIOxINTENB) & (1UL << irqnum))
		return true;

	return false;
}

bool nx_gpio_get_interrupt_pending(u32 idx, s32 irqnum)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	if (readl(&p_register->GPIOxDET) & (1UL << irqnum))
		return true;

	return false;
}

void nx_gpio_clear_interrupt_pending(u32 idx, s32 irqnum)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	writel(1UL << irqnum, &p_register->GPIOxDET);
}

s32 nx_gpio_get_interrupt_pending_number(u32 idx)
{
	struct nx_gpio_reg_set *p_register;
	u32 intnum;
	u32 intpend;

	p_register = gpio_modules[idx].gpio_regs;

	intpend = readl(&p_register->GPIOxDET);

	for (intnum = 0; intnum < 32; intnum++) {
		if (0 != (intpend & (1UL << intnum)))
			return intnum;
	}

	return -1;
}

/*
 * GPIO Operation.
 */

void nx_gpio_set_interrupt_mode(u32 idx, u32 bitnum, int irqmode)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	nx_gpio_setbit2(&p_register->GPIOxDETMODE[bitnum / 16],
			bitnum % 16, (u32)irqmode & 0x03);
	nx_gpio_setbit(&p_register->GPIOxDETMODEEX, bitnum,
		       (u32)(irqmode >> 2));
}

int nx_gpio_get_interrupt_mode(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *p_register;
	u32 regvalue;

	p_register = gpio_modules[idx].gpio_regs;

	regvalue = nx_gpio_getbit2(
	    readl(&p_register->GPIOxDETMODE[bitnum / 16]), bitnum % 16);
	regvalue |=
	    (nx_gpio_getbit(readl(&p_register->GPIOxDETMODEEX), bitnum) << 2);

	return regvalue;
}


/*
 * alive functions
 */

void nx_alive_set_interrupt_enable(s32 irqnum, bool enable)
{
	u32 INTENB_MASK;

	INTENB_MASK = (1UL << irqnum);

	if (enable)
		writel(INTENB_MASK, &alive_regs->ALIVEGPIOINTENBSETREG);
	else
		writel(INTENB_MASK, &alive_regs->ALIVEGPIOINTENBRSTREG);
}

bool nx_alive_get_interrupt_enable(s32 irqnum)
{
	return (bool)((alive_regs->ALIVEGPIOINTENBREADREG >> irqnum) & 0x01);
}

bool nx_alive_get_interrupt_pending(s32 irqnum)
{
	return ((alive_regs->ALIVEGPIODETECTPENDREG >> irqnum) & 0x01);
}

void nx_alive_clear_interrupt_pending(s32 irqnum)
{
	writel(1 << irqnum, &alive_regs->ALIVEGPIODETECTPENDREG);
}

s32 nx_alive_get_interrupt_pending_number(void)
{
	struct nx_alive_reg_set *p_register;
	u32 Pend;
	u32 int_num;

	p_register = alive_regs;

	Pend = (p_register->ALIVEGPIODETECTPENDREG &
		p_register->ALIVEGPIOINTENBREADREG);

	for (int_num = 0; int_num < 6; int_num++)
		if (Pend & (1 << int_num))
			return int_num;

	return -1;
}

/*
 * PAD Configuration
 */

void nx_alive_set_write_enable(bool enable)
{
	writel((u32)enable, &alive_regs->ALIVEPWRGATEREG);
}

bool nx_alive_get_write_enable(void)
{
	return (bool)(alive_regs->ALIVEPWRGATEREG & 0x01);
}

void nx_alive_set_scratch_reg(u32 data)
{
	writel(data, &alive_regs->ALIVESCRATCHSETREG);
	writel(~data, &alive_regs->ALIVESCRATCHRSTREG);
}

u32 nx_alive_get_scratch_reg(void)
{
	return (u32)(alive_regs->ALIVESCRATCHREADREG);
}

void nx_alive_set_pullup_enable(u32 bitnum, bool enable)
{
	u32 PULLUP_MASK;

	PULLUP_MASK = (1UL << bitnum);

	if (enable)
		writel(PULLUP_MASK, &alive_regs->ALIVEGPIOPADPULLUPSETREG);
	else
		writel(PULLUP_MASK, &alive_regs->ALIVEGPIOPADPULLUPRSTREG);
}

bool nx_alive_get_pullup_enable(u32 bitnum)
{
	return (bool)((alive_regs->ALIVEGPIOPADPULLUPREADREG >> bitnum) &
		      0x01);
}

/*
 * Input Setting Function
 */

void nx_alive_set_detect_enable(u32 bitnum, bool enable)
{
	u32 detectenb_mask;

	detectenb_mask = (1UL << bitnum);

	if (enable)
		writel(detectenb_mask, &alive_regs->ALIVEGPIODETECTENBSETREG);
	else
		writel(detectenb_mask, &alive_regs->ALIVEGPIODETECTENBRSTREG);
}

bool nx_alive_get_detect_enable(u32 bitnum)
{
	return (bool)((alive_regs->ALIVEGPIODETECTENBREADREG >> bitnum) &
		      0x01);
}

void nx_alive_set_detect_mode(int detect_mode, u32 bitnum, bool enable)
{
	u32 *pSetReg = 0;
	u32 *pRstReg = 0;

	switch (detect_mode) {
	case nx_alive_detect_mode_async_lowlevel:
		pSetReg = &alive_regs->ALIVEGPIOASYNCDETECTMODESETREG0;
		pRstReg = &alive_regs->ALIVEGPIOASYNCDETECTMODERSTREG0;
		break;

	case nx_alive_detect_mode_async_highlevel:
		pSetReg = &alive_regs->ALIVEGPIOASYNCDETECTMODESETREG1;
		pRstReg = &alive_regs->ALIVEGPIOASYNCDETECTMODERSTREG1;
		break;

	case nx_alive_detect_mode_sync_fallingedge:
		pSetReg = &alive_regs->ALIVEGPIODETECTMODESETREG0;
		pRstReg = &alive_regs->ALIVEGPIODETECTMODERSTREG0;
		break;

	case nx_alive_detect_mode_sync_risingedge:
		pSetReg = &alive_regs->ALIVEGPIODETECTMODESETREG1;
		pRstReg = &alive_regs->ALIVEGPIODETECTMODERSTREG1;
		break;

	case nx_alive_detect_mode_sync_lowlevel:
		pSetReg = &alive_regs->ALIVEGPIODETECTMODESETREG2;
		pRstReg = &alive_regs->ALIVEGPIODETECTMODERSTREG2;
		break;

	case nx_alive_detect_mode_sync_highlevel:
		pSetReg = &alive_regs->ALIVEGPIODETECTMODESETREG3;
		pRstReg = &alive_regs->ALIVEGPIODETECTMODERSTREG3;
		break;

	default:
		break;
	}

	if (enable)
		writel((1UL << bitnum), pSetReg);
	else
		writel((1UL << bitnum), pRstReg);
}

bool nx_alive_get_detect_mode(int detect_mode, u32 bitnum)
{
	switch (detect_mode) {
	case nx_alive_detect_mode_async_lowlevel:
		return (bool)((alive_regs->ALIVEGPIOLOWASYNCDETECTMODEREADREG >>
			       bitnum) & 0x01);

	case nx_alive_detect_mode_async_highlevel:
		return (
		    bool)((alive_regs->ALIVEGPIOHIGHASYNCDETECTMODEREADREG >>
			   bitnum) & 0x01);

	case nx_alive_detect_mode_sync_fallingedge:
		return (bool)((alive_regs->ALIVEGPIOFALLDETECTMODEREADREG >>
			       bitnum) & 0x01);

	case nx_alive_detect_mode_sync_risingedge:
		return (bool)((alive_regs->ALIVEGPIORISEDETECTMODEREADREG >>
			       bitnum) & 0x01);

	case nx_alive_detect_mode_sync_lowlevel:
		return (bool)((alive_regs->ALIVEGPIOLOWDETECTMODEREADREG >>
			       bitnum) & 0x01);

	case nx_alive_detect_mode_sync_highlevel:
		return (bool)((alive_regs->ALIVEGPIOHIGHDETECTMODEREADREG >>
			       bitnum) & 0x01);

	default:
		break;
	}

	return false;
}

bool nx_alive_get_vdd_pwr_toggle(void)
{
	const u32 VDDPWRTOGGLE_BITPOS = 10;

	return (bool)((alive_regs->VDDCTRLREADREG >> VDDPWRTOGGLE_BITPOS) &
		      0x01);
}

/*
 * Output Setting Function
 */

void nx_alive_set_output_enable(u32 bitnum, bool enable)
{
	u32 padoutenb_mask;

	padoutenb_mask = (1UL << bitnum);

	if (enable)
		writel(padoutenb_mask, &alive_regs->ALIVEGPIOPADOUTENBSETREG);
	else
		writel(padoutenb_mask, &alive_regs->ALIVEGPIOPADOUTENBRSTREG);
}

bool nx_alive_get_output_enable(u32 bitnum)
{
	return (bool)((alive_regs->ALIVEGPIOPADOUTENBREADREG >> bitnum) &
		      0x01);
}

bool nx_alive_get_input_enable(u32 bitnum)
{
	return (bool)((alive_regs->ALIVEGPIOPADOUTENBREADREG >> bitnum) &
		      0x01)
		   ? false
		   : true;
}

void nx_alive_set_output_value(u32 bitnum, bool value)
{
	u32 padout_mask;

	padout_mask = (1UL << bitnum);

	if (value)
		writel(padout_mask, &alive_regs->ALIVEGPIOPADOUTSETREG);
	else
		writel(padout_mask, &alive_regs->ALIVEGPIOPADOUTRSTREG);
}

bool nx_alive_get_output_value(u32 bitnum)
{
	return (bool)((alive_regs->ALIVEGPIOPADOUTREADREG >> bitnum) & 0x01);
}

bool nx_alive_get_input_value(u32 bitnum)
{
	return (bool)((alive_regs->ALIVEGPIOINPUTVALUE >> bitnum) & 0x01);
}

u32 nx_alive_get_wakeup_status(void)
{
	u32 status;

	status = alive_regs->WAKEUPSTATUS;
	return status;
}

void nx_alive_clear_wakeup_status(void) { alive_regs->CLEARWAKEUPSTATUS = 1; }

/*
 * end of nx_alive
 */

const unsigned char (*gpio_fn_no)[GPIO_NUM_PER_BANK] = NULL;

const unsigned char s5pxx18_pio_fn_no[][GPIO_NUM_PER_BANK] = {
	ALT_NO_GPIO_A, ALT_NO_GPIO_B, ALT_NO_GPIO_C,
	ALT_NO_GPIO_D, ALT_NO_GPIO_E, ALT_NO_ALIVE,
};

/*----------------------------------------------------------------------------*/
#define ALIVE_INDEX 5			 /* number of gpio module */
static spinlock_t lock[ALIVE_INDEX + 1]; /* A, B, C, D, E, alive */
static unsigned long lock_flags[ALIVE_INDEX + 1];

#define IO_LOCK_INIT(x) spin_lock_init(&lock[x])
#define IO_LOCK(x) spin_lock_irqsave(&lock[x], lock_flags[x])
#define IO_UNLOCK(x) spin_unlock_irqrestore(&lock[x], lock_flags[x])

void nx_soc_gpio_set_io_func(unsigned int io, unsigned int func)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		nx_gpio_set_pad_function(grp, bit, func);
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
		       __func__);
		break;
	};
}

int nx_soc_gpio_get_altnum(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);

	return gpio_fn_no[grp][bit];
}

unsigned int nx_soc_gpio_get_io_func(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	unsigned int fn = -1;

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		fn = nx_gpio_get_pad_function(grp, bit);
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		fn = 0;
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
		       __func__);
		break;
	};
	return fn;
}

void nx_soc_gpio_set_io_dir(unsigned int io, int out)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		nx_gpio_set_output_enable(grp, bit, out ? true : false);
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		IO_LOCK(grp);
		nx_alive_set_output_enable(bit, out ? true : false);
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
		       __func__);
		break;
	};
}

int nx_soc_gpio_get_io_dir(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	int dir = -1;

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		dir = nx_gpio_get_output_enable(grp, bit) ? 1 : 0;
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		IO_LOCK(grp);
		dir = nx_alive_get_output_enable(bit) ? 1 : 0;
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
		       __func__);
		break;
	};
	return dir;
}

void nx_soc_gpio_set_io_pull(unsigned int io, int val)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);

	pr_debug("%s (%d.%02d) sel:%d\n", __func__, grp, bit, val);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		nx_gpio_set_pull_enable(grp, bit, val);
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		IO_LOCK(grp);
		if (val & 1)	/* up */
			nx_alive_set_pullup_enable(bit, true);
		else	/* down, off */
			nx_alive_set_pullup_enable(bit, false);
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
				__func__);
		break;
	};
}

int nx_soc_gpio_get_io_pull(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	int up = -1;

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		up = nx_gpio_get_pull_enable(grp, bit);
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		IO_LOCK(grp);
		up = nx_alive_get_pullup_enable(bit) ? 1 : 0;
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
				__func__);
		break;
	};

	return up;
}

void nx_soc_gpio_set_io_drv(int gpio, int mode)
{
	int grp, bit;

	if (gpio > (PAD_GPIO_ALV - 1))
		return;

	grp = PAD_GET_GROUP(gpio);
	bit = PAD_GET_BITNO(gpio);
	pr_debug("%s (%d.%02d) mode:%d\n", __func__, grp, bit, mode);

	nx_gpio_set_drive_strength(grp, bit, (int)mode);
}

int nx_soc_gpio_get_io_drv(int gpio)
{
	int grp, bit;

	if (gpio > (PAD_GPIO_ALV - 1))
		return -1;

	grp = PAD_GET_GROUP(gpio);
	bit = PAD_GET_BITNO(gpio);
	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	return (int)nx_gpio_get_drive_strength(grp, bit);
}

void nx_soc_gpio_set_out_value(unsigned int io, int high)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		nx_gpio_set_output_value(grp, bit, high ? true : false);
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		IO_LOCK(grp);
		nx_alive_set_output_value(bit, high ? true : false);
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
		       __func__);
		break;
	};
}

int nx_soc_gpio_get_out_value(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	int val = -1;

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		val = nx_gpio_get_output_value(grp, bit) ? 1 : 0;
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		IO_LOCK(grp);
		val = nx_alive_get_output_value(bit) ? 1 : 0;
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
		       __func__);
		break;
	};
	return val;
}

int nx_soc_gpio_get_in_value(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	int val = -1;

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		val = nx_gpio_get_input_value(grp, bit) ? 1 : 0;
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		IO_LOCK(grp);
		val = nx_alive_get_input_value(bit) ? 1 : 0;
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
		       __func__);
		break;
	};
	return val;
}

void nx_soc_gpio_set_int_enable(unsigned int io, int on)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		nx_gpio_set_interrupt_enable(grp, bit, on ? true : false);
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		IO_LOCK(grp);
		nx_alive_set_detect_enable(bit, on ? true : false);
		nx_alive_set_interrupt_enable(bit, on ? true : false);
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
		       __func__);
		break;
	};
}

int nx_soc_gpio_get_int_enable(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	int enb = -1;

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		enb = nx_gpio_get_interrupt_enable(grp, bit) ? 1 : 0;
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		IO_LOCK(grp);
		enb = nx_alive_get_interrupt_enable(bit) ? 1 : 0;
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
		       __func__);
		break;
	};
	return enb;
}

void nx_soc_gpio_set_int_mode(unsigned int io, unsigned int mode)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	int det = 0;

	pr_debug("%s (%d.%02d, %d)\n", __func__, grp, bit, mode);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		nx_gpio_set_interrupt_mode(grp, bit, mode);
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		IO_LOCK(grp);
		/* all disable */
		for (det = 0; 6 > det; det++)
			nx_alive_set_detect_mode(det, bit, false);
		/* enable */
		nx_alive_set_detect_mode(mode, bit, true);
		nx_alive_set_output_enable(bit, false);
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
		       __func__);
		break;
	};
}

int nx_soc_gpio_get_int_mode(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	int mod = -1;
	int det = 0;

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		mod = nx_gpio_get_interrupt_mode(grp, bit);
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		IO_LOCK(grp);
		for (det = 0; 6 > det; det++) {
			if (nx_alive_get_detect_mode(det, bit)) {
				mod = det;
				break;
			}
		}
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
		       __func__);
		break;
	};
	return mod;
}

int nx_soc_gpio_get_int_pend(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	int pend = -1;

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		pend = nx_gpio_get_interrupt_pending(grp, bit) ? 1 : 0;
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		IO_LOCK(grp);
		pend = nx_alive_get_interrupt_pending(bit) ? 1 : 0;
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
		       __func__);
		break;
	};
	return pend;
}

void nx_soc_gpio_clr_int_pend(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
		IO_LOCK(grp);
		nx_gpio_clear_interrupt_pending(grp, bit);
		nx_gpio_get_interrupt_pending(grp, bit);
		IO_UNLOCK(grp);
		break;
	case PAD_GPIO_ALV:
		IO_LOCK(grp);
		nx_alive_clear_interrupt_pending(bit);
		nx_alive_get_interrupt_pending(bit);
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail, soc gpio io:%d, group:%d (%s)\n", io, grp,
		       __func__);
		break;
	};
}

void nx_soc_alive_set_det_enable(unsigned int io, int on)
{
	unsigned int bit = PAD_GET_BITNO(io);

	pr_debug("%s (%d)\n", __func__, bit);

	IO_LOCK(ALIVE_INDEX);

	nx_alive_set_output_enable(bit, false);
	nx_alive_set_detect_enable(bit, on ? true : false);

	IO_UNLOCK(ALIVE_INDEX);
}

int nx_soc_alive_get_det_enable(unsigned int io)
{
	unsigned int bit = PAD_GET_BITNO(io);
	int mod = 0;

	pr_debug("%s (%d)\n", __func__, bit);

	IO_LOCK(ALIVE_INDEX);
	mod = nx_alive_get_detect_enable(bit) ? 1 : 0;
	IO_UNLOCK(ALIVE_INDEX);

	return mod;
}

void nx_soc_alive_set_det_mode(unsigned int io, unsigned int mode, int on)
{
	unsigned int bit = PAD_GET_BITNO(io);

	pr_debug("%s (%d)\n", __func__, bit);

	IO_LOCK(ALIVE_INDEX);
	nx_alive_set_detect_mode(mode, bit, on ? true : false);
	IO_UNLOCK(ALIVE_INDEX);
}

int nx_soc_alive_get_det_mode(unsigned int io, unsigned int mode)
{
	unsigned int bit = PAD_GET_BITNO(io);
	int mod = 0;

	pr_debug("%s (%d)\n", __func__, bit);

	IO_LOCK(ALIVE_INDEX);
	mod = nx_alive_get_detect_mode(mode, bit) ? 1 : 0;
	IO_UNLOCK(ALIVE_INDEX);

	return mod;
}

int nx_soc_alive_get_int_pend(unsigned int io)
{
	unsigned int bit = PAD_GET_BITNO(io);
	int pend = -1;

	pr_debug("%s (%d)\n", __func__, bit);

	IO_LOCK(ALIVE_INDEX);
	pend = nx_alive_get_interrupt_pending(bit);
	IO_UNLOCK(ALIVE_INDEX);

	return pend;
}

void nx_soc_alive_clr_int_pend(unsigned int io)
{
	unsigned int bit = PAD_GET_BITNO(io);

	pr_debug("%s (%d)\n", __func__, bit);

	IO_LOCK(ALIVE_INDEX);
	nx_alive_clear_interrupt_pending(bit);
	IO_UNLOCK(ALIVE_INDEX);
}

int s5pxx18_gpio_suspend(int idx)
{
	struct nx_gpio_reg_set *reg;
	struct nx_gpio_reg_set *gpio_save;

	if (idx < 0 || idx >= NUMBER_OF_GPIO_MODULE)
		return -ENXIO;

	reg = gpio_modules[idx].gpio_regs;
	gpio_save = &gpio_modules[idx].gpio_save;

	gpio_save->GPIOxOUT = readl(&reg->GPIOxOUT);
	gpio_save->GPIOxOUTENB = readl(&reg->GPIOxOUTENB);
	gpio_save->GPIOxALTFN[0] = readl(&reg->GPIOxALTFN[0]);
	gpio_save->GPIOxALTFN[1] = readl(&reg->GPIOxALTFN[1]);
	gpio_save->GPIOxDETMODE[0] = readl(&reg->GPIOxDETMODE[0]);
	gpio_save->GPIOxDETMODE[1] = readl(&reg->GPIOxDETMODE[1]);
	gpio_save->GPIOxDETMODEEX = readl(&reg->GPIOxDETMODEEX);
	gpio_save->GPIOxINTENB = readl(&reg->GPIOxINTENB);

	gpio_save->GPIOx_SLEW = readl(&reg->GPIOx_SLEW);
	gpio_save->GPIOx_SLEW_DISABLE_DEFAULT =
		readl(&reg->GPIOx_SLEW_DISABLE_DEFAULT);
	gpio_save->GPIOx_DRV1 = readl(&reg->GPIOx_DRV1);
	gpio_save->GPIOx_DRV1_DISABLE_DEFAULT =
		readl(&reg->GPIOx_DRV1_DISABLE_DEFAULT);
	gpio_save->GPIOx_DRV0 = readl(&reg->GPIOx_DRV0);
	gpio_save->GPIOx_DRV0_DISABLE_DEFAULT =
		readl(&reg->GPIOx_DRV0_DISABLE_DEFAULT);
	gpio_save->GPIOx_PULLSEL = readl(&reg->GPIOx_PULLSEL);
	gpio_save->GPIOx_PULLSEL_DISABLE_DEFAULT =
		readl(&reg->GPIOx_PULLSEL_DISABLE_DEFAULT);
	gpio_save->GPIOx_PULLENB = readl(&reg->GPIOx_PULLENB);
	gpio_save->GPIOx_PULLENB_DISABLE_DEFAULT =
		readl(&reg->GPIOx_PULLENB_DISABLE_DEFAULT);

	return 0;
}

int s5pxx18_gpio_resume(int idx)
{
	struct nx_gpio_reg_set *reg;
	struct nx_gpio_reg_set *gpio_save;

	if (idx < 0 || idx >= NUMBER_OF_GPIO_MODULE)
		return -ENXIO;

	reg = gpio_modules[idx].gpio_regs;
	gpio_save = &gpio_modules[idx].gpio_save;

	writel(gpio_save->GPIOx_SLEW, &reg->GPIOx_SLEW);
	writel(gpio_save->GPIOx_SLEW_DISABLE_DEFAULT,
		&reg->GPIOx_SLEW_DISABLE_DEFAULT);
	writel(gpio_save->GPIOx_DRV1, &reg->GPIOx_DRV1);
	writel(gpio_save->GPIOx_DRV1_DISABLE_DEFAULT,
		&reg->GPIOx_DRV1_DISABLE_DEFAULT);
	writel(gpio_save->GPIOx_DRV0, &reg->GPIOx_DRV0);
	writel(gpio_save->GPIOx_DRV0_DISABLE_DEFAULT,
		&reg->GPIOx_DRV0_DISABLE_DEFAULT);
	writel(gpio_save->GPIOx_PULLSEL, &reg->GPIOx_PULLSEL);
	writel(gpio_save->GPIOx_PULLSEL_DISABLE_DEFAULT,
		&reg->GPIOx_PULLSEL_DISABLE_DEFAULT);
	writel(gpio_save->GPIOx_PULLENB, &reg->GPIOx_PULLENB);
	writel(gpio_save->GPIOx_PULLENB_DISABLE_DEFAULT,
		&reg->GPIOx_PULLENB_DISABLE_DEFAULT);

	writel(gpio_save->GPIOxOUT, &reg->GPIOxOUT);
	writel(gpio_save->GPIOxOUTENB, &reg->GPIOxOUTENB);
	writel(gpio_save->GPIOxALTFN[0], &reg->GPIOxALTFN[0]);
	writel(gpio_save->GPIOxALTFN[1], &reg->GPIOxALTFN[1]);
	writel(gpio_save->GPIOxDETMODE[0], &reg->GPIOxDETMODE[0]);
	writel(gpio_save->GPIOxDETMODE[1], &reg->GPIOxDETMODE[1]);
	writel(gpio_save->GPIOxDETMODEEX, &reg->GPIOxDETMODEEX);
	writel(gpio_save->GPIOxINTENB, &reg->GPIOxINTENB);
	writel(gpio_save->GPIOxINTENB, &reg->GPIOxDETENB);/* DETECT ENABLE */
	writel((u32)0xFFFFFFFF, &reg->GPIOxDET);	/* CLEAR PENDING */

	return 0;
}

int s5pxx18_alive_suspend(void)
{
	struct nx_alive_reg_set *reg;
	struct nx_alive_reg_set *alive_save;
	u32 both_edge = 0;

	reg = alive_regs;
	alive_save = &alive_saves;

	nx_alive_set_write_enable(true);

	alive_save->ALIVEGPIOLOWASYNCDETECTMODEREADREG =
		readl(&reg->ALIVEGPIOLOWASYNCDETECTMODEREADREG);
	alive_save->ALIVEGPIOHIGHASYNCDETECTMODEREADREG =
		readl(&reg->ALIVEGPIOHIGHASYNCDETECTMODEREADREG);
	alive_save->ALIVEGPIOFALLDETECTMODEREADREG =
		readl(&reg->ALIVEGPIOFALLDETECTMODEREADREG);
	alive_save->ALIVEGPIORISEDETECTMODEREADREG =
		readl(&reg->ALIVEGPIORISEDETECTMODEREADREG);
	alive_save->ALIVEGPIOLOWDETECTMODEREADREG =
		readl(&reg->ALIVEGPIOLOWDETECTMODEREADREG);
	alive_save->ALIVEGPIOHIGHDETECTMODEREADREG =
		readl(&reg->ALIVEGPIOHIGHDETECTMODEREADREG);

	alive_save->ALIVEGPIODETECTENBREADREG =
		readl(&reg->ALIVEGPIODETECTENBREADREG);
	alive_save->ALIVEGPIOINTENBREADREG =
		readl(&reg->ALIVEGPIOINTENBREADREG);
	alive_save->ALIVEGPIOPADOUTENBREADREG =
		readl(&reg->ALIVEGPIOPADOUTENBREADREG);
	alive_save->ALIVEGPIOPADOUTREADREG =
		readl(&reg->ALIVEGPIOPADOUTREADREG);
	alive_save->ALIVEGPIOPADPULLUPREADREG
		= readl(&reg->ALIVEGPIOPADPULLUPREADREG);

	/* change both edge detect to falling only */
	both_edge = (alive_save->ALIVEGPIOFALLDETECTMODEREADREG &
		    alive_save->ALIVEGPIORISEDETECTMODEREADREG);
	writel((u32)both_edge, &reg->ALIVEGPIODETECTMODERSTREG1);

	return 0;
}

int s5pxx18_alive_resume(void)
{
	struct nx_alive_reg_set *reg;
	struct nx_alive_reg_set *alive_save;

	reg = alive_regs;
	alive_save = &alive_saves;

	nx_alive_set_write_enable(true);

	/* clear and set */
	if (alive_save->ALIVEGPIOPADOUTENBREADREG !=
			readl(&reg->ALIVEGPIOPADOUTENBREADREG)) {
		writel((u32)0xFFFFFFFF, &reg->ALIVEGPIOPADOUTENBRSTREG);
		writel(alive_save->ALIVEGPIOPADOUTENBREADREG,
				&reg->ALIVEGPIOPADOUTENBSETREG);
	}
	if (alive_save->ALIVEGPIOPADOUTREADREG !=
			readl(&reg->ALIVEGPIOPADOUTREADREG)) {
		writel((u32)0xFFFFFFFF, &reg->ALIVEGPIOPADOUTRSTREG);
		writel(alive_save->ALIVEGPIOPADOUTREADREG,
				&reg->ALIVEGPIOPADOUTSETREG);
	}
	if (alive_save->ALIVEGPIOPADPULLUPREADREG !=
			readl(&reg->ALIVEGPIOPADPULLUPREADREG)) {
		writel((u32)0xFFFFFFFF, &reg->ALIVEGPIOPADPULLUPRSTREG);
		writel(alive_save->ALIVEGPIOPADPULLUPREADREG,
				&reg->ALIVEGPIOPADPULLUPSETREG);
	}

	writel((u32)0xFFFFFFFF, &reg->ALIVEGPIODETECTENBRSTREG);
	writel((u32)0xFFFFFFFF, &reg->ALIVEGPIOINTENBRSTREG);


	writel((u32)0xFFFFFFFF, &reg->ALIVEGPIOASYNCDETECTMODERSTREG0);
	writel(alive_save->ALIVEGPIOLOWASYNCDETECTMODEREADREG,
				&reg->ALIVEGPIOASYNCDETECTMODESETREG0);

	writel((u32)0xFFFFFFFF, &reg->ALIVEGPIOASYNCDETECTMODERSTREG1);
	writel(alive_save->ALIVEGPIOHIGHASYNCDETECTMODEREADREG,
				&reg->ALIVEGPIOASYNCDETECTMODESETREG1);

	writel((u32)0xFFFFFFFF, &reg->ALIVEGPIODETECTMODERSTREG0);
	writel(alive_save->ALIVEGPIOFALLDETECTMODEREADREG,
				&reg->ALIVEGPIODETECTMODESETREG0);

	writel((u32)0xFFFFFFFF, &reg->ALIVEGPIODETECTMODERSTREG1);
	writel(alive_save->ALIVEGPIORISEDETECTMODEREADREG,
				&reg->ALIVEGPIODETECTMODESETREG1);

	writel((u32)0xFFFFFFFF, &reg->ALIVEGPIODETECTMODERSTREG2);
	writel(alive_save->ALIVEGPIOLOWDETECTMODEREADREG,
				&reg->ALIVEGPIODETECTMODESETREG2);

	writel((u32)0xFFFFFFFF, &reg->ALIVEGPIODETECTMODERSTREG3);
	writel(alive_save->ALIVEGPIOHIGHDETECTMODEREADREG,
				&reg->ALIVEGPIODETECTMODESETREG3);

	writel(alive_save->ALIVEGPIODETECTENBREADREG,
				&reg->ALIVEGPIODETECTENBSETREG);
	writel(alive_save->ALIVEGPIOINTENBREADREG,
				&reg->ALIVEGPIOINTENBSETREG);

	return 0;
}

int s5pxx18_gpio_device_init(struct list_head *banks, int nr_banks)
{
	struct module_init_data *init_data;
	int n = ALIVE_INDEX + 1;
	int i;

	for (i = 0; n > i; i++)
		IO_LOCK_INIT(i);

	gpio_fn_no = s5pxx18_pio_fn_no;

	i = 0;
	list_for_each_entry(init_data, banks, node) {
		if (init_data->bank_type == 1) { /* gpio */
			gpio_modules[i].gpio_regs =
			    (struct nx_gpio_reg_set *)(init_data->bank_base);

			nx_gpio_open_module(i);
			i++;
		} else if (init_data->bank_type == 2) { /* alive */
			alive_regs =
			    (struct nx_alive_reg_set *)(init_data->bank_base);

			/*
			 * ALIVE Power Gate must enable for RTC register access.
			 * must be clear wfi jump address
			 */
			nx_alive_set_write_enable(true);
		}
	}

	return 0;
}

/*
 * irq_chip functions
 */

static void irq_gpio_ack(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	writel((1 << bit), base + GPIO_INT_STATUS); /* irq pend clear */
	ARM_DMB();
}

static void irq_gpio_mask(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	/* mask:irq disable */
	writel(readl(base + GPIO_INT_ENB) & ~(1 << bit), base + GPIO_INT_ENB);
	writel(readl(base + GPIO_INT_DET) & ~(1 << bit), base + GPIO_INT_DET);
}

static void irq_gpio_unmask(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	/* unmask:irq enable */
	writel(readl(base + GPIO_INT_ENB) | (1 << bit), base + GPIO_INT_ENB);
	writel(readl(base + GPIO_INT_DET) | (1 << bit), base + GPIO_INT_DET);
	ARM_DMB();
}

static int irq_gpio_set_type(struct irq_data *irqd, unsigned int type)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;
	u32 val, alt;
	ulong reg;

	int mode = 0;

	pr_debug("%s: gpio irq=%d, %s.%d, type=0x%x\n", __func__, bank->irq,
		 bank->name, bit, type);

	switch (type) {
	case IRQ_TYPE_NONE:
		pr_warn("%s: No edge setting!\n", __func__);
		break;
	case IRQ_TYPE_EDGE_RISING:
		mode = NX_GPIO_INTMODE_RISINGEDGE;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		mode = NX_GPIO_INTMODE_FALLINGEDGE;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		mode = NX_GPIO_INTMODE_BOTHEDGE;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		mode = NX_GPIO_INTMODE_LOWLEVEL;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		mode = NX_GPIO_INTMODE_HIGHLEVEL;
		break;
	default:
		pr_err("%s: No such irq type %d", __func__, type);
		return -1;
	}

	/*
	 * must change mode to gpio to use gpio interrupt
	 */

	/* gpio out : output disable */
	writel(readl(base + GPIO_INT_OUT) & ~(1 << bit), base + GPIO_INT_OUT);

	/* gpio mode : interrupt mode */
	reg = (ulong)(base + GPIO_INT_MODE0 + (bit / 16) * 4);
	val = (readl((void *)reg) & ~(3 << ((bit & 0xf) * 2))) |
	      ((mode & 0x3) << ((bit & 0xf) * 2));
	writel(val, (void *)reg);

	reg = (ulong)(base + GPIO_INT_MODE1);
	val = (readl((void *)reg) & ~(1 << bit)) | (((mode >> 2) & 0x1) << bit);
	writel(val, (void *)reg);

	/* gpio alt : gpio mode for irq */
	reg = (ulong)(base + GPIO_INT_ALT + (bit / 16) * 4);
	val = readl((void *)reg) & ~(3 << ((bit & 0xf) * 2));
	alt = nx_soc_gpio_get_altnum(bank->grange.pin_base + bit);
	val |= alt << ((bit & 0xf) * 2);
	writel(val, (void *)reg);
	pr_debug("%s: set func to gpio. alt:%d, base:%d, bit:%d\n", __func__,
		 alt, bank->grange.pin_base, bit);

	return 0;
}

static void irq_gpio_enable(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	/* unmask:irq enable */
	writel(readl(base + GPIO_INT_ENB) | (1 << bit), base + GPIO_INT_ENB);
	writel(readl(base + GPIO_INT_DET) | (1 << bit), base + GPIO_INT_DET);
}

static void irq_gpio_disable(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	/* mask:irq disable */
	writel(readl(base + GPIO_INT_ENB) & ~(1 << bit), base + GPIO_INT_ENB);
	writel(readl(base + GPIO_INT_DET) & ~(1 << bit), base + GPIO_INT_DET);
}

/*
 * irq_chip for gpio interrupts.
 */
static struct irq_chip s5pxx18_gpio_irq_chip = {
	.name = "GPIO",
	.irq_ack = irq_gpio_ack,
	.irq_mask = irq_gpio_mask,
	.irq_unmask = irq_gpio_unmask,
	.irq_set_type = irq_gpio_set_type,
	.irq_enable = irq_gpio_enable,
	.irq_disable = irq_gpio_disable,
	.flags = IRQCHIP_SKIP_SET_WAKE,
};

static int s5pxx18_gpio_irq_map(struct irq_domain *h, unsigned int virq,
				irq_hw_number_t hw)
{
	struct nexell_pin_bank *b = h->host_data;

	pr_debug("%s domain map: virq %d and hw %d\n", __func__, virq, (int)hw);

	irq_set_chip_data(virq, b);
	irq_set_chip_and_handler(virq, &s5pxx18_gpio_irq_chip,
				 handle_level_irq);
	return 0;
}

/*
 * irq domain callbacks for external gpio interrupt controller.
 */
static const struct irq_domain_ops s5pxx18_gpio_irqd_ops = {
	.map = s5pxx18_gpio_irq_map, .xlate = irq_domain_xlate_twocell,
};

static irqreturn_t s5pxx18_gpio_irq_handler(int irq, void *data)
{
	struct nexell_pin_bank *bank = data;
	void __iomem *base = bank->virt_base;
	u32 stat, mask;
	unsigned int virq;
	int bit;

	mask = readl(base + GPIO_INT_ENB);
	stat = readl(base + GPIO_INT_STATUS) & mask;
	bit = ffs(stat) - 1;

	if (-1 == bit) {
		pr_err("Unknown gpio irq=%d, status=0x%08x, mask=0x%08x\r\n",
		       irq, stat, mask);
		writel(-1, (base + GPIO_INT_STATUS)); /* clear gpio status all*/
		return IRQ_NONE;
	}

	virq = irq_linear_revmap(bank->irq_domain, bit);
	if (!virq)
		return IRQ_NONE;

	pr_debug("Gpio irq=%d [%d] (hw %u), stat=0x%08x, mask=0x%08x\n", irq,
		 bit, virq, stat, mask);
	generic_handle_irq(virq);

	return IRQ_HANDLED;
}

/*
 * s5pxx18_gpio_irq_init() - setup handling of external gpio interrupts.
 * @d: driver data of nexell pinctrl driver.
 */
static int s5pxx18_gpio_irq_init(struct nexell_pinctrl_drv_data *d)
{
	struct nexell_pin_bank *bank;
	struct device *dev = d->dev;
	int ret;
	int i;

	bank = d->ctrl->pin_banks;
	for (i = 0; i < d->ctrl->nr_banks; ++i, ++bank) {
		if (bank->eint_type != EINT_TYPE_GPIO)
			continue;

		ret = devm_request_irq(dev, bank->irq, s5pxx18_gpio_irq_handler,
				       0, dev_name(dev), bank);
		if (ret) {
			dev_err(dev, "irq request failed\n");
			ret = -ENXIO;
			goto err_domains;
		}

		bank->irq_domain = irq_domain_add_linear(
		    bank->of_node, bank->nr_pins, &s5pxx18_gpio_irqd_ops, bank);
		if (!bank->irq_domain) {
			dev_err(dev, "gpio irq domain add failed\n");
			ret = -ENXIO;
			goto err_domains;
		}
	}

	return 0;

err_domains:
	for (--i, --bank; i >= 0; --i, --bank) {
		if (bank->eint_type != EINT_TYPE_GPIO)
			continue;
		irq_domain_remove(bank->irq_domain);
		devm_free_irq(dev, bank->irq, d);
	}

	return ret;
}

static void irq_alive_ack(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: irq=%d, hwirq=%d\n", __func__, irqd->irq, bit);
	/* ack: irq pend clear */
	writel(1 << bit, base + ALIVE_INT_STATUS);

	ARM_DMB();
}

static void irq_alive_mask(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: irq=%d, hwirq=%d\n", __func__, irqd->irq, bit);
	/* mask: irq reset (disable) */
	writel((1 << bit), base + ALIVE_INT_RESET);
}

static void irq_alive_unmask(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: irq=%d, hwirq=%d\n", __func__, irqd->irq, bit);
	writel((1 << bit), base + ALIVE_INT_SET);
	ARM_DMB();
}

static int irq_alive_set_type(struct irq_data *irqd, unsigned int type)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;
	int offs = 0, i = 0;
	int mode = 0;

	pr_debug("%s: irq=%d, hwirq=%d, type=0x%x\n", __func__, irqd->irq, bit,
		 type);

	switch (type) {
	case IRQ_TYPE_NONE:
		pr_warn("%s: No edge setting!\n", __func__);
		break;
	case IRQ_TYPE_EDGE_FALLING:
		mode = NX_ALIVE_DETECTMODE_SYNC_FALLINGEDGE;
		break;
	case IRQ_TYPE_EDGE_RISING:
		mode = NX_ALIVE_DETECTMODE_SYNC_RISINGEDGE;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		mode = NX_ALIVE_DETECTMODE_SYNC_FALLINGEDGE;
		break; /* and Rising Edge */
	case IRQ_TYPE_LEVEL_LOW:
		mode = NX_ALIVE_DETECTMODE_ASYNC_LOWLEVEL;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		mode = NX_ALIVE_DETECTMODE_ASYNC_HIGHLEVEL;
		break;
	default:
		pr_err("%s: No such irq type %d", __func__, type);
		return -1;
	}

	/* setting all alive detect mode set/reset register */
	for (; 6 > i; i++, offs += 0x0C) {
		u32 reg = (i == mode ? ALIVE_MOD_SET : ALIVE_MOD_RESET);

		writel(1 << bit, (base + reg + offs));
	}

	/*
	 * set risingedge mode for both edge
	 * 0x2C : Risingedge
	 */
	if (IRQ_TYPE_EDGE_BOTH == type)
		writel(1 << bit, (base + 0x2C));

	writel(1 << bit, base + ALIVE_DET_SET);
	writel(1 << bit, base + ALIVE_INT_SET);
	writel(1 << bit, base + ALIVE_OUT_RESET);

	return 0;
}

static u32 alive_wake_mask = 0xffffffff;

u32 get_wake_mask(void)
{
	return alive_wake_mask;
}

static int irq_set_alive_wake(struct irq_data *irqd, unsigned int on)
{
	int bit = (int)(irqd->hwirq);

	pr_info("alive wake bit[%d] %s for irq %d\n",
		       bit, on ? "enabled" : "disabled", irqd->irq);

	if (!on)
		alive_wake_mask |= bit;
	else
		alive_wake_mask &= ~bit;

	return 0;
}

static void irq_alive_enable(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: irq=%d, io=%s.%d\n", __func__, bank->irq, bank->name,
		 bit);
	/* unmask:irq set (enable) */
	writel((1 << bit), base + ALIVE_INT_SET);
	ARM_DMB();
}

static void irq_alive_disable(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: irq=%d, io=%s.%d\n", __func__, bank->irq, bank->name,
		 bit);
	/* mask:irq reset (disable) */
	writel((1 << bit), base + ALIVE_INT_RESET);
}

/*
 * irq_chip for wakeup interrupts
 */
static struct irq_chip s5pxx18_alive_irq_chip = {
	.name = "ALIVE",
	.irq_ack = irq_alive_ack,
	.irq_mask = irq_alive_mask,
	.irq_unmask = irq_alive_unmask,
	.irq_set_type = irq_alive_set_type,
	.irq_set_wake = irq_set_alive_wake,
	.irq_enable = irq_alive_enable,
	.irq_disable = irq_alive_disable,
};

static irqreturn_t s5pxx18_alive_irq_handler(int irq, void *data)
{
	struct nexell_pin_bank *bank = data;
	void __iomem *base = bank->virt_base;
	u32 stat, mask;
	unsigned int virq;
	int bit;

	mask = readl(base + ALIVE_INT_SET_READ);
	stat = readl(base + ALIVE_INT_STATUS) & mask;
	bit = ffs(stat) - 1;

	if (-1 == bit) {
		pr_err("Unknown alive irq=%d, status=0x%08x, mask=0x%08x\r\n",
		       irq, stat, mask);
		writel(-1, (base + ALIVE_INT_STATUS)); /* clear alive status */
		return IRQ_NONE;
	}

	virq = irq_linear_revmap(bank->irq_domain, bit);
	pr_debug("alive irq=%d [%d] (hw %u), stat=0x%08x, mask=0x%08x\n", irq,
		 bit, virq, stat, mask);
	if (!virq)
		return IRQ_NONE;

	generic_handle_irq(virq);

	return IRQ_HANDLED;
}

static int s5pxx18_alive_irq_map(struct irq_domain *h, unsigned int virq,
				 irq_hw_number_t hw)
{
	pr_debug("%s domain map: virq %d and hw %d\n", __func__, virq, (int)hw);

	irq_set_chip_and_handler(virq, &s5pxx18_alive_irq_chip,
				 handle_level_irq);
	irq_set_chip_data(virq, h->host_data);
	return 0;
}

static const struct irq_domain_ops s5pxx18_alive_irqd_ops = {
	.map = s5pxx18_alive_irq_map, .xlate = irq_domain_xlate_twocell,
};

/*
 * s5pxx18_alive_irq_init() - setup handling of wakeup interrupts.
 * @d: driver data of nexell pinctrl driver.
 */
static int s5pxx18_alive_irq_init(struct nexell_pinctrl_drv_data *d)
{
	struct nexell_pin_bank *bank;
	struct device *dev = d->dev;
	int ret;
	int i;

	bank = d->ctrl->pin_banks;
	for (i = 0; i < d->ctrl->nr_banks; ++i, ++bank) {
		void __iomem *base = bank->virt_base;

		if (bank->eint_type != EINT_TYPE_WKUP)
			continue;

		/* clear pending, disable irq detect */
		writel(-1, base + ALIVE_INT_RESET);
		writel(-1, base + ALIVE_INT_STATUS);

		ret =
		    devm_request_irq(dev, bank->irq, s5pxx18_alive_irq_handler,
				     0, dev_name(dev), bank);
		if (ret) {
			dev_err(dev, "irq request failed\n");
			ret = -ENXIO;
			goto err_domains;
		}

		bank->irq_domain =
		    irq_domain_add_linear(bank->of_node, bank->nr_pins,
					  &s5pxx18_alive_irqd_ops, bank);
		if (!bank->irq_domain) {
			dev_err(dev, "gpio irq domain add failed\n");
			ret = -ENXIO;
			goto err_domains;
		}
	}

	return 0;

err_domains:
	for (--i, --bank; i >= 0; --i, --bank) {
		if (bank->eint_type != EINT_TYPE_WKUP)
			continue;
		irq_domain_remove(bank->irq_domain);
		devm_free_irq(dev, bank->irq, d);
	}

	return ret;
}

static void s5pxx18_suspend(struct nexell_pinctrl_drv_data *drvdata)
{
	struct nexell_pin_ctrl *ctrl = drvdata->ctrl;
	int nr_banks = ctrl->nr_banks;
	int i;

	for (i = 0; i < nr_banks; i++) {
		struct nexell_pin_bank *bank = &ctrl->pin_banks[i];

		if (bank->eint_type == EINT_TYPE_WKUP) {
			s5pxx18_alive_suspend();
			continue;
		}

		if (bank->eint_type != EINT_TYPE_GPIO)
			continue;

		if (s5pxx18_gpio_suspend(i) < 0)
			dev_err(drvdata->dev, "failed to suspend bank %d\n", i);
	}

	nx_alive_clear_wakeup_status();
}


static const char *wake_event_name[] = {
	[0] = "VDDPOWER",
	[1] = "RTC",
	[2] = "ALIVE 0",
	[3] = "ALIVE 1",
	[4] = "ALIVE 2",
	[5] = "ALIVE 3",
	[6] = "ALIVE 4",
	[7] = "ALIVE 5",
};

#define	WAKE_EVENT_NUM	ARRAY_SIZE(wake_event_name)

static void print_wake_event(void)
{
	int i = 0;
	u32 wake_status = nx_alive_get_wakeup_status();

	for (i = 0; WAKE_EVENT_NUM > i; i++) {
		if (wake_status & (1 << i))
			pr_notice("WAKE SOURCE [%s]\n", wake_event_name[i]);
	}
}

static void s5pxx18_resume(struct nexell_pinctrl_drv_data *drvdata)
{
	struct nexell_pin_ctrl *ctrl = drvdata->ctrl;
	int nr_banks = ctrl->nr_banks;
	int i;

	for (i = 0; i < nr_banks; i++) {
		struct nexell_pin_bank *bank = &ctrl->pin_banks[i];

		if (bank->eint_type == EINT_TYPE_WKUP) {
			s5pxx18_alive_resume();
			continue;
		}

		if (bank->eint_type != EINT_TYPE_GPIO)
			continue;

		if (s5pxx18_gpio_resume(i) < 0)
			dev_err(drvdata->dev, "failed to resume bank %d\n", i);
	}

	print_wake_event();
}

static int s5pxx18_base_init(struct nexell_pinctrl_drv_data *drvdata)
{
	struct nexell_pin_ctrl *ctrl = drvdata->ctrl;
	int nr_banks = ctrl->nr_banks;
	int ret;
	int i;
	struct module_init_data *init_data, *n;
	LIST_HEAD(banks);

	for (i = 0; i < nr_banks; i++) {
		struct nexell_pin_bank *bank = &ctrl->pin_banks[i];

		init_data = kmalloc(sizeof(*init_data), GFP_KERNEL);
		if (!init_data) {
			ret = -ENOMEM;
			goto done;
		}

		INIT_LIST_HEAD(&init_data->node);
		init_data->bank_base = bank->virt_base;
		init_data->bank_type = bank->eint_type;

		list_add_tail(&init_data->node, &banks);
	}

	s5pxx18_gpio_device_init(&banks, nr_banks);

done:
	/* free */
	list_for_each_entry_safe(init_data, n, &banks, node) {
		list_del(&init_data->node);
		kfree(init_data);
	}

	return 0;
}

/* pin banks of s5pxx18 pin-controller 0 */
static struct nexell_pin_bank s5pxx18_pin_banks[] = {
	SOC_PIN_BANK_EINTG(32, 0xA000, "gpioa"),
	SOC_PIN_BANK_EINTG(32, 0xB000, "gpiob"),
	SOC_PIN_BANK_EINTG(32, 0xC000, "gpioc"),
	SOC_PIN_BANK_EINTG(32, 0xD000, "gpiod"),
	SOC_PIN_BANK_EINTG(32, 0xE000, "gpioe"),
	SOC_PIN_BANK_EINTW(6, 0x0800, "alive"),
};

/*
 * Nexell pinctrl driver data for SoC.
 */
const struct nexell_pin_ctrl s5pxx18_pin_ctrl[] = {
	{
		.pin_banks = s5pxx18_pin_banks,
		.nr_banks = ARRAY_SIZE(s5pxx18_pin_banks),
		.base_init = s5pxx18_base_init,
		.gpio_irq_init = s5pxx18_gpio_irq_init,
		.alive_irq_init = s5pxx18_alive_irq_init,
		.suspend = s5pxx18_suspend,
		.resume = s5pxx18_resume,
	}
};
