cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

AMA_LIB_NAME := libama

$(AMA_LIB_NAME)-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

obj-y += $(AMA_LIB_NAME).a

ccflags-y +=  -Iplatform/hal \
              -Iservices/nvrecord \
			  -Iservices/bt_app \
			  -Iservices/bt_if/inc \
			  -Iservices/ai_voice/protocol/ama/ama_protobuf \
	          -Iservices/ai_voice/protocol/ama/ama_protobuf/protobuf-c \
			  -Iservices/ai_voice/transport \
			  -Iservices/ai_voice/manager \
			  -Iutils/cqueue \
			  -Iutils/heap \
			  -Iutils/kfifo \

