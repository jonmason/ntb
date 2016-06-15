/** @file KeyMgmtApTypes_rom.h
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
#ifndef _KEYMGMTAPTYPES_ROM_H_
#define _KEYMGMTAPTYPES_ROM_H_

#include "wltypes.h"
#include "keyMgmtStaTypes.h"

#define KDE_IE_SIZE  (2)	// type+length of KDE_t
#define KDE_SIZE     (KDE_IE_SIZE + 4 )	// OUI+datatype of KDE_t
#define GTK_IE_SIZE (2)
#define KEYDATA_SIZE (4 + GTK_IE_SIZE + TK_SIZE)	// OUI+datatype+
							// GTK_IE+ GTK

typedef enum {
	HSK_NOT_STARTED,
	MSG1_PENDING,
	WAITING_4_MSG2,
	MSG3_PENDING,
	WAITING_4_MSG4,
	GRPMSG1_PENDING,
	WAITING_4_GRPMSG2,
	GRP_REKEY_MSG1_PENDING,
	WAITING_4_GRP_REKEY_MSG2,
	/* the relative positions of the different enum elements ** should not
	   be changed since FW code checks for even/odd ** values at certain
	   places. */
	HSK_DUMMY_STATE,
	HSK_END
} keyMgmtState_e;

/* This sturcture is being accessed in rom code and should be kept intact. */
typedef struct {
	UINT16 staRsnCap;
	SecurityMode_t staSecType;
	Cipher_t staUcstCipher;
	UINT8 staAkmType;
	keyMgmtState_e keyMgmtState;
} apKeyMgmtInfoStaRom_t;

#endif
