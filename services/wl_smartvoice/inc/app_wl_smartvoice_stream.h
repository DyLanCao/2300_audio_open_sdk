#ifndef __APP_WL_SMARTVOICE_STREAM_H
#define __APP_WL_SMARTVOICE_STREAM_H

#include "app_utils.h"
#include "app_wl_smartvoice_encode.h"

#define APP_WL_SMARTVOICE_OPUS_SAMPLE_RATE      AUD_SAMPRATE_16000
#define APP_WL_SMARTVOICE_OPUS_CHANNEL_NUM      AUD_CHANNEL_NUM_1
#define APP_WL_SMARTVOICE_OPUS_BITS_NUM         AUD_BITS_16

#define APP_WL_SMARTVOICE_FRAME_TIME_20_MS      (20)

#define APP_WL_SMARTVOICE_FRAME_SIZE            (APP_WL_SMARTVOICE_OPUS_SAMPLE_RATE/1000)*(APP_WL_SMARTVOICE_OPUS_BITS_NUM/8)*APP_WL_SMARTVOICE_OPUS_CHANNEL_NUM*APP_WL_SMARTVOICE_FRAME_TIME_20_MS

#define APP_WL_SMARTVOICE_OPUS_FRAME_ENCODE_SIZE    (APP_WL_SMARTVOICE_FRAME_SIZE/16)

#define TO_PINGPONG_BUFF_SIZE(size)     ((size)*2)

#define APP_WL_SMARTVOICE_TRANSFER_FRAME_SIZE       (APP_WL_SMARTVOICE_OPUS_FRAME_ENCODE_SIZE*3)

#define APP_WL_SMARTVOICE_LOOP_TEST_BUFF_SIZE    	(1024*10)

#ifdef __cplusplus
extern "C" {
#endif

void app_wl_smartvoice_start_stream(void);
void app_wl_smartvoice_stop_stream(void);

bool app_wl_smartvoice_is_gonging_on(void);

void app_wl_smartvoice_stream_init(void);

#ifdef __cplusplus
}
#endif

#endif