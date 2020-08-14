#ifdef __AMA_VOICE__

#include <stdio.h>
#include "cmsis.h"
#include "cmsis_os.h"
#include "string.h"
#include "hal_trace.h"
#include "cqueue.h"
#include "voice_compression.h"
#include "app_audio.h"
#include "app_status_ind.h"
#include "apps.h"

#include "gapm_task.h"

#include "ai_transport.h"
#include "ai_manager.h"
#include "ai_control.h"
#include "ai_spp.h"
#include "ai_thread.h"
#include "app_ai_ble.h"
#include "app_ai_voice.h"
#include "app_ama_handle.h"
#include "app_ama_bt.h"
#include "app_ama_ble.h"
#include "ama_stream.h"

// mute the on-going a2dp streaming for some time when the ama start speech operation is triggered,
// if ama receives the media control pause request, stop the timer and clear the mute flag
// if ama is disconnected, stop the timer and clear the mute flag
// if ama stops the speech, stop the timer and clear the mute flag
// if timer expired, clear the mute flag


extern "C" void ama_control_send_transport_ver_exchange(void);
extern "C" uint8_t is_sco_mode (void);
#ifdef IS_MULTI_AI_ENABLED
extern "C" void ama_notify_device_configuration (uint8_t is_assistant_override);
extern "C" void gsound_enable_handler(uint8_t isEnable);
#endif

#define APP_AMA_UUID            "\x03\x16\x03\xFE"
#define APP_AMA_UUID_LEN        (4)

#ifdef IS_MULTI_AI_ENABLED
#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
osTimerId ama_ind_rsp_timeout_timer = NULL;
static void ama_indication_rsp_timeout_handler(void const *param);
osTimerDef (AMA_IND_RSP_TIMEOUT_TIMER, ama_indication_rsp_timeout_handler);
extern "C" bool ama_status_update_start;
static void ama_indication_rsp_timeout_handler(void const *param)
{
    TRACE("ama_indication_rsp_timeout_handler");
    if (ama_status_update_start)
    {
        app_bt_start_custom_function_in_bt_thread(0, 0, \
                (uint32_t)ai_manager_spec_update_start_reboot);
    }
}
#endif
#endif

static void app_ama_speech_state_timeout_timer_cb(void const *n)
{
    TRACE("%s", __func__);
    
    if((ai_struct.ai_stream.ai_stream_opened == true)) {
        ai_function_handle(CALLBACK_STOP_SPEECH, NULL, 0);
    }
    ama_reset_connection();
}

osTimerDef (APP_AMA_SPEECH_STATE_TIMEOUT, app_ama_speech_state_timeout_timer_cb); 
osTimerId   app_ama_speech_state_timeout_timer_id = NULL;

void app_ama_start_advertising(void *param1, uint32_t param2)
{
    struct gapm_start_advertise_cmd *cmd = (struct gapm_start_advertise_cmd*)param1;
    
    TRACE("%s", __func__);
    cmd->info.host.adv_data_len = 0;
    cmd->info.host.scan_rsp_data_len = 0;
    cmd->isIncludedFlags = true;
    
    memcpy(&cmd->info.host.adv_data[cmd->info.host.adv_data_len],
        APP_AMA_UUID, APP_AMA_UUID_LEN);
    cmd->info.host.adv_data_len += APP_AMA_UUID_LEN;
}

void app_ama_voice_connected(void *param1, uint32_t param2)
{
    TRACE("%s connect_type %d",__func__, param2);
    
    ai_set_power_key_start();
    ai_struct.transport_type = param2;

    if(param2 == AI_TRANSPORT_SPP){
        ai_data_transport_init(app_ai_spp_send);
        ai_cmd_transport_init(app_ai_spp_send);
        updata_transfer_type();
        if(ai_struct.encode_type == ENCODING_ALGORITHM_SBC) {
            ai_struct.trans_size = 342;
            ai_struct.trans_time = 3;
        } else {
            ai_struct.trans_size = 320;
            ai_struct.trans_time = 1;
        }
    }
#ifdef __IAG_BLE_INCLUDE__
    else if(param2 == AI_TRANSPORT_BLE){
        ai_data_transport_init(app_ama_send_via_notification);
        ai_cmd_transport_init(app_ama_send_via_notification);
        if(ai_struct.encode_type == ENCODING_ALGORITHM_SBC) {
            ai_struct.trans_size = 180;
            ai_struct.trans_time = 6;
        } else {
            ai_struct.trans_size = 160;
            ai_struct.trans_time = 1;
        }
    }
#endif

    ama_control_send_transport_ver_exchange();

    ai_struct.ai_stream.ai_stream_opened=false;
    ai_struct.ai_stream.ai_stream_running=false;

#ifdef IS_MULTI_AI_ENABLED
    ai_voicekey_save_status(true);
    //if gva has been enabled, it will notify to disable gva
    ai_manager_update_spec_table_connected_status_value(AI_SPEC_AMA, 1);
    app_ai_set_ble_adv_uuid(true);
#endif
}

void app_ama_voice_disconnected(void *param1, uint32_t param2)
{
    app_ai_control_mute_a2dp_state(false);
    
    TRACE("%s ama disonnected %d",__func__, ai_struct.transport_type);
    ai_struct.transport_type = AI_TRANSPORT_IDLE;
    ai_struct.ai_stream.ai_setup_complete = false;

    clean_transfer_type();
    app_ai_voice_on_off_control(false);
    
    ai_set_power_key_stop();
    ai_transport_fifo_deinit();
    
#ifdef IS_MULTI_AI_ENABLED
    ai_manager_update_spec_table_connected_status_value(AI_SPEC_AMA, 0);
#endif
    app_ai_set_ble_adv_uuid(true);
}

void app_ama_config_mtu(void *param1, uint32_t param2)  
{
    TRACE("--------->BLE UPDATA MTU SIZE %d", param2);
    ai_struct.txbuff_send_max_size = param2;
}

uint32_t app_ama_voice_send_done(void *param1, uint32_t param2);
uint32_t app_ama_handle_init(void *param1, uint32_t param2)  
{
    TRACE("enter %s", __func__);
    app_ama_speech_state_timeout_timer_id = osTimerCreate(osTimer(APP_AMA_SPEECH_STATE_TIMEOUT), osTimerOnce, NULL);

    ama_send_event_message_callback_init(app_ai_voice_on_off_control);

    app_ai_register_ble_tx_done(app_ama_voice_send_done);
    app_ai_register__spp_tx_done(app_ama_voice_send_done);
#ifdef IS_MULTI_AI_ENABLED
#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
    ama_ind_rsp_timeout_timer = osTimerCreate(osTimer(AMA_IND_RSP_TIMEOUT_TIMER), osTimerOnce, NULL);
#endif
#endif
    TRACE("%s %d ai_stream_opened set false",__func__,__LINE__);

    return 0;
}


void app_ama_parse_cmd(void *param1, uint32_t param2)
{
    uint32_t leftData = LengthOfCQueue(&ai_stream_buf_queue);
    uint8_t dataBuf[128];
    uint8_t tmp[sizeof(ama_stream_header_t)];
    ama_stream_header_t header;
    uint16_t i = 0;
    uint16_t cmd_id = (AMA_VER << 12)|(AMA_CONTROL_STREAM_ID<<7)|(AMA_FLAG<<1);
    uint16_t cmd_head;
    uint16_t stream_header_size = sizeof(ama_stream_header_t)-1;
    uint16_t temp_peekbuf_size;
    int status = 0;
    
    TRACE("%s cmd_id 0x%02x", __func__, cmd_id);

    while (leftData > 0) {        
        if (leftData < stream_header_size) {
            // check whether the header is correct
            TRACE("%s leftData is not enough %d", __func__, leftData);
            return;
        } else {
            temp_peekbuf_size = (leftData >= 128)?128:leftData;
            memset(dataBuf, 0, sizeof(dataBuf));
            if(PeekCQueueToBuf(&ai_stream_buf_queue, dataBuf, temp_peekbuf_size)) {
                status = 1;
                break;
            }

            //TRACE("%s ai_stream_buf_queue is:", __func__);
            //DUMP8("%02x ", dataBuf, leftData);
            for (i = 0; i < (temp_peekbuf_size-1); i++) {
                cmd_head = ((uint16_t)dataBuf[i]<<8)|dataBuf[i+1];
                TRACE("cmd_head 0x%04X", cmd_head);
                if ((cmd_head == cmd_id) || (cmd_head == (cmd_id+1)))
                    break;
            }
            leftData = leftData - i;

            if(leftData < stream_header_size) {
                TRACE("%s leftData too short %d", __func__, leftData);
                return;
            }
            //TRACE("%s leftData %d i %d", __func__, leftData, i);
            if(DeCQueue(&ai_stream_buf_queue, NULL, i)) {
                status = 2;
                break;
            }

            if(PeekCQueueToBuf(&ai_stream_buf_queue, tmp, stream_header_size)) {
                status = 3;
                break;
            }
            if(tmp[1]&0x01) {
                if(PeekCQueueToBuf(&ai_stream_buf_queue, tmp, stream_header_size+1)) {
                    status = 4;
                    break;
                }
            }

            if(false == ama_stream_header_parse(tmp, &header)) {
                status = 5;
                break;
            }
            
            if (header.length > (LengthOfCQueue(&ai_stream_buf_queue) - stream_header_size - header.isUse16BitLength)) {
                TRACE("%s length error %d", __func__, header.length);
                return;
            } else {
                memset(dataBuf, 0, sizeof(dataBuf));
                if(DeCQueue(&ai_stream_buf_queue, NULL, (stream_header_size + header.isUse16BitLength))) {
                    status = 6;
                    break;
                }
                if(DeCQueue(&ai_stream_buf_queue, dataBuf, header.length)) {
                    status = 7;
                    break;
                }
                TRACE("%s Raw data is:", __func__);
                DUMP8("%02x ", dataBuf, header.length);
                TRACE("Stream id is %d", header.streamId);

                if (AMA_CONTROL_STREAM_ID == header.streamId) {
                    ama_control_stream_handler(dataBuf, header.length);
                }
            }
        }

        leftData = LengthOfCQueue(&ai_stream_buf_queue);
    }

    if(status) {
        TRACE("%s error status %d, clean up the queue!", __func__, status);
        ResetCqueue(&ai_stream_buf_queue);
    }
}

uint32_t app_ama_rcv_cmd(void *param1, uint32_t param2)
{
    TRACE("%s len=%d",__func__, param2);
    //DUMP8("%02x ", param1, param2);

    EnCQueue(&ai_stream_buf_queue, (CQItemType *)param1, param2);
    ai_mailbox_put(SIGN_AI_CMD_COME, 0);

    return 0;
}

#define AMA_TRANSFER_DATA_MAX_LEN 400
uint32_t app_ama_stream_init(void *param1, uint32_t param2)
{
    app_capture_audio_mempool_get_buff(&(ai_struct.ai_stream.tmpBufForXfer),     
        AMA_TRANSFER_DATA_MAX_LEN);
    
    ai_transport_fifo_deinit();

    return 0;
}

void app_ama_stream_deinit(void *param1, uint32_t param2)
{
	ai_transport_fifo_deinit();
	ai_struct.ai_stream.ai_stream_opened=false;
	ai_set_power_key_start();
}

uint32_t app_ama_start_speech(void *param1, uint32_t param2)
{
    if((ai_struct.ai_stream.ai_stream_opened == true)) {
        TRACE("%s already open",__func__);
        return 1;
    } else {
        ai_struct.ai_stream.ai_stream_opened = true;

        app_ai_voice_start_mic_stream();
        app_ai_spp_deinit_tx_buf();
        ama_start_speech_request();
    }

    return 0;
}

uint32_t app_ama_stop_speech(void *param1, uint32_t param2)
{
    TRACE("%s",__func__);
    app_ai_control_mute_a2dp_state(false);
    app_ai_voice_stop_mic_stream();
    ama_stop_speech_request(USER_CANCEL);

    return 0;
}

uint32_t app_ama_endpoint_speech(void *param1, uint32_t param2)
{
    TRACE("%s", __func__);
    if(ai_get_power_key()) {
        ama_endpoint_speech_request(0);
        app_ai_control_mute_a2dp_state(false);
        app_ai_voice_stop_mic_stream();
    }

    return 0;
}

uint32_t app_ama_wake_up(void *param1, uint32_t param2)
{
    TRACE("%s power_key %d connected_type %d",
        __func__, ai_get_power_key(), ai_struct.transport_type);
    
    if ((ai_struct.ai_stream.ai_setup_complete==false)||
        (ai_struct.transport_type==AI_TRANSPORT_IDLE)) {
        TRACE("%s no ama connect", __func__);
        return 1;
    }

#ifdef IS_MULTI_AI_ENABLED
    if (ai_manager_get_spec_connected_status(AI_SPEC_AMA) == 0) {
        TRACE("%s  APP not connected!", __func__);
        return 1;
    }
#endif

    if (is_sco_mode()) {
        TRACE("%s is_sco_mode", __func__);
        return 1;
    }

    if (ai_get_power_key() == 0) {
        ai_function_handle(CALLBACK_START_SPEECH, NULL, 0);
        ai_mailbox_put(SIGN_AI_WAKEUP, 0);
        ai_set_power_key_stop();
    } else {
        ai_function_handle(CALLBACK_STOP_SPEECH, NULL, 0);
        ai_set_power_key_start();
    }
    
    return 0;
}

uint32_t app_ama_send_voice_stream(void *param1, uint32_t param2)
{
    uint32_t min_dataBytesToSend=80;
    uint32_t fact_send_len=0;

    uint8_t i = 0;
    uint32_t encodedDataLength = voice_compression_get_encode_buf_size();

#if 1 //!AI_32KBPS_VOICE
    if(!is_ai_audio_stream_allowed_to_send()) {
        //voice_compression_get_encoded_data(NULL, encodedDataLength);
        TRACE("%s ama don't allowed_to_send encodedDataLength %d", __func__, encodedDataLength);
        return 1;
    }
#endif

    for(i=0; i<ai_struct.trans_time; i++) {
        if(ai_struct.transport_type == AI_TRANSPORT_SPP || ai_struct.transport_type == AI_TRANSPORT_BLE) {
            if (encodedDataLength < ai_struct.trans_size) {
                return 2;
            }
            min_dataBytesToSend = ai_struct.trans_size;
        } else {
            TRACE("%s ama transport_type is error  %d", __func__, ai_struct.transport_type);
            ai_audio_stream_allowed_to_send_set(false);
            return 1;
        }

        TRACE("%s encodedDataLength %d credit %d", __func__, encodedDataLength, ai_struct.tx_credit);
        if(0 == ai_struct.tx_credit) {
            TRACE("%s no txCredit", __func__);
            return 2;
        }
        ai_struct.tx_credit--;
        
        fact_send_len = voice_compression_get_encoded_data(ai_struct.ai_stream.tmpBufForXfer
            ,min_dataBytesToSend);
    
        if(fact_send_len) {
            ama_audio_stream_send(ai_struct.ai_stream.tmpBufForXfer,fact_send_len);
        } else {
            TRACE("%s no data send %d", __func__, fact_send_len);
           }
        encodedDataLength = voice_compression_get_encode_buf_size();
    }

    return 0;
}

uint32_t app_ama_voice_send_done(void *param1, uint32_t param2)
{
    //TRACE("%s credit %d", __func__, ai_struct.tx_credit);
    if(ai_struct.tx_credit < MAX_TX_CREDIT) {
        ai_struct.tx_credit++;
    }

    return 0;
}

uint32_t app_ama_enable_handler(void *param1, uint32_t param2)
{
    TRACE("%s %d", __func__, param2);

    if (param2) {
        ama_notify_device_configuration(0);
        app_ai_set_ble_adv_uuid(true);
    } else {
        ama_notify_device_configuration(1);
        app_ai_set_ble_adv_uuid(false);
        //ai_function_handle(CALLBACK_AI_DISCONNECT, NULL, 0);
    }

    return 0;
}

/// AMA handlers function definition
const struct ai_func_handler ama_handler_function_list[] =
{
    {API_HANDLE_INIT,           (ai_handler_func_t)app_ama_handle_init},
    {API_START_ADV,             (ai_handler_func_t)app_ama_start_advertising},
    {API_DATA_SEND,             (ai_handler_func_t)app_ama_send_voice_stream},
    {API_DATA_INIT,             (ai_handler_func_t)app_ama_stream_init},
    {API_DATA_DEINIT,           (ai_handler_func_t)app_ama_stream_deinit},
    {CALLBACK_CMD_RECEIVE,      (ai_handler_func_t)app_ama_rcv_cmd},
    {CALLBACK_DATA_RECEIVE,     (ai_handler_func_t)NULL},
    {CALLBACK_DATA_PARSE,       (ai_handler_func_t)app_ama_parse_cmd},
    {CALLBACK_AI_CONNECT,       (ai_handler_func_t)app_ama_voice_connected},
    {CALLBACK_AI_DISCONNECT,    (ai_handler_func_t)app_ama_voice_disconnected},
    {CALLBACK_UPDATE_MTU,       (ai_handler_func_t)app_ama_config_mtu},
    {CALLBACK_WAKE_UP,          (ai_handler_func_t)app_ama_wake_up},
    {CALLBACK_AI_ENABLE,        (ai_handler_func_t)app_ama_enable_handler},
    {CALLBACK_START_SPEECH,     (ai_handler_func_t)app_ama_start_speech},
    {CALLBACK_ENDPOINT_SPEECH,  (ai_handler_func_t)app_ama_endpoint_speech},
    {CALLBACK_STOP_SPEECH,      (ai_handler_func_t)app_ama_stop_speech},
    {CALLBACK_DATA_SEND_DONE,   (ai_handler_func_t)app_ama_voice_send_done},
};

const struct ai_handler_func_table ama_handler_function_table =
    {&ama_handler_function_list[0], (sizeof(ama_handler_function_list)/sizeof(struct ai_func_handler))};

AI_HANDLER_FUNCTION_TABLE_TO_ADD(AI_SPEC_AMA, &ama_handler_function_table)

#endif

