/*
 * Intel PCIe NTB Linux driver
 *
 * Copyright(c) 2012 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * The full GNU General Public License is included in this distribution
 * in the file called LICENSE.GPL.
 *
 * Contact Information:  FIXME
 * Jon Mason <jon.mason@intel.com>
 */

struct ntb_queue {
	struct list_head list;
	/* Transmit or receive queue...may not be necessary */
	unsigned int dir:1;
	/* Remote side capabilities (e.g., DMA read) */
	unsigned int hw_caps;
};

struct ntb_queue_entry {
	struct list_head entry;
	struct scatterlist *sg;
	unsigned int num_sg;
	bool done;
};

struct ntb_queue *
ntb_transport_create_queue(int dir, int (*handler)(struct ntb_queue *queue));
void ntb_transport_free_queue(struct ntb_queue *queue);
int ntb_transport_enqueue(struct ntb_queue *q, struct ntb_queue_entry *entry);
struct ntb_queue_entry *ntb_tranport_dequeue(struct ntb_queue *queue);
int ntb_transport_link_up(struct ntb_queue *queue);
int ntb_transport_link_down(struct ntb_queue *queue);
int ntb_transport_reg_event_callback(void (*handler)(int status));
bool ntb_tranport_hw_link_query(struct ntb_queue *queue);
