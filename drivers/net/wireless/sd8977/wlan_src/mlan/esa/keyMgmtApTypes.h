/** @file KeyMgmtApTypes.h
 *
 *  @brief This file contains the key management type for ap
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
#ifndef _KEYMGMTAPTYPES_H_
#define _KEYMGMTAPTYPES_H_

#include "wltypes.h"
#include "IEEE_types.h"
#include "keyMgmtStaTypes.h"
#include "keyMgmtApTypes_rom.h"
#include "keyCommonDef.h"

typedef enum {
	STA_ASSO_EVT,
	MSGRECVD_EVT,
	KEYMGMTTIMEOUT_EVT,
	GRPKEYTIMEOUT_EVT,
	UPDATEKEYS_EVT
} keyMgmt_HskEvent_e;

/* Fields till keyMgmtState are being accessed in rom code and
  * should be kept intact. Fields after keyMgmtState can be changed
  * safely.
  */
typedef struct {
	apKeyMgmtInfoStaRom_t rom;
	UINT8 numHskTries;
	UINT32 counterLo;
	UINT32 counterHi;
	UINT8 EAPOL_MIC_Key[EAPOL_MIC_KEY_SIZE];
	UINT8 EAPOL_Encr_Key[EAPOL_ENCR_KEY_SIZE];
	UINT8 EAPOLProtoVersion;
	UINT8 rsvd[3];
} apKeyMgmtInfoSta_t;

/*  Convert an Ascii character to a hex nibble
    e.g. Input is 'b' : Output will be 0xb
         Input is 'E' : Output will be 0xE
         Input is '8' : Output will be 0x8
    Assumption is that input is a-f or A-F or 0-9
*/
#define ASCII2HEX(Asc) (((Asc) >= 'a') ? (Asc - 'a' + 0xA)\
    : ( (Asc) >= 'A' ? ( (Asc) - 'A' + 0xA ) : ((Asc) - '0') ))

#define ETH_P_EAPOL 0x8E88

#endif // _KEYMGMTAPTYPES_H_
