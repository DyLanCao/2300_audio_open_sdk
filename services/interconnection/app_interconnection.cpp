#ifdef __INTERCONNECTION__

#include <stdlib.h>
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "string.h"
#include "sdp_api.h"
#include "spp.h"
#include "app_interconnection.h"
#include "app_interconnection_logic_protocol.h"
#include "app_interconnection_spp.h"
#include "app_interconnection_ota.h"
#include "app_interconnection_ccmp.h"
#include "app_interconnection_customer_realize.h"

// #include "btapp.h"
#include "app_bt.h"

osMailQDef(interconnection_mailbox, 64, INTERCONNECTION_MAIL_T);
osMailQId      interconnection_mail_queue_id = NULL;
static uint8_t interconnection_mail_cnt      = 0;

static void interconnection_read_thread(const void *arg);
osThreadDef(interconnection_read_thread, osPriorityAboveNormal, 1, 4096, "inter_conn");
osThreadId interconnection_read_thread_id = NULL;

static uint8_t               interconnectionReadBuffer[SPP_MAX_PACKET_SIZE];
static INTERCONNECTION_ENV_T interconnectionEnv;


static const uint8_t SppServiceSearchReq[] = {
    /* First parameter is the search pattern in data element format. It
    * is a list of 3 UUIDs.
    */
    /* Data Element Sequence, 9 bytes */
    SDP_ATTRIB_HEADER_8BIT(9),
    SDP_UUID_16BIT(0xFE35),
    SDP_UUID_16BIT(PROT_L2CAP),  /* L2CAP UUID in Big Endian */
    SDP_UUID_16BIT(PROT_RFCOMM), /* UUID for RFCOMM in Big Endian */
    /* The second parameter is the maximum number of bytes that can be
    * be received for the attribute list.
    */
    0x00,
    0x64, /* Max number of bytes for attribute is 100 */
    SDP_ATTRIB_HEADER_8BIT(9),
    SDP_UINT_16BIT(AID_PROTOCOL_DESC_LIST),
    SDP_UINT_16BIT(AID_BT_PROFILE_DESC_LIST),
    SDP_UINT_16BIT(AID_ADDITIONAL_PROT_DESC_LISTS)};

static int interconnection_mailbox_init(void)
{
    interconnection_mail_queue_id = osMailCreate(osMailQ(interconnection_mailbox), NULL);
    if (NULL == interconnection_mail_queue_id)
    {
        TRACE("Failed to Create interconnection mailbox\n");
        return -1;
    }

    interconnection_mail_cnt = 0;
    return 0;
}

bool app_interconnection_get_spp_client_alive_flag(void)
{
    return interconnectionEnv.isSppClientAlive;
}

bool app_interconnection_get_spp_server_alive_flag(void)
{
    return interconnectionEnv.isSppServerAlive;
}

void app_interconnection_set_spp_client_alive_flag(bool flag)
{
    interconnectionEnv.isSppClientAlive = flag ? 1 : 0;
}

void app_interconnection_set_spp_server_alive_flag(bool flag)
{
    interconnectionEnv.isSppServerAlive = flag ? 1 : 0;
}

bool app_interconnection_get_ccmp_alive_flag(void)
{
    return interconnectionEnv.isCcmpAlive;
}

void app_interconnection_set_ccmp_alive_flag(bool flag)
{
    interconnectionEnv.isCcmpAlive = flag ? 1 : 0;
}

bool app_interconnection_get_ccmp_denied_by_mobile_flag(void)
{
    return interconnectionEnv.isCcmpDeniedByMobile;
}

void app_interconnection_set_ccmp_denied_by_mobile_flag(bool flag)
{
    interconnectionEnv.isCcmpDeniedByMobile = flag ? 1 : 0;
}

uint8_t app_interconnection_get_ccmp_channel(void)
{
    return interconnectionEnv.ccmpChannel;
}

void app_interconnection_set_ccmp_channel(uint8_t ccmpChannel)
{
    interconnectionEnv.ccmpChannel = ccmpChannel;
}

uint8_t app_interconnection_get_data_path(void)
{
    return interconnectionEnv.dataPath;
}

void app_interconnection_set_data_path(uint8_t dataPath)
{
    interconnectionEnv.dataPath = dataPath;
}


void app_interconn_env_init(void)
{
    if (interconnection_mailbox_init())
        return;

    memset(&interconnectionEnv, 0, sizeof(interconnectionEnv));
    interconnectionEnv.rxThread = app_interconnection_get_read_thread();
}

/**
 * @brief initialization of interconnection spp module
 *
 */
void app_interconnection_init(void)
{
    TRACE("[%s]", __func__);
    app_interconn_env_init();
}

void app_interconnection_spp_open(void)
{
    app_spp_client_open(( uint8_t * )SppServiceSearchReq, sizeof(SppServiceSearchReq));
    app_spp_server_open();
}

void app_interconn_send_done(void)
{
    TRACE("Data sent out");
}

void app_interconnection_ccmp_client_open(uint8_t port)
{
    app_ccmp_client_open(( uint8_t * )SppServiceSearchReq, sizeof(SppServiceSearchReq), port);
}

int interconnection_mail_put(void *pDev, uint8_t process, uint8_t *pData, uint16_t dataLen)
{
    osStatus status;

    INTERCONNECTION_MAIL_T *msgPtr = NULL;

    msgPtr = ( INTERCONNECTION_MAIL_T * )osMailAlloc(interconnection_mail_queue_id, 0);
    ASSERT(msgPtr, "osMailAlloc error");
    msgPtr->pDev    = pDev;
    msgPtr->process = process;
    msgPtr->data    = pData;
    msgPtr->dataLen = dataLen;

    status = osMailPut(interconnection_mail_queue_id, msgPtr);
    if (osOK == status)
        interconnection_mail_cnt++;
    return ( int )status;
}

static int interconnection_mail_free(INTERCONNECTION_MAIL_T *msgPtr)
{
    osStatus status;

    status = osMailFree(interconnection_mail_queue_id, msgPtr);
    if (osOK == status)
        interconnection_mail_cnt--;

    return ( int )status;
}

static int interconnection_mail_get(INTERCONNECTION_MAIL_T **msgPtr)
{
    osEvent evt;
    evt = osMailGet(interconnection_mail_queue_id, osWaitForever);
    if (evt.status == osEventMail)
    {
        *msgPtr = ( INTERCONNECTION_MAIL_T * )evt.value.p;
        return 0;
    }
    return -1;
}

static void interconnection_read_thread(const void *arg)
{
    INTERCONNECTION_MAIL_T *pMail      = NULL;
    struct spp_device *     pDev       = NULL;
    uint16_t                rcvDataLen = 0;
    bt_status_t             readDataStatus;

    uint8_t *buffer = interconnectionReadBuffer;

    while (1)
    {
        pMail          = NULL;
        readDataStatus = BT_STS_FAILED;

        if (!interconnection_mail_get(&pMail))
        {
            pDev       = ( struct spp_device * )pMail->pDev;
            rcvDataLen = pMail->dataLen;

            if (app_interconnection_get_spp_client() == pDev &&
                app_interconnection_get_spp_client_alive_flag())
            {
                TRACE("interconnection spp client data event");
                // spp data event handle
                switch (pMail->process)
                {
                case INTERCONNECTION_PROCESS_READ:
                    TRACE("read event");
                    readDataStatus = app_interconnection_spp_client_read_data(( char * )buffer, &rcvDataLen);

                    if (BT_STS_SUCCESS == readDataStatus)
                    {
                        app_interconnection_spp_client_data_received_callback(buffer, ( uint32_t )rcvDataLen);
                    }
                    else
                    {
                        TRACE("spp clientread data error.");
                    }
                    break;

                case INTERCONNECTION_PROCESS_WRITE:
                    TRACE("write event");
                    break;

                default:
                    TRACE("process not defined.");
                    break;
                }
            }
            else if (app_interconnection_get_spp_server() == pDev &&
                     app_interconnection_get_spp_server_alive_flag())
            {
                TRACE("interconnection spp server data event.");
                switch (pMail->process)
                {
                case INTERCONNECTION_PROCESS_READ:
                    TRACE("read event");
                    readDataStatus = app_interconnection_spp_server_read_data(( char * )buffer, &rcvDataLen);

                    if (BT_STS_SUCCESS == readDataStatus)
                    {
                        app_interconnection_spp_server_data_received_callback(buffer, ( uint32_t )rcvDataLen);
                    }
                    else
                    {
                        TRACE("spp device read data error.");
                    }
                    break;

                case INTERCONNECTION_PROCESS_WRITE:
                    TRACE("interconnection write event");
                    break;

                default:
                    TRACE("interconnection process not defined.");
                    break;
                }
            }
            else if (app_interconnection_get_ccmp_dev() == pDev &&
                     app_interconnection_get_ccmp_alive_flag())
            {
                TRACE("interconnection ccmp data event");
                // ccmp data event handle
                switch (pMail->process)
                {
                case INTERCONNECTION_PROCESS_READ:
                    TRACE("interconnection read event");
                    readDataStatus = app_interconnection_ccmp_dev_read_data(( char * )buffer, &rcvDataLen);

                    if (BT_STS_SUCCESS == readDataStatus)
                    {
                        app_interconnection_ccmp_data_received_callback(buffer, ( uint32_t )rcvDataLen);
                    }
                    else
                    {
                        TRACE("ccmp device read data error.");
                    }
                    break;
                case INTERCONNECTION_PROCESS_WRITE:
                    TRACE("interconnection write event");
                    break;
                default:
                    TRACE("interconnection process not defined.");
                    break;
                }
            }
            else
            {
                TRACE("interconnection invalid event received.");
            }

            interconnection_mail_free(pMail);
        }
    }
}

static void app_interconnection_creat_read_thread(void)
{
    TRACE("%s %d\n", __func__, __LINE__);
    if (NULL == interconnection_read_thread_id)
    {
        interconnection_read_thread_id = osThreadCreate(osThread(interconnection_read_thread), NULL);
    }
}

void app_interconnection_close_read_thread(void)
{
    TRACE("%s %d\n", __func__, __LINE__);
    if (interconnection_read_thread_id)
    {
        TRACE("terminate interconnection read thread!!!");
        osThreadTerminate(interconnection_read_thread_id);
        interconnection_read_thread_id = NULL;
    }
    else
    {
        TRACE("interconnection read thread already terminated!!!");
    }
}

osThreadId app_interconnection_get_read_thread(void)
{
    if (NULL == interconnection_read_thread_id)
    {
        TRACE("creat interconnection read thread.");
        app_interconnection_creat_read_thread();
    }
    else
    {
        TRACE("interconnection read thread already created.");
    }

    return interconnection_read_thread_id;
}

void app_interconnection_disconnected_callback(void)
{
    app_interconnection_spp_client_disconnection_callback();
    app_interconnection_spp_server_disconnection_callback();
    app_interconnection_ccmp_disconnected_callback();
}

#endif  // #ifdef __INTERCONNECTION__
