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

#ifndef _DT_BINDINGS_NXS_REG_BASE_H
#define _DT_BINDINGS_NXS_REG_BASE_H

#define NXS_BASEADDR			0x20c00000
#define NXS_REGSIZE			0x8000

#define ISSDIRTY_OFFSET			0x0000
#define ISSDIRTY_SIZE			0x0020

#define CSI2NXS_OFFSET			0x0040
#define CSI2NXS_SIZE			0x0080

#define CSC_SIZE			0x0040
#define CSC0_OFFSET			0x0200
#define CSC1_OFFSET			0x0240
#define CSC2_OFFSET			0x0280
#define CSC3_OFFSET			0x02c0

#define CROPPER_SIZE			0x0020
#define CROPPER0_OFFSET			0x0a00
#define CROPPER1_OFFSET			0x0a20

#define SCALER_SIZE			0x0300
#define SCALER0_OFFSET			0x0c00
#define SCALER1_OFFSET			0x0f00
#define SCALER2_OFFSET			0x1200

#define DISPSP_SIZE			0x0008
#define DISPSP_OFFSET			0x14f0

#define MULTITAP_SIZE			0x0020
#define MULTITAP0_OFFSET		0x1500
#define MULTITAP1_OFFSET		0x1520
#define MULTITAP2_OFFSET		0x1540
#define MULTITAP3_OFFSET		0x1560

#define HUE_SIZE			0x0080
#define HUE0_OFFSET			0x1580
#define HUE1_OFFSET			0x1600

#define GAMMA_SIZE			0x0020
#define GAMMA0_OFFSET			0x1680
#define GAMMA1_OFFSET			0x1700

#define DISP2ISP_SIZE			0x0004
#define DISP2ISP_OFFSET			0x1780

#define ISP2DISP_SIZE			0x0004
#define ISP2DISP0_OFFSET		0x1784
#define ISP2DISP1_OFFSET		0x1788

#define DMAR_SIZE			0x0200
#define DMAR0_OFFSET			0x1800
#define DMAR1_OFFSET			0x1a00
#define DMAR2_OFFSET			0x1c00
#define DMAR3_OFFSET			0x1e00
#define DMAR4_OFFSET			0x2000
#define DMAR5_OFFSET			0x2200
#define DMAR6_OFFSET			0x2400
#define DMAR7_OFFSET			0x2600
#define DMAR8_OFFSET			0x2800
#define DMAR9_OFFSET			0x2a00

#define FIFO_SIZE			0x0010
#define FIFO0_OFFSET			0x3d00
#define FIFO1_OFFSET			0x3d10
#define FIFO2_OFFSET			0x3d20
#define FIFO3_OFFSET			0x3d30
#define FIFO4_OFFSET			0x3d40
#define FIFO5_OFFSET			0x3d50
#define FIFO6_OFFSET			0x3d60
#define FIFO7_OFFSET			0x3d70
#define FIFO8_OFFSET			0x3d80
#define FIFO9_OFFSET			0x3d90
#define FIFO10_OFFSET			0x3da0
#define FIFO11_OFFSET			0x3db0

#define TPGEN_SIZE			0x0030
#define TPGEN_OFFSET			0x3e00

#define BOTTOM_SIZE			0x0020
#define BOTTOM0_OFFSET			0x4000
#define BOTTOM1_OFFSET			0x4020

#define BLENDER_SIZE			0x0080
#define BLENDER0_OFFSET			0x4080
#define BLENDER1_OFFSET			0x4100
#define BLENDER2_OFFSET			0x4180
#define BLENDER3_OFFSET			0x4200
#define BLENDER4_OFFSET			0x4280
#define BLENDER5_OFFSET			0x4300
#define BLENDER6_OFFSET			0x4380
#define BLENDER7_OFFSET			0x4400

#define DMAW_SIZE			0x0200
#define DMAW0_OFFSET			0x5000
#define DMAW1_OFFSET			0x5200
#define DMAW2_OFFSET			0x5400
#define DMAW3_OFFSET			0x5600
#define DMAW4_OFFSET			0x5800
#define DMAW5_OFFSET			0x5a00
#define DMAW6_OFFSET			0x5c00
#define DMAW7_OFFSET			0x5e00
#define DMAW8_OFFSET			0x6000
#define DMAW9_OFFSET			0x6200
#define DMAW10_OFFSET			0x6400
#define DMAW11_OFFSET			0x6600

#define CSI2AXI_OFFSET			0x7800
#define CSI2AXI_SIZE			0x0034

#define NXS2DSI_REGBASE			0x20e60000
#define NXS2DSI_SIZE			0x0200

#define NXS2DSI_I80_SIZE		0x0020
#define NXS2DSI0_I80_REGBASE		0x20e68000
#define NXS2DSI1_I80_REGBASE		0x20e78000

#define NXS2HDMI_REGBASE		0x20c30000
#define NXS2HDMI_SIZE			0x0110

#define HDMICEC_REGBASE			0x20011000
#define HDMICEC_SIZE			0x0200

#define HDMIPHY_REGBASE			0x20ed00b0
#define HDMIPHY_SIZE			0x0038

#define HDMILINK_REGBASE		0x20f00000
#define HDMILINK_SIZE			0x6044

#define NXS_REGSIZE_DPC			0x20000
#define NXS_REGBASE_DPC(index)		(0x20c80000 + \
					 (NXS_REGSIZE_DPC * index))

#define NXS_REGSIZE_LVDS		0x10000
#define NXS_REGBASE_LVDS(index)		(0x20cc0000 + \
					 (NXS_REGSIZE_LVDS * index * 2))

#define VIP_REGBASE			0x20b10000
#define VIP_PRESCALER_REGBASE		0x20b20000

#define NXS_REGSIZE_VIP			0x200
#define NXS_REGSIZE_VIP_PRESCALER	0x100

#define NXS_REGBASE_VIP(index)		(VIP_REGBASE + \
					 (NXS_REGSIZE_VIP * index))
#define NXS_REGBASE_VIP_PRESCALER(index) (VIP_PRESCALER_REGBASE + \
					  (NXS_REGSIZE_VIP * index))

#define NXS_REGSIZE_MIPICSI		0x10000
#define NXS_REGBASE_MIPICSI(index)	(0x20e20000 + \
					 (NXS_REGSIZE_MIPICSI * index))

#define NXS_REGSIZE_MIPIDSI		0x30000
#define NXS_REGBASE_MIPIDSI(index)	(0x20e50000 + \
					 (NXS_REGSIZE_MIPIDSI * index))

#define NXS_REGSIZE_MAPCONV		0x10000
#define NXS_REGBASE_MAPCONV(index)	(0x20cd0000 + \
					 (NXS_REGSIZE_MAPCONV * index))

#define NXS_REGSIZE_HDMI		0x140000
#define NXS_REGBASE_HDMI(index)		(0x20ec0000 + \
					 (NXS_REGSIZE_HDMI * index))

#define NXS_REGBASE_ISP			0x20900000
#define NXS_REGBASE_ISPCORE		0x20920000

#define NXS_REGSIZE_ISP			0x104
#define NXS_REGSIZE_ISPCORE		0x7480

#endif
