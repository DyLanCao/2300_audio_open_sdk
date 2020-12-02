CHIP        ?= best2300

DEBUG       ?= 1

FPGA        ?= 0

MBED        ?= 0

RTOS        ?= 1

LIBC_ROM    ?= 1

export USER_SECURE_BOOT	?= 0
# enable:1
# disable:0

WATCHER_DOG ?= 0

DEBUG_PORT  ?= 1
# 0: usb
# 1: uart0
# 2: uart1

FLASH_CHIP	?= ALL
# GD25Q80C
# GD25Q32C
# ALL

AUDIO_OUTPUT_MONO ?= 1

AUDIO_OUTPUT_DIFF ?= 0

HW_FIR_EQ_PROCESS ?= 0

SW_IIR_EQ_PROCESS ?= 0

HW_DAC_IIR_EQ_PROCESS ?= 0

HW_IIR_EQ_PROCESS ?= 0

HW_DC_FILTER_WITH_IIR ?= 0

AUDIO_DRC ?= 0

AUDIO_DRC2 ?= 0

PC_CMD_UART ?= 0

AUDIO_SECTION_ENABLE ?= 0

AUDIO_RESAMPLE ?= 1

RESAMPLE_ANY_SAMPLE_RATE ?= 1

OSC_26M_X4_AUD2BB ?= 0

AUDIO_OUTPUT_VOLUME_DEFAULT ?= 15
# range:1~16

AUDIO_INPUT_CAPLESSMODE ?= 0

AUDIO_INPUT_LARGEGAIN ?= 0

AUDIO_CODEC_ASYNC_CLOSE ?= 1

AUDIO_SCO_BTPCM_CHANNEL ?= 1

SPEECH_SIDETONE ?= 0

SPEECH_TX_DC_FILTER ?= 1

SPEECH_TX_AEC2FLOAT ?= 1

SPEECH_TX_NS2FLOAT ?= 0

SPEECH_TX_2MIC_NS2 ?= 0

SPEECH_TX_COMPEXP ?= 1

SPEECH_TX_EQ ?= 0

SPEECH_TX_POST_GAIN ?= 0

SPEECH_RX_NS2FLOAT ?= 0

SPEECH_RX_EQ ?= 0

SPEECH_RX_POST_GAIN ?= 0

LARGE_RAM ?= 1

HSP_ENABLE ?= 0

HFP_1_6_ENABLE ?= 1

ifeq ($(HFP_1_6_ENABLE),1)
    MSBC_16K_SAMPLE_RATE ?= 1
else
    MSBC_16K_SAMPLE_RATE := 0
endif

SBC_FUNC_IN_ROM ?= 0

ROM_UTILS_ON ?= 1

APP_LINEIN_A2DP_SOURCE ?= 0

APP_I2S_A2DP_SOURCE ?= 0

BT_SELECT_PROF_DEVICE_ID ?= 0

VOICE_PROMPT ?= 1

BISTO ?= 0

AI_VOICE ?= 0

AMA_VOICE ?= 0

DMA_VOICE ?= 0

SMART_VOICE ?= 0

TENCENT_VOICE ?= 0

BLE ?= 1

TOTA ?= 0

BES_OTA_BASIC ?= 0

INTERCONNECTION ?= 0

BT_ONE_BRING_TWO ?= 1

DSD_SUPPORT ?= 0

A2DP_LHDC_ON ?= 0

A2DP_EQ_24BIT ?= 1

A2DP_AAC_ON ?= 1

A2DP_SCALABLE_ON ?= 0

A2DP_LDAC_ON ?= 0

FACTORY_MODE ?= 1

ENGINEER_MODE ?= 1

ULTRA_LOW_POWER	?= 1

DAC_CLASSG_ENABLE ?= 1

NO_SLEEP ?= 0

export APP_WL_SMARTVOICE ?= 1

export POWER_MODE   ?= DIG_DCDC

export SPEECH_CODEC ?= 1

export FLASH_SIZE ?= 0x100000
export FLASH_SUSPEND ?= 1
export FLASH_API_HIGHPERFORMANCE ?= 1

ifeq ($(DSD_SUPPORT),1)
export BTUSB_AUDIO_MODE     ?= 1
export AUDIO_INPUT_MONO     ?= 1
export USB_ISO              ?= 1
export USB_AUDIO_24BIT      ?= 1
export USB_AUDIO_32BIT      ?= 1
export USB_AUDIO_44_1K      ?= 1
export USB_AUDIO_96K        ?= 1
export USB_AUDIO_176_4K     ?= 1
export USB_AUDIO_192K       ?= 1
export USB_AUDIO_352_8K     ?= 1
export USB_AUDIO_384K       ?= 1
export USB_AUDIO_DYN_CFG    ?= 1
export DELAY_STREAM_OPEN    ?= 0
export KEEP_SAME_LATENCY    ?= 1
export HW_FIR_DSD_PROCESS   ?= 1
ifeq ($(HW_FIR_DSD_PROCESS),1)
ifeq ($(CHIP),best2300)
export HW_FIR_DSD_BUF_MID_ADDR  ?= 0x200A0000
export DATA_BUF_START           ?= 0x20040000
endif
endif
export USB_AUDIO_UAC2 ?= 1
export USB_HIGH_SPEED ?= 1
KBUILD_CPPFLAGS += \
    -DHW_FIR_DSD_BUF_MID_ADDR=$(HW_FIR_DSD_BUF_MID_ADDR) \
    -DDATA_BUF_START=$(DATA_BUF_START)
endif

USE_THIRDPARTY ?= 0

export USE_KNOWLES ?= 0

ifeq ($(CURRENT_TEST),1)
export VCODEC_VOLT ?= 1.6V
export VANA_VOLT ?= 1.35V
endif

export BT_XTAL_SYNC ?= 1

export BTADDR_FOR_DEBUG ?= 1

AUTO_TEST ?= 0

export DUMP_NORMAL_LOG ?= 0

export DUMP_CRASH_LOG ?= 0

# three loopback only need one open
export MIC_16K_LOOPBACK ?= 1

export MIC_32K_LOOPBACK ?= 0

export AUDIO_LOOPBACK ?= 0

export OPUS_LOOPBACK ?= 0

export APP_LINEIN_SOURCE ?= 0

export WL_NSX ?= 1

export WL_AGC ?= 0

export WL_AGC_32K ?= 0

export WL_HIGH_SAMPLE ?= 0

export WL_VAD ?= 0

export AUDIO_DEBUG ?= 0

export GCC_PLAT ?= 0

export NOTCH_FILTER ?= 0

export WL_DEBUG_MODE ?= 0

export WL_GPIO_SWITCH ?= 0

export WL_NSX_5MS ?= 0

export WL_TEL_DENOISE ?= 1

export WL_AEC ?= 1

SUPPORT_BATTERY_REPORT ?= 1

SUPPORT_HF_INDICATORS ?= 0

SUPPORT_SIRI ?= 1

init-y      :=
core-y      := platform/ services/ apps/ utils/cqueue/ utils/list/ services/multimedia/ utils/intersyshci/

KBUILD_CPPFLAGS +=  -Iplatform/cmsis/inc \
                    -Iservices/audioflinger \
                    -Iplatform/hal \
                    -Iservices/fs/ \
                    -Iservices/fs/sd \
                    -Iservices/fs/fat \
                    -Iservices/fs/fat/ChaN

KBUILD_CPPFLAGS += \
    -D__APP_KEY_FN_STYLE_A__ \

    # -D__PC_CMD_UART__

#KBUILD_CPPFLAGS += -D__3M_PACK__

#-D_AUTO_SWITCH_POWER_MODE__
#-D__APP_KEY_FN_STYLE_A__
#-D__APP_KEY_FN_STYLE_B__
#-D__EARPHONE_STAY_BOTH_SCAN__
#-D__POWERKEY_CTRL_ONOFF_ONLY__
#-DAUDIO_LINEIN

ifeq ($(CURRENT_TEST),1)
INTSRAM_RUN ?= 1
endif
ifeq ($(INTSRAM_RUN),1)
LDS_FILE    := best1000_intsram.lds
else
LDS_FILE    := best1000.lds
endif

export OTA_SUPPORT_SLAVE_BIN ?= 0
ifeq ($(OTA_SUPPORT_SLAVE_BIN),1)
export SLAVE_BIN_FLASH_OFFSET ?= 0x100000
export SLAVE_BIN_TARGET_NAME ?= anc_usb
endif

ifeq ($(BES_OTA_BASIC),1)
export OTA_BASIC ?= 1
export BES_OTA_BASIC ?= 1
KBUILD_CPPFLAGS += -DBES_OTA_BASIC=1
endif

ifeq ($(TOTA),1)
KBUILD_CPPFLAGS += -DBLE_TOTA_ENABLED
KBUILD_CPPFLAGS += -DTEST_OVER_THE_AIR_ENANBLED
export TEST_OVER_THE_AIR ?= 1
endif

export GSOUND_HOTWORD_ENABLED ?= 0
ifeq ($(GSOUND_HOTWORD_ENABLED), 1)
KBUILD_CFLAGS += -DGSOUND_HOTWORD_ENABLED
endif

ifeq ($(APP_WL_SMARTVOICE),1)
export OPUS_CODEC = 1
KBUILD_CPPFLAGS += -D__APP_WL_SMARTVOICE__ -D__APP_WL_SMARTVOICE_OPUS_ENCODE__ -D__APP_WL_SMARTVOICE_FIX_DEV_NAME__
#-D__APP_WL_SMARTVOICE_OPUS_ENCODE__  #-D__APP_WL_SMARTVOICE_LOOP_TEST__
endif

KBUILD_CPPFLAGS +=

KBUILD_CFLAGS +=

LIB_LDFLAGS += -lstdc++ -lsupc++

#CFLAGS_IMAGE += -u _printf_float -u _scanf_float

#LDFLAGS_IMAGE += --wrap main
