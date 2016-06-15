/** @file supplicant.c
 *
 *  @brief This file defines the API for supplicant
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
#include "keyMgmtSta.h"
#include "keyCommonDef.h"
#include "keyMgmtSta.h"
#include "pmkCache.h"

static __inline void
ProcessEAPoLKeyPkt(phostsa_private priv, mlan_buffer *pmbuf,
		   IEEEtypes_MacAddr_t *sa, IEEEtypes_MacAddr_t *da)
{
	hostsa_mlan_fns *pm_fns = &priv->mlan_fns;
	t_u8 bss_role = pm_fns->Hostsa_get_bss_role(pm_fns->pmlan_private);

	PRINTM(MMSG, "ProcessEAPoLKeyPk bss_type=%x bss_role=%x\n",
	       pm_fns->bss_type, bss_role);

#ifdef MIB_STATS
	INC_MIB_STAT(connPtr, eapolRxForESUPPCnt);
#endif

	switch (bss_role) {
#ifdef BTAMP
	case CONNECTION_TYPE_BTAMP:
		ProcessKeyMgmtDataAmp(bufDesc);
		break;
#endif

	case MLAN_BSS_ROLE_STA:
		/* key data */
		ProcessKeyMgmtDataSta(priv, pmbuf, sa, da);
		break;

	default:
#ifdef AUTHENTICATOR
		if (WIFI_DIRECT_MODE_GO == connPtr->DeviceMode) {
			ProcessKeyMgmtDataAp(bufDesc);
		}
#endif

		break;
	}
}

t_u8
ProcessEAPoLPkt(void *priv, mlan_buffer *pmbuf)
{
	phostsa_private psapriv = (phostsa_private)priv;
	ether_hdr_t *pEthHdr =
		(ether_hdr_t *)(pmbuf->pbuf + pmbuf->data_offset);
	EAP_PacketMsg_t *pEapPkt = NULL;
	UINT8 fPacketProcessed = 0;

	pEapPkt = (EAP_PacketMsg_t *)((t_u8 *)pEthHdr + sizeof(ether_hdr_t));
	switch (pEapPkt->hdr_8021x.pckt_type) {
	case IEEE_8021X_PACKET_TYPE_EAPOL_KEY:
		ProcessEAPoLKeyPkt(psapriv, pmbuf,
				   (IEEEtypes_MacAddr_t *)pEthHdr->sa,
				   (IEEEtypes_MacAddr_t *)pEthHdr->da);
		fPacketProcessed = 1;
		break;

#if 0
	case IEEE_8021X_PACKET_TYPE_EAP_PACKET:
		{
			if (WIFI_DIRECT_MODE_CLIENT == connPtr->DeviceMode
			    || WIFI_DIRECT_MODE_DEVICE == connPtr->DeviceMode) {
				if (pEapPkt->code ==
				    IEEE_8021X_CODE_TYPE_REQUEST) {
					assocAgent_eapRequestRx(sa);
				}
			}
		}
		break;
#endif
	default:
		break;
	}
//    CLEAN_FLUSH_CACHED_SQMEM((UINT32)(pEapPkt), bufDesc->DataLen);
	return fPacketProcessed;
}

Status_e
supplicantRestoreDefaults(void *priv)
{
	pmkCacheInit(priv);
	return SUCCESS;
}

/* This can also be removed*/
//#pragma arm section code = ".init"
void
supplicantFuncInit(void *priv)
{
	supplicantRestoreDefaults(priv);
}

//#pragma arm section code
//#endif /* PSK_SUPPLICANT */
