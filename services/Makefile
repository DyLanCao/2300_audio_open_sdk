obj-y := audio_dump/ audioflinger/ audio_process/ hw_dsp/ bt_profiles/ bt_app/ overlay/ resources/ \
			../utils/crc32/ ../utils/heap/ bt_if/ osif/ norflash_api/

ifeq ($(MBED),1)
obj-y += fs/
endif

ifneq ($(MBED)_$(AI_VOICE),0_0)
obj-y +=../utils/kfifo/
endif

ifeq ($(VOICE_DATAPATH_ENABLED),1)
obj-y += voicepath/
obj-y += voicepath/$(VOICE_DATAPATH_TYPE)/
endif

ifeq ($(BLE),1)
obj-y += ble_app/ bridge/ ble_stack/ ../utils/retention_ram/ ble_profiles/
endif

ifeq ($(AUTO_TEST),1)
obj-y += auto_test/
endif

obj-y += nv_section/

ifneq ($(FPGA),1)
obj-y += nvrecord/
endif

ifeq ($(VOICE_RECOGNITION),1)
obj-y += voice_recognition/
endif

ifeq ($(SMARTVOICE),1)
obj-y += smartvoice/
endif

ifeq ($(INTERCONNECTION),1)
obj-y += interconnection/
endif

ifeq ($(TEST_OVER_THE_AIR),1)
obj-y += tota/
endif

ifeq ($(BES_OTA_BASIC),1)
obj-y += ota/
endif

ifeq ($(AI_VOICE),1)
obj-y += ai_voice/
endif

ifeq ($(CHIP_HAS_CP),1)
obj-y += cp_accel/
endif

ifeq ($(IBRT),1)
obj-y += tws_ibrt/
obj-y += app_ibrt/
endif

obj-y += communication/

ifeq ($(APP_WL_SMARTVOICE),1)
obj-y += wl_smartvoice/
endif


ifeq ($(WL_FIR_FILTER),1)
obj-y += wl_filter/
endif

