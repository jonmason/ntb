/*********************************************************
 * Copyright (C) 2011 - 2015 Samsung Electronics Co., Ltd All Rights Reserved
 * Author: Jaroslaw Pelczar <j.pelczar@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 *********************************************************/

#ifndef __TRUSTZONE_SHARED_INCLUDE_TRUSTWAREOS_PRIVATE_SMC_INTERFACE_H__
#define __TRUSTZONE_SHARED_INCLUDE_TRUSTWAREOS_PRIVATE_SMC_INTERFACE_H__

/* R1 - size of kern info buffer */
/* R2 - virtual address of kern info buffer */
#define SMC_STD_GET_KERNEL_INFO			0
/* R1 - wsm id page */
#define SMC_STD_REGISTER_SYSPAGE		1
/* R1 - wsm id page */
#define SMC_STD_REGISTER_MINIDUMP		2
/* R1 - wsm ID */
#define SMC_STD_UNREGISTER_WSM			4
/* R1 - wsm ID */
#define SMC_STD_SYSLOG_DRAIN			5
/* R1 - flag */
#define SMC_STD_TZDAEMON_DEAD			6
#define SMC_STD_SOFTLOCKUP			7
#define SMC_STD_SLAB_FLUSH			8
#define SMC_STD_SOFTTIMER			9
/* R1 - wsm id, R2 - flags */
#define SMC_STD_RPC				10
/* R1 - devfn, R2 - arg 0, R3 = arg 1, R4 = arg 2 */
#define SMC_STD_WATCH				11
/* TODO: register those */
#define SMC_FIQ_ENTRY				12
#define SMC_IRQ_RESUME				13

#define SMC_STD_REGISTER_PHYS_WSM	14
// R1 - CMD, R2 - wsm_id (only for register cmd)
#define SMC_STD_RESOURCE_MONITOR	15

#ifdef CONFIG_FETCH_TEE_INFO
#define SMC_STD_FETCH_TEE_INFO		16
#endif /* !CONFIG_FETCH_TEE_INFO */

/* TODO: this should be sent to PSCI */
#define SMC_PM_CPU_OFF			0
#define SMC_PM_SYSTEM_OFF		1
#define SMC_PM_CPU_SUSPEND		2
#define SMC_PM_CPU_RESUME		3
#define SMC_PM_SYSTEM_RESET		4

#ifndef CONFIG_PSCI
#define SMC_CPU_SUSPEND				17
#define SMC_CPU_SUSPEND_SYS			18
#define SMC_CPU_RESUME_SYS			19
#define SMC_CPU_RESUME				20
#endif /* !CONFIG_PSCI */

#define SMC_STD_GET_UUID			0xFF01

#endif /* __TRUSTZONE_SHARED_INCLUDE_TRUSTWAREOS_PRIVATE_SMC_INTERFACE_H__ */
