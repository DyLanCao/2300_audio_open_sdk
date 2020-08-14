cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += -Iplatform/drivers/ana \
					-Iplatform/hal \
					-Iservices/bluedroid/adapter/src \
					-Iapps/key \
					-Iservices/audio_process \
					-Iservices/fs/fat \
					-Iservices/fs/sd \
					-Iservices/fs/fat/ChaN \
					-Iservices/overlay \
					-Iservices/nvrecord \
					-Iservices/resources \
					-Iplatform/drivers/uarthci \
					-Iplatform/drivers/ana \
					-Iplatform/drivers/bt \
					-Iutils/cqueue \
					-Iutils/retention_ram \
					-Iservices/audioflinger \
					-Iutils/lockcqueue \
					-Iutils/intersyshci \
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
					-Iservices/ble_app/app_main/ \
					-Iservices/ble_app/app_htp/	\
					-Iservices/multimedia/speech/application/Pcm8kCvsd/include \
					-Iservices/overlay \
					-Iservices/multimedia/speech/codec/codecs/adpcm \
					-Iservices/bt_app/ \
					-Iservices/ble_app/app_voice/app_smartvoice \
					-Iservices/ble_profiles/voicepath/smartvoice \
					-Iutils/crc32 \
					-Iservices/multimedia/opus121/include \
					-Iservices/multimedia/speech/process/lc_mmse_ns/include \
					-Iservices/multimedia/speech/process/ns_and_ec/include \
					-Iservices/multimedia/speech/process/coherent_denoise/include \
					-Iservices/multimedia/speech/process/dual_mic_denoise/include \
					-Iservices/multimedia/speech/process/audio_drc2/include \
					-Iservices/multimedia/speech/process/far_field_speech_enhancement/include \
					-Iservices/multimedia/speech/process/plc/include \
					-Iservices/multimedia/speech/application/Pcm8kCvsd/include \
					-Iservices/multimedia/speech/process/common/include \
					-Ithirdparty/userapi \
					-Iservices/voicepath \
					-Iservices/voicepath/smartvoice \
					-Iservices/multimedia/speech/inc \
					-Iutils/heap
					
					
