/** @file aes_cmac_rom.h
 *
 *  @brief This file contains the define for aes_cmac_rom
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
#ifndef _AES_CMAC_ROM_H_
#define _AES_CMAC_ROM_H_

#include "wltypes.h"

extern BOOLEAN (*mrvl_aes_cmac_hook) (UINT8 *key, UINT8 *input, int length,
				      UINT8 *mac);
extern void mrvl_aes_cmac(phostsa_private priv, UINT8 *key, UINT8 *input,
			  int length, UINT8 *mac);

extern BOOLEAN (*mrvl_aes_128_hook) (UINT8 *key, UINT8 *input, UINT8 *output);
extern void mrvl_aes_128(UINT8 *key, UINT8 *input, UINT8 *output);

/* RAM Linkages */
extern void (*rom_hal_EnableWEU) (void);
extern void (*rom_hal_DisableWEU) (void);

extern void xor_128(UINT8 *a, UINT8 *b, UINT8 *out);
extern void leftshift_onebit(UINT8 *input, UINT8 *output);
extern void padding(UINT8 *lastb, UINT8 *pad, int length);

#endif
