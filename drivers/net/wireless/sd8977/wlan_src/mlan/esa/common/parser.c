/** @file parser.c
 *
 *  @brief This file defines function for 802.11 Management Frames Parsing
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
/*****************************************************************************
*
* File: parser.c
*
*
*
* Author(s):    Rajesh Bhagwat
* Date:         2005-02-04
* Description:  802.11 Management Frames Parsing
*
******************************************************************************/
#include "wltypes.h"
#include "wl_mib.h"
#include "IEEE_types.h"
#include "parser.h"
#include "parser_rom.h"

#include "hostsa_ext_def.h"
#include "authenticator.h"

VendorSpecificIEType_e
IsEpigramHTElement(void *priv, uint8 *pBuffer)
{
	phostsa_private psapriv = (phostsa_private)priv;
	hostsa_util_fns *util_fns = &psapriv->util_fns;
	VendorSpecificIEType_e retVal = VendSpecIE_Other;
	const uint8 szMatchingCapElement[] = { 0x00, 0x90, 0x4c, 0x33 };
	const uint8 szMatchingInfoElement[] = { 0x00, 0x90, 0x4c, 0x34 };

	if (!memcmp(util_fns, pBuffer,
		    szMatchingInfoElement, sizeof(szMatchingInfoElement))) {
		retVal = VendSpecIE_HT_Info;
	} else if (!memcmp(util_fns, pBuffer,
			   szMatchingCapElement,
			   sizeof(szMatchingCapElement))) {
		retVal = VendSpecIE_HT_Cap;
	}

	return retVal;
}

VendorSpecificIEType_e
IsWPSElement(void *priv, UINT8 *pBuffer)
{
	phostsa_private psapriv = (phostsa_private)priv;
	hostsa_util_fns *util_fns = &psapriv->util_fns;
	VendorSpecificIEType_e retVal = VendSpecIE_Other;
	const UINT8 szMatchingInfoElement[] = { 0x00, 0x50, 0xf2, 0x04 };

	if (!memcmp(util_fns, pBuffer,
		    szMatchingInfoElement, sizeof(szMatchingInfoElement))) {
		retVal = VendSpecIE_WPS;
	}

	return retVal;
}

VendorSpecificIEType_e
IsSsIdLElement(void *priv, UINT8 *pBuffer)
{
	phostsa_private psapriv = (phostsa_private)priv;
	hostsa_util_fns *util_fns = &psapriv->util_fns;
	VendorSpecificIEType_e retVal = VendSpecIE_Other;
	const UINT8 szMatchingInfoElement[] = { 0x00, 0x50, 0xf2, 0x05, 0x00 };

	if (!memcmp(util_fns, pBuffer,
		    szMatchingInfoElement, sizeof(szMatchingInfoElement))) {
		retVal = VendSpecIE_SsIdL;
	}

	return retVal;
}

int
ieBufValidate(UINT8 *pIe, int bufLen)
{
	while (bufLen) {
		UINT8 ieLen = *(pIe + 1);
		if (bufLen < (ieLen + 2)) {
			return MLME_FAILURE;
		}
		bufLen -= ieLen + 2;
		pIe += ieLen + 2;
	}

	return MLME_SUCCESS;
}

int
GetIEPointers(void *priv, UINT8 *pIe, int bufLen, IEPointers_t *pIePointers)
{
	phostsa_private psapriv = (phostsa_private)priv;
	hostsa_util_fns *util_fns = &psapriv->util_fns;
	memset(util_fns, pIePointers, 0x00, sizeof(IEPointers_t));

	while (bufLen) {
		if (bufLen < (*(pIe + 1) + 2)) {
			break;
		}

		/* Handle IEs not processed by ROM functions. */
		switch (*pIe) {
		case ELEM_ID_RSN:
			pIePointers->pRsn = (IEEEtypes_RSNElement_t *)pIe;
			break;
		case ELEM_ID_WAPI:
			pIePointers->pWapi = (IEEEtypes_WAPIElement_t *)pIe;
			break;

			/* Add element not handled by ROM_parser_getIEPtr or **
			   override element processing in ROM_parser_getIEPtr
			   ** here. */
		case ELEM_ID_VENDOR_SPECIFIC:
		default:
			if (ROM_parser_getIEPtr(priv, pIe, pIePointers) ==
			    FALSE) {
				if ((*pIe) == ELEM_ID_VENDOR_SPECIFIC) {
					if (IsWPSElement(priv, (pIe + 2))) {
						pIePointers->pWps =
							(IEEEtypes_WPSElement_t
							 *)pIe;
					}
				}
				// Add your code to process vendor specific
				// elements not
				// processed by above ROM_paser_getAssocIEPtr
				// function.
			}
			break;
		}
		bufLen -= *(pIe + 1) + 2;
		pIe += *(pIe + 1) + 2;
	}
	return bufLen;
}

BOOLEAN
parser_getAssocIEs(void *priv, UINT8 *pIe,
		   int bufLen, AssocIePointers_t *pIePointers)
{
	phostsa_private psapriv = (phostsa_private)priv;
	hostsa_util_fns *util_fns = &psapriv->util_fns;
	BOOLEAN ieParseSuccessful = TRUE;

	memset(util_fns, pIePointers, 0x00, sizeof(AssocIePointers_t));

	while (bufLen) {
		UINT8 ieType = *pIe;
		UINT8 ieLen = *(pIe + 1);

		if (bufLen < (ieLen + 2)) {
			ieParseSuccessful = FALSE;
			break;
		}

		switch (ieType) {
			// add code for elements not handled in ROM function.
		case ELEM_ID_AP_CHANNEL_REPORT:
			pIePointers->pApChanRpt =
				(IEEEtypes_ApChanRptElement_t *)pIe;
			break;
#ifdef TDLS
		case ELEM_ID_SUPPORTED_REGCLASS:
			pIePointers->pSuppRegClass =
				(IEEEtypes_SupportedRegClasses_t *)pIe;
			break;
#endif

			/* The following 5 elements, HT CAP, HT INFO, 20/40
			   Coex, OBSS SCAN PARAM, and EXTENDED CAP, are
			   ignored here if 11n is not compiled. When 11n is
			   compiled these 5 elements would be handled in
			   ROM_parser_getAssocIEPtr routine.

			 */
		case ELEM_ID_HT_CAPABILITY:
		case ELEM_ID_HT_INFORMATION:
		case ELEM_ID_2040_BSS_COEXISTENCE:
		case ELEM_ID_OBSS_SCAN_PARAM:
		case ELEM_ID_EXT_CAPABILITIES:
			/* Do not process these elements in ROM routine
			   ROM_parser_getAssocIEPtr Note: a break here. */
			break;

			/* Add element not handled by ROM_parser_getAssocIEPtr
			   or override element processing in
			   ROM_parser_getAssocIEPtr here. \ */

		case ELEM_ID_VENDOR_SPECIFIC:
		default:
			if (ROM_parser_getAssocIEPtr(priv, pIe, pIePointers) ==
			    FALSE) {
				// Add your code to process vendor specific
				// elements not
				// processed by above ROM_paser_getAssocIEPtr
				// function.
				if (!pIePointers->pHtCap ||
				    !pIePointers->pHtInfo) {
					switch (IsEpigramHTElement
						(priv, (pIe + 2))) {

					case VendSpecIE_HT_Cap:
						if (!pIePointers->pHtCap) {
							*(pIe + 4) =
								ELEM_ID_HT_CAPABILITY;
							*(pIe + 5) =
								sizeof
								(IEEEtypes_HT_Capability_t);
							pIePointers->pHtCap =
								(IEEEtypes_HT_Capability_t
								 *)(pIe + 4);
						}
						break;

					case VendSpecIE_HT_Info:
						if (!pIePointers->pHtInfo) {
							*(pIe + 4) =
								ELEM_ID_HT_INFORMATION;
							*(pIe + 5) =
								sizeof
								(IEEEtypes_HT_Information_t);
							pIePointers->pHtInfo =
								(IEEEtypes_HT_Information_t
								 *)(pIe + 4);
						}
						break;

					case VendSpecIE_Other:
					default:
						break;
					}
				}
			}
			break;

		}
		bufLen -= ieLen + 2;
		pIe += ieLen + 2;
	}
	return ieParseSuccessful;
}

UINT8
parser_countNumInfoElements(UINT8 *pIe, int bufLen)
{
	UINT8 ieCount = 0;

	while (bufLen) {
		if (bufLen < (*(pIe + 1) + 2)) {
			break;
		}

		ieCount++;

		bufLen -= *(pIe + 1) + 2;
		pIe += *(pIe + 1) + 2;
	}

	return ieCount;
}
