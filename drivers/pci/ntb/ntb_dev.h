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

#ifndef NTB_DEV_H
#define NTB_DEV_H

#define msix_table_size(control)	((control & PCI_MSIX_FLAGS_QSIZE)+1)

#define NTB_DEV_B2B		0
#define NTB_DEV_CLASSIC		1
#define NTB_DEV_RP		2

#define NTB_BAR_MMIO		0
#define NTB_BAR_23		2
#define NTB_BAR_45		4
#define NTB_BAR_MASK		(1 << NTB_BAR_MMIO) | (1 << NTB_BAR_23) |\
				(1 << NTB_BAR_45)

#define NTB_LINK_DOWN		0
#define NTB_LINK_UP		1

#define NTB_HB_TIMEOUT		msecs_to_jiffies(1000)

typedef int(*db_cb_func)(void);
typedef int(*event_cb_func)(void *handle, unsigned int event);

struct ntb_db_cb {
	struct work_struct db_work;
	db_cb_func callback;
	struct ntb_device *ndev;
};

struct ntb_device {
	struct list_head list;
	struct pci_dev *pdev;
	int dev_type;
	int num_msix;
	struct msix_entry *msix_entries;
	void __iomem *reg_base;
	void __iomem *pbar23;
	void __iomem *pbar45;
	resource_size_t pbar23_sz;
	resource_size_t pbar45_sz;
	struct {
		int max_compat_spads;
		int max_spads;
		int max_db_bits;
		int msix_cnt;
	} limits;
	struct {
		u32 pdb;
		u32 pdb_mask;
		u32 sdb;
		u32 sbar2_xlat;
		u32 sbar4_xlat;
		u32 spad_write;
		u32 spad_read;
		u32 lnk_cntl;
		u32 lnk_stat;
		u32 msix_msgctrl;
	} reg_ofs;
	struct ntb_db_cb *db_cb;
	unsigned int link_status;
	struct delayed_work hb_timer;
	unsigned long last_ts;
	event_cb_func event_cb;
	void *ntb_transport;
};


/**
 * ntb_register_transport() - Register NTB transport with NTB HW driver
 * @transport: transport identifier
 *
 * This function allows a transport to reserve the hardware driver for
 * NTB usage.
 *
 * RETURNS: pointer to ntb_device, NULL on error.
 */
struct ntb_device *ntb_register_transport(void *transport);

/**
 * ntb_unregister_transport() - Unregister the transport with the NTB HW driver
 * @ndev - ntb_device of the transport to be freed
 *
 * This function unregisters the transport from the HW driver and performs any
 * necessary cleanups.
 */
void ntb_unregister_transport(struct ntb_device *ndev);

/**
 * ntb_set_satr() - set secondary address translation register
 * @ndev: pointer to ntb_device instance
 * @bar: BAR index
 * @addr: translation address register value
 *
 * This function allows writing to the secondary address translation register
 * in order for the remote end to access the local DMA mapped memory.
 */
void ntb_set_satr(struct ntb_device *ndev, unsigned int bar, u64 addr);

/**
 * ntb_register_db_callback() - register a callback for doorbell interrupt
 * @ndev: pointer to ntb_device instance
 * @idx: doorbell index to register callback, 0 based
 * @func: callback function to register
 *
 * This function registers a callback function for the doorbell interrupt
 * on the primary side. The function will unmask the doorbell as well to
 * allow interrupt.
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int ntb_register_db_callback(struct ntb_device *ndev, unsigned int idx, db_cb_func func);

/**
 * ntb_unregister_db_callback() - unregister a callback for doorbell interrupt
 * @ndev: pointer to ntb_device instance
 * @idx: doorbell index to register callback, 0 based
 *
 * This function unregisters a callback function for the doorbell interrupt
 * on the primary side. The function will also mask the said doorbell.
 */
void ntb_unregister_db_callback(struct ntb_device *ndev, unsigned int idx);

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
int ntb_register_event_callback(struct ntb_device *ndev, event_cb_func func);

/**
 * ntb_unregister_event_callback() - unregisters the event callback
 * @ndev: pointer to ntb_device instance
 *
 * This function unregisters the existing callback from transport
 */
void ntb_unregister_event_callback(struct ntb_device *ndev);

/**
 * ntb_get_max_spads() - get the total scratch regs usable
 * @ndev: pointer to ntb_device instance
 *
 * This function returns the max 32bit scratchpad registers usable by the
 * upper layer.
 *
 * RETURNS: total number of scratch pad registers available
 */
int ntb_get_max_spads(struct ntb_device *ndev);

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
int ntb_write_spad(struct ntb_device *ndev, unsigned int idx, u32 val);

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
int ntb_read_spad(struct ntb_device *ndev, unsigned int idx, u32 *val);

/**
 * ntb_get_pbar_vbase() - get virtual addr for the NTB BAR
 * @ndev: pointer to ntb_device instance
 * @bar: BAR index
 *
 * This function provides the ioremapped virtual address for the PCI BAR
 * for the NTB device for BAR 2/3 or BAR 4/5. The BAR index is provided
 * as a #define.
 *
 * RETURNS: pointer to BAR virtual address, NULL on error.
 */
void *ntb_get_pbar_vbase(struct ntb_device *ndev, unsigned int bar);

/**
 * ntb_get_pbar_size() - return size of NTB BAR
 * @ndev: pointer to ntb_device instance
 * @bar: BAR index
 *
 * This function provides the size of the NTB PCI BAR 2/3 or 3/4. The BAR
 * index is provided as a #define
 *
 * RETURNS: the size of the NTB PCI BAR 2/3 or 3/4
 */
resource_size_t ntb_get_pbar_size(struct ntb_device *ndev, unsigned int bar);

/**
 * ntb_ring_sdb() - Set the doorbell on the secondary/external side
 * @ndev: pointer to ntb_device instance
 * @db: doorbell index, 0 based
 *
 * This function allows triggering of a doorbell on the secondary/external
 * side that will initiate an interrupt on the remote host
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int ntb_ring_sdb(struct ntb_device *ndev, unsigned int idx);
#endif
