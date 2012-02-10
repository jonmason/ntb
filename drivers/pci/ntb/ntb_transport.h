/*
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 *   redistributing this file, you may do so under either license.
 *
 *   GPL LICENSE SUMMARY
 *
 *   Copyright(c) 2012 Intel Corporation. All rights reserved.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of version 2 of the GNU General Public License as
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *   The full GNU General Public License is included in this distribution
 *   in the file called LICENSE.GPL.
 *
 *   BSD LICENSE
 *
 *   Copyright(c) 2012 Intel Corporation. All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copy
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Intel PCIe NTB Linux driver
 *
 * Contact Information:
 * Jon Mason <jon.mason@intel.com>
 */

struct ntb_transport_qp;

struct ntb_queue_entry {
	/* ntb_queue list reference */
	struct list_head entry;
	/* pointers to data to be transfered */
	struct scatterlist sg;
	/* number of entries on said list */
	unsigned int num_sg;//FIXME - might be unnecessary
	/* NTB is finiahed with this data and it can be reaped */
	bool done;
};

int ntb_transport_init(void);
void ntb_transport_free(void);

/**
 * ntb_transport_create_queue - Create a new NTB transport layer queue pair
 * @handler: callback function 
 *
 * Create a new NTB transport layer queue and provide the queue with a callback
 * routine.  The callback routine will be used when the transport has received
 * data on the queue or when the transport has completed the transmission of the
 * data on the queue.
 *
 * RETURNS: pointer to newly created ntb_queue, NULL on error.
 */
struct ntb_transport_qp *ntb_transport_create_queue(void (*rx_handler)(struct ntb_transport_qp *qp), void (*tx_handler)(struct ntb_transport_qp *qp));

/**
 * ntb_transport_free_queue - Frees NTB transport queue
 * @queue: NTB queue to be freed
 *
 * Frees NTB transport queue 
 */
void ntb_transport_free_queue(struct ntb_transport_qp *qp);

/**
 * ntb_transport_enqueue - Enqueue a new NTB queue entry
 * @q: NTB transport layer queue the entry is to be enqueued on
 * @entry: NTB queue entry to be enqueued
 *
 * Enqueue a new NTB queue entry onto the transport queue for transmission.
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int ntb_transport_rx_enqueue(struct ntb_transport_qp *qp, struct ntb_queue_entry *entry);
int ntb_transport_tx_enqueue(struct ntb_transport_qp *qp, struct ntb_queue_entry *entry);

/**
 * ntb_transport_dequeue - Dequeue a NTB queue entry
 * @queue: NTB transport layer queue to be dequeued from
 * 
 * Dequeue a new NTB queue entry from the transport queue specified.
 *
 * RETURNS: New NTB queue entry from the transport queue, or NULL on empty
 */
struct ntb_queue_entry *ntb_transport_tx_dequeue(struct ntb_transport_qp *qp);
struct ntb_queue_entry *ntb_transport_rx_dequeue(struct ntb_transport_qp *qp);

/**
 * ntb_transport_link_up - Notify NTB transport of client readiness to use queue
 * @queue: NTB transport layer queue to be enabled
 *
 * Notify NTB transport layer of client readiness to use queue
 */
void ntb_transport_link_up(struct ntb_transport_qp *qp);

/**
 * ntb_transport_link_down - Notify NTB transport to no longer enqueue data
 * @queue: NTB transport layer queue to be disabled
 *
 * Notify NTB transport layer of client's desire to no longer receive data on
 * transport queue specified.  It is the client's responsibility to ensure all
 * entries on queue are purged or otherwise handled appropraitely.
 */
void ntb_transport_link_down(struct ntb_transport_qp *qp);



#define LINK_EVENT	(1 << 0)

/**
 * ntb_transport_reg_event_callback - Register event callback handler
 * @handler: Callback routine
 *
 * Register the callback routine to handle all non-data transport related
 * events.  Currently this is only a `link up/down` event, but may be expanded
 * in the future.  A `link up/down` event does not specify the state of the
 * link, only that it has toggled.
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int ntb_transport_reg_event_callback(struct ntb_transport_qp *qp, void (*handler)(int status));

/**
 * ntb_transport_hw_link_query - Query hardware link state
 * @queue: NTB transport layer queue to be queried
 *
 * Query hardware link state of the NTB transport queue 
 *
 * RETURNS: true for link up or false for link down
 */
bool ntb_transport_hw_link_query(struct ntb_transport_qp *qp);
