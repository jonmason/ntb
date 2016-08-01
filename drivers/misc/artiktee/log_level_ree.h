/*********************************************************
 * Copyright (C) 2011 - 2016 Samsung Electronics Co., Ltd All Rights Reserved
 * Author: Jungkyuen <jklolo.lee@samsung.com>
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

#ifndef __LOG_LEVEL_H__
#define __LOG_LEVEL_H__

typedef enum {
#ifndef LOG_DEBUG
	LOG_EMERG = 0,
	LOG_ALERT,
	LOG_CRIT,
	LOG_ERR,
	LOG_WARNING,
	LOG_NOTICE,
	LOG_INFO,
	LOG_DEBUG,
#endif /* LOG_DEBUG */
	LOG_SILENT = 8,
} usr_log_level;

typedef enum {
	K_EMERG = 0,
	K_ALERT,
	K_CRIT,
	K_ERR,
	K_WARNING,
	K_NOTICE,
	K_INFO,
	K_DEBUG,
	K_SILENT,
} kernel_log_level;

typedef enum {
	SWD_USERMODE = 1,
	SWD_KERNMODE,
	NWD_USERMODE,
	NWD_KERNMODE,
	SWD_ENC_USERMODE,
	SWD_ENC_KERNMODE,
} log_header_info;

/* Ree World */
#ifndef NDEBUG
#define TZDAEMON_DLOG_KERN_LOG_LEVEL   LOG_DEBUG  /* OutPut Default */
#define TZDAEMON_DLOG_TA_LOG_LEVEL     LOG_DEBUG  /* OutPut Default */
#define TZDAEMON_FILE_LOG_LEVEL        LOG_DEBUG  /* OutPut Default */
#define TZDAEMON_SYSLOG_LOG_LEVEL      LOG_DEBUG  /* Output Default */
#define TZDEV_TEE_LOG_LEVEL            K_INFO     /* OutPut Default */

#define TZDAEMON_LOCAL_DLOG_LOG_LEVEL   LOG_DEBUG  /* OutPut Default */
#define TZDAEMON_LOCAL_FILE_LOG_LEVEL   LOG_DEBUG  /* OutPut Default */
#define TZDAEMON_LOCAL_SYSLOG_LOG_LEVEL LOG_DEBUG  /* OutPut Default */
#define TZDEV_LOCAL_LOG_LEVEL           K_INFO     /* OutPut Default */

#define NO_HEADER_LOG_LEVEL 			LOG_INFO   /* Generation Default */
#else
#define TZDAEMON_DLOG_KERN_LOG_LEVEL   LOG_ERR     /* OutPut Default */
#define TZDAEMON_DLOG_TA_LOG_LEVEL     LOG_ERR     /* OutPut Default */
#define TZDAEMON_FILE_LOG_LEVEL        LOG_ERR     /* OutPut Default */
#define TZDAEMON_SYSLOG_LOG_LEVEL      LOG_SILENT  /* Output Default */
#define TZDEV_TEE_LOG_LEVEL            K_ERR       /* OutPut Default */

#define TZDAEMON_LOCAL_DLOG_LOG_LEVEL   LOG_ALERT  /* OutPut Default */
#define TZDAEMON_LOCAL_FILE_LOG_LEVEL   LOG_ALERT  /* OutPut Default */
#define TZDAEMON_LOCAL_SYSLOG_LOG_LEVEL LOG_ALERT  /* OutPut Default */
#define TZDEV_LOCAL_LOG_LEVEL           K_ALERT    /* OutPut Default */

#define NO_HEADER_LOG_LEVEL 			LOG_INFO  /* Generation Default */
#endif /* NDEBUG */

/* For Old Log Level */
#define TZLOG_DEBUG K_DEBUG
#define TZLOG_INFO K_INFO
#define TZLOG_WARNING K_WARNING
#define TZLOG_ERROR K_ERR

#endif /* __LOG_LEVEL_H__ */
