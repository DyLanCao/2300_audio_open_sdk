
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ccflags-y := \
	-Iutils/crc32 \
	-Iutils/heap \
	-Iplatform/drivers/ana \
	-Iservices/norflash_api 

ifneq ($(LOG_DUMP_SECTION_SIZE),0)
ccflags-y += -DLOG_DUMP_SECTION_SIZE=$(LOG_DUMP_SECTION_SIZE)
endif

ifneq ($(CRASH_DUMP_SECTION_SIZE),)
ccflags-y += -DCRASH_DUMP_SECTION_SIZE=$(CRASH_DUMP_SECTION_SIZE)
endif

ifeq ($(TRACE_DUMP2FLASH),1)
ccflags-y += -DTRACE_DUMP2FLASH
endif

ifeq ($(TRACE_CRLF),1)
ccflags-y += -DTRACE_CRLF
endif

