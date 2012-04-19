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

struct ntb_transport_qp {
	struct ntb_device *ndev;
	struct workqueue_struct *qp_wq;

	struct work_struct tx_work;
	struct list_head txq;
	struct list_head txc;
	spinlock_t txq_lock;
	spinlock_t txc_lock;
	void *tx_buf_begin;
	void *tx_buf_end;
	void *tx_offset;

	struct work_struct rx_work;
	struct list_head rxq;
	struct list_head rxc;
	spinlock_t rxq_lock;
	spinlock_t rxc_lock;
	void *rx_mw_begin;
	void *rx_mw_end;
	void *rx_offset;

	void (*rx_handler)(struct ntb_transport_qp *qp);
	void (*tx_handler)(struct ntb_transport_qp *qp);
	void (*event_handler)(int status);
	struct delayed_work event_work;
	struct delayed_work link_work;
	//FIXME - unless this is getting larger than a cacheline, bit fields might not be worth it
	unsigned int event_flags:1;
	unsigned int client_ready:1;
	unsigned int qp_link:1;
	unsigned int qp_num:6;/* Only 64 QP's are allowed.  0-63 */

	/* Stats */
	u64 rx_bytes;
	u64 rx_pkts;
	u64 rx_ring_empty;
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
	u64 ver;
	u8 link:1;
};

enum {
	TX_OFFSET = 0,
	RX_OFFSET,
	SPADS_PER_QP,
};

#define QP_TO_MW(qp)		((qp) % NTB_NUM_MW)

#define	free_len(qp, tx, rx)	(tx < rx ? rx - tx : (qp->tx_buf_end - qp->tx_buf_begin) - (tx - rx))

struct ntb_transport *transport = NULL;

static void ntb_list_add_head(spinlock_t *lock, struct list_head *entry, struct list_head *list)
{
	unsigned long flags;

	spin_lock_irqsave(lock, flags);
	list_add(entry, list);
	spin_unlock_irqrestore(lock, flags);
}

static void ntb_list_add_tail(spinlock_t *lock, struct list_head *entry, struct list_head *list)
{
	unsigned long flags;

	spin_lock_irqsave(lock, flags);
	list_add_tail(entry, list);
	spin_unlock_irqrestore(lock, flags);
}

static struct ntb_queue_entry *ntb_list_rm_head(spinlock_t *lock, struct list_head *list)
{
	struct ntb_queue_entry *entry;
	unsigned long flags;

	spin_lock_irqsave(lock, flags);
	if (list_empty(list)) {
		entry = NULL;
		goto out;
	}
	entry = list_first_entry(list, struct ntb_queue_entry, entry);
	list_del(&entry->entry);
out:
	spin_unlock_irqrestore(lock, flags);

	return entry;
}

static int ntb_transport_get_local_offset(struct ntb_transport_qp *qp, u8 offset, u32 *val)
{
	int idx;

	idx = (qp->qp_num * SPADS_PER_QP) + offset;

	return ntb_read_local_spad(qp->ndev, idx, val);
}

static int ntb_transport_get_remote_offset(struct ntb_transport_qp *qp, u8 offset, u32 *val)
{
	int idx;

	idx = (qp->qp_num * SPADS_PER_QP) + offset;

	return ntb_read_remote_spad(qp->ndev, idx, val);
}

static int ntb_transport_put_remote_offset(struct ntb_transport_qp *qp, u8 offset, u32 val)
{
	int idx;

	idx = (qp->qp_num * SPADS_PER_QP) + offset;

	return ntb_write_remote_spad(qp->ndev, idx, val);
}

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
	u32 tx_offset, rx_offset;
	int rc;

	pr_info("NTB Transport stats\n");
	pr_info("rx_bytes - %Ld\n", qp->rx_bytes);
	pr_info("rx_pkts - %Ld\n", qp->rx_pkts);
	pr_info("rx_ring_empty - %Ld\n", qp->rx_ring_empty);
	pr_info("rx_err_no_buf - %Ld\n", qp->rx_err_no_buf);
	pr_info("rx_err_oflow - %Ld\n", qp->rx_err_oflow);
	pr_info("tx_bytes - %Ld\n", qp->tx_bytes);
	pr_info("tx_pkts - %Ld\n", qp->tx_pkts);
	pr_info("tx_ring_full - %Ld\n", qp->tx_ring_full);
	pr_info("tx begin %p, end %p, ring size %ld\n", qp->tx_buf_begin, qp->tx_buf_end, qp->tx_buf_end - qp->tx_buf_begin);

	rc = ntb_transport_get_local_offset(qp, TX_OFFSET, &tx_offset);
	if (rc) {
		pr_err("%s: error reading from offset %d\n", __func__, TX_OFFSET);
		return;
	}

	rc = ntb_transport_get_remote_offset(qp, RX_OFFSET, &rx_offset);
	if (rc) {
		pr_err("%s: error reading from offset %d\n", __func__, RX_OFFSET);
		return;
	}

	pr_info("tx %p, rx %p, free size %ld\n", qp->tx_buf_begin + tx_offset, qp->tx_buf_begin + rx_offset, free_len(qp, tx_offset, rx_offset));
	pr_info("txq len %d, txc len %d\n", list_len(&qp->txq), list_len(&qp->txc));

	rc = ntb_transport_get_local_offset(qp, RX_OFFSET, &rx_offset);
	if (rc) {
		pr_err("%s: error reading from offset %d\n", __func__, RX_OFFSET);
		return;
	}

	pr_info("rx begin %p, end %p, offset %p\n", qp->rx_mw_begin, qp->rx_mw_end, qp->rx_mw_begin + rx_offset);
	pr_info("rxq len %d, rxc len %d\n", list_len(&qp->rxq), list_len(&qp->rxc));
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

			pr_debug("%s: qp %d gets events %x\n", __func__, i, event);

			if (event == NTB_EVENT_HW_ERROR)
				BUG();

			if (event == NTB_EVENT_HW_LINK_UP) {
				/* If the HW link comes up, notify the remote side we're up */
				schedule_delayed_work(&qp->link_work, 0);
				continue;
			}

			if (event == NTB_EVENT_HW_LINK_DOWN) {
				qp->qp_link = NTB_LINK_DOWN;
				cancel_delayed_work_sync(&qp->link_work);
			}

			//FIXME - determine what to send to the clients
			qp->event_flags = qp->qp_link; //FIXME - handle more things
			schedule_delayed_work(&qp->event_work, 0);
		}
}

static int ntb_transport_init(void)
{
	int rc, i;

	if (transport)
		return -EINVAL;

	transport = kmalloc(sizeof(struct ntb_transport), GFP_ATOMIC);
	if (!transport)
		return -ENOMEM;

	transport->ndev = ntb_register_transport(transport);
	if (!transport->ndev) {
		rc = -EIO;
		goto err;
	}

	/* 1 Doorbell bit is used by Link/HB, but the rest can be used for the transport qp's */
	transport->max_qps = ntb_query_db_bits(transport->ndev);
	if (!transport->max_qps) {
		rc = -EIO;
		goto err1;
	}

	transport->qps = kcalloc(transport->max_qps, sizeof(struct ntb_transport_qp), GFP_ATOMIC);
	if (!transport->qps) {
		rc = -ENOMEM;
		goto err1;
	}

	transport->qp_bitmap = ((u64) 1 << transport->max_qps) - 1;

	for (i = 0; i < NTB_NUM_MW; i++) {
		/* Alloc memory for receiving data.  Must be 4k aligned */
		transport->mw[i].size = ALIGN(ntb_get_mw_size(transport->ndev, i), 4096);

		transport->mw[i].virt_addr = dma_alloc_coherent(&transport->ndev->pdev->dev, transport->mw[i].size, &transport->mw[i].dma_addr, GFP_ATOMIC);
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

static void ntb_transport_free(void)
{
	int i;

	if (!transport)
		return;

	/* To be here, all of the queues were already free'd.  No need to try and clean them up */

	ntb_unregister_event_callback(transport->ndev);

	for (i = 0; i < NTB_NUM_MW; i++)
		  dma_free_coherent(&transport->ndev->pdev->dev, transport->mw[i].size, transport->mw[i].virt_addr, transport->mw[i].dma_addr);

	kfree(transport->qps);
	ntb_unregister_transport(transport->ndev);
	kfree(transport);
	transport = NULL;
}

static bool pass_check(u32 tx_orig, u32 tx_curr, u32 rx)
{
	if (tx_orig < rx)
		return (tx_curr >= rx);
	else //tx_orig >= rx
		if (tx_curr < tx_orig) //wrap
			return (tx_curr >= rx);
		else
			return (tx_curr < rx);
}

static bool pass_check_rx(u32 rx_orig, u32 rx_curr, u32 tx)
{
	if (rx_orig < tx)
		return (rx_curr > tx);
	else//rx_orig >= tx
		if (rx_curr < rx_orig) //wrap
			return (rx_curr > tx);
		else
			return (rx_curr < tx);
}

static int ntb_process_rxc(struct ntb_transport_qp *qp)
{
	struct ntb_payload_header *hdr;
	struct ntb_queue_entry *entry;
	void *offset;
	bool oflow;
	int rc;
	u32 tx_offset;
	static u32 last = -1;

	if (qp->qp_link == NTB_LINK_DOWN) {
		pr_debug("QP %d Link Up\n", qp->qp_num);
		qp->event_flags = NTB_LINK_UP; 

		schedule_delayed_work(&qp->event_work, msecs_to_jiffies(1000));
		return -EAGAIN;
	}

	rc = ntb_transport_get_remote_offset(qp, TX_OFFSET, &tx_offset);
	if (rc)
		pr_err("%s: error reading from offset %d\n", __func__, TX_OFFSET);

	if (qp->rx_offset == (qp->rx_mw_begin + tx_offset)) {
		qp->rx_ring_empty++;
		return -EAGAIN;
	}

	entry = ntb_list_rm_head(&qp->rxq_lock, &qp->rxq);
	if (!entry) {
		qp->rx_err_no_buf++;
		return -EAGAIN;
	}

	offset = qp->rx_offset;
	if (offset + sizeof(struct ntb_payload_header) >= qp->rx_mw_end)
		offset = qp->rx_mw_begin;

	hdr = offset;

	if (hdr->ver != qp->rx_pkts) {
		pr_err("Version mismatch, expected %Ld - got %Ld\n", qp->rx_pkts, hdr->ver);
		pr_err("rx offset %lx last %x, last to end %ld, curr to end %ld, tx offset %x, ver %Ld\n",
			 offset - qp->rx_mw_begin, last, qp->rx_mw_end - (qp->rx_mw_begin + last), qp->rx_mw_end - qp->rx_offset, tx_offset, hdr->ver);
		ntb_list_add_tail(&qp->rxq_lock, &entry->entry, &qp->rxq);
		return -EIO;
	}

	pr_debug("rx offset %p, tx offset %x, ver %Ld - %d payload received, buf size %d\n", qp->rx_offset, tx_offset, hdr->ver, hdr->len, entry->len);
	if (hdr->len > entry->len) {
		pr_err("RX overflow! Wanted %d got %d\n", hdr->len, entry->len);
		ntb_transport_dump_qp_stats(qp);
		ntb_list_add_tail(&qp->rxq_lock, &entry->entry, &qp->rxq);
		qp->rx_err_oflow++;
		oflow = true;
	} else {
		if (!hdr->link) {
			pr_debug("QP %d Link Down\n", qp->qp_num);
			qp->event_flags = NTB_LINK_DOWN; 

			schedule_delayed_work(&qp->event_work, msecs_to_jiffies(1000));
			oflow = true;
		} else {
			entry->len = hdr->len;
			oflow = false;
		}
	}

	offset += sizeof(struct ntb_payload_header);

	if (offset + hdr->len >= qp->rx_mw_end)
		offset = qp->rx_mw_begin;

	if (!oflow)
		memcpy(entry->buf, offset, entry->len);
		//print_hex_dump_bytes(" ", 0, entry->buf, entry->len);

	offset += hdr->len;

	WARN_ON(pass_check_rx(qp->rx_offset - qp->rx_mw_begin, offset - qp->rx_mw_begin, tx_offset));

	qp->rx_offset = offset;
	rc = ntb_transport_put_remote_offset(qp, RX_OFFSET, offset - qp->rx_mw_begin);
	if (rc)
		pr_err("%s: error writing to offset %d\n", __func__, RX_OFFSET);

	if (oflow)
		return 0;

	qp->rx_bytes += entry->len;
	qp->rx_pkts++;

	ntb_list_add_tail(&qp->rxc_lock, &entry->entry, &qp->rxc);

	//FIXME - this should kick off wq thread or we should document that it runs in irq context
	if (qp->rx_handler && qp->client_ready)
		qp->rx_handler(qp);

	return 0;
}

static void ntb_transport_rxc(struct work_struct *work)
{
	struct ntb_transport_qp *qp = container_of(work, struct ntb_transport_qp, rx_work);

	while (ntb_process_rxc(qp) == 0);
}

static void ntb_transport_rxc_db(int db_num)
{
	struct ntb_transport_qp *qp = &transport->qps[db_num];

	pr_debug("%s: doorbell %d received\n", __func__, db_num);

	//if (!work_busy(&qp->rx_work))
		queue_work(qp->qp_wq, &qp->rx_work);
}

static int ntb_process_tx(struct ntb_transport_qp *qp, struct ntb_queue_entry *entry, int link)
{
	struct ntb_payload_header *hdr;
	void *offset;
	int rc;
	u32 rx_offset;

	rc = ntb_transport_get_remote_offset(qp, RX_OFFSET, &rx_offset);
	if (rc) {
		pr_err("%s: error reading from offset %d\n", __func__, RX_OFFSET);
//		ntb_list_add_head(&qp->txq_lock, &entry->entry, &qp->txq);
		return -EIO;
	}

	offset = qp->tx_offset;
	if (offset + sizeof(struct ntb_payload_header) >= qp->tx_buf_end)
		offset = qp->tx_buf_begin;

	//did this hit or pass the rx offset
	if (pass_check(qp->tx_offset - qp->tx_buf_begin, offset - qp->tx_buf_begin, rx_offset)) {
//		ntb_list_add_head(&qp->txq_lock, &entry->entry, &qp->txq);
		qp->tx_ring_full++;
		return -EAGAIN;
	}

	hdr = offset;
	offset += sizeof(struct ntb_payload_header);

	/* If it can't fit into whats left of the ring, move to the beginning */
	if (offset + entry->len >= qp->tx_buf_end)
		offset = qp->tx_buf_begin;

	if (pass_check(qp->tx_offset - qp->tx_buf_begin, (offset - qp->tx_buf_begin) + entry->len, rx_offset)) {
//		ntb_list_add_head(&qp->txq_lock, &entry->entry, &qp->txq);
		qp->tx_ring_full++;
		return -EAGAIN;
	}

	pr_debug("%lld - tx %x, rx %x, entry len %d, free size %ld\n", qp->tx_pkts, (u32) (qp->tx_offset - qp->tx_buf_begin), rx_offset, entry->len, free_len(qp, (u32) (qp->tx_offset - qp->tx_buf_begin), rx_offset));
	//print_hex_dump_bytes(" ", 0, entry->buf, entry->len);

	hdr->len = entry->len;
	hdr->ver = qp->tx_pkts;
	hdr->link = (link == NTB_LINK_UP);

	memcpy(offset, entry->buf, entry->len);
	offset += entry->len;

	qp->tx_offset = offset;
	rc = ntb_transport_put_remote_offset(qp, TX_OFFSET, offset - qp->tx_buf_begin);
	if (rc)
		pr_err("%s: error writing to offset %d\n", __func__, TX_OFFSET);

	/* Ring doorbell notifying remote side of new packet */
	rc = ntb_ring_sdb(qp->ndev, qp->qp_num);
	if (rc)
		pr_err("%s: error ringing db %d\n", __func__, qp->qp_num);

	if (entry->len == 0)
		return 0;

	qp->tx_pkts++;
	qp->tx_bytes += entry->len;

	/* Add fully transmitted data to completion queue */
	ntb_list_add_tail(&qp->txc_lock, &entry->entry, &qp->txc);

	//FIXME - might want to kick off a thread/wq to handle the tx completion
	if (qp->tx_handler)
		qp->tx_handler(qp);

	return 0;
}

static void ntb_transport_tx(struct work_struct *work)
{
	struct ntb_transport_qp *qp = container_of(work, struct ntb_transport_qp, tx_work);
	struct ntb_queue_entry *entry;
	int rc;

	while ((entry = ntb_list_rm_head(&qp->txq_lock, &qp->txq))) {
		rc = ntb_process_tx(qp, entry, NTB_LINK_UP);
		if (rc)
			break;
	}
}

static void ntb_transport_event_work(struct work_struct *work)
{
	struct ntb_transport_qp *qp = container_of(work, struct ntb_transport_qp, event_work.work);

	if (qp->qp_link == NTB_LINK_DOWN) {
		cancel_delayed_work_sync(&qp->link_work);
		qp->qp_link = NTB_LINK_UP;
	} else {
		qp->qp_link = NTB_LINK_DOWN;
		qp->rx_pkts = 0;
		qp->tx_pkts = 0;
		qp->tx_offset = qp->tx_buf_begin;
		qp->rx_offset = qp->rx_mw_begin;
		ntb_transport_put_remote_offset(qp, RX_OFFSET, 0); //FIXME - handle rc
		ntb_transport_put_remote_offset(qp, TX_OFFSET, 0); //FIXME - handle rc

		if (qp->ndev->link_status)
			schedule_delayed_work(&qp->link_work, 0);
	}

	if (qp->event_handler)
		qp->event_handler(qp->event_flags);
}

static void ntb_transport_link_work(struct work_struct *work)
{
	struct ntb_transport_qp *qp = container_of(work, struct ntb_transport_qp, link_work.work);
	int rc;

	/* Ring doorbell notifying remote side of new packet */
	rc = ntb_ring_sdb(qp->ndev, qp->qp_num);
	if (rc)
		pr_err("%s: error ringing db %d\n", __func__, qp->qp_num);

	schedule_delayed_work(&qp->link_work, msecs_to_jiffies(1000));
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
ntb_transport_create_queue(handler rx_handler, handler tx_handler, ehandler event_handler)
{
	struct ntb_transport_qp *qp;
	unsigned int free_queue;
	int rc;

	if (!transport) {
		rc = ntb_transport_init();
		if (rc)
			return NULL;
	}

	//FIXME - need to handhake with remote side to determine matching number or some mapping between the 2
	free_queue = ffs(transport->qp_bitmap);
	if (!free_queue)
		goto err;

	/* decrement free_queue to make it zero based */
	free_queue--;

	clear_bit(free_queue, &transport->qp_bitmap);//FIXME - this might be racy, either add lock or make atomic

	qp = &transport->qps[free_queue];
	qp->qp_link = NTB_LINK_DOWN;
	qp->qp_num = free_queue;
	qp->ndev = transport->ndev;

	qp->rx_handler = rx_handler;
	qp->tx_handler = tx_handler;
	qp->event_handler = event_handler;

	//FIXME - workqueue might be overkill, and might not need to be delayed...
	INIT_DELAYED_WORK(&qp->event_work, ntb_transport_event_work);
	INIT_DELAYED_WORK(&qp->link_work, ntb_transport_link_work);

	spin_lock_init(&qp->rxc_lock);
	spin_lock_init(&qp->rxq_lock);
	spin_lock_init(&qp->txc_lock);
	spin_lock_init(&qp->txq_lock);

	INIT_LIST_HEAD(&qp->rxq);
	INIT_LIST_HEAD(&qp->rxc);
	INIT_LIST_HEAD(&qp->txq);
	INIT_LIST_HEAD(&qp->txc);

	INIT_WORK(&qp->rx_work, ntb_transport_rxc);
	INIT_WORK(&qp->tx_work, ntb_transport_tx);

	qp->qp_wq = alloc_workqueue("ntb-wq", WQ_NON_REENTRANT | WQ_UNBOUND | WQ_HIGHPRI | WQ_CPU_INTENSIVE, 2);
	if (!qp->qp_wq)
		goto err1;

	qp->rx_pkts = 0;
	qp->tx_pkts = 0;
	ntb_transport_put_remote_offset(qp, RX_OFFSET, 0); //FIXME - handle rc
	ntb_transport_put_remote_offset(qp, TX_OFFSET, 0); //FIXME - handle rc

	qp->rx_mw_begin = transport->mw[QP_TO_MW(free_queue)].virt_addr;
	//qp->rx_mw_begin = ntb_get_mw_vbase(qp->ndev, QP_TO_MW(free_queue));
	qp->rx_mw_end = qp->rx_mw_begin + transport->mw[QP_TO_MW(free_queue)].size;
	pr_debug("RX MW start %p end %p\n", qp->rx_mw_begin, qp->rx_mw_end);
	qp->rx_offset = qp->rx_mw_begin;

	qp->tx_buf_begin = ntb_get_mw_vbase(qp->ndev, QP_TO_MW(free_queue));
	//qp->tx_buf_begin = transport->mw[QP_TO_MW(free_queue)].virt_addr;
	qp->tx_buf_end = qp->tx_buf_begin + transport->mw[QP_TO_MW(free_queue)].size;
	pr_debug("TX buf start %p end %p\n", qp->tx_buf_begin, qp->tx_buf_end);
	qp->tx_offset = qp->tx_buf_begin;

	//FIXME - transport->qps[free_queue].queue.hw_caps; is it even necessary for transport client to know?

	rc = ntb_register_db_callback(qp->ndev, free_queue, ntb_transport_rxc_db);
	if (rc)
		goto err2;

	if (qp->ndev->link_status)
		schedule_delayed_work(&qp->link_work, 0);

	return qp;

err2:
	destroy_workqueue(qp->qp_wq);
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
	struct ntb_queue_entry entry;

	if (!qp)
		return;

	qp->qp_link = NTB_LINK_DOWN;

	entry.len = 0;
	entry.buf = NULL;
	ntb_process_tx(qp, &entry, NTB_LINK_DOWN);//FIXME - check rc

	cancel_delayed_work_sync(&qp->event_work);
	cancel_delayed_work_sync(&qp->link_work);

	ntb_unregister_db_callback(qp->ndev, qp->qp_num);
	//FIXME - wait for qps to quience or notify the transport to stop?

	cancel_work_sync(&qp->tx_work);
	cancel_work_sync(&qp->rx_work);
	destroy_workqueue(qp->qp_wq);

	ntb_transport_put_remote_offset(qp, RX_OFFSET, 0); //FIXME - handle rc
	ntb_transport_put_remote_offset(qp, TX_OFFSET, 0); //FIXME - handle rc

	ntb_purge_list(&qp->rxq);
	ntb_purge_list(&qp->rxc);
	ntb_purge_list(&qp->txq);
	ntb_purge_list(&qp->txc);

	set_bit(qp->qp_num, &transport->qp_bitmap);

	if (transport->qp_bitmap == ((u64) 1 << transport->max_qps) - 1)
		ntb_transport_free();
}
EXPORT_SYMBOL(ntb_transport_free_queue);

struct ntb_queue_entry *ntb_transport_rx_remove(struct ntb_transport_qp *qp)
{
	if (!qp || qp->client_ready == NTB_LINK_UP)
		return NULL;

	return ntb_list_rm_head(&qp->rxq_lock, &qp->rxq);
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

	ntb_list_add_tail(&qp->rxq_lock, &entry->entry, &qp->rxq);

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
	if (!qp || qp->qp_link != NTB_LINK_UP)
		return -EINVAL;

#if 0 
	ntb_list_add_tail(&qp->txq_lock, &entry->entry, &qp->txq);

	//if (!work_busy(&qp->tx_work))
		queue_work(qp->qp_wq, &qp->tx_work);

	return 0;
#else
	return ntb_process_tx(qp, entry, NTB_LINK_UP);
#endif
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
	if (!qp)
		return NULL;
	
	return ntb_list_rm_head(&qp->txc_lock, &qp->txc);
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
	if (!qp)
		return NULL;

	return ntb_list_rm_head(&qp->rxc_lock, &qp->rxc);
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

	qp->client_ready = NTB_LINK_UP;
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

	qp->client_ready = NTB_LINK_DOWN;
}
EXPORT_SYMBOL(ntb_transport_link_down);

/**
 * ntb_transport_hw_link_query - Query hardware link state
 * @qp: NTB transport layer queue to be queried
 *
 * Query hardware link state of the NTB transport queue 
 *
 * RETURNS: true for link up or false for link down
 */
bool ntb_transport_hw_link_query(struct ntb_transport_qp *qp)//FIXME - rename
{
	return qp->qp_link == NTB_LINK_UP;
}
EXPORT_SYMBOL(ntb_transport_hw_link_query);

size_t ntb_transport_max_size(struct ntb_transport_qp *qp)
{
	return transport->mw[QP_TO_MW(qp->qp_num)].size - sizeof(struct ntb_payload_header);
}
EXPORT_SYMBOL(ntb_transport_max_size);
