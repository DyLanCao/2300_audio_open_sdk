/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#include "plat_addr_map.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_dma.h"
#include "hal_iomux.h"
#include "hal_gpio.h"
#include "hal_wdt.h"
#include "hal_sleep.h"
#include "hal_bootmode.h"
#include "hal_norflash.h"
#include "hal_trace.h"
#include "hal_location.h"
#include "cmsis.h"
#include "hwtimer_list.h"
#include "pmu.h"
#include "analog.h"
#include "apps.h"
#include "stdlib.h"
#include "tgt_hardware.h"
#include "norflash_api.h"
#include "mpu.h"
#ifdef RTOS
#include "cmsis_os.h"
#include "app_factory.h"
#endif
#include "app_bt_stream.h"

#ifdef FIRMWARE_REV
#define SYS_STORE_FW_VER(x) \
      if(fw_rev_##x) { \
        *fw_rev_##x = fw.softwareRevByte##x; \
      }

typedef struct
{
	uint8_t softwareRevByte0;
	uint8_t softwareRevByte1;
	uint8_t softwareRevByte2;
	uint8_t softwareRevByte3;
} FIRMWARE_REV_INFO_T;

static FIRMWARE_REV_INFO_T fwRevInfoInFlash __attribute((section(".fw_rev"))) = {0, 0, 1, 0};
FIRMWARE_REV_INFO_T fwRevInfoInRam;

extern "C" void system_get_info(uint8_t *fw_rev_0, uint8_t *fw_rev_1,
    uint8_t *fw_rev_2, uint8_t *fw_rev_3)
{
  FIRMWARE_REV_INFO_T fw = fwRevInfoInFlash;

  SYS_STORE_FW_VER(0);
  SYS_STORE_FW_VER(1);
  SYS_STORE_FW_VER(2);
  SYS_STORE_FW_VER(3);
}
#endif

#define  system_shutdown_wdt_config(seconds) \
                        do{ \
                            hal_wdt_stop(HAL_WDT_ID_0); \
                            hal_wdt_set_irq_callback(HAL_WDT_ID_0, NULL); \
                            hal_wdt_set_timeout(HAL_WDT_ID_0, seconds); \
                            hal_wdt_start(HAL_WDT_ID_0); \
                            hal_sleep_set_deep_sleep_hook(HAL_DEEP_SLEEP_HOOK_USER_WDT, NULL); \
                        }while(0)

static osThreadId main_thread_tid = NULL;

int system_shutdown(void)
{
    osThreadSetPriority(main_thread_tid, osPriorityRealtime);
    osSignalSet(main_thread_tid, 0x4);
    return 0;
}

int system_reset(void)
{
    osThreadSetPriority(main_thread_tid, osPriorityRealtime);
    osSignalSet(main_thread_tid, 0x8);
    return 0;
}

int tgt_hardware_setup(void)
{
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)cfg_hw_pinmux_pwl, sizeof(cfg_hw_pinmux_pwl)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
    if (app_battery_ext_charger_indicator_cfg.pin != HAL_IOMUX_PIN_NUM){
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&app_battery_ext_charger_indicator_cfg, 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_battery_ext_charger_indicator_cfg.pin, HAL_GPIO_DIR_OUT, 0);
    }
    return 0;
}

#if defined(ROM_UTILS_ON)
void rom_utils_init(void);
#endif

#ifdef FPGA
uint32_t a2dp_audio_more_data(uint8_t *buf, uint32_t len);
uint32_t a2dp_audio_init(void);
extern "C" void app_audio_manager_open(void);
extern "C" void app_bt_init(void);
extern "C" uint32_t hal_iomux_init(const struct HAL_IOMUX_PIN_FUNCTION_MAP *map, uint32_t count);
void app_overlay_open(void);

extern "C" void BesbtInit(void);
extern "C" int app_os_init(void);
extern "C" uint32_t af_open(void);
extern "C" int list_init(void);
extern "C" void app_audio_open(void);


volatile uint32_t ddddd = 0;

#if defined(AAC_TEST)
#include "app_overlay.h"
int decode_aac_frame_test(unsigned char *pcm_buffer, unsigned int pcm_len);
#define AAC_TEST_PCM_BUFF_LEN (4096)
unsigned char aac_test_pcm_buff[AAC_TEST_PCM_BUFF_LEN];
#endif

#endif

#if defined(_AUTO_TEST_)
extern int32_t at_Init(void);
#endif

#ifdef __USE_PSRAM__
#include "hal_psram.h"
#endif

#if defined(__SBC_FUNC_IN_ROM_VBEST2000__)
extern "C" {
#include "stdarg.h"
#include "patch.h"
#include "string.h"
#include "stdio.h"
}

struct patch_func_item {
    void *old_func;
    void *new_func;
};
static uint32_t ALIGNED(0x100) remap_base[256];
char p_romsbc_trace_buffer[128];
SRAM_TEXT_LOC int p_hal_trace_printf_without_crlf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsprintf(p_romsbc_trace_buffer,fmt,args);
    va_end(args);

    TRACE("%s\n", p_romsbc_trace_buffer);

    return 0;
}
SRAM_TEXT_LOC void p_OS_MemSet(U8 *dest, U8 byte, U32 len)
{
	memset(dest, byte, len);
}
SRAM_TEXT_LOC void p_OS_Assert(const char *expression, const char *file, U16 line)
{
    ASSERT(0,"OS_Assert exp: %s, func: %s, line %d\r\n", expression, file, line);
    while(1);
}
SRAM_TEXT_LOC void* p_memcpy(void *dst,const void *src, size_t length)
{
	return memcpy(dst, src, length);
}
struct patch_func_item romsbc_patchs[] = {
    // OS_Memset
    {(void *)0x0003a54d, (void *)p_OS_MemSet},
    // OS_Assert
    {(void *)0x0003a571, (void *)p_OS_Assert},
    // hal_trace_printf_without_crlf
    {(void *)0x0003a601, (void *)p_hal_trace_printf_without_crlf},
    // memcpy
    {(void *)0x0003a635, (void *)p_memcpy},
};

void init_2200_romprofile_sbc_patch(void)
{
    uint32_t i = 0;
    patch_open((uint32_t)remap_base);
    for(i = 0; i < sizeof(romsbc_patchs)/sizeof(struct patch_func_item); ++i) {
        patch_enable(PATCH_TYPE_FUNC, (uint32_t)romsbc_patchs[i].old_func, (uint32_t)romsbc_patchs[i].new_func);
    }
}
#endif

#if defined(DUMP_LOG_ENABLE) 
extern "C" void log_dump_init(void);
#endif
#if defined(DUMP_CRASH_ENABLE)
extern "C" void crash_dump_init(void);
// demo of gsound crash dump.
extern "C" void gsound_crash_dump_init(void);
#endif
int main(int argc, char *argv[])
{
    uint8_t sys_case = 0;
    int ret = 0;
#ifdef __FACTORY_MODE_SUPPORT__
    uint32_t bootmode = hal_sw_bootmode_get();
#endif

    tgt_hardware_setup();

#if defined(ROM_UTILS_ON)
    rom_utils_init();
#endif

    main_thread_tid = osThreadGetId();

    hwtimer_init();

    hal_dma_set_delay_func((HAL_DMA_DELAY_FUNC)osDelay);
    hal_audma_open();
    hal_gpdma_open();
    norflash_api_init();
#if defined(DUMP_LOG_ENABLE)
    log_dump_init();
#endif
#if defined(DUMP_CRASH_ENABLE)
    crash_dump_init();
    gsound_crash_dump_init();
#endif

#ifdef DEBUG
#if (DEBUG_PORT == 1)
    hal_iomux_set_uart0();
#ifdef __FACTORY_MODE_SUPPORT__
    if (!(bootmode & HAL_SW_BOOTMODE_FACTORY))
#endif
    {
        hal_trace_open(HAL_TRACE_TRANSPORT_UART0);
    }
#endif

#if (DEBUG_PORT == 2)
#ifdef __FACTORY_MODE_SUPPORT__
    if (!(bootmode & HAL_SW_BOOTMODE_FACTORY))
#endif
    {
        hal_iomux_set_analog_i2c();
    }
    hal_iomux_set_uart1();
    hal_trace_open(HAL_TRACE_TRANSPORT_UART1);
#endif
    hal_sleep_start_stats(10000, 10000);
#endif

    hal_iomux_ispi_access_init();

#ifndef FPGA
    uint8_t flash_id[HAL_NORFLASH_DEVICE_ID_LEN];
    hal_norflash_get_id(HAL_NORFLASH_ID_0, flash_id, ARRAY_SIZE(flash_id));
    TRACE("FLASH_ID: %02X-%02X-%02X", flash_id[0], flash_id[1], flash_id[2]);
    ASSERT(hal_norflash_opened(HAL_NORFLASH_ID_0), "Failed to init flash: %d", hal_norflash_get_open_state(HAL_NORFLASH_ID_0));

	// Software will load the factory data and user data from the bottom TWO sectors from the flash,
	// the FLASH_SIZE defined is the common.mk must be equal or greater than the actual chip flash size,
	// otherwise the ota will load the wrong information
	uint32_t actualFlashSize = hal_norflash_get_flash_total_size(HAL_NORFLASH_ID_0);
	if (FLASH_SIZE > actualFlashSize)
	{
		TRACE("Wrong FLASH_SIZE defined in target.mk!");
		TRACE("FLASH_SIZE is defined as 0x%x while the actual chip flash size is 0x%x!", FLASH_SIZE, actualFlashSize);
		TRACE("Please change the FLASH_SIZE in common.mk to 0x%x to enable the OTA feature.", actualFlashSize);
		ASSERT(false, "");
	}
#endif

    pmu_open();

    analog_open();

    ret = mpu_open();
    if (ret == 0)
        mpu_null_check_enable();

#if defined(__SBC_FUNC_IN_ROM_VBEST2000__)

    init_2200_romprofile_sbc_patch();
#endif
    srand(hal_sys_timer_get());

#if defined(_AUTO_TEST_)
    at_Init();
#endif

    app_audio_buffer_check();


#ifdef __USE_PSRAM__
    hal_psram_init();
#endif

#ifdef FPGA

    TRACE("\n[best of best of best...]\n");
    TRACE("\n[ps: w4 0x%x,2]", &ddddd);

    ddddd = 1;
    while (ddddd == 1);
    TRACE("bt start");

    list_init();

    app_os_init();
    app_bt_init();
    a2dp_audio_init();

    af_open();
    app_audio_open();
    app_audio_manager_open();
    app_overlay_open();

#if defined(AAC_TEST)
    app_overlay_select(APP_OVERLAY_A2DP_AAC);
    decode_aac_frame_test(aac_test_pcm_buff, AAC_TEST_PCM_BUFF_LEN);
#endif

    SAFE_PROGRAM_STOP();

#else // !FPGA

#ifdef __FACTORY_MODE_SUPPORT__
    if (bootmode & HAL_SW_BOOTMODE_FACTORY){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_FACTORY);
        ret = app_factorymode_init(bootmode);

    }else if(bootmode & HAL_SW_BOOTMODE_CALIB){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_CALIB);
        ret = app_factorymode_calib_only();
    }
#ifdef __USB_COMM__
    else if(bootmode & HAL_SW_BOOTMODE_CDC_COMM)
    {
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_CDC_COMM);
        ret = app_factorymode_cdc_comm();
    }
#endif
    else
#endif
    {
#ifdef FIRMWARE_REV
        fwRevInfoInRam = fwRevInfoInFlash;
        TRACE("The Firmware rev is %d.%d.%d.%d",
        fwRevInfoInRam.softwareRevByte0,
        fwRevInfoInRam.softwareRevByte1,
        fwRevInfoInRam.softwareRevByte2,
        fwRevInfoInRam.softwareRevByte3);
#endif
        ret = app_init();
    }
    if (!ret){
#if defined(_AUTO_TEST_)
        AUTO_TEST_SEND("BT Init ok.");
#endif
        while(1)
        {
            osEvent evt;
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
            osSignalClear (main_thread_tid, 0x0f);
#endif
            //wait any signal
            evt = osSignalWait(0x0, osWaitForever);

            //get role from signal value
            if(evt.status == osEventSignal)
            {
                if(evt.value.signals & 0x04)
                {
                    sys_case = 1;
                    break;
                }
                else if(evt.value.signals & 0x08)
                {
                    sys_case = 2;
                    break;
                }
            }else{
                sys_case = 1;
                break;
            }
         }
    }
#ifdef __WATCHER_DOG_RESET__
    system_shutdown_wdt_config(10);
#endif
    app_deinit(ret);
    TRACE("byebye~~~ %d\n", sys_case);
    if ((sys_case == 1)||(sys_case == 0)){
        TRACE("shutdown\n");
#if defined(_AUTO_TEST_)
        AUTO_TEST_SEND("System shutdown.");
        osDelay(50);
#endif
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
        pmu_shutdown();
    }else if (sys_case == 2){
        TRACE("reset\n");
#if defined(_AUTO_TEST_)
        AUTO_TEST_SEND("System reset.");
        osDelay(50);
#endif
        hal_cmu_sys_reboot();
    }

#endif // !FPGA

    return 0;
}

