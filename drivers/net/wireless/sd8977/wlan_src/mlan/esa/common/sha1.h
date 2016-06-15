/** @file sha1.h
 *
 *  @brief This file contains the sha1 functions
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
#ifndef _SHA1_H_
#define _SHA1_H_

#include "wltypes.h"

enum {
	shaSuccess = 0,
	shaNull,		/* Null pointer parameter */
	shaInputTooLong,	/* input data too long */
	shaStateError		/* called Input after Result */
};

#define A_SHA_DIGEST_LEN 20

/*
 *  This structure will hold context information for the SHA-1
 *  hashing operation
 */
typedef struct {
	UINT32 Intermediate_Hash[A_SHA_DIGEST_LEN / 4];	/* Message Digest */

	UINT32 Length_Low;	/* Message length in bits */
	UINT32 Length_High;	/* Message length in bits */

	UINT32 Scratch[16];	/* This is used to reduce the memory **
				   requirements of the transform **function */
	UINT8 Message_Block[64];	/* 512-bit message blocks */
	/* Index into message block array */
	SINT16 Message_Block_Index;
	UINT8 Computed;		/* Is the digest computed? */
	UINT8 Corrupted;	/* Is the message digest corrupted? */
} Mrvl_SHA1_CTX;

/*
 *  Function Prototypes
 */

extern int Mrvl_SHA1Init(Mrvl_SHA1_CTX *);
extern int Mrvl_SHA1Update(Mrvl_SHA1_CTX *, const UINT8 *, unsigned int);
extern int Mrvl_SHA1Final(void *priv, Mrvl_SHA1_CTX *,
			  UINT8 Message_Digest[A_SHA_DIGEST_LEN]);

extern void Mrvl_PRF(void *priv, unsigned char *key,
		     int key_len,
		     unsigned char *prefix,
		     int prefix_len,
		     unsigned char *data,
		     int data_len, unsigned char *output, int len);

extern void Mrvl_hmac_sha1(void *priv, unsigned char **ppText,
			   int *pTextLen,
			   int textNum,
			   unsigned char *key,
			   int key_len, unsigned char *output, int outputLen);

#endif
