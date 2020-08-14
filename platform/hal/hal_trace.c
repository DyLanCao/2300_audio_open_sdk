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
#if !(defined(DEBUG) || defined(REL_TRACE_ENABLE))
// Implement a local copy of dummy trace functions for library linking (which might be built with DEBUG enabled)
#define TRACE_FUNC_SPEC
#endif
#include "hal_trace.h"
#include "cmsis_nvic.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_cmu.h"
#include "hal_chipid.h"
#include "hal_dma.h"
#include "hal_iomux.h"
#include "hal_memsc.h"
#include "hal_timer.h"
#include "hal_uart.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "hal_bootmode.h"
#include "crash_dump_section.h"

extern const char sys_build_info[];
#define CRASH_REBOOT

#ifdef FAULT_DUMP
static void hal_trace_fault_handler(void);
#endif

#if !(defined(NO_TRACE_TIME_STAMP) || defined(ROM_BUILD) || defined(PROGRAMMER) || defined(AUDIO_DEBUG_V0_1_0))
#define TRACE_TIME_STAMP
#endif
#if (defined(DUMP_LOG_ENABLE) || defined(DUMP_CRASH_ENABLE))
#define TRACE_TO_APP
#endif

#define TRACE_IDLE_OUTPUT               0

#ifndef TRACE_BUF_SIZE
#ifdef AUDIO_DEBUG
#define TRACE_BUF_SIZE                  (6 * 1024)
#else
#define TRACE_BUF_SIZE                  (2 * 1024)
#endif
#endif

#ifndef TRACE_STACK_DUMP_WORD
#define TRACE_STACK_DUMP_WORD           32
#endif
#ifndef TRACE_BACKTRACE_NUM
#define TRACE_BACKTRACE_NUM             20
#endif
#ifndef TRACE_BACKTRACE_SEARCH_WORD
#define TRACE_BACKTRACE_SEARCH_WORD     1024
#endif


#define TRACE_CRLF

#ifdef TRACE_CRLF
#define NEW_LINE_STR                    "\r\n"
#else
#define NEW_LINE_STR                    "\n"
#endif

#define HAL_TRACE_ASSERT_ID             0xBE57AAAA
#define HAL_TRACE_EXCEPTION_ID          0xBE57EEEE

#define HAL_MEMSC_ID_TRACE              HAL_MEMSC_ID_0

struct HAL_TRACE_ASSERT_INFO_T {
    uint32_t ID;
    uint32_t PC;
    const char *FILE;
    const char *FUNC;
    uint32_t LINE;
    const char *FMT;
    uint32_t R[4];
    uint32_t SP;
    uint32_t MSP;
    uint32_t PSP;
    uint32_t CONTROL;
};

struct HAL_TRACE_EXCEPTION_INFO_T {
    uint32_t ID;
    const uint32_t* REGS;
    uint32_t MSP;
    uint32_t PSP;
    uint8_t PRIMASK;
    uint8_t FAULTMASK;
    uint8_t BASEPRI;
    uint8_t CONTROL;
    uint32_t ICSR;
    uint32_t AIRCR;
    uint32_t SCR;
    uint32_t CCR;
    uint32_t SHCSR;
    uint32_t CFSR;
    uint32_t HFSR;
    uint32_t AFSR;
    uint32_t MMFAR;
    uint32_t BFAR;
};

#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))

struct HAL_TRACE_BUF_T {
    unsigned char buf[TRACE_BUF_SIZE];
    unsigned short wptr;
    unsigned short rptr;
#if (TRACE_IDLE_OUTPUT == 0)
    unsigned short sends[2];
#endif
    unsigned short discards;
    bool sending;
    bool in_trace;
    bool wrapped;
};

STATIC_ASSERT(TRACE_BUF_SIZE < (1 << (8 * sizeof(((struct HAL_TRACE_BUF_T *)0)->wptr))), "TRACE_BUF_SIZE is too large to fit in wptr/rptr variable");

static const struct HAL_UART_CFG_T uart_cfg = {
    .parity = HAL_UART_PARITY_NONE,
    .stop = HAL_UART_STOP_BITS_1,
    .data = HAL_UART_DATA_BITS_8,
    .flow = HAL_UART_FLOW_CONTROL_NONE,//HAL_UART_FLOW_CONTROL_RTSCTS,
    .tx_level = HAL_UART_FIFO_LEVEL_1_2,
    .rx_level = HAL_UART_FIFO_LEVEL_1_2,
#ifdef AUDIO_DEBUG
    .baud = 921600,
    // .baud = 2600000,
#else
    .baud = 921600,
#endif
#ifdef HAL_TRACE_RX_ENABLE
    .dma_rx = true,
#else
    .dma_rx = false,
#endif
#if (TRACE_IDLE_OUTPUT == 0)
    .dma_tx = true,
#else
    .dma_tx = false,
#endif
    .dma_rx_stop_on_err = false,
};

#if (TRACE_IDLE_OUTPUT == 0)
static struct HAL_DMA_CH_CFG_T dma_cfg;
static struct HAL_DMA_DESC_T dma_desc[2];
#endif

static enum HAL_TRACE_TRANSPORT_T trace_transport = HAL_TRACE_TRANSPORT_QTY;
static enum HAL_UART_ID_T trace_uart;

static struct HAL_TRACE_BUF_T trace;

static const char POSSIBLY_UNUSED newline[] = NEW_LINE_STR;

static const char discards_prefix[] = NEW_LINE_STR "LOST ";
static const uint32_t max_discards = 99999;
// 5 digits + "\r\n" = 7 chars
static char discards_buf[sizeof(discards_prefix) - 1 + 7];
static const unsigned char discards_digit_start = sizeof(discards_prefix) - 1;

static char crash_buf[100];

#if !(defined(ROM_BUILD) || defined(PROGRAMMER))
static HAL_TRACE_CRASH_DUMP_CB_T crash_dump_cb_list[3];
#ifdef TRACE_TO_APP
static HAL_TRACE_APP_NOTIFY_T app_notify_cb = NULL;
static HAL_TRACE_APP_OUTPUT_T app_output_cb = NULL;
static HAL_TRACE_APP_OUTPUT_T app_crash_fault_cb = NULL;
static bool app_output_enabled =
#if defined(DUMP_LOG_ENABLE)
    true;
#else
    false;
#endif
#endif
#endif

#ifdef TRACE_TIME_STAMP
static bool skip_time_stamp;
#endif

#ifdef HAL_TRACE_RX_ENABLE
static int hal_trace_rx_open();
#endif

#ifdef AUDIO_DEBUG_V0_1_0
static const char trace_head_buf[] = "[trace]";
#endif

static uint32_t trace_level_mask = ~0;

#if (TRACE_IDLE_OUTPUT == 0)

static void hal_trace_uart_send(void)
{
    uint32_t sends[2];
    uint32_t int_state;

    int_state = int_lock();

    // There is a race condition if we do not check s/w flag, but only check the h/w status.
    // [e.g., hal_gpdma_chan_busy(dma_cfg.ch)]
    // When the DMA is done, but DMA IRQ handler is still pending due to interrupt lock
    // or higher priority IRQ, it will have a chance to send the same content twice.
    if (trace.wptr != trace.rptr && !trace.sending) {
        trace.sending = true;

        sends[1] = 0;
        if (trace.wptr > trace.rptr) {
            sends[0] = trace.wptr - trace.rptr;
        } else {
            sends[0] = TRACE_BUF_SIZE - trace.rptr;
            if (sends[0] <= HAL_DMA_MAX_DESC_XFER_SIZE) {
                sends[1] = trace.wptr;
            }
        }
        if (sends[0] > HAL_DMA_MAX_DESC_XFER_SIZE) {
            sends[1] = sends[0] - HAL_DMA_MAX_DESC_XFER_SIZE;
            sends[0] = HAL_DMA_MAX_DESC_XFER_SIZE;
        }
        if (sends[1] > HAL_DMA_MAX_DESC_XFER_SIZE) {
            sends[1] = HAL_DMA_MAX_DESC_XFER_SIZE;
        }

        dma_cfg.src = (uint32_t)&trace.buf[trace.rptr];
        if (sends[1] == 0) {
            dma_cfg.src_tsize = sends[0];
            hal_gpdma_init_desc(&dma_desc[0], &dma_cfg, NULL, 1);
        } else {
            dma_cfg.src_tsize = sends[0];
            hal_gpdma_init_desc(&dma_desc[0], &dma_cfg, &dma_desc[1], 0);

            if (trace.rptr + sends[0] < TRACE_BUF_SIZE) {
                dma_cfg.src = (uint32_t)&trace.buf[trace.rptr + sends[0]];
            } else {
                dma_cfg.src = (uint32_t)&trace.buf[0];
            }
            dma_cfg.src_tsize = sends[1];
            hal_gpdma_init_desc(&dma_desc[1], &dma_cfg, NULL, 1);
        }
        trace.sends[0] = sends[0];
        trace.sends[1] = sends[1];

        hal_gpdma_sg_start(&dma_desc[0], &dma_cfg);
    }

    int_unlock(int_state);
}

static void hal_trace_uart_xfer_done(uint8_t chan, uint32_t remain_tsize, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    if (error) {
        if (lli || trace.sends[1] == 0) {
            if (trace.sends[0] > remain_tsize) {
                trace.sends[0] -= remain_tsize;
            } else {
                trace.sends[0] = 0;
            }
            trace.sends[1] = 0;
        } else {
            if (trace.sends[1] > remain_tsize) {
                trace.sends[1] -= remain_tsize;
            } else {
                trace.sends[1] = 0;
            }
        }
    }

    trace.rptr += trace.sends[0] + trace.sends[1];
    if (trace.rptr >= TRACE_BUF_SIZE) {
        trace.rptr -= TRACE_BUF_SIZE;
    }
    trace.sends[0] = 0;
    trace.sends[1] = 0;
    trace.sending = false;

    if (trace.wptr != trace.rptr) {
        hal_trace_uart_send();
    }
}

static void hal_trace_send(void)
{
    if (trace_transport == HAL_TRACE_TRANSPORT_QTY) {
        return;
    }

#ifdef CHIP_HAS_CP
    if (get_cpu_id()) {
        return;
    }
#endif

    hal_trace_uart_send();
}

#else // TRACE_IDLE_OUTPUT

static void hal_trace_uart_idle_send(void)
{
    int i;
    uint32_t lock;
    unsigned short wptr, rptr;

    lock = int_lock();
    wptr = trace.wptr;
    rptr = trace.rptr;
    int_unlock(lock);

    if (wptr == rptr) {
        return;
    }

    if (wptr < rptr) {
        for (i = rptr; i < TRACE_BUF_SIZE; i++) {
            hal_uart_blocked_putc(trace_uart, trace.buf[i]);
        }
        rptr = 0;
    }

    for (i = rptr; i < wptr; i++) {
        hal_uart_blocked_putc(trace_uart, trace.buf[i]);
    }

    trace.rptr = wptr;
    if (trace.rptr >= TRACE_BUF_SIZE) {
        trace.rptr -= TRACE_BUF_SIZE;
    }
}

void hal_trace_idle_send(void)
{
    if (trace_transport == HAL_TRACE_TRANSPORT_USB ||
            trace_transport >= HAL_TRACE_TRANSPORT_QTY) {
        return;
    }

    hal_trace_uart_idle_send();
}

#endif // TRACE_IDLE_OUTPUT

int hal_trace_open(enum HAL_TRACE_TRANSPORT_T transport)
{
    int ret;

#if (CHIP_HAS_UART > 1)
#ifdef FORCE_TRACE_UART1
    transport = HAL_TRACE_TRANSPORT_UART1;
#endif
#endif

#ifdef CHIP_HAS_USB
    if (transport == HAL_TRACE_TRANSPORT_USB) {
        return 1;
    }
#endif
    if (transport >= HAL_TRACE_TRANSPORT_QTY) {
        return 1;
    }

    if (trace_transport != HAL_TRACE_TRANSPORT_QTY) {
        return hal_trace_switch(transport);
    }

    if (transport == HAL_TRACE_TRANSPORT_UART0) {
        trace_uart = HAL_UART_ID_0;
#if (CHIP_HAS_UART > 1)
    } else if (transport == HAL_TRACE_TRANSPORT_UART1) {
        trace_uart = HAL_UART_ID_1;
#endif
    }

    memcpy(discards_buf, discards_prefix, discards_digit_start);

    trace.wptr = 0;
    trace.rptr = 0;
    trace.discards = 0;
    trace.sending = false;
    trace.in_trace = false;
    trace.wrapped = false;

#if (TRACE_IDLE_OUTPUT == 0)
    trace.sends[0] = 0;
    trace.sends[1] = 0;

    if (transport == HAL_TRACE_TRANSPORT_UART0
#if (CHIP_HAS_UART > 1)
            || transport == HAL_TRACE_TRANSPORT_UART1
#endif
            ) {
        memset(&dma_cfg, 0, sizeof(dma_cfg));
        dma_cfg.dst = 0; // useless
        dma_cfg.dst_bsize = HAL_DMA_BSIZE_8;
#if (CHIP_HAS_UART > 1)
        dma_cfg.dst_periph = (trace_uart == HAL_UART_ID_0) ? HAL_GPDMA_UART0_TX : HAL_GPDMA_UART1_TX;
#else
        dma_cfg.dst_periph = HAL_GPDMA_UART0_TX;
#endif
        dma_cfg.dst_width = HAL_DMA_WIDTH_BYTE;
        dma_cfg.handler = hal_trace_uart_xfer_done;
        dma_cfg.src_bsize = HAL_DMA_BSIZE_32;
        dma_cfg.src_periph = 0; // useless
        dma_cfg.src_width = HAL_DMA_WIDTH_BYTE;
        dma_cfg.type = HAL_DMA_FLOW_M2P_DMA;
        dma_cfg.try_burst = 0;
        dma_cfg.ch = hal_gpdma_get_chan(dma_cfg.dst_periph, HAL_DMA_HIGH_PRIO);

        ASSERT(dma_cfg.ch != HAL_DMA_CHAN_NONE, "Failed to get DMA channel");
        ASSERT(trace_uart == HAL_UART_ID_0
#if (CHIP_HAS_UART > 1)
                || trace_uart == HAL_UART_ID_1
#endif
                ,
            "BAD trace uart ID: %d", trace_uart);
    }
#endif

    ret = hal_uart_open(trace_uart, &uart_cfg);
    if (ret) {
#if (TRACE_IDLE_OUTPUT == 0)
        hal_gpdma_free_chan(dma_cfg.ch);
        dma_cfg.ch = HAL_DMA_CHAN_NONE;
#endif
        return ret;
    }

#ifdef FAULT_DUMP
    NVIC_SetDefaultFaultHandler(hal_trace_fault_handler);
#endif

    trace_transport = transport;

#ifdef HAL_TRACE_RX_ENABLE
    hal_trace_rx_open();
#endif

    // Show build info
    static const char dbl_new_line[] = NEW_LINE_STR NEW_LINE_STR;
    hal_trace_output((unsigned char *)dbl_new_line, sizeof(dbl_new_line) - 1);
    hal_trace_output((unsigned char *)sys_build_info, strlen(sys_build_info));

    char buf[50];
    int len;
    len = snprintf(buf, sizeof(buf),
        NEW_LINE_STR NEW_LINE_STR "------" NEW_LINE_STR "CHIP_ID: %d" NEW_LINE_STR "------" NEW_LINE_STR NEW_LINE_STR,
        hal_get_chip_metal_id());
    hal_trace_output((unsigned char *)buf, len);

    return 0;
}

int hal_trace_switch(enum HAL_TRACE_TRANSPORT_T transport)
{
    uint32_t POSSIBLY_UNUSED lock;
    int ret;

#if (CHIP_HAS_UART > 1)
#ifdef FORCE_TRACE_UART1
    transport = HAL_TRACE_TRANSPORT_UART1;
#endif
#endif

#ifdef CHIP_HAS_USB
    if (transport == HAL_TRACE_TRANSPORT_USB) {
        return 1;
    }
#endif
    if (transport >= HAL_TRACE_TRANSPORT_QTY) {
        return 1;
    }
    if (trace_transport >= HAL_TRACE_TRANSPORT_QTY) {
        return 1;
    }
    if (trace_transport == transport) {
        return 0;
    }

    ret = 0;

#if (CHIP_HAS_UART > 1)

    lock = int_lock();

    if (trace_transport == HAL_TRACE_TRANSPORT_UART0 || trace_transport == HAL_TRACE_TRANSPORT_UART1) {
#if (TRACE_IDLE_OUTPUT == 0)
        if (dma_cfg.ch != HAL_DMA_CHAN_NONE) {
            hal_gpdma_cancel(dma_cfg.ch);
        }
#endif
        hal_uart_close(trace_uart);
    }

    if (transport == HAL_TRACE_TRANSPORT_UART0 || transport == HAL_TRACE_TRANSPORT_UART1) {
        if (transport == HAL_TRACE_TRANSPORT_UART0) {
            trace_uart = HAL_UART_ID_0;
        } else if (transport == HAL_TRACE_TRANSPORT_UART1) {
            trace_uart = HAL_UART_ID_1;
        }
#if (TRACE_IDLE_OUTPUT == 0)
        dma_cfg.dst_periph = (trace_uart == HAL_UART_ID_0) ? HAL_GPDMA_UART0_TX : HAL_GPDMA_UART1_TX;
        trace.sends[0] = 0;
        trace.sends[1] = 0;
#endif
        ret = hal_uart_open(trace_uart, &uart_cfg);
        if (ret) {
#if (TRACE_IDLE_OUTPUT == 0)
            hal_gpdma_free_chan(dma_cfg.ch);
            dma_cfg.ch = HAL_DMA_CHAN_NONE;
#endif
            trace_transport = HAL_TRACE_TRANSPORT_QTY;
            goto _exit;
        }
    }

    trace.sending = false;

    trace_transport = transport;

_exit:
    int_unlock(lock);

#endif // CHIP_HAS_UART > 1

    return ret;
}

int hal_trace_close(void)
{
    if (trace_transport >= HAL_TRACE_TRANSPORT_QTY) {
        goto _exit;
    }
#ifdef CHIP_HAS_USB
    if (trace_transport == HAL_TRACE_TRANSPORT_USB) {
        goto _exit;
    }
#endif

    if (trace_transport == HAL_TRACE_TRANSPORT_UART0
#if (CHIP_HAS_UART > 1)
            || trace_transport == HAL_TRACE_TRANSPORT_UART1
#endif
            ) {
#if (TRACE_IDLE_OUTPUT == 0)
        if (dma_cfg.ch != HAL_DMA_CHAN_NONE) {
            hal_gpdma_cancel(dma_cfg.ch);
            hal_gpdma_free_chan(dma_cfg.ch);
            dma_cfg.ch = HAL_DMA_CHAN_NONE;
        }
#endif
        hal_uart_close(trace_uart);
    }

_exit:
    trace_transport = HAL_TRACE_TRANSPORT_QTY;

    return 0;
}

void hal_trace_get_history_buffer(const unsigned char **buf1, unsigned int *len1, const unsigned char **buf2, unsigned int *len2)
{
    uint32_t lock;
    uint8_t *b1, *b2;
    uint32_t l1, l2;

    b1 = b2 = NULL;
    l1 = l2 = 0;

    lock = int_lock();

    if (TRACE_BUF_SIZE > trace.wptr) {
        if (trace.wrapped) {
            b1 = &trace.buf[trace.wptr];
            l1 = TRACE_BUF_SIZE - trace.wptr;
            b2 = &trace.buf[0];
            l2 = trace.wptr;
        } else {
            b1 = &trace.buf[0];
            l1 = trace.wptr;
            b2 = NULL;
            l2 = 0;
        }
    }

    int_unlock(lock);

    if (buf1) {
        *buf1 = b1;
    }
    if (len1) {
        *len1 = l1;
    }
    if (buf2) {
        *buf2 = b2;
    }
    if (len2) {
        *len2 = l2;
    }
}

static void hal_trace_print_discards(uint32_t discards)
{
    static const uint8_t base = 10;
    char digit[5], *d, *out;
    uint16_t len;
    uint16_t size;

    if (discards > max_discards) {
        discards = max_discards;
    }

    d = &digit[0];
    do {
        *d++ = (discards % base) + '0';
    } while (discards /= base);

    out = &discards_buf[discards_digit_start];
    do {
        *out++ = *--d;
    } while (d > &digit[0]);
#ifdef TRACE_CRLF
    *out++ = '\r';
#endif
    *out++ = '\n';
    len = out - &discards_buf[0];

    size = TRACE_BUF_SIZE - trace.wptr;
    if (size >= len) {
        size = len;
    }
    memcpy(&trace.buf[trace.wptr], &discards_buf[0], size);
    if (size < len) {
        memcpy(&trace.buf[0], &discards_buf[size], len - size);
    }
    trace.wptr += len;
    if (trace.wptr >= TRACE_BUF_SIZE) {
        trace.wptr -= TRACE_BUF_SIZE;
    }
}

#ifdef AUDIO_DEBUG_V0_1_0
static void hal_trace_print_head(void)
{
    uint16_t len;
    uint16_t size;

    len = sizeof(trace_head_buf) - 1;

    size = TRACE_BUF_SIZE - trace.wptr;
    if (size >= len) {
        size = len;
    }
    memcpy(&trace.buf[trace.wptr], &trace_head_buf[0], size);
    if (size < len) {
        memcpy(&trace.buf[0], &trace_head_buf[size], len - size);
    }
    trace.wptr += len;
    if (trace.wptr >= TRACE_BUF_SIZE) {
        trace.wptr -= TRACE_BUF_SIZE;
    }
}
#endif

int hal_trace_output(const unsigned char *buf, unsigned int buf_len)
{
    int ret;
    uint32_t lock;
    uint32_t avail;
    uint32_t out_len;
    uint16_t size;

    ret = 0;

    lock = int_lock();

#ifdef CHIP_HAS_CP
    while (hal_memsc_lock(HAL_MEMSC_ID_TRACE) == 0);
#endif

    // Avoid troubles when NMI occurs during trace
    if (!trace.in_trace) {
        trace.in_trace = true;

        if (trace.wptr >= trace.rptr) {
            avail = TRACE_BUF_SIZE - (trace.wptr - trace.rptr) - 1;
        } else {
            avail = (trace.rptr - trace.wptr) - 1;
        }

        out_len = buf_len;
#ifdef AUDIO_DEBUG_V0_1_0
        out_len += sizeof(trace_head_buf) - 1;
#endif
        if (trace.discards) {
            out_len += sizeof(discards_buf);
        }

        if (avail < out_len) {
            if (trace.discards < (1 << (sizeof(trace.discards) * 8)) - 1) {
                trace.discards++;
            }
            ret = 1;
        } else {
#ifdef AUDIO_DEBUG_V0_1_0
            hal_trace_print_head();
#endif

            if (trace.discards) {
                hal_trace_print_discards(trace.discards);
                trace.discards = 0;
            }

            size = TRACE_BUF_SIZE - trace.wptr;
            if (size >= buf_len) {
                size = buf_len;
            }
            memcpy(&trace.buf[trace.wptr], &buf[0], size);
            if (size < buf_len) {
                memcpy(&trace.buf[0], &buf[size], buf_len - size);
            }
            trace.wptr += buf_len;
            if (trace.wptr >= TRACE_BUF_SIZE) {
                trace.wptr -= TRACE_BUF_SIZE;
                trace.wrapped = true;
            }
#if (TRACE_IDLE_OUTPUT == 0)
            hal_trace_send();
#endif
        }

        trace.in_trace = false;

#if !(defined(ROM_BUILD) || defined(PROGRAMMER))
#ifdef TRACE_TO_APP
        if (app_output_cb && app_output_enabled) {
            bool saved_output_state;

            saved_output_state = app_output_enabled;
            app_output_enabled = false;

            app_output_cb(buf, buf_len);

            app_output_enabled = saved_output_state;
        }
#endif
#endif
    }

#ifdef CHIP_HAS_CP
    hal_memsc_unlock(HAL_MEMSC_ID_TRACE);
#endif
    int_unlock(lock);

    return ret ? 0 : buf_len;
}

#ifdef USE_TRACE_ID

typedef struct {
    uint16_t level:4;
    uint16_t module:4;
    uint16_t tskid:8;
}trace_type_t;

typedef struct {
    uint16_t count:4;
    uint16_t type:12;
}arg_type_t;

typedef struct {
    trace_type_t trace_type;
    arg_type_t arg_type;
    uint32_t timestamp;
    uint32_t msg_addr;
}__attribute__((packed)) LOG_DATA_T;

static int
_newtraceprnt(const char *fmt,unsigned int *value, unsigned short *arg_count, unsigned short *arg_type, va_list ap)
{
    char *p;
    const char *fmt0;
    int ch;
    unsigned long ul;
    int index = 0;
    if (fmt == NULL)
    {
        return 0;
    }

    for (;;)
    {
        while ((ch = *fmt++) != '%')
        {
            if (ch == '\0')
            {
                goto out;
            }
        }
        fmt0 = fmt; // useful to test if we are at the first char
reswitch:   switch (ch = *fmt++) {
        case '0':
            // 0 can be either zero padding if first after %
            //   or part of a precision or width if after... /
            if (fmt == fmt0 + 1)
            {
                goto reswitch;
            }
            // else fall through
        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case '-': case ' ': case '+': case '.': case 'h':
        case 'l':
            goto reswitch;
        case 'c':
            ch = va_arg(ap, int);
            value[index++] = ch & 0x7f;
            break;
        case 's':
            p = va_arg(ap, char *);
            if (p != NULL) {
                if (p < (char*)FLASH_BASE || p >= (char*)(FLASH_BASE + FLASH_SIZE)) {
                    return -1;
                }
            	*arg_type |= 1<<index;
            	value[index++] = (unsigned int)p;
            } else {
                return -1;
            }
            break;
        case 'i':
        case 'd':
            ul = va_arg(ap, long);
            value[index++] = ul;
            break;
        case 'o':
        case 'u':
        case 'p':
        case 'x':
        case 'X':
            ul = va_arg(ap, unsigned long);
            value[index++] = ul;
            break;
        case '%':
            break;
        default:
            if (ch == '\0')
            {
                goto out;
            }
        } /* end switch */
    } /* end for */
out:
    *arg_count = index;
    return 0;
}

static int hal_trace_format_id(uint32_t lvl, char *buf, uint32_t size, const char *fmt, va_list ap)
{
    unsigned short count = 0;
    unsigned short arg_type=0;
    unsigned int value[10];
    LOG_DATA_T trace;

    if (size < 4 + sizeof(trace) + sizeof(value)) {
        return -1;
    }
    if (_newtraceprnt(fmt,value,&count,&arg_type,ap) < 0) {
        return -2;
    }
    if (count > 10) {
        return -3;
    }

    memset(buf, 0, size);

    buf[2] = 0xBE;
    buf[3] = 0xBE;

    trace.trace_type.level = LOG_LV_INFO;
    trace.trace_type.module = LOG_MOD_SYS;
    trace.trace_type.tskid = osGetThreadIntId();
    trace.timestamp = TICKS_TO_MS(hal_sys_timer_get());
    trace.msg_addr = (uint32_t)fmt;
    trace.arg_type.type = arg_type;
    trace.arg_type.count = count;

    memcpy(buf+4,&trace,sizeof(trace));
    if (count != 0) {
        memcpy(buf+4+sizeof(trace), value, 4*count);
    }

    return sizeof(trace) + 4*count + 4;
}

static int hal_trace_format_id_no_arg(uint32_t lvl, char *buf, uint32_t size, const char *fmt)
{
    LOG_DATA_T trace;

    if (size < 4 + sizeof(trace)) {
        return -1;
    }

    memset(buf, 0, size);

    buf[2] = 0xBE;
    buf[3] = 0xBE;

    trace.trace_type.level = LOG_LV_INFO;
    trace.trace_type.module = LOG_MOD_SYS;
    trace.trace_type.tskid = osGetThreadIntId();
    trace.timestamp = TICKS_TO_MS(hal_sys_timer_get());
    trace.msg_addr = (uint32_t)fmt;
    trace.arg_type.type = 0;
    trace.arg_type.count = 0;

    memcpy(buf+4,&trace,sizeof(trace));

    return sizeof(trace) + 4;
}

#endif

static int hal_trace_print_time(char *buf, unsigned int size)
{
#ifdef TRACE_TIME_STAMP
    if (skip_time_stamp) {
        return 0;
    }
#ifdef RTOS
    char ctx[5];
    int8_t ipsr;
    if (0) {
#ifdef CHIP_HAS_CP
    } else if (get_cpu_id()) {
        ctx[0] = ' ';
        ctx[1] = 'C';
        ctx[2] = 'P';
        ctx[3] = '\0';
#endif
    } else if ((ipsr = __get_IPSR()) != 0) {
        int len;

        len = snprintf(ctx, sizeof(ctx), "%2d", (int8_t)(ipsr - NVIC_USER_IRQ_OFFSET));
        if (len + 1 < ARRAY_SIZE(ctx)) {
            ctx[len] = 'E';
            ctx[len + 1] = '\0';
        } else {
            ctx[ARRAY_SIZE(ctx) - 2] = '.';
            ctx[ARRAY_SIZE(ctx) - 1] = '\0';
        }
    } else {
        snprintf(ctx, sizeof(ctx), "%3d", osGetThreadIntId());
    }
    ctx[ARRAY_SIZE(ctx) - 1] = '\0';
    return snprintf(buf, size, "%9lu/%s | ", TICKS_TO_MS(hal_sys_timer_get()), ctx);
#else
    return snprintf(buf, size, "%9lu | ", TICKS_TO_MS(hal_sys_timer_get()));
#endif
#else
    return 0;
#endif
}

static inline int hal_trace_format_va(char *buf, unsigned int size, int crlf, const char *fmt, va_list ap)
{
    int len;

    len = vsnprintf(&buf[0], size, fmt, ap);
    if (crlf) {
#ifdef TRACE_CRLF
        if (len + 2 < size) {
            buf[len++] = '\r';
        }
#endif
        if (len + 1 < size) {
            buf[len++] = '\n';
        }
    }
    //if (len < size) buf[len] = 0;

    return len;
}

int hal_trace_printf_with_lvl(unsigned int lvl, const char *fmt, ...)
{
    if (!(lvl & trace_level_mask)){
        return 0;
    }

    char buf[100];
    int len;
    va_list ap;

    va_start(ap, fmt);
    len = hal_trace_format_va(buf, sizeof(buf), true, fmt, ap);
    va_end(ap);

    return hal_trace_output((unsigned char *)buf, len);
}

static int hal_trace_printf_internal(const char *tag, const char *fmt, int crlf, va_list ap)
{
#ifdef USE_TRACE_ID
    char buf[60];
#else
    char buf[120];
#endif
    int len;

#ifdef USE_TRACE_ID
    if ((len = hal_trace_format_id(HAL_TRACE_LEVEL_0, buf, sizeof(buf), fmt, ap)) < 0)
#endif
    {
        len = 0;
        len += hal_trace_print_time(&buf[len], sizeof(buf) - len);
        if (tag != NULL) {
            len += snprintf(&buf[len], sizeof(buf) - len, "[%s]: ", tag);
        }
        len += hal_trace_format_va(&buf[len], sizeof(buf) - len, crlf, fmt, ap);
    }

    return hal_trace_output((unsigned char *)buf, len);
}

int hal_trace_printf(const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = hal_trace_printf_internal(NULL, fmt, true, ap);
    va_end(ap);

    return ret;
}

void hal_trace_printf_imm(const char *fmt, ...)
{
    va_list ap;

    hal_trace_flush_buffer();

    va_start(ap, fmt);
    hal_trace_printf_internal(NULL, fmt, true, ap);
    va_end(ap);

    hal_trace_flush_buffer();
}

int hal_trace_printf_without_crlf(const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = hal_trace_printf_internal(NULL, fmt, false, ap);
    va_end(ap);

    return ret;
}

int hal_trace_printf_with_tag(const char *tag, const char *fmt, ...)
{
#ifdef AUDIO_DEBUG
    return 0;
#else
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = hal_trace_printf_internal(tag, fmt, true, ap);
    va_end(ap);

    return ret;
#endif
}

int hal_trace_printf_without_crlf_fix_arg(const char *fmt)
{
    char buf[40];
    int len;

#ifdef USE_TRACE_ID
    if ((len = hal_trace_format_id_no_arg(HAL_TRACE_LEVEL_0, buf, sizeof(buf), fmt)) < 0)
#endif
    {
        len = 0;
        len += hal_trace_print_time(&buf[len], sizeof(buf) - len);
        if (len > 0) {
            hal_trace_output((unsigned char *)buf, len);
        }
        return hal_trace_output((unsigned char *)fmt, strlen(fmt));
    }

    return hal_trace_output((unsigned char *)buf, len);
}

int hal_trace_dump(const char *fmt, unsigned int size,  unsigned int count, const void *buffer)
{
    char buf[255];
    int len=0, n=0, i=0;

    switch( size )
    {
        case sizeof(uint32_t):
            while(i<count && len<sizeof(buf))
            {
                len += snprintf(&buf[len], sizeof(buf) - len, fmt, *(uint32_t *)((uint32_t *)buffer+i));
                i++;
            }
            break;
        case sizeof(uint16_t):
                while(i<count && len<sizeof(buf))
                {
                    len += snprintf(&buf[len], sizeof(buf) - len, fmt, *(int16_t *)((int16_t *)buffer+i));
                    i++;
                }
                break;
        case sizeof(uint8_t):
        default:
            while(i<count && len<sizeof(buf))
            {
                len += snprintf(&buf[len], sizeof(buf) - len, fmt, *(uint8_t *)((uint8_t *)buffer+i));
                i++;
            }
            break;
    }

#ifdef TRACE_CRLF
    if (len + 2 < sizeof(buf)) {
        buf[len++] = '\r';
    }
#endif
    if (len + 1 < sizeof(buf)) {
        buf[len++] = '\n';
    }

    n = hal_trace_output((unsigned char *)buf, len);

    return n;
}

int hal_trace_busy(void)
{
    union HAL_UART_FLAG_T flag;

    if (trace_transport == HAL_TRACE_TRANSPORT_UART0 ||
            trace_transport == HAL_TRACE_TRANSPORT_UART1) {
        if (hal_uart_opened(trace_uart)) {
            flag = hal_uart_get_flag(trace_uart);
            return flag.BUSY;
        }
    }
    return 0;
}

int hal_trace_pause(void)
{
    if (trace_transport == HAL_TRACE_TRANSPORT_UART0 ||
            trace_transport == HAL_TRACE_TRANSPORT_UART1) {
        return hal_uart_pause(trace_uart);
    }
    return 1;
}

int hal_trace_continue(void)
{
    if (trace_transport == HAL_TRACE_TRANSPORT_UART0 ||
            trace_transport == HAL_TRACE_TRANSPORT_UART1) {
        return hal_uart_continue(trace_uart);
    }
    return 1;
}

void hal_trace_flush_buffer(void)
{
    uint32_t lock;

    if (trace_transport == HAL_TRACE_TRANSPORT_USB ||
            trace_transport >= HAL_TRACE_TRANSPORT_QTY) {
        return;
    }

#ifdef CHIP_HAS_CP
    if (get_cpu_id()) {
        return;
    }
#endif

    hal_uart_continue(trace_uart);

    lock = int_lock();

#if (TRACE_IDLE_OUTPUT == 0)
    while (trace.wptr != trace.rptr) {
        while (hal_gpdma_chan_busy(dma_cfg.ch));
        hal_gpdma_irq_run_chan(dma_cfg.ch);
    }
#else
    while (trace.wptr != trace.rptr) {
        hal_trace_idle_send();
    }
#endif

    int_unlock(lock);
}

uint32_t hal_trace_get_backtrace_addr(uint32_t addr)
{
    if (!hal_trace_address_executable(addr)) {
        return 0;
    }

#ifdef __ARM_ARCH_7EM__
    // BL Instruction
    // OFFSET: 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    // VALUE :  1  1  1  1  0  -  -  -  -  -  -  -  -  -  -  -  1  1  -  1  -  -  -  -  -  -  -  -  -  -  -  -

    // BLX Instruction
    // OFFSET: 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    // VALUE :  0  1  0  0  0  1  1  1  1  -  -  -  -  -  -  -

    uint16_t val;
    uint32_t new_addr;

    new_addr = (addr & ~1) - 2;

    val = *(uint16_t *)new_addr;
    if ((val & 0xFF80) == 0x4780) {
        // BLX
        return new_addr;
    } else if ((val & 0xD000) == 0xD000) {
        new_addr -= 2;
        val = *(uint32_t *)new_addr;
        if ((val & 0xF800) == 0xF000) {
            // BL
            return new_addr;
        }
    }
#else
#error "Only ARMv7-M function can be patched"
#endif

    return 0;
}

void hal_trace_print_backtrace(uint32_t addr, uint32_t search_cnt, uint32_t print_cnt)
{
    static const char bt_title[] = "Possible Backtrace:" NEW_LINE_STR;
    int i, j;
    int len;
    uint32_t *stack;
    uint32_t call_addr;

    if ((addr & 3) || !hal_trace_address_writable(addr)) {
        return;
    }

    hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);
    hal_trace_output((const unsigned char *)bt_title, sizeof(bt_title) - 1);

    stack = (uint32_t *)addr;
    for (i = 0, j = 0; i < search_cnt && j < print_cnt; i++) {
        if (!hal_trace_address_writable((uint32_t)&stack[i])) {
            break;
        }
        call_addr = hal_trace_get_backtrace_addr(stack[i]);
        if (call_addr) {
            len = snprintf(crash_buf, sizeof(crash_buf), "%8lX" NEW_LINE_STR, call_addr);
            hal_trace_output((unsigned char *)crash_buf, len);
            j++;
        }
    }
}

#if (defined(ROM_BUILD) || defined(PROGRAMMER))
int hal_trace_crash_dump_register(HAL_TRACE_CRASH_DUMP_CB_T cb)
{
    return 0;
}

void hal_trace_app_register(HAL_TRACE_APP_NOTIFY_T notify_cb, HAL_TRACE_APP_OUTPUT_T output_cb)
{

}
#else
int hal_trace_crash_dump_register(HAL_TRACE_CRASH_DUMP_CB_T cb)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(crash_dump_cb_list); i++) {
        if (crash_dump_cb_list[i] == NULL) {
            crash_dump_cb_list[i] = cb;
            return 0;
        }
    }

    return 1;
}

static void hal_trace_crash_dump_callback(void)
{
    int i;

#ifdef TRACE_TIME_STAMP
    skip_time_stamp = true;
#endif

    for (i = 0; i < ARRAY_SIZE(crash_dump_cb_list); i++) {
        if (crash_dump_cb_list[i]) {
            crash_dump_cb_list[i]();
        }
    }
}

void hal_trace_app_register(HAL_TRACE_APP_NOTIFY_T notify_cb, HAL_TRACE_APP_OUTPUT_T output_cb)
{
#ifdef TRACE_TO_APP
    app_notify_cb = notify_cb;
    app_output_cb = output_cb;
#endif
}

void hal_trace_app_crash_register(HAL_TRACE_APP_NOTIFY_T notify_cb, HAL_TRACE_APP_OUTPUT_T output_cb,HAL_TRACE_APP_OUTPUT_T crash_fault_cb)
{
#ifdef TRACE_TO_APP
    app_notify_cb = notify_cb;
    app_output_cb = output_cb;
    app_crash_fault_cb = crash_fault_cb;
#endif
}

#endif

#else // !(DEBUG || REL_TRACE_ENABLE)

int hal_trace_open(enum HAL_TRACE_TRANSPORT_T transport)
{
#ifdef FAULT_DUMP
    NVIC_SetDefaultFaultHandler(hal_trace_fault_handler);
#endif

    return 0;
}

#endif // !(DEBUG || REL_TRACE_ENABLE)

int hal_trace_address_writable(uint32_t addr)
{
    if (RAM_BASE < addr && addr < RAM_BASE + RAM_SIZE) {
        return 1;
    }
#ifdef PSRAM_BASE
    if (PSRAM_BASE < addr && addr < PSRAM_BASE + PSRAM_SIZE) {
        return 1;
    }
#endif
#ifdef PSRAM_NC_BASE
    if (PSRAM_NC_BASE < addr && addr < PSRAM_NC_BASE + PSRAM_SIZE) {
        return 1;
    }
#endif
#ifdef RAMRET_BASE
    if (RAMRET_BASE < addr && addr < RAMRET_BASE + RAMRET_SIZE) {
        return 1;
    }
#endif
    return 0;
}

int hal_trace_address_executable(uint32_t addr)
{
// Avoid reading out of valid memory region when parsing instruction content
#define X_ADDR_OFFSET                   0x10

    // Check thumb code
    if ((addr & 1) == 0) {
        return 0;
    }
    // Check location
    if (RAMX_BASE + X_ADDR_OFFSET < addr && addr < RAMX_BASE + RAM_SIZE) {
        return 1;
    }
    if (FLASHX_BASE + X_ADDR_OFFSET < addr && addr < FLASHX_BASE + FLASH_SIZE) {
        return 1;
    }
#ifdef PSRAMX_BASE
    if (PSRAMX_BASE + X_ADDR_OFFSET < addr && addr < PSRAMX_BASE + PSRAM_SIZE) {
        return 1;
    }
#endif
#ifdef RAMXRET_BASE
    if (RAMXRET_BASE < addr && addr < RAMXRET_BASE + RAMRET_SIZE) {
        return 1;
    }
#endif

//#define CHECK_ROM_CODE
#ifdef CHECK_ROM_CODE
#ifndef USED_ROM_SIZE
#define USED_ROM_SIZE                   ROM_SIZE
#endif
    if (ROM_BASE + (NVIC_USER_IRQ_OFFSET * 4) < addr && addr < ROM_BASE + USED_ROM_SIZE) {
        return 1;
    }
#endif

#if 0
    if (FLASHX_NC_BASE < addr && addr < FLASHX_NC_BASE + FLASH_SIZE) {
        return 1;
    }
#ifdef PSRAMX_NC_BASE
    if (PSRAMX_NC_BASE < addr && addr < PSRAMX_NC_BASE + PSRAM_SIZE) {
        return 1;
    }
#endif
#endif
    return 0;
}

int hal_trace_address_readable(uint32_t addr)
{
    if (hal_trace_address_writable(addr)) {
        return 1;
    }
    if (hal_trace_address_executable(addr)) {
        return 1;
    }
    if (FLASH_BASE < addr && addr < FLASH_BASE + FLASH_SIZE) {
        return 1;
    }
    if (FLASH_NC_BASE < addr && addr < FLASH_NC_BASE + FLASH_SIZE) {
        return 1;
    }
#ifdef PSRAM_NC_BASE
    if (PSRAM_NC_BASE < addr && addr < PSRAM_NC_BASE + PSRAM_SIZE) {
        return 1;
    }
#endif
    return 0;
}

static void NORETURN hal_trace_crash_end(void)
{
    // Tag failure for simulation environment
    hal_cmu_simu_fail();

#ifdef CRASH_REBOOT
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT|HAL_SW_BOOTMODE_REBOOT_FROM_CRASH);
    hal_cmu_sys_reboot();	
#else
    hal_iomux_set_analog_i2c();
    hal_iomux_set_jtag();
    hal_cmu_jtag_clock_enable();
#endif

    SAFE_PROGRAM_STOP();
}

void hal_trace_assert_dump(ASSERT_DUMP_ARGS)
{
    struct HAL_TRACE_ASSERT_INFO_T info;
    register uint32_t r0 asm("r0");
    register uint32_t r1 asm("r1");
    register uint32_t r2 asm("r2");
    register uint32_t r3 asm("r3");
    register uint32_t sp asm("sp");
    register uint32_t lr asm("lr");

    // MUST SAVE REGISTERS FIRST!
    info.PC = lr;
    info.R[0] = r0;
    info.R[1] = r1;
    info.R[2] = r2;
    info.R[3] = r3;
    info.SP = sp;

    int_lock();

    info.ID = HAL_TRACE_ASSERT_ID;
#if (defined(DEBUG) || defined(REL_TRACE_ENABLE)) && defined(ASSERT_SHOW_FILE_FUNC)
    info.FILE = file;
#elif (defined(DEBUG) || defined(REL_TRACE_ENABLE)) && defined(ASSERT_SHOW_FILE)
    info.FILE = scope;
#else
    info.FILE = NULL;
#endif
#if (defined(DEBUG) || defined(REL_TRACE_ENABLE)) && defined(ASSERT_SHOW_FILE_FUNC)
    info.FUNC = func;
#elif (defined(DEBUG) || defined(REL_TRACE_ENABLE)) && defined(ASSERT_SHOW_FUNC)
    info.FUNC = scope;
#else
    info.FUNC = NULL;
#endif
#if (defined(DEBUG) || defined(REL_TRACE_ENABLE)) && (defined(ASSERT_SHOW_FILE_FUNC) || defined(ASSERT_SHOW_FILE) || defined(ASSERT_SHOW_FUNC))
    info.LINE = line;
#else
    info.LINE = 0;
#endif
    info.FMT = fmt;
    info.MSP = __get_MSP();
    info.PSP = __get_PSP();
    info.CONTROL = __get_CONTROL();

    *(volatile uint32_t *)RAM_BASE = (uint32_t)&info;

#if defined(DUMP_CRASH_ENABLE)
    crash_dump_init_buffer();
#endif

#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))
    static const char POSSIBLY_UNUSED desc_file[] = "FILE    : ";
    static const char POSSIBLY_UNUSED desc_func[] = "FUNCTION: ";
    static const char POSSIBLY_UNUSED desc_line[] = "LINE    : ";
    int len;
    va_list ap;

#if !(defined(ROM_BUILD) || defined(PROGRAMMER))
#ifdef TRACE_TO_APP
    if (app_notify_cb) {
        app_notify_cb(HAL_TRACE_STATE_CRASH_ASSERT_START);
    }
    app_output_enabled = true;
#endif
#endif

    hal_trace_flush_buffer();

    len = hal_trace_print_time(&crash_buf[0], sizeof(crash_buf));
    if (len > 0) {
        hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);
        hal_trace_output((unsigned char *)crash_buf, len);
    }
    len = snprintf(&crash_buf[0], sizeof(crash_buf), NEW_LINE_STR "### ASSERT @ 0x%08lX ###" NEW_LINE_STR, info.PC);
    hal_trace_output((unsigned char *)crash_buf, len);

#if defined(ASSERT_SHOW_FILE_FUNC) || defined(ASSERT_SHOW_FILE) || defined(ASSERT_SHOW_FUNC)
    const char separate_line[] = "----------------------------------------" NEW_LINE_STR;
#ifdef ASSERT_SHOW_FILE
    const char *file = scope;
#elif defined(ASSERT_SHOW_FUNC)
    const char *func = scope;
#endif

#if defined(ASSERT_SHOW_FILE_FUNC) || defined(ASSERT_SHOW_FILE)
    hal_trace_output((const unsigned char *)desc_file, sizeof(desc_file) - 1);
    hal_trace_output((const unsigned char *)file, strlen(file));
    hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);
#endif

#if defined(ASSERT_SHOW_FILE_FUNC) || defined(ASSERT_SHOW_FUNC)
    hal_trace_output((const unsigned char *)desc_func, sizeof(desc_func) - 1);
    hal_trace_output((const unsigned char *)func, strlen(func));
    hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);
#endif

    hal_trace_output((const unsigned char *)desc_line, sizeof(desc_func) - 1);
    len = snprintf(crash_buf, sizeof(crash_buf), "%u", line);
    hal_trace_output((const unsigned char *)crash_buf, len);
    hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);

    hal_trace_output((unsigned char *)separate_line, sizeof(separate_line) - 1);

    hal_trace_flush_buffer();
#endif

    va_start(ap, fmt);
    len = hal_trace_format_va(crash_buf, sizeof(crash_buf), true, fmt, ap);
    va_end(ap);

    hal_trace_output((unsigned char *)crash_buf, len);

    hal_trace_flush_buffer();

    hal_trace_print_backtrace(info.SP, TRACE_BACKTRACE_SEARCH_WORD, TRACE_BACKTRACE_NUM);

    hal_trace_flush_buffer();

#if !(defined(ROM_BUILD) || defined(PROGRAMMER))
    hal_trace_crash_dump_callback();
#endif

    hal_trace_flush_buffer();

    hal_sys_timer_delay(MS_TO_TICKS(5));

#if !(defined(ROM_BUILD) || defined(PROGRAMMER))
#ifdef TRACE_TO_APP
    if (app_notify_cb) {
        app_notify_cb(HAL_TRACE_STATE_CRASH_END);
    }
#endif
#endif
#endif // DEBUG || REL_TRACE_ENABLE

    hal_trace_crash_end();
}

#ifdef FAULT_DUMP
static void hal_trace_fault_dump(const uint32_t *regs, const uint32_t *fpu_regs)
{
    struct HAL_TRACE_EXCEPTION_INFO_T info;
    uint32_t primask;

    primask = __get_PRIMASK();

    int_lock();

    info.ID = HAL_TRACE_EXCEPTION_ID;
    info.REGS = regs;
    info.MSP = __get_MSP();
    info.PSP = __get_PSP();
    info.PRIMASK = primask;
    info.FAULTMASK = __get_FAULTMASK();
    info.BASEPRI = __get_BASEPRI();
    info.CONTROL = __get_CONTROL();
    info.ICSR = SCB->ICSR;
    info.AIRCR = SCB->AIRCR;
    info.SCR = SCB->SCR;
    info.CCR = SCB->CCR;
    info.SHCSR = SCB->SHCSR;
    info.CFSR = SCB->CFSR;
    info.HFSR = SCB->HFSR;
    info.AFSR = SCB->AFSR;
    info.MMFAR = SCB->MMFAR;
    info.BFAR = SCB->BFAR;

    *(volatile uint32_t *)RAM_BASE = (uint32_t)&info;

#if defined(DUMP_CRASH_ENABLE)
    crash_dump_init_buffer();
#endif

#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))
    static const char title[] = NEW_LINE_STR "### EXCEPTION ###" NEW_LINE_STR;
    int len;
    int i;
    int index;
    uint32_t val;
    uint32_t *stack;

#if !(defined(ROM_BUILD) || defined(PROGRAMMER))
#ifdef TRACE_TO_APP
    if (app_notify_cb) {
        app_notify_cb(HAL_TRACE_STATE_CRASH_FAULT_START);
    }
    if(!app_crash_fault_cb){
        app_output_enabled = true;
    }
#endif
#endif

    hal_trace_flush_buffer();

    len = hal_trace_print_time(&crash_buf[0], sizeof(crash_buf));
    if (len > 0) {
        hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);
        hal_trace_output((unsigned char *)crash_buf, len);
    }
    hal_trace_output((unsigned char *)title, sizeof(title) - 1);

    len = snprintf(crash_buf, sizeof(crash_buf), "PC =%08lX", regs[15]);
    val = __get_IPSR();
    if (val == 0) {
        len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, ", ThreadMode" NEW_LINE_STR);
    } else {
        len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, ", ExceptionNumber=%d" NEW_LINE_STR, (int)val - 16);
    }
    hal_trace_output((unsigned char *)crash_buf, len);

    for (i = 0; i < 3; i++) {
        index = i * 4;
        len = snprintf(crash_buf, sizeof(crash_buf), "R%-2d=%08lX, R%-2d=%08lX, R%-2d=%08lX, R%-2d=%08lX" NEW_LINE_STR,
            index, regs[index], index + 1, regs[index + 1], index + 2, regs[index + 2], index + 3, regs[index + 3]);
        hal_trace_output((unsigned char *)crash_buf, len);
    }
    len = snprintf(crash_buf, sizeof(crash_buf), "R12=%08lX, SP =%08lX, LR =%08lX" NEW_LINE_STR, regs[12], regs[13], regs[14]);
    hal_trace_output((unsigned char *)crash_buf, len);

    len = snprintf(crash_buf, sizeof(crash_buf), "MSP=%08lX, PSP=%08lX" NEW_LINE_STR, __get_MSP(), __get_PSP());
    hal_trace_output((unsigned char *)crash_buf, len);

    hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);

    len = snprintf(crash_buf, sizeof(crash_buf), "PRIMASK=%02lX, FAULTMASK=%02lX, BASEPRI=%02lX, CONTROL=%02lX" NEW_LINE_STR,
        primask, __get_FAULTMASK(), __get_BASEPRI(), __get_CONTROL());
    hal_trace_output((unsigned char *)crash_buf, len);
    len = snprintf(crash_buf, sizeof(crash_buf), "XPSR=%08lX", regs[16]);
    len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, ", APSR=%c%c%c%c%c",
        (regs[16] & (1 << 31)) ? 'N' : 'n',
        (regs[16] & (1 << 30)) ? 'Z' : 'z',
        (regs[16] & (1 << 29)) ? 'C' : 'c',
        (regs[16] & (1 << 28)) ? 'V' : 'v',
        (regs[16] & (1 << 27)) ? 'Q' : 'q'
        );
    val = regs[16] & 0xFF;
    len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, ", EPSR=%08lX, IPSR=%02lX", regs[16] & 0x0700FE00, val);
    if (val == 0) {
        len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (NoException)");
    }
    hal_trace_output((unsigned char *)crash_buf, len);
    hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);
    hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);

    hal_trace_flush_buffer();

    len = snprintf(crash_buf, sizeof(crash_buf), "ICSR =%08lX, AIRCR=%08lX, SCR  =%08lX, CCR  =%08lX" NEW_LINE_STR, SCB->ICSR, SCB->AIRCR, SCB->SCR, SCB->CCR);
    hal_trace_output((unsigned char *)crash_buf, len);

    len = snprintf(crash_buf, sizeof(crash_buf), "SHCSR=%08lX, CFSR =%08lX, HFSR =%08lX, AFSR =%08lX" NEW_LINE_STR, SCB->SHCSR, SCB->CFSR, SCB->HFSR, SCB->AFSR);
    hal_trace_output((unsigned char *)crash_buf, len);

    len = snprintf(crash_buf, sizeof(crash_buf), "MMFAR=%08lX, BFAR =%08lX" NEW_LINE_STR, SCB->MMFAR, SCB->BFAR);
    hal_trace_output((unsigned char *)crash_buf, len);

    if (SCB->HFSR & (1 << 30)) {
        len = snprintf(crash_buf, sizeof(crash_buf), "(Escalation HardFault)" NEW_LINE_STR);
        hal_trace_output((unsigned char *)crash_buf, len);
    }

    len = snprintf(crash_buf, sizeof(crash_buf), "FaultInfo :");
    if ((SCB->SHCSR & 0xF) == 0) {
        len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (None)");
    } else {
        if (SCB->SHCSR & (1 << 0)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (MemFault)");
        }
        if (SCB->SHCSR & (1 << 1)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (BusFault)");
        }
        if (SCB->SHCSR & (1 << 3)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (UsageFault)");
        }
    }
    hal_trace_output((unsigned char *)crash_buf, len);
    hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);

    len = snprintf(crash_buf, sizeof(crash_buf), "FaultCause:");
    if (SCB->CFSR == 0) {
        len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (None)");
    } else {
        if (SCB->CFSR & (1 << 0)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (Instruction access violation)");
        }
        if (SCB->CFSR & (1 << 1)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (Data access violation)");
        }
        if (SCB->CFSR & (1 << 3)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (MemFault on unstacking for a return from exception)");
        }
        if (SCB->CFSR & (1 << 4)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (MemFault on stacking for exception entry)");
        }
        if (SCB->CFSR & (1 << 5)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (MemFault during floating-point lazy state preservation)");
        }
        if (SCB->CFSR & (1 << 7)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (MMFAR valid)");
        }
        if (len) {
            hal_trace_output((unsigned char *)crash_buf, len);
            hal_trace_flush_buffer();
            len = 0;
        }
        if (SCB->CFSR & (1 << 8)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (Instruction bus error)");
        }
        if (SCB->CFSR & (1 << 9)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (Precise data bus error)");
        }
        if (SCB->CFSR & (1 << 10)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (Imprecise data bus error)");
        }
        if (SCB->CFSR & (1 << 11)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (BusFault on unstacking for a return from exception)");
        }
        if (SCB->CFSR & (1 << 12)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (BusFault on stacking for exception entry)");
        }
        if (SCB->CFSR & (1 << 13)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (BusFault during floating-point lazy state preservation)");
        }
        if (SCB->CFSR & (1 << 15)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (BFAR valid)");
        }
        if (len) {
            hal_trace_output((unsigned char *)crash_buf, len);
            hal_trace_flush_buffer();
            len = 0;
        }
        if (SCB->CFSR & (1 << 16)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (Undefined instruction UsageFault)");
        }
        if (SCB->CFSR & (1 << 17)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (Invalid state UsageFault)");
        }
        if (SCB->CFSR & (1 << 18)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (Invalid PC load by EXC_RETURN UsageFault)");
        }
        if (SCB->CFSR & (1 << 19)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (No coprocessor UsageFault)");
        }
        if (SCB->CFSR & (1 << 24)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (Unaligned access UsageFault)");
        }
        if (SCB->CFSR & (1 << 25)) {
            len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (Divide by zero UsageFault)");
        }
    }
    hal_trace_output((unsigned char *)crash_buf, len);
    hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);

    hal_trace_flush_buffer();

    hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);
    stack = (uint32_t *)(regs[13] & ~3);
    for (i = 0; i < TRACE_STACK_DUMP_WORD; i += 4) {
        len = snprintf(crash_buf, sizeof(crash_buf), "%08lX: %08lX %08lX %08lX %08lX" NEW_LINE_STR,
            (uint32_t)&stack[i], stack[i], stack[i + 1], stack[i + 2], stack[i + 3]);
        hal_trace_output((unsigned char *)crash_buf, len);
        hal_trace_flush_buffer();
    }

    hal_trace_print_backtrace((uint32_t)stack, TRACE_BACKTRACE_SEARCH_WORD, TRACE_BACKTRACE_NUM);

    hal_trace_flush_buffer();

#if !(defined(ROM_BUILD) || defined(PROGRAMMER))
    hal_trace_crash_dump_callback();
#endif

    hal_trace_flush_buffer();

// **********     Begin Crash-Dump "Lite-Version"   ************* //
#if !(defined(ROM_BUILD) || defined(PROGRAMMER))
#ifdef TRACE_TO_APP
        if (app_crash_fault_cb) {
            app_crash_fault_cb((unsigned char *)regs,
                           CRASH_DUMP_REGISTERS_NUM_BYTES);
            stack =  (uint32_t *)(__get_MSP() & ~3);
            app_crash_fault_cb((unsigned char *)stack,
                           ((CRASH_DUMP_STACK_NUM_BYTES)/2));
            stack =  (uint32_t *)(__get_PSP() & ~3);
            app_crash_fault_cb((unsigned char *)stack,
                          ((CRASH_DUMP_STACK_NUM_BYTES)/2));
        }
#endif
#endif
// **********     End Crash-Dump "Lite-Version"     ************* //

    hal_sys_timer_delay(MS_TO_TICKS(5));

#if !(defined(ROM_BUILD) || defined(PROGRAMMER))
#ifdef TRACE_TO_APP
    if (app_notify_cb) {
        app_notify_cb(HAL_TRACE_STATE_CRASH_END);
    }
#endif
#endif
#endif // DEBUG || REL_TRACE_ENABLE

    hal_trace_crash_end();
}

static void NAKED hal_trace_fault_handler(void)
{
    register uint32_t dump_func asm("r2") = (uint32_t)hal_trace_fault_dump;

    asm volatile (
        "tst lr, #0x04 \n"
        "ite eq \n"
        // Using MSP
        "mrseq r3, msp \n"
        // Using PSP
        "mrsne r3, psp \n"
        "tst lr, #0x10 \n"
        "ite eq \n"
        // FPU context saved
        "addeq r1, r3, #8*4 \n"
        // No FPU context
        "movne r1, #0 \n"
        // Make room for r0-r15,psr
        "sub sp, #17*4 \n"
        // Save r4-r11
        "add r0, sp, #4*4 \n"
        "stm r0, {r4-r11} \n"
        // Save r0-r3
        "ldm r3, {r4-r7} \n"
        "stm sp, {r4-r7} \n"
        // Save r12
        "ldr r0, [r3, #4*4] \n"
        "str r0, [sp, #12*4] \n"
        // Save sp
        "teq r1, 0 \n"
        "ite eq \n"
        "addeq r0, r3, #8*4 \n"
        "addne r0, r1, #17*4 \n"
        // -- Check CCR.STKALIGN
        "ldr r4, =0xE000ED14 \n"
        "ldr r4, [r4] \n"
        "tst r4, #(1 << 9) \n"
        "itt ne \n"
        "addne r0, #4 \n"
        "bfcne r0, 0, #3 \n"
        "str r0, [sp, #13*4] \n"
        // Save lr
        "ldr r0, [r3, #5*4] \n"
        "str r0, [sp, #14*4] \n"
        // Save pc
        "ldr r0, [r3, #6*4] \n"
        "str r0, [sp, #15*4] \n"
        // Save PSR
        "ldr r0, [r3, #7*4] \n"
        "str r0, [sp, #16*4] \n"
        // Save current exception lr
        "mov r4, lr \n"
        // Invoke the fault handler
        "mov r0, sp \n"
        "blx %0 \n"
        // Restore current exception lr
        "mov lr, r4 \n"
        // Restore r4-r7
        "add r0, sp, #4*4 \n"
        "ldm r0, {r4-r7} \n"
        "mov r0, r3 \n"
        // Restore sp
        "add sp, #17*4 \n"
        ""
        :
        : "r"(dump_func)
        );
}
#endif

void os_error_str (const char *fmt, ...) __attribute__((alias("hal_trace_assert_dump")));

#ifdef HAL_TRACE_RX_ENABLE

#define HAL_TRACE_RX_HEAD_SIZE   4
#define HAL_TRACE_RX_NAME_SIZE   20
#define HAL_TRACE_RX_BUF_SIZE    1024
#define HAL_TRACE_RX_ROLE_NUM    6

#define HAL_TRACE_RX_HEAD       '['
#define HAL_TRACE_RX_END        ']'
#define HAL_TRACE_RX_SEPARATOR  ','

//static struct HAL_DMA_CH_CFG_T dma_cfg_rx;
static struct HAL_DMA_DESC_T dma_desc_rx;

typedef struct{
    char *name;
    uint32_t len;
    uint8_t  *buf;
}HAL_TRACE_RX_CFG_T;

typedef struct{
    char name[HAL_TRACE_RX_NAME_SIZE];
    HAL_TRACE_RX_CALLBACK_T callback;
}HAL_TRACE_RX_LIST_T;

typedef struct{
    uint32_t list_num;
    HAL_TRACE_RX_LIST_T list[HAL_TRACE_RX_ROLE_NUM];

    uint32_t rx_enable;
    uint32_t pos;
    uint8_t  buf[HAL_TRACE_RX_BUF_SIZE];
}HAL_TRACE_RX_T;

#if defined(_AUTO_TEST_)
extern int auto_test_prase(uint8_t *cmd);
#endif

HAL_TRACE_RX_T hal_trace_rx;

int hal_trace_rx_dump_list(void)
{
    for(int i=0;i<HAL_TRACE_RX_ROLE_NUM;i++)
    {
        TRACE("%d: %s", i, hal_trace_rx.list[i].name);
    }
    return 0;
}

int hal_trace_rx_is_in_list(const char *name)
{
    for(int i=0;i<HAL_TRACE_RX_ROLE_NUM;i++)
    {
        if(!strcmp(hal_trace_rx.list[i].name, name))
        {
            return i;
        }
    }
    hal_trace_rx_dump_list();
    // TRACE("%s", hal_trace_rx.list[0].name);
    // TRACE("%s", name);
    // TRACE("%d", strlen(hal_trace_rx.list[0].name));
    // TRACE("%d", strlen(name));
    // TRACE("%d", strcmp(hal_trace_rx.list[0].name, name));
    return -1;
}

int hal_trace_rx_add_item_to_list(const char *name, HAL_TRACE_RX_CALLBACK_T callback)
{
    for(int i=0;i<HAL_TRACE_RX_ROLE_NUM;i++)
    {
        if(hal_trace_rx.list[i].name[0] == 0)
        {
            memcpy(hal_trace_rx.list[i].name, name, strlen(name));
            hal_trace_rx.list[i].callback = callback;
            hal_trace_rx.list_num++;
            return 0;
        }
    }

    return 1;
}

int hal_trace_rx_del_item_to_list(int id)
{
    memset(hal_trace_rx.list[id].name, 0, sizeof(hal_trace_rx.list[id].name));
    hal_trace_rx.list[id].callback = NULL;
    hal_trace_rx.list_num--;

    return 0;
}

int hal_trace_rx_register(const char *name, HAL_TRACE_RX_CALLBACK_T callback)
{
    TRACE("[%s] Add %s", __func__, name);
    if(hal_trace_rx_is_in_list(name) == -1)
    {
        hal_trace_rx_add_item_to_list(name, callback);
        return 0;
    }
    else
    {
        return 0;
    }
}

int hal_trace_rx_deregister(const char *name)
{
    int id = 0;

    id = hal_trace_rx_is_in_list(name);

    if(id != -1)
    {
        hal_trace_rx_del_item_to_list(id);
        return 0;
    }
    else
    {
        return 1;
    }
}

static int hal_trace_rx_reset(void)
{

    hal_trace_rx.pos = 0;
    memset(hal_trace_rx.buf, 0, HAL_TRACE_RX_BUF_SIZE);

    return 0;
}

// [test,12,102.99]
static int hal_trace_rx_parse(int8_t *buf, HAL_TRACE_RX_CFG_T *cfg)
{
    // TRACE("[%s] Start...", __func__);
    int pos = 0;
    int len = 0;

    for(; pos<HAL_TRACE_RX_HEAD_SIZE; pos++)
    {
        if(buf[pos] == HAL_TRACE_RX_HEAD)
        {
            buf[pos] = 0;
            break;
        }
    }

    if(pos == HAL_TRACE_RX_HEAD_SIZE)
    {
        return 3;
    }

    pos++;

    cfg->name = (char *)(buf+pos);
    for(; pos<HAL_TRACE_RX_NAME_SIZE+HAL_TRACE_RX_HEAD_SIZE; pos++)
    {
        if(buf[pos] == HAL_TRACE_RX_SEPARATOR)
        {
            buf[pos] = 0;
            break;
        }
    }

    // TRACE("Step1: %s", cfg->name);
    // TRACE("%d", strlen(cfg->name));

    if(pos == HAL_TRACE_RX_NAME_SIZE)
    {
        return 1;
    }

    pos++;

    len = 0;
    cfg->buf = (uint8_t*)(buf+pos);
    for(; pos<HAL_TRACE_RX_BUF_SIZE; pos++)
    {
        if(buf[pos] == HAL_TRACE_RX_END)
        {
            buf[pos] = 0;
            break;
        }
        len++;
    }
    cfg->len = len;
    if(pos == HAL_TRACE_RX_BUF_SIZE)
    {
        return 2;
    }

    return 0;
}

#if defined(IBRT)
void app_ibrt_peripheral_perform_test(const char* ibrt_cmd);
#endif

int hal_trace_rx_process(uint8_t *buf)
{
    HAL_TRACE_RX_CFG_T cfg;
    int id = 0;
    int res = 0;

#if defined(IBRT)
    if (buf && strlen((char*)buf) > 10 && strncmp((char*)buf, "ibrt_test:", 10) == 0)
    {
        app_ibrt_peripheral_perform_test((char*)(buf + 10));
        return 0;
    }
#endif

    res = hal_trace_rx_parse((int8_t*)buf, &cfg);

    if(res)
    {
        TRACE("ERROR: hal_trace_rx_parse %d", res);
        return 1;
    }
    else
    {
        // TRACE("%s rx OK", cfg.name);
    }

    id = hal_trace_rx_is_in_list(cfg.name);

    if(id == -1)
    {
        TRACE("%s is invalid", cfg.name);
        return -1;
    }

    if(hal_trace_rx.list[id].callback)
    {
        hal_trace_rx.list[id].callback(cfg.buf, cfg.len);
    }
    else
    {
        TRACE("%s has not callback", hal_trace_rx.list[id].name);
    }

    return 0;
}

void hal_trace_rx_start(void)
{
    unsigned int desc_cnt = 1;
    union HAL_UART_IRQ_T mask;

    mask.reg = 0;
    mask.BE = 0;
    mask.FE = 0;
    mask.OE = 0;
    mask.PE = 0;
    mask.RT = 1;

    hal_uart_dma_recv_mask(trace_uart, hal_trace_rx.buf, HAL_TRACE_RX_BUF_SIZE, &dma_desc_rx, &desc_cnt, &mask);
}

void hal_trace_rx_irq_handler(uint32_t xfer_size, int dma_error, union HAL_UART_IRQ_T status)
{
    int res;
    // TRACE("[%s] %d, %d, %d", __func__, xfer_size, dma_error, status);

    if (xfer_size)
    {
        hal_trace_rx.buf[xfer_size] = 0;
 #if defined(_AUTO_TEST_)
        res = auto_test_prase(hal_trace_rx.buf);
        if(res)
        {
            TRACE("%s:auto_test_prase prase data error, err_code = %d", __func__, res);
        }
 #else
        TRACE("[%s] RX = %s", __func__, hal_trace_rx.buf);
        res = hal_trace_rx_process(hal_trace_rx.buf);
        if(res)
        {
            TRACE("%s:hal_trace_rx_process prase data error, err_code = %d",__func__, res);
        }
 #endif
        hal_trace_rx_reset();
        hal_trace_rx_start();
    }
}

uint32_t app_test_callback(unsigned char *buf, uint32_t len)
{
    TRACE("[%s] len = %d", __func__, len);

    // Process string
    int num_int = 0;
    int num_float = 0.0;
    TRACE("[%s] %s", __func__, buf);
    hal_trace_rx_parser((char*)buf, "%d,%d", &num_int, &num_float);

    TRACE("[%s] %d:%d", __func__, num_int, num_float);

    return 0;
}

int hal_trace_rx_open()
{
    hal_uart_irq_set_dma_handler(trace_uart, hal_trace_rx_irq_handler, NULL, NULL);
    hal_trace_rx_start();

    hal_trace_rx_register("test", (HAL_TRACE_RX_CALLBACK_T)app_test_callback);

    return 0;
}

#endif // HAL_TRACE_RX_ENABLE

