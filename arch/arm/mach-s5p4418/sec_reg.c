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
#include <linux/soc/nexell/sec_reg.h>

#define NEXELL_SMC_BASE			0x82000000

#define NEXELL_SMC_FN(n)		(NEXELL_SMC_BASE +  (n))

#define NEXELL_SMC_SEC_REG_WRITE	NEXELL_SMC_FN(0x0)
#define NEXELL_SMC_SEC_REG_READ		NEXELL_SMC_FN(0x1)

#define SECURE_FILTER_SHIFT		8

#define SEC_4K_OFFSET			((4 * 1024)-1)
#define SEC_64K_OFFSET			((64 * 1024)-1)

asmlinkage int __invoke_nexell_fn_smc(u32, u32, u32, u32);

int write_sec_reg_by_id(void __iomem *reg, int val, int id)
{
	int ret = 0;
	u32 off;
	switch (id) {
		case NEXELL_L2C_SEC_ID:
		case NEXELL_MIPI_SEC_ID:
		case NEXELL_TOFF_SEC_ID:
			off = (u32)reg & SEC_4K_OFFSET;
			break;
		case NEXELL_MALI_SEC_ID:
			off = (u32)reg & SEC_64K_OFFSET;
			break;
	}
	ret = __invoke_nexell_fn_smc(NEXELL_SMC_SEC_REG_WRITE +
			((1 << SECURE_FILTER_SHIFT) + id), off, val, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(write_sec_reg_by_id);

int read_sec_reg_by_id(void __iomem *reg, int id)
{
	int ret = 0;
	u32 off;
	switch (id) {
		case NEXELL_L2C_SEC_ID:
		case NEXELL_MIPI_SEC_ID:
		case NEXELL_TOFF_SEC_ID:
			off = (u32)reg & SEC_4K_OFFSET;
			break;
		case NEXELL_MALI_SEC_ID:
			off = (u32)reg & SEC_64K_OFFSET;
			break;
	}
	ret = __invoke_nexell_fn_smc(NEXELL_SMC_SEC_REG_READ +
			((1 << SECURE_FILTER_SHIFT) + id), off, 0, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(read_sec_reg_by_id);

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
	ret = __invoke_nexell_fn_smc(NEXELL_SMC_SEC_REG_READ, (u32)reg, 0, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(read_sec_reg);
