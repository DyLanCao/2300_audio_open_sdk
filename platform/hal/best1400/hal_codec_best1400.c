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
#include "reg_codec_best1400.h"
#include "hal_codec.h"
#include "hal_cmu.h"
#include "hal_psc.h"
#include "hal_aud.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "analog.h"
#include "cmsis.h"
#include "string.h"
#include "tgt_hardware.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_chipid.h"

#define NO_DAC_RESET

#define SDM_MUTE_NOISE_SUPPRESSION

#if defined(SPEECH_SIDETONE) && defined(CFG_HW_AUD_SIDETONE_MIC_DEV) && defined(CFG_HW_AUD_SIDETONE_GAIN_DBVAL)
#define SIDETONE_ENABLE
#endif

#define RS_CLOCK_FACTOR                     200

#define HAL_CODEC_TX_FIFO_TRIGGER_LEVEL     (3)
#define HAL_CODEC_RX_FIFO_TRIGGER_LEVEL     (4)

#define MAX_DIG_DBVAL                       (50)
#define ZERODB_DIG_DBVAL                    (0)
#define MIN_DIG_DBVAL                       (-99)

#define MAX_SIDETONE_DBVAL                  (30)
#define MIN_SIDETONE_DBVAL                  (-32)
#define SIDETONE_DBVAL_STEP                 (-2)

#define MAX_SIDETONE_REGVAL                 (0)
#define MIN_SIDETONE_REGVAL                 (31)

#ifndef CODEC_DIGMIC_PHASE
#define CODEC_DIGMIC_PHASE                  7
#endif

#define MAX_DIG_MIC_CH_NUM                  1

#define NORMAL_ADC_CH_NUM                   1
// Echo cancel ADC channel number
#define EC_ADC_CH_NUM                       1
#define MAX_ADC_CH_NUM                      (NORMAL_ADC_CH_NUM + EC_ADC_CH_NUM)

#define MAX_DAC_CH_NUM                      2

#define NORMAL_MIC_MAP                      (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_DIGMIC_CH0)
#define NORMAL_ADC_MAP                      (AUD_CHANNEL_MAP_CH0)

#define EC_MIC_MAP                          (AUD_CHANNEL_MAP_CH1)
#define EC_ADC_MAP                          EC_MIC_MAP

#define VALID_MIC_MAP                       (NORMAL_MIC_MAP | EC_MIC_MAP)
#define VALID_ADC_MAP                       (NORMAL_ADC_MAP | EC_ADC_MAP)

#define VALID_SPK_MAP                       (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)
#define VALID_DAC_MAP                       VALID_SPK_MAP

#if (CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV & ~VALID_SPK_MAP)
#error "Invalid CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV"
#endif

#define RSTN_ADC_FREE_RUNNING_CLK           CODEC_SOFT_RSTN_ADC(1 << NORMAL_ADC_CH_NUM)

struct CODEC_DAC_DRE_CFG_T {
    uint8_t dre_delay;
    uint8_t thd_db_offset;
    uint8_t step_mode;
    uint32_t dre_win;
    uint16_t amp_high;
};

struct CODEC_DAC_SAMPLE_RATE_T {
    enum AUD_SAMPRATE_T sample_rate;
    uint32_t codec_freq;
    uint8_t codec_div;
    uint8_t cmu_div;
    uint8_t dac_up;
    uint8_t bypass_cnt;
};

static const struct CODEC_DAC_SAMPLE_RATE_T codec_dac_sample_rate[] = {
#ifdef __AUDIO_RESAMPLE__
    {AUD_SAMPRATE_8463,      CODEC_FREQ_CRYSTAL,               1,             1, 6, 0},
    {AUD_SAMPRATE_16927,     CODEC_FREQ_CRYSTAL,               1,             1, 3, 0},
    {AUD_SAMPRATE_50781,     CODEC_FREQ_CRYSTAL,               1,             1, 1, 0},
#endif
    {AUD_SAMPRATE_7350,   CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 6, 0},
    {AUD_SAMPRATE_8000,   CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 6, 0},
    {AUD_SAMPRATE_14700,  CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 3, 0},
    {AUD_SAMPRATE_16000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 3, 0},
    {AUD_SAMPRATE_22050,  CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 2, 0},
    {AUD_SAMPRATE_24000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 2, 0},
    {AUD_SAMPRATE_44100,  CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 0},
    {AUD_SAMPRATE_48000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 0},
    {AUD_SAMPRATE_88200,  CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 1},
    {AUD_SAMPRATE_96000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 1},
    {AUD_SAMPRATE_176400, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 2},
    {AUD_SAMPRATE_192000, CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 2},
    {AUD_SAMPRATE_352800, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 3},
    {AUD_SAMPRATE_384000, CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 3},
};

struct CODEC_ADC_SAMPLE_RATE_T {
    enum AUD_SAMPRATE_T sample_rate;
    uint32_t codec_freq;
    uint8_t codec_div;
    uint8_t cmu_div;
    uint8_t adc_down;
    uint8_t bypass_cnt;
};

static const struct CODEC_ADC_SAMPLE_RATE_T codec_adc_sample_rate[] = {
#ifdef __AUDIO_RESAMPLE__
    {AUD_SAMPRATE_8463,      CODEC_FREQ_CRYSTAL,               1,             1, 6, 0},
    {AUD_SAMPRATE_16927,     CODEC_FREQ_CRYSTAL,               1,             1, 3, 0},
    {AUD_SAMPRATE_50781,     CODEC_FREQ_CRYSTAL,               1,             1, 1, 0},
#endif
    {AUD_SAMPRATE_7350,   CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 6, 0},
    {AUD_SAMPRATE_8000,   CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 6, 0},
    {AUD_SAMPRATE_14700,  CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 3, 0},
    {AUD_SAMPRATE_16000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 3, 0},
    {AUD_SAMPRATE_44100,  CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 0},
    {AUD_SAMPRATE_48000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 0},
    {AUD_SAMPRATE_88200,  CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 1},
    {AUD_SAMPRATE_96000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 1},
    {AUD_SAMPRATE_176400, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 2},
    {AUD_SAMPRATE_192000, CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 2},
    {AUD_SAMPRATE_352800, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 3},
    {AUD_SAMPRATE_384000, CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 3},
};

const CODEC_ADC_VOL_T WEAK codec_adc_vol[TGT_ADC_VOL_LEVEL_QTY] = {
    -99, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28,
};

static struct CODEC_T * const codec = (struct CODEC_T *)CODEC_BASE;

static bool codec_init = false;
static bool codec_opened = false;

static int8_t digdac_gain[MAX_DAC_CH_NUM];
static int8_t digadc_gain[NORMAL_ADC_CH_NUM];

static bool codec_mute[AUD_STREAM_NUM];

#ifdef AUDIO_OUTPUT_SWAP
static bool output_swap;
#endif

enum HAL_CODEC_OPEN_T {
    HAL_CODEC_OPEN_NORMAL       = (1 << 0),
};

#if defined(NOISE_GATING) && defined(NOISE_REDUCTION)
static bool codec_nr_enabled;
static int8_t digdac_gain_offset_nr;
#endif
#ifdef AUDIO_OUTPUT_DC_CALIB
static int32_t dac_dc_l;
static int32_t dac_dc_r;
static float dac_dc_gain_attn;
#endif
#ifdef __AUDIO_RESAMPLE__
static uint8_t rs_clk_map;
STATIC_ASSERT(sizeof(rs_clk_map) * 8 >= AUD_STREAM_NUM, "rs_clk_map size too small");

static uint32_t resample_clk_freq;
static uint8_t resample_rate_idx[AUD_STREAM_NUM];
#endif
// EC
static uint8_t codec_rate_idx[AUD_STREAM_NUM];

//static HAL_CODEC_DAC_RESET_CALLBACK dac_reset_callback;

static enum AUD_CHANNEL_MAP_T codec_dac_ch_map;
static enum AUD_CHANNEL_MAP_T codec_adc_ch_map;
static enum AUD_CHANNEL_MAP_T codec_mic_ch_map;

static uint8_t dac_delay_ms;

static const uint8_t codec_digmic_phase = CODEC_DIGMIC_PHASE;

#ifdef PERF_TEST_POWER_KEY
static enum HAL_CODEC_PERF_TEST_POWER_T cur_perft_power;
#endif

#ifdef AUDIO_OUTPUT_SW_GAIN
static HAL_CODEC_SW_OUTPUT_COEF_CALLBACK sw_output_coef_callback;
#endif

#if defined(DAC_CLASSG_ENABLE)
static struct dac_classg_cfg _dac_classg_cfg = {
    .thd2 = 0xC0,
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
static int hal_codec_set_adc_down(enum AUD_CHANNEL_MAP_T map, uint32_t val);
static int hal_codec_set_adc_hbf_bypass_cnt(enum AUD_CHANNEL_MAP_T map, uint32_t cnt);
static uint32_t hal_codec_get_adc_chan(enum AUD_CHANNEL_MAP_T mic_map);
#ifdef AUDIO_OUTPUT_SW_GAIN
static void hal_codec_set_sw_gain(int32_t gain);
#endif

static void hal_codec_reg_update_delay(void)
{
    hal_sys_timer_delay_us(2);
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

        codec->REG_0E0 = SET_BITFIELD(codec->REG_0E0, CODEC_CODEC_CLASSG_THD2, config->thd2);
        codec->REG_0E0 = SET_BITFIELD(codec->REG_0E0, CODEC_CODEC_CLASSG_THD1, config->thd1);
        codec->REG_0E0 = SET_BITFIELD(codec->REG_0E0, CODEC_CODEC_CLASSG_THD0, config->thd0);

        // Make class-g set the lowest gain after several samples.
        // Class-g gain will have impact on dc.
        codec->REG_0DC = SET_BITFIELD(codec->REG_0DC, CODEC_CODEC_CLASSG_WINDOW, 6);

        if (config->lr)
            codec->REG_0DC |= CODEC_CODEC_CLASSG_LR;
        else
            codec->REG_0DC &= ~CODEC_CODEC_CLASSG_LR;

        if (config->step_4_3n)
            codec->REG_0DC |= CODEC_CODEC_CLASSG_STEP_3_4N;
        else
            codec->REG_0DC &= ~CODEC_CODEC_CLASSG_STEP_3_4N;

        if (config->quick_down)
            codec->REG_0DC |= CODEC_CODEC_CLASSG_QUICK_DOWN;
        else
            codec->REG_0DC &= ~CODEC_CODEC_CLASSG_QUICK_DOWN;

        codec->REG_0DC |= CODEC_CODEC_CLASSG_EN;

        // Restore class-g window after the gain has been updated
        hal_codec_reg_update_delay();
        codec->REG_0DC = SET_BITFIELD(codec->REG_0DC, CODEC_CODEC_CLASSG_WINDOW, config->wind_width);
    } else {
        codec->REG_0DC &= ~CODEC_CODEC_CLASSG_QUICK_DOWN;
    }
}
#endif

#if defined(AUDIO_OUTPUT_DC_CALIB) || defined(SDM_MUTE_NOISE_SUPPRESSION)
static void hal_codec_dac_dc_offset_enable(int32_t dc_l, int32_t dc_r)
{
    codec->REG_094 &= (CODEC_CODEC_DAC_DC_UPDATE_CH0 | CODEC_CODEC_DAC_DC_UPDATE_CH1);
    hal_codec_reg_update_delay();
    codec->REG_09C = SET_BITFIELD(codec->REG_09C, CODEC_CODEC_DAC_DC_CH0, dc_r);
    codec->REG_098 = SET_BITFIELD(codec->REG_098, CODEC_CODEC_DAC_DC_CH0, dc_l);
    codec->REG_094 |= (CODEC_CODEC_DAC_DC_UPDATE_CH0 | CODEC_CODEC_DAC_DC_UPDATE_CH1);
}
#endif

static int hal_codec_int_open(enum HAL_CODEC_OPEN_T type)
{
    bool first_open = !codec_init;

    analog_aud_pll_open(ANA_AUD_PLL_USER_CODEC);

    if (!codec_init) {
        int i;

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

    codec->REG_03C |= CODEC_EN_CLK_DAC | CODEC_EN_CLK_ADC_MASK | CODEC_EN_CLK_ADC_ANA | CODEC_POL_ADC_ANA;
    codec->REG_044 |= CODEC_SOFT_RSTN_IIR | CODEC_SOFT_RSTN_RS |
        CODEC_SOFT_RSTN_DAC | CODEC_SOFT_RSTN_ADC_MASK | CODEC_SOFT_RSTN_ADC_ANA;
    codec->REG_000 &= ~(CODEC_DMACTRL_TX | CODEC_DMACTRL_RX | CODEC_DAC_ENABLE |
        CODEC_ADC_ENABLE_CH0 | CODEC_ADC_ENABLE_CH1 | CODEC_ADC_ENABLE | CODEC_CODEC_IF_EN);
    codec->REG_004 |= CODEC_RX_FIFO_FLUSH_CH0 | CODEC_RX_FIFO_FLUSH_CH1 | CODEC_TX_FIFO_FLUSH;
    hal_codec_reg_update_delay();
    codec->REG_004 &= ~(CODEC_RX_FIFO_FLUSH_CH0 | CODEC_RX_FIFO_FLUSH_CH1 | CODEC_TX_FIFO_FLUSH);
    codec->REG_000 |= CODEC_CODEC_IF_EN;
    codec->REG_038 |= CODEC_FAULT_MUTE_DAC_ENABLE | CODEC_STAMP_CLR_USED;

#ifdef AUDIO_OUTPUT_SWAP
    if (output_swap) {
        codec->REG_090 |= CODEC_CODEC_DAC_OUT_SWAP;
    } else {
        codec->REG_090 &= ~CODEC_CODEC_DAC_OUT_SWAP;
    }
#endif

    if (first_open) {
        // Enable ADC zero-crossing gain adjustment
        codec->REG_084 |= CODEC_CODEC_ADC_GAIN_SEL_CH0;

        // DRE ini gain and offset
        uint8_t max_gain, ini_gain, dre_offset;
        max_gain = analog_aud_get_max_dre_gain();
        if (max_gain < 0xF) {
            ini_gain = 0xF - max_gain;
        } else {
            ini_gain = 0;
        }
        if (max_gain > 0xF) {
            dre_offset = max_gain - 0xF;
        } else {
            dre_offset = 0;
        }
        codec->REG_0BC = CODEC_CODEC_DRE_INI_ANA_GAIN_CH0(ini_gain) | CODEC_CODEC_DRE_GAIN_OFFSET_CH0(dre_offset);
        codec->REG_0C4 = CODEC_CODEC_DRE_INI_ANA_GAIN_CH1(ini_gain) | CODEC_CODEC_DRE_GAIN_OFFSET_CH1(dre_offset);
        codec->REG_094 = SET_BITFIELD(codec->REG_094, CODEC_CODEC_DAC_ANA_GAIN_UPDATE_DELAY_CH0, 0);
        codec->REG_094 = SET_BITFIELD(codec->REG_094, CODEC_CODEC_DAC_ANA_GAIN_UPDATE_DELAY_CH1, 0);

#if defined(FIXED_CODEC_ADC_VOL) && defined(SINGLE_CODEC_ADC_VOL)
        const CODEC_ADC_VOL_T *adc_gain_db;

        adc_gain_db = hal_codec_get_adc_volume(CODEC_SADC_VOL);
        if (adc_gain_db) {
            hal_codec_set_dig_adc_gain(NORMAL_ADC_MAP, *adc_gain_db);
        }
#endif

#ifdef AUDIO_OUTPUT_DC_CALIB
        hal_codec_dac_dc_offset_enable(dac_dc_l, dac_dc_r);
#elif defined(SDM_MUTE_NOISE_SUPPRESSION)
    hal_codec_dac_dc_offset_enable(1, 1);
#endif

#ifdef AUDIO_OUTPUT_SW_GAIN
        const struct CODEC_DAC_VOL_T *vol_tab_ptr;

        // Init gain settings
        vol_tab_ptr = hal_codec_get_dac_volume(0);
        if (vol_tab_ptr) {
            analog_aud_set_dac_gain(vol_tab_ptr->tx_pa_gain);
            hal_codec_set_dig_dac_gain(VALID_DAC_MAP, ZERODB_DIG_DBVAL);
        }
#else
        // Enable DAC zero-crossing gain adjustment
        codec->REG_08C |= CODEC_CODEC_DAC_GAIN_SEL_CH0;
        codec->REG_090 |= CODEC_CODEC_DAC_GAIN_SEL_CH1;
#endif
        hal_codec_set_dac_gain_value(VALID_DAC_MAP, 0);

#ifdef AUDIO_OUTPUT_DC_CALIB_ANA
        // Reset SDM
        codec->REG_088 |= CODEC_CODEC_DAC_SDM_CLOSE;
#endif

#ifdef __AUDIO_RESAMPLE__
        codec->REG_0D0 &= ~(CODEC_CODEC_RESAMPLE_DAC_ENABLE | CODEC_CODEC_RESAMPLE_ADC_ENABLE |
            CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE | CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE);
#endif
    }

    return 0;
}

int hal_codec_open(enum HAL_CODEC_ID_T id)
{
    return hal_codec_int_open(HAL_CODEC_OPEN_NORMAL);
}

int hal_codec_int_close(enum HAL_CODEC_OPEN_T type)
{
    codec->REG_000 &= ~CODEC_CODEC_IF_EN;
    codec->REG_044 &= ~(CODEC_SOFT_RSTN_IIR | CODEC_SOFT_RSTN_RS |
        CODEC_SOFT_RSTN_DAC | CODEC_SOFT_RSTN_ADC_MASK | CODEC_SOFT_RSTN_ADC_ANA);
    codec->REG_03C &= ~(CODEC_EN_CLK_DAC | CODEC_EN_CLK_ADC_MASK | CODEC_EN_CLK_ADC_ANA | CODEC_POL_ADC_ANA);

    codec_opened = false;

    // NEVER reset or power down CODEC registers, for the CODEC driver expects that last configurations
    // still exist in the next stream setup
    //hal_cmu_codec_reset_set();
    hal_cmu_codec_clock_disable();
    //hal_psc_codec_disable();

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
    codec->REG_0BC = (codec->REG_0BC & ~(CODEC_CODEC_DRE_THD_DB_OFFSET_SIGN_CH0 | CODEC_CODEC_DRE_DELAY_CH0_MASK |
            CODEC_CODEC_DRE_INI_ANA_GAIN_CH0_MASK | CODEC_CODEC_DRE_THD_DB_OFFSET_CH0_MASK | CODEC_CODEC_DRE_STEP_MODE_CH0_MASK)) |
        CODEC_CODEC_DRE_DELAY_CH0(dac_dre_cfg.dre_delay) |
        CODEC_CODEC_DRE_INI_ANA_GAIN_CH0(0xF) | CODEC_CODEC_DRE_THD_DB_OFFSET_CH0(dac_dre_cfg.thd_db_offset) |
        CODEC_CODEC_DRE_THD_DB_OFFSET_CH0(dac_dre_cfg.step_mode) | CODEC_CODEC_DRE_ENABLE_CH0;
    codec->REG_0C0 = (codec->REG_0C4 & ~(CODEC_CODEC_DRE_WINDOW_CH0_MASK | CODEC_CODEC_DRE_AMP_HIGH_CH0_MASK)) |
        CODEC_CODEC_DRE_WINDOW_CH0(dac_dre_cfg.dre_win) | CODEC_CODEC_DRE_AMP_HIGH_CH0(dac_dre_cfg.amp_high);

    codec->REG_0C4 = (codec->REG_0C4 & ~(CODEC_CODEC_DRE_THD_DB_OFFSET_SIGN_CH1 | CODEC_CODEC_DRE_DELAY_CH1_MASK |
            CODEC_CODEC_DRE_INI_ANA_GAIN_CH1_MASK | CODEC_CODEC_DRE_THD_DB_OFFSET_CH1_MASK | CODEC_CODEC_DRE_STEP_MODE_CH1_MASK)) |
        CODEC_CODEC_DRE_DELAY_CH1(dac_dre_cfg.dre_delay) |
        CODEC_CODEC_DRE_INI_ANA_GAIN_CH1(0xF) | CODEC_CODEC_DRE_THD_DB_OFFSET_CH1(dac_dre_cfg.thd_db_offset) |
        CODEC_CODEC_DRE_THD_DB_OFFSET_CH1(dac_dre_cfg.step_mode) | CODEC_CODEC_DRE_ENABLE_CH1;
    codec->REG_0C8 = (codec->REG_0CC & ~(CODEC_CODEC_DRE_WINDOW_CH1_MASK | CODEC_CODEC_DRE_AMP_HIGH_CH1_MASK)) |
        CODEC_CODEC_DRE_WINDOW_CH1(dac_dre_cfg.dre_win) | CODEC_CODEC_DRE_AMP_HIGH_CH1(dac_dre_cfg.amp_high);
}

static void hal_codec_dac_dre_disable(void)
{
    codec->REG_0BC &= ~CODEC_CODEC_DRE_ENABLE_CH0;
    codec->REG_0C4 &= ~CODEC_CODEC_DRE_ENABLE_CH1;
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
        ini_ana_gain = 6;
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

    if (codec->REG_0BC & CODEC_CODEC_DRE_ENABLE_CH0) {
        dac_vol = nominal_vol;
    } else {
        codec->REG_0BC = SET_BITFIELD(codec->REG_0BC, CODEC_CODEC_DRE_INI_ANA_GAIN_CH0, ini_ana_gain);
        codec->REG_0C4 = SET_BITFIELD(codec->REG_0C4, CODEC_CODEC_DRE_INI_ANA_GAIN_CH1, ini_ana_gain);
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

    if (!codec_opened || (codec->REG_088 & CODEC_CODEC_DAC_EN) == 0) {
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
        ini_ana_gain = GET_BITFIELD(codec->REG_0BC, CODEC_CODEC_DRE_INI_ANA_GAIN_CH0);
        digdac_gain_offset_nr = ((0xF - ini_ana_gain) * 11 + 0xF / 2) / 0xF;
        ini_ana_gain = 0xF;
    } else {
        ini_ana_gain = 0xF - (digdac_gain_offset_nr * 0xF + 11 / 2) / 11;
        digdac_gain_offset_nr = 0;
    }

    codec->REG_0BC = SET_BITFIELD(codec->REG_0BC, CODEC_CODEC_DRE_INI_ANA_GAIN_CH0, ini_ana_gain);
    codec->REG_0C4 = SET_BITFIELD(codec->REG_0C4, CODEC_CODEC_DRE_INI_ANA_GAIN_CH1, ini_ana_gain);

#ifdef AUDIO_OUTPUT_SW_GAIN
    hal_codec_set_sw_gain(digdac_gain[0]);
#else
    hal_codec_restore_dig_dac_gain();
#endif
}
#endif

void hal_codec_stop_playback_stream(enum HAL_CODEC_ID_T id)
{
#if (defined(AUDIO_OUTPUT_DC_CALIB_ANA) || defined(AUDIO_OUTPUT_DC_CALIB)) && !(defined(__TWS__) || defined(IBRT))
    // Disable PA
    analog_aud_codec_speaker_enable(false);
#endif

    codec->REG_088 &= ~(CODEC_CODEC_DAC_EN | CODEC_CODEC_DAC_EN_CH0 | CODEC_CODEC_DAC_EN_CH1);

#ifdef DAC_DRE_ENABLE
    hal_codec_dac_dre_disable();
#endif

#if defined(DAC_CLASSG_ENABLE)
    hal_codec_classg_enable(false);
#endif

#ifndef NO_DAC_RESET
    // Reset DAC
    // Avoid DAC outputing noise after it is disabled
    codec->REG_044 &= ~CODEC_SOFT_RSTN_DAC;
    codec->REG_044 |= CODEC_SOFT_RSTN_DAC;
#endif
}

void hal_codec_start_playback_stream(enum HAL_CODEC_ID_T id)
{
#ifndef NO_DAC_RESET
    // Reset DAC
    codec->REG_044 &= ~CODEC_SOFT_RSTN_DAC;
    codec->REG_044 |= CODEC_SOFT_RSTN_DAC;
#endif

#ifdef DAC_DRE_ENABLE
    //if ((codec->REG_02C & CODEC_MODE_16BIT_DAC) == 0)
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

    if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH0) {
        codec->REG_088 |= CODEC_CODEC_DAC_EN_CH0;
    } else {
        codec->REG_088 &= ~CODEC_CODEC_DAC_EN_CH0;
    }
    if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH1) {
        codec->REG_088 |= CODEC_CODEC_DAC_EN_CH1;
    } else {
        codec->REG_088 &= ~CODEC_CODEC_DAC_EN_CH1;
    }

    codec->REG_088 |= CODEC_CODEC_DAC_EN;

#if (defined(AUDIO_OUTPUT_DC_CALIB_ANA) || defined(AUDIO_OUTPUT_DC_CALIB)) && !(defined(__TWS__) || defined(IBRT))
#ifdef AUDIO_OUTPUT_DC_CALIB
    // At least delay 4ms for 8K-sample-rate mute data to arrive at DAC PA
    osDelay(5);
#endif

    // Enable PA
    analog_aud_codec_speaker_enable(true);
#endif
}

int hal_codec_start_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream)
{
    if (stream == AUD_STREAM_PLAYBACK) {
        // Reset and start DAC
        hal_codec_start_playback_stream(id);
    } else {
        {
            // Reset ADC free running clock and ADC ANA
            codec->REG_044 &= ~(RSTN_ADC_FREE_RUNNING_CLK | CODEC_SOFT_RSTN_ADC_ANA);
            codec->REG_044 |= (RSTN_ADC_FREE_RUNNING_CLK | CODEC_SOFT_RSTN_ADC_ANA);
            codec->REG_080 |= CODEC_CODEC_ADC_EN;
        }
    }

    return 0;
}

int hal_codec_stop_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream)
{
    if (stream == AUD_STREAM_PLAYBACK) {
        // Stop and reset DAC
        hal_codec_stop_playback_stream(id);
    } else {
        {
            codec->REG_080 &= ~CODEC_CODEC_ADC_EN;
        }
    }

    return 0;
}

#ifdef __AUDIO_RESAMPLE__
void hal_codec_resample_clock_enable(enum AUD_STREAM_T stream)
{
    uint32_t clk;
    bool set = false;

    // 192K-24BIT requires 52M clock, and 384K-24BIT requires 104M clock
    if (stream == AUD_STREAM_PLAYBACK) {
        clk = codec_dac_sample_rate[resample_rate_idx[AUD_STREAM_PLAYBACK]].sample_rate * RS_CLOCK_FACTOR;
    } else {
        clk = codec_adc_sample_rate[resample_rate_idx[AUD_STREAM_CAPTURE]].sample_rate * RS_CLOCK_FACTOR;
    }

    if (rs_clk_map == 0) {
        set = true;
    } else {
        if (resample_clk_freq < clk) {
            set = true;
        }
    }

    if (set) {
        resample_clk_freq = clk;
        hal_cmu_codec_rs_enable(clk);
    }

    rs_clk_map |= (1 << stream);
}

void hal_codec_resample_clock_disable(enum AUD_STREAM_T stream)
{
    if (rs_clk_map == 0) {
        return;
    }
    rs_clk_map &= ~(1 << stream);
    if (rs_clk_map == 0) {
        hal_cmu_codec_rs_disable();
    }
}
#endif

static void hal_codec_set_ec_down_sel(bool dac_rate_valid)
{
    uint8_t dac_factor;
    uint8_t adc_factor;
    uint8_t d, a;
    uint8_t val;
    uint8_t sel = 0;
    bool err = false;

    if (dac_rate_valid) {
        d = codec_rate_idx[AUD_STREAM_PLAYBACK];
        dac_factor = (6 / codec_dac_sample_rate[d].dac_up) * (codec_dac_sample_rate[d].bypass_cnt + 1);
        a = codec_rate_idx[AUD_STREAM_CAPTURE];
        adc_factor = (6 / codec_adc_sample_rate[a].adc_down) * (codec_adc_sample_rate[a].bypass_cnt + 1);

        val = dac_factor / adc_factor;
        if (val * adc_factor == dac_factor) {
            if (val == 3) {
                sel = 0;
            } else if (val == 6) {
                sel = 1;
            } else if (val == 1) {
                sel = 2;
            } else if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1 && val == 2) {
                sel = 3;
            } else {
                err = true;
            }
        } else {
            err = true;
        }

        ASSERT(!err, "%s: Invalid EC sample rate: play=%u cap=%u", __FUNCTION__,
            codec_dac_sample_rate[d].sample_rate, codec_adc_sample_rate[a].sample_rate);
    } else {
        sel = 0;
    }

    codec->REG_080 = SET_BITFIELD(codec->REG_080, CODEC_CODEC_DOWN_SEL_MC, sel);
}

static void hal_codec_ec_enable(void)
{
    bool dac_rate_valid;
    uint8_t a;

    dac_rate_valid = !!(codec->REG_000 & CODEC_DAC_ENABLE);

    hal_codec_set_ec_down_sel(dac_rate_valid);

    // If no normal ADC chan, ADC0 must be enabled
    if ((codec_adc_ch_map & ~AUD_CHANNEL_MAP_CH1) == 0) {
        a = codec_rate_idx[AUD_STREAM_CAPTURE];
        hal_codec_set_adc_down(AUD_CHANNEL_MAP_CH0, codec_adc_sample_rate[a].adc_down);
        hal_codec_set_adc_hbf_bypass_cnt(AUD_CHANNEL_MAP_CH0, codec_adc_sample_rate[a].bypass_cnt);
        codec->REG_080 |= CODEC_CODEC_ADC_EN_CH0;
    }
    if ((codec->REG_0D0 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) &&
            (codec_adc_ch_map & AUD_CHANNEL_MAP_CH1)) {
        codec->REG_0D0 |= CODEC_CODEC_RESAMPLE_ADC_DUAL_CH;
    }
    codec->REG_080 |= CODEC_CODEC_MC_ENABLE;
}

static void hal_codec_ec_disable(void)
{
    codec->REG_080 &= ~CODEC_CODEC_MC_ENABLE;
    codec->REG_0D0 &= ~CODEC_CODEC_RESAMPLE_ADC_DUAL_CH;
    if ((codec_adc_ch_map & ~AUD_CHANNEL_MAP_CH1) == 0) {
        codec->REG_080 &= ~CODEC_CODEC_ADC_EN_CH0;
    }
}

int hal_codec_start_interface(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream, int dma)
{
    uint32_t fifo_flush = 0;

    if (stream == AUD_STREAM_PLAYBACK) {
#ifdef __AUDIO_RESAMPLE__
        if (codec->REG_0D0 & CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE) {
            codec->REG_0D0 |= CODEC_CODEC_RESAMPLE_DAC_ENABLE;
            hal_codec_resample_clock_enable(stream);
        }
#endif
        if ((codec->REG_000 & CODEC_ADC_ENABLE) && (codec_adc_ch_map & AUD_CHANNEL_MAP_CH1)) {
            hal_codec_set_ec_down_sel(true);
        }
        fifo_flush |= CODEC_TX_FIFO_FLUSH;
        codec->REG_004 |= fifo_flush;
        hal_codec_reg_update_delay();
        codec->REG_004 &= ~fifo_flush;
        if (dma) {
            codec->REG_008 = SET_BITFIELD(codec->REG_008, CODEC_CODEC_TX_THRESHOLD, HAL_CODEC_TX_FIFO_TRIGGER_LEVEL);
            codec->REG_000 |= CODEC_DMACTRL_TX;
            // Delay a little time for DMA to fill the TX FIFO before sending
            for (volatile int i = 0; i < 50; i++);
        }
        codec->REG_000 |= CODEC_DAC_ENABLE;
    } else {
#ifdef __AUDIO_RESAMPLE__
        if (codec->REG_0D0 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) {
            codec->REG_0D0 |= CODEC_CODEC_RESAMPLE_ADC_ENABLE;
            hal_codec_resample_clock_enable(stream);
        }
#endif
        if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH1) {
            hal_codec_ec_enable();
        }
        for (int i = 0; i < MAX_ADC_CH_NUM; i++) {
            if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
                if (i < NORMAL_ADC_CH_NUM) {
                    // Reset ADC channel
                    codec->REG_044 &= ~CODEC_SOFT_RSTN_ADC(1 << i);
                    codec->REG_044 |= CODEC_SOFT_RSTN_ADC(1 << i);
                    codec->REG_080 |= CODEC_CODEC_ADC_EN_CH0;
                }
                codec->REG_000 |= (CODEC_ADC_ENABLE_CH0 << i);
            }
        }
        fifo_flush |= CODEC_RX_FIFO_FLUSH_CH0 | CODEC_RX_FIFO_FLUSH_CH1;
        codec->REG_004 |= fifo_flush;
        hal_codec_reg_update_delay();
        codec->REG_004 &= ~fifo_flush;
        if (dma) {
            codec->REG_008 = SET_BITFIELD(codec->REG_008, CODEC_CODEC_RX_THRESHOLD, HAL_CODEC_RX_FIFO_TRIGGER_LEVEL);
            codec->REG_000 |= CODEC_DMACTRL_RX;
        }
        codec->REG_000 |= CODEC_ADC_ENABLE;
    }

    return 0;
}

int hal_codec_stop_interface(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t fifo_flush = 0;

    if (stream == AUD_STREAM_PLAYBACK) {
        codec->REG_000 &= ~CODEC_DAC_ENABLE;
        codec->REG_000 &= ~CODEC_DMACTRL_TX;
#ifdef __AUDIO_RESAMPLE__
        codec->REG_0D0 &= ~CODEC_CODEC_RESAMPLE_DAC_ENABLE;
        hal_codec_resample_clock_disable(stream);
#endif
        fifo_flush |= CODEC_TX_FIFO_FLUSH;
        codec->REG_004 |= fifo_flush;
        hal_codec_reg_update_delay();
        codec->REG_004 &= ~fifo_flush;
        // Cancel dac sync request
        hal_codec_sync_dac_disable();
        hal_codec_sync_dac_resample_rate_disable();
        hal_codec_sync_dac_gain_disable();
#ifdef NO_DAC_RESET
        // Clean up DAC intermediate states
        osDelay(dac_delay_ms);
#endif
    } else {
        codec->REG_000 &= ~(CODEC_ADC_ENABLE | CODEC_ADC_ENABLE_CH0 | CODEC_ADC_ENABLE_CH1);
        codec->REG_000 &= ~CODEC_DMACTRL_RX;
        codec->REG_080 &= ~CODEC_CODEC_ADC_EN_CH0;
        hal_codec_ec_disable();
#ifdef __AUDIO_RESAMPLE__
        codec->REG_0D0 &= ~CODEC_CODEC_RESAMPLE_ADC_ENABLE;
        hal_codec_resample_clock_disable(stream);
#endif
        fifo_flush |= CODEC_RX_FIFO_FLUSH_CH0 | CODEC_RX_FIFO_FLUSH_CH1;
        codec->REG_004 |= fifo_flush;
        hal_codec_reg_update_delay();
        codec->REG_004 &= ~fifo_flush;
        // Cancel adc sync request
        hal_codec_sync_adc_disable();
        hal_codec_sync_adc_resample_rate_disable();
        hal_codec_sync_adc_gain_disable();
    }

    return 0;
}

static void hal_codec_set_dac_gain_value(enum AUD_CHANNEL_MAP_T map, uint32_t val)
{
    codec->REG_094 &= ~CODEC_CODEC_DAC_GAIN_UPDATE;
    hal_codec_reg_update_delay();
    if (map & AUD_CHANNEL_MAP_CH0) {
        codec->REG_08C = SET_BITFIELD(codec->REG_08C, CODEC_CODEC_DAC_GAIN_CH0, val);
    }
    if (map & AUD_CHANNEL_MAP_CH1) {
        codec->REG_090 = SET_BITFIELD(codec->REG_090, CODEC_CODEC_DAC_GAIN_CH1, val);
    }
    codec->REG_094 |= CODEC_CODEC_DAC_GAIN_UPDATE;
}

void hal_codec_get_dac_gain(float *left_gain,float *right_gain)
{
    uint32_t left_val;
    uint32_t right_val;

    left_val  = GET_BITFIELD(codec->REG_08C, CODEC_CODEC_DAC_GAIN_CH0);
    right_val = GET_BITFIELD(codec->REG_090, CODEC_CODEC_DAC_GAIN_CH1);

    *left_gain = left_val;
    *right_gain = right_val;

    // Gain format: 6.14
    *left_gain /= (1 << 14);
    *right_gain /= (1 << 14);
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

    if (db == ZERODB_DIG_DBVAL) {
        coef = 1;
    } else if (db <= MIN_DIG_DBVAL) {
        coef = 0;
    } else {
        if (db > MAX_DIG_DBVAL) {
            db = MAX_DIG_DBVAL;
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

#if 0
    static const float thd_attn = 0.982878873; // -0.15dB

    // Ensure that THD is good at max gain
    if (coef > thd_attn) {
        coef = thd_attn;
    }
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

#ifdef AUDIO_OUTPUT_DC_CALIB_ANA
    if (codec->REG_088 & CODEC_CODEC_DAC_SDM_CLOSE) {
        mute = true;
    }
#endif

    if (mute) {
        val = 0;
    } else {
        coef = digdac_gain_to_float(gain);

        // Gain format: 6.14
        val = (uint32_t)(coef * (1 << 14));
        val = __USAT(val, 20);
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
    uint32_t gain_update = 0;

    if (map & AUD_CHANNEL_MAP_CH0) {
        codec->REG_084 = SET_BITFIELD(codec->REG_084, CODEC_CODEC_ADC_GAIN_CH0, val);
        gain_update |= CODEC_CODEC_ADC_GAIN_UPDATE_CH0;
    }
    codec->REG_080 &= ~gain_update;
    hal_codec_reg_update_delay();
    codec->REG_080 |= gain_update;
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

int hal_codec_set_chan_vol(enum AUD_STREAM_T stream, enum AUD_CHANNEL_MAP_T ch_map, uint8_t vol)
{
    if (stream == AUD_STREAM_PLAYBACK) {
#ifdef SINGLE_CODEC_DAC_VOL
        ASSERT(false, "%s: Cannot set play chan vol with SINGLE_CODEC_DAC_VOL", __func__);
#else
        const struct CODEC_DAC_VOL_T *vol_tab_ptr;

        vol_tab_ptr = hal_codec_get_dac_volume(vol);
        if (vol_tab_ptr) {
            if (ch_map & AUD_CHANNEL_MAP_CH0) {
                hal_codec_set_dig_adc_gain(AUD_CHANNEL_MAP_CH0, vol_tab_ptr->sdac_volume);
            }
            if (ch_map & AUD_CHANNEL_MAP_CH1) {
                hal_codec_set_dig_adc_gain(AUD_CHANNEL_MAP_CH1, vol_tab_ptr->sdac_volume);
            }
        }
#endif
    } else {
#ifdef SINGLE_CODEC_ADC_VOL
        ASSERT(false, "%s: Cannot set cap chan vol with SINGLE_CODEC_ADC_VOL", __func__);
#else
        uint8_t mic_ch, adc_ch;
        enum AUD_CHANNEL_MAP_T map;
        const CODEC_ADC_VOL_T *adc_gain_db;

        adc_gain_db = hal_codec_get_adc_volume(vol);
        if (adc_gain_db) {
            map = ch_map;
            while (map) {
                mic_ch = get_lsb_pos(map);
                map &= ~(1 << mic_ch);
                adc_ch = hal_codec_get_adc_chan(1 << mic_ch);
                ASSERT(adc_ch < NORMAL_ADC_CH_NUM, "%s: Bad cap ch_map=0x%X (ch=%u)", __func__, ch_map, mic_ch);
                hal_codec_set_dig_adc_gain((1 << adc_ch), *adc_gain_db);
            }
        }
#endif
    }

    return 0;
}

static int hal_codec_set_dac_hbf_bypass_cnt(uint32_t cnt)
{
    uint32_t bypass = 0;
    uint32_t bypass_mask = CODEC_CODEC_DAC_HBF1_BYPASS | CODEC_CODEC_DAC_HBF2_BYPASS | CODEC_CODEC_DAC_HBF3_BYPASS;

    if (cnt == 0) {
    } else if (cnt == 1) {
        bypass = CODEC_CODEC_DAC_HBF3_BYPASS;
    } else if (cnt == 2) {
        bypass = CODEC_CODEC_DAC_HBF2_BYPASS | CODEC_CODEC_DAC_HBF3_BYPASS;
    } else if (cnt == 3) {
        bypass = CODEC_CODEC_DAC_HBF1_BYPASS | CODEC_CODEC_DAC_HBF2_BYPASS | CODEC_CODEC_DAC_HBF3_BYPASS;
    } else {
        ASSERT(false, "%s: Invalid dac bypass cnt: %u", __FUNCTION__, cnt);
    }

    // OSR is fixed to 512
    //codec->REG_088 = SET_BITFIELD(codec->REG_088, CODEC_CODEC_DAC_OSR_SEL, 2);

    codec->REG_088 = (codec->REG_088 & ~bypass_mask) | bypass;
    return 0;
}

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
    codec->REG_088 = SET_BITFIELD(codec->REG_088, CODEC_CODEC_DAC_UP_SEL, sel);
    return 0;
}

static uint32_t POSSIBLY_UNUSED hal_codec_get_dac_up(void)
{
    uint32_t sel;

    sel = GET_BITFIELD(codec->REG_088, CODEC_CODEC_DAC_UP_SEL);
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
        codec->REG_084 = SET_BITFIELD(codec->REG_084, CODEC_CODEC_ADC_DOWN_SEL_CH0, sel);
    }
    return 0;
}

static int hal_codec_set_adc_hbf_bypass_cnt(enum AUD_CHANNEL_MAP_T map, uint32_t cnt)
{
    uint32_t bypass = 0;
    uint32_t bypass_mask = CODEC_CODEC_ADC_HBF1_BYPASS_CH0 | CODEC_CODEC_ADC_HBF2_BYPASS_CH0 | CODEC_CODEC_ADC_HBF3_BYPASS_CH0;

    if (cnt == 0) {
    } else if (cnt == 1) {
        bypass = CODEC_CODEC_ADC_HBF3_BYPASS_CH0;
    } else if (cnt == 2) {
        bypass = CODEC_CODEC_ADC_HBF2_BYPASS_CH0 | CODEC_CODEC_ADC_HBF3_BYPASS_CH0;
    } else if (cnt == 3) {
        bypass = CODEC_CODEC_ADC_HBF1_BYPASS_CH0 | CODEC_CODEC_ADC_HBF2_BYPASS_CH0 | CODEC_CODEC_ADC_HBF3_BYPASS_CH0;
    } else {
        ASSERT(false, "%s: Invalid bypass cnt: %u", __FUNCTION__, cnt);
    }
    if (map & AUD_CHANNEL_MAP_CH0) {
        codec->REG_084 = (codec->REG_084 & ~bypass_mask) | bypass;
    }
    return 0;
}

static void hal_codec_enable_dig_mic(enum AUD_CHANNEL_MAP_T mic_map)
{
    uint32_t phase = 0;
    uint32_t line_map = 0;

    phase = codec->REG_0A0;
    if (mic_map & AUD_CHANNEL_MAP_DIGMIC_CH0) {
        line_map |= (1 << 0);
    }
    phase = (phase & (CODEC_CODEC_PDM_CAP_PHASE_CH0_MASK)) |
       CODEC_CODEC_PDM_CAP_PHASE_CH0(codec_digmic_phase);
    codec->REG_0A0 = phase;
    codec->REG_0A0 |= CODEC_CODEC_PDM_ENABLE;
    hal_iomux_set_dig_mic(line_map);
}

static void hal_codec_disable_dig_mic(void)
{
    codec->REG_0A0 &= ~CODEC_CODEC_PDM_ENABLE;
}

#ifdef __AUDIO_RESAMPLE__
static float get_playback_resample_phase(void)
{
    return (float)codec_dac_sample_rate[resample_rate_idx[AUD_STREAM_PLAYBACK]].codec_freq /
        (hal_cmu_get_crystal_freq() / (CODEC_FREQ_24P576M / CODEC_FREQ_48K_SERIES));
}

static float get_capture_resample_phase(void)
{
    return (float)(hal_cmu_get_crystal_freq() / (CODEC_FREQ_24P576M / CODEC_FREQ_48K_SERIES)) /
        codec_adc_sample_rate[resample_rate_idx[AUD_STREAM_CAPTURE]].codec_freq;
}

static uint32_t resample_phase_float_to_value(float phase)
{
    if (phase >= 4.0) {
        return (uint32_t)-1;
    } else {
        // Phase format: 2.30
        return (uint32_t)(phase * (1 << 30));
    }
}

static float POSSIBLY_UNUSED resample_phase_value_to_float(uint32_t value)
{
    // Phase format: 2.30
    return (float)value / (1 << 30);
}
#endif

int hal_codec_setup_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream, const struct HAL_CODEC_CONFIG_T *cfg)
{
    int i;
    int rate_idx;
    uint32_t ana_dig_div;

    if (stream == AUD_STREAM_PLAYBACK) {
        if ((HAL_CODEC_CONFIG_CHANNEL_MAP | HAL_CODEC_CONFIG_CHANNEL_NUM) & cfg->set_flag) {
            if (cfg->channel_num == AUD_CHANNEL_NUM_2) {
                if (cfg->channel_map != (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)) {
                    TRACE("\n!!! WARNING:%s: Bad play stereo ch map: 0x%X\n", __func__, cfg->channel_map);
                }
                codec->REG_02C |= CODEC_DUAL_CHANNEL_DAC;
            } else {
                ASSERT(cfg->channel_num == AUD_CHANNEL_NUM_1, "%s: Bad play ch num: %u", __func__, cfg->channel_num);
                // Allow to DMA one channel but output 2 channels
                ASSERT((cfg->channel_map & ~(AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)) == 0,
                    "%s: Bad play mono ch map: 0x%X", __func__, cfg->channel_map);
                codec->REG_02C &= ~CODEC_DUAL_CHANNEL_DAC;
            }
            codec_dac_ch_map = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1;
        }

        if (HAL_CODEC_CONFIG_BITS & cfg->set_flag) {
            if (cfg->bits == AUD_BITS_16) {
                codec->REG_02C = (codec->REG_02C & ~CODEC_MODE_32BIT_DAC) | CODEC_MODE_16BIT_DAC;
            } else if (cfg->bits == AUD_BITS_24) {
                codec->REG_02C &= ~(CODEC_MODE_16BIT_DAC | CODEC_MODE_32BIT_DAC);
            } else if (cfg->bits == AUD_BITS_32) {
                codec->REG_02C = (codec->REG_02C & ~CODEC_MODE_16BIT_DAC) | CODEC_MODE_32BIT_DAC;
            } else {
                ASSERT(false, "%s: Bad play bits: %u", __func__, cfg->bits);
            }
        }

        if (HAL_CODEC_CONFIG_SAMPLE_RATE & cfg->set_flag) {
            for (i = 0; i < ARRAY_SIZE(codec_dac_sample_rate); i++) {
                if (codec_dac_sample_rate[i].sample_rate == cfg->sample_rate) {
                    break;
                }
            }
            ASSERT(i < ARRAY_SIZE(codec_dac_sample_rate), "%s: Invalid playback sample rate: %u", __func__, cfg->sample_rate);
            rate_idx = i;
            ana_dig_div = codec_dac_sample_rate[rate_idx].codec_div / codec_dac_sample_rate[rate_idx].cmu_div;
            ASSERT(ana_dig_div * codec_dac_sample_rate[rate_idx].cmu_div == codec_dac_sample_rate[rate_idx].codec_div,
                "%s: Invalid playback div for rate %u: codec_div=%u cmu_div=%u", __func__, cfg->sample_rate,
                codec_dac_sample_rate[rate_idx].codec_div, codec_dac_sample_rate[rate_idx].cmu_div);

            TRACE("[%s] playback sample_rate=%d", __func__, cfg->sample_rate);

            codec_rate_idx[AUD_STREAM_PLAYBACK] = rate_idx;

#ifdef __AUDIO_RESAMPLE__
            uint32_t mask, val;

            mask = CODEC_CODEC_RESAMPLE_DAC_L_ENABLE | CODEC_CODEC_RESAMPLE_DAC_R_ENABLE |
                CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;

            if (hal_cmu_get_audio_resample_status() && codec_dac_sample_rate[rate_idx].codec_freq != CODEC_FREQ_CRYSTAL) {
                if ((codec->REG_0D0 & CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE) == 0 ||
                        resample_rate_idx[AUD_STREAM_PLAYBACK] != rate_idx) {
                    resample_rate_idx[AUD_STREAM_PLAYBACK] = rate_idx;
                    codec->REG_0D0 &= ~mask;
                    hal_codec_reg_update_delay();
                    codec->REG_0D4 = resample_phase_float_to_value(get_playback_resample_phase());
                    hal_codec_reg_update_delay();
                    codec->REG_0D0 |= CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
                    val = 0;
                    if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH0) {
                        val |= CODEC_CODEC_RESAMPLE_DAC_L_ENABLE;
                    }
                    if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH1) {
                        val |= CODEC_CODEC_RESAMPLE_DAC_R_ENABLE;
                    }
                    codec->REG_0D0 |= val;
                }
            } else {
                codec->REG_0D0 &= ~mask;
            }
#endif

            // 8K -> 4ms, 16K -> 2ms, ...
            dac_delay_ms = 4 / ((cfg->sample_rate + AUD_SAMPRATE_8000 / 2) / AUD_SAMPRATE_8000);
            if (dac_delay_ms < 2) {
                dac_delay_ms = 2;
            }

            hal_codec_set_dac_up(codec_dac_sample_rate[rate_idx].dac_up);
            hal_codec_set_dac_hbf_bypass_cnt(codec_dac_sample_rate[rate_idx].bypass_cnt);
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
        enum AUD_CHANNEL_MAP_T mic_map;
        uint8_t cnt;
        uint32_t cfg_set_mask;
        uint32_t cfg_clr_mask;

        mic_map = 0;
        if ((HAL_CODEC_CONFIG_CHANNEL_MAP | HAL_CODEC_CONFIG_CHANNEL_NUM) & cfg->set_flag) {
            codec_adc_ch_map = 0;
            codec_mic_ch_map = 0;
            mic_map = cfg->channel_map;
        }

        if (mic_map) {
            codec_mic_ch_map = mic_map;

            if (codec_mic_ch_map & AUD_CHANNEL_MAP_DIGMIC_ALL) {
                hal_codec_enable_dig_mic(mic_map);
            } else {
                hal_codec_disable_dig_mic();
            }

            if (mic_map & AUD_CHANNEL_MAP_CH1) {
                mic_map &= ~AUD_CHANNEL_MAP_CH1;
                codec_adc_ch_map |= AUD_CHANNEL_MAP_CH1;
                codec->REG_080 &= ~CODEC_CODEC_MC_SEL;
            }

            if (mic_map & NORMAL_MIC_MAP) {
                codec_adc_ch_map |= AUD_CHANNEL_MAP_CH0;
                if (mic_map & AUD_CHANNEL_MAP_DIGMIC_CH0) {
                    mic_map &= ~AUD_CHANNEL_MAP_DIGMIC_CH0;
                    codec->REG_0A0 |= CODEC_CODEC_PDM_ADC_SEL_CH0;
                } else {
                    mic_map &= ~AUD_CHANNEL_MAP_CH0;
                    codec->REG_0A0 &= ~CODEC_CODEC_PDM_ADC_SEL_CH0;
                }
            }

            ASSERT(mic_map == 0, "%s: Bad cap chan map: 0x%X", __func__, mic_map);
        }

        cfg_set_mask = 0;
        cfg_clr_mask = 0;
        if (HAL_CODEC_CONFIG_BITS & cfg->set_flag) {
            if (cfg->bits == AUD_BITS_16) {
                codec->REG_028 &= ~(CODEC_MODE_24BIT_ADC | CODEC_MODE_32BIT_ADC);
                cfg_set_mask = CODEC_MODE_16BIT_ADC_CH0;
            } else if (cfg->bits == AUD_BITS_24) {
                codec->REG_028 = (codec->REG_028 & ~CODEC_MODE_32BIT_ADC) | CODEC_MODE_24BIT_ADC;
                cfg_clr_mask = CODEC_MODE_16BIT_ADC_CH0;
            } else if (cfg->bits == AUD_BITS_32) {
                codec->REG_028 = (codec->REG_028 & ~CODEC_MODE_24BIT_ADC) | CODEC_MODE_32BIT_ADC;
                cfg_clr_mask = CODEC_MODE_16BIT_ADC_CH0;
            } else {
                ASSERT(false, "%s: Bad cap bits: %u", __func__, cfg->bits);
            }
        }

        cnt = 0;
        for (i = 0; i < MAX_ADC_CH_NUM; i++) {
            if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
                cnt++;
                codec->REG_028 |= (cfg_set_mask << i);
                codec->REG_028 &= ~(cfg_clr_mask << i);
            } else {
                codec->REG_000 &= ~(CODEC_ADC_ENABLE_CH0 << i);
            }
        }
        ASSERT(cnt == cfg->channel_num, "%s: Invalid capture stream chan cfg: map=0x%X num=%u",
            __func__, codec_adc_ch_map, cfg->channel_num);

        if (HAL_CODEC_CONFIG_SAMPLE_RATE & cfg->set_flag) {
            for(i = 0; i < ARRAY_SIZE(codec_adc_sample_rate); i++) {
                if(codec_adc_sample_rate[i].sample_rate == cfg->sample_rate) {
                    break;
                }
            }
            ASSERT(i < ARRAY_SIZE(codec_adc_sample_rate), "%s: Invalid capture sample rate: %d", __func__, cfg->sample_rate);
            rate_idx = i;
            ana_dig_div = codec_adc_sample_rate[rate_idx].codec_div / codec_adc_sample_rate[rate_idx].cmu_div;
            ASSERT(ana_dig_div * codec_adc_sample_rate[rate_idx].cmu_div == codec_adc_sample_rate[rate_idx].codec_div,
                "%s: Invalid catpure div for rate %u: codec_div=%u cmu_div=%u", __func__, cfg->sample_rate,
                codec_adc_sample_rate[rate_idx].codec_div, codec_adc_sample_rate[rate_idx].cmu_div);

            TRACE("[%s] capture sample_rate=%d", __func__, cfg->sample_rate);

            codec_rate_idx[AUD_STREAM_CAPTURE] = rate_idx;

#ifdef __AUDIO_RESAMPLE__
            uint32_t mask;

            mask = CODEC_CODEC_RESAMPLE_ADC_DUAL_CH | CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;

            if (hal_cmu_get_audio_resample_status() && codec_adc_sample_rate[rate_idx].codec_freq != CODEC_FREQ_CRYSTAL) {
                if ((codec->REG_0D0 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) == 0 ||
                        resample_rate_idx[AUD_STREAM_CAPTURE] != rate_idx) {
                    resample_rate_idx[AUD_STREAM_CAPTURE] = rate_idx;
                    codec->REG_0D0 &= ~mask;
                    hal_codec_reg_update_delay();
                    codec->REG_0D8 = resample_phase_float_to_value(get_capture_resample_phase());
                    hal_codec_reg_update_delay();
                    codec->REG_0D0 |= CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
                    ASSERT(cfg->channel_num == AUD_CHANNEL_NUM_2 || cfg->channel_num == AUD_CHANNEL_NUM_1,
                        "%s: Invalid capture resample chan num: %d", __func__, cfg->channel_num);
                }
            } else {
                codec->REG_0D0 &= ~mask;
            }
#endif

            hal_codec_set_adc_down(codec_adc_ch_map, codec_adc_sample_rate[rate_idx].adc_down);
            hal_codec_set_adc_hbf_bypass_cnt(codec_adc_ch_map, codec_adc_sample_rate[rate_idx].bypass_cnt);
        }

#if !(defined(FIXED_CODEC_ADC_VOL) && defined(SINGLE_CODEC_ADC_VOL))
        if (HAL_CODEC_CONFIG_VOL & cfg->set_flag) {
#ifdef SINGLE_CODEC_ADC_VOL
            const CODEC_ADC_VOL_T *adc_gain_db;
            adc_gain_db = hal_codec_get_adc_volume(cfg->vol);
            if (adc_gain_db) {
                hal_codec_set_dig_adc_gain(NORMAL_ADC_MAP, *adc_gain_db);
            }
#else
            uint32_t vol;
            uint8_t ch_idx;

            mic_map = codec_mic_ch_map;
            while (mic_map) {
                ch_idx = get_lsb_pos(mic_map);
                mic_map &= ~(1 << ch_idx);
                vol = hal_codec_get_mic_chan_volume_level(1 << ch_idx);
                hal_codec_set_chan_vol(AUD_STREAM_CAPTURE, (1 << ch_idx), vol);
            }
#endif
        }
#endif
    }

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
    // DC calib values are based on 16-bit, but hardware compensation is based on 24-bit
    dac_dc_l = dc_l << 8;
    dac_dc_r = dc_r << 8;
#ifdef SDM_MUTE_NOISE_SUPPRESSION
    if (dac_dc_l == 0) {
        dac_dc_l = 1;
    }
    if (dac_dc_r == 0) {
        dac_dc_r = 1;
    }
#endif
}
#endif

void hal_codec_set_dac_reset_callback(HAL_CODEC_DAC_RESET_CALLBACK callback)
{
    //dac_reset_callback = callback;
}

static uint32_t POSSIBLY_UNUSED hal_codec_get_adc_chan(enum AUD_CHANNEL_MAP_T mic_map)
{
    uint8_t adc_ch;
    uint8_t mic_ch;

    adc_ch = MAX_ADC_CH_NUM;

    mic_ch = get_lsb_pos(mic_map);

    if (((1 << mic_ch) & codec_mic_ch_map) == 0) {
        return adc_ch;
    }

    adc_ch = 0;

    return adc_ch;
}

void hal_codec_sidetone_enable(void)
{
#ifdef SIDETONE_ENABLE
    enum AUD_CHANNEL_MAP_T mic_map = CFG_HW_AUD_SIDETONE_MIC_DEV;
    int gain = CFG_HW_AUD_SIDETONE_GAIN_DBVAL;
    uint32_t val;

    if ((mic_map & (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_DIGMIC_CH0)) == 0) {
        return;
    }

    if (gain > MAX_SIDETONE_DBVAL) {
        gain = MAX_SIDETONE_DBVAL;
    } else if (gain < MIN_SIDETONE_DBVAL) {
        gain = MIN_SIDETONE_DBVAL;
    }

    val = MIN_SIDETONE_REGVAL + (gain - MIN_SIDETONE_DBVAL) / SIDETONE_DBVAL_STEP;

    codec->REG_080 = SET_BITFIELD(codec->REG_080, CODEC_CODEC_SIDE_TONE_GAIN, val);
#endif
}

void hal_codec_sidetone_disable(void)
{
#ifdef SIDETONE_ENABLE
    codec->REG_080 = SET_BITFIELD(codec->REG_080, CODEC_CODEC_SIDE_TONE_GAIN, MIN_SIDETONE_REGVAL);
#endif
}

void hal_codec_sync_dac_enable(enum HAL_CODEC_SYNC_TYPE_T type)
{
    codec->REG_038 = SET_BITFIELD(codec->REG_038, CODEC_CODEC_DAC_ENABLE_SEL, type);
}

void hal_codec_sync_dac_disable(void)
{
    codec->REG_038 = SET_BITFIELD(codec->REG_038, CODEC_CODEC_DAC_ENABLE_SEL, HAL_CODEC_SYNC_TYPE_NONE);
}

void hal_codec_sync_adc_enable(enum HAL_CODEC_SYNC_TYPE_T type)
{
    codec->REG_038 = SET_BITFIELD(codec->REG_038, CODEC_CODEC_ADC_ENABLE_SEL, type);
}

void hal_codec_sync_adc_disable(void)
{
    codec->REG_038 = SET_BITFIELD(codec->REG_038, CODEC_CODEC_ADC_ENABLE_SEL, HAL_CODEC_SYNC_TYPE_NONE);
}

void hal_codec_sync_dac_resample_rate_enable(enum HAL_CODEC_SYNC_TYPE_T type)
{
    codec->REG_0D0 = SET_BITFIELD(codec->REG_0D0, CODEC_CODEC_RESAMPLE_DAC_TRIGGER_SEL, type);
}

void hal_codec_sync_dac_resample_rate_disable(void)
{
    codec->REG_0D0 = SET_BITFIELD(codec->REG_0D0, CODEC_CODEC_RESAMPLE_DAC_TRIGGER_SEL, HAL_CODEC_SYNC_TYPE_NONE);
}

void hal_codec_sync_adc_resample_rate_enable(enum HAL_CODEC_SYNC_TYPE_T type)
{
    codec->REG_0D0 = SET_BITFIELD(codec->REG_0D0, CODEC_CODEC_RESAMPLE_ADC_TRIGGER_SEL, type);
}

void hal_codec_sync_adc_resample_rate_disable(void)
{
    codec->REG_0D0 = SET_BITFIELD(codec->REG_0D0, CODEC_CODEC_RESAMPLE_ADC_TRIGGER_SEL, HAL_CODEC_SYNC_TYPE_NONE);
}

void hal_codec_sync_dac_gain_enable(enum HAL_CODEC_SYNC_TYPE_T type)
{
    codec->REG_094 = SET_BITFIELD(codec->REG_094, CODEC_CODEC_DAC_GAIN_TRIGGER_SEL, type);
}

void hal_codec_sync_dac_gain_disable(void)
{
    codec->REG_094 = SET_BITFIELD(codec->REG_094, CODEC_CODEC_DAC_GAIN_TRIGGER_SEL, HAL_CODEC_SYNC_TYPE_NONE);
}

void hal_codec_sync_adc_gain_enable(enum HAL_CODEC_SYNC_TYPE_T type)
{
}

void hal_codec_sync_adc_gain_disable(void)
{
}

#ifdef AUDIO_OUTPUT_DC_CALIB_ANA
int hal_codec_dac_sdm_reset_set(void)
{
    if (codec_opened) {
        hal_codec_set_dac_gain_value(VALID_DAC_MAP, 0);
        if (codec->REG_088 & CODEC_CODEC_DAC_EN) {
            osDelay(dac_delay_ms);
        }
        codec->REG_088 |= CODEC_CODEC_DAC_SDM_CLOSE;
    }

    return 0;
}

int hal_codec_dac_sdm_reset_clear(void)
{
    if (codec_opened) {
        codec->REG_088 &= ~CODEC_CODEC_DAC_SDM_CLOSE;
        hal_codec_restore_dig_dac_gain();
    }

    return 0;
}
#endif

void hal_codec_tune_resample_rate(enum AUD_STREAM_T stream, float ratio)
{
#ifdef __AUDIO_RESAMPLE__
    uint32_t val;

    if (!codec_opened) {
        return;
    }

    if (stream == AUD_STREAM_PLAYBACK) {
        if (codec->REG_0D0 & CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE) {
            codec->REG_0D0 &= ~CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
            hal_codec_reg_update_delay();
            val = resample_phase_float_to_value(get_playback_resample_phase());
            val += (int)(val * ratio);
            codec->REG_0D4 = val;
            hal_codec_reg_update_delay();
            codec->REG_0D0 |= CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
        }
    } else {
        if (codec->REG_0D0 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) {
            codec->REG_0D0 &= ~CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
            hal_codec_reg_update_delay();
            val = resample_phase_float_to_value(get_capture_resample_phase());
            val -= (int)(val * ratio);
            codec->REG_0D8 = val;
            hal_codec_reg_update_delay();
            codec->REG_0D0 |= CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
        }
    }
#endif
}

void hal_codec_tune_both_resample_rate(float ratio)
{
#ifdef __AUDIO_RESAMPLE__
    bool update[2];
    uint32_t val[2];
    uint32_t lock;

    if (!codec_opened) {
        return;
    }

    update[0] = !!(codec->REG_0D0 & CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE);
    update[1] = !!(codec->REG_0D0 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE);

    if (update[0]) {
        codec->REG_0D0 &= ~CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
        val[0] = resample_phase_float_to_value(get_playback_resample_phase());
        val[0] += (int)(val[0] * ratio);
    }
    if (update[1]) {
        codec->REG_0D0 &= ~CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
        val[1] = resample_phase_float_to_value(get_capture_resample_phase());
        val[1] -= (int)(val[1] * ratio);
    }

    hal_codec_reg_update_delay();

    lock = int_lock();
    if (update[0]) {
        codec->REG_0D4 = val[0];
    }
    if (update[1]) {
        codec->REG_0D8 = val[1];
    }
    int_unlock(lock);

    hal_codec_reg_update_delay();

    if (update[0]) {
        codec->REG_0D0 |= CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
    }
    if (update[1]) {
        codec->REG_0D0 |= CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
    }
#endif
}

int hal_codec_select_clock_out(uint32_t cfg)
{
    uint32_t lock;
    int ret = 1;

    lock = int_lock();

    if (codec_opened) {
        codec->REG_03C = SET_BITFIELD(codec->REG_03C, CODEC_CFG_CLK_OUT, cfg);
        ret = 0;
    }

    int_unlock(lock);

    return ret;
}

void hal_codec_swap_output(bool swap)
{
#ifdef AUDIO_OUTPUT_SWAP
    output_swap = swap;

    if (codec_opened) {
        if (output_swap) {
            codec->REG_090 |= CODEC_CODEC_DAC_OUT_SWAP;
        } else {
            codec->REG_090 &= ~CODEC_CODEC_DAC_OUT_SWAP;
        }
    }
#endif
}

