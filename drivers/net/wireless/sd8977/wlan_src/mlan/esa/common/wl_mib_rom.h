/** @file wl_mib_rom.h
 *
 *  @brieThis file contains the MIB structure definitions
 *          based on IEEE 802.11 specification.
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
#if !defined(WL_MIB_ROM_H__)

#define WL_MIB_ROM_H__
#include "IEEE_types.h"

#define MIB_EDCA_MSDU_LIFETIME_DEFAULT 512
#define WEP_KEY_USER_INPUT              13			/** Also defined in keyApiStaTypes.h */

/*-----------------------------------------*/

/* PHY Supported Transmit Data Rates Table */

/*-----------------------------------------*/

typedef struct MIB_PhySuppDataRatesTx_s {
	UINT8 SuppDataRatesTxIdx;	/* 1 to IEEEtypes_MAX_DATA_RATES_G */
	UINT8 SuppDataRatesTxVal;	/* 2 to 127 */
} MIB_PHY_SUPP_DATA_RATES_TX;

/*------------------------*/
/* WEP Default Keys Table */
/*------------------------*/
/* This struct is used in ROM and it should not be changed at all */
typedef struct MIB_WepDefaultKeys_s {
	UINT8 WepDefaultKeyIdx;	/* 1 to 4 */
	UINT8 WepDefaultKeyType;	/* */
	UINT8 WepDefaultKeyValue[WEP_KEY_USER_INPUT];	/* 5 byte string */
} MIB_WEP_DEFAULT_KEYS;

typedef struct {
	/* Maximum lifetime of an MSDU from when it enters the MAC, 802.11e */
	UINT16 MSDULifetime;	/* 0 to 500, 500 default */
} MIB_EDCA_CONFIG;

#endif /* _WL_MIB_ROM_H_ */
