/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Bon-gyu, KOO <freestyle@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define SOC_PIN_BANK_EINTN(pins, reg, id)		\
	{						\
		.pctl_offset	= reg,			\
		.nr_pins	= pins,			\
		.eint_type	= EINT_TYPE_NONE,	\
		.name		= id			\
	}

#define SOC_PIN_BANK_EINTG(pins, reg, id)	\
	{							\
		.pctl_offset	= reg,				\
		.nr_pins	= pins,				\
		.eint_type	= EINT_TYPE_GPIO,		\
		.name		= id				\
	}

#define SOC_PIN_BANK_EINTW(pins, reg, id)	\
	{							\
		.pctl_offset	= reg,				\
		.nr_pins	= pins,				\
		.eint_type	= EINT_TYPE_WKUP,		\
		.name		= id				\
	}


#define GPIO_INT_OUT (0x04)
#define GPIO_INT_MODE0 (0x08) /* 0x08,0x0C */
#define GPIO_INT_MODE1 (0x28)
#define GPIO_INT_ENB (0x10)
#define GPIO_INT_STATUS (0x14)
#define GPIO_INT_ALT (0x20) /* 0x20,0x24 */
#define GPIO_INT_DET (0x3C)

#define ALIVE_MOD_RESET (0x04) /* detect mode reset */
#define ALIVE_MOD_SET (0x08)   /* detect mode set */
#define ALIVE_MOD_READ (0x0C)  /* detect mode read */
#define ALIVE_DET_RESET (0x4C)
#define ALIVE_DET_SET (0x50)
#define ALIVE_DET_READ (0x54)
#define ALIVE_INT_RESET (0x58)    /* interrupt reset : disable */
#define ALIVE_INT_SET (0x5C)      /* interrupt set   : enable */
#define ALIVE_INT_SET_READ (0x60) /* interrupt set read */
#define ALIVE_INT_STATUS (0x64)   /* interrupt detect pending and clear */
#define ALIVE_OUT_RESET (0x74)
#define ALIVE_OUT_SET (0x78)
#define ALIVE_OUT_READ (0x7C)

/* alive interrupt detect type */
#define NX_ALIVE_DETECTMODE_ASYNC_LOWLEVEL 0
#define NX_ALIVE_DETECTMODE_ASYNC_HIGHLEVEL 1
#define NX_ALIVE_DETECTMODE_SYNC_FALLINGEDGE 2
#define NX_ALIVE_DETECTMODE_SYNC_RISINGEDGE 3
#define NX_ALIVE_DETECTMODE_SYNC_LOWLEVEL 4
#define NX_ALIVE_DETECTMODE_SYNC_HIGHLEVEL 5

/* gpio interrupt detect type */
#define NX_GPIO_INTMODE_LOWLEVEL 0
#define NX_GPIO_INTMODE_HIGHLEVEL 1
#define NX_GPIO_INTMODE_FALLINGEDGE 2
#define NX_GPIO_INTMODE_RISINGEDGE 3
#define NX_GPIO_INTMODE_BOTHEDGE 4

#define NX_GPIO_PADFUNC_0 0
#define NX_GPIO_PADFUNC_1 1
#define NX_GPIO_PADFUNC_2 2
#define NX_GPIO_PADFUNC_3 3

#define NX_GPIO_DRVSTRENGTH_0 0
#define NX_GPIO_DRVSTRENGTH_1 1
#define NX_GPIO_DRVSTRENGTH_2 2
#define NX_GPIO_DRVSTRENGTH_3 3

#define NX_GPIO_PULL_DOWN 0
#define NX_GPIO_PULL_UP 1
#define NX_GPIO_PULL_OFF 2

#ifdef CONFIG_ARM64
#define ARM_DMB() dmb(sy)
#else
#define ARM_DMB() dmb()
#endif


