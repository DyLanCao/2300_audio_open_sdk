framework_obj :=

AI_FRAMEWORK_LIB_NAME := libAI_api


ifeq ($(AMA_VOICE),1)
aimethod_obj += protocol/ama/
AI_FRAMEWORK_LIB_NAME := $(AI_FRAMEWORK_LIB_NAME)_AMA
endif

ifeq ($(DMA_VOICE),1)
aimethod_obj += protocol/dma/
AI_FRAMEWORK_LIB_NAME := $(AI_FRAMEWORK_LIB_NAME)_DMA
endif

ifeq ($(SMART_VOICE),1)
aimethod_obj += protocol/smartvoice/
AI_FRAMEWORK_LIB_NAME := $(AI_FRAMEWORK_LIB_NAME)_SMARTVOICE
endif

ifeq ($(TENCENT_VOICE),1)
aimethod_obj += protocol/tencent/
AI_FRAMEWORK_LIB_NAME := $(AI_FRAMEWORK_LIB_NAME)_TENCENT
endif

$(AI_FRAMEWORK_LIB_NAME)-y := $(framework_obj)

obj-y := $(AI_FRAMEWORK_LIB_NAME).a
obj-y += $(aimethod_obj)
