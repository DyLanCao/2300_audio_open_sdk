cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c $(cur_dir)src/*.cpp $(cur_dir)src/*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y += $(AAC_INCLUDES) \
	-Iservices/audio_process \
	-Iservices/audio_dump/include \
	-Iservices/hw_dsp/inc \
	-Iservices/fs/fat \
	-Iservices/multimedia/wl_alg/gcc_plat/include \
	-Iservices/fs/sd \
	-Iservices/fs/fat/ChaN \
	$(BT_IF_INCLUDES) \
	$(BT_PROFILES_INCLUDES) \
	-Iservices/multimedia/speech/inc \
	-Iservices/bone_sensor/lis25ba \
	-Iservices/overlay \
	-Ithirdparty/tile/tile_common/tile_storage \
	-Iservices/nvrecord \
	-Iservices/resources \
	-Iservices/multimedia/rbcodec \
	-Iservices/multimedia/audio/process/resample/include \
	-Iservices/multimedia/audio/process/filters/include \
	-Iservices/multimedia/audio/process/drc/include \
	-Iservices/multimedia/audio/process/anc/include\
	-Iservices/nv_section/aud_section \
	-Iservices/nv_section/userdata_section \
	-Iservices/nv_section/include \
	-Iservices/voicepath/$(VOICE_DATAPATH_TYPE) \
	-Iservices/voicepath/gsound/gsound_target \
	-Iservices/voicepath/gsound/gsound_custom/inc \
	-Iservices/voicepath/gsound/gsound_target_api_read_only \
	-Iplatform/drivers/uarthci \
	-Iplatform/drivers/ana \
	-Iplatform/cmsis \
	-Iplatform/drivers/bt \
	-Iutils/cqueue \
	-Iutils/heap \
	-Iservices/audioflinger \
	-Iutils/lockcqueue \
	-Iutils/intersyshci \
	-Iapps/anc/inc \
	-Iapps/key \
	-Iapps/main \
	-Iapps/common \
	-Iapps/audioplayers \
	-Iapps/audioplayers/a2dp_decoder \
	-Iapps/battery \
	-Iapps/common \
	-Iapps/factory \
	-Iservices/app_ibrt/inc \
	-Iservices/ble_app \
	-Iutils/hwtimer_list \
	-Iservices/ibrt_core/inc \
	-Iservices/ibrt_ui/inc \
	-Iservices/ble_stack/ble_ip \
	-Iservices/ble_stack/hl/api \
	-Iservices/ble_stack/app/api/  \
	-Iservices/ble_stack/common/api/ \
	-Iservices/ble_stack/hl/inc/ \
	-Iservices/ble_stack/ke/api \
	-Iservices/voicepath \
	-Ithirdparty/userapi \
	-Iservices/ble_app/app_gfps \
	-Iservices/ble_app/app_main \
	-Ithirdparty/audio_codec_lib/liblhdc-dec/inc \
	-Iservices/ai_voice/manager \
	-Iservices/ai_voice/audio \
	-Iservices/ai_voice/transport \
	-Iservices/app_ai/inc \
	-Iservices/interconnection/red \
	-Iservices/interconnection/green \
	-Iservices/interconnection/umm_malloc \
	-Iservices/bt_app \
	-Iservices/multimedia/audio/codec/sbc/inc \
	-Iservices/multimedia/audio/codec/sbc/src/inc \
	-Iservices/bt_app/a2dp_codecs/include \
	-Iservices/app_bt_source/inc \
	-Ithirdparty/tile/tile_target \
	-Iservices/osif \
	-Iservices/ble_app/app_cbox_audio \
	-Iplatform/hal/best2300 \
	-Iservices/ble_app/app_datapath \
	-Iservices/multimedia/opus121/include \
	-Iservices/nv_section/factory_section \
	-Iservices/bt_if/inc \
	-Iservices/wl_smartvoice/inc \
	-Iservices/multimedia/wl_alg/nsx_main/include \
    -Iservices/multimedia/algorithm/fft/include \
