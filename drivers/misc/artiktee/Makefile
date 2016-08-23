MODULE_NAME := tzdev
ccflags-y := -DCONFIG_SECOS_NO_RPMB=1 -DCONFIG_ARM_PSCI=1 -DCONFIG_INSTANCE_DEBUG=1

obj-$(CONFIG_ARTIK_TRUSTZONE_DRIVER) += $(MODULE_NAME).o

$(MODULE_NAME)-objs += tzdev_main.o
$(MODULE_NAME)-objs += tzlog_print.o
$(MODULE_NAME)-objs += tzlog_core.o
$(MODULE_NAME)-objs += tzlog_process.o
$(MODULE_NAME)-objs += nsrpc_ree_slave.o
$(MODULE_NAME)-objs += tzpage.o
$(MODULE_NAME)-objs += ssdev_core.o
$(MODULE_NAME)-objs += ssdev_init.o
$(MODULE_NAME)-objs += ssdev_file.o
$(MODULE_NAME)-objs += ssdev_rpmb.o
$(MODULE_NAME)-objs += dumpdev_core.o
$(MODULE_NAME)-objs += tzsys.o
$(MODULE_NAME)-objs += tzdev_smc.o
$(MODULE_NAME)-objs += tzlinkmgr.o
$(MODULE_NAME)-objs += tzwsm.o
$(MODULE_NAME)-objs += tzmem.o
$(MODULE_NAME)-objs += tzdev_plat.o
