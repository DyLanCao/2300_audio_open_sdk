/***************************************************************************
 *
 * Copyright 2018-2024 whale_audio.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of whale_audio.
 *
 * Use of this work is governed by a license granted by hale_audio.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
/**

|-------|--------------------|-----------------------------------|----------------------------------|-----------|----------------------|
| TX/RX |        New         |                Old                |           description            |  MIPS(M)  |         Note         |
|       |                    |                                   |                                  | 16k    8k |                      |
|-------|--------------------|-----------------------------------|----------------------------------|-----------|----------------------|
|       | SPEECH_TX_AEC      | SPEECH_ECHO_CANCEL                | Acoustic Echo Cancellation(old)  | 40     \  | HWFFT: 37            |
|       | SPEECH_TX_AEC2     | SPEECH_AEC_FIX                    | Acoustic Echo Cancellation(new)  | 39     \  | enable NLP           |
|       | SPEECH_TX_NS       | SPEECH_NOISE_SUPPRESS             | 1 mic noise suppress(old)        | 30     \  | HWFFT: 19            |
|       | SPEECH_TX_NS2      | LC_MMSE_NOISE_SUPPRESS            | 1 mic noise suppress(new)        | 16     \  | HWFFT: 12            |
| TX    | SPEECH_TX_2MIC_NS  | DUAL_MIC_DENOISE                  | 2 mic noise suppres(long space)  | \         |                      |
|       | SPEECH_TX_2MIC_NS2 | COHERENT_DENOISE                  | 2 mic noise suppres(short space) | 32        |                      |
|       | SPEECH_TX_2MIC_NS3 | FAR_FIELD_SPEECH_ENHANCEMENT      | 2 mic noise suppres(far field)   | \         |                      |
|       | SPEECH_TX_2MIC_NS5 | LEFTRIGHT_DENOISE                 | 2 mic noise suppr(left right end)| \         |
|       | SPEECH_TX_AGC      | SPEECH_AGC                        | Automatic Gain Control           | 3         |                      |
|       | SPEECH_TX_EQ       | SPEECH_WEIGHTING_FILTER_SUPPRESS  | Default EQ                       | 0.5     \ | each section         |
|-------|--------------------|-----------------------------------|----------------------------------|-----------|----------------------|
|       | SPEECH_RX_NS       | SPEAKER_NOISE_SUPPRESS            | 1 mic noise suppress(old)        | 30      \ |                      |
| RX    | SPEECH_RX_NS2      | LC_MMSE_NOISE_SUPPRESS_SPK        | 1 mic noise suppress(new)        | 16      \ |                      |
|       | SPEECH_RX_AGC      | SPEECH_AGC_SPK                    | Automatic Gain Control           | 3         |                      |
|       | SPEECH_RX_EQ       | SPEAKER_WEIGHTING_FILTER_SUPPRESS | Default EQ                       | 0.5     \ | each section         |
|-------|--------------------|-----------------------------------|----------------------------------|-----------|----------------------|
**/

#include "plat_types.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "audio_dump.h"
#include "hal_timer.h"
#include "hal_sysfreq.h"
#include "speech_memory.h"
#include "cqueue.h"
#include "cmsis_os.h"


#include "cmsis_os.h"
#include "string.h"
#include "cqueue.h"
#include "list.h"

#include "hal_trace.h"
#include "hal_aud.h"

#include "resources.h"
#include "app_thread.h"
#include "app_audio.h"
#include "app_media_player.h"
#include "app_ring_merge.h"
#include "nvrecord.h"
#include <assert.h>


#include "a2dp_api.h"

#include "btapp.h"
#include "app_bt_media_manager.h"


#ifdef WL_NSX
uint8_t *nsx_heap;
uint8_t *queue_tx_heap;
uint8_t *queue_tx_out_heap;
uint8_t *queue_rx_heap;
uint8_t *queue_rx_out_heap;

static uint32_t POSSIBLY_UNUSED wl_sco_sample = 16000;

#include "nsx_main.h"
#define NSX_FRAME_SIZE 160
short *sco_out_buff,*sco_in_buff;
short *sco_rx_out_buff,*sco_rx_in_buff;
#if defined(WL_AEC)
short *sco_aec_out;
#endif
#endif


#define NSX_BUFF_SIZE 14000
#define NSX_MODE 1

// sco_tx_queue process
static CQueue app_sco_tx_in_pcm_queue;
static osMutexId app_sco_in_pcmbuff_mutex_id = NULL;
osMutexDef(app_sco_in_pcmbuff_mutex);


int app_sco_in_pcmbuff_init(uint8_t *buff, uint16_t len)
{
    if (app_sco_in_pcmbuff_mutex_id  == NULL)
        app_sco_in_pcmbuff_mutex_id  = osMutexCreate((osMutex(app_sco_in_pcmbuff_mutex)));

    if ((buff == NULL)||(app_sco_in_pcmbuff_mutex_id == NULL))
        return -1;

    osMutexWait(app_sco_in_pcmbuff_mutex_id, osWaitForever);
    InitCQueue(&app_sco_tx_in_pcm_queue, len, buff);
    memset(buff, 0x00, len);
    osMutexRelease(app_sco_in_pcmbuff_mutex_id);

    return 0;
}

int app_sco_in_pcmbuff_length(void)
{
    int len;

    osMutexWait(app_sco_in_pcmbuff_mutex_id, osWaitForever);
    len = LengthOfCQueue(&app_sco_tx_in_pcm_queue);
    osMutexRelease(app_sco_in_pcmbuff_mutex_id);

    return len;
}

int app_sco_in_pcmbuff_put(uint8_t *buff, uint16_t len)
{
    int status;

    osMutexWait(app_sco_in_pcmbuff_mutex_id, osWaitForever);
    status = EnCQueue(&app_sco_tx_in_pcm_queue, buff, len);
    osMutexRelease(app_sco_in_pcmbuff_mutex_id);

    return status;
}

int app_sco_in_pcmbuff_get(uint8_t *buff, uint16_t len)
{
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;
    int status;

    osMutexWait(app_sco_in_pcmbuff_mutex_id, osWaitForever);
    status = PeekCQueue(&app_sco_tx_in_pcm_queue, len, &e1, &len1, &e2, &len2);
    if (len==(len1+len2)){
        memcpy(buff,e1,len1);
        memcpy(buff+len1,e2,len2);
        DeCQueue(&app_sco_tx_in_pcm_queue, 0, len1);
        DeCQueue(&app_sco_tx_in_pcm_queue, 0, len2);
    }else{
        memset(buff, 0x00, len);
        status = -1;
    }
    osMutexRelease(app_sco_in_pcmbuff_mutex_id);

    return status;
}

// sco_tx_out_queue process
static CQueue app_sco_tx_out_pcm_queue;
static osMutexId app_sco_out_pcmbuff_mutex_id = NULL;
osMutexDef(app_sco_out_pcmbuff_mutex);


int app_sco_out_pcmbuff_init(uint8_t *buff, uint16_t len)
{
    if (app_sco_out_pcmbuff_mutex_id  == NULL)
        app_sco_out_pcmbuff_mutex_id  = osMutexCreate((osMutex(app_sco_out_pcmbuff_mutex)));

    if ((buff == NULL)||(app_sco_out_pcmbuff_mutex_id == NULL))
        return -1;

    osMutexWait(app_sco_out_pcmbuff_mutex_id, osWaitForever);
    InitCQueue(&app_sco_tx_out_pcm_queue, len, buff);
    memset(buff, 0x00, len);
    osMutexRelease(app_sco_out_pcmbuff_mutex_id);

    return 0;
}

int app_sco_out_pcmbuff_length(void)
{
    int len;

    osMutexWait(app_sco_out_pcmbuff_mutex_id, osWaitForever);
    len = LengthOfCQueue(&app_sco_tx_out_pcm_queue);
    osMutexRelease(app_sco_out_pcmbuff_mutex_id);

    return len;
}

int app_sco_out_pcmbuff_put(uint8_t *buff, uint16_t len)
{
    int status;

    osMutexWait(app_sco_out_pcmbuff_mutex_id, osWaitForever);
    status = EnCQueue(&app_sco_tx_out_pcm_queue, buff, len);
    osMutexRelease(app_sco_out_pcmbuff_mutex_id);

    return status;
}

int app_sco_out_pcmbuff_get(uint8_t *buff, uint16_t len)
{
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;
    int status;

    osMutexWait(app_sco_out_pcmbuff_mutex_id, osWaitForever);
    status = PeekCQueue(&app_sco_tx_out_pcm_queue, len, &e1, &len1, &e2, &len2);
    if (len==(len1+len2)){
        memcpy(buff,e1,len1);
        memcpy(buff+len1,e2,len2);
        DeCQueue(&app_sco_tx_out_pcm_queue, 0, len1);
        DeCQueue(&app_sco_tx_out_pcm_queue, 0, len2);
    }else{
        memset(buff, 0x00, len);
        status = -1;
    }
    osMutexRelease(app_sco_out_pcmbuff_mutex_id);

    return status;
}



// sco_tx_queue process
static CQueue app_sco_rx_in_pcm_queue;
static osMutexId app_sco_rx_in_pcmbuff_mutex_id = NULL;
osMutexDef(app_sco_rx_in_pcmbuff_mutex);


int app_sco_rx_in_pcmbuff_init(uint8_t *buff, uint16_t len)
{
    if (app_sco_rx_in_pcmbuff_mutex_id  == NULL)
        app_sco_rx_in_pcmbuff_mutex_id  = osMutexCreate((osMutex(app_sco_rx_in_pcmbuff_mutex)));

    if ((buff == NULL)||(app_sco_rx_in_pcmbuff_mutex_id == NULL))
        return -1;

    osMutexWait(app_sco_rx_in_pcmbuff_mutex_id, osWaitForever);
    InitCQueue(&app_sco_rx_in_pcm_queue, len, buff);
    memset(buff, 0x00, len);
    osMutexRelease(app_sco_rx_in_pcmbuff_mutex_id);

    return 0;
}

int app_sco_rx_in_pcmbuff_length(void)
{
    int len;

    osMutexWait(app_sco_rx_in_pcmbuff_mutex_id, osWaitForever);
    len = LengthOfCQueue(&app_sco_rx_in_pcm_queue);
    osMutexRelease(app_sco_rx_in_pcmbuff_mutex_id);

    return len;
}

int app_sco_rx_in_pcmbuff_put(uint8_t *buff, uint16_t len)
{
    int status;

    osMutexWait(app_sco_rx_in_pcmbuff_mutex_id, osWaitForever);
    status = EnCQueue(&app_sco_rx_in_pcm_queue, buff, len);
    osMutexRelease(app_sco_rx_in_pcmbuff_mutex_id);

    return status;
}

int app_sco_rx_in_pcmbuff_get(uint8_t *buff, uint16_t len)
{
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;
    int status;

    osMutexWait(app_sco_rx_in_pcmbuff_mutex_id, osWaitForever);
    status = PeekCQueue(&app_sco_rx_in_pcm_queue, len, &e1, &len1, &e2, &len2);
    if (len==(len1+len2)){
        memcpy(buff,e1,len1);
        memcpy(buff+len1,e2,len2);
        DeCQueue(&app_sco_rx_in_pcm_queue, 0, len1);
        DeCQueue(&app_sco_rx_in_pcm_queue, 0, len2);
    }else{
        memset(buff, 0x00, len);
        status = -1;
    }
    osMutexRelease(app_sco_rx_in_pcmbuff_mutex_id);

    return status;
}

// sco_tx_out_queue process
static CQueue app_sco_rx_out_pcm_queue;
static osMutexId app_sco_rx_out_pcmbuff_mutex_id = NULL;
osMutexDef(app_sco_rx_out_pcmbuff_mutex);


int app_sco_rx_out_pcmbuff_init(uint8_t *buff, uint16_t len)
{
    if (app_sco_rx_out_pcmbuff_mutex_id  == NULL)
        app_sco_rx_out_pcmbuff_mutex_id  = osMutexCreate((osMutex(app_sco_rx_out_pcmbuff_mutex)));

    if ((buff == NULL)||(app_sco_rx_out_pcmbuff_mutex_id == NULL))
        return -1;

    osMutexWait(app_sco_rx_out_pcmbuff_mutex_id, osWaitForever);
    InitCQueue(&app_sco_rx_out_pcm_queue, len, buff);
    memset(buff, 0x00, len);
    osMutexRelease(app_sco_rx_out_pcmbuff_mutex_id);

    return 0;
}

int app_sco_rx_out_pcmbuff_length(void)
{
    int len;

    osMutexWait(app_sco_rx_out_pcmbuff_mutex_id, osWaitForever);
    len = LengthOfCQueue(&app_sco_rx_out_pcm_queue);
    osMutexRelease(app_sco_rx_out_pcmbuff_mutex_id);

    return len;
}

int app_sco_rx_out_pcmbuff_put(uint8_t *buff, uint16_t len)
{
    int status;

    osMutexWait(app_sco_rx_out_pcmbuff_mutex_id, osWaitForever);
    status = EnCQueue(&app_sco_rx_out_pcm_queue, buff, len);
    osMutexRelease(app_sco_rx_out_pcmbuff_mutex_id);

    return status;
}

int app_sco_rx_out_pcmbuff_get(uint8_t *buff, uint16_t len)
{
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;
    int status;

    osMutexWait(app_sco_rx_out_pcmbuff_mutex_id, osWaitForever);
    status = PeekCQueue(&app_sco_rx_out_pcm_queue, len, &e1, &len1, &e2, &len2);
    if (len==(len1+len2)){
        memcpy(buff,e1,len1);
        memcpy(buff+len1,e2,len2);
        DeCQueue(&app_sco_rx_out_pcm_queue, 0, len1);
        DeCQueue(&app_sco_rx_out_pcm_queue, 0, len2);
    }else{
        memset(buff, 0x00, len);
        status = -1;
    }
    osMutexRelease(app_sco_rx_out_pcmbuff_mutex_id);

    return status;
}



int wl_sco_process_init(uint32_t sco_sample_rate)
{

    uint32_t ratio = sco_sample_rate/8000;


#if defined(WL_AEC)
    // size_t total = 0, used = 0, max_used = 0;
    // speech_memory_info(&total, &used, &max_used);
    // TRACE("wl_nsx  MEM: total - %d, used - %d, max_used - %d.", total, used, max_used);
    WebRtc_aecm_init(sco_sample_rate,4);

    sco_aec_out = speech_malloc(ratio*NSX_FRAME_SIZE);

#endif

#ifdef WL_NSX

    wl_sco_sample = sco_sample_rate;

    nsx_heap = speech_malloc(NSX_BUFF_SIZE);
    wl_nsx_denoise_init(sco_sample_rate, NSX_MODE, nsx_heap);

    queue_tx_heap = speech_malloc(4*NSX_FRAME_SIZE);
    app_sco_in_pcmbuff_init(queue_tx_heap,4*NSX_FRAME_SIZE);

    queue_tx_out_heap = speech_malloc(4*NSX_FRAME_SIZE);
    app_sco_out_pcmbuff_init(queue_tx_out_heap,4*NSX_FRAME_SIZE);


    queue_rx_heap = speech_malloc(4*NSX_FRAME_SIZE);
    app_sco_rx_in_pcmbuff_init(queue_rx_heap,4*NSX_FRAME_SIZE);

    queue_rx_out_heap = speech_malloc(4*NSX_FRAME_SIZE);
    app_sco_rx_out_pcmbuff_init(queue_rx_out_heap,4*NSX_FRAME_SIZE);

    sco_out_buff = speech_malloc(ratio*NSX_FRAME_SIZE);
    sco_in_buff = speech_malloc(ratio*NSX_FRAME_SIZE);
    sco_rx_out_buff = speech_malloc(ratio*NSX_FRAME_SIZE);
    sco_rx_in_buff = speech_malloc(ratio*NSX_FRAME_SIZE);

#endif

    TRACE("wl nsx init sco_sample_rate:%d ratio:%d ",sco_sample_rate,ratio);
    return 0;
}


int wl_sco_tx_process_run(short *in_frame,short *ref_frame,uint32_t frame_len)
{


    int32_t stime = 0;
    static int32_t nsx_cnt = 0;

    nsx_cnt++;

    //DUMP16("%d,",(short*)buf,30);
    if(false == (nsx_cnt & 0x3F))
    {
        stime = hal_sys_timer_get();
	    //TRACE("aecm  echo time: lens:%d  g_time_cnt:%d ",len, g_time_cnt);
    }




#ifdef WL_NSX

    app_sco_in_pcmbuff_put((uint8_t*)in_frame,2*frame_len);

    if(16000 == wl_sco_sample)
    {
        if(app_sco_in_pcmbuff_length() >= 2*NSX_FRAME_SIZE)
        {
            app_sco_in_pcmbuff_get((uint8_t*)sco_in_buff,2*NSX_FRAME_SIZE);
            #if 0
            for (uint32_t inum = 0; inum < NSX_FRAME_SIZE; inum++)
            {
                /* code */
                sco_out_buff[inum] = sco_in_buff[inum];
            }
            #else

    #if defined(WL_AEC)

            WebRtc_aecm_farend(sco_rx_in_buff,NSX_FRAME_SIZE);
            WebRtc_aecm_process(sco_in_buff,sco_aec_out,NSX_FRAME_SIZE);
            memcpy(sco_in_buff,sco_aec_out,2*NSX_FRAME_SIZE);
            wl_nsx_16k_denoise(sco_in_buff,sco_out_buff);
    #else
            memcpy(sco_out_buff,sco_in_buff,2*NSX_FRAME_SIZE);
            //wl_nsx_16k_denoise(sco_in_buff,sco_out_buff);

    #endif

            #endif

            
            app_sco_out_pcmbuff_put((uint8_t*)sco_out_buff,2*NSX_FRAME_SIZE);
            //TRACE("a sco_in_process sco in buff:%d out buff:%d ",app_sco_in_pcmbuff_length(),app_sco_out_pcmbuff_length());
        }

    }
    else
    {
        /* code */
        if(app_sco_in_pcmbuff_length() >= 1*NSX_FRAME_SIZE)
        {
            app_sco_in_pcmbuff_get((uint8_t*)sco_in_buff,1*NSX_FRAME_SIZE);
            #if 0
            for (uint32_t inum = 0; inum < NSX_FRAME_SIZE; inum++)
            {
                /* code */
                sco_out_buff[inum] = sco_in_buff[inum];
            }
            #else

    #if defined(WL_AEC)


            WebRtc_aecm_farend(sco_rx_in_buff,NSX_FRAME_SIZE/2);
            WebRtc_aecm_process(sco_in_buff,sco_aec_out,NSX_FRAME_SIZE/2);
            memcpy(sco_in_buff,sco_aec_out,1*NSX_FRAME_SIZE);
            wl_nsx_16k_denoise(sco_in_buff,sco_out_buff);
    #else
            //memcpy(sco_out_buff,sco_in_buff,1*NSX_FRAME_SIZE);
            wl_nsx_16k_denoise(sco_in_buff,sco_out_buff);

    #endif

            #endif

            
            app_sco_out_pcmbuff_put((uint8_t*)sco_out_buff,1*NSX_FRAME_SIZE);
            //TRACE("a sco_in_process sco in buff:%d out buff:%d ",app_sco_in_pcmbuff_length(),app_sco_out_pcmbuff_length());
        }

    }
    


    // todo nsx process

    //TRACE("sco_tx_process sco out buff:%d sco_in_buff:%d ",app_sco_out_pcmbuff_length(),app_sco_in_pcmbuff_length());

    if(app_sco_out_pcmbuff_length() >= 2*frame_len)
    {
        //TRACE("b sco_out_process sco in buff:%d ",app_sco_out_pcmbuff_length());

        app_sco_out_pcmbuff_get((uint8_t*)in_frame,2*frame_len);
    }

#endif

    if(false == (nsx_cnt & 0x3F))
    {
        //nsx_cnt = 0;
        TRACE("sco_tx_process nsx_cnt:%d speed time:%d ms and frame_len:%d freq:%d ", nsx_cnt,TICKS_TO_MS(hal_sys_timer_get() - stime), frame_len,hal_sysfreq_get());
    }
    
    return 0;
}


int wl_sco_rx_process_run(short *in_frame,uint32_t frame_len)
{

    int32_t stime = 0;
    static int32_t nsx_cnt = 0;

    nsx_cnt++;

    //DUMP16("%d,",(short*)buf,30);
    if(false == (nsx_cnt & 0x3F))
    {
        stime = hal_sys_timer_get();
	    //TRACE("aecm  echo time: lens:%d  g_time_cnt:%d ",len, g_time_cnt);
    }


#ifdef WL_NSX

    app_sco_rx_in_pcmbuff_put((uint8_t*)in_frame,2*frame_len);

    if(16000 == wl_sco_sample)
    {
        if(app_sco_rx_in_pcmbuff_length() >= 2*NSX_FRAME_SIZE)
        {
            app_sco_rx_in_pcmbuff_get((uint8_t*)sco_rx_in_buff,2*NSX_FRAME_SIZE);
            #if 1
            for (uint32_t inum = 0; inum < NSX_FRAME_SIZE; inum++)
            {
                /* code */
                sco_rx_out_buff[inum] = sco_rx_in_buff[inum];
            }
            #else
            wl_nsx_16k_denoise(sco_in_buff,sco_out_buff);
            #endif

            
            app_sco_rx_out_pcmbuff_put((uint8_t*)sco_rx_out_buff,2*NSX_FRAME_SIZE);
            //TRACE("a sco_in_process sco in buff:%d out buff:%d ",app_sco_in_pcmbuff_length(),app_sco_out_pcmbuff_length());
        }
    }
    else
    {
        if(app_sco_rx_in_pcmbuff_length() >= 1*NSX_FRAME_SIZE)
        {
            app_sco_rx_in_pcmbuff_get((uint8_t*)sco_rx_in_buff,1*NSX_FRAME_SIZE);
            #if 1
            for (uint32_t inum = 0; inum < NSX_FRAME_SIZE/2; inum++)
            {
                /* code */
                sco_rx_out_buff[inum] = sco_rx_in_buff[inum];
            }
            #else
            wl_nsx_16k_denoise(sco_in_buff,sco_out_buff);
            #endif

            
            app_sco_rx_out_pcmbuff_put((uint8_t*)sco_rx_out_buff,1*NSX_FRAME_SIZE);
            //TRACE("a sco_in_process sco in buff:%d out buff:%d ",app_sco_in_pcmbuff_length(),app_sco_out_pcmbuff_length());
        }
    }



    // todo nsx process

    //TRACE("sco_tx_process sco out buff:%d sco_in_buff:%d ",app_sco_out_pcmbuff_length(),app_sco_in_pcmbuff_length());

    if(app_sco_rx_out_pcmbuff_length() >= 2*frame_len)
    {
        //TRACE("b sco_out_process sco in buff:%d ",app_sco_out_pcmbuff_length());

        app_sco_rx_out_pcmbuff_get((uint8_t*)in_frame,2*frame_len);
    }

#endif


    if(false == (nsx_cnt & 0x3F))
    {
        nsx_cnt = 0;
        TRACE("sco_rx_process speed time:%d ms and frame_lens:%d freq:%d ", TICKS_TO_MS(hal_sys_timer_get() - stime), frame_len,hal_sysfreq_get());
    }

    return 0;
}

int wl_sco_process_exit(void)
{

#if defined(WL_NSX)

    wl_nsx_exit();
    speech_free(nsx_heap);
    nsx_heap = NULL;

    speech_free(queue_tx_heap);
    queue_tx_heap = NULL;

    speech_free(queue_tx_out_heap);
    queue_tx_out_heap = NULL;

    speech_free(queue_rx_heap);
    queue_rx_heap = NULL;

    speech_free(queue_rx_out_heap);
    queue_rx_out_heap = NULL;

    speech_free(sco_out_buff);
    sco_out_buff = NULL;
    speech_free(sco_in_buff);
    sco_in_buff = NULL;
    speech_free(sco_rx_out_buff);
    sco_rx_out_buff = NULL;
    speech_free(sco_rx_in_buff);
    sco_rx_in_buff = NULL;
#endif

#if defined(WL_AEC)
    wl_aec_exit();
    speech_free(sco_aec_out);
    sco_aec_out = NULL;
#endif

    TRACE("sco_process success exit");

    return 0;
}