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

#include <linux/module.h>
#include <linux/types.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include "ntb_transport.h"

MODULE_DESCRIPTION(KBUILD_MODNAME);
MODULE_VERSION("0.1");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");

struct ntb_dummy_dev {
	struct ntb_transport_qp *qp;
};

struct ntb_dummy_dev *dev;

static void ntb_dummy_rx_handler(struct ntb_transport_qp *qp)
{
	struct ntb_queue_entry *entry;
	//struct page *pg;
	char *buf;
	int i;

	entry = ntb_transport_rx_dequeue(qp);
	if (!entry)
		return;

#if 0
	pg = sg_page(&entry->sg);
	buf = (char *)page_to_phys(pg);
#else
	buf = (char *)sg_phys(&entry->sg);
#endif

	for (i = 0; i < 100; i++)
		pr_info("addr %p: %x", buf + i, buf[i]);
}

static void ntb_dummy_tx_handler(struct ntb_transport_qp *qp)
{
	struct ntb_queue_entry *entry;

	entry = ntb_transport_tx_dequeue(qp);
	if (!entry)
		return;

	__free_page(sg_page(&entry->sg));
	kfree(entry);
}

static void ntb_dummy_event_handler(int status)
{
	pr_info("%s: Event %x, Link %x\n", KBUILD_MODNAME, status, ntb_transport_hw_link_query(dev->qp));

	if (status & LINK_EVENT && ntb_transport_hw_link_query(dev->qp)) {
		struct ntb_queue_entry *entry;
		char *buf;
		int i, rc;

		entry = kzalloc(sizeof(struct ntb_queue_entry), GFP_KERNEL);
		if (!entry)
			return;

		buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
		if (!buf)
			return;

		for (i = 0; i < 100; i++)
			buf[i] = i;

		sg_init_one(&entry->sg, buf, PAGE_SIZE);
		entry->num_sg = 1;//FIXME - unnecessary

		//FIXME - rc = ntb_transport_rx_enqueue(dev->qp, entry); prepost rxd's
		rc = ntb_transport_tx_enqueue(dev->qp, entry);
		if (rc)
			return;//FIXME - need to cle
		pr_info("%s: ntb_transport_tx_enqueue\n", KBUILD_MODNAME);
	}
}

static int __init ntb_dummy_init_module(void)
{
	int rc;

	pr_info("%s: Probe\n", KBUILD_MODNAME);

	dev = kzalloc(sizeof(struct ntb_dummy_dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	rc = ntb_transport_init();
	if (rc)
		goto err;

	dev->qp = ntb_transport_create_queue(ntb_dummy_rx_handler, ntb_dummy_tx_handler);
	if (!dev->qp) {
		rc = -EIO;
		goto err1;
	}

	rc = ntb_transport_reg_event_callback(dev->qp, ntb_dummy_event_handler);
	if (rc)
		goto err2;

	ntb_transport_link_up(dev->qp);

	return 0;

err2:
	ntb_transport_free_queue(dev->qp);
err1:
	ntb_transport_free();
err:
	kfree(dev);
	return rc;
}
module_init(ntb_dummy_init_module);

static void __exit ntb_dummy_exit_module(void)
{
	ntb_transport_link_down(dev->qp);
	ntb_transport_free_queue(dev->qp);
	ntb_transport_free();
	kfree(dev);

	pr_info("%s: Driver removed\n", KBUILD_MODNAME);
}
module_exit(ntb_dummy_exit_module);
