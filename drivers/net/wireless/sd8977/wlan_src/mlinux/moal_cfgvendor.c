/** @file moal_cfgvendor.c
  *
  * @brief This file contains the functions for CFG80211 vendor.
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

#include    "moal_cfgvendor.h"
#include    "moal_cfg80211.h"

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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
/**marvell vendor command and event*/
#define MRVL_VENDOR_ID  0x005043
/** vendor events */
const struct nl80211_vendor_cmd_info vendor_events[] = {
	{.vendor_id = MRVL_VENDOR_ID,.subcmd = event_hang,},	/* event_id 0 */
	/**add vendor event here*/
};

/**
 * @brief get the event id of the events array
 *
 * @param event     vendor event
 *
 * @return    index of events array
 */
int
woal_get_event_id(int event)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(vendor_events); i++) {
		if (vendor_events[i].subcmd == event)
			return i;
	}

	return event_max;
}

/**
 * @brief send vendor event to kernel
 *
 * @param priv       A pointer to moal_private
 * @param event    vendor event
 * @param data     a pointer to data
 * @param  len     data length
 *
 * @return      0: success  1: fail
 */
int
woal_cfg80211_vendor_event(IN moal_private *priv,
			   IN int event, IN t_u8 *data, IN int len)
{
	struct wiphy *wiphy = priv->wdev->wiphy;
	struct sk_buff *skb = NULL;
	int event_id = 0;
	t_u8 *pos = NULL;
	int ret = 0;

	ENTER();

	PRINTM(MEVENT, "vendor event :0x%x\n", event);
	event_id = woal_get_event_id(event);
	if (event_max == event_id) {
		PRINTM(MERROR, "Not find this event %d \n", event_id);
		ret = 1;
		LEAVE();
		return ret;
	}

	/**allocate skb*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
	skb = cfg80211_vendor_event_alloc(wiphy, NULL, len,
#else
	skb = cfg80211_vendor_event_alloc(wiphy, len,
#endif
					  event_id, GFP_ATOMIC);
	if (!skb) {
		PRINTM(MERROR, "allocate memory fail for vendor event\n");
		ret = 1;
		LEAVE();
		return ret;
	}
	pos = skb_put(skb, len);
	memcpy(pos, data, len);
	/**send event*/
	cfg80211_vendor_event(skb, GFP_ATOMIC);

	LEAVE();
	return ret;
}

/**
 * @brief vendor command to set drvdbg
 *
 * @param wiphy       A pointer to wiphy struct
 * @param wdev     A pointer to wireless_dev struct
 * @param data     a pointer to data
 * @param  len     data length
 *
 * @return      0: success  1: fail
 */
static int
woal_cfg80211_subcmd_set_drvdbg(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
#ifdef DEBUG_LEVEL1
	struct net_device *dev = wdev->netdev;
	moal_private *priv = (moal_private *)woal_get_netdev_priv(dev);
	struct sk_buff *skb = NULL;
	t_u8 *pos = NULL;
#endif
	int ret = 1;

	ENTER();
#ifdef DEBUG_LEVEL1
	/**handle this sub command*/
	DBG_HEXDUMP(MCMD_D, "Vendor drvdbg", (t_u8 *)data, data_len);

	if (data_len) {
		/* Get the driver debug bit masks from user */
		drvdbg = *((t_u32 *)data);
		PRINTM(MIOCTL, "new drvdbg %x\n", drvdbg);
		/* Set the driver debug bit masks into mlan */
		if (woal_set_drvdbg(priv, drvdbg)) {
			PRINTM(MERROR, "Set drvdbg failed!\n");
			ret = 1;
		}
	}
	/** Allocate skb for cmd reply*/
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(drvdbg));
	if (!skb) {
		PRINTM(MERROR, "allocate memory fail for vendor cmd\n");
		ret = 1;
		LEAVE();
		return ret;
	}
	pos = skb_put(skb, sizeof(drvdbg));
	memcpy(pos, &drvdbg, sizeof(drvdbg));
	ret = cfg80211_vendor_cmd_reply(skb);
#endif
	LEAVE();
	return ret;
}

const struct wiphy_vendor_command vendor_commands[] = {
	{
	 .info = {.vendor_id = MRVL_VENDOR_ID,.subcmd = sub_cmd_set_drvdbg,},
	 .flags = WIPHY_VENDOR_CMD_NEED_WDEV |
	 WIPHY_VENDOR_CMD_NEED_NETDEV | WIPHY_VENDOR_CMD_NEED_RUNNING,
	 .doit = woal_cfg80211_subcmd_set_drvdbg,
	 },

};

/**
 * @brief register vendor commands and events
 *
 * @param wiphy       A pointer to wiphy struct
 *
 * @return
 */
void
woal_register_cfg80211_vendor_command(struct wiphy *wiphy)
{
	ENTER();
	wiphy->vendor_commands = vendor_commands;
	wiphy->n_vendor_commands = ARRAY_SIZE(vendor_commands);
	wiphy->vendor_events = vendor_events;
	wiphy->n_vendor_events = ARRAY_SIZE(vendor_events);
	LEAVE();
}
#endif
