/** @file wl_macros.h
 *
 *  @brief  Common macros are defined here. Must include "wltypes.h" before this file
 *
 *  (C) Copyright 2014-2016 Marvell International Ltd. All Rights Reserved
 *
 *  MARVELL CONFIDENTIAL
 *  The source code contained or described herein and all documents related to
 *  the source code ("Material") are owned by Marvell International Ltd or its
 *  suppliers or licensors. Title to the Material remains with Marvell International Ltd
 *  or its suppliers and licensors. The Material contains trade secrets and
 *  proprietary and confidential information of Marvell or its suppliers and
 *  licensors. The Material is protected by worldwide copyright and trade secret
 *  laws and treaty provisions. No part of the Material may be used, copied,
 *  reproduced, modified, published, uploaded, posted, transmitted, distributed,
 *  or disclosed in any way without Marvell's prior express written permission.
 *
 *  No license under any patent, copyright, trade secret or other intellectual
 *  property right is granted to or conferred upon you by disclosure or delivery
 *  of the Materials, either expressly, by implication, inducement, estoppel or
 *  otherwise. Any license under such intellectual property rights must be
 *  express and approved by Marvell in writing.
 *
 */

/******************************************************
Change log:
    03/07/2014: Initial version
******************************************************/
#if !defined(WL_MACROS_H__)
#define WL_MACROS_H__

#define MACRO_START          do {
#define MACRO_END            } while (0)

#define WL_REGS8(x)     (*(volatile unsigned char *)(x))
#define WL_REGS16(x)    (*(volatile unsigned short *)(x))
#define WL_REGS32(x)    (*(volatile unsigned int *)(x))

#define WL_READ_REGS8(reg,val)      ((val) = WL_REGS8(reg))
#define WL_READ_REGS16(reg,val)     ((val) = WL_REGS16(reg))
#define WL_READ_REGS32(reg,val)     ((val) = WL_REGS32(reg))
#define WL_READ_BYTE(reg,val)       ((val) = WL_REGS8(reg))
#define WL_READ_HWORD(reg,val)      ((val) = WL_REGS16(reg))	/* half word; */
/*16bits */
#define WL_READ_WORD(reg,val)       ((val) = WL_REGS32(reg))	/* 32 bits */
#define WL_WRITE_REGS8(reg,val)     (WL_REGS8(reg) = (val))
#define WL_WRITE_REGS16(reg,val)    (WL_REGS16(reg) = (val))
#define WL_WRITE_REGS32(reg,val)    (WL_REGS32(reg) = (val))
#define WL_WRITE_BYTE(reg,val)      (WL_REGS8(reg) = (val))
#define WL_WRITE_HWORD(reg,val)     (WL_REGS16(reg) = (val))	/* half word; */
/*16bits */
#define WL_WRITE_WORD(reg,val)      (WL_REGS32(reg) = (val))	/* 32 bits */
#define WL_REGS8_SETBITS(reg, val)  (WL_REGS8(reg) |= (UINT8)(val))
#define WL_REGS16_SETBITS(reg, val) (WL_REGS16(reg) |= (UINT16)(val))
#define WL_REGS32_SETBITS(reg, val) (WL_REGS32(reg) |= (val))

#define WL_REGS8_CLRBITS(reg, val)  (WL_REGS8(reg) = \
                                     (UINT8)(WL_REGS8(reg)&~(val)))

#define WL_REGS16_CLRBITS(reg, val) (WL_REGS16(reg) = \
                                     (UINT16)(WL_REGS16(reg)&~(val)))

#define WL_REGS32_CLRBITS(reg, val) (WL_REGS32(reg) = \
                                     (WL_REGS32(reg)&~(val)))

#define WL_WRITE_CHUNK(dst, src, length) (memcpy((void*) (dst), \
                                          (void*) (src), (length)))
/*!
 * Bitmask macros
 */
#define WL_BITMASK(nbits)           ((0x1 << nbits) - 1)

/*!
 * Macro to put the WLAN SoC into sleep mode
 */
#define WL_GO_TO_SLEEP   asm volatile ("MCR p15, 0, r3, c7, c0, 4;")

/*!
 * BE vs. LE macros
 */
#ifdef BE			/* Big Endian */
#define SHORT_SWAP(X) (X)
#define WORD_SWAP(X) (X)
#define LONG_SWAP(X) ((l64)(X))
#else /* Little Endian */

#define SHORT_SWAP(X) ((X <<8 ) | (X >> 8))	// !< swap bytes in a 16 bit
						// short

#define WORD_SWAP(X) (((X)&0xff)<<24)+      \
                    (((X)&0xff00)<<8)+      \
                    (((X)&0xff0000)>>8)+    \
                    (((X)&0xff000000)>>24)	// !< swap bytes in a 32 bit
						// word

#define LONG_SWAP(X) ( (l64) (((X)&0xffULL)<<56)+               \
                            (((X)&0xff00ULL)<<40)+              \
                            (((X)&0xff0000ULL)<<24)+            \
                            (((X)&0xff000000ULL)<<8)+           \
                            (((X)&0xff00000000ULL)>>8)+         \
                            (((X)&0xff0000000000ULL)>>24)+      \
                            (((X)&0xff000000000000ULL)>>40)+    \
                            (((X)&0xff00000000000000ULL)>>56))	// !< swap
								// bytes in a
								// 64 bit long
#endif

/*!
 * Alignment macros
 */
#define ALIGN4(x)       (((x) + 3) & ~3)
#define ALIGN4BYTE(x)   (x=ALIGN4(x))
#define ROUNDUP4US(x)   (ALIGN4(x))

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef NELEMENTS
#define NELEMENTS(x) (sizeof(x)/sizeof(x[0]))
#endif

#define HWORD_LOW_BYTE(x)   ((x) & 0xFF)
#define HWORD_HIGH_BYTE(x)  (((x) >> 8) & 0xFF)

#define htons(x) (UINT16)SHORT_SWAP(x)
#define htonl(x) (UINT32)WORD_SWAP(x)

#define ntohs(x) (UINT16)SHORT_SWAP(x)
#define ntohl(x) (UINT32)WORD_SWAP(x)

#define CEIL_aByb(a, b) ((a + b - 1) / b)

#endif /* _WL_MACROS_H_ */
