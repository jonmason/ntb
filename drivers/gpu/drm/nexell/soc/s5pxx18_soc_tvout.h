#ifndef _S5PXX18_SOC_TVOUT_H_
#define	_S5PXX18_SOC_TVOUT_H_

#include "s5pxx18_soc_disp.h"

#define	IRQ_OFFSET	32
#define IRQ_DPC_P	(IRQ_OFFSET + 33)
#define IRQ_DPC_S   (IRQ_OFFSET + 34)

#define NUMBER_OF_DPC_MODULE	2
#define PHY_BASEADDR_DPC0	0xC0102800
#define PHY_BASEADDR_DPC1	0xC0102C00

#define	PHY_BASEADDR_DPC_LIST	\
		{ PHY_BASEADDR_DPC0, PHY_BASEADDR_DPC1 }

struct nx_dpc_register_set {
	u32 ntsc_stata;
	u32 ntsc_ecmda;
	u32 ntsc_ecmdb;
	u32 ntsc_glk;
	u32 ntsc_sch;
	u32 ntsc_hue;
	u32 ntsc_sat;
	u32 ntsc_cont;
	u32 ntsc_bright;
	u32 ntsc_fsc_adjh;
	u32 ntsc_fsc_adjl;
	u32 ntsc_ecmdc;
	u32 ntsc_csdly;
	u32 __ntsc_reserved_0_[3];
	u32 ntsc_dacsel10;
	u32 ntsc_dacsel32;
	u32 ntsc_dacsel54;
	u32 ntsc_daclp;
	u32 ntsc_dacpd;
	u32 __ntsc_reserved_1_[(0x20 - 0x15)];
	u32 ntsc_icntl;
	u32 ntsc_hvoffst;
	u32 ntsc_hoffst;
	u32 ntsc_voffset;
	u32 ntsc_hsvso;
	u32 ntsc_hsob;
	u32 ntsc_hsoe;
	u32 ntsc_vsob;
	u32 ntsc_vsoe;
	u32 __reserved[(0xf8 / 4) - 0x29];
	u32 dpchtotal;
	u32 dpchswidth;
	u32 dpchastart;
	u32 dpchaend;
	u32 dpcvtotal;
	u32 dpcvswidth;
	u32 dpcvastart;
	u32 dpcvaend;
	u32 dpcctrl0;
	u32 dpcctrl1;
	u32 dpcevtotal;
	u32 dpcevswidth;
	u32 dpcevastart;
	u32 dpcevaend;
	u32 dpcctrl2;
	u32 dpcvseoffset;
	u32 dpcvssoffset;
	u32 dpcevseoffset;
	u32 dpcevssoffset;
	u32 dpcdelay0;
	u32 dpcupscalecon0;
	u32 dpcupscalecon1;
	u32 dpcupscalecon2;

	u32 dpcrnumgencon0;
	u32 dpcrnumgencon1;
	u32 dpcrnumgencon2;
	u32 dpcrndconformula_l;
	u32 dpcrndconformula_h;
	u32 dpcfdtaddr;
	u32 dpcfrdithervalue;
	u32 dpcfgdithervalue;
	u32 dpcfbdithervalue;
	u32 dpcdelay1;
	u32 dpcmputime0;
	u32 dpcmputime1;
	u32 dpcmpuwrdatal;
	u32 dpcmpuindex;
	u32 dpcmpustatus;
	u32 dpcmpudatah;
	u32 dpcmpurdatal;
	u32 dpcdummy12;
	u32 dpccmdbufferdatal;
	u32 dpccmdbufferdatah;
	u32 dpcpolctrl;
	u32 dpcpadposition[8];
	u32 dpcrgbmask[2];
	u32 dpcrgbshift;
	u32 dpcdataflush;
	u32 __reserved06[((0x3c0) - (2 * 0x0ec)) / 4];

	u32 dpcclkenb;
	u32 dpcclkgen[2][2];
};

enum {
	nx_dpc_int_vsync = 0
};

enum nx_dpc_format {
	nx_dpc_format_rgb555 = 0ul,
	nx_dpc_format_rgb565 = 1ul,
	nx_dpc_format_rgb666 = 2ul,
	nx_dpc_format_rgb666b = 18ul,
	nx_dpc_format_rgb888 = 3ul,
	nx_dpc_format_mrgb555a = 4ul,
	nx_dpc_format_mrgb555b = 5ul,
	nx_dpc_format_mrgb565 = 6ul,
	nx_dpc_format_mrgb666 = 7ul,
	nx_dpc_format_mrgb888a = 8ul,
	nx_dpc_format_mrgb888b = 9ul,
	nx_dpc_format_ccir656 = 10ul,
	nx_dpc_format_ccir601a = 12ul,
	nx_dpc_format_ccir601b = 13ul,
	nx_dpc_format_srgb888 = 14ul,
	nx_dpc_format_srgbd8888 = 15ul,
	nx_dpc_format_4096color = 1ul,
	nx_dpc_format_16gray = 3ul
};

enum nx_dpc_ycorder {
	nx_dpc_ycorder_cb_ycr_y = 0ul,
	nx_dpc_ycorder_cr_ycb_y = 1ul,
	nx_dpc_ycorder_ycbycr = 2ul,
	nx_dpc_ycorder_ycrycb = 3ul
};

enum nx_dpc_padclk {
	nx_dpc_padclk_vclk = 0ul,
	nx_dpc_padclk_vclk2 = 1ul,
	nx_dpc_padclk_vclk3 = 2ul
};

enum nx_dpc_dither {
	nx_dpc_dither_bypass = 0ul,
	nx_dpc_dither_4bit = 1ul,
	nx_dpc_dither_5bit = 2ul,
	nx_dpc_dither_6bit = 3ul
};

enum nx_dpc_vbs {
	nx_dpc_vbs_ntsc_m = 0ul,
	nx_dpc_vbs_ntsc_n = 1ul,
	nx_dpc_vbs_ntsc_443 = 2ul,
	nx_dpc_vbs_pal_m = 3ul,
	nx_dpc_vbs_pal_n = 4ul,
	nx_dpc_vbs_pal_bghi = 5ul,
	nx_dpc_vbs_pseudo_pal = 6ul,
	nx_dpc_vbs_pseudo_ntsc = 7ul
};

enum nx_dpc_bandwidth {
	nx_dpc_bandwidth_low = 0ul,
	nx_dpc_bandwidth_medium = 1ul,
	nx_dpc_bandwidth_high = 2ul
};

void nx_dpc_set_enable_with_interlace(u32 module_index, int enable, int rgbmode,
	       int use_ntscsync, int use_analog_output, int seavenable);

#endif

