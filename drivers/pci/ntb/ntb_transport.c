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
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/kthread.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/types.h>
#include "ntb_hw.h"
#include "ntb_transport.h"

struct ntb_queue_entry {
	/* ntb_queue list reference */
	struct list_head entry;
	/* pointers to data to be transfered */
	void *callback_data;
	void *buf;
	unsigned int len;
	unsigned int flags;
};

struct ntb_transport_qp {
	struct ntb_device *ndev;
	struct workqueue_struct *qp_wq;

	struct task_struct *tx_work;
	struct list_head txq;
	struct list_head txc;
	struct list_head txe;
	spinlock_t txq_lock;
	spinlock_t txc_lock;
	spinlock_t txe_lock;
	void *tx_mw_begin;
	void *tx_mw_end;
	void *tx_offset;

	struct task_struct *rx_work;
	struct list_head rxq;
	struct list_head rxc;
	struct list_head rxe;
	spinlock_t rxq_lock;
	spinlock_t rxc_lock;
	spinlock_t rxe_lock;
	void *rx_buff_begin;
	void *rx_buff_end;
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
	unsigned int qp_num:6;	/* Only 64 QP's are allowed.  0-63 */

	struct dentry *debugfs_dir;
	struct dentry *debugfs_stats;
	struct dentry *debugfs_rx_to;
	struct dentry *debugfs_tx_to;

	unsigned int rx_ring_timeo;
	unsigned int tx_ring_timeo;

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
	spinlock_t lock;
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
#define	NTB_QP_DEF_RING_TIMEOUT	100
#define NTB_QP_DEF_NUM_ENTRIES	1000

struct ntb_transport *transport = NULL;

static int debugfs_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	return 0;
}

static ssize_t debugfs_read(struct file *filp, char __user *ubuf,
	                    size_t count, loff_t *offp)
{
	struct ntb_transport_qp *qp;
	char *buf;
	ssize_t ret, out_offset, out_count;

	out_count = 1024; //FIXME - magic...
	buf = kmalloc(out_count, GFP_KERNEL);
	if (!buf)
	        return -ENOMEM;

	qp = filp->private_data;
	out_offset = 0;
	out_offset += snprintf(buf + out_offset, out_count - out_offset,
			      "NTB Transport stats\n");
	out_offset += snprintf(buf + out_offset, out_count - out_offset,
				"rx_bytes - %Ld\n", qp->rx_bytes);
	out_offset += snprintf(buf + out_offset, out_count - out_offset,
				"rx_pkts - %Ld\n", qp->rx_pkts);
	out_offset += snprintf(buf + out_offset, out_count - out_offset,
				"rx_ring_empty - %Ld\n", qp->rx_ring_empty);
	out_offset += snprintf(buf + out_offset, out_count - out_offset,
				"rx_err_no_buf - %Ld\n", qp->rx_err_no_buf);
	out_offset += snprintf(buf + out_offset, out_count - out_offset,
				"rx_err_oflow - %Ld\n", qp->rx_err_oflow);
	out_offset += snprintf(buf + out_offset, out_count - out_offset,
				"tx_bytes - %Ld\n", qp->tx_bytes);
	out_offset += snprintf(buf + out_offset, out_count - out_offset,
				"tx_pkts - %Ld\n", qp->tx_pkts);
	out_offset += snprintf(buf + out_offset, out_count - out_offset,
				"tx_ring_full - %Ld\n", qp->tx_ring_full);

	ret = simple_read_from_buffer(ubuf, count, offp, buf, out_offset);
	kfree(buf);
	return ret;
}

static const struct file_operations ntb_qp_debugfs_stats = {
	.owner = THIS_MODULE,
	.open  = debugfs_open,
	.read  = debugfs_read,
};

static void ntb_list_add_head(spinlock_t *lock, struct list_head *entry,
			      struct list_head *list)
{
	unsigned long flags;

	spin_lock_irqsave(lock, flags);
	list_add(entry, list);
	spin_unlock_irqrestore(lock, flags);
}

static void ntb_list_add_tail(spinlock_t *lock, struct list_head *entry,
			      struct list_head *list)
{
	unsigned long flags;

	spin_lock_irqsave(lock, flags);
	list_add_tail(entry, list);
	spin_unlock_irqrestore(lock, flags);
}

static struct ntb_queue_entry *ntb_list_rm_head(spinlock_t *lock,
						struct list_head *list)
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

static int ntb_transport_get_remote_offset(struct ntb_transport_qp *qp,
					   u8 offset, u32 *val)
{
	int idx;

	idx = (qp->qp_num * SPADS_PER_QP) + offset;

	return ntb_read_remote_spad(qp->ndev, idx, val);
}

static void ntb_transport_put_remote_offset(struct ntb_transport_qp *qp,
					    u8 offset, u32 val)
{
	int idx, rc;

	idx = (qp->qp_num * SPADS_PER_QP) + offset;

	rc = ntb_write_remote_spad(qp->ndev, idx, val);
	if (rc)
		pr_err("Error writing %x to remote spad %d\n", val, idx);
}

static void ntb_transport_event_callback(void *data, unsigned int event)
{
	struct ntb_transport *transport = data;
	int i;

	/* Pass along the info to any clients */
	for (i = 0; i < transport->max_qps; i++)
		if (!test_bit(i, &transport->qp_bitmap)) {
			struct ntb_transport_qp *qp = &transport->qps[i];

			pr_debug("%s: qp %d gets events %x\n", __func__, i,
				 event);

			if (event == NTB_EVENT_HW_ERROR)
				BUG();

			if (event == NTB_EVENT_HW_LINK_UP) {
				/* If the HW link comes up, notify the remote side we're up */
				schedule_delayed_work(&qp->link_work, 0);
				continue;
			}

			if (event == NTB_EVENT_HW_LINK_DOWN)
				qp->event_flags = NTB_LINK_DOWN;

			schedule_delayed_work(&qp->event_work, 0);
		}
}

static int ntb_transport_init(void)
{
	int rc, i;

	if (transport)
		return -EINVAL;

	transport = kmalloc(sizeof(struct ntb_transport), GFP_KERNEL);
	if (!transport)
		return -ENOMEM;

	transport->ndev = ntb_register_transport(transport);
	if (!transport->ndev) {
		rc = -EIO;
		goto err;
	}

	transport->max_qps = ntb_query_max_cbs(transport->ndev);
	if (!transport->max_qps) {
		rc = -EIO;
		goto err1;
	}

	transport->qps = kcalloc(transport->max_qps,
				 sizeof(struct ntb_transport_qp),
				 GFP_KERNEL);
	if (!transport->qps) {
		rc = -ENOMEM;
		goto err1;
	}

	transport->qp_bitmap = ((u64) 1 << transport->max_qps) - 1;

	for (i = 0; i < NTB_NUM_MW; i++) {
		/* Alloc memory for receiving data.  Must be 4k aligned */
		transport->mw[i].size =
		    ALIGN(ntb_get_mw_size(transport->ndev, i), 4096);

		//FIXME - have hw return dev or pdev, and won't need struct ndev
		transport->mw[i].virt_addr =
		    dma_alloc_coherent(&transport->ndev->pdev->dev,
				       transport->mw[i].size,
				       &transport->mw[i].dma_addr,
				       GFP_KERNEL);
		if (!transport->mw[i].virt_addr) {
			rc = -ENOMEM;
			goto err2;
		}

		/* Notify HW the memory location of the receive buffer */
		ntb_set_mw_addr(transport->ndev, i, transport->mw[i].dma_addr);
	}

	spin_lock_init(&transport->lock);

	rc = ntb_register_event_callback(transport->ndev,
					 ntb_transport_event_callback);
	if (rc)
		goto err2;

	return 0;

err2:
	for (i--; i >= 0; i--)
		dma_free_coherent(&transport->ndev->pdev->dev,
				  transport->mw[i].size,
				  transport->mw[i].virt_addr,
				  transport->mw[i].dma_addr);
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
		dma_free_coherent(&transport->ndev->pdev->dev,
				  transport->mw[i].size,
				  transport->mw[i].virt_addr,
				  transport->mw[i].dma_addr);

	kfree(transport->qps);
	ntb_unregister_transport(transport->ndev);
	kfree(transport);
	transport = NULL;
}

static bool pass_check(u32 tx_orig, u32 tx_curr, u32 rx)
{
	if (tx_orig < rx)
		return (tx_curr >= rx);
	else	//tx_orig >= rx
	if (tx_curr < tx_orig)	//wrap
		return (tx_curr >= rx);
	else
		return (tx_curr < rx);
}

static bool pass_check_rx(u32 rx_orig, u32 rx_curr, u32 tx)
{
	if (rx_orig < tx)
		return (rx_curr > tx);
	else	//rx_orig >= tx
	if (rx_curr < rx_orig)	//wrap
		return (rx_curr > tx);
	else
		return (rx_curr < tx);
}

static int ntb_process_rxc(struct ntb_transport_qp *qp, u32 tx_offset)
{
	struct ntb_payload_header *hdr;
	struct ntb_queue_entry *entry;
	void *offset;
	bool oflow;

	if (qp->rx_offset == (qp->rx_buff_begin + tx_offset)) {
		qp->rx_ring_empty++;
		return -EAGAIN;
	}

	entry = ntb_list_rm_head(&qp->rxq_lock, &qp->rxq);
	if (!entry) {
		qp->rx_err_no_buf++;
		return -ENOMEM;
	}

	offset = qp->rx_offset;
	if (offset + sizeof(struct ntb_payload_header) >= qp->rx_buff_end)
		offset = qp->rx_buff_begin;

	hdr = offset;

	if (hdr->ver != qp->rx_pkts) {
		pr_err("qp %d: version mismatch, expected %Ld - got %Ld\n",
		       qp->qp_num, qp->rx_pkts, hdr->ver);
		ntb_list_add_tail(&qp->rxq_lock, &entry->entry, &qp->rxq);
		return -EIO;
	}

	if (!hdr->len) {
		pr_err("qp %d: Rx'ed pkt of len 0\n", qp->qp_num);
		ntb_list_add_tail(&qp->rxq_lock, &entry->entry, &qp->rxq);
		return -EIO;
	}

	pr_debug("rx offset %p, tx offset %x, ver %Ld - %d payload received, "
		 "buf size %d\n", qp->rx_offset, tx_offset, hdr->ver, hdr->len,
		 entry->len);
	if (hdr->len > entry->len) {
		pr_err("RX overflow! Wanted %d got %d\n", hdr->len, entry->len);
		ntb_list_add_tail(&qp->rxq_lock, &entry->entry, &qp->rxq);
		qp->rx_err_oflow++;
		oflow = true;
	} else {
		if (!hdr->link) {
			pr_info("qp %d: Link Down\n", qp->qp_num);
			qp->event_flags = NTB_LINK_DOWN;

			schedule_delayed_work(&qp->event_work,
					      msecs_to_jiffies(1000));
			oflow = true;
		} else {
			entry->len = hdr->len;
			oflow = false;
		}
	}

	offset += sizeof(struct ntb_payload_header);

	if (offset + hdr->len >= qp->rx_buff_end)
		offset = qp->rx_buff_begin;

	BUG_ON(offset >= qp->rx_buff_end || offset < qp->rx_buff_begin);

	if (!oflow)
		memcpy(entry->buf, offset, entry->len);
	//print_hex_dump_bytes(" ", 0, entry->buf, entry->len);

	offset += hdr->len;

	WARN_ON(pass_check_rx(qp->rx_offset - qp->rx_buff_begin,
			      offset - qp->rx_buff_begin, tx_offset));

	qp->rx_offset = offset;
	ntb_transport_put_remote_offset(qp, RX_OFFSET,
					offset - qp->rx_buff_begin);

	if (oflow)
		return 0;

	qp->rx_bytes += entry->len;
	qp->rx_pkts++;

	ntb_list_add_tail(&qp->rxc_lock, &entry->entry, &qp->rxc);

	if (qp->rx_handler && qp->client_ready)
		qp->rx_handler(qp);

	return 0;
}

static int ntb_transport_rxc(void *data)
{
	struct ntb_transport_qp *qp = data;
	u32 tx_offset;
	int rc;

	while (!kthread_should_stop()) {
		rc = ntb_transport_get_remote_offset(qp, TX_OFFSET, &tx_offset);
		if (rc)
			pr_err("%s: error reading from offset %d\n", __func__,
			       TX_OFFSET);

		rc = ntb_process_rxc(qp, tx_offset);
		if (!rc)
			continue;

		/* Sleep for a little bit and hope some memory frees up */
		if (rc == -ENOMEM)
			schedule_timeout_interruptible(msecs_to_jiffies
						       (qp->rx_ring_timeo));
		else {
			/* Sleep if no rx work */
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
			set_current_state(TASK_RUNNING);
		}
	}

	return 0;
}

static void ntb_transport_rxc_db(int db_num)
{
	struct ntb_transport_qp *qp = &transport->qps[db_num];

	pr_debug("%s: doorbell %d received\n", __func__, db_num);

	if (qp->qp_link == NTB_LINK_DOWN) {
		pr_info("QP %d Link Up\n", qp->qp_num);
		qp->event_flags = NTB_LINK_UP;

		schedule_delayed_work(&qp->event_work, msecs_to_jiffies(1000));
		return;
	}

	wake_up_process(qp->rx_work);
}

static u64 free_space(void *head, void *tail, u64 size)
{
	if (tail > head)
		return tail - head;
	else
		return size - (head - tail);
}

static int ntb_process_tx(struct ntb_transport_qp *qp,
			  struct ntb_queue_entry *entry,
			  u32 rx_offset)
{
	struct ntb_payload_header *hdr;
	void *offset;
	int rc;

	//pr_info("TX Entry len %d, free space %lld\n", entry->len, free_space(qp->tx_offset, qp->tx_mw_begin + rx_offset, (qp->tx_mw_end - qp->tx_mw_begin)));
	if (free_space(qp->tx_offset, qp->tx_mw_begin + rx_offset, (qp->tx_mw_end - qp->tx_mw_begin)) <= (entry->len + sizeof(struct ntb_payload_header)) * 2) {
		ntb_list_add_head(&qp->txq_lock, &entry->entry, &qp->txq);
		qp->tx_ring_full++;
		return -EAGAIN;
	}

	offset = qp->tx_offset;
	if (offset + sizeof(struct ntb_payload_header) >= qp->tx_mw_end)
		offset = qp->tx_mw_begin;

	hdr = offset;
	offset += sizeof(struct ntb_payload_header);

	/* If it can't fit into whats left of the ring, move to the beginning */
	if (offset + entry->len >= qp->tx_mw_end)
		offset = qp->tx_mw_begin;

	if (pass_check(qp->tx_offset - qp->tx_mw_begin,
		       (offset - qp->tx_mw_begin) + entry->len, rx_offset)) {
		ntb_list_add_head(&qp->txq_lock, &entry->entry, &qp->txq);
		qp->tx_ring_full++;
		return -EAGAIN;
	}

	pr_debug("%lld - tx %x, rx %x, entry len %d\n",
		 qp->tx_pkts, (u32) (qp->tx_offset - qp->tx_mw_begin),
		 rx_offset, entry->len);
	//print_hex_dump_bytes(" ", 0, entry->buf, entry->len);

	hdr->len = entry->len;
	hdr->ver = qp->tx_pkts;
	hdr->link = (entry->flags == NTB_LINK_UP);

	BUG_ON(offset >= qp->tx_mw_end || offset < qp->tx_mw_begin);
	memcpy(offset, entry->buf, entry->len);
	offset += entry->len;

	qp->tx_offset = offset;
	ntb_transport_put_remote_offset(qp, TX_OFFSET,
					offset - qp->tx_mw_begin);

	/* Ring doorbell notifying remote side of new packet */
	rc = ntb_ring_sdb(qp->ndev, qp->qp_num);
	if (rc)
		pr_err("%s: error ringing db %d\n", __func__, qp->qp_num);

	if (entry->len == 0) {
		ntb_list_add_tail(&qp->txe_lock, &entry->entry, &qp->txe);
		return 0;
	}

	qp->tx_pkts++;
	qp->tx_bytes += entry->len;

	/* Add fully transmitted data to completion queue */
	ntb_list_add_tail(&qp->txc_lock, &entry->entry, &qp->txc);

	if (qp->tx_handler)
		qp->tx_handler(qp);

	return 0;
}

static int ntb_transport_tx(void *data)
{
	struct ntb_transport_qp *qp = data;
	struct ntb_queue_entry *entry;
	u32 rx_offset;
	int rc;

	while (!kthread_should_stop()) {
		entry = ntb_list_rm_head(&qp->txq_lock, &qp->txq);
		if (!entry) {
			/* Sleep if no tx work */
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
			set_current_state(TASK_RUNNING);
			continue;
		}

		rc = ntb_transport_get_remote_offset(qp, RX_OFFSET, &rx_offset);
		if (rc)
			pr_err("%s: error reading from offset %d\n", __func__,
			       RX_OFFSET);

		rc = ntb_process_tx(qp, entry, rx_offset);
		if (!rc)
			continue;

		schedule_timeout_interruptible(msecs_to_jiffies(qp->tx_ring_timeo));
	}

	return 0;
}

static void ntb_transport_event_work(struct work_struct *work)
{
	struct ntb_transport_qp *qp = container_of(work, struct ntb_transport_qp,
						   event_work.work);

	if (qp->event_flags == qp->qp_link) {
		pr_err("Erroneous link event.  Link already %s\n",
		       (qp->qp_link == NTB_LINK_UP) ? "up" : "down");
		return;
	}

	if (qp->qp_link == NTB_LINK_DOWN) {
		cancel_delayed_work_sync(&qp->link_work);
		qp->qp_link = NTB_LINK_UP;
	} else {
		qp->qp_link = NTB_LINK_DOWN;
		qp->rx_pkts = 0;
		qp->tx_pkts = 0;
		qp->tx_offset = qp->tx_mw_begin;
		qp->rx_offset = qp->rx_buff_begin;
		ntb_transport_put_remote_offset(qp, RX_OFFSET, 0);
		ntb_transport_put_remote_offset(qp, TX_OFFSET, 0);

		if (qp->ndev->link_status)//FIXME - hw link status call?
			schedule_delayed_work(&qp->link_work, 0);
	}

	if (qp->event_handler)
		qp->event_handler(qp->event_flags);
}

static void ntb_transport_link_work(struct work_struct *work)
{
	struct ntb_transport_qp *qp = container_of(work, struct ntb_transport_qp,
						   link_work.work);
	int rc;

	/* Ring doorbell notifying remote side of new packet */
	rc = ntb_ring_sdb(qp->ndev, qp->qp_num);
	if (rc)
		pr_err("%s: error ringing db %d\n", __func__, qp->qp_num);

	if (qp->ndev->link_status) //FIXME - hw link status call?
		schedule_delayed_work(&qp->link_work, msecs_to_jiffies(1000));
}

static void ntb_send_link_down(struct ntb_transport_qp *qp)
{
	struct ntb_queue_entry *entry;

	if (qp->qp_link == NTB_LINK_DOWN)
		return;

	qp->qp_link = NTB_LINK_DOWN;

	//FIXME - should give this an upper bound...
	while (!(entry = ntb_list_rm_head(&qp->txe_lock, &qp->txe)))
		ssleep(1);

	entry->callback_data = NULL;
	entry->buf = NULL;
	entry->len = 0;
	entry->flags = NTB_LINK_DOWN;

	ntb_list_add_tail(&qp->txq_lock, &entry->entry, &qp->txq);

	wake_up_process(qp->tx_work);
}

static int ntb_transport_setup_mw(unsigned int new_qp_num)
{
	u8 mw_num = QP_TO_MW(new_qp_num);
	int i, num_qps = 0;
	unsigned long qp_bitmap = ~transport->qp_bitmap;
	unsigned int new_size;

	//determine number of mw's using the mw and who's using it
	for (i = mw_num; i < transport->max_qps; i += NTB_NUM_MW)
		if (qp_bitmap & (unsigned long) 1 << i) {
			struct ntb_transport_qp *qp = &transport->qps[i];

			pr_info("Shutting down qp %d while reconfiguring MW %d\n", i, mw_num);
			ntb_send_link_down(qp);

			num_qps++;
		}

	BUG_ON(!num_qps);
	//FIXME - this assumes mw on both systems are the same size
	new_size = transport->mw[mw_num].size / num_qps;
	pr_info("orig size = %d, num qps = %d, new size = %d\n", (int) transport->mw[mw_num].size, num_qps, new_size);

	for (i = mw_num; i < transport->max_qps; i += NTB_NUM_MW)
		if (qp_bitmap & (unsigned long) 1 << i) {
			struct ntb_transport_qp *qp = &transport->qps[i];

			//resize existing partitions buffs and setup new qps buff locations

			qp->rx_buff_begin = transport->mw[mw_num].virt_addr + (i / NTB_NUM_MW * new_size);
			qp->rx_buff_end = qp->rx_buff_begin + new_size;
			pr_info("QP %d - RX Buff start %p end %p\n", qp->qp_num, qp->rx_buff_begin, qp->rx_buff_end);
			qp->rx_offset = qp->rx_buff_begin;
			ntb_transport_put_remote_offset(qp, RX_OFFSET, 0);

			qp->tx_mw_begin = ntb_get_mw_vbase(qp->ndev, mw_num) + (i / NTB_NUM_MW * new_size);
			qp->tx_mw_end = qp->tx_mw_begin + new_size;
			pr_info("QP %d - TX MW start %p end %p\n", qp->qp_num, qp->tx_mw_begin, qp->tx_mw_end);
			qp->tx_offset = qp->tx_mw_begin;
			ntb_transport_put_remote_offset(qp, TX_OFFSET, 0);

			if (new_qp_num != i && qp->ndev->link_status) //FIXME - hw link status call?
				schedule_delayed_work(&qp->link_work, 0);
		}

	return 0;
}

/**
 * ntb_transport_create_queue - Create a new NTB transport layer queue
 * @rx_handler: receive callback function 
 * @tx_handler: transmit callback function 
 * @event_handler: event callback function 
 *
 * Create a new NTB transport layer queue and provide the queue with a callback
 * routine for both transmit and receive.  The receive callback routine will be
 * used to pass up data when the transport has received it on the queue.   The
 * transmit callback routine will be called when the transport has completed the
 * transmission of the data on the queue and the data is ready to be freed.
 *
 * RETURNS: pointer to newly created ntb_queue, NULL on error.
 */
struct ntb_transport_qp *ntb_transport_create_queue(handler rx_handler,
						    handler tx_handler,
						    ehandler event_handler)
{
	struct ntb_queue_entry *entry;
	struct ntb_transport_qp *qp;
	unsigned int free_queue;
	int rc, i;

	if (!transport) {
		rc = ntb_transport_init();
		if (rc)
			return NULL;
	}

	spin_lock(&transport->lock);

	//FIXME - need to handhake with remote side to determine matching number or some mapping between the 2
	free_queue = ffs(transport->qp_bitmap);
	if (!free_queue)
		goto err;

	/* decrement free_queue to make it zero based */
	free_queue--;

	clear_bit(free_queue, &transport->qp_bitmap);

	qp = &transport->qps[free_queue];
	qp->qp_link = NTB_LINK_DOWN;
	qp->qp_num = free_queue;
	qp->ndev = transport->ndev;

	qp->rx_ring_timeo = NTB_QP_DEF_RING_TIMEOUT;
	qp->tx_ring_timeo = NTB_QP_DEF_RING_TIMEOUT;

	if (qp->ndev->debugfs_dir) {
		char debugfs_name[4];

		snprintf(debugfs_name, 4, "qp%d", free_queue);
		qp->debugfs_dir = debugfs_create_dir(debugfs_name, qp->ndev->debugfs_dir);

		qp->debugfs_stats = debugfs_create_file("stats", S_IRUSR | S_IRGRP | S_IROTH, qp->debugfs_dir, qp, &ntb_qp_debugfs_stats);
		qp->debugfs_tx_to = debugfs_create_u32("tx_ring_timeo", S_IRUSR | S_IWUSR, qp->debugfs_dir, &qp->tx_ring_timeo);
		qp->debugfs_rx_to = debugfs_create_u32("rx_ring_timeo", S_IRUSR | S_IWUSR, qp->debugfs_dir, &qp->rx_ring_timeo);
	}

	qp->rx_handler = rx_handler;
	qp->tx_handler = tx_handler;
	qp->event_handler = event_handler;

	//FIXME - workqueue might be overkill, and might not need to be delayed...
	INIT_DELAYED_WORK(&qp->event_work, ntb_transport_event_work);
	INIT_DELAYED_WORK(&qp->link_work, ntb_transport_link_work);

	spin_lock_init(&qp->rxc_lock);
	spin_lock_init(&qp->rxq_lock);
	spin_lock_init(&qp->rxe_lock);
	spin_lock_init(&qp->txc_lock);
	spin_lock_init(&qp->txq_lock);
	spin_lock_init(&qp->txe_lock);

	INIT_LIST_HEAD(&qp->rxq);
	INIT_LIST_HEAD(&qp->rxc);
	INIT_LIST_HEAD(&qp->rxe);
	INIT_LIST_HEAD(&qp->txq);
	INIT_LIST_HEAD(&qp->txc);
	INIT_LIST_HEAD(&qp->txe);

	for (i = 0; i < NTB_QP_DEF_NUM_ENTRIES; i++) {
		entry = kzalloc(sizeof(struct ntb_queue_entry), GFP_ATOMIC);
		if (!entry)
			goto err1;

		ntb_list_add_tail(&qp->rxe_lock, &entry->entry, &qp->rxe);
	}

	for (i = 0; i < NTB_QP_DEF_NUM_ENTRIES; i++) {
		entry = kzalloc(sizeof(struct ntb_queue_entry), GFP_ATOMIC);
		if (!entry)
			goto err2;

		ntb_list_add_tail(&qp->txe_lock, &entry->entry, &qp->txe);
	}

	qp->rx_work = kthread_create(ntb_transport_rxc, qp, "ntb_rx%d", free_queue);
	if (IS_ERR(qp->rx_work)) {
		rc = PTR_ERR(qp->rx_work);
		pr_err("Error allocing rxc kthread\n");
		goto err2;
	}

	qp->tx_work = kthread_create(ntb_transport_tx, qp, "ntb_tx%d", free_queue);
	if (IS_ERR(qp->tx_work)) {
		rc = PTR_ERR(qp->tx_work);
		pr_err("Error allocing tx kthread\n");
		goto err3;
	}

	qp->rx_pkts = 0;
	qp->tx_pkts = 0;

	rc = ntb_transport_setup_mw(free_queue);
	if (rc)
		goto err4;

	//FIXME - transport->qps[free_queue].queue.hw_caps; is it even necessary for transport client to know?

	rc = ntb_register_db_callback(qp->ndev, free_queue,
				      ntb_transport_rxc_db);
	if (rc)
		goto err4;

	spin_unlock(&transport->lock);

	if (qp->ndev->link_status) //FIXME - hw link status call?
		schedule_delayed_work(&qp->link_work, 0);

	pr_info("NTB Transport QP %d created\n", qp->qp_num);

	return qp;

err4:
	kthread_stop(qp->tx_work);
err3:
	kthread_stop(qp->rx_work);
err2:
	while ((entry = ntb_list_rm_head(&qp->txe_lock, &qp->txe)))
		kfree(entry);
err1:
	while ((entry = ntb_list_rm_head(&qp->rxe_lock, &qp->rxe)))
		kfree(entry);
	debugfs_remove_recursive(qp->debugfs_stats);
	set_bit(free_queue, &transport->qp_bitmap);
err:
	spin_unlock(&transport->lock);
	return NULL;
}
EXPORT_SYMBOL(ntb_transport_create_queue);

/**
 * ntb_transport_free_queue - Frees NTB transport queue
 * @qp: NTB queue to be freed
 *
 * Frees NTB transport queue 
 */
void ntb_transport_free_queue(struct ntb_transport_qp *qp)
{
	struct ntb_queue_entry *entry;

	if (!qp)
		return;

	ntb_send_link_down(qp);

	cancel_delayed_work_sync(&qp->event_work);
	cancel_delayed_work_sync(&qp->link_work);

	debugfs_remove_recursive(qp->debugfs_dir);
	ntb_unregister_db_callback(qp->ndev, qp->qp_num);

	kthread_stop(qp->rx_work);
	kthread_stop(qp->tx_work);

	ntb_transport_put_remote_offset(qp, RX_OFFSET, 0);
	ntb_transport_put_remote_offset(qp, TX_OFFSET, 0);

	while ((entry = ntb_list_rm_head(&qp->rxe_lock, &qp->rxe)))
		kfree(entry);

	while ((entry = ntb_list_rm_head(&qp->rxq_lock, &qp->rxq))) {
		pr_warn("Freeing item from a non-empty queue\n");
		kfree(entry);
	}

	while ((entry = ntb_list_rm_head(&qp->rxc_lock, &qp->rxc))) {
		pr_warn("Freeing item from a non-empty queue\n");
		kfree(entry);
	}

	while ((entry = ntb_list_rm_head(&qp->txe_lock, &qp->txe)))
		kfree(entry);

	while ((entry = ntb_list_rm_head(&qp->txq_lock, &qp->txq))) {
		pr_warn("Freeing item from a non-empty queue\n");
		kfree(entry);
	}

	while ((entry = ntb_list_rm_head(&qp->txc_lock, &qp->txc))) {
		pr_warn("Freeing item from a non-empty queue\n");
		kfree(entry);
	}

	debugfs_remove_recursive(qp->debugfs_stats);

	set_bit(qp->qp_num, &transport->qp_bitmap);

	if (transport->qp_bitmap == ((u64) 1 << transport->max_qps) - 1)
		ntb_transport_free();
}
EXPORT_SYMBOL(ntb_transport_free_queue);

/**
 * ntb_transport_rx_remove - Dequeues enqueued rx packet
 * @qp: NTB queue to be freed
 * @len: pointer to variable to write enqueued buffers length
 *
 * Dequeues unused buffers from receive queue.  Should only be used during
 * shutdown of qp.
 *
 * RETURNS: NULL error value on error, or void* for success.
 */
void *ntb_transport_rx_remove(struct ntb_transport_qp *qp, int *len)
{
	struct ntb_queue_entry *entry;
	void *buf;

	if (!qp || qp->client_ready == NTB_LINK_UP)
		return NULL;

	entry = ntb_list_rm_head(&qp->rxq_lock, &qp->rxq);
	if (!entry)
		return NULL;

	buf = entry->callback_data;
	*len = entry->len;
	kfree(entry);

	return buf;
}
EXPORT_SYMBOL(ntb_transport_rx_remove);

/**
 * ntb_transport_rx_enqueue - Enqueue a new NTB queue entry
 * @qp: NTB transport layer queue the entry is to be enqueued on
 * @cb: per buffer pointer for callback function to use
 * @data: pointer to data buffer that incoming packets will be copied into
 * @len: length of the data buffer
 *
 * Enqueue a new receive buffer onto the transport queue into which a NTB
 * payload can be received into.
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int ntb_transport_rx_enqueue(struct ntb_transport_qp *qp, void *cb, void *data, int len)
{
	struct ntb_queue_entry *entry;

	if (!qp)
		return -EINVAL;

	entry = ntb_list_rm_head(&qp->rxe_lock, &qp->rxe);
	if (!entry)
		return -ENOMEM;

	entry->callback_data = cb;
	entry->buf = data;
	entry->len = len;

	ntb_list_add_tail(&qp->rxq_lock, &entry->entry, &qp->rxq);

	return 0;
}
EXPORT_SYMBOL(ntb_transport_rx_enqueue);

/**
 * ntb_transport_tx_enqueue - Enqueue a new NTB queue entry
 * @qp: NTB transport layer queue the entry is to be enqueued on
 * @cb: per buffer pointer for callback function to use
 * @data: pointer to data buffer that will be sent
 * @len: length of the data buffer
 *
 * Enqueue a new transmit buffer onto the transport queue from which a NTB
 * payload will be transmitted.
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int ntb_transport_tx_enqueue(struct ntb_transport_qp *qp, void *cb, void *data, int len)
{
	struct ntb_queue_entry *entry;

	if (!qp || qp->qp_link != NTB_LINK_UP)
		return -EINVAL;

	entry = ntb_list_rm_head(&qp->txe_lock, &qp->txe);
	if (!entry)
		return -ENOMEM;

	entry->callback_data = cb;
	entry->buf = data;
	entry->len = len;
	entry->flags = NTB_LINK_UP;

	ntb_list_add_tail(&qp->txq_lock, &entry->entry, &qp->txq);

	wake_up_process(qp->tx_work);

	return 0;
}
EXPORT_SYMBOL(ntb_transport_tx_enqueue);

/**
 * ntb_transport_tx_dequeue - Dequeue a NTB queue entry
 * @qp: NTB transport layer queue to be dequeued from
 * @len: length of the data buffer
 * 
 * This function will dequeue a buffer from the transmit complete queue.
 * Entries will only be enqueued on this queue after having been
 * transfered to the remote side.
 *
 * RETURNS: callback pointer of the buffer from the transport queue, or NULL
 * on empty
 */
void *ntb_transport_tx_dequeue(struct ntb_transport_qp *qp, int *len)
{
	struct ntb_queue_entry *entry;
	void *buf;

	if (!qp)
		return NULL;

	entry = ntb_list_rm_head(&qp->txc_lock, &qp->txc);
	if (!entry)
		return NULL;

	buf = entry->callback_data;
	*len = entry->len;

	//Sanity check for now
	entry->callback_data = NULL;
	entry->buf = NULL;
	entry->len = 0;

	ntb_list_add_tail(&qp->txe_lock, &entry->entry, &qp->txe);

	return buf;
}
EXPORT_SYMBOL(ntb_transport_tx_dequeue);

/**
 * ntb_transport_rx_dequeue - Dequeue a NTB queue entry
 * @qp: NTB transport layer queue to be dequeued from
 * @len: length of the data buffer
 * 
 * This function will dequeue a buffer from the receive complete queue.
 * Entries will only be enqueued on this queue after having been fully received.
 *
 * RETURNS: callback pointer of the buffer from the transport queue, or NULL
 * on empty
 */
void *ntb_transport_rx_dequeue(struct ntb_transport_qp *qp, int *len)
{
	struct ntb_queue_entry *entry;
	void *buf;

	if (!qp)
		return NULL;

	entry = ntb_list_rm_head(&qp->rxc_lock, &qp->rxc);
	if (!entry)
		return NULL;

	buf = entry->callback_data;
	*len = entry->len;

	//Sanity check for now
	entry->callback_data = NULL;
	entry->buf = NULL;
	entry->len = 0;

	ntb_list_add_tail(&qp->rxe_lock, &entry->entry, &qp->rxe);

	return buf;
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
 * ntb_transport_link_query - Query transport link state
 * @qp: NTB transport layer queue to be queried
 *
 * Query connectivity to the remote system of the NTB transport queue 
 *
 * RETURNS: true for link up or false for link down
 */
bool ntb_transport_link_query(struct ntb_transport_qp *qp)
{
	return qp->qp_link == NTB_LINK_UP;
}
EXPORT_SYMBOL(ntb_transport_link_query);

/**
 * ntb_transport_qp_num - Query the qp number
 * @qp: NTB transport layer queue to be queried
 *
 * Query qp number of the NTB transport queue
 *
 * RETURNS: a zero based number specifying the qp number
 */
unsigned char ntb_transport_qp_num(struct ntb_transport_qp *qp)
{
	return qp->qp_num;
}
EXPORT_SYMBOL(ntb_transport_qp_num);

/**
 * ntb_transport_max_size - Query the max payload size of a qp
 * @qp: NTB transport layer queue to be queried
 *
 * Query the maximum payload size permissible on the given qp
 *
 * RETURNS: the max payload size of a qp
 */
unsigned int ntb_transport_max_size(struct ntb_transport_qp *qp)
{
	return (qp->tx_mw_end - qp->tx_mw_begin) - sizeof(struct ntb_payload_header);
}
EXPORT_SYMBOL(ntb_transport_max_size);
