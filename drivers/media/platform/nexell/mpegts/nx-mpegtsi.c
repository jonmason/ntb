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
#include <linux/types.h>
#include <linux/io.h>

#include "nx-mpegtsi.h"

static struct nx_mpegtsi_register_set *__g_p_register;

bool nx_mpegtsi_initialize(void)
{
	static bool binit;

	if (false == binit)
		binit = true;

	return true;
}

u32 nx_mpegtsi_get_number_of_module(void)
{
	return  NUMBER_OF_MPEGTS_MODULE;
}

u32 nx_mpegtsi_get_physical_address(void)
{
	const u32 pysical_addr[NUMBER_OF_MPEGTS_MODULE] = { 0xc005d000 };

	return pysical_addr[0];
}

u32 nx_mpegtsi_get_size_of_register_set(void)
{
	return sizeof(struct nx_mpegtsi_register_set);
}

void nx_mpegtsi_set_base_address(void *base_address)
{
	__g_p_register = (struct nx_mpegtsi_register_set *)base_address;
}

void *nx_mpegtsi_get_base_address(void)
{
	return (void *)__g_p_register;
}

bool nx_mpegtsi_open_module(void)
{
	return true;
}

bool  nx_mpegtsi_close_module(void)
{
	return true;
}

bool nx_mpegtsi_check_busy(void)
{
	return false;
}

bool nx_mpegtsi_can_power_down(void)
{
	return true;
}

u32 nx_mpegtsi_get_clock_number(void)
{
	static const u32 clkgen_mpegtsi_list[] = {12};

	return clkgen_mpegtsi_list[0];
}

u32    nx_mpegtsi_get_reset_number(void)
{
	const u32 rstcon_mpegtsi_list[] = {34};

	return rstcon_mpegtsi_list[0];
}

s32 nx_mpegtsi_get_interrupt_number(void)
{
	const u32 mpegtsi_int_num_list[NUMBER_OF_MPEGTS_MODULE] = {32};

	return mpegtsi_int_num_list[0];
}

u32 nx_mpegtsi_get_dma_index(u32 channel_index)
{
	const u32 mpegtsi_dma_number[4] = {23, 24, 25, 26};

	return mpegtsi_dma_number[channel_index];
}

u32 nx_mpegtsi_get_dma_bus_width(void)
{
	return 32;
}

void nx_mpegtsi_set_idma_enable(u32 channel_index, bool enable)
{
	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;
	u32 idma_mask;

	p_register = __g_p_register;

	idma_mask = 1ul << channel_index;

	reg_val = p_register->idma_enable;
	reg_val &= ~idma_mask;
	reg_val |= enable << channel_index;

	writel(reg_val, &p_register->idma_enable);
}

bool nx_mpegtsi_get_idma_enable(u32 channel_index)
{
	u32 idma_bit;


	idma_bit = 1ul << channel_index;

	return (bool)((__g_p_register->idma_enable & idma_bit)
				>> channel_index);
}

bool nx_mpegtsi_get_idmabusy_status(u32 channel_index)
{
	u32 idma_bit;

	idma_bit = 1ul << (channel_index+16);

	return (bool)((__g_p_register->idma_enable & idma_bit)
				>> (channel_index+16));
}

void nx_mpegtsi_run_idma(u32 channel_index)
{
	register struct nx_mpegtsi_register_set *p_register;
	u32 idma_mask;

	p_register = __g_p_register;
	idma_mask = 1ul << channel_index;

	writel(idma_mask, &p_register->idma_run);
}

void nx_mpegtsi_stop_idma(u32 channel_index)
{
	register struct nx_mpegtsi_register_set *p_register;
	u32 idma_mask;

	p_register = __g_p_register;
	idma_mask = 1ul << (channel_index+16);

	writel(idma_mask, &p_register->idma_run);
}

void nx_mpegtsi_set_idma_base_addr(u32 channel_index, u32 base_addr)
{
	register struct nx_mpegtsi_register_set *p_register;

	p_register = __g_p_register;

	writel(base_addr, &p_register->idma_addr[channel_index]);
}

u32 nx_mpegtsi_get_idma_base_addr(u32 channel_index)
{

	return (u32)__g_p_register->idma_addr[channel_index];
}

void nx_mpegtsi_set_idma_length(u32 channel_index, u32 length)
{
	register struct nx_mpegtsi_register_set *p_register;

	p_register   = __g_p_register;

	writel(length, &p_register->idma_len[channel_index]);
}

u32 nx_mpegtsi_get_idma_length(u32 channel_index)
{
	return (u32)__g_p_register->idma_len[channel_index];
}

void nx_mpegtsi_set_idma_int_enable(u32 channel_index, bool enable)
{
	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;
	u32 idma_mask;

	p_register = __g_p_register;

	idma_mask = 1ul << (channel_index+16);

	reg_val = p_register->idma_int;
	reg_val &= ~idma_mask;
	reg_val |= enable << (channel_index+16);

	writel(reg_val, &p_register->idma_int);
}

u32 nx_mpegtsi_get_idma_int_enable(void)
{
	return (u32)(((__g_p_register->idma_int)
				>> 16) & 0xF);
}

void nx_mpegtsi_set_idma_int_mask_clear(u32 channel_index, bool unmask)
{
	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;
	u32 idma_mask;

	p_register = __g_p_register;

	idma_mask = 1ul << (channel_index + 24);

	reg_val = p_register->idma_int;
	reg_val &= ~idma_mask;
	reg_val |= unmask << (channel_index + 24);

	writel(reg_val, &p_register->idma_int);
}

u32 nx_mpegtsi_get_idma_int_mask_clear(void)
{
	return (u32)(((__g_p_register->idma_int)
				>> 24) & 0xF);
}

bool nx_mpegtsi_get_idma_int_status(u32 channel_index)
{
	u32 idma_mask;

	idma_mask = 1ul << (channel_index + 8);

	return (bool)((__g_p_register->idma_int & idma_mask)
					>> (channel_index + 8));
}

u32 nx_mpegtsi_get_idma_int_raw_status(void)
{
	return (u32)((__g_p_register->idma_int
					>> 8) & 0xF);
}

void nx_mpegtsi_set_idma_int_clear(u32 channel_index)
{
	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;
	u32 idma_mask;

	p_register = __g_p_register;

	idma_mask = 1ul << (channel_index);

	reg_val = p_register->idma_int;
	reg_val &= ~idma_mask;
	reg_val |= 1ul << (channel_index);

	writel(reg_val, &p_register->idma_int);
}

void nx_mpegtsi_set_cap_enable(u32 cap_idx, bool enable)
{
	const u32 capenb_pos = 0;
	const u32 capenb_mask = 1ul << capenb_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[cap_idx];
	reg_val &= ~capenb_mask;
	reg_val |= enable << capenb_pos;

	writel(reg_val, &p_register->cap_ctrl[cap_idx]);
}

bool nx_mpegtsi_get_cap_enable(u32 cap_idx)
{
	const u32 capenb_pos = 0;
	const u32 capenb_mask = 1ul << capenb_pos;

	return (bool)((__g_p_register->cap_ctrl[cap_idx]
				& capenb_mask) >> capenb_pos);
}

void nx_mpegtsi_set_bypass_enable(u32 cap_idx, bool enable)
{
	const u32 bypassenb_pos = 31;
	const u32 bypassenb_mask = 1ul << bypassenb_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;


	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[cap_idx];
	reg_val &= ~bypassenb_mask;
	reg_val |= enable << bypassenb_pos;

	writel(reg_val, &p_register->cap_ctrl[cap_idx]);
}

bool nx_mpegtsi_get_bypass_enable(u32 cap_idx)
{
	const u32 bypassenb_pos = 31;
	const u32 bypassenb_mask = 1ul << bypassenb_pos;

	return (bool)((__g_p_register->cap_ctrl[cap_idx]
			& bypassenb_mask) >> bypassenb_pos);
}

void nx_mpegtsi_set_serial_enable(u32 cap_idx, bool enable)
{
	const u32 serial_pos = 1;
	const u32 serial_mask = 1ul << serial_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[cap_idx];
	reg_val &= ~serial_mask;
	reg_val |= enable << serial_pos;

	writel(reg_val, &p_register->cap_ctrl[cap_idx]);
}

bool nx_mpegtsi_get_serial_enable(u32 cap_idx)
{
	const u32 serial_pos = 1;
	const u32 serial_mask = 1ul << serial_pos;

	return (bool)((__g_p_register->cap_ctrl[cap_idx]
				& serial_mask) >> serial_pos);
}

void nx_mpegtsi_set_tclk_polarity_enable(u32 cap_idx, bool enable)
{
	const u32 clkpol_pos = 4;
	const u32 clkpol_mask = 1ul << clkpol_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[cap_idx];
	reg_val &= ~clkpol_mask;
	reg_val |= enable << clkpol_pos;

	writel(reg_val, &p_register->cap_ctrl[cap_idx]);
}

bool nx_mpegtsi_get_tclk_polarity_enable(u32 cap_idx)
{
	const u32 clkpol_pos = 4;
	const u32 clkpol_mask = 1ul << clkpol_pos;

	return (bool)((__g_p_register->cap_ctrl[cap_idx]
					& clkpol_mask) >> clkpol_pos);
}

void nx_mpegtsi_set_tdp_polarity_enable(u32 cap_idx, bool enable)
{
	const u32 tdppol_pos = 5;
	const u32 tdppol_mask = 1ul << tdppol_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[cap_idx];
	reg_val &= ~tdppol_mask;
	reg_val |= enable << tdppol_pos;

	writel(reg_val, &p_register->cap_ctrl[cap_idx]);
}

bool nx_mpegtsi_get_tdp_polarity_enable(u32 cap_idx)
{
	const u32 tdppol_pos = 5;
	const u32 tdppol_mask = 1ul << tdppol_pos;

	return (bool)((__g_p_register->cap_ctrl[cap_idx]
				& tdppol_mask) >> tdppol_pos);
}

void nx_mpegtsi_set_tsync_polarity_enable(u32 cap_idx, bool enable)
{
	const u32 synpol_pos = 6;
	const u32 synpol_mask = 1ul << synpol_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[cap_idx];
	reg_val &= ~synpol_mask;
	reg_val |= enable << synpol_pos;

	writel(reg_val, &p_register->cap_ctrl[cap_idx]);
}

bool nx_mpegtsi_get_tsync_polarity_enable(u32 cap_idx)
{
	const u32 synpol_pos = 6;
	const u32 synpol_mask = 1ul << synpol_pos;

	return (bool)((__g_p_register->cap_ctrl[cap_idx]
					& synpol_mask) >> synpol_pos);
}

void nx_mpegtsi_set_terr_polarity_enable(u32 cap_idx, bool enable)
{
	const u32 errpol_pos = 7;
	const u32 errpol_mask = 1ul << errpol_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[cap_idx];
	reg_val &= ~errpol_mask;
	reg_val |= enable << errpol_pos;

	writel(reg_val, &p_register->cap_ctrl[cap_idx]);
}

bool nx_mpegtsi_get_terr_polarity_enable(u32 cap_idx)
{
	const u32 errpol_pos = 7;
	const u32 errpol_mask = 1ul << errpol_pos;


	return (bool)((__g_p_register->cap_ctrl[cap_idx]
				& errpol_mask) >> errpol_pos);
}

void nx_mpegtsi_set_cap_sram_wakeup(u32 cap_idx, bool wake_up)
{
	const u32 capslp_pos = 8;
	const u32 capslp_mask = 1ul << capslp_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;


	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[cap_idx];
	reg_val &= ~capslp_mask;
	reg_val |= wake_up << capslp_pos;

	writel(reg_val, &p_register->cap_ctrl[cap_idx]);
}

bool nx_mpegtsi_get_cap_sram_wakeup(u32 cap_idx)
{
	const u32 capslp_pos = 8;
	const u32 capslp_mask = 1ul << capslp_pos;

	return (bool)(!((__g_p_register->cap_ctrl[cap_idx]
					& capslp_mask) >> capslp_pos));
}

void nx_mpegtsi_set_cap_sram_power_enable(u32 cap_idx, bool enable)
{
	const u32 cappow_pos = 9;
	const u32 cappow_mask = 1ul << cappow_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[cap_idx];
	reg_val &= ~cappow_mask;
	reg_val |= enable << cappow_pos;

	writel(reg_val, &p_register->cap_ctrl[cap_idx]);
}

bool nx_mpegtsi_get_cap_sram_power_enable(u32 cap_idx)
{
	const u32 cappow_pos = 9;
	const u32 cappow_mask = 1ul << cappow_pos;

	return (bool)((__g_p_register->cap_ctrl[cap_idx]
				& cappow_mask) >> cappow_pos);
}

void nx_mpegtsi_set_cap1_output_enable(bool enable)
{
	const u32 outenb_pos = 16;
	const u32 outenb_mask = 1ul << outenb_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[1];
	reg_val &= ~outenb_mask;
	reg_val |= enable << outenb_pos;

	writel(reg_val, &p_register->cap_ctrl[1]);
}

bool nx_mpegtsi_get_cap1_output_enable(void)
{
	const u32 outenb_pos = 16;
	const u32 outenb_mask = 1ul << outenb_pos;

	return (bool)((__g_p_register->cap_ctrl[1]
				& outenb_mask) >> outenb_pos);
}

void nx_mpegtsi_set_cap1_out_tclk_polarity_enable(bool enable)
{
	const u32 outpol_pos = 17;
	const u32 outpol_mask = 1ul << outpol_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[1];
	reg_val &= ~outpol_mask;
	reg_val |= enable << outpol_pos;

	writel(reg_val, &p_register->cap_ctrl[1]);
}

bool nx_mpegtsi_get_cap1_out_tclk_polarity_enable(void)
{
	const u32 outpol_pos = 17;
	const u32 outpol_mask = 1ul << outpol_pos;

	return (bool)((__g_p_register->cap_ctrl[1]
				& outpol_mask) >> outpol_pos);
}

bool nx_mpegtsi_get_cap1_out_polarity_enable(void)
{
	const u32 outpol_pos = 17;
	const u32 outpol_mask = 1ul << outpol_pos;

	return (bool)((__g_p_register->cap_ctrl[1]
				& outpol_mask) >> outpol_pos);
}

void nx_mpegtsi_set_cap_int_lock_enable(u32 cap_idx, bool enable)
{
	const u32 intlock_pos = 24;
	const u32 intlock_mask = 1ul << intlock_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[cap_idx];
	reg_val &= ~intlock_mask;
	reg_val |= enable << intlock_pos;

	writel(reg_val, &p_register->cap_ctrl[cap_idx]);
}

bool nx_mpegtsi_get_cap_int_lock_enable(u32 cap_idx)
{
	const u32 intlock_pos = 24;
	const u32 intlock_mask = 1ul << intlock_pos;

	return (bool)((__g_p_register->cap_ctrl[cap_idx]
		      & intlock_mask) >> intlock_pos);
}

void nx_mpegtsi_set_cap_int_enable(u32 cap_idx, bool enable)
{
	const u32 capint_pos = 25;
	const u32 capint_mask = 1ul << capint_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[cap_idx];
	reg_val &= ~capint_mask;
	reg_val |= enable << capint_pos;

	writel(reg_val, &p_register->cap_ctrl[cap_idx]);
}

bool nx_mpegtsi_get_cap_int_enable(u32 cap_idx)
{
	const u32 capint_pos = 25;
	const u32 capint_mask = 1ul << capint_pos;

	return (bool)((__g_p_register->cap_ctrl[cap_idx]
					& capint_mask) >> capint_pos);
}

void nx_mpegtsi_set_cap_int_mask_clear(u32 cap_idx, bool unmask)
{
	const u32 capmask_pos = 26;
	const u32 capmask_mask = 1ul << capmask_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[cap_idx];
	reg_val &= ~capmask_mask;
	reg_val |= unmask << capmask_pos;

	writel(reg_val, &p_register->cap_ctrl[cap_idx]);
}

bool nx_mpegtsi_get_cap_int_mask_clear(u32 cap_idx)
{
	const u32 capmask_pos = 26;
	const u32 capmask_mask = 1ul << capmask_pos;

	return (bool)((__g_p_register->cap_ctrl[cap_idx]
				& capmask_mask) >> capmask_pos);
}

void nx_mpegtsi_set_cap_int_clear(u32 cap_idx)
{
	const u32 capmask_pos = 27;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->cap_ctrl[cap_idx];
	reg_val |= 1 << capmask_pos;

	writel(reg_val, &p_register->cap_ctrl[cap_idx]);
}

bool nx_mpegtsi_get_cap_int_status(u32 cap_idx)
{
	const u32 capmask_pos = 27;
	const u32 capmask_mask = 1ul << capmask_pos;

	return (bool)((__g_p_register->cap_ctrl[cap_idx]
				& capmask_mask) >> capmask_pos);
}

u32 nx_mpegtsi_get_cap_fifo_data(u32 cap_idx)
{
	return readl(&__g_p_register->cap_data[cap_idx]);
}

void nx_mpegtsi_set_cpuwr_data(u32 wr_data)
{
	writel(wr_data, &__g_p_register->cpu_wrdata);
}

u32 nx_mpegtsi_get_cpuwr_data(void)
{
	return readl(&__g_p_register->cpu_wrdata);
}

void nx_mpegtsi_set_cpuwr_addr(u32 wr_addr)
{
	writel(wr_addr, &__g_p_register->cpu_wraddr);
}

void nx_mpegtsi_set_tsi_enable(bool enable)
{
	const u32 tsirun_pos = 0;
	const u32 tsirun_mask = 1ul << tsirun_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->ctrl0;
	reg_val &= ~tsirun_mask;
	reg_val |= enable << tsirun_pos;

	writel(reg_val, &p_register->ctrl0);
}

bool nx_mpegtsi_get_tsi_enable(void)
{
	const u32 tsirun_pos = 0;
	const u32 tsirun_mask = 1ul << tsirun_pos;

	return (bool)((__g_p_register->ctrl0 & tsirun_mask)
				 >> tsirun_pos);
}

void nx_mpegtsi_set_tsi_encrypt(bool enable)
{
	const u32 tsienc_pos = 1;
	const u32 tsienc_mask = 1ul << tsienc_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->ctrl0;
	reg_val &= ~tsienc_mask;
	reg_val |= enable << tsienc_pos;

	writel(reg_val, &p_register->ctrl0);
}

bool nx_mpegtsi_get_tsi_encrypt(void)
{
	const u32 tsienc_pos = 1;
	const u32 tsienc_mask = 1ul << tsienc_pos;

	return (bool)((__g_p_register->ctrl0 & tsienc_mask)
				 >> tsienc_pos);
}

void nx_mpegtsi_set_tsi_sram_wakeup(bool wake_up)
{
	const u32 tsislp_pos = 6;
	const u32 tsislp_mask = 1ul << tsislp_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->ctrl0;
	reg_val &= ~tsislp_mask;
	reg_val |= wake_up << tsislp_pos;

	writel(reg_val, &p_register->ctrl0);
}

bool nx_mpegtsi_get_tsi_sram_wakeup(void)
{
	const u32 tsislp_pos = 6;
	const u32 tsislp_mask = 1ul << tsislp_pos;

	return (bool)(!((__g_p_register->ctrl0 & tsislp_mask)
				>> tsislp_pos));
}

void nx_mpegtsi_set_tsi_sram_power_enable(bool enable)
{
	const u32 tsipow_pos = 7;
	const u32 tsipow_mask = 1ul << tsipow_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->ctrl0;
	reg_val &= ~tsipow_mask;
	reg_val |= enable << tsipow_pos;

	writel(reg_val, &p_register->ctrl0);
}

bool nx_mpegtsi_get_tsi_sram_power_enable(void)
{
	const u32 tsipow_pos = 7;
	const u32 tsipow_mask = 1ul << tsipow_pos;

	return (bool)((__g_p_register->ctrl0 & tsipow_mask)
				>> tsipow_pos);
}

void nx_mpegtsi_set_tsi_int_enable(bool enable)
{
	const u32 tsiint_pos = 16;
	const u32 tsiint_mask = 1ul << tsiint_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->ctrl0;
	reg_val &= ~tsiint_mask;
	reg_val |= enable << tsiint_pos;

	writel(reg_val, &p_register->ctrl0);
}

bool nx_mpegtsi_get_tsi_int_enable(void)
{
	const u32 tsiint_pos = 16;
	const u32 tsiint_mask = 1ul << tsiint_pos;

	return (bool)((__g_p_register->ctrl0 & tsiint_mask)
				>> tsiint_pos);
}

void nx_mpegtsi_set_tsi_int_mask_clear(bool enable)
{
	const u32 tsimask_pos = 17;
	const u32 tsimask_mask = 1ul << tsimask_pos;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;

	p_register = __g_p_register;

	reg_val = p_register->ctrl0;
	reg_val &= ~tsimask_mask;
	reg_val |= enable << tsimask_pos;

	writel(reg_val, &p_register->ctrl0);
}

bool nx_mpegtsi_get_tsi_int_mask_clear(void)
{
	const u32 tsimask_pos = 17;
	const u32 tsimask_mask = 1ul << tsimask_pos;

	return (bool)((__g_p_register->ctrl0 & tsimask_mask)
				>> tsimask_pos);
}

void nx_mpegtsi_set_tsi_int_clear(void)
{
	const u32 tsimask_pos = 18;

	register struct nx_mpegtsi_register_set *p_register;
	register u32 reg_val;


	p_register = __g_p_register;

	reg_val = p_register->ctrl0;
	reg_val |= 1 << tsimask_pos;

	writel(reg_val, &p_register->ctrl0);
}

bool nx_mpegtsi_get_tsi_int_status(void)
{
	const u32 tsimask_pos = 18;
	const u32 tsimask_mask = 1ul << tsimask_pos;

	return (bool)((__g_p_register->ctrl0 & tsimask_mask)
				>> tsimask_pos);
}

u32 nx_mpegtsi_get_cap_data(u32 cap_idx)
{
	return (bool)(__g_p_register->cap_data[cap_idx]);
}

u32 nx_mpegtsi_get_tsi_out_data(void)
{
	return (bool)(__g_p_register->tsp_outdata);
}

u32 nx_mpegtsi_byte_swap(u32 data)
{
	u32 swap_data = 0;

	swap_data = (data & 0x000000ff) << 24
		| (data & 0x0000ff00) << 8
		| (data & 0x00ff0000) >> 8
		| (data & 0xff000000) >> 24;

	return swap_data;
}

void nx_mpegtsi_write_pid(u32 bank, u32 type, u32 pid_addr, u32 pid_data)
{
	u32 cpu_wr_addr = ((bank << 9) | (type << 7) | pid_addr);

	nx_mpegtsi_set_cpuwr_addr(cpu_wr_addr);
	nx_mpegtsi_set_cpuwr_data(pid_data);
}

void nx_mpegtsi_write_aeskeyiv(u32 cw_addr, u32 cw0, u32 cw1, u32 cw2, u32 cw3,
			u32 iv0, u32 iv1, u32 iv2, u32 iv3)
{
	nx_mpegtsi_write_pid(2, 1, (cw_addr * 8 + 0),
			    nx_mpegtsi_byte_swap(cw0));
	nx_mpegtsi_write_pid(2, 1, (cw_addr * 8 + 1),
			    nx_mpegtsi_byte_swap(cw1));
	nx_mpegtsi_write_pid(2, 1, (cw_addr * 8 + 2),
			    nx_mpegtsi_byte_swap(cw2));
	nx_mpegtsi_write_pid(2, 1, (cw_addr * 8 + 3),
			    nx_mpegtsi_byte_swap(cw3));
	nx_mpegtsi_write_pid(2, 1, (cw_addr * 8 + 4),
			    nx_mpegtsi_byte_swap(iv0));
	nx_mpegtsi_write_pid(2, 1, (cw_addr * 8 + 5),
			    nx_mpegtsi_byte_swap(iv1));
	nx_mpegtsi_write_pid(2, 1, (cw_addr * 8 + 6),
			    nx_mpegtsi_byte_swap(iv2));
	nx_mpegtsi_write_pid(2, 1, (cw_addr * 8 + 7),
			    nx_mpegtsi_byte_swap(iv3));
}

void nx_mpegtsi_write_cascw(u32 cw_addr, u32 cw0, u32 cw1, u32 cw2, u32 cw3)
{
	if (cw_addr & 0x1) {
		nx_mpegtsi_write_pid(2, 1, ((cw_addr>>1)*8 + 0),
					nx_mpegtsi_byte_swap(cw0));
		nx_mpegtsi_write_pid(2, 1, ((cw_addr>>1)*8 + 1),
					nx_mpegtsi_byte_swap(cw1));
		nx_mpegtsi_write_pid(2, 1, ((cw_addr>>1)*8 + 2),
					nx_mpegtsi_byte_swap(cw2));
		nx_mpegtsi_write_pid(2, 1, ((cw_addr>>1)*8 + 3),
					nx_mpegtsi_byte_swap(cw3));
	} else {
		nx_mpegtsi_write_pid(2, 1, ((cw_addr>>1)*8 + 4),
					nx_mpegtsi_byte_swap(cw0));
		nx_mpegtsi_write_pid(2, 1, ((cw_addr>>1)*8 + 5),
					nx_mpegtsi_byte_swap(cw1));
		nx_mpegtsi_write_pid(2, 1, ((cw_addr>>1)*8 + 6),
					nx_mpegtsi_byte_swap(cw2));
		nx_mpegtsi_write_pid(2, 1, ((cw_addr>>1)*8 + 7),
					nx_mpegtsi_byte_swap(cw3));
	}
}
