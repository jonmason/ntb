/*****************************************************************************
	Copyright(c) 2013 FCI Inc. All Rights Reserved

	File name : fc8300_bb.c

	Description : source of baseband driver

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	History :
	----------------------------------------------------------------------
*******************************************************************************/
#include <linux/printk.h>

#include "fci_types.h"
#include "fci_oal.h"
#include "fci_hal.h"
#include "fci_tun.h"
#include "fc8300_regs.h"

#define SCAN_CHK_PERIOD 1 /* 1 ms */

static enum BROADCAST_TYPE broadcast_type;

static u32 fc8300_get_current_clk(HANDLE handle, DEVICEID devid)
{
	u32 pre_sel, post_sel, multi;
	u16 pll_set = 0;

	bbm_word_read(handle, devid, BBM_PLL1_PRE_POST_SELECTION, &pll_set);

	multi = ((pll_set & 0xff00) >> 8) + 2;
	pre_sel = pll_set & 0x000f;
	post_sel = (pll_set & 0x00f0) >> 4;

	return ((BBM_XTAL_FREQ >> pre_sel) * multi) >> post_sel;
}

static u32 fc8300_get_core_clk(HANDLE handle, DEVICEID devid,
		enum BROADCAST_TYPE broadcast, u32 freq)
{
	u32 clk;

#if (BBM_BAND_WIDTH == 6)
#if (BBM_XTAL_FREQ == 16000)
	clk = 100000;
#elif (BBM_XTAL_FREQ == 16384)
	clk = 98304;
#elif (BBM_XTAL_FREQ == 18000)
	clk = 99000;
#elif (BBM_XTAL_FREQ == 19200)
	clk = 100800;

	switch (broadcast) {
	case ISDBT_1SEG:
	case ISDBT_CATV_1SEG:
		switch (freq) {
		case 503143:
		case 605143:
		case 707143:
			clk = 110400;
		}
		break;
	case ISDBTMM_1SEG:
	case ISDBTSB_1SEG:
	case ISDBTSB_3SEG:
		break;
	case ISDBT_13SEG:
	case ISDBT_CATV_13SEG:
		switch (freq) {
		case 503143:
		case 605143:
		case 707143:
			clk = 110400;
		}
		break;
	case ISDBTMM_13SEG:
		break;
	}
#elif (BBM_XTAL_FREQ == 24000)
	clk = 99000;

	switch (broadcast) {
	case ISDBT_1SEG:
	case ISDBTMM_1SEG:
	case ISDBTSB_1SEG:
	case ISDBT_CATV_1SEG:
	case ISDBTSB_3SEG:
		break;
	case ISDBT_13SEG:
	case ISDBT_CATV_13SEG:
		switch (freq) {
		case 497143:
		case 593143:
		case 695143:
			clk = 120000;
		}
		break;
	case ISDBTMM_13SEG:
		break;
	}
#elif (BBM_XTAL_FREQ == 24576)
	clk = 98304;
#elif (BBM_XTAL_FREQ == 26000)
	clk = 100750;

	switch (broadcast) {
	case ISDBT_1SEG:
	case ISDBT_CATV_1SEG:
		switch (freq) {
		case 479143:
		case 503143:
		case 521143:
		case 569143:
		case 605143:
		case 653143:
		case 707143:
		case 731143:
		case 755143:
		case 773143:
		case 809143:
			clk = 107250;
		}
		break;
	case ISDBTMM_1SEG:
	case ISDBTSB_1SEG:
	case ISDBTSB_3SEG:
		break;
	case ISDBT_13SEG:
	case ISDBT_CATV_13SEG:
		switch (freq) {
		case 479143:
		case 503143:
		case 521143:
		case 569143:
		case 605143:
		case 653143:
		case 707143:
		case 731143:
		case 755143:
		case 773143:
		case 803143:
		case 809143:
			clk = 107250;
		}
		break;
	case ISDBTMM_13SEG:
		break;
	}
#elif (BBM_XTAL_FREQ == 27000)
	clk = 97875;
#elif (BBM_XTAL_FREQ == 27120)
	clk = 98310;

	switch (broadcast) {
	case ISDBT_1SEG:
	case ISDBT_CATV_1SEG:
		switch (freq) {
		case 491143:
		case 587143:
		case 689143:
			clk = 101700;
		}
		break;
	case ISDBTMM_1SEG:
		switch (freq) {
		case 221142:
		case 221143:
		case 221144:
			clk = 101700;
		}
		break;
	case ISDBTSB_1SEG:
	case ISDBTSB_3SEG:
		break;
	case ISDBT_13SEG:
	case ISDBT_CATV_13SEG:
		switch (freq) {
		case 491143:
		case 587143:
		case 689143:
			clk = 101700;
		}
		break;
	case ISDBTMM_13SEG:
		break;
	}
#elif (BBM_XTAL_FREQ == 32000)
	clk = 100000;

	switch (broadcast) {
	case ISDBT_1SEG:
	case ISDBT_CATV_1SEG:
		switch (freq) {
		case 683143:
			clk = 104000;
		}
		break;
	case ISDBTMM_1SEG:
	case ISDBTSB_1SEG:
	case ISDBTSB_3SEG:
		break;
	case ISDBT_13SEG:
		switch (freq) {
		case 599143:
		case 701143:
			clk = 116000;
		}
		break;
	case ISDBT_CATV_13SEG:
		switch (freq) {
		case 99143:
		case 153143:
		case 201143:
		case 231143:
		case 249143:
		case 297143:
		case 303143:
		case 351143:
		case 399143:
		case 497143:
		case 503143:
		case 533143:
		case 551143:
		case 599143:
		case 635143:
		case 653143:
		case 749143:
		case 767143:
			clk = 108000;
			break;
		case 647143:
		case 665143:
		case 683143:
			clk = 112000;
			break;
		}
		break;
	case ISDBTMM_13SEG:
		break;
	}
#elif (BBM_XTAL_FREQ == 37200)
	clk = 97650;

	switch (broadcast) {
	case ISDBT_1SEG:
	case ISDBT_CATV_1SEG:
		switch (freq) {
		case 659143:
			clk = 111600;
		}
		break;
	case ISDBTMM_1SEG:
		switch (freq) {
		case 219856:
		case 219857:
		case 219858:
			clk = 111600;
		}
		break;
	case ISDBTSB_1SEG:
	case ISDBTSB_3SEG:
		break;
	case ISDBT_13SEG:
	case ISDBT_CATV_13SEG:
		switch (freq) {
		case 587143:
		case 683143:
			clk = 111600;
		}
		break;
	case ISDBTMM_13SEG:
		break;
	}
#elif (BBM_XTAL_FREQ == 37400)
	clk = 98175;

	switch (broadcast) {
	case ISDBT_1SEG:
	case ISDBT_CATV_1SEG:
		switch (freq) {
		case 491143:
			clk = 102850;
		}
		break;
	case ISDBTMM_1SEG:
	case ISDBTSB_1SEG:
	case ISDBTSB_3SEG:
		break;
	case ISDBT_13SEG:
	case ISDBT_CATV_13SEG:
		switch (freq) {
		case 491143:
		case 587143:
		case 683143:
			clk = 102850;
		}
		break;
	case ISDBTMM_13SEG:
		break;
	}
#else /* (BBM_XTAL_FREQ == 38400) */
	clk = 100800;
#endif /* BBM_XTAL_FREQ */
#elif (BBM_BAND_WIDTH == 7)
#if (BBM_XTAL_FREQ == 16000)
	clk = 116000;
#elif (BBM_XTAL_FREQ == 16384)
	clk = 114688;
#elif (BBM_XTAL_FREQ == 18000)
	clk = 117000;
#elif (BBM_XTAL_FREQ == 19200)
	clk = 115200;
#elif (BBM_XTAL_FREQ == 24000)
	clk = 114000;
#elif (BBM_XTAL_FREQ == 24576)
	clk = 116736;
#elif (BBM_XTAL_FREQ == 26000)
	clk = 117000;
#elif (BBM_XTAL_FREQ == 27000)
	clk = 114750;
#elif (BBM_XTAL_FREQ == 27120)
	clk = 115260;
#elif (BBM_XTAL_FREQ == 32000)
	clk = 116000;
#elif (BBM_XTAL_FREQ == 37200)
	clk = 116250;
#elif (BBM_XTAL_FREQ == 37400)
	clk = 116875;
#else /* (BBM_XTAL_FREQ == 38400) */
	clk = 115200;
#endif /* BBM_XTAL_FREQ */
#else /* (BBM_BAND_WIDTH == 8) */
#if (BBM_XTAL_FREQ == 16000)
	clk = 132000;
#elif (BBM_XTAL_FREQ == 16384)
	clk = 131072;
#elif (BBM_XTAL_FREQ == 18000)
	clk = 130500;
#elif (BBM_XTAL_FREQ == 19200)
	clk = 134400;
#elif (BBM_XTAL_FREQ == 24000)
	clk = 132000;
#elif (BBM_XTAL_FREQ == 24576)
	clk = 135168;
#elif (BBM_XTAL_FREQ == 26000)
	clk = 136500;
#elif (BBM_XTAL_FREQ == 27000)
	clk = 131625;
#elif (BBM_XTAL_FREQ == 27120)
	clk = 132210;
#elif (BBM_XTAL_FREQ == 32000)
	clk = 132000;
#elif (BBM_XTAL_FREQ == 37200)
	clk = 130200;
#elif (BBM_XTAL_FREQ == 37400)
	clk = 130900;
#else /* (BBM_XTAL_FREQ == 38400) */
	clk = 134400;
#endif /* BBM_XTAL_FREQ */

#endif /* #if (BBM_BAND_WIDTH == 6) */

	return clk;
}

static s32 fc8300_set_acif_b31_1seg(HANDLE handle, DEVICEID devid, u32 clk)
{
#if (BBM_BAND_WIDTH == 6)
	switch (clk) {
	case 100750:
	case 100800:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02040200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfcf5f7fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x453c270e);
		break;
	case 105600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfef6f6fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x423a2710);
		break;
	case 110400:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030302);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x00f7f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x40392712);
		break;
	case 115200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xff020302);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x02f8f6fa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x3e382713);
		break;
	case 99000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03040200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfaf4f8ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x473e270d);
		break;
	case 102000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02040201);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfdf5f7fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x443c270f);
		break;
	case 105000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfef6f7fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x423b2710);
		break;
	case 108000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x413a2711);
		break;
	case 98310:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03030200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfaf4f8ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x473e260c);
		break;
	case 101700:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02040201);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfcf5f7fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x443c270f);
		break;
	case 105090:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfef6f7fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x423b2710);
		break;
	case 108480:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x413a2711);
		break;
	case 100000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03040200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfbf5f8fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x463d270d);
		break;
	case 104000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfef6f7fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x433b2710);
		break;
	/*case 108000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x413a2711);
		break;*/
	case 112000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030302);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x01f7f6fa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x3f392712);
		break;
	case 97650:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x030301ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf9f4f9ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x483e260b);
		break;
	case 102300:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02040201);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfdf5f7fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x443c270f);
		break;
	case 106950:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x413a2711);
		break;
	case 111600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030302);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x00f7f6fa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x3f392712);
		break;
	case 98175:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03030100);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfaf4f8ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x483e260c);
		break;
	case 102850:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfdf5f7fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x433b270f);
		break;
	case 107250:
	case 107525:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x413a2711);
		break;
	case 112200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030302);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x01f7f6fa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x3f392712);
		break;
	default:
		return BBM_NOK;
	}
#elif (BBM_BAND_WIDTH == 7)
	switch (clk) {
	case 115200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03040200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfaf4f8ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x473e260c);
		break;
	case 120000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfdf5f7fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x433b270f);
		break;
	case 124800:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x413a2711);
		break;
	case 129600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030302);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x00f7f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x40392712);
		break;
	case 114000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03030100);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf9f4f9ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x483e260c);
		break;
	case 117000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02040200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfbf5f7fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x453d270e);
		break;
	/*case 120000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfdf5f7fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x433b270f);
		break;*/
	case 126000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x413a2711);
		break;
	case 115260:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03040200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfaf4f8ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x473e260c);
		break;
	case 118650:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02040201);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfcf5f7fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x443c270f);
		break;
	case 122040:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfef6f7fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x423b2710);
		break;
	case 125430:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x413a2711);
		break;
	case 116000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03040200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfbf5f8fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x463d270d);
		break;
	/*case 120000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfdf5f7fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x433b270f);
		break;*/
	case 124000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x423a2711);
		break;
	case 128000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030302);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x00f7f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x40392712);
		break;
	case 116250:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03040200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfbf5f8fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x463d270d);
		break;
	case 120900:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfef6f7fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x433b2710);
		break;
	case 125550:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x413a2711);
		break;
	case 130200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030302);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x00f7f6fa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x3f392712);
		break;
	case 116875:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03040200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfbf5f8fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x463d270e);
		break;
	default:
		return BBM_NOK;
	}
#else /* BBM_BAND_WIDTH == 8) */
	switch (clk) {
	case 134400:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02040200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfcf5f7fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x453c270e);
		break;
	case 139200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfef6f7fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x433b2710);
		break;
	case 144000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x413a2711);
		break;
	case 148800:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030302);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x00f7f6fa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x3f392712);
		break;
	case 132000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03040200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfaf4f8ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x473e270d);
		break;
	case 138000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfdf6f7fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x433b2710);
		break;
	/*case 144000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x413a2711);
		break;*/
	case 150000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030302);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x01f7f6fa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x3f392712);
		break;
	case 132210:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03040200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfbf4f8fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x473d270d);
		break;
	case 135600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02040201);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfcf5f7fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x443c270f);
		break;
	case 142380:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x413a2711);
		break;
	case 149160:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030302);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x01f7f6fa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x3f392712);
		break;
	/*case 132000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03040200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfaf4f8ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x473e270d);
		break;*/
	case 136000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02040201);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfdf5f7fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x443c270f);
		break;
	case 140000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfef6f7fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x423b2710);
		break;
	/*case 144000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x413a2711);
		break;*/
	case 130200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x030301ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf9f4f9ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x483e260b);
		break;
	case 134850:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02040200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfcf5f7fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x453c270e);
		break;
	case 139500:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfef6f7fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x423b2710);
		break;
	case 144150:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00ffff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00030301);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfff6f6fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x413a2711);
		break;
	case 130900:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xffff0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03030100);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xfaf4f8ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x483e260c);
		break;
	default:
		return BBM_NOK;
	}
#endif /* #if (BBM_BAND_WIDTH == 6) */

	return BBM_OK;
}

static s32 fc8300_set_acif_1seg(HANDLE handle, DEVICEID devid, u32 clk)
{
#if (BBM_BAND_WIDTH == 6)
	switch (clk) {
	case 100750:
	case 100800:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfeff0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfcfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120b04ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2624201a);
		break;
	case 105600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfdfbfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0601);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23221f19);
		break;
	case 110400:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfdff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0702);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x22211e19);
		break;
	case 115200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfdfeff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00fdfcfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130e0804);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x201f1d19);
		break;
	case 99000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe000101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfbfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120a03fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2725211a);
		break;
	case 102000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdff0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfcfbfbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130b0500);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2524201a);
		break;
	case 105000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdfe0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfdfbfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0601);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x24221f19);
		break;
	case 108000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0702);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23211e19);
		break;
	case 98310:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe000101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfbfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120a03fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2725211a);
		break;
	case 101700:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdff0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfcfbfbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130b05ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2524201a);
		break;
	case 105090:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdfe0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfdfbfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0601);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x24221f19);
		break;
	case 108480:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0702);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x22211e19);
		break;
	case 100000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfeff0101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfcfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120b04ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2625201a);
		break;
	case 104000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdfe0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfdfbfbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0500);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x24231f1a);
		break;
	/*case 108000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0702);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23211e19);
		break;*/
	case 112000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfdfeff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffdfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0803);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x21201d19);
		break;
	case 97650:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe000101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfbfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120a03fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2726211a);
		break;
	case 102300:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdff0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfcfbfbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130b0500);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2524201a);
		break;
	case 106950:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0601);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23221e19);
		break;
	case 111600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfdfe00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0803);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x21201d19);
		break;
	case 98175:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe000101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfbfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120a03fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2725211a);
		break;
	case 102850:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdff0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfdfbfbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0500);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x25231f1a);
		break;
	case 107250:
	case 107525:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0702);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23221e19);
		break;
	case 112200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfdfeff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffdfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0803);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x21201d19);
		break;
	default:
		return BBM_NOK;
	}
#elif (BBM_BAND_WIDTH == 7)
	switch (clk) {
	case 115200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe000101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfbfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120a03fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2725211a);
		break;
	case 120000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdff0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfdfbfbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0500);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x25231f1a);
		break;
	case 124800:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0601);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23221e19);
		break;
	case 129600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfdfe00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0803);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x22201d19);
		break;
	case 114000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe000101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfbfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120a03fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2726211a);
		break;
	case 117000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfeff0101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfcfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120b04ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2624201a);
		break;
	/*case 120000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdff0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfdfbfbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0500);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x25231f1a);
		break;*/
	case 126000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0702);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23211e19);
		break;
	case 115260:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe000101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfbfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120a03fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2725211a);
		break;
	case 118650:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdff0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfcfbfbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130b05ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2524201a);
		break;
	case 122040:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdfe0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfdfbfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0601);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x24231f19);
		break;
	case 125430:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0702);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23221e19);
		break;
	case 116000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe000101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfbfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120a04ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2625201a);
		break;
	/*case 120000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdff0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfdfbfbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0500);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x25231f1a);
		break;*/
	case 124000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefbfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0601);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23221f19);
		break;
	case 128000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfdff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0702);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x22211e19);
		break;
	case 116250:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfeff0101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfbfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120b04ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2625201a);
		break;
	case 120900:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdfe0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfdfbfbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0500);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x24231f1a);
		break;
	case 125550:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0702);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23221e19);
		break;
	case 130200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfdfe00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0803);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x21201d19);
		break;
	case 116875:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfeff0101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfcfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120b04ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2624201a);
		break;
	default:
		return BBM_NOK;
	}
#else /* BBM_BAND_WIDTH == 8) */
	switch (clk) {
	case 134400:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfeff0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfcfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120b04ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2624201a);
		break;
	case 139200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdfe0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfdfbfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0601);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x24231f19);
		break;
	case 144000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0702);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23211e19);
		break;
	case 148800:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfdfe00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0803);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x21201d19);
		break;
	case 132000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe000101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfbfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120a03fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2725211a);
		break;
	case 138000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdff0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfdfbfbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0500);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x24231f1a);
		break;
	/*case 144000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0702);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23211e19);
		break;*/
	case 150000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfdfeff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffdfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130e0803);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x21201d19);
		break;
	case 132210:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe000101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfbfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120a04fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2625211a);
		break;
	case 135600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdff0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfcfbfbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130b05ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2524201a);
		break;
	case 142380:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0601);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23221e19);
		break;
	case 149160:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfdfeff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffdfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0803);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x21201d19);
		break;
	/*case 132000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe000101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfbfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120a03fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2725211a);
		break;*/
	case 136000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdff0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfcfbfbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130b0500);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2524201a);
		break;
	case 140000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdfe0000);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfdfbfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0601);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x24221f19);
		break;
	/*case 144000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0702);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23211e19);
		break;*/
	case 130200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe000101);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfbfafbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120a03fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2726211a);
		break;
	case 134850:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdff0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfcfbfbfc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x120b04ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x2524201a);
		break;
	case 139500:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfdfe0001);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfdfbfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130c0601);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x24231f19);
		break;
	case 144150:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0702);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23211e19);
		break;
	case 130900:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfcfeff00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefcfbfb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0x130d0702);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x23211e19);
		break;
	default:
		return BBM_NOK;
	}
#endif /* #if (BBM_BAND_WIDTH == 6) */

	return BBM_OK;
}

static s32 fc8300_set_acif_3seg(HANDLE handle, DEVICEID devid, u32 clk)
{
#if (BBM_BAND_WIDTH == 6)
	switch (clk) {
	case 100750:
	case 100800:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00fbfd01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefb0808);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x604b1cf6);
		break;
	case 105600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02fcfc00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef80509);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5c491ffa);
		break;
	case 110400:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x01020100);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x05fffcfe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff50208);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x584721fe);
		break;
	case 115200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xff010200);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0701fdfd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf1f3fe07);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x53452302);
		break;
	case 99000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0200ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffbfe02);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefc0807);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x614b1bf5);
		break;
	case 102000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01fbfd01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefa0708);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5f4a1df7);
		break;
	case 105000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020100ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02fcfc00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef80609);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5d4a1ef9);
		break;
	/*case 108000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef60409);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4820fc);
		break;*/
	case 98310:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0200ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffbfe02);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefc0907);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x624b1bf5);
		break;
	case 101700:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01fbfd01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefa0708);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5f4b1df7);
		break;
	case 105090:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02fcfc00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef80609);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5d491ef9);
		break;
	case 108480:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fefcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef60309);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4820fc);
		break;
	case 100000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00fbfd01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefb0808);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x604b1cf6);
		break;
	case 104000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02fcfc00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef90608);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5d4a1ef8);
		break;
	case 108000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef60409);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4820fc);
		break;
	case 112000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00020100);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0600fcfd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf0f40108);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x564722ff);
		break;
	case 97650:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0200ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefbfe02);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefd0907);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x624c1af4);
		break;
	case 102300:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01fbfd01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefa0708);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5f4a1df7);
		break;
	case 106950:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef70409);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5b491ffb);
		break;
	case 111600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00020100);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x06fffcfd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff40108);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x574722ff);
		break;
	case 98175:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0200ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffbfe02);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefc0907);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x624c1bf5);
		break;
	case 102850:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01fbfc01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef90708);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5e4a1df8);
		break;
	case 107250:
	case 107525:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef70409);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4920fb);
		break;
	case 112200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00020100);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0600fcfd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf0f40008);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x564622ff);
		break;
	default:
		return BBM_NOK;
	}
#elif (BBM_BAND_WIDTH == 7)
	switch (clk) {
	case 115200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0200ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffbfe02);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefc0807);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x614b1bf5);
		break;
	case 120000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01fbfc01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef90708);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5e4a1df8);
		break;
	case 124800:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef70409);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5b491ffb);
		break;
	case 129600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x01020100);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x05fffcfe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff50108);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x574722fe);
		break;
	case 114000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0200ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffbfe02);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefd0907);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x624c1af4);
		break;
	case 117000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00fbfd01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefb0808);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x604b1cf6);
		break;
	/*case 120000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01fbfc01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef90708);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5e4a1df8);
		break;*/
	case 126000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef60409);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4820fc);
		break;
	case 115260:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0200ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffbfe02);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefc0807);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x614b1bf5);
		break;
	case 118650:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01fbfd01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefa0708);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5f4b1df7);
		break;
	case 122040:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020100ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02fcfc00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef80609);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5d4a1ef9);
		break;
	case 125430:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef70409);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4920fb);
		break;
	case 116000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0200ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffbfd02);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefb0808);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x614b1bf5);
		break;
	/*case 120000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01fbfc01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef90708);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5e4a1df8);
		break;*/
	case 124000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef70509);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5c491ffa);
		break;
	case 128000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x010201ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x05fefcfe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff50208);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x584821fd);
		break;
	case 116250:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00fbfd01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefb0808);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x614b1cf6);
		break;
	case 120900:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01fcfc00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef90608);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5e4a1ef8);
		break;
	case 125550:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef70409);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4920fb);
		break;
	case 130200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00020100);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x06fffcfd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff40108);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x574722ff);
		break;
	case 116875:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00fbfd01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefb0808);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x604b1cf6);
		break;
	default:
		return BBM_NOK;
	}
#else /* BBM_BAND_WIDTH == 8) */
	switch (clk) {
	case 134400:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00fbfd01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefb0808);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x604b1cf6);
		break;
	case 139200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020100ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02fcfc00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef80609);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5d4a1ef9);
		break;
	case 144000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef60409);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4820fc);
		break;
	case 148800:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00020100);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x06fffcfd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff40108);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x574722ff);
		break;
	case 132000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0200ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffbfe02);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefc0807);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x614b1bf5);
		break;
	case 138000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01fcfc00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef90608);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5e4a1ef8);
		break;
	/*case 144000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef60409);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4820fc);
		break;*/
	case 150000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00020100);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0600fcfd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf0f40008);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x56462200);
		break;
	case 132210:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0200ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffbfd02);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefc0807);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x614b1bf5);
		break;
	case 135600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01fbfd01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefa0708);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5f4b1df7);
		break;
	case 142380:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef70509);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5b491ffb);
		break;
	case 149160:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00020100);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x06fffcfd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf0f40108);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x564722ff);
		break;
	/*case 132000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0200ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfffbfe02);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefc0807);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x614b1bf5);
		break;*/
	case 136000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x01fbfd01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefa0708);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5f4a1df7);
		break;
	case 140000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020100ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02fcfc00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef80609);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5d4a1ef9);
		break;
	/*case 144000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef60409);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4820fc);
		break;*/
	case 130200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0200ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0xfefbfe02);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefd0907);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x624c1af4);
		break;
	case 134850:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0201ffff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x00fbfd01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeefa0708);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x604b1cf7);
		break;
	case 139500:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020100ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02fcfc00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef80609);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5d4a1ef9);
		break;
	case 144150:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef60409);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4820fc);
		break;
	case 130900:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020200ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fdfcff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef60409);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4820fc);
		break;
	default:
		return BBM_NOK;
	}
#endif /* #if (BBM_BAND_WIDTH == 6) */

	return BBM_OK;
}

static s32 fc8300_set_acif_13seg(HANDLE handle, DEVICEID devid, u32 clk)
{
#if (BBM_BAND_WIDTH == 6)
	switch (clk) {
	case 100750:
	case 100800:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x040400fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fbf9fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf6050b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5b4920fa);
		break;
	case 105600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x010503ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x07fff9fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef3010a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x574722fe);
		break;
	case 110400:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe030402);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0902fbfa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf0f1fe08);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x53452402);
		break;
	case 115200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfc010403);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0905fefa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf2f0fb06);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x50432606);
		break;
	case 99000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0504fffc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03fafa00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xecf7070b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5d4a1ff9);
		break;
	case 102000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x030501fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x05fcf9fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf5040b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4920fb);
		break;
	case 105000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020503ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x07fef9fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef4020a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x574722fe);
		break;
	case 108000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00040400);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0800fafa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff2ff09);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x55462301);
		break;
	case 120000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfbfe0304);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x090700fb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf4f0f803);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x4d422709);
		break;
	case 98310:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0503fffc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02fafa00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xecf8070b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5d4a1ef8);
		break;
	case 101700:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x040401fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x05fcf9fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf5050b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4920fb);
		break;
	case 105090:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020503ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x07fef9fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef3020a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x574722fe);
		break;
	case 108480:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xff040401);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0801fafa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff2ff09);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x54462401);
		break;
	case 100000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x040400fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fbf9ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf7060b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5c4a1ffa);
		break;
	case 104000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020502fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x06fdf9fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef4030a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x584821fd);
		break;
	/*case 108000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00040400);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0800fafa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff2ff09);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x55462301);
		break;*/
	case 112000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfd020402);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0903fcfa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf1f1fd07);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x52452504);
		break;
	case 97650:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0503fefc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02fafa01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xecf8080a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5e4a1ef8);
		break;
	case 102300:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x030501fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x05fcf9fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf5040b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4921fc);
		break;
	case 106950:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00040400);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0800fafb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff3000a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x56472300);
		break;
	case 111600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfd020402);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0903fcfa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf1f1fd07);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x52452503);
		break;
	case 116000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfc000404);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0905fefa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf3f0fa05);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x4f432606);
		break;
	case 98175:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0503fffc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02fafa00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xecf8070b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5e4a1ef8);
		break;
	case 102850:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x030501fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x06fdf9fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf5040b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x594821fc);
		break;
	case 107250:
	case 107525:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00040400);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0800fafb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff20009);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x55462300);
		break;
	case 112200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfd020402);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0903fcfa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf1f1fc07);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x52452504);
		break;
	default:
		return BBM_NOK;
	}
#elif (BBM_BAND_WIDTH == 7)
	switch (clk) {
	case 115200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0504fffc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03fafa00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xecf8070b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5d4a1ff9);
		break;
	case 120000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x030501fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x06fdf9fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf5040b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x594821fc);
		break;
	case 124800:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00040400);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0800fafb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff30009);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x56472300);
		break;
	case 129600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe030402);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0903fcfa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf0f1fd08);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x52452403);
		break;
	case 114000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0503fefc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02fafa01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xecf8080a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5e4a1ef8);
		break;
	case 117000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x040400fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fbf9ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf6060b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5c4920fa);
		break;
	/*case 120000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x030501fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x06fdf9fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf5040b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x594821fc);
		break;*/
	case 126000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00040400);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0800fafa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff2ff09);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x55462301);
		break;
	case 115260:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0504fffc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03fafa00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xecf7070b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5d4a1ff9);
		break;
	case 118650:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x040401fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x05fcf9fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf5050b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4920fb);
		break;
	case 122040:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020502fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x07fef9fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef4020a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x584822fe);
		break;
	case 125430:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00040400);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0800fafb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff20009);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x55462300);
		break;
	case 116000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0504fffd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03fafaff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xecf7060b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5c4a1ff9);
		break;
	/*case 120000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x030501fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x06fdf9fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf5040b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x594821fc);
		break;*/
	case 124000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x010403ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x08fffafb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef3010a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x564723ff);
		break;
	case 128000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfe030401);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0902fbfa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf0f2fe08);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x53452402);
		break;
	case 116250:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0404fffd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03fbfaff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xecf7060b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5c4a1ff9);
		break;
	case 120900:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020502fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x06fdf9fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf4030a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x594821fd);
		break;
	case 125550:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00040400);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0800fafb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff20009);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x55462300);
		break;
	case 130200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfd020402);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0903fcfa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf1f1fd07);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x52452503);
		break;
	case 116875:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x040400fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fbf9ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf7060b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5c491ffa);
		break;
	default:
		return BBM_NOK;
	}
#else /* BBM_BAND_WIDTH == 8) */
	switch (clk) {
	case 134400:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x040400fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fbf9fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf6050b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5b4920fa);
		break;
	case 139200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020502fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x07fef9fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef4020a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x584822fd);
		break;
	case 144000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00040400);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0800fafa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff2ff09);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x55462301);
		break;
	case 148800:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfd020402);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0903fcfa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf1f1fd07);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x52452503);
		break;
	case 132000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0504fffc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03fafa00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xecf7070b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5d4a1ff9);
		break;
	case 138000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x030502fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x06fdf9fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf4030a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x594821fd);
		break;
	/*case 144000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00040400);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0800fafa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff2ff09);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x55462301);
		break;*/
	case 150000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfd020403);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0903fcfa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf1f1fc07);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x51442504);
		break;
	case 132210:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0504fffc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03fafa00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xecf7070b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5d4a1ff9);
		break;
	case 135600:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x040401fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x05fcf9fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf5050b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4920fb);
		break;
	case 142380:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00040300);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0800fafb);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff3000a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x56472300);
		break;
	case 149160:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xfd020402);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0903fcfa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xf1f1fd07);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x52452503);
		break;
	/*case 132000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0504fffc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x03fafa00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xecf7070b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5d4a1ff9);
		break;*/
	case 136000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x030501fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x05fcf9fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf5040b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5a4920fb);
		break;
	case 140000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020503ff);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x07fef9fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef4020a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x574722fe);
		break;
	/*case 144000:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x00040400);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0800fafa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff2ff09);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x55462301);
		break;*/
	case 130200:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0503fefc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02fafa01);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xecf8080a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5e4a1ef8);
		break;
	case 134850:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x040400fd);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x04fbf9fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xedf6050b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5b4920fb);
		break;
	case 139500:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x020502fe);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x07fef9fc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeef4020a);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x584822fe);
		break;
	case 144150:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0xff040400);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x0801fafa);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xeff2ff09);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x55462301);
		break;
	case 130900:
		bbm_long_write(handle, devid, BBM_ACIF_COEF_00, 0x0503fffc);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_04, 0x02fafa00);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_08, 0xecf8070b);
		bbm_long_write(handle, devid, BBM_ACIF_COEF_12, 0x5e4a1ef8);
		break;
	default:
		return BBM_NOK;
	}
#endif /* #if (BBM_BAND_WIDTH == 6) */

	return BBM_OK;
}

static s32 fc8300_set_cal_front_1seg(HANDLE handle, DEVICEID devid, u32 clk)
{
#if (BBM_BAND_WIDTH == 6)
	switch (clk) {
	case 97650:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2fd1);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3fead);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x200a999a);
		break;
	case 97875:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2fb5);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3fc53);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x201d8000);
		break;
	case 98175:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2f90);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f935);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x2036b333);
		break;
	case 98304:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2f80);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f7df);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20418937);
		break;
	case 98310:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2f7f);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f7d0);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20420a3d);
		break;
	case 99000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2f2b);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f0bb);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x207c0000);
		break;
	case 100000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2eb2);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3e6a5);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20d00000);
		break;
	case 100750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2e59);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3df36);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x210f0000);
		break;
	case 100800:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2e53);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3deb8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21133333);
		break;
	case 101250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2e1e);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3da51);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21390000);
		break;
	case 101376:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2e10);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d917);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21439581);
		break;
	case 101700:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2dea);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d5f3);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x215ecccd);
		break;
	case 102400:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2d9a);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3cf3d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x2199999a);
		break;
	case 102850:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2d67);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3caf9);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21bf6666);
		break;
	case 103125:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2d48);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3c862);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21d68000);
		break;
	case 104000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2ce6);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3c03c);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x22200000);
		break;
	case 105600:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2c38);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3b1af);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x22a66666);
		break;
	case 107250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2b8a);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3a323);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x23310000);
		break;
	case 107525:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2b6d);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3a0c1);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x2348199a);
		break;
	case 108000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2b3c);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x39cac);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x23700000);
		break;
	case 110400:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2a4c);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x38892);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x2439999a);
		break;
	case 111600:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x29d7);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x37ed8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x249e6666);
		break;
	case 112000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x29b1);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x37ba5);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x24c00000);
		break;
	case 112200:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x299e);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x37a0f);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x24d0cccd);
		break;
	default:
		return BBM_NOK;
	}
#elif (BBM_BAND_WIDTH == 7)
	switch (clk) {
	case 114000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x28f6);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3fe01);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20100000);
		break;
	case 114688:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x28b7);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f7df);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20418937);
		break;
	case 114750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x28b1);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f753);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20460000);
		break;
	case 115200:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2889);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f35c);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20666666);
		break;
	case 115260:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2883);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f2d5);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x206ab852);
		break;
	case 115625:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2862);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3efa4);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20850000);
		break;
	case 116000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2841);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3ec62);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20a00000);
		break;
	case 116250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x282b);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3ea39);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20b20000);
		break;
	case 116736:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2800);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3e60d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20d4fdf4);
		break;
	case 116875:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x27f4);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x35674);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x26598000);
		break;
	case 117000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x27e9);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3e3cc);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20e80000);
		break;
	case 118125:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2788);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3da51);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21390000);
		break;
	case 118650:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x275b);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d5f3);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x215ecccd);
		break;
	case 118750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2752);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d520);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21660000);
		break;
	case 118784:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x274f);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d4d8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x216872b0);
		break;
	case 120000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x26e9);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3cae7);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21c00000);
		break;
	case 120900:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x269f);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3c3ad);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x2200cccd);
		break;
	default:
		return BBM_NOK;
	}
#else /* (BBM_BAND_WIDTH == 8) */
	switch (clk) {
	case 130200:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x23dd);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3fead);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x200a999a);
		break;
	case 130500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x23c8);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3fc53);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x201d8000);
		break;
	case 130900:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x23ac);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x2fae8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x2af3999a);
		break;
	case 131072:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x23a0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f7df);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20418937);
		break;
	case 131250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2394);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f67f);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x204cc000);
		break;
	case 131625:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x237a);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f39b);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20646000);
		break;
	case 132000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2360);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f0bb);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x207c0000);
		break;
	case 132210:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2351);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3ef21);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x20893ae1);
		break;
	case 134400:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x22be);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3deb8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21133333);
		break;
	case 134850:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x22a0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3db69);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x212f8ccd);
		break;
	case 135000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2297);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3da51);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21390000);
		break;
	case 135168:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x228c);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d917);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21439581);
		break;
	case 135600:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x226f);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d5f3);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x215ecccd);
		break;
	case 136000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2256);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d310);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21780000);
		break;
	case 136500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2235);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3cf7a);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21978000);
		break;
	case 137500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x21f6);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3c862);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21d68000);
		break;
	case 138000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x21d6);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3c4e0);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x21f60000);
		break;
	case 139200:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x218b);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3bc8e);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x2241999a);
		break;
	case 139264:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2188);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3bc1e);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x2245a1cb);
		break;
	case 139500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x2179);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3ba80);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x22548000);
		break;
	case 141312:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x210b);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3ae42);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x22c6a7f0);
		break;
	case 141750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x20f1);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3ab59);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x22e24000);
		break;
	case 143000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x20a7);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3a323);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x23310000);
		break;
	default:
		return BBM_NOK;
	}
#endif

	return BBM_OK;
}

static s32 fc8300_set_cal_front_3seg(HANDLE handle, DEVICEID devid, u32 clk)
{
#if (BBM_BAND_WIDTH == 6)
	switch (clk) {
	case 97650:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x7dd6);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3fead);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10054ccd);
		break;
	case 97875:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x7d8c);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3fc53);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x100ec000);
		break;
	case 98175:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x7d2a);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f935);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x101b599a);
		break;
	case 98304:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x7d00);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f7df);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1020c49c);
		break;
	case 98310:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x7cfe);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f7d0);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1021051f);
		break;
	case 99000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x7c1f);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f0bb);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x103e0000);
		break;
	case 100000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x7ae1);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3e6a5);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10680000);
		break;
	case 100750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x79f7);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3df36);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10878000);
		break;
	case 100800:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x79e8);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3deb8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1089999a);
		break;
	case 101250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x795d);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3da51);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x109c8000);
		break;
	case 101376:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x7936);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d917);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10a1cac1);
		break;
	case 101700:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x78d3);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d5f3);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10af6666);
		break;
	case 102000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x7878);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d310);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10bc0000);
		break;
	case 102300:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x781e);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d031);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10c8999a);
		break;
	case 102400:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x7800);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3cf3d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10cccccd);
		break;
	case 102850:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x777a);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3caf9);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10dfb333);
		break;
	case 103125:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x7728);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3c862);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10eb4000);
		break;
	case 103500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x76ba);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3c4e0);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10fb0000);
		break;
	case 104000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x7627);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3c03c);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11100000);
		break;
	case 105600:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x745d);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3b1af);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11533333);
		break;
	case 107250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x7293);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3a323);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11988000);
		break;
	case 107525:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x7248);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3a0c1);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11a40ccd);
		break;
	case 108000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x71c7);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x39cac);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11b80000);
		break;
	case 112000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6db7);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x37ba5);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12600000);
		break;
	case 112200:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6d85);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x37a0f);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12686666);
		break;
	default:
		return BBM_NOK;
	}
#elif (BBM_BAND_WIDTH == 7)
	switch (clk) {
	case 114000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6bca);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3fe01);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10080000);
		break;
	case 114688:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6b25);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f7df);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1020c49c);
		break;
	case 114750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6b16);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f753);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10230000);
		break;
	case 115200:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6aab);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f35c);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10333333);
		break;
	case 115260:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6a9c);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f2d5);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10355c29);
		break;
	case 115625:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6a46);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3efa4);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10428000);
		break;
	case 116000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x69ee);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3ec62);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10500000);
		break;
	case 116250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x69b4);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3ea39);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10590000);
		break;
	case 116736:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6943);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3e60d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x106a7efa);
		break;
	case 116875:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6923);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x35674);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x132cc000);
		break;
	case 117000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6907);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3e3cc);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10740000);
		break;
	case 118125:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6807);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3da51);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x109c8000);
		break;
	case 118650:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6791);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d5f3);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10af6666);
		break;
	case 118750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x677a);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d520);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10b30000);
		break;
	case 118784:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6773);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d4d8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10b43958);
		break;
	case 119808:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6690);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3cc76);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10d91687);
		break;
	case 120000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6666);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3cae7);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10e00000);
		break;
	case 120250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6630);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3c8e3);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10e90000);
		break;
	case 120900:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x65a3);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3c3ad);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11006666);
		break;
	case 121500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x6523);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3beeb);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11160000);
		break;
	default:
		return BBM_NOK;
	}
#else /* BBM_BAND_WIDTH == 8) */
	switch (clk) {
	case 130200:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x5e61);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3fead);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10054ccd);
		break;
	case 130500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x5e29);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3fc53);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x100ec000);
		break;
	case 130900:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x5de0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x2fae8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1579cccd);
		break;
	case 131072:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x5dc0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f7df);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1020c49c);
		break;
	case 131250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x5d9f);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f67f);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10266000);
		break;
	case 131625:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x5d5b);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f39b);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10323000);
		break;
	case 132000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x5d17);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3f0bb);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x103e0000);
		break;
	case 132210:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x5cf1);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3ef21);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10449d71);
		break;
	case 134400:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x5b6e);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3deb8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1089999a);
		break;
	case 135168:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x5ae9);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3d917);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10a1cac1);
		break;
	case 136500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x5a06);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3cf7a);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10cbc000);
		break;
	case 141312:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x56f5);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3ae42);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x116353f8);
		break;
	case 143000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x55ee);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0x3a323);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11988000);
		break;
	default:
		return BBM_NOK;
	}
#endif /* #if (BBM_BAND_WIDTH == 6) */

	return BBM_OK;
}

static s32 fc8300_set_cal_front_13seg(HANDLE handle, DEVICEID devid, u32 clk)
{
#if (BBM_BAND_WIDTH == 6)
	switch (clk) {
	case 97650:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xffab);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10054ccd);
		break;
	case 97875:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xff15);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x100ec000);
		break;
	case 98175:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfe4d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x101b599a);
		break;
	case 98304:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfdf8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1020c49c);
		break;
	case 98310:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfdf4);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1021051f);
		break;
	case 99000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfc2f);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x103e0000);
		break;
	case 100000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf9a9);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10680000);
		break;
	case 100750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf7cd);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10878000);
		break;
	case 100800:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf7ae);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1089999a);
		break;
	case 101250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf694);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x109c8000);
		break;
	case 101376:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf646);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10a1cac1);
		break;
	case 101700:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf57d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10af6666);
		break;
	case 102000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf4c4);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10bc0000);
		break;
	case 102300:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf40c);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10c8999a);
		break;
	case 102400:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf3cf);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10cccccd);
		break;
	case 102850:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf2be);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10dfb333);
		break;
	case 103125:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf218);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10eb4000);
		break;
	case 103500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf138);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10fb0000);
		break;
	case 104000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf00f);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11100000);
		break;
	case 104448:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xef07);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1122d0e5);
		break;
	case 104625:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xeea0);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x112a4000);
		break;
	case 105000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xedc6);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x113a0000);
		break;
	case 105090:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xed92);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x113dc7ae);
		break;
	case 105600:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xec6c);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11533333);
		break;
	case 106250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xeafa);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x116e8000);
		break;
	case 106496:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xea6f);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1178d4fe);
		break;
	case 106950:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe970);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x118be666);
		break;
	case 107250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe8c9);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11988000);
		break;
	case 107525:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe830);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11a40ccd);
		break;
	case 108000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe72b);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11b80000);
		break;
	case 108480:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe625);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11cc28f6);
		break;
	case 109375:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe443);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11f1c000);
		break;
	case 110400:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe224);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x121ccccd);
		break;
	case 110500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe1f0);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12210000);
		break;
	case 110592:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe1c0);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1224dd2f);
		break;
	case 111600:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xdfb6);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x124f3333);
		break;
	case 112000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xdee9);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12600000);
		break;
	case 112200:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xde84);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12686666);
		break;
	case 112500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xddec);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12750000);
		break;
	case 114000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xdb00);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12b40000);
		break;
	case 114750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd992);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12d38000);
		break;
	case 115200:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd8b8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12e66666);
		break;
	case 116000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd73a);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13080000);
		break;
	case 117000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd563);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13320000);
		break;
	case 118125:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd35a);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13614000);
		break;
	case 118750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd23e);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x137b8000);
		break;
	case 120000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd00d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13b00000);
		break;
	case 120250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xcf9e);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13ba8000);
		break;
	default:
		return BBM_NOK;
	}
#elif (BBM_BAND_WIDTH == 7)
	switch (clk) {
	case 114000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xff80);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10080000);
		break;
	case 114688:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfdf8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1020c49c);
		break;
	case 114750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfdd5);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10230000);
		break;
	case 115200:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfcd7);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10333333);
		break;
	case 115260:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfcb5);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10355c29);
		break;
	case 115625:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfbe9);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10428000);
		break;
	case 116000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfb19);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10500000);
		break;
	case 116250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfa8e);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10590000);
		break;
	case 116736:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf983);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x106a7efa);
		break;
	case 116875:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd59d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x132cc000);
		break;
	case 117000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf8f3);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10740000);
		break;
	case 118125:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf694);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x109c8000);
		break;
	case 118650:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf57d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10af6666);
		break;
	case 118750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf548);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10b30000);
		break;
	case 118784:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf536);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10b43958);
		break;
	case 119808:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf31d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10d91687);
		break;
	case 120000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf2ba);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10e00000);
		break;
	case 120250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf239);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10e90000);
		break;
	case 120900:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf0eb);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11006666);
		break;
	case 121500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xefbb);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11160000);
		break;
	case 121875:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xeefe);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11238000);
		break;
	case 122040:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xeeab);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x112970a4);
		break;
	case 122880:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xed09);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1147ae14);
		break;
	case 123500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xebd9);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x115e0000);
		break;
	case 124000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xeae5);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11700000);
		break;
	case 124800:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe964);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x118ccccd);
		break;
	case 124875:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe940);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x118f8000);
		break;
	case 125000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe904);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11940000);
		break;
	case 125430:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe838);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11a37ae1);
		break;
	case 125550:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe7ff);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11a7cccd);
		break;
	case 126000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe72b);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11b80000);
		break;
	case 126750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe5cd);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11d30000);
		break;
	case 126976:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe564);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11db22d1);
		break;
	case 128000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe38e);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12000000);
		break;
	case 128250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe31d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12090000);
		break;
	case 128820:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe21b);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x121d851f);
		break;
	case 129024:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe1c0);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1224dd2f);
		break;
	case 129600:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe0bf);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1239999a);
		break;
	case 130000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe00e);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12480000);
		break;
	case 130200:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xdfb6);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x124f3333);
		break;
	case 130500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xdf32);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x125a0000);
		break;
	case 131072:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xde39);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x126e978d);
		break;
	case 131250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xddec);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12750000);
		break;
	case 131625:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xdd4a);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12828000);
		break;
	case 132000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xdca9);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12900000);
		break;
	case 132210:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xdc4f);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12978f5c);
		break;
	case 134400:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd8b8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12e66666);
		break;
	case 134850:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd7ff);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12f6999a);
		break;
	case 135000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd7c2);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12fc0000);
		break;
	case 135168:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd77d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13020c4a);
		break;
	case 136000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd62c);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13200000);
		break;
	case 136500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd563);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13320000);
		break;
	case 139200:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd13f);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13933333);
		break;
	case 139500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd0cc);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x139e0000);
		break;
	case 140000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd00d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13b00000);
		break;
	case 141750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xcd7b);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13ef0000);
		break;
	default:
		return BBM_NOK;
	}
#else /* BBM_BAND_WIDTH == 8) */
	switch (clk) {
	case 130200:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xffab);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10054ccd);
		break;
	case 130500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xff15);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x100ec000);
		break;
	case 130900:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xbeba);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1579cccd);
		break;
	case 131072:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfdf8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1020c49c);
		break;
	case 131250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfda0);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10266000);
		break;
	case 131625:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfce7);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10323000);
		break;
	case 132000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfc2f);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x103e0000);
		break;
	case 132210:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xfbc8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10449d71);
		break;
	case 134400:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf7ae);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1089999a);
		break;
	case 134850:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf6da);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1097c666);
		break;
	case 135000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf694);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x109c8000);
		break;
	case 135168:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf646);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10a1cac1);
		break;
	case 135600:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf57d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10af6666);
		break;
	case 136000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf4c4);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10bc0000);
		break;
	case 136500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf3de);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10cbc000);
		break;
	case 137500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf218);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10eb4000);
		break;
	case 138000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xf138);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x10fb0000);
		break;
	case 139200:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xef24);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1120cccd);
		break;
	case 139264:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xef07);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1122d0e5);
		break;
	case 139500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xeea0);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x112a4000);
		break;
	case 140000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xedc6);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x113a0000);
		break;
	case 141312:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xeb91);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x116353f8);
		break;
	case 141750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xead6);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11712000);
		break;
	case 142380:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe9cc);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1184f852);
		break;
	case 143000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe8c9);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11988000);
		break;
	case 143360:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe833);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11a3d70a);
		break;
	case 143750:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe792);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11b02000);
		break;
	case 144000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe72b);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11b80000);
		break;
	case 144150:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe6ed);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x11bcb99a);
		break;
	case 147456:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe1c0);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1224dd2f);
		break;
	case 148500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xe02a);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1245c000);
		break;
	case 149500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xdeaa);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12654000);
		break;
	case 150000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xddec);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12750000);
		break;
	case 152000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xdb00);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12b40000);
		break;
	case 153000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd992);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12d38000);
		break;
	case 153450:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd8ee);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12e1accd);
		break;
	case 153600:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd8b8);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x12e66666);
		break;
	case 156000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd563);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13320000);
		break;
	case 156250:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd50b);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x1339e000);
		break;
	case 157500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd35a);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13614000);
		break;
	case 158100:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd28d);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13742666);
		break;
	case 158400:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd227);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x137d999a);
		break;
	case 159744:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xd062);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13a7ef9e);
		break;
	case 162000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xcd7b);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13ef0000);
		break;
	case 162500:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xccda);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x13fec000);
		break;
	case 165888:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xc8ab);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x146978d5);
		break;
	case 169000:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xc4f9);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x14cb8000);
		break;
	case 172032:
		bbm_word_write(handle, devid, BBM_LOW_IF_VALUE, 0x0);
		bbm_long_write(handle, devid, BBM_FREQ_COMPEN_VAL0, 0xc180);
		bbm_long_write(handle, devid, BBM_NCO_OFFSET, 0x152b020c);
		break;
	default:
		return BBM_NOK;
	}
#endif /* #if (BBM_BAND_WIDTH == 6) */

	return BBM_OK;
}

static s32 fc8300_set_default_core_clk(HANDLE handle, DEVICEID devid)
{
	u16 pll_set;

#if (BBM_BAND_WIDTH == 6)
#if (BBM_XTAL_FREQ == 16000)
	pll_set = 0x1711;
#elif (BBM_XTAL_FREQ == 16384)
	pll_set = 0x1611;
#elif (BBM_XTAL_FREQ == 18000)
	pll_set = 0x1411;
#elif (BBM_XTAL_FREQ == 19200)
	pll_set = 0x1311;
#elif (BBM_XTAL_FREQ == 24000)
	pll_set = 0x1f12;
#elif (BBM_XTAL_FREQ == 24576)
	pll_set = 0x1e12;
#elif (BBM_XTAL_FREQ == 26000)
	pll_set = 0x1d12;
#elif (BBM_XTAL_FREQ == 27000)
	pll_set = 0x1b12;
#elif (BBM_XTAL_FREQ == 27120)
	pll_set = 0x1b12;
#elif (BBM_XTAL_FREQ == 32000)
	pll_set = 0x1712;
#elif (BBM_XTAL_FREQ == 37200)
	pll_set = 0x1312;
#elif (BBM_XTAL_FREQ == 37400)
	pll_set = 0x1312;
#elif (BBM_XTAL_FREQ == 38400)
	pll_set = 0x1312;
#endif /* BBM_XTAL_FREQ */
#elif (BBM_BAND_WIDTH == 7)
#if (BBM_XTAL_FREQ == 16000)
	pll_set = 0x1b11;
#elif (BBM_XTAL_FREQ == 16384)
	pll_set = 0x1a11;
#elif (BBM_XTAL_FREQ == 18000)
	pll_set = 0x1811;
#elif (BBM_XTAL_FREQ == 19200)
	pll_set = 0x1611;
#elif (BBM_XTAL_FREQ == 24000)
	pll_set = 0x2412;
#elif (BBM_XTAL_FREQ == 24576)
	pll_set = 0x2412;
#elif (BBM_XTAL_FREQ == 26000)
	pll_set = 0x2212;
#elif (BBM_XTAL_FREQ == 27000)
	pll_set = 0x2012;
#elif (BBM_XTAL_FREQ == 27120)
	pll_set = 0x2012;
#elif (BBM_XTAL_FREQ == 32000)
	pll_set = 0x1b12;
#elif (BBM_XTAL_FREQ == 37200)
	pll_set = 0x1712;
#elif (BBM_XTAL_FREQ == 37400)
	pll_set = 0x1712;
#elif (BBM_XTAL_FREQ == 38400)
	pll_set = 0x1612;
#endif /* BBM_XTAL_FREQ */
#else /* BBM_BAND_WIDTH == 8 */
#if (BBM_XTAL_FREQ == 16000)
	pll_set = 0x1f11;
#elif (BBM_XTAL_FREQ == 16384)
	pll_set = 0x1e11;
#elif (BBM_XTAL_FREQ == 18000)
	pll_set = 0x1b11;
#elif (BBM_XTAL_FREQ == 19200)
	pll_set = 0x1a11;
#elif (BBM_XTAL_FREQ == 24000)
	pll_set = 0x1402;
#elif (BBM_XTAL_FREQ == 24576)
	pll_set = 0x1402;
#elif (BBM_XTAL_FREQ == 26000)
	pll_set = 0x1302;
#elif (BBM_XTAL_FREQ == 27000)
	pll_set = 0x2512;
#elif (BBM_XTAL_FREQ == 27120)
	pll_set = 0x2512;
#elif (BBM_XTAL_FREQ == 32000)
	pll_set = 0x1f12;
#elif (BBM_XTAL_FREQ == 37200)
	pll_set = 0x1a12;
#elif (BBM_XTAL_FREQ == 37400)
	pll_set = 0x1a12;
#elif (BBM_XTAL_FREQ == 38400)
	pll_set = 0x1a12;
#endif /* BBM_XTAL_FREQ */
#endif /* BBM_BAND_WIDTH */

	bbm_byte_write(handle, devid, BBM_PLL_SEL, 0x00);
	bbm_byte_write(handle, devid, BBM_PLL1_RESET, 0x01);
	bbm_word_write(handle, devid, BBM_PLL1_PRE_POST_SELECTION, pll_set);
	bbm_byte_write(handle, devid, BBM_PLL1_RESET, 0x00);
	msWait(1);
	bbm_byte_write(handle, devid, BBM_PLL_SEL, 0x01);

	return fc8300_get_current_clk(handle, devid);
}

#ifdef BBM_I2C_TSIF
static s32 fc8300_set_tsif_clk(HANDLE handle, DEVICEID devid)
{
	bbm_byte_write(handle, devid, BBM_TS_CLK_DIV, 0x00);

#ifdef BBM_TSIF_CLK_ARBITRARY
	u32 pre_sel, post_sel = 0, multi = 0, div_sel;
	u32 input_freq;
	u32 multi_3, multi_2, multi_1;

	if (BBM_XTAL_FREQ <= 10000)
		pre_sel = 0;
	else if (BBM_XTAL_FREQ <= 20000)
		pre_sel = 1;
	else if (BBM_XTAL_FREQ <= 40000)
		pre_sel = 2;
	else
		pre_sel = 3;

	input_freq = BBM_XTAL_FREQ >> pre_sel;

	multi_3 = (BBM_TSIF_CLK << 3) / input_freq;
	multi_2 = (BBM_TSIF_CLK << 2) / input_freq;
	multi_1 = (BBM_TSIF_CLK << 1) / input_freq;

	if (multi_3 >= 10 && multi_3 <= 40) {
		post_sel = 3;
		multi = multi_3;
	} else if (multi_2 >= 10 && multi_2 <= 40) {
		post_sel = 2;
		multi = multi_2;
	} else if (multi_1 >= 10 && multi_1 <= 40) {
		post_sel = 1;
		multi = multi_1;
	} else { /* post_sel == 0 */
		multi = BBM_TSIF_CLK / input_freq;
	}

	div_sel = multi - 2;
	/*pll_out = (input_freq * multi) >> post_sel;*/

	bbm_byte_write(handle, devid, BBM_PLL2_ENABLE, 0x01);
	bbm_byte_write(handle, devid, BBM_PLL2_PD, 0x00);
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION,
		(post_sel << 4) | pre_sel | (div_sel << 8));
#else
	bbm_byte_write(handle, devid, BBM_PLL2_ENABLE, 0x01);
	bbm_byte_write(handle, devid, BBM_PLL2_PD, 0x00);
#if (BBM_TSIF_CLK == 48000)
#if (BBM_XTAL_FREQ == 16000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1621);
#elif (BBM_XTAL_FREQ == 16384)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1521);
#elif (BBM_XTAL_FREQ == 18000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1321);
#elif (BBM_XTAL_FREQ == 19200)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1221);
#elif (BBM_XTAL_FREQ == 24000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1e22);
#elif (BBM_XTAL_FREQ == 24576)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1d22);
#elif (BBM_XTAL_FREQ == 26000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1c22);
#elif (BBM_XTAL_FREQ == 27000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1b22);
#elif (BBM_XTAL_FREQ == 27120)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1a22);
#elif (BBM_XTAL_FREQ == 32000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1622);
#elif (BBM_XTAL_FREQ == 37200)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1322);
#elif (BBM_XTAL_FREQ == 37400)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1322);
#elif (BBM_XTAL_FREQ == 38400)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1222);
#endif /* BBM_XTAL_FREQ */
#elif (BBM_TSIF_CLK == 32000)
#if (BBM_XTAL_FREQ == 16000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0e21);
#elif (BBM_XTAL_FREQ == 16384)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0e21);
#elif (BBM_XTAL_FREQ == 18000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0c21);
#elif (BBM_XTAL_FREQ == 19200)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0b21);
#elif (BBM_XTAL_FREQ == 24000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1322);
#elif (BBM_XTAL_FREQ == 24576)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1322);
#elif (BBM_XTAL_FREQ == 26000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1222);
#elif (BBM_XTAL_FREQ == 27000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1122);
#elif (BBM_XTAL_FREQ == 27120)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x1122);
#elif (BBM_XTAL_FREQ == 32000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0e22);
#elif (BBM_XTAL_FREQ == 37200)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0c22);
#elif (BBM_XTAL_FREQ == 37400)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0c22);
#elif (BBM_XTAL_FREQ == 38400)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0b22);
#endif /* BBM_XTAL_FREQ */
#else /* (BBM_TSIF_CLK == 26000) */
#if (BBM_XTAL_FREQ == 16000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0b21);
#elif (BBM_XTAL_FREQ == 16384)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0b21);
#elif (BBM_XTAL_FREQ == 18000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0921);
#elif (BBM_XTAL_FREQ == 19200)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0921);
#elif (BBM_XTAL_FREQ == 24000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0f22);
#elif (BBM_XTAL_FREQ == 24576)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0f22);
#elif (BBM_XTAL_FREQ == 26000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0e22);
#elif (BBM_XTAL_FREQ == 27000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0d22);
#elif (BBM_XTAL_FREQ == 27120)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0d22);
#elif (BBM_XTAL_FREQ == 32000)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0b22);
#elif (BBM_XTAL_FREQ == 37200)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0922);
#elif (BBM_XTAL_FREQ == 37400)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0922);
#elif (BBM_XTAL_FREQ == 38400)
	bbm_word_write(handle, devid, BBM_PLL2_PRE_POST_SELECTION, 0x0922);
#endif /* BBM_XTAL_FREQ */
#endif /* #if (BBM_TSIF_CLK == 48000) */
#endif /* #ifdef BBM_TSIF_CLK_ARBITRARY */

	bbm_byte_write(handle, devid, BBM_PLL2_RESET, 0x00);

	return BBM_OK;
}
#endif /* #ifdef BBM_I2C_TSIF */

s32 fc8300_reset(HANDLE handle, DEVICEID devid)
{
	bbm_byte_write(handle, DIV_BROADCAST, BBM_SW_RESET, 0x7f);
	msWait(2);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_SW_RESET, 0xcf);
	msWait(2);
	bbm_byte_write(handle, DIV_MASTER, BBM_SW_RESET, 0xff);
	msWait(2);

	return BBM_OK;
}

s32 fc8300_probe(HANDLE handle, DEVICEID devid)
{
	u16 ver;
	bbm_word_read(handle, devid, BBM_CHIP_ID, &ver);

	return (ver == 0x8300) ? BBM_OK : BBM_NOK;
}

s32 fc8300_set_core_clk(HANDLE handle, DEVICEID devid,
		enum BROADCAST_TYPE broadcast, u32 freq)
{
	u32 input_freq, new_pll_clk;
	u32 pre_sel, post_sel = 0, multi = 0, div_sel;
	u32 current_clk, new_clk, pll_set;
	u32 multi_3, multi_2, multi_1;
	s32 res = BBM_OK;

	current_clk = fc8300_get_current_clk(handle, devid);
	new_clk = fc8300_get_core_clk(handle, devid, broadcast, freq);

	if (new_clk == current_clk)
		return BBM_OK;

	if (BBM_XTAL_FREQ <= 10000)
		pre_sel = 0;
	else if (BBM_XTAL_FREQ <= 20000)
		pre_sel = 1;
	else if (BBM_XTAL_FREQ <= 40000)
		pre_sel = 2;
	else
		pre_sel = 3;

	input_freq = BBM_XTAL_FREQ >> pre_sel;

	multi_3 = (new_clk << 3) / input_freq;
	multi_2 = (new_clk << 2) / input_freq;
	multi_1 = (new_clk << 1) / input_freq;

	if (multi_3 >= 10 && multi_3 <= 40) {
		post_sel = 3;
		multi = multi_3;
	} else if (multi_2 >= 10 && multi_2 <= 40) {
		post_sel = 2;
		multi = multi_2;
	} else if (multi_1 >= 10 && multi_1 <= 40) {
		post_sel = 1;
		multi = multi_1;
	} else { /* post_sel == 0 */
		multi = new_clk / input_freq;
	}

	new_pll_clk = (input_freq * multi) >> post_sel;

	if (new_clk != new_pll_clk)
		return BBM_NOK;

	div_sel = multi - 2;

	bbm_byte_write(handle, devid, BBM_PLL_SEL, 0x00);
	bbm_byte_write(handle, devid, BBM_PLL1_RESET, 0x01);
	pll_set = (div_sel << 8) | (post_sel << 4) | pre_sel;
	bbm_word_write(handle, devid, BBM_PLL1_PRE_POST_SELECTION, pll_set);
	bbm_byte_write(handle, devid, BBM_PLL1_RESET, 0x00);
	msWait(1);
	bbm_byte_write(handle, devid, BBM_PLL_SEL, 0x01);

	switch (broadcast) {
	case ISDBT_1SEG:
	case ISDBTMM_1SEG:
	case ISDBTSB_1SEG:
	case ISDBT_CATV_1SEG:
		res |= fc8300_set_cal_front_1seg(handle, devid, new_clk);
		break;
	case ISDBTSB_3SEG:
		res |= fc8300_set_cal_front_3seg(handle, devid, new_clk);
		break;
	case ISDBT_13SEG:
	case ISDBTMM_13SEG:
	case ISDBT_CATV_13SEG:
		res |= fc8300_set_cal_front_13seg(handle, devid, new_clk);
		break;
	}

	switch (broadcast) {
	case ISDBT_1SEG:
	case ISDBT_CATV_1SEG:
		res |= fc8300_set_acif_b31_1seg(handle, devid, new_clk);
		break;
	case ISDBTMM_1SEG:
	case ISDBTSB_1SEG:
		res |= fc8300_set_acif_1seg(handle, devid, new_clk);
		break;
	case ISDBTSB_3SEG:
		res |= fc8300_set_acif_3seg(handle, devid, new_clk);
		break;
	case ISDBT_13SEG:
	case ISDBTMM_13SEG:
	case ISDBT_CATV_13SEG:
		res |= fc8300_set_acif_13seg(handle, devid, new_clk);
		break;
	}

	return res;
}

s32 fc8300_init(HANDLE handle, DEVICEID devid)
{
#if defined(BBM_4_DIVERSITY) /* Don't change following lines */
	bbm_byte_write(handle, DIV_BROADCAST, BBM_XTAL_OUTBUF_EN, 0x01);
	msWait(1);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_XTAL_OUTBUF_EN, 0x01);
	msWait(1);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_XTAL_OUTBUF_EN, 0x01);
	msWait(1);
#ifdef BBM_I2C_TSIF
#ifdef BBM_TS_204
	bbm_byte_write(handle, DIV_MASTER, BBM_TS_SEL, 0xc0);
#else
	bbm_byte_write(handle, DIV_MASTER, BBM_TS_SEL, 0x80);
#endif /* #ifdef BBM_TS_204 */
#endif /* #ifdef BBM_I2C_TSIF */
#else
#ifdef BBM_I2C_TSIF
#ifdef BBM_TS_204
	bbm_byte_write(handle, DIV_MASTER, BBM_TS_SEL, 0xc0);
#else
	bbm_byte_write(handle, DIV_MASTER, BBM_TS_SEL, 0x80);
#endif /* #ifdef BBM_TS_204 */
#endif /* #ifdef BBM_I2C_TSIF */
#if defined(BBM_2_DIVERSITY)
	bbm_byte_write(handle, DIV_BROADCAST, BBM_XTAL_OUTBUF_EN, 0x01);
	msWait(1);
#endif /* #ifdef BBM_2_DIVERSITY */
#endif

#if (BBM_XTAL_FREQ < 30000)
	bbm_byte_write(handle, DIV_BROADCAST, BBM_FUSELOAD, 0x03);
#else
	bbm_byte_write(handle, DIV_BROADCAST, BBM_FUSELOAD, 0x07);
#endif
	msWait(1);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_FUSELOAD, 0x00);

	bbm_byte_write(handle, DIV_BROADCAST, BBM_PLL1_ENABLE, 0x01);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_PLL1_PD, 0x00);

	fc8300_set_default_core_clk(handle, DIV_BROADCAST);
#ifdef BBM_I2C_TSIF
	fc8300_set_tsif_clk(handle, DIV_MASTER);
#endif

#ifdef BBM_2_DIVERSITY
	bbm_byte_write(handle, DIV_MASTER, BBM_DIVERSITY_DEVICE_SEL, 0x01);
	bbm_byte_write(handle, DIV_MASTER, BBM_DIVERSITY_MODE, 0x00);
	bbm_byte_write(handle, DIV_SLAVE0, BBM_DIVERSITY_DEVICE_SEL, 0x02);
	bbm_byte_write(handle, DIV_SLAVE0, BBM_DIVERSITY_MODE, 0x01);
	bbm_byte_write(handle, DIV_SLAVE0, BBM_RESYNC_ENABLE, 0x8f);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_DIVERSITY_EN, 0x21);

	bbm_byte_write(handle, DIV_SLAVE0, BBM_RESTART_BY_TS_EN, 0x00);
#endif /* BBM_2_DIVERSITY */

#ifdef BBM_4_DIVERSITY /* I2C+TSIF or I2C+SPI */
	bbm_byte_write(handle, DIV_MASTER, BBM_DIVERSITY_DEVICE_SEL, 0x01);
	bbm_byte_write(handle, DIV_MASTER, BBM_DIVERSITY_MODE, 0x00);

	/* end slave */
	bbm_byte_write(handle, DIV_SLAVE0, BBM_DIVERSITY_DEVICE_SEL, 0x02);
	bbm_byte_write(handle, DIV_SLAVE0, BBM_DIVERSITY_MODE, 0x03);
	bbm_byte_write(handle, DIV_SLAVE0, BBM_RESYNC_ENABLE, 0x8f);

	/* 2nd slave */
	bbm_byte_write(handle, DIV_SLAVE1, BBM_DIVERSITY_DEVICE_SEL, 0x03);
	bbm_byte_write(handle, DIV_SLAVE1, BBM_DIVERSITY_MODE, 0x02);
	bbm_byte_write(handle, DIV_SLAVE1, BBM_RESYNC_ENABLE, 0x8f);
	bbm_word_write(handle, DIV_SLAVE1, BBM_D_SYNC_TIME_OUT_TH, 0x0100);

	/* 1st slave */
	bbm_byte_write(handle, DIV_SLAVE2, BBM_DIVERSITY_DEVICE_SEL, 0x03);
	bbm_byte_write(handle, DIV_SLAVE2, BBM_DIVERSITY_MODE, 0x01);
	bbm_byte_write(handle, DIV_SLAVE2, BBM_RESYNC_ENABLE, 0x8f);
	bbm_word_write(handle, DIV_SLAVE2, BBM_D_SYNC_TIME_OUT_TH, 0x0200);

	bbm_byte_write(handle, DIV_BROADCAST, BBM_DIVERSITY_EN, 0x41);

	bbm_byte_write(handle, DIV_SLAVE0, BBM_RESTART_BY_TS_EN, 0x00);
	bbm_byte_write(handle, DIV_SLAVE1, BBM_RESTART_BY_TS_EN, 0x00);
	bbm_byte_write(handle, DIV_SLAVE2, BBM_RESTART_BY_TS_EN, 0x00);
#endif /* BBM_4_DIVERSITY */

	fc8300_reset(handle, DIV_BROADCAST);

	bbm_byte_write(handle, DIV_BROADCAST, BBM_ADC_PWRDN, 0x00);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_ADC_RST, 0x00);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_ADC_BIAS, 0x06);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_BB2RF_RFEN, 0x01);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_RF_RST, 0x00);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_RF_POWER_SAVE, 0x00);
	bbm_long_write(handle, DIV_BROADCAST, BBM_MEMORY_RWM0, 0x07777777);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_IQC_EN, 0x71);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_CSF_GAIN_MAX, 0x0a);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_ADC_CTRL, 0x27);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_PSAT_ON_REF_1SEG_QPSK, 0x16);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_PSAT_ON_REF_1SEG_16QAM, 0x16);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_HOLD_RST_EN, 0x06);
	bbm_word_write(handle, DIV_BROADCAST, BBM_REF_DELAY_POST, 0x1900);
	bbm_word_write(handle, DIV_BROADCAST, BBM_REF_AMP, 0x03e0);
	bbm_word_write(handle, DIV_BROADCAST, BBM_AD_GAIN_PERIOD, 0x003f);
	bbm_word_write(handle, DIV_BROADCAST, BBM_FD_RD_LATENCY_1SEG, 0x0620);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_DC_EST_EN, 0x00);

	bbm_byte_write(handle, DIV_BROADCAST, BBM_CFTSCFG_CACPGPOWTH_13SEG,
			0x20);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_CFTSCFG_CIRPGPOWTH_13SEG,
			0x10);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_CFTSCFG_CIRMRGPOWTH_13SEG,
			0x10);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_IFTSCFG_HDDONLOCKCNT, 0x04);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_IFTSCFG_HDDOFFLOCKCNT, 0x20);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_CIR_MARGIN_22, 0x3e);
	bbm_word_write(handle, DIV_BROADCAST, BBM_CIR_THR_22, 0x0080);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_FAIP_MTD_SR_SHIFT_VALUE,
			0x0c);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_MSNR_1D_SWT_EN, 0x00);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_IFTSCFG_ISIC_ENMFDLIMIT,
			0x01);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_FAIP_MTD_SR_SHIFT_TH, 0x40);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_CID_THRESH_13SEG, 0x38);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_CFTSCFG_ORDERFMDISTTH_13SEG,
			0x01);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_CFTSCFG_CACPGDISTTH_13SEG,
			0x32);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_CFTSCFG_CIRPGDISTTH_13SEG,
			0x3f);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_CFTSCFG_CIRMRGDISTTH_13SEG,
			0x3f);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_IFTSCFG_HDDEN, 0x01);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_CFTSCFG_CIRPGDISTMAX_13SEG,
			0xff);
	bbm_byte_write(handle, DIV_BROADCAST, BBM_CFTSCFG_ORDERMDSDISTTH_13SEG,
			0xff);

#if !defined(BBM_2_DIVERSITY) && !defined(BBM_4_DIVERSITY)
	bbm_byte_write(handle, DIV_MASTER, BBM_FD_OUT_MODE, 0x02);
	bbm_byte_write(handle, DIV_MASTER, BBM_DIV_START_MODE, 0x16);
#endif

	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_TS0_START, TS0_BUF_START);
	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_TS0_END, TS0_BUF_END);
	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_TS0_THR, TS0_BUF_THR);

	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_AC_A_START,
						AC_A_BUF_START);
	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_AC_A_END, AC_A_BUF_END);
	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_AC_A_THR, AC_A_BUF_THR);

	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_AC_B_START,
						AC_B_BUF_START);
	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_AC_B_END, AC_B_BUF_END);
	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_AC_B_THR, AC_B_BUF_THR);

	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_AC_C_START,
						AC_C_BUF_START);
	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_AC_C_END, AC_C_BUF_END);
	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_AC_C_THR, AC_C_BUF_THR);

	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_AC_D_START,
						AC_D_BUF_START);
	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_AC_D_END, AC_D_BUF_END);
	bbm_word_write(handle, DIV_BROADCAST, BBM_BUF_AC_D_THR, AC_D_BUF_THR);

#ifdef BBM_I2C_SPI
	bbm_byte_write(handle, DIV_MASTER, BBM_BUF_SPIOUT, 0x10);
#endif

#ifdef BBM_I2C_PARALLEL_TSIF
	bbm_byte_write(handle, DIV_MASTER, BBM_TS_CTRL, 0x8e);
#endif

#ifdef BBM_AUX_INT
	bbm_byte_write(handle, DIV_MASTER, BBM_SYS_MD_INT_EN, 0x7f);
	bbm_byte_write(handle, DIV_MASTER, BBM_AUX_INT_EN, 0xff);
	bbm_byte_write(handle, DIV_MASTER, BBM_FEC_INT_EN, 0x07);
	bbm_byte_write(handle, DIV_MASTER, BBM_BER_AUTO_UP, 0xff);
	bbm_byte_write(handle, DIV_MASTER, BBM_OSS_CFG_EN, 0x01);
#endif

#ifdef BBM_NULL_PID_FILTER
	bbm_byte_write(handle, DIV_MASTER, BBM_NULL_PID_FILTERING, 0x01);
#endif

#ifdef BBM_FAIL_FRAME
	bbm_byte_write(handle, DIV_MASTER, BBM_FAIL_FRAME_TX, 0x07);
#endif

#ifdef BBM_TS_204
	bbm_byte_write(handle, DIV_MASTER, BBM_FEC_CTRL_A, 0x03);
	bbm_byte_write(handle, DIV_MASTER, BBM_FEC_CTRL_B, 0x03);
	bbm_byte_write(handle, DIV_MASTER, BBM_FEC_CTRL_C, 0x03);
	bbm_byte_write(handle, DIV_MASTER, BBM_FEC_CTRL, 0x03);
#endif

#ifdef BBM_INT_LOW_ACTIVE
	bbm_byte_write(handle, DIV_BROADCAST, BBM_INT_POLAR_SEL, 0x00);
#endif

#ifdef BBM_SPI_30M
	bbm_byte_write(handle, DIV_BROADCAST, BBM_MD_MISO, 0x1f);
#endif

#ifdef BBM_DESCRAMBLER
	bbm_byte_write(handle, DIV_BROADCAST, BBM_BCAS_ENABLE, 0x01);
#else
	bbm_byte_write(handle, DIV_BROADCAST, BBM_BCAS_ENABLE, 0x00);
#endif

	bbm_byte_write(handle, DIV_MASTER, BBM_INT_AUTO_CLEAR, 0x01);
	bbm_byte_write(handle, DIV_MASTER, BBM_BUF_ENABLE, 0x01);
	bbm_byte_write(handle, DIV_MASTER, BBM_BUF_INT_ENABLE, 0x01);

	bbm_byte_write(handle, DIV_MASTER, BBM_INT_MASK, 0x01);
	bbm_byte_write(handle, DIV_MASTER, BBM_INT_STS_EN, 0x01);

	return BBM_OK;
}

s32 fc8300_deinit(HANDLE handle, DEVICEID devid)
{
#ifdef BBM_I2C_TSIF
	bbm_byte_write(handle, DIV_MASTER, BBM_TS_SEL, 0x00);
	msWait(24);
#endif

	bbm_byte_write(handle, devid, BBM_SW_RESET, 0x00);

	return BBM_OK;
}

#if defined(BBM_2_DIVERSITY) || defined(BBM_4_DIVERSITY)
s32 fc8300_scan_status(HANDLE handle, DEVICEID devid)
{
	u32 ifagc_timeout       = 70;
	u32 ofdm_timeout        = 160;
	u32 ffs_lock_timeout    = 100;
	u32 cfs_timeout         = 120;
	u32 tmcc_timeout        = 1050;
	u32 ts_err_free_timeout = 0;
	u32 data                = 0;
	u8  a, run;
	u32 i;
#ifdef BBM_2_DIVERSITY
	u8 done = 0x03;
#else
	u8 done = 0x0f;
#endif

	for (i = 0; i < ifagc_timeout; i++) {
		bbm_byte_read(handle, DIV_MASTER, 0x3025, &a);

		if (a & 0x01)
			break;

		bbm_byte_read(handle, DIV_SLAVE0, 0x3025, &a);

		if (a & 0x01)
			break;

#ifdef BBM_4_DIVERSITY
		bbm_byte_read(handle, DIV_SLAVE1, 0x3025, &a);

		if (a & 0x01)
			break;

		bbm_byte_read(handle, DIV_SLAVE2, 0x3025, &a);

		if (a & 0x01)
			break;
#endif

		msWait(SCAN_CHK_PERIOD);
	}

	if (i == ifagc_timeout)
		return BBM_NOK;

	for (run = done; i < ofdm_timeout; i++) {
		if (run & 0x01) {
			bbm_byte_read(handle, DIV_MASTER, 0x3025, &a);

			if (a & 0x08) {
				if (a & 0x04)
					break;

				run -= 0x01; /* master done */
			}
		}

		if (run & 0x02) {
			bbm_byte_read(handle, DIV_SLAVE0, 0x3025, &a);

			if (a & 0x08) {
				if (a & 0x04)
					break;

				run -= 0x02; /* slave0 done */
			}
		}

#ifdef BBM_4_DIVERSITY
		if (run & 0x04) {
			bbm_byte_read(handle, DIV_SLAVE1, 0x3025, &a);

			if (a & 0x08) {
				if (a & 0x04)
					break;

				run -= 0x04; /* slave1 done */
			}
		}

		if (run & 0x08) {
			bbm_byte_read(handle, DIV_SLAVE2, 0x3025, &a);

			if (a & 0x08) {
				if (a & 0x04)
					break;

				run -= 0x08; /* slave2 done */
			}
		}
#endif

		if (run == 0)
			return BBM_NOK;

		msWait(SCAN_CHK_PERIOD);
	}

	if (i == ofdm_timeout)
		return BBM_NOK;

	for (; i < ffs_lock_timeout; i++) {
		bbm_byte_read(handle, DIV_MASTER, 0x3026, &a);

		if ((a & 0x11) == 0x11)
			break;

		bbm_byte_read(handle, DIV_SLAVE0, 0x3026, &a);

		if ((a & 0x11) == 0x11)
			break;

#ifdef BBM_4_DIVERSITY
		bbm_byte_read(handle, DIV_SLAVE1, 0x3026, &a);

		if ((a & 0x11) == 0x11)
			break;

		bbm_byte_read(handle, DIV_SLAVE2, 0x3026, &a);

		if ((a & 0x11) == 0x11)
			break;
#endif

		msWait(SCAN_CHK_PERIOD);
	}

	if (i == ffs_lock_timeout)
		return BBM_NOK;

	for (i = 0, run = done; i < cfs_timeout; i++) {
		if (run & 0x01) {
			bbm_byte_read(handle, DIV_MASTER, 0x3025, &a);

			if (a & 0x40) {
				bbm_byte_read(handle, DIV_MASTER, 0x2023, &a);

				if ((a & 0x01) == 0x00)
					break;

				run -= 0x01; /* master done */
			}
		}

		if (run & 0x02) {
			bbm_byte_read(handle, DIV_SLAVE0, 0x3025, &a);

			if (a & 0x40) {
				bbm_byte_read(handle, DIV_SLAVE0, 0x2023, &a);

				if ((a & 0x01) == 0x00)
					break;

				run -= 0x02; /* slave0 done */
			}
		}

#ifdef BBM_4_DIVERSITY
		if (run & 0x04) {
			bbm_byte_read(handle, DIV_SLAVE1, 0x3025, &a);

			if (a & 0x40) {
				bbm_byte_read(handle, DIV_SLAVE1, 0x2023, &a);

				if ((a & 0x01) == 0x00)
					break;

				run -= 0x04; /* slave1 done */
			}
		}

		if (run & 0x08) {
			bbm_byte_read(handle, DIV_SLAVE2, 0x3025, &a);

			if (a & 0x40) {
				bbm_byte_read(handle, DIV_SLAVE2, 0x2023, &a);

				if ((a & 0x01) == 0x00)
					break;

				run -= 0x08; /* slave2 done */
			}
		}
#endif

		if (run == 0)
			return BBM_NOK;

		msWait(SCAN_CHK_PERIOD);
	}

	if (i == cfs_timeout)
		return BBM_NOK;

	for (i = 0; i < tmcc_timeout; i++) {
		bbm_byte_read(handle, DIV_MASTER, 0x3026, &a);

		if (a & 0x02) {
			bbm_long_read(handle, DIV_MASTER, 0x4113, &data);
			break;
		}

		bbm_byte_read(handle, DIV_SLAVE0, 0x3026, &a);

		if (a & 0x02) {
			bbm_long_read(handle, DIV_SLAVE0, 0x4113, &data);
			break;
		}

#ifdef BBM_4_DIVERSITY
		bbm_byte_read(handle, DIV_SLAVE1, 0x3026, &a);

		if (a & 0x02) {
			bbm_long_read(handle, DIV_SLAVE1, 0x4113, &data);
			break;
		}

		bbm_byte_read(handle, DIV_SLAVE2, 0x3026, &a);

		if (a & 0x02) {
			bbm_long_read(handle, DIV_SLAVE2, 0x4113, &data);
			break;
		}
#endif

		msWait(SCAN_CHK_PERIOD);
	}

	if (i == tmcc_timeout)
		return BBM_NOK;

	ts_err_free_timeout = 950;

	switch (broadcast_type) {
	case ISDBT_1SEG:
		if ((data & 0x00000008) == 0x00000000)
			return BBM_NOK;

		if ((data & 0x00001c70) == 0x00001840)
			ts_err_free_timeout = 700;

		break;
	case ISDBTMM_1SEG:
		if ((data & 0x00001c70) == 0x00001820)
			ts_err_free_timeout = 700;

		break;
	case ISDBTSB_1SEG:
		break;
	case ISDBTSB_3SEG:
		break;
	case ISDBT_13SEG:
		if ((data & 0x3f8e1c78) == 0x0d0c1848)
			ts_err_free_timeout = 700;

		break;
	case ISDBTMM_13SEG:
		if ((data & 0x3f8e1c78) == 0x0f041828)
			ts_err_free_timeout = 700;

		break;
	case ISDBT_CATV_13SEG:
		if ((data & 0x3f8e1c78) == 0x0d0c1848)
			ts_err_free_timeout = 700;

		break;
	case ISDBT_CATV_1SEG:
		if ((data & 0x00000008) == 0x00000000)
			return BBM_NOK;

		if ((data & 0x00001c70) == 0x00001840)
			ts_err_free_timeout = 700;

		break;
	default:
		break;
	}

	for (i = 0; i < ts_err_free_timeout; i++) {
		bbm_byte_read(handle, DIV_MASTER, 0x50c5, &a);

		if (a)
			break;

		msWait(SCAN_CHK_PERIOD);
	}

	if (i == ts_err_free_timeout)
		return BBM_NOK;

	return BBM_OK;
}
#else /* SINGLE */
s32 fc8300_scan_status(HANDLE handle, DEVICEID devid)
{
	u32 ifagc_timeout       = 70;
	u32 ofdm_timeout        = 160;
	u32 ffs_lock_timeout    = 100;
	u32 cfs_timeout         = 120;
	u32 tmcc_timeout        = 1050;
	u32 ts_err_free_timeout = 0;
	u32 data                = 0;
	u8  a;
	u32 i;

	for (i = 0; i < ifagc_timeout; i++) {
		bbm_byte_read(handle, DIV_MASTER, 0x3025, &a);

		if (a & 0x01)
			break;

		msWait(SCAN_CHK_PERIOD);
	}

	if (i == ifagc_timeout)
		return BBM_NOK;

	for (; i < ofdm_timeout; i++) {
		bbm_byte_read(handle, DIV_MASTER, 0x3025, &a);

		if (a & 0x08)
			break;

		msWait(SCAN_CHK_PERIOD);
	}

	if (i == ofdm_timeout)
		return BBM_NOK;

	if (0 == (a & 0x04))
		return BBM_NOK;

	for (; i < ffs_lock_timeout; i++) {
		bbm_byte_read(handle, DIV_MASTER, 0x3026, &a);

		if ((a & 0x11) == 0x11)
			break;

		msWait(SCAN_CHK_PERIOD);
	}

	if (i == ffs_lock_timeout)
		return BBM_NOK;

	for (i = 0; i < cfs_timeout; i++) {
		bbm_byte_read(handle, DIV_MASTER, 0x3025, &a);

		if (a & 0x40)
			break;

		msWait(SCAN_CHK_PERIOD);
	}

	if (i == cfs_timeout)
		return BBM_NOK;

	bbm_byte_read(handle, DIV_MASTER, 0x2023, &a);

	if (a & 0x01)
		return BBM_NOK;

	for (i = 0; i < tmcc_timeout; i++) {
		bbm_byte_read(handle, DIV_MASTER, 0x3026, &a);

		if (a & 0x02)
			break;

		msWait(SCAN_CHK_PERIOD);
	}

	if (i == tmcc_timeout)
		return BBM_NOK;

	ts_err_free_timeout = 950;

	switch (broadcast_type) {
	case ISDBT_1SEG:
		bbm_word_read(handle, DIV_MASTER, 0x4113, (u16 *) &data);

		if ((data & 0x0008) == 0x0000)
			return BBM_NOK;

		if ((data & 0x1c70) == 0x1840)
			ts_err_free_timeout = 700;

		break;
	case ISDBTMM_1SEG:
		bbm_word_read(handle, DIV_MASTER, 0x4113, (u16 *) &data);

		if ((data & 0x1c70) == 0x1820)
			ts_err_free_timeout = 700;

		break;
	case ISDBTSB_1SEG:
		break;
	case ISDBTSB_3SEG:
		break;
	case ISDBT_13SEG:
		bbm_long_read(handle, DIV_MASTER, 0x4113, &data);

		if ((data & 0x3f8e1c78) == 0x0d0c1848)
			ts_err_free_timeout = 700;

		break;
	case ISDBTMM_13SEG:
		bbm_long_read(handle, DIV_MASTER, 0x4113, &data);

		if ((data & 0x3f8e1c78) == 0x0f041828)
			ts_err_free_timeout = 700;

		break;
	case ISDBT_CATV_13SEG:
		bbm_long_read(handle, DIV_MASTER, 0x4113, &data);

		if ((data & 0x3f8e1c78) == 0x0d0c1848)
			ts_err_free_timeout = 700;

		break;
	case ISDBT_CATV_1SEG:
		bbm_word_read(handle, DIV_MASTER, 0x4113, (u16 *) &data);

		if ((data & 0x0008) == 0x0000)
			return BBM_NOK;

		if ((data & 0x1c70) == 0x1840)
			ts_err_free_timeout = 700;

		break;
	default:
		break;
	}

	for (i = 0; i < ts_err_free_timeout; i++) {
		bbm_byte_read(handle, DIV_MASTER, 0x50c5, &a);

		if (a)
			break;

		msWait(SCAN_CHK_PERIOD);
	}

	if (i == ts_err_free_timeout)
		return BBM_NOK;

	return BBM_OK;
}
#endif /* #if defined(BBM_2_DIVERSITY) || defined(BBM_4_DIVERSITY) */

s32 fc8300_set_broadcast_mode(HANDLE handle, DEVICEID devid,
		enum BROADCAST_TYPE broadcast)
{
	s32 res = BBM_OK;
	u32 clk = fc8300_set_default_core_clk(handle, devid);

	broadcast_type = broadcast;

	switch (broadcast) {
	case ISDBT_1SEG:
	case ISDBTMM_1SEG:
	case ISDBTSB_1SEG:
	case ISDBT_CATV_1SEG:
		res |= fc8300_set_cal_front_1seg(handle, devid, clk);
		break;
	case ISDBTSB_3SEG:
		res |= fc8300_set_cal_front_3seg(handle, devid, clk);
		break;
	case ISDBT_13SEG:
	case ISDBTMM_13SEG:
	case ISDBT_CATV_13SEG:
		res |= fc8300_set_cal_front_13seg(handle, devid, clk);
		break;
	}

	switch (broadcast) {
	case ISDBT_1SEG:
	case ISDBT_CATV_1SEG:
		res |= fc8300_set_acif_b31_1seg(handle, devid, clk);
		break;
	case ISDBTMM_1SEG:
	case ISDBTSB_1SEG:
		res |= fc8300_set_acif_1seg(handle, devid, clk);
		break;
	case ISDBTSB_3SEG:
		res |= fc8300_set_acif_3seg(handle, devid, clk);
		break;
	case ISDBT_13SEG:
	case ISDBTMM_13SEG:
	case ISDBT_CATV_13SEG:
		res |= fc8300_set_acif_13seg(handle, devid, clk);
		break;
	}

	/* system mode */
	switch (broadcast) {
	case ISDBT_1SEG:
	case ISDBT_CATV_1SEG:
		bbm_byte_write(handle, devid, BBM_CLK_CTRL, 0x05);
		bbm_byte_write(handle, devid, BBM_SYSTEM_MODE, 0x01);
		break;
	case ISDBT_13SEG:
	case ISDBT_CATV_13SEG:
		bbm_byte_write(handle, devid, BBM_CLK_CTRL, 0x07);
		bbm_byte_write(handle, devid, BBM_SYSTEM_MODE, 0x00);
		break;
	case ISDBTMM_1SEG:
		bbm_byte_write(handle, devid, BBM_CLK_CTRL, 0x05);
		bbm_byte_write(handle, devid, BBM_SYSTEM_MODE, 0x02);
		break;
	case ISDBTMM_13SEG:
		bbm_byte_write(handle, devid, BBM_CLK_CTRL, 0x07);
		bbm_byte_write(handle, devid, BBM_SYSTEM_MODE, 0x00);
		break;
	case ISDBTSB_1SEG:
		bbm_byte_write(handle, devid, BBM_CLK_CTRL, 0x05);
		bbm_byte_write(handle, devid, BBM_SYSTEM_MODE, 0x02);
		break;
	case ISDBTSB_3SEG:
		bbm_byte_write(handle, devid, BBM_CLK_CTRL, 0x06);
		bbm_byte_write(handle, devid, BBM_SYSTEM_MODE, 0x03);
		break;
	}

	/* pre-run */
	switch (broadcast) {
	case ISDBT_1SEG:
	case ISDBT_CATV_1SEG:
		bbm_long_write(handle, devid, BBM_MAN_PARTIAL_EN, 0x000c0101);
		bbm_long_write(handle, devid, BBM_MAN_LAYER_A_MOD_TYPE,
							0x01000301);
		bbm_long_write(handle, devid, BBM_MAN_LAYER_B_CODE_RATE,
							0x02030002);
		bbm_byte_write(handle, devid, BBM_MAN_LAYER_C_TI_LENGTH, 0x00);
		bbm_word_write(handle, devid, BBM_TDI_PRE_A, 0xc213);
		bbm_byte_write(handle, devid, BBM_TDI_PRE_C, 0x03);
		bbm_byte_write(handle, devid, BBM_FEC_LAYER, 0x01);
		break;
	case ISDBT_13SEG:
	case ISDBT_CATV_13SEG:
		bbm_long_write(handle, devid, BBM_MAN_PARTIAL_EN, 0x000C0101);
		bbm_long_write(handle, devid, BBM_MAN_LAYER_A_MOD_TYPE,
							0x01000301);
		bbm_long_write(handle, devid, BBM_MAN_LAYER_B_CODE_RATE,
							0x02030002);
		bbm_byte_write(handle, devid, BBM_MAN_LAYER_C_TI_LENGTH, 0x00);
		bbm_word_write(handle, devid, BBM_TDI_PRE_A, 0xC21B);
		bbm_byte_write(handle, devid, BBM_TDI_PRE_C, 0x03);
		bbm_byte_write(handle, devid, BBM_FEC_LAYER, 0x02);
		break;
	case ISDBTMM_1SEG:
		bbm_long_write(handle, devid, BBM_MAN_PARTIAL_EN, 0x000C0101);
		bbm_long_write(handle, devid, BBM_MAN_LAYER_A_MOD_TYPE,
							0x00000202);
		bbm_long_write(handle, devid, BBM_MAN_LAYER_B_CODE_RATE,
							0x03030000);
		bbm_byte_write(handle, devid, BBM_MAN_LAYER_C_TI_LENGTH, 0x00);
		bbm_word_write(handle, devid, BBM_TDI_PRE_A, 0xCB13);
		bbm_byte_write(handle, devid, BBM_TDI_PRE_C, 0x03);
		bbm_byte_write(handle, devid, BBM_FEC_LAYER, 0x01);
		break;
	case ISDBTMM_13SEG:
		bbm_long_write(handle, devid, BBM_MAN_PARTIAL_EN, 0x000C0101);
		bbm_long_write(handle, devid, BBM_MAN_LAYER_A_MOD_TYPE,
							0x00000202);
		bbm_long_write(handle, devid, BBM_MAN_LAYER_B_CODE_RATE,
							0x03030000);
		bbm_byte_write(handle, devid, BBM_MAN_LAYER_C_TI_LENGTH, 0x00);
		bbm_word_write(handle, devid, BBM_TDI_PRE_A, 0xCB1B);
		bbm_byte_write(handle, devid, BBM_TDI_PRE_C, 0x03);
		bbm_byte_write(handle, devid, BBM_FEC_LAYER, 0x02);
		break;
	case ISDBTSB_1SEG:
		bbm_long_write(handle, devid, BBM_MAN_PARTIAL_EN, 0x00020101);
		bbm_long_write(handle, devid, BBM_MAN_LAYER_A_MOD_TYPE,
							0x00000202);
		bbm_long_write(handle, devid, BBM_MAN_LAYER_B_CODE_RATE,
							0x03030000);
		bbm_byte_write(handle, devid, BBM_MAN_LAYER_C_TI_LENGTH, 0x00);
		bbm_word_write(handle, devid, BBM_TDI_PRE_A, 0x2313);
		bbm_byte_write(handle, devid, BBM_TDI_PRE_C, 0x03);
		bbm_byte_write(handle, devid, BBM_FEC_LAYER, 0x01);
		break;
	case ISDBTSB_3SEG:
		bbm_long_write(handle, devid, BBM_MAN_PARTIAL_EN, 0x00020101);
		bbm_long_write(handle, devid, BBM_MAN_LAYER_A_MOD_TYPE,
							0x00000202);
		bbm_long_write(handle, devid, BBM_MAN_LAYER_B_CODE_RATE,
							0x03030000);
		bbm_byte_write(handle, devid, BBM_MAN_LAYER_C_TI_LENGTH, 0x00);
		bbm_word_write(handle, devid, BBM_TDI_PRE_A, 0x2313);
		bbm_byte_write(handle, devid, BBM_TDI_PRE_C, 0x03);
		bbm_byte_write(handle, devid, BBM_FEC_LAYER, 0x02);
		break;
	}

	if (broadcast == ISDBT_13SEG || broadcast == ISDBTMM_13SEG ||
					broadcast == ISDBT_CATV_13SEG)
		bbm_byte_write(handle, devid, BBM_IIFOECFG_EARLYSTOP_THM, 0x18);
	else
		bbm_byte_write(handle, devid, BBM_IIFOECFG_EARLYSTOP_THM, 0x0e);

	switch (broadcast) {
	case ISDBT_1SEG:
	case ISDBTMM_1SEG:
	case ISDBTSB_1SEG:
	case ISDBT_CATV_1SEG:
		bbm_word_write(handle, devid, BBM_MSNR_FREQ_S_POW_MAN_VALUE3,
								0xb00f);
		break;
	case ISDBTSB_3SEG:
		bbm_word_write(handle, devid, BBM_MSNR_FREQ_S_POW_MAN_VALUE3,
								0x3030);
		break;
	case ISDBT_13SEG:
	case ISDBTMM_13SEG:
	case ISDBT_CATV_13SEG:
		bbm_word_write(handle, devid, BBM_MSNR_FREQ_S_POW_MAN_VALUE3,
								0x00c3);
		break;
	}

	switch (broadcast) {
	case ISDBTMM_1SEG:
	case ISDBTMM_13SEG:
		bbm_byte_write(handle, devid, BBM_ECHOC_EN, 0x00);
		break;
	default:
		bbm_byte_write(handle, devid, BBM_ECHOC_EN, 0x01);
		break;
	}

	switch (broadcast) {
	case ISDBT_CATV_13SEG:
	case ISDBT_CATV_1SEG:
		bbm_byte_write(handle, DIV_BROADCAST, BBM_PGA_GAIN_MAX, 0x00);
		break;
	default:
		bbm_byte_write(handle, DIV_BROADCAST, BBM_PGA_GAIN_MAX, 0x0c);
		break;
	}

	return res;
}

