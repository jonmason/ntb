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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include "ntb_hw.h"
#include "ntb_dev.h"

#define NTB_NAME	"Intel(R) PCIe Non-Transparent Bridge Driver"
#define NTB_VER		"0.2"

MODULE_DESCRIPTION(NTB_NAME);
MODULE_VERSION(NTB_VER);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");

#define MW_TO_BAR(bar)	(bar * 2 + 2)

static struct pci_device_id ntb_pci_tbl[] = {
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_NTB_B2B_JSF) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_NTB_B2B_SNB) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_NTB_B2B_BWD) },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, ntb_pci_tbl);

struct ntb_device *ntbdev;//FIXME - hack

static int pci_read_config_qword(struct pci_dev *dev, int where, u64 *val)
{
	int rc;
	u32 val32;

	rc = pci_read_config_dword(dev, where, &val32);
	if (rc)
		return rc;

	*val = ((u64) val32) << 32;
	rc = pci_read_config_dword(dev, where + 0x4, &val32);
	if (rc)
		return rc;
	*val |= val32;

	return 0;
}

static void ntb_debug_dump(struct ntb_device *ndev)
{
	u64 status64;
	u16 status16;
	int rc;

	dev_info(&ndev->pdev->dev, "%s %s\n", (ndev->conn_type == NTB_CONN_B2B) ? "NTB B2B" : "NTB-RP", (ndev->dev_type == NTB_DEV_USD) ? "USD/DSP" : "DSD/USP");

	rc = pci_read_config_word(ndev->pdev, ndev->reg_ofs.lnk_stat, &status16);
	if (rc)
		return;

	dev_info(&ndev->pdev->dev, "Link %s, Link Width %d, Link Speed %d\n", !!(status16 & NTB_LINK_STATUS_ACTIVE) ? "Up" : "Down", (status16 & 0x3f0) >> 4, (status16 & 0xf));
	dev_info(&ndev->pdev->dev, "Local BAR0 %p, Reg BAR0 %lx, Remote BAR0 %lx, B2BBAR0XLAT %lx\n", ndev->reg_base, readq(ndev->reg_base + 0x10), readq(ndev->reg_base + 0x40) & ~(0x3fff), readq(ndev->reg_base + 0x144));

	rc = pci_read_config_qword(ndev->pdev, 0x10, &status64);
	if (rc)
		return;

	dev_info(&ndev->pdev->dev, "PB01BASE %llx\n", status64);

	if (ndev->dev_type == NTB_DEV_USD) {
		u32 val = 0;
		u64 lbase, rbase;

		ntb_read_spad(ndev, 0, &val);
		lbase = ((u64) val) << 32;
		ntb_read_spad(ndev, 1, &val);
		lbase |= val;

		ntb_read_spad(ndev, 2, &val);
		rbase = ((u64) val) << 32;
		ntb_read_spad(ndev, 3, &val);
		rbase |= val;

		dev_info(&ndev->pdev->dev, "local base %llx remote base %llx\n", lbase, rbase);
	} else {
		u32 val = 0;
		u64 lbase, rbase;

		ntb_read_spad(ndev, 0, &val);
		rbase = ((u64) val) << 32;
		ntb_read_spad(ndev, 1, &val);
		rbase |= val;

		ntb_read_spad(ndev, 2, &val);
		lbase = ((u64) val) << 32;
		ntb_read_spad(ndev, 3, &val);
		lbase |= val;

		dev_info(&ndev->pdev->dev, "local base %llx remote base %llx\n", lbase, rbase);
	}

	dev_info(&ndev->pdev->dev, "Upstream Memory Misses %d\n", readw(ndev->reg_base + 0x70));

	rc = pci_read_config_qword(ndev->pdev, 0x18, &status64);
	if (rc)
		return;

	dev_info(&ndev->pdev->dev, "PBAR2BASE %llx MMIO addr %p\n", status64, ndev->mw[0].vbase);
	dev_info(&ndev->pdev->dev, "PBAR2XLAT %lx PBAR4XLAT %lx\n", readq(ndev->reg_base + 0x10), readq(ndev->reg_base + 0x18));
	dev_info(&ndev->pdev->dev, "SBAR2XLAT %lx SBAR4XLAT %lx\n", readq(ndev->reg_base + 0x30), readq(ndev->reg_base + 0x38));
	dev_info(&ndev->pdev->dev, "SBAR0BASE %lx SBAR2BASE %lx SBAR4BASE %lx\n", readq(ndev->reg_base + 0x40), readq(ndev->reg_base + 0x48), readq(ndev->reg_base + 0x50));
	dev_info(&ndev->pdev->dev, "B2BBAR0XLAT %lx\n", readq(ndev->reg_base + 0x144));
}

/**
 * ntb_query_db_bits() - return the number of doorbell bits
 * @ndev: pointer to ntb_device instance
 *
 * The number of bits in the doorbell can vary depending on the platform
 *
 * RETURNS: the number of doorbell bits being used (16 or 64)
 */
unsigned int ntb_query_db_bits(struct ntb_device *ndev)
{
	return ndev->limits.max_db_bits;
}
EXPORT_SYMBOL(ntb_query_db_bits);

/**
 * ntb_register_event_callback() - register event callback
 * @ndev: pointer to ntb_device instance
 * @func: callback function to register
 *
 * This function registers a callback for any HW driver events such as link up/down,
 * power management notices and etc.
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int ntb_register_event_callback(struct ntb_device *ndev, event_cb_func func)
{
	if (ndev->event_cb)
		return -EBUSY;

	ndev->event_cb = func;

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
	//FIXME - ensure events are flushed
	ndev->event_cb = NULL;
}
EXPORT_SYMBOL(ntb_unregister_event_callback);

/**
 * ntb_register_db_callback() - register a callback for doorbell interrupt
 * @ndev: pointer to ntb_device instance
 * @idx: doorbell index to register callback, zero based
 * @func: callback function to register
 *
 * This function registers a callback function for the doorbell interrupt
 * on the primary side. The function will unmask the doorbell as well to
 * allow interrupt.
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int ntb_register_db_callback(struct ntb_device *ndev,
			     unsigned int idx,
			     db_cb_func func)
{
	unsigned long mask;

	//FIXME - don't let them specify index???
	if (idx >= ndev->limits.max_db_bits || ndev->db_cb[idx].callback) {
		dev_warn(&ndev->pdev->dev, "Invalid Index.\n");
		return -EBUSY;
	}

	ndev->db_cb[idx].callback = func;

	/* unmask interrupt */
	mask = readw(ndev->reg_base + ndev->reg_ofs.pdb_mask);
	clear_bit(idx, &mask);
	writew(mask, ndev->reg_base + ndev->reg_ofs.pdb_mask);

	return 0;
}
EXPORT_SYMBOL(ntb_register_db_callback);

/**
 * ntb_unregister_db_callback() - unregister a callback for doorbell interrupt
 * @ndev: pointer to ntb_device instance
 * @idx: doorbell index to register callback, zero based
 *
 * This function unregisters a callback function for the doorbell interrupt
 * on the primary side. The function will also mask the said doorbell.
 */
void ntb_unregister_db_callback(struct ntb_device *ndev, unsigned int idx)
{
	unsigned long mask;

	if (idx >= ndev->limits.max_db_bits || !ndev->db_cb[idx].callback)
		return;

	//FIXME - ensure the dbs have been flushed
	mask = readw(ndev->reg_base + ndev->reg_ofs.pdb_mask);
	set_bit(idx, &mask);
	writew(mask, ndev->reg_base + ndev->reg_ofs.pdb_mask);

	ndev->db_cb[idx].callback = NULL;
}
EXPORT_SYMBOL(ntb_unregister_db_callback);

/**
 * ntb_register_transport() - Register NTB transport with NTB HW driver
 * @transport: transport identifier
 *
 * This function allows a transport to reserve the hardware driver for
 * NTB usage.
 *
 * RETURNS: pointer to ntb_device, NULL on error.
 */
struct ntb_device *ntb_register_transport(void *transport)
{
	struct ntb_device *ndev = ntbdev;

	if (ndev->ntb_transport)
		return NULL;

	ndev->ntb_transport = transport;
	return ndev;
}
EXPORT_SYMBOL(ntb_register_transport);

/**
 * ntb_unregister_transport() - Unregister the transport with the NTB HW driver
 * @ndev - ntb_device of the transport to be freed
 *
 * This function unregisters the transport from the HW driver and performs any
 * necessary cleanups.
 */
void ntb_unregister_transport(struct ntb_device *ndev)
{
	int i;

	if (!ndev->ntb_transport)
		return;

	for (i = 0; i < ndev->limits.max_db_bits; i++)
		ntb_unregister_db_callback(ndev, i);

	ntb_unregister_event_callback(ndev);
	ndev->ntb_transport = NULL;
}
EXPORT_SYMBOL(ntb_unregister_transport);

/**
 * ntb_get_max_spads() - get the total scratch regs usable
 * @ndev: pointer to ntb_device instance
 *
 * This function returns the max 32bit scratchpad registers usable by the
 * upper layer.
 *
 * RETURNS: total number of scratch pad registers available
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
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int ntb_write_spad(struct ntb_device *ndev, unsigned int idx, u32 val)
{
	if (idx >= ndev->limits.max_compat_spads)
		return -EINVAL;

	dev_info(&ndev->pdev->dev, "Writing %x to scratch pad index %d\n", val, idx);
	writel(val, ndev->reg_base + ndev->reg_ofs.spad_write + idx * 4);

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
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int ntb_read_spad(struct ntb_device *ndev, unsigned int idx, u32 *val)
{
	if (idx >= ndev->limits.max_compat_spads)
		return -EINVAL;

	*val = readl(ndev->reg_base + ndev->reg_ofs.spad_read + idx * 4);
	dev_info(&ndev->pdev->dev, "Reading %x from scratch pad index %d\n", *val, idx);

	return 0;
}
EXPORT_SYMBOL(ntb_read_spad);

/**
 * ntb_get_mw_vbase() - get virtual addr for the NTB memory window
 * @ndev: pointer to ntb_device instance
 * @mw: memory window number
 *
 * This function provides the base virtual address of the memory window specified.
 *
 * RETURNS: pointer to virtual address, or NULL on error.
 */
void *ntb_get_mw_vbase(struct ntb_device *ndev, unsigned int mw)
{
	if (mw > NTB_NUM_MW)
		return NULL;

	return ndev->mw[mw].vbase;
}
EXPORT_SYMBOL(ntb_get_mw_vbase);

/**
 * ntb_get_mw_size() - return size of NTB memory window
 * @ndev: pointer to ntb_device instance
 * @mw: memory window number
 *
 * This function provides the physical size of the memory window specified
 *
 * RETURNS: the size of the memory window or zero on error
 */
resource_size_t ntb_get_mw_size(struct ntb_device *ndev, unsigned int mw)
{
	if (mw > NTB_NUM_MW)
		return 0;

	return ndev->mw[mw].bar_sz;
}
EXPORT_SYMBOL(ntb_get_mw_size);

/**
 * ntb_set_mw_addr - set the memory window address
 * @ndev: pointer to ntb_device instance
 * @mw: memory window number
 * @addr: base address for remote data to be transferred into
 *
 * This function sets the base physical address of the memory window.  This
 * memory address is where data from the remote system will be transfered into.
 */
void ntb_set_mw_addr(struct ntb_device *ndev, unsigned int mw, u64 addr)
{
	if (mw > NTB_NUM_MW)
		return;

	ndev->mw[mw].phys_addr = addr;

	switch (MW_TO_BAR(mw)) {
	case NTB_BAR_23:
		writeq(addr, ndev->reg_base + ndev->reg_ofs.sbar2_xlat);
		break;
	case NTB_BAR_45:
		writeq(addr, ndev->reg_base + ndev->reg_ofs.sbar4_xlat);
		break;
	}
}
EXPORT_SYMBOL(ntb_set_mw_addr);

/**
 * ntb_ring_sdb() - Set the doorbell on the secondary/external side
 * @ndev: pointer to ntb_device instance
 * @db: doorbell(s) to ring
 *
 * This function allows triggering of a doorbell on the secondary/external
 * side that will initiate an interrupt on the remote host
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int ntb_ring_sdb(struct ntb_device *ndev, unsigned int db)
{
	if (db >= 1 << ndev->limits.max_db_bits)
		return -EINVAL;

	dev_info(&ndev->pdev->dev, "%s: ringing doorbell(s) %x\n", __func__, db);

	writew(db, ndev->reg_base + ndev->reg_ofs.sdb);

	return 0;
}
EXPORT_SYMBOL(ntb_ring_sdb);

static void ntb_link_event(struct ntb_device *ndev, int link_state)
{
	if (ndev->link_status == link_state)
		return;

	if (link_state == NTB_LINK_UP) {
		dev_info(&ndev->pdev->dev, "Link Up\n");
		ndev->link_status = NTB_LINK_UP;

		ntb_debug_dump(ndev);
	} else {
		dev_info(&ndev->pdev->dev, "Link Down\n");
		ndev->link_status = NTB_LINK_DOWN;
	}

	/* notify the upper layer if we have an event change */
	if (ndev->event_cb)
		ndev->event_cb(ndev->ntb_transport, link_state);
}

/* BWD doesn't have link status interrupt, so we need to poll on that platform */
static void ntb_handle_heartbeat(struct work_struct *work)
{
	struct ntb_device *ndev = container_of(work, struct ntb_device, hb_timer.work);
	unsigned long ts = jiffies;
	int rc;

	/* Send Heartbeat signal to remote system */
	rc = ntb_ring_sdb(ndev, BWD_DB_HEARTBEAT);
	if (rc) {
		dev_err(&ndev->pdev->dev, "Invalid Index\n");
		return;
	}

	/* Check to see if HB has timed out */
	if (ts > ndev->last_ts + 2 * NTB_HB_TIMEOUT) {
		dev_err(&ndev->pdev->dev, "HB Timeout\n");
		ntb_link_event(ndev, NTB_LINK_DOWN);
	} else
		ntb_link_event(ndev, NTB_LINK_UP);

	schedule_delayed_work(&ndev->hb_timer, NTB_HB_TIMEOUT);
}

static int ntb_link_status(struct ntb_device *ndev)
{
	u16 status;
	int rc;

	rc = pci_read_config_word(ndev->pdev, ndev->reg_ofs.lnk_stat, &status);
	if (rc)
		return rc;

	if (status & NTB_LINK_STATUS_ACTIVE)
		ntb_link_event(ndev, NTB_LINK_UP);
	else
		ntb_link_event(ndev, NTB_LINK_DOWN);

	return 0;
}

static void ntb_snb_b2b_setup(struct ntb_device *ndev)
{
	int i;

	for (i = 0; i < NTB_NUM_MW; i++)
		ndev->mw[i].bar_sz = pci_resource_len(ndev->pdev, MW_TO_BAR(i));

	ndev->reg_ofs.pdb = SNB_PDOORBELL_OFFSET;
	ndev->reg_ofs.pdb_mask = SNB_PDBMSK_OFFSET;
	ndev->reg_ofs.sbar2_xlat = SNB_SBAR2XLAT_OFFSET;
	ndev->reg_ofs.sbar4_xlat = SNB_SBAR4XLAT_OFFSET;
	ndev->reg_ofs.lnk_cntl = SNB_NTBCNTL_OFFSET;
	ndev->reg_ofs.lnk_stat = SNB_LINK_STATUS_OFFSET;

	if (ndev->conn_type == NTB_CONN_B2B) {
		ndev->reg_ofs.sdb = SNB_B2B_DOORBELL_OFFSET;
		ndev->reg_ofs.spad_write = SNB_B2B_SPAD_OFFSET;
		ndev->reg_ofs.spad_read = SNB_B2B_SPAD_OFFSET;
	} else {
		ndev->reg_ofs.sdb = SNB_SDOORBELL_OFFSET; 
		ndev->reg_ofs.spad_write = SNB_SPAD_OFFSET;
		ndev->reg_ofs.spad_read = SNB_SPAD_OFFSET;
	}

	ndev->reg_ofs.msix_msgctrl = SNB_MSIXMSGCTRL_OFFSET;

	ndev->limits.max_compat_spads = SNB_MAX_COMPAT_SPADS;
	ndev->limits.max_spads = SNB_MAX_SPADS;
	/* Reserve the uppermost bit for link interrupt */
	ndev->limits.max_db_bits = SNB_MAX_DB_BITS;
	ndev->limits.msix_cnt = SNB_MSIX_CNT;
}

static void ntb_bwd_setup(struct ntb_device *ndev)
{
	int i;

	for (i = 0; i < NTB_NUM_MW; i++)
		ndev->mw[i].bar_sz = pci_resource_len(ndev->pdev, MW_TO_BAR(i));

	ndev->reg_ofs.pdb = BWD_PDOORBELL_OFFSET;
	ndev->reg_ofs.pdb_mask = BWD_PDBMSK_OFFSET;
	ndev->reg_ofs.sbar2_xlat = BWD_SBAR2XLAT_OFFSET;
	ndev->reg_ofs.sbar4_xlat = BWD_SBAR4XLAT_OFFSET;
	ndev->reg_ofs.lnk_cntl = BWD_NTBCNTL_OFFSET;
	ndev->reg_ofs.lnk_stat = BWD_LINK_STATUS_OFFSET;
	ndev->reg_ofs.spad_read = BWD_SPAD_OFFSET;

	if (ndev->conn_type == NTB_CONN_B2B) {
		ndev->reg_ofs.sdb = BWD_B2B_DOORBELL_OFFSET;
		ndev->reg_ofs.spad_write = BWD_B2B_SPAD_OFFSET;
	} else {
		ndev->reg_ofs.sdb = BWD_PDOORBELL_OFFSET;//FIXME - investigate the lack of sdb
		ndev->reg_ofs.spad_write = BWD_SPAD_OFFSET;
	}

	ndev->reg_ofs.msix_msgctrl = BWD_MSIXMSGCTRL_OFFSET;

	ndev->limits.max_compat_spads = BWD_MAX_COMPAT_SPADS;
	ndev->limits.max_spads = BWD_MAX_SPADS;
	/* Reserve the uppermost bit for link interrupt */
	ndev->limits.max_db_bits = BWD_MAX_DB_BITS;
	ndev->limits.msix_cnt = BWD_MSIX_CNT;

	/* Since bwd doesn't have a link interrupt, setup a heartbeat timer */
	INIT_DELAYED_WORK(&ndev->hb_timer, ntb_handle_heartbeat);
	schedule_delayed_work(&ndev->hb_timer, NTB_HB_TIMEOUT); //FIXME - this might fire before the probe has finished
}

static int ntb_device_setup(struct ntb_device *ndev)
{
	int rc;
	u8 val;

	rc = pci_read_config_byte(ndev->pdev, NTB_PPD_OFFSET, &val);
	if (rc)
		return rc;

	switch (val & NTB_PPD_CONN_TYPE) {
	case NTB_CONN_B2B:
		ndev->conn_type = NTB_CONN_B2B;
		break;
	case NTB_CONN_CLASSIC:
	case NTB_CONN_RP:
	default:
		dev_err(&ndev->pdev->dev, "Only B2B supported at this time\n");
		return -EINVAL;
	}

	if (val & NTB_PPD_DEV_TYPE)
		ndev->dev_type = NTB_DEV_DSD;
	else
		ndev->dev_type = NTB_DEV_USD;

	switch (ndev->pdev->device) {
	case PCI_DEVICE_ID_INTEL_NTB_B2B_JSF:
	case PCI_DEVICE_ID_INTEL_NTB_B2B_SNB:
		ntb_snb_b2b_setup(ndev);
		break;
	case PCI_DEVICE_ID_INTEL_NTB_B2B_BWD:
		ntb_bwd_setup(ndev);
		break;
	default:
		return -ENODEV;
	}

#if 0
	/* Write to spad 0/1 for USD and 2/3 for DSD, to send the BAR address to the remote side */
	if (ndev->dev_type == NTB_DEV_USD) {
		rc = ntb_write_spad(ndev, 0, (u32)((u64) ndev->reg_base >> 32));
		if (rc)
			return rc;

		rc = ntb_write_spad(ndev, 1, (u32)((u64) ndev->reg_base & 0xffffffff));
		if (rc)
			return rc;

		dev_info(&ndev->pdev->dev, "reg base %p spad 0 %x spad 1 %x\n", ndev->reg_base, (u32)((u64) ndev->reg_base >> 32), (u32)((u64) ndev->reg_base & 0xffffffff));
	} else {
		rc = ntb_write_spad(ndev, 2, (u32)((u64) ndev->reg_base >> 32));
		if (rc)
			return rc;

		rc = ntb_write_spad(ndev, 3, (u32)((u64) ndev->reg_base & 0xffffffff)); 
		if (rc)
			return rc;

		dev_info(&ndev->pdev->dev, "reg base %p spad 2 %x spad 3 %x\n", ndev->reg_base, (u32)((u64) ndev->reg_base >> 32), (u32)((u64) ndev->reg_base & 0xffffffff));
	}
#endif
	return 0;
}

static void ntb_device_free(struct ntb_device *ndev)
{
#ifdef BWD
	cancel_delayed_work_sync(&ndev->hb_timer);
#endif
}

//FIXME - find way to sync irq handling between jt/bwd and msix/msi/intx
static irqreturn_t ntb_interrupt(int irq, void *dev)
{
	struct ntb_device *ndev = dev;
	u16 pdb;
	int rc;

	pdb = readw(ndev->reg_base + ndev->reg_ofs.pdb);
	if (!pdb)
		return IRQ_NONE;

	if (pdb & ~SNB_DB_HW_LINK)
		dev_info(&ndev->pdev->dev, "irq %d - pdb = %x\n", irq, pdb);

	if (pdb & BWD_DB_HEARTBEAT)
		ndev->last_ts = jiffies;

	if (pdb & SNB_DB_HW_LINK) {
		rc = ntb_link_status(ndev);
		if (rc)
			dev_err(&ndev->pdev->dev, "Error determining link status\n");
	}

	writew(pdb, ndev->reg_base + ndev->reg_ofs.pdb);

	return IRQ_HANDLED;
}

static irqreturn_t ntb_msix_irq(int irq, void *dev)
{
	struct ntb_device *ndev = dev;
	int rc, i;
#ifdef BWD
	u64 pdb;

	pdb = readq(ndev->reg_base + ndev->reg_ofs.pdb);
#else
	u16 pdb;

	pdb = readw(ndev->reg_base + ndev->reg_ofs.pdb);

	if (pdb & ~SNB_DB_HW_LINK)
		dev_info(&ndev->pdev->dev, "irq %d - pdb = %x sdb %x\n", irq, pdb, readw(ndev->reg_base + ndev->reg_ofs.sdb));
#endif

	for (i = 0; i < ndev->limits.max_db_bits - 1; i++) {
		if (test_bit(i, (const volatile long unsigned int *)&pdb) && ndev->db_cb[i].callback)
				ndev->db_cb[i].callback(ndev->db_cb[i].db_num);
	}
	//FIXME - can this be optimized using ffs or is that unnecessary?
	//FIXME - can use the irq number to determine which bits to clear, otherwise we may be clearing interrupts we don't want to

#ifdef BWD
	if (pdb & BWD_DB_HEARTBEAT)
		ndev->last_ts = jiffies;

	writeq(pdb, ndev->reg_base + ndev->reg_ofs.pdb);
#else
	if (pdb & SNB_DB_HW_LINK) {
		rc = ntb_link_status(ndev);
		if (rc)
			dev_err(&ndev->pdev->dev, "Error determining link status\n");
	}

	writew(pdb, ndev->reg_base + ndev->reg_ofs.pdb);
#endif

	return IRQ_HANDLED;
}

static int ntb_setup_msix(struct ntb_device *ndev)
{
	struct pci_dev *pdev = ndev->pdev;
	struct msix_entry *msix;
	u16 val;
	int msix_entries;
	int rc, i;

	rc = pci_read_config_word(pdev, ndev->reg_ofs.msix_msgctrl, &val);
	if (rc)
		goto err;

	msix_entries = msix_table_size(val);

	if (msix_entries > ndev->limits.msix_cnt) {
		rc = -EINVAL;
		goto err;
	}

	ndev->msix_entries = kmalloc(sizeof(struct msix_entry) * msix_entries,
				     GFP_KERNEL);
	if (!ndev->msix_entries) {
		rc = -ENOMEM;
		goto err;
	}

	for (i = 0; i < msix_entries; i++)
		ndev->msix_entries[i].entry = i;

	rc = pci_enable_msix(pdev, ndev->msix_entries, msix_entries);
	if (rc < 0)
		goto err1;
	if (rc > 0) {
		//FIXME - should be able to handle less than the number of irqs we requested, but we'd have to modify the registers to specify which interrupt bits go on each vector
		rc = -EIO;
		goto err1;
	}

	for (i = 0; i < msix_entries; i++) {
		msix = &ndev->msix_entries[i];
		rc = request_irq(msix->vector, ntb_msix_irq, 0, "ntb-msix", ndev);
		if (rc)
			goto err2;
	}

	/* Disable intx */
	pci_intx(pdev, 0);

	ndev->num_msix = msix_entries;

	return 0;

err2:
	for (i--; i >= 0; i--) {
		msix = &ndev->msix_entries[i];
		free_irq(msix->vector, ndev);
	}
	pci_disable_msix(pdev);
err1:
	kfree(ndev->msix_entries);
err:
	ndev->num_msix = 0;
	dev_err(&pdev->dev, "Error allocating MSI-X interrupt\n");
	return rc;
}

static int ntb_setup_msi(struct ntb_device *ndev)
{
	struct pci_dev *pdev = ndev->pdev;
	int rc;

	rc = pci_enable_msi(pdev);
	if (rc)
		return rc;

	rc = request_irq(pdev->irq, ntb_interrupt, 0, "ntb-msi", ndev);
	if (rc) {
		pci_disable_msi(pdev);
		dev_err(&pdev->dev, "Error allocating MSI interrupt\n");
		return rc;
	}

	return 0;
}

static int ntb_setup_intx(struct ntb_device *ndev)
{
	struct pci_dev *pdev = ndev->pdev;
	int rc;

	pci_msi_off(pdev);

	/* Verify intx is enabled */
	pci_intx(pdev, 1);

	rc = request_irq(pdev->irq, ntb_interrupt, IRQF_SHARED, "ntb-intx", ndev);
	if (rc)
		return rc;

	return 0;
}

static int ntb_setup_interrupts(struct ntb_device *ndev)
{
	int rc;

	/* Enable Link/HB Interrupt, the rest will be unmasked as callbacks are registered */
	writew(~(1 << (ndev->limits.max_db_bits - 1)), ndev->reg_base + ndev->reg_ofs.pdb_mask);

	rc = ntb_setup_msix(ndev);
	if (!rc)
		goto done;

	rc = ntb_setup_msi(ndev);
	if (!rc)
		goto done;

	rc = ntb_setup_intx(ndev);
	if (rc) {
		dev_err(&ndev->pdev->dev, "no usable interrupts\n");
		return rc;
	}

done:
	return 0;
}

static void ntb_free_interrupts(struct ntb_device *ndev)
{
	struct pci_dev *pdev = ndev->pdev;

	/* mask interrupts */
	writew(~0, ndev->reg_base + ndev->reg_ofs.pdb_mask);

	if (ndev->num_msix) {
		struct msix_entry *msix;
		u32 i;

		for (i = 0; i < ndev->num_msix; i++) {
			msix = &ndev->msix_entries[i];
			free_irq(msix->vector, ndev);
		}
		pci_disable_msix(pdev);
	} else
		free_irq(pdev->irq, ndev);
}

static int ntb_create_callbacks(struct ntb_device *ndev)
{
	int i;

	ndev->db_cb = kcalloc(ndev->limits.max_db_bits, sizeof(struct ntb_db_cb), GFP_KERNEL);
	if (!ndev->db_cb)
		return -ENOMEM;

	for (i = 0; i < ndev->limits.max_db_bits; i++)
		ndev->db_cb[i].db_num = i;

	return 0;
}

static void ntb_free_callbacks(struct ntb_device *ndev)
{
	int i;

	for (i = 0; i < ndev->limits.max_db_bits; i++)
		ntb_unregister_db_callback(ndev, i);

	kfree(ndev->db_cb);
}

static int __devinit
ntb_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct ntb_device *ndev;
	int err, i;

	ndev = kzalloc(sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;

	ntbdev = ndev;
	ndev->pdev = pdev;
	pci_set_drvdata(pdev, ndev);

	err = pci_enable_device(pdev);
	if (err)
		goto err;

	pci_set_master(ndev->pdev);

	err = pci_request_selected_regions(pdev, NTB_BAR_MASK, KBUILD_MODNAME);
	if (err)
		goto err1;

	ndev->reg_base = pci_ioremap_bar(pdev, NTB_BAR_MMIO);
	if (!ndev->reg_base) {
		err = -EIO;
		goto err2;
	}

	for (i = 0; i < NTB_NUM_MW; i++) {
		ndev->mw[i].vbase = pci_ioremap_bar(pdev, MW_TO_BAR(i));
		dev_info(&pdev->dev, "Addr %p len %d\n", ndev->mw[i].vbase, (u32) pci_resource_len(pdev, i));
		if (!ndev->mw[i].vbase) {
			err = -EIO;
			goto err3;
		}
	}

	err = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
	if (err) {
		err = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
		if (err)
			goto err3;
		//FIXME - let upper layers know of this limitation?
		dev_warn(&pdev->dev, "Cannot DMA highmem\n");
	}

	err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64));
	if (err) {
		err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
		if (err)
			goto err3;

		dev_warn(&pdev->dev, "Cannot DMA consistent highmem\n");
	}

	/* Enable Bus Master and Memory Space on the secondary side */
	writew(PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER, ndev->reg_base + NTB_PCICMD_OFFSET);

	err = ntb_device_setup(ndev);
	if (err)
		goto err3;

	err = ntb_setup_interrupts(ndev);
	if (err)
		goto err4;

	err = ntb_create_callbacks(ndev);
	if (err)
		goto err5;

	ndev->link_status = NTB_LINK_DOWN;

	/* lets bring the NTB link up */
	writel(NTB_CNTL_BAR23_SNOOP | NTB_CNTL_BAR45_SNOOP, ndev->reg_base + ndev->reg_ofs.lnk_cntl);

	return 0;

err5:
	ntb_free_interrupts(ndev);
err4:
	ntb_device_free(ndev);
err3:
	for (i--; i >= 0; i--)
		iounmap(ndev->mw[i].vbase);
	iounmap(ndev->reg_base);
err2:
	pci_release_selected_regions(pdev, NTB_BAR_MASK);
err1:
	pci_disable_device(pdev);
err:
	kfree(ndev);

	dev_err(&pdev->dev, "Error loading module\n");
	return err;
}

static void __devexit ntb_pci_remove(struct pci_dev *pdev)
{
	struct ntb_device *ndev = pci_get_drvdata(pdev);
	int i;
	u32 ntb_cntl;

	/* Bring NTB link down */
	ntb_cntl = readl(ndev->reg_base + ndev->reg_ofs.lnk_cntl);
	ntb_cntl |= NTB_LINK_DISABLE;
	writel(ntb_cntl, ndev->reg_base + ndev->reg_ofs.lnk_cntl);

	ntb_free_callbacks(ndev);

	ntb_free_interrupts(ndev);

	ntb_device_free(ndev);

	iounmap(ndev->reg_base);

	for (i = 0; i < NTB_NUM_MW; i++)
		iounmap(ndev->mw[i].vbase);

	pci_release_selected_regions(pdev, NTB_BAR_MASK);

	pci_disable_device(pdev);

	kfree(ndev);
}

static struct pci_driver ntb_pci_driver = {
	.name		= KBUILD_MODNAME,
	.id_table	= ntb_pci_tbl,
	.probe		= ntb_pci_probe,
	.remove		= __devexit_p(ntb_pci_remove),
};

static int __init ntb_init_module(void)
{
	pr_info("%s: %s, version %s\n", KBUILD_MODNAME, NTB_NAME, NTB_VER);

	return pci_register_driver(&ntb_pci_driver);
}
module_init(ntb_init_module);

static void __exit ntb_exit_module(void)
{
	pci_unregister_driver(&ntb_pci_driver);

	pr_info("%s: Driver removed\n", KBUILD_MODNAME);
}
module_exit(ntb_exit_module);
