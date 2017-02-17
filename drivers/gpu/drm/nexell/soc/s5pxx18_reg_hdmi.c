/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: junghyun, kim <jhkim@nexell.co.kr>
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
#include <linux/types.h>
#include <linux/io.h>

static void __iomem *hdmi_io_base;

void hdmi_set_base(void __iomem *base)
{
	hdmi_io_base = base;
}

void hdmi_write(u32 reg, u32 val)
{
	writel(val, hdmi_io_base + reg);
}

void hdmi_write_mask(u32 reg, u32 val, u32 mask)
{
	u32 old = readl(hdmi_io_base + reg);

	val = (val & mask) | (old & ~mask);
	writel(val, hdmi_io_base + reg);
}

void hdmi_writeb(u32 reg, u8 val)
{
	writeb(val, hdmi_io_base + reg);
}

void hdmi_write_bytes(u32 reg, u8 *buf, int bytes)
{
	int i;

	for (i = 0; i < bytes; ++i)
		writeb(buf[i], hdmi_io_base + reg + i * 4);
}

u32 hdmi_read(u32 reg)
{
	return readl(hdmi_io_base + reg);
}

u8 hdmi_readb(u32 reg)
{
	return readb(hdmi_io_base + reg);
}

void hdmi_read_bytes(u32 reg, u8 *buf, int bytes)
{
	int i;

	for (i = 0; i < bytes; ++i)
		buf[i] = readb(hdmi_io_base + reg + i * 4);
}

