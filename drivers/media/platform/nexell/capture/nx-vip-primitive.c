/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
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

#include "nx-vip-primitive.h"

static struct nx_vip_register_set *__g_p_register[NUMBER_OF_VIP_MODULE];

bool nx_vip_initialize(void)
{
	static int b_init;

	if (false == b_init)
		b_init = true;
	return true;
}

u32 nx_vip_get_number_of_module(void)
{
	return NUMBER_OF_VIP_MODULE;
}

u32 nx_vip_get_size_of_register_set(void)
{
	return sizeof(struct nx_vip_register_set);
}

void nx_vip_set_base_address(u32 module_index, void *base_address)
{
	__g_p_register[module_index] =
		(struct nx_vip_register_set *)base_address;
}

void *nx_vip_get_base_address(u32 module_index)
{
	return (void *)__g_p_register[module_index];
}

u32 nx_vip_get_physical_address(u32 module_index)
{
	const u32 physical_addr[NUMBER_OF_VIP_MODULE] = {
		0xc0063000,
		0xc0064000,
		0xc0099000
	};
	return physical_addr[module_index];
}

u32 nx_vip_get_clock_number(u32 module_index)
{
	const u32 clock_number[] = {30, 31, 40};
	return clock_number[module_index];
}

u32 nx_vip_get_reset_number(u32 module_index)
{
	const u32 reset_number[] = {68, 67, 69};
	return reset_number[module_index];
}

u32 nx_vip_get_interrupt_number(u32 module_index)
{
	const u32 interrupt_number[] = {37, 38, 72};
	return interrupt_number[module_index];
}

void nx_vip_set_interrupt_enable(u32 module_index, u32 int_num, int enable)
{
	register u16 regvalue;
	register struct nx_vip_register_set *p_register;
	const u16 odintenb_bitpos = 8;

	p_register = __g_p_register[module_index];
	if (2 > int_num) {
		regvalue = p_register->vip_hvint & ~(1 << (int_num));
		if (enable)
			regvalue |= (1 << (int_num));
		else
			regvalue &= ~(1 << (int_num));
		writel(regvalue, &p_register->vip_hvint);
	} else {
		if (enable)
			writel((1 << odintenb_bitpos), &p_register->vip_odint);
		else
			writel(0, &p_register->vip_odint);
	}
}

bool nx_vip_get_interrupt_enable(u32 module_index, u32 int_num)
{
	const u16 odintenb_bitpos = 8;

	const u16 odintenb_mask = 1 << odintenb_bitpos;

	if (2 > int_num)
		return (__g_p_register[module_index]->vip_hvint &
			(1 << (int_num)))
			? true
			: false;

	return (__g_p_register[module_index]->vip_odint & odintenb_mask)
		? true
		: false;
}

bool nx_vip_get_interrupt_pending(u32 module_index, u32 int_num)
{
	const u16 odintpend_bitpos = 0;

	const u16 odintpend_mask = 1 << odintpend_bitpos;

	if (2 > int_num)
		return (__g_p_register[module_index]->vip_hvint &
			(1 << int_num))
			? true
			: false;

	return (__g_p_register[module_index]->vip_odint & odintpend_mask)
		? true
		: false;
}

void nx_vip_clear_interrupt_pending(u32 module_index, u32 int_num)
{
	register struct nx_vip_register_set *p_register;
	register u16 regvalue;
	const u32 vipintenb_mask = 0x03 << 8;

	p_register = __g_p_register[module_index];
	if (2 > int_num) {
		regvalue = p_register->vip_hvint & vipintenb_mask;
		regvalue |= (0x01 << int_num);
		writel(regvalue, &p_register->vip_hvint);
	} else {
		regvalue = p_register->vip_odint;
		regvalue |= 1;
		writel(regvalue, &p_register->vip_odint);
	}
}

void nx_vip_set_interrupt_enable_all(u32 module_index, int enable)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	if (enable) {
		writel((u16)(0x03 << 8), &p_register->vip_hvint);
		writel((u16)(0x01 << 8), &p_register->vip_odint);
	} else {
		writel((u16)0x00, &p_register->vip_hvint);
		writel((u16)0x00, &p_register->vip_odint);
	}
}

bool nx_vip_get_interrupt_enable_all(u32 module_index)
{
	register struct nx_vip_register_set *p_register;
	const u16 vhsintenb_mask = 0x03 << 8;

	const u16 odintenb_mask = 0x01 << 8;

	p_register = __g_p_register[module_index];
	if ((p_register->vip_hvint & vhsintenb_mask) ||
	    (p_register->vip_odint & odintenb_mask))
		return true;
	return false;
}

bool nx_vip_get_interrupt_pending_all(u32 module_index)
{
	register struct nx_vip_register_set *p_register;
	const u16 vhsintpend_mask = 0x03;

	const u16 odintpend_mask = 0x01;

	p_register = __g_p_register[module_index];
	if ((p_register->vip_hvint & vhsintpend_mask) ||
	    (p_register->vip_odint & odintpend_mask))
		return true;
	return false;
}

void nx_vip_clear_interrupt_pending_all(u32 module_index)
{
	register struct nx_vip_register_set *p_register;
	register u16 regvalue;
	const u16 vhsintpend_mask = 0x03;

	const u16 odintpend_mask = 0x01;

	p_register = __g_p_register[module_index];
	regvalue = p_register->vip_hvint;
	regvalue |= vhsintpend_mask;
	writel(regvalue, &p_register->vip_hvint);
	regvalue = p_register->vip_odint;
	regvalue |= odintpend_mask;
	writel(regvalue, &p_register->vip_odint);
}

int32_t nx_vip_get_interrupt_pending_number(u32 module_index)
{
	register struct nx_vip_register_set *p_register;
	register u16 pend;
	register u16 pend0;
	register u16 pend1;
	const int16_t vip_pend[8] = {-1, 0, 1, 0, 2, 0, 1, 0};

	p_register = __g_p_register[module_index];
	pend0 = p_register->vip_hvint;
	pend0 = pend0 & (pend0 >> 8);
	pend1 = p_register->vip_odint;
	pend1 = pend1 & (pend1 >> 8);
	pend = (pend1 << 2 | pend0);
	return vip_pend[pend];
}

u32 nx_vip_get_number_of_padmode(u32 module_index)
{
	const u32 number_of_pad_mode[] = {1, 2};

	return number_of_pad_mode[module_index];
}

void nx_vip_set_vipenable(u32 module_index, int b_vipenb, int b_sep_enb,
			  int b_clip_enb, int b_deci_enb)
{
	register u16 temp;
	register struct nx_vip_register_set *p_register;
	const u16 vipenb = 1u << 0;

	const u16 sepenb = 1u << 8;

	const u16 clipenb = 1u << 1;

	const u16 decienb = 1u << 0;

	p_register = __g_p_register[module_index];
	temp = p_register->vip_config;
	if (b_vipenb)
		temp |= (u16)vipenb;
	else
		temp &= (u16)~vipenb;
	writel(temp, &p_register->vip_config);
	temp = 0;
	if (b_sep_enb)
		temp |= (u16)sepenb;
	if (b_clip_enb)
		temp |= (u16)clipenb;
	if (b_deci_enb)
		temp |= (u16)decienb;
	writel(temp, &p_register->vip_cdenb);
}

void nx_vip_get_vipenable(u32 module_index, int *p_vipenb, int *p_sep_enb,
			  int *p_clip_enb, int *p_deci_enb)
{
	register struct nx_vip_register_set *p_register;
	const u16 vipenb = 1u << 0;

	const u16 sepenb = 1u << 8;

	const u16 clipenb = 1u << 1;

	const u16 decienb = 1u << 0;

	p_register = __g_p_register[module_index];
	if (NULL != p_vipenb)
		*p_vipenb = (p_register->vip_config & vipenb) ? true : false;
	if (NULL != p_sep_enb)
		*p_sep_enb = (p_register->vip_cdenb & sepenb) ? true : false;
	if (NULL != p_clip_enb)
		*p_clip_enb = (p_register->vip_cdenb & clipenb) ? true : false;
	if (NULL != p_deci_enb)
		*p_deci_enb = (p_register->vip_cdenb & decienb) ? true : false;
}

void nx_vip_set_input_port(u32 module_index, u32 input_port)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel((u32)input_port, &p_register->vip_vip1);
}

u32 nx_vip_get_input_port(u32 module_index)
{
	const u16 sipenb_mask = 1 << 0;

	return (__g_p_register[module_index]->vip_vip1 & sipenb_mask);
}

void nx_vip_set_data_mode(u32 module_index, u32 data_order, u32 data_width)
{
	const u32 dorder_pos = 2;

	const u32 dorder_mask = 3ul << dorder_pos;

	const u32 dwidth_pos = 1;

	const u32 dwidth_mask = 1ul << dwidth_pos;

	register u32 temp;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	temp = (u32)p_register->vip_config;
	temp &= ~(dorder_mask | dwidth_mask);
	temp |= ((u32)data_order << dorder_pos);
	temp |= ((8 == data_width) ? dwidth_mask : 0);
	writel((u16)temp, &p_register->vip_config);
}

void nx_vip_get_data_mode(u32 module_index, u32 *p_data_order,
			  u32 *p_data_width)
{
	const u32 dorder_pos = 2;

	const u32 dorder_mask = 3ul << dorder_pos;

	const u32 dwidth_pos = 1;

	const u32 dwidth_mask = 1ul << dwidth_pos;

	register u32 temp;

	temp = (u32)__g_p_register[module_index]->vip_config;
	if (NULL != p_data_order)
		*p_data_order = ((temp & dorder_mask) >> dorder_pos);
	if (NULL != p_data_width)
		*p_data_width = (temp & dwidth_mask) ? 8 : 16;
}

void nx_vip_set_sync(u32 module_index, int b_ext_sync,
		     u32 source_bits, u32 avw, u32 avh, u32 hsw,
		     u32 hfp, u32 hbp, u32 vsw, u32 vfp, u32 vbp)
{
	const u32 drange = 1ul << 9;

	const u32 extsyncenb = 1ul << 8;

	register u32 temp;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel(0xffffffff, &p_register->vip_vbegin);
	writel(0xffffffff, &p_register->vip_vend);
	writel(0xffffffff, &p_register->vip_hbegin);
	writel(0xffffffff, &p_register->vip_hend);
	temp = (u32)p_register->vip_config;
	temp &= ~drange;
	if (b_ext_sync)
		temp |= extsyncenb;
	else
		temp &= ~extsyncenb;
	writel((u16)temp, &p_register->vip_config);

	switch (source_bits) {
	case nx_vip_vd_8bit:

#ifdef CONFIG_ARCH_S5P6818
		writel((u16)(avw >> 1), &p_register->vip_imgwidth);
#else
		if (b_ext_sync)
			writel((u16)(avw >> 1), &p_register->vip_imgwidth);
		else
			writel((u16)(avw >> 1) + 2, &p_register->vip_imgwidth);
#endif
		writel((u16)avh, &p_register->vip_imgheight);
		break;
	case nx_vip_vd_16bit:
		writel((u16)avw, &p_register->vip_imgwidth);
		writel((u16)avh, &p_register->vip_imgheight);
		break;
	default:
		break;
	}

	if (b_ext_sync) {
		if (0 != vbp) {
			temp = (u32)p_register->vip_syncctrl;
			temp &= ~(3 << 11);
			writel((u16)temp, &p_register->vip_syncctrl);
			writel((u16)(vbp - 1), &p_register->vip_vbegin);
			writel((u16)(vbp + avh - 1), &p_register->vip_vend);
		} else {
			temp = (u32)p_register->vip_syncctrl;
			temp |= (3 << 11);
			writel((u16)temp, &p_register->vip_syncctrl);
			writel((u16)(vfp + 1), &p_register->vip_vbegin);
			writel((u16)(vfp + vsw + 1), &p_register->vip_vend);
		}
		if (0 != hbp) {
			temp = (u32)p_register->vip_syncctrl;
			temp &= ~(1 << 10);
			writel((u16)temp, &p_register->vip_syncctrl);
			writel((u16)(hbp - 1), &p_register->vip_hbegin);
			writel((u16)(hbp + avw - 1), &p_register->vip_hend);
		} else {
			temp = (u32)p_register->vip_syncctrl;
			temp |= (1 << 10);
			writel((u16)temp, &p_register->vip_syncctrl);
			writel((u16)(hfp), &p_register->vip_hbegin);
			writel((u16)(hfp + hsw), &p_register->vip_hend);
		}
	} else {
		writel((u16)(vfp + 1), &p_register->vip_vbegin);
		writel((u16)(vfp + vsw + 1), &p_register->vip_vend);
		writel((u16)(hfp - 7), &p_register->vip_hbegin);
		writel((u16)(hfp + hsw - 7), &p_register->vip_hend);
	}
}

void nx_vip_set_hvsync(u32 module_index, int b_ext_sync, u32 avw, u32 avh,
		       u32 hsw, u32 hfp, u32 hbp, u32 vsw, u32 vfp, u32 vbp)
{
	nx_vip_set_sync(module_index, b_ext_sync, nx_vip_vd_8bit, avw, avh, hsw,
			hfp, hbp, vsw, vfp, vbp);
}

void nx_vip_set_hvsync_for_mipi(u32 module_index, u32 avw, u32 avh, u32 hsw,
				u32 hfp, u32 hbp, u32 vsw, u32 vfp, u32 vbp)
{
#ifdef CONFIG_ARCH_S5P6818
	int b_ext_sync = true;
	int b_ext_dvalid = true;
	int bypass_ext_dvalid = true;
	int bypass_ext_sync = false;

	nx_vip_set_sync(module_index, b_ext_sync, nx_vip_vd_16bit, avw >> 1,
			avh, hsw, hfp, 0, vsw, vfp, 0);
	nx_vip_set_dvalid_mode(module_index, b_ext_dvalid, bypass_ext_dvalid,
			       bypass_ext_sync);
#else
	const u32 drange = 1ul << 9;
	const u32 extsyncenb = 1ul << 8;
	register u32 temp;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];

	temp = p_register->vip_config;
	temp &= ~drange;
	temp &= ~extsyncenb;
	writel(temp, &p_register->vip_config);

	temp = p_register->vip_syncctrl;
	temp |= 0x300;
	writel(temp, &p_register->vip_syncctrl);

	writel(avw >> 1, &p_register->vip_imgwidth);
	writel(avh, &p_register->vip_imgheight);
	writel(vfp + 1, &p_register->vip_vbegin);
	writel(vfp + vsw + 1, &p_register->vip_vend);
	writel(hfp - 7, &p_register->vip_hbegin);
	writel(hfp + hsw - 7, &p_register->vip_hend);
#endif
}

void nx_vip_get_hvsync(u32 module_index, int *p_ext_sync, u32 *pavw, u32 *pavh,
		       u32 *phbegin, u32 *phend, u32 *pvbegin, u32 *pvend)
{
	const u16 extsyncenb = 1u << 8;
	int b_ext_sync;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	b_ext_sync = (p_register->vip_config & extsyncenb) ? true : false;
	if (NULL != p_ext_sync)
		*p_ext_sync = b_ext_sync;
	if (NULL != pavw)
		*pavw = ((u32)(p_register->vip_imgwidth) << 1) -
			((b_ext_sync) ? 0 : 4);
	if (NULL != pavh)
		*pavh = (u32)p_register->vip_imgheight;
	if (NULL != pvbegin)
		*pvbegin = (u32)p_register->vip_vbegin;
	if (NULL != pvend)
		*pvend = (u32)p_register->vip_vend;
	if (NULL != phbegin)
		*phbegin = (u32)p_register->vip_hbegin;
	if (NULL != phend)
		*phend = (u32)p_register->vip_hend;
}

void nx_vip_set_dvalid_mode(u32 module_index, int b_ext_dvalid,
			    int b_dvalid_pol, int b_sync_pol)
{
	const u32 dvalidpol = 1ul << 4;
	const u32 extdvenb = 1ul << 2;
	register u32 temp;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	temp = (u32)p_register->vip_syncctrl;
	if (b_ext_dvalid)
		temp |= extdvenb;
	else
		temp &= ~extdvenb;
	if (b_dvalid_pol)
		temp |= dvalidpol;
	else
		temp &= ~dvalidpol;
	temp &= ~((1 << 8) | (1 << 9));
	temp |= (((b_sync_pol & 0x1) << 8) | ((b_sync_pol & 0x1) << 9));
	writel((u16)temp, &p_register->vip_syncctrl);
}

void nx_vip_get_dvalid_mode(u32 module_index, int *p_ext_dvalid,
			    int *p_dvalid_pol)
{
	const u32 dvalidpol = 1ul << 4;
	const u32 extdvenb = 1ul << 2;
	register u32 temp;

	temp = (u32)__g_p_register[module_index]->vip_syncctrl;
	if (NULL != p_ext_dvalid)
		*p_ext_dvalid = (temp & extdvenb) ? true : false;
	if (NULL != p_dvalid_pol)
		*p_dvalid_pol = (temp & dvalidpol) ? true : false;
}

void nx_vip_set_field_mode(u32 module_index, int b_ext_field,
			   u32 field_sel, int b_interlace,
			   int b_inv_field)
{
	const u32 extfieldenb = 1ul << 3;
	const u32 nx_vip_fieldsel_mask = 3ul << 0;
	const u32 interlaceenb = 1ul << 1;
	const u32 fieldinv = 1ul << 0;
	register u32 temp;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	temp = (u32)p_register->vip_syncctrl;
	if (b_ext_field)
		temp |= extfieldenb;
	else
		temp &= ~extfieldenb;
	temp &= ~nx_vip_fieldsel_mask;
	temp |= (u32)(field_sel);
	writel((u16)temp, &p_register->vip_syncctrl);
	writel((u16)(((b_interlace) ? interlaceenb : 0) |
		     ((b_inv_field) ? fieldinv : 0)),
	       &p_register->vip_scanmode);
}

void nx_vip_get_field_mode(u32 module_index, int *p_ext_field,
			   u32 *p_field_sel, int *p_interlace,
			   int *p_inv_field)
{
	const u32 extfieldenb = 1ul << 3;
	const u32 nx_vip_fieldsel_mask = 3ul << 0;
	const u32 interlaceenb = 1ul << 1;
	const u32 fieldinv = 1ul << 0;
	register u32 temp;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	temp = (u32)p_register->vip_syncctrl;

	if (NULL != p_ext_field)
		*p_ext_field = (temp & extfieldenb) ? true : false;
	if (NULL != p_field_sel)
		*p_field_sel = temp & nx_vip_fieldsel_mask;

	temp = (u32)p_register->vip_scanmode;

	if (NULL != p_interlace)
		*p_interlace = (temp & interlaceenb) ? true : false;
	if (NULL != p_inv_field)
		*p_inv_field = (temp & fieldinv) ? true : false;
}

bool nx_vip_get_field_status(u32 module_index)
{
	const u16 lastfield = 1u << 5;

	return (__g_p_register[module_index]->vip_syncctrl & lastfield) ? true
		: false;
}

bool nx_vip_get_hsync_status(u32 module_index)
{
	const u16 curhsync = 1u << 1;

	return (__g_p_register[module_index]->vip_syncmon & curhsync) ? true
		: false;
}

bool nx_vip_get_vsync_status(u32 module_index)
{
	const u16 curvsync = 1u << 0;

	return (__g_p_register[module_index]->vip_syncmon & curvsync) ? true
		: false;
}

void nx_vip_set_fiforeset_mode(u32 module_index, u32 fiforeset)
{
	const u32 resetfifosel_pos = 1ul;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel((u16)(fiforeset << resetfifosel_pos), &p_register->vip_fifoctrl);
}

u32 nx_vip_get_fiforeset_mode(u32 module_index)
{
	const u16 resetfifosel_pos = 1;
	const u16 resetfifosel_mask = 3u << resetfifosel_pos;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	return ((p_register->vip_fifoctrl & resetfifosel_mask)
				  >> resetfifosel_pos);
}

u32 nx_vip_get_fifostatus(u32 module_index)
{
	const u32 fifostatus_pos = 8ul;

	return (u32)(__g_p_register[module_index]->vip_fifoctrl >>
		     fifostatus_pos);
}

void nx_vip_reset_fifo(u32 module_index)
{
	const u16 resetfifo = 1u << 0;
	register u16 temp;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	temp = p_register->vip_fifoctrl;
	writel((u16)(temp | resetfifo), &p_register->vip_fifoctrl);
	writel((u16)temp, &p_register->vip_fifoctrl);
}

u32 nx_vip_get_hor_count(u32 module_index)
{
	return (u32)__g_p_register[module_index]->vip_hcount;
}

u32 nx_vip_get_ver_count(u32 module_index)
{
	return (u32)__g_p_register[module_index]->vip_vcount;
}

void nx_vip_set_clip_region(u32 module_index, u32 left, u32 top, u32 right,
			    u32 bottom)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel((u16)left, &p_register->clip_left);
	writel((u16)right, &p_register->clip_right);
	writel((u16)top, &p_register->clip_top);
	writel((u16)bottom, &p_register->clip_bottom);
}

void nx_vip_get_clip_region(u32 module_index, u32 *p_left, u32 *p_top,
			    u32 *p_right, u32 *p_bottom)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	if (NULL != p_left)
		*p_left = (u32)p_register->clip_left;
	if (NULL != p_top)
		*p_top = (u32)p_register->clip_top;
	if (NULL != p_right)
		*p_right = (u32)p_register->clip_right;
	if (NULL != p_bottom)
		*p_bottom = (u32)p_register->clip_bottom;
}

static u32 deci_src_width[2];
static u32 deci_src_height[2];
void nx_vip_set_decimation(u32 module_index, u32 src_width, u32 src_height,
			   u32 dst_width, u32 dst_height)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel((u16)dst_width, &p_register->deci_targetw);
	writel((u16)dst_height, &p_register->deci_targeth);
	writel((u16)(src_width - dst_width), &p_register->deci_deltaw);
	writel((u16)(src_height - dst_height), &p_register->deci_deltah);
	writel((int16_t)((dst_width << 1) - src_width),
	       (u16 *)&p_register->deci_clearw);
	writel((int16_t)((dst_height << 1) - src_height),
	       (u16 *)&p_register->deci_clearh);
	deci_src_width[module_index] = src_width;
	deci_src_height[module_index] = src_height;
}

void nx_vip_get_decimation(u32 module_index, u32 *p_dst_width,
			   u32 *p_dst_height, u32 *p_delta_width,
			   u32 *p_delta_height, int32_t *p_clear_width,
			   int32_t *p_clear_height)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	if (NULL != p_dst_width)
		*p_dst_width = (u32)p_register->deci_targetw;
	if (NULL != p_dst_height)
		*p_dst_height = (u32)p_register->deci_targeth;
	if (NULL != p_delta_width)
		*p_delta_width = (u32)p_register->deci_deltaw;
	if (NULL != p_delta_height)
		*p_delta_height = (u32)p_register->deci_deltah;
	if (NULL != p_clear_width)
		*p_clear_width = (int32_t)((u32)p_register->deci_clearw |
					   ((p_register->deci_clearw &
					     (1 << 12)) ? 0xffffe000 : 0));
	if (NULL != p_clear_height)
		*p_clear_height = (int32_t)((u32)p_register->deci_clearh |
					    ((p_register->deci_clearh &
					      (1 << 11)) ? 0xfffff000 : 0));
}

void nx_vip_set_clipper_format(u32 module_index, u32 format)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
#ifdef CONFIG_ARCH_S5P4418
	if (format == nx_vip_format_yuyv)
		writel(1, &p_register->clip_yuyvenb);
	else
		writel(0, &p_register->clip_yuyvenb);
#endif
	writel((u16)format, &p_register->clip_format);

#ifdef CONFIG_ARCH_S5P4418
	writel(0, &p_register->clip_rotflip);
#endif
}

void nx_vip_get_clipper_format(u32 module_index, u32 *p_format)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	if (NULL != p_format)
		*p_format = p_register->clip_format;
}

void nx_vip_set_decimator_format(u32 module_index, u32 format)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel((u16)format, &p_register->deci_format);
}

void nx_vip_get_decimator_format(u32 module_index, u32 *p_format)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	if (NULL != p_format)
		*p_format = p_register->deci_format;
}

void nx_vip_set_clipper_addr(u32 module_index, u32 format, u32 width,
			     u32 height, u32 lu_addr, u32 cb_addr, u32 cr_addr,
			     u32 stride_y, u32 stride_cb_cr)
{
#ifdef CONFIG_ARCH_S5P6818
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel(lu_addr, &p_register->clip_luaddr);
	writel(stride_y, &p_register->clip_lustride);
	writel(cr_addr, &p_register->clip_craddr);
	writel(stride_cb_cr, &p_register->clip_crstride);
	writel(cb_addr, &p_register->clip_cbaddr);
	writel(stride_cb_cr, &p_register->clip_cbstride);
#else
	register struct nx_vip_register_set *p_register;
	register u32 segment, left, top;

	p_register = __g_p_register[module_index];
	segment = lu_addr >> 30;
	left = lu_addr & 0x00007fff;
	top = (lu_addr & 0x3fff8000) >> 15;

	writel(segment, &p_register->clip_luseg);
	writel(left, &p_register->clip_luleft);
	writel(left + width, &p_register->clip_luright);
	writel(top, &p_register->clip_lutop);
	writel(top + height, &p_register->clip_lubottom);

	if (format == nx_vip_format_420) {
		width >>= 1;
		height >>= 1;
	} else if (format == nx_vip_format_422) {
		width >>= 1;
	}

	segment = cb_addr >> 30;
	left = cb_addr & 0x00007fff;
	top = (cb_addr & 0x3fff8000) >> 15;

	writel(segment, &p_register->clip_cbseg);
	writel(left, &p_register->clip_cbleft);
	writel(left + width, &p_register->clip_cbright);
	writel(top, &p_register->clip_cbtop);
	writel(top + height, &p_register->clip_cbbottom);

	segment = cr_addr >> 30;
	left = cr_addr & 0x00007fff;
	top = (cr_addr & 0x3fff8000) >> 15;

	writel(segment, &p_register->clip_crseg);
	writel(left, &p_register->clip_crleft);
	writel(left + width, &p_register->clip_crright);
	writel(top, &p_register->clip_crtop);
	writel(top + height, &p_register->clip_crbottom);

	writel(stride_y, &p_register->clip_strideh);
	writel(stride_cb_cr, &p_register->clip_stridel);
#endif
}

void nx_vip_set_decimator_addr(u32 module_index, u32 format,
			       u32 width, u32 height, u32 lu_addr, u32 cb_addr,
			       u32 cr_addr, u32 stride_y, u32 stride_cb_cr)
{
#ifdef CONFIG_ARCH_S5P6818
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel(lu_addr, &p_register->deci_luaddr);
	writel(stride_y, &p_register->deci_lustride);
	if (format == nx_vip_format_420) {
		width >>= 1;
		height >>= 1;
	} else if (format == nx_vip_format_422) {
		width >>= 1;
	}
	writel(cr_addr, &p_register->deci_craddr);
	writel(stride_cb_cr, &p_register->deci_crstride);
	writel(cb_addr, &p_register->deci_cbaddr);
	writel(stride_cb_cr, &p_register->deci_cbstride);
#else
	register struct nx_vip_register_set *p_register;
	register u32 segment, left, top;

	p_register = __g_p_register[module_index];

	segment = lu_addr >> 30;
	left = lu_addr & 0x00007fff;
	top = (lu_addr & 0x3fff8000) >> 15;

	writel(segment, &p_register->deci_luseg);
	writel(left, &p_register->deci_luleft);
	writel(left + width, &p_register->deci_luright);
	writel(top, &p_register->deci_lutop);
	writel(top + height, &p_register->deci_lubottom);

	if (format == nx_vip_format_420) {
		width >>= 1;
		height >>= 1;
	} else if (format == nx_vip_format_422) {
		width >>= 1;
	}

	segment = cb_addr >> 30;
	left = cb_addr & 0x00007fff;
	top = (cb_addr & 0x3fff8000) >> 15;

	writel(segment, &p_register->deci_cbseg);
	writel(left, &p_register->deci_cbleft);
	writel(left + width, &p_register->deci_cbright);
	writel(top, &p_register->deci_cbtop);
	writel(top + height, &p_register->deci_cbbottom);

	segment = cr_addr >> 30;
	left = cr_addr & 0x00007fff;
	top = (cr_addr & 0x3fff8000) >> 15;

	writel(segment, &p_register->deci_crseg);
	writel(left, &p_register->deci_crleft);
	writel(left + width, &p_register->deci_crright);
	writel(top, &p_register->deci_crtop);
	writel(top + height, &p_register->deci_crbottom);

	writel(stride_y, &p_register->clip_strideh);
	writel(stride_cb_cr, &p_register->clip_stridel);
#endif
}

int nx_vip_smoke_test(u32 module_index)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	if (0x00000200 != p_register->vip_fifoctrl)
		return false;
	if (0x00000003 != p_register->vip_syncmon)
		return false;

	writel(0xc0debeef, &p_register->vip_imgwidth);

	if (0x00003eef != p_register->vip_imgwidth)
		return false;

	return true;
}

void nx_vip_get_deci_source(u32 module_index, u32 *p_src_width,
			    u32 *p_src_height)
{
	if (p_src_width)
		*p_src_width = deci_src_width[module_index];
	if (p_src_height)
		*p_src_height = deci_src_height[module_index];
}

void nx_vip_dump_register(u32 module)
{
	struct nx_vip_register_set *p_reg =
		(struct nx_vip_register_set *)nx_vip_get_base_address(module);

	pr_info("DUMP VIP %d Registers\n", module);
#ifdef CONFIG_ARCH_S5P6818
	pr_info(" VIP_CONFIG     = 0x%04x\r\n", p_reg->vip_config);
	pr_info(" VIP_HVINT      = 0x%04x\r\n", p_reg->vip_hvint);
	pr_info(" VIP_SYNCCTRL   = 0x%04x\r\n", p_reg->vip_syncctrl);
	pr_info(" VIP_SYNCMON    = 0x%04x\r\n", p_reg->vip_syncmon);
	pr_info(" VIP_VBEGIN     = 0x%04x\r\n", p_reg->vip_vbegin);
	pr_info(" VIP_VEND       = 0x%04x\r\n", p_reg->vip_vend);
	pr_info(" VIP_HBEGIN     = 0x%04x\r\n", p_reg->vip_hbegin);
	pr_info(" VIP_HEND       = 0x%04x\r\n", p_reg->vip_hend);
	pr_info(" VIP_FIFOCTRL   = 0x%04x\r\n", p_reg->vip_fifoctrl);
	pr_info(" VIP_HCOUNT     = 0x%04x\r\n", p_reg->vip_hcount);
	pr_info(" VIP_VCOUNT     = 0x%04x\r\n", p_reg->vip_vcount);
	pr_info(" VIP_PADCLK_SEL = 0x%04x\r\n", p_reg->vip_padclk_sel);
	pr_info(" VIP_INFIFOCLR  = 0x%04x\r\n", p_reg->vip_infifoclr);
	pr_info(" VIP_CDENB      = 0x%04x\r\n", p_reg->vip_cdenb);
	pr_info(" VIP_ODINT      = 0x%04x\r\n", p_reg->vip_odint);
	pr_info(" VIP_IMGWIDTH   = 0x%04x\r\n", p_reg->vip_imgwidth);
	pr_info(" VIP_IMGHEIGHT  = 0x%04x\r\n", p_reg->vip_imgheight);
	pr_info(" CLIP_LEFT      = 0x%04x\r\n", p_reg->clip_left);
	pr_info(" CLIP_RIGHT     = 0x%04x\r\n", p_reg->clip_right);
	pr_info(" CLIP_TOP       = 0x%04x\r\n", p_reg->clip_top);
	pr_info(" CLIP_BOTTOM    = 0x%04x\r\n", p_reg->clip_bottom);
	pr_info(" DECI_TARGETW   = 0x%04x\r\n", p_reg->deci_targetw);
	pr_info(" DECI_TARGETH   = 0x%04x\r\n", p_reg->deci_targeth);
	pr_info(" DECI_DELTAW    = 0x%04x\r\n", p_reg->deci_deltaw);
	pr_info(" DECI_DELTAH    = 0x%04x\r\n", p_reg->deci_deltah);
	pr_info(" DECI_CLEARW    = 0x%04x\r\n", p_reg->deci_clearw);
	pr_info(" DECI_CLEARH    = 0x%04x\r\n", p_reg->deci_clearh);
	pr_info(" DECI_FORMAT    = 0x%04x\r\n", p_reg->deci_format);
	pr_info(" DECI_LUADDR    = 0x%04x\r\n", p_reg->deci_luaddr);
	pr_info(" DECI_LUSTRIDE  = 0x%04x\r\n", p_reg->deci_lustride);
	pr_info(" DECI_CRADDR    = 0x%04x\r\n", p_reg->deci_craddr);
	pr_info(" DECI_CRSTRIDE  = 0x%04x\r\n", p_reg->deci_crstride);
	pr_info(" DECI_CBADDR    = 0x%04x\r\n", p_reg->deci_cbaddr);
	pr_info(" DECI_CBSTRIDE  = 0x%04x\r\n", p_reg->deci_cbstride);
	pr_info(" CLIP_FORMAT    = 0x%04x\r\n", p_reg->clip_format);
	pr_info(" CLIP_LUADDR    = 0x%04x\r\n", p_reg->clip_luaddr);
	pr_info(" CLIP_LUSTRIDE  = 0x%04x\r\n", p_reg->clip_lustride);
	pr_info(" CLIP_CRADDR    = 0x%04x\r\n", p_reg->clip_craddr);
	pr_info(" CLIP_CRSTRIDE  = 0x%04x\r\n", p_reg->clip_crstride);
	pr_info(" CLIP_CBADDR    = 0x%04x\r\n", p_reg->clip_cbaddr);
	pr_info(" CLIP_CBSTRIDE  = 0x%04x\r\n", p_reg->clip_cbstride);
	pr_info(" VIP_SCANMODE   = 0x%04x\r\n", p_reg->vip_scanmode);
	pr_info(" VIP_VIP1       = 0x%04x\r\n", p_reg->vip_vip1);
#else
	pr_info(" VIP_CONFIG	 = 0x%04x\r\n", p_reg->vip_config);
	pr_info(" VIP_INTCTRL	 = 0x%04x\r\n", p_reg->vip_hvint);
	pr_info(" VIP_SYNCCTRL	 = 0x%04x\r\n", p_reg->vip_syncctrl);
	pr_info(" VIP_SYNCMON	 = 0x%04x\r\n", p_reg->vip_syncmon);
	pr_info(" VIP_VBEGIN	 = 0x%04x\r\n", p_reg->vip_vbegin);
	pr_info(" VIP_VEND	 = 0x%04x\r\n", p_reg->vip_vend);
	pr_info(" VIP_HBEGIN	 = 0x%04x\r\n", p_reg->vip_hbegin);
	pr_info(" VIP_HEND	 = 0x%04x\r\n", p_reg->vip_hend);
	pr_info(" VIP_FIFOCTRL	 = 0x%04x\r\n", p_reg->vip_fifoctrl);
	pr_info(" VIP_HCOUNT	 = 0x%04x\r\n", p_reg->vip_hcount);
	pr_info(" VIP_VCOUNT	 = 0x%04x\r\n", p_reg->vip_vcount);
	pr_info(" VIP_CDENB	 = 0x%04x\r\n", p_reg->vip_cdenb);
	pr_info(" VIP_ODINT	 = 0x%04x\r\n", p_reg->vip_odint);
	pr_info(" VIP_IMGWIDTH	 = 0x%04x\r\n", p_reg->vip_imgwidth);
	pr_info(" VIP_IMGHEIGHT	 = 0x%04x\r\n", p_reg->vip_imgheight);
	pr_info(" CLIP_LEFT	 = 0x%04x\r\n", p_reg->clip_left);
	pr_info(" CLIP_RIGHT	 = 0x%04x\r\n", p_reg->clip_right);
	pr_info(" CLIP_TOP	 = 0x%04x\r\n", p_reg->clip_top);
	pr_info(" CLIP_BOTTOM	 = 0x%04x\r\n", p_reg->clip_bottom);
	pr_info(" DECI_TARGETW	 = 0x%04x\r\n", p_reg->deci_targetw);
	pr_info(" DECI_TARGETH	 = 0x%04x\r\n", p_reg->deci_targeth);
	pr_info(" DECI_DELTAW	 = 0x%04x\r\n", p_reg->deci_deltaw);
	pr_info(" DECI_DELTAH	 = 0x%04x\r\n", p_reg->deci_deltah);
	pr_info(" DECI_CLEARW	 = 0x%04x\r\n", p_reg->deci_clearw);
	pr_info(" DECI_CLEARH	 = 0x%04x\r\n", p_reg->deci_clearh);
	pr_info(" DECI_LUSEG	 = 0x%04x\r\n", p_reg->deci_luseg);
	pr_info(" DECI_CRSEG	 = 0x%04x\r\n", p_reg->deci_crseg);
	pr_info(" DECI_CBSEG	 = 0x%04x\r\n", p_reg->deci_cbseg);
	pr_info(" DECI_FORMAT	 = 0x%04x\r\n", p_reg->deci_format);
	pr_info(" DECI_ROTFLIP	 = 0x%04x\r\n", p_reg->deci_rotflip);
	pr_info(" DECI_LULEFT	 = 0x%04x\r\n", p_reg->deci_luleft);
	pr_info(" DECI_CRLEFT	 = 0x%04x\r\n", p_reg->deci_crleft);
	pr_info(" DECI_CBLEFT	 = 0x%04x\r\n", p_reg->deci_cbleft);
	pr_info(" DECI_LURIGHT	 = 0x%04x\r\n", p_reg->deci_luright);
	pr_info(" DECI_CRRIGHT	 = 0x%04x\r\n", p_reg->deci_crright);
	pr_info(" DECI_CBRIGHT	 = 0x%04x\r\n", p_reg->deci_cbright);
	pr_info(" DECI_LUTOP	 = 0x%04x\r\n", p_reg->deci_lutop);
	pr_info(" DECI_CRTOP	 = 0x%04x\r\n", p_reg->deci_crtop);
	pr_info(" DECI_CBTOP	 = 0x%04x\r\n", p_reg->deci_cbtop);
	pr_info(" DECI_LUBOTTOM	 = 0x%04x\r\n", p_reg->deci_lubottom);
	pr_info(" DECI_CRBOTTOM	 = 0x%04x\r\n", p_reg->deci_crbottom);
	pr_info(" DECI_CBBOTTOM	 = 0x%04x\r\n", p_reg->deci_cbbottom);
	pr_info(" CLIP_LUSEG	 = 0x%04x\r\n", p_reg->clip_luseg);
	pr_info(" CLIP_CRSEG	 = 0x%04x\r\n", p_reg->clip_crseg);
	pr_info(" CLIP_CBSEG	 = 0x%04x\r\n", p_reg->clip_cbseg);
	pr_info(" CLIP_FORMAT	 = 0x%04x\r\n", p_reg->clip_format);
	pr_info(" CLIP_ROTFLIP	 = 0x%04x\r\n", p_reg->clip_rotflip);
	pr_info(" CLIP_LULEFT	 = 0x%04x\r\n", p_reg->clip_luleft);
	pr_info(" CLIP_CRLEFT	 = 0x%04x\r\n", p_reg->clip_crleft);
	pr_info(" CLIP_CBLEFT	 = 0x%04x\r\n", p_reg->clip_cbleft);
	pr_info(" CLIP_LURIGHT	 = 0x%04x\r\n", p_reg->clip_luright);
	pr_info(" CLIP_CRRIGHT	 = 0x%04x\r\n", p_reg->clip_crright);
	pr_info(" CLIP_CBRIGHT	 = 0x%04x\r\n", p_reg->clip_cbright);
	pr_info(" CLIP_LUTOP	 = 0x%04x\r\n", p_reg->clip_lutop);
	pr_info(" CLIP_CRTOP	 = 0x%04x\r\n", p_reg->clip_crtop);
	pr_info(" CLIP_CBTOP	 = 0x%04x\r\n", p_reg->clip_cbtop);
	pr_info(" CLIP_LUBOTTOM	 = 0x%04x\r\n", p_reg->clip_lubottom);
	pr_info(" CLIP_CRBOTTOM	 = 0x%04x\r\n", p_reg->clip_crbottom);
	pr_info(" CLIP_CBBOTTOM	 = 0x%04x\r\n", p_reg->clip_cbbottom);
	pr_info(" VIP_SCANMODE	 = 0x%04x\r\n", p_reg->vip_scanmode);
	pr_info(" CLIP_YUYVENB	 = 0x%04x\r\n", p_reg->clip_yuyvenb);
	pr_info(" CLIP_BASEADDRH = 0x%04x\r\n", p_reg->clip_baseaddrh);
	pr_info(" CLIP_BASEADDRL = 0x%04x\r\n", p_reg->clip_baseaddrl);
	pr_info(" CLIP_STRIDEH = 0x%04x\r\n", p_reg->clip_strideh);
	pr_info(" CLIP_STRIDEL = 0x%04x\r\n", p_reg->clip_stridel);
	pr_info(" VIP_VIP1 = 0x%04x\r\n", p_reg->vip_vip1);
#endif
}
