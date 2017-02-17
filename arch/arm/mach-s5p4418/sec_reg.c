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

#include <linux/init.h>
#include <asm/compiler.h>
#include <asm/errno.h>
#include <asm/system_misc.h>
#include <linux/kernel.h>
#include <linux/io.h>

#define NEXELL_SMC_BASE			0x82000000

#define NEXELL_SMC_FN(n)		(NEXELL_SMC_BASE +  (n))

#define NEXELL_SMC_SEC_REG_WRITE	NEXELL_SMC_FN(0)
#define NEXELL_SMC_SEC_REG_READ		NEXELL_SMC_FN(1)

asmlinkage int __invoke_nexell_fn_smc(u32, u32, u32, u32);

int write_sec_reg(void __iomem *reg, int val)
{
	int ret = 0;
	ret = __invoke_nexell_fn_smc(NEXELL_SMC_SEC_REG_WRITE,
			(u32)reg, val, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(write_sec_reg);

int read_sec_reg(void __iomem *reg)
{
	int ret = 0;
	ret = __invoke_nexell_fn_smc(NEXELL_SMC_SEC_REG_READ,
			(u32)reg, 0, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(read_sec_reg);
