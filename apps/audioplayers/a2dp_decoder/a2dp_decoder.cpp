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
#include "cmsis_os.h"
#include "plat_types.h"
#include <string.h>
#include "heap_api.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "app_audio.h"
#include "codec_sbc.h"
#include "avdtp_api.h"
#include "a2dp_decoder.h"
#include "a2dp_decoder_internal.h"
#if defined(IBRT)
#include "app_ibrt_if.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ibrt_audio_analysis.h"
#include "app_tws_ibrt_audio_sync.h"
#endif

#define A2DP_AUDIO_MEMPOOL_SIZE (72*1024)
#define A2DP_AUDIO_WAIT_TIMEOUT_MS (500)
#define A2DP_AUDIO_MUTE_FRAME_CNT_AFTER_NO_CACHE   (25)
#define A2DP_AUDIO_SKIP_FRAME_LIMIT_AFTER_NO_CACHE (50)

#define A2DP_AUDIO_REFILL_AFTER_NO_CACHE     (1)

#define A2DP_AUDIO_SYNC_INTERVAL             (1000)
#define A2DP_AUDIO_SYNC_FACTOR_FAST_LIMIT    (0.0002f)
#define A2DP_AUDIO_SYNC_FACTOR_SLOW_LIMIT    (-0.0005f)

osSemaphoreDef(audio_buffer_semaphore);
osMutexDef(audio_buffer_mutex);
osMutexDef(audio_status_mutex);


A2DP_AUDIO_CONTEXT_T a2dp_audio_context;

static heap_handle_t a2dp_audio_heap;

static A2DP_AUDIO_LASTFRAME_INFO_T a2dp_audio_lastframe_info;

A2DP_AUDIO_DETECT_NEXT_PACKET_CALLBACK a2dp_audio_detect_next_packet_callback = NULL;

void a2dp_audio_heap_init(void *begin_addr, uint32_t size)
{
    a2dp_audio_heap = heap_register(begin_addr,size);
}

void *a2dp_audio_heap_malloc(uint32_t size)
{
    void *ptr = heap_malloc(a2dp_audio_heap,size);
    A2DP_DECODER_ASSERT(ptr, "%s size:%d", __func__, size);
    return ptr;
}

void *a2dp_audio_heap_cmalloc(uint32_t size)
{
    void *ptr = heap_malloc(a2dp_audio_heap,size);
    A2DP_DECODER_ASSERT(ptr, "%s size:%d", __func__, size);
    memset(ptr, 0, size);    
    return ptr;
}

void *a2dp_audio_heap_realloc(void *rmem, uint32_t newsize)
{
    void *ptr = heap_realloc(a2dp_audio_heap, rmem, newsize);
    A2DP_DECODER_ASSERT(ptr, "%s rmem:%08x size:%d", __func__, rmem,newsize);
    return ptr;
}

void a2dp_audio_heap_free(void *rmem)
{
    A2DP_DECODER_ASSERT(rmem, "%s rmem:%08x", __func__, rmem);
    heap_free(a2dp_audio_heap,rmem);
}

int inline a2dp_audio_semaphore_init(void)
{
    if (a2dp_audio_context.audio_semaphore.semaphore == NULL){
        a2dp_audio_context.audio_semaphore.semaphore = osSemaphoreCreate(osSemaphore(audio_buffer_semaphore), 0);
    }
    a2dp_audio_context.audio_semaphore.enalbe = false;
    return 0;
}

int inline a2dp_audio_buffer_mutex_init(void)
{    
    if (a2dp_audio_context.audio_buffer_mutex == NULL){
        a2dp_audio_context.audio_buffer_mutex = osMutexCreate((osMutex(audio_buffer_mutex)));
    }
    return 0;
}

int inline a2dp_audio_buffer_mutex_lock(void)
{    
    osMutexWait((osMutexId)a2dp_audio_context.audio_buffer_mutex, osWaitForever);
    return 0;
}

int inline a2dp_audio_buffer_mutex_unlock(void)
{
    osMutexRelease((osMutexId)a2dp_audio_context.audio_buffer_mutex);
    return 0;
}

int inline a2dp_audio_status_mutex_init(void)
{    
    if (a2dp_audio_context.audio_status_mutex == NULL){
        a2dp_audio_context.audio_status_mutex = osMutexCreate((osMutex(audio_status_mutex)));
    }
    return 0;
}

int inline a2dp_audio_status_mutex_lock(void)
{    
    osMutexWait((osMutexId)a2dp_audio_context.audio_status_mutex, osWaitForever);
    return 0;
}

int inline a2dp_audio_status_mutex_unlock(void)
{
    osMutexRelease((osMutexId)a2dp_audio_context.audio_status_mutex);
    return 0;
}

int inline a2dp_audio_semaphore_wait(uint32_t timeout_ms)
{
    osSemaphoreId semaphore_id = (osSemaphoreId)a2dp_audio_context.audio_semaphore.semaphore;

    a2dp_audio_buffer_mutex_lock();
    a2dp_audio_context.audio_semaphore.enalbe = true;    
    a2dp_audio_buffer_mutex_unlock();
    
    int32_t nRet = osSemaphoreWait(semaphore_id, timeout_ms);
    if ((0 == nRet) || (-1 == nRet)){        
        TRACE("%s wait timerout", __func__);
        return -1;
    }
    return 0;
}

int inline a2dp_audio_semaphore_release(void)
{
    bool enalbe = false;

    a2dp_audio_buffer_mutex_lock();
    if (a2dp_audio_context.audio_semaphore.enalbe){
        a2dp_audio_context.audio_semaphore.enalbe = false;
        enalbe = true;
    }    
    a2dp_audio_buffer_mutex_unlock();

    if (enalbe){
        osSemaphoreId semaphore_id = (osSemaphoreId)a2dp_audio_context.audio_semaphore.semaphore;
        osSemaphoreRelease(semaphore_id);
    }
    return 0;
}

list_node_t *a2dp_audio_list_begin(const list_t *list)
{
    a2dp_audio_buffer_mutex_lock();
    list_node_t *node = list_begin(list);
    a2dp_audio_buffer_mutex_unlock();
    return node;
}

list_node_t *a2dp_audio_list_end(const list_t *list)
{
    a2dp_audio_buffer_mutex_lock();
    list_node_t *node = list_end(list);
    a2dp_audio_buffer_mutex_unlock();
    return node;
}

uint32_t a2dp_audio_list_length(const list_t *list)
{
    a2dp_audio_buffer_mutex_lock();
    uint32_t length = list_length(list);
    a2dp_audio_buffer_mutex_unlock();
    return length;
}

void *a2dp_audio_list_node(const list_node_t *node)
{
    a2dp_audio_buffer_mutex_lock();
    void *data = list_node(node);
    a2dp_audio_buffer_mutex_unlock();
    return data;
}

list_node_t *a2dp_audio_list_next(const list_node_t *node)
{
    a2dp_audio_buffer_mutex_lock();
    list_node_t *next =list_next(node);
    a2dp_audio_buffer_mutex_unlock();
    return next;
}

bool a2dp_audio_list_remove(list_t *list, void *data)
{
    a2dp_audio_buffer_mutex_lock();
    bool nRet = list_remove(list, data);
    a2dp_audio_buffer_mutex_unlock();
    return nRet;
}

bool a2dp_audio_list_append(list_t *list, void *data)
{
    a2dp_audio_buffer_mutex_lock();
    bool nRet = list_append(list, data);
    a2dp_audio_buffer_mutex_unlock();
    return nRet;
}

void a2dp_audio_list_clear(list_t *list)
{
    a2dp_audio_buffer_mutex_lock();
    list_clear(list);
    a2dp_audio_buffer_mutex_unlock();
}

void a2dp_audio_list_free(list_t *list)
{
    a2dp_audio_buffer_mutex_lock();
    list_free(list);
    a2dp_audio_buffer_mutex_unlock();
}

list_t *a2dp_audio_list_new(list_free_cb callback, list_mempool_zmalloc zmalloc, list_mempool_free free)
{
    a2dp_audio_buffer_mutex_lock();
    list_t *list =list_new(callback, zmalloc, free);
    a2dp_audio_buffer_mutex_unlock();
    return list;
}

int inline a2dp_audio_set_status(enum A2DP_AUDIO_DECODER_STATUS decoder_status)
{
    a2dp_audio_status_mutex_lock();
    a2dp_audio_context.audio_decoder_status = decoder_status;
    a2dp_audio_status_mutex_unlock();

    return 0;
}

enum A2DP_AUDIO_DECODER_STATUS inline a2dp_audio_get_status(void)
{
    enum A2DP_AUDIO_DECODER_STATUS decoder_status;
    a2dp_audio_status_mutex_lock();
    decoder_status = a2dp_audio_context.audio_decoder_status;
    a2dp_audio_status_mutex_unlock();

    return decoder_status;
}

int a2dp_audio_sync_pid_config(void)
{
    A2DP_AUDIO_SYNC_T *audio_sync = &a2dp_audio_context.audio_sync;
    A2DP_AUDIO_SYNC_PID_T *pid = &audio_sync->pid;

    pid->proportiongain = 0.2f;
    pid->integralgain =   0.1f;
    pid->derivativegain = 0.0f;            
    pid->alpha = 1.f;

    return 0;
}

int a2dp_audio_sync_init(void)
{
    A2DP_AUDIO_SYNC_T *audio_sync = &a2dp_audio_context.audio_sync;

    a2dp_audio_status_mutex_lock();
    audio_sync->tick = 0;
    audio_sync->cnt = 0;
    a2dp_audio_sync_pid_config();    
    a2dp_audio_sync_tune_sample_rate(a2dp_audio_context.output_cfg.factor_reference);
    a2dp_audio_status_mutex_unlock();

    return 0;
}

int a2dp_audio_sync_reset_data(void)
{
    A2DP_AUDIO_SYNC_T *audio_sync = &a2dp_audio_context.audio_sync;

    a2dp_audio_status_mutex_lock();
    audio_sync->tick = 0;
    audio_sync->cnt = 0;
    a2dp_audio_sync_pid_config();    
    a2dp_audio_status_mutex_unlock();
    
    return 0;
}

int a2dp_audio_sync_tune_sample_rate(double ratio)
{
    float resample_rate_ratio;

    a2dp_audio_status_mutex_lock();
    a2dp_audio_context.output_cfg.factor_reference = (float)ratio;
    resample_rate_ratio = (float)(ratio - (double)1.0);
    af_codec_tune_resample_rate(AUD_STREAM_PLAYBACK, resample_rate_ratio);
    
    TRACE("%s ppb:%d", __func__, (int32_t)(resample_rate_ratio * 10000000));
    a2dp_audio_status_mutex_unlock();
    
    return 0;
}

int a2dp_audio_sync_tune(float ratio)
{
#if defined(IBRT)
    if (app_ibrt_ui_ibrt_connected()){        
        if (app_tws_ibrt_mobile_link_connected()){            
            APP_TWS_IBRT_AUDIO_SYNC_TUNE_T sync_tune;
            sync_tune.factor_reference = ratio;
            tws_ctrl_send_cmd(APP_TWS_CMD_SYNC_TUNE, (uint8_t*)&sync_tune, sizeof(APP_TWS_IBRT_AUDIO_SYNC_TUNE_T));
        }
    }else{
        a2dp_audio_sync_tune_sample_rate(ratio);
    }
#else
    a2dp_audio_sync_tune_sample_rate(ratio);
#endif
    return 0;
}

float a2dp_audio_sync_pid_calc(A2DP_AUDIO_SYNC_PID_T *pid, float diff)
{
    float increment;
    float deltaDev;
    float pError,dError,iError;

    pid->error[0] = diff;
    pError=pid->error[0]-pid->error[1];
    iError=pid->error[0];   
    dError=pid->error[0]-2*pid->error[1]+pid->error[2];

    deltaDev = pid->proportiongain*(1 - pid->alpha)*dError + pid->alpha*pid->deltaDev_last;
    increment = pid->proportiongain*pError+pid->integralgain*iError+deltaDev;

    pid->error[2] = pid->error[1];
    pid->error[1] = pid->error[0];
    pid->deltaDev_last = deltaDev;

    pid->result += increment;
    
    return pid->result;
}

int a2dp_audio_sync_handler(uint8_t *buffer, uint32_t buffer_bytes)
{
    A2DP_AUDIO_LASTFRAME_INFO_T *lastframe_info;
    A2DP_AUDIO_SYNC_T *audio_sync = &a2dp_audio_context.audio_sync;
    float diff_mtu = 0;
    int32_t frame_mtu = 0;
    int32_t total_mtu = 0;
    float diff_factor = 0;
    
    if (audio_sync->tick++%A2DP_AUDIO_SYNC_INTERVAL == 0){
        list_t *list = a2dp_audio_context.audio_datapath.input_raw_packet_list;
        A2DP_AUDIO_SYNC_PID_T *pid = &audio_sync->pid;
        //valid limter 0x80000
        if (audio_sync->cnt < 0x80000){
            audio_sync->cnt += A2DP_AUDIO_SYNC_INTERVAL;
        }
        a2dp_audio_lastframe_info_ptr_get(&lastframe_info);
        frame_mtu = lastframe_info->stream_info->frame_samples/lastframe_info->frame_samples;
        total_mtu = audio_sync->cnt * frame_mtu;
        diff_mtu = a2dp_audio_context.average_packet_mut - (float)a2dp_audio_context.dest_packet_mut;
#if 1
        TRACE("audio_sync sample:%d/%d diff:%d/%d/%d/%d curr:%d", lastframe_info->frame_samples,
                                                             lastframe_info->stream_info->frame_samples,
                                                             (int32_t)(diff_mtu+0.5f),
                                                             (int32_t)(a2dp_audio_context.average_packet_mut+0.5f),
                                                             a2dp_audio_context.dest_packet_mut,
                                                             total_mtu,
                                                             a2dp_audio_list_length(list));
#else
        TRACE("audio_sync diff:%10.9f/%10.9f frame_mut:%d dest:%d total:%d curr:%d", (double)diff_mtu, (double)a2dp_audio_context.average_packet_mut, frame_mtu,
                                                                                      a2dp_audio_context.dest_packet_mut, total_mtu, a2dp_audio_list_length(list));
        TRACE("audio_sync try tune:%d, %10.9f %10.9f", (diff_mtu - frame_mtu) > ((float)frame_mtu/3.f),
                                                        (double)ABS(diff_mtu - frame_mtu), (double)((float)frame_mtu/3.f));
#endif
        if (ABS(diff_mtu - frame_mtu) > ((float)frame_mtu/4.f) &&
            audio_sync->tick != 1){
            diff_factor = diff_mtu/(float)total_mtu;
            if (a2dp_audio_sync_pid_calc(pid, diff_factor)){
                float dest_pid_result = pid->result;                
                if (pid->result > A2DP_AUDIO_SYNC_FACTOR_FAST_LIMIT){
                    dest_pid_result = A2DP_AUDIO_SYNC_FACTOR_FAST_LIMIT;
                }else if (pid->result < A2DP_AUDIO_SYNC_FACTOR_SLOW_LIMIT){
                    dest_pid_result = A2DP_AUDIO_SYNC_FACTOR_SLOW_LIMIT;
                }
                pid->result = dest_pid_result;
                dest_pid_result = a2dp_audio_context.output_cfg.factor_reference + dest_pid_result;
                if (!a2dp_audio_sync_tune(dest_pid_result)){
                    audio_sync->cnt = 0;
                }
//                TRACE("audio_sync tune %10.9f %10.9f lim:%10.9f", (double)diff_factor, (double)pid->result, (double)dest_pid_result);
            }            
        }
    }
    return 0;
}

/*
1,  2^1
3,  2^2
7,  2^3
15, 2^4
31, 2^5
*/
#define AUDIO_ALPHA_PRAMS_1 (3) 
#define AUDIO_ALPHA_PRAMS_2 (4)

static inline float a2dp_audio_alpha_filter(float y, float x)
{
    if (y){
        y = ((AUDIO_ALPHA_PRAMS_1*y)+x)/AUDIO_ALPHA_PRAMS_2;
    }else{
        y = x;
    }
    return y;
}

static void inline a2dp_audio_convert_16bit_to_24bit(int32_t *out, int16_t *in, int len)
{
    for (int i = len - 1; i >= 0; i--) {
        out[i] = ((int32_t)in[i] << 8);
    }
}

static void inline a2dp_audio_channel_select(A2DP_AUDIO_CHANNEL_SELECT_E chnl_sel, uint8_t *buffer, uint32_t buffer_bytes)
{
    uint32_t samples;
    uint32_t i;

    ASSERT(a2dp_audio_context.output_cfg.num_channels == 2, "%s num_channels:%d", __func__, a2dp_audio_context.output_cfg.num_channels);

    if (a2dp_audio_context.output_cfg.bits_depth == 24){
        int32_t *buf_l_p = (int32_t *)buffer;
        int32_t *buf_r_p = (int32_t *)buffer + 1;

        samples = buffer_bytes/4/2;        
        switch (chnl_sel)
        {
            case A2DP_AUDIO_CHANNEL_SELECT_LRMERGE:
                for (i = 0; i<samples; i++, buf_l_p+=2, buf_r_p+=2){
                    int32_t tmp_sample = (*buf_r_p + *buf_r_p)>>1;
                    *buf_l_p = tmp_sample;
                    *buf_r_p = tmp_sample;
                }
                break;
            case A2DP_AUDIO_CHANNEL_SELECT_LCHNL:
                for (i = 0; i<samples; i++, buf_l_p+=2, buf_r_p+=2){                    
                    *buf_r_p = *buf_l_p;
                }
                break;
            case A2DP_AUDIO_CHANNEL_SELECT_RCHNL:
                for (i = 0; i<samples; i++, buf_l_p+=2, buf_r_p+=2){                    
                    *buf_l_p = *buf_r_p;
                }
                break;
            case A2DP_AUDIO_CHANNEL_SELECT_STEREO:
            default:
                break;
        }
    }else{    
        int16_t *buf_l_p = (int16_t *)buffer;
        int16_t *buf_r_p = (int16_t *)buffer + 1;        

        samples = buffer_bytes/2/2;        
        switch (chnl_sel)
        {
            case A2DP_AUDIO_CHANNEL_SELECT_LRMERGE:
                for (i = 0; i<samples; i++, buf_l_p+=2, buf_r_p+=2){
                    int16_t tmp_sample = ((int32_t)*buf_r_p + (int32_t)*buf_r_p+1)>>1;
                    *buf_l_p = tmp_sample;
                    *buf_r_p = tmp_sample;
                }
                break;
            case A2DP_AUDIO_CHANNEL_SELECT_LCHNL:
                for (i = 0; i<samples; i++, buf_l_p+=2, buf_r_p+=2){                    
                    *buf_r_p = *buf_l_p;
                }
                break;
            case A2DP_AUDIO_CHANNEL_SELECT_RCHNL:
                for (i = 0; i<samples; i++, buf_l_p+=2, buf_r_p+=2){                    
                    *buf_l_p = *buf_r_p;
                }
                break;
            case A2DP_AUDIO_CHANNEL_SELECT_STEREO:
            default:
                break;
        }
    }
}

int a2dp_audio_store_packet(btif_media_header_t * header, unsigned char *buf, unsigned int len)
{
    int nRet = A2DP_DECODER_NO_ERROR;

    if (a2dp_audio_get_status() != A2DP_AUDIO_DECODER_STATUS_START){
        TRACE("%s skip packet status:%d", __func__, a2dp_audio_get_status());
        goto exit;
    }     
    
    a2dp_audio_status_mutex_lock();
    if (a2dp_audio_context.need_detect_first_packet){
        a2dp_audio_context.need_detect_first_packet = false;        
        a2dp_audio_context.audio_decoder.audio_decoder_preparse_packet(header, buf, len);
    }

    if (a2dp_audio_detect_next_packet_callback){
        a2dp_audio_detect_next_packet_callback(header, buf, len);
    }

    nRet = a2dp_audio_context.audio_decoder.audio_decoder_store_packet(header, buf, len);
#if defined(IBRT)
    if (nRet == A2DP_DECODER_MTU_LIMTER_ERROR){
        if (app_tws_ibrt_mobile_link_connected()){
            // try again
            a2dp_audio_status_mutex_unlock();
            a2dp_audio_semaphore_wait(A2DP_AUDIO_WAIT_TIMEOUT_MS);            
            a2dp_audio_status_mutex_lock();
            if (a2dp_audio_get_status() != A2DP_AUDIO_DECODER_STATUS_START){                
                TRACE("%s skip packet status:%d", __func__, a2dp_audio_get_status());
                goto status_mutex_unlock;
            }            
            nRet = a2dp_audio_context.audio_decoder.audio_decoder_store_packet(header, buf, len);
        }
        if (nRet == A2DP_DECODER_MTU_LIMTER_ERROR){                
            a2dp_audio_discards_packet(1);            
            TRACE("%s MTU_LIMTER so discards_packet", __func__);
        }
    }
#else
    if (nRet == A2DP_DECODER_MTU_LIMTER_ERROR){        
        a2dp_audio_status_mutex_unlock();
        a2dp_audio_semaphore_wait(A2DP_AUDIO_WAIT_TIMEOUT_MS);        
        a2dp_audio_status_mutex_lock();
        if (a2dp_audio_get_status() != A2DP_AUDIO_DECODER_STATUS_START){            
            TRACE("%s skip packet status:%d", __func__, a2dp_audio_get_status());
            goto status_mutex_unlock;
        }     
         
        nRet = a2dp_audio_context.audio_decoder.audio_decoder_store_packet(header, buf, len);
        if (nRet == A2DP_DECODER_MTU_LIMTER_ERROR){
            TRACE("%s MTU_LIMTER skip packet need add more process", __func__);
        }
    }
#endif
status_mutex_unlock:
    a2dp_audio_status_mutex_unlock();
exit:

    return 0;
}

uint32_t a2dp_audio_playback_handler(uint8_t *buffer, uint32_t buffer_bytes)
{
    uint32_t len = buffer_bytes;
    int nRet = A2DP_DECODER_NO_ERROR;
    A2DP_AUDIO_LASTFRAME_INFO_T *lastframe_info;
    list_t *list = a2dp_audio_context.audio_datapath.input_raw_packet_list;

    if (a2dp_audio_get_status() != A2DP_AUDIO_DECODER_STATUS_START){        
        TRACE("%s skip handler status:%d", __func__, a2dp_audio_get_status());
        goto exit;
    }         
  
    if (a2dp_audio_context.average_packet_mut == 0){
        a2dp_audio_context.average_packet_mut = a2dp_audio_list_length(list);        
        TRACE("%s init average_packet_mut:%d", __func__, (uint16_t)(a2dp_audio_context.average_packet_mut+0.5f));
    }else{
        uint16_t packet_mut = a2dp_audio_list_length(list);
        a2dp_audio_context.average_packet_mut = a2dp_audio_alpha_filter((float)a2dp_audio_context.average_packet_mut, (float)packet_mut);
        a2dp_audio_sync_handler(buffer, buffer_bytes);
    }
#if defined(A2DP_AUDIO_REFILL_AFTER_NO_CACHE)
    if (a2dp_audio_context.skip_frame_cnt_after_no_cache){        
        if (a2dp_audio_list_length(list) >= a2dp_audio_context.dest_packet_mut){
            a2dp_audio_context.skip_frame_cnt_after_no_cache = 0;
        }
        memset(buffer, 0, buffer_bytes);
        TRACE("%s decode refill skip_cnt:%d, list:%d", __func__, a2dp_audio_context.skip_frame_cnt_after_no_cache, a2dp_audio_list_length(list));
        if (a2dp_audio_context.skip_frame_cnt_after_no_cache > 0){
            a2dp_audio_context.skip_frame_cnt_after_no_cache--;
        }else{
            a2dp_audio_context.mute_frame_cnt_after_no_cache = A2DP_AUDIO_MUTE_FRAME_CNT_AFTER_NO_CACHE;
        }
    }else
#endif
    {
        if (a2dp_audio_context.output_cfg.bits_depth == 24 &&
            a2dp_audio_context.audio_decoder.stream_info.bits_depth == 16){
     
            len = len / (sizeof(int32_t) / sizeof(int16_t));

            nRet = a2dp_audio_context.audio_decoder.audio_decoder_decode_frame(buffer, len);
            if  (nRet < 0 || a2dp_audio_context.mute_frame_cnt_after_no_cache){
                TRACE("%s decode failed nRet=%d mute_cnt:%d", __func__, nRet, a2dp_audio_context.mute_frame_cnt_after_no_cache);
                //mute frame
                memset(buffer, 0, len);
            }
            a2dp_audio_convert_16bit_to_24bit((int32_t *)buffer, (int16_t *)buffer, len / sizeof(int16_t));
            
            // Restore len to 24-bit sample buffer length
            len = len * (sizeof(int32_t) / sizeof(int16_t));

        }else if (a2dp_audio_context.output_cfg.bits_depth == 
                  a2dp_audio_context.audio_decoder.stream_info.bits_depth){
                  
            nRet = a2dp_audio_context.audio_decoder.audio_decoder_decode_frame(buffer, len);
            if  (nRet < 0 || a2dp_audio_context.mute_frame_cnt_after_no_cache){
              //mute frame          
              TRACE("%s decode failed nRet=%d mute_cnt:%d", __func__, nRet, a2dp_audio_context.mute_frame_cnt_after_no_cache);
              memset(buffer, 0, len);
            }

        }
        a2dp_audio_channel_select(a2dp_audio_context.chnl_sel, buffer, buffer_bytes);
    }
    a2dp_audio_semaphore_release();

    if (nRet == A2DP_DECODER_CACHE_UNDERFLOW_ERROR){
        a2dp_audio_context.mute_frame_cnt_after_no_cache = A2DP_AUDIO_MUTE_FRAME_CNT_AFTER_NO_CACHE;
        a2dp_audio_context.skip_frame_cnt_after_no_cache = A2DP_AUDIO_SKIP_FRAME_LIMIT_AFTER_NO_CACHE;        
        a2dp_audio_context.average_packet_mut = 0;
        a2dp_audio_sync_reset_data();
        TRACE("A2DP_DECODER_CACHE_UNDERFLOW_ERROR");
    }else{
        if (a2dp_audio_context.mute_frame_cnt_after_no_cache > 0){            
            a2dp_audio_context.mute_frame_cnt_after_no_cache--;            
            a2dp_audio_context.average_packet_mut = 0;
            if (a2dp_audio_context.mute_frame_cnt_after_no_cache >= 1){
                a2dp_audio_context.audio_decoder.audio_decoder_synchronize_dest_packet_mut(a2dp_audio_context.dest_packet_mut);
            }
        }
    }    
    a2dp_audio_lastframe_info_ptr_get(&lastframe_info);
    lastframe_info->stream_info->factor_reference = a2dp_audio_context.output_cfg.factor_reference;
    lastframe_info->average_frames = (uint32_t)(a2dp_audio_context.average_packet_mut + 0.5f);
exit:
    return len;
}

static void a2dp_audio_packet_free(void *packet)
{
    if (a2dp_audio_context.audio_decoder.audio_decoder_packet_free){
        a2dp_audio_context.audio_decoder.audio_decoder_packet_free(packet);
    }else{
        a2dp_audio_heap_free(packet);
    }
}

void a2dp_audio_clear_input_raw_packet_list(void)
{
    //just clean the packet list to start receive ai data again
    if(a2dp_audio_context.audio_datapath.input_raw_packet_list)
        a2dp_audio_list_clear(a2dp_audio_context.audio_datapath.input_raw_packet_list);
}

extern A2DP_AUDIO_DECODER_T a2dp_audio_sbc_decoder_config;
extern A2DP_AUDIO_DECODER_T a2dp_audio_aac_lc_decoder_config;

int a2dp_audio_init(A2DP_AUDIO_OUTPUT_CONFIG_T *config, A2DP_AUDIO_CHANNEL_SELECT_E chnl_sel, uint8_t codec_type, uint16_t dest_packet_mut)
{
    uint8_t *heap_buff = NULL;
    uint32_t heap_size = 0;
    A2DP_AUDIO_OUTPUT_CONFIG_T decoder_output_config;

    TRACE("%s codec:%d sample_rate:%d ch:%d bit:%d samples:%d dest_mut:%d", __func__, codec_type,
                                                                                      config->sample_rate,
                                                                                      config->num_channels,
                                                                                      config->bits_depth,
                                                                                      config->frame_samples,
                                                                                      dest_packet_mut);

    a2dp_audio_semaphore_init();
    a2dp_audio_buffer_mutex_init();
    a2dp_audio_status_mutex_init();

    a2dp_audio_status_mutex_lock();

    a2dp_audio_detect_next_packet_callback_register(NULL);            

    heap_size = A2DP_AUDIO_MEMPOOL_SIZE;
    app_audio_mempool_get_buff(&heap_buff, heap_size);
    A2DP_DECODER_ASSERT(heap_buff, "%s size:%d", __func__, heap_size);
    a2dp_audio_heap_init(heap_buff, heap_size);

    a2dp_audio_context.audio_datapath.input_raw_packet_list = a2dp_audio_list_new(a2dp_audio_packet_free, 
                                                       (list_mempool_zmalloc)a2dp_audio_heap_cmalloc, 
                                                       (list_mempool_free)a2dp_audio_heap_free);
    
    a2dp_audio_context.audio_datapath.output_pcm_packet_list = a2dp_audio_list_new(a2dp_audio_packet_free, 
                                                       (list_mempool_zmalloc)a2dp_audio_heap_cmalloc, 
                                                       (list_mempool_free)a2dp_audio_heap_free);

    memcpy(&(a2dp_audio_context.output_cfg), config, sizeof(A2DP_AUDIO_OUTPUT_CONFIG_T));

    a2dp_audio_context.chnl_sel = chnl_sel;
    a2dp_audio_context.dest_packet_mut = dest_packet_mut;
    a2dp_audio_context.average_packet_mut = 0;

    switch (codec_type)
    {
        case BTIF_AVDTP_CODEC_TYPE_SBC:
            decoder_output_config.sample_rate = config->sample_rate;
            decoder_output_config.num_channels = 2;
            decoder_output_config.bits_depth = 16;
            decoder_output_config.frame_samples = config->frame_samples;
            decoder_output_config.factor_reference = 1.0;
            memcpy(&(a2dp_audio_context.audio_decoder), &a2dp_audio_sbc_decoder_config, sizeof(A2DP_AUDIO_DECODER_T));
            break;
        case BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC:
            decoder_output_config.sample_rate = config->sample_rate;
            decoder_output_config.num_channels = 2;
            decoder_output_config.bits_depth = 16;
            decoder_output_config.frame_samples = config->frame_samples;            
            decoder_output_config.factor_reference = 1.0;
            memcpy(&(a2dp_audio_context.audio_decoder), &a2dp_audio_aac_lc_decoder_config, sizeof(A2DP_AUDIO_DECODER_T));
            break;
        case BTIF_AVDTP_CODEC_TYPE_NON_A2DP:
            break;
        default:
            break;
    }
    
    a2dp_audio_context.audio_decoder.audio_decoder_init(&decoder_output_config, (void *)&a2dp_audio_context);
    a2dp_audio_context.need_detect_first_packet = true;
    a2dp_audio_context.skip_frame_cnt_after_no_cache = 0;
    a2dp_audio_context.mute_frame_cnt_after_no_cache = 0;

    memset(&a2dp_audio_lastframe_info, 0, sizeof(A2DP_AUDIO_LASTFRAME_INFO_T));

    TRACE("%s list:%08x", __func__,a2dp_audio_context.audio_datapath.input_raw_packet_list);    

    a2dp_audio_context.audio_decoder_status = A2DP_AUDIO_DECODER_STATUS_READY;

    a2dp_audio_sync_init();

    a2dp_audio_status_mutex_unlock();

    return 0;
}

int a2dp_audio_deinit(void)
{
    a2dp_audio_status_mutex_lock();

    a2dp_audio_detect_next_packet_callback_register(NULL);            

    a2dp_audio_context.audio_decoder.audio_decoder_deinit();
    memset(&(a2dp_audio_context.audio_decoder), 0, sizeof(A2DP_AUDIO_DECODER_T));
    a2dp_audio_list_clear(a2dp_audio_context.audio_datapath.input_raw_packet_list);
    a2dp_audio_list_free(a2dp_audio_context.audio_datapath.input_raw_packet_list);
    a2dp_audio_context.audio_datapath.input_raw_packet_list = NULL;
    a2dp_audio_list_clear(a2dp_audio_context.audio_datapath.output_pcm_packet_list);
    a2dp_audio_list_free(a2dp_audio_context.audio_datapath.output_pcm_packet_list);
    a2dp_audio_context.audio_datapath.output_pcm_packet_list = NULL;
    
    a2dp_audio_context.audio_decoder_status = A2DP_AUDIO_DECODER_STATUS_NULL;

    a2dp_audio_status_mutex_unlock();

    return 0;
}

int a2dp_audio_stop(void)
{
    a2dp_audio_status_mutex_lock();
    a2dp_audio_set_status(A2DP_AUDIO_DECODER_STATUS_STOP);
    a2dp_audio_semaphore_release();    
    a2dp_audio_status_mutex_unlock();
    return 0;
}

int a2dp_audio_start(void)
{
    a2dp_audio_status_mutex_lock();
    a2dp_audio_set_status(A2DP_AUDIO_DECODER_STATUS_START);    
    a2dp_audio_status_mutex_unlock();
    return 0;
}

int a2dp_audio_detect_next_packet_callback_register(A2DP_AUDIO_DETECT_NEXT_PACKET_CALLBACK callback)
{
    a2dp_audio_status_mutex_lock();
    a2dp_audio_detect_next_packet_callback = callback;    
    a2dp_audio_status_mutex_unlock();

    return 0;
}

int a2dp_audio_detect_first_packet(void)
{
    a2dp_audio_status_mutex_lock();    
    a2dp_audio_context.need_detect_first_packet = true;
    a2dp_audio_status_mutex_unlock();
    return 0;
}

int a2dp_audio_discards_packet(uint32_t packets)
{
    int nRet = 0;

    if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START){
        a2dp_audio_status_mutex_lock();
        nRet = a2dp_audio_context.audio_decoder.audio_decoder_discards_packet(packets);
        a2dp_audio_status_mutex_unlock();
    }else{
        nRet = -1;
    }

    return nRet;
}

int a2dp_audio_discards_samples(uint32_t samples)
{
    int nRet = 0;

    if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START){
        a2dp_audio_status_mutex_lock();

        a2dp_audio_status_mutex_unlock();
    }else{
        nRet = -1;
    }

    return nRet;
}

int a2dp_audio_get_packet_samples(void)
{
    A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
    uint32_t packet_samples = 0;
    uint16_t totalSubSequenceNumber = 1;

    a2dp_audio_status_mutex_lock();
    a2dp_audio_lastframe_info_get(&lastframe_info);            
    a2dp_audio_status_mutex_unlock();

    if (lastframe_info.totalSubSequenceNumber){
        totalSubSequenceNumber = lastframe_info.totalSubSequenceNumber;
    }
    
    packet_samples = totalSubSequenceNumber * lastframe_info.frame_samples;

    return packet_samples;
}

int a2dp_audio_lastframe_info_ptr_get(A2DP_AUDIO_LASTFRAME_INFO_T **lastframe_info)
{
    int nRet = 0;

    a2dp_audio_buffer_mutex_lock();
    if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START){
        *lastframe_info = &a2dp_audio_lastframe_info;
    }else{
        nRet = -1;
    }
    a2dp_audio_buffer_mutex_unlock();
    
    return nRet;
}

int a2dp_audio_lastframe_info_get(A2DP_AUDIO_LASTFRAME_INFO_T *lastframe_info)
{
    int nRet = 0;
    
    if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START){
        a2dp_audio_buffer_mutex_lock();
        memcpy(lastframe_info, &a2dp_audio_lastframe_info, sizeof(A2DP_AUDIO_LASTFRAME_INFO_T));
        a2dp_audio_buffer_mutex_unlock();
    }else{
        memset(lastframe_info, 0, sizeof(A2DP_AUDIO_LASTFRAME_INFO_T));
        nRet = -1;
    }

    return nRet;
}

int a2dp_audio_decoder_lastframe_info_set(A2DP_AUDIO_DECODER_LASTFRAME_INFO_T *lastframe_info)
{
    a2dp_audio_buffer_mutex_lock();
    a2dp_audio_lastframe_info.sequenceNumber        = lastframe_info->sequenceNumber;
    a2dp_audio_lastframe_info.timestamp             = lastframe_info->timestamp;
    a2dp_audio_lastframe_info.curSubSequenceNumber  = lastframe_info->curSubSequenceNumber;
    a2dp_audio_lastframe_info.totalSubSequenceNumber= lastframe_info->totalSubSequenceNumber;
    a2dp_audio_lastframe_info.frame_samples         = lastframe_info->frame_samples;
    a2dp_audio_lastframe_info.decoded_frames        = lastframe_info->decoded_frames;
    a2dp_audio_lastframe_info.undecode_frames       = lastframe_info->undecode_frames;
    a2dp_audio_lastframe_info.stream_info           = lastframe_info->stream_info;
    a2dp_audio_buffer_mutex_unlock();

    return 0;
}

int a2dp_audio_synchronize_packet(A2DP_AUDIO_SYNCFRAME_INFO_T *sync_info)
{
    int nRet = 0;

    if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START){
        a2dp_audio_status_mutex_lock();
        nRet = a2dp_audio_context.audio_decoder.audio_decoder_synchronize_packet(sync_info);
        a2dp_audio_status_mutex_unlock();
    }else{
        nRet = -1;
    }

    return nRet;
}

int a2dp_audio_refill_packet(void)
{
    int refill_cnt = 0;

#if defined(A2DP_AUDIO_REFILL_AFTER_NO_CACHE)
    refill_cnt += a2dp_audio_context.skip_frame_cnt_after_no_cache;
#endif
    refill_cnt += a2dp_audio_context.mute_frame_cnt_after_no_cache;

    return refill_cnt;
}

