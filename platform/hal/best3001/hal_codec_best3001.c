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
#include "reg_codec_best3001.h"
#include "hal_codec.h"
#include "hal_chipid.h"
#include "hal_cmu.h"
#include "hal_aud.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "analog.h"
#include "cmsis.h"
#include "tgt_hardware.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif

#ifndef DAC_RAMP_STEP
#define DAC_RAMP_STEP       0
#endif

#ifndef DAC_RAMP_INTERVAL
#define DAC_RAMP_INTERVAL   0
#endif

#define NO_CODEC_RESET

#define HAL_CODEC_TX_FIFO_TRIGGER_LEVEL     (3)
#define HAL_CODEC_RX_FIFO_TRIGGER_LEVEL     (4)

#define MAX_DIG_DAC_DBVAL                   (50)
#define ZERODB_DIG_DAC_DBVAL                (0)
#define MIN_DIG_DAC_DBVAL                   (-99)

#ifndef CODEC_DIGMIC_PHASE
#define CODEC_DIGMIC_PHASE                  7
#endif

#define MAX_DIG_MIC_CH_NUM                  2
#define NORMAL_ADC_CH_NUM                   2
#define MAX_ADC_CH_NUM                      NORMAL_ADC_CH_NUM

#define MAX_DAC_CH_NUM                      2

#define NORMAL_MIC_MAP                      (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)
#define VALID_MIC_MAP                       NORMAL_MIC_MAP

#define NORMAL_ADC_MAP                      (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)
#define VALID_ADC_MAP                       NORMAL_ADC_MAP

#define VALID_SPK_MAP                       (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)
#define VALID_DAC_MAP                       VALID_SPK_MAP

#if (CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV & ~VALID_SPK_MAP)
#error "Invalid CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV"
#endif

struct CODEC_DAC_DRE_CFG_T {
    uint8_t dre_delay;
    uint8_t thd_db_offset;
    uint8_t step_mode;
    uint32_t dre_win;
    uint16_t amp_high;
};

struct CODEC_DAC_SAMPLE_RATE_T {
    enum AUD_SAMPRATE_T sample_rate;
    uint32_t    codec_freq;
    uint8_t     osr_sel;
    uint8_t     dac_up;
    uint16_t    bypass_cnt;
    uint32_t    resample_phase;
};

#define DAC_RSMPL_PHASE_48K			0x4189374c
#define DAC_RSMPL_PHASE_44P1K		0x3c361134

static const struct CODEC_DAC_SAMPLE_RATE_T codec_dac_sample_rate[] = {
    {AUD_SAMPRATE_8000,     CODEC_FREQ_24M,     2,      6,      0,      DAC_RSMPL_PHASE_48K},
    {AUD_SAMPRATE_16000,    CODEC_FREQ_24M,     2,      3,      0,      DAC_RSMPL_PHASE_48K},
    {AUD_SAMPRATE_24000,    CODEC_FREQ_24M,     2,      2,      0,      DAC_RSMPL_PHASE_48K},
    
    {AUD_SAMPRATE_44100,    CODEC_FREQ_24M,     2,      1,      0,      DAC_RSMPL_PHASE_44P1K},
    {AUD_SAMPRATE_48000,    CODEC_FREQ_24M,     2,      1,      0,      DAC_RSMPL_PHASE_48K},

    // high-quality config for 88.2k/96k
    {AUD_SAMPRATE_88200,    CODEC_FREQ_24M,     1,      1,      0,      DAC_RSMPL_PHASE_44P1K},
    {AUD_SAMPRATE_96000,    CODEC_FREQ_24M,     1,      1,      0,      DAC_RSMPL_PHASE_48K},
    
/*
    // low-latency config for 88.2k/96k
    {AUD_SAMPRATE_88200,    CODEC_FREQ_24M,     2,      1,      1,      DAC_RSMPL_PHASE_44P1K},
    {AUD_SAMPRATE_96000,    CODEC_FREQ_24M,     2,      1,      1,      DAC_RSMPL_PHASE_48K},
*/

    // high-quality config for 176.4k/192k
    {AUD_SAMPRATE_176400,   CODEC_FREQ_24M,     0,      1,      0,      DAC_RSMPL_PHASE_44P1K},
    {AUD_SAMPRATE_192000,   CODEC_FREQ_24M,     0,      1,      0,      DAC_RSMPL_PHASE_48K},

/*
    // low-latency config for 176.4k/192k
    {AUD_SAMPRATE_176400,   CODEC_FREQ_24M,     2,      1,      2,      DAC_RSMPL_PHASE_44P1K},
    {AUD_SAMPRATE_192000,   CODEC_FREQ_24M,     2,      1,      2,      DAC_RSMPL_PHASE_48K},
*/

/*
    // compromised config for 176.4k/192k
    {AUD_SAMPRATE_176400,   CODEC_FREQ_24M,     1,      1,      1,      DAC_RSMPL_PHASE_44P1K},
    {AUD_SAMPRATE_192000,   CODEC_FREQ_24M,     1,      1,      1,      DAC_RSMPL_PHASE_48K},
*/

    // high-quality config for 352.8k/384k
    {AUD_SAMPRATE_352800,   CODEC_FREQ_24M,     0,      1,      1,      DAC_RSMPL_PHASE_44P1K},
    {AUD_SAMPRATE_384000,   CODEC_FREQ_24M,     0,      1,      1,      DAC_RSMPL_PHASE_48K},
    
/*
    // low-latency config for 176.4k/192k
    {AUD_SAMPRATE_352800,   CODEC_FREQ_24M,     2,      1,      3,      DAC_RSMPL_PHASE_44P1K},
    {AUD_SAMPRATE_384000,   CODEC_FREQ_24M,     2,      1,      3,      DAC_RSMPL_PHASE_48K},

    // compromised config for 176.4k/192k
    {AUD_SAMPRATE_352800,   CODEC_FREQ_24M,     1,      1,      2,      0x3c361134},
    {AUD_SAMPRATE_384000,   CODEC_FREQ_24M,     1,      1,      2,      DAC_RSMPL_PHASE_48K},
*/

};

struct CODEC_ADC_SAMPLE_RATE_T {
    enum AUD_SAMPRATE_T sample_rate;
    uint32_t    codec_freq;
    uint8_t     adc_down;
    uint8_t     bypass_cnt;
    uint32_t    resample_phase;
};

#define ADC_RSMPL_PHASE_48K			0x3e800000
#define ADC_RSMPL_PHASE_44P1K		0x4406f74b

static const struct CODEC_ADC_SAMPLE_RATE_T codec_adc_sample_rate[] = {
    {AUD_SAMPRATE_8000,     CODEC_FREQ_24M,     6,      0,      ADC_RSMPL_PHASE_48K},
    {AUD_SAMPRATE_16000,    CODEC_FREQ_24M,     3,      0,      ADC_RSMPL_PHASE_48K},

    {AUD_SAMPRATE_44100,    CODEC_FREQ_24M,     1,      0,      ADC_RSMPL_PHASE_44P1K},
    {AUD_SAMPRATE_48000,    CODEC_FREQ_24M,     1,      0,      ADC_RSMPL_PHASE_48K},

    {AUD_SAMPRATE_88200,    CODEC_FREQ_24M,     1,      1,      ADC_RSMPL_PHASE_44P1K},
    {AUD_SAMPRATE_96000,    CODEC_FREQ_24M,     1,      1,      ADC_RSMPL_PHASE_48K},

    {AUD_SAMPRATE_176400,   CODEC_FREQ_24M,     1,      2,      ADC_RSMPL_PHASE_44P1K},
    {AUD_SAMPRATE_192000,   CODEC_FREQ_24M,     1,      2,      ADC_RSMPL_PHASE_48K},

    {AUD_SAMPRATE_352800,   CODEC_FREQ_24M,     1,      3,      ADC_RSMPL_PHASE_44P1K},
    {AUD_SAMPRATE_384000,   CODEC_FREQ_24M,     1,      3,      ADC_RSMPL_PHASE_48K},
};

#if 0
static const uint32_t pdm_mux[MAX_ANA_MIC_CH_NUM] = {
    CODEC_PDM_CH0_ADC_SEL,
    CODEC_PDM_CH1_ADC_SEL,
};
#endif

const CODEC_ADC_VOL_T WEAK codec_adc_vol[TGT_ADC_VOL_LEVEL_QTY] = {
    -99, -12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12, 14, 16,
};

static struct CODEC_T * const codec = (struct CODEC_T *)CODEC_BASE;

static bool codec_init = false;
static bool codec_opened = false;

static int8_t digdac_gain[MAX_DAC_CH_NUM];
static int8_t digadc_gain[NORMAL_ADC_CH_NUM];

static bool codec_mute[AUD_STREAM_NUM];

enum HAL_CODEC_OPEN_T {
    HAL_CODEC_OPEN_NORMAL       = (1 << 0),
};

#if defined(NOISE_GATING) && defined(NOISE_REDUCTION)
static bool codec_nr_enabled;
static int8_t digdac_gain_offset_nr;
#endif

#ifdef AUDIO_OUTPUT_DC_CALIB
//static int16_t dac_dc_l;
//static int16_t dac_dc_r;
//static float dac_dc_gain_attn;
int16_t dac_dc_l;
int16_t dac_dc_r;
float dac_dc_gain_attn;
#endif

//static HAL_CODEC_DAC_RESET_CALLBACK dac_reset_callback;

static enum AUD_CHANNEL_MAP_T codec_dac_ch_map;
static enum AUD_CHANNEL_MAP_T codec_adc_ch_map;
//static enum AUD_CHANNEL_MAP_T codec_mic_ch_map;

static uint8_t dac_delay_ms;
static uint8_t adc_delay_ms;

#if defined(AUDIO_ANC_FB_MC) && defined(__AUDIO_RESAMPLE__)
#error "Music cancel cannot work with audio resample"
#endif
#if defined(AUDIO_ANC_FB_MC) && defined(__TWS__)
#error "Music cancel cannot work with TWS")
#endif

#ifdef PERF_TEST_POWER_KEY
static enum HAL_CODEC_PERF_TEST_POWER_T cur_perft_power;
#endif

#ifdef AUDIO_OUTPUT_SW_GAIN
static HAL_CODEC_SW_OUTPUT_COEF_CALLBACK sw_output_coef_callback;
#endif

#if defined(DAC_CLASSG_ENABLE)
static struct dac_classg_cfg _dac_classg_cfg = {
    .thd2 = 0x80,//0xC0,
    .thd1 = 0x10,
    .thd0 = 0x10,
    .lr = 1,
    .step_4_3n = 0,
    .quick_down = 1,
    .wind_width = 0x400,
};
#endif

#ifdef DAC_DRE_ENABLE
static const struct CODEC_DAC_DRE_CFG_T dac_dre_cfg = {
    .dre_delay = 8,
    .thd_db_offset = 0xF, //5,
    .step_mode = 0,
    .dre_win = 0x6000,
    .amp_high = 2, //0x10,
};
#endif

static void hal_codec_set_dig_adc_gain(enum AUD_CHANNEL_MAP_T map, int32_t gain);
static void hal_codec_set_dig_dac_gain(enum AUD_CHANNEL_MAP_T map, int32_t gain);
static void hal_codec_restore_dig_dac_gain(void);
static void hal_codec_set_dac_gain_value(enum AUD_CHANNEL_MAP_T map, uint32_t val);
static void hal_codec_set_adc_gain_value(enum AUD_CHANNEL_MAP_T map, uint32_t val);
#ifdef AUDIO_OUTPUT_SW_GAIN
static void hal_codec_set_sw_gain(int32_t gain);
#endif

static void hal_codec_reg_update_delay(void)
{
    for (int i = 0; i < 10; i++) {
        asm volatile("nop");
    }
}

#if defined(DAC_CLASSG_ENABLE)
void hal_codec_classg_config(const struct dac_classg_cfg *cfg)
{
    _dac_classg_cfg = *cfg;
}

static void hal_codec_classg_enable(bool en)
{
    struct dac_classg_cfg *config;

    if (en) {
        config = &_dac_classg_cfg;

        codec->CLASSG_THD = SET_BITFIELD(codec->CLASSG_THD, CODEC_CLASSG_THD2, config->thd2);
        codec->CLASSG_THD = SET_BITFIELD(codec->CLASSG_THD, CODEC_CLASSG_THD1, config->thd1);
        codec->CLASSG_THD = SET_BITFIELD(codec->CLASSG_THD, CODEC_CLASSG_THD0, config->thd0);

        codec->CLASSG_CFG = SET_BITFIELD(codec->CLASSG_CFG, CODEC_CLASSG_WINDOW, config->wind_width);

        if (config->lr)
            codec->CLASSG_CFG |= CODEC_CLASSG_LR;
        else
            codec->CLASSG_CFG &= ~CODEC_CLASSG_LR;

        if (config->step_4_3n)
            codec->CLASSG_CFG |= CODEC_CLASSG_STEP_4_3N;
        else
            codec->CLASSG_CFG &= ~CODEC_CLASSG_STEP_4_3N;

        if (config->quick_down)
            codec->CLASSG_CFG |= CODEC_CLASSG_QUICK_DOWN;
        else
            codec->CLASSG_CFG &= ~CODEC_CLASSG_QUICK_DOWN;

        codec->CLASSG_CFG |= CODEC_CLASSG_EN;
    } else {
        codec->CLASSG_CFG &= ~CODEC_CLASSG_EN;
    }
}
#endif

#ifdef AUDIO_OUTPUT_DC_CALIB
static void hal_codec_dac_dc_offset_enable(int16_t dc_l, int16_t dc_r)
{
    codec->DAC_GAIN_CFG &= ~(CODEC_DAC_CH1_DC_UPDATE | CODEC_DAC_CH0_DC_UPDATE);
    
    hal_codec_reg_update_delay();
    
    codec->DAC_DC_CFG = SET_BITFIELD(codec->DAC_DC_CFG, CODEC_DAC_CH0_DC, dc_l);
    codec->DAC_DC_CFG = SET_BITFIELD(codec->DAC_DC_CFG, CODEC_DAC_CH1_DC, dc_r);
    
    codec->DAC_GAIN_CFG |= (CODEC_DAC_CH1_DC_UPDATE | CODEC_DAC_CH0_DC_UPDATE);
}
#endif

static int hal_codec_int_open(enum HAL_CODEC_OPEN_T type)
{
    int i;

    TRACE("%s: type[%d]", __func__, type);

    if (!codec_init) {
        for (i = 0; i < CFG_HW_AUD_INPUT_PATH_NUM; i++) {
            if (cfg_audio_input_path_cfg[i].cfg & AUD_CHANNEL_MAP_ALL & ~VALID_MIC_MAP) {
                ASSERT(false, "Invalid input path cfg: i=%d io_path=%d cfg=0x%X",
                    i, cfg_audio_input_path_cfg[i].io_path, cfg_audio_input_path_cfg[i].cfg);
            }
        }
        codec_init = true;
    }

    hal_cmu_codec_clock_enable();
    hal_cmu_codec_reset_clear();

    codec_opened = true;

    codec->SOFT_RSTN |= CODEC_SOFT_RSTN_IIR | CODEC_SOFT_RSTN_RS | CODEC_SOFT_RSTN_DAC | CODEC_SOFT_RSTN_ADC_MASK | CODEC_SOFT_RSTN_ADC_ANA_MASK;
    codec->CONFIG &= ~(CODEC_CFG_ADC_ENABLE | CODEC_CFG_DAC_ENABLE | CODEC_CFG_DMACTRL_RX | CODEC_CFG_DMACTRL_TX);
    
    codec->FIFO_FLUSH |= (CODEC_FIFO_FLUSH_TX_FIFO_FLUSH | CODEC_FIFO_FLUSH_RX_FIFO_FLUSH_CH0 | CODEC_FIFO_FLUSH_RX_FIFO_FLUSH_CH1 | CODEC_FIFO_FLUSH_DSD_RX_FIFO_FLUSH | CODEC_FIFO_FLUSH_DSD_TX_FIFO_FLUSH);
    hal_codec_reg_update_delay();
    codec->FIFO_FLUSH &= ~(CODEC_FIFO_FLUSH_TX_FIFO_FLUSH | CODEC_FIFO_FLUSH_RX_FIFO_FLUSH_CH0 | CODEC_FIFO_FLUSH_RX_FIFO_FLUSH_CH1 | CODEC_FIFO_FLUSH_DSD_RX_FIFO_FLUSH | CODEC_FIFO_FLUSH_DSD_TX_FIFO_FLUSH);    

    // to enable CODEC interface
    codec->CONFIG |= CODEC_CFG_CODEC_IF_EN;

    codec->ADC0_CFG |= CODEC_ADC_CH0_GAIN_SEL;
    codec->ADC1_CFG |= CODEC_ADC_CH1_GAIN_SEL;

    // Enable sync stamp auto clear
    codec->SYNC_CFG |= CODEC_SYNC_STAMP_CLR_USED;

#ifdef FIXED_CODEC_ADC_VOL
    const CODEC_ADC_VOL_T *adc_gain_db;

    adc_gain_db = hal_codec_get_adc_volume(CODEC_SADC_VOL);
    if (adc_gain_db) {
        hal_codec_set_dig_adc_gain(NORMAL_ADC_MAP, *adc_gain_db);
    }
#endif

#ifdef AUDIO_OUTPUT_DC_CALIB
    hal_codec_dac_dc_offset_enable(dac_dc_l, dac_dc_r);
#endif

#ifdef AUDIO_OUTPUT_SW_GAIN
    const struct CODEC_DAC_VOL_T *vol_tab_ptr;

    // Init gain settings
    vol_tab_ptr = hal_codec_get_dac_volume(0);
    if (vol_tab_ptr) {
        analog_aud_set_dac_gain(vol_tab_ptr->tx_pa_gain);
        hal_codec_set_dig_dac_gain(VALID_DAC_MAP, ZERODB_DIG_DAC_DBVAL);
    }
#else
    // Enable DAC zero-crossing gain adjustment
    codec->DAC0_GAIN_CFG |= CODEC_DAC_CH0_GAIN_SEL;
    codec->DAC1_GAIN_CFG |= CODEC_DAC_CH1_GAIN_SEL;
#endif
    hal_codec_set_dac_gain_value(VALID_DAC_MAP, 0);

    // disable Resampler
    codec->RSPL_CFG &= ~(CODEC_RSPL_DAC_EN | CODEC_RSPL_ADC_EN | CODEC_RSPL_DAC_PHASE_UPD | CODEC_RSPL_ADC_PHASE_UPD);

    return 0;
}

int hal_codec_open(enum HAL_CODEC_ID_T id)
{
    return hal_codec_int_open(HAL_CODEC_OPEN_NORMAL);
}

int hal_codec_int_close(enum HAL_CODEC_OPEN_T type)
{
    // to disable the codec interface 
    codec->CONFIG &= ~CODEC_CFG_CODEC_IF_EN;

    // to stop the clock of the CODEC
    hal_cmu_clock_disable(HAL_CMU_MOD_O_CODEC);
    
    // to reset the CODEC
    hal_cmu_reset_set(HAL_CMU_MOD_O_CODEC);

    codec_opened = false;

    // AUDIO PLL Close
    analog_aud_pll_close(ANA_AUD_PLL_USER_CODEC);

    return 0;
}

int hal_codec_close(enum HAL_CODEC_ID_T id)
{
    return hal_codec_int_close(HAL_CODEC_OPEN_NORMAL);
}

#ifdef DAC_DRE_ENABLE
static void hal_codec_dac_dre_enable(void)
{
    codec->DAC0_DRE_CFG0 = (codec->DAC0_DRE_CFG0 & ~(CODEC_DAC_CH0_DRE_THD_DB_OFFSET_SIGN | CODEC_DAC_CH0_DRE_DELAY_MASK |
            CODEC_DAC_CH0_DRE_INI_ANA_GAIN_MASK | CODEC_DAC_CH0_DRE_THD_DB_OFFSET_MASK | CODEC_DAC_CH0_DRE_STEP_MODE_MASK)) |
        CODEC_DAC_CH0_DRE_DELAY(dac_dre_cfg.dre_delay) |
        CODEC_DAC_CH0_DRE_INI_ANA_GAIN(0xF) | CODEC_DAC_CH0_DRE_THD_DB_OFFSET(dac_dre_cfg.thd_db_offset) |
        CODEC_DAC_CH0_DRE_STEP_MODE(dac_dre_cfg.step_mode) | CODEC_DAC_CH0_DRE_EN;
    
    codec->DAC0_DRE_CFG1 = (codec->DAC0_DRE_CFG1 & ~(CODEC_DAC_CH0_DRE_WINDOW_MASK | CODEC_DAC_CH0_DRE_AMP_HIGH_MASK)) |
        CODEC_DAC_CH0_DRE_WINDOW(dac_dre_cfg.dre_win) | CODEC_DAC_CH0_DRE_AMP_HIGH(dac_dre_cfg.amp_high);

    codec->DAC1_DRE_CFG0 = (codec->DAC1_DRE_CFG0 & ~(CODEC_DAC_CH1_DRE_THD_DB_OFFSET_SIGN | CODEC_DAC_CH1_DRE_DELAY_MASK |
            CODEC_DAC_CH1_DRE_INI_ANA_GAIN_MASK | CODEC_DAC_CH1_DRE_THD_DB_OFFSET_MASK | CODEC_DAC_CH1_DRE_STEP_MODE_MASK)) |
        CODEC_DAC_CH1_DRE_DELAY(dac_dre_cfg.dre_delay) |
        CODEC_DAC_CH1_DRE_INI_ANA_GAIN(0xF) | CODEC_DAC_CH1_DRE_THD_DB_OFFSET(dac_dre_cfg.thd_db_offset) |
        CODEC_DAC_CH1_DRE_STEP_MODE(dac_dre_cfg.step_mode) | CODEC_DAC_CH1_DRE_EN;

    codec->DAC1_DRE_CFG1 = (codec->DAC1_DRE_CFG1 & ~(CODEC_DAC_CH1_DRE_WINDOW_MASK | CODEC_DAC_CH1_DRE_AMP_HIGH_MASK)) |
        CODEC_DAC_CH1_DRE_WINDOW(dac_dre_cfg.dre_win) | CODEC_DAC_CH1_DRE_AMP_HIGH(dac_dre_cfg.amp_high);
}

static void hal_codec_dac_dre_disable(void)
{
    codec->DAC0_DRE_CFG0 &= ~CODEC_DAC_CH0_DRE_EN;
    codec->DAC1_DRE_CFG0 &= ~CODEC_DAC_CH1_DRE_EN;
}
#endif

#ifdef PERF_TEST_POWER_KEY
static void hal_codec_update_perf_test_power(void)
{
    int32_t nominal_vol;
    uint32_t ini_ana_gain;
    int32_t dac_vol;

    if (!codec_opened) {
        return;
    }

    dac_vol = 0;
    if (cur_perft_power == HAL_CODEC_PERF_TEST_30MW) {
        nominal_vol = 0;
        ini_ana_gain = 0;
    } else if (cur_perft_power == HAL_CODEC_PERF_TEST_10MW) {
        nominal_vol = -5;
        ini_ana_gain = 7;
    } else if (cur_perft_power == HAL_CODEC_PERF_TEST_5MW) {
        nominal_vol = -8;
        ini_ana_gain = 0xA;
    } else if (cur_perft_power == HAL_CODEC_PERF_TEST_M60DB) {
        nominal_vol = -60;
        ini_ana_gain = 0xF; // about -11 dB
        dac_vol = -49;
    } else {
        return;
    }

    if (codec->DAC0_DRE_CFG0 & CODEC_DAC_CH0_DRE_EN) {
        dac_vol = nominal_vol;
    } else {
        codec->DAC0_DRE_CFG0 = SET_BITFIELD(codec->DAC0_DRE_CFG0, CODEC_DAC_CH0_DRE_INI_ANA_GAIN, ini_ana_gain);
        codec->DAC1_DRE_CFG0 = SET_BITFIELD(codec->DAC1_DRE_CFG0, CODEC_DAC_CH1_DRE_INI_ANA_GAIN, ini_ana_gain);
    }

#ifdef AUDIO_OUTPUT_SW_GAIN
    hal_codec_set_sw_gain(dac_vol);
#else
    hal_codec_set_dig_dac_gain(VALID_DAC_MAP, dac_vol);
#endif

#if defined(NOISE_GATING) && defined(NOISE_REDUCTION)
    if (codec_nr_enabled) {
        codec_nr_enabled = false;
        hal_codec_set_noise_reduction(true);
    }
#endif
}

void hal_codec_dac_gain_m60db_check(enum HAL_CODEC_PERF_TEST_POWER_T type)
{
    cur_perft_power = type;

    if (!codec_opened || (codec->REG32B_04 & CODEC_04_DAC_EN) == 0) {
        return;
    }

    hal_codec_update_perf_test_power();
}
#endif

#if defined(NOISE_GATING) && defined(NOISE_REDUCTION)
void hal_codec_set_noise_reduction(bool enable)
{
    uint32_t ini_ana_gain;

    if (codec_nr_enabled == enable) {
        // Avoid corrupting digdac_gain_offset_nr or using an invalid one
        return;
    }

    codec_nr_enabled = enable;

    if (!codec_opened) {
        return;
    }

    // ini_ana_gain=0   -->   0dB
    // ini_ana_gain=0xF --> -11dB
    if (enable) {
        ini_ana_gain = GET_BITFIELD(codec->DAC0_DRE_CFG0, CODEC_DAC_CH0_DRE_INI_ANA_GAIN);
        digdac_gain_offset_nr = ((0xF - ini_ana_gain) * 11 + 0xF / 2) / 0xF;
        ini_ana_gain = 0xF;
    } else {
        ini_ana_gain = 0xF - (digdac_gain_offset_nr * 0xF + 11 / 2) / 11;
        digdac_gain_offset_nr = 0;
    }

    codec->DAC0_DRE_CFG0 = SET_BITFIELD(codec->DAC0_DRE_CFG0, CODEC_DAC_CH0_DRE_INI_ANA_GAIN, ini_ana_gain);
    codec->DAC1_DRE_CFG0 = SET_BITFIELD(codec->DAC1_DRE_CFG0, CODEC_DAC_CH1_DRE_INI_ANA_GAIN, ini_ana_gain);

#ifdef AUDIO_OUTPUT_SW_GAIN
    hal_codec_set_sw_gain(digdac_gain[0]);
#else
    hal_codec_restore_dig_dac_gain();
#endif
}
#endif

void hal_codec_stop_playback_stream(enum HAL_CODEC_ID_T id)
{
    TRACE("hal_codec_stop_playback_stream");

#if (defined(AUDIO_OUTPUT_DC_CALIB_ANA) || defined(AUDIO_OUTPUT_DC_CALIB)) && (!defined(__TWS__) || defined(__BT_ANC__))
    // Disable PA
    analog_aud_codec_speaker_enable(false);
#endif

    codec->DAC_CFG &= ~(CODEC_DAC_CH0_EN | CODEC_DAC_CH1_EN | CODEC_DAC_EN);
    codec->RSPL_CFG &= ~(CODEC_RSPL_DAC_R_EN | CODEC_RSPL_DAC_L_EN | CODEC_RSPL_DAC_EN);

#ifdef DAC_DRE_ENABLE
    hal_codec_dac_dre_disable();
#endif

#if defined(DAC_CLASSG_ENABLE)
    hal_codec_classg_enable(false);
#endif

    hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECDA0);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECDA1);

    hal_cmu_reset_set(HAL_CMU_MOD_O_CODECDA0);    
    hal_cmu_reset_set(HAL_CMU_MOD_O_CODECDA1);

	hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECRS);
	hal_cmu_reset_set(HAL_CMU_MOD_O_CODECRS);

#ifndef NO_CODEC_RESET
    // Reset DAC
    // Avoid DAC outputing noise after it is disabled
    codec->SOFT_RSTN &= ~CODEC_SOFT_RSTN_DAC;
    codec->SOFT_RSTN |= CODEC_SOFT_RSTN_DAC;
#endif
}

void hal_codec_start_playback_stream(enum HAL_CODEC_ID_T id)
{
    TRACE("%s: dac_ch[%x]", __func__, codec_dac_ch_map);

#ifndef NO_CODEC_RESET
    // Reset DAC
    codec->SOFT_RSTN &= ~CODEC_SOFT_RSTN_DAC;
    codec->SOFT_RSTN |= CODEC_SOFT_RSTN_DAC;
#endif

#ifdef DAC_DRE_ENABLE
    //if ((codec->DAC_MODE & CODEC_DAC_MODE_16BITS) == 0)
    {
        hal_codec_dac_dre_enable();
    }
#endif

#ifdef PERF_TEST_POWER_KEY
    hal_codec_update_perf_test_power();
#endif

#if defined(DAC_CLASSG_ENABLE)
    hal_codec_classg_enable(true);
#endif

    hal_cmu_clock_enable(HAL_CMU_MOD_O_CODECRS);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_CODECRS);

    codec->REG32B_040 |= 1<<8;
    
    if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH0) {
        hal_cmu_clock_enable(HAL_CMU_MOD_O_CODECDA0);  
        hal_cmu_reset_clear(HAL_CMU_MOD_O_CODECDA0);
        
        codec->RSPL_CFG |= CODEC_RSPL_DAC_L_EN;        
        codec->DAC_CFG |= CODEC_DAC_CH0_EN;
    } else {
        codec->DAC_CFG &= ~CODEC_DAC_CH0_EN;

        hal_cmu_reset_set(HAL_CMU_MOD_O_CODECDA0);
        hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECDA0);
    }
    
    if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH1) {
        hal_cmu_clock_enable(HAL_CMU_MOD_O_CODECDA1);
        hal_cmu_reset_clear(HAL_CMU_MOD_O_CODECDA1);

        codec->RSPL_CFG |= CODEC_RSPL_DAC_R_EN;
        codec->DAC_CFG |= CODEC_DAC_CH1_EN;
    } else {
        codec->DAC_CFG &= ~CODEC_DAC_CH1_EN;

        hal_cmu_reset_set(HAL_CMU_MOD_O_CODECDA1);
        hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECDA1);
    }

    codec->RSPL_CFG |= CODEC_RSPL_DAC_EN;
    codec->DAC_CFG |= CODEC_DAC_EN;

#if (defined(AUDIO_OUTPUT_DC_CALIB_ANA) || defined(AUDIO_OUTPUT_DC_CALIB)) && (!defined(__TWS__) || defined(__BT_ANC__))
#ifdef CHIP_BEST3001
	// zz 190122
	//hal_sys_timer_delay(MS_TO_TICKS(40));
#endif
    // Enable PA
     analog_aud_codec_speaker_enable(true);
#endif
}

int hal_codec_start_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream)
{
    TRACE("%s: stream[%d]", __func__, stream);

    if (stream == AUD_STREAM_PLAYBACK) {
        // Reset and start DAC
        hal_codec_start_playback_stream(id);
    } else {
#ifndef NO_CODEC_RESET
        codec->SOFT_RSTN &= ~CODEC_SOFT_RSTN_ADC_ALL;
        codec->SOFT_RSTN |= CODEC_SOFT_RSTN_ADC_ALL;
#endif

		hal_cmu_clock_enable(HAL_CMU_MOD_O_CODECRS);
        hal_cmu_reset_clear(HAL_CMU_MOD_O_CODECRS);

        hal_cmu_clock_enable(HAL_CMU_MOD_O_CODECADC);
        hal_cmu_reset_clear(HAL_CMU_MOD_O_CODECADC);
        
        if ((codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0|AUD_CHANNEL_MAP_CH1)) == (AUD_CHANNEL_MAP_CH0|AUD_CHANNEL_MAP_CH1)) {
            hal_cmu_clock_enable(HAL_CMU_MOD_O_CODECAD0);
            hal_cmu_clock_enable(HAL_CMU_MOD_O_CODECAD1);
            
            hal_cmu_reset_clear(HAL_CMU_MOD_O_CODECAD0);
            hal_cmu_reset_clear(HAL_CMU_MOD_O_CODECAD1);
            
            codec->ADC_CFG |= CODEC_ADC_CH0_EN | CODEC_ADC_CH1_EN;
            codec->RSPL_CFG |= CODEC_RSPL_ADC_DUAL_CH;
        } else if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH0) {
            codec->ADC_CFG |= CODEC_ADC_CH0_EN;
            codec->RSPL_CFG = ~CODEC_RSPL_ADC_DUAL_CH;
        }

        codec->RSPL_CFG |= CODEC_RSPL_ADC_EN;
        codec->ADC_CFG |= CODEC_ADC_EN;
    }

    return 0;
}

int hal_codec_stop_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream)
{

    if (stream == AUD_STREAM_PLAYBACK) {
        // Stop and reset DAC
        hal_codec_stop_playback_stream(id);
    } else {
        codec->ADC_CFG &= ~(CODEC_ADC_EN | CODEC_ADC_CH0_EN | CODEC_ADC_CH1_EN);
        codec->RSPL_CFG &= ~CODEC_RSPL_ADC_EN;

        hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECAD0);
        hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECAD1);
        hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECADC);
				
        hal_cmu_reset_set(HAL_CMU_MOD_O_CODECAD0);
        hal_cmu_reset_set(HAL_CMU_MOD_O_CODECAD1);
        hal_cmu_reset_set(HAL_CMU_MOD_O_CODECADC);

		hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECRS);
		hal_cmu_reset_set(HAL_CMU_MOD_O_CODECRS);		
    }

    return 0;
}

int hal_codec_start_interface(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream, int dma)
{
    if (stream == AUD_STREAM_PLAYBACK) {

        codec->FIFO_FLUSH |= CODEC_FIFO_FLUSH_TX_FIFO_FLUSH;
        hal_codec_reg_update_delay();
        codec->FIFO_FLUSH &= ~CODEC_FIFO_FLUSH_TX_FIFO_FLUSH;

        if (dma) {
            codec->FIFO_THRESHOLD = SET_BITFIELD(codec->FIFO_THRESHOLD, CODEC_FIFO_THRESH_TX, HAL_CODEC_TX_FIFO_TRIGGER_LEVEL);
            codec->CONFIG |= CODEC_CFG_DMACTRL_TX;
            
            // Delay a little time for DMA to fill the TX FIFO before sending
            for (volatile int i = 0; i < 50; i++);
        }

        codec->CONFIG |= CODEC_CFG_DAC_ENABLE;
        
    } else {

        codec->FIFO_FLUSH |= CODEC_FIFO_FLUSH_RX_FIFO_FLUSH_CH1 | CODEC_FIFO_FLUSH_RX_FIFO_FLUSH_CH0;
        hal_codec_reg_update_delay();
        codec->FIFO_FLUSH &= ~(CODEC_FIFO_FLUSH_RX_FIFO_FLUSH_CH1 | CODEC_FIFO_FLUSH_RX_FIFO_FLUSH_CH0);

        if (dma) {
            codec->FIFO_THRESHOLD = SET_BITFIELD(codec->FIFO_THRESHOLD, CODEC_FIFO_THRESH_RX, HAL_CODEC_RX_FIFO_TRIGGER_LEVEL);
            codec->CONFIG |= CODEC_CFG_DMACTRL_RX;
        }

        if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH0) {
            codec->CONFIG |= CODEC_CFG_ADC_ENABLE_CH0;
        }

        if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH1) {
            codec->CONFIG |= CODEC_CFG_ADC_ENABLE_CH1;
        }

        codec->CONFIG |= CODEC_CFG_ADC_ENABLE;

    }

    return 0;
}

int hal_codec_stop_interface(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream)
{
    TRACE("%s: stream[%d]", __func__, stream);

    if (stream == AUD_STREAM_PLAYBACK) {
#ifdef __AUDIO_RESAMPLE__
        // TODO:
#endif
        codec->CONFIG &= ~(CODEC_CFG_DAC_ENABLE | CODEC_CFG_DMACTRL_TX);

        codec->FIFO_FLUSH |= CODEC_FIFO_FLUSH_TX_FIFO_FLUSH;
        hal_codec_reg_update_delay();
        codec->FIFO_FLUSH &= ~CODEC_FIFO_FLUSH_TX_FIFO_FLUSH;

        // Cancel dac sync request
        hal_codec_sync_dac_disable();
        
#if defined(NO_CODEC_RESET) || defined(__AUDIO_RESAMPLE__)
        // Clean up DAC and resample intermediate states
        osDelay(dac_delay_ms);
#endif

    } else {
        codec->CONFIG &= ~(CODEC_CFG_ADC_ENABLE | CODEC_CFG_ADC_ENABLE_CH0 | CODEC_CFG_ADC_ENABLE_CH1 | CODEC_CFG_DMACTRL_RX);

        codec->FIFO_FLUSH |= CODEC_FIFO_FLUSH_RX_FIFO_FLUSH_CH1 | CODEC_FIFO_FLUSH_RX_FIFO_FLUSH_CH0;
        hal_codec_reg_update_delay();
        codec->FIFO_FLUSH &= ~(CODEC_FIFO_FLUSH_RX_FIFO_FLUSH_CH1 | CODEC_FIFO_FLUSH_RX_FIFO_FLUSH_CH0);
        
        // Cancel adc sync request
        hal_codec_sync_adc_disable();
        
#ifdef NO_CODEC_RESET
        // Clean up ADC intermediate states
        osDelay(adc_delay_ms);
#endif
    }

    return 0;
}

static void hal_codec_set_dac_gain_value(enum AUD_CHANNEL_MAP_T map, uint32_t val)
{
    TRACE("hal_codec_set_dac_gain_value: val[%d]", val);
    
    codec->DAC_GAIN_CFG &= ~CODEC_DAC_GAIN_UPDATE;
    hal_codec_reg_update_delay();
    
    if (map & AUD_CHANNEL_MAP_CH0) {
        codec->DAC0_GAIN_CFG = SET_BITFIELD(codec->DAC0_GAIN_CFG, CODEC_DAC_CH0_GAIN, val);
    }
    if (map & AUD_CHANNEL_MAP_CH1) {
        codec->DAC1_GAIN_CFG = SET_BITFIELD(codec->DAC1_GAIN_CFG, CODEC_DAC_CH1_GAIN, val);
    }
    
    codec->DAC_GAIN_CFG |= CODEC_DAC_GAIN_UPDATE;
    //hal_codec_reg_update_delay();
    //codec->DAC_GAIN_CFG &= ~CODEC_DAC_GAIN_UPDATE;
}

void hal_codec_get_dac_gain(float *left_gain,float *right_gain)
{
    uint32_t left_val;
    uint32_t right_val;

    left_val  = GET_BITFIELD(codec->DAC0_GAIN_CFG, CODEC_DAC_CH0_GAIN);
    right_val = GET_BITFIELD(codec->DAC1_GAIN_CFG, CODEC_DAC_CH1_GAIN);

    *left_gain = left_val;
    *right_gain = right_val;

    *left_gain /= (1 << 12);
    *right_gain /= (1 << 12);
}

void hal_codec_dac_mute(bool mute)
{
    codec_mute[AUD_STREAM_PLAYBACK] = mute;

#ifdef AUDIO_OUTPUT_SW_GAIN
    hal_codec_set_sw_gain(digdac_gain[0]);
#else
    if (mute) {
        hal_codec_set_dac_gain_value(VALID_DAC_MAP, 0);
    } else {
        hal_codec_restore_dig_dac_gain();
    }
#endif
}

static float db_to_amplitude_ratio(int32_t db)
{
    float coef;

    if (db == ZERODB_DIG_DAC_DBVAL) {
        coef = 1;
    } else if (db <= MIN_DIG_DAC_DBVAL) {
        coef = 0;
    } else {
        if (db > MAX_DIG_DAC_DBVAL) {
            db = MAX_DIG_DAC_DBVAL;
        }
        coef = db_to_float(db);
    }

    return coef;
}

static float digdac_gain_to_float(int32_t gain)
{
    float coef;

#if defined(NOISE_GATING) && defined(NOISE_REDUCTION)
    gain += digdac_gain_offset_nr;
#endif

    coef = db_to_amplitude_ratio(gain);

#ifdef AUDIO_OUTPUT_DC_CALIB
    coef *= dac_dc_gain_attn;
#endif

    return coef;
}

static void hal_codec_set_dig_dac_gain(enum AUD_CHANNEL_MAP_T map, int32_t gain)
{
    uint32_t val;
    float coef;
    bool mute;

    if (map & AUD_CHANNEL_MAP_CH0) {
        digdac_gain[0] = gain;
    }
    if (map & AUD_CHANNEL_MAP_CH1) {
        digdac_gain[1] = gain;
    }

    mute = codec_mute[AUD_STREAM_PLAYBACK];

    if (mute) {
        val = 0;
    } else {
        coef = digdac_gain_to_float(gain);

        val = (uint32_t)(coef * (1 << 12));
    }

    hal_codec_set_dac_gain_value(map, val);
}

static void hal_codec_restore_dig_dac_gain(void)
{
    if (digdac_gain[0] == digdac_gain[1]) {
        hal_codec_set_dig_dac_gain(VALID_DAC_MAP, digdac_gain[0]);
    } else {
        hal_codec_set_dig_dac_gain(AUD_CHANNEL_MAP_CH0, digdac_gain[0]);
        hal_codec_set_dig_dac_gain(AUD_CHANNEL_MAP_CH1, digdac_gain[1]);
    }
}

#ifdef AUDIO_OUTPUT_SW_GAIN
static void hal_codec_set_sw_gain(int32_t gain)
{
    float coef;
    bool mute;

    digdac_gain[0] = gain;

    mute = codec_mute[AUD_STREAM_PLAYBACK];

    if (mute) {
        coef = 0;
    } else {
        coef = digdac_gain_to_float(gain);
    }

    if (sw_output_coef_callback) {
        sw_output_coef_callback(coef);
    }
}

void hal_codec_set_sw_output_coef_callback(HAL_CODEC_SW_OUTPUT_COEF_CALLBACK callback)
{
    sw_output_coef_callback = callback;
}
#endif

static void hal_codec_set_adc_gain_value(enum AUD_CHANNEL_MAP_T map, uint32_t val)
{
    if (map & AUD_CHANNEL_MAP_CH0) {
        codec->ADC0_CFG |= CODEC_ADC_CH0_GAIN_SEL;
        codec->ADC0_CFG = SET_BITFIELD(codec->ADC0_CFG, CODEC_ADC_CH0_GAIN, val);
    }
    if (map & AUD_CHANNEL_MAP_CH1) {
        codec->ADC1_CFG |= CODEC_ADC_CH1_GAIN_SEL;
        codec->ADC1_CFG = SET_BITFIELD(codec->ADC1_CFG, CODEC_ADC_CH1_GAIN, val);
    }
}

static int hal_codec_set_dac_hbf_bypass_cnt(uint32_t cnt)
{
    uint32_t bypass = 0;
    uint32_t bypass_mask = CODEC_DAC_HBF1_BYPASS | CODEC_DAC_HBF2_BYPASS | CODEC_DAC_HBF3_BYPASS;

    if (cnt == 0) {
    } else if (cnt == 1) {
        bypass = CODEC_DAC_HBF3_BYPASS;
    } else if (cnt == 2) {
        bypass = CODEC_DAC_HBF2_BYPASS | CODEC_DAC_HBF3_BYPASS;
    } else if (cnt == 3) {
        bypass = CODEC_DAC_HBF1_BYPASS | CODEC_DAC_HBF2_BYPASS | CODEC_DAC_HBF3_BYPASS;
    } else {
        ASSERT(false, "%s: Invalid dac bypass cnt: %u", __FUNCTION__, cnt);
    }

    codec->DAC_CFG = (codec->DAC_CFG & ~bypass_mask) | bypass;
    return 0;
}

static int hal_codec_set_dac_osr_sel (uint32_t val)
{
    ASSERT((val < 3), "%s: invalid DAC OSR SEL[%d]", __func__, val);

    codec->DAC_CFG = SET_BITFIELD(codec->DAC_CFG, CODEC_DAC_OSR_SEL, val);
    return 0;
}

/*
static uint32_t hal_codec_get_dac_osr_sel (void)
{
    return GET_BITFIELD(codec->DAC_CFG, CODEC_DAC_OSR_SEL);
}
*/

static int hal_codec_set_dac_up(uint32_t val)
{
    uint32_t sel = 0;

    if (val == 2) {
        sel = 0;
    } else if (val == 3) {
        sel = 1;
    } else if (val == 4) {
        sel = 2;
    } else if (val == 6) {
        sel = 3;
    } else if (val == 1) {
        sel = 4;
    } else {
        ASSERT(false, "%s: Invalid dac up: %u", __FUNCTION__, val);
    }
    codec->DAC_CFG = SET_BITFIELD(codec->DAC_CFG, CODEC_DAC_UP_SEL, sel);
    return 0;
}

static uint32_t POSSIBLY_UNUSED hal_codec_get_dac_up(void)
{
    uint32_t sel;

    sel = GET_BITFIELD(codec->DAC_CFG, CODEC_DAC_UP_SEL);
    if (sel == 0) {
        return 2;
    } else if (sel == 1) {
        return 3;
    } else if (sel == 2) {
        return 4;
    } else if (sel == 3) {
        return 6;
    } else {
        return 1;
    }
}

static int hal_codec_set_adc_down(enum AUD_CHANNEL_MAP_T map, uint32_t val)
{
    uint32_t sel = 0;

    if (val == 3) {
        sel = 0;
    } else if (val == 6) {
        sel = 1;
    } else if (val == 1) {
        sel = 2;
    } else {
        ASSERT(false, "%s: Invalid adc down: %u", __FUNCTION__, val);
    }

    if (map & AUD_CHANNEL_MAP_CH0) {
        codec->ADC0_CFG = SET_BITFIELD(codec->ADC0_CFG, CODEC_ADC_CH0_DOWN_SEL, sel);
    }

    if (map & AUD_CHANNEL_MAP_CH1) {
        codec->ADC1_CFG = SET_BITFIELD(codec->ADC1_CFG, CODEC_ADC_CH1_DOWN_SEL, sel);
    }

    return 0;
}

static int hal_codec_set_adc_hbf_bypass_cnt(enum AUD_CHANNEL_MAP_T map, uint32_t cnt)
{
    //uint32_t bypass = 0;
    uint32_t bypass_ch0 = 0;
    uint32_t bypass_ch1 = 0;

    if (cnt == 0) {
        //bypass_ch0 = 0;
    } else if (cnt == 1) {
        bypass_ch0 = CODEC_ADC_CH0_HBF3_BYPASS;
        bypass_ch1 = CODEC_ADC_CH1_HBF3_BYPASS;
    } else if (cnt == 2) {
        bypass_ch0 = CODEC_ADC_CH0_HBF3_BYPASS | CODEC_ADC_CH0_HBF2_BYPASS;
        bypass_ch1 = CODEC_ADC_CH1_HBF3_BYPASS | CODEC_ADC_CH1_HBF2_BYPASS;
    } else if (cnt == 3) {
        bypass_ch0 = CODEC_ADC_CH0_HBF3_BYPASS | CODEC_ADC_CH0_HBF2_BYPASS | CODEC_ADC_CH0_HBF1_BYPASS;
        bypass_ch1 = CODEC_ADC_CH1_HBF3_BYPASS | CODEC_ADC_CH1_HBF2_BYPASS | CODEC_ADC_CH1_HBF1_BYPASS;
    } else {
        ASSERT(false, "%s: Invalid bypass cnt: %u", __FUNCTION__, cnt);
    }

    if (map & AUD_CHANNEL_MAP_CH0) {
        codec->ADC0_CFG = (codec->ADC0_CFG & (~bypass_ch0)) | bypass_ch0;
    }

    if (map & AUD_CHANNEL_MAP_CH1) {
        codec->ADC1_CFG = (codec->ADC1_CFG & (~bypass_ch1)) | bypass_ch1;
    }

    return 0;
}

/*
static void hal_codec_dac_compensate(enum AUD_SAMPRATE_T sample_rate)
{
    // TODO:
}
*/

/*
static void hal_codec_enable_dig_mic(enum AUD_CHANNEL_MAP_T mic_map)
{
    uint32_t line_map = 0;

    for (int i = 0; i < MAX_DIG_MIC_CH_NUM; i++) {
        if (mic_map & (AUD_CHANNEL_MAP_DIGMIC_CH0 << i)) {
            line_map |= (1 << (i / 2));
        }

        if (i == 0) {
            codec->PDM_CFG &= ~CODEC_PDM_CH0_CAP_PHASE_MASK;
            codec->PDM_CFG |= CODEC_PDM_CH0_CAP_PHASE(CODEC_DIGMIC_PHASE);
        }

        if (i == 1) {
            codec->PDM_CFG &= ~CODEC_PDM_CH1_CAP_PHASE_MASK;
            codec->PDM_CFG |= CODEC_PDM_CH1_CAP_PHASE(CODEC_DIGMIC_PHASE);
        }
    }
    codec->PDM_CFG |= CODEC_PDM_EN;
    
    hal_iomux_set_dig_mic(line_map);

}
*/

/*
static void hal_codec_disable_dig_mic(void)
{
    codec->PDM_CFG &= ~CODEC_PDM_EN;
}
*/

static uint8_t resample_rate_idx[AUD_STREAM_NUM];

int hal_codec_setup_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream, const struct HAL_CODEC_CONFIG_T *cfg)
{
    int i;
    int rate_idx;

    TRACE("codec_setup: stream[%d], set_flag[%x]", stream, cfg->set_flag);

    if (stream == AUD_STREAM_PLAYBACK) {

        // DAC Channel Setup
        if ((HAL_CODEC_CONFIG_CHANNEL_MAP | HAL_CODEC_CONFIG_CHANNEL_NUM) & cfg->set_flag) {
            if (cfg->channel_num == AUD_CHANNEL_NUM_2) {
                if (cfg->channel_map != (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)) {
                    TRACE("\n!!! WARNING:%s: Bad play stereo ch map: 0x%X\n", __func__, cfg->channel_map);
                }
                codec->DAC_CFG &= ~CODEC_DAC_SDM_SWAP;
                codec->DAC_MODE |= CODEC_DAC_DUAL_CHANNEL;
                codec->RSPL_CFG |= CODEC_RSPL_DAC_L_EN | CODEC_RSPL_DAC_R_EN;
                
                codec_dac_ch_map = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1;
            } else {
                ASSERT(cfg->channel_map == AUD_CHANNEL_MAP_CH0 || cfg->channel_map == AUD_CHANNEL_MAP_CH1,
                    "%s: Bad play mono ch map: 0x%X", __func__, cfg->channel_map);
                
                codec->DAC_MODE &= ~CODEC_DAC_DUAL_CHANNEL;
                codec->RSPL_CFG |= CODEC_RSPL_DAC_L_EN;

                codec_dac_ch_map = AUD_CHANNEL_MAP_CH0;
            }
        }

        // DAC BITS Setup
        if (HAL_CODEC_CONFIG_BITS & cfg->set_flag) {
            codec->DAC_MODE &= ~(CODEC_DAC_MODE_USB_24BITS | CODEC_DAC_MODE_32BITS | CODEC_DAC_MODE_24BITS | CODEC_DAC_MODE_16BITS);
            if (cfg->bits == AUD_BITS_16) {
                codec->DAC_MODE |= CODEC_DAC_MODE_16BITS;
            } else if (cfg->bits == AUD_BITS_24) {
                codec->DAC_MODE |= CODEC_DAC_MODE_24BITS;
            } else if (cfg->bits == AUD_BITS_32) {
                codec->DAC_MODE |= CODEC_DAC_MODE_32BITS;
            } else {
                // TODO: to config USB24BITS
                ASSERT(false, "%s: Bad play bits: %u", __func__, cfg->bits);
            }
        }

        // DAC Sample Rate
        if (HAL_CODEC_CONFIG_SAMPLE_RATE & cfg->set_flag) {
            for (i = 0; i < ARRAY_SIZE(codec_dac_sample_rate); i++) {
                if (codec_dac_sample_rate[i].sample_rate == cfg->sample_rate) {
                    break;
                }
            }
            ASSERT(i < ARRAY_SIZE(codec_dac_sample_rate), "%s: Invalid playback sample rate: %u", __func__, cfg->sample_rate);
            rate_idx = i;

            resample_rate_idx[AUD_STREAM_PLAYBACK] = rate_idx;
            TRACE("play rate idx [%d]", rate_idx);

            hal_codec_set_dac_osr_sel(codec_dac_sample_rate[rate_idx].osr_sel);
            hal_codec_set_dac_up(codec_dac_sample_rate[rate_idx].dac_up);
            hal_codec_set_dac_hbf_bypass_cnt(codec_dac_sample_rate[rate_idx].bypass_cnt);

            codec->RSPL_CFG &= ~(CODEC_RSPL_DAC_PHASE_UPD | CODEC_RSPL_DAC_L_EN | CODEC_RSPL_DAC_R_EN);
            hal_codec_reg_update_delay();

            codec->RSPL_DAC_PHASE_INC = codec_dac_sample_rate[rate_idx].resample_phase;

            hal_codec_reg_update_delay();

            codec->RSPL_CFG |= CODEC_RSPL_DAC_PHASE_UPD;            

            if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH0) {
                codec->RSPL_CFG |= CODEC_RSPL_DAC_L_EN;
            }
            if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH1) {
                codec->RSPL_CFG |= CODEC_RSPL_DAC_R_EN;
            }

            codec->RSPL_CFG |= CODEC_RSPL_DAC_EN;

        }

        if (HAL_CODEC_CONFIG_VOL & cfg->set_flag) {
            const struct CODEC_DAC_VOL_T *vol_tab_ptr;

            vol_tab_ptr = hal_codec_get_dac_volume(cfg->vol);
            if (vol_tab_ptr) {
#ifdef AUDIO_OUTPUT_SW_GAIN
                hal_codec_set_sw_gain(vol_tab_ptr->sdac_volume);
#else
                analog_aud_set_dac_gain(vol_tab_ptr->tx_pa_gain);
                hal_codec_set_dig_dac_gain(VALID_DAC_MAP, vol_tab_ptr->sdac_volume);
#endif

#ifdef PERF_TEST_POWER_KEY
                // Update performance test power after applying new dac volume
                hal_codec_update_perf_test_power();
#endif
            }
        }
        
    } else {

        if ((HAL_CODEC_CONFIG_CHANNEL_MAP | HAL_CODEC_CONFIG_CHANNEL_NUM) & cfg->set_flag) {

#if 0
            enum AUD_CHANNEL_MAP_T mic_map = cfg->channel_map;
            TRACE("-> MIC CH MAP[%x]", mic_map);
    
            if ((mic_map & (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)) == (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)) {
                // dual channel
                codec->RSPL_CFG |= CODEC_RSPL_ADC_DUAL_CH;

                codec->ADC0_CFG &= ~CODEC_ADC_CH0_IN_SEL;
                codec->ADC1_CFG &= ~CODEC_ADC_CH1_IN_SEL;

                codec_adc_ch_map = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1;
            } else {
                // For single-mic application, only CH0 can be chosen for the resampler can only work on CH0
                codec->ADC_CFG &= ~(CODEC_ADC_CH1_EN | CODEC_ADC_EN | CODEC_ADC_GAIN_UPD_CH1);
                codec->ADC_CFG |= CODEC_ADC_CH0_EN | CODEC_ADC_GAIN_UPD_CH0;

                if (mic_map & AUD_CHANNEL_MAP_CH1) {
                    codec->ADC0_CFG |= CODEC_ADC_CH0_IN_SEL;            // CH0 From Analog ADC1
                } 
                codec->RSPL_CFG &= ~CODEC_RSPL_ADC_DUAL_CH;

                //codec->CONFIG |= CODEC_CFG_ADC_ENABLE_CH0;
                //codec->CONFIG &= ~(CODEC_CFG_ADC_ENABLE | CODEC_CFG_ADC_ENABLE_CH1);
                codec_adc_ch_map = AUD_CHANNEL_MAP_CH0;
            }
#endif
            TRACE("-> MIC CH MAP[%x], CH NUM[%d]", cfg->channel_map, cfg->channel_num);
            switch (cfg->channel_num) {
                case AUD_CHANNEL_NUM_1: {
                        // For single-channel application, only CH0 can be chosen for the resampler can only work on CH0
                        codec->ADC_CFG &= ~(CODEC_ADC_CH1_EN | CODEC_ADC_EN | CODEC_ADC_GAIN_UPD_CH1);
                        codec->ADC_CFG |= CODEC_ADC_CH0_EN | CODEC_ADC_GAIN_UPD_CH0;
                        
                        codec->RSPL_CFG &= ~CODEC_RSPL_ADC_DUAL_CH;
                        if (cfg->channel_map & AUD_CHANNEL_MAP_CH1) {
                            codec->ADC0_CFG |= CODEC_ADC_CH0_IN_SEL;    // CH0 from ADC1
                        }
                        
                        codec_adc_ch_map = AUD_CHANNEL_MAP_CH0;
                    }
                    break;

                case AUD_CHANNEL_NUM_2: {
                        codec->ADC_CFG &= ~CODEC_ADC_EN;
                        codec->ADC_CFG |= CODEC_ADC_CH0_EN | CODEC_ADC_CH1_EN | CODEC_ADC_GAIN_UPD_CH0 | CODEC_ADC_GAIN_UPD_CH1;

                        codec->RSPL_CFG |= CODEC_RSPL_ADC_DUAL_CH;
                        if (cfg->channel_map == (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)) {
                            codec->ADC0_CFG &= ~CODEC_ADC_CH0_IN_SEL; // CH0 from ADC0
                            codec->ADC1_CFG &= ~CODEC_ADC_CH1_IN_SEL; // CH1 from ADC1
                        } else if (cfg->channel_map == AUD_CHANNEL_MAP_CH0) {
                            codec->ADC1_CFG |= CODEC_ADC_CH1_IN_SEL; // CH1 from ADC0
                        } else if (cfg->channel_map == AUD_CHANNEL_MAP_CH1) {
                            codec->ADC0_CFG |= CODEC_ADC_CH0_IN_SEL; // CH0 from ADC1
                        } else {
                            ASSERT(0, "%s: invalid capture channel map[%x]", __func__, cfg->channel_map);
                        }

                        codec_adc_ch_map = (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1);
                    }
                    break;

                default:
                    ASSERT(0, "%s: invalid capture chan num[%d]", __func__, cfg->channel_num);
                    break;
            }

            // TODO: DIGMIC setting
        }

        if (HAL_CODEC_CONFIG_BITS & cfg->set_flag) {
            
            TRACE("-> MIC BITS[%x]", cfg->bits);

            if (cfg->bits == AUD_BITS_16) {
                codec->ADC_MODE = (CODEC_ADC_MODE_16BITS_CH0 | CODEC_ADC_MODE_16BITS_CH1);
            } else if (cfg->bits == AUD_BITS_24) {
                codec->ADC_MODE = CODEC_ADC_MODE_24BITS;
            } else if (cfg->bits == AUD_BITS_32) {
                codec->ADC_MODE = CODEC_ADC_MODE_32BITS;
            }else {
                ASSERT(false, "%s: Bad cap bits: %u", __func__, cfg->bits);
            }
        }

        if (HAL_CODEC_CONFIG_SAMPLE_RATE & cfg->set_flag) {
            for(i = 0; i < ARRAY_SIZE(codec_adc_sample_rate); i++) {
                if(codec_adc_sample_rate[i].sample_rate == cfg->sample_rate) {
                    break;
                }
            }
            ASSERT(i < ARRAY_SIZE(codec_adc_sample_rate), "%s: Invalid capture sample rate: %d", __func__, cfg->sample_rate);
            rate_idx = i;
            resample_rate_idx[AUD_STREAM_CAPTURE] = rate_idx;
            TRACE("-> ADC SAMPLE RATE [%d -> %d]", i, codec_adc_sample_rate[i].sample_rate);           

            hal_codec_set_adc_down(codec_adc_ch_map, codec_adc_sample_rate[rate_idx].adc_down);
            hal_codec_set_adc_hbf_bypass_cnt(codec_adc_ch_map, codec_adc_sample_rate[rate_idx].bypass_cnt);

            codec->RSPL_CFG &= ~CODEC_RSPL_ADC_PHASE_UPD;
            hal_codec_reg_update_delay();

            //codec->RSPL_ADC_PHASE_INC = resample_phase_double_to_value(get_capture_resample_phase());
            codec->RSPL_ADC_PHASE_INC = codec_adc_sample_rate[rate_idx].resample_phase;
            
            hal_codec_reg_update_delay();

            codec->RSPL_CFG |= CODEC_RSPL_ADC_PHASE_UPD;

        }

#ifndef FIXED_CODEC_ADC_VOL
        if (HAL_CODEC_CONFIG_VOL & cfg->set_flag) {
            const CODEC_ADC_VOL_T *adc_gain_db;
            adc_gain_db = hal_codec_get_adc_volume(cfg->vol);
            if (adc_gain_db) {
                hal_codec_set_dig_adc_gain(NORMAL_ADC_MAP, *adc_gain_db);
            }
        }
#endif
    }
    
    return 0;
}

static void hal_codec_set_dig_adc_gain(enum AUD_CHANNEL_MAP_T map, int32_t gain)
{
    uint32_t val;
    float coef;
    bool mute;
    int i;

    for (i = 0; i < NORMAL_ADC_CH_NUM; i++) {
        if (map & (1 << i)) {
            digadc_gain[i] = gain;
        }
    }

    mute = codec_mute[AUD_STREAM_CAPTURE];

    if (mute) {
        val = 0;
    } else {
        coef = db_to_amplitude_ratio(gain);

        // Gain format: 8.12
        val = (uint32_t)(coef * (1 << 12));
        val = __USAT(val, 20);
    }

    hal_codec_set_adc_gain_value(map, val);
}

static void hal_codec_restore_dig_adc_gain(void)
{
    int i;

    for (i = 0; i < NORMAL_ADC_CH_NUM; i++) {
        hal_codec_set_dig_adc_gain((1 << i), digadc_gain[i]);
    }
}

void hal_codec_adc_mute(bool mute)
{
    codec_mute[AUD_STREAM_CAPTURE] = mute;

    if (mute) {
        hal_codec_set_adc_gain_value(NORMAL_ADC_MAP, 0);
    } else {
        hal_codec_restore_dig_adc_gain();
    }
}

int hal_codec_anc_adc_enable(enum ANC_TYPE_T type)
{
    return 0;
}

int hal_codec_anc_adc_disable(enum ANC_TYPE_T type)
{
    return 0;
}

enum AUD_SAMPRATE_T hal_codec_anc_convert_rate(enum AUD_SAMPRATE_T rate)
{
    return AUD_SAMPRATE_NULL;

}

int hal_codec_anc_dma_enable(enum HAL_CODEC_ID_T id)
{
    return 0;
}

int hal_codec_anc_dma_disable(enum HAL_CODEC_ID_T id)
{
    return 0;
}

int hal_codec_aux_mic_dma_enable(enum HAL_CODEC_ID_T id)
{
    return 0;
}

int hal_codec_aux_mic_dma_disable(enum HAL_CODEC_ID_T id)
{
    return 0;
}

uint32_t hal_codec_get_alg_dac_shift(void)
{
    return 0;
}

#ifdef AUDIO_OUTPUT_DC_CALIB
void hal_codec_set_dac_dc_gain_attn(float attn)
{
    dac_dc_gain_attn = attn;
}

void hal_codec_set_dac_dc_offset(int16_t dc_l, int16_t dc_r)
{
    // DC calib values are based on 16-bit, but hardware compensation is based on 20-bit
    dac_dc_l = dc_l;
    dac_dc_r = dc_r;
#ifdef DAC_DC_SW_EXTRA_OFFSET_L
	dac_dc_l += DAC_DC_SW_EXTRA_OFFSET_L;
#endif
#ifdef DAC_DC_SW_EXTRA_OFFSET_R
	dac_dc_r += DAC_DC_SW_EXTRA_OFFSET_R;
#endif
}
#endif

void hal_codec_set_dac_reset_callback(HAL_CODEC_DAC_RESET_CALLBACK callback)
{
    //dac_reset_callback = callback;
}

void hal_codec_sync_dac_enable(enum HAL_CODEC_SYNC_TYPE_T type)
{
    codec->SYNC_CFG = SET_BITFIELD(codec->SYNC_CFG, CODEC_SYNC_DAC_ENABLE_SEL, type);
}

void hal_codec_sync_dac_disable(void)
{
    codec->SYNC_CFG = SET_BITFIELD(codec->SYNC_CFG, CODEC_SYNC_DAC_ENABLE_SEL, HAL_CODEC_SYNC_TYPE_NONE);
}

void hal_codec_sync_adc_enable(enum HAL_CODEC_SYNC_TYPE_T type)
{
    codec->SYNC_CFG = SET_BITFIELD(codec->SYNC_CFG, CODEC_SYNC_ADC_ENABLE_SEL, type);
}

void hal_codec_sync_adc_disable(void)
{
    codec->SYNC_CFG = SET_BITFIELD(codec->SYNC_CFG, CODEC_SYNC_ADC_ENABLE_SEL, HAL_CODEC_SYNC_TYPE_NONE);
}

int hal_codec_dac_reset_set(void)
{
    int ret;

    TRACE("%s: codec_opened[%d]", __func__, codec_opened);

    if (codec_opened) {
        ret = !!(codec->SOFT_RSTN & CODEC_SOFT_RSTN_DAC);
        codec->SOFT_RSTN &= ~CODEC_SOFT_RSTN_DAC;
    } else {
        ret = 0;
    }

    return ret;
}

int hal_codec_dac_reset_clear(void)
{
    int ret;

    TRACE("%s: codec_opened[%d]", __func__, codec_opened);

    if (codec_opened) {
        ret = !!(codec->SOFT_RSTN & CODEC_SOFT_RSTN_DAC);
        codec->SOFT_RSTN |= CODEC_SOFT_RSTN_DAC;
    } else {
        ret = 0;
    }

    return ret;
}

void hal_codec_tune_resample_rate(enum AUD_STREAM_T stream, float ratio)
{
/*
    uint32_t val;
    
    if (stream >= AUD_STREAM_NUM) {
        return;
    }

    if (!codec_opened) {
        return;
    }

    TRACE("%s: stream[%d] factor[%f]", __func__, stream, factor);

    if (stream == AUD_STREAM_PLAYBACK) {
        codec->RSPL_CFG &= ~CODEC_RSPL_DAC_PHASE_UPD;

        hal_codec_reg_update_delay();
        val = codec_dac_sample_rate[rate_idx].resample_phase;
        val += (int)(val * ratio);
        codec->RSPL_DAC_PHASE_INC = val;
        hal_codec_reg_update_delay();

        codec->RSPL_CFG |= CODEC_RSPL_DAC_PHASE_UPD;
    } else {
        codec->RSPL_CFG &= ~CODEC_RSPL_ADC_PHASE_UPD;

        hal_codec_reg_update_delay();       
        val = codec_adc_sample_rate[rate_idx].resample_phase;
        val -= (int)(val * ratio);
        codec->RSPL_ADC_PHASE_INC = val;
        hal_codec_reg_update_delay();

        codec->RSPL_CFG |= CODEC_RSPL_ADC_PHASE_UPD;
    }
*/
}

void hal_codec_tune_resample_ppm (int32_t ppm)
{
#define TUNE_CNT_PER_PPM		110

	int32_t tune_cnt = ppm * TUNE_CNT_PER_PPM;

	// "ppm" less than 0, meaning DAC is slow, and the CODEC resample coef needs to get larger;
	
	if (codec->RSPL_CFG & CODEC_RSPL_DAC_EN) {
		codec->RSPL_DAC_PHASE_INC += tune_cnt;
	}

#if 0
	if (codec->RSPL_CFG & CODEC_RSPL_ADC_EN) {
		codec->RSPL_ADC_PHASE_INC -= tune_cnt;
	}
#endif
}

void hal_codec_sidetone_set (int8_t gain_db)
{
#define MAX_SIDETONE_GAIN		36		// in dB
#define MIN_SIDETONE_GAIN		-24		// in dB

	if (gain_db < MIN_SIDETONE_GAIN) {
		gain_db = 0x1f;
	} else {

		if (gain_db > MAX_SIDETONE_GAIN) {
			gain_db = MAX_SIDETONE_GAIN;
		} else 

		gain_db -= MAX_SIDETONE_GAIN;
		gain_db /= 2;
		gain_db = -gain_db;
	}
	
	codec->ADC_CFG &= ~CODEC_SIDETONE_GAIN_MASK;
	codec->ADC_CFG |= CODEC_SIDETONE_GAIN(gain_db);
	
}

void hal_codec_swap_output(bool swap)
{
}

