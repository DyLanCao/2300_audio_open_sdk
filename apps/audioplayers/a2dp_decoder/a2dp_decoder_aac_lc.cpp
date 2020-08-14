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
// Standard C Included Files
#include "cmsis.h"
#include "plat_types.h"
#include <string.h>
#include "heap_api.h"
#include "hal_location.h"
#include "a2dp_decoder_internal.h"

#if defined(A2DP_AAC_ON)
#include "heap_api.h"
#include "aacdecoder_lib.h"
#include "aacenc_lib.h"


#define AAC_MTU_LIMITER (32) /*must <= 23*/

#define AAC_OUTPUT_FRAME_SAMPLES (1024)
#define DECODE_AAC_PCM_FRAME_LENGTH  (2048)
#define AAC_READBUF_SIZE (1024)
#define AAC_MEMPOOL_SIZE (36500)

static A2DP_AUDIO_CONTEXT_T *a2dp_audio_context_p = NULL;
static A2DP_AUDIO_OUTPUT_CONFIG_T a2dp_audio_aac_output_config;

static HANDLE_AACDECODER aacDec_handle = NULL;
static uint8_t *aac_mempoll = NULL;
heap_handle_t aac_memhandle = NULL;

static A2DP_AUDIO_DECODER_LASTFRAME_INFO_T a2dp_audio_aac_lastframe_info;

static uint16_t aac_mtu_limiter = AAC_MTU_LIMITER;

typedef struct {
    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint8_t *aac_buffer;
    uint32_t aac_buffer_len;
} a2dp_audio_aac_decoder_frame_t;

static void a2dp_audio_aac_lc_decoder_init(void)
{
    if (aacDec_handle == NULL){
        TRANSPORT_TYPE transportFmt = TT_MP4_LATM_MCP1;

        aacDec_handle = aacDecoder_Open(transportFmt, 1 /* nrOfLayers */);
        A2DP_DECODER_ASSERT(aacDec_handle, "aacDecoder_Open failed");

        aacDecoder_SetParam(aacDec_handle,AAC_PCM_LIMITER_ENABLE,0);
        aacDecoder_SetParam(aacDec_handle, AAC_DRC_ATTENUATION_FACTOR, 0);
        aacDecoder_SetParam(aacDec_handle, AAC_DRC_BOOST_FACTOR, 0);
    }
}

static void a2dp_audio_aac_lc_decoder_deinit(void)
{
    if (aacDec_handle){
        aacDecoder_Close(aacDec_handle);
        aacDec_handle = NULL;
    }
}

static void a2dp_audio_aac_lc_decoder_reinit(void)
{
    if (aacDec_handle){
        a2dp_audio_aac_lc_decoder_deinit();
    }
    a2dp_audio_aac_lc_decoder_init();
}

#ifdef A2DP_CP_ACCEL
struct A2DP_CP_AAC_LC_IN_FRM_INFO_T {
    uint16_t sequenceNumber;
    uint32_t timestamp;
};

struct A2DP_CP_AAC_LC_OUT_FRM_INFO_T {
    struct A2DP_CP_AAC_LC_IN_FRM_INFO_T in_info;
    uint16_t frame_samples;
    uint16_t decoded_frames;
    uint16_t frame_idx;
    uint16_t pcm_len;
};

static bool cp_codec_reset;

TEXT_AAC_LOC
static int a2dp_cp_aac_lc_mcu_decode(uint8_t *buffer, uint32_t buffer_bytes)
{
    a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;
    list_node_t *node = NULL;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    int ret, dec_ret;
    struct A2DP_CP_AAC_LC_IN_FRM_INFO_T in_info;
    struct A2DP_CP_AAC_LC_OUT_FRM_INFO_T *p_out_info;
    uint8_t *out;
    uint32_t out_len;
    uint32_t out_frame_len;

    out_frame_len = sizeof(*p_out_info) + buffer_bytes;

    ret = a2dp_cp_decoder_init(out_frame_len);
    ASSERT(ret == 0, "%s: a2dp_cp_decoder_init() failed: ret=%d", __func__, ret);

    while ((node = a2dp_audio_list_begin(list)) != NULL) {
        aac_decoder_frame_p = (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_list_node(node);

        in_info.sequenceNumber = aac_decoder_frame_p->sequenceNumber;
        in_info.timestamp = aac_decoder_frame_p->timestamp;

        ret = a2dp_cp_put_in_frame(&in_info, sizeof(in_info), aac_decoder_frame_p->aac_buffer, aac_decoder_frame_p->aac_buffer_len);
        if (ret) {
            break;
        }

        a2dp_audio_list_remove(list, aac_decoder_frame_p);
    }

    ret = a2dp_cp_get_full_out_frame((void **)&out, &out_len);
    if (ret) {
        return A2DP_DECODER_DECODE_ERROR;
    }
    if (out_len == 0) {
        memset(buffer, 0, buffer_bytes);
        a2dp_cp_consume_full_out_frame();
        return A2DP_DECODER_NO_ERROR;
    }
    ASSERT(out_len == out_frame_len, "%s: Bad out len %u (should be %u)", __func__, out_len, out_frame_len);

    p_out_info = (struct A2DP_CP_AAC_LC_OUT_FRM_INFO_T *)out;
    if (p_out_info->pcm_len) {
        a2dp_audio_aac_lastframe_info.sequenceNumber = p_out_info->in_info.sequenceNumber;
        a2dp_audio_aac_lastframe_info.timestamp = p_out_info->in_info.timestamp;
        a2dp_audio_aac_lastframe_info.curSubSequenceNumber = 0;
        a2dp_audio_aac_lastframe_info.totalSubSequenceNumber = 0;
        a2dp_audio_aac_lastframe_info.frame_samples = p_out_info->frame_samples;
        a2dp_audio_aac_lastframe_info.decoded_frames += p_out_info->decoded_frames;
        a2dp_audio_aac_lastframe_info.undecode_frames =
            a2dp_audio_list_length(list) + a2dp_cp_get_in_frame_cnt_by_index(p_out_info->frame_idx) - 1;
        a2dp_audio_decoder_lastframe_info_set(&a2dp_audio_aac_lastframe_info);
    }

    if (p_out_info->pcm_len == buffer_bytes) {
        memcpy(buffer, p_out_info + 1, p_out_info->pcm_len);
        dec_ret = A2DP_DECODER_NO_ERROR;
    } else {
        dec_ret = A2DP_DECODER_DECODE_ERROR;
    }

    ret = a2dp_cp_consume_full_out_frame();
    ASSERT(ret == 0, "%s: a2dp_cp_consume_full_out_frame() failed: ret=%d", __func__, ret);

    return dec_ret;
}

TEXT_AAC_LOC
int a2dp_cp_aac_lc_cp_decode(void)
{
    int ret;
    enum CP_EMPTY_OUT_FRM_T out_frm_st;
    uint8_t *out;
    uint32_t out_len;
    uint8_t *dec_start;
    uint32_t dec_len;
    struct A2DP_CP_AAC_LC_IN_FRM_INFO_T *p_in_info;
    struct A2DP_CP_AAC_LC_OUT_FRM_INFO_T *p_out_info;
    uint8_t *in_buf;
    uint32_t in_len;
    uint32_t dec_sum;
    int error;
    uint32_t aac_maxreadBytes = AAC_READBUF_SIZE;
    UINT bufferSize = 0, bytesValid = 0;
    AAC_DECODER_ERROR decoder_err = AAC_DEC_OK;
    CStreamInfo *stream_info=NULL;

    if (cp_codec_reset) {
        cp_codec_reset = false;
        a2dp_audio_aac_lc_decoder_init();
    }

    out_frm_st = a2dp_cp_get_emtpy_out_frame((void **)&out, &out_len);
    if (out_frm_st != CP_EMPTY_OUT_FRM_OK && out_frm_st != CP_EMPTY_OUT_FRM_WORKING) {
        return 1;
    }
    ASSERT(out_len > sizeof(*p_out_info), "%s: Bad out_len %u (should > %u)", __func__, out_len, sizeof(*p_out_info));

    p_out_info = (struct A2DP_CP_AAC_LC_OUT_FRM_INFO_T *)out;
    if (out_frm_st == CP_EMPTY_OUT_FRM_OK) {
        p_out_info->pcm_len = 0;
        p_out_info->decoded_frames = 0;
    }
    ASSERT(out_len > sizeof(*p_out_info) + p_out_info->pcm_len, "%s: Bad out_len %u (should > %u + %u)", __func__, out_len, sizeof(*p_out_info), p_out_info->pcm_len);

    dec_start = (uint8_t *)(p_out_info + 1) + p_out_info->pcm_len;
    dec_len = out_len - (dec_start - (uint8_t *)out);

    if(dec_len < DECODE_AAC_PCM_FRAME_LENGTH){
        TRACE("aac_lc_decode pcm_len = %d \n", dec_len);
        return 2;
    }
    if(!aacDec_handle){
        TRACE("aac_lc_decode not ready");
        return 3;
    }

    dec_sum = 0;
    error = 0;

    while (dec_sum < dec_len && error == 0) {
        ret = a2dp_cp_get_in_frame((void **)&in_buf, &in_len);
        if (ret) {
            break;
        }
        ASSERT(in_len > sizeof(*p_in_info), "%s: Bad in_len %u (should > %u)", __func__, in_len, sizeof(*p_in_info));

        p_in_info = (struct A2DP_CP_AAC_LC_IN_FRM_INFO_T *)in_buf;
        in_buf += sizeof(*p_in_info);
        in_len -= sizeof(*p_in_info);

        if (in_len < 64)
            aac_maxreadBytes = 64;
        else if (in_len < 128)
            aac_maxreadBytes = 128;
        else if (in_len < 256)
            aac_maxreadBytes = 256;
        else if (in_len < 512)
            aac_maxreadBytes = 512;
        else if (in_len < 1024)
            aac_maxreadBytes = 1024;

        bufferSize = aac_maxreadBytes;
        bytesValid = aac_maxreadBytes;
        decoder_err = aacDecoder_Fill(aacDec_handle, &in_buf, &bufferSize, &bytesValid);
        if (decoder_err != AAC_DEC_OK) {
            TRACE("aacDecoder_Fill failed:0x%x", decoder_err);
            //if aac failed reopen it again
            if(is_aacDecoder_Close(aacDec_handle)){
                a2dp_audio_aac_lc_decoder_reinit();
                TRACE("aac_lc_decode reinin codec \n");
            }
            error = 1;
            goto end_decode;
        }

        /* decode one AAC frame */
        decoder_err = aacDecoder_DecodeFrame(aacDec_handle, (short *)(dec_start + dec_sum), (dec_len - dec_sum) / 2, 0 /* flags */);
        A2DP_DECODER_TRACE("aac_lc_decode seq:%d len:%d err:%x", p_in_info->sequenceNumber,
                                                                (dec_len - dec_sum),
                                                                decoder_err);
        if (decoder_err != AAC_DEC_OK){
            TRACE("aac_lc_decode failed:0x%x", decoder_err);
            //if aac failed reopen it again
            if(is_aacDecoder_Close(aacDec_handle)){
                a2dp_audio_aac_lc_decoder_reinit();
                TRACE("aac_lc_decode reinin codec \n");
            }
            error = 1;
            goto end_decode;
        }

        stream_info = aacDecoder_GetStreamInfo(aacDec_handle);
        if (!stream_info || stream_info->sampleRate <= 0) {
            TRACE("aac_lc_decode invalid stream info");
            error = 1;
            goto end_decode;
        }

        bufferSize = stream_info->frameSize * stream_info->numChannels * 2;//sizeof(pcm_buffer[0]);
        ASSERT(AAC_OUTPUT_FRAME_SAMPLES == bufferSize/4, "aac_lc_decode output mismatch samples:%d", bufferSize/4);

        dec_sum += bufferSize;

end_decode:
        memcpy(&p_out_info->in_info, p_in_info, sizeof(*p_in_info));
        p_out_info->decoded_frames++;
        p_out_info->frame_samples = AAC_OUTPUT_FRAME_SAMPLES;
        p_out_info->frame_idx = a2dp_cp_get_in_frame_index();

        ret = a2dp_cp_consume_in_frame();
        ASSERT(ret == 0, "%s: a2dp_cp_consume_in_frame() failed: ret=%d", __func__, ret);
    }

    p_out_info->pcm_len += dec_sum;

    if (error || out_len <= sizeof(*p_out_info) + p_out_info->pcm_len) {
        ret = a2dp_cp_consume_emtpy_out_frame();
        ASSERT(ret == 0, "%s: a2dp_cp_consume_emtpy_out_frame() failed: ret=%d", __func__, ret);
    }

    return error;
}
#endif

int a2dp_audio_aac_lc_init(A2DP_AUDIO_OUTPUT_CONFIG_T *config, void *context)
{
    A2DP_DECODER_TRACE("%s", __func__);

    TRACE("\n\nA2DP AAC-LC INIT\n");

    a2dp_audio_context_p = (A2DP_AUDIO_CONTEXT_T *)context;

    memcpy(&a2dp_audio_aac_output_config, config, sizeof(A2DP_AUDIO_OUTPUT_CONFIG_T));

    memset(&a2dp_audio_aac_lastframe_info, 0, sizeof(A2DP_AUDIO_DECODER_LASTFRAME_INFO_T));
    a2dp_audio_aac_lastframe_info.stream_info = &a2dp_audio_aac_output_config;

    ASSERT(a2dp_audio_context_p->dest_packet_mut < AAC_MTU_LIMITER, "%s MTU OVERFLOW:%d/%d", a2dp_audio_context_p->dest_packet_mut, AAC_MTU_LIMITER);

    aac_mempoll = (uint8_t *)a2dp_audio_heap_malloc(AAC_MEMPOOL_SIZE);
    A2DP_DECODER_ASSERT(aac_mempoll, "aac_mempoll = NULL");
    aac_memhandle = heap_register(aac_mempoll, AAC_MEMPOOL_SIZE);

#ifdef A2DP_CP_ACCEL
    int ret;

    cp_codec_reset = true;
    ret = a2dp_cp_init(a2dp_cp_aac_lc_cp_decode, CP_PROC_DELAY_1_FRAME);
    ASSERT(ret == 0, "%s: a2dp_cp_init() failed: ret=%d", __func__, ret);
#else
    a2dp_audio_aac_lc_decoder_init();
#endif

    return A2DP_DECODER_NO_ERROR;
}


int a2dp_audio_aac_lc_deinit(void)
{
#ifdef A2DP_CP_ACCEL
    a2dp_cp_deinit();
#endif

    a2dp_audio_aac_lc_decoder_deinit();
    size_t total = 0, used = 0, max_used = 0;
    heap_memory_info(aac_memhandle, &total, &used, &max_used);
    a2dp_audio_heap_free(aac_mempoll);
    A2DP_DECODER_TRACE("AAC MALLOC MEM: total - %d, used - %d, max_used - %d.",
        total, used, max_used);

    TRACE("\n\nA2DP AAC-LC DEINIT\n");

    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_aac_lc_mcu_decode_frame(uint8_t *buffer, uint32_t buffer_bytes)
{
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    list_node_t *node = NULL;
    a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;

    UINT bufferSize = 0, bytesValid = 0;
    AAC_DECODER_ERROR decoder_err = AAC_DEC_OK;
    CStreamInfo *stream_info=NULL;

    bool cache_underflow = false;
    uint32_t aac_maxreadBytes = AAC_READBUF_SIZE;
    int output_byte = 0;

    if(buffer_bytes < DECODE_AAC_PCM_FRAME_LENGTH){
        TRACE("aac_lc_decode pcm_len = %d \n", buffer_bytes);
        return A2DP_DECODER_NO_ERROR;
    }
    if(!aacDec_handle){
        TRACE("aac_lc_decode not ready");
        return A2DP_DECODER_NO_ERROR;
    }

    node = a2dp_audio_list_begin(list);    
    if (!node){
        TRACE("aac_lc_decode cache underflow");
        cache_underflow = true;
        goto exit;
    }else{
        aac_decoder_frame_p = (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_list_node(node);            
        
        if (aac_decoder_frame_p->aac_buffer_len < 64)
            aac_maxreadBytes = 64;
        else if (aac_decoder_frame_p->aac_buffer_len < 128)
            aac_maxreadBytes = 128;
        else if (aac_decoder_frame_p->aac_buffer_len < 256)
            aac_maxreadBytes = 256;
        else if (aac_decoder_frame_p->aac_buffer_len < 512)
            aac_maxreadBytes = 512;
        else if (aac_decoder_frame_p->aac_buffer_len < 1024)
            aac_maxreadBytes = 1024;
        
        bufferSize = aac_maxreadBytes;
        bytesValid = aac_maxreadBytes;
        decoder_err = aacDecoder_Fill(aacDec_handle, &(aac_decoder_frame_p->aac_buffer), &bufferSize, &bytesValid);
        if (decoder_err != AAC_DEC_OK) {
            TRACE("aacDecoder_Fill failed:0x%x", decoder_err);
            //if aac failed reopen it again
            if(is_aacDecoder_Close(aacDec_handle)){
                a2dp_audio_aac_lc_decoder_reinit();
                TRACE("aac_lc_decode reinin codec \n");
            }
            goto end_decode;
        }

        /* decode one AAC frame */
        decoder_err = aacDecoder_DecodeFrame(aacDec_handle, (short *)buffer, buffer_bytes/2, 0 /* flags */);
        A2DP_DECODER_TRACE("aac_lc_decode seq:%d len:%d err:%x", aac_decoder_frame_p->sequenceNumber,
                                                                aac_decoder_frame_p->aac_buffer_len,
                                                                decoder_err);
        if (decoder_err != AAC_DEC_OK){
            TRACE("aac_lc_decode failed:0x%x", decoder_err);
            //if aac failed reopen it again
            if(is_aacDecoder_Close(aacDec_handle)){
                a2dp_audio_aac_lc_decoder_reinit();
                TRACE("aac_lc_decode reinin codec \n");
            }
            goto end_decode;
        }
        
        stream_info = aacDecoder_GetStreamInfo(aacDec_handle);
        if (!stream_info || stream_info->sampleRate <= 0) {
            TRACE("aac_lc_decode invalid stream info");
            goto end_decode;
        }
        
        output_byte = stream_info->frameSize * stream_info->numChannels * 2;//sizeof(pcm_buffer[0]);
        ASSERT(AAC_OUTPUT_FRAME_SAMPLES == output_byte/4, "aac_lc_decode output mismatch samples:%d", output_byte/4);
end_decode:
        a2dp_audio_aac_lastframe_info.sequenceNumber = aac_decoder_frame_p->sequenceNumber;
        a2dp_audio_aac_lastframe_info.timestamp = aac_decoder_frame_p->timestamp;
        a2dp_audio_aac_lastframe_info.curSubSequenceNumber = 0;
        a2dp_audio_aac_lastframe_info.totalSubSequenceNumber = 0;
        a2dp_audio_aac_lastframe_info.frame_samples = AAC_OUTPUT_FRAME_SAMPLES;
        a2dp_audio_aac_lastframe_info.decoded_frames++;
        a2dp_audio_aac_lastframe_info.undecode_frames = a2dp_audio_list_length(list)-1;
        a2dp_audio_decoder_lastframe_info_set(&a2dp_audio_aac_lastframe_info);        
        a2dp_audio_list_remove(list, aac_decoder_frame_p);
    }
exit:
    if(cache_underflow){        
        a2dp_audio_aac_lastframe_info.undecode_frames = 0;
        a2dp_audio_decoder_lastframe_info_set(&a2dp_audio_aac_lastframe_info);        
        output_byte = A2DP_DECODER_CACHE_UNDERFLOW_ERROR;
    }
    return output_byte;
}

int a2dp_audio_aac_lc_decode_frame(uint8_t *buffer, uint32_t buffer_bytes)
{
#ifdef A2DP_CP_ACCEL
    return a2dp_cp_aac_lc_mcu_decode(buffer, buffer_bytes);
#else
    return a2dp_audio_aac_lc_mcu_decode_frame(buffer, buffer_bytes);
#endif
}

int a2dp_audio_aac_lc_preparse_packet(btif_media_header_t * header, uint8_t *buffer, uint32_t buffer_bytes)
{
    a2dp_audio_aac_lastframe_info.sequenceNumber = header->sequenceNumber;
    a2dp_audio_aac_lastframe_info.timestamp = header->timestamp;
    a2dp_audio_aac_lastframe_info.curSubSequenceNumber = 0;
    a2dp_audio_aac_lastframe_info.totalSubSequenceNumber = 0;
    a2dp_audio_aac_lastframe_info.frame_samples = AAC_OUTPUT_FRAME_SAMPLES;
    a2dp_audio_aac_lastframe_info.decoded_frames = 0;
    a2dp_audio_aac_lastframe_info.undecode_frames = 0;
    a2dp_audio_decoder_lastframe_info_set(&a2dp_audio_aac_lastframe_info);

    TRACE("%s seq:%d timestamp:%08x", __func__, header->sequenceNumber, header->timestamp);

    return A2DP_DECODER_NO_ERROR;
}

static void *a2dp_audio_aac_lc_frame_malloc(uint32_t packet_len)
{
    a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;
    uint8_t *aac_buffer = NULL;

    aac_buffer = (uint8_t *)a2dp_audio_heap_malloc(AAC_READBUF_SIZE);
    aac_decoder_frame_p = (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_heap_malloc(sizeof(a2dp_audio_aac_decoder_frame_t));
    aac_decoder_frame_p->aac_buffer = aac_buffer;
    aac_decoder_frame_p->aac_buffer_len = packet_len;
    return (void *)aac_decoder_frame_p;
}

void a2dp_audio_aac_lc_free(void *packet)
{
    a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = (a2dp_audio_aac_decoder_frame_t *)packet; 
    a2dp_audio_heap_free(aac_decoder_frame_p->aac_buffer);
    a2dp_audio_heap_free(aac_decoder_frame_p);
}

int a2dp_audio_aac_lc_store_packet(btif_media_header_t * header, uint8_t *buffer, uint32_t buffer_bytes)
{
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    int nRet = A2DP_DECODER_NO_ERROR;
    if (a2dp_audio_list_length(list) < aac_mtu_limiter){
        a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_aac_lc_frame_malloc(buffer_bytes);
        A2DP_DECODER_TRACE("%s seq:%d len:%d", __func__, header->sequenceNumber, buffer_bytes);
        aac_decoder_frame_p->sequenceNumber = header->sequenceNumber;
        aac_decoder_frame_p->timestamp = header->timestamp;
        memcpy(aac_decoder_frame_p->aac_buffer, buffer, buffer_bytes);
        aac_decoder_frame_p->aac_buffer_len = buffer_bytes;
        a2dp_audio_list_append(list, aac_decoder_frame_p);
        nRet = A2DP_DECODER_NO_ERROR;
    }else{
//        TRACE("%s list full current len:%d", __func__, a2dp_audio_list_length(list));
        nRet = A2DP_DECODER_MTU_LIMTER_ERROR;
    }

    return nRet;
}

int a2dp_audio_aac_lc_discards_packet(uint32_t packets)
{
    int nRet = A2DP_DECODER_MEMORY_ERROR;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    list_node_t *node = NULL;    
    a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;

#ifdef A2DP_CP_ACCEL
    a2dp_cp_reset_frame();
#endif

    if (packets <= a2dp_audio_list_length(list)){
        for (uint8_t i=0; i<packets; i++){
            node = a2dp_audio_list_begin(list);
            aac_decoder_frame_p = (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_list_node(node);            
            a2dp_audio_list_remove(list, aac_decoder_frame_p);
        }
        nRet = A2DP_DECODER_NO_ERROR;
    }

    TRACE("%s packets:%d nRet:%d", __func__, packets, nRet);
    return nRet;
}

int a2dp_audio_aac_lc_info_get(void *info)
{
    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_aac_lc_synchronize_packet(A2DP_AUDIO_SYNCFRAME_INFO_T *sync_info)
{
    int nRet = A2DP_DECODER_SYNC_ERROR;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    list_node_t *node = NULL;
    int list_len;
    a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;

#ifdef A2DP_CP_ACCEL
    a2dp_cp_reset_frame();
#endif

    list_len = a2dp_audio_list_length(list);

    for (uint16_t i=0; i<list_len; i++){        
        node = a2dp_audio_list_begin(list);                
        aac_decoder_frame_p = (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_list_node(node);
        if (aac_decoder_frame_p->sequenceNumber == sync_info->sequenceNumber &&
            aac_decoder_frame_p->timestamp      == sync_info->timestamp){
            nRet = A2DP_DECODER_NO_ERROR;
            break;
        }        
        a2dp_audio_list_remove(list, aac_decoder_frame_p);
    }

    TRACE("%s nRet:%d", __func__, nRet);
    return nRet;
}

int a2dp_audio_aac_lc_synchronize_dest_packet_mut(uint16_t packet_mut)
{
    list_node_t *node = NULL;
    uint32_t list_len = 0;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;

    list_len = a2dp_audio_list_length(list);
    if (list_len > packet_mut){
        do{        
            node = a2dp_audio_list_begin(list);            
            aac_decoder_frame_p = (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_list_node(node);
            a2dp_audio_list_remove(list, aac_decoder_frame_p);
        }while(a2dp_audio_list_length(list) > packet_mut);
    }

    TRACE("%s list:%d", __func__, a2dp_audio_list_length(list));
    return A2DP_DECODER_NO_ERROR;
}

A2DP_AUDIO_DECODER_T a2dp_audio_aac_lc_decoder_config = {
                                                        {44100, 2, 16},
                                                        a2dp_audio_aac_lc_init,
                                                        a2dp_audio_aac_lc_deinit,
                                                        a2dp_audio_aac_lc_decode_frame,
                                                        a2dp_audio_aac_lc_preparse_packet,
                                                        a2dp_audio_aac_lc_store_packet,
                                                        a2dp_audio_aac_lc_discards_packet,
                                                        a2dp_audio_aac_lc_synchronize_packet,
                                                        a2dp_audio_aac_lc_synchronize_dest_packet_mut,
                                                        a2dp_audio_aac_lc_info_get,
                                                        a2dp_audio_aac_lc_free,
                                                     } ;
#else
A2DP_AUDIO_DECODER_T a2dp_audio_aac_lc_decoder_config = {0,} ;
#endif
