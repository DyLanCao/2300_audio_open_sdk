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
#if 0
#include <stdio.h>
#include "hal_trace.h"
#include "log_section.h"
#include <string.h>
#include "hal_timer.h"
#include "hal_norflash.h"
#include "norflash_api.h"
#include "cmsis.h"
#include "hal_cache.h"
#include "pmu.h"
#include "cmsis_os.h"
#include "cqueue.h"
#include "hal_sleep.h"
#include "app_utils.h"

#ifdef DEBUG_LOG_ENABLED
#ifdef __NEW_FLASH_MAP_ON__
#define DEBUG_TRACE_LOG_SECTION_SIZE    0x60000 // 0x80000->0x60000, ZHK @ 2019/02/21 CH-5173B

#ifdef DEBUG_LOG_ENABLED
uint32_t volatile fixed_dump_addr = 0x3c380000; // 0x3c320000->0x3c380000, ZHK @ 2019/02/21 CH-5173B
#endif

static uint32_t volatile __debug_trace_start = 0x3c381000; // 0x3c321000->0x3c381000, ZHK @ 2019/02/21 CH-5173B
#else
#define DEBUG_TRACE_LOG_SECTION_SIZE    0x100000

#ifdef DEBUG_LOG_ENABLED
uint32_t volatile fixed_dump_addr = 0x3c200000;
#endif

static uint32_t volatile __debug_trace_start = 0x3c201000;

#endif
static uint32_t section_size_debug_trace_log = (uint32_t)DEBUG_TRACE_LOG_SECTION_SIZE - 0x1000;
#define LOG_DUMP_PREFIX     "__LOG_DUMP:"
#define LOG_DUMP_TRACE(fmt, ...)  TRACE(fmt, ##__VA_ARGS__)
// #define TRACE_EX                        TRACE

#define LOG_COMPRESS_RATION             1

#define FLASH_SECTOR_SIZE_IN_BYTES      0x1000

#define BRIDGE_TRACE_LOG_BUFFER_SIZE    0x1000
#define HISTORY_TRACE_LOG_BUFFER_SIZE   0x8000
#define MARGIN_HISTORY_TRACE_LOG_BUFFER_SIZE    (BRIDGE_TRACE_LOG_BUFFER_SIZE*6)

#define SIZE_TO_PUSH_HISTORY_TRACE_LOG      (BRIDGE_TRACE_LOG_BUFFER_SIZE/LOG_COMPRESS_RATION)
//extern uint32_t volatile __log_addr_start[];
extern uint32_t __log_dump_start[];
uint32_t log_start_addr = 0;

#if HISTORY_TRACE_LOG_BUFFER_SIZE%BRIDGE_TRACE_LOG_BUFFER_SIZE
#error hisotry trace log buffer size must be integral multiple of the bridge trace log buffer!
#endif

static uint32_t trace_log_size_in_bridge_buffer = 0;
static uint8_t bridge_trace_log_buffer[BRIDGE_TRACE_LOG_BUFFER_SIZE];

static uint32_t trace_log_size_in_history_buffer = 0;
static uint32_t offset_to_fill_trace_log_history_buffer = 0;
static uint8_t history_trace_log_buffer[HISTORY_TRACE_LOG_BUFFER_SIZE];

#ifdef DEBUG_LOG_ENABLED
static uint32_t offset_in_trace_section_to_write = 0;
#endif
#define FLUSH_IN_IDLE_STATE

#ifdef FLUSH_IN_IDLE_STATE
#define Q_SIZE_FLUSH_IDLE_STATE (0x1000*16) //64K
#define SZ_OF_BLOCK_FLUSH   (0x1000)
static uint32_t flush_buff_idle_state[Q_SIZE_FLUSH_IDLE_STATE/sizeof(uint32_t)];
static uint32_t flush_block_buff[SZ_OF_BLOCK_FLUSH/sizeof(uint32_t)];
static CQueue   flush_buff_q;
static uint32_t offset_of_flush_addr = 0;
void complete_last_operation_of_flush(void);
extern   void nv_record_flash_complete(void );

#endif
enum NORFLASH_API_RET_T flash_op_eret = NORFLASH_API_OK;
enum NORFLASH_API_RET_T flash_op_wret = NORFLASH_API_OK;
static bool flush_immediately = false; //crash or shutdown case
static bool flush_addr_progress = false; //crash or shutdown case
static bool flush_log_progress = false; //crash or shutdown case
static bool LOG_flush_disable = false; //crash or shutdown case
static uint32_t g_next_start_addr = 0;

osMutexDef(log_dump_mutex);
// static uint8_t isDumpLogPendingForFlush = false;
extern bool bt_media_is_media_active(void);
extern bool btapp_hfp_is_call_active(void);

#define DUMP_LOG_SUPPORT_SUSPEND    1

static enum NORFLASH_API_RET_T _flash_api_read(uint8_t* buffer, uint32_t addr, uint32_t len)
{
    ret norflash_sync_read(NORFLASH_API_MODULE_ID_TRACE_DUMP,
                addr,
                buffer,
                len);
}

static void _flash_api_flush(void)
{
    hal_trace_pause();
    do
    {
        if(flush_immediately)
        {
            LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:%d flush_immediately.",__func__,__LINE__);
            break;
        }
        norflash_api_flush();
        osDelay(10);
    } while(!norflash_api_buffer_is_free(NORFLASH_API_MODULE_ID_TRACE_DUMP));

    hal_trace_continue();
}

static enum NORFLASH_API_RET_T _flash_api_erase(uint32_t addr)
{
    uint32_t lock;
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;

    do
    {
        if(flush_immediately)
        {
            LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:%d flush_immediately.",__func__,__LINE__);
            ret = NORFLASH_API_OK;
            break;
        }
        lock = int_lock_global();
        hal_trace_pause();

#if DUMP_LOG_SUPPORT_SUSPEND
        ret = norflash_api_erase(NORFLASH_API_MODULE_ID_TRACE_DUMP,
            addr,
            FLASH_SECTOR_SIZE_IN_BYTES,true);
#else
        ret = norflash_api_erase(NORFLASH_API_MODULE_ID_TRACE_DUMP,
            addr,
            FLASH_SECTOR_SIZE_IN_BYTES,false);
#endif

        hal_trace_continue();
        int_unlock_global(lock);

        if (NORFLASH_API_OK == ret)
        {
            TRACE("%s: norflash_api_erase ok!",__func__);
            break;
        }
        else if(NORFLASH_API_BUFFER_FULL == ret)
        {
            TRACE("Flash async cache overflow! To flush.");
            _flash_api_flush();
        }
        else
        {
            ASSERT(0, "_flash_api_erase: norflash_api_erase failed. ret = %d", ret);
        }
    } while (1);

    return ret;
}

static enum NORFLASH_API_RET_T _flash_api_write(uint32_t addr, uint8_t* ptr, uint32_t len)
{
    uint32_t lock;
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;

    do
    {
        if(flush_immediately)
        {
            LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:%d flush_immediately.",__func__,__LINE__);
            ret = NORFLASH_API_OK;
            break;
        }

        lock = int_lock_global();
        hal_trace_pause();

#if DUMP_LOG_SUPPORT_SUSPEND
        ret = norflash_api_write(NORFLASH_API_MODULE_ID_TRACE_DUMP,
            addr,
            ptr,
            len,
            true);
#else
        ret = norflash_api_write(NORFLASH_API_MODULE_ID_TRACE_DUMP,
            addr,
            ptr,
            len,
            false);
#endif

        hal_trace_continue();

        int_unlock_global(lock);

        if (NORFLASH_API_OK == ret)
        {
            TRACE("%s: norflash_api_write ok!",__func__);
            break;
        }
        else if (NORFLASH_API_BUFFER_FULL == ret)
        {
            TRACE("Flash async cache overflow! To flush.");
            _flash_api_flush();
        }
        else
        {
            ASSERT(0,"_flash_api_write: norflash_api_write failed. ret = %d",ret);
        }
    }while (1);

    return ret;
}



static void compress_log(uint8_t* input, uint8_t* output, uint32_t len)
{
    // TODO:
    memcpy(output, input, len);
}


uint32_t refresh_start_addr(uint32_t block_size, bool force)
{
    uint32_t lock;
    uint32_t volatile curr_start_addr = 0;

    hal_trace_pause();
    curr_start_addr = *((uint32_t volatile *)__log_dump_start);
    if(!force)
        LOG_DUMP_TRACE(LOG_DUMP_PREFIX"refresh_start_addr.__log_addr_start[]=%x curr_start_addr=0x%x", *((uint32_t volatile *)__log_dump_start),curr_start_addr);

    g_next_start_addr = curr_start_addr + block_size;
    if((g_next_start_addr >= section_size_debug_trace_log) || (curr_start_addr == 0xffffffff))
    {
        curr_start_addr = 0;
        g_next_start_addr = curr_start_addr + block_size;
    }
    if(!force)
    {
        LOG_DUMP_TRACE(LOG_DUMP_PREFIX"refresh_start_addr next_start_addr=0x%x block_size=0x%x", g_next_start_addr, block_size);
    }
    else
    {
        lock = int_lock();
        flash_op_eret = norflash_api_erase(NORFLASH_API_MODULE_ID_TRACE_DUMP, (uint32_t)__log_dump_start, SZ_OF_BLOCK_FLUSH,false);

        flash_op_eret = norflash_api_write(NORFLASH_API_MODULE_ID_TRACE_DUMP, (uint32_t )__log_dump_start, (const uint8_t* )&g_next_start_addr, 4,false);
        int_unlock(lock);
        goto pre_complete;
    }

    //DeCQueue(&flush_buff_q,(CQItemType *)flush_block_buff,SZ_OF_BLOCK_FLUSH);
    flash_op_eret = _flash_api_erase((uint32_t)__log_dump_start);
    // LOG_DUMP_TRACE(LOG_DUMP_PREFIX"refresh_start_addr.hal_norflash_erase_suspend RUN1!!! wret=%d", flash_op_wret);

    if(flush_immediately)
    {
        LOG_DUMP_TRACE(LOG_DUMP_PREFIX"refresh_start_addr ! flush_immediately=%d", flush_immediately);
        goto pre_complete;
    }

    flash_op_wret = _flash_api_write((uint32_t )__log_dump_start, (const uint8_t* )&g_next_start_addr, 4);
   // LOG_DUMP_TRACE(LOG_DUMP_PREFIX"2.hal_norflash_write_suspend RUN2!!! wret=%d", flash_op_wret);

    if((flash_op_eret != HAL_NORFLASH_OK)||(flash_op_wret != HAL_NORFLASH_OK)) {
        LOG_DUMP_TRACE(LOG_DUMP_PREFIX"!!!!! refresh_start_addr W ERET %d  WRET %d ",flash_op_eret,flash_op_wret);
    }
    //offset_of_flush_addr = __log_dump_start[0];
    flash_op_wret = HAL_NORFLASH_OK;

pre_complete:
    flash_op_wret = HAL_NORFLASH_OK;
    flush_addr_progress = false;
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"refresh_start_addr end offset_of_flush_addr=%x ", *((uint32_t volatile *)__log_dump_start));
    return curr_start_addr;
}

#ifndef FLUSH_IN_IDLE_STATE
static void dump_data_into_debug_log_section(uint8_t* ptrSource, uint32_t lengthToBurn,
    uint32_t offsetInFlashToProgram)
{
    uint32_t lock;
    uint32_t current_start_addr = 0;
    uint32_t next_start_addr = 0;
    enum HAL_NORFLASH_RET_T wRet = 0;

    lock = int_lock_global();

    //LOG_DUMP_TRACE(LOG_DUMP_PREFIX"offsetInFlashToProgram=0x%08x, debug_trace_start=0x%08x,section_size_debug_trace_log=0x%08x",
        //offsetInFlashToProgram, __debug_trace_start, section_size_debug_trace_log);
    //wRet = hal_norflash_read(HAL_NORFLASH_ID_0, fixed_dump_addr, (uint8_t*)&current_start_addr, 4);
    current_start_addr = (uint32_t)fixed_dump_addr;
    if((current_start_addr == 0xffffffff)
        || (current_start_addr >= (__debug_trace_start + section_size_debug_trace_log)))
    {
        current_start_addr = offsetInFlashToProgram;
    }
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:%d,Write %d bytes log into 0x%x",
             __func__,__LINE__,lengthToBurn, current_start_addr);

    next_start_addr = current_start_addr + lengthToBurn;

    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"before write, next_start_addr=0x%08x", next_start_addr);
    wRet = _flash_api_erase(fixed_dump_addr);
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"hal_norflash_erase wRet=%d", wRet);
    wRet = _flash_api_write(fixed_dump_addr, (uint8_t*)&(next_start_addr), 4);
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"hal_norflash_write wRet=%d", wRet);

    _flash_api_read(fixed_dump_addr, (uint8_t*)&next_start_addr, 4);
    //memcpy((uint8_t*)&next_start_addr,(uint8_t*)fixed_dump_addr, 4);
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"after write, next_start_addr=0x%08x", next_start_addr);


    uint32_t preBytes = (FLASH_SECTOR_SIZE_IN_BYTES - (current_start_addr%FLASH_SECTOR_SIZE_IN_BYTES))%FLASH_SECTOR_SIZE_IN_BYTES;
    if (lengthToBurn < preBytes)
    {
        preBytes = lengthToBurn;
    }

    uint32_t middleBytes = 0;
    if (lengthToBurn > preBytes)
    {
       middleBytes = ((lengthToBurn - preBytes)/FLASH_SECTOR_SIZE_IN_BYTES*FLASH_SECTOR_SIZE_IN_BYTES);
    }
    uint32_t postBytes = 0;
    if (lengthToBurn > (preBytes + middleBytes))
    {
        postBytes = (current_start_addr + lengthToBurn)%FLASH_SECTOR_SIZE_IN_BYTES;
    }

    if (preBytes > 0)
    {
        _flash_api_write(current_start_addr,
            ptrSource, preBytes);

        ptrSource += preBytes;
        current_start_addr += preBytes;
    }

    uint32_t sectorIndexInFlash = current_start_addr/FLASH_SECTOR_SIZE_IN_BYTES;

    if (middleBytes > 0)
    {
        uint32_t sectorCntToProgram = middleBytes/FLASH_SECTOR_SIZE_IN_BYTES;
        for (uint32_t sector = 0;sector < sectorCntToProgram;sector++)
        {
            _flash_api_erase(sectorIndexInFlash*FLASH_SECTOR_SIZE_IN_BYTES);

            _flash_api_write(sectorIndexInFlash*FLASH_SECTOR_SIZE_IN_BYTES,
                ptrSource + sector*FLASH_SECTOR_SIZE_IN_BYTES, FLASH_SECTOR_SIZE_IN_BYTES);

            sectorIndexInFlash++;
        }

        ptrSource += middleBytes;
    }

    if (postBytes > 0)
    {
        _flash_api_erase(sectorIndexInFlash*FLASH_SECTOR_SIZE_IN_BYTES, FLASH_SECTOR_SIZE_IN_BYTES);
        _flash_api_write(sectorIndexInFlash*FLASH_SECTOR_SIZE_IN_BYTES,
                ptrSource, postBytes);
    }
    int_unlock_global(lock);
}
#endif

static void flush_pending_trace_log(void)
{
#ifdef FLUSH_IN_IDLE_STATE
    uint32_t lock;
    //LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s\n",__func__);
    //LOG_DUMP_TRACE(LOG_DUMP_PREFIX"offset_to_fill_trace_log_history_buffer=%d,trace_log_size_in_bridge_buffer=%d\n",offset_to_fill_trace_log_history_buffer,trace_log_size_in_bridge_buffer);
    lock = int_lock();

    compress_log(bridge_trace_log_buffer,
        &history_trace_log_buffer[offset_to_fill_trace_log_history_buffer],
        trace_log_size_in_bridge_buffer);

    trace_log_size_in_history_buffer += (trace_log_size_in_bridge_buffer/LOG_COMPRESS_RATION);
    if (trace_log_size_in_history_buffer > HISTORY_TRACE_LOG_BUFFER_SIZE)
    {
        trace_log_size_in_history_buffer = HISTORY_TRACE_LOG_BUFFER_SIZE;
    }

    offset_to_fill_trace_log_history_buffer +=
        (trace_log_size_in_bridge_buffer/LOG_COMPRESS_RATION);

    uint32_t historyTraceLogSize = trace_log_size_in_history_buffer;
    uint32_t leftSpaceInDebugLogSection = section_size_debug_trace_log -
        offset_in_trace_section_to_write;
    uint32_t seqnum = 0;
    //LOG_DUMP_TRACE(LOG_DUMP_PREFIX"leftSpaceInDebugLogSection= %d, historyTraceLogSize= %d",
        //leftSpaceInDebugLogSection, historyTraceLogSize);

    if (leftSpaceInDebugLogSection < historyTraceLogSize)
    {
        if(EnCQueue(&flush_buff_q,(CQItemType *)&history_trace_log_buffer[seqnum],leftSpaceInDebugLogSection) == CQ_ERR){
         // if(!flush_immediately)
          //  LOG_DUMP_TRACE(LOG_DUMP_PREFIX"Drop trace len 0x%x",leftSpaceInDebugLogSection);
        }

        //LOG_DUMP_TRACE(LOG_DUMP_PREFIX"offset in trace log section is %d at line %d",
            //offset_in_trace_section_to_write, __LINE__);

        offset_in_trace_section_to_write = 0;

        seqnum += leftSpaceInDebugLogSection;

        historyTraceLogSize -= leftSpaceInDebugLogSection;
    }

    if(EnCQueue(&flush_buff_q,(CQItemType *)&history_trace_log_buffer[seqnum],historyTraceLogSize) == CQ_ERR){
       // if(!flush_immediately)
       //     LOG_DUMP_TRACE(LOG_DUMP_PREFIX"Drop trace len 0x%x",leftSpaceInDebugLogSection);
    }

    offset_in_trace_section_to_write += historyTraceLogSize;

    //LOG_DUMP_TRACE(LOG_DUMP_PREFIX"offset in trace log section is %d at line %d", offset_in_trace_section_to_write, __LINE__);

    trace_log_size_in_bridge_buffer = 0;
    trace_log_size_in_history_buffer = 0;
    offset_to_fill_trace_log_history_buffer = 0;

    if(flush_immediately) {
        CQItemType *e1 = NULL ;
        unsigned int len1 = 0;
        CQItemType *e2 = NULL ;
        unsigned int len2 = 0;
        unsigned int len = 0;
        uint8_t block_count = 0;

        //nv_record_flash_complete();
        //complete_last_operation_of_flush();
        if(CQ_ERR != PeekCQueue(&flush_buff_q, LengthOfCQueue(&flush_buff_q), &e1, &len1, &e2, &len2))
        {
            offset_of_flush_addr = (offset_of_flush_addr + len1 +len2) > DEBUG_TRACE_LOG_SECTION_SIZE ? 0: offset_of_flush_addr;
        }

        len = len1 + len2;
#if 1
        block_count = ((len/SZ_OF_BLOCK_FLUSH)*SZ_OF_BLOCK_FLUSH < len )?(len/SZ_OF_BLOCK_FLUSH)+1:(len/SZ_OF_BLOCK_FLUSH);
#else
        block_count = (len/SZ_OF_BLOCK_FLUSH)+1;
#endif
        offset_of_flush_addr = refresh_start_addr((block_count)*SZ_OF_BLOCK_FLUSH,true);
        if(len > 0){
            _flash_api_erase(__debug_trace_start + offset_of_flush_addr, (block_count)*SZ_OF_BLOCK_FLUSH);
            if(len1 > 0)
                _flash_api_write(__debug_trace_start + offset_of_flush_addr,e1,len1);
            offset_of_flush_addr += len1;
            if(len2 > 0)
                _flash_api_write(__debug_trace_start + offset_of_flush_addr,e2,len2);

            offset_of_flush_addr += len2;
            DeCQueue(&flush_buff_q, 0,len1 +len2);
        }
    }

    int_unlock(lock);
#else
    uint32_t lock;
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s\n",__func__);
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"offset_to_fill_trace_log_history_buffer=%d,trace_log_size_in_bridge_buffer=%d\n",offset_to_fill_trace_log_history_buffer,trace_log_size_in_bridge_buffer);
    lock = int_lock();

    compress_log(bridge_trace_log_buffer,
        &history_trace_log_buffer[offset_to_fill_trace_log_history_buffer],
        trace_log_size_in_bridge_buffer);

    trace_log_size_in_history_buffer += (trace_log_size_in_bridge_buffer/LOG_COMPRESS_RATION);
    if (trace_log_size_in_history_buffer > HISTORY_TRACE_LOG_BUFFER_SIZE)
    {
        trace_log_size_in_history_buffer = HISTORY_TRACE_LOG_BUFFER_SIZE;
    }

    offset_to_fill_trace_log_history_buffer +=
        (trace_log_size_in_bridge_buffer/LOG_COMPRESS_RATION);

    uint32_t historyTraceLogSize = trace_log_size_in_history_buffer;
    uint32_t leftSpaceInDebugLogSection = section_size_debug_trace_log -
        offset_in_trace_section_to_write;
    uint32_t seqnum = 0;
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"leftSpaceInDebugLogSection= %d, historyTraceLogSize= %d",
        leftSpaceInDebugLogSection, historyTraceLogSize);

    if (leftSpaceInDebugLogSection < historyTraceLogSize)
    {
        dump_data_into_debug_log_section(&history_trace_log_buffer[seqnum],
            leftSpaceInDebugLogSection,
            (uint32_t)(__debug_trace_start) +
            offset_in_trace_section_to_write);

        LOG_DUMP_TRACE(LOG_DUMP_PREFIX"offset in trace log section is %d at line %d",
            offset_in_trace_section_to_write, __LINE__);

        offset_in_trace_section_to_write = 0;

        seqnum += leftSpaceInDebugLogSection;

        historyTraceLogSize -= leftSpaceInDebugLogSection;
    }

    dump_data_into_debug_log_section(&history_trace_log_buffer[seqnum],
            historyTraceLogSize,
            (uint32_t)(__debug_trace_start) +
            offset_in_trace_section_to_write);

    offset_in_trace_section_to_write += historyTraceLogSize;

    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"offset in trace log section is %d at line %d", offset_in_trace_section_to_write, __LINE__);

	//test
	//test_dump_log_from_flash(__debug_trace_start, offset_in_trace_section_to_write);

    trace_log_size_in_bridge_buffer = 0;
    trace_log_size_in_history_buffer = 0;
    offset_to_fill_trace_log_history_buffer = 0;

    int_unlock(lock);
#endif
}

void set_flush_flag_disable(bool disalbe)
{
    LOG_flush_disable = disalbe;
}

void set_flush_immediately_flag(void)
{
    //set_flush_flag_disable(true);
    flush_immediately = true;
    //TRACE("%s flush_immediately=%d", __func__, flush_immediately);
}

void clear_flush_immediately_flag(void)
{
    flush_immediately = false;
    //TRACE("%s flush_immediately=%d", __func__, flush_immediately);
}

void dump_whole_logs(void)
{
    uint32_t lock;
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s RUN!!!", __func__);
    lock = int_lock();
    set_flush_immediately_flag();
    int_unlock(lock);
#ifdef DEBUG_LOG_ENABLED
    flush_pending_trace_log();
#endif
    lock = int_lock();
    clear_flush_immediately_flag();
    int_unlock(lock);
}


static bool isCantDump = false;
void logsection_set_dont_dump(bool cantdump)
{
    isCantDump = cantdump;
}


void push_trace_log_into_bridge_buffer(const unsigned char *ptr, unsigned int len)
{
	//LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s ENTER!!!", __func__);
	//LOG_DUMP_TRACE(LOG_DUMP_PREFIX"section_size_debug_trace_log=%d",section_size_debug_trace_log);
    uint32_t lock;
    uint32_t seqnum = 0, leftBytes;
    uint32_t sizeToPushBridgeBuffer = 0;
    bool isToFlushPendingTraceLog = false;
    if (isCantDump || ((int)section_size_debug_trace_log) <= 0)
        return;
    lock = int_lock();

    leftBytes = len;

    while ((trace_log_size_in_bridge_buffer + leftBytes) >= BRIDGE_TRACE_LOG_BUFFER_SIZE)
    {
        sizeToPushBridgeBuffer = BRIDGE_TRACE_LOG_BUFFER_SIZE - trace_log_size_in_bridge_buffer;

        memcpy(((uint8_t *)bridge_trace_log_buffer) + trace_log_size_in_bridge_buffer,
                ptr + seqnum, sizeToPushBridgeBuffer);
        if (offset_to_fill_trace_log_history_buffer+BRIDGE_TRACE_LOG_BUFFER_SIZE < HISTORY_TRACE_LOG_BUFFER_SIZE)
        {
            compress_log(bridge_trace_log_buffer,
                    &history_trace_log_buffer[offset_to_fill_trace_log_history_buffer],
                    BRIDGE_TRACE_LOG_BUFFER_SIZE);

            trace_log_size_in_history_buffer += SIZE_TO_PUSH_HISTORY_TRACE_LOG;
            offset_to_fill_trace_log_history_buffer += SIZE_TO_PUSH_HISTORY_TRACE_LOG;
        }
        else
        {
            LOG_DUMP_TRACE(LOG_DUMP_PREFIX"Losing trace");
        }
#if 0
        if ((HISTORY_TRACE_LOG_BUFFER_SIZE-MARGIN_HISTORY_TRACE_LOG_BUFFER_SIZE) <=
                offset_to_fill_trace_log_history_buffer)
#else
        if (0x1000 <= offset_to_fill_trace_log_history_buffer)
#endif
        {
#ifdef FLUSH_IN_IDLE_STATE
            isToFlushPendingTraceLog = true;
#else

            if (bt_media_is_media_active())
            {
                LOG_DUMP_TRACE(LOG_DUMP_PREFIX"bt_media_is_media_active=%d",bt_media_is_media_active());
                isToFlushPendingTraceLog = false;
            }
            else
            {
                isToFlushPendingTraceLog = true;
            }
#endif
        }
        leftBytes -= sizeToPushBridgeBuffer;
        seqnum += sizeToPushBridgeBuffer;
        trace_log_size_in_bridge_buffer = 0;
    }

    sizeToPushBridgeBuffer = leftBytes;
    memcpy(((uint8_t *)bridge_trace_log_buffer) + trace_log_size_in_bridge_buffer,
            ptr + seqnum, sizeToPushBridgeBuffer);
    trace_log_size_in_bridge_buffer += sizeToPushBridgeBuffer;

    int_unlock(lock);

    if (isToFlushPendingTraceLog&&!LOG_flush_disable&&!flush_log_progress)
    {
#ifdef DEBUG_LOG_ENABLED
        flush_pending_trace_log();
#endif
    }
}

void clear_dump_log(void)
{
    uint32_t lock = int_lock_global();
    _flash_api_erase(HAL_NORFLASH_ID_0, (uint32_t)(__debug_trace_start),
        section_size_debug_trace_log);
    int_unlock_global(lock);
}

void trace_log_state_handler(enum HAL_TRACE_STATE_T state)
{
    if (state == HAL_TRACE_STATE_CRASH_END) {
        dump_whole_logs();
    }
}

void complete_last_operation_of_flush(void)
{
    if((flash_op_eret == HAL_NORFLASH_SUSPENDED)||(flash_op_wret == HAL_NORFLASH_SUSPENDED))
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s flash_op_eret=%d flash_op_wret=%d g_next_start_addr=0x%08x addr?%d ", __func__, flash_op_eret, flash_op_wret, g_next_start_addr,flush_addr_progress);
    uint32_t lock = int_lock();
    if(flash_op_eret == HAL_NORFLASH_SUSPENDED){
        hal_norflash_erase_resume(HAL_NORFLASH_ID_0,0);
        flash_op_eret = HAL_NORFLASH_OK;

        if(!flush_addr_progress) {
            hal_norflash_write(HAL_NORFLASH_ID_0, __debug_trace_start+offset_of_flush_addr, (const uint8_t*)flush_block_buff, SZ_OF_BLOCK_FLUSH);
        }
        else {
            // LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s g_next_start_addr=0x%08x", __func__,g_next_start_addr);
             hal_norflash_write(HAL_NORFLASH_ID_0, (uint32_t )__log_dump_start, (const uint8_t* )&g_next_start_addr, 4);
        }
        flash_op_wret = HAL_NORFLASH_OK;
    }

    if(flash_op_wret == HAL_NORFLASH_SUSPENDED){
        hal_norflash_write_resume(HAL_NORFLASH_ID_0,0);
        flash_op_wret = HAL_NORFLASH_OK;

    }
    int_unlock(lock);
}
#ifdef FLUSH_IN_IDLE_STATE

int idle_flush_to_flush(void)
{
    uint32_t lock;
    uint32_t start_addr = 0;
    //bool refresh_flag = true;

    //app_sysfreq_req(APP_SYSFREQ_USER_APP_8, APP_SYSFREQ_208M);

    lock = int_lock();
    flush_log_progress = true;
    hal_trace_pause();

    pmu_flash_write_config();
    //if((offset_of_flush_addr + SZ_OF_BLOCK_FLUSH) > section_size_debug_trace_log)
     //   offset_of_flush_addr = 0;

    int_unlock(lock);

    while(LengthOfCQueue(&flush_buff_q) > SZ_OF_BLOCK_FLUSH){

        start_addr = refresh_start_addr(SZ_OF_BLOCK_FLUSH,false);

        if(flush_immediately)
            goto pre_complete;
        offset_of_flush_addr = start_addr;

        LOG_DUMP_TRACE(LOG_DUMP_PREFIX"2.__log_dump_start[0]=0x%08x, offset_of_flush_addr=0x%x, start_addr=0x%08x",
        *((uint32_t volatile *)__log_dump_start), offset_of_flush_addr, start_addr);

        lock = int_lock();
        DeCQueue(&flush_buff_q,(CQItemType *)flush_block_buff,SZ_OF_BLOCK_FLUSH);
        flash_op_eret = hal_norflash_erase_suspend(HAL_NORFLASH_ID_0, __debug_trace_start+offset_of_flush_addr, SZ_OF_BLOCK_FLUSH,1);
        int_unlock(lock);
      //  LOG_DUMP_TRACE(LOG_DUMP_PREFIX"1.hal_norflash_erase_suspend RUN1!!! wret=%d", flash_op_wret);

        while(flash_op_eret == HAL_NORFLASH_SUSPENDED) {
            osThreadYield();
            //osDelay(1);
            if(flush_immediately)
            {
              //  LOG_DUMP_TRACE(LOG_DUMP_PREFIX"hal_norflash_erase_suspend resume! flush_immediately=%d", flush_immediately);
                goto pre_complete;
            }
            if(flash_op_eret == HAL_NORFLASH_OK)
                goto pre_complete;
            lock = int_lock();
            if(flash_op_eret == HAL_NORFLASH_SUSPENDED)
                flash_op_eret = hal_norflash_erase_resume(HAL_NORFLASH_ID_0, 1);
            int_unlock(lock);
        }
        flash_op_eret = HAL_NORFLASH_OK;

        lock = int_lock();
        flash_op_wret = hal_norflash_write_suspend(HAL_NORFLASH_ID_0, __debug_trace_start+offset_of_flush_addr, (const uint8_t*)flush_block_buff, SZ_OF_BLOCK_FLUSH, 1);
        int_unlock(lock);
      //  LOG_DUMP_TRACE(LOG_DUMP_PREFIX"2.hal_norflash_write_suspend RUN2!!! wret=%d", flash_op_wret);

        while(flash_op_wret == HAL_NORFLASH_SUSPENDED) {
            osThreadYield();
            //osDelay(1);
            if(flush_immediately)
            {
               // LOG_DUMP_TRACE(LOG_DUMP_PREFIX"hal_norflash_write_suspend resume! flush_immediately=%d", flush_immediately);
                goto pre_complete;
            }
            if(flash_op_wret == HAL_NORFLASH_OK)
                goto pre_complete;
            lock = int_lock();
            if(flash_op_wret == HAL_NORFLASH_SUSPENDED)
                flash_op_wret = hal_norflash_write_resume(HAL_NORFLASH_ID_0, 1);
            //LOG_DUMP_TRACE(LOG_DUMP_PREFIX"hal_norflash_write_resume RUN!!! wret=%d", flash_op_wret);
            int_unlock(lock);
        }

        lock = int_lock();
        offset_of_flush_addr += SZ_OF_BLOCK_FLUSH;
        flash_op_wret = HAL_NORFLASH_OK;
        int_unlock(lock);
        LOG_DUMP_TRACE(LOG_DUMP_PREFIX"end write opreation offset_of_flush_addr=%p", offset_of_flush_addr);
    }

pre_complete:
    lock = int_lock();
    pmu_flash_read_config();

    hal_trace_continue();
    int_unlock(lock);
    //app_sysfreq_req(APP_SYSFREQ_USER_APP_8, APP_SYSFREQ_32K);
    flush_log_progress = false;
    return 0;
}



#endif
void init_dump_log(void)
{
    hal_trace_app_register(trace_log_state_handler, push_trace_log_into_bridge_buffer);
#ifdef FLUSH_IN_IDLE_STATE
    InitCQueue(&flush_buff_q, Q_SIZE_FLUSH_IDLE_STATE,
                (CQItemType *)flush_buff_idle_state);
   // hal_sleep_set_sleep_hook(HAL_SLEEP_HOOK_USER_TRACE, idle_flush_to_flush);
#endif
}

uint8_t test_buff_r[FLASH_SECTOR_SIZE_IN_BYTES];

uint32_t test_dump_log_from_flash(uint32_t addr,uint32_t size)
{
	//uint32_t start_addr;
	uint32_t i;
	//uint8_t value = 0;
	int32_t ret = 0;
	enum HAL_NORFLASH_RET_T result = HAL_NORFLASH_OK;

	LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s enter!!!", __func__);

	for(i = 0; i < size/FLASH_SECTOR_SIZE_IN_BYTES; i++)
	{
		result = hal_norflash_read(HAL_NORFLASH_ID_0,(uint32_t)(addr)+ (i*FLASH_SECTOR_SIZE_IN_BYTES),
			test_buff_r,FLASH_SECTOR_SIZE_IN_BYTES);
		if(result != HAL_NORFLASH_OK)
		{
			ret = -1;
			//LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s ret=%d", __func__, ret);
			goto _func_end;
		}
		LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s", test_buff_r);
	}
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s end!!!", __func__);

_func_end:
	LOG_DUMP_TRACE(LOG_DUMP_PREFIX"_debug: flash checking end. ret = %d.",ret);
	return ret;
}

#endif

#else
#include <stdio.h>
#include <string.h>
#include "hal_trace.h"
#include "log_section.h"
#include <string.h>
#include "hal_timer.h"
#include "hal_norflash.h"
#include "norflash_api.h"
#include "cmsis.h"
#include "hal_cache.h"
#include "hal_sleep.h"
#include "pmu.h"

#define LOG_DUMP_PREFIX     "__LOG_DUMP:"
#define LOG_DUMP_TRACE(fmt, ...)   TRACE(fmt, ##__VA_ARGS__)
#define LOG_DUMP_TRACE_FORCE(fmt, ...) TRACE(fmt, ##__VA_ARGS__)
#define DUMP_LOG_ALIGN(x,a)  (uint32_t)(((x + a - 1)/a) * a)

extern uint32_t __log_dump_start;
extern uint32_t __log_dump_end;

static const uint32_t log_dump_flash_start_addr = (uint32_t)&__log_dump_start;
static const uint32_t log_dump_flash_end_addr =  (uint32_t)&__log_dump_end;
static uint32_t log_dump_flash_len;

static DATA_BUFFER data_buffer_list[LOG_DUMP_SECTOR_BUFFER_COUNT];
static uint32_t log_dump_w_seqnum = 0;
static uint32_t log_dump_f_seqnum = 0;
static uint32_t log_dump_flash_offs = 0;
static uint32_t log_dump_cur_dump_seqnum;
static bool log_dump_is_init = false;
static bool log_dump_is_immediately;
static enum LOG_DUMP_FLUSH_STATE log_dump_flash_state = LOG_DUMP_FLASH_STATE_IDLE;

static enum NORFLASH_API_RET_T _flash_api_read(uint32_t addr, uint8_t* buffer, uint32_t len)
{

    return norflash_sync_read(NORFLASH_API_MODULE_ID_TRACE_DUMP,
                addr,
                buffer,
                len);
}

static void _flash_api_flush(void)
{
    hal_trace_pause();
    do
    {
        norflash_api_flush();
        //osDelay(10);
    } while(!norflash_api_buffer_is_free(NORFLASH_API_MODULE_ID_TRACE_DUMP));

    hal_trace_continue();
}

static enum NORFLASH_API_RET_T _flash_api_erase(uint32_t addr,
                                                      bool is_suspend)
{
    uint32_t lock;
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;

    do
    {
        if(log_dump_is_immediately && is_suspend)
        {
            LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s: log_dump_is_immediately!",__func__);
            break;
        }
        lock = int_lock_global();
        hal_trace_pause();
        ret = norflash_api_erase(NORFLASH_API_MODULE_ID_TRACE_DUMP,
                                addr,
                                LOG_DUMP_SECTOR_SIZE,
                                is_suspend);
        hal_trace_continue();
        int_unlock_global(lock);

        if (NORFLASH_API_OK == ret)
        {
            // LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s: norflash_api_erase ok!",__func__);
            break;
        }
        else if(NORFLASH_API_BUFFER_FULL == ret)
        {
            //LOG_DUMP_TRACE(LOG_DUMP_PREFIX"Flash async cache overflow! To flush.");
            if(log_dump_is_immediately && is_suspend)
            {
                LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s: log_dump_is_immediately!",__func__);
                break;
            }
            _flash_api_flush();
        }
        else
        {
            ASSERT(0, "_flash_api_erase: norflash_api_erase failed. ret = %d,addr = 0x%x.", ret,addr);
        }
    } while (1);

    return ret;
}

static enum NORFLASH_API_RET_T _flash_api_write(uint32_t addr,
                                                     uint8_t* ptr,
                                                     uint32_t len,
                                                     bool is_suspend)
{
    uint32_t lock;
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;

    do
    {
        if(log_dump_is_immediately && is_suspend)
        {
            LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s: log_dump_is_immediately!",__func__);
            break;
        }
        lock = int_lock_global();
        hal_trace_pause();
        ret = norflash_api_write(NORFLASH_API_MODULE_ID_TRACE_DUMP,
                                addr,
                                ptr,
                                len,
                                is_suspend);

        hal_trace_continue();

        int_unlock_global(lock);

        if (NORFLASH_API_OK == ret)
        {
            // LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s: norflash_api_write ok!",__func__);
            break;
        }
        else if (NORFLASH_API_BUFFER_FULL == ret)
        {
            //LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:Flash async cache overflow! To flush.",__func__);
            if(log_dump_is_immediately && is_suspend)
            {
                LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s: log_dump_is_immediately!",__func__);
                break;
            }
            _flash_api_flush();
        }
        else
        {
            ASSERT(0,"_flash_api_write: norflash_api_write failed. ret = %d",ret);
        }
    }while (1);

    return ret;
}

static int32_t _get_seqnum(uint32_t * dump_seqnum, uint32_t *sector_seqnum)
{
    uint32_t i;
    uint32_t count;
    static enum NORFLASH_API_RET_T result;
    LOG_DUMP_HEADER log_dump_header;
    uint32_t max_dump_seqnum = 0;
    uint32_t max_sector_seqnum = 0;
    bool is_existed = false;

    count = (log_dump_flash_end_addr-log_dump_flash_start_addr)/LOG_DUMP_SECTOR_SIZE;
    for(i = 0; i < count; i++)
    {
        result = _flash_api_read(
                    log_dump_flash_start_addr + i*LOG_DUMP_SECTOR_SIZE,
                    (uint8_t*)&log_dump_header,
                    sizeof(LOG_DUMP_HEADER));
        if(result == NORFLASH_API_OK)
        {
            if(log_dump_header.magic == DUMP_LOG_MAGIC
               && log_dump_header.version == DUMP_LOG_VERSION
              )
            {
                is_existed = true;
                if(log_dump_header.seqnum > max_dump_seqnum)
                {
                    max_dump_seqnum = log_dump_header.seqnum;
                    max_sector_seqnum = i;
                }
            }
        }
        else
        {
            ASSERT(0,"_get_cur_sector_seqnum: _flash_api_read failed!result = %d.",result);
        }
    }
    if(is_existed)
    {
        *dump_seqnum = max_dump_seqnum + 1;
        *sector_seqnum = max_sector_seqnum + 1 >= count ? 0: max_sector_seqnum + 1;
    }
    else
    {
        *dump_seqnum = 0;
        *sector_seqnum = 0;
    }
    return 0;
}

static int _log_dump_flush(uint32_t buff_state,bool is_suspend)
{
    enum NORFLASH_API_RET_T result;
    static uint32_t total_len = 0;
    DATA_BUFFER *data_buff;

    if(log_dump_flash_state == LOG_DUMP_FLASH_STATE_IDLE
       && log_dump_flash_offs % LOG_DUMP_SECTOR_SIZE == 0)
    {
        // LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:%d,state = idle,addr = 0x%x.",
        //        __func__,__LINE__,log_dump_flash_start_addr + log_dump_flash_offs);
        result = _flash_api_erase(log_dump_flash_start_addr + log_dump_flash_offs,is_suspend);
        if(result == NORFLASH_API_OK)
        {
            LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:%d,erase ok,addr = 0x%x,",
                __func__,__LINE__,log_dump_flash_start_addr + log_dump_flash_offs);
            log_dump_flash_state = LOG_DUMP_FLASH_STATE_ERASED;
        }
        else
        {
            LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s: _flash_api_erase failed!addr = 0x%x,ret = %d.",
                __func__,log_dump_flash_start_addr + log_dump_flash_offs, result);
        }
    }
    else if(log_dump_flash_state == LOG_DUMP_FLASH_STATE_ERASED)
    {
        // LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:%d,state = ERASED,f_seqnum = %d.",
        //        __func__,__LINE__,log_dump_f_seqnum);
        data_buff = &data_buffer_list[log_dump_f_seqnum];
        if((data_buff->state & buff_state) != 0
            && data_buff->offset > 0)
        {
            log_dump_flash_state = LOG_DUMP_FLASH_STATE_WRITTING;
            result = _flash_api_write(
                    log_dump_flash_start_addr + log_dump_flash_offs,
                    data_buff->buffer,
                    data_buff->offset,
                    is_suspend);
            if(result == NORFLASH_API_OK)
            {
                LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:%d,write ok,addr = 0x%x,f_seqnum = 0x%x,total_len = 0x%x.",
                        __func__,__LINE__,
                        log_dump_flash_start_addr + log_dump_flash_offs,
                        log_dump_f_seqnum,
                        total_len);

                data_buff->state = DATA_BUFFER_STATE_FREE;
                log_dump_f_seqnum = log_dump_f_seqnum + 1 == LOG_DUMP_SECTOR_BUFFER_COUNT ?
                               0 : log_dump_f_seqnum + 1;
                total_len += data_buff->offset;

                log_dump_flash_offs = log_dump_flash_offs + data_buff->offset >= log_dump_flash_len ?
                               0 : log_dump_flash_offs + data_buff->offset;
                log_dump_flash_state = LOG_DUMP_FLASH_STATE_IDLE;
            }
            else
            {
                LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s: _flash_api_write failed!ret = %d.",__func__,result);
                return 3;
            }
        }
    }
    else
    {
         LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:%d state = %d.",
                __func__,__LINE__,log_dump_flash_state);
    }
    return 0;
}

void _log_dump_flush_remain(void)
{
    uint32_t i;
    uint32_t unfree_count = 0;

    do{
        for(i = 0; i < LOG_DUMP_SECTOR_BUFFER_COUNT; i++)
        {
            if(data_buffer_list[i].state != DATA_BUFFER_STATE_FREE)
            {
                unfree_count ++;
            }
        }
        if(unfree_count == 0)
        {
            break;
        }
        _log_dump_flush(DATA_BUFFER_STATE_WRITTING | DATA_BUFFER_STATE_WRITTEN,false);
    }while(1);

    _flash_api_flush();
}

int log_dump_flush(void)
{
    if(!log_dump_is_init)
    {
        return 0;
    }
    return _log_dump_flush(DATA_BUFFER_STATE_WRITTEN, true);
}

void log_dump_notify_handler(enum HAL_TRACE_STATE_T state)
{
    uint32_t lock;

    if(!log_dump_is_init)
    {
        return;
    }
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s: state = %d.",__func__,state);
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s: start_addr = 0x%x,end_addr = 0x%x.",
                __func__,log_dump_flash_start_addr,log_dump_flash_end_addr);
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s: dump_seqnum = 0x%x,flash_offset = 0x%x.",
                __func__,log_dump_cur_dump_seqnum,log_dump_flash_offs);

    if(state == HAL_TRACE_STATE_CRASH_ASSERT_START
       || state == HAL_TRACE_STATE_CRASH_FAULT_START)
    {
        log_dump_is_immediately = true;
    }
    else
    {
        lock = int_lock_global();
        //hal_trace_pause();
        _log_dump_flush_remain();
        //hal_trace_continue();
        int_unlock_global(lock);
        log_dump_is_immediately = false;
    }
}

void log_dump_output_handler(const unsigned char *buf, unsigned int buf_len)
{
    //uint32_t len,len1;
    uint32_t uint_len;
    uint32_t written_len;
    uint32_t remain_len;
    LOG_DUMP_HEADER log_header;
    DATA_BUFFER *data_buff;

    if(!log_dump_is_init)
    {
        return;
    }

    if(strstr((char*)buf,LOG_DUMP_PREFIX) != NULL)
    {
        return;
    }
/*
#if defined(DUMP_CRASH_ENABLE) && !defined(DUMP_LOG_ENABLE)
    if(!log_dump_is_immediately)
    {
        return;
    }
#endif
*/
    data_buff = &data_buffer_list[log_dump_w_seqnum];
    remain_len = buf_len;
    written_len = 0;
    do{
        if(data_buff->state ==  DATA_BUFFER_STATE_FREE)
        {
            // LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:%d data_buff->state is free.w_seqnum = %d.",
            //    __func__,__LINE__,log_dump_w_seqnum);
            data_buff->state = DATA_BUFFER_STATE_WRITTING;
            data_buff->offset = 0;
            memset(data_buff->buffer,0,LOG_DUMP_SECTOR_SIZE);

        }

        if(data_buff->offset ==  0)
        {
            // LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:%d offset = 0.w_seqnum = %d,dump_seqnum = %d.",
            //    __func__,__LINE__,log_dump_w_seqnum,log_dump_cur_dump_seqnum);
            memset((uint8_t*)&log_header,0,sizeof(log_header));
            log_header.magic = DUMP_LOG_MAGIC;
            log_header.version = DUMP_LOG_VERSION;
            log_header.seqnum = log_dump_cur_dump_seqnum;
            log_header.reseved[0] = '\0';
            log_header.reseved[1] = '\0';
            log_header.reseved[2] = '\0';
            log_header.reseved[3] = '\n';
            memcpy(data_buff->buffer + data_buff->offset,
                    (uint8_t*)&log_header,
                    sizeof(log_header));

            data_buff->offset += sizeof(log_header);
            log_dump_cur_dump_seqnum ++;
        }

        if(data_buff->offset + remain_len > LOG_DUMP_SECTOR_SIZE)
        {
            uint_len = (LOG_DUMP_SECTOR_SIZE - data_buff->offset);
        }
        else
        {
            uint_len = remain_len;
        }
        if(uint_len > 0)
        {
            memcpy(data_buff->buffer + data_buff->offset,
                    buf + written_len,
                    uint_len);
            data_buff->offset += uint_len;
            written_len += uint_len;
        }
        if(data_buff->offset == LOG_DUMP_SECTOR_SIZE)
        {
            data_buff->state = DATA_BUFFER_STATE_WRITTEN;
        }

        remain_len -= uint_len;

        if(data_buff->offset == LOG_DUMP_SECTOR_SIZE)
        {
            log_dump_w_seqnum = log_dump_w_seqnum + 1 == LOG_DUMP_SECTOR_BUFFER_COUNT ?
                               0 : log_dump_w_seqnum + 1;
            LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:%d w_seqnum = %d,dump_seqnum = %d.",
                __func__,__LINE__,log_dump_w_seqnum,log_dump_cur_dump_seqnum);
            data_buff = &data_buffer_list[log_dump_w_seqnum];

            ASSERT(data_buff->state == DATA_BUFFER_STATE_FREE ||
                   data_buff->state == DATA_BUFFER_STATE_WRITTEN,
                   "log_dump_output_handler: data_buff state error! state = %d.",
                   data_buff->state);

            if(data_buff->state != DATA_BUFFER_STATE_FREE)
            {
                LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:%d: buffer state = %d,Lost %d.",
                __func__,__LINE__,data_buff->state,remain_len);
                remain_len = 0;
            }
        }
    }while(remain_len > 0);

}

void log_dump_callback(void* param)
{
    NORFLASH_API_OPERA_RESULT *opera_result;

    opera_result = (NORFLASH_API_OPERA_RESULT*)param;

    opera_result = opera_result;

    LOG_DUMP_TRACE_FORCE(LOG_DUMP_PREFIX"%s:type = %d, addr = 0x%x,len = 0x%x,remain = %d,result = %d.",
                __func__,
                opera_result->type,
                opera_result->addr,
                opera_result->len,
                opera_result->remain_num,
                opera_result->result);
    {
        static uint32_t is_fst = 1;
        if(is_fst)
        {
            LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s:log_dump_start = 0x%x, log_dump_end = 0x%x.",
                    __func__,
                    log_dump_flash_start_addr,
                    log_dump_flash_end_addr);
            is_fst = 0;
        }
    }
}

void log_dump_init(void)
{
    uint32_t block_size = 0;
    uint32_t sector_size = 0;
    uint32_t page_size = 0;
    uint32_t buffer_len = 0;
    uint32_t dump_seqnum = 0;
    uint32_t sector_seqnum = 0;
    enum NORFLASH_API_RET_T result;
    uint32_t i;

    //_log_dum_mutex = osMutexCreate((osMutex(log_dump_mutex)));
    hal_norflash_get_size(HAL_NORFLASH_ID_0,NULL, &block_size, &sector_size, &page_size);
    buffer_len = LOG_DUMP_NORFALSH_BUFFER_LEN;
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s: log_dump_start = 0x%x, log_dump_len = 0x%x, buff_len = 0x%x.",
                    __func__,
                    log_dump_flash_start_addr,
                    log_dump_flash_len,
                    buffer_len);

    result = norflash_api_register(
                    NORFLASH_API_MODULE_ID_TRACE_DUMP,
                    HAL_NORFLASH_ID_0,
                    log_dump_flash_start_addr,
                    log_dump_flash_end_addr - log_dump_flash_start_addr,
                    block_size,
                    sector_size,
                    page_size,
                    buffer_len,
                    log_dump_callback
                    );

    if(result == NORFLASH_API_OK)
    {
        LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s: norflash_api_register ok.",result);
    }
    else
    {
        LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s: norflash_api_register failed,result = %d.",result);
        return;
    }
    hal_trace_app_register(log_dump_notify_handler, log_dump_output_handler);
    hal_sleep_set_sleep_hook(HAL_SLEEP_HOOK_DUMP_LOG, log_dump_flush);
    _get_seqnum(&dump_seqnum,&sector_seqnum);
    log_dump_cur_dump_seqnum = dump_seqnum;
    log_dump_flash_len = log_dump_flash_end_addr - log_dump_flash_start_addr;
    log_dump_flash_offs = sector_seqnum*LOG_DUMP_SECTOR_SIZE;
    memset((uint8_t*)&data_buffer_list,0,sizeof(data_buffer_list));
    for(i = 0;i < LOG_DUMP_SECTOR_BUFFER_COUNT; i++)
    {
        data_buffer_list[i].state = DATA_BUFFER_STATE_FREE;
        data_buffer_list[i].offset = 0;
    }
    log_dump_w_seqnum = 0;
    log_dump_f_seqnum = 0;
    log_dump_flash_state = LOG_DUMP_FLASH_STATE_IDLE;
    log_dump_is_immediately = false;
    log_dump_is_init = true;
}

void log_dump_clear(void)
{
    uint32_t i;
    uint32_t addr;
    uint32_t lock = int_lock_global();

    for(i = 0; i < log_dump_flash_len/LOG_DUMP_SECTOR_SIZE; i++)
    {
        addr = log_dump_flash_start_addr + i*LOG_DUMP_SECTOR_SIZE;
        _flash_api_erase(addr,false);
    }

    int_unlock_global(lock);
}

#if 0
uint8_t test_buff_r[LOG_DUMP_SECTOR_SIZE];

uint32_t test_log_dump_from_flash(uint32_t addr,uint32_t size)
{
	//uint32_t start_addr;
	uint32_t i;
	//uint8_t value = 0;
	int32_t ret = 0;
	enum NORFLASH_API_RET_T result = HAL_NORFLASH_OK;

	LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s enter!!!", __func__);

	for(i = 0; i < size/LOG_DUMP_SECTOR_SIZE; i++)
	{
		result = _flash_api_read((uint32_t)(addr)+ (i*LOG_DUMP_SECTOR_SIZE),
			test_buff_r,LOG_DUMP_SECTOR_SIZE);
		if(result != NORFLASH_API_OK)
		{
			ret = -1;
			//LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s ret=%d", __func__, ret);
			goto _func_end;
		}
		LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s", test_buff_r);
	}
    LOG_DUMP_TRACE(LOG_DUMP_PREFIX"%s end!!!", __func__);

_func_end:
	LOG_DUMP_TRACE(LOG_DUMP_PREFIX"_debug: flash checking end. ret = %d.",ret);
	return ret;
}
#endif
#endif

