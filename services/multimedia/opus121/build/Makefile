cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)../src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)../celt/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)../celt/fixed/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)../silk/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)../silk/fixed/*.c))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ccflags-y += \
	-Iservices/multimedia/opus121/celt \
	-Iservices/multimedia/opus121/celt/fixed \
	-Iservices/multimedia/opus121/silk \
	-Iservices/multimedia/opus121/silk/fixed \
	-Iservices/multimedia/opus121/include \
	-Iservices/multimedia/opus121/src \
	-Iservices/multimedia/opus121

ccflags-y += -O3 -DARM_MATH_CM4 -DOPUS_HAVE_CONFIG_H -DHAVE_CONFIG_H

