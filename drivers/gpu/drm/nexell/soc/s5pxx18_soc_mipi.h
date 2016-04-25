/*
 * Copyright (C) 2016  Nexell Co., Ltd.
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
#ifndef _S5PXX18_SOC_MIPI_H_
#define _S5PXX18_SOC_MIPI_H_

#define NUMBER_OF_MIPI_MODULE 1
#define PHY_BASEADDR_MIPI_MODULE	0xC00D0000
#define	PHY_BASEADDR_MIPI_LIST	\
		{ PHY_BASEADDR_MIPI_MODULE }

#define nx_mipi_numberof_csi_channels 2

struct nx_mipi_register_set {
	u32 csis_control;
	u32 csis_dphyctrl;
	u32 csis_config_ch0;
	u32 csis_dphysts;
	u32 csis_intmsk;
	u32 csis_intsrc;
	u32 csis_ctrl2;
	u32 csis_version;
	u32 csis_dphyctrl_0;
	u32 csis_dphyctrl_1;
	u32 __reserved0;
	u32 csis_resol_ch0;
	u32 __reserved1;
	u32 __reserved2;
	u32 sdw_config_ch0;
	u32 sdw_resol_ch0;
	u32 csis_config_ch1;
	u32 csis_resol_ch1;
	u32 sdw_config_ch1;
	u32 sdw_resol_ch1;
	u32 csis_config_ch2;
	u32 csis_resol_ch2;
	u32 sdw_config_ch2;
	u32 sdw_resol_ch2;
	u32 csis_config_ch3;
	u32 csis_resol_ch3;
	u32 sdw_config_ch3;
	u32 sdw_resol_3;
	u32 __reserved3[(16 + 128) / 4];

	u32 dsim_status;
	u32 dsim_swrst;
	u32 dsim_clkctrl;
	u32 dsim_timeout;
	u32 dsim_config;
	u32 dsim_escmode;
	u32 dsim_mdresol;
	u32 dsim_mvporch;
	u32 dsim_mhporch;
	u32 dsim_msync;
	u32 dsim_sdresol;
	u32 dsim_intsrc;
	u32 dsim_intmsk;
	u32 dsim_pkthdr;
	u32 dsim_payload;
	u32 dsim_rxfifo;
	u32 dsim_fifothld;
	u32 dsim_fifoctrl;
	u32 dsim_memacchr;
	u32 dsim_pllctrl;
	u32 dsim_plltmr;
	u32 dsim_phyacchr;
	u32 dsim_phyacchr1;

	u32 __reserved4[(0x2000 - 0x015C) / 4];
	u32 mipi_csis_pktdata[0x2000 / 4];
};

enum nx_mipi_csi_format {
	nx_mipi_csi_format_yuv420_8 = 0x18,
	nx_mipi_csi_format_yuv420_10 = 0x19,
	nx_mipi_csi_format_yuv420_8l = 0x1A,
	nx_mipi_csi_format_yuv420_8c = 0x1C,
	nx_mipi_csi_format_yuv420_10c = 0x1D,
	nx_mipi_csi_format_yuv422_8 = 0x1E,
	nx_mipi_csi_format_yuv422_10 = 0x1F,
	nx_mipi_csi_format_rgb565 = 0x22,
	nx_mipi_csi_format_rgb666 = 0x23,
	nx_mipi_csi_format_rgb888 = 0x24,
	nx_mipi_csi_format_raw6 = 0x28,
	nx_mipi_csi_format_raw7 = 0x29,
	nx_mipi_csi_format_raw8 = 0x2A,
	nx_mipi_csi_format_raw10 = 0x2B,
	nx_mipi_csi_format_raw12 = 0x2C,
	nx_mipi_csi_format_raw14 = 0x2D,
	nx_mipi_csi_format_user0 = 0x30,
	nx_mipi_csi_format_user1 = 0x31,
	nx_mipi_csi_format_user2 = 0x32,
	nx_mipi_csi_format_user3 = 0x33,
};

enum nx_mipi_csi_interleave {
	nx_mipi_csi_interleave_ch0 = 0,
	nx_mipi_csi_interleave_dt = 1,
	nx_mipi_csi_interleave_vc = 2,
	nx_mipi_csi_interleave_vcdt = 3
};

enum nx_mipi_csi_prediction {
	nx_mipi_csi_prediction_simple = 0,
	nx_mipi_csi_prediction_normal = 1
};

enum nx_mipi_csi_yuv422layout {
	nx_mipi_csi_yuv422layout_half = 0,
	nx_mipi_csi_yuv422layout_full = 1
};

enum nx_mipi_csi_vclksrc {
	nx_mipi_csi_vclksrc_pclk = 0,
	nx_mipi_csi_vclksrc_extclk = 1
};

enum nx_mipi_csi_rgblayout {
	nx_mipi_csi_rgblayout_rgb = 0,
	nx_mipi_csi_rgblayout_bgr = 1
};

enum nx_mipi_dsi_syncmode {
	nx_mipi_dsi_syncmode_event = 0,
	nx_mipi_dsi_syncmode_pulse = 1,
};

enum nx_mipi_dsi_format {
	nx_mipi_dsi_format_command3 = 0,
	nx_mipi_dsi_format_command8 = 1,
	nx_mipi_dsi_format_command12 = 2,
	nx_mipi_dsi_format_command16 = 3,
	nx_mipi_dsi_format_rgb565 = 4,
	nx_mipi_dsi_format_rgb666_packed = 5,
	nx_mipi_dsi_format_rgb666 = 6,
	nx_mipi_dsi_format_rgb888 = 7
};

enum nx_mipi_dsi_lpmode {
	nx_mipi_dsi_lpmode_hs = 0,
	nx_mipi_dsi_lpmode_lp = 1
};

enum nx_mipi_phy_b_dphyctl {
	nx_mipi_phy_b_dphyctl_m_txclkesc_20_mhz = 0x1F4,
	nx_mipi_phy_b_dphyctl_m_txclkesc_19_mhz = 0x1DB,
	nx_mipi_phy_b_dphyctl_m_txclkesc_18_mhz = 0x1C2,
	nx_mipi_phy_b_dphyctl_m_txclkesc_17_mhz = 0x1A9,
	nx_mipi_phy_b_dphyctl_m_txclkesc_16_mhz = 0x190,
	nx_mipi_phy_b_dphyctl_m_txclkesc_15_mhz = 0x177,
	nx_mipi_phy_b_dphyctl_m_txclkesc_14_mhz = 0x15E,
	nx_mipi_phy_b_dphyctl_m_txclkesc_13_mhz = 0x145,
	nx_mipi_phy_b_dphyctl_m_txclkesc_12_mhz = 0x12C,
	nx_mipi_phy_b_dphyctl_m_txclkesc_11_mhz = 0x113,
	nx_mipi_phy_b_dphyctl_m_txclkesc_10_mhz = 0x0FA,
	nx_mipi_phy_b_dphyctl_m_txclkesc_9_mhz = 0x0E1,
	nx_mipi_phy_b_dphyctl_m_txclkesc_8_mhz = 0x0C8,
	nx_mipi_phy_b_dphyctl_m_txclkesc_7_mhz = 0x0AF,
	nx_mipi_phy_b_dphyctl_m_txclkesc_6_mhz = 0x096,
	nx_mipi_phy_b_dphyctl_m_txclkesc_5_mhz = 0x07D,
	nx_mipi_phy_b_dphyctl_m_txclkesc_4_mhz = 0x064,
	nx_mipi_phy_b_dphyctl_m_txclkesc_3_mhz = 0x04B,
	nx_mipi_phy_b_dphyctl_m_txclkesc_2_mhz = 0x032,
	nx_mipi_phy_b_dphyctl_m_txclkesc_1_mhz = 0x019,
	nx_mipi_phy_b_dphyctl_m_txclkesc_0_10_mhz = 0x003,
	nx_mipi_phy_b_dphyctl_m_txclkesc_0_01_mhz = 0x000
};

enum {
	nx_mipi_rst = 0,
	nx_mipi_rst_dsi_i,
	nx_mipi_rst_csi_i,
	nx_mipi_rst_phy_s,
	nx_mipi_rst_phy_m
};

enum nx_mipi_int {
	nx_mipi_int_csi_even_before = 31,
	nx_mipi_int_csi_even_after = 30,
	nx_mipi_int_csi_odd_before = 29,
	nx_mipi_int_csi_odd_after = 28,
	nx_mipi_int_csi_frame_start_ch3 = 27,
	nx_mipi_int_csi_frame_start_ch2 = 26,
	nx_mipi_int_csi_frame_start_ch1 = 25,
	nx_mipi_int_csi_frame_start_ch0 = 24,
	nx_mipi_int_csi_frame_end_ch3 = 23,
	nx_mipi_int_csi_frame_end_ch2 = 22,
	nx_mipi_int_csi_frame_end_ch1 = 21,
	nx_mipi_int_csi_frame_end_ch0 = 20,
	nx_mipi_int_csi_err_sot_hs_ch3 = 19,
	nx_mipi_int_csi_err_sot_hs_ch2 = 18,
	nx_mipi_int_csi_err_sot_hs_ch1 = 17,
	nx_mipi_int_csi_err_sot_hs_ch0 = 16,
	nx_mipi_int_csi_err_lost_fs_ch3 = 15,
	nx_mipi_int_csi_err_lost_fs_ch2 = 14,
	nx_mipi_int_csi_err_lost_fs_ch1 = 13,
	nx_mipi_int_csi_err_lost_fs_ch0 = 12,
	nx_mipi_int_csi_err_lost_fe_ch3 = 11,
	nx_mipi_int_csi_err_lost_fe_ch2 = 10,
	nx_mipi_int_csi_err_lost_fe_ch1 = 9,
	nx_mipi_int_csi_err_lost_fe_ch0 = 8,
	nx_mipi_int_csi_err_over_ch3 = 7,
	nx_mipi_int_csi_err_over_ch2 = 6,
	nx_mipi_int_csi_err_over_ch1 = 5,
	nx_mipi_int_csi_err_over_ch0 = 4,

	nx_mipi_int_csi_err_ecc = 2,
	nx_mipi_int_csi_err_crc = 1,
	nx_mipi_int_csi_err_id = 0,
	nx_mipi_int_dsi_pll_stable = 32 + 31,
	nx_mipi_int_dsi_sw_rst_release = 32 + 30,
	nx_mipi_int_dsi_sfrplfifoempty = 32 + 29,
	nx_mipi_int_dsi_sfrphfifoempty = 32 + 28,
	nx_mipi_int_dsi_sync_override = 32 + 27,

	nx_mipi_int_dsi_bus_turn_over = 32 + 25,
	nx_mipi_int_dsi_frame_done = 32 + 24,

	nx_mipi_int_dsi_lpdr_tout = 32 + 21,
	nx_mipi_int_dsi_ta_tout = 32 + 20,

	nx_mipi_int_dsi_rx_dat_done = 32 + 18,
	nx_mipi_int_dsi_rx_te = 32 + 17,
	nx_mipi_int_dsi_rx_ack = 32 + 16,
	nx_mipi_int_dsi_err_rx_ecc = 32 + 15,
	nx_mipi_int_dsi_err_rx_crc = 32 + 14,
	nx_mipi_int_dsi_err_esc3 = 32 + 13,
	nx_mipi_int_dsi_err_esc2 = 32 + 12,
	nx_mipi_int_dsi_err_esc1 = 32 + 11,
	nx_mipi_int_dsi_err_esc0 = 32 + 10,
	nx_mipi_int_dsi_err_sync3 = 32 + 9,
	nx_mipi_int_dsi_err_sync2 = 32 + 8,
	nx_mipi_int_dsi_err_sync1 = 32 + 7,
	nx_mipi_int_dsi_err_sync0 = 32 + 6,
	nx_mipi_int_dsi_err_control3 = 32 + 5,
	nx_mipi_int_dsi_err_control2 = 32 + 4,
	nx_mipi_int_dsi_err_control1 = 32 + 3,
	nx_mipi_int_dsi_err_control0 = 32 + 2,
	nx_mipi_int_dsi_err_content_lp0 = 32 + 1,
	nx_mipi_int_dsi_err_content_lp1 = 32 + 0,
};

#define DSI_TX_FIFO_SIZE	2048
#define DSI_RX_FIFO_SIZE	256
#define DSI_RX_FIFO_EMPTY	0x30800002

void nx_mipi_csi_set_size(u32 module_index,
				int channel, u32 width, u32 height);
void nx_mipi_csi_set_format(u32 module_index, int channel,
				enum nx_mipi_csi_format format);

void nx_mipi_csi_set_interleave_mode(u32 module_index,
				enum nx_mipi_csi_interleave
				interleave_mode);

void nx_mipi_csi_set_timing_control(u32 module_index, int channel,
				int t1, int t2, int t5);

void nx_mipi_csi_set_interleave_channel(u32 module_index, int channel,
				int interleave_channel);
void nx_mipi_csi_enable_decompress(u32 module_index, int enable);

void nx_mipi_csi_set_prediction(u32 module_index, int channel,
				enum nx_mipi_csi_prediction prediction);

void nx_mipi_csi_set_yuv422_layout(u32 module_index, int channel,
				enum nx_mipi_csi_yuv422layout
				yuv422_layout);
void nx_mipi_csi_set_parallel_data_alignment32(u32 module_index,
				int channel,
				int enable_align32);

void nx_mipi_csi_set_rgblayout(u32 module_index, int channel,
				enum nx_mipi_csi_rgblayout rgb_layout);

void nx_mipi_csi_set_vclk(u32 module_index, int channel,
				enum nx_mipi_csi_vclksrc clock_source);
void nx_mipi_csi_software_reset(u32 module_index);

void nx_mipi_csi_set_enable(u32 module_index, int enable);
void nx_mipi_csi_set_phy(u32 module_index, u32 number_of_data_lanes,
				int enable_clock_lane, int enable_data_lane0,
				int enable_data_lane1, int enable_data_lane2,
				int enable_data_lane3, int swap_clock_lane,
				int swap_data_lane);

void nx_mipi_csi_get_current_state(u32 module_index, int channel,
				int *pwidth, int *pheight, int *pt1,
				int *pt2, int *pt5,
				enum nx_mipi_csi_format *pformat,
				int *pinterleavechannel);

u32 nx_mipi_csi_get_non_image_data(u32 module_index, u32 address32);
void nx_mipi_csi_get_status(u32 module_index, u32 *pulps, u32 *pstop);
u32 nx_mipi_csi_get_version(u32 module_index);

void nx_mipi_dsi_get_status(u32 module_index, u32 *pulps, u32 *pstop,
				u32 *pispllstable, u32 *pisinreset,
				u32 *pisbackward, u32 *pishsclockready);

void nx_mipi_dsi_software_reset(u32 module_index);

void nx_mipi_dsi_set_clock(u32 module_index, int enable_txhsclock,
				int use_external_clock, int enable_byte_clock,
				int enable_escclock_clock_lane,
				int enable_escclock_data_lane0,
				int enable_escclock_data_lane1,
				int enable_escclock_data_lane2,
				int enable_escclock_data_lane3,
				int enable_escprescaler,
				u32 escprescalervalue);

void nx_mipi_dsi_set_timeout(u32 module_index, u32 bta_tout,
				u32 lpdrtout);

void nx_mipi_dsi_set_config_video_mode(u32 module_index,
				int enable_auto_flush_main_display_fifo,
				int enable_auto_vertical_count,
				int enable_burst,
				enum nx_mipi_dsi_syncmode
				sync_mode, int enable_eo_tpacket,
				int enable_hsync_end_packet,
				int enable_hfp, int enable_hbp,
				int enable_hsa,
				u32 number_of_virtual_channel,
				enum nx_mipi_dsi_format format,
				u32 number_of_words_in_hfp,
				u32 number_of_words_in_hbp,
				u32 number_of_words_in_hsync,
				u32 number_of_lines_in_vfp,
				u32 number_of_lines_in_vbp,
				u32 number_of_lines_in_vsync,
				u32 number_of_lines_in_command_allow);

void nx_mipi_dsi_set_config_command_mode(u32 module_index,
				int enable_auto_flush_main_display_fifo,
				int enable_eo_tpacket,
				u32 number_of_virtual_channel,
				enum nx_mipi_dsi_format format);

void nx_mipi_dsi_set_escape_mode(u32 module_index, u32 stop_state_count,
				int force_stop_state, int force_bta,
				enum nx_mipi_dsi_lpmode cmdin_lp,
				enum nx_mipi_dsi_lpmode txinlp);
void nx_mipi_dsi_set_escape_lp(u32 module_index,
				enum nx_mipi_dsi_lpmode cmdin_lp,
				enum nx_mipi_dsi_lpmode txinlp);

void nx_mipi_dsi_remote_reset_trigger(u32 module_index);
void nx_mipi_dsi_set_ulps(u32 module_index, int ulpsclocklane,
				int ulpsdatalane);
void nx_mipi_dsi_set_size(u32 module_index, u32 width, u32 height);
void nx_mipi_dsi_set_enable(u32 module_index, int enable);
void nx_mipi_dsi_set_phy(u32 module_index, u32 number_of_data_lanes,
				int enable_clock_lane, int enable_data_lane0,
				int enable_data_lane1, int enable_data_lane2,
				int enable_data_lane3, int swap_clock_lane,
				int swap_data_lane);

void nx_mipi_dsi_set_pll(u32 module_index, int enable,
				u32 pllstabletimer, u32 m_pllpms, u32 m_bandctl,
				u32 m_dphyctl, u32 b_dphyctl);

void nx_mipi_dsi_write_pkheader(u32 module_index, u32 data);
void nx_mipi_dsi_write_payload(u32 module_index, u32 data);
u32 nx_mipi_dsi_read_fifo(u32 module_index);

int nx_mipi_smoke_test(u32 module_index);
int nx_mipi_initialize(void);
u32 nx_mipi_get_number_of_module(void);
u32 nx_mipi_get_size_of_register_set(void);
void nx_mipi_set_base_address(u32 module_index, void *base_address);
void *nx_mipi_get_base_address(u32 module_index);
u32 nx_mipi_get_physical_address(u32 module_index);
int nx_mipi_open_module(u32 module_index);
int nx_mipi_close_module(u32 module_index);
int nx_mipi_check_busy(u32 module_index);

void nx_mipi_set_interrupt_enable(u32 module_index,
				u32 int_num, int enable);
int nx_mipi_get_interrupt_enable(u32 module_index, u32 int_num);
int nx_mipi_get_interrupt_pending(u32 module_index, u32 int_num);
void nx_mipi_clear_interrupt_pending(u32 module_index, u32 int_num);
void nx_mipi_set_interrupt_enable_all(u32 module_index, int enable);
int nx_mipi_get_interrupt_enable_all(u32 module_index);
int nx_mipi_get_interrupt_pending_all(u32 module_index);
void nx_mipi_clear_interrupt_pending_all(u32 module_index);
int32_t nx_mipi_get_interrupt_pending_number(u32 module_index);

#endif
