cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += -Iapps/main \
			  -Iapps/app_anker/device_drivers/bes_hal \
			  -Iapps/audioplayers \
			  -Iapps/battery \
			  -Iapps/key \
			  -Iplatform/hal \
			  -Iplatform/drivers/ana \
			  -Iservices/ai_voice/algorithm \
			  -Iservices/ai_voice/audio \
			  -Iservices/ai_voice/codec/compression \
			  -Iservices/ai_voice/codec/sbc \
			  -Iservices/ai_voice/transport \
			  -Iservices/ai_voice/manager \
			  -Iservices/ai_voice/protocol/smartvoice/smartvoice_bt \
			  -Iservices/ai_voice/protocol/smartvoice/smartvoice_ble \
			  -Iservices/ai_voice/protocol/smartvoice/smartvoice_gatt \
			  -Iservices/ble_app/app_main/  \
			  -Iservices/ble_stack/ble_ip/  \
			  -Iservices/ble_stack/common/api  \
			  -Iservices/ble_stack/hl/api \
			  -Iservices/ble_stack/ke/api  \
			  -Iservices/bt_app \
			  -Iservices/bt_if/inc \
			  -Iservices/ai_voice/codec/opus121/include \
			  -Iservices/nvrecord \
	          -Iservices/resources \
			  -Iutils/cqueue \
			  -Iutils/heap \
			  -Iutils/crc32 \
