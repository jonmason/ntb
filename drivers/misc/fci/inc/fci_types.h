/*****************************************************************************
	Copyright(c) 2013 FCI Inc. All Rights Reserved

	File name : fci_types.h

	Description : header of type definition

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
#ifndef __FCI_TYPES_H__
#define __FCI_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HANDLE
#define HANDLE          void*
#endif

#define BBM_SPI         1
#define BBM_I2C         3 /* I2C+TSIF */
#define BBM_PPI         4 /* EBI2 LCD */

#define s8              signed char
#define s16             short int
#define s32             int
#define u8              unsigned char
#define u16             unsigned short
#define u32             unsigned int
#define ulong           unsigned long

#define TRUE            1
#define FALSE           0

#ifndef NULL
#define NULL            0
#endif

#define BBM_OK          0
#define BBM_NOK         1

#define BBM_E_FAIL           0x00000001
#define BBM_E_HOSTIF_SELECT  0x00000002
#define BBM_E_HOSTIF_INIT    0x00000003
#define BBM_E_BB_WRITE       0x00000100
#define BBM_E_BB_READ        0x00000101
#define BBM_E_TN_WRITE       0x00000200
#define BBM_E_TN_READ        0x00000201
#define BBM_E_TN_CTRL_SELECT 0x00000202
#define BBM_E_TN_CTRL_INIT   0x00000203
#define BBM_E_TN_SELECT      0x00000204
#define BBM_E_TN_INIT        0x00000205
#define BBM_E_TN_RSSI        0x00000206
#define BBM_E_TN_SET_FREQ    0x00000207

#define DIV_MASTER      0x5800
#define DIV_SLAVE0      0x5901
#define DIV_SLAVE1      0x5a02
#define DIV_SLAVE2      0x5b03
#define DIV_BROADCAST   0x5f04

#define DEVICEID        unsigned short

enum BROADCAST_TYPE {
	ISDBT_1SEG       = 0, /* B-31, T    1-SEG */
	ISDBTMM_1SEG     = 1, /* B-46, Tmm  1-SEG */
	ISDBTSB_1SEG     = 2, /* B-29, Tsb  1-SEG */
	ISDBTSB_3SEG     = 3, /* B-29, Tsb  3-SEG */
	ISDBT_13SEG      = 4, /* B-31, T   13-SEG */
	ISDBTMM_13SEG    = 5, /* B-46, Tmm 13-SEG */
	ISDBT_CATV_13SEG = 6,
	ISDBT_CATV_1SEG  = 7
};

#ifdef __cplusplus
}
#endif

#endif /* __FCI_TYPES_H__ */

