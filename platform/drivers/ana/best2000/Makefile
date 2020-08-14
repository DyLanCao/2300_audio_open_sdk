cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

# VCODEC_VOLT default value is set in common.mk
ifneq ($(filter-out 1.5V 1.6V 1.8V 2.5V 3.5V,$(VCODEC_VOLT)),)
$(error Invalid VCODEC_VOLT=$(VCODEC_VOLT))
endif
ifneq ($(VCODEC_VOLT),)
VCODEC_STEM := $(subst .,P,$(VCODEC_VOLT))
ANA_CFG_FLAGS += -DVCODEC_$(VCODEC_STEM)
PMU_CFG_FLAGS += -DVCODEC_$(VCODEC_STEM)
ifneq ($(filter 1.5V 1.6V 1.8V,$(VCODEC_VOLT)),)
VCODEC_OFF ?= 1
ifeq ($(VANA_VOLT),)
VANA_VOLT := $(VCODEC_VOLT)
endif
ifneq ($(VCODEC_VOLT),$(VANA_VOLT))
$(error Incompatible VCODEC_VOLT=$(VCODEC_VOLT) with VANA_VOLT=$(VANA_VOLT))
endif
endif
endif

ifeq ($(VCODEC_OFF),1)
PMU_CFG_FLAGS += -DVCODEC_OFF
endif

VANA_VOLT ?= 1.6V
ifneq ($(filter-out 1.5V 1.6V 1.8V,$(VANA_VOLT)),)
$(error Invalid VANA_VOLT=$(VANA_VOLT))
endif
ifneq ($(VANA_VOLT),)
VANA_STEM := $(subst .,P,$(VANA_VOLT))
PMU_CFG_FLAGS += -DVANA_$(VANA_STEM)
endif

ifeq ($(PMU_DCDC_ANA2_SUPPORT),1)
PMU_CFG_FLAGS += -DPMU_DCDC_ANA2_SUPPORT
endif

ifeq ($(PMU_DCDC_DIG2_SUPPORT),1)
PMU_CFG_FLAGS += -DPMU_DCDC_DIG2_SUPPORT
endif

ifeq ($(VANA_DEC_50MV),1)
PMU_CFG_FLAGS += -DVANA_DEC_50MV
endif

ifeq ($(DAC_CLASSG_ENABLE),1)
PMU_CFG_FLAGS += -DDAC_CLASSG_ENABLE
endif

LOW_POWER_ADC ?= 1
ifeq ($(LOW_POWER_ADC),1)
ANA_CFG_FLAGS += -DLOW_POWER_ADC
endif

ifeq ($(AUDIO_USE_BBPLL),1)
ANA_CFG_FLAGS += -DAUDIO_USE_BBPLL
PMU_CFG_FLAGS += -DAUDIO_USE_BBPLL
endif

CFLAGS_pmu_$(CHIP).o += $(PMU_CFG_FLAGS)

CFLAGS_analog_$(CHIP).o += $(ANA_CFG_FLAGS)

CFLAGS_usbphy_$(CHIP).o += $(USBPHY_CFG_FLAGS)

