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

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/pci.h>
#include <linux/export.h>

#include "ntb_dev.h"
#include "ntb_transport.h"

struct ntb_transport_qp {
	struct list_head txq;
	struct list_head tx_comp_q;
	void *tx_mw_offset;

	struct list_head rxq;
	struct list_head rx_comp_q;
	void *rx_mw_offset;

	void (*rx_handler)(struct ntb_transport_qp *qp);
	void (*tx_handler)(struct ntb_transport_qp *qp);
	void (*event_handler)(int status);
	struct delayed_work event_work;
	unsigned int event_flags:1;
	unsigned int link:1;
	unsigned int qp_num:6;/* Only 64 QP's are allowed.  0-63 */

	/* Stats */
	u64 rx_bytes;
	u64 rx_pkts;
	u64 rx_err_no_buf;
	u64 rx_err_oflow;
	u64 tx_bytes;
	u64 tx_pkts;
};

struct ntb_transport {
	struct ntb_device *ndev;

	struct {
		size_t size;
		void *virt_addr;
		dma_addr_t dma_addr;
	} mw[NTB_NUM_MW];

	unsigned int max_qps;
	struct ntb_transport_qp *qps;
	unsigned long qp_bitmap;
};

struct ntb_payload_header {
	unsigned int len;
	u32 ver;
	u32 done:1;
	u32 wrap:1;
};


#define QLEN(rxq, txq, size)	((rxq < txq) ? size - (txq - rxq) : rxq - txq)
#define DB_TO_MW(db)		(((db) / 4) % 2)
#define QP_TO_MW(qp)		DB_TO_MW(qp)


struct ntb_transport *transport = NULL;

static void ntb_transport_event_callback(void *handle, unsigned int event) //FIXME - handle is unnecessary, what would it be used for ever?????
{
	int i;

	for (i = 0; i < transport->max_qps; i++)
		if (!test_bit(i, &transport->qp_bitmap) && transport->qps[i].event_handler) {
			struct ntb_transport_qp *qp = &transport->qps[i];

			qp->event_flags |= event; //FIXME - racy and only 1 bit
			schedule_delayed_work(&qp->event_work, 0);
		}
}

int ntb_transport_init(void)
{
	int rc, i;

	if (transport)
		return -EINVAL;//FIXME - add refcount

	transport = kmalloc(sizeof(struct ntb_transport), GFP_KERNEL);
	if (!transport)
		return -ENOMEM;

	transport->ndev = ntb_register_transport(transport);
	if (!transport->ndev) {
		rc = -EIO;
		goto err;
	}

	/* 1 Doorbell bit is used by Link/HB, but the rest can be used for the transport qp's */
	transport->max_qps = ntb_query_db_bits(transport->ndev) - 1;
	if (!transport->max_qps) {
		rc = -EIO;
		goto err1;
	}

	transport->qps = kcalloc(transport->max_qps, sizeof(struct ntb_transport_qp), GFP_KERNEL);
	if (!transport->qps) {
		rc = -ENOMEM;
		goto err1;
	}

	transport->qp_bitmap = (1 << transport->max_qps) - 1;

	for (i = 0; i < NTB_NUM_MW; i++) {
		/* Alloc memory for receiving data, Must be 4k aligned */
		transport->mw[i].size = ALIGN(ntb_get_mw_size(transport->ndev, i), 4096);

		//FIXME - might not need to be coherent if we don't ever touch it by the cpu on this side
		transport->mw[i].virt_addr = dma_alloc_coherent(&transport->ndev->pdev->dev, transport->mw[i].size, &transport->mw[i].dma_addr, GFP_KERNEL);
		if (!transport->mw[i].virt_addr) {
			rc = -ENOMEM;
			goto err2;
		}

		/* Notify HW the memory location of the receive buffer */
		ntb_set_mw_addr(transport->ndev, i, transport->mw[i].dma_addr);
	}

	rc = ntb_register_event_callback(transport->ndev, ntb_transport_event_callback);
	if (rc)
		goto err2;

	return 0;

err2:
	for (i--; i >= 0; i--)
		  dma_free_coherent(&transport->ndev->pdev->dev, transport->mw[i].size, transport->mw[i].virt_addr, transport->mw[i].dma_addr);
	kfree(transport->qps);
err1:
	ntb_unregister_transport(transport->ndev);
err:
	kfree(transport);
	return rc;
} 
EXPORT_SYMBOL(ntb_transport_init);

void ntb_transport_free(void)
{
	int i;

	//FIXME - verify that the event and db callbacks are empty?

	for (i = 0; i < NTB_NUM_MW; i++)
		  dma_free_coherent(&transport->ndev->pdev->dev, transport->mw[i].size, transport->mw[i].virt_addr, transport->mw[i].dma_addr);
	kfree(transport->qps);
	ntb_unregister_transport(transport->ndev);
	kfree(transport);//FIXME - add refcount
}
EXPORT_SYMBOL(ntb_transport_free);

static void ntb_transport_dbcb(int db_num)
{
	struct ntb_transport_qp *qp = &transport->qps[db_num];
	struct device *dev = &transport->ndev->pdev->dev;
	struct ntb_queue_entry *entry;
	struct ntb_payload_header *hdr;

	if (!qp || !qp->link)
		return;

	//FIXME - need to have error paths update the rx_mw_offset, otherwise the tx and rx will get out of sync

	dev_info(dev, "%s: doorbell %d received\n", __func__, db_num);

	hdr = qp->rx_mw_offset;
	dev_info(dev, "Header len %d, ver %x, wrap %x, done %x\n", hdr->len, hdr->ver, hdr->wrap, hdr->done);

	if (!hdr->done) {
		dev_err(dev, "Desc not done\n");
		return;
	}

	if (hdr->wrap) {
		qp->rx_mw_offset = ntb_get_mw_vbase(transport->ndev, DB_TO_MW(db_num));

		/* update scratchpad register keeping track of current location, which would have an offset of 0 here */
		ntb_write_spad(transport->ndev, 0, 0); //FIXME - handle rc
		ntb_write_spad(transport->ndev, 1, 0); //FIXME - handle rc
		dev_err(dev, "Wrap!\n");
		return;
	}

	if (list_empty(&qp->rxq)) {
		dev_info(dev, "No RX buf\n");
		qp->rx_err_no_buf++;
		return;
	}

	entry = list_first_entry(&qp->rxq, struct ntb_queue_entry, entry);
	list_del(&entry->entry);

	dev_info(dev, "%d payload received, buf size %d\n", hdr->len, entry->len);
	if (hdr->len > entry->len) {
		dev_info(dev, "RX overflow\n");
		list_add_tail(&entry->entry, &qp->rxq);
		qp->rx_err_oflow++;
		return;
	}

	qp->rx_mw_offset += sizeof(struct ntb_payload_header);

	//FIXME - doesn't handle multple packets in the buffer
	memcpy(entry->buf, qp->rx_mw_offset, hdr->len);
	print_hex_dump_bytes(" ", 0, qp->rx_mw_offset, hdr->len);
	entry->len = hdr->len;

	qp->rx_mw_offset += hdr->len;

#if 0
	/* update scratchpad register keeping track of current location, which is the offset of the base */
	offset = qp->rx_mw_offset - ntb_get_mw_vbase(transport->ndev, DB_TO_MW(db_num));
	ntb_write_spad(transport->ndev, 0, offset >> 32); //FIXME - handle rc
	ntb_write_spad(transport->ndev, 1, offset & 0xffffffff); //FIXME - handle rc
#endif

	qp->rx_bytes += hdr->len;
	qp->rx_pkts++;

	list_add_tail(&entry->entry, &qp->rx_comp_q);

	//FIXME - this should kick off wq thread or we should document that it runs in irq context
	qp->rx_handler(qp);
}

/**
 * ntb_transport_create_queue - Create a new NTB transport layer queue
 * @rx_handler: receive callback function 
 * @tx_handler: transmit callback function 
 *
 * Create a new NTB transport layer queue and provide the queue with a callback
 * routine for both transmit and receive.  The receive callback routine will be
 * used to pass up data when the transport has received it on the queue.   The
 * transmit callback routine will be called when the transport has completed the
 * transmission of the data on the queue and the data is ready to be freed.
 *
 * RETURNS: pointer to newly created ntb_queue, NULL on error.
 */
struct ntb_transport_qp *
ntb_transport_create_queue(handler rx_handler, handler tx_handler)
{
	struct ntb_transport_qp *qp;
	unsigned int free_queue;
	int rc;

	//FIXME - need to handhake with remote side to determine matching number or some mapping between the 2
	free_queue = ffs(transport->qp_bitmap);
	if (!free_queue)
		return NULL;

	/* decrement free_queue to make it zero based */
	free_queue--;

	clear_bit(free_queue, &transport->qp_bitmap);//FIXME - this might be racy, either add lock or make atomic

	qp = &transport->qps[free_queue];
	qp->qp_num = free_queue;

	qp->rx_handler = rx_handler;
	qp->tx_handler = tx_handler;

	INIT_LIST_HEAD(&qp->rxq);
	INIT_LIST_HEAD(&qp->rx_comp_q);
	INIT_LIST_HEAD(&qp->txq);
	INIT_LIST_HEAD(&qp->tx_comp_q);

	qp->tx_mw_offset = ntb_get_mw_vbase(transport->ndev, QP_TO_MW(free_queue));
	qp->rx_mw_offset = transport->mw[QP_TO_MW(free_queue)].virt_addr;
	//FIXME - transport->qps[free_queue].queue.hw_caps; is it even necessary for transport client to know?

	rc = ntb_register_db_callback(transport->ndev, free_queue, ntb_transport_dbcb);
	if (rc) {
		set_bit(free_queue, &transport->qp_bitmap);
		return NULL;
	}

	return qp;
}
EXPORT_SYMBOL(ntb_transport_create_queue);


static void ntb_purge_list(struct list_head *list)
{
	while (!list_empty(list)) {
		struct device *dev = &transport->ndev->pdev->dev;

		dev_warn(dev, "Freeing item from a non-empty queue\n");
		list_del(list->next);
	}
}

/**
 * ntb_transport_free_queue - Frees NTB transport queue
 * @qp: NTB queue to be freed
 *
 * Frees NTB transport queue 
 */
void ntb_transport_free_queue(struct ntb_transport_qp *qp)
{
	if (!qp)
		return;

	qp->link = 0;
	ntb_unregister_db_callback(transport->ndev, qp->qp_num);
	//FIXME - wait for qps to quience or notify the transport to stop?

	ntb_purge_list(&qp->rxq);
	ntb_purge_list(&qp->rx_comp_q);
	ntb_purge_list(&qp->txq);
	ntb_purge_list(&qp->tx_comp_q);

	set_bit(qp->qp_num, &transport->qp_bitmap);

	if (transport->qp_bitmap == ~(transport->max_qps - 1))
		ntb_transport_free();
}
EXPORT_SYMBOL(ntb_transport_free_queue);

/**
 * ntb_transport_rx_enqueue - Enqueue a new NTB queue entry
 * @qp: NTB transport layer queue the entry is to be enqueued on
 * @entry: NTB queue entry to be enqueued
 *
 * Enqueue a new NTB queue entry onto the transport queue into which a NTB
 * payload can be received into.
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int ntb_transport_rx_enqueue(struct ntb_transport_qp *qp, struct ntb_queue_entry *entry)
{
	if (!qp)
		return -EINVAL;

	list_add_tail(&entry->entry, &qp->rxq);

	return 0;
}
EXPORT_SYMBOL(ntb_transport_rx_enqueue);

/**
 * ntb_transport_tx_enqueue - Enqueue a new NTB queue entry
 * @qp: NTB transport layer queue the entry is to be enqueued on
 * @entry: NTB queue entry to be enqueued
 *
 * Enqueue a new NTB queue entry onto the transport queue from which a NTB
 * payload will be transmitted.
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int ntb_transport_tx_enqueue(struct ntb_transport_qp *qp, struct ntb_queue_entry *entry)
{
	int rc, mw;
	struct ntb_payload_header *hdr;
	void *payload;
	u64 mw_base;
	u32 mw_size;

	//FIXME - enqueue to qp->txq and start a thread to copy data over

	if (!qp || qp->link != NTB_LINK_UP)
		return -EINVAL;

	mw = QP_TO_MW(qp->qp_num);
	mw_size = transport->mw[mw].size;
	mw_base = (u64) transport->mw[mw].virt_addr;

	/* Check to see if it could ever fit in the mem window */
	if (entry->len > mw_size) {
		//FIXME - add counter for this
		return -EINVAL;
	}

	hdr = qp->tx_mw_offset;
	payload = ((void *)hdr) + sizeof(struct ntb_payload_header);

	print_hex_dump_bytes(" ", 0, entry->buf, entry->len);

	memcpy(payload, entry->buf, entry->len);

	hdr->len = entry->len;
	hdr->ver = qp->tx_pkts;
	//hdr->ver = 0xdeadbeef;
	hdr->done = 1;

	pr_info("Header len %d, ver %x, wrap %x, done %x\n", hdr->len, hdr->ver, hdr->wrap, hdr->done);

	/* Ring doorbell notifying remote side of new packet */
	rc = ntb_ring_sdb(transport->ndev, 1 << qp->qp_num);
	if (rc)
		return rc;//FIXME - this will reuse the same location, but data has already been transmitted.  is this what we want?

	qp->tx_mw_offset += sizeof(struct ntb_payload_header) + entry->len;

	qp->tx_pkts++;
	qp->tx_bytes += entry->len;

	/* Add fully transmitted data to completion queue */
	list_add_tail(&entry->entry, &qp->tx_comp_q);

	//FIXME - might want to kick off a thread/wq to handle the tx completion
	qp->tx_handler(qp);

	return 0;
//FIXME - handle errors and increment stats
}
EXPORT_SYMBOL(ntb_transport_tx_enqueue);

/**
 * ntb_transport_tx_dequeue - Dequeue a NTB queue entry
 * @qp: NTB transport layer queue to be dequeued from
 * 
 * This function will dequeue a NTB queue entry from the transmit complete
 * queue.  Entries will only be enqueued on this queue after having been
 * transfered to the remote side.
 *
 * RETURNS: NTB queue entry from the transport queue, or NULL on empty
 */
struct ntb_queue_entry *ntb_transport_tx_dequeue(struct ntb_transport_qp *qp)
{
	struct ntb_queue_entry *entry;

	if (list_empty(&qp->tx_comp_q))
		return NULL;
	
	entry = list_first_entry(&qp->tx_comp_q, struct ntb_queue_entry, entry);
	list_del(&entry->entry);

	return entry;
}
EXPORT_SYMBOL(ntb_transport_tx_dequeue);

/**
 * ntb_transport_rx_dequeue - Dequeue a NTB queue entry
 * @qp: NTB transport layer queue to be dequeued from
 * 
 * This function will dequeue a new NTB queue entry from the receive complete
 * queue of the transport queue specified.  Entries will only be enqueued on
 * this queue after having been fully received.
 *
 * RETURNS: NTB queue entry from the transport queue, or NULL on empty
 */
struct ntb_queue_entry *ntb_transport_rx_dequeue(struct ntb_transport_qp *qp)
{
	struct ntb_queue_entry *entry;

	if (list_empty(&qp->rx_comp_q))
		return NULL;
	
	entry = list_first_entry(&qp->rx_comp_q, struct ntb_queue_entry, entry);
	list_del(&entry->entry);

	return entry;
}
EXPORT_SYMBOL(ntb_transport_rx_dequeue);

/**
 * ntb_transport_link_up - Notify NTB transport of client readiness to use queue
 * @qp: NTB transport layer queue to be enabled
 *
 * Notify NTB transport layer of client readiness to use queue
 */
void ntb_transport_link_up(struct ntb_transport_qp *qp)
{
	if (!qp)
		return;

	qp->link = 1;
}
EXPORT_SYMBOL(ntb_transport_link_up);

/**
 * ntb_transport_link_down - Notify NTB transport to no longer enqueue data
 * @qp: NTB transport layer queue to be disabled
 *
 * Notify NTB transport layer of client's desire to no longer receive data on
 * transport queue specified.  It is the client's responsibility to ensure all
 * entries on queue are purged or otherwise handled appropraitely.
 */
void ntb_transport_link_down(struct ntb_transport_qp *qp)
{
	if (!qp)
		return;

	qp->link = 0;
}
EXPORT_SYMBOL(ntb_transport_link_down);


static void ntb_transport_event_work(struct work_struct *work)
{
	struct ntb_transport_qp *qp = container_of(work, struct ntb_transport_qp, event_work.work);

	qp->event_handler(qp->event_flags);
}

/**
 * ntb_transport_reg_event_callback - Register event callback handler
 * @qp: NTB transport layer queue to which the callback is bound
 * @handler: Callback routine
 *
 * Register the callback routine to handle all non-data transport related
 * events.  Currently this is only a `link up/down` event, but may be expanded
 * in the future.  A `link up/down` event does not specify the state of the
 * link, only that it has toggled.
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int ntb_transport_reg_event_callback(struct ntb_transport_qp *qp, void (*handler)(int status))
{
	if (!qp)
		return -EINVAL;

	qp->event_handler = handler;

	//FIXME - workqueue might be overkill, and might not need to be delayed...
	INIT_DELAYED_WORK(&qp->event_work, ntb_transport_event_work);

	return 0;
}
EXPORT_SYMBOL(ntb_transport_reg_event_callback);

/**
 * ntb_transport_hw_link_query - Query hardware link state
 * @qp: NTB transport layer queue to be queried
 *
 * Query hardware link state of the NTB transport queue 
 *
 * RETURNS: true for link up or false for link down
 */
bool ntb_transport_hw_link_query(struct ntb_transport_qp *qp)
{
	return transport->ndev->link_status;
}
EXPORT_SYMBOL(ntb_transport_hw_link_query);
