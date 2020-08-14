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
/*******************************************************************************
** namer    : Audio Process
** description  : Manage auido process algorithms, include :hw iir eq, sw iir eq, 
**                  hw fir eq, drc...
** version  : V1.0
** author   : Yunjie Huo
** modify   : 2018.12.4
** todo     : NULL
** MIPS     :   NO PROCESS: 34M(TWS, one ear, Mono, AAC, 48k)
**              DRC1: 36M(3 bands)
**              DRC2: 12M
*******************************************************************************/
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "string.h"
#include "audio_process.h"
#include "stdbool.h"
#include "hal_location.h"
#include "hw_codec_iir_process.h"
#include "hw_iir_process.h"
#include "tgt_hardware.h"
#include "drc.h"
#include "limiter.h"
#include "audio_cfg.h"

#ifdef __PC_CMD_UART__
#include "hal_cmd.h"

#if defined(__SW_IIR_EQ_PROCESS__)
#define AUDIO_EQ_SW_IIR_UPDATE_CFG
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__) 
#define AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
#define AUDIO_EQ_HW_IIR_UPDATE_CFG
#endif

#ifdef __HW_FIR_EQ_PROCESS__
#define AUDIO_EQ_HW_FIR_UPDATE_CFG
#endif

#ifdef __AUDIO_DRC__
#define AUDIO_DRC_UPDATE_CFG
#endif

#ifdef __AUDIO_DRC2__
#define AUDIO_DRC2_UPDATE_CFG
#endif

#endif

#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG) || defined(AUDIO_EQ_HW_FIR_UPDATE_CFG)|| defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG)|| defined(AUDIO_EQ_HW_IIR_UPDATE_CFG) || defined(AUDIO_DRC_UPDATE_CFG) || defined(AUDIO_DRC2_UPDATE_CFG)
#define AUDIO_UPDATE_CFG
#endif

#ifdef __AUDIO_DRC__
#define AUDIO_DRC_NEEDED_SIZE (1024*12)
#else
#define AUDIO_DRC_NEEDED_SIZE (0)
#endif

#ifdef __AUDIO_DRC2__
#define AUDIO_DRC2_NEEDED_SIZE (1024*5)
#else
#define AUDIO_DRC2_NEEDED_SIZE (0)
#endif

#if defined(__PC_CMD_UART__) && (defined(__AUDIO_DRC) || defined(__AUDIO_DRC2__))
// FIXME: drc/limiter calls malloc/free in set_config, so we need more ram for audiotool
#define AUDIOTOOL_RESERVED_SIZE (1024*6)
#else
#define AUDIOTOOL_RESERVED_SIZE (0)
#endif

#define AUDIO_MEMORY_SIZE (AUDIO_DRC_NEEDED_SIZE + AUDIO_DRC2_NEEDED_SIZE + AUDIOTOOL_RESERVED_SIZE)

#if AUDIO_MEMORY_SIZE > 0
#include "audio_memory.h"
#endif

typedef signed int          pcm_24bits_t;
typedef signed short int    pcm_16bits_t;

typedef struct{
    enum AUD_BITS_T         sample_bits;
    enum AUD_SAMPRATE_T     sample_rate;
    enum AUD_CHANNEL_NUM_T  ch_num; 

#if defined(__SW_IIR_EQ_PROCESS__)
    bool sw_iir_enable;
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
    bool hw_dac_iir_enable;
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
    bool hw_iir_enable;
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
    bool hw_fir_enable;
#endif

#if AUDIO_MEMORY_SIZE > 0
    uint8_t *audio_heap;
#endif

#ifdef __AUDIO_DRC__
    bool drc_enable;
    DrcState *drc_st;
#endif

#ifdef __AUDIO_DRC2__
    bool drc2_enable;
    LimiterState *drc2_st;
#endif

#ifdef AUDIO_UPDATE_CFG
    bool update_cfg;
#endif

#ifdef AUDIO_EQ_SW_IIR_UPDATE_CFG
    IIR_CFG_T sw_iir_cfg;
#endif

#ifdef AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
    IIR_CFG_T hw_dac_iir_cfg;
#endif

#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
    IIR_CFG_T hw_iir_cfg;
#endif

#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
    FIR_CFG_T hw_fir_cfg;
#endif

#ifdef AUDIO_DRC_UPDATE_CFG
    DrcConfig drc_cfg;
#endif

#ifdef AUDIO_DRC2_UPDATE_CFG
    LimiterConfig drc2_cfg;
#endif
} AUDIO_PROCESS_T;

#ifdef __AUDIO_DRC__
extern const DrcConfig audio_drc_cfg;
#endif

#ifdef __AUDIO_DRC2__
extern const LimiterConfig audio_drc2_cfg;
#endif

static AUDIO_PROCESS_T audio_process = {
    .sample_bits = AUD_BITS_NULL,
    .sample_rate = AUD_SAMPRATE_NULL,
    .ch_num = AUD_CHANNEL_NUM_NULL,
    
#if defined(__SW_IIR_EQ_PROCESS__) 
    .sw_iir_enable =  false,
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
    .hw_dac_iir_enable =  false,
#endif

#if defined(__HW_IIR_EQ_PROCESS__)   
    .hw_iir_enable =  false,
#endif

#if defined(__HW_FIR_EQ_PROCESS__)   
    .hw_fir_enable =  false,
#endif

#ifdef __AUDIO_DRC__
    .drc_enable = false,
#endif

#ifdef __AUDIO_DRC2__
    .drc2_enable = false,
#endif

#ifdef AUDIO_UPDATE_CFG
    .update_cfg = false,
#endif

#ifdef AUDIO_EQ_SW_IIR_UPDATE_CFG
    .sw_iir_cfg = {.num = 0},
#endif

#ifdef AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
    .hw_dac_iir_cfg = {.num = 0},
#endif

#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
    .hw_iir_cfg = {.num = 0},
#endif

#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
    .hw_fir_cfg = {.len = 0},
#endif

};

int audio_eq_set_cfg(const FIR_CFG_T *fir_cfg, const IIR_CFG_T *iir_cfg, AUDIO_EQ_TYPE_T audio_eq_type)
{

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)|| defined(__HW_DAC_IIR_EQ_PROCESS__)|| defined(__HW_IIR_EQ_PROCESS__)
    switch (audio_eq_type)
    {
#if defined(__SW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_SW_IIR:
        {
            if(iir_cfg)
            {
                audio_process.sw_iir_enable = false;
                iir_set_cfg(iir_cfg);
                audio_process.sw_iir_enable = true;
            }
            else
            {
                audio_process.sw_iir_enable = false;
            }
        }           
        break;
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_FIR:
        {
            if(fir_cfg)
            {
                audio_process.hw_fir_enable = false;
                fir_set_cfg(fir_cfg);
                audio_process.hw_fir_enable = true;
            }
            else
            {
                audio_process.hw_fir_enable = false;
            }
        }                
        break;
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_DAC_IIR:
        {
            if(iir_cfg)
            {
                HW_CODEC_IIR_CFG_T *hw_iir_cfg_dac=NULL; 
#ifdef __AUDIO_RESAMPLE__
                enum AUD_SAMPRATE_T sample_rate_hw_iir=AUD_SAMPRATE_50781;
#else
                enum AUD_SAMPRATE_T sample_rate_hw_iir=audio_process.sample_rate;
#endif
                audio_process.hw_dac_iir_enable = false;
                hw_iir_cfg_dac = hw_codec_iir_get_cfg(sample_rate_hw_iir,iir_cfg);
                ASSERT(hw_iir_cfg_dac != NULL, "[%s] %d codec IIR parameter error!", __func__, hw_iir_cfg_dac);
                hw_codec_iir_set_cfg(hw_iir_cfg_dac, sample_rate_hw_iir, HW_CODEC_IIR_DAC);
                audio_process.hw_dac_iir_enable = true;
            }
            else
            {
                audio_process.hw_dac_iir_enable = false;
            }
        }
        break;
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_IIR:
        {
            if(iir_cfg)
            {
                HW_IIR_CFG_T *hw_iir_cfg=NULL; 
#ifdef __AUDIO_RESAMPLE__
                enum AUD_SAMPRATE_T sample_rate_hw_iir=AUD_SAMPRATE_50781;
#else
                enum AUD_SAMPRATE_T sample_rate_hw_iir=audio_process.sample_rate;
#endif
                audio_process.hw_iir_enable = false;
                hw_iir_cfg = hw_iir_get_cfg(sample_rate_hw_iir,iir_cfg);
                ASSERT(hw_iir_cfg != NULL, "[%s] %d codec IIR parameter error!", __func__, hw_iir_cfg);
                hw_iir_set_cfg(hw_iir_cfg);
                audio_process.hw_iir_enable = true;
            }
            else
            {
                audio_process.hw_iir_enable = false;
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

    return 0;
}

#ifdef __AUDIO_DRC__
int audio_drc_1_set_cfg(const DrcConfig *cfg)
{
	if(cfg){
        audio_process.drc_enable = false;
        drc_set_config(audio_process.drc_st, cfg);
        audio_process.drc_enable = true;
    }else{
        audio_process.drc_enable = false;
    }

    return 0;
}
#endif

#ifdef __AUDIO_DRC2__
int audio_drc_2_set_cfg(const LimiterConfig *cfg)
{
    if(cfg){
        audio_process.drc2_enable = false;
        limiter_set_config(audio_process.drc2_st, cfg);
        audio_process.drc2_enable = true;
    }else{
        audio_process.drc2_enable = false;
    }

    return 0;
}
#endif

int SRAM_TEXT_LOC audio_process_run(uint8_t *buf, uint32_t len)
{
    int POSSIBLY_UNUSED pcm_len = 0;
    
    if(audio_process.sample_bits == AUD_BITS_16)
    {
        pcm_len = len / sizeof(pcm_16bits_t);
    }
    else if(audio_process.sample_bits == AUD_BITS_24)
    {
        pcm_len = len / sizeof(pcm_24bits_t);
    }
    else
    {
        ASSERT(0, "[%s] bits(%d) is invalid", __func__, audio_process.sample_bits);
    }

    //int32_t s_time,e_time;
    //s_time = hal_fast_sys_timer_get();

#ifdef __SW_IIR_EQ_PROCESS__
    if(audio_process.sw_iir_enable)
    {
        sw_iir_run(buf, pcm_len);
    }
#endif

#ifdef __HW_FIR_EQ_PROCESS__
    if(audio_process.hw_fir_enable)
    {
        fir_run(buf, pcm_len);
    }
#endif

#ifdef __AUDIO_DRC__
	if(audio_process.drc_enable)
	{
		drc_process(audio_process.drc_st, buf, pcm_len);
	}
#endif

    //int32_t m_time = hal_fast_sys_timer_get();

#ifdef __AUDIO_DRC2__
    if(audio_process.drc2_enable)
    {
        limiter_process(audio_process.drc2_st, buf, pcm_len);
    }
#endif

#ifdef __HW_IIR_EQ_PROCESS__
    if(audio_process.hw_iir_enable)
    {
        hw_iir_run(buf, pcm_len);
    }
#endif

    //e_time = hal_fast_sys_timer_get();
    //TRACE("[%s] Sample len = %d, drc1 %d us, drc2 %d us",
    //    __func__, pcm_len, FAST_TICKS_TO_US(m_time - s_time), FAST_TICKS_TO_US(e_time - m_time));

    return 0;
}

int audio_process_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_BITS_T sample_bits, enum AUD_CHANNEL_NUM_T ch_num, int32_t frame_size, void *eq_buf, uint32_t len)
{
    TRACE("[%s] sample_rate = %d, sample_bits = %d", __func__, sample_rate, sample_bits);

    audio_process.sample_rate = sample_rate;
    audio_process.sample_bits = sample_bits;
    audio_process.ch_num = ch_num;

#if defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
    void *fir_eq_buf = eq_buf;
    uint32_t fir_len = len/2;
    void *iir_eq_buf = (uint8_t *)eq_buf+fir_len;
    uint32_t iir_len = len/2;
#elif defined(__HW_FIR_EQ_PROCESS__) && !defined(__HW_IIR_EQ_PROCESS__)
    void *fir_eq_buf = eq_buf;
    uint32_t fir_len = len;
#elif !defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
    void *iir_eq_buf = eq_buf;
    uint32_t iir_len = len;  
#endif
    
#ifdef __SW_IIR_EQ_PROCESS__
    iir_open(sample_rate, sample_bits,ch_num);
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
#ifdef __AUDIO_RESAMPLE__
    enum AUD_SAMPRATE_T sample_rate_hw_dac_iir = AUD_SAMPRATE_50781;
#else
    enum AUD_SAMPRATE_T sample_rate_hw_dac_iir = sample_rate;
#endif
    hw_codec_iir_open(sample_rate_hw_dac_iir, HW_CODEC_IIR_DAC);
#endif

#ifdef __HW_IIR_EQ_PROCESS__
#ifdef __AUDIO_RESAMPLE__
    enum AUD_SAMPRATE_T sample_rate_hw_iir = AUD_SAMPRATE_50781;
#else
    enum AUD_SAMPRATE_T sample_rate_hw_iir = sample_rate;
#endif
    hw_iir_open(sample_rate_hw_iir, sample_bits, ch_num, iir_eq_buf, iir_len);
#endif

#ifdef __HW_FIR_EQ_PROCESS__
#if defined(CHIP_BEST1000) && defined(FIR_HIGHSPEED)
    hal_cmu_fir_high_speed_enable(HAL_CMU_FIR_USER_EQ);
#endif
    fir_open(sample_rate, sample_bits, ch_num, fir_eq_buf, fir_len);
#endif

#if AUDIO_MEMORY_SIZE > 0
    syspool_get_buff(&audio_process.audio_heap, AUDIO_MEMORY_SIZE);
    audio_heap_init(audio_process.audio_heap, AUDIO_MEMORY_SIZE);
#endif

#ifdef __AUDIO_DRC__
    audio_process.drc_st = drc_create(sample_rate, frame_size / ch_num, sample_bits, ch_num, &audio_drc_cfg);
    audio_process.drc_enable = true;
#endif

#ifdef __AUDIO_DRC2__
    audio_process.drc2_st = limiter_create(sample_rate, frame_size / ch_num, sample_bits, ch_num, &audio_drc2_cfg);
    audio_process.drc2_enable = true;
#endif

	return 0;
}

int audio_process_close(void)
{
#ifdef __SW_IIR_EQ_PROCESS__
    audio_process.sw_iir_enable = false;
    iir_close();
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
    audio_process.hw_dac_iir_enable = false;
    hw_codec_iir_close(HW_CODEC_IIR_DAC);
#endif

#ifdef __HW_IIR_EQ_PROCESS__
    audio_process.hw_iir_enable = false;
    hw_iir_close();
#endif

#ifdef __HW_FIR_EQ_PROCESS__
    audio_process.hw_fir_enable = false;
    fir_close();
#if defined(CHIP_BEST1000) && defined(FIR_HIGHSPEED)
    hal_cmu_fir_high_speed_disable(HAL_CMU_FIR_USER_EQ);
#endif
#endif

#ifdef __AUDIO_DRC__
    audio_process.drc_enable = false;
    drc_destroy(audio_process.drc_st);
#endif

#ifdef __AUDIO_DRC2__
    audio_process.drc2_enable = false;
    limiter_destroy(audio_process.drc2_st);
#endif

#if AUDIO_MEMORY_SIZE > 0
    size_t total = 0, used = 0, max_used = 0;
    audio_memory_info(&total, &used, &max_used);
    TRACE("AUDIO MALLOC MEM: total - %d, used - %d, max_used - %d.", total, used, max_used);
    ASSERT(used == 0, "[%s] used != 0", __func__);
#endif

    return 0;
}

#ifdef __PC_CMD_UART__
int audio_ping_callback(uint8_t *buf, uint32_t len)
{
    //TRACE("");
    return 0;
}

#ifdef AUDIO_EQ_SW_IIR_UPDATE_CFG
#ifndef USB_AUDIO_APP
int audio_eq_sw_iir_callback(uint8_t *buf, uint32_t  len)
{
    TRACE("[%s] len = %d, sizeof(struct) = %d", __func__, len, sizeof(IIR_CFG_T));

    if (len != sizeof(IIR_CFG_T))
    {
        return 1;
    }

    memcpy(&audio_process.sw_iir_cfg, buf, sizeof(IIR_CFG_T));
    TRACE("band num:%d gain0:%d, gain1:%d", 
                                                (int32_t)audio_process.sw_iir_cfg.num,
                                                (int32_t)(audio_process.sw_iir_cfg.gain0*10),      
                                                (int32_t)(audio_process.sw_iir_cfg.gain1*10));
    for (uint8_t i = 0; i<audio_process.sw_iir_cfg.num; i++){
        TRACE("band num:%d type %d gain:%d fc:%d q:%d", i,
			                           (int32_t)(audio_process.sw_iir_cfg.param[i].type), 
                                                (int32_t)(audio_process.sw_iir_cfg.param[i].gain*10), 
                                                (int32_t)(audio_process.sw_iir_cfg.param[i].fc*10), 
                                                (int32_t)(audio_process.sw_iir_cfg.param[i].Q*10));
    }

#ifdef __SW_IIR_EQ_PROCESS__
    {
        iir_set_cfg(&audio_process.sw_iir_cfg);
        audio_process.sw_iir_enable = true;
    }
#endif
   
    return 0;
}
#else
int audio_eq_sw_iir_callback(uint8_t *buf, uint32_t  len)
{
    // IIR_CFG_T *rx_iir_cfg = NULL;

    // rx_iir_cfg = (IIR_CFG_T *)buf;

    // TRACE("[%s] left gain = %f, right gain = %f", __func__, rx_iir_cfg->gain0, rx_iir_cfg->gain1);

    // for(int i=0; i<rx_iir_cfg->num; i++)
    // {
    //     TRACE("[%s] iir[%d] gain = %f, f = %f, Q = %f", __func__, i, rx_iir_cfg->param[i].gain, rx_iir_cfg->param[i].fc, rx_iir_cfg->param[i].Q);
    // }

    // audio_eq_set_cfg(NULL,(const IIR_CFG_T *)rx_iir_cfg,AUDIO_EQ_TYPE_SW_IIR);

    update_iir_cfg_tbl(buf, len);

    return 0;
}
#endif
#endif

#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
int audio_eq_hw_fir_callback(uint8_t *buf, uint32_t  len)
{
    TRACE("[%s] len = %d, sizeof(struct) = %d", __func__, len, sizeof(FIR_CFG_T));

    if (len != sizeof(FIR_CFG_T))
    {
        return 1;
    }

    FIR_CFG_T *rx_fir_cfg = NULL;

    rx_fir_cfg = (FIR_CFG_T *)buf;

    TRACE("[%s] left gain = %d, right gain = %d", __func__, rx_fir_cfg->gain0, rx_fir_cfg->gain1);

    TRACE("[%s] len = %d, coef: %d, %d......%d, %d", __func__, rx_fir_cfg->len, rx_fir_cfg->coef[0], rx_fir_cfg->coef[1], rx_fir_cfg->coef[rx_fir_cfg->len-2], rx_fir_cfg->coef[rx_fir_cfg->len-1]);

    rx_fir_cfg->gain0 = 6;
    rx_fir_cfg->gain1 = 6;

    if(rx_fir_cfg)
    {
        memcpy(&audio_process.fir_cfg, rx_fir_cfg, sizeof(audio_process.fir_cfg));
        audio_process.fir_enable = true;
        fir_set_cfg(&audio_process.fir_cfg);
    }
    else
    {
        audio_process.fir_enable = false;
    }
    
    return 0;
}
#endif

#ifdef AUDIO_DRC_UPDATE_CFG
int audio_drc_callback(uint8_t *buf, uint32_t  len)
{
	TRACE("[%s] len = %d, sizeof(struct) = %d", __func__, len, sizeof(DrcConfig));

    if (len != sizeof(DrcConfig))
    {
        return 1;
    }

	memcpy(&audio_process.drc_cfg, buf, sizeof(DrcConfig));
    audio_drc_1_set_cfg(&audio_process.drc_cfg);

    return 0;
}
#endif

#ifdef AUDIO_DRC2_UPDATE_CFG
int audio_drc2_callback(uint8_t *buf, uint32_t  len)
{
	TRACE("[%s] len = %d, sizeof(struct) = %d", __func__, len, sizeof(LimiterConfig));

    if (len != sizeof(LimiterConfig))
    {
        return 1;
    }

	memcpy(&audio_process.drc2_cfg, buf, sizeof(LimiterConfig));
	audio_drc_2_set_cfg(&audio_process.drc2_cfg);
#if 0
    TRACE("\n**********[%s]**********", __func__);
    TRACE("knee = %d",__func__, audio_process.drc2_cfg.knee);
	TRACE("look_ahead_time = %d", audio_process.drc2_cfg.look_ahead_time);
    TRACE("threshold = %d", audio_process.drc2_cfg.threshold);
    TRACE("makeup_gain = %f", audio_process.drc2_cfg.makeup_gain);
    TRACE("ratio = %d", audio_process.drc2_cfg.ratio);
    TRACE("attack_time = %d", audio_process.drc2_cfg.attack_time);
    TRACE("release_time = %d", audio_process.drc2_cfg.release_time);
    TRACE("**********[%s]**********\n", __func__);
#endif

    return 0;
}
#endif

#ifdef AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
int audio_eq_hw_dac_iir_callback(uint8_t *buf, uint32_t  len)
{
    TRACE("[%s] len = %d, sizeof(struct) = %d", __func__, len, sizeof(IIR_CFG_T));

    if (len != sizeof(IIR_CFG_T))
    {
        return 1;
    }

    memcpy(&audio_process.hw_dac_iir_cfg, buf, sizeof(IIR_CFG_T));
    TRACE("band num:%d gain0:%d, gain1:%d", 
                                                (int32_t)audio_process.hw_dac_iir_cfg.num,
                                                (int32_t)(audio_process.hw_dac_iir_cfg.gain0*10),      
                                                (int32_t)(audio_process.hw_dac_iir_cfg.gain1*10));
    for (uint8_t i = 0; i<audio_process.hw_dac_iir_cfg.num; i++){
        TRACE("band num:%d type %d gain:%d fc:%d q:%d", i,
			                           (int32_t)(audio_process.hw_dac_iir_cfg.param[i].type), 
                                                (int32_t)(audio_process.hw_dac_iir_cfg.param[i].gain*10), 
                                                (int32_t)(audio_process.hw_dac_iir_cfg.param[i].fc*10), 
                                                (int32_t)(audio_process.hw_dac_iir_cfg.param[i].Q*10));
    }

#ifdef __HW_DAC_IIR_EQ_PROCESS__
    {
        HW_CODEC_IIR_CFG_T *hw_iir_cfg_dac = NULL; 
#ifdef __AUDIO_RESAMPLE__
        enum AUD_SAMPRATE_T sample_rate_hw_iir = AUD_SAMPRATE_50781;
#else
        enum AUD_SAMPRATE_T sample_rate_hw_iir = audio_process.sample_rate;
#endif
        hw_iir_cfg_dac = hw_codec_iir_get_cfg(sample_rate_hw_iir,&audio_process.hw_dac_iir_cfg);
        ASSERT(hw_iir_cfg_dac != NULL, "[%s] %d codec IIR parameter error!", __func__, hw_iir_cfg_dac);
        
        // hal_codec_iir_dump(hw_iir_cfg_dac);

        hw_codec_iir_set_cfg(hw_iir_cfg_dac, sample_rate_hw_iir, HW_CODEC_IIR_DAC);
        audio_process.hw_dac_iir_enable = true;

    }
#endif
    
    return 0;
}
#endif


#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
int audio_eq_hw_iir_callback(uint8_t *buf, uint32_t  len)
{
    TRACE("[%s] len = %d, sizeof(struct) = %d", __func__, len, sizeof(IIR_CFG_T));

    if (len != sizeof(IIR_CFG_T))
    {
        return 1;
    }

    memcpy(&audio_process.hw_iir_cfg, buf, sizeof(IIR_CFG_T));
    TRACE("band num:%d gain0:%d, gain1:%d", 
                                                (int32_t)audio_process.hw_iir_cfg.num,
                                                (int32_t)(audio_process.hw_iir_cfg.gain0*10),      
                                                (int32_t)(audio_process.hw_iir_cfg.gain1*10));
    
    for (uint8_t i = 0; i<audio_process.hw_iir_cfg.num; i++)
    {
        TRACE("band num:%d type %d gain:%d fc:%d q:%d", i,
			                                    (int32_t)(audio_process.hw_iir_cfg.param[i].type), 
                                                (int32_t)(audio_process.hw_iir_cfg.param[i].gain*10), 
                                                (int32_t)(audio_process.hw_iir_cfg.param[i].fc*10), 
                                                (int32_t)(audio_process.hw_iir_cfg.param[i].Q*10));
    }

#ifdef __HW_IIR_EQ_PROCESS__
    {
        HW_IIR_CFG_T *hw_iir_cfg=NULL; 
#ifdef __AUDIO_RESAMPLE__
        enum AUD_SAMPRATE_T sample_rate_hw_iir=AUD_SAMPRATE_50781;
#else
        enum AUD_SAMPRATE_T sample_rate_hw_iir=audio_process.sample_rate;
#endif
        hw_iir_cfg = hw_iir_get_cfg(sample_rate_hw_iir,&audio_process.hw_iir_cfg);
        ASSERT(hw_iir_cfg != NULL, "[%s] %d codec IIR parameter error!", __func__, hw_iir_cfg);
        hw_iir_set_cfg(hw_iir_cfg);
        audio_process.hw_iir_enable = true;
    }
#endif
    
    return 0;
}
#endif
#endif // #ifdef __PC_CMD_UART__

typedef struct {
    uint8_t type;
    uint8_t maxEqBandNum;
    uint16_t sample_rate_num;
    uint8_t sample_rate[50];
} query_eq_info_t;


extern int getMaxEqBand(void);
extern int getSampleArray(uint8_t* buf, uint16_t *num);

extern void hal_cmd_set_res_playload(uint8_t* data, int len);
#define CMD_TYPE_QUERY_DUT_EQ_INFO  0x00
int audio_cmd_callback(uint8_t *buf, uint32_t len)
{
    uint8_t type;
    //uint32_t* sample_rate;
    //uint8_t* p;
    query_eq_info_t  info;

    type = buf[0];
    //p = buf + 1;

    TRACE("%s type: %d", __func__, type);
    switch (type) {
        case CMD_TYPE_QUERY_DUT_EQ_INFO:
            info.type = CMD_TYPE_QUERY_DUT_EQ_INFO;
            info.maxEqBandNum = getMaxEqBand();
            getSampleArray(info.sample_rate, &info.sample_rate_num);

            hal_cmd_set_res_playload((uint8_t*)&info, 4 + info.sample_rate_num * 4);
            break;
        default:
            break;
    }

    return 0;
}

#ifdef AUDIO_SECTION_ENABLE
int audio_cfg_burn_callback(uint8_t *buf, uint32_t  len)
{
    TRACE("[%s] len = %d, sizeof(struct) = %d", __func__, len, sizeof_audio_cfg());

    if (len != sizeof_audio_cfg())
    {
        return 1;
    }

    int res = 0;
    res = store_audio_cfg_into_audio_section((AUDIO_CFG_T *)buf);

    if(res)
    {
        TRACE("[%s] ERROR: res = %d", __func__, res);
        res += 100;
    }
    else
    {
        TRACE("[%s] Store audio cfg into audio section!!!", __func__);
    }

    return res;
}
#endif

int audio_process_init(void)
{ 
#ifdef __PC_CMD_UART__
    hal_cmd_init();

#ifdef AUDIO_EQ_SW_IIR_UPDATE_CFG
    hal_cmd_register("iir_eq", audio_eq_sw_iir_callback);           // Will be removed
    hal_cmd_register("sw_iir_eq", audio_eq_sw_iir_callback);
#endif

#ifdef AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
    hal_cmd_register("iir_eq", audio_eq_hw_dac_iir_callback);       // Will be removed
    hal_cmd_register("dac_iir_eq", audio_eq_hw_dac_iir_callback);
#endif

#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
    hal_cmd_register("iir_eq", audio_eq_hw_iir_callback);           // Will be removed
    hal_cmd_register("hw_iir_eq", audio_eq_hw_iir_callback);
#endif

#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
    hal_cmd_register("fir_eq", audio_eq_hw_fir_callback);
#endif

#ifdef AUDIO_DRC_UPDATE_CFG
    hal_cmd_register("drc", audio_drc_callback);
#endif

#ifdef AUDIO_DRC2_UPDATE_CFG
    hal_cmd_register("limiter", audio_drc2_callback);
#endif

#ifdef AUDIO_SECTION_ENABLE
    hal_cmd_register("burn", audio_cfg_burn_callback);
    hal_cmd_register("audio_burn", audio_cfg_burn_callback);
#endif

    hal_cmd_register("cmd", audio_cmd_callback);
    hal_cmd_register("ping", audio_ping_callback);
#endif

    return 0;
}
