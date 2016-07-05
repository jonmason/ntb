MODULE_NAME := tzdev
ccflags-y := -DCONFIG_SECOS_NO_RPMB=1 -DCONFIG_PSCI=1
ifeq ($(CONFIG_ARTIK_FETCH_TEE_INFO),y)
ccflags-y += -DCONFIG_FETCH_TEE_INFO=1
endif

obj-$(CONFIG_ARTIK_TRUSTZONE_DRIVER) += $(MODULE_NAME).o

$(MODULE_NAME)-objs += tzdev_main.o
$(MODULE_NAME)-objs += tzlog_print.o
$(MODULE_NAME)-objs += tzlog_core.o
$(MODULE_NAME)-objs += tzlog_process.o
$(MODULE_NAME)-objs += sstransaction.o
$(MODULE_NAME)-objs += tzpage.o
$(MODULE_NAME)-objs += ss_core.o
$(MODULE_NAME)-objs += ss_dev.o
$(MODULE_NAME)-objs += ss_file.o
$(MODULE_NAME)-objs += ss_rpmb.o
$(MODULE_NAME)-objs += tzsys.o
$(MODULE_NAME)-objs += tzdev_smc.o
$(MODULE_NAME)-objs += tzlinkmgr.o
$(MODULE_NAME)-objs += tzwsm.o
$(MODULE_NAME)-objs += tzmem.o
$(MODULE_NAME)-objs += tzdev_plat.o
ifeq ($(CONFIG_ARTIK_FETCH_TEE_INFO),y)
$(MODULE_NAME)-objs += tzinfo.o
endif
