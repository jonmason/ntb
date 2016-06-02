/** @file moal_cfgvendor.h
  *
  * @brief This file contains the CFG80211 vendor specific defines.
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

#ifndef _MOAL_CFGVENDOR_H_
#define _MOAL_CFGVENDOR_H_

#include    "moal_main.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
/**vendor event*/
enum vendor_event { event_hang = 0, event_max,
};

/**vendor sub command*/
enum vendor_sub_command { sub_cmd_set_drvdbg = 0, sub_cmd_max,
};
void woal_register_cfg80211_vendor_command(struct wiphy *wiphy );
int woal_cfg80211_vendor_event(IN moal_private *priv, IN int event,
				IN t_u8 *data, IN int len);

#endif	/*  */

#endif	/* _MOAL_CFGVENDOR_H_ */
