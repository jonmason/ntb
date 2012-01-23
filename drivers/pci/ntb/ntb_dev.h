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
 */

#ifndef NTB_DEV_H
#define NTB_DEV_H

#define DRV_NAME "ntb"

#define NTB_DEV_B2B		0
#define NTB_DEV_CLASSIC		1
#define NTB_DEV_RP		2

#define NTB_BAR_MMIO		0
#define NTB_BAR_23		2
#define NTB_BAR_45		4
#define NTB_BAR_MASK		(1 << NTB_BAR_MMIO) | (1 << NTB_BAR_23) |\
				(1 << NTB_BAR_45)

#define NTB_MSIXMSGCTRL_ENTRIES_MASK		0x7ff
#define NTB_MSIXMSGCTRL_ENABLED_MASK            0x8000

#define NTB_DB_HID_HEARTBEAT	0x0001
#define NTB_DB_LAST		NTB_DB_HID_HEARTBEAT

#define NTB_LINK_UNKNOWN	0x0
#define NTB_LINK_DOWN		0x1
#define NTB_LINK_SELF_UP	0x2
#define NTB_LINK_PEER_UP	0x6

#define NTB_HB_TIMEOUT		msecs_to_jiffies(1000)
#define NTB_HB_SEND_TIMEOUT	msecs_to_jiffies(500)

#define NTB_EVENT_LINK_UP	0x1
#define NTB_EVENT_LINK_DOWN	0x2

struct ntb_bars {
	u32 state;
	void __iomem *vir_addr;
	dma_addr_t bus_addr;
};

typedef int(*db_cb_func)(void *handle, unsigned int db);
typedef int(*event_cb_func)(void *handle, unsigned int event);

struct ntb_device {
	int dev_id;
	struct pci_dev *pdev;
	u16 msixmsgctrl;
	int dev_type;
	struct msix_entry *msix_entries;
	void __iomem *reg_base;
	void __iomem *pbar23;
	void __iomem *pbar45;
	resource_size_t pbar23_sz;
	resource_size_t pbar45_sz;
	struct {
		int max_compat_spads;
		int max_spads;
		int max_sdbs;
		int msix_cnt;
	} limits;
	struct {
		u32 pdb_ofs;
		u32 pdb_mask_ofs;
		u32 sdb_ofs;
		u32 sbar2_xlat_ofs;
		u32 sbar4_xlat_ofs;
		u32 spad_write_ofs;
		u32 spad_read_ofs;
		u32 lnk_cntl_ofs;
		u32 lnk_stat_ofs;
		u32 msix_msgctrl_ofs;
	} reg_ofs;
	unsigned int link_status;
	void *ntb_transport;
	struct timer_list hb_timer;
	struct timer_list hb_send_timer;
	unsigned long last_ts;
	spinlock_t hb_lock;
	struct tasklet_struct db_tasklet;
	unsigned long db_status;
	void *transport;
	db_cb_func db_callbacks[16];
	event_cb_func event_cb;
	struct list_head list;
};

struct ntb_device * ntb_register_transport(void *transport);
int ntb_unregister_transport(struct ntb_device *ndev, void *transport);
void ntb_set_satr(struct ntb_device *ndev, unsigned int bar, u64 addr);
int ntb_register_db_callback(struct ntb_device *, unsigned int, db_cb_func);
void ntb_unregister_db_callback(struct ntb_device *ndev, unsigned int idx);
int ntb_register_event_callback(struct ntb_device *ndev, event_cb_func func);
void ntb_unregister_event_callback(struct ntb_device *ndev);
int ntb_get_max_spads(struct ntb_device *ndev);
int ntb_write_spad(struct ntb_device *ndev, unsigned int idx, u32 val);
int ntb_read_spad(struct ntb_device *ndev, unsigned int idx, u32 *val);
void * ntb_get_pbar_vbase(struct ntb_device *ndev, unsigned int bar);
resource_size_t ntb_get_pbar_size(struct ntb_device *ndev, unsigned int bar);
int ntb_ring_sdb(struct ntb_device *ndev, unsigned int idx);

#endif
