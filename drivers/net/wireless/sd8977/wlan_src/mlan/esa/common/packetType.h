/** @file packetType.h
 *
 *  @brief This file defines Packet Type enumeration used for PacketType fields in RX and TX
 *          packet descriptors
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
#ifndef _PACKETTYPE_H_
#define _PACKETTYPE_H_

/**
*** @brief Enumeration of different Packets Types.
**/

typedef enum {
	PKT_TYPE_802DOT3_DEFAULT = 0,	/* !< For RX packets it represents **
					   IEEE 802.3 SNAP frame .  For ** TX
					   Packets.  This Field is for **
					   backwards compatibility only and **
					   should not be used going ** forward. */
	PKT_TYPE_802DOT3_LLC = 1,	// !< IEEE 802.3 frame with LLC header
	PKT_TYPE_ETHERNET_V2 = 2,	// !< Ethernet version 2 frame
	PKT_TYPE_802DOT3_SNAP = 3,	// !< IEEE 802.3 SNAP frame
	PKT_TYPE_802DOT3 = 4,	// !< IEEE 802.3 frame
	PKT_TYPE_802DOT11 = 5,	// !< IEEE 802.11 frame
	PKT_TYPE_ETCP_SOCKET_DATA = 7,	// !< eTCP Socket Data
	PKT_TYPE_RAW_DATA = 8,	// !< Non socket data when using eTCP
	PKT_TYPE_MRVL_MESH = 9,	// !< Marvell Mesh frame

	/* Marvell Internal firmware packet types ** Range from 0x0E to 0xEE **
	   These internal Packet types should grow from ** 0xEE down.  This
	   will leave room incase the packet ** types between the driver &
	   firmware need to be expanded */
	PKT_TYPE_MRVL_EAPOL_MSG = 0xDF,
	PKT_TYPE_MRVL_BT_AMP = 0xE0,
	PKT_TYPE_FWD_MGT = 0xE2,
	PKT_TYPE_MGT = 0xE5,
	PKT_TYPE_MRVL_AMSDU = 0xE6,
	PKT_TYPE_MRVL_BAR = 0xE7,
	PKT_TYPE_MRVL_LOOPBACK = 0xE8,
	PKT_TYPE_MRVL_DATA_MORE = 0xE9,
	PKT_TYPE_MRVL_DATA_LAST = 0xEA,
	PKT_TYPE_MRVL_DATA_NULL = 0xEB,
	PKT_TYPE_MRVL_UNKNOWN = 0xEC,
	PKT_TYPE_MRVL_SEND_TO_DATASWITCH = 0xED,
	PKT_TYPE_MRVL_SEND_TO_HOST = 0xEE,

	PKT_TYPE_DEBUG = 0xEF,

	// Customer range for Packet Types
	// Range 0xF0 to 0xFF
} PacketType;

#endif
