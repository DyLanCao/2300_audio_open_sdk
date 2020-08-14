#ifndef __APP_WL_SMARTVOICE_ENCODE_H
#define __APP_WL_SMARTVOICE_ENCODE_H

#include "hal_trace.h"

// opus config
#ifndef __APP_WL_SMARTVOICE_LOOP_TEST__
#define WL_SMARTVOICE_OPUS_CONFIG_HEAP_SIZE                (32*1024)
#else
#define WL_SMARTVOICE_OPUS_CONFIG_HEAP_SIZE                (64*1024)
#endif

#define WL_SMARTVOICE_OPUS_CONFIG_CHANNEL_COUNT            (1)
#define WL_SMARTVOICE_OPUS_CONFIG_COMPLEXITY               (0)
#define WL_SMARTVOICE_OPUS_CONFIG_PACKET_LOSS_PERC         (0)
#define WL_SMARTVOICE_OPUS_CONFIG_SIZE_PER_SAMPLE          (2)
#define WL_SMARTVOICE_OPUS_CONFIG_APP                      (OPUS_APPLICATION_VOIP)
#define WL_SMARTVOICE_OPUS_CONFIG_BANDWIDTH                (OPUS_BANDWIDTH_WIDEBAND)
#define WL_SMARTVOICE_OPUS_CONFIG_BITRATE                  (16000)
#define WL_SMARTVOICE_OPUS_CONFIG_SAMPLE_RATE              (16000)
#define WL_SMARTVOICE_OPUS_CONFIG_SIGNAL_TYPE              (OPUS_SIGNAL_VOICE)
#define WL_SMARTVOICE_OPUS_CONFIG_FRAME_PERIOD             (OPUS_FRAMESIZE_20_MS)
#define WL_SMARTVOICE_OPUS_CONFIG_USE_VBR                  (0)
#define WL_SMARTVOICE_OPUS_CONFIG_CONSTRAINT_USE_VBR       (0)
#define WL_SMARTVOICE_OPUS_CONFIG_USE_INBANDFEC            (0)
#define WL_SMARTVOICE_OPUS_CONFIG_USE_DTX                  (0)

#ifdef __cplusplus
extern "C" {
#endif

void wl_smartvoice_opus_init(uint8_t* opus_heap);
void wl_smartvoice_opus_deinit(void);
int wl_smartvoice_opus_encode(uint8_t* in, uint8_t* out, uint32_t len);
int wl_smartvoice_opus_decode(uint8_t* in, uint32_t in_len,uint8_t* out, uint32_t out_len);

void wl_smartvoice_sbc_init(void);
void wl_smartvoice_sbc_deinit(void);
int wl_smartvoice_sbc_decode(uint8_t* in, uint32_t in_len,uint8_t* out, uint32_t out_len);
int wl_smartvoice_sbc_encode(uint8_t* in, uint32_t in_len,uint8_t* out, uint32_t out_len);

#ifdef __cplusplus
}
#endif

#endif