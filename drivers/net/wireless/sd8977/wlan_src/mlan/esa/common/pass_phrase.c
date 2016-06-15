/** @file pass_phrase.c
 *
 *  @brief This file defines passphase hash
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
#include "wl_macros.h"
#include "pass_phrase.h"
//#include "keyMgmtSta.h"
#include "sha1.h"
#include "hostsa_ext_def.h"
#include "authenticator.h"

static INLINE t_u32
pass_strlen(const char *str)
{
	t_u32 i;

	for (i = 0; str[i] != 0; i++) {
	}
	return i;
}

/*
 * F(P, S, c, i) = U1 xor U2 xor ... Uc
 * U1 = PRF(P, S || Int(i))
 * U2 = PRF(P, U1)
 * Uc = PRF(P, Uc-1)
 */
void
Mrvl_F(void *priv, char *password, unsigned char *ssid, int ssidlength,
       int iterations, int count, unsigned char *output)
{
	phostsa_private psapriv = (phostsa_private)priv;
	hostsa_util_fns *util_fns = &psapriv->util_fns;
	static unsigned char digest[36], digest1[A_SHA_DIGEST_LEN];
	int i, j;
	int len = pass_strlen(password);
	int tmpLen = ssidlength + 4;
	unsigned char *pTemp = digest;

	/* U1 = PRF(P, S || int(i)) */
	memcpy(util_fns, digest, ssid, ssidlength);
	digest[ssidlength] = (unsigned char)((count >> 24) & 0xff);
	digest[ssidlength + 1] = (unsigned char)((count >> 16) & 0xff);
	digest[ssidlength + 2] = (unsigned char)((count >> 8) & 0xff);
	digest[ssidlength + 3] = (unsigned char)(count & 0xff);

	Mrvl_hmac_sha1((void *)priv, &pTemp,
		       &tmpLen,
		       1,
		       (unsigned char *)password,
		       len, digest1, A_SHA_DIGEST_LEN);

	/* output = U1 */
	memcpy(util_fns, output, digest1, A_SHA_DIGEST_LEN);
	pTemp = digest1;
	for (i = 1; i < iterations; i++) {
		tmpLen = A_SHA_DIGEST_LEN;

		/* Un = PRF(P, Un-1) */
		Mrvl_hmac_sha1((void *)priv, &pTemp,
			       &tmpLen,
			       1,
			       (unsigned char *)password,
			       len, digest, A_SHA_DIGEST_LEN);

		memcpy(util_fns, digest1, digest, A_SHA_DIGEST_LEN);

		/* output = output xor Un */
		for (j = 0; j < A_SHA_DIGEST_LEN; j++) {
			output[j] ^= digest[j];
		}
	}
}

/*
 * password - ascii string up to 63 characters in length
 * ssid - octet string up to 32 octets
 * ssidlength - length of ssid in octets
 * output must be 32 octets in length and outputs 256 bits of key
 */
int
Mrvl_PasswordHash(void *priv, char *password, unsigned char *ssid,
		  int ssidlength, unsigned char *output)
{
	phostsa_private psapriv = (phostsa_private)priv;
	hostsa_util_fns *util_fns = &psapriv->util_fns;
	if ((pass_strlen(password) > 63) || (ssidlength > 32)) {
		return 0;
	}

	Mrvl_F((void *)priv, password, ssid, ssidlength, 4096, 2, output);
	memcpy(util_fns, output + A_SHA_DIGEST_LEN, output, 12);
	Mrvl_F((void *)priv, password, ssid, ssidlength, 4096, 1, output);
	return 1;
}
