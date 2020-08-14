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
#ifndef __APP_INTERCONNECTION_SPP_H__
#define __APP_INTERCONNECTION_SPP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth.h"

#define SPP_RECV_BUFFER_SIZE 1024

/**
 * @brief : parse the EA data for interconnection
 *
 * @param sourceDataPtr : pointer of data to be parsed
 * @param eaDataPtr : pointer of EA data
 * @return uint8_t* : pointer of data after EA data parsed
 */
uint8_t *app_interconnection_ea_parse_data(uint8_t *sourceDataPtr, uint32_t *eaDataPtr);

/**
 * @brief : calculate crc 16 for interconnection data
 *
 * @param buf : buffer of data to be calculated
 * @param len : length of data to be calculated
 * @return uint16_t : crc result
 */
uint16_t app_interconnection_spp_data_calculate_crc16(uint8_t *buf, uint32_t len);

/**
 * @brief : get the pointer of interconnection spp client device
 *
 * @return struct spp_device* : the pointer of interconnection spp client device
 */
struct spp_device *app_interconnection_get_spp_client();

/**
 * @brief : get the pointer of interconnection spp server device
 *
 * @return struct spp_device* : the pointer of interconnection spp server device
 */
struct spp_device *app_interconnection_get_spp_server(void);

/**
 * @brief : type of transmition done callback of spp device
 *
 */
typedef void (*app_spp_tx_done_t)(void);

/**
 * @brief : register transmition done callback function for interconnection spp client
 *
 * @param callback : callback function to be registered
 */
void app_spp_register_tx_done(app_spp_tx_done_t callback);

/**
 * @brief : open spp client
 *
 * @param pServiceSearchRequest : pointer of service search request
 * @param serviceSearchRequestLen : length of service search request
 */
void app_spp_client_open(uint8_t *pServiceSearchRequest, uint8_t serviceSearchRequestLen);

/**
 * @brief : open spp server
 *
 */
void app_spp_server_open(void);

/**
 * @brief : spp client read data function
 *
 * @param buffer : data buffer to be read
 * @param ptrLen : length of data to be read
 * @return bt_status_t : status of read process
 */
bt_status_t app_interconnection_spp_client_read_data(char *buffer, uint16_t *ptrLen);

/**
 * @brief : spp server read data function
 *
 * @param buffer : data buffer to be read
 * @param ptrLen : length of data to be read
 * @return bt_status_t : status of read process
 */
bt_status_t app_interconnection_spp_server_read_data(char *buffer, uint16_t *ptrLen);

/**
 * @brief : send data via interconnection spp
 *
 * @param ptrData : pointer of data to be sent
 * @param length : length of data to be sent
 */
void app_interconnection_send_data_via_spp(struct spp_device *sppDev, uint8_t *ptrData, uint32_t length);

/**
 * @brief : interconnection spp client disconnection callback function
 *
 */
void app_interconnection_spp_client_disconnection_callback(void);

/**
 * @brief ： interconnection spp server disconnection callback function
 *
 */
void app_interconnection_spp_server_disconnection_callback(void);

#ifdef __cplusplus
}
#endif

#endif  // #ifndef __APP_INTERCONNECTION_SPP_H__
#endif  // #ifdef __INTERCONNECTION__