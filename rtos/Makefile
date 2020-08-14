ifeq ($(KERNEL),RTX)
obj-y := rtx/TARGET_CORTEX_M/
else ifeq ($(KERNEL),FREERTOS)
obj-y := freertos/ svc_sleep/
endif

ifeq ($(MBED),1)
obj-y += rtos/
endif

subdir-ccflags-y += -Iplatform/cmsis/inc -Iplatform/hal -Iutils/hwtimer_list
subdir-asflags-y += -Iplatform/cmsis/inc
