#include <stdio.h>
#include "cmsis.h"
#include "cmsis_os.h"
#include "string.h"
#include "hal_trace.h"
#include "nvrecord.h"
#include "accessories.pb-c.h"
#include <protobuf-c/protobuf-c.h>
#include <time.h>
#include "../ama_manager/app_ama_handle.h"
#include "ama_stream.h"
#include "ai_transport.h"
#include "ai_manager.h"
#include "ai_thread.h"
#include "ai_spp.h"

#ifdef __AMA_VOICE__

extern void app_bt_suspend_current_a2dp_streaming(void);
#ifdef IS_MULTI_AI_ENABLED
extern void gsound_enable_handler(uint8_t isEnable);
#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
bool ama_status_update_start = false;
#define AMA_INDICATION_RSP_TIMEOUT (3000)
extern osTimerId ama_ind_rsp_timeout_timer;
#endif
#endif

ProtobufCAllocator  ama_stack;

typedef struct ama_stack_buffer {
    uint16_t poolSize; 
    uint8_t *pool; 
} ama_stack_buffer_t; 

void * ama_stack_alloc(void *allocator_data, size_t size) {
#if 0
    ama_stack_buffer_t * stack_buffer = (ama_stack_buffer_t *)allocator_data; 
    ASSERT(stack_buffer->poolSize >= size,"AMA MEMORY ALLOC ERROR!!");
    return stack_buffer->pool;
#endif
    ama_stack_buffer_t * stack_buffer = (ama_stack_buffer_t *)allocator_data; 
    TRACE("buffer poolsize :%d , alloc size : %d", stack_buffer->poolSize, size);
    assert(stack_buffer->poolSize >= size);
    stack_buffer->poolSize -= size;
    uint8_t * ptr = stack_buffer->pool;
    stack_buffer->pool += size; 
    return ptr;

}

void  ama_stack_free(void *allocator_data, void *pointer) {
    //nothing to do 
}

void  ama_stack_allocator_init(){
    ama_stack.alloc = ama_stack_alloc; 
    ama_stack.free = ama_stack_free;
    ama_stack.allocator_data = NULL;
}

#define AMA_STACK_ALLOC_INIT_POOL(a,b)   \
    ama_stack_buffer_t  *allocator_data = (ama_stack_buffer_t  *)(a); \
    allocator_data->poolSize = ((b)-sizeof(ama_stack_buffer_t));  \
    allocator_data->pool = (((uint8_t *)(a))+ sizeof(ama_stack_buffer_t)); \
    ama_stack.alloc = ama_stack_alloc; \
    ama_stack.free = ama_stack_free;\
    ama_stack.allocator_data  = allocator_data;

bool ama_stream_header_parse(uint8_t *buffer, ama_stream_header_t *header)
{
    uint16_t h;
    uint8_t*p;
    p = (uint8_t*)&h; 
    p[0]= buffer[1];
    p[1]= buffer[0];

    header->version = (h >> 12) &0x0f;
    header->streamId = (h>> 7) & 0x1f; 
    header->flags = (h >>1) & 0x3f; 
    header->isUse16BitLength = h&0x01;

    header->length = buffer[2];
    if(header->isUse16BitLength == 1) {
        header->length = (header->length << 8) | buffer[3];
    }

    if(header->streamId != AMA_CONTROL_STREAM_ID && header->streamId != AMA_VOICE_STREAM_ID) {
        TRACE("%s stream ID is error %d", __func__, header->streamId);
        return false;
    }
    
    TRACE("version=%d stream_id=%d flags=%d payload_length_flag=%d length=%d", 
        header->version, header->streamId, header->flags, header->isUse16BitLength, header->length);
    return true;
}

int32_t ama_stream_add_header_data(const ama_stream_header_t *header, uint8_t *buf)
{
    uint16_t h; 
    uint8_t *p;
    uint8_t i = 0;

    h = header->version; 
    h = (h << 12) | (header->streamId <<7) | (header->flags <<1 ) | header->isUse16BitLength; 
    p = (uint8_t*)&h;
    buf[i++] = p[1];
    buf[i++] = p[0];
    if(header->isUse16BitLength){
        buf[i++] = header->length>>8;
        buf[i++] = header->length &0xff;
    }
    else 
        buf[i++] = header->length;

    return i;
}

static int (*ama_send_event_message)(bool on);

#define AMA_RESTART_TIMEOUT_INTERVEL 5000

static void ama_restart_mic_timeout_timer_cb(void const *n);
osTimerDef (AMA_STATE_MONITOR_TIMEOUT, ama_restart_mic_timeout_timer_cb); 
osTimerId    ama_restart_timeout_timer_id = NULL;

SpeechState ama_gloable_state;

static void ama_restart_mic_timeout_timer_cb(void const *n)
{
    TRACE("TIME OUT====================================================================================>");
    if(ama_gloable_state!=SPEECH_STATE__IDLE){
        TRACE("timeout restart ama stream!!!!");
        ama_send_event_message(true);
    }
}


volatile unsigned int ama_each_stream_len;
extern void a2dp_handleKey(uint8_t a2dp_key);

bool ama_audio_stream_send(uint8_t *in_buf,uint32_t len)
{
    uint8_t audio_stream_buf[128];
    uint8_t count = 0;
    uint32_t send_len = 0;
    int ret;
    
	TRACE("%s len %d max_size %d", __func__, len, ai_struct.txbuff_send_max_size);
    
    if(in_buf==NULL){
        TRACE("%s the buff is NULL", __func__);
        return false;
    }
    ama_stream_header_t header = AMA_AUDIO_STREAM_HEADER__INIT;
    size_t offset=0;
    size_t frame_len = ama_each_stream_len-3;
    size_t size = 0;

    for(count=0; count<(len/frame_len); count++){
        size=frame_len;

        if(size < 256) {
            memcpy(audio_stream_buf+3,in_buf+offset,frame_len);
        } else {
            memcpy(audio_stream_buf+4,in_buf+offset,frame_len);
        }
        
        offset+=frame_len;
        header.length = size;

        size += ama_stream_add_header_data(&header, audio_stream_buf);

        ret = ai_transport_data_put(audio_stream_buf, size);
        send_len += size;
        if (ret == false) {
            TRACE("%s ai transport error ", __func__);
        }
    }
    
    ai_mailbox_put(SIGN_AI_TRANSPORT, send_len);
    return true;
}

typedef struct
{
    uint16_t protocolId;
    uint8_t majorversion;
    uint8_t minorversion;
    uint8_t reserve[16];
} TRANSPORT_VER_EXCHANGE_INFO_T;

void ama_control_send_transport_ver_exchange(void)
{
    TRACE("%s", __func__);
#if 0
    TRANSPORT_VER_EXCHANGE_INFO_T exchangeInfo = 
    {
        0x03,
        0xFE, 
        1,    // major version must be 1
        0,  // minor version must be 0
        0,
    };
#endif
    uint8_t send_buf[20]={0};
    send_buf[0]=0xFE;
    send_buf[1]=0X03;
    send_buf[2]=0X01;
    send_buf[3]=0X00;

    ai_cmd_transport(send_buf, sizeof(send_buf));
}

static bool ama_control_stream_send(const ControlEnvelope *message,uint32_t len)
{
    size_t size = 0;
    int ret;
    
    uint8_t outputBuf[128];

    ama_stream_header_t header = AMA_CTRL_STREAM_HEADER__INIT;
    size = len;

    if(size < 256)
        size = control_envelope__pack(message, outputBuf+3);
    else 
        size = control_envelope__pack(message, outputBuf+4);

    header.length = size;
    TRACE("%s size=%d", __func__, size);

    size +=ama_stream_add_header_data(&header, outputBuf);

    ret = ai_cmd_transport(outputBuf, size);
    
    //DUMP8("%02x ", outputBuf, size);
    if (ret == false) {
        TRACE("%s error", __func__);
        return false;
    }

    return true;
}

uint32_t dialog_id = 0;

void dialog_id_set(uint32_t id)
{
    TRACE("set dialog id to %d",id);
    dialog_id = id;
}

uint32_t dialog_id_get(void)
{
    TRACE("get dialog id: %d",dialog_id);
    return dialog_id;
}

uint32_t start_dialog_id_get(void){
    static uint32_t start_dialog_id = 0;
    ++start_dialog_id;
    if(start_dialog_id>0x7fffffff)
        start_dialog_id= 0;
    TRACE("start dialog id :%d ",start_dialog_id);
    return start_dialog_id;
}

void dialog_id_reset()
{
    //start_dialog_id=0;
}



int implement_ama_media_control(IssueMediaControl *media_control)
{
    TRACE("implement_ama_media_control : %d ",media_control->control);
    switch(media_control->control){
        case MEDIA_CONTROL__PLAY:
            a2dp_handleKey(2);
            break;
        case MEDIA_CONTROL__PAUSE:            
            //app_bt_suspend_current_a2dp_streaming();
            a2dp_handleKey(3);
            break;
        case MEDIA_CONTROL__NEXT:
            a2dp_handleKey(4);
            break;
        case MEDIA_CONTROL__PREVIOUS:
            a2dp_handleKey(5);
            break;
        default:
            break;
    }
    return 0;
}


extern osTimerId app_ama_speech_state_timeout_timer_id;
static void ama_cmd_handler(ControlEnvelope* msg, uint8_t* parameter)
{
    char dev_bt_addr[6];
    bool isToResponse = true;
    size_t size = 0;
    uint8_t index = 0;

    ControlEnvelope__PayloadCase payloadCase = msg->payload_case;
    Command command = msg->command;

    ControlEnvelope controlMessage;
    control_envelope__init(&controlMessage);

    Response responseMessage;
    response__init(&responseMessage);
    

    TRACE("----->payload case %d",payloadCase);
    TRACE("----->command %d",msg->command);

    if(payloadCase==CONTROL_ENVELOPE__PAYLOAD_RESPONSE){
        TRACE("rsp err code %d",msg->response->error_code); 
        isToResponse = false;
        switch (command)
        {    
            case COMMAND__START_SPEECH:
            {
                //app_voice_report(APP_STATUS_INDICATION_GSOUND_MIC_OPEN, 0);
                TRACE("COMMAND__START_SPEECH dialog: %d error_code %d",
                    msg->response->dialog->id, msg->response->error_code);
                dialog_id_set(msg->response->dialog->id);
                if(msg->response->error_code == ERROR_CODE__SUCCESS) {
#if AI_32KBPS_VOICE
                    ai_audio_stream_allowed_to_send_set(true);
#endif
                    TRACE("COMMAND__START_SPEECH success");
                } else {
                    TRACE("XXXXXXX---COMMAND__START_SPEECH----xxxxxxFAIL");
                }
                break;
            }
            
            case COMMAND__STOP_SPEECH:
            {
                TRACE("COMMAND__STOP_SPEECH ---response");      
                break;
            }
            
            case COMMAND__INCOMING_CALL:
            {
                TRACE("COMMAND__INCOMING_CALL ---response");
                break; 
            }
            
            case COMMAND__SYNCHRONIZE_STATE:
            {
                TRACE("COMMAND__SYNCHRONIZE_STATE ---response");
                break;
            }
            
            case COMMAND__ENDPOINT_SPEECH:
            {
                TRACE("COMMAND__ENDPOINT_SPEECH ---response");
                break; 
            }
            case COMMAND__GET_CENTRAL_INFORMATION:
            {
                TRACE("COMMAND__GET_CENTRAL_INFORMATION ---response");
                break;
            }
            case COMMAND__RESET_CONNECTION:
            {
                TRACE("COMMAND__RESET_CONNECTION ---response");
                break;
            }
            case COMMAND__KEEP_ALIVE:
            {
                TRACE("COMMAND__KEEP_ALIVE ---response");
                break;
            }
            case COMMAND__NOTIFY_DEVICE_CONFIGURATION:
            {
                TRACE("COMMAND__NOTIFY_DEVICE_CONFIGURATION ---response");
#ifdef IS_MULTI_AI_ENABLED
#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
                if (ama_status_update_start)
                {
                    osTimerStop(ama_ind_rsp_timeout_timer);
                    ai_manager_spec_update_start_reboot();
                }
#endif
#endif
                break;
            }
            default:
                break;
        }
    }else{
    
        switch (command)
        {
            case COMMAND__GET_DEVICE_INFORMATION:
            {
                Transport transport[2] = {TRANSPORT__BLUETOOTH_RFCOMM,TRANSPORT__BLUETOOTH_LOW_ENERGY};
                DeviceInformation deviceInfo;
                
                TRACE("COMMAND__GET_DEVICE_INFORMATION");
                controlMessage.command =  command; 
                 controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                 controlMessage.response = &responseMessage;

                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD_DEVICE_INFORMATION;
                responseMessage.device_information = &deviceInfo;

                device_information__init(&deviceInfo);
                deviceInfo.serial_number = AMA_SERIAL_NUM;
                deviceInfo.name =nvrec_dev_get_bt_name();

                deviceInfo.n_supported_transports  = 1;
                if(ai_struct.transport_type == AI_TRANSPORT_SPP)
                    deviceInfo.supported_transports = &transport[0];
                else if(ai_struct.transport_type == AI_TRANSPORT_BLE)
                    deviceInfo.supported_transports = &transport[1];
                else
                    TRACE("%s connect type error %d", __func__, ai_struct.transport_type);
                
                deviceInfo.device_type = AMA_DEVICE_TYPE;
                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__GET_DEVICE_CONFIGURATION:
            {
                DeviceConfiguration deviceConfiguration;

                TRACE("COMMAND__GET_DEVICE_CONFIGURATION");
                controlMessage.command =  msg->command; 
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS; 
                responseMessage.payload_case = RESPONSE__PAYLOAD_DEVICE_CONFIGURATION; 
                responseMessage.device_configuration = &deviceConfiguration;
                
                device_configuration__init(&deviceConfiguration);
#ifdef IS_MULTI_AI_ENABLED
                if ((ai_manager_get_ama_assistant_enable_status() == 0) || \
                            (ai_manager_spec_get_status_is_in_invalid() == true))
                {
                    TRACE("current ai status not in ama");
                    deviceConfiguration.needs_assistant_override = 1;
                }
                else
#endif
                {
                    TRACE("current ai status in ama");
                    deviceConfiguration.needs_assistant_override = 0;
                }
                deviceConfiguration.needs_setup = 0; 
                
                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__UPGRADE_TRANSPORT:
            {
                ConnectionDetails connection_details;
                
                TRACE("COMMAND__UPGRADE_TRANSPORT");
                if (TRANSPORT__BLUETOOTH_RFCOMM == msg->upgrade_transport->transport)
                {
                    uint8_t tem_buf[6];
                    nvrec_dev_get_btaddr(dev_bt_addr);
                    memcpy(tem_buf,dev_bt_addr,6);
                    for(index=0; index<6; index++){
                        dev_bt_addr[index]=tem_buf[5-index];
                    }
                    DUMP8("%02x ",dev_bt_addr,6);

                    controlMessage.command =command;
                    controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                
                    controlMessage.response = &responseMessage;
                    responseMessage.error_code = ERROR_CODE__SUCCESS; 
                    responseMessage.payload_case = RESPONSE__PAYLOAD_CONNECTION_DETAILS; 
                    responseMessage.connection_details=&connection_details;

                    connection_details__init(&connection_details);
                    connection_details.identifier.len = 6;
                    connection_details.identifier.data=(uint8_t    *)dev_bt_addr;
                    
                    size = control_envelope__get_packed_size(&controlMessage);
                }
                break;
            }

            case COMMAND__SWITCH_TRANSPORT:
            {
                TRACE("COMMAND__SWITCH_TRANSPORT");
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__START_SETUP:
            {
                TRACE("COMMAND__START_SETUP");
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__COMPLETE_SETUP:
            {
                TRACE("COMMAND__COMPLETE_SETUP");
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                size = control_envelope__get_packed_size(&controlMessage);
#ifdef IS_MULTI_AI_ENABLED
                if (ai_manager_get_ama_assistant_enable_status() == 0)
                {
                    TRACE("current ama assistant has been overided");
                    return;
                }
                if ((ai_manager_get_current_spec() == AI_SPEC_GSOUND))
                {
                    ai_manager_set_current_spec(AI_SPEC_AMA, 1);
                    gsound_enable_handler(false);
                }
                else
                {
                    ai_manager_set_current_spec(AI_SPEC_AMA, 1);
                }
#endif
                break;
            }

            case COMMAND__NOTIFY_SPEECH_STATE:
            {
                /*
                IDLE=0;
                LISTENING=1;
                PROCESSING=2;
                SPEAKING=3;
                */
                TRACE("COMMAND__NOTIFY_SPEECH_STATE state %d", msg->notify_speech_state->state);

                if(SPEECH_STATE__LISTENING == msg->notify_speech_state->state)
                    ai_audio_stream_allowed_to_send_set(true);

                if(msg->notify_speech_state->state &&
                    (ai_struct.ai_stream.ai_speech_type != TYPE__PROVIDE_SPEECH && msg->notify_speech_state->state != SPEECH_STATE__SPEAKING)) {
                    osTimerStop(app_ama_speech_state_timeout_timer_id);
                    osTimerStart(app_ama_speech_state_timeout_timer_id, APP_AMA_SPEECH_STATE_TIMEOUT_INTERVEL);    
                } else {
                    osTimerStop(app_ama_speech_state_timeout_timer_id); 
                }
                
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;
                
                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__KEEP_ALIVE:
            {
                TRACE("COMMAND__KEEP_ALIVE");
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;
                
                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__PROVIDE_SPEECH:
            {
                TRACE("COMMAND__PROVIDE_SPEECH");
                SpeechProvider speech_provider = SPEECH_PROVIDER__INIT;
                SpeechSettings settings = SPEECH_SETTINGS__INIT; 
                Dialog dialog = DIALOG__INIT;
                
                ai_struct.ai_stream.ai_stream_opened = true;
                ai_struct.ai_stream.ai_speech_type = TYPE__PROVIDE_SPEECH;
                ai_struct.tx_credit = MAX_TX_CREDIT;
                ama_send_event_message(true);//REOPEN MIC
                app_ai_spp_deinit_tx_buf();

                settings.audio_profile = AUDIO_PROFILE__NEAR_FIELD;
                if(ai_struct.encode_type == ENCODING_ALGORITHM_OPUS)
                {
#if AI_32KBPS_VOICE
                    settings.audio_format = AUDIO_FORMAT__OPUS_16KHZ_32KBPS_CBR_0_20MS; // 16KHZ : sample rate     32KBPS: opus encode bitrate
#else
                    settings.audio_format = AUDIO_FORMAT__OPUS_16KHZ_16KBPS_CBR_0_20MS;
#endif    
                } else if(ai_struct.encode_type == ENCODING_ALGORITHM_SBC)
                    settings.audio_format = AUDIO_FORMAT__MSBC;
                else
                    settings.audio_format = AUDIO_FORMAT__PCM_L16_16KHZ_MONO;
                settings.audio_source = AUDIO_SOURCE__STREAM;

                dialog_id_set(msg->provide_speech->dialog->id);
                dialog.id = msg->provide_speech->dialog->id;
                
                speech_provider.dialog = &dialog;
                speech_provider.speech_settings = &settings;
                
                controlMessage.command = command; 
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD_SPEECH_PROVIDER;
                responseMessage.speech_provider = &speech_provider;
                
                size = control_envelope__get_packed_size(&controlMessage);

                ai_audio_stream_allowed_to_send_set(true);
                ai_mailbox_put(SIGN_AI_WAKEUP, 0);
                break;
            }

            case COMMAND__ENDPOINT_SPEECH:
            {
                TRACE("COMMAND__ENDPOINT_SPEECH");
                
                app_ai_control_mute_a2dp_state(false);
                ai_audio_stream_allowed_to_send_set(false);            
                ama_send_event_message(false);

                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;
                
                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }
        //add oct 27 by tj
            case COMMAND__STOP_SPEECH:
            {
                TRACE("COMMAND__STOP_SPEECH");
                
                app_ai_control_mute_a2dp_state(false);
                ai_audio_stream_allowed_to_send_set(false);            
                ama_send_event_message(false);
                
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;
                
                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__OVERRIDE_ASSISTANT:
            {
                TRACE("COMMAND__OVERRIDE_ASSISTANT");
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;

                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                size = control_envelope__get_packed_size(&controlMessage);
                ama_control_stream_send(&controlMessage, size);
#ifdef IS_MULTI_AI_ENABLED
                //notify alexa app to revert back to ama again
                if (msg->override_assistant->error_code == ERROR_CODE__SUCCESS)
                {
                    TRACE("enable switch back to alexa");
                    ama_notify_device_configuration(0);
                    if ((ai_manager_get_current_spec() == AI_SPEC_GSOUND))
                    {
                        ai_manager_set_current_spec(AI_SPEC_AMA, 1);
                        gsound_enable_handler(false);
                    }
                    else
                    {
                        ai_manager_set_current_spec(AI_SPEC_AMA, 1);
                    }
                }
                else
                {
                    TRACE("cancel switch back to alexa");
                }
#endif
                isToResponse = false;
                break;
            }

            case COMMAND__SYNCHRONIZE_SETTINGS:
            {
                TRACE("COMMAND__SYNCHRONIZE_SETTINGS");
                
                ai_struct.ai_stream.ai_setup_complete = true;
                
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;
                
                size = control_envelope__get_packed_size(&controlMessage);
                TRACE("%s size=%d", __func__, size);
                break;
            }

            case COMMAND__GET_STATE:
            {
                TRACE("COMMAND__GET_STATE");

                break;
            }

            case COMMAND__SET_STATE:
            {
                TRACE("COMMAND__SET_STATE state %d", msg->set_state->state->feature);

                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;
                
                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__ISSUE_MEDIA_CONTROL:
            {
                TRACE("COMMAND__ISSUE_MEDIA_CONTROL control %d", msg->issue_media_control->control);

                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;
                
                size = control_envelope__get_packed_size(&controlMessage);
                implement_ama_media_control(msg->issue_media_control);
                osDelay(100);
                break;
            }

            case COMMAND__FORWARD_AT_COMMAND:
            {
                TRACE("COMMAND__FORWARD_AT_COMMAND");
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;
                
                size = control_envelope__get_packed_size(&controlMessage);
                break;

            }
            case COMMAND__SYNCHRONIZE_STATE:
            {
                /*
                STATE__VALUE__NOT_SET = 0,
                STATE__VALUE_BOOLEAN = 2,
                STATE__VALUE_INTEGER = 3
                */
                TRACE("COMMAND__SYNCHRONIZE_STATE %d", msg->synchronize_state->state->value_case);
                //ama_control_send_transport_ver_exchange();
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;
                
                size = control_envelope__get_packed_size(&controlMessage);
                break;

            }

            default:
            {
                TRACE("AMA NOT SUPPORT CMD!!!");
                responseMessage.error_code = ERROR_CODE__UNSUPPORTED;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                controlMessage.command = command; 
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;

                size = control_envelope__get_packed_size(&controlMessage);
                isToResponse = true;
                break;
            }
        }
    }
    
    if (isToResponse) {    
        ama_control_stream_send(&controlMessage, size);
    }
}


void ama_control_stream_handler(uint8_t* ptr, uint32_t len)
{
    static uint8_t out[128];

    uint32_t lock;

    lock = int_lock();
    AMA_STACK_ALLOC_INIT_POOL(out, sizeof(out));
    ControlEnvelope* msg = control_envelope__unpack(&ama_stack, 
        len, ptr);
    int_unlock(lock);

    ama_cmd_handler(msg, NULL);
}

void ama_reset_connection (void)
{
    size_t size = 0;
    struct  _ResetConnection
    {
      ProtobufCMessage base;
      uint32_t timeout;
      protobuf_c_boolean force_disconnect;
    };

    ResetConnection req = RESET_CONNECTION__INIT;
    req.timeout = 1;
    req.force_disconnect = true;

    ControlEnvelope controlMessage = CONTROL_ENVELOPE__INIT;
    controlMessage.command = COMMAND__RESET_CONNECTION;
    controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESET_CONNECTION;
    controlMessage.reset_connection = &req;

    size = control_envelope__get_packed_size(&controlMessage);
    TRACE("%s <------> size = %d", __func__, size);
    ama_control_stream_send(&controlMessage, size);
}

void ama_notify_device_configuration (uint8_t is_assistant_override)
{
#ifdef IS_MULTI_AI_ENABLED
    if (ai_manager_get_spec_connected_status(AI_SPEC_AMA) == 1)
#endif
    {
        size_t size = 0;
        DeviceConfiguration devicecfg;
        device_configuration__init(&devicecfg);
        devicecfg.needs_assistant_override = is_assistant_override;
        devicecfg.needs_setup = 0;

        NotifyDeviceConfiguration req = NOTIFY_DEVICE_CONFIGURATION__INIT;
        req.device_configuration = &devicecfg;

        ControlEnvelope controlMessage = CONTROL_ENVELOPE__INIT;
        controlMessage.command = COMMAND__NOTIFY_DEVICE_CONFIGURATION;
        controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_NOTIFY_DEVICE_CONFIGURATION;
        controlMessage.notify_device_configuration = &req;

        size = control_envelope__get_packed_size(&controlMessage);
        TRACE("%s<------>size=%d", __func__, size);
        ama_control_stream_send(&controlMessage, size);
    }

#ifdef IS_MULTI_AI_ENABLED
#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
    if (is_assistant_override)
    {
        ai_manager_set_ama_assistant_enable_status(0);
        ama_status_update_start = true;
        osTimerStart(ama_ind_rsp_timeout_timer, AMA_INDICATION_RSP_TIMEOUT);
    }
    else
    {
        ai_manager_set_ama_assistant_enable_status(1);
        ama_status_update_start = false;
    }
#endif
#endif

}

//devices request speech
void ama_start_speech_request(void) 
{
    size_t size = 0;

    TRACE("%s", __func__);
    ai_transport_fifo_deinit();
    ai_struct.ai_stream.ai_speech_type = TYPE__NORMAL_SPEECH;
    ai_struct.tx_credit = MAX_TX_CREDIT;

    ControlEnvelope controlMessage;
    StartSpeech speech;
    SpeechSettings settings; 
    SpeechInitiator initiator; 
    Dialog  dialog;
    SpeechInitiator__WakeWord wake_Word;

    control_envelope__init(&controlMessage);
    controlMessage.command = COMMAND__START_SPEECH; 
    controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_START_SPEECH;
    
    start_speech__init(&speech);

    speech_settings__init(&settings);
    if(ai_struct.wake_up_type == TYPE__KEYWORD_WAKEUP) {
        settings.audio_profile = AUDIO_PROFILE__FAR_FIELD;
    } else {
        settings.audio_profile = AUDIO_PROFILE__NEAR_FIELD;
    }

    if(ai_struct.encode_type == ENCODING_ALGORITHM_OPUS)
    {
#if AI_32KBPS_VOICE
        settings.audio_format = AUDIO_FORMAT__OPUS_16KHZ_32KBPS_CBR_0_20MS;
#else
        settings.audio_format = AUDIO_FORMAT__OPUS_16KHZ_16KBPS_CBR_0_20MS;
#endif    
    } else if(ai_struct.encode_type == ENCODING_ALGORITHM_SBC)
        settings.audio_format = AUDIO_FORMAT__MSBC;
    else
        settings.audio_format = AUDIO_FORMAT__PCM_L16_16KHZ_MONO;
    settings.audio_source = AUDIO_SOURCE__STREAM;

    if(settings.audio_format==AUDIO_FORMAT__OPUS_16KHZ_16KBPS_CBR_0_20MS){
        ama_each_stream_len=43;
    }else if(settings.audio_format==AUDIO_FORMAT__OPUS_16KHZ_32KBPS_CBR_0_20MS){
        ama_each_stream_len=83;
    }else if(settings.audio_format==AUDIO_FORMAT__MSBC){
        ama_each_stream_len=60;
    }else{
        ama_each_stream_len=171;
    }
    
    speech_initiator__wake_word__init(&wake_Word);
    if(ai_struct.wake_up_type == TYPE__KEYWORD_WAKEUP) {
        if(settings.audio_format==AUDIO_FORMAT__OPUS_16KHZ_16KBPS_CBR_0_20MS) {
#ifdef __KNOWLES
            wake_Word.start_index_in_samples = 0;
            wake_Word.end_index_in_samples = 0; 
#else
            wake_Word.start_index_in_samples = 0;
            wake_Word.end_index_in_samples = 12000; 
#endif
        } else if(settings.audio_format==AUDIO_FORMAT__MSBC) {
            wake_Word.start_index_in_samples = 0;
            wake_Word.end_index_in_samples = 12000;
        } else {
            wake_Word.start_index_in_samples = 200;
            wake_Word.end_index_in_samples = 24000;
        }
    } else {
        wake_Word.start_index_in_samples = 0;
        wake_Word.end_index_in_samples = 0;
    }

    speech_initiator__init(&initiator);
    if(ai_struct.wake_up_type == TYPE__KEYWORD_WAKEUP) {
        initiator.type = SPEECH_INITIATOR__TYPE__WAKEWORD;
    } else if(ai_struct.wake_up_type == TYPE__PRESS_AND_HOLD) {
        initiator.type = SPEECH_INITIATOR__TYPE__PRESS_AND_HOLD;
    } else if(ai_struct.wake_up_type == TYPE__TAP) {
        initiator.type = SPEECH_INITIATOR__TYPE__TAP;
    }

    initiator.wake_word = &wake_Word;

    dialog__init(&dialog);
    dialog.id = start_dialog_id_get();
    TRACE("START....dialog = %x", dialog.id);

    speech.initiator = &initiator;
    speech.settings = &settings; 
    speech.dialog = &dialog;
    speech.suppressearcon = false;

    controlMessage.start_speech = &speech; 

    size = control_envelope__get_packed_size(&controlMessage);

    TRACE("<------>size = %d", size);

    ama_control_stream_send(&controlMessage, size);

    //ai_audio_stream_allowed_to_send_set(true);
}

void ama_stop_speech_request (ama_stop_reason stop_reason) 
{
    size_t size = 0;

    Dialog dialog = DIALOG__INIT;
    dialog.id = dialog_id_get();
    TRACE("%s dialog: %d", __func__, dialog.id);
    
    StopSpeech stop_speech = STOP_SPEECH__INIT;
    stop_speech.dialog = &dialog;
    if(stop_reason==USER_CANCEL){    
        stop_speech.error_code = ERROR_CODE__USER_CANCELLED;
    }else if(stop_reason==TIME_OUT){
        stop_speech.error_code = ERROR_CODE__INTERNAL;
    }else{
        stop_speech.error_code = ERROR_CODE__UNKNOWN;
    }
    
    ControlEnvelope controlMessage = CONTROL_ENVELOPE__INIT;
    controlMessage.command = COMMAND__STOP_SPEECH;
    controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_STOP_SPEECH;
    controlMessage.stop_speech = &stop_speech;

    size = control_envelope__get_packed_size(&controlMessage);
    TRACE("<------>size = %d", size);
    ama_control_stream_send(&controlMessage, size);
}


void ama_endpoint_speech_request(uint32_t dialog_id)
{
    size_t size = 0;

    Dialog dialog = DIALOG__INIT;
    dialog.id = dialog_id_get();
    TRACE("%s dialog: %d", __func__, dialog.id);
    
    EndpointSpeech endpoint_speech = ENDPOINT_SPEECH__INIT;
    endpoint_speech.dialog = &dialog;
    
    ControlEnvelope controlMessage = CONTROL_ENVELOPE__INIT;
    controlMessage.command = COMMAND__ENDPOINT_SPEECH;
    controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_ENDPOINT_SPEECH;
    controlMessage.endpoint_speech = &endpoint_speech;

    size = control_envelope__get_packed_size(&controlMessage);
    TRACE("<------>size = %d", size);
    ama_control_stream_send(&controlMessage, size);
}

void ama_send_event_message_callback_init(int (*cb)(bool))
{
    ama_send_event_message=cb;
}

#endif

