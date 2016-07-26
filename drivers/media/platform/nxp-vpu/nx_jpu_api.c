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

#include "vpu_hw_interface.h"		/* Register Access */
#include "nx_vpu_api.h"
#include "nx_port_func.h"
#include "vpu_types.h"


#define	INFO_MSG					0

#define DC_TABLE_INDEX0				0
#define AC_TABLE_INDEX0				1
#define DC_TABLE_INDEX1				2
#define AC_TABLE_INDEX1				3

#define Q_COMPONENT0				0
#define Q_COMPONENT1				0x40
#define Q_COMPONENT2				0x80


enum {
	INT_JPU_DONE = 0,
	INT_JPU_ERROR = 1,
	INT_JPU_BBC_INTERRUPT = 2,
	INT_JPU_BIT_BUF_EMPTY = 3,
	INT_JPU_BIT_BUF_FULL = 3,
	INT_JPU_PARIAL_OVERFLOW = 3
};


/* ----------------------------------------------------------------------------
 * File: VpuJpegTable.h
 *
 * Copyright (c) 2006, Chips & Media.  All rights reserved.
 * -------------------------------------------------------------------------- */
#ifndef JPEG_TABLE_H
#define JPEG_TABLE_H

static unsigned char DefHuffmanBits[4][16] = {
	{
		/* DC index 0 (Luminance DC) */
		0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{
		/* AC index 0 (Luminance AC) */
		0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
		0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D
	},
	{
		/* DC index 1 (Chrominance DC) */
		0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{
		/* AC index 1 (Chrominance AC) */
		0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
		0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77
	}
};

static unsigned char DefHuffmanValue[4][162] = {
	{
		/* DC index 0 (Luminance DC) */
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
		0x0A, 0x0B,
	},
	{
		/* AC index 0 (Luminance AC) */
		0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31,
		0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32,
		0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52,
		0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
		0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A,
		0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45,
		0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57,
		0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
		0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83,
		0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94,
		0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
		0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
		0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
		0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8,
		0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8,
		0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
		0xF9, 0xFA,
	},
	{
		/* DC index 1 (Chrominance DC) */
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
		0x0A, 0x0B,
	},
	{
		/* AC index 1 (Chrominance AC) */
		0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06,
		0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81,
		0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33,
		0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34,
		0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26, 0x27, 0x28,
		0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44,
		0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56,
		0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
		0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
		0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92,
		0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3,
		0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
		0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
		0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6,
		0xD7, 0xD8, 0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
		0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
		0xF9, 0xFA,
	}
};

#ifdef USE_PRE_DEF_QTABLE
static unsigned char lumaQ[64] = {
	0x06, 0x04, 0x04, 0x04, 0x05, 0x04, 0x06, 0x05,
	0x05, 0x06, 0x09, 0x06, 0x05, 0x06, 0x09, 0x0B,
	0x08, 0x06, 0x06, 0x08, 0x0B, 0x0C, 0x0A, 0x0A,
	0x0B, 0x0A, 0x0A, 0x0C, 0x10, 0x0C, 0x0C, 0x0C,
	0x0C, 0x0C, 0x0C, 0x10, 0x0C, 0x0C, 0x0C, 0x0C,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
};

static unsigned char chromaBQ[64] = {
	0x07, 0x07, 0x07, 0x0D, 0x0C, 0x0D, 0x18, 0x10,
	0x10, 0x18, 0x14, 0x0E, 0x0E, 0x0E, 0x14, 0x14,
	0x0E, 0x0E, 0x0E, 0x0E, 0x14, 0x11, 0x0C, 0x0C,
	0x0C, 0x0C, 0x0C, 0x11, 0x11, 0x0C, 0x0C, 0x0C,
	0x0C, 0x0C, 0x0C, 0x11, 0x0C, 0x0C, 0x0C, 0x0C,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
};
#endif

/* These are the sample quantization tables given in JPEG spec section K.1.
 * The spec says that the values given produce "good" quality, and
 * when divided by 2, "very good" quality. */
static unsigned int std_luminance_quant_tbl[64] = {
	16,  11,  10,  16,  24,  40,  51,  61,
	12,  12,  14,  19,  26,  58,  60,  55,
	14,  13,  16,  24,  40,  57,  69,  56,
	14,  17,  22,  29,  51,  87,  80,  62,
	18,  22,  37,  56,  68, 109, 103,  77,
	24,  35,  55,  64,  81, 104, 113,  92,
	49,  64,  78,  87, 103, 121, 120, 101,
	72,  92,  95,  98, 112, 100, 103,  99
};

static unsigned int std_chrominance_quant_tbl[64] = {
	17,  18,  24,  47,  99,  99,  99,  99,
	18,  21,  26,  66,  99,  99,  99,  99,
	24,  26,  56,  99,  99,  99,  99,  99,
	47,  66,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99
};

static unsigned char cInfoTable[5][24] = {
	{ 0, 2, 2, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 3, }, /* 420 */
	{ 0, 2, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 3, }, /* 422H */
	{ 0, 1, 2, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 3, }, /* 422V */
	{ 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 3, }, /* 444 */
	{ 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 3, }, /* 400 */
};

#endif	/* JPEG_TABLE_H */


/*-----------------------------------------------------------------------------
 *      For Jpeg Encoder
 *----------------------------------------------------------------------------*/
static void SetupJpegEncPara(struct enc_jpeg_info *pJpgInfo, int quality)
{
	int scale_factor;
	int i;
	long temp;
	unsigned char *p;
	int format = pJpgInfo->format;
	const int force_baseline = 1;

	if (format == IMG_FORMAT_420) {
		pJpgInfo->busReqNum = 2;
		pJpgInfo->mcuBlockNum = 6;
		pJpgInfo->compNum = 3;
		pJpgInfo->compInfo[0] = 10;
		pJpgInfo->compInfo[1] = 5;
		pJpgInfo->compInfo[2] = 5;
	} else if (format == IMG_FORMAT_422) {
		pJpgInfo->busReqNum = 3;
		pJpgInfo->mcuBlockNum = 4;
		pJpgInfo->compNum = 3;
		pJpgInfo->compInfo[0] = 9;
		pJpgInfo->compInfo[1] = 5;
		pJpgInfo->compInfo[2] = 5;
	} else if (format == IMG_FORMAT_224) {
		pJpgInfo->busReqNum  = 3;
		pJpgInfo->mcuBlockNum = 4;
		pJpgInfo->compNum = 3;
		pJpgInfo->compInfo[0] = 6;
		pJpgInfo->compInfo[1] = 5;
		pJpgInfo->compInfo[2] = 5;
	} else if (format == IMG_FORMAT_444) {
		pJpgInfo->busReqNum = 4;
		pJpgInfo->mcuBlockNum = 3;
		pJpgInfo->compNum = 3;
		pJpgInfo->compInfo[0] = 5;
		pJpgInfo->compInfo[1] = 5;
		pJpgInfo->compInfo[2] = 5;
	} else if (format == IMG_FORMAT_400) {
		pJpgInfo->busReqNum = 4;
		pJpgInfo->mcuBlockNum = 1;
		pJpgInfo->compNum = 1;
		pJpgInfo->compInfo[0] = 5;
		pJpgInfo->compInfo[1] = 0;
		pJpgInfo->compInfo[2] = 0;
	}

	p = &cInfoTable[format][0];
	NX_DrvMemcpy((void *)&pJpgInfo->cInfoTab[0], (void *)p, 6);
	p += 6;
	NX_DrvMemcpy((void *)&pJpgInfo->cInfoTab[1], (void *)p, 6);
	p += 6;
	NX_DrvMemcpy((void *)&pJpgInfo->cInfoTab[2], (void *)p, 6);
	p += 6;
	NX_DrvMemcpy((void *)&pJpgInfo->cInfoTab[3], (void *)p, 6);

	if (quality <= 0)
		quality = 1;
	if (quality > 100)
		quality = 100;

	/* The basic table is used as-is (scaling 100) for a quality of 50.
	* Qualities 50..100 are converted to scaling percentage 200 - 2*Q;
	* note that at Q=100 the scaling is 0, which will cause
	* jpeg_add_quant_table to make all the table entries 1
	* (hence, minimum quantization loss).
	* Qualities 1..50 are converted to scaling percentage 5000/Q. */
	if (quality < 50)
		scale_factor = 5000 / quality;
	else
		scale_factor = 200 - quality*2;

	for (i = 0; i < 64; i++) {
		temp = ((long)std_luminance_quant_tbl[i] * scale_factor + 50L)
			/ 100L;
		/* limit the values to the valid range */
		if (temp <= 0L)
			temp = 1L;
		/* max quantizer needed for 12 bits */
		if (temp > 32767L)
			temp = 32767L;
		/* limit to baseline range if requested */
		if (force_baseline && temp > 255L)
			temp = 255L;

		pJpgInfo->qMatTab[DC_TABLE_INDEX0][i] = (unsigned char)temp;
	}

	for (i = 0; i < 64; i++) {
		temp = ((long)std_chrominance_quant_tbl[i] * scale_factor + 50L)
			/ 100L;
		/* limit the values to the valid range */
		if (temp <= 0L)
			temp = 1L;
		/* max quantizer needed for 12 bits */
		if (temp > 32767L)
			temp = 32767L;
		/* limit to baseline range if requested */
		if (force_baseline && temp > 255L)
			temp = 255L;

		pJpgInfo->qMatTab[AC_TABLE_INDEX0][i] = (unsigned char)temp;
	}

	/* setting of qmatrix table information */
#ifdef USE_CNM_DEFAULT_QMAT_TABLE
	NX_DrvMemcpy((void *)&pJpgInfo->qMatTab[DC_TABLE_INDEX0],
		(void *)lumaQ, 64);
	NX_DrvMemcpy((void *)&pJpgInfo->qMatTab[AC_TABLE_INDEX0],
		(void *)chromaBQ, 64);
#endif

	NX_DrvMemcpy((void *)&pJpgInfo->qMatTab[DC_TABLE_INDEX1],
		(void *)&pJpgInfo->qMatTab[DC_TABLE_INDEX0], 64);
	NX_DrvMemcpy((void *)&pJpgInfo->qMatTab[AC_TABLE_INDEX1],
		(void *)&pJpgInfo->qMatTab[AC_TABLE_INDEX0], 64);

	/* setting of huffman table information */
	/* Luma DC BitLength */
	NX_DrvMemcpy((void *)&pJpgInfo->huffBits[DC_TABLE_INDEX0],
		(void *)&DefHuffmanBits[0][0], 16);

	/* Luma DC HuffValue */
	NX_DrvMemcpy((void *)&pJpgInfo->huffVal[DC_TABLE_INDEX0],
		(void *)&DefHuffmanValue[0][0], 16);
	/* Luma AC BitLength */
	NX_DrvMemcpy((void *)&pJpgInfo->huffBits[AC_TABLE_INDEX0],
		(void *)&DefHuffmanBits[1][0], 16);
	/* Luma AC HuffValue */
	NX_DrvMemcpy((void *)&pJpgInfo->huffVal[AC_TABLE_INDEX0],
		(void *)&DefHuffmanValue[1][0], 162);
	/* Chroma DC BitLength */
	NX_DrvMemcpy((void *)&pJpgInfo->huffBits[DC_TABLE_INDEX1],
		(void *)&DefHuffmanBits[2][0], 16);
	/* Chroma DC HuffValue */
	NX_DrvMemcpy((void *)&pJpgInfo->huffVal[DC_TABLE_INDEX1],
		(void *)&DefHuffmanValue[2][0], 16);
	/* Chroma AC BitLength */
	NX_DrvMemcpy((void *)&pJpgInfo->huffBits[AC_TABLE_INDEX1],
		(void *)&DefHuffmanBits[3][0], 16);
	/* Chorma AC HuffValue */
	NX_DrvMemcpy((void *)&pJpgInfo->huffVal[AC_TABLE_INDEX1],
		(void *)&DefHuffmanValue[3][0], 162);
}

#define PUT_BYTE(_p, _b) \
	{ \
		*_p++ = (unsigned char)(_b); \
		tot++; \
	}

static int EncodeJpegHeader(struct enc_jpeg_info *pJpgInfo)
{
	unsigned char *p = pJpgInfo->jpegHeader;
	int tot = 0;
	int i;
	/* int jfifLen = 16, pad; */

	/* SOI Header */
	PUT_BYTE(p, 0xFF);
	PUT_BYTE(p, 0xD8);

#if 0
	/* JFIF marker Header : Added by Ray Park for Normal Jpeg File */
	PUT_BYTE(p, 0xFF);
	PUT_BYTE(p, 0xE0);
	PUT_BYTE(p, (jfifLen>>8));		/* Legnth */
	PUT_BYTE(p, (jfifLen&0xFF));
	PUT_BYTE(p, 'J');			/* Identifier */
	PUT_BYTE(p, 'F');
	PUT_BYTE(p, 'I');
	PUT_BYTE(p, 'F');
	PUT_BYTE(p, 0x00);
	PUT_BYTE(p, 0x01);		/* Major Version */
	PUT_BYTE(p, 0x01);		/* Minor Version */
	PUT_BYTE(p, 0x00);		/* Density Units */
	PUT_BYTE(p, 0x00);		/* X density */
	PUT_BYTE(p, 0x01);
	PUT_BYTE(p, 0x00);		/* Y density */
	PUT_BYTE(p, 0x01);
	PUT_BYTE(p, 0x00);		/* Thumbnail Width */
	PUT_BYTE(p, 0x00);		/* THumbnail Height */
#endif

	/* APP9 Header */
	PUT_BYTE(p, 0xFF);
	PUT_BYTE(p, 0xE9);

	PUT_BYTE(p, 0x00);
	PUT_BYTE(p, 0x04);

	PUT_BYTE(p, (pJpgInfo->frameIdx >> 8));
	PUT_BYTE(p, (pJpgInfo->frameIdx & 0xFF));

	/* DRI header */
	if (pJpgInfo->rstIntval) {
		PUT_BYTE(p, 0xFF);
		PUT_BYTE(p, 0xDD);

		PUT_BYTE(p, 0x00);
		PUT_BYTE(p, 0x04);

		PUT_BYTE(p, (pJpgInfo->rstIntval >> 8));
		PUT_BYTE(p, (pJpgInfo->rstIntval & 0xff));
	}

	/* DQT Header */
	PUT_BYTE(p, 0xFF);
	PUT_BYTE(p, 0xDB);

	PUT_BYTE(p, 0x00);
	PUT_BYTE(p, 0x43);

	PUT_BYTE(p, 0x00);

	for (i = 0; i < 64; i++)
		PUT_BYTE(p, pJpgInfo->qMatTab[0][i]);

	if (pJpgInfo->format != IMG_FORMAT_400) {
		PUT_BYTE(p, 0xFF);
		PUT_BYTE(p, 0xDB);

		PUT_BYTE(p, 0x00);
		PUT_BYTE(p, 0x43);

		PUT_BYTE(p, 0x01);

		for (i = 0; i < 64; i++)
			PUT_BYTE(p, pJpgInfo->qMatTab[1][i]);
	}

	/* DHT Header */
	PUT_BYTE(p, 0xFF);
	PUT_BYTE(p, 0xC4);

	PUT_BYTE(p, 0x00);
	PUT_BYTE(p, 0x1F);

	PUT_BYTE(p, 0x00);

	for (i = 0; i < 16; i++)
		PUT_BYTE(p, pJpgInfo->huffBits[0][i]);

	for (i = 0; i < 12; i++)
		PUT_BYTE(p, pJpgInfo->huffVal[0][i]);

	PUT_BYTE(p, 0xFF);
	PUT_BYTE(p, 0xC4);

	PUT_BYTE(p, 0x00);
	PUT_BYTE(p, 0xB5);

	PUT_BYTE(p, 0x10);

	for (i = 0; i < 16; i++)
		PUT_BYTE(p, pJpgInfo->huffBits[1][i]);

	for (i = 0; i < 162; i++)
		PUT_BYTE(p, pJpgInfo->huffVal[1][i]);

	if (pJpgInfo->format != IMG_FORMAT_400) {
		PUT_BYTE(p, 0xFF);
		PUT_BYTE(p, 0xC4);

		PUT_BYTE(p, 0x00);
		PUT_BYTE(p, 0x1F);

		PUT_BYTE(p, 0x01);

		for (i = 0; i < 16; i++)
			PUT_BYTE(p, pJpgInfo->huffBits[2][i]);

		for (i = 0; i < 12; i++)
			PUT_BYTE(p, pJpgInfo->huffVal[2][i]);

		PUT_BYTE(p, 0xFF);
		PUT_BYTE(p, 0xC4);

		PUT_BYTE(p, 0x00);
		PUT_BYTE(p, 0xB5);

		PUT_BYTE(p, 0x11);

		for (i = 0; i < 16; i++)
			PUT_BYTE(p, pJpgInfo->huffBits[3][i]);

		for (i = 0; i < 162; i++)
			PUT_BYTE(p, pJpgInfo->huffVal[3][i]);
	}

	/* SOF header */
	PUT_BYTE(p, 0xFF);
	PUT_BYTE(p, 0xC0);

	PUT_BYTE(p, (((8+(pJpgInfo->compNum*3)) >> 8) & 0xFF));
	PUT_BYTE(p, ((8+(pJpgInfo->compNum*3)) & 0xFF));

	PUT_BYTE(p, 0x08);

	if (pJpgInfo->rotationEnable && (pJpgInfo->rotationAngle == 90 ||
		pJpgInfo->rotationAngle == 270)) {
		PUT_BYTE(p, (pJpgInfo->picWidth >> 8));
		PUT_BYTE(p, (pJpgInfo->picWidth & 0xFF));
		PUT_BYTE(p, (pJpgInfo->picHeight >> 8));
		PUT_BYTE(p, (pJpgInfo->picHeight & 0xFF));
	} else {
		PUT_BYTE(p, (pJpgInfo->picHeight >> 8));
		PUT_BYTE(p, (pJpgInfo->picHeight & 0xFF));
		PUT_BYTE(p, (pJpgInfo->picWidth >> 8));
		PUT_BYTE(p, (pJpgInfo->picWidth & 0xFF));
	}

	PUT_BYTE(p, pJpgInfo->compNum);

	for (i = 0; i < pJpgInfo->compNum; i++) {
		PUT_BYTE(p, (i+1));
		PUT_BYTE(p, ((pJpgInfo->cInfoTab[i][1]<<4) & 0xF0) +
			(pJpgInfo->cInfoTab[i][2] & 0x0F));
		PUT_BYTE(p, pJpgInfo->cInfoTab[i][3]);
	}

	/* tot = p - para->pParaSet; */

	if (tot % 8) {
		int pad = tot % 8;

		pad = 8 - pad;
		for (i = 0; i < pad; i++)
			PUT_BYTE(p, 0x00);
	}

	pJpgInfo->frameIdx++;
	pJpgInfo->headerSize = tot;

	return VPU_RET_OK;
}

static int GenerateJpegEncHuffmanTable(struct enc_jpeg_info *pJpgInfo,
	int tabNum)
{
	static int huffsize[256];
	static int huffcode[256];
	int p, i, l, lastp, si, maxsymbol, code;
	unsigned char *bitleng, *huffval;
	unsigned int *ehufco, *ehufsi;

	bitleng	= pJpgInfo->huffBits[tabNum];
	huffval	= pJpgInfo->huffVal[tabNum];
	ehufco	= pJpgInfo->huffCode[tabNum];
	ehufsi	= pJpgInfo->huffSize[tabNum];

	maxsymbol = tabNum & 1 ? 256 : 16;

	/* Figure C.1: make table of Huffman code length for each symbol */
	p = 0;
	for (l = 1; l <= 16; l++) {
		i = bitleng[l-1];
		if (i < 0 || p + i > maxsymbol)
			return -1;
		while (i--)
			huffsize[p++] = l;
	}
	lastp = p;

	/* Figure C.2: generate the codes themselves */
	/* We also validate that the counts represent a legal Huffman
	 * code tree. */
	code = 0;
	si = huffsize[0];
	p = 0;
	while (p < lastp) {
		while (huffsize[p] == si) {
			huffcode[p++] = code;
			code++;
		}
		if (code >= (1 << si))
			return -1;
		code <<= 1;
		si++;
	}

	/* Figure C.3: generate encoding tables */
	/* These are code and size indexed by symbol value */
	for (i = 0; i < 256; i++)
		ehufsi[i] = 0x00;

	for (i = 0; i < 256; i++)
		ehufco[i] = 0x00;

	for (p = 0; p < lastp; p++) {
		i = huffval[p];
		if (i < 0 || i >= maxsymbol || ehufsi[i])
			return -1;
		ehufco[i] = huffcode[p];
		ehufsi[i] = huffsize[p];
	}

	return 0;
}

static int LoadJpegEncHuffmanTable(struct enc_jpeg_info *pJpgInfo)
{
	int i, j, t;
	int huffData;

	for (i = 0; i < 4; i++)
		GenerateJpegEncHuffmanTable(pJpgInfo, i);

	VpuWriteReg(MJPEG_HUFF_CTRL_REG, 0x3);

	for (j = 0; j < 4; j++) {
		t = (j == 0) ? AC_TABLE_INDEX0 : (j == 1) ? AC_TABLE_INDEX1 :
			(j == 2) ? DC_TABLE_INDEX0 : DC_TABLE_INDEX1;

		for (i = 0; i < 256; i++) {
			/* DC */
			if ((t == DC_TABLE_INDEX0 || t == DC_TABLE_INDEX1) &&
				(i > 15))
				break;

			if ((pJpgInfo->huffSize[t][i] == 0) &&
				(pJpgInfo->huffCode[t][i] == 0)) {
				huffData = 0;
			} else {
				/* Code length (1 ~ 16), 4-bit */
				huffData = (pJpgInfo->huffSize[t][i] - 1);
				/* Code word, 16-bit */
				huffData = (huffData << 16) |
					(pJpgInfo->huffCode[t][i]);
			}

			VpuWriteReg(MJPEG_HUFF_DATA_REG, huffData);
		}
	}

	VpuWriteReg(MJPEG_HUFF_CTRL_REG, 0x0);

	return 0;
}

static int LoadJpegEncQuantizaitonTable(struct enc_jpeg_info *pJpgInfo)
{
	long quotient, dividend = 0x80000;
	int quantID, divisor, comp, i, t;

	for (comp = 0; comp < 3; comp++) {
		quantID = pJpgInfo->cInfoTab[comp][3];
		if (quantID >= 4)
			return -1;
		t = (comp == 0) ? Q_COMPONENT0 :
			(comp == 1) ? Q_COMPONENT1 : Q_COMPONENT2;
		VpuWriteReg(MJPEG_QMAT_CTRL_REG, 0x3 + t);
		for (i = 0; i < 64; i++) {
			divisor = pJpgInfo->qMatTab[quantID][i];
			quotient = dividend / divisor;
			VpuWriteReg(MJPEG_QMAT_DATA_REG, (int)quotient);
		}
		VpuWriteReg(MJPEG_QMAT_CTRL_REG, t);
	}

	return 0;
}

int JPU_EncRunFrame(struct nx_vpu_codec_inst *pInst,
	struct vpu_enc_run_frame_arg *runArg)
{
	int val, reason;
	unsigned int rotMirMode;
	struct vpu_enc_info *pInfo = &pInst->codecInfo.encInfo;
	struct enc_jpeg_info *pJpgInfo = &pInfo->enc_codec_para.jpgEncInfo;
	int yuv_format = pJpgInfo->format;
	unsigned int mapEnable;
	int stride = runArg->inImgBuffer.stride[0];

	NX_DbgMsg(INFO_MSG, ("Jpeg Encode Info : Rotate = %d, Mirror = %d\n",
		pJpgInfo->rotationAngle, pJpgInfo->mirrorDirection));

	if (pJpgInfo->headerSize == 0) {
		SetupJpegEncPara(pJpgInfo, pInfo->jpegQuality);
		EncodeJpegHeader(pJpgInfo);

		NX_DrvMemcpy((void *)pInfo->strmBufVirAddr,
			(void *)pJpgInfo->jpegHeader, pJpgInfo->headerSize);

		pInfo->strmBufPhyAddr += pJpgInfo->headerSize;
		pInfo->strmBufSize -= pJpgInfo->headerSize;
	}

	pJpgInfo->alignedWidth = ((pInfo->srcWidth+15)/16)*16;
	pJpgInfo->alignedHeight = ((pInfo->srcHeight+15)/16)*16;

	VpuWriteReg(MJPEG_BBC_BAS_ADDR_REG, pInfo->strmBufPhyAddr);
	VpuWriteReg(MJPEG_BBC_END_ADDR_REG, pInfo->strmBufPhyAddr +
		pInfo->strmBufSize);
	VpuWriteReg(MJPEG_BBC_WR_PTR_REG, pInfo->strmBufPhyAddr);
	VpuWriteReg(MJPEG_BBC_RD_PTR_REG, pInfo->strmBufPhyAddr);
	VpuWriteReg(MJPEG_BBC_CUR_POS_REG, 0);
	/* 64 * 4 byte == 32 * 8 byte */
	VpuWriteReg(MJPEG_BBC_DATA_CNT_REG, 256 / 4);
	VpuWriteReg(MJPEG_BBC_EXT_ADDR_REG, pInfo->strmBufPhyAddr);
	VpuWriteReg(MJPEG_BBC_INT_ADDR_REG, 0);

	/* JpgEncGbuResetReg */
	VpuWriteReg(MJPEG_GBU_BT_PTR_REG, 0);
	VpuWriteReg(MJPEG_GBU_WD_PTR_REG, 0);
	VpuWriteReg(MJPEG_GBU_BBSR_REG, 0);

	VpuWriteReg(MJPEG_GBU_BBER_REG, ((256 / 4) * 2) - 1);
	/* 64 * 4 byte == 32 * 8 byte */
	VpuWriteReg(MJPEG_GBU_BBIR_REG, 256 / 4);
	VpuWriteReg(MJPEG_GBU_BBHR_REG, 256 / 4);

	VpuWriteReg(MJPEG_PIC_CTRL_REG, 0x18);

	VpuWriteReg(MJPEG_PIC_SIZE_REG, pJpgInfo->alignedWidth<<16 |
		pJpgInfo->alignedHeight);

	rotMirMode = 0;

	if (pJpgInfo->rotationEnable) {
		switch (pJpgInfo->rotationAngle) {
		case 90:
			rotMirMode |= 0x1;
			break;
		case 180:
			rotMirMode |= 0x2;
			break;
		case 270:
			rotMirMode |= 0x3;
			break;
		default:
			rotMirMode |= 0x0;
		}
	}

	if (pJpgInfo->mirrorEnable) {
		switch (pJpgInfo->mirrorDirection) {
		case MIRDIR_VER:
			rotMirMode |= 0x4;
			break;
		case MIRDIR_HOR:
			rotMirMode |= 0x8;
			break;
		case MIRDIR_HOR_VER:
			rotMirMode |= 0xC;
			break;
		default:
			rotMirMode |= 0x0;
		}
	}

	if (pJpgInfo->rotationEnable || pJpgInfo->rotationEnable)
		rotMirMode |= 0x10;

	VpuWriteReg(MJPEG_ROT_INFO_REG, rotMirMode);

	if (rotMirMode & 0x01)
		yuv_format = (pJpgInfo->format == IMG_FORMAT_422) ?
			IMG_FORMAT_224 : (pJpgInfo->format == IMG_FORMAT_224) ?
			IMG_FORMAT_422 : pJpgInfo->format;

	if (yuv_format == IMG_FORMAT_422) {
		if (rotMirMode & 1)
			pJpgInfo->compInfo[0] = 6;
		else
			pJpgInfo->compInfo[0] = 9;
	} else if (yuv_format == IMG_FORMAT_224) {
		if (rotMirMode & 1)
			pJpgInfo->compInfo[0] = 9;
		else
			pJpgInfo->compInfo[0] = 6;
	}

	VpuWriteReg(MJPEG_MCU_INFO_REG, pJpgInfo->mcuBlockNum << 16 |
		pJpgInfo->compNum << 12 | pJpgInfo->compInfo[0] << 8 |
		pJpgInfo->compInfo[1] << 4 | pJpgInfo->compInfo[2]);

	VpuWriteReg(MJPEG_SCL_INFO_REG, 0);
	VpuWriteReg(MJPEG_DPB_CONFIG_REG, VPU_FRAME_BUFFER_ENDIAN << 1 |
		CBCR_INTERLEAVE);
	VpuWriteReg(MJPEG_RST_INTVAL_REG, pJpgInfo->rstIntval);
	VpuWriteReg(MJPEG_BBC_CTRL_REG, ((VPU_STREAM_ENDIAN & 3) << 1) | 1);

	VpuWriteReg(MJPEG_OP_INFO_REG, pJpgInfo->busReqNum);

	if (LoadJpegEncHuffmanTable(pJpgInfo) < 0)
		return VPU_RET_ERR_PARAM;

	if (LoadJpegEncQuantizaitonTable(pJpgInfo) < 0)
		return VPU_RET_ERR_PARAM;

	/* gdi status */
	val = 0;
	VpuWriteReg(GDI_CONTROL, 1);
	while (!val)
		val = VpuReadReg(GDI_STATUS);

	mapEnable = 0;

	VpuWriteReg(GDI_INFO_CONTROL, (mapEnable << 20) |
		((pJpgInfo->format & 0x07) << 17) | (CBCR_INTERLEAVE << 16) |
		stride);
	VpuWriteReg(GDI_INFO_PIC_SIZE, (pJpgInfo->alignedWidth << 16) |
		pJpgInfo->alignedHeight);

	VpuWriteReg(GDI_INFO_BASE_Y,  runArg->inImgBuffer.phyAddr[0]);
	VpuWriteReg(GDI_INFO_BASE_CB, runArg->inImgBuffer.phyAddr[1]);
	VpuWriteReg(GDI_INFO_BASE_CR, runArg->inImgBuffer.phyAddr[2]);

	VpuWriteReg(MJPEG_DPB_BASE00_REG, 0);

	VpuWriteReg(GDI_CONTROL, 0);
	VpuWriteReg(GDI_PIC_INIT_HOST, 1);

	VpuWriteReg(MJPEG_PIC_START_REG, 1);

	/* Wait Jpeg Interrupt */
	reason = JPU_WaitInterrupt(pInst->devHandle, JPU_ENC_TIMEOUT);

	if (reason == 0)
		return VPU_RET_ERR_TIMEOUT;

	if (reason != 1) {
		NX_ErrMsg(("JPG Encode Error(reason = 0x%08x)\n", reason));
		return VPU_RET_ERROR;
	}

	/* Post Porcessing */
	val = VpuReadReg(MJPEG_PIC_STATUS_REG);
	if ((val & 0x4) >> 2) {
		NX_ErrMsg(("JPG Encode Error(reason = 0x%08x)\n", reason));
		NX_ErrMsg((" : VPU_RET_ERR_WRONG_SEQ\n"));
		return VPU_RET_ERR_WRONG_SEQ;
	}

	if (val != 0)
		VpuWriteReg(MJPEG_PIC_STATUS_REG, val);

	runArg->outStreamAddr = (uint64_t)pInfo->strmBufVirAddr;
	runArg->outStreamSize = VpuReadReg(MJPEG_BBC_WR_PTR_REG) -
		pInfo->strmBufPhyAddr + pJpgInfo->headerSize;

	VpuWriteReg(MJPEG_BBC_FLUSH_CMD_REG, 0);

	return VPU_RET_OK;
}


/*-----------------------------------------------------------------------------
 *      For Jpeg Decoder
 *----------------------------------------------------------------------------*/
struct vld_stream {
	uint32_t	dwUsedBits;
	uint8_t *pbyStart;
	uint32_t	dwPktSize;
};

static uint32_t vld_get_bits(struct vld_stream *pstVldStm, int32_t iBits)
{
	uint32_t dwUsedBits = pstVldStm->dwUsedBits;
	int32_t iBitCnt = 8 - (dwUsedBits & 0x7);
	uint8_t *pbyRead = (uint8_t *)pstVldStm->pbyStart + (dwUsedBits >> 3);
	uint32_t dwRead;

	pstVldStm->dwUsedBits += iBits;

	dwRead  = *pbyRead++ << 24;
	if (iBits > iBitCnt) {
		dwRead  += *pbyRead++ << 16;
		if (iBits > iBitCnt + 8) {
			dwRead  += *pbyRead++ << 8;
			if (iBits > iBitCnt + 16)
				dwRead  += *pbyRead++;
		}
	}

	return (dwRead << (8 - iBitCnt)) >> (32 - iBits);
}

static void vld_flush_bits(struct vld_stream *pstVldStm, int32_t iBits)
{
	pstVldStm->dwUsedBits += iBits;
}

static void SetJpegDecHuffmanTable(struct vpu_dec_info *pInfo)
{
	int i, j;
	unsigned int HuffData;
	int HuffLength;
	int temp;

	if (pInfo->userHuffTable == 0)
		return;

	/* MIN Table */
	VpuWriteReg(MJPEG_HUFF_CTRL_REG, 3);

	/* DC Luma */
	for (j = 0 ; j < 16 ; j++) {
		HuffData = pInfo->huffMin[0][j];
		temp = (HuffData & 0x8000) >> 15;
		temp = (temp << 15) | (temp << 14) | (temp << 13) |
			(temp << 12) | (temp << 11) | (temp << 10) |
			(temp << 9) | (temp << 8) | (temp << 7) |
			(temp << 6) | (temp << 5) | (temp << 4) |
			(temp << 3) | (temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFF) << 16) |
			HuffData);
	}

	/* DC Chroma */
	for (j = 0; j < 16; j++) {
		HuffData = pInfo->huffMin[2][j];
		temp = (HuffData & 0x8000) >> 15;
		temp = (temp << 15) | (temp << 14) | (temp << 13) |
			(temp << 12) | (temp << 11) | (temp << 10) |
			(temp << 9) | (temp << 8) | (temp << 7) |
			(temp << 6) | (temp << 5) | (temp << 4) |
			(temp << 3) | (temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFF) << 16) |
			HuffData);
	}

	/* AC Luma */
	for (j = 0; j < 16; j++) {
		HuffData = pInfo->huffMin[1][j];
		temp = (HuffData & 0x8000) >> 15;
		temp = (temp << 15) | (temp << 14) | (temp << 13) |
			(temp << 12) | (temp << 11) | (temp << 10) |
			(temp << 9) | (temp << 8) | (temp << 7) |
			(temp << 6) | (temp << 5) | (temp << 4) |
			(temp << 3) | (temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFF) << 16) |
			HuffData);
	}

	/* AC Chroma */
	for (j = 0; j < 16; j++) {
		HuffData = pInfo->huffMin[3][j];
		temp = (HuffData & 0x8000) >> 15;
		temp = (temp << 15) | (temp << 14) | (temp << 13) |
			(temp << 12) | (temp << 11) | (temp << 10) |
			(temp << 9) | (temp << 8) | (temp << 7) |
			(temp << 6) | (temp << 5) | (temp << 4) |
			(temp << 3) | (temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFF) << 16) |
			HuffData);
	}

	/* MAX Tables */
	VpuWriteReg(MJPEG_HUFF_CTRL_REG, 0x403);
	VpuWriteReg(MJPEG_HUFF_ADDR_REG, 0x440);

	/* DC Luma */
	for (j = 0; j < 16; j++) {
		HuffData = pInfo->huffMax[0][j];
		temp = (HuffData & 0x8000) >> 15;
		temp = (temp << 15) | (temp << 14) | (temp << 13) |
			(temp << 12) | (temp << 11) | (temp << 10) |
			(temp << 9) | (temp << 8) | (temp << 7) |
			(temp << 6) | (temp << 5) | (temp << 4) |
			(temp << 3) | (temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFF) << 16) |
			HuffData);
	}

	/* DC Chroma */
	for (j = 0; j < 16; j++) {
		HuffData = pInfo->huffMax[2][j];
		temp = (HuffData & 0x8000) >> 15;
		temp = (temp << 15) | (temp << 14) | (temp << 13) |
			(temp << 12) | (temp << 11) | (temp << 10) |
			(temp << 9) | (temp << 8) | (temp << 7) |
			(temp << 6) | (temp << 5) | (temp << 4) |
			(temp << 3) | (temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFF) << 16) |
			HuffData);
	}

	/* AC Luma */
	for (j = 0; j < 16; j++) {
		HuffData = pInfo->huffMax[1][j];
		temp = (HuffData & 0x8000) >> 15;
		temp = (temp << 15) | (temp << 14) | (temp << 13) |
			(temp << 12) | (temp << 11) | (temp << 10) |
			(temp << 9) | (temp << 8) | (temp << 7) |
			(temp << 6) | (temp << 5) | (temp << 4) |
			(temp << 3) | (temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFF) << 16) |
			HuffData);
	}

	/* AC Chroma */
	for (j = 0; j < 16; j++) {
		HuffData = pInfo->huffMax[3][j];
		temp = (HuffData & 0x8000) >> 15;
		temp = (temp << 15) | (temp << 14) | (temp << 13) |
			(temp << 12) | (temp << 11) | (temp << 10) |
			(temp << 9) | (temp << 8) | (temp << 7) |
			(temp << 6) | (temp << 5) | (temp << 4) |
			(temp << 3) | (temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFF) << 16) |
			HuffData);
	}

	/* PTR Tables */
	VpuWriteReg(MJPEG_HUFF_CTRL_REG, 0x803);
	VpuWriteReg(MJPEG_HUFF_ADDR_REG, 0x880);

	/* DC Luma */
	for (j = 0; j < 16; j++) {
		HuffData = pInfo->huffPtr[0][j];
		temp = (HuffData & 0x80) >> 7;
		temp = (temp << 23) | (temp << 22) | (temp << 21) |
			(temp << 20) | (temp << 19) | (temp << 18) |
			(temp << 17) | (temp << 16) | (temp << 15) |
			(temp << 14) | (temp << 13) | (temp << 12) |
			(temp << 11) | (temp << 10) | (temp << 9) |
			(temp << 8) | (temp << 7) | (temp << 6) |
			(temp << 5) | (temp << 4) | (temp << 3) |
			(temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFFFF) << 8) |
			HuffData);
	}

	/* DC Chroma */
	for (j = 0; j < 16; j++) {
		HuffData = pInfo->huffPtr[2][j];
		temp = (HuffData & 0x80) >> 7;
		temp = (temp << 23) | (temp << 22) | (temp << 21) |
			(temp << 20) | (temp << 19) | (temp << 18) |
			(temp << 17) | (temp << 16) | (temp << 15) |
			(temp << 14) | (temp << 13) | (temp << 12) |
			(temp << 11) | (temp << 10) | (temp << 9) |
			(temp << 8) | (temp << 7) | (temp << 6) |
			(temp << 5) | (temp << 4) | (temp << 3) |
			(temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFFFF) << 8) |
			HuffData);
	}

	/* AC Luma */
	for (j = 0; j < 16; j++) {
		HuffData = pInfo->huffPtr[1][j];
		temp = (HuffData & 0x80) >> 7;
		temp = (temp << 23) | (temp << 22) | (temp << 21) |
			(temp << 20) | (temp << 19) | (temp << 18) |
			(temp << 17) | (temp << 16) | (temp << 15) |
			(temp << 14) | (temp << 13) | (temp << 12) |
			(temp << 11) | (temp << 10) | (temp << 9) |
			(temp << 8) | (temp << 7) | (temp << 6) |
			(temp << 5) | (temp << 4) | (temp << 3) |
			(temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFFFF) << 8) |
			HuffData);
	}

	/* AC Chroma */
	for (j = 0; j < 16; j++) {
		HuffData = pInfo->huffPtr[3][j];
		temp = (HuffData & 0x80) >> 7;
		temp = (temp << 23) | (temp << 22) | (temp << 21) |
			(temp << 20) | (temp << 19) | (temp << 18) |
			(temp << 17) | (temp << 16) | (temp << 15) |
			(temp << 14) | (temp << 13) | (temp << 12) |
			(temp << 11) | (temp << 10) | (temp << 9) |
			(temp << 8) | (temp << 7) | (temp << 6) |
			(temp << 5) | (temp << 4) | (temp << 3) |
			(temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFFFF) << 8) |
			HuffData);
	}

	/* VAL Tables */
	VpuWriteReg(MJPEG_HUFF_CTRL_REG, 0xC03);

	/* VAL DC Luma */
	HuffLength = 0;
	for (i = 0; i < 12; i++)
		HuffLength += pInfo->huffBits[0][i];

	/* 8-bit, 12 row, 1 category (DC Luma) */
	for (i = 0; i < HuffLength; i++) {
		HuffData = pInfo->huffValue[0][i];
		temp = (HuffData & 0x80) >> 7;
		temp = (temp << 23) | (temp << 22) | (temp << 21) |
			(temp << 20) | (temp << 19) | (temp << 18) |
			(temp << 17) | (temp << 16) | (temp << 15) |
			(temp << 14) | (temp << 13) | (temp << 12) |
			(temp << 11) | (temp << 10) | (temp << 9) |
			(temp << 8) | (temp << 7) | (temp << 6) |
			(temp << 5) | (temp << 4) | (temp << 3) |
			(temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFFFF) << 8) |
			HuffData);
	}

	for (i = 0; i < 12 - HuffLength; i++)
		VpuWriteReg(MJPEG_HUFF_DATA_REG, 0xFFFFFFFF);

	/* VAL DC Chroma */
	HuffLength = 0;
	for (i = 0; i < 12; i++)
		HuffLength += pInfo->huffBits[2][i];

	/* 8-bit, 12 row, 1 category (DC Chroma) */
	for (i = 0; i < HuffLength; i++) {
		HuffData = pInfo->huffValue[2][i];
		temp = (HuffData & 0x80) >> 7;
		temp = (temp << 23) | (temp << 22) | (temp << 21) |
			(temp << 20) | (temp << 19) | (temp << 18) |
			(temp << 17) | (temp << 16) | (temp << 15) |
			(temp << 14) | (temp << 13) | (temp << 12) |
			(temp << 11) | (temp << 10) | (temp << 9) |
			(temp << 8) | (temp << 7) | (temp << 6) |
			(temp << 5) | (temp << 4) | (temp << 3) |
			(temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFFFF) << 8) |
			HuffData);
	}

	for (i = 0; i < 12 - HuffLength; i++)
		VpuWriteReg(MJPEG_HUFF_DATA_REG, 0xFFFFFFFF);

	/* VAL AC Luma */
	HuffLength = 0;
	for (i = 0; i < 162; i++)
		HuffLength += pInfo->huffBits[1][i];

	/* 8-bit, 162 row, 1 category (AC Luma) */
	for (i = 0; i < HuffLength; i++) {
		HuffData = pInfo->huffValue[1][i];
		temp = (HuffData & 0x80) >> 7;
		temp = (temp << 23) | (temp << 22) | (temp << 21) |
			(temp << 20) | (temp << 19) | (temp << 18) |
			(temp << 17) | (temp << 16) | (temp << 15) |
			(temp << 14) | (temp << 13) | (temp << 12) |
			(temp << 11) | (temp << 10) | (temp << 9) |
			(temp << 8) | (temp << 7) | (temp << 6) |
			(temp << 5) | (temp << 4) | (temp << 3) |
			(temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFFFF) << 8) |
			HuffData);
	}

	for (i = 0; i < 162 - HuffLength; i++)
		VpuWriteReg(MJPEG_HUFF_DATA_REG, 0xFFFFFFFF);

	/* VAL AC Chroma */
	HuffLength = 0;
	for (i = 0; i < 162; i++)
		HuffLength += pInfo->huffBits[3][i];

	/* 8-bit, 162 row, 1 category (AC Chroma) */
	for (i = 0; i < HuffLength; i++) {
		HuffData = pInfo->huffValue[3][i];
		temp = (HuffData & 0x80) >> 7;
		temp = (temp << 23) | (temp << 22) | (temp << 21) |
			(temp << 20) | (temp << 19) | (temp << 18) |
			(temp << 17) | (temp << 16) | (temp << 15) |
			(temp << 14) | (temp << 13) | (temp << 12) |
			(temp << 11) | (temp << 10) | (temp << 9) |
			(temp << 8) | (temp << 7) | (temp << 6) |
			(temp << 5) | (temp << 4) | (temp << 3) |
			(temp << 2) | (temp << 1) | (temp);
		VpuWriteReg(MJPEG_HUFF_DATA_REG, ((temp & 0xFFFFFF) << 8) |
			HuffData);
	}

	for (i = 0; i < 162 - HuffLength; i++)
		VpuWriteReg(MJPEG_HUFF_DATA_REG, 0xFFFFFFFF);

	/* end SerPeriHuffTab */
	VpuWriteReg(MJPEG_HUFF_CTRL_REG, 0x000);
}

static void SetJpegDecQuantizationTable(struct vpu_dec_info *pInfo)
{
	int i;
	int table;
	int val;

	/* SetPeriQMatTab */
	/* Comp 0 */
	VpuWriteReg(MJPEG_QMAT_CTRL_REG, 0x03);
	table = pInfo->infoTable[0][3];
	for (i = 0; i < 64; i++) {
		val = pInfo->quantTable[table][i];
		VpuWriteReg(MJPEG_QMAT_DATA_REG, val);
	}
	VpuWriteReg(MJPEG_QMAT_CTRL_REG, 0x00);

	/* Comp 1 */
	VpuWriteReg(MJPEG_QMAT_CTRL_REG, 0x43);
	table = pInfo->infoTable[1][3];
	for (i = 0; i < 64; i++) {
		val = pInfo->quantTable[table][i];
		VpuWriteReg(MJPEG_QMAT_DATA_REG, val);
	}
	VpuWriteReg(MJPEG_QMAT_CTRL_REG, 0x00);

	/* Comp 2 */
	VpuWriteReg(MJPEG_QMAT_CTRL_REG, 0x83);
	table = pInfo->infoTable[2][3];
	for (i = 0; i < 64; i++) {
		val = pInfo->quantTable[table][i];
		VpuWriteReg(MJPEG_QMAT_DATA_REG, val);
	}
	VpuWriteReg(MJPEG_QMAT_CTRL_REG, 0x00);
}

static void SetupJpegDecGram(struct vpu_dec_info *pInfo)
{
	int dExtBitBufCurPos;
	int dExtBitBufBaseAddr;
	int dMibStatus = 1;

	dExtBitBufCurPos = pInfo->pagePtr;
	dExtBitBufBaseAddr = pInfo->strmBufPhyAddr;

	VpuWriteReg(MJPEG_BBC_CUR_POS_REG, dExtBitBufCurPos);
	VpuWriteReg(MJPEG_BBC_EXT_ADDR_REG, dExtBitBufBaseAddr +
		(dExtBitBufCurPos << 8));
	VpuWriteReg(MJPEG_BBC_INT_ADDR_REG, (dExtBitBufCurPos & 1) << 6);
	/* 64 * 4 byte == 32 * 8 byte */
	VpuWriteReg(MJPEG_BBC_DATA_CNT_REG, 256 / 4);
	VpuWriteReg(MJPEG_BBC_COMMAND_REG, (VPU_STREAM_ENDIAN << 1) | 0);

	while (dMibStatus == 1)
		dMibStatus = VpuReadReg(MJPEG_BBC_BUSY_REG);

	dMibStatus = 1;
	dExtBitBufCurPos = dExtBitBufCurPos + 1;

	VpuWriteReg(MJPEG_BBC_CUR_POS_REG, dExtBitBufCurPos);
	VpuWriteReg(MJPEG_BBC_EXT_ADDR_REG, dExtBitBufBaseAddr +
		(dExtBitBufCurPos << 8));
	VpuWriteReg(MJPEG_BBC_INT_ADDR_REG, (dExtBitBufCurPos & 1) << 6);
	/* 64 * 4 byte == 32 * 8 byte */
	VpuWriteReg(MJPEG_BBC_DATA_CNT_REG, 256 / 4);
	VpuWriteReg(MJPEG_BBC_COMMAND_REG, (VPU_STREAM_ENDIAN << 1) | 0);

	while (dMibStatus == 1)
		dMibStatus = VpuReadReg(MJPEG_BBC_BUSY_REG);

	dMibStatus = 1;
	dExtBitBufCurPos = dExtBitBufCurPos + 1;

	/* next uint page pointe */
	VpuWriteReg(MJPEG_BBC_CUR_POS_REG, dExtBitBufCurPos);

	VpuWriteReg(MJPEG_BBC_CTRL_REG, ((VPU_STREAM_ENDIAN & 3) << 1) | 1);

	VpuWriteReg(MJPEG_GBU_WD_PTR_REG, pInfo->wordPtr);
	VpuWriteReg(MJPEG_GBU_BBSR_REG, 0);
	VpuWriteReg(MJPEG_GBU_BBER_REG, ((256 / 4) * 2) - 1);

	if (pInfo->pagePtr & 1) {
		VpuWriteReg(MJPEG_GBU_BBIR_REG, 0);
		VpuWriteReg(MJPEG_GBU_BBHR_REG, 0);
	} else {
		/* 64 * 4 byte == 32 * 8 byte */
		VpuWriteReg(MJPEG_GBU_BBIR_REG, 256 / 4);
		VpuWriteReg(MJPEG_GBU_BBHR_REG, 256 / 4);
	}

	VpuWriteReg(MJPEG_GBU_CTRL_REG, 4);
	VpuWriteReg(MJPEG_GBU_FF_RPTR_REG, pInfo->bitPtr);
}

static int32_t DecodeJpegSOF(struct vpu_dec_info *pInfo,
	struct vld_stream *pstStrm)
{
	int32_t iWidth, iHeight;
	int32_t iCompNum;
	int32_t iSampleFactor;
	int32_t i;

	/* frame header length */
	vld_flush_bits(pstStrm, 16);
	/* sample precision */
	if (vld_get_bits(pstStrm, 8) != 8)
		return -1;

	iHeight = vld_get_bits(pstStrm, 16);
	iWidth  = vld_get_bits(pstStrm, 16);

	/* number of image components in frame */
	iCompNum = vld_get_bits(pstStrm, 8);
	if (iCompNum > 3)
		return -1;

	for (i = 0 ; i < iCompNum ; i++) {
		/* CompId */
		pInfo->infoTable[i][0] = vld_get_bits(pstStrm, 8);
		/* HSampligFactor */
		pInfo->infoTable[i][1] = vld_get_bits(pstStrm, 4);
		/* VSampligFactor */
		pInfo->infoTable[i][2] = vld_get_bits(pstStrm, 4);
		/* QTableDestSelector */
		pInfo->infoTable[i][3] = vld_get_bits(pstStrm, 8);
	}

	if (iCompNum == 1) {
		pInfo->imgFormat = IMG_FORMAT_400;
		pInfo->width = (iWidth + 7) & (~7);
		pInfo->height = (iHeight + 7) & (~7);
		pInfo->busReqNum = 4;
		pInfo->mcuBlockNum = 1;
		pInfo->compNum = 1;
		pInfo->compInfo[0] = 5;
		pInfo->compInfo[1] = 0;
		pInfo->compInfo[2] = 0;
		pInfo->mcuWidth = 8;
		pInfo->mcuHeight = 8;
	} else if (iCompNum == 3) {
		iSampleFactor = ((pInfo->infoTable[0][1]&3) << 4) |
			(pInfo->infoTable[0][2]&3);
		switch (iSampleFactor) {
		case 0x11:
			pInfo->imgFormat = IMG_FORMAT_444;
			pInfo->width = (iWidth + 7) & (~7);
			pInfo->height = (iHeight + 7) & (~7);
			pInfo->busReqNum = 4;
			pInfo->mcuBlockNum = 3;
			pInfo->compNum = 3;
			pInfo->compInfo[0] = 5;
			pInfo->compInfo[1] = 5;
			pInfo->compInfo[2] = 5;
			pInfo->mcuWidth = 8;
			pInfo->mcuHeight = 8;
			break;
		case 0x12:
			pInfo->imgFormat = IMG_FORMAT_224;
			pInfo->width = (iWidth + 7) & (~7);
			pInfo->height = (iHeight + 15) & (~15);
			pInfo->busReqNum = 3;
			pInfo->mcuBlockNum = 4;
			pInfo->compNum = 3;
			pInfo->compInfo[0] = 6;
			pInfo->compInfo[1] = 5;
			pInfo->compInfo[2] = 5;
			pInfo->mcuWidth = 8;
			pInfo->mcuHeight = 16;
			break;
		case 0x21:
			pInfo->imgFormat = IMG_FORMAT_422;
			pInfo->width = (iWidth + 15) & (~15);
			pInfo->height = (iHeight + 7) & (~7);
			pInfo->busReqNum = 3;
			pInfo->mcuBlockNum = 4;
			pInfo->compNum = 3;
			pInfo->compInfo[0] = 9;
			pInfo->compInfo[1] = 5;
			pInfo->compInfo[2] = 5;
			pInfo->mcuWidth = 16;
			pInfo->mcuHeight = 8;
			break;
		case 0x22:
			pInfo->imgFormat = IMG_FORMAT_420;
			pInfo->width = (iWidth + 15) & (~15);
			pInfo->height = (iHeight + 15) & (~15);
			pInfo->busReqNum = 2;
			pInfo->mcuBlockNum = 6;
			pInfo->compNum = 3;
			pInfo->compInfo[0] = 10;
			pInfo->compInfo[1] = 5;
			pInfo->compInfo[2] = 5;
			pInfo->mcuWidth = 16;
			pInfo->mcuHeight = 16;
			break;
		default:
			return -1;
		}
	}

	return 0;
}

static void DecodeJpegDHT(struct vpu_dec_info *pInfo,
	struct vld_stream *pstStrm)
{
	int32_t i;
	int32_t iLength;
	int32_t iTC, iTH, iTcTh;
	int32_t iBitCnt;

	iLength = vld_get_bits(pstStrm, 16) - 2;
	do {
		/* table class */
		iTC = vld_get_bits(pstStrm, 4);
		/* table destination identifier */
		iTH = vld_get_bits(pstStrm, 4);
		iTcTh = ((iTH & 1) << 1) | (iTC & 1);

		/* Get Huff Bits list */
		iBitCnt = 0;
		for (i = 0 ; i < 16 ; i++) {
			pInfo->huffBits[iTcTh][i] = vld_get_bits(pstStrm, 8);
			iBitCnt += pInfo->huffBits[iTcTh][i];
			if (DefHuffmanBits[iTcTh][i] !=
				pInfo->huffBits[iTcTh][i])
				pInfo->userHuffTable = 1;
		}

		/* Get Huff Val list */
		for (i = 0 ; i < iBitCnt ; i++) {
			pInfo->huffValue[iTcTh][i] = vld_get_bits(pstStrm, 8);
			if (DefHuffmanValue[iTcTh][i] !=
				pInfo->huffValue[iTcTh][i])
				pInfo->userHuffTable = 1;
		}
	} while (iLength > (pstStrm->dwUsedBits >> 3));
}

static int32_t DecodeJpegDQT(struct vpu_dec_info *pInfo,
	struct vld_stream *pstStrm)
{
	int32_t i;
	int32_t iLength;
	int32_t iTQ;
	uint8_t *pbyQTable;

	iLength = vld_get_bits(pstStrm, 16) - 2;

	do {
		/* Pq */
		if (vld_get_bits(pstStrm, 4) >= 1)
			return -1;

		iTQ = vld_get_bits(pstStrm, 4);
		if (iTQ > 3)
			return -1;

		pbyQTable = pInfo->quantTable[iTQ];

		for (i = 0 ; i < 64 ; i++) {
			pbyQTable[i] = vld_get_bits(pstStrm, 8);
			if (pbyQTable[i] == 0)
				return -1;
		}
	} while (iLength > (pstStrm->dwUsedBits >> 3));

	return 0;
}

static void DecodeJpegSOS(struct vpu_dec_info *pInfo,
	struct vld_stream *pstStrm)
{
	int32_t i, j;
	int32_t iCompNum, iCompId;
	int32_t iDcHuffTabIdx[3];
	int32_t iAcHuffTabIdx[3];

	vld_flush_bits(pstStrm, 16);
	iCompNum = vld_get_bits(pstStrm, 8);
	for (i = 0 ; i < iCompNum ; i++) {
		iCompId = vld_get_bits(pstStrm, 8);
		iDcHuffTabIdx[i] = vld_get_bits(pstStrm, 4);
		iAcHuffTabIdx[i] = vld_get_bits(pstStrm, 4);

		for (j = 0 ; j < iCompNum ; j++) {
			if (iCompId == pInfo->infoTable[j][0]) {
				pInfo->infoTable[i][4] = iDcHuffTabIdx[i];
				pInfo->infoTable[i][5] = iAcHuffTabIdx[i];
			}
		}
	}
}

static void GenerateJpegDecHuffmanTable(struct vpu_dec_info *pInfo,
	int32_t iTabNum)
{
	int32_t iPtrCnt = 0;
	int32_t iHuffCode = 0;
	int32_t iZeroFlag = 0;
	int32_t iDataFlag = 0;
	int32_t i;
	uint8_t *pbyHuffBits = pInfo->huffBits[iTabNum];
	uint8_t *pbyHuffPtr = pInfo->huffPtr[iTabNum];
	uint32_t *uHuffMax = pInfo->huffMax[iTabNum];
	uint32_t *uHuffMin = pInfo->huffMin[iTabNum];

	for (i = 0; i < 16; i++) {
		/* if there is bit cnt value */
		if (pbyHuffBits[i]) {
			pbyHuffPtr[i] = iPtrCnt;
			iPtrCnt += pbyHuffBits[i];
			uHuffMin[i] = iHuffCode;
			uHuffMax[i] = iHuffCode + (pbyHuffBits[i] - 1);
			iDataFlag = 1;
			iZeroFlag = 0;
		} else {
			pbyHuffPtr[i] = 0xFF;
			uHuffMin[i] = 0xFFFF;
			uHuffMax[i] = 0xFFFF;
			iZeroFlag = 1;
		}

		if (iDataFlag == 1)
			iHuffCode = (iZeroFlag == 1) ? (iHuffCode << 1) :
				((uHuffMax[i] + 1) << 1);
	}
}

int JPU_DecSetSeqInfo(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_seq_init_arg *seqArg)
{
	struct vpu_dec_info *pInfo = &pInst->codecInfo.decInfo;

	pInfo->thumbnailMode = seqArg->thumbnailMode;

	if (JPU_DecParseHeader(pInfo, (uint8_t *)seqArg->seqData,
		seqArg->seqDataSize) < 0)
		return -1;

	seqArg->imgFormat = pInfo->imgFormat;
	seqArg->outWidth = pInfo->width;
	seqArg->outHeight = pInfo->height;
	seqArg->cropRight = seqArg->outWidth;
	seqArg->cropBottom = seqArg->outHeight;
	seqArg->minFrameBufCnt = 1;

	return 0;
}

int JPU_DecRegFrameBuf(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_reg_frame_arg *pArg)
{
	struct vpu_dec_info *pInfo = &pInst->codecInfo.decInfo;
	int32_t i;

	pInfo->cbcrInterleave = pArg->chromaInterleave;
	pInfo->numFrameBuffer = pArg->numFrameBuffer;

	for (i = 0 ; i < pInfo->numFrameBuffer ; i++) {
		NX_DrvMemcpy((void *)&pInfo->frameBuffer[i],
			(void *)&pArg->frameBuffer[i],
			sizeof(struct nx_vid_memory_info));
		pInfo->frmBufferValid[i] = 0;
	}

	return 0;
}

int JPU_DecParseHeader(struct vpu_dec_info *pInfo, uint8_t *pbyStream,
	int32_t iSize)
{
	uint32_t uPreFourByte = 0;
	uint8_t *pbyStrm = pbyStream;
	int32_t i, iStartCode;

	if ((pbyStrm == NULL) || (iSize <= 0))
		return -1;

	/* SOI(Start Of Image) check */
	if ((pbyStrm[0] != 0xFF) || (pbyStrm[1] != 0xD8))
		return -1;

	pbyStrm += 2;
	do {
		if (pbyStrm >= (pbyStream + iSize))
			return -1;

		uPreFourByte = (uPreFourByte << 8) + *pbyStrm++;
		iStartCode = uPreFourByte & 0xFFFF;

		if (iStartCode == 0xFFC0) {
			/* SOF(Start of Frame)- Baseline DCT */
			struct vld_stream stStrm = { 0, pbyStrm, iSize };

			if (DecodeJpegSOF(pInfo, &stStrm) < 0)
				return -1;
			pbyStrm += (stStrm.dwUsedBits >> 3);
		} else if (iStartCode == 0xFFC4) {
			/* DHT(Define Huffman Table) */
			struct vld_stream stStrm = { 0, pbyStrm, iSize };

			DecodeJpegDHT(pInfo, &stStrm);
			pbyStrm += (stStrm.dwUsedBits >> 3);
		} else if (iStartCode == 0xFFDA) {
			/* SOS(Start Of Scan) */
			struct vld_stream stStrm = { 0, pbyStrm, iSize };

			DecodeJpegSOS(pInfo, &stStrm);
			pbyStrm += (stStrm.dwUsedBits >> 3);
			break;
		} else if (iStartCode == 0xFFDB) {
			/* DQT(Define Quantization Table) */
			struct vld_stream stStrm = { 0, pbyStrm, iSize };

			if (DecodeJpegDQT(pInfo, &stStrm) < 0)
				return -1;
			pbyStrm += (stStrm.dwUsedBits >> 3);
		} else if (iStartCode == 0xFFDD) {
			/* DRI(Define Restart Interval) */
			pInfo->rstInterval = (pbyStrm[2] << 8) | (pbyStrm[3]);
			pbyStrm += 4;
		} else if (iStartCode == 0xFFD8) {
			/* SOI */
			if (pInfo->thumbnailMode == 0) {
				do {
					if (pbyStrm >= (pbyStream + iSize))
						return -1;
					uPreFourByte = (uPreFourByte << 8)
						+ *pbyStrm++;
					iStartCode = uPreFourByte & 0xFFFF;
				} while (iStartCode != 0xFFD9);
			}
		} else if (iStartCode == 0xFFD9) {
			/* EOI(End Of Image) */
			break;
		}
	} while (1);

	pInfo->headerSize = (uint32_t)((long)pbyStrm - (long)pbyStream + 3);

	{
		int ecsPtr = 0;

		pInfo->pagePtr = ecsPtr / 256;
		pInfo->wordPtr = (ecsPtr % 256) / 4;
		if (pInfo->pagePtr & 1)
			pInfo->wordPtr += 64;
		if (pInfo->wordPtr & 1)
			pInfo->wordPtr -= 1;

		pInfo->bitPtr = (ecsPtr % 4) * 8;
		if (((ecsPtr % 256) / 4) & 1)
			pInfo->bitPtr += 32;
	}

	/* Generate Huffman table information */
	for (i = 0; i < 4; i++)
		GenerateJpegDecHuffmanTable(pInfo, i);

	pInfo->qIdx = (pInfo->infoTable[0][3] << 2) |
		(pInfo->infoTable[1][3] << 1) | (pInfo->infoTable[2][3]);
	pInfo->huffDcIdx = (pInfo->infoTable[0][4] << 2) |
		(pInfo->infoTable[1][4] << 1) | (pInfo->infoTable[2][4]);
	pInfo->huffAcIdx = (pInfo->infoTable[0][5] << 2) |
		(pInfo->infoTable[1][5] << 1) | (pInfo->infoTable[2][5]);

	return 0;
}

int JPU_DecRunFrame(struct nx_vpu_codec_inst *pInst,
	struct vpu_dec_frame_arg *pArg)
{
	struct vpu_dec_info *pInfo = &pInst->codecInfo.decInfo;
	unsigned int val, reason = 0;
	int32_t i, idx;

	for (i = 0 ; i <= pInfo->numFrameBuffer ; i++) {
		idx = pInfo->decodeIdx + i;

		if (idx >= pInfo->numFrameBuffer)
			idx -= pInfo->numFrameBuffer;

		if (pInfo->frmBufferValid[idx] == 0) {
			pInfo->decodeIdx = idx;
			pArg->hCurrFrameBuffer = &pInfo->frameBuffer[idx];
			pArg->indexFrameDecoded = idx;
			pArg->indexFrameDisplay = idx;
			break;
		}
	}

	if (i > pInfo->numFrameBuffer) {
		NX_ErrMsg(("Frame Buffer for Decoding is not sufficient!!!\n"));
		return -1;
	}

	VPU_SWReset(SW_RESET_SAFETY);

	VpuWriteReg(MJPEG_BBC_RD_PTR_REG, pInfo->strmBufPhyAddr);
	VpuWriteReg(MJPEG_BBC_WR_PTR_REG, pInfo->writePos);

	VpuWriteReg(MJPEG_BBC_BAS_ADDR_REG, pInfo->readPos);
	VpuWriteReg(MJPEG_BBC_END_ADDR_REG, pInfo->writePos);

	VpuWriteReg(MJPEG_BBC_STRM_CTRL_REG, 0);

	VpuWriteReg(MJPEG_GBU_TT_CNT_REG, 0);
	VpuWriteReg(MJPEG_GBU_TT_CNT_REG+4, 0);

	VpuWriteReg(MJPEG_PIC_SIZE_REG, (pInfo->width << 16) | (pInfo->height));
	VpuWriteReg(MJPEG_PIC_CTRL_REG, (pInfo->huffAcIdx << 10) |
		(pInfo->huffDcIdx << 7) | (pInfo->userHuffTable << 6) |
		(1 << 2) | 0);

	VpuWriteReg(MJPEG_ROT_INFO_REG, 0);

	VpuWriteReg(MJPEG_MCU_INFO_REG, (pInfo->mcuBlockNum << 16) |
		(pInfo->compNum << 12) | (pInfo->compInfo[0] << 8) |
		(pInfo->compInfo[1] << 4) | (pInfo->compInfo[2]));
	VpuWriteReg(MJPEG_OP_INFO_REG, pInfo->busReqNum);

	if (pArg->downScaleWidth || pArg->downScaleHeight)
		VpuWriteReg(MJPEG_SCL_INFO_REG,  (1 << 4) |
			(pArg->downScaleWidth << 2) | (pArg->downScaleHeight));
	else
		VpuWriteReg(MJPEG_SCL_INFO_REG,  0);

	VpuWriteReg(MJPEG_DPB_CONFIG_REG, VPU_FRAME_BUFFER_ENDIAN << 1 |
		CBCR_INTERLEAVE);
	VpuWriteReg(MJPEG_RST_INTVAL_REG, pInfo->rstInterval);

	SetJpegDecHuffmanTable(pInfo);
	SetJpegDecQuantizationTable(pInfo);

	SetupJpegDecGram(pInfo);

	/* RST index at the beginning */
	VpuWriteReg(MJPEG_RST_INDEX_REG, 0);
	VpuWriteReg(MJPEG_RST_COUNT_REG, 0);

	VpuWriteReg(MJPEG_DPCM_DIFF_Y_REG, 0);
	VpuWriteReg(MJPEG_DPCM_DIFF_CB_REG, 0);
	VpuWriteReg(MJPEG_DPCM_DIFF_CR_REG, 0);

	VpuWriteReg(MJPEG_GBU_FF_RPTR_REG, pInfo->bitPtr);
	VpuWriteReg(MJPEG_GBU_CTRL_REG, 3);

	val = 0;	/* gdi status */
	VpuWriteReg(GDI_CONTROL, 1);
	while (!val)
		val = VpuReadReg(GDI_STATUS);

	VpuWriteReg(GDI_INFO_CONTROL, (0 << 20) | (pInfo->imgFormat << 17) |
		(CBCR_INTERLEAVE << 16) | (pArg->hCurrFrameBuffer->stride[0]));

	VpuWriteReg(GDI_INFO_PIC_SIZE, (pInfo->width << 16) | pInfo->height);

	VpuWriteReg(GDI_INFO_BASE_Y, pArg->hCurrFrameBuffer->phyAddr[0]);
	VpuWriteReg(GDI_INFO_BASE_CB,  pArg->hCurrFrameBuffer->phyAddr[1]);
	VpuWriteReg(GDI_INFO_BASE_CR,  pArg->hCurrFrameBuffer->phyAddr[2]);

	VpuWriteReg(MJPEG_DPB_BASE00_REG, 0);

	VpuWriteReg(GDI_CONTROL, 0);
	VpuWriteReg(GDI_PIC_INIT_HOST, 1);

	VpuWriteReg(MJPEG_PIC_START_REG, 1);

	reason = JPU_WaitInterrupt(pInst->devHandle, JPU_DEC_TIMEOUT);
	if (!reason) {
		NX_ErrMsg(("JPU_DecRunFrame() Failed. Timeout(%d)\n",
			JPU_DEC_TIMEOUT));
		return VPU_RET_ERR_TIMEOUT;
	}

	if (reason & (1 << INT_JPU_ERROR)) {
		NX_ErrMsg(("JPU Decode Error(reason = 0x%08x)\n", reason));
		return VPU_RET_ERROR;
	}
	if (reason & (1 << INT_JPU_BBC_INTERRUPT)) {
		NX_ErrMsg(("JPU BBC Interrupt Error(reason = 0x%08x)\n",
			reason));
		return VPU_RET_ERROR;
	}
	if (reason & (1 << INT_JPU_BIT_BUF_FULL)) {
		NX_ErrMsg(("JPU Overflow Error( reason = 0x%08x)\n", reason));
		return VPU_RET_ERR_STRM_FULL;
	}
	if (!(reason & (1 << INT_JPU_DONE)))
		return VPU_RET_ERR_RUN;

	pArg->outWidth = pInfo->width >> pArg->downScaleWidth;
	pArg->outHeight = pInfo->height >> pArg->downScaleHeight;
	pArg->mcuWidth = pInfo->mcuWidth;
	pArg->mcuHeight = pInfo->mcuHeight;
	pArg->outRect.left = 0;
	pArg->outRect.top = 0;
	pArg->outRect.right = pArg->outWidth;
	pArg->outRect.bottom = pArg->outHeight;
	pArg->numOfErrMBs = VpuReadReg(MJPEG_PIC_ERRMB_REG);

	NX_DrvMemcpy((void *)(&pArg->outFrameBuffer),
		(void *)(pArg->hCurrFrameBuffer),
		sizeof(struct nx_vid_memory_info));

	pInfo->readPos = VpuReadReg(pInfo->streamRdPtrRegAddr);
	pArg->strmReadPos = pInfo->readPos - pInfo->strmBufPhyAddr;
	pArg->strmWritePos = pInfo->writePos - pInfo->strmBufPhyAddr;

	pInfo->frmBufferValid[idx] = -1;

	VpuWriteReg(MJPEG_BBC_FLUSH_CMD_REG, 0);

	VPU_SWReset(SW_RESET_SAFETY);

	return VPU_RET_OK;
}
