/** @file rijndael.h
 *
 *  @brief This file contains the function optimised ANSI C code for the Rijndael cipher (now AES)
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

#ifndef __RIJNDAEL_H
#define __RIJNDAEL_H
#include "wltypes.h"

#define MAXKC   (256/32)
#define MAXKB   (256/8)
#define MAXNR   14
/*
typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
*/
/*  The structure for key information */
typedef struct {
	int decrypt;
	int Nr;			/* key-length-dependent number of rounds */
	UINT key[4 * (MAXNR + 1)];	/* encrypt or decrypt key schedule */
} rijndael_ctx;

void rijndael_set_key(rijndael_ctx *, UINT8 *, int, int);
void rijndael_decrypt(rijndael_ctx *, UINT8 *, UINT8 *);
void rijndael_encrypt(rijndael_ctx *, UINT8 *, UINT8 *);

#endif /* __RIJNDAEL_H */
