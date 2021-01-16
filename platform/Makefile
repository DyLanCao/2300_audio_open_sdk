obj-y := cmsis/ drivers/ hal/
mmlib_obj := cmsis/ drivers/ hal/

ifneq ($(DUAL_BOOT),1)
obj-y += main/
mmlib_obj += main/
endif

subdir-ccflags-y += -Iplatform/cmsis/inc -Iplatform/hal -Iutils/hwtimer_list

PLAT_LIB_NAME := whale_plarform

ifeq ($(ROM_UTILS_ON),1)
PLAT_LIB_NAME := $(PLAT_LIB_NAME)_v12
endif

$(PLAT_LIB_NAME)-y := $(mmlib_obj)

obj-y := $(PLAT_LIB_NAME).a $(rel_src_obj)
