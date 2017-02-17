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

#ifndef __NX_VPU_CONFIG_H__
#define __NX_VPU_CONFIG_H__


#define	NX_MAX_VPU_INSTANCE		16

/* VPU Clock Gating */
#define	ENABLE_CLOCK_GATING
/* #define	ENABLE_POWER_SAVING */

#define	NX_DBG_INFO			0

/* Debug Register Access */
#define	NX_REG_ACC_DEBUG		0	/* Use Debug Function */
#define	NX_REG_EN_MSG			0	/* Enable Debug Message */


/*
 *	Memory Size Config
 *
 *	 -----------------------   High Address
 *	| Instance       (1MB)  |
 *	 -----------------------
 *	| Param Buffer   (1MB)  |
 *	 -----------------------
 *	| Temp Buf       (1MB)  |
 *	 -----------------------
 *	| Working Buffer (1MB)  |
 *	 -----------------------
 *	| Code Buffer    (1MB)  |
 *	 -----------------------   Low Address
 */


#define PARA_BUF_SIZE		(12  * 1024)
#define	TEMP_BUF_SIZE		(204 * 1024)
#define	CODE_BUF_SIZE		(260 * 1024)

#define	COMMON_BUF_SIZE		(CODE_BUF_SIZE+TEMP_BUF_SIZE+PARA_BUF_SIZE)

#define	WORK_BUF_SIZE		(80  * 1024)
#define	INST_BUF_SIZE		(NX_MAX_VPU_INSTANCE*WORK_BUF_SIZE)

#define	DEC_STREAM_SIZE		        (1 * 1024 * 1024)

#define	VPU_LITTLE_ENDIAN	        0
#define	VPU_BIG_ENDIAN		        1

#define VPU_FRAME_ENDIAN	        VPU_LITTLE_ENDIAN
#define	VPU_FRAME_BUFFER_ENDIAN	        VPU_LITTLE_ENDIAN
#define VPU_STREAM_ENDIAN	        VPU_LITTLE_ENDIAN


#define	CBCR_INTERLEAVE			0
#define VPU_ENABLE_BWB			1
#define	ENC_FRAME_BUF_CBCR_INTERLEAVE	1


/* AXI Expander Select */
#define	USE_NX_EXPND			1

/* Timeout */
#define	VPU_BUSY_CHECK_TIMEOUT		500	/* 500 msec */
#define VPU_ENC_TIMEOUT			1000	/* 1 sec */
#define VPU_DEC_TIMEOUT			300	/* 300 msec */
#define JPU_ENC_TIMEOUT			1000	/* 1 sec */
#define JPU_DEC_TIMEOUT			1000	/* 1 sec */


#define VPU_GBU_SIZE		        1024	/* No modification required */
#define JPU_GBU_SIZE			512	/* No modification required */


#define	MAX_REG_FRAME			31


/*----------------------------------------------------------------------------
 *	Encoder Configurations
 */
#define	VPU_ENC_MAX_FRAME_BUF		3
#define VPU_ME_LINEBUFFER_MODE		2


/*----------------------------------------------------------------------------
 *	Decoder Configurations
 */
#define	VPU_REORDER_ENABLE		1
#define VPU_GMC_PROCESS_METHOD	0
#define VPU_AVC_X264_SUPPORT		1

#define VPU_SPP_CHUNK_SIZE		1024	/* AVC SPP */

#endif	/* __NX_VPU_CONFIG_H__ */
