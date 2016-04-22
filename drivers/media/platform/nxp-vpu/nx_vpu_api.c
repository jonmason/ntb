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

#ifndef UNUSED
#define UNUSED(p) ((void)(p))
#endif

#include <linux/delay.h>
#include <linux/io.h>

#include <dt-bindings/tieoff/s5p6818-tieoff.h>

#include "blackbird_v2.3.10.h"
#include "vpu_hw_interface.h"		/* Register Access */
#include "nx_vpu_api.h"


#define	DBG_POWER			0
#define	DBG_CLOCK			0
#define	INFO_MSG			0


/*--------------------------------------------------------------------------- */
/* Static Global Variables */
static int gstIsInitialized;
static int gstIsVPUOn;
static unsigned int gstVpuRegStore[64];

static uint32_t *gstCodaClockEnRegVir;

static struct nx_vpu_codec_inst gstVpuInstance[NX_MAX_VPU_INST_SPACE];


/*--------------------------------------------------------------------------- */
/*  Define Static Functions */
static unsigned int VPU_IsBusy(void);


/*----------------------------------------------------------------------------
 *		Nexell Specific VPU Hardware On/Off Logic
 *--------------------------------------------------------------------------- */
#define VPU_ALIVEGATE_REG		0xC0010800
#define VPU_NISOLATE_REG		0xC0010D00
#define VPU_NPRECHARGE_REG		0xC0010D04
#define VPU_NPOWERUP_REG		0xC0010D08
#define VPU_NPOWERACK_REG		0xC0010D0C
#define CODA960CLKENB_REG		0xC00C7000

#define	POWER_PMU_VPU_MASK		0x00000002

#if defined(CONFIG_ARCH_S5P6818)

#if 1
#define	TIEOFF_REG131			0xC001120C

#define	VPU_ASYNCXUI0_CSYSACK		(1<<14)
#define	VPU_ASYNCXUI0_CACTIVE		(1<<15)
#define	VPU_ASYNCXUI1_CSYSACK		(1<<16)
#define	VPU_ASYNCXUI1_CACTIVE		(1<<17)

#define	TIEOFF_REG69			0xC0011114
#define	VPU_ASYNCXUI0_CSYSREQ		(1<<3)
#define	VPU_ASYNCXUI1_CSYSREQ		(1<<4)
#endif


/*	Async XUI Power Down
 *
 *	Step 1. Waiting until CACTIVE to High
 *	Step 2. Set CSYSREQ to Low
 *	Step 3. Waiting until CSYSACK to Low
 */
static void NX_ASYNCXUI_PowerDown(void)
{
#if 1
	int32_t tmpVal;
	uint32_t *tieoff69;
	uint32_t *tieoff131;
#endif

	FUNC_IN();

#if 0
	nx_tieoff_set(NX_TIEOFF_INST_ASYNCXUI0_CSYSREQ, 0);
	nx_tieoff_set(NX_TIEOFF_INST_ASYNCXUI1_CSYSREQ, 0);

	nx_tieoff_set(NX_TIEOFF_INST_CODA960_ASYNCXIU0_CSYSACK_S, 0);
	nx_tieoff_set(NX_TIEOFF_INST_CODA960_ASYNCXIU0_CACTIVE_S, 0);
	nx_tieoff_set(NX_TIEOFF_INST_CODA960_ASYNCXIU1_CSYSACK_S, 0);
	nx_tieoff_set(NX_TIEOFF_INST_CODA960_ASYNCXIU1_CACTIVE_S, 0);
#else
	tieoff69 = ioremap(TIEOFF_REG69, 128);
	tieoff131 = ioremap(TIEOFF_REG131, 128);

	/* Apply To Async XUI 0 */

	/* Step 1. Waiting until CACTIVE to High */
	do {
		tmpVal = ReadReg32(tieoff131);
	} while (!(tmpVal&VPU_ASYNCXUI0_CACTIVE));

	/* Step 2. Set CSYSREQ to Low */
	tmpVal = ReadReg32(tieoff69);
	tmpVal &= (~VPU_ASYNCXUI0_CSYSREQ);
	WriteReg32(tieoff69, tmpVal);

	/*Step 3. Waiting until CSYSACK to Low */
	do {
		tmpVal = ReadReg32(tieoff131);
	} while (tmpVal&VPU_ASYNCXUI0_CSYSACK);


	/* Apply To Async XUI 1 */

	/* Step 1. Waiting until CACTIVE to High */
	do {
		tmpVal = ReadReg32(tieoff131);
	} while (!(tmpVal&VPU_ASYNCXUI1_CACTIVE));

	/* Step 2. Set CSYSREQ to Low */
	tmpVal = ReadReg32(tieoff69);
	tmpVal &= (~VPU_ASYNCXUI1_CSYSREQ);
	WriteReg32(tieoff69, tmpVal);

	/* Step 3. Waiting until CSYSACK to Low */
	do {
		tmpVal = ReadReg32(tieoff131);
	} while (tmpVal&VPU_ASYNCXUI1_CSYSACK);

	iounmap(tieoff69);
	iounmap(tieoff131);
#endif

	FUNC_OUT();
}

/*	Async XUI Power Up
 *
 *	Step 1. Set CSYSREQ to High
 *	Step 2. Waiting until CSYSACK to High
 */
static void NX_ASYNCXUI_PowerUp(void)
{
#if 1
	int32_t tmpVal;
	uint32_t *tieoff69;
	uint32_t *tieoff131;
#endif

	FUNC_IN();

#if 0
	nx_tieoff_set(NX_TIEOFF_INST_ASYNCXUI0_CSYSREQ, 1);
	nx_tieoff_set(NX_TIEOFF_INST_ASYNCXUI1_CSYSREQ, 1);

	nx_tieoff_set(NX_TIEOFF_INST_CODA960_ASYNCXIU0_CSYSACK_S, 1);
	nx_tieoff_set(NX_TIEOFF_INST_CODA960_ASYNCXIU0_CACTIVE_S, 1);
	nx_tieoff_set(NX_TIEOFF_INST_CODA960_ASYNCXIU1_CSYSACK_S, 1);
	nx_tieoff_set(NX_TIEOFF_INST_CODA960_ASYNCXIU1_CACTIVE_S, 1);
#else
	tieoff69 = ioremap(TIEOFF_REG69, 128);
	tieoff131 = ioremap(TIEOFF_REG131, 128);

	/* Apply To Async XUI 0 */

	/* Step 1. Set CSYSREQ to High */
	tmpVal = ReadReg32(tieoff69);
	tmpVal |= VPU_ASYNCXUI0_CSYSREQ;
	WriteReg32(tieoff69, tmpVal);

	/* Step 2. Waiting until CSYSACK to High */
	do {
		tmpVal = ReadReg32(tieoff131);
	} while (!(tmpVal&VPU_ASYNCXUI0_CSYSACK));

	/* Apply To Async XUI 1 */

	/* Step 1. Set CSYSREQ to High */
	tmpVal = ReadReg32(tieoff69);
	tmpVal |= VPU_ASYNCXUI1_CSYSREQ;
	WriteReg32(tieoff69, tmpVal);

	/* Step 2. Waiting until CSYSACK to High */
	do {
		tmpVal = ReadReg32(tieoff131);
	} while (!(tmpVal&VPU_ASYNCXUI1_CSYSACK));

	iounmap(tieoff69);
	iounmap(tieoff131);
#endif

	FUNC_OUT();
}
#endif

void NX_VPU_HwOn(void *dev, void *pVpuBaseAddr)
{
	uint32_t tmpVal;
	uint32_t *pIsolateBase;
	uint32_t *pAliveBase;
	uint32_t *pNPreCharge, *pNPowerUp, *pNPowerAck;

	NX_DbgMsg(DBG_POWER, ("NX_VPU_HwOn() ++\n"));

	/* Already Power On */
	if (gstIsVPUOn)
		return;

	vpu_soc_peri_reset_enter(dev);

	InitVpuRegister(pVpuBaseAddr);

	/* Initialize ISolate Register's */
	pIsolateBase = ioremap(VPU_NISOLATE_REG, 128);
	pNPreCharge  = pIsolateBase + 1;
	pNPowerUp    = pIsolateBase + 2;
	pNPowerAck   = pIsolateBase + 3;

	/* Initialize Alivegate Register */
	pAliveBase = ioremap(VPU_ALIVEGATE_REG, 128);

	/* printk("====================================\n");
	 * printk("pVpuBaseAddr = %p\n", pVpuBaseAddr);
	 * printk("pIsolateBase = %p\n", pIsolateBase);
	 * printk("pNPreCharge  = %p\n", pNPreCharge );
	 * printk("pNPowerUp    = %p\n", pNPowerUp   );
	 * printk("pNPowerAck   = %p\n", pNPowerAck  );
	 * printk("====================================\n"); */

	WriteReg32(pAliveBase,  0x3);

	/* Enable PreCharge */
	tmpVal = ReadReg32(pNPreCharge);
	tmpVal &= (~POWER_PMU_VPU_MASK);
	WriteReg32(pNPreCharge, tmpVal);

	/* Enable Power On */
	tmpVal = ReadReg32(pNPowerUp);
	tmpVal &= (~POWER_PMU_VPU_MASK);
	WriteReg32(pNPowerUp, tmpVal);

	/* Disable ISolate */
	tmpVal = ReadReg32(pIsolateBase);
	tmpVal |= (POWER_PMU_VPU_MASK);
	WriteReg32(pIsolateBase, tmpVal);

	mdelay(1);

	NX_VPU_Clock(1);

#if defined(CONFIG_ARCH_S5P6818)
	NX_ASYNCXUI_PowerUp();
#endif

	vpu_soc_peri_reset_exit(dev);

	gstIsVPUOn = 1;

	iounmap(pIsolateBase);
	iounmap(pAliveBase);

	NX_DbgMsg(DBG_POWER, ("NX_VPU_HwOn() --\n"));
}

void NX_VPU_HWOff(void *dev)
{
	FUNC_IN();

	if (gstIsVPUOn) {
		unsigned int tmpVal;
		uint32_t *pIsolateBase;
		uint32_t *pAliveBase;
		uint32_t *pNPreCharge, *pNPowerUp, *pNPowerAck;

#if defined(CONFIG_ARCH_S5P6818)
		NX_ASYNCXUI_PowerDown();
#endif
		/* H/W Reset */
		vpu_soc_peri_reset_enter(dev);

		NX_DbgMsg(DBG_POWER, ("NX_VPU_HWOff() ++\n"));

		/* Initialize ISolate Register's */
		pIsolateBase = ioremap(VPU_NISOLATE_REG, 16);
		pNPreCharge = pIsolateBase + 4;
		pNPowerUp = pIsolateBase + 8;
		pNPowerAck = pIsolateBase + 12;

		/* Initialize Alivegate Register */
		pAliveBase = ioremap(VPU_ALIVEGATE_REG, 16);

		/* Enter Coda Reset State */
		WriteReg32(pAliveBase,  0x3);

		/* Isolate VPU H/W */
		tmpVal = ReadReg32(pIsolateBase);
		tmpVal &= (~POWER_PMU_VPU_MASK);
		WriteReg32(pIsolateBase, tmpVal);

		/* Pre Charget Off */
		tmpVal = ReadReg32(pNPreCharge);
		tmpVal |= POWER_PMU_VPU_MASK;
		WriteReg32(pNPreCharge, tmpVal);

		/* Power Down */
		tmpVal = ReadReg32(pNPowerUp);
		tmpVal |= POWER_PMU_VPU_MASK;
		WriteReg32(pNPowerUp, tmpVal);

		/* Isolate VPU H/W */
		tmpVal = ReadReg32(pIsolateBase);
		tmpVal &= (~POWER_PMU_VPU_MASK);
		WriteReg32(pIsolateBase, tmpVal);

		iounmap(pIsolateBase);
		iounmap(pAliveBase);
		gstIsVPUOn = 0;

		NX_DbgMsg(DBG_POWER, ("NX_VPU_HWOff() --\n"));
	}

	FUNC_OUT();
}

int NX_VPU_GetCurPowerState(void)
{
	return gstIsVPUOn;
}

void NX_VPU_Clock(int on)
{
	FUNC_IN();

	if (gstCodaClockEnRegVir == NULL)
		gstCodaClockEnRegVir = ioremap(CODA960CLKENB_REG, 4);

	if (on) {
		WriteReg32(gstCodaClockEnRegVir, 0x0000000F);
		NX_DbgMsg(DBG_CLOCK, ("NX_VPU_Clock() ON\n"));
	} else {
		WriteReg32(gstCodaClockEnRegVir, 0x00000000);
		NX_DbgMsg(DBG_CLOCK, ("NX_VPU_Clock() OFF\n"));
	}

	FUNC_OUT();
}

static unsigned int VPU_IsBusy(void)
{
	unsigned int ret = ReadRegNoMsg(BIT_BUSY_FLAG);

	return ret != 0;
}

static int VPU_WaitBusBusy(int mSeconds, unsigned int busyFlagReg)
{
	while (mSeconds > 0) {
		if (0x77 == VpuReadReg(busyFlagReg))
			return VPU_RET_OK;
		DrvMSleep(1);
		mSeconds--;
	}
	return VPU_RET_ERR_TIMEOUT;
}

int VPU_WaitVpuBusy(int mSeconds, unsigned int busyFlagReg)
{
	while (mSeconds > 0) {
		if (ReadRegNoMsg(busyFlagReg) == 0)
			return VPU_RET_OK;

		DrvMSleep(1);
		mSeconds--;
	}
	return VPU_RET_ERR_TIMEOUT;
}

void VPU_GetVersionInfo(unsigned int *versionInfo, unsigned int *revision,
	unsigned int *productId)
{
	unsigned int ver;
	unsigned int rev;
	unsigned int pid;

	if (versionInfo && revision) {
		VpuWriteReg(RET_FW_VER_NUM, 0);
		VpuWriteReg(BIT_WORK_BUF_ADDR, 0);
		VpuWriteReg(BIT_BUSY_FLAG, 1);
		VpuWriteReg(BIT_RUN_INDEX, 0);
		VpuWriteReg(BIT_RUN_COD_STD, 0);
		VpuWriteReg(BIT_RUN_AUX_STD, 0);
		VpuWriteReg(BIT_RUN_COMMAND, FIRMWARE_GET);
		if (VPU_RET_OK != VPU_WaitVpuBusy(VPU_BUSY_CHECK_TIMEOUT,
			BIT_BUSY_FLAG)) {
			NX_ErrMsg(("Version Read Failed!!!\n"));
			return;
		}

		ver = VpuReadReg(RET_FW_VER_NUM);
		rev = VpuReadReg(RET_FW_CODE_REV);

		*versionInfo = ver;
		*revision = rev;
	}

	pid = VpuReadReg(DBG_CONFIG_REPORT_1);
	if ((pid&0xff00) == 0x3200)
		pid = 0x3200;

	if (productId)
		*productId = pid;
}

void CheckVersion(void)
{
	unsigned int version;
	unsigned int revision;
	unsigned int productId;

	VPU_GetVersionInfo(&version, &revision, &productId);

	NX_DbgMsg(INFO_MSG, ("Firmware Version => projectId : %x | ",
		(unsigned int)(version>>16)));
	NX_DbgMsg(INFO_MSG, ("version : %04d.%04d.%08d | revision : r%d\n",
		(unsigned int)((version>>(12))&0x0f),
		(unsigned int)((version>>(8))&0x0f),
		(unsigned int)((version)&0xff),
		revision));
	NX_DbgMsg(INFO_MSG, ("Hardware Version => %04x\n", productId));
}


/*--------------------------------------------------------------------------- */

/* VPU_SWReset
 * IN
 *    forcedReset : 1 if there is no need to waiting for BUS transaction,
 *                     0 for otherwise
 * OUT
 *    RetCode : RETCODE_FAILURE if failed to reset,
 *                RETCODE_SUCCESS for otherwise
 */

/* SW Reset command */
#define VPU_SW_RESET_BPU_CORE	0x008
#define VPU_SW_RESET_BPU_BUS	0x010
#define VPU_SW_RESET_VCE_CORE	0x020
#define VPU_SW_RESET_VCE_BUS	0x040
#define VPU_SW_RESET_GDI_CORE	0x080
#define VPU_SW_RESET_GDI_BUS	0x100

int VPU_SWReset(int resetMode)
{
	unsigned int cmd;

	if (resetMode == SW_RESET_SAFETY || resetMode == SW_RESET_ON_BOOT) {
		/* Waiting for completion of bus transaction */
		/* Step1 : No more request */
		/* no more request {3'b0,no_more_req_sec,3'b0,no_more_req} */
		VpuWriteReg(GDI_BUS_CTRL, 0x11);

		/* Step2 : Waiting for completion of bus transaction */
		/* while (VpuReadReg(coreIdx, GDI_BUS_STATUS) != 0x77) */
		if (VPU_WaitBusBusy(VPU_BUSY_CHECK_TIMEOUT, GDI_BUS_STATUS) ==
			VPU_RET_ERR_TIMEOUT) {
			VpuWriteReg(GDI_BUS_CTRL, 0x00);
			return VPU_RET_ERR_TIMEOUT;
		}

		/* Step3 : clear GDI_BUS_CTRL */
		VpuWriteReg(GDI_BUS_CTRL, 0x00);
	}

	cmd = 0;
	/* Software Reset Trigger */
	if (resetMode != SW_RESET_ON_BOOT)
		cmd =  VPU_SW_RESET_BPU_CORE | VPU_SW_RESET_BPU_BUS;
	cmd |= VPU_SW_RESET_VCE_CORE | VPU_SW_RESET_VCE_BUS;
	if (resetMode == SW_RESET_ON_BOOT)
		/* If you reset GDI, tiled map should be reconfigured */
		cmd |= VPU_SW_RESET_GDI_CORE | VPU_SW_RESET_GDI_BUS;
	VpuWriteReg(BIT_SW_RESET, cmd);

	/* wait until reset is done */
	if (VPU_WaitVpuBusy(VPU_BUSY_CHECK_TIMEOUT, BIT_SW_RESET_STATUS) ==
		VPU_RET_ERR_TIMEOUT) {
		VpuWriteReg(BIT_SW_RESET, 0x00);
		return VPU_RET_ERR_TIMEOUT;
	}

	VpuWriteReg(BIT_USE_NX_EXPND, USE_NX_EXPND);
	VpuWriteReg(BIT_SW_RESET, 0);

	return VPU_RET_OK;
}

/*----------------------------------------------------------------------------
 *	VPU Initialization.
 *--------------------------------------------------------------------------- */
int NX_VpuInit(void *dev, void *baseAddr, void *firmVirAddr,
	uint32_t firmPhyAddr)
{
	enum nx_vpu_ret ret = VPU_RET_OK;
	int32_t i;
	uint32_t tmpData;
	uint32_t codeBufAddr, tmpBufAddr, paramBufAddr;

	if (gstIsInitialized)
		return VPU_RET_OK;

	codeBufAddr = firmPhyAddr;
	tmpBufAddr  = codeBufAddr + CODE_BUF_SIZE;
	paramBufAddr = tmpBufAddr + TEMP_BUF_SIZE;

	NX_VPU_HwOn(dev, baseAddr);

	/* if BIT processor is not running. */
	if (VpuReadReg(BIT_CUR_PC) == 0) {
		for (i = 0; i < 64; i++)
			VpuWriteReg(BIT_BASE + 0x100 + (i*4), 0x0);
	}

	VPU_SWReset(SW_RESET_ON_BOOT);

	{
		unsigned char *dst = (unsigned char *)firmVirAddr;

		for (i = 0; i < ARRAY_SIZE(bit_code) ; i += 4) {
			*dst++ = (unsigned char)(bit_code[i+3] >> 0);
			*dst++ = (unsigned char)(bit_code[i+3] >> 8);

			*dst++ = (unsigned char)(bit_code[i+2] >> 0);
			*dst++ = (unsigned char)(bit_code[i+2] >> 8);

			*dst++ = (unsigned char)(bit_code[i+1] >> 0);
			*dst++ = (unsigned char)(bit_code[i+1] >> 8);

			*dst++ = (unsigned char)(bit_code[i+0] >> 0);
			*dst++ = (unsigned char)(bit_code[i+0] >> 8);
		}
	}

	VpuWriteReg(BIT_INT_ENABLE, 0);
	VpuWriteReg(BIT_CODE_RUN, 0);

	for (i = 0 ; i < 2048 ; i++) {
		tmpData = bit_code[i];
		WriteRegNoMsg(BIT_CODE_DOWN, (i<<16)|tmpData);
	}

	VpuWriteReg(BIT_PARA_BUF_ADDR, paramBufAddr);
	VpuWriteReg(BIT_CODE_BUF_ADDR, codeBufAddr);
	VpuWriteReg(BIT_TEMP_BUF_ADDR, tmpBufAddr);

	VpuWriteReg(BIT_BIT_STREAM_CTRL, VPU_STREAM_ENDIAN);

	/* Interleave bit position is modified */
	VpuWriteReg(BIT_FRAME_MEM_CTRL, CBCR_INTERLEAVE<<2|VPU_FRAME_ENDIAN);

	VpuWriteReg(BIT_BIT_STREAM_PARAM, 0);

	VpuWriteReg(BIT_AXI_SRAM_USE, 0);
	VpuWriteReg(BIT_INT_ENABLE, 0);
	VpuWriteReg(BIT_ROLLBACK_STATUS, 0);

	tmpData  = (1<<VPU_INT_BIT_BIT_BUF_FULL);
	tmpData |= (1<<VPU_INT_BIT_BIT_BUF_EMPTY);
	tmpData |= (1<<VPU_INT_BIT_DEC_MB_ROWS);
	tmpData |= (1<<VPU_INT_BIT_SEQ_INIT);
	tmpData |= (1<<VPU_INT_BIT_DEC_FIELD);
	tmpData |= (1<<VPU_INT_BIT_PIC_RUN);
	VpuWriteReg(BIT_INT_ENABLE, tmpData);
	VpuWriteReg(BIT_INT_CLEAR, 0x1);

	VpuWriteReg(BIT_USE_NX_EXPND, USE_NX_EXPND);
	VpuWriteReg(BIT_BUSY_FLAG, 0x1);
	VpuWriteReg(BIT_CODE_RESET, 1);
	VpuWriteReg(BIT_CODE_RUN, 1);

	if (VPU_RET_OK != VPU_WaitVpuBusy(VPU_BUSY_CHECK_TIMEOUT,
		BIT_BUSY_FLAG)) {
		NX_ErrMsg(("NX_VpuInit() Failed. Timeout(%d)\n",
			VPU_BUSY_CHECK_TIMEOUT));
		return ret;
	}

	CheckVersion();

	for (i = 0 ; i < NX_MAX_VPU_INST_SPACE ; i++) {
		gstVpuInstance[i].inUse = 0;
		gstVpuInstance[i].paramPhyAddr = paramBufAddr;
		gstVpuInstance[i].paramVirAddr = (uint64_t)(firmVirAddr +
			CODE_BUF_SIZE + TEMP_BUF_SIZE);
		gstVpuInstance[i].paramBufSize = PARA_BUF_SIZE;
	}
	gstIsInitialized = 1;

	return ret;
}

int NX_VpuDeInit(void *dev)
{
	if (!gstIsInitialized) {
		NX_ErrMsg(("VPU Already Denitialized!!!\n"));
		return VPU_RET_ERR_INIT;
	}

	if (VPU_IsBusy()) {
		NX_ErrMsg(("NX_VpuDeInit() failed. VPU_IsBusy!!!\n"));
		return VPU_RET_BUSY;
	}

	NX_VPU_HWOff(dev);

	gstIsInitialized = 0;
	return VPU_RET_OK;
}

int NX_VpuSuspend(void *dev)
{
	int i;

	if (!gstIsInitialized)
		return VPU_RET_ERR_INIT;

	if (VPU_IsBusy())
		return VPU_RET_BUSY;
	for (i = 0 ; i < 64 ; i++)
		gstVpuRegStore[i] = VpuReadReg(BIT_BASE+0x100 + (i * 4));

	NX_VPU_HWOff(dev);

	return VPU_RET_OK;
}

int NX_VpuResume(void)
{
	int i;
	unsigned int value;

	if (!gstIsInitialized)
		return VPU_RET_ERR_INIT;

	NX_VPU_HwOn(NULL, NULL);

	VPU_SWReset(SW_RESET_ON_BOOT);

	for (i = 0 ; i < 64 ; i++)
		VpuWriteReg(BIT_BASE+0x100+(i * 4), gstVpuRegStore[i]);

	VpuWriteReg(BIT_CODE_RUN, 0);
	/* Bit Code */
	for (i = 0; i < 2048; i++) {
		value = bit_code[i];
		VpuWriteReg(BIT_CODE_DOWN, ((i << 16) | value));
	}

	VpuWriteReg(BIT_BUSY_FLAG, 1);
	VpuWriteReg(BIT_CODE_RESET, 1);
	VpuWriteReg(BIT_CODE_RUN, 1);

	VpuWriteReg(BIT_USE_NX_EXPND, USE_NX_EXPND);

	if (VPU_RET_OK != VPU_WaitVpuBusy(VPU_BUSY_CHECK_TIMEOUT,
		BIT_BUSY_FLAG)) {
		NX_ErrMsg(("NX_VpuResume() Failed. Timeout(%d)\n",
			VPU_BUSY_CHECK_TIMEOUT));
		return VPU_RET_ERR_TIMEOUT;
	}

	return VPU_RET_OK;
}

struct nx_vpu_codec_inst *NX_VpuGetInstance(int *pIndex)
{
	int i;
	struct nx_vpu_codec_inst *hInst = 0;

	for (i = 0; i < NX_MAX_VPU_INSTANCE ; i++) {
		if (!gstVpuInstance[i].inUse) {
			hInst = &gstVpuInstance[i];
			*pIndex = i;
			break;
		}
	}
	return hInst;
}

int NX_VpuIsInitialized(void)
{
	return gstIsInitialized;
}

int swap_endian(unsigned char *data, int len)
{
	unsigned int *p;
	unsigned int v1, v2, v3;
	int i;
	int swap = 0;

	p = (unsigned int *)data;

	for (i = 0; i < len/4; i += 2) {
		v1 = p[i];
		v2  = (v1 >> 24) & 0xFF;
		v2 |= ((v1 >> 16) & 0xFF) <<  8;
		v2 |= ((v1 >>  8) & 0xFF) << 16;
		v2 |= ((v1 >>  0) & 0xFF) << 24;
		v3 =  v2;
		v1  = p[i+1];
		v2  = (v1 >> 24) & 0xFF;
		v2 |= ((v1 >> 16) & 0xFF) <<  8;
		v2 |= ((v1 >>  8) & 0xFF) << 16;
		v2 |= ((v1 >>  0) & 0xFF) << 24;
		p[i]   =  v2;
		p[i+1] = v3;
	}

	return swap;
}

void NX_VpuParaInitialized(void)
{
	gstIsInitialized = 0;
	gstIsVPUOn = 0;
	gstCodaClockEnRegVir = NULL;
}
