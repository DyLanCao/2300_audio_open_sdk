cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)../ns/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)../nsx_base/src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)../nsx_main/src/*.c))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ccflags-y += \
	-Iservices/multimedia/wl_alg/ns/include \
	-Iservices/multimedia/wl_alg/nsx_main/include \
	-Iservices/multimedia/wl_alg/signal_processing/include \
	-Iservices/multimedia/wl_alg/signal_processing \

ccflags-y += -O3 -DARM_MATH_CM4 

