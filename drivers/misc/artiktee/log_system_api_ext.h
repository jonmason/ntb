/*********************************************************
 * Copyright (C) 2011 - 2016 Samsung Electronics Co., Ltd All Rights Reserved
 * Author: Soonhong Kwon <aron.kwon@samsung.com>
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

#ifndef __LOG_SYSTEM_API_REE_H__
#define __LOG_SYSTEM_API_REE_H__

/* Changable */
#define LOG_HD_MAGIC_SIZE 4
#define LOG_HD_GP_SIZE 1
#define LOG_HD_LEVEL_SIZE 1
#define LOG_HD_LABEL_SIZE 2
#define LOG_HD_BODY_SIZE 3
#define LOG_HD_PID_SIZE 4

/* Do Not Change */
#define LOG_HD_MAGIC_POS        0
#define LOG_HD_GP_POS        ((LOG_HD_MAGIC_POS) + (LOG_HD_MAGIC_SIZE))
#define LOG_HD_LEVEL_POS        ((LOG_HD_GP_POS) + (LOG_HD_GP_SIZE))
#define LOG_HD_LABEL_POS        ((LOG_HD_LEVEL_POS) + (LOG_HD_LEVEL_SIZE))
#define LOG_HD_BODY_POS         ((LOG_HD_LABEL_POS) + (LOG_HD_LABEL_SIZE))
#define LOG_HD_PID_POS          ((LOG_HD_BODY_POS) + (LOG_HD_BODY_SIZE))
#define LHDSIZE                 ((LOG_HD_PID_POS) + (LOG_HD_PID_SIZE))
#define LOG_HD_MAGIC_VAL        0xAE18
#define LOG_MAGIC               "AE18"

typedef struct {
	uint32_t magic;
	uint32_t log_gen_point;
	uint32_t log_level;
	uint32_t log_label_size;
	uint32_t log_body_size;
	uint32_t log_pid;
} log_header_type;

enum {
	NO_MAGIC = 0,
	TRUNCATED_MAGIC,
	COMPLETE_MAGIC,
};

#define MIN(a, b) ((a > b) ? b : a)

static inline uint32_t hex2int(char ch)
{
	if (ch >= '0' && ch <= '9')
		return (uint32_t) (ch - '0');
	else if (ch >= 'a' && ch <= 'f')
		return (uint32_t) (ch - 'a' + 10);
	else if (ch >= 'A' && ch <= 'F')
		return (uint32_t) (ch - 'A' + 10);
	else
		return 0;
}

static inline uint32_t get_log_gp(char *buffer)
{				/* 1 Byte */
	if (buffer == NULL)
		return 0;

	return hex2int(buffer[0]);
}

static inline uint32_t get_log_level(char *buffer)
{				/* 1 Byte */
	if (buffer == NULL)
		return 0;

	return hex2int(buffer[0]);
}

static inline uint32_t get_log_label_size(char *buffer)
{				/* 2 Byte */
	if (buffer == NULL)
		return 0;

	return (hex2int(buffer[0]) << 4) + (hex2int(buffer[1]));
}

static inline uint32_t get_log_body_size(char *buffer)
{				/* 3 Byte */
	if (buffer == NULL)
		return 0;

	return (hex2int(buffer[0]) << 8) +
	    (hex2int(buffer[1]) << 4) + (hex2int(buffer[2]));
}

static inline uint32_t get_log_pid(char *buffer)
{
	if (buffer == NULL)
		return 0;

	return (hex2int(buffer[0]) << 12) +
	    (hex2int(buffer[1]) << 8) +
	    (hex2int(buffer[2]) << 4) + (hex2int(buffer[3]));
}

static inline uint32_t get_log_header(char *buffer, log_header_type *header)
{
	int i = 0;
	char temp[5] = LOG_MAGIC;

	if (buffer == NULL || header == NULL)
		return 0;

	for (i = 0; i < 4; ++i) {
		if (temp[i] != buffer[i])
			return 0;
	}
	header->log_gen_point = get_log_gp(buffer + LOG_HD_GP_POS);
	header->log_level = get_log_level(buffer + LOG_HD_LEVEL_POS);
	header->log_label_size = get_log_label_size(buffer + LOG_HD_LABEL_POS);
	header->log_body_size = get_log_body_size(buffer + LOG_HD_BODY_POS);
	header->log_pid = get_log_pid(buffer + LOG_HD_PID_POS);
	return 1;
}

static inline uint32_t get_check_magic(char *buffer)
{
	int i = 0;
	char temp[5] = LOG_MAGIC;

	uint32_t ret = NO_MAGIC;
	if (buffer == NULL)
		return 0;

	for (i = 0; i < 4; ++i) {
		if (temp[i] != buffer[i]) {
			if (buffer[i] == 0)
				ret = TRUNCATED_MAGIC;
			break;
		} else
			ret = COMPLETE_MAGIC;
	}
	return ret;
}
#endif /* __LOG_SYSTEM_API_REE_H__ */
