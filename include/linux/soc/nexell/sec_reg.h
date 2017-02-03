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

#ifdef CONFIG_SECURE_REG_ACCESS

#define NEXELL_L2C_SEC_ID	0
#define NEXELL_MALI_SEC_ID	2
#define NEXELL_MIPI_SEC_ID	4
#define NEXELL_TOFF_SEC_ID	6

int write_sec_reg_by_id(void __iomem *reg, int val, int id);
int read_sec_reg_by_id(void __iomem *reg, int id);
int write_sec_reg(void __iomem *reg, int val);
int read_sec_reg(void __iomem *reg);
#endif

