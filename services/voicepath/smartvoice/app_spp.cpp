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
#include "nvrecord_dev.h"

#include "cqueue.h"
#include "resources.h"
#include "app_spp.h"
#include "spp.h"
#include "sdp_api.h"
#include "app_spp.h"
#include "plat_types.h"

#define SPP_ID					0
#define SPP_MAX_PACKET_SIZE     L2CAP_MTU
#define SPP_MAX_PACKET_NUM      5

#define SPP_MAX_DATA_PACKET_SIZE    700

static bool isSppConnected = false;
static struct spp_device   spp_dev;

osMutexDef(sv_spp_mutex);

#if SPP_SERVER == XA_ENABLED
static struct spp_service  sppService;
static sdp_record_t   sppSdpRecord;

static app_spp_tx_done_t app_spp_tx_done_func = NULL;

#define SPP_TX_BUF_SIZE     4096
// in case (offsetToFillSppTxData + dataLen) > SPP_TX_BUF_SIZE
#define SPP_TX_BUF_EXTRA_SIZE   SPP_MAX_DATA_PACKET_SIZE 

static uint8_t sppTxBuf[SPP_TX_BUF_SIZE + SPP_TX_BUF_EXTRA_SIZE];
static uint32_t occupiedSppTxBufSize;
static uint32_t offsetToFillSppTxData;
static uint8_t* ptrSppTxBuf;

uint16_t app_spp_tx_buf_size(void)
{
    return SPP_TX_BUF_SIZE;
}

void app_spp_init_tx_buf(uint8_t* ptr)
{
    ptrSppTxBuf = ptr;
    occupiedSppTxBufSize = 0;
    offsetToFillSppTxData = 0;
}

static void app_spp_free_tx_buf(uint8_t* ptrData, uint32_t dataLen)
{
    if (occupiedSppTxBufSize > 0)
    {
        occupiedSppTxBufSize -= dataLen;
    }
    TRACE("occupiedSppTxBufSize %d", occupiedSppTxBufSize);
}

uint8_t* app_spp_fill_data_into_tx_buf(uint8_t* ptrData, uint32_t dataLen)
{
    ASSERT((occupiedSppTxBufSize + dataLen) < SPP_TX_BUF_SIZE, 
        "Pending SPP tx data has exceeded the tx buffer size !");

    uint8_t* filledPtr = ptrSppTxBuf + offsetToFillSppTxData;
    memcpy(filledPtr, ptrData, dataLen);
        
    if ((offsetToFillSppTxData + dataLen) > SPP_TX_BUF_SIZE)
    {        
        offsetToFillSppTxData = 0;
    }
    else
    {
        offsetToFillSppTxData += dataLen;
    }

    occupiedSppTxBufSize += dataLen;

    //TRACE("dataLen %d offsetToFillSppTxData %d occupiedSppTxBufSize %d",
    //    dataLen, offsetToFillSppTxData, occupiedSppTxBufSize);
    
    return filledPtr;
}

extern "C" APP_SV_CMD_RET_STATUS_E app_sv_data_received(uint8_t* ptrData, uint32_t dataLength);
extern "C" APP_SV_CMD_RET_STATUS_E app_sv_cmd_received(uint8_t* ptrData, uint32_t dataLength);


/****************************************************************************
 * SPP SDP Entries
 ****************************************************************************/

/*---------------------------------------------------------------------------
 *
 * ServiceClassIDList 
 */
static const U8 SppClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(3),        /* Data Element Sequence, 6 bytes */ 
    SDP_UUID_16BIT(SC_SERIAL_PORT),     /* Hands-Free UUID in Big Endian */ 
};

static const U8 SppProtoDescList[] = {
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
    SDP_UINT_8BIT(RFCOMM_CHANNEL_SMARTVOICE)
};

/*
 * BluetoothProfileDescriptorList
 */
static const U8 SppProfileDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(8),        /* Data element sequence, 8 bytes */

    /* Data element sequence for ProfileDescriptor, 6 bytes */
    SDP_ATTRIB_HEADER_8BIT(6),

    SDP_UUID_16BIT(SC_SERIAL_PORT),   /* Uuid16 SPP */
    SDP_UINT_16BIT(0x0102)            /* As per errata 2239 */ 
};

/*
 * * OPTIONAL *  ServiceName
 */
static const U8 SppServiceName1[] = {
    SDP_TEXT_8BIT(5),          /* Null terminated text string */ 
    'S', 'p', 'p', '1', '\0'
};

static const U8 SppServiceName2[] = {
    SDP_TEXT_8BIT(5),          /* Null terminated text string */ 
    'S', 'p', 'p', '2', '\0'
};

/* SPP attributes.
 *
 * This is a ROM template for the RAM structure used to register the
 * SPP SDP record.
 */
static sdp_attribute_t sppSdpAttributes1[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, SppClassId), 

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, SppProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, SppProfileDescList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), SppServiceName1),
};

static sdp_attribute_t POSSIBLY_UNUSED sppSdpAttributes2[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, SppClassId), 

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, SppProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, SppProfileDescList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), SppServiceName2),
};
#endif

static void spp_callback(struct spp_device *locDev, struct spp_callback_parms *Info)
{
	if (SPP_EVENT_REMDEV_CONNECTED == Info->event)
	{
		TRACE("::SPP_EVENT_REMDEV_CONNECTED %d\n", Info->event);
        isSppConnected = true;
        app_spp_create_read_thread();
    #ifdef VOICE_DATAPATH
        app_voicepath_connected(APP_SMARTVOICE_SPP_CONNECTED);
    #endif
	}
	else if (SPP_EVENT_REMDEV_DISCONNECTED == Info->event)
	{
		TRACE("::SPP_EVENT_REMDEV_DISCONNECTED %d\n", Info->event);
        isSppConnected = false;
        app_spp_close_read_thread();
#ifdef VOICE_DATAPATH
        app_voicepath_disconnected(APP_SMARTVOICE_SPP_DISCONNECTED);
#endif 
	}
	else if (SPP_EVENT_DATA_SENT == Info->event)
	{
        struct spp_tx_done *pTxDone = (struct spp_tx_done *)(Info->p.other);
        app_spp_free_tx_buf(pTxDone->tx_buf, pTxDone->tx_data_length);
        if (app_spp_tx_done_func)
        {
            app_spp_tx_done_func();
        }		
	}
    else
    {
        TRACE("::unknown event %d\n", Info->event);
    }
}

static void app_spp_send_data(uint8_t* ptrData, uint16_t length)
{
    if (!isSppConnected)
    {
        return;
    }

    spp_write(&spp_dev, (char *)ptrData, &length);
}

void app_sv_send_cmd_via_spp(uint8_t* ptrData, uint32_t length)
{
    uint8_t* ptrBuf = app_spp_fill_data_into_tx_buf(ptrData, length);
    app_spp_send_data(ptrBuf, (uint16_t)length);
}

void app_sv_send_data_via_spp(uint8_t* ptrData, uint32_t length)
{
    uint8_t* ptrBuf = app_spp_fill_data_into_tx_buf(ptrData, length);
    app_spp_send_data(ptrBuf, (uint16_t)length);
}

void app_spp_register_tx_done(app_spp_tx_done_t callback)
{
	app_spp_tx_done_func = callback;
}

static uint8_t spp_rx_buf[SPP_RECV_BUFFER_SIZE];

void app_spp_init(void)
{
    uint8_t *rx_buf;
    osMutexId mid;

    app_spp_init_tx_buf(sppTxBuf);

	spp_dev.portType = SPP_SERVER_PORT;
    rx_buf = &spp_rx_buf[0];
    spp_init_rx_buf(&spp_dev, rx_buf, SPP_RECV_BUFFER_SIZE);

    mid = osMutexCreate(osMutex(sv_spp_mutex));
    if (!mid) {
        ASSERT(0, "cannot create mutex");
    }

#if SPP_SERVER == XA_ENABLED
	sppService.rf_service.serviceId = RFCOMM_CHANNEL_SMARTVOICE;
	sppService.numPorts = 0;
    sppSdpRecord.attribs = &sppSdpAttributes1[0],
    sppSdpRecord.num = ARRAY_SIZE(sppSdpAttributes1);
    sppSdpRecord.classOfDevice = BTIF_COD_MAJOR_PERIPHERAL;
    spp_service_setup(&spp_dev, &sppService, &sppSdpRecord);
#endif
    spp_init_device(&spp_dev, SPP_MAX_PACKET_NUM, mid);
    spp_open(&spp_dev, NULL, spp_callback); 
}

#ifdef UUID_128_ENABLE
void app_spp_extend_uuid_init(void)
{
    app_spp_init_tx_buf(sppTxBuf);

	spp_dev.portType = SPP_SERVER_PORT;
	spp_dev.osDev	  = (void *)&osSppDev;
	spp_dev.type.sppService = &sppService;
	spp_dev.type.sppService->sdpRecord = &sppSdpRecord;

	osSppDev.mutex = (void *)&spp_mutex;
	osSppDev.sppDev = &spp_dev;
    InitCQueue(&osSppDev.rx_queue, SPP_RECV_BUFFER_SIZE, osSppDev.sppRxBuffer);
	OS_MemSet(osSppDev.sppRxBuffer, 0, SPP_RECV_BUFFER_SIZE);

	SppRegister128UuidSdpServices(&spp_dev, 0);

    SPP_InitDevice(&spp_dev, &spp_txPacket[0], SPP_MAX_PACKET_NUM);
    SPP_Open(&spp_dev, NULL, spp_callback);
}
#endif
