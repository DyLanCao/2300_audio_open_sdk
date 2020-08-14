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
#include "hal_trace.h"
#include "audioflinger.h"
#include "app_overlay.h"
#include "app_audio.h"
#include "app_utils.h"
#include "hal_aud.h"
#include "string.h"
#include "cmsis_os.h"
#include "app_smartvoice.h"
#include "g726.h"
#include "app_sv_cmd_code.h"
#include "app_sv_cmd_handler.h"
#include "app_sv_data_handler.h"
#include "app_sv.h"
#include "voice_opus.h"
#include "voice_sbc.h"
#include "cqueue.h"
#include "app_spp.h"
#include <stdlib.h>
#include "hal_timer.h"
#include "nvrecord.h"

extern "C" {
#include "eventmgr.h"
#include "me.h"
#include "sec.h"
#include "a2dp.h"
#include "avdtp.h"
#include "avctp.h"
#include "avrcp.h"
#include "btalloc.h"
}

#include "btapp.h"
#include "app_bt.h"
#include "app_bt_media_manager.h"
#include "apps.h"
#include "app_thread.h"
#include "cqueue.h"
#include "hal_location.h"
#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif
#include "app_voicepath.h"

#define APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS 5000

#define APP_SMARTVOICE_STOP_REQ                  (4)
#define APP_SMARTVOICE_STOP_RSP                  (5)

#define MAX_CMD_BUFFER                      (20)

#if IS_USE_ENCODED_DATA_SEQN_PROTECTION
#define SMARTVOICE_ENCODED_DATA_SEQN_SIZE	    sizeof(uint8_t)
#else
#define SMARTVOICE_ENCODED_DATA_SEQN_SIZE	    0
#endif

#if IS_SMARTVOICE_DEBUG_ON
#define SMARTVOICE_TRACE    TRACE
#else
#define SMARTVOICE_TRACE(str, ...)    
#endif

/**< configure the voice settings   */
#define VOB_VOICE_BIT_NUMBER                    (AUD_BITS_16)
#if defined(__AUDIO_RESAMPLE__)
#define VOB_VOICE_SAMPLE_RATE                   (AUD_SAMPRATE_16927)
#else
#define VOB_VOICE_SAMPLE_RATE                   (AUD_SAMPRATE_16000)
#endif
#define VOB_TARGET_VOICE_SAMPLE_RATE            (AUD_SAMPRATE_16000)
#define VOB_VOICE_CAPTURE_VOLUME                (12)
#define VOB_VOICE_SIDE_TONE_VOLUME              (12)
#define VOB_MIC_CHANNEL_COUNT					1

#define VOB_PCM_SIZE_TO_SAMPLE_CNT(size)		((size)*8/VOB_VOICE_BIT_NUMBER)
#define VOB_SAMPLE_CNT_TO_PCM_SIZE(sample_cnt)	((sample_cnt)*VOB_VOICE_BIT_NUMBER/8)

#define VOB_ENCODED_DATA_STORAGE_BUF_SIZE       (4000)
#define VOB_PCM_DATA_BUF_SIZE                   (4000)

#if VOB_ENCODING_ALGORITHM==ENCODING_ALGORITHM_OPUS
#define VOB_VOICE_CAPTURE_INTERVAL_IN_MS        (100)
#define VOB_VOICE_CAPTURE_FRAME_CNT             (VOB_VOICE_CAPTURE_INTERVAL_IN_MS/VOICE_FRAME_PERIOD_IN_MS)
#define VOB_VOICE_PCM_FRAME_SIZE                (VOICE_OPUS_PCM_DATA_SIZE_PER_FRAME)
#define VOB_VOICE_PCM_CAPTURE_CHUNK_SIZE        (VOB_VOICE_PCM_FRAME_SIZE*VOB_VOICE_CAPTURE_FRAME_CNT)
#define VOB_VOICE_ENCODED_DATA_FRAME_SIZE       (VOICE_OPUS_ENCODED_DATA_SIZE_PER_FRAME)
#elif VOB_ENCODING_ALGORITHM==ENCODING_ALGORITHM_SBC
#undef  VOICE_SBC_CAPTURE_INTERVAL_IN_MS
#define VOICE_SBC_CAPTURE_INTERVAL_IN_MS	    (48)

#define VOB_VOICE_CAPTURE_INTERVAL_IN_MS        (VOICE_SBC_CAPTURE_INTERVAL_IN_MS)
#define VOB_VOICE_CAPTURE_FRAME_CNT             (VOICE_SBC_CAPTURE_INTERVAL_IN_MS/VOICE_SBC_FRAME_PERIOD_IN_MS)
#define VOB_VOICE_PCM_FRAME_SIZE                (VOICE_SBC_PCM_DATA_SIZE_PER_FRAME)
#define VOB_VOICE_PCM_CAPTURE_CHUNK_SIZE        (VOICE_SBC_PCM_DATA_SIZE_PER_FRAME*VOB_VOICE_CAPTURE_FRAME_CNT)
#define VOB_VOICE_ENCODED_DATA_FRAME_SIZE       (VOICE_SBC_ENCODED_DATA_SIZE_PER_FRAME)
#else
#define VOB_VOICE_CAPTURE_INTERVAL_IN_MS        (100)
#define VOB_VOICE_PCM_FRAME_SIZE                (32000/10)
#define VOB_VOICE_PCM_CAPTURE_CHUNK_SIZE        (VOB_VOICE_PCM_FRAME_SIZE)
#endif

#define VOB_VOICE_XFER_CHUNK_SIZE               (VOB_VOICE_ENCODED_DATA_FRAME_SIZE*VOB_VOICE_CAPTURE_FRAME_CNT)

#define VOB_IGNORED_FRAME_CNT                   (2)

// ping-pong buffer for codec
#define VOB_VOICE_PCM_DATA_SIZE                 (VOB_VOICE_PCM_CAPTURE_CHUNK_SIZE*2)

#define DEFAULT_SMARTVOICE_SPP_DATA_SEC_SIZE    600
#define DEFAULT_SMARTVOICE_BLE_DATA_SEC_SIZE    20

#define TMP_BUF_FOR_ENCODING_DECODING_SIZE      1024

#define VOB_LOOPBACK_PLAYBACK_CHANNEL_NUM       AUD_CHANNEL_NUM_2

#if VOB_ENCODING_ALGORITHM==ENCODING_ALGORITHM_OPUS
#define SMARTVOICE_SYSTEM_FREQ       APP_SYSFREQ_208M   
#else
#define SMARTVOICE_SYSTEM_FREQ       APP_SYSFREQ_104M 
#endif

typedef struct
{
    CQueue      encodedDataBufQueue;
    uint8_t*    encodedDataBuf;
	uint8_t		isAudioAlgorithmEngineReseted;
    uint8_t*    tmpBufForEncoding;
    uint8_t*    tmpBufForXfer;
    
    uint32_t    sentDataSizeWithInFrame;
    uint8_t     seqNumWithInFrame;    
    CQueue      pcmDataBufQueue;
    uint8_t*    pcmDataBuf;    
    uint8_t*    tmpPcmDataBuf;    


} APP_SMARTVOICE_AUDIO_STREAM_ENV_T;           

typedef enum
{
	CLOUD_PLATFORM_0,
	CLOUD_PLATFORM_1,	
	CLOUD_PLATFORM_2,
} CLOUD_PLATFORM_E;

typedef struct {
    volatile uint8_t                     isSvStreamRunning   : 1;
    volatile uint8_t                     isMicStreamOpened   : 1;
    volatile uint8_t                     isDataXferOnGoing   : 1;
    volatile uint8_t                     isPlaybackStreamRunning : 1;
    volatile uint8_t            		 isThroughputTestOn  : 1;
    uint8_t                     reserve             : 3;
    uint8_t                     connType;   
    uint8_t                     conidx;
    APP_SV_TRANSMISSION_PATH_E  pathType;
    uint16_t                    smartVoiceDataSectorSize;
    CLOUD_PLATFORM_E            currentPlatform;
    
} smartvoice_context_t;

typedef struct
{
    uint8_t           voiceStreamPathToUse;
} START_VOICE_STREAM_REQ_T;

typedef struct
{
    uint8_t           voiceStreamPathToUse;
} START_VOICE_STREAM_RSP_T;

typedef struct
{
    uint8_t           cloudMusicPlayingControlReq;
} CLOUD_MUSIC_PLAYING_CONTROL_REQ_T;

typedef struct
{
    uint8_t           cloudPlatform;
} CLOUD_PLATFORM_CHOOSING_REQ_T;

typedef struct 
{
    uint8_t     algorithm;		// ENCODING_ALGORITHM_OPUS or ENCODING_ALGORITHM_SBC
    uint8_t     frameCountPerXfer;
    uint16_t    encodedFrameSize;
	uint16_t    pcmFrameSize;
	
	uint8_t		channelCnt;
	uint8_t		complexity;
	uint8_t		packetLossPercentage;
	uint8_t		sizePerSample;
	uint16_t	appType;	
	uint16_t	bandWidth;
	uint32_t	bitRate;
	uint32_t	sampleRate;
	uint32_t	signalType;
	uint32_t	periodPerFrame;
	uint8_t		isUseVbr;
	uint8_t		isConstraintUseVbr;
	uint8_t		isUseInBandFec;
	uint8_t		isUseDtx;
} __attribute__((packed)) VOICE_OPUS_CONFIG_INFO_T;

typedef struct 
{
    uint8_t     algorithm;		// ENCODING_ALGORITHM_OPUS or ENCODING_ALGORITHM_SBC
    uint8_t     frameCountPerXfer;
    uint16_t    encodedFrameSize;
	uint16_t    pcmFrameSize;

    uint16_t    blockSize;
	uint8_t		channelCnt;
	uint8_t		channelMode;
	uint8_t		bitPool;
	uint8_t		sizePerSample;
	uint8_t		sampleRate;
	uint8_t		numBlocks;
	uint8_t 	numSubBands;
	uint8_t		mSbcFlag;
	uint8_t		allocMethod;

} __attribute__((packed)) VOICE_SBC_CONFIG_INFO_T;
static osMutexId vob_env_mutex_id;
osMutexDef(vob_env_mutex);

static APP_SMARTVOICE_AUDIO_STREAM_ENV_T    smartvoice_audio_stream;
static smartvoice_context_t                 smartvoice_env;
#ifdef MIX_MIC_DURING_MUSIC
static uint32_t more_mic_data(uint8_t* ptrBuf, uint32_t length);
#endif

static uint32_t ignoredPcmDataRounds = 0; 
extern void app_hfp_enable_audio_link(bool isEnable);
void app_bt_stream_copy_track_one_to_two_16bits(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len);
static void app_smartvoice_start_stream(void);
static void app_smartvoice_stop_stream(void);
void app_smartvoice_mailbox_put(uint32_t message_id, uint32_t param0, uint32_t param1, uint32_t ptr);

static void app_smart_voice_mic_timeout_timer_cb(void const *n);
osTimerDef (APP_SMART_VOICE_MIC_TIMEOUT, app_smart_voice_mic_timeout_timer_cb); 
osTimerId	app_mic_timeout_timer_id = NULL;

static void app_smart_voice_mic_timeout_timer_cb(void const *n)
{
    app_smartvoice_mailbox_put(0, 0, 0, (uint32_t)app_sv_stop_voice_stream);
}

static void start_capture_stream(void)
{
    if (!app_audio_manager_capture_is_active())
    {
        app_voice_report(APP_STATUS_INDICATION_ALEXA_START, 0);
        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START, 
            BT_STREAM_CAPTURE, 0, 0);
    }
}

int app_voicepath_stop_audio_stream(void)
{
    if (smartvoice_env.isSvStreamRunning)
    {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

        if (smartvoice_env.isPlaybackStreamRunning)
        {
            smartvoice_env.isPlaybackStreamRunning = false;
            af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
            af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        }
        
        // release the acquired system clock
        app_sysfreq_req(APP_SYSFREQ_USER_VOICEPATH, APP_SYSFREQ_32K);

        capture_stream_deinit_disctributor();

    #ifdef MIX_MIC_DURING_MUSIC
        app_voice_deinit_resampler();
        app_voicepath_disable_hw_sidetone();
    #endif

        app_voicepath_set_pending_started_stream(VOICEPATH_STREAMING, false);
        app_voicepath_set_stream_state(VOICEPATH_STREAMING, false);

        if (app_voicepath_get_stream_pending_state(AUDIO_OUTPUT_STREAMING))
        {
            af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
            app_voicepath_set_stream_state(AUDIO_OUTPUT_STREAMING, true);
            app_voicepath_set_pending_started_stream(AUDIO_OUTPUT_STREAMING, false);
        }

        smartvoice_env.isSvStreamRunning = false;
        return 0;
    }
    else
    {
        return -1;
    }
}

void app_voicepath_init_voice_encoder(void)
{
#ifdef __THIRDPARTY
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO1,THIRDPARTY_STOP);
#endif
    
    // encoded data buffer
    app_capture_audio_mempool_get_buff(&(smartvoice_audio_stream.encodedDataBuf),   
        VOB_ENCODED_DATA_STORAGE_BUF_SIZE);
    InitCQueue(&(smartvoice_audio_stream.encodedDataBufQueue), VOB_ENCODED_DATA_STORAGE_BUF_SIZE, 
                (CQItemType *)(smartvoice_audio_stream.encodedDataBuf));

    app_capture_audio_mempool_get_buff(&(smartvoice_audio_stream.tmpBufForEncoding),   
            TMP_BUF_FOR_ENCODING_DECODING_SIZE);
	app_capture_audio_mempool_get_buff(&(smartvoice_audio_stream.tmpBufForXfer),   
            VOB_VOICE_XFER_CHUNK_SIZE);

    app_capture_audio_mempool_get_buff(&(smartvoice_audio_stream.pcmDataBuf),   
        VOB_PCM_DATA_BUF_SIZE);
    InitCQueue(&(smartvoice_audio_stream.pcmDataBufQueue), VOB_PCM_DATA_BUF_SIZE, 
                (CQItemType *)(smartvoice_audio_stream.pcmDataBuf));
    app_capture_audio_mempool_get_buff(&(smartvoice_audio_stream.tmpPcmDataBuf),   
                VOB_VOICE_PCM_FRAME_SIZE);

    smartvoice_audio_stream.seqNumWithInFrame = 0;
    smartvoice_audio_stream.sentDataSizeWithInFrame = 0; 
	
#if VOB_ENCODING_ALGORITHM==ENCODING_ALGORITHM_OPUS
    uint8_t* opus_voice_heap_ptr;
    app_capture_audio_mempool_get_buff(&opus_voice_heap_ptr,   
   		VOICE_OPUS_HEAP_SIZE);
	uint8_t* opus_voice_stack_ptr;
	app_capture_audio_mempool_get_buff(&opus_voice_stack_ptr,	
		VOICE_OPUS_STACK_SIZE);

    VOICE_OPUS_CONFIG_T opusConfig = 
    {
        VOICE_OPUS_HEAP_SIZE,
        VOICE_OPUS_CHANNEL_COUNT,
        VOICE_OPUS_COMPLEXITY,
        VOICE_OPUS_PACKET_LOSS_PERC,
        VOICE_SIZE_PER_SAMPLE,
        VOICE_OPUS_APP,
        VOICE_OPUS_BANDWIDTH,
        VOICE_OPUS_BITRATE,
        VOICE_OPUS_SAMPLE_RATE,
        VOICE_SIGNAL_TYPE,
        VOICE_FRAME_PERIOD,
        
        VOICE_OPUS_USE_VBR,
        VOICE_OPUS_CONSTRAINT_USE_VBR,
        VOICE_OPUS_USE_INBANDFEC,
        VOICE_OPUS_USE_DTX,
    };
    
    voice_opus_init(&opusConfig, opus_voice_heap_ptr, opus_voice_stack_ptr);


#elif VOB_ENCODING_ALGORITHM==ENCODING_ALGORITHM_SBC
    static VOICE_SBC_CONFIG_T	sbcConfig =
    {
        VOICE_SBC_CHANNEL_COUNT     ,
        VOICE_SBC_CHANNEL_MODE 	    ,
        VOICE_SBC_BIT_POOL	 	    ,
        VOICE_SBC_SIZE_PER_SAMPLE   ,
        VOICE_SBC_SAMPLE_RATE	    ,
        VOICE_SBC_NUM_BLOCKS	    ,
        VOICE_SBC_NUM_SUB_BANDS	    ,
        VOICE_SBC_MSBC_FLAG		    ,
        VOICE_SBC_ALLOC_METHOD	    ,
    };

    voice_sbc_init(&sbcConfig);
#endif

    smartvoice_audio_stream.isAudioAlgorithmEngineReseted = true;
}

void app_voicepath_deinit_voice_encoder(void)
{
#if VOB_ENCODING_ALGORITHM==ENCODING_ALGORITHM_OPUS    
    voice_opus_deinit();
#endif

#ifdef __THIRDPARTY
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO1,THIRDPARTY_START);
#endif
}

extern enum AUD_SAMPRATE_T a2dp_sample_rate;
extern uint32_t a2dp_data_buf_size;
int app_voicepath_start_audio_stream(void)
{
    if (!smartvoice_env.isSvStreamRunning)
    {
        struct AF_STREAM_CONFIG_T stream_cfg;

        TRACE("%s", __func__);
        app_capture_audio_mempool_init();
        
        uint8_t* bt_audio_buff = NULL;

    #if IS_FORCE_STOP_VOICE_STREAM
        if (NULL != app_mic_timeout_timer_id) {
            osTimerStop(app_mic_timeout_timer_id); 
            osTimerStart(app_mic_timeout_timer_id, APP_SMARTVOICE_MIC_TIMEOUT_INTERVEL);
        }
    #endif

        // acuqire the sufficient system clock
        app_sysfreq_req(APP_SYSFREQ_USER_VOICEPATH, SMARTVOICE_SYSTEM_FREQ);


        // create the audio flinger stream
        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits = VOB_VOICE_BIT_NUMBER;
        stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)VOB_MIC_CHANNEL_COUNT;

#ifdef MIX_MIC_DURING_MUSIC
        if (!app_voicepath_get_stream_state(AUDIO_OUTPUT_STREAMING))
        {          
            stream_cfg.data_size = VOB_VOICE_PCM_DATA_SIZE;
            stream_cfg.sample_rate = VOB_VOICE_SAMPLE_RATE;
            app_voice_setup_resampler(false);
        }
        else
        {            
            // The reason to create the resampler is:
            // To use the HW side-tone, the capture sample rate has to be the same as the playback's
            stream_cfg.sample_rate = app_voicepath_audio_output_sample_rate();
            stream_cfg.data_size = app_voicepath_audio_output_data_buf_size()/2;
            TRACE("audio_output_sample_rate %d", stream_cfg.sample_rate);
            if (stream_cfg.sample_rate != VOB_VOICE_SAMPLE_RATE)
            {
                app_voice_setup_resampler(true);
                uint32_t resample_unit_size = 
                    (((uint32_t)((float)stream_cfg.data_size/2/(((float)stream_cfg.sample_rate)/((float)VOB_VOICE_SAMPLE_RATE)))) + 3)/4*4; 
                app_voicepath_init_resampler((uint8_t)VOB_MIC_CHANNEL_COUNT, resample_unit_size, 
                    stream_cfg.sample_rate, VOB_VOICE_SAMPLE_RATE); 
            }
            else
            {
                app_voice_setup_resampler(false);
            }
        }
#else
        stream_cfg.data_size = VOB_VOICE_PCM_DATA_SIZE;
        stream_cfg.sample_rate = VOB_VOICE_SAMPLE_RATE;  
#endif

#if defined(__AUDIO_RESAMPLE__)
        app_voice_setup_resampler(true);
        uint32_t resample_unit_size = 
            (((uint32_t)((float)stream_cfg.data_size/2/(((float)stream_cfg.sample_rate)/((float)VOB_TARGET_VOICE_SAMPLE_RATE)))) + 3)/4*4; 
        app_voicepath_init_resampler((uint8_t)VOB_MIC_CHANNEL_COUNT, resample_unit_size, 
            stream_cfg.sample_rate, VOB_VOICE_SAMPLE_RATE); 

#endif

        app_capture_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
        stream_cfg.vol = VOB_VOICE_CAPTURE_VOLUME;       
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        ignoredPcmDataRounds = 0;

        stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
        stream_cfg.handler = capture_stream_mic_data_disctributor;
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

        capture_stream_init_disctributor();
        
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
            
        // if a2dp streaming is not on, start the playback stream
        if (!app_voicepath_get_stream_state(AUDIO_OUTPUT_STREAMING))
        {              
        #ifdef MIX_MIC_DURING_MUSIC
            TRACE("A2dp streaming is not on, so start the loopback playback stream.");

            smartvoice_env.isPlaybackStreamRunning = true;
            
            stream_cfg.vol = app_bt_stream_volume_get_ptr()->a2dp_vol;
            stream_cfg.channel_num = VOB_LOOPBACK_PLAYBACK_CHANNEL_NUM;
            stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
            stream_cfg.handler = more_mic_data;
            stream_cfg.data_size = VOB_VOICE_PCM_DATA_SIZE*VOB_LOOPBACK_PLAYBACK_CHANNEL_NUM;

            // can make use of the audio mempool
            app_audio_mempool_init_with_specific_size(APP_AUDIO_BUFFER_SIZE);
            uint8_t* bt_audio_buff_play;
            app_audio_mempool_get_buff(&bt_audio_buff_play, stream_cfg.data_size);
            stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff_play);
            af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
            af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

            app_voicepath_enable_hw_sidetone(0, 0);
        #endif  
            app_voicepath_set_stream_state(VOICEPATH_STREAMING, true);
            af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        }
        else
        {            
            app_voicepath_set_pending_started_stream(VOICEPATH_STREAMING, true);       
        }
        
        smartvoice_env.isSvStreamRunning = true;
        return 0;
    }
    else
    {
        return -1;
    }
}

void app_smartvoice_send_done(void);
void app_voicepath_connected(uint8_t connType)
{
    smartvoice_env.connType |= connType;
    if (APP_SMARTVOICE_BLE_CONNECTED == connType)
    {
        smartvoice_env.pathType = APP_SV_VIA_NOTIFICATION;
        smartvoice_env.smartVoiceDataSectorSize = DEFAULT_SMARTVOICE_BLE_DATA_SEC_SIZE;
        app_sv_register_tx_done(app_smartvoice_send_done);

    }
    else
    {
        smartvoice_env.pathType = APP_SV_VIA_SPP;
        smartvoice_env.smartVoiceDataSectorSize = DEFAULT_SMARTVOICE_SPP_DATA_SEC_SIZE;
        
        app_spp_register_tx_done(app_smartvoice_send_done);
    }

    app_sv_data_reset_env();
    app_sv_data_set_path_type(smartvoice_env.pathType);
    
    TRACE("SV connection event, Conn type 0x%x, path type 0x%x", smartvoice_env.connType, smartvoice_env.pathType);
}

void app_voicepath_disconnected(uint8_t disconnType) 
{
    if (APP_SMARTVOICE_BLE_DISCONNECTED == disconnType)
    {
        app_bt_allow_sniff(BT_DEVICE_ID_1);
    }
	app_sv_data_reset_env();

    app_sv_stop_throughput_test();
    smartvoice_env.connType &= disconnType;

    app_smartvoice_stop_stream();

    TRACE("SV disconnected.");
}

static void app_smartvoice_send_encoded_audio_data(void)
{
    TRACE("isOn %d", smartvoice_env.isDataXferOnGoing);
    if (smartvoice_env.isDataXferOnGoing)
    {
        return;
    }

    if (!app_sv_data_is_data_transmission_started())
    {
        return;
    }

    if (LengthOfCQueue(&(smartvoice_audio_stream.encodedDataBufQueue)) < VOB_VOICE_XFER_CHUNK_SIZE)
    {
        return;
    }
    
	uint32_t dataBytesToSend = 0;
	
	osMutexWait(vob_env_mutex_id, osWaitForever);

    uint32_t encodedDataLength = LengthOfCQueue(&(smartvoice_audio_stream.encodedDataBufQueue));
    if (encodedDataLength > 0)
    {
        if (encodedDataLength > smartvoice_env.smartVoiceDataSectorSize)
        {
            dataBytesToSend = smartvoice_env.smartVoiceDataSectorSize;
        }
        else
        {
            dataBytesToSend = encodedDataLength;
        }

        if ((smartvoice_audio_stream.sentDataSizeWithInFrame + dataBytesToSend) >= VOB_VOICE_XFER_CHUNK_SIZE)
        {
            dataBytesToSend = VOB_VOICE_XFER_CHUNK_SIZE - smartvoice_audio_stream.sentDataSizeWithInFrame;
        }

        smartvoice_audio_stream.sentDataSizeWithInFrame += dataBytesToSend;
    }

	osMutexRelease(vob_env_mutex_id);

	if (dataBytesToSend > 0)
	{
        smartvoice_env.isDataXferOnGoing = true;
		SMARTVOICE_TRACE("Send %d bytes of encoded data to DST, former encoded data size %d current encoded data size %d", 
			dataBytesToSend, encodedDataLength, encodedDataLength-dataBytesToSend);

        memcpy(smartvoice_audio_stream.tmpBufForXfer, &(smartvoice_audio_stream.seqNumWithInFrame), 
            SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
        app_pop_data_from_cqueue(&(smartvoice_audio_stream.encodedDataBufQueue), 
            smartvoice_audio_stream.tmpBufForXfer + SMARTVOICE_ENCODED_DATA_SEQN_SIZE, 
            dataBytesToSend);

        app_sv_send_data(smartvoice_env.pathType, 
            smartvoice_audio_stream.tmpBufForXfer, 
            dataBytesToSend + SMARTVOICE_ENCODED_DATA_SEQN_SIZE);

        if (smartvoice_audio_stream.sentDataSizeWithInFrame < VOB_VOICE_XFER_CHUNK_SIZE)
        {
            smartvoice_audio_stream.seqNumWithInFrame++;
        }
        else
        {
            smartvoice_audio_stream.seqNumWithInFrame = 0;
            smartvoice_audio_stream.sentDataSizeWithInFrame = 0;
        }
	}
}

#ifdef MIX_MIC_DURING_MUSIC
static uint32_t more_mic_data(uint8_t* ptrBuf, uint32_t length)
{ 
    // during the HW side-tone, just fill the mute data to the playback stream
    memset(ptrBuf, 0, length);
    return length;
}
#endif

void smartvoice_stream_deinit(void)
{
    
}

// this function will be called for two cases:
// 1. Without the a2dp streaming, called at the time of the audio data captured
// 2. With the a2dp streaming, called when the audio data re-sampling is done
int app_voicepath_mic_data_come(uint8_t* ptrBuf, uint32_t length)
{        
    if (!smartvoice_env.isMicStreamOpened)
    {
        return length;
    }

	if (0 == AvailableOfCQueue(&(smartvoice_audio_stream.pcmDataBufQueue)))
	{
		SMARTVOICE_TRACE("voice PCM data buffer is full!!!");
		return length;
	}

    EnCQueue(&(smartvoice_audio_stream.pcmDataBufQueue), 
                ptrBuf, 
                length);     
    return length;
}

int app_voicepath_resampled_data_come(uint8_t* ptrBuf, uint32_t length)
{
    TRACE("Get resampled data %d bytes", length);
    app_voicepath_mic_data_come(ptrBuf, length);
    app_voicepath_mic_data_encoding();
    return 0;
}

void app_voicepath_mic_data_encoding(void)
{
    uint32_t length = LengthOfCQueue(&(smartvoice_audio_stream.pcmDataBufQueue));
    if (0 == length)
	{
		SMARTVOICE_TRACE("voice PCM data buffer is empty!!!");
		return;
	}
        
	if (0 == AvailableOfCQueue(&(smartvoice_audio_stream.encodedDataBufQueue)))
	{
		SMARTVOICE_TRACE("voice encoded data buffer is full!!!");
		return;
	}

    uint8_t* ptrBuf = smartvoice_audio_stream.tmpPcmDataBuf;

    // encode the voice PCM data
    SMARTVOICE_TRACE("%s len:%d", __func__, length);

    uint32_t outputSize;
    uint32_t formerTicks = hal_sys_timer_get();
    
    while (LengthOfCQueue(&(smartvoice_audio_stream.pcmDataBufQueue)) >= VOB_VOICE_PCM_FRAME_SIZE)
    {  
        if (ignoredPcmDataRounds < VOB_IGNORED_FRAME_CNT)
        {
            ignoredPcmDataRounds++;
            app_pop_data_from_cqueue(&(smartvoice_audio_stream.pcmDataBufQueue), 
                ptrBuf, 
                VOB_VOICE_PCM_FRAME_SIZE);
            memset(ptrBuf, 0, VOB_VOICE_PCM_FRAME_SIZE);
        }
        else
        {
            app_pop_data_from_cqueue(&(smartvoice_audio_stream.pcmDataBufQueue), 
                ptrBuf, 
                VOB_VOICE_PCM_FRAME_SIZE);
        }
            
#if VOB_ENCODING_ALGORITHM==ENCODING_ALGORITHM_ADPCM
    	outputSize = g726_Encode(ptrBuf,
                ((char*)(smartvoice_audio_stream.tmpBufForEncoding)), 
                VOB_PCM_SIZE_TO_SAMPLE_CNT(VOB_VOICE_PCM_FRAME_SIZE), 
                smartvoice_audio_stream.isAudioAlgorithmEngineReseted);
        SMARTVOICE_TRACE("adpcm outputs %d bytes", outputSize);
        EnCQueue(&(smartvoice_audio_stream.encodedDataBufQueue), 
                smartvoice_audio_stream.tmpBufForEncoding, 
                outputSize);  
#elif VOB_ENCODING_ALGORITHM==ENCODING_ALGORITHM_OPUS
        outputSize = voice_opus_encode(ptrBuf,
    			smartvoice_audio_stream.tmpBufForEncoding, 
    			VOB_PCM_SIZE_TO_SAMPLE_CNT(VOB_VOICE_PCM_FRAME_SIZE), 
    			smartvoice_audio_stream.isAudioAlgorithmEngineReseted);
    			
        SMARTVOICE_TRACE("opus outputs %d bytes", outputSize);
        EnCQueue(&(smartvoice_audio_stream.encodedDataBufQueue), 
                smartvoice_audio_stream.tmpBufForEncoding, 
                outputSize);   
#elif VOB_ENCODING_ALGORITHM==ENCODING_ALGORITHM_SBC
        uint32_t purchasedBytes;
        outputSize = voice_sbc_encode(ptrBuf, 
                VOB_VOICE_PCM_FRAME_SIZE, &purchasedBytes,
    		    smartvoice_audio_stream.tmpBufForEncoding, 
    			smartvoice_audio_stream.isAudioAlgorithmEngineReseted);
        SMARTVOICE_TRACE("SBC outputs %d bytes", outputSize);

        // assure this is always the case, as every encoding will come out with one time slice
        // with the times of frame length
#if 0 //IS_IGNORME_SBC_FRAME_HEADER  
        uint32_t frameLen = voice_sbc_get_frame_len();
        uint32_t frameCount = outputSize/frameLen;
        uint32_t trimedLen = frameLen - VOICE_SBC_FRAME_HEADER_LEN;
        uint32_t offsetToFill = 0;
        uint32_t offsetToFetch = VOICE_SBC_FRAME_HEADER_LEN;
        for (uint8_t frame = 0;frame < frameCount;frame++)
        {
            memcpy(&smartvoice_audio_stream.tmpBufForEncoding[offsetToFill],
                &smartvoice_audio_stream.tmpBufForEncoding[offsetToFetch],
                trimedLen);
            offsetToFill += trimedLen;
            offsetToFetch += frameLen;
        }
        outputSize = offsetToFill;
#endif

        EnCQueue(&(smartvoice_audio_stream.encodedDataBufQueue), 
                smartvoice_audio_stream.tmpBufForEncoding, 
                outputSize);    
#else
        EnCQueue(&(smartvoice_audio_stream.decodedDataBufQueue), ptrBuf, 
                    length);
        return length;
#endif
    	smartvoice_audio_stream.isAudioAlgorithmEngineReseted = false;  

    }

    TRACE("Voice encoding cost %d ms", TICKS_TO_MS(hal_sys_timer_get()-formerTicks));

	app_smartvoice_send_encoded_audio_data();

    return;
}

void app_smartvoice_send_done(void)
{
    TRACE("Clear isOn.");
    SMARTVOICE_TRACE("Data sent out");

    smartvoice_env.isDataXferOnGoing = false;
    if (LengthOfCQueue(&(smartvoice_audio_stream.encodedDataBufQueue))> 0)
	{	
        app_smartvoice_send_encoded_audio_data();
	}
    if (smartvoice_env.isThroughputTestOn)
    {
        app_sv_throughput_test_transmission_handler();
    }
}

void app_smartvoice_start_xfer(void)
{
#if 0  
    app_sv_update_conn_parameter(SV_HIGH_SPEED_BLE_CONNECTION_INTERVAL_MIN_IN_MS, 
        SV_HIGH_SPEED_BLE_CONNECTION_INTERVAL_MAX_IN_MS, 
        SV_HIGH_SPEED_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS,0);
#endif

#if IS_ENABLE_START_DATA_XFER_STEP                
    APP_SV_START_DATA_XFER_T req;
    memset((uint8_t *)&req, 0, sizeof(req));
    req.isHasCrcCheck = false;
    app_sv_start_data_xfer(smartvoice_env.pathType, &req);
#else
    app_sv_kickoff_dataxfer();
#endif
}

void app_smartvoice_stop_xfer(void)
{
#if 0  
    app_sv_update_conn_parameter(SV_LOW_SPEED_BLE_CONNECTION_INTERVAL_MIN_IN_MS, 
        SV_LOW_SPEED_BLE_CONNECTION_INTERVAL_MAX_IN_MS, 
        SV_LOW_SPEED_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS,0);
#endif

#if IS_ENABLE_START_DATA_XFER_STEP     
    APP_SV_STOP_DATA_XFER_T req;
    memset((uint8_t *)&req, 0, sizeof(req));
    req.isHasWholeCrcCheck = false;
    app_sv_stop_data_xfer(smartvoice_env.pathType, &req);  
#else
    app_sv_stop_dataxfer();
#endif
}

void app_sv_data_xfer_started(APP_SV_CMD_RET_STATUS_E retStatus)
{
    TRACE("SV data xfer is started with ret %d", retStatus);
}

void app_sv_data_xfer_stopped(APP_SV_CMD_RET_STATUS_E retStatus)
{
    app_hfp_enable_audio_link(false);
    TRACE("SV data xfer is stopped with ret %d", retStatus);
}

void app_sv_start_voice_stream_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    uint8_t voicePathType = ((START_VOICE_STREAM_REQ_T *)ptrParam)->voiceStreamPathToUse;
    TRACE("Received the wakeup req from the remote dev. Voice Path type %d", voicePathType);
    START_VOICE_STREAM_RSP_T rsp;
    rsp.voiceStreamPathToUse = voicePathType;
    
    if (!smartvoice_env.isMicStreamOpened)
    {     
        if ((APP_SMARTVOICE_PATH_TYPE_BLE == voicePathType) ||     
            (APP_SMARTVOICE_PATH_TYPE_SPP == voicePathType))
        {    
            if (app_voicepath_check_hfp_active())
            {
                app_sv_send_response_to_command(funcCode, CMD_HANDLING_FAILED, (uint8_t *)&rsp, sizeof(rsp), smartvoice_env.pathType);
            }
            else
            {
                app_sv_send_response_to_command(funcCode, NO_ERROR, (uint8_t *)&rsp, sizeof(rsp), smartvoice_env.pathType);
                
                app_smartvoice_start_stream();
                // start the data xfer
                app_smartvoice_start_xfer(); 
            }
        }
        else if ((APP_SMARTVOICE_PATH_TYPE_HFP == voicePathType) &&
            !app_is_hfp_service_connected())
        {
            app_sv_send_response_to_command(funcCode, CMD_HANDLING_FAILED, (uint8_t *)&rsp, sizeof(rsp), smartvoice_env.pathType);           
        }
        else
        {
            smartvoice_env.isMicStreamOpened = true;
            app_sv_send_response_to_command(funcCode, NO_ERROR, (uint8_t *)&rsp, sizeof(rsp), smartvoice_env.pathType);
        }
    }
    else 
    {
        app_sv_send_response_to_command(funcCode, DATA_XFER_ALREADY_STARTED, (uint8_t *)&rsp, sizeof(rsp), smartvoice_env.pathType);
    }
}

void app_sv_start_voice_stream_via_ble(void)
{
    TRACE("Start Smart Voice Stream Via BLE.");
    if (app_voicepath_check_hfp_active())
    {
        return;
    }
    
    if (APP_SMARTVOICE_BLE_CONNECTED == smartvoice_env.connType)
    {
        if (!smartvoice_env.isMicStreamOpened)
        {
            START_VOICE_STREAM_REQ_T req;
            req.voiceStreamPathToUse = APP_SMARTVOICE_PATH_TYPE_BLE;
            app_sv_send_command(OP_START_SMART_VOICE_STREAM, (uint8_t *)&req, sizeof(req), smartvoice_env.pathType);
        }
    }
}

void app_sv_start_voice_stream_via_spp(void)
{
    TRACE("Start Smart Voice Stream Via SPP.");
    if (app_voicepath_check_hfp_active())
    {
        return;
    }
    
    if (APP_SMARTVOICE_SPP_CONNECTED == smartvoice_env.connType)
    {
        if (!smartvoice_env.isMicStreamOpened)
        {
            START_VOICE_STREAM_REQ_T req;
            req.voiceStreamPathToUse = APP_SMARTVOICE_PATH_TYPE_SPP;
            app_sv_send_command(OP_START_SMART_VOICE_STREAM, (uint8_t *)&req, sizeof(req), smartvoice_env.pathType);
        }
    }
}

void app_sv_start_voice_stream_via_hfp(void)
{
    if (smartvoice_env.connType > 0)
    {
        TRACE("Start Smart Voice Stream Via HFP.");
        app_hfp_enable_audio_link(true);
        START_VOICE_STREAM_REQ_T req;
        req.voiceStreamPathToUse = APP_SMARTVOICE_PATH_TYPE_HFP;
        app_sv_send_command(OP_START_SMART_VOICE_STREAM, (uint8_t *)&req, sizeof(req), smartvoice_env.pathType);
    }
}

static void app_sv_start_voice_stream_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE("Received the response %d to the start voice stream request.", retStatus);
    if (NO_ERROR == retStatus)
    {
        START_VOICE_STREAM_RSP_T* rsp = (START_VOICE_STREAM_RSP_T *)ptrParam;
        if ((APP_SMARTVOICE_PATH_TYPE_BLE == rsp->voiceStreamPathToUse) ||
            (APP_SMARTVOICE_PATH_TYPE_SPP == rsp->voiceStreamPathToUse))
        {
            if (!smartvoice_env.isMicStreamOpened)
            {                
                app_smartvoice_start_stream();
                // start the data xfer
                app_smartvoice_start_xfer();
            }else{
				app_smartvoice_start_xfer();				
			}
        }
    } 
    else
    {
        // workaround, should be changed to false
        app_hfp_enable_audio_link(false);
    }

    smartvoice_env.isMicStreamOpened = true;
}

void app_voicepath_bt_link_disconnected_handler(uint8_t* pBtAddr)
{
	app_voicepath_disconnected(APP_SMARTVOICE_SPP_DISCONNECTED);
}

void app_smartvoice_config_mtu(uint16_t MTU)
{
    TRACE("The SV negotiated MTU is %d", MTU);
    smartvoice_env.smartVoiceDataSectorSize = (MTU - 3) - SMARTVOICE_CMD_CODE_SIZE - SMARTVOICE_ENCODED_DATA_SEQN_SIZE;
}

bool app_voicepath_is_btaddr_connected(uint8_t* btAddr)
{
	return false;
}

void app_voicepath_bt_init(void)
{

}

void app_voicepath_bt_cleanup(void)
{

}

void app_voicepath_bt_link_connected_handler(uint8_t* pBtAddr)
{
	
}

void app_voicepath_disconnect_bt(void)
{

}

void app_voicepath_disconnect_ble(void)
{

}

void app_voicepath_custom_init(void)
{
    TRACE("%s", __func__);

	vob_env_mutex_id = osMutexCreate((osMutex(vob_env_mutex)));

    memset(&smartvoice_audio_stream,    0x00, sizeof(smartvoice_audio_stream));

    memset(&smartvoice_env,         0x00, sizeof(smartvoice_env));

    memset(&smartvoice_audio_stream,     0x00, sizeof(smartvoice_audio_stream));

#if IS_FORCE_STOP_VOICE_STREAM
    app_mic_timeout_timer_id = osTimerCreate(osTimer(APP_SMART_VOICE_MIC_TIMEOUT), osTimerOnce, NULL); 
#endif

    app_sv_cmd_handler_init();
    app_sv_data_reset_env();

#if APP_SMARTVOICE_PATH_TYPE&APP_SMARTVOICE_PATH_TYPE_BLE
    app_sv_register_tx_done(app_smartvoice_send_done);
#endif

#if APP_SMARTVOICE_PATH_TYPE&APP_SMARTVOICE_PATH_TYPE_SPP
#ifdef UUID_128_ENABLE
    app_spp_extend_uuid_init();
#else
    app_spp_init();
#endif
#endif
    app_sv_throughput_test_init();
}

static void app_sv_stop_voice_stream_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE("Received the stop voice stream request from the remote dev.");
    if (smartvoice_env.isMicStreamOpened)
    {
		app_sv_send_response_to_command(funcCode, NO_ERROR, NULL, 0, smartvoice_env.pathType);
        smartvoice_env.isDataXferOnGoing = false;
        app_smartvoice_stop_stream();
        // Stop the data xfer
        app_smartvoice_stop_xfer(); 
    }
    else
    {
        app_sv_send_response_to_command(funcCode, DATA_XFER_NOT_STARTED_YET, NULL, 0, smartvoice_env.pathType);
    }
}

static void app_sv_stop_voice_stream_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE("Received the stop voice stream response %d from the remote dev.", retStatus);
    if ((NO_ERROR == retStatus) || (WAITING_RSP_TIMEOUT == retStatus))
    {
        app_smartvoice_stop_stream();
        app_smartvoice_stop_xfer(); 
    }

    smartvoice_env.isMicStreamOpened = false;
}

void app_sv_stop_voice_stream(void)
{
    if (smartvoice_env.connType > 0)
    {
        if (smartvoice_env.isMicStreamOpened)
        {
        #if IS_FORCE_STOP_VOICE_STREAM    
            if (NULL != app_mic_timeout_timer_id)
            {
                osTimerStop(app_mic_timeout_timer_id);
            }
        #endif
            app_sv_send_command(OP_STOP_SMART_VOICE_STREAM, NULL, 0, smartvoice_env.pathType);
        }
    }
}

void app_smartvoice_start_stream(void)
{
    if (app_voicepath_check_hfp_active())
    {
        return;
    }
    
    if (!smartvoice_env.isMicStreamOpened)
    {
        app_bt_stop_sniff(BT_DEVICE_ID_1);
        app_bt_stay_active(BT_DEVICE_ID_1);
        TRACE("Start smart voice stream.");
        smartvoice_env.isMicStreamOpened = true;

        start_capture_stream();
    }
}

void app_smartvoice_stop_stream(void)
{
    if (smartvoice_env.isMicStreamOpened)
    {
        app_bt_allow_sniff(BT_DEVICE_ID_1);
        TRACE("Stop smart voice stream.");
        smartvoice_env.isMicStreamOpened = false;
        ResetCqueue(&(smartvoice_audio_stream.encodedDataBufQueue));
        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_CAPTURE, 0, 0);
    }
}

uint32_t app_voice_catpure_interval_in_ms(void)
{
	return VOB_VOICE_CAPTURE_INTERVAL_IN_MS;
}

void app_smartvoice_switch_hfp_stream_status(void)
{
    if (smartvoice_env.isMicStreamOpened)
    {
        app_sv_stop_voice_stream();
    }
    else
    {
        app_sv_start_voice_stream_via_hfp();
    }
}

void app_smartvoice_switch_ble_stream_status(void)
{
    if (smartvoice_env.isMicStreamOpened)
    {
        app_sv_stop_voice_stream();
    }
    else
    {
        app_sv_start_voice_stream_via_ble();
        if (APP_SMARTVOICE_BLE_CONNECTED == smartvoice_env.connType){
        	app_smartvoice_start_stream();//open the stream
        }
    }
}

void app_smartvoice_switch_spp_stream_status(void)
{
    if (smartvoice_env.isMicStreamOpened)
    {
        app_sv_stop_voice_stream();
    }
    else
    {
        app_sv_start_voice_stream_via_spp();
    }
}

static void app_sv_cloud_music_playing_control_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE("Received the cloud music playing control request from the remote dev.");
}

static void app_sv_cloud_music_playing_control_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE("Music control returns %d", retStatus);
}

void app_sv_control_cloud_music_playing(CLOUD_MUSIC_CONTROL_REQ_E req)
{
    if (smartvoice_env.connType > 0)
    {
        if (smartvoice_env.isMicStreamOpened)
        {
            CLOUD_MUSIC_PLAYING_CONTROL_REQ_T request;
            request.cloudMusicPlayingControlReq = (uint8_t)req;
            app_sv_send_command(OP_CLOUD_MUSIC_PLAYING_CONTROL, (uint8_t *)&request, sizeof(request), smartvoice_env.pathType);
        }
    }
}

static void app_sv_choose_cloud_platform(CLOUD_PLATFORM_E req)
{
    CLOUD_PLATFORM_CHOOSING_REQ_T request;
    request.cloudPlatform = req;
    app_sv_send_command(OP_CHOOSE_CLOUD_PLATFORM, (uint8_t *)&request, sizeof(request), smartvoice_env.pathType);
}

void app_sv_switch_cloud_platform(void)
{
    if (CLOUD_PLATFORM_0 == smartvoice_env.currentPlatform)
    {
        smartvoice_env.currentPlatform = CLOUD_PLATFORM_1;
    }
    else
    {
        smartvoice_env.currentPlatform = CLOUD_PLATFORM_0;
    }
    app_sv_choose_cloud_platform(smartvoice_env.currentPlatform);
}

static void app_sv_choose_cloud_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    
}

static void app_sv_spp_data_ack_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    SMARTVOICE_TRACE("%s", __func__);
    if (APP_SV_VIA_SPP == smartvoice_env.pathType)
    {
        smartvoice_env.isDataXferOnGoing = false;
#if 0
        if (LengthOfCQueue(&(smartvoice_audio_stream.encodedDataBufQueue)) > 0)
        {
            app_smartvoice_send_encoded_audio_data();
        }
#endif
    }
}

static void app_sv_spp_data_ack_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{

}

static void app_sv_inform_algorithm_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
#if VOB_ENCODING_ALGORITHM==ENCODING_ALGORITHM_SBC  
    VOICE_SBC_CONFIG_INFO_T tInfo = 
    {
        VOB_ENCODING_ALGORITHM,  
        VOB_VOICE_CAPTURE_FRAME_CNT,
        VOB_VOICE_ENCODED_DATA_FRAME_SIZE,
        VOB_VOICE_PCM_FRAME_SIZE,
        
        VOICE_SBC_BLOCK_SIZE,
        VOICE_SBC_CHANNEL_COUNT     ,
        VOICE_SBC_CHANNEL_MODE 	    ,
        VOICE_SBC_BIT_POOL	 	    ,
        VOICE_SBC_SIZE_PER_SAMPLE   ,
        VOICE_SBC_SAMPLE_RATE	    ,
        VOICE_SBC_NUM_BLOCKS	    ,
        VOICE_SBC_NUM_SUB_BANDS	    ,
        VOICE_SBC_MSBC_FLAG		    ,
        VOICE_SBC_ALLOC_METHOD	    ,
    };
#else
    VOICE_OPUS_CONFIG_INFO_T tInfo = 
    {
        VOB_ENCODING_ALGORITHM,        
        VOB_VOICE_ENCODED_DATA_FRAME_SIZE,
        VOB_VOICE_PCM_FRAME_SIZE,
        VOB_VOICE_CAPTURE_FRAME_CNT,

        VOICE_OPUS_CHANNEL_COUNT,
        VOICE_OPUS_COMPLEXITY,
        VOICE_OPUS_PACKET_LOSS_PERC,
        VOICE_SIZE_PER_SAMPLE,
        VOICE_OPUS_APP,
        VOICE_OPUS_BANDWIDTH,
        VOICE_OPUS_BITRATE,
        VOICE_OPUS_SAMPLE_RATE,
        VOICE_SIGNAL_TYPE,
        VOICE_FRAME_PERIOD,
        
        VOICE_OPUS_USE_VBR,
        VOICE_OPUS_CONSTRAINT_USE_VBR,
        VOICE_OPUS_USE_INBANDFEC,
        VOICE_OPUS_USE_DTX,
    };
#endif

    TRACE("Inform the information of the used algorithm.");

    uint32_t sentBytes = 0;
    uint8_t tmpBuf[sizeof(tInfo)+1];
    // the first byte will tell the offset of the info data, in case that 
    // payloadPerPacket is smaller than the info size and multiple packets have
    // to be sent
    uint32_t payloadPerPacket = smartvoice_env.smartVoiceDataSectorSize - 1;
    while ((sizeof(tInfo) - sentBytes) >= payloadPerPacket)
    {
        tmpBuf[0] = sentBytes;
        memcpy(&tmpBuf[1], ((uint8_t *)&tInfo) + sentBytes, payloadPerPacket);
        app_sv_send_response_to_command(funcCode, NO_ERROR, 
            tmpBuf, payloadPerPacket + 1, smartvoice_env.pathType);  

        sentBytes += payloadPerPacket;
    }

    if (sentBytes < sizeof(tInfo))
    {
        tmpBuf[0] = sentBytes;
        memcpy(&tmpBuf[1], ((uint8_t *)&tInfo) + sentBytes, sizeof(tInfo)-sentBytes);
        app_sv_send_response_to_command(funcCode, NO_ERROR, 
            tmpBuf, sizeof(tInfo) - sentBytes + 1, smartvoice_env.pathType);  
    }
}

#if !DEBUG_BLE_DATAPATH
CUSTOM_COMMAND_TO_ADD(OP_CLOUD_MUSIC_PLAYING_CONTROL, app_sv_cloud_music_playing_control_handler, TRUE,  
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_cloud_music_playing_control_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_START_SMART_VOICE_STREAM, app_sv_start_voice_stream_handler, TRUE,  
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_start_voice_stream_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_STOP_SMART_VOICE_STREAM, app_sv_stop_voice_stream_handler, TRUE,  
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_stop_voice_stream_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_SPP_DATA_ACK, app_sv_spp_data_ack_handler, FALSE,      
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_spp_data_ack_rsp_handler);

#else
CUSTOM_COMMAND_TO_ADD(OP_CLOUD_MUSIC_PLAYING_CONTROL, app_sv_cloud_music_playing_control_handler, FALSE,  
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_cloud_music_playing_control_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_START_SMART_VOICE_STREAM, app_sv_start_voice_stream_handler, FALSE,  
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_start_voice_stream_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_STOP_SMART_VOICE_STREAM, app_sv_stop_voice_stream_handler, FALSE,  
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_stop_voice_stream_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_SPP_DATA_ACK, app_sv_spp_data_ack_handler, FALSE,      
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_spp_data_ack_rsp_handler);

#endif

CUSTOM_COMMAND_TO_ADD(OP_INFORM_ALGORITHM_TYPE, app_sv_inform_algorithm_handler, FALSE,      
    0,    NULL);


CUSTOM_COMMAND_TO_ADD(OP_CHOOSE_CLOUD_PLATFORM, app_sv_choose_cloud_handler, FALSE,  
    0,    NULL );

void app_voicepath_key(APP_KEY_STATUS *status, void *param)
{
    
}

typedef enum
{
    PATTERN_RANDOM = 0,
    PATTERN_11110000,
    PATTERN_10101010,
    PATTERN_ALL_1,
    PATTERN_ALL_0,
    PATTERN_00001111,
    PATTERN_0101,
} THROUGHPUT_TEST_DATA_PATTER_E;

typedef enum
{
    UP_STREAM = 0,
    DOWN_STREAM
} THROUGHPUT_TEST_DATA_DIRECTION_E;

typedef enum
{
    WITHOUT_RSP = 0,
    WITH_RSP
} THROUGHPUT_TEST_RESPONSE_MODEL_E;


typedef struct
{
    uint8_t     dataPattern;
    uint16_t    lastTimeInSecond;
    uint16_t    dataPacketSize;
    uint8_t     direction;
    uint8_t     responseModel;
    uint8_t     isToUseSpecificConnParameter;
    uint16_t    minConnIntervalInMs;
    uint16_t    maxConnIntervalInMs;
    uint8_t     reserve[4];
} __attribute__((__packed__)) THROUGHPUT_TEST_CONFIG_T;

static THROUGHPUT_TEST_CONFIG_T throughputTestConfig;

#define APP_SV_THROUGHPUT_PRE_CONFIG_PENDING_TIME_IN_MS    2000
static void app_sv_throughput_pre_config_pending_timer_cb(void const *n);
osTimerDef (APP_SV_THROUGHPUT_PRE_CONFIG_PENDING_TIMER, app_sv_throughput_pre_config_pending_timer_cb); 
osTimerId   app_sv_throughput_pre_config_pending_timer_id = NULL;

static void app_sv_throughput_test_data_xfer_lasting_timer_cb(void const *n);
osTimerDef (APP_SV_THROUGHPUT_TEST_DATA_XFER_LASTING_TIMER, app_sv_throughput_test_data_xfer_lasting_timer_cb); 
osTimerId   app_sv_throughput_test_data_xfer_lasting_timer_id = NULL;


static uint8_t app_sv_throughput_datapattern[MAXIMUM_DATA_PACKET_LEN];

static void app_sv_throughput_test_data_xfer_lasting_timer_cb(void const *n)
{
    app_sv_send_command(OP_THROUGHPUT_TEST_DONE, NULL, 0,
        smartvoice_env.pathType);

    app_sv_stop_throughput_test();
}

static void app_sv_throughput_pre_config_pending_timer_cb(void const *n)
{
    // inform the configuration
    app_sv_send_command(OP_INFORM_THROUGHPUT_TEST_CONFIG, 
        (uint8_t *)&throughputTestConfig, sizeof(throughputTestConfig), 
        smartvoice_env.pathType);

    if (UP_STREAM == throughputTestConfig.direction)
    {
        app_sv_throughput_test_transmission_handler();
        osTimerStart(app_sv_throughput_test_data_xfer_lasting_timer_id, 
            throughputTestConfig.lastTimeInSecond*1000);
    }
}

void app_sv_throughput_test_init(void)
{
    app_sv_throughput_pre_config_pending_timer_id = 
        osTimerCreate(osTimer(APP_SV_THROUGHPUT_PRE_CONFIG_PENDING_TIMER), 
        osTimerOnce, NULL);

    app_sv_throughput_test_data_xfer_lasting_timer_id = 
        osTimerCreate(osTimer(APP_SV_THROUGHPUT_TEST_DATA_XFER_LASTING_TIMER), 
        osTimerOnce, NULL);
}

extern "C" void app_voicepath_ble_conn_parameter_updated(uint8_t conidx, uint16_t minConnIntervalInMs, uint16_t maxConnIntervalInMs)
{
    throughputTestConfig.minConnIntervalInMs = minConnIntervalInMs;
    throughputTestConfig.maxConnIntervalInMs = maxConnIntervalInMs;
}

void app_sv_throughput_test_transmission_handler(void)
{
    if (UP_STREAM == throughputTestConfig.direction)
    {
        app_sv_send_command(OP_THROUGHPUT_TEST_DATA, 
            app_sv_throughput_datapattern, throughputTestConfig.dataPacketSize - 4, 
            smartvoice_env.pathType);  
    }
}

static void app_sv_throughput_test_data_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    if ((WITH_RSP == throughputTestConfig.responseModel) && 
        (APP_SV_VIA_SPP == smartvoice_env.pathType))
    {
        app_sv_send_command(OP_THROUGHPUT_TEST_DATA_ACK, 
            NULL, 0, 
            smartvoice_env.pathType); 
    }
}

static void app_sv_throughput_test_data_ack_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    if (smartvoice_env.isThroughputTestOn &&
        (WITH_RSP == throughputTestConfig.responseModel))
    {
        app_sv_throughput_test_transmission_handler();
    }
}

static void app_sv_throughput_test_done_signal_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    app_sv_stop_throughput_test();
}

void app_sv_stop_throughput_test(void)
{
    smartvoice_env.isThroughputTestOn = false;
    osTimerStop(app_sv_throughput_pre_config_pending_timer_id);
    osTimerStop(app_sv_throughput_test_data_xfer_lasting_timer_id);
    app_spp_register_tx_done(NULL);
}

static void app_sv_throughput_test_config_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    throughputTestConfig = *(THROUGHPUT_TEST_CONFIG_T *)ptrParam;

    TRACE("data patter is %d", throughputTestConfig.dataPattern);

    // generate the data pattern
    switch (throughputTestConfig.dataPattern)
    {
        case PATTERN_RANDOM:
        {
            for (uint32_t index = 0;index < MAXIMUM_DATA_PACKET_LEN;index++)
            {
                app_sv_throughput_datapattern[index] = (uint8_t)rand();                
            }
            break;
        }
        case PATTERN_11110000:
        {
            memset(app_sv_throughput_datapattern, 0xF0, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_10101010:
        {
            memset(app_sv_throughput_datapattern, 0xAA, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_ALL_1:
        {
            memset(app_sv_throughput_datapattern, 0xFF, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_ALL_0:
        {
            memset(app_sv_throughput_datapattern, 0, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_00001111:
        {
            memset(app_sv_throughput_datapattern, 0x0F, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_0101:
        {
            memset(app_sv_throughput_datapattern, 0x55, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        default:
            throughputTestConfig.dataPattern = 0;
            break;        
    }

    smartvoice_env.isThroughputTestOn = true;

    if (WITHOUT_RSP == throughputTestConfig.responseModel)
    {
        app_spp_register_tx_done(app_sv_throughput_test_transmission_handler);
    }
    else
    {
        app_spp_register_tx_done(NULL);
    }
    
    // check whether need to use the new conn parameter
    if (((APP_SV_VIA_NOTIFICATION == smartvoice_env.pathType) ||
        (APP_SV_VIA_INDICATION == smartvoice_env.pathType))
        && throughputTestConfig.isToUseSpecificConnParameter)
    {
        if (WITHOUT_RSP == throughputTestConfig.responseModel)
        {
            smartvoice_env.pathType = APP_SV_VIA_NOTIFICATION;
        }
        else
        {
            smartvoice_env.pathType = APP_SV_VIA_INDICATION;
        }
        app_sv_update_conn_parameter(smartvoice_env.conidx, throughputTestConfig.minConnIntervalInMs,
            throughputTestConfig.maxConnIntervalInMs, 
            SV_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS,
            SV_BLE_CONNECTION_SLAVELATENCY);       
    
        osTimerStart(app_sv_throughput_pre_config_pending_timer_id, 
            APP_SV_THROUGHPUT_PRE_CONFIG_PENDING_TIME_IN_MS);
    }
    else
    {
        app_sv_send_command(OP_INFORM_THROUGHPUT_TEST_CONFIG, 
            (uint8_t *)&throughputTestConfig, sizeof(throughputTestConfig), 
            smartvoice_env.pathType);

        if (UP_STREAM == throughputTestConfig.direction)
        {
            app_sv_throughput_test_transmission_handler();
            osTimerStart(app_sv_throughput_test_data_xfer_lasting_timer_id, 
                throughputTestConfig.lastTimeInSecond*1000);
        }
    }
}

CUSTOM_COMMAND_TO_ADD(OP_INFORM_THROUGHPUT_TEST_CONFIG, app_sv_throughput_test_config_handler, FALSE,  
    0,    NULL );

CUSTOM_COMMAND_TO_ADD(OP_THROUGHPUT_TEST_DATA, app_sv_throughput_test_data_handler, FALSE,  
    0,    NULL );

CUSTOM_COMMAND_TO_ADD(OP_THROUGHPUT_TEST_DATA_ACK, app_sv_throughput_test_data_ack_handler, FALSE,  
    0,    NULL );

CUSTOM_COMMAND_TO_ADD(OP_THROUGHPUT_TEST_DONE, app_sv_throughput_test_done_signal_handler, FALSE,  
    0,    NULL );

