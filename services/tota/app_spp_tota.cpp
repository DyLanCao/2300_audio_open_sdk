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
#include <stdio.h>
#include "cmsis_os.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_chipid.h"
#include "analog.h"
#include "app_audio.h"
#include "app_status_ind.h"
#include "app_bt_stream.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
//#include "nvrecord_dev.h"

#include "bluetooth.h"
#include "cqueue.h"
#include "resources.h"
#include "app_spp_tota.h"
#include "app_tota_cmd_code.h"
#include "app_tota.h"
#include "app_spp.h"
#include "app_tota_cmd_handler.h"
#include "app_tota_data_handler.h"
//#include "anc_prase_data.h"
#include "plat_types.h"
#include "spp.h"
#include "sdp_api.h"
//#include "app_bt_conn_mgr.h"

#define SPP_SHARE_BUF            0

static void tota_spp_read_thread(const void *arg);
osThreadDef(tota_spp_read_thread, osPriorityAboveNormal, 1, 3072, "tota_spp_read");

static bool isTotaSppConnected = false;
static struct spp_device  tota_spp_dev;  
static struct spp_service  totaSppService;
osMutexDef(tota_spp_mutex);

static sdp_record_t tota_sdp_record;

osThreadId  tota_spp_read_thread_id = NULL;

static app_spp_tota_tx_done_t app_spp_tota_tx_done_func = NULL;

#if (SPP_SHARE_BUF==1)
extern uint8_t tws_big_buff[PCM_BUFFER_SZ_TWS+TRANS_BUFFER_SZ_TWS+A2DP_BUFFER_SZ_TWS];
static uint8_t* totaSppTxBuf = &tws_big_buff[ANC_PRASE_DATA_BUFF_OFFSET-ANC_PRASE_DATA_BUFF_SIZE - TOTA_SPP_TX_BUF_SIZE];
#else
static uint8_t totaSppTxBuf[TOTA_SPP_TX_BUF_SIZE];
#endif

static uint32_t occupiedTotaSppTxBufSize;
static uint32_t offsetToFillTotaSppTxData;
static uint8_t* ptrTotaSppTxBuf;

uint16_t app_spp_tota_tx_buf_size(void)
{
    return TOTA_SPP_TX_BUF_SIZE;
}

void app_spp_tota_init_tx_buf(uint8_t* ptr)
{
    ptrTotaSppTxBuf = ptr;
    occupiedTotaSppTxBufSize = 0;
    offsetToFillTotaSppTxData = 0;
}

static void app_spp_tota_free_tx_buf(uint8_t* ptrData, uint32_t dataLen)
{
    if (occupiedTotaSppTxBufSize > 0)
    {
        occupiedTotaSppTxBufSize -= dataLen;
    }
    TOTA_LOG_DBG("occupiedTotaSppTxBufSize %d", occupiedTotaSppTxBufSize);
}

#if 1
uint8_t* app_spp_tota_fill_data_into_tx_buf(uint8_t* ptrData, uint32_t dataLen)
{
    ASSERT((occupiedTotaSppTxBufSize + dataLen) < TOTA_SPP_TX_BUF_SIZE, 
        "Pending SPP tx data has exceeded the tx buffer size !");

        
    if ((offsetToFillTotaSppTxData + dataLen) > TOTA_SPP_TX_BUF_SIZE)
    {
        offsetToFillTotaSppTxData = 0;
    }

    uint8_t* filledPtr = ptrTotaSppTxBuf + offsetToFillTotaSppTxData;
    memcpy(filledPtr, ptrData, dataLen);

    offsetToFillTotaSppTxData += dataLen;

    occupiedTotaSppTxBufSize += dataLen;

    TOTA_LOG_DBG("dataLen %d offsetToFillTotaSppTxData %d occupiedTotaSppTxBufSize %d",
        dataLen, offsetToFillTotaSppTxData, occupiedTotaSppTxBufSize);
    
    return filledPtr;
}
#else
uint8_t* app_spp_tota_fill_data_into_tx_buf(uint8_t* ptrData, uint32_t dataLen)
{
    TOTA_LOG_DBG("dataLen %d offsetToFillTotaSppTxData %d occupiedTotaSppTxBufSize %d",
        dataLen, offsetToFillTotaSppTxData, occupiedTotaSppTxBufSize);

    ASSERT((occupiedTotaSppTxBufSize + dataLen) < TOTA_SPP_TX_BUF_SIZE, 
        "Pending SPP tx data has exceeded the tx buffer size !");

    uint8_t* filledPtr = ptrTotaSppTxBuf + offsetToFillTotaSppTxData;
    memcpy(filledPtr, ptrData, dataLen);
        
    if ((offsetToFillTotaSppTxData + dataLen) > TOTA_SPP_TX_BUF_SIZE)
    {        
        offsetToFillTotaSppTxData = 0;
    }
    else
    {
        offsetToFillTotaSppTxData += dataLen;
    }

    occupiedTotaSppTxBufSize += dataLen;

    TOTA_LOG_DBG("dataLen %d offsetToFillTotaSppTxData %d occupiedTotaSppTxBufSize %d",
        dataLen, offsetToFillTotaSppTxData, occupiedTotaSppTxBufSize);
    
    return filledPtr;
}
#endif

extern "C" APP_TOTA_CMD_RET_STATUS_E app_tota_data_received(uint8_t* ptrData, uint32_t dataLength);
extern "C" APP_TOTA_CMD_RET_STATUS_E app_tota_cmd_received(uint8_t* ptrData, uint32_t dataLength);


/****************************************************************************
 * TOTA SPP SDP Entries
 ****************************************************************************/

/*---------------------------------------------------------------------------
 *
 * ServiceClassIDList 
 */
static const U8 TotaSppClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(3),        /* Data Element Sequence, 6 bytes */ 
    SDP_UUID_16BIT(SC_SERIAL_PORT),     /* Hands-Free UUID in Big Endian */ 
};

static const U8 TotaSppProtoDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(12),  /* Data element sequence, 12 bytes */ 

    /* Each element of the list is a Protocol descriptor which is a 
     * data element sequence. The first element is L2CAP which only 
     * has a UUID element.  
     */ 
    SDP_ATTRIB_HEADER_8BIT(3),   /* Data element sequence for L2CAP, 3 
                                  * bytes 
                                  */ 

    SDP_UUID_16BIT(PROT_L2CAP),  /* Uuid16 L2CAP */ 

    /* Next protocol descriptor in the list is RFCOMM. It contains two 
     * elements which are the UUID and the channel. Ultimately this 
     * channel will need to filled in with value returned by RFCOMM.  
     */ 

    /* Data element sequence for RFCOMM, 5 bytes */
    SDP_ATTRIB_HEADER_8BIT(5),

    SDP_UUID_16BIT(PROT_RFCOMM), /* Uuid16 RFCOMM */

    /* Uint8 RFCOMM channel number - value can vary */
    SDP_UINT_8BIT(RFCOMM_CHANNEL_TOTA)
};

/*
 * BluetoothProfileDescriptorList
 */
static const U8 TotaSppProfileDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(8),        /* Data element sequence, 8 bytes */

    /* Data element sequence for ProfileDescriptor, 6 bytes */
    SDP_ATTRIB_HEADER_8BIT(6),

    SDP_UUID_16BIT(SC_SERIAL_PORT),   /* Uuid16 SPP */
    SDP_UINT_16BIT(0x0102)            /* As per errata 2239 */ 
};

/*
 * * OPTIONAL *  ServiceName
 */
static const U8 TotaSppServiceName1[] = {
    SDP_TEXT_8BIT(5),          /* Null terminated text string */ 
    'S', 'p', 'p', '1', '\0'
};

static const U8 TotaSppServiceName2[] = {
    SDP_TEXT_8BIT(5),          /* Null terminated text string */ 
    'S', 'p', 'p', '2', '\0'
};

/* SPP attributes.
 *
 * This is a ROM template for the RAM structure used to register the
 * SPP SDP record.
 */
//static const SdpAttribute TotaSppSdpAttributes1[] = {
static sdp_attribute_t TotaSppSdpAttributes1[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, TotaSppClassId), 

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, TotaSppProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, TotaSppProfileDescList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), TotaSppServiceName1),
};

/*
static sdp_attribute_t TotaSppSdpAttributes2[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, TotaSppClassId), 

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, TotaSppProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, TotaSppProfileDescList),

    
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), TotaSppServiceName2),
};
*/

extern "C" void reset_programmer_state(unsigned char **buf, size_t *len);
extern unsigned char *g_buf;
extern size_t g_len;

static void tota_spp_read_thread(const void *arg)
{
	uint8_t buffer[TOTA_SPP_MAX_PACKET_SIZE];
	U16 maxBytes;
#if defined(APP_ANC_TEST)
    reset_programmer_state(&g_buf, &g_len);
#endif
    
    while (1)
	{
		maxBytes = TOTA_SPP_MAX_PACKET_SIZE;
		
		spp_read(&tota_spp_dev, (char *)buffer, &maxBytes);
        TOTA_LOG_DBG("[%s]data receive length = %d",__func__,maxBytes);
        TOTA_LOG_DUMP("[0x%x]",buffer,maxBytes);
#if defined(APP_ANC_TEST)
        app_anc_tota_cmd_received(buffer, (uint32_t)maxBytes);
#else
        // the first two bytes of the data packet is the fixed value 0xFFFF
        app_tota_handle_received_data(buffer, maxBytes);
#endif
    }
}

static void app_spp_tota_create_read_thread(void)
{
    TOTA_LOG_DBG("%s %d\n", __func__, __LINE__);
    tota_spp_read_thread_id = osThreadCreate(osThread(tota_spp_read_thread), NULL);
}

static void app_spp_tota_close_read_thread(void)
{
    TOTA_LOG_DBG("%s %d\n", __func__, __LINE__);
    if(tota_spp_read_thread_id)
    {
        osThreadTerminate(tota_spp_read_thread_id);
        tota_spp_read_thread_id = NULL;
    }
}

#if defined(APP_ANC_TEST)
static void app_synccmd_timehandler(void const *param);
osTimerDef (APP_SYNCCMD, app_synccmd_timehandler);
osTimerId app_check_send_synccmd_timer = NULL;
extern "C" void send_sync_cmd_to_tool();
static void app_synccmd_timehandler(void const *param)
{
    send_sync_cmd_to_tool();
}
#endif

static void spp_tota_callback(struct spp_device *locDev, struct spp_callback_parms *Info)
{
	if (SPP_EVENT_REMDEV_CONNECTED == Info->event)
	{
		TOTA_LOG_DBG("::SPP_EVENT_REMDEV_CONNECTED %d\n", Info->event);
        isTotaSppConnected = true;
        app_spp_tota_create_read_thread();
        app_tota_connected(APP_TOTA_CONNECTED);
        app_tota_update_datapath(APP_TOTA_VIA_SPP);
        //conn_stop_connecting_mobile_supervising();
#if defined(APP_ANC_TEST)
        //add a send sync timer
        osTimerStop(app_check_send_synccmd_timer);
        osTimerStart(app_check_send_synccmd_timer, 2000);
#endif
	}
	else if (SPP_EVENT_REMDEV_DISCONNECTED == Info->event)
	{
		TOTA_LOG_DBG("::SPP_EVENT_REMDEV_DISCONNECTED %d\n", Info->event);
        isTotaSppConnected = false;
        app_spp_tota_close_read_thread();
        app_tota_disconnected(APP_TOTA_DISCONNECTED);
        app_tota_update_datapath(APP_TOTA_PATH_IDLE);

#if defined(APP_ANC_TEST)
        osTimerStop(app_check_send_synccmd_timer);
#endif
        app_spp_tota_tx_done_func = NULL;
	}
	else if (SPP_EVENT_DATA_SENT == Info->event)
    {
        //app_spp_tota_free_tx_buf(Info->tx_buf, Info->tx_data_len);
        struct spp_tx_done *pTxDone = (struct spp_tx_done *)(Info->p.other);
        app_spp_tota_free_tx_buf(pTxDone->tx_buf, pTxDone->tx_data_length);
        if (app_spp_tota_tx_done_func)
        {
            app_spp_tota_tx_done_func();
        }		
    }
    else
    {
        TOTA_LOG_DBG("::unknown event %d\n", Info->event);
    }
}

static void app_spp_tota_send_data(uint8_t* ptrData, uint16_t length)
{
    if (!isTotaSppConnected)
    {
        return;
    }

    spp_write(&tota_spp_dev, (char *)ptrData, &length);
}

void app_tota_send_cmd_via_spp(uint8_t* ptrData, uint32_t length)
{
    uint8_t* ptrBuf = app_spp_tota_fill_data_into_tx_buf(ptrData, length);
    app_spp_tota_send_data(ptrBuf, (uint16_t)length);
}

void app_tota_send_data_via_spp(uint8_t* ptrData, uint32_t length)
{
    TOTA_LOG_DBG("[%s]tota send data length = %d",__func__,length);
    uint8_t* ptrBuf = app_spp_tota_fill_data_into_tx_buf(ptrData, length);
    app_spp_tota_send_data(ptrBuf, (uint16_t)length);
}

void app_spp_tota_register_tx_done(app_spp_tota_tx_done_t callback)
{
	app_spp_tota_tx_done_func = callback;
}

#if (SPP_SHARE_BUF==0)
static uint8_t spp_rx_buf[SPP_RECV_BUFFER_SIZE];
#endif

void app_spp_tota_init(void)
{
    uint8_t *rx_buf;
    osMutexId mid;

#if (SPP_SHARE_BUF==1)
	rx_buf = &tws_big_buff[ANC_PRASE_DATA_BUFF_OFFSET - ANC_PRASE_DATA_BUFF_SIZE - TOTA_SPP_TX_BUF_SIZE - SPP_RECV_BUFFER_SIZE];
#else
    rx_buf = tota_spp_dev.rx_buffer = &spp_rx_buf[0];
#endif

	TOTA_LOG_DBG("totaSppTxBuf: 0x%08x", totaSppTxBuf);
    app_spp_tota_init_tx_buf(totaSppTxBuf);
    spp_init_rx_buf(&tota_spp_dev, rx_buf, SPP_RECV_BUFFER_SIZE);

    mid = osMutexCreate(osMutex(tota_spp_mutex));
    if (!mid) {
        ASSERT(0, "cannot create mutex");
    }

    tota_sdp_record.attribs = &TotaSppSdpAttributes1[0],
    tota_sdp_record.num = ARRAY_SIZE(TotaSppSdpAttributes1);
    tota_sdp_record.classOfDevice = BTIF_COD_MAJOR_PERIPHERAL;

	totaSppService.rf_service.serviceId = RFCOMM_CHANNEL_TOTA;
	totaSppService.numPorts = 0;
    spp_service_setup(&tota_spp_dev, &totaSppService, &tota_sdp_record);

	tota_spp_dev.portType = SPP_SERVER_PORT;
    spp_init_device(&tota_spp_dev, 5, mid);

    spp_open(&tota_spp_dev, NULL,  spp_tota_callback);
#if defined(APP_ANC_TEST)
    if (app_check_send_synccmd_timer == NULL)
        app_check_send_synccmd_timer = osTimerCreate (osTimer(APP_SYNCCMD), osTimerPeriodic, NULL);
#endif
}

