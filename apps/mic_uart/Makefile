cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))

mmlib_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

PLAT_LIB_NAME := mic_uart

ifeq ($(ROM_UTILS_ON),1)
PLAT_LIB_NAME := $(PLAT_LIB_NAME)_v12
endif

$(PLAT_LIB_NAME)-y := $(mmlib_obj)

rel_src_obj :=

obj-y := $(PLAT_LIB_NAME).a $(rel_src_obj)

ccflags-y +=  \
    -Iutils/boot_struct \
    -Iutils/intersyshci \
    -Iutils/hwtimer_list \
    -Iservices/bt_app \
    -Iservices/nvrecord \
    -Iservices/wl_filter \
    -Iservices/bt_if/inc \
    -Iservices/resources \
    -Iservices/audio_dump/include \
    -Iservices/multimedia/wl_alg/gcc_plat/include \
    -Iservices/multimedia/wl_alg/vad_user/include \
    -Iservices/overlay \
    -Iservices/multimedia/wl_alg/nsx_main/include \
    -Iservices/multimedia/wl_alg/agc/include \
    -Iservices/multimedia/wl_alg/utility/include \
    -Iservices/multimedia/wl_alg/notch_filter/include \
    -Iservices/multimedia/algorithm/fft/include \
    -Iservices/multimedia/speech/inc \
    -Iapps/key \
    -Iplatform/drivers/bt \
    -Iplatform/drivers/ana \
    -Iplatform/drivers/usb/usb_dev/inc \
	-Iapps/battery



