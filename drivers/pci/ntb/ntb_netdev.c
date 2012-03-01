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
 * Intel PCIe NTB Network Linux driver
 *
 * Contact Information:
 * Jon Mason <jon.mason@intel.com>
 */

struct ntb_netdev {
	struct ntb_transport_qp *qp;
};

#define	NTB_TX_TIMEOUT_MS	1000

struct net_device *netdev;

static struct ntb_queue_entry *alloc_entry(unsigned int len)
{
	struct ntb_queue_entry *entry;
	char *buf;

	entry = kzalloc(sizeof(struct ntb_queue_entry), GFP_KERNEL);
	if (!entry)
		goto err;

	buf = kmalloc(len, GFP_KERNEL);
	if (!buf)
		goto err1;

	sg_init_one(&entry->sg, buf, len);

	return entry;

err1:
	kfree(entry);
err:
	return NULL;
}

static void free_entry(struct ntb_queue_entry *entry)
{
	kfree(sg_virt(&entry->sg));
	kfree(entry);
}

static void ntb_netdev_event_handler(int status)
{
	pr_info("%s: Event %x, Link %x\n", KBUILD_MODNAME, status, ntb_transport_hw_link_query(dev->qp));

	//FIXME - currently, only link status event is supported
	if (ntb_transport_hw_link_query(dev->qp))
		netif_carrier_on(dev);
	else
		netif_carrier_off(dev);
}

static void ntb_netdev_rx_handler(struct ntb_transport_qp *qp)
{
	struct ntb_queue_entry *entry;
	u32 *buf;
	int i;

	pr_info("%s\n", __func__);

	entry = ntb_transport_rx_dequeue(dev->qp);
	if (!entry)
		return;

	buf = (u32 *)sg_virt(&entry->sg);

	pr_info("%d byte payload received\n", entry->sg.length);
	for (i = 0; i < 64; i += sizeof(u32))
		pr_info("addr %p: %x", buf + i, buf[i]);

	free_entry(entry);
}

static void ntb_netdev_tx_handler(struct ntb_transport_qp *qp)
{
	struct ntb_queue_entry *entry;

	pr_info("%s\n", __func__);

	entry = ntb_transport_tx_dequeue(qp);
	if (!entry)
		return;

	free_entry(entry);
}

static netdev_tx_t ntb_netdev_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct ntb_queue_entry *entry;
	int rc;

	pr_info("%s: ntb_transport_tx_enqueue\n", KBUILD_MODNAME);

	entry = alloc_entry(PAGE_SIZE);
	if (!entry) {
		rc = -ENOMEM;
		goto err;
	}

	sg_init_one(&entry->sg, buf, len);

	rc = ntb_transport_tx_enqueue(dev->qp, entry);
	if (rc)
		goto err1;

	return 0;

err1:
	free_entry(entry);
err:
	return rc;
}

static int ntb_netdev_open(struct net_device *netdev)
{
	struct ntb_queue_entry *entry;
	struct ntb_netdev *dev = netdev_priv(netdev);
	int rc;

	dev->qp = ntb_transport_create_queue(ntb_netdev_rx_handler, ntb_netdev_tx_handler);
	if (!dev->qp) {
		rc = -EIO;
		goto err;
	}

	rc = ntb_transport_reg_event_callback(dev->qp, ntb_netdev_event_handler);
	if (rc)
		goto err1;

	//FIXME - add more bufs...
	/* Add some empty rx bufs */
	entry = alloc_entry(dev->mtu);
	if (!entry) {
		rc = -ENOMEM;
		goto err1;
	}

	rc = ntb_transport_rx_enqueue(dev->qp, entry);
	if (rc)
		goto err2;

	ntb_transport_link_up(dev->qp);

	if (ntb_transport_hw_link_query(dev->qp))
		netif_carrier_on(dev);
	else
		netif_carrier_off(dev);

	return 0;

err2:
	free_entry(entry);
err1:
	ntb_transport_free_queue(dev->qp);
err:
	return rc;
}

static int ntb_netdev_close(struct net_device *netdev)
{
	struct ntb_netdev *dev = netdev_priv(netdev);

	//FIXME - purge rxq
	ntb_transport_link_down(dev->qp);
	ntb_transport_free_queue(dev->qp);

	return 0;
}

static int ntb_netdev_change_mtu(struct net_device *dev, int new_mtu)
{
	//FIXME - check against size of mw
	if (0)
		return -EINVAL;

	dev->mtu = new_mtu;

	return 0;
}

static void ntb_netdev_tx_timeout(struct net_device *dev)
{
	pr_err("%s\n", __func__);

	ntb_netdev_open(dev);//FIXME - do something with rc
	ntb_netdev_close(dev);//FIXME - do something with rc
}

static const struct net_device_ops ntb_netdev_ops = {
	.ndo_open = ntb_netdev_open,
	.ndo_stop = ntb_netdev_close,
	.ndo_start_xmit = ntb_netdev_start_xmit,
	.ndo_change_mtu = ntb_netdev_change_mtu,
	.ndo_tx_timeout = ntb_netdev_tx_timeout,
};

static int __init ntb_netdev_init_module(void)
{
	struct ntb_netdev *dev;
	int rc;

	pr_info("%s: Probe\n", KBUILD_MODNAME);

	netdev = alloc_etherdev((sizeof(struct net_netdev)));//FIXME - might be worth trying multiple queues...
	if (!netdev)
		return -ENOMEM;

	dev = netdev_priv(netdev);
	dev->features = NETIF_F_HIGHDMA; //FIXME - check this against the flags returned by the ntb dev???
	//FIXME - there is bound to be more flags supported.  NETIF_F_SG  comes to mind, prolly NETIF_F_FRAGLIST, NETIF_F_NO_CSUM, 

	dev->hw_features = dev->features;
	dev->watchdog_timeo = msecs_to_jiffies(NTB_TX_TIMEOUT_MS);

	dev->mtu = PAGE_SIZE;

	random_ether_addr(dev->perm_addr);
	memcpy(dev->dev_addr, dev->perm_addr, dev->addr_len);

	ndev->netdev_ops = &ntb_netdev_ops;
	//SET_ETHTOOL_OPS(ndev, &ethtool_ops);
	//SET_NETDEV_DEV(ndev, &pdev->dev);

	rc = register_netdev(netdev);
	if (rc)
		goto err;

	return 0;

err:
	free_netdev(netdev);
	return rc;
}
module_init(ntb_netdev_init_module);

static void __exit ntb_netdev_exit_module(void)
{
	unregister_netdev(netdev);
	free_netdev(netdev);

	pr_info("%s: Driver removed\n", KBUILD_MODNAME);
}
module_exit(ntb_netdev_exit_module);
