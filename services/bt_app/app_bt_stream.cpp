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
//#include "mbed.h"
#include <stdio.h>
#include <assert.h>

#include "cmsis_os.h"
#include "tgt_hardware.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_overlay.h"
#include "analog.h"
#include "app_bt_stream.h"
#include "app_overlay.h"
#include "app_audio.h"
#include "app_utils.h"
#ifdef ANC_APP
#include "app_anc.h"
#endif
#include "bluetooth.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "nvrecord_dev.h"
#include "resample_coef.h"
#include "hal_codec.h"
#ifdef MEDIA_PLAYER_SUPPORT
#include "resources.h"
#include "app_media_player.h"
#endif
#ifdef __FACTORY_MODE_SUPPORT__
#include "app_factory_audio.h"
#endif
#ifdef TX_RX_PCM_MASK
#include "hal_chipid.h"
#endif
#ifdef VOICE_DATAPATH
#include "app_voicepath.h"
#endif

#ifdef __APP_WL_SMARTVOICE__
#include "app_wl_smartvoice.h"
#endif

#include "app_ring_merge.h"

#include "bt_drv.h"
#include "bt_xtal_sync.h"
#include "bt_drv_reg_op.h"
#include "besbt.h"

#include "cqueue.h"
#include "btapp.h"
#include "app_hfp.h"
#include "app_bt.h"
#include "os_api.h"
#include "audio_process.h"
#if (A2DP_DECODER_VER == 2)
#include "a2dp_decoder.h"
#endif
#if defined(__AUDIO_SPECTRUM__)
#include "audio_spectrum.h"
#endif

#ifdef __AI_VOICE__
#include "app_ai_voice.h"
#include "ai_manager.h"
#include "ai_thread.h"
#endif

#if defined(IBRT)
#include "app_ibrt_if.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_audio_analysis.h"
#include "app_tws_ibrt_audio_sync.h"
#undef MUSIC_DELAY_CONTROL
#endif

#if defined(__SW_IIR_EQ_PROCESS__)
static uint8_t audio_eq_sw_iir_index = 0;
extern const IIR_CFG_T * const audio_eq_sw_iir_cfg_list[];
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
static uint8_t audio_eq_hw_fir_index = 0;
extern const FIR_CFG_T * const audio_eq_hw_fir_cfg_list[];
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
static uint8_t audio_eq_hw_dac_iir_index = 0;
extern const IIR_CFG_T * const audio_eq_hw_dac_iir_cfg_list[];
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
static uint8_t audio_eq_hw_iir_index = 0;
extern const IIR_CFG_T * const audio_eq_hw_iir_cfg_list[];
#endif

#if defined(HW_DC_FILTER_WITH_IIR)
#include "hw_filter_codec_iir.h"
#include "hw_codec_iir_process.h"

hw_filter_codec_iir_cfg POSSIBLY_UNUSED adc_iir_cfg = {
    .bypass = 0,
    .iir_device = HW_CODEC_IIR_ADC,
#if 1
    .iir_cfg = {
        .iir_filtes_l = {
            .iir_bypass_flag = 0,
            .iir_counter = 2,
            .iir_coef = {
                    {{0.994406, -1.988812, 0.994406}, {1.000000, -1.988781, 0.988843}}, // iir_designer('highpass', 0, 20, 0.7, 16000);
                    {{4.0, 0.0, 0.0}, {1.0, 0.0, 0.0}},
            }
        },
        .iir_filtes_r = {
            .iir_bypass_flag = 0,
            .iir_counter = 2,
            .iir_coef = {
                    {{0.994406, -1.988812, 0.994406}, {1.000000, -1.988781, 0.988843}},
                    {{4.0, 0.0, 0.0}, {1.0, 0.0, 0.0}},
            }
        }
    }
#else
    .iir_cfg = {
        .gain0 = 0,
        .gain1 = 0,
        .num = 1,
        .param = {
            {IIR_TYPE_HIGH_PASS, 0,   20.0,   0.7},
        }
    }
#endif
};

hw_filter_codec_iir_state *hw_filter_codec_iir_st;
#endif

#include "audio_cfg.h"

extern uint8_t bt_audio_get_eq_index(AUDIO_EQ_TYPE_T audio_eq_type,uint8_t anc_status);
extern uint32_t bt_audio_set_eq(AUDIO_EQ_TYPE_T audio_eq_type,uint8_t index);
extern uint8_t bt_audio_updata_eq_for_anc(uint8_t anc_status);

#include "app_bt_media_manager.h"

#include "string.h"
#include "hal_location.h"

#include "bt_drv_interface.h"

#include "audio_resample_ex.h"

#if defined(CHIP_BEST1400)
#define BT_INIT_XTAL_SYNC_FCAP_RANGE (0x1FF)
#else
#define BT_INIT_XTAL_SYNC_FCAP_RANGE (0xFF)
#endif
#define BT_INIT_XTAL_SYNC_MIN (20)
#define BT_INIT_XTAL_SYNC_MAX (BT_INIT_XTAL_SYNC_FCAP_RANGE - BT_INIT_XTAL_SYNC_MIN)

#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
#include"anc_process.h"

#define DELAY_SAMPLE_MC (36*2)     //  2:ch
static int32_t delay_buf_bt[DELAY_SAMPLE_MC];
#endif

#if defined(SCO_DMA_SNAPSHOT)
#define MASTER_MOBILE_BTCLK_OFFSET (3)
#define MASTER_MOBILE_BTCNT_OFFSET (MASTER_MOBILE_BTCLK_OFFSET*625)

extern void  app_tws_ibrt_audio_mobile_clkcnt_get(uint32_t btclk, uint16_t btcnt,
                                                     uint32_t *mobile_master_clk, uint16_t *mobile_master_cnt);

static uint8_t *playback_buf_codecpcm;
static uint32_t playback_size_codecpcm;
static uint8_t *capture_buf_codecpcm;
static uint32_t capture_size_codecpcm;

static uint8_t *playback_buf_btpcm;
static uint32_t playback_size_btpcm;
static uint8_t *capture_buf_btpcm;
static uint32_t capture_size_btpcm;

static enum AUD_SAMPRATE_T playback_samplerate_codecpcm;
static int32_t mobile_master_clk_offset_init;   
#endif

enum PLAYER_OPER_T
{
    PLAYER_OPER_START,
    PLAYER_OPER_STOP,
    PLAYER_OPER_RESTART,
};

#if (AUDIO_OUTPUT_VOLUME_DEFAULT < 1) || (AUDIO_OUTPUT_VOLUME_DEFAULT > 17)
#error "AUDIO_OUTPUT_VOLUME_DEFAULT out of range"
#endif
int8_t stream_local_volume = (AUDIO_OUTPUT_VOLUME_DEFAULT);
#ifdef AUDIO_LINEIN
int8_t stream_linein_volume = (AUDIO_OUTPUT_VOLUME_DEFAULT);
#endif

struct btdevice_volume *btdevice_volume_p;
#ifdef __BT_ANC__
uint8_t bt_sco_samplerate_ratio = 0;
static uint8_t *bt_anc_sco_dec_buf;
extern void us_fir_init(void);
extern uint32_t voicebtpcm_pcm_resample (short* src_samp_buf, uint32_t src_smpl_cnt, short* dst_samp_buf);
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
static enum AUD_BITS_T sample_size_play_bt;
static enum AUD_SAMPRATE_T sample_rate_play_bt;
static uint32_t data_size_play_bt;

static uint8_t *playback_buf_bt;
static uint32_t playback_size_bt;
static int32_t playback_samplerate_ratio_bt;

static uint8_t *playback_buf_mc;
static uint32_t playback_size_mc;
static enum AUD_CHANNEL_NUM_T  playback_ch_num_bt;
#endif

#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P))
static enum AUD_BITS_T lowdelay_sample_size_play_bt;
static enum AUD_SAMPRATE_T lowdelay_sample_rate_play_bt;
static uint32_t lowdelay_data_size_play_bt;
static enum AUD_CHANNEL_NUM_T  lowdelay_playback_ch_num_bt;
#endif

extern "C" uint8_t is_sbc_mode (void);
uint8_t bt_sbc_mode;
extern "C" uint8_t __attribute__((section(".fast_text_sram")))  is_sbc_mode(void)
{
    return bt_sbc_mode;
}

extern "C" uint8_t is_sco_mode (void);

uint8_t bt_sco_mode;
extern "C"   uint8_t __attribute__((section(".fast_text_sram")))  is_sco_mode(void)
{
    return bt_sco_mode;
}

#ifdef A2DP_LHDC_ON
extern struct BT_DEVICE_T  app_bt_device;
#ifdef A2DP_CP_ACCEL
#define LHDC_AUDIO_BUFF_SIZE   (512*2*4)
#else
#define LHDC_AUDIO_BUFF_SIZE   (512*2*10)
#endif
#endif
uint16_t gStreamplayer = APP_BT_STREAM_INVALID;

uint32_t a2dp_audio_more_data(uint8_t codec_type, uint8_t *buf, uint32_t len);
int a2dp_audio_init(void);
int a2dp_audio_deinit(void);
enum AUD_SAMPRATE_T a2dp_sample_rate = AUD_SAMPRATE_48000;
uint32_t a2dp_data_buf_size;
#ifdef RB_CODEC
extern int app_rbplay_audio_onoff(bool onoff, uint16_t aud_id);
#endif

#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
int app_a2dp_source_linein_on(bool on);
#endif
#if defined(APP_I2S_A2DP_SOURCE)
#include "app_status_ind.h"
#include "app_a2dp_source.h"
//player channel should <= capture channel number
//player must be 2 channel
#define LINEIN_PLAYER_CHANNEL (2)
#ifdef __AUDIO_INPUT_MONO_MODE__
#define LINEIN_CAPTURE_CHANNEL (1)
#else
#define LINEIN_CAPTURE_CHANNEL (2)
#endif

#if (LINEIN_CAPTURE_CHANNEL == 1)
#define LINEIN_PLAYER_BUFFER_SIZE (1024*LINEIN_PLAYER_CHANNEL)
#define LINEIN_CAPTURE_BUFFER_SIZE (LINEIN_PLAYER_BUFFER_SIZE/2)
#elif (LINEIN_CAPTURE_CHANNEL == 2)
#define LINEIN_PLAYER_BUFFER_SIZE (1024*LINEIN_PLAYER_CHANNEL)
//#define LINEIN_CAPTURE_BUFFER_SIZE (LINEIN_PLAYER_BUFFER_SIZE)
#define LINEIN_CAPTURE_BUFFER_SIZE (1024*10)
#endif

static int16_t *app_linein_play_cache = NULL;

int8_t app_linein_buffer_is_empty(void)
{
    if (app_audio_pcmbuff_length())
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

uint32_t app_linein_pcm_come(uint8_t * pcm_buf, uint32_t len)
{
    //DUMP16("%d ", pcm_buf, 10);
    DUMP8("0x%02x ", pcm_buf, 10);
    TRACE("app_linein_pcm_come");
    app_audio_pcmbuff_put(pcm_buf, len);

    return len;
}

uint32_t app_linein_need_pcm_data(uint8_t* pcm_buf, uint32_t len)
{

#if (LINEIN_CAPTURE_CHANNEL == 1)
    app_audio_pcmbuff_get((uint8_t *)app_linein_play_cache, len/2);
    //app_play_audio_lineinmode_more_data((uint8_t *)app_linein_play_cache,len/2);
    app_bt_stream_copy_track_one_to_two_16bits((int16_t *)pcm_buf, app_linein_play_cache, len/2/2);
#elif (LINEIN_CAPTURE_CHANNEL == 2)
    app_audio_pcmbuff_get((uint8_t *)pcm_buf, len);
    //app_play_audio_lineinmode_more_data((uint8_t *)pcm_buf, len);
#endif

#if defined(__AUDIO_OUTPUT_MONO_MODE__)
    merge_stereo_to_mono_16bits((int16_t *)buf, (int16_t *)pcm_buf, len/2);
#endif

#ifdef ANC_APP
    bt_audio_updata_eq_for_anc(app_anc_work_status());
#endif

    audio_process_run(pcm_buf, len);

    return len;
}
extern "C" void pmu_linein_onoff(unsigned char en);
extern "C" int hal_analogif_reg_read(unsigned short reg, unsigned short *val);
int app_a2dp_source_I2S_onoff(bool onoff)
{
    static bool isRun =  false;
    uint8_t *linein_audio_cap_buff = 0;
    uint8_t *linein_audio_play_buff = 0;
    uint8_t *linein_audio_loop_buf = NULL;
    struct AF_STREAM_CONFIG_T stream_cfg;

    TRACE("app_a2dp_source_I2S_onoff work:%d op:%d", isRun, onoff);

    if (isRun == onoff)
        return 0;

    if (onoff)
    {
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);
        app_overlay_select(APP_OVERLAY_A2DP);
        app_audio_mempool_init();
        app_audio_mempool_get_buff(&linein_audio_cap_buff, LINEIN_CAPTURE_BUFFER_SIZE);
//        app_audio_mempool_get_buff(&linein_audio_play_buff, LINEIN_PLAYER_BUFFER_SIZE);
//        app_audio_mempool_get_buff(&linein_audio_loop_buf, LINEIN_PLAYER_BUFFER_SIZE<<2);
//        app_audio_pcmbuff_init(linein_audio_loop_buf, LINEIN_PLAYER_BUFFER_SIZE<<2);

#if (LINEIN_CAPTURE_CHANNEL == 1)
        app_audio_mempool_get_buff((uint8_t **)&app_linein_play_cache, LINEIN_PLAYER_BUFFER_SIZE/2/2);
        //app_play_audio_lineinmode_init(LINEIN_CAPTURE_CHANNEL, LINEIN_PLAYER_BUFFER_SIZE/2/2);
#elif (LINEIN_CAPTURE_CHANNEL == 2)
        //app_play_audio_lineinmode_init(LINEIN_CAPTURE_CHANNEL, LINEIN_PLAYER_BUFFER_SIZE/2);
#endif

        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)LINEIN_PLAYER_CHANNEL;
        stream_cfg.sample_rate = AUD_SAMPRATE_44100;

#if 0
#if FPGA==0
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#endif

        stream_cfg.vol = 10;//stream_linein_volume;
        //TRACE("vol = %d",stream_linein_volume);
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.handler = app_linein_need_pcm_data;
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(linein_audio_play_buff);
        stream_cfg.data_size = LINEIN_PLAYER_BUFFER_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif

#if 1
        stream_cfg.device = AUD_STREAM_USE_I2S_SLAVE;
//     stream_cfg.io_path = AUD_INPUT_PATH_LINEIN;
//      stream_cfg.handler = app_linein_pcm_come;
        stream_cfg.handler = a2dp_source_linein_more_pcm_data;
//      stream_cfg.handler = app_linein_pcm_come;
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(linein_audio_cap_buff);
        stream_cfg.data_size = LINEIN_CAPTURE_BUFFER_SIZE;//2k

//        pmu_linein_onoff(1);
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

        audio_process_init();
        audio_process_open(stream_cfg.sample_rate, stream_cfg.bits, stream_cfg.channel_num, NULL, 0);

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
#endif
        //app_status_indication_set(APP_STATUS_INDICATION_LINEIN_ON);
    }
    else
    {
//       clear buffer data
        a2dp_source.pcm_queue.write=0;
        a2dp_source.pcm_queue.len=0;
        a2dp_source.pcm_queue.read=0;
//       pmu_linein_onoff(0);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        //app_status_indication_set(APP_STATUS_INDICATION_LINEIN_OFF);
        app_overlay_unloadall();
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    }

    isRun = onoff;
    TRACE("%s end!\n", __func__);
    return 0;
}
#endif


enum AUD_SAMPRATE_T bt_parse_sbc_sample_rate(uint8_t sbc_samp_rate)
{
    enum AUD_SAMPRATE_T sample_rate;
    sbc_samp_rate = sbc_samp_rate & A2D_STREAM_SAMP_FREQ_MSK;

    switch (sbc_samp_rate)
    {
        case A2D_SBC_IE_SAMP_FREQ_16:
//            sample_rate = AUD_SAMPRATE_16000;
//            break;
        case A2D_SBC_IE_SAMP_FREQ_32:
//            sample_rate = AUD_SAMPRATE_32000;
//            break;
        case A2D_SBC_IE_SAMP_FREQ_48:
            sample_rate = AUD_SAMPRATE_48000;
            break;
        case A2D_SBC_IE_SAMP_FREQ_44:
            sample_rate = AUD_SAMPRATE_44100;
            break;
#if defined(A2DP_LHDC_ON) ||defined(A2DP_SCALABLE_ON)
        case A2D_SBC_IE_SAMP_FREQ_96:
            sample_rate = AUD_SAMPRATE_96000;
            break;
#endif
#if defined(A2DP_LDAC_ON)
        case A2D_SBC_IE_SAMP_FREQ_96:
            sample_rate = AUD_SAMPRATE_96000;
            break;
#endif

        default:
            ASSERT(0, "[%s] 0x%x is invalid", __func__, sbc_samp_rate);
            break;
    }
    return sample_rate;
}

void bt_store_sbc_sample_rate(enum AUD_SAMPRATE_T sample_rate)
{
    a2dp_sample_rate = sample_rate;
}

enum AUD_SAMPRATE_T bt_get_sbc_sample_rate(void)
{
    return a2dp_sample_rate;
}

enum AUD_SAMPRATE_T bt_parse_store_sbc_sample_rate(uint8_t sbc_samp_rate)
{
    enum AUD_SAMPRATE_T sample_rate;

    sample_rate = bt_parse_sbc_sample_rate(sbc_samp_rate);
    bt_store_sbc_sample_rate(sample_rate);

    return sample_rate;
}

int bt_sbc_player_setup(uint8_t freq)
{
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    static uint8_t sbc_samp_rate = 0xff;
    uint32_t ret;

    if (sbc_samp_rate == freq)
        return 0;

    switch (freq)
    {
        case A2D_SBC_IE_SAMP_FREQ_16:
        case A2D_SBC_IE_SAMP_FREQ_32:
        case A2D_SBC_IE_SAMP_FREQ_48:
            a2dp_sample_rate = AUD_SAMPRATE_48000;
            break;
#if defined(A2DP_LHDC_ON) ||defined(A2DP_SCALABLE_ON)
        case A2D_SBC_IE_SAMP_FREQ_96:
            a2dp_sample_rate = AUD_SAMPRATE_96000;
            TRACE("%s:Sample rate :%d", __func__, freq);
            break;
#endif
#ifdef  A2DP_LDAC_ON
        case A2D_SBC_IE_SAMP_FREQ_96:
            a2dp_sample_rate = AUD_SAMPRATE_96000;
            TRACE("%s:Sample rate :%d", __func__, freq);
            break;
#endif

        case A2D_SBC_IE_SAMP_FREQ_44:
            a2dp_sample_rate = AUD_SAMPRATE_44100;
            break;
        default:
            break;
    }

    ret = af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, true);
    if (ret == 0) {
        stream_cfg->sample_rate = a2dp_sample_rate;
        af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, stream_cfg);
    }

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
    ret = af_stream_get_cfg(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg, true);
    if (ret == 0) {
        stream_cfg->sample_rate = a2dp_sample_rate;
        sample_rate_play_bt=stream_cfg->sample_rate;
        af_stream_setup(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, stream_cfg);
        anc_mc_run_setup(hal_codec_anc_convert_rate(sample_rate_play_bt));
    }
#endif

    sbc_samp_rate = freq;

    return 0;
}

void merge_stereo_to_mono_16bits(int16_t *src_buf, int16_t *dst_buf,  uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; i+=2)
    {
        dst_buf[i] = (src_buf[i]>>1) + (src_buf[i+1]>>1);
        dst_buf[i+1] = dst_buf[i];
    }
}

void merge_stereo_to_mono_24bits(int32_t *src_buf, int32_t *dst_buf,  uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; i+=2)
    {
        dst_buf[i] = (src_buf[i]>>1) + (src_buf[i+1]>>1);
        dst_buf[i+1] = dst_buf[i];
    }
}

#ifdef __HEAR_THRU_PEAK_DET__
#include "peak_detector.h"
// Depend on codec_dac_vol
const float pkd_vol_multiple[18] = {0.281838, 0.000010, 0.005623, 0.007943, 0.011220, 0.015849, 0.022387, 0.031623, 0.044668, 0.063096, 0.089125, 0.125893, 0.177828, 0.251189, 0.354813, 0.501187, 0.707946, 1.000000};
int app_bt_stream_local_volume_get(void);
#endif
extern void a2dp_get_curStream_remDev(btif_remote_device_t   **           p_remDev);

#ifdef PLAYBACK_FORCE_48K
#ifndef RESAMPLE_ANY_SAMPLE_RATE
#error "need RESAMPLE_ANY_SAMPLE_RATE"
#endif
static struct APP_RESAMPLE_T *force48k_resample;
static int app_force48k_resample_iter(uint8_t *buf, uint32_t len);
struct APP_RESAMPLE_T *app_force48k_resample_any_open(enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
        float ratio_step);
int app_playback_resample_run(struct APP_RESAMPLE_T *resamp, uint8_t *buf, uint32_t len);
#endif

#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P))

#define BT_USPERCLK (625)
#define BT_MUTIUSPERSECOND (1000000/BT_USPERCLK)

#define CALIB_DEVIATION_MS (2)
#define CALIB_FACTOR_MAX_THRESHOLD (0.0001f)
#define CALIB_BT_CLOCK_FACTOR_STEP (0.0000005f)

#define CALIB_FACTOR_DELAY (0.001f)

//bt time
static int32_t  bt_old_clock_us=0;
static uint32_t bt_old_clock_mutius=0;
static int32_t  bt_old_offset_us=0;

static int32_t  bt_clock_us=0;
static uint32_t bt_clock_total_mutius=0;
static int32_t  bt_total_offset_us=0;

static int32_t bt_clock_ms=0;

//local time
static uint32_t local_total_samples=0;
static uint32_t local_total_frames=0;

//static uint32_t local_clock_us=0;
static int32_t local_clock_ms=0;

//bt and local time
static uint32_t bt_local_clock_s=0;

//calib time
static int32_t calib_total_delay=0;
static int32_t calib_flag=0;

//calib factor
static float   calib_factor_offset=0.0f;
static int32_t calib_factor_flag=0;
static volatile int calib_reset=1;
#endif

bool process_delay(int32_t delay_ms)
{
#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P))
    if (delay_ms == 0)return 0;

    TRACE("delay_ms:%d", delay_ms);

    if(calib_flag==0)
    {
        calib_total_delay=calib_total_delay+delay_ms;
        calib_flag=1;
        return 1;
    }
    else
    {
        return 0;
    }
#else
    return 0;
#endif
}

#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P))
void a2dp_clock_calib_process(uint32_t len)
{
    btif_remote_device_t   * p_a2dp_remDev=NULL;
    uint32_t smplcnt = 0;
    int32_t btoffset = 0;

    uint32_t btclk = 0;
    uint32_t btcnt = 0;
    uint32_t btofs = 0;
    btclk = *((volatile uint32_t*)0xd02201fc);
    btcnt = *((volatile uint32_t*)0xd02201f8);
    btcnt=0;

   // TRACE("bt_sbc_player_more_data btclk:%08x,btcnt:%08x\n", btclk, btcnt);


    a2dp_get_curStream_remDev(&p_a2dp_remDev);
    if(p_a2dp_remDev != NULL)
    {
        btofs = btdrv_rf_bitoffset_get( btif_me_get_remote_device_hci_handle(p_a2dp_remDev) -0x80);

        if(calib_reset==1)
        {
            calib_reset=0;

            bt_clock_total_mutius=0;

            bt_old_clock_us=btcnt;
            bt_old_clock_mutius=btclk;

            bt_total_offset_us=0;


            local_total_samples=0;
            local_total_frames=0;

            bt_local_clock_s=0;

            bt_old_offset_us=btofs;

            calib_factor_offset=0.0f;
            calib_factor_flag=0;

        }
        else
        {
            btoffset=btofs-bt_old_offset_us;

            if(btoffset<-BT_USPERCLK/3)
            {
                btoffset=btoffset+BT_USPERCLK;
            }
            else if(btoffset>BT_USPERCLK/3)
            {
                btoffset=btoffset-BT_USPERCLK;
            }

            bt_total_offset_us=bt_total_offset_us+btoffset;
            bt_old_offset_us=btofs;

            local_total_frames++;
            if(lowdelay_sample_size_play_bt==AUD_BITS_16)
            {
                smplcnt=len/(2*lowdelay_playback_ch_num_bt);
            }
            else
            {
                smplcnt=len/(4*lowdelay_playback_ch_num_bt);
            }

            local_total_samples=local_total_samples+smplcnt;

            bt_clock_us=btcnt-bt_old_clock_us-bt_total_offset_us;

            btoffset=btclk-bt_old_clock_mutius;
            if(btoffset<0)
            {
                btoffset=0;
            }
            bt_clock_total_mutius=bt_clock_total_mutius+btoffset;

            bt_old_clock_us=btcnt;
            bt_old_clock_mutius=btclk;

            if((bt_clock_total_mutius>BT_MUTIUSPERSECOND)&&(local_total_samples>lowdelay_sample_rate_play_bt))
            {
                bt_local_clock_s++;
                bt_clock_total_mutius=bt_clock_total_mutius-BT_MUTIUSPERSECOND;
                local_total_samples=local_total_samples-lowdelay_sample_rate_play_bt;
            }

            bt_clock_ms=(bt_clock_total_mutius*BT_USPERCLK/1000)+bt_clock_us/625;
            local_clock_ms=(local_total_samples*1000)/lowdelay_sample_rate_play_bt;

            local_clock_ms=local_clock_ms+calib_total_delay;

            //TRACE("A2DP bt_clock_ms:%8d,local_clock_ms:%8d,bt_total_offset_us:%8d\n",bt_clock_ms, local_clock_ms,bt_total_offset_us);

            if(bt_clock_ms>(local_clock_ms+CALIB_DEVIATION_MS))
            {
#ifdef  __AUDIO_RESAMPLE__
#ifdef SW_PLAYBACK_RESAMPLE
                app_resample_tune(a2dp_resample, CALIB_FACTOR_DELAY);
#else
                af_codec_tune_resample_rate(AUD_STREAM_PLAYBACK, CALIB_FACTOR_DELAY);
#endif
#else
                af_codec_tune_pll(CALIB_FACTOR_DELAY);
#endif 
                calib_factor_flag=1;
                //TRACE("*************1***************");
            }
            else if(bt_clock_ms<(local_clock_ms-CALIB_DEVIATION_MS))
            {
#ifdef  __AUDIO_RESAMPLE__
#ifdef SW_PLAYBACK_RESAMPLE
                app_resample_tune(a2dp_resample, -CALIB_FACTOR_DELAY);
#else
                af_codec_tune_resample_rate(AUD_STREAM_PLAYBACK, -CALIB_FACTOR_DELAY);
#endif
#else
                af_codec_tune_pll(-CALIB_FACTOR_DELAY);
#endif              
                calib_factor_flag=-1;
                //TRACE("*************-1***************");
            }
            else
            {
                if((calib_factor_flag==1||calib_factor_flag==-1)&&(bt_clock_ms==local_clock_ms))
                {
                    if(calib_factor_offset<CALIB_FACTOR_MAX_THRESHOLD&&calib_flag==0)
                    {
                        if(calib_factor_flag==1)
                        {
                            calib_factor_offset=calib_factor_offset+CALIB_BT_CLOCK_FACTOR_STEP;
                        }
                        else
                        {
                            calib_factor_offset=calib_factor_offset-CALIB_BT_CLOCK_FACTOR_STEP;
                        }

                    }
#ifdef  __AUDIO_RESAMPLE__
#ifdef SW_PLAYBACK_RESAMPLE
                    app_resample_tune(a2dp_resample, calib_factor_offset);
#else
                    af_codec_tune_resample_rate(AUD_STREAM_PLAYBACK, calib_factor_offset);
#endif
#else
                    af_codec_tune_pll(calib_factor_offset);
#endif 
                    calib_factor_flag=0;
                    calib_flag=0;
                    //TRACE("*************0***************");
                }
            }
          //  TRACE("factoroffset:%d\n",(int32_t)((factoroffset)*(float)10000000.0f));
        }
    }

    return;
}

#endif

FRAM_TEXT_LOC uint32_t bt_sbc_player_more_data(uint8_t *buf, uint32_t len)
{

#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P))
    a2dp_clock_calib_process(len);
#endif

#ifdef VOICE_DATAPATH
    //if (AI_SPEC_GSOUND == ai_manager_get_current_spec())
    {
        if (app_voicepath_get_stream_pending_state(VOICEPATH_STREAMING))
        {
            af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        #ifdef MIX_MIC_DURING_MUSIC
            app_voicepath_enable_hw_sidetone(0, HW_SIDE_TONE_MAX_ATTENUATION_COEF);
        #endif
            app_voicepath_set_stream_state(VOICEPATH_STREAMING, true);
            app_voicepath_set_pending_started_stream(VOICEPATH_STREAMING, false);
        }
    }
#endif
#ifdef BT_XTAL_SYNC
#ifdef BT_XTAL_SYNC_NEW_METHOD
    btif_remote_device_t   * p_a2dp_remDev=NULL;
    a2dp_get_curStream_remDev(&p_a2dp_remDev);
    if(p_a2dp_remDev != NULL)
    {
        uint32_t bitoffset = btdrv_rf_bitoffset_get( btif_me_get_remote_device_hci_handle(p_a2dp_remDev) -0x80);
        bt_xtal_sync_new(bitoffset,BT_XTAL_SYNC_MODE_WITH_MOBILE);
    }
#else
    bt_xtal_sync(BT_XTAL_SYNC_MODE_MUSIC);
#endif
#endif

#ifndef FPGA
    uint8_t codec_type = bt_sbc_player_get_codec_type();
    uint32_t overlay_id = 0;
    if(codec_type ==  BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC)
    {
        overlay_id = APP_OVERLAY_A2DP_AAC;
    }

#if defined(A2DP_LHDC_ON)
    else if(codec_type ==  BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
    {
        overlay_id = APP_OVERLAY_A2DP_LHDC;
    }
#endif
#if defined(A2DP_LDAC_ON)
    else if(codec_type ==  BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
    {
        overlay_id = APP_OVERLAY_A2DP_LDAC;
    }
#endif


#if defined(A2DP_SCALABLE_ON)
    else if(codec_type ==  BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
    {
        overlay_id = APP_OVERLAY_A2DP_SCALABLE;
    }
#endif
    else
        overlay_id = APP_OVERLAY_A2DP;

    memset(buf, 0, len);
    
    if(app_get_current_overlay() != overlay_id)
    {
        return len;
    }
#endif
#ifdef __AI_VOICE__
    if (app_ai_is_to_mute_a2dp_during_ai_starting_speech())
    {
        memset(buf, 0, len);
        return len;
    }
#endif

#ifdef PLAYBACK_FORCE_48K
    app_playback_resample_run(force48k_resample, buf, len);
#else
#if (A2DP_DECODER_VER == 2)
    a2dp_audio_playback_handler(buf, len);
#else
    a2dp_audio_more_data(overlay_id, buf, len);
#endif
#endif

#ifdef __AUDIO_SPECTRUM__
    audio_spectrum_run(buf, len);
#endif

#ifdef __AUDIO_OUTPUT_MONO_MODE__
#ifdef A2DP_EQ_24BIT
    merge_stereo_to_mono_24bits((int32_t *)buf, (int32_t *)buf, len/sizeof(int32_t));
#else
    merge_stereo_to_mono_16bits((int16_t *)buf, (int16_t *)buf, len/sizeof(int16_t));
#endif
#endif

#ifdef __HEAR_THRU_PEAK_DET__
#ifdef ANC_APP
    if(app_anc_work_status())
#endif
    {
        int vol_level = 0;
        vol_level = app_bt_stream_local_volume_get();
        peak_detector_run(buf, len, pkd_vol_multiple[vol_level]);
    }
#endif

#ifdef ANC_APP
    bt_audio_updata_eq_for_anc(app_anc_work_status());
#endif

    audio_process_run(buf, len);

    app_ring_merge_more_data(buf, len);

#if defined(IBRT) && !defined(FPGA)
    app_tws_ibrt_audio_analysis_audiohandler_tick();
#endif

    osapi_notify_evm();

    return len;
}
#ifdef __THIRDPARTY
bool start_by_sbc = false;
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
static uint32_t audio_mc_data_playback_a2dp(uint8_t *buf, uint32_t mc_len_bytes)
{
   uint32_t begin_time;
   //uint32_t end_time;
   begin_time = hal_sys_timer_get();
   TRACE("music cancel: %d",begin_time);

   float left_gain;
   float right_gain;
   int playback_len_bytes,mc_len_bytes_8;
   int i,j,k;
   int delay_sample;

   hal_codec_get_dac_gain(&left_gain,&right_gain);

   //TRACE("playback_samplerate_ratio:  %d",playback_samplerate_ratio);

  // TRACE("left_gain:  %d",(int)(left_gain*(1<<12)));
  // TRACE("right_gain: %d",(int)(right_gain*(1<<12)));

   playback_len_bytes=mc_len_bytes/playback_samplerate_ratio_bt;

   mc_len_bytes_8=mc_len_bytes/8;

    if (sample_size_play_bt == AUD_BITS_16)
    {
        int16_t *sour_p=(int16_t *)(playback_buf_bt+playback_size_bt/2);
        int16_t *mid_p=(int16_t *)(buf);
        int16_t *mid_p_8=(int16_t *)(buf+mc_len_bytes-mc_len_bytes_8);
        int16_t *dest_p=(int16_t *)buf;

        if(buf == playback_buf_mc)
        {
            sour_p=(int16_t *)playback_buf_bt;
        }

        delay_sample=DELAY_SAMPLE_MC;

        for(i=0,j=0;i<delay_sample;i=i+2)
        {
            mid_p[j++]=delay_buf_bt[i];
            mid_p[j++]=delay_buf_bt[i+1];
        }

        for(i=0;i<playback_len_bytes/2-delay_sample;i=i+2)
        {
            mid_p[j++]=sour_p[i];
            mid_p[j++]=sour_p[i+1];
        }

        for(j=0;i<playback_len_bytes/2;i=i+2)
        {
            delay_buf_bt[j++]=sour_p[i];
            delay_buf_bt[j++]=sour_p[i+1];
        }

        if(playback_samplerate_ratio_bt<=8)
        {
            for(i=0,j=0;i<playback_len_bytes/2;i=i+2*(8/playback_samplerate_ratio_bt))
            {
                mid_p_8[j++]=mid_p[i];
                mid_p_8[j++]=mid_p[i+1];
            }
        }
        else
        {
            for(i=0,j=0;i<playback_len_bytes/2;i=i+2)
            {
                for(k=0;k<playback_samplerate_ratio_bt/8;k++)
                {
                    mid_p_8[j++]=mid_p[i];
                    mid_p_8[j++]=mid_p[i+1];
                }
            }
        }

        anc_mc_run_stereo((uint8_t *)mid_p_8,mc_len_bytes_8,left_gain,right_gain,AUD_BITS_16);

        for(i=0,j=0;i<(int)(mc_len_bytes_8)/2;i=i+2)
        {
            for(k=0;k<8;k++)
            {
                dest_p[j++]=mid_p_8[i];
                dest_p[j++]=mid_p_8[i+1];
            }
        }

    }
    else if (sample_size_play_bt == AUD_BITS_24)
    {
        int32_t *sour_p=(int32_t *)(playback_buf_bt+playback_size_bt/2);
        int32_t *mid_p=(int32_t *)(buf);
        int32_t *mid_p_8=(int32_t *)(buf+mc_len_bytes-mc_len_bytes_8);
        int32_t *dest_p=(int32_t *)buf;

        if(buf == (playback_buf_mc))
        {
            sour_p=(int32_t *)playback_buf_bt;
        }

        delay_sample=DELAY_SAMPLE_MC;

        for(i=0,j=0;i<delay_sample;i=i+2)
        {
            mid_p[j++]=delay_buf_bt[i];
            mid_p[j++]=delay_buf_bt[i+1];

        }

         for(i=0;i<playback_len_bytes/4-delay_sample;i=i+2)
        {
            mid_p[j++]=sour_p[i];
            mid_p[j++]=sour_p[i+1];
        }

         for(j=0;i<playback_len_bytes/4;i=i+2)
        {
            delay_buf_bt[j++]=sour_p[i];
            delay_buf_bt[j++]=sour_p[i+1];
        }

        if(playback_samplerate_ratio_bt<=8)
        {
            for(i=0,j=0;i<playback_len_bytes/4;i=i+2*(8/playback_samplerate_ratio_bt))
            {
                mid_p_8[j++]=mid_p[i];
                mid_p_8[j++]=mid_p[i+1];
            }
        }
        else
        {
            for(i=0,j=0;i<playback_len_bytes/4;i=i+2)
            {
                for(k=0;k<playback_samplerate_ratio_bt/8;k++)
                {
                    mid_p_8[j++]=mid_p[i];
                    mid_p_8[j++]=mid_p[i+1];
                }
            }
        }

        anc_mc_run_stereo((uint8_t *)mid_p_8,mc_len_bytes_8,left_gain,right_gain,AUD_BITS_24);

        for(i=0,j=0;i<(mc_len_bytes_8)/4;i=i+2)
        {
            for(k=0;k<8;k++)
            {
                dest_p[j++]=mid_p_8[i];
                dest_p[j++]=mid_p_8[i+1];
            }
        }

    }

  //  end_time = hal_sys_timer_get();

 //   TRACE("%s:run time: %d", __FUNCTION__, end_time-begin_time);

    return 0;
}
#endif

static uint8_t g_current_eq_index = 0xff;
static bool isMeridianEQON = false;

bool app_is_meridian_on()
{
    return isMeridianEQON;
}

uint8_t app_audio_get_eq()
{
    return g_current_eq_index;
}

bool app_meridian_eq(bool onoff)
{
    isMeridianEQON = onoff;
    return onoff;
}

int app_audio_set_eq(uint8_t index)
{
#ifdef __SW_IIR_EQ_PROCESS__
    if (index >=EQ_SW_IIR_LIST_NUM)
        return -1;
#endif
#ifdef __HW_FIR_EQ_PROCESS__
    if (index >=EQ_HW_FIR_LIST_NUM)
        return -1;
#endif
#ifdef __HW_DAC_IIR_EQ_PROCESS__
    if (index >=EQ_HW_DAC_IIR_LIST_NUM)
        return -1;
#endif
#ifdef __HW_IIR_EQ_PROCESS__
    if (index >=EQ_HW_IIR_LIST_NUM)
        return -1;
#endif
    g_current_eq_index = index;
    return index;
}

void bt_audio_updata_eq(uint8_t index)
{
    TRACE("[%s]anc_status = %d", __func__, index);    
#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)|| defined(__HW_DAC_IIR_EQ_PROCESS__)|| defined(__HW_IIR_EQ_PROCESS__)
    AUDIO_EQ_TYPE_T audio_eq_type;
#ifdef __SW_IIR_EQ_PROCESS__
    audio_eq_type = AUDIO_EQ_TYPE_SW_IIR;
#endif

#ifdef __HW_FIR_EQ_PROCESS__
    audio_eq_type = AUDIO_EQ_TYPE_HW_FIR;
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
    audio_eq_type = AUDIO_EQ_TYPE_HW_DAC_IIR;
#endif

#ifdef __HW_IIR_EQ_PROCESS__
    audio_eq_type = AUDIO_EQ_TYPE_HW_IIR;
#endif
    bt_audio_set_eq(audio_eq_type,index);
#endif
}


uint8_t bt_audio_updata_eq_for_anc(uint8_t anc_status)
{
#ifdef ANC_APP
    static uint8_t anc_status_record = 0xff;

    anc_status = app_anc_work_status();
    if(anc_status_record != anc_status)
    {
        anc_status_record = anc_status;
        TRACE("[%s]anc_status = %d", __func__, anc_status);
#ifdef __SW_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_SW_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_SW_IIR,anc_status));
#endif

#ifdef __HW_FIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_FIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_FIR,anc_status));
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_DAC_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_DAC_IIR,anc_status));
#endif

#ifdef __HW_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_IIR,anc_status));
#endif
    }
#endif
    return 0;
}

uint8_t bt_audio_get_eq_index(AUDIO_EQ_TYPE_T audio_eq_type,uint8_t anc_status)
{
    uint8_t index_eq=0;

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)|| defined(__HW_DAC_IIR_EQ_PROCESS__)|| defined(__HW_IIR_EQ_PROCESS__)
    switch (audio_eq_type)
    {
#if defined(__SW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_SW_IIR:
        {
            if(anc_status)
            {
                index_eq=audio_eq_sw_iir_index+1;
            }
            else
            {
                index_eq=audio_eq_sw_iir_index;
            }

        }
        break;
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_FIR:
        {
            if(a2dp_sample_rate == AUD_SAMPRATE_44100) {
                index_eq = 0;
            } else if(a2dp_sample_rate == AUD_SAMPRATE_48000) {
                index_eq = 1;
            } else if(a2dp_sample_rate == AUD_SAMPRATE_96000) {
                index_eq = 2;
            } else {
                ASSERT(0, "[%s] sample_rate_recv(%d) is not supported", __func__, a2dp_sample_rate);
            }
            audio_eq_hw_fir_index=index_eq;

            if(anc_status)
            {
                index_eq=index_eq+3;
            }
        }
        break;
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_DAC_IIR:
        {
            if(anc_status)
            {
                index_eq=audio_eq_hw_dac_iir_index+1;
            }
            else
            {
                index_eq=audio_eq_hw_dac_iir_index;
            }
        }
        break;
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_IIR:
        {
            if(anc_status)
            {
                index_eq=audio_eq_hw_iir_index+1;
            }
            else
            {
                index_eq=audio_eq_hw_iir_index;
            }
        }
        break;
#endif
        default:
        {
            ASSERT(false,"[%s]Error eq type!",__func__);
        }
    }
#endif
    return index_eq;
}


uint32_t bt_audio_set_eq(AUDIO_EQ_TYPE_T audio_eq_type, uint8_t index)
{
    const FIR_CFG_T *fir_cfg=NULL;
    const IIR_CFG_T *iir_cfg=NULL;

    TRACE("[%s]audio_eq_type=%d,index=%d", __func__, audio_eq_type,index);

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)|| defined(__HW_DAC_IIR_EQ_PROCESS__)|| defined(__HW_IIR_EQ_PROCESS__)
    switch (audio_eq_type)
    {
#if defined(__SW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_SW_IIR:
        {
            if(index >= EQ_SW_IIR_LIST_NUM)
            {
                TRACE("[%s] index > EQ_SW_IIR_LIST_NUM", __func__, index);
                return 1;
            }

            iir_cfg=audio_eq_sw_iir_cfg_list[index];
        }
        break;
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_FIR:
        {
            if(index >= EQ_HW_FIR_LIST_NUM)
            {
                TRACE("[%s] index > EQ_HW_FIR_LIST_NUM", __func__, index);
                return 1;
            }

            fir_cfg=audio_eq_hw_fir_cfg_list[index];
        }
        break;
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_DAC_IIR:
        {
            if(index >= EQ_HW_DAC_IIR_LIST_NUM)
            {
                TRACE("[%s] index > EQ_HW_DAC_IIR_LIST_NUM", __func__, index);
                return 1;
            }

            iir_cfg=audio_eq_hw_dac_iir_cfg_list[index];
        }
        break;
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_IIR:
        {
            if(index >= EQ_HW_IIR_LIST_NUM)
            {
                TRACE("[%s] index > EQ_HW_IIR_LIST_NUM", __func__, index);
                return 1;
            }

            iir_cfg=audio_eq_hw_iir_cfg_list[index];
        }
        break;
#endif
        default:
        {
            ASSERT(false,"[%s]Error eq type!",__func__);
        }
    }
#endif

#ifdef AUDIO_SECTION_ENABLE
    const IIR_CFG_T *iir_cfg_from_audio_section = (const IIR_CFG_T *)load_audio_cfg_from_audio_section(AUDIO_PROCESS_TYPE_IIR_EQ);
    if (iir_cfg_from_audio_section)
    {
        iir_cfg = iir_cfg_from_audio_section;
    }
#endif

    return audio_eq_set_cfg(fir_cfg,iir_cfg,audio_eq_type);
}

#define __BT_SBC_PLAYER_USE_BT_TRIGGER__

/********************************
        AUD_BITS_16
        dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
        AUD_BITS_24
        dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

        dma_buffer_delay_us
        aac delay = 1024/sample*1000*n ms
        aac delay = 1024/44100*1000*5 = 116ms
        audio_delay = aac delay + dma_buffer_delay_us/2
 *********************************/

#define BT_SBC_PLAYER_PLAYBACK_DELAY_AAC_MTU (5)
#define BT_SBC_PLAYER_PLAYBACK_DELAY_AAC_BASE (23000)
#define BT_SBC_PLAYER_PLAYBACK_DELAY_AAC_US (BT_SBC_PLAYER_PLAYBACK_DELAY_AAC_BASE*BT_SBC_PLAYER_PLAYBACK_DELAY_AAC_MTU)

/********************************
    AUD_BITS_16
    dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
    AUD_BITS_24
    dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

    sbc delay = 128/sample*n ms
    sbc delay = 128/44100*45 = 130ms
    audio_delay = aac delay + dma_buffer_delay_us/2 (23219us)
*********************************/    

#define BT_SBC_PLAYER_PLAYBACK_DELAY_SBC_MTU (45)
#define BT_SBC_PLAYER_PLAYBACK_DELAY_SBC_BASE (2800)
#define BT_SBC_PLAYER_PLAYBACK_DELAY_SBC_US (BT_SBC_PLAYER_PLAYBACK_DELAY_SBC_BASE*BT_SBC_PLAYER_PLAYBACK_DELAY_SBC_MTU)

enum BT_STREAM_TRIGGER_STATUS_T {
    BT_STREAM_TRIGGER_STATUS_NULL = 0,
    BT_STREAM_TRIGGER_STATUS_INIT,
    BT_STREAM_TRIGGER_STATUS_WAIT,
    BT_STREAM_TRIGGER_STATUS_OK,
};

static uint32_t tg_acl_trigger_time = 0;
static uint32_t tg_acl_trigger_start_time = 0;
static uint32_t tg_acl_trigger_init_time = 0;
static enum BT_STREAM_TRIGGER_STATUS_T bt_stream_trigger_status = BT_STREAM_TRIGGER_STATUS_NULL;

void app_bt_stream_playback_irq_notification(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

inline void app_bt_stream_trigger_stauts_set(enum BT_STREAM_TRIGGER_STATUS_T stauts)
{
    bt_stream_trigger_status = stauts;
}

inline enum BT_STREAM_TRIGGER_STATUS_T app_bt_stream_trigger_stauts_get(void)
{
    return bt_stream_trigger_status;
}

uint32_t app_bt_stream_get_dma_buffer_delay_us(void)
{
    uint32_t dma_buffer_delay_us = 0;
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, false);
    
    if (stream_cfg->bits <= AUD_BITS_16){
        dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
    }else{
        dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;
    }
    return dma_buffer_delay_us;
}

uint32_t app_bt_stream_get_dma_buffer_samples(void)
{
    uint32_t dma_buffer_delay_samples = 0;
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, false);
    
    if (stream_cfg->bits <= AUD_BITS_16){
        dma_buffer_delay_samples = stream_cfg->data_size/stream_cfg->channel_num/2;
    }else{
        dma_buffer_delay_samples = stream_cfg->data_size/stream_cfg->channel_num/4;
    }
    return dma_buffer_delay_samples;
}

#if defined(IBRT)
void app_bt_stream_ibrt_set_trigger_time(APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger);

APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T app_bt_stream_ibrt_auto_synchronize_trigger;
int app_bt_stream_ibrt_audio_mismatch_stopaudio(void);

int app_bt_stream_ibrt_auto_synchronize_trigger_start(btif_media_header_t * header, unsigned char *buf, unsigned int len)
{
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger = &app_bt_stream_ibrt_auto_synchronize_trigger;
    a2dp_audio_detect_next_packet_callback_register(NULL);
    TRACE("[auto_synchronize_trigger_start] trigger_time:%d seq:%d timestamp:%d SubSeq:%d/%d",
                                                                                   sync_trigger->trigger_time, 
                                                                                   sync_trigger->audio_info.sequenceNumber, 
                                                                                   sync_trigger->audio_info.timestamp,
                                                                                   sync_trigger->audio_info.curSubSequenceNumber,
                                                                                   sync_trigger->audio_info.totalSubSequenceNumber
                                                                                   );
    
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    
    if (app_ibrt_ui_ibrt_connected()){
        if (sync_trigger->trigger_time >= bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle)){
            app_bt_stream_ibrt_set_trigger_time(sync_trigger);
        }else{    
            TRACE("[%s]dataind_seq timestamp:%d/%d mismatch need resume", __func__, sync_trigger->trigger_time, bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle));        
            app_bt_stream_ibrt_audio_mismatch_stopaudio();
        }
    }
    return 0;
}

int app_bt_stream_ibrt_auto_synchronize_dataind_cb(btif_media_header_t * header, unsigned char *buf, unsigned int len)
{
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger = &app_bt_stream_ibrt_auto_synchronize_trigger;    
    A2DP_AUDIO_SYNCFRAME_INFO_T sync_info;
    bool synchronize_ok = false;
    int32_t timestamp_diff = 0;
    int32_t dma_buffer_samples = 0;

    TRACE("[%s] dataind_seq:%d/%d timestamp:%d/%d", __func__, header->sequenceNumber,
                                                              sync_trigger->audio_info.sequenceNumber,
                                                              header->timestamp,
                                                              sync_trigger->audio_info.timestamp);
    
    if (sync_trigger->audio_info.sequenceNumber < header->sequenceNumber &&
        sync_trigger->audio_info.timestamp < header->timestamp){        
        if (app_ibrt_ui_ibrt_connected()){
            a2dp_audio_detect_next_packet_callback_register(NULL);            
            TRACE("[%s]dataind_seq timestamp:%d/%d mismatch need resume", __func__, sync_trigger->audio_info.timestamp, header->timestamp);        
            app_bt_stream_ibrt_audio_mismatch_stopaudio();
        }
    }else{
        if (sync_trigger->audio_info.sequenceNumber == header->sequenceNumber){
            synchronize_ok = true;
        }else if (sync_trigger->audio_info.timestamp == header->timestamp){
            synchronize_ok = true;
        }
        if (sync_trigger->audio_info.timestamp >= header->timestamp && sync_trigger->audio_info.totalSubSequenceNumber){
            dma_buffer_samples = app_bt_stream_get_dma_buffer_samples()/2;
            timestamp_diff = sync_trigger->audio_info.timestamp - header->timestamp;
            if (timestamp_diff < dma_buffer_samples){
                synchronize_ok = true;
            }
        }
     
        //flush all
        sync_info.sequenceNumber = UINT16_MAX;    
        sync_info.timestamp = UINT32_MAX;
        a2dp_audio_synchronize_packet(&sync_info);

        if (synchronize_ok){        
            A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
            a2dp_audio_lastframe_info_get(&lastframe_info); 

            TRACE("[%s]synchronize ok timestamp_diff:%d frame_samples:%d", __func__, timestamp_diff, lastframe_info.frame_samples);        

            sync_trigger->trigger_type = APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_LOCAL;
            sync_trigger->audio_info.sequenceNumber = header->sequenceNumber;
            sync_trigger->audio_info.timestamp = header->timestamp;
            sync_trigger->audio_info.curSubSequenceNumber = timestamp_diff/lastframe_info.frame_samples;
            sync_trigger->audio_info.totalSubSequenceNumber = lastframe_info.totalSubSequenceNumber;
            sync_trigger->audio_info.frame_samples = lastframe_info.frame_samples;
            sync_trigger->audio_info.decoded_frames = lastframe_info.decoded_frames;
            sync_trigger->audio_info.sample_rate = lastframe_info.stream_info->sample_rate;
            
            a2dp_audio_detect_next_packet_callback_register(app_bt_stream_ibrt_auto_synchronize_trigger_start);   
        }else{
            a2dp_audio_detect_first_packet();
        }
    }
    return 0;
}

void app_bt_stream_ibrt_auto_synchronize_start(APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger)
{
    TRACE("[%s] trigger_time:%d seq:%d timestamp:%d SubSeq:%d/%d", __func__, 
                                                                   sync_trigger->trigger_time, 
                                                                   sync_trigger->audio_info.sequenceNumber, 
                                                                   sync_trigger->audio_info.timestamp,
                                                                   sync_trigger->audio_info.curSubSequenceNumber,
                                                                   sync_trigger->audio_info.totalSubSequenceNumber
                                                                   );
    app_bt_stream_ibrt_auto_synchronize_trigger = *sync_trigger;
    a2dp_audio_detect_next_packet_callback_register(app_bt_stream_ibrt_auto_synchronize_dataind_cb);
    a2dp_audio_detect_first_packet();
}

void app_bt_stream_ibrt_auto_synchronize_stop(void)
{
    a2dp_audio_detect_next_packet_callback_register(NULL);
}

void app_bt_stream_ibrt_start_sbc_player_callback(uint32_t status, uint32_t param)
{
    if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)){        
        TRACE("[%s] trigger(%d)-->tg(%d)", __func__, param, ((APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *)param)->trigger_time);
        app_bt_stream_ibrt_set_trigger_time((APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *)param);
    }else{
        TRACE("[%s] try again", __func__);
        app_audio_manager_sendrequest_need_callback(
                                                    APP_BT_STREAM_MANAGER_START,BT_STREAM_SBC, 
                                                    BT_DEVICE_ID_1, 
                                                    MAX_RECORD_NUM, 
                                                    (uint32_t)app_bt_stream_ibrt_start_sbc_player_callback,
                                                    param);
    }
}

int app_bt_stream_ibrt_start_sbc_player(APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger)
{
    TRACE("[%s] tg(%d)", __func__, sync_trigger->trigger_time);
    app_audio_manager_sendrequest_need_callback(
                                                APP_BT_STREAM_MANAGER_START,BT_STREAM_SBC, 
                                                BT_DEVICE_ID_1, 
                                                MAX_RECORD_NUM, 
                                                (uint32_t)app_bt_stream_ibrt_start_sbc_player_callback,
                                                (uint32_t)sync_trigger);
    return 0;
}

void app_bt_stream_ibrt_set_trigger_time(APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger)
{
    uint32_t curr_ticks = 0;
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    uint32_t tg_tick = sync_trigger->trigger_time;
    A2DP_AUDIO_SYNCFRAME_INFO_T sync_info;
    int synchronize_ret;

    if (!app_bt_is_a2dp_streaming(BTIF_DEVICE_ID_1)){        
        TRACE("[%s] streaming not ready skip it", __func__);
        return;
    }
    
    sync_info.sequenceNumber = sync_trigger->audio_info.sequenceNumber;
    sync_info.timestamp = sync_trigger->audio_info.timestamp;
    sync_info.curSubSequenceNumber = sync_trigger->audio_info.curSubSequenceNumber;
    sync_info.totalSubSequenceNumber = sync_trigger->audio_info.totalSubSequenceNumber;
    synchronize_ret = a2dp_audio_synchronize_packet(&sync_info);

    if (app_ibrt_ui_ibrt_connected()){
        curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle);
        if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)){
            if (tg_tick >= curr_ticks && synchronize_ret >= 0){                
                btdrv_syn_trigger_codec_en(0);
                btdrv_syn_clr_trigger();
                btdrv_enable_playback_triggler(ACL_TRIGGLE_MODE);
                bt_syn_set_tg_ticks(tg_tick, p_ibrt_ctrl->ibrt_conhandle, BT_TRIG_MASTER_ROLE);
                tg_acl_trigger_time = tg_tick;
                TRACE("[%s] trigger curr(%d)-->tg(%d)", __func__, curr_ticks, tg_tick);
                
                app_tws_ibrt_audio_analysis_start(0, ANALYSIS_CHECKER_INTERVEL_INVALID);
                app_tws_ibrt_audio_sync_start();
                app_tws_ibrt_audio_sync_new_reference(sync_trigger->factor_reference);
            }else{
#if defined(IBRT_FORCE_AUDIO_RETRIGGER)
                app_ibrt_if_force_audio_retrigger()
#else
                TRACE("[%s] trigger invalid curr(%d)-->tg(%d) packet:%d", __func__, curr_ticks, tg_tick, synchronize_ret);
                if (sync_trigger->trigger_type == APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_INIT_SYNC){
                    int analysis_interval = app_tws_ibrt_audio_analysis_interval_get();
                    sync_trigger->trigger_time += US_TO_BTCLKS(analysis_interval * 
                                                               (app_bt_stream_get_dma_buffer_delay_us()/2)*
                                                               (double)sync_trigger->factor_reference);
                    sync_trigger->audio_info.sequenceNumber += analysis_interval;
                    sync_trigger->audio_info.timestamp += analysis_interval*(app_bt_stream_get_dma_buffer_samples()/2);
                    TRACE("[%s] TYPE_INIT_SYNC reconfig seq:%d tg_tick(%d)", __func__, sync_trigger->audio_info.sequenceNumber, sync_trigger->trigger_time);
                }
                app_bt_stream_ibrt_auto_synchronize_start(sync_trigger);
#endif
            }
        }else if (!app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)){            
            TRACE("[%s] sbc player not active, so try to start it", __func__);
            app_bt_stream_ibrt_auto_synchronize_trigger = *sync_trigger;
            app_bt_stream_ibrt_start_sbc_player(&app_bt_stream_ibrt_auto_synchronize_trigger);
        }
    }else{
        TRACE("[%s] Not Connected", __func__);
    }
}
void app_bt_stream_ibrt_audio_mismatch_stopaudio_cb(uint32_t status, uint32_t param)
{
    if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)){
        TRACE("[%s] try again", __func__);
        app_audio_manager_sendrequest_need_callback(
                                                    APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC, 
                                                    BT_DEVICE_ID_1, 
                                                    MAX_RECORD_NUM, 
                                                    (uint32_t)app_bt_stream_ibrt_audio_mismatch_stopaudio_cb,
                                                    (uint32_t)NULL);
    }else{
        app_tws_ibrt_audio_sync_mismatch_resume_notify();
    }    
}

int app_bt_stream_ibrt_audio_mismatch_stopaudio(void)
{
    if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)){
        app_audio_manager_sendrequest_need_callback(
                                                    APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC, 
                                                    BT_DEVICE_ID_1, 
                                                    MAX_RECORD_NUM, 
                                                    (uint32_t)app_bt_stream_ibrt_audio_mismatch_stopaudio_cb,
                                                    (uint32_t)NULL);
    }else{
        app_tws_ibrt_audio_sync_mismatch_resume_notify();
    }
    return 0;
}
#endif
void app_bt_stream_set_trigger_time(uint32_t trigger_time_us)
{
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    uint32_t curr_ticks = 0;
    uint32_t dma_buffer_delay_us = 0;
    uint32_t tg_acl_trigger_offset_time = 0;


    if (trigger_time_us){
#if defined(IBRT)
        ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
        curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->mobile_conhandle);
#else
        curr_ticks = btdrv_syn_get_curr_ticks();
#endif
        af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, false);
        btdrv_syn_trigger_codec_en(0);
        btdrv_syn_clr_trigger();

        btdrv_enable_playback_triggler(ACL_TRIGGLE_MODE);

        dma_buffer_delay_us = app_bt_stream_get_dma_buffer_delay_us();
        dma_buffer_delay_us /= 2;
        TRACE("[%s] ch:%d sample_rate:%d bit:%d dma_size:%d delay:%d", __func__, stream_cfg->channel_num, stream_cfg->sample_rate, stream_cfg->bits, stream_cfg->data_size, dma_buffer_delay_us);

        tg_acl_trigger_offset_time = US_TO_BTCLKS(trigger_time_us-dma_buffer_delay_us);
        tg_acl_trigger_time = curr_ticks + tg_acl_trigger_offset_time;
        tg_acl_trigger_start_time = curr_ticks;
#if defined(IBRT)
        bt_syn_set_tg_ticks(tg_acl_trigger_time, p_ibrt_ctrl->mobile_conhandle, BT_TRIG_SLAVE_ROLE);
        TRACE("[%s] ibrt trigger curr(%d)-->tg(%d) trig_delay:%d audio_delay:%dus", __func__, 
                                            curr_ticks, tg_acl_trigger_time, trigger_time_us-dma_buffer_delay_us, trigger_time_us+dma_buffer_delay_us);
#else
        bt_syn_set_tg_ticks(tg_acl_trigger_time, 0, BT_TRIG_NONE_ROLE);
        TRACE("[%s] trigger curr(%d)-->tg(%d) trig_delay:%d audio_delay:%dus", __func__, 
                                    curr_ticks, tg_acl_trigger_time, trigger_time_us-dma_buffer_delay_us, trigger_time_us+dma_buffer_delay_us);
#endif

        btdrv_syn_trigger_codec_en(1);
        app_bt_stream_trigger_stauts_set(BT_STREAM_TRIGGER_STATUS_WAIT);
    }else{
        tg_acl_trigger_time = 0;
        tg_acl_trigger_start_time = 0;
        btdrv_syn_trigger_codec_en(0);
        btdrv_syn_clr_trigger();
        app_bt_stream_trigger_stauts_set(BT_STREAM_TRIGGER_STATUS_NULL);
        TRACE("[%s] trigger clear", __func__);
    }
}

void app_bt_stream_trigger_result(void)
{
    uint32_t curr_ticks = 0; 

    if(tg_acl_trigger_time){
#if defined(IBRT)
        ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
        if (app_tws_ibrt_mobile_link_connected()){
            curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->mobile_conhandle);
        }else if (app_tws_ibrt_ibrt_link_connected()){
            curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle);
        }else{
            TRACE("%S mobile_link:%d %04x ibrt_link:%d %04x", __func__, app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
                                                                     app_tws_ibrt_ibrt_link_connected(),   p_ibrt_ctrl->ibrt_conhandle);
        }
#else
        curr_ticks = btdrv_syn_get_curr_ticks();
#endif
        TRACE("[trigger_result] trig(%d)-->curr(%d)-->tg(%d) start:%d", (curr_ticks - (uint32_t)US_TO_BTCLKS(app_bt_stream_get_dma_buffer_delay_us()/2)),
                                                                         curr_ticks, tg_acl_trigger_time, tg_acl_trigger_start_time);
        TRACE("tg_trig_diff:%d trig_diff:%d", (uint32_t)BTCLKS_TO_US(curr_ticks-tg_acl_trigger_time), 
                                              (uint32_t)BTCLKS_TO_US(curr_ticks-tg_acl_trigger_start_time));                                                               
        app_bt_stream_set_trigger_time(0);        
        app_bt_stream_trigger_stauts_set(BT_STREAM_TRIGGER_STATUS_OK);
    }
}

void app_bt_stream_playback_irq_notification(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    if (id != AUD_STREAM_ID_0 || stream != AUD_STREAM_PLAYBACK) {
        return;
    }
    app_bt_stream_trigger_result();
#if defined(IBRT)
    app_tws_ibrt_audio_analysis_interrupt_tick();
#endif
}
extern void a2dp_audio_set_mtu_limit(uint8_t mut);
extern uint8_t a2dp_lhdc_config_llc_get(void);

void app_bt_stream_trigger_init(void)
{
#if defined(IBRT)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (app_tws_ibrt_mobile_link_connected()){
        tg_acl_trigger_init_time = bt_syn_get_curr_ticks(p_ibrt_ctrl->mobile_conhandle);
    }else if (app_tws_ibrt_ibrt_link_connected()){
        tg_acl_trigger_init_time = bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle);
    }else{
        TRACE("%S mobile_link:%d %04x ibrt_link:%d %04x", __func__, app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
                                                                 app_tws_ibrt_ibrt_link_connected(),   p_ibrt_ctrl->ibrt_conhandle);
   }
#else
    tg_acl_trigger_init_time = btdrv_syn_get_curr_ticks();
#endif
    app_bt_stream_set_trigger_time(0);
    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, false);
    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, true);
    app_bt_stream_trigger_stauts_set(BT_STREAM_TRIGGER_STATUS_INIT);
}

void app_bt_stream_trigger_deinit(void)
{
    app_bt_stream_set_trigger_time(0);
}

void app_bt_stream_trigger_start(void)
{
    uint32_t tg_trigger_time;
    uint32_t curr_ticks;        

#if defined(IBRT)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->mobile_conhandle);
#else
    curr_ticks = btdrv_syn_get_curr_ticks();    
#endif

    TRACE("[%s] init(%d)-->set_trig(%d) %dus", __func__, tg_acl_trigger_init_time, curr_ticks, (uint32_t)BTCLKS_TO_US(curr_ticks-tg_acl_trigger_init_time));
#if defined(A2DP_AAC_ON)
    if(bt_sbc_player_get_codec_type() == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC){
        tg_trigger_time = BT_SBC_PLAYER_PLAYBACK_DELAY_AAC_US;        
#if (A2DP_DECODER_VER < 2)              
        a2dp_audio_set_mtu_limit(BT_SBC_PLAYER_PLAYBACK_DELAY_AAC_MTU);
#endif
    }else
#endif
#if defined(A2DP_LHDC_ON)
    if(bt_sbc_player_get_codec_type() == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
        if (a2dp_lhdc_config_llc_get()){
            tg_trigger_time = 11000*2;            
            a2dp_audio_set_mtu_limit(3);
        }else{
            tg_trigger_time = 23000*6;            
            a2dp_audio_set_mtu_limit(6);
        }
    }else
#endif

    {
        tg_trigger_time = BT_SBC_PLAYER_PLAYBACK_DELAY_SBC_US;
#if (A2DP_DECODER_VER < 2)        
        a2dp_audio_set_mtu_limit(BT_SBC_PLAYER_PLAYBACK_DELAY_SBC_MTU/5);
#endif
    }
    app_bt_stream_set_trigger_time(tg_trigger_time);   
}

bool app_bt_stream_trigger_onprocess(void)
{
    if (app_bt_stream_trigger_stauts_get() == BT_STREAM_TRIGGER_STATUS_INIT){
        return true;
    }else{
        return false;
    }
}

int app_bt_stream_detect_next_packet_cb(btif_media_header_t * header, unsigned char *buf, unsigned int len)
{
    if(app_bt_stream_trigger_onprocess()){
#if defined(IBRT)
        ibrt_ctrl_t  *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

        A2DP_AUDIO_SYNCFRAME_INFO_T sync_info;
        sync_info.sequenceNumber = UINT16_MAX;    
        sync_info.timestamp = UINT32_MAX;

        if (!app_ibrt_ui_ibrt_connected()){            
            if (p_ibrt_ctrl->mobile_mode == IBRT_SNIFF_MODE){
                //flush all
                a2dp_audio_synchronize_packet(&sync_info);
                a2dp_audio_detect_first_packet();
                TRACE("cache skip delay dma trigger\n");
                return 0;
            }
            TRACE("cache ok use dma trigger\n");
            app_bt_stream_trigger_start();
        }else{    
            if (app_ibrt_ui_ibrt_connected() &&
                (p_ibrt_ctrl->tws_mode == IBRT_SNIFF_MODE    || 
                 p_ibrt_ctrl->mobile_mode == IBRT_SNIFF_MODE ||
                !app_ibrt_ui_is_profile_exchanged())){
                //flush all
                a2dp_audio_synchronize_packet(&sync_info);
                a2dp_audio_detect_first_packet();
                TRACE("cache skip delay dma trigger\n");
                return 0;
            }
            TRACE("cache ok use dma trigger\n");
            a2dp_audio_synchronize_packet(&sync_info);
            app_bt_stream_trigger_start();
            if (app_tws_ibrt_mobile_link_connected() &&
                app_ibrt_ui_ibrt_connected() &&
                app_ibrt_ui_is_profile_exchanged()){
                APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T sync_trigger;
                A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
                sync_trigger.trigger_time = tg_acl_trigger_time;
                sync_trigger.trigger_type = APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_INIT_SYNC;
                a2dp_audio_lastframe_info_get(&lastframe_info);
                sync_trigger.audio_info.sequenceNumber = lastframe_info.sequenceNumber;
                sync_trigger.audio_info.timestamp = lastframe_info.timestamp;
                sync_trigger.audio_info.curSubSequenceNumber = lastframe_info.curSubSequenceNumber;
                sync_trigger.audio_info.totalSubSequenceNumber = lastframe_info.totalSubSequenceNumber;
                sync_trigger.audio_info.frame_samples = lastframe_info.frame_samples;
                sync_trigger.audio_info.decoded_frames = lastframe_info.decoded_frames;
                sync_trigger.audio_info.sample_rate = lastframe_info.stream_info->sample_rate;
                sync_trigger.factor_reference = lastframe_info.stream_info->factor_reference;
                tws_ctrl_send_cmd(APP_TWS_CMD_SET_TRIGGER_TIME, (uint8_t*)&sync_trigger, sizeof(APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T));
            }
        }
#else
        TRACE("cache ok use dma trigger\n");
        app_bt_stream_trigger_start();        
#endif
    }
    return 0;
}

void app_audio_buffer_check(void)
{
    TRACE("audio buf size[%d] capture buf size[%d] total available space[%d]",
        APP_AUDIO_BUFFER_SIZE, APP_CAPTURE_AUDIO_BUFFER_SIZE, syspool_original_size());
        
    ASSERT((APP_AUDIO_BUFFER_SIZE + APP_CAPTURE_AUDIO_BUFFER_SIZE) <= syspool_original_size(),
        "Audio buffer[%d]+Capture buffer[%d] exceeds the maximum ram sapce[%d]",
        APP_AUDIO_BUFFER_SIZE, APP_CAPTURE_AUDIO_BUFFER_SIZE, syspool_original_size());
}

static bool isRun =  false;
int bt_sbc_player(enum PLAYER_OPER_T on, enum APP_SYSFREQ_FREQ_T freq)
{
    struct AF_STREAM_CONFIG_T stream_cfg;
    enum AUD_SAMPRATE_T sample_rate;

    uint8_t* bt_audio_buff = NULL;

    uint8_t POSSIBLY_UNUSED *bt_eq_buff = NULL;
    uint32_t POSSIBLY_UNUSED eq_buff_size = 0;
    uint8_t POSSIBLY_UNUSED play_samp_size;

    TRACE("bt_sbc_player work:%d op:%d freq:%d :sample:%d \n", isRun, on, freq,a2dp_sample_rate);

    if ((isRun && on == PLAYER_OPER_START) || (!isRun && on == PLAYER_OPER_STOP))
        return 0;

#if defined(A2DP_LHDC_ON) ||defined(A2DP_AAC_ON) || defined(A2DP_SCALABLE_ON) || defined(A2DP_LDAC_ON)
    uint8_t codec_type POSSIBLY_UNUSED = bt_sbc_player_get_codec_type();
#endif

    if (on == PLAYER_OPER_STOP || on == PLAYER_OPER_RESTART)
    {
#ifdef __THIRDPARTY
        start_by_sbc = false;
#endif

        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        af_stream_stop(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P))
        calib_reset = 1;
        af_stream_dma_tc_irq_disable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif
#if defined(IBRT)
        af_stream_dma_tc_irq_disable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif

#if defined(__AUDIO_SPECTRUM__)
        audio_spectrum_close();
#endif

        audio_process_close();

        TRACE("[%s] syspool free size: %d/%d", __FUNCTION__, syspool_free_size(), syspool_total_size());

        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        af_stream_close(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

#if defined(__THIRDPARTY)
        bt_sbc_mode = 0;
#endif
#ifdef VOICE_DATAPATH
        app_voicepath_set_stream_state(AUDIO_OUTPUT_STREAMING, false);
        app_voicepath_set_pending_started_stream(AUDIO_OUTPUT_STREAMING, false);
#endif
        af_set_irq_notification(NULL);
#if defined(IBRT)
        app_bt_stream_ibrt_auto_synchronize_stop();
        app_tws_ibrt_audio_analysis_stop();
        app_tws_ibrt_audio_sync_stop();
#endif
        if (on == PLAYER_OPER_STOP)
        {
#ifdef __BT_SBC_PLAYER_USE_BT_TRIGGER__    
            app_bt_stream_trigger_deinit();
#endif
#ifndef FPGA
#ifdef BT_XTAL_SYNC
            bt_term_xtal_sync(false);
            bt_term_xtal_sync_default();
#endif
#endif
#if (A2DP_DECODER_VER == 2)
            a2dp_audio_stop();
#endif
            a2dp_audio_deinit();
            app_overlay_unloadall();
#ifdef __THIRDPARTY
            app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO1,THIRDPARTY_STOP2MIC);
            app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO1,THIRDPARTY_START);
#endif
            app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
            af_set_priority(osPriorityAboveNormal);
        }
    }

    if (on == PLAYER_OPER_START || on == PLAYER_OPER_RESTART)
    {
#ifdef __THIRDPARTY
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO1,THIRDPARTY_STOP);
#endif
        af_set_priority(osPriorityHigh);
        bt_media_volume_ptr_update_by_mediatype(BT_STREAM_SBC);
        stream_local_volume = btdevice_volume_p->a2dp_vol;
        app_audio_mempool_init_with_specific_size(APP_AUDIO_BUFFER_SIZE);

#ifdef __BT_ONE_BRING_TWO__
        if (btif_me_get_activeCons()>1)
        {
            if (freq < APP_SYSFREQ_104M) {
                freq = APP_SYSFREQ_104M;
            }
        }
#endif
#ifdef __PC_CMD_UART__
        if (freq < APP_SYSFREQ_104M) {
            freq = APP_SYSFREQ_104M;
        }
#endif
#if defined(__SW_IIR_EQ_PROCESS__)&&defined(__HW_FIR_EQ_PROCESS__)&&defined(CHIP_BEST1000)
        if (audio_eq_hw_fir_cfg_list[bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_FIR,0)]->len>128)
        {
            if (freq < APP_SYSFREQ_104M)
            {
                freq = APP_SYSFREQ_104M;
            }
        }
#endif
#if defined(APP_MUSIC_26M) && !defined(__SW_IIR_EQ_PROCESS__)&& !defined(__HW_IIR_EQ_PROCESS__)&& !defined(__HW_FIR_EQ_PROCESS__)
        if (freq < APP_SYSFREQ_26M) {
            freq = APP_SYSFREQ_26M;
        }
#else
        if (freq < APP_SYSFREQ_52M) {
            freq = APP_SYSFREQ_52M;
        }
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        if (freq < APP_SYSFREQ_104M) {
            freq = APP_SYSFREQ_104M;
        }
#endif

#if defined(A2DP_AAC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
            if(freq < APP_SYSFREQ_52M) {
                freq = APP_SYSFREQ_52M;
            }
        }
#endif
#if defined(A2DP_SCALABLE_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
        {
            if(a2dp_sample_rate==44100)
            {
                if(freq < APP_SYSFREQ_78M) {
                    freq = APP_SYSFREQ_78M;
                }
            }
            else if(a2dp_sample_rate==96000)
            {
                if (freq < APP_SYSFREQ_208M) {
                    freq = APP_SYSFREQ_208M;
                }
            }
        }
        TRACE("a2dp_sample_rate=%d",a2dp_sample_rate);
#endif
#if defined(A2DP_LHDC_ON) && defined(__SW_IIR_EQ_PROCESS__)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
            if (freq < APP_SYSFREQ_104M) {
                freq = APP_SYSFREQ_104M;
            }
        }
#endif
#if defined(A2DP_LDAC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
            if (freq < APP_SYSFREQ_104M) {
                freq = APP_SYSFREQ_104M;
            }
        }
#endif

#if defined(__AUDIO_DRC__) || defined(__AUDIO_DRC2__)
        freq = (freq < APP_SYSFREQ_208M)?APP_SYSFREQ_208M:freq;
#endif

#ifdef AUDIO_OUTPUT_SW_GAIN
        freq = (freq < APP_SYSFREQ_104M)?APP_SYSFREQ_104M:freq;
#endif

#ifdef PLAYBACK_FORCE_48K
        freq = (freq < APP_SYSFREQ_104M)?APP_SYSFREQ_104M:freq;
#endif

#ifdef A2DP_CP_ACCEL
        if (0) {
        } else if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
            freq = APP_SYSFREQ_52M;
        } else {
            freq = APP_SYSFREQ_26M;
        }
#endif

        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, freq);
        TRACE("bt_sbc_player: app_sysfreq_req %d", freq);
        TRACE("sys freq calc : %d\n", hal_sys_timer_calc_cpu_freq(5, 0));

        if (on == PLAYER_OPER_START)
        {
            if (0)
            {
            }
#if defined(A2DP_AAC_ON)
            else if (codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC)
            {
                app_overlay_select(APP_OVERLAY_A2DP_AAC);
            }
#endif
#if defined(A2DP_LHDC_ON)
            else if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
            {
                app_overlay_select(APP_OVERLAY_A2DP_LHDC);
            }
#endif
#if defined(A2DP_SCALABLE_ON)
            else if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
            {
                app_overlay_select(APP_OVERLAY_A2DP_SCALABLE);
            }
#endif
#if defined(A2DP_LDAC_ON)
            else if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
            {
                TRACE("bt_sbc_player ldac overlay select \n"); //toto
                app_overlay_select(APP_OVERLAY_A2DP_LDAC);
            }
#endif
            else
            {
                app_overlay_select(APP_OVERLAY_A2DP);
            }

#ifdef BT_XTAL_SYNC

#ifdef __TWS__
            if(app_tws_mode_is_only_mobile())
            {
                btdrv_rf_bit_offset_track_enable(false);
            }
            else
#endif
            {
                btdrv_rf_bit_offset_track_enable(true);
            }
            bt_init_xtal_sync(BT_XTAL_SYNC_MODE_MUSIC, BT_INIT_XTAL_SYNC_MIN, BT_INIT_XTAL_SYNC_MAX, BT_INIT_XTAL_SYNC_FCAP_RANGE);
#endif // BT_XTAL_SYNC
#ifdef __THIRDPARTY     
            app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO1,THIRDPARTY_START2MIC);
#endif
#if defined(__THIRDPARTY)
            bt_sbc_mode = 1;
#endif
        }

#ifdef __THIRDPARTY
        //app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO1,THIRDPARTY_STOP);
        start_by_sbc = true;
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO1,THIRDPARTY_START);
#endif
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
        sample_rate = AUD_SAMPRATE_50781;
#else
        sample_rate = a2dp_sample_rate;
#endif

        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
#ifdef PLAYBACK_FORCE_48K
        stream_cfg.sample_rate = AUD_SAMPRATE_48000;
#else
        stream_cfg.sample_rate = sample_rate;
#endif

#ifdef FPGA
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#endif
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.vol = stream_local_volume;
        stream_cfg.handler = bt_sbc_player_more_data;

#if defined(A2DP_SCALABLE_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
            ASSERT(BT_AUDIO_BUFF_SIZE >= (8*SCALABLE_FRAME_SIZE), "BT_AUDIO_BUFF_SIZE should >= (8*SCALABLE_FRAME_SIZE)");
            stream_cfg.data_size = SCALABLE_FRAME_SIZE*8;
        }else
#endif
#if defined(A2DP_LHDC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
            if (a2dp_lhdc_config_llc_get()){
                stream_cfg.data_size = LHDC_AUDIO_BUFF_SIZE; //1024;
            }else{
                stream_cfg.data_size = LHDC_AUDIO_BUFF_SIZE;
            }
        }else
#endif
#if defined(A2DP_LDAC_ON)
        if(codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
            stream_cfg.data_size = BT_AUDIO_BUFF_AAC_SIZE;
        else
#endif
#if defined(A2DP_AAC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC)
            stream_cfg.data_size = BT_AUDIO_BUFF_AAC_SIZE;
        else
#endif
        {
            stream_cfg.data_size = BT_AUDIO_BUFF_SBC_SIZE;
        }

        stream_cfg.bits = AUD_BITS_16;

#ifdef A2DP_EQ_24BIT
        stream_cfg.data_size *= 2;
        stream_cfg.bits = AUD_BITS_24;
#elif defined(A2DP_SCALABLE_ON) || defined(A2DP_LHDC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
            stream_cfg.data_size *= 2;
            stream_cfg.bits = AUD_BITS_24;
        }
#endif

        a2dp_data_buf_size = stream_cfg.data_size;

#ifdef VOICE_DATAPATH
        app_voicepath_set_audio_output_sample_rate(stream_cfg.sample_rate);
        app_voicepath_set_audio_output_data_buf_size(stream_cfg.data_size);
#endif

        app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P))
        lowdelay_sample_size_play_bt=stream_cfg.bits;
        lowdelay_sample_rate_play_bt=stream_cfg.sample_rate;
        lowdelay_data_size_play_bt=stream_cfg.data_size;
        lowdelay_playback_ch_num_bt=stream_cfg.channel_num;
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        sample_size_play_bt=stream_cfg.bits;
        sample_rate_play_bt=stream_cfg.sample_rate;
        data_size_play_bt=stream_cfg.data_size;
        playback_buf_bt=stream_cfg.data_ptr;
        playback_size_bt=stream_cfg.data_size;
        playback_samplerate_ratio_bt=8;
        playback_ch_num_bt=stream_cfg.channel_num;
#endif

#ifdef PLAYBACK_FORCE_48K
        force48k_resample= app_force48k_resample_any_open( stream_cfg.channel_num,
                            app_force48k_resample_iter, stream_cfg.data_size / stream_cfg.channel_num,
                            (float)sample_rate / AUD_SAMPRATE_48000);
#endif

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

#ifdef __HEAR_THRU_PEAK_DET__
        PEAK_DETECTOR_CFG_T peak_detector_cfg;
        peak_detector_cfg.fs = stream_cfg.sample_rate;
        peak_detector_cfg.bits = stream_cfg.bits;
        peak_detector_cfg.factor_up = 0.6;
        peak_detector_cfg.factor_down = 2.0;
        peak_detector_cfg.reduce_dB = -30;
        peak_detector_init();
        peak_detector_setup(&peak_detector_cfg);
#endif

#if defined(__AUDIO_SPECTRUM__)
        audio_spectrum_open(stream_cfg.sample_rate, stream_cfg.bits);
#endif

#if defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
        eq_buff_size = stream_cfg.data_size*2;
#elif defined(__HW_FIR_EQ_PROCESS__) && !defined(__HW_IIR_EQ_PROCESS__)

        play_samp_size = (stream_cfg.bits <= AUD_BITS_16) ? 2 : 4;
#if defined(CHIP_BEST2000)
        eq_buff_size = stream_cfg.data_size * sizeof(int32_t) / play_samp_size;
#elif defined(CHIP_BEST1000)
        eq_buff_size = stream_cfg.data_size * sizeof(int16_t) / play_samp_size;
#elif defined(CHIP_BEST2300) || defined(CHIP_BEST2300P)
        eq_buff_size = stream_cfg.data_size;
#endif
#elif !defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
        eq_buff_size = stream_cfg.data_size;
#else
        eq_buff_size = 0;
        bt_eq_buff = NULL;
#endif

        if(eq_buff_size > 0)
        {
            app_audio_mempool_get_buff(&bt_eq_buff, eq_buff_size);
        }

        audio_process_init();
        audio_process_open(stream_cfg.sample_rate, stream_cfg.bits, stream_cfg.channel_num, stream_cfg.data_size/(stream_cfg.bits <= AUD_BITS_16 ? 2 : 4)/2, bt_eq_buff, eq_buff_size);

#ifdef __SW_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_SW_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_SW_IIR,0));
#endif

#ifdef __HW_FIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_FIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_FIR,0));
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_DAC_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_DAC_IIR,0));
#endif

#ifdef __HW_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_IIR,0));
#endif

#if defined(IBRT)
        APP_TWS_IBRT_AUDIO_SYNC_CFG_T sync_config;
        sync_config.factor_reference  = TWS_IBRT_AUDIO_SYNC_FACTOR_REFERENCE;
        sync_config.factor_fast_limit = TWS_IBRT_AUDIO_SYNC_FACTOR_FAST_LIMIT;
        sync_config.factor_slow_limit = TWS_IBRT_AUDIO_SYNC_FACTOR_SLOW_LIMIT;;
        sync_config.dead_zone_us      = TWS_IBRT_AUDIO_SYNC_DEAD_ZONE_US;
        app_tws_ibrt_audio_sync_reconfig(&sync_config);
#else
#ifdef __AUDIO_RESAMPLE__
#ifndef SW_PLAYBACK_RESAMPLE
        af_codec_tune_resample_rate(AUD_STREAM_PLAYBACK, 0);
#endif
#else
        af_codec_tune_pll(0);
#endif
#endif
        if (on == PLAYER_OPER_START)
        {
            // This might use all of the rest buffer in the mempool,
            // so it must be the last configuration before starting stream.
#if (A2DP_DECODER_VER == 2)
            A2DP_AUDIO_OUTPUT_CONFIG_T output_config;
            output_config.sample_rate = sample_rate;    
            output_config.num_channels = 2;
            #ifdef A2DP_EQ_24BIT
            output_config.bits_depth = 24;
            #else
            output_config.bits_depth = 16;
            #endif
            output_config.frame_samples = app_bt_stream_get_dma_buffer_samples()/2;
            output_config.factor_reference = 1.0f;
#if defined(IBRT)
            ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
            uint8_t codec_type = bt_sbc_player_get_codec_type();
            uint16_t dest_packet_mut = 0;

            switch (codec_type)
            {
                case BTIF_AVDTP_CODEC_TYPE_SBC:
                    dest_packet_mut = BT_SBC_PLAYER_PLAYBACK_DELAY_SBC_MTU;
                    break;
                case BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC:
                    dest_packet_mut = BT_SBC_PLAYER_PLAYBACK_DELAY_AAC_MTU;
                    break;
                case BTIF_AVDTP_CODEC_TYPE_NON_A2DP:
                    break;
                default:
                    break;
            }            
            output_config.factor_reference = TWS_IBRT_AUDIO_SYNC_FACTOR_REFERENCE;
            a2dp_audio_init(&output_config, (A2DP_AUDIO_CHANNEL_SELECT_E)p_ibrt_ctrl->audio_chnl_sel, codec_type, dest_packet_mut);

            if (app_tws_ibrt_mobile_link_connected()){
                app_tws_ibrt_audio_analysis_start(0, ANALYSIS_CHECKER_INTERVEL_INVALID);
                app_tws_ibrt_audio_sync_start();
                a2dp_audio_detect_next_packet_callback_register(app_bt_stream_detect_next_packet_cb);
            }else if (app_tws_ibrt_ibrt_link_connected()){
                //do nothing
            }else{
                TRACE("%S mobile_link:%d %04x ibrt_link:%d %04x", __func__, app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
                                                                         app_tws_ibrt_ibrt_link_connected(),   p_ibrt_ctrl->ibrt_conhandle);
            }
#else
            uint8_t codec_type = bt_sbc_player_get_codec_type();
            uint16_t dest_packet_mut = 0;

            switch (codec_type)
            {
                case BTIF_AVDTP_CODEC_TYPE_SBC:
                    dest_packet_mut = BT_SBC_PLAYER_PLAYBACK_DELAY_SBC_MTU;
                    break;
                case BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC:
                    dest_packet_mut = BT_SBC_PLAYER_PLAYBACK_DELAY_AAC_MTU;
                    break;
                case BTIF_AVDTP_CODEC_TYPE_NON_A2DP:
                    break;
                default:
                    break;
            }
            a2dp_audio_init(&output_config, A2DP_AUDIO_CHANNEL_SELECT_STEREO, codec_type, dest_packet_mut);
            a2dp_audio_detect_next_packet_callback_register(app_bt_stream_detect_next_packet_cb);
#endif
            a2dp_audio_start();
#else
            a2dp_audio_init();
#endif
        }

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        stream_cfg.bits = sample_size_play_bt;
        stream_cfg.channel_num = playback_ch_num_bt;
        stream_cfg.sample_rate = sample_rate_play_bt;
        stream_cfg.device = AUD_STREAM_USE_MC;
        stream_cfg.vol = 0;
        stream_cfg.handler = audio_mc_data_playback_a2dp;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;

        app_audio_mempool_get_buff(&bt_audio_buff, data_size_play_bt*8);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
        stream_cfg.data_size = data_size_play_bt*8;

        playback_buf_mc=stream_cfg.data_ptr;
        playback_size_mc=stream_cfg.data_size;

        anc_mc_run_init(hal_codec_anc_convert_rate(sample_rate_play_bt));

        af_stream_open(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg);
        //ASSERT(ret == 0, "af_stream_open playback failed: %d", ret);
#endif

#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P))
        calib_reset = 1;
        af_stream_dma_tc_irq_enable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif
#if defined(IBRT)
        af_stream_dma_tc_irq_enable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif

#ifdef __BT_SBC_PLAYER_USE_BT_TRIGGER__
        app_bt_stream_trigger_init();
#endif
#ifdef VOICE_DATAPATH
        if (app_voicepath_get_stream_state(VOICEPATH_STREAMING))
        {
            app_voicepath_set_pending_started_stream(AUDIO_OUTPUT_STREAMING, true);
        }
        else
        {
            app_voicepath_set_stream_state(AUDIO_OUTPUT_STREAMING, true);
            af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        }
#else
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        af_stream_start(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

#ifdef __THIRDPARTY
        //app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO1,THIRDPARTY_OTHER_EVENT);
#endif
    }

    isRun = (on != PLAYER_OPER_STOP);
    return 0;
}

#if defined(SCO_DMA_SNAPSHOT)
static uint32_t sco_trigger_wait_codecpcm = 0;
static uint32_t sco_trigger_wait_btpcm = 0;
void app_bt_stream_sco_trigger_set_codecpcm_triggle(uint8_t triggle_en)
{
    sco_trigger_wait_codecpcm = triggle_en;
}

uint32_t app_bt_stream_sco_trigger_wait_codecpcm_triggle(void)
{
    return sco_trigger_wait_codecpcm;
}

void app_bt_stream_sco_trigger_set_btpcm_triggle(uint32_t triggle_en)
{
    sco_trigger_wait_btpcm = triggle_en;
}

uint32_t app_bt_stream_sco_trigger_wait_btpcm_triggle(void)
{
    return sco_trigger_wait_btpcm;
}

int app_bt_stream_sco_trigger_codecpcm_tick(void)
{
    if(app_bt_stream_sco_trigger_wait_codecpcm_triggle()){
        app_bt_stream_sco_trigger_set_codecpcm_triggle(0);
#if defined(IBRT)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (app_tws_ibrt_mobile_link_connected()){
        TRACE("[%s]tick:%x/%x", __func__, btdrv_syn_get_curr_ticks(), bt_syn_get_curr_ticks(p_ibrt_ctrl->mobile_conhandle));
    }else if (app_tws_ibrt_ibrt_link_connected()){
        TRACE("[%s]tick:%x/%x", __func__, btdrv_syn_get_curr_ticks(), bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle));
    }else{
        TRACE("%S mobile_link:%d %04x ibrt_link:%d %04x", __func__, app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
                                                                 app_tws_ibrt_ibrt_link_connected(),   p_ibrt_ctrl->ibrt_conhandle);
    }
#else
    uint16_t conhdl = 0xFFFF;
    int curr_sco;

    curr_sco = app_audio_manager_get_active_sco_num();
    if (curr_sco != BT_DEVICE_NUM)
    {
        conhdl = btif_hf_get_remote_hci_handle(*app_audio_manager_get_active_sco_chnl());
    }
    TRACE("[%s] tick:%x", __func__, bt_syn_get_curr_ticks(conhdl));
#endif
        btdrv_syn_clr_trigger();
        return 1;
    }
    return 0;
}

int app_bt_stream_sco_trigger_btpcm_tick(void)
{
    if(app_bt_stream_sco_trigger_wait_btpcm_triggle()){
        app_bt_stream_sco_trigger_set_btpcm_triggle(0);
#if defined(IBRT)
     ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
     if (app_tws_ibrt_mobile_link_connected()){
         TRACE("[%s]tick:%x/%x", __func__, btdrv_syn_get_curr_ticks(), bt_syn_get_curr_ticks(p_ibrt_ctrl->mobile_conhandle));
     }else if (app_tws_ibrt_ibrt_link_connected()){
         TRACE("[%s]tick:%x/%x", __func__, btdrv_syn_get_curr_ticks(), bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle));
     }else{
         TRACE("%S mobile_link:%d %04x ibrt_link:%d %04x", __func__, app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
                                                                  app_tws_ibrt_ibrt_link_connected(),   p_ibrt_ctrl->ibrt_conhandle);
     }
#else
    uint16_t conhdl = 0xFFFF;
    int curr_sco;

    curr_sco = app_audio_manager_get_active_sco_num();
    if (curr_sco != BT_DEVICE_NUM)
    {
        conhdl = btif_hf_get_remote_hci_handle(*app_audio_manager_get_active_sco_chnl());
    }
    TRACE("[%s] tick:%x", __func__, bt_syn_get_curr_ticks(conhdl));
#endif
        btdrv_syn_clr_trigger();
        return 1;
    }
    return 0;
}


void app_bt_stream_sco_trigger_btpcm_start(void )
{
    uint32_t curr_ticks = 0;    
    uint32_t tg_acl_trigger_offset_time = 0;
#if defined(IBRT)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (app_tws_ibrt_mobile_link_connected()){
        curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->mobile_conhandle);
    }else if (app_tws_ibrt_ibrt_link_connected()){
        curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle);
    }else{
        TRACE("%S mobile_link:%d %04x ibrt_link:%d %04x", __func__, app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
                                                                 app_tws_ibrt_ibrt_link_connected(),   p_ibrt_ctrl->ibrt_conhandle);

    }    
    TRACE("app_bt_stream_sco_trigger_btpcm_start get ticks:%d,",curr_ticks);
#else    
    uint16_t conhdl = 0xFFFF;
    int curr_sco;

    curr_sco = app_audio_manager_get_active_sco_num();
    if (curr_sco != BT_DEVICE_NUM)
    {
        conhdl = btif_hf_get_remote_hci_handle(*app_audio_manager_get_active_sco_chnl());
    }
    curr_ticks = bt_syn_get_curr_ticks(conhdl);
#endif
    tg_acl_trigger_offset_time = (curr_ticks+0x180) - ((curr_ticks+0x180)%192);

    btdrv_set_bt_pcm_triggler_en(0);
    btdrv_set_bt_pcm_en(0); 
    btdrv_syn_clr_trigger();
    btdrv_enable_playback_triggler(SCO_TRIGGLE_MODE);

#if defined(IBRT)
    if (app_tws_ibrt_mobile_link_connected()){
        bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, p_ibrt_ctrl->mobile_conhandle, BT_TRIG_SLAVE_ROLE);
        TRACE("app_bt_stream_sco_trigger_btpcm_start set ticks:%d,",tg_acl_trigger_offset_time);  
    }else if (app_tws_ibrt_ibrt_link_connected()){
        bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, p_ibrt_ctrl->ibrt_conhandle, BT_TRIG_SLAVE_ROLE);
        TRACE("app_bt_stream_sco_trigger_btpcm_start set ticks:%d,",tg_acl_trigger_offset_time);  
    }else{
        TRACE("%S mobile_link:%d %04x ibrt_link:%d %04x", __func__, app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
                                                                 app_tws_ibrt_ibrt_link_connected(),   p_ibrt_ctrl->ibrt_conhandle);
    }    
    TRACE("app_bt_stream_sco_trigger_btpcm_start get ticks:%d,",curr_ticks);
    
#else
    bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, conhdl, BT_TRIG_SLAVE_ROLE);
#endif
    btdrv_set_bt_pcm_triggler_en(1);
    btdrv_set_bt_pcm_en(1); 
    app_bt_stream_sco_trigger_set_btpcm_triggle(1);
    TRACE("[%s]enable pcm_trigger curr clk=%x, triggle_clk=%x, bt_clk=%x", __func__,
                                                                       btdrv_syn_get_curr_ticks(), 
                                                                       tg_acl_trigger_offset_time,
                                                                       curr_ticks);



}

void app_bt_stream_sco_trigger_btpcm_stop(void)
{
    return;
}

void app_bt_stream_sco_trigger_codecpcm_start(uint32_t btclk, uint16_t btcnt )
{
    uint32_t curr_ticks = 0;    
    uint32_t tg_acl_trigger_offset_time = 0;


#if defined(IBRT)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (app_tws_ibrt_mobile_link_connected()){
        curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->mobile_conhandle);
    }else if (app_tws_ibrt_ibrt_link_connected()){
        curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle);
    }else{
        TRACE("%S mobile_link:%d %04x ibrt_link:%d %04x", __func__, app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
                                                                 app_tws_ibrt_ibrt_link_connected(),   p_ibrt_ctrl->ibrt_conhandle);
    }    
    TRACE("app_bt_stream_sco_trigger_codecpcm_start get 1 curr_ticks:%d,btclk:%d,btcnt:%d,",curr_ticks,btclk,btcnt);  
#else
    uint16_t conhdl = 0xFFFF;
    int curr_sco;

    curr_sco = app_audio_manager_get_active_sco_num();
    if (curr_sco != BT_DEVICE_NUM)
    {
        conhdl = btif_hf_get_remote_hci_handle(*app_audio_manager_get_active_sco_chnl());
    }
    curr_ticks = bt_syn_get_curr_ticks(conhdl);
#endif

#ifdef PCM_FAST_MODE
#ifdef LOW_DELAY_SCO
    tg_acl_trigger_offset_time=btclk+12+MASTER_MOBILE_BTCLK_OFFSET;
#else
    tg_acl_trigger_offset_time=btclk+12+24+MASTER_MOBILE_BTCLK_OFFSET;
#endif
#else
#ifdef LOW_DELAY_SCO
    tg_acl_trigger_offset_time=btclk+12+MASTER_MOBILE_BTCLK_OFFSET;
#else
    tg_acl_trigger_offset_time=btclk+24+MASTER_MOBILE_BTCLK_OFFSET;
#endif
#endif


    tg_acl_trigger_offset_time = tg_acl_trigger_offset_time * 2;
    tg_acl_trigger_offset_time &= 0x0fffffff;

    btdrv_syn_trigger_codec_en(0);
    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, false);
    af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, false);
    //btdrv_syn_clr_trigger();
    btdrv_enable_playback_triggler(ACL_TRIGGLE_MODE);

#if defined(IBRT)
    if (app_tws_ibrt_mobile_link_connected()){
        bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, p_ibrt_ctrl->mobile_conhandle, BT_TRIG_SLAVE_ROLE);
        TRACE("app_bt_stream_sco_trigger_codecpcm_start set 2 tg_acl_trigger_offset_time:%d",tg_acl_trigger_offset_time);
    }else if (app_tws_ibrt_ibrt_link_connected()){
        bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, p_ibrt_ctrl->ibrt_conhandle, BT_TRIG_SLAVE_ROLE);
        TRACE("app_bt_stream_sco_trigger_codecpcm_start set 2 tg_acl_trigger_offset_time:%d",tg_acl_trigger_offset_time);
    }else{
        TRACE("%S mobile_link:%d %04x ibrt_link:%d %04x", __func__, app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
                                                                 app_tws_ibrt_ibrt_link_connected(),   p_ibrt_ctrl->ibrt_conhandle);
    }    
#else
    bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, conhdl, BT_TRIG_SLAVE_ROLE);
#endif
    btdrv_syn_trigger_codec_en(1);
    app_bt_stream_sco_trigger_set_codecpcm_triggle(1);
    TRACE("[%s]enable pcm_trigger curr clk=%x, triggle_clk=%x, bt_clk=%x", __func__,
                                                                       btdrv_syn_get_curr_ticks(), 
                                                                       tg_acl_trigger_offset_time,
                                                                       curr_ticks);

    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, true);
    af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, true);


}

void app_bt_stream_sco_trigger_codecpcm_stop(void)
{
    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, false);
    af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, false);    
}
#endif

void speech_tx_aec_set_frame_len(int len);
int voicebtpcm_pcm_echo_buf_queue_init(uint32_t size);
void voicebtpcm_pcm_echo_buf_queue_reset(void);
void voicebtpcm_pcm_echo_buf_queue_deinit(void);
int voicebtpcm_pcm_audio_init(int sco_sample_rate, int codec_sample_rate);
int voicebtpcm_pcm_audio_deinit(void);
uint32_t voicebtpcm_pcm_audio_data_come(uint8_t *buf, uint32_t len);
uint32_t voicebtpcm_pcm_audio_more_data(uint8_t *buf, uint32_t len);
int store_voicebtpcm_m2p_buffer(unsigned char *buf, unsigned int len);
int get_voicebtpcm_p2m_frame(unsigned char *buf, unsigned int len);
static uint32_t mic_force_mute = 0;
static uint32_t spk_force_mute = 0;
static uint32_t bt_sco_player_code_type = 0;

static enum AUD_CHANNEL_NUM_T sco_play_chan_num;
static enum AUD_CHANNEL_NUM_T sco_cap_chan_num;

bool bt_sco_codec_is_msbc(void)
{
    bool en = false;
#ifdef HFP_1_6_ENABLE
    if (app_audio_manager_get_scocodecid() == BTIF_HF_SCO_CODEC_MSBC)
    {
        en = true;
    }
    else
#endif
    {
        en = false;
    }

    return en;
}

void bt_sco_mobile_clkcnt_get(uint32_t btclk, uint16_t btcnt,
                                     uint32_t *mobile_master_clk, uint16_t *mobile_master_cnt)
{
#if defined(IBRT)
    app_tws_ibrt_audio_mobile_clkcnt_get(btclk, btcnt, mobile_master_clk, mobile_master_cnt);
#else
    uint16_t conhdl = 0xFFFF;
    int32_t clock_offset;
    uint16_t bit_offset;
    int curr_sco;
    
    curr_sco = app_audio_manager_get_active_sco_num();
    if (curr_sco != BT_DEVICE_NUM){
        conhdl = btif_hf_get_remote_hci_handle(*app_audio_manager_get_active_sco_chnl());
    }

    if (conhdl != 0xFFFF){
        bt_drv_reg_piconet_clk_offset_get(conhdl, &clock_offset, &bit_offset); 
        //TRACE("mobile piconet clk:%d bit:%d loc clk:%d cnt:%d", clock_offset, bit_offset, btclk, btcnt);
        btdrv_slave2master_clkcnt_convert(btclk, btcnt,
                                          clock_offset, bit_offset,
                                          mobile_master_clk, mobile_master_cnt);
    }else{
        TRACE("%s warning conhdl NULL role:%d conhdl:%x", __func__, conhdl);
        *mobile_master_clk = 0;
        *mobile_master_cnt = 0;
    }
#endif
}


#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)

#ifdef CHIP_BEST1000
#error "Unsupport SW_SCO_RESAMPLE on best1000 by now"
#endif
#ifdef NO_SCO_RESAMPLE
#error "Conflicted config: NO_SCO_RESAMPLE and SW_SCO_RESAMPLE"
#endif

// The decoded playback data in the first irq is output to DAC after the second irq (PING-PONG buffer)
#define SCO_PLAY_RESAMPLE_ALIGN_CNT     2

static uint8_t sco_play_irq_cnt;
static bool sco_dma_buf_err;
static struct APP_RESAMPLE_T *sco_capture_resample;
static struct APP_RESAMPLE_T *sco_playback_resample;

static int bt_sco_capture_resample_iter(uint8_t *buf, uint32_t len)
{
    voicebtpcm_pcm_audio_data_come(buf, len);
    return 0;
}

static int bt_sco_playback_resample_iter(uint8_t *buf, uint32_t len)
{
    voicebtpcm_pcm_audio_more_data(buf, len);
    return 0;
}

#endif

//( codec:mic-->btpcm:tx
// codec:mic
static uint32_t bt_sco_codec_capture_data(uint8_t *buf, uint32_t len)
{
#if defined(SCO_DMA_SNAPSHOT)
    return len;
#else
    if (mic_force_mute||btapp_hfp_mic_need_skip_frame())
    {
        memset(buf, 0, len);
    }
    
#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
    if(hal_cmu_get_audio_resample_status())
    {
        if (af_stream_buffer_error(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE)) {
            sco_dma_buf_err = true;
        }
        // The decoded playback data in the first irq is output to DAC after the second irq (PING-PONG buffer),
        // so it is aligned with the capture data after 2 playback irqs.
        if (sco_play_irq_cnt < SCO_PLAY_RESAMPLE_ALIGN_CNT) {
            // Skip processing
            return len;
        }
        app_capture_resample_run(sco_capture_resample, buf, len);
    }
    else
#endif
    {
        voicebtpcm_pcm_audio_data_come(buf, len);
    }

    return len;
#endif  
}

#ifdef _SCO_BTPCM_CHANNEL_
// btpcm:tx
static uint32_t bt_sco_btpcm_playback_data(uint8_t *buf, uint32_t len)
{
#if defined(SCO_DMA_SNAPSHOT)
    return len;
#else
    get_voicebtpcm_p2m_frame(buf, len);

    if (btapp_hfp_need_mute())
    {
        memset(buf, 0, len);
    }
    return len;
#endif
}
//)
extern CQueue* get_rx_esco_queue_ptr();

static volatile bool is_codec_stream_started = false;

//( btpcm:rx-->codec:spk
// btpcm:rx

static uint32_t bt_sco_btpcm_capture_data(uint8_t *buf, uint32_t len)
{
#if defined(SCO_DMA_SNAPSHOT)
/*
    uint32_t curr_ticks;

    ibbt_ctrl_t *p_ibbt_ctrl = app_tws_ibbt_get_bt_ctrl_ctx();
    if (app_tws_ibbt_tws_link_connected()){
        curr_ticks = bt_syn_get_curr_ticks(IBBT_MASTER == p_ibbt_ctrl->current_role ? p_ibbt_ctrl->mobile_conhandle : p_ibbt_ctrl->ibbt_conhandle);
         TRACE("bt_sco_btpcm_capture_data +++++++++++++++++++++++++++++++++curr_ticks:%d,",curr_ticks);
    }else{            
        curr_ticks = btdrv_syn_get_curr_ticks();
     TRACE("--------------------------------------");
    }
*/
    if(app_bt_stream_sco_trigger_btpcm_tick())
    {   
        uint32_t btclk;
        uint16_t btcnt;

        uint32_t mobile_master_clk;
        uint16_t mobile_master_cnt;
        
        //uint16_t *source=(uint16_t *)buf;
        //DUMP16("%02x,", source, MSBC_FRAME_LEN);
        bt_drv_reg_dma_tc_clkcnt_get(&btclk, &btcnt);
        bt_sco_mobile_clkcnt_get(btclk, btcnt,&mobile_master_clk, &mobile_master_cnt);
        mobile_master_clk_offset_init=mobile_master_clk%12;
        app_bt_stream_sco_trigger_codecpcm_start(mobile_master_clk,mobile_master_cnt);
        af_stream_dma_tc_irq_enable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        //af_stream_dma_tc_irq_disable(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
           af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        is_codec_stream_started = true;
           TRACE("bt_sco_btpcm_capture_data:btclk:%d,btcnt:%d,",mobile_master_clk,mobile_master_cnt);
           
    } 
    else
    {
        uint32_t btclk;
        uint16_t btcnt;

        uint32_t mobile_master_clk;
        uint16_t mobile_master_cnt;

        bt_drv_reg_dma_tc_clkcnt_get(&btclk, &btcnt);
        bt_sco_mobile_clkcnt_get(btclk, btcnt,&mobile_master_clk, &mobile_master_cnt);
        TRACE("bt_sco_btpcm_capture_data:btclk:%d,btcnt:%d,",mobile_master_clk,mobile_master_cnt);
    }

    return len;
    
#else
    if(!is_sco_mode()){
        TRACE("sco player exit!");
        memset(buf,0x0,len);
        return len;
    }

#if defined(TX_RX_PCM_MASK)
    TRACE("TX_RX_PCM_MASK");
    CQueue* Rx_esco_queue_temp = NULL;
    Rx_esco_queue_temp = get_rx_esco_queue_ptr();
    if(bt_sco_codec_is_msbc() && btdrv_is_pcm_mask_enable() ==1 )
    {
        memset(buf,0,len);        
        int status = 0;
        len /= 2;
        uint8_t rx_data[len];
        status = DeCQueue(Rx_esco_queue_temp,rx_data,len);
        for(uint32_t i = 0; i<len; i++)
        {
            buf[2*i+1] = rx_data[i];
        }
        len*=2;
        if(status)
        {
            TRACE("Rx Dec Fail");
         }
    }
#endif

    if (is_codec_stream_started == false) {
        if (bt_sco_codec_is_msbc() == false)
            hal_sys_timer_delay_us(3000);

        TRACE("[%s] start codec %d", __FUNCTION__, FAST_TICKS_TO_US(hal_fast_sys_timer_get()));
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        af_stream_start(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

        is_codec_stream_started = true;

        return len;
    }
    store_voicebtpcm_m2p_buffer(buf, len);
    return len;
#endif  
}
#endif

#ifdef __BT_ANC__
static void bt_anc_sco_down_sample_16bits(int16_t *dst, int16_t *src, uint32_t dst_cnt)
{
    for (uint32_t i = 0; i < dst_cnt; i++) {
        dst[i] = src[i * bt_sco_samplerate_ratio * sco_play_chan_num];
    }
}
#endif
//uint8_t  app_hfp_get_chnl_via_remDev(HfChannel ** p_hfp_chnl);
uint8_t  app_hfp_get_chnl_via_remDev(hf_chan_handle_t * p_hfp_chnl);

#if defined(SCO_DMA_SNAPSHOT)
extern int process_downlink_bt_voice_frames(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf,uint32_t out_len,int32_t codec_type);
extern int process_uplink_bt_voice_frames(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf,uint32_t out_len,int32_t codec_type);
#define MSBC_FRAME_LEN (60)
#define PCM_LEN_PER_FRAME (240)

static void bt_sco_codec_tuning(void)
{
    uint32_t btclk;
    uint16_t btcnt;

    uint32_t mobile_master_clk;
    uint16_t mobile_master_cnt;

    uint32_t mobile_master_clk_offset;
    int32_t mobile_master_cnt_offset;    

    static float fre_offset=0.0f;
    static int32_t mobile_master_cnt_offset_init;
    static int32_t mobile_master_cnt_offset_old;

#ifdef  __AUDIO_RESAMPLE__
    static uint32_t frame_counter=0;
    static int32_t mobile_master_cnt_offset_max=0;
    static int32_t mobile_master_cnt_offset_min=0;

    static int32_t mobile_master_cnt_offset_resample=0;

	
     int32_t offset_max=0;
     int32_t offset_min=0;	
#endif

    bt_drv_reg_dma_tc_clkcnt_get(&btclk, &btcnt);
    bt_sco_mobile_clkcnt_get(btclk, btcnt,
                                             &mobile_master_clk, &mobile_master_cnt);

    TRACE("bt_sco_codec_capture_playback_data:btclk:%d,btcnt:%d,",mobile_master_clk,mobile_master_cnt);

    mobile_master_clk_offset=(mobile_master_clk-mobile_master_clk_offset_init)%12;
    mobile_master_cnt_offset=mobile_master_clk_offset*625+(625-mobile_master_cnt);
    mobile_master_cnt_offset=mobile_master_cnt_offset-(MASTER_MOBILE_BTCNT_OFFSET+mobile_master_cnt_offset_init);

    if(app_bt_stream_sco_trigger_codecpcm_tick())
    {
	    fre_offset=0.0f;
	    if(mobile_master_clk_offset<MASTER_MOBILE_BTCLK_OFFSET)
	    {
	        mobile_master_cnt_offset_init=-mobile_master_cnt;
	    }
	    else
	    {
	        mobile_master_cnt_offset_init=625-mobile_master_cnt;
	    }
	    mobile_master_cnt_offset=0;
	    mobile_master_cnt_offset_old=0;

#ifdef  ANC_APP
           if(playback_samplerate_codecpcm==AUD_SAMPRATE_16000)
           {
		 mobile_master_cnt_offset_init=-232;
           }
           else if(playback_samplerate_codecpcm==AUD_SAMPRATE_8000)
           {
		 mobile_master_cnt_offset_init=-512;
           }		   
#endif
		
#ifdef  __AUDIO_RESAMPLE__
    	    frame_counter=0;
	    mobile_master_cnt_offset_max=0;
	    mobile_master_cnt_offset_min=0;
           mobile_master_cnt_offset_resample=0;
#endif
    }


#if defined(  __AUDIO_RESAMPLE__) &&!defined(SW_PLAYBACK_RESAMPLE)
   if(playback_samplerate_codecpcm==AUD_SAMPRATE_16000)
   {
	offset_max=28;
	offset_min=-33;
   }
   else if(playback_samplerate_codecpcm==AUD_SAMPRATE_8000)
   {
	offset_max=12;
	offset_min=-112;
   }

   if(mobile_master_cnt_offset>mobile_master_cnt_offset_max)
   {
   	mobile_master_cnt_offset_max=mobile_master_cnt_offset;
   }
   
   if(mobile_master_cnt_offset<mobile_master_cnt_offset_min)
   {
   	mobile_master_cnt_offset_min=mobile_master_cnt_offset;
   }
   
    frame_counter++;
   
    if(frame_counter>=22)
    {
       if(mobile_master_cnt_offset_min<offset_min)
       {
            mobile_master_cnt_offset_resample=mobile_master_cnt_offset_min-offset_min;
       }
       else if(mobile_master_cnt_offset_max>offset_max)
       {
           mobile_master_cnt_offset_resample=mobile_master_cnt_offset_max-offset_max;
       }
       else
       {
           mobile_master_cnt_offset_resample=0;
       }     
      TRACE("mobile_master_cnt_offset_min:%d,mobile_master_cnt_offset_max:%d",mobile_master_cnt_offset_min,mobile_master_cnt_offset_max);	 	   
       mobile_master_cnt_offset=mobile_master_cnt_offset_resample;

	fre_offset=fre_offset+((int32_t)(mobile_master_cnt_offset*0.5f))*0.0000001f
			+(mobile_master_cnt_offset-mobile_master_cnt_offset_old)*0.0000001f;

	mobile_master_cnt_offset_old=mobile_master_cnt_offset;	   
	
       TRACE("mobile_master_cnt_offset:%d",mobile_master_cnt_offset);	
	   
       mobile_master_cnt_offset_max=0;
       mobile_master_cnt_offset_min=0;
       frame_counter=0;	
    }

#else
    fre_offset=fre_offset+((int32_t)(mobile_master_cnt_offset*0.5f))*0.00000001f
		+(mobile_master_cnt_offset-mobile_master_cnt_offset_old)*0.00000001f;

    mobile_master_cnt_offset_old=mobile_master_cnt_offset;
	
   TRACE("mobile_master_cnt_offset:%d",mobile_master_cnt_offset);	
#endif

   TRACE("fre_offset:%d",(int)(fre_offset*10000000.0f));

    if(fre_offset>0.0001f)fre_offset=0.0001f;
    if(fre_offset<-0.0001f)fre_offset=-0.0001f;

#ifdef  __AUDIO_RESAMPLE__
#ifdef SW_PLAYBACK_RESAMPLE
    app_resample_tune(playback_samplerate_codecpcm, fre_offset);
#else
    af_codec_tune_both_resample_rate(fre_offset);
    //af_codec_tune_resample_rate(AUD_STREAM_PLAYBACK, fre_offset);
    //af_codec_tune_resample_rate(AUD_STREAM_CAPTURE, fre_offset);
#endif
#else
    af_codec_tune_pll(fre_offset);
#endif

    return;
}
#endif

static uint32_t bt_sco_codec_playback_data(uint8_t *buf, uint32_t len)
{
#if defined(SCO_DMA_SNAPSHOT)
    int pingpang;

    //TRACE("pcm length:%d",len);

    //processing clock
    bt_sco_codec_tuning();

    //processing  ping pang flag
    if(buf==playback_buf_codecpcm)
    {
     pingpang=0;
    }
    else
    {
     pingpang=1;
    }

    //processing spk
    uint8_t *playbakce_pcm_frame_p=playback_buf_codecpcm+pingpang*(playback_size_codecpcm)/2;
    uint16_t *source=(uint16_t *)(capture_buf_btpcm+(pingpang)*capture_size_btpcm/2);
#if defined(HFP_1_6_ENABLE)
    process_downlink_bt_voice_frames((uint8_t *)source,(playback_size_btpcm)/2,
            playbakce_pcm_frame_p,(playback_size_codecpcm)/2,app_audio_manager_get_scocodecid());
#else
    process_downlink_bt_voice_frames((uint8_t *)source,(playback_size_btpcm)/2,
            playbakce_pcm_frame_p,(playback_size_codecpcm)/2,BTIF_HF_SCO_CODEC_CVSD);
#endif
    
    //processing mic
    uint8_t *capture_pcm_frame_p=capture_buf_codecpcm+pingpang*(capture_size_codecpcm)/2;
    uint8_t *dst=(uint8_t *)(playback_buf_btpcm+(pingpang)*playback_size_btpcm/2);

#if defined(HFP_1_6_ENABLE)
    process_uplink_bt_voice_frames(capture_pcm_frame_p,(capture_size_codecpcm)/2,
        dst,(capture_size_btpcm)/2,app_audio_manager_get_scocodecid());
#else
    process_uplink_bt_voice_frames(capture_pcm_frame_p,(capture_size_codecpcm)/2,
        dst,(capture_size_btpcm)/2,BTIF_HF_SCO_CODEC_CVSD);
#endif
    
    return len;
    
#else
    uint8_t *dec_buf;
    uint32_t mono_len;
    
#ifdef BT_XTAL_SYNC
#if !defined(IBRT)
#ifdef BT_XTAL_SYNC_NEW_METHOD
    //HfChannel * hf_chnl = NULL;
    hf_chan_handle_t hf_chnl = NULL;
    app_hfp_get_chnl_via_remDev(&hf_chnl);
    if(hf_chnl != NULL)
    {
        btif_hci_handle_t hci_handle = btif_hf_get_remote_hci_handle(hf_chnl);

        //uint32_t bitoffset = btdrv_rf_bitoffset_get(hf_chnl->cmgrHandler.remDev->hciHandle-0x80);
        uint32_t bitoffset = btdrv_rf_bitoffset_get((uint16_t)hci_handle-0x80);
        bt_xtal_sync_new(bitoffset,BT_XTAL_SYNC_MODE_WITH_MOBILE);
    }
#else
    bt_xtal_sync(BT_XTAL_SYNC_MODE_VOICE);
#endif
#endif
#endif

#ifdef __BT_ANC__
    mono_len = len / sco_play_chan_num / bt_sco_samplerate_ratio;
    dec_buf = bt_anc_sco_dec_buf;
#else
    mono_len = len / sco_play_chan_num;
    dec_buf = buf;
#endif

#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
    if(hal_cmu_get_audio_resample_status())
    {
        if (sco_play_irq_cnt < SCO_PLAY_RESAMPLE_ALIGN_CNT) {
            sco_play_irq_cnt++;
        }
        if (sco_dma_buf_err || af_stream_buffer_error(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK)) {
            sco_dma_buf_err = false;
            sco_play_irq_cnt = 0;
            app_resample_reset(sco_playback_resample);
            app_resample_reset(sco_capture_resample);
            voicebtpcm_pcm_echo_buf_queue_reset();
            TRACE("%s: DMA buffer error: reset resample", __func__);
        }
        app_playback_resample_run(sco_playback_resample, dec_buf, mono_len);
    }
    else
#endif
    {
#ifdef __BT_ANC__
        bt_anc_sco_down_sample_16bits((int16_t *)dec_buf, (int16_t *)buf, mono_len / 2);
#else
        if (sco_play_chan_num == AUD_CHANNEL_NUM_2) {
            // Convert stereo data to mono data (to save into echo_buf)
            app_bt_stream_copy_track_two_to_one_16bits((int16_t *)dec_buf, (int16_t *)buf, mono_len / 2);
        }
#endif
        voicebtpcm_pcm_audio_more_data(dec_buf, mono_len);
    }

#ifdef __BT_ANC__
    voicebtpcm_pcm_resample((int16_t *)dec_buf, mono_len / 2, (int16_t *)buf);
#endif

    if (sco_play_chan_num == AUD_CHANNEL_NUM_2) {
        // Convert mono data to stereo data
        app_bt_stream_copy_track_one_to_two_16bits((int16_t *)buf, (int16_t *)buf, len / 2 / 2);
    }

    app_ring_merge_more_data(buf, len);

    if (spk_force_mute)
    {
        memset(buf, 0, len);
    }

    return len;
#endif
}

int bt_sco_player_forcemute(bool mic_mute, bool spk_mute)
{
    mic_force_mute = mic_mute;
    spk_force_mute = spk_mute;
    return 0;
}

int bt_sco_player_get_codetype(void)
{
    if (gStreamplayer & APP_BT_STREAM_HFP_PCM)
    {
        return bt_sco_player_code_type;
    }
    else
    {
        return 0;
    }
}

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
static uint32_t audio_mc_data_playback_sco(uint8_t *buf, uint32_t mc_len_bytes)
{
  // uint32_t begin_time;
   //uint32_t end_time;
  // begin_time = hal_sys_timer_get();
  // TRACE("phone cancel: %d",begin_time);

   float left_gain;
   float right_gain;
   int32_t playback_len_bytes,mc_len_bytes_8;
   int32_t i,j,k;
   int delay_sample;

   mc_len_bytes_8=mc_len_bytes/8;

   hal_codec_get_dac_gain(&left_gain,&right_gain);

   TRACE("playback_samplerate_ratio:  %d,ch:%d,sample_size:%d.",playback_samplerate_ratio_bt,playback_ch_num_bt,sample_size_play_bt);
   TRACE("mc_len_bytes:  %d",mc_len_bytes);

  // TRACE("left_gain:  %d",(int)(left_gain*(1<<12)));
  // TRACE("right_gain: %d",(int)(right_gain*(1<<12)));

   playback_len_bytes=mc_len_bytes/playback_samplerate_ratio_bt;

    if (sample_size_play_bt == AUD_BITS_16)
    {
        int16_t *sour_p=(int16_t *)(playback_buf_bt+playback_size_bt/2);
        int16_t *mid_p=(int16_t *)(buf);
        int16_t *mid_p_8=(int16_t *)(buf+mc_len_bytes-mc_len_bytes_8);
        int16_t *dest_p=(int16_t *)buf;

        if(buf == playback_buf_mc)
        {
            sour_p=(int16_t *)playback_buf_bt;
        }

        if(playback_ch_num_bt==AUD_CHANNEL_NUM_2)
        {
            delay_sample=DELAY_SAMPLE_MC;

            for(i=0,j=0;i<delay_sample;i=i+2)
            {
                mid_p[j++]=delay_buf_bt[i];
                mid_p[j++]=delay_buf_bt[i+1];
            }

            for(i=0;i<playback_len_bytes/2-delay_sample;i=i+2)
            {
                mid_p[j++]=sour_p[i];
                mid_p[j++]=sour_p[i+1];
            }

            for(j=0;i<playback_len_bytes/2;i=i+2)
            {
                delay_buf_bt[j++]=sour_p[i];
                delay_buf_bt[j++]=sour_p[i+1];
            }

            if(playback_samplerate_ratio_bt<=8)
            {
                for(i=0,j=0;i<playback_len_bytes/2;i=i+2*(8/playback_samplerate_ratio_bt))
                {
                    mid_p_8[j++]=mid_p[i];
                    mid_p_8[j++]=mid_p[i+1];
                }
            }
            else
            {
                for(i=0,j=0;i<playback_len_bytes/2;i=i+2)
                {
                    for(k=0;k<playback_samplerate_ratio_bt/8;k++)
                    {
                        mid_p_8[j++]=mid_p[i];
                        mid_p_8[j++]=mid_p[i+1];
                    }
                }
            }

            anc_mc_run_stereo((uint8_t *)mid_p_8,mc_len_bytes_8,left_gain,right_gain,AUD_BITS_16);

            for(i=0,j=0;i<(mc_len_bytes_8)/2;i=i+2)
            {
                for(k=0;k<8;k++)
                {
                    dest_p[j++]=mid_p_8[i];
                    dest_p[j++]=mid_p_8[i+1];
                }
            }

        }
        else if(playback_ch_num_bt==AUD_CHANNEL_NUM_1)
        {
            delay_sample=DELAY_SAMPLE_MC/2;

            for(i=0,j=0;i<delay_sample;i=i+1)
            {
                mid_p[j++]=delay_buf_bt[i];
            }

            for(i=0;i<playback_len_bytes/2-delay_sample;i=i+1)
            {
                mid_p[j++]=sour_p[i];
            }

            for(j=0;i<playback_len_bytes/2;i=i+1)
            {
                delay_buf_bt[j++]=sour_p[i];
            }

            if(playback_samplerate_ratio_bt<=8)
            {
                for(i=0,j=0;i<playback_len_bytes/2;i=i+1*(8/playback_samplerate_ratio_bt))
                {
                    mid_p_8[j++]=mid_p[i];
                }
            }
            else
            {
                for(i=0,j=0;i<playback_len_bytes/2;i=i+1)
                {
                    for(k=0;k<playback_samplerate_ratio_bt/8;k++)
                    {
                        mid_p_8[j++]=mid_p[i];
                    }
                }
            }

            anc_mc_run_mono((uint8_t *)mid_p_8,mc_len_bytes_8,left_gain,AUD_BITS_16);

            for(i=0,j=0;i<(mc_len_bytes_8)/2;i=i+1)
            {
                for(k=0;k<8;k++)
                {
                    dest_p[j++]=mid_p_8[i];
                }
            }
        }

    }
    else if (sample_size_play_bt == AUD_BITS_24)
    {
        int32_t *sour_p=(int32_t *)(playback_buf_bt+playback_size_bt/2);
        int32_t *mid_p=(int32_t *)(buf);
        int32_t *mid_p_8=(int32_t *)(buf+mc_len_bytes-mc_len_bytes_8);
        int32_t *dest_p=(int32_t *)buf;

        if(buf == playback_buf_mc)
        {
            sour_p=(int32_t *)playback_buf_bt;
        }

        if(playback_ch_num_bt==AUD_CHANNEL_NUM_2)
        {
            delay_sample=DELAY_SAMPLE_MC;

            for(i=0,j=0;i<delay_sample;i=i+2)
            {
                mid_p[j++]=delay_buf_bt[i];
                mid_p[j++]=delay_buf_bt[i+1];
            }

            for(i=0;i<playback_len_bytes/4-delay_sample;i=i+2)
            {
                mid_p[j++]=sour_p[i];
                mid_p[j++]=sour_p[i+1];
            }

            for(j=0;i<playback_len_bytes/4;i=i+2)
            {
                delay_buf_bt[j++]=sour_p[i];
                delay_buf_bt[j++]=sour_p[i+1];
            }

            if(playback_samplerate_ratio_bt<=8)
            {
                for(i=0,j=0;i<playback_len_bytes/4;i=i+2*(8/playback_samplerate_ratio_bt))
                {
                    mid_p_8[j++]=mid_p[i];
                    mid_p_8[j++]=mid_p[i+1];
                }
            }
            else
            {
                for(i=0,j=0;i<playback_len_bytes/4;i=i+2)
                {
                    for(k=0;k<playback_samplerate_ratio_bt/8;k++)
                    {
                        mid_p_8[j++]=mid_p[i];
                        mid_p_8[j++]=mid_p[i+1];
                    }
                }
            }

            anc_mc_run_stereo((uint8_t *)mid_p_8,mc_len_bytes_8,left_gain,right_gain,AUD_BITS_16);

            for(i=0,j=0;i<(mc_len_bytes_8)/4;i=i+2)
            {
                for(k=0;k<8;k++)
                {
                    dest_p[j++]=mid_p_8[i];
                    dest_p[j++]=mid_p_8[i+1];
                }
            }

        }
        else if(playback_ch_num_bt==AUD_CHANNEL_NUM_1)
        {
            delay_sample=DELAY_SAMPLE_MC/2;

            for(i=0,j=0;i<delay_sample;i=i+1)
            {
                mid_p[j++]=delay_buf_bt[i];
            }

            for(i=0;i<playback_len_bytes/4-delay_sample;i=i+1)
            {
                mid_p[j++]=sour_p[i];
            }

            for(j=0;i<playback_len_bytes/4;i=i+1)
            {
                delay_buf_bt[j++]=sour_p[i];
            }

            if(playback_samplerate_ratio_bt<=8)
            {
                for(i=0,j=0;i<playback_len_bytes/4;i=i+1*(8/playback_samplerate_ratio_bt))
                {
                    mid_p_8[j++]=mid_p[i];
                }
            }
            else
            {
                for(i=0,j=0;i<playback_len_bytes/4;i=i+1)
                {
                    for(k=0;k<playback_samplerate_ratio_bt/8;k++)
                    {
                        mid_p_8[j++]=mid_p[i];
                    }
                }
            }

            anc_mc_run_mono((uint8_t *)mid_p_8,mc_len_bytes_8,left_gain,AUD_BITS_16);

            for(i=0,j=0;i<(mc_len_bytes_8)/4;i=i+1)
            {
                for(k=0;k<8;k++)
                {
                    dest_p[j++]=mid_p_8[i];
                }
            }
        }

    }

  //  end_time = hal_sys_timer_get();

 //   TRACE("%s:run time: %d", __FUNCTION__, end_time-begin_time);

    return 0;
}
#endif

#if defined(LOW_DELAY_SCO)
int speech_get_frame_size(int fs, int ch, int ms)
{
    return (fs / 1000 * ch * ms)/2;
}
#else
int speech_get_frame_size(int fs, int ch, int ms)
{
    return (fs / 1000 * ch * ms);
}
#endif


int speech_get_af_buffer_size(int fs, int ch, int ms)
{
    return speech_get_frame_size(fs, ch, ms) * 2 * 2;
}

enum AUD_SAMPRATE_T speech_sco_get_sample_rate(void)
{
    enum AUD_SAMPRATE_T sample_rate;

#if defined(HFP_1_6_ENABLE)
    if (bt_sco_codec_is_msbc())
    {
        sample_rate = AUD_SAMPRATE_16000;
    }
    else
#endif
    {
        sample_rate = AUD_SAMPRATE_8000;
    }

    return sample_rate;
}

enum AUD_SAMPRATE_T speech_codec_get_sample_rate(void)
{
    enum AUD_SAMPRATE_T sample_rate;

#if defined(MSBC_8K_SAMPLE_RATE)
    sample_rate = AUD_SAMPRATE_8000;
#else
    if (bt_sco_codec_is_msbc())
    {

        sample_rate = AUD_SAMPRATE_16000;
    }
    else
    {
        sample_rate = AUD_SAMPRATE_8000;
    }
#endif

    return sample_rate;
}
extern "C" void ble_stop_activities(void);
int app_bt_stream_volumeset(int8_t vol);

int bt_sco_player(bool on, enum APP_SYSFREQ_FREQ_T freq)
{
    struct AF_STREAM_CONFIG_T stream_cfg;
    static bool isRun =  false;
    uint8_t * bt_audio_buff = NULL;
    enum AUD_SAMPRATE_T sample_rate;

    TRACE("bt_sco_player work:%d op:%d freq:%d", isRun, on, freq);

#ifdef CHIP_BEST2000
    btdrv_enable_one_packet_more_head(0);
#endif

    if (isRun==on)
        return 0;

    if (on)
    {
    #ifdef __IAG_BLE_INCLUDE__
        ble_stop_activities();
    #endif

#if defined(PCM_FAST_MODE) && defined(CHIP_BEST2300P)
        btdrv_open_pcm_fast_mode_enable();
#endif

#ifdef __THIRDPARTY
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO1,THIRDPARTY_STOP);
		app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO2,THIRDPARTY_MIC_OPEN); 
#endif
        //bt_syncerr set to max(0x0a)
//        BTDIGITAL_REG_SET_FIELD(REG_BTCORE_BASE_ADDR, 0x0f, 0, 0x0f);
//        af_set_priority(osPriorityRealtime);
        af_set_priority(osPriorityHigh);
        bt_media_volume_ptr_update_by_mediatype(BT_STREAM_VOICE);
        stream_local_volume = btdevice_volume_p->hfp_vol;
        app_audio_manager_sco_status_checker();

#if defined(HFP_1_6_ENABLE)
        bt_sco_player_code_type = app_audio_manager_get_scocodecid();
#endif

        if (freq < APP_SYSFREQ_104M)
        {
            freq = APP_SYSFREQ_104M;
        }

#if defined(SPEECH_TX_AEC2FLOAT)
        if (freq < APP_SYSFREQ_208M) {
            freq = APP_SYSFREQ_208M;
        }
#endif

#if defined(SPEECH_TX_2MIC_NS3)
        if (freq < APP_SYSFREQ_208M)
        {
            freq = APP_SYSFREQ_208M;
        }
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        if (freq < APP_SYSFREQ_208M) {
            freq = APP_SYSFREQ_208M;
        }
#endif
        app_sysfreq_req(APP_SYSFREQ_USER_BT_SCO, freq);
        //TRACE("bt_sco_player: app_sysfreq_req %d", freq);
        //TRACE("sys freq calc : %d\n", hal_sys_timer_calc_cpu_freq(5, 0));

#ifndef FPGA
        app_overlay_select(APP_OVERLAY_HFP);
#ifdef BT_XTAL_SYNC
        bt_init_xtal_sync(BT_XTAL_SYNC_MODE_VOICE, BT_INIT_XTAL_SYNC_MIN, BT_INIT_XTAL_SYNC_MAX, BT_INIT_XTAL_SYNC_FCAP_RANGE);
#endif
#endif
        btdrv_rf_bit_offset_track_enable(true);

#if !defined(SCO_DMA_SNAPSHOT)      
        int aec_frame_len = speech_get_frame_size(speech_codec_get_sample_rate(), 1, SPEECH_SCO_FRAME_MS);
        speech_tx_aec_set_frame_len(aec_frame_len);
#endif

        bt_sco_player_forcemute(false, false);

        bt_sco_mode = 1;

        app_audio_mempool_init();

#ifndef _SCO_BTPCM_CHANNEL_
        memset(&hf_sendbuff_ctrl, 0, sizeof(hf_sendbuff_ctrl));
#endif

        sample_rate = speech_codec_get_sample_rate();

        sco_cap_chan_num = (enum AUD_CHANNEL_NUM_T)SPEECH_CODEC_CAPTURE_CHANNEL_NUM;
        memset(&stream_cfg, 0, sizeof(stream_cfg));

        // codec:mic
        stream_cfg.channel_num = sco_cap_chan_num;
        stream_cfg.data_size = speech_get_af_buffer_size(sample_rate, sco_cap_chan_num, SPEECH_SCO_FRAME_MS);

#if defined(__AUDIO_RESAMPLE__) && defined(NO_SCO_RESAMPLE)
        // When __AUDIO_RESAMPLE__ is defined,
        // resample is off by default on best1000, and on by default on other platforms
#ifndef CHIP_BEST1000
        hal_cmu_audio_resample_disable();
#endif
#endif

#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
        if (sample_rate == AUD_SAMPRATE_8000)
        {
            stream_cfg.sample_rate = AUD_SAMPRATE_8463;
        }
        else if (sample_rate == AUD_SAMPRATE_16000)
        {
            stream_cfg.sample_rate = AUD_SAMPRATE_16927;
        }
#ifdef RESAMPLE_ANY_SAMPLE_RATE
        sco_capture_resample = app_capture_resample_any_open( stream_cfg.channel_num,
                            bt_sco_capture_resample_iter, stream_cfg.data_size / 2,
                            (float)CODEC_FREQ_26M / CODEC_FREQ_24P576M);
#else
        sco_capture_resample = app_capture_resample_open(sample_rate, stream_cfg.channel_num,
                            bt_sco_capture_resample_iter, stream_cfg.data_size / 2);
#endif
        uint32_t mono_cap_samp_cnt = stream_cfg.data_size / 2 / 2 / stream_cfg.channel_num;
        uint32_t cap_irq_cnt_per_frm = ((mono_cap_samp_cnt * stream_cfg.sample_rate + (sample_rate - 1)) / sample_rate +
            (aec_frame_len - 1)) / aec_frame_len;
        if (cap_irq_cnt_per_frm == 0) {
            cap_irq_cnt_per_frm = 1;
        }
#else
        stream_cfg.sample_rate = sample_rate;
#endif

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.vol = stream_local_volume;

#ifdef FPGA
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#endif
        stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
        stream_cfg.handler = bt_sco_codec_capture_data;
        app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

#if defined(SCO_DMA_SNAPSHOT)
    capture_buf_codecpcm=stream_cfg.data_ptr;
    capture_size_codecpcm=stream_cfg.data_size;
#endif
        TRACE("capture sample_rate:%d, data_size:%d",stream_cfg.sample_rate,stream_cfg.data_size);

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

#if defined(HW_DC_FILTER_WITH_IIR)
        hw_filter_codec_iir_st = hw_filter_codec_iir_create(stream_cfg.sample_rate, stream_cfg.channel_num, stream_cfg.bits, &adc_iir_cfg);
#endif

#if defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST1400)
    #if defined(SCO_DMA_SNAPSHOT) && (defined(CHIP_BEST2300P)||defined(CHIP_BEST1400))
        btdrv_set_bt_pcm_triggler_delay(2);
    #else
        btdrv_set_bt_pcm_triggler_delay(59);
    #endif
#else
        btdrv_set_bt_pcm_triggler_delay(55);
#endif
        // codec:spk
        sample_rate = speech_codec_get_sample_rate();
#if defined(CHIP_BEST1000)
        sco_play_chan_num = AUD_CHANNEL_NUM_2;
#else
     sco_play_chan_num = AUD_CHANNEL_NUM_1;
#endif
        stream_cfg.channel_num = sco_play_chan_num;
        // stream_cfg.data_size = BT_AUDIO_SCO_BUFF_SIZE * stream_cfg.channel_num;
        stream_cfg.data_size = speech_get_af_buffer_size(sample_rate, sco_play_chan_num, SPEECH_SCO_FRAME_MS);
#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
        if (sample_rate == AUD_SAMPRATE_8000)
        {
            stream_cfg.sample_rate = AUD_SAMPRATE_8463;
        }
        else if (sample_rate == AUD_SAMPRATE_16000)
        {
            stream_cfg.sample_rate = AUD_SAMPRATE_16927;
        }
#ifdef RESAMPLE_ANY_SAMPLE_RATE
        sco_playback_resample = app_playback_resample_any_open( AUD_CHANNEL_NUM_1,
                            bt_sco_playback_resample_iter, stream_cfg.data_size / stream_cfg.channel_num / 2,
                            (float)CODEC_FREQ_24P576M / CODEC_FREQ_26M);
#else
        sco_playback_resample = app_playback_resample_open(sample_rate, AUD_CHANNEL_NUM_1,
                            bt_sco_playback_resample_iter, stream_cfg.data_size / stream_cfg.channel_num / 2);
#endif
        sco_play_irq_cnt = 0;
        sco_dma_buf_err = false;

        uint32_t mono_play_samp_cnt = stream_cfg.data_size / 2 / 2 / stream_cfg.channel_num;
        uint32_t play_irq_cnt_per_frm = ((mono_play_samp_cnt * stream_cfg.sample_rate + (sample_rate - 1)) / sample_rate +
            (aec_frame_len - 1)) / aec_frame_len;
        if (play_irq_cnt_per_frm == 0) {
            play_irq_cnt_per_frm = 1;
        }
        uint32_t play_samp_cnt_per_frm = mono_play_samp_cnt * play_irq_cnt_per_frm;
        uint32_t cap_samp_cnt_per_frm = mono_cap_samp_cnt * cap_irq_cnt_per_frm;
        uint32_t max_samp_cnt_per_frm = (play_samp_cnt_per_frm >= cap_samp_cnt_per_frm) ? play_samp_cnt_per_frm : cap_samp_cnt_per_frm;
        uint32_t echo_q_samp_cnt = (((max_samp_cnt_per_frm + mono_play_samp_cnt * SCO_PLAY_RESAMPLE_ALIGN_CNT) *
            // convert to 8K/16K sample cnt
             sample_rate +(stream_cfg.sample_rate - 1)) / stream_cfg.sample_rate +
            // aligned with aec_frame_len
            (aec_frame_len - 1)) / aec_frame_len * aec_frame_len;
        if (echo_q_samp_cnt == 0) {
            echo_q_samp_cnt = aec_frame_len;
        }
        voicebtpcm_pcm_echo_buf_queue_init(echo_q_samp_cnt * 2);
#else
        stream_cfg.sample_rate = sample_rate;
#endif

#ifdef __BT_ANC__
        // Mono channel decoder buffer (8K or 16K sample rate)
        app_audio_mempool_get_buff(&bt_anc_sco_dec_buf, stream_cfg.data_size / 2 / sco_play_chan_num);
        // The playback size for the actual sample rate
        bt_sco_samplerate_ratio = 6/(sample_rate/AUD_SAMPRATE_8000);
        stream_cfg.data_size *= bt_sco_samplerate_ratio;
#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
        stream_cfg.sample_rate = AUD_SAMPRATE_50781;
#else
        stream_cfg.sample_rate = AUD_SAMPRATE_48000;
#endif
        //damic_init();
        //init_amic_dc_bt();
        //ds_fir_init();
        us_fir_init();
#endif

        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.handler = bt_sco_codec_playback_data;

        app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

        TRACE("playback sample_rate:%d, data_size:%d",stream_cfg.sample_rate,stream_cfg.data_size);

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        sample_size_play_bt=stream_cfg.bits;
        sample_rate_play_bt=stream_cfg.sample_rate;
        data_size_play_bt=stream_cfg.data_size;
        playback_buf_bt=stream_cfg.data_ptr;
        playback_size_bt=stream_cfg.data_size;

#ifdef __BT_ANC__
        playback_samplerate_ratio_bt=8;
#else
        if (sample_rate_play_bt == AUD_SAMPRATE_8000)
        {
            playback_samplerate_ratio_bt=8*3*2;
        }
        else if (sample_rate_play_bt == AUD_SAMPRATE_16000)
        {
            playback_samplerate_ratio_bt=8*3;
        }
#endif

        playback_ch_num_bt=stream_cfg.channel_num;
#endif

#if defined(SCO_DMA_SNAPSHOT)
        playback_buf_codecpcm=stream_cfg.data_ptr;
        playback_size_codecpcm=stream_cfg.data_size;
        playback_samplerate_codecpcm=stream_cfg.sample_rate;
#endif
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

#if defined(SCO_DMA_SNAPSHOT)
        af_stream_dma_tc_irq_enable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        stream_cfg.bits = sample_size_play_bt;
        stream_cfg.channel_num = playback_ch_num_bt;
        stream_cfg.sample_rate = sample_rate_play_bt;
        stream_cfg.device = AUD_STREAM_USE_MC;
        stream_cfg.vol = 0;
        stream_cfg.handler = audio_mc_data_playback_sco;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;

        app_audio_mempool_get_buff(&bt_audio_buff, data_size_play_bt*playback_samplerate_ratio_bt);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
        stream_cfg.data_size = data_size_play_bt*playback_samplerate_ratio_bt;

        playback_buf_mc=stream_cfg.data_ptr;
        playback_size_mc=stream_cfg.data_size;

        anc_mc_run_init(hal_codec_anc_convert_rate(sample_rate_play_bt));

        af_stream_open(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg);
#endif

        // Must call this function before af_stream_start
        // Get all free app audio buffer except SCO_BTPCM used(2k)
        voicebtpcm_pcm_audio_init(speech_sco_get_sample_rate(), speech_codec_get_sample_rate());

        /*
        TRACE("[%s] start codec %d", __FUNCTION__, FAST_TICKS_TO_US(hal_fast_sys_timer_get()));
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        af_stream_start(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        */

#ifdef SPEECH_SIDETONE
        hal_codec_sidetone_enable();
#endif


#ifdef _SCO_BTPCM_CHANNEL_
        stream_cfg.sample_rate = speech_sco_get_sample_rate();
        stream_cfg.channel_num = AUD_CHANNEL_NUM_1;
        // stream_cfg.data_size = BT_AUDIO_SCO_BUFF_SIZE * stream_cfg.channel_num;

        if (bt_sco_codec_is_msbc())
        {
            stream_cfg.data_size = speech_get_af_buffer_size(stream_cfg.sample_rate, stream_cfg.channel_num, SPEECH_SCO_FRAME_MS)/2;
        }
        else
        {
            stream_cfg.data_size = speech_get_af_buffer_size(stream_cfg.sample_rate, stream_cfg.channel_num, SPEECH_SCO_FRAME_MS);
        }

        // btpcm:rx
        stream_cfg.device = AUD_STREAM_USE_BT_PCM;
        stream_cfg.handler = bt_sco_btpcm_capture_data;
        app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

        TRACE("sco btpcm sample_rate:%d, data_size:%d",stream_cfg.sample_rate,stream_cfg.data_size);

#if defined(SCO_DMA_SNAPSHOT)
        capture_buf_btpcm=stream_cfg.data_ptr;
        capture_size_btpcm=stream_cfg.data_size;
#endif
        af_stream_open(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE, &stream_cfg);

        // btpcm:tx
        stream_cfg.device = AUD_STREAM_USE_BT_PCM;
        stream_cfg.handler = bt_sco_btpcm_playback_data;
        app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

#if defined(SCO_DMA_SNAPSHOT)
        playback_buf_btpcm=stream_cfg.data_ptr;
        playback_size_btpcm=stream_cfg.data_size;
#endif      

        af_stream_open(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK, &stream_cfg);

#if defined(SCO_DMA_SNAPSHOT)
        app_bt_stream_sco_trigger_btpcm_start();
        af_stream_dma_tc_irq_enable(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
#endif  

#ifdef __AUDIO_RESAMPLE__
#ifndef SW_PLAYBACK_RESAMPLE
        af_codec_tune_resample_rate(AUD_STREAM_PLAYBACK, 0);
        af_codec_tune_resample_rate(AUD_STREAM_CAPTURE, 0);
#endif
#else
        af_codec_tune_pll(0);
#endif

        TRACE("[%s] start btpcm %d", __FUNCTION__, FAST_TICKS_TO_US(hal_fast_sys_timer_get()));
        af_stream_start(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);

        is_codec_stream_started = false;

#if defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST1400)
#if defined(CVSD_BYPASS)
        btdrv_cvsd_bypass_enable();
#endif
#if !defined(SCO_DMA_SNAPSHOT)
        btdrv_pcm_enable();
#endif
#endif

#endif

#ifdef FPGA
        app_bt_stream_volumeset(stream_local_volume);
        //btdrv_set_bt_pcm_en(1);
#endif
        TRACE("bt_sco_player on");
    }
    else
    {
#ifdef __THIRDPARTY
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO2,THIRDPARTY_MIC_CLOSE);
#endif
#if defined(SCO_DMA_SNAPSHOT)
        app_bt_stream_sco_trigger_codecpcm_stop();
        af_stream_dma_tc_irq_disable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif
#if defined(IBRT)
#ifdef CHIP_BEST2300P
        bt_drv_reg_op_clean_ibrt_sco_flags();
#endif
#endif
        if (is_codec_stream_started == true) {
            af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
            af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
            af_stream_stop(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif
            is_codec_stream_started = false;
        }

#ifdef _SCO_BTPCM_CHANNEL_
        af_stream_stop(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);

        af_stream_close(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
#endif

#if defined(PCM_FAST_MODE) && defined(CHIP_BEST2300P)
        btdrv_open_pcm_fast_mode_disable();
#endif

#if defined(HW_DC_FILTER_WITH_IIR)
        hw_filter_codec_iir_destroy(hw_filter_codec_iir_st);
#endif

        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        af_stream_close(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

#ifdef SPEECH_SIDETONE
        hal_codec_sidetone_disable();
#endif

#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
        app_capture_resample_close(sco_capture_resample);
        sco_capture_resample = NULL;
        app_capture_resample_close(sco_playback_resample);
        sco_playback_resample = NULL;
#endif

#if defined(__AUDIO_RESAMPLE__) && defined(NO_SCO_RESAMPLE)
#ifndef CHIP_BEST1000
        // When __AUDIO_RESAMPLE__ is defined,
        // resample is off by default on best1000, and on by default on other platforms
        hal_cmu_audio_resample_enable();
#endif
#endif

        bt_sco_mode = 0;

#ifdef __BT_ANC__
        bt_anc_sco_dec_buf = NULL;
        //damic_deinit();
        //app_cap_thread_stop();
#endif
        voicebtpcm_pcm_audio_deinit();

#ifndef FPGA
#ifdef BT_XTAL_SYNC
        bt_term_xtal_sync(false);
        bt_term_xtal_sync_default();
#endif
#endif
#if defined(HFP_1_6_ENABLE)
        bt_sco_player_code_type = BTIF_HF_SCO_CODEC_NONE;
#endif
        TRACE("bt_sco_player off");
        app_overlay_unloadall();
        app_sysfreq_req(APP_SYSFREQ_USER_BT_SCO, APP_SYSFREQ_32K);
        af_set_priority(osPriorityAboveNormal);

        //bt_syncerr set to default(0x07)
//       BTDIGITAL_REG_SET_FIELD(REG_BTCORE_BASE_ADDR, 0x0f, 0, 0x07);
#ifdef __THIRDPARTY
        //app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO1,THIRDPARTY_START);
#endif
    }

    isRun=on;
    return 0;
}

#ifdef AUDIO_LINEIN
#include "app_status_ind.h"
//player channel should <= capture channel number
//player must be 2 channel
#define LINEIN_PLAYER_CHANNEL (2)
#ifdef __AUDIO_OUTPUT_MONO_MODE__
#define LINEIN_CAPTURE_CHANNEL (1)
#else
#define LINEIN_CAPTURE_CHANNEL (2)
#endif

#if (LINEIN_CAPTURE_CHANNEL == 1)
#define LINEIN_PLAYER_BUFFER_SIZE (1024*LINEIN_PLAYER_CHANNEL)
#define LINEIN_CAPTURE_BUFFER_SIZE (LINEIN_PLAYER_BUFFER_SIZE/2)
#elif (LINEIN_CAPTURE_CHANNEL == 2)
#define LINEIN_PLAYER_BUFFER_SIZE (1024*LINEIN_PLAYER_CHANNEL)
#define LINEIN_CAPTURE_BUFFER_SIZE (LINEIN_PLAYER_BUFFER_SIZE)
#endif

static int16_t *app_linein_play_cache = NULL;

int8_t app_linein_buffer_is_empty(void)
{
    if (app_audio_pcmbuff_length()){
        return 0;
    }else{
        return 1;
    }
}

uint32_t app_linein_pcm_come(uint8_t * pcm_buf, uint32_t len)
{
    app_audio_pcmbuff_put(pcm_buf, len);

    return len;
}

uint32_t app_linein_need_pcm_data(uint8_t* pcm_buf, uint32_t len)
{
#if (LINEIN_CAPTURE_CHANNEL == 1)
    app_audio_pcmbuff_get((uint8_t *)app_linein_play_cache, len/2);
    app_play_audio_lineinmode_more_data((uint8_t *)app_linein_play_cache,len/2);
    app_bt_stream_copy_track_one_to_two_16bits((int16_t *)pcm_buf, app_linein_play_cache, len/2/2);
#elif (LINEIN_CAPTURE_CHANNEL == 2)
    app_audio_pcmbuff_get((uint8_t *)pcm_buf, len);
    app_play_audio_lineinmode_more_data((uint8_t *)pcm_buf, len);
#endif

#if defined(__AUDIO_OUTPUT_MONO_MODE__)
    merge_stereo_to_mono_16bits((int16_t *)buf, (int16_t *)pcm_buf, len/2);
#endif

#ifdef ANC_APP
    bt_audio_updata_eq_for_anc(app_anc_work_status());
#else
    bt_audio_updata_eq(app_audio_get_eq());
#endif

    audio_process_run(pcm_buf, len);

    return len;
}

int app_play_linein_onoff(bool onoff)
{
    static bool isRun =  false;
    uint8_t *linein_audio_cap_buff = 0;
    uint8_t *linein_audio_play_buff = 0;
    uint8_t *linein_audio_loop_buf = NULL;
    struct AF_STREAM_CONFIG_T stream_cfg;

    uint8_t POSSIBLY_UNUSED *bt_eq_buff = NULL;
    uint32_t POSSIBLY_UNUSED eq_buff_size;
    uint8_t POSSIBLY_UNUSED play_samp_size;

    TRACE("app_play_linein_onoff work:%d op:%d", isRun, onoff);

    if (isRun == onoff)
        return 0;

     if (onoff){
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);
        app_overlay_select(APP_OVERLAY_A2DP);
        app_audio_mempool_init();
        app_audio_mempool_get_buff(&linein_audio_cap_buff, LINEIN_CAPTURE_BUFFER_SIZE);
        app_audio_mempool_get_buff(&linein_audio_play_buff, LINEIN_PLAYER_BUFFER_SIZE);
        app_audio_mempool_get_buff(&linein_audio_loop_buf, LINEIN_PLAYER_BUFFER_SIZE<<2);
        app_audio_pcmbuff_init(linein_audio_loop_buf, LINEIN_PLAYER_BUFFER_SIZE<<2);

#if (LINEIN_CAPTURE_CHANNEL == 1)
        app_audio_mempool_get_buff((uint8_t **)&app_linein_play_cache, LINEIN_PLAYER_BUFFER_SIZE/2/2);
        app_play_audio_lineinmode_init(LINEIN_CAPTURE_CHANNEL, LINEIN_PLAYER_BUFFER_SIZE/2/2);
#elif (LINEIN_CAPTURE_CHANNEL == 2)
        app_play_audio_lineinmode_init(LINEIN_CAPTURE_CHANNEL, LINEIN_PLAYER_BUFFER_SIZE/2);
#endif

        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)LINEIN_PLAYER_CHANNEL;
#if defined(__AUDIO_RESAMPLE__)
        stream_cfg.sample_rate = AUD_SAMPRATE_50781;
#else
        stream_cfg.sample_rate = AUD_SAMPRATE_44100;
#endif
#if FPGA==0
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#endif
        stream_cfg.vol = stream_linein_volume;
        TRACE("vol = %d",stream_linein_volume);
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.handler = app_linein_need_pcm_data;
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(linein_audio_play_buff);
        stream_cfg.data_size = LINEIN_PLAYER_BUFFER_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

        stream_cfg.io_path = AUD_INPUT_PATH_LINEIN;
        stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)LINEIN_CAPTURE_CHANNEL;
        stream_cfg.channel_map = (enum AUD_CHANNEL_MAP_T)hal_codec_get_input_path_cfg(stream_cfg.io_path);
        stream_cfg.handler = app_linein_pcm_come;
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(linein_audio_cap_buff);
        stream_cfg.data_size = LINEIN_CAPTURE_BUFFER_SIZE;

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

#if defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
        eq_buff_size = stream_cfg.data_size*2;
#elif defined(__HW_FIR_EQ_PROCESS__) && !defined(__HW_IIR_EQ_PROCESS__)

        play_samp_size = (stream_cfg.bits <= AUD_BITS_16) ? 2 : 4;
#if defined(CHIP_BEST2000)
        eq_buff_size = stream_cfg.data_size * sizeof(int32_t) / play_samp_size;
#elif  defined(CHIP_BEST1000)
        eq_buff_size = stream_cfg.data_size * sizeof(int16_t) / play_samp_size;
#elif defined(CHIP_BEST2300) || defined(CHIP_BEST2300P)
        eq_buff_size = stream_cfg.data_size;
#endif

#elif !defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
        eq_buff_size = stream_cfg.data_size;
#else
        eq_buff_size = 0;
        bt_eq_buff = NULL;
#endif

        if(eq_buff_size>0)
        {
            app_audio_mempool_get_buff(&bt_eq_buff, eq_buff_size);
        }

        audio_process_init();
        audio_process_open(stream_cfg.sample_rate, stream_cfg.bits, stream_cfg.channel_num, bt_eq_buff, eq_buff_size);

#ifdef __SW_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_SW_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_SW_IIR,0));
#endif

#ifdef __HW_FIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_FIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_FIR,0));
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_DAC_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_DAC_IIR,0));
#endif

#ifdef __HW_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_IIR,0));
#endif

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
     }else     {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

        aaudio_process_close();

        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

        app_overlay_unloadall();
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
     }

    isRun = onoff;
    TRACE("%s end!\n", __func__);
    return 0;
}
#endif

int app_bt_stream_open(APP_AUDIO_STATUS* status)
{
    int nRet = -1;
    uint16_t player = status->id;
    APP_AUDIO_STATUS next_status;
    enum APP_SYSFREQ_FREQ_T freq = (enum APP_SYSFREQ_FREQ_T)status->freq;

    TRACE("app_bt_stream_open prev:%d cur:%d freq:%d", gStreamplayer, player, freq);

    if (gStreamplayer != APP_BT_STREAM_INVALID)
    {
    #if !ISOLATED_AUDIO_STREAM_ENABLED
        TRACE("Close prev bt stream before opening");
        nRet = app_bt_stream_close(gStreamplayer);
        if (nRet)
        {
            return -1;
        }
        else
        {
            app_audio_list_rmv_callback(status, &next_status, APP_BT_SETTING_Q_POS_TAIL);
        }
    #else
        if (gStreamplayer & player)
        {
            return -1;
        }

        if (player >= APP_BT_STREAM_BORDER_INDEX)
        {
            if (APP_BT_INPUT_STREAM_INDEX(gStreamplayer) > 0)
            {
                TRACE("Close prev input bt stream before opening");
                nRet = app_bt_stream_close(APP_BT_INPUT_STREAM_INDEX(gStreamplayer));
                if (nRet)
                {
                    return -1;
                }
                else
                {
                    app_audio_list_rmv_callback(status, &next_status, APP_BT_SETTING_Q_POS_TAIL);
                }
            }
        }
        else
        {
            if (APP_BT_OUTPUT_STREAM_INDEX(gStreamplayer) > 0)
            {
                TRACE("Close prev output bt stream before opening");
                nRet = app_bt_stream_close(APP_BT_OUTPUT_STREAM_INDEX(gStreamplayer));
                if (nRet)
                {
                    return -1;
                }
                else
                {
                    app_audio_list_rmv_callback(status, &next_status,APP_BT_SETTING_Q_POS_TAIL);
                }
            }
        }
    #endif
    }

    switch (player)
    {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            nRet = bt_sco_player(true, freq);
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
            nRet = bt_sbc_player(PLAYER_OPER_START, freq);
            break;
#ifdef __FACTORY_MODE_SUPPORT__
        case APP_FACTORYMODE_AUDIO_LOOP:
            nRet = app_factorymode_audioloop(true, freq);
            break;
#endif
#ifdef MEDIA_PLAYER_SUPPORT
        case APP_PLAY_BACK_AUDIO:
            nRet = app_play_audio_onoff(true, status);
            break;
#endif

#ifdef RB_CODEC
        case APP_BT_STREAM_RBCODEC:
            nRet = app_rbplay_audio_onoff(true, 0);
            break;
#endif

#ifdef VOICE_DATAPATH
        case APP_BT_STREAM_VOICEPATH:
            nRet = app_voicepath_start_audio_stream();
            break;
#endif

#ifdef __APP_WL_SMARTVOICE__
		case APP_BT_STREAM_WL_SMARTVOICE:
			nRet = app_wl_smartvoice_player(true, APP_SYSFREQ_208M);
            break;
#endif

#ifdef AUDIO_LINEIN
        case APP_PLAY_LINEIN_AUDIO:
            nRet = app_play_linein_onoff(true);
            break;
#endif

#if defined(APP_LINEIN_A2DP_SOURCE)
        case APP_A2DP_SOURCE_LINEIN_AUDIO:
            nRet = app_a2dp_source_linein_on(true);
            break;
#endif
#if defined(APP_I2S_A2DP_SOURCE)
        case APP_A2DP_SOURCE_I2S_AUDIO:
            nRet = app_a2dp_source_I2S_onoff(true);
            break;
#endif
        default:
            nRet = -1;
            break;
    }

    if (!nRet)
    {
        gStreamplayer |= player;
        TRACE("gStreamplayer is updated to 0x%x", gStreamplayer);
    }
    return nRet;
}

int app_bt_stream_close(uint16_t player)
{
    int nRet = -1;
    TRACE("%s gStreamplayer: 0x%x player:0x%x", __FUNCTION__, gStreamplayer, player);

    if ((gStreamplayer & player) != player)
    {
        return -1;
    }

    switch (player)
    {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            nRet = bt_sco_player(false, APP_SYSFREQ_32K);
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
            nRet = bt_sbc_player(PLAYER_OPER_STOP, APP_SYSFREQ_32K);
            break;
#ifdef __FACTORY_MODE_SUPPORT__
        case APP_FACTORYMODE_AUDIO_LOOP:
            nRet = app_factorymode_audioloop(false, APP_SYSFREQ_32K);
            break;
#endif
#ifdef MEDIA_PLAYER_SUPPORT
        case APP_PLAY_BACK_AUDIO:
            nRet = app_play_audio_onoff(false, NULL);
            break;
#endif
#ifdef RB_CODEC
        case APP_BT_STREAM_RBCODEC:
            nRet = app_rbplay_audio_onoff(false, 0);
            break;
#endif
#ifdef VOICE_DATAPATH
        case APP_BT_STREAM_VOICEPATH:
            nRet = app_voicepath_stop_audio_stream();
            break;
#endif

#ifdef __APP_WL_SMARTVOICE__
		case APP_BT_STREAM_WL_SMARTVOICE:
			nRet = app_wl_smartvoice_player(false, APP_SYSFREQ_32K);
            break;
#endif

#ifdef AUDIO_LINEIN
        case APP_PLAY_LINEIN_AUDIO:
            nRet = app_play_linein_onoff(false);
            break;
#endif

#if defined(APP_LINEIN_A2DP_SOURCE)
        case APP_A2DP_SOURCE_LINEIN_AUDIO:
            nRet = app_a2dp_source_linein_on(false);
            break;
#endif
#if defined(APP_I2S_A2DP_SOURCE)
        case APP_A2DP_SOURCE_I2S_AUDIO:
            nRet = app_a2dp_source_I2S_onoff(false);
            break;
#endif
        default:
            nRet = -1;
            break;
    }
    if (!nRet)
    {
        gStreamplayer &= (~player);
        TRACE("gStreamplayer is updated to 0x%x", gStreamplayer);
    }
    return nRet;
}

int app_bt_stream_setup(uint16_t player, uint8_t status)
{
    int nRet = -1;

    TRACE("app_bt_stream_setup prev:%d cur:%d sample:%d", gStreamplayer, player, status);

    switch (player)
    {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
            bt_sbc_player_setup(status);
            break;
        default:
            nRet = -1;
            break;
    }

    return nRet;
}

int app_bt_stream_restart(APP_AUDIO_STATUS* status)
{
    int nRet = -1;
    uint16_t player = status->id;
    enum APP_SYSFREQ_FREQ_T freq = (enum APP_SYSFREQ_FREQ_T)status->freq;

    TRACE("app_bt_stream_restart prev:%d cur:%d freq:%d", gStreamplayer, player, freq);

    if ((gStreamplayer & player) != player)
    {
        return -1;
    }

    switch (player)
    {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            nRet = bt_sco_player(false, freq);
            nRet = bt_sco_player(true, freq);
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
#ifdef __BT_ONE_BRING_TWO__
            if (btif_me_get_activeCons()>1)
            {
                enum APP_SYSFREQ_FREQ_T sysfreq;

#ifdef A2DP_CP_ACCEL
                sysfreq = APP_SYSFREQ_26M;
#else
                sysfreq = APP_SYSFREQ_104M;
#endif
                app_sysfreq_req(APP_SYSFREQ_USER_APP_0, sysfreq);
                bt_media_volume_ptr_update_by_mediatype(BT_STREAM_SBC);
                app_bt_stream_volumeset(btdevice_volume_p->a2dp_vol);
            }
#endif
            break;
#ifdef __FACTORY_MODE_SUPPORT__
        case APP_FACTORYMODE_AUDIO_LOOP:
            break;
#endif
#ifdef MEDIA_PLAYER_SUPPORT
        case APP_PLAY_BACK_AUDIO:
            break;
#endif
        default:
            nRet = -1;
            break;
    }

    return nRet;
}

void app_bt_stream_volumeup(void)
{
#if defined AUDIO_LINEIN
    if(app_bt_stream_isrun(APP_PLAY_LINEIN_AUDIO))
    {
        stream_linein_volume ++;
        if (stream_linein_volume > TGT_VOLUME_LEVEL_15)
        stream_linein_volume = TGT_VOLUME_LEVEL_15;
        app_bt_stream_volumeset(stream_linein_volume);
        TRACE("set linein volume %d\n", stream_linein_volume);
    }else
#endif
    {
        if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM))
        {
            TRACE("%s set hfp volume", __func__);
            btdevice_volume_p->hfp_vol++;
            if (btdevice_volume_p->hfp_vol > TGT_VOLUME_LEVEL_15)
            {
                btdevice_volume_p->hfp_vol = TGT_VOLUME_LEVEL_15;
            }
            else
            {
                app_bt_stream_volumeset(btdevice_volume_p->hfp_vol);
            }
        }
        else if ((app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)) ||
            (app_bt_stream_isrun(APP_BT_STREAM_INVALID)))
        {
            TRACE("%s set audio volume", __func__);

            btdevice_volume_p->a2dp_vol++;
            if (btdevice_volume_p->a2dp_vol > TGT_VOLUME_LEVEL_15)
            {
                btdevice_volume_p->a2dp_vol = TGT_VOLUME_LEVEL_15;
            }
            else
            {
                app_bt_stream_volumeset(btdevice_volume_p->a2dp_vol);
            }
        }
        if (btdevice_volume_p->a2dp_vol == TGT_VOLUME_LEVEL_15)
        {
#ifdef MEDIA_PLAYER_SUPPORT
            media_PlayAudio(AUD_ID_BT_WARNING,0);
#endif
        }
        if (btdevice_volume_p->hfp_vol == TGT_VOLUME_LEVEL_15)
        {
#ifdef MEDIA_PLAYER_SUPPORT
            media_PlayAudio(AUD_ID_BT_WARNING,0);
#endif
        }

        TRACE("%s a2dp: %d", __func__, btdevice_volume_p->a2dp_vol);
        TRACE("%s hfp: %d", __func__, btdevice_volume_p->hfp_vol);
    }
#ifndef FPGA
    nv_record_touch_cause_flush();
#endif
}

void app_bt_set_volume(uint16_t type,uint8_t level)
{
    if ((type&APP_BT_STREAM_HFP_PCM) && app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)) {
        TRACE("%s set hfp volume", __func__);
        if (level >= TGT_VOLUME_LEVEL_MUTE && level <= TGT_VOLUME_LEVEL_15)
        {
            btdevice_volume_p->hfp_vol = level;
            app_bt_stream_volumeset(btdevice_volume_p->hfp_vol);
        }
    }
    if ((type&APP_BT_STREAM_A2DP_SBC) && ((app_bt_stream_isrun(APP_BT_STREAM_INVALID)) ||
        (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)))) {
        TRACE("%s set a2dp volume", __func__);
        if (level >= TGT_VOLUME_LEVEL_MUTE && level <= TGT_VOLUME_LEVEL_15)
        {
            btdevice_volume_p->a2dp_vol = level;
            app_bt_stream_volumeset(btdevice_volume_p->a2dp_vol);
        }
    }
    if (btdevice_volume_p->a2dp_vol == TGT_VOLUME_LEVEL_MUTE)
    {
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio(AUD_ID_BT_WARNING,0);
#endif
    }
    if (btdevice_volume_p->hfp_vol == TGT_VOLUME_LEVEL_0)
    {
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio(AUD_ID_BT_WARNING,0);
#endif
    }

    TRACE("%s a2dp: %d", __func__, btdevice_volume_p->a2dp_vol);
    TRACE("%s hfp: %d", __func__, btdevice_volume_p->hfp_vol);
#ifndef FPGA
    nv_record_touch_cause_flush();
#endif
}

void app_bt_stream_volumedown(void)
{
#if defined AUDIO_LINEIN
    if(app_bt_stream_isrun(APP_PLAY_LINEIN_AUDIO))
    {
        stream_linein_volume --;
        if (stream_linein_volume < TGT_VOLUME_LEVEL_MUTE)
            stream_linein_volume = TGT_VOLUME_LEVEL_MUTE;
        app_bt_stream_volumeset(stream_linein_volume);
        TRACE("set linein volume %d\n", stream_linein_volume);
    }else
#endif
    {
        if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)) {
            TRACE("%s set hfp volume", __func__);
            btdevice_volume_p->hfp_vol--;
            if (btdevice_volume_p->hfp_vol < TGT_VOLUME_LEVEL_MUTE) {
                btdevice_volume_p->hfp_vol = TGT_VOLUME_LEVEL_MUTE;
            }else
            app_bt_stream_volumeset(btdevice_volume_p->hfp_vol);
        } else if ((app_bt_stream_isrun(APP_BT_STREAM_INVALID)) ||
            (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC))) {
            TRACE("%s set a2dp volume", __func__);
            btdevice_volume_p->a2dp_vol--;
            if (btdevice_volume_p->a2dp_vol < TGT_VOLUME_LEVEL_MUTE) {
                btdevice_volume_p->a2dp_vol = TGT_VOLUME_LEVEL_MUTE;
            }else{
                app_bt_stream_volumeset(btdevice_volume_p->a2dp_vol);
            }
        }
        if (btdevice_volume_p->a2dp_vol == TGT_VOLUME_LEVEL_MUTE)
        {
#ifdef MEDIA_PLAYER_SUPPORT
            media_PlayAudio(AUD_ID_BT_WARNING,0);
#endif
        }
        if (btdevice_volume_p->hfp_vol == TGT_VOLUME_LEVEL_0)
        {
#ifdef MEDIA_PLAYER_SUPPORT
            media_PlayAudio(AUD_ID_BT_WARNING,0);
#endif
        }

        TRACE("%s a2dp: %d", __func__, btdevice_volume_p->a2dp_vol);
        TRACE("%s hfp: %d", __func__, btdevice_volume_p->hfp_vol);
    }
#ifndef FPGA
    nv_record_touch_cause_flush();
#endif
}
int app_bt_stream_volumeset(int8_t vol)
{
    uint32_t ret;
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    TRACE("app_bt_stream_volumeset vol=%d", vol);

    if (vol > TGT_VOLUME_LEVEL_15)
        vol = TGT_VOLUME_LEVEL_15;
    if (vol < TGT_VOLUME_LEVEL_MUTE)
        vol = TGT_VOLUME_LEVEL_MUTE;

    stream_local_volume = vol;
    if (!app_bt_stream_isrun(APP_PLAY_BACK_AUDIO))
    {
        ret = af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, true);
        if (ret == 0) {
            stream_cfg->vol = vol;
            af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, stream_cfg);
        }
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        ret = af_stream_get_cfg(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg, true);
        if (ret == 0) {
            stream_cfg->vol = vol;
            af_stream_setup(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, stream_cfg);
        }
#endif


    }
    return 0;
}

int app_bt_stream_local_volume_get(void)
{
    return stream_local_volume;
}

uint8_t app_bt_stream_a2dpvolume_get(void)
{
    return btdevice_volume_p->a2dp_vol;
}

uint8_t app_bt_stream_hfpvolume_get(void)
{
    return btdevice_volume_p->hfp_vol;
}

void app_bt_stream_a2dpvolume_reset(void)
{
    btdevice_volume_p->a2dp_vol = NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT ;
}

void app_bt_stream_hfpvolume_reset(void)
{
    btdevice_volume_p->hfp_vol = NVRAM_ENV_STREAM_VOLUME_HFP_VOL_DEFAULT;
}

void app_bt_stream_volume_ptr_update(uint8_t *bdAddr)
{
    static struct btdevice_volume stream_volume = {NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT,NVRAM_ENV_STREAM_VOLUME_HFP_VOL_DEFAULT};

#ifndef FPGA
    nvrec_btdevicerecord *record = NULL;

    if (!nv_record_btdevicerecord_find((bt_bdaddr_t*)bdAddr,&record))
    {
        btdevice_volume_p = &(record->device_vol);
        DUMP8("0x%02x ", bdAddr, BTIF_BD_ADDR_SIZE);
        TRACE("%s a2dp_vol:%d hfp_vol:%d ptr:0x%x", __func__, btdevice_volume_p->a2dp_vol, btdevice_volume_p->hfp_vol,btdevice_volume_p);
    }
    else
#endif
    {
        btdevice_volume_p = &stream_volume;
        TRACE("%s default", __func__);
    }
}

struct btdevice_volume * app_bt_stream_volume_get_ptr(void)
{
    return btdevice_volume_p;
}

bool app_bt_stream_isrun(uint16_t player)
{
    if ((gStreamplayer & player) == player)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int app_bt_stream_closeall()
{
    TRACE("app_bt_stream_closeall");

    bt_sco_player(false, APP_SYSFREQ_32K);
    bt_sbc_player(PLAYER_OPER_STOP, APP_SYSFREQ_32K);

#ifdef MEDIA_PLAYER_SUPPORT
    app_play_audio_onoff(false, 0);
#endif
#ifdef RB_CODEC
    app_rbplay_audio_onoff(false, 0);
#endif

#ifdef VOICE_DATAPATH
    app_voicepath_stop_audio_stream();
#endif

#ifdef __APP_WL_SMARTVOICE__
    app_wl_smartvoice_player(false, APP_SYSFREQ_32K);
#endif

    gStreamplayer = APP_BT_STREAM_INVALID;

    return 0;
}

void app_bt_stream_copy_track_one_to_two_16bits(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (int i = (int)(src_len - 1); i >= 0; i--)
    {
        dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = src_buf[i];
    }
}

void app_bt_stream_copy_track_two_to_one_16bits(int16_t *dst_buf, int16_t *src_buf, uint32_t dst_len)
{
    for (uint32_t i = 0; i < dst_len; i++)
    {
        dst_buf[i] = src_buf[i*2];
    }
}

void app_bt_stream_adaptive_frequency_adjusting(void)
{
#ifndef A2DP_CP_ACCEL
    if (isRun)
    {
        if (hal_sysfreq_get() < HAL_CMU_FREQ_104M)
        {
            app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);
        }
    }
#endif
}

// =======================================================
// APP RESAMPLE
// =======================================================

#ifndef MIX_MIC_DURING_MUSIC
#include "resample_coef.h"
#endif

static APP_RESAMPLE_BUF_ALLOC_CALLBACK resamp_buf_alloc = app_audio_mempool_get_buff;

static void memzero_int16(void *dst, uint32_t len)
{
    int16_t *dst16 = (int16_t *)dst;
    int16_t *dst16_end = dst16 + len / 2;

    while (dst16 < dst16_end)
    {
        *dst16++ = 0;
    }
}

static struct APP_RESAMPLE_T *app_resample_open(enum AUD_STREAM_T stream, const struct RESAMPLE_COEF_T *coef, enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
        float ratio_step)
{
    TRACE("app resample ratio: %d/1000", uint32_t(ratio_step * 1000));
    struct APP_RESAMPLE_T *resamp;
    struct RESAMPLE_CFG_T cfg;
    enum RESAMPLE_STATUS_T status;
    uint32_t size, resamp_size;
    uint8_t *buf;

    resamp_size = audio_resample_ex_get_buffer_size(chans, AUD_BITS_16, coef->phase_coef_num);

    size = sizeof(struct APP_RESAMPLE_T);
    size += ALIGN(iter_len, 4);
    size += resamp_size;

    resamp_buf_alloc(&buf, size);

    resamp = (struct APP_RESAMPLE_T *)buf;
    buf += sizeof(*resamp);
    resamp->stream = stream;
    resamp->cb = cb;
    resamp->iter_buf = buf;
    buf += ALIGN(iter_len, 4);
    resamp->iter_len = iter_len;
    resamp->offset = iter_len;
    resamp->ratio_step = ratio_step;

    memset(&cfg, 0, sizeof(cfg));
    cfg.chans = chans;
    cfg.bits = AUD_BITS_16;
    cfg.ratio_step = ratio_step;
    cfg.coef = coef;
    cfg.buf = buf;
    cfg.size = resamp_size;

    status = audio_resample_ex_open(&cfg, (RESAMPLE_ID *)&resamp->id);
    ASSERT(status == RESAMPLE_STATUS_OK, "%s: Failed to open resample: %d", __func__, status);

#ifdef CHIP_BEST1000
    hal_cmu_audio_resample_enable();
#endif

    return resamp;
}

static int app_resample_close(struct APP_RESAMPLE_T *resamp)
{
#ifdef CHIP_BEST1000
    hal_cmu_audio_resample_disable();
#endif

    if (resamp)
    {
        audio_resample_ex_close((RESAMPLE_ID *)resamp->id);
    }

    return 0;
}

struct APP_RESAMPLE_T *app_playback_resample_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len)
{
    const struct RESAMPLE_COEF_T *coef = NULL;

    if (sample_rate == AUD_SAMPRATE_8000)
    {
        coef = &resample_coef_8k_to_8p4k;
    }
    else if (sample_rate == AUD_SAMPRATE_16000)
    {
        coef = &resample_coef_8k_to_8p4k;
    }
    else if (sample_rate == AUD_SAMPRATE_32000)
    {
        coef = &resample_coef_32k_to_50p7k;
    }
    else if (sample_rate == AUD_SAMPRATE_44100)
    {
        coef = &resample_coef_44p1k_to_50p7k;
    }
    else if (sample_rate == AUD_SAMPRATE_48000)
    {
        coef = &resample_coef_48k_to_50p7k;
    }
    else
    {
        ASSERT(false, "%s: Bad sample rate: %u", __func__, sample_rate);
    }

    return app_resample_open(AUD_STREAM_PLAYBACK, coef, chans, cb, iter_len, 0);
}

struct APP_RESAMPLE_T *app_playback_resample_any_open(enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
        float ratio_step)
{
    const struct RESAMPLE_COEF_T *coef = &resample_coef_any_up256;

    return app_resample_open(AUD_STREAM_PLAYBACK, coef, chans, cb, iter_len, ratio_step);
}

#ifdef PLAYBACK_FORCE_48K
static int app_force48k_resample_iter(uint8_t *buf, uint32_t len)
{
    uint8_t codec_type = bt_sbc_player_get_codec_type();
    uint32_t overlay_id = 0;
    if(codec_type ==  BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC)
    {
        overlay_id = APP_OVERLAY_A2DP_AAC;
    }

    a2dp_audio_more_data(overlay_id, buf, len);
    return 0;
}

struct APP_RESAMPLE_T *app_force48k_resample_any_open(enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
        float ratio_step)
{
    const struct RESAMPLE_COEF_T *coef = &resample_coef_any_up256;

    return app_resample_open(AUD_STREAM_PLAYBACK, coef, chans, cb, iter_len, ratio_step);
}
#endif

int app_playback_resample_close(struct APP_RESAMPLE_T *resamp)
{
    return app_resample_close(resamp);
}

int app_playback_resample_run(struct APP_RESAMPLE_T *resamp, uint8_t *buf, uint32_t len)
{
    uint32_t in_size, out_size;
    struct RESAMPLE_IO_BUF_T io;
    enum RESAMPLE_STATUS_T status;
    int ret;
    //uint32_t lock;

    if (resamp == NULL)
    {
        goto _err_exit;
    }

    io.out_cyclic_start = NULL;
    io.out_cyclic_end = NULL;

    if (resamp->offset < resamp->iter_len)
    {
        io.in = resamp->iter_buf + resamp->offset;
        io.in_size = resamp->iter_len - resamp->offset;
        io.out = buf;
        io.out_size = len;

        //lock = int_lock();
        status = audio_resample_ex_run((RESAMPLE_ID *)resamp->id, &io, &in_size, &out_size);
        //int_unlock(lock);
        if (status != RESAMPLE_STATUS_OUT_FULL && status != RESAMPLE_STATUS_IN_EMPTY &&
            status != RESAMPLE_STATUS_DONE)
        {
            goto _err_exit;
        }

        buf += out_size;
        len -= out_size;
        resamp->offset += in_size;

        ASSERT(len == 0 || resamp->offset == resamp->iter_len,
            "%s: Bad resample offset: len=%d offset=%u iter_len=%u",
            __func__, len, resamp->offset, resamp->iter_len);
    }

    while (len)
    {
        ret = resamp->cb(resamp->iter_buf, resamp->iter_len);
        if (ret)
        {
            goto _err_exit;
        }

        io.in = resamp->iter_buf;
        io.in_size = resamp->iter_len;
        io.out = buf;
        io.out_size = len;

        //lock = int_lock();
        status = audio_resample_ex_run((RESAMPLE_ID *)resamp->id, &io, &in_size, &out_size);
        //int_unlock(lock);
        if (status != RESAMPLE_STATUS_OUT_FULL && status != RESAMPLE_STATUS_IN_EMPTY &&
            status != RESAMPLE_STATUS_DONE)
        {
            goto _err_exit;
        }

        ASSERT(out_size <= len, "%s: Bad resample out_size: out_size=%u len=%d", __func__, out_size, len);
        ASSERT(in_size <= resamp->iter_len, "%s: Bad resample in_size: in_size=%u iter_len=%u", __func__, in_size, resamp->iter_len);

        buf += out_size;
        len -= out_size;
        if (in_size != resamp->iter_len)
        {
            resamp->offset = in_size;

            ASSERT(len == 0, "%s: Bad resample len: len=%d out_size=%u", __func__, len, out_size);
        }
    }

    return 0;

_err_exit:
    if (resamp)
    {
        app_resample_reset(resamp);
    }

    memzero_int16(buf, len);

    return 1;
}

struct APP_RESAMPLE_T *app_capture_resample_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len)
{
    const struct RESAMPLE_COEF_T *coef = NULL;

    if (sample_rate == AUD_SAMPRATE_8000)
    {
        coef = &resample_coef_8p4k_to_8k;
    }
    else if (sample_rate == AUD_SAMPRATE_16000)
    {
        // Same coef as 8K sample rate
        coef = &resample_coef_8p4k_to_8k;
    }
    else
    {
        ASSERT(false, "%s: Bad sample rate: %u", __func__, sample_rate);
    }

    return app_resample_open(AUD_STREAM_CAPTURE, coef, chans, cb, iter_len, 0);

}

struct APP_RESAMPLE_T *app_capture_resample_any_open(enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
        float ratio_step)
{
    const struct RESAMPLE_COEF_T *coef = &resample_coef_any_up256;
    return app_resample_open(AUD_STREAM_CAPTURE, coef, chans, cb, iter_len, ratio_step);
}

int app_capture_resample_close(struct APP_RESAMPLE_T *resamp)
{
    return app_resample_close(resamp);
}

int app_capture_resample_run(struct APP_RESAMPLE_T *resamp, uint8_t *buf, uint32_t len)
{
    uint32_t in_size, out_size;
    struct RESAMPLE_IO_BUF_T io;
    enum RESAMPLE_STATUS_T status;
    int ret;

    if (resamp == NULL)
    {
        goto _err_exit;
    }

    io.out_cyclic_start = NULL;
    io.out_cyclic_end = NULL;

    if (resamp->offset < resamp->iter_len)
    {
        io.in = buf;
        io.in_size = len;
        io.out = resamp->iter_buf + resamp->offset;
        io.out_size = resamp->iter_len - resamp->offset;

        status = audio_resample_ex_run((RESAMPLE_ID *)resamp->id, &io, &in_size, &out_size);
        if (status != RESAMPLE_STATUS_OUT_FULL && status != RESAMPLE_STATUS_IN_EMPTY &&
            status != RESAMPLE_STATUS_DONE)
        {
            goto _err_exit;
        }

        buf += in_size;
        len -= in_size;
        resamp->offset += out_size;

        ASSERT(len == 0 || resamp->offset == resamp->iter_len,
            "%s: Bad resample offset: len=%d offset=%u iter_len=%u",
            __func__, len, resamp->offset, resamp->iter_len);

        if (resamp->offset == resamp->iter_len)
        {
            ret = resamp->cb(resamp->iter_buf, resamp->iter_len);
            if (ret)
            {
                goto _err_exit;
            }
        }
    }

    while (len)
    {
        io.in = buf;
        io.in_size = len;
        io.out = resamp->iter_buf;
        io.out_size = resamp->iter_len;

        status = audio_resample_ex_run((RESAMPLE_ID *)resamp->id, &io, &in_size, &out_size);
        if (status != RESAMPLE_STATUS_OUT_FULL && status != RESAMPLE_STATUS_IN_EMPTY &&
            status != RESAMPLE_STATUS_DONE)
        {
            goto _err_exit;
        }

        ASSERT(in_size <= len, "%s: Bad resample in_size: in_size=%u len=%u", __func__, in_size, len);
        ASSERT(out_size <= resamp->iter_len, "%s: Bad resample out_size: out_size=%u iter_len=%u", __func__, out_size, resamp->iter_len);

        buf += in_size;
        len -= in_size;
        if (out_size == resamp->iter_len)
        {
            ret = resamp->cb(resamp->iter_buf, resamp->iter_len);
            if (ret)
            {
                goto _err_exit;
            }
        }
        else
        {
            resamp->offset = out_size;

            ASSERT(len == 0, "%s: Bad resample len: len=%u in_size=%u", __func__, len, in_size);
        }
    }

    return 0;

_err_exit:
    if (resamp)
    {
        app_resample_reset(resamp);
    }

    memzero_int16(buf, len);

    return 1;
}

void app_resample_reset(struct APP_RESAMPLE_T *resamp)
{
    audio_resample_ex_flush((RESAMPLE_ID *)resamp->id);
    resamp->offset = resamp->iter_len;
}

void app_resample_tune(struct APP_RESAMPLE_T *resamp, float ratio)
{
    float new_step;

    if (resamp == NULL)
    {
        return;
    }

    TRACE("%s: stream=%d ratio=%d", __FUNCTION__, resamp->stream, FLOAT_TO_PPB_INT(ratio));

    if (resamp->stream == AUD_STREAM_PLAYBACK) {
        new_step = resamp->ratio_step + resamp->ratio_step * ratio;
    } else {
        new_step = resamp->ratio_step - resamp->ratio_step * ratio;
    }
    audio_resample_ex_set_ratio_step(resamp->id, new_step);
}

APP_RESAMPLE_BUF_ALLOC_CALLBACK app_resample_set_buf_alloc_callback(APP_RESAMPLE_BUF_ALLOC_CALLBACK cb)
{
    APP_RESAMPLE_BUF_ALLOC_CALLBACK old_cb;

    old_cb = resamp_buf_alloc;
    resamp_buf_alloc = cb;

    return old_cb;
}

extern CQueue* get_tx_esco_queue_ptr();
extern CQueue* get_rx_esco_queue_ptr();
#ifdef TX_RX_PCM_MASK
void store_encode_frame2buff()
{
    CQueue* Tx_esco_queue_temp = NULL;
    CQueue* Rx_esco_queue_temp = NULL;
    Tx_esco_queue_temp = get_tx_esco_queue_ptr();
    Rx_esco_queue_temp = get_rx_esco_queue_ptr();
    unsigned int len;
    len= 60;
    int status = 0;
    if(bt_sco_codec_is_msbc())
    {
        status = DeCQueue(Tx_esco_queue_temp,(uint8_t *)(*(volatile uint32_t *)(MIC_BUFF_ADRR_REG)),len); 
        if(status){
            //TRACE("TX DeC Fail");
        }
        status =EnCQueue(Rx_esco_queue_temp, (uint8_t *)(*(volatile uint32_t *)(RX_BUFF_ADRR)), len);
        if(status){
            //TRACE("RX EnC Fail");
        }
    }

}
#endif

