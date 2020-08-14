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
#ifndef __HAL_TRACE_H__
#define __HAL_TRACE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"


// #define AUDIO_DEBUG
// #define AUDIO_DEBUG_V0_1_0
// #define HAL_TRACE_RX_ENABLE

// #define INTERSYS_RAW_DATA_ONLY

/*
 * Total number of core registers stored
 */
#define CRASH_DUMP_REGISTERS_NUM   17
#define CRASH_DUMP_REGISTERS_NUM_BYTES   ((CRASH_DUMP_REGISTERS_NUM)*4)

/*
 * Number bytes to store from stack
 *   - this is total, not per PSP/MSP
 */
#define CRASH_DUMP_STACK_NUM_BYTES 384

enum HAL_TRACE_MODUAL {
    HAL_TRACE_LEVEL_0  = 1<<0,
    HAL_TRACE_LEVEL_1  = 1<<1,
    HAL_TRACE_LEVEL_2  = 1<<2,
    HAL_TRACE_LEVEL_3  = 1<<3,
    HAL_TRACE_LEVEL_4  = 1<<4,
    HAL_TRACE_LEVEL_5  = 1<<5,
    HAL_TRACE_LEVEL_6  = 1<<6,
    HAL_TRACE_LEVEL_7  = 1<<7,
    HAL_TRACE_LEVEL_8  = 1<<8,
    HAL_TRACE_LEVEL_9  = 1<<9,
    HAL_TRACE_LEVEL_10 = 1<<10,
    HAL_TRACE_LEVEL_11 = 1<<11,
    HAL_TRACE_LEVEL_12 = 1<<12,
    HAL_TRACE_LEVEL_13 = 1<<13,
    HAL_TRACE_LEVEL_14 = 1<<14,
    HAL_TRACE_LEVEL_15 = 1<<15,
    HAL_TRACE_LEVEL_16 = 1<<16,
    HAL_TRACE_LEVEL_17 = 1<<17,
    HAL_TRACE_LEVEL_18 = 1<<18,
    HAL_TRACE_LEVEL_19 = 1<<19,
    HAL_TRACE_LEVEL_20 = 1<<20,
    HAL_TRACE_LEVEL_21 = 1<<21,
    HAL_TRACE_LEVEL_22 = 1<<22,
    HAL_TRACE_LEVEL_23 = 1<<23,
    HAL_TRACE_LEVEL_24 = 1<<24,
    HAL_TRACE_LEVEL_25 = 1<<25,
    HAL_TRACE_LEVEL_26 = 1<<26,
    HAL_TRACE_LEVEL_27 = 1<<27,
    HAL_TRACE_LEVEL_28 = 1<<28,
    HAL_TRACE_LEVEL_29 = 1<<29,
    HAL_TRACE_LEVEL_30 = 1<<30,
    HAL_TRACE_LEVEL_31 = 1<<31,
    HAL_TRACE_LEVEL_ALL = 0XFFFFFFFF,
};

typedef enum {
    LOG_LV_NONE,
    LOG_LV_FATAL,
    LOG_LV_ERROR,
    LOG_LV_WARN,
    LOG_LV_INFO,
    LOG_LV_DEBUG,
}LOG_LEVEL_T;

typedef enum {
    LOG_MOD_NONE,
    LOG_MOD_OS,
    LOG_MOD_BT_STACK,
    LOG_MOD_SYS,
    LOG_MOD_MEDIA,
    LOG_MOD_APP,
}LOG_MODULE_T;

#if defined(_AUTO_TEST_)
#define HAL_TRACE_RX_ENABLE
extern int auto_test_send(char *resp);
#define AUTO_TEST_SEND(str)         auto_test_send((char*)str)
#endif

#if defined(TRACE_STR_SECTION) && !defined(ROM_BUILD) && !defined(PROGRAMMER) && !defined(FPGA)
#define TRC_STR_LOC                 __attribute__((section(".trc_str")))
#define TRC_STR(s)                  (({ static const char TRC_STR_LOC __trc[] = (s); __trc; }))
#else
#define TRC_STR_LOC
#define TRC_STR(s)                  (s)
#endif

#define TRACE_DUMMY(str, ...)       hal_trace_dummy(str, ##__VA_ARGS__)

#if (defined(DEBUG) || defined(REL_TRACE_ENABLE)) && (!defined(AUDIO_DEBUG) || defined(AUDIO_DEBUG_V0_1_0))
#define REL_TRACE(str, ...)         hal_trace_printf(TRC_STR(str), ##__VA_ARGS__)
#define REL_TRACE_NOCRLF(str, ...)  hal_trace_printf_without_crlf(str, ##__VA_ARGS__)
#define REL_TRACE_IMM(str, ...)     hal_trace_printf_imm(str, ##__VA_ARGS__)
#define REL_TRACE_OUTPUT(str, len)  hal_trace_output(str, len)
#define REL_TRACE_FLUSH()           hal_trace_flush_buffer()
#define REL_FUNC_ENTRY_TRACE()      hal_trace_printf(__FUNCTION__)
#define REL_DUMP8(str, buf, cnt)    hal_trace_dump(str, sizeof(uint8_t), cnt, buf)
#define REL_DUMP16(str, buf, cnt)   hal_trace_dump(str, sizeof(uint16_t), cnt, buf)
#define REL_DUMP32(str, buf, cnt)   hal_trace_dump(str, sizeof(uint32_t), cnt, buf)
#else
#define REL_TRACE(str, ...)         hal_trace_dummy(str, ##__VA_ARGS__)
#define REL_TRACE_NOCRLF(str, ...)  hal_trace_dummy(str, ##__VA_ARGS__)
#define REL_TRACE_IMM(str, ...)     hal_trace_dummy(str, ##__VA_ARGS__)
#define REL_TRACE_OUTPUT(str, len)  hal_trace_dummy((const char *)str, len)
#define REL_TRACE_FLUSH()           hal_trace_dummy(NULL)
#define REL_FUNC_ENTRY_TRACE()      hal_trace_dummy(NULL)
#define REL_DUMP8(str, buf, cnt)    hal_trace_dummy(str, buf, cnt)
#define REL_DUMP16(str, buf, cnt)   hal_trace_dummy(str, buf, cnt)
#define REL_DUMP32(str, buf, cnt)   hal_trace_dummy(str, buf, cnt)
#endif

#if defined(INTERSYS_RAW_DATA_ONLY)
#define TRACE_RAW                   REL_TRACE
#define DUMP8_RAW                   REL_DUMP8
#endif

#if (!defined(DEBUG) && defined(REL_TRACE_ENABLE)) && (!defined(AUDIO_DEBUG) || defined(AUDIO_DEBUG_V0_1_0)) || defined(INTERSYS_RAW_DATA_ONLY)
// To avoid warnings on unused variables
#define TRACE(str, ...)             hal_trace_dummy(str, ##__VA_ARGS__)
#define TRACE_NOCRLF(str, ...)      hal_trace_dummy(str, ##__VA_ARGS__)
#define TRACE_IMM(str, ...)         hal_trace_dummy(str, ##__VA_ARGS__)
#define TRACE_OUTPUT(str, len)      hal_trace_dummy((const char *)str, len)
#define TRACE_FLUSH()               hal_trace_dummy(NULL)
#define FUNC_ENTRY_TRACE()          hal_trace_dummy(NULL)
#define DUMP8(str, buf, cnt)        hal_trace_dummy(str, buf, cnt)
#define DUMP16(str, buf, cnt)       hal_trace_dummy(str, buf, cnt)
#define DUMP32(str, buf, cnt)       hal_trace_dummy(str, buf, cnt)
#else
#define TRACE                       REL_TRACE
#define TRACE_NOCRLF                REL_TRACE_NOCRLF
#define TRACE_IMM                   REL_TRACE_IMM
#define TRACE_OUTPUT                REL_TRACE_OUTPUT
#define TRACE_FLUSH                 REL_TRACE_FLUSH
#define FUNC_ENTRY_TRACE            REL_FUNC_ENTRY_TRACE
#define DUMP8                       REL_DUMP8
#define DUMP16                      REL_DUMP16
#define DUMP32                      REL_DUMP32
#endif

#if (defined(DEBUG) || defined(REL_TRACE_ENABLE)) && defined(ASSERT_SHOW_FILE_FUNC)
#define ASSERT(cond, str, ...)      { if (!(cond)) { hal_trace_assert_dump(__FILE__, __FUNCTION__, __LINE__, str, ##__VA_ARGS__); } }
#define ASSERT_DUMP_ARGS            const char *file, const char *func, unsigned int line, const char *fmt, ...
#elif (defined(DEBUG) || defined(REL_TRACE_ENABLE)) && defined(ASSERT_SHOW_FILE)
#define ASSERT(cond, str, ...)      { if (!(cond)) { hal_trace_assert_dump(__FILE__, __LINE__, str, ##__VA_ARGS__); } }
#define ASSERT_DUMP_ARGS            const char *file, const char *func, unsigned int line, const char *fmt, ...
#elif (defined(DEBUG) || defined(REL_TRACE_ENABLE)) && defined(ASSERT_SHOW_FUNC)
#define ASSERT(cond, str, ...)      { if (!(cond)) { hal_trace_assert_dump(__FUNCTION__, __LINE__, str, ##__VA_ARGS__); } }
#define ASSERT_DUMP_ARGS            const char *scope, unsigned int line, const char *fmt, ...
#elif (defined(DEBUG) || defined(REL_TRACE_ENABLE))
#define ASSERT(cond, str, ...)      { if (!(cond)) { hal_trace_assert_dump(str, ##__VA_ARGS__); } }
#define ASSERT_DUMP_ARGS            const char *fmt, ...
#else
#define ASSERT(cond, str, ...)      { if (!(cond)) { hal_trace_dummy(str, ##__VA_ARGS__); hal_trace_assert_dump(NULL); } }
#define ASSERT_DUMP_ARGS            const char *fmt
#endif

#define ASSERT_NODUMP(cond)         { if (!(cond)) { SAFE_PROGRAM_STOP(); } }

#ifdef CHIP_BEST1000
// Avoid CPU instruction fetch blocking the system bus on BEST1000
#define SAFE_PROGRAM_STOP()         do { asm volatile("nop \n nop \n nop \n nop"); } while (1)
#else
#define SAFE_PROGRAM_STOP()         do { } while (1)
#endif

#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))
#define TRACE_FUNC_DECLARE(d, r)    d
#else
#ifndef TRACE_FUNC_SPEC
#define TRACE_FUNC_SPEC             static inline
#endif
#define TRACE_FUNC_DECLARE(d, r)    TRACE_FUNC_SPEC d { r; }
#endif

enum HAL_TRACE_TRANSPORT_T {
#ifdef CHIP_HAS_USB
    HAL_TRACE_TRANSPORT_USB,
#endif
    HAL_TRACE_TRANSPORT_UART0,
#if (CHIP_HAS_UART > 1)
    HAL_TRACE_TRANSPORT_UART1,
#endif

    HAL_TRACE_TRANSPORT_QTY
};

enum HAL_TRACE_STATE_T {
    HAL_TRACE_STATE_CRASH_ASSERT_START,
    HAL_TRACE_STATE_CRASH_FAULT_START,
    HAL_TRACE_STATE_CRASH_END,
};

typedef void (*HAL_TRACE_CRASH_DUMP_CB_T)(void);

typedef void (*HAL_TRACE_APP_NOTIFY_T)(enum HAL_TRACE_STATE_T state);

typedef void (*HAL_TRACE_APP_OUTPUT_T)(const unsigned char *buf, unsigned int buf_len);

int hal_trace_open(enum HAL_TRACE_TRANSPORT_T transport);

TRACE_FUNC_DECLARE(int hal_trace_switch(enum HAL_TRACE_TRANSPORT_T transport), return 0);

TRACE_FUNC_DECLARE(int hal_trace_close(void), return 0);

TRACE_FUNC_DECLARE(void hal_trace_get_history_buffer(const unsigned char **buf1, unsigned int *len1, \
    const unsigned char **buf2, unsigned int *len2), \
    { if (buf1) { *buf1 = NULL; } if (len1) { *len1 = 0; } if (buf2) { *buf2 = NULL; } if (len2) { *len2 = 0; } });

TRACE_FUNC_DECLARE(int hal_trace_output(const unsigned char *buf, unsigned int buf_len), return 0);

TRACE_FUNC_DECLARE(int hal_trace_printf_with_lvl(unsigned int lvl, const char *fmt, ...), return 0);

TRACE_FUNC_DECLARE(int hal_trace_printf(const char *fmt, ...), return 0);

TRACE_FUNC_DECLARE(void hal_trace_printf_imm(const char *fmt, ...), return);

TRACE_FUNC_DECLARE(int hal_trace_printf_without_crlf(const char *fmt, ...), return 0);

TRACE_FUNC_DECLARE(int hal_trace_printf_with_tag(const char *tag, const char *fmt, ...), return 0);

TRACE_FUNC_DECLARE(int hal_trace_printf_without_crlf_fix_arg(const char *fmt), return 0);

TRACE_FUNC_DECLARE(int hal_trace_dump(const char *fmt, unsigned int size,  unsigned int count, const void *buffer), return 0);

TRACE_FUNC_DECLARE(int hal_trace_busy(void), return 0);

TRACE_FUNC_DECLARE(int hal_trace_pause(void), return 0);

TRACE_FUNC_DECLARE(int hal_trace_continue(void), return 0);

TRACE_FUNC_DECLARE(void hal_trace_flush_buffer(void), return);

TRACE_FUNC_DECLARE(void hal_trace_idle_send(void), return);

TRACE_FUNC_DECLARE(int hal_trace_crash_dump_register(HAL_TRACE_CRASH_DUMP_CB_T cb), return 0);

TRACE_FUNC_DECLARE(void hal_trace_app_register(HAL_TRACE_APP_NOTIFY_T notify_cb, HAL_TRACE_APP_OUTPUT_T output_cb), return);

TRACE_FUNC_DECLARE(void hal_trace_app_crash_register(HAL_TRACE_APP_NOTIFY_T notify_cb, HAL_TRACE_APP_OUTPUT_T output_cb,HAL_TRACE_APP_OUTPUT_T crash_fault_cb), return);

TRACE_FUNC_DECLARE(void hal_trace_print_backtrace(uint32_t addr, uint32_t search_cnt, uint32_t print_cnt), return);

static inline void hal_trace_dummy(const char *fmt, ...) { }

void NORETURN hal_trace_assert_dump(ASSERT_DUMP_ARGS);

int hal_trace_address_writable(uint32_t addr);

int hal_trace_address_executable(uint32_t addr);

int hal_trace_address_readable(uint32_t addr);


//==============================================================================
// AUDIO_DEBUG
//==============================================================================

#ifdef AUDIO_DEBUG
#define AUDIO_DEBUG_TRACE(str, ...)         hal_trace_printf(str, ##__VA_ARGS__)
#define AUDIO_DEBUG_DUMP(buf, cnt)          hal_trace_output(buf, cnt)
#endif


//==============================================================================
// TRACE RX
//==============================================================================

#ifdef HAL_TRACE_RX_ENABLE

#include "stdio.h"

#define hal_trace_rx_parser(buf, str, ...)  sscanf(buf, str, ##__VA_ARGS__)

typedef unsigned int (*HAL_TRACE_RX_CALLBACK_T)(unsigned char *buf, unsigned int  len);

int hal_trace_rx_register(const char *name, HAL_TRACE_RX_CALLBACK_T callback);
int hal_trace_rx_deregister(const char *name);

#endif

#ifdef __cplusplus
}
#endif

#endif
