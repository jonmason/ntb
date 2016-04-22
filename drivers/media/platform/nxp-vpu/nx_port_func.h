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

#ifndef __NX_ALLOC_MEM_H__
#define __NX_ALLOC_MEM_H__

#include <linux/types.h>
#include <linux/kernel.h>


/* #define USE_ION_MEMORY */

#define FUNC_MSG		0


#ifndef NX_DTAG
#define NX_DTAG			"[DRV|VPU]"
#endif

#define NX_DbgMsg(COND, MSG)	do {					\
					if (COND) {			\
						printk(NX_DTAG);	\
						printk MSG;		\
					}				\
				} while (0)

#define NX_ErrMsg(MSG)  do {						\
				printk("%s%s(%d) : ", NX_DTAG, __FILE__,\
					__LINE__);			\
				printk MSG;				\
			} while (0)

#if FUNC_MSG
	#define FUNC_IN()	printk("%s() %d IN.\n", __func__, __LINE__)
	#define FUNC_OUT()	printk("%s() %d OUT.\n", __func__, __LINE__)
#else
	#define FUNC_IN()	do {} while (0)
	#define FUNC_OUT()	do {} while (0)
#endif

#ifndef ALIGN
#define ALIGN(X, N)		((X+N-1) & (~(N-1)))
#endif

#define NX_MAX_PLANES		4


/*
 * struct nx_memory_info - nexell private memory type
 */
struct nx_memory_info {
	void *fd;
	int32_t size;
	int32_t align;
	void *virAddr;
	dma_addr_t phyAddr;
};

/*
 * struct nx_vid_memory_info - nexell private video memory type
 */
struct nx_vid_memory_info {
	int32_t width;
	int32_t height;
	int32_t align;
	int32_t planes;
	uint32_t format;			/* Pixel Format(N/A) */

	void *fd[NX_MAX_PLANES];
	int32_t size[NX_MAX_PLANES];		/* Each plane's size. */
	int32_t stride[NX_MAX_PLANES];		/* Each plane's stride */
	void *virAddr[NX_MAX_PLANES];
	uint32_t phyAddr[NX_MAX_PLANES];
};

/*
 * nexell private memory allocator
 */
struct nx_memory_info *nx_alloc_memory(void *drv, int32_t size, int32_t align);
void nx_free_memory(struct nx_memory_info *mem);

/*
 * video specific allocator wrapper
 */
struct nx_vid_memory_info *nx_alloc_frame_memory(void *drv, int32_t width,
	int32_t height, int32_t planes, uint32_t format, int32_t align);
void nx_free_frame_memory(struct nx_vid_memory_info *vid);

void NX_DrvMemset(void *ptr, int value, int size);
void NX_DrvMemcpy(void *dst, void *src, int size);

void DrvMSleep(unsigned int mSeconds);
void DrvUSleep(unsigned int uSeconds);

#endif		/* __NX_ALLOC_MEM_H__ */
