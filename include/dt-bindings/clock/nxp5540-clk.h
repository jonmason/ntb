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

#ifndef _NXP5430_CMU_H
#define _NXP5430_CMU_H

/*
 * clock
 */
#define CLK_PLL_BASE 		(PHYS_BASE_SYSCTRLTOP)
#define CLK_PLL_0 0
#define CLK_PLL_1 1
#define CLK_PLL_2 2
#define CLK_PLL_3 3
#define CLK_PLL_4 4
#define CLK_PLL_5 5
#define CLK_PLL_6 6
#define CLK_PLL_7 7

#define CLK_CPU_PLL0 "sys-pll0"
#define CLK_CPU_PLL1 "sys-pll1"
#define CLK_CPU_PLL2 "sys-pll2"
#define CLK_CPU_PLL3 "sys-pll3"
#define CLK_CPU_PLL4 "sys-pll4"
#define CLK_CPU_PLL5 "sys-pll5"
#define CLK_CPU_PLL6 "sys-pll6"
#define CLK_CPU_PLL7 "sys-pll7"

/*
 * CMU_SYS_SCLK
 */
#define SYS_AXI_CLK_BASE		(0x20100200)
#define DOUT_SCLK_PLL_0_BASE		(0x20100600)
#define DOUT_SCLK_PLL_1_BASE		(0x20100800)
#define DOUT_SCLK_PLL_2_BASE		(0x20100A00)
#define DOUT_SCLK_PLL_3_BASE		(0x20100c00)
#define DOUT_SCLK_PLL_4_BASE		(0x20100e00)
#define DOUT_SCLK_PLL_5_BASE		(0x20101000)
#define DOUT_SCLK_PLL_6_BASE		(0x20101200)
#define DOUT_SCLK_PLL_7_BASE		(0x20101400)
#define DOUT_SCLK_USB_OPHY_BASE		(0x20101600)
#define DOUT_SCLK_USB_HPHY_BASE		(0x20101800)
#define DOUT_SCLK_HDMI_PIXEL_BASE	(0x20101A00)
#define DOUT_SCLK_HDMI_TMDS_BASE	(0x20101C00)

#define DOUT_SCLK_PLL_0		0
#define DOUT_SCLK_PLL_1		1
#define DOUT_SCLK_PLL_2		2
#define DOUT_SCLK_PLL_3		3
#define DOUT_SCLK_PLL_4		4
#define DOUT_SCLK_PLL_5		5
#define DOUT_SCLK_PLL_6		6
#define DOUT_SCLK_PLL_7		7
#define DOUT_SCLK_USB_OPHY	8
#define DOUT_SCLK_USB_HPHY	9
#define DOUT_SCLK_HDMI_PIXEL	10
#define DOUT_SCLK_HDMI_TMDS	11

/*
 * CMU_SYS AXI_BUS
 */
#define SYS_AXI_CLK		0
#define CCI400_AXI_CLK		1
#define TIEOFF_CCI_AXI_CLK	2
#define CRYPTO_AXI_CLK		3
#define SYS_AHB_CLK		4
#define SYS_APB_CLK		5
#define TIMER_0_3_APBCLK	6
#define TIMER_4_7_APBCLK	7
#define PWM_0_3_APB_CLK		8
#define PWM_4_7_APB_CLK		9
#define PWM_8_11_APB_CLK	10
#define PWM_12_APB_CLK		11
#define SYSCTRL_APB_CLK		12
#define HPM_APB_CLK		13
#define Q_ENHANCER_APB_CLK	14
#define CYPTO_APB_CLK		15
#define WDT_APB0_CLK		16
#define WDT_POR0_CLK		17
#define WDT_APB1_CLK		18
#define WDT_POR1_CLK		19
#define TZPC_APB_CLK		20
#define ECID_APB_CLK		21
#define DMAC_0_APB_CLK		22
#define DMAC_1_APB_CLK		23
#define SDMAC_0_APB_CLK		24
#define SDMAC_1_APB_CLK		25
#define MDMAC_0_APB_CLK		26
#define SYSTIEOFF_APB_CLK	27
#define MCUS_APB_CLK		28

#define SYS_AXI_NAME		"cmu_sys_axi"
#define CCI400_AXI_NAME		"cmu_cci400"
#define TIEOFF_CCI_AXI_NAME	"cmu_tieoff_cci"
#define CRYPTO_AXI_NAME		"cmu_crypto_axi"
#define SYS_AHB_NAME		"cmu_ahb"
#define SYS_APB_NAME		"cmu_apb"
#define TIMER_0_3_APB_NAME	"cmu_timer_0_3"
#define TIMER_4_7_APB_NAME	"cmu_timer_4_7"
#define PWM_0_3_APB_NAME	"cmu_pwm_0_3"
#define PWM_4_7_APB_NAME	"cmu_pwm_4_7"
#define PWM_8_11_APB_NAME	"cmu_pwm_8_11"
#define PWM_12_APB_NAME		"cmu_pwm_12"
#define SYSCTRL_APB_NAME	"cmu_sysctrl"
#define HPM_APB_NAME		"cmu_hpm"
#define Q_ENHANCER_APB_NAME	"cmu_q_enhancer"
#define CYPTO_APB_NAME		"cmu_crypto"
#define WDT_APB0_NAME		"cmu_wdt0"
#define WDT_POR0_NAME		"cmu_wdt_por0"
#define WDT_APB1_NAME		"cmu_wdt1"
#define WDT_POR1_NAME		"cmu_wdt_por1"
#define TZPC_APB_NAME		"cmu_tpzc"
#define ECID_APB_NAME		"cmu_ecid"
#define DMAC_0_APB_NAME		"cmu_dmac0"
#define DMAC_1_APB_NAME		"cmu_dmac1"
#define SDMAC_0_APB_NAME	"cmu_sdmac0"
#define SDMAC_1_APB_NAME	"cmu_sdmac1"
#define MDMAC_0_APB_NAME	"cmu_mdmac0"
#define SYSTIEOFF_APB_NAME	"cmu_systieoff"
#define MCUS_APB_NAME		"cmu_mucs"

/*
 * CMU_SYS PERI
 */
#define PDM_APB0_NAME		"cmu_pdm_apb0"
#define PDM_APB1_NAME		"cmu_pdm_apb1"
#define PDM_APB2_NAME		"cmu_pdm_apb2"
#define PDM_APB3_NAME		"cmu_pdm_apb3"
#define AUDIO_IO_NAME		"cmu_audio_io"
#define PKA_CORE_NAME		"cmu_pka"
#define CSSYS_SRC_NAME		"cmu_cs_src"
#define MCUSTOP_NAME		"cmu_mcustop"
#define TIMER0_NAME		"cmu_timer0"
#define TIMER1_NAME		"cmu_timer1"
#define TIMER2_NAME		"cmu_timer2"
#define TIMER3_NAME		"cmu_timer3"
#define TIMER4_NAME		"cmu_timer4"
#define TIMER5_NAME		"cmu_timer5"
#define TIMER6_NAME		"cmu_timer6"
#define TIMER7_NAME		"cmu_timer7"
#define PWM_0_NAME		"cmu_pwm0"
#define PWM_1_NAME		"cmu_pwm1"
#define PWM_2_NAME		"cmu_pwm2"
#define PWM_3_NAME		"cmu_pwm3"
#define PWM_4_NAME		"cmu_pwm4"
#define PWM_5_NAME		"cmu_pwm5"
#define PWM_6_NAME		"cmu_pwm6"
#define PWM_7_NAME		"cmu_pwm7"
#define PWM_8_NAME		"cmu_pwm8"
#define PWM_9_NAME		"cmu_pwm9"
#define PWM_10_NAME		"cmu_pwm10"
#define PWM_11_NAME		"cmu_pwm11"
#define PWM_12_NAME		"cmu_pwm12"
#define PWM_13_NAME		"cmu_pwm13"
#define PWM_14_NAME		"cmu_pwm14"
#define PWM_15_NAME		"cmu_pwm15"
#define PO_0_OUT_NAME		"cmu_po_out0"
#define PO_1_OUT_NAME		"cmu_po_out1"
#define PO_2_OUT_NAME		"cmu_po_out2"
#define PO_3_OUT_NAME		"cmu_po_out3"
#define PO_4_OUT_NAME		"cmu_po_out4"
#define DMA_BUS_AXI_NAME	"cmu_dma_bus"
#define GIC400_AXI_NAME		"cmu_gic400"


#define PDM_APB0_BASE		(0x20101e00)
#define PDM_APB1_BASE		(0x20102000)
#define PDM_APB2_BASE		(0x20102200)
#define PDM_APB3_BASE		(0x20102400)
#define AUDIO_IO_BASE		(0x20102600)
#define PKA_CORE_BASE		(0x20102800)
#define CSSYS_SRC_BASE		(0x20102a00)
#define MCUSTOP_BASE		(0x20102c00)
#define TIMER0_BASE		(0x20102e00)
#define TIMER1_BASE		(0x20103000)
#define TIMER2_BASE		(0x20103200)
#define TIMER3_BASE		(0x20103400)
#define TIMER4_BASE		(0x20103600)
#define TIMER5_BASE		(0x20103800)
#define TIMER6_BASE		(0x20103a00)
#define TIMER7_BASE		(0x20103c00)
#define PWM_0_BASE		(0x20103e00)
#define PWM_1_BASE		(0x20104000)
#define PWM_2_BASE		(0x20104200)
#define PWM_3_BASE		(0x20104400)
#define PWM_4_BASE		(0x20104600)
#define PWM_5_BASE		(0x20104800)
#define PWM_6_BASE		(0x20104a00)
#define PWM_7_BASE		(0x20104c00)
#define PWM_8_BASE		(0x20104e00)
#define PWM_9_BASE		(0x20105000)
#define PWM_10_BASE		(0x20105200)
#define PWM_11_BASE		(0x20105400)
#define PWM_12_BASE		(0x20105600)
#define PWM_13_BASE		(0x20105800)
#define PWM_14_BASE		(0x20105a00)
#define PWM_15_BASE		(0x20105c00)
#define PO_0_OUT_BASE		(0x20105e00)
#define PO_1_OUT_BASE		(0x20106000)
#define PO_2_OUT_BASE		(0x20106200)
#define PO_3_OUT_BASE		(0x20106400)
#define PO_4_OUT_BASE		(0x20106600)
#define DMA_BUS_AXI_BASE	(0x20106800)
#define GIC400_AXI_BASE		(0x20106a00)

#define PDM_APB0_CLK		0
#define PDM_APB1_CLK		1
#define PDM_APB2_CLK		2
#define PDM_APB3_CLK		3
#define AUDIO_IO_CLK		4
#define PKA_CORE_CLK		5
#define CSSYS_SRC_CLK		6
#define MCUSTOP_CLK		7
#define TIMER0_TCLK		8
#define TIMER1_TCLK		9
#define TIMER2_TCLK		10
#define TIMER3_TCLK		11
#define TIMER4_TCLK		12
#define TIMER5_TCLK		13
#define TIMER6_TCLK		14
#define TIMER7_TCLK		15
#define PWM_0_TCLK		16
#define PWM_1_TCLK		17
#define PWM_2_TCLK		18
#define PWM_3_TCLK		19
#define PWM_4_TCLK		20
#define PWM_5_TCLK		21
#define PWM_6_TCLK		22
#define PWM_7_TCLK		23
#define PWM_8_TCLK		24
#define PWM_9_TCLK		25
#define PWM_10_TCLK		26
#define PWM_11_TCLK		27
#define PWM_12_TCLK		28
#define PWM_13_TCLK		29
#define PWM_14_TCLK		30
#define PWM_15_TCLK		31
#define PO_0_OUT_CLK		32
#define PO_1_OUT_CLK		33
#define PO_2_OUT_CLK		34
#define PO_3_OUT_CLK		35
#define PO_4_OUT_CLK		36
#define DMA_BUS_AXI_CLK		37
#define GIC400_AXI_CLK		38

/*
 * CMU_TBUS AXI
 */
#define TBUS_AXI_CLK		0
#define TBUS_AHB_CLK		1
#define TBUS_APB_CLK		2
#define TBUS_I2S0_APB_CLK	3
#define TBUS_I2S1_APB_CLK	4
#define TBUS_I2S2_APB_CLK	5
#define TBUS_AC97_APB_CLK	6
#define TBUS_I2C0_APB_CLK	7
#define TBUS_I2C1_APB_CLK	8

#define TBUS_AXI_BASE		(0x20110200)
#define TBUS_AHB_BASE		(0x20110200)
#define TBUS_APB_BASE		(0x20110200)
#define TBUS_I2S0_APB_BASE	(0x20110200)
#define TBUS_I2S1_APB_BASE	(0x20110200)
#define TBUS_I2S2_APB_BASE	(0x20110200)
#define TBUS_AC97_APB_BASE	(0x20110200)
#define TBUS_I2C0_APB_BASE	(0x20110200)
#define TBUS_I2C1_APB_BASE	(0x20110200)

#define TBUS_AXI_NAME		"cmu_tbus_axi"
#define TBUS_AHB_NAME		"cmu_tbus_ahb"
#define TBUS_APB_NAME		"cmu_tbus_apb"
#define TBUS_I2S0_APB_NAME	"cmu_i2s0_apb"
#define TBUS_I2S1_APB_NAME	"cmu_i2s1_apb"
#define TBUS_I2S2_APB_NAME	"cmu_i2s2_apb"
#define TBUS_AC97_APB_NAME	"cmu_ac97_apb"
#define TBUS_I2C0_APB_NAME	"cmu_i2c0_apb"
#define TBUS_I2C1_APB_NAME	"cmu_i2c1_apb"

/*
 * CMU_TBUS PERI
 */
#define I2S0_CLK		0
#define I2S1_CLK		1
#define I2S2_CLK		2
#define I2S3_CLK		3

#define I2S0_BASE		(0x20110400)
#define I2S1_BASE		(0x20110600)
#define I2S2_BASE		(0x20110800)
#define I2S3_BASE		(0x20110a00)

#define I2S0_NAME		"cmu_i2s0"
#define I2S1_NAME		"cmu_i2s1"
#define I2S2_NAME		"cmu_i2s2"
#define I2S3_NAME		"cmu_i2s3"

/*
 * CMU_LBUS AXI
 */
#define LBUS_AXI_CLK		0
#define LBUS_AHB_CLK		1
#define LBUS_GMAC_AHB_CLK	2
#define LBUS_MP2TS_AHB_CLK	3
#define LBUS_APB_CLK		4
#define LBUS_GMAC_CSR_CLK	5
#define LBUS_TIEOFF_APB_CLK	6
#define LBUS_GPIO0_APB_CLK	7
#define LBUS_GPIO1_APB_CLK	8
#define LBUS_I2C2_APB_CLK	9
#define LBUS_I2C3_APB_CLK	10
#define LBUS_I2C4_APB_CLK	11

#define LBUS_AXI_BASE		(0x20120200)
#define LBUS_AHB_BASE		(0x20120200)
#define LBUS_GMAC_AHB_BASE	(0x20120200)
#define LBUS_MP2TS_AHB_BASE	(0x20120200)
#define LBUS_APB_BASE		(0x20120200)
#define LBUS_GMAC_CSR_BASE	(0x20120200)
#define LBUS_TIEOFF_APB_BASE	(0x20120200)
#define LBUS_GPIO0_APB_BASE	(0x20120200)
#define LBUS_GPIO1_APB_BASE	(0x20120200)
#define LBUS_I2C2_APB_BASE	(0x20120200)
#define LBUS_I2C3_APB_BASE	(0x20120200)
#define LBUS_I2C4_APB_BASE	(0x20120200)

#define LBUS_AXI_NAME		"cmu_lbus_axi"
#define LBUS_AHB_NAME		"cmu_lbus_ahb"
#define LBUS_GMAC_AHB_NAME	"cmu_gmac_ahb"
#define LBUS_MP2TS_AHB_NAME	"cmu_lmp2ts_ahb"
#define LBUS_APB_NAME		"cmu_lbus_apb"
#define LBUS_GMAC_CSR_NAME	"cmu_lgmac_csr_apb"
#define LBUS_TIEOFF_APB_NAME	"cmu_lbus_tieoff_apb"
#define LBUS_GPIO0_APB_NAME	"cmu_gpio0_apb"
#define LBUS_GPIO1_APB_NAME	"cmu_gpio1_apb"
#define LBUS_I2C2_APB_NAME	"cmu_i2c2_apb"
#define LBUS_I2C3_APB_NAME	"cmu_i2c3_apb"
#define LBUS_I2C4_APB_NAME	"cmu_i2c4_apb"

/*
 * CMU_LBUS PERI
 */
#define SDMMC0_CORE_CLK		0
#define SDMMC0_AXI_CLK		1
#define SDMMC1_CORE_CLK		2
#define SDMMC1_AXI_CLK		3
#define SDMMC2_CORE_CLK		4
#define SDMMC2_AXI_CLK		5
#define GMAC_MII_CLK		6
#define GMAC_TX_CLK		7

#define SDMMC0_CORE_NAME	"sdmmc0_core"
#define SDMMC0_AXI_NAME		"sdmmc0_axi"
#define SDMMC1_CORE_NAME	"sdmmc1_core"
#define SDMMC1_AXI_NAME		"sdmmc1_axi"
#define SDMMC2_CORE_NAME	"sdmmc2_core"
#define SDMMC2_AXI_NAME		"sdmmc2_axi"
#define GMAC_MII_NAME		"gmac_mii"
#define GMAC_TX_NAME		"gmac_tx"

#define SDMMC0_CORE_BASE	(0x20120400)
#define SDMMC0_AXI_BASE		(0x20120600)
#define SDMMC1_CORE_BASE	(0x20120800)
#define SDMMC1_AXI_BASE		(0x20120a00)
#define SDMMC2_CORE_BASE	(0x20120c00)
#define SDMMC2_AXI_BASE		(0x20120e00)
#define GMAC_MII_BASE		(0x20121000)
#define GMAC_TX_BASE		(0x20121000)

/*
 * CMU_BBUS AXI
 */
#define BBUS_AXI_CLK		0
#define BBUS_AHB_CLK		1
#define BBUS_APB_CLK		2
#define UART0_APB_CLK		3
#define UART1_APB_CLK		4
#define UART2_APB_CLK		5
#define UART3_APB_CLK		6
#define UART4_APB_CLK		7
#define UART5_APB_CLK		8
#define UART6_APB_CLK		9
#define UART7_APB_CLK		10
#define UART8_APB_CLK		11
#define SPDIFTX0_APB_CLK	12
#define PDIFRTX0_APB_CLK	13
#define TMU_APB_CLK		14
#define BBUS_TIEOFF_APB_CLK	15
#define ADC_0_APB_CLK		16
#define GPIO_2_APB_CLK		17
#define GPIO_3_APB_CLK		18
#define GPIO_4_APB_CLK		19

#define BBUS_AXI_NAME		"cmu_bbus_axi"
#define BBUS_AHB_NAME		"cmu_bbus_ahb"
#define BBUS_APB_NAME		"cmu_bbus_apb"
#define UART0_APB_NAME		"cmu_uart0_apb"
#define UART1_APB_NAME		"cmu_uart1_apb"
#define UART2_APB_NAME		"cmu_uart2_apb"
#define UART3_APB_NAME		"cmu_uart3_apb"
#define UART4_APB_NAME		"cmu_uart4_apb"
#define UART5_APB_NAME		"cmu_uart5_apb"
#define UART6_APB_NAME		"cmu_uart6_apb"
#define UART7_APB_NAME		"cmu_uart7_apb"
#define UART8_APB_NAME		"cmu_uart8_apb"
#define SPDIFTX0_APB_NAME	"cmu_spdiftx0_apb"
#define PDIFRTX0_APB_NAME	"cmu_pdifrtx0_apb"
#define TMU_APB_NAME		"cmu_tmu_apb"
#define BBUS_TIEOFF_APB_NAME	"cmu_bbus_tieoff_apb"
#define ADC_0_APB_NAME		"cmu_adc_0_apb"
#define GPIO_2_APB_NAME		"cmu_gpio_2_apb"
#define GPIO_3_APB_NAME		"cmu_gpio_3_apb"
#define GPIO_4_APB_NAME		"cmu_gpio_4_apb"

#define BBUS_AXI_BASE		(0x20130200)
#define BBUS_AHB_BASE		(0x20130200)
#define BBUS_APB_BASE		(0x20130200)
#define UART0_APB_BASE		(0x20130200)
#define UART1_APB_BASE		(0x20130200)
#define UART2_APB_BASE		(0x20130200)
#define UART3_APB_BASE		(0x20130200)
#define UART4_APB_BASE		(0x20130200)
#define UART5_APB_BASE		(0x20130200)
#define UART6_APB_BASE		(0x20130200)
#define UART7_APB_BASE		(0x20130200)
#define UART8_APB_BASE		(0x20130200)
#define SPDIFTX0_APB_BASE	(0x20130200)
#define PDIFRTX0_APB_BASE	(0x20130200)
#define TMU_APB_BASE		(0x20130200)
#define BBUS_TIEOFF_APB_BASE	(0x20130200)
#define ADC_0_APB_BASE		(0x20130200)
#define GPIO_2_APB_BASE		(0x20130200)
#define GPIO_3_APB_BASE		(0x20130200)
#define GPIO_4_APB_BASE		(0x20130200)

/*
 * CMU_BBUS PERI
 */
#define UART0_CORE_CLK		0
#define UART1_CORE_CLK		1
#define UART2_CORE_CLK		2
#define UART3_CORE_CLK		3
#define UART4_CORE_CLK		4
#define UART5_CORE_CLK		5
#define UART6_CORE_CLK		6
#define UART7_CORE_CLK		7
#define UART8_CORE_CLK		8
#define SPI0_APB_CLK		9
#define SPI0_CORE_CLK		10
#define SPI1_APB_CLK		11
#define SPI1_CORE_CLK		12
#define SPI2_APB_CLK		13
#define SPI2_CORE_CLK		14
#define SPDIFTX0_CORE_CLK	15

#define UART0_CORE_NAME		"uart0_core"
#define UART1_CORE_NAME		"uart1_core"
#define UART2_CORE_NAME		"uart2_core"
#define UART3_CORE_NAME		"uart3_core"
#define UART4_CORE_NAME		"uart4_core"
#define UART5_CORE_NAME		"uart5_core"
#define UART6_CORE_NAME		"uart6_core"
#define UART7_CORE_NAME		"uart7_core"
#define UART8_CORE_NAME		"uart8_core"
#define SPI0_APB_NAME		"spi0_apb"
#define SPI1_APB_NAME		"spi1_apb"
#define SPI2_APB_NAME		"spi2_apb"
#define SPI0_CORE_NAME		"spi0_core"
#define SPI1_CORE_NAME		"spi1_core"
#define SPI2_CORE_NAME		"spi2_core"
#define SPDIFTX0_CORE_NAME	"spdiftx0_core"

#define UART0_CORE_BASE		(0x20130400)
#define UART1_CORE_BASE		(0x20130600)
#define UART2_CORE_BASE		(0x20130800)
#define UART3_CORE_BASE		(0x20130a00)
#define UART4_CORE_BASE		(0x20130c00)
#define UART5_CORE_BASE		(0x20130e00)
#define UART6_CORE_BASE		(0x20131000)
#define UART7_CORE_BASE		(0x20131200)
#define UART8_CORE_BASE		(0x20131400)
#define SPI0_APB_BASE		(0x20131600)
#define SPI0_CORE_BASE		(0x20131800)
#define SPI1_APB_BASE		(0x20131a00)
#define SPI1_CORE_BASE		(0x20131c00)
#define SPI2_APB_BASE		(0x20131e00)
#define SPI2_CORE_BASE		(0x20132000)
#define SPDIFTX0_CORE_BASE	(0x20132200)

/*
 * CMU ISP SYS
 */
#define ISP_0_AXI_CLK	0
#define ISP_0_DIV4_CLK	1

#define ISP_0_AXI_NAME	"cmu_isp0_axi"
#define ISP_0_DIV4_NAME	"cmu_isp0_div4"

#define ISP_0_AXI_BASE	(0x20200200)
#define ISP_0_DIV4_BASE	(0x20200200)

/*
 * CMU HDMI SYS
 */
#define HDMI_0_AXI_CLK		0
#define HDMIV2_0_AXI_CLK 	1
#define HDMI_0_APB_CLK 		2
#define HDMIV2_0_APB_CLK 	3
#define HDMIV2_0_APBPHY_CLK 	4
#define HDMIV2_0_PHY_CLK 	5

#define HDMI_0_AXI_NAME		"cmu_hdmi_0_axi"
#define HDMIV2_0_AXI_NAME 	"cmu_hdmiv2_0_axi"
#define HDMI_0_APB_NAME		"cmu_hdmi_0_apb"
#define HDMIV2_0_APB_NAME 	"cmu_hdmiv2_0_apb"
#define HDMIV2_0_APBPHY_NAME 	"cmu_hdmiv2_0_apbphy"
#define HDMIV2_0_PHY_NAME 	"cmu_hdmiv2_0_phy"

#define HDMI_0_AXI_BASE		(0x20240200)
#define HDMIV2_0_AXI_BASE	(0x20240200)
#define HDMI_0_APB_BASE 	(0x20240200)
#define HDMIV2_0_APB_BASE 	(0x20240200)
#define HDMIV2_0_APBPHY_BASE 	(0x20240200)
#define HDMIV2_0_PHY_BASE	(0x20240200)

#define HDMIV2_0_TMDS_10B_CLK	0
#define HDMIV2_0_TMDS_20B_CLK	1
#define HDMIV2_0_PIXELX2_CLK	2
#define HDMIV2_0_PIXELX_CLK	3
#define HDMIV2_0_AUDIO_CLK	4

#define HDMIV2_0_TMDS_10B_NAME	"hdmiv2_tmds_10b"
#define HDMIV2_0_TMDS_20B_NAME	"hdmiv2_tmds_20b"
#define HDMIV2_0_PIXELX2_NAME	"hdmiv2_pixelx2"
#define HDMIV2_0_PIXELX_NAME	"hdmiv2_pixelx"
#define HDMIV2_0_AUDIO_NAME	"hdmiv2_audio"

#define HDMIV2_0_TMDS_10B_BASE	(0x20240400)
#define HDMIV2_0_TMDS_20B_BASE	(0x20240400)
#define HDMIV2_0_PIXELX2_BASE	(0x20240600)
#define HDMIV2_0_PIXELX_BASE	(0x20240600)
#define HDMIV2_0_AUDIO_BASE	(0x20240800)

/*
 * CMU WAVE SYS
 */
#define WAVE_AXI_CLK		0
#define WAVE_APB_CLK		1

#define WAVE_AXI_NAME		"cmu_wave_axi"
#define WAVE_APB_NAME		"cmu_wave_apb"

#define WAVE_AXI_BASE		(0x20260200)
#define WAVE_APB_BASE		(0x20260200)

/*
 * CMU WAVE PERI
 */
#define WAVE_V_0_CLK		0
#define WAVE_M_0_CLK		1
#define WAVE_C_0_CLK		2
#define WAVE_B_0_CLK		3

#define WAVE_V_0_NAME		"wave_v_0"
#define WAVE_M_0_NAME		"wave_m_0"
#define WAVE_C_0_NAME		"wave_c_0"
#define WAVE_B_0_NAME		"wave_b_0"

#define WAVE_V_0_BASE		(0x20260400)
#define WAVE_M_0_BASE		(0x20260600)
#define WAVE_C_0_BASE		(0x20260800)
#define WAVE_B_0_BASE		(0x20260A00)

/*
 * CMU DISP SYS
 */
#define DISP_0_AXI_CLK			0
#define MIPI_0_AXI_CLK			1
#define CSI_TO_AXI_0_AXI_CLK		2
#define CSI_TO_NXS_0_AXI_CLK		3
#define CSI_TO_NXS_1_AXI_CLK		4
#define CSI_TO_NXS_2_AXI_CLK		5
#define CSI_TO_NXS_3_AXI_CLK		6
#define MLC_0_BOTTOM0_CLK		7
#define MLC_0_BOTTOM1_CLK		8
#define MLC_0_BLENDER0_CLK		9
#define MLC_0_BLENDER1_CLK		10
#define MLC_0_BLENDER2_CLK		11
#define MLC_0_BLENDER3_CLK		12
#define MLC_0_BLENDER4_CLK		13
#define MLC_0_BLENDER5_CLK		14
#define MLC_0_BLENDER6_CLK		15
#define MLC_0_BLENDER7_CLK		16
#define VIP_0_AXI_CLK			17
#define VIP_1_AXI_CLK			18
#define VIP_2_AXI_CLK			19
#define VIP_3_AXI_CLK			20
#define MCD_CPUIF_0_AXI_CLK		21
#define VIP_CPUIF_0_AXI_CLK		22
#define VIP_PRES_CPUIF_0_AXI_CLK	23
#define ISS_CPUIF_0_AXI_CLK		24
#define DISP2ISP_0_AXI_CLK		25
#define ISP2DISP_0_AXI_CLK		26
#define ISP2DISP_1_AXI_CLK		27
#define CROP_0_AXI_CLK			28
#define CROP_1_AXI_CLK			29
#define CSC_0_AXI_CLK			30
#define CSC_1_AXI_CLK			31
#define CSC_2_AXI_CLK			32
#define CSC_3_AXI_CLK			33
#define SCALER_0_AXI_CLK		34
#define SCALER_1_AXI_CLK		35
#define SCALER_2_AXI_CLK		36
#define MULTI_TAP_0_AXI_CLK		37
#define MULTI_TAP_1_AXI_CLK		38
#define MULTI_TAP_2_AXI_CLK		39
#define MULTI_TAP_3_AXI_CLK		40
#define DMAR_0_AXI_CLK			41
#define DMAR_1_AXI_CLK			42
#define DMAR_2_AXI_CLK			43
#define DMAR_3_AXI_CLK			44
#define DMAR_4_AXI_CLK			45
#define DMAR_5_AXI_CLK			46
#define DMAR_6_AXI_CLK			47
#define DMAR_7_AXI_CLK			48
#define DMAR_8_AXI_CLK			49
#define DMAR_9_AXI_CLK			50
#define DMAW_0_AXI_CLK			51
#define DMAW_1_AXI_CLK			52
#define DMAW_2_AXI_CLK			53
#define DMAW_3_AXI_CLK			54
#define DMAW_4_AXI_CLK			55
#define DMAW_5_AXI_CLK			56
#define DMAW_6_AXI_CLK			57
#define DMAW_7_AXI_CLK			58
#define DMAW_8_AXI_CLK			59
#define DMAW_9_AXI_CLK			60
#define DMAW_10_AXI_CLK			61
#define DMAW_11_AXI_CLK			62
#define HUE_0_AXI_CLK			63
#define HUE_1_AXI_CLK			64
#define GAMMA_0_AXI_CLK			65
#define GAMMA_1_AXI_CLK			66
#define DPC_0_AXI_CLK			67
#define DPC_1_AXI_CLK			68
#define LVDS_0_AXI_CLK			69
#define LVDS_0_PHY_CLK			70
#define LVDS_1_AXI_CLK			71
#define LVDS_1_PHY_CLK			72
#define NXS_FIFO_0_AXI_CLK		73
#define NXS_FIFO_1_AXI_CLK		74
#define NXS_FIFO_2_AXI_CLK		75
#define NXS_FIFO_3_AXI_CLK		76
#define NXS_FIFO_4_AXI_CLK		77
#define NXS_FIFO_5_AXI_CLK		78
#define NXS_FIFO_6_AXI_CLK		79
#define NXS_FIFO_7_AXI_CLK		80
#define NXS_FIFO_8_AXI_CLK		81
#define NXS_FIFO_9_AXI_CLK		82
#define NXS_FIFO_10_AXI_CLK		83
#define NXS_FIFO_11_AXI_CLK		84
#define NXS2HDMI_0_AXI_CLK		85
#define TPGEN_0_AXI_CLK			86
#define DISP_0_APB_CLK			87
#define DEINTERLACE_0_APB_CLK		88
#define DISP_TIEOFF_0_APB_CLK		89
#define DISP_TZPC_0_APB_CLK		90
#define DISP_TZPC_1_APB_CLK		91
#define MIPI_0_APBCSI_CLK		92
#define MIPI_0_APBDSI_CLK		93
#define MIPI_0_CSIPHY_CLK		94

#define DISP_0_AXI_NAME			"disp_0_axi"
#define MIPI_0_AXI_NAME			"mipi_0_axi"
#define CSI_TO_AXI_0_AXI_NAME		"csi_to_axi_0_axi"
#define CSI_TO_NXS_0_AXI_NAME		"csi_to_nxs_0_axi"
#define CSI_TO_NXS_1_AXI_NAME		"csi_to_nxs_1_axi"
#define CSI_TO_NXS_2_AXI_NAME		"csi_to_nxs_2_axi"
#define CSI_TO_NXS_3_AXI_NAME		"csi_to_nxs_3_axi"
#define MLC_0_BOTTOM0_NAME		"mlc_0_bottom0"
#define MLC_0_BOTTOM1_NAME		"mlc_0_bottom1"
#define MLC_0_BLENDER0_NAME		"mlc_0_blender0"
#define MLC_0_BLENDER1_NAME		"mlc_0_blender1"
#define MLC_0_BLENDER2_NAME		"mlc_0_blender2"
#define MLC_0_BLENDER3_NAME		"mlc_0_blender3"
#define MLC_0_BLENDER4_NAME		"mlc_0_blender4"
#define MLC_0_BLENDER5_NAME		"mlc_0_blender5"
#define MLC_0_BLENDER6_NAME		"mlc_0_blender6"
#define MLC_0_BLENDER7_NAME		"mlc_0_blender7"
#define VIP_0_AXI_NAME			"vip_0_axi"
#define VIP_1_AXI_NAME			"vip_1_axi"
#define VIP_2_AXI_NAME			"vip_2_axi"
#define VIP_3_AXI_NAME			"vip_3_axi"
#define MCD_CPUIF_0_AXI_NAME		"mcd_cpuif_0_axi"
#define VIP_CPUIF_0_AXI_NAME		"vip_cpuif_0_axi"
#define VIP_PRES_CPUIF_0_AXI_NAME	"vip_pres_cpuif_0_axi"
#define ISS_CPUIF_0_AXI_NAME		"iss_cpuif_0_axi"
#define DISP2ISP_0_AXI_NAME		"disp2isp_0_axi"
#define ISP2DISP_0_AXI_NAME		"isp2disp_0_axi"
#define ISP2DISP_1_AXI_NAME		"isp2disp_1_axi"
#define CROP_0_AXI_NAME			"crop_0_axi"
#define CROP_1_AXI_NAME			"crop_1_axi"
#define CSC_0_AXI_NAME			"csc_0_axi"
#define CSC_1_AXI_NAME			"csc_1_axi"
#define CSC_2_AXI_NAME			"csc_2_axi"
#define CSC_3_AXI_NAME			"csc_3_axi"
#define SCALER_0_AXI_NAME		"scaler_0_axi"
#define SCALER_1_AXI_NAME		"scaler_1_axi"
#define SCALER_2_AXI_NAME		"scaler_2_axi"
#define MULTI_TAP_0_AXI_NAME		"multi_tap_0_axi"
#define MULTI_TAP_1_AXI_NAME		"multi_tap_1_axi"
#define MULTI_TAP_2_AXI_NAME		"multi_tap_2_axi"
#define MULTI_TAP_3_AXI_NAME		"multi_tap_3_axi"
#define DMAR_0_AXI_NAME			"dmar_0_axi"
#define DMAR_1_AXI_NAME			"dmar_1_axi"
#define DMAR_2_AXI_NAME			"dmar_2_axi"
#define DMAR_3_AXI_NAME			"dmar_3_axi"
#define DMAR_4_AXI_NAME			"dmar_4_axi"
#define DMAR_5_AXI_NAME			"dmar_5_axi"
#define DMAR_6_AXI_NAME			"dmar_6_axi"
#define DMAR_7_AXI_NAME			"dmar_7_axi"
#define DMAR_8_AXI_NAME			"dmar_8_axi"
#define DMAR_9_AXI_NAME			"dmar_9_axi"
#define DMAW_0_AXI_NAME			"dmaw_0_axi"
#define DMAW_1_AXI_NAME			"dmaw_1_axi"
#define DMAW_2_AXI_NAME			"dmaw_2_axi"
#define DMAW_3_AXI_NAME			"dmaw_3_axi"
#define DMAW_4_AXI_NAME			"dmaw_4_axi"
#define DMAW_5_AXI_NAME			"dmaw_5_axi"
#define DMAW_6_AXI_NAME			"dmaw_6_axi"
#define DMAW_7_AXI_NAME			"dmaw_7_axi"
#define DMAW_8_AXI_NAME			"dmaw_8_axi"
#define DMAW_9_AXI_NAME			"dmaw_9_axi"
#define DMAW_10_AXI_NAME		"dmaw_10_axi"
#define DMAW_11_AXI_NAME		"dmaw_11_axi"
#define HUE_0_AXI_NAME			"hue_0_axi"
#define HUE_1_AXI_NAME			"hue_1_axi"
#define GAMMA_0_AXI_NAME		"gamma_0_axi"
#define GAMMA_1_AXI_NAME		"gamma_1_axi"
#define DPC_0_AXI_NAME			"dpc_0_axi"
#define DPC_1_AXI_NAME			"dpc_1_axi"
#define LVDS_0_AXI_NAME			"lvds_0_axi"
#define LVDS_0_PHY_NAME			"lvds_0_phy"
#define LVDS_1_AXI_NAME			"lvds_1_axi"
#define LVDS_1_PHY_NAME			"lvds_1_phy"
#define NXS_FIFO_0_AXI_NAME		"nxs_fifo_0_axi"
#define NXS_FIFO_1_AXI_NAME		"nxs_fifo_1_axi"
#define NXS_FIFO_2_AXI_NAME		"nxs_fifo_2_axi"
#define NXS_FIFO_3_AXI_NAME		"nxs_fifo_3_axi"
#define NXS_FIFO_4_AXI_NAME		"nxs_fifo_4_axi"
#define NXS_FIFO_5_AXI_NAME		"nxs_fifo_5_axi"
#define NXS_FIFO_6_AXI_NAME		"nxs_fifo_6_axi"
#define NXS_FIFO_7_AXI_NAME		"nxs_fifo_7_axi"
#define NXS_FIFO_8_AXI_NAME		"nxs_fifo_8_axi"
#define NXS_FIFO_9_AXI_NAME		"nxs_fifo_9_axi"
#define NXS_FIFO_10_AXI_NAME		"nxs_fifo_10_axi"
#define NXS_FIFO_11_AXI_NAME		"nxs_fifo_11_axi"
#define NXS2HDMI_0_AXI_NAME		"nxs2hdmi_0_axi"
#define TPGEN_0_AXI_NAME		"tpgen_0_axi"
#define DISP_0_APB_NAME			"disp_0_apb"
#define DEINTERLACE_0_APB_NAME		"deinterlace_0_apb"
#define DISP_TIEOFF_0_APB_NAME		"disp_tieoff_0_apb"
#define DISP_TZPC_0_APB_NAME		"disp_tzpc_0_apb"
#define DISP_TZPC_1_APB_NAME		"disp_tzpc_1_apb"
#define MIPI_0_APBCSI_NAME		"mipi_0_apbcsi"
#define MIPI_0_APBDSI_NAME		"mipi_0_apbdsi"
#define MIPI_0_CSIPHY_NAME		"mipi_0_csiphy"

#endif

