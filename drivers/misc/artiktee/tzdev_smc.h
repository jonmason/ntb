/*********************************************************
 * Copyright (C) 2011 - 2015 Samsung Electronics Co., Ltd All Rights Reserved
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

#ifndef __TRUSTZONE_REE_SOURCE_TZDEV_TZDEV_SMC_H__
#define __TRUSTZONE_REE_SOURCE_TZDEV_TZDEV_SMC_H__

#define SMC_STANDARD_CALL		0x00000000
#define SMC_FAST_CALL			0x80000000

#define SMC_32CALL			0x00000000
#define SMC_64CALL			0x40000000

#define SMC_ENTITY_MASK			0x3F000000
#define SMC_ENTITY_SHIFT		24

#define SMC_FUNCTION_MASK		0x0000FFFF

#define SMC_ENTITY_ARM_ARCH		(0 << SMC_ENTITY_SHIFT)
#define SMC_ENTITY_CPU_SVC		(1 << SMC_ENTITY_SHIFT)
#define SMC_ENTITY_SIP_SVC		(2 << SMC_ENTITY_SHIFT)
#define SMC_ENTITY_OEM_SVC		(3 << SMC_ENTITY_SHIFT)
#define SMC_ENTITY_STD_CALL		(4 << SMC_ENTITY_SHIFT)
#define SMC_ENTITY_TA_CALL0		(48 << SMC_ENTITY_SHIFT)
#define SMC_ENTITY_TA_CALL1		(49 << SMC_ENTITY_SHIFT)
#define SMC_ENTITY_TRUSTED_OS(X)	((50 + (X)) << SMC_ENTITY_SHIFT)
#define SMC_ENTITY_SECUREOS		SMC_ENTITY_TRUSTED_OS(0)

struct monitor_uuid {
	uint32_t uuid0;
	uint32_t uuid1;
	uint32_t uuid2;
	uint32_t uuid3;
};

#define __MONITOR_MAKE_UUID(A, B, C, D, E, F) \
		{(A), ((B) << 16) | (C), ((D) << 16) | (E), (F)}

#define MONITOR_SECURE_OS_UID	\
	__MONITOR_MAKE_UUID(0xdf35d0b4, \
			0x4635, 0xcf73, 0x3a12,	\
			0x28be, 0x3a8d326b)

struct monitor_arguments {
	uint32_t function_id;
	uint32_t arg[4];
};

#define SMC_PREEMPTED		0xfffffffe

struct monitor_result {
	uint32_t res[4];
};

static inline int monitor_uuid_same(const struct monitor_uuid x,
				    const struct monitor_uuid y)
{
	return x.uuid0 == y.uuid0 &&
	    x.uuid1 == y.uuid1 && x.uuid2 == y.uuid2 && x.uuid3 == y.uuid3;
}

#include "smc_interface.h"

struct secos_kern_info;

int smc_init_monitor(void);
int scm_query_kernel_info(struct secos_kern_info *kinfo);
int scm_syscrash_register(int wsm_id);
int scm_minidump_register(int wsm_id);
int scm_register_wsm(const void *addr, size_t size, unsigned int flags,
		     unsigned int tee_ctx_id);
int scm_unregister_wsm(int memid);
int scm_syslog(int memid, unsigned int option);
int scm_tzdaemon_dead(int flag);
int scm_softlockup(void);
int scm_slab_flush(void);
int scm_soft_timer(void);
int scm_invoke_svc(unsigned long channel, unsigned long flags);
int scm_watch(unsigned long devfn, unsigned long a0, unsigned long a1,
	      unsigned long a2);
int scm_register_phys_wsm(phys_addr_t arg_pfn);

#ifdef CONFIG_FETCH_TEE_INFO
int scm_fetch_tzinfo(int cmd, int arg);
#endif /* !CONFIG_FETCH_TEE_INFO */

#ifndef CONFIG_PSCI
int scm_cpu_suspend(void);
int scm_cpu_resume(void);
int scm_sys_suspend(void);
int scm_sys_resume(void);
#endif /* !CONFIG_PSCI */

#endif /* __TRUSTZONE_REE_SOURCE_TZDEV_TZDEV_SMC_H__ */
