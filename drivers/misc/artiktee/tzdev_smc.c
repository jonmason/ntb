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

#include <linux/kernel.h>
#include <linux/cpu.h>

#include "tzdev.h"
#include "tzdev_internal.h"
#include "tzdev_smc.h"
#include "tzpage.h"
#include "tzlog_print.h"

static inline void __do_call_smc_internal(struct monitor_arguments *args,
					  struct monitor_result *result)
{
#if defined(__aarch64__)
	register unsigned long r0 asm("x0");
	register unsigned long r1 asm("x1");
	register unsigned long r2 asm("x2");
	register unsigned long r3 asm("x3");
	register unsigned long r4 asm("x4");
#elif defined(__arm__)
	register unsigned long r0 asm("r0");
	register unsigned long r1 asm("r1");
	register unsigned long r2 asm("r2");
	register unsigned long r3 asm("r3");
	register unsigned long r4 asm("r4");
#else
#error Unsupported architecture
#endif
	r0 = args->function_id;
	r1 = args->arg[0];
	r2 = args->arg[1];
	r3 = args->arg[2];
	r4 = args->arg[3];

	__asm__ __volatile__(
#ifdef __arm__
	".arch_extension sec\n"
#endif
	"smc 0x0" : "=r"(r0), "=r"(r1), "=r"(r2), "=r"(r3)
	: "r"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r4)
	: "memory");

	result->res[0] = r0;
	result->res[1] = r1;
	result->res[2] = r2;
	result->res[3] = r3;
}

static inline void do_call_smc_internal(struct monitor_arguments *args,
					struct monitor_result *result)
{
	__do_call_smc_internal(args, result);
}

int smc_init_monitor(void)
{
	struct monitor_uuid uuid;
	struct monitor_arguments args = { 0, {0,} };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_FAST_CALL | SMC_ENTITY_SECUREOS | SMC_STD_GET_UUID;
	do_call_smc_internal(&args, &res);
	if (res.res[0] == -1) {
		tzlog_print(TZLOG_ERROR, "Failed to query SecureOS UUID.\n");
		return -1;
	}
	uuid.uuid0 = res.res[0];
	uuid.uuid1 = res.res[1];
	uuid.uuid2 = res.res[2];
	uuid.uuid3 = res.res[3];

	if (!monitor_uuid_same
	    (uuid, (struct monitor_uuid)MONITOR_SECURE_OS_UID)) {
		tzlog_print(TZLOG_ERROR, "Incompatible SecureOS ID\n");
		return -1;
	}

	return 0;
}

int scm_query_kernel_info(struct secos_kern_info *kinfo)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_STD_GET_KERNEL_INFO;
	args.arg[0] = SECOS_KERN_INFO_V1;
	args.arg[1] = (uint32_t) (uintptr_t) kinfo;
#ifdef CONFIG_64BIT
	args.arg[2] = (uint32_t) (((uintptr_t) kinfo) >> 32);
	compiletime_assert(sizeof(uintptr_t) == 8, "Incorrect pointer size");
#else
	args.arg[2] = 0;
	compiletime_assert(sizeof(uintptr_t) == 4, "Incorrect pointer size");
#endif

	do_call_smc_internal(&args, &res);
	return res.res[0];
}

int scm_syscrash_register(int wsm_id)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_STD_REGISTER_SYSPAGE;
	args.arg[0] = wsm_id;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}

int scm_minidump_register(int wsm_id)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_STD_REGISTER_MINIDUMP;
	args.arg[0] = wsm_id;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}

int scm_unregister_wsm(int memid)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_STD_UNREGISTER_WSM;
	args.arg[0] = memid;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}

int scm_syslog(int memid, unsigned int option)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_STD_SYSLOG_DRAIN;
	args.arg[0] = memid;
	args.arg[1] = option;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}

int scm_tzdaemon_dead(int flag)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_STD_TZDAEMON_DEAD;
	args.arg[0] = flag;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}

int scm_softlockup(void)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_STD_SOFTLOCKUP;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}

int scm_slab_flush(void)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_STD_SLAB_FLUSH;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}

int scm_soft_timer(void)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_STD_SOFTTIMER;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}

int scm_invoke_svc(unsigned long channel, unsigned long flags)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS | SMC_STD_RPC;
	args.arg[0] = channel;
	args.arg[1] = flags;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}

int scm_watch(unsigned long devfn, unsigned long a0, unsigned long a1,
	      unsigned long a2)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_STD_WATCH;
	args.arg[0] = devfn;
	args.arg[1] = a0;
	args.arg[2] = a1;
	args.arg[3] = a2;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}

int scm_register_phys_wsm(phys_addr_t arg_pfn)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_STD_REGISTER_PHYS_WSM;
	args.arg[0] = lower_32_bits(arg_pfn);
	args.arg[1] = upper_32_bits(arg_pfn);
	args.arg[2] = 0;
	args.arg[3] = 0;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}

#ifdef CONFIG_FETCH_TEE_INFO
int scm_fetch_tzinfo(int cmd, int arg)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
			SMC_STD_FETCH_TEE_INFO;

	args.arg[0] = cmd;
	args.arg[1] = arg;
	args.arg[2] = 0;
	args.arg[3] = 0;

	__do_call_smc_internal(&args, &res);

	return res.res[0];
}
#endif /* !CONFIG_FETCH_TEE_INFO */

#ifndef CONFIG_PSCI
int scm_cpu_suspend(void)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_CPU_SUSPEND;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}

int scm_cpu_resume(void)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_CPU_RESUME;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}

int scm_sys_suspend(void)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_CPU_SUSPEND_SYS;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}

int scm_sys_resume(void)
{
	struct monitor_arguments args = { 0, };
	struct monitor_result res = { {0,} };

	args.function_id =
	    SMC_32CALL | SMC_STANDARD_CALL | SMC_ENTITY_SECUREOS |
	    SMC_CPU_RESUME_SYS;

	do_call_smc_internal(&args, &res);

	return res.res[0];
}
#endif /* !CONFIG_PSCI */
