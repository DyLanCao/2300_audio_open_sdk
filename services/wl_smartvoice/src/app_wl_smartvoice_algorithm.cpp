#include "app_wl_smartvoice_algorithm.h"
#include "app_wl_smartvoice.h"
#include "string.h"
#include "hal_timer.h"

#ifdef WL_NSX
#include "nsx_main.h"
#include "app_overlay.h"
#endif

#define APP_WL_SMARTVOICE_SAMPLE_COUNT_OF_10MS              (160)
static bool b_nsx_denoise = true;
static uint8_t nsx_level = 2;
void app_wl_smartvoice_algorithm_disable(void)
{
    b_nsx_denoise = false;
}

void app_wl_smartvoice_algorithm_enable(void)
{
    b_nsx_denoise = true;
}

void app_wl_smartvoice_algorithm_denoise_level_set(uint8_t level)
{
    nsx_level = level;
}
void app_wl_smartvoice_algorithm_init(uint8_t* nsx_heap)
{
#ifdef WL_NSX
    wl_nsx_denoise_init(16000, nsx_level, nsx_heap);
#endif
}

void app_wl_smartvoice_algorithm_deinit(void)
{
    
}

void app_wl_smartvoice_algorithm_process(uint8_t* data, uint16_t len)
{
#ifdef WL_NSX
    uint16_t sample_cnt;
    //uint32_t now;
    short* in;
    static short out[APP_WL_SMARTVOICE_SAMPLE_COUNT_OF_10MS];

    in = (short*)data;
    sample_cnt = len/2;


    if (b_nsx_denoise) {
        for (int i=0; i<sample_cnt/APP_WL_SMARTVOICE_SAMPLE_COUNT_OF_10MS; i++) {
            //now = hal_sys_timer_get();
            wl_nsx_16k_denoise(in+i*APP_WL_SMARTVOICE_SAMPLE_COUNT_OF_10MS, out);
            //WL_SV_DEBUG("denoise, i: %d %d ms", i, TICKS_TO_MS(hal_sys_timer_get() - now));
            memcpy(in+i*APP_WL_SMARTVOICE_SAMPLE_COUNT_OF_10MS, out, sizeof(out));
        }
    }
#endif
}