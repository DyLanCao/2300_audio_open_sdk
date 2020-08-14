cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))


obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)app_main/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)app_htp/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)app_datapath/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)app_hrps/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)app_hid/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)app_sec/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)app_batt/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)app_voice/app_$(VOICE_DATAPATH_TYPE)/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)app_ota/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)app_ancc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)app_amsc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)app_gfps/*.c))

ifeq ($(TEST_OVER_THE_AIR),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)/app_tota/*.c))
subdir-ccflags-y +=-Iservices/tota/ \
		-Iservices/ble_profiles/tota/\
		-Iservices/ble_app/app_tota/
endif

obj-y := $(obj_c:.c=.o)

subdir-ccflags-y += \
	-Iservices/audio_process \
	-Iservices/fs/fat \
	-Iservices/fs/sd \
	-Iservices/fs/fat/ChaN \
	-Iservices/bt_profiles/inc \
	-Iservices/bt_if/inc \
	-Iservices/bt_profiles/inc/sys \
	-Iservices/bt_if/inc \
	-Iservices/overlay \
	-Iservices/nvrecord \
	-Iservices/resources \
	-Iservices/voicepath/ \
	-Iservices/voicepath/$(VOICE_DATAPATH_TYPE) \
	-Iplatform/drivers/uarthci \
	-Iplatform/drivers/ana \
	-Iplatform/drivers/bt \
	-Iutils/cqueue \
	-Iutils/retention_ram \
	-Iservices/audioflinger \
	-Iutils/lockcqueue \
	-Iutils/intersyshci \
	-Iutils/heap \
	-Iutils/crc16 \
	-Iapps/battery \
	-Iapps/key \
	-Iapps/main \
	-Iapps/common \
	-Iapps/audioplayers \
	-Iapps/factory \
	-Iservices/resources \
	-Iservices/ble_app \
	-Iservices/ble_stack/ble_ip \
	-Iservices/ble_stack/hl/api \
	-Iservices/ble_stack/app/api/  \
	-Iservices/ble_stack/common/api/ \
	-Iservices/ble_stack/hl/inc/ \
	-Iservices/ble_stack/ke/api \
	-Iservices/bridge/ \
	-Iservices/bt_profiles/inc/ \
	-Iservices/bt_profiles/inc/sys/ \
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
	-Iservices/ble_stack/ble_ip/  \
	-Iservices/ble_stack/app/api/  \
	-Iservices/ble_stack/app/src/  \
	-Iservices/ble_profiles/htp/htpt/api/  \
	-Iservices/ble_profiles/htp/htpt/src/  \
	-Iservices/ble_profiles/htp/	\
	-Iservices/ble_profiles/datapath/datapathps/api/  \
	-Iservices/ble_app/app_main/ \
	-Iservices/ble_app/app_htp/	\
	-Iservices/ble_app/app_datapath/	\
	-Iservices/ble_app/app_hrps/	\
	-Iservices/ble_profiles/hrp/hrps/api/ \
	-Iservices/ble_profiles/hrp/hrps/src \
	-Iservices/ble_profiles/hrp/ \
	-Iservices/ble_app/app_sec/ \
	-Iservices/ble_app/app_batt/ \
	-Iservices/ble_app/app_hid/ \
	-Iservices/ble_app/app_gfps/ \
	-Iservices/ble_profiles/gfps/gfps_provider/api/ \
	-Iservices/ble_profiles/gfps/api/ \
	-Iservices/ble_profiles/bas/bass/api/ \
	-Iservices/ble_profiles/hogp/hogpd/api/ \
	-Iservices/ble_profiles/hogp/ \
	-Iservices/multimedia/speech/inc \
	-Iservices/ble_app/app_voice \
	-Iservices/voicepath/ \
	-Iservices/voicepath/$(VOICE_DATAPATH_TYPE)/ \
	-Iservices/overlay \
	-Iservices/ble_profiles/voicepath/$(VOICE_DATAPATH_TYPE)/ \
	-Iservices/ble_app/app_voice/app_$(VOICE_DATAPATH_TYPE)/ \
	-Iservices/ble_profiles/ota \
	-Iservices/ble_app/app_ota \
	-Iservices/ble_profiles/anc/\
	-Iservices/ble_profiles/anc/ancc/\
	-Iservices/ble_app/app_ancc/ \
	-Iservices/ble_profiles/ams/\
	-Iservices/ble_profiles/ams/amsc/\
	-Iservices/ble_app/app_amsc/ \
	-Iservices/bt_app \
	-Iservices/voicepath/gsound/gsound_target \
	-Iservices/voicepath/gsound/gsound_target_api_read_only \
	-Iservices/nvrecord/ \
	-Iservices/ai_voice/manager \
	-Iservices/ai_voice/transport \
	-Iservices/wl_smartvoice/inc

ifeq ($(AUDIO_RESAMPLE),1)
CFLAGS_voice_over_ble.o += -D__AUDIO_RESAMPLE__
endif
ifeq ($(SW_PLAYBACK_RESAMPLE),1)
CFLAGS_voice_over_ble.o += -DSW_PLAYBACK_RESAMPLE
endif
ifeq ($(SW_CAPTURE_RESAMPLE),1)
CFLAGS_voice_over_ble.o += -DSW_CAPTURE_RESAMPLE
endif
ifeq ($(BES_OTA_BASIC),1)
ccflags-y += -Iservices/ota
endif

