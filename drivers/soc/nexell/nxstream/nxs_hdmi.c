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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>

#include <linux/of.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/regmap.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_res_manager.h>

#include <video/videomode.h>
#include <video/of_display_timing.h>

#define	CONFIG_HDMI20_CTRL_SUPPORT

#include "nxs_hdmi.h"

#define	HDMI_IRQ_UPDATE	(1<<0)
#define	HDMI_IRQ_HPD	(1<<1)

#define HDCP_1P4_KEY_SIZE	288 /* 0x120 (HDCP 1.4) */
#define HDCP_2P2_KEY_SIZE	320 /* 0x140 (HDCP 1.4 and HDCP 2.2) */

#define	MAX_RESOURCE_NUM	6

#define	DBG(h, m...)	dev_dbg((const struct device *)h->nxs_dev.dev, m)
#define	ERR(h, m...)	dev_err((const struct device *)h->nxs_dev.dev, m)

struct nxs_hdmi {
	struct nxs_dev nxs_dev;
	/* resources */
	struct clk *clks[MAX_RESOURCE_NUM];
	int num_clks;
	struct reset_control *resets[MAX_RESOURCE_NUM];
	int num_resets;

	/* phy config */
	const struct hdmi_phy_conf **phy_confs_list;
	int phy_confs_num;
	const struct hdmi_phy_conf *phy_conf;
	const struct hdmi_preferred_conf *preferred;
	int preferred_num;

	/* hdmi control */
	struct nxs_control_display  display;
	struct videomode vm;

	/* hdmi video/audio info */
	bool pattern;
	bool dvi_mode;
	int pixelbits;	/* 8, 10bits */
	enum hdmi_tmds_bitclk_ratio tmds_clk_ratio;
	struct hdmi_video_info video;
	struct hdmi_audio_info audio;
	bool audio_enabled;

	/* registers */
	void __iomem *reg_hdmi;	/* nxs_to_hdmi */
	void __iomem *reg_link;
	void __iomem *reg_phy;
	void __iomem *reg_cec;
	void __iomem *reg_tieoff;
	struct hdmi_clock *clocks;

	/* interrupts */
	int irq;	/* for syncgen */
	int hpd_irq;	/* for hpd */
	bool hpd_irq_enabled;
};
#define nxs_to_hdmi(dev)	container_of(dev, struct nxs_hdmi, nxs_dev)

#define __FNS(s)	nxs_function_to_str(s->dev_function)
#define __FNI(s)	s->dev_inst_index

#define	_BM(n)	((n << 1) - 1)
static inline void __write_bitw(void __iomem *addr,
			int val, int bitpos, int bitsize)
{
	val  = readl(addr) & ~(_BM(bitsize) << bitpos);
	val |= (val & _BM(bitsize)) << bitpos;
	writel(val, addr);
}

static inline u32 __read_bitw(void __iomem *addr, int bitpos, int bitsize)
{
	u32 val = readl(addr) & (_BM(bitsize) << bitpos);

	return (val >> bitpos) & _BM(bitsize);
}

static inline void hdmi_set_bit(void __iomem *addr, u32 offs,
			u32 val, int bitpos)
{
	__write_bitw((addr + offs), val, bitpos, 1);
}

static inline void hdmi_set_bitw(void __iomem *addr, u32 offs,
			u32 val, int bitpos, int bitsize)
{
	__write_bitw((addr + offs), val, bitpos, bitsize);
}

static inline u32 hdmi_get_bit(void __iomem *addr, u32 offs, int bitpos)
{
	return __read_bitw((addr + offs), bitpos, 1);
}

static inline void hdmi_writel(void __iomem *addr, u32 offs, u32 val)
{
	writel(val, (addr + offs));
}

static inline u32 hdmi_readl(void __iomem *addr, u32 offs)
{
	return readl(addr + offs);
}

#include "nxs_hdmi_clk.c"


/* video modes (VIC) with pixel repetition */
static const int __vic_pixel_repetition[] = {
	6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	35, 36, 37, 38, 44, 45, 50, 51, 54, 55,
	58, 59
};

static void hdmiphy_v2_mode_set(void __iomem *phy, bool en)
{
	hdmi_set_bitw(phy, HDMI_PHY_MODE_SET, en ? 1 : 0, 7, 1);
}

static void hdmiphy_v2_pll_conf(void __iomem *phy,
			const char *data, size_t size)
{
	int i;

	if (WARN_ON(!data))
		return;

	for (i = 0; i < size; i++)
		writel(data, phy + HDMI_PHY_REG04 + (i * 4));
}

static int hdmiphy_v2_tmds_ratio(void __iomem *phy,
			enum hdmi_tmds_bitclk_ratio ratio)
{
	switch (ratio) {
	case HDMI_TMDS_BIT_CLOCK_RATIO_1_10:
		writel(0x00, phy + HDMI_PHY_REG74);
		writel(0x3E, phy + HDMI_PHY_REG78);
		break;
	case HDMI_TMDS_BIT_CLOCK_RATIO_1_40:
		/* writel(0x04, phy + HDMI_PHY_REG74); */
		writel(0x08, phy + HDMI_PHY_REG74);
		writel(0x00, phy + HDMI_PHY_REG78);
		break;
	default:
		pr_err("%s: Failed unknown TMDS bit/clock ratio(%d)\n",
			__func__, ratio);
		return -EINVAL;
	}
	return 0;
}

#ifdef CONFIG_HDMI20_TMDS_BIT_CLOCK_RATIO
static void hdmiphy_v2_output(void __iomem *phy, bool on)
{
	int val = on ? 0 : 1;

	hdmi_set_bitw(phy, HDMI_PHY_PD_EN, 1, 1, 1);
	hdmi_set_bitw(phy, HDMI_PHY_DRV_CLK, val, 6, 1);
	hdmi_set_bitw(phy, HDMI_PHY_DRV_DATA, val, 7, 1);
	hdmi_set_bitw(phy, HDMI_PHY_PD_EN, 0, 1, 1);
}
#endif

static int hdmilink_v2_sync_pol(void __iomem *link,
			int hsync_pol, int vsync_pol)
{
	pr_debug("%s: hp[%d], vp[%d]\n", __func__, hsync_pol, vsync_pol);

	/* set HSYNC and VSYNC polarity */
	hdmi_set_bit(link, HDMI_HSYNC_POL, hsync_pol ? 1 : 0, 8);
	hdmi_set_bit(link, HDMI_VSYNC_POL, vsync_pol ? 1 : 0, 9);

	return 0;
}

static void hdmilink_v2_hpd_on(void __iomem *link, bool on)
{
	pr_info("HPD [%s]\n", on ? "ON" : "OFF");

	if (!on)
		return;

	hdmi_set_bitw(link, HDMI_HPD_EN, 1, 0, 1);
}

static inline bool hdmilink_v2_connected(void __iomem *link)
{
	bool plug;

	plug = hdmi_get_bit(link, HDMI_HPD, 6) ? true : false;
	pr_debug("%s: %s\n", __func__, plug ? "connected" : "disconnected");

	return plug;
}

static void hdmilink_v2_hpd_irq_done(struct nxs_hdmi *hdmi)
{
	void __iomem *link = hdmi->reg_link;
	bool plug;

	plug = hdmilink_v2_connected(link);

	/* clear interrupt */
	hdmi_set_bitw(link, HDMI_HPD_INTPEND, 1, 0, 1);
}

static inline bool hdmilink_v2_phy_ready(void __iomem *link)
{
	int ret = hdmi_get_bit(link, HDMI_PHY_READY, 1);

	return ret ? true : false;
}

static int hdmilink_v2_pattern(void __iomem *link,
			struct videomode *vm,
			enum hdmi_video_format format,
			enum hdmi_3d_structure struct_3d,
			int hsync_pol, int vsync_pol, bool on)
{
	bool interlaced = vm->flags & DISPLAY_FLAGS_INTERLACED;

	int has = vm->hactive, hsw = vm->hsync_len;
	int hfp = vm->hfront_porch, hbp = vm->hback_porch;
	int vas = vm->vactive, vsw = vm->vsync_len;
	int vfp = vm->vfront_porch, vbp = vm->vback_porch;

	pr_debug("%s: %s %s (3D:%d)\n", __func__, on ? "PATTERN" : "PASS",
		format == HDMI_VIDEO_FORMAT_3D ? "3D" : "2D", struct_3d);

	if (!on) {
		hdmi_set_bit(link, HDMI_INTERNAL_PTTN, 0, 4);
		return 0;
	}

	if (interlaced) {
#ifdef CONFIG_HDMI20_JF_EVT1
		u32 v_off = (has + hfp + hsw + hbp) / 4;

		/*
		 * Voff = Htotal / 2; Htotal = Hactive +
		 * Hblank = Hactive + Hfront + Hsync + Hback
		 */
		hdmi_writel(link, HDMI_TEST_PTTN_VOFF, v_off);
		hdmi_writel(link, HDMI_TEST_PTTN_VFP, vfp);
		hdmi_writel(link, HDMI_TEST_PTTN_VS, vsw);
		hdmi_writel(link, HDMI_TEST_PTTN_VBP, vbp);

		if (format == HDMI_VIDEO_FORMAT_3D &&
		struct_3d == HDMI_3D_STRUCTURE_FRAME_PACKING)
			hdmi_writel(link, HDMI_TEST_PTTN_VLINE,
				((vas * 2) + ((vfp + vsw + vbp) * 3) + 2));
		else
			hdmi_writel(link, HDMI_TEST_PTTN_VLINE, (vas / 2));

		hdmi_writel(link, HDMI_TEST_PTTN_DE, (has / 2));
		hdmi_writel(link, HDMI_TEST_PTTN_HBP, (hbp / 2));
		hdmi_writel(link, HDMI_TEST_PTTN_HS, (hsw / 2));
		hdmi_writel(link, HDMI_TEST_PTTN_HFP, (hfp / 2));
#endif
	} else {
		hdmi_writel(link, HDMI_TEST_PTTN_VFP, vfp);
		hdmi_writel(link, HDMI_TEST_PTTN_VS, vsw);
		hdmi_writel(link, HDMI_TEST_PTTN_VBP, vbp);
		if (format == HDMI_VIDEO_FORMAT_3D &&
		struct_3d == HDMI_3D_STRUCTURE_FRAME_PACKING) {
			hdmi_writel(link, HDMI_TEST_PTTN_VLINE,
				(vas * 2) + vfp + vsw + vbp);
		} else {
			hdmi_writel(link, HDMI_TEST_PTTN_VLINE, vas);
		}
		hdmi_writel(link, HDMI_TEST_PTTN_DE, (has / 2));
		hdmi_writel(link, HDMI_TEST_PTTN_HBP, (hbp / 2));
		hdmi_writel(link, HDMI_TEST_PTTN_HS, (hsw / 2));
		hdmi_writel(link, HDMI_TEST_PTTN_HFP, (hfp / 2));
	}

#ifdef CONFIG_HDMI20_JF_EVT0
	/* set PORT */
	hdmi_set_bitw(link, HDMI_TEST_PTTN_PORT, 1, 12, 4);
#endif

#ifdef CONFIG_HDMI20_JF_EVT1
	/* set progressive/interlaced mode */
	if (interlaced &&
	format == HDMI_VIDEO_FORMAT_3D &&
	struct_3d != HDMI_3D_STRUCTURE_FRAME_PACKING)
		hdmi_set_bit(link, HDMI_TEST_PTTN_INTERLACED, 1, 10);
	else
		hdmi_set_bit(link, HDMI_TEST_PTTN_INTERLACED, 0, 10);
#endif

	/* set pattern type */
	hdmi_set_bitw(link, HDMI_TEST_PTTN_TYPE, HDMI20_PTTN_TYPE_USER, 0, 4);

	/* set RGB data for HDMI20_PTTN_TYPE_USER */
	/* GREEN color, limited range, 24bpp */
	hdmi_writel(link, HDMI_TEST_PTTN_USER0_B, 16 << 4);
	hdmi_writel(link, HDMI_TEST_PTTN_USER0_G, 204 << 4);
	hdmi_writel(link, HDMI_TEST_PTTN_USER0_R, 16 << 4);
	hdmi_writel(link, HDMI_TEST_PTTN_USER1_B, 16 << 4);
	hdmi_writel(link, HDMI_TEST_PTTN_USER1_G, 204 << 4);
	hdmi_writel(link, HDMI_TEST_PTTN_USER1_R, 16 << 4);

	hdmi_set_bit(link, HDMI_TEST_PTTN_POL_HS, hsync_pol ? 1 : 0, 9);
	hdmi_set_bit(link, HDMI_TEST_PTTN_POL_VS, vsync_pol ? 1 : 0, 8);

	/* run internal pattern generator */
	hdmi_set_bit(link, HDMI_TEST_PTTN_RUN, 1, 0);

	/* switch to internal pattern generator */
	hdmi_set_bit(link, HDMI_INTERNAL_PTTN, 0, 4);

	pr_debug("%s: SCAN %s\n",
		__func__, interlaced ? "Interlaced" : "Progessive");
	pr_debug("%s: HSYNC haw:%4d, hfp:%4d, hbp:%4d, hsw:%4d, pol:%d\n",
		__func__, has, hfp, hbp, hsw, hsync_pol);
	pr_debug("%s: VSYNC vaw:%4d, vfp:%4d, vb:%4d, vsw:%4d, pol:%d\n",
		__func__, vas, vfp, vbp, vsw, vsync_pol);

	return 0;
}

static void hdmilink_v2_checksum(struct __hdmi_infoframe *infoframe)
{
	int length;
	u8 sum = 0;
	int i;

	length = (infoframe->hb2 & HDMI20_INFOFRAME_HB2_LEN_MASK);

	/* packet header */
	for (i = 0; i < HDMI_PACKET_HEADER_LENGTH; i++)
		sum += infoframe->hb[i];

	/* packet contents */
	for (i = 0; i < length; i++)
		sum += infoframe->pb[i + HDMI_PACKET_CHECKSUM_LENGTH];

	infoframe->pb00 = 0x00 - sum;
}


static void hdmilink_v2_write_infoframe(void __iomem *link,
			u32 offset, struct __hdmi_infoframe *infoframe)
{
	u32 byte, reg = 0;
	int i = 0;

	hdmilink_v2_checksum(infoframe);

	/* __hdmi_infoframe Packet Header */
	reg |= infoframe->hb[0];
	reg |= infoframe->hb[1] << 8;
	reg |= infoframe->hb[2] << 16;

	hdmi_writel(link, offset, reg);

	do {
		byte = 0, reg = 0;
		do {
			reg |= infoframe->pb[i] << (byte * 8);
			byte++,	i++;
		} while (byte < 4);

		hdmi_writel(link, offset + i, reg);
	} while (i <= HDMI_PACKET_LENGTH_MAX);
}

#ifdef CONFIG_HDMI20_CTRL_SUPPORT
static u8 hdmilink_v2_get_vic(
			struct videomode *vm, enum hdmi_picture_aspect ratio)
{
	return 0xFF;	/* not found */
}
#endif

static void hdmilink_v2_avi_infoframe(void __iomem *link,
			struct videomode *vm, struct hdmi_video_info *video)
{
	struct __hdmi_infoframe infoframe;
	enum hdmi_pixel_repetition pixel_rep = HDMI_REPET_NONE;
	const enum hdmi_picture_aspect aspect_ratio = video->aspect_ratio;
	enum hdmi_colorspace colorspace = video->colorspace;
	enum hdmi_video_format format = video->format;
	enum hdmi_colorimetry c_metry = video->colorimetry;
	enum hdmi_extended_colorimetry c_metry_ext = video->ext_colorimetry;
	enum hdmi_quantization_range q_range = video->q_range;
	int color_form = 0;
	int i;

	pr_debug("%s: enter\n", __func__);

	memset(&infoframe, 0, sizeof(struct __hdmi_infoframe));

	/* Packet Header */
	infoframe.hb0 = HDMI_INFOFRAME_TYPE_AVI;
	infoframe.hb1 = HDMI_AVI_VERSION;
	infoframe.hb2 =
		SET_MASK(HDMI_AVI_LENGTH, HDMI20_INFOFRAME_HB2_LEN_MASK);

	/* Color format (Y0, Y1, Y2), follow CEA format set */
	switch (colorspace) {
	case HDMI_COLORSPACE_RGB:
		color_form = 0;
		break;
	case HDMI_COLORSPACE_YUV444:
		color_form = 2;
		break;
	case HDMI_COLORSPACE_YUV422:
		color_form = 1;
		break;
	case HDMI_COLORSPACE_YUV420:
		color_form = 3;
		break;
	default:
		break;
	}

	infoframe.pb01 |=
		SET_MASK(color_form, HDMI20_AVI_PB1_CS_FMT_MASK);

	/* No Active Format Information (A0) */
	if (colorspace == HDMI_COLORSPACE_RGB) {
		infoframe.pb02 |= SET_MASK(c_metry, HDMI20_AVI_PB2_CM_MASK);
		infoframe.pb03 |= SET_MASK(q_range, HDMI20_AVI_PB3_QT_MASK);
	} else {
		infoframe.pb02 |= SET_MASK(c_metry, HDMI20_AVI_PB2_CM_MASK);
		infoframe.pb03 |= SET_MASK(c_metry_ext, HDMI20_AVI_PB3_EC_MASK);
		if (format != HDMI_VIDEO_FORMAT_UD)
			infoframe.pb05 |=
				SET_MASK(q_range, HDMI20_AVI_PB5_QR_MASK);
	}

	if (format != HDMI_VIDEO_FORMAT_UD) {
		u8 vic;

#ifdef CONFIG_HDMI20_CTRL_SUPPORT
		vic = hdmilink_v2_get_vic(vm, aspect_ratio);
#endif
		pr_debug("%s: vic = %d\n", __func__, vic);

		infoframe.pb02 |= SET_MASK(aspect_ratio,
			HDMI20_AVI_PB2_ASPECT_RATIO_MASK);
		infoframe.pb04 |= SET_MASK(vic, HDMI20_AVI_PB4_VIC_MASK);
		/* Pixel Repetition */
		for (i = 0; i < ARRAY_SIZE(__vic_pixel_repetition); i++) {
			if (__vic_pixel_repetition[i] == vic) {
				pixel_rep = HDMI_REPET_PIXEL_SENT_2TIMES;
				break;
			}
		}
		infoframe.pb05 |= SET_MASK(pixel_rep, HDMI20_AVI_PB5_PR_MASK);
	} else {
		/* extended video format (HDMI_VIC) */
	#if 0
		/* Picture Aspect Ratio */
		infoframe.pb02 |= SET_MASK(HDMI_PICTURE_ASPECT_NONE,
			HDMI20_AVI_PB2_ASPECT_RATIO_MASK);
		/* Video Format Identification Code VIC0...VIC7 */
		/* set the AVI __hdmi_infoframe VIC field to zero */
		infoframe.pb04 |= SET_MASK(0, HDMI20_AVI_PB4_VIC_MASK);
		/* Pixel Repetition */
		infoframe.pb05 |= SET_MASK(HDMI_REPET_NONE,
			HDMI20_AVI_PB5_PR_MASK);
	#endif
	}

	/* write AVI packet */
	hdmilink_v2_write_infoframe(link, HDMI_AVI_CTRL, &infoframe);

	/* AVI packet enable */
	hdmi_set_bit(link, HDMI_AVI_EN, 1, 12);
}

static void hdmilink_v2_vsi_infoframe(void __iomem *link,
			struct videomode *vm, struct hdmi_video_info *video)
{
	struct __hdmi_infoframe infoframe;
	enum hdmi_video_format format = video->format;
	enum hdmi_3d_structure struct_3d = video->struct_3d;
	u8 vic;

	pr_debug("%s: enter\n", __func__);

	memset(&infoframe, 0, sizeof(struct __hdmi_infoframe));

	/* disable VSI packets */
	if (format == HDMI_VIDEO_FORMAT_2D) {
		hdmi_set_bit(link, HDMI_VSI_EN, 0, 11);
		return;
	}

#ifdef CONFIG_HDMI20_CTRL_SUPPORT
	vic = hdmilink_v2_get_vic(vm, HDMI_PICTURE_ASPECT_NONE);
#endif

	/* Packet Header */
	infoframe.hb0 = HDMI_INFOFRAME_TYPE_VENDOR;
	infoframe.hb1 = HDMI_VSI_VERSION;

	/* 24bit IEEE Organizationally Unique Identifier (0x000C03) */
	infoframe.pb01 = HDMI_IEEE_OUI & 0xff;
	infoframe.pb02 = (HDMI_IEEE_OUI >> 8) & 0xff;
	infoframe.pb03 = (HDMI_IEEE_OUI >> 16) & 0xff;

	/* HDMI_Video_Format */
	infoframe.pb04 |= SET_MASK(format, HDMI20_VSI_H14B_PB4_VIDEO_FMT_MASK);

	if (format == HDMI_VIDEO_FORMAT_3D) {
		/* 3D_Structure */
		infoframe.pb05 |=
			SET_MASK(struct_3d, HDMI20_VSI_H14B_PB5_3D_MASK);
	} else if (format == HDMI_VIDEO_FORMAT_UD) {
		/* HDMI_VIC */
		infoframe.pb05 |=
			SET_MASK(vic, HDMI20_VSI_H14B_PB5_VIC_MASK);
	} else {
		pr_err("%s: Failed unknown HDMI video format (0x%x)\n",
			__func__, format);
		return;
	}

	/* 3D_Ext_Data and Length */
	if (struct_3d == HDMI_3D_STRUCTURE_SIDE_BY_SIDE_HALF) {
		infoframe.pb06 |= SET_MASK(HDMI_H_SUB_SAMPLE,
			HDMI20_VSI_H14B_PB6_3D_EXT_MASK);
		infoframe.hb2 = SET_MASK(HDMI_H14B_VSI_LENGTH + 1,
			HDMI20_INFOFRAME_HB2_LEN_MASK);
	} else {
		infoframe.hb2 = SET_MASK(HDMI_H14B_VSI_LENGTH,
			HDMI20_INFOFRAME_HB2_LEN_MASK);
	}

	/* write Vendor-Specific infoframe packet */
	hdmilink_v2_write_infoframe(link, HDMI_VSI_CTRL, &infoframe);

	/* enable VSI packets */
	hdmi_set_bit(link, HDMI_VSI_EN, 1, 11);
}

static void hdmilink_v2_gcp(void __iomem *link, struct hdmi_video_info *video)
{
#ifdef CONFIG_HDMI20_CTRL_SUPPORT
	enum hdmi_color_depth depth = video->color_depth;

	switch (depth) {
	case HDMI_COLOR_DEPTH_24BPP:
		hdmi_set_bitw(link, HDMI_DEEP_COLOR_MODE, 0, 0, 2);
		hdmi_set_bitw(link, HDMI_GCP_CD, 0x4, 8, 4);
		break;
	case HDMI_COLOR_DEPTH_30BPP:
		hdmi_set_bitw(link, HDMI_DEEP_COLOR_MODE, 1, 0, 2);
		hdmi_set_bitw(link, HDMI_GCP_CD, 0x5, 8, 4);
		break;
	case HDMI_COLOR_DEPTH_36BPP:
		hdmi_set_bitw(link, HDMI_DEEP_COLOR_MODE, 2, 0, 2);
		hdmi_set_bitw(link, HDMI_GCP_CD, 0x6, 8, 4);
		break;
	default:
		pr_err("%s: Failed unsupported color depth mode (%d)\n",
			__func__, depth);
		return;
	}

	if (depth == HDMI_COLOR_DEPTH_24BPP)
		/* disable General Control Packets */
		hdmi_set_bit(link, HDMI_GCP_EN, 0, 6);
	else
		/* enable General Control Packets */
		hdmi_set_bit(link, HDMI_GCP_EN, 1, 6);
#endif
}

#define CEA_640X480P_60HZ	25175000
#define DMT_640X480_60HZ	25175000
#define CVT_1440X900_60HZ_RB	88750000

static bool is_limited_q_range(int piexelclk)
{
	if ((piexelclk < DMT_640X480_60HZ) &&
	(piexelclk > CVT_1440X900_60HZ_RB) &&
	(piexelclk != CEA_640X480P_60HZ))
		return true;

	return false;
}

static int hdmilink_v2_avi_enable(void __iomem *link,
			struct videomode *vm, struct hdmi_video_info *video)
{
	u32 q_range, val;
	u32 min, max;
	int pixelclock = vm->pixelclock;
	enum hdmi_colorspace colorspace = video->colorspace;
	bool dvi_mode = video->dvi_mode;

	if (dvi_mode) {
		val = hdmi_readl(link, HDMI_PKT_CTRL) & ~(0x1FFF1);
		hdmi_set_bit(link, HDMI_DVI_MODE, 1, 4);
		hdmi_writel(link, HDMI_PKT_CTRL, val);
#ifdef CONFIG_HDMI20_JF_EVT1
		hdmi_set_bit(link, HDMI_OESS_MODE, 1, 7);
#endif
		return 0;
	}

	/* HDMI mode */
	hdmi_set_bit(link, HDMI_DVI_MODE, 0, 4);

#ifdef CONFIG_HDMI20_JF_EVT1
	hdmi_set_bit(link, HDMI_OESS_MODE, 0, 7);
#endif

	/* Pixel format for the pixel limit control*/
	if (colorspace != HDMI_COLORSPACE_RGB &&
	colorspace != HDMI_COLORSPACE_YUV422 &&
	colorspace != HDMI_COLORSPACE_YUV444 &&
	colorspace != HDMI_COLORSPACE_YUV420) {
		pr_err("%s: Failed not support color mode %d !!!\n",
			__func__, colorspace);
		return -EINVAL;
	}
	hdmi_set_bitw(link, HDMI_PIXEL_LIMIT_FORMAT, colorspace, 12, 2);

	/* Video Quantization Range */
	if (is_limited_q_range(pixelclock)) {
		/* CE, set limited range */
		q_range = HDMI_QUANTIZATION_RANGE_LIMITED;
		min = 0x10, max = 0xFEB;
	} else {
		/* IT (VESA), set full range */
		q_range = HDMI_QUANTIZATION_RANGE_FULL;
		min = 0x0, max = 0xFFF;
	}
	video->q_range = q_range;

	if (colorspace == HDMI_COLORSPACE_RGB) {
		/* TODO: add support for Deep Color modes */
		hdmi_set_bitw(link, HDMI_RGB_Y_LIMIT_MIN, min, 0, 12);
		hdmi_set_bitw(link, HDMI_RGB_Y_LIMIT_MAX, max, 16, 12);
	} else {
		hdmi_set_bitw(link, HDMI_RGB_Y_LIMIT_MIN, min,  0, 12);
		hdmi_set_bitw(link, HDMI_RGB_Y_LIMIT_MAX, max, 16, 12);
		hdmi_set_bitw(link, HDMI_CB_CR_LIMIT_MIN, min,  0, 12);
		hdmi_set_bitw(link, HDMI_CB_CR_LIMIT_MAX, max, 16, 12);
	}

	hdmilink_v2_avi_infoframe(link, vm, video);
	hdmilink_v2_vsi_infoframe(link, vm, video);
	hdmilink_v2_gcp(link, video);

	return 0;
}

#ifdef CONFIG_HDMI20_JF_EVT1
static void hdmilink_v2_aui_fifo_flush(void __iomem *link)
{
	hdmi_set_bit(link, HDMI_AUDIO_FIFO_FLUSH, 1, 0);
	hdmi_set_bit(link, HDMI_AUDIO_FIFO_FLUSH, 0, 0);
}
#endif

static void hdmilink_v2_aui_infoframe(
			void __iomem *link, struct hdmi_audio_info *audio)
{
	struct __hdmi_infoframe infoframe;
	enum hdmi_audio_coding_type coding = audio->channels_num;
	int channels = audio->channels_num;

	memset(&infoframe, 0, sizeof(struct __hdmi_infoframe));

	/* Packet Header */
	infoframe.hb0 = HDMI_INFOFRAME_TYPE_AUDIO;
	infoframe.hb1 = HDMI_AUI_VERSION;
	infoframe.hb2 =
		SET_MASK(HDMI_AUI_LENGTH, HDMI20_INFOFRAME_HB2_LEN_MASK);

	/**
	 * Channel Count [CC0...CC2]
	 * Channel Allocation [CA0...CA7]
	 */
	if (coding == HDMI_AUDIO_CODING_TYPE_PCM) {
		int ch, ca;

		switch (channels) {
		case 2:
			ch = HDMI_AUI_DATA_CC_2CH;
			ca = HDMI_AUI_DATA_CA_2CH;
			break;
		case 3:
			ch = HDMI_AUI_DATA_CC_3CH;
			ca = HDMI_AUI_DATA_CA_3CH;
			break;
		case 4:
			ch = HDMI_AUI_DATA_CC_4CH;
			ca = HDMI_AUI_DATA_CA_4CH;
			break;
		case 5:
			ch = HDMI_AUI_DATA_CC_5CH;
			ca = HDMI_AUI_DATA_CA_5CH;
			break;
		case 6:
			ch = HDMI_AUI_DATA_CC_6CH;
			ca = HDMI_AUI_DATA_CA_6CH;
			break;
		case 7:
			ch = HDMI_AUI_DATA_CC_7CH;
			ca = HDMI_AUI_DATA_CA_7CH;
			break;
		case 8:
			ch = HDMI_AUI_DATA_CC_8CH;
			ca = HDMI_AUI_DATA_CA_8CH;
			break;
		default:
			pr_err("%s: Failed unsupported audio channels (%d)\n",
				__func__, channels);
			return;
		}

		infoframe.pb01 |= SET_MASK(ch, HDMI20_AUI_PB1_CC_MASK);
		infoframe.pb04 |= SET_MASK(ca, HDMI20_AUI_PB4_CA_MASK);
	} else {
		/* Channel Count */
		infoframe.pb01 |= SET_MASK(HDMI_AUI_DATA_CC_HEADER,
			HDMI20_AUI_PB1_CC_MASK);
		/**
		 * The CA field is not valid for
		 * IEC 61937 compressed audio streams
		 */
	}

	/* write Audio packet */
	hdmilink_v2_write_infoframe(link, HDMI_AUI_CTRL, &infoframe);
}

static int hdmilink_v2_get_acr(
			enum hdmi_audio_sample_frequency rate,
			enum hdmi_color_depth depth,
			int pixelclock)
{
	int acr;

	switch (rate) {
	case HDMI_AUDIO_SAMPLE_FREQUENCY_32000:
		if (pixelclock == 25175000) {
			acr = 4576;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 9152;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 9152;
		} else if (pixelclock == 74176000 || pixelclock == 146250000) {
			acr = 11648;
		} else if (pixelclock == 296703000) {
			acr = 5824;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 11648;
		} else if (pixelclock == 297000000) {
			acr = 3072;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 6144;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 4096;
		} else if (pixelclock == 594000000 &&
			depth == HDMI_COLOR_DEPTH_24BPP) {
			acr = 3072;
		} else {
			acr = 4096; /* others */
			if (pixelclock == 27027000) {
				/* 33.75*1.001 (30bpp) */
				if (depth == HDMI_COLOR_DEPTH_30BPP)
					acr = 8192;
				if (depth == HDMI_COLOR_DEPTH_36BPP)
					acr = 8192;
			}

			if (pixelclock == 54054000 ||
			    pixelclock == 74250000)
				if (depth == HDMI_COLOR_DEPTH_30BPP)
					acr = 8192;
		}
		break;

	case HDMI_AUDIO_SAMPLE_FREQUENCY_44100:
		if (pixelclock == 25175000) {
			acr = 7007;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 14014;
		} else if (pixelclock == 74176000) {
			acr = 17836;
		} else if (pixelclock == 146250000) {
			acr = 8918;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 17836;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 17836;
		} else if (pixelclock == 296703000) {
			acr = 4459;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 17836;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 8918;
			if (depth == HDMI_COLOR_DEPTH_48BPP)
				acr = 4459;
		} else if (pixelclock == 594000000 &&
			depth == HDMI_COLOR_DEPTH_24BPP) {
			acr = 9408;
		} else {
			acr = 6272; /* others */
			if (pixelclock == 27027000)
				if (depth == HDMI_COLOR_DEPTH_30BPP)
					acr = 12544;
		}
		break;

	case HDMI_AUDIO_SAMPLE_FREQUENCY_48000:
		if (pixelclock == 25175000) {
			acr = 6864;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 9152;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 9152;
		} else if (pixelclock == 74176000) {
			acr = 11648;
		} else if (pixelclock == 146250000) {
			acr = 5824;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 11648;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 11648;
		} else if (pixelclock == 296703000) {
			acr = 5824;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 11648;
		} else if (pixelclock == 297000000) {
			acr = 5120;
		} else if (pixelclock == 594000000 &&
			depth == HDMI_COLOR_DEPTH_24BPP) {
			acr = 6144;
		} else {
			acr = 6144; /* others */
			if (pixelclock == 27027000) {
				if (depth == HDMI_COLOR_DEPTH_30BPP)
					acr = 8192;
				if (depth == HDMI_COLOR_DEPTH_36BPP)
					acr = 8192;
			}
			if (pixelclock == 54054000) {
				if (depth == HDMI_COLOR_DEPTH_30BPP)
					acr = 8192;
			}
			if (pixelclock == 74250000) {
				if (depth == HDMI_COLOR_DEPTH_30BPP)
					acr = 12288;
			}
		}
		break;

	case HDMI_AUDIO_SAMPLE_FREQUENCY_88200:
		if (pixelclock == 25175000) {
			acr = 14014;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 28028;
		} else if (pixelclock == 74176000) {
			acr = 35672;
		} else if (pixelclock == 146250000) {
			acr = 17836;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 35672;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 35672;
		} else if (pixelclock == 296703000) {
			acr = 8918;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 35672;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 17836;
			if (depth == HDMI_COLOR_DEPTH_48BPP)
				acr = 8918;
		} else if (pixelclock == 594000000 &&
			depth == HDMI_COLOR_DEPTH_24BPP) {
			acr = 18816;
		} else {
			acr = 12544; /* others */
			if (pixelclock == 27027000) {
				if (depth == HDMI_COLOR_DEPTH_30BPP)
					acr = 25088;
			}
		}
		break;

	case HDMI_AUDIO_SAMPLE_FREQUENCY_96000:
		if (pixelclock == 25175000) {
			acr = 13728;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 18304;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 18304;
		} else if (pixelclock == 74176000) {
			acr = 23296;
		} else if (pixelclock == 146250000) {
			acr = 11648;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 23296;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 23296;
		} else if (pixelclock == 296703000) {
			acr = 11648;

			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 23296;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 11648;
			if (depth == HDMI_COLOR_DEPTH_48BPP)
				acr = 11648;
		} else if (pixelclock == 297000000) {
			acr = 10240;
		} else if (pixelclock == 594000000 &&
			depth == HDMI_COLOR_DEPTH_24BPP) {
			acr = 12288;
		} else {
			acr = 12288; /* others */
			if (pixelclock == 27027000) {
				if (depth == HDMI_COLOR_DEPTH_30BPP)
					acr = 16384;
				if (depth == HDMI_COLOR_DEPTH_36BPP)
					acr = 16384;
			}
			if (pixelclock == 54054000) {
				if (depth == HDMI_COLOR_DEPTH_30BPP)
					acr = 16384;
			}
			if (pixelclock == 74250000) {
				if (depth == HDMI_COLOR_DEPTH_30BPP)
					acr = 24576;
			}
		}
		break;

	case HDMI_AUDIO_SAMPLE_FREQUENCY_176400:
		if (pixelclock == 25175000) {
			acr = 28028;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 56056;
		} else if (pixelclock == 74176000) {
			acr = 71344;
		} else if (pixelclock == 146250000) {
			acr = 35672;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 71344;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 71344;
		} else if (pixelclock == 296703000) {
			acr = 17836;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 71344;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 35672;
			if (depth == HDMI_COLOR_DEPTH_48BPP)
				acr = 17836;
		} else if (pixelclock == 297000000) {
			acr = 18816;
		} else if (pixelclock == 594000000 &&
			depth == HDMI_COLOR_DEPTH_24BPP) {
			acr = 37632;
		} else {
			acr = 25088; /* others */
			if (pixelclock == 27027000) {
				if (depth == HDMI_COLOR_DEPTH_30BPP)
					acr = 50176;
			}
		}
		break;

	case HDMI_AUDIO_SAMPLE_FREQUENCY_192000:
		if (pixelclock == 25175000) {
			acr = 27456;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 36608;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 36608;
		} else if (pixelclock == 74176000) {
			acr = 46592;
		} else if (pixelclock == 146250000) {
			acr = 23296;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 46592;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 46592;
		} else if (pixelclock == 296703000) {
			acr = 23296;
			if (depth == HDMI_COLOR_DEPTH_30BPP)
				acr = 46592;
			if (depth == HDMI_COLOR_DEPTH_36BPP)
				acr = 23296;
			if (depth == HDMI_COLOR_DEPTH_48BPP)
				acr = 23296;
		} else if (pixelclock == 297000000) {
			acr = 20480;
		} else if (pixelclock == 594000000 &&
			depth == HDMI_COLOR_DEPTH_24BPP) {
			acr = 24576;
		} else {
			acr = 24576; /* others */
			if (pixelclock == 27027000) {
				if (depth == HDMI_COLOR_DEPTH_30BPP)
					acr = 32768;
				if (depth == HDMI_COLOR_DEPTH_36BPP)
					acr = 32768;
			}
			if (pixelclock == 54054000) {
				if (depth == HDMI_COLOR_DEPTH_30BPP)
					acr = 32768;
			}
			if (pixelclock == 74250000) {
				if (depth == HDMI_COLOR_DEPTH_30BPP)
					acr = 49152;
			}
		}
		break;

	default:
		pr_err("%s: Failed unknown sample rate %d for %d pixelclock\n",
			__func__, rate, pixelclock);
		return -EINVAL;
	}

	return acr;
}

static void hdmilink_v2_aui_acr(void __iomem *link,
			struct hdmi_audio_info *audio,
			struct hdmi_video_info *video, int pixelclock)
{
	bool dvi_mode = audio->dvi_mode;
	enum hdmi_audio_sample_frequency rate = audio->sample_rate;
	enum hdmi_audio_input in = audio->input;
	enum hdmi_3d_structure struct_3d = video->struct_3d;
	enum hdmi_color_depth depth = video->color_depth;
	enum hdmi_colorspace colorspace = video->colorspace;
	enum hdmi_video_format format = video->format;
	int acr;

	/* disable ACR packets */
	if (dvi_mode) {
		hdmi_set_bit(link, HDMI_ACR_EN, 0, 4);
		return;
	}

	/*
	 * BugFix-S:
	 *  In case 3D frame packing,
	 *  3D pixel clock frequency is x2 of 2D pixel clock frequency
	 */
#ifdef CONFIG_HDMI20_CTRL_SUPPORT
	if (colorspace == HDMI_COLORSPACE_YUV420)
		pixelclock = pixelclock >> 1;

	if (format == HDMI_VIDEO_FORMAT_3D) {
		switch (struct_3d) {
		case HDMI_3D_STRUCTURE_FRAME_PACKING:
		case HDMI_3D_STRUCTURE_FIELD_ALTERNATIVE:
		case HDMI_3D_STRUCTURE_LINE_ALTERNATIVE:
		case HDMI_3D_STRUCTURE_SIDE_BY_SIDE_FULL:
		case HDMI_3D_STRUCTURE_L_DEPTH:
			pixelclock *= 2;
			break;
		case HDMI_3D_STRUCTURE_L_DEPTH_GFX_GFX_DEPTH:
			pixelclock *= 4;
			break;
		case HDMI_3D_STRUCTURE_TOP_AND_BOTTOM:
		case HDMI_3D_STRUCTURE_SIDE_BY_SIDE_HALF:
			break;
		default:
			pr_err("%s: Failed not support 3dmode %d !!!\n",
				__func__, struct_3d);
			break;
		}
	}
#endif
	acr = hdmilink_v2_get_acr(rate, depth, pixelclock);
	if (acr < 0)
		return;

	/*  I2S / SPDIF audio interface -> CTS will be calculated in HW */
	if ((in  == HDMI_AUDIO_IN_I2S) ||
		(in == HDMI_AUDIO_IN_SPDIF)) {
		hdmi_set_bitw(link, HDMI_ACR_CTS, acr, 0, 20);
		/* Clear SFR CTS bit setting */
		hdmi_set_bitw(link, HDMI_ACR_CTS, 0, 0, 20);
		hdmi_set_bitw(link, HDMI_ACR_CTS_MODE, 0x3, 4, 2);
	} else {
		u64 cts, divider;
		int sample_freq = 0;

		/* calculate CTS */
		switch (rate) {
		case HDMI_AUDIO_SAMPLE_FREQUENCY_32000:
			sample_freq = 32000;
			break;
		case HDMI_AUDIO_SAMPLE_FREQUENCY_44100:
			sample_freq = 44100;
			break;
		case HDMI_AUDIO_SAMPLE_FREQUENCY_48000:
			sample_freq = 48000;
			break;
		case HDMI_AUDIO_SAMPLE_FREQUENCY_88200:
			sample_freq = 88200;
			break;
		case HDMI_AUDIO_SAMPLE_FREQUENCY_96000:
			sample_freq = 96000;
			break;
		case HDMI_AUDIO_SAMPLE_FREQUENCY_176400:
			sample_freq = 176400;
			break;
		case HDMI_AUDIO_SAMPLE_FREQUENCY_192000:
			sample_freq = 192000;
			break;
		default:
			pr_err("%s: Failed unknown sample rate enum %d\n",
				__func__, rate);
			break;
		}

		cts = pixelclock * acr;
		divider = sample_freq * 128;
		do_div(cts, divider);

		hdmi_set_bitw(link, HDMI_ACR_CTS, (u32)cts, 0, 20);
		hdmi_set_bitw(link, HDMI_ACR_N, acr, 0, 20);
		hdmi_set_bitw(link, HDMI_ACR_CTS_MODE, 0x0, 4, 2);
	}

	/*
	 * if TMDS character rate > 340 Mcsc (TMDS bit/clock ratio 1/40),
	 * set HDMI20_ACR_SUB_CTS.HDMI20_ACR_SUB_CTS_DIV4_BIT to "1"
	 */
	hdmi_set_bit(link, HDMI_ACR_CTS_DIV4, 0, 20);
}

static void hdmilink_v2_aui_asp(
			void __iomem *link, struct hdmi_audio_info *audio)
{
	enum hdmi_audio_packet packet_type = audio->packet_type;
	enum hdmi_audio_coding_type coding = audio->coding;
	int channels = audio->channels_num;

	/* Audio Sample Packets */
	switch (packet_type) {
	case HDMI_AUDIO_PACKET_ASP:
		/* ASP packets type */
		hdmi_set_bitw(link, HDMI_AUD_TYPE, 0, 4, 2);

		if (coding == HDMI_AUDIO_CODING_TYPE_PCM) {
			if (channels == 2) {
				hdmi_set_bit(link, HDMI_AUD_LAYOUT, 0, 12);
			} else if (channels == 3 || channels == 4 ||
				channels == 5 || channels == 6 ||
				channels == 7 || channels == 8) {
				hdmi_set_bit(link, HDMI_AUD_LAYOUT, 1, 12);
				hdmi_set_bit(link, HDMI_AUD_LAYOUT, 1, 12);
			} else {
				pr_err("%s: Faliled unknown channels(%d)\n",
					__func__, channels);
				return;
			}
			hdmi_set_bitw(link, HDMI_AUD_CH_NUM, channels, 16, 4);
		} else {
			/* Non-linear PCM */
			hdmi_set_bit(link, HDMI_AUD_LAYOUT, 0, 12);
			hdmi_set_bitw(link, HDMI_AUD_CH_NUM, 2, 16, 4);
		}
		break;

	case HDMI_AUDIO_PACKET_HBR:
		/* High Bitrate (HBR) Audio Stream Packets */
		/* set HBR packets type */
		hdmi_set_bitw(link, HDMI_AUD_TYPE, 1, 4, 2);
		hdmi_set_bit(link, HDMI_AUD_LAYOUT, 1, 12);
		hdmi_set_bitw(link, HDMI_AUD_LAYOUT, 8, 16, 4);
		break;

	case HDMI_AUDIO_PACKET_MSA:
		/* Multi-Stream Audio Sample Packets */
		/* set MST packets type */
		hdmi_set_bitw(link, HDMI_AUD_TYPE, 2, 4, 2);
	default:
		pr_err("%s: Failed unsupported audio packet type (%d)\n",
			__func__, packet_type);
		return;
	}
}

static int hdmilink_v2_ch_status(
			void __iomem *link, struct hdmi_audio_info *audio)
{
	enum hdmi_audio_input input = audio->input;
	enum hdmi_audio_sample_frequency rate = audio->sample_rate;
	enum hdmi_audio_coding_type coding = audio->coding;
	enum hdmi_audio_bit_length length = audio->bit_length;
	u32 val = 0;

	/* linear/non-linear */
	if (coding != HDMI_AUDIO_CODING_TYPE_PCM)
		val |= HDMI_AUDIO_CHST_NON_LINEAR;

	/* set sampling frequency */
	switch (rate) {
	case HDMI_AUDIO_SAMPLE_FREQUENCY_32000:
		val |= HDMI_AUDIO_CHST_32KHZ;
		break;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_44100:
		val |= HDMI_AUDIO_CHST_44KHZ;
		break;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_48000:
		val |= HDMI_AUDIO_CHST_48KHZ;
		break;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_88200:
		val |= HDMI_AUDIO_CHST_88KHZ;
		break;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_96000:
		val |= HDMI_AUDIO_CHST_96KHZ;
		break;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_176400:
		val |= HDMI_AUDIO_CHST_176KHZ;
		break;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_192000:
		val |= HDMI_AUDIO_CHST_192KHZ;
		break;
	default:
		pr_err("%s: Failed unsupported sampling frequency\n",
			__func__);
		return -1;
	}

	hdmi_writel(link, HDMI_CH0_ST_0, val);
	hdmi_writel(link, HDMI_CH1_ST_0, val);
	hdmi_writel(link, HDMI_CH2_ST_0, val);
	hdmi_writel(link, HDMI_CH3_ST_0, val);
	hdmi_writel(link, HDMI_CH4_ST_0, val);
	hdmi_writel(link, HDMI_CH5_ST_0, val);
	hdmi_writel(link, HDMI_CH6_ST_0, val);
	hdmi_writel(link, HDMI_CH7_ST_0, val);

	val = 0;

	/* SPDIF case */
	if (input == HDMI_AUDIO_IN_SPDIF) {
		/* set word length */
		if (length == HDMI_AUDIO_BIT_LEN_24)
			val |= HDMI_AUDIO_CHST_SPDIF_MAX24_24BITS;
		else if (length == HDMI_AUDIO_BIT_LEN_16)
			val |= HDMI_AUDIO_CHST_SPDIF_MAX20_16BITS;
		else if (length == HDMI_AUDIO_BIT_LEN_17)
			val |= HDMI_AUDIO_CHST_SPDIF_MAX20_17BITS;
		else if (length == HDMI_AUDIO_BIT_LEN_18)
			val |= HDMI_AUDIO_CHST_SPDIF_MAX20_18BITS;
		else if (length == HDMI_AUDIO_BIT_LEN_19)
			val |= HDMI_AUDIO_CHST_SPDIF_MAX20_19BITS;
		else if (length == HDMI_AUDIO_BIT_LEN_20)
			val |= HDMI_AUDIO_CHST_SPDIF_MAX24_20BITS;
		else if (length == HDMI_AUDIO_BIT_LEN_21)
			val |= HDMI_AUDIO_CHST_SPDIF_MAX24_21BITS;
		else if (length == HDMI_AUDIO_BIT_LEN_22)
			val |= HDMI_AUDIO_CHST_SPDIF_MAX24_22BITS;
		else if (length == HDMI_AUDIO_BIT_LEN_23)
			val |= HDMI_AUDIO_CHST_SPDIF_MAX24_23BITS;
		else {
			pr_err("%s: Failed unsupported word length (%d)\n",
				__func__, length);
			return -1;
		}

		hdmi_writel(link, HDMI_CH0_ST_1, val);
		hdmi_writel(link, HDMI_CH1_ST_1, val);
	/* I2S and DMA case Audio input reg setting */
	} else {
		/* set word length */
		if (length == HDMI_AUDIO_BIT_LEN_24) {
			val |= HDMI_AUDIO_CHST_MAX_24BITS;
			val |= HDMI_AUDIO_CHST_MAX24_24BITS;
		} else if (length == HDMI_AUDIO_BIT_LEN_16) {
			val |= HDMI_AUDIO_CHST_MAX20_16BITS;
		} else {
			pr_err("%s: Failed unsupported word length (%d)\n",
				__func__, length);
			return -1;
		}

		hdmi_writel(link, HDMI_CH0_ST_1, val);
		hdmi_writel(link, HDMI_CH1_ST_1, val);
		hdmi_writel(link, HDMI_CH2_ST_1, val);
		hdmi_writel(link, HDMI_CH3_ST_1, val);
		hdmi_writel(link, HDMI_CH4_ST_1, val);
		hdmi_writel(link, HDMI_CH5_ST_1, val);
		hdmi_writel(link, HDMI_CH6_ST_1, val);
		hdmi_writel(link, HDMI_CH7_ST_1, val);
	}

	/* apply channel status bits */
	hdmi_set_bit(link, HDMI_CHANNEL_STATUS_BIT_RELOAD, 1, 0);

	return 0;
}

static int hdmilink_v2_aui_input(void __iomem *link,
			struct hdmi_audio_info *audio, bool enable)
{
	enum hdmi_audio_packet packet_type = audio->packet_type;
	enum hdmi_audio_coding_type coding = audio->coding;
	int channels = audio->channels_num;
	int bits_per_ch = audio->bits_per_ch;

	pr_debug("%s: enter\n", __func__);

	if (!enable) {
	#ifndef CONFIG_HDMI20_JF_EVT0
		/* disable audio input system */
		hdmi_set_bit(link, HDMI_AUDIO_INPUT_SYSTEM_ENABLE, 0, 0);
	#endif
		return 0;
	}

	hdmi_set_bit(link, HDMI_PCM_SELECT,  1, 0);

	/* set HBR mode */
	if (packet_type == HDMI_AUDIO_PACKET_HBR)
		hdmi_set_bit(link, HDMI_HBR, 1, 0);
	else
		hdmi_set_bit(link, HDMI_HBR, 0, 0);

	/* set channels */
	if (packet_type == HDMI_AUDIO_PACKET_ASP) {
		if (coding == HDMI_AUDIO_CODING_TYPE_PCM)
			/* linear PCM */
			hdmi_set_bitw(link, HDMI_CHANEL_NUMBER, channels, 0, 4);
		else
			/* non-linear PCM */
			hdmi_set_bitw(link, HDMI_CHANEL_NUMBER, 2, 0, 4);
	} else if (packet_type == HDMI_AUDIO_PACKET_HBR) {
		hdmi_set_bitw(link, HDMI_CHANEL_NUMBER, 8, 0, 4);
	} else if (packet_type == HDMI_AUDIO_PACKET_MSA) {
		pr_info("%s: TODO implement\n", __func__);
	} else {
		pr_err("%s: Failed unsupported audio packet type (%d)\n",
			__func__, packet_type);
		return -1;
	}

	/* set number of bits per channel */
	if (bits_per_ch == HDMI_AUDIO_BIT_PER_CH_16) {
		hdmi_set_bitw(link, HDMI_CHANNEL_BIT, 1, 0, 2);
	} else if (bits_per_ch == HDMI_AUDIO_BIT_PER_CH_24) {
		hdmi_set_bitw(link, HDMI_CHANNEL_BIT, 2, 0, 2);
	} else {
		pr_err("%s: Failed unsupported bits per channel\n",
			__func__);
		return -1;
	}

	/* set channel status bits mode */
	if (packet_type == HDMI_AUDIO_PACKET_HBR)
		hdmi_set_bitw(link, HDMI_CUV_SELECT, 2, 0, 3); /* HBR */
	else
		hdmi_set_bitw(link, HDMI_CUV_SELECT, 1, 0, 3);

	/* enable P (parity) bit calculation */
	hdmi_set_bit(link, HDMI_PSEL, 0, 0);

#ifdef CONFIG_HDMI20_JF_EVT0
	/* audio input system enable */
	hdmi_set_bit(link, HDMI_AUDIO_INPUT_SYSTEM_ENABLE, 1, 0);

	/*
	 * Note:
	 * when set clears channel status bits.
	 * Set HDMI_AUDIO_SYS_EN_BIT before setting channel status bits.
	 */
	hdmilink_v2_ch_status(link, audio);
#else
	/* set channel status bits */
	hdmilink_v2_ch_status(link, audio);

	/* audio input system enable */
	hdmi_set_bit(link, HDMI_AUDIO_INPUT_SYSTEM_ENABLE, 1, 0);
#endif
	return 0;
}

#if defined(CONFIG_HDMI_AUDIO_I2S)
#define HDMI_I2S_MSB_FIRST_MODE		(0 << 6)
#define HDMI_I2S_BASIC_FORMAT		(0)

#define HDMI_I2S_SET_BIT_CH(x)		(((x) & 0x3) << 4)
#define HDMI_I2S_SET_SDATA_BIT(x)	(((x) & 0x3) << 2)

static void hdmilink_v2_aui_i2s(void __iomem *link,
			struct hdmi_audio_info *audio)
{
	enum hdmi_audio_bit_length length = audio->bit_length;
	u32 data_bit, bit_ch, val;

	/* reset I2S */
	hdmi_set_bit(link, HDMI_I2S_CLK_CON, 0, 0);
	hdmi_set_bit(link, HDMI_I2S_CLK_CON, 1, 0);

	/* configure I2S input pins */
	hdmi_writel(link, HDMI_PIN_SEL_0, 1 | (0 << 4));
	hdmi_writel(link, HDMI_PIN_SEL_2, 3 | (3 << 4));
	hdmi_writel(link, HDMI_PIN_SEL_4, 4 | (5 << 4));
	hdmi_writel(link, HDMI_PIN_SEL_6, 6);

	val = (0 << 1) | (0);	/* FALLING_EDGE | LOW_POL */
	hdmi_writel(link, HDMI_I2S_CON_1, val);

	switch (length) {
	case HDMI_AUDIO_BIT_LEN_16:
		data_bit = 1;
		break;
	case HDMI_AUDIO_BIT_LEN_20:
		data_bit = 2;
		break;
	case HDMI_AUDIO_BIT_LEN_24:
		data_bit = 3;
		break;
	default:
		pr_err("%s: Failed unknown bits per channel\n",
			__func__);
		return;
	}

	bit_ch = 2; /* 64fs */

	val = HDMI_I2S_MSB_FIRST_MODE | HDMI_I2S_SET_BIT_CH(bit_ch) |
		HDMI_I2S_SET_SDATA_BIT(data_bit) | HDMI_I2S_BASIC_FORMAT;
	hdmi_writel(link, HDMI_I2S_CON_2, val);

	/* config mux */
	hdmi_writel(link, HDMI_I2S_IN_MUX_CON,
		HDMI_I2S_IN_ENABLE | HDMI_I2S_AUD_I2S |
		HDMI_I2S_CUV_I2S_ENABLE | HDMI_I2S_MUX_ENABLE);

	hdmi_writel(link, HDMI_I2S_MUX_CH, 0xFF);
}
#endif

#if defined(CONFIG_HDMI_AUDIO_SPDIF)
static void hdmilink_v2_aui_spdif(void __iomem *link,
			struct hdmi_audio_info *audio)
{
	/* reset I2S */
	hdmi_set_bit(link, HDMI_I2S_CLK_CON, 0, 0);
	hdmi_set_bit(link, HDMI_I2S_CLK_CON, 1, 0);

	hdmi_writel(link, HDMI_SPDIFIN_CONFIG_1,
		HDMI_SPDIFIN_CFG_NOISE_FILTER_2_SAMPLE |
		HDMI_SPDIFIN_CFG_WORD_LENGTH_MANUAL |
		HDMI_SPDIFIN_CFG_HDMI_2_BURST |
		HDMI_SPDIFIN_CFG_DATA_ALIGN_32);

	hdmi_writel(link, HDMI_SPDIFIN_USER_VALUE_1,
		HDMI_SPDIFIN_USER_VAL_WORD_LENGTH_24);

	hdmi_writel(link, HDMI_I2S_IN_MUX_CON,
		HDMI_I2S_IN_ENABLE |
		HDMI_I2S_AUD_SPDIF |
		HDMI_I2S_MUX_ENABLE);

	hdmi_writel(link, HDMI_I2S_MUX_CH, HDMI_I2S_CH_ALL_EN);

	hdmi_writel(link, HDMI_SPDIFIN_CLK_CTRL, 0);
	hdmi_set_bit(link, HDMI_SPDIFIN_CLK_CTRL, HDMI_SPDIFIN_CLK_ON);

	hdmi_writel(link, HDMI_SPDIFIN_OP_CTRL,
			HDMI_SPDIFIN_STATUS_CHECK_MODE);
	hdmi_writel(link, HDMI_SPDIFIN_OP_CTRL,
			HDMI_SPDIFIN_STATUS_CHECK_MODE_HDMI);
}
#endif

static void hdmilink_v2_aui_enable(void __iomem *link, bool enable)
{
	if (enable) {
		/*
		 * enable
		 * Audio InfoFrames, ACR packets, ASP packets
		 */
		hdmi_set_bit(link, HDMI_AUI_EN, 1, 14);
		hdmi_set_bit(link, HDMI_ACR_EN, 1, 4);
		hdmi_set_bit(link, HDMI_AUD_EN, 1, 5);
	} else {
		/*
		 * disble
		 * Audio InfoFrames, ACR packets, ASP packets
		 */
		hdmi_set_bit(link, HDMI_AUI_EN, 0, 14);
		hdmi_set_bit(link, HDMI_ACR_EN, 0, 4);
		hdmi_set_bit(link, HDMI_AUD_EN, 0, 5);
	}
}

static void hdmilink_v2_enable(void __iomem *link, bool enable)
{
	pr_debug("%s: enter\n", __func__);

	hdmi_set_bit(link, HDMI_HDMI_EN, enable ? 1 : 0, 0);
}

static int hdmi_phy_wait_ready(struct nxs_hdmi *hdmi)
{
	void __iomem *link  = hdmi->reg_link;
	int count = 50;
	bool ret = false;

	do {
		ret = hdmilink_v2_phy_ready(link);
		if (ret)
			break;
		mdelay(10);
	} while (count--);

	pr_info("HDMI: PHY [%s][0x%x] ...\n",
	       ret ? "Ready Done" : "Fail : Not Ready", ret);

	return ret ? 0 : -EIO;
}

static int hdmi_find_phyconf(struct nxs_hdmi *hdmi, int pixelclock)
{
	const struct hdmi_phy_conf *phy_conf;
	int i;

	DBG(hdmi, "pixelclock : %dhz\n", pixelclock);

	for (i = 0; i < hdmi->phy_confs_num; i++) {
		phy_conf = hdmi->phy_confs_list[i];
		if (phy_conf->pixel_clock == pixelclock)
			return i;
	}

	DBG(hdmi, "Could not find phy pixelclock : %dhz\n",
		pixelclock);

	return -EINVAL;
}

static int hdmi_find_preferred(struct nxs_hdmi *hdmi,
			struct videomode *vm, int vrefresh)
{
	int pixelclock = vm->pixelclock;
	int i;

	DBG(hdmi, "search %4d x %4d, %2d fps, %dhz\n",
		 vm->hactive, vm->vactive, vrefresh, pixelclock);

	for (i = 0; hdmi->preferred_num > i; i++) {
		const struct hdmi_preferred_conf *preferred =
				&hdmi->preferred[i];

		if (preferred->vm.hactive != vm->hactive ||
		preferred->vm.vactive != vm->vactive)
			continue;

		if (vrefresh && (preferred->hz != vrefresh))
			continue;

		ERR(hdmi, "Find %4d x %4d, %2d fps, %ldhz [%s]\n",
			 preferred->vm.hactive, preferred->vm.vactive,
			 preferred->hz, preferred->vm.pixelclock,
			 preferred->name);
		return i;
	}

	DBG(hdmi, "Not Find !\n");
	return -EINVAL;
}

static int hdmi_phy_set(struct nxs_hdmi *hdmi)
{
	void __iomem *phy = hdmi->reg_phy;
	const struct hdmi_phy_conf *phy_conf = hdmi->phy_conf;
	int pixelbits = hdmi->pixelbits;
	enum hdmi_tmds_bitclk_ratio ratio = hdmi->tmds_clk_ratio;
	enum hdmi_video_format format = hdmi->video.format;
	enum hdmi_colorspace colorspace = hdmi->video.colorspace;
	enum hdmi_3d_structure struct_3d = hdmi->video.struct_3d;
	const char *data;
	int pixelclock;
	int ret;

	if (WARN_ON(!phy_conf))
		return -EINVAL;

#ifdef CONFIG_HDMI20_TMDS_BIT_CLOCK_RATIO
	/* disable HDMI clk and data lines */
	hdmiphy_v2_output(phy, false);

	/*
	 * The Source shall allow a min of 1 ms and a max of 100 ms from the
	 * time the TMDS_Bit_Clock_Ratio is written until resuming transmission
	 * of TMDS clock and data at the updated data rate.
	 */
	usleep(1*1000);
#endif

	/* reconfig phy_conf */
	pixelclock = phy_conf->pixel_clock;

	if (colorspace == HDMI_COLORSPACE_YUV420)
		pixelclock = pixelclock >> 1;

	if (format == HDMI_VIDEO_FORMAT_3D) {
		switch (struct_3d) {
		case HDMI_3D_STRUCTURE_FRAME_PACKING:
		case HDMI_3D_STRUCTURE_FIELD_ALTERNATIVE:
		case HDMI_3D_STRUCTURE_LINE_ALTERNATIVE:
		case HDMI_3D_STRUCTURE_SIDE_BY_SIDE_FULL:
		case HDMI_3D_STRUCTURE_L_DEPTH:
			pixelclock *= 2;
			break;
		case HDMI_3D_STRUCTURE_L_DEPTH_GFX_GFX_DEPTH:
			pixelclock *= 4;
			break;
		case HDMI_3D_STRUCTURE_TOP_AND_BOTTOM:
		case HDMI_3D_STRUCTURE_SIDE_BY_SIDE_HALF:
			break;
		default:
			ERR(hdmi, "Failed not support 3dmode %d !!!\n",
				struct_3d);
			return -EINVAL;
		}
	}

	/* find new phy config for new pixel clock */
	if (pixelclock != phy_conf->pixel_clock) {
		int err = hdmi_find_phyconf(hdmi, pixelclock);

		if (err < 0) {
			ERR(hdmi, "Failed to new phy conf %d ->%d ",
				phy_conf->pixel_clock, pixelclock);
			ERR(hdmi, "color:%d, 3D:%d\n",
				colorspace, struct_3d);
			return -EINVAL;
		}
		phy_conf = hdmi->phy_confs_list[err];
	}


	switch (pixelbits) {
	case 8:
		data = phy_conf->pixel08;
		break;
	case 10:
		data = phy_conf->pixel10;
		break;
	default:
		ERR(hdmi, "Failed not support bitpixel (%d)\n", pixelbits);
		return -EINVAL;
	}

	/*
	 * configure HDMI PHY
	 */
	hdmiphy_v2_mode_set(phy, false);
	hdmiphy_v2_pll_conf(phy, data, PHY_CONFIG_SIZE);

	ret = hdmiphy_v2_tmds_ratio(phy, ratio);
	if (ret)
		return ret;

	hdmiphy_v2_mode_set(phy, true);

	return ret;
}

static int hdmi_audio_run(struct nxs_hdmi *hdmi)
{
	void __iomem *link = hdmi->reg_link;
	struct hdmi_video_info *video = &hdmi->video;
	struct hdmi_audio_info *audio = &hdmi->audio;
	bool dvi_mode = hdmi->dvi_mode;
	int pixelclock = hdmi->vm.pixelclock;
	int ret;

	audio->dvi_mode = dvi_mode;
	DBG(hdmi, "enter %s mode\n", dvi_mode ? "DVI" : "HDMI");

	if (dvi_mode)
		return 0;

	if (hdmi->audio_enabled) {
		DBG(hdmi, "audio already enabled\n");
		return 0;
	}

#ifdef CONFIG_HDMI20_JF_EVT1
	/* flush audio FIFO */
	hdmilink_v2_aui_fifo_flush(link);
#endif
	hdmilink_v2_aui_infoframe(link, audio);
	hdmilink_v2_aui_acr(link, audio, video, pixelclock);
	hdmilink_v2_aui_asp(link, audio);
	hdmilink_v2_aui_enable(link, true);

	ret = hdmilink_v2_aui_input(link, audio, true);
	if (ret < 0)
		return ret;

#ifdef CONFIG_HDMI_AUDIO_I2S
	if (in == HDMI_AUDIO_IN_I2S)
		hdmilink_v2_aui_i2s(link, audio);
#endif
#ifdef CONFIG_HDMI_AUDIO_SPDIF
	if (in == HDMI_AUDIO_IN_SPDIF)
		hdmilink_v2_aui_spdif(link, audio);
#endif
	hdmi->audio_enabled = true;

	return 0;
}

static void hdmi_audio_stop(struct nxs_hdmi *hdmi)
{
	void __iomem *link = hdmi->reg_link;
	struct hdmi_audio_info *audio = &hdmi->audio;

	DBG(hdmi, "enter\n");

	if (!hdmi->audio_enabled) {
		DBG(hdmi, "audio already disabled\n");
		return;
	}

	hdmilink_v2_aui_input(link, audio, false);
	hdmilink_v2_aui_enable(link, false);
	hdmi->audio_enabled = false;
}

static int hdmi_video_run(struct nxs_hdmi *hdmi)
{
	void __iomem *link = hdmi->reg_link;
	struct videomode *vm = &hdmi->vm;
	struct hdmi_video_info *video = &hdmi->video;
	int hpol, vpol;

	video->dvi_mode = hdmi->dvi_mode;
	hpol = vm->flags | DISPLAY_FLAGS_HSYNC_HIGH ? 1 : 0;
	vpol = vm->flags | DISPLAY_FLAGS_VSYNC_HIGH ? 1 : 0;

	DBG(hdmi, "enter\n");

	hdmilink_v2_sync_pol(link, hpol, vpol);
	hdmilink_v2_pattern(link, vm, video->format,
		video->struct_3d, hpol, vpol, hdmi->pattern);

	hdmilink_v2_avi_enable(link, vm, video);

	return 0;
}

static void hdmi_hpd_run(struct nxs_hdmi *hdmi, bool on)
{
	if (on && hdmi->hpd_irq_enabled)
		return;

	hdmilink_v2_hpd_on(hdmi->reg_link, on);
	hdmi->hpd_irq_enabled = on;
}

static void hdmi_run(struct nxs_hdmi *hdmi)
{
	void __iomem *link = hdmi->reg_link;

	hdmilink_v2_enable(link, true);
}

static void hdmi_clk_tmds(struct nxs_hdmi *hdmi)
{
	struct hdmi_clock *clks = hdmi->clocks;

	/* hdmiv2_0_pixelx2 */
	__clock_source(&clks->hdmiv2_0_pixelx2,  9); /* SCLK_HDMIPHY_PIXEL */

	/* hdmiv2_0_tmds_10b */
	__clock_source(&clks->hdmiv2_0_tmds_10b, 10); /* SCLK_HDMIPHY_TMDS */
}

static void hdmi_clk_pixel(struct nxs_hdmi *hdmi)
{
	struct hdmi_clock *clks = hdmi->clocks;

	__clock_divisor(&clks->hdmiv2_0_pixel, 2);
}

static int hdmi_clk_supply(struct nxs_hdmi *hdmi)
{
	hdmi_clk_tmds(hdmi);
	hdmi_clk_pixel(hdmi);

	return 0;
}

static void hdmi_clk_reset_init(struct nxs_hdmi *hdmi)
{
	struct hdmi_clock *clks = &hdmi_clocks;
	struct clock_manage **cmus = &clks->cmu_sys;
	struct clock_manage *cmu;
	int i;

	hdmi->clocks = clks;

	for (i = 0; i < clks->num_cmus; i++) {
		cmu = cmus[i];
		cmu->cpu_addr = ioremap(cmu->dma_addr, cmu->size);
		DBG(hdmi, "%s remap 0x%x to %p (%x) (%p)\n",
			cmu->name, (u32)cmu->dma_addr,
			cmu->cpu_addr, (u32)cmu->size, cmu);
	}

	/* reset cmu */
	__reset_cmu_deassert(clks->cmu_disp, true);
	__reset_cmu_deassert(clks->cmu_hdmi, true);

	/* reset nxs2hdmi */
	__clock_enable(&clks->nxs2hdmi_0_axi, true);
	__reset_enable(&clks->nxs2hdmi_0_axi, true);

	/* reset nxs2hdmi */
	__clock_enable(&clks->hdmi_0_axi, false);
	/* no reset synchronizer mode */
	__reset_mode(&clks->hdmi_0_axi, 1);
	__reset_enable(&clks->hdmi_0_axi, true);
	__clock_enable(&clks->hdmi_0_axi, true);
	__reset_mode(&clks->hdmi_0_axi, 0);

	__clock_enable(&clks->hdmi_0_apb, true);
	__clock_enable(&clks->hdmiv2_0_axi, true);
	__clock_enable(&clks->hdmiv2_0_apb, true);
	__clock_enable(&clks->hdmiv2_0_apbphy, true);
	__clock_enable(&clks->hdmiv2_0_phy, true);

	__reset_enable(&clks->hdmi_0_apb, true);
	__reset_enable(&clks->hdmiv2_0_axi, true);
	__reset_enable(&clks->hdmiv2_0_apb, true);
	__reset_enable(&clks->hdmiv2_0_apbphy, true);
	__reset_enable(&clks->hdmiv2_0_phy, true);
}

static void hdmi_clk_reset(struct nxs_hdmi *hdmi)
{
	void __iomem *tieoff  = hdmi->reg_tieoff;
	struct hdmi_clock *clks = hdmi->clocks;

	DBG(hdmi, "enter\n");

	/* TIEOFF */
	hdmi_set_bit(tieoff, 0, 1, HDMI_TIEOFF_PHY_EN);
	__reset_enable(&clks->hdmiv2_0_phy, true);
	__reset_enable(&clks->hdmiv2_0_apbphy, true);

	/*
	 * HDMI PIXEL/TMDS reset
	*/
	__clock_enable(&clks->hdmiphy_pixel_0, true);
	__clock_enable(&clks->hdmiphy_tmds_0, true);
	__clock_enable(&clks->hdmiv2_0_tmds_10b, true);
	__clock_enable(&clks->hdmiv2_0_tmds_20b, true);
	__clock_enable(&clks->hdmiv2_0_pixelx2, true);
	__clock_enable(&clks->hdmiv2_0_pixel, true);
	__clock_enable(&clks->hdmiv2_0_audio, true);

	__reset_enable(&clks->hdmiv2_0_tmds_10b, true);
	__reset_enable(&clks->hdmiv2_0_tmds_20b, true);
	__reset_enable(&clks->hdmiv2_0_pixelx2, true);
	__reset_enable(&clks->hdmiv2_0_pixel, true);
	__reset_enable(&clks->hdmiv2_0_audio, true);
}

#define	PACKET_OFFSET	(29 * 2)

static int hdmi_vps_sub(struct nxs_hdmi *hdmi)
{
	struct nxs_hdmi_syncgen_reg *reg = hdmi->reg_hdmi;
	struct videomode *vm = &hdmi->vm;
	enum hdmi_video_format format = hdmi->video.format;
	enum hdmi_colorspace colorspace = hdmi->video.colorspace;
	enum hdmi_3d_structure struct_3d = hdmi->video.struct_3d;
	enum hdmi_color_depth depth = hdmi->video.color_depth;

	int pk_hc_clr, pk_vc_offs;
	int hto = vm->hactive  + vm->hfront_porch +
			vm->hback_porch + vm->hsync_len - 1;
	int has = vm->hback_porch + vm->hsync_len - 1;
	int hae = vm->hactive  + vm->hback_porch +
			vm->hsync_len - 1;
	bool interlaced = vm->flags & DISPLAY_FLAGS_INTERLACED ? true : false;
	int val;
	bool dither;

	/* HDMI 3D MODE */
	if (format == HDMI_VIDEO_FORMAT_3D) {
		switch (struct_3d) {
		case HDMI_3D_STRUCTURE_LINE_ALTERNATIVE:
		case HDMI_3D_STRUCTURE_SIDE_BY_SIDE_FULL:
		case HDMI_3D_STRUCTURE_TOP_AND_BOTTOM:
		case HDMI_3D_STRUCTURE_SIDE_BY_SIDE_HALF:
			val = interlaced ? 0 : 1;
			__write_bitw(&reg->syncgen.debug_1, val, 0, 1);
			break;
		case HDMI_3D_STRUCTURE_FIELD_ALTERNATIVE:
		case HDMI_3D_STRUCTURE_FRAME_PACKING:
			__write_bitw(&reg->syncgen.ctrl_0, 1, 4, 1);
			__write_bitw(&reg->syncgen.ctrl_0, 1, 6, 1);

			val = interlaced ? 1 : 0;
			__write_bitw(&reg->compsel0, val, 28, 4);

			val = interlaced ? 0xe : 0x2;
			__write_bitw(&reg->syncgen.ctrl_0, val, 6, 1);
			__write_bitw(&reg->compsel0, 0x2, 28, 4);
			__write_bitw(&reg->syncgen.debug_1, 0, 0, 1);
			break;
		case HDMI_3D_STRUCTURE_L_DEPTH:
			__write_bitw(&reg->syncgen.ctrl_0, 1, 4, 1);
			__write_bitw(&reg->syncgen.ctrl_0, 0, 6, 1);
			__write_bitw(&reg->compsel0, 0x2, 28, 4);
			__write_bitw(&reg->syncgen.debug_1, 0, 0, 1);
			break;
		case HDMI_3D_STRUCTURE_L_DEPTH_GFX_GFX_DEPTH:
			__write_bitw(&reg->syncgen.ctrl_0, 1, 4, 1);
			__write_bitw(&reg->syncgen.ctrl_0, 1, 6, 1);
			__write_bitw(&reg->compsel0, 0xE, 28, 4);
			__write_bitw(&reg->syncgen.debug_1, 0, 0, 1);
			break;
		default:
			ERR(hdmi, "Failed not support 3dmode %d !!!\n",
				struct_3d);
			return -EINVAL;
		}
	}
	DBG(hdmi, "%s (3D:%d)\n",
		format == HDMI_VIDEO_FORMAT_3D ? "3D" : "2D", struct_3d);

	/*
	 * HDMI COLOR MODE
	 * Normal or 3D Frame Packing or 3D L-Depth or 3D GFX L-Depth
	 */
	if (colorspace == HDMI_COLORSPACE_RGB) {
		__write_bitw(&reg->compsel0, 2, 24, 4);
		__write_bitw(&reg->compsel0, 1, 20, 4);
		__write_bitw(&reg->compsel0, 0, 16, 4);
		__write_bitw(&reg->compsel0, 2, 12, 4);
		__write_bitw(&reg->compsel0, 1, 8, 4);
		__write_bitw(&reg->compsel0, 0, 4, 4);
		__write_bitw(&reg->syncgen.ctrl_1, 0, 3, 1);
		__write_bitw(&reg->compsel1, 0, 0, 1);
	} else if (colorspace == HDMI_COLORSPACE_YUV444) {
		__write_bitw(&reg->compsel0, 1, 24, 4);
		__write_bitw(&reg->compsel0, 0, 20, 4);
		__write_bitw(&reg->compsel0, 2, 16, 4);
		__write_bitw(&reg->compsel0, 1, 12, 4);
		__write_bitw(&reg->compsel0, 0, 8, 4);
		__write_bitw(&reg->compsel0, 2, 4, 4);
		__write_bitw(&reg->syncgen.ctrl_1, 0, 3, 1);
		__write_bitw(&reg->compsel1, 0, 0, 1);
	} else if (colorspace == HDMI_COLORSPACE_YUV422) {
		__write_bitw(&reg->compsel0, 14, 24, 4); /* y,cr lsb */
		__write_bitw(&reg->compsel0, 0, 20, 4); /* y[1] */
		__write_bitw(&reg->compsel0, 10, 16, 4); /* (cr[1]+cr[0])/2 */
		__write_bitw(&reg->compsel0, 13, 12, 4); /* y,cb lsb */
		__write_bitw(&reg->compsel0, 0, 8, 4); /* y[0] */
		__write_bitw(&reg->compsel0, 9, 4, 4); /* (cb[1]+cb[0])/2 */
		__write_bitw(&reg->syncgen.ctrl_1, 0, 3, 1);
		__write_bitw(&reg->compsel1, 0, 0, 1);
	} else if (colorspace == HDMI_COLORSPACE_YUV420) {
		__write_bitw(&reg->compsel0, 9, 24, 4); /* y,cr lsb */
		__write_bitw(&reg->compsel0, 0, 20, 4); /* y[1] */
		__write_bitw(&reg->compsel0, 4, 16, 4); /* (cr[1]+cr[0])/2 */
		__write_bitw(&reg->compsel0, 9, 12, 4); /* y,cb lsb */
		__write_bitw(&reg->compsel0, 0, 8, 4); /* y[0] */
		__write_bitw(&reg->compsel0, 4, 4, 4); /* (cb[1]+cb[0])/2 */
		__write_bitw(&reg->compsel1, 10, 24, 4); /* cr[2] */
		__write_bitw(&reg->compsel1, 0, 20, 4); /* y[2] */
		__write_bitw(&reg->compsel1, 4, 16, 4); /* y[3] */
		__write_bitw(&reg->compsel1, 10, 12, 4); /* cr[0] */
		__write_bitw(&reg->compsel1, 0, 8, 4); /* y[0] */
		__write_bitw(&reg->compsel1, 4, 4, 4); /* y[1] */
		__write_bitw(&reg->syncgen.ctrl_1, 1, 3, 1);
		__write_bitw(&reg->compsel1, 1, 0, 1);

		hto = hto - ((hae - has) / 2);
		hae = hae - ((hae - has) / 2);
		__write_bitw(&reg->syncgen.htotal, hto, 0, 16);
		__write_bitw(&reg->syncgen.haend, hae, 0, 16);
	} else {
		ERR(hdmi, "Failed not support color mode %d !!!\n",
			colorspace);
		return -EINVAL;
	}
	DBG(hdmi, "colorspace :%s\n",
		colorspace == HDMI_COLORSPACE_RGB ? "RGB" :
		colorspace == HDMI_COLORSPACE_YUV420 ? "YUV420" :
		colorspace == HDMI_COLORSPACE_YUV422 ? "YUV422" :
		colorspace == HDMI_COLORSPACE_YUV444 ? "YUV424" : "unknown");

	/* dither and rgb expand */
	dither = depth != HDMI_COLOR_DEPTH_24BPP ? 1 : 0;
	__write_bitw(&reg->compsel1, dither, 1, 1);
	__write_bitw(&reg->syncgen.decdither, dither, 16, 1);
	DBG(hdmi, "dither(%s)\n",
		dither ? "on" : "off");

	/* HDMI PACKET_EN_0 */
	if (has >= PACKET_OFFSET) {
		pk_hc_clr = has - PACKET_OFFSET;
		pk_vc_offs = 1;
	} else {
		pk_hc_clr = hto + (has - PACKET_OFFSET);
		pk_vc_offs = 0;
	}
	__write_bitw(&reg->packeten0, pk_hc_clr, 0, 16);
	__write_bitw(&reg->packeten0, pk_vc_offs, 16, 2);
	DBG(hdmi, "packet 0 vcnt:%d hcnt:%d\n",
		pk_hc_clr, pk_vc_offs);

	/* HDMI PACKET_EN_1 */
	if (has >= PACKET_OFFSET) {
		pk_hc_clr = has - PACKET_OFFSET;
		pk_vc_offs = 1;
	} else {
		pk_hc_clr  = hto + (has - PACKET_OFFSET);
		pk_vc_offs = 0;
	}

	__write_bitw(&reg->packeten1, pk_hc_clr, 0, 16);
	__write_bitw(&reg->packeten1, pk_vc_offs, 16, 2);
	DBG(hdmi, "packet 1 vcnt:%d hcnt:%d\n",
		pk_hc_clr, pk_vc_offs);

	return 0;
}

static int hdmi_vps_top(struct nxs_hdmi *hdmi)
{
	struct nxs_hdmi_syncgen_reg *reg = hdmi->reg_hdmi;
	struct videomode *vm = &hdmi->vm;

	int hto, hsw, has, hae;
	int vto, vsw, vas, vae;
	int evto = 0, evsw = 0, evas = 0, evae = 0;
	int vsp = 0, vcp = 0, evsp = 0, evcp = 0;
	bool interlaced = vm->flags & DISPLAY_FLAGS_INTERLACED ? true : false;

	hto = vm->hactive  + vm->hfront_porch +
		vm->hback_porch + vm->hsync_len - 1;
	has = vm->hback_porch + vm->hsync_len - 1;
	hsw = vm->hsync_len - 1;
	hae = vm->hactive  + vm->hback_porch + vm->hsync_len - 1;
	vto = vm->vactive + vm->vfront_porch +
		vm->vback_porch + vm->vsync_len - 1;
	vsw = vm->vsync_len - 1;
	vas = vm->vback_porch + vm->vsync_len - 1;
	vae = vm->vactive + vm->vback_porch + vm->vsync_len - 1;

	if (interlaced) {
		evto = vm->hactive + vm->vfront_porch +
			vm->vback_porch + vm->vsync_len - 1;
		evsw = vm->vsync_len - 1;
		evas = vm->vback_porch + vm->vsync_len - 1;
		evae = vm->vactive + vm->vback_porch +
			vm->vsync_len - 1;
		vsp = 0;
		vcp = (vm->hsync_len + vm->hfront_porch +
			vm->hback_porch + vm->hactive) / 2;
		evsp = (vm->hsync_len + vm->hfront_porch +
			vm->hback_porch + vm->hactive) / 2;
		evcp = 0;
	}

	__write_bitw(&reg->syncgen.htotal, hto, 0, 16);
	__write_bitw(&reg->syncgen.hswidth, hsw, 0, 16);
	__write_bitw(&reg->syncgen.hastart, has, 0, 16);
	__write_bitw(&reg->syncgen.haend, hae, 0, 16);
	__write_bitw(&reg->syncgen.vtotal, vto, 0, 16);
	__write_bitw(&reg->syncgen.vswidth, vsw, 0, 16);
	__write_bitw(&reg->syncgen.vastart, vas, 0, 16);
	__write_bitw(&reg->syncgen.vaend, vae, 0, 16);
	__write_bitw(&reg->syncgen.evtotal, evto, 0, 16);
	__write_bitw(&reg->syncgen.evswidth, evsw, 0, 16);
	__write_bitw(&reg->syncgen.evastart, evas, 0, 16);
	__write_bitw(&reg->syncgen.evaend, evae, 0, 16);
	__write_bitw(&reg->syncgen.vsetpixel, vsp, 0, 16);
	__write_bitw(&reg->syncgen.vclrpixel, vcp, 0, 16);
	__write_bitw(&reg->syncgen.evenvsetpixel, evsp, 0, 16);
	__write_bitw(&reg->syncgen.evenvclrpixel, evcp, 0, 16);
	__write_bitw(&reg->syncgen.ctrl_0, interlaced ? 1 : 0, 4, 1);
	__write_bitw(&reg->syncgen.use_even_vconfig, interlaced ? 1 : 0, 0, 1);

	DBG(hdmi, "SCAN %s\n",
		interlaced ? "Interlaced" : "Progessive");
	DBG(hdmi, "HSYNC haw:%4d, hsw:%4d, hbp:%4d, hfp:%4d, pol:%s\n",
		vm->hactive, vm->hsync_len, vm->hback_porch,
		vm->hfront_porch,
		vm->flags & DISPLAY_FLAGS_HSYNC_LOW ? "L" : "H");
	DBG(hdmi, "-> hto:%4d, hsw:%4d, has:%4d, hae:%4d\n",
		hto, hsw, has, hae);
	DBG(hdmi, "VSYNC vaw:%4d, vsw:%4d, vbp:%4d, vfp:%4d, pol:%s\n",
		vm->vactive, vm->vsync_len, vm->vback_porch,
		vm->vfront_porch,
		vm->flags & DISPLAY_FLAGS_VSYNC_LOW ? "L" : "H");
	DBG(hdmi, "-> vto:%4d, vsw:%4d, vas:%4d, vae:%4d\n",
		vto, vsw, vas, vae);
	DBG(hdmi, "EVEN evto:%4d, evsw:%4d, evas:%4d, evae:%4d\n",
		evto, evsw, evas, evae);
	DBG(hdmi, "EVEN vsp:%4d, vcp:%4d, evsp:%4d, evcp:%4d\n",
		vsp, vcp, evsp, evcp);

	return 0;
}

static int hdmi_vps_set(struct nxs_hdmi *hdmi)
{
	int ret;

	ret = hdmi_clk_supply(hdmi);
	if (ret < 0)
		return ret;

	ret = hdmi_vps_top(hdmi);
	if (ret < 0)
		return ret;

	return hdmi_vps_sub(hdmi);
}

static bool hdmi_is_connected(struct nxs_hdmi *hdmi)
{
	return hdmilink_v2_connected(hdmi->reg_link);
}

static void hdmi_vm_to_sync(struct nxs_hdmi *hdmi, const struct videomode *vm,
			struct nxs_control_syncinfo *sync)
{
	sync->pixelclock = vm->pixelclock;
	sync->width = vm->hactive;
	sync->hfrontporch = vm->hfront_porch;
	sync->hbackporch = vm->hback_porch;
	sync->hsyncwidth = vm->hsync_len;
	sync->height = vm->vactive;
	sync->vfrontporch = vm->vfront_porch;
	sync->vbackporch = vm->vback_porch;
	sync->vsyncwidth = vm->vsync_len;
	sync->flags = 0;

	if (vm->flags & DISPLAY_FLAGS_INTERLACED)
		sync->interlaced = 1;

	if (vm->flags & DISPLAY_FLAGS_VSYNC_LOW)
		sync->vsyncpol = 1;

	if (vm->flags & DISPLAY_FLAGS_HSYNC_LOW)
		sync->hsyncpol = 1;

	DBG(hdmi, "SCAN %s\n",
		vm->flags & DISPLAY_FLAGS_INTERLACED ?
		"Interlaced" : "Progessive");
	DBG(hdmi, "HSYNC haw:%4d, hsw:%4d, hbp:%4d, hfp:%4d, pol:%s\n",
		vm->hactive, vm->hsync_len, vm->hback_porch,
		vm->hfront_porch,
		vm->flags & DISPLAY_FLAGS_HSYNC_LOW ? "L" : "H");
	DBG(hdmi, "VSYNC vaw:%4d, vsw:%4d, vbp:%4d, vfp:%4d, pol:%s\n",
		vm->vactive, vm->vsync_len, vm->vback_porch,
		vm->vfront_porch,
		vm->flags & DISPLAY_FLAGS_VSYNC_LOW ? "L" : "H");
}

static void hdmi_sync_to_vm(struct nxs_hdmi *hdmi,
			const struct nxs_control_syncinfo *sync,
			struct videomode *vm)
{
	vm->pixelclock = sync->pixelclock;
	vm->hactive = sync->width;
	vm->hfront_porch = sync->hfrontporch;
	vm->hback_porch = sync->hbackporch;
	vm->hsync_len = sync->hsyncwidth;
	vm->vactive = sync->height;
	vm->vfront_porch = sync->vfrontporch;
	vm->vback_porch = sync->vbackporch;
	vm->vsync_len = sync->vsyncwidth;
	vm->flags = 0;

	if (sync->interlaced)
		vm->flags |= DISPLAY_FLAGS_INTERLACED;

	if (sync->vsyncpol)
		vm->flags |= DISPLAY_FLAGS_VSYNC_LOW;

	if (sync->hsyncpol)
		vm->flags |= DISPLAY_FLAGS_HSYNC_LOW;

	DBG(hdmi, "SCAN %s\n",
		vm->flags & DISPLAY_FLAGS_INTERLACED ?
		"Interlaced" : "Progessive");
	DBG(hdmi, "HSYNC haw:%4d, hsw:%4d, hbp:%4d, hfp:%4d, pol:%s\n",
		vm->hactive, vm->hsync_len, vm->hback_porch,
		vm->hfront_porch,
		vm->flags & DISPLAY_FLAGS_HSYNC_LOW ? "L" : "H");
	DBG(hdmi, "VSYNC vaw:%4d, vsw:%4d, vbp:%4d, vfp:%4d, pol:%s\n",
		vm->vactive, vm->vsync_len, vm->vback_porch,
		vm->vfront_porch,
		vm->flags & DISPLAY_FLAGS_VSYNC_LOW ? "L" : "H");
}

static int hdmi_get_preferred_sync(const struct nxs_dev *nxs_dev,
			struct nxs_control *control)
{
	struct nxs_hdmi *hdmi = nxs_to_hdmi(nxs_dev);
	struct nxs_control_syncinfo *sync = &control->u.sync_info;
	const struct hdmi_preferred_conf *preferred;
	struct videomode vm;
	int err;

	DBG(hdmi, "enter\n");

	hdmi_sync_to_vm(hdmi, sync, &vm);

	err = hdmi_find_preferred(hdmi, &vm, sync->fps);
	if (err < 0) {
		ERR(hdmi, "not support %d x %d %dhz!!!\n",
			vm.hactive, vm.vactive, sync->fps);
		return -EINVAL;
	}

	preferred = &hdmi->preferred[err];
	hdmi->phy_conf = preferred->conf;
	hdmi->pixelbits = 8;
	hdmi->tmds_clk_ratio = HDMI_TMDS_BIT_CLOCK_RATIO_1_10;
	hdmi->pattern = false;

	hdmi_vm_to_sync(hdmi, &preferred->vm, sync);

	DBG(hdmi, "%dhz done\n", hdmi->phy_conf->pixel_clock);

	return 0;
}

static int hdmi_set_sync(const struct nxs_dev *nxs_dev,
			const struct nxs_control *control)
{
	struct nxs_hdmi *hdmi = nxs_to_hdmi(nxs_dev);
	const struct nxs_control_syncinfo *sync = &control->u.sync_info;
	int pixelclock = sync->pixelclock;
	bool dvi_mode = sync->flags ? true : false;
	int err;

	DBG(hdmi, "%s\n", dvi_mode ? "dvi monitor" : "hdmi monitor");

	err = hdmi_find_phyconf(hdmi, pixelclock);
	if (err < 0) {
		ERR(hdmi, "not support %dhz !!!\n", pixelclock);
		return -EINVAL;
	}

	hdmi->phy_conf = hdmi->phy_confs_list[err];
	hdmi->dvi_mode = dvi_mode;
	hdmi->pixelbits = 8;
	hdmi->tmds_clk_ratio = HDMI_TMDS_BIT_CLOCK_RATIO_1_10;
	hdmi->pattern = false;

	hdmi_sync_to_vm(hdmi, sync, &hdmi->vm);

	DBG(hdmi, "%dhz done\n", hdmi->phy_conf->pixel_clock);

	return 0;
}

static int hdmi_get_valid(struct nxs_hdmi *hdmi,
			const struct nxs_control_syncinfo *sync)
{
	int pixelclock = sync->pixelclock;
	int err;

	DBG(hdmi, "pixelclock %d\n", pixelclock);

	err = hdmi_find_phyconf(hdmi, pixelclock);

	return err;
}

static int hdmi_get_status(const struct nxs_dev *nxs_dev,
			struct nxs_control *control)
{
	struct nxs_hdmi *hdmi = nxs_to_hdmi(nxs_dev);
	const struct nxs_control_syncinfo *sync = &control->u.sync_info;
	u32 type = control->type;

	switch (type) {
	case NXS_DISPLAY_STATUS_CONNECT:
		/*
		 * Enable the HPD interrupt after hdmi resource has been
	 	 * connected.
	 	 */
		hdmi_hpd_run(hdmi, true);
		return hdmi_is_connected(hdmi) ? 1 : 0;

	case NXS_DISPLAY_STATUS_MODE:
		return hdmi_get_valid(hdmi, sync);

	default:
		ERR(hdmi, "[%s:%d] unknown status type %d\n",
			__FNS(nxs_dev), __FNI(nxs_dev), type);
		return -EINVAL;
	}
	return 0;
}

static void hdmi_irq_enable(const struct nxs_dev *nxs_dev,
			enum nxs_event_type type, bool enable)
{
}

static u32 hdmi_irq_status(const struct nxs_dev *nxs_dev,
			enum nxs_event_type type)
{
	return 0;
}

static u32 hdmi_irq_pending(const struct nxs_dev *nxs_dev,
			enum nxs_event_type type)
{
	return 0;
}

static void hdmi_irq_done(const struct nxs_dev *nxs_dev,
			enum nxs_event_type type)
{
	struct nxs_hdmi *hdmi = nxs_to_hdmi(nxs_dev);

	hdmilink_v2_hpd_irq_done(hdmi);
}

static int hdmi_start(const struct nxs_dev *nxs_dev)
{
	struct nxs_hdmi *hdmi = nxs_to_hdmi(nxs_dev);
	const struct hdmi_phy_conf *phy_conf = hdmi->phy_conf;
	int pipe = 0;
	int ret;

	DBG(hdmi, "pipe.%d\n", pipe);

	if (!phy_conf) {
		ERR(hdmi, "not selected hdmi phy mode !!!\n");
		return -EINVAL;
	}

	hdmi_clk_reset(hdmi);

	ret = hdmi_phy_set(hdmi);
	if (ret)
		return ret;

	ret = hdmi_phy_wait_ready(hdmi);
	if (ret)
		return -EIO;

	hdmi_vps_set(hdmi);
	hdmi_video_run(hdmi);
	hdmi_audio_run(hdmi);
	hdmi_run(hdmi);

	DBG(hdmi, "done\n");
	return 0;
}

static int hdmi_set_dirty(const struct nxs_dev *nxs_dev, u32 type)
{
	struct nxs_hdmi_syncgen_reg *reg = nxs_to_hdmi(nxs_dev)->reg_hdmi;

	if (type != NXS_DEV_DIRTY_NORMAL)
		return 0;

	__write_bitw(&reg->syncgen.ctrl_0, 1, 12, 1);

	return 0;
}

static int hdmi_stop(const struct nxs_dev *nxs_dev)
{
	return 0;
}

static irqreturn_t hdmi_irq_handler(int irq, void *data)
{
	struct nxs_hdmi *hdmi = (struct nxs_hdmi *)data;
	struct nxs_dev *nxs_dev = &hdmi->nxs_dev;
	struct nxs_irq_callback *cb;

	list_for_each_entry(cb, &nxs_dev->irq_callback, list) {
		if (cb) {
			if (irq == hdmi->hpd_irq && cb->type == HDMI_IRQ_HPD)
				cb->handler(nxs_dev, cb->data);
			else if (irq == hdmi->irq && cb->type == HDMI_IRQ_UPDATE)
				cb->handler(nxs_dev, cb->data);
			else
				pr_err("ERROR: no hdmi irq callback !!!\n");
		}
	}

	return IRQ_HANDLED;
}

static int hdmi_open(const struct nxs_dev *nxs_dev)
{
	DBG(nxs_to_hdmi(nxs_dev), "enter\n");
	return 0;
}

static int hdmi_close(const struct nxs_dev *nxs_dev)
{
	struct nxs_hdmi *hdmi = nxs_to_hdmi(nxs_dev);

	/* hdmi plug detect on */
	hdmi_hpd_run(hdmi, false);

	return 0;
}

static inline void __iomem *hdmi_resource_base(struct nxs_hdmi *hdmi, int id)
{
	struct nxs_dev *nxs_dev = &hdmi->nxs_dev;
	struct device *dev = nxs_dev->dev;
	struct device_node *node = dev->of_node;
	void __iomem *base;
	struct resource r;
	int size, ret;

	ret = of_address_to_resource(node, id, &r);
	if (ret) {
		pr_err("[%s:%d] failed to get resource.%d base address\n",
			__FNS(nxs_dev), __FNI(nxs_dev), id);
		return NULL;
	}
	size = resource_size(&r);
#if 0
	base = devm_ioremap_nocache(dev, r.start, size);
	if (!base) {
		pr_err("[%s:%d] failed to ioremap for [0x%x,0x%x]\n",
			__FNS(nxs_dev), __FNI(nxs_dev), r.start, size);
		return NULL;
	}
#else
	base = ioremap(r.start, size);
	if (!base) {
		pr_err("[%s:%d] failed to ioremap for [0x%x,0x%x]\n",
			__FNS(nxs_dev), __FNI(nxs_dev), r.start, size);
		return NULL;
	}
#endif
	DBG(hdmi, "[%s:%d] map 0x%x,0x%x -> %p\n",
		__FNS(nxs_dev), __FNI(nxs_dev), r.start, size, base);

	return base;
}

static int hdmi_probe_setup(struct platform_device *pdev, struct nxs_hdmi *hdmi)
{
	struct nxs_dev *nxs_dev = &hdmi->nxs_dev;
	const char *strings[2];
	int i, size, ret;

	nxs_dev->dev = &pdev->dev;

	hdmi->reg_hdmi = hdmi_resource_base(hdmi, 0);
	if (!hdmi->reg_hdmi)
		return -EINVAL;

	hdmi->reg_link = hdmi_resource_base(hdmi, 1);
	if (!hdmi->reg_link)
		return -EINVAL;

	hdmi->reg_phy = hdmi_resource_base(hdmi, 2);
	if (!hdmi->reg_phy)
		return -EINVAL;

	hdmi->reg_cec = hdmi_resource_base(hdmi, 3);
	if (!hdmi->reg_cec)
		return -EINVAL;

	hdmi->reg_tieoff = hdmi_resource_base(hdmi, 4);
	if (!hdmi->reg_tieoff)
		return -EINVAL;

	hdmi->phy_confs_list = hdmi2_phy_conf;
	hdmi->phy_confs_num = ARRAY_SIZE(hdmi2_phy_conf);
	hdmi->preferred = hdmi_preferred_confs;
	hdmi->preferred_num = ARRAY_SIZE(hdmi_preferred_confs);

	hdmi_clk_reset_init(hdmi);

	/* HDMI Interrupts */
	size = of_property_read_string_array(pdev->dev.of_node,
			"interrupts-names", strings, ARRAY_SIZE(strings));
	if (size > ARRAY_SIZE(strings))
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(strings); i++) {
		const char *c = strings[i];
		int irq = platform_get_irq(pdev, i);

		if (irq < 0) {
			dev_err(&pdev->dev,
				"failed to get irq for %s\n", c);
			return -EINVAL;
		}

		ret = request_irq(irq, hdmi_irq_handler,
				  IRQF_TRIGGER_NONE, "nxs-hdmi", hdmi);
		if (ret < 0) {
			dev_err(&pdev->dev,
				"failed to request irq.%d (%s)\n",
				irq, c);
			return ret;
		}

		if (!strcasecmp(strings[i], "hpd"))
			hdmi->hpd_irq = irq;

		if (!strcasecmp(strings[i], "syncgen"))
			hdmi->irq = irq;
	}

	return 0;
}

static int hdmi_stream_register(struct nxs_hdmi *hdmi)
{
	struct nxs_dev *nxs_dev = &hdmi->nxs_dev;
	int ret;

	nxs_dev->open = hdmi_open;
	nxs_dev->close = hdmi_close;
	nxs_dev->start = hdmi_start;
	nxs_dev->stop = hdmi_stop;
	nxs_dev->set_dirty = hdmi_set_dirty;
	nxs_dev->set_control = nxs_set_control;
	nxs_dev->get_control = nxs_get_control;

	nxs_dev->irq = hdmi->hpd_irq;
	nxs_dev->set_interrupt_enable = hdmi_irq_enable;
	nxs_dev->get_interrupt_enable = hdmi_irq_status;
	nxs_dev->get_interrupt_pending = hdmi_irq_pending;
	nxs_dev->clear_interrupt_pending = hdmi_irq_done;

	nxs_dev->dev_services[0].type = NXS_CONTROL_SYNCINFO;
	nxs_dev->dev_services[0].set_control = hdmi_set_sync;
	nxs_dev->dev_services[0].get_control = hdmi_get_preferred_sync;

	nxs_dev->dev_services[1].type = NXS_CONTROL_STATUS;
	nxs_dev->dev_services[1].get_control = hdmi_get_status;

	ret = register_nxs_dev(nxs_dev);
	if (ret)
		return ret;

	return ret;
}

static int hdmi_probe(struct platform_device *pdev)
{
	struct nxs_hdmi *hdmi;
	int ret;

	hdmi = devm_kzalloc(&pdev->dev, sizeof(*hdmi), GFP_KERNEL);
	if (!hdmi)
		return -ENOMEM;

	ret = nxs_dev_parse_dt(pdev, &hdmi->nxs_dev);
	if (ret)
		return ret;

	ret = hdmi_probe_setup(pdev, hdmi);
	if (ret)
		return ret;

	ret = hdmi_stream_register(hdmi);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, hdmi);

	return 0;
}

static int hdmi_remove(struct platform_device *pdev)
{
	struct nxs_hdmi *hdmi = platform_get_drvdata(pdev);

	if (hdmi)
		unregister_nxs_dev(&hdmi->nxs_dev);

	return 0;
}

static const struct of_device_id nxs_hdmi_match[] = {
	{ .compatible = "nexell,hdmi-nxs-1.0", },
	{},
};

static struct platform_driver nxs_hdmi_driver = {
	.probe	= hdmi_probe,
	.remove	= hdmi_remove,
	.driver	= {
		.name = "nxs-hdmi",
		.of_match_table = of_match_ptr(nxs_hdmi_match),
	},
};

static int __init nxs_hdmi_init(void)
{
	return platform_driver_register(&nxs_hdmi_driver);
}
/* subsys_initcall(nxs_hdmi_init); */
fs_initcall(nxs_hdmi_init);

static void __exit nxs_hdmi_exit(void)
{
	platform_driver_unregister(&nxs_hdmi_driver);
}
module_exit(nxs_hdmi_exit);

MODULE_DESCRIPTION("Nexell Stream HDMI driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");
