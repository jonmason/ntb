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
#include <linux/random.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

MODULE_DESCRIPTION(KBUILD_MODNAME);
MODULE_VERSION("0.1");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");

struct ntb_dummy_dev {
	struct ntb_transport_qp *qp;
	struct dentry *debug_dir;
	struct dentry *debug_rx;
	struct dentry *debug_tx;
};

struct ntb_dummy_dev *dev;

static void ntb_dummy_rx_handler(struct ntb_transport_qp *qp)
{
	pr_info("%s\n", __func__);
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
}

static int ntb_debug_tx_open(struct inode *inode, struct file *file)
{
	struct ntb_queue_entry *entry;
	char *buf;
	int i, rc;

	entry = kzalloc(sizeof(struct ntb_queue_entry), GFP_KERNEL);
	if (!entry)
		return -ENOMEM;

	buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

#if 0
	for (i = 0; i < PAGE_SIZE; i++)
		buf[i] = i;
#else
	get_random_bytes(buf, PAGE_SIZE);
#endif

	sg_init_one(&entry->sg, buf, PAGE_SIZE);

	//FIXME - rc = ntb_transport_rx_enqueue(dev->qp, entry); prepost rxd's
	rc = ntb_transport_tx_enqueue(dev->qp, entry);
	if (rc)
		return -EIO;//FIXME - need to cle

	pr_info("%s: ntb_transport_tx_enqueue\n", KBUILD_MODNAME);

	return 0;
}

static const struct file_operations ntb_tx_fops = {
	.owner = THIS_MODULE,
	//.read = seq_read,
	//.llseek = seq_lseek,
	//.release = single_release,
	.open = ntb_debug_tx_open,
	//.write = 
};

static int ntb_debug_show_rx(struct seq_file *s, void *unused)
{
	struct ntb_queue_entry *entry;
	char *buf;
	int i;

	entry = ntb_transport_rx_dequeue(dev->qp);
	if (!entry)
		return 0;

#if 0
	pg = sg_page(&entry->sg);
	buf = (char *)page_to_phys(pg);
#else
	buf = (char *)sg_phys(&entry->sg);
#endif

	seq_printf(s, "Payload received\n");
	for (i = 0; i < PAGE_SIZE; i++)
		seq_printf(s, "addr %p: %x", buf + i, buf[i]);

	return 0;
}

static int ntb_debug_rx_open(struct inode *inode, struct file *file)
{
	return single_open(file, ntb_debug_show_rx, NULL);
}

static const struct file_operations ntb_rx_fops = {
	.owner = THIS_MODULE,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.open = ntb_debug_rx_open,
	//.write = 
};

static int __init ntb_dummy_init_module(void)
{
	int rc;

	pr_info("%s: Probe\n", KBUILD_MODNAME);

	dev = kzalloc(sizeof(struct ntb_dummy_dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->qp = ntb_transport_create_queue(ntb_dummy_rx_handler, ntb_dummy_tx_handler);
	if (!dev->qp) {
		rc = -EIO;
		goto err;
	}

	rc = ntb_transport_reg_event_callback(dev->qp, ntb_dummy_event_handler);
	if (rc)
		goto err1;



	dev->debug_dir = debugfs_create_dir(KBUILD_MODNAME, NULL);
	if (!dev->debug_dir)
		return -ENOMEM; //FIXME - return errono and handle exit properly

	dev->debug_rx = debugfs_create_file("receive", S_IRUSR, dev->debug_dir, NULL, &ntb_rx_fops);
	if (!dev->debug_rx)
		return -ENOMEM; //FIXME - return errono and handle exit properly

	dev->debug_tx = debugfs_create_file("transmit", S_IRUSR, dev->debug_dir, NULL, &ntb_tx_fops);
	if (!dev->debug_tx)
		return -ENOMEM; //FIXME - return errono and handle exit properly

	ntb_transport_link_up(dev->qp);

	return 0;

err1:
	ntb_transport_free_queue(dev->qp);
err:
	kfree(dev);
	return rc;
}
module_init(ntb_dummy_init_module);

static void __exit ntb_dummy_exit_module(void)
{
	debugfs_remove(dev->debug_tx);
	debugfs_remove(dev->debug_rx);
	debugfs_remove(dev->debug_dir);

	ntb_transport_link_down(dev->qp);
	ntb_transport_free_queue(dev->qp);
	kfree(dev);

	pr_info("%s: Driver removed\n", KBUILD_MODNAME);
}
module_exit(ntb_dummy_exit_module);
