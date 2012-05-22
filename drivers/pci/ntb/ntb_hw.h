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

#define msix_table_size(control)	((control & PCI_MSIX_FLAGS_QSIZE)+1)

#define NTB_BAR_MMIO		0
#define NTB_BAR_23		2
#define NTB_BAR_45		4
#define NTB_BAR_MASK		(1 << NTB_BAR_MMIO) | (1 << NTB_BAR_23) |\
				(1 << NTB_BAR_45)

#define NTB_LINK_DOWN		0
#define NTB_LINK_UP		1

#define NTB_HB_TIMEOUT		msecs_to_jiffies(1000)

typedef void (*db_cb_func) (int db_num);
typedef void (*event_cb_func) (void *handle, unsigned int event);

struct ntb_db_cb {
	db_cb_func callback;
	unsigned int db_num;
	struct ntb_device *ndev;
};

#define NTB_NUM_MW	2

enum {
	NTB_EVENT_SW_EVENT0	= (1 << 0),
	NTB_EVENT_SW_EVENT1	= (1 << 1),
	NTB_EVENT_SW_EVENT2	= (1 << 2),
	NTB_EVENT_HW_ERROR	= (1 << 3),
	NTB_EVENT_HW_LINK_UP	= (1 << 4),
	NTB_EVENT_HW_LINK_DOWN	= (1 << 5),
};

bool ntb_hw_link_status(struct ntb_device *ndev);
struct pci_dev *ntb_query_pdev(struct ntb_device *ndev);
unsigned int ntb_query_max_cbs(struct ntb_device *ndev);
struct ntb_device *ntb_register_transport(void *transport);
void ntb_unregister_transport(struct ntb_device *ndev);
void ntb_set_mw_addr(struct ntb_device *ndev, unsigned int mw, u64 addr);
int ntb_register_db_callback(struct ntb_device *ndev, unsigned int idx,
			     db_cb_func func);
void ntb_unregister_db_callback(struct ntb_device *ndev, unsigned int idx);
int ntb_register_event_callback(struct ntb_device *ndev, event_cb_func func);
void ntb_unregister_event_callback(struct ntb_device *ndev);
int ntb_get_max_spads(struct ntb_device *ndev);
int ntb_write_local_spad(struct ntb_device *ndev, unsigned int idx, u32 val);
int ntb_read_local_spad(struct ntb_device *ndev, unsigned int idx, u32 *val);
int ntb_write_remote_spad(struct ntb_device *ndev, unsigned int idx, u32 val);
int ntb_read_remote_spad(struct ntb_device *ndev, unsigned int idx, u32 *val);
void *ntb_get_mw_vbase(struct ntb_device *ndev, unsigned int mw);
resource_size_t ntb_get_mw_size(struct ntb_device *ndev, unsigned int mw);
int ntb_ring_sdb(struct ntb_device *ndev, unsigned int idx);
