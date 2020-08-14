
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))

BTBRIDGE_LIB_NAME := $(CHIP)_libbridge

ifeq ($(HFP_1_6_ENABLE), 1)
BTBRIDGE_LIB_NAME := $(BTBRIDGE_LIB_NAME)_sbc_enc
endif
ifeq ($(A2DP_AAC_ON), 1)
BTBRIDGE_LIB_NAME := $(BTBRIDGE_LIB_NAME)_aac
endif

ifeq ($(A2DP_LHDC_ON), 1)
BTBRIDGE_LIB_NAME := $(BTBRIDGE_LIB_NAME)_lhdc
endif

ifeq ($(A2DP_LDAC_ON), 1)
BTBRIDGE_LIB_NAME := $(BTBRIDGE_LIB_NAME)_ldac
endif

$(BTBRIDGE_LIB_NAME)-y := $(obj_c:.c=.o)

obj-y := $(BTBRIDGE_LIB_NAME).a

subdir-ccflags-y += \
	-Iservices/bridge/ \
	-Iservices/ble_stack/common/api/ \
	-Iservices/ble_stack/hl/inc/ \
	-Iservices/ble_stack/hl/api/ \
	-Iservices/ble_stack/hl/src/gap/ \
	-Iservices/ble_stack/hl/src/gap/gapc/ \
	-Iservices/ble_stack/hl/src/gap/gapm/ \
	-Iservices/ble_stack/hl/src/gap/smpc/ \
	-Iservices/ble_stack/ke/api/ \
	-Iservices/ble_stack/ke/src/ \
	-Iservices/ble_stack/hl/src/gatt/attc/ \
	-Iservices/ble_stack/hl/src/gatt/attm/ \
	-Iservices/ble_stack/hl/src/gatt/atts/ \
	-Iservices/ble_stack/hl/src/gatt/gattc/ \
	-Iservices/ble_stack/hl/src/gatt/gattm/ \
	-Iservices/ble_stack/hl/src/gatt/ \
	-Iservices/ble_stack/hl/src/l2c/l2cc/ \
	-Iservices/ble_stack/hl/src/l2c/l2cm/ \
	-Iservices/ble_stack/hci/api/ \
	-Iservices/ble_stack/hci/src/ \
	-Iservices/ble_stack/dbg/api/ \
	-Iservices/ble_stack/dbg/src/ \
	-Iservices/ble_stack/ble_ip/ \
	-Iservices/bt_if/inc/ \
	-Iservices/bt_profiles/inc/ \
	-Iservices/bt_profiles/inc/sys/ \

