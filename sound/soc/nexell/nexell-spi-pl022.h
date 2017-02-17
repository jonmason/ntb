/*
 * (C) Copyright 2009
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __NEXELL_SPI_PL022_H__
#define __NEXELL_SPI_PL022_H__

#include <linux/amba/pl022.h>

/*
 * Macros to access SSP Registers with their offsets
 */
#define SSP_CR0(r)	(r + 0x000)
#define SSP_CR1(r)	(r + 0x004)
#define SSP_DR(r)	(r + 0x008)
#define SSP_SR(r)	(r + 0x00C)
#define SSP_CPSR(r)	(r + 0x010)
#define SSP_IMSC(r)	(r + 0x014)
#define SSP_RIS(r)	(r + 0x018)
#define SSP_MIS(r)	(r + 0x01C)
#define SSP_ICR(r)	(r + 0x020)
#define SSP_DMACR(r)	(r + 0x024)
#define SSP_CSR(r)	(r + 0x030) /* vendor extension */
#define SSP_ITCR(r)	(r + 0x080)
#define SSP_ITIP(r)	(r + 0x084)
#define SSP_ITOP(r)	(r + 0x088)
#define SSP_TDR(r)	(r + 0x08C)

#define SSP_PID0(r)	(r + 0xFE0)
#define SSP_PID1(r)	(r + 0xFE4)
#define SSP_PID2(r)	(r + 0xFE8)
#define SSP_PID3(r)	(r + 0xFEC)

#define SSP_CID0(r)	(r + 0xFF0)
#define SSP_CID1(r)	(r + 0xFF4)
#define SSP_CID2(r)	(r + 0xFF8)
#define SSP_CID3(r)	(r + 0xFFC)

enum ssp_protocol {
	SSP_PROTOCOL_SPI = 0,	/* Motorola Synchronous Serial Protocol */
	SSP_PROTOCOL_SSP = 1,	/* TI Serial Peripheral Interface Protocol */
	SSP_PROTOCOL_NM	= 2	/* National Microwire frame format */
};

static inline void pl022_enable(void __iomem *base, int on)
{
	u32 mask = 0x1, shift = 1;
	u32 val = readl(SSP_CR1(base)) & ~(mask << shift);

	if (on)
		val |= (1 << shift);

	writel(val, SSP_CR1(base));
}

static inline void pl022_protocol(void __iomem *base,
			enum ssp_protocol protocol)
{
	u32 mask = 0x3, shift = 4;
	u32 val = (readl(SSP_CR0(base)) & ~(mask << shift)) |
		((protocol & mask) << shift);

	writel(val, SSP_CR0(base));
}

static inline void pl022_clock_polarity(void __iomem *base, int invert)
{
	u32 mask = 0x1, shift = 6;
	u32 val = readl(SSP_CR0(base)) & ~(mask << shift);

	if (invert)
		val |= (1 << shift);

	writel(val, SSP_CR0(base));
}

static inline void pl022_clock_phase(void __iomem *base, int phase)
{
	u32 mask = 0x1, shift = 7;
	u32 val = readl(SSP_CR0(base)) & ~(mask << shift);

	if (phase)
		val |= (1 << shift);

	writel(val, SSP_CR0(base));
}

static inline void pl022_bit_width(void __iomem *base, u32 bit_w)
{
	u32 mask = 0xf, shift = 0;
	u32 val = (readl(SSP_CR0(base)) & ~(mask << shift)) |
			(((bit_w - 1) & mask) << shift);

	writel(val, SSP_CR0(base));
}

static inline void pl022_mode(void __iomem *base, int master)
{
	u32 mask = 0x1, shift = 2;
	u32 val = readl(SSP_CR1(base)) & ~(mask << shift);

	if (!master)
		val |= (1 << shift);	/* MASTER:0, SLAVE:1 */

	writel(val, SSP_CR1(base));
}

static inline void pl022_irq_enable(void __iomem *base, u32 mask, int on)
{
	u32 val = readl(SSP_IMSC(base)) & ~(0xf); /* all 0xf */

	if (on)
		val |= (mask & 0xf);
	else
		val &= ~(mask & 0xf);

	writel(val, SSP_IMSC(base));
}

static inline void pl022_slave_output(void __iomem *base, int on)
{
	u32 mask = 0x1, shift = 3;
	u32 val = readl(SSP_CR1(base)) & ~(mask << shift);

	if (!on)
		val |= (1 << shift); /* enable: 0, diable: 1 */

	writel(val, SSP_CR1(base));

}

static inline void pl022_dma_mode(void __iomem *base, int tx, int rx)
{
	u32 mask = 0x3, shift = 0;
	u32 val = readl(SSP_DMACR(base)) & ~(mask << shift);

	if (rx)
		val |= (1 << shift);

	if (tx)
		val |= (1 << (shift + 1));

	writel(val, SSP_DMACR(base));
}

static inline u32 pl022_status(void __iomem *base)
{
	return readl(SSP_SR(base));
}

#endif
