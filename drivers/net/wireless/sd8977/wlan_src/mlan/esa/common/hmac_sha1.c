/** @file hmac_sha1.c
 *
 *  @brief This file defines algorithm for hmac sha1
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
#include "wltypes.h"
#include "wl_macros.h"
#include "sha1.h"
#include "hostsa_ext_def.h"
#include "authenticator.h"

void
Mrvl_hmac_sha1(void *priv, unsigned char **ppText,
	       int *pTextLen,
	       int textNum,
	       unsigned char *key,
	       int key_len, unsigned char *output, int outputLen)
{
	/*
	   Note- Some of the variables are made static in this function
	   becuase currently this function executes in the idle task. The idle
	   task dosent have enough stack space to accomodate these varables.
	   In the future if this function in executed in a different task or
	   if we increase the stack size of the idle task then we can put
	   these variables on the stack */
	// BufferDesc_t * pDesc = NULL;
	phostsa_private psapriv = (phostsa_private)priv;
	hostsa_util_fns *util_fns = &psapriv->util_fns;
	UINT8 buf[400] = { 0 };
	Mrvl_SHA1_CTX *context;
	unsigned char *k_pad;	/* padding - key XORd with i/opad */
	unsigned char *digest;
	int i;

	k_pad = (unsigned char *)buf;
	digest = k_pad + 64;
	context = (Mrvl_SHA1_CTX *)(k_pad + 64 + 20);

	/* if key is longer than 64 bytes reset it to key=SHA1(key) */
	if (key_len > 64) {
		Mrvl_SHA1Init(context);
		Mrvl_SHA1Update(context, key, key_len);
		Mrvl_SHA1Final((void *)priv, context, key);

		key_len = 20;
	}

	/*
	 * the HMAC_SHA1 transform looks like:
	 *
	 * SHA1(K XOR opad, SHA1(K XOR ipad, text))
	 *
	 * where K is an n byte key
	 * ipad is the byte 0x36 repeated 64 times
	 * opad is the byte 0x5c repeated 64 times
	 * and text is the data being protected
	 */

	/* perform inner SHA1 */
	/* start out by storing key in pads */
	memset(util_fns, k_pad, 0, 64);
	memcpy(util_fns, k_pad, key, key_len);

	/* XOR key with ipad and opad values */
	for (i = 0; i < 8; i++) {
		((UINT64 *)k_pad)[i] ^= 0x3636363636363636ULL;
	}

	Mrvl_SHA1Init(context);	/* init context for 1st pass */
	Mrvl_SHA1Update(context, k_pad, 64);	/* start with inner pad */
	for (i = 0; i < textNum; i++) {
		/* then text of datagram */
		Mrvl_SHA1Update(context, ppText[i], pTextLen[i]);
	}

	Mrvl_SHA1Final((void *)priv, context, digest);	/* finish up 1st pass */

	/* perform outer SHA1 */
	/* start out by storing key in pads */
	memset(util_fns, k_pad, 0, 64);
	memcpy(util_fns, k_pad, key, key_len);

	/* XOR key with ipad and opad values */
	for (i = 0; i < 8; i++) {
		((UINT64 *)k_pad)[i] ^= 0x5c5c5c5c5c5c5c5cULL;
	}

	Mrvl_SHA1Init(context);	/* init context for 2nd pass */
	Mrvl_SHA1Update(context, k_pad, 64);	/* start with outer pad */
	Mrvl_SHA1Update(context, digest, 20);	/* then results of 1st hash */
	Mrvl_SHA1Final((void *)priv, context, digest);	/* finish up 2nd pass */
	memcpy(util_fns, output, digest, outputLen);

//    bml_FreeBuffer((UINT32)pDesc);
}

/*
 * PRF -- Length of output is in octets rather than bits
 *     since length is always a multiple of 8 output array is
 *     organized so first N octets starting from 0 contains PRF output
 *
 *     supported inputs are 16, 32, 48, 64
 *     output array must be 80 octets to allow for sha1 overflow
 */
void
Mrvl_PRF(void *priv, unsigned char *key, int key_len,
	 unsigned char *prefix, int prefix_len,
	 unsigned char *data, int data_len, unsigned char *output, int len)
{
	phostsa_private psapriv = (phostsa_private)priv;
	hostsa_util_fns *util_fns = &psapriv->util_fns;
	int i;
	int currentindex = 0;
	int total_len;
	UINT8 prf_input[120];	/* concatenated input */
	unsigned char *pText = prf_input;
	SINT8 remain = len;

	memset(util_fns, prf_input, 0x00, sizeof(prf_input));

	if (prefix) {
		memcpy(util_fns, prf_input, prefix, prefix_len);
		prf_input[prefix_len] = 0;	/* single octet 0 */
		memcpy(util_fns, &prf_input[prefix_len + 1], data, data_len);
		total_len = prefix_len + 1 + data_len;
	} else {
		memcpy(util_fns, prf_input, data, data_len);
		total_len = data_len;
	}

	prf_input[total_len] = 0;	/* single octet count, starts at 0 */
	total_len++;
	for (i = 0; i < (len + 19) / 20; i++) {
		Mrvl_hmac_sha1(priv, (UINT8 **)&pText,
			       &total_len,
			       1,
			       key,
			       key_len, &output[currentindex], MIN(20, remain));
		currentindex += MIN(20, remain);	/* next concatenation
							   location */
		remain -= 20;
		prf_input[total_len - 1]++;	/* increment octet count */
	}
}
