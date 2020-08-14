cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

ifeq ($(CODEC_HIGH_QUALITY),1)
VCODEC_VOLT ?= 2.5V
else
VCODEC_VOLT ?= 1.6V
endif
ifneq ($(filter-out 1.5V 1.6V 1.8V 2.5V 3.5V,$(VCODEC_VOLT)),)
$(error Invalid VCODEC_VOLT=$(VCODEC_VOLT))
endif
ifneq ($(VCODEC_VOLT),)
VCODEC_STEM := $(subst .,P,$(VCODEC_VOLT))
ANA_CFG_FLAGS += -DVCODEC_$(VCODEC_STEM)
PMU_CFG_FLAGS += -DVCODEC_$(VCODEC_STEM)
ifneq ($(filter 1.5V 1.6V 1.8V 2.5V,$(VCODEC_VOLT)),)
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
ifneq ($(filter-out 1.5V 1.6V 1.8V 2.5V,$(VANA_VOLT)),)
$(error Invalid VANA_VOLT=$(VANA_VOLT))
endif
ifneq ($(VANA_VOLT),)
VANA_STEM := $(subst .,P,$(VANA_VOLT))
PMU_CFG_FLAGS += -DVANA_$(VANA_STEM)
endif

PMU_DCDC_ANA2_SUPPORT ?= 1
ifeq ($(PMU_DCDC_ANA2_SUPPORT),1)
PMU_CFG_FLAGS += -DPMU_DCDC_ANA2_SUPPORT
endif

LOW_POWER_ADC ?= 1
ifeq ($(LOW_POWER_ADC),1)
ANA_CFG_FLAGS += -DLOW_POWER_ADC
endif

CFLAGS_pmu_$(CHIP).o += $(PMU_CFG_FLAGS)

CFLAGS_analog_$(CHIP).o += $(ANA_CFG_FLAGS)

CFLAGS_usbphy_$(CHIP).o += $(USBPHY_CFG_FLAGS)

