cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj-y := $(obj-y:.c=.o)

CFLAGS_hal_cmu_$(CHIP).o += $(CMU_CFG_FLAGS)

CFLAGS_hal_codec_$(CHIP).o += $(CODEC_CFG_FLAGS)

CFLAGS_hal_iomux_$(CHIP).o += $(IOMUX_CFG_FLAGS)

