/** @file rc4.c
 *
 *  @brief This file defines rc4 encrypt algorithm
 *
 *  (C) Copyright 2014-2016 Marvell International Ltd. All Rights Reserved
 *
 *  MARVELL CONFIDENTIAL
 *  The source code contained or described herein and all documents related to
 *  the source code ("Material") are owned by Marvell International Ltd or its
 *  suppliers or licensors. Title to the Material remains with Marvell International Ltd
 *  or its suppliers and licensors. The Material contains trade secrets and
 *  proprietary and confidential information of Marvell or its suppliers and
 *  licensors. The Material is protected by worldwide copyright and trade secret
 *  laws and treaty provisions. No part of the Material may be used, copied,
 *  reproduced, modified, published, uploaded, posted, transmitted, distributed,
 *  or disclosed in any way without Marvell's prior express written permission.
 *
 *  No license under any patent, copyright, trade secret or other intellectual
 *  property right is granted to or conferred upon you by disclosure or delivery
 *  of the Materials, either expressly, by implication, inducement, estoppel or
 *  otherwise. Any license under such intellectual property rights must be
 *  express and approved by Marvell in writing.
 *
 */

/******************************************************
Change log:
    03/07/2014: Initial version
******************************************************/
#include "wltypes.h"
#include "rc4_rom.h"
#include "hostsa_ext_def.h"
#include "authenticator.h"

typedef struct rc4_key {
	unsigned char state[256];
	unsigned char x;
	unsigned char y;
} rc4_key;

static rc4_key rc4key;

static void swap_byte(unsigned char *a, unsigned char *b);

void
prepare_key(unsigned char *key_data_ptr, int key_data_len, rc4_key *key)
{
	unsigned char index1;
	unsigned char index2;
	unsigned char *state;
	short counter;

	state = &key->state[0];
	for (counter = 0; counter < 256; counter++) {
		state[counter] = counter;
	}
	key->x = 0;
	key->y = 0;
	index1 = 0;
	index2 = 0;
	for (counter = 0; counter < 256; counter++) {
		index2 = (key_data_ptr[index1] + state[counter] + index2) % 256;
		swap_byte(&state[counter], &state[index2]);

		index1 = (index1 + 1) % key_data_len;
	}
}

void
rc4(unsigned char *buffer_ptr, int buffer_len, int skip, rc4_key *key)
{
	unsigned char x;
	unsigned char y;
	unsigned char *state;
	unsigned char xorIndex;
	short counter;

	x = key->x;
	y = key->y;

	state = &key->state[0];

	for (counter = 0; counter < skip; counter++) {
		x = (x + 1) % 256;
		y = (state[x] + y) % 256;
		swap_byte(&state[x], &state[y]);
	}

	for (counter = 0; counter < buffer_len; counter++) {
		x = (x + 1) % 256;
		y = (state[x] + y) % 256;
		swap_byte(&state[x], &state[y]);

		xorIndex = (state[x] + state[y]) % 256;

		buffer_ptr[counter] ^= state[xorIndex];
	}
	key->x = x;
	key->y = y;
}

static void
swap_byte(unsigned char *a, unsigned char *b)
{
	unsigned char swapByte;

	swapByte = *a;
	*a = *b;
	*b = swapByte;
}

void
RC4_Encrypt(void *priv, unsigned char *Encr_Key,
	    unsigned char *IV,
	    unsigned short iv_length,
	    unsigned char *Data,
	    unsigned short data_length, unsigned short skipBytes)
{
	phostsa_private psapriv = (phostsa_private)priv;
	hostsa_util_fns *util_fns = &psapriv->util_fns;
	unsigned char key[32];

	if (iv_length + 16 > sizeof(key)) {
		return;
	}

	memcpy(util_fns, key, IV, iv_length);
	memcpy(util_fns, key + iv_length, Encr_Key, 16);

	prepare_key(key, iv_length + 16, &rc4key);
	rc4(Data, data_length, skipBytes, &rc4key);
}
