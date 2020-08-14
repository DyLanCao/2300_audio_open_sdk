cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj-y := $(obj-y:.c=.o)

ifeq ($(USB_CLK_SRC_26M_X4),1)
CMU_CFG_FLAGS += -DUSB_CLK_SRC_26M_X4
else
ifeq ($(USB_CLK_SRC_26M_X2),1)
CMU_CFG_FLAGS += -DUSB_CLK_SRC_26M_X2
endif
endif

ifeq ($(AUDIO_USE_BBPLL),1)
CMU_CFG_FLAGS += -DAUDIO_USE_BBPLL
endif

ifeq ($(DAC_CLASSG_ENABLE),1)
CODEC_CFG_FLAGS += -DDAC_CLASSG_ENABLE
endif

ifeq ($(DAC_DRE_ENABLE),1)
CODEC_CFG_FLAGS += -DDAC_DRE_ENABLE
endif

ifeq ($(DIG_MIC_WORKAROUND),1)
CODEC_CFG_FLAGS += -DDIG_MIC_WORKAROUND
endif

ifeq ($(AUDIO_OUTPUT_DIFF),1)
CODEC_CFG_FLAGS += -DAUDIO_OUTPUT_DIFF
endif

ifeq ($(JTAG_BT),1)
PSC_CFG_FLAGS += -DJTAG_BT
JTAG_TDI_TDO_PIN ?= 1
endif

ifeq ($(JTAG_WIFI),1)
PSC_CFG_FLAGS += -DJTAG_WIFI
JTAG_TDI_TDO_PIN ?= 1
endif

ifeq ($(JTAG_TDI_TDO_PIN),1)
IOMUX_CFG_FLAGS += -DJTAG_TDI_TDO_PIN
endif

ifeq ($(JTAG_RESET_PIN),1)
IOMUX_CFG_FLAGS += -DJTAG_RESET_PIN
endif

CFLAGS_hal_analogif_$(CHIP).o += $(ANAIF_CFG_FLAGS)

CFLAGS_hal_cmu_$(CHIP).o += $(CMU_CFG_FLAGS)

CFLAGS_hal_psc_$(CHIP).o += $(PSC_CFG_FLAGS)

CFLAGS_hal_codec_$(CHIP).o += $(CODEC_CFG_FLAGS)

CFLAGS_hal_iomux_$(CHIP).o += $(IOMUX_CFG_FLAGS)

