/**
 ****************************************************************************************
 *
 * @file app_tencent_svc_cmd_handler.c
 *
 * @date 23rd Nov 2017
 *
 * @brief The framework of the smart voice command handler
 *
 * Copyright (C) 2017
 *
 *
 ****************************************************************************************
 */
#ifdef __INTERCONNECTION__

#include "hal_trace.h"
#include "string.h"

#include "umm_malloc.h"
#include "app_interconnection.h"
#include "app_interconnection_tlv.h"
#include "app_interconnection_logic_protocol.h"
#include "app_interconnection_spp.h"
#include "app_interconnection_ccmp.h"
#include "app_interconnection_ota.h"
#include "app_interconnection_customer_realize.h"

#define LOGIC_PROTOCOL (1)
#define LOG_TAG "[LOGIC_PROTOCOL]"

#if LOGIC_PROTOCOL
#define LOG_DBG(str, ...) TRACE(LOG_TAG "DBG:"  \
                                        "" str, \
                                ##__VA_ARGS__)  // DEBUG OUTPUT
#define LOG_MSG(str, ...) TRACE(LOG_TAG "MSG:"  \
                                        "" str, \
                                ##__VA_ARGS__)  // MESSAGE OUTPUT
#define LOG_ERR(str, ...) TRACE(LOG_TAG "ERR:"  \
                                        "" str, \
                                ##__VA_ARGS__)  // ERROR OUTPUT
#define LOG_FUNC_LINE() TRACE(LOG_TAG "%s:%d\n", __FUNCTION__, __LINE__)
#define LOG_FUNC_IN() TRACE(LOG_TAG "%s ++++\n", __FUNCTION__)
#define LOG_FUNC_OUT() TRACE(LOG_TAG "%s ----\n", __FUNCTION__)
#define LOG_DUMP DUMP8
#else
#define LOG_DBG(str, ...)
#define LOG_MSG(str, ...)
#define LOG_ERR(str, ...)
#define LOG_FUNC_LINE()
#define LOG_FUNC_IN()
#define LOG_FUNC_OUT()
#define LOG_DUMP
#endif

#define CCMP_NEGO_PACKET_MAX_LENGTH (100)

static TLV_ITEM_T rootItem;
static uint8_t *  itemStr = NULL;

static char reconnectTrigger[] = "\x6B\x7D\x01\x00\x00\x03\x00\x5A";//"\x5A\x00\x03\x00\x00\x01\x7D\x6B";

static const uint16_t crc16_tab_ccmp[] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D, 0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4, 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC, 0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823, 0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B, 0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A, 0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49, 0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78, 0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F, 0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067, 0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634, 0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3, 0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92, 0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1, 0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0};

uint16_t app_interconn_calculate_ccmp_crc16(uint8_t *buf, uint32_t len)
{
    LOG_MSG("ccmp crc data length to be calculated is %d", len);
    uint16_t cksum = 0x0000;

    for (uint32_t i = 0; i < len; i++)
    {
        cksum = crc16_tab_ccmp[((cksum >> 8) ^ *buf++) & 0xFF] ^ (cksum << 8);
    }
    return cksum;
}

uint32_t uint32_convert_endian(uint32_t data)
{
    UINT32_CONV_U d1, d2;
    d1.f    = data;
    d2.c[0] = d1.c[3];
    d2.c[1] = d1.c[2];
    d2.c[2] = d1.c[1];
    d2.c[3] = d1.c[0];
    return d2.f;
}

uint16_t uint16_convert_endian(uint16_t data)
{
    UINT16_CONV_U d1, d2;
    d1.f    = data;
    d2.c[0] = d1.c[1];
    d2.c[1] = d1.c[0];
    return d2.f;
}

uint8_t app_interconnection_ccmp_creat_error_code(uint8_t module, uint8_t errType, uint32_t *errorPtr)
{
    if (NULL == errorPtr)
    {
        LOG_MSG("error pointer is null.");
        return INTERCONNECTION_STATUS_ERROR;
    }
    uint32_t errorCode = CCMP_ERR_HEAD * CCMP_ERR_HEAD_BASE + module * CCMP_ERR_MODULE_BASE + errType;
    LOG_MSG("error code is %d", errorCode);

    *errorPtr = uint32_convert_endian(errorCode);

    return INTERCONNECTION_STATUS_SUCCESS;
}

void app_interconnection_spp_send_ccmp_init_command()
{
    CCMP_NEGO_HEAD_T *ccmpInitPtr = ( CCMP_NEGO_HEAD_T * )INTERCONNECTION_MALLOC(CCMP_NEGO_PACKET_MAX_LENGTH);
    uint8_t           dataLength  = 0;
    TLV_T *           temp        = NULL;
    uint16_t          crc16       = 0;

    ccmpInitPtr->sof             = SOF_VALUE;
    ccmpInitPtr->type            = EA_WITHOUT_EXTENSION(CCMP_NEGO_TYPE_CHANNEL_MANAGER);
    ccmpInitPtr->flag.flagType   = EA_WITHOUT_EXTENSION(CCMP_NEGO_FLAG_WITHOUT_ENCRYPTION);
    ccmpInitPtr->flag.flagLength = EA_WITHOUT_EXTENSION(CCMP_NEGO_FLAG_LENGTH);

    temp           = ( TLV_T * )&ccmpInitPtr->payload[dataLength];
    temp->type     = EA_WITHOUT_EXTENSION(CCMP_INIT_REQUEST_VERSION);
    temp->length   = EA_WITHOUT_EXTENSION(CCMP_INIT_VERSION_LENGTH);
    temp->value[0] = CCMP_INIT_VERSION_VALUE;
    dataLength += 3;

    temp         = ( TLV_T * )&ccmpInitPtr->payload[dataLength];
    temp->type   = EA_WITHOUT_EXTENSION(CCMP_INIT_REQUEST_SUMMARY);
    temp->length = EA_WITHOUT_EXTENSION(strlen(get_summary_value_ptr()));
    memcpy(temp->value, get_summary_value_ptr(), strlen(get_summary_value_ptr()));
    dataLength += (2 + strlen(get_summary_value_ptr()));

    temp           = ( TLV_T * )&ccmpInitPtr->payload[dataLength];
    temp->type     = EA_WITHOUT_EXTENSION(CCMP_INIT_REQUEST_SECURITY);
    temp->length   = EA_WITHOUT_EXTENSION(CCMP_INIT_SECURITY_LENGTH);
    temp->value[0] = CCMP_NEGO_FLAG_WITHOUT_ENCRYPTION;
    dataLength += 3;

    ccmpInitPtr->length = EA_WITHOUT_EXTENSION(dataLength);

    LOG_MSG("length of data to be calculated is %d", dataLength);
    crc16 = app_interconnection_spp_data_calculate_crc16(( uint8_t * )ccmpInitPtr, (dataLength + CCMP_NEGO_PAYLOAD_OFFSET));
    memcpy(&ccmpInitPtr->payload[dataLength], ( uint8_t * )(&crc16), CRC16_LENGTH);
    dataLength += (CRC16_LENGTH + CCMP_NEGO_PAYLOAD_OFFSET);

    app_interconnection_ccmp_negotiation_data_send(( uint8_t * )ccmpInitPtr, dataLength);
    INTERCONNECTION_FREE(ccmpInitPtr);
}

static void app_interconnection_spp_send_ccmp_business_command()
{
    CCMP_NEGO_HEAD_T *ccmpInitPtr = ( CCMP_NEGO_HEAD_T * )INTERCONNECTION_MALLOC(CCMP_NEGO_PACKET_MAX_LENGTH);
    uint8_t           dataLength  = 0;
    TLV_T *           temp        = NULL;
    uint16_t          crc16       = 0;

    ccmpInitPtr->sof             = SOF_VALUE;
    ccmpInitPtr->type            = EA_WITHOUT_EXTENSION(CCMP_NEGO_TYPE_INSTANT_MESSAGE);
    ccmpInitPtr->flag.flagType   = EA_WITHOUT_EXTENSION(CCMP_NEGO_FLAG_WITHOUT_ENCRYPTION);
    ccmpInitPtr->flag.flagLength = EA_WITHOUT_EXTENSION(CCMP_NEGO_FLAG_LENGTH);

    temp           = ( TLV_T * )&ccmpInitPtr->payload[dataLength];
    temp->type     = EA_WITHOUT_EXTENSION(CCMP_BUSINESS_REQUEST_COMMAND);
    temp->length   = EA_WITHOUT_EXTENSION(CCMP_BUSINESS_REQUEST_DATA_LENGTH);
    temp->value[0] = EA_WITHOUT_EXTENSION(CCMP_BUSINESS_REQUEST_ICONNECT);
    temp->value[1] = EA_WITHOUT_EXTENSION(CCMP_BUSINESS_REQUEST_SECURITY);
    dataLength += 4;

    ccmpInitPtr->length = EA_WITHOUT_EXTENSION(dataLength);

    LOG_MSG("length of data to be calculated is %d", dataLength);
    crc16 = app_interconnection_spp_data_calculate_crc16(( uint8_t * )ccmpInitPtr, (dataLength + CCMP_NEGO_PAYLOAD_OFFSET));
    memcpy(&ccmpInitPtr->payload[dataLength], ( uint8_t * )(&crc16), CRC16_LENGTH);
    dataLength += (CRC16_LENGTH + CCMP_NEGO_PAYLOAD_OFFSET);

    app_interconnection_ccmp_negotiation_data_send(( uint8_t * )ccmpInitPtr, dataLength);
    INTERCONNECTION_FREE(ccmpInitPtr);
}

static void app_interconnection_spp_send_ccmp_socket_command()
{
    CCMP_NEGO_HEAD_T *ccmpInitPtr = ( CCMP_NEGO_HEAD_T * )INTERCONNECTION_MALLOC(CCMP_NEGO_PACKET_MAX_LENGTH);
    uint8_t           dataLength  = 0;
    TLV_T *           temp        = NULL;
    uint16_t          crc16       = 0;

    ccmpInitPtr->sof             = SOF_VALUE;
    ccmpInitPtr->type            = EA_WITHOUT_EXTENSION(CCMP_NEGO_TYPE_INSTANT_MESSAGE);
    ccmpInitPtr->flag.flagType   = EA_WITHOUT_EXTENSION(CCMP_NEGO_FLAG_WITHOUT_ENCRYPTION);
    ccmpInitPtr->flag.flagLength = EA_WITHOUT_EXTENSION(CCMP_NEGO_FLAG_LENGTH);

    temp                      = ( TLV_T * )&ccmpInitPtr->payload[dataLength];
    temp->type                = EA_WITHOUT_EXTENSION(CCMP_SOCKET_REQUEST_COMMAND);
    temp->value[dataLength++] = EA_WITHOUT_EXTENSION(CCMP_SOCKET_CREAT_CHANNEL_BT);
    temp->value[dataLength++] = EA_WITHOUT_EXTENSION(CCMP_SOCKET_CREAT_BUSINESS_ICONNECT);
    temp->value[dataLength++] = EA_WITHOUT_EXTENSION(strlen(get_business_tag_ptr()));
    memcpy(&temp->value[dataLength], get_business_tag_ptr(), strlen(get_business_tag_ptr()));
    dataLength += strlen(get_business_tag_ptr());
    temp->value[dataLength++] = EA_WITHOUT_EXTENSION(CCMP_SOCKET_CREAT_PROTOCOL_BT);
    temp->value[dataLength++] = EA_WITHOUT_EXTENSION(CCMP_SOCKET_CREAT_SERVICE_USE_DEFAULT_UUID);
    temp->length              = EA_WITHOUT_EXTENSION(dataLength);
    dataLength += 2;  // command: 1 byte  length: 1 byte

    ccmpInitPtr->length = EA_WITHOUT_EXTENSION(dataLength);

    LOG_MSG("length of data to be calculated is %d", dataLength);
    crc16 = app_interconnection_spp_data_calculate_crc16(( uint8_t * )ccmpInitPtr, (dataLength + CCMP_NEGO_PAYLOAD_OFFSET));
    memcpy(&ccmpInitPtr->payload[dataLength], ( uint8_t * )(&crc16), CRC16_LENGTH);
    dataLength += (CRC16_LENGTH + CCMP_NEGO_PAYLOAD_OFFSET);

    app_interconnection_ccmp_negotiation_data_send(( uint8_t * )ccmpInitPtr, dataLength);
    INTERCONNECTION_FREE(ccmpInitPtr);
}

uint8_t app_interconnection_business_command_check(uint32_t command)
{
    return (CCMP_BUSINESS_RESPONSE_COMMAND == command) ? INTERCONNECTION_STATUS_SUCCESS : INTERCONNECTION_STATUS_ERROR;
}

uint8_t app_interconnection_command_length_check(uint32_t parsedLength, uint32_t realLength)
{
    return (parsedLength == realLength) ? INTERCONNECTION_STATUS_SUCCESS : INTERCONNECTION_STATUS_ERROR;
}

uint8_t app_interconnection_business_id_check(uint32_t businessId)
{
    return (CCMP_BUSINESS_REQUEST_ICONNECT == businessId) ? INTERCONNECTION_STATUS_SUCCESS : INTERCONNECTION_STATUS_ERROR;
}

uint8_t app_interconnection_business_support_check(uint32_t business)
{
    return (CCMP_BUSINESS_RESPONSE_ICONNECT_SUPPORT == business) ? INTERCONNECTION_STATUS_SUCCESS : INTERCONNECTION_STATUS_ERROR;
}

uint8_t app_interconnection_security_id_check(uint32_t securityId)
{
    return (CCMP_BUSINESS_REQUEST_SECURITY == securityId) ? INTERCONNECTION_STATUS_SUCCESS : INTERCONNECTION_STATUS_ERROR;
}

uint8_t app_interconnection_security_support_check(uint32_t security)
{
    return (CCMP_BUSINESS_RESPONSE_SECURITY_SUPPORT == security) ? INTERCONNECTION_STATUS_SUCCESS : INTERCONNECTION_STATUS_ERROR;
}

static uint8_t app_interconnection_parse_business_data(uint8_t *ptrData, uint32_t dataLength)
{
    uint8_t *dataPtr = ptrData;
    uint32_t parsedData;

    dataPtr = app_interconnection_ea_parse_data(dataPtr, &parsedData);
    if (INTERCONNECTION_STATUS_ERROR == app_interconnection_business_command_check(parsedData))
    {
        LOG_MSG("business command invalid, stop parse business data.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    dataPtr = app_interconnection_ea_parse_data(dataPtr, &parsedData);
    if (INTERCONNECTION_STATUS_ERROR == app_interconnection_command_length_check(parsedData, (dataLength - (dataPtr - ptrData))))
    {
        LOG_MSG("business length invalid, stop parse business data. parsedData is %d, real data length is %d", parsedData, (dataLength - (dataPtr - ptrData)));
        return INTERCONNECTION_STATUS_ERROR;
    }

    dataPtr = app_interconnection_ea_parse_data(dataPtr, &parsedData);
    if (INTERCONNECTION_STATUS_ERROR == app_interconnection_business_id_check(parsedData))
    {
        LOG_MSG("business id invalid, stop parse business data.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    dataPtr = app_interconnection_ea_parse_data(dataPtr, &parsedData);
    if (INTERCONNECTION_STATUS_ERROR == app_interconnection_business_support_check(parsedData))
    {
        LOG_MSG("business is not supported, stop parse business data.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    dataPtr = app_interconnection_ea_parse_data(dataPtr, &parsedData);
    if (INTERCONNECTION_STATUS_ERROR == app_interconnection_security_id_check(parsedData))
    {
        LOG_MSG("security id is invalid, stop parse business data.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    dataPtr = app_interconnection_ea_parse_data(dataPtr, &parsedData);
    if (INTERCONNECTION_STATUS_ERROR == app_interconnection_security_support_check(parsedData))
    {
        LOG_MSG("security is not supported.");
        // return INTERCONNECTION_STATUS_ERROR;
    }
    else
    {
        LOG_MSG("security is supported.");
    }

    app_interconnection_spp_send_ccmp_socket_command();
    return INTERCONNECTION_STATUS_SUCCESS;
}

uint8_t app_interconnection_socket_command_check(uint32_t command)
{
    return (CCMP_SOCKET_RESPONSE_COMMAND == command) ? INTERCONNECTION_STATUS_SUCCESS : INTERCONNECTION_STATUS_ERROR;
}

uint8_t app_interconnection_socket_result_check(uint32_t result)
{
    return (CCMP_SOCKET_RESPONSE_RESULT_ACCEPTED == result) ? INTERCONNECTION_STATUS_SUCCESS : INTERCONNECTION_STATUS_ERROR;
}

uint8_t app_interconnection_socket_port_check(uint32_t port)
{
    if ((127 <= port) || (port == CCMP_SOCKET_RESPONSE_PORT_INVALID))
    {
        LOG_MSG("invalid port number!");
        return INTERCONNECTION_STATUS_ERROR;
    }

    app_interconnection_set_ccmp_channel(( uint8_t )port);
    return INTERCONNECTION_STATUS_SUCCESS;
}

uint8_t app_interconnection_channel_id_check(uint32_t channel)
{
    return (CCMP_SOCKET_CREAT_CHANNEL_BT == channel) ? INTERCONNECTION_STATUS_SUCCESS : INTERCONNECTION_STATUS_ERROR;
}

uint8_t app_interconnection_business_tag_check(uint32_t tagLength, uint8_t **ptrTag)
{
    if (memcmp(CCMP_SOCKET_CREAT_BUSINESS_TAG, *ptrTag, tagLength))
    {
        LOG_ERR("business tag error. Tag length is %d", tagLength);
        LOG_ERR("local tag is:");
        LOG_DUMP("0x%02x ", CCMP_SOCKET_CREAT_BUSINESS_TAG, tagLength);
        LOG_ERR("received tag is:");
        LOG_DUMP("0x%02x ", *ptrTag, tagLength);
        return INTERCONNECTION_STATUS_ERROR;
    }

    *ptrTag += tagLength;
    return INTERCONNECTION_STATUS_SUCCESS;
}

uint8_t app_interconnection_socket_protocol_check(uint32_t protocol)
{
    return (CCMP_SOCKET_CREAT_PROTOCOL_BT == protocol) ? INTERCONNECTION_STATUS_SUCCESS : INTERCONNECTION_STATUS_ERROR;
}

uint8_t app_interconnection_parse_socket_data(uint8_t *ptrData, uint32_t dataLength)
{
    uint8_t *dataPtr = ptrData;
    uint32_t parsedData;

    dataPtr = app_interconnection_ea_parse_data(dataPtr, &parsedData);
    if (INTERCONNECTION_STATUS_ERROR == app_interconnection_socket_command_check(parsedData))
    {
        LOG_MSG("ccmp socket command invalid, stop parse socket data.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    dataPtr = app_interconnection_ea_parse_data(dataPtr, &parsedData);
    if (INTERCONNECTION_STATUS_ERROR == app_interconnection_command_length_check(parsedData, (dataLength - (dataPtr - ptrData))))
    {
        LOG_MSG("ccmp socket length invalid, stop parse socket data.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    dataPtr = app_interconnection_ea_parse_data(dataPtr, &parsedData);
    if (INTERCONNECTION_STATUS_ERROR == app_interconnection_socket_result_check(parsedData))
    {
        LOG_MSG("ccmp socket creat denied by mobile, stop parse socket data.");
        app_interconnection_set_ccmp_denied_by_mobile_flag(true);
        return INTERCONNECTION_STATUS_ERROR;
    }

    dataPtr = app_interconnection_ea_parse_data(dataPtr, &parsedData);
    if (INTERCONNECTION_STATUS_ERROR == app_interconnection_socket_port_check(parsedData))
    {
        LOG_MSG("invalid socket port, stop parse socket data.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    dataPtr = app_interconnection_ea_parse_data(dataPtr, &parsedData);
    if (INTERCONNECTION_STATUS_ERROR == app_interconnection_channel_id_check(parsedData))
    {
        LOG_MSG("channel id is invalid, stop parse socket data.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    dataPtr = app_interconnection_ea_parse_data(dataPtr, &parsedData);
    if (INTERCONNECTION_STATUS_ERROR == app_interconnection_business_id_check(parsedData))
    {
        LOG_MSG("business id invalid, stop parse socket data.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    dataPtr = app_interconnection_ea_parse_data(dataPtr, &parsedData);
    if (INTERCONNECTION_STATUS_ERROR == app_interconnection_business_tag_check(parsedData, &dataPtr))
    {
        LOG_MSG("business tag invalid, stop parse socket data.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    dataPtr = app_interconnection_ea_parse_data(dataPtr, &parsedData);
    if (INTERCONNECTION_STATUS_ERROR == app_interconnection_socket_protocol_check(parsedData))
    {
        LOG_MSG("socket protocol invalid, stop parse socket data.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    app_interconnection_ccmp_client_open(app_interconnection_get_ccmp_channel());

    return INTERCONNECTION_STATUS_SUCCESS;
}

/**
 * @brief : spp client receive the data from the peer device and parse them
 *
 * @param ptrData : Pointer of the received data
 * @param dataLength : Length of the received data
 * @return : APP_TENCENT_SV_CMD_RET_STATUS_E
 */
uint8_t app_interconnection_spp_client_data_received_callback(uint8_t *ptrData, uint32_t dataLength)
{
    LOG_DBG("Receive cmd data:");  //receive command;
    LOG_DUMP("0x%02x ", ptrData, dataLength);

    CCMP_NEGO_HEAD_T *dataPtr = ( CCMP_NEGO_HEAD_T * )ptrData;

    if (SOF_VALUE != dataPtr->sof)
    {
        LOG_ERR("sof value error, stop parse this packet.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    // flag type check
    if (EA_WITHOUT_EXTENSION(CCMP_NEGO_FLAG_WITHOUT_ENCRYPTION) != dataPtr->flag.flagType)
    {
        LOG_ERR("flag type error, stop parse this packet.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    // flag length check
    if (EA_WITHOUT_EXTENSION(CCMP_NEGO_FLAG_LENGTH) != dataPtr->flag.flagLength)
    {
        LOG_ERR("flag length error, stop parse this packet.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    // length check
    if (EA_WITHOUT_EXTENSION(dataLength - CCMP_NEGO_PAYLOAD_OFFSET - CRC16_LENGTH) !=
        dataPtr->length)
    {
        LOG_ERR("data length error, stop parse this packet.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    // calculate crc
    uint16_t crc16 = app_interconnection_spp_data_calculate_crc16(ptrData, dataLength - 2);
    if (*(( uint16_t * )&ptrData[dataLength - 2]) != crc16)
    {
        LOG_ERR("crc error, stop parse this packet.");
        return INTERCONNECTION_STATUS_ERROR;
    }

    if (EA_WITHOUT_EXTENSION(CCMP_NEGO_TYPE_CHANNEL_MANAGER) == dataPtr->type)  //0x00
    {
        app_interconnection_spp_send_ccmp_business_command();
    }
    else if (EA_WITHOUT_EXTENSION(CCMP_NEGO_TYPE_INSTANT_MESSAGE) == dataPtr->type)  //0x02
    {
        TLV_T *payloadPtr = ( TLV_T * )dataPtr->payload;

        if (EA_WITHOUT_EXTENSION(CCMP_BUSINESS_RESPONSE_COMMAND) == payloadPtr->type)
        {
            LOG_DBG("business response received!");
            if (INTERCONNECTION_STATUS_SUCCESS !=
                app_interconnection_parse_business_data(dataPtr->payload, dataLength - sizeof(CCMP_NEGO_HEAD_T) - 1))  // CRC not included
            {
                LOG_ERR("business response parse error!");
                return INTERCONNECTION_STATUS_ERROR;
            }
        }
        else if (EA_WITHOUT_EXTENSION(CCMP_SOCKET_RESPONSE_COMMAND) == payloadPtr->type)
        {
            LOG_DBG("socket creat response received!");
            if (INTERCONNECTION_STATUS_SUCCESS !=
                app_interconnection_parse_socket_data(dataPtr->payload, (dataLength - sizeof(CCMP_NEGO_HEAD_T) - 1)))  // CRC not included
            {
                LOG_ERR("socket creat response parse error!");
                return INTERCONNECTION_STATUS_ERROR;
            }
        }
        else
        {
            LOG_MSG("ccmp nego command %d not recognized!", payloadPtr->type);
            return INTERCONNECTION_STATUS_ERROR;
        }
    }
    else
    {
        LOG_MSG("ccmp nego type %d not recognized!", dataPtr->type);
        return INTERCONNECTION_STATUS_ERROR;
    }
    return INTERCONNECTION_STATUS_SUCCESS;
}

/**
 * @brief : spp server receive the data from the peer device and parse them
 *
 * @param ptrData : pointer of the received data
 * @param dataLength : length of the received data
 * @return uint8_t : APP_TENCENT_SV_CMD_RET_STATUS_E
 */
uint8_t app_interconnection_spp_server_data_received_callback(uint8_t *ptrData, uint32_t dataLength)
{
    if (memcmp(ptrData, reconnectTrigger, sizeof(reconnectTrigger)-1))
    {
        TRACE("reconnect trigger is received.");
        if (app_interconnection_get_spp_client_alive_flag() &&
            !app_interconnection_get_ccmp_alive_flag())
        {
            app_interconnection_spp_send_ccmp_init_command();
        }
        else
        {
            if (!app_interconnection_get_spp_client_alive_flag())
            {
                app_interconnection_spp_open();
            }
            else
            {
                TRACE("spp device ptr is null!");
            }
        }
    }
    else
    {
        TRACE("spp server received unrecognized data!");
    }

    return 0;
}

void app_interconnection_handle_favorite_music_through_ccmp(uint8_t process)
{
    LOG_MSG("[%s]", __func__);
    uint8_t psiInfo[CCMP_NEGO_PACKET_MAX_LENGTH];
    uint8_t psiLen    = 0;
    TLV_T * item      = NULL;
    uint8_t lengthPos = 0;

    psiInfo[psiLen++] = CCMP_SOF_VALUE;
    lengthPos         = psiLen;
    psiLen += 2;
    psiInfo[psiLen++] = CCMP_CONTROL;
    psiInfo[psiLen++] = CCMP_MODULE_MIDWARE_SERVICE;
    psiInfo[psiLen++] = CCMP_MIDWARE_COMMAND_ID_HUAWEI_MUSIC;

    item           = ( TLV_T * )(&psiInfo[psiLen]);
    item->type     = TYPE_FAVORITE_INFO;
    item->length   = 1;
    item->value[0] = process;
    psiLen += (item->length + 2);

    psiInfo[lengthPos++] = (psiLen - 3) >> 8;
    psiInfo[lengthPos]   = (psiLen - 3) & 0xFF;

    uint16_t crc      = app_interconn_calculate_ccmp_crc16(psiInfo, psiLen);
    psiInfo[psiLen++] = crc >> 8;
    psiInfo[psiLen++] = crc & 0xFF;

    ASSERT((psiLen < CCMP_NEGO_PACKET_MAX_LENGTH), "CCMP send buffer over flow!!!");

    app_interconnection_ccmp_data_send(( uint8_t * )psiInfo, psiLen);
}

static void pack_and_send_frame(TLV_ITEM_T *root, uint8_t serviceID, uint8_t commandID)
{
    LOG_FUNC_IN();

    CCMP_COMMUNICATION_HEAD_T frameHeader;
    uint32_t                  frameLength = 0;
    uint16_t                  crc16       = 0;

    if (NULL != itemStr)
    {
        INTERCONNECTION_FREE(itemStr);
    }

    itemStr = app_interconnection_tlv_malloc_for_item_tree_with_margin(root, CCMP_ITEM_OFFSET, CRC16_LENGTH, &frameLength);
    if (NULL == itemStr)
    {
        LOG_ERR("malloc for send data error!!!");
        return;
    }

    LOG_DBG("frame length is %d", frameLength);

    // init and fill head
    frameHeader.sof         = CCMP_SOF_VALUE;
    frameHeader.length      = uint16_convert_endian((uint16_t)(frameLength - CCMP_PAYLOAD_OFFSET - CRC16_LENGTH));
    frameHeader.ccmpControl = CCMP_CONTROL;
    frameHeader.serviceID   = serviceID;
    frameHeader.commandID   = commandID;
    memcpy(itemStr, &frameHeader, CCMP_ITEM_OFFSET);

    // fill crc
    crc16 = uint16_convert_endian(app_interconn_calculate_ccmp_crc16(itemStr, (frameLength - CRC16_LENGTH)));
    memcpy((itemStr + frameLength - CRC16_LENGTH), ( uint8_t * )&crc16, CRC16_LENGTH);

    app_interconnection_ccmp_data_send(itemStr, frameLength);

    if (NULL != itemStr)
    {
        INTERCONNECTION_FREE(itemStr);
        itemStr = NULL;
    }

    LOG_FUNC_OUT();
}

/**
 * @brief : used for cases where only error code is need to report
 *
 * @param module : used to generate error code
 * @param err : used to generate error code
 * @param service : used to fill service ID
 * @param command :used to fill command ID
 */
void app_interconnection_response_error_code(uint8_t module, uint8_t err, uint8_t serviceId, uint8_t commandId)
{
    uint32_t errorCode = 0;
    app_interconnection_ccmp_creat_error_code(module, err, &errorCode);
    memset(&rootItem, 0, sizeof(TLV_ITEM_T));
    rootItem.type   = CCMP_GENERAL_USED_ERROR_CODE_TYPE;
    rootItem.length = CCMP_GENERAL_USED_ERROR_CODE_LEN;
    rootItem.value  = ( uint8_t * )&errorCode;

    pack_and_send_frame(&rootItem, serviceId, commandId);
}

#if 0
static void service_or_command_not_support_handler(uint8_t serviceId, uint8_t commandId, uint8_t errType)
{
    LOG_FUNC_IN();

    uint32_t errorCode = 0;

    // creat error code
    app_interconnection_ccmp_creat_error_code(CCMP_MODULE_GENERAL, errType, &errorCode);

    memset(&rootItem, 0, sizeof(TLV_ITEM_T));
    rootItem.type = CCMP_GENERAL_USED_ERROR_CODE_TYPE;
    rootItem.length = CCMP_GENERAL_USED_ERROR_CODE_LEN;
    rootItem.value = (uint8_t*)&errorCode;

    pack_and_send_frame(&rootItem, serviceId, commandId);

    LOG_FUNC_OUT();
}
#endif

static void link_param_command_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    uint8_t random_value[18] = {0x00, 0x02, 0x55, 0xa1, 0xe5, 0xfa, 0x11, 0x99, 0x13, 0x26, 0xae, 0x8b, 0xd9, 0xe6, 0x3b, 0x6c, 0x57, 0xb7};

    uint8_t  status    = 0;
    uint32_t errorCode = 0;

    TLV_ITEM_T maxFrameSizeItem;
    TLV_ITEM_T maxTrUnitItem;
    TLV_ITEM_T intervalItem;
    TLV_ITEM_T randomValueItem;

    uint16_t maxFramSize;
    uint16_t maxTrUnit;
    uint16_t interval;
    uint16_t tempUint16;

    if (NULL == ptrItem)
    {
        app_interconnection_ccmp_creat_error_code(CCMP_MODULE_GENERAL, CCMP_GEN_ERR_CODE_REQUEST_FORMAT_ERROR, &errorCode);
        status = 1;
    }

    if (NULL == app_interconnection_tlv_get_item_with_type(ptrItem, 1))
    {
        app_interconnection_ccmp_creat_error_code(CCMP_MODULE_GENERAL, CCMP_GEN_ERR_CODE_REQUEST_FORMAT_ERROR, &errorCode);
        status = 1;
    }

    if (NULL == app_interconnection_tlv_get_item_with_type(ptrItem, 2))
    {
        app_interconnection_ccmp_creat_error_code(CCMP_MODULE_GENERAL, CCMP_GEN_ERR_CODE_REQUEST_FORMAT_ERROR, &errorCode);
        status = 1;
    }

    if (NULL == app_interconnection_tlv_get_item_with_type(ptrItem, 3))
    {
        app_interconnection_ccmp_creat_error_code(CCMP_MODULE_GENERAL, CCMP_GEN_ERR_CODE_REQUEST_FORMAT_ERROR, &errorCode);
        status = 1;
    }

    if (NULL == app_interconnection_tlv_get_item_with_type(ptrItem, 4))
    {
        app_interconnection_ccmp_creat_error_code(CCMP_MODULE_GENERAL, CCMP_GEN_ERR_CODE_REQUEST_FORMAT_ERROR, &errorCode);
        status = 1;
    }

    if (status)
    {
        memset(&rootItem, 0, sizeof(TLV_ITEM_T));
        rootItem.type   = CCMP_GENERAL_USED_ERROR_CODE_TYPE;
        rootItem.length = CCMP_GENERAL_USED_ERROR_CODE_LEN;
        rootItem.value  = ( uint8_t * )&errorCode;
    }
    else
    {
        memset(&rootItem, 0, sizeof(TLV_ITEM_T));
        rootItem.type   = TYPE_PROTOCOL_VERSION;
        rootItem.length = 0x01;
        rootItem.value  = get_link_param_protocol_version();

        memset(&maxFrameSizeItem, 0, sizeof(TLV_ITEM_T));
        maxFrameSizeItem.type   = TYPE_MAX_FRAME_SIZE;
        maxFrameSizeItem.length = 0x02;
        tempUint16              = MTU_MAX_LENGTH;
        maxFramSize             = uint16_convert_endian(tempUint16);
        maxFrameSizeItem.value  = ( uint8_t * )&(maxFramSize);
        tlv_item_add_brother(&rootItem, &maxFrameSizeItem);

        memset(&maxTrUnitItem, 0, sizeof(TLV_ITEM_T));
        maxTrUnitItem.type   = TYPE_MAX_TRANSIMISSION_UNIT;
        maxTrUnitItem.length = 0x02;
        tempUint16           = MTU_MAX_LENGTH;
        maxTrUnit            = uint16_convert_endian(tempUint16);
        maxTrUnitItem.value  = ( uint8_t * )&(maxTrUnit);
        tlv_item_add_brother(&rootItem, &maxTrUnitItem);

        memset(&intervalItem, 0, sizeof(TLV_ITEM_T));
        intervalItem.type   = TYPE_LINK_INTERVAL;
        intervalItem.length = 0x02;
        tempUint16          = 10;
        interval            = uint16_convert_endian(tempUint16);
        intervalItem.value  = ( uint8_t * )&(interval);
        tlv_item_add_brother(&rootItem, &intervalItem);

        memset(&randomValueItem, 0, sizeof(TLV_ITEM_T));
        randomValueItem.type   = TYPE_RANDOM_VALUE;
        randomValueItem.length = sizeof(random_value);
        randomValueItem.value  = random_value;
        tlv_item_add_brother(&rootItem, &randomValueItem);
    }

    pack_and_send_frame(&rootItem, CCMP_MODULE_DEVICE_MANAGEMENT, CCMP_DEVICE_COMMAND_ID_LINK_PARAM);

    LOG_FUNC_OUT();
}

static void service_ability_command_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    uint8_t  status           = 0;
    uint32_t errorCode        = 0;
    uint8_t  supportStatus[4] = {0, 0, 0, 0};

    if (TYPE_SUPPORT_SERVICE_REQUEST != ptrItem->type)
    {
        LOG_MSG("invalid income param.");
        app_interconnection_ccmp_creat_error_code(CCMP_MODULE_DEVICE_MANAGEMENT, CCMP_DEV_ERR_CODE_ILLEGAL_PARAM, &errorCode);
        status = 1;
    }

    memset(&rootItem, 0, sizeof(TLV_ITEM_T));

    if (status)
    {
        rootItem.type   = CCMP_GENERAL_USED_ERROR_CODE_TYPE;
        rootItem.length = CCMP_GENERAL_USED_ERROR_CODE_LEN;
        rootItem.value  = ( uint8_t * )&errorCode;
    }
    else
    {
        for (uint8_t i = 0; i < ptrItem->length; i++)
        {
            if (is_service_support(ptrItem->value[i]))
            {
                supportStatus[i] = 1;
            }
        }

        rootItem.type   = TYPE_SUPPORT_SERVICE_RESPONSE;
        rootItem.length = ptrItem->length;
        rootItem.value  = supportStatus;
    }

    pack_and_send_frame(&rootItem, CCMP_MODULE_DEVICE_MANAGEMENT, CCMP_DEVICE_COMMAND_ID_SERVICE_ABILITY);

    LOG_FUNC_OUT();
}

static void command_ability_command_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    LOG_FUNC_OUT();
}

static void time_format_command_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    LOG_FUNC_OUT();
}

static void set_device_time_command_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    LOG_FUNC_OUT();
}

static void get_device_time_command_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    LOG_FUNC_OUT();
}

static void get_mobile_batt_command_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    LOG_FUNC_OUT();
}

static void device_info_command_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    if (NULL == ptrItem)
    {
        LOG_ERR("item pointer is null.");
        return;
    }

    TLV_ITEM_T deviceTypeItem;
    TLV_ITEM_T hardwareVersionItem;
    TLV_ITEM_T phoneNumberItem;
    TLV_ITEM_T btMacItem;
    TLV_ITEM_T imeiItem;
    TLV_ITEM_T softwareVersionItem;
    TLV_ITEM_T openSourceVersionItem;
    TLV_ITEM_T snItem;
    TLV_ITEM_T modelIDItem;
    TLV_ITEM_T emmcItem;
    TLV_ITEM_T nameItem;

    memset(&rootItem, 0, sizeof(TLV_ITEM_T));
    rootItem.type   = TYPE_BT_VERSION;
    rootItem.length = strlen(( char * )get_bt_version_ptr());
    rootItem.value  = ( uint8_t * )get_bt_version_ptr();

    memset(&deviceTypeItem, 0, sizeof(TLV_ITEM_T));
    deviceTypeItem.type   = TYPE_DEVICE_TYPE;
    deviceTypeItem.length = 0x01;
    deviceTypeItem.value  = get_device_type_ptr();
    tlv_item_add_brother(&rootItem, &deviceTypeItem);

    if (NULL != app_interconnection_tlv_get_item_with_type(ptrItem, TYPE_DEVICE_VERSION))
    {
        memset(&hardwareVersionItem, 0, sizeof(TLV_ITEM_T));
        hardwareVersionItem.type   = TYPE_DEVICE_VERSION;
        hardwareVersionItem.length = strlen(get_hardware_version_ptr());
        hardwareVersionItem.value  = ( uint8_t * )get_hardware_version_ptr();
        tlv_item_add_brother(&rootItem, &hardwareVersionItem);
    }

    if (NULL != app_interconnection_tlv_get_item_with_type(ptrItem, TYPE_DEVICE_PHONE_NUMBER))
    {
        memset(&phoneNumberItem, 0, sizeof(TLV_ITEM_T));
        phoneNumberItem.type   = TYPE_DEVICE_PHONE_NUMBER;
        phoneNumberItem.length = strlen(get_phone_number_ptr());
        phoneNumberItem.value  = ( uint8_t * )get_phone_number_ptr();
        tlv_item_add_brother(&rootItem, &phoneNumberItem);
    }

    if (NULL != app_interconnection_tlv_get_item_with_type(ptrItem, TYPE_DEVICE_BT_MAC))
    {
        memset(&btMacItem, 0, sizeof(TLV_ITEM_T));
        btMacItem.type   = TYPE_DEVICE_BT_MAC;
        btMacItem.length = strlen(get_device_bt_mac_ptr());
        btMacItem.value  = ( uint8_t * )get_device_bt_mac_ptr();
        tlv_item_add_brother(&rootItem, &btMacItem);
    }

    if (NULL != app_interconnection_tlv_get_item_with_type(ptrItem, TYPE_DEVICE_IMEI))
    {
        memset(&imeiItem, 0, sizeof(TLV_ITEM_T));
        imeiItem.type   = TYPE_DEVICE_IMEI;
        imeiItem.length = strlen(get_device_imei_ptr());
        imeiItem.value  = ( uint8_t * )get_device_imei_ptr();
        tlv_item_add_brother(&rootItem, &imeiItem);
    }

    memset(&softwareVersionItem, 0, sizeof(TLV_ITEM_T));
    softwareVersionItem.type   = TYPE_DEVICE_SOFTWARE_VERSION;
    softwareVersionItem.length = strlen(get_software_version_ptr());
    softwareVersionItem.value  = ( uint8_t * )get_software_version_ptr();
    tlv_item_add_brother(&rootItem, &softwareVersionItem);

    if (NULL != app_interconnection_tlv_get_item_with_type(ptrItem, TYPE_DEVICE_OPEN_SOURCE_VERSION))
    {
        memset(&openSourceVersionItem, 0, sizeof(TLV_ITEM_T));
        openSourceVersionItem.type   = TYPE_DEVICE_OPEN_SOURCE_VERSION;
        openSourceVersionItem.length = strlen(get_open_source_version_ptr());
        openSourceVersionItem.value  = ( uint8_t * )get_open_source_version_ptr();
        tlv_item_add_brother(&rootItem, &openSourceVersionItem);
    }

    if (NULL != app_interconnection_tlv_get_item_with_type(ptrItem, TYPE_DEVICE_SN))
    {
        memset(&snItem, 0, sizeof(TLV_ITEM_T));
        snItem.type   = TYPE_DEVICE_SN;
        snItem.length = strlen(get_sn_ptr());
        snItem.value  = ( uint8_t * )get_sn_ptr();
        tlv_item_add_brother(&rootItem, &snItem);
    }

    if (NULL != app_interconnection_tlv_get_item_with_type(ptrItem, TYPE_DEVICE_MODEL_ID))
    {
        memset(&modelIDItem, 0, sizeof(TLV_ITEM_T));
        modelIDItem.type   = TYPE_DEVICE_MODEL_ID;
        modelIDItem.length = strlen(get_model_id_ptr());
        modelIDItem.value  = ( uint8_t * )get_model_id_ptr();
        tlv_item_add_brother(&rootItem, &modelIDItem);
    }

    if (NULL != app_interconnection_tlv_get_item_with_type(ptrItem, TYPE_DEVICE_EMMC_ID))
    {
        memset(&emmcItem, 0, sizeof(TLV_ITEM_T));
        emmcItem.type   = TYPE_DEVICE_EMMC_ID;
        emmcItem.length = strlen(get_device_emmc_ptr());
        emmcItem.value  = ( uint8_t * )get_device_emmc_ptr();
        tlv_item_add_brother(&rootItem, &emmcItem);
    }

    if (NULL != app_interconnection_tlv_get_item_with_type(ptrItem, TYPE_DEVICE_NAME))
    {
        memset(&nameItem, 0, sizeof(TLV_ITEM_T));
        nameItem.type   = TYPE_DEVICE_NAME;
        nameItem.length = strlen(get_device_name_ptr());
        nameItem.value  = ( uint8_t * )get_device_name_ptr();
        tlv_item_add_brother(&rootItem, &nameItem);
    }

    pack_and_send_frame(&rootItem, CCMP_MODULE_DEVICE_MANAGEMENT, CCMP_DEVICE_COMMAND_ID_DEVICE_INFO);

    LOG_FUNC_OUT();
}

static void device_management_service_event_handler(uint8_t commandId, TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    // null pointer check
    if (NULL == ptrItem)
    {
        LOG_ERR("item pointer is null @%s", __func__);
        return;
    }

    switch (commandId)
    {
    case CCMP_DEVICE_COMMAND_ID_LINK_PARAM:
        LOG_DBG("link param received!");
        link_param_command_handler(ptrItem);
        break;

    case CCMP_DEVICE_COMMAND_ID_SERVICE_ABILITY:
        LOG_DBG("service ability command received!");
        service_ability_command_handler(ptrItem);
        break;

    case CCMP_DEVICE_COMMAND_ID_COMMAND_ABILITY:
        LOG_DBG("command ability command received!");
        command_ability_command_handler(ptrItem);
        break;

    case CCMP_DEVICE_COMMAND_ID_TIME_FORMAT:
        LOG_DBG("time format command received!");
        time_format_command_handler(ptrItem);
        break;

    case CCMP_DEVICE_COMMAND_ID_SET_DEVICE_TIME:
        LOG_DBG("set device time command received!");
        set_device_time_command_handler(ptrItem);
        break;

    case CCMP_DEVICE_COMMAND_ID_GET_DEVICE_TIME:
        LOG_DBG("get device time command received!");
        get_device_time_command_handler(ptrItem);
        break;

    case CCMP_DEVICE_COMMAND_ID_DEVICE_INFO:
        LOG_DBG("device info command received!");
        device_info_command_handler(ptrItem);
        break;

    case CCMP_DEVICE_COMMAND_ID_GET_MOBILE_BATT:
        LOG_DBG("get mobile battery level command received!");
        get_mobile_batt_command_handler(ptrItem);
        break;

    default:
        LOG_MSG("command ID not defined!");
        // app_interconnection_response_error_code(CCMP_MODULE_DEVICE_MANAGEMENT, CCMP_GEN_ERR_CODE_COMMAND_UNSUPPORTED, CCMP_MODULE_DEVICE_MANAGEMENT, commandId);
        break;
    }

    LOG_FUNC_OUT();
}
/**
 * @brief
 *
 * device management service related handler----end
 *
 */

/**
 * @brief
 *
 * ota service related handler----start
 *
 */

void app_interconnection_ota_apply_update_data(uint32_t fileOffset, uint8_t fileBitmap, uint32_t fileLength)
{
    LOG_FUNC_IN();
    TRACE("ask for update firmware offset is %d, length is %d, bitmap is %d", fileOffset, fileLength, fileBitmap);

    uint32_t otaFileOffset;
    uint32_t otaFileLength;
    uint8_t  otaFileBitmap;

    TLV_ITEM_T otaFileBitmapItem;
    TLV_ITEM_T otaFileLengthItem;

    otaFileOffset = uint32_convert_endian(fileOffset);
    memset(&rootItem, 0, sizeof(TLV_ITEM_T));
    rootItem.type   = TYPE_FILE_OFFSET;
    rootItem.length = FILE_OFFSET_LENGTH;
    rootItem.value  = ( uint8_t * )&(otaFileOffset);

    otaFileLength = uint32_convert_endian(fileLength);
    memset(&otaFileLengthItem, 0, sizeof(TLV_ITEM_T));
    otaFileLengthItem.type   = TYPE_FILE_LENGTH;
    otaFileLengthItem.length = FILE_LENGTH;
    otaFileLengthItem.value  = ( uint8_t * )&(otaFileLength);
    tlv_item_add_brother(&rootItem, &otaFileLengthItem);

    otaFileBitmap = fileBitmap;
    memset(&otaFileBitmapItem, 0, sizeof(TLV_ITEM_T));
    otaFileBitmapItem.type   = TYPE_FILE_BITMAP;
    otaFileBitmapItem.length = FILE_BITMAP_LENGTH;
    otaFileBitmapItem.value  = ( uint8_t * )&(otaFileBitmap);
    tlv_item_add_brother(&rootItem, &otaFileBitmapItem);

    pack_and_send_frame(&rootItem, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_DEVICE_REPORT);

    LOG_FUNC_OUT();
}

static void ota_update_control_handler(uint8_t *ptrData, uint32_t dataLength)
{
    LOG_FUNC_IN();

    bool                       pass    = false;
    CCMP_COMMUNICATION_HEAD_T *headPtr = ( CCMP_COMMUNICATION_HEAD_T * )ptrData;

    // sof check
    if (headPtr->sof != CCMP_SOF_VALUE)
    {
        LOG_MSG("update data packet sof value error.");
        pass = true;
    }

    // data length check
    LOG_MSG("length is %d", uint16_convert_endian(headPtr->length));
    uint16_t frameLen = uint16_convert_endian(headPtr->length) + CCMP_PAYLOAD_OFFSET + CRC16_LENGTH;
    if (frameLen != dataLength)
    {
        LOG_MSG("update data packet data length error.");
        pass = true;
    }

    // control flag check
    if (headPtr->ccmpControl != CCMP_CONTROL)
    {
        LOG_MSG("update data packet control error. ccmpControl is %d", headPtr->ccmpControl);
        pass = true;
    }

    // crc check
    uint16_t *rawDataPtr  = ( uint16_t * )&ptrData[dataLength - 2];
    uint16_t  receivedCrc = uint16_convert_endian(*rawDataPtr);
    LOG_MSG("received crc is 0x%04x", receivedCrc);
    uint16_t crc16 = app_interconn_calculate_ccmp_crc16(ptrData, dataLength - 2);
    LOG_MSG("local calculated crc is 0x%04x", crc16);

    if (receivedCrc != crc16)
    {
        LOG_MSG("update data packet crc error.");
        pass = true;
    }

    LOG_MSG("value of pass is %d", pass ? 1 : 0);
    if (app_interconnection_ota_is_image_info_received())
    {
        app_interconnection_ota_fw_data_received_handler(headPtr->item, (dataLength - CCMP_ITEM_OFFSET - CRC16_LENGTH), pass);
    }
    else
    {
        app_interconnection_ota_image_info_received_handler(headPtr->item, (dataLength - CCMP_ITEM_OFFSET - CRC16_LENGTH));
    }

    LOG_FUNC_OUT();
}

void app_interconnection_ota_package_validity_response(bool validity)
{
    LOG_FUNC_IN();

    uint8_t val = 0;

    if (validity)
    {
        val = 1;
    }
    else
    {
        val = 0;
    }

    memset(&rootItem, 0, sizeof(TLV_ITEM_T));
    rootItem.type   = PACKAGE_VALIDITY;
    rootItem.length = 0x01;
    rootItem.value  = &val;

    pack_and_send_frame(&rootItem, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_PACKAGE_VALIDITY);

    LOG_FUNC_OUT();
}

/**
 * @brief : response the update permission command of mobile
 *
 */
static void ota_update_permission_response(void)
{
    LOG_FUNC_IN();

    uint32_t   errorCode        = 0;
    uint8_t    batteryThreshold = 0;
    TLV_ITEM_T battItem;

    memset(&rootItem, 0, sizeof(TLV_ITEM_T));

    rootItem.type   = CCMP_GENERAL_USED_ERROR_CODE_TYPE;
    rootItem.length = CCMP_GENERAL_USED_ERROR_CODE_LEN;

    errorCode      = get_update_permission_error_code();
    rootItem.value = ( uint8_t * )&errorCode;

    memset(&battItem, 0, sizeof(TLV_ITEM_T));

    battItem.type    = TYPE_BATTERY_THRESHOLD;
    battItem.length  = 0x01;
    batteryThreshold = app_interconnection_ota_get_battery_threshold();
    battItem.value   = &batteryThreshold;

    tlv_item_add_brother(&rootItem, &battItem);
    pack_and_send_frame(&rootItem, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_UPDATE_PERMISSION);
}

static void ota_update_permission_command_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();
    if (NULL == ptrItem)
    {
        LOG_MSG("item pointer is null @%s", __func__);
        // response error code to mobile phone
        app_interconnection_response_error_code(CCMP_MODULE_OTA, CCMP_OTA_ERR_CODE_ILLEGAL_PARM, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_UPDATE_PERMISSION);
        return;
    }

    TLV_ITEM_T *itemPtr = NULL;

    itemPtr = app_interconnection_tlv_get_item_with_type(ptrItem, TYPE_PACKET_VERSION);
    if (NULL != itemPtr)
    {
        LOG_MSG("received software version length is %d", itemPtr->length);
        LOG_MSG("received software version is:");
        LOG_DUMP("0x%02x ", itemPtr->value, itemPtr->length);

        char *localVersion = get_software_version_ptr();
        if (memcmp(localVersion, itemPtr->value, itemPtr->length))
        {
            LOG_MSG("NEED TO UPDATE!!!");
            app_interconnection_ota_set_update_flag(true);
        }
        else
        {
            LOG_MSG("NOT NEED TO UPDATE!!!");
            app_interconnection_ota_set_update_flag(false);

            app_interconnection_response_error_code(CCMP_MODULE_OTA, CCMP_OTA_ERR_CODE_UPDATE_DENIED, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_UPDATE_PERMISSION);
            return;
        }
    }

    itemPtr = app_interconnection_tlv_get_item_with_type(ptrItem, TYPE_COMPONENT_SIZE);
    if (NULL != itemPtr)
    {
        uint16_t componentSize = 0;
        app_interconnection_tlv_get_big_endian_u16_from_item(itemPtr, &componentSize);
        LOG_MSG("received component size is %d", componentSize);
    }

    itemPtr = app_interconnection_tlv_get_item_with_type(ptrItem, TYPE_OTA_WORK_MODE);
    if (NULL != itemPtr)
    {
        uint32_t workMode = 0;
        app_interconnection_tlv_get_big_endian_u32_from_item(itemPtr, &workMode);
        LOG_MSG("received work mode is %d", workMode);
    }

    ota_update_permission_response();
    LOG_FUNC_OUT();
}

static void ota_param_negotiation_response(void)
{
    LOG_FUNC_IN();

    uint16_t app_wait_timeout;
    uint16_t device_restart_timeout;
    uint16_t ota_unit_size;
    uint16_t ota_interval;
    uint8_t  ack_enable;

    TLV_ITEM_T tlv_device_restart_timeout;
    TLV_ITEM_T tlv_ota_unit_size;
    TLV_ITEM_T tlv_interval;
    TLV_ITEM_T tlv_ackenable;
    uint16_t   logic_frame_len;

    app_wait_timeout = uint16_convert_endian(APP_WAIT_TIMEOUT);
    memset(&rootItem, 0, sizeof(TLV_ITEM_T));
    rootItem.type   = TYPE_APP_WAIT_TIMEOUT;
    rootItem.length = 0x02;
    rootItem.value  = ( uint8_t * )&(app_wait_timeout);

    device_restart_timeout = uint16_convert_endian(DEVICE_RESTART_TIMEOUT);
    memset(&tlv_device_restart_timeout, 0, sizeof(TLV_ITEM_T));
    tlv_device_restart_timeout.type   = TYPE_RESTART_TIMEOUT;
    tlv_device_restart_timeout.length = 0x02;
    tlv_device_restart_timeout.value  = ( uint8_t * )&(device_restart_timeout);
    tlv_item_add_brother(&rootItem, &tlv_device_restart_timeout);

    logic_frame_len = MTU_MAX_LENGTH;
    ota_unit_size   = uint16_convert_endian(logic_frame_len);

    LOG_MSG("ota_unit_size is %d", logic_frame_len);
    memset(&tlv_ota_unit_size, 0, sizeof(TLV_ITEM_T));
    tlv_ota_unit_size.type   = TYPE_OTA_UNIT_SIZE;
    tlv_ota_unit_size.length = 0x02;
    tlv_ota_unit_size.value  = ( uint8_t * )&(ota_unit_size);
    tlv_item_add_brother(&rootItem, &tlv_ota_unit_size);

    ota_interval = uint16_convert_endian(INTERVAL);
    memset(&tlv_interval, 0, sizeof(TLV_ITEM_T));
    tlv_interval.type   = TYPE_INTERVAL;
    tlv_interval.length = 0x02;
    tlv_interval.value  = ( uint8_t * )&(ota_interval);
    tlv_item_add_brother(&rootItem, &tlv_interval);

    ack_enable = 0;
    memset(&tlv_ackenable, 0, sizeof(TLV_ITEM_T));
    tlv_ackenable.type   = TYPE_ACK_ENABLE;
    tlv_ackenable.length = 0x01;
    tlv_ackenable.value  = &(ack_enable);
    tlv_item_add_brother(&rootItem, &tlv_ackenable);

    pack_and_send_frame(&rootItem, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_PARAM_NEGOTIATION);

    LOG_FUNC_OUT();
}

static void ota_param_negotiation_event_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    // // null pointer check
    // if(NULL == ptrItem)
    // {
    //     // response error code to mobile
    //     app_interconnection_response_error_code(CCMP_MODULE_OTA, CCMP_OTA_ERR_CODE_ILLEGAL_PARM, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_PARAM_NEGOTIATION);
    // }
    // else
    // {
    // report update param to mobile
    ota_param_negotiation_response();
    // }

    LOG_FUNC_OUT();
}

static void ota_device_report_event_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    // ack is enabled
    INTERCONN_OTA_ENV_T *otaEnvPtr = app_interconnection_ota_get_env_ptr();
    if (otaEnvPtr->ackEnable)
    {
        if (NULL == ptrItem)
        {
            app_interconnection_response_error_code(CCMP_MODULE_OTA, CCMP_OTA_ERR_CODE_ILLEGAL_PARM, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_DEVICE_REPORT);
        }
    }
    else
    {
        app_interconnection_response_error_code(CCMP_MODULE_OTA, CCMP_OTA_ERR_CODE_ILLEGAL_PARM, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_DEVICE_REPORT);
    }

    LOG_FUNC_OUT();
}

void app_interconnection_package_size_report_handler(uint32_t packageSize, uint32_t receivedSize)
{
    LOG_FUNC_IN();

    TLV_ITEM_T receivedSizeItem;
    uint32_t   totalSize = uint32_convert_endian(packageSize);
    uint32_t   recSize   = uint32_convert_endian(receivedSize);

    memset(&rootItem, 0, sizeof(TLV_ITEM_T));
    rootItem.type   = TYPE_VALID_SIZE;
    rootItem.length = 4;
    rootItem.value  = ( uint8_t * )&totalSize;

    memset(&receivedSizeItem, 0, sizeof(TLV_ITEM_T));
    receivedSizeItem.type   = TYPE_FILE_SIZE;
    receivedSizeItem.length = 4;
    receivedSizeItem.value  = ( uint8_t * )&recSize;
    tlv_item_add_brother(&rootItem, &receivedSizeItem);

    pack_and_send_frame(&rootItem, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_PACKAGE_SIZE);

    LOG_FUNC_OUT();
}

static void ota_package_validity_event_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    app_interconnection_ota_finished_handler();

    LOG_FUNC_OUT();
}

static void ota_update_cancel_event_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    // report error code to mobile
    app_interconnection_response_error_code(CCMP_MODULE_OTA, CCMP_OTA_ERR_CODE_BT_UPDATE_FAILURE, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_UPDATE_CANCEL);

    app_interconnection_ota_finished_handler();

    LOG_FUNC_OUT();
}

static void ota_update_status_notify_event_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    //uint32_t errorCode = 0;

    if (NULL == ptrItem)
    {
        LOG_MSG("item pointer is null @%s", __func__);
        return;
    }

    TLV_ITEM_T *itemPtr = NULL;

    itemPtr = app_interconnection_tlv_get_item_with_type(ptrItem, UPDATE_STATUS_NOTIFY);

    // incoming data check
    if (NULL != itemPtr)
    {
        if (UPDATE_PARAM_RECEIVE_SUCCESS == ptrItem->value[0])
        {
            if (app_interconnection_ota_get_update_flag())
            {
                // init ota environment variable
                app_interconn_ota_handler_init();
                app_interconnection_ota_set_ota_in_progress(1);

                // request ota image info data
                app_interconnection_ota_apply_update_data(0, 0, IMAGE_INFO_SIZE);
            }
            else
            {
                // reply mobile that ota is not allowed
                app_interconnection_response_error_code(CCMP_MODULE_OTA, CCMP_OTA_ERR_CODE_UPDATE_DENIED, CCMP_MODULE_OTA, CCMP_OTA_ERR_CODE_DATA_VERIFY_FAILURE);
            }
        }
        else
        {
            LOG_ERR("mobile receive update data failed!");
            ota_param_negotiation_response();
        }
    }
    else
    {
        // TODO: reponse error code
    }

    LOG_FUNC_OUT();
}

/**
 * @brief : handler of ota update request of ota service
 *
 */
void ota_update_request(void)
{
    LOG_FUNC_IN();

    if (app_interconnection_ota_get_update_flag())
    {
        LOG_DBG("send update request to mobile!!!");
        uint8_t request = 1;  // request to update

        memset(&rootItem, 0, sizeof(TLV_ITEM_T));
        rootItem.type   = UPDATE_REQUEST;
        rootItem.length = 0x01;
        rootItem.value  = &request;

        pack_and_send_frame(&rootItem, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_UPDATE_REQUEST);
    }
    else
    {
        LOG_DBG("software version is lastest, no need to send update request.");
    }

    LOG_FUNC_OUT();
}

/**
 * @brief : handler of update request command in OTA service
 *
 * @param ptrItem : pointer of tlv structured data
 */
static void ota_update_request_event_handler(TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    if (NULL == ptrItem)
    {
        LOG_MSG("item pointer is null @%s", __func__);
        // TODO: response error code to mobile phone
        return;
    }

    TLV_ITEM_T *itemPtr = NULL;

    itemPtr = app_interconnection_tlv_get_item_with_type(ptrItem, UPDATE_REQUEST);
    if (NULL != itemPtr)
    {
        // TODO: parse error code
        uint32_t errorCode = 0;
        app_interconnection_tlv_get_big_endian_u32_from_item(itemPtr, &errorCode);
        LOG_MSG("received error code is %d", errorCode);
    }

    LOG_FUNC_OUT();
}

/**
 * @brief : event handler of ota service
 * parse the data packed in TLV structure according to command ID of OTA service
 *
 * @param commandId : command ID of OTA service
 * @param ptrItem : pointer of received tlv structured data
 */
static void ota_service_event_handler(uint8_t commandId, TLV_ITEM_T *ptrItem)
{
    // null pointer check
    if (NULL == ptrItem)
    {
        LOG_MSG("item pointer is null @%s", __func__);
    }

    switch (commandId)
    {
    case CCMP_OTA_COMMAND_ID_UPDATE_PERMISSION:
        ota_update_permission_command_handler(ptrItem);
        break;

    case CCMP_OTA_COMMAND_ID_PARAM_NEGOTIATION:
        ota_param_negotiation_event_handler(ptrItem);
        break;

    case CCMP_OTA_COMMAND_ID_DEVICE_REPORT:
        ota_device_report_event_handler(ptrItem);
        break;

        // case CCMP_OTA_COMMAND_ID_FIRMWARE_DATA:
        //     ota_firmware_event_handler(ptrItem);
        //     break;

    case CCMP_OTA_COMMAND_ID_PACKAGE_SIZE:
        break;

    case CCMP_OTA_COMMAND_ID_PACKAGE_VALIDITY:
        ota_package_validity_event_handler(ptrItem);
        break;

    case CCMP_OTA_COMMAND_ID_UPDATE_STATUS_REPORT:
        break;

    case CCMP_OTA_COMMAND_ID_UPDATE_CANCEL:
        ota_update_cancel_event_handler(ptrItem);
        break;

    case CCMP_OTA_COMMAND_ID_UPDATE_STATUS_NOTIFY:
        ota_update_status_notify_event_handler(ptrItem);
        break;

    case CCMP_OTA_COMMAND_ID_UPDATE_REQUEST:
        ota_update_request_event_handler(ptrItem);
        break;

    default:
        LOG_MSG("command id is not defined!!!");
        app_interconnection_response_error_code(CCMP_MODULE_GENERAL, CCMP_GEN_ERR_CODE_COMMAND_UNSUPPORTED, CCMP_MODULE_DEVICE_MANAGEMENT, commandId);
        break;
    }
}
/**
 * @brief
 *
 * ota related service handler----end
 *
 */

/**
 * @brief : event handler of midware service
 *
 * @param commandId
 * @param ptrItem : pointer of tlv structured data
 */
static void midware_service_event_handler(uint8_t commandId, TLV_ITEM_T *ptrItem)
{
    uint32_t    errorCode = 0;
    uint32_t *  ptrError  = NULL;
    TLV_ITEM_T *itemPtr   = NULL;

    switch (commandId)
    {
    case CCMP_MIDWARE_COMMAND_ID_HUAWEI_MUSIC:
        itemPtr   = app_interconnection_tlv_get_item_with_type(ptrItem, CCMP_GENERAL_USED_ERROR_CODE_TYPE);
        ptrError  = ( uint32_t * )itemPtr->value;
        errorCode = uint32_convert_endian(*ptrError);
        LOG_DBG("error code is %d", errorCode);
        switch (errorCode)
        {
        case CCMP_GEN_ERR_CODE_SUCCESS:
            LOG_DBG("music handle success.");
            break;

        case CCMP_MIDWARE_ERR_CODE_NO_HUAWEI_MUSIC:
            LOG_DBG("no huawei music application.");
            break;

        case CCMP_MIDWARE_ERR_CODE_USER_NOT_SIGNED:
            LOG_DBG("no user is signed in.");
            break;

        case CCMP_MIDWARE_ERR_CODE_NO_NETWORK:
            LOG_DBG("no network is avaliable.");
            break;

        case CCMP_MIDWARE_ERR_CODE_ADD_FAILURE:
            LOG_DBG("music add  to favorite failure.");
            break;

        case CCMP_MIDWARE_ERR_CODE_NO_MUSIC:
            LOG_DBG("no music to play.");
            break;

        case CCMP_MIDWARE_ERR_CODE_PLAYING_FAVORITE:
            LOG_DBG("playing favorite music now.");
            break;

        case CCMP_MIDWARE_ERR_CODE_PLAYING_FAILURE:
            LOG_DBG("playing music failure.");
            break;

        default:
            LOG_DBG("error code not defined.");
            break;
        }
        break;

    default:
        LOG_MSG("command not defined.");
        app_interconnection_response_error_code(CCMP_MODULE_GENERAL, CCMP_GEN_ERR_CODE_COMMAND_UNSUPPORTED, CCMP_MODULE_MIDWARE_SERVICE, commandId);
        break;
    }
}

/**
 * @brief : ccmp communication data received handler
 *
 * @param serviceID
 * @param commandID
 * @param ptrItem
 */
static void ccmp_protocol_event_handler(uint8_t serviceId, uint8_t commandId, TLV_ITEM_T *ptrItem)
{
    LOG_FUNC_IN();

    switch (serviceId)
    {
    case CCMP_MODULE_DEVICE_MANAGEMENT:
        device_management_service_event_handler(commandId, ptrItem);
        break;

    case CCMP_MODULE_OTA:
        ota_service_event_handler(commandId, ptrItem);
        break;

    case CCMP_MODULE_MIDWARE_SERVICE:
        midware_service_event_handler(commandId, ptrItem);
        break;

    default:
        LOG_MSG("ccmp service %d not defined!", serviceId);
        app_interconnection_response_error_code(CCMP_MODULE_GENERAL, CCMP_GEN_ERR_CODE_COMMAND_UNSUPPORTED, serviceId, commandId);
        break;
    }

    LOG_FUNC_OUT();
}

/**
 * @brief ： check the validity of received data and parse the receive data into
 * tlv structure
 *
 * @param ptrData
 * @param dataLength
 */
static void ccmp_frame_received_callback(uint8_t *ptrData, uint32_t dataLength, uint8_t preParseRes)
{
    INTERCONN_OTA_ENV_T *      otaEnvPtr = app_interconnection_ota_get_env_ptr();
    CCMP_COMMUNICATION_HEAD_T *headPtr   = ( CCMP_COMMUNICATION_HEAD_T * )ptrData;
    //uint32_t errorCode = 0;

    // special case, no need to parse tlv
    if (((CCMP_MODULE_OTA == headPtr->serviceID) && (CCMP_OTA_COMMAND_ID_FIRMWARE_DATA == headPtr->commandID)) || (0 != preParseRes && otaEnvPtr->isOTAInProgress))
    {
        ota_update_control_handler(ptrData, dataLength);
    }
    else if (0 == preParseRes)  // parse tlv tree
    {
        TLV_ITEM_T *tlvTree = app_interconnection_tlv_item_tree_malloc(headPtr->item,
                                                                       (dataLength - CCMP_ITEM_OFFSET - CRC16_LENGTH));
        ccmp_protocol_event_handler(headPtr->serviceID, headPtr->commandID, tlvTree);

        if (NULL != tlvTree)
        {
            INTERCONNECTION_FREE(tlvTree);
            tlvTree = NULL;
        }
    }
}

uint8_t app_interconnection_ccmp_data_received_callback(uint8_t *data, uint32_t dataLength)
{
    uint8_t *                  ptrData = data;
    uint8_t                    result  = 0;
    CCMP_COMMUNICATION_HEAD_T *headPtr = NULL;

    if (NULL == ptrData)
    {
        LOG_ERR("data pointer error!!!");
        return INTERCONNECTION_STATUS_ERROR;
    }

    LOG_MSG("ccmp receive cmd data length is %d", dataLength);  //receive command;
    // DUMP8("%02x ", ptrData, dataLength);

    headPtr = ( CCMP_COMMUNICATION_HEAD_T * )ptrData;

    // sof check
    if (headPtr->sof != CCMP_SOF_VALUE)
    {
        LOG_MSG("ccmp communication sof value error.");
        result = 1;
    }

    // data length check
    uint16_t frameLen = uint16_convert_endian(headPtr->length) + CCMP_PAYLOAD_OFFSET + CRC16_LENGTH;
    LOG_MSG("frame length is %d", frameLen);
    if (dataLength != frameLen)
    {
        result = 2;
    }

    // control flag check
    if (headPtr->ccmpControl != CCMP_CONTROL)
    {
        LOG_MSG("ccmp communication control error. ccmpControl is %d", headPtr->ccmpControl);
        result = 3;
    }

    // crc check
    uint16_t *rawDataPtr  = ( uint16_t * )&ptrData[frameLen - 2];
    uint16_t  receivedCrc = uint16_convert_endian(*rawDataPtr);
    LOG_MSG("received crc is 0x%04x", receivedCrc);
    uint16_t crc16 = app_interconn_calculate_ccmp_crc16(ptrData, frameLen - 2);
    LOG_MSG("local calculated crc is 0x%04x", crc16);

    if (receivedCrc != crc16)
    {
        LOG_MSG("ccmp communication crc error.");
        result = 4;
    }

    ccmp_frame_received_callback(( uint8_t * )headPtr, frameLen, result);

    return INTERCONNECTION_STATUS_SUCCESS;
}

void app_interconnection_ccmp_negotiation_data_send(uint8_t *param, uint8_t paramLen)
{
    app_interconnection_send_data_via_spp(app_interconnection_get_spp_client(), param, paramLen);
}

void app_interconnection_ccmp_data_send(uint8_t *param, uint8_t paramLen)
{
    app_interconnection_send_data_via_ccmp(param, paramLen);
}

#endif  // #ifdef __INTERCONNECTION__