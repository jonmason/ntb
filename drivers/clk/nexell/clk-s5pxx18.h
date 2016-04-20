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

#ifndef _CLK_S5PXX18_H
#define _CLK_S5PXX18_H

#define I_PLL0_BIT (0)
#define I_PLL1_BIT (1)
#define I_PLL2_BIT (2)
#define I_PLL3_BIT (3)
#define I_EXT1_BIT (4)
#define I_EXT2_BIT (5)
#define I_CLKn_BIT (7)

#define I_CLOCK_NUM 6 /* PLL0, PLL1, PLL2, PLL3, EXT1, EXT2 */

#ifdef CONFIG_ARM_NEXELL_CPUFREQ
#define I_EXECEPT_CLK (1 << CONFIG_NEXELL_CPUFREQ_PLLDEV)
#else
#define I_EXECEPT_CLK (0)
#endif

#define I_CLOCK_MASK (((1 << I_CLOCK_NUM) - 1) & ~I_EXECEPT_CLK)
/*
 * clock
 */
#define CLK_CPU_PLL0 "sys-pll0"
#define CLK_CPU_PLL1 "sys-pll1"
#define CLK_CPU_PLL2 "sys-pll2"
#define CLK_CPU_PLL3 "sys-pll3"
#define CLK_CPU_FCLK "sys-cfclk"
#define CLK_CPU_HCLK "sys-chclk"
#define CLK_MEM_FCLK "sys-mfclk"
#define CLK_MEM_DCLK "sys-mdclk"
#define CLK_MEM_BCLK "sys-mbclk"
#define CLK_MEM_PCLK "sys-mpclk"
#define CLK_BUS_BCLK "sys-bbclk"
#define CLK_BUS_PCLK "sys-bpclk"
#define CLK_VPU_BCLK "sys-vpubclk"
#define CLK_VPU_PCLK "sys-vpupclk"
#define CLK_DIS_BCLK "sys-disbclk"
#define CLK_DIS_PCLK "sys-disspclk"
#define CLK_CCI_BCLK "sys-ccibclk"
#define CLK_CCI_PCLK "sys-ccipclk"
#define CLK_G3D_BCLK "sys-g3dbclk"

#define CLK_ID_TIMER_1 0
#define CLK_ID_TIMER_2 1
#define CLK_ID_TIMER_3 2
#define CLK_ID_PWM_1 3
#define CLK_ID_PWM_2 4
#define CLK_ID_PWM_3 5
#define CLK_ID_I2C_0 6
#define CLK_ID_I2C_1 7
#define CLK_ID_I2C_2 8
#define CLK_ID_MIPI 9
#define CLK_ID_GMAC 10 /* External Clock 1 */
#define CLK_ID_SPDIF_TX 11
#define CLK_ID_MPEGTSI 12
#define CLK_ID_PWM_0 13
#define CLK_ID_TIMER_0 14
#define CLK_ID_I2S_0 15 /* External Clock 1 */
#define CLK_ID_I2S_1 16 /* External Clock 1 */
#define CLK_ID_I2S_2 17 /* External Clock 1 */
#define CLK_ID_SDHC_0 18
#define CLK_ID_SDHC_1 19
#define CLK_ID_SDHC_2 20
#define CLK_ID_VR 21
#define CLK_ID_UART_0 22 /* UART0_MODULE */
#define CLK_ID_UART_2 23 /* UART1_MODULE */
#define CLK_ID_UART_1 24 /* pl01115_Uart_modem_MODULE  */
#define CLK_ID_UART_3 25 /* pl01115_Uart_nodma0_MODULE */
#define CLK_ID_UART_4 26 /* pl01115_Uart_nodma1_MODULE */
#define CLK_ID_UART_5 27 /* pl01115_Uart_nodma2_MODULE */
#define CLK_ID_DIT 28
#define CLK_ID_PPM 29
#define CLK_ID_VIP_0 30    /* External Clock 1 */
#define CLK_ID_VIP_1 31    /* External Clock 1, 2 */
#define CLK_ID_USB2HOST 32 /* External Clock 2 */
#define CLK_ID_CODA 33
#define CLK_ID_CRYPTO 34
#define CLK_ID_SCALER 35
#define CLK_ID_PDM 36
#define CLK_ID_SPI_0 37
#define CLK_ID_SPI_1 38
#define CLK_ID_SPI_2 39
#define CLK_ID_MAX 39
#define CLK_ID_USBOTG 40 /* Shared with USB2HOST */

#define I_PLL0_BIT (0)
#define I_PLL1_BIT (1)
#define I_PLL2_BIT (2)
#define I_PLL3_BIT (3)
#define I_EXT1_BIT (4)
#define I_EXT2_BIT (5)
#define I_CLKn_BIT (7)

#define I_PLL0 (1 << I_PLL0_BIT)
#define I_PLL1 (1 << I_PLL1_BIT)
#define I_PLL2 (1 << I_PLL2_BIT)
#define I_PLL3 (1 << I_PLL3_BIT)
#define I_EXTCLK1 (1 << I_EXT1_BIT)
#define I_EXTCLK2 (1 << I_EXT2_BIT)

#define I_PLL_0_1 (I_PLL0 | I_PLL1)
#define I_PLL_0_2 (I_PLL_0_1 | I_PLL2)
#define I_PLL_0_3 (I_PLL_0_2 | I_PLL3)
#define I_CLKnOUT (0)

#define I_PCLK (1 << 8)
#define I_BCLK (1 << 9)
#define I_GATE_PCLK (1 << 12)
#define I_GATE_BCLK (1 << 13)
#define I_PCLK_MASK (I_GATE_PCLK | I_PCLK)
#define I_BCLK_MASK (I_GATE_BCLK | I_BCLK)

#define CLK_INPUT_TIMER (I_PLL_0_2)
#define CLK_INPUT_UART (I_PLL_0_2)
#define CLK_INPUT_PWM (I_PLL_0_2)
#define CLK_INPUT_I2C (I_GATE_PCLK)
#define CLK_INPUT_SDHC (I_PLL_0_2 | I_GATE_PCLK)
#define CLK_INPUT_I2S (I_PLL_0_3 | I_EXTCLK1)
#define CLK_INPUT_I2S_IN1 (I_CLKnOUT)
#define CLK_INPUT_SPI (I_PLL_0_2)
#define CLK_INPUT_VIP0 (I_PLL_0_3 | I_EXTCLK1 | I_GATE_BCLK)
#define CLK_INPUT_VIP1 (I_PLL_0_3 | I_EXTCLK1 | I_EXTCLK2 | I_GATE_BCLK)
#define CLK_INPUT_MIPI (I_PLL_0_2)
#define CLK_INPUT_GMAC (I_PLL_0_3 | I_EXTCLK1)
#define CLK_INPUT_GMAC_IN1 (I_CLKnOUT)
#define CLK_INPUT_SPDIFTX (I_PLL_0_2)
#define CLK_INPUT_MPEGTS (I_GATE_BCLK)
#define CLK_INPUT_VR (I_GATE_BCLK)
#define CLK_INPUT_DIT (I_GATE_BCLK)
#define CLK_INPUT_PPM (I_PLL_0_2)
#define CLK_INPUT_EHCI (I_PLL_0_3)
#define CLK_INPUT_EHCI_IN1 (I_PLL_0_3 | I_EXTCLK1)
#define CLK_INPUT_VPU (I_GATE_PCLK | I_GATE_BCLK)
#define CLK_INPUT_CRYPTO (I_GATE_PCLK)
#define CLK_INPUT_SCALER (I_GATE_BCLK)
#define CLK_INPUT_OTG (I_PLL_0_3)
#define CLK_INPUT_OTG_IN1 (I_PLL_0_3 | I_EXTCLK1)
#define CLK_INPUT_PDM (I_GATE_PCLK)

#endif
