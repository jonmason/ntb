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
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/module.h>
#include "ntb_transport.h"

#define NTB_NETDEV_VER	"0.4"

MODULE_DESCRIPTION(KBUILD_MODNAME);
MODULE_VERSION(NTB_NETDEV_VER);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");

struct ntb_netdev {
	struct net_device *ndev;
	struct ntb_transport_qp *qp;
};

#define	NTB_TX_TIMEOUT_MS	1000
#define	NTB_RXQ_SIZE		100

struct net_device *netdev;

static void ntb_netdev_event_handler(int status)
{
	struct ntb_netdev *dev = netdev_priv(netdev);

	pr_debug("%s: Event %x, Link %x\n", KBUILD_MODNAME, status,
		 ntb_transport_link_query(dev->qp));

	/* Currently, only link status event is supported */
	if (status)
		netif_carrier_on(netdev);
	else
		netif_carrier_off(netdev);
}

static void ntb_netdev_rx_handler(struct ntb_transport_qp *qp)
{
	struct net_device *ndev = netdev;
	struct sk_buff *skb;
	int len, rc;

	while ((skb = ntb_transport_rx_dequeue(qp, &len))) {
		pr_debug("%s: %d byte payload received\n", __func__, len);

		skb_put(skb, len);
		skb->protocol = eth_type_trans(skb, ndev);
		skb->ip_summed = CHECKSUM_NONE;

		if (netif_rx(skb) == NET_RX_DROP) {
			ndev->stats.rx_errors++;
			ndev->stats.rx_dropped++;
		} else {
			ndev->stats.rx_packets++;
			ndev->stats.rx_bytes += len;
		}

		skb = netdev_alloc_skb(ndev, ndev->mtu + ETH_HLEN);
		if (!skb) {
			ndev->stats.rx_errors++;
			ndev->stats.rx_frame_errors++;
			pr_err("%s: No skb\n", __func__);
			break;
		}

		rc = ntb_transport_rx_enqueue(qp, skb, skb->data,
					      ndev->mtu + ETH_HLEN);
		if (rc) {
			ndev->stats.rx_errors++;
			ndev->stats.rx_fifo_errors++;
			pr_err("%s: error re-enqueuing\n", __func__);
			break;
		}
	}
}

static void ntb_netdev_tx_handler(struct ntb_transport_qp *qp)
{
	struct net_device *ndev = netdev;
	struct sk_buff *skb;
	int len;

	while ((skb = ntb_transport_tx_dequeue(qp, &len))) {
		ndev->stats.tx_packets++;
		ndev->stats.tx_bytes += skb->len;
		dev_kfree_skb(skb);
	}

	if (netif_queue_stopped(ndev))
		netif_wake_queue(ndev);
}

static netdev_tx_t ntb_netdev_start_xmit(struct sk_buff *skb,
					 struct net_device *ndev)
{
	struct ntb_netdev *dev = netdev_priv(ndev);
	int rc;

	pr_debug("%s: ntb_transport_tx_enqueue\n", KBUILD_MODNAME);

	rc = ntb_transport_tx_enqueue(dev->qp, skb, skb->data, skb->len);
	if (rc)
		goto err;

	return NETDEV_TX_OK;

err:
	ndev->stats.tx_dropped++;
	ndev->stats.tx_errors++;
	netif_stop_queue(ndev);
	return NETDEV_TX_BUSY;
}

static int ntb_netdev_open(struct net_device *ndev)
{
	struct ntb_netdev *dev = netdev_priv(ndev);
	struct sk_buff *skb;
	int rc, i, len;

	/* Add some empty rx bufs */
	for (i = 0; i < NTB_RXQ_SIZE; i++) {
		skb = netdev_alloc_skb(ndev, ndev->mtu + ETH_HLEN);
		if (!skb) {
			rc = -ENOMEM;
			goto err;
		}

		rc = ntb_transport_rx_enqueue(dev->qp, skb, skb->data,
					      ndev->mtu + ETH_HLEN);
		if (rc == -EINVAL)
			goto err;
	}

	netif_carrier_off(ndev);
	ntb_transport_link_up(dev->qp);

	return 0;

err:
	while ((skb = ntb_transport_rx_remove(dev->qp, &len)))
		kfree(skb);
	return rc;
}

static int ntb_netdev_close(struct net_device *ndev)
{
	struct ntb_netdev *dev = netdev_priv(ndev);
	struct sk_buff *skb;
	int len;

	ntb_transport_link_down(dev->qp);

	while ((skb = ntb_transport_rx_remove(dev->qp, &len)))
		kfree(skb);

	return 0;
}

static int ntb_netdev_change_mtu(struct net_device *ndev, int new_mtu)
{
	struct ntb_netdev *dev = netdev_priv(ndev);
	struct sk_buff *skb;
	int len, rc;

	if (new_mtu > ntb_transport_max_size(dev->qp) - ETH_HLEN)
		return -EINVAL;

	if (!netif_running(ndev)) {
		ndev->mtu = new_mtu;
		return 0;
	}

	/* Bring down the link and dispose of posted rx entries */
	ntb_transport_link_down(dev->qp);

	if (ndev->mtu < new_mtu) {
		int i;

		for (i = 0; (skb = ntb_transport_rx_remove(dev->qp, &len)); i++)
			kfree(skb);

		for (; i; i--) {
			skb = netdev_alloc_skb(ndev, new_mtu + ETH_HLEN);
			if (!skb) {
				rc = -ENOMEM;
				goto err;
			}

			rc = ntb_transport_rx_enqueue(dev->qp, skb, skb->data,
						      new_mtu + ETH_HLEN);
			if (rc) {
				kfree(skb);
				goto err;
			}
		}
	}

	ndev->mtu = new_mtu;

	ntb_transport_link_up(dev->qp);

	return 0;

err:
	ntb_transport_link_down(dev->qp);

	while ((skb = ntb_transport_rx_remove(dev->qp, &len)))
		kfree(skb);

	pr_err("Error changing MTU, device inoperable\n");
	return rc;
}

static void ntb_netdev_tx_timeout(struct net_device *ndev)
{
	if (netif_running(ndev))
		netif_wake_queue(ndev);
}

static const struct net_device_ops ntb_netdev_ops = {
	.ndo_open = ntb_netdev_open,
	.ndo_stop = ntb_netdev_close,
	.ndo_start_xmit = ntb_netdev_start_xmit,
	.ndo_change_mtu = ntb_netdev_change_mtu,
	.ndo_tx_timeout = ntb_netdev_tx_timeout,
	.ndo_set_mac_address = eth_mac_addr,
};

static void ntb_get_drvinfo(struct net_device *dev,
			    struct ethtool_drvinfo *info)
{
	strcpy(info->driver, KBUILD_MODNAME);
	strcpy(info->version, NTB_NETDEV_VER);
}

static const char ntb_nic_stats[][ETH_GSTRING_LEN] = {
	"rx_packets", "rx_bytes", "rx_errors", "rx_dropped", "rx_length_errors",
	"rx_frame_errors", "rx_fifo_errors",
	"tx_packets", "tx_bytes", "tx_errors", "tx_dropped",
};

static int ntb_get_stats_count(struct net_device *dev)
{
	return ARRAY_SIZE(ntb_nic_stats);
}

static int ntb_get_sset_count(struct net_device *dev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return ntb_get_stats_count(dev);
	default:
		return -EOPNOTSUPP;
	}
}

static void ntb_get_strings(struct net_device *dev, u32 sset, u8 *data)
{
	switch (sset) {
	case ETH_SS_STATS:
		memcpy(data, *ntb_nic_stats, sizeof(ntb_nic_stats));
	}
}

static void ntb_get_ethtool_stats(struct net_device *dev,
				  struct ethtool_stats *stats, u64 *data)
{
	int i = 0;

	data[i++] = dev->stats.rx_packets;
	data[i++] = dev->stats.rx_bytes;
	data[i++] = dev->stats.rx_errors;
	data[i++] = dev->stats.rx_dropped;
	data[i++] = dev->stats.rx_length_errors;
	data[i++] = dev->stats.rx_frame_errors;
	data[i++] = dev->stats.rx_fifo_errors;
	data[i++] = dev->stats.tx_packets;
	data[i++] = dev->stats.tx_bytes;
	data[i++] = dev->stats.tx_errors;
	data[i++] = dev->stats.tx_dropped;
}

static const struct ethtool_ops ntb_ethtool_ops = {
	.get_drvinfo = ntb_get_drvinfo,
	.get_sset_count = ntb_get_sset_count,
	.get_strings = ntb_get_strings,
	.get_ethtool_stats = ntb_get_ethtool_stats,
	.get_link = ethtool_op_get_link,
};

static int __init ntb_netdev_init_module(void)
{
	struct ntb_netdev *dev;
	int rc;

	pr_info("%s: Probe\n", KBUILD_MODNAME);

	netdev = alloc_etherdev(sizeof(struct ntb_netdev));
	if (!netdev)
		return -ENOMEM;

	dev = netdev_priv(netdev);
	dev->ndev = netdev;
	netdev->features = NETIF_F_HIGHDMA;

	netdev->hw_features = netdev->features;
	netdev->watchdog_timeo = msecs_to_jiffies(NTB_TX_TIMEOUT_MS);

	random_ether_addr(netdev->perm_addr);
	memcpy(netdev->dev_addr, netdev->perm_addr, netdev->addr_len);

	netdev->netdev_ops = &ntb_netdev_ops;
	SET_ETHTOOL_OPS(netdev, &ntb_ethtool_ops);

	dev->qp = ntb_transport_create_queue(ntb_netdev_rx_handler,
					     ntb_netdev_tx_handler,
					     ntb_netdev_event_handler);
	if (!dev->qp) {
		rc = -EIO;
		goto err;
	}

	netdev->mtu = ntb_transport_max_size(dev->qp) - ETH_HLEN;

	rc = register_netdev(netdev);
	if (rc)
		goto err1;

	pr_info("%s: %s created\n", KBUILD_MODNAME, netdev->name);
	return 0;

err1:
	ntb_transport_free_queue(dev->qp);
err:
	free_netdev(netdev);
	return rc;
}
module_init(ntb_netdev_init_module);

static void __exit ntb_netdev_exit_module(void)
{
	struct ntb_netdev *dev = netdev_priv(netdev);

	unregister_netdev(netdev);
	ntb_transport_free_queue(dev->qp);
	free_netdev(netdev);

	pr_info("%s: Driver removed\n", KBUILD_MODNAME);
}
module_exit(ntb_netdev_exit_module);
