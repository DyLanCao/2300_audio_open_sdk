
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

ifeq ($(A2DP_DECODER_VER),2)
obj_cpp := $(filter-out a2dpplay.cpp,$(obj_cpp))
obj_cpp += a2dp_decoder/
endif

ifeq ($(RB_CODEC),1)
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rbplay/*.cpp))
endif

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

CFLAGS_a2dpplay.o += -O3

ifeq ($(A2DP_AAC_ON),1)
AAC_INCLUDES =  \
	-Iservices/multimedia/audio/codec/fdkaac_codec/libAACdec/include \
	-Iservices/multimedia/audio/codec/fdkaac_codec/libAACenc/include \
	-Iservices/multimedia/audio/codec/fdkaac_codec/libFDK/include \
	-Iservices/multimedia/audio/codec/fdkaac_codec/libMpegTPDec/include \
	-Iservices/multimedia/audio/codec/fdkaac_codec/libMpegTPEnc/include \
	-Iservices/multimedia/audio/codec/fdkaac_codec/libPCMutils/include \
	-Iservices/multimedia/audio/codec/fdkaac_codec/libSBRdec/include \
	-Iservices/multimedia/audio/codec/fdkaac_codec/libSBRenc/include \
	-Iservices/multimedia/audio/codec/fdkaac_codec/libSYS/include
else
AAC_INCLUDES =
endif

ccflags-y := \
	$(AAC_INCLUDES) \
	-Iservices/audio_process \
	-Iservices/fs/fat \
	-Iservices/fs/sd \
	-Iservices/resources \
	-Iservices/fs/fat/ChaN \
	-Iservices/bt_app \
	-Iservices/bt_if/inc \
	-Iplatform/drivers/uarthci \
	-Iutils/cqueue \
	-Iservices/audio_dump/include \
	-Iservices/multimedia/speech/inc \
	-Iservices/multimedia/rbcodec/inc \
	-Iservices/multimedia/wl_alg/nsx_main/include \
	-Iservices/multimedia/audio/process/eq/include \
	-Iservices/multimedia/audio/process/resample/include \
	-Iservices/multimedia/audio/process/filters/include \
	-Iservices/multimedia/fm/inc \
	-Iservices/nv_section/aud_section \
	-Iservices/nv_section/include \
	-Iservices/overlay \
	-Iservices/nvrecord \
	-Iservices/nv_section/log_section \
	-Iservices/bt_if/inc \
	-Iapps/main \
	-Iapps/audioplayers/rbplay/ \
	-Iapps/audioplayers/a2dp_decoder \
	-Iutils/list \
	-Iutils/heap \
	-Iplatform/drivers/ana \
	-Ithirdparty/audio_codec_lib/scalable/ \
	-Ithirdparty/audio_codec_lib/liblhdc-dec/inc \
	-Iapps/apptester \
	-Iapps/key  \
	-Iplatform/drivers/bt \
	-Iapps/anc/inc

ifeq ($(IBRT),1)
ccflags-y += -Iservices/tws_ibrt/inc
ccflags-y += -Iservices/app_ibrt/inc
endif

ifeq ($(A2DP_LDAC_ON),1)
ccflags-y += -Ithirdparty/audio_codec_lib/ldac/inc
endif

ifeq ($(A2DP_CP_ACCEL),1)
ccflags-y += -Iservices/cp_accel
endif

ifeq ($(APP_TEST_AUDIO),1)
CFLAGS_app_audio.o += -DAPP_TEST_AUDIO
endif

ifeq ($(AUDIO_RESAMPLE),1)
CFLAGS_a2dpplay.o += -D__AUDIO_RESAMPLE__
CFLAGS_voicebtpcmplay.o += -D__AUDIO_RESAMPLE__
endif
ifeq ($(SW_PLAYBACK_RESAMPLE),1)
CFLAGS_a2dpplay.o += -DSW_PLAYBACK_RESAMPLE
endif
ifeq ($(RESAMPLE_ANY_SAMPLE_RATE),1)
CFLAGS_a2dpplay.o += -DRESAMPLE_ANY_SAMPLE_RATE
endif
ifeq ($(SW_SCO_RESAMPLE),1)
CFLAGS_voicebtpcmplay.o += -DSW_SCO_RESAMPLE
endif

ifeq ($(SPEECH_TX_AEC),1)
CFLAGS_voicebtpcmplay.o += -DSPEECH_TX_AEC
endif

ifeq ($(SPEECH_TX_NS),1)
CFLAGS_voicebtpcmplay.o += -DSPEECH_TX_NS
endif

ifeq ($(SPEECH_TX_NS2),1)
CFLAGS_voicebtpcmplay.o += -DSPEECH_TX_NS2
endif

ifeq ($(SPEECH_TX_2MIC_NS2),1)
CFLAGS_voicebtpcmplay.o += -DSPEECH_TX_2MIC_NS2
endif

ifeq ($(SPEECH_TX_2MIC_NS5),1)
CFLAGS_voicebtpcmplay.o += -DSPEECH_TX_2MIC_NS5
endif

ifeq ($(SPEECH_RX_PLC),1)
CFLAGS_voicebtpcmplay.o += -DSPEECH_RX_PLC
endif

ifeq ($(SPEECH_TX_EQ),1)
CFLAGS_voicebtpcmplay.o += -DSPEECH_TX_EQ
endif

ifeq ($(SPEECH_RX_NS),1)
CFLAGS_voicebtpcmplay.o += -DSPEECH_RX_NS
CFLAGS_app_audio.o += -DSPEECH_RX_NS
endif

ifeq ($(SPEECH_RX_NS2),1)
CFLAGS_voicebtpcmplay.o += -DSPEECH_RX_NS2
endif

ifeq ($(SPEECH_RX_EQ),1)
CFLAGS_voicebtpcmplay.o += -DSPEECH_RX_EQ
endif

ifeq ($(VOICE_PROMPT),1)
CFLAGS_app_audio.o += -DMEDIA_PLAYER_SUPPORT
endif

ifeq ($(AUDIO_QUEUE_SUPPORT),1)
CFLAGS_app_audio.o += -D__AUDIO_QUEUE_SUPPORT__
endif

ifeq ($(SPEECH_TX_AEC2),1)
ccflags-y += -DVOS_ARM -D_MED_C89_ -DUSE_DSPFNS -DFIXED_POINT -DUSE_KISS_FFT
endif

ifeq ($(ANC_APP),1)
CFLAGS_app_audio.o += -DANC_APP
endif

ifeq ($(A2DP_EQ_24BIT),1)
CFLAGS_app_audio.o += -DA2DP_EQ_24BIT
CFLAGS_a2dpplay.o += -DA2DP_EQ_24BIT
endif

ifeq ($(A2DP_TRACE_CP_ACCEL),1)
CFLAGS_a2dpplay.o += -DA2DP_TRACE_CP_ACCEL
endif

ifeq ($(A2DP_TRACE_DEC_TIME),1)
CFLAGS_a2dpplay.o += -DA2DP_TRACE_DEC_TIME
endif

ifeq ($(A2DP_TRACE_CP_DEC_TIME),1)
CFLAGS_a2dpplay.o += -DA2DP_TRACE_CP_DEC_TIME
endif

