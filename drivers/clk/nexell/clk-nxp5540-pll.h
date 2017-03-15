/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Youngbok, Park <ybpark@nexell.co.kr>
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

#ifndef _CLK_NXP5540_PLL_H
#define _CLK_NXP5540_PLL_H

#define CLK_CPU_PLL0 "sys-pll0"
#define CLK_CPU_PLL1 "sys-pll1"
#define CLK_CPU_PLL2 "sys-pll2"
#define CLK_CPU_PLL3 "sys-pll3"
#define CLK_CPU_PLL4 "sys-pll4"
#define CLK_CPU_PLL5 "sys-pll5"
#define CLK_CPU_PLL6 "sys-pll6"
#define CLK_CPU_PLL7 "sys-pll7"


enum {
	ID_CPU_PLL0 = 0,
	ID_CPU_PLL1,
	ID_CPU_PLL2,
	ID_CPU_PLL3,
	ID_CPU_PLL4,
	ID_CPU_PLL5,
	ID_CPU_PLL6,
	ID_CPU_PLL7,
	ID_END,
};

struct  reg_clkpwr {
	unsigned int CLKMODEREG0;
	unsigned int CLKMODEREG1;
	unsigned int PLLSETREG[8];
	unsigned int __Reserved1[8];
	unsigned int PLLSETREG_SSCG[8];
	unsigned int __reserved2[6];
	unsigned char __Reserved3[0x200 - 0x80];
	unsigned int GPIOWAKEUPRISEENB;
	unsigned int GPIOWAKEUPFALLENB;
	unsigned int GPIORSTENB;
	unsigned int GPIOWAKEUPENB;
	unsigned int GPIOINTENB;
	unsigned int GPIOINTPEND;
	unsigned int RESETSTATUS;
	unsigned int INTENABLE;
	unsigned int INTPEND;
	unsigned int PWRCONT;
	unsigned int PWRMODE;
	unsigned int __Reserved5;
	unsigned int SCRATCH[3];
	unsigned int SYSRSTCONFIG;
	unsigned int __Reserved6[0x100-0x80];
	unsigned int PADSTRENGTHGPIO[5][2];
	unsigned int __Reserved7[2];
	unsigned int PADSTRENGTHBUS;
};

#define to_clk_core(_hw) container_of(_hw, struct clk_core, hw)

#endif
