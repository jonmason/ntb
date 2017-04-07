/*****************************************************************************
	Copyright(c) 2013 FCI Inc. All Rights Reserved

	File name : fci_tun.h

	Description : header of tuner control driver

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
#ifndef __FCI_TUN_H__
#define __FCI_TUN_H__

#define CENTER_SUBCH_NUM    0x16

#ifdef __cplusplus
extern "C" {
#endif

enum I2C_TYPE {
	FCI_HPI_TYPE        = 3
};

enum PRODUCT_TYPE {
	FC8300_TUNER        = 8300,
	FC8300B_TUNER       = 8301
};

extern s32 tuner_ctrl_select(HANDLE handle, DEVICEID devid,
		enum I2C_TYPE type);
extern s32 tuner_ctrl_deselect(HANDLE handle, DEVICEID devid);
extern s32 tuner_select(HANDLE handle, DEVICEID devid,
		enum PRODUCT_TYPE product, enum BROADCAST_TYPE broadcast);
extern s32 tuner_deselect(HANDLE handle, DEVICEID devid);

extern s32 tuner_i2c_read(HANDLE handle, DEVICEID devid,
		u8 addr, u8 alen, u8 *data, u8 len);
extern s32 tuner_i2c_write(HANDLE handle, DEVICEID devid,
		u8 addr, u8 alen, u8 *data, u8 len);
extern s32 tuner_set_freq(HANDLE handle, DEVICEID devid, u32 freq, u8 subch);
extern s32 tuner_get_rssi(HANDLE handle, DEVICEID devid, s32 *rssi);

#ifdef __cplusplus
}
#endif

#endif /* __FCI_TUN_H__ */

