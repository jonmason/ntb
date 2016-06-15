/** @file parser_rom.h
 *
 *  @brief This file contains the data structrue for iepointer and declare the parse function
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
#ifndef PARSER_ROM_H__
#define PARSER_ROM_H__
#include "IEEE_types.h"

typedef enum {
	VendSpecIE_Other = 0,
	VendSpecIE_WMM_Info,
	VendSpecIE_WMM_Param,
	VendSpecIE_WPA,
	VendSpecIE_WPS,
	VendSpecIE_TSPEC,
	VendSpecIE_SsIdL,
	VendSpecIE_WFD,

	VendSpecIE_HT_Cap,
	VendSpecIE_HT_Info,

} VendorSpecificIEType_e;

typedef struct {
	/* IMPORTANT: please read before you modify this struct: Some of the
	   members of this struct are used in ROM code. Therefore, please do
	   not change any existing field, including its name and type. If you
	   want to add a new element into this struct add it at the end. */
	IEEEtypes_SsIdElement_t *pSsid;
	IEEEtypes_TimElement_t *pTim;
	IEEEtypes_WPAElement_t *pWpa;
	IEEEtypes_WMM_InfoElement_t *pWmmInfo;
	IEEEtypes_WMM_ParamElement_t *pWmmParam;
	IEEEtypes_DsParamElement_t *pDsParam;
	IEEEtypes_SuppRatesElement_t *pSupportedRates;
	IEEEtypes_ExtSuppRatesElement_t *pExtSupportedRates;
	IEEEtypes_ERPInfoElement_t *pErpInfo;
	IEEEtypes_IbssParamElement_t *pIbssParam;
	IEEEtypes_CountryInfoElement_t *pCountry;

	IEEEtypes_MobilityDomainElement_t *pMdie;

	IEEEtypes_RSNElement_t *pRsn;

	IEEEtypes_HT_Capability_t *pHtCap;
	IEEEtypes_HT_Information_t *pHtInfo;
	IEEEtypes_20N40_BSS_Coexist_t *p2040Coexist;
	IEEEtypes_OBSS_ScanParam_t *pHtScanParam;
	IEEEtypes_ExtCapability_t *pExtCap;

	IEEEtypes_WPSElement_t *pWps;
	IEEEtypes_WAPIElement_t *pWapi;

} IEPointers_t;

typedef struct {
	/* IMPORTANT: please read before you modify this struct: Some of the
	   members of this struct are used in ROM code. Therefore, please do
	   not change any existing field, including its name and type. If you
	   want to add a new element into this struct add it at the end. */
	IEEEtypes_SsIdElement_t *pSsid;
	IEEEtypes_TimElement_t *pTim;
	IEEEtypes_DsParamElement_t *pDsParam;

	IEEEtypes_CountryInfoElement_t *pCountry;

	UINT8 numSsIdLs;
	IEEEtypes_SsIdLElement_t *pSsIdL;	/* Only the first SSIDL found,
						   ** need iterator to get next
						   since ** multiple may be in
						   beacon */
} ScanIePointers_t;

typedef struct {
	/* IMPORTANT: please read before you modify this struct: Some of the
	   members of this struct are used in ROM code. Therefore, please do
	   not change any existing field, including its name and type. If you
	   want to add a new element into this struct add it at the end. */
	IEEEtypes_SsIdElement_t *pSsid;
	IEEEtypes_DsParamElement_t *pDsParam;

	IEEEtypes_CountryInfoElement_t *pCountry;
	IEEEtypes_ApChanRptElement_t *pApChanRpt;
	IEEEtypes_PowerConstraintElement_t *pPwrCon;

	IEEEtypes_SuppRatesElement_t *pSupportedRates;
	IEEEtypes_ExtSuppRatesElement_t *pExtSupportedRates;

	IEEEtypes_WPAElement_t *pWpa;
	IEEEtypes_WMM_InfoElement_t *pWmmInfo;
	IEEEtypes_WMM_ParamElement_t *pWmmParam;

	IEEEtypes_MobilityDomainElement_t *pMdie;

	IEEEtypes_RSNElement_t *pRsn;

	IEEEtypes_HT_Information_t *pHtInfo;
	IEEEtypes_HT_Capability_t *pHtCap;
	IEEEtypes_20N40_BSS_Coexist_t *p2040Coexist;
	IEEEtypes_OBSS_ScanParam_t *pHtScanParam;
	IEEEtypes_ExtCapability_t *pExtCap;

} AssocIePointers_t;
extern BOOLEAN ROM_parser_getIEPtr(void *priv, uint8 *pIe,
				   IEPointers_t *pIePointers);
extern BOOLEAN ROM_parser_getAssocIEPtr(void *priv, uint8 *pIe,
					AssocIePointers_t *pIePointers);

#endif // _PARSER_ROM_H_
