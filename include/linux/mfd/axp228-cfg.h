/*
 * axp228-cfg.h  --  PMIC driver for the X-Powers AXP228
 *
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongshin, Park <pjsin865@nexell.co.kr>
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

#ifndef __LINUX_AXP_CFG_H_
#define __LINUX_AXP_CFG_H_

/* i2c slave address */
#define AXP_DEVICES_ADDR (0x68 >> 1)

#define BATCAP 3000 /* CFG_BATTERY_CAP */

#define BATRDC 137 /* 100 */

#define AXP_VOL_MAX 1

#define CHGEN 1

#define STACHGCUR (1500 * 1000) /* AXP22:300~2550,100/step */
#define EARCHGCUR (1500 * 1000) /* AXP22:300~2550,100/step */
#define SUSCHGCUR (1500 * 1000) /* AXP22:300~2550,100/step */
#define CLSCHGCUR (1500 * 1000) /* AXP22:300~2550,100/step */

/* AC current charge */
/*#define AC_CHARGE_CURRENT	1500*1000*/
/* AC current limit */
#define AC_LIMIT_CURRENT_1500 (1500 * 1000)
#define AC_LIMIT_CURRENT_500 (500 * 1000)

#define CHGVOL (4200 * 1000) /* AXP22:4100/4220/4200/4240 */

#define ENDCHGRATE 10 /* AXP22:10\15 */

#define SHUTDOWNVOL 2600

#define ADCFREQ 100 /* AXP22:100\200\400\800 */

#define CHGPRETIME 50 /* AXP22:40\50\60\70 */

#define CHGCSTTIME 480 /* AXP22:360\480\600\720 */

/* pok open time set */
#define PEKOPEN 1000 /* AXP22:128/1000/2000/3000 */

/* pok long time set*/
#define PEKLONG 1500 /* AXP22:1000/1500/2000/2500 */

/* pek offlevel poweroff en set*/
#define PEKOFFEN 1

/*Init offlevel restart or not */
#define PEKOFFRESTART 0

/* pek delay set */
#define PEKDELAY 32 /* AXP20:8/16/32/64 */

/*  pek offlevel time set */
#define PEKOFF 6000 /* AXP22:4000/6000/8000/10000 */

/* Init PMU Over Temperature protection*/
#define OTPOFFEN 0

#define USBVOLLIMEN 1

#define USBVOLLIM 4700 /* AXP22:4000~4700£¬100/step */

#define USBVOLLIMPC 4700 /* AXP22:4000~4700£¬100/step */

#define USBCURLIMEN 0

#define USBCURLIM 500 /* AXP22:500/900 */

#define USBCURLIMPC 0 /* AXP22:500/900 */

/* Init IRQ wakeup en*/
#define IRQWAKEUP 0

/* Init N_VBUSEN status*/
#define VBUSEN 1

/* Init InShort status*/
#define VBUSACINSHORT 0

/* Init CHGLED function*/
#define CHGLEDFUN 1

/* set CHGLED Indication Type*/
#define CHGLEDTYPE 0

/* Init battery capacity correct function*/
#define BATCAPCORRENT 1

/* Init battery regulator enable or not when charge finish*/
#define BATREGUEN 0

#define BATDET 1

/* Init 16's Reset PMU en */
#define PMURESET 0

/* set lowe power warning level */
#define BATLOWLV1 15 /* AXP22:5%~20% */

/* set lowe power shutdown level */
#define BATLOWLV2 0 /* AXP22:0%~15% */

#define ABS(x) ((x) > 0 ? (x) : -(x))

/*AXP GPIO start NUM, */
#define AXP22_NR_BASE 100

/*AXP GPIO NUM, LCD power VBUS driver pin*/
#define AXP22_NR 5

/*  AXP IRQ */
#define AXP_IRQ_USBLO AXP22_IRQ_USBLO
#define AXP_IRQ_USBRE AXP22_IRQ_USBRE
#define AXP_IRQ_USBIN AXP22_IRQ_USBIN
#define AXP_IRQ_USBOV AXP22_IRQ_USBOV
#define AXP_IRQ_ACRE AXP22_IRQ_ACRE
#define AXP_IRQ_ACIN AXP22_IRQ_ACIN
#define AXP_IRQ_ACOV AXP22_IRQ_ACOV
#define AXP_IRQ_TEMLO AXP22_IRQ_TEMLO
#define AXP_IRQ_TEMOV AXP22_IRQ_TEMOV
#define AXP_IRQ_CHAOV AXP22_IRQ_CHAOV
#define AXP_IRQ_CHAST AXP22_IRQ_CHAST
#define AXP_IRQ_BATATOU AXP22_IRQ_BATATOU
#define AXP_IRQ_BATATIN AXP22_IRQ_BATATIN
#define AXP_IRQ_BATRE AXP22_IRQ_BATRE
#define AXP_IRQ_BATIN AXP22_IRQ_BATIN
#define AXP_IRQ_PEKLO AXP22_IRQ_POKLO
#define AXP_IRQ_PEKSH AXP22_IRQ_POKSH

#define AXP_IRQ_CHACURLO AXP22_IRQ_CHACURLO
#define AXP_IRQ_ICTEMOV AXP22_IRQ_ICTEMOV
#define AXP_IRQ_EXTLOWARN2 AXP22_IRQ_EXTLOWARN2
#define AXP_IRQ_EXTLOWARN1 AXP22_IRQ_EXTLOWARN1

#define AXP_IRQ_GPIO0TG AXP22_IRQ_GPIO0TG
#define AXP_IRQ_GPIO1TG AXP22_IRQ_GPIO1TG

#define AXP_IRQ_PEKFE AXP22_IRQ_PEKFE
#define AXP_IRQ_PEKRE AXP22_IRQ_PEKRE
#define AXP_IRQ_TIMER AXP22_IRQ_TIMER

static const uint64_t AXP22_NOTIFIER_ON =
	(AXP_IRQ_USBIN | AXP_IRQ_USBRE |
	AXP_IRQ_ACIN | AXP_IRQ_ACRE |
	AXP_IRQ_BATIN | AXP_IRQ_BATRE |
	AXP_IRQ_CHAST | AXP_IRQ_CHAOV |
	AXP_IRQ_PEKLO | AXP_IRQ_PEKSH |
	(uint64_t)AXP_IRQ_PEKFE |
	(uint64_t)AXP_IRQ_PEKRE |
	(uint64_t)AXP_IRQ_EXTLOWARN2 |
	(uint64_t)AXP_IRQ_EXTLOWARN1);

#define POWER_START 0

/* debug log */
/* #define ENABLE_DEBUG */
/* #define DBG_AXP_PSY */

#endif
