include $(srctree)/drivers/misc/mediatek/Makefile.custom

#ccflags-y += -I$(MTK_CUSTOM_PATH)/touchpanel/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/GT9XX/
#ccflags-y += -I$(MTK_CUSTOM_PATH)/touchpanel/

ccflags-y += -I$(MTK_PROJECT_PATH_ROOT)/touchpanel/ft5446i/

# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/ft5446i/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/


ifeq ($(CONFIG_MTK_PLATFORM),"mt6572")
    ccflags-y += -DMT6572
endif

obj-y	+=  focaltech_ctl.o
obj-y	+=  ft6x06_button.o
obj-y	+=  ft6x06_driver.o
obj-y	+=  ft6x06_iic_wrap.o
obj-y	+=  ft6x06_isp.o

