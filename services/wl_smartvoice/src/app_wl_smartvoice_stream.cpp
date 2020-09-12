#include "app_wl_smartvoice.h"

#include "cmsis_os.h"
#include "hal_trace.h"
#include "resources.h"
#include "app_bt_stream.h"
#include "app_media_player.h"
#include "app_bt_media_manager.h"
#include "app_factory.h"
#include "string.h"

// for audio
#include "audioflinger.h"
#include "app_audio.h"
#include "app_factory_audio.h"
#include "app_wl_smartvoice_stream.h"
#include "app_wl_smartvoice_channel.h"
#include "app_wl_smartvoice_algorithm.h"

#include "cqueue.h"
#include "app_overlay.h"

#ifdef AUDIO_DEBUG
#include "audio_dump.h"
#endif

#ifdef __APP_WL_SMARTVOICE_LOOP_TEST__

static enum APP_AUDIO_CACHE_T voice_cache_status = APP_AUDIO_CACHE_QTY;
static int16_t *loop_test_voice_cache = NULL;

#endif

#define APP_WL_SMARTVOICE_CACHE_COUNT   (1024*10)

typedef struct {
    CQueue  queue;
    osMutexId mutex;
    uint8_t buff[APP_WL_SMARTVOICE_CACHE_COUNT];
} app_wl_sv_cache_t;

osMutexDef(voice_cache_mutex);
static app_wl_sv_cache_t voice_cache;

static app_wl_sv_cache_t* app_wl_smartvoice_cache_instance_get(void)
{
    return &voice_cache;
}

static void app_wl_smartvoice_cache_init(app_wl_sv_cache_t* cache)
{
    InitCQueue(&cache->queue, APP_WL_SMARTVOICE_CACHE_COUNT, ( CQItemType *)cache->buff);
    cache->mutex = osMutexCreate((osMutex(voice_cache_mutex)));
}

POSSIBLY_UNUSED static int app_wl_smartvoice_cache_in(app_wl_sv_cache_t* cache, uint8_t* data, uint16_t len)
{
    int status;
    CQueue *Q;
    CQItemType *e;
    uint32_t bytesToTheEnd;

    Q = &cache->queue;
    e = ( CQItemType *)data;

    osMutexWait(cache->mutex, osWaitForever);

    if (AvailableOfCQueue(Q) < (int)len) {
        status = CQ_ERR;
        goto exit;
	}

	Q->len += len;

	bytesToTheEnd = Q->size - Q->write;
	if (bytesToTheEnd > len)
	{
		memcpy((uint8_t *)&Q->base[Q->write], (uint8_t *)e, len);
		Q->write += len;
	}
	else
	{
		memcpy((uint8_t *)&Q->base[Q->write], (uint8_t *)e, bytesToTheEnd);
		memcpy((uint8_t *)&Q->base[0], (((uint8_t *)e)+bytesToTheEnd), len-bytesToTheEnd);
		Q->write = len-bytesToTheEnd;
	}

    status = CQ_OK;
exit:
    osMutexRelease(cache->mutex);

    return status;
}

POSSIBLY_UNUSED static int app_wl_smartvoice_cache_out(app_wl_sv_cache_t* cache, uint8_t* buff, uint16_t len)
{
    int status;
    uint8_t *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;

    osMutexWait(cache->mutex, osWaitForever);

    status = PeekCQueue(&(cache->queue), len, &e1, &len1, &e2, &len2);
    if (len==(len1+len2)){
        memcpy(buff,e1,len1);
        memcpy(buff+len1,e2,len2);
        DeCQueue(&(cache->queue), 0, len1);
        DeCQueue(&(cache->queue), 0, len2);
    }else{
        memset(buff, 0x00, len);
        status = -1;
    }

    osMutexRelease(cache->mutex);
    return status;
}

static void app_wl_smartvoice_cache_reset(app_wl_sv_cache_t* cache)
{
    cache->queue.write=0;
    cache->queue.len=0;
    cache->queue.read=0;

    memset(cache->buff, 0x00, sizeof(cache->buff));
}

static int app_wl_smartvoice_cache_length(app_wl_sv_cache_t* cache)
{
    return cache->queue.len;
}

static void app_wl_smartvoice_sent_done(void)
{
    int status;
    uint8_t buff[APP_WL_SMARTVOICE_TRANSFER_FRAME_SIZE];

    WL_SV_DEBUG("cache len: %d", app_wl_smartvoice_cache_length(app_wl_smartvoice_cache_instance_get()));
    status = app_wl_smartvoice_cache_out(app_wl_smartvoice_cache_instance_get(), buff, sizeof(buff));

    if (0 == status) {
        app_wl_smartvoice_send_voice_data(buff, sizeof(buff));
    }
}

static uint32_t app_wl_smartvoice_capture_data(uint8_t *buf, uint32_t len)
{
    WL_SV_FUNC_ENTER();

    //WL_SV_DEBUG("capture len: %d", len);
    DUMP16("%d,",(short*)buf,30);
    
    // 先过一遍降噪算法
    //app_wl_smartvoice_algorithm_process(buf, len);

#ifdef __APP_WL_SMARTVOICE_OPUS_ENCODE__
    uint8_t out[APP_WL_SMARTVOICE_OPUS_FRAME_ENCODE_SIZE];
    int out_size,now;

    now = hal_sys_timer_get();


    out_size = wl_smartvoice_opus_encode(buf, out, len);
    if (APP_WL_SMARTVOICE_OPUS_FRAME_ENCODE_SIZE != out_size) {
        WL_SV_DEBUG("OPUS Encode Fail Ret: %d", out_size);
        memset(out, 0x00, APP_WL_SMARTVOICE_OPUS_FRAME_ENCODE_SIZE);
    }


#ifdef AUDIO_DEBUG
    static uint32_t dump_cnt = 0;

    dump_cnt++;

    #if 1
    if(dump_cnt > 0x1FF)
    {
        audio_dump_clear_up();

        // for(uint16_t iicnt = 0; iicnt < out_size; iicnt++)
        // {
        //     out[iicnt] = iicnt;
        // }

        audio_dump_add_channel_data(0, out, out_size>>1);
        //audio_dump_add_channel_data(0, two_buff, pcm_len>>1);	
        //audio_dump_add_channel_data(0, three_buff, pcm_len>>1);	

        audio_dump_run();

        dump_cnt = 0x4FF;
    }

    #else
    audio_dump_clear_up();

    audio_dump_add_channel_data(0, pcm_buff, pcm_len);
    //audio_dump_add_channel_data(0, two_buff, pcm_len>>1);	
    //audio_dump_add_channel_data(0, three_buff, pcm_len>>1);	

    audio_dump_run();
    #endif

#endif


    WL_SV_DEBUG("opus encode :%d ms encode_size:%d ", TICKS_TO_MS(hal_sys_timer_get() - now),out_size);

#endif

#ifdef __APP_WL_SMARTVOICE_LOOP_TEST__

#ifdef __APP_WL_SMARTVOICE_OPUS_ENCODE__
    app_audio_pcmbuff_put(out, APP_WL_SMARTVOICE_OPUS_FRAME_ENCODE_SIZE);
#else
    app_audio_pcmbuff_put(buf, len);
#endif
    if (voice_cache_status == APP_AUDIO_CACHE_QTY){
        voice_cache_status = APP_AUDIO_CACHE_OK;
    }
#else
    app_wl_smartvoice_cache_in(app_wl_smartvoice_cache_instance_get(), out, APP_WL_SMARTVOICE_OPUS_FRAME_ENCODE_SIZE);
    app_wl_smartvoice_sent_done();
#endif

    WL_SV_FUNC_EXIT();

    return len;
}

#ifdef __APP_WL_SMARTVOICE_LOOP_TEST__
static uint32_t app_wl_smartvoice_speaker_more_data(uint8_t *buf, uint32_t len)
{
    WL_SV_FUNC_ENTER();

    WL_SV_DEBUG("palyback len: %d", len);
    if (voice_cache_status != APP_AUDIO_CACHE_QTY){
#ifdef __APP_WL_SMARTVOICE_OPUS_ENCODE__
        static uint8_t pcm_buff[APP_WL_SMARTVOICE_FRAME_SIZE];
        int out_size;

        app_audio_pcmbuff_get((uint8_t *)loop_test_voice_cache, APP_WL_SMARTVOICE_OPUS_FRAME_ENCODE_SIZE);

        out_size = wl_smartvoice_opus_decode((uint8_t *)loop_test_voice_cache, APP_WL_SMARTVOICE_OPUS_FRAME_ENCODE_SIZE, pcm_buff, APP_WL_SMARTVOICE_FRAME_SIZE);
        if (APP_WL_SMARTVOICE_FRAME_SIZE/2 != out_size) {
            WL_SV_DEBUG("OPUS Decode Fail Ret: %d", out_size);
            memset(pcm_buff, 0x00, APP_WL_SMARTVOICE_FRAME_SIZE);
        }

        WL_SV_DEBUG("OPUS Decode Size: %d", out_size);
        app_bt_stream_copy_track_one_to_two_16bits((int16_t *)buf, (int16_t *)pcm_buff, len/2/2);
#else
        app_audio_pcmbuff_get((uint8_t *)loop_test_voice_cache, len/2);
        app_bt_stream_copy_track_one_to_two_16bits((int16_t *)buf, loop_test_voice_cache, len/2/2);
#endif
    }

    WL_SV_FUNC_EXIT();

    return len;
}
#endif

static bool b_wl_sv_on = false;

bool app_wl_smartvoice_is_gonging_on(void)
{
    return b_wl_sv_on;
}

int app_wl_smartvoice_player(bool on, enum APP_SYSFREQ_FREQ_T freq)
{
    static bool isRun =  false;
    struct AF_STREAM_CONFIG_T stream_cfg;
    uint8_t *buff_capture = NULL;
    uint8_t* nsx_heap;

    WL_SV_DEBUG("app_wl_smartvoice_player work:%d op:%d freq:%d", isRun, on, freq);

    if (isRun==on)
        return 0;

    if (on) {
        if (freq < APP_SYSFREQ_52M) {
            freq = APP_SYSFREQ_52M;
        }
        app_sysfreq_req(APP_SYSFREQ_USER_WL_SMARTVOICE, freq);
        app_audio_mempool_init();
        app_audio_mempool_get_buff(&buff_capture, TO_PINGPONG_BUFF_SIZE(APP_WL_SMARTVOICE_FRAME_SIZE));

#ifdef __APP_WL_SMARTVOICE_OPUS_ENCODE__
        uint8_t* opus_heap;
        app_audio_mempool_get_buff(&opus_heap, WL_SMARTVOICE_OPUS_CONFIG_HEAP_SIZE);
        wl_smartvoice_opus_init(opus_heap);
#endif


#ifdef AUDIO_DEBUG
        audio_dump_init(20, sizeof(short), 1);
#endif

        app_overlay_select(APP_OVERLAY_FM);
        app_audio_mempool_get_buff(&nsx_heap, APP_WL_SMARTVOICE_WEBRTC_NSX_BUFF_SIZE);
        app_wl_smartvoice_algorithm_init(nsx_heap);

        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits = APP_WL_SMARTVOICE_OPUS_BITS_NUM;
        stream_cfg.channel_num = APP_WL_SMARTVOICE_OPUS_CHANNEL_NUM;
        stream_cfg.sample_rate = APP_WL_SMARTVOICE_OPUS_SAMPLE_RATE;
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.vol = TGT_VOLUME_LEVEL_15;
        stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
        stream_cfg.handler = app_wl_smartvoice_capture_data;
        stream_cfg.data_size = TO_PINGPONG_BUFF_SIZE(APP_WL_SMARTVOICE_FRAME_SIZE);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(buff_capture);
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

#ifdef __APP_WL_SMARTVOICE_LOOP_TEST__
        uint8_t *buff_play = NULL;
        uint8_t *buff_loop = NULL;

        voice_cache_status = APP_AUDIO_CACHE_QTY;

        app_audio_mempool_get_buff(&buff_loop, APP_WL_SMARTVOICE_LOOP_TEST_BUFF_SIZE);
        app_audio_pcmbuff_init(buff_loop, APP_WL_SMARTVOICE_LOOP_TEST_BUFF_SIZE);
        app_audio_mempool_get_buff((uint8_t **)&loop_test_voice_cache, APP_WL_SMARTVOICE_FRAME_SIZE);

        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.handler = app_wl_smartvoice_speaker_more_data;

        app_audio_mempool_get_buff(&buff_play, TO_PINGPONG_BUFF_SIZE(APP_WL_SMARTVOICE_FRAME_SIZE)*stream_cfg.channel_num);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(buff_play);
        stream_cfg.data_size = TO_PINGPONG_BUFF_SIZE(APP_WL_SMARTVOICE_FRAME_SIZE)*stream_cfg.channel_num;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif

        b_wl_sv_on = true;
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

        WL_SV_DEBUG("app_wl_smartvoice_player on");
    } else {
        b_wl_sv_on = false;

#ifdef __APP_WL_SMARTVOICE_OPUS_ENCODE__
        wl_smartvoice_opus_deinit();
#endif

        app_wl_smartvoice_algorithm_deinit();

        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
#ifdef __APP_WL_SMARTVOICE_LOOP_TEST__
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        WL_SV_DEBUG("app_wl_smartvoice_player off");

        app_wl_smartvoice_cache_reset(app_wl_smartvoice_cache_instance_get());

        app_sysfreq_req(APP_SYSFREQ_USER_WL_SMARTVOICE, APP_SYSFREQ_32K);
    }

    isRun=on;
    return 0;
}

static void app_wl_smartvoice_start_stream_execute(void)
{
    WL_SV_FUNC_ENTER();
    if (b_wl_sv_on) {
        return;
    }

    WL_SV_DEBUG("Recording......");

    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START, BT_STREAM_WL_SMARTVOICE, 0, 0);
    WL_SV_FUNC_EXIT();
}

osAccurateTimerDef(delay_start_wl_stream_timer, delay_start_wl_stream_timer_handler)
static void delay_start_wl_stream_timer_handler(void)
{
    app_wl_smartvoice_start_stream_execute();
}

void app_wl_smartvoice_start_stream(void)
{
    osAccurateTimerStart(delay_start_wl_stream_timer, 1000);
}

void app_wl_smartvoice_stop_stream(void)
{
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_WL_SMARTVOICE, 0, 0);
}

void app_wl_smartvoice_stream_init(void)
{
    memset(&voice_cache, 0x00, sizeof(app_wl_sv_cache_t));
    app_wl_smartvoice_cache_init(app_wl_smartvoice_cache_instance_get());

    app_wl_smartvoice_channel_data_tx_done_subscribe(app_wl_smartvoice_sent_done);
}