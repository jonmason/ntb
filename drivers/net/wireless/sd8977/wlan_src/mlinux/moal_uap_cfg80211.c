/** @file moal_uap_cfg80211.c
  *
  * @brief This file contains the functions for uAP CFG80211.
  *
  * Copyright (C) 2011-2016, Marvell International Ltd.
  *
  * This software file (the "File") is distributed by Marvell International
  * Ltd. under the terms of the GNU General Public License Version 2, June 1991
  * (the "License").  You may use, redistribute and/or modify this File in
  * accordance with the terms and conditions of the License, a copy of which
  * is available by writing to the Free Software Foundation, Inc.,
  * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
  * worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
  *
  * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
  * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
  * this warranty disclaimer.
  *
  */

#include "moal_cfg80211.h"
#include "moal_uap_cfg80211.h"
/** secondary channel is below */
#define SECOND_CHANNEL_BELOW    0x30
/** secondary channel is above */
#define SECOND_CHANNEL_ABOVE    0x10
/** no secondary channel */
#define SECONDARY_CHANNEL_NONE     0x00
/** deauth reason code */
#define  REASON_CODE_DEAUTH_LEAVING 3
/********************************************************
				Local Variables
********************************************************/

/********************************************************
				Global Variables
********************************************************/

/********************************************************
				Local Functions
********************************************************/

/********************************************************
				Global Functions
********************************************************/

/**
 * @brief send deauth to station
 *
 * @param                 A pointer to moal_private
 * @param mac			  A pointer to station mac address
 * @param reason_code     ieee deauth reason code
 * @return                0 -- success, otherwise fail
 */
static int
woal_deauth_station(moal_private *priv, u8 *mac_addr, u16 reason_code)
{
	mlan_ioctl_req *ioctl_req = NULL;
	mlan_ds_bss *bss = NULL;
	int ret = 0;
	mlan_status status = MLAN_STATUS_SUCCESS;

	ENTER();

	ioctl_req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
	if (ioctl_req == NULL) {
		ret = -ENOMEM;
		goto done;
	}
	bss = (mlan_ds_bss *)ioctl_req->pbuf;
	bss->sub_command = MLAN_OID_UAP_DEAUTH_STA;
	ioctl_req->req_id = MLAN_IOCTL_BSS;
	ioctl_req->action = MLAN_ACT_SET;

	memcpy(bss->param.deauth_param.mac_addr, mac_addr,
	       MLAN_MAC_ADDR_LENGTH);
	bss->param.deauth_param.reason_code = reason_code;
	status = woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT);
	if (status != MLAN_STATUS_SUCCESS) {
		ret = -EFAULT;
		goto done;
	}

done:
	if (status != MLAN_STATUS_PENDING)
		kfree(ioctl_req);
	LEAVE();
	return ret;
}

/**
 * @brief send deauth to all station
 *
 * @param                 A pointer to moal_private
 * @param mac			  A pointer to station mac address
 *
 * @return                0 -- success, otherwise fail
 */
static int
woal_deauth_all_station(moal_private *priv)
{
	int ret = -EFAULT;
	int i = 0;
	mlan_ds_get_info *info = NULL;
	mlan_ioctl_req *ioctl_req = NULL;
	mlan_status status = MLAN_STATUS_SUCCESS;

	ENTER();
	if (priv->media_connected == MFALSE) {
		PRINTM(MINFO, "cfg80211: Media not connected!\n");
		LEAVE();
		return 0;
	}
	PRINTM(MIOCTL, "del all station\n");
	/* Allocate an IOCTL request buffer */
	ioctl_req =
		(mlan_ioctl_req *)
		woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_get_info));
	if (ioctl_req == NULL) {
		ret = -ENOMEM;
		goto done;
	}

	info = (mlan_ds_get_info *)ioctl_req->pbuf;
	info->sub_command = MLAN_OID_UAP_STA_LIST;
	ioctl_req->req_id = MLAN_IOCTL_GET_INFO;
	ioctl_req->action = MLAN_ACT_GET;

	status = woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT);
	if (status != MLAN_STATUS_SUCCESS)
		goto done;
	if (!info->param.sta_list.sta_count)
		goto done;
	for (i = 0; i < info->param.sta_list.sta_count; i++) {
		PRINTM(MIOCTL, "deauth station " MACSTR "\n",
		       MAC2STR(info->param.sta_list.info[i].mac_address));
		ret = woal_deauth_station(priv,
					  info->param.sta_list.info[i].
					  mac_address,
					  REASON_CODE_DEAUTH_LEAVING);
	}
	woal_sched_timeout(200);
done:
	if (status != MLAN_STATUS_PENDING)
		kfree(ioctl_req);
	return ret;
}

/**
 * @brief Verify RSN IE
 *
 * @param rsn_ie          Pointer RSN IE
 * @param sys_config      Pointer to mlan_uap_bss_param structure
 *
 * @return                MTRUE/MFALSE
 */
static t_u8
woal_check_rsn_ie(IEEEtypes_Rsn_t *rsn_ie, mlan_uap_bss_param *sys_config)
{
	int left = 0;
	int count = 0;
	int i = 0;
	wpa_suite_auth_key_mgmt_t *key_mgmt = NULL;
	left = rsn_ie->len + 2;
	if (left < sizeof(IEEEtypes_Rsn_t))
		return MFALSE;
	sys_config->wpa_cfg.group_cipher = 0;
	sys_config->wpa_cfg.pairwise_cipher_wpa2 = 0;
	sys_config->key_mgmt = 0;
	/* check the group cipher */
	switch (rsn_ie->group_cipher.type) {
	case WPA_CIPHER_TKIP:
		sys_config->wpa_cfg.group_cipher = CIPHER_TKIP;
		break;
	case WPA_CIPHER_AES_CCM:
		sys_config->wpa_cfg.group_cipher = CIPHER_AES_CCMP;
		break;
	default:
		break;
	}
	count = le16_to_cpu(rsn_ie->pairwise_cipher.count);
	for (i = 0; i < count; i++) {
		switch (rsn_ie->pairwise_cipher.list[i].type) {
		case WPA_CIPHER_TKIP:
			sys_config->wpa_cfg.pairwise_cipher_wpa2 |= CIPHER_TKIP;
			break;
		case WPA_CIPHER_AES_CCM:
			sys_config->wpa_cfg.pairwise_cipher_wpa2 |=
				CIPHER_AES_CCMP;
			break;
		default:
			break;
		}
	}
	left -= sizeof(IEEEtypes_Rsn_t) + (count - 1) * sizeof(wpa_suite);
	if (left < sizeof(wpa_suite_auth_key_mgmt_t))
		return MFALSE;
	key_mgmt =
		(wpa_suite_auth_key_mgmt_t *)((u8 *)rsn_ie +
					      sizeof(IEEEtypes_Rsn_t) + (count -
									 1) *
					      sizeof(wpa_suite));
	count = le16_to_cpu(key_mgmt->count);
	if (left <
	    (sizeof(wpa_suite_auth_key_mgmt_t) +
	     (count - 1) * sizeof(wpa_suite)))
		return MFALSE;
	for (i = 0; i < count; i++) {
		switch (key_mgmt->list[i].type) {
		case RSN_AKM_8021X:
			sys_config->key_mgmt |= KEY_MGMT_EAP;
			break;
		case RSN_AKM_PSK:
			sys_config->key_mgmt |= KEY_MGMT_PSK;
			break;
		case RSN_AKM_PSK_SHA256:
			sys_config->key_mgmt |= KEY_MGMT_PSK_SHA256;
			break;
		}
	}
	return MTRUE;
}

/**
 * @brief Verify WPA IE
 *
 * @param wpa_ie          Pointer WPA IE
 * @param sys_config      Pointer to mlan_uap_bss_param structure
 *
 * @return                MTRUE/MFALSE
 */
static t_u8
woal_check_wpa_ie(IEEEtypes_Wpa_t *wpa_ie, mlan_uap_bss_param *sys_config)
{
	int left = 0;
	int count = 0;
	int i = 0;
	wpa_suite_auth_key_mgmt_t *key_mgmt = NULL;
	left = wpa_ie->len + 2;
	if (left < sizeof(IEEEtypes_Wpa_t))
		return MFALSE;
	sys_config->wpa_cfg.group_cipher = 0;
	sys_config->wpa_cfg.pairwise_cipher_wpa = 0;
	switch (wpa_ie->group_cipher.type) {
	case WPA_CIPHER_TKIP:
		sys_config->wpa_cfg.group_cipher = CIPHER_TKIP;
		break;
	case WPA_CIPHER_AES_CCM:
		sys_config->wpa_cfg.group_cipher = CIPHER_AES_CCMP;
		break;
	default:
		break;
	}
	count = le16_to_cpu(wpa_ie->pairwise_cipher.count);
	for (i = 0; i < count; i++) {
		switch (wpa_ie->pairwise_cipher.list[i].type) {
		case WPA_CIPHER_TKIP:
			sys_config->wpa_cfg.pairwise_cipher_wpa |= CIPHER_TKIP;
			break;
		case WPA_CIPHER_AES_CCM:
			sys_config->wpa_cfg.pairwise_cipher_wpa |=
				CIPHER_AES_CCMP;
			break;
		default:
			break;
		}
	}
	left -= sizeof(IEEEtypes_Wpa_t) + (count - 1) * sizeof(wpa_suite);
	if (left < sizeof(wpa_suite_auth_key_mgmt_t))
		return MFALSE;
	key_mgmt =
		(wpa_suite_auth_key_mgmt_t *)((u8 *)wpa_ie +
					      sizeof(IEEEtypes_Wpa_t) + (count -
									 1) *
					      sizeof(wpa_suite));
	count = le16_to_cpu(key_mgmt->count);
	if (left <
	    (sizeof(wpa_suite_auth_key_mgmt_t) +
	     (count - 1) * sizeof(wpa_suite)))
		return MFALSE;
	for (i = 0; i < count; i++) {
		switch (key_mgmt->list[i].type) {
		case RSN_AKM_8021X:
			sys_config->key_mgmt = KEY_MGMT_EAP;
			break;
		case RSN_AKM_PSK:
			sys_config->key_mgmt = KEY_MGMT_PSK;
			break;
		}
	}
	return MTRUE;
}

/**
 * @brief Find RSN/WPA IES
 *
 * @param ie              Pointer IE buffer
 * @param sys_config      Pointer to mlan_uap_bss_param structure
 *
 * @return                MTRUE/MFALSE
 */
static t_u8
woal_find_wpa_ies(const t_u8 *ie, int len, mlan_uap_bss_param *sys_config)
{
	int bytes_left = len;
	const t_u8 *pcurrent_ptr = ie;
	t_u16 total_ie_len;
	t_u8 element_len;
	t_u8 wpa2 = 0;
	t_u8 wpa = 0;
	t_u8 ret = MFALSE;
	IEEEtypes_ElementId_e element_id;
	IEEEtypes_VendorSpecific_t *pvendor_ie;
	const t_u8 wpa_oui[4] = { 0x00, 0x50, 0xf2, 0x01 };

	while (bytes_left >= 2) {
		element_id = (IEEEtypes_ElementId_e)(*((t_u8 *)pcurrent_ptr));
		element_len = *((t_u8 *)pcurrent_ptr + 1);
		total_ie_len = element_len + sizeof(IEEEtypes_Header_t);
		if (bytes_left < total_ie_len) {
			PRINTM(MERROR,
			       "InterpretIE: Error in processing IE, bytes left < IE length\n");
			bytes_left = 0;
			continue;
		}
		switch (element_id) {
		case RSN_IE:
			wpa2 = woal_check_rsn_ie((IEEEtypes_Rsn_t *)
						 pcurrent_ptr, sys_config);
			break;
		case VENDOR_SPECIFIC_221:
			pvendor_ie = (IEEEtypes_VendorSpecific_t *)pcurrent_ptr;
			if (!memcmp
			    (pvendor_ie->vend_hdr.oui, wpa_oui,
			     sizeof(pvendor_ie->vend_hdr.oui)) &&
			    (pvendor_ie->vend_hdr.oui_type == wpa_oui[3])) {
				wpa = woal_check_wpa_ie((IEEEtypes_Wpa_t *)
							pcurrent_ptr,
							sys_config);
			}
			break;
		default:
			break;
		}
		pcurrent_ptr += element_len + 2;
		/* Need to account for IE ID and IE Len */
		bytes_left -= (element_len + 2);
	}
	if (wpa && wpa2) {
		sys_config->protocol = PROTOCOL_WPA | PROTOCOL_WPA2;
		ret = MTRUE;
	} else if (wpa2) {
		sys_config->protocol = PROTOCOL_WPA2;
		ret = MTRUE;
	} else if (wpa) {
		sys_config->protocol = PROTOCOL_WPA;
		ret = MTRUE;
	}
	return ret;
}

/**
 * @brief Find and set WMM IES
 *
 * @param ie              Pointer IE buffer
 * @param sys_config      Pointer to mlan_uap_bss_param structure
 *
 * @return                N/A
 */
static t_void
woal_set_wmm_ies(const t_u8 *ie, int len, mlan_uap_bss_param *sys_config)
{
	int bytes_left = len;
	const t_u8 *pcurrent_ptr = ie;
	t_u16 total_ie_len;
	t_u8 element_len;
	IEEEtypes_VendorSpecific_t *pvendor_ie;
	IEEEtypes_ElementId_e element_id;
	const t_u8 wmm_oui[4] = { 0x00, 0x50, 0xf2, 0x02 };

	while (bytes_left >= 2) {
		element_id = (IEEEtypes_ElementId_e)(*((t_u8 *)pcurrent_ptr));
		element_len = *((t_u8 *)pcurrent_ptr + 1);
		total_ie_len = element_len + sizeof(IEEEtypes_Header_t);
		if (bytes_left < total_ie_len) {
			PRINTM(MERROR,
			       "InterpretIE: Error in processing IE, bytes left < IE length\n");
			bytes_left = 0;
			continue;
		}
		switch (element_id) {
		case VENDOR_SPECIFIC_221:
			pvendor_ie = (IEEEtypes_VendorSpecific_t *)pcurrent_ptr;
			if (!memcmp
			    (pvendor_ie->vend_hdr.oui, wmm_oui,
			     sizeof(wmm_oui))) {
				if (total_ie_len ==
				    sizeof(IEEEtypes_WmmParameter_t)) {
					/*
					 * Only accept and copy the WMM IE if
					 * it matches the size expected for the
					 * WMM Parameter IE.
					 */
					memcpy(&sys_config->wmm_para,
					       pcurrent_ptr +
					       sizeof(IEEEtypes_Header_t),
					       element_len);
				}
			}

			break;
		default:
			break;
		}
		pcurrent_ptr += element_len + 2;
		/* Need to account for IE ID and IE Len */
		bytes_left -= (element_len + 2);
	}

}

/**
 * @brief Get second channel offset
 *
 * @param chan             channel num
 * @return                second channel offset
 */
static t_u8
woal_get_second_channel_offset(int chan)
{
	t_u8 chan2Offset = SECONDARY_CHANNEL_NONE;

	switch (chan) {
	case 36:
	case 44:
	case 52:
	case 60:
	case 100:
	case 108:
	case 116:
	case 124:
	case 132:
	case 140:
	case 149:
	case 157:
		chan2Offset = SECOND_CHANNEL_ABOVE;
		break;
	case 40:
	case 48:
	case 56:
	case 64:
	case 104:
	case 112:
	case 120:
	case 128:
	case 136:
	case 144:
	case 153:
	case 161:
		chan2Offset = SECOND_CHANNEL_BELOW;
		break;
	case 165:
		/* Special Case: 20Mhz-only Channel */
		chan2Offset = SECONDARY_CHANNEL_NONE;
		break;
	}
	return chan2Offset;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
/**
 * @brief initialize AP or GO bss config
 * @param priv            A pointer to moal private structure
 * @param params          A pointer to cfg80211_ap_settings structure
 * @return                0 -- success, otherwise fail
 */
t_u8
woal_check_11ac_capability(moal_private *priv,
			   struct cfg80211_ap_settings *params)
#else
/**
 * @brief initialize AP or GO bss config
 * @param priv            A pointer to moal private structure
 * @return                0 -- success, otherwise fail
 */
t_u8
woal_check_11ac_capability(moal_private *priv)
#endif
{
	mlan_fw_info fw_info;
	t_u8 enable_11ac = MFALSE;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
	const u8 *vht_ie = NULL;
#endif
	ENTER();
	woal_request_get_fw_info(priv, MOAL_IOCTL_WAIT, &fw_info);
	if (!(fw_info.fw_bands & BAND_AAC)) {
		LEAVE();
		return enable_11ac;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
	vht_ie = cfg80211_find_ie(WLAN_EID_VHT_CAPABILITY, params->beacon.tail,
				  params->beacon.tail_len);
	if (vht_ie)
		enable_11ac = MTRUE;
	else
		enable_11ac = MFALSE;
#else
	enable_11ac = MTRUE;
#endif
	LEAVE();
	return enable_11ac;
}

/**
 * @brief get ht_cap from beacon ie
 *
 * @param ie              Pointer to IEs
 * @param len             Total length of ie
 *
 * @return                ht_cap
 */
static t_u16
woal_get_htcap_from_beacon_ies(const t_u8 *ie, int len)
{
	int left_len = len;
	t_u8 *pos = (t_u8 *)ie;
	int length;
	t_u8 id = 0;
	IEEEtypes_HTCap_t *phtcap_ie = NULL;

	/* HT_CAPABILITY will be fileter out */
	while (left_len >= 2) {
		length = *(pos + 1);
		id = *pos;
		if ((length + 2) > left_len)
			break;
		if (HT_CAPABILITY == id) {
			phtcap_ie = (IEEEtypes_HTCap_t *)pos;
			PRINTM(MINFO, "Get ht_cap from beacon ies: 0x%x\n",
			       phtcap_ie->ht_cap.ht_cap_info);
			return phtcap_ie->ht_cap.ht_cap_info;
		}
		pos += (length + 2);
		left_len -= (length + 2);
	}
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
/** Starting Frequency for 11A band */
#define START_FREQ_11A_BAND     5000	/* in MHz */
/**
 * @brief convert channel width

 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param chandef                 A pointer to cfg80211_chan_def structure
 *
 * @return                0 -- success, otherwise fail
 */
static t_u8
woal_covert_chan_width(enum nl80211_chan_width width)
{
	t_u8 ret;

	ENTER();

	switch (width) {
	case NL80211_CHAN_WIDTH_20_NOHT:
	case NL80211_CHAN_WIDTH_20:
		ret = 0;
		break;
	case NL80211_CHAN_WIDTH_40:
		ret = 2;
		break;
	case NL80211_CHAN_WIDTH_80:
		ret = 3;
		break;
	default:
		/* for other cases, we have declared our Driver and Fw do NOT
		   support when register wiphy. so kernel would not deliver
		   other cases to Driver */
		ret = 0xff;
	}
	LEAVE();
	return ret;
}

/**
 * @brief Enable radar detect for DFS channel
 *
 * @param priv            A pointer to moal private structure
 * @param chandef         A pointer to cfg80211_chan_def structure
 * @return                N/A
 */
static void
woal_enable_dfs_support(moal_private *priv, struct cfg80211_chan_def *chandef)
{
	mlan_ioctl_req *req = NULL;
	mlan_ds_11h_chan_rep_req *pchan_rpt_req = NULL;
	mlan_ds_11h_cfg *p11h_cfg = NULL;
	mlan_status status = MLAN_STATUS_SUCCESS;
	ENTER();
	if (!(chandef->chan->flags & IEEE80211_CHAN_RADAR)) {
		PRINTM(MIOCTL, "No radar channel\n");
		LEAVE();
		return;
	}
	PRINTM(MIOCTL, "start Radar detect, chan %d , Bw %d \n",
	       chandef->chan->hw_value, chandef->width);

	req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_11h_cfg));
	if (NULL == req) {
		PRINTM(MIOCTL, "No Memory to allocate ioctl buffer\n");
		LEAVE();
		return;
	}
	p11h_cfg = (mlan_ds_11h_cfg *)req->pbuf;
	pchan_rpt_req = &p11h_cfg->param.chan_rpt_req;
	pchan_rpt_req->startFreq = 5000;
	pchan_rpt_req->chanNum = (t_u8)chandef->chan->hw_value;
	pchan_rpt_req->chanWidth = woal_covert_chan_width(chandef->width);
	pchan_rpt_req->host_based = MTRUE;
	pchan_rpt_req->millisec_dwell_time = 0;

	p11h_cfg->sub_command = MLAN_OID_11H_CHANNEL_CHECK;
	req->req_id = MLAN_IOCTL_11H_CFG;
	req->action = MLAN_ACT_SET;
	/* Send Channel Check command and wait until the report is ready */
	status = woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT);
	if (status != MLAN_STATUS_PENDING)
		kfree(req);
	LEAVE();
	return;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
/**
 * @brief initialize AP or GO bss config
 *
 * @param priv            A pointer to moal private structure
 * @param params          A pointer to cfg80211_ap_settings structure
 * @return                0 -- success, otherwise fail
 */
static int
woal_cfg80211_beacon_config(moal_private *priv,
			    struct cfg80211_ap_settings *params)
#else
/**
 * @brief initialize AP or GO bss config
 *
 * @param priv            A pointer to moal private structure
 * @param params          A pointer to beacon_parameters structure
 * @return                0 -- success, otherwise fail
 */
static int
woal_cfg80211_beacon_config(moal_private *priv,
			    struct beacon_parameters *params)
#endif
{
	struct wiphy *wiphy = NULL;
	int ret = 0;
	mlan_uap_bss_param sys_config;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0) || defined(COMPAT_WIRELESS)
	int i = 0;
#else
	t_u8 wpa_ies;
	const t_u8 *ssid_ie = NULL;
	struct ieee80211_mgmt *head = NULL;
	t_u16 capab_info = 0;
#endif
	t_u8 rates_bg[13] = {
		0x82, 0x84, 0x8b, 0x96,
		0x0c, 0x12, 0x18, 0x24,
		0x30, 0x48, 0x60, 0x6c,
		0x00
	};
	t_u8 rates_a[9] = {
		0x8c, 0x12, 0x98, 0x24,
		0xb0, 0x48, 0x60, 0x6c,
		0x00
	};
#ifdef WIFI_DIRECT_SUPPORT
	t_u8 rates_wfd[9] = {
		0x8c, 0x12, 0x18, 0x24,
		0x30, 0x48, 0x60, 0x6c,
		0x00
	};
#endif
	t_u8 chan2Offset = 0;
	t_u8 enable_11n = MTRUE;
	t_u16 ht_cap = 0;
	t_u8 enable_11ac = MFALSE;
	t_u8 vht20_40 = MFALSE;
	ENTER();

	if (params == NULL) {
		ret = -EFAULT;
		goto done;
	}

	wiphy = priv->phandle->wiphy;
	if (priv->bss_type != MLAN_BSS_TYPE_UAP
#ifdef WIFI_DIRECT_SUPPORT
	    && priv->bss_type != MLAN_BSS_TYPE_WIFIDIRECT
#endif
		) {
		ret = -EFAULT;
		goto done;
	}

	/* Initialize the uap bss values which are uploaded from firmware */
	if (MLAN_STATUS_SUCCESS != woal_set_get_sys_config(priv,
							   MLAN_ACT_GET,
							   MOAL_IOCTL_WAIT,
							   &sys_config)) {
		PRINTM(MERROR, "Error getting AP confiruration\n");
		ret = -EFAULT;
		goto done;
	}

	/* Setting the default values */
	sys_config.channel = 6;
	sys_config.preamble_type = 0;
	sys_config.mgmt_ie_passthru_mask = priv->mgmt_subtype_mask;
	memcpy(sys_config.mac_addr, priv->current_addr, ETH_ALEN);

	/* Set frag_threshold, rts_threshold, and retry limit */
	sys_config.frag_threshold = wiphy->frag_threshold;
	sys_config.rts_threshold = wiphy->rts_threshold;
	sys_config.retry_limit = wiphy->retry_long;
	if (sys_config.frag_threshold == MLAN_FRAG_RTS_DISABLED) {
		sys_config.frag_threshold = MLAN_FRAG_MAX_VALUE;
	}
	if (sys_config.rts_threshold == MLAN_FRAG_RTS_DISABLED) {
		sys_config.rts_threshold = MLAN_RTS_MAX_VALUE;
	}

	if (priv->bss_type == MLAN_BSS_TYPE_UAP) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
		if (params->beacon_interval)
			sys_config.beacon_period = params->beacon_interval;
#else
		if (params->interval)
			sys_config.beacon_period = params->interval;
#endif
		if (params->dtim_period)
			sys_config.dtim_period = params->dtim_period;
	}
	if (priv->channel) {
		memset(sys_config.rates, 0, sizeof(sys_config.rates));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
		switch (params->chandef.width) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0)
		case NL80211_CHAN_WIDTH_5:
		case NL80211_CHAN_WIDTH_10:
#endif
		case NL80211_CHAN_WIDTH_20_NOHT:
			enable_11n = MFALSE;
			break;
		case NL80211_CHAN_WIDTH_20:
			break;
		case NL80211_CHAN_WIDTH_40:
			if (params->chandef.center_freq1 <
			    params->chandef.chan->center_freq)
				chan2Offset = SECOND_CHANNEL_BELOW;
			else
				chan2Offset = SECOND_CHANNEL_ABOVE;
			break;
		case NL80211_CHAN_WIDTH_80:
		case NL80211_CHAN_WIDTH_80P80:
		case NL80211_CHAN_WIDTH_160:
			chan2Offset =
				woal_get_second_channel_offset(priv->channel);
			break;
		default:
			PRINTM(MWARN, "Unknown channel width: %d\n",
			       params->chandef.width);
			break;
		}
#else
		switch (params->channel_type) {
		case NL80211_CHAN_NO_HT:
			enable_11n = MFALSE;
			break;
		case NL80211_CHAN_HT20:
			break;
		case NL80211_CHAN_HT40PLUS:
			chan2Offset = SECOND_CHANNEL_ABOVE;
			break;
		case NL80211_CHAN_HT40MINUS:
			chan2Offset = SECOND_CHANNEL_BELOW;
			break;
		default:
			PRINTM(MWARN, "Unknown channel type: %d\n",
			       params->channel_type);
			break;
		}
#endif
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0) */
		sys_config.channel = priv->channel;
		if (priv->channel <= MAX_BG_CHANNEL) {
			sys_config.band_cfg = BAND_CONFIG_2G;
#ifdef WIFI_DIRECT_SUPPORT
			if (priv->bss_type == MLAN_BSS_TYPE_WIFIDIRECT)
				memcpy(sys_config.rates, rates_wfd,
				       sizeof(rates_wfd));
			else
#endif
				memcpy(sys_config.rates, rates_bg,
				       sizeof(rates_bg));
		} else {
			sys_config.band_cfg = BAND_CONFIG_5G;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
			chan2Offset =
				woal_get_second_channel_offset(priv->channel);
#else
#ifdef WIFI_DIRECT_SUPPORT
			/* Force enable 40MHZ on WFD interface */
			if (priv->bss_type == MLAN_BSS_TYPE_WIFIDIRECT)
				chan2Offset =
					woal_get_second_channel_offset(priv->
								       channel);
#endif
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
			enable_11ac = woal_check_11ac_capability(priv, params);
			if (enable_11ac &&
			    ((params->chandef.width == NL80211_CHAN_WIDTH_20)
			     || (params->chandef.width ==
				 NL80211_CHAN_WIDTH_40)))
				vht20_40 = MTRUE;
#else
			enable_11ac = woal_check_11ac_capability(priv);
#endif
#ifdef WIFI_DIRECT_SUPPORT
			if (priv->bss_type == MLAN_BSS_TYPE_WIFIDIRECT)
				memcpy(sys_config.rates, rates_wfd,
				       sizeof(rates_wfd));
			else
#endif
				memcpy(sys_config.rates, rates_a,
				       sizeof(rates_a));
		}
		/* Disable GreenField by default */
		sys_config.ht_cap_info = 0x10c;
		if (enable_11n)
			sys_config.ht_cap_info |= 0x20;
		if (chan2Offset) {
			sys_config.band_cfg |= chan2Offset;
			sys_config.ht_cap_info |= 0x1042;
			sys_config.ampdu_param = 3;
		}
		if (priv->bss_type == MLAN_BSS_TYPE_UAP) {
			ht_cap = woal_get_htcap_from_beacon_ies(
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
								       params->
								       beacon.
								       tail,
								       params->
								       beacon.
								       tail_len
#else
								       params->
								       tail,
								       params->
								       tail_len
#endif
				);
			if (ht_cap)
				sys_config.ht_cap_info =
					ht_cap & (wiphy->
						  bands[IEEE80211_BAND_2GHZ]->
						  ht_cap.cap & 0x10f3);
		}
		PRINTM(MCMND,
		       "11n=%d, ht_cap=0x%x, channel=%d, band_cfg=0x%x\n",
		       enable_11n, sys_config.ht_cap_info, priv->channel,
		       sys_config.band_cfg);
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0) || defined(COMPAT_WIRELESS)
	if (!params->ssid || !params->ssid_len) {
		ret = -EINVAL;
		goto done;
	}
	memcpy(sys_config.ssid.ssid, params->ssid,
	       MIN(MLAN_MAX_SSID_LENGTH, params->ssid_len));
	sys_config.ssid.ssid_len = MIN(MLAN_MAX_SSID_LENGTH, params->ssid_len);
	if (params->hidden_ssid)
		sys_config.bcast_ssid_ctl = 0;
	else
		sys_config.bcast_ssid_ctl = 1;
	if (params->auth_type == NL80211_AUTHTYPE_SHARED_KEY)
		sys_config.auth_mode = MLAN_AUTH_MODE_SHARED;
	else
		sys_config.auth_mode = MLAN_AUTH_MODE_OPEN;
	if (params->crypto.n_akm_suites) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
		woal_find_wpa_ies(params->beacon.tail,
				  (int)params->beacon.tail_len, &sys_config);
#else
		woal_find_wpa_ies(params->tail, params->tail_len, &sys_config);
#endif
	}
	for (i = 0; i < params->crypto.n_akm_suites; i++) {
		switch (params->crypto.akm_suites[i]) {
		case WLAN_AKM_SUITE_8021X:
			sys_config.key_mgmt |= KEY_MGMT_EAP;
			if ((params->crypto.
			     wpa_versions & NL80211_WPA_VERSION_1) &&
			    (params->crypto.
			     wpa_versions & NL80211_WPA_VERSION_2))
				sys_config.protocol =
					PROTOCOL_WPA | PROTOCOL_WPA2;
			else if (params->crypto.
				 wpa_versions & NL80211_WPA_VERSION_2)
				sys_config.protocol = PROTOCOL_WPA2;
			else if (params->crypto.
				 wpa_versions & NL80211_WPA_VERSION_1)
				sys_config.protocol = PROTOCOL_WPA;
			break;
		case WLAN_AKM_SUITE_PSK:
			sys_config.key_mgmt |= KEY_MGMT_PSK;
			if ((params->crypto.
			     wpa_versions & NL80211_WPA_VERSION_1) &&
			    (params->crypto.
			     wpa_versions & NL80211_WPA_VERSION_2))
				sys_config.protocol =
					PROTOCOL_WPA | PROTOCOL_WPA2;
			else if (params->crypto.
				 wpa_versions & NL80211_WPA_VERSION_2)
				sys_config.protocol = PROTOCOL_WPA2;
			else if (params->crypto.
				 wpa_versions & NL80211_WPA_VERSION_1)
				sys_config.protocol = PROTOCOL_WPA;
			break;
		}
	}
	sys_config.wpa_cfg.pairwise_cipher_wpa = 0;
	sys_config.wpa_cfg.pairwise_cipher_wpa2 = 0;
	for (i = 0; i < params->crypto.n_ciphers_pairwise; i++) {
		switch (params->crypto.ciphers_pairwise[i]) {
		case WLAN_CIPHER_SUITE_WEP40:
		case WLAN_CIPHER_SUITE_WEP104:
			break;
		case WLAN_CIPHER_SUITE_TKIP:
			if (params->crypto.wpa_versions & NL80211_WPA_VERSION_1)
				sys_config.wpa_cfg.pairwise_cipher_wpa |=
					CIPHER_TKIP;
			if (params->crypto.wpa_versions & NL80211_WPA_VERSION_2)
				sys_config.wpa_cfg.pairwise_cipher_wpa2 |=
					CIPHER_TKIP;
			break;
		case WLAN_CIPHER_SUITE_CCMP:
			if (params->crypto.wpa_versions & NL80211_WPA_VERSION_1)
				sys_config.wpa_cfg.pairwise_cipher_wpa |=
					CIPHER_AES_CCMP;
			if (params->crypto.wpa_versions & NL80211_WPA_VERSION_2)
				sys_config.wpa_cfg.pairwise_cipher_wpa2 |=
					CIPHER_AES_CCMP;
			break;
		}
	}
	switch (params->crypto.cipher_group) {
	case WLAN_CIPHER_SUITE_WEP40:
	case WLAN_CIPHER_SUITE_WEP104:
		if ((priv->cipher == WLAN_CIPHER_SUITE_WEP40) ||
		    (priv->cipher == WLAN_CIPHER_SUITE_WEP104)) {
			sys_config.protocol = PROTOCOL_STATIC_WEP;
			sys_config.key_mgmt = KEY_MGMT_NONE;
			sys_config.wpa_cfg.length = 0;
			memcpy(&sys_config.wep_cfg.key0, &priv->uap_wep_key[0],
			       sizeof(wep_key));
			memcpy(&sys_config.wep_cfg.key1, &priv->uap_wep_key[1],
			       sizeof(wep_key));
			memcpy(&sys_config.wep_cfg.key2, &priv->uap_wep_key[2],
			       sizeof(wep_key));
			memcpy(&sys_config.wep_cfg.key3, &priv->uap_wep_key[3],
			       sizeof(wep_key));
		}
		break;
	case WLAN_CIPHER_SUITE_TKIP:
		sys_config.wpa_cfg.group_cipher = CIPHER_TKIP;
		break;
	case WLAN_CIPHER_SUITE_CCMP:
		sys_config.wpa_cfg.group_cipher = CIPHER_AES_CCMP;
		break;
	}
#else
	/* Since in Android ICS 4.0.1's wpa_supplicant, there is no way to set
	   ssid when GO (AP) starts up, so get it from beacon head parameter
	   TODO: right now use hard code 24 -- ieee80211 header lenth, 12 --
	   fixed element length for beacon */
#define BEACON_IE_OFFSET	36
	/* Find SSID in head SSID IE id: 0, right now use hard code */
	ssid_ie = woal_parse_ie_tlv(params->head + BEACON_IE_OFFSET,
				    params->head_len - BEACON_IE_OFFSET, 0);

	if (!ssid_ie) {
		PRINTM(MERROR, "No ssid IE found.\n");
		ret = -EFAULT;
		goto done;
	}
	if (*(ssid_ie + 1) > 32) {
		PRINTM(MERROR, "ssid len error: %d\n", *(ssid_ie + 1));
		ret = -EFAULT;
		goto done;
	}
	memcpy(sys_config.ssid.ssid, ssid_ie + 2, *(ssid_ie + 1));
	sys_config.ssid.ssid_len = *(ssid_ie + 1);
	head = (struct ieee80211_mgmt *)params->head;

	capab_info = le16_to_cpu(head->u.beacon.capab_info);
	PRINTM(MIOCTL, "capab_info=0x%x\n", head->u.beacon.capab_info);
	sys_config.auth_mode = MLAN_AUTH_MODE_OPEN;
	/** For ICS, we don't support OPEN mode */
	if ((priv->cipher == WLAN_CIPHER_SUITE_WEP40) ||
	    (priv->cipher == WLAN_CIPHER_SUITE_WEP104)) {
		sys_config.protocol = PROTOCOL_STATIC_WEP;
		sys_config.key_mgmt = KEY_MGMT_NONE;
		sys_config.wpa_cfg.length = 0;
		memcpy(&sys_config.wep_cfg.key0, &priv->uap_wep_key[0],
		       sizeof(wep_key));
		memcpy(&sys_config.wep_cfg.key1, &priv->uap_wep_key[1],
		       sizeof(wep_key));
		memcpy(&sys_config.wep_cfg.key2, &priv->uap_wep_key[2],
		       sizeof(wep_key));
		memcpy(&sys_config.wep_cfg.key3, &priv->uap_wep_key[3],
		       sizeof(wep_key));
	} else {
		/** Get cipher and key_mgmt from RSN/WPA IE */
		if (capab_info & WLAN_CAPABILITY_PRIVACY) {
			wpa_ies =
				woal_find_wpa_ies(params->tail,
						  params->tail_len,
						  &sys_config);
			if (wpa_ies == MFALSE) {
				/* hard code setting to wpa2-psk */
				sys_config.protocol = PROTOCOL_WPA2;
				sys_config.key_mgmt = KEY_MGMT_PSK;
				sys_config.wpa_cfg.pairwise_cipher_wpa2 =
					CIPHER_AES_CCMP;
				sys_config.wpa_cfg.group_cipher =
					CIPHER_AES_CCMP;
			}
		}
	}
#endif /* COMPAT_WIRELESS */

	if (priv->bss_type == MLAN_BSS_TYPE_UAP) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
		/* find and set wmm ie */
		woal_set_wmm_ies(params->beacon.tail,
				 (int)params->beacon.tail_len, &sys_config);
#else
		woal_set_wmm_ies(params->tail, params->tail_len, &sys_config);
#endif
	}
	/* If the security mode is configured as WEP or WPA-PSK, it will
	   disable 11n automatically, and if configured as open(off) or
	   wpa2-psk, it will automatically enable 11n */
	if ((sys_config.protocol == PROTOCOL_STATIC_WEP) ||
	    (sys_config.protocol == PROTOCOL_WPA))
		enable_11n = MFALSE;
	if (!enable_11n) {
		woal_set_uap_ht_tx_cfg(priv, sys_config.band_cfg, MFALSE);
		woal_uap_set_11n_status(&sys_config, MLAN_ACT_DISABLE);
	} else {
		woal_set_uap_ht_tx_cfg(priv, sys_config.band_cfg, MTRUE);
		woal_uap_set_11n_status(&sys_config, MLAN_ACT_ENABLE);
		woal_set_get_tx_bf_cap(priv, MLAN_ACT_GET,
				       &sys_config.tx_bf_cap);
	}
	if (sys_config.band_cfg & BAND_CONFIG_5G) {
		if (enable_11ac && enable_11n)
			woal_uap_set_11ac_status(priv, MLAN_ACT_ENABLE,
						 vht20_40);
		else
			woal_uap_set_11ac_status(priv, MLAN_ACT_DISABLE,
						 vht20_40);
	}
	if (MLAN_STATUS_SUCCESS != woal_set_get_sys_config(priv,
							   MLAN_ACT_SET,
							   MOAL_IOCTL_WAIT,
							   &sys_config)) {
		ret = -EFAULT;
		goto done;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
	woal_enable_dfs_support(priv, &params->chandef);
#endif
done:
	LEAVE();
	return ret;
}

static int
woal_mon_open(struct net_device *ndev)
{
	ENTER();
	LEAVE();
	return 0;
}

static int
woal_mon_close(struct net_device *ndev)
{
	ENTER();
	LEAVE();
	return 0;
}

static int
woal_mon_set_mac_address(struct net_device *ndev, void *addr)
{
	ENTER();
	LEAVE();
	return 0;
}

static void
woal_mon_set_multicast_list(struct net_device *ndev)
{
	ENTER();
	LEAVE();
}

static int
woal_mon_hard_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	int len_rthdr;
	int qos_len = 0;
	int dot11_hdr_len = 24;
	int snap_len = 6;
	unsigned char *pdata;
	unsigned short fc;
	unsigned char src_mac_addr[6];
	unsigned char dst_mac_addr[6];
	struct ieee80211_hdr *dot11_hdr;
	struct ieee80211_radiotap_header *prthdr =
		(struct ieee80211_radiotap_header *)skb->data;
	monitor_iface *mon_if = netdev_priv(ndev);

	ENTER();

	if (mon_if == NULL || mon_if->base_ndev == NULL) {
		goto fail;
	}

	/* check for not even having the fixed radiotap header part */
	if (unlikely(skb->len < sizeof(struct ieee80211_radiotap_header))) {
		PRINTM(MERROR, "Invalid radiotap hdr length,"
		       "skb->len: %d\n", skb->len);
		goto fail;	/* too short to be possibly valid */
	}

	/* is it a header version we can trust to find length from? */
	if (unlikely(prthdr->it_version))
		goto fail;	/* only version 0 is supported */

	/* then there must be a radiotap header with a length we can use */
	len_rthdr = ieee80211_get_radiotap_len(skb->data);

	/* does the skb contain enough to deliver on the alleged length? */
	if (unlikely(skb->len < len_rthdr)) {
		PRINTM(MERROR, "Invalid data length,"
		       "skb->len: %d\n", skb->len);
		goto fail;	/* skb too short for claimed rt header extent */
	}

	/* Skip the ratiotap header */
	skb_pull(skb, len_rthdr);

	dot11_hdr = (struct ieee80211_hdr *)skb->data;
	fc = le16_to_cpu(dot11_hdr->frame_control);
	if ((fc & IEEE80211_FCTL_FTYPE) == IEEE80211_FTYPE_DATA) {
		/* Check if this ia a Wireless Distribution System (WDS) frame
		   which has 4 MAC addresses */
		if (dot11_hdr->frame_control & 0x0080)
			qos_len = 2;
		if ((dot11_hdr->frame_control & 0x0300) == 0x0300)
			dot11_hdr_len += 6;

		memcpy(dst_mac_addr, dot11_hdr->addr1, sizeof(dst_mac_addr));
		memcpy(src_mac_addr, dot11_hdr->addr2, sizeof(src_mac_addr));

		/* Skip the 802.11 header, QoS (if any) and SNAP, but leave
		   spaces for for two MAC addresses */
		skb_pull(skb,
			 dot11_hdr_len + qos_len + snap_len -
			 sizeof(src_mac_addr) * 2);
		pdata = (unsigned char *)skb->data;
		memcpy(pdata, dst_mac_addr, sizeof(dst_mac_addr));
		memcpy(pdata + sizeof(dst_mac_addr), src_mac_addr,
		       sizeof(src_mac_addr));

		LEAVE();
		return woal_hard_start_xmit(skb, mon_if->base_ndev);
	}

fail:
	dev_kfree_skb(skb);
	LEAVE();
	return NETDEV_TX_OK;
}

/**
 *  @brief This function returns the network statistics
 *
 *  @param dev     A pointer to net_device structure
 *
 *  @return        A pointer to net_device_stats structure
 */
struct net_device_stats *
woal_mon_get_stats(struct net_device *dev)
{
	monitor_iface *mon_if = (monitor_iface *)netdev_priv(dev);
	return &mon_if->stats;
}

static const struct net_device_ops woal_cfg80211_mon_if_ops = {
	.ndo_open = woal_mon_open,
	.ndo_start_xmit = woal_mon_hard_start_xmit,
	.ndo_stop = woal_mon_close,
	.ndo_get_stats = woal_mon_get_stats,
	.ndo_set_mac_address = woal_mon_set_mac_address,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
	.ndo_set_rx_mode = woal_mon_set_multicast_list,
#else
	.ndo_set_multicast_list = woal_mon_set_multicast_list,
#endif
};

static void
woal_mon_if_setup(struct net_device *dev)
{
	ENTER();
	ether_setup(dev);
	dev->netdev_ops = &woal_cfg80211_mon_if_ops;
	dev->destructor = free_netdev;
	LEAVE();
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
/**
 * @brief Request the driver to add a monitor interface
 *
 * @param wiphy             A pointer to wiphy structure
 * @param name              Virtual interface name
 * @param name_assign_type  Interface name assignment type
 * @param flags             Flags for the virtual interface
 * @param params            A pointer to vif_params structure
 * @param new_dev           Netdevice to be passed out
 *
 * @return                  0 -- success, otherwise fail
 */
static int
woal_cfg80211_add_mon_if(struct wiphy *wiphy,
			 const char *name,
			 unsigned char name_assign_type,
			 u32 *flags, struct vif_params *params,
			 struct net_device **new_dev)
#else
/**
 * @brief Request the driver to add a monitor interface
 *
 * @param wiphy           A pointer to wiphy structure
 * @param name            Virtual interface name
 * @param flags           Flags for the virtual interface
 * @param params          A pointer to vif_params structure
 * @param new_dev         Netdevice to be passed out
 *
 * @return                0 -- success, otherwise fail
 */
static int
woal_cfg80211_add_mon_if(struct wiphy *wiphy,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
			 const
#endif
			 char *name, u32 *flags, struct vif_params *params,
			 struct net_device **new_dev)
#endif
{
	int ret;
	struct net_device *ndev = NULL;
	monitor_iface *mon_if = NULL;
	moal_private *priv = NULL;
	moal_handle *handle = (moal_handle *)woal_get_wiphy_priv(wiphy);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
	chan_band_info chan_info;
#endif

	ENTER();

	ASSERT_RTNL();

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
	if (woal_is_any_interface_active(handle)) {
		PRINTM(MERROR,
		       "Cannot start net monitor when Interface Active\n");
		ret = -EFAULT;
		goto fail;
	}
#endif

	if (handle->mon_if) {
		PRINTM(MERROR, "%s: monitor interface exist: %s basedev %s\n",
		       __func__, handle->mon_if->mon_ndev->name,
		       handle->mon_if->base_ndev->name);
		ret = -EFAULT;
		goto fail;
	}

	priv = (moal_private *)woal_get_priv(handle, MLAN_BSS_ROLE_STA);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 29)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
	ndev = alloc_netdev_mq(sizeof(*mon_if), name, name_assign_type,
			       woal_mon_if_setup, 1);
#else
	ndev = alloc_netdev_mq(sizeof(*mon_if), name, NET_NAME_UNKNOWN,
			       woal_mon_if_setup, 1);
#endif
#else
	ndev = alloc_netdev_mq(sizeof(*mon_if), name, woal_mon_if_setup, 1);
#endif
#else
	ndev = alloc_netdev_mq(sizeof(*mon_if), name, woal_mon_if_setup);
#endif
	if (!ndev) {
		PRINTM(MFATAL, "Init virtual ethernet device failed\n");
		ret = -EFAULT;
		goto fail;
	}

	dev_net_set(ndev, wiphy_net(wiphy));

	ret = dev_alloc_name(ndev, ndev->name);
	if (ret < 0) {
		PRINTM(MFATAL, "Net device alloc name fail.\n");
		goto fail;
	}

	memcpy(ndev->perm_addr, wiphy->perm_addr, ETH_ALEN);
	memcpy(ndev->dev_addr, ndev->perm_addr, ETH_ALEN);
	SET_NETDEV_DEV(ndev, wiphy_dev(wiphy));

	mon_if = netdev_priv(ndev);
	ndev->ieee80211_ptr = &mon_if->wdev;
	mon_if->wdev.iftype = NL80211_IFTYPE_MONITOR;
	mon_if->wdev.wiphy = wiphy;
	memcpy(mon_if->ifname, ndev->name, IFNAMSIZ);

	ndev->type = ARPHRD_IEEE80211_RADIOTAP;
	ndev->netdev_ops = &woal_cfg80211_mon_if_ops;

	mon_if->priv = priv;
	mon_if->mon_ndev = ndev;
	mon_if->base_ndev = priv->netdev;
	mon_if->radiotap_enabled = 1;
	mon_if->flag = 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
	/* Set default band channel config */
	mon_if->band_chan_cfg.band = BAND_B | BAND_G;
	mon_if->band_chan_cfg.band |= BAND_GN;
	mon_if->band_chan_cfg.channel = 1;
	mon_if->band_chan_cfg.chan_bandwidth = CHANNEL_BW_20MHZ;
	memset(&chan_info, 0x00, sizeof(chan_info));
	chan_info.channel = 1;
	chan_info.is_11n_enabled = MTRUE;
	if (MLAN_STATUS_FAILURE ==
	    woal_chandef_create(priv, &mon_if->chandef, &chan_info)) {
		ret = -EFAULT;
		goto fail;
	}
	if (MLAN_STATUS_SUCCESS != woal_set_net_monitor(priv, MOAL_IOCTL_WAIT,
							MTRUE, 0x7,
							&mon_if->
							band_chan_cfg)) {
		PRINTM(MERROR, "%s: woal_set_net_monitor fail\n", __func__);
		ret = -EFAULT;
		goto fail;
	}
#endif

	ret = register_netdevice(ndev);
	if (ret) {
		PRINTM(MFATAL, "register net_device failed, ret=%d\n", ret);
		goto fail;
	}

	handle->mon_if = mon_if;

	if (new_dev)
		*new_dev = ndev;

fail:
	if (ret && ndev)
		free_netdev(ndev);
	LEAVE();
	return ret;
}

#if defined(WIFI_DIRECT_SUPPORT)
#if LINUX_VERSION_CODE >= WIFI_DIRECT_KERNEL_VERSION
/**
 * @brief Callback function for virtual interface
 *      setup
 *
 *  @param dev    A pointer to structure net_device
 *
 *  @return       N/A
 */
static void
woal_virt_if_setup(struct net_device *dev)
{
	ENTER();
	ether_setup(dev);
	dev->destructor = free_netdev;
	LEAVE();
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
/**
 * @brief This function adds a new interface. It will
 *        allocate, initialize and register the device.
 *
 *  @param handle           A pointer to moal_handle structure
 *  @param bss_index        BSS index number
 *  @param name_assign_type Interface name assignment type
 *  @param bss_type         BSS type
 *
 *  @return                 A pointer to the new priv structure
 */
moal_private *
woal_alloc_virt_interface(moal_handle *handle, t_u8 bss_index,
			  unsigned char name_assign_type,
			  t_u8 bss_type, const char *name)
#else
/**
 * @brief This function adds a new interface. It will
 *        allocate, initialize and register the device.
 *
 *  @param handle    A pointer to moal_handle structure
 *  @param bss_index BSS index number
 *  @param bss_type  BSS type
 *
 *  @return          A pointer to the new priv structure
 */
moal_private *
woal_alloc_virt_interface(moal_handle *handle, t_u8 bss_index, t_u8 bss_type,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
			  const
#endif
			  char *name)
#endif
{
	struct net_device *dev = NULL;
	moal_private *priv = NULL;
	ENTER();

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 29)
#ifndef MAX_WMM_QUEUE
#define MAX_WMM_QUEUE   4
#endif
	/* Allocate an Ethernet device */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
	dev = alloc_netdev_mq(sizeof(moal_private), name, name_assign_type,
			      woal_virt_if_setup, MAX_WMM_QUEUE);
#else
	dev = alloc_netdev_mq(sizeof(moal_private), name, NET_NAME_UNKNOWN,
			      woal_virt_if_setup, MAX_WMM_QUEUE);
#endif
#else
	dev = alloc_netdev_mq(sizeof(moal_private), name, woal_virt_if_setup,
			      MAX_WMM_QUEUE);
#endif
#else
	dev = alloc_netdev(sizeof(moal_private), name, woal_virt_if_setup);
#endif
	if (!dev) {
		PRINTM(MFATAL, "Init virtual ethernet device failed\n");
		goto error;
	}
	/* Allocate device name */
	if ((dev_alloc_name(dev, name) < 0)) {
		PRINTM(MERROR, "Could not allocate device name\n");
		goto error;
	}

	priv = (moal_private *)netdev_priv(dev);
	/* Save the priv to handle */
	handle->priv[bss_index] = priv;

	/* Use the same handle structure */
	priv->phandle = handle;
	priv->netdev = dev;
	priv->bss_index = bss_index;
	priv->bss_type = bss_type;
	priv->bss_role = MLAN_BSS_ROLE_STA;

	INIT_LIST_HEAD(&priv->tcp_sess_queue);
	spin_lock_init(&priv->tcp_sess_lock);

	INIT_LIST_HEAD(&priv->tx_stat_queue);
	spin_lock_init(&priv->tx_stat_lock);
	spin_lock_init(&priv->connect_lock);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	SET_MODULE_OWNER(dev);
#endif

	PRINTM(MCMND, "Alloc virtual interface%s\n", dev->name);

	LEAVE();
	return priv;
error:
	if (dev)
		free_netdev(dev);
	LEAVE();
	return NULL;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
/**
 * @brief Request the driver to add a virtual interface
 *
 * @param wiphy             A pointer to wiphy structure
 * @param name              Virtual interface name
 * @param name_assign_type  Interface name assignment type
 * @param type              Virtual interface type
 * @param flags             Flags for the virtual interface
 * @param params            A pointer to vif_params structure
 * @param new_dev		    new net_device to return
 *
 * @return                  0 -- success, otherwise fail
 */
int
woal_cfg80211_add_virt_if(struct wiphy *wiphy,
			  const char *name,
			  unsigned char name_assign_type,
			  enum nl80211_iftype type, u32 *flags,
			  struct vif_params *params,
			  struct net_device **new_dev)
#else
/**
 * @brief Request the driver to add a virtual interface
 *
 * @param wiphy           A pointer to wiphy structure
 * @param name            Virtual interface name
 * @param type            Virtual interface type
 * @param flags           Flags for the virtual interface
 * @param params          A pointer to vif_params structure
 * @param new_dev		  new net_device to return
 *
 * @return                0 -- success, otherwise fail
 */
int
woal_cfg80211_add_virt_if(struct wiphy *wiphy,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
			  const
#endif
			  char *name, enum nl80211_iftype type, u32 *flags,
			  struct vif_params *params,
			  struct net_device **new_dev)
#endif
{
	int ret = 0;
	struct net_device *ndev = NULL;
	moal_private *priv = NULL, *new_priv = NULL;
	moal_handle *handle = (moal_handle *)woal_get_wiphy_priv(wiphy);
	struct wireless_dev *wdev = NULL;
	moal_private *vir_priv;
	int i = 0;

	ENTER();
	ASSERT_RTNL();
	priv = (moal_private *)woal_get_priv_bss_type(handle,
						      MLAN_BSS_TYPE_WIFIDIRECT);
	if (!priv || !priv->phandle) {
		PRINTM(MERROR, "priv or handle is NULL\n");
		LEAVE();
		return -EFAULT;
	}
	if (priv->phandle->drv_mode.intf_num == priv->phandle->priv_num) {
		PRINTM(MERROR, "max virtual interface limit reached\n");
		for (i = 0; i < priv->phandle->priv_num; i++) {
			vir_priv = priv->phandle->priv[i];
			if (vir_priv->bss_virtual) {
				woal_cfg80211_del_virt_if(wiphy,
							  vir_priv->netdev);
				break;
			}
		}
		if (priv->phandle->drv_mode.intf_num == priv->phandle->priv_num) {
			LEAVE();
			return -ENOMEM;
		}
	}
	PRINTM(MMSG, "Add virtual interface %s\n", name);
	if ((type != NL80211_IFTYPE_P2P_CLIENT) &&
	    (type != NL80211_IFTYPE_P2P_GO)) {
		PRINTM(MERROR, "Invalid iftype: %d\n", type);
		LEAVE();
		return -EINVAL;
	}

	handle = priv->phandle;
	/* Cancel previous scan req */
	woal_cancel_scan(priv, MOAL_IOCTL_WAIT);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
	new_priv =
		woal_alloc_virt_interface(handle, handle->priv_num,
					  name_assign_type,
					  MLAN_BSS_TYPE_WIFIDIRECT, name);
#else
	new_priv =
		woal_alloc_virt_interface(handle, handle->priv_num,
					  MLAN_BSS_TYPE_WIFIDIRECT, name);
#endif
	if (!new_priv) {
		PRINTM(MERROR, "Add virtual interface fail.");
		LEAVE();
		return -EFAULT;
	}
	handle->priv_num++;

	wdev = (struct wireless_dev *)&new_priv->w_dev;
	memset(wdev, 0, sizeof(struct wireless_dev));
	ndev = new_priv->netdev;
	SET_NETDEV_DEV(ndev, wiphy_dev(wiphy));
	ndev->ieee80211_ptr = wdev;
	wdev->iftype = type;
	wdev->wiphy = wiphy;
	new_priv->wdev = wdev;
	new_priv->bss_virtual = MTRUE;
	new_priv->pa_netdev = priv->netdev;

	woal_init_sta_dev(ndev, new_priv);

	/* Initialize priv structure */
	woal_init_priv(new_priv, MOAL_CMD_WAIT);
    /** Init to GO/CLIENT mode */
	if (type == NL80211_IFTYPE_P2P_CLIENT)
		woal_cfg80211_init_p2p_client(new_priv);
	else if (type == NL80211_IFTYPE_P2P_GO)
		woal_cfg80211_init_p2p_go(new_priv);
	ret = register_netdevice(ndev);
	if (ret) {
		handle->priv[new_priv->bss_index] = NULL;
		handle->priv_num--;
		if (ndev->reg_state == NETREG_REGISTERED) {
			unregister_netdevice(ndev);
			free_netdev(ndev);
			ndev = NULL;
		}
		PRINTM(MFATAL, "register net_device failed, ret=%d\n", ret);
		goto done;
	}
	netif_carrier_off(ndev);
	woal_stop_queue(ndev);
	if (new_dev)
		*new_dev = ndev;
#ifdef CONFIG_PROC_FS
	woal_create_proc_entry(new_priv);
#ifdef PROC_DEBUG
	woal_debug_entry(new_priv);
#endif /* PROC_DEBUG */
#endif /* CONFIG_PROC_FS */
done:
	LEAVE();
	return ret;
}

/**
 *  @brief Notify mlan BSS will be removed.
 *
 *  @param priv          A pointer to moal_private structure
 *
 *  @return              MLAN_STATUS_SUCCESS/MLAN_STATUS_PENDING -- success, otherwise fail
 */
mlan_status
woal_bss_remove(moal_private *priv)
{
	mlan_ioctl_req *req = NULL;
	mlan_ds_bss *bss = NULL;
	mlan_status status;

	ENTER();

	/* Allocate an IOCTL request buffer */
	req = (mlan_ioctl_req *)woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
	if (req == NULL) {
		status = MLAN_STATUS_FAILURE;
		goto done;
	}

	/* Fill request buffer */
	bss = (mlan_ds_bss *)req->pbuf;
	bss->sub_command = MLAN_OID_BSS_REMOVE;
	req->req_id = MLAN_IOCTL_BSS;
	req->action = MLAN_ACT_SET;
	/* Send IOCTL request to MLAN */
	status = woal_request_ioctl(priv, req, MOAL_CMD_WAIT);

done:
	if (status != MLAN_STATUS_PENDING)
		kfree(req);
	LEAVE();
	return status;
}

/**
 *  @brief This function removes an virtual interface.
 *
 *  @param wiphy    A pointer to the wiphy structure
 *  @param dev      A pointer to the net_device structure
 *
 *  @return         0 -- success, otherwise fail
 */
int
woal_cfg80211_del_virt_if(struct wiphy *wiphy, struct net_device *dev)
{
	int ret = 0;
	int i = 0;
	moal_private *priv = NULL;
	moal_private *vir_priv = NULL;
	moal_private *remain_priv = NULL;
	moal_handle *handle = (moal_handle *)woal_get_wiphy_priv(wiphy);

	for (i = 0; i < handle->priv_num; i++) {
		vir_priv = handle->priv[i];
		if (vir_priv) {
			if (vir_priv->netdev == dev) {
				PRINTM(MMSG,
				       "Del virtual interface %s, index=%d\n",
				       dev->name, i);
				break;
			}
		}
	}
	if (vir_priv && vir_priv->bss_type == MLAN_BSS_TYPE_UAP) {
		woal_cfg80211_del_beacon(wiphy, dev);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
		vir_priv->wdev->beacon_interval = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0)
		memset(&vir_priv->wdev->chandef, 0,
		       sizeof(vir_priv->wdev->chandef));
#endif
#endif
		vir_priv->wdev->ssid_len = 0;
		PRINTM(MMSG, "Skip del UAP virtual interface %s", dev->name);
		return ret;
	}

	priv = (moal_private *)woal_get_priv_bss_type(handle,
						      MLAN_BSS_TYPE_WIFIDIRECT);
	if (!priv)
		return ret;
	if (vir_priv && vir_priv->netdev == dev) {
		woal_stop_queue(dev);
		netif_carrier_off(dev);
		netif_device_detach(dev);
		if (handle->is_remain_timer_set) {
			woal_cancel_timer(&handle->remain_timer);
			woal_remain_timer_func(handle);
		}

	/*** cancel pending scan */
		woal_cancel_scan(vir_priv, MOAL_IOCTL_WAIT);

		woal_flush_tx_stat_queue(vir_priv);

		/* cancel previous remain on channel to avoid firmware hang */
		if (priv->phandle->remain_on_channel) {
			t_u8 channel_status;
			remain_priv =
				priv->phandle->priv[priv->phandle->
						    remain_bss_index];
			if (remain_priv) {
				if (woal_cfg80211_remain_on_channel_cfg
				    (remain_priv, MOAL_IOCTL_WAIT, MTRUE,
				     &channel_status, NULL, 0, 0))
					PRINTM(MERROR,
					       "del_virt_if: Fail to cancel remain on channel\n");

				if (priv->phandle->cookie) {
					cfg80211_remain_on_channel_expired(
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
										  remain_priv->
										  netdev,
#else
										  remain_priv->
										  wdev,
#endif
										  priv->
										  phandle->
										  cookie,
										  &priv->
										  phandle->
										  chan,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
										  priv->
										  phandle->
										  channel_type,
#endif
										  GFP_ATOMIC);
					priv->phandle->cookie = 0;
				}
				priv->phandle->remain_on_channel = MFALSE;
			}
		}

		woal_clear_all_mgmt_ies(vir_priv, MOAL_IOCTL_WAIT);
		woal_cfg80211_deinit_p2p(vir_priv);
		woal_bss_remove(vir_priv);
#ifdef CONFIG_PROC_FS
#ifdef PROC_DEBUG
		/* Remove proc debug */
		woal_debug_remove(vir_priv);
#endif /* PROC_DEBUG */
		woal_proc_remove(vir_priv);
#endif /* CONFIG_PROC_FS */
		/* Last reference is our one */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 37)
		PRINTM(MINFO, "refcnt = %d\n", atomic_read(&dev->refcnt));
#else
		PRINTM(MINFO, "refcnt = %d\n", netdev_refcnt_read(dev));
#endif
		PRINTM(MINFO, "netdev_finish_unregister: %s\n", dev->name);
		/* Clear the priv in handle */
		vir_priv->phandle->priv[vir_priv->bss_index] = NULL;
		priv->phandle->priv_num--;
		if (dev->reg_state == NETREG_REGISTERED)
			unregister_netdevice(dev);
	}
	return ret;
}

/**
 *  @brief This function removes an virtual interface.
 *
 *  @param handle    A pointer to the moal_handle structure
 *
 *  @return        N/A
 */
void
woal_remove_virtual_interface(moal_handle *handle)
{
	moal_private *priv = NULL;
	int vir_intf = 0;
	int i = 0;
	ENTER();
	rtnl_lock();
	for (i = 0; i < handle->priv_num; i++) {
		priv = handle->priv[i];
		if (priv) {
			if (priv->bss_virtual) {
				PRINTM(MCMND, "Remove virtual interface %s\n",
				       priv->netdev->name);
#ifdef CONFIG_PROC_FS
#ifdef PROC_DEBUG
				/* Remove proc debug */
				woal_debug_remove(priv);
#endif /* PROC_DEBUG */
				woal_proc_remove(priv);
#endif /* CONFIG_PROC_FS */
				netif_device_detach(priv->netdev);
				if (priv->netdev->reg_state ==
				    NETREG_REGISTERED)
					unregister_netdevice(priv->netdev);
				handle->priv[i] = NULL;
				vir_intf++;
			}
		}
	}
	if (handle->mon_if) {
		netif_device_detach(handle->mon_if->mon_ndev);
		if (handle->mon_if->mon_ndev->reg_state == NETREG_REGISTERED)
			unregister_netdevice(handle->mon_if->mon_ndev);
		handle->mon_if = NULL;
	}
	rtnl_unlock();
	handle->priv_num -= vir_intf;
	LEAVE();
}
#endif
#endif

/**
 *  @brief This function check if uap interface is ready
 *
 * @param wiphy           A pointer to wiphy structure
 * @param name            Virtual interface name
 *
 *  @return        MTRUE/MFALSE;
 */
static t_u8
woal_uap_interface_ready(struct wiphy *wiphy, char *name,
			 struct net_device **new_dev)
{
	moal_handle *handle = (moal_handle *)woal_get_wiphy_priv(wiphy);
	moal_private *priv = NULL;
	int i;

	for (i = 0; i < handle->priv_num; i++) {
		priv = handle->priv[i];
		if (priv && (priv->bss_type == MLAN_BSS_TYPE_UAP) &&
		    !strcmp(priv->netdev->name, name)) {
			priv->wdev->iftype = NL80211_IFTYPE_AP;
			*new_dev = priv->netdev;
			break;
		}
	}
	if (priv)
		return MTRUE;
	else
		return MFALSE;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 37) || defined(COMPAT_WIRELESS)
/**
 * @brief Request the driver to add a virtual interface
 *
 * @param wiphy           A pointer to wiphy structure
 * @param name            Virtual interface name
 * @param type            Virtual interface type
 * @param flags           Flags for the virtual interface
 * @param params          A pointer to vif_params structure
 *
 * @return                A pointer to net_device -- success, otherwise null
 */
struct net_device *
woal_cfg80211_add_virtual_intf(struct wiphy *wiphy,
			       char *name, enum nl80211_iftype type, u32 *flags,
			       struct vif_params *params)
#else
/**
 * @brief Request the driver to add a virtual interface
 *
 * @param wiphy           A pointer to wiphy structure
 * @param name            Virtual interface name
 * @param type            Virtual interface type
 * @param flags           Flags for the virtual interface
 * @param params          A pointer to vif_params structure
 *
 * @return                0 -- success, otherwise fail
 */
int
woal_cfg80211_add_virtual_intf(struct wiphy *wiphy,
			       char *name, enum nl80211_iftype type, u32 *flags,
			       struct vif_params *params)
#endif
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
/**
 * @brief Request the driver to add a virtual interface
 *
 * @param wiphy           A pointer to wiphy structure
 * @param name            Virtual interface name
 * @param type            Virtual interface type
 * @param flags           Flags for the virtual interface
 * @param params          A pointer to vif_params structure
 *
 * @return                A pointer to wireless_dev -- success, otherwise null
 */
struct wireless_dev *
woal_cfg80211_add_virtual_intf(struct wiphy *wiphy,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
			       const
#endif
			       char *name, enum nl80211_iftype type, u32 *flags,
			       struct vif_params *params)
#else
/**
 * @brief Request the driver to add a virtual interface
 *
 * @param wiphy             A pointer to wiphy structure
 * @param name              Virtual interface name
 * @param name_assign_type  Interface name assignment type
 * @param type              Virtual interface type
 * @param flags             Flags for the virtual interface
 * @param params            A pointer to vif_params structure
 *
 * @return                  A pointer to wireless_dev -- success, otherwise null
 */
struct wireless_dev *
woal_cfg80211_add_virtual_intf(struct wiphy *wiphy,
			       const char *name,
			       unsigned char name_assign_type,
			       enum nl80211_iftype type, u32 *flags,
			       struct vif_params *params)
#endif
#endif
{
	struct net_device *ndev = NULL;
	int ret = 0;

	ENTER();
	PRINTM(MIOCTL, "add virtual intf: %d name: %s\n", type, name);
	switch (type) {
	case NL80211_IFTYPE_MONITOR:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
		ret = woal_cfg80211_add_mon_if(wiphy, name, name_assign_type,
					       flags, params, &ndev);
#else
		ret = woal_cfg80211_add_mon_if(wiphy, name, flags, params,
					       &ndev);
#endif
		break;
#if defined(WIFI_DIRECT_SUPPORT)
#if LINUX_VERSION_CODE >= WIFI_DIRECT_KERNEL_VERSION
	case NL80211_IFTYPE_P2P_CLIENT:
	case NL80211_IFTYPE_P2P_GO:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
		ret = woal_cfg80211_add_virt_if(wiphy, name, name_assign_type,
						type, flags, params, &ndev);
#else
		ret = woal_cfg80211_add_virt_if(wiphy, name, type, flags,
						params, &ndev);
#endif
		break;
#endif
#endif
	case NL80211_IFTYPE_AP:
		if (!woal_uap_interface_ready(wiphy, (char *)name, &ndev)) {
			PRINTM(MMSG,
			       "Not support dynamically create %s UAP interface\n",
			       name);
			ret = -EFAULT;
		}
		break;
	default:
		PRINTM(MWARN, "Not supported if type: %d\n", type);
		ret = -EFAULT;
		break;
	}
	LEAVE();
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 37) || defined(COMPAT_WIRELESS)
	if (ret)
		return ERR_PTR(ret);
	else
		return ndev;
#else
	return ret;
#endif
#else
	if (ret)
		return ERR_PTR(ret);
	else
		return ndev->ieee80211_ptr;
#endif
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
/**
 * @brief Request the driver to del a virtual interface
 *
 * @param wiphy           A pointer to wiphy structure
 * @param dev             The pointer to net_device
 *
 * @return               0 -- success, otherwise fail
 */
int
woal_cfg80211_del_virtual_intf(struct wiphy *wiphy, struct net_device *dev)
#else
/**
 * @brief Request the driver to del a virtual interface
 *
 * @param wiphy           A pointer to wiphy structure
 * @param wdev            The pointer to wireless_dev
 *
 * @return               0 -- success, otherwise fail
 */
int
woal_cfg80211_del_virtual_intf(struct wiphy *wiphy, struct wireless_dev *wdev)
#endif
{
	int ret = 0;
	moal_handle *handle = (moal_handle *)woal_get_wiphy_priv(wiphy);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
	struct net_device *dev = wdev->netdev;
#endif
	ENTER();

	PRINTM(MIOCTL, "del virtual intf %s\n", dev->name);
	ASSERT_RTNL();
	if (dev->ieee80211_ptr->iftype == NL80211_IFTYPE_MONITOR) {
		if ((handle->mon_if) && (handle->mon_if->mon_ndev == dev)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
			if (MLAN_STATUS_SUCCESS !=
			    woal_set_net_monitor(handle->mon_if->priv,
						 MOAL_IOCTL_WAIT, MFALSE, 0,
						 NULL)) {
				PRINTM(MERROR,
				       "%s: woal_set_net_monitor fail\n",
				       __func__);
				ret = -EFAULT;
			}
#endif
			handle->mon_if = NULL;
		}
		unregister_netdevice(dev);
		LEAVE();
		return ret;
	}
#if defined(WIFI_DIRECT_SUPPORT)
#if LINUX_VERSION_CODE >= WIFI_DIRECT_KERNEL_VERSION
	ret = woal_cfg80211_del_virt_if(wiphy, dev);
#endif
#endif
	LEAVE();
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
/**
 * @brief initialize AP or GO parameters

 *
 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param params          A pointer to cfg80211_ap_settings structure
 * @return                0 -- success, otherwise fail
 */
int
woal_cfg80211_add_beacon(struct wiphy *wiphy,
			 struct net_device *dev,
			 struct cfg80211_ap_settings *params)
#else
/**
 * @brief initialize AP or GO parameters

 *
 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param params          A pointer to beacon_parameters structure
 * @return                0 -- success, otherwise fail
 */
int
woal_cfg80211_add_beacon(struct wiphy *wiphy,
			 struct net_device *dev,
			 struct beacon_parameters *params)
#endif
{
	moal_private *priv = (moal_private *)woal_get_netdev_priv(dev);
	int ret = 0;

	ENTER();

	PRINTM(MMSG, "wlan: Starting AP\n");
#ifdef STA_CFG80211
	/*** cancel pending scan */
	woal_cancel_scan(priv, MOAL_IOCTL_WAIT);
#endif
	if (!params) {
		LEAVE();
		return -EFAULT;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
	priv->channel =
		ieee80211_frequency_to_channel(params->chandef.chan->
					       center_freq);
#else
	priv->channel =
		ieee80211_frequency_to_channel(params->channel->center_freq);
#endif
#endif
	/* bss config */
	if (MLAN_STATUS_SUCCESS != woal_cfg80211_beacon_config(priv, params)) {
		ret = -EFAULT;
		goto done;
	}

	/* set mgmt frame ies */
	ret = woal_cfg80211_mgmt_frame_ie(priv,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0) && !defined(COMPAT_WIRELESS)
					  params->tail, params->tail_len, NULL,
					  0, NULL, 0, NULL, 0, MGMT_MASK_BEACON
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
					  params->beacon.tail,
					  params->beacon.tail_len,
					  params->beacon.proberesp_ies,
					  params->beacon.proberesp_ies_len,
					  params->beacon.assocresp_ies,
					  params->beacon.assocresp_ies_len,
#else
					  params->tail, params->tail_len,
					  params->proberesp_ies,
					  params->proberesp_ies_len,
					  params->assocresp_ies,
					  params->assocresp_ies_len,
#endif
					  NULL, 0,
					  MGMT_MASK_BEACON |
					  MGMT_MASK_PROBE_RESP |
					  MGMT_MASK_ASSOC_RESP
#endif
					  , MOAL_IOCTL_WAIT);
	if (ret)
		goto done;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0) || defined(COMPAT_WIRELESS)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	if (params->beacon.beacon_ies && params->beacon.beacon_ies_len) {
		ret = woal_cfg80211_mgmt_frame_ie(priv,
						  params->beacon.beacon_ies,
						  params->beacon.beacon_ies_len,
						  NULL, 0, NULL, 0, NULL, 0,
						  MGMT_MASK_BEACON_WPS_P2P,
						  MOAL_IOCTL_WAIT);
		if (ret) {
			PRINTM(MERROR, "Failed to set beacon wps/p2p ie\n");
			goto done;
		}
	}
#else
	if (params->beacon_ies && params->beacon_ies_len) {
		ret = woal_cfg80211_mgmt_frame_ie(priv,
						  params->beacon_ies,
						  params->beacon_ies_len, NULL,
						  0, NULL, 0, NULL, 0,
						  MGMT_MASK_BEACON_WPS_P2P,
						  MOAL_IOCTL_WAIT);
		if (ret) {
			PRINTM(MERROR, "Failed to set beacon wps/p2p ie\n");
			goto done;
		}
	}
#endif
#endif
	priv->uap_host_based = MTRUE;
	/* if the bss is stopped, then start it */
	if (priv->bss_started == MFALSE) {
		if (MLAN_STATUS_SUCCESS !=
		    woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_START)) {
			priv->uap_host_based = MFALSE;
			ret = -EFAULT;
			goto done;
		}
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
	memcpy(&priv->chan, &params->chandef, sizeof(struct cfg80211_chan_def));
#endif
	PRINTM(MMSG, "wlan: AP started\n");
done:
	LEAVE();
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
/**
 * @brief set AP or GO parameter
 *
 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param params          A pointer to cfg80211_beacon_data structure
 * @return                0 -- success, otherwise fail
 */
int
woal_cfg80211_set_beacon(struct wiphy *wiphy,
			 struct net_device *dev,
			 struct cfg80211_beacon_data *params)
#else
/**
 * @brief set AP or GO parameter
 *
 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param params          A pointer to beacon_parameters structure
 * @return                0 -- success, otherwise fail
 */
int
woal_cfg80211_set_beacon(struct wiphy *wiphy,
			 struct net_device *dev,
			 struct beacon_parameters *params)
#endif
{
	moal_private *priv = (moal_private *)woal_get_netdev_priv(dev);
	int ret = 0;

	ENTER();

	PRINTM(MIOCTL, "set beacon\n");
	if (params != NULL) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0) && !defined(COMPAT_WIRELESS)
		if (params->tail && params->tail_len) {
			ret = woal_cfg80211_mgmt_frame_ie(priv,
							  params->tail,
							  params->tail_len,
							  NULL, 0, NULL, 0,
							  NULL, 0,
							  MGMT_MASK_BEACON,
							  MOAL_IOCTL_WAIT);
			if (ret)
				goto done;
		}
#else
		t_u16 mask = 0;
		if (params->tail && params->tail_len)
			mask |= MGMT_MASK_BEACON;
		if (params->proberesp_ies && params->proberesp_ies_len)
			mask |= MGMT_MASK_PROBE_RESP;
		if (params->assocresp_ies && params->assocresp_ies_len)
			mask |= MGMT_MASK_ASSOC_RESP;
		PRINTM(MIOCTL, "Set beacon: mask=0x%x\n", mask);
		if (mask) {
			ret = woal_cfg80211_mgmt_frame_ie(priv, params->tail,
							  params->tail_len,
							  params->proberesp_ies,
							  params->
							  proberesp_ies_len,
							  params->assocresp_ies,
							  params->
							  assocresp_ies_len,
							  NULL, 0, mask,
							  MOAL_IOCTL_WAIT);
			if (ret)
				goto done;
		}
		if (params->beacon_ies && params->beacon_ies_len) {
			ret = woal_cfg80211_mgmt_frame_ie(priv,
							  params->beacon_ies,
							  params->
							  beacon_ies_len, NULL,
							  0, NULL, 0, NULL, 0,
							  MGMT_MASK_BEACON_WPS_P2P,
							  MOAL_IOCTL_WAIT);
			if (ret) {
				PRINTM(MERROR,
				       "Failed to set beacon wps/p2p ie\n");
				goto done;
			}
		}
#endif
	}

done:
	LEAVE();
	return ret;
}

/**
 * @brief reset AP or GO parameters
 *
 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 *
 * @return                0 -- success, otherwise fail
 */
int
woal_cfg80211_del_beacon(struct wiphy *wiphy, struct net_device *dev)
{
	moal_private *priv = (moal_private *)woal_get_netdev_priv(dev);
	int ret = 0;
#ifdef STA_SUPPORT
	moal_private *pmpriv = NULL;
#endif

	ENTER();

	priv->phandle->driver_state = woal_check_driver_status(priv->phandle);
	if (priv->phandle->driver_state) {
		PRINTM(MERROR,
		       "Block  woal_cfg80211_del_beacon in abnormal driver state\n");
		LEAVE();
		return 0;
	}
	if (priv->bss_started != MTRUE) {
		PRINTM(MMSG, "wlan: AP already stopped!");
		LEAVE();
		return 0;
	}
	priv->uap_host_based = MFALSE;
	PRINTM(MMSG, "wlan: Stoping AP\n");
#ifdef STA_SUPPORT
	woal_cancel_scan(priv, MOAL_IOCTL_WAIT);
#endif
	memset(priv->dscp_map, 0xFF, sizeof(priv->dscp_map));
	woal_deauth_all_station(priv);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
	memset(&priv->chan, 0, sizeof(struct cfg80211_chan_def));
	if (priv->phandle->is_cac_timer_set &&
	    priv->bss_index == priv->phandle->cac_bss_index) {
		woal_cancel_timer(&priv->phandle->cac_timer);
		priv->phandle->is_cac_timer_set = MFALSE;
		/* Make sure Chan Report is cancelled */
		woal_11h_cancel_chan_report_ioctl(priv, MOAL_IOCTL_WAIT);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
		cfg80211_cac_event(priv->netdev, &priv->phandle->dfs_channel,
				   NL80211_RADAR_CAC_ABORTED, GFP_KERNEL);
#else
		cfg80211_cac_event(priv->netdev, NL80211_RADAR_CAC_ABORTED,
				   GFP_KERNEL);
#endif
		memset(&priv->phandle->dfs_channel, 0,
		       sizeof(struct cfg80211_chan_def));
		priv->phandle->cac_bss_index = 0xff;
	}
	if (priv->csa_workqueue)
		flush_workqueue(priv->csa_workqueue);
#endif
	/* if the bss is still running, then stop it */
	if (priv->bss_started == MTRUE) {
		if (MLAN_STATUS_SUCCESS !=
		    woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_STOP)) {
			ret = -EFAULT;
			goto done;
		}
		if (MLAN_STATUS_SUCCESS !=
		    woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_RESET)) {
			ret = -EFAULT;
			goto done;
		}
		/* Set WLAN MAC addresses */
		if (MLAN_STATUS_SUCCESS != woal_request_set_mac_address(priv)) {
			PRINTM(MERROR, "Set MAC address failed\n");
			ret = -EFAULT;
			goto done;
		}
	}
	woal_clear_all_mgmt_ies(priv, MOAL_IOCTL_WAIT);
#ifdef STA_SUPPORT
	if (!woal_is_any_interface_active(priv->phandle)) {
		pmpriv = woal_get_priv((moal_handle *)priv->phandle,
				       MLAN_BSS_ROLE_STA);
		if (pmpriv)
			woal_set_scan_time(pmpriv, ACTIVE_SCAN_CHAN_TIME,
					   PASSIVE_SCAN_CHAN_TIME,
					   SPECIFIC_SCAN_CHAN_TIME);
	}
#endif

	priv->cipher = 0;
	memset(priv->uap_wep_key, 0, sizeof(priv->uap_wep_key));
	priv->channel = 0;
	PRINTM(MMSG, "wlan: AP stopped\n");
done:
	LEAVE();
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
/**
 * @brief change BSS
 *
 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param params          A pointer to bss_parameters structure
 *
 * @return                0 -- success, otherwise fail
 */
int
woal_cfg80211_change_bss(struct wiphy *wiphy, struct net_device *dev,
			 struct bss_parameters *params)
{
	int ret = 0;
	t_u8 change = MFALSE;
	mlan_uap_bss_param sys_config;
	u8 bss_started = MFALSE;
	t_u8 pkt_forward_ctl;
	moal_private *priv = (moal_private *)woal_get_netdev_priv(dev);

	ENTER();
	PRINTM(MIOCTL, "isolate=%d\n", params->ap_isolate);

	if (MLAN_STATUS_SUCCESS != woal_set_get_sys_config(priv,
							   MLAN_ACT_GET,
							   MOAL_IOCTL_WAIT,
							   &sys_config)) {
		PRINTM(MERROR, "Error getting AP confiruration\n");
		ret = -EFAULT;
		goto done;
	}

	pkt_forward_ctl = sys_config.pkt_forward_ctl;
	if (params->ap_isolate) {
	/** disable packet forwarding */
		sys_config.pkt_forward_ctl |= PKT_FWD_INTRA_BCAST;
		sys_config.pkt_forward_ctl |= PKT_FWD_INTRA_UCAST;
	} else {
		sys_config.pkt_forward_ctl &= ~PKT_FWD_INTRA_BCAST;
		sys_config.pkt_forward_ctl &= ~PKT_FWD_INTRA_UCAST;
	}
	if (pkt_forward_ctl != sys_config.pkt_forward_ctl) {
		change = MTRUE;
		PRINTM(MIOCTL, "ap_isolate=%xd\n", params->ap_isolate);
	}
	if (change) {
		if (priv->bss_started == MTRUE) {
			bss_started = MTRUE;
			woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_STOP);
		}
		if (MLAN_STATUS_SUCCESS == woal_set_get_sys_config(priv,
								   MLAN_ACT_SET,
								   MOAL_IOCTL_WAIT,
								   &sys_config))
			ret = 0;
		if (bss_started)
			woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_START);
	}
done:
	LEAVE();
	return ret;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
/**
 * @brief del station
 *
 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param param           A pointer tostation_del_parameters structure
 *
 * @return                0 -- success, otherwise fail
 */
#else
/**
 * @brief del station
 *
 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param mac_addr		  A pointer to station mac address
 *
 * @return                0 -- success, otherwise fail
 */
#endif
int
woal_cfg80211_del_station(struct wiphy *wiphy, struct net_device *dev,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
			  struct station_del_parameters *param)
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
			  const u8 *mac_addr)
#else
			  u8 *mac_addr)
#endif
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
	const u8 *mac_addr = NULL;
#endif
	u16 reason_code = REASON_CODE_DEAUTH_LEAVING;
	moal_private *priv = (moal_private *)woal_get_netdev_priv(dev);
	ENTER();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
	if (priv->phandle->is_cac_timer_set &&
	    priv->bss_index == priv->phandle->cac_bss_index) {
		woal_cancel_timer(&priv->phandle->cac_timer);
		priv->phandle->is_cac_timer_set = MFALSE;
		/* Make sure Chan Report is cancelled */
		woal_11h_cancel_chan_report_ioctl(priv, MOAL_IOCTL_WAIT);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
		cfg80211_cac_event(priv->netdev, &priv->phandle->dfs_channel,
				   NL80211_RADAR_CAC_ABORTED, GFP_KERNEL);
#else
		cfg80211_cac_event(priv->netdev, NL80211_RADAR_CAC_ABORTED,
				   GFP_KERNEL);
#endif
		memset(&priv->phandle->dfs_channel, 0,
		       sizeof(struct cfg80211_chan_def));
		priv->phandle->cac_bss_index = 0xff;
	}
#endif
	if (priv->media_connected == MFALSE) {
		PRINTM(MINFO, "cfg80211: Media not connected!\n");
		LEAVE();
		return 0;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
	if (param) {
		mac_addr = param->mac;
		reason_code = param->reason_code;
	}
#endif
    /** we will not send deauth to p2p interface, it might cause WPS failure */
	if (mac_addr) {
		PRINTM(MMSG, "wlan: deauth station " MACSTR "\n",
		       MAC2STR(mac_addr));
#ifdef WIFI_DIRECT_SUPPORT
		if (!priv->phandle->is_go_timer_set ||
		    priv->bss_type != MLAN_BSS_TYPE_WIFIDIRECT)
#endif
			woal_deauth_station(priv, (u8 *)mac_addr, reason_code);
	} else {
		PRINTM(MIOCTL, "del all station\n");
	}
	LEAVE();
	return 0;

}

/**
 * @brief Get station info
 *
 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param mac			  A pointer to station mac address
 * @param stainfo		  A pointer to station_info structure
 *
 * @return                0 -- success, otherwise fail
 */
int
woal_uap_cfg80211_get_station(struct wiphy *wiphy, struct net_device *dev,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
			      const u8 *mac,
#else
			      u8 *mac,
#endif
			      struct station_info *stainfo)
{
	moal_private *priv = (moal_private *)woal_get_netdev_priv(dev);
	int ret = -EFAULT;
	int i = 0;
	mlan_ds_get_info *info = NULL;
	mlan_ioctl_req *ioctl_req = NULL;
	mlan_status status = MLAN_STATUS_SUCCESS;

	ENTER();
	if (priv->media_connected == MFALSE) {
		PRINTM(MINFO, "cfg80211: Media not connected!\n");
		LEAVE();
		return -ENOENT;
	}

	/* Allocate an IOCTL request buffer */
	ioctl_req =
		(mlan_ioctl_req *)
		woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_get_info));
	if (ioctl_req == NULL) {
		ret = -ENOMEM;
		goto done;
	}

	info = (mlan_ds_get_info *)ioctl_req->pbuf;
	info->sub_command = MLAN_OID_UAP_STA_LIST;
	ioctl_req->req_id = MLAN_IOCTL_GET_INFO;
	ioctl_req->action = MLAN_ACT_GET;

	status = woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT);
	if (status != MLAN_STATUS_SUCCESS)
		goto done;
	for (i = 0; i < info->param.sta_list.sta_count; i++) {
		if (!memcmp
		    (info->param.sta_list.info[i].mac_address, mac, ETH_ALEN)) {
			PRINTM(MIOCTL, "Get station: " MACSTR " RSSI=%d\n",
			       MAC2STR(mac),
			       (int)info->param.sta_list.info[i].rssi);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
			stainfo->filled = BIT(NL80211_STA_INFO_INACTIVE_TIME) |
				BIT(NL80211_STA_INFO_SIGNAL);
#else
			stainfo->filled =
				STATION_INFO_INACTIVE_TIME |
				STATION_INFO_SIGNAL;
#endif
			stainfo->inactive_time = 0;
			stainfo->signal = info->param.sta_list.info[i].rssi;
			ret = 0;
			break;
		}
	}
done:
	if (status != MLAN_STATUS_PENDING)
		kfree(ioctl_req);
	LEAVE();
	return ret;
}

/**
 * @brief Request the driver to dump the station information
 *
 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param idx             Station index
 * @param mac             MAC address of the station
 * @param sinfo           A pointer to station_info structure
 *
 * @return                0 -- success, otherwise fail
 */
int
woal_uap_cfg80211_dump_station(struct wiphy *wiphy,
			       struct net_device *dev, int idx,
			       t_u8 *mac, struct station_info *sinfo)
{
	moal_private *priv = (moal_private *)woal_get_netdev_priv(dev);
	int ret = -EFAULT;
	mlan_ds_get_info *info = NULL;
	mlan_ioctl_req *ioctl_req = NULL;
	mlan_status status = MLAN_STATUS_SUCCESS;

	ENTER();
	if (priv->media_connected == MFALSE) {
		PRINTM(MINFO, "cfg80211: Media not connected!\n");
		LEAVE();
		return -ENOENT;
	}

	/* Allocate an IOCTL request buffer */
	ioctl_req =
		(mlan_ioctl_req *)
		woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_get_info));
	if (ioctl_req == NULL) {
		ret = -ENOMEM;
		goto done;
	}

	info = (mlan_ds_get_info *)ioctl_req->pbuf;
	info->sub_command = MLAN_OID_UAP_STA_LIST;
	ioctl_req->req_id = MLAN_IOCTL_GET_INFO;
	ioctl_req->action = MLAN_ACT_GET;

	status = woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT);
	if (status != MLAN_STATUS_SUCCESS)
		goto done;
	if (idx >= info->param.sta_list.sta_count) {
		ret = -EFAULT;
		goto done;
	}
	ret = 0;
	memcpy(mac, info->param.sta_list.info[idx].mac_address, ETH_ALEN);
	PRINTM(MIOCTL, "Dump station: " MACSTR " RSSI=%d\n", MAC2STR(mac),
	       (int)info->param.sta_list.info[idx].rssi);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
	sinfo->filled =
		BIT(NL80211_STA_INFO_INACTIVE_TIME) |
		BIT(NL80211_STA_INFO_SIGNAL);
#else
	sinfo->filled = STATION_INFO_INACTIVE_TIME | STATION_INFO_SIGNAL;
#endif
	sinfo->inactive_time = 0;
	sinfo->signal = info->param.sta_list.info[idx].rssi;
done:
	if (status != MLAN_STATUS_PENDING)
		kfree(ioctl_req);
	LEAVE();
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
/**
 * @brief set mac filter
 *
 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param params		  A pointer to cfg80211_acl_data structure
 *
 * @return                0 -- success, otherwise fail
 */
int
woal_cfg80211_set_mac_acl(struct wiphy *wiphy, struct net_device *dev,
			  const struct cfg80211_acl_data *params)
{
	int ret = -EFAULT;
	mlan_uap_bss_param sys_config;
	moal_private *priv = (moal_private *)woal_get_netdev_priv(dev);
	u8 bss_started = MFALSE;
	ENTER();

	PRINTM(MIOCTL, "Set mac acl, entries=%d, policy=%d\n",
	       params->n_acl_entries, params->acl_policy);
	/* Initialize the uap bss values which are uploaded from firmware */
	if (MLAN_STATUS_SUCCESS != woal_set_get_sys_config(priv,
							   MLAN_ACT_GET,
							   MOAL_IOCTL_WAIT,
							   &sys_config)) {
		PRINTM(MERROR, "Error getting AP confiruration\n");
		ret = -EFAULT;
		goto done;
	}
	memset(&sys_config.filter, 0, sizeof(mac_filter));
	if (params->n_acl_entries <= MAX_MAC_FILTER_NUM)
		sys_config.filter.mac_count = params->n_acl_entries;
	else
		sys_config.filter.mac_count = MAX_MAC_FILTER_NUM;

	if (params->acl_policy == NL80211_ACL_POLICY_DENY_UNLESS_LISTED)
		sys_config.filter.filter_mode = MAC_FILTER_MODE_ALLOW_MAC;
	else if (params->acl_policy == NL80211_ACL_POLICY_ACCEPT_UNLESS_LISTED)
		sys_config.filter.filter_mode = MAC_FILTER_MODE_BLOCK_MAC;
	memcpy(sys_config.filter.mac_list, params->mac_addrs,
	       sys_config.filter.mac_count * sizeof(mlan_802_11_mac_addr));
	if (priv->bss_started == MTRUE) {
		bss_started = MTRUE;
		woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_STOP);
	}
	if (MLAN_STATUS_SUCCESS == woal_set_get_sys_config(priv,
							   MLAN_ACT_SET,
							   MOAL_IOCTL_WAIT,
							   &sys_config))
		ret = 0;
done:
	if (bss_started)
		woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_START);
	LEAVE();
	return ret;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 1, 0)|| defined(COMPAT_WIRELESS)
/**
 * @brief Set txq parameters

 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param params		  A pointer to ieee80211_txq_params structure
 *
 * @return                0 -- success, otherwise fail
 */
int
woal_cfg80211_set_txq_params(struct wiphy *wiphy, struct net_device *dev,
			     struct ieee80211_txq_params *params)
{
	int ret = 0;
	u8 ac = 0;
	wmm_parameter_t ap_wmm_para;
	moal_private *priv = (moal_private *)woal_get_netdev_priv(dev);

	ENTER();

	/* AC_BE: 0, AC_BK:1, AC_VI: 2, AC_VO:3 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
	switch (params->ac) {
	case NL80211_AC_VO:
		ac = 3;
		break;
	case NL80211_AC_VI:
		ac = 2;
		break;
	case NL80211_AC_BK:
		ac = 1;
		break;
	case NL80211_AC_BE:
		ac = 0;
		break;
	default:
		break;
	}
#else
	switch (params->queue) {
	case NL80211_TXQ_Q_VO:
		ac = 3;
		break;
	case NL80211_TXQ_Q_VI:
		ac = 2;
		break;
	case NL80211_TXQ_Q_BK:
		ac = 1;
		break;
	case NL80211_TXQ_Q_BE:
		ac = 0;
		break;
	default:
		break;
	}
#endif

	PRINTM(MMSG, "Set AC=%d, txop=%d cwmin=%d, cwmax=%d aifs=%d\n", ac,
	       params->txop, params->cwmin, params->cwmax, params->aifs);

	memset(&ap_wmm_para, 0, sizeof(wmm_parameter_t));

	if (MLAN_STATUS_SUCCESS !=
	    woal_set_get_ap_wmm_para(priv, MLAN_ACT_GET, &ap_wmm_para)) {
		PRINTM(MERROR, "wlan: We don't support AP WMM parameter\n");
		LEAVE();
		return ret;
	}
	ap_wmm_para.ac_params[ac].aci_aifsn.aifsn = params->aifs;
	ap_wmm_para.ac_params[ac].ecw.ecw_max = ilog2(params->cwmax + 1);
	ap_wmm_para.ac_params[ac].ecw.ecw_min = ilog2(params->cwmin + 1);
	ap_wmm_para.ac_params[ac].tx_op_limit = params->txop;
	if (MLAN_STATUS_SUCCESS !=
	    woal_set_get_ap_wmm_para(priv, MLAN_ACT_SET, &ap_wmm_para)) {
		PRINTM(MERROR, "wlan: Fail to set AP WMM parameter\n");
		ret = -EFAULT;
	}
	LEAVE();
	return ret;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
/**
 * @brief cac timer call back function.
 *
 * @param context   a pointer to moal_handle
 *
 * @return           N/A
 */
void
woal_cac_timer_func(void *context)
{
	moal_handle *handle = (moal_handle *)context;
	moal_private *priv = handle->priv[handle->cac_bss_index];

	PRINTM(MEVENT, "cac_timer fired.\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
	cfg80211_cac_event(priv->netdev, &handle->dfs_channel,
			   NL80211_RADAR_CAC_ABORTED, GFP_KERNEL);
#else
	cfg80211_cac_event(priv->netdev, NL80211_RADAR_CAC_ABORTED, GFP_KERNEL);
#endif
	handle->is_cac_timer_set = MFALSE;
	memset(&handle->dfs_channel, 0, sizeof(struct cfg80211_chan_def));
	handle->cac_bss_index = 0xff;
}

/**
 * @brief This function switch AP's channel
 *        1. clear mgmt IEs  		2. stop uAP
 *   	  3. set beacon after		4. set new channel
 *   	  5. start uAP    		    6. notify cfg80211
 *
 * @param priv          a pointer to moal_private
 * @param wait_option   wait option
 *
 * @return           N/A
 */
void
woal_switch_uap_channel(moal_private *priv, t_u8 wait_option)
{

	mlan_chan_info uap_channel;
	t_u8 chan2Offset = 0;
	ENTER();
	woal_clear_all_mgmt_ies(priv, MOAL_CMD_WAIT);
	if (MLAN_STATUS_SUCCESS != woal_uap_bss_ctrl(priv,
						     wait_option,
						     UAP_BSS_STOP)) {
		PRINTM(MERROR, "%s: stop uap failed \n", __func__);
		goto done;
	}
	if (woal_cfg80211_set_beacon(priv->wdev->wiphy, priv->netdev,
				     &priv->beacon_after)) {
		PRINTM(MERROR, "%s: set mgmt ies failed \n", __func__);
		goto done;
	}

	uap_channel.channel =
		ieee80211_frequency_to_channel(priv->csa_chan.chan->
					       center_freq);
	switch (priv->csa_chan.width) {
	case NL80211_CHAN_WIDTH_5:
	case NL80211_CHAN_WIDTH_10:
	case NL80211_CHAN_WIDTH_20_NOHT:
		break;
	case NL80211_CHAN_WIDTH_20:
		break;
	case NL80211_CHAN_WIDTH_40:
		if (priv->csa_chan.center_freq1 <
		    priv->csa_chan.chan->center_freq)
			chan2Offset = SECOND_CHANNEL_BELOW;
		else
			chan2Offset = SECOND_CHANNEL_ABOVE;
		break;
	case NL80211_CHAN_WIDTH_80:
	case NL80211_CHAN_WIDTH_80P80:
	case NL80211_CHAN_WIDTH_160:
		chan2Offset =
			woal_get_second_channel_offset(uap_channel.channel);
		break;
	default:
		PRINTM(MWARN, "Unknown channel width: %d\n",
		       priv->csa_chan.width);
		break;
	}
	if (priv->csa_chan.chan->band == IEEE80211_BAND_2GHZ)
		uap_channel.band_cfg = BAND_CONFIG_2G;
	else if (priv->csa_chan.chan->band == IEEE80211_BAND_5GHZ)
		uap_channel.band_cfg = BAND_CONFIG_5G;
	uap_channel.band_cfg |= chan2Offset;
	if (MLAN_STATUS_SUCCESS !=
	    woal_set_get_ap_channel(priv, MLAN_ACT_SET, wait_option,
				    &uap_channel)) {
		PRINTM(MERROR, "Fail to set ap channel \n");
		goto done;
	}
	if (MLAN_STATUS_SUCCESS !=
	    woal_uap_bss_ctrl(priv, wait_option, UAP_BSS_START)) {
		PRINTM(MERROR, "%s: start uap failed \n", __func__);
		goto done;
	}
	PRINTM(MMSG, "CSA: old chan %d => new chan %d \n", priv->channel,
	       uap_channel.channel);
	priv->channel = uap_channel.channel;
	memcpy(&priv->chan, &priv->csa_chan, sizeof(struct cfg80211_chan_def));
	cfg80211_ch_switch_notify(priv->netdev, &priv->chan);
	if (priv->uap_tx_blocked) {
		if (!netif_carrier_ok(priv->netdev))
			netif_carrier_on(priv->netdev);
		woal_start_queue(priv->netdev);
		priv->uap_tx_blocked = MFALSE;
	}
done:
	LEAVE();
	return;
}

/**
 * @brief csa work handler
 *
 * @param work            a pointer to work_struct
 *
 * @return                0 -- success, otherwise fail
 */
void
woal_csa_work_queue(struct work_struct *work)
{
	struct delayed_work *delayed_work =
		container_of(work, struct delayed_work, work);
	moal_private *priv = container_of(delayed_work, moal_private, csa_work);
	ENTER();
	if (priv->bss_started == MTRUE)
		woal_switch_uap_channel(priv, MOAL_CMD_WAIT);
	LEAVE();
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0)
/**
 * @brief start radar detection
 *
 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param chandef         A pointer to cfg80211_chan_def structure
 * @param cac_time_ms     A cac dwell time
 * @return                0 -- success, otherwise fail
 */

int
woal_cfg80211_start_radar_detection(struct wiphy *wiphy,
				    struct net_device *dev,
				    struct cfg80211_chan_def *chandef,
				    u32 cac_time_ms)
#else
/**
 * @brief start radar detection
 *
 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param chandef         A pointer to cfg80211_chan_def structure
 * @return                0 -- success, otherwise fail
 */

int
woal_cfg80211_start_radar_detection(struct wiphy *wiphy,
				    struct net_device *dev,
				    struct cfg80211_chan_def *chandef)
#endif
{
	moal_private *priv = (moal_private *)woal_get_netdev_priv(dev);
	moal_handle *handle = priv->phandle;
	mlan_ioctl_req *req = NULL;
	mlan_ds_11h_chan_rep_req *pchan_rpt_req = NULL;
	mlan_ds_11h_cfg *p11h_cfg = NULL;
	int ret = 0;
	mlan_status status = MLAN_STATUS_SUCCESS;

	ENTER();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0)
	PRINTM(MIOCTL, "start Radar detect, chan %d , Bw %d , Time %d \n",
	       chandef->chan->hw_value, chandef->width, cac_time_ms);
#else
	PRINTM(MIOCTL, "start Radar detect, chan %d , Bw %d \n",
	       chandef->chan->hw_value, chandef->width);
#endif

	if (priv->bss_started == MTRUE) {
		PRINTM(MERROR, "recv CAC request when bss already started \n");
		ret = -EFAULT;
		goto done;
	}
	if (priv->phandle->cac_period || handle->is_cac_timer_set) {
		PRINTM(MERROR,
		       "Maybe other interface is doing CAC, please defer your oper\n");
		ret = -EBUSY;
		goto done;
	}
	req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_11h_cfg));
	if (NULL == req) {
		ret = -ENOMEM;
		goto done;
	}

	p11h_cfg = (mlan_ds_11h_cfg *)req->pbuf;
	pchan_rpt_req = &p11h_cfg->param.chan_rpt_req;
	pchan_rpt_req->startFreq = START_FREQ_11A_BAND;
	pchan_rpt_req->chanNum = (t_u8)chandef->chan->hw_value;
	pchan_rpt_req->chanWidth = woal_covert_chan_width(chandef->width);
	pchan_rpt_req->host_based = MTRUE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0)
	pchan_rpt_req->millisec_dwell_time = cac_time_ms,
#else
	pchan_rpt_req->millisec_dwell_time = IEEE80211_DFS_MIN_CAC_TIME_MS;

	if ((woal_is_etsi_country(priv->phandle->country_code) == MTRUE)) {
		if (chandef->chan->hw_value == 120 ||
		    chandef->chan->hw_value == 124 ||
		    chandef->chan->hw_value == 128) {
			pchan_rpt_req->millisec_dwell_time =
				IEEE80211_DFS_MIN_CAC_TIME_MS * 10;
		}
		if (chandef->chan->hw_value == 116 &&
		    ((chandef->width == NL80211_CHAN_WIDTH_40) ||
		     (chandef->width == NL80211_CHAN_WIDTH_80))) {
			pchan_rpt_req->millisec_dwell_time =
				IEEE80211_DFS_MIN_CAC_TIME_MS * 10;
		}
	}
#endif

	p11h_cfg->sub_command = MLAN_OID_11H_CHANNEL_CHECK;
	req->req_id = MLAN_IOCTL_11H_CFG;
	req->action = MLAN_ACT_SET;

	/* Send Channel Check command and wait until the report is ready */
	status = woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT);
	if (status != MLAN_STATUS_SUCCESS) {
		PRINTM(MERROR, "Fail to start radar detection\n");
		ret = -EFAULT;
	} else {
		memcpy(&handle->dfs_channel, chandef,
		       sizeof(struct cfg80211_chan_def));
		handle->cac_bss_index = priv->bss_index;
		handle->is_cac_timer_set = MTRUE;
		/* avoid EVENT_CHANNEL_RAPORT_READY missing, add 1s gap */
		woal_mod_timer(&handle->cac_timer,
			       pchan_rpt_req->millisec_dwell_time + 1000);
	}
done:
	if (status != MLAN_STATUS_PENDING)
		kfree(req);
	LEAVE();
	return ret;
}

/**
 * @brief channel switch

 * @param wiphy           A pointer to wiphy structure
 * @param dev             A pointer to net_device structure
 * @param params		  A pointer to cfg80211_csa_settings structure
 *
 * @return                0 -- success, otherwise fail
 */
int
woal_cfg80211_channel_switch(struct wiphy *wiphy,
			     struct net_device *dev,
			     struct cfg80211_csa_settings *params)
{
	int ret = 0;
	moal_private *priv = (moal_private *)woal_get_netdev_priv(dev);
	t_u32 chsw_msec;
	mlan_uap_bss_param bss_cfg;

	ENTER();

	if (!params) {
		ret = -EINVAL;
		goto done;
	}

	/* TODO: support this case in next version */
	if (params->radar_required) {
		PRINTM(MMSG,
		       " hostapd handle this case by disable and re-enable interface\n");
		ret = -ENOTSUPP;
		goto done;
	}

	/* actually hostapd would always choose one diff channel */
	if (cfg80211_chandef_identical(&params->chandef, &priv->chan)) {
		PRINTM(MMSG,
		       "csa channel is same with current channel, invaild\n");
		ret = -EINVAL;
		goto done;
	}

	if (params->block_tx) {
		if (netif_carrier_ok(dev))
			netif_carrier_off(dev);
		woal_stop_queue(dev);
		priv->uap_tx_blocked = MTRUE;
	}

	woal_clear_all_mgmt_ies(priv, MOAL_IOCTL_WAIT);
	if (woal_cfg80211_set_beacon(wiphy, dev, &params->beacon_csa)) {
		PRINTM(MERROR, "%s: setting csa mgmt ies failed\n", __func__);
		goto done;
	}

	memcpy(&priv->csa_chan, &params->chandef,
	       sizeof(struct cfg80211_chan_def));
	memcpy(&priv->beacon_after, &params->beacon_after,
	       sizeof(struct cfg80211_beacon_data));

	if (!priv->phandle->fw_ecsa_enable) {
		if (MLAN_STATUS_SUCCESS !=
		    woal_set_get_sys_config(priv, MLAN_ACT_GET, MOAL_IOCTL_WAIT,
					    &bss_cfg)) {
			PRINTM(MERROR, "%s: get uap config failed\n", __func__);
			ret = -EFAULT;
			goto done;
		}
		chsw_msec = params->count * bss_cfg.beacon_period;
		queue_delayed_work(priv->csa_workqueue, &priv->csa_work,
				   msecs_to_jiffies(chsw_msec));
	}
done:
	LEAVE();
	return ret;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
/**
 * @brief Notify cfg80211 uap channel changed
 *
 * @param priv          A pointer moal_private structure
 * @param pchan_info    A pointer to chan_band structure
 *
 * @return          N/A
 */
void
woal_cfg80211_notify_uap_channel(moal_private *priv,
				 chan_band_info * pchan_info)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
	struct cfg80211_chan_def chandef;
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
	enum nl80211_channel_type type;
	enum ieee80211_band band;
	int freq = 0;
#endif
#endif
	ENTER();

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
	if (MLAN_STATUS_SUCCESS ==
	    woal_chandef_create(priv, &chandef, pchan_info))
		cfg80211_ch_switch_notify(priv->netdev, &chandef);
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
	if (pchan_info->band_config.chanBand == BAND_2GHZ)
		band = IEEE80211_BAND_2GHZ;
	else if (pchan_info->band_config.chanBand == BAND_5GHZ)
		band = IEEE80211_BAND_5GHZ;
	else {
		LEAVE();
		return;
	}
	freq = ieee80211_channel_to_frequency(pchan_info->channel, band);
	switch (pchan_info->band_config.chanWidth) {
	case CHAN_BW_20MHZ:
		if (pchan_info->is_11n_enabled)
			type = NL80211_CHAN_HT20;
		else
			type = NL80211_CHAN_NO_HT;
		break;
	default:
		if (pchan_info->band_config.chan2Offset == SEC_CHAN_ABOVE)
			type = NL80211_CHAN_HT40PLUS;
		else if (pchan_info->band_config.chan2Offset == SEC_CHAN_BELOW)
			type = NL80211_CHAN_HT40MINUS;
		else
			type = NL80211_CHAN_HT20;
		break;
	}
	cfg80211_ch_switch_notify(priv->netdev, freq, type);
#endif
#endif
	LEAVE();
}
#endif

/**
 * @brief Register the device with cfg80211
 *
 * @param dev       A pointer to net_device structure
 * @param bss_type  BSS type
 *
 * @return          MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_register_uap_cfg80211(struct net_device *dev, t_u8 bss_type)
{
	mlan_status ret = MLAN_STATUS_SUCCESS;
	moal_private *priv = (moal_private *)netdev_priv(dev);
	struct wireless_dev *wdev = NULL;

	ENTER();

	wdev = (struct wireless_dev *)&priv->w_dev;
	memset(wdev, 0, sizeof(struct wireless_dev));

	wdev->wiphy = priv->phandle->wiphy;
	if (!wdev->wiphy) {
		LEAVE();
		return MLAN_STATUS_FAILURE;
	}

	if (bss_type == MLAN_BSS_TYPE_UAP)
		wdev->iftype = NL80211_IFTYPE_AP;

	dev_net_set(dev, wiphy_net(wdev->wiphy));
	dev->ieee80211_ptr = wdev;
	SET_NETDEV_DEV(dev, wiphy_dev(wdev->wiphy));
	priv->wdev = wdev;

	LEAVE();
	return ret;
}
