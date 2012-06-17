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

#define NTB_NETDEV_VER	"0.5"

MODULE_DESCRIPTION(KBUILD_MODNAME);
MODULE_VERSION(NTB_NETDEV_VER);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");

static int num_qps = 1;
module_param(num_qps, uint, 0644);
MODULE_PARM_DESC(num_qps, "Number of NTB transport connections");

struct ntb_netdev {
	struct net_device *ndev;
	struct ntb_transport_qp **qp;
	struct work_struct txto_work;
};

#define	NTB_TX_TIMEOUT_MS	1000
#define	NTB_RXQ_SIZE		1000

struct net_device *netdev;

static void ntb_netdev_event_handler(int status)
{
//FIXME - need counter to see if max, then enable

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
	u16 rx_queue;

	rx_queue = ntb_transport_qp_num(qp);

	while ((skb = ntb_transport_rx_dequeue(qp, &len))) {
		pr_debug("%s: %d byte payload received on qp %d\n", __func__, len, rx_queue);

		skb_put(skb, len);
		skb->protocol = eth_type_trans(skb, ndev);
		skb->ip_summed = CHECKSUM_NONE;
		skb_record_rx_queue(skb, rx_queue);

		if (netif_rx(skb) == NET_RX_DROP) {
			ndev->stats.rx_errors++;
			ndev->stats.rx_dropped++;
		} else {
			ndev->stats.rx_packets++;
			ndev->stats.rx_bytes += len;
		}

		skb = netdev_alloc_skb(ndev, ndev->mtu + ETH_HLEN + NET_IP_ALIGN);
		if (!skb) {
			ndev->stats.rx_errors++;
			ndev->stats.rx_frame_errors++;
			pr_err("%s: No skb\n", __func__);
			break;
		}

		skb_reserve(skb, NET_IP_ALIGN);

		rc = ntb_transport_rx_enqueue(qp, skb, skb->data, ndev->mtu + ETH_HLEN);
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
		struct netdev_queue *txq = netdev_get_tx_queue(ndev, skb->queue_mapping);

		ndev->stats.tx_packets++;
		ndev->stats.tx_bytes += skb->len;
		dev_kfree_skb(skb);

		if (netif_tx_queue_stopped(txq))
			netif_tx_wake_queue(txq);
	}
}

static netdev_tx_t ntb_netdev_start_xmit(struct sk_buff *skb,
					 struct net_device *ndev)
{
	struct ntb_netdev *dev = netdev_priv(ndev);
	struct netdev_queue *txq;
	int rc, qp_num;

	qp_num = skb->queue_mapping;
	txq = netdev_get_tx_queue(ndev, qp_num);

	pr_debug("%s: ntb_transport_tx_enqueue on qp %d\n", KBUILD_MODNAME, qp_num);

	rc = ntb_transport_tx_enqueue(dev->qp[qp_num], skb, skb->data, skb->len);
	if (rc)
		goto err;

	return NETDEV_TX_OK;

err:
	ndev->stats.tx_dropped++;
	ndev->stats.tx_errors++;
	netif_tx_stop_queue(txq);
	kfree_skb(skb);
	return NETDEV_TX_OK;
}

static int ntb_netdev_open(struct net_device *ndev)
{
	struct ntb_netdev *dev = netdev_priv(ndev);
	struct sk_buff *skb;
	int rc, qp_num, i, len;

	netif_carrier_off(ndev);

	/* Add some empty rx bufs */
	for (qp_num = 0; qp_num < num_qps; qp_num++)
		for (i = 0; i < NTB_RXQ_SIZE; i++) {
			skb = netdev_alloc_skb(ndev, ndev->mtu + ETH_HLEN + NET_IP_ALIGN);
			if (!skb) {
				rc = -ENOMEM;
				goto err;
			}

			skb_reserve(skb, NET_IP_ALIGN);

			rc = ntb_transport_rx_enqueue(dev->qp[qp_num], skb, skb->data, ndev->mtu + ETH_HLEN);
			if (rc)
				goto err;
		}


	for (qp_num = 0; qp_num < num_qps; qp_num++)
		ntb_transport_link_up(dev->qp[qp_num]);

	return 0;

err:
	for (qp_num = 0; qp_num < num_qps; qp_num++)
		while ((skb = ntb_transport_rx_remove(dev->qp[qp_num], &len)))
			kfree(skb);
	return rc;
}

static int ntb_netdev_close(struct net_device *ndev)
{
	struct ntb_netdev *dev = netdev_priv(ndev);
	struct sk_buff *skb;
	int len, qp_num;

	for (qp_num = 0; qp_num < num_qps; qp_num++) {
		ntb_transport_link_down(dev->qp[qp_num]);

		while ((skb = ntb_transport_rx_remove(dev->qp[qp_num], &len)))
			kfree(skb);
	}

	return 0;
}

static int ntb_netdev_change_mtu(struct net_device *ndev, int new_mtu)
{
	struct ntb_netdev *dev = netdev_priv(ndev);
	struct sk_buff *skb;
	int len, qp_num, rc;

	for (qp_num = 0; qp_num < num_qps; qp_num++)
		if (new_mtu > ntb_transport_max_size(dev->qp[qp_num]) - ETH_HLEN)
			return -EINVAL;

	if (!netif_running(ndev)) {
		ndev->mtu = new_mtu;
		return 0;
	}

	/* Bring down the link and dispose of posted rx entries */
	for (qp_num = 0; qp_num < num_qps; qp_num++)
		ntb_transport_link_down(dev->qp[qp_num]);

	if (ndev->mtu < new_mtu) {
		int i;

		for (qp_num = 0; qp_num < num_qps; qp_num++) {
			for (i = 0; (skb = ntb_transport_rx_remove(dev->qp[qp_num], &len)); i++)
				kfree(skb);

			for (; i; i--) {
				skb = netdev_alloc_skb(ndev, ndev->mtu + ETH_HLEN + NET_IP_ALIGN);
				if (!skb) {
					rc = -ENOMEM;
					goto err;
				}

				skb_reserve(skb, NET_IP_ALIGN);

				rc = ntb_transport_rx_enqueue(dev->qp[qp_num], skb, skb->data, new_mtu + ETH_HLEN);
				if (rc) {
					kfree(skb);
					goto err;
				}
			}
		}
	}

	ndev->mtu = new_mtu;

	for (qp_num = 0; qp_num < num_qps; qp_num++)
		ntb_transport_link_up(dev->qp[qp_num]);

	return 0;

err:
	for (qp_num = 0; qp_num < num_qps; qp_num++)
		while ((skb = ntb_transport_rx_remove(dev->qp[qp_num], &len)))
			kfree(skb);

	pr_err("Error changing MTU, device inoperable\n");
	return rc;
}

static void ntb_netdev_txto_work(struct work_struct *work)
{
	struct ntb_netdev *dev = container_of(work, struct ntb_netdev,
					      txto_work);
	struct net_device *ndev = dev->ndev;

	if (netif_running(ndev)) {
#if 0 
		int rc;

		ntb_netdev_close(ndev);
		rc = ntb_netdev_open(ndev);
		if (rc)
			pr_err("%s: Open failed\n", __func__);
#else
		netif_tx_wake_all_queues(ndev);
#endif
	}
}

static void ntb_netdev_tx_timeout(struct net_device *ndev)
{
	struct ntb_netdev *dev = netdev_priv(ndev);

	schedule_work(&dev->txto_work);
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
	int rc, i;

	pr_info("%s: Probe\n", KBUILD_MODNAME);

	netdev = alloc_etherdev_mq(sizeof(struct ntb_netdev), num_qps);
	if (!netdev)
		return -ENOMEM;

	dev = netdev_priv(netdev);
	dev->ndev = netdev;
	netdev->features = NETIF_F_HIGHDMA;	//FIXME - check this against the flags returned by the ntb dev???
	//FIXME - there is bound to be more flags supported.  NETIF_F_SG comes to mind, prolly NETIF_F_FRAGLIST, NETIF_F_NO_CSUM, 

	netdev->hw_features = netdev->features;
	netdev->watchdog_timeo = msecs_to_jiffies(NTB_TX_TIMEOUT_MS);
	INIT_WORK(&dev->txto_work, ntb_netdev_txto_work);

	random_ether_addr(netdev->perm_addr);
	memcpy(netdev->dev_addr, netdev->perm_addr, netdev->addr_len);

	netdev->netdev_ops = &ntb_netdev_ops;
	SET_ETHTOOL_OPS(netdev, &ntb_ethtool_ops);

	dev->qp = kcalloc(sizeof(struct ntb_transport_qp *), num_qps, GFP_KERNEL);
	if (!dev->qp) {
		pr_err("Error allocating memory\n");
		rc = -ENOMEM;
		goto err;
	}

	for (i = 0; i < num_qps; i++) {
		dev->qp[i] = ntb_transport_create_queue(ntb_netdev_rx_handler,
						     ntb_netdev_tx_handler,
						     ntb_netdev_event_handler);
		if (!dev->qp[i]) {
			pr_err("Error creating qp %d\n", i);
			rc = -EIO;
			goto err1;
		}

		//FIXME - magical MTU that has better perf
		//FIXME - should be the min size of all the qp mtus
		netdev->mtu = ntb_transport_max_size(dev->qp[i]) - ETH_HLEN;
	}

	rc = register_netdev(netdev);
	if (rc)
		goto err1;

	pr_info("%s: %s created\n", KBUILD_MODNAME, netdev->name);
	return 0;

err1:
	for (i--; i >= 0 && dev->qp[i]; i--)
		ntb_transport_free_queue(dev->qp[i]);
	kfree(dev->qp);
err:
	free_netdev(netdev);
	return rc;
}
module_init(ntb_netdev_init_module);

static void __exit ntb_netdev_exit_module(void)
{
	struct ntb_netdev *dev = netdev_priv(netdev);
	int i;

	unregister_netdev(netdev);

	for (i = 0; i < num_qps; i++)
		ntb_transport_free_queue(dev->qp[i]);

	kfree(dev->qp);
	free_netdev(netdev);

	pr_info("%s: Driver removed\n", KBUILD_MODNAME);
}
module_exit(ntb_netdev_exit_module);
