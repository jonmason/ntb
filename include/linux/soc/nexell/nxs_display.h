/*
 * Copyright (C) 2017  Nexell Co., Ltd.
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _NXS_DISPLAY_H
#define _NXS_DISPLAY_H

enum nxs_display_type {
	NXS_DISPLAY_RGB,
	NXS_DISPLAY_LVDS,
	NXS_DISPLAY_MIPI_DSI,
	NXS_DISPLAY_HDMI,
	NXS_DISPLAY_HDMI_DVI,
	NXS_DISPLAY_TVOUT,
};

/* nxs_control's type parameter */
enum nxs_display_status_type {
	NXS_DISPLAY_STATUS_CONNECT,
	NXS_DISPLAY_STATUS_MODE,
	NXS_DISPLAY_STATUS_EVENT,
};

struct nxs_control_display {
	enum nxs_display_type type;
	int module;
	u32 format;		/* out format */
	u32 mode;		/* LVDS 0:VESA, 1:JEIDA, 2: Location */
	bool i80;
	u32 dual_link_enb;
	u32 dual_link_mode;
	/* sysnc share */
	u32 sync_share;
	u32 sync_share_muxsel;
	struct debug {
		bool fcs;
	} debug;
};

#endif

