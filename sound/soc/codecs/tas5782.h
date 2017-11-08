/*
 * TAS571x amplifier audio driver
 *
 * Copyright (C) 2015 Google, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _TAS5782_H
#define _TAS5782_H

/* device registers */
#define	TAS5782_DEV_ID			0x58

#define TAS5782_SOFT_MUTE_REG		0x03
#define TAS5782_SOFT_MUTE_RIGHT_SHIFT	0
#define TAS5782_SOFT_MUTE_LEFT_SHIFT	3

#define TAS5782_DIGITAL_VOL_REG		0x3C
#define TAS5782_DIGITAL_LEFT_VOL_REG		0x3D
#define TAS5782_DIGITAL_RIGHT_VOL_REG		0x3E

#include "tas5782m.h"


#endif /* _TAS5782_H */
