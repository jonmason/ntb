/*
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 *   redistributing this file, you may do so under either license.
 *
 *   GPL LICENSE SUMMARY
 *
 *   Copyright(c) 2010,2011 Intel Corporation. All rights reserved.
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
 *   Contact Information:
 *   Intel Corporation
 *
 *   BSD LICENSE
 *
 *   Copyright(c) 2010,2011 Intel Corporation. All rights reserved.
 *   All rights reserved.
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
 *
 * Intel PCIe NTB Linux driver
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include "hw.h"
#include "ntb_dev.h"

MODULE_VERSION("0.1");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");

static struct list_head ntb_dev_list = LIST_HEAD_INIT(ntb_dev_list);

static struct pci_device_id ntb_pci_tbl[] = {
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_NTB_B2B_JSF) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_NTB_B2B_SNB) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_NTB_BWD) },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, ntb_pci_tbl);

/**
 * ntb_register_transport() - Allows registration of transports to HID
 * @transport:		cookie to identify the transport
 *
 * This function allows a transport to reserve the hardware driver for
 * NTB usage. For the time being we just pick the hardware driver off
 * of the global list as we aren't expecting additional NTBs in the system,
 * but that could be a possibility in the future.
 */
struct ntb_device * ntb_register_transport(void *transport)
{
	struct ntb_device *ndev;

	list_for_each_entry(ndev, &ntb_dev_list, list) {
		if (!ndev->transport) {
			ndev->transport = transport;
			return ndev;
		}
	}

	pr_warn("NTB: Unable to register transport to device\n");

	return NULL;
}
EXPORT_SYMBOL(ntb_register_transport);

/**
 * ntb_unregister_transport() - Allows unregistering of the transport
 * @ndev - instance to ntb_device
 * @transport - the transport handle to unregister
 *
 * This function unregisters the transport from the HID and perform any
 * necessary cleanups.
 */
int ntb_unregister_transport(struct ntb_device *ndev, void *transport)
{
	if (ndev->transport == transport) {
		int i;

		ndev->transport = NULL;
		for (i = 0; i < ndev->limits.max_sdbs; i++)
			ndev->db_callbacks[i] = NULL;

		ndev->event_cb = NULL;
	} else
		return -EINVAL;

	return 0;
}
EXPORT_SYMBOL(ntb_unregister_transport);

/**
 * ntb_register_event_callback() - register event callback
 * @ndev: pointer to ntb_device instance
 * @func: callback function to register
 *
 * This function registers a callback for any HID events such as link up/down,
 * power management notices and etc.
 */
int ntb_register_event_callback(struct ntb_device *ndev, event_cb_func func)
{
	if (!ndev->event_cb)
		ndev->event_cb = func;
	else
		return -EBUSY;

	return 0;
}
EXPORT_SYMBOL(ntb_register_event_callback);

/**
 * ntb_unregister_event_callback() - unregisters the event callback
 * @ndev: pointer to ntb_device instance
 *
 * This function unregisters the existing callback from transport
 */
void ntb_unregister_event_callback(struct ntb_device *ndev)
{
	ndev->event_cb = NULL;
}
EXPORT_SYMBOL(ntb_unregister_event_callback);

/**
 * ntb_register_db_callback() - register a callback for doorbell interrupt
 * @ndev: pointer to ntb_device instance
 * @idx: doorbell index to register callback, 0 based
 * @func: callback function to register
 *
 * This function registers a callback function for the doorbell interrupt
 * on the primary side. The function will unmask the doorbell as well to
 * allow interrupt.
 */
int ntb_register_db_callback(struct ntb_device *ndev,
			     unsigned int idx,
			     db_cb_func func)
{
	if (!ndev->db_callbacks[idx]) {
		unsigned long mask;

		ndev->db_callbacks[idx] = func;
		/* unmask interrupt */
		mask = readw(ndev->reg_base + ndev->reg_ofs.pdb_mask_ofs);
		clear_bit(idx, &mask);
		writew(mask, ndev->reg_base + ndev->reg_ofs.pdb_mask_ofs);
		return 0;
	} else {
		dev_warn(&ndev->pdev->dev,
			 "Doorbell callback already registered.\n");
		return -EBUSY;
	}
}
EXPORT_SYMBOL(ntb_register_db_callback);

/**
 * ntb_unregister_db_callback() - unregister a callback for doorbell interrupt
 * @ndev: pointer to ntb_device instance
 * @idx: doorbell index to register callback, 0 based
 *
 * This function unregisters a callback function for the doorbell interrupt
 * on the primary side. The function will also mask the said doorbell.
 */
void ntb_unregister_db_callback(struct ntb_device *ndev, unsigned int idx)
{
	if (ndev->db_callbacks[idx]) {
		unsigned long mask;

		ndev->db_callbacks[idx] = NULL;
		mask = readw(ndev->reg_base + ndev->reg_ofs.pdb_mask_ofs);
		set_bit(idx, &mask);
		writew(mask, ndev->reg_base + ndev->reg_ofs.pdb_mask_ofs);
	}
}
EXPORT_SYMBOL(ntb_unregister_db_callback);

/**
 * ntb_get_max_spads() - get the total scratch regs usable
 * @ndev: pointer to ntb_device instance
 *
 * This function returns the max 32bit scratchpad registers usable by the
 * upper layer.
 */
int ntb_get_max_spads(struct ntb_device *ndev)
{
	return ndev->limits.max_compat_spads;
}
EXPORT_SYMBOL(ntb_get_max_spads);

/**
 * ntb_write_spad() - write to the secondary scratchpad register
 * @ndev: pointer to ntb_device instance
 * @idx: index to the scratchpad register, 0 based
 * @val: the data value to put into the register
 *
 * This function allows writing of a 32bit value to the indexed scratchpad
 * register. The register resides on the secondary (external) side.
 */
int ntb_write_spad(struct ntb_device *ndev, unsigned int idx, u32 val)
{
	if (idx >= ndev->limits.max_compat_spads)
		return -EINVAL;

	writel(val, ndev->reg_base + ndev->reg_ofs.spad_write_ofs + idx * 4);
	return 0;
}
EXPORT_SYMBOL(ntb_write_spad);

/**
 * ntb_read_spad() - read from the primary scratchpad register
 * @ndev: pointer to ntb_device instance
 * @idx: index to scratchpad register, 0 based
 * @val: pointer to 32bit integer for storing the register value
 *
 * This function allows reading of the 32bit scratchpad register on
 * the primary (internal) side.
 */
int ntb_read_spad(struct ntb_device *ndev, unsigned int idx, u32 *val)
{
	if (idx >= ndev->limits.max_compat_spads)
		return -EINVAL;

	*val = readl(ndev->reg_base + ndev->reg_ofs.spad_read_ofs + idx * 4);

	return 0;
}
EXPORT_SYMBOL(ntb_read_spad);

/**
 * ntb_get_pbar_vbase() - get virtual addr for the NTB BAR
 * @ndev: pointer to ntb_device instance
 * @bar: BAR index
 *
 * This function provides the ioremapped virtual address for the PCI BAR
 * for the NTB device for BAR 2/3 or BAR 4/5. The BAR index is provided
 * as a #define.
 */
void * ntb_get_pbar_vbase(struct ntb_device *ndev, unsigned int bar)
{
	switch(bar) {
	case NTB_BAR_23:
		return ndev->pbar23;
	case NTB_BAR_45:
		return ndev->pbar45;
	default:
		return NULL;
	}
}
EXPORT_SYMBOL(ntb_get_pbar_vbase);

/**
 * ntb_get_pbar_size() - return size of NTB BAR
 * @ndev: pointer to ntb_device instance
 * @bar: BAR index
 *
 * This function provides the size of the NTB PCI BAR 2/3 or 3/4. The BAR
 * index is provided as a #define
 */
resource_size_t ntb_get_pbar_size(struct ntb_device *ndev, unsigned int bar)
{
	switch(bar) {
	case NTB_BAR_23:
		return ndev->pbar23_sz;
	case NTB_BAR_45:
		return ndev->pbar45_sz;
	default:
		return 0;
	}
}
EXPORT_SYMBOL(ntb_get_pbar_size);

/**
 * ntb_set_satr() - set secondary address translation register
 * @ndev: pointer to ntb_device instance
 * @bar: BAR index
 * @addr: translation address register value
 *
 * This function allows writing to the secondary address translation register
 * in order for the remote end to access the local DMA mapped memory.
 */
void ntb_set_satr(struct ntb_device *ndev, unsigned int bar, u64 addr)
{
	if (bar == NTB_BAR_23)
		writeq(addr, ndev->reg_base + ndev->reg_ofs.sbar2_xlat_ofs);
	else if (bar == NTB_BAR_45)
		writeq(addr, ndev->reg_base + ndev->reg_ofs.sbar4_xlat_ofs);
}
EXPORT_SYMBOL(ntb_set_satr);

/**
 * ntb_ring_sdb() - Set the doorbell on the secondary/external side
 * @ndev: pointer to ntb_device instance
 * @db: doorbell index, 0 based
 *
 * This function allows triggering of a doorbell on the secondary/external
 * side that will initiate an interrupt on the remote host
 */
int ntb_ring_sdb(struct ntb_device *ndev, unsigned int idx)
{
	if (idx >= ndev->limits.max_sdbs)
		return -EINVAL;

	writew(1 << idx, ndev->reg_base + ndev->reg_ofs.sdb_ofs);

	return 0;
}
EXPORT_SYMBOL(ntb_ring_sdb);

static void ntb_send_heartbeat(unsigned long data)
{
	struct ntb_device *ndev = (struct ntb_device *)data;
	unsigned long ts = jiffies;

	ntb_ring_sdb(ndev, NTB_DB_HID_HEARTBEAT);
	mod_timer(&ndev->hb_send_timer, ts + NTB_HB_SEND_TIMEOUT);
}

static void ntb_handle_heartbeat(unsigned long data)
{
	struct ntb_device *ndev = (struct ntb_device *)data;
	unsigned long flags;
	unsigned long ts = jiffies;
	unsigned int link;

	link = ndev->link_status;

	spin_lock_irqsave(&ndev->hb_lock, flags);

	if (ndev->last_ts && (ts > ndev->last_ts + NTB_HB_TIMEOUT)) {
		if (ndev->link_status != NTB_LINK_DOWN)
			ndev->link_status = NTB_LINK_SELF_UP;
	} else if (ndev->link_status != NTB_LINK_PEER_UP)
		ndev->link_status = NTB_LINK_PEER_UP;

	mod_timer(&ndev->hb_timer, ts + NTB_HB_TIMEOUT);
	ndev->last_ts = ts;

	spin_unlock_irqrestore(&ndev->hb_lock, flags);

	/* notify the upper layer if we have an event change */
	if (ndev->event_cb) {
		if (link != ndev->link_status) {
			if (ndev->link_status == NTB_LINK_PEER_UP) {
				ndev->event_cb(ndev->transport,
					       NTB_EVENT_LINK_UP);
			} else if (ndev->link_status == NTB_LINK_SELF_UP) {
				ndev->event_cb(ndev->transport,
					       NTB_EVENT_LINK_DOWN);
			}
		}
	}
}

static void ntb_handle_dbs(unsigned long data)
{
	struct ntb_device *ndev = (struct ntb_device *)data;
	unsigned int db;

	for (db = NTB_DB_LAST; db < ndev->limits.max_sdbs; db++) {
		if (test_bit(db, &ndev->db_status)) {
			if (ndev->db_callbacks[db]) {
				clear_bit(db, &ndev->db_status);
				ndev->db_callbacks[db](ndev->transport, db);
			} else {
				dev_err(&ndev->pdev->dev,
					"Interrupt w/ no registered handler!");
			}
		}
	}
}

static bool is_j_b2b(struct pci_dev *pdev)
{
	switch (pdev->device) {
	case PCI_DEVICE_ID_INTEL_NTB_B2B_JSF:
	case PCI_DEVICE_ID_INTEL_NTB_B2B_SNB:
		return true;
	default:
		return false;
	};
}

static bool is_bwd(struct pci_dev *pdev)
{
	switch (pdev->device) {
	case PCI_DEVICE_ID_INTEL_NTB_BWD:
		return true;
	default:
		return false;
	};
}

static int ntb_get_link_status(struct ntb_device *ndev)
{
	u16 link_status;

	if (ndev->link_status == NTB_LINK_UNKNOWN) {
		pci_read_config_word(ndev->pdev,
				     ndev->reg_ofs.lnk_stat_ofs,
				     &link_status);

		ndev->link_status =
			(link_status & NTB_LINK_STATUS_ACTIVE) ?
			NTB_LINK_SELF_UP : NTB_LINK_DOWN;
	}

	return ndev->link_status;
}

static void ntb_snb_b2b_setup(struct ntb_device *ndev)
{
	ndev->pbar23_sz = pci_resource_len(ndev->pdev, NTB_BAR_23 * 2);
	ndev->pbar45_sz = pci_resource_len(ndev->pdev, NTB_BAR_45 * 2);

	ndev->reg_ofs.pdb_ofs = SNB_PDOORBELL_OFFSET;
	ndev->reg_ofs.pdb_mask_ofs = SNB_PDBMSK_OFFSET;
	ndev->reg_ofs.sbar2_xlat_ofs = SNB_SBAR2XLAT_OFFSET;
	ndev->reg_ofs.sbar4_xlat_ofs = SNB_SBAR4XLAT_OFFSET;
	ndev->reg_ofs.spad_read_ofs = SNB_SPAD_OFFSET;
	ndev->reg_ofs.lnk_cntl_ofs = SNB_NTBCNTL_OFFSET;
	ndev->reg_ofs.lnk_stat_ofs = SNB_LINK_STATUS_OFFSET;

	ndev->reg_ofs.sdb_ofs = SNB_B2B_DOORBELL_OFFSET;
	ndev->reg_ofs.spad_write_ofs = SNB_B2B_SPAD_OFFSET;

	ndev->reg_ofs.msix_msgctrl_ofs = SNB_MSIXMSGCTRL_OFFSET;

	ndev->limits.max_compat_spads = SNB_MAX_COMPAT_SPADS;
	ndev->limits.max_spads = SNB_MAX_SPADS;
	ndev->limits.max_sdbs = SNB_MAX_SDBS;
	ndev->limits.msix_cnt = SNB_MSIX_CNT;

	ndev->dev_type = NTB_DEV_B2B;
}

static void ntb_bwd_setup(struct ntb_device *ndev)
{
	ndev->pbar23_sz = pci_resource_len(ndev->pdev, NTB_BAR_23 * 2);
	ndev->pbar45_sz = pci_resource_len(ndev->pdev, NTB_BAR_45 * 2);

	ndev->reg_ofs.pdb_ofs = BWD_PDOORBELL_OFFSET;
	ndev->reg_ofs.pdb_mask_ofs = BWD_PDBMSK_OFFSET;
	ndev->reg_ofs.sbar2_xlat_ofs = BWD_SBAR2XLAT_OFFSET;
	ndev->reg_ofs.sbar4_xlat_ofs = BWD_SBAR4XLAT_OFFSET;
	ndev->reg_ofs.spad_read_ofs = BWD_SPAD_OFFSET;
	ndev->reg_ofs.lnk_cntl_ofs = BWD_NTBCNTL_OFFSET;
	ndev->reg_ofs.lnk_stat_ofs = BWD_LINK_STATUS_OFFSET;

	ndev->reg_ofs.sdb_ofs = BWD_B2B_DOORBELL_OFFSET;
	ndev->reg_ofs.spad_write_ofs = BWD_B2B_SPAD_OFFSET;

	ndev->reg_ofs.msix_msgctrl_ofs = BWD_MSIXMSGCTRL_OFFSET;

	ndev->limits.max_compat_spads = BWD_MAX_COMPAT_SPADS;
	ndev->limits.max_spads = BWD_MAX_SPADS;
	ndev->limits.max_sdbs = BWD_MAX_SDBS;
	ndev->limits.msix_cnt = BWD_MSIX_CNT;

	ndev->dev_type = NTB_DEV_B2B;
}

static int ntb_device_setup(struct ntb_device *ndev)
{
	u32 ntb_cntl;

	if (is_j_b2b(ndev->pdev))
		ntb_snb_b2b_setup(ndev);
	else if (is_bwd(ndev->pdev))
		ntb_bwd_setup(ndev);
	return
		-ENODEV;

	/* clearing the secondary doorbells */
	ntb_ring_sdb(ndev, 0);

	ntb_get_link_status(ndev);

	/* lets bring the NTB link up */
	ntb_cntl = readl(ndev->reg_base + ndev->reg_ofs.lnk_cntl_ofs);
	ntb_cntl &= ~NTB_LINK_DISABLE;
	writel(ntb_cntl, ndev->reg_base + ndev->reg_ofs.lnk_cntl_ofs);
	ndev->link_status = NTB_LINK_SELF_UP;

	/* send heartbeat to remote */
	init_timer(&ndev->hb_send_timer);
	ndev->hb_send_timer.function = ntb_send_heartbeat;
	ndev->hb_send_timer.data = (unsigned long)ndev;
	ntb_send_heartbeat((unsigned long)ndev);
	mod_timer(&ndev->hb_send_timer, jiffies + NTB_HB_SEND_TIMEOUT);

	/* setup heartbeat timer */
	init_timer(&ndev->hb_timer);
	ndev->hb_timer.function = ntb_handle_heartbeat;
	ndev->hb_timer.data = (unsigned long)ndev;
	mod_timer(&ndev->hb_timer, jiffies + NTB_HB_TIMEOUT);

	return 0;
}

static irqreturn_t ntb_interrupt(int irq, void *data)
{
	struct ntb_device *ndev = data;
	u16 db;

	/* read doorbell register */
	db = readw(ndev->reg_base + ndev->reg_ofs.pdb_ofs);

	if (db) {
		ndev->db_status |= db;
		tasklet_schedule(&ndev->db_tasklet);
	} else
		return IRQ_NONE;

	/* clear interrupt */
	writew(db, ndev->reg_base + ndev->reg_ofs.pdb_ofs);

	if (is_j_b2b(ndev->pdev)) {
		if (ndev->db_status & SNB_DB_HW_LINK)
			clear_bit(SNB_DB_HW_LINK, &ndev->db_status);
	}

	return IRQ_HANDLED;
}

static int ntb_setup_interrupts(struct ntb_device *ndev)
{
	struct pci_dev *pdev = ndev->pdev;
	struct device *dev = &pdev->dev;
	struct msix_entry *msix;
	int msix_entries = 0;
	int i, err, j;

	pci_read_config_word(pdev,
			     ndev->reg_ofs.msix_msgctrl_ofs,
			     &ndev->msixmsgctrl);
	msix_entries = ndev->msixmsgctrl & NTB_MSIXMSGCTRL_ENTRIES_MASK;
	msix_entries++;

	if (msix_entries > ndev->limits.msix_cnt)
		return -EINVAL;

	for (i = 0; i < msix_entries; i++)
		ndev->msix_entries[i].entry = i;

	err = pci_enable_msix(pdev, ndev->msix_entries, ndev->limits.msix_cnt);
	if (err < 0)
		goto msi;
	if (err > 0)
		goto msix_single_vector;

	if (msix_entries == 0) {
		pci_disable_msix(pdev);
		goto msi;
	}

	for (i = 0; i < msix_entries; i++) {
		msix = &ndev->msix_entries[i];
		err = devm_request_irq(dev, msix->vector,
				       ntb_interrupt, 0,
				       "ntb-msix", ndev);
		if (err) {
			for (j = 0; j < i; j++) {
				msix = &ndev->msix_entries[j];
				devm_free_irq(dev, msix->vector, ndev);
			}
		}
		goto msix_single_vector;
	}
	goto done;

msix_single_vector:
	msix = &ndev->msix_entries[0];
	msix->entry = 0;
	err = pci_enable_msix(pdev, ndev->msix_entries, 1);
	if (err)
		goto msi;

	err = devm_request_irq(dev, msix->vector, ntb_interrupt, 0,
			       "ntb-msix", ndev);
	if (err) {
		pci_disable_msix(pdev);
		goto msi;
	}
	goto done;

msi:
	err = pci_enable_msi(pdev);
	if (err)
		goto intx;

	err = devm_request_irq(dev, pdev->irq, ntb_interrupt, 0,
			       "ntb-msi", ndev);
	if (err) {
		pci_disable_msi(pdev);
		goto intx;
	}
	goto done;

intx:
	err = devm_request_irq(dev, pdev->irq, ntb_interrupt,
			       IRQF_SHARED, "ntb-intx", ndev);
	if (err)
		goto err_no_irq;

done:
	tasklet_init(&ndev->db_tasklet, ntb_handle_dbs, (unsigned long)ndev);

	/* only turn on interrupts we need */
	writew(~NTB_DB_HID_HEARTBEAT,
	       ndev->reg_base + ndev->reg_ofs.pdb_mask_ofs);

	return 0;

err_no_irq:
	/* mask interrrupt mask register here */
	writew(0xffff, ndev->reg_base + ndev->reg_ofs.pdb_mask_ofs);
	dev_err(dev, "no usable interrupts\n");
	return -ENODEV;
}

static int __devinit ntb_pci_probe(struct pci_dev *pdev,
				   const struct pci_device_id *id)
{
	void __iomem * const *iomap;
	struct ntb_device *ndev;
	int err;

	err = pcim_enable_device(pdev);
	if (err)
		return err;

	err = pcim_iomap_regions(pdev, NTB_BAR_MASK, DRV_NAME);
	if (err)
		return err;

	iomap = pcim_iomap_table(pdev);
	if (!iomap)
		return -ENOMEM;

	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;

	ndev->reg_base = iomap[0];
	ndev->pbar23 = iomap[1];
	ndev->pbar45 = iomap[2];

	spin_lock_init(&ndev->hb_lock);
	INIT_LIST_HEAD(&ndev->list);

	ndev->pdev = pdev;

	err = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
	if (err) {
		err = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
		if (err)
			return err;
	}

	err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64));
	if (err) {
		err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
		if (err)
			return err;
	}

	err = ntb_device_setup(ndev);
	if (err)
		return err;

	ndev->msix_entries = devm_kzalloc(&pdev->dev,
					  sizeof(struct msix_entry) *
					  ndev->limits.msix_cnt,
					  GFP_KERNEL);
	if (!ndev->msix_entries)
		return -ENOMEM;

	pci_set_master(pdev);
	pci_set_drvdata(pdev, ndev);

	err = ntb_setup_interrupts(ndev);
	if (err)
		return err;

	list_add(&ndev->list, &ntb_dev_list);

	return 0;
}

static void __devexit ntb_pci_remove(struct pci_dev *pdev)
{
	struct ntb_device *ndev = pci_get_drvdata(pdev);
	del_timer_sync(&ndev->hb_send_timer);
	del_timer_sync(&ndev->hb_timer);
}

static struct pci_driver ntb_pci_driver = {
	.name		= DRV_NAME,
	.id_table	= ntb_pci_tbl,
	.probe		= ntb_pci_probe,
	.remove		= __devexit_p(ntb_pci_remove),
};

static int __init ntb_init_module(void)
{
	pr_info("%s: Intel(R) PCIe Non-Transparent Bridge Driver\n", DRV_NAME);

	return pci_register_driver(&ntb_pci_driver);
}
module_init(ntb_init_module);

static void __exit ntb_exit_module(void)
{
	pci_unregister_driver(&ntb_pci_driver);
}
module_exit(ntb_exit_module);

