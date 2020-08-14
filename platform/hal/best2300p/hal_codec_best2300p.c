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
#include "reg_codec_best2300p.h"
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

#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
//#define CODEC_TIMER
#endif

#ifdef CODEC_DSD
#ifdef ANC_APP
#error "ANC_APP conflicts with CODEC_DSD"
#endif
#ifdef AUDIO_ANC_FB_MC
#error "AUDIO_ANC_FB_MC conflicts with CODEC_DSD"
#endif
#ifdef __AUDIO_RESAMPLE__
#error "__AUDIO_RESAMPLE__ conflicts with CODEC_DSD"
#endif
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

#ifndef MC_DELAY_COMMON
#define MC_DELAY_COMMON                     26
#endif

#ifndef CODEC_DIGMIC_PHASE
#define CODEC_DIGMIC_PHASE                  7
#endif

#define ADC_IIR_CH_NUM                      2

#define MAX_DIG_MIC_CH_NUM                  5

#define NORMAL_ADC_CH_NUM                   5
// Echo cancel ADC channel number
#define EC_ADC_CH_NUM                       2
#define MAX_ADC_CH_NUM                      (NORMAL_ADC_CH_NUM + EC_ADC_CH_NUM)

#define MAX_DAC_CH_NUM                      2

#ifdef CODEC_DSD
#define NORMAL_MIC_MAP                      (AUD_CHANNEL_MAP_ALL & ~(AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6 | AUD_CHANNEL_MAP_CH7 | \
                                                AUD_CHANNEL_MAP_DIGMIC_CH5 | AUD_CHANNEL_MAP_DIGMIC_CH6 | AUD_CHANNEL_MAP_DIGMIC_CH7 | \
                                                AUD_CHANNEL_MAP_DIGMIC_CH2 | AUD_CHANNEL_MAP_DIGMIC_CH3))
#else
#define NORMAL_MIC_MAP                      (AUD_CHANNEL_MAP_ALL & ~(AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6 | AUD_CHANNEL_MAP_CH7 | \
                                                AUD_CHANNEL_MAP_DIGMIC_CH5 | AUD_CHANNEL_MAP_DIGMIC_CH6 | AUD_CHANNEL_MAP_DIGMIC_CH7))
#endif
#define NORMAL_ADC_MAP                      (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1 | AUD_CHANNEL_MAP_CH2 | AUD_CHANNEL_MAP_CH3 | AUD_CHANNEL_MAP_CH4)

#define EC_MIC_MAP                          (AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6)
#define EC_ADC_MAP                          (AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6)

#define VALID_MIC_MAP                       (NORMAL_MIC_MAP | EC_MIC_MAP)
#define VALID_ADC_MAP                       (NORMAL_ADC_MAP | EC_ADC_MAP)

#define VALID_SPK_MAP                       (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)
#define VALID_DAC_MAP                       VALID_SPK_MAP

#if (CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV & ~VALID_SPK_MAP)
#error "Invalid CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV"
#endif

#define RSTN_ADC_FREE_RUNNING_CLK           CODEC_SOFT_RSTN_ADC(1 << NORMAL_ADC_CH_NUM)

enum CODEC_ADC_EN_REQ_T {
    CODEC_ADC_EN_REQ_STREAM,
    CODEC_ADC_EN_REQ_MC,
    CODEC_ADC_EN_REQ_DSD,

    CODEC_ADC_EN_REQ_QTY,
};

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
    uint8_t mc_delay;
};

static const struct CODEC_DAC_SAMPLE_RATE_T codec_dac_sample_rate[] = {
#ifdef __AUDIO_RESAMPLE__
    {AUD_SAMPRATE_8463,      CODEC_FREQ_CRYSTAL,               1,             1, 6, 0, MC_DELAY_COMMON + 160},
    {AUD_SAMPRATE_16927,     CODEC_FREQ_CRYSTAL,               1,             1, 3, 0, MC_DELAY_COMMON +  85},
    {AUD_SAMPRATE_50781,     CODEC_FREQ_CRYSTAL,               1,             1, 1, 0, MC_DELAY_COMMON +  29},
#endif
    {AUD_SAMPRATE_7350,   CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 6, 0, MC_DELAY_COMMON + 174},
    {AUD_SAMPRATE_8000,   CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 6, 0, MC_DELAY_COMMON + 168}, // T
    {AUD_SAMPRATE_14700,  CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 3, 0, MC_DELAY_COMMON +  71},
    {AUD_SAMPRATE_16000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 3, 0, MC_DELAY_COMMON +  88}, // T
    {AUD_SAMPRATE_22050,  CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 2, 0, MC_DELAY_COMMON +  60},
    {AUD_SAMPRATE_24000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 2, 0, MC_DELAY_COMMON +  58},
    {AUD_SAMPRATE_44100,  CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 0, MC_DELAY_COMMON +  31}, // T
    {AUD_SAMPRATE_48000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 0, MC_DELAY_COMMON +  30}, // T
    {AUD_SAMPRATE_88200,  CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 1, MC_DELAY_COMMON +  12},
    {AUD_SAMPRATE_96000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 1, MC_DELAY_COMMON +  12}, // T
    {AUD_SAMPRATE_176400, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 2, MC_DELAY_COMMON +   5},
    {AUD_SAMPRATE_192000, CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 2, MC_DELAY_COMMON +   5},
    {AUD_SAMPRATE_352800, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 3, MC_DELAY_COMMON +   2},
    {AUD_SAMPRATE_384000, CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 3, MC_DELAY_COMMON +   2},
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
    HAL_CODEC_OPEN_ANC_INIT     = (1 << 1),
};

#ifdef ANC_APP
static enum HAL_CODEC_OPEN_T open_type;
static int8_t digdac_gain_offset_anc;
#endif
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
#ifdef CODEC_TIMER
static uint32_t cur_codec_freq;
#endif
// EC
static uint8_t codec_rate_idx[AUD_STREAM_NUM];

//static HAL_CODEC_DAC_RESET_CALLBACK dac_reset_callback;

static enum AUD_CHANNEL_MAP_T codec_dac_ch_map;
static enum AUD_CHANNEL_MAP_T codec_adc_ch_map;
static enum AUD_CHANNEL_MAP_T anc_adc_ch_map;
static enum AUD_CHANNEL_MAP_T codec_mic_ch_map;
static enum AUD_CHANNEL_MAP_T anc_mic_ch_map;
#ifdef SIDETONE_ENABLE
static enum AUD_CHANNEL_MAP_T sidetone_adc_ch_map;
static int8_t sidetone_adc_gain;
static int8_t sidetone_gain_offset;
#endif

static uint8_t dac_delay_ms;

#ifdef ANC_PROD_TEST
#define OPT_TYPE
#else
#define OPT_TYPE                        const
#endif

static OPT_TYPE uint8_t codec_digmic_phase = CODEC_DIGMIC_PHASE;

#if defined(AUDIO_ANC_FB_MC) && defined(__AUDIO_RESAMPLE__)
#error "Music cancel cannot work with audio resample"
#endif
#if defined(AUDIO_ANC_FB_MC) && (defined(__TWS__) || defined(IBRT))
#error "Music cancel cannot work with TWS")
#endif
#ifdef AUDIO_ANC_FB_MC
static bool mc_enabled;
static bool mc_dual_chan;
static bool mc_16bit;
#endif

#ifdef CODEC_DSD
static bool dsd_enabled;
static uint8_t dsd_rate_idx;
#endif

#if defined(AUDIO_ANC_FB_MC) || defined(CODEC_DSD)
static uint8_t adc_en_map;
STATIC_ASSERT(sizeof(adc_en_map) * 8 >= CODEC_ADC_EN_REQ_QTY, "adc_en_map size too small");
#endif

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
#ifdef __AUDIO_RESAMPLE__
static float get_playback_resample_phase(void);
static float get_capture_resample_phase(void);
static uint32_t resample_phase_float_to_value(float phase);
static float resample_phase_value_to_float(uint32_t value);
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

        codec->REG_0B4 = SET_BITFIELD(codec->REG_0B4, CODEC_CODEC_CLASSG_THD2, config->thd2);
        codec->REG_0B4 = SET_BITFIELD(codec->REG_0B4, CODEC_CODEC_CLASSG_THD1, config->thd1);
        codec->REG_0B4 = SET_BITFIELD(codec->REG_0B4, CODEC_CODEC_CLASSG_THD0, config->thd0);

        // Make class-g set the lowest gain after several samples.
        // Class-g gain will have impact on dc.
        codec->REG_0B0 = SET_BITFIELD(codec->REG_0B0, CODEC_CODEC_CLASSG_WINDOW, 6);

        if (config->lr)
            codec->REG_0B0 |= CODEC_CODEC_CLASSG_LR;
        else
            codec->REG_0B0 &= ~CODEC_CODEC_CLASSG_LR;

        if (config->step_4_3n)
            codec->REG_0B0 |= CODEC_CODEC_CLASSG_STEP_3_4N;
        else
            codec->REG_0B0 &= ~CODEC_CODEC_CLASSG_STEP_3_4N;

        if (config->quick_down)
            codec->REG_0B0 |= CODEC_CODEC_CLASSG_QUICK_DOWN;
        else
            codec->REG_0B0 &= ~CODEC_CODEC_CLASSG_QUICK_DOWN;

        codec->REG_0B0 |= CODEC_CODEC_CLASSG_EN;

        // Restore class-g window after the gain has been updated
        hal_codec_reg_update_delay();
        codec->REG_0B0 = SET_BITFIELD(codec->REG_0B0, CODEC_CODEC_CLASSG_WINDOW, config->wind_width);
    } else {
        codec->REG_0B0 &= ~CODEC_CODEC_CLASSG_QUICK_DOWN;
    }
}
#endif

#if defined(AUDIO_OUTPUT_DC_CALIB) || defined(SDM_MUTE_NOISE_SUPPRESSION)
static void hal_codec_dac_dc_offset_enable(int32_t dc_l, int32_t dc_r)
{
    codec->REG_0E0 &= CODEC_CODEC_DAC_DC_UPDATE_CH0;
    hal_codec_reg_update_delay();
    codec->REG_0E8 = SET_BITFIELD(codec->REG_0E8, CODEC_CODEC_DAC_DC_CH1, dc_r);
    codec->REG_0E0 = SET_BITFIELD(codec->REG_0E0, CODEC_CODEC_DAC_DC_CH0, dc_l) | CODEC_CODEC_DAC_DC_UPDATE_CH0;
}
#endif

static int hal_codec_int_open(enum HAL_CODEC_OPEN_T type)
{
    int i;

#ifdef ANC_APP
    enum HAL_CODEC_OPEN_T old_type;

    old_type = open_type;
    open_type |= type;
    if (old_type) {
        return 0;
    }
#endif

    bool first_open = !codec_init;

    analog_aud_pll_open(ANA_AUD_PLL_USER_CODEC);

    if (!codec_init) {
        for (i = 0; i < CFG_HW_AUD_INPUT_PATH_NUM; i++) {
            if (cfg_audio_input_path_cfg[i].cfg & AUD_CHANNEL_MAP_ALL & ~VALID_MIC_MAP) {
                ASSERT(false, "Invalid input path cfg: i=%d io_path=%d cfg=0x%X",
                    i, cfg_audio_input_path_cfg[i].io_path, cfg_audio_input_path_cfg[i].cfg);
            }
        }
        // It will power down some modules first
        hal_psc_codec_enable();
        codec_init = true;
    }
    hal_cmu_codec_clock_enable();
    hal_cmu_codec_reset_clear();

    codec_opened = true;

    codec->REG_060 |= CODEC_EN_CLK_DAC | CODEC_EN_CLK_ADC_MASK | CODEC_EN_CLK_ADC_ANA_MASK | CODEC_POL_ADC_ANA_MASK;
    codec->REG_064 |= CODEC_SOFT_RSTN_32K | CODEC_SOFT_RSTN_IIR | CODEC_SOFT_RSTN_RS |
        CODEC_SOFT_RSTN_DAC | CODEC_SOFT_RSTN_ADC_MASK | CODEC_SOFT_RSTN_ADC_ANA_MASK;
    codec->REG_000 &= ~(CODEC_DMACTRL_TX | CODEC_DMACTRL_RX | CODEC_DAC_ENABLE |
        CODEC_ADC_ENABLE_CH0 | CODEC_ADC_ENABLE_CH1 | CODEC_ADC_ENABLE_CH2 | CODEC_ADC_ENABLE_CH3 |
        CODEC_ADC_ENABLE_CH4 | CODEC_ADC_ENABLE | CODEC_CODEC_IF_EN);
    codec->REG_04C &= ~CODEC_MC_ENABLE;
    codec->REG_004 |= CODEC_RX_FIFO_FLUSH_CH0 | CODEC_RX_FIFO_FLUSH_CH1 | CODEC_RX_FIFO_FLUSH_CH2 |
        CODEC_RX_FIFO_FLUSH_CH3 | CODEC_RX_FIFO_FLUSH_CH4 | CODEC_TX_FIFO_FLUSH |
        CODEC_DSD_TX_FIFO_FLUSH | CODEC_MC_FIFO_FLUSH | CODEC_IIR_RX_FIFO_FLUSH | CODEC_IIR_TX_FIFO_FLUSH;
    hal_codec_reg_update_delay();
    codec->REG_004 &= ~(CODEC_RX_FIFO_FLUSH_CH0 | CODEC_RX_FIFO_FLUSH_CH1 | CODEC_RX_FIFO_FLUSH_CH2 |
        CODEC_RX_FIFO_FLUSH_CH3 | CODEC_RX_FIFO_FLUSH_CH4 | CODEC_TX_FIFO_FLUSH |
        CODEC_DSD_TX_FIFO_FLUSH | CODEC_MC_FIFO_FLUSH | CODEC_IIR_RX_FIFO_FLUSH | CODEC_IIR_TX_FIFO_FLUSH);
    codec->REG_000 |= CODEC_CODEC_IF_EN;

#ifdef AUDIO_OUTPUT_SWAP
    if (output_swap) {
        codec->REG_0A0 |= CODEC_CODEC_DAC_OUT_SWAP;
    } else {
        codec->REG_0A0 &= ~CODEC_CODEC_DAC_OUT_SWAP;
    }
#endif

    if (first_open) {
#ifdef AUDIO_ANC_FB_MC
        codec->REG_04C = CODEC_DMA_CTRL_MC;
#endif

        // ANC zero-crossing
        codec->REG_0D4 |= CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FF_CH0 | CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FF_CH1;
        codec->REG_0D8 |= CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FB_CH0 | CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FB_CH1;

        // Enable ADC zero-crossing gain adjustment
        for (i = 0; i < NORMAL_ADC_CH_NUM; i++) {
            *(&codec->REG_084 + i) |= CODEC_CODEC_ADC_GAIN_SEL_CH0;
        }

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
        codec->REG_0C0 = CODEC_CODEC_DRE_INI_ANA_GAIN_CH0(ini_gain) | CODEC_CODEC_DRE_GAIN_OFFSET_CH0(dre_offset);
        codec->REG_0C8 = CODEC_CODEC_DRE_INI_ANA_GAIN_CH1(ini_gain) | CODEC_CODEC_DRE_GAIN_OFFSET_CH1(dre_offset);
        codec->REG_0E0 = CODEC_CODEC_DAC_ANA_GAIN_UPDATE_DELAY_CH0(0);
        codec->REG_0E8 = CODEC_CODEC_DAC_ANA_GAIN_UPDATE_DELAY_CH1(0);

#ifdef AUDIO_ANC_FB_MC
        // Enable ADC + music cancel.
        codec->REG_130 |= CODEC_CODEC_FB_CHECK_KEEP_CH0;
        codec->REG_134 |= CODEC_CODEC_FB_CHECK_KEEP_CH1;
#endif

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
        codec->REG_09C |= CODEC_CODEC_DAC_GAIN_SEL_CH0;
        codec->REG_0A0 |= CODEC_CODEC_DAC_GAIN_SEL_CH1;
#endif
        hal_codec_set_dac_gain_value(VALID_DAC_MAP, 0);

#ifdef AUDIO_OUTPUT_DC_CALIB_ANA
        // Reset SDM
        codec->REG_098 |= CODEC_CODEC_DAC_SDM_CLOSE;
#endif

#ifdef __AUDIO_RESAMPLE__
        codec->REG_0E4 &= ~(CODEC_CODEC_RESAMPLE_DAC_ENABLE | CODEC_CODEC_RESAMPLE_ADC_ENABLE |
            CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE | CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE);
#endif

#ifdef CODEC_DSD
        for(i = 0; i < ARRAY_SIZE(codec_adc_sample_rate); i++) {
            if(codec_adc_sample_rate[i].sample_rate == AUD_SAMPRATE_44100) {
                break;
            }
        }
        hal_codec_set_adc_down((AUD_CHANNEL_MAP_CH2 | AUD_CHANNEL_MAP_CH3), codec_adc_sample_rate[i].adc_down);
        hal_codec_set_adc_hbf_bypass_cnt((AUD_CHANNEL_MAP_CH2 | AUD_CHANNEL_MAP_CH3), codec_adc_sample_rate[i].bypass_cnt);
#endif

        // Mute DAC when cpu fault occurs
        hal_cmu_codec_set_fault_mask(0x3F);
        codec->REG_054 |= CODEC_FAULT_MUTE_DAC_ENABLE;

#ifdef CODEC_TIMER
        // Disable sync stamp auto clear to avoid impacting codec timer capture
        codec->REG_054 &= ~CODEC_STAMP_CLR_USED;
#else
        // Enable sync stamp auto clear
        codec->REG_054 |= CODEC_STAMP_CLR_USED;
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
#ifdef ANC_APP
    open_type &= ~type;
    if (open_type) {
        return 0;
    }
#endif

    codec->REG_000 &= ~CODEC_CODEC_IF_EN;
    codec->REG_064 &= ~(CODEC_SOFT_RSTN_32K | CODEC_SOFT_RSTN_IIR | CODEC_SOFT_RSTN_RS |
        CODEC_SOFT_RSTN_DAC | CODEC_SOFT_RSTN_ADC_MASK | CODEC_SOFT_RSTN_ADC_ANA_MASK);
    codec->REG_060 &= ~(CODEC_EN_CLK_DAC | CODEC_EN_CLK_ADC_MASK | CODEC_EN_CLK_ADC_ANA_MASK | CODEC_POL_ADC_ANA_MASK);

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
    codec->REG_0C0 = (codec->REG_0C0 & ~(CODEC_CODEC_DRE_THD_DB_OFFSET_SIGN_CH0 | CODEC_CODEC_DRE_DELAY_CH0_MASK |
            CODEC_CODEC_DRE_INI_ANA_GAIN_CH0_MASK | CODEC_CODEC_DRE_THD_DB_OFFSET_CH0_MASK | CODEC_CODEC_DRE_STEP_MODE_CH0_MASK)) |
        DRE_DAC_DRE_DELAY(dac_dre_cfg.dre_delay) |
        CODEC_CODEC_DRE_INI_ANA_GAIN_CH0(0xF) | CODEC_CODEC_DRE_THD_DB_OFFSET_CH0(dac_dre_cfg.thd_db_offset) |
        CODEC_CODEC_DRE_THD_DB_OFFSET_CH0(dac_dre_cfg.step_mode) | CODEC_CODEC_DRE_ENABLE_CH0;
    codec->REG_0C4 = (codec->REG_0C4 & ~(CODEC_CODEC_DRE_WINDOW_CH0_MASK | CODEC_CODEC_DRE_AMP_HIGH_CH0_MASK)) |
        CODEC_CODEC_DRE_WINDOW_CH0(dac_dre_cfg.dre_win) | CODEC_CODEC_DRE_AMP_HIGH_CH0(dac_dre_cfg.amp_high);

    codec->REG_0C8 = (codec->REG_0C8 & ~(CODEC_CODEC_DRE_THD_DB_OFFSET_SIGN_CH1 | CODEC_CODEC_DRE_DELAY_CH1_MASK |
            CODEC_CODEC_DRE_INI_ANA_GAIN_CH1_MASK | CODEC_CODEC_DRE_THD_DB_OFFSET_CH1_MASK | CODEC_CODEC_DRE_STEP_MODE_CH1_MASK)) |
        DRE_DAC_DRE_DELAY(dac_dre_cfg.dre_delay) |
        CODEC_CODEC_DRE_INI_ANA_GAIN_CH1(0xF) | CODEC_CODEC_DRE_THD_DB_OFFSET_CH1(dac_dre_cfg.thd_db_offset) |
        CODEC_CODEC_DRE_THD_DB_OFFSET_CH1(dac_dre_cfg.step_mode) | CODEC_CODEC_DRE_ENABLE_CH1;
    codec->REG_0CC = (codec->REG_0CC & ~(CODEC_CODEC_DRE_WINDOW_CH1_MASK | CODEC_CODEC_DRE_AMP_HIGH_CH1_MASK)) |
        CODEC_CODEC_DRE_WINDOW_CH1(dac_dre_cfg.dre_win) | CODEC_CODEC_DRE_AMP_HIGH_CH1(dac_dre_cfg.amp_high);
}

static void hal_codec_dac_dre_disable(void)
{
    codec->REG_0C0 &= ~CODEC_CODEC_DRE_ENABLE_CH0;
    codec->REG_0C8 &= ~CODEC_CODEC_DRE_ENABLE_CH1;
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

    if (codec->REG_0C0 & CODEC_CODEC_DRE_ENABLE_CH0) {
        dac_vol = nominal_vol;
    } else {
        codec->REG_0C0 = SET_BITFIELD(codec->REG_0C0, CODEC_CODEC_DRE_INI_ANA_GAIN_CH0, ini_ana_gain);
        codec->REG_0C8 = SET_BITFIELD(codec->REG_0C8, CODEC_CODEC_DRE_INI_ANA_GAIN_CH1, ini_ana_gain);
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

    if (!codec_opened || (codec->REG_098 & CODEC_CODEC_DAC_EN) == 0) {
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
        ini_ana_gain = GET_BITFIELD(codec->REG_0C0, CODEC_CODEC_DRE_INI_ANA_GAIN_CH0);
        digdac_gain_offset_nr = ((0xF - ini_ana_gain) * 11 + 0xF / 2) / 0xF;
        ini_ana_gain = 0xF;
    } else {
        ini_ana_gain = 0xF - (digdac_gain_offset_nr * 0xF + 11 / 2) / 11;
        digdac_gain_offset_nr = 0;
    }

    codec->REG_0C0 = SET_BITFIELD(codec->REG_0C0, CODEC_CODEC_DRE_INI_ANA_GAIN_CH0, ini_ana_gain);
    codec->REG_0C8 = SET_BITFIELD(codec->REG_0C8, CODEC_CODEC_DRE_INI_ANA_GAIN_CH1, ini_ana_gain);

#ifdef AUDIO_OUTPUT_SW_GAIN
    hal_codec_set_sw_gain(digdac_gain[0]);
#else
    hal_codec_restore_dig_dac_gain();
#endif
}
#endif

void hal_codec_stop_playback_stream(enum HAL_CODEC_ID_T id)
{
#if (defined(AUDIO_OUTPUT_DC_CALIB_ANA) || defined(AUDIO_OUTPUT_DC_CALIB)) && (!(defined(__TWS__) || defined(IBRT)) || defined(ANC_APP))
    // Disable PA
    analog_aud_codec_speaker_enable(false);
#endif

    codec->REG_098 &= ~(CODEC_CODEC_DAC_EN | CODEC_CODEC_DAC_EN_CH0 | CODEC_CODEC_DAC_EN_CH1);

#ifdef CODEC_TIMER
    // Reset codec timer
    codec->REG_054 &= ~CODEC_EVENT_FOR_CAPTURE;
#endif

#ifdef DAC_DRE_ENABLE
    hal_codec_dac_dre_disable();
#endif

#if defined(DAC_CLASSG_ENABLE)
    hal_codec_classg_enable(false);
#endif

#ifndef NO_DAC_RESET
    // Reset DAC
    // Avoid DAC outputing noise after it is disabled
    codec->REG_064 &= ~CODEC_SOFT_RSTN_DAC;
    codec->REG_064 |= CODEC_SOFT_RSTN_DAC;
#endif
}

void hal_codec_start_playback_stream(enum HAL_CODEC_ID_T id)
{
#ifndef NO_DAC_RESET
    // Reset DAC
    codec->REG_064 &= ~CODEC_SOFT_RSTN_DAC;
    codec->REG_064 |= CODEC_SOFT_RSTN_DAC;
#endif

#ifdef DAC_DRE_ENABLE
    if (
            //(codec->REG_044 & CODEC_MODE_16BIT_DAC) == 0 &&
#ifdef ANC_APP
            anc_adc_ch_map == 0 &&
#endif
            1
            )
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

#ifdef CODEC_TIMER
    // Enable codec timer and record time by bt event instead of gpio event
    codec->REG_054 = (codec->REG_054 & ~CODEC_EVENT_SEL) | CODEC_EVENT_FOR_CAPTURE;
#endif

    if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH0) {
        codec->REG_098 |= CODEC_CODEC_DAC_EN_CH0;
    } else {
        codec->REG_098 &= ~CODEC_CODEC_DAC_EN_CH0;
    }
    if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH1) {
        codec->REG_098 |= CODEC_CODEC_DAC_EN_CH1;
    } else {
        codec->REG_098 &= ~CODEC_CODEC_DAC_EN_CH1;
    }

#if (defined(AUDIO_OUTPUT_DC_CALIB_ANA) || defined(AUDIO_OUTPUT_DC_CALIB)) && (!(defined(__TWS__) || defined(IBRT)) || defined(ANC_APP))
#if 0
    uint32_t cfg_en;
    uint32_t anc_ff_gain, anc_fb_gain;

    cfg_en = codec->REG_000 & CODEC_DAC_ENABLE;
    anc_ff_gain = codec->REG_0D4;
    anc_fb_gain = codec->REG_0D8;

    if (cfg_en) {
        codec->REG_000 &= ~cfg_en;
    }
    if (anc_ff_gain) {
        codec->REG_0D4 = CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FF_CH0 | CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FF_CH1;
        anc_ff_gain |= CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FF_CH0 | CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FF_CH1;
    }
    if (anc_fb_gain) {
        codec->REG_0D8 = CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FB_CH0 | CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FB_CH1;
        anc_fb_gain = CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FB_CH0 | CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FB_CH1;
    }
    osDelay(1);
#endif
#endif

    codec->REG_098 |= CODEC_CODEC_DAC_EN;

#if (defined(AUDIO_OUTPUT_DC_CALIB_ANA) || defined(AUDIO_OUTPUT_DC_CALIB)) && (!(defined(__TWS__) || defined(IBRT)) || defined(ANC_APP))
#ifdef AUDIO_OUTPUT_DC_CALIB
    // At least delay 4ms for 8K-sample-rate mute data to arrive at DAC PA
    osDelay(5);
#endif

#if 0
    if (cfg_en) {
        codec->REG_000 |= cfg_en;
    }
    if (anc_ff_gain) {
        codec->REG_0D4 = anc_ff_gain;
    }
    if (anc_fb_gain) {
        codec->REG_0D8 = anc_fb_gain;
    }
#endif

    // Enable PA
    analog_aud_codec_speaker_enable(true);

#ifdef AUDIO_ANC_FB_MC
    if (mc_enabled) {
        uint32_t lock;
        lock = int_lock();
        // MC FIFO and DAC FIFO must be started at the same time
        codec->REG_04C |= CODEC_MC_ENABLE;
        codec->REG_000 |= CODEC_DAC_ENABLE;
        int_unlock(lock);
    }
#endif
#endif
}

int hal_codec_start_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream)
{
    if (stream == AUD_STREAM_PLAYBACK) {
        // Reset and start DAC
        hal_codec_start_playback_stream(id);
    } else {
#if defined(AUDIO_ANC_FB_MC) || defined(CODEC_DSD)
        adc_en_map |= CODEC_ADC_EN_REQ_STREAM;
        if (adc_en_map == CODEC_ADC_EN_REQ_STREAM)
#endif
        {
            // Reset ADC free running clock and ADC ANA
            codec->REG_064 &= ~(RSTN_ADC_FREE_RUNNING_CLK | CODEC_SOFT_RSTN_ADC_ANA_MASK);
            codec->REG_064 |= (RSTN_ADC_FREE_RUNNING_CLK | CODEC_SOFT_RSTN_ADC_ANA_MASK);
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
#if defined(AUDIO_ANC_FB_MC) || defined(CODEC_DSD)
        adc_en_map &= ~CODEC_ADC_EN_REQ_STREAM;
        if (adc_en_map == 0)
#endif
        {
            codec->REG_080 &= ~CODEC_CODEC_ADC_EN;
        }
    }

    return 0;
}

#ifdef CODEC_DSD
void hal_codec_dsd_enable(void)
{
    dsd_enabled = true;
}

void hal_codec_dsd_disable(void)
{
    dsd_enabled = false;
}

static void hal_codec_dsd_cfg_start(void)
{
    hal_cmu_dma_dsd_enable();

    codec->REG_004 |= CODEC_DSD_RX_FIFO_FLUSH | CODEC_DSD_TX_FIFO_FLUSH;
    hal_codec_reg_update_delay();
    codec->REG_004 &= ~(CODEC_DSD_RX_FIFO_FLUSH | CODEC_DSD_TX_FIFO_FLUSH);

    codec->REG_0B8 = CODEC_CODEC_DSD_ENABLE_L | CODEC_CODEC_DSD_ENABLE_R | CODEC_CODEC_DSD_SAMPLE_RATE(dsd_rate_idx);
    codec->REG_048 = CODEC_DSD_IF_EN | CODEC_DSD_ENABLE | CODEC_DSD_DUAL_CHANNEL | CODEC_MODE_24BIT_DSD |
        /* CODEC_DMA_CTRL_RX_DSD | */ CODEC_DMA_CTRL_TX_DSD | CODEC_DSD_IN_16BIT;

    codec->REG_080 = (codec->REG_080 & ~(CODEC_CODEC_LOOP_SEL_L_MASK | CODEC_CODEC_LOOP_SEL_R_MASK)) |
        CODEC_CODEC_ADC_LOOP | CODEC_CODEC_LOOP_SEL_L(2) | CODEC_CODEC_LOOP_SEL_R(3);

    codec->REG_0A8 = SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH2, 2);
    codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH2;
    codec->REG_0A8 = SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH3, 3);
    codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH3;

    codec->REG_064 &= ~(CODEC_SOFT_RSTN_ADC(1 << 2) | CODEC_SOFT_RSTN_ADC(1 << 3));
    codec->REG_064 |= CODEC_SOFT_RSTN_ADC(1 << 2) | CODEC_SOFT_RSTN_ADC(1 << 3);
    codec->REG_080 |= CODEC_CODEC_ADC_EN_CH2 | CODEC_CODEC_ADC_EN_CH3;

    if (adc_en_map == 0) {
        // Reset ADC free running clock and ADC ANA
        codec->REG_064 &= ~(RSTN_ADC_FREE_RUNNING_CLK | CODEC_SOFT_RSTN_ADC_ANA_MASK);
        codec->REG_064 |= (RSTN_ADC_FREE_RUNNING_CLK | CODEC_SOFT_RSTN_ADC_ANA_MASK);
        codec->REG_080 |= CODEC_CODEC_ADC_EN;
    }
    adc_en_map |= CODEC_ADC_EN_REQ_DSD;
}

static void hal_codec_dsd_cfg_stop(void)
{
    adc_en_map &= ~CODEC_ADC_EN_REQ_DSD;
    if (adc_en_map == 0) {
        codec->REG_080 &= ~CODEC_CODEC_ADC_EN;
    }

    codec->REG_080 &= ~(CODEC_CODEC_ADC_EN_CH2 | CODEC_CODEC_ADC_EN_CH3);
    codec->REG_0A4 &= ~(CODEC_CODEC_PDM_ADC_SEL_CH2 | CODEC_CODEC_PDM_ADC_SEL_CH3);
    codec->REG_048 = 0;
    codec->REG_0B8 = 0;

    codec->REG_080 &= ~CODEC_CODEC_ADC_LOOP;

    hal_cmu_dma_dsd_disable();
}
#endif

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
        if (codec_dac_sample_rate[d].sample_rate < AUD_SAMPRATE_44100) {
            dac_factor = (6 / codec_dac_sample_rate[d].dac_up) * (codec_dac_sample_rate[d].bypass_cnt + 1);
        } else {
            // SINC to 48K/44.1K
            dac_factor = (6 / 1) * (0 + 1);
        }
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
            } else if (val == 2) {
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

    uint32_t ec_mask, ec_val;

    ec_mask = 0;
    ec_val = 0;
    if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH5) {
        ec_mask |= CODEC_CODEC_DOWN_SEL_MC_CH0_MASK;
        ec_val |= CODEC_CODEC_DOWN_SEL_MC_CH0(sel);
    }
    if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH6) {
        ec_mask |= CODEC_CODEC_DOWN_SEL_MC_CH1_MASK;
        ec_val |= CODEC_CODEC_DOWN_SEL_MC_CH1(sel);
    }
    codec->REG_228 = (codec->REG_228 & ~ec_mask) | ec_val;
}

static void hal_codec_ec_enable(void)
{
    uint32_t ec_val;
    bool dac_rate_valid;
    uint8_t a;

    dac_rate_valid = !!(codec->REG_000 & CODEC_DAC_ENABLE);

    hal_codec_set_ec_down_sel(dac_rate_valid);

    // If no normal ADC chan, ADC0 must be enabled
    if ((codec_adc_ch_map & ~(AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6)) == 0) {
        a = codec_rate_idx[AUD_STREAM_CAPTURE];
        hal_codec_set_adc_down(AUD_CHANNEL_MAP_CH0, codec_adc_sample_rate[a].adc_down);
        hal_codec_set_adc_hbf_bypass_cnt(AUD_CHANNEL_MAP_CH0, codec_adc_sample_rate[a].bypass_cnt);
        codec->REG_080 |= CODEC_CODEC_ADC_EN_CH0;
    }

    ec_val = 0;
    if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH5) {
        ec_val |= CODEC_CODEC_MC_ENABLE_CH0;
    }
    if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH6) {
        ec_val |= CODEC_CODEC_MC_ENABLE_CH1;
    }
    if (codec->REG_0E4 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) {
        ec_val |= CODEC_CODEC_RESAMPLE_MC_ENABLE;
        if ((codec_adc_ch_map & (AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6)) ==
                (AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6)) {
            ec_val |= CODEC_CODEC_RESAMPLE_MC_DUAL_CH;
        }
    }
    codec->REG_228 |= ec_val;
}

static void hal_codec_ec_disable(void)
{
    codec->REG_228 &= ~(CODEC_CODEC_MC_ENABLE_CH0 | CODEC_CODEC_MC_ENABLE_CH1 |
        CODEC_CODEC_RESAMPLE_MC_ENABLE | CODEC_CODEC_RESAMPLE_MC_DUAL_CH);
    if ((codec_adc_ch_map & ~(AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6)) == 0) {
        codec->REG_080 &= ~CODEC_CODEC_ADC_EN_CH0;
    }
}

int hal_codec_start_interface(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream, int dma)
{
    uint32_t fifo_flush = 0;

    if (stream == AUD_STREAM_PLAYBACK) {
#ifdef CODEC_DSD
        if (dsd_enabled) {
            hal_codec_dsd_cfg_start();
        }
#endif
#ifdef __AUDIO_RESAMPLE__
        if (codec->REG_0E4 & CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE) {
#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
            codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
            hal_codec_reg_update_delay();
            codec->REG_0F4 = resample_phase_float_to_value(get_playback_resample_phase());
            hal_codec_reg_update_delay();
            codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
#endif
            codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_DAC_ENABLE;
            hal_codec_resample_clock_enable(stream);
        }
#endif
        if ((codec->REG_000 & CODEC_ADC_ENABLE) && (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6))) {
            hal_codec_set_ec_down_sel(true);
        }
#ifdef AUDIO_ANC_FB_MC
        fifo_flush |= CODEC_MC_FIFO_FLUSH;
#endif
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
#ifdef AUDIO_ANC_FB_MC
        if (mc_dual_chan) {
            codec->REG_04C |= CODEC_DUAL_CHANNEL_MC;
        } else {
            codec->REG_04C &= ~CODEC_DUAL_CHANNEL_MC;
        }
        if (mc_16bit) {
            codec->REG_04C |= CODEC_MODE_16BIT_MC;
        } else {
            codec->REG_04C &= ~CODEC_MODE_16BIT_MC;
        }
        if (adc_en_map == 0) {
            // Reset ADC free running clock and ADC ANA
            codec->REG_064 &= ~(RSTN_ADC_FREE_RUNNING_CLK | CODEC_SOFT_RSTN_ADC_ANA_MASK);
            codec->REG_064 |= (RSTN_ADC_FREE_RUNNING_CLK | CODEC_SOFT_RSTN_ADC_ANA_MASK);
            codec->REG_080 |= CODEC_CODEC_ADC_EN;
        }
        adc_en_map |= CODEC_ADC_EN_REQ_MC;
        // If codec function has been enabled, start FIFOs directly;
        // otherwise, start FIFOs after PA is enabled
        if (codec->REG_098 & CODEC_CODEC_DAC_EN) {
            uint32_t lock;
            lock = int_lock();
            // MC FIFO and DAC FIFO must be started at the same time
            codec->REG_04C |= CODEC_MC_ENABLE;
            codec->REG_000 |= CODEC_DAC_ENABLE;
            int_unlock(lock);
        }
        mc_enabled = true;
#else
        codec->REG_000 |= CODEC_DAC_ENABLE;
#endif
    } else {
#ifdef __AUDIO_RESAMPLE__
        if (codec->REG_0E4 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) {
#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
            codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
            hal_codec_reg_update_delay();
            codec->REG_0F8 = resample_phase_float_to_value(get_capture_resample_phase());
            hal_codec_reg_update_delay();
            codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
#endif
            codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_ADC_ENABLE;
            hal_codec_resample_clock_enable(stream);
        }
#endif
        if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6)) {
            hal_codec_ec_enable();
        }
        for (int i = 0; i < MAX_ADC_CH_NUM; i++) {
            if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
                if (i < NORMAL_ADC_CH_NUM &&
                        (codec->REG_080 & (CODEC_CODEC_ADC_EN_CH0 << i)) == 0) {
                    // Reset ADC channel
                    codec->REG_064 &= ~CODEC_SOFT_RSTN_ADC(1 << i);
                    codec->REG_064 |= CODEC_SOFT_RSTN_ADC(1 << i);
                    codec->REG_080 |= (CODEC_CODEC_ADC_EN_CH0 << i);
                }
                codec->REG_000 |= (CODEC_ADC_ENABLE_CH0 << i);
            }
        }
        fifo_flush = CODEC_RX_FIFO_FLUSH_CH0 | CODEC_RX_FIFO_FLUSH_CH1 | CODEC_RX_FIFO_FLUSH_CH2 |
            CODEC_RX_FIFO_FLUSH_CH3 | CODEC_RX_FIFO_FLUSH_CH4 | CODEC_RX_FIFO_FLUSH_CH5 | CODEC_RX_FIFO_FLUSH_CH6;
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
        codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_DAC_ENABLE;
        hal_codec_resample_clock_disable(stream);
#endif
#ifdef CODEC_DSD
        hal_codec_dsd_cfg_stop();
        dsd_enabled = false;
#endif
#ifdef AUDIO_ANC_FB_MC
        mc_enabled = false;
        codec->REG_04C &= ~CODEC_MC_ENABLE;
        adc_en_map &= ~CODEC_ADC_EN_REQ_MC;
        if (adc_en_map == 0) {
            codec->REG_080 &= ~CODEC_CODEC_ADC_EN;
        }
        fifo_flush |= CODEC_MC_FIFO_FLUSH;
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
        codec->REG_000 &= ~(CODEC_ADC_ENABLE | CODEC_ADC_ENABLE_CH0 | CODEC_ADC_ENABLE_CH1 | CODEC_ADC_ENABLE_CH2 |
            CODEC_ADC_ENABLE_CH3 | CODEC_ADC_ENABLE_CH4 | CODEC_ADC_ENABLE_CH5 | CODEC_ADC_ENABLE_CH6);
        codec->REG_000 &= ~CODEC_DMACTRL_RX;
        for (int i = 0; i < MAX_ADC_CH_NUM; i++) {
            if (i < NORMAL_ADC_CH_NUM &&
                    (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) &&
                    (anc_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) == 0) {
                codec->REG_080 &= ~(CODEC_CODEC_ADC_EN_CH0 << i);
            }
        }
        hal_codec_ec_disable();
#ifdef __AUDIO_RESAMPLE__
        codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_ADC_ENABLE;
        hal_codec_resample_clock_disable(stream);
#endif
        fifo_flush = CODEC_RX_FIFO_FLUSH_CH0 | CODEC_RX_FIFO_FLUSH_CH1 | CODEC_RX_FIFO_FLUSH_CH2 |
            CODEC_RX_FIFO_FLUSH_CH3 | CODEC_RX_FIFO_FLUSH_CH4 | CODEC_RX_FIFO_FLUSH_CH5 | CODEC_RX_FIFO_FLUSH_CH6;
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
    codec->REG_09C &= ~CODEC_CODEC_DAC_GAIN_UPDATE;
    hal_codec_reg_update_delay();
    if (map & AUD_CHANNEL_MAP_CH0) {
        codec->REG_09C = SET_BITFIELD(codec->REG_09C, CODEC_CODEC_DAC_GAIN_CH0, val);
    }
    if (map & AUD_CHANNEL_MAP_CH1) {
        codec->REG_0A0 = SET_BITFIELD(codec->REG_0A0, CODEC_CODEC_DAC_GAIN_CH1, val);
    }
    codec->REG_09C |= CODEC_CODEC_DAC_GAIN_UPDATE;
}

void hal_codec_get_dac_gain(float *left_gain,float *right_gain)
{
    uint32_t left_val;
    uint32_t right_val;

    left_val  = GET_BITFIELD(codec->REG_09C, CODEC_CODEC_DAC_GAIN_CH0);
    right_val = GET_BITFIELD(codec->REG_0A0, CODEC_CODEC_DAC_GAIN_CH1);

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

#ifdef ANC_APP
    gain += digdac_gain_offset_anc;
#endif
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
    if (codec->REG_098 & CODEC_CODEC_DAC_SDM_CLOSE) {
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

    for (int i = 0; i < NORMAL_ADC_CH_NUM; i++) {
        if (map & (AUD_CHANNEL_MAP_CH0 << i)) {
            *(&codec->REG_084 + i) = SET_BITFIELD(*(&codec->REG_084 + i), CODEC_CODEC_ADC_GAIN_CH0, val);
            gain_update |= (CODEC_CODEC_ADC_GAIN_UPDATE_CH0 << i);
        }
    }
    codec->REG_09C &= ~gain_update;
    hal_codec_reg_update_delay();
    codec->REG_09C |= gain_update;
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
            map = ch_map & ~EC_MIC_MAP;
            while (map) {
                mic_ch = get_lsb_pos(map);
                map &= ~(1 << mic_ch);
                adc_ch = hal_codec_get_adc_chan(1 << mic_ch);
                ASSERT(adc_ch < NORMAL_ADC_CH_NUM, "%s: Bad cap ch_map=0x%X (ch=%u)", __func__, ch_map, mic_ch);
                hal_codec_set_dig_adc_gain((1 << adc_ch), *adc_gain_db);
#ifdef SIDETONE_ENABLE
                if ((1 << mic_ch) == CFG_HW_AUD_SIDETONE_MIC_DEV) {
                    sidetone_adc_gain = *adc_gain_db;
                    hal_codec_set_dig_adc_gain(sidetone_adc_ch_map, sidetone_adc_gain + sidetone_gain_offset);
                }
#endif
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
    //codec->REG_098 = SET_BITFIELD(codec->REG_098, CODEC_CODEC_DAC_OSR_SEL, 2);

    codec->REG_098 = (codec->REG_098 & ~bypass_mask) | bypass;
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
    codec->REG_098 = SET_BITFIELD(codec->REG_098, CODEC_CODEC_DAC_UP_SEL, sel);
    return 0;
}

static uint32_t POSSIBLY_UNUSED hal_codec_get_dac_up(void)
{
    uint32_t sel;

    sel = GET_BITFIELD(codec->REG_098, CODEC_CODEC_DAC_UP_SEL);
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
    for (int i = 0; i < NORMAL_ADC_CH_NUM; i++) {
        if (map & (AUD_CHANNEL_MAP_CH0 << i)) {
            *(&codec->REG_084 + i) = SET_BITFIELD(*(&codec->REG_084 + i), CODEC_CODEC_ADC_DOWN_SEL_CH0, sel);
        }
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
    for (int i = 0; i < NORMAL_ADC_CH_NUM; i++) {
        if (map & (AUD_CHANNEL_MAP_CH0 << i)) {
            *(&codec->REG_084 + i) = (*(&codec->REG_084 + i) & ~bypass_mask) | bypass;
        }
    }
    return 0;
}

static void hal_codec_enable_dig_mic(enum AUD_CHANNEL_MAP_T mic_map)
{
    uint32_t phase = 0;
    uint32_t line_map = 0;

    phase = codec->REG_0A8;
    for (int i = 0; i < MAX_DIG_MIC_CH_NUM; i++) {
        if (mic_map & (AUD_CHANNEL_MAP_DIGMIC_CH0 << i)) {
            line_map |= (1 << (i / 2));
        }
        phase = (phase & ~(CODEC_CODEC_PDM_CAP_PHASE_CH0_MASK << (i * 2))) |
            (CODEC_CODEC_PDM_CAP_PHASE_CH0(codec_digmic_phase) << (i * 2));
    }
    codec->REG_0A8 = phase;
    codec->REG_0A4 |= CODEC_CODEC_PDM_ENABLE;
    hal_iomux_set_dig_mic(line_map);
}

static void hal_codec_disable_dig_mic(void)
{
    codec->REG_0A4 &= ~CODEC_CODEC_PDM_ENABLE;
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
    enum AUD_SAMPRATE_T sample_rate;

    if (stream == AUD_STREAM_PLAYBACK) {
        if ((HAL_CODEC_CONFIG_CHANNEL_MAP | HAL_CODEC_CONFIG_CHANNEL_NUM) & cfg->set_flag) {
            if (cfg->channel_num == AUD_CHANNEL_NUM_2) {
                if (cfg->channel_map != (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)) {
                    TRACE("\n!!! WARNING:%s: Bad play stereo ch map: 0x%X\n", __func__, cfg->channel_map);
                }
                codec->REG_044 |= CODEC_DUAL_CHANNEL_DAC;
            } else {
                ASSERT(cfg->channel_num == AUD_CHANNEL_NUM_1, "%s: Bad play ch num: %u", __func__, cfg->channel_num);
                // Allow to DMA one channel but output 2 channels
                ASSERT((cfg->channel_map & ~(AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)) == 0,
                    "%s: Bad play mono ch map: 0x%X", __func__, cfg->channel_map);
                codec->REG_044 &= ~CODEC_DUAL_CHANNEL_DAC;
            }
            codec_dac_ch_map = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1;
        }

        if (HAL_CODEC_CONFIG_BITS & cfg->set_flag) {
            if (cfg->bits == AUD_BITS_16) {
                codec->REG_044 = (codec->REG_044 & ~CODEC_MODE_32BIT_DAC) | CODEC_MODE_16BIT_DAC;
                codec->REG_04C = (codec->REG_04C & ~CODEC_MODE_32BIT_MC) | CODEC_MODE_16BIT_MC;
            } else if (cfg->bits == AUD_BITS_24) {
                codec->REG_044 &= ~(CODEC_MODE_16BIT_DAC | CODEC_MODE_32BIT_DAC);
                codec->REG_04C &= ~(CODEC_MODE_16BIT_MC | CODEC_MODE_32BIT_MC);
            } else if (cfg->bits == AUD_BITS_32) {
                codec->REG_044 = (codec->REG_044 & ~CODEC_MODE_16BIT_DAC) | CODEC_MODE_32BIT_DAC;
                codec->REG_04C = (codec->REG_04C & ~CODEC_MODE_16BIT_MC) | CODEC_MODE_32BIT_MC;
            } else {
                ASSERT(false, "%s: Bad play bits: %u", __func__, cfg->bits);
            }
        }

        if (HAL_CODEC_CONFIG_SAMPLE_RATE & cfg->set_flag) {
            sample_rate = cfg->sample_rate;
#ifdef CODEC_DSD
            if (dsd_enabled) {
                if (sample_rate == AUD_SAMPRATE_176400) {
                    dsd_rate_idx = 0;
                } else if (sample_rate == AUD_SAMPRATE_352800) {
                    dsd_rate_idx = 1;
                } else if (sample_rate == AUD_SAMPRATE_705600) {
                    dsd_rate_idx = 2;
                } else {
                    ASSERT(false, "%s: Bad DSD sample rate: %u", __func__, sample_rate);
                }
                sample_rate = AUD_SAMPRATE_44100;
            }
#endif

            for (i = 0; i < ARRAY_SIZE(codec_dac_sample_rate); i++) {
                if (codec_dac_sample_rate[i].sample_rate == sample_rate) {
                    break;
                }
            }
            ASSERT(i < ARRAY_SIZE(codec_dac_sample_rate), "%s: Invalid playback sample rate: %u", __func__, sample_rate);
            rate_idx = i;
            ana_dig_div = codec_dac_sample_rate[rate_idx].codec_div / codec_dac_sample_rate[rate_idx].cmu_div;
            ASSERT(ana_dig_div * codec_dac_sample_rate[rate_idx].cmu_div == codec_dac_sample_rate[rate_idx].codec_div,
                "%s: Invalid playback div for rate %u: codec_div=%u cmu_div=%u", __func__, sample_rate,
                codec_dac_sample_rate[rate_idx].codec_div, codec_dac_sample_rate[rate_idx].cmu_div);

            TRACE("[%s] playback sample_rate=%d", __func__, sample_rate);

#ifdef CODEC_TIMER
            cur_codec_freq = codec_dac_sample_rate[rate_idx].codec_freq;
#endif

            codec_rate_idx[AUD_STREAM_PLAYBACK] = rate_idx;

#ifdef __AUDIO_RESAMPLE__
            uint32_t mask, val;

            mask = CODEC_CODEC_RESAMPLE_DAC_L_ENABLE | CODEC_CODEC_RESAMPLE_DAC_R_ENABLE |
                CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;

            if (hal_cmu_get_audio_resample_status() && codec_dac_sample_rate[rate_idx].codec_freq != CODEC_FREQ_CRYSTAL) {
#ifdef CODEC_TIMER
                cur_codec_freq = CODEC_FREQ_CRYSTAL;
#endif
                bool update_factor = false;

#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
                if ((codec->REG_000 & CODEC_DAC_ENABLE) == 0) {
                    update_factor = true;
                }
#else
                if ((codec->REG_0E4 & CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE) == 0 ||
                        resample_rate_idx[AUD_STREAM_PLAYBACK] != rate_idx) {
                    update_factor = true;
                }
#endif
                if (update_factor) {
                    resample_rate_idx[AUD_STREAM_PLAYBACK] = rate_idx;
                    codec->REG_0E4 &= ~mask;
                    hal_codec_reg_update_delay();
#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
                    enum HAL_CODEC_SYNC_TYPE_T sync_type;

                    sync_type = GET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_DAC_TRIGGER_SEL);
                    if (sync_type != HAL_CODEC_SYNC_TYPE_NONE) {
                        codec->REG_0E4 = SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_DAC_TRIGGER_SEL, HAL_CODEC_SYNC_TYPE_NONE);
                        hal_codec_reg_update_delay();
                    }
                    codec->REG_0F4 = resample_phase_float_to_value(1.0f);
#else
                    codec->REG_0F4 = resample_phase_float_to_value(get_playback_resample_phase());
#endif
                    hal_codec_reg_update_delay();
                    codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
                    if (sync_type != HAL_CODEC_SYNC_TYPE_NONE) {
                        hal_codec_reg_update_delay();
                        codec->REG_0E4 = SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_DAC_TRIGGER_SEL, sync_type);
                    }
#endif
                    val = 0;
                    if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH0) {
                        val |= CODEC_CODEC_RESAMPLE_DAC_L_ENABLE;
                    }
                    if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH1) {
                        val |= CODEC_CODEC_RESAMPLE_DAC_R_ENABLE;
                    }
                    codec->REG_0E4 |= val;
                }
            } else {
                codec->REG_0E4 &= ~mask;
            }
#endif

            // 8K -> 4ms, 16K -> 2ms, ...
            dac_delay_ms = 4 / ((sample_rate + AUD_SAMPRATE_8000 / 2) / AUD_SAMPRATE_8000);
            if (dac_delay_ms < 2) {
                dac_delay_ms = 2;
            }

#ifdef __AUDIO_RESAMPLE__
            if (!hal_cmu_get_audio_resample_status())
#endif
            {
#ifdef __AUDIO_RESAMPLE__
                ASSERT(codec_dac_sample_rate[rate_idx].codec_freq != CODEC_FREQ_CRYSTAL,
                    "%s: playback sample rate %u is for resample only", __func__, sample_rate);
#endif
                analog_aud_freq_pll_config(codec_dac_sample_rate[rate_idx].codec_freq, codec_dac_sample_rate[rate_idx].codec_div);
                hal_cmu_codec_dac_set_div(codec_dac_sample_rate[rate_idx].cmu_div);
            }
            hal_codec_set_dac_up(codec_dac_sample_rate[rate_idx].dac_up);
            hal_codec_set_dac_hbf_bypass_cnt(codec_dac_sample_rate[rate_idx].bypass_cnt);
#ifdef AUDIO_ANC_FB_MC
            codec->REG_04C = SET_BITFIELD(codec->REG_04C, CODEC_MC_DELAY, codec_dac_sample_rate[rate_idx].mc_delay);
#endif
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
        enum AUD_CHANNEL_MAP_T reserv_map;
        uint8_t cnt;
        uint8_t ch_idx;
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
            } else if ((anc_mic_ch_map & AUD_CHANNEL_MAP_DIGMIC_ALL) == 0) {
                hal_codec_disable_dig_mic();
            }

            reserv_map = 0;

#ifdef ANC_APP
#if defined(ANC_FF_MIC_CH_L) || defined(ANC_FF_MIC_CH_R)
#ifdef ANC_PROD_TEST
            if ((ANC_FF_MIC_CH_L & ~NORMAL_MIC_MAP) || (ANC_FF_MIC_CH_L & (ANC_FF_MIC_CH_L - 1))) {
                ASSERT(false, "Invalid ANC_FF_MIC_CH_L: 0x%04X", ANC_FF_MIC_CH_L);
            }
            if ((ANC_FF_MIC_CH_R & ~NORMAL_MIC_MAP) || (ANC_FF_MIC_CH_R & (ANC_FF_MIC_CH_R - 1))) {
                ASSERT(false, "Invalid ANC_FF_MIC_CH_R: 0x%04X", ANC_FF_MIC_CH_R);
            }
            if (ANC_FF_MIC_CH_L & ANC_FF_MIC_CH_R) {
                ASSERT(false, "Conflicted ANC_FF_MIC_CH_L (0x%04X) and ANC_FF_MIC_CH_R (0x%04X)", ANC_FF_MIC_CH_L, ANC_FF_MIC_CH_R);
            }
#if defined(ANC_FB_MIC_CH_L) || defined(ANC_FB_MIC_CH_R)
            if ((ANC_FF_MIC_CH_L & ANC_FB_MIC_CH_L) || (ANC_FF_MIC_CH_L & ANC_FB_MIC_CH_R) ||
                    (ANC_FF_MIC_CH_R & ANC_FB_MIC_CH_L) || (ANC_FF_MIC_CH_R & ANC_FB_MIC_CH_R)) {
                ASSERT(false, "Conflicted FF MIC (0x%04X/0x%04X) and FB MIC (0x%04X/0x%04X)",
                    ANC_FF_MIC_CH_L, ANC_FF_MIC_CH_R, ANC_FB_MIC_CH_L, ANC_FB_MIC_CH_R);
            }
#endif
#else // !ANC_PROD_TEST
#if (ANC_FF_MIC_CH_L & ~NORMAL_MIC_MAP) || (ANC_FF_MIC_CH_L & (ANC_FF_MIC_CH_L - 1))
#error "Invalid ANC_FF_MIC_CH_L"
#endif
#if (ANC_FF_MIC_CH_R & ~NORMAL_MIC_MAP) || (ANC_FF_MIC_CH_R & (ANC_FF_MIC_CH_R - 1))
#error "Invalid ANC_FF_MIC_CH_R"
#endif
#if (ANC_FF_MIC_CH_L & ANC_FF_MIC_CH_R)
#error "Conflicted ANC_FF_MIC_CH_L and ANC_FF_MIC_CH_R"
#endif
#if defined(ANC_FB_MIC_CH_L) || defined(ANC_FB_MIC_CH_R)
#if (ANC_FF_MIC_CH_L & ANC_FB_MIC_CH_L) || (ANC_FF_MIC_CH_L & ANC_FB_MIC_CH_R) || \
        (ANC_FF_MIC_CH_R & ANC_FB_MIC_CH_L) || (ANC_FF_MIC_CH_R & ANC_FB_MIC_CH_R)
#error "Conflicted ANC_FF_MIC_CH_L and ANC_FF_MIC_CH_R, ANC_FB_MIC_CH_L, ANC_FB_MIC_CH_R"
#endif
#endif
#endif // !ANC_PROD_TEST
            if (mic_map & ANC_FF_MIC_CH_L) {
                codec_adc_ch_map |= AUD_CHANNEL_MAP_CH0;
                mic_map &= ~ANC_FF_MIC_CH_L;
                ch_idx = get_msb_pos(ANC_FF_MIC_CH_L);
                if (ANC_FF_MIC_CH_L & AUD_CHANNEL_MAP_DIGMIC_ALL) {
                    codec->REG_0A8 = SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH0, ch_idx);
                    codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH0;
                } else {
                    codec->REG_084 = SET_BITFIELD(codec->REG_084, CODEC_CODEC_ADC_IN_SEL_CH0, ch_idx);
                    codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH0;
                }
            } else if (ANC_FF_MIC_CH_L & AUD_CHANNEL_MAP_ALL) {
                reserv_map |= AUD_CHANNEL_MAP_CH0;
            }
            if (mic_map & ANC_FF_MIC_CH_R) {
                codec_adc_ch_map |= AUD_CHANNEL_MAP_CH1;
                mic_map &= ~ANC_FF_MIC_CH_R;
                ch_idx = get_msb_pos(ANC_FF_MIC_CH_R);
                if (ANC_FF_MIC_CH_R & AUD_CHANNEL_MAP_DIGMIC_ALL) {
                    codec->REG_0A8 = SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH1, ch_idx);
                    codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH1;
                } else {
                    codec->REG_088 = SET_BITFIELD(codec->REG_088, CODEC_CODEC_ADC_IN_SEL_CH1, ch_idx);
                    codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH1;
                }
            } else if (ANC_FF_MIC_CH_R & AUD_CHANNEL_MAP_ALL) {
                reserv_map |= AUD_CHANNEL_MAP_CH1;
            }
#endif

#if defined(ANC_FB_MIC_CH_L) || defined(ANC_FB_MIC_CH_R)
#ifdef ANC_PROD_TEST
            if ((ANC_FB_MIC_CH_L & ~NORMAL_MIC_MAP) || (ANC_FB_MIC_CH_L & (ANC_FB_MIC_CH_L - 1))) {
                ASSERT(false, "Invalid ANC_FB_MIC_CH_L: 0x%04X", ANC_FB_MIC_CH_L);
            }
            if ((ANC_FB_MIC_CH_R & ~NORMAL_MIC_MAP) || (ANC_FB_MIC_CH_R & (ANC_FB_MIC_CH_R - 1))) {
                ASSERT(false, "Invalid ANC_FB_MIC_CH_R: 0x%04X", ANC_FB_MIC_CH_R);
            }
            if (ANC_FB_MIC_CH_L & ANC_FB_MIC_CH_R) {
                ASSERT(false, "Conflicted ANC_FB_MIC_CH_L (0x%04X) and ANC_FB_MIC_CH_R (0x%04X)", ANC_FB_MIC_CH_L, ANC_FB_MIC_CH_R);
            }
#else // !ANC_PROD_TEST
#if (ANC_FB_MIC_CH_L & ~NORMAL_MIC_MAP) || (ANC_FB_MIC_CH_L & (ANC_FB_MIC_CH_L - 1))
#error "Invalid ANC_FB_MIC_CH_L"
#endif
#if (ANC_FB_MIC_CH_R & ~NORMAL_MIC_MAP) || (ANC_FB_MIC_CH_R & (ANC_FB_MIC_CH_R - 1))
#error "Invalid ANC_FB_MIC_CH_R"
#endif
#if (ANC_FB_MIC_CH_L & ANC_FB_MIC_CH_R)
#error "Conflicted ANC_FB_MIC_CH_L and ANC_FB_MIC_CH_R"
#endif
#endif // !ANC_PROD_TEST
            if (mic_map & ANC_FB_MIC_CH_L) {
                codec_adc_ch_map |= AUD_CHANNEL_MAP_CH2;
                mic_map &= ~ANC_FB_MIC_CH_L;
                ch_idx = get_msb_pos(ANC_FB_MIC_CH_L);
                if (ANC_FB_MIC_CH_L & AUD_CHANNEL_MAP_DIGMIC_ALL) {
                    codec->REG_0A8 = SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH2, ch_idx);
                    codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH2;
                } else {
                    codec->REG_08C = SET_BITFIELD(codec->REG_08C, CODEC_CODEC_ADC_IN_SEL_CH2, ch_idx);
                    codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH2;
                }
            } else if (ANC_FB_MIC_CH_L & AUD_CHANNEL_MAP_ALL) {
                reserv_map |= AUD_CHANNEL_MAP_CH2;
            }
            if (mic_map & ANC_FB_MIC_CH_R) {
                codec_adc_ch_map |= AUD_CHANNEL_MAP_CH3;
                mic_map &= ~ANC_FB_MIC_CH_R;
                ch_idx = get_msb_pos(ANC_FB_MIC_CH_R);
                if (ANC_FB_MIC_CH_R & AUD_CHANNEL_MAP_DIGMIC_ALL) {
                    codec->REG_0A8 = SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH3, ch_idx);
                    codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH3;
                } else {
                    codec->REG_090 = SET_BITFIELD(codec->REG_090, CODEC_CODEC_ADC_IN_SEL_CH3, ch_idx);
                    codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH3;
                }
            } else if (ANC_FB_MIC_CH_R & AUD_CHANNEL_MAP_ALL) {
                reserv_map |= AUD_CHANNEL_MAP_CH3;
            }
#endif
#endif // ANC_APP

#ifdef CODEC_DSD
            reserv_map |= AUD_CHANNEL_MAP_CH2 | AUD_CHANNEL_MAP_CH3;
#endif

            if (mic_map & AUD_CHANNEL_MAP_CH4) {
                codec_adc_ch_map |= AUD_CHANNEL_MAP_CH4;
                mic_map &= ~AUD_CHANNEL_MAP_CH4;
                codec->REG_094 = SET_BITFIELD(codec->REG_094, CODEC_CODEC_ADC_IN_SEL_CH4, 4);
                codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH4;
            }
            if (mic_map & AUD_CHANNEL_MAP_CH5) {
                codec_adc_ch_map |= AUD_CHANNEL_MAP_CH5;
                mic_map &= ~AUD_CHANNEL_MAP_CH5;
                codec->REG_228 &= ~CODEC_CODEC_MC_SEL_CH0;
            }
            if (mic_map & AUD_CHANNEL_MAP_CH6) {
                codec_adc_ch_map |= AUD_CHANNEL_MAP_CH6;
                mic_map &= ~AUD_CHANNEL_MAP_CH6;
                codec->REG_228 |= CODEC_CODEC_MC_SEL_CH1;
            }
            reserv_map |= codec_adc_ch_map;
#ifdef SIDETONE_ENABLE
            // Alloc sidetone adc chan
            if ((reserv_map & AUD_CHANNEL_MAP_CH0) == 0) {
                sidetone_adc_ch_map = AUD_CHANNEL_MAP_CH0;
                codec->REG_080 &= ~CODEC_CODEC_SIDE_TONE_MIC_SEL;
                codec->REG_078 &= ~CODEC_CODEC_SIDE_TONE_CH_SEL;
            } else if ((reserv_map & AUD_CHANNEL_MAP_CH2) == 0) {
                sidetone_adc_ch_map = AUD_CHANNEL_MAP_CH2;
                codec->REG_080 &= ~CODEC_CODEC_SIDE_TONE_MIC_SEL;
                codec->REG_078 |= CODEC_CODEC_SIDE_TONE_CH_SEL;
            } else if ((reserv_map & AUD_CHANNEL_MAP_CH4) == 0) {
                sidetone_adc_ch_map = AUD_CHANNEL_MAP_CH4;
                codec->REG_080 |= CODEC_CODEC_SIDE_TONE_MIC_SEL;
            } else {
                ASSERT(false, "%s: Cannot allocate sidetone adc: reserv_map=0x%X", __func__, reserv_map);
            }
            // Associate mic and sidetone adc
            ch_idx = get_lsb_pos(CFG_HW_AUD_SIDETONE_MIC_DEV);
            i = get_lsb_pos(sidetone_adc_ch_map);
            if ((1 << ch_idx) & AUD_CHANNEL_MAP_DIGMIC_ALL) {
                codec->REG_0A8 = (codec->REG_0A8 & ~(CODEC_CODEC_PDM_MUX_CH0_MASK << (3 * i))) |
                    (CODEC_CODEC_PDM_MUX_CH0(ch_idx) << (3 * i));
                codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH0 << i;
            } else {
                *(&codec->REG_084 + i) = SET_BITFIELD(*(&codec->REG_084 + i), CODEC_CODEC_ADC_IN_SEL_CH0, ch_idx);
                codec->REG_0A4 &= ~(CODEC_CODEC_PDM_ADC_SEL_CH0 << i);
            }
            // Mark sidetone adc as used
            reserv_map |= sidetone_adc_ch_map;
#endif
            i = 0;
            while (mic_map && i < NORMAL_ADC_CH_NUM) {
                ASSERT(i < MAX_ANA_MIC_CH_NUM || (mic_map & AUD_CHANNEL_MAP_DIGMIC_ALL),
                    "%s: Not enough ana cap chan: mic_map=0x%X adc_map=0x%X reserv_map=0x%X",
                    __func__, mic_map, codec_adc_ch_map, reserv_map);
                ch_idx = get_lsb_pos(mic_map);
                mic_map &= ~(1 << ch_idx);
                while ((reserv_map & (AUD_CHANNEL_MAP_CH0 << i)) && i < NORMAL_ADC_CH_NUM) {
                    i++;
                }
                if (i < NORMAL_ADC_CH_NUM) {
                    codec_adc_ch_map |= (AUD_CHANNEL_MAP_CH0 << i);
                    reserv_map |= codec_adc_ch_map;
                    if ((1 << ch_idx) & AUD_CHANNEL_MAP_DIGMIC_ALL) {
                        codec->REG_0A8 = (codec->REG_0A8 & ~(CODEC_CODEC_PDM_MUX_CH0_MASK << (3 * i))) |
                            (CODEC_CODEC_PDM_MUX_CH0(ch_idx) << (3 * i));
                        codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH0 << i;
                    } else {
                        *(&codec->REG_084 + i) = SET_BITFIELD(*(&codec->REG_084 + i), CODEC_CODEC_ADC_IN_SEL_CH0, ch_idx);
                        codec->REG_0A4 &= ~(CODEC_CODEC_PDM_ADC_SEL_CH0 << i);
                    }
                    i++;
                }
            }

            ASSERT(mic_map == 0, "%s: Bad cap chan map: 0x%X", __func__, mic_map);
        }

        cfg_set_mask = 0;
        cfg_clr_mask = 0;
        if (HAL_CODEC_CONFIG_BITS & cfg->set_flag) {
            if (cfg->bits == AUD_BITS_16) {
                codec->REG_040 &= ~(CODEC_MODE_24BIT_ADC | CODEC_MODE_32BIT_ADC);
                cfg_set_mask = CODEC_MODE_16BIT_ADC_CH0;
            } else if (cfg->bits == AUD_BITS_24) {
                codec->REG_040 = (codec->REG_040 & ~CODEC_MODE_32BIT_ADC) | CODEC_MODE_24BIT_ADC;
                cfg_clr_mask = CODEC_MODE_16BIT_ADC_CH0;
            } else if (cfg->bits == AUD_BITS_32) {
                codec->REG_040 = (codec->REG_040 & ~CODEC_MODE_24BIT_ADC) | CODEC_MODE_32BIT_ADC;
                cfg_clr_mask = CODEC_MODE_16BIT_ADC_CH0;
            } else {
                ASSERT(false, "%s: Bad cap bits: %d", __func__, cfg->bits);
            }
        }

        cnt = 0;
        for (i = 0; i < MAX_ADC_CH_NUM; i++) {
            if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
                cnt++;
                codec->REG_040 |= (cfg_set_mask << i);
                codec->REG_040 &= ~(cfg_clr_mask << i);
            } else {
                codec->REG_000 &= ~(CODEC_ADC_ENABLE_CH0 << i);
            }
        }
        ASSERT(cnt == cfg->channel_num, "%s: Invalid capture stream chan cfg: map=0x%X num=%u",
            __func__, codec_adc_ch_map, cfg->channel_num);

        if (HAL_CODEC_CONFIG_SAMPLE_RATE & cfg->set_flag) {
            sample_rate = cfg->sample_rate;

            for(i = 0; i < ARRAY_SIZE(codec_adc_sample_rate); i++) {
                if(codec_adc_sample_rate[i].sample_rate == sample_rate) {
                    break;
                }
            }
            ASSERT(i < ARRAY_SIZE(codec_adc_sample_rate), "%s: Invalid capture sample rate: %d", __func__, sample_rate);
            rate_idx = i;
            ana_dig_div = codec_adc_sample_rate[rate_idx].codec_div / codec_adc_sample_rate[rate_idx].cmu_div;
            ASSERT(ana_dig_div * codec_adc_sample_rate[rate_idx].cmu_div == codec_adc_sample_rate[rate_idx].codec_div,
                "%s: Invalid catpure div for rate %u: codec_div=%u cmu_div=%u", __func__, sample_rate,
                codec_adc_sample_rate[rate_idx].codec_div, codec_adc_sample_rate[rate_idx].cmu_div);

            TRACE("[%s] capture sample_rate=%d", __func__, sample_rate);

#ifdef CODEC_TIMER
            cur_codec_freq = codec_adc_sample_rate[rate_idx].codec_freq;
#endif

            codec_rate_idx[AUD_STREAM_CAPTURE] = rate_idx;

            if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6)) {
                // If EC enabled, init resample-adc-ch0 to adc0
                codec->REG_0E4 = SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_ADC_CH0_SEL, 0);
            }

#ifdef __AUDIO_RESAMPLE__
            uint32_t mask, val;
            uint32_t normal_chan_num;

            mask = CODEC_CODEC_RESAMPLE_ADC_DUAL_CH |
                CODEC_CODEC_RESAMPLE_ADC_CH0_SEL_MASK | CODEC_CODEC_RESAMPLE_ADC_CH1_SEL_MASK |
                CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;

            normal_chan_num = cfg->channel_num;
            if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH5) {
                normal_chan_num--;
            }
            if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH6) {
                normal_chan_num--;
            }

            if (hal_cmu_get_audio_resample_status() && codec_adc_sample_rate[rate_idx].codec_freq != CODEC_FREQ_CRYSTAL) {
#ifdef CODEC_TIMER
                cur_codec_freq = CODEC_FREQ_CRYSTAL;
#endif
                bool update_factor = false;

#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
                if ((codec->REG_000 & CODEC_ADC_ENABLE) == 0) {
                    update_factor = true;
                }
#else
                if ((codec->REG_0E4 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) == 0 ||
                        resample_rate_idx[AUD_STREAM_CAPTURE] != rate_idx) {
                    update_factor = true;
                }
#endif
                if (update_factor) {
                    resample_rate_idx[AUD_STREAM_CAPTURE] = rate_idx;
                    codec->REG_0E4 &= ~mask;
                    hal_codec_reg_update_delay();
#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
                    enum HAL_CODEC_SYNC_TYPE_T sync_type;

                    sync_type = GET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_ADC_TRIGGER_SEL);
                    if (sync_type != HAL_CODEC_SYNC_TYPE_NONE) {
                        codec->REG_0E4 = SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_ADC_TRIGGER_SEL, HAL_CODEC_SYNC_TYPE_NONE);
                        hal_codec_reg_update_delay();
                    }
                    codec->REG_0F8 = resample_phase_float_to_value(1.0f);
#else
                    codec->REG_0F8 = resample_phase_float_to_value(get_capture_resample_phase());
#endif
                    hal_codec_reg_update_delay();
                    codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
                    if (sync_type != HAL_CODEC_SYNC_TYPE_NONE) {
                        hal_codec_reg_update_delay();
                        codec->REG_0E4 = SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_ADC_TRIGGER_SEL, sync_type);
                    }
#endif
                    ASSERT(cfg->channel_num == AUD_CHANNEL_NUM_2 || cfg->channel_num == AUD_CHANNEL_NUM_1,
                        "%s: Invalid capture resample chan num: %d", __func__, cfg->channel_num);
                    cnt = 0;
                    val = 0;
                    for (i = 0; i < NORMAL_ADC_CH_NUM; i++) {
                        if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
                            if (cnt == 0) {
                                val |= CODEC_CODEC_RESAMPLE_ADC_CH0_SEL(i);
                            } else {
                                val |= CODEC_CODEC_RESAMPLE_ADC_CH1_SEL(i);
                            }
                            cnt++;
                        }
                    }
                    if (normal_chan_num == AUD_CHANNEL_NUM_2) {
                        val |= CODEC_CODEC_RESAMPLE_ADC_DUAL_CH;
                    }
#ifdef SIDETONE_ENABLE
                    ASSERT(normal_chan_num <= AUD_CHANNEL_NUM_1, "%s: Cap normal resample chan num must <= 1 when sidetone enabled", __func__);
                    i = get_lsb_pos(sidetone_adc_ch_map);
                    val |= CODEC_CODEC_RESAMPLE_ADC_CH1_SEL(i);
#endif
                    codec->REG_0E4 |= val;
                }
            } else {
                codec->REG_0E4 &= ~mask;
            }

            if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6)) {
                if (normal_chan_num && (codec->REG_0E4 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) == 0) {
                    for (i = 0; i < NORMAL_ADC_CH_NUM; i++) {
                        if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
                            codec->REG_0E4 = SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_ADC_CH0_SEL, i);
                            break;
                        }
                    }
                }
            }
#endif

#ifdef __AUDIO_RESAMPLE__
            if (!hal_cmu_get_audio_resample_status())
#endif
            {
#ifdef __AUDIO_RESAMPLE__
                ASSERT(codec_adc_sample_rate[rate_idx].codec_freq != CODEC_FREQ_CRYSTAL,
                    "%s: capture sample rate %u is for resample only", __func__, sample_rate);
#endif
                analog_aud_freq_pll_config(codec_adc_sample_rate[rate_idx].codec_freq, codec_adc_sample_rate[rate_idx].codec_div);
                hal_cmu_codec_adc_set_div(codec_adc_sample_rate[rate_idx].cmu_div);
            }
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

int hal_codec_anc_adc_enable(enum ANC_TYPE_T type)
{
#ifdef ANC_APP
    enum AUD_CHANNEL_MAP_T map;
    enum AUD_CHANNEL_MAP_T mic_map;
    uint8_t ch_idx;

    map = 0;
    mic_map = 0;
    if (type == ANC_FEEDFORWARD) {
#if defined(ANC_FF_MIC_CH_L) || defined(ANC_FF_MIC_CH_R)
        if (ANC_FF_MIC_CH_L) {
            ch_idx = get_msb_pos(ANC_FF_MIC_CH_L);
            if (ANC_FF_MIC_CH_L & AUD_CHANNEL_MAP_DIGMIC_ALL) {
                codec->REG_0A8 = SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH0, ch_idx);
                codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH0;
            } else {
                codec->REG_084 = SET_BITFIELD(codec->REG_084, CODEC_CODEC_ADC_IN_SEL_CH0, ch_idx);
                codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH0;
            }
            map |= AUD_CHANNEL_MAP_CH0;
            mic_map |= ANC_FF_MIC_CH_L;
        }
        if (ANC_FF_MIC_CH_R) {
            ch_idx = get_msb_pos(ANC_FF_MIC_CH_R);
            if (ANC_FF_MIC_CH_R & AUD_CHANNEL_MAP_DIGMIC_ALL) {
                codec->REG_0A8 = SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH1, ch_idx);
                codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH1;
            } else {
                codec->REG_088 = SET_BITFIELD(codec->REG_088, CODEC_CODEC_ADC_IN_SEL_CH1, ch_idx);
                codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH1;
            }
            map |= AUD_CHANNEL_MAP_CH1;
            mic_map |= ANC_FF_MIC_CH_R;
        }
#else
        ASSERT(false, "No ana adc ff ch defined");
#endif
    } else if (type == ANC_FEEDBACK) {
#if defined(ANC_FB_MIC_CH_L) || defined(ANC_FB_MIC_CH_R)
        if (ANC_FB_MIC_CH_L) {
            ch_idx = get_msb_pos(ANC_FB_MIC_CH_L);
            if (ANC_FB_MIC_CH_L & AUD_CHANNEL_MAP_DIGMIC_ALL) {
                codec->REG_0A8 = SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH2, ch_idx);
                codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH2;
            } else {
                codec->REG_08C = SET_BITFIELD(codec->REG_08C, CODEC_CODEC_ADC_IN_SEL_CH2, ch_idx);
                codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH2;
            }
            map |= AUD_CHANNEL_MAP_CH2;
            mic_map |= ANC_FB_MIC_CH_L;
        }
        if (ANC_FB_MIC_CH_R) {
            ch_idx = get_msb_pos(ANC_FB_MIC_CH_R);
            if (ANC_FB_MIC_CH_R & AUD_CHANNEL_MAP_DIGMIC_ALL) {
                codec->REG_0A8 = SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH3, ch_idx);
                codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH3;
            } else {
                codec->REG_090 = SET_BITFIELD(codec->REG_090, CODEC_CODEC_ADC_IN_SEL_CH3, ch_idx);
                codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH3;
            }
            map |= AUD_CHANNEL_MAP_CH3;
            mic_map |= ANC_FB_MIC_CH_R;
        }
#else
        ASSERT(false, "No ana adc fb ch defined");
#endif
    }
    anc_adc_ch_map |= map;
    anc_mic_ch_map |= mic_map;

    if (anc_mic_ch_map & AUD_CHANNEL_MAP_DIGMIC_ALL) {
        hal_codec_enable_dig_mic(anc_mic_ch_map);
    }

    for (int i = 0; i < NORMAL_ADC_CH_NUM; i++) {
        if (map & (AUD_CHANNEL_MAP_CH0 << i)) {
            if ((codec->REG_080 & (CODEC_CODEC_ADC_EN_CH0 << i)) == 0) {
                // Reset ADC channel
                codec->REG_064 &= ~CODEC_SOFT_RSTN_ADC(1 << i);
                codec->REG_064 |= CODEC_SOFT_RSTN_ADC(1 << i);
                codec->REG_080 |= (CODEC_CODEC_ADC_EN_CH0 << i);
            }
        }
    }

#ifdef DAC_DRE_ENABLE
    if (anc_adc_ch_map && (codec->REG_098 & CODEC_CODEC_DAC_EN)) {
        hal_codec_dac_dre_disable();
    }
#endif
#endif

    return 0;
}

int hal_codec_anc_adc_disable(enum ANC_TYPE_T type)
{
#ifdef ANC_APP
    enum AUD_CHANNEL_MAP_T map;
    enum AUD_CHANNEL_MAP_T mic_map;

    map = 0;
    mic_map = 0;
    if (type == ANC_FEEDFORWARD) {
#if defined(ANC_FF_MIC_CH_L) || defined(ANC_FF_MIC_CH_R)
        if (ANC_FF_MIC_CH_L) {
            map |= AUD_CHANNEL_MAP_CH0;
            mic_map |= ANC_FF_MIC_CH_L;
        }
        if (ANC_FF_MIC_CH_R) {
            map |= AUD_CHANNEL_MAP_CH1;
            mic_map |= ANC_FF_MIC_CH_R;
        }
#endif
    } else if (type == ANC_FEEDBACK) {
#if defined(ANC_FB_MIC_CH_L) || defined(ANC_FB_MIC_CH_R)
        if (ANC_FB_MIC_CH_L) {
            map |= AUD_CHANNEL_MAP_CH2;
            mic_map |= ANC_FB_MIC_CH_L;
        }
        if (ANC_FB_MIC_CH_R) {
            map |= AUD_CHANNEL_MAP_CH3;
            mic_map |= ANC_FB_MIC_CH_R;
        }
#endif
    }
    anc_adc_ch_map &= ~map;
    anc_mic_ch_map &= ~mic_map;

    if ((anc_mic_ch_map & AUD_CHANNEL_MAP_DIGMIC_ALL) == 0 &&
            (codec_mic_ch_map & AUD_CHANNEL_MAP_DIGMIC_ALL) == 0) {
        hal_codec_disable_dig_mic();
    }

    for (int i = 0; i < NORMAL_ADC_CH_NUM; i++) {
        if ((map & (AUD_CHANNEL_MAP_CH0 << i)) && ((codec->REG_000 & CODEC_ADC_ENABLE) == 0 ||
                (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) == 0)) {
            codec->REG_080 &= ~(CODEC_CODEC_ADC_EN_CH0 << i);
        }
    }

#ifdef DAC_DRE_ENABLE
    if (anc_adc_ch_map == 0 && (codec->REG_098 & CODEC_CODEC_DAC_EN) &&
            //(codec->REG_044 & CODEC_MODE_16BIT_DAC) == 0 &&
            1) {
        hal_codec_dac_dre_enable();
    }
#endif
#endif

    return 0;
}

enum AUD_SAMPRATE_T hal_codec_anc_convert_rate(enum AUD_SAMPRATE_T rate)
{
    if (hal_cmu_get_audio_resample_status()) {
        return AUD_SAMPRATE_50781;
    } else if (CODEC_FREQ_48K_SERIES / rate * rate == CODEC_FREQ_48K_SERIES) {
        return AUD_SAMPRATE_48000;
    } else /* if (CODEC_FREQ_44_1K_SERIES / rate * rate == CODEC_FREQ_44_1K_SERIES) */ {
        return AUD_SAMPRATE_44100;
    }
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

#ifdef ANC_APP
void hal_codec_apply_dac_gain_offset(enum HAL_CODEC_ID_T id, int16_t offset)
{
    int16_t new_offset;

    offset += (offset > 0) ? 2 : -2;
    new_offset = QDB_TO_DB(offset);

    if (digdac_gain_offset_anc == new_offset) {
        return;
    }

    TRACE("hal_codec: apply dac gain offset: %d", new_offset);

    digdac_gain_offset_anc = new_offset;

#ifdef AUDIO_OUTPUT_SW_GAIN
    hal_codec_set_sw_gain(digdac_gain[0]);
#else
    hal_codec_restore_dig_dac_gain();
#endif
}

void hal_codec_anc_init_speedup(bool enable)
{
    if (enable) {
        hal_codec_int_open(HAL_CODEC_OPEN_ANC_INIT);
        codec->REG_1C0 = SET_BITFIELD(codec->REG_1C0, CODEC_CODEC_ADC_DRE_INI_ANA_GAIN_CH0, 0xF);
        codec->REG_1C8 = SET_BITFIELD(codec->REG_1C8, CODEC_CODEC_ADC_DRE_INI_ANA_GAIN_CH1, 0xF);
        codec->REG_1D0 = SET_BITFIELD(codec->REG_1D0, CODEC_CODEC_ADC_DRE_INI_ANA_GAIN_CH2, 0xF);
        codec->REG_1D8 = SET_BITFIELD(codec->REG_1D8, CODEC_CODEC_ADC_DRE_INI_ANA_GAIN_CH3, 0xF);
    } else {
        codec->REG_1C0 = SET_BITFIELD(codec->REG_1C0, CODEC_CODEC_ADC_DRE_INI_ANA_GAIN_CH0, 0);
        codec->REG_1C8 = SET_BITFIELD(codec->REG_1C8, CODEC_CODEC_ADC_DRE_INI_ANA_GAIN_CH1, 0);
        codec->REG_1D0 = SET_BITFIELD(codec->REG_1D0, CODEC_CODEC_ADC_DRE_INI_ANA_GAIN_CH2, 0);
        codec->REG_1D8 = SET_BITFIELD(codec->REG_1D8, CODEC_CODEC_ADC_DRE_INI_ANA_GAIN_CH3, 0);
        hal_codec_int_close(HAL_CODEC_OPEN_ANC_INIT);
    }
}
#endif

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
    uint8_t digmic_ch0;
    uint8_t en_ch;
    bool digmic;
    int i;

    adc_ch = MAX_ADC_CH_NUM;

    mic_ch = get_lsb_pos(mic_map);

    if (((1 << mic_ch) & codec_mic_ch_map) == 0) {
        return adc_ch;
    }

    digmic_ch0 = get_lsb_pos(AUD_CHANNEL_MAP_DIGMIC_CH0);

    if (mic_ch >= digmic_ch0) {
        mic_ch -= digmic_ch0;
        digmic = true;
    } else {
        digmic = false;
    }

    for (i = 0; i < NORMAL_ADC_CH_NUM; i++) {
        if (codec_adc_ch_map & (1 << i)) {
            if (digmic) {
                en_ch = (codec->REG_0A8 & (CODEC_CODEC_PDM_MUX_CH0_MASK << (3 * i))) >> (CODEC_CODEC_PDM_MUX_CH0_SHIFT + 3 * i);
            } else {
                en_ch = GET_BITFIELD(*(&codec->REG_084 + i), CODEC_CODEC_ADC_IN_SEL_CH0);
            }
            if (mic_ch == en_ch) {
                adc_ch = i;
                break;
            }
        }
    }

    return adc_ch;
}

void hal_codec_sidetone_enable(void)
{
#ifdef SIDETONE_ENABLE
#if (CFG_HW_AUD_SIDETONE_MIC_DEV & (CFG_HW_AUD_SIDETONE_MIC_DEV - 1))
#error "Invalid CFG_HW_AUD_SIDETONE_MIC_DEV: only 1 mic can be defined"
#endif
#if (CFG_HW_AUD_SIDETONE_MIC_DEV == 0) || (CFG_HW_AUD_SIDETONE_MIC_DEV & ~NORMAL_MIC_MAP)
#error "Invalid CFG_HW_AUD_SIDETONE_MIC_DEV: bad mic channel"
#endif
    enum AUD_CHANNEL_MAP_T mic_map = CFG_HW_AUD_SIDETONE_MIC_DEV;
    int gain = CFG_HW_AUD_SIDETONE_GAIN_DBVAL;
    uint32_t val;
    uint8_t adc_ch;
    uint8_t metal_id;

    sidetone_gain_offset = 0;
    if (gain > MAX_SIDETONE_DBVAL) {
        sidetone_gain_offset = gain - MAX_SIDETONE_DBVAL;
        gain = MAX_SIDETONE_DBVAL;
    } else if (gain < MIN_SIDETONE_DBVAL) {
        sidetone_gain_offset = gain - MIN_SIDETONE_DBVAL;
        gain = MIN_SIDETONE_DBVAL;
    }

    val = MIN_SIDETONE_REGVAL + (gain - MIN_SIDETONE_DBVAL) / SIDETONE_DBVAL_STEP;

    metal_id = hal_get_chip_metal_id();
    if (metal_id < HAL_CHIP_METAL_ID_3) {
        ASSERT((codec->REG_0E4 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) == 0,
            "%s: Not support resample for metal %u", __func__, metal_id);

        adc_ch = hal_codec_get_adc_chan(mic_map);
        if (adc_ch >= NORMAL_ADC_CH_NUM) {
            return;
        }

        codec->REG_080 = (codec->REG_080 & ~(CODEC_CODEC_SIDE_TONE_GAIN_MASK | CODEC_CODEC_SIDE_TONE_MIC_SEL | CODEC_CODEC_LOOP_SEL_L_MASK)) |
            CODEC_CODEC_SIDE_TONE_GAIN(val) | CODEC_CODEC_LOOP_SEL_L(adc_ch);
    } else {
        adc_ch = get_lsb_pos(sidetone_adc_ch_map);
        codec->REG_080 = SET_BITFIELD(codec->REG_080, CODEC_CODEC_SIDE_TONE_GAIN, val) | (CODEC_CODEC_ADC_EN_CH0 << adc_ch);
        hal_codec_set_dig_adc_gain(sidetone_adc_ch_map, sidetone_adc_gain + sidetone_gain_offset);
#ifdef CFG_HW_AUD_SIDETONE_IIR_INDEX
#if (CFG_HW_AUD_SIDETONE_IIR_INDEX >= ADC_IIR_CH_NUM + 0UL)
#error "Invalid CFG_HW_AUD_SIDETONE_IIR_INDEX"
#endif
        uint32_t mask, val;

        if (CFG_HW_AUD_SIDETONE_IIR_INDEX == 0) {
            mask = CODEC_CODEC_ADC_IIR_CH0_SEL_MASK;
            val = CODEC_CODEC_ADC_IIR_CH0_SEL(adc_ch);
        } else {
            mask = CODEC_CODEC_ADC_IIR_CH1_SEL_MASK;
            val = CODEC_CODEC_ADC_IIR_CH1_SEL(adc_ch);
        }
        codec->REG_078 = (codec->REG_078 & ~mask) | val;
#endif
    }
#endif
}

void hal_codec_sidetone_disable(void)
{
#ifdef SIDETONE_ENABLE
    codec->REG_080 = SET_BITFIELD(codec->REG_080, CODEC_CODEC_SIDE_TONE_GAIN, MIN_SIDETONE_REGVAL);
    if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_3) {
        if (sidetone_adc_ch_map) {
            uint8_t adc_ch;

            adc_ch = get_lsb_pos(sidetone_adc_ch_map);
            codec->REG_080 &= ~(CODEC_CODEC_ADC_EN_CH0 << adc_ch);
        }
    }
#endif
}

void hal_codec_select_adc_iir_mic(uint32_t index, enum AUD_CHANNEL_MAP_T mic_map)
{
    uint32_t mask, val;
    uint8_t adc_ch;

    ASSERT(index < ADC_IIR_CH_NUM, "%s: Bad index=%u", __func__, index);
    ASSERT(mic_map && (mic_map & (mic_map - 1)) == 0, "%s: Bad mic_map=0x%X", __func__, mic_map);
#ifdef CFG_HW_AUD_SIDETONE_IIR_INDEX
    ASSERT(index != CFG_HW_AUD_SIDETONE_IIR_INDEX, "%s: Adc iir index conflicts with sidetone", __func__);
#endif

    adc_ch = hal_codec_get_adc_chan(mic_map);
    if (index == 0) {
        mask = CODEC_CODEC_ADC_IIR_CH0_SEL_MASK;
        val = CODEC_CODEC_ADC_IIR_CH0_SEL(adc_ch);
    } else {
        mask = CODEC_CODEC_ADC_IIR_CH1_SEL_MASK;
        val = CODEC_CODEC_ADC_IIR_CH1_SEL(adc_ch);
    }
    codec->REG_078 = (codec->REG_078 & ~mask) | val;
}

void hal_codec_sync_dac_enable(enum HAL_CODEC_SYNC_TYPE_T type)
{
#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
    hal_codec_sync_dac_resample_rate_enable(type);
    codec->REG_054 = SET_BITFIELD(codec->REG_054, CODEC_DAC_ENABLE_SEL, type);
#else
    codec->REG_054 = SET_BITFIELD(codec->REG_054, CODEC_CODEC_DAC_ENABLE_SEL, type);
#endif
}

void hal_codec_sync_dac_disable(void)
{
#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
    hal_codec_sync_dac_resample_rate_disable();
    codec->REG_054 = SET_BITFIELD(codec->REG_054, CODEC_DAC_ENABLE_SEL, HAL_CODEC_SYNC_TYPE_NONE);
#else
    codec->REG_054 = SET_BITFIELD(codec->REG_054, CODEC_CODEC_DAC_ENABLE_SEL, HAL_CODEC_SYNC_TYPE_NONE);
#endif
}

void hal_codec_sync_adc_enable(enum HAL_CODEC_SYNC_TYPE_T type)
{
#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
    hal_codec_sync_adc_resample_rate_enable(type);
    codec->REG_054 = SET_BITFIELD(codec->REG_054, CODEC_ADC_ENABLE_SEL, type);
#else
    codec->REG_054 = SET_BITFIELD(codec->REG_054, CODEC_CODEC_ADC_ENABLE_SEL, type);
#endif
}

void hal_codec_sync_adc_disable(void)
{
#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
    hal_codec_sync_adc_resample_rate_disable();
    codec->REG_054 = SET_BITFIELD(codec->REG_054, CODEC_ADC_ENABLE_SEL, HAL_CODEC_SYNC_TYPE_NONE);
#else
    codec->REG_054 = SET_BITFIELD(codec->REG_054, CODEC_CODEC_ADC_ENABLE_SEL, HAL_CODEC_SYNC_TYPE_NONE);
#endif
}

void hal_codec_sync_dac_resample_rate_enable(enum HAL_CODEC_SYNC_TYPE_T type)
{
    codec->REG_0E4 = SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_DAC_TRIGGER_SEL, type);
}

void hal_codec_sync_dac_resample_rate_disable(void)
{
    codec->REG_0E4 = SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_DAC_TRIGGER_SEL, HAL_CODEC_SYNC_TYPE_NONE);
}

void hal_codec_sync_adc_resample_rate_enable(enum HAL_CODEC_SYNC_TYPE_T type)
{
    codec->REG_0E4 = SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_ADC_TRIGGER_SEL, type);
}

void hal_codec_sync_adc_resample_rate_disable(void)
{
    codec->REG_0E4 = SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_ADC_TRIGGER_SEL, HAL_CODEC_SYNC_TYPE_NONE);
}

void hal_codec_sync_dac_gain_enable(enum HAL_CODEC_SYNC_TYPE_T type)
{
    codec->REG_09C = SET_BITFIELD(codec->REG_09C, CODEC_CODEC_DAC_GAIN_TRIGGER_SEL, type);
}

void hal_codec_sync_dac_gain_disable(void)
{
    codec->REG_09C = SET_BITFIELD(codec->REG_09C, CODEC_CODEC_DAC_GAIN_TRIGGER_SEL, HAL_CODEC_SYNC_TYPE_NONE);
}

void hal_codec_sync_adc_gain_enable(enum HAL_CODEC_SYNC_TYPE_T type)
{
}

void hal_codec_sync_adc_gain_disable(void)
{
}

#ifdef CODEC_TIMER
uint32_t hal_codec_timer_get(void)
{
    if (codec_opened) {
        return codec->REG_050;
    }

    return 0;
}

uint32_t hal_codec_timer_ticks_to_us(uint32_t ticks)
{
    uint32_t timer_freq;

    timer_freq = cur_codec_freq / 4;

    return (uint32_t)((float)ticks * 1000000 / timer_freq);
}

void hal_codec_timer_trigger_read(void)
{
    if (codec_opened) {
        codec->REG_078 ^= CODEC_GET_CNT_TRIG;
        hal_codec_reg_update_delay();
    }
}
#endif

#ifdef AUDIO_OUTPUT_DC_CALIB_ANA
int hal_codec_dac_sdm_reset_set(void)
{
    if (codec_opened) {
        hal_codec_set_dac_gain_value(VALID_DAC_MAP, 0);
        if (codec->REG_098 & CODEC_CODEC_DAC_EN) {
            osDelay(dac_delay_ms);
        }
        codec->REG_098 |= CODEC_CODEC_DAC_SDM_CLOSE;
    }

    return 0;
}

int hal_codec_dac_sdm_reset_clear(void)
{
    if (codec_opened) {
        codec->REG_098 &= ~CODEC_CODEC_DAC_SDM_CLOSE;
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
        if (codec->REG_0E4 & CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE) {
            codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
            hal_codec_reg_update_delay();
            val = resample_phase_float_to_value(get_playback_resample_phase());
            val += (int)(val * ratio);
            codec->REG_0F4 = val;
            hal_codec_reg_update_delay();
            codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
        }
    } else {
        if (codec->REG_0E4 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) {
            codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
            hal_codec_reg_update_delay();
            val = resample_phase_float_to_value(get_capture_resample_phase());
            val -= (int)(val * ratio);
            codec->REG_0F8 = val;
            hal_codec_reg_update_delay();
            codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
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

    update[0] = !!(codec->REG_0E4 & CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE);
    update[1] = !!(codec->REG_0E4 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE);

    if (update[0]) {
        codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
        val[0] = resample_phase_float_to_value(get_playback_resample_phase());
        val[0] += (int)(val[0] * ratio);
    }
    if (update[1]) {
        codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
        val[1] = resample_phase_float_to_value(get_capture_resample_phase());
        val[1] -= (int)(val[1] * ratio);
    }

    hal_codec_reg_update_delay();

    lock = int_lock();
    if (update[0]) {
        codec->REG_0F4 = val[0];
    }
    if (update[1]) {
        codec->REG_0F8 = val[1];
    }
    int_unlock(lock);

    hal_codec_reg_update_delay();

    if (update[0]) {
        codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
    }
    if (update[1]) {
        codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
    }
#endif
}

int hal_codec_select_clock_out(uint32_t cfg)
{
    uint32_t lock;
    int ret = 1;

    lock = int_lock();

    if (codec_opened) {
        codec->REG_060 = SET_BITFIELD(codec->REG_060, CODEC_CFG_CLK_OUT, cfg);
        ret = 0;
    }

    int_unlock(lock);

    return ret;
}

#ifdef AUDIO_ANC_FB_MC
void hal_codec_setup_mc(enum AUD_CHANNEL_NUM_T channel_num, enum AUD_BITS_T bits)
{
    if (channel_num == AUD_CHANNEL_NUM_2) {
        mc_dual_chan = true;
    } else {
        mc_dual_chan = false;
    }

    if (bits <= AUD_BITS_16) {
        mc_16bit = true;
    } else {
        mc_16bit = false;
    }
}
#endif

void hal_codec_swap_output(bool swap)
{
#ifdef AUDIO_OUTPUT_SWAP
    output_swap = swap;

    if (codec_opened) {
        if (output_swap) {
            codec->REG_0A0 |= CODEC_CODEC_DAC_OUT_SWAP;
        } else {
            codec->REG_0A0 &= ~CODEC_CODEC_DAC_OUT_SWAP;
        }
    }
#endif
}

/* AUDIO CODEC VOICE ACTIVE DETECTION DRIVER */
//#define CODEC_VAD_DEBUG

struct codec_vad_cb {
    codec_vad_handler_t handler;
    uint8_t *buf_addr;
    uint32_t buf_size;
};

static struct codec_vad_cb vad_cb[1];

static void hal_codec_vad_setup_cb(codec_vad_handler_t h, uint8_t *buf_addr, uint32_t buf_size)
{
    struct codec_vad_cb *pcb = vad_cb;

    pcb->handler = h;
    pcb->buf_addr = buf_addr;
    pcb->buf_size = buf_size;
}

static inline void hal_codec_vad_set_udc(int v)
{
    codec->REG_14C &= ~CODEC_VAD_U_DC(0xf);
    codec->REG_14C |= CODEC_VAD_U_DC(v);
}

static inline void hal_codec_vad_set_upre(int v)
{
    codec->REG_14C &= ~CODEC_VAD_U_PRE(0x7);
    codec->REG_14C |= CODEC_VAD_U_PRE(v);
}

static inline void hal_codec_vad_set_frame_len(int v)
{
    codec->REG_14C &= ~CODEC_VAD_FRAME_LEN(0xff);
    codec->REG_14C |= CODEC_VAD_FRAME_LEN(v);
}

static inline void hal_codec_vad_set_mvad(int v)
{
    codec->REG_14C &= ~CODEC_VAD_MVAD(0xf);
    codec->REG_14C |= CODEC_VAD_MVAD(v);
}

static inline void hal_codec_vad_set_pre_gain(int v)
{
    codec->REG_14C &= ~CODEC_VAD_PRE_GAIN(0x3f);
    codec->REG_14C |= CODEC_VAD_PRE_GAIN(v);
}

static inline void hal_codec_vad_set_sth(int v)
{
    codec->REG_14C &= ~CODEC_VAD_STH(0x3f);
    codec->REG_14C |= CODEC_VAD_STH(v);
}

static inline void hal_codec_vad_set_frame_th1(int v)
{
    codec->REG_150 &= ~CODEC_VAD_FRAME_TH1(0xff);
    codec->REG_150 |= CODEC_VAD_FRAME_TH1(v);
}

static inline void hal_codec_vad_set_frame_th2(int v)
{
    codec->REG_150 &= ~CODEC_VAD_FRAME_TH2(0x3ff);
    codec->REG_150 |= CODEC_VAD_FRAME_TH2(v);
}

static inline void hal_codec_vad_set_frame_th3(int v)
{
    codec->REG_150 &= ~CODEC_VAD_FRAME_TH3(0x3fff);
    codec->REG_150 |= CODEC_VAD_FRAME_TH3(v);
}

static inline void hal_codec_vad_set_range1(int v)
{
    codec->REG_154 &= ~CODEC_VAD_RANGE1(0x1f);
    codec->REG_154 |= CODEC_VAD_RANGE1(v);
}

static inline void hal_codec_vad_set_range2(int v)
{
    codec->REG_154 &= ~CODEC_VAD_RANGE2(0x7f);
    codec->REG_154 |= CODEC_VAD_RANGE2(v);
}

static inline void hal_codec_vad_set_range3(int v)
{
    codec->REG_154 &= ~CODEC_VAD_RANGE3(0x1ff);
    codec->REG_154 |= CODEC_VAD_RANGE3(v);
}

static inline void hal_codec_vad_set_range4(int v)
{
    codec->REG_154 &= ~CODEC_VAD_RANGE4(0x3ff);
    codec->REG_154 |= CODEC_VAD_RANGE4(v);
}

static inline void hal_codec_vad_set_psd_th1(int v)
{
    codec->REG_158 &= ~CODEC_VAD_PSD_TH1(0x7ffffff);
    codec->REG_158 |= CODEC_VAD_PSD_TH1(v);
}

static inline void hal_codec_vad_set_psd_th2(int v)
{
    codec->REG_15C &= ~CODEC_VAD_PSD_TH2(0x7ffffff);
    codec->REG_15C |= CODEC_VAD_PSD_TH2(v);
}

static inline void hal_codec_vad_en(int enable)
{
    if (enable) {
        codec->REG_148 |= CODEC_VAD_EN; //enable vad
    } else {
        codec->REG_148 &= ~CODEC_VAD_EN; //disable vad
        codec->REG_148 |= CODEC_VAD_FINISH;
    }
}

static inline void hal_codec_vad_bypass_ds(int bypass)
{
    if (bypass)
        codec->REG_148 |= CODEC_VAD_DS_BYPASS; //bypass ds
    else
        codec->REG_148 &= ~CODEC_VAD_DS_BYPASS; //not bypass ds
}

static inline void hal_codec_vad_bypass_dc(int bypass)
{
    if (bypass)
        codec->REG_148 |= CODEC_VAD_DC_CANCEL_BYPASS; // bypass dc
    else
        codec->REG_148 &= ~CODEC_VAD_DC_CANCEL_BYPASS; //not bypass dc
}

static inline void hal_codec_vad_bypass_pre(int bypass)
{
    if (bypass)
        codec->REG_148 |= CODEC_VAD_PRE_BYPASS; //bypass pre
    else
        codec->REG_148 &= ~CODEC_VAD_PRE_BYPASS; //not bypass pre
}

static inline void hal_codec_vad_dig_mode(int enable)
{
    if (enable)
        codec->REG_148 |= CODEC_VAD_DIG_MODE; //digital mode
    else
        codec->REG_148 &= ~CODEC_VAD_DIG_MODE; //not digital mode
}

static inline void hal_codec_vad_adc_en(int enable)
{
    if (enable)
        codec->REG_080 |= (CODEC_CODEC_ADC_EN | CODEC_CODEC_ADC_EN_CH4);
    else
        codec->REG_080 &= ~CODEC_CODEC_ADC_EN_CH4;
}

static inline void hal_codec_vad_irq_en(int enable)
{
    if (enable){
        codec->REG_010 |= CODEC_VAD_FIND_MSK;
        codec->REG_010 |= CODEC_VAD_NOT_FIND_MSK;
    }
    else{
        codec->REG_010 &= ~CODEC_VAD_FIND_MSK;
        codec->REG_010 &= ~CODEC_VAD_NOT_FIND_MSK;
    }
}

static inline void hal_codec_vad_adc_if_en(int enable)
{
    if (enable)
        codec->REG_000 |= (CODEC_DMACTRL_RX | CODEC_ADC_ENABLE_CH4
                        | CODEC_ADC_ENABLE | CODEC_CODEC_IF_EN);
    else
        codec->REG_000 &= ~(CODEC_DMACTRL_RX | CODEC_ADC_ENABLE_CH4
                        | CODEC_ADC_ENABLE | CODEC_CODEC_IF_EN);
}

static inline void hal_codec_vad_adc_rst(int reset)
{
    if (!reset) { // release
        codec->REG_064 |= (CODEC_SOFT_RSTN_ADC_MASK | CODEC_SOFT_RSTN_ADC_ANA_MASK
                            | CODEC_SOFT_RSTN_32K);
    } else {
        codec->REG_064 &= (CODEC_SOFT_RSTN_ADC_MASK | CODEC_SOFT_RSTN_ADC_ANA_MASK
                            | CODEC_SOFT_RSTN_32K);
    }
}

static inline void hal_codec_vad_clk_en(int enable)
{
    if (enable) {
        codec->REG_060 |= (CODEC_EN_CLK_ADC(0x30) | CODEC_EN_CLK_ADC_ANA(0x11)
                            | CODEC_POL_ADC_ANA(0x11));
    } else {
        codec->REG_060 &= ~(CODEC_EN_CLK_ADC(0x30) | CODEC_EN_CLK_ADC_ANA(0x11)
                            | CODEC_POL_ADC_ANA(0x11));
    }
}

static inline void hal_codec_vad_adc_down(int v)
{
    unsigned int regval = codec->REG_094;

    regval &= ~CODEC_CODEC_ADC_DOWN_SEL_CH4(0x3);
    regval |= CODEC_CODEC_ADC_DOWN_SEL_CH4(v);
    codec->REG_094 = regval;
}

#ifdef CODEC_VAD_DEBUG
void hal_codec_vad_reg_dump(void)
{
    TRACE("codec base = %8x\n", (int)&(codec->REG_000));
    TRACE("codec->REG_000 = %x\n", codec->REG_000);
    TRACE("codec->REG_00C = %x\n", codec->REG_00C);
    TRACE("codec->REG_010 = %x\n", codec->REG_010);
    TRACE("codec->REG_060 = %x\n", codec->REG_060);
    TRACE("codec->REG_064 = %x\n", codec->REG_064);
    TRACE("codec->REG_080 = %x\n", codec->REG_080);
    TRACE("codec->REG_094 = %x\n", codec->REG_094);
    TRACE("codec->REG_148 = %x\n", codec->REG_148);
    TRACE("codec->REG_14C = %x\n", codec->REG_14C);
    TRACE("codec->REG_150 = %x\n", codec->REG_150);
    TRACE("codec->REG_154 = %x\n", codec->REG_154);
    TRACE("codec->REG_158 = %x\n", codec->REG_158);
    TRACE("codec->REG_15C = %x\n", codec->REG_15C);
}
#endif

static inline unsigned int hal_codec_get_irq_status(void)
{
    return codec->REG_00C;
}

static inline void hal_codec_vad_data_info(uint32_t *data_cnt, uint32_t *addr_cnt)
{
    uint32_t regval = codec->REG_160;

    *data_cnt = (regval & CODEC_VAD_MEM_DATA_CNT_MASK)
                    >> CODEC_VAD_MEM_DATA_CNT_SHIFT;

    *addr_cnt = (regval & CODEC_VAD_MEM_ADDR_CNT_MASK)
                    >> CODEC_VAD_MEM_ADDR_CNT_SHIFT;
}

uint32_t hal_codec_vad_recv_data(uint8_t *dst, uint32_t dst_size)
{
#define CODEC_VAD_BUF_SIZE 0x2000
#define CODEC_VAD_BUF_ADDR 0x40304000

    uint8_t *src = (uint8_t *)CODEC_VAD_BUF_ADDR;
    uint32_t src_size = CODEC_VAD_BUF_SIZE;
    uint32_t len, data_cnt = 0, addr_cnt = 0;

    hal_codec_vad_data_info(&data_cnt, &addr_cnt);

    TRACE("%s, dst=%x, dst_size=%d, data_cnt=%d, addr_cnt=%d",
        __func__, (uint32_t)dst, dst_size, data_cnt, addr_cnt);

    addr_cnt = (addr_cnt + 1) * 4;//to bytes number
    data_cnt = (data_cnt + 1) * 4;//to bytes number

    if (data_cnt > src_size || addr_cnt > src_size) {
        return 0;
    }

    if (addr_cnt < data_cnt) {
        // copy first part
        len = src_size - addr_cnt;
        if (len > dst_size)
            len = dst_size;
        memcpy(dst, src + addr_cnt, len);

        // copy second part
        dst_size -= len;
        if (dst_size > 0) {
            uint32_t len2;

            len2 = (addr_cnt < dst_size) ? addr_cnt : dst_size;
            memcpy(dst + len, src, len2);
            len += len2;
        }
    } else {
        len = (data_cnt < dst_size) ? data_cnt : dst_size;
        memcpy(dst, src, len);
    }

    TRACE("%s, len=%d", __func__, len);
    return len;
}

static inline void hal_codec_clr_irq_status(unsigned int mask)
{
    codec->REG_00C = mask;
}

static void hal_codec_vad_isr(void)
{
    uint32_t irq_status;
    uint32_t len = 0;
    struct codec_vad_cb *p = vad_cb;

    irq_status = hal_codec_get_irq_status();

    TRACE("%s, irq_status=0x%x\n", __func__, irq_status);

    if((irq_status & CODEC_VAD_FIND)){
    if (p->buf_addr && (p->buf_size > 0)) {
        len = hal_codec_vad_recv_data(p->buf_addr, p->buf_size);
    }
#if 0
    analog_aud_codec_vad_enable(false);
    hal_codec_vad_en(0);
#endif

    if (p->handler) {
            p->handler(p->buf_addr, len, irq_status);
    }

    // clear irq status
    hal_codec_clr_irq_status(CODEC_VAD_FIND);
    }else if(irq_status & CODEC_VAD_NOT_FIND){
        if (p->handler) {
            p->handler(p->buf_addr, len, irq_status);
        }
           // clear irq status
        hal_codec_clr_irq_status(CODEC_VAD_NOT_FIND);
    }
#if 0
    hal_codec_vad_en(1);
    analog_aud_codec_vad_enable(true);
#endif
}

static inline void hal_codec_vad_nvic_irq_en(int enable)
{
    /* FIXME
     * BUG: we should not set NVIC Vector for VAD ISR because of
     * CODEC's sub-module shares the same IRQ source.
     */
    if (enable) {
        NVIC_SetVector(CODEC_IRQn, (uint32_t)hal_codec_vad_isr);
        NVIC_SetPriority(CODEC_IRQn, IRQ_PRIORITY_NORMAL);
        NVIC_ClearPendingIRQ(CODEC_IRQn);
        NVIC_EnableIRQ(CODEC_IRQn);
    } else {
        NVIC_DisableIRQ(CODEC_IRQn);
        NVIC_ClearPendingIRQ(CODEC_IRQn);
    }
}

int hal_codec_vad_open(void)
{
    uint32_t lock;

    // open analog vad
    analog_aud_codec_vad_open();

    // enable vad clock
    hal_cmu_codec_vad_clock_enable(1);

    // release codec
    hal_cmu_codec_reset_clear();

    lock = int_lock();

    // release adc
    hal_codec_vad_adc_rst(0);

    // enable vad clock
    hal_codec_vad_clk_en(1);

    // enable adc if
    hal_codec_vad_adc_if_en(1);

    // enable adc
    hal_codec_vad_adc_en(1);

    // select adc down 16KHz
    hal_codec_vad_adc_down(0);

    int_unlock(lock);

    return 0;
}

void hal_codec_vad_close(unsigned int close_adc)
{
    uint32_t lock;

    lock = int_lock();
    TRACE("%s", __func__);

#ifdef I2C_VAD
    codec->REG_230 &= ~(CODEC_VAD_EXT_EN | CODEC_VAD_SRC_SEL);
#endif

    // disable vad
    //hal_codec_vad_en(0);

    // disable adc if
//    hal_codec_vad_adc_if_en(0);

    // disable adc
//    hal_codec_vad_adc_en(0);

    // disable vad clock
//    hal_codec_vad_clk_en(0);

    // disable vad clock
    //hal_cmu_codec_vad_clock_enable(0);

    int_unlock(lock);

    // close analog vad
    //analog_aud_codec_vad_close();
}

int hal_codec_vad_config(struct codec_vad_conf *conf)
{
    uint32_t lock;

    if (!conf)
        return -1;

    lock = int_lock();

    hal_codec_vad_en(0);
    hal_codec_vad_irq_en(0);

    hal_codec_vad_set_udc(conf->udc);
    hal_codec_vad_set_upre(conf->upre);
    hal_codec_vad_set_frame_len(conf->frame_len);
    hal_codec_vad_set_mvad(conf->mvad);
    hal_codec_vad_set_pre_gain(conf->pre_gain);
    hal_codec_vad_set_sth(conf->sth);
    hal_codec_vad_set_frame_th1(conf->frame_th[0]);
    hal_codec_vad_set_frame_th2(conf->frame_th[1]);
    hal_codec_vad_set_frame_th3(conf->frame_th[2]);
    hal_codec_vad_set_range1(conf->range[0]);
    hal_codec_vad_set_range2(conf->range[1]);
    hal_codec_vad_set_range3(conf->range[2]);
    hal_codec_vad_set_range4(conf->range[3]);
    hal_codec_vad_set_psd_th1(conf->psd_th[0]);
    hal_codec_vad_set_psd_th2(conf->psd_th[1]);
    hal_codec_vad_dig_mode(0);
    hal_codec_vad_bypass_ds(0);
    hal_codec_vad_bypass_dc(0);
    hal_codec_vad_bypass_pre(0);

    codec->REG_220 = 320;
    codec->REG_224 = 32000*3;//vad timeout value
#ifdef I2C_VAD
    codec->REG_230 |= CODEC_VAD_EXT_EN | CODEC_VAD_SRC_SEL;
#endif

#if !(defined(FIXED_CODEC_ADC_VOL) && defined(SINGLE_CODEC_ADC_VOL))
    const CODEC_ADC_VOL_T *adc_gain_db;

#ifdef SINGLE_CODEC_ADC_VOL
    adc_gain_db = hal_codec_get_adc_volume(CODEC_SADC_VOL);
#else
    adc_gain_db = hal_codec_get_adc_volume(hal_codec_get_mic_chan_volume_level(AUD_CHANNEL_MAP_CH4));
#endif
    if (adc_gain_db) {
        hal_codec_set_dig_adc_gain(AUD_CHANNEL_MAP_CH4, *adc_gain_db);
    }
#endif
    hal_codec_vad_setup_cb(conf->handler, conf->buf_addr, conf->buf_size);

    int_unlock(lock);
    return 0;
}

int hal_codec_vad_start(void)
{
    uint32_t lock;

    // digital vad
    lock = int_lock();
    hal_codec_clr_irq_status(CODEC_VAD_FIND | CODEC_VAD_NOT_FIND);
    hal_codec_vad_irq_en(1);
    hal_codec_vad_nvic_irq_en(1);
    hal_codec_vad_en(1);
    int_unlock(lock);

    // analog vad
    analog_aud_codec_vad_enable(true);

    return 0;
}

int hal_codec_vad_stop(void)
{
    uint32_t lock;
    TRACE("%s", __func__);
    lock = int_lock();
    hal_codec_vad_irq_en(0);
    hal_codec_vad_en(0);
    int_unlock(lock);

    //analog_aud_codec_vad_enable(false);
    return 0;
}

int hal_codec_config_digmic_phase(uint8_t phase)
{
#ifdef ANC_PROD_TEST
    codec_digmic_phase = phase;
#endif
    return 0;
}
