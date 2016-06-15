# File: Makefile
#
# Copyright (C) 2008-2016, Marvell International Ltd.
#
# This software file (the "File") is distributed by Marvell International
# Ltd. under the terms of the GNU General Public License Version 2, June 1991
# (the "License").  You may use, redistribute and/or modify this File in
# accordance with the terms and conditions of the License, a copy of which
# is available by writing to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
# worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
#
# A copy of the GPL is available in file gpl-2.0.txt accompanying in this
# deliverables.
#
# THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
# IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
# ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
# this warranty disclaimer.

COMPATDIR=/lib/modules/$(KERNELVERSION_X86)/build/compat-wireless-3.2-rc1-1/include
CC=		$(CROSS_COMPILE)gcc -I$(COMPATDIR)
LD=		$(CROSS_COMPILE)ld
BACKUP=		/root/backup
YMD=		`date +%Y%m%d%H%M`

#############################################################################
# Configuration Options
#############################################################################

# Debug Option
# DEBUG LEVEL n/1/2:
# n: NO DEBUG
# 1: Only PRINTM(MMSG,...), PRINTM(MFATAL,...), ...
# 2: All PRINTM()
CONFIG_DEBUG=1

# Proc debug file
CONFIG_PROC_DEBUG=y

# Enable STA mode support
CONFIG_STA_SUPPORT=y

# Enable uAP mode support
CONFIG_UAP_SUPPORT=y

# Enable WIFIDIRECT support
CONFIG_WIFI_DIRECT_SUPPORT=y

# Enable WIFIDISPLAY support
CONFIG_WIFI_DISPLAY_SUPPORT=y

# Re-association in driver
CONFIG_REASSOCIATION=y


# Manufacturing firmware support
CONFIG_MFG_CMD_SUPPORT=y

# Big-endian platform
CONFIG_BIG_ENDIAN=n

# Enable driver based authenticator
CONFIG_DRV_EMBEDDED_AUTHENTICATOR=y


# Enable driver based supplicant
CONFIG_DRV_EMBEDDED_SUPPLICANT=y

ifeq ($(CONFIG_DRV_EMBEDDED_SUPPLICANT), y)
CONFIG_EMBEDDED_SUPP_AUTH=y
else
ifeq ($(CONFIG_DRV_EMBEDDED_AUTHENTICATOR), y)
CONFIG_EMBEDDED_SUPP_AUTH=y
endif
endif

# Enable SDIO multi-port Tx aggregation
CONFIG_SDIO_MULTI_PORT_TX_AGGR=y

# Enable SDIO multi-port Rx aggregation
CONFIG_SDIO_MULTI_PORT_RX_AGGR=y

# SDIO suspend/resume
CONFIG_SDIO_SUSPEND_RESUME=y

# DFS testing support
CONFIG_DFS_TESTING_SUPPORT=y

# Multi-channel support
CONFIG_MULTI_CHAN_SUPPORT=y




#32bit app over 64bit kernel support
CONFIG_USERSPACE_32BIT_OVER_KERNEL_64BIT=n


#############################################################################
# Select Platform Tools
#############################################################################

MODEXT = ko
EXTRA_CFLAGS += -I./drivers/net/wireless/sd8977/wlan_src/mlan
EXTRA_CFLAGS += -DLINUX


ifeq ($(CONFIG_EMBEDDED_SUPP_AUTH), y)
EXTRA_CFLAGS += -I./drivers/net/wireless/sd8977/wlan_src/mlan/esa
EXTRA_CFLAGS += -I./drivers/net/wireless/sd8977/wlan_src/mlan/esa/common
endif


KERNELVERSION_X86 := 	$(shell uname -r)
KERNELDIR ?= /lib/modules/$(KERNELVERSION_X86)/build
LD += -S

BINDIR = ../bin_sd8xxx
APPDIR= $(shell if test -d "mapp"; then echo mapp; fi)

#############################################################################
# Compiler Flags
#############################################################################

	EXTRA_CFLAGS += -I$(KERNELDIR)/include

	EXTRA_CFLAGS += -DFPNUM='"68"'

ifeq ($(CONFIG_DEBUG),1)
	EXTRA_CFLAGS += -DDEBUG_LEVEL1
endif

ifeq ($(CONFIG_DEBUG),2)
	EXTRA_CFLAGS += -DDEBUG_LEVEL1
	EXTRA_CFLAGS += -DDEBUG_LEVEL2
	DBG=	-dbg
endif

ifeq ($(CONFIG_PROC_DEBUG),y)
	EXTRA_CFLAGS += -DPROC_DEBUG
	export CONFIG_PROC_DEBUG
endif

ifeq ($(CONFIG_64BIT), y)
	EXTRA_CFLAGS += -DMLAN_64BIT
endif

ifeq ($(CONFIG_STA_SUPPORT),y)
	EXTRA_CFLAGS += -DSTA_SUPPORT
ifeq ($(CONFIG_REASSOCIATION),y)
	EXTRA_CFLAGS += -DREASSOCIATION
endif
else
CONFIG_WIFI_DIRECT_SUPPORT=n
CONFIG_WIFI_DISPLAY_SUPPORT=n
CONFIG_STA_WEXT=n
CONFIG_STA_CFG80211=n
endif

ifeq ($(CONFIG_UAP_SUPPORT),y)
	EXTRA_CFLAGS += -DUAP_SUPPORT
else
CONFIG_WIFI_DIRECT_SUPPORT=n
CONFIG_WIFI_DISPLAY_SUPPORT=n
CONFIG_UAP_WEXT=n
CONFIG_UAP_CFG80211=n
endif

ifeq ($(CONFIG_WIFI_DIRECT_SUPPORT),y)
	EXTRA_CFLAGS += -DWIFI_DIRECT_SUPPORT
endif
ifeq ($(CONFIG_WIFI_DISPLAY_SUPPORT),y)
	EXTRA_CFLAGS += -DWIFI_DISPLAY_SUPPORT
endif

ifeq ($(CONFIG_MFG_CMD_SUPPORT),y)
	EXTRA_CFLAGS += -DMFG_CMD_SUPPORT
endif

ifeq ($(CONFIG_BIG_ENDIAN),y)
	EXTRA_CFLAGS += -DBIG_ENDIAN_SUPPORT
endif

ifeq ($(CONFIG_USERSPACE_32BIT_OVER_KERNEL_64BIT),y)
	EXTRA_CFLAGS += -DUSERSPACE_32BIT_OVER_KERNEL_64BIT
endif

ifeq ($(CONFIG_SDIO_MULTI_PORT_TX_AGGR),y)
	EXTRA_CFLAGS += -DSDIO_MULTI_PORT_TX_AGGR
endif

ifeq ($(CONFIG_SDIO_MULTI_PORT_RX_AGGR),y)
	EXTRA_CFLAGS += -DSDIO_MULTI_PORT_RX_AGGR
endif

ifeq ($(CONFIG_SDIO_SUSPEND_RESUME),y)
	EXTRA_CFLAGS += -DSDIO_SUSPEND_RESUME
endif

ifeq ($(CONFIG_MULTI_CHAN_SUPPORT),y)
	EXTRA_CFLAGS += -DMULTI_CHAN_SUPPORT
endif

ifeq ($(CONFIG_DFS_TESTING_SUPPORT),y)
	EXTRA_CFLAGS += -DDFS_TESTING_SUPPORT
endif


ifeq ($(CONFIG_ANDROID_KERNEL), y)
	EXTRA_CFLAGS += -DANDROID_KERNEL
endif

ifeq ($(CONFIG_T50), y)
	EXTRA_CFLAGS += -DT50
	EXTRA_CFLAGS += -DT40
	EXTRA_CFLAGS += -DT3T
endif

# add -Wno-packed-bitfield-compat when GCC version greater than 4.4
GCC_VERSION := $(shell echo `gcc -dumpversion | cut -f1-2 -d.` \>= 4.4 | sed -e 's/\./*100+/g' | bc )
ifeq ($(GCC_VERSION),1)
	EXTRA_CFLAGS += -Wno-packed-bitfield-compat
endif

ifeq ($(CONFIG_MRVL_SDIO_OOB_IRQ),y)
	EXTRA_CFLAGS += -DSDIO_OOB_IRQ
endif

#############################################################################
# Make Targets
#############################################################################

ifneq ($(KERNELRELEASE),)

ifeq ($(CONFIG_WIRELESS_EXT),y)
ifeq ($(CONFIG_WEXT_PRIV),y)
	# Enable WEXT for STA
	CONFIG_STA_WEXT=y
	# Enable WEXT for uAP
	CONFIG_UAP_WEXT=y
else
# Disable WEXT for STA
	CONFIG_STA_WEXT=n
# Disable WEXT for uAP
	CONFIG_UAP_WEXT=n
endif
endif

# Enable CFG80211 for STA
ifeq ($(CONFIG_CFG80211),y)
	CONFIG_STA_CFG80211=y
else
ifeq ($(CONFIG_CFG80211),m)
	CONFIG_STA_CFG80211=y
else
	CONFIG_STA_CFG80211=n
endif
endif

# Enable CFG80211 for uAP
ifeq ($(CONFIG_CFG80211),y)
	CONFIG_UAP_CFG80211=y
else
ifeq ($(CONFIG_CFG80211),m)
	CONFIG_UAP_CFG80211=y
else
	CONFIG_UAP_CFG80211=n
endif
endif

ifneq ($(CONFIG_STA_SUPPORT),y)
	CONFIG_WIFI_DIRECT_SUPPORT=n
	CONFIG_WIFI_DISPLAY_SUPPORT=n
	CONFIG_STA_WEXT=n
	CONFIG_STA_CFG80211=n
endif

ifneq ($(CONFIG_UAP_SUPPORT),y)
	CONFIG_WIFI_DIRECT_SUPPORT=n
	CONFIG_WIFI_DISPLAY_SUPPORT=n
	CONFIG_UAP_WEXT=n
	CONFIG_UAP_CFG80211=n
endif

ifeq ($(CONFIG_STA_SUPPORT),y)
ifeq ($(CONFIG_STA_WEXT),y)
	EXTRA_CFLAGS += -DSTA_WEXT
endif
ifeq ($(CONFIG_STA_CFG80211),y)
	EXTRA_CFLAGS += -DSTA_CFG80211
endif
endif
ifeq ($(CONFIG_UAP_SUPPORT),y)
ifeq ($(CONFIG_UAP_WEXT),y)
	EXTRA_CFLAGS += -DUAP_WEXT
endif
ifeq ($(CONFIG_UAP_CFG80211),y)
	EXTRA_CFLAGS += -DUAP_CFG80211
endif
endif

print:
ifeq ($(CONFIG_STA_SUPPORT),y)
ifeq ($(CONFIG_STA_WEXT),n)
ifeq ($(CONFIG_STA_CFG80211),n)
	@echo "Can not build STA without WEXT or CFG80211"
	exit 2
endif
endif
endif
ifeq ($(CONFIG_UAP_SUPPORT),y)
ifeq ($(CONFIG_UAP_WEXT),n)
ifeq ($(CONFIG_UAP_CFG80211),n)
	@echo "Can not build UAP without WEXT or CFG80211"
	exit 2
endif
endif
endif


ifeq ($(CONFIG_DRV_EMBEDDED_AUTHENTICATOR), y)
    EXTRA_CFLAGS += -DDRV_EMBEDDED_AUTHENTICATOR
endif

ifeq ($(CONFIG_DRV_EMBEDDED_SUPPLICANT), y)
    EXTRA_CFLAGS += -DDRV_EMBEDDED_SUPPLICANT
endif


MOALOBJS =	mlinux/moal_main.o \
		mlinux/moal_ioctl.o \
		mlinux/moal_shim.o \
		mlinux/moal_eth_ioctl.o

MLANOBJS =	mlan/mlan_shim.o mlan/mlan_init.o \
		mlan/mlan_txrx.o \
		mlan/mlan_cmdevt.o mlan/mlan_misc.o \
		mlan/mlan_cfp.o \
		mlan/mlan_module.o

MLANOBJS += mlan/mlan_wmm.o
MLANOBJS += mlan/mlan_sdio.o
MLANOBJS += mlan/mlan_11n_aggr.o
MLANOBJS += mlan/mlan_11n_rxreorder.o
MLANOBJS += mlan/mlan_11n.o
MLANOBJS += mlan/mlan_11ac.o
MLANOBJS += mlan/mlan_11d.o
MLANOBJS += mlan/mlan_11h.o
ifeq ($(CONFIG_STA_SUPPORT),y)
MLANOBJS += mlan/mlan_meas.o
MLANOBJS += mlan/mlan_scan.o \
			mlan/mlan_sta_ioctl.o \
			mlan/mlan_sta_rx.o \
			mlan/mlan_sta_tx.o \
			mlan/mlan_sta_event.o \
			mlan/mlan_sta_cmd.o \
			mlan/mlan_sta_cmdresp.o \
			mlan/mlan_join.o
ifeq ($(CONFIG_STA_WEXT),y)
MOALOBJS += mlinux/moal_priv.o \
            mlinux/moal_wext.o
endif
endif
ifeq ($(CONFIG_UAP_SUPPORT),y)
MLANOBJS += mlan/mlan_uap_ioctl.o
MLANOBJS += mlan/mlan_uap_cmdevent.o
MLANOBJS += mlan/mlan_uap_txrx.o
MOALOBJS += mlinux/moal_uap.o
ifeq ($(CONFIG_UAP_WEXT),y)
MOALOBJS += mlinux/moal_uap_priv.o
MOALOBJS += mlinux/moal_uap_wext.o
endif
endif
ifeq ($(CONFIG_STA_CFG80211),y)
MOALOBJS += mlinux/moal_cfg80211.o
MOALOBJS += mlinux/moal_cfgvendor.o
MOALOBJS += mlinux/moal_sta_cfg80211.o
endif
ifeq ($(CONFIG_UAP_CFG80211),y)
MOALOBJS += mlinux/moal_cfg80211.o
MOALOBJS += mlinux/moal_cfgvendor.o
MOALOBJS += mlinux/moal_uap_cfg80211.o
endif

ifdef CONFIG_PROC_FS
MOALOBJS += mlinux/moal_proc.o
ifeq ($(CONFIG_PROC_DEBUG),y)
MOALOBJS += mlinux/moal_debug.o
endif
endif



ifeq ($(CONFIG_EMBEDDED_SUPP_AUTH), y)
MLANOBJS += mlan/esa/common/rc4.o \
			mlan/esa/common/aes_cmac_rom.o \
			mlan/esa/common/hmac_sha1.o \
			mlan/esa/common/md5.o \
			mlan/esa/common/mrvl_sha256_crypto.o \
			mlan/esa/common/hmac_md5.o \
			mlan/esa/common/crypt_new_rom.o \
			mlan/esa/common/rijndael.o \
			mlan/esa/common/sha1.o \
			mlan/esa/common/sha256.o \
			mlan/esa/common/pass_phrase.o \
			mlan/esa/common/pmkCache.o \
			mlan/esa/common/pmkCache_rom.o \
			mlan/esa/common/parser.o \
			mlan/esa/common/parser_rom.o \
			mlan/esa/keyMgmtApStaCommon.o \
			mlan/esa/hostsa_init.o \
			mlan/esa/authenticator_api.o
endif


ifeq ($(CONFIG_DRV_EMBEDDED_SUPPLICANT),y)
MLANOBJS += mlan/esa/keyMgmtSta.o \
			mlan/esa/keyMgmtSta_rom.o \
			mlan/esa/supplicant.o
endif

ifeq ($(CONFIG_DRV_EMBEDDED_AUTHENTICATOR),y)
MLANOBJS += mlan/esa/AssocAp_srv_rom.o \
			mlan/esa/keyMgmtAp_rom.o \
			mlan/esa/keyMgmtAp.o
endif

obj-$(CONFIG_MRVL_WLAN_DRIVER) := mlan.o
mlan-objs := $(MLANOBJS)
MOALOBJS += mlinux/moal_sdio_mmc.o
obj-$(CONFIG_MRVL_WLAN_DRIVER) += sd8xxx.o
sd8xxx-objs := $(MOALOBJS)

# Otherwise we were called directly from the command line; invoke the kernel build system.
else

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

###############################################################

export		CC LD EXTRA_CFLAGS KERNELDIR

ifeq ($(CONFIG_STA_SUPPORT),y)
ifeq ($(CONFIG_UAP_SUPPORT),y)
.PHONY: mapp/mlanconfig mapp/mlan2040coex mapp/mlanevent mapp/uaputl mapp/mlanutl clean distclean
else
.PHONY: mapp/mlanconfig mapp/mlanevent mapp/mlan2040coex mapp/mlanutl clean distclean
endif
else
ifeq ($(CONFIG_UAP_SUPPORT),y)
.PHONY: mapp/mlanevent mapp/uaputl clean distclean
endif
endif
	@echo "Finished Making Marvell Wlan Linux Driver"

ifeq ($(CONFIG_STA_SUPPORT),y)
mapp/mlanconfig:
	$(MAKE) -C $@
mapp/mlanutl:
	$(MAKE) -C $@
mapp/mlan2040coex:
	$(MAKE) -C $@
endif
ifeq ($(CONFIG_UAP_SUPPORT),y)
mapp/uaputl:
	$(MAKE) -C $@
endif
ifeq ($(CONFIG_WIFI_DIRECT_SUPPORT),y)
mapp/wifidirectutl:
	$(MAKE) -C $@
endif
mapp/mlanevent:
	$(MAKE) -C $@

echo:

build:		echo default

	@if [ ! -d $(BINDIR) ]; then \
		mkdir $(BINDIR); \
	fi

	cp -f mlan.$(MODEXT) $(BINDIR)/mlan$(DBG).$(MODEXT)
	cp -f sd8xxx.$(MODEXT) $(BINDIR)/sd8xxx$(DBG).$(MODEXT)
	cp -rpf script/sdio_mmc/* $(BINDIR)/

ifeq ($(CONFIG_STA_SUPPORT),y)
	cp -f README $(BINDIR)
	cp -f README_MLAN $(BINDIR)
	cp -f README_RBC $(BINDIR)
ifneq ($(APPDIR),)
	$(MAKE) -C mapp/mlanconfig $@ INSTALLDIR=$(BINDIR)
	$(MAKE) -C mapp/mlanutl $@ INSTALLDIR=$(BINDIR)
	$(MAKE) -C mapp/mlan2040coex $@ INSTALLDIR=$(BINDIR)
endif
endif
ifeq ($(CONFIG_UAP_SUPPORT),y)
	cp -f README_UAP $(BINDIR)
ifneq ($(APPDIR),)
	$(MAKE) -C mapp/uaputl $@ INSTALLDIR=$(BINDIR)
endif
endif
ifeq ($(CONFIG_WIFI_DIRECT_SUPPORT),y)
	cp -f README_WIFIDIRECT $(BINDIR)
	cp -rpf script/wifidirect $(BINDIR)
ifeq ($(CONFIG_WIFI_DISPLAY_SUPPORT),y)
	cp -rpf script/wifidisplay $(BINDIR)
endif
ifneq ($(APPDIR),)
	$(MAKE) -C mapp/wifidirectutl $@ INSTALLDIR=$(BINDIR)
endif
endif
ifneq ($(APPDIR),)
	$(MAKE) -C mapp/mlanevent $@ INSTALLDIR=$(BINDIR)
endif

clean:
	-find . -name "*.o" -exec rm {} \;
	-find . -name "*.ko" -exec rm {} \;
	-find . -name ".*.cmd" -exec rm {} \;
	-find . -name "*.mod.c" -exec rm {} \;
	-find . -name "Module.symvers" -exec rm {} \;
	-find . -name "Module.markers" -exec rm {} \;
	-find . -name "modules.order" -exec rm {} \;
	-rm -rf .tmp_versions
ifneq ($(APPDIR),)
ifeq ($(CONFIG_STA_SUPPORT),y)
	$(MAKE) -C mapp/mlanconfig $@
	$(MAKE) -C mapp/mlanutl $@
	$(MAKE) -C mapp/mlan2040coex $@
endif
ifeq ($(CONFIG_UAP_SUPPORT),y)
	$(MAKE) -C mapp/uaputl $@
endif
ifeq ($(CONFIG_WIFI_DIRECT_SUPPORT),y)
	$(MAKE) -C mapp/wifidirectutl $@
endif
	$(MAKE) -C mapp/mlanevent $@
endif

install: default

	cp -f mlan.$(MODEXT) $(INSTALLDIR)/mlan$(DBG).$(MODEXT)
	cp -f ../io/sdio/$(PLATFORM)/sdio.$(MODEXT) $(INSTALLDIR)

distclean:
	-find . -name "*.o" -exec rm {} \;
	-find . -name "*.orig" -exec rm {} \;
	-find . -name "*.swp" -exec rm {} \;
	-find . -name "*.*~" -exec rm {} \;
	-find . -name "*~" -exec rm {} \;
	-find . -name "*.d" -exec rm {} \;
	-find . -name "*.a" -exec rm {} \;
	-find . -name "tags" -exec rm {} \;
	-find . -name ".*" -exec rm -rf 2> /dev/null \;
	-find . -name "*.ko" -exec rm {} \;
	-find . -name ".*.cmd" -exec rm {} \;
	-find . -name "*.mod.c" -exec rm {} \;
	-rm -rf .tmp_versions
ifneq ($(APPDIR),)
ifeq ($(CONFIG_STA_SUPPORT),y)
	$(MAKE) -C mapp/mlanconfig $@
	$(MAKE) -C mapp/mlanutl $@
	$(MAKE) -C mapp/mlan2040coex $@
endif
ifeq ($(CONFIG_UAP_SUPPORT),y)
	$(MAKE) -C mapp/uaputl $@
endif
ifeq ($(CONFIG_WIFI_DIRECT_SUPPORT),y)
	$(MAKE) -C mapp/wifidirectutl $@
endif
	$(MAKE) -C mapp/mlanevent $@
endif

# End of file
