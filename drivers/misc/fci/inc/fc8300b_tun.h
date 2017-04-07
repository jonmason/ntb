/*****************************************************************************
	Copyright(c) 2013 FCI Inc. All Rights Reserved

	File name : fc8300b_tun.h

	Description : header of FC8300B tuner driver

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
#ifndef __FC8300B_TUN_H__
#define __FC8300B_TUN_H__

#ifdef __cplusplus
extern "C" {
#endif

extern s32 fc8300b_tuner_init(HANDLE handle, DEVICEID devid,
				enum BROADCAST_TYPE broadcast);
extern s32 fc8300b_set_freq(HANDLE handle, DEVICEID devid, u32 freq);
extern s32 fc8300b_get_rssi(HANDLE handle, DEVICEID devid, s32 *rssi);
extern s32 fc8300b_tuner_deinit(HANDLE handle, DEVICEID devid);

#ifdef __cplusplus
}
#endif

#endif /* __FC8300B_TUN_H__ */

