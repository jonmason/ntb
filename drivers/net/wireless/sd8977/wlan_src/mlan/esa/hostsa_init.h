/** @file hostsa_init.h
 *
 *  @brief This file contains definitions the APIs.
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

#ifndef _HOSTSA_INIT_H_
#define _HOSTSA_INIT_H_

#ifndef MACSTR
/** MAC address security format */
#define MACSTR "%02x:XX:XX:XX:%02x:%02x"
#endif

#ifndef MAC2STR
/** MAC address security print arguments */
#define MAC2STR(a) (a)[0], (a)[4], (a)[5]
#endif

extern mlan_status hostsa_init(pmlan_private pmpriv);
extern mlan_status hostsa_cleanup(pmlan_private pmpriv);

#endif /* _MPL_MAIN_H_ */
