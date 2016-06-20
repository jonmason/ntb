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


#define DBG_REGISTER		0
#define DBG_ES_ADDR		0
#define INFO_MSG		0


/*--------------------------------------------------------------------------- */
/*  Define Static Functions */
static void VPU_EncDefaultParam(struct vpu_enc_info *pInfo);
static int VPU_EncSeqCommand(struct nx_vpu_codec_inst *pInst);
static int VPU_EncSetFrameBufCommand(struct nx_vpu_codec_inst
	*pInst, uint32_t sramAddr, uint32_t sramSize);
static int VPU_EncOneFrameCommand(struct nx_vpu_codec_inst *pInst,
	struct vpu_enc_run_frame_arg *runArg);
static int VPU_EncChangeParameterCommand(struct nx_vpu_codec_inst
	*pInst, struct vpu_enc_chg_para_arg *chgArg);
static int VPU_EncGetHeaderCommand(struct nx_vpu_codec_inst *pInst,
	unsigned int headerType, void **ptr, int *size);
static int VPU_EncCloseCommand(struct nx_vpu_codec_inst *pInst,
	void *vpu_event_present);


/*----------------------------------------------------------------------------
 *			Encoder APIs
 */

int NX_VpuEncOpen(struct vpu_open_arg *pOpenArg, void *devHandle,
	struct nx_vpu_codec_inst **ppInst)
{
	struct vpu_enc_info *pEncInfo;
	struct nx_vpu_codec_inst *hInst = 0;

	*ppInst = 0;
	if (!NX_VpuIsInitialized())
		return VPU_RET_ERR_INIT;

	hInst = NX_VpuGetInstance(pOpenArg->instIndex);
	if (!hInst)
		return VPU_RET_ERR_INST;

	if (CODEC_STD_AVC == pOpenArg->codecStd) {
		hInst->codecMode = AVC_ENC;
		hInst->auxMode = 0;
	} else if (CODEC_STD_MPEG4 == pOpenArg->codecStd) {
		hInst->codecMode = MP4_ENC;
		hInst->auxMode = 0;
	} else if (CODEC_STD_H263 == pOpenArg->codecStd) {
		hInst->codecMode = MP4_ENC;
		hInst->auxMode = 1;
	} else if (CODEC_STD_MJPG == pOpenArg->codecStd) {
		hInst->codecMode = MJPG_ENC;
		hInst->auxMode = 0;
	} else {
		NX_ErrMsg(("NX_VpuEncOpen() failed!!!\n"));
		NX_ErrMsg(("Cannot support codec standard(%d)\n",
			pOpenArg->codecStd));
		return VPU_RET_ERR_PARAM;
	}

	NX_DrvMemset(&hInst->codecInfo, 0, sizeof(hInst->codecInfo));

	hInst->inUse = 1;
	hInst->instIndex = pOpenArg->instIndex;
	hInst->devHandle = devHandle;

	hInst->instBufPhyAddr = (uint64_t)pOpenArg->instanceBuf.phyAddr;
	hInst->instBufVirAddr = (uint64_t)pOpenArg->instanceBuf.virAddr;
	hInst->instBufSize    = pOpenArg->instanceBuf.size;

	pEncInfo = &hInst->codecInfo.encInfo;
	pEncInfo->codecStd = pOpenArg->codecStd;

	VPU_EncDefaultParam(pEncInfo);

	*ppInst = hInst;
	return VPU_RET_OK;
}

int NX_VpuEncClose(struct nx_vpu_codec_inst *pInst, void *vpu_event_present)
{
	enum nx_vpu_ret ret;

	if (MJPG_ENC == pInst->codecMode)
		return VPU_RET_OK;

	ret = VPU_EncCloseCommand(pInst, vpu_event_present);
	if (ret != VPU_RET_OK)
		NX_ErrMsg(("NX_VpuEncClose() failed.(%d)\n", ret));

	return ret;
}

int NX_VpuEncSetSeqParam(struct nx_vpu_codec_inst *pInst,
	struct vpu_enc_seq_arg *pSeqArg)
{
	struct vpu_enc_info *pEncInfo = &pInst->codecInfo.encInfo;

	pEncInfo->srcWidth = pSeqArg->srcWidth;
	pEncInfo->srcHeight = pSeqArg->srcHeight;
	pEncInfo->encWidth = pSeqArg->srcWidth;
	pEncInfo->encHeight = pSeqArg->srcHeight;
	pEncInfo->gopSize = pSeqArg->gopSize;
	pEncInfo->frameRateNum = pSeqArg->frameRateNum;
	pEncInfo->frameRateDen = pSeqArg->frameRateDen;

	pEncInfo->rcEnable = (pSeqArg->bitrate) ? (1) : (0);
	pEncInfo->bitRate = (pSeqArg->bitrate/1024)&0x7FFF;
	pEncInfo->userQpMax = pSeqArg->maxQP;
	pEncInfo->enableAutoSkip = !pSeqArg->disableSkip;
	pEncInfo->initialDelay = pSeqArg->initialDelay;
	pEncInfo->vbvBufSize = pSeqArg->vbvBufferSize;
	pEncInfo->userGamma = pSeqArg->gammaFactor;
	pEncInfo->frameQp = pSeqArg->initQP;

	pEncInfo->MESearchRange = pSeqArg->searchRange;
	pEncInfo->intraRefresh = pSeqArg->intraRefreshMbs;

	pEncInfo->cbcrInterleave = pSeqArg->chromaInterleave;
	pEncInfo->cbcrInterleaveRefFrame = pSeqArg->refChromaInterleave;

	pEncInfo->rotateAngle = pSeqArg->rotAngle / 90;
	pEncInfo->mirrorDirection = pSeqArg->mirDirection;

	pEncInfo->strmBufVirAddr = pSeqArg->strmBufVirAddr;
	pEncInfo->strmBufPhyAddr = pSeqArg->strmBufPhyAddr;
	pEncInfo->strmBufSize = pSeqArg->strmBufSize;

	pEncInfo->jpegQuality = pSeqArg->quality;

	if (pSeqArg->annexFlg) {
		pEncInfo->enc_codec_para.h263EncParam.h263AnnexJEnable = 1;
		pEncInfo->enc_codec_para.h263EncParam.h263AnnexTEnable = 1;
	}

	if (pSeqArg->enableAUDelimiter)
		pEncInfo->enc_codec_para.avcEncParam.audEnable = 1;

	NX_DbgMsg(INFO_MSG, ("NX_VpuEncSetSeqParam() information\n"));
	NX_DbgMsg(INFO_MSG, ("Reloution : %d x %d\n",
		pEncInfo->srcWidth, pEncInfo->srcHeight));
	NX_DbgMsg(INFO_MSG, ("Fps : %d/%d\n",
		pEncInfo->frameRateNum, pEncInfo->frameRateDen));
	NX_DbgMsg(INFO_MSG, ("Target bitrate : %d kbps\n", pEncInfo->bitRate));
	NX_DbgMsg(INFO_MSG, ("GOP : %d\n", pEncInfo->gopSize));
	NX_DbgMsg(INFO_MSG, ("Max QP : %d\n", pEncInfo->userQpMax));
	NX_DbgMsg(INFO_MSG, ("SR : %d\n", pEncInfo->MESearchRange));
	NX_DbgMsg(INFO_MSG, ("Stream_buffer : 0x%llx, 0x%llx))\n",
		pEncInfo->strmBufPhyAddr, pEncInfo->strmBufVirAddr));

	if (CODEC_STD_MJPG == pEncInfo->codecStd) {
		struct enc_jpeg_info *pJpgInfo =
			&pEncInfo->enc_codec_para.jpgEncInfo;

		if (0 != pEncInfo->rotateAngle) {
			pJpgInfo->rotationEnable = 1;
			pJpgInfo->rotationAngle = pEncInfo->rotateAngle;
		}

		if (0 != pEncInfo->mirrorDirection) {
			pJpgInfo->mirrorEnable = 1;
			pJpgInfo->mirrorDirection = pEncInfo->mirrorDirection;
		}

		if (pSeqArg->quality == 0 || pSeqArg->quality < 0 ||
			pSeqArg->quality > 100)
			pEncInfo->jpegQuality = 90;
		else
			pEncInfo->jpegQuality = pSeqArg->quality;

		pJpgInfo->format = pSeqArg->imgFormat;
		pJpgInfo->picWidth = pEncInfo->srcWidth;
		pJpgInfo->picHeight = pEncInfo->srcHeight;

		return VPU_RET_OK;
	}

	return VPU_EncSeqCommand(pInst);
}

/* Frame Buffer Address Allocation */
int NX_VpuEncSetFrame(struct nx_vpu_codec_inst *pInst,
	struct vpu_enc_set_frame_arg *pFrmArg)
{
	int i;
	struct vpu_enc_info *pEncInfo = &pInst->codecInfo.encInfo;

	if (CODEC_STD_MJPG == pEncInfo->codecStd)
		return VPU_RET_OK;

	pEncInfo->minFrameBuffers = pFrmArg->numFrameBuffer;
	for (i = 0 ; i < pEncInfo->minFrameBuffers ; i++)
		pEncInfo->frameBuffer[i] = pFrmArg->frameBuffer[i];

	pEncInfo->subSampleAPhyAddr = pFrmArg->subSampleBuffer[0].phyAddr;
	pEncInfo->subSampleBPhyAddr = pFrmArg->subSampleBuffer[1].phyAddr;

	return VPU_EncSetFrameBufCommand(pInst, pFrmArg->sramAddr,
		pFrmArg->sramSize);
}

int NX_VpuEncGetHeader(struct nx_vpu_codec_inst *pInst,
	union vpu_enc_get_header_arg *pHeader)
{
	enum nx_vpu_ret ret = VPU_RET_OK;
	void *ptr;
	int size;

	if (AVC_ENC == pInst->codecMode) {
		/* SPS */
		ret = VPU_EncGetHeaderCommand(pInst, SPS_RBSP, &ptr, &size);
		if (ret != VPU_RET_OK) {
			NX_ErrMsg(("NX_VpuEncGetHeader() SPS_RBSP Error!\n"));
			goto GET_HEADER_EXIT;
		}
		NX_DrvMemcpy(pHeader->avcHeader.spsData, ptr, size);
		pHeader->avcHeader.spsSize = size;
		/* PPS */
		ret = VPU_EncGetHeaderCommand(pInst, PPS_RBSP, &ptr, &size);
		if (ret != VPU_RET_OK) {
			NX_ErrMsg(("NX_VpuEncGetHeader() PPS_RBSP Error!\n"));
			goto GET_HEADER_EXIT;
		}
		NX_DrvMemcpy(pHeader->avcHeader.ppsData, ptr, size);
		pHeader->avcHeader.ppsSize = size;
	} else if (MP4_ENC == pInst->codecMode) {
		ret = VPU_EncGetHeaderCommand(pInst, VOS_HEADER, &ptr, &size);
		/* VOS */
		if (ret != VPU_RET_OK) {
			NX_ErrMsg(("NX_VpuEncGetHeader() VOS_HEADER Error!\n"));
			goto GET_HEADER_EXIT;
		}
		NX_DrvMemcpy(pHeader->mp4Header.vosData, ptr, size);
		pHeader->mp4Header.vosSize = size;
		/* VOL */
		ret = VPU_EncGetHeaderCommand(pInst, VOL_HEADER, &ptr, &size);
		if (ret != VPU_RET_OK) {
			NX_ErrMsg(("NX_VpuEncGetHeader() VOL_HEADER Error!\n"));
			goto GET_HEADER_EXIT;
		}
		NX_DrvMemcpy(pHeader->mp4Header.volData, ptr, size);
		pHeader->mp4Header.volSize = size;
	}

GET_HEADER_EXIT:
	return ret;
}

int NX_VpuEncRunFrame(struct nx_vpu_codec_inst *pInst,
	struct vpu_enc_run_frame_arg *pRunArg)
{
	if (pInst->codecMode != MJPG_ENC)
		return VPU_EncOneFrameCommand(pInst, pRunArg);
	else
		return JPU_EncRunFrame(pInst, pRunArg);
}

int NX_VpuEncChgParam(struct nx_vpu_codec_inst *pInst,
	struct vpu_enc_chg_para_arg *pChgArg)
{
	if (pInst->codecMode != MJPG_ENC)
		return VPU_EncChangeParameterCommand(pInst, pChgArg);
	else
		return -1;
}


/*---------------------------------------------------------------------------
 * Encoder Specific Static Functions
 */

static void VPU_EncDefaultParam(struct vpu_enc_info *pEncInfo)
{
	/* Set Default Frame Rate */
	pEncInfo->frameRateNum = 30;
	pEncInfo->frameRateDen = 1;

	/* Set Slice Mode */
	pEncInfo->sliceMode = 0;		/* one slice per picture */
	pEncInfo->sliceSizeMode = 0;
	pEncInfo->sliceSize = 0;

	/* Set GOP Size */
	pEncInfo->gopSize = 30;

	/* Rate Control Related */
	pEncInfo->rcEnable = 0;
	pEncInfo->intraRefresh = 0;	        /* Disalbe Intra Refresh */
	pEncInfo->rcIntraQp = -1;		/* Disable */

	pEncInfo->bwbEnable = VPU_ENABLE_BWB;
	pEncInfo->cbcrInterleave = CBCR_INTERLEAVE;
	pEncInfo->cbcrInterleaveRefFrame = ENC_FRAME_BUF_CBCR_INTERLEAVE;
	pEncInfo->frameEndian = VPU_FRAME_ENDIAN;

	pEncInfo->rcIntervalMode = 0;	        /* Frame Mode */
	pEncInfo->rcIntraCostWeigth = 400;
	pEncInfo->enableAutoSkip = 0;
	pEncInfo->initialDelay = 0;
	pEncInfo->vbvBufSize = 0;

	/* (0*32768 < gamma < 1*32768) */
	pEncInfo->userGamma = (unsigned int)(0.75*32768);

	if (CODEC_STD_AVC == pEncInfo->codecStd) {
		struct enc_avc_param *pAvcParam =
			&pEncInfo->enc_codec_para.avcEncParam;

		pAvcParam->chromaQpOffset = 0;
		pAvcParam->constrainedIntraPredFlag = 0;
		pAvcParam->disableDeblk = 0;
		pAvcParam->deblkFilterOffsetAlpha = 0;
		pAvcParam->audEnable = 0;

		pEncInfo->userQpMax = 51;
	} else if (CODEC_STD_MPEG4 == pEncInfo->codecStd) {
		struct enc_mp4_param *pMp4Param =
			&pEncInfo->enc_codec_para.mp4EncParam;
		pMp4Param->mp4DataPartitionEnable = 0;
		pMp4Param->mp4ReversibleVlcEnable = 0;
		pMp4Param->mp4IntraDcVlcThr = 0;
		pMp4Param->mp4HecEnable	= 0;
		pMp4Param->mp4Verid = 2;

		pEncInfo->userQpMax = 31;
	} else if (CODEC_STD_H263 == pEncInfo->codecStd) {
		struct enc_h263_param *pH263Param =
			&pEncInfo->enc_codec_para.h263EncParam;
		pH263Param->h263AnnexIEnable = 0;
		pH263Param->h263AnnexJEnable = 0;
		pH263Param->h263AnnexKEnable = 0;
		pH263Param->h263AnnexTEnable = 0;

		pEncInfo->userQpMax = 31;
	} else if (CODEC_STD_MJPG == pEncInfo->codecStd) {
		struct enc_jpeg_info *pJpgInfo =
			&pEncInfo->enc_codec_para.jpgEncInfo;
		pJpgInfo->format = IMG_FORMAT_420;
		pJpgInfo->frameIdx = 0;
		pJpgInfo->rstIntval = 60;
	}

	/* Motion Estimation */
	pEncInfo->MEUseZeroPmv = 0;
	pEncInfo->MESearchRange = 1;
	pEncInfo->MEBlockMode = 0;	/* Use All Macro Block Type */
}

static int VPU_EncSeqCommand(struct nx_vpu_codec_inst *pInst)
{
	struct vpu_enc_info *pEncInfo = &pInst->codecInfo.encInfo;
	unsigned int tmpData;

	/* Write Bitstream Buffer Information */
	VpuWriteReg(CMD_ENC_SEQ_BB_START, pEncInfo->strmBufPhyAddr);
	VpuWriteReg(CMD_ENC_SEQ_BB_SIZE, pEncInfo->strmBufSize / 1024);

	/* Set Source Image Information */
	if (90 == pEncInfo->rotateAngle || 270 == pEncInfo->rotateAngle)
		tmpData = (pEncInfo->srcHeight << 16) | pEncInfo->srcWidth;
	else
		tmpData = (pEncInfo->srcWidth << 16) | pEncInfo->srcHeight;

	VpuWriteReg(CMD_ENC_SEQ_SRC_SIZE, tmpData);
	VpuWriteReg(CMD_ENC_SEQ_SRC_F_RATE, (pEncInfo->frameRateNum) |
		((pEncInfo->frameRateDen-1) << 16));

	if (pEncInfo->codecStd == CODEC_STD_MPEG4) {
		struct enc_mp4_param *pMp4Param =
			&pEncInfo->enc_codec_para.mp4EncParam;
		VpuWriteReg(CMD_ENC_SEQ_COD_STD, 3);
		tmpData = pMp4Param->mp4IntraDcVlcThr << 2 |
			      pMp4Param->mp4ReversibleVlcEnable << 1 |
			      pMp4Param->mp4DataPartitionEnable;
		tmpData |= ((pMp4Param->mp4HecEnable > 0) ? 1:0)<<5;
		tmpData |= ((pMp4Param->mp4Verid == 2) ? 0:1) << 6;
		VpuWriteReg(CMD_ENC_SEQ_MP4_PARA, tmpData);
	} else if (pEncInfo->codecStd == CODEC_STD_H263) {
		struct enc_h263_param *pH263Param =
			&pEncInfo->enc_codec_para.h263EncParam;

		VpuWriteReg(CMD_ENC_SEQ_COD_STD, 11);
		tmpData = pH263Param->h263AnnexIEnable << 3 |
			pH263Param->h263AnnexJEnable << 2 |
			pH263Param->h263AnnexKEnable << 1|
			pH263Param->h263AnnexTEnable;
		VpuWriteReg(CMD_ENC_SEQ_263_PARA, tmpData);
	} else if (pEncInfo->codecStd == CODEC_STD_AVC) {
		struct enc_avc_param *pAvcParam =
			&pEncInfo->enc_codec_para.avcEncParam;
		VpuWriteReg(CMD_ENC_SEQ_COD_STD, 0x0);
		tmpData = (pAvcParam->deblkFilterOffsetBeta & 15) << 12
			| (pAvcParam->deblkFilterOffsetAlpha & 15) << 8
			| pAvcParam->disableDeblk << 6
			| pAvcParam->constrainedIntraPredFlag << 5
			| (pAvcParam->chromaQpOffset & 31);
		VpuWriteReg(CMD_ENC_SEQ_264_PARA, tmpData);
	}

	/* Slice Mode */
	tmpData  = pEncInfo->sliceSize << 2 |  pEncInfo->sliceSizeMode << 1
			| pEncInfo->sliceMode;
	VpuWriteReg(CMD_ENC_SEQ_SLICE_MODE, tmpData);

	/* Write GOP Size */
	if (pEncInfo->rcEnable)
		VpuWriteReg(CMD_ENC_SEQ_GOP_NUM, pEncInfo->gopSize);
	else
		VpuWriteReg(CMD_ENC_SEQ_GOP_NUM, 0);

	/* Rate Control */
	if (pEncInfo->rcEnable) {
		tmpData = (!pEncInfo->enableAutoSkip) << 31 |
		pEncInfo->initialDelay << 16 | pEncInfo->bitRate<<1 | 1;
		VpuWriteReg(CMD_ENC_SEQ_RC_PARA, tmpData);
	} else {
		VpuWriteReg(CMD_ENC_SEQ_RC_PARA, 0);
	}

	VpuWriteReg(CMD_ENC_SEQ_RC_BUF_SIZE, pEncInfo->vbvBufSize);
	VpuWriteReg(CMD_ENC_SEQ_INTRA_REFRESH, pEncInfo->intraRefresh);

	if (pEncInfo->rcIntraQp >= 0) {
		tmpData = (1 << 5);
		VpuWriteReg(CMD_ENC_SEQ_INTRA_QP, pEncInfo->rcIntraQp);
	} else {
		tmpData = 0;
		VpuWriteReg(CMD_ENC_SEQ_INTRA_QP, (unsigned int)-1);
	}

	if (pEncInfo->codecStd == CODEC_STD_AVC)
		tmpData |= (pEncInfo->enc_codec_para.avcEncParam.audEnable<<2);

	if (pEncInfo->userQpMax >= 0) {
		tmpData |= (1<<6);
		VpuWriteReg(CMD_ENC_SEQ_RC_QP_MAX, pEncInfo->userQpMax);
	} else {
		VpuWriteReg(CMD_ENC_SEQ_RC_QP_MAX, 0);
	}

	if (pEncInfo->userGamma >= 0) {
		tmpData |= (1<<7);
		VpuWriteReg(CMD_ENC_SEQ_RC_GAMMA, pEncInfo->userGamma);
	} else {
		VpuWriteReg(CMD_ENC_SEQ_RC_GAMMA, 0);
	}

	VpuWriteReg(CMD_ENC_SEQ_OPTION, tmpData);

	VpuWriteReg(CMD_ENC_SEQ_RC_INTERVAL_MODE, (pEncInfo->mbInterval<<2)
			| pEncInfo->rcIntervalMode);
	VpuWriteReg(CMD_ENC_SEQ_INTRA_WEIGHT, pEncInfo->rcIntraCostWeigth);
	VpuWriteReg(CMD_ENC_SEQ_ME_OPTION, (pEncInfo->MEBlockMode << 3)
		| (pEncInfo->MEUseZeroPmv << 2) | pEncInfo->MESearchRange);

	VpuWriteReg(BIT_WR_PTR, pEncInfo->strmBufPhyAddr);
	VpuWriteReg(BIT_RD_PTR, pEncInfo->strmBufPhyAddr);

	tmpData = 0;
	tmpData |= (pEncInfo->bwbEnable<<12)|(pEncInfo->cbcrInterleave<<2);
	tmpData |= pEncInfo->frameEndian;
	VpuWriteReg(BIT_FRAME_MEM_CTRL, tmpData);

	/* Ring Buffer Disable */
	if (pEncInfo->ringBufferEnable == 0) {
		/* Line Buffer Interrupt Enable */
		tmpData  = (0x1<<6);
		/* The value of 1 means that bitstream buffer is reset at every
		picture encoding command. */
		tmpData |= (0x1<<5);
		/* Enables dynamic picture stream buffer allocation in encoder
		operations. */
		tmpData |= (0x1<<4);
		tmpData |= VPU_STREAM_ENDIAN;
	} else {
		/* Ring Buffer Enabled */
		tmpData  = (0x1<<3);
		tmpData |= VPU_STREAM_ENDIAN;
	}

	VpuWriteReg(BIT_BIT_STREAM_CTRL, tmpData);

#if (DBG_REGISTER)
	{
		int reg;

		NX_DbgMsg(DBG_REGISTER, ("[SEQ_INIT_Reg]\n"));
		for (reg = 0x0 ; reg < 0x200 ; reg += 16) {
			NX_DbgMsg(DBG_REGISTER, ("[Addr = %3x]%x %x %x %x\n",
				reg, VpuReadReg(BIT_BASE + reg),
				VpuReadReg(BIT_BASE + reg + 4),
				VpuReadReg(BIT_BASE + reg + 8),
				VpuReadReg(BIT_BASE + reg + 12)));
		}
	}
#endif


	VpuBitIssueCommand(pInst, SEQ_INIT);

#if (DBG_REGISTER)
	{
		int reg;

		NX_DbgMsg(DBG_REGISTER, ("[SEQ_INIT_Reg]\n"));
		for (reg = 0x0 ; reg < 0x200 ; reg += 16) {
			NX_DbgMsg(DBG_REGISTER, ("[Addr = %3x]%x %x %x %x\n",
				reg, VpuReadReg(BIT_BASE + reg),
				VpuReadReg(BIT_BASE + reg + 4),
				VpuReadReg(BIT_BASE + reg + 8),
				VpuReadReg(BIT_BASE + reg + 12)));
		}
	}
#endif

	if (!VPU_WaitBitInterrupt(pInst->devHandle, VPU_DEC_TIMEOUT)) {
		NX_ErrMsg(("VPU_EncSeqCommand() Failed. Timeout(%d)\n",
			VPU_BUSY_CHECK_TIMEOUT));
		return VPU_RET_ERR_TIMEOUT;
	}

	/* Get Encoder Frame Buffer Information */
	if (VpuReadReg(RET_ENC_SEQ_ENC_SUCCESS) & (1<<31))
		return VPU_RET_ERR_MEM_ACCESS;

	if (VpuReadReg(RET_ENC_SEQ_ENC_SUCCESS) == 0)
		return VPU_RET_ERR_INIT;

	if (pInst->codecMode == MJPG_ENC)
		pEncInfo->minFrameBuffers = 0;
	else
		/* reconstructed frame + reference frame + subsample */
		pEncInfo->minFrameBuffers = 2;

	pEncInfo->strmWritePrt = VpuReadReg(BIT_WR_PTR);
	pEncInfo->strmEndFlag = VpuReadReg(BIT_BIT_STREAM_PARAM);

	NX_DbgMsg(INFO_MSG, ("VPU_EncSeqCommand() Success.\n"));
	NX_DbgMsg(INFO_MSG, ("Writer Ptr = 0x%08x, Stream End Flag = %d\n",
		pEncInfo->strmWritePrt, pEncInfo->strmEndFlag));

	pInst->isInitialized = 1;
	return VPU_RET_OK;
}

static int VPU_EncSetFrameBufCommand(struct nx_vpu_codec_inst *pInst,
	uint32_t sramAddr, uint32_t sramSize)
{
	int i;
	unsigned char frameAddr[22][3][4];
	struct vpu_enc_info *pEncInfo = &pInst->codecInfo.encInfo;
	unsigned int frameBufStride = ((pEncInfo->srcWidth+15)&~15);
	unsigned int val = 0;

	NX_DrvMemset(frameAddr, 0, sizeof(frameAddr));

	/* Set Second AXI Memory (SRAM) Configuration */
	if (VPU_RET_OK != ConfigEncSecAXI(pEncInfo->codecStd,
		&pEncInfo->sec_axi_info, pEncInfo->srcWidth,
		pEncInfo->srcHeight, sramAddr, sramSize))
		return VPU_RET_ERR_SRAM;

	/* decoder(0), cbcr interleve(0), bypass(0), burst(0), merge(3),
		maptype(linear), wayshape(15) */
	pEncInfo->cacheConfig = MaverickCache2Config(0,
		pEncInfo->cbcrInterleaveRefFrame, 0, 0, 3, 0, 15);

	SetTiledMapType(VPU_LINEAR_FRAME_MAP, frameBufStride,
		pEncInfo->cbcrInterleaveRefFrame);

	if (pEncInfo->frameBufMapType) {
		if (pEncInfo->frameBufMapType == VPU_TILED_FRAME_MB_RASTER_MAP
		|| pEncInfo->frameBufMapType == VPU_TILED_FIELD_MB_RASTER_MAP)
			val |= (pEncInfo->cbcrInterleaveRefFrame<<11) |
				(0x03<<9)|(IMG_FORMAT_420<<6);
		else
			val |= (pEncInfo->cbcrInterleaveRefFrame<<11) |
				(0x02<<9)|(IMG_FORMAT_420<<6);
	}

	/* Interleave bit position is modified */
	val |= (pEncInfo->cbcrInterleaveRefFrame<<2);
	val |= VPU_FRAME_ENDIAN;
	VpuWriteReg(BIT_FRAME_MEM_CTRL, val);

	/* Let the decoder know the addresses of the frame buffers. */
	for (i = 0; i < pEncInfo->minFrameBuffers; i++) {
		struct nx_vid_memory_info *frameBuffer =
			&pEncInfo->frameBuffer[i];

		/* Y */
		frameAddr[i][0][0] = (frameBuffer->phyAddr[0] >> 24) & 0xFF;
		frameAddr[i][0][1] = (frameBuffer->phyAddr[0] >> 16) & 0xFF;
		frameAddr[i][0][2] = (frameBuffer->phyAddr[0] >>  8) & 0xFF;
		frameAddr[i][0][3] = (frameBuffer->phyAddr[0] >>  0) & 0xFF;
		/* Cb */
		frameAddr[i][1][0] = (frameBuffer->phyAddr[1] >> 24) & 0xFF;
		frameAddr[i][1][1] = (frameBuffer->phyAddr[1] >> 16) & 0xFF;
		frameAddr[i][1][2] = (frameBuffer->phyAddr[1] >>  8) & 0xFF;
		frameAddr[i][1][3] = (frameBuffer->phyAddr[1] >>  0) & 0xFF;
		/* Cr */
		frameAddr[i][2][0] = (frameBuffer->phyAddr[2] >> 24) & 0xFF;
		frameAddr[i][2][1] = (frameBuffer->phyAddr[2] >> 16) & 0xFF;
		frameAddr[i][2][2] = (frameBuffer->phyAddr[2] >>  8) & 0xFF;
		frameAddr[i][2][3] = (frameBuffer->phyAddr[2] >>  0) & 0xFF;
	}

	swap_endian((unsigned char *)frameAddr, sizeof(frameAddr));
	NX_DrvMemcpy((void *)pInst->paramVirAddr, frameAddr, sizeof(frameAddr));

	/* Tell the codec how much frame buffers were allocated. */
	VpuWriteReg(CMD_SET_FRAME_BUF_NUM, pEncInfo->minFrameBuffers);
	VpuWriteReg(CMD_SET_FRAME_BUF_STRIDE, frameBufStride);
	VpuWriteReg(CMD_SET_FRAME_AXI_BIT_ADDR,
		pEncInfo->sec_axi_info.bufBitUse);
	VpuWriteReg(CMD_SET_FRAME_AXI_IPACDC_ADDR,
		pEncInfo->sec_axi_info.bufIpAcDcUse);
	VpuWriteReg(CMD_SET_FRAME_AXI_DBKY_ADDR,
		pEncInfo->sec_axi_info.bufDbkYUse);
	VpuWriteReg(CMD_SET_FRAME_AXI_DBKC_ADDR,
		pEncInfo->sec_axi_info.bufDbkCUse);
	VpuWriteReg(CMD_SET_FRAME_AXI_OVL_ADDR,
		pEncInfo->sec_axi_info.bufOvlUse);
	VpuWriteReg(CMD_SET_FRAME_CACHE_CONFIG,
		pEncInfo->cacheConfig);

	/* Set Sub-Sampling buffer for ME-Reference and DBK-Reconstruction */
	/* BPU will swap below two buffer internally every pic by pic */
	VpuWriteReg(CMD_SET_FRAME_SUBSAMP_A, pEncInfo->subSampleAPhyAddr);
	VpuWriteReg(CMD_SET_FRAME_SUBSAMP_B, pEncInfo->subSampleBPhyAddr);

	if (pInst->codecMode == MP4_ENC) {
		/* MPEG4 Encoder Data-Partitioned bitstream temporal buffer */
		VpuWriteReg(CMD_SET_FRAME_DP_BUF_BASE,
			pEncInfo->usbSampleDPPhyAddr);
		VpuWriteReg(CMD_SET_FRAME_DP_BUF_SIZE,
			pEncInfo->usbSampleDPSize);
	}

#if (DBG_REGISTER)
	{
		int reg;

		NX_DbgMsg(DBG_REGISTER, ("[ENC_SET_FRM_BUF_Reg]\n"));
		for (reg = 0x180 ; reg < 0x200 ; reg += 16) {
			NX_DbgMsg(DBG_REGISTER, ("[Addr = %3x]%x %x %x %x\n",
				reg, VpuReadReg(BIT_BASE + reg),
				VpuReadReg(BIT_BASE + reg + 4),
				VpuReadReg(BIT_BASE + reg + 8),
				VpuReadReg(BIT_BASE + reg + 12)));
		}
	}
#endif

	VpuBitIssueCommand(pInst, SET_FRAME_BUF);
	if (VPU_RET_OK != VPU_WaitVpuBusy(VPU_BUSY_CHECK_TIMEOUT,
		BIT_BUSY_FLAG)) {
		NX_ErrMsg(("VPU_EncSetFrameBuffer() Failed. Timeout(%d)\n",
			VPU_BUSY_CHECK_TIMEOUT));
		return VPU_RET_ERR_TIMEOUT;
	}

	if (VpuReadReg(RET_SET_FRAME_SUCCESS) & (1<<31))
		return VPU_RET_ERR_MEM_ACCESS;

	return VPU_RET_OK;
}

static int VPU_EncGetHeaderCommand(struct nx_vpu_codec_inst *pInst,
	unsigned int headerType, void **ptr, int *size)
{
	struct vpu_enc_info *pEncInfo = &pInst->codecInfo.encInfo;
	int flag = 0;
	unsigned int wdPtr, rdPtr;
	int headerSize;

	if (pEncInfo->ringBufferEnable == 0) {
		VpuWriteReg(CMD_ENC_HEADER_BB_START, pEncInfo->strmBufPhyAddr);
		VpuWriteReg(CMD_ENC_HEADER_BB_SIZE, pEncInfo->strmBufSize/1024);
	}

	if (pInst->codecMode == AVC_ENC && headerType == SPS_RBSP) {
		struct enc_avc_param *avcParam =
			&pEncInfo->enc_codec_para.avcEncParam;
		int CropH = 0, CropV = 0;

		if (pEncInfo->encWidth & 0xF) {
			flag = 1;
			avcParam->cropRight = 16 - (pEncInfo->encWidth&0xF);
			CropH  = avcParam->cropLeft << 16;
			CropH |= avcParam->cropRight;
		}

		if (pEncInfo->encHeight & 0xF) {
			flag = 1;
			avcParam->cropBottom = 16 - (pEncInfo->encHeight&0xF);
			CropV  = avcParam->cropTop << 16;
			CropV |= avcParam->cropBottom;
		}

		VpuWriteReg(CMD_ENC_HEADER_FRAME_CROP_H, CropH);
		VpuWriteReg(CMD_ENC_HEADER_FRAME_CROP_V, CropV);
	}

	/* 0: SPS, 1: PPS */
	VpuWriteReg(CMD_ENC_HEADER_CODE, headerType | (flag << 3));

	VpuWriteReg(BIT_RD_PTR, pEncInfo->strmBufPhyAddr);
	VpuWriteReg(BIT_WR_PTR, pEncInfo->strmBufPhyAddr);

#if (DBG_REGISTER)
	{
		int reg;

		NX_DbgMsg(DBG_REGISTER, ("[ENC_HEADER_Reg]\n"));
		for (reg = 0x180 ; reg < 0x200 ; reg += 16) {
			NX_DbgMsg(DBG_REGISTER, ("[Addr = %3x]%x %x %x %x\n",
				reg, VpuReadReg(BIT_BASE + reg),
				VpuReadReg(BIT_BASE + reg + 4),
				VpuReadReg(BIT_BASE + reg + 8),
				VpuReadReg(BIT_BASE + reg + 12)));
		}
	}
#endif

	VpuBitIssueCommand(pInst, ENCODE_HEADER);
	if (VPU_RET_OK != VPU_WaitVpuBusy(VPU_BUSY_CHECK_TIMEOUT,
		BIT_BUSY_FLAG))
		return VPU_RET_ERR_TIMEOUT;

	if (pEncInfo->ringBufferEnable == 0) {
		rdPtr = pEncInfo->strmBufPhyAddr;
		wdPtr = VpuReadReg(BIT_WR_PTR);
		headerSize = wdPtr - rdPtr;
	} else {
		rdPtr = VpuReadReg(BIT_RD_PTR);
		wdPtr = VpuReadReg(BIT_WR_PTR);
		headerSize = wdPtr - rdPtr;
	}

	pEncInfo->strmWritePrt = wdPtr;
	pEncInfo->strmReadPrt = rdPtr;

	*ptr = (void *)pEncInfo->strmBufVirAddr;
	*size = headerSize;

	return VPU_RET_OK;
}

static int VPU_EncOneFrameCommand(struct nx_vpu_codec_inst *pInst,
	struct vpu_enc_run_frame_arg *pRunArg)
{
	unsigned int readPtr, writePtr;
	int size, picFlag/*, frameIndex*/, val, reason;
	unsigned int sliceNumber, picEncResult, picType;
	struct vpu_enc_info *pEncInfo = &pInst->codecInfo.encInfo;

	/* For Linear Frame Buffer Mode */
	VpuWriteReg(GDI_TILEDBUF_BASE, 0x00);

	/* Mirror/Rotate Mode */
	if (pEncInfo->rotateAngle == 0 && pEncInfo->mirrorDirection == 0)
		VpuWriteReg(CMD_ENC_PIC_ROT_MODE, 0);
	else
		VpuWriteReg(CMD_ENC_PIC_ROT_MODE, (1 << 4) |
			(pEncInfo->mirrorDirection << 2) |
			(pEncInfo->rotateAngle));

	/* If rate control is enabled, this register is ignored.
	 *(MPEG-4/H.263 : 1~31, AVC : 0 ~51) */
	VpuWriteReg(CMD_ENC_PIC_QS, pRunArg->quantParam);

	if (pRunArg->forceSkipPicture) {
		VpuWriteReg(CMD_ENC_PIC_OPTION, 1);
	} else {
		/* Registering Source Frame Buffer information */
		/* Hide GDI IF under FW level */
		if (pRunArg->inImgBuffer.planes == 2)
			pEncInfo->cbcrInterleave = 1;

		VpuWriteReg(CMD_ENC_PIC_SRC_INDEX, 2);
		VpuWriteReg(CMD_ENC_PIC_SRC_STRIDE,
			pRunArg->inImgBuffer.stride[0]);
		VpuWriteReg(CMD_ENC_PIC_SRC_ADDR_Y,
			pRunArg->inImgBuffer.phyAddr[0]);
		VpuWriteReg(CMD_ENC_PIC_SRC_ADDR_CB,
			pRunArg->inImgBuffer.phyAddr[1]);
		VpuWriteReg(CMD_ENC_PIC_SRC_ADDR_CR,
			pRunArg->inImgBuffer.phyAddr[2]);
		VpuWriteReg(CMD_ENC_PIC_OPTION,
			(pRunArg->forceIPicture << 1 & 0x2));
	}

	if (pEncInfo->ringBufferEnable == 0) {
		VpuWriteReg(CMD_ENC_PIC_BB_START, pEncInfo->strmBufPhyAddr);
		VpuWriteReg(CMD_ENC_PIC_BB_SIZE, pEncInfo->strmBufSize/1024);
		VpuWriteReg(BIT_RD_PTR, pEncInfo->strmBufPhyAddr);
	}

	val = 0;
	val = (
		(pEncInfo->sec_axi_info.useBitEnable & 0x01)<<0 |
		(pEncInfo->sec_axi_info.useIpEnable & 0x01)<<1 |
		(pEncInfo->sec_axi_info.useDbkYEnable & 0x01)<<2 |
		(pEncInfo->sec_axi_info.useDbkCEnable & 0x01)<<3 |

		(pEncInfo->sec_axi_info.useOvlEnable & 0x01)<<4 |
		(pEncInfo->sec_axi_info.useBtpEnable & 0x01)<<5 |

		(pEncInfo->sec_axi_info.useBitEnable & 0x01)<<8 |
		(pEncInfo->sec_axi_info.useIpEnable & 0x01)<<9 |
		(pEncInfo->sec_axi_info.useDbkYEnable & 0x01)<<10|
		(pEncInfo->sec_axi_info.useDbkCEnable & 0x01)<<11|

		(pEncInfo->sec_axi_info.useOvlEnable & 0x01)<<12|
		(pEncInfo->sec_axi_info.useBtpEnable & 0x01)<<13);

	VpuWriteReg(BIT_AXI_SRAM_USE, val);

	VpuWriteReg(BIT_WR_PTR, pEncInfo->strmBufPhyAddr);
	VpuWriteReg(BIT_RD_PTR, pEncInfo->strmBufPhyAddr);
	VpuWriteReg(BIT_BIT_STREAM_PARAM, pEncInfo->strmEndFlag);

	val = 0;
	val |= (pEncInfo->bwbEnable<<12) | (pEncInfo->cbcrInterleave<<2);
	val |= pEncInfo->frameEndian;
	VpuWriteReg(BIT_FRAME_MEM_CTRL, val);

	if (pEncInfo->ringBufferEnable == 0) {
		val = 0;
		val |= (0x1<<6);
		val |= (0x1<<5);
		val |= (0x1<<4);
	} else {
		val |= (0x1<<3);
	}
	val |= VPU_STREAM_ENDIAN;
	VpuWriteReg(BIT_BIT_STREAM_CTRL, val);

#if (DBG_REGISTER)
	{
		int reg;

		NX_DbgMsg(DBG_REGISTER, ("[ENC_RUN_Reg]\n"));
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

	do {
		reason = VPU_WaitBitInterrupt(pInst->devHandle,
			VPU_ENC_TIMEOUT);

		if (reason == 0)
			return VPU_RET_ERR_TIMEOUT;
		if (reason & (1<<VPU_INT_BIT_PIC_RUN))
			break;
		else if (reason &  (1<<VPU_INT_BIT_BIT_BUF_FULL))
			return VPU_RET_ERR_STRM_FULL;
	} while (1);

	picEncResult = VpuReadReg(RET_ENC_PIC_SUCCESS);
	if (picEncResult & (1<<31))
		return VPU_RET_ERR_MEM_ACCESS;

	picType = VpuReadReg(RET_ENC_PIC_TYPE);
	readPtr  = VpuReadReg(BIT_RD_PTR);
	writePtr = VpuReadReg(BIT_WR_PTR);

	size = writePtr - readPtr;
	sliceNumber = VpuReadReg(RET_ENC_PIC_SLICE_NUM);

	picFlag = VpuReadReg(RET_ENC_PIC_FLAG);
	pRunArg->reconImgIdx = VpuReadReg(RET_ENC_PIC_FRAME_IDX);

	pEncInfo->strmEndFlag = VpuReadReg(BIT_BIT_STREAM_PARAM);

	pRunArg->frameType = picType;
	pRunArg->outStreamSize = size;
	pRunArg->outStreamAddr = pEncInfo->strmBufVirAddr;

	/*NX_DbgMsg(INFO_MSG, ("Encoded Size = %d, PicType = %d, picFlag = %d,
		sliceNumber = %d\n", size, picType, picFlag, sliceNumber));*/

	return VPU_RET_OK;
}

static int VPU_EncChangeParameterCommand(struct nx_vpu_codec_inst *pInst,
	struct vpu_enc_chg_para_arg *pChgArg)
{
	int ret;

	if (pChgArg->chgFlg | VPU_BIT_CHG_GOP)
		VpuWriteReg(CMD_ENC_SEQ_PARA_RC_GOP, pChgArg->gopSize);

	if (pChgArg->chgFlg | VPU_BIT_CHG_INTRAQP)
		VpuWriteReg(CMD_ENC_SEQ_PARA_RC_INTRA_QP, pChgArg->intraQp);

	if (pChgArg->chgFlg | VPU_BIT_CHG_BITRATE)
		VpuWriteReg(CMD_ENC_SEQ_PARA_RC_BITRATE, pChgArg->bitrate);

	if (pChgArg->chgFlg | VPU_BIT_CHG_FRAMERATE)
		VpuWriteReg(CMD_ENC_SEQ_PARA_RC_FRAME_RATE,
		(pChgArg->frameRateNum) | ((pChgArg->frameRateDen-1) << 16));

	if (pChgArg->chgFlg | VPU_BIT_CHG_INTRARF)
		VpuWriteReg(CMD_ENC_SEQ_PARA_INTRA_MB_NUM,
			pChgArg->intraRefreshMbs);

	if (pChgArg->chgFlg | VPU_BIT_CHG_SLICEMOD)
		VpuWriteReg(CMD_ENC_SEQ_PARA_SLICE_MODE, (pChgArg->sliceMode) |
		(pChgArg->sliceSizeMode << 2) | (pChgArg->sliceSizeNum << 3));

	if (pChgArg->chgFlg | VPU_BIT_CHG_HECMODE)
		VpuWriteReg(CMD_ENC_SEQ_PARA_HEC_MODE, pChgArg->hecMode);

	VpuWriteReg(CMD_ENC_SEQ_PARA_CHANGE_ENABLE, pChgArg->chgFlg & 0x7F);

	VpuBitIssueCommand(pInst, RC_CHANGE_PARAMETER);
	if (VPU_RET_OK != VPU_WaitVpuBusy(VPU_BUSY_CHECK_TIMEOUT,
		BIT_BUSY_FLAG))
		return VPU_RET_ERR_TIMEOUT;

	ret = VpuReadReg(RET_ENC_SEQ_PARA_CHANGE_SECCESS);
	if ((ret & 1) == 0)
		return VPU_RET_ERR_CHG_PARAM;
	if (ret >> 31)
		return VPU_RET_ERR_MEM_ACCESS;

	return VPU_RET_OK;
}

static int VPU_EncCloseCommand(struct nx_vpu_codec_inst *pInst,
	void *vpu_event_present)
{
	FUNC_IN();

	if (pInst->isInitialized) {
		struct vpu_enc_info *pEncInfo = &pInst->codecInfo.encInfo;

		VpuWriteReg(BIT_WR_PTR, pEncInfo->strmBufPhyAddr);
		VpuWriteReg(BIT_RD_PTR, pEncInfo->strmBufPhyAddr);

		VpuBitIssueCommand(pInst, SEQ_END);
		if (VPU_RET_OK != VPU_WaitVpuBusy(VPU_BUSY_CHECK_TIMEOUT,
			BIT_BUSY_FLAG)) {
			VpuWriteReg(BIT_INT_CLEAR, 0x1);
			atomic_set((atomic_t *)vpu_event_present, 0);
			NX_ErrMsg(("VPU_EncCloseCommand() Failed!!!\n"));
			NX_ErrMsg(("Timeout(%d)\n", VPU_BUSY_CHECK_TIMEOUT));
			VPU_SWReset(SW_RESET_SAFETY);
			pInst->isInitialized = 0;
			return VPU_RET_ERR_TIMEOUT;
		}
		pInst->isInitialized = 0;
	}

	VpuWriteReg(BIT_INT_CLEAR, 0x1);	/* For Signal Break Out */
	atomic_set((atomic_t *)vpu_event_present, 0);/* Clear Atomic */

	FUNC_OUT();
	return VPU_RET_OK;
}
