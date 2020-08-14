/**
 * @file app_interconnection_ccmp.cpp
 *
 * @brief
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
 */
#ifdef __INTERCONNECTION__

#include <stdlib.h>
#include "hal_trace.h"
#include "string.h"

#include "bluetooth.h"
#include "spp.h"
#include "app_bt.h"

#include "umm_malloc.h"
#include "app_interconnection.h"
#include "app_interconnection_logic_protocol.h"
#include "app_interconnection_ccmp.h"
#include "app_interconnection_ota.h"

#define CCMP_GET_PACKET_LENGTH 3

static struct spp_device ccmp_dev;
osMutexDef(ccmp_spp_mutex);

static uint8_t ccmp_rx_buf[CCMP_RECV_BUFFER_SIZE];

struct spp_device *app_interconnection_get_ccmp_dev()
{
    return &ccmp_dev;
}

uint8_t *app_interconnection_ccmp_ea_parse_data(uint8_t *sourceDataPtr, uint32_t *eaDataPtr)
{
    uint8_t eaIndex = 0;
    *eaDataPtr      = 0;

    do
    {
        if (eaIndex >= EA_MAX_LENGTH)
        {
            return NULL;
        }

        *eaDataPtr = (((uint32_t)(*sourceDataPtr & EA_DATA_MASK)) << (eaIndex * 7)) | (*eaDataPtr);

        eaIndex++;
        sourceDataPtr++;

    } while ((*sourceDataPtr & EA_HEAD_MASK) == CCMP_EA_HEAD_WITH_EXTENSION);

    return sourceDataPtr;
}

void app_ccmp_init_tx_buf(uint8_t *ptr)
{
}

static void app_ccmp_free_tx_buf(uint8_t *ptrData, uint32_t dataLen)
{
    INTERCONNECTION_FREE(( void * )ptrData);
}

uint8_t *app_ccmp_fill_data_into_tx_buf(uint8_t *ptrData, uint32_t dataLen)
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


static void lock_ccmp_device(struct spp_device *osDev)
{
    INTERCONNECTION_DEVICE_CHECK(osDev);

    osMutexWait(osDev->mutex_id, osWaitForever);
}

static void unlock_ccmp_device(struct spp_device *osDev)
{
    INTERCONNECTION_DEVICE_CHECK(osDev);

    osMutexRelease(osDev->mutex_id);
}

bt_status_t app_interconnection_ccmp_dev_read_data(char *buffer, uint16_t *ptrLen)
{
    bt_status_t readDataStatus = BT_STS_FAILED;
    uint16_t    length         = *ptrLen;

    lock_ccmp_device(&ccmp_dev);

    // read data from spp device
    readDataStatus = spp_read(&ccmp_dev, ( char * )buffer, &length);
    *ptrLen        = length;

    unlock_ccmp_device(&ccmp_dev);
    return readDataStatus;
}


static void app_ccmp_send_data(uint8_t *ptrData, uint16_t length)
{
    if (!app_interconnection_get_ccmp_alive_flag())
    {
        TRACE("ccmp channel not connected!");
        return;
    }

    spp_write(&ccmp_dev, ( char * )ptrData, &length);
}

void app_interconnection_send_data_via_ccmp(uint8_t *ptrData, uint32_t length)
{
    uint8_t *ptrBuf = app_ccmp_fill_data_into_tx_buf(ptrData, length);
    if (NULL == ptrBuf)
    {
        TRACE("error, null pointer for ccmp tx data.");
        return;
    }

    app_ccmp_send_data(ptrBuf, ( uint16_t )length);
}


void app_interconnection_ccmp_disconnected_callback(void)
{
    TRACE("ccmp disconnected.");
    app_interconnection_set_ccmp_alive_flag(false);

    struct _spp_dev *dev = &ccmp_dev.sppDev;

#if BTIF_SPP_CLIENT == BTIF_ENABLED
    spp_free(dev->type.client.sdpToken);
    dev->type.client.sdpToken = NULL;
#endif
    spp_free(dev->channel);
    dev->channel = NULL;
    spp_free(ccmp_dev.tx_buffer);
    ccmp_dev.tx_buffer = NULL;

    app_interconnection_ota_finished_handler();
    if (false == app_interconnection_get_spp_client_alive_flag() &&
        false == app_interconnection_get_spp_server_alive_flag() &&
        false == app_interconnection_get_ccmp_alive_flag())
    {
        app_interconnection_close_read_thread();
    }
}

static void ccmp_callback(struct spp_device *locDev, struct spp_callback_parms *Info)
{
    switch (Info->event)
    {
    case SPP_EVENT_REMDEV_CONNECTED:
        TRACE("::CCMP_EVENT_REMDEV_CONNECTED %d\n", Info->event);
        app_interconnection_set_ccmp_alive_flag(true);
        break;

    case SPP_EVENT_REMDEV_DISCONNECTED:
        TRACE("::CCMP_EVENT_REMDEV_DISCONNECTED %d\n", Info->event);
        app_interconnection_ccmp_disconnected_callback();
        break;

    case SPP_EVENT_DATA_SENT:
        app_ccmp_free_tx_buf((( struct spp_tx_done * )(Info->p.other))->tx_buf,
                             (( struct spp_tx_done * )(Info->p.other))->tx_data_length);
        break;

    default:
        TRACE("::unknown event %d\n", Info->event);
        break;
    }
}

void app_ccmp_client_open(uint8_t *pServiceSearchRequest, uint8_t serviceSearchRequestLen, uint8_t port)
{
    osMutexId mid;

    TRACE("[%s]", __func__);

    bt_status_t status;
    app_ccmp_init_tx_buf(NULL);
    spp_init_rx_buf(&ccmp_dev, ccmp_rx_buf, CCMP_RECV_BUFFER_SIZE);

    mid = osMutexCreate(osMutex(ccmp_spp_mutex));
    if (!mid) {
        ASSERT(0, "cannot create mutex");
    }

    ccmp_dev.portType = SPP_CLIENT_PORT;
    spp_init_device(&ccmp_dev, SPP_MAX_PACKET_NUM, mid);
    ccmp_dev.sppDev.type.client.rfcommServiceSearchRequestPtr = pServiceSearchRequest;
    ccmp_dev.sppDev.type.client.rfcommServiceSearchRequestLen = serviceSearchRequestLen;
    ccmp_dev.useMessageInformRxData                           = true;
    ccmp_dev.spp_handle_data_event_func                       = interconnection_mail_put;
    ccmp_dev.reader_thread_id                                 = app_interconnection_get_read_thread();
    status                                                    = ccmp_open(&ccmp_dev, app_bt_get_connected_mobile_device_ptr(), ccmp_callback, port);
    TRACE("ccmp open return status is %d", status);
}


#endif  // #ifdef __INTERCONNECTION__
