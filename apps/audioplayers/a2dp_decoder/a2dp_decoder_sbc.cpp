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
#include "codec_sbc.h"
#include "a2dp_decoder_internal.h"


#define SBC_MTU_LIMITER (320) /*must <= 332*/

static A2DP_AUDIO_CONTEXT_T *a2dp_audio_context_p = NULL;
static A2DP_AUDIO_OUTPUT_CONFIG_T a2dp_audio_sbc_output_config;

typedef struct  {
    btif_sbc_decoder_t *sbc_decoder;
    btif_sbc_pcm_data_t *pcm_data;
} a2dp_audio_sbc_decoder_t;

typedef struct {
    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint16_t curSubSequenceNumber;
    uint16_t totalSubSequenceNumber;
    uint8_t *sbc_buffer;
    uint32_t sbc_buffer_len;
} a2dp_audio_sbc_decoder_frame_t;

static a2dp_audio_sbc_decoder_t a2dp_audio_sbc_decoder;

static A2DP_AUDIO_DECODER_LASTFRAME_INFO_T a2dp_audio_sbc_lastframe_info;

static uint16_t sbc_mtu_limiter = SBC_MTU_LIMITER;

static void sbc_codec_init(void)
{
    btif_sbc_init_decoder(a2dp_audio_sbc_decoder.sbc_decoder);
    a2dp_audio_sbc_decoder.pcm_data->data = NULL;
    a2dp_audio_sbc_decoder.pcm_data->dataLen = 0;
}

#ifdef A2DP_CP_ACCEL
struct A2DP_CP_SBC_IN_FRM_INFO_T {
    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint16_t curSubSequenceNumber;
    uint16_t totalSubSequenceNumber;
};

struct A2DP_CP_SBC_OUT_FRM_INFO_T {
    struct A2DP_CP_SBC_IN_FRM_INFO_T in_info;
    uint16_t frame_samples;
    uint16_t decoded_frames;
    uint16_t frame_idx;
    uint16_t pcm_len;
};

static bool cp_codec_reset;

TEXT_SBC_LOC
static int a2dp_cp_sbc_mcu_decode(uint8_t *buffer, uint32_t buffer_bytes)
{
    a2dp_audio_sbc_decoder_frame_t *sbc_decoder_frame;
    list_node_t *node = NULL;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    int ret, dec_ret;
    struct A2DP_CP_SBC_IN_FRM_INFO_T in_info;
    struct A2DP_CP_SBC_OUT_FRM_INFO_T *p_out_info;
    uint8_t *out;
    uint32_t out_len;
    uint32_t out_frame_len;

    out_frame_len = sizeof(*p_out_info) + buffer_bytes;

    ret = a2dp_cp_decoder_init(out_frame_len);
    ASSERT(ret == 0, "%s: a2dp_cp_decoder_init() failed: ret=%d", __func__, ret);

    while ((node = a2dp_audio_list_begin(list)) != NULL) {
        sbc_decoder_frame = (a2dp_audio_sbc_decoder_frame_t *)a2dp_audio_list_node(node);

        in_info.sequenceNumber = sbc_decoder_frame->sequenceNumber;
        in_info.timestamp = sbc_decoder_frame->timestamp;
        in_info.curSubSequenceNumber = sbc_decoder_frame->curSubSequenceNumber;
        in_info.totalSubSequenceNumber = sbc_decoder_frame->totalSubSequenceNumber;

        ret = a2dp_cp_put_in_frame(&in_info, sizeof(in_info), sbc_decoder_frame->sbc_buffer, sbc_decoder_frame->sbc_buffer_len);
        if (ret) {
            break;
        }

        a2dp_audio_list_remove(list, sbc_decoder_frame);
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

    p_out_info = (struct A2DP_CP_SBC_OUT_FRM_INFO_T *)out;
    if (p_out_info->pcm_len) {
        a2dp_audio_sbc_lastframe_info.sequenceNumber = p_out_info->in_info.sequenceNumber;
        a2dp_audio_sbc_lastframe_info.timestamp = p_out_info->in_info.timestamp;
        a2dp_audio_sbc_lastframe_info.curSubSequenceNumber = p_out_info->in_info.curSubSequenceNumber;
        a2dp_audio_sbc_lastframe_info.totalSubSequenceNumber = p_out_info->in_info.totalSubSequenceNumber;
        a2dp_audio_sbc_lastframe_info.frame_samples = p_out_info->frame_samples;
        a2dp_audio_sbc_lastframe_info.decoded_frames += p_out_info->decoded_frames;
        a2dp_audio_sbc_lastframe_info.undecode_frames =
            a2dp_audio_list_length(list) + a2dp_cp_get_in_frame_cnt_by_index(p_out_info->frame_idx) - 1;
        a2dp_audio_decoder_lastframe_info_set(&a2dp_audio_sbc_lastframe_info);
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

TEXT_SBC_LOC
int a2dp_cp_sbc_cp_decode(void)
{
    int ret;
    enum CP_EMPTY_OUT_FRM_T out_frm_st;
    uint8_t *out;
    uint32_t out_len;
    uint8_t *dec_start;
    uint32_t dec_len;
    struct A2DP_CP_SBC_IN_FRM_INFO_T *p_in_info;
    struct A2DP_CP_SBC_OUT_FRM_INFO_T *p_out_info;
    uint8_t *in_buf;
    uint32_t in_len;
    uint16_t bytes_parsed = 0;
    float sbc_subbands_gain[8]={1,1,1,1,1,1,1,1};
    btif_sbc_decoder_t *sbc_decoder;
    btif_sbc_pcm_data_t *pcm_data;
    bt_status_t decoder_err;
    int error;

    if (cp_codec_reset) {
        cp_codec_reset = false;
        sbc_codec_init();
    }

    sbc_decoder = a2dp_audio_sbc_decoder.sbc_decoder;
    pcm_data = a2dp_audio_sbc_decoder.pcm_data;

    out_frm_st = a2dp_cp_get_emtpy_out_frame((void **)&out, &out_len);
    if (out_frm_st != CP_EMPTY_OUT_FRM_OK && out_frm_st != CP_EMPTY_OUT_FRM_WORKING) {
        return 1;
    }
    ASSERT(out_len > sizeof(*p_out_info), "%s: Bad out_len %u (should > %u)", __func__, out_len, sizeof(*p_out_info));

    p_out_info = (struct A2DP_CP_SBC_OUT_FRM_INFO_T *)out;
    if (out_frm_st == CP_EMPTY_OUT_FRM_OK) {
        p_out_info->pcm_len = 0;
        p_out_info->decoded_frames = 0;
    }
    ASSERT(out_len > sizeof(*p_out_info) + p_out_info->pcm_len, "%s: Bad out_len %u (should > %u + %u)", __func__, out_len, sizeof(*p_out_info), p_out_info->pcm_len);

    dec_start = (uint8_t *)(p_out_info + 1) + p_out_info->pcm_len;
    dec_len = out_len - (dec_start - (uint8_t *)out);

    pcm_data->data = dec_start;
    pcm_data->dataLen = 0;
    error = 0;

    while (pcm_data->dataLen < dec_len && error == 0) {
        ret = a2dp_cp_get_in_frame((void **)&in_buf, &in_len);
        if (ret) {
            break;
        }
        ASSERT(in_len > sizeof(*p_in_info), "%s: Bad in_len %u (should > %u)", __func__, in_len, sizeof(*p_in_info));

        p_in_info = (struct A2DP_CP_SBC_IN_FRM_INFO_T *)in_buf;
        in_buf += sizeof(*p_in_info);
        in_len -= sizeof(*p_in_info);

        decoder_err = btif_sbc_decode_frames(sbc_decoder, in_buf, in_len,
                                    &bytes_parsed,
                                    pcm_data,
                                    dec_len,
                                    sbc_subbands_gain);
        switch (decoder_err)
        {
            case BT_STS_SUCCESS:
            case BT_STS_CONTINUE:
                break;
            case BT_STS_NO_RESOURCES:
                error = 1;
                ASSERT(0, "sbc_decode BT_STS_NO_RESOURCES pcm has no more buffer, i think can't reach here");
                break;
            case BT_STS_FAILED:
            default:
                error = 1;
                btif_sbc_init_decoder(sbc_decoder);
                break;
        }

        memcpy(&p_out_info->in_info, p_in_info, sizeof(*p_in_info));
        p_out_info->decoded_frames++;
        p_out_info->frame_samples = sbc_decoder->maxPcmLen/4;
        p_out_info->frame_idx = a2dp_cp_get_in_frame_index();

        ret = a2dp_cp_consume_in_frame();
        ASSERT(ret == 0, "%s: a2dp_cp_consume_in_frame() failed: ret=%d", __func__, ret);
    }

    p_out_info->pcm_len += pcm_data->dataLen;

    if (error || out_len <= sizeof(*p_out_info) + p_out_info->pcm_len) {
        ret = a2dp_cp_consume_emtpy_out_frame();
        ASSERT(ret == 0, "%s: a2dp_cp_consume_emtpy_out_frame() failed: ret=%d", __func__, ret);
    }

    return error;
}
#endif

int a2dp_audio_sbc_init(A2DP_AUDIO_OUTPUT_CONFIG_T *config, void *context)
{
    A2DP_DECODER_TRACE("%s", __func__);

    TRACE("\n\nA2DP SBC INIT\n");

    a2dp_audio_context_p = (A2DP_AUDIO_CONTEXT_T *)context;

    memcpy(&a2dp_audio_sbc_output_config, config, sizeof(A2DP_AUDIO_OUTPUT_CONFIG_T));

    memset(&a2dp_audio_sbc_lastframe_info, 0, sizeof(A2DP_AUDIO_DECODER_LASTFRAME_INFO_T));
    a2dp_audio_sbc_lastframe_info.stream_info = &a2dp_audio_sbc_output_config;

    ASSERT(a2dp_audio_context_p->dest_packet_mut < SBC_MTU_LIMITER, "%s MTU OVERFLOW:%d/%d", a2dp_audio_context_p->dest_packet_mut, SBC_MTU_LIMITER);

    a2dp_audio_sbc_decoder.sbc_decoder = (btif_sbc_decoder_t *)a2dp_audio_heap_malloc(sizeof(btif_sbc_decoder_t));
    a2dp_audio_sbc_decoder.pcm_data = (btif_sbc_pcm_data_t *)a2dp_audio_heap_malloc(sizeof(btif_sbc_pcm_data_t));

#ifdef A2DP_CP_ACCEL
    int ret;

    cp_codec_reset = true;
    ret = a2dp_cp_init(a2dp_cp_sbc_cp_decode, CP_PROC_DELAY_1_FRAME);
    ASSERT(ret == 0, "%s: a2dp_cp_init() failed: ret=%d", __func__, ret);
#else
    sbc_codec_init();
#endif

    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_sbc_deinit(void)
{
#ifdef A2DP_CP_ACCEL
    a2dp_cp_deinit();
#endif

    a2dp_audio_heap_free(a2dp_audio_sbc_decoder.sbc_decoder);
    a2dp_audio_heap_free(a2dp_audio_sbc_decoder.pcm_data);

    TRACE("\n\nA2DP SBC DEINIT\n");

    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_sbc_mcu_decode_frame(uint8_t *buffer, uint32_t buffer_bytes)
{
    bt_status_t ret = BT_STS_SUCCESS;
    uint16_t bytes_parsed = 0;
    float sbc_subbands_gain[8]={1,1,1,1,1,1,1,1};
    btif_sbc_decoder_t *sbc_decoder;
    btif_sbc_pcm_data_t *pcm_data;
    uint16_t frame_pcmbyte;
    uint16_t pcm_output_byte;
    bool cache_underflow = false;
    
    sbc_decoder = a2dp_audio_sbc_decoder.sbc_decoder;
    pcm_data = a2dp_audio_sbc_decoder.pcm_data;
    frame_pcmbyte = sbc_decoder->maxPcmLen;


    a2dp_audio_sbc_decoder_frame_t *sbc_decoder_frame;
    list_node_t *node = NULL;

    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;

    pcm_data->data = buffer;
    pcm_data->dataLen = 0;

    A2DP_DECODER_TRACE("sbc decoder sub frame size:%d", a2dp_audio_list_length(list));

    for (pcm_output_byte = 0; pcm_output_byte<buffer_bytes; pcm_output_byte += frame_pcmbyte){
        node = a2dp_audio_list_begin(list);
        
        if (node){
            uint32_t lock;
            
            sbc_decoder_frame = (a2dp_audio_sbc_decoder_frame_t *)a2dp_audio_list_node(node);
            
            lock = int_lock();            
            ret = btif_sbc_decode_frames(sbc_decoder, sbc_decoder_frame->sbc_buffer, sbc_decoder_frame->sbc_buffer_len,
                                        &bytes_parsed,
                                        pcm_data,
                                        buffer_bytes,
                                        sbc_subbands_gain);
            int_unlock(lock);
            A2DP_DECODER_TRACE("sbc decoder sub seq:%d/%d/%d len:%d ret:%d used:%d", sbc_decoder_frame->curSubSequenceNumber,
                                                                        sbc_decoder_frame->totalSubSequenceNumber,
                                                                        sbc_decoder_frame->sequenceNumber,
                                                                        sbc_decoder_frame->sbc_buffer_len,
                                                                        ret,
                                                                        bytes_parsed);
            
            a2dp_audio_sbc_lastframe_info.sequenceNumber = sbc_decoder_frame->sequenceNumber;
            a2dp_audio_sbc_lastframe_info.timestamp = sbc_decoder_frame->timestamp;
            a2dp_audio_sbc_lastframe_info.curSubSequenceNumber = sbc_decoder_frame->curSubSequenceNumber;
            a2dp_audio_sbc_lastframe_info.totalSubSequenceNumber = sbc_decoder_frame->totalSubSequenceNumber;
            a2dp_audio_sbc_lastframe_info.frame_samples = sbc_decoder->maxPcmLen/4;
            a2dp_audio_sbc_lastframe_info.decoded_frames++;
            a2dp_audio_sbc_lastframe_info.undecode_frames = a2dp_audio_list_length(list)-1;
            a2dp_audio_decoder_lastframe_info_set(&a2dp_audio_sbc_lastframe_info);
            a2dp_audio_list_remove(list, sbc_decoder_frame);
            switch (ret)
            {
                case BT_STS_SUCCESS:
                    if (pcm_data->dataLen != buffer_bytes){
                        TRACE("!!!WARNING pcm buffer size mismatch");
                    }
                    break;
                case BT_STS_CONTINUE:
                    continue;
                    break;
                case BT_STS_NO_RESOURCES:
                    ASSERT(0, "sbc_decode BT_STS_NO_RESOURCES pcm has no more buffer, i think can't reach here");
                    break;
                case BT_STS_FAILED:
                default:
                    btif_sbc_init_decoder(sbc_decoder);
                    goto exit;
            }
        }else{
            TRACE("A2DP PACKET CACHE UNDERFLOW");
            ret = BT_STS_FAILED;
            cache_underflow = true;
            goto exit;
        }

    }
exit:
    if (cache_underflow){     
        A2DP_DECODER_TRACE("A2DP PACKET CACHE UNDERFLOW need add some process");        
        a2dp_audio_sbc_lastframe_info.undecode_frames = a2dp_audio_list_length(list)-1;
        a2dp_audio_decoder_lastframe_info_set(&a2dp_audio_sbc_lastframe_info);
        ret = A2DP_DECODER_CACHE_UNDERFLOW_ERROR;
    }
    return ret;
}

int a2dp_audio_sbc_decode_frame(uint8_t *buffer, uint32_t buffer_bytes)
{
#ifdef A2DP_CP_ACCEL
    return a2dp_cp_sbc_mcu_decode(buffer, buffer_bytes);
#else
    return a2dp_audio_sbc_mcu_decode_frame(buffer, buffer_bytes);
#endif
}

int a2dp_audio_sbc_preparse_packet(btif_media_header_t * header, uint8_t *buffer, uint32_t buffer_bytes)
{
    uint16_t bytes_parsed = 0;
    float sbc_subbands_gain[8]={1,1,1,1,1,1,1,1};
    btif_sbc_decoder_t *sbc_decoder = a2dp_audio_sbc_decoder.sbc_decoder;

    uint32_t frame_num = 0;
    uint8_t *parser_p = buffer;

    frame_num = *parser_p;
    parser_p++;
    buffer_bytes--;

    // TODO: Remove the following sbc init and decode codes. They might conflict with the calls
    //       during CP process. CP process is triggered by audioflinger PCM callback.

    sbc_codec_init();

    btif_sbc_decode_frames(sbc_decoder, 
                           parser_p, buffer_bytes,
                           &bytes_parsed,
                           NULL, 0,
                           sbc_subbands_gain);
    TRACE("%s seq:%d timestamp:%08x sampleFreq:%d chnl:%d pcmLen:%d frame_num:%d bytes:%d", __func__, 
                                                                    header->sequenceNumber,
                                                                    header->timestamp,
                                                                    sbc_decoder->streamInfo.sampleFreq,
                                                                    sbc_decoder->streamInfo.channelMode,
                                                                    sbc_decoder->maxPcmLen,
                                                                    frame_num,
                                                                    buffer_bytes);
    
    a2dp_audio_sbc_lastframe_info.sequenceNumber = header->sequenceNumber;
    a2dp_audio_sbc_lastframe_info.timestamp = header->timestamp;
    a2dp_audio_sbc_lastframe_info.curSubSequenceNumber = 0;
    a2dp_audio_sbc_lastframe_info.totalSubSequenceNumber = frame_num;
    a2dp_audio_sbc_lastframe_info.frame_samples = sbc_decoder->maxPcmLen/4;            
    a2dp_audio_sbc_lastframe_info.decoded_frames = 0;
    a2dp_audio_sbc_lastframe_info.undecode_frames = 0;
    a2dp_audio_decoder_lastframe_info_set(&a2dp_audio_sbc_lastframe_info);
    
    return A2DP_DECODER_NO_ERROR;
}

static void *a2dp_audio_sbc_subframe_malloc(uint32_t sbc_len)
{
    a2dp_audio_sbc_decoder_frame_t *sbc_decoder_frame_p = NULL;
    uint8_t *sbc_buffer = NULL;

    sbc_buffer = (uint8_t *)a2dp_audio_heap_malloc(sbc_len);
    sbc_decoder_frame_p = (a2dp_audio_sbc_decoder_frame_t *)a2dp_audio_heap_malloc(sizeof(a2dp_audio_sbc_decoder_frame_t));
    sbc_decoder_frame_p->sbc_buffer = sbc_buffer;
    sbc_decoder_frame_p->sbc_buffer_len = sbc_len;
    return (void *)sbc_decoder_frame_p;
}

void a2dp_audio_sbc_subframe_free(void *packet)
{
    a2dp_audio_sbc_decoder_frame_t *sbc_decoder_frame_p = (a2dp_audio_sbc_decoder_frame_t *)packet; 
    a2dp_audio_heap_free(sbc_decoder_frame_p->sbc_buffer);
    a2dp_audio_heap_free(sbc_decoder_frame_p);
}

int a2dp_audio_sbc_store_packet(btif_media_header_t * header, uint8_t *buffer, uint32_t buffer_bytes)
{
    int nRet = A2DP_DECODER_NO_ERROR;

    uint32_t frame_cnt = 0;
    uint32_t frame_num = 0;
    uint32_t frame_len = 0;
    uint8_t *parser_p = buffer;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;

    frame_num = *parser_p;
    parser_p++;
    buffer_bytes--;
    frame_len = buffer_bytes/frame_num;

    if ((a2dp_audio_list_length(list)+frame_num) < sbc_mtu_limiter){
        for (uint32_t i=0; i<buffer_bytes; i+=frame_len, frame_cnt++){
            if (*(parser_p+i) == 0x9c){
                a2dp_audio_sbc_decoder_frame_t *frame_p = (a2dp_audio_sbc_decoder_frame_t *)a2dp_audio_sbc_subframe_malloc(frame_len);
                frame_p->sequenceNumber = header->sequenceNumber;
                frame_p->timestamp = header->timestamp;
                frame_p->curSubSequenceNumber = frame_cnt;
                frame_p->totalSubSequenceNumber = frame_num;
                memcpy(frame_p->sbc_buffer, (parser_p+i), frame_len);
                frame_p->sbc_buffer_len = frame_len;
                a2dp_audio_list_append(list, frame_p);            
            }else{
                TRACE("ERROR SBC FRAME !!!");
                DUMP8("%02x ", parser_p+i, 5);
            }
        }
        nRet = A2DP_DECODER_NO_ERROR;
    }else{
//        TRACE("%s list full current len:%d", __func__, a2dp_audio_list_length(list));
        nRet = A2DP_DECODER_MTU_LIMTER_ERROR;
    }
    return nRet;
}

int a2dp_audio_sbc_discards_packet(uint32_t packets)
{
    int nRet = A2DP_DECODER_MEMORY_ERROR;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    list_node_t *node = NULL;
    a2dp_audio_sbc_decoder_frame_t *sbc_decoder_frame;
    uint16_t totalSubSequenceNumber;
    uint8_t j = 0;

#ifdef A2DP_CP_ACCEL
    a2dp_cp_reset_frame();
#endif

    node = a2dp_audio_list_begin(list);
    sbc_decoder_frame = (a2dp_audio_sbc_decoder_frame_t *)a2dp_audio_list_node(node);            

    for (j=0; j<a2dp_audio_list_length(list); j++){
        node = a2dp_audio_list_begin(list);                
        sbc_decoder_frame = (a2dp_audio_sbc_decoder_frame_t *)a2dp_audio_list_node(node);
        if (sbc_decoder_frame->curSubSequenceNumber != 0){
            a2dp_audio_list_remove(list, sbc_decoder_frame);
        }else{
            break;
        }
    }

    node = a2dp_audio_list_begin(list);
    sbc_decoder_frame = (a2dp_audio_sbc_decoder_frame_t *)a2dp_audio_list_node(node);            
    ASSERT(sbc_decoder_frame->curSubSequenceNumber == 0, "sbc_discards_packet not align curSubSequenceNumber:%d", 
                                                                          sbc_decoder_frame->curSubSequenceNumber);

    totalSubSequenceNumber = sbc_decoder_frame->totalSubSequenceNumber;

    if (packets <= a2dp_audio_list_length(list)/totalSubSequenceNumber){
        for (uint8_t i=0; i<packets; i++){
            for (j=0; j<totalSubSequenceNumber; j++){
                node = a2dp_audio_list_begin(list);                
                sbc_decoder_frame = (a2dp_audio_sbc_decoder_frame_t *)a2dp_audio_list_node(node);            
                a2dp_audio_list_remove(list, sbc_decoder_frame);
            }
        }
        nRet = A2DP_DECODER_NO_ERROR;
    }

    TRACE("%s packets:%d nRet:%d", __func__, packets, nRet);
    return nRet;
}

int a2dp_audio_sbc_info_get(void *info)
{
    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_sbc_synchronize_packet(A2DP_AUDIO_SYNCFRAME_INFO_T *sync_info)
{
    int nRet = A2DP_DECODER_SYNC_ERROR;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    list_node_t *node = NULL;
    int list_len;
    a2dp_audio_sbc_decoder_frame_t *sbc_decoder_frame;

#ifdef A2DP_CP_ACCEL
    a2dp_cp_reset_frame();
#endif

    list_len = a2dp_audio_list_length(list);

    for (uint16_t i=0; i<list_len; i++){        
        node = a2dp_audio_list_begin(list);                
        sbc_decoder_frame = (a2dp_audio_sbc_decoder_frame_t *)a2dp_audio_list_node(node);
        if (sbc_decoder_frame->sequenceNumber         == sync_info->sequenceNumber &&
            sbc_decoder_frame->timestamp              == sync_info->timestamp &&
            sbc_decoder_frame->curSubSequenceNumber  == sync_info->curSubSequenceNumber &&
            sbc_decoder_frame->totalSubSequenceNumber == sync_info->totalSubSequenceNumber){
            nRet = A2DP_DECODER_NO_ERROR;
            break;
        }        
        a2dp_audio_list_remove(list, sbc_decoder_frame);
    }

    TRACE("%s nRet:%d", __func__, nRet);
    return nRet;
}

int a2dp_audio_sbc_synchronize_dest_packet_mut(uint16_t packet_mut)
{
    list_node_t *node = NULL;
    uint32_t list_len = 0;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    a2dp_audio_sbc_decoder_frame_t *sbc_decoder_frame = NULL;

    list_len = a2dp_audio_list_length(list);
    if (list_len > packet_mut){
        do{        
            node = a2dp_audio_list_begin(list);            
            sbc_decoder_frame = (a2dp_audio_sbc_decoder_frame_t *)a2dp_audio_list_node(node);
            a2dp_audio_list_remove(list, sbc_decoder_frame);
        }while(a2dp_audio_list_length(list) > packet_mut);
    }
    
    TRACE("%s list:%d", __func__, a2dp_audio_list_length(list));

    return A2DP_DECODER_NO_ERROR;
}

A2DP_AUDIO_DECODER_T a2dp_audio_sbc_decoder_config = {
                                                        {44100, 2, 16},
                                                        a2dp_audio_sbc_init,
                                                        a2dp_audio_sbc_deinit,
                                                        a2dp_audio_sbc_decode_frame,
                                                        a2dp_audio_sbc_preparse_packet,
                                                        a2dp_audio_sbc_store_packet,
                                                        a2dp_audio_sbc_discards_packet,
                                                        a2dp_audio_sbc_synchronize_packet,
                                                        a2dp_audio_sbc_synchronize_dest_packet_mut,
                                                        a2dp_audio_sbc_info_get,
                                                        a2dp_audio_sbc_subframe_free,
                                                     } ;

