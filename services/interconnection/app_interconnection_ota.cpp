#ifdef __INTERCONNECTION__
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_timer.h"
#include "hal_gpio.h"
#include "hal_trace.h"
#include "string.h"
#include "crc32.h"
#include "pmu.h"
#include "hal_norflash.h"
#include "nvrecord.h"
#include "hal_cache.h"
#include "cmsis_nvic.h"
#include "hal_bootmode.h"

#include "app_battery.h"
#include "umm_malloc.h"
#include "app_interconnection.h"
#include "app_interconnection_logic_protocol.h"
#include "app_interconnection_ota.h"
#include "app_interconnection_ccmp.h"
#include "app_interconnection_customer_realize.h"


static INTERCONN_OTA_ENV_T  otaEnv;


extern uint32_t __factory_start[];
static uint32_t user_data_nv_flash_offset;

#define LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC    512
//static const char* image_info_sanity_crc_key_word = "CRC32_OF_IMAGE=";

void app_interconnection_ota_set_update_flag(bool flag)
{
    otaEnv.isUpdateNecessary = flag;
}

bool app_interconnection_ota_get_update_flag(void)
{
    return otaEnv.isUpdateNecessary;
}

uint8_t app_interconnection_ota_get_battery_threshold(void)
{
    return otaEnv.batteryThreshold;
}

uint32_t app_interconnection_ota_get_file_offset()
{
    return otaEnv.alreadyReceivedDataSizeOfImage;
}

void app_interconnection_ota_set_ota_in_progress(uint8_t value)
{
    otaEnv.isOTAInProgress = value;
}

uint8_t app_interconnection_ota_get_file_bitmap()
{
    return otaEnv.bitmap;
}

bool app_interconnection_ota_is_image_info_received(void)
{
    return otaEnv.isImageInfoReceived;
}

INTERCONN_OTA_ENV_T* app_interconnection_ota_get_env_ptr()
{
    return &otaEnv;
}

/****************/

static void ota_reboot_to_use_new_image(void)
{
    TRACE("OTA data receiving is done successfully, systerm will reboot ");
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_ENTER_HIDE_BOOT);
    hal_cmu_sys_reboot();
}

static void ota_flush_data_to_flash(uint8_t* source, uint32_t lengthToBurn, uint32_t offsetInFlashToProgram)
{
    uint32_t lock;
    uint8_t* ptrSource = source;

    TRACE_IMM("flush %d bytes to flash offset 0x%x", lengthToBurn, offsetInFlashToProgram);

    lock = int_lock_global();
    pmu_flash_write_config();

    uint32_t preBytes = (FLASH_SECTOR_SIZE_IN_BYTES - (offsetInFlashToProgram%FLASH_SECTOR_SIZE_IN_BYTES))%FLASH_SECTOR_SIZE_IN_BYTES;
    if (lengthToBurn < preBytes)
    {
        preBytes = lengthToBurn;
    }

    uint32_t middleBytes = 0;
    if (lengthToBurn > preBytes)
    {
       middleBytes = ((lengthToBurn - preBytes)/FLASH_SECTOR_SIZE_IN_BYTES*FLASH_SECTOR_SIZE_IN_BYTES);
    }
    uint32_t postBytes = 0;
    if (lengthToBurn > (preBytes + middleBytes))
    {
        postBytes = (offsetInFlashToProgram + lengthToBurn)%FLASH_SECTOR_SIZE_IN_BYTES;
    }

    TRACE("Prebytes is %d middlebytes is %d postbytes is %d", preBytes, middleBytes, postBytes);

    if (preBytes > 0)
    {
        hal_norflash_write(HAL_NORFLASH_ID_0, offsetInFlashToProgram, ptrSource, preBytes);

        ptrSource += preBytes;
        offsetInFlashToProgram += preBytes;
    }

    uint32_t sectorIndexInFlash = offsetInFlashToProgram/FLASH_SECTOR_SIZE_IN_BYTES;

    if (middleBytes > 0)
    {
        uint32_t sectorCntToProgram = middleBytes/FLASH_SECTOR_SIZE_IN_BYTES;
        for (uint32_t sector = 0;sector < sectorCntToProgram;sector++)
        {
            TRACE_IMM("flash address to be wite is 0x%08x", sectorIndexInFlash * FLASH_SECTOR_SIZE_IN_BYTES);
            hal_norflash_erase(HAL_NORFLASH_ID_0, sectorIndexInFlash * FLASH_SECTOR_SIZE_IN_BYTES, FLASH_SECTOR_SIZE_IN_BYTES);
            hal_norflash_write(HAL_NORFLASH_ID_0, sectorIndexInFlash*FLASH_SECTOR_SIZE_IN_BYTES,
                ptrSource + sector*FLASH_SECTOR_SIZE_IN_BYTES, FLASH_SECTOR_SIZE_IN_BYTES);

            sectorIndexInFlash++;
        }

        ptrSource += middleBytes;
    }

    if (postBytes > 0)
    {
        hal_norflash_erase(HAL_NORFLASH_ID_0, sectorIndexInFlash*FLASH_SECTOR_SIZE_IN_BYTES, FLASH_SECTOR_SIZE_IN_BYTES);
        hal_norflash_write(HAL_NORFLASH_ID_0, sectorIndexInFlash*FLASH_SECTOR_SIZE_IN_BYTES,
                ptrSource, postBytes);
    }

    pmu_flash_read_config();

    int_unlock(lock);

    uint32_t err_offset = memcmp(source, (uint8_t *)(OTA_FLASH_LOGIC_ADDR + offsetInFlashToProgram), lengthToBurn);
    uint32_t checkOffset = 0;
    if (err_offset)
    {
        while(checkOffset < lengthToBurn){
            if (lengthToBurn - checkOffset > 512) {
                TRACE("data in buffer:");
                DUMP8("%02x ", (uint8_t *)(source+checkOffset), 512);
                TRACE("data in flash:");
                DUMP8("%02x ", (uint8_t *)(OTA_FLASH_LOGIC_ADDR+offsetInFlashToProgram+checkOffset), 512);
                checkOffset += 512;
            }
            else
            {
                TRACE("data in buffer:");
                DUMP8("%02x ", (uint8_t *)(source+checkOffset), lengthToBurn - checkOffset);
                TRACE("data in flash:");
                DUMP8("%02x ", (uint8_t *)(OTA_FLASH_LOGIC_ADDR+offsetInFlashToProgram+checkOffset), lengthToBurn - checkOffset);
                checkOffset += lengthToBurn;
            }
        }
    }
}

// void ota_upgradeSize_log(void)
// {
//     uint32_t lock;

//     lock = int_lock_global();
//     pmu_flash_write_config();

//     if(++otaEnv.i_log >= sizeof(otaUpgradeLog.upgradeSize)/4)
//     {
//         otaEnv.i_log = 0;
//         memcpy(otaEnv.dataBufferForBurning, (uint8_t *)&otaUpgradeLog, sizeof(otaUpgradeLog.version_md5));
//         hal_norflash_erase(HAL_NORFLASH_ID_0, (uint32_t)&otaUpgradeLog, FLASH_SECTOR_SIZE_IN_BYTES);
//         hal_norflash_write(HAL_NORFLASH_ID_0, (uint32_t)&otaUpgradeLog, otaEnv.dataBufferForBurning, sizeof(otaUpgradeLog.version_md5));
//     }

//     hal_norflash_write(HAL_NORFLASH_ID_0, (uint32_t)&otaUpgradeLog.upgradeSize[otaEnv.i_log],
//                     (uint8_t*)&otaEnv.alreadyReceivedDataSizeOfImage, 4);

//     TRACE("{i_log: %d, RecSize: 0x%x, FlashWrSize: 0x%x}", otaEnv.i_log, otaEnv.alreadyReceivedDataSizeOfImage, otaUpgradeLog.upgradeSize[otaEnv.i_log]);

//     pmu_flash_read_config();
//     int_unlock(lock);
// }


// static void ota_upgradeLog_destroy(void)
// {
//     uint32_t lock;
//     lock = int_lock_global();
//     otaEnv.resume_at_breakpoint = false;
//     pmu_flash_write_config();
//     hal_norflash_erase(HAL_NORFLASH_ID_0, (uint32_t)&otaUpgradeLog, FLASH_SECTOR_SIZE_IN_BYTES);
//     pmu_flash_read_config();
//     int_unlock_global(lock);

//     TRACE("Destroyed upgrade log in flash.");
// }



static uint32_t ota_check_whole_image_crc(void)
{
    uint32_t crc32Value = 0xffffffff;


    uint32_t startFlashAddr = OTA_FLASH_LOGIC_ADDR + otaEnv.dstFlashOffsetForNewImage;

    TRACE_IMM("start word: 0x%x", *(uint32_t *)startFlashAddr);

    if (*(uint32_t *)startFlashAddr != NORMAL_BOOT)
    {
        return 0;
    }
    else
    {
        uint32_t firstWord = 0xFFFFFFFF;
        crc32Value = crc32_image(crc32Value, (uint8_t *)&firstWord, 4);
        crc32Value = crc32_image(crc32Value, (uint8_t *)((uint8_t*)startFlashAddr + 4), otaEnv.totalImageSize - 4);

        return crc32Value;
    }
}
#if 0
static int32_t find_key_word(uint8_t* targetArray, uint32_t targetArrayLen,
    uint8_t* keyWordArray,
    uint32_t keyWordArrayLen)
{
    if ((keyWordArrayLen > 0) && (targetArrayLen >= keyWordArrayLen))
    {
        uint32_t index = 0, targetIndex = 0;
        for (targetIndex = 0;targetIndex < targetArrayLen;targetIndex++)
        {
            for (index = 0;index < keyWordArrayLen;index++)
            {
                if (targetArray[targetIndex + index] != keyWordArray[index])
                {
                    break;
                }
            }

            if (index == keyWordArrayLen)
            {
                return targetIndex;
            }
        }

        return -1;
    }
    else
    {
        return -1;
    }
}
#endif

static void ota_update_boot_info(FLASH_OTA_BOOT_INFO_T* otaBootInfo)
{
    uint32_t lock;

    lock = int_lock_global();
    pmu_flash_write_config();
    hal_norflash_erase(HAL_NORFLASH_ID_0, OTA_BOOT_INFO_FLASH_OFFSET, FLASH_SECTOR_SIZE_IN_BYTES);
    hal_norflash_write(HAL_NORFLASH_ID_0, OTA_BOOT_INFO_FLASH_OFFSET, (uint8_t*)otaBootInfo, sizeof(FLASH_OTA_BOOT_INFO_T));
    pmu_flash_read_config();

    int_unlock_global(lock);
}

// uint32_t get_upgradeSize_log(void)
// {
//     int32_t *p = (int32_t*)otaUpgradeLog.upgradeSize,
//             left = 0, right = sizeof(otaUpgradeLog.upgradeSize)/4 - 1, mid;

//     if(p[0] != -1)
//     {
//         while(left < right)
//         {
//             mid = (left + right) / 2;
//             if(p[mid] == -1)
//                 right = mid - 1;
//             else
//                 left = mid + 1;
//         }
//     }
//     if(p[left]==-1)
//         left--;

//     otaEnv.i_log = left;
//     otaEnv.breakPoint = left!=-1 ? p[left] : 0;
//     otaEnv.resume_at_breakpoint = otaEnv.breakPoint?true:false;

//     TRACE("ota_env.i_log: %d", otaEnv.i_log);
//     return otaEnv.breakPoint;
// }

// void ota_version_md5_log(uint8_t version_md5[])
// {
//     uint32_t lock;
//     lock = int_lock();
//     pmu_flash_write_config();
//     hal_norflash_erase(HAL_NORFLASH_ID_0, (uint32_t)&otaUpgradeLog, FLASH_SECTOR_SIZE_IN_BYTES);
//     hal_norflash_write(HAL_NORFLASH_ID_0, (uint32_t)&otaUpgradeLog, version_md5, sizeof(otaUpgradeLog.version_md5));
//     pmu_flash_read_config();
//     int_unlock(lock);
// }

#if 0
static void ota_update_nv_data(void)
{
    uint32_t lock;

    if (1)
    {
        uint32_t* pOrgFactoryData, *pUpdatedFactoryData;
        pOrgFactoryData = (uint32_t *)(OTA_FLASH_LOGIC_ADDR + otaEnv.flasehOffsetOfFactoryDataPool);
        memcpy(otaEnv.dataBufferForBurning, (uint8_t *)pOrgFactoryData,
            FLASH_SECTOR_SIZE_IN_BYTES);
        pUpdatedFactoryData = (uint32_t *)&(otaEnv.dataBufferForBurning);

        uint32_t nv_record_dev_rev = pOrgFactoryData[dev_version_and_magic] >> 16;

        if (NVREC_DEV_VERSION_1 == nv_record_dev_rev)
        {

            pUpdatedFactoryData[dev_crc] =
                crc32(0,(uint8_t *)(&pUpdatedFactoryData[dev_reserv1]),(dev_data_len-dev_reserv1)*sizeof(uint32_t));
        }
        else
        {

            pUpdatedFactoryData[rev2_dev_crc] = crc32(
                0, (uint8_t *) (&pUpdatedFactoryData[rev2_dev_section_start_reserved]),
                pUpdatedFactoryData[rev2_dev_data_len]);
        }

        lock = int_lock_global();
        pmu_flash_write_config();
        hal_norflash_erase(HAL_NORFLASH_ID_0, OTA_FLASH_LOGIC_ADDR + otaEnv.flasehOffsetOfFactoryDataPool, FLASH_SECTOR_SIZE_IN_BYTES);

        hal_norflash_write(HAL_NORFLASH_ID_0, OTA_FLASH_LOGIC_ADDR + otaEnv.flasehOffsetOfFactoryDataPool, (uint8_t *)pUpdatedFactoryData, FLASH_SECTOR_SIZE_IN_BYTES);
        pmu_flash_read_config();
        int_unlock_global(lock);
    }
}
#endif


/************/

static void ota_reset_env(void)
{
    otaEnv.startLocationToWriteImage = NEW_IMAGE_FLASH_OFFSET;
    otaEnv.dstFlashOffsetForNewImage = NEW_IMAGE_FLASH_OFFSET;
    otaEnv.endLocationToWriteImage = otaEnv.startLocationToWriteImage + OTA_FLASH_SIZE;
    otaEnv.offsetInFlashToProgram = otaEnv.startLocationToWriteImage;
    otaEnv.offsetInFlashOfCurrentSegment = otaEnv.offsetInFlashToProgram;

    INTERCONNECTION_FREE(otaEnv.dataBufferForBurning);
    otaEnv.dataBufferForBurning = NULL;

    INTERCONNECTION_FREE(otaEnv.imageInfoPtr);
    otaEnv.imageInfoPtr = NULL;

    otaEnv.flasehOffsetOfUserDataPool = user_data_nv_flash_offset;
    otaEnv.flasehOffsetOfFactoryDataPool = user_data_nv_flash_offset + FLASH_SECTOR_SIZE_IN_BYTES;
    otaEnv.crcOfSegment = 0;
    otaEnv.crcOfImage = 0;
    otaEnv.offsetInDataBufferForBurning = 0;
    otaEnv.alreadyReceivedDataSizeOfImage = 0;
    otaEnv.addressOfStartApplyImage = 0;
    otaEnv.offsetOfImageOfCurrentSegment = 0;
    otaEnv.isOTAInProgress = false;
    otaEnv.isPendingForReboot = false;
    otaEnv.bitmap = 0;
    otaEnv.isLastPacketReceived = false;
    otaEnv.isImageInfoReceived = false;
    otaEnv.receivedImageInfoLength = 0;
    otaEnv.isLastPacketReceived = false;
    otaEnv.packetNum = 0;
    otaEnv.isAskAgain = false;
    otaEnv.askAgainPacketNum = 0;
    otaEnv.askAgainPacketTotal = 0;

    if(otaEnv.resume_at_breakpoint == false)
    {
        otaEnv.breakPoint = 0;
        otaEnv.i_log = -1;
    }

}

void app_interconn_ota_handler_init(void)
{
#ifdef __APP_USER_DATA_NV_FLASH_OFFSET__
    user_data_nv_flash_offset = __APP_USER_DATA_NV_FLASH_OFFSET__;
#else
    user_data_nv_flash_offset = hal_norflash_get_flash_total_size(HAL_NORFLASH_ID_0) - 2*4096;
#endif

    // app_interconn_sv_register_tx_done(app_interconn_send_done);

    ota_reset_env();
}

void app_interconnection_ota_finished_handler(void)
{
    if (otaEnv.isPendingForReboot)
    {
        osDelay(100);
        ota_reboot_to_use_new_image();
    }
    else
    {
        ota_reset_env();
    }
}

/**
 * @brief : handle the image info data
 *
 * @param ptrParam : pointer of received data
 * @param paramLen : length of received data
 */
void app_interconnection_ota_image_info_received_handler(uint8_t* ptrParam, uint32_t paramLen)
{
    //uint8_t psn = *ptrParam;
    uint8_t* dataPtr = ptrParam + 1;
    uint32_t dataLength = paramLen - 1;

    if (!otaEnv.isOTAInProgress)
    {
        return;
    }

    if(NULL == otaEnv.imageInfoPtr)
    {
        otaEnv.imageInfoPtr = (uint8_t*)INTERCONNECTION_MALLOC(IMAGE_INFO_SIZE + 10);

        if(NULL == otaEnv.imageInfoPtr)
        {
            TRACE("malloc for OTA update image info failed!!!");
            return;
        }
    }

    memcpy((otaEnv.imageInfoPtr + otaEnv.receivedImageInfoLength), dataPtr, dataLength);
    otaEnv.receivedImageInfoLength += dataLength;

    if(IMAGE_INFO_SIZE <= otaEnv.receivedImageInfoLength)
    {
        otaEnv.isImageInfoReceived = true;

        // syn received header info to ota environment variable
        if(INTERCONNECTION_STATUS_SUCCESS != customer_realize_image_info_receive_finished_hook())
        {
            app_interconnection_response_error_code(CCMP_MODULE_OTA, CCMP_OTA_ERR_CODE_UPDATE_DENIED, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_FIRMWARE_DATA);
            ota_reset_env();
            TRACE("stop ota progress!!!");
            return;
        }
        else
        {
            if(otaEnv.isUpdateNecessary)
            {
                // apply for first image data
                app_interconnection_ota_apply_update_data(otaEnv.addressOfStartApplyImage + otaEnv.alreadyReceivedDataSizeOfImage, 0, ((otaEnv.totalImageSize > FLASH_SECTOR_SIZE_IN_BYTES)?FLASH_SECTOR_SIZE_IN_BYTES:otaEnv.totalImageSize));
            }
        }
    }
}


//Breakpoint resume
// static void ota_resume_verify_handler(uint8_t* ptrParam, uint32_t paramLen)
// {
//     OTA_RESUME_VERIFY_T *ptParam = (OTA_RESUME_VERIFY_T*)ptrParam;
//     uint32_t breakPoint;
// 	OTA_RESUME_VERIFY_RESPONSE_T rsp;

//     TRACE("Receive command resuming verification");

//     TRACE("Receive md5:");
//     DUMP8("%02x ", ptParam->version_md5,paramLen);

//     TRACE("Device's md5:");
//     DUMP8("%02x ", otaUpgradeLog.version_md5, sizeof(otaUpgradeLog.version_md5));

//     breakPoint = get_upgradeSize_log();
// 	TRACE("breakpoint is %d",breakPoint);

//     if(!memcmp(otaUpgradeLog.version_md5, ptParam->version_md5, sizeof(otaUpgradeLog.version_md5)))
//     {
//         TRACE("OTA can resume. Resuming from the breakpoint at: 0x%x.", breakPoint);
//     }
//     else
//     {
//         TRACE("OTA can't resume because the version_md5 is inconsistent. [breakPoint: 0x%x]", breakPoint);

//         breakPoint = otaEnv.breakPoint = otaEnv.resume_at_breakpoint = 0;
//         ota_version_md5_log(ptParam->version_md5);
//     }

//     if(otaEnv.resume_at_breakpoint == true)
//     {
//         otaEnv.alreadyReceivedDataSizeOfImage = otaEnv.breakPoint;
//         otaEnv.offsetOfImageOfCurrentSegment = otaEnv.alreadyReceivedDataSizeOfImage;
//         otaEnv.offsetInFlashOfCurrentSegment = otaEnv.dstFlashOffsetForNewImage + otaEnv.offsetOfImageOfCurrentSegment;
//         otaEnv.offsetInFlashToProgram = otaEnv.offsetInFlashOfCurrentSegment;
//     }

// 	if(breakPoint ==0)
// 	{
// 	 //send response, transmit from the first;
// 	}
// 	else
// 	{
// 	   rsp.breakpoint_flag =1;
// 	   rsp.offset= breakPoint;

// 	  // int len=sizeof(bool)+sizeof(uint32_t);

//      // send response,transmit from the break

// 	}
// }

static uint8_t get_ask_again_total_packet_num(uint8_t bitMask, bool isLastPacketReceived)
{
    uint8_t calTime = 0;
    uint8_t calTotal;
    uint8_t packetNum = 0;

    if(isLastPacketReceived)
    {
        calTotal = otaEnv.imageLastRequestBitmapMask;
    }
    else
    {
        calTotal = 0xFF;
    }

    for (uint8_t i = 0; i < 8; i++)
    {
        if(calTotal & (1<<i))
        {
            calTime++;
        }
    }
    TRACE("calTime is %d", calTime);

    for (uint8_t j = 0; j < calTime; j++)
    {
        if(!(bitMask & (1<<j)))
        {
            packetNum++;
        }
    }
    TRACE("ask again packet num is %d", packetNum);

    return packetNum;
}

/**
 * @brief handle the firmware data
 *
 * @param ptrParam : pointer of received data
 * @param paramLen : length of received data
 */
void app_interconnection_ota_fw_data_received_handler(uint8_t *ptrParam, uint32_t paramLen, bool pass)
{
    //5.9.4command app send datapacket
    if (!otaEnv.isOTAInProgress)
    {
        TRACE("ota is not in progress!!!");
        ota_reset_env();
        return;
    }

    if(NULL == otaEnv.dataBufferForBurning)
    {
        otaEnv.dataBufferForBurning = (uint8_t*)INTERCONNECTION_MALLOC(OTA_DATA_BUFFER_SIZE_FOR_BURNING);
        if(NULL == otaEnv.dataBufferForBurning)
        {
            TRACE("malloc for dataBufferForBurning failed!!!");
            ota_reset_env();
            return;
        }
    }

    // judge if last packet is received
    if(OTA_UNIT_SIZE_SPP + otaEnv.alreadyReceivedDataSizeOfImage >= otaEnv.totalImageSize)
    {
        otaEnv.isLastPacketReceived = true;
    }
    else
    {
        otaEnv.isLastPacketReceived = false;
    }
    TRACE("already received data length is %d, total image size is %d, is last packet received is %d", otaEnv.alreadyReceivedDataSizeOfImage, otaEnv.totalImageSize, otaEnv.isLastPacketReceived?1:0);

    // received packet num update
    if(otaEnv.isAskAgain)
    {
        otaEnv.askAgainPacketNum++;
    }
    else
    {
        otaEnv.packetNum++;
    }

    // packet abnormal handle
    if(pass)
    {
        if(((8 == otaEnv.packetNum || otaEnv.isLastPacketReceived) && !otaEnv.isAskAgain) || (otaEnv.askAgainPacketTotal == otaEnv.askAgainPacketNum && otaEnv.isAskAgain))
        {
            app_interconnection_ota_apply_update_data(otaEnv.addressOfStartApplyImage + (otaEnv.offsetInFlashToProgram - otaEnv.dstFlashOffsetForNewImage), otaEnv.bitmap, FLASH_SECTOR_SIZE_IN_BYTES);
            otaEnv.isAskAgain = true;
            otaEnv.askAgainPacketNum = 0;
            otaEnv.askAgainPacketTotal = get_ask_again_total_packet_num(otaEnv.bitmap, otaEnv.isLastPacketReceived);
            TRACE("bit map error request again.");
        }
        return;
    }

    uint8_t psn_num = *ptrParam;

    // bit map check
    TRACE("psn_num is %d", psn_num);
    TRACE("current packet num is %d", otaEnv.packetNum);
    TRACE("isAskAgain is %d", otaEnv.isAskAgain);
    TRACE("current bit mask is 0x%02x", otaEnv.bitmap);
    if (psn_num < 8)
    {
        // current request state is ask again
        if(otaEnv.isAskAgain)
        {
            // unexpected data packet
            if(otaEnv.bitmap & 1<<psn_num)
            {
                otaEnv.askAgainPacketNum--;
                TRACE("unexpected ASK AGAIN data is received! RETURN");
                return;
            }
        }
        else // current request state is normal
        {
            // unexpected data pacekt
            if(psn_num+1 != otaEnv.packetNum)
            {
                otaEnv.packetNum--;
                TRACE("unexpected FW data is received! RETURN");
                return;
            }
        }
        otaEnv.bitmap |= 1 << psn_num;
    }
    else
    {
        ASSERT(0, "invalid psn number is received, please check!");
    }


    uint8_t* rawDataPtr = ptrParam + 1;
    uint32_t rawDataSize = paramLen-1;
    TRACE("Received image data size %d", rawDataSize);

    uint32_t leftDataSize = rawDataSize;
    uint32_t offsetInReceivedRawData = 0;
    uint32_t bytesToCopy = 0;

    do
    {
        // copy to data buffer
        if ((otaEnv.offsetInDataBufferForBurning + leftDataSize) >
            OTA_DATA_BUFFER_SIZE_FOR_BURNING)
        {
            bytesToCopy = OTA_DATA_BUFFER_SIZE_FOR_BURNING - otaEnv.offsetInDataBufferForBurning;
        }
        else
        {
            bytesToCopy = leftDataSize;
        }

        leftDataSize -= bytesToCopy;

        memcpy(&otaEnv.dataBufferForBurning[psn_num * OTA_UNIT_SIZE_SPP],
                &rawDataPtr[offsetInReceivedRawData], bytesToCopy);
        offsetInReceivedRawData += bytesToCopy;
        otaEnv.offsetInDataBufferForBurning += bytesToCopy;
        TRACE("offsetInDataBufferForBurning is %d", otaEnv.offsetInDataBufferForBurning);

        if (FLASH_SECTOR_SIZE_IN_BYTES == otaEnv.offsetInDataBufferForBurning || (otaEnv.isLastPacketReceived && otaEnv.imageLastRequestSize == otaEnv.offsetInDataBufferForBurning))//when datapacket is 4k or last packet is received, write to flash
        {
            if(otaEnv.isLastPacketReceived)
            {
                if(otaEnv.imageLastRequestBitmapMask != otaEnv.bitmap)
                {
                    app_interconnection_ota_apply_update_data(otaEnv.addressOfStartApplyImage + (otaEnv.offsetInFlashToProgram - otaEnv.dstFlashOffsetForNewImage), otaEnv.bitmap, otaEnv.imageLastRequestSize);
                    otaEnv.isAskAgain = true;
                    otaEnv.askAgainPacketNum = 0;
                    otaEnv.askAgainPacketTotal = get_ask_again_total_packet_num(otaEnv.bitmap, otaEnv.isLastPacketReceived);
                    TRACE("bit map error request again.");
                    return;
                }
            }
            else
            {
                if(0xFF != otaEnv.bitmap)
                {
                    app_interconnection_ota_apply_update_data(otaEnv.addressOfStartApplyImage + (otaEnv.offsetInFlashToProgram - otaEnv.dstFlashOffsetForNewImage), otaEnv.bitmap, FLASH_SECTOR_SIZE_IN_BYTES);
                    otaEnv.isAskAgain = true;
                    otaEnv.askAgainPacketNum = 0;
                    otaEnv.askAgainPacketTotal = get_ask_again_total_packet_num(otaEnv.bitmap, otaEnv.isLastPacketReceived);
                    TRACE("bit map error request again.");
                    return;
                }
            }
            TRACE("Program the image to flash.");

#if (IMAGE_RECV_FLASH_CHECK == 1)
            if((otaEnv.offsetInFlashToProgram - otaEnv.dstFlashOffsetForNewImage >= otaEnv.totalImageSize) ||
                (otaEnv.totalImageSize > MAX_IMAGE_SIZE) || (otaEnv.offsetInFlashToProgram + OTA_DATA_BUFFER_SIZE_FOR_BURNING > otaEnv.endLocationToWriteImage))
            {
                TRACE("ERROR: IMAGE_RECV_FLASH_CHECK");
                TRACE(" otaEnv(.offsetInFlashToProgram - .dstFlashOffsetForNewImage >= .totalImageSize)");
                TRACE(" or (otaEnv.totalImageSize > %d)", MAX_IMAGE_SIZE);
                TRACE(" or .offsetInFlashToProgram isn't segment aligned");
                TRACE(".offsetInFlashToProgram:0x%x  .dstFlashOffsetForNewImage:0x%x  .totalImageSize:%d", otaEnv.offsetInFlashToProgram, otaEnv.dstFlashOffsetForNewImage, otaEnv.totalImageSize);

                app_interconnection_response_error_code(CCMP_MODULE_OTA, CCMP_OTA_ERR_CODE_FLASH_PROCESS_FAILURE, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_FIRMWARE_DATA);

                app_interconnection_ota_finished_handler();

                return;
            }
            else
#endif
            {
                ota_flush_data_to_flash(otaEnv.dataBufferForBurning, OTA_DATA_BUFFER_SIZE_FOR_BURNING, otaEnv.offsetInFlashToProgram);

                otaEnv.offsetInFlashToProgram += OTA_DATA_BUFFER_SIZE_FOR_BURNING;
                otaEnv.offsetInDataBufferForBurning = 0;
                memset(otaEnv.dataBufferForBurning, 0, OTA_DATA_BUFFER_SIZE_FOR_BURNING);
                app_interconnection_package_size_report_handler(otaEnv.totalImageSize, otaEnv.alreadyReceivedDataSizeOfImage);
            }
        }
    } while (offsetInReceivedRawData < rawDataSize);

    otaEnv.alreadyReceivedDataSizeOfImage += rawDataSize;
    TRACE("Image already received %d", otaEnv.alreadyReceivedDataSizeOfImage);

 #if (IMAGE_RECV_FLASH_CHECK == 1)
    if((otaEnv.alreadyReceivedDataSizeOfImage > otaEnv.totalImageSize) ||
        (otaEnv.totalImageSize > MAX_IMAGE_SIZE))
    {
        TRACE("ERROR: IMAGE_RECV_FLASH_CHECK");
        TRACE(" otaEnv(.alreadyReceivedDataSizeOfImage > .totalImageSize)");
        TRACE(" or (otaEnv.totalImageSize > %d)", MAX_IMAGE_SIZE);
        TRACE(".alreadyReceivedDataSizeOfImage:%d  .totalImageSize:%d", otaEnv.alreadyReceivedDataSizeOfImage, otaEnv.totalImageSize);

        app_interconnection_response_error_code(CCMP_MODULE_OTA, CCMP_OTA_ERR_CODE_UPDATE_PACKET_ABNORMAL, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_FIRMWARE_DATA);

        app_interconnection_ota_finished_handler();

        return;
    }
    else
#endif
    {
        if(otaEnv.isLastPacketReceived)
        {
            app_interconnection_ota_firmware_verify_handler();
        }
        else
        {
            if(0xFF == otaEnv.bitmap)
            {
                otaEnv.bitmap = 0;
                otaEnv.packetNum = 0;
                otaEnv.isAskAgain = false;
                otaEnv.askAgainPacketNum = 0;
                otaEnv.askAgainPacketTotal = 0;
                app_interconnection_ota_apply_update_data(otaEnv.addressOfStartApplyImage + otaEnv.alreadyReceivedDataSizeOfImage, 0, (otaEnv.totalImageSize - otaEnv.alreadyReceivedDataSizeOfImage < FLASH_SECTOR_SIZE_IN_BYTES)?(otaEnv.totalImageSize - otaEnv.alreadyReceivedDataSizeOfImage):FLASH_SECTOR_SIZE_IN_BYTES);
            }
        }
    }

}

static void ota_update_magic_number_of_app_image(uint32_t newMagicNumber)
{
    uint32_t lock;

    uint32_t startFlashAddr = OTA_FLASH_LOGIC_ADDR +
            otaEnv.dstFlashOffsetForNewImage;
    TRACE_IMM("address to be changed is 0x%08x", startFlashAddr);

    memcpy(otaEnv.dataBufferForBurning, (uint8_t *)startFlashAddr, FLASH_SECTOR_SIZE_IN_BYTES);


    *(uint32_t *)otaEnv.dataBufferForBurning = newMagicNumber;

    lock = int_lock();
    pmu_flash_write_config();
    hal_norflash_erase(HAL_NORFLASH_ID_0, otaEnv.dstFlashOffsetForNewImage,
        FLASH_SECTOR_SIZE_IN_BYTES);
    hal_norflash_write(HAL_NORFLASH_ID_0, otaEnv.dstFlashOffsetForNewImage,
        otaEnv.dataBufferForBurning, FLASH_SECTOR_SIZE_IN_BYTES);
    pmu_flash_read_config();
    int_unlock(lock);
}

static uint32_t ota_convert_2_bes_crc(uint32_t dataPtr, uint32_t dataLen)
{
    uint32_t crc32Value = 0;

    crc32Value = crc32(crc32Value, (uint8_t *)(dataPtr + OTA_FLASH_LOGIC_ADDR), dataLen);

    return crc32Value;
}

void app_interconnection_ota_firmware_verify_handler(void)
{
    uint32_t firmware_crc32;
    uint32_t besCrc32 = 0;

    // check whether all image data have been received
    if (otaEnv.alreadyReceivedDataSizeOfImage == otaEnv.totalImageSize)
    {
        TRACE("The final image programming and crc32 check.");

        // flush the remaining data to flash
        if(otaEnv.offsetInDataBufferForBurning !=0)
        {
            ota_flush_data_to_flash(otaEnv.dataBufferForBurning,
                                    otaEnv.offsetInDataBufferForBurning,
                                    otaEnv.offsetInFlashToProgram);
        }

        besCrc32 = ota_convert_2_bes_crc(otaEnv.dstFlashOffsetForNewImage, otaEnv.totalImageSize);

        ota_update_magic_number_of_app_image(NORMAL_BOOT);

        // check the crc32 of the image
        firmware_crc32 = ota_check_whole_image_crc();

        if(firmware_crc32 == otaEnv.crcOfImage)
        {
            FLASH_OTA_BOOT_INFO_T otaBootInfo = {COPY_NEW_IMAGE, otaEnv.totalImageSize, besCrc32};
            ota_update_boot_info(&otaBootInfo);
            // ota_update_nv_data();

            otaEnv.isPendingForReboot = 1;

            TRACE("Whole image verification pass.");

            // response to mobile of image check result
            app_interconnection_ota_package_validity_response(true);

        }
        else
        {
            TRACE("Whole image verification failed.");
            TRACE("local calculated crc is 0x%08x, received image crc is 0x%08x", firmware_crc32, otaEnv.crcOfImage);

            // response to mobile the image check result
            app_interconnection_ota_package_validity_response(false);
        }

    }
    else
    {
        // received data length error
        app_interconnection_response_error_code(CCMP_MODULE_OTA, CCMP_OTA_ERR_CODE_UPDATE_PACKET_ABNORMAL, CCMP_MODULE_OTA, CCMP_OTA_COMMAND_ID_UPDATE_PERMISSION);
    }

    app_interconnection_ota_finished_handler();
}

#endif