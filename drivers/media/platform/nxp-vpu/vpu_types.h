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

#ifndef __VPU_TYPES_H__
#define __VPU_TYPES_H__

#include <linux/types.h>

#include "nx_port_func.h"

struct vpu_rect {
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
};

struct vpu_open_arg {
	/* Input Arguments */
	int32_t codecStd;
	int32_t isEncoder;
	int32_t mp4Class;

	struct nx_memory_info instanceBuf;
	struct nx_memory_info streamBuf;

	/* Output Arguments */
	int32_t instIndex;
};

struct vpu_enc_seq_arg {
	/* input image size */
	int32_t srcWidth;
	int32_t srcHeight;

	/* Set Stream Buffer Handle */
	uint64_t strmBufVirAddr;
	uint64_t strmBufPhyAddr;
	int32_t strmBufSize;

	int32_t frameRateNum;		/* frame rate */
	int32_t frameRateDen;
	int32_t gopSize;		/* group of picture size */

	/* Rate Control */
	int32_t bitrate;		/* Target Bitrate */
	int32_t disableSkip;		/* Flag of Skip frame disable */

	/* This value is valid if RCModule is 1.(MAX 0x7FFF)
	 * 0 does not check Reference decoder buffer delay constraint. */
	int32_t initialDelay;

	/* Reference Decoder buffer size in bytes
	  * This valid is ignored if RCModule is 1 and initialDelay is is 0.
	  * (MAX 0x7FFFFFFF) */
	int32_t vbvBufferSize;

	int32_t gammaFactor;

	/* Quantization Scale [ H.264/AVC(0~51), MPEG4(1~31) ] */
	int32_t maxQP;			/* Max Quantization Scale */

	/* This value is Initial QP when CBR
	 *       (Initial QP is computed if initQP is 0)
	 * This value is user QP when VBR. */
	int32_t initQP;

	/* Input Buffer Chroma Interleaved */
	/* Input Buffer Chroma Interleaved Format */
	int32_t chromaInterleave;
	/* Reference Buffer's Chorma Interleaved Format */
	int32_t refChromaInterleave;

	/* ME_SEARCH_RAGME_[0~3] (recomand ME_SEARCH_RAGME_2)
	 *	0 : H(-128~127), V(-64~63)
	 *	1 : H(-64~ 63), V(-32~31)
	 *	2 : H(-32~ 31), V(-16~15)
	 *	3 : H(-16~ 15), V(-16~15) */
	int32_t searchRange;

	/* Other Options */
	/* an Intra MB refresh number. It must be less than total MacroBlocks.*/
	int32_t intraRefreshMbs;

	int32_t rotAngle;
	int32_t mirDirection;

	/* AVC Only */
	int32_t enableAUDelimiter;

	/* JPEG Specific */
	int32_t quality;
	uint32_t imgFormat;

	/* H.263 Only */
	int32_t annexFlg;
};

struct vpu_enc_set_frame_arg {
	/* Reconstruct Buffer */
	int32_t numFrameBuffer;
	struct nx_vid_memory_info frameBuffer[2];

	/* Sub Sample Buffers(1 sub sample buffer size = Framebuffer size/4) */
	struct nx_memory_info subSampleBuffer[2];

	/* Data partition Buffer size(MAX WIDTH * MAX HEIGHT * 3 / 4) */
	struct nx_memory_info dataPartitionBuffer;

	/* For Sram */
	uint32_t sramAddr;
	uint32_t sramSize;
};

union vpu_enc_get_header_arg {
	struct {
		uint8_t vosData[512];
		int32_t vosSize;
		uint8_t volData[512];
		int32_t volSize;
		uint8_t voData[512];
		int32_t voSize;
	} mp4Header;
	struct {
		uint8_t spsData[512];
		int32_t spsSize;
		uint8_t ppsData[512];
		int32_t ppsSize;
	} avcHeader;
	struct {
		uint8_t jpegHeader[1024];
		int32_t headerSize;
	} jpgHeader;
};

struct vpu_enc_run_frame_arg {
	/* Input Prameter */
	struct nx_vid_memory_info inImgBuffer;

	/* Rate Control Parameters */
	int32_t changeFlag;
	int32_t enableRc;
	int32_t forceIPicture;
	int32_t quantParam;		/* User quantization Parameter */
	int32_t forceSkipPicture;

	/* Output Parameter */
	int32_t frameType;		/* I, P, B, SKIP,.. etc */
	uint64_t outStreamAddr;		/* mmapped virtual address */
	int32_t outStreamSize;		/* Stream buffer size */
	int32_t reconImgIdx;		/*  reconstructed image buffer index */
};

struct vpu_enc_chg_para_arg {
	int32_t chgFlg;
	int32_t gopSize;
	int32_t intraQp;
	int32_t bitrate;
	int32_t frameRateNum;
	int32_t frameRateDen;
	int32_t intraRefreshMbs;
	int32_t sliceMode;
	int32_t sliceSizeMode;
	int32_t sliceSizeNum;
	int32_t hecMode;
};

struct avc_vui_info {
	int32_t fixedFrameRateFlag;
	int32_t timingInfoPresent;
	int32_t chromaLocBotField;
	int32_t chromaLocTopField;
	int32_t chromaLocInfoPresent;
	int32_t colorPrimaries;
	int32_t colorDescPresent;
	int32_t isExtSAR;
	int32_t vidFullRange;
	int32_t vidFormat;
	int32_t vidSigTypePresent;
	int32_t vuiParamPresent;
	int32_t vuiPicStructPresent;
	int32_t vuiPicStruct;
};

/*
 *	Decoder Structures
 */

struct vpu_dec_seq_init_arg {
	/* Input Information */
	uint64_t seqData;
	int32_t seqDataSize;
	int32_t disableOutReorder;

	/* General Output Information */
	int32_t outWidth;
	int32_t outHeight;
	int32_t frameRateNum;		/* Frame Rate Numerator */
	int32_t frameRateDen;		/* Frame Rate Denominator */
	uint32_t bitrate;

	int32_t profile;
	int32_t level;
	int32_t interlace;
	int32_t direct8x8Flag;
	int32_t constraint_set_flag[4];
	int32_t aspectRateInfo;

	/* Frame Buffer Information */
	int32_t minFrameBufCnt;
	int32_t frameBufDelay;

	/* 1 : Deblock filter, 0 : Disable post filter */
	int32_t enablePostFilter;

	/* Mpeg4 Specific Info */
	int32_t mp4ShortHeader;
	int32_t mp4PartitionEnable;
	int32_t mp4ReversibleVlcEnable;
	int32_t h263AnnexJEnable;
	uint32_t mp4Class;

	/* VP8 Specific Info */
	int32_t vp8HScaleFactor;
	int32_t vp8VScaleFactor;
	int32_t vp8ScaleWidth;
	int32_t vp8ScaleHeight;

	/* H.264(AVC) Specific Info */
	struct avc_vui_info vui_info;
	int32_t avcIsExtSAR;
	int32_t cropLeft;
	int32_t cropTop;
	int32_t cropRight;
	int32_t cropBottom;
	int32_t numSliceSize;
	int32_t worstSliceSize;
	int32_t maxNumRefFrmFlag;

	/* VC-1 */
	int32_t	vc1Psf;

	/* Mpeg2 */
	int32_t mp2LowDelay;
	int32_t mp2DispVerSize;
	int32_t mp2DispHorSize;
	int32_t userDataNum;
	int32_t userDataSize;
	int32_t userDataBufFull;
	int32_t enableUserData;
	struct nx_memory_info userDataBuffer;

	/* Jpeg */
	int32_t thumbnailMode;

	uint64_t strmReadPos;
	uint64_t strmWritePos;

	uint32_t imgFormat;
};

struct vpu_dec_reg_frame_arg {
	/* Frame Buffers */
	int32_t numFrameBuffer;
	struct nx_vid_memory_info frameBuffer[30];

	/* MV Buffer Address */
	struct nx_memory_info colMvBuffer;

	/* AVC Slice Buffer */
	struct nx_memory_info sliceBuffer;

	/* VPX Codec Specific */
	struct nx_memory_info pvbSliceBuffer;

	int32_t chromaInterleave;

	/* For Sram */
	uint32_t sramAddr;
	uint32_t sramSize;
};

/* VP8 specific display information */
struct vp8_scale_info {
	uint32_t hScaleFactor	: 2;
	uint32_t vScaleFactor	: 2;
	uint32_t picWidth	: 14;
	uint32_t picHeight	: 14;
};

/* VP8 specific header information */
struct vp8_pic_info {
	uint32_t showFrame	: 1;
	uint32_t versionNumber	: 3;
	uint32_t refIdxLast	: 8;
	uint32_t refIdxAltr	: 8;
	uint32_t refIdxGold	: 8;
};

struct vpu_dec_frame_arg {
	/* Input Arguments */
	uint64_t strmData;
	int32_t strmDataSize;
	int32_t iFrameSearchEnable;
	int32_t skipFrameMode;
	int32_t decSkipFrameNum;
	int32_t eos;

	/* Output Arguments */
	int32_t outWidth;
	int32_t outHeight;

	struct vpu_rect outRect;

	int32_t indexFrameDecoded;
	int32_t indexFrameDisplay;

	int32_t picType;
	int32_t picTypeFirst;
	int32_t isInterace;
	int32_t picStructure;
	int32_t topFieldFirst;
	int32_t repeatFirstField;
	int32_t progressiveFrame;
	int32_t fieldSequence;
	int32_t npf;

	int32_t isSuccess;

	int32_t errReason;
	int32_t errAddress;
	int32_t numOfErrMBs;
	int32_t sequenceChanged;

	uint32_t strmReadPos;
	uint32_t strmWritePos;

	/* AVC Specific Information */
	int32_t avcFpaSeiExist;
	int32_t avcFpaSeiValue1;
	int32_t avcFpaSeiValue2;

	/* Output Bitstream Information */
	uint32_t frameStartPos;
	uint32_t frameEndPos;

	uint32_t notSufficientPsBuffer;
	uint32_t notSufficientSliceBuffer;

	uint32_t fRateNumerator;
	uint32_t fRateDenominator;

	/* Use struct vp8_scale_info & struct vp8_pic_info in VP8 */
	uint32_t aspectRateInfo;

	uint32_t mp4ModuloTimeBase;
	uint32_t mp4TimeIncrement;

	/* VP8 Scale Info */
	struct vp8_scale_info scale_info;
	struct vp8_pic_info pic_info;

	/*  VC1 Info */
	int32_t multiRes;

	struct nx_vid_memory_info outFrameBuffer;

	/* MPEG2 User Data */
	int32_t userDataNum;
	int32_t userDataSize;
	int32_t userDataBufFull;
	int32_t activeFormat;

	int32_t iRet;

	/* Jpeg Info */
	/* 0:No scaling, 1:1/2 down scaling, 2:1/4 down scaling,
		3:1/8 down scaling */
	int32_t downScaleWidth;
	int32_t downScaleHeight;

	int32_t mcuWidth;
	int32_t mcuHeight;

	struct nx_vid_memory_info *hCurrFrameBuffer;
};

struct vpu_dec_clr_dsp_flag_arg {
	int32_t indexFrameDisplay;
	struct nx_vid_memory_info frameBuffer;
};


/*////////////////////////////////////////////////////////////////////////////
 *		Command Arguments
 */

/* Define Codec Standard */
enum {
	CODEC_STD_AVC	= 0,
	CODEC_STD_VC1	= 1,
	CODEC_STD_MPEG2	= 2,
	CODEC_STD_MPEG4	= 3,
	CODEC_STD_H263	= 4,
	CODEC_STD_DIV3	= 5,
	CODEC_STD_RV	= 6,
	CODEC_STD_AVS	= 7,
	CODEC_STD_MJPG	= 8,

	CODEC_STD_THO	= 9,
	CODEC_STD_VP3	= 10,
	CODEC_STD_VP8	= 11
};

/* Search Range */
enum {
	ME_SEARCH_RAGME_0,	/* Horizontal(-128 ~ 127), Vertical(-64 ~ 64) */
	ME_SEARCH_RAGME_1,	/* Horizontal( -64 ~  63), Vertical(-32 ~ 32) */
	ME_SEARCH_RAGME_2,	/* Horizontal( -32 ~  31), Vertical(-16 ~ 15) */
	ME_SEARCH_RAGME_3,	/* Horizontal( -16 ~  15), Vertical(-16 ~ 15) */
};

/* Frame Buffer Format for JPEG */
enum {
	IMG_FORMAT_420 = 0,
	IMG_FORMAT_422 = 1,
	IMG_FORMAT_224 = 2,
	IMG_FORMAT_444 = 3,
	IMG_FORMAT_400 = 4
};

/* JPEG Mirror Direction */
enum {
	MIRDIR_NONE,
	MIRDIR_VER,
	MIRDIR_HOR,
	MIRDIR_HOR_VER,
};

/*/////////////////////////////////////////////////////////////////////////// */

#endif		/* __VPU_TYPES_H__ */
