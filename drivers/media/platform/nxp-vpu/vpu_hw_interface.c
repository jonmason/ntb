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

#include "vpu_hw_interface.h"

#define DBG_VBS 0

static void *gstBaseAddr;


/*----------------------------------------------------------------------------
 *	Register Interface
 */

void VpuWriteReg(uint32_t offset, uint32_t value)
{
	uint32_t *addr = (uint32_t *)((void *)(gstBaseAddr + offset));
#if NX_REG_EN_MSG
	NX_DbgMsg(NX_REG_EN_MSG, ("write(0x%08x, 0x%08x)addr(%p)\n", offset,
		value, addr));
#endif
	*addr = value;
}

uint32_t VpuReadReg(uint32_t offset)
{
#if NX_REG_EN_MSG

	NX_DbgMsg(NX_REG_EN_MSG, ("read(0x%08x, 0x%08x)\n", offset,
		*((int32_t *)(gstBaseAddr + offset))));
#endif
	return *((uint32_t *)(gstBaseAddr+offset));
}

void WriteRegNoMsg(uint32_t offset, uint32_t value)
{
	uint32_t *addr = gstBaseAddr + offset;
	*addr = value;
}

uint32_t ReadRegNoMsg(uint32_t offset)
{
	return *((uint32_t *)(gstBaseAddr+offset));
}

void WriteReg32(uint32_t *address, uint32_t value)
{
	*address = value;
}

uint32_t ReadReg32(uint32_t *address)
{
	return *address;
}

void InitVpuRegister(void *virAddr)
{
	gstBaseAddr = virAddr;
}

uint32_t *GetVpuRegBase(void)
{
	return gstBaseAddr;
}

/*----------------------------------------------------------------------------
 *		Host Command
 */
void VpuBitIssueCommand(struct nx_vpu_codec_inst *inst, enum nx_vpu_cmd cmd)
{
	NX_DbgMsg(DBG_VBS, ("VpuBitIssueCommand : cmd = %d, address=0x%llx, ",
		cmd, inst->instBufPhyAddr));
	NX_DbgMsg(DBG_VBS, ("instIndex=%d, codecMode=%d, auxMode=%d\n",
		inst->instIndex, inst->codecMode, inst->auxMode));

	VpuWriteReg(BIT_WORK_BUF_ADDR, (uint32_t)inst->instBufPhyAddr);
	VpuWriteReg(BIT_BUSY_FLAG, 1);
	VpuWriteReg(BIT_RUN_INDEX, inst->instIndex);
	VpuWriteReg(BIT_RUN_COD_STD, inst->codecMode);
	VpuWriteReg(BIT_RUN_AUX_STD, inst->auxMode);
	VpuWriteReg(BIT_RUN_COMMAND, cmd);
}
