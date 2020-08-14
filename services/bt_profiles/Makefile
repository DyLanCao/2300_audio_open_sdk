
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)hcitrans/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)hcitrans/modinit/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)besport/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)profiles/a2dp/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)profiles/avctp/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)profiles/avdtp/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)profiles/avrcp/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)profiles/common/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)profiles/dip/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)profiles/hf/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)profiles/hsp/*.c))
#obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)profiles/spp/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)stack/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)stack/hci/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)stack/l2cap/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)stack/me/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)stack/rfcomm/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)stack/sdp/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)profiles/hid/*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

ifeq ($(BES_AUD),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)profiles/besaud/*.c))
endif

BTPROFILE_LIB_NAME := $(CHIP)_libbt_profiles

ifeq ($(HFP_1_6_ENABLE), 1)
BTPROFILE_LIB_NAME := $(BTPROFILE_LIB_NAME)_sbc_enc
endif
ifeq ($(A2DP_AAC_ON), 1)
BTPROFILE_LIB_NAME := $(BTPROFILE_LIB_NAME)_aac
endif

ifeq ($(A2DP_LHDC_ON), 1)
BTPROFILE_LIB_NAME := $(BTPROFILE_LIB_NAME)_lhdc
endif

ifeq ($(A2DP_LDAC_ON), 1)
BTPROFILE_LIB_NAME := $(BTPROFILE_LIB_NAME)_ldac
endif

ifeq ($(ROM_UTILS_ON),1)
BTPROFILE_LIB_NAME := $(BTPROFILE_LIB_NAME)_romaac
endif

ifeq ($(BLE),1)
BTPROFILE_LIB_NAME := $(BTPROFILE_LIB_NAME)_ble
endif

ifeq ($(BT_RF_PREFER), 2M)
BTPROFILE_LIB_NAME := $(BTPROFILE_LIB_NAME)_2m
endif

ifeq ($(BT_RF_PREFER), 3M)
BTPROFILE_LIB_NAME := $(BTPROFILE_LIB_NAME)_3m
endif

$(BTPROFILE_LIB_NAME)-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

-include $(cur_dir)lib_proc/Makefile

obj-y += $(BTPROFILE_LIB_NAME).a

CFLAGS_a2alloc.o += -O3
CFLAGS_a2dp.o += -O3
CFLAGS_a2dp_sdp.o += -O3
CFLAGS_a2dp_sm.o += -O3
CFLAGS_sbc.o += -O3
CFLAGS_sbc_math.o += -O3

ifeq ($(ROM_UTILS_ON),1)
CFLAGS_sbc.o += -DSBC_FUNC_ATTR=WEAK
else
CFLAGS_sbc.o += -DSBC_FUNC_ATTR=
endif

subdir-ccflags-y += \
    -Iservices/fs/fat \
    -Iservices/fs/sd \
    -Iservices/fs/fat/ChaN \
    -Iservices/bt_profiles/inc \
    -Iservices/bt_if/inc \
    -Iplatform/drivers/uarthci \
    -Iplatform/drivers/ana \
    -Iplatform/drivers/bt \
    -Iutils/cqueue \
    -Iutils/heap \
    -Iservices/bt_app \
    -Iservices/audioflinger \
    -Iservices/nvrecord \
    -Iservices/osif \
    -Iutils/lockcqueue \
    -Iapps/key \
    -Iapps/audioplayers

