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

#define ADD_FIELD(NAME)                 LOG_HD_##NAME
#define ADD_SIZE(NAME, SIZE)            LOG_HD_##NAME##_SIZE = SIZE
#define ADD_POS(NAME, IN)               \
                LOG_HD_##NAME##_POS = ((LOG_HD_##IN##_POS) + (LOG_HD_##IN##_SIZE))

#define FIELD_NAMES(NAME1, SIZE1, NAME2, SIZE2, \
                    NAME3, SIZE3, NAME4, SIZE4, \
                    NAME5, SIZE5, NAME6, SIZE6, \
                    NAME7, SIZE7, NAME8, SIZE8) \
typedef enum {                                  \
        ADD_FIELD(NAME1) = 0,                   \
        ADD_FIELD(NAME2),                       \
        ADD_FIELD(NAME3),                       \
        ADD_FIELD(NAME4),                       \
        ADD_FIELD(NAME5),                       \
        ADD_FIELD(NAME6),                       \
        ADD_FIELD(NAME7),                       \
        ADD_FIELD(NAME8)                        \
} log_header_field_name;                        \
                                                \
typedef enum {                                  \
        ADD_SIZE(NAME1, SIZE1),                 \
        ADD_SIZE(NAME2, SIZE2),                 \
        ADD_SIZE(NAME3, SIZE3),                 \
        ADD_SIZE(NAME4, SIZE4),                 \
        ADD_SIZE(NAME5, SIZE5),                 \
        ADD_SIZE(NAME6, SIZE6),                 \
        ADD_SIZE(NAME7, SIZE7),                 \
        ADD_SIZE(NAME8, SIZE8)                  \
} log_header_field_size;                        \
                                                \
typedef enum {                                  \
        LOG_HD_MAGIC_POS = 0,                   \
        ADD_POS(NAME2, NAME1),                  \
        ADD_POS(NAME3, NAME2),                  \
        ADD_POS(NAME4, NAME3),                  \
        ADD_POS(NAME5, NAME4),                  \
        ADD_POS(NAME6, NAME5),                  \
        ADD_POS(NAME7, NAME6),                  \
        ADD_POS(NAME8, NAME7)                   \
} log_header_field_pos;                         \
                                                \
static const int log_field_size[] = {SIZE1, SIZE2, SIZE3, SIZE4, SIZE5, SIZE6, SIZE7, SIZE8 }; \
static const int log_field_pos[] = { LOG_HD_##NAME1##_POS,      \
                                     LOG_HD_##NAME2##_POS,      \
                                     LOG_HD_##NAME3##_POS,      \
                                     LOG_HD_##NAME4##_POS,      \
                                     LOG_HD_##NAME5##_POS,      \
                                     LOG_HD_##NAME6##_POS,      \
                                     LOG_HD_##NAME7##_POS,      \
                                     LOG_HD_##NAME8##_POS };

FIELD_NAMES(MAGIC, 4,   \
            LEVEL, 2,   \
            LABEL, 2,   \
            BODY, 3,    \
            RFU1, 0,    \
            RFU2, 0,    \
            RFU3, 0,    \
            PID, 4)

#define BIT_USER_KERN	4
#define BIT_SWD_NWD     5
#define BIT_ENC         6
#define BIT_PREFIX      7

typedef enum {
        USER_MODE       = 0,
        KERN_MODE	= 1 << BIT_USER_KERN,
        SWD_MODE	= 0,
        NWD_MODE	= 1 << BIT_SWD_NWD,
        PLAIN_MODE	= 0,
        ENC_MODE	= 1 << BIT_ENC,
        PREFIX_MODE	= 0,
        NOPREFIX_MODE	= 1 << BIT_PREFIX,
} log_header_info_type;

#define IS_USER(A)      (A & (1 << BIT_USER_KERNEL)) ? 0 : 1
#define IS_SWD(A)       (A & (1 << BIT_SWD_NWD)) ? 0 : 1
#define IS_PLAIN(A)     (A & (1 << BIT_ENC)) ? 0 : 1
#define IS_PREFIX(A)    (A & (1 << BIT_PREFIX)) ? 0 : 1
#define GET_LEVEL(A)    (A & 0x7)

typedef struct
{
        uint32_t magic;
        uint32_t level;
        uint32_t label_size;
        uint32_t body_size;

        uint32_t pid;
} log_header_type;

#define LHDSIZE                 ((LOG_HD_PID_POS) + (LOG_HD_PID_SIZE))
#define LOG_HD_MAGIC_VAL        0xAE18
#define LOG_MAGIC               "AE18"

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

static inline uint32_t get_header_field(char *buffer, log_header_field_name type)
{
	int i;
	int shift, temp;
	uint32_t ret = 0;
	char *pos = NULL;
	if(buffer == NULL)
		return 0;

	pos = buffer + log_field_pos[type];

	for(i = 0; i < log_field_size[type]; ++i)
	{
		temp = log_field_size[type] - i - 1;
		shift = 4 * temp;
		ret += (hex2int(pos[i]) << shift);
	}
	return ret;
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
	header->level = get_header_field(buffer, LOG_HD_LEVEL);
	header->label_size = get_header_field(buffer, LOG_HD_LABEL);
	header->body_size = get_header_field(buffer, LOG_HD_BODY);
	header->pid = get_header_field(buffer, LOG_HD_PID);
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
