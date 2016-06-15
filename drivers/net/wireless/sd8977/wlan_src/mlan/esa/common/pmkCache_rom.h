/** @file pmkcache_rom.h
 *
 *  @brief This file contains the defien for pmk cache
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
#ifndef PMK_CACHE_ROM_H__
#define PMK_CACHE_ROM_H__

#include "wltypes.h"
#include "IEEE_types.h"
#include "hostsa_ext_def.h"

#define PSK_PASS_PHRASE_LEN_MAX 63
#define PMK_LEN_MAX  32
#define MAX_PMK_SIZE 32

typedef struct {
	union {
		IEEEtypes_MacAddr_t Bssid;
		char Ssid[32];
	} key;
	UINT8 PMK[MAX_PMK_SIZE];	/* PMK / PSK */
	UINT8 length;
	UINT8 passphrase[PSK_PASS_PHRASE_LEN_MAX];
	SINT8 replacementRank;
} pmkElement_t;

/*!
** \brief      Finds a PMK matching a given BSSID
** \param      pBssid pointer to the desired BSSID
** \return     pointer to the matching PMK.
**             NULL, if no matching PMK entry is found
*/
extern UINT8 *pmkCacheFindPMK(void *priv, IEEEtypes_MacAddr_t *pBssid);
#if 0
extern BOOLEAN (*pmkCacheFindPSKElement_hook) (UINT8 *pSsid,
					       UINT8 ssidLen,
					       pmkElement_t **ptr_val);
#endif
extern pmkElement_t *pmkCacheFindPSKElement(void *priv, UINT8 *pSsid,
					    UINT8 ssidLen);

/*!
** \brief      adds a new PMK entry to PMK cache.
** \param      pBssid pointer to Bssid for which to add the PMK
** \param      pPMK pointer to PMK data
*/
//extern BOOLEAN (*pmkCacheAddPMK_hook)(IEEEtypes_MacAddr_t * pBssid,
//                                      UINT8 *pPMK);
extern void pmkCacheAddPMK(void *priv, IEEEtypes_MacAddr_t *pBssid,
			   UINT8 *pPMK);

/*!
** \brief      Adds a new PSK to PMK cache.
** \param      pSsid pointer to desired SSID for which to add the PSK entry.
** \param      ssidLen length of the SSID string.
** \param      pPSK pointer to PSK to store.
*/
#if 0
extern BOOLEAN (*pmkCacheAddPSK_hook) (UINT8 *pSsid,
				       UINT8 ssidLen, UINT8 *pPSK);
#endif
extern void pmkCacheAddPSK(void *priv, UINT8 *pSsid,
			   UINT8 ssidLen, UINT8 *pPSK, UINT8 *pPassphrase);

/*!
** \brief      Delete a particular PMK entry from PMK cache.
** \param      pBssid pointer to BSSID that needs to be deleted
*/
extern void pmkCacheDeletePMK(void *priv, t_u8 *pBssid);

/*!
** \brief      delete a particular PSK entry from PMK cache
** \param      Ssid pointer to SSID that needs to be deleted
** \param      ssidLen length of the string pointed to by Ssid
*/
extern void pmkCacheDeletePSK(void *priv, UINT8 *ssid, UINT8 ssidLen);
#if 0
extern BOOLEAN (*pmkCacheGeneratePSK_hook) (UINT8 *pSsid,
					    UINT8 ssidLen,
					    char *pPassphrase, UINT8 *pPSK);
#endif
extern void pmkCacheGeneratePSK(void *priv, UINT8 *pSsid,
				UINT8 ssidLen, char *pPassphrase, UINT8 *pPSK);

//extern BOOLEAN (*pmkCacheNewElement_hook)(pmkElement_t ** ptr_val);
extern pmkElement_t *pmkCacheNewElement(void *priv);

//extern BOOLEAN (*pmkCacheFindPMKElement_hook)(IEEEtypes_MacAddr_t * pBssid,
//                                              pmkElement_t ** ptr_val);
extern pmkElement_t *pmkCacheFindPMKElement(void *priv,
					    IEEEtypes_MacAddr_t *pBssid);

extern void pmkCacheUpdateReplacementRank(pmkElement_t *pPMKElement);

extern SINT8 replacementRankMax;

/* ROM linkages */
extern SINT32 ramHook_MAX_PMK_CACHE_ENTRIES;
extern pmkElement_t *ramHook_pmkCache;
extern char *ramHook_PSKPassPhrase;

//extern void (*ramHook_hal_SetCpuMaxSpeed)(void);
//extern void (*ramHook_hal_RestoreCpuSpeed)(void);

#endif
