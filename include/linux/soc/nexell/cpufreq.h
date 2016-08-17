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

/* BUS CLOCK */
#define NX_BUS_CLK_HIGH_KHZ     400000
#define NX_BUS_CLK_MID_KHZ      150000
#define NX_BUS_CLK_LOW_KHZ      100000

/* defines for per IP */
#define NX_BUS_CLK_GPU_KHZ	NX_BUS_CLK_HIGH_KHZ
#define NX_BUS_CLK_VPU_KHZ	NX_BUS_CLK_HIGH_KHZ
#define NX_BUS_CLK_MMC_KHZ	NX_BUS_CLK_HIGH_KHZ
#define NX_BUS_CLK_SPI_KHZ	NX_BUS_CLK_HIGH_KHZ
#define NX_BUS_CLK_VIP_KHZ	NX_BUS_CLK_MID_KHZ
#define NX_BUS_CLK_DISP_KHZ	NX_BUS_CLK_MID_KHZ
#define NX_BUS_CLK_AUDIO_KHZ	NX_BUS_CLK_MID_KHZ
#define NX_BUS_CLK_IDLE_KHZ	NX_BUS_CLK_LOW_KHZ


void nx_bus_qos_update(int val);
int  nx_bus_add_notifier(void *data);
void nx_bus_remove_notifier(void *data);

#endif
