cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

#obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))

ifeq ($(DSP_LIB_ALL_ENABLE),1)

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*/Makefile))
obj-y := $(obj-y:/Makefile=/)

else

obj-y :=
obj-y += \
	CommonTables/ \
	BasicMathFunctions/arm_dot_prod_q15.c \
	FilteringFunctions/arm_biquad_cascade_df1_init_f32.c \
	FilteringFunctions/arm_biquad_cascade_df1_f32.c \
	FilteringFunctions/arm_fir_init_q15.c \
	FilteringFunctions/arm_fir_fast_q15.c \
	TransformFunctions/arm_bitreversal2.S \
	TransformFunctions/arm_cfft_f32.c \
	TransformFunctions/arm_cfft_q15.c \
	TransformFunctions/arm_cfft_radix4_q15.c \
	TransformFunctions/arm_cfft_radix8_f32.c \
	TransformFunctions/arm_rfft_fast_init_f32.c \
	TransformFunctions/arm_rfft_fast_f32.c \
	TransformFunctions/arm_rfft_init_q15.c \
	TransformFunctions/arm_rfft_q15.c \
        SupportFunctions/arm_copy_q31.c \
        SupportFunctions/arm_fill_q31.c \
        BasicMathFunctions/arm_dot_prod_q7.c \
        StatisticsFunctions/

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

endif

