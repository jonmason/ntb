obj-y += tieoff_init.o
obj-$(CONFIG_SOC_S5P6818_GPIO)		+= s5p6818-gpio.o
ifeq ($(CONFIG_ARM64),y)
obj-y += platsmp_64.o
obj-$(CONFIG_SMP) += headsmp_64.o
endif
