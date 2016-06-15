/** @file keyMgntAP.h
 *
 *  @brief This file contains the eapol paket process and key management for authenticator
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
#ifndef _KEYMGMTAP_H_
#define _KEYMGMTAP_H_
//Authenticator related data structures, function prototypes

#include "wltypes.h"
#include "keyMgmtAp_rom.h"

#define STA_PS_EAPOL_HSK_TIMEOUT 3000	// ms
#define AP_RETRY_EAPOL_HSK_TIMEOUT 1000	// ms

extern void KeyMgmtInit(void *priv);
extern void ReInitGTK(void *priv);
int isValidReplayCount(t_void *priv, apKeyMgmtInfoSta_t *pKeyMgmtInfo,
		       UINT8 *pRxReplayCount);

extern void KeyMgmtGrpRekeyCountUpdate(t_void *context);
extern void KeyMgmtStartHskTimer(void *context);
extern void KeyMgmtStopHskTimer(void *context);
extern void KeyMgmtHskTimeout(t_void *context);
extern UINT32 keyApi_ApUpdateKeyMaterial(void *priv, cm_Connection *connPtr,
					 BOOLEAN updateGrpKey);

extern Status_e ProcessPWKMsg2(hostsa_private *priv,
			       cm_Connection *connPtr, t_u8 *pbuf, t_u32 len);
extern Status_e ProcessPWKMsg4(hostsa_private *priv,
			       cm_Connection *connPtr, t_u8 *pbuf, t_u32 len);
extern Status_e ProcessGrpMsg2(hostsa_private *priv,
			       cm_Connection *connPtr, t_u8 *pbuf, t_u32 len);
extern Status_e GenerateApEapolMsg(hostsa_private *priv,
				   t_void *pconnPtr, keyMgmtState_e msgState);
extern void ApMicErrTimerExpCb(t_void *context);
extern void ApMicCounterMeasureInvoke(t_void *pconnPtr);
extern t_u16 keyMgmtAp_FormatWPARSN_IE(void *priv,
				       IEEEtypes_InfoElementHdr_t *pIe,
				       UINT8 isRsn,
				       Cipher_t *pCipher,
				       UINT8 cipherCount,
				       Cipher_t *pMcastCipher,
				       UINT16 authKey, UINT16 authKeyCount);
#endif // _KEYMGMTAP_H_
