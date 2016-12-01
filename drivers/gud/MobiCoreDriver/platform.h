/*
 * Copyright (c) 2015-2016 TRUSTONIC LIMITED
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef _MC_DRV_PLATFORM_H_
#define _MC_DRV_PLATFORM_H_

/* #include <mach/irqs.h> */

/* Tee Interrupt. */
#define MC_INTR_SSIQ	42

#define TBASE_CORE_SWITCHER

#define COUNT_OF_CPUS 4

/* Values of MPIDR regs */
#define CPU_IDS {0x0A00, 0x0A01, 0x0A02, 0x0A03}

/*
 * Enable Fastcall worker thread
 *
 * #define MC_FASTCALL_WORKER_THREAD
 */

/*
 * On Kernel >= 4.4 debugfs_create_bool API changed
 * and this flag should be defined
 * define DEBUGFS_CREATE_BOOL_TAKES_A_BOOL
 */
#define DEBUGFS_CREATE_BOOL_TAKES_A_BOOL

#endif /* _MC_DRV_PLATFORM_H_ */
