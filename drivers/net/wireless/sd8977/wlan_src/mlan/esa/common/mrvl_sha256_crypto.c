/** @file Mrvl_sha256_crypto.c
 *
 *  @brief This file  defines sha256 crypto
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
#include <linux/string.h>

#include "wltypes.h"
#include "wl_macros.h"

#include "sha_256.h"
#include "mrvl_sha256_crypto.h"

#include "hostsa_ext_def.h"
#include "authenticator.h"

/* Helper function to allocate scratch memory and call sha256_vector() */
void
mrvl_sha256_crypto_vector(void *priv, size_t num_elem,
			  UINT8 *addr[], size_t * len, UINT8 *mac)
{

	// BufferDesc_t* pBufDesc = NULL;
	UINT8 buf[SHA256_MIN_SCRATCH_BUF] = { 0 };

	sha256_vector((void *)priv, num_elem, addr, len, mac, (UINT8 *)buf);

//    bml_FreeBuffer((UINT32)pBufDesc);
}

void
mrvl_sha256_crypto_kdf(void *priv, UINT8 *pKey,
		       UINT8 key_len,
		       char *label,
		       UINT8 label_len,
		       UINT8 *pContext,
		       UINT16 context_len, UINT8 *pOutput, UINT16 output_len)
{
	phostsa_private psapriv = (phostsa_private)priv;
	hostsa_util_fns *util_fns = &psapriv->util_fns;
	UINT8 *vectors[4 + 1];
	size_t vectLen[NELEMENTS(vectors)];
	UINT8 *pResult;
	UINT8 *pLoopResult;
	UINT8 iterations;
	UINT16 i;
	// BufferDesc_t* pBufDesc = NULL;
	UINT8 buf[HMAC_SHA256_MIN_SCRATCH_BUF] = { 0 };

	pResult = pContext + context_len;

	/*
	 ** Working memory layout:
	 **            | KDF-Len output --- >
	 **            |
	 ** [KDF Input][HMAC output#1][HMAC output#2][...]
	 **
	 */

	/* Number of SHA256 digests needed to meet the bit length output_len */
	iterations = (output_len + 255) / 256;

	pLoopResult = pResult;

	for (i = 1; i <= iterations; i++) {
		/* Skip vectors[0]; Used internally in hmac_sha256_vector
		   function */
		vectors[1] = (UINT8 *)&i;
		vectLen[1] = sizeof(i);

		vectors[2] = (UINT8 *)label;
		vectLen[2] = label_len;

		vectors[3] = pContext;
		vectLen[3] = context_len;

		vectors[4] = (UINT8 *)&output_len;
		vectLen[4] = sizeof(output_len);

		/*
		 **
		 **  KDF input = (K, i || label || Context || Length)
		 **
		 */
		hmac_sha256_vector(priv, pKey, key_len,
				   NELEMENTS(vectors), vectors, vectLen,
				   pLoopResult, (UINT8 *)buf);

		/* Move the hmac output pointer so another digest can be
		   appended ** if more loop iterations are needed to get the
		   output_len key ** bit total */
		pLoopResult += SHA256_MAC_LEN;
	}

//    bml_FreeBuffer((UINT32)pBufDesc);

	memcpy(util_fns, pOutput, pResult, output_len / 8);

}
