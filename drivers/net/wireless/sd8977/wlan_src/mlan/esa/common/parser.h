/** @file parser.h
 *
 *  @brief This file contains definitions of 802.11 Management Frames
 *               and Information Element Parsing
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
#ifndef _PARSER_H_
#define _PARSER_H_

#include "IEEE_types.h"
#include "parser_rom.h"

typedef struct {
	IEEEtypes_HT_Capability_t *pHtCap;
	IEEEtypes_HT_Information_t *pHtInfo;
	IEEEtypes_20N40_BSS_Coexist_t *p2040Coexist;
	IEEEtypes_OBSS_ScanParam_t *pHtScanParam;
	IEEEtypes_ExtCapability_t *pHtExtCap;
} dot11nIEPointers_t;

#ifdef DOT11AC
typedef struct {
	IEEEtypes_VHT_Capability_t *pVhtCap;
	IEEEtypes_VHT_Operation_t *pVhtOper;
} dot11acIEPointers_t;
#endif

#ifdef TDLS
typedef struct {
	IEEEtypes_MobilityDomainElement_t *pMdie;
	IEEEtypes_FastBssTransElement_t *pFtie;
	IEEEtypes_RSNElement_t *pRsn;
	IEEEtypes_TimeoutIntervalElement_t *pTie[2];
	IEEEtypes_RICDataElement_t *pFirstRdie;

} Dot11rIePointers_t;
#endif
extern VendorSpecificIEType_e IsWMMElement(void *priv, UINT8 *pBuffer);
extern VendorSpecificIEType_e IsWPAElement(void *priv, UINT8 *pBuffer);

extern int ieBufValidate(UINT8 *pIe, int bufLen);

extern int GetIEPointers(void *priv, UINT8 *pIe,
			 int bufLen, IEPointers_t *pIePointers);

extern BOOLEAN parser_getAssocIEs(void *priv, UINT8 *pIe,
				  int bufLen, AssocIePointers_t *pIePointers);

extern
IEEEtypes_InfoElementHdr_t *parser_getSpecificIE(IEEEtypes_ElementId_e elemId,
						 UINT8 *pIe, int bufLen);

extern UINT8 parser_countNumInfoElements(UINT8 *pIe, int bufLen);

#endif // _PARSER_H_
