cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

ifeq ($(VCODEC_OFF),1)
PMU_CFG_FLAGS += -DVCODEC_OFF
endif

ifeq ($(ANA_HIGH_VOLT),1)
PMU_CFG_FLAGS += -DANA_HIGH_VOLT
endif

CFLAGS_pmu_$(CHIP).o += $(PMU_CFG_FLAGS)

CFLAGS_analog_$(CHIP).o += $(ANA_CFG_FLAGS)

ifeq ($(CODEC_HIGH_THD_WORKAROUND),1)
CFLAGS_analog_$(CHIP).o += -DCODEC_HIGH_THD_WORKAROUND
endif

ifeq ($(ANA_OLD_DC_CALIB),1)
CFLAGS_analog_$(CHIP).o += -DANA_OLD_DC_CALIB
endif

