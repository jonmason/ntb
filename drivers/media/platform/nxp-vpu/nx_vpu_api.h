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

#ifndef __NX_VPU_API_H__
#define	__NX_VPU_API_H__

#include "nx_port_func.h"
#include "vpu_types.h"
#include "nx_vpu_config.h"
#include <soc/nexell/tieoff.h>

/* Codec Mode */
enum {
	AVC_DEC = 0,
	VC1_DEC = 1,
	MP2_DEC = 2,
	MP4_DEC = 3,
	DV3_DEC = 3,
	RV_DEC = 4,
	AVS_DEC = 5,
	MJPG_DEC = 6,
	VPX_DEC = 7,

	AVC_ENC = 8,
	MP4_ENC = 11,
	MJPG_ENC = 13
};

/* MPEG4 Aux Mode */
enum {
	MP4_AUX_MPEG4 = 0,
	MP4_AUX_DIVX3 = 1
};

/* VPX Aux Mode */
enum {
	VPX_AUX_THO = 0,
	VPX_AUX_VP6 = 1,
	VPX_AUX_VP8 = 2
};

/* AVC Aux Mode */
enum {
	AVC_AUX_AVC = 0,
	AVC_AUX_MVC = 1
};

/* Interrupt Register Bit Enumerate */
enum {
	VPU_INT_BIT_INIT            = 0,
	VPU_INT_BIT_SEQ_INIT        = 1,
	VPU_INT_BIT_SEQ_END         = 2,
	VPU_INT_BIT_PIC_RUN         = 3,
	VPU_INT_BIT_FRAMEBUF_SET    = 4,
	VPU_INT_BIT_ENC_HEADER      = 5,
	VPU_INT_BIT_DEC_PARA_SET    = 7,
	VPU_INT_BIT_DEC_BUF_FLUSH   = 8,
	VPU_INT_BIT_USERDATA        = 9,
	VPU_INT_BIT_DEC_FIELD       = 10,
	VPU_INT_BIT_DEC_MB_ROWS     = 13,
	VPU_INT_BIT_BIT_BUF_EMPTY   = 14,
	VPU_INT_BIT_BIT_BUF_FULL    = 15
};

enum {
	SW_RESET_SAFETY,
	SW_RESET_FORCE,
	SW_RESET_ON_BOOT
};

enum nx_vpu_bit_chg_para {
	VPU_BIT_CHG_GOP		= 1,	        /* GOP */
	VPU_BIT_CHG_INTRAQP	= (1 << 1),     /* Intra Qp */
	VPU_BIT_CHG_BITRATE	= (1 << 2),     /* Bit Rate */
	VPU_BIT_CHG_FRAMERATE	= (1 << 3),     /* Frame Rate */
	VPU_BIT_CHG_INTRARF	= (1 << 4),     /* Intra Refresh */
	VPU_BIT_CHG_SLICEMOD	= (1 << 5),     /* Slice Mode */
	VPU_BIT_CHG_HECMODE	= (1 << 6),     /* HEC Mode */
};

enum bit_stream_mode {
	BS_MODE_INTERRUPT,
	BS_MODE_ROLLBACK,
	BS_MODE_PIC_END
};

enum mp4_header_type {
	VOL_HEADER,
	VOS_HEADER,
	VIS_HEADER
};

enum avc_header_type {
	SPS_RBSP,
	PPS_RBSP,
	END_SEQ_RBSP,
	END_STREAM_RBSP,
	SPS_RBSP_MVC,
	PPS_RBSP_MVC,
};

/* Define VPU Low-Level Return Value */
enum nx_vpu_ret {
	VPU_RET_ERR_STRM_FULL	= -24,	/* Bitstream Full */
	VPU_RET_ERR_SRAM	= -23,	/* VPU SRAM Configruation Failed */
	VPU_RET_ERR_INST	= -22,	/* VPU Have No Instance Space */
	VPU_RET_BUSY		= -21,	/* VPU H/W Busy */
	VPU_RET_ERR_TIMEOUT	= -20,	/* VPU Wait Timeout */
	VPU_RET_ERR_MEM_ACCESS	= -19,	/* Memory Access Violation */

	VPU_RET_ERR_CHG_PARAM	= -6,	/* VPU Not Changed */
	VPU_RET_ERR_WRONG_SEQ	= -5,	/* Wrong Sequence */
	VPU_RET_ERR_PARAM	= -4,	/* VPU Invalid Parameter */
	VPU_RET_ERR_RUN		= -3,
	VPU_RET_ERR_INIT	= -2,	/* VPU Not Initialized */
	VPU_RET_ERROR		= -1,	/* General operation failed */
	VPU_RET_OK		= 0,
	VPU_RET_NEED_STREAM	= 1,	/* Need More Stream */
};


/* Common Memory Information */

struct sec_axi_info {
	int useBitEnable;
	int useIpEnable;
	int useDbkYEnable;
	int useDbkCEnable;
	int useOvlEnable;
	int useBtpEnable;
	unsigned int bufBitUse;
	unsigned int bufIpAcDcUse;
	unsigned int bufDbkYUse;
	unsigned int bufDbkCUse;
	unsigned int bufOvlUse;
	unsigned int bufBtpUse;
	int bufSize;
};

struct enc_mp4_param {
	int mp4DataPartitionEnable;
	int mp4ReversibleVlcEnable;
	int mp4IntraDcVlcThr;
	int mp4HecEnable;
	int mp4Verid;
};

struct enc_h263_param {
	int h263AnnexIEnable;
	int h263AnnexJEnable;
	int h263AnnexKEnable;
	int h263AnnexTEnable;
};

struct enc_avc_param {
	/* CMD_ENC_SEQ_264_PARA Register (0x1A0) */
	int chromaQpOffset;		/* bit [4:0] */
	int constrainedIntraPredFlag;	/* bit [5] */
	int disableDeblk;		/* bit [7:6] */
	int deblkFilterOffsetAlpha;	/* bit [11:8] */
	int deblkFilterOffsetBeta;	/* bit [15:12] */

	/* CMD_ENC_SEQ_OPTION Register */
	int audEnable;			/* bit[2] AUD(Access Unit Delimiter) */

	/* Crop Info */
	int enableCrop;
	int cropLeft;
	int cropTop;
	int cropRight;
	int cropBottom;
};

struct enc_jpeg_info {
	int picWidth;
	int picHeight;
	int alignedWidth;
	int alignedHeight;
	int frameIdx;
	int format;

	int rotationEnable;
	int rotationAngle;
	int mirrorEnable;
	int mirrorDirection;

	int rstIntval;
	int busReqNum;
	int mcuBlockNum;
	int compNum;
	int compInfo[3];

	unsigned int huffCode[4][256];
	unsigned int huffSize[4][256];
	unsigned char huffVal[4][162];
	unsigned char huffBits[4][256];
	unsigned char qMatTab[4][64];
	unsigned char cInfoTab[4][6];

	uint8_t jpegHeader[1024];
	int32_t headerSize;
};

struct vpu_enc_info {
	int codecStd;

	/* input picture */
	int srcWidth;
	int srcHeight;

	/* encoding image size */
	int encWidth;
	int encHeight;

	int gopSize;
	int bitRate;
	int frameRateNum;	        /* framerate */
	int frameRateDen;

	int rotateAngle;		/* 0/90/180/270 */
	int mirrorDirection;	        /* 0/1/2/3 */

	int sliceMode;
	int sliceSizeMode;
	int sliceSize;

	int bwbEnable;
	int cbcrInterleave;		/* Input Frame's CbCrInterleave */
	int cbcrInterleaveRefFrame;	/* Reference Frame's CbCrInterleave */
	int frameEndian;

	int frameQp;
	int jpegQuality;

	/* Frame Buffers */
	int minFrameBuffers;
	int frameBufMapType;
	struct nx_vid_memory_info frameBuffer[3];
	uint64_t subSampleAPhyAddr;
	uint64_t subSampleBPhyAddr;
	struct sec_axi_info sec_axi_info;
	/* for CMD_SET_FRAME_CACHE_CONFIG register */
	unsigned int cacheConfig;

	/* Mpeg4 Encoder Only (for data partition) */
	uint64_t usbSampleDPPhyAddr;
	uint32_t usbSampleDPSize;

	int linear2TiledEnable;

	/* Output Stream Buffer's Address & Size */
	uint64_t strmBufVirAddr;
	uint64_t strmBufPhyAddr;
	int strmBufSize;

	unsigned int ringBufferEnable;
	unsigned int strmWritePrt;	/* Bitstream Write Ptr */
	unsigned int strmReadPrt;	/* Bitstream Read Ptr */
	unsigned int strmEndFlag;	/* Bitstream End Flag */

	int userQpMax;			/* User Max Quantization */
	int userGamma;			/* User Gamma Factor */

	/* Rate Control */
	int rcEnable;			/* Rate Control Enable */
	int rcIntraQp;

	/* (MB Mode(0), Frame Mode(1), Slice Mode(2), MB-NumMode(3) */
	int rcIntervalMode;
	int mbInterval;
	int rcIntraCostWeigth;
	int enableAutoSkip;
	int initialDelay;
	int vbvBufSize;
	int intraRefresh;

	union {
		struct enc_avc_param avcEncParam;
		struct enc_mp4_param mp4EncParam;
		struct enc_h263_param h263EncParam;
		struct enc_jpeg_info jpgEncInfo;
	} enc_codec_para;

	/* Motion Estimation */
	int MEUseZeroPmv;
	int MESearchRange;
	int MEBlockMode;
};

struct low_delay_info {
	int lowDelayEn;
	int numRows;
};

struct vpu_dec_info {
	int codecStd;

	int width;
	int height;

	/* Input Stream Buffer Address */
	uint64_t streamRdPtrRegAddr;	/* BIT_RD_PTR or MJPEG_BBC_RD_PTR_REG */
	uint64_t streamWrPtrRegAddr;	/* BIT_WR_PTR or MJPEG_BBC_WR_PTR_REG */
	uint64_t frameDisplayFlagRegAddr;

	uint64_t strmBufPhyAddr;
	uint64_t strmBufVirAddr;
	int needMoreFrame;
	int strmBufSize;
	int bitStreamMode;

	int bwbEnable;
	int cbcrInterleave;

	int seqInitEscape;

	/* User Data */
	int userDataEnable;
	int userDataReportMode;
	uint64_t userDataBufPhyAddr;
	uint64_t userDataBufVirAddr;
	int userDataBufSize;

	/* Low Dealy Information */
	struct low_delay_info low_delay_info;

	unsigned int readPos;
	unsigned int writePos;

	/* Frame Buffer Information (Instance Global) */
	int numFrameBuffer;
	struct nx_vid_memory_info frameBuffer[MAX_REG_FRAME];
	struct sec_axi_info sec_axi_info;
	/* for CMD_SET_FRAME_CACHE_CONFIG register */
	int cacheConfig;

	int bytePosFrameStart;
	int bytePosFrameEnd;

	/* Options */
	int enableReordering;		/* enable reordering */
	int enableMp4Deblock;		/* Mpeg4 Deblocking Option */

	/* MPEG-4/Divx5.0 or Higher/XVID/Divx4.0/old XVID
		(Compress Type --> class) */
	int mp4Class;

	/* VC1 Specific Information */
	int vc1BframeDisplayValid;

	/* AVC Specific Information */
	int avcErrorConcealMode;

	int frameDelay;
	int streamEndflag;
	int streamEndian;
	int frameDisplayFlag;
	int clearDisplayIndexes;

	/* Jpeg Specific Information */
	int frmBufferValid[MAX_REG_FRAME];

	unsigned int headerSize;
	int thumbnailMode;
	int decodeIdx;

	int imgFormat;
	int rstInterval;
	int userHuffTable;

	unsigned char huffBits[4][256];
	unsigned char huffPtr[4][16];
	unsigned int huffMin[4][16];
	unsigned int huffMax[4][16];

	unsigned char huffValue[4][162];
	unsigned char infoTable[4][6];
	unsigned char quantTable[4][64];

	int huffDcIdx;
	int huffAcIdx;
	int qIdx;

	int busReqNum;
	int mcuBlockNum;
	int compNum;
	int compInfo[3];
	int mcuWidth;
	int mcuHeight;

	int pagePtr;
	int wordPtr;
	int bitPtr;

	int validFlg;
};

struct nx_vpu_codec_inst {
	void *devHandle;
	int inUse;
	int instIndex;
	int isInitialized;
	int codecMode;
	int auxMode;
	uint64_t paramPhyAddr;	        /* Common Area */
	uint64_t paramVirAddr;
	uint32_t paramBufSize;
	uint64_t instBufPhyAddr;
	uint64_t instBufVirAddr;
	uint32_t instBufSize;
	union{
		struct vpu_dec_info decInfo;
		struct vpu_enc_info encInfo;
	} codecInfo;
};

/* BIT_RUN command */
enum nx_vpu_cmd {
	SEQ_INIT = 1,
	SEQ_END = 2,
	PIC_RUN = 3,
	SET_FRAME_BUF = 4,
	ENCODE_HEADER = 5,
	ENC_PARA_SET = 6,
	DEC_PARA_SET = 7,
	DEC_BUF_FLUSH = 8,
	RC_CHANGE_PARAMETER = 9,
	VPU_SLEEP = 10,
	VPU_WAKE = 11,
	ENC_ROI_INIT = 12,
	FIRMWARE_GET = 0xf,

	GET_ENC_INSTANCE = 0x100,
	ENC_RUN = 0x101,

	GET_DEC_INSTANCE = 0x200,
	DEC_RUN = 0x201,
};

enum vpu_gdi_tiled_map_type {
	VPU_LINEAR_FRAME_MAP  = 0,
	VPU_TILED_FRAME_V_MAP = 1,
	VPU_TILED_FRAME_H_MAP = 2,
	VPU_TILED_FIELD_V_MAP = 3,
	VPU_TILED_MIXED_V_MAP = 4,
	VPU_TILED_FRAME_MB_RASTER_MAP = 5,
	VPU_TILED_FIELD_MB_RASTER_MAP = 6,
	VPU_TILED_MAP_TYPE_MAX
};


/* H/W Level APIs */
void NX_VPU_HwOn(void *, void *);
void NX_VPU_HWOff(void *);
int NX_VPU_GetCurPowerState(void);
void NX_VPU_Clock(int on);

int NX_VpuInit(void *pv, void *baseAddr, void *firmVirAddr,
	uint32_t firmPhyAddr);
int NX_VpuDeInit(void *);

int NX_VpuSuspend(void *dev);
int NX_VpuResume(void *dev, void *pVpuBaseAddr);

int VPU_WaitVpuBusy(int mSeconds, unsigned int busyFlagReg);

int VPU_SWReset(int resetMode);

struct nx_vpu_codec_inst *NX_VpuGetInstance(int index);
int NX_VpuIsInitialized(void);
int NX_VpuParaInitialized(void *dev);
int swap_endian(unsigned char *data, int len);

/* Encoder Specific APIs */
int NX_VpuEncOpen(struct vpu_open_arg *pOpenArg, void *devHandle,
	struct nx_vpu_codec_inst **ppInst);
int NX_VpuEncClose(struct nx_vpu_codec_inst *pInst,
	void *vpu_event_present);
int NX_VpuEncSetSeqParam(struct nx_vpu_codec_inst *pInst,
	struct vpu_enc_seq_arg *pSeqArg);
int NX_VpuEncSetFrame(struct nx_vpu_codec_inst *pInst,
	struct vpu_enc_set_frame_arg *pFrmArg);
int NX_VpuEncGetHeader(struct nx_vpu_codec_inst *pInst,
	union vpu_enc_get_header_arg *pHeader);
int NX_VpuEncRunFrame(struct nx_vpu_codec_inst *pInst,
	struct vpu_enc_run_frame_arg *pRunArg);
int NX_VpuEncChgParam(struct nx_vpu_codec_inst *pInst,
	struct vpu_enc_chg_para_arg *pChgArg);

/* Decoder Specific APIs */
int NX_VpuDecOpen(struct vpu_open_arg *pOpenArg, void *devHandle,
	struct nx_vpu_codec_inst **ppInst);
int NX_VpuDecClose(struct nx_vpu_codec_inst *pInst,
	void *vpu_event_present);
int NX_VpuDecSetSeqInfo(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_seq_init_arg *pSeqArg);
int NX_VpuDecRegFrameBuf(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_reg_frame_arg *pFrmArg);
int NX_VpuDecRunFrame(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_frame_arg *pRunArg);
int NX_VpuDecFlush(struct nx_vpu_codec_inst *pInst);
int NX_VpuDecClrDspFlag(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_clr_dsp_flag_arg *pArg);

/* Jpeg Encoder Specific APIs */
int JPU_EncRunFrame(struct nx_vpu_codec_inst *pInst,
	struct vpu_enc_run_frame_arg *pRunArg);

/* Jpeg Decoder Specific APIs */
int JPU_DecSetSeqInfo(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_seq_init_arg *pSeqArg);
int JPU_DecParseHeader(struct vpu_dec_info *pInfo, uint8_t *pbyStream,
	int32_t iSize);
int JPU_DecRegFrameBuf(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_reg_frame_arg *pFrmArg);
int JPU_DecRunFrame(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_frame_arg *pRunArg);


extern void vpu_soc_peri_reset_enter(void *pv);
extern void vpu_soc_peri_reset_exit(void *pv);

extern int VPU_WaitBitInterrupt(void *devHandle, int mSeconds);
extern int JPU_WaitInterrupt(void *devHandle, int timeOut);

#endif/* __NX_VPU_API_H__ */
