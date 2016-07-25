/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef FIMC_IS_DEVICE_4EC_REG_H
#define FIMC_IS_DEVICE_4EC_REG_H

static const u32 s5k4ec_init_reg1[] = {
/*==========================================================
// 00.History
//==========================================================
//2010 : EVT1.1
//20110429 : LSI CSE Standard
//==========================================================
//==========================================================
// 01.Start Setting
//=========================================================*/
	0xFCFCD000, /* 01.Start Setting */
	0x00100001, /* S/W Reset */
	0x10300000, /* contint_host_int */
	0x00140001, /* sw_load_complete-Release CORE(Arm)from reset state */
	0xFFFF000A, /* Delay 10ms */

};

static const u32 s5k4ec_init_reg2[] = {
/*==========================================================
//02.ETC Setting
//=========================================================*/
	0x0028D000, /* 02.ETC Setting */
	0x002A1082,
	0x0F120155,
	0x002A1084,
	0x0F120155,
	0x002A1086,
	0x0F120055,
	0x002A1088,
	0x0F120555,
	0x002A100E,
	0x0F120000,

/*==========================================================
// 03.Analog Setting & ASP Control
//=========================================================*/

/*This register is for FACTORY ONLY.
//If you change it without prior notification
//YOU are RESPONSIBLE for the FAILURE that will happen in the future */

	0x0028D000, /* 03.Analog Setting & ASP Control-1 */
	0x002A007A,
	0x0F120000, /* config_clk_setting */
	0x002AE406,
	0x0F120082, /* fadlc_disable */
	0x002AE410,
	0x0F123804, /* adlc_fadlc_filter_co */
	0x002AE41A,
	0x0F120010, /* adlc_ptune_total */
	0x002AF132,
	0x0F120200,
	0x002AF142,
	0x0F120200, /* 110404 AE haunting - from_LSI */
	0x002AE420,
	0x0F120003, /* adlc_fadlc_filter_refresh */
	0x0F120060, /* adlc_filter_level_diff_threshold */
	0x002AE42E,
	0x0F120004, /* adlc_qec */
	0x002AF400,
	0x0F125A3C, /* aig_shutter_width */
	0x0F120023, /* aig_cds_tune */
	0x0F128080, /* aig_cds_option */
	0x0F1203AF, /* aig_mx */
	0x0F12000A, /* aig_mode_en */
	0x0F12AA54, /* aig_ms */
	0x0F120040, /* aig_rmp_tune_1 */
	0x0F12464E, /* aig_rmp_tune_2 */
	0x0F120240, /* aig_bist_sig_width_e */
	0x0F120240, /* aig_bist_sig_width_o */
	0x0F120040, /* aig_dbs_bist */
	0x0F121000, /* aig_dbs_tune */
	0x0F1255FF, /* aig_bias_tune */
	0x0F12D000, /* aig_ref_tune_1 */
	0x0F120010, /* aig_ref_tune_2 */
	0x0F120202, /* aig_reg_tune_1 */
	0x0F120401, /* aig_reg_tune_2 */
	0x0F120022, /* aig_rosc_tune */
	0x0F120088, /* aig_dbr_tune_1 */
	0x0F12009F, /* aig_dbr_tune_2 */
	0x0F120000, /* aig_bist_en_cintr */
	0x0F121800, /* aig_vdec_tune */
	0x0F120088, /* aig_pmg_reg_tune */
	0x0F120000, /* aig_pmg_tune_1 */
	0x0F122428, /* aig_shutter_gap */
	0x0F120000, /* aig_atx_option */
	0x0F1203EE, /* aig_avg_half */
	0x0F120000, /* aig_hvs_test_reg */
	0x0F120000, /* aig_dbus_bist_auto */
	0x0F120000, /* aig_dbr_option */
	0x002AF552,
	0x0F120708, /* aig_1h_time_1 */
	0x0F12080C, /* aig_1h_time_2 */

/*==========================================================
// 04.Trap and Patch
//=========================================================*/
/* Start of Patch data */
	0x00287000, /* 05.Trap and Patch */
	0x002A3AF8,
	0x0F12B5F8,
	0x0F124B44,
	0x0F124944,
	0x0F124845,
	0x0F122200,
	0x0F12C008,
	0x0F126001,
	0x0F124944,
	0x0F124844,
	0x0F122401,
	0x0F12F000,
	0x0F12FCA4,
	0x0F124943,
	0x0F124844,
	0x0F122702,
	0x0F120022,
	0x0F12F000,
	0x0F12FC9E,
	0x0F120260,
	0x0F124C42,
	0x0F128020,
	0x0F122600,
	0x0F128066,
	0x0F124941,
	0x0F124841,
	0x0F126041,
	0x0F124941,
	0x0F124842,
	0x0F12003A,
	0x0F122503,
	0x0F12F000,
	0x0F12FC90,
	0x0F12483D,
	0x0F124940,
	0x0F1230C0,
	0x0F1263C1,
	0x0F124F3B,
	0x0F12483F,
	0x0F123F80,
	0x0F126438,
	0x0F12483E,
	0x0F12493F,
	0x0F126388,
	0x0F12002A,
	0x0F12493E,
	0x0F12483F,
	0x0F122504,
	0x0F12F000,
	0x0F12FC7F,
	0x0F12002A,
	0x0F12493D,
	0x0F12483E,
	0x0F122505,
	0x0F12F000,
	0x0F12F8A7,
	0x0F12483C,
	0x0F12002A,
	0x0F12493C,
	0x0F122506,
	0x0F121D80,
	0x0F12F000,
	0x0F12F8A0,
	0x0F124838,
	0x0F12002A,
	0x0F124939,
	0x0F122507,
	0x0F12300C,
	0x0F12F000,
	0x0F12F899,
	0x0F124835,
	0x0F12002A,
	0x0F124937,
	0x0F122508,
	0x0F123010,
	0x0F12F000,
	0x0F12F892,
	0x0F12002A,
	0x0F124935,
	0x0F124835,
	0x0F122509,
	0x0F12F000,
	0x0F12FC5E,
	0x0F12002A,
	0x0F124934,
	0x0F124834,
	0x0F12250A,
	0x0F12F000,
	0x0F12FC58,
	0x0F12002A,
	0x0F124933,
	0x0F124833,
	0x0F12250B,
	0x0F12F000,
	0x0F12FC52,
	0x0F12002A,
	0x0F124932,
	0x0F124832,
	0x0F12250C,
	0x0F12F000,
	0x0F12FC4C,
	0x0F12002A,
	0x0F124931,
	0x0F124831,
	0x0F12250D,
	0x0F12F000,
	0x0F12FC46,
	0x0F12002A,
	0x0F124930,
	0x0F124830,
	0x0F12250E,
	0x0F12F000,
	0x0F12FC40,
	0x0F12002A,
	0x0F12492F,
	0x0F12482F,
	0x0F12250F,
	0x0F12F000,
	0x0F12FC3A,
	0x0F128626,
	0x0F1220FF,
	0x0F121C40,
	0x0F128660,
	0x0F12482C,
	0x0F1264F8,
	0x0F12492C,
	0x0F12482D,
	0x0F122410,
	0x0F12002A,
	0x0F12F000,
	0x0F12FC2E,
	0x0F12492B,
	0x0F12482C,
	0x0F120022,
	0x0F12F000,
	0x0F12FC29,
	0x0F12BCF8,
	0x0F12BC08,
	0x0F124718,
	0x0F12019C,
	0x0F124EC2,
	0x0F1273FF,
	0x0F120000,
	0x0F121F90,
	0x0F127000,
	0x0F123CCD,
	0x0F127000,
	0x0F12E38B,
	0x0F120000,
	0x0F123D05,
	0x0F127000,
	0x0F12C3B1,
	0x0F120000,
	0x0F124780,
	0x0F127000,
	0x0F123D63,
	0x0F127000,
	0x0F120080,
	0x0F127000,
	0x0F123D9F,
	0x0F127000,
	0x0F12B49D,
	0x0F120000,
	0x0F123E4B,
	0x0F127000,
	0x0F123DFF,
	0x0F127000,
	0x0F12FFFF,
	0x0F1200FF,
	0x0F1217E0,
	0x0F127000,
	0x0F123FC7,
	0x0F127000,
	0x0F12053D,
	0x0F120000,
	0x0F120000,
	0x0F120A89,
	0x0F126CD2,
	0x0F120000,
	0x0F1202C9,
	0x0F120000,
	0x0F120000,
	0x0F120A9A,
	0x0F120000,
	0x0F1202D2,
	0x0F124015,
	0x0F127000,
	0x0F129E65,
	0x0F120000,
	0x0F124089,
	0x0F127000,
	0x0F127C49,
	0x0F120000,
	0x0F1240FD,
	0x0F127000,
	0x0F127C63,
	0x0F120000,
	0x0F124119,
	0x0F127000,
	0x0F128F01,
	0x0F120000,
	0x0F1241BB,
	0x0F127000,
	0x0F127F3F,
	0x0F120000,
	0x0F124249,
	0x0F127000,
	0x0F1298C5,
	0x0F120000,
	0x0F1243B5,
	0x0F127000,
	0x0F126099,
	0x0F120000,
	0x0F12430F, /* End of TnP */
	0x0F127000,
	0x0F124365,
	0x0F127000,
	0x0F12A70B,
	0x0F120000,
	0x0F124387,
	0x0F127000,
	0x0F12400D,
	0x0F120000,
	0x0F12B570,
	0x0F12000C,
	0x0F120015,
	0x0F120029,
	0x0F12F000,
	0x0F12FBD4,
	0x0F1249F8,
	0x0F1200A8,
	0x0F12500C,
	0x0F12BC70,
	0x0F12BC08,
	0x0F124718,
	0x0F126808,
	0x0F120400,
	0x0F120C00,
	0x0F126849,
	0x0F120409,
	0x0F120C09,
	0x0F124AF3,
	0x0F128992,
	0x0F122A00,
	0x0F12D00D,
	0x0F122300,
	0x0F121A89,
	0x0F12D400,
	0x0F12000B,
	0x0F120419,
	0x0F120C09,
	0x0F1223FF,
	0x0F1233C1,
	0x0F121810,
	0x0F124298,
	0x0F12D800,
	0x0F120003,
	0x0F120418,
	0x0F120C00,
	0x0F124AEB,
	0x0F128150,
	0x0F128191,
	0x0F124770,
	0x0F12B5F3,
	0x0F120004,
	0x0F12B081,
	0x0F129802,
	0x0F126800,
	0x0F120600,
	0x0F120E00,
	0x0F122201,
	0x0F120015,
	0x0F120021,
	0x0F123910,
	0x0F12408A,
	0x0F1240A5,
	0x0F124FE4,
	0x0F120016,
	0x0F122C10,
	0x0F12DA03,
	0x0F128839,
	0x0F1243A9,
	0x0F128039,
	0x0F12E002,
	0x0F128879,
	0x0F1243B1,
	0x0F128079,
	0x0F12F000,
	0x0F12FBA0,
	0x0F122C10,
	0x0F12DA03,
	0x0F128839,
	0x0F124329,
	0x0F128039,
	0x0F12E002,
	0x0F128879,
	0x0F124331,
	0x0F128079,
	0x0F1249DA,
	0x0F128809,
	0x0F122900,
	0x0F12D102,
	0x0F12F000,
	0x0F12FB99,
	0x0F122000,
	0x0F129902,
	0x0F126008,
	0x0F12BCFE,
	0x0F12BC08,
	0x0F124718,
	0x0F12B538,
	0x0F129C04,
	0x0F120015,
	0x0F12002A,
	0x0F129400,
	0x0F12F000,
	0x0F12FB94,
	0x0F124AD1,
	0x0F128811,
	0x0F122900,
	0x0F12D00F,
	0x0F128820,
	0x0F124281,
	0x0F12D20C,
	0x0F128861,
	0x0F128853,
	0x0F124299,
	0x0F12D200,
	0x0F121E40,
	0x0F120400,
	0x0F120C00,
	0x0F128020,
	0x0F128851,
	0x0F128061,
	0x0F124368,
	0x0F121840,
	0x0F126060,
	0x0F12BC38,
	0x0F12BC08,
	0x0F124718,
	0x0F12B5F8,
	0x0F120004,
	0x0F126808,
	0x0F120400,
	0x0F120C00,
	0x0F122201,
	0x0F120015,
	0x0F120021,
	0x0F123910,
	0x0F12408A,
	0x0F1240A5,
	0x0F124FBE,
	0x0F120016,
	0x0F122C10,
	0x0F12DA03,
	0x0F128839,
	0x0F1243A9,
	0x0F128039,
	0x0F12E002,
	0x0F128879,
	0x0F1243B1,
	0x0F128079,
	0x0F12F000,
	0x0F12FB6D,
	0x0F122C10,
	0x0F12DA03,
	0x0F128838,
	0x0F124328,
	0x0F128038,
	0x0F12E002,
	0x0F128878,
	0x0F124330,
	0x0F128078,
	0x0F1248B6,
	0x0F128800,
	0x0F120400,
	0x0F12D507,
	0x0F124BB5,
	0x0F127819,
	0x0F124AB5,
	0x0F127810,
	0x0F127018,
	0x0F127011,
	0x0F1249B4,
	0x0F128188,
	0x0F12BCF8,
	0x0F12BC08,
	0x0F124718,
	0x0F12B538,
	0x0F1248B2,
	0x0F124669,
	0x0F12F000,
	0x0F12FB58,
	0x0F1248B1,
	0x0F1249B0,
	0x0F1269C2,
	0x0F122400,
	0x0F1231A8,
	0x0F122A00,
	0x0F12D008,
	0x0F1261C4,
	0x0F12684A,
	0x0F126242,
	0x0F126282,
	0x0F12466B,
	0x0F12881A,
	0x0F126302,
	0x0F12885A,
	0x0F126342,
	0x0F126A02,
	0x0F122A00,
	0x0F12D00A,
	0x0F126204,
	0x0F126849,
	0x0F126281,
	0x0F12466B,
	0x0F128819,
	0x0F126301,
	0x0F128859,
	0x0F126341,
	0x0F1249A5,
	0x0F1288C9,
	0x0F1263C1,
	0x0F12F000,
	0x0F12FB40,
	0x0F12E7A6,
	0x0F12B5F0,
	0x0F12B08B,
	0x0F1220FF,
	0x0F121C40,
	0x0F1249A1,
	0x0F1289CC,
	0x0F124E9E,
	0x0F126AB1,
	0x0F124284,
	0x0F12D101,
	0x0F12489F,
	0x0F126081,
	0x0F126A70,
	0x0F120200,
	0x0F12F000,
	0x0F12FB37,
	0x0F120400,
	0x0F120C00,
	0x0F124A96,
	0x0F128A11,
	0x0F129109,
	0x0F122101,
	0x0F120349,
	0x0F124288,
	0x0F12D200,
	0x0F120001,
	0x0F124A92,
	0x0F128211,
	0x0F124D97,
	0x0F128829,
	0x0F129108,
	0x0F124A8B,
	0x0F122303,
	0x0F123222,
	0x0F121F91,
	0x0F12F000,
	0x0F12FB28,
	0x0F128028,
	0x0F12488E,
	0x0F124987,
	0x0F126BC2,
	0x0F126AC0,
	0x0F124282,
	0x0F12D201,
	0x0F128CC8,
	0x0F128028,
	0x0F1288E8,
	0x0F129007,
	0x0F122240,
	0x0F124310,
	0x0F1280E8,
	0x0F122000,
	0x0F120041,
	0x0F12194B,
	0x0F12001E,
	0x0F123680,
	0x0F128BB2,
	0x0F12AF04,
	0x0F12527A,
	0x0F124A7D,
	0x0F12188A,
	0x0F128897,
	0x0F1283B7,
	0x0F1233A0,
	0x0F12891F,
	0x0F12AE01,
	0x0F125277,
	0x0F128A11,
	0x0F128119,
	0x0F121C40,
	0x0F120400,
	0x0F120C00,
	0x0F122806,
	0x0F12D3E9,
	0x0F12F000,
	0x0F12FB09,
	0x0F12F000,
	0x0F12FB0F,
	0x0F124F79,
	0x0F1237A8,
	0x0F122800,
	0x0F12D10A,
	0x0F121FE0,
	0x0F1238FD,
	0x0F12D001,
	0x0F121CC0,
	0x0F12D105,
	0x0F124874,
	0x0F128829,
	0x0F123818,
	0x0F126840,
	0x0F124348,
	0x0F126078,
	0x0F124972,
	0x0F126878,
	0x0F126B89,
	0x0F124288,
	0x0F12D300,
	0x0F120008,
	0x0F126078,
	0x0F122000,
	0x0F120041,
	0x0F12AA04,
	0x0F125A53,
	0x0F12194A,
	0x0F12269C,
	0x0F1252B3,
	0x0F12AB01,
	0x0F125A59,
	0x0F1232A0,
	0x0F128111,
	0x0F121C40,
	0x0F120400,
	0x0F120C00,
	0x0F122806,
	0x0F12D3F0,
	0x0F124965,
	0x0F129809,
	0x0F128208,
	0x0F129808,
	0x0F128028,
	0x0F129807,
	0x0F1280E8,
	0x0F121FE0,
	0x0F1238FD,
	0x0F12D13B,
	0x0F124D64,
	0x0F1289E8,
	0x0F121FC1,
	0x0F1239FF,
	0x0F12D136,
	0x0F124C5F,
	0x0F128AE0,
	0x0F12F000,
	0x0F12FADE,
	0x0F120006,
	0x0F128B20,
	0x0F12F000,
	0x0F12FAE2,
	0x0F129000,
	0x0F126AA1,
	0x0F126878,
	0x0F121809,
	0x0F120200,
	0x0F12F000,
	0x0F12FAB5,
	0x0F120400,
	0x0F120C00,
	0x0F120022,
	0x0F123246,
	0x0F120011,
	0x0F12310A,
	0x0F122305,
	0x0F12F000,
	0x0F12FAB2,
	0x0F1266E8,
	0x0F126B23,
	0x0F120002,
	0x0F120031,
	0x0F120018,
	0x0F12F000,
	0x0F12FAD3,
	0x0F12466B,
	0x0F128518,
	0x0F126EEA,
	0x0F126B60,
	0x0F129900,
	0x0F12F000,
	0x0F12FACC,
	0x0F12466B,
	0x0F128558,
	0x0F120029,
	0x0F12980A,
	0x0F123170,
	0x0F12F000,
	0x0F12FACD,
	0x0F120028,
	0x0F123060,
	0x0F128A02,
	0x0F124946,
	0x0F123128,
	0x0F12808A,
	0x0F128A42,
	0x0F1280CA,
	0x0F128A80,
	0x0F128108,
	0x0F12B00B,
	0x0F12BCF0,
	0x0F12BC08,
	0x0F124718,
	0x0F12B570,
	0x0F122400,
	0x0F124D46,
	0x0F124846,
	0x0F128881,
	0x0F124846,
	0x0F128041,
	0x0F122101,
	0x0F128001,
	0x0F12F000,
	0x0F12FABC,
	0x0F124842,
	0x0F123820,
	0x0F128BC0,
	0x0F12F000,
	0x0F12FABF,
	0x0F124B42,
	0x0F12220D,
	0x0F120712,
	0x0F1218A8,
	0x0F128806,
	0x0F1200E1,
	0x0F1218C9,
	0x0F1281CE,
	0x0F128846,
	0x0F12818E,
	0x0F128886,
	0x0F12824E,
	0x0F1288C0,
	0x0F128208,
	0x0F123508,
	0x0F12042D,
	0x0F120C2D,
	0x0F121C64,
	0x0F120424,
	0x0F120C24,
	0x0F122C07,
	0x0F12D3EC,
	0x0F12E658,
	0x0F12B510,
	0x0F124834,
	0x0F124C34,
	0x0F1288C0,
	0x0F128060,
	0x0F122001,
	0x0F128020,
	0x0F124831,
	0x0F123820,
	0x0F128BC0,
	0x0F12F000,
	0x0F12FA9C,
	0x0F1288E0,
	0x0F124A31,
	0x0F122800,
	0x0F12D003,
	0x0F124930,
	0x0F128849,
	0x0F122900,
	0x0F12D009,
	0x0F122001,
	0x0F1203C0,
	0x0F128050,
	0x0F1280D0,
	0x0F122000,
	0x0F128090,
	0x0F128110,
	0x0F12BC10,
	0x0F12BC08,
	0x0F124718,
	0x0F128050,
	0x0F128920,
	0x0F1280D0,
	0x0F128960,
	0x0F120400,
	0x0F121400,
	0x0F128090,
	0x0F1289A1,
	0x0F120409,
	0x0F121409,
	0x0F128111,
	0x0F1289E3,
	0x0F128A24,
	0x0F122B00,
	0x0F12D104,
	0x0F1217C3,
	0x0F120F5B,
	0x0F121818,
	0x0F1210C0,
	0x0F128090,
	0x0F122C00,
	0x0F12D1E6,
	0x0F1217C8,
	0x0F120F40,
	0x0F121840,
	0x0F1210C0,
	0x0F128110,
	0x0F12E7E0,
	0x0F12B510,
	0x0F12000C,
	0x0F124919,
	0x0F122204,
	0x0F126820,
	0x0F125E8A,
	0x0F120140,
	0x0F121A80,
	0x0F120280,
	0x0F128849,
	0x0F12F000,
	0x0F12FA6A,
	0x0F126020,
	0x0F12E7D2,
	0x0F1238D4,
	0x0F127000,
	0x0F1217D0,
	0x0F127000,
	0x0F125000,
	0x0F12D000,
	0x0F121100,
	0x0F12D000,
	0x0F12171A,
	0x0F127000,
	0x0F124780,
	0x0F127000,
	0x0F122FCA,
	0x0F127000,
	0x0F122FC5,
	0x0F127000,
	0x0F122FC6,
	0x0F127000,
	0x0F122ED8,
	0x0F127000,
	0x0F122BD0,
	0x0F127000,
	0x0F1217E0,
	0x0F127000,
	0x0F122DE8,
	0x0F127000,
	0x0F1237E0,
	0x0F127000,
	0x0F12210C,
	0x0F127000,
	0x0F121484,
	0x0F127000,
	0x0F12A006,
	0x0F120000,
	0x0F120724,
	0x0F127000,
	0x0F12A000,
	0x0F12D000,
	0x0F122270,
	0x0F127000,
	0x0F122558,
	0x0F127000,
	0x0F12146C,
	0x0F127000,
	0x0F12B510,
	0x0F12000C,
	0x0F1249C7,
	0x0F122208,
	0x0F126820,
	0x0F125E8A,
	0x0F120140,
	0x0F121A80,
	0x0F120280,
	0x0F1288C9,
	0x0F12F000,
	0x0F12FA30,
	0x0F126020,
	0x0F12E798,
	0x0F12B5FE,
	0x0F12000C,
	0x0F126825,
	0x0F126866,
	0x0F1268A0,
	0x0F129001,
	0x0F1268E7,
	0x0F121BA8,
	0x0F1242B5,
	0x0F12DA00,
	0x0F121B70,
	0x0F129000,
	0x0F1249BB,
	0x0F1248BC,
	0x0F12884A,
	0x0F128843,
	0x0F12435A,
	0x0F122304,
	0x0F125ECB,
	0x0F120A92,
	0x0F1218D2,
	0x0F1202D2,
	0x0F120C12,
	0x0F1288CB,
	0x0F128880,
	0x0F124343,
	0x0F120A98,
	0x0F122308,
	0x0F125ECB,
	0x0F1218C0,
	0x0F1202C0,
	0x0F120C00,
	0x0F120411,
	0x0F120400,
	0x0F121409,
	0x0F121400,
	0x0F121A08,
	0x0F1249B0,
	0x0F1239E0,
	0x0F126148,
	0x0F129801,
	0x0F123040,
	0x0F127880,
	0x0F122800,
	0x0F12D103,
	0x0F129801,
	0x0F120029,
	0x0F12F000,
	0x0F12FA03,
	0x0F128839,
	0x0F129800,
	0x0F124281,
	0x0F12D814,
	0x0F128879,
	0x0F129800,
	0x0F124281,
	0x0F12D20C,
	0x0F129801,
	0x0F120029,
	0x0F12F000,
	0x0F12F9FF,
	0x0F129801,
	0x0F120029,
	0x0F12F000,
	0x0F12F9FB,
	0x0F129801,
	0x0F120029,
	0x0F12F000,
	0x0F12F9F7,
	0x0F12E003,
	0x0F129801,
	0x0F120029,
	0x0F12F000,
	0x0F12F9F2,
	0x0F129801,
	0x0F120032,
	0x0F120039,
	0x0F12F000,
	0x0F12F9F5,
	0x0F126020,
	0x0F12E5D0,
	0x0F12B57C,
	0x0F12489A,
	0x0F12A901,
	0x0F120004,
	0x0F12F000,
	0x0F12F979,
	0x0F12466B,
	0x0F1288D9,
	0x0F128898,
	0x0F124B95,
	0x0F123346,
	0x0F121E9A,
	0x0F12F000,
	0x0F12F9ED,
	0x0F124894,
	0x0F124992,
	0x0F123812,
	0x0F123140,
	0x0F128A42,
	0x0F12888B,
	0x0F1218D2,
	0x0F128242,
	0x0F128AC2,
	0x0F1288C9,
	0x0F121851,
	0x0F1282C1,
	0x0F120020,
	0x0F124669,
	0x0F12F000,
	0x0F12F961,
	0x0F12488D,
	0x0F12214D,
	0x0F128301,
	0x0F122196,
	0x0F128381,
	0x0F12211D,
	0x0F123020,
	0x0F128001,
	0x0F12F000,
	0x0F12F9DB,
	0x0F12F000,
	0x0F12F9E1,
	0x0F124888,
	0x0F124C88,
	0x0F126E00,
	0x0F1260E0,
	0x0F12466B,
	0x0F128818,
	0x0F128859,
	0x0F120025,
	0x0F121A40,
	0x0F123540,
	0x0F1261A8,
	0x0F12487F,
	0x0F129900,
	0x0F123060,
	0x0F12F000,
	0x0F12F9D9,
	0x0F12466B,
	0x0F128819,
	0x0F121DE0,
	0x0F1230F9,
	0x0F128741,
	0x0F128859,
	0x0F128781,
	0x0F122000,
	0x0F1271A0,
	0x0F1274A8,
	0x0F12BC7C,
	0x0F12BC08,
	0x0F124718,
	0x0F12B5F8,
	0x0F120005,
	0x0F126808,
	0x0F120400,
	0x0F120C00,
	0x0F12684A,
	0x0F120412,
	0x0F120C12,
	0x0F12688E,
	0x0F1268CC,
	0x0F124970,
	0x0F12884B,
	0x0F124343,
	0x0F120A98,
	0x0F122304,
	0x0F125ECB,
	0x0F1218C0,
	0x0F1202C0,
	0x0F120C00,
	0x0F1288CB,
	0x0F124353,
	0x0F120A9A,
	0x0F122308,
	0x0F125ECB,
	0x0F1218D1,
	0x0F1202C9,
	0x0F120C09,
	0x0F122701,
	0x0F12003A,
	0x0F1240AA,
	0x0F129200,
	0x0F12002A,
	0x0F123A10,
	0x0F124097,
	0x0F122D10,
	0x0F12DA06,
	0x0F124A69,
	0x0F129B00,
	0x0F128812,
	0x0F12439A,
	0x0F124B67,
	0x0F12801A,
	0x0F12E003,
	0x0F124B66,
	0x0F12885A,
	0x0F1243BA,
	0x0F12805A,
	0x0F120023,
	0x0F120032,
	0x0F12F000,
	0x0F12F981,
	0x0F122D10,
	0x0F12DA05,
	0x0F124961,
	0x0F129A00,
	0x0F128808,
	0x0F124310,
	0x0F128008,
	0x0F12E003,
	0x0F12485E,
	0x0F128841,
	0x0F124339,
	0x0F128041,
	0x0F124D5B,
	0x0F122000,
	0x0F123580,
	0x0F1288AA,
	0x0F125E30,
	0x0F122100,
	0x0F12F000,
	0x0F12F98D,
	0x0F128030,
	0x0F122000,
	0x0F1288AA,
	0x0F125E20,
	0x0F122100,
	0x0F12F000,
	0x0F12F986,
	0x0F128020,
	0x0F12E587,
	0x0F12B510,
	0x0F12F000,
	0x0F12F989,
	0x0F124A53,
	0x0F128D50,
	0x0F122800,
	0x0F12D007,
	0x0F12494E,
	0x0F1231C0,
	0x0F12684B,
	0x0F124950,
	0x0F124283,
	0x0F12D202,
	0x0F128D90,
	0x0F1281C8,
	0x0F12E6A0,
	0x0F128DD0,
	0x0F1281C8,
	0x0F12E69D,
	0x0F12B5F8,
	0x0F12F000,
	0x0F12F97E,
	0x0F124D49,
	0x0F128E28,
	0x0F122800,
	0x0F12D01F,
	0x0F124E49,
	0x0F124844,
	0x0F1268B4,
	0x0F126800,
	0x0F124284,
	0x0F12D903,
	0x0F121A21,
	0x0F120849,
	0x0F121847,
	0x0F12E006,
	0x0F124284,
	0x0F12D203,
	0x0F121B01,
	0x0F120849,
	0x0F121A47,
	0x0F12E000,
	0x0F120027,
	0x0F120020,
	0x0F12493B,
	0x0F123120,
	0x0F127A0C,
	0x0F122C00,
	0x0F12D004,
	0x0F120200,
	0x0F120039,
	0x0F12F000,
	0x0F12F8C3,
	0x0F128668,
	0x0F122C00,
	0x0F12D000,
	0x0F1260B7,
	0x0F12E54D,
	0x0F1220FF,
	0x0F121C40,
	0x0F128668,
	0x0F12E549,
	0x0F12B510,
	0x0F12000C,
	0x0F126820,
	0x0F120400,
	0x0F120C00,
	0x0F124933,
	0x0F128E0A,
	0x0F122A00,
	0x0F12D003,
	0x0F128E49,
	0x0F120200,
	0x0F12F000,
	0x0F12F8AD,
	0x0F126020,
	0x0F120400,
	0x0F120C00,
	0x0F12E661,
	0x0F12B570,
	0x0F12680C,
	0x0F124D2F,
	0x0F120020,
	0x0F126F29,
	0x0F12F000,
	0x0F12F946,
	0x0F126F69,
	0x0F121D20,
	0x0F12F000,
	0x0F12F942,
	0x0F124827,
	0x0F128E00,
	0x0F122800,
	0x0F12D006,
	0x0F124922,
	0x0F122214,
	0x0F123168,
	0x0F120008,
	0x0F12383C,
	0x0F12F000,
	0x0F12F93F,
	0x0F12E488,
	0x0F12B5F8,
	0x0F120004,
	0x0F124D24,
	0x0F128B68,
	0x0F122800,
	0x0F12D012,
	0x0F124823,
	0x0F128A00,
	0x0F1206C0,
	0x0F12D50E,
	0x0F124822,
	0x0F127800,
	0x0F122800,
	0x0F12D00A,
	0x0F12481D,
	0x0F126FC1,
	0x0F122000,
	0x0F12F000,
	0x0F12F923,
	0x0F128B28,
	0x0F122201,
	0x0F122180,
	0x0F12F000,
	0x0F12F92C,
	0x0F128328,
	0x0F122101,
	0x0F12000D,
	0x0F120020,
	0x0F123810,
	0x0F124081,
	0x0F1240A5,
	0x0F124F11,
	0x0F12000E,
	0x0F122C10,
	0x0F12DA03,
	0x0F128838,
	0x0F1243A8,
	0x0F128038,
	0x0F12E002,
	0x0F128878,
	0x0F1243B0,
	0x0F128078,
	0x0F12F000,
	0x0F12F920,
	0x0F122C10,
	0x0F12DA03,
	0x0F128838,
	0x0F124328,
	0x0F128038,
	0x0F12E4EF,
	0x0F128878,
	0x0F124330,
	0x0F128078,
	0x0F12E4EB,
	0x0F122558,
	0x0F127000,
	0x0F122AB8,
	0x0F127000,
	0x0F12145E,
	0x0F127000,
	0x0F122698,
	0x0F127000,
	0x0F122BB8,
	0x0F127000,
	0x0F122998,
	0x0F127000,
	0x0F121100,
	0x0F12D000,
	0x0F124780,
	0x0F127000,
	0x0F12E200,
	0x0F12D000,
	0x0F12210C,
	0x0F127000,
	0x0F120000,
	0x0F127000,
	0x0F12308C,
	0x0F127000,
	0x0F12B040,
	0x0F12D000,
	0x0F123858,
	0x0F127000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F121789,
	0x0F120001,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F1216F1,
	0x0F120001,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F12C3B1,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F12C36D,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F12F6D7,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F12B49D,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F127EDF,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F12448D,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12F004,
	0x0F12E51F,
	0x0F1229EC,
	0x0F120001,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F122EF1,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F12EE03,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F12A58B,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F127C49,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F127C63,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F122DB7,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F12EB3D,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F12F061,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F12F0EF,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12F004,
	0x0F12E51F,
	0x0F122824,
	0x0F120001,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F128EDD,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F128DCB,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F128E17,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F1298C5,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F127C7D,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F127E31,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F127EAB,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F127501,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F12F63F,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F123D0B,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F1229BF,
	0x0F120001,
	0x0F124778,
	0x0F1246C0,
	0x0F12F004,
	0x0F12E51F,
	0x0F1226D8,
	0x0F120001,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F12306B,
	0x0F120000,
	0x0F124778,
	0x0F1246C0,
	0x0F12C000,
	0x0F12E59F,
	0x0F12FF1C,
	0x0F12E12F,
	0x0F126099,
	0x0F120000, /* End of Patch Data(Last : 7000449Eh) */
/*
// End of Patch Data(Last : 7000465Ah)
// Total Size 2916 (0B64)
// Addr : 3AF8  Size : 2914(B62h)

// TNP_USER_MBCV_CONTROL
// TNP_4EC_MBR_TUNE
// TNP_4EC_FORBIDDEN_TUNE
// TNP_AF_FINESEARCH_DRIVEBACK
// TNP_FLASH_ALG
// TNP_GAS_ALPHA_OTP
// TNP_AWB_MODUL_COMP
// TNP_AWB_INIT_QUEUE
// TNP_AWB_GRID_LOWBR
// TNP_AWB_GRID_MODULECOMP
// TNP_ADLC_TUNE
// TNP_1FRAME_AE
// TNP_TG_OFF_CFG_CHG_IN_SPOOF_MODE
*/

/*==========================================================
// OTP setting
//=========================================================*/
	0x002A0722, /* OTP block */
	0x0F120100,
	0x002A0726,
	0x0F120001,
	0x002A08D6,
	0x0F120001,
	0x002A146E,
	0x0F120000,
	0x002A08DC,
	0x0F120000,
	0x0028D000,
	0x002A1000,
	0x0F120001,

/*==========================================================
//04.Gas_Anti Shading_Otp
//=========================================================*/
	0x00287000,
	0x002A08B4,
	0x0F120001, /* wbt_bUseOutdoorASH */

/* Refer Mon_AWB_RotGain */
	0x002A08BC,
	0x0F1200C0, /* TVAR_ash_AwbAshCord_0_ 2300K */
	0x0F1200DF, /* TVAR_ash_AwbAshCord_1_ 2750K */
	0x0F120100, /* TVAR_ash_AwbAshCord_2_ 3300K */
	0x0F120125, /* TVAR_ash_AwbAshCord_3_ 4150K */
	0x0F12015F, /* TVAR_ash_AwbAshCord_4_ 5250K */
	0x0F12017C, /* TVAR_ash_AwbAshCord_5_ 6400K */
	0x0F120194, /* TVAR_ash_AwbAshCord_6_ 7500K */

/* GAS Alpha Table */
	0x002A08F6,
	0x0F124000, /* TVAR_ash_GASAlpha_0__0_ R  // 2300K */
	0x0F124000, /* TVAR_ash_GASAlpha_0__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_0__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_0__3_ B */
	0x0F124000, /* TVAR_ash_GASAlpha_1__0_ R  // 2750K */
	0x0F124000, /* TVAR_ash_GASAlpha_1__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_1__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_1__3_ B */
	0x0F124100, /* TVAR_ash_GASAlpha_2__0_ R  // 3300K */
	0x0F124000, /* TVAR_ash_GASAlpha_2__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_2__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_2__3_ B */
	0x0F123F80, /* TVAR_ash_GASAlpha_3__0_ R  // 4150K */
	0x0F124000, /* TVAR_ash_GASAlpha_3__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_3__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_3__3_ B */
	0x0F124080, /* TVAR_ash_GASAlpha_4__0_ R  // 5250K */
	0x0F124000, /* TVAR_ash_GASAlpha_4__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_4__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_4__3_ B */
	0x0F124100, /* TVAR_ash_GASAlpha_5__0_ R  // 6400K */
	0x0F124000, /* TVAR_ash_GASAlpha_5__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_5__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_5__3_ B */
	0x0F124150, /* TVAR_ash_GASAlpha_6__0_ R  // 7500K */
	0x0F124000, /* TVAR_ash_GASAlpha_6__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_6__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_6__3_ B */

/* Outdoor GAS Alpha */
	0x0F124600, /* TVAR_ash_GASOutdoorAlpha_0_ R */
	0x0F124000, /* TVAR_ash_GASOutdoorAlpha_1_ GR */
	0x0F124000, /* TVAR_ash_GASOutdoorAlpha_2_ GB */
	0x0F124000, /* TVAR_ash_GASOutdoorAlpha_3_ B */
	0x002A08F4,
	0x0F120001, /* ash_bUseGasAlpha */

/*==========================================================
// 07. Analog Setting 2
//=========================================================*/
/* This register is for FACTORY ONLY.
//If you change it without prior notification
//YOU are RESPONSIBLE for the FAILURE that will happen in the future
//For subsampling Size */

	0x00287000, /* REG_ANALOG_SETTING2 */
	0x002A18BC,
	0x0F120004, /* senHal_ContPtrs_senModesDataArr_0_ */
	0x0F1205B6, /* senHal_ContPtrs_senModesDataArr_1_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_2_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_3_ */
	0x0F120001, /* senHal_ContPtrs_senModesDataArr_4_ */
	0x0F1205BA, /* senHal_ContPtrs_senModesDataArr_5_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_6_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_7_ */
	0x0F120007, /* senHal_ContPtrs_senModesDataArr_8_ */
	0x0F1205BA, /* senHal_ContPtrs_senModesDataArr_9_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_10_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_11_ */
	0x0F1201F4, /* senHal_ContPtrs_senModesDataArr_12_ */
	0x0F12024E, /* senHal_ContPtrs_senModesDataArr_13_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_14_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_15_ */
	0x0F1201F4, /* senHal_ContPtrs_senModesDataArr_16_ */
	0x0F1205B6, /* senHal_ContPtrs_senModesDataArr_17_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_18_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_19_ */
	0x0F1201F4, /* senHal_ContPtrs_senModesDataArr_20_ */
	0x0F1205BA, /* senHal_ContPtrs_senModesDataArr_21_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_22_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_23_ */
	0x0F1201F4, /* senHal_ContPtrs_senModesDataArr_24_ */
	0x0F12024F, /* senHal_ContPtrs_senModesDataArr_25_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_26_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_27_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_28_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_29_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_30_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_31_ */
	0x0F120075, /* senHal_ContPtrs_senModesDataArr_32_ */
	0x0F1200CF, /* senHal_ContPtrs_senModesDataArr_33_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_34_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_35_ */
	0x0F120075, /* senHal_ContPtrs_senModesDataArr_36_ */
	0x0F1200D6, /* senHal_ContPtrs_senModesDataArr_37_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_38_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_39_ */
	0x0F120004, /* senHal_ContPtrs_senModesDataArr_40_ */
	0x0F1201F4, /* senHal_ContPtrs_senModesDataArr_41_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_42_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_43_ */
	0x0F1200F0, /* senHal_ContPtrs_senModesDataArr_44_ */
	0x0F1201F4, /* senHal_ContPtrs_senModesDataArr_45_ */
	0x0F12029E, /* senHal_ContPtrs_senModesDataArr_46_ */
	0x0F1205B2, /* senHal_ContPtrs_senModesDataArr_47_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_48_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_49_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_50_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_51_ */
	0x0F1201F8, /* senHal_ContPtrs_senModesDataArr_52_ */
	0x0F120228, /* senHal_ContPtrs_senModesDataArr_53_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_54_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_55_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_56_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_57_ */
	0x0F120208, /* senHal_ContPtrs_senModesDataArr_58_ */
	0x0F120238, /* senHal_ContPtrs_senModesDataArr_59_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_60_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_61_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_62_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_63_ */
	0x0F120218, /* senHal_ContPtrs_senModesDataArr_64_ */
	0x0F120238, /* senHal_ContPtrs_senModesDataArr_65_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_66_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_67_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_68_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_69_ */
	0x0F120001, /* senHal_ContPtrs_senModesDataArr_70_ */
	0x0F120009, /* senHal_ContPtrs_senModesDataArr_71_ */
	0x0F1200DE, /* senHal_ContPtrs_senModesDataArr_72_ */
	0x0F1205C0, /* senHal_ContPtrs_senModesDataArr_73_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_74_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_75_ */
	0x0F1200DF, /* senHal_ContPtrs_senModesDataArr_76_ */
	0x0F1200E4, /* senHal_ContPtrs_senModesDataArr_77_ */
	0x0F1201F8, /* senHal_ContPtrs_senModesDataArr_78_ */
	0x0F1201FD, /* senHal_ContPtrs_senModesDataArr_79_ */
	0x0F1205B6, /* senHal_ContPtrs_senModesDataArr_80_ */
	0x0F1205BB, /* senHal_ContPtrs_senModesDataArr_81_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_82_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_83_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_84_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_85_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_86_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_87_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_88_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_89_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_90_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_91_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_92_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_93_ */
	0x0F1201F8, /* senHal_ContPtrs_senModesDataArr_94_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_95_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_96_ */
	0x0F120077, /* senHal_ContPtrs_senModesDataArr_97_ */
	0x0F12007E, /* senHal_ContPtrs_senModesDataArr_98_ */
	0x0F12024F, /* senHal_ContPtrs_senModesDataArr_99_ */
	0x0F12025E, /* senHal_ContPtrs_senModesDataArr_100_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_101_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_102_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_103_ */
	0x0F120000, /* senHal_ContPtrs_senModesDataArr_104_ */

	0x0F120004, /* senHal_ContPtrs_senAvgModesDataArr_0_ */
	0x0F1209D1, /* senHal_ContPtrs_senAvgModesDataArr_1_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_2_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_3_ */
	0x0F120001, /* senHal_ContPtrs_senAvgModesDataArr_4_ */
	0x0F1209D5, /* senHal_ContPtrs_senAvgModesDataArr_5_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_6_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_7_ */
	0x0F120008, /* senHal_ContPtrs_senAvgModesDataArr_8_ */
	0x0F1209D5, /* senHal_ContPtrs_senAvgModesDataArr_9_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_10_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_11_ */
	0x0F1202AA, /* senHal_ContPtrs_senAvgModesDataArr_12_ */
	0x0F120326, /* senHal_ContPtrs_senAvgModesDataArr_13_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_14_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_15_ */
	0x0F1202AA, /* senHal_ContPtrs_senAvgModesDataArr_16_ */
	0x0F1209D1, /* senHal_ContPtrs_senAvgModesDataArr_17_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_18_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_19_ */
	0x0F1202AA, /* senHal_ContPtrs_senAvgModesDataArr_20_ */
	0x0F1209D5, /* senHal_ContPtrs_senAvgModesDataArr_21_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_22_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_23_ */
	0x0F1202AA, /* senHal_ContPtrs_senAvgModesDataArr_24_ */
	0x0F120327, /* senHal_ContPtrs_senAvgModesDataArr_25_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_26_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_27_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_28_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_29_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_30_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_31_ */
	0x0F120008, /* senHal_ContPtrs_senAvgModesDataArr_32_ */
	0x0F120084, /* senHal_ContPtrs_senAvgModesDataArr_33_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_34_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_35_ */
	0x0F120008, /* senHal_ContPtrs_senAvgModesDataArr_36_ */
	0x0F12008D, /* senHal_ContPtrs_senAvgModesDataArr_37_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_38_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_39_ */
	0x0F120008, /* senHal_ContPtrs_senAvgModesDataArr_40_ */
	0x0F1202AA, /* senHal_ContPtrs_senAvgModesDataArr_41_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_42_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_43_ */
	0x0F1200AA, /* senHal_ContPtrs_senAvgModesDataArr_44_ */
	0x0F1202AA, /* senHal_ContPtrs_senAvgModesDataArr_45_ */
	0x0F1203AD, /* senHal_ContPtrs_senAvgModesDataArr_46_ */
	0x0F1209CD, /* senHal_ContPtrs_senAvgModesDataArr_47_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_48_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_49_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_50_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_51_ */
	0x0F1202AE, /* senHal_ContPtrs_senAvgModesDataArr_52_ */
	0x0F1202DE, /* senHal_ContPtrs_senAvgModesDataArr_53_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_54_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_55_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_56_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_57_ */
	0x0F1202BE, /* senHal_ContPtrs_senAvgModesDataArr_58_ */
	0x0F1202EE, /* senHal_ContPtrs_senAvgModesDataArr_59_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_60_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_61_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_62_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_63_ */
	0x0F1202CE, /* senHal_ContPtrs_senAvgModesDataArr_64_ */
	0x0F1202EE, /* senHal_ContPtrs_senAvgModesDataArr_65_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_66_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_67_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_68_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_69_ */
	0x0F120001, /* senHal_ContPtrs_senAvgModesDataArr_70_ */
	0x0F120009, /* senHal_ContPtrs_senAvgModesDataArr_71_ */
	0x0F120095, /* senHal_ContPtrs_senAvgModesDataArr_72_ */
	0x0F1209DB, /* senHal_ContPtrs_senAvgModesDataArr_73_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_74_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_75_ */
	0x0F120096, /* senHal_ContPtrs_senAvgModesDataArr_76_ */
	0x0F12009B, /* senHal_ContPtrs_senAvgModesDataArr_77_ */
	0x0F1202AE, /* senHal_ContPtrs_senAvgModesDataArr_78_ */
	0x0F1202B3, /* senHal_ContPtrs_senAvgModesDataArr_79_ */
	0x0F1209D1, /* senHal_ContPtrs_senAvgModesDataArr_80_ */
	0x0F1209D6, /* senHal_ContPtrs_senAvgModesDataArr_81_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_82_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_83_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_84_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_85_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_86_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_87_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_88_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_89_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_90_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_91_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_92_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_93_ */
	0x0F1202AE, /* senHal_ContPtrs_senAvgModesDataArr_94_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_95_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_96_ */
	0x0F120009, /* senHal_ContPtrs_senAvgModesDataArr_97_ */
	0x0F120010, /* senHal_ContPtrs_senAvgModesDataArr_98_ */
	0x0F120327, /* senHal_ContPtrs_senAvgModesDataArr_99_ */
	0x0F120336, /* senHal_ContPtrs_senAvgModesDataArr_100_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_101_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_102_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_103_ */
	0x0F120000, /* senHal_ContPtrs_senAvgModesDataArr_104_ */

	0x002A1AF8,
	0x0F125A3C, /* senHal_TuneStr_AngTuneData1_2_ register at subsampling */
	0x002A1896,
	0x0F120002, /* senHal_SamplingType */
	0x0F120000, /* senHal_SamplingMode 0000 : 2PLA, 0001 : 4PLA */
	0x0F120003, /* senHal_PLAOption [0] VPLA enable, [1] HPLA enable */

	0x002A1B00, /* Add for low lux flash from LSI */
	0x0F12F428,
	0x0F12FFFF,
	0x0F120000,

	0x002A189E,
	0x0F120FB0, /* senHal_ExpMinPixels */

	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F120A20, /* senHal_uMinColsBin */
	0x0F120AB0, /* senHal_uMinColsNoBin */

	0x002A1AEA,
	0x0F128080, /* senHal_SubF404Tune */
	0x0F120080, /* senHal_FullF404Tune */
	0x002A1AE0,
	0x0F120000, /* senHal_bSenAAC */

	0x002A1A72,
	0x0F120000, /* senHal_bSRX SRX off */
	0x002A18A2,
	0x0F120004, /* senHal_NExpLinesCheckFine extend Forbidden area line */
	0x002A1A6A,
	0x0F12009A, /* senHal_usForbiddenRightOfs extend right Forbidden
			area line */
	0x002A385E,
	0x0F12024C, /* Mon_Sen_uExpPixelsOfs */

	0x002A0EE6,
	0x0F120000, /* setot_bUseDigitalHbin */
	0x002A1B2A,
	0x0F120300, /* senHal_TuneStr2_usAngTuneGainTh */
	0x0F1200D6, /* senHal_TuneStr2_AngTuneF4CA_0_ */
	0x0F12008D, /* senHal_TuneStr2_AngTuneF4CA_1_ */
	0x0F1200CF, /* senHal_TuneStr2_AngTuneF4C2_0_ */
	0x0F120084, /* senHal_TuneStr2_AngTuneF4C2_1_ */

	0x002A0EEC,
	0x0F12000A, /* setot_usSubSXBayerOffset */
	0x0F120012, /* setot_usFullXBayerOffset */

/*==========================================================
// 12.AF Setting
//=========================================================*/
/* AF interface setting */
	0x00287000,
	0x002A01FC,
	0x0F120001, /* REG_TC_IPRM_LedGpio */

	0x002A01FE,
	0x0F120003, /* REG_TC_IPRM_CM_Init_AfModeType VCM IIC */
	0x0F120000, /* REG_TC_IPRM_CM_Init_PwmConfig1 */
	0x002A0204,
	0x0F120061, /* REG_TC_IPRM_CM_Init_GpioConfig1 AF Enable GPIO 6 */
	0x002A020C,
	0x0F122F0C, /* REG_TC_IPRM_CM_Init_Mi2cBits */
	0x0F120190, /* REG_TC_IPRM_CM_Init_Mi2cRateKhz IIC Speed */

/* AF Windo,w Settings */
	0x002A0294,
	0x0F120100, /* REG_TC_AF_FstWinStartX */
	0x0F1200E3, /* REG_TC_AF_FstWinStartY */
	0x0F120200, /* REG_TC_AF_FstWinSizeX */
	0x0F120238, /* REG_TC_AF_FstWinSizeY */
	0x0F1201C6, /* LSI_Cho AF Window Center from_LSI */
	0x0F120166, /* REG_TC_AF_ScndWinStartY */
	0x0F120074, /* LSI_Cho AF Fail when Size change from_LSI */
	0x0F120132, /* REG_TC_AF_ScndWinSizeY */
	0x0F120001, /* REG_TC_AF_WinSizesUpdated */

/* 2nd search setting */
	0x002A070E,
	0x0F1200C0,
	0x002A071E,
	0x0F120000,
	0x002A163C,
	0x0F120000,
	0x002A1648,
	0x0F129002,
	0x002A1652,
	0x0F120002,
	0x0F120000,
	0x002A15E0,
	0x0F120802,

/* Peak Threshold */
	0x002A164C,
	0x0F120003,
	0x002A163E,
	0x0F1200C2,
	0x0F120098,

/* Home Pos */
	0x002A15D4,
	0x0F120000, /* af_pos_usHomePos */
	0x0F12D000, /* af_pos_usLowConfPos */

/* AF statistics */
	0x002A169A,
	0x0F12FF95,
	0x002A166A,
	0x0F120280,
	0x002A1676,
	0x0F1203A0,
	0x0F120320,
	0x002A16BC,
	0x0F120030,
	0x002A16E0,
	0x0F120060,
	0x002A16D4,
	0x0F120010,
	0x002A1656,
	0x0F120000,
	0x002A15E6,
	0x0F12003C,

	0x0F120018,
	0x0F120024,
	0x0F120028,
	0x0F12002C,
	0x0F12002E,
	0x0F120032,
	0x0F120036,
	0x0F12003A,
	0x0F120040,
	0x0F120048,
	0x0F120050,
	0x0F120058,
	0x0F120060,
	0x0F120068,
	0x0F120070,
	0x0F120078,
	0x0F120080,
	0x0F120088,
	0x0F120090,
	0x0F120098,
	0x0F1200A0,
	0x0F1200A8,
	0x0F1200B0,
	0x0F1200B8,
	0x0F1200C0,
	0x0F1200C8,

	0x002A1722,
	0x0F128000,
	0x0F120006,
	0x0F123FF0,
	0x0F1203E8,
	0x0F120000,
	0x0F120008,
	0x0F120001,
	0x0F120004,
	0x0F120080,
	0x0F1200C0,
	0x0F1200E0,
	0x0F120000,

	0x002A028C,
	0x0F120003,

/*==========================================================
// 14.AWB-BASIC setting
//=========================================================*/

	/* AWB init Start point */
	0x002A145E,
	0x0F120580, /* awbb_GainsInit_0_ */
	0x0F120428, /* awbb_GainsInit_1_ */
	0x0F1207B0,

	/* White Locus */
	0x002A11F0,
	0x0F120120, /* awbb_IntcR */
	0x0F120121, /* awbb_IntcB */

	/* Indoor Zone */
	0x002A101C,
	0x0F12036E,
	0x0F1203AC,
	0x0F120324,
	0x0F1203A2,
	0x0F1202EA,
	0x0F120386,
	0x0F1202B0,
	0x0F120358,
	0x0F120284,
	0x0F120326,
	0x0F120266,
	0x0F1202F4,
	0x0F12024E,
	0x0F1202D0,
	0x0F120236,
	0x0F1202B2,
	0x0F120220,
	0x0F120298,
	0x0F12020A,
	0x0F120282,
	0x0F1201F4,
	0x0F12026E,
	0x0F1201DE,
	0x0F120258,
	0x0F1201BA,
	0x0F120242,
	0x0F1201A4,
	0x0F120226,
	0x0F1201C2,
	0x0F1201F0,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120005,
	0x002A1070,
	0x0F12000F,
	0x002A1074,
	0x0F12011A,

	/* Outdoor Zone */
	0x002A1078,
	0x0F120258,
	0x0F120284,
	0x0F12023C,
	0x0F120290,
	0x0F120222,
	0x0F120290,
	0x0F12021A,
	0x0F120286,
	0x0F12021A,
	0x0F120278,
	0x0F12021C,
	0x0F120264,
	0x0F120230,
	0x0F120244,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,

	0x0F120004,
	0x002A10AC,
	0x0F120007,
	0x002A10B0,
	0x0F120216,

	0x002A10B4, /* LowBr Zone */
	0x0F120394,
	0x0F1203C2,
	0x0F120312,
	0x0F1203EA,
	0x0F120270,
	0x0F1203F0,
	0x0F120204,
	0x0F1203C2,
	0x0F1201C2,
	0x0F120366,
	0x0F120194,
	0x0F120312,
	0x0F120166,
	0x0F1202D4,
	0x0F120142,
	0x0F120294,
	0x0F12011C,
	0x0F120258,
	0x0F12015A,
	0x0F120214,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,

	0x0F120006,
	0x002A10E8,
	0x0F12000A,
	0x002A10EC,
	0x0F120120,

	0x002A10F0,
	0x0F120380,
	0x0F120000,
	0x0F120168,
	0x0F120000,
	0x0F122D90,
	0x0F120000,

	0x002A1464,
	0x0F120008,
	0x0F120190,
	0x0F1200A0,

	0x002A1228,
	0x0F1200C0,
	0x002A122C,
	0x0F120010,
	0x002A122A,
	0x0F120010,

	0x002A120A,
	0x0F1205D5,
	0x002A120E,
	0x0F120000,

	0x0F120771,
	0x0F1203A4,
	0x0F120036,
	0x0F12002A,

	0x002A1278,
	0x0F12FEF7,
	0x0F120021,
	0x0F120DAC, /* Outdoor TH : about 2500lux */
	0x0F120DAC, /* Outdoor TH : about 2500lux */
	0x0F12018F,
	0x0F120096,
	0x0F12000E,
	0x002A1224,
	0x0F120032,
	0x0F12001E,
	0x0F1200C0,
	0x0F120010,
	0x0F120002,
	0x002A2BA4,
	0x0F120004,

	0x002A146C,
	0x0F120002,

	/* grid const */
	0x002A1434,
	0x0F1202CE,
	0x0F120347,
	0x0F1203C2,
	0x0F121060,
	0x0F121000,
	0x0F121150,
	0x0F1211E5,
	0x0F12120A,
	0x0F121296,
	0x0F1200AB,
	0x0F1200BF,
	0x0F1200D2,
	0x0F120093,

	/* Indoor Grid Offset_LYA */
	0x002A13A4,
	0x0F12FFD8,
	0x0F12FFF0,
	0x0F12FFF5,
	0x0F12FFC9,
	0x0F120014,
	0x0F120028,
	0x0F12FFD8,
	0x0F12FFF0,
	0x0F12FFF5,
	0x0F12FFC9,
	0x0F120014,
	0x0F120028,
	0x0F12FFD8,
	0x0F12FFF0,
	0x0F12FFF5,
	0x0F12FFC9,
	0x0F120014,
	0x0F120028,
	0x0F12FFE2,
	0x0F12FFDD,
	0x0F12FFB5,
	0x0F12FF51,
	0x0F12FE70,
	0x0F12FE70,
	0x0F12FFE2,
	0x0F12FFDD,
	0x0F12FFB5,
	0x0F12FF51,
	0x0F12FE70,
	0x0F12FE70,
	0x0F12FFE2,
	0x0F12FFDD,
	0x0F12FFB5,
	0x0F12FF51,
	0x0F12FE70,
	0x0F12FE70,

	/* Outdoor Grid Offset_LYA */
	0x0F120000,
	0x0F120000,
	0x0F120010,
	0x0F120000,
	0x0F120000,
	0x0F120000,

	0x0F120000,
	0x0F120000,
	0x0F120010,
	0x0F120000,
	0x0F120000,
	0x0F120000,

	0x0F120000,
	0x0F120000,
	0x0F120010,
	0x0F120000,
	0x0F120000,
	0x0F120000,

	0x0F120002,
	0x0F120002,
	0x0F120002,
	0x0F120000,
	0x0F120000,
	0x0F120000,

	0x0F120002,
	0x0F120002,
	0x0F120002,
	0x0F120000,
	0x0F120000,
	0x0F120000,

	0x0F120002,
	0x0F120002,
	0x0F120002,
	0x0F120000,
	0x0F120000,
	0x0F120000,

	0x002A1208,
	0x0F120020,

	0x002A144E,
	0x0F120000,
	0x0F12FFE0,
	0x0F120000,

/*==========================================================
// 10.Clock Setting                          PCLK  X
//=========================================================*/

/* Input Clock (Mclk) */
	0x002A01F8,
	0x0F125DC0, /* REG_TC_IPRM_InClockLSBs */
	0x002A0212,
	0x0F120000, /* REG_TC_IPRM_UseNPviClocks */
	0x0F120002, /* REG_TC_IPRM_UseNMipiClocks */
	0x0F120002, /* REG_TC_IPRM_NumberOfMipiLanes */

/* System Clock & Output clock (Pclk) */
	0x002A021A,
	0x0F123A98, /* REG_TC_IPRM_OpClk4KHz_0 //SCLK : 60Mhz */
	0x0F122904, /* REG_TC_IPRM_MinOutRate4KHz_0 */
	0x0F1229FE, /* REG_TC_IPRM_MaxOutRate4KHz_0 */
	0x0F124F1A, /* REG_TC_IPRM_MinOutRate4KHz_1 */
	0x0F122904, /* REG_TC_IPRM_MinOutRate4KHz_1 //PCLK Min : 43Mhz */
	0x0F1229FE, /* REG_TC_IPRM_MaxOutRate4KHz_1 //PCLK Max : 44Mhz */

/*==========================================================
// 11.Auto Flicker Detection
//=========================================================*/

	0x002A0F30,
	0x0F120001, /* AFC_D_ConvAccelerPower */
	0x002A0F2A,
	0x0F120000, /* AFC_Default BIT[0] 1:60Hz 0:50Hz */
	0x002A04E6,
	0x0F12077F, /* REG_TC_DBG 7F: 60Hz  5F:50Hz */
	0x002A0F20,
	0x0F120002, /* AFC_SmallP_MaxFrames default :2 20131104 */
	0x002A0F08,
	0x0F120260, /* AFC_NewS_XSumDiff default :2 20131104 */
	0x002A0F2E,
	0x0F120004, /* AFC_D_MaxHitsCnt default :2 20131104 */
	0x002A0F0A,
	0x0F120110, /* AFC_NewS_XBatchVarLimit default :2 20131104 */
	0x002A0F12,
	0x0F120030, /* AFC_D_AbsEnergyThresh default :2 20131104 */
	0x002A0F1C,
	0x0F120700, /* AFC_D_B_Coef default :2 20131104 */
	0x002A0F14,
	0x0F120010, /* AFC_D_RelEnergyThresh default :2 20131104 */

/*==========================================================
// 12.AE Setting
//=========================================================*/
/* AE Target */
	0x002A1484,
	0x0F12003C, /* TVAR_ae_BrAve */

/*ae_StatMode bit[3] BLC has to be bypassed
* to prevent AE weight change especially
* backlight scene */
	0x002A148A,
	0x0F12000F, /* ae_StatMode */

	0x002A058C,
	0x0F123520,

	0x0F120000,
	0x0F12C350,
	0x0F120000,
	0x0F123520,
	0x0F120000,
	0x0F12C350,
	0x0F120000,
	0x002A059C,
	0x0F120470,
	0x0F120C00,
	0x0F120100,
	0x0F121000,

	0x002A0544,
	0x0F120111,
	0x0F1200EF,

	0x002A0608,
	0x0F120001,
	0x0F120001,
	0x0F120800,
	0x0F120100,

	0x0F120001,
	0x0F120000,
	0x0F120A3C,
	0x0F120000,
	0x0F120D05,
	0x0F120000,
	0x0F124008,
	0x0F120000,
	0x0F127000,
	0x0F120000,
	0x0F129E00,
	0x0F120000,
	0x0F12AD00,
	0x0F120001,
	0x0F12F1D4,
	0x0F120002,
	0x0F12DC00,
	0x0F120005,
	0x0F12DC00,
	0x0F120005,

	0x002A0638,
	0x0F120001,
	0x0F120000,
	0x0F120A3C,
	0x0F120000,
	0x0F120D05,
	0x0F120000,
	0x0F123408,
	0x0F120000,
	0x0F123408,
	0x0F120000,
	0x0F126810,
	0x0F120000,
	0x0F128214,
	0x0F120000,
	0x0F12C350,
	0x0F120000,
	0x0F12C350,
	0x0F120000,
	0x0F12C350,
	0x0F120000,

	0x002A0660,
	0x0F120650,
	0x0F120100,

/* Lei Control */
	0x002A06B8,
	0x0F12802C,
	0x0F120005, /* lt_uMaxLei */

	0x002A05D0,
	0x0F120000,

/*==========================================================
// 13.AE Weight (Normal)
//=========================================================*/
	0x002A1492,
	0x0F120100,
	0x0F120101,
	0x0F120101,
	0x0F120001,
	0x0F120101,
	0x0F120201,
	0x0F120102,
	0x0F120101,
	0x0F120101,
	0x0F120202,
	0x0F120202,
	0x0F120101,
	0x0F120201,
	0x0F120302,
	0x0F120203,
	0x0F120102,
	0x0F120201,
	0x0F120302,
	0x0F120203,
	0x0F120102,
	0x0F120201,
	0x0F120202,
	0x0F120202,
	0x0F120102,
	0x0F120101,
	0x0F120202,
	0x0F120202,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,

/*==========================================================
// 14.Flash Setting
//=========================================================*/

	0x00287000,
	0x002A0484,
	0x0F120002, /* REG_TC_FLS_Mode */

	0x002A183A,
	0x0F120001, /* _afl_usCounter */

	0x002A17F6,
	0x0F120260, /* _afl_DefaultWPr //5C 54 54 */
	0x0F120210, /* _afl_DefaultWPb //28 30 28 */

	0x002A1840,
	0x0F120001, /* _afl_bFlsMode */

	0x0F120100, /* _afl_FlsAFIn_0_ */
	0x0F120120, /* _afl_FlsAFIn_1_ */
	0x0F120180, /* _afl_FlsAFIn_2_ */
	0x0F120200, /* _afl_FlsAFIn_3_ */
	0x0F120400, /* _afl_FlsAFIn_4_ */
	0x0F120800, /* _afl_FlsAFIn_5_ */
	0x0F120A00, /* _afl_FlsAFIn_6_ */
	0x0F121000, /* _afl_FlsAFIn_7_ */

	0x0F120100, /* _afl_FlsAFOut_0_ */
	0x0F1200A0, /* _afl_FlsAFOut_1_ */
	0x0F120090, /* _afl_FlsAFOut_2_ */
	0x0F120080, /* _afl_FlsAFOut_3_ */
	0x0F120070, /* _afl_FlsAFOut_4_ */
	0x0F120045, /* _afl_FlsAFOut_5_ */
	0x0F120030, /* _afl_FlsAFOut_6_ */
	0x0F120010, /* _afl_FlsAFOut_7_ */

	0x002A1884,
	0x0F120100, /* _afl_FlsNBOut_0_ */
	0x0F120100, /* _afl_FlsNBOut_1_ */
	0x0F120100, /* _afl_FlsNBOut_2_ */
	0x0F120100, /* _afl_FlsNBOut_3_ */
	0x0F120100, /* _afl_FlsNBOut_4_ */
	0x0F120100, /* _afl_FlsNBOut_5_ */
	0x0F120100, /* _afl_FlsNBOut_6_ */
	0x0F120100, /* _afl_FlsNBOut_7_ */

	0x002A1826,

	0x0F120100, /* _afl_FlashWP_Weight2_0_ */
	0x0F1200C0, /* _afl_FlashWP_Weight2_1_ */
	0x0F120080, /* _afl_FlashWP_Weight2_2_ */
	0x0F12000A, /* _afl_FlashWP_Weight2_3_ */
	0x0F120000, /* _afl_FlashWP_Weight2_4_ */

	0x0F120030, /* _afl_FlashWP_Lei_Thres2_0_ */
	0x0F120040, /* _afl_FlashWP_Lei_Thres2_1_ */
	0x0F120048, /* _afl_FlashWP_Lei_Thres2_2_ */
	0x0F120050, /* _afl_FlashWP_Lei_Thres2_3_ */
	0x0F120060, /* _afl_FlashWP_Lei_Thres2_4_ */

	0x002A4784,
	0x0F1200A0, /* TNP_Regs_FlsWeightRIn  weight tune start in */
	0x0F1200C0,
	0x0F1200D0,
	0x0F120100,
	0x0F120200,
	0x0F120300,

	0x0F120088, /* TNP_Regs_FlsWeightROut  weight tune start out */
	0x0F1200B0,
	0x0F1200C0,
	0x0F120100,
	0x0F120200,
	0x0F120300,

	0x002A479C,

	0x0F120120, /* Fls  BRIn */
	0x0F120150,
	0x0F120200,

	0x0F12003C, /* Fls  BROut */
	0x0F12003B,
	0x0F120026, /* brightness */

/*==========================================================
// 15.CCM Setting
//=========================================================*/
	0x002A08A6,
	0x0F1200C0,
	0x0F120100,
	0x0F120125,
	0x0F12015F,
	0x0F12017C,
	0x0F120194,

	0x0F120001, /* wbt_bUseOutdoorCCM */

	0x002A0898,
	0x0F124800,
	0x0F127000, /* TVAR_wbt_pBaseCcms */
	0x002A08A0,
	0x0F1248D8,
	0x0F127000, /* TVAR_wbt_pOutdoorCcm */

	0x002A4800, /* Horizon */
	0x0F120208, /* _wbt_pBaseCcms[0] */
	0x0F12FFB5, /* _wbt_pBaseCcms[1] */
	0x0F12FFE8, /* _wbt_pBaseCcms[2] */
	0x0F12FF99, /* _wbt_pBaseCcms[3] */
	0x0F1200EB, /* _wbt_pBaseCcms[4] */
	0x0F12FFAD, /* _wbt_pBaseCcms[5] */
	0x0F120022, /* _wbt_pBaseCcms[6] */
	0x0F12FFEA, /* _wbt_pBaseCcms[7] */
	0x0F1201C2, /* _wbt_pBaseCcms[8] */
	0x0F1200C6, /* _wbt_pBaseCcms[9] */
	0x0F120095, /* _wbt_pBaseCcms[10] */
	0x0F12FEFD, /* _wbt_pBaseCcms[11] */
	0x0F120206, /* _wbt_pBaseCcms[12] */
	0x0F12FF7F, /* _wbt_pBaseCcms[13] */
	0x0F120191, /* _wbt_pBaseCcms[14] */
	0x0F12FF06, /* _wbt_pBaseCcms[15] */
	0x0F1201BA, /* _wbt_pBaseCcms[16] */
	0x0F120108, /* _wbt_pBaseCcms[17] */

	0x0F120208, /* TVAR_wbt_pBaseCcms[18] inca A */
	0x0F12FFB5, /* _wbt_pBaseCcms[19] */
	0x0F12FFE8, /* _wbt_pBaseCcms[20] */
	0x0F12FF82, /* _wbt_pBaseCcms[21] */
	0x0F120115, /* _wbt_pBaseCcms[22] */
	0x0F12FF9A, /* _wbt_pBaseCcms[23] */
	0x0F12FFFB, /* _wbt_pBaseCcms[24] */
	0x0F12FFB4, /* _wbt_pBaseCcms[25] */
	0x0F12021E, /* _wbt_pBaseCcms[26] */
	0x0F1200C6, /* _wbt_pBaseCcms[27] */
	0x0F120095, /* _wbt_pBaseCcms[28] */
	0x0F12FEFD, /* _wbt_pBaseCcms[29] */
	0x0F120206, /* _wbt_pBaseCcms[30] */
	0x0F12FF7F, /* _wbt_pBaseCcms[31] */
	0x0F120191, /* _wbt_pBaseCcms[32] */
	0x0F12FF06, /* _wbt_pBaseCcms[33] */
	0x0F1201BA, /* _wbt_pBaseCcms[34] */
	0x0F120108, /* _wbt_pBaseCcms[35] */

	0x0F120208, /* TVAR_wbt_pBaseCcms[36] WW*/
	0x0F12FFB5, /* _wbt_pBaseCcms[37] */
	0x0F12FFE8, /* _wbt_pBaseCcms[38] */
	0x0F12FF99, /* _wbt_pBaseCcms[39] */
	0x0F1200EB, /* _wbt_pBaseCcms[40] */
	0x0F12FFAD, /* _wbt_pBaseCcms[41] */
	0x0F120022, /* _wbt_pBaseCcms[42] */
	0x0F12FFEA, /* _wbt_pBaseCcms[43] */
	0x0F1201C2, /* _wbt_pBaseCcms[44] */
	0x0F1200C6, /* _wbt_pBaseCcms[45] */
	0x0F120095, /* _wbt_pBaseCcms[46] */
	0x0F12FEFD, /* _wbt_pBaseCcms[47] */
	0x0F120206, /* _wbt_pBaseCcms[48] */
	0x0F12FF7F, /* _wbt_pBaseCcms[49] */
	0x0F120191, /* _wbt_pBaseCcms[50] */
	0x0F12FF06, /* _wbt_pBaseCcms[51] */
	0x0F1201BA, /* _wbt_pBaseCcms[52] */
	0x0F120108, /* _wbt_pBaseCcms[53] */

	0x0F120204, /* _wbt_pBaseCcms[54] // CW */
	0x0F12FFB2, /* _wbt_pBaseCcms[55] */
	0x0F12FFF5, /* _wbt_pBaseCcms[56] */
	0x0F12FEF1, /* _wbt_pBaseCcms[57] */
	0x0F12014E, /* _wbt_pBaseCcms[58] */
	0x0F12FF18, /* _wbt_pBaseCcms[59] */
	0x0F12FFE6, /* _wbt_pBaseCcms[60] */
	0x0F12FFDD, /* _wbt_pBaseCcms[61] */
	0x0F1201B2, /* _wbt_pBaseCcms[62] */
	0x0F1200F2, /* _wbt_pBaseCcms[63] */
	0x0F1200CA, /* _wbt_pBaseCcms[64] */
	0x0F12FF48, /* _wbt_pBaseCcms[65] */
	0x0F120151, /* _wbt_pBaseCcms[66] */
	0x0F12FF50, /* _wbt_pBaseCcms[67] */
	0x0F120147, /* _wbt_pBaseCcms[68] */
	0x0F12FF75, /* _wbt_pBaseCcms[69] */
	0x0F120187, /* _wbt_pBaseCcms[70] */
	0x0F1201BF, /* _wbt_pBaseCcms[71] */

	0x0F120204, /* _wbt_pBaseCcms[72] // D50 */
	0x0F12FFB2, /* _wbt_pBaseCcms[73] */
	0x0F12FFF5, /* _wbt_pBaseCcms[74] */
	0x0F12FEF1, /* _wbt_pBaseCcms[75] */
	0x0F12014E, /* _wbt_pBaseCcms[76] */
	0x0F12FF18, /* _wbt_pBaseCcms[77] */
	0x0F12FFD9, /* _wbt_pBaseCcms[78] */
	0x0F12FFBA, /* _wbt_pBaseCcms[79] */
	0x0F1201D4, /* _wbt_pBaseCcms[80] */
	0x0F1200F2, /* _wbt_pBaseCcms[81] */
	0x0F1200CA, /* _wbt_pBaseCcms[82] */
	0x0F12FF48, /* _wbt_pBaseCcms[83] */
	0x0F120151, /* _wbt_pBaseCcms[84] */
	0x0F12FF50, /* _wbt_pBaseCcms[85] */
	0x0F120147, /* _wbt_pBaseCcms[86] */
	0x0F12FF75, /* _wbt_pBaseCcms[87] */
	0x0F120187, /* _wbt_pBaseCcms[88] */
	0x0F1201BF, /* _wbt_pBaseCcms[89] */

	0x0F120204, /* _wbt_pBaseCcms[90] // D65 */
	0x0F12FFB2, /* _wbt_pBaseCcms[91] */
	0x0F12FFF5, /* _wbt_pBaseCcms[92] */
	0x0F12FEF1, /* _wbt_pBaseCcms[93] */
	0x0F12014E, /* _wbt_pBaseCcms[94] */
	0x0F12FF18, /* _wbt_pBaseCcms[95] */
	0x0F12FFD9, /* _wbt_pBaseCcms[96] */
	0x0F12FFBA, /* _wbt_pBaseCcms[97] */
	0x0F1201D4, /* _wbt_pBaseCcms[98] */
	0x0F1200F2, /* _wbt_pBaseCcms[99] */
	0x0F1200CA, /* _wbt_pBaseCcms[100] */
	0x0F12FF48, /* _wbt_pBaseCcms[101] */
	0x0F120151, /* _wbt_pBaseCcms[102] */
	0x0F12FF50, /* _wbt_pBaseCcms[103] */
	0x0F120147, /* _wbt_pBaseCcms[104] */
	0x0F12FF75, /* _wbt_pBaseCcms[105] */
	0x0F120187, /* _wbt_pBaseCcms[106] */
	0x0F1201BF, /* _wbt_pBaseCcms[107] */

	0x0F1201E9, /* _wbt_pOutdoorCcm[0] */
	0x0F12FFA8, /* _wbt_pOutdoorCcm[1] */
	0x0F12FFC9, /* _wbt_pOutdoorCcm[2] */
	0x0F12FE6E, /* _wbt_pOutdoorCcm[3] */
	0x0F12012A, /* _wbt_pOutdoorCcm[4] */
	0x0F12FF56, /* _wbt_pOutdoorCcm[5] */
	0x0F120002, /* _wbt_pOutdoorCcm[6] */
	0x0F120042, /* _wbt_pOutdoorCcm[7] */
	0x0F1201E6, /* _wbt_pOutdoorCcm[8] */
	0x0F1200B6, /* _wbt_pOutdoorCcm[9] */
	0x0F120115, /* _wbt_pOutdoorCcm[10] */
	0x0F12FF40, /* _wbt_pOutdoorCcm[11] */
	0x0F1201EC, /* _wbt_pOutdoorCcm[12] */
	0x0F12FF6E, /* _wbt_pOutdoorCcm[13] */
	0x0F120181, /* _wbt_pOutdoorCcm[14] */
	0x0F12FEC1, /* _wbt_pOutdoorCcm[15] */
	0x0F120140, /* _wbt_pOutdoorCcm[16] */
	0x0F120176, /* _wbt_pOutdoorCcm[17] */

/*==========================================================
// 16.GAMMA
//=========================================================*/
	/* RGB Indoor Gamma */
	0x002A0734,  /* R */
	0x0F120001,
	0x0F120003,
	0x0F12000F,
	0x0F120028,
	0x0F120066,
	0x0F1200D9,
	0x0F120138,
	0x0F120163,
	0x0F120189,
	0x0F1201C6,
	0x0F1201F8,
	0x0F120222,
	0x0F120247,
	0x0F120282,
	0x0F1202B5,
	0x0F12030F,
	0x0F12035F,
	0x0F1203A2,
	0x0F1203D8,
	0x0F1203FF,
	0x0F120001, /* G */
	0x0F120003,
	0x0F12000F,
	0x0F120028,
	0x0F120066,
	0x0F1200D9,
	0x0F120138,
	0x0F120163,
	0x0F120189,
	0x0F1201C6,
	0x0F1201F8,
	0x0F120222,
	0x0F120247,
	0x0F120282,
	0x0F1202B5,
	0x0F12030F,
	0x0F12035F,
	0x0F1203A2,
	0x0F1203D8,
	0x0F1203FF,
	0x0F120001, /* B */
	0x0F120003,
	0x0F12000F,
	0x0F120028,
	0x0F120066,
	0x0F1200D9,
	0x0F120138,
	0x0F120163,
	0x0F120189,
	0x0F1201C6,
	0x0F1201F8,
	0x0F120222,
	0x0F120247,
	0x0F120282,
	0x0F1202B5,
	0x0F12030F,
	0x0F12035F,
	0x0F1203A2,
	0x0F1203D8,
	0x0F1203FF,

	0x0F120007, /* RED */
	0x0F120012,
	0x0F120020,
	0x0F120038,
	0x0F120071,
	0x0F1200DA,
	0x0F120137,
	0x0F120161,
	0x0F120187,
	0x0F1201C3,
	0x0F1201FE,
	0x0F12021B,
	0x0F120245,
	0x0F12028C,
	0x0F1202CB,
	0x0F120325,
	0x0F120365,
	0x0F12039A,
	0x0F1203C7,
	0x0F1203F4,

	0x0F120005,
	0x0F120010,
	0x0F12001E,
	0x0F120036,
	0x0F12006F,
	0x0F1200D8,
	0x0F120135,
	0x0F12015F,
	0x0F120185,
	0x0F1201C1,
	0x0F1201F3,
	0x0F120220,
	0x0F12024A,
	0x0F120291,
	0x0F1202D0,
	0x0F12032A,
	0x0F12036A,
	0x0F12039F,
	0x0F1203CC,
	0x0F1203F9,

	0x0F120003, /* Blue */
	0x0F12000E,
	0x0F12001C,
	0x0F120034,
	0x0F12006D,
	0x0F1200D6,
	0x0F120133,
	0x0F12015D,
	0x0F120183,
	0x0F1201BF,
	0x0F1201F5,
	0x0F120222,
	0x0F12024C,
	0x0F120293,
	0x0F1202D2,
	0x0F12032C,
	0x0F12036C,
	0x0F1203A1,
	0x0F1203CE,
	0x0F1203FB,

/*==========================================================
// 17.AFIT
//=========================================================*/
	0x002A0944, /* Noise Index setting */
	0x0F120050, /* afit_uNoiseIndInDoor */
	0x0F1200B0, /* afit_uNoiseIndInDoor */
	0x0F120196, /* afit_uNoiseIndInDoor */
	0x0F120245, /* afit_uNoiseIndInDoor */
	0x0F120300, /* afit_uNoiseIndInDoor */
	0x002A0976,
	0x0F120070, /* afit_usGamutTh */
	0x0F120005, /* afit_usNeargrayOffset */
	0x0F120000, /* afit_bUseSenBpr */
	0x0F1201CC, /* afit_usBprThr_0_ */
	0x0F1201CC, /* afit_usBprThr_1_ */
	0x0F1201CC, /* afit_usBprThr_2_ */
	0x0F1201CC, /* afit_usBprThr_3_ */
	0x0F1201CC, /* afit_usBprThr_4_ */
	0x0F120180, /* afit_NIContrastAFITValue */
	0x0F120196, /* afit_NIContrastTh */
	0x002A0938,
	0x0F120000, /* on/off AFIT by NB option */
	0x0F120014, /* SARR_uNormBrInDoor */
	0x0F1200D2, /* SARR_uNormBrInDoor */
	0x0F120384, /* SARR_uNormBrInDoor */
	0x0F1207D0, /* SARR_uNormBrInDoor */
	0x0F121388, /* SARR_uNormBrInDoor */
	0x002A098C,
	0x0F12FFFB, /* 7000098C AFIT16_BRIGHTNESS */
	0x0F120000, /* 7000098E AFIT16_CONTRAST */
	0x0F120000, /* 70000990 AFIT16_SATURATION */
	0x0F120000, /* 70000992 AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000994 AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000996 AFIT16_bnr_edge_high */
	0x0F120064, /* 70000998 AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 7000099A AFIT16_postdmsc_iHighBright */
	0x0F12005F, /* 7000099C AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 7000099E AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 700009A0 AFIT16_postdmsc_iTune */
	0x0F120040, /* 700009A2 AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 700009A4 AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 700009A6 AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 700009A8 AFIT16_yuvemix_mPosRanges_0 */
	0x0F120040, /* 700009AA AFIT16_yuvemix_mPosRanges_1 */
	0x0F1200A0, /* 700009AC AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 700009AE AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 700009B0 AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 700009B2 AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F123604, /* 700009B4 AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F12032A, /* 700009B6 AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120103, /* 700009B8 AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F121205, /* 700009BA AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F12400D, /* 700009BC AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120080, /* 700009BE AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F122080, /* 700009C0 AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F123840, /* 700009C2 AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120638, /* 700009C4 AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 700009C6 AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 700009C8 AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 700009CA AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120000, /* 700009CC AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120400, /* 700009CE AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12245A, /* 700009D0 AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F12102A, /* 700009D2 AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 700009D4 AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F120600, /* 700009D6 AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 700009D8 AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120505, /* 700009DA AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 700009DC AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 700009DE AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 700009E0 AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F122828, /* 700009E2 AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F12040F, /* 700009E4 AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 700009E6 AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F120800, /* 700009E8 AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F121804, /* 700009EA AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 700009EC AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 700009EE AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 700009F0 AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 700009F2 AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 700009F4 AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 700009F6 AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120004, /* 700009F8 AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 700009FA AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 700009FC AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 700009FE AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000A00 AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000A02 AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000A04 AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120205, /* 70000A06 AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000A08 AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000A0A AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120306, /* 70000A0C AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120407, /* 70000A0E AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F121C04, /* 70000A10 AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F120214, /* 70000A12 AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121002, /* 70000A14 AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120610, /* 70000A16 AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000A18 AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F122818, /* 70000A1A AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F120080, /* 70000A1C AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120350, /* 70000A1E AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000A20 AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000A22 AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000A24 AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F123141, /* 70000A26 AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000A28 AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F123140, /* 70000A2A AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000A2C AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000A2E AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000A30 AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000A32 AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F123601, /* 70000A34 AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F12242A, /* 70000A36 AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F123660, /* 70000A38 AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF2A, /* 70000A3A AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000A3C AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000A3E AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000A40 AFITB_bnr_nClustLevel_C      [0] */
	0x0F120000, /* 70000A42 AFIT16_BRIGHTNESS */
	0x0F120000, /* 70000A44 AFIT16_CONTRAST */
	0x0F12FFFB, /*  0000, */
	0x0F120000, /* 70000A48 AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000A4A AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000A4C AFIT16_bnr_edge_high */
	0x0F120064, /* 70000A4E AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 70000A50 AFIT16_postdmsc_iHighBright */
	0x0F120051, /* 70000A52 AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 70000A54 AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 70000A56 AFIT16_postdmsc_iTune */
	0x0F120040, /* 70000A58 AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000A5A AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000A5C AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000A5E AFIT16_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000A60 AFIT16_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000A62 AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000A64 AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 70000A66 AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 70000A68 AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F122404, /* 70000A6A AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F12031B, /* 70000A6C AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120103, /* 70000A6E AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F121004, /* 70000A70 AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F123A0C, /* 70000A72 AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120070, /* 70000A74 AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F121C80, /* 70000A76 AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F123030, /* 70000A78 AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120630, /* 70000A7A AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 70000A7C AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 70000A7E AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 70000A80 AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120000, /* 70000A82 AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120300, /* 70000A84 AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12245A, /* 70000A86 AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F121018, /* 70000A88 AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 70000A8A AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F120B00, /* 70000A8C AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 70000A8E AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120505, /* 70000A90 AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 70000A92 AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 70000A94 AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 70000A96 AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F122928, /* 70000A98 AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F120415, /* 70000A9A AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 70000A9C AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F120800, /* 70000A9E AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F121004, /* 70000AA0 AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 70000AA2 AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 70000AA4 AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 70000AA6 AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 70000AA8 AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 70000AAA AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 70000AAC AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120003, /* 70000AAE AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 70000AB0 AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 70000AB2 AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 70000AB4 AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000AB6 AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000AB8 AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000ABA AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120205, /* 70000ABC AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000ABE AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000AC0 AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120306, /* 70000AC2 AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120407, /* 70000AC4 AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F121F04, /* 70000AC6 AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F120218, /* 70000AC8 AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121102, /* 70000ACA AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120611, /* 70000ACC AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000ACE AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F126418, /* 70000AD0 AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F120080, /* 70000AD2 AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120380, /* 70000AD4 AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000AD6 AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000AD8 AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000ADA AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F122832, /* 70000ADC AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000ADE AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F12272C, /* 70000AE0 AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000AE2 AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000AE4 AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000AE6 AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000AE8 AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F122401, /* 70000AEA AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F12241B, /* 70000AEC AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F121E60, /* 70000AEE AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF18, /* 70000AF0 AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000AF2 AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000AF4 AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000AF6 AFITB_bnr_nClustLevel_C      [0] */
	0x0F120000, /* 70000AF8 AFIT16_BRIGHTNESS */
	0x0F120000, /* 70000AFA AFIT16_CONTRAST */
	0x0F12FFFB, /* 70000AFC AFIT16_SATURATION */
	0x0F120000, /* 70000AFE AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000B00 AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000B02 AFIT16_bnr_edge_high */
	0x0F120064, /* 70000B04 AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 70000B06 AFIT16_postdmsc_iHighBright */
	0x0F120043, /* 70000B08 AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 70000B0A AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 70000B0C AFIT16_postdmsc_iTune */
	0x0F120040, /* 70000B0E AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000B10 AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000B12 AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000B14 AFIT16_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000B16 AFIT16_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000B18 AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000B1A AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 70000B1C AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 70000B1E AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F121B04, /* 70000B20 AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F120312, /* 70000B22 AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120003, /* 70000B24 AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F120C03, /* 70000B26 AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F122806, /* 70000B28 AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120060, /* 70000B2A AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F121580, /* 70000B2C AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F122020, /* 70000B2E AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120620, /* 70000B30 AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 70000B32 AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 70000B34 AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 70000B36 AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120000, /* 70000B38 AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120300, /* 70000B3A AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12145A, /* 70000B3C AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F121010, /* 70000B3E AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 70000B40 AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F120E00, /* 70000B42 AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 70000B44 AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120504, /* 70000B46 AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 70000B48 AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 70000B4A AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 70000B4C AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F122B28, /* 70000B4E AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F120417, /* 70000B50 AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 70000B52 AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F128000, /* 70000B54 AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F120A04, /* 70000B56 AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 70000B58 AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 70000B5A AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 70000B5C AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 70000B5E AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 70000B60 AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 70000B62 AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120002, /* 70000B64 AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 70000B66 AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 70000B68 AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 70000B6A AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000B6C AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000B6E AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000B70 AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120207, /* 70000B72 AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000B74 AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000B76 AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120306, /* 70000B78 AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120407, /* 70000B7A AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F122404, /* 70000B7C AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F120221, /* 70000B7E AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121202, /* 70000B80 AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120613, /* 70000B82 AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000B84 AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F126E18, /* 70000B86 AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F120080, /* 70000B88 AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120080, /* 70000B8A AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000B8C AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000B8E AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000B90 AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F122630, /* 70000B92 AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000B94 AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F121616, /* 70000B96 AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000B98 AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000B9A AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000B9C AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000B9E AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F121B01, /* 70000BA0 AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F122412, /* 70000BA2 AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F120C60, /* 70000BA4 AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF0C, /* 70000BA6 AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000BA8 AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000BAA AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000BAC AFITB_bnr_nClustLevel_C      [0] */
	0x0F120000, /* 70000BAE AFIT16_BRIGHTNESS */
	0x0F120000, /* 70000BB0 AFIT16_CONTRAST */
	0x0F120000, /* 70000BB2 AFIT16_SATURATION */
	0x0F120000, /* 70000BB4 AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000BB6 AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000BB8 AFIT16_bnr_edge_high */
	0x0F120064, /* 70000BBA AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 70000BBC AFIT16_postdmsc_iHighBright */
	0x0F120032, /* 70000BBE AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 70000BC0 AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 70000BC2 AFIT16_postdmsc_iTune */
	0x0F120040, /* 70000BC4 AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000BC6 AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000BC8 AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000BCA AFIT16_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000BCC AFIT16_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000BCE AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000BD0 AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 70000BD2 AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 70000BD4 AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F121504, /* 70000BD6 AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F12030F, /* 70000BD8 AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120003, /* 70000BDA AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F120902, /* 70000BDC AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F122004, /* 70000BDE AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120050, /* 70000BE0 AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F121140, /* 70000BE2 AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F12201C, /* 70000BE4 AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120620, /* 70000BE6 AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 70000BE8 AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 70000BEA AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 70000BEC AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120000, /* 70000BEE AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120300, /* 70000BF0 AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12145A, /* 70000BF2 AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F121010, /* 70000BF4 AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 70000BF6 AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F121000, /* 70000BF8 AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 70000BFA AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120503, /* 70000BFC AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 70000BFE AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 70000C00 AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 70000C02 AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F123028, /* 70000C04 AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F12041A, /* 70000C06 AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 70000C08 AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F12FF00, /* 70000C0A AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F120904, /* 70000C0C AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 70000C0E AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 70000C10 AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 70000C12 AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 70000C14 AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 70000C16 AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 70000C18 AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120002, /* 70000C1A AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 70000C1C AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 70000C1E AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 70000C20 AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000C22 AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000C24 AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000C26 AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120206, /* 70000C28 AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000C2A AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000C2C AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120305, /* 70000C2E AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120406, /* 70000C30 AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F122804, /* 70000C32 AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F120228, /* 70000C34 AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121402, /* 70000C36 AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120618, /* 70000C38 AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000C3A AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F126E18, /* 70000C3C AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F120080, /* 70000C3E AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120080, /* 70000C40 AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000C42 AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000C44 AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000C46 AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F12232B, /* 70000C48 AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000C4A AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F121414, /* 70000C4C AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000C4E AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000C50 AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000C52 AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000C54 AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F121501, /* 70000C56 AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F12240F, /* 70000C58 AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F120A60, /* 70000C5A AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF0A, /* 70000C5C AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000C5E AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000C60 AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000C62 AFITB_bnr_nClustLevel_C      [0] */
	0x0F120000, /* 70000C64 AFIT16_BRIGHTNESS      //AFIT 4 */
	0x0F120000, /* 70000C66 AFIT16_CONTRAST */
	0x0F120000, /* 70000C68 AFIT16_SATURATION */
	0x0F120000, /* 70000C6A AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000C6C AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000C6E AFIT16_bnr_edge_high */
	0x0F120064, /* 70000C70 AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 70000C72 AFIT16_postdmsc_iHighBright */
	0x0F120032, /* 70000C74 AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 70000C76 AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 70000C78 AFIT16_postdmsc_iTune */
	0x0F120040, /* 70000C7A AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000C7C AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000C7E AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000C80 AFIT16_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000C82 AFIT16_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000C84 AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000C86 AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 70000C88 AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 70000C8A AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F120F04, /* 70000C8C AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F12030C, /* 70000C8E AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120003, /* 70000C90 AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F120602, /* 70000C92 AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F121803, /* 70000C94 AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120040, /* 70000C96 AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F120E20, /* 70000C98 AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F122018, /* 70000C9A AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120620, /* 70000C9C AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 70000C9E AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 70000CA0 AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 70000CA2 AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120000, /* 70000CA4 AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120200, /* 70000CA6 AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12145A, /* 70000CA8 AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F121010, /* 70000CAA AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 70000CAC AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F121200, /* 70000CAE AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 70000CB0 AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120502, /* 70000CB2 AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 70000CB4 AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 70000CB6 AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 70000CB8 AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F124028, /* 70000CBA AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F120430, /* 70000CBC AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 70000CBE AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F12FF00, /* 70000CC0 AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F120804, /* 70000CC2 AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 70000CC4 AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 70000CC6 AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 70000CC8 AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 70000CCA AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 70000CCC AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 70000CCE AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120002, /* 70000CD0 AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 70000CD2 AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 70000CD4 AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 70000CD6 AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000CD8 AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000CDA AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000CDC AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120205, /* 70000CDE AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000CE0 AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000CE2 AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120306, /* 70000CE4 AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120407, /* 70000CE6 AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F122C04, /* 70000CE8 AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F12022C, /* 70000CEA AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121402, /* 70000CEC AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120618, /* 70000CEE AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000CF0 AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F126E18, /* 70000CF2 AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F120080, /* 70000CF4 AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120080, /* 70000CF6 AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000CF8 AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000CFA AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000CFC AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F121114, /* 70000CFE AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000D00 AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F121212, /* 70000D02 AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000D04 AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000D06 AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000D08 AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000D0A AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F120F01, /* 70000D0C AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F12240C, /* 70000D0E AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F120860, /* 70000D10 AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF08, /* 70000D12 AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000D14 AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000D16 AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000D18 AFITB_bnr_nClustLevel_C [0]   bWideWide[1] */
	0x0F1223CE, /* 70000D1A	//[0]CAFITB_bnr_bypass */
	0x0F12FDC8, /* 70000D1C	//[0]CAFITB_bnr_bSlopenessTune */
	0x0F12112E, /* 70000D1E	//[0]CAFITB_ee_bReduceNegMedSh */
	0x0F1293A5, /* 70000D20	//[0]CAFITB_dmsc_bDoDesat */
	0x0F12FE67, /* 70000D22	//[0]CAFITB_postdmsc_bSat */
	0x0F120000, /* 70000D24	//[0]CAFITB_yuviirnr_bWideY */


/*==========================================================
// 18.JPEG Thumnail Setting
//=========================================================*/
	/* JPEG Quality */
	0x002A0478,
	0x0F12005F, /* REG_TC_BRC_usPrevQuality */
	0x0F12005F, /* REG_TC_BRC_usCaptureQuality */

	0x0F120001, /* REG_TC_THUMB_Thumb_bActive // JPEG Thumnail */
	0x0F120280, /* REG_TC_THUMB_Thumb_uWidth //640 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight //480 */
	0x0F120005, /* REG_TC_THUMB_Thumb_Format //YUV */

	0x002A17DC, /* JPEG setting */
	0x0F120054, /* jpeg_ManualMBCV */
	0x002A1AE4,
	0x0F12001C, /* senHal_bExtraAddLine */
	0x002A0284,
	0x0F120001, /* REG_TC_GP_bBypassScalerJpg */
	0x002A028A,
	0x0F120000, /* REG_TC_GP_bUse1FrameCaptureMode */

	0x002A1CC2, /* DRx_uDRxWeight for AutoCont function */
	0x0F120100,
	0x0F120100,
	0x0F120100,
	0x0F120100,
	0x002A147C, /* Brightness min/Max */
	0x0F120170, /* bp_uMaxBrightnessFactor */
	0x002A1482,
	0x0F1201E0, /*bp_uMinBrightnessFactor */

/*==========================================================
// 07.Input Size Setting
//=========================================================*/
	0x002A0250,
	0x0F120A10,
	0x0F12078C,
	0x0F120008,
	0x0F120006,
	0x0F120A10,
	0x0F12078C,
	0x0F120008,
	0x0F120006,
	0x002A0494,
	0x0F120A10,
	0x0F12078C,
	0x0F120000,
	0x0F120000,
	0x0F120A10,
	0x0F12078C,
	0x0F120000,
	0x0F120000,
	0x002A0262,
	0x0F120001,
	0x0F120001,

/*==========================================================
// Preview
//=========================================================*/
	0x002A02A6, /* Preview config[0] 640 480  10~30fps */
	0x0F120280, /* REG_0TC_PCFG_usWidth */
	0x0F1201E0, /* REG_0TC_PCFG_usHeight */
	0x0F120005, /* REG_0TC_PCFG_Format //5:YUV}, 7:RAW}, 9:JPEG */
	0x0F1229FE, /* REG_0TC_PCFG_usMaxOut4KHzRate */
	0x0F122904, /* REG_0TC_PCFG_usMinOut4KHzRate */
	0x0F120100, /* REG_0TC_PCFG_OutClkPerPix88 */
	0x0F120300, /* REG_0TC_PCFG_uBpp88 */
	0x0F120002, /* REG_0TC_PCFG_PVIMask //[1]:PCLK Inversion */
	0x0F120000, /* REG_0TC_PCFG_OIFMask */
	0x0F1201E0, /* REG_0TC_PCFG_usJpegPacketSize */
	0x0F120000, /* REG_0TC_PCFG_usJpegTotalPackets */
	0x0F120000, /* REG_0TC_PCFG_uClockInd */
	0x0F120000, /* REG_0TC_PCFG_usFrTimeType */
	0x0F120001, /* REG_0TC_PCFG_FrRateQualityType */
	0x0F1203E8, /* REG_0TC_PCFG_usMaxFrTimeMsecMult10 //03E8h:10fps */
	0x0F12014A, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 //014Ah:30fps */
	0x002A02D0,
	0x0F120000, /* REG_0TC_PCFG_uPrevMirror */
	0x0F120000, /* REG_0TC_PCFG_uCaptureMirror */

/*==========================================================
// Capture
//=========================================================*/
	0x002A0396,
	0x0F120000, /* REG_0TC_CCFG_uCaptureMode */
	0x0F120A10, /* REG_0TC_CCFG_usWidth //2576 */
	0x0F12078C, /* REG_0TC_CCFG_usHeight //1932 */
	0x0F120005, /* REG_0TC_CCFG_Format //5:YUV}, 7:RAW}, 9:JPEG */
	0x0F1229FE, /* REG_0TC_CCFG_usMaxOut4KHzRate */
	0x0F122904, /* REG_0TC_CCFG_usMinOut4KHzRate */
	0x0F120100, /* REG_0TC_CCFG_OutClkPerPix88 */
	0x0F120300, /* REG_0TC_CCFG_uBpp88 */
	0x0F120002, /* REG_0TC_CCFG_PVIMask //[1]:PCLK Inversion */
	0x0F120070, /* REG_0TC_CCFG_OIFMask */
	0x0F120810, /* REG_0TC_CCFG_usJpegPacketSize //2064d */
	0x0F120900, /* REG_0TC_CCFG_usJpegTotalPackets  */
	0x0F120001, /* REG_0TC_CCFG_uClockInd */
	0x0F120000, /* REG_0TC_CCFG_usFrTimeType */
	0x0F120002, /* REG_0TC_CCFG_FrRateQualityType */
	0x0F120535, /* REG_0TC_CCFG_usMaxFrTimeMsecMult10 //0535h:7.5fps */
	0x0F12029A, /* REG_0TC_CCFG_usMinFrTimeMsecMult10 //029Ah:15fps */
	0x002A022C,
	0x0F120001, /* REG_TC_IPRM_InitParamsUpdated */

/*==========================================================
// 21.Select Cofigration Display
//=========================================================*/
	0x00287000,
	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
	0x002A026E,
	0x0f120000,
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync// */
	0x002A023E,
	0x0F120001, /* REG_TC_GP_EnablePreview */
	0x0F120001, /* REG_TC_GP_EnablePreviewChanged */

/*==========================================================
// 22. ESD Check
//=========================================================*/
	0x00287000,
	0x002A01A8, /* ESD Check*/
	0x0F12AAAA,
	0x0028147C,
	0x0F120170,
	0x00281482,
	0x0F1201E0,
};


static const u32 s5k4ec_init_reg3[] = {
/*==========================================================
// 21.Select Cofigration Display
//=========================================================*/
	0x00287000,
	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
	0x002A026E,
	0x0f120000,
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A023E,
	0x0F120001, /* REG_TC_GP_EnablePreview */
	0x0F120001, /* REG_TC_GP_EnablePreviewChanged */

/*==========================================================
// 22. ESD Check
//=========================================================*/
	0x00287000,
	0x002A01A8, /* ESD Check */
	0x0F12AAAA,
	0x0028147C,
	0x0F120170,
	0x00281482,
	0x0F1201E0,

/*==========================================================
// 23. Brightness min/Max
//=========================================================*/
	0x002A147C,  /* Brightness min/Max */
	0x0F120170,  /* bp_uMaxBrightnessFactor */
	0x002A1482,
	0x0F1201E0,  /* bp_uMinBrightnessFactor	*/

/*==========================================================
// 24.ISSUE
//
//20110728 : Sequence Changed by image dev.(by J.M.Ahn)
//20110728 : ESD Check Register Address Change
//20110829 : TnP Changed by S.Y.Lee
//20120104 : init Parm Update sequence changed(by J.M.Ahn)
//20120201 : Flash ֺ Green Noise  setting (by J.M.Ahn)
//20120228 : Add Brightness Block
//20120717 : Lowtemp bypass Ŵ
//=========================================================*/

};

static const u32 s5k4ec_init_reg4[] = {
/*==========================================================
// 19.Select Cofigration Display
//=========================================================*/
	0x00287000,
	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
	0x002A026E,
	0x0f120000,
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A023E,
	0x0F120001, /* REG_TC_GP_EnablePreview */
	0x0F120001, /* REG_TC_GP_EnablePreviewChanged */
};

static const u32 s5k4ec_normal_snapshot[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0242,
	0x0F120001,
	0x002A024E,
	0x0F120001,
	0x002A0244,
	0x0F120001,
};

static const u32 s5k4ec_dtp_init[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0944,
	0x0F12FFF0, /*af_uNoise_0_*/
	0x0F12FFF1, /*afit_uNoiseIndInDoor_1_ */
	0x0F12FFF2, /*afit_uNoiseIndInDoor_2_ */
	0x0F12FFF3, /*afit_uNoiseIndInDoor_3_ */
	0x0F12FFF4, /*afit_uNoiseIndInDoor_4_ */
	0x002A0938,
	0x0F120000, /*afit_bUseNB_Afit */
	0x0F12FFF0, /*SARR_uNormBrInDoor_0_ */
	0x0F12FFF1, /*SARR_uNormBrInDoor_1_ */
	0x0F12FFF2, /*SARR_uNormBrInDoor_2_ */
	0x0F12FFF3, /*SARR_uNormBrInDoor_3_ */
	0x0F12FFF4, /*SARR_uNormBrInDoor_4_ */
	0x00287000,
	0x002A04A6,
	0x0F120001,
	0x002A04AA,
	0x0F120001,
	0x0028D000,
	0x002A4200,
	0x0F1208A3, /*GAS bypass */
	0x002A6600,
	0x0F120001, /*CCM bypass */
	0x002A6700,
	0x0F120001, /*Gamma bypass */
	0x002A4900,
	0x0F120001, /*AWB bypass */

/*Delay 50ms*/
	0x0028D000,
	0x002A3100,
	0x0F120002, /*Colorbar pattern */
};

static const u32 s5k4ec_dtp_stop[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0944,
	0x0F12004B, /*afit_uNoiser0*/
	0x0F120092, /*afit_uNoiseIndInDoor_1_ */
	0x0F120196, /*afit_uNoiseIndInDoor_2_ */
	0x0F120217, /*afit_uNoiseIndInDoor_3_ */
	0x0F1202B0, /*afit_uNoiseIndInDoor_4_ */

	/* Normal Brightness setting */
	0x002A0938,
	0x0F120000, /*afit_bUseNB_Afit */
	0x0F120014, /*SARR_uNormBrInDoor_0_ */
	0x0F1200D2, /*SARR_uNormBrInDoor_1_ */
	0x0F120384, /*SARR_uNormBrInDoor_2_ */
	0x0F1207D0, /*SARR_uNormBrInDoor_3_ */
	0x0F121388, /*SARR_uNormBrInDoor_4_ */
	0x00287000,
	0x002A04A6,
	0x0F120000,
	0x002A04AA,
	0x0F120001,
	0x0028D000,
	0x002A4200,
	0x0F1208A2, /*GAS bypass */
	0x002A6600,
	0x0F120000, /*CCM bypass */
	0x002A6700,
	0x0F120000, /*Gamma bypass */
	0x002A4900,
	0x0F120000, /*AWB bypass */
	0xFFFE0032, /*p50 */
	0x0028D000,
	0x002A3100,
	0x0F120000, /*Colorbar pattern */
};

static const u32 s5k4ec_50hz[] = {
	0x002A0F30,
	0x0F120001, /* AFC_D_ConvAccelerPower */
	0x002A0F2A,
	0x0F120000, /* AFC_Default BIT[0] 1:60Hz 0:50Hz */
};

static const u32 s5k4ec_60hz[] = {
	0x002A0F30,
	0x0F120001, /* AFC_D_ConvAccelerPower */
	0x002A0F2A,
	0x0F120001, /* AFC_Default BIT[0] 1:60Hz 0:50Hz */
};

static const u32 s5k4ec_fps_auto[] = {
	0xFCFCD000,
	0x00287000,
	0x002A02BE,
	0x0F120000, /*usFrTimeType*/
	0x0F120001, /*REG_0TC_PCFG_FrRateQualityType */
	0x0F1203E8, /*REG_0TC_PCFG_usMaxFrTimeMsecMult10 */ /* 029Ah:15fps */
	0x0F12014A, /*REG_0TC_PCFG_usMinFrTimeMsecMult10 */ /*014Ah:30fps */
	0x002A0266,
	0x0F120000, /*REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /*REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /*REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /*REG_TC_GP_PrevConfigChanged */
};

static const u32 s5k4ec_fps_5[] = {
};

static const u32 s5k4ec_fps_7[] = {
	0xFCFCD000,
	0x00287000,
	0x002A02B4,
	0x0F120012,
	0x002A02BE,
	0x0F120000, /* REG_0TC_PCFG_usFrTimeType */
	0x0F120001, /* REG_0TC_PCFG_FrRateQualityType */
	0x0F120535, /* REG_0TC_PCFG_usMaxFrTimeMsecMult10 //0535h:7.5fps */
	0x0F120535, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 //0535h:7.5fps */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

static const u32 s5k4ec_fps_10[] = {
	/* frame rate 10~10fps*/
	0xFCFCD000,
	0x0028D000,
	0x002AF132,
	0x0F120200,
	0x002AF142,
	0x0F120200, /* 110404 AE haunting */
	0x00287000,
	0x002A02BE,
	0x0F120000, /* usFrTimeType */
	0x0F120001, /* REG_0TC_PCFG_FrRateQualityTy */
	0x0F1203E8, /* 029A/REG_0TC_PCFG_usMaxFrTimeMsecMult10 //03E8h:10fps */
	0x0F1203E8, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 //03E8h:10fps */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

static const u32 s5k4ec_fps_12[] = {
	/* frame rate 12~12fps*/
	0xFCFCD000,
	0x0028D000,
	0x002AF132,
	0x0F120200,
	0x002AF142,
	0x0F120200, /* 110404 AE haunting */
	0x00287000,
	0x002A02BE,
	0x0F120000, /* usFrTimeType */
	0x0F120001, /* REG_0TC_PCFG_FrRateQualityType */
	0x0F120341, /* 029A/REG_0TC_PCFG_usMaxFrTimeMsecMult10 //0341h:12fps */
	0x0F120341, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 //0341h:12fps */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

static const u32 s5k4ec_fps_15[] = {
	0xFCFCD000,
	0x00287000,
	0x002A02B4,
	0x0F120012,
	0x002A02BE,
	0x0F120000, /* REG_0TC_PCFG_usFrTimeType */
	0x0F120001, /* REG_0TC_PCFG_FrRateQualityType */
	0x0F12029A, /* REG_0TC_PCFG_usMaxFrTimeMsecMult10 //029Ah:15fps */
	0x0F12029A, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 //029Ah:15fps */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

static const u32 s5k4ec_fps_20[] = {
	0xFCFCD000,
	0x00287000,
	0x002A02B4,
	0x0F120012,
	0x002A02BE,
	0x0F120000, /* REG_0TC_PCFG_usFrTimeType */
	0x0F120001, /* REG_0TC_PCFG_FrRateQualityType */
	0x0F1201F4, /* REG_0TC_PCFG_usMaxFrTimeMsecMult10 //01F4h:20fps */
	0x0F1201F4, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 //01F4h:20fps */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

static const u32 s5k4ec_fps_25[] = {
	0xFCFCD000,
	0x0028D000,
	0x002AF132,
	0x0F120200,
	0x002AF142,
	0x0F120200, /* 110404 AE haunting */
	0x00287000,
	0x002A02BE,
	0x0F120000, /* REG_0TC_PCFG_usFrTimeType */
	0x0F120001, /* REG_0TC_PCFG_FrRateQualityType */
	0x0F120190, /* REG_0TC_PCFG_usMaxFrTimeMsecMult10 //0190h:25fps */
	0x0F120190, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 //0190h:25fps */
	0x002A0266,
	0x0F120000,
	0x002A026A,
	0x0F120001,
	0x002A024E,
	0x0F120001, /* #REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* #REG_TC_GP_CapConfigChanged */
};

static const u32 s5k4ec_fps_30[] = {
	0xFCFCD000,
	0x0028D000,
	0x002AF132,
	0x0F120006,
	0x002AF142,
	0x0F120000, /* 110404 AE haunting */
	0x00287000,
	0x002A02BE,
	0x0F120000,
	0x0F120001,
	0x0F12014A,
	0x0F12014A,
	0x002A0266,
	0x0F120000,
	0x002A026A,
	0x0F120001,
	0x002A024E,
	0x0F120001, /* #REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001,
};

static const u32 s5k4ec_effect_normal[] = {
	0xFCFCD000,
	0x00287000,
	0x002A023C,
	0x0F120000,
};

static const u32 s5k4ec_effect_solarization[] = {
	0xFCFCD000,
	0x00287000,
	0x002A023C,
	0x0F120002,
};

static const u32 s5k4ec_effect_negative[] = {
	0xFCFCD000,
	0x00287000,
	0x002A023C,
	0x0F120003,
};

static const u32 s5k4ec_effect_sepia[] = {
	0xFCFCD000,
	0x00287000,
	0x002A023C,
	0x0F120004,
};

static const u32 s5k4ec_effect_black_white[] = {
	0xFCFCD000,
	0x00287000,
	0x002A023C,
	0x0F120001,
};

static const u32 s5k4ec_effect_aqua[] = {

};

static const u32 s5k4ec_effect_sketch[] = {

};

static const u32 s5k4ec_wb_auto[] = {
	0xFCFCD000,
	0x00287000,
	0x002A04E6,
	0x0F12077F,

	0x002A1826,
	0x0F120100, /* _afl_FlashWP_Weight2_0_ */
	0x0F1200C0, /* _afl_FlashWP_Weight2_1_ */
	0x0F120080, /* _afl_FlashWP_Weight2_2_ */
	0x0F12000A, /* _afl_FlashWP_Weight2_3_ */
	0x0F120000, /* _afl_FlashWP_Weight2_4_ */
};

static const u32 s5k4ec_wb_sunny[] = {
	0xFCFCD000,
	0x00287000,
	0x002A04E6,
	0x0F120777,

	0x002A04BA, /* R gain */
	0x0F1205A0,

	0x002A04BE, /* G gain */
	0x0F120400,

	0x002A04C2, /* B gain */
	0x0F120570,

	0x002A04C6, /* RGB gain changed */
	0x0F120001,

	0x002A1826,
	0x0F120000, /* _afl_FlashWP_Weight2_0_ */
	0x0F120000, /* _afl_FlashWP_Weight2_1_ */
	0x0F120000, /* _afl_FlashWP_Weight2_2_ */
	0x0F120000, /* _afl_FlashWP_Weight2_3_ */
	0x0F120000, /* _afl_FlashWP_Weight2_4_ */
};

static const u32 s5k4ec_wb_cloudy[] = {
	0xFCFCD000,
	0x00287000,
	0x002A04E6,
	0x0F120777,

	0x002A04BA,  /* R gain */
	0x0F120670,

	0x002A04BE,  /* G gain */
	0x0F120400,

	0x002A04C2,  /* B gain */
	0x0F1204C0,

	0x002A04C6,
	0x0F120001,

	0x002A1826,
	0x0F120000, /* _afl_FlashWP_Weight2_0_ */
	0x0F120000, /* _afl_FlashWP_Weight2_1_ */
	0x0F120000, /* _afl_FlashWP_Weight2_2_ */
	0x0F120000, /* _afl_FlashWP_Weight2_3_ */
	0x0F120000, /* _afl_FlashWP_Weight2_4_ */
};

static const u32 s5k4ec_wb_tungsten[] = {
	0xFCFCD000,
	0x00287000,
	0x002A04E6,
	0x0F120777,

	0x002A04BA,
	0x0F120420,

	0x002A04BE,
	0x0F120430,

	0x002A04C2,
	0x0F120990,

	0x002A04C6,
	0x0F120001,

	0x002A1826,
	0x0F120000, /* _afl_FlashWP_Weight2_0_ */
	0x0F120000, /* _afl_FlashWP_Weight2_1_ */
	0x0F120000, /* _afl_FlashWP_Weight2_2_ */
	0x0F120000, /* _afl_FlashWP_Weight2_3_ */
	0x0F120000, /* _afl_FlashWP_Weight2_4_ */
};

static const u32 s5k4ec_wb_fluorescent[] = {
	0xFCFCD000,
	0x00287000,
	0x002A04E6,
	0x0F120777,

	0x002A04BA,  /* R gain */
	0x0F1205C0,

	0x002A04BE,  /* G gain */
	0x0F120400,

	0x002A04C2,  /* B gain */
	0x0F1207F0,

	0x002A04C6,
	0x0F120001,

	0x002A1826,
	0x0F120000, /* _afl_FlashWP_Weight2_0_ */
	0x0F120000, /* _afl_FlashWP_Weight2_1_ */
	0x0F120000, /* _afl_FlashWP_Weight2_2_ */
	0x0F120000, /* _afl_FlashWP_Weight2_3_ */
	0x0F120000, /* _afl_FlashWP_Weight2_4_ */
};

static const u32 s5k4ec_wdr_on[] = {
	0x002A1BEA,
	0x0F120000,
};

static const u32 s5k4ec_wdr_off[] = {
	0x002A1BEA,
	0x0F120001,
};

static const u32 s5k4ec_iso_auto[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0938, /* afit_bUseNB_Afit */
	0x0F120000,
	0x0F120014, /* SARR_uNormBrInDoor_0_ */
	0x0F1200D2, /* SARR_uNormBrInDoor_1_ */
	0x0F120384, /* SARR_uNormBrInDoor_2_ */
	0x0F1207D0, /* SARR_uNormBrInDoor_3_ */
	0x0F121388, /* SARR_uNormBrInDoor_4_ */

	0x002A2C64,
	0x0F120001, /*Auto Flicker Enable */

	0x002A0230,
	0x0F120000,

	0x002A04D0,
	0x0F120000,
	0x0F120000,
	0x0F120001,

	0x002A06C2,
	0x0F120200,
};

static const u32 s5k4ec_iso_50[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0938,
	0x0F120001, /* afit_bUseNB_Afit */
	0x0F120014, /* SARR_uNormBrInDoor_0_ */
	0x0F1200D2, /* SARR_uNormBrInDoor_1_ */
	0x0F120384, /* SARR_uNormBrInDoor_2_ */
	0x0F1207D0, /* SARR_uNormBrInDoor_3_ */
	0x0F121388, /* SARR_uNormBrInDoor_4_ */

	0x002A04D6,
	0x0F120000, /* REG_SF_USER_FlickerQuant */
	0x0F120001, /* REG_SF_USER_FlickerQuantChanged */

	0x002A04D0,
	0x0F120001, /* REG_SF_USER_IsoType*/
	0x0F120100, /* REG_SF_USER_IsoVal */
	0x0F120001, /* REG_SF_USER_IsoChanged */
	0x002A06C2,
	0x0F120100, /* lt_bUseSecISODgain */
};

static const u32 s5k4ec_iso_100[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0938,
	0x0F120001, /* afit_bUseNB_Afit */
	0x0F120014, /* SARR_uNormBrInDoor_0_ */
	0x0F1200D2, /* SARR_uNormBrInDoor_1_ */
	0x0F120384, /* SARR_uNormBrInDoor_2_ */
	0x0F1207D0, /* SARR_uNormBrInDoor_3_ */
	0x0F121388, /* SARR_uNormBrInDoor_4_ */

	0x002A04D6,
	0x0F120000, /*REG_SF_USER_FlickerQuant */
	0x0F120001, /*REG_SF_USER_FlickerQuantChanged */

	0x002A04D0,
	0x0F120001, /*REG_SF_USER_IsoType */
	0x0F1201BA, /*REG_SF_USER_IsoVal/1BA/1CA:16.9msec/1AA: 17.8msec */
	0x0F120001, /*REG_SF_USER_IsoChanged */
	0x002A06C2,
	0x0F120100, /*lt_bUseSecISODgain */
};

static const u32 s5k4ec_iso_200[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0938,
	0x0F120001, /* afit_bUseNB_Afit */
	0x0F120114, /* SARR_uNormBrInDoor_0_ */
	0x0F1204A2, /* SARR_uNormBrInDoor_1_ */
	0x0F120584, /* SARR_uNormBrInDoor_2_ */
	0x0F1208D0, /* SARR_uNormBrInDoor_3_ */
	0x0F121388, /* SARR_uNormBrInDoor_4_ */

	0x002A04D6,
	0x0F120000, /*REG_SF_USER_FlickerQuant */
	0x0F120001, /*REG_SF_USER_FlickerQuantChanged */

	0x002A04D0,
	0x0F120001, /* REG_SF_USER_IsoType */
	0x0F120374,
	0x0F120001, /* REG_SF_USER_IsoChanged */
	0x002A06C2,
	0x0F120100, /* lt_bUseSecISODgain */
};

static const u32 s5k4ec_iso_400[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0938,
	0x0F120001, /* afit_bUseNB_Afit */
	0x0F120214, /* SARR_uNormBrInDoor_0_ */
	0x0F120BD2, /* SARR_uNormBrInDoor_1_ */
	0x0F120C84, /* SARR_uNormBrInDoor_2_ */
	0x0F1210D0, /* SARR_uNormBrInDoor_3_ */
	0x0F121388, /* SARR_uNormBrInDoor_4_ */

	0x002A04D6,
	0x0F120000, /* REG_SF_USER_FlickerQuant */
	0x0F120001, /* REG_SF_USER_FlickerQuantChanged */

	0x002A04D0,
	0x0F120001, /* REG_SF_USER_IsoType */
	0x0F1206F4, /* REGSFUSER_IsoVal/6F4*/
	0x0F120001, /* REG_SF_USER_IsoChanged */
	0x002A06C2,
	0x0F120100, /* lt_bUseSecISODgain */
};

static const u32 s5k4ec_metering_matrix[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1492,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,

	0x002A0268, /* REG_TC_GP_PrevConfigChanged */
	0x0F120001,
};

static const u32 s5k4ec_metering_center[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1492,
	0x0F120100,
	0x0F120101,
	0x0F120101,
	0x0F120001,
	0x0F120101,
	0x0F120201,
	0x0F120102,
	0x0F120101,
	0x0F120101,
	0x0F120202,
	0x0F120202,
	0x0F120101,
	0x0F120201,
	0x0F120302,
	0x0F120203,
	0x0F120102,
	0x0F120201,
	0x0F120302,
	0x0F120203,
	0x0F120102,
	0x0F120201,
	0x0F120202,
	0x0F120202,
	0x0F120102,
	0x0F120101,
	0x0F120202,
	0x0F120202,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,

	0x002A0268, /* REG_TC_GP_PrevConfigChanged */
	0x0F120001,
};

static const u32 s5k4ec_metering_spot[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1492,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120101,
	0x0F120101,
	0x0F120000,
	0x0F120000,
	0x0F120F01,
	0x0F12010F,
	0x0F120000,
	0x0F120000,
	0x0F120F01,
	0x0F12010F,
	0x0F120000,
	0x0F120000,
	0x0F120101,
	0x0F120101,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,

	0x002A0268, /* REG_TC_GP_PrevConfigChanged */
	0x0F120001,
};

static const u32 s5k4ec_anti_banding_flicker_50[] = {
	0x00287000,
	0x002a04e6,
	0x0f12075f,
	0x002a04d6,
	0x0f120001,
	0x0f120001,
	0xFFFFFFFF,
};

static const u32 s5k4ec_anti_banding_flicker_60[] = {
	0x00287000,
	0x002a04e6,
	0x0f12075f,
	0x002a04d6,
	0x0f120002,
	0x0f120001,
	0xFFFFFFFF,
};

/* brightness_level_0 */
static const u32 s5k4ec_ev_minus_4[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F120018, /* _ae_BrAve */

	0x002A0544,
	0x0F12011F,
	0x0F1200E1,
};

/* brightness_level_1 */
static const u32 s5k4ec_ev_minus_3[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F12001E, /* _ae_BrAve */

	0x002A0544,
	0x0F12011F,
	0x0F1200E1,
};

/* brightness_level_2 */
static const u32 s5k4ec_ev_minus_2[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F120025, /* _ae_BrAve */

	0x002A0544,
	0x0F12011F,
	0x0F1200E1,
};

/* brightness_level_3 */
static const u32 s5k4ec_ev_minus_1[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F120030, /* _ae_BrAve */
};

/* brightness_level_4 */
static const u32 s5k4ec_ev_default[] = {
	0xFCFCD000,  /* default */
	0x00287000,
	0x002A1484,
	0x0F12003C, /* _ae_BrAve */

	0x002A0544,
	0x0F120111,
	0x0F1200EF,
};

/* brightness_level_5 */
static const u32 s5k4ec_ev_plus_1[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F12004E, /* _ae_BrAve */
};

/* brightness_level_6 */
static const u32 s5k4ec_ev_plus_2[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F12005C, /* TVAR_ae_BrAve 1101 0060->005C */
};

/* brightness_level_7 */
static const u32 s5k4ec_ev_plus_3[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F120070, /* TVAR_ae_BrAve */
};

/* brightness_level_8 */
static const u32 s5k4ec_ev_plus_4[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F120080, /* TVAR_ae_BrAve */
};

/* camcorder brightness_level_0 */
static const u32 s5k4ec_camcorder_ev_minus_4[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F120012, /* _ae_BrAve */
};

/* camcorder brightness_level_1 */
static const u32 s5k4ec_camcorder_ev_minus_3[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F12001A, /* _ae_BrAve */
};

/* camcorder brightness_level_2 */
static const u32 s5k4ec_camcorder_ev_minus_2[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F120022, /* _ae_BrAve */
};

/* camcorder brightness_level_3 */
static const u32 s5k4ec_camcorder_ev_minus_1[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F12002A, /* _ae_BrAve */
};

/* camcorder brightness_level_4 */
static const u32 s5k4ec_camcorder_ev_default[] = {
	0xFCFCD000,  /* default */
	0x00287000,
	0x002A1484,
	0x0F120032, /* _ae_BrAve */
};

/* camcorder brightness_level_5 */
static const u32 s5k4ec_camcorder_ev_plus_1[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F12003F, /* _ae_BrAve */
};

/* camcorder brightness_level_6 */
static const u32 s5k4ec_camcorder_ev_plus_2[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F12004C, /* TVAR_ae_BrAve 1101 0060->005C */
};

/* camcorder brightness_level_7 */
static const u32 s5k4ec_camcorder_ev_plus_3[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F120059, /* TVAR_ae_BrAve */
};

/* camcorder brightness_level_8 */
static const u32 s5k4ec_camcorder_ev_plus_4[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1484,
	0x0F120066, /* TVAR_ae_BrAve */
};

/* contrast_level_0 */
static const u32 s5k4ec_contrast_minus_2[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0232,
	0x0F12FFC0,
};

/* contrast_level_1 */
static const u32 s5k4ec_contrast_minus_1[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0232,
	0x0F12FFE0,
};

/* contrast_level_2 */
static const u32 s5k4ec_contrast_default[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0232,
	0x0F120000,  /* default */
};

/* contrast_level_3 */
static const u32 s5k4ec_contrast_plus_1[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0232,
	0x0F120020,
};

/* contrast_level_4 */
static const u32 s5k4ec_contrast_plus_2[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0232,
	0x0F120040,
};

static const u32 s5k4ec_auto_contrast_on[] = {
	0x002A098E,
	0x0F12FFB0,
	0x002A0A44,
	0x0F12FFB0,
	0x002A0AFA,
	0x0F12FFB0,
	0x002A0BB0,
	0x0F12FFB0,
	0x002A0C66,
	0x0F12FFB0,
};

static const u32 s5k4ec_auto_contrast_off[] = {
	0x002A098E,
	0x0F12FFB0,
	0x002A0A44,
	0x0F12FFB0,
	0x002A0AFA,
	0x0F12FFB0,
	0x002A0BB0,
	0x0F12FFB0,
	0x002A0C66,
	0x0F12FFB0,
};

/* sharpness_level_0 */
static const u32 s5k4ec_sharpness_minus_2[] = {
	0x00287000,
	0x002A0A28,
	0x0F122010,
	0x002A0ADE,
	0x0F122010,
	0x002A0B94,
	0x0F122010,
	0x002A0C4A,
	0x0F122010,
	0x002A0D00,
	0x0F122010,
};

/* sharpness_level_1 */
static const u32 s5k4ec_sharpness_minus_1[] = {
	0x00287000,
	0x002A0A28,
	0x0F124020,
	0x002A0ADE,
	0x0F124020,
	0x002A0B94,
	0x0F124020,
	0x002A0C4A,
	0x0F124020,
	0x002A0D00,
	0x0F124020,
};

/* sharpness_level_2 */
static const u32 s5k4ec_sharpness_default[] = {
	0x00287000,
	0x002A0A28,
	0x0F126024,
	0x002A0ADE,
	0x0F126024,
	0x002A0B94,
	0x0F126024,
	0x002A0C4A,
	0x0F126024,
	0x002A0D00,
	0x0F126024,
};

/* sharpness_level_3 */
static const u32 s5k4ec_sharpness_plus_1[] = {
	0x00287000,
	0x002A0A28,
	0x0F128040,
	0x002A0ADE,
	0x0F128040,
	0x002A0B94,
	0x0F128040,
	0x002A0C4A,
	0x0F128040,
	0x002A0D00,
	0x0F128040,
};

/* sharpness_level_4 */
static const u32 s5k4ec_sharpness_plus_2[] = {
	0x00287000,
	0x002A0A28,
	0x0F12A060,
	0x002A0ADE,
	0x0F12A060,
	0x002A0B94,
	0x0F12A060,
	0x002A0C4A,
	0x0F12A060,
	0x002A0D00,
	0x0F12A060,
};

/* saturation_level_0 */
static const u32 s5k4ec_saturation_minus_2[] = {
	0x00287000,
	0x002A0234,
	0x0F12FF81,
};

/* saturation_level_1 */
static const u32 s5k4ec_saturation_minus_1[] = {
	0x00287000,
	0x002A0234,
	0x0F12FFC0,
};

/* saturation_level_2 */
static const u32 s5k4ec_saturation_default[] = {
	0x00287000,
	0x002A0234,
	0x0F120000,
};

/* saturation_level_3 */
static const u32 s5k4ec_saturation_plus_1[] = {
	0x00287000,
	0x002A0234,
	0x0F120040,
};

/* saturation_level_4 */
static const u32 s5k4ec_saturation_plus_2[] = {
	0x00287000,
	0x002A0234,
	0x0F12007F,
};

static const u32 s5k4ec_jpeg_quality_high[] = {
	0x00287000,
	0x002A0478,
	0x0F12005F,
	0x0F12005F,
};

static const u32 s5k4ec_jpeg_quality_normal[] = {
	0x00287000,
	0x002A0478,
	0x0F12005A,
	0x0F12005A,
};

static const u32 s5k4ec_jpeg_quality_low[] = {
	0x00287000,
	0x002A0478,
	0x0F120054,
	0x0F120054,
};

static const u32 s5k4ec_scene_default[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1492,
	0x0F120100,
	0x0F120101,
	0x0F120101,
	0x0F120001,
	0x0F120101,
	0x0F120201,
	0x0F120102,
	0x0F120101,
	0x0F120101,
	0x0F120202,
	0x0F120202,
	0x0F120101,
	0x0F120201,
	0x0F120302,
	0x0F120203,
	0x0F120102,
	0x0F120201,
	0x0F120302,
	0x0F120203,
	0x0F120102,
	0x0F120201,
	0x0F120202,
	0x0F120202,
	0x0F120102,
	0x0F120101,
	0x0F120202,
	0x0F120202,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,

/* Sharpness 0*/
	0x002A0A28,
	0x0F126024, /* _ee_iLowSharpPower */
	0x002A0ADE,
	0x0F126024, /* _ee_iLowSharpPower */
	0x002A0B94,
	0x0F126024, /* _ee_iLowSharpPower */
	0x002A0C4A,
	0x0F126024, /* _ee_iLowSharpPower */
	0x002A0D00,
	0x0F126024, /* _ee_iLowSharpPower */

/* Saturation 0*/
	0x002A0234,
	0x0F120000, /* REG_TC_UserSaturation */
	0x002A06B8,
	0x0F12802C,
	0x0F120005, /* lt_uMaxLei */

	0x002A098C,
	0x0F12FFFB, /* 7000098C AFIT16_BRIGHTNESS */

	0x002A0A1E,
	0x0F120350, /*_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset init */

	/* GAS Alpha Table */
	0x002A08F6,
	0x0F124000, /* TVAR_ash_GASAlpha_0__0_ R  // 2300K */
	0x0F124000, /* TVAR_ash_GASAlpha_0__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_0__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_0__3_ B */
	0x0F124000, /* TVAR_ash_GASAlpha_1__0_ R  // 2750K */
	0x0F124000, /* TVAR_ash_GASAlpha_1__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_1__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_1__3_ B */
	0x0F124100, /* TVAR_ash_GASAlpha_2__0_ R  // 3300K */
	0x0F124000, /* TVAR_ash_GASAlpha_2__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_2__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_2__3_ B */
	0x0F123F80, /* TVAR_ash_GASAlpha_3__0_ R  // 4150K */
	0x0F124000, /* TVAR_ash_GASAlpha_3__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_3__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_3__3_ B */
	0x0F124080, /* TVAR_ash_GASAlpha_4__0_ R  // 5250K */
	0x0F124000, /* TVAR_ash_GASAlpha_4__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_4__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_4__3_ B */
	0x0F124100, /* TVAR_ash_GASAlpha_5__0_ R  // 6400K */
	0x0F124000, /* TVAR_ash_GASAlpha_5__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_5__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_5__3_ B */
	0x0F124150, /* TVAR_ash_GASAlpha_6__0_ R  // 7500K */
	0x0F124000, /* TVAR_ash_GASAlpha_6__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_6__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_6__3_ B */

	0x002A0638,
	0x0F120001,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_0_ */
	0x0F120A3C,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_1_ */
	0x0F120D05,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_2_ */
	0x0F123408,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_3_ */
	0x0F123408,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_4_ */
	0x0F126810,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_5_ */
	0x0F128214,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_6_ */
	0x0F12C350,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_7_ */
	0x0F12C350,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_8_ */
	0x0F12C350,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_9_ */

	0x002A02C2,
	0x0F1203E8, /* REG_0TC_PCFG_usMaxFrTimeMsecMult10 *//*03E8h:10fps */
	0x0F12014A, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 *//*014Ah:30fps */
	0x002A03B4,
	0x0F120535, /* REG_0TC_CCFG_usMaxFrTimeMsecMult10 *//*0535h:7.5fps */
	0x0F12029A, /* REG_0TC_CCFG_usMinFrTimeMsecMult10 *//*029Ah:15fps */

	0x002A0938,
	0x0F120000, /* afit_bUseNB_Afit */

	0x002A04E6,
	0x0F12077F, /* REG_TC_DBG_AutoAlgEnBits */

	0x002A1484,
	0x0F12003C,

	0x002A0544,
	0x0F120111,
	0x0F1200EF,

	0x002A04D0,
	0x0F120000, /*REG_SF_USER_IsoType */
	0x0F120000, /*REG_SF_USER_IsoVal */
	0x0F120001, /*REG_SF_USER_IsoChanged */

	0x002A06C2,
	0x0F120200, /*lt_bUseSecISODgain */

	0x002A1648,
	0x0F129002, /*af_search_usSingleAfFlags */

	0x002A15E8,
	0x0F120018, /*af_pos_usTableLastInd */
	0x0F12002A,
	0x0F120030,
	0x0F120036,
	0x0F12003C,
	0x0F120042,
	0x0F120048,
	0x0F12004E,
	0x0F120054,
	0x0F12005A,
	0x0F120060,
	0x0F120066,
	0x0F12006C,
	0x0F120072,
	0x0F120078,
	0x0F12007E,
	0x0F120084,
	0x0F12008A,
	0x0F120090,
	0x0F120096,
	0x0F12009C,
	0x0F1200A2,
	0x0F1200A8,
	0x0F1200AE,
	0x0F1200B4,
	0x0F1200BA,

	0x002A0266,
	0x0F120000, /*REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /*REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /*REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /*REG_TC_GP_PrevConfigChanged */
	0x002A0270,
	0x0F120001, /*REG_TC_GP_CapConfigChanged */
	0x002A023E,
	0x0F120001, /*REG_TC_GP_EnablePreview */
	0x0F120001, /*REG_TC_GP_EnablePreviewChanged */
};

static const u32 s5k4ec_scene_portrait[] = {
	0x00287000,
	0x002A0A28,
	0x0F124020,
	0x002A0ADE,
	0x0F124020,
	0x002A0B94,
	0x0F124020,
	0x002A0C4A,
	0x0F124020,
	0x002A0D00,
	0x0F124020,
};

static const u32 s5k4ec_scene_nightshot[] = {
	0xFCFCD000,
	0x00287000,
	0x002A06B8,
	0x0F12FFFF, /* lt_uMaxLei */
	0x0F1200FF, /* lt_usMinExp */

	0x002A098C,
	0x0F12FFF0, /* 7000098C AFIT16_BRIGHTNESS */
	0x002A0A1E,
	0x0F120A78, /*_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */

	/* GAS Alpha Table */
	0x002A08F6,
	0x0F124000, /* TVAR_ash_GASAlpha_0__0_ R  // 2300K */
	0x0F124000, /* TVAR_ash_GASAlpha_0__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_0__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_0__3_ B */
	0x0F124000, /* TVAR_ash_GASAlpha_1__0_ R  // 2750K */
	0x0F124000, /* TVAR_ash_GASAlpha_1__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_1__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_1__3_ B */
	0x0F124500, /* TVAR_ash_GASAlpha_2__0_ R  // 3300K */
	0x0F124000, /* TVAR_ash_GASAlpha_2__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_2__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_2__3_ B */
	0x0F124500, /* TVAR_ash_GASAlpha_3__0_ R  // 4150K */
	0x0F124000, /* TVAR_ash_GASAlpha_3__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_3__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_3__3_ B */
	0x0F124500, /* TVAR_ash_GASAlpha_4__0_ R  // 5250K */
	0x0F124000, /* TVAR_ash_GASAlpha_4__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_4__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_4__3_ B */
	0x0F124500, /* TVAR_ash_GASAlpha_5__0_ R  // 6400K */
	0x0F124000, /* TVAR_ash_GASAlpha_5__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_5__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_5__3_ B */
	0x0F124500, /* TVAR_ash_GASAlpha_6__0_ R  // 7500K */
	0x0F124000, /* TVAR_ash_GASAlpha_6__1_ GR */
	0x0F124000, /* TVAR_ash_GASAlpha_6__2_ GB */
	0x0F124000, /* TVAR_ash_GASAlpha_6__3_ B */

	0x002A0638,
	0x0F120001,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_0_ */
	0x0F120A3C,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_1_ */
	0x0F120D05,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_2_ */
	0x0F123408,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_3_ */
	0x0F123408,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_4_ */
	0x0F126810,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_5_ */
	0x0F128214,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_6_ */
	0x0F121A80,
	0x0F120006, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_7_ */
	0x0F121A80,
	0x0F120006, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_8_ */
	0x0F121A80,
	0x0F120006, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_9_ */

	0x002A02C2,
	0x0F1207D0,
	0x0F12014A, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 014Ah:30fps */

	0x002A03B4,
	0x0F121388, /* REG_0TC_CCFG_usMaxFrTimeMsecMult10 1388h:2fps */
	0x0F121388, /* REG_0TC_CCFG_usMinFrTimeMsecMult10 1388h:2fps */

	0x002A1648, /*af_search_usSingleAfFlags */
	0x0F129000,

	0x002A15E8,
	0x0F120006, /* af_pos_usTableLastInd */
	0x0F120036,
	0x0F12003A,
	0x0F120040,
	0x0F120048,
	0x0F120050,
	0x0F120058,
	0x0F120060,

	0x002A0266,
	0x0F120000, /*REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /*REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /*REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /*REG_TC_GP_PrevConfigChanged */
	0x002A0270,
	0x0F120001, /*REG_TC_GP_CapConfigChanged */
	0x002A023E,
	0x0F120001, /*REG_TC_GP_EnablePreview */
	0x0F120001, /*REG_TC_GP_EnablePreviewChanged */
};

static const u32 s5k4ec_scene_backlight[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1492,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120101,
	0x0F120101,
	0x0F120000,
	0x0F120000,
	0x0F120F01,
	0x0F12010F,
	0x0F120000,
	0x0F120000,
	0x0F120F01,
	0x0F12010F,
	0x0F120000,
	0x0F120000,
	0x0F120101,
	0x0F120101,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000,
	0x0F120000
};

static const u32 s5k4ec_scene_landscape[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1492,
	0x0F120101, /* ae_WeightTbl_16 */
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,

	0x002A0A28,
	0x0F12E082, /* _ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x002A0ADE,
	0x0F12E082, /* _ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x002A0B94,
	0x0F12E082, /* _ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x002A0C4A,
	0x0F12E082, /* _ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x002A0D00,
	0x0F12E082, /* _ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */

	0x002A0234,
	0x0F120030, /* REG_TC_UserSaturation */
};

static const u32 s5k4ec_scene_sports[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0544,
	0x0F120130, /* lt_uLimitHigh */
	0x0F1200D0, /* lt_uLimitLow */

	0x002A0638,
	0x0F120001,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_0_ */
	0x0F120A3C,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_1_ */
	0x0F120D05,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_2_ */
	0x0F123408,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_3_ */
	0x0F123408,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_4_ */
	0x0F123408,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_5_ */
	0x0F123408,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_6_ */
	0x0F123408,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_7_ */
	0x0F123408,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_8_ */
	0x0F123408,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_9_ */
	0x002A0938,
	0x0F120001, /* afit_bUseNB_Afit */
	0x002A04D0,
	0x0F120003, /* REG_SF_USER_IsoType */
	0x0F120200, /* REG_SF_USER_IsoVal */
	0x0F120001, /* REG_SF_USER_IsoChanged */
	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
	0x002A023E,
	0x0F120001, /* REG_TC_GP_EnablePreview */
	0x0F120001, /* REG_TC_GP_EnablePreviewChanged */
};

static const u32 s5k4ec_scene_party_indoor[] = {
	0xFCFCD000,
	0x00287000,

	0x002A0938,
	0x0F120001, /* afit_bUseNB_Afit */

	0x002A04D0,
	0x0F120001, /* REG_SF_USER_IsoType */
	0x0F120377, /* REG_SF_USER_IsoVal */
	0x0F120001, /* REG_SF_USER_IsoChanged */
	0x002A06C2,
	0x0F120180, /* lt_bUseSecISODgain */

	0x002A0234,
	0x0F120030, /*REG_TC_UserSaturation */
};

static const u32 s5k4ec_scene_beach_snow[] = {
	0xFCFCD000,
	0x00287000,

	0x002A1484,
	0x0F120045, /* TVAR_ae_BrAve */

	0x002A0938,
	0x0F120001, /* afit_bUseNB_Afit */

	0x002A04D0,
	0x0F120001, /* REG_SF_USER_IsoType */
	0x0F1200D0, /* REG_SF_USER_IsoVal */
	0x0F120001, /* REG_SF_USER_IsoChanged */
	0x002A06C2,
	0x0F120150, /* lt_bUseSecISODgain */

	0x002A0234,
	0x0F120030, /* REG_TC_UserSaturation */
};

static const u32 s5k4ec_scene_sunset[] = {
	0xFCFCD000,
	0x00287000,
	0x002A04E6,
	0x0F120777, /* REG_TC_DBG_AutoAlgEnBits AWB Off */

	0x002A04BA,
	0x0F1204DA,

	0x002A04BE,
	0x0F120400,

	0x002A04C2,
	0x0F120550,

	0x002A04C6,
	0x0F120001, /* REG_SF_USER_RGBGainChanged */
};

static const u32 s5k4ec_scene_duskdawn[] = {
	0xFCFCD000,
	0x00287000,
	0x002A04E6,
	0x0F120777, /* REG_TC_DBG_AutoAlgEnBits AWB Off */

	0x002A04BA,
	0x0F120558,

	0x002A04BE,
	0x0F120400,

	0x002A04C2,
	0x0F120955,

	0x002A04C6, /* REG_SF_USER_RGBGainChanged */
	0x0F120001,
};

static const u32 s5k4ec_scene_fall_color[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0234,
	0x0F120060,
};

static const u32 s5k4ec_scene_fireworks[] = {
	0xFCFCD000,
	0x00287000,

	/* AE_state */
	0x002A0544,
	0x0F12012C, /* lt_uLimitHigh */
	0x0F1200D4, /* lt_uLimitLow */

	0x002A0638,
	0x0F120001,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_0_ */
	0x0F120A3C,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_1_ */
	0x0F120D05,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_2_ */
	0x0F123408,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_3_ */
	0x0F123408,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_4_ */
	0x0F12D020,
	0x0F120000, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_5_ */
	0x0F120428,
	0x0F120001, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_6_ */
	0x0F121A80,
	0x0F120006, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_7_ */
	0x0F121A80,
	0x0F120006, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_8_ */
	0x0F121A80,
	0x0F120006, /* lt_ExpGain_ExpCurveGainMaxStr_0__ulExpOut_9_ */

	0x002A02C2,
	0x0F1203E8, /* REG_0TC_PCFG_usMaxFrTimeMsecMult10 09C4h:4fps */
	0x0F12014A, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 014Ah:30fps */

	0x002A03B4,
	0x0F122710, /* REG_0TC_CCFG_usMaxFrTimeMsecMult10 // 2710h:1fps */
	0x0F122710, /* REG_0TC_CCFG_usMinFrTimeMsecMult10 // 2710h:1fps */

	0x002A04D0,
	0x0F120001, /* REG_SF_USER_IsoType */
	0x0F120100, /* REG_SF_USER_IsoVal */
	0x0F120001, /* REG_SF_USER_IsoChanged */
	0x002A06C2,
	0x0F120180, /* lt_bUseSecISODgain */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
	0x002A023E,
	0x0F120001, /* REG_TC_GP_EnablePreview */
	0x0F120001, /* REG_TC_GP_EnablePreviewChanged */
};

static const u32 s5k4ec_scene_text[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0A28,
	0x0F12A060, /* _ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x002A0ADE,
	0x0F12A060, /* _ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x002A0B94,
	0x0F12A060, /* _ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x002A0C4A,
	0x0F12A060, /* _ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x002A0D00,
	0x0F12A060, /* _ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
};

static const u32 s5k4ec_scene_candle_light[] = {
	0xFCFCD000,
	0x00287000,
	0x002A04E6,
	0x0F120777, /* REG_TC_DBG_AutoAlgEnBits AWB Off */

	0x002004BA,
	0x0F1004DA,

	0x002A04BE,
	0x0F120400,

	0x002A04C2,
	0x0F120550,

	0x002A04C6,
	0x0F120001, /* REG_SF_USER_RGBGainChanged */
};

static const u32 s5k4ec_night_capture[] = {
	0x002A0608,
	0x0F120001, /* #lt_ExpGain_uSubsamplingmode */
	0x0F120001, /* #lt_ExpGain_uNonSubsampling */
	0x0F120900, /* #lt_ExpGain_ExpCurveGainMaxStr */
};

static const u32 s5k4ec_af_return_macro_pos[] = {
	0x002A15E8,
	0x0F120018,
	0x0F12002A,
	0x0F120030,
	0x0F120036,
	0x0F12003C,
	0x0F120042,
	0x0F120048,
	0x0F12004E,
	0x0F120054,
	0x0F12005A,
	0x0F120060,
	0x0F120066,
	0x0F12006C,
	0x0F120072,
	0x0F120078,
	0x0F12007E,
	0x0F120084,
	0x0F12008A,
	0x0F120090,
	0x0F120096,
	0x0F12009C,
	0x0F1200A2,
	0x0F1200A8,
	0x0F1200AE,
	0x0F1200B4,
	0x0F1200BA,
};

static const u32 s5k4ec_af_normal_mode_1[] = {
	0xFCFCD000,
	0x00287000,
	0x002A028E,
	0x0F120000,
};

static const u32 s5k4ec_af_normal_mode_2[] = {
	0xFCFCD000,
	0x00287000,
	0x002A028C,
	0x0F120004,

};

static const u32 s5k4ec_af_normal_mode_3[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1648,
	0x0F129002,

	0x002A15E8,
	0x0F120015,
	0x0F120032,
	0x0F120038,
	0x0F12003E,
	0x0F120044,
	0x0F12004A,
	0x0F120050,
	0x0F120056,
	0x0F12005C,
	0x0F120062,
	0x0F120068,
	0x0F12006E,
	0x0F120074,
	0x0F12007A,
	0x0F120080,
	0x0F120086,
	0x0F12008C,
	0x0F120092,
	0x0F120098,
	0x0F12009E,
	0x0F1200A4,
	0x0F1200AA,
	0x0F1200B0,
};

static const u32 s5k4ec_af_macro_mode_1[] = {
	0xFCFCD000,
	0x00287000,
	0x002A028E,
	0x0F1200D0,
};

static const u32 s5k4ec_af_macro_mode_2[] = {
	0xFCFCD000,
	0x00287000,
	0x002A028C,
	0x0F120004, /* REG_TC_AF_AfCmd */
};

static const u32 s5k4ec_af_macro_mode_3[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1648,
	0x0F129042,

	0x002A15E8,
	0x0F120017,
	0x0F120032,
	0x0F120038,
	0x0F12003E,
	0x0F120044,
	0x0F12004A,
	0x0F120050,
	0x0F120056,
	0x0F12005C,
	0x0F120062,
	0x0F120068,
	0x0F12006E,
	0x0F120074,
	0x0F12007A,
	0x0F120080,
	0x0F120086,
	0x0F12008C,
	0x0F120092,
	0x0F120098,
	0x0F12009E,
	0x0F1200A4,
	0x0F1200AA,
	0x0F1200B0,
	0x0F1200C0,
	0x0F1200D0,

	0x002A15DA,
	0x0F121700,
};

static const u32 s5k4ec_af_low_light_mode_on[] = {
	0x002A15DA,
	0x0F120C00,

	0x002A15E8,
	0x0F12000C,
	0x0F12002A,
	0x0F120033,
	0x0F12003C,
	0x0F120045,
	0x0F12004E,
	0x0F120057,
	0x0F120063,
	0x0F12006F,
	0x0F12007B,
	0x0F120087,
	0x0F120093,
	0x0F1200A2,
	0x0F1200B1,
};

static const u32 s5k4ec_af_low_light_mode_off[] = {
	0x002A15DA,
	0x0F121800,

	0x002A15E8,
	0x0F120018,
	0x0F12002A,
	0x0F120030,
	0x0F120036,
	0x0F12003C,
	0x0F120042,
	0x0F120048,
	0x0F12004E,
	0x0F120054,
	0x0F12005A,
	0x0F120060,
	0x0F120066,
	0x0F12006C,
	0x0F120072,
	0x0F120078,
	0x0F12007E,
	0x0F120084,
	0x0F12008A,
	0x0F120090,
	0x0F120096,
	0x0F12009C,
	0x0F1200A2,
	0x0F1200A8,
	0x0F1200AE,
	0x0F1200B4,
	0x0F1200BA,
};

static const u32 s5k4ec_single_af_start[] = {
	0xFCFCD000,
	0x00287000,
	0x002A028C,
	0x0F120005,
};

static const u32 s5k4ec_single_af_off_1[] = {
	0xFCFCD000,
	0x00287000,
	0x002A028E,
	0x0F120000,
	0x002A028C,
	0x0F120004,
};

static const u32 s5k4ec_single_af_off_2[] = {
	0xFCFCD000,
	0x00287000,
	0x002A028E,
	0x0F120000,
};

static const u32 s5k4ec_single_af_off_3[] = {

};

/*==========================================================
// 08.AF Setting
//=========================================================*/
/* AF Lens Position Table Settings */
static const u32 s5k4ec_continuous_af_off[] = {
	0x002A15E8,
	0x0F120010, /* af_pos_usTableLastInd */

	/* Peak Threshold */
	0x002A164C,
	0x0F120003, /* af_search_usMinPeakSamples */
	0x002A163E,
	0x0F1200C0, /* af_search_usPeakThr */
	0x0F120080, /* af_search_usPeakThrLow */

	0x0F120018, /* af_pos_usTable */
	0x0F120020, /* af_pos_usTable */
	0x0F120028, /* af_pos_usTable */
	0x0F120030, /* af_pos_usTable */
	0x0F120038, /* af_pos_usTable */
	0x0F120040, /* af_pos_usTable */
	0x0F120048, /* af_pos_usTable */
	0x0F120050, /* af_pos_usTable */
	0x0F120058, /* af_pos_usTable */
	0x0F120060, /* af_pos_usTable */
	0x0F120068, /* af_pos_usTable */
	0x0F120070, /* af_pos_usTable */
	0x0F120080, /* af_pos_usTable */
	0x0F120090, /* af_pos_usTable */
	0x0F1200A0, /* af_pos_usTable */
	0x0F1200B0, /* af_pos_usTable */
	0x0F1200C0, /* af_pos_usTable */

	/* VCM AF driver with PWM/I2C */
	0x002A1722,
	0x0F128000, /* afd_usParam[0] I2C power down command */
	0x0F120006, /* afd_usParam[1] Position Right Shift */
	0x0F123FF0, /* afd_usParam[2] I2C Data Mask */
	0x0F1203E8, /* afd_usParam[3] PWM Period */
	0x0F120000, /* afd_usParam[4] PWM Divider */
	0x0F120020, /* afd_usParam[5] SlowMotion Delay 4.  */
	0x0F120010, /* afd_usParam[6] SlowMotion Threshold */
	0x0F120008, /* afd_usParam[7] Signal Shaping */
	0x0F120040, /* afd_usParam[8] Signal Shaping level */
	0x0F120080, /* afd_usParam[9] Signal Shaping level */
	0x0F1200C0, /* afd_usParam[10] Signal Shaping level */
	0x0F1200E0, /* afd_usParam[11] Signal Shaping level */

	0x002A028C,
	0x0F120001,  /* REG_TC_AF_AfCmd   Abort */
};

static const u32 s5k4ec_continuous_af_on[] = {
	0xFCFCD000,

	/* 4. AF Peak Threshold Setting */
	0x002A164C,
	0x0F120003, /* #af_search_usMinPeakSamples */
	0x002A163E,
	0x0F1200F0,

	0x0F1200D0, /* #af_search_usPeakThrLow,  Continuous AF tuning point */

	0x00287000,
	0x002A15E8,
	0x0F120018, /* #af_pos_usTableLastInd// table0 ~ table24,  25 Steps */
	0x0F120020, /* #af_pos_usTable_0_ // af_pos_usTable */
	0x0F120024, /* #af_pos_usTable_1_ */
	0x0F120028, /* #af_pos_usTable_2_ */
	0x0F12002C, /* #af_pos_usTable_3_ */
	0x0F120030, /* #af_pos_usTable_4_ */
	0x0F120034, /* #af_pos_usTable_5_ */
	0x0F120038, /* #af_pos_usTable_6_ */
	0x0F12003C, /* #af_pos_usTable_7_ */
	0x0F120040, /* #af_pos_usTable_8_ */
	0x0F120044, /* #af_pos_usTable_9_ */
	0x0F120048, /* #af_pos_usTable_10_ */
	0x0F12004C, /* #af_pos_usTable_11_ */
	0x0F120050, /* #af_pos_usTable_12_ */
	0x0F120054, /* #af_pos_usTable_13_ */
	0x0F120058, /* #af_pos_usTable_14_ */
	0x0F12005C, /* #af_pos_usTable_15_ */
	0x0F120060, /* #af_pos_usTable_16_ */
	0x0F120064, /* #af_pos_usTable_17_ */
	0x0F120068, /* #af_pos_usTable_18_ */
	0x0F12006C, /* #af_pos_usTable_19_ */
	0x0F120070, /* #af_pos_usTable_20_ */
	0x0F120074, /* #af_pos_usTable_21_ */
	0x0F120078, /* #af_pos_usTable_22_ */
	0x0F12007C, /* #af_pos_usTable_23_ */
	0x0F120080, /* #af_pos_usTable_24_ */

	/* 7. Continuous AF,  Continuous AF tuning point */
	/* 7-1. Continuous AF,  timing */
	0x002A16EE,
	0x0F120028, /* #af_refocus_usFlframes,  28h = 40frame */
	0x002A170E,
	0x0F12000A, /* Unit : x4 frame,  0Ah x 4 = 40frame */

	/* 7-2. Continuous AF,  sensitivity */
	0x002A16EA,
	0x0F123030, /* #af_refocus_usFlUpLow,  normal case, */
	/* MSB : upper boundary(Thr_High),  LSB : lower boundary(Thr_Low), */
	0x0F123030, /* #af_refocus_usFlLUpLow,  Low confidence case, */
	/* MSB : upper boundary(Thr_High),  LSB : lower boundary(Thr_Low), */

	/* 7-3. Lens movement for continuous AF */
	0x002A15D8,
	0x0F128080, /* #af_pos_usMiddlePos MSB : macro,
		LSB : normal,  lens moving direction */

	/* 9. VCM AF driver with PWM/I2C */
	0x002A1722,
	0x0F128000, /* #afd_usParam[0]  I2C power down command */
	0x0F120006, /* #afd_usParam[1]  Position Right Shift */
	0x0F123FF0, /* #afd_usParam[2]  I2C Data Mask */
	0x0F1203E8, /* #afd_usParam[3]  PWM Period */
	0x0F120000, /* #afd_usParam[4]  PWM Divider */
	0x0F120070, /* #afd_usParam[5]  SlowMotion Delay
		reduce lens collision noise. Continuous AF tuning point */
	0x0F120004, /* #afd_usParam[6]  SlowMotion Threshold,
		Continuous AF tuning point */
	0x0F120048, /* #afd_usParam[7]  Signal Shaping,
		Continuous AF tuning point */
	0x0F120040, /* #afd_usParam[8]  Signal Shaping level */
	0x0F120080, /* #afd_usParam[9]  Signal Shaping level */
	0x0F1200C0, /* #afd_usParam[10] Signal Shaping level */
	0x0F120000, /* #afd_usParam[11] Signal Shaping level */

	0x002A028E,
	0x0F120000,

	0x002A028C,
	0x0F120006,  /* REG_TC_AF_AfCmd   init,  AF initialization */
};

static const u32 s5k4ec_face_detection_on[] = {
	0x002A0294,
	0x0F120100,
	0x0F1200E3,
	0x0F120200,
	0x0F120238,
	0x0F1201C6,
	0x0F120166,
	0x0F120074,
	0x0F120132,
	0x0F120001,
};

static const u32 s5k4ec_face_detection_off[] = {
	0x002A0294,
	0x0F120100,
	0x0F1200E3,
	0x0F120200,
	0x0F120238,
	0x0F1201C6,
	0x0F120166,
	0x0F120074,
	0x0F120132,
	0x0F120001,
};

static const u32 s5k4ec_low_cap_on[] = {
	0x002A06B8,
	0x0F12FFFF,
	0x0F1200FF, /* #lt_uMaxLei */

	0x002A0A1A,
	0x0F124A18, /* #Gamma linearity */

	0x002A0608,
	0x0F120001, /* #lt_ExpGain_uSubsamplingmode */
	0x0F120001, /* #lt_ExpGain_uNonSubsampling */
	0x0F120850, /* #lt_ExpGain_ExpCurveGainMaxStr */

	0x002A0938,
	0x0F120001,
	0x0F120012, /* #SARR_uNormBrInDoor_0_ */
	0x0F120022, /* #SARR_uNormBrInDoor_1_ */
	0x0F120384, /* #SARR_uNormBrInDoor_2_ */
	0x0F1207D0, /* #SARR_uNormBrInDoor_3_ */
	0x0F121388, /* #SARR_uNormBrInDoor_4_ */
};

static const u32 s5k4ec_low_cap_off[] = {
	0x002A06B8,
	0x0F12802C,
	0x0F120005, /* #lt_uMaxLei */

	0x002A0A1A,
	0x0F123718, /* #Gamma linearity */

	0x002A0608,
	0x0F120001, /* #lt_ExpGain_uSubsamplingmode */
	0x0F120001, /* #lt_ExpGain_uNonSubsampling */
	0x0F120800, /* #lt_ExpGain_ExpCurveGainMaxStr */

	0x002A0938,
	0x0F120000,
	0x0F120014, /* #SARR_uNormBrInDoor_0_ */
	0x0F1200D2, /* #SARR_uNormBrInDoor_1_ */
	0x0F120384, /* #SARR_uNormBrInDoor_2_ */
	0x0F1207D0, /* #SARR_uNormBrInDoor_3_ */
	0x0F121388, /* #SARR_uNormBrInDoor_4_ */
};

/* restores crop settings to full resolution */
static const u32 s5k4ec_reset_crop[] = {
	0x002A0250,
	0x0F120A00,
	0x0F120780,
	0x0F120010,
	0x0F12000C,
	0x0F120A00,
	0x0F120780,
	0x0F120010,
	0x0F12000C,
	0x002A0494,
	0x0F120A00,
	0x0F120780,
	0x0F120000,
	0x0F120000,
	0x0F120A00,
	0x0F120780,
	0x0F120000,
	0x0F120000,

	0x002A0262,
	0x0F120001,
	0x0F120001,
};

static const u32 s5k4ec_capture_start[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0242,
	0x0F120001,
	0x002A024E,
	0x0F120001,
	0x002A0244,
	0x0F120001,
};

static const u32 s5k4ec_preview_return[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0242,
	0x0F120000, /* REG_TC_GP_EnableCapture */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0244,
	0x0F120001, /* REG_TC_GP_EnableCaptureChanged */
};

static const u32 s5k4ec_camcorder[] = {
	0xFCFCD000,
	0x0028D000,
	0x002AE410,
	0x0F123E01,
	0x00287000,


	/* METERING */
	0x002A1492,	/* Matrix */
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,
	0x0F120101,

	/* SHARPNESS n NOISE */
	0x002A0938,
	0x0F120001, /* afit_bUseNB_Afit on 1  off 0 */
	0x0F120014, /* SARR_uNormBrInDoor_0_ */
	0x0F1200D2, /* SARR_uNormBrInDoor_1_ */
	0x0F120784, /* SARR_uNormBrInDoor_2_ */
	0x0F1210D0, /* SARR_uNormBrInDoor_3_ */
	0x0F121388, /* SARR_uNormBrInDoor_4_ */

	0x002A098C, /* AFIT 0 */
	0x0F120024, /* 7000098C AFIT16_BRIGHTNESS */
	0x0F120010, /* 7000098E AFIT16_CONTRAST */
	0x0F120000, /* 70000990 AFIT16_SATURATION */
	0x0F120000, /* 70000992 AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000994 AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000996 AFIT16_bnr_edge_high */
	0x0F120064, /* 70000998 AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 7000099A AFIT16_postdmsc_iHighBright */
	0x0F120051, /* 7000099C AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 7000099E AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 700009A0 AFIT16_postdmsc_iTune */
	0x0F120040, /* 700009A2 AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 700009A4 AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 700009A6 AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 700009A8 AFIT16_yuvemix_mPosRanges_0 */
	0x0F120060, /* 700009AA AFIT16_yuvemix_mPosRanges_1 */
	0x0F120100, /* 700009AC AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 700009AE AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 700009B0 AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 700009B2 AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F122404, /* 700009B4 AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F12031B, /* 700009B6 AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120103, /* 700009B8 AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F121205, /* 700009BA AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F12400D, /* 700009BC AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120080, /* 700009BE AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F122080, /* 700009C0 AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F123040, /* 700009C2 AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120630, /* 700009C4 AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 700009C6 AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 700009C8 AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 700009CA AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120404, /* 700009CC AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120300, /* 700009CE AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12245A, /* 700009D0 AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F121018, /* 700009D2 AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 700009D4 AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F120B00, /* 700009D6 AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 700009D8 AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120505, /* 700009DA AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 700009DC AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 700009DE AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 700009E0 AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F123428, /* 700009E2 AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F12041C, /* 700009E4 AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 700009E6 AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F120800, /* 700009E8 AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F121004, /* 700009EA AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 700009EC AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 700009EE AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 700009F0 AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 700009F2 AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 700009F4 AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 700009F6 AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120000, /* 700009F8 AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 700009FA AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 700009FC AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 700009FE AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000A00 AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000A02 AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000A04 AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120205, /* 70000A06 AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000A08 AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000A0A AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120306, /* 70000A0C AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120407, /* 70000A0E AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F121804, /* 70000A10 AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F120214, /* 70000A12 AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121002, /* 70000A14 AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120610, /* 70000A16 AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000A18 AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F123218, /* 70000A1A AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F1200B0, /* 70000A1C AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120332, /* 70000A1E AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000A20 AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000A22 AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000A24 AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F121B24, /* 70000A26 AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000A28 AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F121D22, /* 70000A2A AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000A2C AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000A2E AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000A30 AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000A32 AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F122401, /* 70000A34 AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F12241B, /* 70000A36 AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F121E60, /* 70000A38 AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF18, /* 70000A3A AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000A3C AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000A3E AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000A40 AFITB_bnr_nClustLevel_C [0]   bWideWide[1] */
	0x0F120000, /* 70000A42 AFIT16_BRIGHTNESS */
	0x0F120000, /* 70000A44 AFIT16_CONTRAST */
	0x0F120000, /* 70000A46 AFIT16_SATURATION */
	0x0F120000, /* 70000A48 AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000A4A AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000A4C AFIT16_bnr_edge_high */
	0x0F120064, /* 70000A4E AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 70000A50 AFIT16_postdmsc_iHighBright */
	0x0F120051, /* 70000A52 AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 70000A54 AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 70000A56 AFIT16_postdmsc_iTune */
	0x0F120040, /* 70000A58 AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000A5A AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000A5C AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000A5E AFIT16_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000A60 AFIT16_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000A62 AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000A64 AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 70000A66 AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 70000A68 AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F121B04, /* 70000A6A AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F120312, /* 70000A6C AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120003, /* 70000A6E AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F120C03, /* 70000A70 AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F122806, /* 70000A72 AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120060, /* 70000A74 AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F121540, /* 70000A76 AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F12201C, /* 70000A78 AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120620, /* 70000A7A AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 70000A7C AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 70000A7E AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 70000A80 AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120404, /* 70000A82 AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120300, /* 70000A84 AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12145A, /* 70000A86 AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F121010, /* 70000A88 AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 70000A8A AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F120B00, /* 70000A8C AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 70000A8E AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120503, /* 70000A90 AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 70000A92 AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 70000A94 AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 70000A96 AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F123C28, /* 70000A98 AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F120428, /* 70000A9A AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 70000A9C AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F128000, /* 70000A9E AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F121004, /* 70000AA0 AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 70000AA2 AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 70000AA4 AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 70000AA6 AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 70000AA8 AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 70000AAA AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 70000AAC AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120000, /* 70000AAE AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 70000AB0 AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 70000AB2 AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 70000AB4 AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000AB6 AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000AB8 AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000ABA AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120205, /* 70000ABC AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000ABE AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000AC0 AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120306, /* 70000AC2 AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120407, /* 70000AC4 AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F121804, /* 70000AC6 AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F120214, /* 70000AC8 AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121002, /* 70000ACA AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120610, /* 70000ACC AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000ACE AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F126418, /* 70000AD0 AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F1200A0, /* 70000AD2 AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120250, /* 70000AD4 AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000AD6 AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000AD8 AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000ADA AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F121B24, /* 70000ADC AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000ADE AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F120C0C, /* 70000AE0 AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000AE2 AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000AE4 AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000AE6 AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000AE8 AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F121501, /* 70000AEA AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F12240F, /* 70000AEC AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F120C60, /* 70000AEE AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF0C, /* 70000AF0 AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000AF2 AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000AF4 AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000AF6 AFITB_bnr_nClustLevel_C [0]   bWideWide[1] */
	0x0F120000, /* 70000AF8 AFIT16_BRIGHTNESS */
	0x0F120000, /* 70000AFA AFIT16_CONTRAST */
	0x0F120000, /* 70000AFC AFIT16_SATURATION */
	0x0F120000, /* 70000AFE AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000B00 AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000B02 AFIT16_bnr_edge_high */
	0x0F120064, /* 70000B04 AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 70000B06 AFIT16_postdmsc_iHighBright */
	0x0F120043, /* 70000B08 AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 70000B0A AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 70000B0C AFIT16_postdmsc_iTune */
	0x0F120040, /* 70000B0E AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000B10 AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000B12 AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000B14 AFIT16_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000B16 AFIT16_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000B18 AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000B1A AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 70000B1C AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 70000B1E AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F121B04, /* 70000B20 AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F120312, /* 70000B22 AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120003, /* 70000B24 AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F120C03, /* 70000B26 AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F122806, /* 70000B28 AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120060, /* 70000B2A AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F121540, /* 70000B2C AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F12201C, /* 70000B2E AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120620, /* 70000B30 AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 70000B32 AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 70000B34 AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 70000B36 AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120404, /* 70000B38 AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120300, /* 70000B3A AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12145A, /* 70000B3C AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F121010, /* 70000B3E AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 70000B40 AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F120E00, /* 70000B42 AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 70000B44 AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120503, /* 70000B46 AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 70000B48 AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 70000B4A AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 70000B4C AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F123C28, /* 70000B4E AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F120428, /* 70000B50 AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 70000B52 AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F128000, /* 70000B54 AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F120A04, /* 70000B56 AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 70000B58 AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 70000B5A AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 70000B5C AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 70000B5E AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 70000B60 AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 70000B62 AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120000, /* 70000B64 AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 70000B66 AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 70000B68 AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 70000B6A AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000B6C AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000B6E AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000B70 AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120205, /* 70000B72 AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000B74 AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000B76 AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120306, /* 70000B78 AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120407, /* 70000B7A AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F121804, /* 70000B7C AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F120214, /* 70000B7E AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121002, /* 70000B80 AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120610, /* 70000B82 AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000B84 AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F126E18, /* 70000B86 AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F120080, /* 70000B88 AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120080, /* 70000B8A AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000B8C AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000B8E AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000B90 AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F12141D, /* 70000B92 AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000B94 AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F120C0C, /* 70000B96 AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000B98 AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000B9A AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000B9C AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000B9E AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F121501, /* 70000BA0 AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F124C0F, /* 70000BA2 AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F120C88, /* 70000BA4 AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF0C, /* 70000BA6 AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000BA8 AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000BAA AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000BAC AFITB_bnr_nClustLevel_C [0]   bWideWide[1] */
	0x0F120000, /* 70000BAE AFIT16_BRIGHTNESS */
	0x0F120000, /* 70000BB0 AFIT16_CONTRAST */
	0x0F120000, /* 70000BB2 AFIT16_SATURATION */
	0x0F120000, /* 70000BB4 AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000BB6 AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000BB8 AFIT16_bnr_edge_high */
	0x0F120064, /* 70000BBA AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 70000BBC AFIT16_postdmsc_iHighBright */
	0x0F120032, /* 70000BBE AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 70000BC0 AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 70000BC2 AFIT16_postdmsc_iTune */
	0x0F120040, /* 70000BC4 AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000BC6 AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000BC8 AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000BCA AFIT16_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000BCC AFIT16_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000BCE AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000BD0 AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 70000BD2 AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 70000BD4 AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F121504, /* 70000BD6 AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F12030F, /* 70000BD8 AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120003, /* 70000BDA AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F120902, /* 70000BDC AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F122004, /* 70000BDE AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120050, /* 70000BE0 AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F121140, /* 70000BE2 AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F12201C, /* 70000BE4 AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120620, /* 70000BE6 AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 70000BE8 AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 70000BEA AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 70000BEC AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120404, /* 70000BEE AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120300, /* 70000BF0 AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12145A, /* 70000BF2 AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F121010, /* 70000BF4 AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 70000BF6 AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F121000, /* 70000BF8 AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 70000BFA AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120503, /* 70000BFC AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 70000BFE AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 70000C00 AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 70000C02 AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F123C28, /* 70000C04 AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F12042C, /* 70000C06 AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 70000C08 AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F128000, /* 70000C0A AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F120904, /* 70000C0C AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 70000C0E AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 70000C10 AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 70000C12 AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 70000C14 AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 70000C16 AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 70000C18 AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120000, /* 70000C1A AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 70000C1C AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 70000C1E AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 70000C20 AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000C22 AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000C24 AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000C26 AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120205, /* 70000C28 AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000C2A AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000C2C AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120306, /* 70000C2E AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120407, /* 70000C30 AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F122804, /* 70000C32 AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F120228, /* 70000C34 AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121402, /* 70000C36 AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120618, /* 70000C38 AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000C3A AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F126E18, /* 70000C3C AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F120080, /* 70000C3E AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120080, /* 70000C40 AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000C42 AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000C44 AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000C46 AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F121117, /* 70000C48 AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000C4A AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F120A0A, /* 70000C4C AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000C4E AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000C50 AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000C52 AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000C54 AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F121501, /* 70000C56 AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F124C0F, /* 70000C58 AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F120A88, /* 70000C5A AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF0A, /* 70000C5C AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000C5E AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000C60 AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000C62 AFITB_bnr_nClustLevel_C [0]   bWideWide[1] */
	0x0F120000, /* 70000C64 AFIT16_BRIGHTNESS */
	0x0F120000, /* 70000C66 AFIT16_CONTRAST */
	0x0F120000, /* 70000C68 AFIT16_SATURATION */
	0x0F120000, /* 70000C6A AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000C6C AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000C6E AFIT16_bnr_edge_high */
	0x0F120064, /* 70000C70 AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 70000C72 AFIT16_postdmsc_iHighBright */
	0x0F120032, /* 70000C74 AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 70000C76 AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 70000C78 AFIT16_postdmsc_iTune */
	0x0F120040, /* 70000C7A AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000C7C AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000C7E AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000C80 AFIT16_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000C82 AFIT16_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000C84 AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000C86 AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 70000C88 AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 70000C8A AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F120F04, /* 70000C8C AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F12030C, /* 70000C8E AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120003, /* 70000C90 AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F120602, /* 70000C92 AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F121803, /* 70000C94 AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120040, /* 70000C96 AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F120E20, /* 70000C98 AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F122018, /* 70000C9A AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120620, /* 70000C9C AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 70000C9E AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 70000CA0 AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 70000CA2 AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120404, /* 70000CA4 AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120200, /* 70000CA6 AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12145A, /* 70000CA8 AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F121010, /* 70000CAA AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 70000CAC AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F121200, /* 70000CAE AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 70000CB0 AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120502, /* 70000CB2 AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 70000CB4 AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 70000CB6 AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 70000CB8 AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F124028, /* 70000CBA AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F120430, /* 70000CBC AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 70000CBE AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F12FF00, /* 70000CC0 AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F120804, /* 70000CC2 AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 70000CC4 AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 70000CC6 AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 70000CC8 AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 70000CCA AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 70000CCC AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 70000CCE AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120000, /* 70000CD0 AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 70000CD2 AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 70000CD4 AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 70000CD6 AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000CD8 AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000CDA AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000CDC AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120205, /* 70000CDE AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000CE0 AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000CE2 AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120306, /* 70000CE4 AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120407, /* 70000CE6 AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F122C04, /* 70000CE8 AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F12022C, /* 70000CEA AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121402, /* 70000CEC AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120618, /* 70000CEE AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000CF0 AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F126E18, /* 70000CF2 AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F120080, /* 70000CF4 AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120080, /* 70000CF6 AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000CF8 AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000CFA AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000CFC AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F120C0F, /* 70000CFE AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000D00 AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F120808, /* 70000D02 AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000D04 AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000D06 AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000D08 AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000D0A AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F120F01, /* 70000D0C AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F12240C, /* 70000D0E AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F120860, /* 70000D10 AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF08, /* 70000D12 AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000D14 AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000D16 AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000D18 AFITB_bnr_nClustLevel_C [0]   bWideWide[1] */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A02AB,
	0x0F120006, /* REG_0TC_PCFG_Format 05:
			yuv (0~255)  06:yuv (16~234) 07: raw 09 : jpeg */
	0x002A0266,
	0x0F120000, /* #REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* #REG_TC_GP_PrevOpenAfterChange  */
	0x002A024E,
	0x0F120001, /* #REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* #REG_TC_GP_PrevConfigChanged */
	0x002A0270,
	0x0F120001, /* #REG_TC_GP_CapConfigChanged */
	0x002A023E,
	0x0F120001, /* #REG_TC_GP_EnablePreview */
	0x0F120001, /* #REG_TC_GP_EnablePreviewChanged */
};

static const u32 s5k4ec_camcorder_disable[] = {
	0xFCFCD000,
	0x0028D000,
	0x002AE410,
	0x0F123804, /*[15:8]fadlc_filter_co_b,
			[7:0]fadlc_filter_co_a*/
	0x00287000,
	0x002A18AC,
	0x0F120060, /*senHal_uAddColsBin        */
	0x0F120060, /*senHal_uAddColsNoBin      */
	0x0F120A20, /*senHal_uMinColsBin        */
	0x0F120AB0, /*senHal_uMinColsNoBin      */

	/* SLOW AE*/
	0x002A1568,
	0x0F120010, /* ae_GainIn_0_ */
	0x0F120020, /* ae_GainIn_1_ */
	0x0F120040, /* ae_GainIn_2_ */
	0x0F120080, /* ae_GainIn_3_ */
	0x0F120100, /* ae_GainIn_4_ FIX */
	0x0F120200, /* ae_GainIn_5_ */
	0x0F120400, /* ae_GainIn_6_ */
	0x0F120800, /* ae_GainIn_7_ */
	0x0F122000, /* ae_GainIn_8_ */
	0x0F120010, /* ae_GainOut_0_ */
	0x0F120020, /* ae_GainOut_1_ */
	0x0F120040, /* ae_GainOut_2_ */
	0x0F120080, /* ae_GainOut_3_ */
	0x0F120100, /* ae_GainOut_4_ FIX */
	0x0F120200, /* ae_GainOut_5_ */
	0x0F120400, /* ae_GainOut_6_ */
	0x0F120800, /* ae_GainOut_7_ */
	0x0F122000, /* ae_GainOut_8_ */

	0x002A0544,
	0x0F120111, /* lt_uLimitHigh */
	0x0F1200EF, /* lt_uLimitLow */

	0x002A0588,
	0x0F120002, /* lt_uInitPostToleranceCnt */

	0x002A0582,
	0x0F120000, /* lt_uSlowFilterCoef */


	0x002A47B0,
	0x0F120000, /*TNP_Regs_BUse1FrameAE	(0: off, 1: on)	*/


	/* SLOW AWB */
	0x002A139A,
	0x0F120258, /* 0258 awbb_GainsMaxMove */

	/* AWB Convergence Speed */
	0x002A1464,
	0x0F120008,
	0x0F120190,
	0x0F1200A0,
	0x0F120004,

	/* SHARPNESS n NOISE */
	0x002A0938,
	0x0F120000, /* on/off AFIT by NB option */
	0x0F120014, /*SARR_uNormBrInDoor */
	0x0F1200D2, /*SARR_uNormBrInDoor */
	0x0F120384, /*SARR_uNormBrInDoor */
	0x0F1207D0, /*SARR_uNormBrInDoor */
	0x0F121388, /*SARR_uNormBrInDoor */

	0x002A098C,
	0x0F12FFFB, /* 7000098C AFIT16_BRIGHTNESS */
	0x0F120000, /* 7000098E AFIT16_CONTRAST */
	0x0F120000, /* 70000990 AFIT16_SATURATION */
	0x0F120000, /* 70000992 AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000994 AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000996 AFIT16_bnr_edge_high */
	0x0F120064, /* 70000998 AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 7000099A AFIT16_postdmsc_iHighBright */
	0x0F12005F, /* 7000099C AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 7000099E AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 700009A0 AFIT16_postdmsc_iTune */
	0x0F120040, /* 700009A2 AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 700009A4 AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 700009A6 AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 700009A8 AFIT16_yuvemix_mPosRanges_0 */
	0x0F120040, /* 700009AA AFIT16_yuvemix_mPosRanges_1 */
	0x0F1200A0, /* 700009AC AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 700009AE AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 700009B0 AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 700009B2 AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F123604, /* 700009B4 AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F12032A, /* 700009B6 AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120103, /* 700009B8 AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F121205, /* 700009BA AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F12400D, /* 700009BC AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120080, /* 700009BE AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F122080, /* 700009C0 AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F123840, /* 700009C2 AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120638, /* 700009C4 AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 700009C6 AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 700009C8 AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 700009CA AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120000, /* 700009CC AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120400, /* 700009CE AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12245A, /* 700009D0 AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F12102A, /* 700009D2 AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 700009D4 AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F120600, /* 700009D6 AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 700009D8 AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120505, /* 700009DA AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 700009DC AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 700009DE AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 700009E0 AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F122828, /* 700009E2 AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F12040F, /* 700009E4 AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 700009E6 AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F120800, /* 700009E8 AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F121804, /* 700009EA AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 700009EC AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 700009EE AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 700009F0 AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 700009F2 AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 700009F4 AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 700009F6 AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120004, /* 700009F8 AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 700009FA AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 700009FC AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 700009FE AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000A00 AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000A02 AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000A04 AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120205, /* 70000A06 AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000A08 AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000A0A AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120306, /* 70000A0C AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120407, /* 70000A0E AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F121C04, /* 70000A10 AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F120214, /* 70000A12 AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121002, /* 70000A14 AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120610, /* 70000A16 AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000A18 AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F122818, /* 70000A1A AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F120080, /* 70000A1C AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120350, /* 70000A1E AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000A20 AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000A22 AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000A24 AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F123141, /* 70000A26 AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000A28 AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F123140, /* 70000A2A AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000A2C AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000A2E AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000A30 AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000A32 AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F123601, /* 70000A34 AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F12242A, /* 70000A36 AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F123660, /* 70000A38 AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF2A, /* 70000A3A AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000A3C AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000A3E AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000A40 AFITB_bnr_nClustLevel_C [0] */
	0x0F120000, /* 70000A42 AFIT16_BRIGHTNESS */
	0x0F120000, /* 70000A44 AFIT16_CONTRAST */
	0x0F12FFFB, /* 70000A46 AFIT16_SATURATION */
	0x0F120000, /* 70000A48 AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000A4A AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000A4C AFIT16_bnr_edge_high */
	0x0F120064, /* 70000A4E AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 70000A50 AFIT16_postdmsc_iHighBright */
	0x0F120051, /* 70000A52 AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 70000A54 AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 70000A56 AFIT16_postdmsc_iTune */
	0x0F120040, /* 70000A58 AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000A5A AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000A5C AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000A5E AFIT16_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000A60 AFIT16_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000A62 AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000A64 AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 70000A66 AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 70000A68 AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F122404, /* 70000A6A AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F12031B, /* 70000A6C AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120103, /* 70000A6E AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F121004, /* 70000A70 AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F123A0C, /* 70000A72 AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120070, /* 70000A74 AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F121C80, /* 70000A76 AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F123030, /* 70000A78 AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120630, /* 70000A7A AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 70000A7C AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 70000A7E AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 70000A80 AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120000, /* 70000A82 AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120300, /* 70000A84 AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12245A, /* 70000A86 AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F121018, /* 70000A88 AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 70000A8A AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F120B00, /* 70000A8C AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 70000A8E AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120505, /* 70000A90 AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 70000A92 AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 70000A94 AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 70000A96 AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F122928, /* 70000A98 AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F120415, /* 70000A9A AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 70000A9C AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F120800, /* 70000A9E AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F121004, /* 70000AA0 AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 70000AA2 AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 70000AA4 AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 70000AA6 AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 70000AA8 AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 70000AAA AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 70000AAC AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120003, /* 70000AAE AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 70000AB0 AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 70000AB2 AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 70000AB4 AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000AB6 AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000AB8 AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000ABA AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120205, /* 70000ABC AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000ABE AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000AC0 AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120306, /* 70000AC2 AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120407, /* 70000AC4 AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F121F04, /* 70000AC6 AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F120218, /* 70000AC8 AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121102, /* 70000ACA AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120611, /* 70000ACC AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000ACE AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F126418, /* 70000AD0 AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F120080, /* 70000AD2 AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120380, /* 70000AD4 AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000AD6 AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000AD8 AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000ADA AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F122832, /* 70000ADC AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000ADE AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F12272C, /* 70000AE0 AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000AE2 AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000AE4 AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000AE6 AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000AE8 AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F122401, /* 70000AEA AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F12241B, /* 70000AEC AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F121E60, /* 70000AEE AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF18, /* 70000AF0 AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000AF2 AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000AF4 AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000AF6 AFITB_bnr_nClustLevel_C [0] */
	0x0F120000, /* 70000AF8 AFIT16_BRIGHTNESS */
	0x0F120000, /* 70000AFA AFIT16_CONTRAST */
	0x0F12FFFB, /* 70000AFC AFIT16_SATURATION */
	0x0F120000, /* 70000AFE AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000B00 AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000B02 AFIT16_bnr_edge_high */
	0x0F120064, /* 70000B04 AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 70000B06 AFIT16_postdmsc_iHighBright */
	0x0F120043, /* 70000B08 AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 70000B0A AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 70000B0C AFIT16_postdmsc_iTune */
	0x0F120040, /* 70000B0E AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000B10 AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000B12 AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000B14 AFIT16_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000B16 AFIT16_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000B18 AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000B1A AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 70000B1C AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 70000B1E AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F121B04, /* 70000B20 AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F120312, /* 70000B22 AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120003, /* 70000B24 AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F120C03, /* 70000B26 AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F122806, /* 70000B28 AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120060, /* 70000B2A AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F121580, /* 70000B2C AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F122020, /* 70000B2E AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120620, /* 70000B30 AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 70000B32 AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 70000B34 AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 70000B36 AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120000, /* 70000B38 AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120300, /* 70000B3A AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12145A, /* 70000B3C AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F121010, /* 70000B3E AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 70000B40 AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F120E00, /* 70000B42 AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 70000B44 AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120504, /* 70000B46 AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 70000B48 AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 70000B4A AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 70000B4C AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F122B28, /* 70000B4E AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F120417, /* 70000B50 AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 70000B52 AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F128000, /* 70000B54 AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F120A04, /* 70000B56 AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 70000B58 AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 70000B5A AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 70000B5C AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 70000B5E AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 70000B60 AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 70000B62 AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120002, /* 70000B64 AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 70000B66 AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 70000B68 AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 70000B6A AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000B6C AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000B6E AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000B70 AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120207, /* 70000B72 AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000B74 AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000B76 AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120306, /* 70000B78 AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120407, /* 70000B7A AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F122404, /* 70000B7C AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F120221, /* 70000B7E AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121202, /* 70000B80 AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120613, /* 70000B82 AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000B84 AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F126E18, /* 70000B86 AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F120080, /* 70000B88 AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120080, /* 70000B8A AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000B8C AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000B8E AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000B90 AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F122630, /* 70000B92 AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000B94 AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F121616, /* 70000B96 AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000B98 AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000B9A AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000B9C AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000B9E AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F121B01, /* 70000BA0 AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F122412, /* 70000BA2 AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F120C60, /* 70000BA4 AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF0C, /* 70000BA6 AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000BA8 AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000BAA AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000BAC AFITB_bnr_nClustLevel_C [0] */
	0x0F120000, /* 70000BAE AFIT16_BRIGHTNESS */
	0x0F120000, /* 70000BB0 AFIT16_CONTRAST */
	0x0F120000, /* 70000BB2 AFIT16_SATURATION */
	0x0F120000, /* 70000BB4 AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000BB6 AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000BB8 AFIT16_bnr_edge_high */
	0x0F120064, /* 70000BBA AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 70000BBC AFIT16_postdmsc_iHighBright */
	0x0F120032, /* 70000BBE AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 70000BC0 AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 70000BC2 AFIT16_postdmsc_iTune */
	0x0F120040, /* 70000BC4 AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000BC6 AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000BC8 AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000BCA AFIT16_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000BCC AFIT16_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000BCE AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000BD0 AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 70000BD2 AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 70000BD4 AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F121504, /* 70000BD6 AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F12030F, /* 70000BD8 AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120003, /* 70000BDA AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F120902, /* 70000BDC AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F122004, /* 70000BDE AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120050, /* 70000BE0 AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F121140, /* 70000BE2 AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F12201C, /* 70000BE4 AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120620, /* 70000BE6 AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 70000BE8 AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 70000BEA AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 70000BEC AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120000, /* 70000BEE AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120300, /* 70000BF0 AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12145A, /* 70000BF2 AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F121010, /* 70000BF4 AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 70000BF6 AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F121000, /* 70000BF8 AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 70000BFA AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120503, /* 70000BFC AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 70000BFE AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 70000C00 AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 70000C02 AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F123028, /* 70000C04 AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F12041A, /* 70000C06 AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 70000C08 AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F12FF00, /* 70000C0A AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F120904, /* 70000C0C AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 70000C0E AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 70000C10 AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 70000C12 AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 70000C14 AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 70000C16 AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 70000C18 AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120002, /* 70000C1A AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 70000C1C AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 70000C1E AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 70000C20 AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000C22 AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000C24 AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000C26 AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120206, /* 70000C28 AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000C2A AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000C2C AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120305, /* 70000C2E AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120406, /* 70000C30 AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F122804, /* 70000C32 AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F120228, /* 70000C34 AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121402, /* 70000C36 AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120618, /* 70000C38 AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000C3A AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F126E18, /* 70000C3C AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F120080, /* 70000C3E AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120080, /* 70000C40 AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000C42 AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000C44 AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000C46 AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F12232B, /* 70000C48 AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000C4A AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F121414, /* 70000C4C AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000C4E AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000C50 AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000C52 AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000C54 AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F121501, /* 70000C56 AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F12240F, /* 70000C58 AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F120A60, /* 70000C5A AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF0A, /* 70000C5C AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000C5E AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000C60 AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000C62 AFITB_bnr_nClustLevel_C [0] */
	0x0F120000, /* 70000C64 AFIT16_BRIGHTNESS      //AFIT 4 */
	0x0F120000, /* 70000C66 AFIT16_CONTRAST */
	0x0F120000, /* 70000C68 AFIT16_SATURATION */
	0x0F120000, /* 70000C6A AFIT16_SHARP_BLUR */
	0x0F120000, /* 70000C6C AFIT16_GLAMOUR */
	0x0F1200C0, /* 70000C6E AFIT16_bnr_edge_high */
	0x0F120064, /* 70000C70 AFIT16_postdmsc_iLowBright */
	0x0F120384, /* 70000C72 AFIT16_postdmsc_iHighBright */
	0x0F120032, /* 70000C74 AFIT16_postdmsc_iLowSat */
	0x0F1201F4, /* 70000C76 AFIT16_postdmsc_iHighSat */
	0x0F120070, /* 70000C78 AFIT16_postdmsc_iTune */
	0x0F120040, /* 70000C7A AFIT16_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000C7C AFIT16_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000C7E AFIT16_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000C80 AFIT16_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000C82 AFIT16_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000C84 AFIT16_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000C86 AFIT8_bnr_edge_low
			[7:0] AFIT8_bnr_repl_thresh */
	0x0F120201, /* 70000C88 AFIT8_bnr_repl_force
			[7:0] AFIT8_bnr_iHotThreshHigh */
	0x0F120204, /* 70000C8A AFIT8_bnr_iHotThreshLow
			[7:0] AFIT8_bnr_iColdThreshHigh */
	0x0F120F04, /* 70000C8C AFIT8_bnr_iColdThreshLow
			[7:0] AFIT8_bnr_DispTH_Low */
	0x0F12030C, /* 70000C8E AFIT8_bnr_DispTH_High
			[7:0] AFIT8_bnr_DISP_Limit_Low */
	0x0F120003, /* 70000C90 AFIT8_bnr_DISP_Limit_High
			[7:0] AFIT8_bnr_iDistSigmaMin */
	0x0F120602, /* 70000C92 AFIT8_bnr_iDistSigmaMax
			[7:0] AFIT8_bnr_iDiffSigmaLow */
	0x0F121803, /* 70000C94 AFIT8_bnr_iDiffSigmaHigh
			[7:0] AFIT8_bnr_iNormalizedSTD_TH */
	0x0F120040, /* 70000C96 AFIT8_bnr_iNormalizedSTD_Limit
			[7:0] AFIT8_bnr_iDirNRTune */
	0x0F120E20, /* 70000C98 AFIT8_bnr_iDirMinThres
			[7:0] AFIT8_bnr_iDirFltDiffThresHigh */
	0x0F122018, /* 70000C9A AFIT8_bnr_iDirFltDiffThresLow
			[7:0] AFIT8_bnr_iDirSmoothPowerHigh */
	0x0F120620, /* 70000C9C AFIT8_bnr_iDirSmoothPowerLow
			[7:0] AFIT8_bnr_iLowMaxSlopeAllowed */
	0x0F120306, /* 70000C9E AFIT8_bnr_iHighMaxSlopeAllowed
			[7:0] AFIT8_bnr_iLowSlopeThresh */
	0x0F122003, /* 70000CA0 AFIT8_bnr_iHighSlopeThresh
			[7:0] AFIT8_bnr_iSlopenessTH */
	0x0F12FF01, /* 70000CA2 AFIT8_bnr_iSlopeBlurStrength
			[7:0] AFIT8_bnr_iSlopenessLimit */
	0x0F120000, /* 70000CA4 AFIT8_bnr_AddNoisePower1
			[7:0] AFIT8_bnr_AddNoisePower2 */
	0x0F120200, /* 70000CA6 AFIT8_bnr_iRadialTune
			[7:0] AFIT8_bnr_iRadialPower */
	0x0F12145A, /* 70000CA8 AFIT8_bnr_iRadialLimit
			[7:0] AFIT8_ee_iFSMagThLow */
	0x0F121010, /* 70000CAA AFIT8_ee_iFSMagThHigh
			[7:0] AFIT8_ee_iFSVarThLow */
	0x0F12000B, /* 70000CAC AFIT8_ee_iFSVarThHigh
			[7:0] AFIT8_ee_iFSThLow */
	0x0F121200, /* 70000CAE AFIT8_ee_iFSThHigh
			[7:0] AFIT8_ee_iFSmagPower */
	0x0F125A0F, /* 70000CB0 AFIT8_ee_iFSVarCountTh
			[7:0] AFIT8_ee_iRadialLimit */
	0x0F120502, /* 70000CB2 AFIT8_ee_iRadialPower
			[7:0] AFIT8_ee_iSmoothEdgeSlope */
	0x0F121802, /* 70000CB4 AFIT8_ee_iROADThres
			[7:0] AFIT8_ee_iROADMaxNR */
	0x0F120000, /* 70000CB6 AFIT8_ee_iROADSubMaxNR
			[7:0] AFIT8_ee_iROADSubThres */
	0x0F122006, /* 70000CB8 AFIT8_ee_iROADNeiThres
			[7:0] AFIT8_ee_iROADNeiMaxNR */
	0x0F124028, /* 70000CBA AFIT8_ee_iSmoothEdgeThres
			[7:0] AFIT8_ee_iMSharpen */
	0x0F120430, /* 70000CBC AFIT8_ee_iWSharpen
			[7:0] AFIT8_ee_iMShThresh */
	0x0F120101, /* 70000CBE AFIT8_ee_iWShThresh
			[7:0] AFIT8_ee_iReduceNegative */
	0x0F12FF00, /* 70000CC0 AFIT8_ee_iEmbossCentAdd
			[7:0] AFIT8_ee_iShDespeckle */
	0x0F120804, /* 70000CC2 AFIT8_ee_iReduceEdgeThresh
			[7:0] AFIT8_dmsc_iEnhThresh */
	0x0F124008, /* 70000CC4 AFIT8_dmsc_iDesatThresh
			[7:0] AFIT8_dmsc_iDemBlurHigh */
	0x0F120540, /* 70000CC6 AFIT8_dmsc_iDemBlurLow
			[7:0] AFIT8_dmsc_iDemBlurRange */
	0x0F128006, /* 70000CC8 AFIT8_dmsc_iDecisionThresh
			[7:0] AFIT8_dmsc_iCentGrad */
	0x0F120020, /* 70000CCA AFIT8_dmsc_iMonochrom
			[7:0] AFIT8_dmsc_iGBDenoiseVal */
	0x0F120000, /* 70000CCC AFIT8_dmsc_iGRDenoiseVal
			[7:0] AFIT8_dmsc_iEdgeDesatThrHigh */
	0x0F121800, /* 70000CCE AFIT8_dmsc_iEdgeDesatThrLow
			[7:0] AFIT8_dmsc_iEdgeDesat */
	0x0F120002, /* 70000CD0 AFIT8_dmsc_iNearGrayDesat
			[7:0] AFIT8_dmsc_iEdgeDesatLimit */
	0x0F121E10, /* 70000CD2 AFIT8_postdmsc_iBCoeff
			[7:0] AFIT8_postdmsc_iGCoeff */
	0x0F12000B, /* 70000CD4 AFIT8_postdmsc_iWideMult
			[7:0] AFIT8_yuvemix_mNegSlopes_0 */
	0x0F120607, /* 70000CD6 AFIT8_yuvemix_mNegSlopes_1
			[7:0] AFIT8_yuvemix_mNegSlopes_2 */
	0x0F120005, /* 70000CD8 AFIT8_yuvemix_mNegSlopes_3
			[7:0] AFIT8_yuvemix_mPosSlopes_0 */
	0x0F120607, /* 70000CDA AFIT8_yuvemix_mPosSlopes_1
			[7:0] AFIT8_yuvemix_mPosSlopes_2 */
	0x0F120405, /* 70000CDC AFIT8_yuvemix_mPosSlopes_3
			[7:0] AFIT8_yuviirnr_iXSupportY */
	0x0F120205, /* 70000CDE AFIT8_yuviirnr_iXSupportUV
			[7:0] AFIT8_yuviirnr_iLowYNorm */
	0x0F120304, /* 70000CE0 AFIT8_yuviirnr_iHighYNorm
			[7:0] AFIT8_yuviirnr_iLowUVNorm */
	0x0F120409, /* 70000CE2 AFIT8_yuviirnr_iHighUVNorm
			[7:0] AFIT8_yuviirnr_iYNormShift */
	0x0F120306, /* 70000CE4 AFIT8_yuviirnr_iUVNormShift
			[7:0] AFIT8_yuviirnr_iVertLength_Y */
	0x0F120407, /* 70000CE6 AFIT8_yuviirnr_iVertLength_UV
			[7:0] AFIT8_yuviirnr_iDiffThreshL_Y */
	0x0F122C04, /* 70000CE8 AFIT8_yuviirnr_iDiffThreshH_Y
			[7:0] AFIT8_yuviirnr_iDiffThreshL_UV */
	0x0F12022C, /* 70000CEA AFIT8_yuviirnr_iDiffThreshH_UV
			[7:0] AFIT8_yuviirnr_iMaxThreshL_Y */
	0x0F121402, /* 70000CEC AFIT8_yuviirnr_iMaxThreshH_Y
			[7:0] AFIT8_yuviirnr_iMaxThreshL_UV */
	0x0F120618, /* 70000CEE AFIT8_yuviirnr_iMaxThreshH_UV
			[7:0] AFIT8_yuviirnr_iYNRStrengthL */
	0x0F121A02, /* 70000CF0 AFIT8_yuviirnr_iYNRStrengthH
			[7:0] AFIT8_yuviirnr_iUVNRStrengthL */
	0x0F126E18, /* 70000CF2 AFIT8_yuviirnr_iUVNRStrengthH
			[7:0] AFIT8_byr_gras_iShadingPower */
	0x0F120080, /* 70000CF4 AFIT8_RGBGamma2_iLinearity
			[7:0] AFIT8_RGBGamma2_iDarkReduce */
	0x0F120080, /* 70000CF6 AFIT8_ccm_oscar_iSaturation
			[7:0] AFIT8_RGB2YUV_iYOffset */
	0x0F120180, /* 70000CF8 AFIT8_RGB2YUV_iRGBGain
			[7:0] AFIT8_bnr_nClustLevel_H */
	0x0F120A0A, /* 70000CFA AFIT8_bnr_iClustMulT_H
			[7:0] AFIT8_bnr_iClustMulT_C */
	0x0F120101, /* 70000CFC AFIT8_bnr_iClustThresh_H
			[7:0] AFIT8_bnr_iClustThresh_C */
	0x0F121114, /* 70000CFE AFIT8_bnr_iDenThreshLow
			[7:0] AFIT8_bnr_iDenThreshHigh */
	0x0F126024, /* 70000D00 AFIT8_ee_iLowSharpPower
			[7:0] AFIT8_ee_iHighSharpPower */
	0x0F121212, /* 70000D02 AFIT8_ee_iLowShDenoise
			[7:0] AFIT8_ee_iHighShDenoise */
	0x0F12FFFF, /* 70000D04 AFIT8_ee_iLowSharpClamp
			[7:0] AFIT8_ee_iHighSharpClamp */
	0x0F120808, /* 70000D06 AFIT8_ee_iReduceEdgeMinMult
			[7:0] AFIT8_ee_iReduceEdgeSlope */
	0x0F120A01, /* 70000D08 AFIT8_bnr_nClustLevel_H_Bin
			[7:0] AFIT8_bnr_iClustMulT_H_Bin */
	0x0F12010A, /* 70000D0A AFIT8_bnr_iClustMulT_C_Bin
			[7:0] AFIT8_bnr_iClustThresh_H_Bin */
	0x0F120F01, /* 70000D0C AFIT8_bnr_iClustThresh_C_Bin
			[7:0] AFIT8_bnr_iDenThreshLow_Bin */
	0x0F12240C, /* 70000D0E AFIT8_bnr_iDenThreshHigh_Bin
			[7:0] AFIT8_ee_iLowSharpPower_Bin */
	0x0F120860, /* 70000D10 AFIT8_ee_iHighSharpPower_Bin
			[7:0] AFIT8_ee_iLowShDenoise_Bin */
	0x0F12FF08, /* 70000D12 AFIT8_ee_iHighShDenoise_Bin
			[7:0] AFIT8_ee_iLowSharpClamp_Bin */
	0x0F1208FF, /* 70000D14 AFIT8_ee_iHighSharpClamp_Bin
			[7:0] AFIT8_ee_iReduceEdgeMinMult_Bin */
	0x0F120008, /* 70000D16 AFIT8_ee_iReduceEdgeSlope_Bin [7:0] */
	0x0F120001, /* 70000D18 AFITB_bnr_nClustLevel_C [0]   bWideWide[1] */

	0x002A060C,
	0x0F120800, /* lt_ExpGain_ExpCurveGainMaxStr */
	/* FOR HD CAMCORDER */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A02A6,
	0x0F120280, /* REG_0TC_PCFG_usWidth */
	0x0F1201E0, /* REG_0TC_PCFG_usHeight */
	0x002A02AB,
	0x0F120005, /* REG_0TC_PCFG_Format
			05 : yuv (0~255)  06:yuv (16~234) 07: raw 09 : jpeg */
	0x002A0266,
	0x0F120000, /* #REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* #REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* #REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* #REG_TC_GP_PrevConfigChanged */
	0x002A0270,
	0x0F120001, /* #REG_TC_GP_CapConfigChanged */
	0x002A023E,
	0x0F120001, /* #REG_TC_GP_EnablePreview */
	0x0F120001, /* #REG_TC_GP_EnablePreviewChanged */
};

static const u32 s5k4ec_preview_disable_1280_720[] = {
	0xFCFCD000,
	0x0028D000,
	0x002AE410,
	0x0F123804, /* [15:8]fadlc_filter_co_b, [7:0]fadlc_filter_co_a */
	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin        */
	0x0F120060, /* senHal_uAddColsNoBin      */
	0x0F120A20, /* senHal_uMinColsBin        */
	0x0F120AB0, /* senHal_uMinColsNoBin      */

	/* SLOW AE*/
	0x002A1568,
	0x0F120010, /* ae_GainIn_0_ */
	0x0F120020, /* ae_GainIn_1_ */
	0x0F120040, /* ae_GainIn_2_ */
	0x0F120080, /* ae_GainIn_3_ */
	0x0F120100, /* ae_GainIn_4_ FIX */
	0x0F120200, /* ae_GainIn_5_ */
	0x0F120400, /* ae_GainIn_6_ */
	0x0F120800, /* ae_GainIn_7_ */
	0x0F122000, /* ae_GainIn_8_ */
	0x0F120010, /* ae_GainOut_0_ */
	0x0F120020, /* ae_GainOut_1_ */
	0x0F120040, /* ae_GainOut_2_ */
	0x0F120080, /* ae_GainOut_3_ */
	0x0F120100, /* ae_GainOut_4_ FIX */
	0x0F120200, /* ae_GainOut_5_ */
	0x0F120400, /* ae_GainOut_6_ */
	0x0F120800, /* ae_GainOut_7_ */
	0x0F122000, /* ae_GainOut_8_ */

	0x002A0544,
	0x0F120111, /* lt_uLimitHigh */
	0x0F1200EF, /* lt_uLimitLow */

	0x002A0588,
	0x0F120002, /* lt_uInitPostToleranceCnt */

	0x002A0582,
	0x0F120000, /* lt_uSlowFilterCoef */

	0x002A47B0,
	0x0F120000, /* TNP_Regs_BUse1FrameAE	(0: off, 1: on) */

	/* SLOW AWB */
	0x002A139A,
	0x0F120258, /* 0258 awbb_GainsMaxMove */

	/* AWB Convergence Speed */
	0x002A1464,
	0x0F120008,
	0x0F120190,
	0x0F1200A0,
	0x0F120004,

	/* SHARPNESS n NOISE */
	0x002A0938,
	0x0F120000, /* on/off AFIT by NB option */
	0x0F120014, /* SARR_uNormBrInDoor */
	0x0F1200D2, /* SARR_uNormBrInDoor */
	0x0F120384, /* SARR_uNormBrInDoor */
	0x0F1207D0, /* SARR_uNormBrInDoor */
	0x0F121388, /* SARR_uNormBrInDoor */

	0x002A098C,
	0x0F12FFFB, /* 7000098C_BRIGHTNESS   AFIT 0 */
	0x0F120000, /* 7000098E_CONTRAST */
	0x0F120000, /* 70000990_SATURATION */
	0x0F120000, /* 70000992_SHARP_BLUR */
	0x0F120000, /* 70000994_GLAMOUR */
	0x0F1200C0, /* 70000996_bnr_edge_high */
	0x0F120064, /* 70000998_postdmsc_iLowBright */
	0x0F120384, /* 7000099A_postdmsc_iHighBright */
	0x0F12005F, /* 7000099C_postdmsc_iLowSat */
	0x0F1201F4, /* 7000099E_postdmsc_iHighSat */
	0x0F120070, /* 700009A0_postdmsc_iTune */
	0x0F120040, /* 700009A2_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 700009A4_yuvemix_mNegRanges_1 */
	0x0F120100, /* 700009A6_yuvemix_mNegRanges_2 */
	0x0F120010, /* 700009A8_yuvemix_mPosRanges_0 */
	0x0F120040, /* 700009AA_yuvemix_mPosRanges_1 */
	0x0F1200A0, /* 700009AC_yuvemix_mPosRanges_2 */
	0x0F121430, /* 700009AE_bnr_edge_low  */
	0x0F120201, /* 700009B0_bnr_repl_force  */
	0x0F120204, /* 700009B2_bnr_iHotThreshLow   */
	0x0F123604, /* 700009B4_bnr_iColdThreshLow   */
	0x0F12032A, /* 700009B6_bnr_DispTH_High   */
	0x0F120103, /* 700009B8_bnr_DISP_Limit_High   */
	0x0F121205, /* 700009BA_bnr_iDistSigmaMax   */
	0x0F12400D, /* 700009BC_bnr_iDiffSigmaHigh   */
	0x0F120080, /* 700009BE_bnr_iNormalizedSTD_Limit   */
	0x0F122080, /* 700009C0_bnr_iDirMinThres   */
	0x0F123840, /* 700009C2_bnr_iDirFltDiffThresLow  */
	0x0F120638, /* 700009C4_bnr_iDirSmoothPowerLow   */
	0x0F120306, /* 700009C6_bnr_iHighMaxSlopeAllowed   */
	0x0F122003, /* 700009C8_bnr_iHighSlopeThresh   */
	0x0F12FF01, /* 700009CA_bnr_iSlopeBlurStrength   */
	0x0F120000, /* 700009CC_bnr_AddNoisePower1   */
	0x0F120400, /* 700009CE_bnr_iRadialTune   */
	0x0F12245A, /* 700009D0_bnr_iRadialLimit   */
	0x0F12102A, /* 700009D2_ee_iFSMagThHigh   */
	0x0F12000B, /* 700009D4_ee_iFSVarThHigh   */
	0x0F120600, /* 700009D6_ee_iFSThHigh   */
	0x0F125A0F, /* 700009D8_ee_iFSVarCountTh   */
	0x0F120505, /* 700009DA_ee_iRadialPower   */
	0x0F121802, /* 700009DC_ee_iROADThres   */
	0x0F120000, /* 700009DE_ee_iROADSubMaxNR   */
	0x0F122006, /* 700009E0_ee_iROADNeiThres   */
	0x0F122828, /* 700009E2_ee_iSmoothEdgeThres   */
	0x0F12040F, /* 700009E4_ee_iWSharpen   */
	0x0F120101, /* 700009E6_ee_iWShThresh   */
	0x0F120800, /* 700009E8_ee_iEmbossCentAdd   */
	0x0F121804, /* 700009EA_ee_iReduceEdgeThresh   */
	0x0F124008, /* 700009EC_dmsc_iDesatThresh   */
	0x0F120540, /* 700009EE_dmsc_iDemBlurLow   */
	0x0F128006, /* 700009F0_dmsc_iDecisionThresh   */
	0x0F120020, /* 700009F2_dmsc_iMonochrom   */
	0x0F120000, /* 700009F4_dmsc_iGRDenoiseVal   */
	0x0F121800, /* 700009F6_dmsc_iEdgeDesatThrLow   */
	0x0F120004, /* 700009F8_dmsc_iNearGrayDesat   */
	0x0F121E10, /* 700009FA_postdmsc_iBCoeff   */
	0x0F12000B, /* 700009FC_postdmsc_iWideMult   */
	0x0F120607, /* 700009FE_yuvemix_mNegSlopes_1   */
	0x0F120005, /* 70000A00_yuvemix_mNegSlopes_3   */
	0x0F120607, /* 70000A02_yuvemix_mPosSlopes_1   */
	0x0F120405, /* 70000A04_yuvemix_mPosSlopes_3   */
	0x0F120205, /* 70000A06_yuviirnr_iXSupportUV   */
	0x0F120304, /* 70000A08_yuviirnr_iHighYNorm   */
	0x0F120409, /* 70000A0A_yuviirnr_iHighUVNorm   */
	0x0F120306, /* 70000A0C_yuviirnr_iUVNormShift   */
	0x0F120407, /* 70000A0E_yuviirnr_iVertLength_UV   */
	0x0F121C04, /* 70000A10_yuviirnr_iDiffThreshH_Y   */
	0x0F120214, /* 70000A12_yuviirnr_iDiffThreshH_UV   */
	0x0F121002, /* 70000A14_yuviirnr_iMaxThreshH_Y   */
	0x0F120610, /* 70000A16_yuviirnr_iMaxThreshH_UV   */
	0x0F121A02, /* 70000A18_yuviirnr_iYNRStrengthH   */
	0x0F122818, /* 70000A1A_yuviirnr_iUVNRStrengthH   */
	0x0F120080, /* 70000A1C_RGBGamma2_iLinearity   */
	0x0F120350, /* 70000A1E_ccm_oscar_iSaturation   */
	0x0F120180, /* 70000A20_RGB2YUV_iRGBGain   */
	0x0F120A0A, /* 70000A22_bnr_iClustMulT_H   */
	0x0F120101, /* 70000A24_bnr_iClustThresh_H   */
	0x0F123141, /* 70000A26_bnr_iDenThreshLow   */
	0x0F126024, /* 70000A28_ee_iLowSharpPower   */
	0x0F123140, /* 70000A2A_ee_iLowShDenoise   */
	0x0F12FFFF, /* 70000A2C_ee_iLowSharpClamp   */
	0x0F120808, /* 70000A2E_ee_iReduceEdgeMinMult   */
	0x0F120A01, /* 70000A30_bnr_nClustLevel_H_Bin   */
	0x0F12010A, /* 70000A32_bnr_iClustMulT_C_Bin   */
	0x0F123601, /* 70000A34_bnr_iClustThresh_C_Bin   */
	0x0F12242A, /* 70000A36_bnr_iDenThreshHigh_Bin   */
	0x0F123660, /* 70000A38_ee_iHighSharpPower_Bin   */
	0x0F12FF2A, /* 70000A3A_ee_iHighShDenoise_Bin   */
	0x0F1208FF, /* 70000A3C_ee_iHighSharpClamp_Bin   */
	0x0F120008, /* 70000A3E_ee_iReduceEdgeSlope_Bin */
	0x0F120001, /* 70000A40_bnr_nClustLevel_C      */
	0x0F120000, /* 70000A42_BRIGHTNESS   AFIT 1 */
	0x0F120000, /* 70000A44_CONTRAST */
	0x0F12FFFB, /* 70000A46_SATURATION */
	0x0F120000, /* 70000A48_SHARP_BLUR */
	0x0F120000, /* 70000A4A_GLAMOUR */
	0x0F1200C0, /* 70000A4C_bnr_edge_high */
	0x0F120064, /* 70000A4E_postdmsc_iLowBright */
	0x0F120384, /* 70000A50_postdmsc_iHighBright */
	0x0F120051, /* 70000A52_postdmsc_iLowSat */
	0x0F1201F4, /* 70000A54_postdmsc_iHighSat */
	0x0F120070, /* 70000A56_postdmsc_iTune */
	0x0F120040, /* 70000A58_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000A5A_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000A5C_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000A5E_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000A60_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000A62_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000A64_bnr_edge_low  */
	0x0F120201, /* 70000A66_bnr_repl_force  */
	0x0F120204, /* 70000A68_bnr_iHotThreshLow   */
	0x0F122404, /* 70000A6A_bnr_iColdThreshLow   */
	0x0F12031B, /* 70000A6C_bnr_DispTH_High   */
	0x0F120103, /* 70000A6E_bnr_DISP_Limit_High   */
	0x0F121004, /* 70000A70_bnr_iDistSigmaMax   */
	0x0F123A0C, /* 70000A72_bnr_iDiffSigmaHigh   */
	0x0F120070, /* 70000A74_bnr_iNormalizedSTD_Limit   */
	0x0F121C80, /* 70000A76_bnr_iDirMinThres   */
	0x0F123030, /* 70000A78_bnr_iDirFltDiffThresLow   */
	0x0F120630, /* 70000A7A_bnr_iDirSmoothPowerLow   */
	0x0F120306, /* 70000A7C_bnr_iHighMaxSlopeAllowed   */
	0x0F122003, /* 70000A7E_bnr_iHighSlopeThresh   */
	0x0F12FF01, /* 70000A80_bnr_iSlopeBlurStrength   */
	0x0F120000, /* 70000A82_bnr_AddNoisePower1   */
	0x0F120300, /* 70000A84_bnr_iRadialTune   */
	0x0F12245A, /* 70000A86_bnr_iRadialLimit   */
	0x0F121018, /* 70000A88_ee_iFSMagThHigh   */
	0x0F12000B, /* 70000A8A_ee_iFSVarThHigh   */
	0x0F120B00, /* 70000A8C_ee_iFSThHigh   */
	0x0F125A0F, /* 70000A8E_ee_iFSVarCountTh   */
	0x0F120505, /* 70000A90_ee_iRadialPower   */
	0x0F121802, /* 70000A92_ee_iROADThres   */
	0x0F120000, /* 70000A94_ee_iROADSubMaxNR   */
	0x0F122006, /* 70000A96_ee_iROADNeiThres   */
	0x0F122928, /* 70000A98_ee_iSmoothEdgeThres   */
	0x0F120415, /* 70000A9A_ee_iWSharpen   */
	0x0F120101, /* 70000A9C_ee_iWShThresh   */
	0x0F120800, /* 70000A9E_ee_iEmbossCentAdd   */
	0x0F121004, /* 70000AA0_ee_iReduceEdgeThresh   */
	0x0F124008, /* 70000AA2_dmsc_iDesatThresh   */
	0x0F120540, /* 70000AA4_dmsc_iDemBlurLow   */
	0x0F128006, /* 70000AA6_dmsc_iDecisionThresh   */
	0x0F120020, /* 70000AA8_dmsc_iMonochrom   */
	0x0F120000, /* 70000AAA_dmsc_iGRDenoiseVal   */
	0x0F121800, /* 70000AAC_dmsc_iEdgeDesatThrLow   */
	0x0F120003, /* 70000AAE_dmsc_iNearGrayDesat   */
	0x0F121E10, /* 70000AB0_postdmsc_iBCoeff   */
	0x0F12000B, /* 70000AB2_postdmsc_iWideMult   */
	0x0F120607, /* 70000AB4_yuvemix_mNegSlopes_1   */
	0x0F120005, /* 70000AB6_yuvemix_mNegSlopes_3   */
	0x0F120607, /* 70000AB8_yuvemix_mPosSlopes_1   */
	0x0F120405, /* 70000ABA_yuvemix_mPosSlopes_3   */
	0x0F120205, /* 70000ABC_yuviirnr_iXSupportUV   */
	0x0F120304, /* 70000ABE_yuviirnr_iHighYNorm   */
	0x0F120409, /* 70000AC0_yuviirnr_iHighUVNorm   */
	0x0F120306, /* 70000AC2_yuviirnr_iUVNormShift   */
	0x0F120407, /* 70000AC4_yuviirnr_iVertLength_UV   */
	0x0F121F04, /* 70000AC6_yuviirnr_iDiffThreshH_Y   */
	0x0F120218, /* 70000AC8_yuviirnr_iDiffThreshH_UV   */
	0x0F121102, /* 70000ACA_yuviirnr_iMaxThreshH_Y   */
	0x0F120611, /* 70000ACC_yuviirnr_iMaxThreshH_UV   */
	0x0F121A02, /* 70000ACE_yuviirnr_iYNRStrengthH   */
	0x0F126418, /* 70000AD0_yuviirnr_iUVNRStrengthH   */
	0x0F120080, /* 70000AD2_RGBGamma2_iLinearity   */
	0x0F120380, /* 70000AD4_ccm_oscar_iSaturation   */
	0x0F120180, /* 70000AD6_RGB2YUV_iRGBGain   */
	0x0F120A0A, /* 70000AD8_bnr_iClustMulT_H   */
	0x0F120101, /* 70000ADA_bnr_iClustThresh_H   */
	0x0F122832, /* 70000ADC_bnr_iDenThreshLow   */
	0x0F126024, /* 70000ADE_ee_iLowSharpPower   */
	0x0F12272C, /* 70000AE0_ee_iLowShDenoise   */
	0x0F12FFFF, /* 70000AE2_ee_iLowSharpClamp   */
	0x0F120808, /* 70000AE4_ee_iReduceEdgeMinMult   */
	0x0F120A01, /* 70000AE6_bnr_nClustLevel_H_Bin   */
	0x0F12010A, /* 70000AE8_bnr_iClustMulT_C_Bin   */
	0x0F122401, /* 70000AEA_bnr_iClustThresh_C_Bin   */
	0x0F12241B, /* 70000AEC_bnr_iDenThreshHigh_Bin   */
	0x0F121E60, /* 70000AEE_ee_iHighSharpPower_Bin   */
	0x0F12FF18, /* 70000AF0_ee_iHighShDenoise_Bin   */
	0x0F1208FF, /* 70000AF2_ee_iHighSharpClamp_Bin   */
	0x0F120008, /* 70000AF4_ee_iReduceEdgeSlope_Bin */
	0x0F120001, /* 70000AF6_bnr_nClustLevel_C      */
	0x0F120000, /* 70000AF8_BRIGHTNESS   AFIT 2 */
	0x0F120000, /* 70000AFA_CONTRAST */
	0x0F12FFFB, /* 70000AFC_SATURATION */
	0x0F120000, /* 70000AFE_SHARP_BLUR */
	0x0F120000, /* 70000B00_GLAMOUR */
	0x0F1200C0, /* 70000B02_bnr_edge_high */
	0x0F120064, /* 70000B04_postdmsc_iLowBright */
	0x0F120384, /* 70000B06_postdmsc_iHighBright */
	0x0F120043, /* 70000B08_postdmsc_iLowSat */
	0x0F1201F4, /* 70000B0A_postdmsc_iHighSat */
	0x0F120070, /* 70000B0C_postdmsc_iTune */
	0x0F120040, /* 70000B0E_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000B10_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000B12_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000B14_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000B16_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000B18_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000B1A_bnr_edge_low  */
	0x0F120201, /* 70000B1C_bnr_repl_force  */
	0x0F120204, /* 70000B1E_bnr_iHotThreshLow   */
	0x0F121B04, /* 70000B20_bnr_iColdThreshLow   */
	0x0F120312, /* 70000B22_bnr_DispTH_High   */
	0x0F120003, /* 70000B24_bnr_DISP_Limit_High   */
	0x0F120C03, /* 70000B26_bnr_iDistSigmaMax   */
	0x0F122806, /* 70000B28_bnr_iDiffSigmaHigh   */
	0x0F120060, /* 70000B2A_bnr_iNormalizedSTD_Limit   */
	0x0F121580, /* 70000B2C_bnr_iDirMinThres   */
	0x0F122020, /* 70000B2E_bnr_iDirFltDiffThresLow   */
	0x0F120620, /* 70000B30_bnr_iDirSmoothPowerLow   */
	0x0F120306, /* 70000B32_bnr_iHighMaxSlopeAllowed   */
	0x0F122003, /* 70000B34_bnr_iHighSlopeThresh   */
	0x0F12FF01, /* 70000B36_bnr_iSlopeBlurStrength   */
	0x0F120000, /* 70000B38_bnr_AddNoisePower1   */
	0x0F120300, /* 70000B3A_bnr_iRadialTune   */
	0x0F12145A, /* 70000B3C_bnr_iRadialLimit   */
	0x0F121010, /* 70000B3E_ee_iFSMagThHigh   */
	0x0F12000B, /* 70000B40_ee_iFSVarThHigh   */
	0x0F120E00, /* 70000B42_ee_iFSThHigh   */
	0x0F125A0F, /* 70000B44_ee_iFSVarCountTh   */
	0x0F120504, /* 70000B46_ee_iRadialPower   */
	0x0F121802, /* 70000B48_ee_iROADThres   */
	0x0F120000, /* 70000B4A_ee_iROADSubMaxNR   */
	0x0F122006, /* 70000B4C_ee_iROADNeiThres   */
	0x0F122B28, /* 70000B4E_ee_iSmoothEdgeThres   */
	0x0F120417, /* 70000B50_ee_iWSharpen   */
	0x0F120101, /* 70000B52_ee_iWShThresh   */
	0x0F128000, /* 70000B54_ee_iEmbossCentAdd   */
	0x0F120A04, /* 70000B56_ee_iReduceEdgeThresh   */
	0x0F124008, /* 70000B58_dmsc_iDesatThresh   */
	0x0F120540, /* 70000B5A_dmsc_iDemBlurLow   */
	0x0F128006, /* 70000B5C_dmsc_iDecisionThresh   */
	0x0F120020, /* 70000B5E_dmsc_iMonochrom   */
	0x0F120000, /* 70000B60_dmsc_iGRDenoiseVal   */
	0x0F121800, /* 70000B62_dmsc_iEdgeDesatThrLow   */
	0x0F120002, /* 70000B64_dmsc_iNearGrayDesat   */
	0x0F121E10, /* 70000B66_postdmsc_iBCoeff   */
	0x0F12000B, /* 70000B68_postdmsc_iWideMult   */
	0x0F120607, /* 70000B6A_yuvemix_mNegSlopes_1   */
	0x0F120005, /* 70000B6C_yuvemix_mNegSlopes_3   */
	0x0F120607, /* 70000B6E_yuvemix_mPosSlopes_1   */
	0x0F120405, /* 70000B70_yuvemix_mPosSlopes_3   */
	0x0F120207, /* 70000B72_yuviirnr_iXSupportUV   */
	0x0F120304, /* 70000B74_yuviirnr_iHighYNorm   */
	0x0F120409, /* 70000B76_yuviirnr_iHighUVNorm   */
	0x0F120306, /* 70000B78_yuviirnr_iUVNormShift   */
	0x0F120407, /* 70000B7A_yuviirnr_iVertLength_UV   */
	0x0F122404, /* 70000B7C_yuviirnr_iDiffThreshH_Y   */
	0x0F120221, /* 70000B7E_yuviirnr_iDiffThreshH_UV   */
	0x0F121202, /* 70000B80_yuviirnr_iMaxThreshH_Y   */
	0x0F120613, /* 70000B82_yuviirnr_iMaxThreshH_UV   */
	0x0F121A02, /* 70000B84_yuviirnr_iYNRStrengthH   */
	0x0F126E18, /* 70000B86_yuviirnr_iUVNRStrengthH   */
	0x0F120080, /* 70000B88_RGBGamma2_iLinearity   */
	0x0F120080, /* 70000B8A_ccm_oscar_iSaturation   */
	0x0F120180, /* 70000B8C_RGB2YUV_iRGBGain   */
	0x0F120A0A, /* 70000B8E_bnr_iClustMulT_H   */
	0x0F120101, /* 70000B90_bnr_iClustThresh_H   */
	0x0F122630, /* 70000B92_bnr_iDenThreshLow   */
	0x0F126024, /* 70000B94_ee_iLowSharpPower   */
	0x0F121616, /* 70000B96_ee_iLowShDenoise   */
	0x0F12FFFF, /* 70000B98_ee_iLowSharpClamp   */
	0x0F120808, /* 70000B9A_ee_iReduceEdgeMinMult   */
	0x0F120A01, /* 70000B9C_bnr_nClustLevel_H_Bin   */
	0x0F12010A, /* 70000B9E_bnr_iClustMulT_C_Bin   */
	0x0F121B01, /* 70000BA0_bnr_iClustThresh_C_Bin   */
	0x0F122412, /* 70000BA2_bnr_iDenThreshHigh_Bin   */
	0x0F120C60, /* 70000BA4_ee_iHighSharpPower_Bin   */
	0x0F12FF0C, /* 70000BA6_ee_iHighShDenoise_Bin   */
	0x0F1208FF, /* 70000BA8_ee_iHighSharpClamp_Bin   */
	0x0F120008, /* 70000BAA_ee_iReduceEdgeSlope_Bin */
	0x0F120001, /* 70000BAC_bnr_nClustLevel_C      */
	0x0F120000, /* 70000BAE_BRIGHTNESS   AFIT 3 */
	0x0F120000, /* 70000BB0_CONTRAST */
	0x0F120000, /* 70000BB2_SATURATION */
	0x0F120000, /* 70000BB4_SHARP_BLUR */
	0x0F120000, /* 70000BB6_GLAMOUR */
	0x0F1200C0, /* 70000BB8_bnr_edge_high */
	0x0F120064, /* 70000BBA_postdmsc_iLowBright */
	0x0F120384, /* 70000BBC_postdmsc_iHighBright */
	0x0F120032, /* 70000BBE_postdmsc_iLowSat */
	0x0F1201F4, /* 70000BC0_postdmsc_iHighSat */
	0x0F120070, /* 70000BC2_postdmsc_iTune */
	0x0F120040, /* 70000BC4_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000BC6_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000BC8_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000BCA_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000BCC_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000BCE_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000BD0_bnr_edge_low  */
	0x0F120201, /* 70000BD2_bnr_repl_force  */
	0x0F120204, /* 70000BD4_bnr_iHotThreshLow   */
	0x0F121504, /* 70000BD6_bnr_iColdThreshLow   */
	0x0F12030F, /* 70000BD8_bnr_DispTH_High   */
	0x0F120003, /* 70000BDA_bnr_DISP_Limit_High   */
	0x0F120902, /* 70000BDC_bnr_iDistSigmaMax   */
	0x0F122004, /* 70000BDE_bnr_iDiffSigmaHigh   */
	0x0F120050, /* 70000BE0_bnr_iNormalizedSTD_Limit   */
	0x0F121140, /* 70000BE2_bnr_iDirMinThres   */
	0x0F12201C, /* 70000BE4_bnr_iDirFltDiffThresLow   */
	0x0F120620, /* 70000BE6_bnr_iDirSmoothPowerLow   */
	0x0F120306, /* 70000BE8_bnr_iHighMaxSlopeAllowed   */
	0x0F122003, /* 70000BEA_bnr_iHighSlopeThresh   */
	0x0F12FF01, /* 70000BEC_bnr_iSlopeBlurStrength   */
	0x0F120000, /* 70000BEE_bnr_AddNoisePower1   */
	0x0F120300, /* 70000BF0_bnr_iRadialTune   */
	0x0F12145A, /* 70000BF2_bnr_iRadialLimit   */
	0x0F121010, /* 70000BF4_ee_iFSMagThHigh   */
	0x0F12000B, /* 70000BF6_ee_iFSVarThHigh   */
	0x0F121000, /* 70000BF8_ee_iFSThHigh   */
	0x0F125A0F, /* 70000BFA_ee_iFSVarCountTh   */
	0x0F120503, /* 70000BFC_ee_iRadialPower   */
	0x0F121802, /* 70000BFE_ee_iROADThres   */
	0x0F120000, /* 70000C00_ee_iROADSubMaxNR   */
	0x0F122006, /* 70000C02_ee_iROADNeiThres   */
	0x0F123028, /* 70000C04_ee_iSmoothEdgeThres   */
	0x0F12041A, /* 70000C06_ee_iWSharpen   */
	0x0F120101, /* 70000C08_ee_iWShThresh   */
	0x0F12FF00, /* 70000C0A_ee_iEmbossCentAdd   */
	0x0F120904, /* 70000C0C_ee_iReduceEdgeThresh   */
	0x0F124008, /* 70000C0E_dmsc_iDesatThresh   */
	0x0F120540, /* 70000C10_dmsc_iDemBlurLow   */
	0x0F128006, /* 70000C12_dmsc_iDecisionThresh   */
	0x0F120020, /* 70000C14_dmsc_iMonochrom   */
	0x0F120000, /* 70000C16_dmsc_iGRDenoiseVal   */
	0x0F121800, /* 70000C18_dmsc_iEdgeDesatThrLow   */
	0x0F120002, /* 70000C1A_dmsc_iNearGrayDesat   */
	0x0F121E10, /* 70000C1C_postdmsc_iBCoeff   */
	0x0F12000B, /* 70000C1E_postdmsc_iWideMult   */
	0x0F120607, /* 70000C20_yuvemix_mNegSlopes_1   */
	0x0F120005, /* 70000C22_yuvemix_mNegSlopes_3   */
	0x0F120607, /* 70000C24_yuvemix_mPosSlopes_1   */
	0x0F120405, /* 70000C26_yuvemix_mPosSlopes_3   */
	0x0F120206, /* 70000C28_yuviirnr_iXSupportUV   */
	0x0F120304, /* 70000C2A_yuviirnr_iHighYNorm   */
	0x0F120409, /* 70000C2C_yuviirnr_iHighUVNorm   */
	0x0F120305, /* 70000C2E_yuviirnr_iUVNormShift   */
	0x0F120406, /* 70000C30_yuviirnr_iVertLength_UV   */
	0x0F122804, /* 70000C32_yuviirnr_iDiffThreshH_Y   */
	0x0F120228, /* 70000C34_yuviirnr_iDiffThreshH_UV   */
	0x0F121402, /* 70000C36_yuviirnr_iMaxThreshH_Y   */
	0x0F120618, /* 70000C38_yuviirnr_iMaxThreshH_UV   */
	0x0F121A02, /* 70000C3A_yuviirnr_iYNRStrengthH   */
	0x0F126E18, /* 70000C3C_yuviirnr_iUVNRStrengthH   */
	0x0F120080, /* 70000C3E_RGBGamma2_iLinearity   */
	0x0F120080, /* 70000C40_ccm_oscar_iSaturation   */
	0x0F120180, /* 70000C42_RGB2YUV_iRGBGain   */
	0x0F120A0A, /* 70000C44_bnr_iClustMulT_H   */
	0x0F120101, /* 70000C46_bnr_iClustThresh_H   */
	0x0F12232B, /* 70000C48_bnr_iDenThreshLow   */
	0x0F126024, /* 70000C4A_ee_iLowSharpPower   */
	0x0F121414, /* 70000C4C_ee_iLowShDenoise   */
	0x0F12FFFF, /* 70000C4E_ee_iLowSharpClamp   */
	0x0F120808, /* 70000C50_ee_iReduceEdgeMinMult   */
	0x0F120A01, /* 70000C52_bnr_nClustLevel_H_Bin   */
	0x0F12010A, /* 70000C54_bnr_iClustMulT_C_Bin   */
	0x0F121501, /* 70000C56_bnr_iClustThresh_C_Bin   */
	0x0F12240F, /* 70000C58_bnr_iDenThreshHigh_Bin   */
	0x0F120A60, /* 70000C5A_ee_iHighSharpPower_Bin   */
	0x0F12FF0A, /* 70000C5C_ee_iHighShDenoise_Bin   */
	0x0F1208FF, /* 70000C5E_ee_iHighSharpClamp_Bin   */
	0x0F120008, /* 70000C60_ee_iReduceEdgeSlope_Bin */
	0x0F120001, /* 70000C62_bnr_nClustLevel_C      */
	0x0F120000, /* 70000C64_BRIGHTNESS   AFIT 4 */
	0x0F120000, /* 70000C66_CONTRAST */
	0x0F120000, /* 70000C68_SATURATION */
	0x0F120000, /* 70000C6A_SHARP_BLUR */
	0x0F120000, /* 70000C6C_GLAMOUR */
	0x0F1200C0, /* 70000C6E_bnr_edge_high */
	0x0F120064, /* 70000C70_postdmsc_iLowBright */
	0x0F120384, /* 70000C72_postdmsc_iHighBright */
	0x0F120032, /* 70000C74_postdmsc_iLowSat */
	0x0F1201F4, /* 70000C76_postdmsc_iHighSat */
	0x0F120070, /* 70000C78_postdmsc_iTune */
	0x0F120040, /* 70000C7A_yuvemix_mNegRanges_0 */
	0x0F1200A0, /* 70000C7C_yuvemix_mNegRanges_1 */
	0x0F120100, /* 70000C7E_yuvemix_mNegRanges_2 */
	0x0F120010, /* 70000C80_yuvemix_mPosRanges_0 */
	0x0F120060, /* 70000C82_yuvemix_mPosRanges_1 */
	0x0F120100, /* 70000C84_yuvemix_mPosRanges_2 */
	0x0F121430, /* 70000C86_bnr_edge_low  */
	0x0F120201, /* 70000C88_bnr_repl_force  */
	0x0F120204, /* 70000C8A_bnr_iHotThreshLow   */
	0x0F120F04, /* 70000C8C_bnr_iColdThreshLow   */
	0x0F12030C, /* 70000C8E_bnr_DispTH_High   */
	0x0F120003, /* 70000C90_bnr_DISP_Limit_High   */
	0x0F120602, /* 70000C92_bnr_iDistSigmaMax   */
	0x0F121803, /* 70000C94_bnr_iDiffSigmaHigh   */
	0x0F120040, /* 70000C96_bnr_iNormalizedSTD_Limit   */
	0x0F120E20, /* 70000C98_bnr_iDirMinThres   */
	0x0F122018, /* 70000C9A_bnr_iDirFltDiffThresLow   */
	0x0F120620, /* 70000C9C_bnr_iDirSmoothPowerLow   */
	0x0F120306, /* 70000C9E_bnr_iHighMaxSlopeAllowed   */
	0x0F122003, /* 70000CA0_bnr_iHighSlopeThresh   */
	0x0F12FF01, /* 70000CA2_bnr_iSlopeBlurStrength   */
	0x0F120000, /* 70000CA4_bnr_AddNoisePower1   */
	0x0F120200, /* 70000CA6_bnr_iRadialTune   */
	0x0F12145A, /* 70000CA8_bnr_iRadialLimit   */
	0x0F121010, /* 70000CAA_ee_iFSMagThHigh   */
	0x0F12000B, /* 70000CAC_ee_iFSVarThHigh   */
	0x0F121200, /* 70000CAE_ee_iFSThHigh   */
	0x0F125A0F, /* 70000CB0_ee_iFSVarCountTh   */
	0x0F120502, /* 70000CB2_ee_iRadialPower   */
	0x0F121802, /* 70000CB4_ee_iROADThres   */
	0x0F120000, /* 70000CB6_ee_iROADSubMaxNR   */
	0x0F122006, /* 70000CB8_ee_iROADNeiThres   */
	0x0F124028, /* 70000CBA_ee_iSmoothEdgeThres   */
	0x0F120430, /* 70000CBC_ee_iWSharpen   */
	0x0F120101, /* 70000CBE_ee_iWShThresh   */
	0x0F12FF00, /* 70000CC0_ee_iEmbossCentAdd   */
	0x0F120804, /* 70000CC2_ee_iReduceEdgeThresh   */
	0x0F124008, /* 70000CC4_dmsc_iDesatThresh   */
	0x0F120540, /* 70000CC6_dmsc_iDemBlurLow   */
	0x0F128006, /* 70000CC8_dmsc_iDecisionThresh   */
	0x0F120020, /* 70000CCA_dmsc_iMonochrom   */
	0x0F120000, /* 70000CCC_dmsc_iGRDenoiseVal   */
	0x0F121800, /* 70000CCE_dmsc_iEdgeDesatThrLow   */
	0x0F120002, /* 70000CD0_dmsc_iNearGrayDesat   */
	0x0F121E10, /* 70000CD2_postdmsc_iBCoeff   */
	0x0F12000B, /* 70000CD4_postdmsc_iWideMult   */
	0x0F120607, /* 70000CD6_yuvemix_mNegSlopes_1   */
	0x0F120005, /* 70000CD8_yuvemix_mNegSlopes_3   */
	0x0F120607, /* 70000CDA_yuvemix_mPosSlopes_1   */
	0x0F120405, /* 70000CDC_yuvemix_mPosSlopes_3   */
	0x0F120205, /* 70000CDE_yuviirnr_iXSupportUV   */
	0x0F120304, /* 70000CE0_yuviirnr_iHighYNorm   */
	0x0F120409, /* 70000CE2_yuviirnr_iHighUVNorm   */
	0x0F120306, /* 70000CE4_yuviirnr_iUVNormShift   */
	0x0F120407, /* 70000CE6_yuviirnr_iVertLength_UV   */
	0x0F122C04, /* 70000CE8_yuviirnr_iDiffThreshH_Y   */
	0x0F12022C, /* 70000CEA_yuviirnr_iDiffThreshH_UV   */
	0x0F121402, /* 70000CEC_yuviirnr_iMaxThreshH_Y   */
	0x0F120618, /* 70000CEE_yuviirnr_iMaxThreshH_UV   */
	0x0F121A02, /* 70000CF0_yuviirnr_iYNRStrengthH   */
	0x0F126E18, /* 70000CF2_yuviirnr_iUVNRStrengthH   */
	0x0F120080, /* 70000CF4_RGBGamma2_iLinearity   */
	0x0F120080, /* 70000CF6_ccm_oscar_iSaturation   */
	0x0F120180, /* 70000CF8_RGB2YUV_iRGBGain   */
	0x0F120A0A, /* 70000CFA_bnr_iClustMulT_H   */
	0x0F120101, /* 70000CFC_bnr_iClustThresh_H   */
	0x0F121114, /* 70000CFE_bnr_iDenThreshLow   */
	0x0F126024, /* 70000D00_ee_iLowSharpPower   */
	0x0F121212, /* 70000D02_ee_iLowShDenoise   */
	0x0F12FFFF, /* 70000D04_ee_iLowSharpClamp   */
	0x0F120808, /* 70000D06_ee_iReduceEdgeMinMult   */
	0x0F120A01, /* 70000D08_bnr_nClustLevel_H_Bin   */
	0x0F12010A, /* 70000D0A_bnr_iClustMulT_C_Bin   */
	0x0F120F01, /* 70000D0C_bnr_iClustThresh_C_Bin   */
	0x0F12240C, /* 70000D0E_bnr_iDenThreshHigh_Bin   */
	0x0F120860, /* 70000D10_ee_iHighSharpPower_Bin   */
	0x0F12FF08, /* 70000D12_ee_iHighShDenoise_Bin   */
	0x0F1208FF, /* 70000D14_ee_iHighSharpClamp_Bin   */
	0x0F120008, /* 70000D16_ee_iReduceEdgeSlope_Bin */
	0x0F120001, /* 70000D18_bnr_nClustLevel_C      */

	0x002A060C,
	0x0F120800, /* lt_ExpGain_ExpCurveGainMaxStr */
	/* FOR HD CAMCORDER */

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs */
	0x0F120A00, /* REG_TC_GP_CapReqInputWidth */
	0x0F120780, /* REG_TC_GP_CapReqInputHeight */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs */
	0x0F12000C, /* REG_TC_GP_CapInputHeightOfs */
	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_PrevZoomReqInputWidth */
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputHeight */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */
	0x0F120A00, /* REG_TC_PZOOM_CapZoomReqInputWidth */
	0x0F120780, /* REG_TC_PZOOM_CapZoomReqInputHeight */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A02A6,
	0x0F120280, /* REG_0TC_PCFG_usWidth */
	0x0F1201E0, /* REG_0TC_PCFG_usHeight */
	0x002A02AB,
	0x0F120005, /* REG_0TC_PCFG_Format
			05 : yuv (0~255)  06:yuv (16~234) 07: raw 09 : jpeg */
	0x002A0266,
	0x0F120000, /* #REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* #REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* #REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* #REG_TC_GP_PrevConfigChanged */
	0x002A0270,
	0x0F120001, /* #REG_TC_GP_CapConfigChanged */
	0x002A023E,
	0x0F120001, /* #REG_TC_GP_EnablePreview */
	0x0F120001, /* #REG_TC_GP_EnablePreviewChanged	*/
};

static const u32 s5k4ec_flash_init[] = {
	0x002A0484,
	0x0F120002, /* capture flash on */

	0x002A183A,
	0x0F120001, /* one frame AE */

	0x002A17F6,
	0x0F120260, /* AWB R point */
	0x0F120210, /* AWB B point */

	0x002A1840,
	0x0F120001, /* Fls AE tune start */

	0x0F120100, /* fls_afl_FlsAFIn  Rin */
	0x0F120120,
	0x0F120180,
	0x0F120200,
	0x0F120400,
	0x0F120800,
	0x0F120A00,
	0x0F121000,

	0x0F120100, /* fls_afl_FlsAFOut  Rout */
	0x0F1200A0,
	0x0F120090,
	0x0F120080,
	0x0F120070,
	0x0F120045,
	0x0F120030,
	0x0F120010,

	0x002A1884,
	0x0F120100, /* fls_afl_FlsNBOut  flash NB default */
	0x0F120100,
	0x0F120100,
	0x0F120100,
	0x0F120100,
	0x0F120100,
	0x0F120100,
	0x0F120100,

	0x002A4784,
	0x0F1200A0, /* TNP_Regs_FlsWeightRIn  weight tune start in */
	0x0F1200C0,
	0x0F1200D0,
	0x0F120100,
	0x0F120200,
	0x0F120300,

	0x0F120088, /* TNP_Regs_FlsWeightROut  weight tune start out */
	0x0F1200B0,
	0x0F1200C0,
	0x0F120100,
	0x0F120200,
	0x0F120300,

	0x002A479C,

	0x0F120120, /* Fls  BRIn  */
	0x0F120150,
	0x0F120200,

	0x0F12003C, /* Fls  BROut */
	0x0F12003B,
	0x0F120026,
};

static const u32 s5k4ec_pre_flash_start[] = {
	0xFCFCD000,
	0x00287000,
	0x002A17FC, /* fls_FlashWP_0_Pre_Flash_Start */
	0x0F120001,
};

static const u32 s5k4ec_pre_flash_end[] = {
	0xFCFCD000,
	0x00287000,
	0x002A1800, /* fls_afl_FlashWP_Weight_0_Pre_Flash_end */
	0x0F120001,
};

static const u32 s5k4ec_flash_start[] = {
	0xFCFCD000,
	0x00287000,
	0x002A17E8, /* fls_afl_FlashMode:Flash alg start */
	0x0F120001,
	0x002A180C, /* fls_afl_FlashWP_Weight_4:flash br avg */
	0x0F120027,
};

static const u32 s5k4ec_flash_end[] = {
	0xFCFCD000,
	0x00287000,
	0x002A17E8, /* fls_afl_FlashMode  Flash alg end */
	0x0F120000,
};

static const u32 s5k4ec_fast_ae_on[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0588, /* _afl_FlashMode  Flash alg end */
	0x0F120000,
};

static const u32 s5k4ec_fast_ae_off[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0588, /* _afl_FlashMode  Flash alg end */
	0x0F120002,
};

static const u32 s5k4ec_luxcheck[] = {

};

static const u32 s5k4ec_50hz_auto[] = {
/* Step1 : Set 50Hz fixed mode in order to stop previous setting */
	0x002A0F30,
	0x0F120001, /* AFC_D_ConvAccelerPower */
	0x002A04E6,
	0x0F12075F,
	0x002A04D6,
	0x0F120001,
	0x002A04D8,
	0x0F120001,

/* Step2 : Set 50Hz auto mode */
	0x002A0F30,
	0x0F120001, /* AFC_D_ConvAccelerPower */
	0x002A0F2A,
	0x0F120000, /* AFC_Default BIT[0] 1:60Hz 0:50Hz */

/* Step3 : Restore  auto mode */
	0xFFFF0064,	/* Delay 100ms */
	0x002A04E6,
	0x0F12077F
};

static const u32 s5k4ec_50hz_fixed[] = {
	0x002A0F30,
	0x0F120001, /* AFC_D_ConvAccelerPower */
	0x002A04E6,
	0x0F12075F,
	0x002A04D6,
	0x0F120001,
	0x002A04D8,
	0x0F120001,
};

static const u32 s5k4ec_60hz_auto[] = {
/* Step1 : Set 60Hz fixed mode in order to stop previous setting */
	0x002A0F30,
	0x0F120001, /* AFC_D_ConvAccelerPower */
	0x002A04E6,
	0x0F12075F,
	0x002A04D6,
	0x0F120002,
	0x002A04D8,
	0x0F120001,

/* Step2 : Set 60Hz auto mode */
	0x002A0F30,
	0x0F120001, /* AFC_D_ConvAccelerPower */
	0x002A0F2A,
	0x0F120001, /* AFC_Default BIT[0] 1:60Hz 0:50Hz */

/* Step3 : Restore  auto mode */
	0xFFFF0064, /* Delay 100ms */
	0x002A04E6,
	0x0F12077F
};

static const u32 s5k4ec_60hz_fixed[] = {
	0x002A0F30,
	0x0F120001, /* AFC_D_ConvAccelerPower */
	0x002A04E6,
	0x0F12075F,
	0x002A04D6,
	0x0F120002,
	0x002A04D8,
	0x0F120001,
};

/* Capture : 2576 x 1932 */
static const u32 s5k4ec_2576_1932_capture[] = {
	0xFCFCD000,

	0x00287000,
	0x002A0258,
	0x0F120A10, /* REG_TC_GP_CapReqInputWidth 2576 */
	0x0F12078C, /* REG_TC_GP_CapReqInputHeight 1932 */
	0x0F120008, /* REG_TC_GP_CapInputWidthOfs (2592-2576)/2 */
	0x0F120006, /* REG_TC_GP_CapInputHeightOfs (1944-1932)/2 */

	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A10, /* REG_TC_PZOOM_CapZoomReqInputWidth 2576 */
	0x0F12078C, /* REG_TC_PZOOM_CapZoomReqInputHeight 1932 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120280, /* REG_TC_THUMB_Thumb_uWidth 640 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight 480 */

	0x002A0398,
	0x0F120A10, /* REG_0TC_CCFG_usWidth 2576 */
	0x0F12078C, /* REG_0TC_CCFG_usHeight 1932 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : 5M 2560 x 1920 */
static const u32 s5k4ec_5m_capture[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0258,
	0x0F120A00, /* REG_TC_GP_CapReqInputWidth //2560 */
	0x0F120780, /* REG_TC_GP_CapReqInputHeight //1920 */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs //(2592-2560)/2 */
	0x0F12000C, /* REG_TC_GP_CapInputHeightOfs //(1944-1920)/2 */
	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A00, /* REG_TC_PZOOM_CapZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_CapZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120280, /* REG_TC_THUMB_Thumb_uWidth //640 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight //480 */

	0x00287000,
	0x002A0396,
	0x0F120000, /* REG_0TC_CCFG_uCaptureMode */
	0x0F120A00, /* REG_0TC_CCFG_usWidth */
	0x0F120780, /* REG_0TC_CCFG_usHeight */
	0x0F120005, /* REG_0TC_CCFG_Format */
	0x0F12278D, /* REG_0TC_CCFG_usMaxOut4KHzRate */
	0x0F12278D, /* REG_0TC_CCFG_usMinOut4KHzRate */
	0x0F120100, /* REG_0TC_CCFG_OutClkPerPix88 */
	0x0F120300, /* REG_0TC_CCFG_uBpp88 */
	0x0F120002, /* REG_0TC_CCFG_PVIMask */
	0x0F120000, /* REG_0TC_CCFG_OIFMask */
	0x0F120810, /* REG_0TC_CCFG_usJpegPacketSize */
	0x0F120000, /* REG_0TC_CCFG_usJpegTotalPackets */
	0x0F120000, /* REG_0TC_CCFG_uClockInd */
	0x0F120000, /* REG_0TC_CCFG_usFrTimeType */
	0x0F120002, /* REG_0TC_CCFG_FrRateQualityType */
	0x0F120535, /* REG_0TC_CCFG_usMaxFrTimeMsecMult10 */
	0x0F12029A, /* REG_0TC_CCFG_usMinFrTimeMsecMult10 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture 2560 x 1440 */
static const u32 s5k4ec_2560_1440_capture[] = {
	0xFCFCD000,

	0x00287000,
	0x002A0258,
	0x0F120A00, /* REG_TC_GP_CapReqInputWidth //2560 */
	0x0F1205A0, /* REG_TC_GP_CapReqInputHeight //1440 */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs //(2592-2560)/2 */
	0x0F1200FC, /* REG_TC_GP_CapInputHeightOfs //(1944-1440)/2 */

	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A00, /* REG_TC_PZOOM_CapZoomReqInputWidth //2560 */
	0x0F1205A0, /* REG_TC_PZOOM_CapZoomReqInputHeight //1440 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A0398,
	0x0F120A00, /* REG_0TC_CCFG_usWidth //2560 */
	0x0F1205A0, /* REG_0TC_CCFG_usHeight //1440 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : W4MP 2560 x 1536 */
static const u32 s5k4ec_w4mp_capture[] = {
	0xFCFCD000,

	0x00287000,
	0x002A0258,
	0x0F120A00, /* REG_TC_GP_CapReqInputWidth //2560 */
	0x0F120600, /* REG_TC_GP_CapReqInputHeight //1536 */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs //(2592-2560)/2 */
	0x0F1200CC, /* REG_TC_GP_CapInputHeightOfs //(1944-1536)/2 */

	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A00, /* REG_TC_PZOOM_CapZoomReqInputWidth //2560 */
	0x0F120600, /* REG_TC_PZOOM_CapZoomReqInputHeight //1536 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120320, /* REG_TC_THUMB_Thumb_uWidth //800 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight //480 */

	0x002A0398,
	0x0F120A00, /* REG_0TC_CCFG_usWidth //2560 */
	0x0F120600, /* REG_0TC_CCFG_usHeight //1536 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : 2576 x 1544 */
static const u32 s5k4ec_2576_1544_capture[] = {
	0xFCFCD000,

	0x00287000,
	0x002A0258,
	0x0F120A10, /* REG_TC_GP_CapReqInputWidth 2576 */
	0x0F120608, /* REG_TC_GP_CapReqInputHeight 1544 */
	0x0F120008, /* REG_TC_GP_CapInputWidthOfs (2592-2576)/2 */
	0x0F1200C8, /* REG_TC_GP_CapInputHeightOfs (1944-1544)/2 */

	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A10, /* REG_TC_PZOOM_CapZoomReqInputWidth 2576 */
	0x0F120608, /* REG_TC_PZOOM_CapZoomReqInputHeight 1544 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120320, /* REG_TC_THUMB_Thumb_uWidth 800 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight 480 */

	0x002A0398,
	0x0F120A10, /* REG_0TC_CCFG_usWidth //2576 */
	0x0F120608, /* REG_0TC_CCFG_usHeight //1544 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : 3M 2048 x 1536 */
static const u32 s5k4ec_3m_capture[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0258,
	0x0F120A00, /* REG_TC_GP_CapReqInputWidth //2560 */
	0x0F120780, /* REG_TC_GP_CapReqInputHeight //1920 */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs //(2592-2560)/2 */
	0x0F12000C, /* REG_TC_GP_CapInputHeightOfs //(1944-1920)/2 */
	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A00, /* REG_TC_PZOOM_CapZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_CapZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120280, /* REG_TC_THUMB_Thumb_uWidth //640 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight //480 */

	0x00287000,
	0x002A0398,
	0x0F120800, /* REG_0TC_CCFG_usWidth */
	0x0F120600, /* REG_0TC_CCFG_usWidth */
	0x0F120005, /* REG_0TC_CCFG_Format */
	0x0F12278D, /* REG_0TC_CCFG_usMaxOut4KHzRate */
	0x0F12278D, /* REG_0TC_CCFG_usMinOut4KHzRate */
	0x0F120100, /* REG_0TC_CCFG_OutClkPerPix88 */
	0x0F120300, /* REG_0TC_CCFG_uBpp88 */
	0x0F120002, /* REG_0TC_CCFG_PVIMask */
	0x0F120000, /* REG_0TC_CCFG_OIFMask */
	0x0F120810, /* REG_0TC_CCFG_usJpegPacketSize */
	0x0F120000, /* REG_0TC_CCFG_usJpegTotalPackets */
	0x0F120000, /* REG_0TC_CCFG_uClockInd */
	0x0F120000, /* REG_0TC_CCFG_usFrTimeType */
	0x0F120002, /* REG_0TC_CCFG_FrRateQualityType */
	0x0F120535, /* REG_0TC_CCFG_usMaxFrTimeMsecMult10 */
	0x0F12029A, /* REG_0TC_CCFG_usMinFrTimeMsecMult10 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : 2048 x 1152 */
static const u32 s5k4ec_2048_1152_capture[] = {
	0xFCFCD000,

	0x00287000,
	0x002A0258,
	0x0F120A00, /* REG_TC_GP_CapReqInputWidth //2560 */
	0x0F1205A0, /* REG_TC_GP_CapReqInputHeight //1440 */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs //(2592-2560)/2 */
	0x0F1200FC, /* REG_TC_GP_CapInputHeightOfs //(1944-1440)/2 */

	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A00, /* REG_TC_PZOOM_CapZoomReqInputWidth //2560 */
	0x0F1205A0, /* REG_TC_PZOOM_CapZoomReqInputHeight //1440 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A0398,
	0x0F120800, /* REG_0TC_CCFG_usWidth //2048 */
	0x0F120480, /* REG_0TC_CCFG_usHeight //1152 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : 2048 x 1232 */
static const u32 s5k4ec_2048_1232_capture[] = {
	0xFCFCD000,

	0x00287000,
	0x002A0258,
	0x0F120A00, /* REG_TC_GP_CapReqInputWidth //2560 */
	0x0F120600, /* REG_TC_GP_CapReqInputHeight //1536 */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs //(2592-2560)/2 */
	0x0F1200CC, /* REG_TC_GP_CapInputHeightOfs //(1944-1536)/2 */

	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A00, /* REG_TC_PZOOM_CapZoomReqInputWidth //2560 */
	0x0F120600, /* REG_TC_PZOOM_CapZoomReqInputHeight //1536 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120320, /* REG_TC_THUMB_Thumb_uWidth //800 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight //480 */

	0x002A0398,
	0x0F120800, /* REG_0TC_CCFG_usWidth //2048 */
	0x0F1204D0, /* REG_0TC_CCFG_usHeight //1232 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : 1920 x 1920 */
static const u32 s5k4ec_1920_1920_capture[] = {
	0xFCFCD000,

	0x00287000,
	0x002A0258,
	0x0F12078C, /* REG_TC_GP_CapReqInputWidth 1932 */
	0x0F12078C, /* REG_TC_GP_CapReqInputHeight 1932 */
	0x0F12014A, /* REG_TC_GP_CapInputWidthOfs (2592-1932)/2 */
	0x0F120006, /* REG_TC_GP_CapInputHeightOfs (1944-1932)/2 */

	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F12078C, /* REG_TC_PZOOM_CapZoomReqInputWidth 1932 */
	0x0F12078C, /* REG_TC_PZOOM_CapZoomReqInputHeight 1932 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uWidth 480 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight 480 */

	0x002A0398,
	0x0F120780, /* REG_0TC_CCFG_usWidth 1920 */
	0x0F120780, /* REG_0TC_CCFG_usHeight 1920 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : 1928 x 1928 */
static const u32 s5k4ec_1928_1928_capture[] = {
	0xFCFCD000,

	0x00287000,
	0x002A0258,
	0x0F120788, /* REG_TC_GP_CapReqInputWidth 1928 */
	0x0F120788, /* REG_TC_GP_CapReqInputHeight 1928 */
	0x0F12014C, /* REG_TC_GP_CapInputWidthOfs (2592-1928)/2 */
	0x0F120008, /* REG_TC_GP_CapInputHeightOfs (1944-1928)/2 */

	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120788, /* REG_TC_PZOOM_CapZoomReqInputWidth 1928 */
	0x0F120788, /* REG_TC_PZOOM_CapZoomReqInputHeight 1928 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uWidth 480 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight 480 */

	0x002A0398,
	0x0F120788, /* REG_0TC_CCFG_usWidth //1928 */
	0x0F120788, /* REG_0TC_CCFG_usHeight //1928 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : 2036 x 1526 */
static const u32 s5k4ec_2036_1526_capture[] = {
	0xFCFCD000,

	0x00287000,
	0x002A0258,
	0x0F120A10, /* REG_TC_GP_CapReqInputWidth 2576 */
	0x0F12078C, /* REG_TC_GP_CapReqInputHeight 1932 */
	0x0F120008, /* REG_TC_GP_CapInputWidthOfs (2592-2576)/2 */
	0x0F120006, /* REG_TC_GP_CapInputHeightOfs (1944-1932)/2 */

	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A10, /* REG_TC_PZOOM_CapZoomReqInputWidth 2576 */
	0x0F12078C, /* REG_TC_PZOOM_CapZoomReqInputHeight 1932 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120280, /* REG_TC_THUMB_Thumb_uWidth 640 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight 480 */

	0x002A0398,
	0x0F1207F4, /* REG_0TC_CCFG_usWidth 2036 */
	0x0F1205F6, /* REG_0TC_CCFG_usHeight 1526 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : 1632 x 1218 */
static const u32 s5k4ec_1632_1218_capture[] = {
	0xFCFCD000,

	0x00287000,
	0x002A0258,
	0x0F120A10, /* REG_TC_GP_CapReqInputWidth 2576 */
	0x0F12078C, /* REG_TC_GP_CapReqInputHeight 1932 */
	0x0F120008, /* REG_TC_GP_CapInputWidthOfs (2592-2576)/2 */
	0x0F120006, /* REG_TC_GP_CapInputHeightOfs (1944-1932)/2 */

	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A10, /* REG_TC_PZOOM_CapZoomReqInputWidth 2576 */
	0x0F12078C, /* REG_TC_PZOOM_CapZoomReqInputHeight 1932 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120280, /* REG_TC_THUMB_Thumb_uWidth 640 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight 480 */

	0x002A0398,
	0x0F120660, /* REG_0TC_CCFG_usWidth 1632 */
	0x0F1204C2, /* REG_0TC_CCFG_usHeight 1218 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : W1MP 1600 x 960 */
static const u32 s5k4ec_w1mp_capture[] = {
	0xFCFCD000,

	0x00287000,
	0x002A0258,
	0x0F120A00, /* REG_TC_GP_CapReqInputWidth //2560 */
	0x0F120600, /* REG_TC_GP_CapReqInputHeight //1536 */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs //(2592-2560)/2 */
	0x0F1200CC, /* REG_TC_GP_CapInputHeightOfs //(1944-1536)/2 */

	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A00, /* REG_TC_PZOOM_CapZoomReqInputWidth //2560 */
	0x0F120600, /* REG_TC_PZOOM_CapZoomReqInputHeight //1536 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120320, /* REG_TC_THUMB_Thumb_uWidth //800 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight //480 */

	0x002A0398,
	0x0F120640, /* REG_0TC_CCFG_usWidth //1600 */
	0x0F1203C0, /* REG_0TC_CCFG_usHeight //960 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : WVGA 800 x 480 */
static const u32 s5k4ec_wvga_capture[] = {
	0xFCFCD000,

	0x00287000,
	0x002A0258,
	0x0F120A00, /* REG_TC_GP_CapReqInputWidth //2560 */
	0x0F120600, /* REG_TC_GP_CapReqInputHeight //1536 */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs //(2592-2560)/2 */
	0x0F1200CC, /* REG_TC_GP_CapInputHeightOfs //(1944-1536)/2 */

	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A00, /* REG_TC_PZOOM_CapZoomReqInputWidth //2560 */
	0x0F120600, /* REG_TC_PZOOM_CapZoomReqInputHeight //1536 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120320, /* REG_TC_THUMB_Thumb_uWidth //800 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight //480 */

	0x002A0398,
	0x0F120320, /* REG_0TC_CCFG_usWidth //800 */
	0x0F1201E0, /* REG_0TC_CCFG_usHeight //480 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : 2M 1600 x 1200 */
static const u32 s5k4ec_2m_capture[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0258,
	0x0F120A00, /* REG_TC_GP_CapReqInputWidth //2560 */
	0x0F120780, /* REG_TC_GP_CapReqInputHeight //1920 */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs //(2592-2560)/2 */
	0x0F12000C, /* REG_TC_GP_CapInputHeightOfs //(1944-1920)/2 */
	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A00, /* REG_TC_PZOOM_CapZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_CapZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120000, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120280, /* REG_TC_THUMB_Thumb_uWidth //640 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight //480 */

	0x00287000,
	0x002A0398,
	0x0F120640, /* REG_0TC_CCFG_usWidth */
	0x0F1204B0, /* REG_0TC_CCFG_usHeight */
	0x0F120005, /* REG_0TC_CCFG_Format */
	0x0F12278D, /* REG_0TC_CCFG_usMaxOut4KHzRate */
	0x0F12278D, /* REG_0TC_CCFG_usMinOut4KHzRate */
	0x0F120100, /* REG_0TC_CCFG_OutClkPerPix88 */
	0x0F120300, /* REG_0TC_CCFG_uBpp88 */
	0x0F120002, /* REG_0TC_CCFG_PVIMask */
	0x0F120000, /* REG_0TC_CCFG_OIFMask */
	0x0F120810, /* REG_0TC_CCFG_usJpegPacketSize */
	0x0F120000, /* REG_0TC_CCFG_usJpegTotalPackets */
	0x0F120000, /* REG_0TC_CCFG_uClockInd */
	0x0F120000, /* REG_0TC_CCFG_usFrTimeType */
	0x0F120002, /* REG_0TC_CCFG_FrRateQualityType */
	0x0F120535, /* REG_0TC_CCFG_usMaxFrTimeMsecMult10 */
	0x0F12029A, /* REG_0TC_CCFG_usMinFrTimeMsecMult10 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : 1M 1280 x 960 */
static const u32 s5k4ec_1m_capture[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0258,
	0x0F120A00, /* REG_TC_GP_CapReqInputWidth //2560 */
	0x0F120780, /* REG_TC_GP_CapReqInputHeight //1920 */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs //(2592-2560)/2 */
	0x0F12000C, /* REG_TC_GP_CapInputHeightOfs //(1944-1920)/2 */
	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A00, /* REG_TC_PZOOM_CapZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_CapZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120280, /* REG_TC_THUMB_Thumb_uWidth //640 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight //480 */

	0x00287000,
	0x002A0398,
	0x0F120500, /* REG_0TC_CCFG_usWidth */
	0x0F1203C0, /* REG_0TC_CCFG_usHeight */
	0x0F120005, /* REG_0TC_CCFG_Format */
	0x0F12278D, /* REG_0TC_CCFG_usMaxOut4KHzRate */
	0x0F12278D, /* REG_0TC_CCFG_usMinOut4KHzRate */
	0x0F120100, /* REG_0TC_CCFG_OutClkPerPix88 */
	0x0F120300, /* REG_0TC_CCFG_uBpp88 */
	0x0F120002, /* REG_0TC_CCFG_PVIMask */
	0x0F120000, /* REG_0TC_CCFG_OIFMask */
	0x0F120810, /* REG_0TC_CCFG_usJpegPacketSize */
	0x0F120000, /* REG_0TC_CCFG_usJpegTotalPackets */
	0x0F120000, /* REG_0TC_CCFG_uClockInd */
	0x0F120000, /* REG_0TC_CCFG_usFrTimeType */
	0x0F120002, /* REG_0TC_CCFG_FrRateQualityType */
	0x0F120535, /* REG_0TC_CCFG_usMaxFrTimeMsecMult10 */
	0x0F12029A, /* REG_0TC_CCFG_usMinFrTimeMsecMult10 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : XGA 1024 x 768 */
static const u32 s5k4ec_xga_capture[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0258,
	0x0F120A00, /* REG_TC_GP_CapReqInputWidth //2560 */
	0x0F120780, /* REG_TC_GP_CapReqInputHeight //1920 */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs //(2592-2560)/2 */
	0x0F12000C, /* REG_TC_GP_CapInputHeightOfs //(1944-1920)/2 */
	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A00, /* REG_TC_PZOOM_CapZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_CapZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120280, /* REG_TC_THUMB_Thumb_uWidth //640 */
	0x0F1201E0, /* REG_TC_THUMB_Thumb_uHeight //480 */

	0x00287000,
	0x002A0398,
	0x0F120400, /* REG_0TC_CCFG_usWidth */
	0x0F120300, /* REG_0TC_CCFG_usHeight */
	0x0F120005, /* REG_0TC_CCFG_Format */
	0x0F12278D, /* REG_0TC_CCFG_usMaxOut4KHzRate */
	0x0F12278D, /* REG_0TC_CCFG_usMinOut4KHzRate */
	0x0F120100, /* REG_0TC_CCFG_OutClkPerPix88 */
	0x0F120300, /* REG_0TC_CCFG_uBpp88 */
	0x0F120002, /* REG_0TC_CCFG_PVIMask */
	0x0F120000, /* REG_0TC_CCFG_OIFMask */
	0x0F120810, /* REG_0TC_CCFG_usJpegPacketSize */
	0x0F120000, /* REG_0TC_CCFG_usJpegTotalPackets */
	0x0F120000, /* REG_0TC_CCFG_uClockInd */
	0x0F120000, /* REG_0TC_CCFG_usFrTimeType */
	0x0F120002, /* REG_0TC_CCFG_FrRateQualityType */
	0x0F120535, /* REG_0TC_CCFG_usMaxFrTimeMsecMult10 */
	0x0F12029A, /* REG_0TC_CCFG_usMinFrTimeMsecMult10 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : VGA 640 x 480 */
static const u32 s5k4ec_vga_capture[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0258,
	0x0F120A00, /* REG_TC_GP_CapReqInputWidth //2560 */
	0x0F120780, /* REG_TC_GP_CapReqInputHeight //1920 */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs //(2592-2560)/2 */
	0x0F12000C, /* REG_TC_GP_CapInputHeightOfs //(1944-1920)/2 */
	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A00, /* REG_TC_PZOOM_CapZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_CapZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120000, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120140, /* REG_TC_THUMB_Thumb_uWidth //640 */
	0x0F1200F0, /* REG_TC_THUMB_Thumb_uHeight //480 */

	0x00287000,
	0x002A0398,
	0x0F120280, /* REG_0TC_CCFG_usWidth */
	0x0F1201E0, /* REG_0TC_CCFG_usHeight */
	0x0F120005, /* REG_0TC_CCFG_Format */
	0x0F12278D, /* REG_0TC_CCFG_usMaxOut4KHzRate */
	0x0F12278D, /* REG_0TC_CCFG_usMinOut4KHzRate */
	0x0F120100, /* REG_0TC_CCFG_OutClkPerPix88 */
	0x0F120300, /* REG_0TC_CCFG_uBpp88 */
	0x0F120002, /* REG_0TC_CCFG_PVIMask */
	0x0F120000, /* REG_0TC_CCFG_OIFMask */
	0x0F120810, /* REG_0TC_CCFG_usJpegPacketSize */
	0x0F120000, /* REG_0TC_CCFG_usJpegTotalPackets */
	0x0F120000, /* REG_0TC_CCFG_uClockInd */
	0x0F120000, /* REG_0TC_CCFG_usFrTimeType */
	0x0F120002, /* REG_0TC_CCFG_FrRateQualityType */
	0x0F120535, /* REG_0TC_CCFG_usMaxFrTimeMsecMult10 */
	0x0F12029A, /* REG_0TC_CCFG_usMinFrTimeMsecMult10 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Capture : QVGA 320 x 240 */
static const u32 s5k4ec_qvga_capture[] = {
	0xFCFCD000,
	0x00287000,
	0x002A0258,
	0x0F120A00, /* REG_TC_GP_CapReqInputWidth //2560 */
	0x0F120780, /* REG_TC_GP_CapReqInputHeight //1920 */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs //(2592-2560)/2 */
	0x0F12000C, /* REG_TC_GP_CapInputHeightOfs //(1944-1920)/2 */
	0x002A0264,
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A049C,
	0x0F120A00, /* REG_TC_PZOOM_CapZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_CapZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	0x002A047C,
	0x0F120001, /* REG_TC_THUMB_Thumb_bActive */
	0x0F120140, /* REG_TC_THUMB_Thumb_uWidth //640 */
	0x0F1200F0, /* REG_TC_THUMB_Thumb_uHeight //480 */

	0x002A0398,
	0x0F120140, /* #REG_0TC_CCFG_usWidth */
	0x0F1200F0, /* #REG_0TC_CCFG_usHeight */
	0x0F120005,
	0x002A03B2,
	0x0F120002,
	0x002A03B0,
	0x0F120002,
	0x002A024E,
	0x0F120001,
	0x002A0270,
	0x0F120001,
};

/*
* ///////////////// Preview ///////////////////
*/

/* Preview : 2560 x 1920 */
static const u32 s5k4ec_max_preview[] = {
	0xFCFCD000,
	0x00287000,

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs */

	0x0F120A00, /* REG_TC_GP_CapReqInputWidth */
	0x0F120780, /* REG_TC_GP_CapReqInputHeight */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs */
	0x0F12000C, /* REG_TC_GP_CapInputHeightOfs */

	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_ZoomInputWidth */
	0x0F120780, /* REG_TC_PZOOM_ZoomInputHeight */
	0x0F120000, /* REG_TC_PZOOM_ZoomInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_ZoomInputHeightOfs */
	0x0F120A00, /* REG_TC_CZOOM_ZoomInputWidth */
	0x0F120780, /* REG_TC_CZOOM_ZoomInputHeight */
	0x0F120000, /* REG_TC_CZOOM_ZoomInputWidthOfs */
	0x0F120000, /* REG_TC_CZOOM_ZoomInputHeightOfs */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A02A6,
	0x0F120A00, /* REG_0TC_PCFG_usWidth */
	0x0F120780, /* REG_0TC_PCFG_usHeight */
	0x0F120005, /* REG_0TC_PCFG_Format */
	0x0F12278D, /* REG_0TC_PCFG_usMaxOut4KHzRate */
	0x0F12278D, /* REG_0TC_PCFG_usMinOut4KHzRate */
	0x0F120100, /* REG_0TC_PCFG_OutClkPerPix88 */
	0x0F120300, /* REG_0TC_PCFG_uBpp88 */
	0x0F120002, /* REG_0TC_PCFG_PVIMask */
	0x0F120000, /* REG_0TC_PCFG_OIFMask */
	0x0F1201E0, /* REG_0TC_PCFG_usJpegPacketSize */
	0x0F120000, /* REG_0TC_PCFG_usJpegTotalPackets */
	0x0F120000, /* REG_0TC_PCFG_uClockInd */
	0x0F120000, /* REG_0TC_PCFG_usFrTimeType */
	0x0F120002, /* REG_0TC_PCFG_FrRateQualityType */
	0x0F120535,
	0x0F12028A, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Preview : 2048 x 1536 */
static const u32 s5k4ec_2048_preview[] = {
	0xFCFCD000,
	0x00287000,

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs */

	0x0F120A00, /* REG_TC_GP_CapReqInputWidth */
	0x0F120780, /* REG_TC_GP_CapReqInputHeight */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs */
	0x0F12000C, /* REG_TC_GP_CapInputHeightOfs */

	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_ZoomInputWidth */
	0x0F120780, /* REG_TC_PZOOM_ZoomInputHeight */
	0x0F120000, /* REG_TC_PZOOM_ZoomInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_ZoomInputHeightOfs */
	0x0F120A00, /* REG_TC_CZOOM_ZoomInputWidth */
	0x0F120780, /* REG_TC_CZOOM_ZoomInputHeight */
	0x0F120000, /* REG_TC_CZOOM_ZoomInputWidthOfs */
	0x0F120000, /* REG_TC_CZOOM_ZoomInputHeightOfs */

	0x002A0262,
	0x0F120001,	/* REG_TC_GP_bUseReqInputInPre */
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A02A6,
	0x0F120800,	 /* REG_0TC_PCFG_usWidth */
	0x0F120600,	 /* REG_0TC_PCFG_usHeight */
	0x0F120005,	 /* REG_0TC_PCFG_Format */
	0x0F12278D,	 /* REG_0TC_PCFG_usMaxOut4KHzRate */
	0x0F12278D,	 /* REG_0TC_PCFG_usMinOut4KHzRate */
	0x0F120100,	 /* REG_0TC_PCFG_OutClkPerPix88 */
	0x0F120300,	 /* REG_0TC_PCFG_uBpp88 */
	0x0F120002,	 /* REG_0TC_PCFG_PVIMask */
	0x0F120000,	 /* REG_0TC_PCFG_OIFMask */
	0x0F1201E0,	 /* REG_0TC_PCFG_usJpegPacketSize */
	0x0F120000,	 /* REG_0TC_PCFG_usJpegTotalPackets */
	0x0F120000,	 /* REG_0TC_PCFG_uClockInd */
	0x0F120000,	 /* REG_0TC_PCFG_usFrTimeType */
	0x0F120002,	 /* REG_0TC_PCFG_FrRateQualityType */
	0x0F120535,
	0x0F12028A,	 /* REG_0TC_PCFG_usMinFrTimeMsecMult10 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Preview : 1600 x 1200 */
static const u32 s5k4ec_1600_preview[] = {
	0xFCFCD000,
	0x00287000,

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs */

	0x0F120A00, /* REG_TC_GP_CapReqInputWidth */
	0x0F120780, /* REG_TC_GP_CapReqInputHeight */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs */
	0x0F12000C, /* REG_TC_GP_CapInputHeightOfs */

	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_ZoomInputWidth */
	0x0F120780, /* REG_TC_PZOOM_ZoomInputHeight */
	0x0F120000, /* REG_TC_PZOOM_ZoomInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_ZoomInputHeightOfs */
	0x0F120A00, /* REG_TC_CZOOM_ZoomInputWidth */
	0x0F120780, /* REG_TC_CZOOM_ZoomInputHeight */
	0x0F120000, /* REG_TC_CZOOM_ZoomInputWidthOfs */
	0x0F120000, /* REG_TC_CZOOM_ZoomInputHeightOfs */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A02A6,
	0x0F120640, /* REG_0TC_PCFG_usWidth */
	0x0F1204B0, /* REG_0TC_PCFG_usHeight */
	0x0F120005, /* REG_0TC_PCFG_Format */
	0x0F12278D, /* REG_0TC_PCFG_usMaxOut4KHzRate */
	0x0F12278D, /* REG_0TC_PCFG_usMinOut4KHzRate */
	0x0F120100, /* REG_0TC_PCFG_OutClkPerPix88 */
	0x0F120300, /* REG_0TC_PCFG_uBpp88 */
	0x0F120002, /* REG_0TC_PCFG_PVIMask */
	0x0F120000, /* REG_0TC_PCFG_OIFMask */
	0x0F1201E0, /* REG_0TC_PCFG_usJpegPacketSize */
	0x0F120000, /* REG_0TC_PCFG_usJpegTotalPackets */
	0x0F120000, /* REG_0TC_PCFG_uClockInd */
	0x0F120000, /* REG_0TC_PCFG_usFrTimeType */
	0x0F120002,
	0x0F120535,
	0x0F12028A, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Preview : 720 x 540 */
static const u32 s5k4ec_720_540_preview[] = {
	0xFCFCD000,

	0x00287000,
	0x002A18AC,
	0x0F120060,	/* senHal_uAddColsBin */
	0x0F120060,	/* senHal_uAddColsNoBin */
	0x0F1206C8,	/* senHal_uMinColsBin */
	0x0F1206C8,	/* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth //2560 */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight //1920 */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs //(2592-2560)/2 */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs //(1944-1920)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F1202D0, /* REG_0TC_PCFG_usWidth //720 */
	0x0F12021C, /* REG_0TC_PCFG_usHeight //540 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : 704 x 576 */
static const u32 s5k4ec_704_576_preview[] = {
	0xFCFCD000,

	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F1206C8,	/* senHal_uMinColsBin */
	0x0F1206C8,	/* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F120928, /* REG_TC_GP_PrevReqInputWidth //2344 */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight //1920 */
	0x0F12007C, /* REG_TC_GP_PrevInputWidthOfs //(2592-2344)/2 */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs //(1944-1920)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120928, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2344 */
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F1202C0, /* REG_0TC_PCFG_usWidth //704 */
	0x0F120240, /* REG_0TC_PCFG_usHeight //576 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : 720 x 480 */
static const u32 s5k4ec_720_preview[] = {
	0xFCFCD000,

	0x00287000,
	0x002A18AC,
	0x0F120060,	/* senHal_uAddColsBin */
	0x0F120060,	/* senHal_uAddColsNoBin */
	0x0F1206C8,	/* senHal_uMinColsBin */
	0x0F1206C8,	/* senHal_uMinColsNoBin */
	0x002A02BE,
	0x0F120000, /* REG_0TC_PCFG_usFrTimeType */
	0x0F120001, /* REG_0TC_PCFG_FrRateQualityType */
	0x0F12014A, /* REG_0TC_PCFG_usMaxFrTimeMsecMult10 //014Ah:30fps */
	0x0F12014A, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 //014Ah:30fps */

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth //2560 */
	0x0F1206A8, /* REG_TC_GP_PrevReqInputHeight //1704 */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs //(2592-2560)/2 */
	0x0F120078, /* REG_TC_GP_PrevInputHeightOfs //(1944-1704)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2560 */
	0x0F1206A8, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1704 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F1202D0, /* REG_0TC_PCFG_usWidth //720 */
	0x0F1201E0, /* REG_0TC_PCFG_usHeight //480 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : 720 x 720 */
static const u32 s5k4ec_720_720_preview[] = {
	0xFCFCD000,

	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F1206C8, /* senHal_uMinColsBin */
	0x0F1206C8, /* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F120780, /* REG_TC_GP_PrevReqInputWidth //1920 */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight //1920 */
	0x0F120150, /* REG_TC_GP_PrevInputWidthOfs //(2592-1920)/2 */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs //(1944-1920)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F1202D0, /* REG_0TC_PCFG_usWidth //720 */
	0x0F1202D0, /* REG_0TC_PCFG_usHeight //720 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : WVGA 800 x 480 */
static const u32 s5k4ec_wvga_preview[] = {
	0xFCFCD000,

	0x00287000,
	0x002A18AC,
	0x0F120060,	/* senHal_uAddColsBin */
	0x0F120060,	/* senHal_uAddColsNoBin */
	0x0F1206C8,	/* senHal_uMinColsBin */
	0x0F1206C8,	/* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth //2560 */
	0x0F120600, /* REG_TC_GP_PrevReqInputHeight //1536 */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs //(2592-2560)/2 */
	0x0F1200CC, /* REG_TC_GP_PrevInputHeightOfs //(1944-1536)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2560 */
	0x0F120600, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1536 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F120320, /* REG_0TC_PCFG_usWidth //800 */
	0x0F1201E0, /* REG_0TC_PCFG_usHeight //480 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : 1920 x 1080 */
static const u32 s5k4ec_1920_preview[] = {
	0xFCFCD000,
	0x00287000,

	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120200, /* senHal_uAddColsNoBin */

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth */
	0x0F120438, /* REG_TC_GP_PrevReqInputHeight */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs */
	0x0F1201A4, /* REG_TC_GP_PrevInputHeightOfs */

	0x0F120780, /* REG_TC_GP_CapReqInputWidth */
	0x0F120438, /* REG_TC_GP_CapReqInputHeight */
	0x0F120010, /* REG_TC_GP_CapInputWidthOfs */
	0x0F1201A4, /* REG_TC_GP_CapInputHeightOfs */

	0x002A0494,
	0x0F120780, /* REG_TC_PZOOM_ZoomInputWidth */
	0x0F120438, /* REG_TC_PZOOM_ZoomInputHeight */
	0x0F120130, /* REG_TC_PZOOM_ZoomInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_ZoomInputHeightOfs */
	0x0F120780, /* REG_TC_CZOOM_ZoomInputWidth */
	0x0F120438, /* REG_TC_CZOOM_ZoomInputHeight */
	0x0F120000, /* REG_TC_CZOOM_ZoomInputWidthOfs */
	0x0F120000, /* REG_TC_CZOOM_ZoomInputHeightOfs */

	0x002A0262,
	0x0F120001,	/* REG_TC_GP_bUseReqInputInPre */
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A02A6,
	0x0F120780, /* REG_0TC_PCFG_usWidth */
	0x0F120438, /* REG_0TC_PCFG_usHeight */
	0x0F120005, /* REG_0TC_PCFG_Format */
	0x0F12278D, /* REG_0TC_PCFG_usMaxOut4KHzRate */
	0x0F12278D, /* REG_0TC_PCFG_usMinOut4KHzRate */
	0x0F120100, /* REG_0TC_PCFG_OutClkPerPix88 */
	0x0F120300, /* REG_0TC_PCFG_uBpp88 */
	0x0F120002, /* REG_0TC_PCFG_PVIMask */
	0x0F120000, /* REG_0TC_PCFG_OIFMask */
	0x0F1201E0, /* REG_0TC_PCFG_usJpegPacketSize */
	0x0F120000, /* REG_0TC_PCFG_usJpegTotalPackets */
	0x0F120000, /* REG_0TC_PCFG_uClockInd */
	0x0F120000, /* REG_0TC_PCFG_usFrTimeType */
	0x0F120002, /* REG_0TC_PCFG_FrRateQualityType */
	0x0F120535,
	0x0F12028A, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};
/* Preview : W1280 1280 x 720 */
static const u32 s5k4ec_w1280_preview[] = {
	0xFCFCD000,
	0x0028D000,
	0x002AE410,
	0x0F123E01,
	0x00287000,
	0x002A18AC,

	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F1207DC, /* senHal_uMinColsBin */
	0x0F1205C0, /* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth //2560 */
	0x0F1205A0, /* REG_TC_GP_PrevReqInputHeight //1536 */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs //(2592-2560)/2 */
	0x0F1200FC, /* REG_TC_GP_PrevInputHeightOfs/(1944-1536)/2 */
	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */
	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2560 */
	0x0F1205A0, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1536 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */
	0x002A02A6,
	0x0F120500, /* REG_0TC_PCFG_usWidth //1280 */
	0x0F1202D0, /* REG_0TC_PCFG_usHeight //720 */

	0x002A0266,
	0x0F120000,
	0x002A026A,
	0x0F120001,
	0x002A024E,
	0x0F120001,
	0x002A0268,
	0x0F120001,
};

/* Preview WXGA 1280 x 768 */
static const u32 s5k4ec_1280_wxga_preview[] = {
	0xFCFCD000,

	0x0028D000,
	0x002AE410,
	0x0F123E01,
	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F120A20, /* senHal_uMinColsBin */
	0x0F120AB0, /* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth //2560 */
	0x0F120600, /* REG_TC_GP_PrevReqInputHeight //1536 */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs //(2592-2560)/2 */
	0x0F1200CC, /* REG_TC_GP_PrevInputHeightOfs //(1944-1536)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2560 */
	0x0F120600, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1536 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F120500, /* REG_0TC_PCFG_usWidth //1280 */
	0x0F120300, /* REG_0TC_PCFG_usHeight //768 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : 960 x 960 */
static const u32 s5k4ec_960_preview[] = {
	0xFCFCD000,

	0x0028D000,
	0x002AE410,
	0x0F123E01,
	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F120A20, /* senHal_uMinColsBin */
	0x0F120AB0, /* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F120780, /* REG_TC_GP_PrevReqInputWidth //1920 */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight //1920 */
	0x0F120150, /* REG_TC_GP_PrevInputWidthOfs //(2592-1920)/2 */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs //(1944-1920)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputWidth //1920 */
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F1203C0, /* REG_0TC_PCFG_usWidth //960 */
	0x0F1203C0, /* REG_0TC_PCFG_usHeight //960 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : 960 x 720 */
static const u32 s5k4ec_960_720_preview[] = {
	0xFCFCD000,

	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F1206C8, /* senHal_uMinColsBin */
	0x0F1206C8, /* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth //2560 */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight //1920 */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs //(2592-2560)/2 */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs //(1944-1920)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F1203C0, /* REG_0TC_PCFG_usWidth //960 */
	0x0F1202D0, /* REG_0TC_PCFG_usHeight //720 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : 960 x 540 */
static const u32 s5k4ec_960_540_preview[] = {
	0xFCFCD000,

	0x00287000,
	0x002A18AC,
	0x0F120060,	/* senHal_uAddColsBin */
	0x0F120060,	/* senHal_uAddColsNoBin */
	0x0F1206C8,	/* senHal_uMinColsBin */
	0x0F1206C8,	/* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth //2560 */
	0x0F1205A0, /* REG_TC_GP_PrevReqInputHeight //1440 */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs //(2592-2560)/2 */
	0x0F1200FC, /* REG_TC_GP_PrevInputHeightOfs //(1944-1440)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2560 */
	0x0F1205A0, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1440 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F1203C0, /* REG_0TC_PCFG_usWidth //960 */
	0x0F12021C, /* REG_0TC_PCFG_usHeight //480 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : 640 x 480 */
static const u32 s5k4ec_640_preview[] = {
	0xFCFCD000,

	0x00287000,
	0x002A18AC,
	0x0F120060,	/* senHal_uAddColsBin */
	0x0F120060,	/* senHal_uAddColsNoBin */
	0x0F1206C8,	/* senHal_uMinColsBin */
	0x0F1206C8,	/* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth //2560 */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight //1920 */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs //(2592-2560)/2 */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs //(1944-1920)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F120280, /* REG_0TC_PCFG_usWidth //640 */
	0x0F1201E0, /* REG_0TC_PCFG_usHeight //480 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : 880 x 720 */
static const u32 s5k4ec_880_preview[] = {
	0xFCFCD000,
	0x00287000,

	0x002A18AC,
	0x0F120060,	/* senHal_uAddColsBin */
	0x0F120060,	/* senHal_uAddColsNoBin */
	0x0F1207DC,	/* senHal_uMinColsBin */

	0x002A0250,
	0x0F1206E0, /* REG_TC_GP_PrevReqInputWidth */
	0x0F1205A0, /* REG_TC_GP_PrevReqInputHeight */
	0x0F120190, /* REG_TC_GP_PrevInputWidthOfs */
	0x0F1200F0, /* REG_TC_GP_PrevInputHeightOfs */

	0x0F1206E0, /* REG_TC_GP_CapReqInputWidth */
	0x0F1205A0, /* REG_TC_GP_CapReqInputHeight */
	0x0F120190, /* REG_TC_GP_CapInputWidthOfs */
	0x0F1200F0, /* REG_TC_GP_CapInputHeightOfs */

	0x002A0494,
	0x0F1206E0, /* REG_TC_PZOOM_ZoomInputWidth */
	0x0F1205A0, /* REG_TC_PZOOM_ZoomInputHeight */
	0x0F120000, /* REG_TC_PZOOM_ZoomInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_ZoomInputHeightOfs */
	0x0F120A00, /* REG_TC_CZOOM_ZoomInputWidth */
	0x0F1205A0, /* REG_TC_CZOOM_ZoomInputHeight */
	0x0F120000, /* REG_TC_CZOOM_ZoomInputWidthOfs */
	0x0F120000, /* REG_TC_CZOOM_ZoomInputHeightOfs */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */
	0x0F120001, /* REG_TC_GP_bUseReqInputInCap */

	0x002A0A1E,
	0x0F120028, /* AfitBaseVals_0__73_ 0040		Why?? */
	0x002A0AD4,
	0x0F12003C, /* AfitBaseVals_1__73_ 0060		Why?? */

	0x002A02A6,
	0x0F120370, /* REG_0TC_PCFG_usWidth  1280 */
	0x0F1202D0, /* REG_0TC_PCFG_usHeight 0720 */
	0x0F120005, /* REG_0TC_PCFG_Format 0005 */
	0x002A024E,
	0x0F120001, /* #REG_TC_GP_NewConfigSync */

	0x002A02A6,
	0x0F120370, /* REG_0TC_PCFG_usWidth  1280 */
	0x0F1202D0, /* REG_0TC_PCFG_usHeight 0720 */
	0x0F120005, /* REG_0TC_PCFG_Format 0005 */
	0x0F12278D, /* REG_0TC_PCFG_usMaxOut4KHzRate */
	0x0F12278D, /* REG_0TC_PCFG_usMinOut4KHzRate */
	0x0F120100, /* REG_0TC_PCFG_OutClkPerPix88 */
	0x0F120300, /* REG_0TC_PCFG_uBpp88 */
	0x0F120002, /* REG_0TC_PCFG_PVIMask */
	0x0F120000, /* REG_0TC_PCFG_OIFMask */
	0x0F1201E0, /* REG_0TC_PCFG_usJpegPacketSize */
	0x0F120000, /* REG_0TC_PCFG_usJpegTotalPackets */
	0x0F120000, /* REG_0TC_PCFG_uClockInd */
	0x0F120000, /* REG_0TC_PCFG_usFrTimeType */
	0x0F120001, /* REG_0TC_PCFG_FrRateQualityType */
	0x0F1203E8,
	0x0F12014D, /* REG_0TC_PCFG_usMinFrTimeMsecMult10 */
	0x002A02E0,
	0x0F120000, /* REG_0TC_PCFG_uPrevMirror */
	0x0F120000, /* REG_0TC_PCFG_uCaptureMirror */

	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0270,
	0x0F120001, /* REG_TC_GP_CapConfigChanged */
};

/* Preview : 1280 x 960 */
static const u32 s5k4ec_1280_preview[] = {
	0xFCFCD000,

	0x0028D000,
	0x002AE410,
	0x0F123E01,

	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F120A20, /* senHal_uMinColsBin */
	0x0F120AB0, /* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth //2560 */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight //1920 */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs //(2592-2560)/2 */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs //(1944-1920)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F120500, /* REG_0TC_PCFG_usWidth //1280 */
	0x0F1203C0, /* REG_0TC_PCFG_usHeight //960 */

	0x002A0266,
	0x0F120000,
	0x002A026A,
	0x0F120001,
	0x002A024E,
	0x0F120001,
	0x002A0268,
	0x0F120001,
};

/* Preview : 1024 x 768 */
static const u32 s5k4ec_1024_preview[] = {
	0xFCFCD000,

	0x0028D000,
	0x002AE410,
	0x0F123E01,

	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F1206C8, /* senHal_uMinColsBin */
	0x0F1206C8, /* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth //2560 */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight //1920 */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs //(2592-2560)/2 */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs //(1944-1920)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F120400, /* REG_0TC_PCFG_usWidth //1024 */
	0x0F120300, /* REG_0TC_PCFG_usHeight //768 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : 320 * 240 */
static const u32 s5k4ec_320_preview[] = {
	0xFCFCD000,

	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F1206C8, /* senHal_uMinColsBin */
	0x0F1206C8, /* senHal_uMinColsNoBin */
	0x002A02BE,
	0x0F120000, /* REG_0TC_PCFG_usFrTimeType */
	0x0F120001, /* REG_0TC_PCFG_FrRateQualityType */

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth //2560 */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight //1920 */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs //(2592-2560)/2 */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs //(1944-1920)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F120140, /* REG_0TC_PCFG_usWidth //320 */
	0x0F1200F0, /* REG_0TC_PCFG_usHeight //240 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : 352 x 288 */
static const u32 s5k4ec_352_preview[] = {
	0xFCFCD000,

	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F1206C8, /* senHal_uMinColsBin */
	0x0F1206C8, /* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F120928, /* REG_TC_GP_PrevReqInputWidth //2344 */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight //1920 */
	0x0F12007C, /* REG_TC_GP_PrevInputWidthOfs //(2592-2344)/2 */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs //(1944-1920)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120928, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2344 */
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F120160, /* REG_0TC_PCFG_usWidth //352 */
	0x0F120120, /* REG_0TC_PCFG_usHeight //288 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : QVGA 320 x 240 */
static const u32 s5k4ec_qvga_preview[] = {
	0xFCFCD000,

	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F1206C8, /* senHal_uMinColsBin */
	0x0F1206C8, /* senHal_uMinColsNoBin */
	0x002A02BE,
	0x0F120000, /* REG_0TC_PCFG_usFrTimeType */
	0x0F120001, /* REG_0TC_PCFG_FrRateQualityType */

	0x002A0250,
	0x0F120A00, /* REG_TC_GP_PrevReqInputWidth //2560 */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight //1920 */
	0x0F120010, /* REG_TC_GP_PrevInputWidthOfs //(2592-2560)/2 */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs //(1944-1920)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120A00, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2560 */
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F120140, /* REG_0TC_PCFG_usWidth //320 */
	0x0F1200F0, /* REG_0TC_PCFG_usHeight //240 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : 176 x 144 */
static const u32 s5k4ec_176_preview[] = {
	0xFCFCD000,

	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F1206C8, /* senHal_uMinColsBin */
	0x0F1206C8, /* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F120928, /* REG_TC_GP_PrevReqInputWidth //2344 */
	0x0F120780, /* REG_TC_GP_PrevReqInputHeight //1920 */
	0x0F12007C, /* REG_TC_GP_PrevInputWidthOfs //(2592-2344)/2 */
	0x0F12000C, /* REG_TC_GP_PrevInputHeightOfs //(1944-1920)/2 */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F120928, /* REG_TC_PZOOM_PrevZoomReqInputWidth //2344 */
	0x0F120780, /* REG_TC_PZOOM_PrevZoomReqInputHeight //1920 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F1200B0, /* REG_0TC_PCFG_usWidth //176 */
	0x0F120090, /* REG_0TC_PCFG_usHeight //144 */

	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : 144 x 176 */
static const u32 s5k4ec_144_176_preview[] = {
	0xFCFCD000,

	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F1206C8, /* senHal_uMinColsBin */
	0x0F1206C8, /* senHal_uMinColsNoBin */

	0x002A0250,
	0x0F1205A0, /* #REG_TC_GP_PrevReqInputWidth */
	0x0F1206E0, /* #REG_TC_GP_PrevReqInputHeight */
	0x0F120240, /* #REG_TC_GP_PrevInputWidthOfs	*/
	0x0F12005C, /* #REG_TC_GP_PrevInputHeightOfs */

	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */

	0x002A0494,
	0x0F1205A0, /* #REG_TC_PZOOM_PrevZoomReqInputWidth */
	0x0F1206E0, /* #REG_TC_PZOOM_PrevZoomReqInputHeight	*/
	0x0F120000, /* #REG_TC_PZOOM_PrevZoomReqInputWidthOfs	*/
	0x0F120000, /* #REG_TC_PZOOM_PrevZoomReqInputHeightOfs */

	0x002A02A6,
	0x0F120090, /* REG_0TC_PCFG_usWidth */
	0x0F1200B0, /* REG_0TC_PCFG_usHeight */
	0x002A0266,
	0x0F120000, /* REG_TC_GP_ActivePrevConfig */
	0x002A026A,
	0x0F120001, /* REG_TC_GP_PrevOpenAfterChange */
	0x002A024E,
	0x0F120001, /* REG_TC_GP_NewConfigSync */
	0x002A0268,
	0x0F120001, /* REG_TC_GP_PrevConfigChanged */
};

/* Preview : 1288 x 966 */
static const u32 s5k4ec_1288_966_preview[] = {
	0xFCFCD000,
	0x0028D000,
	0x002AE410,
	0x0F123E01,
	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F120A20, /* senHal_uMinColsBin */
	0x0F120AB0, /* senHal_uMinColsNoBin */
	0x002A0250,
	0x0F120A10, /* REG_TC_GP_PrevReqInputWidth //2576 */
	0x0F12078C, /* REG_TC_GP_PrevReqInputHeight //1932 */
	0x0F120008, /* REG_TC_GP_PrevInputWidthOfs //(2592-2576)/2 */
	0x0F120006, /* REG_TC_GP_PrevInputHeightOfs/(1944-1932)/2 */
	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */
	0x002A0494,
	0x0F120A10, /* REG_TC_GP_PrevReqInputWidth //2576 */
	0x0F12078C, /* REG_TC_GP_PrevReqInputHeight //1932 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */
	0x002A02A6,
	0x0F120508, /* REG_0TC_PCFG_usWidth //1288 */
	0x0F1203C6, /* REG_0TC_PCFG_usHeight //966 */

	0x002A0266,
	0x0F120000,
	0x002A026A,
	0x0F120001,
	0x002A024E,
	0x0F120001,
	0x002A0268,
	0x0F120001,
};

/* Preview : 1288 x 772 */
static const u32 s5k4ec_1288_772_preview[] = {
	0xFCFCD000,
	0x0028D000,
	0x002AE410,
	0x0F123E01,
	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F120A20, /* senHal_uMinColsBin */
	0x0F120AB0, /* senHal_uMinColsNoBin */
	0x002A0250,
	0x0F120A10, /* REG_TC_GP_PrevReqInputWidth //2576 */
	0x0F120608, /* REG_TC_GP_PrevReqInputHeight //1544 */
	0x0F120008, /* REG_TC_GP_PrevInputWidthOfs //(2592-2576)/2 */
	0x0F1200C8, /* REG_TC_GP_PrevInputHeightOfs/(1944-1544/2 */
	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */
	0x002A0494,
	0x0F120A10, /* REG_TC_GP_PrevReqInputWidth //2576 */
	0x0F120608, /* REG_TC_GP_PrevReqInputHeight //1544 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */
	0x002A02A6,
	0x0F120508, /* REG_0TC_PCFG_usWidth //1288 */
	0x0F120304, /* REG_0TC_PCFG_usHeight //772 */

	0x002A0266,
	0x0F120000,
	0x002A026A,
	0x0F120001,
	0x002A024E,
	0x0F120001,
	0x002A0268,
	0x0F120001,
};

/* Preview : 964 x 964 */
static const u32 s5k4ec_964_964_preview[] = {
	0xFCFCD000,
	0x0028D000,
	0x002AE410,
	0x0F123E01,
	0x00287000,
	0x002A18AC,
	0x0F120060, /* senHal_uAddColsBin */
	0x0F120060, /* senHal_uAddColsNoBin */
	0x0F120A20, /* senHal_uMinColsBin */
	0x0F120AB0, /* senHal_uMinColsNoBin */
	0x002A0250,
	0x0F120788, /* REG_TC_GP_PrevReqInputWidth //1928 */
	0x0F120788, /* REG_TC_GP_PrevReqInputHeight //1928 */
	0x0F12014C, /* REG_TC_GP_PrevInputWidthOfs //(2592-1928)/2 */
	0x0F120008, /* REG_TC_GP_PrevInputHeightOfs/(1944-1928)/2 */
	0x002A0262,
	0x0F120001, /* REG_TC_GP_bUseReqInputInPre */
	0x002A0494,
	0x0F120788, /* REG_TC_GP_PrevReqInputWidth //1928 */
	0x0F120788, /* REG_TC_GP_PrevReqInputHeight //1928 */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputWidthOfs */
	0x0F120000, /* REG_TC_PZOOM_PrevZoomReqInputHeightOfs */
	0x002A02A6,
	0x0F1203C4, /* REG_0TC_PCFG_usWidth //964 */
	0x0F1203C4, /* REG_0TC_PCFG_usHeight //964 */

	0x002A0266,
	0x0F120000,
	0x002A026A,
	0x0F120001,
	0x002A024E,
	0x0F120001,
	0x002A0268,
	0x0F120001,
};

static const u32 s5k4ec_ae_awb_lock_on[] = {
	/* AE Lock */
	0xFCFCD000,
	0x00287000,
	0x002A2C5E,
	0x0F120000,

	/* AWB Lock */
	0xFCFCD000,
	0x00287000,
	0x002A2C66,
	0x0F120000,
};

static const u32 s5k4ec_ae_lock_on_awb_lock_off[] = {
	/* AE Lock */
	0xFCFCD000,
	0x00287000,
	0x002A2C5E,
	0x0F120000,

	/* AWB Unlock */
	0xFCFCD000,
	0x00287000,
	0x002A2C66,
	0x0F120001,
};

static const u32 s5k4ec_ae_lock_off_awb_lock_on[] = {
	/* AE Unlock */
	0xFCFCD000,
	0x00287000,
	0x002A2C5E,
	0x0F120001,

	/* AWB Lock */
	0xFCFCD000,
	0x00287000,
	0x002A2C66,
	0x0F120000,
};

static const u32 s5k4ec_ae_awb_lock_off[] = {
	/* AE Unlock */
	0xFCFCD000,
	0x00287000,
	0x002A2C5E,
	0x0F120001,

	/* AWB Unlock */
	0xFCFCD000,
	0x00287000,
	0x002A2C66,
	0x0F120001,
};

static const u32 s5k4ec_ae_lock_on[] = {
	/* AE Lock */
	0xFCFCD000,
	0x00287000,
	0x002A2C5E,
	0x0F120000,
};

static const u32 s5k4ec_awb_lock_on[] = {
	/* AWB Lock */
	0xFCFCD000,
	0x00287000,
	0x002A2C66,
	0x0F120000,
};

static const u32 s5k4ec_ae_lock_off[] = {
	/* AE Unlock */
	0xFCFCD000,
	0x00287000,
	0x002A2C5E,
	0x0F120001,
};

static const u32 s5k4ec_awb_lock_off[] = {
	/* AWB Unlock */
	0xFCFCD000,
	0x00287000,
	0x002A2C66,
	0x0F120001,
};

static const u32 s5k4ec_get_ae_stable_status[] = {
	0x002E2C74,
};

static const u32 s5k4ec_get_light_level[] = {
	0x002C7000,
	0x002E2C18,
};

static const u32 s5k4ec_get_frame_duration_reg[] = {
	0x002C7000,
	0x002E2128,
};

static const u32 s5k4ec_get_1st_af_search_status[] = {
	0x002E2EEE,
};

static const u32 s5k4ec_get_2nd_af_search_status[] = {
	0x002E2207,
};

static const u32 s5k4ec_get_capture_status[] = {
	0x002E0530,
};

static const u32 s5k4ec_get_esd_status[] = {
	0xFCFCD000,
	0x002CD000,
	0x002E0060,
};

static const u32 s5k4ec_get_iso_reg[] = {
	0x002C7000,
	0x002E2BC4,
};

static const u32 s5k4ec_get_shutterspeed_reg[] = {
	0x002C7000,
	0x002E2134,
};

static const u32 s5k4ec_get_exptime_reg[] = {
	0x002C7000,
	0x002E2BC0,
};

static const u32 s5k4ec_get_frame_count_reg[] = {
	0x002C7000,
	0x002E1F6C,
};

static const u32 s5k4ec_get_preview_status_reg[] = {
	0x002C7000,
	0x002E0240,
};

static const u32 s5k4ec_get_pid_reg[] = {
	0x002C7000,
	0x002E01A4,
};

static const u32 s5k4ec_get_revision_reg[] = {
	0x002C7000,
	0x002E01A6,
};

static const u32 s5k4ec_get_modechange_check_reg[] = {
	0x002C7000,
	0x002E215F,
};

static const u32 s5k4ec_set_vendor_id_read_reg[] = {
	0xFCFCD000,
	0x0028D000,
	0x002A0012,
	0x0F120001,
	0x002A007A,
	0x0F120000,
	0x002AA000,
	0x0F120004,
	0x002AA002,
	0x0F120006,
	0x002AA000,
	0x0F120001,
};

static const u32 s5k4ec_get_vendor_id_read_reg[] = {
	0x002CD000,
	0x002EA006,
};

#endif

