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
#include <assert.h>
#include "hal_sleep.h"
#include "besbt.h"
#include "nvrecord.h"
#include "nvrecord_dev.h"
#include "nvrecord_env.h"
#include "crc32.h"
#include "hal_timer.h"
#include "hal_norflash.h"
#include "pmu.h"
#include "tgt_hardware.h"
#include "norflash_api.h"

//#define NVREC_BURN_TEST
#define OS_DUMP8(x...)
#define NVRECORD_CACHE_2_UNCACHE(addr)  \
    ((unsigned char *)((unsigned int)addr & ~(0x04000000)))

#define USERDATA_POOL_LOC               __attribute__((section(".userdata_pool")))

extern uint32_t __factory_start[];
extern uint32_t __userdata_start[];

uint8_t nv_record_dev_rev = NVREC_DEV_NEWEST_REV;

char classics_bt_name[BLE_NAME_LEN_IN_NV-5]= "LIVE";
extern int nv_record_flash_flush_in_sleep(void);
void nv_callback(void* param);

nv_record_struct nv_record_config;
static bool nvrec_init = false;
static bool nvrec_mempool_init = false;
static bool dev_sector_valid = false;

static uint32_t USERDATA_POOL_LOC usrdata_ddblist_pool[1024];

static void nv_record_print_dev_record(const btif_device_record_t* record)
{
#ifdef nv_record_debug

    nvrec_trace("nv record bdAddr = ");
    DUMP8("%02x ",record->bdAddr.address,sizeof(record->bdAddr.address));
    nvrec_trace("record_trusted = ");
    DUMP8("%d ",&record->trusted,sizeof((uint8_t)record->trusted));
    nvrec_trace("record_linkKey = ");
    DUMP8("%02x ",record->linkKey,sizeof(record->linkKey));
    nvrec_trace("record_keyType = ");
    DUMP8("%x ",&record->keyType,sizeof(record->keyType));
    nvrec_trace("record_pinLen = ");
    DUMP8("%x ",&record->pinLen,sizeof(record->pinLen));

#endif
}

heap_handle_t g_nv_mempool = NULL;

static void nv_record_mempool_init(void)
{
    unsigned char *poolstart = 0;

    poolstart = (unsigned char *)(usrdata_ddblist_pool+mempool_pre_offset);
    if(!nvrec_mempool_init)
    {
        g_nv_mempool = heap_register((unsigned char*)poolstart, (size_t)(sizeof(usrdata_ddblist_pool)-(mempool_pre_offset*sizeof(uint32_t))));
        nvrec_mempool_init = TRUE;
    }
    /*add other memory pool */

}

static bool nv_record_data_is_valid(void)
{
    uint32_t crc;
    uint32_t flsh_crc;
    uint32_t verandmagic;

    memcpy((void *)usrdata_ddblist_pool,(void *)__userdata_start,sizeof(usrdata_ddblist_pool));
    verandmagic = usrdata_ddblist_pool[0];
    if(((nvrecord_struct_version<<16)|nvrecord_magic) != verandmagic)
        return false;
    crc = crc32(0,(uint8_t *)(&usrdata_ddblist_pool[pos_heap_contents]),(sizeof(usrdata_ddblist_pool)-(pos_heap_contents*sizeof(uint32_t))));
    nvrec_trace("%s,crc=0x%x",nvrecord_tag,crc);
    flsh_crc = usrdata_ddblist_pool[pos_crc];
    nvrec_trace("%s,read crc from flash is 0x%x",nvrecord_tag,flsh_crc);
    if (flsh_crc == crc)
        return true;

    return false;
}

void nv_record_init(void)
{
    enum NORFLASH_API_RET_T result;
    uint32_t sector_size = 0;
    uint32_t block_size = 0;
    uint32_t page_size = 0;

    hal_norflash_get_size(HAL_NORFLASH_ID_0,
               NULL,
               &block_size,
               &sector_size,
               &page_size);
    result = norflash_api_register(NORFLASH_API_MODULE_ID_USERDATA,
                    HAL_NORFLASH_ID_0,
                    ((uint32_t)__userdata_start),
                    sizeof(usrdata_ddblist_pool),
                    block_size,
                    sector_size,
                    page_size,
                    sizeof(usrdata_ddblist_pool)*2,
                    nv_callback
                    );
    ASSERT(result == NORFLASH_API_OK,"nv_record_init: module register failed! result = %d.",result);
}

bt_status_t nv_record_open(SECTIONS_ADP_ENUM section_id)
{
    bt_status_t ret_status = BT_STS_FAILED;
    uint32_t lock;
    if(nvrec_init)
        return BT_STS_SUCCESS;
    lock = int_lock_global();
    switch(section_id)
    {
        case section_usrdata_ddbrecord:
            if(nv_record_data_is_valid() && 
                (usrdata_ddblist_pool[pos_start_ram_addr]==(uint32_t)usrdata_ddblist_pool))
            {
                TRACE("%s,normal crc valid.",__func__);
                nv_record_config.config = (nvrec_config_t *)usrdata_ddblist_pool[pos_config_addr];
                nvrec_trace("%s,nv_record_config.config=0x%x,g_nv_mempool=0x%x,ln=%d\n",nvrecord_tag,usrdata_ddblist_pool[pos_config_addr],g_nv_mempool,__LINE__);
                g_nv_mempool = (heap_handle_t)(&usrdata_ddblist_pool[mempool_pre_offset]);
                TRACE("%s,g_nv_mempool=0x%x.",__func__,g_nv_mempool);
                nv_record_config.is_update = false;
                nv_record_config.state = NV_STATE_IDLE;
                ret_status = BT_STS_SUCCESS;
            }
            else
            {
                TRACE("%s,crc invalid or the user data pool's ram address is modified. ",__func__);
                memset(usrdata_ddblist_pool,0,sizeof(usrdata_ddblist_pool));

                nv_record_mempool_init();
                nv_record_config.config = nvrec_config_new("PRECIOUS 4K.");
                // nv_record_config.is_update = true;
                nvrec_trace("%s,nv_record_config.config=0x%x,g_nv_mempool=0x%x,ln=%d\n",nvrecord_tag,nv_record_config.config,g_nv_mempool,__LINE__);
                // nv_record_flash_flush();
                ret_status = BT_STS_SUCCESS;;
            }
            break;
        default:
            break;
    }
    nvrec_init = true;

    if (ret_status == BT_STS_SUCCESS)
    {
#ifdef FLASH_SUSPEND
        hal_sleep_set_sleep_hook(HAL_SLEEP_HOOK_USER_NVRECORD, nv_record_flash_flush_in_sleep);
#else
        hal_sleep_set_deep_sleep_hook(HAL_DEEP_SLEEP_HOOK_USER_NVRECORD, nv_record_flash_flush_in_sleep);
#endif
    }

    int_unlock_global(lock);
    nvrec_trace("%s,open,ret_status=%d\n",nvrecord_tag,ret_status);
    return ret_status;
}

bt_status_t nv_record_config_rebuild(void)
{
    nvrec_mempool_init = false;
    memset(usrdata_ddblist_pool,0,sizeof(usrdata_ddblist_pool));
    nv_record_mempool_init();
    nv_record_config.config = nvrec_config_new("PRECIOUS 4K.");
    if(nv_record_config.config == NULL)
        return BT_STS_FAILED;

    nvrec_init = true;
    nv_record_config.is_update = true;
    return BT_STS_SUCCESS;
}

static size_t config_entries_get_ddbrec_length(const char *secname)
{
    size_t entries_len = 0;

    if(NULL != nv_record_config.config)
    {
        nvrec_section_t *sec = NULL;
        sec = nvrec_section_find(nv_record_config.config,secname);
        if (NULL != sec)
            entries_len = sec->entries->length;
    }
    return entries_len;
}

#if 0
static bool config_entries_has_ddbrec(const btif_device_record_t* param_rec)
{
    char key[BD_ADDR_SIZE+1] = {0,};
    assert(param_rec != NULL);

    memcpy(key,param_rec->bdAddr.address,BTIF_BD_ADDR_SIZE);
    if(nvrec_config_has_key(nv_record_config.config,section_name_ddbrec,(const char *)key))
        return true;
    return false;
}
#endif

static void config_entries_ddbdev_delete_head(void)//delete the oldest record.
{
    list_node_t *head_node = NULL;
    list_t *entry = NULL;
    nvrec_section_t *sec = NULL;

    sec = nvrec_section_find(nv_record_config.config,section_name_ddbrec);
    entry = sec->entries;
    head_node = list_begin_ext(entry);
    if(NULL != head_node)
    {
        btif_device_record_t *rec = NULL;
        unsigned int recaddr = 0;

        nvrec_entry_t *entry_temp = list_node_ext(head_node);
        recaddr = nvrec_config_get_int(nv_record_config.config,section_name_ddbrec,entry_temp->key,0);
        rec = (btif_device_record_t *)recaddr;
        pool_free(rec);
        nvrec_entry_free(entry_temp);
         if(1 == config_entries_get_ddbrec_length(section_name_ddbrec))
            entry->head = entry->tail = NULL;
        else
            entry->head = list_next_ext(head_node);
        pool_free(head_node);
        entry->length -= 1;
    }
}

static void config_entries_ddbdev_delete_tail(void)
{
    list_node_t *node;
    list_node_t *temp_ptr;
    list_node_t *tailnode;
    size_t entrieslen = 0;
    nvrec_entry_t *entry_temp = NULL;
    nvrec_section_t *sec = nvrec_section_find(nv_record_config.config,section_name_ddbrec);
    btif_device_record_t *recaddr = NULL;
    unsigned int addr = 0;

    if (!sec)
        assert(0);
    sec->entries->length -= 1;
    entrieslen = sec->entries->length;
    node = list_begin_ext(sec->entries);
    while(entrieslen > 1)
    {
        node = list_next_ext(node);
        entrieslen--;
    }
    tailnode = list_next_ext(node);
    entry_temp = list_node_ext(tailnode);
    addr = nvrec_config_get_int(nv_record_config.config,section_name_ddbrec,entry_temp->key,0);
    recaddr = (btif_device_record_t *)addr;
    pool_free(recaddr);
    nvrec_entry_free(entry_temp);
    //pool_free(entry_temp);
    temp_ptr = node->next;
    node->next = NULL;
    pool_free(temp_ptr);
    sec->entries->tail = node;
}

static void config_entries_ddbdev_delete(const btif_device_record_t* param_rec)
{
    nvrec_section_t *sec = NULL;
    list_node_t *entry_del = NULL;
    list_node_t *entry_pre = NULL;
    list_node_t *entry_next = NULL;
    list_node_t *node = NULL;
    btif_device_record_t *recaddr = NULL;
    unsigned int addr = 0;
    int pos = 0,pos_pre=0;
    nvrec_entry_t *entry_temp = NULL;

    sec = nvrec_section_find(nv_record_config.config,section_name_ddbrec);
    for(node=list_begin_ext(sec->entries);node!=NULL;node=list_next_ext(node))
    {
        nvrec_entry_t *entries = list_node_ext(node);
        pos++;
        if(0 == memcmp(entries->key,param_rec->bdAddr.address,BTIF_BD_ADDR_SIZE))
        {
            entry_del = node;
            entry_next = entry_del->next;
            break;
        }
    }

    if (entry_del){
        /*get entry_del pre node*/
        pos_pre = (pos-1);
        pos=0;
        node=list_begin_ext(sec->entries);
        pos++;
        while(pos < pos_pre)
        {
            node=list_next_ext(node);
            pos += 1;
        }
        entry_pre = node;

        /*delete entry_del following...*/
        entry_temp = list_node_ext(entry_del);
        addr = nvrec_config_get_int(nv_record_config.config,section_name_ddbrec,(char *)entry_temp->key,0);
        assert(0!=addr);
        recaddr = (btif_device_record_t *)addr;
        pool_free(recaddr);
        nvrec_entry_free(entry_temp);
        //pool_free(entry_temp);
        pool_free(entry_pre->next);
        entry_pre->next = entry_next;
        sec->entries->length -= 1;
    }
}

static bool config_entries_ddbdev_is_head(const btif_device_record_t* param_rec)
{
    list_node_t *head_node = NULL;
    nvrec_section_t *sec = NULL;
    list_t *entry = NULL;
    nvrec_entry_t *entry_temp = NULL;

    sec = nvrec_section_find(nv_record_config.config,section_name_ddbrec);
    entry = sec->entries;
    head_node = list_begin_ext(entry);
    entry_temp = list_node_ext(head_node);
    if(0 == memcmp(entry_temp->key,param_rec->bdAddr.address,BTIF_BD_ADDR_SIZE))
        return true;
    return false;
}

static bool config_entries_ddbdev_is_tail(const btif_device_record_t* param_rec)
{
    list_node_t *node;
    nvrec_entry_t *entry_temp = NULL;

    nvrec_section_t *sec = nvrec_section_find(nv_record_config.config,section_name_ddbrec);
    if (!sec)
        assert(0);
    for (node = list_begin_ext(sec->entries); node != list_end_ext(sec->entries); node = list_next_ext(node))
    {
        entry_temp = list_node_ext(node);
    }
    if(0 == memcmp(entry_temp->key,param_rec->bdAddr.address,BTIF_BD_ADDR_SIZE))
        return true;
    return false;
}

/*
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
*/
static void config_specific_entry_value_delete(const btif_device_record_t* param_rec)
{
    if(config_entries_ddbdev_is_head(param_rec))
        config_entries_ddbdev_delete_head();
    else if(config_entries_ddbdev_is_tail(param_rec))
        config_entries_ddbdev_delete_tail();
    else
        config_entries_ddbdev_delete(param_rec);
    nv_record_update_runtime_userdata();
}


/**********************************************
gyt add:list head is the odest,list tail is the latest.
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
**********************************************/
static bt_status_t nv_record_ddbrec_add(const btif_device_record_t* param_rec)
{
    btdevice_volume device_vol;
    btdevice_profile device_plf;
    char key[BTIF_BD_ADDR_SIZE+1] = {0,};
    nvrec_btdevicerecord *nvrec_pool_record = NULL;
    nvrec_btdevicerecord *param_rec_record = NULL;
    bool ddbrec_exist = false;
    bool is_flush = true;
    int getint;

    if(NULL == param_rec)
        return BT_STS_FAILED;
    memcpy(key,param_rec->bdAddr.address,BTIF_BD_ADDR_SIZE);
    getint = nvrec_config_get_int(nv_record_config.config,section_name_ddbrec,(char *)key,0);
    if(getint){
        ddbrec_exist = true;
        nvrec_pool_record = (nvrec_btdevicerecord *)getint;
        device_vol.a2dp_vol = nvrec_pool_record->device_vol.a2dp_vol;
        device_vol.hfp_vol = nvrec_pool_record->device_vol.hfp_vol;
        device_plf.hfp_act = nvrec_pool_record->device_plf.hfp_act;
        device_plf.hsp_act = nvrec_pool_record->device_plf.hsp_act;
        device_plf.a2dp_act = nvrec_pool_record->device_plf.a2dp_act;

        param_rec_record = (nvrec_btdevicerecord*)param_rec;
        if( param_rec_record-> device_vol.a2dp_vol == device_vol.a2dp_vol
            && param_rec_record-> device_vol.hfp_vol == device_vol.hfp_vol
            && param_rec_record-> device_plf.hfp_act == device_plf.hfp_act
            && param_rec_record-> device_plf.hsp_act == device_plf.hsp_act
            && param_rec_record-> device_plf.a2dp_act == device_plf.a2dp_act
            && config_entries_ddbdev_is_tail(param_rec)
           ){
           is_flush = false;
        }

    }

    if(is_flush){
        nvrec_pool_record = (nvrec_btdevicerecord *)pool_malloc(sizeof(nvrec_btdevicerecord));
        if(NULL == nvrec_pool_record){
            nvrec_trace("%s,pool_malloc failure.",nvrecord_tag);
            return BT_STS_FAILED;
        }
        nvrec_trace("%s,pool_malloc addr = 0x%x\n",nvrecord_tag,(unsigned int)nvrec_pool_record);
        memcpy(nvrec_pool_record,param_rec,sizeof(btif_device_record_t));
        memcpy(key,param_rec->bdAddr.address,BTIF_BD_ADDR_SIZE);
        if (ddbrec_exist){
            nvrec_pool_record->device_vol.a2dp_vol = device_vol.a2dp_vol;
            nvrec_pool_record->device_vol.hfp_vol = device_vol.hfp_vol;
            nvrec_pool_record->device_plf.hfp_act = device_plf.hfp_act;
            nvrec_pool_record->device_plf.hsp_act = device_plf.hsp_act;
            nvrec_pool_record->device_plf.a2dp_act = device_plf.a2dp_act;
        }else{
            nvrec_pool_record->device_vol.a2dp_vol = NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT;
            nvrec_pool_record->device_vol.hfp_vol = NVRAM_ENV_STREAM_VOLUME_HFP_VOL_DEFAULT;
            nvrec_pool_record->device_plf.hfp_act = false;
            nvrec_pool_record->device_plf.hsp_act = false;
            nvrec_pool_record->device_plf.a2dp_act = false;
        }
        if (ddbrec_exist){
            config_specific_entry_value_delete(param_rec);
        }
        if(DDB_RECORD_NUM == config_entries_get_ddbrec_length(section_name_ddbrec)) {
            nvrec_trace("%s,ddbrec list is full,delete the oldest and add param_rec to list.",nvrecord_tag);
            config_entries_ddbdev_delete_head();
        }
        nvrec_config_set_int(nv_record_config.config,section_name_ddbrec,(char *)key,(int)nvrec_pool_record);
#ifdef nv_record_debug
        getint = nvrec_config_get_int(nv_record_config.config,section_name_ddbrec,(char *)key,0);
        nvrec_trace("%s,get pool_malloc addr = 0x%x\n",nvrecord_tag,(unsigned int)getint);
#endif
        nv_record_update_runtime_userdata();
    }
    return BT_STS_SUCCESS;
}
/*
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
*/
bt_status_t nv_record_add(SECTIONS_ADP_ENUM type,void *record)
{
    bt_status_t retstatus = BT_STS_FAILED;

    if ((NULL == record) || (section_none == type))
        return BT_STS_FAILED;
    switch(type)
    {
        case section_usrdata_ddbrecord:
            retstatus = nv_record_ddbrec_add(record);
            break;
        default:
            break;
    }

    return retstatus;
}

/*
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
*/
bt_status_t nv_record_ddbrec_find(const bt_bdaddr_t* bd_ddr,btif_device_record_t *record)
{
    unsigned int getint = 0;
    char key[BTIF_BD_ADDR_SIZE+1] = {0,};
    btif_device_record_t *getrecaddr = NULL;

    if((NULL == nv_record_config.config) || (NULL == bd_ddr) || (NULL == record))
        return BT_STS_FAILED;
    memcpy(key,bd_ddr->address,BTIF_BD_ADDR_SIZE);
    getint = nvrec_config_get_int(nv_record_config.config,section_name_ddbrec,(char *)key,0);
    if(0 == getint)
        return BT_STS_FAILED;
    getrecaddr = (btif_device_record_t *)getint;
    memcpy(record,(void *)getrecaddr,sizeof(btif_device_record_t));
    return BT_STS_SUCCESS;
}

int nv_record_btdevicerecord_find(const bt_bdaddr_t *bd_ddr, nvrec_btdevicerecord **record)
{
    unsigned int getint = 0;
    char key[BTIF_BD_ADDR_SIZE+1] = {0,};

    if((NULL == nv_record_config.config) || (NULL == bd_ddr) || (NULL == record))
        return -1;
    memcpy(key,bd_ddr->address, BTIF_BD_ADDR_SIZE);
    getint = nvrec_config_get_int(nv_record_config.config,section_name_ddbrec,(char *)key,0);
    if(0 == getint)
        return -1;
    *record = (nvrec_btdevicerecord *)getint;
    return 0;
}

/*
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
*/
bt_status_t nv_record_ddbrec_delete(const bt_bdaddr_t *bdaddr)
{
    unsigned int getint = 0;
    btif_device_record_t *getrecaddr = NULL;
    char key[BTIF_BD_ADDR_SIZE+1] = {0,};

    memcpy(key,bdaddr->address,BTIF_BD_ADDR_SIZE);
    getint = nvrec_config_get_int(nv_record_config.config,section_name_ddbrec,(char *)key,0);
    if(0 == getint)
        return BT_STS_FAILED;
    //found ddb record,del it and return succeed.
    getrecaddr = (btif_device_record_t *)getint;
    config_specific_entry_value_delete((const btif_device_record_t *)getrecaddr);
    return BT_STS_SUCCESS;
}

/*
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
*/
bt_status_t nv_record_enum_dev_records(unsigned short index,btif_device_record_t* record)
{
    nvrec_section_t *sec = NULL;
    list_node_t *node;
    unsigned short pos = 0;
    nvrec_entry_t *entry_temp = NULL;
    btif_device_record_t *recaddr = NULL;
    unsigned int addr = 0;

    if((index >= DDB_RECORD_NUM) || (NULL == record) || (NULL == nv_record_config.config))
        return BT_STS_FAILED;
    sec = nvrec_section_find(nv_record_config.config,section_name_ddbrec);
    if(NULL == sec)
        return BT_STS_INVALID_PARM;
    if(NULL == sec->entries)
        return BT_STS_INVALID_PARM;
    if(0 == sec->entries->length)
        return BT_STS_FAILED;
    if(index >= sec->entries->length)
        return BT_STS_FAILED;
    node = list_begin_ext(sec->entries);

    while(pos < index)
    {
        node = list_next_ext(node);
        pos++;
    }
    entry_temp = list_node_ext(node);
    addr = nvrec_config_get_int(nv_record_config.config,section_name_ddbrec,(char *)entry_temp->key,0);
    if(0 == addr)
        return BT_STS_FAILED;
    recaddr = (btif_device_record_t *)addr;
    memcpy(record,recaddr,sizeof(btif_device_record_t));
    nv_record_print_dev_record(record);
    return BT_STS_SUCCESS;
}

/*
return:
    -1:     enum dev failure.
    0:      without paired dev.
    1:      only 1 paired dev,store@record1.
    2:      get 2 paired dev.notice:record1 is the latest record.
*/
int nv_record_enum_latest_two_paired_dev(btif_device_record_t* record1,btif_device_record_t* record2)
{
    bt_status_t getret1 = BT_STS_FAILED;
    bt_status_t getret2 = BT_STS_FAILED;
    nvrec_section_t *sec = NULL;
    size_t entries_len = 0;
    list_t *entry = NULL;

    if((NULL == record1) || (NULL == record2))
        return -1;
    sec = nvrec_section_find(nv_record_config.config,section_name_ddbrec);
    if(NULL == sec)
        return 0;
    entry = sec->entries;
    if(NULL == entry)
        return 0;
    entries_len = entry->length;
    if(0 == entries_len)
        return 0;
    else if(entries_len < 0)
        return -1;
    if(1 == entries_len)
    {
        getret1 = nv_record_enum_dev_records(0,record1);
        if(BT_STS_SUCCESS == getret1)
            return 1;
        return -1;
    }

    getret1 = nv_record_enum_dev_records(entries_len-1,record1);
    getret2 = nv_record_enum_dev_records(entries_len-2,record2);
    if((BT_STS_SUCCESS != getret1) || (BT_STS_SUCCESS != getret2))
    {
        memset(record1,0x0,sizeof(btif_device_record_t));
        memset(record2,0x0,sizeof(btif_device_record_t));
        return -1;
    }
    return 2;
}

int nv_record_touch_cause_flush(void)
{
	nv_record_update_runtime_userdata();
	return 0;
}

void nv_record_all_ddbrec_print(void)
{
    list_t *entry = NULL;
    int tmp_i = 0;
    nvrec_section_t *sec = NULL;
    size_t entries_len = 0;

    sec = nvrec_section_find(nv_record_config.config,section_name_ddbrec);
    if(NULL == sec)
        return;
    entry = sec->entries;
    if(NULL == entry)
        return;
    entries_len = entry->length;
    if(0 == entries_len)
    {
        nvrec_trace("%s,without btdevicerec.",nvrecord_tag,__LINE__);
        return;
    }
    for(tmp_i=0;tmp_i<entries_len;tmp_i++)
    {
        btif_device_record_t record;
        bt_status_t ret_status;
        ret_status = nv_record_enum_dev_records(tmp_i,&record);
        if(BT_STS_SUCCESS== ret_status)
            nv_record_print_dev_record(&record);
    }
}

void nv_record_sector_clear(void)
{
    uint32_t lock;
    enum NORFLASH_API_RET_T ret;

    lock = int_lock_global();
    ret = norflash_api_erase(NORFLASH_API_MODULE_ID_USERDATA,
                          (uint32_t)__userdata_start,
                          sizeof(usrdata_ddblist_pool),
                          false);
    nvrec_init = false;
	nvrec_mempool_init = false;
    int_unlock_global(lock);
    ASSERT(ret == NORFLASH_API_OK,"nv_record_sector_clear: norflash_api_erase failed! ret = %d.",ret);
}

#define DISABLE_NV_RECORD_CRC_CHECK_BEFORE_FLUSH	1
void nv_record_update_runtime_userdata(void)
{
    uint32_t lock;

	if (NULL == nv_record_config.config)
	{
		return;
	}
	lock = int_lock();
	nv_record_config.is_update = true;

#if !DISABLE_NV_RECORD_CRC_CHECK_BEFORE_FLUSH
	buffer_alloc_ctx* heapctx = memory_buffer_heap_getaddr();
    memcpy((void *)(&usrdata_ddblist_pool[pos_heap_contents]),heapctx,sizeof(buffer_alloc_ctx));
    uint32_t crc = crc32(0,(uint8_t *)(&usrdata_ddblist_pool[pos_heap_contents]),(sizeof(usrdata_ddblist_pool)-(pos_heap_contents*sizeof(uint32_t))));

    usrdata_ddblist_pool[pos_crc] = crc;
#endif

	int_unlock(lock);
}


static int nv_record_flash_flush_int(bool is_async)
{
    uint32_t crc;
    uint32_t lock;
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;
    uint8_t *burn_buf;

#if defined(FLASH_SUSPEND) && defined(NVREC_BURN_TEST)
    static uint32_t stime = 0;
    if (hal_sys_timer_get() - stime > MS_TO_TICKS(20000)) {
        stime = hal_sys_timer_get();
        nv_record_config.is_update = true;
    }
#endif

    if(false == nv_record_config.is_update)
    {
        //nvrec_trace("%s,without update.\n",nvrecord_tag);
        goto _func_end;
    }
    if(NULL == nv_record_config.config)
    {
        nvrec_trace("%s,nv_record_config.config is null.\n",nvrecord_tag);
        goto _func_end;
    }

    burn_buf = (uint8_t *)&usrdata_ddblist_pool[0];
    if(is_async)
    {
        hal_trace_pause();
        lock = int_lock_global();
        if(nv_record_config.state == NV_STATE_IDLE)
        {
            ret = norflash_api_erase(NORFLASH_API_MODULE_ID_USERDATA,
                    (uint32_t)__userdata_start,
                    sizeof(usrdata_ddblist_pool),
                    true);
            if(ret == NORFLASH_API_OK)
            {
                nv_record_config.state = NV_STATE_ERASED;
                nvrec_trace("%s: norflash_api_erase ok,addr = 0x%x.",__func__,(uint32_t)__userdata_start);
            }
            else if(ret == NORFLASH_API_BUFFER_FULL)
            {
            }
            else
            {
                ASSERT(0,"%s: norflash_api_erase err,ret = %d,addr = 0x%x.",__func__,ret,(uint32_t)__userdata_start);
            }
        }

        if(nv_record_config.state == NV_STATE_ERASED)
        {
            crc = crc32(0,(uint8_t *)(&usrdata_ddblist_pool[pos_heap_contents]),(sizeof(usrdata_ddblist_pool)-(pos_heap_contents*sizeof(uint32_t))));
            //nvrec_trace("%s,crc=%x.\n",nvrecord_tag,crc);
            usrdata_ddblist_pool[pos_version_and_magic] = ((nvrecord_struct_version<<16)|nvrecord_magic);
            usrdata_ddblist_pool[pos_crc] = crc;
            usrdata_ddblist_pool[pos_start_ram_addr] = (uint32_t)&usrdata_ddblist_pool;
            usrdata_ddblist_pool[pos_reserv2] = 0;
            usrdata_ddblist_pool[pos_config_addr] = (uint32_t)nv_record_config.config;
            ret = norflash_api_write(NORFLASH_API_MODULE_ID_USERDATA,
                    (uint32_t)__userdata_start,
                    burn_buf,
                    sizeof(usrdata_ddblist_pool),
                    true
                    );
            if(ret == NORFLASH_API_OK)
            {
                nv_record_config.is_update = false;
                nv_record_config.state = NV_STATE_IDLE;
                nvrec_trace("%s: norflash_api_write ok,addr = 0x%x.",__func__,(uint32_t)__userdata_start);
            }
            else if(ret == NORFLASH_API_BUFFER_FULL)
            {
                // nvrec_trace("%s: norflash_api_write err,cache node too more,addr = 0x%x.",__func__,(uint32_t)__userdata_start);
            }
            else
            {
                ASSERT(0,"%s: norflash_api_write err,ret = %d,addr = 0x%x.",__func__,ret,(uint32_t)__userdata_start);
            }
        }
        int_unlock_global(lock);
        hal_trace_continue();
    }
    else
    {
        if(nv_record_config.state == NV_STATE_IDLE)
        {
            do
            {
                hal_trace_pause();
                lock = int_lock_global();
                ret = norflash_api_erase(NORFLASH_API_MODULE_ID_USERDATA,
                            (uint32_t)__userdata_start,
                            sizeof(usrdata_ddblist_pool),
                            true);
                int_unlock_global(lock);
                if(ret == NORFLASH_API_OK)
                {
                    nv_record_config.state = NV_STATE_ERASED;
                    nvrec_trace("%s: norflash_api_erase ok,addr = 0x%x.",__func__,(uint32_t)__userdata_start);
                }
                else if(ret == NORFLASH_API_BUFFER_FULL)
                {
                    do
                    {
                        norflash_api_flush();
                    }while(norflash_api_get_free_buffer_count(NORFLASH_API_ERASING) == 0);
                }
                else
                {
                    ASSERT(0,"%s: norflash_api_erase err,ret = %d,addr = 0x%x.",__func__,ret,(uint32_t)__userdata_start);
                }
                hal_trace_continue();
            }while(ret == NORFLASH_API_BUFFER_FULL);
        }

        if(nv_record_config.state == NV_STATE_ERASED)
        {
            do
            {
                hal_trace_pause();
                lock = int_lock_global();
                crc = crc32(0,(uint8_t *)(&usrdata_ddblist_pool[pos_heap_contents]),(sizeof(usrdata_ddblist_pool)-(pos_heap_contents*sizeof(uint32_t))));
                //nvrec_trace("%s,crc=%x.\n",nvrecord_tag,crc);
                usrdata_ddblist_pool[pos_version_and_magic] = ((nvrecord_struct_version<<16)|nvrecord_magic);
                usrdata_ddblist_pool[pos_crc] = crc;
                usrdata_ddblist_pool[pos_start_ram_addr] = (uint32_t)&usrdata_ddblist_pool;
                usrdata_ddblist_pool[pos_reserv2] = 0;
                usrdata_ddblist_pool[pos_config_addr] = (uint32_t)nv_record_config.config;
                ret = norflash_api_write(NORFLASH_API_MODULE_ID_USERDATA,
                        (uint32_t)__userdata_start,
                        burn_buf,
                        sizeof(usrdata_ddblist_pool),
                        true
                        );
                int_unlock_global(lock);
                hal_trace_continue();
                if(ret == NORFLASH_API_OK)
                {
                    nv_record_config.is_update = false;
                    nv_record_config.state = NV_STATE_IDLE;
                    nvrec_trace("%s: norflash_api_write ok,addr = 0x%x.",__func__,(uint32_t)__userdata_start);
                }
                else if(ret == NORFLASH_API_BUFFER_FULL)
                {
                    do
                    {
                        norflash_api_flush();
                    }while(norflash_api_get_free_buffer_count(NORFLASH_API_WRITTING) == 0);
                }
                else
                {
                    ASSERT(0,"%s: norflash_api_write err,ret = %d,addr = 0x%x.",__func__,ret,(uint32_t)__userdata_start);
                }
            }while(ret == NORFLASH_API_BUFFER_FULL);

            do
            {
                norflash_api_flush();
            }while(norflash_api_get_used_buffer_count(NORFLASH_API_MODULE_ID_USERDATA,NORFLASH_API_ALL) > 0);
        }
#if 0 //#ifdef nv_record_debug
        if (ret == NORFLASH_API_OK)
        {
            uint32_t recrc;
            uint32_t crc;
            uint8_t* burn_buf;

            uint8_t* read_buffer = (uint8_t*)__userdata_start;

            burn_buf = (uint8_t *)&usrdata_ddblist_pool[0];
            crc = ((uint32_t *)burn_buf)[pos_crc];
            recrc = crc32(0,((uint8_t *)read_buffer + sizeof(uint32_t)*pos_heap_contents),(sizeof(usrdata_ddblist_pool)-(pos_heap_contents*sizeof(uint32_t))));
            ASSERT(crc == recrc,"%s, 0x%x,recrc=%08x crc=%08x\n",
                     __func__,(uint32_t)__userdata_start,recrc,crc);
            TRACE("%s crc ok.",__func__);
        }
#endif

    }
_func_end:
    return (ret == NORFLASH_API_OK) ? 0:1;
}

void nv_record_flash_flush(void)
{
    nv_record_flash_flush_int(false);
}

int nv_record_flash_flush_in_sleep(void)
{
    nv_record_flash_flush_int(true);
    return 0;
}

void nv_callback(void* param)
{
    NORFLASH_API_OPERA_RESULT *opera_result;
    uint8_t *burn_buf;

    opera_result = (NORFLASH_API_OPERA_RESULT*)param;

    nvrec_trace("%s:type = %d, addr = 0x%x,len = 0x%x,remain = %d,result = %d.",
                __func__,
                opera_result->type,
                opera_result->addr,
                opera_result->len,
                opera_result->remain_num,
                opera_result->result);

#ifdef nv_record_debug
        if (opera_result->result == NORFLASH_API_OK &&
            opera_result->remain_num == 0 &&
            nv_record_config.is_update == false &&
            opera_result->type == NORFLASH_API_WRITTING) {
            uint32_t recrc;
            uint32_t crc;
            uint8_t* read_buffer = (uint8_t*)__userdata_start;

            burn_buf = (uint8_t *)&usrdata_ddblist_pool[0];
            crc = ((uint32_t *)burn_buf)[pos_crc];
            //DUMP8("0x%x,",burn_buf,8);
            //DUMP8("0x%x,",read_buffer,8);
            recrc = crc32(0,((uint8_t *)read_buffer + sizeof(uint32_t)*pos_heap_contents),(sizeof(usrdata_ddblist_pool)-(pos_heap_contents*sizeof(uint32_t))));
            ASSERT(crc == recrc,"%s, 0x%x,recrc=%08x crc=%08x\n",
                     __func__,(uint32_t)__userdata_start,recrc,crc);

        }
#endif

}

/*
    dev_version_and_magic,      //0
    dev_crc,                    // 1
    dev_reserv1,                // 2
    dev_reserv2,                //3// 3
    dev_name,                   //[4~66]
    dev_bt_addr = 67,                //[67~68]
    dev_ble_addr = 69,               //[69~70]
    dev_dongle_addr = 71,
    dev_xtal_fcap = 73,              //73
    dev_data_len = 74,

    dev_ble_name = 75,  //[75~82]
*/
bool nvrec_dev_data_open(void)
{
    uint32_t dev_zone_crc,dev_zone_flsh_crc;
    uint32_t vermagic;

    vermagic = __factory_start[dev_version_and_magic];
    TRACE("%s,vermagic=0x%x",nvdev_tag,vermagic);
	if ((nvrec_dev_magic != (vermagic&0xFFFF)) ||
		((vermagic >> 16) > NVREC_DEV_NEWEST_REV))
    {
        dev_sector_valid = false;
        TRACE("%s,dev sector invalid.",nvdev_tag);
        return dev_sector_valid;
    }

	// following the nv rec version number programmed by the downloader tool,
	// to be backward compatible
	nv_record_dev_rev = vermagic >> 16;
	TRACE("Nv record dev version %d", nv_record_dev_rev);

	if (NVREC_DEV_VERSION_1 == nv_record_dev_rev)
	{
    	dev_zone_flsh_crc = __factory_start[dev_crc];
    	dev_zone_crc = crc32(0,(uint8_t *)(&__factory_start[dev_reserv1]),(dev_data_len-dev_reserv1)*sizeof(uint32_t));
	}
	else
	{
		// check the data length
		if ((rev2_dev_section_start_reserved*sizeof(uint32_t)) + __factory_start[rev2_dev_data_len]
			> 4096)
		{
			TRACE("nv rec dev data len %d has exceeds the facory sector size!.",
				__factory_start[rev2_dev_data_len]);
			dev_sector_valid = false;
			return false;
		}

		// assure that in future, if the nv dev data structure is extended, the former tool
		// and former bin can still be workable
    	dev_zone_flsh_crc = __factory_start[rev2_dev_crc];
    	dev_zone_crc = crc32(0,(uint8_t *)(&__factory_start[rev2_dev_section_start_reserved]),
			__factory_start[rev2_dev_data_len]);
	}

	TRACE("data len 0x%x,dev_zone_flsh_crc=0x%x,dev_zone_crc=0x%x",
		__factory_start[rev2_dev_data_len],dev_zone_flsh_crc,dev_zone_crc);
    if (dev_zone_flsh_crc == dev_zone_crc)
    {
        dev_sector_valid = true;
    }

	if (dev_sector_valid)
	{
		TRACE("nv rec dev is valid.");
	}
	else
	{
		TRACE("nv rec dev is invalid.");
	}
    return dev_sector_valid;
}

bool nvrec_dev_localname_addr_init(dev_addr_name *dev)
{
    uint32_t *p_devdata_cache = __factory_start;
    if(true == dev_sector_valid)
    {
        TRACE("nv dev data valid");
        if (NVREC_DEV_VERSION_1 == nv_record_dev_rev)
        {
            memcpy((void *) dev->btd_addr,(void *)&p_devdata_cache[dev_bt_addr],BTIF_BD_ADDR_SIZE);
            memcpy((void *) dev->ble_addr,(void *)&p_devdata_cache[dev_ble_addr],BTIF_BD_ADDR_SIZE);
            dev->localname = (char *)&p_devdata_cache[dev_name];
        }
        else
        {
            memcpy((void *) dev->btd_addr,(void *)&p_devdata_cache[rev2_dev_bt_addr],BTIF_BD_ADDR_SIZE);
            memcpy((void *) dev->ble_addr,(void *)&p_devdata_cache[rev2_dev_ble_addr],BTIF_BD_ADDR_SIZE);
            dev->localname = (char *)&p_devdata_cache[rev2_dev_name];
            dev->ble_name = (char *)&p_devdata_cache[rev2_dev_ble_name];
        }
#ifdef __APP_WL_SMARTVOICE_FIX_DEV_NAME__
        dev->localname = BT_LOCAL_NAME;
        dev->ble_name = BLE_DEFAULT_NAME;
#endif

        memcpy(classics_bt_name,dev->localname,strlen(dev->localname));
        bt_set_local_name(classics_bt_name);
        bt_set_local_address(dev->btd_addr);
    }
    else
    {
        TRACE("nv dev data invalid");
    }

    TRACE("BT addr is:");
    DUMP8("%02x ", dev->btd_addr, BTIF_BD_ADDR_SIZE);
    TRACE("BLE addr is:");
    DUMP8("%02x ", dev->ble_addr, BTIF_BD_ADDR_SIZE);
    TRACE("localname=%s, namelen=%d", dev->localname, strlen(dev->localname));
    if (dev->ble_name)
        TRACE("blename=%s, namelen=%d", dev->ble_name, strlen(dev->ble_name));

    return dev_sector_valid;
}

int nvrec_dev_force_get_btaddress(unsigned char *btd_addr)
{
    uint32_t *p_devdata_cache = __factory_start;
    memcpy((void *) btd_addr,(void *)&p_devdata_cache[dev_bt_addr],BTIF_BD_ADDR_SIZE);
    return 0;
}
static void nvrec_dev_data_fill_xtal_fcap(uint32_t *mem_pool,uint32_t val)
{
    uint8_t *btaddr = NULL;
    uint8_t *bleaddr = NULL;

    assert(0 != mem_pool);
	if (!dev_sector_valid)
	{
		mem_pool[dev_version_and_magic] = ((nv_record_dev_rev<<16)|nvrec_dev_magic);
	}

	if (NVREC_DEV_VERSION_1 == nv_record_dev_rev)
	{
	    if(dev_sector_valid)
	    {
	        memcpy((void *)mem_pool,(void *)__factory_start,0x1000);
        	mem_pool[dev_xtal_fcap] = val;
        	mem_pool[dev_crc] = crc32(0,(uint8_t *)(&mem_pool[dev_reserv1]),(dev_data_len-dev_reserv1)*sizeof(uint32_t));
    	}
    	else
	    {
	        const char *localname = bt_get_local_name();
	        unsigned int namelen = strlen(localname);

	        btaddr = bt_get_local_address();
	        bleaddr = bt_get_ble_local_address();

	        mem_pool[dev_reserv1] = 0;
	        mem_pool[dev_reserv2] = 0;
	        memcpy((void *)&mem_pool[dev_name],(void *)localname,(size_t)namelen);
	        nvrec_dev_rand_btaddr_gen(btaddr);
	        nvrec_dev_rand_btaddr_gen(bleaddr);
	        memcpy((void *)&mem_pool[dev_bt_addr],(void *)btaddr,BTIF_BD_ADDR_SIZE);
	        memcpy((void *)&mem_pool[dev_ble_addr],(void *)bleaddr,BTIF_BD_ADDR_SIZE);
	        memset((void *)&mem_pool[dev_dongle_addr],0x0,BTIF_BD_ADDR_SIZE);
	        mem_pool[dev_xtal_fcap] = val;
	        mem_pool[dev_crc] = crc32(0,(uint8_t *)(&mem_pool[dev_reserv1]),(dev_data_len-dev_reserv1)*sizeof(uint32_t));
	        nvrec_trace("%s,%s,mem_pool[dev_crc]=%x.\n",nvdev_tag,__FUNCTION__,mem_pool[dev_crc]);
	    }
	}
	else
	{
		if(dev_sector_valid)
	    {
	        memcpy((void *)mem_pool,(void *)__factory_start,0x1000);
	        mem_pool[rev2_dev_xtal_fcap] = val;
	        mem_pool[rev2_dev_crc] = crc32(0,
				(uint8_t *)(&mem_pool[rev2_dev_section_start_reserved]),mem_pool[rev2_dev_data_len]);
		}
	    else
	    {
	        const char *localname = bt_get_local_name();
	        unsigned int namelen = strlen(localname);

	        btaddr = bt_get_local_address();
	        bleaddr = bt_get_ble_local_address();
	        mem_pool[rev2_dev_section_start_reserved] = 0;
	        mem_pool[rev2_dev_reserv2] = 0;
	        memcpy((void *)&mem_pool[rev2_dev_name],(void *)localname,(size_t)namelen);
			memcpy((void *)&mem_pool[rev2_dev_ble_name], (void *)bt_get_ble_local_name(), BLE_NAME_LEN_IN_NV);
	        nvrec_dev_rand_btaddr_gen(btaddr);
	        nvrec_dev_rand_btaddr_gen(bleaddr);
	        memcpy((void *)&mem_pool[rev2_dev_bt_addr],(void *)btaddr,BTIF_BD_ADDR_SIZE);
	        memcpy((void *)&mem_pool[rev2_dev_ble_addr],(void *)bleaddr,BTIF_BD_ADDR_SIZE);
	        memset((void *)&mem_pool[rev2_dev_dongle_addr],0x0,BTIF_BD_ADDR_SIZE);
	        mem_pool[rev2_dev_xtal_fcap] = val;
			mem_pool[rev2_dev_data_len] = (rev2_dev_section_end-rev2_dev_section_start_reserved)*sizeof(uint32_t);
	        mem_pool[rev2_dev_crc] = crc32(0,
				(uint8_t *)(&mem_pool[rev2_dev_section_start_reserved]),
				mem_pool[rev2_dev_data_len]);
			nvrec_trace("%s,%s,mem_pool[rev2_dev_crc]=%x.\n",
				nvdev_tag,__FUNCTION__,mem_pool[rev2_dev_crc]);
	    }
	}
}

void nvrec_dev_flash_flush(unsigned char *mempool)
{
    uint32_t lock;
#ifdef nv_record_debug
    uint32_t devdata[dev_data_len] = {0,};
    uint32_t recrc;
#endif

    if(NULL == mempool)
        return;
    lock = int_lock_global();
    pmu_flash_write_config();

    hal_norflash_erase(HAL_NORFLASH_ID_0,(uint32_t)NVRECORD_CACHE_2_UNCACHE(__factory_start),0x1000);
    hal_norflash_write(HAL_NORFLASH_ID_0,(uint32_t)NVRECORD_CACHE_2_UNCACHE(__factory_start),(uint8_t *)mempool,0x1000);//dev_data_len*sizeof(uint32_t));

    pmu_flash_read_config();
    int_unlock_global(lock);

#ifdef nv_record_debug
	if (NVREC_DEV_VERSION_1 == nv_record_dev_rev)
	{
	    memset(devdata,0x0,dev_data_len*sizeof(uint32_t));
	    memcpy(devdata,__factory_start,dev_data_len*sizeof(uint32_t));
	    recrc = crc32(0,(uint8_t *)(&devdata[dev_reserv1]),(dev_data_len-dev_reserv1)*sizeof(uint32_t));
	    nvrec_trace("%s,%d,devdata[dev_crc]=%x.recrc=%x\n",nvdev_tag,__LINE__,devdata[dev_crc],recrc);
	    if(devdata[dev_crc] != recrc)
	        assert(0);
	}
#endif
}

void nvrec_dev_rand_btaddr_gen(uint8_t* bdaddr)
{
    unsigned int seed;
    int i;

    OS_DUMP8("%x ",bdaddr,6);
    seed = hal_sys_timer_get();
    for(i=0;i<BTIF_BD_ADDR_SIZE;i++)
    {
        unsigned int randval;
        srand(seed);
        randval = rand();
        bdaddr[i] = (randval&0xff);
        seed += rand();
    }
    OS_DUMP8("%x ",bdaddr,6);
}

void nvrec_dev_set_xtal_fcap(unsigned int xtal_fcap)
{
    uint8_t *mempool = NULL;
    uint32_t lock;
#ifdef nv_record_debug
    uint32_t devdata[dev_data_len] = {0,};
    uint32_t recrc;
#endif

    syspool_init();
    syspool_get_buff(&mempool, 0x1000);
    nvrec_dev_data_fill_xtal_fcap((uint32_t *)mempool,(uint32_t)xtal_fcap);
    lock = int_lock_global();
    pmu_flash_write_config();

    hal_norflash_erase(HAL_NORFLASH_ID_0,(uint32_t)NVRECORD_CACHE_2_UNCACHE(__factory_start),0x1000);
    hal_norflash_write(HAL_NORFLASH_ID_0,(uint32_t)NVRECORD_CACHE_2_UNCACHE(__factory_start),(uint8_t *)mempool,0x1000);//dev_data_len*sizeof(uint32_t));

    pmu_flash_read_config();
    int_unlock_global(lock);
#ifdef nv_record_debug
	if (NVREC_DEV_VERSION_1 == nv_record_dev_rev)
	{
	    memset(devdata,0x0,dev_data_len*sizeof(uint32_t));
	    memcpy(devdata,__factory_start,dev_data_len*sizeof(uint32_t));
	    nvrec_trace("LN=%d,xtal fcap = %d",__LINE__,devdata[dev_xtal_fcap]);
	    recrc = crc32(0,(uint8_t *)(&devdata[dev_reserv1]),(dev_data_len-dev_reserv1)*sizeof(uint32_t));
	    nvrec_trace("%s,%d,devdata[dev_crc]=%x.recrc=%x\n",nvdev_tag,__LINE__,devdata[dev_crc],recrc);
	    if(devdata[dev_crc] != recrc)
	        assert(0);
	}
#endif
}

int nvrec_dev_get_xtal_fcap(unsigned int *xtal_fcap)
{
	unsigned int xtal_fcap_addr;
	if (NVREC_DEV_VERSION_1 == nv_record_dev_rev)
	{
    	xtal_fcap_addr = (unsigned int)(__factory_start + dev_xtal_fcap);
	}
	else
	{
		xtal_fcap_addr = (unsigned int)(__factory_start + rev2_dev_xtal_fcap);
	}

    unsigned int tmpval[1] = {0,};

    if(false == dev_sector_valid)
        return -1;
    if (NULL == xtal_fcap)
        return -1;
    memcpy((void *)tmpval,(void *)xtal_fcap_addr,sizeof(unsigned int));
    *xtal_fcap = tmpval[0];
    return 0;
}

int nvrec_dev_get_dongleaddr(bt_bdaddr_t *dongleaddr)
{
	unsigned int dongle_addr_pos;
	if (NVREC_DEV_VERSION_1 == nv_record_dev_rev)
	{
    	dongle_addr_pos = (unsigned int)(__factory_start + dev_dongle_addr);
	}
	else
	{
		dongle_addr_pos = (unsigned int)(__factory_start + rev2_dev_dongle_addr);
	}

    if(false == dev_sector_valid)
        return -1;
    if(NULL == dongleaddr)
        return -1;
    memcpy((void *)dongleaddr,(void *)dongle_addr_pos,BTIF_BD_ADDR_SIZE);
    return 0;
}

int nvrec_dev_get_btaddr(char *btaddr)
{
	unsigned int bt_addr_pos;
	if (NVREC_DEV_VERSION_1 == nv_record_dev_rev)
	{
    	bt_addr_pos = (unsigned int)(__factory_start + dev_bt_addr);
	}
	else
	{
		bt_addr_pos = (unsigned int)(__factory_start + rev2_dev_bt_addr);
	}

    if(false == dev_sector_valid)
        return 0;
    if(NULL == btaddr)
        return 0;
    memcpy((void *)btaddr,(void *)bt_addr_pos,BTIF_BD_ADDR_SIZE);
    return 1;
}

char* nvrec_dev_get_bt_name(void)
{
	return classics_bt_name;
}

const char* nvrec_dev_get_ble_name(void)
{
	if ((NVREC_DEV_VERSION_1 == nv_record_dev_rev) || (!dev_sector_valid))
	{
		return BLE_DEFAULT_NAME;
	}
	else
	{

		return (const char *)(&__factory_start[rev2_dev_ble_name]);
	}
}

#if defined(NVREC_BAIDU_DATA_SECTION)
#define BAIDU_DATA_FM_SPEC_VALUE (0xffee)
int nvrec_baidu_data_init(void)
{
    int _fmfreq = 0;
    if (!nvrec_config_has_section(nv_record_config.config, BAIDU_DATA_SECTIN)) {
        TRACE("no baidu section default, new one!");
        nvrec_config_set_int(nv_record_config.config, BAIDU_DATA_SECTIN, BAIDU_DATA_FM_KEY, BAIDU_DATA_DEF_FM_FREQ);
    }
    else {
        TRACE("has baidu section, fmfreq is %d", nvrec_config_get_int(nv_record_config.config, BAIDU_DATA_SECTIN, BAIDU_DATA_FM_KEY, BAIDU_DATA_FM_SPEC_VALUE));
        _fmfreq = nvrec_config_get_int(nv_record_config.config, BAIDU_DATA_SECTIN, BAIDU_DATA_FM_KEY, BAIDU_DATA_FM_SPEC_VALUE);
        if (_fmfreq == BAIDU_DATA_FM_SPEC_VALUE) {
            TRACE("fm bas bad value, set to %d", BAIDU_DATA_DEF_FM_FREQ);
            nvrec_config_set_int(nv_record_config.config, BAIDU_DATA_SECTIN, BAIDU_DATA_FM_KEY, BAIDU_DATA_DEF_FM_FREQ);
            _fmfreq = nvrec_config_get_int(nv_record_config.config, BAIDU_DATA_SECTIN, BAIDU_DATA_FM_KEY, BAIDU_DATA_FM_SPEC_VALUE);
            if (_fmfreq == BAIDU_DATA_FM_SPEC_VALUE) {
                TRACE("get fm freq still fail!!!");
                ASSERT(0, "nvrecord fail, system down!");
            }
        }
    }

    return 0;
}

int nvrec_get_fm_freq(void)
{
    int _fmfreq = 0;
    _fmfreq = nvrec_config_get_int(nv_record_config.config, BAIDU_DATA_SECTIN, BAIDU_DATA_FM_KEY, BAIDU_DATA_FM_SPEC_VALUE);
    if (_fmfreq == BAIDU_DATA_FM_SPEC_VALUE) {
        TRACE("%s : get fm freq fail, reinit fmfreq data", __func__);
        nvrec_baidu_data_init();
    }
    _fmfreq = nvrec_config_get_int(nv_record_config.config, BAIDU_DATA_SECTIN, BAIDU_DATA_FM_KEY, BAIDU_DATA_FM_SPEC_VALUE);
    if (_fmfreq == BAIDU_DATA_FM_SPEC_VALUE) {
        TRACE("%s : get fm freq still fail", __func__);
        ASSERT(0, "%s : nvrecord fail, system down!", __func__);
    }

    TRACE("%s:get fm freq %d", __func__, _fmfreq);
    return _fmfreq;
}

int nvrec_set_fm_freq(int fmfreq)
{
    int t = hal_sys_timer_get();
    TRACE("%s:fmfreq %d", __func__, fmfreq);
    nvrec_config_set_int(nv_record_config.config, BAIDU_DATA_SECTIN, BAIDU_DATA_FM_KEY, fmfreq);
    TRACE("%s: use %d ms", __func__, TICKS_TO_MS(hal_sys_timer_get()-t));
    nv_record_config.is_update = true;

#if defined(NVREC_BAIDU_DATA_FLUSH_DIRECT)
    nv_record_flash_flush_int(false);
#endif
    TRACE("%s: use %d ms", __func__, TICKS_TO_MS(hal_sys_timer_get()-t));

    return 0;
}

int nvrec_get_rand(char *rand)
{
    char * _rand = 0;

    if (rand == NULL)
        return 1;

#if 0
    _rand = nvrec_config_get_string(nv_record_config.config, BAIDU_DATA_SECTIN, BAIDU_DATA_RAND_KEY, BAIDU_DATA_DEF_RAND);
    if (strcmp(_rand, BAIDU_DATA_DEF_RAND)==0 || _rand == NULL) {
        TRACE("%s : get rand fail!", __func__);
        return 1;
    }
#else
    _rand = BAIDU_DATA_DEF_RAND;
#endif

    memcpy(rand, _rand, BAIDU_DATA_RAND_LEN);

    TRACE("%s:rand %s", __func__, rand);

    return 0;
}

int nvrec_set_rand(char *rand)
{
    int t = hal_sys_timer_get();
    if (rand != NULL)
        TRACE("%s:rand %s", __func__, rand);
    else
        TRACE("%s:rand nul", __func__);
    nvrec_config_set_string(nv_record_config.config, BAIDU_DATA_SECTIN, BAIDU_DATA_RAND_KEY, rand);
    TRACE("%s: use %d ms", __func__, TICKS_TO_MS(hal_sys_timer_get()-t));
    nv_record_config.is_update = true;

#if defined(NVREC_BAIDU_DATA_FLUSH_DIRECT)
    nv_record_flash_flush_int(false);
#endif
    TRACE("%s: use %d ms", __func__, TICKS_TO_MS(hal_sys_timer_get()-t));

    return 0;
}

int nvrec_clear_rand(void)
{
    nvrec_set_rand(NULL);
    return 0;
}

int nvrec_dev_get_sn(char *sn)
{
	unsigned int sn_addr;
	if (NVREC_DEV_VERSION_1 == nv_record_dev_rev)
	{
        return -1;
	}
	else
	{
		sn_addr = (unsigned int)(__factory_start + rev2_dev_prod_sn);
	}

    if(false == dev_sector_valid)
        return -1;
    if (NULL == sn)
        return -1;

    memcpy((void *)sn, (void *)sn_addr, BAIDU_DATA_SN_LEN);

    return 0;
}
#endif



