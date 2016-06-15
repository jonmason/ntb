/** @file sha_256.h
 *
 *  @brief This file contains the SHA256 hash implementation and interface functions
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
#ifndef SHA256_H
#define SHA256_H

#include "wltypes.h"

#define SHA256_MAC_LEN                 32
#define HMAC_SHA256_MIN_SCRATCH_BUF   (500)
#define SHA256_MIN_SCRATCH_BUF        (400)

struct sha256_state {
	UINT64 length;
	UINT32 state[8], curlen;
	UINT8 buf[64];
};

void sha256_init(struct sha256_state *md);

/**
 * sha256_compress - Compress 512-bits.
 * @md: Pointer to the element holding hash state.
 * @msgBuf: Pointer to the buffer containing the data to be hashed.
 * @pScratchMem: Scratch memory; At least 288 bytes of free memory *
 *
 */
int sha256_compress(void *priv, struct sha256_state *md,
		    UINT8 *msgBuf, UINT8 *pScratchMem);

/**
 * sha256_vector - SHA256 hash for data vector
 * @num_elem: Number of elements in the data vector
 * @addr: Pointers to the data areas
 * @len: Lengths of the data blocks
 * @mac: Buffer for the hash
 * @pScratchMem: Scratch memory; Buffer of SHA256_MIN_SCRATCH_BUF size
 */
void sha256_vector(void *priv, size_t num_elem,
		   UINT8 *addr[], size_t * len, UINT8 *mac, UINT8 *pScratchMem);

/**
 * hmac_sha256_vector - HMAC-SHA256 over data vector (RFC 2104)
 * @key: Key for HMAC operations
 * @key_len: Length of the key in bytes
 * @num_elem: Number of elements in the data vector; including [0]
 * @addr: Pointers to the data areas, [0] element must be left as spare
 * @len: Lengths of the data blocks, [0] element must be left as spare
 * @mac: Buffer for the hash (32 bytes)
 * @pScratchMem: Scratch Memory; Buffer of HMAC_SHA256_MIN_SCRATCH_BUF size
 */
void hmac_sha256_vector(void *priv, UINT8 *key,
			size_t key_len,
			size_t num_elem,
			UINT8 *addr[],
			size_t * len, UINT8 *mac, UINT8 *pScratchMem);

#endif /* SHA256_H */
