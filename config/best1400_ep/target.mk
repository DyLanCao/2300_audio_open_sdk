CHIP		?= best1400

DEBUG		?= 1

FPGA		?= 0

MBED		?= 0

RTOS		?= 1

export USER_SECURE_BOOT	?= 0
# enable:1
# disable:0

WATCHER_DOG ?= 0

DEBUG_PORT	?= 1
# 0: usb
# 1: uart0
# 2: uart1

FLASH_CHIP	?= ALL
# GD25Q80C
# GD25Q32C
# ALL

export BTDUMP_ENABLE ?= 1

AUDIO_OUTPUT_MONO ?= 0

AUDIO_OUTPUT_DIFF ?= 0

HW_FIR_EQ_PROCESS ?= 0

SW_IIR_EQ_PROCESS ?= 0

HW_DAC_IIR_EQ_PROCESS ?= 1

HW_IIR_EQ_PROCESS ?= 0

AUDIO_DRC ?= 0

AUDIO_DRC2 ?= 0

AUDIO_RESAMPLE ?= 1

RESAMPLE_ANY_SAMPLE_RATE ?= 1

OSC_26M_X4_AUD2BB ?= 1

export ANA_26M_X6_ENABLE ?= 0

# big digital use digital 26x4 as clock source
# 1  sys and flash will use this clock as source
# 0  sys and flash will use analog x4/x6 (rf 0xF reg) clock as source
DIGX4_AS_HIGHSPEED_CLK ?= 0

# big digital use digital 26x2 as 52M clock source
# 1  sys and flash will use this clock as source
# 0  sys and flash will use analog x2 (diff port from x4/x6) clock as source
DIGX2_AS_52M_CLK ?= 0

AUDIO_OUTPUT_VOLUME_DEFAULT ?= 17
# range:1~16

AUDIO_INPUT_CAPLESSMODE ?= 0

AUDIO_INPUT_LARGEGAIN ?= 0

AUDIO_CODEC_ASYNC_CLOSE ?= 1

AUDIO_SCO_BTPCM_CHANNEL ?= 1

#for tws sco and low delay sco
#export SCO_DMA_SNAPSHOT ?= 1

#export CVSD_BYPASS ?= 1

#export LOW_DELAY_SCO ?= 1

SCO_OPTIMIZE_FOR_RAM ?= 1

SPEECH_TX_DC_FILTER ?= 0

SPEECH_TX_AEC2FLOAT ?= 0

SPEECH_TX_NS2FLOAT ?= 0

SPEECH_TX_2MIC_NS2 ?= 0

SPEECH_TX_COMPEXP ?= 0

SPEECH_TX_EQ ?= 0

SPEECH_TX_POST_GAIN ?= 0

SPEECH_RX_PLC ?= 1

SPEECH_RX_NS2FLOAT ?= 0

SPEECH_RX_EQ ?= 0

SPEECH_RX_POST_GAIN ?= 0

LARGE_RAM ?= 1

HSP_ENABLE ?= 0

HFP_1_6_ENABLE ?= 1

MSBC_PLC_ENABLE ?= 1

MSBC_PLC_ENCODER ?= 1

MSBC_16K_SAMPLE_RATE ?= 1

SBC_FUNC_IN_ROM ?= 0

ROM_UTILS_ON ?= 1

APP_LINEIN_A2DP_SOURCE ?= 0

APP_I2S_A2DP_SOURCE ?= 0

VOICE_PROMPT ?= 1

BISTO ?= 0

AI_VOICE ?= 0

AMA_VOICE ?= 0

DMA_VOICE ?= 0

SMART_VOICE ?= 0

TENCENT_VOICE ?= 0

ifeq ($(AI_VOICE),1)
OSC_26M_X4_AUD2BB ?= 0
endif

BLE ?= 0

BT_ONE_BRING_TWO ?= 1

DSD_SUPPORT ?= 0

A2DP_LHDC_ON ?= 0

A2DP_AAC_ON ?= 1

BES_OTA_BASIC ?= 0

export TX_RX_PCM_MASK ?= 0

A2DP_SCALABLE_ON ?= 0

FACTORY_MODE ?= 1

ENGINEER_MODE ?= 1

ULTRA_LOW_POWER	?= 1

DAC_CLASSG_ENABLE ?= 1

NO_SLEEP ?= 0

export POWER_MODE	?= DIG_DCDC

export SPEECH_CODEC ?= 1

USE_THIRDPARTY ?= 0

export VCODEC_VOLT ?= 1.66V
#export VANA_VOLT ?= 1.35V

AAC_TEXT_PARTIAL_IN_FLASH ?= 1

export BT_XTAL_SYNC ?= 1

AUTO_TEST ?= 0

init-y		:=
core-y		:= platform/ services/ apps/ utils/cqueue/ utils/list/ services/multimedia/ utils/intersyshci/

KBUILD_CPPFLAGS += -Iplatform/cmsis/inc -Iservices/audioflinger -Iplatform/hal -Iservices/fs/ -Iservices/fs/sd -Iservices/fs/fat  -Iservices/fs/fat/ChaN

KBUILD_CPPFLAGS += \
    -D__APP_KEY_FN_STYLE_A__ \
	-D__EARPHONE_STAY_BOTH_SCAN__

#    -D__PC_CMD_UART__

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
LDS_FILE	:= best1000_intsram.lds
else
LDS_FILE	:= best1000.lds
endif

ifeq ($(BES_OTA_BASIC),1)
export OTA_BASIC ?= 1
export BES_OTA_BASIC ?= 1
KBUILD_CPPFLAGS += -DBES_OTA_BASIC=1
endif

KBUILD_CPPFLAGS +=

KBUILD_CFLAGS +=

LIB_LDFLAGS += -lstdc++ -lsupc++

#CFLAGS_IMAGE += -u _printf_float -u _scanf_float

#LDFLAGS_IMAGE += --wrap main

