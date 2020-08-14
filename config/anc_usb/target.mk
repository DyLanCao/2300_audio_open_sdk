CHIP		?= best2300

DEBUG		?= 1

FPGA		?= 0

FULL_APP	?= 0

ifeq ($(FULL_APP),1)
RTOS		?= 1
APP_TEST_MODE	?= 1
APP_TEST_AUDIO	?= 1
endif

ifeq ($(RTOS),1)
MBED		?= 1
else
export INTSRAM_RUN	?= 1
endif

ifeq ($(INTSRAM_RUN),1)
ULTRA_LOW_POWER	?= 0
else
ULTRA_LOW_POWER	?= 0
endif

NOSTD		?= 0

DEBUG_PORT	?= 1

FLASH_CHIP	?= ALL

export AUDIO_OUTPUT_DIFF ?= 1

export AUDIO_ANC_FB_MC ?= 0

export ANC_FB_CHECK ?= 0

export HW_FIR_DSD_PROCESS			?= 0
ifeq ($(HW_FIR_DSD_PROCESS),1)
ifeq ($(CHIP),best2300)
export HW_FIR_DSD_BUF_MID_ADDR		?= 0x200A0000
export RAM_SIZE						?= 0x00080000
export DATA_BUF_START				?= 0x20040000
endif
endif

export HW_FIR_EQ_PROCESS			?= 0

export SW_IIR_EQ_PROCESS			?= 0

export HW_DAC_IIR_EQ_PROCESS		?= 0

export HW_IIR_EQ_PROCESS			?= 0

export USB_AUDIO_APP ?= 1

ifeq ($(CHIP),best1400)
export ANA_26M_X6_ENABLE ?= 1
endif

ifneq ($(filter best3001 best1400, $(CHIP)),)
export ANC_APP ?= 0
else
export ANC_APP ?= 1
endif

ifeq ($(ANC_APP),1)
ifeq ($(CHIP),best1000)
AUD_PLL_DOUBLE	?= 0
DUAL_AUX_MIC	?= 1
else
export ANC_FF_ENABLED	?= 1
export ANC_FB_ENABLED	?= 1
export AUDIO_SECTION_SUPPT ?= 1
endif
export ANC_INIT_SPEEDUP	?= 1
endif

export NO_LIGHT_SLEEP	?= 0

export USB_ISO ?= 1

export USB_HIGH_SPEED ?= 0

export USB_AUDIO_UAC2 ?= 0

export USB_AUDIO_96K	?= 0

export USB_AUDIO_192K	?= 0

export USB_AUDIO_384K	?= 0

export USB_AUDIO_24BIT	?= 0

export USB_AUDIO_DYN_CFG ?= 1

export DELAY_STREAM_OPEN ?= 0

export SPEECH_TX_DC_FILTER ?= 0

export SPEECH_TX_AEC2FLOAT ?= 0

export SPEECH_TX_NS2FLOAT ?= 0

export SPEECH_TX_2MIC_NS2 ?= 0

export SPEECH_TX_COMPEXP ?= 0

export SPEECH_TX_EQ ?= 0

export SPEECH_TX_POST_GAIN ?= 0

export POWER_MODE	?= DIG_DCDC

export BLE	:= 0

export MIC_KEY   ?= 0

LETV		?= 0
ifeq ($(LETV),1)
export USB_AUDIO_VENDOR_ID	?= 0x262A
export USB_AUDIO_PRODUCT_ID	?= 0x1534
endif

export PC_CMD_UART	?= 0

export FACTORY_SECTION_SUPPT    ?= 0

export SPEECH_TX_2MIC_NS3 ?= 0

init-y		:=
ifeq ($(FULL_APP),1)
core-y		:= platform/ services/ apps/ utils/cqueue/ utils/list/ services/multimedia/ utils/intersyshci/
KBUILD_CPPFLAGS += -Iplatform/cmsis/inc -Iservices/audioflinger -Iplatform/hal -Iservices/fs/ -Iservices/fs/sd -Iservices/fs/fat -Iservices/fs/fat/ChaN
else
core-y		:= tests/anc_usb/ platform/cmsis/ platform/hal/ platform/drivers/usb/usb_dev/ platform/drivers/norflash/ platform/drivers/ana/ platform/drivers/codec/ services/audioflinger/ utils/hwtimer_list/
KBUILD_CPPFLAGS += -Iplatform/cmsis/inc -Iplatform/hal -Iplatform/drivers/ana -Iservices/audioflinger
ifeq ($(RTOS),1)
core-y		+= services/fs/
KBUILD_CPPFLAGS += -Iservices/fs/fat/ChaN
endif
endif

ifeq ($(BLE),1)
core-y		+= utils/jansson/
KBUILD_CPPFLAGS += -D__IAG_BLE_INCLUDE__=1
#KBUILD_CPPFLAGS += -DGATT_CLIENT=1
endif

KBUILD_CPPFLAGS +=
#-D_AUTO_SWITCH_POWER_MODE__
#-D_BEST1000_QUAL_DCDC_
#-DSPEECH_TX_AEC
#-DSPEECH_TX_NS
#-DMEDIA_PLAYER_SUPPORT

ifeq ($(INTSRAM_RUN),1)
LDS_FILE	:= best1000_intsram.lds
else
LDS_FILE	:= best1000.lds
endif

KBUILD_CPPFLAGS +=

ifeq ($(MIC_KEY), 1)
KBUILD_CFLAGS += -DCFG_MIC_KEY
endif

ifeq ($(OTAGEN_SLAVE_BIN),1)
export OTA_CODE_OFFSET ?= 0x18000+0x100000
endif

LIB_LDFLAGS += -lstdc++ -lsupc++

#CFLAGS_IMAGE += -u _printf_float -u _scanf_float

#LDFLAGS_IMAGE += --wrap main

