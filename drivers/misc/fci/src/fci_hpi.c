/*****************************************************************************
	Copyright(c) 2013 FCI Inc. All Rights Reserved

	File name : fci_hpi.c

	Description : source of internal hpi driver

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
#include <linux/slab.h>

#include "fci_types.h"
#include "fci_hal.h"
#include "fc8300_regs.h" /* deprecated */

static DEFINE_MUTEX(fci_hpi_lock);

s32 fci_hpi_init(HANDLE handle, DEVICEID devid, s32 speed, s32 slaveaddr)
{
	return BBM_OK;
}

s32 fci_hpi_read(HANDLE handle, DEVICEID devid,
		u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
	s32 res;

	mutex_lock(&fci_hpi_lock);
	res = bbm_bulk_read(handle, devid, 0x0f00 | addr, data, len);
	mutex_unlock(&fci_hpi_lock);


	return res;
}

s32 fci_hpi_write(HANDLE handle, DEVICEID devid,
		u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
	s32 res;

	mutex_lock(&fci_hpi_lock);
	res = bbm_bulk_write(handle, devid, 0x0f00 | addr, data, len);
	mutex_unlock(&fci_hpi_lock);

	return BBM_OK;
}

s32 fci_hpi_deinit(HANDLE handle, DEVICEID devid)
{
	return BBM_OK;
}

