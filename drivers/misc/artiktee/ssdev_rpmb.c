/*********************************************************
 * Copyright (C) 2011 - 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 *********************************************************/
#ifdef CONFIG_SECOS_NO_RPMB

#else
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/vmalloc.h>
#include <linux/syscalls.h>
#include <linux/mmc/ioctl.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/version.h>

#ifndef CONFIG_SECOS_NO_SECURE_STORAGE

#if defined(CONFIG_MMC) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))

#include "ssdev_rpmb.h"
#include "tzlog_print.h"

#define RPMB_DEVICE         "mmcblk0rpmb"

struct rpmb_frame frames_in[2];
struct rpmb_frame frame_out;

/* Performs RPMB operation.
 *
 * @bdev: block device on which RPMB operation should be performed
 * @frame_in: input RPMB frame, should be properly inited
 * @frame_out: output (result) RPMB frame. Caller is responsible for checking
 *             result and req_resp for output frame.
 * @out_cnt: count of outer frames. Used only for multiple blocks reading,
 *           in the other cases -EINVAL will be returned.
 */

static void dump_mmc_ioc_cmd(struct mmc_ioc_cmd_mult *mul)
{
	int i;

	tzlog_print(TZLOG_ERROR, "MMC_IOC_CMD_MUL : IOC_CNT :  %d\n",
		    mul->ioc_cnt);
	for (i = 0; i < mul->ioc_cnt; i++) {
		struct mmc_ioc_cmd *cmd =
		    (struct mmc_ioc_cmd *)(uintptr_t) (mul->ioc_vec);
		tzlog_print(TZLOG_ERROR, " * IOC_CMD[%d](%p)\n", i,
			    (void *)&cmd[i]);
		tzlog_print(TZLOG_ERROR,
			    "  + arg              : %p  write_flag : %d\n",
			    (void *)cmd[i].arg, cmd[i].write_flag);
		tzlog_print(TZLOG_ERROR,
			    "  + blksz            : %-10d  blocks     : %d\n",
			    cmd[i].blksz, cmd[i].blocks);
		tzlog_print(TZLOG_ERROR,
			    "  + opcode           : 0x%-8x  flags      : 0x%x\n",
			    cmd[i].opcode, cmd[i].flags);
		tzlog_print(TZLOG_ERROR,
			    "  + postsleep_min_us : %-10d  post_sleep_max_us  : %d\n",
			    cmd[i].postsleep_min_us, cmd[i].postsleep_max_us);
		tzlog_print(TZLOG_ERROR, "  + data_ptr         : %p",
			    (void *)(uintptr_t) cmd[i].data_ptr);

	}
}

static int mmc_rpmb_op(struct block_device *bdev,
		       struct rpmb_frame *frame_in,
		       struct rpmb_frame *frame_out, unsigned int out_cnt)
{
	int err;
	static int call_count;
	u_int16_t rpmb_type;
	const struct block_device_operations *ops;
	mm_segment_t oldfs;
	struct mmc_ioc_cmd_mult mmc_ioc_cmd_mul;

	memset(&mmc_ioc_cmd_mul, 0, sizeof(struct mmc_ioc_cmd_mult));

	/* Paranoid checks */
	if (unlikely(!bdev || !bdev->bd_disk || !bdev->bd_disk->fops ||
		     !bdev->bd_disk->fops->ioctl))
		return -EINVAL;

	if (unlikely(!frame_in || !frame_out || !out_cnt))
		return -EINVAL;


	rpmb_type = be16_to_cpu(frame_in[0].req_resp);
	ops = bdev->bd_disk->fops;
	oldfs = get_fs();

	++call_count;
	/* We have to increase the limit */
	set_fs(KERNEL_DS);

	switch (rpmb_type) {
	case MMC_RPMB_WRITE:
	case MMC_RPMB_WRITE_KEY:{
			struct mmc_ioc_cmd ioc[3] = {
				/* Write request */
				{
				 .arg = 0x0,
				 .blksz = 512,
				 .blocks = 1,
				 .write_flag = 1 | 1 << 31,
				 .opcode = MMC_WRITE_MULTIPLE_BLOCK,
				 .flags = MMC_RSP_R1B | MMC_CMD_ADTC,
				 .postsleep_min_us = 1000,
				 .postsleep_max_us = 2000},
				/* Result request */
				{
				 .arg = 0x0,
				 .blksz = 512,
				 .blocks = 1,
				 .write_flag = 1,
				 .opcode = MMC_WRITE_MULTIPLE_BLOCK,
				 .flags = MMC_RSP_R1B | MMC_CMD_ADTC,
				 .postsleep_min_us = 1000,
				 .postsleep_max_us = 2000},
				/* Get response */
				{
				 .arg = 0x0,
				 .blksz = 512,
				 .blocks = 1,
				 .write_flag = 0,
				 .opcode = MMC_READ_MULTIPLE_BLOCK,
				 .flags = MMC_RSP_R1 | MMC_CMD_ADTC,
				 .postsleep_min_us = 1000,
				 .postsleep_max_us = 2000}
			};

			if (out_cnt != 1) {
				err = -EINVAL;
				goto out;
			}

			frame_in[1].req_resp = cpu_to_be16(MMC_RPMB_READ_RESP);
			memset(frame_out, 0, sizeof(*frame_out));

			ioc[0].data_ptr = (uintptr_t)&frame_in[0];
			ioc[1].data_ptr = (uintptr_t)&frame_in[1];
			ioc[2].data_ptr = (uintptr_t)frame_out;

			mmc_ioc_cmd_mul.ioc_cnt = 3;
			mmc_ioc_cmd_mul.ioc_vec = (unsigned long)ioc;

			err =
			    ops->ioctl(bdev, 0, MMC_IOC_CMD_MULT,
				       (unsigned long)&mmc_ioc_cmd_mul);
			if (err < 0)
				goto out;
		}
		break;

	case MMC_RPMB_READ_CNT:
		if (out_cnt != 1) {
			err = -EINVAL;
			goto out;
		}
		/* fall through */

	case MMC_RPMB_READ:
		/* Request */
		{
			struct mmc_ioc_cmd ioc[2] = {
				{
				 .arg = 0x0,
				 .blksz = 512,
				 .blocks = 1,
				 .write_flag = 1,
				 .opcode = MMC_WRITE_MULTIPLE_BLOCK,
				 .flags = MMC_RSP_R1B | MMC_CMD_ADTC,
				 .postsleep_min_us = 1000,
				 .postsleep_max_us = 2000},
				/* Get response */
				{
				 .arg = 0x0,
				 .blksz = 512,
				 .write_flag = 0,
				 .opcode = MMC_READ_MULTIPLE_BLOCK,
				 .flags = MMC_RSP_R1 | MMC_CMD_ADTC,
				 .postsleep_min_us = 1000,
				 .postsleep_max_us = 2000}
			};

			memset(frame_out, 0, sizeof(*frame_out));

			ioc[0].data_ptr = (uintptr_t)&frame_in[0];
			ioc[1].data_ptr = (uintptr_t) frame_out;
			ioc[1].blocks = out_cnt;

			mmc_ioc_cmd_mul.ioc_cnt = 2;
			mmc_ioc_cmd_mul.ioc_vec = (unsigned long)ioc;

			err =
			    ops->ioctl(bdev, 0, MMC_IOC_CMD_MULT,
				       (unsigned long)&mmc_ioc_cmd_mul);
			if (err < 0)
				goto out;
		}

		break;

	default:
		err = -EINVAL;
		goto out;
	}

out:
	if (err < 0) {
		tzlog_print(TZLOG_ERROR, "RPMB Operation Fail, req_cnt : %d\n",
			    call_count);
		dump_mmc_ioc_cmd(&mmc_ioc_cmd_mul);
	}
	set_fs(oldfs);

	return err;
}

static dev_t rpmb_device(void)
{
	static dev_t rpmb_dev = MKDEV(0, 0);

	if (rpmb_dev == MKDEV(0, 0)) {
		rpmb_dev = blk_lookup_devt(RPMB_DEVICE, 0);
		if (rpmb_dev == MKDEV(0, 0)) {
			tzlog_print(TZLOG_ERROR,
				    "/dev/mmcblk0rpmb no exist.\n");
		}
	}

	return rpmb_dev;
}

/* Write data to rpmb device.
 * @rpmb_frame: input&output RPMB frame.
 * @buf: input pure data.
 * @size: how many bytes are requested to write.
 *
 * Note: now only single block writing is supported.
 */
int ss_rpmb_write_block(struct rpmb_frame *rpmb_frame, u8 *buf, size_t size)
{
	int result = -1;
	struct block_device *bdev;

#ifdef RPMB_MEASURE_TIME
	struct timespec time1;
	struct timespec time2;
	struct timespec time3;
#endif

	BUG_ON(size % RPMB_SECOTR);
	BUG_ON(!buf);

#ifdef RPMB_MEASURE_TIME
	ktime_get_ts(&time1);
#endif

	bdev = blkdev_get_by_dev(rpmb_device(), FMODE_READ | FMODE_WRITE, NULL);
	if (bdev == NULL) {
		tzlog_print(TZLOG_ERROR,
			    "Failed to get block device /dev/mmcblk0rpmb.\n");
		return -ENXIO;
	}
#ifdef RPMB_MEASURE_TIME
	ktime_get_ts(&time2);
#endif

	memcpy(&frames_in[0], rpmb_frame, sizeof(struct rpmb_frame));
	memcpy(&frames_in[0].data, buf, RPMB_SECOTR);
	memset(&frames_in[0].stuff, 0, RPMB_FRAME_STUFF_SIZE);
	memset(&frames_in[0].nonce, 0, RPMB_FRAME_NONCE_SIZE);
	frames_in[0].result = 0;

	memset(&frames_in[1], 0, sizeof(struct rpmb_frame));
	memset(&frame_out, 0, sizeof(struct rpmb_frame));

	/*
	Debug_rpmb_frame(&frames_in[0]);
	Debug_rpmb_frame(&frames_in[1]);
	*/

	result = mmc_rpmb_op(bdev, frames_in, &frame_out, 1);
	if (result < 0 || frame_out.result) {
		tzlog_print(TZLOG_ERROR,
			    "Failed to write data to rpmb. result = %d target->result = 0x%04x\n",
			    result, frame_out.result);
		result = -EBUSY;
		goto write_exit;
	}

	result = size;

write_exit:
	memcpy(rpmb_frame, &frame_out, sizeof(struct rpmb_frame));
	blkdev_put(bdev, FMODE_READ | FMODE_WRITE);

#ifdef RPMB_MEASURE_TIME
	ktime_get_ts(&time3);
	tzlog_print(TZLOG_DEBUG,
		    " Get block device takes: %d seconds %d nanoseconds\n",
		    time2.tv_sec - time1.tv_sec, time2.tv_nsec - time1.tv_nsec);
	tzlog_print(TZLOG_DEBUG,
		    " Writing one block takes: %d seconds %d nanoseconds\n",
		    time3.tv_sec - time1.tv_sec, time3.tv_nsec - time1.tv_nsec);
#endif

	return result;
}

/* Read data from rpmb device.
 * @rpmb_frame: input&output RPMB frame.
 * @buf: output pure data.
 * @size: how many bytes are requested to read.
 */
int ss_rpmb_read_block(struct rpmb_frame *rpmb_frame, u8 *buf, size_t size)
{
	int blk_cnt;
	char *p = buf;
	struct rpmb_frame *pframe_out = NULL;
	char *buffer = NULL;
	int result = -1;
	struct block_device *bdev = NULL;

	BUG_ON(size % RPMB_SECOTR);
	BUG_ON(!buf);

	bdev = blkdev_get_by_dev(rpmb_device(), FMODE_READ | FMODE_WRITE, NULL);
	if (bdev == NULL) {
		tzlog_print(TZLOG_ERROR,
			    "Failed to get block device /dev/mmcblk0rpmb.\n");
		return -ENXIO;
	}

	blk_cnt = size / RPMB_SECOTR;
	buffer = (char *)vmalloc(blk_cnt * EMMC_SECOTR_SIZE);
	if (NULL == buffer) {
		tzlog_print(TZLOG_ERROR,
			    "Failed to allocate memory with vmalloc.\n");
		result = -ENOMEM;
		goto read_exit;
	}

	pframe_out = (struct rpmb_frame *)buffer;
	memset((void *)pframe_out, 0, blk_cnt * EMMC_SECOTR_SIZE);

	memset(&frames_in[0], 0, sizeof(struct rpmb_frame));
	memcpy(&frames_in[0].nonce, rpmb_frame->nonce, RPMB_FRAME_NONCE_SIZE);
	frames_in[0].addr = rpmb_frame->addr;
	frames_in[0].req_resp = cpu_to_be16(MMC_RPMB_READ);

	result = mmc_rpmb_op(bdev, frames_in, pframe_out, blk_cnt);
	if (result < 0 || pframe_out->result) {
		tzlog_print(TZLOG_ERROR,
			    "Failed to read data from rpmb. result = %d target->result = 0x%04x\n",
			    result, pframe_out->result);
		result = -EBUSY;
		goto read_exit;
	}

	while (blk_cnt-- > 0) {
		memcpy(p, pframe_out->data, RPMB_SECOTR);
		if (!blk_cnt) {
			memcpy((void *)rpmb_frame, (void *)pframe_out,
			       sizeof(struct rpmb_frame));
		}

		p += RPMB_SECOTR;
		pframe_out++;
	}

	result = size;

read_exit:
	if (buffer)
		vfree(buffer);

	blkdev_put(bdev, FMODE_READ | FMODE_WRITE);

	return result;

}

/* Get RPMB's write counter.
 * @wctr: the value of the write counter.
 */
int ss_rpmb_get_wctr(u32 *wctr)
{
	int result = -1;
	struct block_device *bdev;

	bdev = blkdev_get_by_dev(rpmb_device(), FMODE_READ | FMODE_WRITE, NULL);
	if (bdev == NULL) {
		tzlog_print(TZLOG_ERROR,
			    "Failed to get block device /dev/mmcblk0rpmb.\n");
		return -ENXIO;
	}

	memset(&frames_in[0], 0, sizeof(struct rpmb_frame));
	frames_in[0].req_resp = cpu_to_be16(MMC_RPMB_READ_CNT);
	memset(&frame_out, 0, sizeof(struct rpmb_frame));

	result = mmc_rpmb_op(bdev, frames_in, &frame_out, 1);
	if (result < 0 || frame_out.result) {

		if (frame_out.result == RPMB_KEY_NO_PROGRAM_FLAG) {
			result = -RPMB_KEY_NO_PROGRAM_FLAG;
			tzlog_print(TZLOG_ERROR,
				    "Rpmb key has not been programed. result = %d rpmb_frame.result = 0x%04x\n",
				    result, frame_out.result);
		} else {
			tzlog_print(TZLOG_ERROR,
				    "Failed to get write counter. result = %d rpmb_frame.result = 0x%04x\n",
				    result, frame_out.result);

		}
		goto exit;
	}

	*wctr = frame_out.write_counter;

exit:
	blkdev_put(bdev, FMODE_READ | FMODE_WRITE);

	return result;
}

#endif /* defined(CONFIG_MMC) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)) */

#endif /* CONFIG_SECOS_NO_SECURE_STORAGE */
#endif /* CONFIG_SECOS_NO_RPMB */
