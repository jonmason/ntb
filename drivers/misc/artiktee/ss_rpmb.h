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

#ifndef __SECURE_RPMB_H__
#define __SECURE_RPMB_H__

#define EMMC_SECOTR_SIZE 512
#define RPMB_SECOTR 256
#define RPMB_SECOTR_MASK (RPMB_SECOTR - 1)
#define RPMB_READ_BLOCKS_UNIT	4
#define RPMB_WRITE_BLOCKS_UNIT   1
#define RPMB_READ_RETRY_TIME	 1
#define RPMB_WRITE_RETRY_TIME	 1

#define RPMB_FRAME_STUFF_SIZE				196
#define RPMB_FRAME_KEYMAC_SIZE				32
#define RPMB_FRAME_DATA_SIZE				256
#define RPMB_FRAME_NONCE_SIZE				16
#define RPMB_FRAME_WRITECOUNTER_SIZE		4
#define RPMB_FRAME_ADDR_SIZE				2
#define RPMB_FRAME_BLOCKCOUNT_SIZE			2
#define RPMB_FRAME_RESULT_SIZE				2
#define RPMB_FRAME_REQRESP_SIZE				2

#define RPMB_EXTRA_DATA_SIZE	\
		(RPMB_FRAME_NONCE_SIZE + RPMB_FRAME_WRITECOUNTER_SIZE + \
		RPMB_FRAME_ADDR_SIZE + RPMB_FRAME_BLOCKCOUNT_SIZE + \
		RPMB_FRAME_RESULT_SIZE + RPMB_FRAME_REQRESP_SIZE)

enum rpmb_op_type {
	MMC_RPMB_WRITE_KEY = 0x01,
	MMC_RPMB_READ_CNT = 0x02,
	MMC_RPMB_WRITE = 0x03,
	MMC_RPMB_READ = 0x04,

	/* For internal usage only, do not use it directly */
	MMC_RPMB_READ_RESP = 0x05
};

#define RPMB_KEY_NO_PROGRAM_FLAG   0x0700

struct rpmb_frame {
	u_int8_t stuff[196];
	u_int8_t key_mac[32];
	u_int8_t data[256];
	u_int8_t nonce[16];
	u_int32_t write_counter;
	u_int16_t addr;
	u_int16_t block_count;
	u_int16_t result;
	u_int16_t req_resp;
};

int ss_rpmb_write_block(struct rpmb_frame *rpmb_frame, u8 *buf, size_t size);
int ss_rpmb_read_block(struct rpmb_frame *rpmb_frame, u8 *buf, size_t size);
int ss_rpmb_get_wctr(u32 *wctr);
#endif /* __SECURE_RPMB_H__ */
