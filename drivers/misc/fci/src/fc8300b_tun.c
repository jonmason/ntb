/*****************************************************************************
	Copyright(c) 2013 FCI Inc. All Rights Reserved

	File name : fc8300b_tun.c

	Description : source of FC8300B tuner driver

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
	0p5 -> 20140114
	0p6 -> 20140123
	0p7 -> 20140324
	0p8 -> 20160825
******************************************************************************/
#include "fci_types.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fc8300_regs.h"
#include "fc8300b_tun.h"
#include "fc8300b_tun_table.h"

#define FC8300B_FREQ_XTAL BBM_XTAL_FREQ

#define DRIVER_VERSION         0x08 /* Driver B1_V0p7 */
#define RFADC_COUNT            3
#define CAL_REPEAT             1

#define XTAL_FREQ_19200        0
#define XTAL_FREQ_24000        1
#define XTAL_FREQ_26000        2
#define XTAL_FREQ_27120        3
#define XTAL_FREQ_32000        4
#define XTAL_FREQ_37200        5
#define XTAL_FREQ_37400        6

#define FC8300B_FILTER_CHECK    1

#define FC8300B_XTAL_FREQ        BBM_XTAL_FREQ
#define FC8300B_BAND_WIDTH       BBM_BAND_WIDTH

static enum BROADCAST_TYPE broadcast_type = ISDBT_13SEG;

static u8 xtal_freq = 4;
static u8 catv_status;
static u8 tf_value[4] = {0, 0, 0, 0};

static u32 tf_offset[2][12] = {
	{470000, 485000, 497000, 515000, 533000, 581000, 605000, 635000,
		671000, 737000, 803000, 810000},
	{11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0}
};

s32 fc8300b_write(HANDLE handle, DEVICEID devid, u8 addr, u8 data)
{
	s32 res;

	res = tuner_i2c_write(handle, devid, addr, 1, &data, 1);

	return res;
}

s32 fc8300b_read(HANDLE handle, DEVICEID devid, u8 addr, u8 *data)
{
	s32 res;

	res = tuner_i2c_read(handle, devid, addr, 1, data, 1);

	return res;
}

/*s32 fc8300b_bb_write(HANDLE handle, DEVICEID devid, u16 addr, u8 data)
{
	s32 res;

	res = bbm_write(handle, devid, addr, data);

	return res;
}

s32 fc8300b_bb_read(HANDLE handle, DEVICEID devid, u16 addr, u8 *data)
{
	s32 res;

	res = bbm_read(handle, devid, addr, data);

	return res;
}*/

static u8 tf_cal(HANDLE handle, DEVICEID devid)
{
	u8 rfadc[RFADC_COUNT];
	u32 rfadc_avg = 0;

	u8 max_cb_code[CAL_REPEAT];
	u8 max_rfadc[CAL_REPEAT];
	u8 cal_temp = 0;

	u32 x, i, j;

	for (x = 0; x < CAL_REPEAT; x++) {
		max_cb_code[x] = 0;
		max_rfadc[x] = 0;

		for (i = 3; i < 12; i++) {
			rfadc[x] = 0;
			rfadc_avg = 0;

			cal_temp = (0x10 + i);

			fc8300b_write(handle, devid, 0x1c, cal_temp);

			for (j = 0; j < RFADC_COUNT; j++) {
				fc8300b_read(handle, devid, 0xf5, &rfadc[j]);
				rfadc_avg = rfadc_avg + rfadc[j];
			}

			rfadc_avg = rfadc_avg / RFADC_COUNT;

			if (max_rfadc[x] < rfadc_avg) {
				max_rfadc[x] = (u8) rfadc_avg;
				max_cb_code[x] = (0x10 + i);
			}
		}
	}

	return max_cb_code[0];
}

static u32 vco_cal(HANDLE handle, DEVICEID devid, u8 offset)
{
	u8 vco_table[4][4] = {
		{0xbb, 0xbb, 0xbb, 0xbb},
		{0xb3, 0xbb, 0xbb, 0xbb},
		{0x32, 0x43, 0x54, 0x65},
		{0x0f, 0x10, 0x21, 0x32}
	};

	u8 ib[4] = {0, 0, 0, 0};

	u8 i = 0;
	u8 j = 0;

	u8 read_temp = 0;

	if (offset == vco_table[3][0])
		offset = 1;
	else if (offset == vco_table[3][1])
		offset = 2;
	else if (offset == vco_table[3][2])
		offset = 3;

	for (j = offset; j < 4; j++) {
		for (i = 0; i < 4; i++)
			fc8300b_write(handle, devid, (0x71 + i),
					vco_table[i][j]);

		fc8300b_write(handle, devid, 0x5f, 0x40);

		fc8300b_write(handle, devid, 0x6d, 0x0f);
		fc8300b_write(handle, devid, 0x6e, 0x1f);
		fc8300b_write(handle, devid, 0x6f, 0x3f);

		fc8300b_write(handle, devid, 0x6a, 0x04);
		fc8300b_write(handle, devid, 0x6a, 0x14);
		fc8300b_write(handle, devid, 0x75, 0x30);

		for (i = 0; i < 10; i++) {
			msWait(1);
			fc8300b_read(handle, devid, 0x76, &read_temp);

			if (read_temp == 0x20)
				break;
		}

		for (i = 0; i < 4; i++) {
			fc8300b_write(handle, devid, 0x75, (0x34 + i));
			fc8300b_read(handle, devid, 0x76, &read_temp);
			ib[i] = read_temp;
		}

		read_temp = (ib[0] == 0) ? 0 :
			(ib[1] == 0) ? 0 :
			(ib[2] == 0) ? 0 :
			(ib[3] == 0) ? 0 : 1;

		if (read_temp != 0) {
			fc8300b_write(handle, devid, 0x5f, 0x00);
			return BBM_OK;
		}
	}

	fc8300b_write(handle, devid, 0x6a, 0x15);
	fc8300b_write(handle, devid, 0x70, 0x7f);

	return BBM_OK;
}

static s32 fc8300b_tuner_set_pll(HANDLE handle, DEVICEID devid, u32 freq,
							u32 offset)
{
	u8 data_0x51, data_0x57, lo_select;
	u32 f_diff, f_diff_shifted, n_val, k_val, f_vco;
	u32 f_vco_max = 6000000;
	u8 lo_divide_ratio[8] = {8, 12, 16, 24, 32, 48, 64, 96};
	u8 pre_shift_bits = 4;
	u8 lock_detect = 0;
	u8 lock_bank = 0;
	u8 lock_condition = 0x11;
	u8 i, j;

	for (lo_select = 0; lo_select < 7; lo_select++)
		if (f_vco_max < freq * lo_divide_ratio[lo_select + 1])
			break;

	f_vco = freq * lo_divide_ratio[lo_select];

	n_val =	f_vco / FC8300B_XTAL_FREQ;
	f_diff = f_vco - FC8300B_XTAL_FREQ * n_val;

	f_diff_shifted = f_diff << (20 - pre_shift_bits);

	k_val = f_diff_shifted / (FC8300B_XTAL_FREQ >> pre_shift_bits);
	k_val = k_val | 1;

	data_0x57 = ((n_val >> 3) & 0x20);
	data_0x57 += (k_val >> 16);

	data_0x51 = 0x00;

	data_0x51 += (lo_select * 2) + 1;

	for (j = 0; j < 4; j++) {
		fc8300b_write(handle, devid, 0x75, 0x31);
		fc8300b_write(handle, devid, 0x77, 0x00);
		fc8300b_write(handle, devid, 0x51, data_0x51);
		fc8300b_write(handle, devid, 0x57, data_0x57);
		fc8300b_write(handle, devid, 0x5a, (u8) n_val);
		fc8300b_write(handle, devid, 0x50, 0xe3);
		fc8300b_write(handle, devid, 0x50, 0xff);
		fc8300b_write(handle, devid, 0x58, (u8) ((k_val >> 8) & 0xff));
		fc8300b_write(handle, devid, 0x59, (u8) ((k_val) & 0xff));

		msWait(2);


		for (i = 1; i < 5; i++) {

			fc8300b_read(handle, devid, 0x64, &lock_detect);
			fc8300b_read(handle, devid, 0x65, &lock_bank);

			if (((lock_detect & lock_condition) == lock_condition)
							&& (lock_bank > 1)) {

				fc8300b_read(handle, devid, 0x76, &lock_bank);

				if (lock_bank < 62) {
					if (((lo_select % 2) != 0) || (f_vco >
								6200000)) {
						fc8300b_write(handle, devid,
								0x77, 0x02);
						fc8300b_write(handle, devid,
								0x50, 0xe3);
						fc8300b_write(handle, devid,
								0x50, 0xff);
					}
				}
				break;
			}

		}

		if ((lock_detect & lock_condition) != lock_condition) {
			fc8300b_read(handle, devid, 0x74, &lock_detect);
			vco_cal(handle, devid, lock_detect);
		} else {
			break;
		}
	}

	return BBM_OK;
}

static void fc8300b_1seg_init(HANDLE handle, DEVICEID devid, u16 filter_cal)
{
	fc8300b_write(handle, devid, 0x00, 0x00);
	fc8300b_write(handle, devid, 0x02, 0x01);
	fc8300b_write(handle, devid, 0x03, 0x04);
	fc8300b_write(handle, devid, 0x04, 0x10);

	fc8300b_write(handle, devid, 0x08, 0xe4);

	fc8300b_write(handle, devid, 0x13, 0x07);
	fc8300b_write(handle, devid, 0x19, 0x53);
	fc8300b_write(handle, devid, 0x1d, 0x10);
	fc8300b_write(handle, devid, 0x1f, 0x79);
	fc8300b_write(handle, devid, 0x20, 0x22);
	fc8300b_write(handle, devid, 0x21, 0x30);
	fc8300b_write(handle, devid, 0x24, 0x98);

	fc8300b_write(handle, devid, 0x3f, 0x01);
	fc8300b_write(handle, devid, 0x40, (0x44 + (filter_cal / 256)));
	fc8300b_write(handle, devid, 0x41, (filter_cal % 256));
	fc8300b_write(handle, devid, 0x3f, 0x00);

	fc8300b_write(handle, devid, 0x33, 0x84);
	fc8300b_write(handle, devid, 0x34, 0x32);
	fc8300b_write(handle, devid, 0x37, 0x65);
	fc8300b_write(handle, devid, 0x38, 0x55);
	fc8300b_write(handle, devid, 0x39, 0x05);
	fc8300b_write(handle, devid, 0x3e, 0xab);
	fc8300b_write(handle, devid, 0x3c, 0xaa);
	fc8300b_write(handle, devid, 0x3d, 0xaa);

	fc8300b_write(handle, devid, 0x4d, 0x01);

	fc8300b_write(handle, devid, 0x50, 0xff);
	fc8300b_write(handle, devid, 0x53, 0x1a);
	fc8300b_write(handle, devid, 0x54, 0x80);

	fc8300b_write(handle, devid, 0x70, 0x5f);
	fc8300b_write(handle, devid, 0x78, 0x31);

	fc8300b_write(handle, devid, 0x82, 0x88);
	fc8300b_write(handle, devid, 0x84, 0x14);
	fc8300b_write(handle, devid, 0x85, 0x0f);
	fc8300b_write(handle, devid, 0x86, 0x3f);
	fc8300b_write(handle, devid, 0x87, 0x18);
	fc8300b_write(handle, devid, 0x88, 0x0e);
	fc8300b_write(handle, devid, 0x89, 0x0b);
	fc8300b_write(handle, devid, 0x8a, 0x15);
	fc8300b_write(handle, devid, 0x8b, 0x0e);
	fc8300b_write(handle, devid, 0x8f, 0xb6);

	fc8300b_write(handle, devid, 0x90, 0xb2);
	fc8300b_write(handle, devid, 0x91, 0xf8);

	fc8300b_write(handle, devid, 0xb2, 0x00);
	fc8300b_write(handle, devid, 0xb3, 0x07);
	fc8300b_write(handle, devid, 0xb4, 0x00);
	fc8300b_write(handle, devid, 0xb5, 0x07);
	fc8300b_write(handle, devid, 0xb6, 0x00);
	fc8300b_write(handle, devid, 0xb7, 0x07);
	fc8300b_write(handle, devid, 0xb8, 0x00);
	fc8300b_write(handle, devid, 0xb9, 0x07);
	fc8300b_write(handle, devid, 0xba, 0x00);
	fc8300b_write(handle, devid, 0xbb, 0x07);
	fc8300b_write(handle, devid, 0xbc, 0x00);
	fc8300b_write(handle, devid, 0xbd, 0x0a);
	fc8300b_write(handle, devid, 0xbe, 0x00);
	fc8300b_write(handle, devid, 0xbf, 0x0a);

	fc8300b_write(handle, devid, 0xc0, 0x0a);
	fc8300b_write(handle, devid, 0xc1, 0x00);
	fc8300b_write(handle, devid, 0xc2, 0x13);
	fc8300b_write(handle, devid, 0xc3, 0x00);

	fc8300b_write(handle, devid, 0xd3, 0x00);
	fc8300b_write(handle, devid, 0xd4, 0x01);
	fc8300b_write(handle, devid, 0xd5, 0x05);
	fc8300b_write(handle, devid, 0xd6, 0x0d);
	fc8300b_write(handle, devid, 0xd7, 0x0e);
	fc8300b_write(handle, devid, 0xd8, 0x0f);
	fc8300b_write(handle, devid, 0xd9, 0x2f);
	fc8300b_write(handle, devid, 0xda, 0x4f);
	fc8300b_write(handle, devid, 0xdb, 0x6f);
	fc8300b_write(handle, devid, 0xdc, 0x77);
	fc8300b_write(handle, devid, 0xdd, 0x97);

	fc8300b_write(handle, devid, 0xe9, 0x79);
	fc8300b_write(handle, devid, 0xed, 0x0c);
	fc8300b_write(handle, devid, 0xef, 0xb7);

	fc8300b_write(handle, devid, 0xcd, 0x41);
	fc8300b_write(handle, devid, 0xce, 0x88);
	fc8300b_write(handle, devid, 0xcf, 0x03);

}

static void fc8300b_tmm_1seg_init(HANDLE handle, DEVICEID devid, u16 filter_cal)
{
	fc8300b_write(handle, devid, 0x00, 0x00);
	fc8300b_write(handle, devid, 0x02, 0x03);
	fc8300b_write(handle, devid, 0x03, 0x04);
	fc8300b_write(handle, devid, 0x04, 0x10);

	fc8300b_write(handle, devid, 0x08, 0xe4);

	fc8300b_write(handle, devid, 0x13, 0x07);
	fc8300b_write(handle, devid, 0x15, 0x46);
	fc8300b_write(handle, devid, 0x16, 0x42);
	fc8300b_write(handle, devid, 0x17, 0x42);
	fc8300b_write(handle, devid, 0x1a, 0x42);
	fc8300b_write(handle, devid, 0x1c, 0x00);
	fc8300b_write(handle, devid, 0x1d, 0x00);
	fc8300b_write(handle, devid, 0x1f, 0x79);

	fc8300b_write(handle, devid, 0x20, 0x21);
	fc8300b_write(handle, devid, 0x21, 0x10);
	fc8300b_write(handle, devid, 0x24, 0x98);

	fc8300b_write(handle, devid, 0x3f, 0x01);
	fc8300b_write(handle, devid, 0x40, (0x44 + (filter_cal / 256)));
	fc8300b_write(handle, devid, 0x41, (filter_cal % 256));
	fc8300b_write(handle, devid, 0x3f, 0x00);

	fc8300b_write(handle, devid, 0x33, 0x84);
	fc8300b_write(handle, devid, 0x34, 0x32);
	fc8300b_write(handle, devid, 0x37, 0x65);
	fc8300b_write(handle, devid, 0x38, 0x55);
	fc8300b_write(handle, devid, 0x39, 0x05);
	fc8300b_write(handle, devid, 0x3c, 0xff);
	fc8300b_write(handle, devid, 0x3d, 0xff);
	fc8300b_write(handle, devid, 0x3e, 0xab);

	fc8300b_write(handle, devid, 0x50, 0xff);

	fc8300b_write(handle, devid, 0x70, 0x5f);
	fc8300b_write(handle, devid, 0x78, 0x31);

	fc8300b_write(handle, devid, 0x82, 0x88);
	fc8300b_write(handle, devid, 0x84, 0x14);
	fc8300b_write(handle, devid, 0x85, 0x0f);
	fc8300b_write(handle, devid, 0x86, 0x3f);
	fc8300b_write(handle, devid, 0x87, 0x18);
	fc8300b_write(handle, devid, 0x88, 0x0e);
	fc8300b_write(handle, devid, 0x89, 0x0b);
	fc8300b_write(handle, devid, 0x8a, 0x15);
	fc8300b_write(handle, devid, 0x8b, 0x0e);
	fc8300b_write(handle, devid, 0x8f, 0xb6);

	fc8300b_write(handle, devid, 0x90, 0xb2);
	fc8300b_write(handle, devid, 0x91, 0xf8);

	fc8300b_write(handle, devid, 0xb2, 0x00);
	fc8300b_write(handle, devid, 0xb3, 0x07);
	fc8300b_write(handle, devid, 0xb4, 0x00);
	fc8300b_write(handle, devid, 0xb5, 0x07);
	fc8300b_write(handle, devid, 0xb6, 0x00);
	fc8300b_write(handle, devid, 0xb7, 0x07);
	fc8300b_write(handle, devid, 0xb8, 0x00);
	fc8300b_write(handle, devid, 0xb9, 0x07);
	fc8300b_write(handle, devid, 0xba, 0x00);
	fc8300b_write(handle, devid, 0xbb, 0x07);
	fc8300b_write(handle, devid, 0xbc, 0x00);
	fc8300b_write(handle, devid, 0xbd, 0x07);
	fc8300b_write(handle, devid, 0xbe, 0x00);
	fc8300b_write(handle, devid, 0xbf, 0x0a);

	fc8300b_write(handle, devid, 0xc0, 0x0a);
	fc8300b_write(handle, devid, 0xc1, 0x00);
	fc8300b_write(handle, devid, 0xc2, 0x13);
	fc8300b_write(handle, devid, 0xc3, 0x00);
	fc8300b_write(handle, devid, 0xcf, 0x03);

	fc8300b_write(handle, devid, 0xd3, 0x00);
	fc8300b_write(handle, devid, 0xd4, 0x01);
	fc8300b_write(handle, devid, 0xd5, 0x05);
	fc8300b_write(handle, devid, 0xd6, 0x0d);
	fc8300b_write(handle, devid, 0xd7, 0x0e);
	fc8300b_write(handle, devid, 0xd8, 0x0f);
	fc8300b_write(handle, devid, 0xd9, 0x2f);
	fc8300b_write(handle, devid, 0xda, 0x4f);
	fc8300b_write(handle, devid, 0xdb, 0x6f);
	fc8300b_write(handle, devid, 0xdc, 0x77);
	fc8300b_write(handle, devid, 0xdd, 0x97);

	fc8300b_write(handle, devid, 0xe9, 0x79);
	fc8300b_write(handle, devid, 0xed, 0x0c);
	fc8300b_write(handle, devid, 0xef, 0xb7);
}

static void fc8300b_tsb_1seg_init(HANDLE handle, DEVICEID devid, u16 filter_cal)
{
	fc8300b_write(handle, devid, 0x00, 0x00);
	fc8300b_write(handle, devid, 0x02, 0x05);
	fc8300b_write(handle, devid, 0x03, 0x04);
	fc8300b_write(handle, devid, 0x04, 0x10);

	fc8300b_write(handle, devid, 0x08, 0x21);

	fc8300b_write(handle, devid, 0x13, 0x07);
	fc8300b_write(handle, devid, 0x15, 0x46);
	fc8300b_write(handle, devid, 0x16, 0x42);
	fc8300b_write(handle, devid, 0x17, 0x42);
	fc8300b_write(handle, devid, 0x1a, 0x42);
	fc8300b_write(handle, devid, 0x1b, 0x42);
	fc8300b_write(handle, devid, 0x1d, 0x00);
	fc8300b_write(handle, devid, 0x1f, 0x79);
	fc8300b_write(handle, devid, 0x20, 0x21);
	fc8300b_write(handle, devid, 0x21, 0x10);
	fc8300b_write(handle, devid, 0x24, 0x98);

	fc8300b_write(handle, devid, 0x2e, 0x64);
	fc8300b_write(handle, devid, 0x2f, 0x81);

	fc8300b_write(handle, devid, 0x3f, 0x01);
	fc8300b_write(handle, devid, 0x40,
		(0x44 + (filter_cal / 256)));
	fc8300b_write(handle, devid, 0x41,
		(filter_cal % 256));
	fc8300b_write(handle, devid, 0x3f, 0x00);

	fc8300b_write(handle, devid, 0x33, 0x84);
	fc8300b_write(handle, devid, 0x34, 0x32);
	fc8300b_write(handle, devid, 0x37, 0x65);
	fc8300b_write(handle, devid, 0x38, 0x55);
	fc8300b_write(handle, devid, 0x39, 0x05);
	fc8300b_write(handle, devid, 0x3c, 0xff);
	fc8300b_write(handle, devid, 0x3d, 0xff);
	fc8300b_write(handle, devid, 0x3e, 0xab);

	fc8300b_write(handle, devid, 0x50, 0xff);

	fc8300b_write(handle, devid, 0x70, 0x5f);
	fc8300b_write(handle, devid, 0x78, 0x31);

	fc8300b_write(handle, devid, 0x82, 0x88);
	fc8300b_write(handle, devid, 0x84, 0x14);
	fc8300b_write(handle, devid, 0x85, 0x0f);
	fc8300b_write(handle, devid, 0x86, 0x3f);
	fc8300b_write(handle, devid, 0x87, 0x18);
	fc8300b_write(handle, devid, 0x88, 0x0e);
	fc8300b_write(handle, devid, 0x89, 0x0b);
	fc8300b_write(handle, devid, 0x8a, 0x15);
	fc8300b_write(handle, devid, 0x8b, 0x0e);

	fc8300b_write(handle, devid, 0x90, 0xb2);
	fc8300b_write(handle, devid, 0x91, 0x78);

	fc8300b_write(handle, devid, 0xb2, 0x00);
	fc8300b_write(handle, devid, 0xb3, 0x07);
	fc8300b_write(handle, devid, 0xb4, 0x00);
	fc8300b_write(handle, devid, 0xb5, 0x07);
	fc8300b_write(handle, devid, 0xb6, 0x00);
	fc8300b_write(handle, devid, 0xb7, 0x07);
	fc8300b_write(handle, devid, 0xb8, 0x00);
	fc8300b_write(handle, devid, 0xb9, 0x07);
	fc8300b_write(handle, devid, 0xba, 0x00);
	fc8300b_write(handle, devid, 0xbb, 0x07);
	fc8300b_write(handle, devid, 0xbc, 0x00);
	fc8300b_write(handle, devid, 0xbd, 0x0a);
	fc8300b_write(handle, devid, 0xbe, 0x00);
	fc8300b_write(handle, devid, 0xbf, 0x0a);

	fc8300b_write(handle, devid, 0xc0, 0x0a);
	fc8300b_write(handle, devid, 0xc1, 0x00);
	fc8300b_write(handle, devid, 0xc2, 0x13);
	fc8300b_write(handle, devid, 0xc3, 0x00);

	fc8300b_write(handle, devid, 0xd3, 0x00);
	fc8300b_write(handle, devid, 0xd4, 0x01);
	fc8300b_write(handle, devid, 0xd5, 0x05);
	fc8300b_write(handle, devid, 0xd6, 0x0d);
	fc8300b_write(handle, devid, 0xd7, 0x0e);
	fc8300b_write(handle, devid, 0xd8, 0x0f);
	fc8300b_write(handle, devid, 0xd9, 0x2f);
	fc8300b_write(handle, devid, 0xda, 0x4f);
	fc8300b_write(handle, devid, 0xdb, 0x6f);
	fc8300b_write(handle, devid, 0xdc, 0x77);
	fc8300b_write(handle, devid, 0xdd, 0x97);

	fc8300b_write(handle, devid, 0x8f, 0xb6);

	fc8300b_write(handle, devid, 0xe9, 0x79);
	fc8300b_write(handle, devid, 0xed, 0x0c);
	fc8300b_write(handle, devid, 0xef, 0xb7);
}

static void fc8300b_tsb_3seg_init(HANDLE handle, DEVICEID devid, u8 filter_cal)
{
	fc8300b_write(handle, devid, 0x00, 0x00);
	fc8300b_write(handle, devid, 0x02, 0x05);
	fc8300b_write(handle, devid, 0x03, 0x04);
	fc8300b_write(handle, devid, 0x04, 0x10);
	fc8300b_write(handle, devid, 0x08, 0x21);

	fc8300b_write(handle, devid, 0x13, 0x07);
	fc8300b_write(handle, devid, 0x15, 0x46);
	fc8300b_write(handle, devid, 0x16, 0x42);
	fc8300b_write(handle, devid, 0x17, 0x42);
	fc8300b_write(handle, devid, 0x1a, 0x42);
	fc8300b_write(handle, devid, 0x1b, 0x42);
	fc8300b_write(handle, devid, 0x1d, 0x00);
	fc8300b_write(handle, devid, 0x1f, 0x79);

	fc8300b_write(handle, devid, 0x20, 0x21);
	fc8300b_write(handle, devid, 0x21, 0x10);
	fc8300b_write(handle, devid, 0x24, 0x98);
	fc8300b_write(handle, devid, 0x2e, 0x64);
	fc8300b_write(handle, devid, 0x2f, 0x71);

	fc8300b_write(handle, devid, 0x3f, 0x01);
	fc8300b_write(handle, devid, 0x41, filter_cal);
	fc8300b_write(handle, devid, 0x3f, 0x00);

	fc8300b_write(handle, devid, 0x33, 0x84);
	fc8300b_write(handle, devid, 0x34, 0x32);
	fc8300b_write(handle, devid, 0x37, 0x65);
	fc8300b_write(handle, devid, 0x38, 0x55);
	fc8300b_write(handle, devid, 0x39, 0x05);
	fc8300b_write(handle, devid, 0x3c, 0xff);
	fc8300b_write(handle, devid, 0x3d, 0xff);
	fc8300b_write(handle, devid, 0x3e, 0xab);

	fc8300b_write(handle, devid, 0x50, 0xff);

	fc8300b_write(handle, devid, 0x70, 0x5f);
	fc8300b_write(handle, devid, 0x78, 0x31);

	fc8300b_write(handle, devid, 0x82, 0x88);
	fc8300b_write(handle, devid, 0x84, 0x14);
	fc8300b_write(handle, devid, 0x85, 0x0f);
	fc8300b_write(handle, devid, 0x86, 0x3f);
	fc8300b_write(handle, devid, 0x87, 0x18);
	fc8300b_write(handle, devid, 0x88, 0x0e);
	fc8300b_write(handle, devid, 0x89, 0x0b);
	fc8300b_write(handle, devid, 0x8a, 0x15);
	fc8300b_write(handle, devid, 0x8b, 0x0e);
	fc8300b_write(handle, devid, 0x8f, 0xb6);

	fc8300b_write(handle, devid, 0x90, 0xb2);
	fc8300b_write(handle, devid, 0x91, 0x68);

	fc8300b_write(handle, devid, 0xb2, 0x00);
	fc8300b_write(handle, devid, 0xb3, 0x07);
	fc8300b_write(handle, devid, 0xb4, 0x00);
	fc8300b_write(handle, devid, 0xb5, 0x07);
	fc8300b_write(handle, devid, 0xb6, 0x00);
	fc8300b_write(handle, devid, 0xb7, 0x07);
	fc8300b_write(handle, devid, 0xb8, 0x00);
	fc8300b_write(handle, devid, 0xb9, 0x07);
	fc8300b_write(handle, devid, 0xba, 0x00);
	fc8300b_write(handle, devid, 0xbb, 0x07);
	fc8300b_write(handle, devid, 0xbc, 0x00);
	fc8300b_write(handle, devid, 0xbd, 0x0a);
	fc8300b_write(handle, devid, 0xbe, 0x00);
	fc8300b_write(handle, devid, 0xbf, 0x0a);

	fc8300b_write(handle, devid, 0xc0, 0x0a);
	fc8300b_write(handle, devid, 0xc1, 0x00);
	fc8300b_write(handle, devid, 0xc2, 0x13);
	fc8300b_write(handle, devid, 0xc3, 0x00);

	fc8300b_write(handle, devid, 0xd3, 0x00);
	fc8300b_write(handle, devid, 0xd4, 0x01);
	fc8300b_write(handle, devid, 0xd5, 0x05);
	fc8300b_write(handle, devid, 0xd6, 0x0d);
	fc8300b_write(handle, devid, 0xd7, 0x0e);
	fc8300b_write(handle, devid, 0xd8, 0x0f);
	fc8300b_write(handle, devid, 0xd9, 0x2f);
	fc8300b_write(handle, devid, 0xda, 0x4f);
	fc8300b_write(handle, devid, 0xdb, 0x6f);
	fc8300b_write(handle, devid, 0xdc, 0x77);
	fc8300b_write(handle, devid, 0xdd, 0x97);

	fc8300b_write(handle, devid, 0xe9, 0x79);
	fc8300b_write(handle, devid, 0xed, 0x0c);
	fc8300b_write(handle, devid, 0xef, 0xb7);
}

static void fc8300b_13seg_init(HANDLE handle, DEVICEID devid, u8 filter_cal)
{
	fc8300b_write(handle, devid, 0x00, 0x00);
	fc8300b_write(handle, devid, 0x02, 0x01);
	fc8300b_write(handle, devid, 0x03, 0x04);
	fc8300b_write(handle, devid, 0x04, 0x10);

	fc8300b_write(handle, devid, 0x08, 0xe4);

	fc8300b_write(handle, devid, 0x13, 0x07);
	fc8300b_write(handle, devid, 0x15, 0x46);
	fc8300b_write(handle, devid, 0x19, 0x53);
	fc8300b_write(handle, devid, 0x1d, 0x10);
	fc8300b_write(handle, devid, 0x1f, 0x79);

	fc8300b_write(handle, devid, 0x20, 0x42);
	fc8300b_write(handle, devid, 0x21, 0x30);
	fc8300b_write(handle, devid, 0x24, 0x98);

	fc8300b_write(handle, devid, 0x3f, 0x01);
	fc8300b_write(handle, devid, 0x41, filter_cal);
	fc8300b_write(handle, devid, 0x3f, 0x00);

	fc8300b_write(handle, devid, 0x33, 0x86);
	fc8300b_write(handle, devid, 0x34, 0x44);
	fc8300b_write(handle, devid, 0x37, 0x65);
	fc8300b_write(handle, devid, 0x38, 0x55);
	fc8300b_write(handle, devid, 0x39, 0x05);
	fc8300b_write(handle, devid, 0x3c, 0xff);
	fc8300b_write(handle, devid, 0x3d, 0xff);
	fc8300b_write(handle, devid, 0x3e, 0xab);

	fc8300b_write(handle, devid, 0x4d, 0x01);

	fc8300b_write(handle, devid, 0x50, 0xff);
	fc8300b_write(handle, devid, 0x53, 0x1a);
	fc8300b_write(handle, devid, 0x54, 0x80);

	fc8300b_write(handle, devid, 0x70, 0x5f);
	fc8300b_write(handle, devid, 0x78, 0x31);

	fc8300b_write(handle, devid, 0x82, 0x88);
	fc8300b_write(handle, devid, 0x84, 0x18);
	fc8300b_write(handle, devid, 0x85, 0x0c);
	fc8300b_write(handle, devid, 0x86, 0x3f);
	fc8300b_write(handle, devid, 0x87, 0x18);
	fc8300b_write(handle, devid, 0x88, 0x0e);
	fc8300b_write(handle, devid, 0x89, 0x0b);
	fc8300b_write(handle, devid, 0x8a, 0x15);
	fc8300b_write(handle, devid, 0x8b, 0x0e);
	fc8300b_write(handle, devid, 0x8f, 0xb6);

	fc8300b_write(handle, devid, 0x91, 0xf8);
	fc8300b_write(handle, devid, 0x90, 0xb2);

	fc8300b_write(handle, devid, 0xb2, 0x00);
	fc8300b_write(handle, devid, 0xb3, 0x07);
	fc8300b_write(handle, devid, 0xb4, 0x00);
	fc8300b_write(handle, devid, 0xb5, 0x07);
	fc8300b_write(handle, devid, 0xb6, 0x00);
	fc8300b_write(handle, devid, 0xb7, 0x07);
	fc8300b_write(handle, devid, 0xb8, 0x00);
	fc8300b_write(handle, devid, 0xb9, 0x0a);
	fc8300b_write(handle, devid, 0xba, 0x05);
	fc8300b_write(handle, devid, 0xbb, 0x0c);
	fc8300b_write(handle, devid, 0xbc, 0x05);
	fc8300b_write(handle, devid, 0xbd, 0x0c);
	fc8300b_write(handle, devid, 0xbe, 0x05);
	fc8300b_write(handle, devid, 0xbf, 0x0c);

	fc8300b_write(handle, devid, 0xc0, 0x0c);
	fc8300b_write(handle, devid, 0xc1, 0x05);
	fc8300b_write(handle, devid, 0xc2, 0x13);
	fc8300b_write(handle, devid, 0xc3, 0x05);

	fc8300b_write(handle, devid, 0xd3, 0x00);
	fc8300b_write(handle, devid, 0xd4, 0x01);
	fc8300b_write(handle, devid, 0xd5, 0x05);
	fc8300b_write(handle, devid, 0xd6, 0x0d);
	fc8300b_write(handle, devid, 0xd7, 0x0e);
	fc8300b_write(handle, devid, 0xd8, 0x0f);
	fc8300b_write(handle, devid, 0xd9, 0x2f);
	fc8300b_write(handle, devid, 0xda, 0x4f);
	fc8300b_write(handle, devid, 0xdb, 0x6f);
	fc8300b_write(handle, devid, 0xdc, 0x77);
	fc8300b_write(handle, devid, 0xdd, 0x97);

	fc8300b_write(handle, devid, 0xe9, 0x79);
	fc8300b_write(handle, devid, 0xed, 0x0c);
	fc8300b_write(handle, devid, 0xef, 0xb7);

	fc8300b_write(handle, devid, 0xcd, 0x41);
	fc8300b_write(handle, devid, 0xce, 0x88);
	fc8300b_write(handle, devid, 0xcf, 0x03);

}

static void fc8300b_tmm_13seg_init(HANDLE handle, DEVICEID devid, u8 filter_cal)
{
	fc8300b_write(handle, devid, 0x00, 0x00);
	fc8300b_write(handle, devid, 0x02, 0x03);
	fc8300b_write(handle, devid, 0x03, 0x04);
	fc8300b_write(handle, devid, 0x08, 0xe4);
	fc8300b_write(handle, devid, 0x04, 0x10);

	fc8300b_write(handle, devid, 0x13, 0x07);
	fc8300b_write(handle, devid, 0x15, 0x46);
	fc8300b_write(handle, devid, 0x16, 0x42);
	fc8300b_write(handle, devid, 0x17, 0x42);
	fc8300b_write(handle, devid, 0x1a, 0x42);
	fc8300b_write(handle, devid, 0x1c, 0x00);
	fc8300b_write(handle, devid, 0x1d, 0x00);
	fc8300b_write(handle, devid, 0x1f, 0x79);

	fc8300b_write(handle, devid, 0x20, 0x42);
	fc8300b_write(handle, devid, 0x21, 0x10);
	fc8300b_write(handle, devid, 0x24, 0x98);

	fc8300b_write(handle, devid, 0x3f, 0x01);
	fc8300b_write(handle, devid, 0x41, filter_cal);
	fc8300b_write(handle, devid, 0x3f, 0x00);

	fc8300b_write(handle, devid, 0x33, 0x86);
	fc8300b_write(handle, devid, 0x34, 0x44);
	fc8300b_write(handle, devid, 0x37, 0x65);
	fc8300b_write(handle, devid, 0x38, 0x55);
	fc8300b_write(handle, devid, 0x39, 0x05);
	fc8300b_write(handle, devid, 0x3c, 0xff);
	fc8300b_write(handle, devid, 0x3d, 0xff);
	fc8300b_write(handle, devid, 0x3e, 0xab);

	fc8300b_write(handle, devid, 0x50, 0xff);

	fc8300b_write(handle, devid, 0x70, 0x5f);
	fc8300b_write(handle, devid, 0x78, 0x31);

	fc8300b_write(handle, devid, 0x82, 0x88);
	fc8300b_write(handle, devid, 0x84, 0x14);
	fc8300b_write(handle, devid, 0x85, 0x0f);
	fc8300b_write(handle, devid, 0x86, 0x3f);
	fc8300b_write(handle, devid, 0x87, 0x18);
	fc8300b_write(handle, devid, 0x88, 0x0e);
	fc8300b_write(handle, devid, 0x89, 0x0b);
	fc8300b_write(handle, devid, 0x8a, 0x15);
	fc8300b_write(handle, devid, 0x8b, 0x0e);
	fc8300b_write(handle, devid, 0x8f, 0xb6);

	fc8300b_write(handle, devid, 0x90, 0xb2);
	fc8300b_write(handle, devid, 0x91, 0xf8);

	fc8300b_write(handle, devid, 0xb2, 0x00);
	fc8300b_write(handle, devid, 0xb3, 0x07);
	fc8300b_write(handle, devid, 0xb4, 0x00);
	fc8300b_write(handle, devid, 0xb5, 0x07);
	fc8300b_write(handle, devid, 0xb6, 0x00);
	fc8300b_write(handle, devid, 0xb7, 0x07);
	fc8300b_write(handle, devid, 0xb8, 0x00);
	fc8300b_write(handle, devid, 0xb9, 0x07);
	fc8300b_write(handle, devid, 0xba, 0x00);
	fc8300b_write(handle, devid, 0xbb, 0x07);
	fc8300b_write(handle, devid, 0xbc, 0x00);
	fc8300b_write(handle, devid, 0xbd, 0x0a);
	fc8300b_write(handle, devid, 0xbe, 0x00);
	fc8300b_write(handle, devid, 0xbf, 0x0a);

	fc8300b_write(handle, devid, 0xc0, 0x0a);
	fc8300b_write(handle, devid, 0xc1, 0x00);
	fc8300b_write(handle, devid, 0xc2, 0x13);
	fc8300b_write(handle, devid, 0xc3, 0x00);
	fc8300b_write(handle, devid, 0xcf, 0x03);

	fc8300b_write(handle, devid, 0xd3, 0x00);
	fc8300b_write(handle, devid, 0xd4, 0x01);
	fc8300b_write(handle, devid, 0xd5, 0x05);
	fc8300b_write(handle, devid, 0xd6, 0x0d);
	fc8300b_write(handle, devid, 0xd7, 0x0e);
	fc8300b_write(handle, devid, 0xd8, 0x0f);
	fc8300b_write(handle, devid, 0xd9, 0x2f);
	fc8300b_write(handle, devid, 0xda, 0x4f);
	fc8300b_write(handle, devid, 0xdb, 0x6f);
	fc8300b_write(handle, devid, 0xdc, 0x77);
	fc8300b_write(handle, devid, 0xdd, 0x97);

	fc8300b_write(handle, devid, 0xe9, 0x79);
	fc8300b_write(handle, devid, 0xed, 0x0c);
	fc8300b_write(handle, devid, 0xef, 0xb7);
}

s32 fc8300b_tuner_init(HANDLE handle, DEVICEID devid,
		enum BROADCAST_TYPE broadcast)
{
	u8 filter[4] = {0, 0, 0, 0};
	u8 filter_cal_18 = 0;
	u8 filter_cal_60 = 0;
	u8 filter_check = 0;

	u16 filter_cal_06 = 0;
	u16 filter_cal_09 = 0;

	u32 cal_temp = 0;

	s32 i = 0;

	tf_value[0] = 0;
	tf_value[1] = 0;
	tf_value[2] = 0;
	tf_value[3] = 0;

	broadcast_type = broadcast;

	/*
	 * ISDBT_1SEG = 0  ==> 0.9
	 * ISDBTMM_1SEG = 1 ==> 0.9
	 * ISDBTSB_1SEG = 2 ==> 0.6
	 * ISDBTSB_3SEG = 3 ==> 1.6
	 * ISDBT_13SEG = 4  ==> 6 / 7 / 8
	 * ISDBTMM_13SEG = 5 ==> 6 / 7 / 8
	 */

	cal_temp = FC8300B_XTAL_FREQ * 720 / 6000;

	if ((cal_temp % 10) >= 5)
		filter_cal_06 = (cal_temp / 10) + 1;
	else
		filter_cal_06 = cal_temp / 10;

	cal_temp = 0;

	cal_temp = FC8300B_XTAL_FREQ * 720 / 9000;

	if ((cal_temp % 10) >= 5)
		filter_cal_09 = (cal_temp / 10) + 1;
	else
		filter_cal_09 = cal_temp / 10;

	cal_temp = 0;

	cal_temp = FC8300B_XTAL_FREQ * 720 / 16000;

	if ((cal_temp % 10) >= 5)
		filter_cal_18 = (cal_temp / 10) + 1;
	else
		filter_cal_18 = cal_temp / 10;

	cal_temp = 0;

#if (BBM_BAND_WIDTH == 6)
	cal_temp = FC8300B_XTAL_FREQ * 720 / 30000;
#elif (BBM_BAND_WIDTH == 7)
	cal_temp = FC8300B_XTAL_FREQ * 720 / 35000;
#else /* BBM_BAND_WIDTH == 8 */
	cal_temp = FC8300B_XTAL_FREQ * 720 / 40000;
#endif /* #if (BBM_BAND_WIDTH == 6) */

	if ((cal_temp % 10) >= 5)
		filter_cal_60 = (cal_temp / 10) + 1;
	else
		filter_cal_60 = cal_temp / 10;

	switch (broadcast_type) {
	case ISDBT_1SEG:
		fc8300b_1seg_init(handle, devid, filter_cal_09);
		break;
	case ISDBTMM_1SEG:
		fc8300b_tmm_1seg_init(handle, devid, filter_cal_09);
		break;
	case ISDBTSB_1SEG:
		fc8300b_tsb_1seg_init(handle, devid, filter_cal_06);
		break;
	case ISDBTSB_3SEG:
		fc8300b_tsb_3seg_init(handle, devid, filter_cal_18);
		break;
	case ISDBT_13SEG:
		fc8300b_13seg_init(handle, devid, filter_cal_60);
		break;
	case ISDBTMM_13SEG:
		fc8300b_tmm_13seg_init(handle, devid, filter_cal_60);
		break;
	case ISDBT_CATV_13SEG:
		if (catv_status == 1)
			fc8300b_tmm_13seg_init(handle, devid, filter_cal_60);
		else if (catv_status == 2)
			fc8300b_13seg_init(handle, devid, filter_cal_60);
		else
			return BBM_OK;
		break;
	default:
		break;
	}

	fc8300b_write(handle, devid, 0x49, FC8300B_BAND_WIDTH);
	fc8300b_write(handle, devid, 0x4a, broadcast_type);
	fc8300b_write(handle, devid, 0x4b, (u8) (FC8300B_XTAL_FREQ / 1000));
	fc8300b_write(handle, devid, 0xfe, DRIVER_VERSION);

	if (devid == DIV_BROADCAST) {
		for (i = 0; i < 30; i++) {
			msWait(1);

			filter_check = 0;

			fc8300b_read(handle, DIV_MASTER, 0x43, &filter[0]);

			if (((filter[0] >> 4) & 0x01) == 1)
				filter_check += 1;

			if (filter_check == FC8300B_FILTER_CHECK)
				break;
		}
	} else {
		for (i = 0; i < 30; i++) {
			msWait(1);

			fc8300b_read(handle, devid, 0x43, &filter[0]);

			if (((filter[0] >> 4) & 0x01) == 1)
				break;
		}
	}

	if (FC8300B_XTAL_FREQ == 19200) {
		fc8300b_write(handle, devid, 0xee, 0x02);
		fc8300b_write(handle, devid, 0xf1, 0x52);
		fc8300b_write(handle, devid, 0xf2, 0x22);
		fc8300b_write(handle, devid, 0xf3, 0xf2);
		fc8300b_write(handle, devid, 0xf4, 0x22);

		fc8300b_write(handle, devid, 0x7c, 0x30);
		fc8300b_write(handle, devid, 0x7e, 0xff);
		fc8300b_write(handle, devid, 0x7f, 0xff);
	} else if (FC8300B_XTAL_FREQ == 24000) {
		fc8300b_write(handle, devid, 0xee, 0x02);
		fc8300b_write(handle, devid, 0xf1, 0x52);
		fc8300b_write(handle, devid, 0xf2, 0x2c);
		fc8300b_write(handle, devid, 0xf3, 0xf2);
		fc8300b_write(handle, devid, 0xf4, 0x2c);

		fc8300b_write(handle, devid, 0x7c, 0x30);
		fc8300b_write(handle, devid, 0x7e, 0xff);
		fc8300b_write(handle, devid, 0x7f, 0xff);
	} else if (FC8300B_XTAL_FREQ == 26000) {
		fc8300b_write(handle, devid, 0xee, 0x02);
		fc8300b_write(handle, devid, 0xf1, 0x52);
		fc8300b_write(handle, devid, 0xf2, 0x30);
		fc8300b_write(handle, devid, 0xf3, 0xf2);
		fc8300b_write(handle, devid, 0xf4, 0x30);

		fc8300b_write(handle, devid, 0x7c, 0x30);
		fc8300b_write(handle, devid, 0x7e, 0xff);
		fc8300b_write(handle, devid, 0x7f, 0xff);
	} else if (FC8300B_XTAL_FREQ == 27120) {
		fc8300b_write(handle, devid, 0xee, 0x02);
		fc8300b_write(handle, devid, 0xf1, 0x52);
		fc8300b_write(handle, devid, 0xf2, 0x32);
		fc8300b_write(handle, devid, 0xf3, 0xf2);
		fc8300b_write(handle, devid, 0xf4, 0x32);

		fc8300b_write(handle, devid, 0x7c, 0x30);
		fc8300b_write(handle, devid, 0x7e, 0xff);
		fc8300b_write(handle, devid, 0x7f, 0xff);
	} else if (FC8300B_XTAL_FREQ == 32000) {
		fc8300b_write(handle, devid, 0xee, 0x02);
		fc8300b_write(handle, devid, 0xf1, 0x53);
		fc8300b_write(handle, devid, 0xf2, 0x1a);
		fc8300b_write(handle, devid, 0xf3, 0xf3);
		fc8300b_write(handle, devid, 0xf4, 0x1a);

		fc8300b_write(handle, devid, 0x7c, 0x30);
		fc8300b_write(handle, devid, 0x7e, 0xff);
		fc8300b_write(handle, devid, 0x7f, 0xff);
	} else if (FC8300B_XTAL_FREQ == 37200) {
		fc8300b_write(handle, devid, 0xee, 0x02);
		fc8300b_write(handle, devid, 0xf1, 0x53);
		fc8300b_write(handle, devid, 0xf2, 0x20);
		fc8300b_write(handle, devid, 0xf3, 0xf3);
		fc8300b_write(handle, devid, 0xf4, 0x20);

		fc8300b_write(handle, devid, 0x7c, 0x30);
		fc8300b_write(handle, devid, 0x7e, 0xff);
		fc8300b_write(handle, devid, 0x7f, 0xff);
	} else if (FC8300B_XTAL_FREQ == 37400) {
		fc8300b_write(handle, devid, 0xee, 0x02);
		fc8300b_write(handle, devid, 0xf1, 0x53);
		fc8300b_write(handle, devid, 0xf2, 0x21);
		fc8300b_write(handle, devid, 0xf3, 0xf3);
		fc8300b_write(handle, devid, 0xf4, 0x21);

		fc8300b_write(handle, devid, 0x7c, 0x30);
		fc8300b_write(handle, devid, 0x7e, 0xff);
		fc8300b_write(handle, devid, 0x7f, 0xff);
	}

	if (devid == DIV_BROADCAST)
		vco_cal(handle, DIV_MASTER, 0);
	else
		vco_cal(handle, devid, 0);

	if ((broadcast_type == ISDBT_13SEG) || (broadcast_type == ISDBT_1SEG) ||
							(catv_status == 2)) {
		fc8300b_set_freq(handle, devid, 545143);

		fc8300b_write(handle, devid, 0x1e, 0x10);
		fc8300b_write(handle, devid, 0x1f, 0xf9);
		fc8300b_write(handle, devid, 0x78, 0x37);

		fc8300b_write(handle, devid, 0x04, 0x12);
		fc8300b_write(handle, devid, 0x8d, 0x08);
		fc8300b_write(handle, devid, 0x8e, 0x13);

		fc8300b_write(handle, devid, 0x13, 0x07);

		fc8300b_write(handle, devid, 0x22, 0x01);
		fc8300b_write(handle, devid, 0x24, 0x11);
		fc8300b_write(handle, devid, 0x23, 0x17);

		if (devid == DIV_BROADCAST)
			tf_value[0] = (tf_cal(handle, DIV_MASTER) - 7);
		else
			tf_value[0] = (tf_cal(handle, devid) - 7);

		fc8300b_write(handle, devid, 0x1e, 0x00);
		fc8300b_write(handle, devid, 0x1f, 0x79);
		fc8300b_write(handle, devid, 0x78, 0x31);
		fc8300b_write(handle, devid, 0x04, 0x10);
		fc8300b_write(handle, devid, 0x22, 0x00);
	}

	return BBM_OK;
}

static void fc8300b_set_freq_1seg(HANDLE handle, DEVICEID devid, u32 freq)
{
	u8 i = 0;

	for (i = 0; i < 56; i++) {
		if (((b_ch_mode_0[xtal_freq][i][0] + 3000) > freq) &&
			((b_ch_mode_0[xtal_freq][i][0] - 3000) <= freq))
			break;
	}

	fc8300b_write(handle, devid, 0xe9, b_ch_mode_0[xtal_freq][i][1]);
	fc8300b_write(handle, devid, 0x52, b_ch_mode_0[xtal_freq][i][2]);
}

static void fc8300b_set_freq_tmm_1seg(HANDLE handle, DEVICEID devid, u32 freq)
{
	u8 i = 0;

	for (i = 0; i < 8; i++) {
		if (((b_ch_mode_1[xtal_freq][i][0] + 100) > freq) &&
			((b_ch_mode_1[xtal_freq][i][0] - 400) <= freq))
			break;
	}

	fc8300b_write(handle, devid, 0xe9, b_ch_mode_1[xtal_freq][i][1]);
	fc8300b_write(handle, devid, 0x52, b_ch_mode_1[xtal_freq][i][2]);
}

static void fc8300b_set_freq_13seg(HANDLE handle, DEVICEID devid, u32 freq)
{
	u8 i;

	for (i = 0; i < 90; i++) {
		if (((b_ch_mode_4[xtal_freq][i][0] + 3000) > freq) &&
			((b_ch_mode_4[xtal_freq][i][0] - 3000) <= freq))
			break;
	}

	fc8300b_write(handle, devid, 0xe9, b_ch_mode_4[xtal_freq][i][1]);
	fc8300b_write(handle, devid, 0x52, b_ch_mode_4[xtal_freq][i][2]);
	fc8300b_write(handle, devid, 0x1f, 0x79);
}

static void fc8300b_set_freq_tmm_13seg(HANDLE handle, DEVICEID devid, u32 freq)
{
	u8 i;

	for (i = 0; i < 2; i++) {
		if (((b_ch_mode_5[xtal_freq][i][0] + 3000) > freq) &&
			((b_ch_mode_5[xtal_freq][i][0] - 3000) <= freq))
			break;
	}

	fc8300b_write(handle, devid, 0xe9, b_ch_mode_5[xtal_freq][i][1]);
	fc8300b_write(handle, devid, 0x52, b_ch_mode_5[xtal_freq][i][2]);
}

static void fc8300b_set_freq_catv_13seg(HANDLE handle, DEVICEID devid, u32 freq)
{
	u8 i;

	for (i = 0; i < 113; i++) {
		if (((b_ch_mode_6[xtal_freq][i][0] + 3000) > freq) &&
			((b_ch_mode_6[xtal_freq][i][0] - 3000) <= freq))
			break;
	}

	fc8300b_write(handle, devid, 0xe9, b_ch_mode_6[xtal_freq][i][1]);
	fc8300b_write(handle, devid, 0x52, b_ch_mode_6[xtal_freq][i][2]);
}

s32 fc8300b_set_freq(HANDLE handle, DEVICEID devid, u32 freq)
{
	u8 i;
	u8 offset = 0;
	u8 tf_cal_set = 0;

	if (FC8300B_XTAL_FREQ == 19200)
		xtal_freq = XTAL_FREQ_19200;
	else if (FC8300B_XTAL_FREQ == 24000)
		xtal_freq = XTAL_FREQ_24000;
	else if (FC8300B_XTAL_FREQ == 26000)
		xtal_freq = XTAL_FREQ_26000;
	else if (FC8300B_XTAL_FREQ == 27120)
		xtal_freq = XTAL_FREQ_27120;
	else if (FC8300B_XTAL_FREQ == 32000)
		xtal_freq = XTAL_FREQ_32000;
	else if (FC8300B_XTAL_FREQ == 37200)
		xtal_freq = XTAL_FREQ_37200;
	else if (FC8300B_XTAL_FREQ == 37400)
		xtal_freq = XTAL_FREQ_37400;

	switch (broadcast_type) {
	case ISDBT_1SEG:
		fc8300b_set_freq_1seg(handle, devid, freq);
		break;
	case ISDBTMM_1SEG:
		fc8300b_set_freq_tmm_1seg(handle, devid, freq);
		break;
	case ISDBT_13SEG:
		fc8300b_set_freq_13seg(handle, devid, freq);
		break;
	case ISDBTMM_13SEG:
		fc8300b_set_freq_tmm_13seg(handle, devid, freq);
		break;
	case ISDBT_CATV_13SEG:
		if (freq < 285143) {
			if (catv_status != 1) {
				catv_status = 1;
				fc8300b_tuner_init(handle, devid,
						broadcast_type);
			}
		} else if (freq >= 285143) {
			if (catv_status != 2) {
				catv_status = 2;
				fc8300b_tuner_init(handle, devid,
						broadcast_type);
			}
		}

		msWait(200);
		fc8300b_set_freq_catv_13seg(handle, devid, freq);

		break;
	default:
		break;
	}

	if (tf_value[0] != 0) {
		if (tf_offset[0][0] > freq) {
			fc8300b_write(handle, devid, 0x1c, 0x1c);
		} else if (tf_offset[0][11] < freq) {
			fc8300b_write(handle, devid, 0x1c, 0x10);
		} else {
			for (i = 0; i < 12; i++) {
				if ((tf_offset[0][i] < freq) &&
					(tf_offset[0][i + 1] >= freq))
					break;
			}

			if (devid != DIV_BROADCAST) {
				tf_cal_set = tf_value[devid & 0x0f] +
						tf_offset[1][i];

				if (tf_cal_set < 0x10)
					tf_cal_set = 0x10;
				else if (tf_cal_set > 0x1f)
					tf_cal_set = 0x1f;

				fc8300b_write(handle, devid, 0x1c, tf_cal_set);
			} else {
				tf_cal_set = tf_value[0] + tf_offset[1][i];

				if (tf_cal_set < 0x10)
					tf_cal_set = 0x10;
				else if (tf_cal_set > 0x1f)
					tf_cal_set = 0x1f;

				fc8300b_write(handle, DIV_MASTER, 0x1c,
							tf_cal_set);
			}
		}
	}

	if (devid != DIV_BROADCAST)
		fc8300b_tuner_set_pll(handle, devid, freq, offset);
	else
		fc8300b_tuner_set_pll(handle, DIV_MASTER, freq, offset);

	fc8300b_write(handle, devid, 0x75, 0x31);
	fc8300b_write(handle, devid, 0x78, 0xf1);
	fc8300b_write(handle, devid, 0x04, 0x12);

	msWait(3);

	fc8300b_write(handle, devid, 0x78, 0x31);
	fc8300b_write(handle, devid, 0x04, 0x10);

	return BBM_OK;
}

s32 fc8300b_get_rssi(HANDLE handle, DEVICEID devid, s32 *rssi)
{
	s8 tmp = 0;
	u8 filter_r_value = 0;

	fc8300b_read(handle, devid, 0xeb, (u8 *) &tmp);
	*rssi = tmp;

	if (tmp < 0)
		*rssi += 1;

	if (broadcast_type == ISDBT_1SEG || broadcast_type == ISDBTMM_1SEG) {
		fc8300b_read(handle, devid, 0x44, &filter_r_value);
		filter_r_value = (filter_r_value & 0x7f);

		if ((filter_r_value < 0x60) && (filter_r_value > 0x50))
			*rssi += 6;
	}

	return BBM_OK;
}

s32 fc8300b_tuner_deinit(HANDLE handle, DEVICEID devid)
{
	return BBM_OK;
}

