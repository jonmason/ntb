#
# Makefile for the drm device driver.  This driver provides support for the
# Direct Rendering Infrastructure (DRI) in XFree86 4.1.0 and higher.

obj-y += s5pxx18_drm_dp.o s5pxx18_dp_dev.o	\
		s5pxx18_soc_mlc.o s5pxx18_soc_dpc.o s5pxx18_soc_disptop.o	\
		s5pxx18_soc_disptop_clk.o

obj-$(CONFIG_DRM_NX_RGB) += s5pxx18_dp_rgb.o
obj-$(CONFIG_DRM_NX_LVDS) += s5pxx18_soc_lvds.o s5pxx18_dp_lvds.o
obj-$(CONFIG_DRM_NX_MIPI_DSI) += s5pxx18_soc_mipi.o s5pxx18_dp_mipi.o
obj-$(CONFIG_DRM_NX_HDMI) += s5pxx18_hdmi_presets.o s5pxx18_reg_hdmi.o s5pxx18_dp_hdmi.o


