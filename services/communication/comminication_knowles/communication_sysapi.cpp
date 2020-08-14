#include "cmsis_os.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "cmsis_nvic.h"
#include "tool_msg.h"
//#include "hal_timer_raw.h"
#include "hal_uart.h"
#include "hal_cmu.h"
#include "hal_bootmode.h"
#include "hal_iomux.h"
#include "pmu.h"
#include "app_thread.h"
#include "communication_sysapi.h"


//#define FLASH_BASE                              0x6C000000
//#define FLASH_TO_FLASHX(d)                      ((uint32_t)(d) & 0x0FFFFFFF)

#define WAIT_TRACE_TIMEOUT                      MS_TO_TICKS(200)

static uint32_t send_timeout = MS_TO_TICKS(500);

static const struct HAL_UART_CFG_T uart_cfg = {
    HAL_UART_PARITY_NONE,
#ifdef __KNOWLES
	HAL_UART_STOP_BITS_2,
#else
    HAL_UART_STOP_BITS_1,
#endif
    HAL_UART_DATA_BITS_8,
    HAL_UART_FLOW_CONTROL_NONE, // RTC/CTS pins might be unavailable for some chip packages
    HAL_UART_FIFO_LEVEL_1_2,
    HAL_UART_FIFO_LEVEL_1_2,
#ifdef __PC_CMD_UART__ 
        115200,
#else
        921600,
#endif
    true,
    false,
    false,
};
#ifdef __PC_CMD_UART__
static const enum HAL_UART_ID_T comm_uart = HAL_UART_ID_0;
#else
static const enum HAL_UART_ID_T comm_uart = HAL_UART_ID_1;
#endif
static volatile enum UART_DMA_STATE uart_dma_rx_state = UART_DMA_IDLE;
static volatile uint32_t uart_dma_rx_size = 0;
static bool uart_opened = false;


static volatile bool cancel_xfer = false;

static uint32_t xfer_err_time = 0;
static uint32_t xfer_err_cnt = 0;

#ifdef __KNOWLES

#include "knowles_mic_demo.h"
#include "audio_dump.h"
#include "cqueue.h"

uint32_t kw_start_index = 0;
uint32_t kw_end_index = 0;
bool is_markers_read_done = false;


#ifdef KNOWLES_UART_DATA

uint8_t out_buf[ FRAME_SIZE ];
uint8_t found = 0;

static osMutexId uart_audio_pcmbuff_mutex_id = NULL;
osMutexDef(uart_audio_pcmbuff_mutex);

typedef enum {
    PP_PING = 0,
    PP_PANG = 1
} FIR_PP_T;


#define PP_PINGPANG(v) \
    (v == PP_PING ? PP_PANG: PP_PING)

static FIR_PP_T pp_index=PP_PING;
static uint32_t uart_dma_interval;

#define knowles_uart_packet_len FRAME_SIZE //640//20MS
static uint8_t audio_data_receive_buf[knowles_uart_packet_len*2]; //ping_pong_buffer


#ifdef SYSTEM_USE_PSRAM
#include "hal_location.h"
#define AMA_STREAM_UART_FIFO_SIZE (1024*32)
PSRAM_BSS_LOC  static unsigned char ama_uart_buff[AMA_STREAM_UART_FIFO_SIZE];
#else
#define AMA_STREAM_UART_FIFO_SIZE (1024*16)
static unsigned char ama_uart_buff[AMA_STREAM_UART_FIFO_SIZE];
#endif
static CQueue ama_uart_queue;

extern osThreadId knowles_uart_audio_tid;

#define APP_UART_FLOW_CONTROL_TIMEOUT_INTERVEL           (10)

static void app_uart_flow_control_timeout_timer_cb(void const *n);
osTimerDef (APP_UART_FLOW_CONTROL_TIMEOUT, app_uart_flow_control_timeout_timer_cb); 
osTimerId	app_uart_flow_control_timeout_timer_id = NULL;



static void app_uart_flow_control_timeout_timer_cb(void const *n)
{
    uint32_t available_buffer;
    const unsigned char get_next_chunk_cmd[4] = {0x90, 0x47, 0x02, 0x00};

    available_buffer = avil_len_of_the_fifo();
    available_buffer = AMA_STREAM_UART_FIFO_SIZE - available_buffer;
    
    TRACE("UART_FLOW_CONTROL: Available UART Buffer %d\n", available_buffer);
    if(available_buffer > 1024)
    {
        //TRACE("More than 1024 bytes availble hence sending next chunk command \n");
        send_data(get_next_chunk_cmd,4);
        osTimerStop(app_uart_flow_control_timeout_timer_id);
        osTimerStart(app_uart_flow_control_timeout_timer_id, 10);   
    }
    else
    {
        //TRACE("no buffer available hence triggering timer for 10ms\n");
        osTimerStop(app_uart_flow_control_timeout_timer_id);
        osTimerStart(app_uart_flow_control_timeout_timer_id, APP_UART_FLOW_CONTROL_TIMEOUT_INTERVEL);
    }
}

void ama_uart_stream_fifo_init()
{
    if (uart_audio_pcmbuff_mutex_id == NULL)
        uart_audio_pcmbuff_mutex_id = osMutexCreate((osMutex(uart_audio_pcmbuff_mutex)));

    pp_index=PP_PING;

    osSignalClear(knowles_uart_audio_tid, UART_OUT_SIGNAL_ID);

    memset(audio_data_receive_buf,0,sizeof(audio_data_receive_buf));
    InitCQueue(&ama_uart_queue, AMA_STREAM_UART_FIFO_SIZE, 
                (CQItemType *)ama_uart_buff);
}

uint32_t ama_uart_get_fifo_data(uint8_t *buf)
{
    uint32_t avail = 0;

    avail = LengthOfCQueue(&ama_uart_queue);
    if (avail > 0) {
        if (avail < knowles_uart_packet_len) {
            TRACE("%s: Invalid  UART STREAM SIZE: %d", __FUNCTION__, avail);
            return 0;
        } else {
            //TRACE("UART_GET_len %d",knowles_uart_packet_len);
            if(!DeCQueue(&ama_uart_queue, buf, knowles_uart_packet_len))
                return knowles_uart_packet_len;
            TRACE("%s: queue get data error len %d", __func__, knowles_uart_packet_len);
            return 0;
        }
    } else {
        TRACE("uart no stream to get");
        return 0;
    }
}

uint32_t avil_len_of_the_fifo()
{
	return LengthOfCQueue(&ama_uart_queue);
}

extern "C" void OS_NotifyEvm(void);

void send_message()
{
	OS_NotifyEvm();
}
#endif
#endif

extern uint8_t is_smart_voice_stream_run(void);
static void uart_rx_dma_handler(uint32_t xfer_size, int dma_error, union HAL_UART_IRQ_T status)
{
    // The DMA transfer has been cancelled
#ifndef KNOWLES_UART_DATA    
    if (uart_dma_rx_state != UART_DMA_START) {
        return;
    }
#endif
    uart_dma_rx_size = xfer_size;
    if (dma_error || status.FE || status.OE || status.PE || status.BE) {
        TRACE("UART-RX Error: dma_error=%d, status=0x%08x", dma_error, status.reg);
        uart_dma_rx_state = UART_DMA_ERROR;
    } else {
        //TRACE("UART-RX OK: rx_size=%d", uart_dma_rx_size);
        uart_dma_rx_state = UART_DMA_DONE;      
        int ret = 0;
        //TRACE("UART DMA INTERUPT TIME=%d",hal_sys_timer_get()-uart_dma_interval);
        uart_dma_interval=hal_sys_timer_get();
         if(is_smart_voice_stream_run())
         {
            if(!EnCQueue(&ama_uart_queue, (uint8_t *)&audio_data_receive_buf[pp_index*knowles_uart_packet_len], FRAME_SIZE))
                ret = FRAME_SIZE;
            if (ret < FRAME_SIZE) {
                TRACE("%s:WARNING !!! UART STREAM OVERFLOW Dropping %d Bytes", __FUNCTION__, (FRAME_SIZE - ret));
            }

            osSignalSet(knowles_uart_audio_tid, UART_OUT_SIGNAL_ID);
         }
        pp_index=PP_PINGPANG(pp_index); 
    }
}

static void uart_break_handler(void)
{
    TRACE("****** Handle break ******");

    cancel_xfer = true;
    hal_uart_stop_dma_recv(comm_uart);
    hal_uart_stop_dma_send(comm_uart);
    uart_dma_rx_state = UART_DMA_ERROR;
}

void recv_data_state_get(enum UART_DMA_STATE *state)
{
    *state = uart_dma_rx_state;
}

void recv_data_reset(void)
{
    union HAL_UART_IRQ_T mask;
    mask.reg = 0;
    mask.BE = 1;	
    hal_uart_irq_set_mask(comm_uart, mask);	
    uart_dma_rx_state = UART_DMA_IDLE;
}

void init_transport(void)
{
    union HAL_UART_IRQ_T mask;
    struct HAL_UART_CFG_T comm_uart_cfg;
#ifdef KNOWLES_UART_DATA
	//audio_dump_init(320, 1);
	ama_uart_stream_fifo_init();
#endif
    if (!uart_opened) {
#ifdef __PC_CMD_UART__		
		hal_iomux_set_uart0();
#else
		hal_iomux_set_uart1();
#endif
        memcpy(&comm_uart_cfg, &uart_cfg, sizeof(comm_uart_cfg));	
        hal_uart_open(comm_uart, &comm_uart_cfg);
        mask.reg = 0;
        mask.BE = 1;
        mask.FE = 1;
        mask.OE = 1;
        mask.PE = 1;

        hal_uart_irq_set_dma_handler(comm_uart, uart_rx_dma_handler, NULL, uart_break_handler);
        hal_uart_irq_set_mask(comm_uart, mask);
        uart_opened = true;
    }

    cancel_xfer = false;

    hal_uart_flush(comm_uart,0);
#ifdef KNOWLES_UART_DATA
    if(recv_data(&audio_data_receive_buf[0], knowles_uart_packet_len*2) < 0)
        TRACE("%s error", __func__);
    smart_mic_stream_header_parser_init(FRAME_SIZE);
#endif

    app_uart_flow_control_timeout_timer_id = osTimerCreate(osTimer(APP_UART_FLOW_CONTROL_TIMEOUT), osTimerOnce, NULL); 

}


void deinit_transport(void)
{
    union HAL_UART_IRQ_T mask;

    mask.reg = 0;

	//audio_dump_clear_up(); //added by punith
	
    hal_uart_stop_dma_recv(comm_uart);//punith
    uart_dma_rx_state = UART_DMA_IDLE;
    hal_uart_irq_set_mask(comm_uart, mask);
    hal_uart_irq_set_dma_handler(comm_uart, NULL, NULL, NULL);
    hal_uart_close(comm_uart);


	hal_uart_flush(comm_uart,0);
    uart_opened = false;
	cancel_xfer = true;

	osTimerDelete(app_uart_flow_control_timeout_timer_id);

}




void reinit_transport(void)
{
    union HAL_UART_IRQ_T mask;
    struct HAL_UART_CFG_T comm_uart_cfg;

    uart_opened = false;
    memcpy(&comm_uart_cfg, &uart_cfg, sizeof(comm_uart_cfg));
    hal_uart_open(comm_uart, &comm_uart_cfg);
    mask.reg = 0;
    mask.BE = 1;
    mask.FE = 1;
    mask.OE = 1;
    mask.PE = 1;        
    hal_uart_irq_set_dma_handler(comm_uart, uart_rx_dma_handler, NULL, uart_break_handler);
    hal_uart_irq_set_mask(comm_uart, mask);
    uart_opened = true;

    cancel_xfer = false;

    hal_uart_flush(comm_uart,0);
}

void set_send_timeout(uint32_t timeout)
{
    send_timeout = MS_TO_TICKS(timeout);
}

int debug_read_enabled(void)
{
    return !!(hal_sw_bootmode_get() & HAL_SW_BOOTMODE_READ_ENABLED);
}

int debug_write_enabled(void)
{
    return !!(hal_sw_bootmode_get() & HAL_SW_BOOTMODE_WRITE_ENABLED);
}

static int uart_send_data(const unsigned char *buf, unsigned int len)
{
    uint32_t start;
    uint32_t sent = 0;

    start = hal_sys_timer_get();
    while (sent < len) {
        while (!cancel_xfer && !hal_uart_writable(comm_uart) &&
                hal_sys_timer_get() - start < send_timeout);
        if (cancel_xfer) {
            break;
        }
        if (hal_uart_writable(comm_uart)) {
            hal_uart_putc(comm_uart, buf[sent++]);
        } else {
            break;
        }
    }

    if (sent != len) {
        return 1;
    }

    return 0;
}

int send_data(const unsigned char *buf, unsigned int len)
{
    if (cancel_xfer) {
        return -1;
    }

    return uart_send_data(buf, len);
}


static struct HAL_DMA_DESC_T dma_desc[17] = {0};

static int uart_recv_data_dma(unsigned char *buf, unsigned int len, unsigned int expect)
{
    int ret;
    union HAL_UART_IRQ_T mask;
    
    unsigned int desc_cnt = ARRAY_SIZE(dma_desc);

    if (uart_dma_rx_state != UART_DMA_IDLE) {
        ret = -3;
        goto _no_state_exit;
    }

    uart_dma_rx_state = UART_DMA_START;
    uart_dma_rx_size = 0;

#ifdef KNOWLES_UART_DATA
	ret = hal_uart_dma_recv_pingpang(comm_uart, buf, expect, &dma_desc[0], &desc_cnt);
#else
	ret = hal_uart_dma_recv(comm_uart, buf, expect, &dma_desc[0], &desc_cnt);
#endif
	
    if (ret) {
        uart_dma_rx_state = UART_DMA_ERROR;
        goto err_exit;
    }

    mask.reg = 0;
    mask.BE = 1;
    mask.FE = 1;
    mask.OE = 1;
    mask.PE = 1;
    //mask.RT = 1;
    hal_uart_irq_set_mask(comm_uart, mask);

_no_state_exit:
    TRACE("%s ret %d", __func__, ret);
    return ret;
err_exit:
    TRACE("%s err_exit ret %d", __func__, ret);
    return -1;
}

int recv_data(unsigned char *buf, unsigned int len)
{
	TRACE("%s %d %d",__func__, cancel_xfer, len);
    if (cancel_xfer) {
        return -1;
    }
    return uart_recv_data_dma(buf, len, len);
}

int recv_data_dma(unsigned char *buf, unsigned int len, unsigned int expect)
{
    if (cancel_xfer) {
        return -1;
    }
    
    return uart_recv_data_dma(buf, len, expect);
}

static int uart_handle_error(void)
{
    TRACE("****** Send break ******");

    // Send break signal, to tell the peer to reset the connection
    hal_uart_break_set(comm_uart);
    osDelay(100);
    hal_uart_break_clear(comm_uart);

    return 0;
}

int handle_error(void)
{
    int ret = 0;
    uint32_t err_time;

    osDelay(200);

    if (!cancel_xfer) {
        ret = uart_handle_error();
    }

    xfer_err_cnt++;
    err_time = hal_sys_timer_get();
    if (err_time - xfer_err_time > MS_TO_TICKS(5000)) {
        xfer_err_cnt = 0;
    }
    xfer_err_time = err_time;
    if (xfer_err_cnt < 2) {
        osDelay(500);
    } else if (xfer_err_cnt < 5) {
        osDelay(1000);
    } else {
        osDelay(2000);
    }

    return ret;
}

static int uart_cancel_input(void)
{
    hal_uart_flush(comm_uart,0);
    return 0;
}

int cancel_input(void)
{
    return uart_cancel_input();
}

void system_reboot(void)
{
    hal_cmu_sys_reboot();
}

void system_shutdown(void)
{
    pmu_shutdown();
}

void system_set_bootmode(unsigned int bootmode)
{
    bootmode &= ~(HAL_SW_BOOTMODE_READ_ENABLED | HAL_SW_BOOTMODE_WRITE_ENABLED);
    hal_sw_bootmode_set(bootmode);
}

void system_clear_bootmode(unsigned int bootmode)
{
    bootmode &= ~(HAL_SW_BOOTMODE_READ_ENABLED | HAL_SW_BOOTMODE_WRITE_ENABLED);
    hal_sw_bootmode_clear(bootmode);
}

unsigned int system_get_bootmode(void)
{
    return hal_sw_bootmode_get();
}

void wait_trace_finished(void)
{
    uint32_t time;
    int idle_cnt = 0;

    time = hal_sys_timer_get();

    while (idle_cnt < 2 && hal_sys_timer_get() - time < WAIT_TRACE_TIMEOUT) {
        osDelay(10);
        idle_cnt = hal_trace_busy() ? 0 : (idle_cnt + 1);
    }
}

unsigned int get_current_time(void)
{
    return hal_sys_timer_get();
}

