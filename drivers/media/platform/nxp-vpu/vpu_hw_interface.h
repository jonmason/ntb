/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Seonghee, Kim <kshblue@nexell.co.kr>
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

#ifndef __VPU_HW_INTERFACE_H__
#define	__VPU_HW_INTERFACE_H__

#include "nx_vpu_config.h"
#include "regdefine.h"
#include "nx_vpu_api.h"


/*----------------------------------------------------------------------------
 *		Register Interface
 */


/* Offset Based Register Access for VPU */
void VpuWriteReg(uint32_t offset, uint32_t value);
uint32_t VpuReadReg(uint32_t offset);
void WriteRegNoMsg(uint32_t offset, uint32_t value);
uint32_t ReadRegNoMsg(uint32_t offset);


/* Direct Register Access API */
void WriteReg32(uint32_t *address, uint32_t value);
uint32_t ReadReg32(uint32_t *address);

void InitVpuRegister(void *virAddr);
uint32_t *GetVpuRegBase(void);

/*----------------------------------------------------------------------------
 *		Host Command Interface
 */
void VpuBitIssueCommand(struct nx_vpu_codec_inst *inst, enum nx_vpu_cmd cmd);

#endif		/* __VPU_HW_INTERFACE_H__ */
