/** @file md5.h
 *
 *  @brief This file contains define for md5.
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
#ifndef _MD5_H_
#define _MD5_H_

typedef struct {
	unsigned int state[4];	/* state (ABCD) */
	unsigned int count[2];	/* number of bits, modulo 2^64 (lsb first) */
	unsigned int scratch[16];	/* This is used to reduce the memory *
					   requirements of the transform *
					   function */
	unsigned char buffer[64];	/* input buffer */
} Mrvl_MD5_CTX;

void wpa_MD5Init(Mrvl_MD5_CTX *context);
void wpa_MD5Update(void *priv, Mrvl_MD5_CTX *context, UINT8 *input,
		   UINT32 inputLen);
void wpa_MD5Final(void *priv, unsigned char digest[16], Mrvl_MD5_CTX *context);
void Mrvl_hmac_md5(void *priv, UINT8 *text, int text_len, UINT8 *key,
		   int key_len, void *digest);

#endif
