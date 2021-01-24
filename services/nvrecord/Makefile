cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))

mmlib_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

PLAT_LIB_NAME := nv_record

ifeq ($(ROM_UTILS_ON),1)
PLAT_LIB_NAME := $(PLAT_LIB_NAME)_v12
endif

$(PLAT_LIB_NAME)-y := $(mmlib_obj)

rel_src_obj :=

obj-y := $(PLAT_LIB_NAME).a $(rel_src_obj)


ccflags-y += -Iutils/hexdump    \
        -Iapps/key  \
        -Iservices/bt_app   \
        -Iservices/bt_if/inc  \
        -Iutils/crc32   \
        -Iplatform/drivers/ana \
        -Iservices/nv_section/customparam_section \
        -Iservices/ble_stack/common/api \
        -Iservices/ble_stack/ble_ip \
        -Iutils/heap \
        -Iservices/norflash_api

CFLAGS_nvrecord.o += -DAUDIO_OUTPUT_VOLUME_DEFAULT=$(AUDIO_OUTPUT_VOLUME_DEFAULT)

ifeq ($(FLASH_SUSPEND),1)
CFLAGS_nvrecord.o += -DFLASH_SUSPEND
endif

