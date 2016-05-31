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

#include <linux/platform_device.h>

#include "vpu_hw_interface.h"           /* Register Access */
#include "nx_vpu_api.h"
#include "nx_vpu_gdi.h"


#define DBG_USERDATA			0
#define DBG_REGISTER			0
#define DBG_ES_ADDR			0
#define	INFO_MSG			0


/*--------------------------------------------------------------------------- */
/* Decoder Functions */
static int FillBuffer(struct nx_vpu_codec_inst *pInst, unsigned char *stream,
	int size);
static int VPU_DecSeqInitCommand(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_seq_init_arg *pArg);
static int VPU_DecSeqComplete(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_seq_init_arg *pArg);
static int VPU_DecRegisterFrameBufCommand(struct nx_vpu_codec_inst
	*pInst, struct vpu_dec_reg_frame_arg *pArg);
static int VPU_DecStartOneFrameCommand(struct nx_vpu_codec_inst
	*pInst, struct vpu_dec_frame_arg *pArg);
static int VPU_DecGetOutputInfo(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_frame_arg *pArg);
static int VPU_DecCloseCommand(struct nx_vpu_codec_inst *pInst,
	void *vpu_event_present);


/*----------------------------------------------------------------------------
 *			Decoder APIs
 */

int NX_VpuDecOpen(struct vpu_open_arg *pOpenArg, void *devHandle,
	struct nx_vpu_codec_inst **ppInst)
{
	int val;
	struct vpu_dec_info *pDecInfo;
	struct nx_vpu_codec_inst *hInst = 0;

	FUNC_IN();

	*ppInst = 0;
	if (!NX_VpuIsInitialized())
		return VPU_RET_ERR_INIT;

	hInst = NX_VpuGetInstance(pOpenArg->instIndex);
	if (!hInst)
		return VPU_RET_ERR_INST;

	if (pOpenArg->codecStd == CODEC_STD_MPEG4) {
		hInst->codecMode = MP4_DEC;
		hInst->auxMode = MP4_AUX_MPEG4;
	} else if (pOpenArg->codecStd == CODEC_STD_AVC) {
		hInst->codecMode = AVC_DEC;
		hInst->auxMode = AVC_AUX_AVC;
	} else if (pOpenArg->codecStd == CODEC_STD_VC1) {
		hInst->codecMode = VC1_DEC;
	} else if (pOpenArg->codecStd == CODEC_STD_MPEG2) {
		hInst->codecMode = MP2_DEC;
		hInst->auxMode = 0;
	} else if (pOpenArg->codecStd == CODEC_STD_H263) {
		hInst->codecMode = MP4_DEC;
		hInst->auxMode = 0;
	} else if (pOpenArg->codecStd == CODEC_STD_DIV3) {
		hInst->codecMode = DV3_DEC;
		hInst->auxMode = MP4_AUX_DIVX3;
	} else if (pOpenArg->codecStd == CODEC_STD_RV) {
		hInst->codecMode = RV_DEC;
	} else if (pOpenArg->codecStd == CODEC_STD_AVS) {
		hInst->codecMode = AVS_DEC;
	} else if (pOpenArg->codecStd == CODEC_STD_MJPG) {
		hInst->codecMode = MJPG_DEC;
	} else if (pOpenArg->codecStd == CODEC_STD_THO) {
		hInst->codecMode = VPX_DEC;
		hInst->auxMode = VPX_AUX_THO;
	} else if (pOpenArg->codecStd == CODEC_STD_VP3) {
		hInst->codecMode = VPX_DEC;
		hInst->auxMode = VPX_AUX_THO;
	} else if (pOpenArg->codecStd == CODEC_STD_VP8) {
		hInst->codecMode = VPX_DEC;
		hInst->auxMode = VPX_AUX_VP8;
	} else {
		NX_ErrMsg(("NX_VpuDecOpen() failed!!!\n"));
		NX_ErrMsg(("Cannot support codec standard (%d)\n",
			pOpenArg->codecStd));
		return VPU_RET_ERR_PARAM;
	}

	/* Set Base Information */
	hInst->inUse = 1;
	hInst->instIndex = pOpenArg->instIndex;
	hInst->devHandle = devHandle;

	hInst->instBufPhyAddr = (uint64_t)pOpenArg->instanceBuf.phyAddr;
	hInst->instBufVirAddr = (uint64_t)pOpenArg->instanceBuf.virAddr;
	hInst->instBufSize    = pOpenArg->instanceBuf.size;
	pDecInfo = &hInst->codecInfo.decInfo;

	/* Clrear Instnace Information */
	NX_DrvMemset(&hInst->codecInfo, 0, sizeof(hInst->codecInfo));
	pDecInfo->codecStd = pOpenArg->codecStd;
	pDecInfo->mp4Class = pOpenArg->mp4Class;

	if (hInst->codecMode != MJPG_DEC) {
		pDecInfo->streamRdPtrRegAddr = BIT_RD_PTR;
		pDecInfo->streamWrPtrRegAddr = BIT_WR_PTR;
		pDecInfo->frameDisplayFlagRegAddr = BIT_FRM_DIS_FLG;
	} else {
		pDecInfo->streamRdPtrRegAddr = MJPEG_BBC_RD_PTR_REG;
		pDecInfo->streamWrPtrRegAddr = MJPEG_BBC_WR_PTR_REG;
		pDecInfo->frameDisplayFlagRegAddr = 0;
	}

	pDecInfo->strmBufPhyAddr = (uint64_t)pOpenArg->streamBuf.phyAddr;
	pDecInfo->strmBufVirAddr = (uint64_t)pOpenArg->streamBuf.virAddr;
	pDecInfo->strmBufSize    = pOpenArg->streamBuf.size;
	NX_DrvMemset((unsigned char *)pDecInfo->strmBufVirAddr, 0,
		pDecInfo->strmBufSize);

	/* Set Other Parameters */
	pDecInfo->frameDelay = -1;
	pDecInfo->userDataEnable = 0;
	pDecInfo->enableReordering = VPU_REORDER_ENABLE;
	pDecInfo->vc1BframeDisplayValid = 0;
	pDecInfo->avcErrorConcealMode = 0;
	pDecInfo->enableMp4Deblock = 0;

	if (pDecInfo->codecStd == CODEC_STD_AVC) {
		pDecInfo->bitStreamMode = BS_MODE_PIC_END;
		/*pDecInfo->bitStreamMode = BS_MODE_ROLLBACK; */
	} else if (pDecInfo->codecStd == CODEC_STD_H263 ||
		pDecInfo->codecStd == CODEC_STD_MPEG4) {
		pDecInfo->bitStreamMode = BS_MODE_PIC_END;
	} else {
		pDecInfo->bitStreamMode = BS_MODE_ROLLBACK;
	}

	pDecInfo->streamEndflag = 0;/* Frame Unit Operation */
	pDecInfo->bwbEnable = VPU_ENABLE_BWB;
	pDecInfo->seqInitEscape = 0;
	pDecInfo->streamEndian = VPU_STREAM_ENDIAN;

	NX_DrvMemset((void *)hInst->paramVirAddr, 0, PARA_BUF_SIZE);

	VpuWriteReg(pDecInfo->streamWrPtrRegAddr, pDecInfo->strmBufPhyAddr);
	VpuWriteReg(pDecInfo->streamRdPtrRegAddr, pDecInfo->strmBufPhyAddr);

	pDecInfo->readPos  = pDecInfo->strmBufPhyAddr;
	pDecInfo->writePos = pDecInfo->strmBufPhyAddr;

	if (hInst->codecMode != MJPG_DEC) {
		val = VpuReadReg(BIT_BIT_STREAM_PARAM);
		val &= ~(1 << 2);
		/* clear stream end flag at start */
		VpuWriteReg(BIT_BIT_STREAM_PARAM, val);
		pDecInfo->streamEndflag = val;
	} else {
		pDecInfo->streamEndflag = 0;
		VpuWriteReg(MJPEG_BBC_BAS_ADDR_REG, pDecInfo->strmBufPhyAddr);
		VpuWriteReg(MJPEG_BBC_END_ADDR_REG, pDecInfo->strmBufPhyAddr +
			pDecInfo->strmBufSize);
		VpuWriteReg(MJPEG_BBC_STRM_CTRL_REG, 0);
	}

	*ppInst = hInst;

	NX_DbgMsg(INFO_MSG, ("===================================\n"));
	NX_DbgMsg(INFO_MSG, (" VPU Open Information:\n"));
	NX_DbgMsg(INFO_MSG, ("  Instance Index : %d\n", hInst->instIndex));
	NX_DbgMsg(INFO_MSG, ("  BitStream Mode : %d\n",
		pDecInfo->bitStreamMode));
	NX_DbgMsg(INFO_MSG, ("  Codec Standard : %d\n", hInst->codecMode));
	NX_DbgMsg(INFO_MSG, ("  Codec AUX Mode : %d\n", hInst->auxMode));
	NX_DbgMsg(INFO_MSG, ("===================================\n"));

	FUNC_OUT();
	return VPU_RET_OK;
}

int NX_VpuDecSetSeqInfo(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_seq_init_arg *pSeqArg)
{
	enum nx_vpu_ret ret;

	FUNC_IN();

	if (pInst->codecMode != MJPG_DEC) {
		/* FillBuffer */
		if (0 > FillBuffer(pInst, (uint8_t *)pSeqArg->seqData,
			pSeqArg->seqDataSize)) {
			NX_ErrMsg(("FillBuffer Error!!!\n"));
			return VPU_RET_ERROR;
		}

#if 0
		if (pInfo->bitStreamMode != BS_MODE_PIC_END) {
			int streamSize;

			if (pInfo->writePos > pInfo->readPos)
				streamSize = pInfo->writePos - pInfo->readPos;
			else
				streamSize = pInfo->strmBufSize -
					(pInfo->readPos - pInfo->writePos);

			if (streamSize < VPU_GBU_SIZE*2)
				return VPU_RET_NEED_STREAM;
		}
#endif

		ret = VPU_DecSeqInitCommand(pInst, pSeqArg);
		if (ret != VPU_RET_OK)
			return ret;
		ret = VPU_DecSeqComplete(pInst, pSeqArg);
	} else {
		ret = JPU_DecSetSeqInfo(pInst, pSeqArg);

		/* Fill Data */
		if (0 > FillBuffer(pInst, (uint8_t *)pSeqArg->seqData,
			pSeqArg->seqDataSize)) {
			NX_ErrMsg(("FillBuffer Failed.\n"));
			return VPU_RET_ERROR;
		}
	}

	FUNC_OUT();
	return ret;
}

int NX_VpuDecRegFrameBuf(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_reg_frame_arg *pFrmArg)
{
	if (pInst->codecMode != MJPG_DEC)
		return VPU_DecRegisterFrameBufCommand(pInst, pFrmArg);
	else
		return JPU_DecRegFrameBuf(pInst, pFrmArg);
}

int NX_VpuDecRunFrame(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_frame_arg *pRunArg)
{
	struct vpu_dec_info *pInfo = &pInst->codecInfo.decInfo;
	enum nx_vpu_ret ret;

	UNUSED(pInfo);

	if (pInst->codecMode != MJPG_DEC) {
		/* Fill Data */
		if (0 > FillBuffer(pInst, (uint8_t *)pRunArg->strmData,
			pRunArg->strmDataSize)) {
			NX_ErrMsg(("FillBuffer Failed.\n"));
			return VPU_RET_ERROR;
		}

#if 0
		if (pInfo->bitStreamMode != BS_MODE_PIC_END) {
			int streamSize;

			if (pInfo->writePos > pInfo->readPos)
				streamSize = pInfo->writePos - pInfo->readPos;
			else
				streamSize = pInfo->strmBufSize -
					(pInfo->readPos - pInfo->writePos);

			if (streamSize < VPU_GBU_SIZE*2) {
				pRunArg->indexFrameDecoded = -1;
				pRunArg->indexFrameDisplay = -1;
				return VPU_RET_NEED_STREAM;
			}
		}
#endif

		ret = VPU_DecStartOneFrameCommand(pInst, pRunArg);
		if (ret == VPU_RET_OK)
			ret = VPU_DecGetOutputInfo(pInst, pRunArg);
	} else {
		if (pInfo->headerSize == 0) {
			ret = JPU_DecParseHeader(pInfo,
				(uint8_t *)pRunArg->strmData,
				pRunArg->strmDataSize);
			if (ret < 0) {
				NX_ErrMsg(("JpgHeader is failed(Error = %d)!\n",
					ret));
				return -1;
			}
		}

		/* Fill Data */
		if (0 > FillBuffer(pInst, (uint8_t *)pRunArg->strmData,
			pRunArg->strmDataSize)) {
			NX_ErrMsg(("FillBuffer Failed.\n"));
			return VPU_RET_ERROR;
		}

		if (pInfo->validFlg > 0) {
			ret = JPU_DecRunFrame(pInst, pRunArg);
			pInfo->validFlg -= 1;
		} else {
			ret = -1;
		}
	}

	return ret;
}

int NX_VpuDecFlush(struct nx_vpu_codec_inst *pInst)
{
	unsigned int val;
	struct vpu_dec_info *pDecInfo = &pInst->codecInfo.decInfo;

	if (pInst->codecMode != MJPG_DEC) {
		val = pDecInfo->frameDisplayFlag;
		val &= ~pDecInfo->clearDisplayIndexes;
		VpuWriteReg(pDecInfo->frameDisplayFlagRegAddr, val);
		pDecInfo->clearDisplayIndexes = 0;
		pDecInfo->writePos = pDecInfo->readPos;
		VpuBitIssueCommand(pInst, DEC_BUF_FLUSH);

		if (VPU_RET_OK != VPU_WaitVpuBusy(VPU_BUSY_CHECK_TIMEOUT,
			BIT_BUSY_FLAG))
			return VPU_RET_ERR_TIMEOUT;

		pDecInfo->frameDisplayFlag = VpuReadReg(
			pDecInfo->frameDisplayFlagRegAddr);
		pDecInfo->frameDisplayFlag = 0;
		/* Clear End of Stream */
		pDecInfo->streamEndflag &= ~(1 << 2);
		pDecInfo->readPos = pDecInfo->strmBufPhyAddr;
		pDecInfo->writePos = pDecInfo->strmBufPhyAddr;
		NX_DrvMemset((unsigned char *)pDecInfo->strmBufVirAddr, 0,
			pDecInfo->strmBufSize);
	} else {
		int i;

		for (i = 0 ; i < pDecInfo->numFrameBuffer ; i++)
			pDecInfo->frmBufferValid[i] = 0;
	}

	return VPU_RET_OK;
}

int NX_VpuDecClrDspFlag(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_clr_dsp_flag_arg *pArg)
{
	struct vpu_dec_info *pInfo = &pInst->codecInfo.decInfo;

	if (pInst->codecMode != MJPG_DEC)
		pInfo->clearDisplayIndexes |= (1<<pArg->indexFrameDisplay);
	else
		pInfo->frmBufferValid[pArg->indexFrameDisplay] = 0;

	return VPU_RET_OK;
}

int NX_VpuDecClose(struct nx_vpu_codec_inst *pInst,
	void *vpu_event_present)
{
	enum nx_vpu_ret ret;

	if (pInst->codecMode == MJPG_DEC)
		return VPU_RET_OK;

	ret = VPU_DecCloseCommand(pInst, vpu_event_present);
	if (ret != VPU_RET_OK)
		NX_ErrMsg(("NX_VpuDecClose() failed.(%d)\n", ret));

	return ret;
}


/*----------------------------------------------------------------------------
 *		Decoder Specific Static Functions
 */

static int FillBuffer(struct nx_vpu_codec_inst *pInst, unsigned char *stream,
	int size)
{
	uint32_t vWriteOffset, vReadOffset;/* Virtual Read/Write Position */
	int32_t bufSize;
	struct vpu_dec_info *pDecInfo = &pInst->codecInfo.decInfo;

	/* EOS */
	if (size == 0)
		return 0;
	if (!stream || size < 0)
		return -1;

	if (pDecInfo->codecStd == CODEC_STD_MJPG) {
		stream += pDecInfo->headerSize;
		size -= pDecInfo->headerSize;

		pDecInfo->writePos = pDecInfo->strmBufPhyAddr;
		pDecInfo->readPos  = pDecInfo->strmBufPhyAddr;

		pDecInfo->validFlg += 1;
	}

	vWriteOffset = pDecInfo->writePos - pDecInfo->strmBufPhyAddr;
	vReadOffset = pDecInfo->readPos - pDecInfo->strmBufPhyAddr;
	bufSize  = pDecInfo->strmBufSize;

	if (bufSize < vWriteOffset || bufSize < vReadOffset) {
		NX_ErrMsg(("%s, stream_buffer(Addr=x0%08x, size=%d)\n",
			__func__, (uint32_t)pDecInfo->strmBufVirAddr,
			pDecInfo->strmBufSize));
		NX_ErrMsg(("InBuffer(Addr=%p, size=%d)", stream, size));
		NX_ErrMsg(("vWriteOffset = %d, vReadOffset = %d\n",
			vWriteOffset, vReadOffset));
		return -1;
	}

	if ((bufSize - vWriteOffset) > size) {
		/* Just Memory Copy */
#if 1
		NX_DrvMemcpy((uint8_t *)(pDecInfo->strmBufVirAddr +
			vWriteOffset), stream, size);
#else
		if (copy_from_user((uint8_t *)(pDecInfo->strmBufVirAddr +
			vWriteOffset), stream, size))
			return -1;
#endif
		vWriteOffset += size;
	} else {
		/* Memory Copy */
		int remain = bufSize - vWriteOffset;
#if 1
		NX_DrvMemcpy((uint8_t *)(pDecInfo->strmBufVirAddr +
			vWriteOffset), stream, remain);
		NX_DrvMemcpy((uint8_t *)(pDecInfo->strmBufVirAddr), stream +
			remain, size-remain);
#else
		if (copy_from_user((uint8_t *)(pDecInfo->strmBufVirAddr +
			vWriteOffset), stream, remain))
			return -1;
		if (copy_from_user((uint8_t *)(pDecInfo->strmBufVirAddr),
			stream+remain, size-remain))
			return -1;
#endif
		vWriteOffset = size-remain;
	}

	pDecInfo->writePos = vWriteOffset + pDecInfo->strmBufPhyAddr;
	return 0;
}

static int VPU_DecSeqComplete(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_seq_init_arg *pArg)
{
	unsigned int val, val2;
	int errReason;
	struct vpu_dec_info *pInfo = &pInst->codecInfo.decInfo;

	if (pInfo->bitStreamMode == BS_MODE_INTERRUPT && pInfo->seqInitEscape) {
		pInfo->streamEndflag &= ~(3<<3);
		VpuWriteReg(BIT_BIT_STREAM_PARAM, pInfo->streamEndflag);
		pInfo->seqInitEscape = 0;
	}

	pInfo->frameDisplayFlag = VpuReadReg(pInfo->frameDisplayFlagRegAddr);
	pInfo->readPos = VpuReadReg(pInfo->streamRdPtrRegAddr);
	pInfo->streamEndflag = VpuReadReg(BIT_BIT_STREAM_PARAM);

	errReason = 0;
	val = VpuReadReg(RET_DEC_SEQ_SUCCESS);
	if (val & (1<<31))
		return VPU_RET_ERR_MEM_ACCESS;

	if (pInfo->bitStreamMode == BS_MODE_PIC_END ||
		pInfo->bitStreamMode == BS_MODE_ROLLBACK) {
		if (val & (1<<4)) {
			errReason = VpuReadReg(RET_DEC_SEQ_SEQ_ERR_REASON);
			NX_ErrMsg(("Error Reason = 0x%08x\n", errReason));
			return VPU_RET_ERROR;
		}
	}

	if (val == 0) {
		errReason = VpuReadReg(RET_DEC_SEQ_SEQ_ERR_REASON);
		NX_ErrMsg(("Error Reason = 0x%08x\n", errReason));
		return VPU_RET_ERROR;
	}

	val = VpuReadReg(RET_DEC_SEQ_SRC_SIZE);
	pArg->outWidth  = ((val >> 16) & 0xffff);
	pArg->outHeight = (val & 0xffff);

	pArg->frameRateNum = VpuReadReg(RET_DEC_SEQ_FRATE_NR);
	pArg->frameRateDen = VpuReadReg(RET_DEC_SEQ_FRATE_DR);

	if (pInst->codecMode == AVC_DEC && pArg->frameRateDen > 0)
		pArg->frameRateDen  *= 2;

	if (pInst->codecMode  == MP4_DEC) {
		val = VpuReadReg(RET_DEC_SEQ_INFO);
		pArg->mp4ShortHeader = (val >> 2) & 1;
		pArg->mp4PartitionEnable = val & 1;
		pArg->mp4ReversibleVlcEnable =
			pArg->mp4PartitionEnable ?
			((val >> 1) & 1) : 0;
		pArg->h263AnnexJEnable = (val >> 3) & 1;
	} else if (pInst->codecMode == VPX_DEC && pInst->auxMode ==
		VPX_AUX_VP8) {
		/* h_scale[31:30] v_scale[29:28] pic_width[27:14]
		 * pic_height[13:0] */
		val = VpuReadReg(RET_DEC_SEQ_VP8_SCALE_INFO);
		pArg->vp8HScaleFactor = (val >> 30) & 0x03;
		pArg->vp8VScaleFactor = (val >> 28) & 0x03;
		pArg->vp8ScaleWidth = (val >> 14) & 0x3FFF;
		pArg->vp8ScaleHeight = (val >> 0) & 0x3FFF;
	}

	pArg->minFrameBufCnt = VpuReadReg(RET_DEC_SEQ_FRAME_NEED);
	pArg->frameBufDelay = VpuReadReg(RET_DEC_SEQ_FRAME_DELAY);

	if (pInst->codecMode == AVC_DEC || pInst->codecMode == MP2_DEC) {
		val  = VpuReadReg(RET_DEC_SEQ_CROP_LEFT_RIGHT);
		val2 = VpuReadReg(RET_DEC_SEQ_CROP_TOP_BOTTOM);
		if (val == 0 && val2 == 0) {
			pArg->cropLeft = 0;
			pArg->cropRight = pArg->outWidth;
			pArg->cropTop = 0;
			pArg->cropBottom = pArg->outHeight;
		} else {
			pArg->cropLeft = ((val>>16) & 0xFFFF);
			pArg->cropRight = pArg->outWidth - ((val & 0xFFFF));
			pArg->cropTop = ((val2>>16) & 0xFFFF);
			pArg->cropBottom = pArg->outHeight - ((val2 & 0xFFFF));
		}

		val = (pArg->outWidth * pArg->outHeight * 3 / 2) / 1024;
		pArg->numSliceSize = val / 4;
		pArg->worstSliceSize = val / 2;
	} else {
		pArg->cropLeft = 0;
		pArg->cropRight = pArg->outWidth;
		pArg->cropTop = 0;
		pArg->cropBottom = pArg->outHeight;
	}

	val = VpuReadReg(RET_DEC_SEQ_HEADER_REPORT);
	pArg->profile                =	(val >> 0) & 0xFF;
	pArg->level                  =	(val >> 8) & 0xFF;
	pArg->interlace              =	(val >> 16) & 0x01;
	pArg->direct8x8Flag          =	(val >> 17) & 0x01;
	pArg->vc1Psf                 =	(val >> 18) & 0x01;
	pArg->constraint_set_flag[0] =	(val >> 19) & 0x01;
	pArg->constraint_set_flag[1] =	(val >> 20) & 0x01;
	pArg->constraint_set_flag[2] =	(val >> 21) & 0x01;
	pArg->constraint_set_flag[3] =	(val >> 22) & 0x01;
	pArg->avcIsExtSAR            =  (val >> 25) & 0x01;
	pArg->maxNumRefFrmFlag       =  (val >> 31) & 0x01;

	pArg->aspectRateInfo = VpuReadReg(RET_DEC_SEQ_ASPECT);

	val = VpuReadReg(RET_DEC_SEQ_BIT_RATE);
	pArg->bitrate = val;

	if (pInst->codecMode == AVC_DEC) {
		val = VpuReadReg(RET_DEC_SEQ_VUI_INFO);
		pArg->vui_info.fixedFrameRateFlag    = val & 1;
		pArg->vui_info.timingInfoPresent     = (val>>1) & 0x01;
		pArg->vui_info.chromaLocBotField     = (val>>2) & 0x07;
		pArg->vui_info.chromaLocTopField     = (val>>5) & 0x07;
		pArg->vui_info.chromaLocInfoPresent  = (val>>8) & 0x01;
		pArg->vui_info.colorPrimaries        = (val>>16) & 0xff;
		pArg->vui_info.colorDescPresent      = (val>>24) & 0x01;
		pArg->vui_info.isExtSAR              = (val>>25) & 0x01;
		pArg->vui_info.vidFullRange          = (val>>26) & 0x01;
		pArg->vui_info.vidFormat             = (val>>27) & 0x07;
		pArg->vui_info.vidSigTypePresent     = (val>>30) & 0x01;
		pArg->vui_info.vuiParamPresent       = (val>>31) & 0x01;

		val = VpuReadReg(RET_DEC_SEQ_VUI_PIC_STRUCT);
		pArg->vui_info.vuiPicStructPresent = (val & 0x1);
		pArg->vui_info.vuiPicStruct = (val>>1);
	}

	if (pInst->codecMode == MP2_DEC) {
		/* seq_ext info */
		val = VpuReadReg(RET_DEC_SEQ_EXT_INFO);
		pArg->mp2LowDelay = val & 1;
		pArg->mp2DispVerSize = (val>>1) & 0x3fff;
		pArg->mp2DispHorSize = (val>>15) & 0x3fff;
	}

	pInfo->writePos = VpuReadReg(pInfo->streamWrPtrRegAddr);
	pInfo->readPos  = VpuReadReg(pInfo->streamRdPtrRegAddr);

	pArg->strmWritePos = pInfo->writePos - pInfo->strmBufPhyAddr;
	pArg->strmReadPos = pInfo->readPos - pInfo->strmBufPhyAddr;

	pInfo->width = pArg->outWidth;
	pInfo->height = pArg->outHeight;

	pInst->isInitialized = 1;

	return VPU_RET_OK;
}

static int VPU_DecSeqInitCommand(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_seq_init_arg *pArg)
{
	unsigned int val, reason;
	struct vpu_dec_info *pInfo = &pInst->codecInfo.decInfo;

	if (pArg->disableOutReorder) {
		NX_DbgMsg(INFO_MSG, ("Disable Out Reordering!!!\n"));
		pInfo->low_delay_info.lowDelayEn = 1;
		pInfo->low_delay_info.numRows = 0;
	}

	if (pInfo->needMoreFrame) {
		pInfo->needMoreFrame = 0;
		VpuWriteReg(pInfo->streamWrPtrRegAddr, pInfo->writePos);
		NX_DbgMsg(INFO_MSG, ("Need More Buffer!!!!!\n"));
		goto WAIT_INTERRUPT;
	}

	pInfo->enableMp4Deblock = pArg->enablePostFilter;

	VpuWriteReg(CMD_DEC_SEQ_BB_START, pInfo->strmBufPhyAddr);
	VpuWriteReg(CMD_DEC_SEQ_BB_SIZE, pInfo->strmBufSize / 1024);

#if (DBG_REGISTER)
	{
		int i;

		VpuWriteReg(BIT_BIT_STREAM_PARAM, 0);

		/* Clear Stream end flag */
		i = (int)VpuReadReg(BIT_BIT_STREAM_PARAM);
		if (i & (1 << (pInst->instIndex + 2)))
			i -= 1 << (pInst->instIndex + 2);
		VpuWriteReg(BIT_BIT_STREAM_PARAM, i);
	}
#endif

	if (pArg->enableUserData) {
		pInfo->userDataBufPhyAddr =
			(uint64_t)pArg->userDataBuffer.phyAddr;
		pInfo->userDataBufVirAddr =
			(uint64_t)pArg->userDataBuffer.virAddr;
		pInfo->userDataBufSize = pArg->userDataBuffer.size;
		pInfo->userDataEnable = 1;
		pInfo->userDataReportMode = 1;

		val  = 0;
		val |= (pInfo->userDataReportMode << 10);
		/*  val |= (pInfo->userDataEnable << 5); */
		VpuWriteReg(CMD_DEC_SEQ_USER_DATA_OPTION, val);
		VpuWriteReg(CMD_DEC_SEQ_USER_DATA_BASE_ADDR,
			pInfo->userDataBufPhyAddr);
		VpuWriteReg(CMD_DEC_SEQ_USER_DATA_BUF_SIZE,
			pInfo->userDataBufSize);
	} else {
		VpuWriteReg(CMD_DEC_SEQ_USER_DATA_OPTION, 0);
		VpuWriteReg(CMD_DEC_SEQ_USER_DATA_BASE_ADDR, 0);
		VpuWriteReg(CMD_DEC_SEQ_USER_DATA_BUF_SIZE, 0);
	}
	val  = 0;

	if (!pInfo->low_delay_info.lowDelayEn)
		val |= (pInfo->enableReordering<<1) & 0x2;

	val |= (pInfo->enableMp4Deblock & 0x1);

	/*Enable error conceal on missing reference in h.264/AVC */
	val |= (pInfo->avcErrorConcealMode << 2);

	VpuWriteReg(CMD_DEC_SEQ_OPTION, val);

	switch (pInst->codecMode) {
	case VC1_DEC:
		/*  VpuWriteReg(CMD_DEC_SEQ_VC1_STREAM_FMT, 1);
		VpuWriteReg(CMD_DEC_SEQ_VC1_STREAM_FMT, 2); */
		VpuWriteReg(CMD_DEC_SEQ_VC1_STREAM_FMT, (0 << 3) & 0x08);
		break;
	case MP4_DEC:
		VpuWriteReg(CMD_DEC_SEQ_MP4_ASP_CLASS,
			(VPU_GMC_PROCESS_METHOD<<3) | pInfo->mp4Class);
		break;
	case AVC_DEC:
		VpuWriteReg(CMD_DEC_SEQ_X264_MV_EN, VPU_AVC_X264_SUPPORT);
		break;
	}

	if (pInst->codecMode == AVC_DEC)
		VpuWriteReg(CMD_DEC_SEQ_SPP_CHUNK_SIZE, VPU_GBU_SIZE);

	VpuWriteReg(pInfo->streamWrPtrRegAddr, pInfo->writePos);
	VpuWriteReg(pInfo->streamRdPtrRegAddr, pInfo->readPos);

	/* Clear Stream Flag */
	pInfo->streamEndflag &= ~(1<<2);	/* Clear End of Stream */
	pInfo->streamEndflag &= ~(3<<3);	/* Clear Bitstream Mode */
	if (pInfo->bitStreamMode == BS_MODE_ROLLBACK) /*rollback mode */
		pInfo->streamEndflag |= (1<<3);
	else if (pInfo->bitStreamMode == BS_MODE_PIC_END)
		pInfo->streamEndflag |= (2<<3);
	else {	/* Interrupt Mode */
		if (pInfo->seqInitEscape)
			pInfo->streamEndflag |= (2<<3);
	}

	VpuWriteReg(BIT_BIT_STREAM_PARAM, pInfo->streamEndflag);
	VpuWriteReg(BIT_BIT_STREAM_CTRL, pInfo->streamEndian);

	val = 0;
	val |= (pInfo->bwbEnable<<12);
	val |= (pInfo->cbcrInterleave<<2);
	VpuWriteReg(BIT_FRAME_MEM_CTRL, val);

	if (pInst->codecMode != MJPG_DEC)
		VpuWriteReg(pInfo->frameDisplayFlagRegAddr, 0);

#if (DBG_REGISTER)
	{
		int reg;

		NX_DbgMsg(DBG_REGISTER, ("[DEC_SEQ_INIT]\n"));
		NX_DbgMsg(DBG_REGISTER, ("[Strm_CTRL : 0x10C]%x\n",
			VpuReadReg(BIT_BIT_STREAM_CTRL)));
		for (reg = 0x180 ; reg < 0x200 ; reg += 16) {
			NX_DbgMsg(DBG_REGISTER, ("[Addr = %3x]%x %x %x %x\n",
				reg, VpuReadReg(BIT_BASE + reg),
				VpuReadReg(BIT_BASE + reg + 4),
				VpuReadReg(BIT_BASE + reg + 8),
				VpuReadReg(BIT_BASE + reg + 12)));
		}
	}
#endif

	VpuBitIssueCommand(pInst, SEQ_INIT);

WAIT_INTERRUPT:
	reason = VPU_WaitBitInterrupt(pInst->devHandle, VPU_DEC_TIMEOUT);
	if (!reason) {
		NX_ErrMsg(("VPU_DecSeqInitCommand() Failed. Timeout(%d)\n",
			VPU_BUSY_CHECK_TIMEOUT));
		NX_ErrMsg(("WritePos = 0x%.8x, ReadPos = 0x%.8x\n",
			VpuReadReg(pInfo->streamWrPtrRegAddr),
			VpuReadReg(pInfo->streamRdPtrRegAddr)));
		return VPU_RET_ERR_TIMEOUT;
	}

	if (reason & (1<<VPU_INT_BIT_SEQ_INIT)) {
		return VPU_RET_OK;
	} else if (reason & (1<<VPU_INT_BIT_BIT_BUF_EMPTY)) {
		pInfo->needMoreFrame = 1;
		return VPU_RET_NEED_STREAM;
	} else {
		return VPU_RET_ERROR;
	}
}

static int VPU_DecRegisterFrameBufCommand(struct nx_vpu_codec_inst
	*pInst, struct vpu_dec_reg_frame_arg *pArg)
{
	struct vpu_dec_info *pInfo = &pInst->codecInfo.decInfo;
	unsigned char frameAddr[MAX_REG_FRAME][3][4];
	unsigned char colMvAddr[MAX_REG_FRAME][4];
	int bufferStride = pArg->frameBuffer[0].stride[0];
	unsigned int val, mvStartAddr;
	int i;

	FUNC_IN();

	NX_DrvMemset(frameAddr, 0, sizeof(frameAddr));
	NX_DrvMemset(colMvAddr, 0, sizeof(colMvAddr));

	pInfo->cbcrInterleave = pArg->chromaInterleave;
	pInfo->cacheConfig =
		MaverickCache2Config(1, pInfo->cbcrInterleave, 0, 0, 3, 0, 15);

	SetTiledMapType(VPU_LINEAR_FRAME_MAP, bufferStride,
		pInfo->cbcrInterleave);

	for (i = 0; i < pArg->numFrameBuffer; i++) {
		struct nx_vid_memory_info *frameBuffer = &pArg->frameBuffer[i];

		frameAddr[i][0][0] = (frameBuffer->phyAddr[0] >> 24) & 0xFF;
		frameAddr[i][0][1] = (frameBuffer->phyAddr[0] >> 16) & 0xFF;
		frameAddr[i][0][2] = (frameBuffer->phyAddr[0] >>  8) & 0xFF;
		frameAddr[i][0][3] = (frameBuffer->phyAddr[0] >>  0) & 0xFF;

		frameAddr[i][1][0] = (frameBuffer->phyAddr[1] >> 24) & 0xFF;
		frameAddr[i][1][1] = (frameBuffer->phyAddr[1] >> 16) & 0xFF;
		frameAddr[i][1][2] = (frameBuffer->phyAddr[1] >>  8) & 0xFF;
		frameAddr[i][1][3] = (frameBuffer->phyAddr[1] >>  0) & 0xFF;

		if (pInfo->cbcrInterleave == 0) {
			frameAddr[i][2][0] = (frameBuffer->phyAddr[2] >> 24)
				& 0xFF;
			frameAddr[i][2][1] = (frameBuffer->phyAddr[2] >> 16)
				& 0xFF;
			frameAddr[i][2][2] = (frameBuffer->phyAddr[2] >>  8)
				& 0xFF;
			frameAddr[i][2][3] = (frameBuffer->phyAddr[2] >>  0)
				& 0xFF;
		}
	}

	swap_endian((unsigned char *)frameAddr, sizeof(frameAddr));
	NX_DrvMemcpy((void *)pInst->paramVirAddr, frameAddr, sizeof(frameAddr));
	NX_DrvMemcpy(pInfo->frameBuffer, pArg->frameBuffer,
		sizeof(pArg->frameBuffer));

	mvStartAddr = pArg->colMvBuffer.phyAddr;

	/* MV allocation and registe */
	if (pInst->codecMode == AVC_DEC ||	pInst->codecMode == VC1_DEC ||
		pInst->codecMode == MP4_DEC || pInst->codecMode == RV_DEC  ||
		pInst->codecMode == AVS_DEC) {
		/*unsigned long   bufMvCol; */
		int size_mvcolbuf;

		mvStartAddr = pArg->colMvBuffer.phyAddr;

		if (pInst->codecMode == AVC_DEC ||
			pInst->codecMode == VC1_DEC ||
			pInst->codecMode == MP4_DEC ||
			pInst->codecMode == RV_DEC ||
			pInst->codecMode == AVS_DEC) {
			size_mvcolbuf =  ((pInfo->width+31)&~31) *
				((pInfo->height+31)&~31);
			size_mvcolbuf = (size_mvcolbuf*3) / 2;
			size_mvcolbuf = (size_mvcolbuf+4) / 5;
			size_mvcolbuf = ((size_mvcolbuf+7) / 8) * 8;

			if (pInst->codecMode == AVC_DEC) {
				for (i = 0; i < pArg->numFrameBuffer; i++) {
					colMvAddr[i][0] = (mvStartAddr >> 24)
						& 0xFF;
					colMvAddr[i][1] = (mvStartAddr >> 16)
						& 0xFF;
					colMvAddr[i][2] = (mvStartAddr >>  8)
						& 0xFF;
					colMvAddr[i][3] = (mvStartAddr >>  0)
						& 0xFF;
					mvStartAddr += size_mvcolbuf;
				}
			} else {
				colMvAddr[0][0] = (mvStartAddr >> 24) & 0xFF;
				colMvAddr[0][1] = (mvStartAddr >> 16) & 0xFF;
				colMvAddr[0][2] = (mvStartAddr >>  8) & 0xFF;
				colMvAddr[0][3] = (mvStartAddr >>  0) & 0xFF;
			}
		}
		swap_endian((unsigned char *)colMvAddr, sizeof(colMvAddr));
		NX_DrvMemcpy((void *)(pInst->paramVirAddr+384), colMvAddr,
			sizeof(colMvAddr));
	}

	if (!ConfigDecSecAXI(pInfo->codecStd, &pInfo->sec_axi_info,
		pInfo->width, pInfo->height, pArg->sramAddr, pArg->sramSize)) {
		NX_ErrMsg(("ConfigDecSecAXI() failed !!!\n"));
		NX_ErrMsg(("Width = %d, Heigth = %d\n",
			pInfo->width, pInfo->height));
		return VPU_RET_ERR_SRAM;
	}

	/* Tell the decoder how much frame buffers were allocated. */
	VpuWriteReg(CMD_SET_FRAME_BUF_NUM, pArg->numFrameBuffer);
	VpuWriteReg(CMD_SET_FRAME_BUF_STRIDE, bufferStride);
	VpuWriteReg(CMD_SET_FRAME_AXI_BIT_ADDR, pInfo->sec_axi_info.bufBitUse);
	VpuWriteReg(CMD_SET_FRAME_AXI_IPACDC_ADDR,
		pInfo->sec_axi_info.bufIpAcDcUse);
	VpuWriteReg(CMD_SET_FRAME_AXI_DBKY_ADDR,
		pInfo->sec_axi_info.bufDbkYUse);
	VpuWriteReg(CMD_SET_FRAME_AXI_DBKC_ADDR,
		pInfo->sec_axi_info.bufDbkCUse);
	VpuWriteReg(CMD_SET_FRAME_AXI_OVL_ADDR, pInfo->sec_axi_info.bufOvlUse);
	VpuWriteReg(CMD_SET_FRAME_AXI_BTP_ADDR, pInfo->sec_axi_info.bufBtpUse);
	VpuWriteReg(CMD_SET_FRAME_DELAY, pInfo->frameDelay);

	/* Maverick Cache Configuration */
	VpuWriteReg(CMD_SET_FRAME_CACHE_CONFIG, pInfo->cacheConfig);

	if (pInst->codecMode == VPX_DEC) {
		VpuWriteReg(CMD_SET_FRAME_MB_BUF_BASE,
			pArg->pvbSliceBuffer.phyAddr);
	}

	if (pInst->codecMode == AVC_DEC) {
		VpuWriteReg(CMD_SET_FRAME_SLICE_BB_START,
			pArg->sliceBuffer.phyAddr);
		VpuWriteReg(CMD_SET_FRAME_SLICE_BB_SIZE,
			pArg->sliceBuffer.size/1024);
	}

	val = 0;
	val |= (VPU_ENABLE_BWB<<12);
	val |= (pInfo->cbcrInterleave<<2);
	val |= VPU_FRAME_ENDIAN;
	VpuWriteReg(BIT_FRAME_MEM_CTRL, val);
	VpuWriteReg(CMD_SET_FRAME_MAX_DEC_SIZE, 0);

	VpuBitIssueCommand(pInst, SET_FRAME_BUF);

#if (DBG_REGISTER)
	{
		int reg;

		NX_DbgMsg(DBG_REGISTER, ("[DEC_SET_FRM_BUF_Reg]\n"));
		for (reg = 0x180 ; reg < 0x200 ; reg += 16) {
			NX_DbgMsg(DBG_REGISTER, ("[Addr = %3x]%x %x %x %x\n",
				reg, VpuReadReg(BIT_BASE + reg),
				VpuReadReg(BIT_BASE + reg + 4),
				VpuReadReg(BIT_BASE + reg + 8),
				VpuReadReg(BIT_BASE + reg + 12)));
		}
	}
#endif

	if (VPU_RET_OK != VPU_WaitVpuBusy(VPU_BUSY_CHECK_TIMEOUT,
		BIT_BUSY_FLAG)) {
		NX_ErrMsg(("Error VPU_DecRegisterFrameBufCommand failed!!!\n"));
		return VPU_RET_ERR_INIT;
	}

	if (VpuReadReg(RET_SET_FRAME_SUCCESS) & (1<<31))
			return VPU_RET_ERR_MEM_ACCESS;

	FUNC_OUT();
	return VPU_RET_OK;
}

static int VPU_DecGetOutputInfo(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_frame_arg *pArg)
{
	struct vpu_dec_info *pInfo = &pInst->codecInfo.decInfo;
	unsigned int val, val2;
	struct vpu_rect rectInfo;

	val = VpuReadReg(RET_DEC_PIC_SUCCESS);
	if (val & (1<<31)) {
		pArg->errReason  = VpuReadReg(GDI_WPROT_ERR_RSN);
		pArg->errAddress = VpuReadReg(GDI_WPROT_ERR_ADR);
		return VPU_RET_ERR_MEM_ACCESS;
	}

	if (pInst->codecMode == AVC_DEC) {
		pArg->notSufficientPsBuffer = (val >> 3) & 0x1;
		pArg->notSufficientSliceBuffer = (val >> 2) & 0x1;
	}

	pArg->indexFrameDecoded	= VpuReadReg(RET_DEC_PIC_DECODED_IDX);
	pArg->indexFrameDisplay	= VpuReadReg(RET_DEC_PIC_DISPLAY_IDX);

	val = VpuReadReg(RET_DEC_PIC_SIZE);/* decoding picture size */
	pArg->outWidth  = (val>>16) & 0xFFFF;
	pArg->outHeight = (val) & 0xFFFF;

	/*if (pArg->indexFrameDecoded >= 0 && pArg->indexFrameDecoded <
		MAX_REG_FRAME) */
	{
		if ((pInst->codecMode == VPX_DEC) && (pInst->auxMode ==
			VPX_AUX_VP8)) {
			/* VP8 specific header information
			 * h_scale[31:30] v_scale[29:28] pic_width[27:14]
			 * pic_height[13:0] */
			val = VpuReadReg(RET_DEC_PIC_VP8_SCALE_INFO);
			pArg->scale_info.hScaleFactor = (val >> 30) & 0x03;
			pArg->scale_info.vScaleFactor = (val >> 28) & 0x03;
			pArg->scale_info.picWidth = (val >> 14) & 0x3FFF;
			pArg->scale_info.picHeight = (val >> 0) & 0x3FFF;

			/* ref_idx_gold[31:24], ref_idx_altr[23:16],
			 * ref_idx_last[15: 8],
			 * version_number[3:1], show_frame[0] */
			val = VpuReadReg(RET_DEC_PIC_VP8_PIC_REPORT);
			pArg->pic_info.refIdxGold = (val >> 24) & 0x0FF;
			pArg->pic_info.refIdxAltr = (val >> 16) & 0x0FF;
			pArg->pic_info.refIdxLast = (val >> 8) & 0x0FF;
			pArg->pic_info.versionNumber = (val >> 1) & 0x07;
			pArg->pic_info.showFrame = (val >> 0) & 0x01;
		}

		/* default value */
		rectInfo.left   = 0;
		rectInfo.right  = pArg->outWidth;
		rectInfo.top    = 0;
		rectInfo.bottom = pArg->outHeight;

		if (pInst->codecMode == AVC_DEC ||
			pInst->codecMode == MP2_DEC) {
			val = VpuReadReg(RET_DEC_PIC_CROP_LEFT_RIGHT);
			val2 = VpuReadReg(RET_DEC_PIC_CROP_TOP_BOTTOM);

			if (val == (unsigned int)-1 || val == 0) {
				rectInfo.left  = 0;
				rectInfo.right = pArg->outWidth;
			} else {
				rectInfo.left  = ((val>>16) & 0xFFFF);
				rectInfo.right = pArg->outWidth - (val&0xFFFF);
			}

			if (val2 == (unsigned int)-1 || val2 == 0) {
				rectInfo.top    = 0;
				rectInfo.bottom = pArg->outHeight;
			} else {
				rectInfo.top    = ((val2>>16) & 0xFFFF);
				rectInfo.bottom	= pArg->outHeight -
					(val2&0xFFFF);
			}
		}

		pArg->outRect.left   = rectInfo.left;
		pArg->outRect.top    = rectInfo.top;
		pArg->outRect.right  = rectInfo.right;
		pArg->outRect.bottom = rectInfo.bottom;
	}

	val = VpuReadReg(RET_DEC_PIC_TYPE);
	pArg->isInterace = (val >> 18) & 0x1;

	if (pInst->codecMode == MP2_DEC) {
		pArg->progressiveFrame  = (val >> 23) & 0x0003;
		pArg->isInterace = (pArg->progressiveFrame == 0) ? (1) : (0);
		pArg->picStructure  = (val >> 19) & 0x0003;
	}

	if (pArg->isInterace) {
		pArg->topFieldFirst = (val >> 21) & 0x0001;
		pArg->picTypeFirst = (val & 0x38) >> 3;
		pArg->picType  = val & 0x7;
		pArg->npf = (val >> 15) & 1;
	} else {
		pArg->topFieldFirst     = 0;
		pArg->picTypeFirst   = 6;
		pArg->picType = val & 0x7;
	}

	if (pInst->codecMode == AVC_DEC) {
		if (val & 0x40) {
			if (pArg->isInterace) {
				pArg->picTypeFirst = 6;	/* IDR */
				pArg->picType = 6;
			} else {
				pArg->picType = 6;	/* IDR */
			}
		}
	}

	pArg->fRateNumerator = VpuReadReg(RET_DEC_PIC_FRATE_NR);
	pArg->fRateDenominator  = VpuReadReg(RET_DEC_PIC_FRATE_DR);
	if (pInst->codecMode == AVC_DEC && pArg->fRateDenominator > 0)
		pArg->fRateDenominator  *= 2;
	if (pInst->codecMode == MP4_DEC) {
		pArg->mp4ModuloTimeBase =
			VpuReadReg(RET_DEC_PIC_MODULO_TIME_BASE);
		pArg->mp4TimeIncrement =
			VpuReadReg(RET_DEC_PIC_VOP_TIME_INCREMENT);
	}

	if (pInst->codecMode == VPX_DEC)
		pArg->aspectRateInfo = 0;
	else
		pArg->aspectRateInfo = VpuReadReg(RET_DEC_PIC_ASPECT);

	pArg->numOfErrMBs = VpuReadReg(RET_DEC_PIC_ERR_MB);
	val = VpuReadReg(RET_DEC_PIC_SUCCESS);
	pArg->isSuccess	= val;
	pArg->sequenceChanged = ((val>>20) & 0x1);

	if (pArg->indexFrameDisplay >= 0 &&
		pArg->indexFrameDisplay < pInfo->numFrameBuffer) {
		pArg->outFrameBuffer =
			pInfo->frameBuffer[pArg->indexFrameDisplay];
	}

	if (pInst->codecMode == VC1_DEC && pArg->indexFrameDisplay != -3) {
		if (pInfo->vc1BframeDisplayValid == 0) {
			if (pArg->picType == 2)
				pArg->indexFrameDisplay = -3;
			else
				pInfo->vc1BframeDisplayValid = 1;
		}
	}

	if (pInfo->codecStd == CODEC_STD_VC1)
		pArg->multiRes = (VpuReadReg(RET_DEC_PIC_POST) >> 1) & 3;

	pInfo->readPos = VpuReadReg(pInfo->streamRdPtrRegAddr);
	pInfo->frameDisplayFlag = VpuReadReg(pInfo->frameDisplayFlagRegAddr);

	pInfo->bytePosFrameStart = VpuReadReg(BIT_BYTE_POS_FRAME_START);
	pInfo->bytePosFrameEnd = VpuReadReg(BIT_BYTE_POS_FRAME_END);

	pArg->strmReadPos = pInfo->readPos  - pInfo->strmBufPhyAddr;
	pArg->strmWritePos = pInfo->writePos - pInfo->strmBufPhyAddr;

	return VPU_RET_OK;
}

static int VPU_DecStartOneFrameCommand(struct nx_vpu_codec_inst
	*pInst, struct vpu_dec_frame_arg *pArg)
{
	struct vpu_dec_info *pInfo = &pInst->codecInfo.decInfo;
	unsigned int val, reason = 0;

	if (pInfo->needMoreFrame) {
		pInfo->needMoreFrame = 0;
		VpuWriteReg(pInfo->streamWrPtrRegAddr, pInfo->writePos);
		NX_DbgMsg(INFO_MSG, ("Need More Buffer!!!!!\n"));
		goto WAIT_INTERRUPT;
	}

	VpuWriteReg(RET_DEC_PIC_CROP_LEFT_RIGHT, 0);
	VpuWriteReg(RET_DEC_PIC_CROP_TOP_BOTTOM, 0);

	VpuWriteReg(GDI_TILEDBUF_BASE, 0);

	if ((pInfo->enableMp4Deblock & 2) &&
		((pInfo->codecStd == CODEC_STD_MPEG4) ||
		(pInfo->codecStd == CODEC_STD_MPEG2) ||
		(pInfo->codecStd == CODEC_STD_H263) ||
		(pInfo->codecStd == CODEC_STD_DIV3)))
		VpuWriteReg(CMD_DEC_PIC_ROT_MODE, 1 << 5);
	else
		VpuWriteReg(CMD_DEC_PIC_ROT_MODE, 0);

	if (pInfo->userDataEnable) {
		VpuWriteReg(CMD_DEC_PIC_USER_DATA_BASE_ADDR,
			pInfo->userDataBufPhyAddr);
		VpuWriteReg(CMD_DEC_PIC_USER_DATA_BUF_SIZE,
			pInfo->userDataBufSize);
	} else {
		VpuWriteReg(CMD_DEC_PIC_USER_DATA_BASE_ADDR, 0);
		VpuWriteReg(CMD_DEC_PIC_USER_DATA_BUF_SIZE, 0);
	}

	val = 0;
	/* if iframeSearch is Enable, other bit is ignore; */
	if (pArg->iFrameSearchEnable != 0) {
		val |= (pInfo->userDataReportMode << 10);
		if (pInst->codecMode == AVC_DEC) {
			if (pArg->iFrameSearchEnable == 1)
				val |= (1 << 11) | (1 << 2);
			else if (pArg->iFrameSearchEnable == 2)
				val |= (1 << 2);
		} else {
			val |= ((pArg->iFrameSearchEnable & 0x1) << 2);
		}
	} else {
		val |= (pInfo->userDataReportMode	<< 10);
		if (!pArg->skipFrameMode)
			val |= (pInfo->userDataEnable << 5);
		val |= (pArg->skipFrameMode << 3);
	}

	if (pInst->codecMode == AVC_DEC && pInfo->low_delay_info.lowDelayEn)
		val |= (pInfo->low_delay_info.lowDelayEn << 18);

	VpuWriteReg(CMD_DEC_PIC_OPTION, val);

	if (pInfo->low_delay_info.lowDelayEn)
		VpuWriteReg(CMD_DEC_PIC_NUM_ROWS,
			pInfo->low_delay_info.numRows);
	else
		VpuWriteReg(CMD_DEC_PIC_NUM_ROWS, 0);

	val = 0;
	val = (
		(pInfo->sec_axi_info.useBitEnable&0x01)<<0 |
		(pInfo->sec_axi_info.useIpEnable&0x01)<<1 |
		(pInfo->sec_axi_info.useDbkYEnable&0x01)<<2 |
		(pInfo->sec_axi_info.useDbkCEnable&0x01)<<3 |
		(pInfo->sec_axi_info.useOvlEnable&0x01)<<4 |
		(pInfo->sec_axi_info.useBtpEnable&0x01)<<5 |
		(pInfo->sec_axi_info.useBitEnable&0x01)<<8 |
		(pInfo->sec_axi_info.useIpEnable&0x01)<<9 |
		(pInfo->sec_axi_info.useDbkYEnable&0x01)<<10 |
		(pInfo->sec_axi_info.useDbkCEnable&0x01)<<11 |
		(pInfo->sec_axi_info.useOvlEnable&0x01)<<12 |
		(pInfo->sec_axi_info.useBtpEnable&0x01)<<13);

	VpuWriteReg(BIT_AXI_SRAM_USE, val);

	VpuWriteReg(pInfo->streamWrPtrRegAddr, pInfo->writePos);
	VpuWriteReg(pInfo->streamRdPtrRegAddr, pInfo->readPos);

	val = pInfo->frameDisplayFlag;
	val &= ~pInfo->clearDisplayIndexes;
	VpuWriteReg(pInfo->frameDisplayFlagRegAddr, val);
	pInfo->clearDisplayIndexes = 0;

	if (pArg->eos)
		pInfo->streamEndflag |= 1<<2;
	else
		pInfo->streamEndflag &= ~(1<<2);

	pInfo->streamEndflag &= ~(3<<3);
	if (pInfo->bitStreamMode == BS_MODE_ROLLBACK)
		pInfo->streamEndflag |= (1<<3);
	else if (pInfo->bitStreamMode == BS_MODE_PIC_END)
		pInfo->streamEndflag |= (2<<3);

	VpuWriteReg(BIT_BIT_STREAM_PARAM, pInfo->streamEndflag);

	val = 0;
	val |= (pInfo->bwbEnable<<12);
	val |= ((pInfo->cbcrInterleave)<<2);
	/* val |= pInfo->frameEndian; */
	VpuWriteReg(BIT_FRAME_MEM_CTRL, val);

	val = pInfo->streamEndian;
	VpuWriteReg(BIT_BIT_STREAM_CTRL, val);

#if (DBG_REGISTER)
	{
		int reg;

		NX_DbgMsg(DBG_REGISTER, ("[DEC_FRAME]\n"));
		for (reg = 0x180 ; reg < 0x200 ; reg += 16) {
			NX_DbgMsg(DBG_REGISTER, ("[Addr = %3x]%x %x %x %x\n",
				reg, VpuReadReg(BIT_BASE + reg),
				VpuReadReg(BIT_BASE + reg + 4),
				VpuReadReg(BIT_BASE + reg + 8),
				VpuReadReg(BIT_BASE + reg + 12)));
		}
	}
#endif

	VpuBitIssueCommand(pInst, PIC_RUN);

WAIT_INTERRUPT:
	reason = VPU_WaitBitInterrupt(pInst->devHandle, VPU_DEC_TIMEOUT);
	if (!reason) {
		NX_ErrMsg(("VPU_DecStartOneFrameCommand() Fail. Timeout(%d)\n",
			VPU_BUSY_CHECK_TIMEOUT));
		return VPU_RET_ERR_TIMEOUT;
	}

	if (reason & (1<<VPU_INT_BIT_PIC_RUN)) {
		return VPU_RET_OK;
	} else if (reason &  (1<<VPU_INT_BIT_BIT_BUF_EMPTY)) {
		pInfo->needMoreFrame = 1;
		return VPU_RET_NEED_STREAM;
	} else {
		return VPU_RET_ERROR;
	}
}

static int VPU_DecCloseCommand(struct nx_vpu_codec_inst *pInst,
	void *vpu_event_present)
{
	FUNC_IN();
	if (pInst->isInitialized) {
		VpuBitIssueCommand(pInst, SEQ_END);
		if (VPU_RET_OK != VPU_WaitVpuBusy(VPU_BUSY_CHECK_TIMEOUT,
			BIT_BUSY_FLAG)) {
			VpuWriteReg(BIT_INT_CLEAR, 0x1);
			atomic_set((atomic_t *)vpu_event_present, 0);
			NX_ErrMsg(("VPU_DecCloseCommand() Failed!!!\n"));
			NX_ErrMsg(("Timeout(%d)\n", VPU_BUSY_CHECK_TIMEOUT));
			VPU_SWReset(SW_RESET_SAFETY);
			pInst->isInitialized = 0;
			return VPU_RET_ERR_TIMEOUT;
		}
		pInst->isInitialized = 0;
	}

	VpuWriteReg(BIT_INT_CLEAR, 0x1);
	atomic_set((atomic_t *)vpu_event_present, 0);
	FUNC_OUT();
	return VPU_RET_OK;
}
