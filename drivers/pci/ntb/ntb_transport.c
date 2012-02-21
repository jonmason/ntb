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
	struct list_head rxq;
	struct list_head rx_comp_q;
	void (*rx_handler)(struct ntb_transport_qp *qp);
	void (*tx_handler)(struct ntb_transport_qp *qp);
	void (*event_handler)(int status);
	struct delayed_work event_work;
	unsigned int event_flags:1;
	unsigned int link:1;
	unsigned int qp_num:6;/* Only 64 QP's are allowed.  0-63 */
};

struct ntb_transport {
	struct ntb_device *ndev;
	size_t payload_size;
	void *payload_virt_addr;
	dma_addr_t payload_dma_addr;

	unsigned int max_qps;
	struct ntb_transport_qp *qps;
	unsigned long qp_bitmap;
};

struct ntb_transport *transport = NULL;

//FIXME - reorder the functions to remove the need for these pre-declirations
int ntb_transport_init(void);
void ntb_transport_free(void);

static void ntb_transport_dbcb(int db_num)
{
	struct device *dev = &transport->ndev->pdev->dev;

	//FIXME - this should kick off wq thread or we should document that it runs in irq context

	//FIXME - do some magic to move rx'ed frame off of rxq and onto rx_comp_q

	dev_info(dev, "%s: doorbell %d received\n", __func__, db_num);
	transport->qps[db_num].rx_handler(&transport->qps[db_num]);
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

	if (!transport) {
		rc = ntb_transport_init();
		if (rc)
			return NULL; //FIXME  - use ERR_PTR for all NULL
	}

	if (!transport->qp_bitmap)
		return NULL;

	//FIXME - need to handhake with remote side to determine matching number or some mapping between the 2
	free_queue = ffs(transport->qp_bitmap);
	clear_bit(free_queue, &transport->qp_bitmap);//FIXME - this might be racy, either add lock or make atomic

	qp = &transport->qps[free_queue];
	qp->qp_num = free_queue;

	qp->rx_handler = rx_handler;
	qp->tx_handler = tx_handler;

	INIT_LIST_HEAD(&qp->rxq);
	INIT_LIST_HEAD(&qp->rx_comp_q);
	INIT_LIST_HEAD(&qp->txq);
	INIT_LIST_HEAD(&qp->tx_comp_q);

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
	return -EINVAL;
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
	struct scatterlist *sg;
	int rc;
	struct device *dev = &transport->ndev->pdev->dev;

	dev_info(dev, "%s\n", __func__);
	if (!qp || qp->link != NTB_LINK_UP)
		return -EINVAL;

//FIXME - see also sg_copy_to_buffer
	for (sg = &entry->sg; sg; sg = sg_next(sg))
		memcpy((void *)transport->payload_dma_addr, (void *)sg_dma_address(sg), sg_dma_len(sg));
//FIXME - copying over same memory each time

	/* Ring doorbell notifying remote side of new packet */
	rc = ntb_ring_sdb(transport->ndev, 1 << qp->qp_num);
	if (rc)
		return rc;

	dev_info(dev, "%s: ringing doorbell %d\n", __func__, qp->qp_num);

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

	//FIXME - hack to make link up even fire immediately after registering
	INIT_DELAYED_WORK(&qp->event_work, ntb_transport_event_work);
	qp->event_flags = NTB_LINK_UP;//FIXME -racy
	schedule_delayed_work(&qp->event_work, msecs_to_jiffies(1000));//FIXME - magic number

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

int ntb_transport_init()
{
	int rc;

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
		goto err;
	}

	transport->qps = kcalloc(transport->max_qps, sizeof(struct ntb_transport_qp), GFP_KERNEL);
	if (!transport->qps) {
		rc = -ENOMEM;
		goto err;
	}

	transport->qp_bitmap = ~(transport->max_qps - 1);

	//alloc memory to be transfered into
	/* Must be 4k aligned */
	transport->payload_size = ALIGN(PAGE_SIZE, 4096);
	//FIXME - might not need to be coherent if we don't ever touch it by the cpu on this side
	transport->payload_virt_addr = dma_alloc_coherent(&transport->ndev->pdev->dev, transport->payload_size, &transport->payload_dma_addr, GFP_KERNEL);
	if (!transport->payload_virt_addr) {
		rc = -ENOMEM;
		goto err1;
	}

	//set mem addr to SBAR2XLAT to specify where incoming messages should go
	ntb_set_mw_addr(transport->ndev, 0, transport->payload_dma_addr);

	rc = ntb_register_event_callback(transport->ndev, ntb_transport_event_callback);
	if (rc)
		return rc;//FIXME - cleanup
#if 0
	//setup/init transport
	int ntb_get_max_spads(struct ntb_device *ndev);
	int ntb_write_spad(struct ntb_device *ndev, unsigned int idx, u32 val);
	int ntb_read_spad(struct ntb_device *ndev, unsigned int idx, u32 *val);
	void *ntb_get_pbar_vbase(struct ntb_device *ndev, unsigned int bar);
	resource_size_t ntb_get_pbar_size(struct ntb_device *ndev, unsigned int bar);
	int ntb_ring_sdb(struct ntb_device *ndev, unsigned int idx);


	//*register callbacks
	int ntb_register_db_callback(struct ntb_device *ndev, unsigned int idx, db_cb_func func);
	int ntb_register_event_callback(struct ntb_device *ndev, event_cb_func func);
#endif
	return 0;

err1:
	ntb_unregister_transport(transport->ndev);
err:
	kfree(transport);
	return rc;
} 
EXPORT_SYMBOL(ntb_transport_init);


void ntb_transport_free()
{
	dma_free_coherent(&transport->ndev->pdev->dev, transport->payload_size, transport->payload_virt_addr, transport->payload_dma_addr);
	ntb_unregister_transport(transport->ndev);
	kfree(transport);
#if 0
	//teardown transport
	//*free callbacks
	void ntb_unregister_event_callback(struct ntb_device *ndev);
	void ntb_unregister_db_callback(struct ntb_device *ndev, unsigned int idx);
	//*unregister transprot with dev
	void ntb_unregister_transport(struct ntb_device *ndev);
#endif
}
EXPORT_SYMBOL(ntb_transport_free);
