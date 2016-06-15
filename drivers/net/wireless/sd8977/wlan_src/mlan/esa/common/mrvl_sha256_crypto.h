/** @file Mrvl_sha256_crypto.h
 *
 *  @brief This file contains the define for sha256
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

#ifndef _MRVL_SHA256_CRYPTO_H
#define _MRVL_SHA256_CRYPTO_H

extern void mrvl_sha256_crypto_vector(void *priv, size_t num_elem,
				      UINT8 *addr[], size_t * len, UINT8 *mac);

extern void mrvl_sha256_crypto_kdf(void *priv, UINT8 *pKey,
				   UINT8 key_len,
				   char *label,
				   UINT8 label_len,
				   UINT8 *pContext,
				   UINT16 context_len,
				   UINT8 *pOutput, UINT16 output_len);

#endif
