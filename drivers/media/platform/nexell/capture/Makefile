obj-$(CONFIG_VIDEO_NEXELL_CAPTURE)	+= nx-vip-primitive.o nx-vip.o nx-csi.o nx-clipper.o
obj-$(CONFIG_VIDEO_NEXELL_DECIMATOR)    += nx-decimator.o
obj-$(CONFIG_VIDEO_NEXELL_REARCAM)	+= nx-rearcam.o

ifeq ($(CONFIG_VIDEO_NEXELL_REARCAM_VENDOR_FMS), y)
	obj-$(CONFIG_VIDEO_NEXELL_REARCAM)	+= nx-rearcam-vendor-fms.o
else
	obj-$(CONFIG_VIDEO_NEXELL_REARCAM)	+= nx-rearcam-vendor.o
endif
