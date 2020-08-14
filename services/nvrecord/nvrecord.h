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
#ifndef __NVRECORD_H__
#define __NVRECORD_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"
#include "cmsis.h"
#include "nvrec_config.h"
#include "bluetooth.h"
#include "me_api.h"
#include "hal_trace.h"
#define DDB_RECORD_NUM  10
#define section_name_ddbrec     "ddbrec"
#define nvrecord_mem_pool_size      4096
#define nvrecord_tag    "nvrecord"
#define nvdev_tag       "nvdev_tag"
#define section_name_other     "other"

#define NVREC_DEV_NEWEST_REV   		2
#define NVREC_DEV_VERSION_1			1

#define BLE_NAME_LEN_IN_NV	32
/*
    version|magic           16bit|16bit             0001 cb91
    crc                            32bit
    reserve[0]                 32bit
    reserve[1]                 32bit
    config_addr                32bit
    heap_contents            32bit
    mempool                   ...
*/
#define nvrecord_struct_version         4
#define nvrecord_magic                      0xcb91

#define nv_record_debug
#ifdef nv_record_debug
#define nvrec_trace     TRACE
#else
#define nvrec_trace(...)
#endif

typedef enum
{
    pos_version_and_magic,// 0
    pos_crc,                            // 1
    pos_start_ram_addr,                    // 2
    pos_reserv2,                    // 3
    pos_config_addr,            // 4
    pos_heap_contents,      // 5

}nvrec_mempool_pre_enum;



/*
    this is the nvrec dev zone struct :
    version|magic   16bit|16bit
    crc         32bit
    reserve[0]      32bit
    reserv[1]       32bit
    dev local name  max 249*8bit
    dev bt addr         64bit
    dev ble addr        64bit
    calib data      32bit
*/
#define nvrec_dev_version   1
#define nvrec_dev_magic      0xba80
typedef enum
{
    dev_version_and_magic,      //0
    dev_crc,                    // 1
    dev_reserv1,                // 2
    dev_reserv2,                //3// 3
    dev_name,                   //[4~66]
    dev_bt_addr = 67,                //[67~68]
    dev_ble_addr = 69,               //[69~70]
    dev_dongle_addr = 71,
    dev_xtal_fcap = 73,              //73
    dev_data_len,
}nvrec_dev_enum;

// following the former nv rec dev info
typedef enum
{
    rev2_dev_data_len = 75,				// 75, length of the valid content, excluding crc
    rev2_dev_crc,                    //76, crc value of the following data
    rev2_dev_section_start_reserved, //77
    rev2_dev_reserv2,                //78
    rev2_dev_name,                   //[79~141]
    rev2_dev_bt_addr = 142,           //[142~143]
    rev2_dev_ble_addr = 144,          //[144~145]
    rev2_dev_dongle_addr = 146,		//[146~147]
    rev2_dev_xtal_fcap = 148,         //148
    rev2_dev_ble_name = 149,			//[149~156]
#if defined(NVREC_BAIDU_DATA_SECTION)
    rev2_dev_prod_sn = 157,			//[157~160]
#endif
	// TODO: add the new section in the future if needed

	rev2_dev_section_end = 157,

}nvrec_dev_rev_2_enum;

#define mempool_pre_offset       (1+pos_config_addr+((sizeof(heap_handle_t)/sizeof(uint32_t))+1))

typedef enum
{
    NV_STATE_IDLE,
    NV_STATE_ERASED,
}NV_STATE;

typedef struct
{
    bool is_update;
    NV_STATE state;
    nvrec_config_t *config;
}nv_record_struct;

typedef enum
{
    section_usrdata_ddbrecord,
    section_none
}SECTIONS_ADP_ENUM;

typedef  struct btdevice_volume{
    int8_t a2dp_vol;
    int8_t hfp_vol;
}btdevice_volume;

typedef  struct btdevice_profile{
    bool hfp_act;
    bool hsp_act;
    bool a2dp_act;
    uint8_t a2dp_codectype;
}btdevice_profile;

typedef struct _nvrec_btdevicerecord
{
    btif_device_record_t record;
    btdevice_volume device_vol;
    btdevice_profile device_plf;
} nvrec_btdevicerecord;

extern uint8_t nv_record_dev_rev;

void nv_record_init(void);
bt_status_t nv_record_open(SECTIONS_ADP_ENUM section_id);
bt_status_t nv_record_enum_dev_records(unsigned short index,btif_device_record_t* record);
bt_status_t nv_record_add(SECTIONS_ADP_ENUM type,void *record);
bt_status_t nv_record_ddbrec_find(const bt_bdaddr_t *bd_ddr, btif_device_record_t*record);
bt_status_t nv_record_ddbrec_delete(const bt_bdaddr_t *bdaddr);
int nv_record_btdevicerecord_find(const bt_bdaddr_t *bd_ddr, nvrec_btdevicerecord **record);
int nv_record_touch_cause_flush(void);

void nv_record_sector_clear(void);
void nv_record_flash_flush(void);
int nv_record_flash_flush_in_sleep(void);


int nv_record_enum_latest_two_paired_dev(btif_device_record_t* record1,btif_device_record_t* record2);
void nv_record_all_ddbrec_print(void);
int nvrec_dev_get_dongleaddr(bt_bdaddr_t *dongleaddr);

int nvrec_dev_get_btaddr(char *btaddr);
char* nvrec_dev_get_bt_name(void);
const char* nvrec_dev_get_ble_name(void);
void nv_record_update_runtime_userdata(void);
bt_status_t nv_record_config_rebuild(void);

#if defined(NVREC_BAIDU_DATA_SECTION)
#define BAIDU_DATA_SECTIN ("baidu_data")
#define BAIDU_DATA_FM_KEY ("fmfreq")
#define BAIDU_DATA_RAND_KEY ("rand")

#define BAIDU_DATA_DEF_RAND "abcdefghi"

int nvrec_baidu_data_init(void);
int nvrec_get_fm_freq(void);
int nvrec_set_fm_freq(int fmfreq);
int nvrec_get_rand(char *rand);
int nvrec_set_rand(char *rand);
int nvrec_clear_rand(void);
int nvrec_dev_get_sn(char *sn);
#endif

#ifdef __cplusplus
}
#endif
#endif /* __NVRECORD_H__*/
