/** @file parser_rom.c
 *
 *  @brief This file define the  function for 802.11 Management Frames Parsing
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
#include "parser_rom.h"

#include "hostsa_ext_def.h"
#include "authenticator.h"

#define WMM_IE_MIN_LEN  7

VendorSpecificIEType_e
IsWMMElement(void *priv, uint8 *pBuffer)
{
	phostsa_private psapriv = (phostsa_private)priv;
	hostsa_util_fns *util_fns = &psapriv->util_fns;

	VendorSpecificIEType_e retVal = VendSpecIE_Other;
	const uint8 szMatchingInfoElement[] = { 0x00, 0x50, 0xf2,
		0x02, 0x00, 0x01
	};
	const uint8 szMatchingParamElement[] = { 0x00, 0x50, 0xf2,
		0x02, 0x01, 0x01
	};
	const uint8 szMatchingTspecElement[] = { 0x00, 0x50, 0xf2,
		0x02, 0x02, 0x01
	};

	if (!memcmp(util_fns, pBuffer,
		    szMatchingInfoElement, sizeof(szMatchingInfoElement))) {
		retVal = VendSpecIE_WMM_Info;
	} else if (!memcmp(util_fns, pBuffer,
			   szMatchingParamElement,
			   sizeof(szMatchingParamElement))) {
		retVal = VendSpecIE_WMM_Param;
	} else if (!memcmp(util_fns, pBuffer,
			   szMatchingTspecElement,
			   sizeof(szMatchingTspecElement))) {
		retVal = VendSpecIE_TSPEC;
	}

	return retVal;
}

VendorSpecificIEType_e
IsWPAElement(void *priv, uint8 *pBuffer)
{
	phostsa_private psapriv = (phostsa_private)priv;
	hostsa_util_fns *util_fns = &psapriv->util_fns;

	VendorSpecificIEType_e retVal = VendSpecIE_Other;
	const uint8 szMatchingInfoElement[] = { 0x00, 0x50, 0xf2,
		0x01, 0x01, 0x00
	};

	if (!memcmp(util_fns, pBuffer,
		    szMatchingInfoElement, sizeof(szMatchingInfoElement))) {
		retVal = VendSpecIE_WPA;
	}

	return retVal;
}

BOOLEAN
ROM_parser_getIEPtr(void *priv, uint8 *pIe, IEPointers_t *pIePointers)
{
	BOOLEAN status = TRUE;
	switch (*pIe) {
	case ELEM_ID_SSID:
		pIePointers->pSsid = (IEEEtypes_SsIdElement_t *)pIe;
		break;

	case ELEM_ID_DS_PARAM_SET:
		pIePointers->pDsParam = (IEEEtypes_DsParamElement_t *)pIe;
		break;

	case ELEM_ID_TIM:
		pIePointers->pTim = (IEEEtypes_TimElement_t *)pIe;
		break;

	case ELEM_ID_SUPPORTED_RATES:
		pIePointers->pSupportedRates =
			(IEEEtypes_SuppRatesElement_t *)pIe;
		break;

	case ELEM_ID_EXT_SUPPORTED_RATES:
		pIePointers->pExtSupportedRates
			= (IEEEtypes_ExtSuppRatesElement_t *)pIe;
		break;

	case ELEM_ID_ERP_INFO:
		pIePointers->pErpInfo = (IEEEtypes_ERPInfoElement_t *)pIe;
		break;

	case ELEM_ID_IBSS_PARAM_SET:
		pIePointers->pIbssParam = (IEEEtypes_IbssParamElement_t *)pIe;
		break;

	case ELEM_ID_COUNTRY:
		pIePointers->pCountry = (IEEEtypes_CountryInfoElement_t *)pIe;
		break;

	case ELEM_ID_RSN:
		pIePointers->pRsn = (IEEEtypes_RSNElement_t *)pIe;
		break;

	case ELEM_ID_VENDOR_SPECIFIC:

		if (IsWPAElement(priv, (pIe + 2))) {
			pIePointers->pWpa = (IEEEtypes_WPAElement_t *)pIe;
		} else {
			switch (IsWMMElement(priv, (pIe + 2))) {
			case VendSpecIE_Other:
			case VendSpecIE_TSPEC:
			default:
				status = FALSE;
				break;

			case VendSpecIE_WMM_Info:
				pIePointers->pWmmInfo =
					(IEEEtypes_WMM_InfoElement_t *)pIe;
				break;

			case VendSpecIE_WMM_Param:
				pIePointers->pWmmParam
					= (IEEEtypes_WMM_ParamElement_t *)pIe;
				break;
			}

		}
		break;
	default:
		status = FALSE;
		break;
	}

	return status;
}

BOOLEAN
ROM_parser_getAssocIEPtr(void *priv, uint8 *pIe, AssocIePointers_t *pIePointers)
{
	BOOLEAN status = TRUE;
	switch (*pIe) {
	case ELEM_ID_SSID:
		pIePointers->pSsid = (IEEEtypes_SsIdElement_t *)pIe;
		break;

	case ELEM_ID_COUNTRY:
		pIePointers->pCountry = (IEEEtypes_CountryInfoElement_t *)pIe;
		break;

	case ELEM_ID_DS_PARAM_SET:
		pIePointers->pDsParam = (IEEEtypes_DsParamElement_t *)pIe;
		break;

	case ELEM_ID_SUPPORTED_RATES:
		pIePointers->pSupportedRates =
			(IEEEtypes_SuppRatesElement_t *)pIe;
		break;

	case ELEM_ID_EXT_SUPPORTED_RATES:
		pIePointers->pExtSupportedRates
			= (IEEEtypes_ExtSuppRatesElement_t *)pIe;
		break;

	case ELEM_ID_RSN:
		pIePointers->pRsn = (IEEEtypes_RSNElement_t *)pIe;
		break;

	case ELEM_ID_VENDOR_SPECIFIC:
		if (IsWPAElement(priv, (pIe + 2))) {
			pIePointers->pWpa = (IEEEtypes_WPAElement_t *)pIe;
		} else {
			switch (IsWMMElement(priv, (pIe + 2))) {
			case VendSpecIE_Other:
			case VendSpecIE_TSPEC:
			default:
				status = FALSE;
				break;

			case VendSpecIE_WMM_Info:
				pIePointers->pWmmInfo
					= (IEEEtypes_WMM_InfoElement_t *)pIe;
				break;

			case VendSpecIE_WMM_Param:
				pIePointers->pWmmParam
					= (IEEEtypes_WMM_ParamElement_t *)pIe;
				break;
			}
		}
		break;
	default:
		status = FALSE;
		break;
	}

	return status;
}

IEEEtypes_InfoElementHdr_t *
parser_getSpecificIE(IEEEtypes_ElementId_e elemId, UINT8 *pIe, int bufLen)
{
	if (!pIe) {
		return NULL;
	}

	while (bufLen) {
		if (bufLen < (*(pIe + 1) + 2)) {
			break;
		}

		if (*pIe == elemId) {
			return (IEEEtypes_InfoElementHdr_t *)pIe;
		}

		bufLen -= *(pIe + 1) + 2;
		pIe += *(pIe + 1) + 2;
	}

	return NULL;
}
