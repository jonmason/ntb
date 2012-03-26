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

#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include "ntb_hw.h"
#include "ntb_transport.h"
#include <linux/kthread.h>

struct ntb_transport_qp {
	struct task_struct *tx_work;
	struct task_struct *txc_work;
	struct list_head txq;
	struct list_head tx_comp_q;
	void *tx_buf_begin;
	void *tx_buf_end;
	void *tx_buf_head;
	void *tx_buf_tail;

	struct task_struct *rx_work;
	struct list_head rxq;
	struct list_head rx_comp_q;
	void *rx_mw_begin;
	void *rx_mw_end;
	void *rx_mw_offset;

	void (*rx_handler)(struct ntb_transport_qp *qp);
	void (*tx_handler)(struct ntb_transport_qp *qp);
	void (*event_handler)(int status);
	struct delayed_work event_work;
	//FIXME - unless this is getting larger than a cacheline, bit fields might not be worth it
	unsigned int event_flags:1;
	unsigned int sw_link:1;
	unsigned int hw_link:1;
	unsigned int qp_num:6;/* Only 64 QP's are allowed.  0-63 */

	/* Stats */
	u64 rx_bytes;
	u64 rx_pkts;
	u64 rx_err_no_buf;
	u64 rx_err_oflow;
	u64 tx_bytes;
	u64 tx_pkts;
	u64 tx_ring_full;
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
	u32 tx_done:1;
	u32 rx_done:1;
};


#define DB_PER_QP		2

#define QP_TO_DB(qp)		((qp) * DB_PER_QP)
#define DB_TO_QP(db)		((db) / DB_PER_QP)
#define QP_TO_MW(qp)		((qp) % NTB_NUM_MW)

#define	free_len(qp)	(qp->tx_buf_head < qp->tx_buf_tail ? qp->tx_buf_tail - qp->tx_buf_head : (qp->tx_buf_end - qp->tx_buf_head) + (qp->tx_buf_tail - qp->tx_buf_begin))

struct ntb_transport *transport = NULL;

static int list_len(struct list_head *list)
{
	struct list_head *pos;
	int i = 0;

	list_for_each(pos, list)
		i++;

	return i;
}

void ntb_transport_dump_qp_stats(struct ntb_transport_qp *qp)
{
	pr_info("NTB Transport stats\n");
	pr_info("rx_bytes - %Ld\n", qp->rx_bytes);
	pr_info("rx_pkts - %Ld\n", qp->rx_pkts);
	pr_info("rx_err_no_buf - %Ld\n", qp->rx_err_no_buf);
	pr_info("rx_err_oflow - %Ld\n", qp->rx_err_oflow);
	pr_info("tx_bytes - %Ld\n", qp->tx_bytes);
	pr_info("tx_pkts - %Ld\n", qp->tx_pkts);
	pr_info("tx_ring_full - %Ld\n", qp->tx_ring_full);
	pr_info("tx begin %p, end %p, ring size %ld\n", qp->tx_buf_begin, qp->tx_buf_end, qp->tx_buf_end - qp->tx_buf_begin);
	pr_info("tx head %p, tail %p, free size %ld\n", qp->tx_buf_head, qp->tx_buf_tail, free_len(qp));
	pr_info("txq len %d, tx_comp_q len %d\n", list_len(&qp->txq), list_len(&qp->tx_comp_q));
	pr_info("rx begin %p, end %p, offset %p\n", qp->rx_mw_begin, qp->rx_mw_end, qp->rx_mw_offset);
	pr_info("rxq len %d, rx_comp_q len %d\n", list_len(&qp->rxq), list_len(&qp->rx_comp_q));
}
EXPORT_SYMBOL(ntb_transport_dump_qp_stats);

static void ntb_transport_event_callback(void *data, unsigned int event)
{
	struct ntb_transport *transport = data;
	int i;

	//FIXME - use handle as a way to pass client specified data back to it.

	/* Pass along the info to any clients */
	for (i = 0; i < transport->max_qps; i++)
		if (!test_bit(i, &transport->qp_bitmap)) {
			struct ntb_transport_qp *qp = &transport->qps[i];

			if (event == NTB_LINK_UP) {
				/* Reset tx rings on link-up.  This gives the rx ring a little time to catch up on link down. */
				qp->tx_buf_head = qp->tx_buf_begin;
				qp->tx_buf_tail = qp->tx_buf_begin;
				qp->rx_mw_offset = qp->rx_mw_begin;

				qp->hw_link = NTB_LINK_UP;
			} else
				qp->hw_link = NTB_LINK_DOWN;

			if (transport->qps[i].event_handler) {
				qp->event_flags |= event; //FIXME - could be racy and only 1 bit
				schedule_delayed_work(&qp->event_work, 0);
			}
		}
}

//FIXME - currently we have to expose this to the clients, but we should really init it all on the first qp created and free when the alst one is removed
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
	transport->max_qps = (ntb_query_db_bits(transport->ndev) - 1) / DB_PER_QP;
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
		/* Alloc memory for receiving data.  Must be 4k aligned */
		transport->mw[i].size = ALIGN(ntb_get_mw_size(transport->ndev, i), 4096);

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
	ntb_unregister_event_callback(transport->ndev);

	for (i = 0; i < NTB_NUM_MW; i++)
		  dma_free_coherent(&transport->ndev->pdev->dev, transport->mw[i].size, transport->mw[i].virt_addr, transport->mw[i].dma_addr);
	kfree(transport->qps);
	ntb_unregister_transport(transport->ndev);
	kfree(transport);//FIXME - add refcount
}
EXPORT_SYMBOL(ntb_transport_free);

static int ntb_process_rxc(struct ntb_transport_qp *qp)
{
	struct ntb_payload_header *hdr;
	struct ntb_queue_entry *entry;
	void *offset;
	bool oflow;
	int rc;

	offset = qp->rx_mw_offset;
	hdr = offset;

	if (!hdr->tx_done || hdr->rx_done)
		return -EAGAIN;

	if (list_empty(&qp->rxq)) {
		pr_err("No RX buf\n");
		qp->rx_err_no_buf++;
		return -EAGAIN;
	}

	entry = list_first_entry(&qp->rxq, struct ntb_queue_entry, entry);
	list_del(&entry->entry);

	pr_debug("%d payload received, buf size %d\n", hdr->len, entry->len);
	if (hdr->len > entry->len) {
		pr_err("RX overflow! Wanted %d got %d\n", hdr->len, entry->len);
		list_add_tail(&entry->entry, &qp->rxq);
		qp->rx_err_oflow++;
		oflow = true;
	} else
		oflow = false;

	offset += sizeof(struct ntb_payload_header);

	if (offset + hdr->len > qp->rx_mw_end) {
		u32 delta = qp->rx_mw_end - offset;

		/* copy to end of buf */
		if (!oflow)
			memcpy(entry->buf, offset, delta);

		/* copy the rest of the payload at the beginning on the MW */
		offset = qp->rx_mw_begin;
		if (!oflow)
			memcpy(entry->buf + delta, offset, hdr->len - delta);

		/* update offset to point to the end of the buff */
		offset += hdr->len - delta;
	} else {
		if (!oflow)
			memcpy(entry->buf, offset, hdr->len);

		offset += hdr->len;

		if (offset + sizeof(struct ntb_payload_header) > qp->rx_mw_end)
			offset = qp->rx_mw_begin;
	}

	//print_hex_dump_bytes(" ", 0, entry->buf, entry->len);

	hdr->rx_done = 1;
	//FIXME - flush???

	qp->rx_mw_offset = offset;

	/* Ring doorbell notifying remote side to update TXC */
	rc = ntb_ring_sdb(transport->ndev, QP_TO_DB(qp->qp_num) + 1);
	if (rc)
		pr_err("%s: error ringing db %d\n", __func__, QP_TO_DB(qp->qp_num) + 1);

	if (oflow)
		return 0;

	qp->rx_bytes += hdr->len;
	qp->rx_pkts++;

	entry->len = hdr->len;

	list_add_tail(&entry->entry, &qp->rx_comp_q);

	//FIXME - this should kick off wq thread or we should document that it runs in irq context
	if (qp->rx_handler && qp->sw_link)
		qp->rx_handler(qp);

	return 0;
}

static int ntb_transport_rxc(void *data)
{
	struct ntb_transport_qp *qp = data;

	while (!kthread_should_stop()) {
		while (ntb_process_rxc(qp) == 0);

		/* Sleep if no rx work */
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}

	return 0;
}

static void ntb_transport_rxc_db(int db_num)
{
	struct ntb_transport_qp *qp = &transport->qps[DB_TO_QP(db_num)];

	pr_debug("%s: doorbell %d received\n", __func__, db_num);
#if 0
	wake_up_process(qp->rx_work);
#else
	while (ntb_process_rxc(qp) == 0);
#endif
}

static void ntb_transport_txc_db(int db_num)
{
	struct ntb_transport_qp *qp = &transport->qps[DB_TO_QP(db_num)];

	pr_debug("%s: doorbell %d received\n", __func__, db_num);
	wake_up_process(qp->txc_work);
}

static int ntb_transport_txc(void *data)
{
	struct ntb_transport_qp *qp = data;
	struct ntb_payload_header *hdr;
	void *offset;

	/* Update tail pointer */

	while (!kthread_should_stop()) {
		do {
			hdr = qp->tx_buf_tail;

			if (!(hdr->tx_done && hdr->rx_done))
				break;

			offset = qp->tx_buf_tail;
			offset += sizeof(struct ntb_payload_header);

			if (offset + hdr->len > qp->tx_buf_end)
				offset = qp->tx_buf_begin + (hdr->len - (qp->tx_buf_end - offset));
			else {
				offset += hdr->len;

				if (offset + sizeof(struct ntb_payload_header) > qp->tx_buf_end)
					offset = qp->tx_buf_begin;
			}

			qp->tx_buf_tail = offset;
		} while (qp->tx_buf_tail != qp->tx_buf_head);

		/* Sleep if no txc work */
		//FIXME - this might be the passe
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}

	return 0;
}

static int ntb_process_tx(struct ntb_transport_qp *qp, struct ntb_queue_entry *entry)
{
	struct ntb_payload_header *hdr;
	void *offset;
	int rc;

	pr_debug("%lld - tx head %p, tail %p, entry len %d, free size %ld\n", qp->tx_pkts, qp->tx_buf_head, qp->tx_buf_tail, entry->len, free_len(qp));

	/* check to see if there is enough room on the local buf */
	if (entry->len > free_len(qp)) {
		qp->tx_ring_full++;
		list_add(&entry->entry, &qp->txq);
		return -EAGAIN;
	}

	offset = qp->tx_buf_head;

	hdr = offset;
	offset += sizeof(struct ntb_payload_header);

	/* If it can't fit into whats left of the ring, copy to the end and put the rest at the beginning */
	if (offset + entry->len > qp->tx_buf_end) {
		u32 delta = qp->tx_buf_end - offset;

		/* copy to end of buf */
		memcpy(offset, entry->buf, delta);

		/* copy the rest of the payload at the beginning on the MW */
		memcpy(qp->tx_buf_begin, entry->buf + delta, entry->len - delta);

		/* update offset to point to the end of the buff */
		offset = qp->tx_buf_begin + (entry->len - delta);
	} else {
		memcpy(offset, entry->buf, entry->len);
		offset += entry->len;

		if (offset + sizeof(struct ntb_payload_header) > qp->tx_buf_end)
			offset = qp->tx_buf_begin;
	}

	hdr->tx_done = 1;
	hdr->rx_done = 0;
	hdr->len = entry->len;
	hdr->ver = qp->tx_pkts;

	qp->tx_buf_head = offset;

	/* set next hdr to not done for remote rxq */
	hdr = offset;
	hdr->tx_done = 0;
	hdr->rx_done = 0;

	//print_hex_dump_bytes(" ", 0, entry->buf, entry->len);

	/* Ring doorbell notifying remote side of new packet */
	rc = ntb_ring_sdb(transport->ndev, QP_TO_DB(qp->qp_num));
	if (rc)
		pr_err("%s: error ringing db %d\n", __func__, 1 << qp->qp_num);

	qp->tx_pkts++;
	qp->tx_bytes += entry->len;

	/* Add fully transmitted data to completion queue */
	list_add_tail(&entry->entry, &qp->tx_comp_q);

	//FIXME - might want to kick off a thread/wq to handle the tx completion
	if (qp->tx_handler)
		qp->tx_handler(qp);

	return 0;
}

static int ntb_transport_tx(void *data)
{
	struct ntb_transport_qp *qp = data;
	struct ntb_queue_entry *entry;
	int rc;

	while (!kthread_should_stop()) {
		while (!list_empty(&qp->txq)) {
			entry = list_first_entry(&qp->txq, struct ntb_queue_entry, entry);
			list_del(&entry->entry);

			rc = ntb_process_tx(qp, entry);
			if (rc)
				break;
		}

		/* Sleep if no tx work */
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}

	return 0;
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
	int rc, irq;

	//FIXME - need to handhake with remote side to determine matching number or some mapping between the 2
	free_queue = ffs(transport->qp_bitmap);
	if (!free_queue)
		goto err;

	/* decrement free_queue to make it zero based */
	free_queue--;

	clear_bit(free_queue, &transport->qp_bitmap);//FIXME - this might be racy, either add lock or make atomic

	qp = &transport->qps[free_queue];
	qp->qp_num = free_queue;

	qp->rx_handler = rx_handler;
	qp->tx_handler = tx_handler;
	qp->event_handler = NULL;

#if 0
	qp->rx_work = kthread_create(ntb_transport_rxc, qp, "ntb_rx");
	if (IS_ERR(qp->rx_work)) {
		rc = PTR_ERR(qp->rx_work);
		pr_err("Error allocing rxc kthread\n");
		goto err1;
	}
#endif
	qp->txc_work = kthread_create(ntb_transport_txc, qp, "ntb_txc");
	if (IS_ERR(qp->txc_work)) {
		rc = PTR_ERR(qp->txc_work);
		pr_err("Error allocing txc kthread\n");
		goto err2;
	}

	qp->tx_work = kthread_create(ntb_transport_tx, qp, "ntb_tx");
	if (IS_ERR(qp->tx_work)) {
		rc = PTR_ERR(qp->tx_work);
		pr_err("Error allocing tx kthread\n");
		goto err3;
	}

	INIT_LIST_HEAD(&qp->rxq);
	INIT_LIST_HEAD(&qp->rx_comp_q);
	INIT_LIST_HEAD(&qp->txq);
	INIT_LIST_HEAD(&qp->tx_comp_q);

	qp->rx_mw_begin = ntb_get_mw_vbase(transport->ndev, QP_TO_MW(free_queue));
	qp->rx_mw_end = qp->rx_mw_begin + transport->mw[QP_TO_MW(free_queue)].size;
	qp->rx_mw_offset = qp->rx_mw_begin;
	pr_debug("RX MW start %p end %p\n", qp->rx_mw_begin, qp->rx_mw_end);

	qp->tx_buf_begin = transport->mw[QP_TO_MW(free_queue)].virt_addr;
	qp->tx_buf_end = qp->tx_buf_begin + transport->mw[QP_TO_MW(free_queue)].size;
	qp->tx_buf_head = qp->tx_buf_begin;
	qp->tx_buf_tail = qp->tx_buf_begin;
	pr_debug("TX buf start %p end %p\n", qp->tx_buf_begin, qp->tx_buf_end);

	//FIXME - transport->qps[free_queue].queue.hw_caps; is it even necessary for transport client to know?

	irq = free_queue * DB_PER_QP;

	rc = ntb_register_db_callback(transport->ndev, irq, ntb_transport_rxc_db);
	if (rc)
		goto err4;

	rc = ntb_register_db_callback(transport->ndev, irq + 1, ntb_transport_txc_db);
	if (rc)
		goto err5;

	//wake_up_process(qp->rx_work);
	wake_up_process(qp->tx_work);
	wake_up_process(qp->txc_work);

	if (transport->ndev->link_status)
		qp->hw_link = NTB_LINK_UP;
	else
		qp->hw_link = NTB_LINK_DOWN;

	return qp;

err5:
	ntb_unregister_db_callback(transport->ndev, irq);
err4:
	kthread_stop(qp->tx_work);
err3:
	kthread_stop(qp->txc_work);
err2:
	//kthread_stop(qp->rx_work);
err1:
	set_bit(free_queue, &transport->qp_bitmap);
err:
	return NULL;
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

	qp->sw_link = NTB_LINK_DOWN;
	qp->hw_link = NTB_LINK_DOWN;
	qp->event_handler = NULL;
	ntb_unregister_db_callback(transport->ndev, QP_TO_DB(qp->qp_num));
	ntb_unregister_db_callback(transport->ndev, QP_TO_DB(qp->qp_num) + 1);
	//FIXME - wait for qps to quience or notify the transport to stop?

	kthread_stop(qp->txc_work);
	kthread_stop(qp->tx_work);
	//kthread_stop(qp->rx_work);

	ntb_purge_list(&qp->rxq);
	ntb_purge_list(&qp->rx_comp_q);
	ntb_purge_list(&qp->txq);
	ntb_purge_list(&qp->tx_comp_q);

	set_bit(qp->qp_num, &transport->qp_bitmap);

	if (transport->qp_bitmap == ~(transport->max_qps - 1))
		ntb_transport_free();
}
EXPORT_SYMBOL(ntb_transport_free_queue);

struct ntb_queue_entry *ntb_transport_rx_remove(struct ntb_transport_qp *qp)
{
	struct ntb_queue_entry *entry;

	if (!qp || qp->sw_link || list_empty(&qp->rxq))
		return NULL;

	entry = list_first_entry(&qp->rxq, struct ntb_queue_entry, entry);
	list_del(&entry->entry);

	return entry;

}
EXPORT_SYMBOL(ntb_transport_rx_remove);

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
	if (!qp || qp->hw_link != NTB_LINK_UP)
		return -EINVAL;
#if 0
	list_add_tail(&entry->entry, &qp->txq);

	wake_up_process(qp->tx_work);
#else
	ntb_process_tx(qp, entry);
#endif
	return 0;
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

	qp->sw_link = 1;
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

	qp->sw_link = 0;
}
EXPORT_SYMBOL(ntb_transport_link_down);


static void ntb_transport_event_work(struct work_struct *work)
{
	struct ntb_transport_qp *qp = container_of(work, struct ntb_transport_qp, event_work.work);

	if (qp->event_handler)
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

size_t ntb_transport_max_size(struct ntb_transport_qp *qp)
{
	return transport->mw[QP_TO_MW(qp->qp_num)].size - sizeof(struct ntb_payload_header);
}
EXPORT_SYMBOL(ntb_transport_max_size);
