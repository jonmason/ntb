/** @file crypt_new.h
 *
 *  @brief This file contains define for rc4 decrypt/encrypt
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
/*
 * AES API header file
 *
 */

#ifndef _CRYPT_NEW_H_
#define _CRYPT_NEW_H_

#include "crypt_new_rom.h"

/* forward decl */
typedef void (*MRVL_ENDECRYPT_FUNC_p) (MRVL_ENDECRYPT_t *enDeCrypt, int *pErr);
#if 0
#if (HW_IP_AEU_VERSION < 100000)

#ifdef WAPI_HW_SUPPORT
extern void MRVL_WapiEncrypt(MRVL_ENDECRYPT_t *crypt, int *pErr);
extern void MRVL_WapiDecrypt(MRVL_ENDECRYPT_t *crypt, int *pErr);

#define MRVL_WAPI_ENCRYPT          MRVL_WapiEncrypt
#define MRVL_WAPI_DECRYPT          MRVL_WapiDecrypt
#endif

#ifdef DIAG_AES_CCM
#define MRVL_AES_CCM_ENCRYPT      MRVL_AesCCMEncrypt
#define MRVL_AES_CCM_DECRYPT      MRVL_AesCCMDecrypt
#endif

#endif /* (HW_IP_AEU_VERSION < 100000) */
#endif
#define MRVL_AES_PRIMITIVE_ENCRYPT MRVL_AesPrimitiveEncrypt
#define MRVL_AES_PRIMITIVE_DECRYPT MRVL_AesPrimitiveDecrypt
#define MRVL_AES_WRAP_ENCRYPT      MRVL_AesWrapEncrypt
#define MRVL_AES_WRAP_DECRYPT      MRVL_AesWrapDecrypt

#ifdef RC4

#define MRVL_RC4_ENCRYPT       MRVL_Rc4Cryption
#define MRVL_RC4_DECRYPT       MRVL_Rc4Cryption

#endif /* RC4 */

typedef struct {
	MRVL_ENDECRYPT_e action;
	MRVL_ENDECRYPT_FUNC_p pActionFunc;

} MRVL_ENDECRYPT_SETUP_t;

extern MRVL_ENDECRYPT_SETUP_t mrvlEnDecryptSetup[2][6];

extern void cryptNewRomInit(void);

#endif /* _CRYPT_NEW_H_ */
