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
#include "plat_types.h"
#include "tgt_hardware.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "bt_drv.h"
#include "hal_timer.h"
#include "hal_chipid.h"
#include "bt_drv_1400_internal.h"
#include "bt_drv_interface.h"

typedef struct
{
    uint8_t is_act;
    uint16_t opcode;
    uint8_t parlen;
    uint8_t *param;

} BTDRV_CFG_TBL_STRUCT;


#define BTDRV_CONFIG_ACTIVE   1
#define BTDRV_CONFIG_INACTIVE 0

#if (BT_RFMODE == BT_LOW_POWER_MODE)
const int8_t btdrv_rf_env[]=
{
    0x01,0x00,  //rf api
    0x01,   //rf env
    136,     //rf length
    0x2,     //txpwr_max
    0,    ///rssi high thr
    -90,   //rssi low thr
    -70,  //rssi interf thr
    0xf,  //rssi interf gain thr
    2,  //wakeup delay
    0xe, 0, //skew
    0xe8,0x3,    //ble agc inv thr
    0xff,     //sw gain set
    1,      //sw gain reset factor
    1,    //bt sw gain cntl enable
    0,   //ble sw gain cntl en
    1,  //bt interfere  detector en
    0,  //ble interfere detector en

    51,0,-80,
    41,0,-80,
    32,0,-80,
    21,0,-80,
    15,0,-80,
    11,0,-80,
    5,0,-80,
    -7,0,-80,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,  //rx gain tbl ptr

    -79,-75,
    -63,-63,
    -47,-45,
    -36,-37,
    -30,-26,
    -25,-15,
    -18,-15,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,    //rx gain ths tbl ptr

    0,0,
    0,0,
    0,0,
    0,0,
    0,0,
    0,1,
    0,2,
    0,2,
    0,2,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,    //flpha filter factor ptr
    -23,-20,-17,-14,-11,-8,-5,-2,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,   //tx pw onv tbl ptr
};
#elif (BT_RFMODE == BT_HIGH_PERFORMANCE_MODE)
const int8_t btdrv_rf_env[]=
{
    0x01,0x00,  //rf api
    0x01,   //rf env
    136,     //rf length
    0x7,     //txpwr_max
    -20,    ///rssi high thr
    -60,   //rssi low thr
    -70,  //rssi interf thr
    0xf,  //rssi interf gain thr
    2,  //wakeup delay
    0xe, 0, //skew
    0xe8,0x3,    //ble agc inv thr
    0xff,     //sw gain set
    1,      //sw gain reset factor
    1,    //bt sw gain cntl enable
    1,   //ble sw gain cntl en
    0,  //bt interfere  detector en
    0,  //ble interfere detector en


    66,0,-80,
    57,0,-80,
    47,0,-80,
    36,0,-80,
    26,0,-80,
    16,0,-80,
    10,0,-80,

    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,  //rx gain tbl ptr

    -70,-70,
    -62,-62,
    -53,-53,
    -42,-42,
    -31,-31,
    -20,-20,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,    //rx gain ths tbl ptr


    0,0,
    0,0,
    0,0,
    0,0,
    0,0,
    0,1,
    0,2,
    0,2,
    0,2,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f    //flpha filter factor ptr
    -23,-20,-17,-14,-11,-8,-5,-2,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,   //tx pw onv tbl ptr
};
#endif

#if (BT_RFMODE == BT_LOW_POWER_MODE)
const int8_t btdrv_rf_env_1[]=
{
    0x01,0x00,  //rf api
    0x01,   //rf env
    185,     //rf length
    0x2,     //txpwr_max
    0,    ///rssi high thr
    -90,   //rssi low thr
    -70,  //rssi interf thr
    0xf,  //rssi interf gain thr
    2,  //wakeup delay
    0xe, 0, //skew
    0xe8,0x3,    // ble agc inv thr
#ifdef __HW_AGC__
      0x1,//hw_sw_agc_flag
#else
      0x0,//
#endif
    0xff, 0xff,  // sw gain set *
    0x7f,        // bt inq page iscan pscan dbm *
    0x7f,        // ble scan adv dbm *
    1,      //sw gain reset factor
    1,    //bt sw gain cntl enable
    0,   //ble sw gain cntl en
    1,  //bt interfere  detector en
    0,  //ble interfere detector en

    -21,-27,-25,
    -18,-24,-19,
    -15,-21,-13,
    -12,-18,-6,
    -9,-15,0,
    -6,-12,0x7f,
    -3,-9,0x7f,
    0,-6,0x7f,
    0x7f,-3,0x7f,
    0x7f,0,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f, //rx hwgain tbl ptr *

    51,0,-80,
    41,0,-80,
    32,0,-80,
    21,0,-80,
    15,0,-80,
    11,0,-80,
    5,0,-80,
    -7,0,-80,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,  //rx gain tbl ptr

    -79,-75,
    -63,-63,
    -47,-45,
    -36,-37,
    -30,-26,
    -25,-15,
    -18,-15,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,    //rx gain ths tbl ptr

    0,0,
    0,0,
    0,0,
    0,0,
    0,0,
    0,1,
    0,2,
    0,2,
    0,2,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,    //flpha filter factor ptr
    -23,-20,-17,-14,-11,-8,-5,-2,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,   //tx pw onv tbl ptr
};
#elif (BT_RFMODE == BT_HIGH_PERFORMANCE_MODE)
const int8_t btdrv_rf_env_1[]=
{
    0x01,0x00,  //rf api
    0x01,   //rf env
    185,     //rf length
    0x7,     //txpwr_max
    -20,    ///rssi high thr
    -60,   //rssi low thr
    -70,  //rssi interf thr
    0xf,  //rssi interf gain thr
    2,  //wakeup delay
    0xe, 0, //skew
    0xe8,0x3,    // ble agc inv thr
    0x00,        // hw sw agc flag *
    0xff, 0xff,  // sw gain set *
    0x7f,        // bt inq page iscan pscan dbm *
    0x7f,        // ble scan adv dbm *
    1,      //sw gain reset factor
    1,    //bt sw gain cntl enable
    1,   //ble sw gain cntl en
    0,  //bt interfere  detector en
    0,  //ble interfere detector en

    -21,-27,-25,
    -18,-24,-19,
    -15,-21,-13,
    -12,-18,-6,
    -9,-15,0,
    -6,-12,0x7f,
    -3,-9,0x7f,
    0,-6,0x7f,
    0x7f,-3,0x7f,
    0x7f,0,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f, //rx hwgain tbl ptr *

    66,0,-80,
    57,0,-80,
    47,0,-80,
    36,0,-80,
    26,0,-80,
    16,0,-80,
    10,0,-80,

    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,  //rx gain tbl ptr

    -70,-70,
    -62,-62,
    -53,-53,
    -42,-42,
    -31,-31,
    -20,-20,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,    //rx gain ths tbl ptr

    0,0,
    0,0,
    0,0,
    0,0,
    0,0,
    0,1,
    0,2,
    0,2,
    0,2,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f,
    0x7f,0x7f    //flpha filter factor ptr
    -23,-20,-17,-14,-11,-8,-5,-2,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,   //tx pw onv tbl ptr
};
#endif

const int8_t btdrv_bt_spi_cntl[]=
{
    0x01,0x00,  /*set rf env*/
    0x02,       /*set bt_spicntl*/
    0x10,       /*cmd length*/
    0xff,0xff,0xff,0xff,/*spiposbuf_addr*/
    0xff,0xff,0xff,0xff,/*spiposbuf_addr*/
    0x00,0x00,  /*spibufoffset*/
    0x00,       /*bit[30:24]: spi trig after rxon pos*/
    0x00,       /*bit[22:16]: spi trig before rxon pos*/
    0x33,       /*bit[14:8] : spi trig after txon pos*/
    0x00,           /*bit[6:0] : spi trig before txon pos*/
    0x00,       /*bit[14:8]: spi trig after rxon neg*/
    3           /*bit[6:0]: spi trig after txon neg*/
};

const int8_t btdrv_ble_spi_cntl[]=
{
    0x01,0x00,  /*set rf env*/
    0x04,       /*set ble_spicntl*/
    0x0f,       /*cmd length*/
    0xff,0xff,0xff,0xff,/*spiposbuf_addr*/
    0xff,0xff,0xff,0xff,/*spiposbuf_addr*/
    0x00,0x00,  /*spibufoffset*/
    0x00,       /*bit[23:16]: spi trig before/after the first txon/rxon pos*/
    0x00,       /*bit[14:8]: spi trig after rxon neg*/
    15,     /*bit[6:0]: spi trig after txon neg*/
    0x00,       /*bit[15:8]: spi trig before/after the no-first rxon pos*/
    15      /*bit[7:0]: spi trig before/after the no-first  txon pos*/
};


const int8_t btdrv_afh_env[] =
{
    0x02,0x00,   //afh env
    0x00,      //ignore
    33,          //length
    5,   //nb_reass_chnl
    10,  //win_len
    -70,  //rf_rssi_interf_thr
    10,  //per_thres_bad
    20,  //reass_int
    20,   //n_min
    20,   //afh_rep_intv_max
    96,    //ths_min
    2,   //chnl_assess_report_style
    15,  //chnl_assess_diff_thres
    60, // chnl_assess_interfere_per_thres_bad
    9,  //chnl_assess_stat_cnt_max
    -9,  //chnl_assess_stat_cnt_min
    1,2,3,2,1,   //chnl_assess_stat_cnt_inc_mask[5]
    1,2,3,2,1,    //chnl_assess_stat_cnt_dec_mask
    0xd0,0x7,      //chnl_assess_timer
    -48,        //chnl_assess_min_rssi
    0x64,0,   //chnl_assess_nb_pkt
    0x32,0,     //chnl_assess_nb_bad_pkt
    6,    //chnl_reassess_cnt_val
    0x3c,0,     //chnl_assess_interfere_per_thres_bad
};

const uint8_t lpclk_drift_jitter[] =
{
    0xfa,0x00,  //  drift  250ppm
    0x0a,0x00    //jitter  +-10us

};

const uint8_t  wakeup_timing[] =
{
    0xe8,0x3,   //exernal_wakeup_time 600us
    0xe8,0x3,    //oscillater_wakeup_time  600us
    0xe8,0x3,    //radio_wakeup_time  600us
    0xa0,0xf,    //wakeup_delay_time
};


uint8_t  sleep_param[] =
{
    1,    // sleep_en;
    0,    // exwakeup_en;
    0xc8,0,    //  lpo_calib_interval;   lpo calibration interval
    0x32,0,0,0,    // lpo_calib_time;  lpo count lpc times
};

uint8_t  unsleep_param[] =
{

    0,    // sleep_en;
    0,    // exwakeup_en;
    0xc8,0,    //  lpo_calib_interval;   lpo calibration interval
    0x32,0,0,0,    // lpo_calib_time;  lpo count lpc times
};

uint32_t me_init_param[][2] =
{
    {0xffffffff, 0xffffffff},
  //  {0xc0003ae0, 0xd02134fc},
    //{0xd02200f0, 0x021F0103},
};

const uint16_t me_bt_default_page_timeout = 0x2000;

const uint8_t  sync_config[] =
{
    1,1,   //sco path config   0:hci  1:pcm
    0,      //sync use max buff length   0:sync data length= packet length 1:sync data length = host sync buff len
    0,        //cvsd bypass     0:cvsd2pcm   1:cvsd transparent

};


//pcm general ctrl
#define PCM_PCMEN_POS            15
#define PCM_LOOPBCK_POS          14
#define PCM_MIXERDSBPOL_POS      11
#define PCM_MIXERMODE_POS        10
#define PCM_STUTTERDSBPOL_POS    9
#define PCM_STUTTERMODE_POS      8
#define PCM_CHSEL_POS            6
#define PCM_MSTSLV_POS           5
#define PCM_PCMIRQEN_POS         4
#define PCM_DATASRC_POS          0


//pcm phy ctrl
#define PCM_LRCHPOL_POS     15
#define PCM_CLKINV_POS      14
#define PCM_IOM_PCM_POS     13
#define PCM_BUSSPEED_LSB    10
#define PCM_SLOTEN_MASK     ((uint32_t)0x00000380)
#define PCM_SLOTEN_LSB      7
#define PCM_WORDSIZE_MASK   ((uint32_t)0x00000060)
#define PCM_WORDSIZE_LSB    5
#define PCM_DOUTCFG_MASK    ((uint32_t)0x00000018)
#define PCM_DOUTCFG_LSB     3
#define PCM_FSYNCSHP_MASK   ((uint32_t)0x00000007)
#define PCM_FSYNCSHP_LSB    0




/// Enumeration of PCM status
enum PCM_STAT
{
    PCM_DISABLE = 0,
    PCM_ENABLE
};

/// Enumeration of PCM channel selection
enum PCM_CHANNEL
{
    PCM_CH_0 = 0,
    PCM_CH_1
};

/// Enumeration of PCM role
enum PCM_MSTSLV
{
    PCM_SLAVE = 0,
    PCM_MASTER
};

/// Enumeration of PCM data source
enum PCM_SRC
{
    PCM_SRC_DPV = 0,
    PCM_SRC_REG
};



/// Enumeration of PCM left/right channel selection versus frame sync polarity
enum PCM_LR_CH_POL
{
    PCM_LR_CH_POL_RIGHT_LEFT = 0,
    PCM_LR_CH_POL_LEFT_RIGHT
};

/// Enumeration of PCM clock inversion
enum PCM_CLK_INV
{
    PCM_CLK_RISING_EDGE = 0,
    PCM_CLK_FALLING_EDGE
};

/// Enumeration of PCM mode selection
enum PCM_MODE
{
    PCM_MODE_PCM = 0,
    PCM_MODE_IOM
};

/// Enumeration of PCM bus speed
enum PCM_BUS_SPEED
{
    PCM_BUS_SPEED_128k = 0,
    PCM_BUS_SPEED_256k,
    PCM_BUS_SPEED_512k,
    PCM_BUS_SPEED_1024k,
    PCM_BUS_SPEED_2048k
};

/// Enumeration of PCM slot enable
enum PCM_SLOT
{
    PCM_SLOT_NONE = 0,
    PCM_SLOT_0,
    PCM_SLOT_0_1,
    PCM_SLOT_0_2,
    PCM_SLOT_0_3
};

/// Enumeration of PCM word size
enum PCM_WORD_SIZE
{
    PCM_8_BITS = 0,
    PCM_13_BITS,
    PCM_14_BITS,
    PCM_16_BITS
};

/// Enumeration of PCM DOUT pad configuration
enum PCM_DOUT_CFG
{
    PCM_OPEN_DRAIN = 0,
    PCM_PUSH_PULL_HZ,
    PCM_PUSH_PULL_0
};

/// Enumeration of PCM FSYNC physical shape
enum PCM_FSYNC
{
    PCM_FSYNC_LF = 0,
    PCM_FSYNC_FR,
    PCM_FSYNC_FF,
    PCM_FSYNC_LONG,
    PCM_FSYNC_LONG_16
};




const uint32_t pcm_setting[] =
{
    //pcm_general_ctrl
    (PCM_DISABLE<<PCM_PCMEN_POS) |                      //enable auto
    (PCM_DISABLE << PCM_LOOPBCK_POS)  |                 //LOOPBACK test
    (PCM_DISABLE << PCM_MIXERDSBPOL_POS)  |
    (PCM_DISABLE << PCM_MIXERMODE_POS)  |
    (PCM_DISABLE <<PCM_STUTTERDSBPOL_POS) |
    (PCM_DISABLE <<PCM_STUTTERMODE_POS) |
    (PCM_CH_0<< PCM_CHSEL_POS) |
    (PCM_MASTER<<PCM_MSTSLV_POS) |                      //BT clock
    (PCM_DISABLE << PCM_PCMIRQEN_POS) |
    (PCM_SRC_DPV<<PCM_DATASRC_POS),

    //pcm_phy_ctrl
    (PCM_LR_CH_POL_RIGHT_LEFT << PCM_LRCHPOL_POS) |
    (PCM_CLK_FALLING_EDGE << PCM_CLKINV_POS) |
    (PCM_MODE_PCM << PCM_IOM_PCM_POS) |
    (PCM_BUS_SPEED_2048k << PCM_BUSSPEED_LSB) |         //8k sample rate; 2048k = slot_num * sample_rate * bit= 16 * 8k * 16
    (PCM_SLOT_0_1 << PCM_SLOTEN_LSB) |
    (PCM_16_BITS << PCM_WORDSIZE_LSB) |
    (PCM_PUSH_PULL_0 << PCM_DOUTCFG_LSB) |
    (PCM_FSYNC_LF << PCM_FSYNCSHP_LSB),
};

#if 1
uint8_t local_feature[] =
{
#if defined(__3M_PACK__)
    0xBF, 0xeE, 0xCD,0xFe,0xdb,0xFd,0x7b,0x87
#else
    0xBF, 0xeE, 0xCD,0xFa,0xdb,0xFd,0x7b,0x87

    //0xBF,0xFE,0xCD,0xFa,0xDB,0xFd,0x73,0x87   // disable simple pairing
#endif
};

#else
// disable simple pairing
uint8_t local_feature[] =
{
    0xBF,0xFE,0xCD,0xFE,0xDB,0xFd,0x73,0x87
};
#endif
const uint8_t local_ex_feature_page1[] =
{
    1,   //page
    0,0,0,0,0,0,0,0,   //page 1 feature
};

const uint8_t local_ex_feature_page2[] =
{
    2,   //page
    0x1f,0x03,0x00,0x00,0x00,0x00,0x00,0x00,   //page 2 feature
};

const uint8_t bt_rf_timing[] =
{
    0xE,   //tx delay
    0x13,    //rx delay
    0x42,  //rx pwrup
    0x0f, ///tx down
    0x56,  //tx pwerup

};

const uint8_t ble_rf_timing[] =
{
    0xC,    //tx delay   tx after rx delay
    0x2C,    //win count   rx window count
    0xe,    ///ble rtrip delay
    0x42,  ///ble rx pwrup
    0x7,    ///ble tx pwerdown
    0x40,   ///ble tx pwerup
};



const uint8_t ble_rl_size[] =
{
    10,    //rl size
};

uint8_t bt_setting_1400[] =
{
    0x00,//clk_off_force_even
    0x01,//msbc_pcmdout_zero_flag
    0x01,//ld_sco_switch_timeout
    0x01,//stop_latency2
    0x01,//force_max_slot
    0xc8,0x00,//send_connect_info_to
    0x20,0x03,//sco_to_threshold
    0x40,0x06,//acl_switch_to_threshold
    0x3a,0x00,//sync_win_size
    0x01,//polling_rxseqn_mode
    0x00,//two_slave_sched_en
    0xff,//music_playing_link
    0x04,//wesco_nego = 2;
    0x17,0x11,0x00,0x00,//two_slave_extra_duration_add (7*625);
    0x01,//ble_adv_ignore_interval
    0x04,//slot_num_diff
    0x01,//csb_tx_complete_event_enable
    0x04,//csb_afh_update_period
    0x00,//csb_rx_fixed_len_enable
    0x01,//force_max_slot_acc
    0x00,//force_5_slot;
    0x00,//dbg_trace_level
    0x00,//bd_addr_switch
    0x00,//force_rx_error
    0x01,//retx_extra_dura
    0x08,//data_tx_adjust
    0x02,//ble_tx_power;
    0x50,//sco_margin
    0x78,//acl_margin
    0x00,//pca_disable_in_nosync
    0x01,//master_2_poll
    0x3a,//sync2_add_win
    0x5e,//no_sync_add_win
    0x09,//rwbt_sw_version_major
    0x03,0x00,//rwbt_sw_version_minor
    0x21,0x00,//rwbt_sw_version_build
    0x00,//rwbt_sw_version_sub_build
};

uint8_t bt_setting_1400_1[] =
{
    0x00,//clk_off_force_even
    0x01,//msbc_pcmdout_zero_flag
    0x03,//ld_sco_switch_timeout
    0x01,//stop_latency2
    0x01,//force_max_slot
    0xc8,0x00,//send_connect_info_to

    0x20,0x03,//sco_to_threshold
    0x40,0x06,//acl_switch_to_threshold
    0x3a,0x00,//sync_win_size
    0x01,//polling_rxseqn_mode
    0x00,//two_slave_sched_en

    0xff,//music_playing_link
    0x04,//wesco_nego = 2;
    0x17,0x11,0x00,0x00,//two_slave_extra_duration_add (7*625);

    0x01,//ble_adv_ignore_interval
    0x04,//slot_num_diff
    0x01,//csb_tx_complete_event_enable
    0x04,//csb_afh_update_period
    0x00,//csb_rx_fixed_len_enable
    0x01,//force_max_slot_acc
    0x00,//force_5_slot;
    0x03,//dbg_trace_level
    0x00,//bd_addr_switch
    0x00,//force_rx_error
    0x08,//data_tx_adjust
    0x02,//ble_tx_power;
    0x50,//sco_margin
    0x78,//acl_margin
    0x01,//pca_disable_in_nosync
    0x01,//master_2_poll

    0x3a,//sync2_add_win
    0x5e,//no_sync_add_win
    0x09,//rwbt_sw_version_major
    0x03,0x00,//rwbt_sw_version_minor
    0x21,0x00,//rwbt_sw_version_build

    0x01,//rwbt_sw_version_sub_build
    0x00,//public_key_check;
    0x00,//sco_txbuff_en;
    0x3f,//role_switch_packet_br;
    0x2b,//role_switch_packet_edr;
    0x00,//dual_slave_tws_en;
    0x07,//dual_slave_slot_add;
    0x00,//master_continue_tx_en;

    0x20,0x03,//get_rssi_to_threshold;
    0x05,//rssi_get_times;
    0x00,//ptt_check;
    0x12,//protect_sniff_slot;
    0x01,//power_control_type;
    #ifdef TX_RX_PCM_MASK
    0x01,//sco_buff_copy;
    #else
    0x00,
    #endif
    0x64,0x00,//acl_interv_in_snoop_mode;
    0x78,0x00,//acl_interv_in_snoop_sco_mode;

    0x06,//acl_slot_in_snoop_mode;
    0x01,//calculate_en;
    0x00,//check_sniff_en;
    0x01,//sco_avoid_ble_en;
    0x01,//check_ble_en;
    0x01,//ble_slot;
    0x00,//same_link_slave_extra;
    0x00,//other_link_slave_extra;

    0x00,//master_extra;
    0x00,//ble_extra;
    0x00,//sco_extra;
    0x00,//sniff_extra;
    0x00,0x00,0x00,0x00,//same_link_duration;
    0x00,0x00,0x00,0x00,//other_link_duration;

    0x06,//dual_slave;
    0x0a,//clk_check_threshold;
    0x90,0x01,//target_clk_add;
    0x01,//close_check_sco_to;
  //0x00,
  //0x00,                //chip tports_level (wrong) use btdrv_1401_set_tports_level()
  //0x00,
    0x01,//(tports_level)//chip read close_loopbacken_flag
    0x01,                //chip read based_clk_select
    0x01,                //chip read check_role_switch
    0x00,                //chip read rsw_end_prority
    0x00,                //chip read ecc_enable
    0x00,//padding to 97-byte
    0x00,//padding to 97-byte
    0x00,//padding to 97-byte
    0x00,//padding to 97-byte
};

void btdrv_1401_set_tports_level(uint16_t tports_level)
{
    if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1) {
        uint32_t value = bt_setting_1400_1[87]; // close_check_sco_to
        value |= (tports_level << 16);
        *((volatile uint32_t*)0xc00058b0) = value;
    }
}

const uint8_t bt_edr_thr[] =
{
    30,0,60,0,5,0,60,0,0,1,
    30,0,60,0,5,0,60,0,1,1,
    30,0,60,0,5,0,60,0,1,1,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
};


const uint8_t bt_edr_algo[]=
{
    0,0,1,
    8,0,3,
    16,0,0,
    0xff,0xff,0xff,
    0xff,0xff,0xff,
    0xff,0xff,0xff,
    0xff,0xff,0xff,
};

const uint8_t bt_rssi_thr[]=
{
    -20, //high
    -60, //low
    -70, //interf
};


const uint8_t ble_dle_dft_value[]=
{
    0xfb,0x00, ///tx octets
    0x48,0x08, ///tx time
    0xfb,0x00, ///rx octets
    0x48,0x08, ///rx time
};

const uint8_t bt_lmp_record[]=
{
    1,  //en
    1,  //only opcode
};

const static BTDRV_CFG_TBL_STRUCT  btdrv_cfg_tbl[] =
{
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_LOCAL_FEATURE_CMD_OPCODE,sizeof(local_feature),local_feature},
    {BTDRV_CONFIG_INACTIVE,HCI_DBG_SET_BT_SETTING_CMD_OPCODE,sizeof(bt_setting_1400),bt_setting_1400},
    {BTDRV_CONFIG_INACTIVE,HCI_DBG_SET_BT_SETTING_CMD_OPCODE,sizeof(bt_setting_1400_1),bt_setting_1400_1},
    {BTDRV_CONFIG_ACTIVE,  HCI_DBG_SET_SLEEP_SETTING_CMD_OPCODE,sizeof(sleep_param),sleep_param},
    //{BTDRV_CONFIG_INACTIVE,HCI_DBG_SET_BD_ADDR_CMD_OPCODE,sizeof(bt_addr),bt_addr},
    {BTDRV_CONFIG_INACTIVE,HCI_DBG_SET_CUSTOM_PARAM_CMD_OPCODE,sizeof(btdrv_rf_env),(uint8_t *)&btdrv_rf_env},
    {BTDRV_CONFIG_INACTIVE,HCI_DBG_SET_CUSTOM_PARAM_CMD_OPCODE,sizeof(btdrv_rf_env_1),(uint8_t *)&btdrv_rf_env_1},
    //{BTDRV_CONFIG_INACTIVE,HCI_DBG_SET_CUSTOM_PARAM_CMD_OPCODE,sizeof(btdrv_bt_spi_cntl),(uint8_t *)&btdrv_bt_spi_cntl},
    //{BTDRV_CONFIG_INACTIVE,HCI_DBG_SET_CUSTOM_PARAM_CMD_OPCODE,sizeof(btdrv_ble_spi_cntl),(uint8_t *)&btdrv_ble_spi_cntl},
    //{BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_CUSTOM_PARAM_CMD_OPCODE,sizeof(btdrv_afh_env),(uint8_t *)&btdrv_afh_env},
    //{BTDRV_CONFIG_INACTIVE,HCI_DBG_SET_LPCLK_DRIFT_JITTER_CMD_OPCODE,sizeof(lpclk_drift_jitter),(uint8_t *)&lpclk_drift_jitter},
    //{BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_WAKEUP_TIME_CMD_OPCODE,sizeof(wakeup_timing),(uint8_t *)&wakeup_timing},
#ifdef _SCO_BTPCM_CHANNEL_
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_SYNC_CONFIG_CMD_OPCODE,sizeof(sync_config),(uint8_t *)&sync_config},
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_PCM_SETTING_CMD_OPCODE,sizeof(pcm_setting),(uint8_t *)&pcm_setting},
#endif
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_RSSI_THRHLD_CMD_OPCODE,sizeof(bt_rssi_thr),(uint8_t *)&bt_rssi_thr},
    //{BTDRV_CONFIG_INACTIVE,HCI_DBG_SET_LOCAL_EX_FEATURE_CMD_OPCODE,sizeof(local_ex_feature_page1),(uint8_t *)&local_ex_feature_page1},
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_LOCAL_EX_FEATURE_CMD_OPCODE,sizeof(local_ex_feature_page2),(uint8_t *)&local_ex_feature_page2},
    //{BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_BT_RF_TIMING_CMD_OPCODE,sizeof(bt_rf_timing),(uint8_t *)&bt_rf_timing},
    //{BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_BLE_RF_TIMING_CMD_OPCODE,sizeof(ble_rf_timing),(uint8_t *)&ble_rf_timing},
    //{BTDRV_CONFIG_INACTIVE,HCI_DBG_SET_RL_SIZE_CMD_OPCODE,sizeof(ble_rl_size),(uint8_t *)&ble_rl_size},

    //{BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_EDR_THRESHOLD_CMD_OPCODE,sizeof(bt_edr_thr),(uint8_t *)&bt_edr_thr},
    //{BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_EDR_ALGORITHM_CMD_OPCODE,sizeof(bt_edr_algo),(uint8_t *)&bt_edr_algo},
    //{BTDRV_CONFIG_ACTIVE,HCI_DBG_WR_DLE_DFT_VALUE_CMD_OPCODE,sizeof(ble_dle_dft_value),(uint8_t *)&ble_dle_dft_value},
    //{BTDRV_CONFIG_ACTIVE,HCI_DBG_LMP_MESSAGE_RECORD_CMD_OPCODE,sizeof(bt_lmp_record),(uint8_t *)&bt_lmp_record},
};

const uint32_t mem_config_1400[][2] =
{
};

//2019/03/18,Move 1400 digital config from tws_master-ibrt
#if defined(IBRT)

void btdrv_digital_config_init(void) /* 1400 IBRT config */
{
    BTDIGITAL_REG(0x40000048)=0xc000860b;
    BTDIGITAL_REG(0x40000010)=0x4000866b;

    if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0){
        BTDIGITAL_REG(0xd0350300)=0x44;
        BTDIGITAL_REG(0xd0350308)=0x300c;//basic digital gain
        BTDIGITAL_REG(0xd0350340)=0x04;
        BTDIGITAL_REG(0xd0350344)=0x0d;//edr digital gain
        BTDIGITAL_REG(0xd0350348)=0x003f003f;//digitial offset
        BTDIGITAL_REG(0xd0350240) = 0x0001A003;//Bw2m
        BTDIGITAL_REG(0xd03502c8) = 0x00000080;//dfd_kfactor
        BTDIGITAL_REG(0xd03502cc) = 0x00000015;//coc_kfactor
        BTDIGITAL_REG(0xd03503a0)=0x1c070051;
        BTDIGITAL_REG(0xd03503a0) |= 0x1<<2;
    }else if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1){
        BTDIGITAL_REG(0xd0350300)=0x44;
        BTDIGITAL_REG(0xd0350308)=0x3c0f;//basic digital gain
        BTDIGITAL_REG(0xd0350340)=0x04;
        BTDIGITAL_REG(0xd0350344)=0x0f;//edr digital gain
        BTDIGITAL_REG(0xd0350348)=0x00000000;//digitial offset
        BTDIGITAL_REG(0xd0220468) |= (1<<22);
        BTDIGITAL_REG(0xD035020c)=0x00f1002c;
        BTDIGITAL_REG(0xD0350210)=0x00f1002c;
        //[OPT] by li hua
        BTDIGITAL_REG(0xd0350240)=0x0001a00b;//bw2m
        BTDIGITAL_REG(0xd03502c8)=0x00000080;//dfd_kfactor
        BTDIGITAL_REG(0xd03502cc)=0x00000015;//coc_kfactor
        BTDIGITAL_REG(0xd03503a4)=0x00000020;//corr_th
        BTDIGITAL_REG(0xd03503a0)=0x1c070045;//new corr en
        BTDIGITAL_REG(0xd0350364)=0x2baeb908;//lau_gain
        BTDIGITAL_REG(0xd0350374)=0x00601f00;//c1c2

        BTDIGITAL_REG(0xd022046c)&=~(1<<31);//enable DM1 empty fastack
    }

    BTDIGITAL_REG(0xd0350360)=0x003fe040;
    BTDIGITAL_REG(0xd0220080)=0x0e421340;
    BTDIGITAL_REG(0xd0340000)=0x23620200;//tport related
    BTDIGITAL_REG(0xd0220050)=0xb1b1;

    if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1)
    {
        BTDIGITAL_REG(0xd02204a8) |= (1 << 5);
        btdrv_delay(10);
    }
}

#else
void btdrv_digital_config_init(void) /* 1400 non-ibrt config */
{
    BTDIGITAL_REG(0x40000048)=0xc000860b;
    BTDIGITAL_REG(0x40000010)=0x4000866b;

    if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0){
        BTDIGITAL_REG(0xd0350300)=0x44;
        BTDIGITAL_REG(0xd0350308)=0x300c;//basic digital gain
        BTDIGITAL_REG(0xd0350340)=0x04;
        BTDIGITAL_REG(0xd0350344)=0x0d;//edr digital gain
        BTDIGITAL_REG(0xd0350348)=0x003f003f;//digitial offset
    }else if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1){
        BTDIGITAL_REG(0xd0350300)=0x44;
        BTDIGITAL_REG(0xd0350308)=0x3c0f;//basic digital gain
        BTDIGITAL_REG(0xd0350340)=0x04;
        BTDIGITAL_REG(0xd0350344)=0x0f;//edr digital gain
        BTDIGITAL_REG(0xd0350348)=0x00000000;//digitial offset

        BTDIGITAL_REG(0xd03502c4) = 0x003f02ef; // jiangpeng - te_timeinit
        BTDIGITAL_REG(0xd0350280) = 0x00000006; // jiangpeng - psd_avgen le_dfe_forceraw
        BTDIGITAL_REG(0xd0350284) = 0x02000200; // jiangpeng - gfo_p2pthr_le
        BTDIGITAL_REG(0xd0220280) = 0x11421456; // jiangpeng - enlarge tx edge close time for tx cmpl
    }

    BTDIGITAL_REG(0xd0350360)=0x003fe040;
    BTDIGITAL_REG(0xd0220080)=0x0e421340;
    BTDIGITAL_REG(0xd03503a0)=0x1c070051;

    BTDIGITAL_REG(0xd0340000)=0x23620200;//tport related
    BTDIGITAL_REG(0xd0220050)=0xb1b1;

    BTDIGITAL_REG(0xd0350240) = 0x0001A003;//Bw2m
    btdrv_delay(10);
    BTDIGITAL_REG(0xd03502c8) = 0x00000080;//dfd_kfactor
    btdrv_delay(10);
    BTDIGITAL_REG(0xd03502cc) = 0x00000015;//coc_kfactor
    btdrv_delay(10);
    BTDIGITAL_REG(0xd03503a0) |= 0x1<<2;
    btdrv_delay(10);

    if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1) {
        BTDIGITAL_REG(0xd02204a8) |= (1 << 5);
        btdrv_delay(10);
    }
}
#endif

void btdrv_config_init(void)
{
    TRACE("%s",__func__);

    for(uint8_t i=0; i<sizeof(btdrv_cfg_tbl)/sizeof(btdrv_cfg_tbl[0]); i++)
    {
        if (btdrv_cfg_tbl[i].opcode == HCI_DBG_SET_BT_SETTING_CMD_OPCODE)
        {
            if ((hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0 && btdrv_cfg_tbl[i].parlen == sizeof(bt_setting_1400)) ||
                (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1 && btdrv_cfg_tbl[i].parlen == sizeof(bt_setting_1400_1)))
            {
                btdrv_send_cmd(btdrv_cfg_tbl[i].opcode,btdrv_cfg_tbl[i].parlen,btdrv_cfg_tbl[i].param);
                btdrv_delay(1);
            }
        }
        if (btdrv_cfg_tbl[i].opcode == HCI_DBG_SET_CUSTOM_PARAM_CMD_OPCODE)
        {
            if ((hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0 && btdrv_cfg_tbl[i].parlen == sizeof(btdrv_rf_env)) ||
                (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1 && btdrv_cfg_tbl[i].parlen == sizeof(btdrv_rf_env_1)))
            {
                btdrv_send_cmd(btdrv_cfg_tbl[i].opcode,btdrv_cfg_tbl[i].parlen,btdrv_cfg_tbl[i].param);
                btdrv_delay(1);
                //btdrv_1401_set_tports_level(3);
            }
        }
        //BT other config
        if(btdrv_cfg_tbl[i].is_act == BTDRV_CONFIG_ACTIVE)
        {
            btdrv_send_cmd(btdrv_cfg_tbl[i].opcode,btdrv_cfg_tbl[i].parlen,btdrv_cfg_tbl[i].param);
            btdrv_delay(1);
        }
    }

    //BT registers config
    for(uint8_t i=0; i<sizeof(mem_config_1400)/sizeof(mem_config_1400[0]); i++)
    {
        btdrv_write_memory(_32_Bit,mem_config_1400[i][0],(uint8_t *)&mem_config_1400[i][1],4);
        btdrv_delay(1);
    }

    // init BT digital config
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0)
    {

        btdrv_digital_config_init();
    }
}


////////////////////////////////////////test mode////////////////////////////////////////////
uint8_t testmode_local_feature[] =
{
    0xBF,0xFE,0xCD,0xFE,0xDB,0xFd,0x7b,0x87
};

const static BTDRV_CFG_TBL_STRUCT  btdrv_testmode_cfg_tbl[] =
{
    {BTDRV_CONFIG_ACTIVE,HCI_DBG_SET_LOCAL_FEATURE_CMD_OPCODE,sizeof(testmode_local_feature),testmode_local_feature},
};

void btdrv_testmode_config_init(void)
{
    return;

    for(uint8_t i=0; i<sizeof(btdrv_testmode_cfg_tbl)/sizeof(btdrv_testmode_cfg_tbl[0]); i++)
    {
        if(btdrv_testmode_cfg_tbl[i].is_act == BTDRV_CONFIG_ACTIVE)
        {
            btdrv_send_cmd(btdrv_testmode_cfg_tbl[i].opcode,btdrv_testmode_cfg_tbl[i].parlen,btdrv_testmode_cfg_tbl[i].param);
            btdrv_delay(1);
        }
    }
}

void btdrv_sleep_config(uint8_t sleep_en)
{
    sleep_param[0] = sleep_en;
    btdrv_send_cmd(HCI_DBG_SET_SLEEP_SETTING_CMD_OPCODE,8,sleep_param);
    btdrv_delay(1);
}

void btdrv_feature_default(void)
{
    uint8_t feature[] = {0xBF, 0xeE, 0xCD,0xFe,0xc3,0xFf,0x7b,0x87};
    btdrv_send_cmd(HCI_DBG_SET_LOCAL_FEATURE_CMD_OPCODE,8,feature);
    btdrv_delay(1);
}

uint8_t test_mode_addr[6] = {0x77,0x77,0x77,0x77,0x77,0x77};
void btdrv_test_mode_addr_set(void)
{
    return;

    btdrv_send_cmd(HCI_DBG_SET_BD_ADDR_CMD_OPCODE,sizeof(test_mode_addr),test_mode_addr);
    btdrv_delay(1);
}

#define REG_BT_EM_CS_BASE_ADDR 0xD0210000
#define BT_BTSPIPOSBUF_COUNT  632
#define BT_BTSPINEGBUF_COUNT  632
#define BT_BLESPIPOSBUF_COUNT  320


#define EM_BT_SPIPOSBUF_OFFSET (0x00006D14)
#define EM_BT_SPINEGBUF_OFFSET (EM_BT_SPIPOSBUF_OFFSET+(BT_BTSPIPOSBUF_COUNT<<1))
#define EM_BLE_SPIPOSBUF_OFFSET (EM_BT_SPINEGBUF_OFFSET+(BT_BTSPINEGBUF_COUNT<<1))
#define EM_BLE_SPINEGBUF_OFFSET (EM_BLE_SPIPOSBUF_OFFSET+(BT_BLESPIPOSBUF_COUNT<<1))

#define EM_BT_SPIPOSBUF_ADRESS_BASE (REG_BT_EM_CS_BASE_ADDR + EM_BT_SPIPOSBUF_OFFSET)
#define EM_BT_SPINEGBUF_ADRESS_BASE (REG_BT_EM_CS_BASE_ADDR + EM_BT_SPINEGBUF_OFFSET)
#define EM_BLE_SPIPOSBUF_ADRESS_BASE (REG_BT_EM_CS_BASE_ADDR + EM_BLE_SPIPOSBUF_OFFSET)
#define EM_BLE_SPINEGBUF_ADRESS_BASE (REG_BT_EM_CS_BASE_ADDR + EM_BLE_SPINEGBUF_OFFSET)



#define BT_RFSPI_WR (0<<7)
#define BT_RFSPI_RD (1<<7)
#define BT_RFSPI_SETFREQ (1<<6)

#define BT_RFSPI_FRAME(n) (((n-1)&0xf)<<2)
#define BT_RFSPI_BYTEPREFRAME(n) (((n-2)&0x3)<<0)


#define pos_frame  3
#define neg_frame  (3 + pos_frame)

uint8_t btdrv_bt_spi_pos_raw[]=
{
    (BT_RFSPI_WR|BT_RFSPI_FRAME(pos_frame)|BT_RFSPI_BYTEPREFRAME(3)),
    0x09,0x16,0x00,//sel ee
    0x09,0x15,0x00,//sel ed
    0x09,0x00,0x00,//back to agc control
};

const uint8_t btdrv_bt_spi_neg_raw[]=
{
    (BT_RFSPI_WR|BT_RFSPI_FRAME(neg_frame)|BT_RFSPI_BYTEPREFRAME(3)),
    0x09,0x15,0x00,//sel ed
    0x09,0x16,0x00,//sel ee
    0x09,0x17,0x00,//sel ef
};

const uint8_t btdrv_bt_spi_bitoffset_and_btclock_raw[]=
{
    (BT_RFSPI_WR|BT_RFSPI_FRAME(1)|BT_RFSPI_BYTEPREFRAME(1)),
    0xc2,0x79,0x00,
};

const uint8_t btdrv_ble_spi_pos_raw[16]=
{
    (BT_RFSPI_WR|BT_RFSPI_FRAME(3)|BT_RFSPI_BYTEPREFRAME(3)),
    0x18,0x00,0x7f,
    0x19,0x3C,0x40,
    0x1a,0x00,0x00,
};

const uint8_t btdrv_ble_spi_neg_raw[16]=
{
    (BT_RFSPI_WR|BT_RFSPI_FRAME(3)|BT_RFSPI_BYTEPREFRAME(3)),
    0x18,0x00,0x6c,
    0x19,0x24,0x40,
    0x1a,0x00,0x10,
};

void btdrv_bt_spi_setneg_rawbuf(uint8_t wr_type, const uint8_t *spirawbuf, uint8_t pos_len,uint8_t neg_len)
{
    if (1 == wr_type)
        btdrv_write_memory(_8_Bit, EM_BT_SPIPOSBUF_ADRESS_BASE, spirawbuf, neg_len);

    if (2 == wr_type)
        btdrv_write_memory(_8_Bit, (EM_BT_SPINEGBUF_ADRESS_BASE + pos_len), spirawbuf, neg_len);

    if (3 == wr_type)
        btdrv_write_memory(_8_Bit, EM_BLE_SPIPOSBUF_ADRESS_BASE, spirawbuf, neg_len);

    if (4 == wr_type)
        btdrv_write_memory(_8_Bit, EM_BLE_SPINEGBUF_ADRESS_BASE, spirawbuf, neg_len);
}

void btdrv_bt_spi_setpos_rawbuf(uint8_t wr_type, const uint8_t *spirawbuf, uint8_t pos_len)
{
    if (1 == wr_type)
        btdrv_write_memory(_8_Bit, EM_BT_SPIPOSBUF_ADRESS_BASE, spirawbuf, pos_len);

    if (2 == wr_type)
        btdrv_write_memory(_8_Bit, EM_BT_SPINEGBUF_ADRESS_BASE, spirawbuf, pos_len);

    if (3 == wr_type)
        btdrv_write_memory(_8_Bit, EM_BLE_SPIPOSBUF_ADRESS_BASE, spirawbuf, pos_len);

    if (4 == wr_type)
        btdrv_write_memory(_8_Bit, EM_BLE_SPINEGBUF_ADRESS_BASE, spirawbuf, pos_len);
}

#define SPI_TRIG_NEG_BUF_PTR_ADDR 0xD0220078
void btdrv_rf_set_spi_neg_buff_ptr(void)
{
    BTDIGITAL_REG(SPI_TRIG_NEG_BUF_PTR_ADDR) = (BTDIGITAL_REG(SPI_TRIG_NEG_BUF_PTR_ADDR)&0xffff) | (0x7204<<16);
}

void btdrv_bt_spi_xtal_init(void)
{
    return;

    //TRACE("%s\n", __func__);
    //btdrv_bt_spi_set_rawbuf(1, (uint8_t *)btdrv_bt_spi_pos_raw, sizeof(btdrv_bt_spi_pos_raw));
    //btdrv_bt_spi_set_rawbuf(2, (uint8_t *)btdrv_bt_spi_neg_raw, sizeof(btdrv_bt_spi_neg_raw));
    //btdrv_bt_spi_set_rawbuf(3, (uint8_t *)btdrv_ble_spi_pos_raw, sizeof(btdrv_ble_spi_pos_raw));
    //btdrv_bt_spi_set_rawbuf(4, (uint8_t *)btdrv_ble_spi_neg_raw, sizeof(btdrv_ble_spi_neg_raw));

    btdrv_bt_spi_setneg_rawbuf(2, btdrv_bt_spi_bitoffset_and_btclock_raw, 0,sizeof(btdrv_bt_spi_bitoffset_and_btclock_raw));
    btdrv_rf_set_spi_neg_buff_ptr();
    //btdrv_send_cmd(HCI_DBG_SET_CUSTOM_PARAM_CMD_OPCODE,sizeof(btdrv_bt_spi_cntl), (uint8_t *)&btdrv_bt_spi_cntl);
}

void btdrv_bt_spi_rawbuf_init(void)
{
    return;

    BTDIGITAL_REG(0xd02201c8)=0x348C6442; // enable spi high speed
    //TRACE("%s\n", __func__);
    btdrv_bt_spi_setpos_rawbuf(1, btdrv_bt_spi_pos_raw, sizeof(btdrv_bt_spi_pos_raw));
    btdrv_bt_spi_setneg_rawbuf(2, btdrv_bt_spi_neg_raw, (sizeof(btdrv_bt_spi_pos_raw)),sizeof(btdrv_bt_spi_neg_raw));

    btdrv_send_cmd(HCI_DBG_SET_CUSTOM_PARAM_CMD_OPCODE,sizeof(btdrv_bt_spi_cntl), (uint8_t *)&btdrv_bt_spi_cntl);
    *(volatile uint32_t *)(0xD0220074) = 0x26d14;    //spi ptr
    *(volatile uint32_t *)(0xD0220078) = 0x72040420; //spi neg ptr

    btdrv_delay(10);
}

uint8_t meInit_param_get_entry_idx = 0;

int btdrv_meinit_param_init(void)
{
    int size = 0;
    if ((me_init_param[0][0] == 0xffffffff) &&
        (me_init_param[0][1] == 0xffffffff))
    {
        size = -1;
    }
    meInit_param_get_entry_idx = 0;
    return size;
}

int btdrv_meinit_param_remain_size_get(void)
{
    int remain_size;
    if ((me_init_param[0][0] == 0xffffffff) &&
        (me_init_param[0][1] == 0xffffffff))
    {
        return -1;
    }
    remain_size = sizeof(me_init_param)/sizeof(me_init_param[0]) - meInit_param_get_entry_idx;
    return remain_size;
}

int btdrv_meinit_param_next_entry_get(uint32_t *addr, uint32_t *val)
{
    if (meInit_param_get_entry_idx > (sizeof(me_init_param)/sizeof(me_init_param[0])-1))
        return -1;
    *addr = me_init_param[meInit_param_get_entry_idx][0];
    *val = me_init_param[meInit_param_get_entry_idx][1];
    meInit_param_get_entry_idx++;
    return 0;
}

enum
{
    SYNC_IDLE,
    SYNC_64_ORG,
    SYNC_68_ORG,
    SYNC_72_ORG,
    SYNC_64_NEW,
    SYNC_68_NEW,
    SYNC_72_NEW,
};

enum
{
    SYNC_64_BIT,
    SYNC_68_BIT,
    SYNC_72_BIT,
};

uint32_t bt_sync_type = SYNC_IDLE;
void btdrv_sync_config(void)
{
    uint32_t corr_mode = BTDIGITAL_REG(0xd0220460);
    uint32_t dfe_mode = BTDIGITAL_REG(0xd0350360);
    uint32_t timeinit = BTDIGITAL_REG(0xd03502c4);
    if(bt_sync_type == SYNC_IDLE)
        return;

    corr_mode = (corr_mode & 0xfffffff8);//bit2: enh dfe; [1:0]: bt_corr_mode
    dfe_mode = (dfe_mode & 0xffffffe0);//bit4: dfe_header_mode_bt; [3:0]: dfe_delay_cycle
    timeinit = (timeinit & 0xfffff800);//[10:0]: tetimeinit value

    switch(bt_sync_type)
    {
        case SYNC_64_ORG:
            corr_mode |= 0x0;
            dfe_mode |= 0x00;
            timeinit |= 0x2eb;
            break;
        case SYNC_68_ORG:
            corr_mode |= 0x1;
            dfe_mode |= 0x00;
            timeinit |= 0x2eb;
            break;
        case  SYNC_72_ORG:
            corr_mode |= 0x2;
            dfe_mode |= 0x00;
            timeinit |= 0x2b7;
            break;
        case SYNC_64_NEW:
            corr_mode |= 0x4;
            dfe_mode |= 0x14;
            timeinit |= 0x2eb;
            break;
        case SYNC_68_NEW:
            corr_mode |= 0x5;
            dfe_mode |= 0x14;
            timeinit |= 0x2eb;
            break;
        case  SYNC_72_NEW:
            corr_mode |= 0x6;
            dfe_mode |= 0x10;
            timeinit |= 0x2b7;
            break;
    }
    BTDIGITAL_REG(0xd0220460) = corr_mode;
    BTDIGITAL_REG(0xd0350360) = dfe_mode;
    BTDIGITAL_REG(0xd03502c4) = timeinit;
}


static bool bt_slave_role;

void btdrv_store_device_role(bool slave)
{
    bt_slave_role = slave;
}

bool btdrv_device_role_is_slave(void)
{
    return bt_slave_role;
}

#define REG_SPI_TRIG_SELECT_LINK0_ADDR EM_BT_BT_EXT1_ADDR        //114a+66
#define REG_SPI_TRIG_SELECT_LINK1_ADDR (EM_BT_BT_EXT1_ADDR+BT_EM_SIZE) //11b8+66
#define REG_SPI_TRIG_NUM_ADDR 0xd0220400
#define REG_SPI0_TRIG_POS_ADDR 0xd0220454
#define REG_SPI1_TRIG_POS_ADDR 0xd0220458

struct SPI_TRIG_NUM_T
{
    uint32_t spi0_txon_num:3;//spi0 number of tx rising edge
    uint32_t spi0_txoff_num:3;//spi0 number of tx falling edge
    uint32_t spi0_rxon_num:2;//spi0 number of rx rising edge
    uint32_t spi0_rxoff_num:2;//spi0 number of rx falling edge
    uint32_t spi0_fast_mode:1;
    uint32_t spi0_gap:4;
    uint32_t hwspi_en:1;
    uint32_t spi1_txon_num:3;//spi1 number of tx rising edge
    uint32_t spi1_txoff_num:3;//spi1 number of tx falling edge
    uint32_t spi1_rxon_num:2;//spi1 number of rx rising edge
    uint32_t spi1_rxoff_num:2;//spi1 number of rx falling edge
    uint32_t spi1_fast_mode:1;
    uint32_t spi1_gap:4;
    uint32_t no_use:1;
};

struct SPI_TRIG_POS_T
{
    uint32_t spi_txon_pos:7;
    uint32_t spi_txoff_pos:9;
    uint32_t spi_rxon_pos:7;
    uint32_t spi_rxoff_pos:9;
};

uint32_t spi0_trig_data_tbl[][2] =
{
    //{addr,data([23:0])}
    {0xd0220404,0x8080e9},//spi0_trig_txdata1
    {0xd0220408,0x000000},//spi0_trig_txdata2
    {0xd022040c,0x000000},//spi0_trig_txdata3
    {0xd0220410,0x000000},//spi0_trig_txdata4
    {0xd022041c,0x000000},//spi0_trig_rxdata1
    {0xd0220420,0x000000},//spi0_trig_rxdata2
    {0xd0220424,0x000000},//spi0_trig_rxdata3
    {0xd0220428,0x000000},//spi0_trig_rxdata4
    {0xd0220414,0x000000},//spi0_trig_trxdata5
    {0xd0220418,0x000000},//spi0_trig_trxdata6
};

uint32_t spi1_trig_data_tbl[][2] =
{
    //{addr,data([23:0])}
    {0xd022042c,0x000000},//spi1_trig_txdata1
    {0xd0220430,0x000000},//spi1_trig_txdata2
    {0xd0220434,0x000000},//spi1_trig_txdata3
    {0xd0220438,0x000000},//spi1_trig_txdata4
    {0xd0220444,0x000000},//spi1_trig_rxdata1
    {0xd0220448,0x000000},//spi1_trig_rxdata2
    {0xd022044c,0x000000},//spi1_trig_rxdata3
    {0xd0220450,0x000000},//spi1_trig_rxdata4
    {0xd022043c,0x000000},//spi1_trig_trxdata5
    {0xd0220440,0x000000},//spi1_trig_trxdata6
};

void btdrv_spi_trig_data_change(uint8_t spi_sel, uint8_t index, uint32_t value)
{
    if(!spi_sel)
    {
        spi0_trig_data_tbl[index][1] = value&0xFFFFFF;
    }
    else
    {
        spi1_trig_data_tbl[index][1] = value&0xFFFFFF;
    }
}

void btdrv_spi_trig_data_set(uint8_t spi_sel)
{
    if(!spi_sel)
    {
        for(uint8_t i=0; i< sizeof(spi0_trig_data_tbl)/sizeof(spi0_trig_data_tbl[0]); i++)
        {
            BTDIGITAL_REG(spi0_trig_data_tbl[i][0]) = spi0_trig_data_tbl[i][1];
        }
    }
    else
    {
        for(uint8_t i=0; i< sizeof(spi1_trig_data_tbl)/sizeof(spi1_trig_data_tbl[0]); i++)
        {
            BTDIGITAL_REG(spi1_trig_data_tbl[i][0]) = spi1_trig_data_tbl[i][1];
        }
    }
}

void btdrv_spi_trig_num_set(uint8_t spi_sel, struct SPI_TRIG_NUM_T *spi_trig_num)
{
    uint8_t tx_onoff_total_num;
    uint8_t rx_onoff_total_num;

    if(!spi_sel)
    {
        tx_onoff_total_num = spi_trig_num->spi0_txon_num + spi_trig_num->spi0_txoff_num;
        rx_onoff_total_num = spi_trig_num->spi0_rxon_num + spi_trig_num->spi0_rxoff_num;
    }
    else
    {
        tx_onoff_total_num = spi_trig_num->spi1_txon_num + spi_trig_num->spi1_txoff_num;
        rx_onoff_total_num = spi_trig_num->spi1_rxon_num + spi_trig_num->spi1_rxoff_num;
    }
    ASSERT((tx_onoff_total_num <= 6), "spi trig tx_onoff_total_num>6");
    ASSERT((rx_onoff_total_num <= 6), "spi trig rx_onoff_total_num>6");

    BTDIGITAL_REG(REG_SPI_TRIG_NUM_ADDR) = *(uint32_t *)spi_trig_num;
}

void btdrv_spi_trig_pos_set(uint8_t spi_sel, struct SPI_TRIG_POS_T *spi_trig_pos)
{
    if(!spi_sel)
    {
        BTDIGITAL_REG(REG_SPI0_TRIG_POS_ADDR) = *(uint32_t *)spi_trig_pos;
    }
    else
    {
        BTDIGITAL_REG(REG_SPI1_TRIG_POS_ADDR) = *(uint32_t *)spi_trig_pos;
    }
}

void btdrv_spi_trig_init(void)
{
    struct SPI_TRIG_NUM_T spi_trig_num;
    struct SPI_TRIG_POS_T spi0_trig_pos;
    struct SPI_TRIG_POS_T spi1_trig_pos;

    spi_trig_num.spi0_txon_num = 0;
    spi_trig_num.spi0_txoff_num = 1;
    spi_trig_num.spi0_rxon_num = 0;
    spi_trig_num.spi0_rxoff_num = 0;
    spi_trig_num.spi0_fast_mode = 0;
    spi_trig_num.spi0_gap = 4;
    spi_trig_num.hwspi_en = 0;

    spi_trig_num.spi1_txon_num = 0;
    spi_trig_num.spi1_txoff_num = 0;
    spi_trig_num.spi1_rxon_num = 0;
    spi_trig_num.spi1_rxoff_num = 0;
    spi_trig_num.spi1_fast_mode = 0;
    spi_trig_num.spi1_gap = 0;
    spi_trig_num.no_use = 0;

    btdrv_spi_trig_num_set(0,&spi_trig_num);
    btdrv_spi_trig_num_set(1,&spi_trig_num);

    spi0_trig_pos.spi_txon_pos = 0;
    spi0_trig_pos.spi_txoff_pos = 20;
    spi0_trig_pos.spi_rxon_pos = 0;
    spi0_trig_pos.spi_rxoff_pos = 0;

    spi1_trig_pos.spi_txon_pos = 0;
    spi1_trig_pos.spi_txoff_pos = 0;
    spi1_trig_pos.spi_rxon_pos = 0;
    spi1_trig_pos.spi_rxoff_pos = 0;

    btdrv_spi_trig_pos_set(0,&spi0_trig_pos);
    btdrv_spi_trig_pos_set(1,&spi1_trig_pos);

    btdrv_spi_trig_data_set(0);
    btdrv_spi_trig_data_set(1);
}

void btdrv_spi_trig_select(uint8_t link_id, bool spi1_set)
{
    if(spi1_set)
    {
        if(!link_id)
        {
            BTDIGITAL_REG(REG_SPI_TRIG_SELECT_LINK0_ADDR) |= 0x00004000;
        }
        else
        {
            BTDIGITAL_REG(REG_SPI_TRIG_SELECT_LINK1_ADDR) |= 0x00004000;
        }
    }
}

uint8_t btdrv_get_spi_trig_enable(void)
{
    return ((BTDIGITAL_REG(REG_SPI_TRIG_NUM_ADDR)&0x8000)>>15);
}

void btdrv_set_spi_trig_enable(void)
{
    BTDIGITAL_REG(REG_SPI_TRIG_NUM_ADDR)|= 0x8000;
}

void btdrv_clear_spi_trig_enable(void)
{
    BTDIGITAL_REG(REG_SPI_TRIG_NUM_ADDR)&= ~0x8000;
}

