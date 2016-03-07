#
# Makefile for the drm device driver.  This driver provides support for the
# Direct Rendering Infrastructure (DRI) in XFree86 4.1.0 and higher.

ccflags-y := -Iinclude/drm -Idrivers/gpu/drm/nexell
nx_drm-y  := nx_drm_drv.o nx_drm_connector.o nx_drm_encoder.o \
			nx_drm_crtc.o nx_drm_fbdev.o nx_drm_fb.o \
			nx_drm_plane.o  nx_drm_gem.o

obj-$(CONFIG_DRM_NX)			+= soc/
obj-$(CONFIG_NX_DISPLAY_LCD)	+= nx_drm_lcd.o
obj-$(CONFIG_DRM_NX)			+= nx_drm.o
