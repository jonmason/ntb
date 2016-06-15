/** @file Authenticator_api.h
 *
 *  @brief This file defines the main APIs for authenticator.
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
#ifndef _AUTHENTICATORAPI_H
#define _AUTHENTICATORAPI_H

/******************************************************
Change log:
    03/01/2014: Initial version
******************************************************/
#ifdef DRV_EMBEDDED_AUTHENTICATOR
extern t_u8 IsAuthenticatorEnabled(void *priv);
extern void AuthenitcatorInitBssConfig(void *priv);
extern void AuthenticatorKeyMgmtInit(void *priv, t_u8 *addr);
extern void AuthenticatorkeyClear(void *priv);
extern void authenticator_init_client(void *priv, void **ppconnPtr, t_u8 *mac);
extern void authenticator_free_client(void *priv, void *ppconnPtr);
extern void authenticator_get_sta_security_info(void *priv,
						t_void *pconnPtr, t_u8 *pIe,
						t_u8 ieLen);
extern t_void AuthenticatorSendEapolPacket(t_void *priv, t_void *pconnPtr);
extern t_u8 AuthenticatorProcessEapolPacket(void *priv, t_u8 *pbuf, t_u32 len);
extern t_u8 AuthenticatorBssConfig(void *priv, t_u8 *pbss_config, t_u8 appendIE,
				   t_u8 clearIE, t_u8 SetConfigToMlan);
#endif
mlan_status supplicant_authenticator_init(t_void **pphostsa_priv,
					  t_void *psa_util_fns,
					  t_void *psa_mlan_fns, t_u8 *addr);
mlan_status supplicant_authenticator_free(t_void *phostsa_priv);
#ifdef DRV_EMBEDDED_SUPPLICANT
extern void supplicantClrEncryptKey(void *priv);
extern void *processRsnWpaInfo(void *priv, void *prsnwpa_ie);
extern void pmkCacheDeletePMK(void *priv, t_u8 *pBssid);
extern void supplicantInitSession(void *priv,
				  t_u8 *pSsid,
				  t_u16 len, t_u8 *pBssid, t_u8 *pStaAddr);
extern void supplicantStopSessionTimer(void *priv);
extern t_u8 supplicantIsEnabled(void *priv);
extern void supplicantQueryPassphraseAndEnable(void *priv, t_u8 *pbuf);
extern void supplicantDisable(void *priv);
extern t_u8 ProcessEAPoLPkt(void *priv, mlan_buffer *pmbuf);
extern void SupplicantClearPMK(void *priv, void *pPassphrase);
extern t_u16 SupplicantSetPassphrase(void *priv, void *pPassphraseBuf);
extern void SupplicantQueryPassphrase(void *priv, void *pPassphraseBuf);
extern t_u8 supplicantFormatRsnWpaTlv(void *priv, void *rsn_wpa_ie,
				      void *rsn_ie_tlv);
#endif
#endif	/**_AUTHENTICATORAPI_H*/
