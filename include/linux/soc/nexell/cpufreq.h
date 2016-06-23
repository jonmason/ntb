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
#ifndef __ARCH_CPUFREQ_H__
#define __ARCH_CPUFREQ_H__

/*
 *    CPU Freq platform data
 */
struct nxp_cpufreq_plat_data {
	int pll_dev;                    /* core pll : 0, 1, 2, 3 */
	unsigned long (*dvfs_table)[2]; /* [freq KHz].[u volt] */
	int  table_size;
	char *supply_name;		/* voltage regulator name */
	long supply_delay_us;
};

void nx_pll_set_rate(int PLL, int P, int M, int S);

#endif
