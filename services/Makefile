
obj-y := audio_dump/ audioflinger/ audio_process/ hw_dsp/ bt_profiles/ bt_app/ overlay/ resources/ \
			../utils/crc32/ ../utils/heap/ bt_if/ osif/ norflash_api/


mmlib_obj := audio_dump/ audioflinger/ audio_process/ hw_dsp/ bt_profiles/ bt_app/ overlay/ resources/ \
			../utils/crc32/ ../utils/heap/ bt_if/ osif/ norflash_api/

ifeq ($(MBED),1)
obj-y += fs/
mmlib_obj += fs/
endif


ifneq ($(MBED)_$(AI_VOICE),0_0)
obj-y +=../utils/kfifo/
mmlib_obj +=../utils/kfifo/
endif

ifeq ($(VOICE_DATAPATH_ENABLED),1)
obj-y += voicepath/
obj-y += voicepath/$(VOICE_DATAPATH_TYPE)/
endif

ifeq ($(BLE),1)
obj-y += ble_app/ bridge/ ble_stack/ ../utils/retention_ram/ ble_profiles/
mmlib_obj += ble_app/ bridge/ ble_stack/ ../utils/retention_ram/ ble_profiles/
endif

ifeq ($(AUTO_TEST),1)
obj-y += auto_test/
endif

obj-y += nv_section/
mmlib_obj +=nv_section/

ifneq ($(FPGA),1)
obj-y += nvrecord/
mmlib_obj +=nvrecord/
endif

ifeq ($(VOICE_RECOGNITION),1)
obj-y += voice_recognition/
endif

ifeq ($(SMARTVOICE),1)
obj-y += smartvoice/
endif

ifeq ($(INTERCONNECTION),1)
obj-y += interconnection/
mmlib_obj += interconnection/
endif

ifeq ($(TEST_OVER_THE_AIR),1)
obj-y += tota/
endif

ifeq ($(BES_OTA_BASIC),1)
obj-y += ota/
endif

ifeq ($(CHIP_HAS_CP),1)
obj-y += cp_accel/
endif


obj-y += communication/
mmlob_obj += communication/


ifeq ($(WL_FIR_FILTER),1)
obj-y += wl_filter/
endif

MULTIMEDIA_LIB_NAME := whale_services

ifeq ($(ROM_UTILS_ON),1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_v12
endif

$(MULTIMEDIA_LIB_NAME)-y := $(mmlib_obj)

obj-y := $(MULTIMEDIA_LIB_NAME).a $(rel_src_obj)
