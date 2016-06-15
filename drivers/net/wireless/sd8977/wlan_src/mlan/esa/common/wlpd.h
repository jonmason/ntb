/** @file wlpd.h
 *
 *  @brief RX and TX packet descriptor
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
#ifndef WLPD_H__
#define WLPD_H__

/** include files **/
#include "packetType.h"
#include "wltypes.h"

 /** @defgroup PacketTypes Tx Rx Data Packet Types
 *  Functions exported by wlpd.h
 *  @{
 */

/*****************************************************************************/
typedef MLAN_PACK_START struct {
	UINT8 tdlsPkt:1;
	UINT8 rsvd:7;
} MLAN_PACK_END rxFlags_t;
/**
*** @brief Enumeration of action to be take for returned Rx Packets.
**/

/**
*** @brief Receive Packet Descriptor
**/
typedef MLAN_PACK_START struct RxPD_t {
	/* TODO: Port EMBEDDED_TCPIP and VISTA_802_11_DRIVER_INTERFACE members
	   to W8786 */
	UINT8 RxBSSType;
	UINT8 RxBSSNum;
	UINT16 RxPacketLength;	// !< Rx Packet Length
	SINT16 RxPacketOffset;	// !< Offset to the Rx Data
	UINT16 RxPacketType;
	UINT16 SeqNum;
	UINT8 userPriority;
	UINT8 RxRate;		// LG 0-3 (11b), 5-12(11g), HT :MCS# (11n)
	SINT8 SNR;
	SINT8 RxSQ2;		// defined to RxNF
	UINT8 RxHTInfo;		// [Bit o] RxRate format : Legacy = 0 , HT =1
	// [Bit 1] HT Bandwidth :BW20 =0 , BW40 = 1
	// [Bit 2] HT Guard Interval : LGI = 0, SGI = 1
#if defined(VISTA_802_11_DRIVER_INTERFACE) || defined(SNIFFER_MODE_ENABLE)
	UINT8 PacketType;
	UINT8 NumFragments;
	UINT8 EncryptionStatus;
#else
	UINT8 Reserved2[3];
#endif
	rxFlags_t flags;
	UINT8 Reserved3;
} MLAN_PACK_END RxPD_t;

#define PACKET_TYPE_802_3               0
#define PACKET_TYPE_802_11              1
#define PACKET_TYPE_802_11_QOS          2
#define PACKET_TYPE_802_11_MRVL_MESH    3
#define PACKET_TYPE_TDLS                4

#define MESH_FWD_PACKET_MASK  (1 << 0)
#define MESH_OLPC_PKT_MASK    (1 << 1)

#define PACKET_DECRYPTED        0
#define PACKET_NOT_DECRYPTED    1

#if defined(VISTA_802_11_DRIVER_INTERFACE)

#define PACKET_NO_DECRYPT_NEEDED 2
#define MAGIC_PACKET_MARKER_BITMASK     (1<<3)
#define COALESCED_PACKET_MARKER_BITMASK (1<<2)

#endif

#define RxNF    RxSQ2

// Since Small Debug print and Myung's Debug Facility both use
// PKT_TYPE_DEBUG rx pkt, the following debug header is required to distinguish
// Small Debug from Myung's.
//
typedef MLAN_PACK_START struct {
	UINT8 dbg_type;
	UINT8 reserve[3];
} MLAN_PACK_END to_host_dbg_hdr_t;
#define DBG_TYPE_SMALL  2

/* The following fields have been added for Null frame handling
   in Power Save Mode.
 */
typedef MLAN_PACK_START struct {
	UINT8 nullPkt:1;
	UINT8 overRideFwPM:1;
	UINT8 pmVal:1;
	UINT8 lastTxPkt:1;
	UINT8 tdlsPkt:1;
	UINT8 rsvd:3;
} MLAN_PACK_END wcb_flags_t;

/**
*** @brief Transmit Packet Descriptor
**/
typedef MLAN_PACK_START struct {
	/* TODO: Port EMBEDDED_TCPIP and VISTA_802_11_DRIVER_INTERFACE members
	   to W8786 */
	UINT8 TxBSSType;
	UINT8 TxBSSNum;
	UINT16 TxPacketLength;	// !< Tx Packet Length
	UINT16 TxPacketOffset;	// !< Offset to Tx Data
	UINT16 TxPacketType;	// !< Tx Packet Type
	UINT32 TxControl;	// b3-0: RateID; b4:HostRateCtrl;
	// b11-8: RetryLimit; b12:HostRetryCtrl;
	// b14-13: Ack Policy, 10 ACK_IMMD
	// 11 NO_ACK 0x ACK_PER_FRM
	UINT8 userPriority;
	wcb_flags_t flags;	// These BitFields are for Null Frame Handling
				// and
	// other Power Save requirements.
	UINT8 PktDelay_2ms;	/* Driver queue delay used in stats and MSDU **
				   lifetime expiry calcs; value is represented
				   ** by 2ms units (ms bit shifted by 1) */
#ifdef VISTA_802_11_DRIVER_INTERFACE
	/* Include Packet type for NWF */
	UINT8 PacketType;
	UINT8 EncrOpt;
#else
	UINT8 Reserved[2];
#endif
	UINT8 TxTokenId;

} MLAN_PACK_END wcb_t;

// Encryption Option is an 8 bit field
#define ENCR_OPT_NORMAL          0x00	// Normal packet. Follows Enc rules in
					// FW
#define ENCR_OPT_FORCE_PTEXT     0x01	// Force plain text. No encryption.
#define ENCR_OPT_FW_KEY_MAP      0x02	// Encrypt using key mapping table in
					// FW
#define ENCR_OPT_PTEXT_80211_PKT 0x03	// No encryption for 802.11 pkt

/*@}*/

#endif /* _WLPD_H_ */
