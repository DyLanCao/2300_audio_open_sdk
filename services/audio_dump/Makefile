
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))

mmlib_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

PLAT_LIB_NAME := whale_dump

ifeq ($(ROM_UTILS_ON),1)
PLAT_LIB_NAME := $(PLAT_LIB_NAME)_v12
endif

$(PLAT_LIB_NAME)-y := $(mmlib_obj)

obj-y := $(PLAT_LIB_NAME).a $(rel_src_obj)



ccflags-y := \
	-Iservices/audio_dump/include

