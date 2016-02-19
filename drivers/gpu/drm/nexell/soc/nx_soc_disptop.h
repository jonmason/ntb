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
#ifndef _NX_SOC_DISPTOP_H_
#define _NX_SOC_DISPTOP_H_

#include "nx_soc_disp.h"

#define hdmi_addr_offset                                                       \
	(((PHY_BASEADDR_DISPLAYTOP_MODULE / 0x00100000) % 2) ? 0x100000        \
							     : 0x000000)
#define other_addr_offset                                                      \
	(((PHY_BASEADDR_DISPLAYTOP_MODULE / 0x00100000) % 2) ? 0x000000        \
							     : 0x100000)
#define PHY_BASEADDR_DISPLAYTOP_MODULE_OFFSET (other_addr_offset + 0x001000)
#define PHY_BASEADDR_DUALDISPLAY_MODULE                                        \
	(PHY_BASEADDR_DISPLAYTOP_MODULE + other_addr_offset + 0x002000)
#define PHY_BASEADDR_RESCONV_MODULE                                            \
	(PHY_BASEADDR_DISPLAYTOP_MODULE + other_addr_offset + 0x003000)
#define PHY_BASEADDR_LCDINTERFACE_MODULE                                       \
	(PHY_BASEADDR_DISPLAYTOP_MODULE + other_addr_offset + 0x004000)
#define PHY_BASEADDR_HDMI_MODULE (PHY_BASEADDR_DISPLAYTOP_MODULE + 0x000000)
#define PHY_BASEADDR_LVDS_MODULE                                               \
	(PHY_BASEADDR_DISPLAYTOP_MODULE + other_addr_offset + 0x00a000)

#define NUMBER_OF_DUALDISPLAY_MODULE 1
#define INTNUM_OF_DUALDISPLAY_MODULE_PRIMIRQ                                   \
	INTNUM_OF_DISPLAYTOP_MODULE_DUALDISPLAY_PRIMIRQ
#define INTNUM_OF_DUALDISPLAY_MODULE_SECONDIRQ                                 \
	INTNUM_OF_DISPLAYTOP_MODULE_DUALDISPLAY_SECONDIRQ
#define RESETINDEX_OF_DUALDISPLAY_MODULE_i_nRST                                \
	RESETINDEX_OF_DISPLAYTOP_MODULE_i_DualDisplay_nRST
#define padindex_of_dualdisplay_o_ncs                                          \
	padindex_of_displaytop_o_dual_display_padprimvclk
#define padindex_of_dualdisplay_o_nrd                                          \
	padindex_of_displaytop_o_dual_display_prim_padn_hsync
#define padindex_of_dualdisplay_o_rs                                           \
	padindex_of_displaytop_o_dual_display_prim_padn_vsync
#define padindex_of_dualdisplay_o_nwr                                          \
	padindex_of_displaytop_o_dual_display_prim_padde
#define padindex_of_dualdisplay_padprimvclk                                    \
	padindex_of_displaytop_o_dual_display_padprimvclk
#define padindex_of_dualdisplay_o_prim_padn_hsync                              \
	padindex_of_displaytop_o_dual_display_prim_padn_hsync
#define padindex_of_dualdisplay_o_prim_padn_vsync                              \
	padindex_of_displaytop_o_dual_display_prim_padn_vsync
#define padindex_of_dualdisplay_o_prim_padde                                   \
	padindex_of_displaytop_o_dual_display_prim_padde
#define padindex_of_dualdisplay_prim_0_                                        \
	padindex_of_displaytop_dual_display_prim_0_
#define padindex_of_dualdisplay_prim_1_                                        \
	padindex_of_displaytop_dual_display_prim_1_
#define padindex_of_dualdisplay_prim_2_                                        \
	padindex_of_displaytop_dual_display_prim_2_
#define padindex_of_dualdisplay_prim_3_                                        \
	padindex_of_displaytop_dual_display_prim_3_
#define padindex_of_dualdisplay_prim_4_                                        \
	padindex_of_displaytop_dual_display_prim_4_
#define padindex_of_dualdisplay_prim_5_                                        \
	padindex_of_displaytop_dual_display_prim_5_
#define padindex_of_dualdisplay_prim_6_                                        \
	padindex_of_displaytop_dual_display_prim_6_
#define padindex_of_dualdisplay_prim_7_                                        \
	padindex_of_displaytop_dual_display_prim_7_
#define padindex_of_dualdisplay_prim_8_                                        \
	padindex_of_displaytop_dual_display_prim_8_
#define padindex_of_dualdisplay_prim_9_                                        \
	padindex_of_displaytop_dual_display_prim_9_
#define padindex_of_dualdisplay_prim_10_                                       \
	padindex_of_displaytop_dual_display_prim_10_
#define padindex_of_dualdisplay_prim_11_                                       \
	padindex_of_displaytop_dual_display_prim_11_
#define padindex_of_dualdisplay_prim_12_                                       \
	padindex_of_displaytop_dual_display_prim_12_
#define padindex_of_dualdisplay_prim_13_                                       \
	padindex_of_displaytop_dual_display_prim_13_
#define padindex_of_dualdisplay_prim_14_                                       \
	padindex_of_displaytop_dual_display_prim_14_
#define padindex_of_dualdisplay_prim_15_                                       \
	padindex_of_displaytop_dual_display_prim_15_
#define padindex_of_dualdisplay_prim_16_                                       \
	padindex_of_displaytop_dual_display_prim_16_
#define padindex_of_dualdisplay_prim_17_                                       \
	padindex_of_displaytop_dual_display_prim_17_
#define padindex_of_dualdisplay_prim_18_                                       \
	padindex_of_displaytop_dual_display_prim_18_
#define padindex_of_dualdisplay_prim_19_                                       \
	padindex_of_displaytop_dual_display_prim_19_
#define padindex_of_dualdisplay_prim_20_                                       \
	padindex_of_displaytop_dual_display_prim_20_
#define padindex_of_dualdisplay_prim_21_                                       \
	padindex_of_displaytop_dual_display_prim_21_
#define padindex_of_dualdisplay_prim_22_                                       \
	padindex_of_displaytop_dual_display_prim_22_
#define padindex_of_dualdisplay_prim_23_                                       \
	padindex_of_displaytop_dual_display_prim_23_
#define padindex_of_dualdisplay_padsecondvclk                                  \
	padindex_of_displaytop_o_dual_display_padprimvclk
#define padindex_of_dualdisplay_o_second_padn_hsync                            \
	padindex_of_displaytop_o_dual_display_prim_padn_hsync
#define padindex_of_dualdisplay_o_second_padn_vsync                            \
	padindex_of_displaytop_o_dual_display_prim_padn_vsync
#define padindex_of_dualdisplay_o_second_padde                                 \
	padindex_of_displaytop_o_dual_display_prim_padde
#define padindex_of_dualdisplay_second_0_                                      \
	padindex_of_displaytop_dual_display_prim_0_
#define padindex_of_dualdisplay_second_1_                                      \
	padindex_of_displaytop_dual_display_prim_1_
#define padindex_of_dualdisplay_second_2_                                      \
	padindex_of_displaytop_dual_display_prim_2_
#define padindex_of_dualdisplay_second_3_                                      \
	padindex_of_displaytop_dual_display_prim_3_
#define padindex_of_dualdisplay_second_4_                                      \
	padindex_of_displaytop_dual_display_prim_4_
#define padindex_of_dualdisplay_second_5_                                      \
	padindex_of_displaytop_dual_display_prim_5_
#define padindex_of_dualdisplay_second_6_                                      \
	padindex_of_displaytop_dual_display_prim_6_
#define padindex_of_dualdisplay_second_7_                                      \
	padindex_of_displaytop_dual_display_prim_7_
#define padindex_of_dualdisplay_second_8_                                      \
	padindex_of_displaytop_dual_display_prim_8_
#define padindex_of_dualdisplay_second_9_                                      \
	padindex_of_displaytop_dual_display_prim_9_
#define padindex_of_dualdisplay_second_10_                                     \
	padindex_of_displaytop_dual_display_prim_10_
#define padindex_of_dualdisplay_second_11_                                     \
	padindex_of_displaytop_dual_display_prim_11_
#define padindex_of_dualdisplay_second_12_                                     \
	padindex_of_displaytop_dual_display_prim_12_
#define padindex_of_dualdisplay_second_13_                                     \
	padindex_of_displaytop_dual_display_prim_13_
#define padindex_of_dualdisplay_second_14_                                     \
	padindex_of_displaytop_dual_display_prim_14_
#define padindex_of_dualdisplay_second_15_                                     \
	padindex_of_displaytop_dual_display_prim_15_
#define padindex_of_dualdisplay_second_16_                                     \
	padindex_of_displaytop_dual_display_prim_16_
#define padindex_of_dualdisplay_second_17_                                     \
	padindex_of_displaytop_dual_display_prim_17_
#define padindex_of_dualdisplay_second_18_                                     \
	padindex_of_displaytop_dual_display_prim_18_
#define padindex_of_dualdisplay_second_19_                                     \
	padindex_of_displaytop_dual_display_prim_19_
#define padindex_of_dualdisplay_second_20_                                     \
	padindex_of_displaytop_dual_display_prim_20_
#define padindex_of_dualdisplay_second_21_                                     \
	padindex_of_displaytop_dual_display_prim_21_
#define padindex_of_dualdisplay_second_22_                                     \
	padindex_of_displaytop_dual_display_prim_22_
#define padindex_of_dualdisplay_second_23_                                     \
	padindex_of_displaytop_dual_display_prim_23_

#define NUMBER_OF_RESCONV_MODULE 1
#define INTNUM_OF_RESCONV_MODULE INTNUM_OF_DISPLAYTOP_MODULE_RESCONV_IRQ
#define RESETINDEX_OF_RESCONV_MODULE_i_nRST                                    \
	RESETINDEX_OF_DISPLAYTOP_MODULE_i_ResConv_nRST
#define RESETINDEX_OF_RESCONV_MODULE RESETINDEX_OF_RESCONV_MODULE_i_nRST
#define NUMBER_OF_LCDINTERFACE_MODULE 1
#define RESETINDEX_OF_LCDINTERFACE_MODULE_i_nRST                               \
	RESETINDEX_OF_DISPLAYTOP_MODULE_i_LCDIF_nRST
#define padindex_of_lcdinterface_o_vclk                                        \
	padindex_of_displaytop_o_dual_display_padprimvclk
#define padindex_of_lcdinterface_o_nhsync                                      \
	padindex_of_displaytop_o_dual_display_prim_padn_hsync
#define padindex_of_lcdinterface_o_nvsync                                      \
	padindex_of_displaytop_o_dual_display_prim_padn_vsync
#define padindex_of_lcdinterface_o_de                                          \
	padindex_of_displaytop_o_dual_display_prim_padde
#define padindex_of_lcdinterface_rgb24_0_                                      \
	padindex_of_displaytop_dual_display_prim_0_
#define padindex_of_lcdinterface_rgb24_1_                                      \
	padindex_of_displaytop_dual_display_prim_1_
#define padindex_of_lcdinterface_rgb24_2_                                      \
	padindex_of_displaytop_dual_display_prim_2_
#define padindex_of_lcdinterface_rgb24_3_                                      \
	padindex_of_displaytop_dual_display_prim_3_
#define padindex_of_lcdinterface_rgb24_4_                                      \
	padindex_of_displaytop_dual_display_prim_4_
#define padindex_of_lcdinterface_rgb24_5_                                      \
	padindex_of_displaytop_dual_display_prim_5_
#define padindex_of_lcdinterface_rgb24_6_                                      \
	padindex_of_displaytop_dual_display_prim_6_
#define padindex_of_lcdinterface_rgb24_7_                                      \
	padindex_of_displaytop_dual_display_prim_7_
#define padindex_of_lcdinterface_rgb24_8_                                      \
	padindex_of_displaytop_dual_display_prim_8_
#define padindex_of_lcdinterface_rgb24_9_                                      \
	padindex_of_displaytop_dual_display_prim_9_
#define padindex_of_lcdinterface_rgb24_10_                                     \
	padindex_of_displaytop_dual_display_prim_10_
#define padindex_of_lcdinterface_rgb24_11_                                     \
	padindex_of_displaytop_dual_display_prim_11_
#define padindex_of_lcdinterface_rgb24_12_                                     \
	padindex_of_displaytop_dual_display_prim_12_
#define padindex_of_lcdinterface_rgb24_13_                                     \
	padindex_of_displaytop_dual_display_prim_13_
#define padindex_of_lcdinterface_rgb24_14_                                     \
	padindex_of_displaytop_dual_display_prim_14_
#define padindex_of_lcdinterface_rgb24_15_                                     \
	padindex_of_displaytop_dual_display_prim_15_
#define padindex_of_lcdinterface_rgb24_16_                                     \
	padindex_of_displaytop_dual_display_prim_16_
#define padindex_of_lcdinterface_rgb24_17_                                     \
	padindex_of_displaytop_dual_display_prim_17_
#define padindex_of_lcdinterface_rgb24_18_                                     \
	padindex_of_displaytop_dual_display_prim_18_
#define padindex_of_lcdinterface_rgb24_19_                                     \
	padindex_of_displaytop_dual_display_prim_19_
#define padindex_of_lcdinterface_rgb24_20_                                     \
	padindex_of_displaytop_dual_display_prim_20_
#define padindex_of_lcdinterface_rgb24_21_                                     \
	padindex_of_displaytop_dual_display_prim_21_
#define padindex_of_lcdinterface_rgb24_22_                                     \
	padindex_of_displaytop_dual_display_prim_22_
#define padindex_of_lcdinterface_rgb24_23_                                     \
	padindex_of_displaytop_dual_display_prim_23_

#define NUMBER_OF_HDMI_MODULE 1
#define INTNUM_OF_HDMI_MODULE INTNUM_OF_DISPLAYTOP_MODULE_HDMI_IRQ
#define RESETINDEX_OF_HDMI_MODULE_i_nRST                                       \
	RESETINDEX_OF_DISPLAYTOP_MODULE_i_HDMI_nRST
#define RESETINDEX_OF_HDMI_MODULE_i_nRST_VIDEO                                 \
	RESETINDEX_OF_DISPLAYTOP_MODULE_i_HDMI_VIDEO_nRST
#define RESETINDEX_OF_HDMI_MODULE_i_nRST_SPDIF                                 \
	RESETINDEX_OF_DISPLAYTOP_MODULE_i_HDMI_SPDIF_nRST
#define RESETINDEX_OF_HDMI_MODULE_i_nRST_TMDS                                  \
	RESETINDEX_OF_DISPLAYTOP_MODULE_i_HDMI_TMDS_nRST
#define RESETINDEX_OF_HDMI_MODULE_i_nRST_PHY                                   \
	RESETINDEX_OF_DISPLAYTOP_MODULE_i_HDMI_PHY_nRST
#define padindex_of_hdmi_i_phy_clki padindex_of_displaytop_i_hdmi_clki
#define padindex_of_hdmi_o_phy_clko padindex_of_displaytop_o_hdmi_clko
#define padindex_of_hdmi_io_phy_rext padindex_of_displaytop_io_hdmi_rext
#define padindex_of_hdmi_o_phy_tx0p padindex_of_displaytop_o_hdmi_tx0p
#define padindex_of_hdmi_o_phy_tx0n padindex_of_displaytop_o_hdmi_tx0n
#define padindex_of_hdmi_o_phy_tx1p padindex_of_displaytop_o_hdmi_tx1p
#define padindex_of_hdmi_o_phy_tx1n padindex_of_displaytop_o_hdmi_tx1n
#define padindex_of_hdmi_o_phy_tx2p padindex_of_displaytop_o_hdmi_tx2p
#define padindex_of_hdmi_o_phy_tx2n padindex_of_displaytop_o_hdmi_tx2n
#define padindex_of_hdmi_o_phy_txcp padindex_of_displaytop_o_hdmi_txcp
#define padindex_of_hdmi_o_phy_txcn padindex_of_displaytop_o_hdmi_txcn
#define padindex_of_hdmi_i_hotplug padindex_of_displaytop_i_hdmi_hotplug_5v
#define padindex_of_hdmi_io_pad_cec padindex_of_displaytop_io_hdmi_cec
#define NUMBER_OF_LVDS_MODULE 1

#define RESETINDEX_OF_LVDS_MODULE_I_RESETN                                     \
	RESETINDEX_OF_DISPLAYTOP_MODULE_i_LVDS_nRST
#define RESETINDEX_OF_LVDS_MODULE RESETINDEX_OF_LVDS_MODULE_I_RESETN

#define padindex_of_lvds_tap padindex_of_displaytop_lvds_txp_a
#define padindex_of_lvds_tan padindex_of_displaytop_lvds_txn_a
#define padindex_of_lvds_tbp padindex_of_displaytop_lvds_txp_b
#define padindex_of_lvds_tbn padindex_of_displaytop_lvds_txn_b
#define padindex_of_lvds_tcp padindex_of_displaytop_lvds_txp_c
#define padindex_of_lvds_tcn padindex_of_displaytop_lvds_txn_c
#define padindex_of_lvds_tdp padindex_of_displaytop_lvds_txp_d
#define padindex_of_lvds_tdn padindex_of_displaytop_lvds_txn_d
#define padindex_of_lvds_tclkp padindex_of_displaytop_lvds_txp_clk
#define padindex_of_lvds_tclkn padindex_of_displaytop_lvds_txn_clk
#define padindex_of_lvds_rout padindex_of_displaytop_lvds_rout
#define padindex_of_lvds_tep padindex_of_displaytop_lvds_txn_e
#define padindex_of_lvds_ten padindex_of_displaytop_lvds_txn_e
#define NUMBER_OF_DISPTOP_CLKGEN_MODULE 5

enum disptop_clkgen_module_index {
	res_conv_clkgen = 0,
	lcdif_clkgen = 1,
	to_mipi_clkgen = 2,
	to_lvds_clkgen = 3,
	hdmi_clkgen = 4,
};
enum disptop_res_conv_iclk_cclk {
	res_conv_iclk = 0,
	res_conv_cclk = 1,
};
enum disptop_res_conv_oclk {
	res_conv_oclk = 1,
};

enum disptop_lcdif_clk {
	lcdif_pixel_clkx_n = 0,
	lcdif_pixel_clk = 1,
};
#define hdmi_spdif_clkgen 2
#define hdmi_spdif_clkout 0
#define hdmi_i_vclk_clkout 0
#define PHY_BASEADDR_DISPTOP_CLKGEN0_MODULE                                    \
	(PHY_BASEADDR_DISPLAYTOP_MODULE + other_addr_offset + 0x006000)
#define PHY_BASEADDR_DISPTOP_CLKGEN1_MODULE                                    \
	(PHY_BASEADDR_DISPLAYTOP_MODULE + other_addr_offset + 0x007000)
#define PHY_BASEADDR_DISPTOP_CLKGEN2_MODULE                                    \
	(PHY_BASEADDR_DISPLAYTOP_MODULE + other_addr_offset + 0x005000)
#define PHY_BASEADDR_DISPTOP_CLKGEN3_MODULE                                    \
	(PHY_BASEADDR_DISPLAYTOP_MODULE + other_addr_offset + 0x008000)
#define PHY_BASEADDR_DISPTOP_CLKGEN4_MODULE                                    \
	(PHY_BASEADDR_DISPLAYTOP_MODULE + other_addr_offset + 0x009000)

struct nx_disp_top_register_set {
	u32 resconv_mux_ctrl;
	u32 interconv_mux_ctrl;
	u32 mipi_mux_ctrl;
	u32 lvds_mux_ctrl;
	u32 hdmifixctrl0;
	u32 hdmisyncctrl0;
	u32 hdmisyncctrl1;
	u32 hdmisyncctrl2;
	u32 hdmisyncctrl3;
	u32 tftmpu_mux;
	u32 hdmifieldctrl;
	u32 greg0;
	u32 greg1;
	u32 greg2;
	u32 greg3;
	u32 greg4;
	u32 greg5;
};

extern int nx_disp_top_initialize(void);
extern u32 nx_disp_top_get_number_of_module(void);

extern u32 nx_disp_top_get_physical_address(void);
extern u32 nx_disp_top_get_size_of_register_set(void);
extern void nx_disp_top_set_base_address(void *base_address);
extern void *nx_disp_top_get_base_address(void);
extern int nx_disp_top_open_module(void);
extern int nx_disp_top_close_module(void);
extern int nx_disp_top_check_busy(void);

enum mux_index {
	primary_mlc = 0,
	secondary_mlc = 1,
	resolution_conv = 2,
};
enum prim_pad_mux_index {
	padmux_primary_mlc = 0,
	padmux_primary_mpu = 1,
	padmux_secondary_mlc = 2,
	padmux_resolution_conv = 3,
};

extern void nx_disp_top_set_resconvmux(int benb, u32 sel);
extern void nx_disp_top_set_hdmimux(int benb, u32 sel);
extern void nx_disp_top_set_mipimux(int benb, u32 sel);
extern void nx_disp_top_set_lvdsmux(int benb, u32 sel);
extern void nx_disp_top_set_primary_mux(u32 sel);
extern void nx_disp_top_hdmi_set_vsync_start(u32 sel);
extern void nx_disp_top_hdmi_set_vsync_hsstart_end(u32 start, u32 end);
extern void nx_disp_top_hdmi_set_hactive_start(u32 sel);
extern void nx_disp_top_hdmi_set_hactive_end(u32 sel);

extern void nx_disp_top_set_hdmifield(u32 enable, u32 init_val, u32 vsynctoggle,
			       u32 hsynctoggle, u32 vsyncclr, u32 hsyncclr,
			       u32 field_use, u32 muxsel);

enum padclk_config {
	padclk_clk = 0,
	padclk_inv_clk = 1,
	padclk_reserved_clk = 2,
	padclk_reserved_inv_clk = 3,
	padclk_clk_div2_0 = 4,
	padclk_clk_div2_90 = 5,
	padclk_clk_div2_180 = 6,
	padclk_clk_div2_270 = 7,
};

extern void nx_disp_top_set_padclock(u32 mux_index, u32 padclk_cfg);
extern void nx_disp_top_set_lcdif_enb(int enb);
extern void nx_disp_top_set_hdmifield(u32 enable, u32 init_val, u32 vsynctoggle,
			       u32 hsynctoggle, u32 vsyncclr, u32 hsyncclr,
			       u32 field_use, u32 muxsel);

#endif
