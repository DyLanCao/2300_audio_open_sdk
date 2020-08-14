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
#ifndef __BT_DRV_REG_OP_H__
#define  __BT_DRV_REG_OP_H__

#include <stdio.h>
#include "stdint.h"
#include "stdbool.h"
#include "plat_types.h"


#ifdef __cplusplus
extern "C" {
#endif

struct ld_afh_env
{
/// RSSI threshold for interference detection (in dBm)
    int8_t rf_rssi_interf_thr;
/// AFH update period (in sec)
    uint8_t afh_update_period;
/// Maximum duration without receiving a packet before considering a channel quality unknown  (in slots) (2 secs)
    uint16_t afh_access_valid_to;
/// Maximum duration without receiving a packet before reintroducing a channel to the map (in slots) (20 secs)
    uint16_t afh_reaccess_to;
/// Maximum counter value for channel assessment
    int8_t afh_access_count_max;
/// Counter threshold to consider a channel good
    int8_t afh_access_count_thr_good;
/// Counter threshold to consider a channel bad
    int8_t afh_access_count_thr_bad;    
/// Minimum counter value for channel assessment
    int8_t afh_access_count_min;
};

/// RSSI threshold for interference detection (in dBm)
#define BT_AFH_RSSI_INTERF_THR       (-70)

/// AFH update period (in sec)
#define BT_AFH_UPDATE_PERIOD         4

/// Maximum duration without receiving a packet before considering a channel quality unknown  (in slots) (2 secs)
#define BT_AFH_ASSESS_VALID_TO       0x1900

/// Maximum duration without receiving a packet before reintroducing a channel to the map (in slots) (20 secs)
#define BT_AFH_REASSESS_TO           0x7D00

/// Maximum counter value for channel assessment
#define BT_AFH_ASSESS_COUNT_MAX      4
/// Counter threshold to consider a channel good
#define BT_AFH_ASSESS_COUNT_THR_GOOD 2
/// Counter threshold to consider a channel bad
#define BT_AFH_ASSESS_COUNT_THR_BAD  -2
/// Minimum counter value for channel assessment
#define BT_AFH_ASSESS_COUNT_MIN      -4

#define RETX_NB_0                 0
#define RETX_NB_1                 1
#define RETX_NB_2                 2
#define RETX_NEGO                 3
#ifdef CHIP_BEST2000
#define MIC_BUFF_ADRR_REG        (0xc0000040)
#define RX_BUFF_ADRR        (0xc0000044)
#elif CHIP_BEST1400
#define MIC_BUFF_ADRR_REG        (0xc0000058)
#define RX_BUFF_ADRR        (0xc000005c)
#elif CHIP_BEST2300P
#define MIC_BUFF_ADRR_REG        (0xc0000058)
#define RX_BUFF_ADRR        (0xc000005c)
#endif

//BT acl tx silence
#define TX_SILENCE_ON                   1
#define TX_SILENCE_OFF                  0

void bt_drv_reg_op_rssi_set(uint16_t rssi);
void bt_drv_reg_op_scan_intv_set(uint32_t scan_intv);
void bt_drv_reg_op_encryptchange_errcode_reset(uint16_t hci_handle);
void bt_drv_reg_op_sco_sniffer_checker(void);
void bt_drv_reg_op_trigger_time_checker(void);
void bt_drv_reg_op_tws_output_power_fix_separate(uint16_t hci_handle, uint16_t pwr);
bool bt_drv_reg_op_ld_sniffer_env_monitored_dev_state_get(void);
void bt_drv_reg_op_ld_sniffer_env_monitored_dev_state_set(bool state);
void bt_drv_reg_op_ld_sniffer_master_addr_set(uint8_t * addr);
int bt_drv_reg_op_currentfreeaclbuf_get(void);
void bt_drv_reg_op_save_mobile_airpath_info(uint16_t hciHandle);
void bt_drv_reg_op_block_xfer_with_mobile(uint16_t hciHandle);
void bt_drv_reg_op_resume_xfer_with_mobile(uint16_t hciHandle);
int bt_drv_reg_op_packet_type_checker(uint16_t hciHandle);
void bt_drv_reg_op_max_slot_setting_checker(uint16_t hciHandle);
void bt_drv_reg_op_force_task_dbg_idle(void);
void bt_drv_reg_op_afh_follow_mobile_mobileidx_set(uint16_t hciHandle);
void bt_drv_reg_op_afh_follow_mobile_twsidx_set(uint16_t hciHandle);
void bt_drv_reg_op_sco_status_store(void);
void bt_drv_reg_op_sco_status_restore(void);
bool bt_drv_reg_op_sco_tx_buf_restore(uint8_t *trigger_test);
void bt_drv_reg_op_afh_bak_reset(void);
void bt_drv_reg_op_afh_bak_save(uint8_t role, uint16_t mobileHciHandle);
void bt_drv_reg_op_connection_checker(void);
void bt_drv_reg_op_bt_info_checker(void);
void bt_drv_reg_op_bt_packet_checker(void);
void bt_drv_reg_op_ble_buffer_cleanup(void);
void bt_drv_reg_op_crash_dump(void);
void bt_drv_reg_op_set_tx_pwr(uint16_t connHandle, uint8_t txpwr);
uint8_t bt_drv_reg_op_get_tx_pwr(uint16_t connHandle);
void bt_drv_reg_op_fix_tx_pwr(uint16_t connHandle);
void bt_drv_reg_op_lsto_hack(uint8_t link_id);
void bt_drv_reg_op_enable_emsack_mode(uint16_t connHandle, uint8_t master, uint8_t enable);
void bt_drv_reg_op_set_accessible_mode(uint8_t mode);
void bt_drv_reg_op_force_retrans(bool enalbe);
void bt_drv_reg_enable_pcm_tx_hw_cal(void);
void bt_drv_reg_monitor_clk(void);
int8_t bt_drv_read_rssi_in_dbm(uint16_t connHandle);
void bt_drv_reg_op_set_swagc_mode(uint8_t mode);
void bt_drv_reg_op_set_reboot_pairing_mode(uint8_t mode);

void bt_drv_reg_op_acl_silence(uint16_t connHandle, uint8_t silence);
void bt_drv_reg_op_call_monitor(uint16_t connHandle, uint8_t tws_role);
void bt_drv_reg_op_lock_sniffer_sco_resync(void);
void bt_drv_reg_op_unlock_sniffer_sco_resync(void);
uint8_t bt_drv_reg_op_retx_att_nb_manager(uint8_t mode);
void bt_drv_reg_op_afh_set_default(void);
void bt_drv_reg_op_update_sniffer_bitoffset(uint16_t mobile_conhdl,uint16_t master_conhdl);
void bt_drv_reg_op_modify_bitoff_timer(uint16_t time_out);
uint16_t em_bt_bitoff_getf(int elt_idx);
void bt_drv_reg_op_ble_llm_substate_hacker(void);
void bt_drv_reg_op_esco_acl_sniff_delay_cal(uint16_t hciHandle,bool enable);
bool  bt_drv_reg_op_check_esco_acl_sniff_conflict(uint16_t hciHandle);
void bt_drv_reg_op_set_tpoll(uint8_t linkid,uint16_t poll_interval);
uint8_t  bt_drv_reg_op_get_role(uint8_t linkid);
void bt_drv_set_hwagc_read_en(uint8_t en);
int8_t  bt_drv_rssi_correction(int8_t rssi);
void bt_drv_reg_set_rssi_seed(uint32_t seed);
void bt_drv_reg_piconet_clk_offset_get(uint16_t connHandle, int32_t *clock_offset, uint16_t *bit_offset);
void bt_drv_reset_sw_seq_filter(uint16_t connHandle);
bool bt_drv_reg_op_get_control_state(void);
void bt_drv_reg_dma_tc_clkcnt_get(uint32_t *btclk, uint16_t *btcnt);

void bt_drv_clean_flags_of_ble_and_sco(void);
void bt_drv_sco_txfifo_reset(uint16_t codec_id);
void bt_drv_hack_start_snoop_in_sco(void);

#define RETX_NB_0                 0
#define RETX_NB_1                 1
#define RETX_NB_2                 2
#define RETX_NEGO                 3
#ifdef CHIP_BEST2000
#define MIC_BUFF_ADRR_REG        (0xc0000040)
#define RX_BUFF_ADRR        (0xc0000044)
#elif CHIP_BEST1400
#define MIC_BUFF_ADRR_REG        (0xc0000058)
#define RX_BUFF_ADRR        (0xc000005c)
#elif CHIP_BEST2300P
#define MIC_BUFF_ADRR_REG        (0xc0000058)
#define RX_BUFF_ADRR        (0xc000005c)
#endif
void bt_drv_reg_afh_env_reset(void);
void bt_drv_acl_tx_silence(uint16_t connHandle, uint8_t on);
void bt_drv_reg_op_clean_ibrt_sco_flags(void);
void bt_drv_reg_ibrt_env_reset(void);
void btdrv_set_tws_acl_poll_interval(uint16_t poll_interval);
void btdrv_reg_op_enable_fix_tws_poll_interval(bool enable);
void btdrv_reg_op_set_tws_link_duration(uint8_t slot_num);
void btdrv_decrease_tx_pwr_when_reconnect(bool enable);

#ifdef __cplusplus
}
#endif

#endif

