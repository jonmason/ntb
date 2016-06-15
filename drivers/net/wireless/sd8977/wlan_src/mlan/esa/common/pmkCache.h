/** @file pmkcache.h
 *
 *  @brief This file contains define for pmk cache
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
#ifndef PMK_CACHE_H__
#define PMK_CACHE_H__

#include "wltypes.h"
#include "IEEE_types.h"
#include "pmkCache_rom.h"

/*!
** \brief      If a matching SSID entry is present in the PMK Cache, returns a
**             pointer to the PSK.  If no entry is found in the cache, a
**             new PSK entry will be generated if a PassPhrase is set.
** \param      pSsid pointer to string containing desired SSID.
** \param      ssidLen length of the SSID string *pSsid.
** \exception  Does not handle the case when multiple matching SSID entries are
**             found. Returns the first match.
** \return     pointer to PSK with matching SSID entry from PMK cache,
**             NULL if no matching entry found.
*/
extern UINT8 *pmkCacheFindPSK(void *priv, UINT8 *pSsid, UINT8 ssidLen);

/*!
** \brief      Flushes all entries in PMK cache
*/
extern void pmkCacheFlush(void *priv);

extern void pmkCacheGetPassphrase(void *priv, char *pPassphrase);

extern void pmkCacheSetPassphrase(void *priv, char *pPassphrase);
extern void pmkCacheInit(void *priv);
extern void pmkCacheRomInit(void);

extern void supplicantDisable(void *priv);

#endif
