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
#ifdef __INTERCONNECTION__

#include <stdlib.h>
#include "hal_trace.h"
#include "string.h"

#include "spp.h"
#include "app_bt.h"

#include "umm_malloc.h"
#include "app_interconnection.h"
#include "app_interconnection_logic_protocol.h"
#include "app_interconnection_spp.h"
#include "app_interconnection_ota.h"
#include "app_interconnection_customer_realize.h"


static struct spp_device sppClient;
static struct spp_device sppServer;
osMutexDef(client_spp_mutex);
osMutexDef(server_spp_mutex);

static struct spp_service sppService;
static sdp_record_t       sppSdpRecord;

static uint8_t sppClientRxBuf[SPP_RECV_BUFFER_SIZE];
static uint8_t sppServerRxBuf[SPP_RECV_BUFFER_SIZE];

static app_spp_tx_done_t app_spp_tx_done_func = NULL;

/*---------------------------------------------------------------------------
 *
 * ServiceClassIDList
 */
static const U8 sppClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(3),        /* Data Element Sequence, 6 bytes */
    SDP_UUID_16BIT(SC_SERIAL_PORT),     /* Hands-Free UUID in Big Endian */
};

static const U8 sppProtoDescList[] = {
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
    SDP_UINT_8BIT(RFCOMM_CHANNEL_INTERCONNECTION)
};

/*
 * BluetoothProfileDescriptorList
 */
static const U8 sppProfileDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(8),        /* Data element sequence, 8 bytes */

    /* Data element sequence for ProfileDescriptor, 6 bytes */
    SDP_ATTRIB_HEADER_8BIT(6),

    SDP_UUID_16BIT(SC_SERIAL_PORT),   /* Uuid16 SPP */
    SDP_UINT_16BIT(0x0102)            /* As per errata 2239 */
};

/*
 * * OPTIONAL *  ServiceName
 */
static const U8 sppServiceName[] = {
    SDP_TEXT_8BIT(16),          /* Null terminated text string */
    'I', 'n', 't', 'e', 'r', 'c', 'o', 'n', 'n', 'e', 'c', 't', 'i', 'o', 'n', '\0'
};
/* SPP attributes.
 *
 * This is a ROM template for the RAM structure used to register the
 * SPP SDP record.
 */
static sdp_attribute_t sppSdpAttributes[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, sppClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, sppProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, sppProfileDescList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), sppServiceName),
};

static const uint16_t crc16_tab[] = {
    0x0000,
    0xC0C1,
    0xC181,
    0x0140,
    0xC301,
    0x03C0,
    0x0280,
    0xC241,
    0xC601,
    0x06C0,
    0x0780,
    0xC741,
    0x0500,
    0xC5C1,
    0xC481,
    0x0440,
    0xCC01,
    0x0CC0,
    0x0D80,
    0xCD41,
    0x0F00,
    0xCFC1,
    0xCE81,
    0x0E40,
    0x0A00,
    0xCAC1,
    0xCB81,
    0x0B40,
    0xC901,
    0x09C0,
    0x0880,
    0xC841,
    0xD801,
    0x18C0,
    0x1980,
    0xD941,
    0x1B00,
    0xDBC1,
    0xDA81,
    0x1A40,
    0x1E00,
    0xDEC1,
    0xDF81,
    0x1F40,
    0xDD01,
    0x1DC0,
    0x1C80,
    0xDC41,
    0x1400,
    0xD4C1,
    0xD581,
    0x1540,
    0xD701,
    0x17C0,
    0x1680,
    0xD641,
    0xD201,
    0x12C0,
    0x1380,
    0xD341,
    0x1100,
    0xD1C1,
    0xD081,
    0x1040,
    0xF001,
    0x30C0,
    0x3180,
    0xF141,
    0x3300,
    0xF3C1,
    0xF281,
    0x3240,
    0x3600,
    0xF6C1,
    0xF781,
    0x3740,
    0xF501,
    0x35C0,
    0x3480,
    0xF441,
    0x3C00,
    0xFCC1,
    0xFD81,
    0x3D40,
    0xFF01,
    0x3FC0,
    0x3E80,
    0xFE41,
    0xFA01,
    0x3AC0,
    0x3B80,
    0xFB41,
    0x3900,
    0xF9C1,
    0xF881,
    0x3840,
    0x2800,
    0xE8C1,
    0xE981,
    0x2940,
    0xEB01,
    0x2BC0,
    0x2A80,
    0xEA41,
    0xEE01,
    0x2EC0,
    0x2F80,
    0xEF41,
    0x2D00,
    0xEDC1,
    0xEC81,
    0x2C40,
    0xE401,
    0x24C0,
    0x2580,
    0xE541,
    0x2700,
    0xE7C1,
    0xE681,
    0x2640,
    0x2200,
    0xE2C1,
    0xE381,
    0x2340,
    0xE101,
    0x21C0,
    0x2080,
    0xE041,
    0xA001,
    0x60C0,
    0x6180,
    0xA141,
    0x6300,
    0xA3C1,
    0xA281,
    0x6240,
    0x6600,
    0xA6C1,
    0xA781,
    0x6740,
    0xA501,
    0x65C0,
    0x6480,
    0xA441,
    0x6C00,
    0xACC1,
    0xAD81,
    0x6D40,
    0xAF01,
    0x6FC0,
    0x6E80,
    0xAE41,
    0xAA01,
    0x6AC0,
    0x6B80,
    0xAB41,
    0x6900,
    0xA9C1,
    0xA881,
    0x6840,
    0x7800,
    0xB8C1,
    0xB981,
    0x7940,
    0xBB01,
    0x7BC0,
    0x7A80,
    0xBA41,
    0xBE01,
    0x7EC0,
    0x7F80,
    0xBF41,
    0x7D00,
    0xBDC1,
    0xBC81,
    0x7C40,
    0xB401,
    0x74C0,
    0x7580,
    0xB541,
    0x7700,
    0xB7C1,
    0xB681,
    0x7640,
    0x7200,
    0xB2C1,
    0xB381,
    0x7340,
    0xB101,
    0x71C0,
    0x7080,
    0xB041,
    0x5000,
    0x90C1,
    0x9181,
    0x5140,
    0x9301,
    0x53C0,
    0x5280,
    0x9241,
    0x9601,
    0x56C0,
    0x5780,
    0x9741,
    0x5500,
    0x95C1,
    0x9481,
    0x5440,
    0x9C01,
    0x5CC0,
    0x5D80,
    0x9D41,
    0x5F00,
    0x9FC1,
    0x9E81,
    0x5E40,
    0x5A00,
    0x9AC1,
    0x9B81,
    0x5B40,
    0x9901,
    0x59C0,
    0x5880,
    0x9841,
    0x8801,
    0x48C0,
    0x4980,
    0x8941,
    0x4B00,
    0x8BC1,
    0x8A81,
    0x4A40,
    0x4E00,
    0x8EC1,
    0x8F81,
    0x4F40,
    0x8D01,
    0x4DC0,
    0x4C80,
    0x8C41,
    0x4400,
    0x84C1,
    0x8581,
    0x4540,
    0x8701,
    0x47C0,
    0x4680,
    0x8641,
    0x8201,
    0x42C0,
    0x4380,
    0x8341,
    0x4100,
    0x81C1,
    0x8081,
    0x4040,
};

uint16_t app_interconnection_spp_data_calculate_crc16(uint8_t *buf, uint32_t len)
{
    uint16_t cksum = 0x0000;

    for (uint32_t i = 0; i < len; i++)
    {
        cksum = crc16_tab[(cksum ^ *buf++) & 0xFF] ^ (cksum >> 8);
    }

    return cksum;
}

struct spp_device *app_interconnection_get_spp_client()
{
    return &sppClient;
}

struct spp_device *app_interconnection_get_spp_server(void)
{
    return &sppServer;
}

/**
 * @brief parse the EA structure data
 *
 * @param sourceDataPtr pointer of data to be parsed
 * @param eaDataPtr pointer of data value parsed from EA structure
 * @return uint8_t* pointer of source data after being parsed, return NULL if parse failed
 *                  need to check if the returned value is null
 */
uint8_t *app_interconnection_ea_parse_data(uint8_t *sourceDataPtr, uint32_t *eaDataPtr)
{
    uint8_t  eaIndex     = 0;
    uint8_t *pSourceData = sourceDataPtr;
    *eaDataPtr           = 0;

    pSourceData--;

    do
    {
        pSourceData++;
        if (eaIndex >= EA_MAX_LENGTH)
        {
            return NULL;
        }

        *eaDataPtr = (((uint32_t)(*pSourceData & EA_DATA_MASK)) << (eaIndex * 7)) | (*eaDataPtr);

        eaIndex++;

    } while ((*pSourceData & EA_HEAD_MASK) == EA_HEAD_WITH_EXTENSION);

    return (pSourceData + 1);
}

static void app_spp_init_tx_buf(uint8_t *ptr)
{
}


static void app_spp_free_tx_buf(uint8_t *ptrData, uint32_t dataLen)
{
    INTERCONNECTION_FREE(( void * )ptrData);
}


static uint8_t *app_spp_fill_data_into_tx_buf(uint8_t *ptrData, uint32_t dataLen)
{
    uint8_t *txDataPtr = NULL;

    txDataPtr = ( uint8_t * )INTERCONNECTION_MALLOC(dataLen);

    if (NULL == txDataPtr)
    {
        TRACE("failed to malloc for tx buffer.");
        return NULL;
    }

    memcpy(txDataPtr, ptrData, dataLen);

    return txDataPtr;
}

void app_interconnection_spp_client_connected_callback(uint8_t conidx, uint8_t connType)
{
    app_interconnection_set_spp_client_alive_flag(true);

    app_spp_register_tx_done(app_interconn_send_done);

    //when connect, send CCMP_Init Req command
    app_interconnection_spp_send_ccmp_init_command();
}

void app_interconnection_spp_server_connected_callback(uint8_t conidx, uint8_t connType)
{
    app_interconnection_set_spp_server_alive_flag(true);
    // TODO: spp server is not used for data transmission for now
}

void app_interconnection_spp_client_disconnection_callback(void)
{
    struct _spp_dev *dev = &sppClient.sppDev;

    app_interconnection_set_spp_client_alive_flag(false);
    app_interconnection_set_ccmp_denied_by_mobile_flag(false);
    app_spp_tx_done_func = NULL;

#if BTIF_SPP_CLIENT == BTIF_ENABLED
    spp_free(dev->type.client.sdpToken);
    dev->type.client.sdpToken = NULL;
#endif
    spp_free(dev->channel);
    dev->channel = NULL;
    spp_free(sppClient.tx_buffer);
    sppClient.tx_buffer = NULL;

    if (false == app_interconnection_get_ccmp_alive_flag() &&
        false == app_interconnection_get_spp_server_alive_flag() &&
        false == app_interconnection_get_spp_client_alive_flag())
    {
        app_interconnection_close_read_thread();
    }
}

void app_interconnection_spp_server_disconnection_callback(void)
{
    // TODO:
    struct _spp_dev *dev = &sppServer.sppDev;
    app_interconnection_set_spp_client_alive_flag(false);

    spp_free(dev->channel);
    dev->channel = NULL;
    spp_free(sppServer.tx_buffer);
    sppServer.tx_buffer = NULL;

    if (false == app_interconnection_get_ccmp_alive_flag() &&
        false == app_interconnection_get_spp_server_alive_flag() &&
        false == app_interconnection_get_spp_client_alive_flag())
    {
        app_interconnection_close_read_thread();
    }
}

static void spp_callback(struct spp_device *locDev, struct spp_callback_parms *Info)
{
    switch (Info->event)
    {
    case SPP_EVENT_REMDEV_CONNECTED:
        TRACE("::SPP_EVENT_REMDEV_CONNECTED %d\n", Info->event);
        if (locDev == app_interconnection_get_spp_client())
        {
            app_interconnection_spp_client_connected_callback(0, APP_INTERCONN_SPP_CONNECTED);
        }
        else if (locDev == app_interconnection_get_spp_server())
        {
            app_interconnection_spp_server_connected_callback(0, APP_INTERCONN_SPP_CONNECTED);
        }
        else
        {
            ASSERT(0, "illegal device please check!");
        }

        break;

    case SPP_EVENT_REMDEV_DISCONNECTED:
        TRACE("::SPP_EVENT_REMDEV_DISCONNECTED %d\n", Info->event);
        if (locDev == app_interconnection_get_spp_client())
        {
            app_interconnection_spp_client_disconnection_callback();
        }
        else if (locDev == app_interconnection_get_spp_server())
        {
            app_interconnection_spp_server_disconnection_callback();
        }
        else
        {
            ASSERT(0, "illegal device please check!");
        }

        break;

    case SPP_EVENT_DATA_SENT:
        if (locDev == app_interconnection_get_spp_client())
        {
            app_spp_free_tx_buf((( struct spp_tx_done * )(Info->p.other))->tx_buf,
                                (( struct spp_tx_done * )(Info->p.other))->tx_data_length);
            if (app_spp_tx_done_func)
            {
                app_spp_tx_done_func();
            }
        }
        break;

    default:
        TRACE("::unknown event %d\n", Info->event);
        break;
    }
}

static void lock_spp_device(struct spp_device *osDev)
{
    INTERCONNECTION_DEVICE_CHECK(osDev);

    osMutexWait(osDev->mutex_id, osWaitForever);
}

static void unlock_spp_device(struct spp_device *osDev)
{
    INTERCONNECTION_DEVICE_CHECK(osDev);

    osMutexRelease(osDev->mutex_id);
}

bt_status_t app_interconnection_spp_client_read_data(char *buffer, uint16_t *ptrLen)
{
    bt_status_t readDataStatus = BT_STS_FAILED;
    uint16_t    length         = *ptrLen;

    lock_spp_device(&sppClient);

    // read data from spp device
    readDataStatus = spp_read(&sppClient, ( char * )buffer, &length);
    *ptrLen        = length;

    unlock_spp_device(&sppClient);
    return readDataStatus;
}

bt_status_t app_interconnection_spp_server_read_data(char *buffer, uint16_t *ptrLen)
{
    bt_status_t readDataStatus = BT_STS_FAILED;
    uint16_t    length         = *ptrLen;

    lock_spp_device(&sppServer);

    // read data from spp device
    readDataStatus = spp_read(&sppServer, ( char * )buffer, &length);
    *ptrLen        = length;

    unlock_spp_device(&sppServer);
    return readDataStatus;
}

static void app_spp_send_data(struct spp_device *pSppDev, uint8_t *ptrData, uint16_t length)
{
    if ((pSppDev == &sppClient && !app_interconnection_get_spp_client_alive_flag()) ||
        (pSppDev == &sppServer && !app_interconnection_get_spp_server_alive_flag()))
    {
        TRACE("SPP not connected!");
        umm_free(ptrData);
        return;
    }

    spp_write(pSppDev, ( char * )ptrData, &length);
}


void app_interconnection_send_data_via_spp(struct spp_device *pSppDev, uint8_t *ptrData, uint32_t length)
{
    uint8_t *ptrBuf = app_spp_fill_data_into_tx_buf(ptrData, length);

    if (NULL == ptrBuf)
    {
        TRACE("error, null pointer for ccmp tx data.");
        return;
    }

    app_spp_send_data(pSppDev, ptrBuf, ( uint16_t )length);
}

void app_spp_register_tx_done(app_spp_tx_done_t callback)
{
    app_spp_tx_done_func = callback;
}

/**
 * @brief: initialize the spp client to establish a spp connection with server
 *
 * @param pServiceSearchRequest: service search list to be found from server
 * @param serviceSearchRequestLen: length of service search list
 */
void app_spp_client_open(uint8_t *pServiceSearchRequest, uint8_t serviceSearchRequestLen)
{
    TRACE("[%s]", __func__);
    bt_status_t status;
    osMutexId mid;

    app_spp_init_tx_buf(NULL);
    spp_init_rx_buf(&sppClient, sppClientRxBuf, SPP_RECV_BUFFER_SIZE);

    mid = osMutexCreate(osMutex(client_spp_mutex));
    if (!mid) {
        ASSERT(0, "cannot create mutex");
    }
    sppClient.portType = SPP_CLIENT_PORT;

    spp_init_device(&sppClient, SPP_MAX_PACKET_NUM, mid);

    sppClient.sppDev.type.client.rfcommServiceSearchRequestPtr = pServiceSearchRequest;
    sppClient.sppDev.type.client.rfcommServiceSearchRequestLen = serviceSearchRequestLen;
    sppClient.useMessageInformRxData = true;
    sppClient.spp_handle_data_event_func = interconnection_mail_put;
    sppClient.reader_thread_id = app_interconnection_get_read_thread();
    status  = spp_open(&sppClient, app_bt_get_connected_mobile_device_ptr(), spp_callback);
    TRACE("spp open status is %d", status);
}

void app_spp_server_open(void)
{
    TRACE("[%s]", __func__);
    if (app_interconnection_get_spp_server_alive_flag())
    {
        TRACE("interconnection server is already connected!");
    }
    else
    {
        bt_status_t status;
        osMutexId mid;

        app_spp_init_tx_buf(NULL);
        spp_init_rx_buf(&sppServer, sppServerRxBuf, SPP_RECV_BUFFER_SIZE);

        mid = osMutexCreate(osMutex(server_spp_mutex));
        if (!mid) {
            ASSERT(0, "cannot create mutex");
        }

        sppService.rf_service.serviceId = 0;
        sppService.numPorts             = 0;
        sppSdpRecord.attribs            = &sppSdpAttributes[0],
        sppSdpRecord.num                = ARRAY_SIZE(sppSdpAttributes);
        sppSdpRecord.classOfDevice      = BTIF_COD_MAJOR_PERIPHERAL;
        spp_service_setup(&sppServer, &sppService, &sppSdpRecord);

        sppServer.portType = SPP_SERVER_PORT;
        spp_init_device(&sppServer, SPP_MAX_PACKET_NUM, mid);

        sppServer.useMessageInformRxData     = true;
        sppServer.spp_handle_data_event_func = interconnection_mail_put;
        sppServer.reader_thread_id           = app_interconnection_get_read_thread();

        status = spp_open(&sppServer, NULL, spp_callback);
        TRACE("spp open status is %d", status);
    }
}

#endif
