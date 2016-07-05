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

#ifndef __TRUSTZONE_SHARED_INCLUDE_TRUSTWAREOS_PRIVATE_WSM_H__
#define __TRUSTZONE_SHARED_INCLUDE_TRUSTWAREOS_PRIVATE_WSM_H__

typedef uint64_t		ns_phys_addr_t;

#define NS_PAGES_PER_LEVEL			((PAGE_SIZE - 16) / sizeof(ns_phys_addr_t))
#define NS_PHYS_ADDR_IS_LEVEL_1		0x1UL

struct ns_level_registration {
	uint32_t		num_pfns;
	uint32_t		flags;
	uint64_t		ctx_id;
	ns_phys_addr_t	address[NS_PAGES_PER_LEVEL];
};

#define NS_WSM_FLAG_KERNEL		1

#endif /* __TRUSTZONE_SHARED_INCLUDE_TRUSTWAREOS_PRIVATE_WSM_H__ */
