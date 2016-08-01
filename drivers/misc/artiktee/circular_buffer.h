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

#ifndef __SHARED_INCLUDE_CHIMERA_CIRCULAR_BUFFER_H__
#define __SHARED_INCLUDE_CHIMERA_CIRCULAR_BUFFER_H__

#ifdef __KERNEL__
#if defined(__SecureOS__)
#include <core/types.h>
#include <core/usercopy.h>
#include <core/barrier.h>
#include <core/klogger.h>
#include <core/xmalloc.h>
#include <core/error.h>
#include <core/vfs.h>
#else
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/uio.h>
#include <linux/slab.h>
#include "tzlog_print.h"
#endif /* __SecureOS__ */
#else
#include <sys/types.h>
#include <sys/uio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#endif /* __KERNEL */

/*!
 * Same data type, size and format must be shared between Linux and TEE.
 */
struct chimera_ring_buffer {
	int32_t first;
	int32_t in;
	int32_t size;
	uint8_t buffer[0];
};

static inline ssize_t __chimera_read_from_buffer(
		struct chimera_ring_buffer *buffer,
		uint8_t *data,
		ssize_t length,
		size_t buf_size_limit
#ifdef __KERNEL__
		 , int user
#endif /* __KERNEL__ */
		)
{
#if defined(__KERNEL__) && defined(__SecureOS__)
	rmb();
#endif /* defined(__KERNEL__) && defined(__SecureOS__) */

	int buffer_in = buffer->in;
	int buffer_size = buffer->size;
	int buffer_first = buffer->first;
	int32_t available;
	ssize_t bytesRead;

#if defined(__KERNEL__) && defined(__SecureOS__)
	rmb();
#endif /* defined(__KERNEL__) && defined(__SecureOS__) */

	if (buffer_in < 0 || buffer_in > buffer_size || buffer_size < 32
	    || buffer_size > (0x100000) || buffer_first < 0
	    || buffer_first > buffer_size
	    || buffer_size > (int)buf_size_limit - (int)sizeof(struct chimera_ring_buffer)) {
#ifdef __KERNEL__
#ifdef __SecureOS__
		printk(KERN_PANIC,
		       "Queue corrupted, in = %d, out = %d, size = %d\n",
		       buffer_in, buffer_first, buffer_size);
#else
		tzlog_print(TZLOG_ERROR,
			    "Queue corrupted, in = %d, out = %d, size = %d\n",
			    buffer_in, buffer_first, buffer_size);
#endif /* __SecureOS__ */
#endif /* __KERNEL__ */
		return -EFAULT;
	}

	available = buffer_in;

	if (length > available) {
		length = available;
	}

	if (length <= 0) {
		return 0;
	}

	bytesRead = length;

	if (buffer_first + length <= buffer_size) {
		/* simple copy */
#ifdef __KERNEL__
		if (user) {
#if defined(__SecureOS__)
			if (memcpy_user
			    (data, buffer->buffer + buffer_first, length) < 0) {
				return -EFAULT;
			}
#else
			if (copy_to_user
			    (data, buffer->buffer + buffer_first, length)) {
				return -EFAULT;
			}
#endif /* defined(__SecureOS__) */
		} else
#endif /* __KERNEL__ */
			memcpy(data, buffer->buffer + buffer_first, (size_t)length);
	} else {
		/* need to copy both ends */
		size_t upper = (size_t)buffer_size - (size_t)buffer_first;
		size_t lower = (size_t)length - upper;

#ifdef __KERNEL__
		if (user) {
#if defined(__SecureOS__)
			if (memcpy_user
			    (data, buffer->buffer + buffer_first, upper) < 0) {
				return -EFAULT;
			}

			if (memcpy_user(data + upper, buffer->buffer, lower) <
			    0) {
				return -EFAULT;
			}
#else
			if (copy_to_user
			    (data, buffer->buffer + buffer_first, upper)) {
				return -EFAULT;
			}

			if (copy_to_user(data + upper, buffer->buffer, lower)) {
				return -EFAULT;
			}
#endif /* defined(__SecureOS__) */
		} else
#endif /* __KERNEL__ */
		{
			memcpy(data, buffer->buffer + buffer_first, upper);
			memcpy(data + upper, buffer->buffer, lower);
		}
	}

#if defined(__KERNEL__) && defined(__SecureOS__)
	wmb();
#endif /* defined(__KERNEL__) && defined(__SecureOS__) */

	buffer->first = (buffer_first + bytesRead) % buffer_size;
	buffer->in = buffer_in - bytesRead;

#if defined(__KERNEL__) && defined(__SecureOS__)
	mb();
#endif /* defined(__KERNEL__) && defined(__SecureOS__) */

	return bytesRead;
}

static inline ssize_t __chimera_write_to_buffer(
		struct chimera_ring_buffer *buffer,
		const uint8_t *data,
		ssize_t length,
		size_t buf_size_limit
#ifdef __KERNEL__
		, int user
#endif /* __KERNEL__ */
		)
{
#if defined(__KERNEL__) && defined(__SecureOS__)
	rmb();
#endif /* defined(__KERNEL__) && defined(__SecureOS__) */

	int buffer_in = buffer->in;
	int buffer_size = buffer->size;
	int buffer_first = buffer->first;
	int32_t left;
	ssize_t bytesWritten;
	int32_t position;

#if defined(__KERNEL__) && defined(__SecureOS__)
	rmb();
#endif /* defined(__KERNEL__) && defined(__SecureOS__) */

	if (buffer_in < 0 || buffer_in > buffer_size || buffer_size < 32
	    || buffer_size > (0x100000) || buffer_first < 0
	    || buffer_first > buffer_size
	    || buffer_size > (int)buf_size_limit - (int)sizeof(struct chimera_ring_buffer)) {
#ifdef __KERNEL__
#ifdef __SecureOS__
		printk(KERN_PANIC,
		       "Queue corrupted, in = %d, out = %d, size = %d\n",
		       buffer_in, buffer_first, buffer_size);
#else
		tzlog_print(TZLOG_ERROR,
			    "Queue corrupted, in = %d, out = %d, size = %d\n",
			    buffer_in, buffer_first, buffer_size);
#endif /* __SecureOS__ */
#endif /* __KERNEL__ */
		return -EFAULT;
	}

	left = buffer_size - buffer_in;

	if (length > left) {
		length = left;
	}

	if (length <= 0) {
		return 0;
	}

	bytesWritten = length;
	position = (buffer_first + buffer_in) % buffer_size;

	if (position + length <= buffer_size) {
		/* simple copy */
#ifdef __KERNEL__
		if (user) {
#if defined(__SecureOS__)
			if (memcpy_user(buffer->buffer + position, data, length)
			    < 0) {
				return -EFAULT;
			}
#else
			if (copy_from_user
			    (buffer->buffer + position, data, length)) {
				return -EFAULT;
			}
#endif /* defined(__SecureOS__) */
		} else
#endif /* __KERNEL__ */
			memcpy(buffer->buffer + position, data, (size_t)length);
	} else {
		/* need to copy both ends */
		size_t upper = (size_t)buffer_size - (size_t)position;
		size_t lower = (size_t)length - upper;

#ifdef __KERNEL__
		if (user) {
#if defined(__SecureOS__)
			if (memcpy_user(buffer->buffer + position, data, upper)
			    < 0
			    || memcpy_user(buffer->buffer, data + upper,
					   lower) < 0) {
				return -EFAULT;
			}
#else
			if (copy_from_user
			    (buffer->buffer + position, data, upper) != 0
			    || copy_from_user(buffer->buffer, data + upper,
					      lower) != 0) {
				return -EFAULT;
			}
#endif /* defined(__SecureOS__) */
		} else
#endif /* __KERNEL__ */
		{
			memcpy(buffer->buffer + position, data, upper);
			memcpy(buffer->buffer, data + upper, lower);
		}
	}

	buffer->in = buffer_in + bytesWritten;

	return bytesWritten;
}

static inline void chimera_ring_buffer_clear(struct chimera_ring_buffer *buffer)
{
	buffer->in = 0;
	buffer->first = 0;
}

static inline struct chimera_ring_buffer *chimera_create_ring_buffer_etc(
		void *memory,
		size_t size,
		int init_from_memory
#if defined(__KERNEL__)
#if defined(__SecureOS__)
		, int wait_flags
#else
		, gfp_t gfp
#endif /* defined(__SecureOS__) */
#endif /* defined(__KERNEL__) */
		)
{
	if (memory == NULL) {
#if defined(__KERNEL__)
#if defined(__SecureOS__)
		struct chimera_ring_buffer *buffer =
		    (struct chimera_ring_buffer *)
		    malloc(sizeof(struct chimera_ring_buffer) + size,
			   M_DEVBUF,
			wait_flags);
#else
		struct chimera_ring_buffer *buffer =
		    (struct chimera_ring_buffer *)
		    kmalloc(sizeof(struct chimera_ring_buffer) + size, gfp);
#endif /* defined(__SecureOS__) */
#else
		struct chimera_ring_buffer *buffer =
		    (struct chimera_ring_buffer *)
		    malloc(sizeof(struct chimera_ring_buffer) + size);
#endif /* defined(__KERNEL__) */
		if (buffer == NULL) {
			return NULL;
		}

		buffer->size = (int32_t)size;
		chimera_ring_buffer_clear(buffer);

		return buffer;
	} else {
		struct chimera_ring_buffer *buffer =
		    (struct chimera_ring_buffer *)memory;
		size -= sizeof(struct chimera_ring_buffer);

		buffer->size = (int32_t)size;
		if (init_from_memory && (size_t) buffer->size == size
		    && buffer->in >= 0 && (size_t) buffer->in <= size
		    && buffer->first >= 0 && (size_t) buffer->first < size) {
			/* structure looks valid */
		} else {
			chimera_ring_buffer_clear(buffer);
		}

		return buffer;
	}
}

static inline struct chimera_ring_buffer *chimera_create_ring_buffer(
		void *memory,
		size_t size
#if defined(__KERNEL__)
#if defined(__SecureOS__)
		, int wait_flags
#else
		, gfp_t gfp
#endif /* defined(__SecureOS__) */
#endif /* defined(__KERNEL__) */
		)
{
#if defined(__KERNEL__)
#if defined(__SecureOS__)
	return chimera_create_ring_buffer_etc(memory, size, 0, wait_flags);
#else
	return chimera_create_ring_buffer_etc(memory, size, 0, gfp);
#endif /* defined(__SecureOS__) */
#else
	return chimera_create_ring_buffer_etc(memory, size, 0);
#endif /* defined(__KERNEL__) */
}

static inline void chimera_ring_buffer_delete(
		struct chimera_ring_buffer *buffer)
{
#if defined(__KERNEL__)
#if defined(__SecureOS__)
	free(buffer, M_DEVBUF);
#else
	kfree(buffer);
#endif /* defined(__SecureOS__) */
#else
	free(buffer);
#endif /* defined(__KERNEL__) */
}

static inline size_t chimera_ring_buffer_readable(
		struct chimera_ring_buffer *buffer)
{
	return (size_t)buffer->in;
}

static inline size_t chimera_ring_buffer_writable(
		struct chimera_ring_buffer *buffer)
{
	return (size_t)(buffer->size - buffer->in);
}

static inline void chimera_ring_buffer_flush(
		struct chimera_ring_buffer *buffer,
		size_t length)
{
	/* we can't flush more bytes than there are */
	if (length > (size_t)buffer->in) {
		length = (size_t)buffer->in;
	}

	buffer->in -= (int32_t)length;
	buffer->first = (buffer->first + (int32_t)length) % buffer->size;
}

static inline ssize_t chimera_ring_buffer_read(
							struct chimera_ring_buffer *buffer,
							uint8_t *data,
							ssize_t length,
							size_t buf_size_limit)
{
#ifdef __KERNEL__
	return __chimera_read_from_buffer(buffer, data, length, buf_size_limit,
					  0);
#else
	return __chimera_read_from_buffer(buffer, data, length, buf_size_limit);
#endif /* __KERNEL__ */
}

static inline ssize_t chimera_ring_buffer_write(
		struct chimera_ring_buffer *buffer,
		const uint8_t *data,
		ssize_t length,
		size_t buf_size_limit)
{
#ifdef __KERNEL__
	return __chimera_write_to_buffer(
			buffer,
			data,
			length,
			buf_size_limit,
			0);
#else
	return __chimera_write_to_buffer(
			buffer,
			data,
			length,
			buf_size_limit);
#endif /* __KERNEL__ */
}

#ifdef __KERNEL__
static inline ssize_t chimera_ring_buffer_user_read(
		struct chimera_ring_buffer *buffer,
		uint8_t *data,
		ssize_t length,
		size_t buf_size_limit)
{
	return __chimera_read_from_buffer(
			buffer,
			data,
			length,
			buf_size_limit,
			1);
}

static inline ssize_t chimera_ring_buffer_user_write(
		struct chimera_ring_buffer *buffer,
		const uint8_t *data,
		ssize_t length,
		size_t buf_size_limit)
{
	return __chimera_write_to_buffer(
			buffer,
			data,
			length,
			buf_size_limit,
			1);
}
#endif /* __KERNEL__ */

/*!	Reads data from the ring buffer, but doesn't remove the data from it.
 * 	\param buffer The ring buffer.
 * 	\param offset The offset relative to the beginning of the data in the ring
 * 		buffer at which to start reading.
 * 	\param data The buffer to which to copy the data.
 * 	\param length The number of bytes to read at maximum.
 * 	\return The number of bytes actually read from the buffer.
 */
static inline ssize_t chimera_ring_buffer_peek(
		struct chimera_ring_buffer *buffer,
		size_t offset,
		void *data, size_t length,
		size_t buf_size_limit)
{
#if defined(__KERNEL__) && defined(__SecureOS__)
	rmb();
#endif

	int buffer_in = buffer->in;
	int buffer_size = buffer->size;
	int buffer_first = buffer->first;

	size_t available;

	if (buffer_in < 0 || buffer_in > buffer_size || buffer_size < 32
	    || buffer_size > (0x100000) || buffer_first < 0
	    || buffer_first > buffer_size
	    || buffer_size >
	    (int)buf_size_limit - (int)sizeof(struct chimera_ring_buffer)) {
#ifdef __KERNEL__
#ifdef __SecureOS__
		printk(KERN_PANIC,
			"Queue corrupted, in = %d, out = %d, size = %d\n",
			buffer_in, buffer_first, buffer_size);
#else
		tzlog_print(TZLOG_ERROR,
			"Queue corrupted, in = %d, out = %d, size = %d\n",
			buffer_in, buffer_first, buffer_size);
#endif /* __SecureOS__ */
#endif /* __KERNEL__ */
		return -EFAULT;
	}

	available = (size_t)buffer_in;

	if (offset >= available || length == 0) {
		return 0;
	}

	if (offset + length > available) {
		length = available - offset;
	}

	offset += (size_t)buffer_first;

	if (offset >= (size_t) buffer_size) {
		offset -= (size_t)buffer_size;
	}

	if (offset + length <= (size_t) buffer->size) {
		/* simple copy */
		memcpy(data, buffer->buffer + offset, length);
	} else {
		/* need to copy both ends */
		size_t upper = (size_t)buffer_size - offset;
		size_t lower = length - upper;

		memcpy(data, buffer->buffer + offset, upper);
		memcpy((uint8_t *) data + upper, buffer->buffer, lower);
	}

	return (ssize_t)length;
}

/*!	Returns iovecs describing the contents of the ring buffer.
 * 	\param buffer The ring buffer.
 * 	\param vecs Pointer to an iovec array with at least 2 elements to be filled
 * 		in by the function.
 * 	\return The number of iovecs the function has filled in to describe the
 * 		contents of the ring buffer. \c 0, if empty, \c 2 at maximum.
 */
static inline int chimera_ring_buffer_get_vecs(
		struct chimera_ring_buffer *buffer,
#if defined(__SecureOS__) && defined(__KERNEL__)
		struct vfs_iovec *vecs,
#else
		struct iovec *vecs,
#endif
		size_t buf_size_limit)
{
	int buffer_in = buffer->in;
	int buffer_size = buffer->size;
	int buffer_first = buffer->first;

	size_t upper;
	size_t lower;

	if (buffer_in < 0 || buffer_in > buffer_size || buffer_size < 32
		|| buffer_size > (0x100000) || buffer_first < 0
		|| buffer_first > buffer_size
		|| buffer_size >  (int)buf_size_limit - (int)sizeof(struct chimera_ring_buffer)) {
#ifdef __KERNEL__
#ifdef __SecureOS__
		printk(KERN_PANIC,
			"Queue corrupted, in = %d, out = %d, size = %d\n",
			buffer_in, buffer_first, buffer_size);
#else
		tzlog_print(TZLOG_ERROR,
			"Queue corrupted, in = %d, out = %d, size = %d\n",
			buffer_in, buffer_first, buffer_size);
#endif /* __SecureOS__ */
#endif /* __KERNEL__ */
		return -EFAULT;
	}

	if (buffer_in == 0) {
		return 0;
	}

	if (buffer_first + buffer_in <= buffer_size) {
		/* one element */
#if defined(__SecureOS__) && defined(__KERNEL__)
		vecs[0].buffer = buffer->buffer + buffer_first;
		vecs[0].length = buffer_in;
#else
		vecs[0].iov_base = buffer->buffer + buffer_first;
		vecs[0].iov_len = (size_t)buffer_in;
#endif
		return 1;
	}

	/* two elements */
	upper = (size_t)buffer_size - (size_t)buffer_first;
	lower = (size_t)buffer_in - upper;

#if defined(__SecureOS__) && defined(__KERNEL__)
	vecs[0].buffer = buffer->buffer + buffer_first;
	vecs[0].length = upper;
	vecs[1].buffer = buffer->buffer;
	vecs[1].length = lower;
#else
	vecs[0].iov_base = buffer->buffer + buffer_first;
	vecs[0].iov_len = upper;
	vecs[1].iov_base = buffer->buffer;
	vecs[1].iov_len = lower;
#endif

	return 2;
}

#endif /* __SHARED_INCLUDE_CHIMERA_CIRCULAR_BUFFER_H__ */
