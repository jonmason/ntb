/** @file keyMgmtSta.h
 *
 *  @brief This file contains the defines for key management.
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
#ifndef _KEY_MGMT_STA_H_
#define _KEY_MGMT_STA_H_

#include "keyCommonDef.h"
#include "KeyApiStaDefs.h"
#include "IEEE_types.h"
#include "keyMgmtStaTypes.h"
#include "keyMgmtSta_rom.h"

#ifdef BTAMP
#include "btamp_config.h"
#define BTAMP_SUPPLICATNT_SESSIONS  AMPHCI_MAX_PHYSICAL_LINK_SUPPORTED
#else
#define BTAMP_SUPPLICATNT_SESSIONS  0
#endif

//longl test
#define MAX_SUPPLICANT_SESSIONS     (10)

void keyMgmtControlledPortOpen(phostsa_private priv);

extern BOOLEAN supplicantAkmIsWpaWpa2(phostsa_private priv, AkmSuite_t *pAkm);
extern BOOLEAN supplicantAkmIsWpaWpa2Psk(phostsa_private priv,
					 AkmSuite_t *pAkm);
extern BOOLEAN supplicantAkmUsesKdf(phostsa_private priv, AkmSuite_t *pAkm);
extern BOOLEAN supplicantGetPmkid(phostsa_private priv,
				  IEEEtypes_MacAddr_t *pBssid,
				  IEEEtypes_MacAddr_t *pSta, AkmSuite_t *pAkm,
				  UINT8 *pPMKID);

extern void keyMgmtSetIGtk(phostsa_private priv,
			   keyMgmtInfoSta_t *pKeyMgmtInfoSta,
			   IGtkKde_t *pIGtkKde, UINT8 iGtkKdeLen);

extern UINT8 *keyMgmtGetIGtk(phostsa_private priv);

extern void keyMgmtSta_RomInit(void);

#if 0
extern BufferDesc_t *GetTxEAPOLBuffer(struct cm_ConnectionInfo *connPtr,
				      EAPOL_KeyMsg_Tx_t **ppTxEapol,
				      BufferDesc_t * pBufDesc);
#endif
extern void allocSupplicantData(void *priv);
extern void freeSupplicantData(void *priv);
extern void supplicantInit(void *priv, supplicantData_t *suppData);
extern BOOLEAN keyMgmtProcessMsgExt(phostsa_private priv,
				    keyMgmtInfoSta_t *pKeyMgmtInfoSta,
				    EAPOL_KeyMsg_t *pKeyMsg);

extern void ProcessKeyMgmtDataSta(phostsa_private priv, mlan_buffer *pmbuf,
				  IEEEtypes_MacAddr_t *sa,
				  IEEEtypes_MacAddr_t *da);

#endif
