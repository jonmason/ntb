/*
 * Nexell SoC USB 1.1/2.0 PHY driver
 *
 * Copyright (C) 2016 Nexell Co., Ltd.
 * Author: Hyunseok Jung <hsjung@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/phy/phy.h>
#include <linux/regmap.h>
#include "phy-samsung-usb2.h"

/* Nexell USBHOST PHY registers */

/* USBHOST Configuration 0 Register */
#define NX_HOST_CON0				0x14
#define NX_HOST_CON0_SS_WORD_IF			BIT(26)
#define NX_HOST_CON0_SS_WORD_IF_ENB		BIT(25)
#define NX_HOST_CON0_SS_WORD_IF_16 ( \
	NX_HOST_CON0_SS_WORD_IF | \
	NX_HOST_CON0_SS_WORD_IF_ENB)

#define NX_HOST_CON0_HSIC_480M_FROM_OTG_PHY	BIT(24)
#define NX_HOST_CON0_HSIC_FREE_CLOCK_ENB	BIT(23)
#define NX_HOST_CON0_HSIC_CLK_MASK		(0x3 << 23)

#define NX_HOST_CON0_N_HOST_HSIC_RESET_SYNC	BIT(22)
#define NX_HOST_CON0_N_HOST_UTMI_RESET_SYNC	BIT(21)
#define NX_HOST_CON0_N_HOST_PHY_RESET_SYNC	BIT(20)
#define NX_HOST_CON0_UTMI_RESET_SYNC ( \
	NX_HOST_CON0_N_HOST_HSIC_RESET_SYNC | \
	NX_HOST_CON0_N_HOST_UTMI_RESET_SYNC | \
	NX_HOST_CON0_N_HOST_PHY_RESET_SYNC)

#define NX_HOST_CON0_N_AUXWELL_RESET_SYNC	BIT(19)
#define NX_HOST_CON0_N_OHCI_RESET_SYNC		BIT(18)
#define NX_HOST_CON0_N_RESET_SYNC		BIT(17)
#define NX_HOST_CON0_AHB_RESET_SYNC ( \
	NX_HOST_CON0_N_AUXWELL_RESET_SYNC | \
	NX_HOST_CON0_N_OHCI_RESET_SYNC | \
	NX_HOST_CON0_N_RESET_SYNC)

#define NX_HOST_CON0_HSIC_EN_PORT1		(0x2 << 14)
#define NX_HOST_CON0_HSIC_EN_MASK		(0x7 << 14)

/* USBHOST Configuration 1 Register */
#define NX_HOST_CON1				0x18

/* USBHOST Configuration 2 Register */
#define NX_HOST_CON2				0x1C
#define NX_HOST_CON2_SS_ENA_INCRX_ALIGN		(0x1 << 28)
#define NX_HOST_CON2_SS_ENA_INCR4		(0x1 << 27)
#define NX_HOST_CON2_SS_ENA_INCR8		(0x1 << 26)
#define NX_HOST_CON2_SS_ENA_INCR16		(0x1 << 25)
#define NX_HOST_CON2_SS_DMA_BURST_MASK  \
	(NX_HOST_CON2_SS_ENA_INCR16 | NX_HOST_CON2_SS_ENA_INCR8 | \
	 NX_HOST_CON2_SS_ENA_INCR4 | NX_HOST_CON2_SS_ENA_INCRX_ALIGN)

#define NX_HOST_CON2_EHCI_SS_ENABLE_DMA_BURST \
	(NX_HOST_CON2_SS_ENA_INCR16 | NX_HOST_CON2_SS_ENA_INCR8 | \
	 NX_HOST_CON2_SS_ENA_INCR4 | NX_HOST_CON2_SS_ENA_INCRX_ALIGN)

#define NX_HOST_CON2_OHCI_SS_ENABLE_DMA_BURST \
	(NX_HOST_CON2_SS_ENA_INCR4 | NX_HOST_CON2_SS_ENA_INCRX_ALIGN)

#define NX_HOST_CON2_SS_FLADJ_VAL_0_OFFSET	(21)
#define NX_HOST_CON2_SS_FLADJ_VAL_OFFSET	(3)
#define NX_HOST_CON2_SS_FLADJ_VAL_NUM		(6)
#define NX_HOST_CON2_SS_FLADJ_VAL_0_SEL		BIT(5)
#define NX_HOST_CON2_SS_FLADJ_VAL_MAX		0x7

/* USBHOST Configuration 3 Register */
#define NX_HOST_CON3				0x20
#define NX_HOST_CON3_POR			BIT(8)
#define NX_HOST_CON3_POR_ENB			BIT(7)
#define NX_HOST_CON3_POR_MASK			(0x3 << 7)

/* USBHOST Configuration 4 Register */
#define NX_HOST_CON4				0x24
#define NX_HOST_CON4_WORDINTERFACE		BIT(9)
#define NX_HOST_CON4_WORDINTERFACE_ENB		BIT(8)
#define NX_HOST_CON4_WORDINTERFACE_16 ( \
	NX_HOST_CON4_WORDINTERFACE | \
	NX_HOST_CON4_WORDINTERFACE_ENB)

/* USBHOST Configuration 5 Register */
#define NX_HOST_CON5				0x28
#define NX_HOST_CON5_HSIC_POR			BIT(19)
#define NX_HOST_CON5_HSIC_POR_ENB		BIT(18)
#define NX_HOST_CON5_HSIC_POR_MASK		(0x3 << 18)

/* USBHOST Configuration 6 Register */
#define NX_HOST_CON6				0x2C
#define NX_HOST_CON6_HSIC_WORDINTERFACE		BIT(13)
#define NX_HOST_CON6_HSIC_WORDINTERFACE_ENB	BIT(12)
#define NX_HOST_CON6_HSIC_WORDINTERFACE_16 ( \
	NX_HOST_CON6_HSIC_WORDINTERFACE | \
	NX_HOST_CON6_HSIC_WORDINTERFACE_ENB)

/* Nexell USBOTG PHY registers */

/* USBOTG Configuration 0 Register */
#define NX_OTG_CON0				0x30
#define NX_OTG_CON0_SS_SCALEDOWN_MODE		(3 << 0)

/* USBOTG Configuration 1 Register */
#define NX_OTG_CON1				0x34
#define NX_OTG_CON1_VBUSVLDEXTSEL		BIT(25)
#define NX_OTG_CON1_VBUSVLDEXT			BIT(24)
#define NX_OTG_CON1_VBUS_INTERNAL ~( \
	NX_OTG_CON1_VBUSVLDEXTSEL | \
	NX_OTG_CON1_VBUSVLDEXT)
#define NX_OTG_CON1_VBUS_VLDEXT0 ( \
	NX_OTG_CON1_VBUSVLDEXTSEL | \
	NX_OTG_CON1_VBUSVLDEXT)

#define NX_OTG_CON1_POR				BIT(8)
#define NX_OTG_CON1_POR_ENB			BIT(7)
#define NX_OTG_CON1_POR_MASK			(0x3 << 7)
#define NX_OTG_CON1_RST				BIT(3)
#define NX_OTG_CON1_UTMI_RST			BIT(2)

/* USBOTG Configuration 2 Register */
#define NX_OTG_CON2				0x38
#define NX_OTG_CON2_OTGTUNE_MASK		(0x7 << 23)
#define NX_OTG_CON2_WORDINTERFACE		BIT(9)
#define NX_OTG_CON2_WORDINTERFACE_ENB		BIT(8)
#define NX_OTG_CON2_WORDINTERFACE_16 ( \
	NX_OTG_CON2_WORDINTERFACE | \
	NX_OTG_CON2_WORDINTERFACE_ENB)

/* USBOTG Configuration 3 Register */
#define NX_OTG_CON3				0x3C
#define NX_OTG_CON3_ACAENB			BIT(15)
#define NX_OTG_CON3_DCDENB			BIT(14)
#define NX_OTG_CON3_VDATSRCENB			BIT(13)
#define NX_OTG_CON3_VDATDETENB			BIT(12)
#define NX_OTG_CON3_CHRGSEL			BIT(11)
#define NX_OTG_CON3_DET_N_CHG ( \
	NX_OTG_CON3_ACAENB | \
	NX_OTG_CON3_DCDENB | \
	NX_OTG_CON3_VDATSRCENB | \
	NX_OTG_CON3_VDATDETENB | \
	NX_OTG_CON3_CHRGSEL)

enum nx_phy_id {
	NX_OTG,
	NX_HOST,
	NX_HSIC,
	NX_NUM_PHYS,
};

static int nx_otg_power_on(struct samsung_usb2_phy_instance *inst)
{
	struct samsung_usb2_phy_driver *drv = inst->drv;
	u32 reg;

	writel(readl((void *)(drv->reg_phy + NX_OTG_CON3)) &
	       ~NX_OTG_CON3_DET_N_CHG,
	       (void *)(drv->reg_phy + NX_OTG_CON3));

	reg  = readl((void *)(drv->reg_phy + NX_OTG_CON2)) &
		~NX_OTG_CON2_OTGTUNE_MASK;
	writel(reg, (void *)(drv->reg_phy +  NX_OTG_CON2));

	writel(readl((void *)(drv->reg_phy + NX_OTG_CON0)) &
	       ~NX_OTG_CON0_SS_SCALEDOWN_MODE,
	       (void *)(drv->reg_phy + NX_OTG_CON0));

	writel(readl((void *)(drv->reg_phy + NX_OTG_CON2)) |
	       NX_OTG_CON2_WORDINTERFACE_16,
	       (void *)(drv->reg_phy + NX_OTG_CON2));

	writel(readl((void *)(drv->reg_phy + NX_OTG_CON1)) &
	       NX_OTG_CON1_VBUS_INTERNAL,
	       (void *)(drv->reg_phy + NX_OTG_CON1));

	reg = readl((void *)(drv->reg_phy + NX_OTG_CON1));
	reg &= ~NX_OTG_CON1_POR_MASK;
	reg |= NX_OTG_CON1_POR_ENB;
	writel(reg, (void *)(drv->reg_phy + NX_OTG_CON1));
	udelay(1);
	reg |= NX_OTG_CON1_POR_MASK;
	writel(reg, (void *)(drv->reg_phy + NX_OTG_CON1));
	udelay(1);
	reg &= ~NX_OTG_CON1_POR;
	writel(reg, (void *)(drv->reg_phy + NX_OTG_CON1));
	udelay(10);

	writel(readl((void *)(drv->reg_phy + NX_OTG_CON1)) | NX_OTG_CON1_RST,
	       (void *)(drv->reg_phy + NX_OTG_CON1));
	udelay(1);

	writel(readl((void *)(drv->reg_phy + NX_OTG_CON1)) |
	       NX_OTG_CON1_UTMI_RST,
	       (void *)(drv->reg_phy + NX_OTG_CON1));
	udelay(1);

	return 0;
}

static int nx_otg_power_off(struct samsung_usb2_phy_instance *inst)
{
	struct samsung_usb2_phy_driver *drv = inst->drv;

	writel(readl((void *)(drv->reg_phy + NX_OTG_CON1)) |
	       NX_OTG_CON1_VBUS_VLDEXT0,
	       (void *)(drv->reg_phy + NX_OTG_CON1));

	writel(readl((void *)(drv->reg_phy + NX_OTG_CON1)) & ~NX_OTG_CON1_RST,
	       (void *)(drv->reg_phy + NX_OTG_CON1));
	udelay(10);

	writel(readl((void *)(drv->reg_phy + NX_OTG_CON1)) &
	       ~NX_OTG_CON1_UTMI_RST,
	       (void *)(drv->reg_phy + NX_OTG_CON1));
	udelay(10);

	writel(readl((void *)(drv->reg_phy + NX_OTG_CON1)) |
	       NX_OTG_CON1_POR_MASK,
	       (void *)(drv->reg_phy + NX_OTG_CON1));
	udelay(10);

	return 0;
}

static int nx_host_power_on(struct samsung_usb2_phy_instance *inst)
{
	struct samsung_usb2_phy_driver *drv = inst->drv;
	u32 reg;
	u32 reg1, reg2;
	u32 fladj_val, bit_num, bit_pos = NX_HOST_CON2_SS_FLADJ_VAL_0_OFFSET;

	fladj_val = NX_HOST_CON2_SS_FLADJ_VAL_0_SEL;

	reg = fladj_val;

	for (bit_num = 0; bit_num < NX_HOST_CON2_SS_FLADJ_VAL_NUM; bit_num++) {
		if (fladj_val & (1 << bit_num))
			reg |= (NX_HOST_CON2_SS_FLADJ_VAL_MAX << bit_pos);
		bit_pos -= NX_HOST_CON2_SS_FLADJ_VAL_OFFSET;
	}

	writel(reg, (void *)(drv->reg_phy + NX_HOST_CON2));

	reg = readl((void *)(drv->reg_phy + NX_HOST_CON2)) &
		~NX_HOST_CON2_SS_DMA_BURST_MASK;
	writel(reg | NX_HOST_CON2_EHCI_SS_ENABLE_DMA_BURST,
	       (void *)(drv->reg_phy +	NX_HOST_CON2));

	reg1 = readl((void *)(drv->reg_phy + NX_HOST_CON0)) |
		NX_HOST_CON0_SS_WORD_IF_16;
	reg2 = readl((void *)(drv->reg_phy + NX_HOST_CON4)) |
		NX_HOST_CON4_WORDINTERFACE_16;

	writel(reg1, (void *)(drv->reg_phy + NX_HOST_CON0));
	writel(reg2, (void *)(drv->reg_phy + NX_HOST_CON4));

	reg   = readl((void *)(drv->reg_phy + NX_HOST_CON3));
	reg  &= ~NX_HOST_CON3_POR_MASK;
	reg  |=  NX_HOST_CON3_POR_ENB;
	writel(reg, (void *)(drv->reg_phy + NX_HOST_CON3));
	udelay(10);

	reg = readl((void *)(drv->reg_phy + NX_HOST_CON0)) |
		NX_HOST_CON0_N_HOST_UTMI_RESET_SYNC |
		NX_HOST_CON0_N_HOST_PHY_RESET_SYNC;
	writel(reg, (void *)(drv->reg_phy + NX_HOST_CON0));

	writel(readl((void *)(drv->reg_phy + NX_HOST_CON0)) |
	       NX_HOST_CON0_AHB_RESET_SYNC,
	       (void *)(drv->reg_phy + NX_HOST_CON0));

	return 0;
}

static int nx_host_power_off(struct samsung_usb2_phy_instance *inst)
{
	struct samsung_usb2_phy_driver *drv = inst->drv;
	u32 reg;

	writel(readl((void *)(drv->reg_phy + NX_HOST_CON0)) &
	       ~NX_HOST_CON0_AHB_RESET_SYNC,
	       (void *)(drv->reg_phy + NX_HOST_CON0));

	writel(readl((void *)(drv->reg_phy + NX_HOST_CON0)) &
	       ~NX_HOST_CON0_UTMI_RESET_SYNC,
	       (void *)(drv->reg_phy + NX_HOST_CON0));

	reg    = readl((void *)(drv->reg_phy + NX_HOST_CON3));
	reg   &= ~NX_HOST_CON3_POR_MASK;
	reg   |=  NX_HOST_CON3_POR_ENB;
	writel(reg, (void *)(drv->reg_phy + NX_HOST_CON3));
	udelay(1);
	reg   |=  NX_HOST_CON3_POR_MASK;
	writel(reg, (void *)(drv->reg_phy + NX_HOST_CON3));
	udelay(1);
	reg   &= ~(NX_HOST_CON3_POR);
	writel(reg, (void *)(drv->reg_phy + NX_HOST_CON3));

	udelay(10);

	return 0;
}

static int nx_hsic_power_on(struct samsung_usb2_phy_instance *inst)
{
	struct samsung_usb2_phy_driver *drv = inst->drv;
	u32 reg;

	writel(readl((void *)(drv->reg_phy + NX_HOST_CON0)) &
	       ~NX_HOST_CON0_HSIC_CLK_MASK,
	       (void *)(drv->reg_phy + NX_HOST_CON0));
	writel(readl((void *)(drv->reg_phy + NX_HOST_CON0)) |
	       NX_HOST_CON0_HSIC_480M_FROM_OTG_PHY,
	       (void *)(drv->reg_phy + NX_HOST_CON0));

	writel(readl((void *)(drv->reg_phy + NX_HOST_CON0)) &
	       ~NX_HOST_CON0_HSIC_EN_MASK,
	       (void *)(drv->reg_phy + NX_HOST_CON0));
	writel(readl((void *)(drv->reg_phy + NX_HOST_CON0)) |
	       NX_HOST_CON0_HSIC_EN_PORT1,
	       (void *)(drv->reg_phy + NX_HOST_CON0));

	reg = readl((void *)(drv->reg_phy + NX_HOST_CON6)) |
		NX_HOST_CON6_HSIC_WORDINTERFACE_16;
	writel(reg, (void *)(drv->reg_phy + NX_HOST_CON6));

	writel(readl((void *)(drv->reg_phy + NX_HOST_CON0)) |
	       NX_HOST_CON0_HSIC_CLK_MASK,
	       (void *)(drv->reg_phy + NX_HOST_CON0));

	writel(readl((void *)(drv->reg_phy + NX_HOST_CON5)) &
	       ~NX_HOST_CON5_HSIC_POR_MASK,
	       (void *)(drv->reg_phy + NX_HOST_CON5));
	writel(readl((void *)(drv->reg_phy + NX_HOST_CON5)) |
	       NX_HOST_CON5_HSIC_POR_ENB,
	       (void *)(drv->reg_phy + NX_HOST_CON5));

	udelay(100);

	reg = readl((void *)(drv->reg_phy + NX_HOST_CON0)) |
		NX_HOST_CON0_N_HOST_HSIC_RESET_SYNC;
	writel(reg, (void *)(drv->reg_phy + NX_HOST_CON0));

	return 0;
}

static int nx_hsic_power_off(struct samsung_usb2_phy_instance *inst)
{
	struct samsung_usb2_phy_driver *drv = inst->drv;

	writel(readl((void *)(drv->reg_phy + NX_HOST_CON0)) &
	       ~NX_HOST_CON0_HSIC_CLK_MASK,
	       (void *)(drv->reg_phy + NX_HOST_CON0));

	writel(readl((void *)(drv->reg_phy + NX_HOST_CON5)) |
	       NX_HOST_CON5_HSIC_POR_MASK,
	       (void *)(drv->reg_phy + NX_HOST_CON5));

	return 0;
}

static const struct samsung_usb2_common_phy nx_phys[] = {
	{
		.label		= "otg",
		.id		= NX_OTG,
		.power_on	= nx_otg_power_on,
		.power_off	= nx_otg_power_off,
	},
	{
		.label		= "host",
		.id		= NX_HOST,
		.power_on	= nx_host_power_on,
		.power_off	= nx_host_power_off,
	},
	{
		.label		= "hsic",
		.id		= NX_HSIC,
		.power_on	= nx_hsic_power_on,
		.power_off	= nx_hsic_power_off,
	},
};

const struct samsung_usb2_phy_config nexell_usb2_phy_config = {
	.num_phys		= NX_NUM_PHYS,
	.phys			= nx_phys,
};
