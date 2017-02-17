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

#include "vpu_hw_interface.h"		/* Register Access */
#include "nx_vpu_gdi.h"
#include "nx_port_func.h"


#define XY2CONFIG(A, B, C, D, E, F, G, H, I) \
	((A)<<20 | (B)<<19 | (C)<<18 | (D)<<17 | (E)<<16 | (F)<<12 | (G)<<8 | \
	 (H)<<4 | (I))
#define XY2(A, B, C, D)                 ((A)<<12 | (B)<<8 | (C)<<4 | (D))
#define XY2BANK(A, B, C, D, E, F) \
	((A)<<13 | (B)<<12 | (C)<<8 | (D)<<5 | (E)<<4 | (F))
#define RBC(A, B, C, D)                 ((A)<<10 | (B)<<6 | (C)<<4 | (D))
#define RBC_SAME(A, B)                  ((A)<<10 | (B)<<6 | (A)<<4 | (B))

#define NUM_MB_720	                ((1280/16) * (720/16))
#define NUM_MB_1080	                ((1920/16) * (1088/16))
#define NUM_MB_SD	                ((720/16) * (576/16))

/* DRAM configuration for TileMap access */
#define EM_RAS                          13
#define EM_BANK                         3
#define EM_CAS                          10
#define EM_WIDTH                        2


struct gdi_tiled_map {
	int xy2ca_map[16];
	int xy2ba_map[16];
	int xy2ra_map[16];
	int rbc2axi_map[32];
	int MapType;

	int xy2rbc_config;
	int tb_separate_map;
	int top_bot_split;
	int tiledMap;
	int ca_inc_hor;
	int val;
};

enum {
	CA_SEL  = 0,
	BA_SEL  = 1,
	RA_SEL  = 2,
	Z_SEL   = 3,
};

enum {
	X_SEL   = 0,
	Y_SEL   = 1,
};

struct tiled_map_config {
	int  xy2caMap[16];
	int  xy2baMap[16];
	int  xy2raMap[16];
	int  rbc2axiMap[32];
	int  mapType;
	int  xy2rbcConfig;

	int  tiledBaseAddr;
	int  tbSeparateMap;
	int  topBotSplit;
	int  tiledMap;
	int  caIncHor;
	int  convLinear;
};

struct dram_config {
	int  rasBit;
	int  casBit;
	int  bankBit;
	int  busBit;
};


int SetTiledMapType(int mapType, int stride, int interleave)
{
	int ret;
	int luma_map;
	int chro_map;
	int i;
	struct tiled_map_config mapCfg;
	struct dram_config dramCfg;

	dramCfg.rasBit = EM_RAS;
	dramCfg.casBit = EM_CAS;
	dramCfg.bankBit = EM_BANK;
	dramCfg.busBit = EM_WIDTH;

	NX_DrvMemset(&mapCfg, 0, sizeof(mapCfg));

	mapCfg.mapType = mapType;
	mapCfg.xy2rbcConfig = 0;

	/* inv = 1'b0, zero = 1'b1 , tbxor = 1'b0, xy = 1'b0, bit = 4'd0 */
	luma_map = 64;
	chro_map = 64;

	for (i = 0; i < 16 ; i = i+1)
		mapCfg.xy2caMap[i] = luma_map << 8 | chro_map;

	for (i = 0; i < 4;  i = i+1)
		mapCfg.xy2baMap[i] = luma_map << 8 | chro_map;

	for (i = 0; i < 16; i = i+1)
		mapCfg.xy2raMap[i] = luma_map << 8 | chro_map;

	/* this will be removed after map size optimizing. */
	ret = stride;
	ret = 0;

	if (dramCfg.casBit == 9 && dramCfg.bankBit == 2 &&
		dramCfg.rasBit == 13) {
		mapCfg.rbc2axiMap[0]  = RBC(Z_SEL,  0, Z_SEL,  0);
		mapCfg.rbc2axiMap[1]  = RBC(Z_SEL,  0, Z_SEL,  0);
		mapCfg.rbc2axiMap[2]  = RBC(Z_SEL,  0, Z_SEL,  0);
		mapCfg.rbc2axiMap[3]  = RBC(CA_SEL, 0, CA_SEL, 0);
		mapCfg.rbc2axiMap[4]  = RBC(CA_SEL, 1, CA_SEL, 1);
		mapCfg.rbc2axiMap[5]  = RBC(CA_SEL, 2, CA_SEL, 2);
		mapCfg.rbc2axiMap[6]  = RBC(CA_SEL, 3, CA_SEL, 3);
		mapCfg.rbc2axiMap[7]  = RBC(CA_SEL, 4, CA_SEL, 4);
		mapCfg.rbc2axiMap[8]  = RBC(CA_SEL, 5, CA_SEL, 5);
		mapCfg.rbc2axiMap[9]  = RBC(CA_SEL, 6, CA_SEL, 6);
		mapCfg.rbc2axiMap[10] = RBC(CA_SEL, 7, CA_SEL, 7);
		mapCfg.rbc2axiMap[11] = RBC(CA_SEL, 8, CA_SEL, 8);

		mapCfg.rbc2axiMap[12] = RBC(BA_SEL, 0, BA_SEL, 0);
		mapCfg.rbc2axiMap[13] = RBC(BA_SEL, 1, BA_SEL, 1);

		mapCfg.rbc2axiMap[14] = RBC(RA_SEL, 0,  RA_SEL, 0);
		mapCfg.rbc2axiMap[15] = RBC(RA_SEL, 1,  RA_SEL, 1);
		mapCfg.rbc2axiMap[16] = RBC(RA_SEL, 2,  RA_SEL, 2);
		mapCfg.rbc2axiMap[17] = RBC(RA_SEL, 3,  RA_SEL, 3);
		mapCfg.rbc2axiMap[18] = RBC(RA_SEL, 4,  RA_SEL, 4);
		mapCfg.rbc2axiMap[19] = RBC(RA_SEL, 5,  RA_SEL, 5);
		mapCfg.rbc2axiMap[20] = RBC(RA_SEL, 6,  RA_SEL, 6);
		mapCfg.rbc2axiMap[21] = RBC(RA_SEL, 7,  RA_SEL, 7);
		mapCfg.rbc2axiMap[22] = RBC(RA_SEL, 8,  RA_SEL, 8);
		mapCfg.rbc2axiMap[23] = RBC(RA_SEL, 9,  RA_SEL, 9);
		mapCfg.rbc2axiMap[24] = RBC(RA_SEL, 10, RA_SEL, 10);
		mapCfg.rbc2axiMap[25] = RBC(RA_SEL, 11, RA_SEL, 11);
		mapCfg.rbc2axiMap[26] = RBC(RA_SEL, 12, RA_SEL, 12);
		mapCfg.rbc2axiMap[27] = RBC(RA_SEL, 13, RA_SEL, 13);
		mapCfg.rbc2axiMap[28] = RBC(RA_SEL, 14, RA_SEL, 14);
		mapCfg.rbc2axiMap[29] = RBC(RA_SEL, 15, RA_SEL, 15);
		mapCfg.rbc2axiMap[30] = RBC(Z_SEL,  0,  Z_SEL,  0);
		mapCfg.rbc2axiMap[31] = RBC(Z_SEL,  0,  Z_SEL,  0);
		ret = 1;
	} else if (dramCfg.casBit == 10 && dramCfg.bankBit == 3 &&
		dramCfg.rasBit == 13) {
		mapCfg.rbc2axiMap[0]  = RBC(Z_SEL,  0,  Z_SEL, 0);
		mapCfg.rbc2axiMap[1]  = RBC(Z_SEL,  0,  Z_SEL, 0);
		mapCfg.rbc2axiMap[2]  = RBC(CA_SEL, 0, CA_SEL, 0);
		mapCfg.rbc2axiMap[3]  = RBC(CA_SEL, 1, CA_SEL, 1);
		mapCfg.rbc2axiMap[4]  = RBC(CA_SEL, 2, CA_SEL, 2);
		mapCfg.rbc2axiMap[5]  = RBC(CA_SEL, 3, CA_SEL, 3);
		mapCfg.rbc2axiMap[6]  = RBC(CA_SEL, 4, CA_SEL, 4);
		mapCfg.rbc2axiMap[7]  = RBC(CA_SEL, 5, CA_SEL, 5);
		mapCfg.rbc2axiMap[8]  = RBC(CA_SEL, 6, CA_SEL, 6);
		mapCfg.rbc2axiMap[9]  = RBC(CA_SEL, 7, CA_SEL, 7);
		mapCfg.rbc2axiMap[10] = RBC(CA_SEL, 8, CA_SEL, 8);
		mapCfg.rbc2axiMap[11] = RBC(CA_SEL, 9, CA_SEL, 9);

		mapCfg.rbc2axiMap[12] = RBC(BA_SEL, 0, BA_SEL, 0);
		mapCfg.rbc2axiMap[13] = RBC(BA_SEL, 1, BA_SEL, 1);
		mapCfg.rbc2axiMap[14] = RBC(BA_SEL, 2, BA_SEL, 2);

		mapCfg.rbc2axiMap[15] = RBC(RA_SEL, 0,  RA_SEL, 0);
		mapCfg.rbc2axiMap[16] = RBC(RA_SEL, 1,  RA_SEL, 1);
		mapCfg.rbc2axiMap[17] = RBC(RA_SEL, 2,  RA_SEL, 2);
		mapCfg.rbc2axiMap[18] = RBC(RA_SEL, 3,  RA_SEL, 3);
		mapCfg.rbc2axiMap[19] = RBC(RA_SEL, 4,  RA_SEL, 4);
		mapCfg.rbc2axiMap[20] = RBC(RA_SEL, 5,  RA_SEL, 5);
		mapCfg.rbc2axiMap[21] = RBC(RA_SEL, 6,  RA_SEL, 6);
		mapCfg.rbc2axiMap[22] = RBC(RA_SEL, 7,  RA_SEL, 7);
		mapCfg.rbc2axiMap[23] = RBC(RA_SEL, 8,  RA_SEL, 8);
		mapCfg.rbc2axiMap[24] = RBC(RA_SEL, 9,  RA_SEL, 9);
		mapCfg.rbc2axiMap[25] = RBC(RA_SEL, 10, RA_SEL, 10);
		mapCfg.rbc2axiMap[26] = RBC(RA_SEL, 11, RA_SEL, 11);
		mapCfg.rbc2axiMap[27] = RBC(RA_SEL, 12, RA_SEL, 12);
		mapCfg.rbc2axiMap[28] = RBC(Z_SEL,  0,  Z_SEL,  0);
		mapCfg.rbc2axiMap[29] = RBC(Z_SEL,  0,  Z_SEL,  0);
		mapCfg.rbc2axiMap[30] = RBC(Z_SEL,  0,  Z_SEL,  0);
		mapCfg.rbc2axiMap[31] = RBC(Z_SEL,  0,  Z_SEL,  0);
		ret = 1;
	}

	/*xy2ca_map */
	for (i = 0; i < 16; i++)
		VpuWriteReg(GDI_XY2_CAS_0 + 4*i, mapCfg.xy2caMap[i]);

	/*xy2baMap */
	for (i = 0; i < 4; i++)
		VpuWriteReg(GDI_XY2_BA_0  + 4*i, mapCfg.xy2baMap[i]);

	/*xy2raMap */
	for (i = 0; i < 16; i++)
		VpuWriteReg(GDI_XY2_RAS_0 + 4*i, mapCfg.xy2raMap[i]);

	/*xy2rbcConfig */
	VpuWriteReg(GDI_XY2_RBC_CONFIG, mapCfg.xy2rbcConfig);

	/*// fast access for reading */
	mapCfg.tbSeparateMap = (mapCfg.xy2rbcConfig >> 19) & 0x1;
	mapCfg.topBotSplit = (mapCfg.xy2rbcConfig >> 18) & 0x1;
	mapCfg.tiledMap = (mapCfg.xy2rbcConfig >> 17) & 0x1;
	mapCfg.caIncHor	= (mapCfg.xy2rbcConfig >> 16) & 0x1;

	/* RAS, BA, CAS -> Axi Addr */
	for (i = 0; i < 32; i++)
		VpuWriteReg(GDI_RBC2_AXI_0 + 4*i, mapCfg.rbc2axiMap[i]);

	return ret;
}

/*
 *	Internal SRAM Memory Map (for Encding)
 *
 *	|                  |
 *	|------------------| SRAM High Address
 *	|                  |
 *	|  Deblock Chroma  | <-- H.263 : if (width>720) use external ram
 *	|                  |
 *	|------------------|
 *	|                  |
 *	|  Deblock Luma    | <-- H.263 : if (width>720) use external ram
 *	|                  |
 *	|------------------|
 *	|                  |
 *	|  Prediction Buf  |
 *	|     (AC/DC)      |
 *	|                  |
 *	|------------------|
 *	|                  |
 *	| Intra Prediction | <-- H.264 : if (width>720) use external ram
 *	|                  |
 *	|------------------|
 *	|                  |
 *	|  BIT Processor   |
 *	|                  |
 *	|------------------| SRAM Low Address
 *	|                  |
 */

int ConfigEncSecAXI(int codStd, struct sec_axi_info *sa, int width,
	int height, uint32_t sramAddr, uint32_t sramSize)
{
	int offset;
	int MbNumX = ((width & 0xFFFF) + 15) / 16;
	int MbNumY = ((height & 0xFFFF) + 15) / 16;
	int totalMB = MbNumX * MbNumY;
	uint32_t sramPhyAddr = sramAddr;

	/* useIpEnable : Intra Prediction
	 * useDbkYEnable : Deblocking Luminance
	 * useDbkCEnable : Deblocking Chrominance
	 * useBitEnable : BitAxiSecEn (USE Bit Processor)
	 * useOvlEnable : Enabled Overlap Filter(VC-1 Only)
	 * useBtpEnable : Enable BTP(Bit-Plane)(VC-1 Only)
	 */
	if (sramSize > 0) {
		switch (codStd) {
		case CODEC_STD_AVC:
			sa->useIpEnable = 1;
			sa->useBitEnable = 1;
			sa->useDbkYEnable = 0;
			sa->useDbkCEnable = 0;
			sa->useOvlEnable = 0;
			sa->useBtpEnable = 0;
			break;
		case CODEC_STD_MPEG4:
			sa->useBitEnable = 1;
			sa->useIpEnable = 1;
			sa->useDbkYEnable = 1;
			sa->useDbkCEnable = 1;
			sa->useOvlEnable = 0;
			sa->useBtpEnable = 0;
			break;
		case CODEC_STD_H263:
			if (totalMB > NUM_MB_SD) {
				sa->useDbkYEnable = 0;
				sa->useDbkCEnable = 0;
			} else {
				sa->useDbkYEnable = 1;
				sa->useDbkCEnable = 1;
			}
			sa->useBitEnable = 1;
			sa->useIpEnable = 1;
			sa->useOvlEnable = 0;
			sa->useBtpEnable = 0;
			break;
		}
	} else {
		sa->useIpEnable = 0;
		sa->useBitEnable = 0;
		sa->useDbkYEnable = 0;
		sa->useDbkCEnable = 0;
		sa->useOvlEnable = 0;
		sa->useBtpEnable = 0;
	}

	offset = 0;

	/* BIT Processor */
	if (sa->useBitEnable) {
		sa->bufBitUse = sramPhyAddr + offset;
		if (CODEC_STD_AVC == codStd)
			offset = offset + MbNumX * 144;
		else
			offset = offset + MbNumX *  16;
	}

	/* Intra Prediction,(H.264 Only) */
	if (sa->useIpEnable /*&& CODEC_STD_AVC == codStd*/) {
		sa->bufIpAcDcUse = sramPhyAddr + offset;
		offset = offset + (MbNumX * 64);
	}

	/* Deblock Luma */
	if (sa->useDbkYEnable) {
		sa->bufDbkYUse = sramPhyAddr + offset;
		if (CODEC_STD_AVC == codStd)
			offset = offset + (MbNumX * 64);
		else if (CODEC_STD_H263 == codStd)
			offset = offset + MbNumX * 128;
	}

	/* Deblock Chroma */
	if (sa->useDbkCEnable) {
		sa->bufDbkCUse = sramPhyAddr + offset;
		if (CODEC_STD_AVC == codStd)
			offset = offset + (MbNumX * 64);
		else if (CODEC_STD_H263 == codStd)
			offset = offset + MbNumX * 128;
	}
	sa->bufSize = offset;

	if (sa->bufSize > sramSize) {
		NX_ErrMsg(("ConfigEncSecAXI() Failed!!!"));
		NX_ErrMsg(("(bufSz=%d, sramSz=%d)\n", sa->bufSize, sramSize));
		return VPU_RET_ERR_SRAM;
	}

	return VPU_RET_OK;
}

int ConfigDecSecAXI(int codStd, struct sec_axi_info *sa, int width, int height,
	uint32_t sramAddr, uint32_t sramSize)
{
	int offset;
	int MbNumX = ((width & 0xFFFF) + 15) / 16;
	int MbNumY = ((height & 0xFFFF) + 15) / 16;
	int totalMB = MbNumX * MbNumY;
	int sramPhyAddr = sramAddr;

	/* useIpEnable : Intra Prediction
	 * useDbkYEnable : Deblocking Luminance
	 * useDbkCEnable : Deblocking Chrominance
	 * useBitEnable : BitAxiSecEn (USE Bit Processor)
	 * useOvlEnable : Enabled Overlap Filter(VC-1 Only)
	 * useBtpEnable : Enable BTP(Bit-Plane)(VC-1 Only) */
	if (sramSize > 0) {
		switch (codStd) {
		case CODEC_STD_AVC:
			if ((totalMB > NUM_MB_SD) && (totalMB <= NUM_MB_720)) {
				sa->useIpEnable = 1;
				sa->useDbkYEnable = 0;
				sa->useDbkCEnable = 0;
			}
			if (totalMB > NUM_MB_720) {
				sa->useIpEnable = 1;
				sa->useDbkYEnable = 0;
				sa->useDbkCEnable = 0;
			} else {
				sa->useIpEnable = 1;
				sa->useDbkYEnable = 0;
				sa->useDbkCEnable = 0;
			}
			sa->useBitEnable = 1;
			sa->useOvlEnable = 1;
			sa->useBtpEnable = 1;
			break;
		case CODEC_STD_VC1:
			sa->useBitEnable = 1;
			sa->useIpEnable = 0;
			sa->useDbkYEnable = 0;
			sa->useDbkCEnable = 0;
			sa->useOvlEnable = 1;
			sa->useBtpEnable = 1;
			break;
		case CODEC_STD_MPEG4:
		case CODEC_STD_H263:
		case CODEC_STD_MPEG2:
			sa->useBitEnable = 1;
			sa->useIpEnable = 1;
			sa->useDbkYEnable = 0;
			sa->useDbkCEnable = 0;
			sa->useOvlEnable = 1;
			sa->useBtpEnable = 1;
			break;
		case CODEC_STD_RV:
			sa->useBitEnable = 1;
			sa->useIpEnable = 1;
			sa->useDbkYEnable = 0;
			sa->useDbkCEnable = 0;
			sa->useOvlEnable = 1;
			sa->useBtpEnable = 1;
			break;
		}
	} else {
		sa->useBitEnable = 0;
		sa->useIpEnable = 0;
		sa->useDbkYEnable = 0;
		sa->useDbkCEnable = 0;
		sa->useOvlEnable = 0;
		sa->useBtpEnable = 0;
	}

	offset = 0;

	/* BIT */
	if (sa->useBitEnable) {
		sa->useBitEnable = 1;
		sa->bufBitUse = sramPhyAddr + offset;

		switch (codStd) {
		case CODEC_STD_AVC:
			offset = offset + MbNumX * 144;
			break;
		case CODEC_STD_RV:
			offset = offset + MbNumX * 128;
			break;
		case CODEC_STD_VC1:
			offset = offset + MbNumX * 64;
			break;
		case CODEC_STD_AVS:
			offset = offset + (MbNumX + (MbNumX%4)) * 32;
			break;
		case CODEC_STD_MPEG2:
			offset = offset + MbNumX * 0;
			break;
		case CODEC_STD_VP8:
			offset = offset + MbNumX * 128;
			break;
		default:
			offset = offset + MbNumX * 16;
			break;/* MPEG-4, Divx3 */
		}

		if (offset > sramSize) {
			sa->bufSize = 0;
			return 0;
		}
	}

	/* Intra Prediction, ACDC */
	if (sa->useIpEnable) {
		sa->bufIpAcDcUse = sramPhyAddr + offset;
		sa->useIpEnable = 1;

		switch (codStd) {
		case CODEC_STD_AVC:
			offset = offset + MbNumX * 64;
			break;
		case CODEC_STD_RV:
			offset = offset + MbNumX * 64;
			break;
		case CODEC_STD_VC1:
			offset = offset + MbNumX * 128;
			break;
		case CODEC_STD_AVS:
			offset = offset + MbNumX * 64;
			break;
		case CODEC_STD_MPEG2:
			offset = offset + MbNumX * 0;
			break;
		case CODEC_STD_VP8:
			offset = offset + MbNumX * 64;
			break;
		default:
			offset = offset + MbNumX * 128;
			break;/* MPEG-4, Divx3 */
		}

		if (offset > sramSize) {
			sa->bufSize = 0;
			return 0;
		}
	}

	/* Deblock Chroma */
	if (sa->useDbkCEnable) {
		sa->bufDbkCUse = sramPhyAddr + offset;
		sa->useDbkCEnable = 1;
		switch (codStd) {
		case CODEC_STD_AVC:
			offset = offset + (MbNumX * 128);
			break;
		case CODEC_STD_RV:
			offset = offset + MbNumX * 128;
			break;
		case CODEC_STD_VC1:
			offset = offset + MbNumX * 256;
			break;
		case CODEC_STD_AVS:
			offset = offset + MbNumX * 128;
			break;
		case CODEC_STD_MPEG2:
			offset = offset + MbNumX * 64;
			break;
		case CODEC_STD_VP8:
			offset = offset + MbNumX * 128;
			break;
		default:
			offset = offset + MbNumX * 64;
			break;
		}

		if (offset > sramSize) {
			sa->bufSize = 0;
			return 0;
		}
	}

	/* Deblock Luma */
	if (sa->useDbkYEnable) {
		sa->bufDbkYUse = sramPhyAddr + offset;
		sa->useDbkYEnable = 1;

		switch (codStd) {
		case CODEC_STD_AVC:
			offset = offset + (MbNumX * 128);
			break;
		case CODEC_STD_RV:
			offset = offset + MbNumX * 128;
			break;
		case CODEC_STD_VC1:
			offset = offset + MbNumX * 256;
			break;
		case CODEC_STD_AVS:
			offset = offset + MbNumX * 128;
			break;
		case CODEC_STD_MPEG2:
			offset = offset + MbNumX * 128;
			break;
		case CODEC_STD_VP8:
			offset = offset + MbNumX * 128;
			break;
		default:
			offset = offset + MbNumX * 128;
			break;
		}

		if (offset > sramSize) {
			sa->bufSize = 0;
			return 0;
		}
	}

	/* VC1 Bit-plane */
	if (sa->useBtpEnable) {
		if (codStd != CODEC_STD_VC1) {
			sa->useBtpEnable = 0;
		} else {
			int oneBTP;

			offset = ((offset+255)&~255);
			sa->bufBtpUse = sramPhyAddr + offset;
			sa->useBtpEnable = 1;

			oneBTP  = (((MbNumX+15)/16) * MbNumY + 1) * 2;
			oneBTP  = (oneBTP%256) ? ((oneBTP/256)+1)*256 : oneBTP;

			offset = offset + oneBTP * 3;
		}
	}

	/*VC1 Overlap */
	if (sa->useOvlEnable) {
		if (codStd != CODEC_STD_VC1) {
			sa->useOvlEnable = 0;
		} else {
			sa->bufOvlUse = sramPhyAddr + offset;
			sa->useOvlEnable = 1;

			offset = offset + MbNumX *  80;
		}
	}

	sa->bufSize = offset;

	if (sa->bufSize > sramSize) {
		NX_ErrMsg(("ConfigDecSecAXI() Failed!!!"));
		NX_ErrMsg(("bufSz=%d, sramSz=%d)\n", sa->bufSize, sramSize));
		return VPU_RET_ERR_SRAM;
	}

	return 1;
}

/* Maverick Cache II */
unsigned int MaverickCache2Config(int decoder, int interleave, int bypass,
	int burst, int merge, int mapType, int wayshape)
{
	unsigned int cacheConfig = 0;

	if (decoder) {
		/* LINEAR_FRAME_MAP */
		if (mapType == 0) {
			/* VC1 opposite field padding is not allowable in UV
			separated, burst 8 and linear map */
			if (!interleave)
				burst = 0;

			wayshape = 15;

			if (merge == 1)
				merge = 3;

			/*GDI constraint. Width should not be over 64 */
			if ((merge == 1) && (burst))
				burst = 0;
		} else {
			/*horizontal merge constraint in tiled map */
			if (merge == 1)
				merge = 3;
		}
	} else {
		if (mapType == 0) {
			wayshape = 15;

			/*GDI constraint. Width should not be over 64 */
			if ((merge == 1) && (burst))
				burst = 0;
		} else {
			/*horizontal merge constraint in tiled map */
			if (merge == 1)
				merge = 3;
		}
	}

	cacheConfig = (merge & 0x3) << 9;
	cacheConfig = cacheConfig | ((wayshape & 0xf) << 5);
	cacheConfig = cacheConfig | ((burst & 0x1) << 3);
	cacheConfig = cacheConfig | (bypass & 0x3);

	if (mapType != 0)
		cacheConfig = cacheConfig | 0x00000004;

	return cacheConfig;
}
