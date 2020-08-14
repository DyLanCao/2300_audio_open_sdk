
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

mmlib_obj :=

mmlib_obj += algorithm/fft/src/

mmlib_obj += audio/process/adp/src/
mmlib_obj += audio/process/anc/src/
mmlib_obj += audio/process/common/src/
mmlib_obj += audio/process/drc/src/
mmlib_obj += audio/process/filters/src/
mmlib_obj += audio/process/limiter/src/
mmlib_obj += audio/process/resample/src/
mmlib_obj += audio/process/floatlimiter/src/

ifeq ($(FM_ENABLE),1)
mmlib_obj += fm/
endif

ifeq ($(A2DP_OPUS_ON),1)
#mmlib_obj += opus121/src/
endif


ifeq ($(A2DP_AAC_ON),1)
mmlib_obj += audio/codec/fdkaac_codec/libAACdec/src/
mmlib_obj += audio/codec/fdkaac_codec/libAACenc/src/
mmlib_obj += audio/codec/fdkaac_codec/libFDK/src/
mmlib_obj += audio/codec/fdkaac_codec/libMpegTPDec/src/
mmlib_obj += audio/codec/fdkaac_codec/libMpegTPEnc/src/
mmlib_obj += audio/codec/fdkaac_codec/libPCMutils/src/
mmlib_obj += audio/codec/fdkaac_codec/libSBRdec/src/
mmlib_obj += audio/codec/fdkaac_codec/libSBRenc/src/
mmlib_obj += audio/codec/fdkaac_codec/libSYS/src/
ifeq ($(ROM_UTILS_ON),1)
subdir-ccflags-y += -DFDK_FUNC_ATTR=WEAK
else
subdir-ccflags-y += -DFDK_FUNC_ATTR=
endif
endif

#mmlib_obj += rbcodec/src/

ifeq ($(SPEECH_LIB),1)
mmlib_obj += speech/src/
endif

MULTIMEDIA_LIB_NAME := $(CHIP)_libmultimedia

ifeq ($(ROM_UTILS_ON),1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_romaac
endif
$(MULTIMEDIA_LIB_NAME)-y := $(mmlib_obj)

rel_src_obj :=
rel_src_obj += audio/process/anc/cfg/
rel_src_obj += audio/process/filters/cfg/
rel_src_obj += audio/process/resample/coef/

ifeq ($(OPUS_CODEC),1)
rel_src_obj += opus121/build/
endif

ifeq ($(WL_NSX),1)
rel_src_obj += wl_alg/build/
endif

obj-y := $(MULTIMEDIA_LIB_NAME).a $(rel_src_obj)

subdir-ccflags-y += \
	-Iutils/heap \

