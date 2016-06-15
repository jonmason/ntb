/** @file AssocAp_src_rom.h
 *
 *  @brief This file contains define check rsn/wpa ie
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
#ifndef ASSOCAP_SRV_ROM_H_
#define ASSOCAP_SRV_ROM_H_

#include "wltypes.h"
#include "IEEE_types.h"
#include "keyMgmtStaTypes.h"
#include "keyMgmtApTypes_rom.h"
#include "authenticator.h"

WL_STATUS assocSrvAp_checkRsnWpa(cm_Connection *connPtr,
				 apKeyMgmtInfoStaRom_t *pKeyMgmtInfo,
				 Cipher_t apWpaCipher,
				 Cipher_t apWpa2Cipher,
				 Cipher_t apMcstCipher,
				 UINT16 apAuthKey,
				 SecurityMode_t *pSecType,
				 IEEEtypes_RSNElement_t *pRsn,
				 IEEEtypes_WPAElement_t *pWpa,
				 BOOLEAN validate4WayHandshakeIE);

SINT32 assocSrvAp_CheckSecurity(cm_Connection *connPtr,
				IEEEtypes_WPSElement_t *pWps,
				IEEEtypes_RSNElement_t *pRsn,
				IEEEtypes_WPAElement_t *pWpa,
				IEEEtypes_WAPIElement_t *pWapi,
				IEEEtypes_StatusCode_t *pResult);
#endif
