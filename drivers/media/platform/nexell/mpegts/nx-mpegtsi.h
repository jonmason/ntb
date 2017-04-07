/*
 * Copyright (C) 2017  Nexell Co., Ltd.
 * Author: Jongkeun, Choi <jkchoi@nexell.co.kr>
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

#ifndef __NX_MPEGTSI_H__
#define __NX_MPEGTSI_H__

#define NUMBER_OF_MPEGTS_MODULE	1

struct  nx_mpegtsi_register_set {
	 u32 cap_ctrl[2];
	 u32 cpu_wrdata;
	 u32 cpu_wraddr;
	 u32 cap_data[2];
	 u32 tsp_indata;
	 u32 tsp_outdata;
	 u32 ctrl0;
	 u32 idma_enable;
	 u32 idma_run;
	 u32 idma_int;
	 u32 idma_addr[4];
	 u32 idma_len[4];
};

bool	nx_mpegtsi_initialize(void);
u32	nx_mpegtsi_get_number_of_module(void);

u32     nx_mpegtsi_get_physical_address(void);
u32     nx_mpegtsi_get_size_of_register_set(void);
void    nx_mpegtsi_set_base_address(void *base_address);
void	*nx_mpegtsi_get_base_address(void);
bool	nx_mpegtsi_open_module(void);
bool	nx_mpegtsi_close_module(void);
bool	nx_mpegtsi_check_busy(void);
bool	nx_mpegtsi_can_power_down(void);
void	nx_mpegtsi_enable_channel_pad(u32 ch_idx);

u32     nx_mpegtsi_get_clock_number(void);
u32     nx_mpegtsi_get_reset_number(void);

s32     nx_mpegtsi_get_interrupt_number(void);

u32     nx_mpegtsi_get_dma_index(u32 ch_idx);
u32     nx_mpegtsi_get_dma_bus_width(void);

void    nx_mpegtsi_set_idma_enable(u32 ch_idx, bool enable);
bool	nx_mpegtsi_get_idma_enable(u32 ch_idx);
bool	nx_mpegtsi_get_idma_busy_status(u32 ch_idx);
void    nx_mpegtsi_run_idma(u32 ch_idx);
void    nx_mpegtsi_stop_idma(u32 ch_idx);
void    nx_mpegtsi_set_idma_base_addr(u32 ch_idx, u32 base_addr);
u32     nx_mpegtsi_get_idma_base_addr(u32 ch_idx);
void    nx_mpegtsi_set_idma_length(u32 ch_idx, u32 length);
u32     nx_mpegtsi_get_idma_length(u32 ch_idx);
void    nx_mpegtsi_set_idma_int_enable(u32 ch_idx, bool enable);
u32     nx_mpegtsi_get_idma_int_enable(void);
void    nx_mpegtsi_set_idma_int_mask_clear(u32 ch_idx, bool unmask);
u32     nx_mpegtsi_get_idma_int_mask_clear(void);
bool	nx_mpegtsi_get_idma_int_status(u32 ch_idx);
u32     nx_mpegtsi_get_idma_int_raw_status(void);
void    nx_mpegtsi_set_idma_int_clear(u32 ch_idx);

void    nx_mpegtsi_set_cap_enable(u32 cap_idx, bool enable);
bool	nx_mpegtsi_get_cap_enable(u32 cap_idx);
void    nx_mpegtsi_set_bypass_enable(u32 cap_idx, bool enable);
bool	nx_mpegtsi_get_bypass_enable(u32 cap_idx);
void    nx_mpegtsi_set_serial_enable(u32 cap_idx, bool enable);
bool	nx_mpegtsi_get_serial_enable(u32 cap_idx);
void    nx_mpegtsi_set_tclk_polarity_enable(u32 cap_idx, bool enable);
bool	nx_mpegtsi_get_tclk_polarity_enable(u32 cap_idx);
void    nx_mpegtsi_set_tdp_polarity_enable(u32 cap_idx, bool enable);
bool	nx_mpegtsi_get_tdp_polarity_enable(u32 cap_idx);
void    nx_mpegtsi_set_tsync_polarity_enable(u32 cap_idx, bool enable);
bool	nx_mpegtsi_get_tsync_polarity_enable(u32 cap_idx);
void    nx_mpegtsi_set_terr_polarity_enable(u32 cap_idx, bool enable);
bool	nx_mpegtsi_get_terr_polarity_enable(u32 cap_idx);
void    nx_mpegtsi_set_cap_int_lock_enable(u32 cap_idx, bool enable);
void    nx_mpegtsi_set_cap_sram_wakeup(u32 cap_idx, bool wakeon);
bool	nx_mpegtsi_get_cap_sram_wakeup(u32 cap_idx);
void    nx_mpegtsi_set_cap_sram_power_enable(u32 cap_idx, bool enable);
bool	nx_mpegtsi_get_cap_sram_power_enable(u32 cap_idx);

void    nx_mpegtsi_set_cap1_output_enable(bool enable);
bool	nx_mpegtsi_get_cap1_output_enable(void);
void    nx_mpegtsi_set_cap1_out_tclk_polarity_enable(bool enable);
bool	nx_mpegtsi_get_cap1_out_tclk_polarity_enable(void);
bool	nx_mpegtsi_get_cap1_out_polarity_enable(void);

void    nx_mpegtsi_set_cap_int_lock_enable(u32 cap_idx, bool enable);
bool	nx_mpegtsi_get_cap_int_lock_enable(u32 cap_idx);
void    nx_mpegtsi_set_cap_int_enable(u32 cap_idx, bool enable);
bool	nx_mpegtsi_get_cap_int_enable(u32 cap_idx);
void    nx_mpegtsi_set_cap_int_mask_clear(u32 cap_idx, bool unmask);
bool	nx_mpegtsi_get_cap_int_mask_clear(u32 cap_idx);
void    nx_mpegtsi_set_cap_int_clear(u32 cap_idx);
bool	nx_mpegtsi_get_cap_int_status(u32 cap_idx);
u32     nx_mpegtsi_get_cap_fifo_data(u32 cap_idx);
void    nx_mpegtsi_set_cpuwr_data(u32 wr_data);
u32     nx_mpegtsi_get_cpuwr_data(void);
void    nx_mpegtsi_set_cpuwr_addr(u32 wr_addr);
void    nx_mpegtsi_set_tsi_enable(bool enable);
bool	nx_mpegtsi_get_tsi_enable(void);
void    nx_mpegtsi_set_tsi_encrypt(bool enable);
bool	nx_mpegtsi_get_tsi_encrypt(void);
void    nx_mpegtsi_set_tsi_sram_wakeup(bool wakeup);
bool	nx_mpegtsi_get_tsi_sram_wakeup(void);
void    nx_mpegtsi_set_tsi_sram_power_enable(bool enable);
bool	nx_mpegtsi_get_tsi_sram_power_enable(void);
void    nx_mpegtsi_set_tsi_int_enable(bool enable);
bool	nx_mpegtsi_get_tsi_int_enable(void);
void    nx_mpegtsi_set_tsi_int_mask_clear(bool enable);
bool	nx_mpegtsi_get_tsi_int_mask_clear(void);
void    nx_mpegtsi_set_tsi_int_clear(void);
bool	nx_mpegtsi_get_tsi_int_status(void);
u32     nx_mpegtsi_get_cap_data(u32 cap_idx);
u32     nx_mpegtsi_get_tsi_out_data(void);

u32     nx_mpegtsi_byte_swap(u32 data);
void    nx_mpegtsi_write_pid(u32 bank, u32 type, u32 pid_addr, u32 pid_data);
void    nx_mpegtsi_write_aeskey_iv(u32 cwaddr, u32 cw0, u32 cw1, u32 cw2,
				    u32 cw3, u32 iv0, u32 iv1, u32 iv2,
				    u32 iv3);
void    nx_mpegtsi_write_cas_cw(u32 cwaddr, u32 cw0, u32 cw1, u32 cw2,
				u32 cw3);
#endif
