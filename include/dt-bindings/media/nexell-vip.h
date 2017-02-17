/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
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

#ifndef __DT_BINDINGS_MEDIA_NEXELL_VIP_H__
#define __DT_BINDINGS_MEDIA_NEXELL_VIP_H__

/* capture hw interface type */
#define NX_CAPTURE_INTERFACE_PARALLEL		0
#define NX_CAPTURE_INTERFACE_MIPI_CSI		1

/* camera sensor <--> vip data order(yuv422) */
#define NX_VIN_CBY0CRY1				0
#define NX_VIN_CRY1CBY0				1
#define NX_VIN_Y0CBY1CR				2
#define NX_VIN_Y1CRY0CB				3

/* camera sensor configuration interface */
#define NX_CAPTURE_SENSOR_I2C			0
#define NX_CAPTURE_SENSOR_SPI			1
#define NX_CAPTURE_SENSOR_LOOPBACK		2

/* camera enable sequence marker */
#define NX_ACTION_START				0x12345678
#define NX_ACTION_END				0x87654321
#define NX_ACTION_TYPE_GPIO			0xffff0001
#define NX_ACTION_TYPE_PMIC			0xffff0002
#define NX_ACTION_TYPE_CLOCK			0xffff0003

#endif /* __DT_BINDINGS_MEDIA_NEXELL_V4L2_H__ */
