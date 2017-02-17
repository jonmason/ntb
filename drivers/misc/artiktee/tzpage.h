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

#ifndef __SOURCE_TZDEV_TZPAGE_H__
#define __SOURCE_TZDEV_TZPAGE_H__

struct __tzdev_page {
	struct list_head queue;
	unsigned int busy;
};

struct tzdev_page {
	union {
		struct page *linux_page;
		struct __tzdev_page *tz_page;
	} u;
	int type;
};

void tzpage_init(phys_addr_t phys_addr, size_t size);

#endif /* __SOURCE_TZDEV_TZPAGE_H__ */
