/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Youngbok, Park <ybpark@nexell.co.kr>
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

#ifndef _S5P4418_IRQ_H
#define _S5P4418_IRQ_H

/*
 * GIC remmap hwirq to hwirq+16+ (0~16: For SGI, 16~31:PPI)
 */
#define IRQ_OFFSET			(0)
#define IRQ_MCUSTOP			(IRQ_OFFSET +  0)
#define IRQ_DMA0			(IRQ_OFFSET +  1)
#define IRQ_DMA1			(IRQ_OFFSET +  2)
#define IRQ_INTREQPWR			(IRQ_OFFSET +  3)
#define IRQ_ALIVE			(IRQ_OFFSET +  4)
#define IRQ_RTC				(IRQ_OFFSET +  5)
#define IRQ_UART1			(IRQ_OFFSET +  6)
#define IRQ_UART0			(IRQ_OFFSET +  7)
#define IRQ_UART2			(IRQ_OFFSET +  8)
#define IRQ_UART3			(IRQ_OFFSET +  9)
#define IRQ_UART4			(IRQ_OFFSET + 10)
#define IRQ_UART5			(IRQ_OFFSET + 11)
#define IRQ_SSP0			(IRQ_OFFSET + 12)
#define IRQ_SSP1			(IRQ_OFFSET + 13)
#define IRQ_SSP2			(IRQ_OFFSET + 14)
#define IRQ_I2C0			(IRQ_OFFSET + 15)
#define IRQ_I2C1			(IRQ_OFFSET + 16)
#define IRQ_I2C2			(IRQ_OFFSET + 17)
#define IRQ_DEINTERLACE			(IRQ_OFFSET + 18)
#define IRQ_SCALER			(IRQ_OFFSET + 19)
#define IRQ_AC97			(IRQ_OFFSET + 20)
#define IRQ_SPDIFRX			(IRQ_OFFSET + 21)
#define IRQ_SPDIFTX			(IRQ_OFFSET + 22)
#define IRQ_TIMER0			(IRQ_OFFSET + 23)
#define IRQ_TIMER1			(IRQ_OFFSET + 24)
#define IRQ_TIMER2			(IRQ_OFFSET + 25)
#define IRQ_TIMER3			(IRQ_OFFSET + 26)
#define IRQ_PWM_INT0			(IRQ_OFFSET + 27)
#define IRQ_PWM_INT1			(IRQ_OFFSET + 28)
#define IRQ_PWM_INT2			(IRQ_OFFSET + 29)
#define IRQ_PWM_INT3			(IRQ_OFFSET + 30)
#define IRQ_WDT				(IRQ_OFFSET + 31)
#define IRQ_MPEGTSI			(IRQ_OFFSET + 32)
#define IRQ_DPC_P			(IRQ_OFFSET + 33)
#define IRQ_DPC_S			(IRQ_OFFSET + 34)
#define IRQ_RESCONV			(IRQ_OFFSET + 35)
#define IRQ_HDMI			(IRQ_OFFSET + 36)
#define IRQ_VIP0			(IRQ_OFFSET + 37)
#define IRQ_VIP1			(IRQ_OFFSET + 38)
#define IRQ_MIPI			(IRQ_OFFSET + 39)
#define IRQ_VR				(IRQ_OFFSET + 40)
#define IRQ_ADC				(IRQ_OFFSET + 41)
#define IRQ_PPM				(IRQ_OFFSET + 42)
#define IRQ_SDMMC0			(IRQ_OFFSET + 43)
#define IRQ_SDMMC1			(IRQ_OFFSET + 44)
#define IRQ_SDMMC2			(IRQ_OFFSET + 45)
#define IRQ_CODA960_HOST		(IRQ_OFFSET + 46)
#define IRQ_CODA960_JPG			(IRQ_OFFSET + 47)
#define IRQ_GMAC			(IRQ_OFFSET + 48)
#define IRQ_USB20OTG			(IRQ_OFFSET + 49)
#define IRQ_USB20HOST			(IRQ_OFFSET + 50)
#define IRQ_CAN0			(IRQ_OFFSET + 51)
#define IRQ_CAN1			(IRQ_OFFSET + 52)
#define IRQ_GPIOA			(IRQ_OFFSET + 53)
#define IRQ_GPIOB			(IRQ_OFFSET + 54)
#define IRQ_GPIOC			(IRQ_OFFSET + 55)
#define IRQ_GPIOD			(IRQ_OFFSET + 56)
#define IRQ_GPIOE			(IRQ_OFFSET + 57)
#define IRQ_CRYPTO			(IRQ_OFFSET + 58)
#define IRQ_PDM				(IRQ_OFFSET + 59)

#define IRQ_PHY_NR			(64)		/* GIC: GIC_DIST_CTR */

/*
 * gpio interrupt Number 160 (64~223)
 */
#define IRQ_GPIO_START			IRQ_PHY_NR
#define IRQ_GPIO_END			(IRQ_GPIO_START + 32 * 5)

#define IRQ_GPIO_A_START		(IRQ_GPIO_START + 32*0)
#define IRQ_GPIO_B_START		(IRQ_GPIO_START + 32*1)
#define IRQ_GPIO_C_START		(IRQ_GPIO_START + 32*2)
#define IRQ_GPIO_D_START		(IRQ_GPIO_START + 32*3)
#define IRQ_GPIO_E_START		(IRQ_GPIO_START + 32*4)

#define IRQ_GPIO_NR			(IRQ_GPIO_END-IRQ_GPIO_START)

/*
 * ALIVE Interrupt Number 8 (224~231)
 */
#define IRQ_ALIVE_START			IRQ_GPIO_END
#define IRQ_ALIVE_END			(IRQ_ALIVE_START + 8)

#define IRQ_ALIVE_0			(IRQ_ALIVE_START + 0)
#define IRQ_ALIVE_1			(IRQ_ALIVE_START + 1)
#define IRQ_ALIVE_2			(IRQ_ALIVE_START + 2)
#define IRQ_ALIVE_3			(IRQ_ALIVE_START + 3)
#define IRQ_ALIVE_4			(IRQ_ALIVE_START + 4)
#define IRQ_ALIVE_5			(IRQ_ALIVE_START + 5)
#define IRQ_ALIVE_6			(IRQ_ALIVE_START + 6)
#define IRQ_ALIVE_7			(IRQ_ALIVE_START + 7)

#define IRQ_ALIVE_NR			(IRQ_ALIVE_END-IRQ_ALIVE_START)
/*
 * GIC PPI Interrupt
 */
#define IRQ_GIC_START			(0)
#define IRQ_GIC_PPI_PVT			(IRQ_GIC_START + 13)
#define IRQ_GIC_PPI_WDT			(IRQ_GIC_START + 14)
#define IRQ_GIC_PPI_VIC			(IRQ_GIC_START + 15)
#define IRQ_GIC_END			(IRQ_GIC_START + 16)

/*
 * MAX(Physical+Virtual) Interrupt Number
 */
#define IRQ_RESERVED_START		IRQ_ALIVE_END
#define IRQ_RESERVED_NR			72

#define IRQ_TOTAL_NR			(IRQ_RESERVED_START + IRQ_RESERVED_NR)

#endif
