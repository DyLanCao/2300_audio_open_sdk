/**
 * @file app_interconnection_customer_realize.cpp
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
#include "hal_trace.h"
#include "app_interconnection_customer_realize.h"
#include "app_interconnection_logic_protocol.h"
#include "app_interconnection_ota.h"
#include "app_battery.h"

static uint8_t LINK_PARM_PROTOCOL_VERSION = 2;

static uint8_t DEVICE_TYPE = 22;

SUPPORT_SERVICE_T supportList[] = {
    {CCMP_MODULE_DEVICE_MANAGEMENT, 1},
    {CCMP_MODULE_NOTIFICATION, 1},
    {CCMP_MODULE_MUSIC, 1},
    {CCMP_MODULE_OTA, 1},
    {CCMP_MODULE_MUSIC_MANAGEMENT, 1},
    {CCMP_MODULE_BT_TEST, 1},
    {CCMP_MODULE_MCU_TEST, 1},
    {CCMP_MODULE_MIDWARE_SERVICE, 1},
};

uint8_t* get_link_param_protocol_version(void)
{
    return &LINK_PARM_PROTOCOL_VERSION;
}

bool is_service_support(uint8_t serviceID)
{
    uint8_t supportListLength = sizeof(supportList) / sizeof(supportList[0]);
    for (uint8_t i = 0; i < supportListLength; i++)
    {
        if(serviceID == supportList[i].serviceID && 1 == supportList[i].support)
        {
            return true;
        }
    }

    return false;
}

char* get_bt_version_ptr(void)
{
    return (char*)BT_VERSION;
}

uint8_t* get_device_type_ptr(void)
{
    return &DEVICE_TYPE;
}

char* get_hardware_version_ptr(void)
{
    return (char*)DEVICE_HARDWARE_VERSION;
}

char* get_phone_number_ptr(void)
{
    return (char*)PHONE_NUMBER;
}

char* get_device_bt_mac_ptr(void)
{
    return (char*)DEVICE_BT_MAC;
}

char* get_device_imei_ptr(void)
{
    return (char*)DEVICE_IMEI;
}

char* get_software_version_ptr(void)
{
    return (char*)SOFTWARE_VERSION;
}


char* get_open_source_version_ptr(void)
{
    return (char*)DEVICE_OPEN_SOURCE_VERSION;
}


char* get_sn_ptr(void)
{
    return (char*)DEVICE_SN;
}


char* get_model_id_ptr(void)
{
    return (char*)DEVICE_MODEL_ID;
}

char* get_device_emmc_ptr(void)
{
    return (char*)DEVICE_EMMC;
}

char* get_device_name_ptr(void)
{
    return (char*)DEVICE_NAME;
}


char* get_summary_value_ptr()
{
    return (char*)CCMP_SUMMARY_VALUE;
}

char* get_business_tag_ptr()
{
    return (char*)BUSINESS_TAG;
}

/**
 * @brief Get the update permission error code object
 * creat a error code according to the device status
 * 
 * @return uint32_t 
 */
uint32_t get_update_permission_error_code()
{
    uint32_t errorCode = 0;

    uint8_t batteryLevel = 0;
    app_battery_get_info(NULL, &batteryLevel, NULL);
    batteryLevel &= 0x7f;

    if(batteryLevel < app_interconnection_ota_get_battery_threshold())
    {
        app_interconnection_ccmp_creat_error_code(CCMP_MODULE_OTA, CCMP_OTA_ERR_CODE_LOW_BATTERY, &errorCode);
    }
    else
    {
        app_interconnection_ccmp_creat_error_code(CCMP_MODULE_GENERAL, CCMP_GENERAL_USED_ERROR_CODE_NO_ERROR, &errorCode);
    }

    return errorCode;
}


unsigned long _crctbl[] = {
0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t crc32_image(uint32_t crc, uint8_t* input, uint32_t len)
{  
    uint32_t i;
    uint32_t crc_val = crc;
    uint8_t index;
    uint8_t *p = input;

    for (i = 0; i < len; i++)
    {
        index = (*p ^ crc_val);
        crc_val = (crc_val >> 8) ^ _crctbl[index];
        p++;
    }

    return crc_val;
}


uint8_t CrcCheck(uint32_t rec_packet_crc, uint8_t *buf, uint32_t lens)
{
    uint32_t crc_cal = 0xffffffff;
    crc_cal = crc32_image(crc_cal, buf, lens);
    if(rec_packet_crc == crc_cal)
    {
        return INTERCONNECTION_STATUS_SUCCESS;
    }
    TRACE("received crc is 0x%08x", rec_packet_crc);
    TRACE("local crc is 0x%08x", crc_cal);
    return INTERCONNECTION_STATUS_ERROR;
}


uint8_t customer_realize_image_info_receive_finished_hook()
{
    uint8_t mask = 0;
    uint8_t lastRequestPacketNum = 0;
    INTERCONN_OTA_ENV_T *otaEnvPtr = app_interconnection_ota_get_env_ptr();
    REC_HEAD_PACKET_T *headPtr = (REC_HEAD_PACKET_T *)otaEnvPtr->imageInfoPtr;

    // null pointer check and handler
    if(NULL == headPtr)
    {
        TRACE("image info ptr is null.");
        return INTERCONNECTION_STATUS_ERROR;
    }

#ifdef H2_FOR_TEST
    // used for h2
    headPtr->h_header =uint16_convert_endian(headPtr->h_header);
    headPtr->h_head_lens = uint16_convert_endian(headPtr->h_head_lens);
    headPtr->h_headset_addr_offset = uint32_convert_endian(headPtr->h_headset_addr_offset);
    headPtr->h_headset_data_lens = uint32_convert_endian(headPtr->h_headset_data_lens);
    headPtr->h_headset_crc = uint32_convert_endian(headPtr->h_headset_crc);
    headPtr->h_data_crc = uint32_convert_endian(headPtr->h_data_crc);
#endif

    // fixed header check
    if(FIXED_IMAGE_INFO_HEADER != headPtr->h_header)
    {
        TRACE("received fixed header is 0x%04x", headPtr->h_header);
        return INTERCONNECTION_STATUS_ERROR;
    }
    
    // image info size check
    if(sizeof(REC_HEAD_PACKET_T) != headPtr->h_head_lens)
    {
        TRACE("received head length is %d, local length is %d", headPtr->h_head_lens, sizeof(REC_HEAD_PACKET_T));
        return INTERCONNECTION_STATUS_ERROR;
    }

    // component check
    if(OTA_COMPONENT_ID_HEAD_SET != headPtr->h_headset_firmware_flag)
    {
        TRACE("update component is %d", headPtr->h_headset_firmware_flag);
        return INTERCONNECTION_STATUS_ERROR;
    }

    // component digital signature

    // image info crc check
#ifndef H2_FOR_TEST
    if(INTERCONNECTION_STATUS_SUCCESS != CrcCheck(headPtr->h_data_crc, (uint8_t*)headPtr, headPtr->h_head_lens-4))
    {
        TRACE("crc check of header is error");
        return INTERCONNECTION_STATUS_ERROR;
    }
#endif

    // update component image offset in update packet
    otaEnvPtr->addressOfStartApplyImage = headPtr->h_headset_addr_offset;

    // update component image size
    otaEnvPtr->totalImageSize = headPtr->h_headset_data_lens;

    // update component crc value
    otaEnvPtr->crcOfImage = headPtr->h_headset_crc;

    // update upgrade way
    if(FORCE_UPGRADE != headPtr->h_upgrade_mode)
    {
        // otaEnvPtr->isUpdateNecessary = false;
    }

    otaEnvPtr->imageRequestTime = (otaEnvPtr->totalImageSize % FLASH_SECTOR_SIZE_IN_BYTES == 0) ? (otaEnvPtr->totalImageSize / FLASH_SECTOR_SIZE_IN_BYTES) : (otaEnvPtr->totalImageSize / FLASH_SECTOR_SIZE_IN_BYTES + 1);

    otaEnvPtr->imageLastRequestSize = otaEnvPtr->totalImageSize - (otaEnvPtr->imageRequestTime - 1) * FLASH_SECTOR_SIZE_IN_BYTES;
    lastRequestPacketNum = (otaEnvPtr->imageLastRequestSize % OTA_UNIT_SIZE_SPP == 0) ? (OTA_UNIT_SIZE_SPP) : (otaEnvPtr->imageLastRequestSize / OTA_UNIT_SIZE_SPP + 1);

    for (uint8_t i = 0; i < lastRequestPacketNum; i++)
    {
        mask |= 1 << i;
    }

    TRACE("last request bitmap mask is 0x%02x", mask);

    otaEnvPtr->imageLastRequestBitmapMask = mask;

    return INTERCONNECTION_STATUS_SUCCESS;
}