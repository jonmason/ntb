/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Seonghee, Kim <kshblue@nexell.co.kr>
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

#include <linux/slab.h>
#include <linux/videodev2.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <media/videobuf2-dma-contig.h>
#include "nx_port_func.h"


struct nx_memory_info *nx_alloc_memory(void *drv, int32_t size, int32_t align)
{
	struct nx_memory_info *handle;

	handle = devm_kzalloc(drv, sizeof(*handle), GFP_KERNEL);
	if (NULL == handle)
		goto Error_Exit;

#ifndef USE_ION_MEMORY
	handle->virAddr = dma_alloc_coherent(drv, size, &handle->phyAddr,
		GFP_KERNEL);
	if (!handle->virAddr) {
		handle->phyAddr = 0;
		goto Error_Exit;
	}
#else
	size = (size + 4095) & ~4095;
	handle->phyAddr = cma_alloc(drv, "nxp-ion", size, align);
	if (handle->phyAddr == (-EINVAL)) {
		dev_err(drv, "nx_alloc_memory is error\n");
		handle->phyAddr = 0;
		goto Error_Exit;
	}

	handle->virAddr = (void *)cma_get_virt(handle->phyAddr, size, 1);
	if (handle->virAddr == 0) {
		dev_err(drv, "Failed to cma_get_virt(0x%08x)\n",
			(int)handle->phyAddr);
		goto Error_Exit;
	}
#endif

	NX_DrvMemset(handle->virAddr, 0, size);

	handle->fd = drv;
	handle->size = size;
	handle->align = align;

	return handle;

Error_Exit:
	dev_err(drv, "nx_alloc_memory is failed\n");

	if (handle) {
		if (handle->phyAddr) {
#ifndef USE_ION_MEMORY
			dma_free_coherent(drv, handle->size, handle->virAddr,
				handle->phyAddr);
#else
			cma_free(handle->phyAddr);
#endif
		}
	}

	return NULL;
}

void nx_free_memory(struct nx_memory_info *mem)
{
	if (mem) {
		if (0 != mem->phyAddr) {
#ifndef USE_ION_MEMORY
			dma_free_coherent(mem->fd, mem->size, mem->virAddr,
				mem->phyAddr);
#else
			cma_free(mem->phyAddr);
#endif
		}
	}
}

struct nx_vid_memory_info *nx_alloc_frame_memory(void *drv, int32_t width,
	int32_t height, int32_t planes, uint32_t format, int32_t align)
{
	int32_t i, chroma_planes;
	int32_t lWidth, lHeight;
	int32_t cWidth, cHeight;
	uint32_t lSize;		/* Luminance plane size */
	uint32_t cSize;		/* Chrominance plane size */
	struct nx_vid_memory_info *vid = NULL;
	struct nx_memory_info *mem[NX_MAX_PLANES];

	vid = devm_kzalloc(drv, sizeof(*vid), GFP_KERNEL);
	if (NULL == vid)
		goto Error_Exit;

	for (i = 0 ; i < NX_MAX_PLANES ; i++)
		mem[i] = NULL;

	lWidth  = ALIGN(width,  32);
	lHeight = ALIGN(height, 16);
	lSize = lWidth * lHeight;

	switch (format) {
	case V4L2_PIX_FMT_YUV420M:
		cWidth  = ALIGN(width/2,  16);
		cHeight = ALIGN(height/2, 16);
		chroma_planes = 2;
		break;
	case V4L2_PIX_FMT_NV12M:
		cWidth  = lWidth;
		cHeight = lHeight/2;
		chroma_planes = 1;
		break;
	default:
		NX_ErrMsg(("Unknown fourCC type.\n"));
		goto Error_Exit;
	}
	cSize = cWidth * cHeight;

	mem[0] = nx_alloc_memory(drv, lSize, align);
	for (i = 1 ; i <= chroma_planes ; i++)
		mem[i] = nx_alloc_memory(drv, cSize, align);

	vid->width  = width;
	vid->height = height;
	vid->align = align;
	vid->planes = planes;
	vid->format = format;

	for (i = 0 ; i <= chroma_planes ; i++) {
		vid->fd[i] = mem[i]->fd;
		vid->size[i] = mem[i]->size;
		vid->stride[i] = (i == 0) ? (lWidth) : (cWidth);
		vid->virAddr[i] = mem[i]->virAddr;
		vid->phyAddr[i] = mem[i]->phyAddr;
	}

	return vid;

Error_Exit:
	if (vid) {
		for (i = 0 ; i < planes ; i++)
			nx_free_memory(mem[i]);
	}

	return NULL;
}

void nx_free_frame_memory(struct nx_vid_memory_info *vid)
{
	if (vid) {
		int32_t i;

		for (i = 0 ; i < vid->planes ; i++) {
#ifndef USE_ION_MEMORY
			dma_free_coherent(vid->fd[i], vid->size[i],
				vid->virAddr[i], vid->phyAddr[i]);
#else
			cma_free(vid->phyAddr[i]);
#endif
		}

	}
}

void NX_DrvMemset(void *ptr, int value, int size)
{
	memset(ptr, value, size);
}

void NX_DrvMemcpy(void *dst, void *src, int size)
{
	memcpy(dst, src, size);
}

void DrvMSleep(unsigned int mSeconds)
{
	msleep(mSeconds);
}

void DrvUSleep(unsigned int uSeconds)
{
	udelay(uSeconds);
}
