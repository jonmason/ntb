/*********************************************************
 * Copyright (C) 2011 - 2016 Samsung Electronics Co., Ltd All Rights Reserved
 * Author : Jongtak Lee <jong-tak.lee@samsung.com>
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

#ifndef __TZRSRC_MSG_H__
#define __TZRSRC_MSG_H__

#define RSRC_REGISTER_WSM		(1)
#define RSRC_REQUEST_MONINFO		(2)
#define RSRC_LAST_CMD			(3)

#ifndef UUID_STR_LEN
#define UUID_STR_LEN			(37)
#endif

#define TASK_NAME_LEN			(128)
#define THREAD_NAME_MAX_LENGTH		(32)
#define VM_AREA_NAME_MAX		(31)

struct rsrc_msg {
	int32_t cmd;
	int32_t arg0;
	uint32_t arg1;
};
/*
   WSM for TZRSRC Version 0.1

   *-------------------------------------------*
   |            rsrc_info_hdr                  |
   |                 .totalTasks #N            |
   *-------------------------------------------*
   |            rsrc_info_task #1              |
   |                 .memareaCount #Y          |
   |                 .threadCount #Z           |
   |    *---------------------------------*    |
   |    |       rsrc_info_mem_area #1     |    |
   |    |                ...              |    |
   |    |       rsrc_info_mem_area #Y     |    |
   |    *---------------------------------*    |
   |    |       rsrc_info_thread #1       |    |
   |    |                ...              |    |
   |    |       rsrc_info_thread #Z       |    |
   |    *---------------------------------*    |
   |                                           |
   *-------------------------------------------*
   |                                           |
   |                     ...                   |
   |                                           |
   *-------------------------------------------*
   |            rsrc_info_task #N              |
   |                 .memareaCount #Y          |
   |                 .threadCount #Z           |
   |    *---------------------------------*    |
   |    |       rsrc_info_mem_area #1     |    |
   |    |                ...              |    |
   |    |       rsrc_info_mem_area #Y     |    |
   |    *---------------------------------*    |
   |    |       rsrc_info_thread #1       |    |
   |    |                ...              |    |
   |    |       rsrc_info_thread #Z       |    |
   |    *---------------------------------*    |
   |                                           |
   *-------------------------------------------*
*/


typedef struct
{
	uint64_t	snapshot_time_sec;
	uint32_t	snapshot_time_nsec;
	uint32_t	totalTasks;
	uint32_t	totalPages;
	uint32_t	totalFreePages;
	uint32_t	unsatisfiedReservations;
}
rsrc_info_hdr;

typedef struct
{
	uint32_t	threadCount;
	uint32_t	memareaCount;
	int32_t		pid;
	unsigned char	uid;
	unsigned char	gid;
	uint32_t 	totalMemSize;
	uint64_t	utime;
	uint64_t	stime;
	char		name[TASK_NAME_LEN];
	char		uuid[UUID_STR_LEN];
}
rsrc_info_task;

typedef struct
{
	unsigned long 	start;
	unsigned long 	end;
	uint32_t	flags;
	off_t		offset;
	char		area_name[VM_AREA_NAME_MAX + 1];
}
rsrc_info_mem_area;

typedef struct
{
	int32_t		tid;
	unsigned long	status;
	uint64_t	user_time;
	uint64_t	kernel_time;
	uint64_t	total_irq_time;
	char		name[THREAD_NAME_MAX_LENGTH];
}
rsrc_info_thread;


#ifndef __SecureOS__
#ifdef __KERNEL__
int resource_monitor_cmd(struct rsrc_msg *__user rmsg);
#endif
#endif

#endif // __TZRSRC_MSG_H__
