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
#ifndef _CODEC_LHDC_H
#define _CODEC_LHDC_H

#define BTIF_A2DP_LHDC_OCTET_NUMBER                     (9)
#define BTIF_A2DP_LHDC_VENDOR_ID                       0x0000053a
#define BTIF_A2DP_LHDC_CODEC_ID                        0x4C32
//To indicate Sampling Rate.
#define BTIF_A2DP_LHDC_SR_96000                        0x01
#define BTIF_A2DP_LHDC_SR_88200                        0x02
#define BTIF_A2DP_LHDC_SR_48000                        0x04
#define BTIF_A2DP_LHDC_SR_44100                        0x08
#define BTIF_A2DP_LHDC_SR_DATA(X)                      (X & (BTIF_A2DP_LHDC_SR_96000 | BTIF_A2DP_LHDC_SR_88200 | BTIF_A2DP_LHDC_SR_48000 | BTIF_A2DP_LHDC_SR_44100))

//To indicate bits per sample.
#define BTIF_A2DP_LHDC_FMT_24                          0x10
#define BTIF_A2DP_LHDC_FMT_16                          0x20
#define BTIF_A2DP_LHDC_FMT_DATA(X)                     (X & (BTIF_A2DP_LHDC_FMT_24 | BTIF_A2DP_LHDC_FMT_16))

#define BTIF_A2DP_LHDC_VERSION_NUM                      0x00
#define BTIF_A2DP_LHDC_MAX_SR_900                       0x00
#define BTIF_A2DP_LHDC_MAX_SR_500                       0x10
#define BTIF_A2DP_LHDC_MAX_SR_400                       0x20
#define BTIF_A2DP_LHDC_MAX_SR_RESERVED                  0x30
#define BTIF_A2DP_LHDC_LLC_ENABLE                       0x40

#define BTIF_A2DP_LHDC_COF_CSC_DISABLE                  0x01
#define BTIF_A2DP_LHDC_COF_CSC                          0x02 //
#define BTIF_A2DP_LHDC_COF_CSC_PRE                      0x04
#define BTIF_A2DP_LHDC_COF_CSC_RESERVED                 0x08
#define BTIF_A2DP_LHDC_COF_DATA(X)                     (X & (BTIF_A2DP_LHDC_COF_CSC_DISABLE | BTIF_A2DP_LHDC_COF_CSC | BTIF_A2DP_LHDC_COF_CSC_PRE))
typedef enum {
    LHDC_CHANNEL_SPLIT_DISABLE = 0,
    LHDC_CHANNEL_SPLIT,         //For forwarding type TWS used
    LHDC_CHANNEL_SPLIT_FROM_ENCODER,    // Pre-split left/right frame at encode side.
    LHDC_CHANNEL_SPLIT_INVALID,
} compressor_output_format_t;

typedef struct {
    uint32_t vendor_id;
    uint16_t codec_id;
    uint8_t bits;
    uint8_t sample_rater;       //uint:K
    uint8_t version_num;
    uint16_t max_sample_rate;   //uint:K  
    bool llc_enable;            //low latency control
    compressor_output_format_t compress_output_format;
} lhdc_info_t;

#ifdef __cplusplus
extern "C" {
#endif                          /*  */

    void lhdc_info_parse(uint8_t * elements, lhdc_info_t * info);
    uint8_t a2dp_lhdc_get_sample_rate(uint8_t * elements);

#ifdef __cplusplus
}
#endif

#endif

