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

#ifndef _CLK_S5P6818_PLL_H
#define _CLK_S5P6818_PLL_H

enum { ID_CPU_PLL0 = 0,
	ID_CPU_PLL1,
	ID_CPU_PLL2,
	ID_CPU_PLL3,
	ID_CPU_FCLK,
	ID_CPU_HCLK,
	ID_BUS_BCLK,
	ID_BUS_PCLK,
	ID_MEM_FCLK,
	ID_MEM_DCLK,
	ID_MEM_BCLK,
	ID_MEM_PCLK,
	ID_G3D_BCLK,
	ID_VPU_BCLK,
	ID_VPU_PCLK,
	ID_DIS_BCLK,
	ID_DIS_PCLK,
	ID_CCI_BCLK,
	ID_CCI_PCLK,
	ID_END,
};

struct reg_clkpwr {
	unsigned int CLKMODEREG0;
	unsigned int __Reserved0;
	unsigned int PLLSETREG[4];
	unsigned int __Reserved1[2];
	unsigned int DVOREG[9];
	unsigned int __Reserved2;
	unsigned int PLLSETREG_SSCG[6];
	unsigned int __reserved3[8];
	unsigned char __Reserved4[0x200 - 0x80];
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
	unsigned char __Reserved6[0x2A0 - 0x240];
	unsigned int CPUPOWERDOWNREQ;
	unsigned int CPUPOWERONREQ;
	unsigned int CPURESETMODE;
	unsigned int CPUWARMRESETREQ;
	unsigned int __Reserved7;
	unsigned int CPUSTATUS;
	unsigned char __Reserved8[0x400 - 0x2B8];
};

#define to_clk_core(_hw) container_of(_hw, struct clk_core, hw)

#endif
