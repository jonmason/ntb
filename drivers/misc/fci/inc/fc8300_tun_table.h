/*****************************************************************************
	Copyright(c) 2013 FCI Inc. All Rights Reserved

	File name : fc8300_tun_table.c

	Description : header of FC8300 tuner driver

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
******************************************************************************/
#include "fci_types.h"

#ifndef __FC8300_TUN_TABLE_H__
#define __FC8300_TUN_TABLE_H__

#ifdef __cplusplus
extern "C" {
#endif

extern u32 ch_mode_0[7][57][20];
extern u32 ch_mode_1[7][9][16];
extern u32 ch_mode_4[7][57][20];
extern u32 ch_mode_5[7][2][16];
extern u32 ch_mode_6[7][113][21];

#ifdef __cplusplus
}
#endif

#endif /* __FC8300_TUN_TABLE_H__ */
