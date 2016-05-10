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
#include <linux/module.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/types.h>

#include <linux/soc/nexell/s5pxx18-gpio.h>

static struct {
	struct nx_gpio_reg_set *gpio_regs;
} gpio_modules[NUMBER_OF_GPIO_MODULE];

static struct nx_alive_reg_set *alive_regs;

/*
 * start of nx_gpio
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

u32 nx_gpio_get_number_of_module(void) { return NUMBER_OF_GPIO_MODULE; }

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

u32 nx_gpio_get_detect_enable32(u32 idx)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	return readl(&p_register->GPIOxDETENB);
}

void nx_gpio_set_detect_enable(u32 idx, u32 bitnum, bool enable)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	nx_gpio_setbit(&p_register->GPIOxDETENB, bitnum, enable);
}

void nx_gpio_set_detect_enable32(u32 idx, u32 enable)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	writel(enable, &p_register->GPIOxDETENB);
}

bool nx_gpio_get_output_enable(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	return nx_gpio_getbit(readl(&p_register->GPIOxOUTENB), bitnum);
}

void nx_gpio_set_output_enable32(u32 idx, bool enable)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	if (enable)
		writel(0xFFFFFFFF, &p_register->GPIOxOUTENB);
	else
		writel(0x0, &p_register->GPIOxOUTENB);
}

u32 nx_gpio_get_output_enable32(u32 idx)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	return readl(&p_register->GPIOxOUTENB);
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

void nx_gpio_set_output_value32(u32 idx, u32 value)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	writel(value, &p_register->GPIOxOUT);
}

u32 nx_gpio_get_output_value32(u32 idx)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	return readl(&p_register->GPIOxOUT);
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

void nx_gpio_set_pull_select32(u32 idx, u32 value)
{
	writel(value, &gpio_modules[idx].gpio_regs->GPIOx_PULLSEL);
}

bool nx_gpio_get_pull_select(u32 idx, u32 bitnum)
{
	return nx_gpio_getbit(gpio_modules[idx].gpio_regs->GPIOx_PULLSEL,
			      bitnum);
}

u32 nx_gpio_get_pull_select32(u32 idx)
{
	return gpio_modules[idx].gpio_regs->GPIOx_PULLSEL;
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

void nx_gpio_set_pad_function32(u32 idx, u32 msbvalue, u32 lsbvalue)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	writel(lsbvalue, &p_register->GPIOxALTFN[0]);
	writel(msbvalue, &p_register->GPIOxALTFN[1]);
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

void nx_gpio_set_slew32(u32 idx, u32 value)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	writel(value, &p_register->GPIOx_SLEW);
}

u32 nx_gpio_get_slew32(u32 idx)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	return readl(&p_register->GPIOx_SLEW);
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

void nx_gpio_set_pull_enable32(u32 idx, u32 pullenb, u32 pullsel)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	writel(pullsel, &p_register->GPIOx_PULLSEL);
	writel(pullenb, &p_register->GPIOx_PULLENB);
}

void nx_gpio_get_pull_enable32(u32 idx, u32 *pullenb, u32 *pullsel)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	*pullenb = readl(&p_register->GPIOx_PULLENB);
	*pullsel = readl(&p_register->GPIOx_PULLSEL);
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

void nx_gpio_set_interrupt_enable32(u32 idx, u32 enable)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	writel(enable, &p_register->GPIOxINTENB);

	nx_gpio_set_detect_enable32(idx, enable);
}

u32 nx_gpio_get_interrupt_enable32(u32 idx)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	return (u32)readl(&p_register->GPIOxINTENB);
}

bool nx_gpio_get_interrupt_pending(u32 idx, s32 irqnum)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	if (readl(&p_register->GPIOxDET) & (1UL << irqnum))
		return true;

	return false;
}

u32 nx_gpio_get_interrupt_pending32(u32 idx)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	return (u32)readl(&p_register->GPIOxDET);
}

void nx_gpio_clear_interrupt_pending(u32 idx, s32 irqnum)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	writel(1UL << irqnum, &p_register->GPIOxDET);
}

void nx_gpio_clear_interrupt_pending32(u32 idx, u32 pending)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	writel(pending, &p_register->GPIOxDET);
}

void nx_gpio_set_interrupt_enable_all(u32 idx, bool enable)
{
	struct nx_gpio_reg_set *p_register;
	u32 setvalue;

	p_register = gpio_modules[idx].gpio_regs;

	if (enable)
		setvalue = 0xFFFFFFFF;
	else
		setvalue = 0x0;

	writel(setvalue, &p_register->GPIOxINTENB);

	nx_gpio_set_interrupt_enable32(idx, setvalue);
}

bool nx_gpio_get_interrupt_enable_all(u32 idx)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	if (readl(&p_register->GPIOxINTENB))
		return true;

	return false;
}

bool nx_gpio_get_interrupt_pending_all(u32 idx)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	if (readl(&p_register->GPIOxDET))
		return true;

	return false;
}

void nx_gpio_clear_interrupt_pending_all(u32 idx)
{
	struct nx_gpio_reg_set *p_register;

	p_register = gpio_modules[idx].gpio_regs;

	writel((u32)(0xFFFFFFFF), &p_register->GPIOxDET);
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
 * end of nx_gpio
 */

/*
 * start of nx_alive
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

void nx_alive_set_interrupt_enable32(u32 enable)
{
	u32 Set_Mask;
	u32 ReSet_Mask;

	Set_Mask = enable & 0x03F;
	ReSet_Mask = (~enable & 0x03F);

	writel(ReSet_Mask, &alive_regs->ALIVEGPIOINTENBRSTREG);
	writel(Set_Mask, &alive_regs->ALIVEGPIOINTENBSETREG);
}

u32 nx_alive_get_interrupt_enable32(void)
{
	return (u32)(alive_regs->ALIVEGPIOINTENBREADREG & 0x03F);
}

bool nx_alive_get_interrupt_pending(s32 irqnum)
{
	return ((alive_regs->ALIVEGPIODETECTPENDREG >> irqnum) & 0x01);
}

u32 nx_alive_get_interrupt_pending32(void)
{
	return (u32)(alive_regs->ALIVEGPIODETECTPENDREG & 0x03F);
}

void nx_alive_clear_interrupt_pending(s32 irqnum)
{
	writel(1 << irqnum, &alive_regs->ALIVEGPIODETECTPENDREG);
}

void nx_alive_clear_interrupt_pending32(u32 pending)
{
	writel(pending & 0x03F, &alive_regs->ALIVEGPIODETECTPENDREG);
}

void nx_alive_set_interrupt_enable_all(bool enable)
{
	if (enable)
		writel(0x03F, &alive_regs->ALIVEGPIOINTENBSETREG);
	else
		writel(0x03F, &alive_regs->ALIVEGPIOINTENBRSTREG);
}

bool nx_alive_get_interrupt_enable_all(void)
{
	const u32 INTENB_MASK = 0x03F;

	if (alive_regs->ALIVEGPIOINTENBREADREG & INTENB_MASK)
		return true;

	return false;
}

bool nx_alive_get_interrupt_pending_all(void)
{
	const u32 ALIVEGPIODETECTPEND_MASK = 0x03F;

	if (alive_regs->ALIVEGPIODETECTPENDREG & ALIVEGPIODETECTPEND_MASK)
		return true;

	return false;
}

void nx_alive_clear_interrupt_pending_all(void)
{
	const u32 ALIVEGPIODETECTPEND_MASK = 0x03F;

	writel(ALIVEGPIODETECTPEND_MASK, &alive_regs->ALIVEGPIODETECTPENDREG);
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

void nx_alive_set_pullup_enable32(u32 value32)
{
	writel(~value32, &alive_regs->ALIVEGPIOPADPULLUPRSTREG);
	writel(value32, &alive_regs->ALIVEGPIOPADPULLUPSETREG);
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

void nx_alive_set_detect_enable32(u32 value32)
{
	writel(value32, &alive_regs->ALIVEGPIODETECTENBSETREG);
	writel(~value32, &alive_regs->ALIVEGPIODETECTENBRSTREG);
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

void nx_alive_set_detect_mode32(int detect_mode, u32 value32)
{
	u32 i;

	for (i = 0; i < nx_alive_number_of_gpio; i++)
		nx_alive_set_detect_mode(detect_mode, i,
					 (bool)((value32 >> i) & 1));
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

void nx_alive_set_output_enable32(u32 value)
{
	writel(~value, &alive_regs->ALIVEGPIOPADOUTENBRSTREG);
	writel(value, &alive_regs->ALIVEGPIOPADOUTENBSETREG);
}

void nx_alive_set_input_enable32(u32 value)
{
	writel(~value, &alive_regs->ALIVEGPIOPADOUTENBSETREG);
	writel(value, &alive_regs->ALIVEGPIOPADOUTENBRSTREG);
}

bool nx_alive_get_output_enable(u32 bitnum)
{
	return (bool)((alive_regs->ALIVEGPIOPADOUTENBREADREG >> bitnum) &
		      0x01);
}

u32 nx_alive_get_output_enable32(void)
{
	return alive_regs->ALIVEGPIOPADOUTENBREADREG & 0x3F;
}

bool nx_alive_get_input_enable(u32 bitnum)
{
	return (bool)((alive_regs->ALIVEGPIOPADOUTENBREADREG >> bitnum) &
		      0x01)
		   ? false
		   : true;
}

u32 nx_alive_get_input_enable32(void)
{
	return (~alive_regs->ALIVEGPIOPADOUTENBREADREG) & 0x3F;
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

void nx_alive_set_output_high32(u32 value32)
{
	writel(~value32, &alive_regs->ALIVEGPIOPADOUTRSTREG);
	writel(value32, &alive_regs->ALIVEGPIOPADOUTSETREG);
}


bool nx_alive_get_output_value(u32 bitnum)
{
	return (bool)((alive_regs->ALIVEGPIOPADOUTREADREG >> bitnum) & 0x01);
}

u32 nx_alive_get_output_value32(void)
{
	return alive_regs->ALIVEGPIOPADOUTREADREG;
}

bool nx_alive_get_input_value(u32 bitnum)
{
	return (bool)((alive_regs->ALIVEGPIOINPUTVALUE >> bitnum) & 0x01);
}

u32 nx_alive_get_input_value32(void) { return alive_regs->ALIVEGPIOINPUTVALUE; }

void nx_alive_set_vdd_pwron(bool vddpwron, bool vddpwron_ddr)
{
	const u32 VDDPWRON = (1UL << 0);
	const u32 VDDPWRON_DDR = (1UL << 1);

	u32 regset = 0;
	u32 regrst = 0;

	if (vddpwron)
		regset |= VDDPWRON;
	else
		regrst |= VDDPWRON;

	if (vddpwron_ddr)
		regset |= VDDPWRON_DDR;
	else
		regrst |= VDDPWRON_DDR;

	writel(regset, &alive_regs->VDDCTRLSETREG);
	writel(regrst, &alive_regs->VDDCTRLRSTREG);
}

bool nx_alive_get_vdd_pwron(void)
{
	const u32 VDDPWRON = (1UL << 0);

	return (alive_regs->VDDCTRLREADREG & VDDPWRON) ? true : false;
}

bool nx_alive_get_vdd_pwron_ddr(void)
{
	const u32 VDDPWRON_DDR = (1UL << 1);

	return (alive_regs->VDDCTRLREADREG & VDDPWRON_DDR) ? true : false;
}

u32 nx_alive_get_core_poweroff_delay_time(void)
{
	if (alive_regs->VDDOFFCNTVALUE1 != alive_regs->VDDOFFCNTVALUE0)
		return 0xffffffff;
	else
		return alive_regs->VDDOFFCNTVALUE1;
}

void nx_alive_set_core_poweroff_delay_time(u32 delay)
{
	alive_regs->VDDOFFCNTVALUERST = 0xffffffff;
	alive_regs->VDDOFFCNTVALUESET = delay;
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
