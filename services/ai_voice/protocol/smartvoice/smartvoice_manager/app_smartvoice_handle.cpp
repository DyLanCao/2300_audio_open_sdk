#include "hal_trace.h"
#include "hal_timer.h"
#include "cmsis_os.h"
#include "cqueue.h"
#include <stdlib.h>
#include "app_audio.h"
#include "app_ble_mode_switch.h"
#include "gapm_task.h"

#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "hal_location.h"
#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif


//#include "aes.h"
#include "tgt_hardware.h"

#include "nvrecord.h"
#include "nvrecord_env.h"

#include "ai_transport.h"
#include "ai_manager.h"
#include "ai_control.h"
#include "ai_spp.h"
#include "ai_thread.h"
#include "app_ai_ble.h"
#include "app_ai_voice.h"
#include "app_smartvoice_handle.h"
#include "app_sv_cmd_code.h"
#include "app_sv_cmd_handler.h"
#include "app_sv_data_handler.h"
#include "app_smartvoice_ble.h"
#include "app_smartvoice_bt.h"
#include "voice_compression.h"

extern uint8_t bt_addr[6];

#define APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS 5000

#if VOB_ENCODING_ALGORITHM == ENCODING_ALGORITHM_OPUS
#define SMARTVOICE_SYSTEM_FREQ       APP_SYSFREQ_208M   
#else
#define SMARTVOICE_SYSTEM_FREQ       APP_SYSFREQ_104M 
#endif

typedef struct
{
    CQueue      encodedDataBufQueue;
    uint8_t*    encodedDataBuf;
	uint8_t		isAudioAlgorithmEngineReseted;
    uint8_t*    tmpBufForEncoding;
    uint8_t*    tmpBufForXfer;
    
    uint32_t    sentDataSizeWithInFrame;
    uint8_t     seqNumWithInFrame;    
    CQueue      pcmDataBufQueue;
    uint8_t*    pcmDataBuf;    
    uint8_t*    tmpPcmDataBuf;    


} APP_SMARTVOICE_AUDIO_STREAM_ENV_T;           

typedef enum
{
	CLOUD_PLATFORM_0,
	CLOUD_PLATFORM_1,	
	CLOUD_PLATFORM_2,
} CLOUD_PLATFORM_E;

typedef struct {
    volatile uint8_t                     isSvStreamRunning   : 1;
    volatile uint8_t                     isDataXferOnGoing   : 1;
    volatile uint8_t                     isPlaybackStreamRunning : 1;
    volatile uint8_t            		 isThroughputTestOn  : 1;
    uint8_t                     reserve             : 4;
    uint8_t                     connType;   
    uint8_t                     conidx;
    uint16_t                    smartVoiceDataSectorSize;
    CLOUD_PLATFORM_E            currentPlatform;
    
} smartvoice_context_t;

typedef struct {
    uint8_t           voiceStreamPathToUse;
} START_VOICE_STREAM_REQ_T;

typedef struct {
    uint8_t           voiceStreamPathToUse;
} START_VOICE_STREAM_RSP_T;

typedef struct {
    uint8_t           cloudMusicPlayingControlReq;
} CLOUD_MUSIC_PLAYING_CONTROL_REQ_T;

typedef struct {
    uint8_t           cloudPlatform;
} CLOUD_PLATFORM_CHOOSING_REQ_T;

static smartvoice_context_t                 smartvoice_env;
#if (MIX_MIC_DURING_MUSIC)
static uint32_t more_mic_data(uint8_t* ptrBuf, uint32_t length);
#endif

extern void app_hfp_enable_audio_link(bool isEnable);
void app_bt_stream_copy_track_one_to_two_16bits(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len);

#define APP_SCNRSP_DATA         "\x11\x07\xa2\xaf\x34\xd6\xba\x17\x53\x8b\xd4\x4d\x3c\x8a\xdc\x0c\x3e\x26"
#define APP_SCNRSP_DATA_LEN     (18)

uint32_t app_sv_start_advertising(void *param1, uint32_t param2)
{
    struct gapm_start_advertise_cmd *cmd = (struct gapm_start_advertise_cmd*)param1;
        
    cmd->info.host.adv_data_len = 0;
    cmd->info.host.scan_rsp_data_len = 0;
     cmd->isIncludedFlags = true;
     
    memcpy(&cmd->info.host.scan_rsp_data[cmd->info.host.scan_rsp_data_len],
           APP_SCNRSP_DATA, APP_SCNRSP_DATA_LEN);
    cmd->info.host.scan_rsp_data_len += APP_SCNRSP_DATA_LEN;
	
    return 0;
}

uint32_t app_sv_voice_connected(void *param1, uint32_t param2)
{
    TRACE("%s connect_type %d",__func__, param2);
    
    ai_set_power_key_start();
    ai_struct.transport_type = param2;

    if(param2 == AI_TRANSPORT_SPP){
        ai_data_transport_init(app_ai_spp_send);
        ai_cmd_transport_init(app_ai_spp_send);
        updata_transfer_type();
        
        //if now spp connectedd, stop ble advertising;
        ble_stop_activities();
    }
#ifdef __IAG_BLE_INCLUDE__
    else if(param2 == AI_TRANSPORT_BLE){
        ai_data_transport_init(app_sv_send_data_via_notification);
        ai_cmd_transport_init(app_sv_send_cmd_via_notification);
        //if ble connected, close spp;
		app_ai_spp_disconnlink();
    }
#endif

    app_sv_data_reset_env();
    ai_struct.ai_stream.ai_stream_opened=false;
    ai_struct.ai_stream.ai_stream_running=false;
    ai_struct.ai_stream.ai_setup_complete = true;
    
#if (IS_MULTI_AI_ENABLED)
    ai_voicekey_save_status(true);
    ai_manager_update_spec_table_connected_status_value(AI_SPEC_BES, 1);
#endif

    return 0;
}

uint32_t app_sv_voice_disconnected(void *param1, uint32_t param2)
{
    TRACE("%s transport_type %d",__func__, ai_struct.transport_type);
	
    if (AI_TRANSPORT_BLE == param2) {
        app_bt_allow_sniff(BT_DEVICE_ID_1);
    }
	app_sv_data_reset_env();
    app_sv_stop_throughput_test();
    
	app_ai_control_mute_a2dp_state(false);
	
	ai_struct.transport_type = AI_TRANSPORT_IDLE;
    ai_struct.ai_stream.ai_setup_complete = false;

	clean_transfer_type();
	app_ai_voice_on_off_control(false);
	
	ai_set_power_key_stop();
	ai_transport_fifo_deinit();
    
#ifdef IS_MULTI_AI_ENABLED
    ai_manager_update_spec_table_connected_status_value(AI_SPEC_BES, 0);
#endif

    return 0;
}

uint32_t app_sv_voice_send_done(void *param1, uint32_t param2)
{
    TRACE("%s", __func__);
    return 0;
}

void app_smartvoice_start_xfer(void)
{
    TRACE("%s", __func__);
    
#if IS_ENABLE_START_DATA_XFER_STEP
    APP_TENCENT_SV_START_DATA_XFER_T req;
    memset((uint8_t *)&req, 0, sizeof(req));
    req.isHasCrcCheck = false;
    app_sv_start_data_xfer(&req);
#else
    app_sv_kickoff_dataxfer();
#endif
}

void app_smartvoice_stop_xfer(void)
{
#if 0  
    app_sv_update_conn_parameter(SV_LOW_SPEED_BLE_CONNECTION_INTERVAL_MIN_IN_MS, 
        SV_LOW_SPEED_BLE_CONNECTION_INTERVAL_MAX_IN_MS, 
        SV_LOW_SPEED_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS,0);
#endif

#if IS_ENABLE_START_DATA_XFER_STEP     
    APP_SV_STOP_DATA_XFER_T req;
    memset((uint8_t *)&req, 0, sizeof(req));
    req.isHasWholeCrcCheck = false;
    app_sv_stop_data_xfer(&req);  
#else
   TRACE("%s", __func__);
    app_sv_stop_dataxfer();
#endif
}

void app_sv_data_xfer_started(APP_SV_CMD_RET_STATUS_E retStatus)
{
    TRACE("%s ret %d", __func__, retStatus);
}

void app_sv_data_xfer_stopped(APP_SV_CMD_RET_STATUS_E retStatus)
{
    app_hfp_enable_audio_link(false);
    TRACE("%s ret %d", __func__, retStatus);
}

void app_sv_start_voice_stream_via_ble(void)
{
    TRACE("%s", __func__);
    
    if (AI_TRANSPORT_BLE == ai_struct.transport_type) {
        if (!ai_struct.ai_stream.ai_stream_opened) {
            START_VOICE_STREAM_REQ_T req;
            req.voiceStreamPathToUse = APP_SMARTVOICE_PATH_TYPE_BLE;
            app_sv_send_command(OP_START_SMART_VOICE_STREAM, (uint8_t *)&req, sizeof(req));
        } else {   
            TRACE("%s ai_stream has open", __func__);
        }
    } else {   
        TRACE("%s transport_type is error %d", __func__, ai_struct.transport_type);
    }
}

void app_sv_start_voice_stream_via_spp(void)
{
    TRACE("%s", __func__);
    
    if (AI_TRANSPORT_SPP == ai_struct.transport_type) {
        if (!ai_struct.ai_stream.ai_stream_opened) {
            START_VOICE_STREAM_REQ_T req;
            req.voiceStreamPathToUse = APP_SMARTVOICE_PATH_TYPE_SPP;
            app_sv_send_command(OP_START_SMART_VOICE_STREAM, (uint8_t *)&req, sizeof(req));
        } else {   
            TRACE("%s ai_stream has open", __func__);
        }
    } else {   
        TRACE("%s transport_type is error %d", __func__, ai_struct.transport_type);
    }
}

void app_sv_start_voice_stream_via_hfp(void)
{
    if (AI_TRANSPORT_IDLE != ai_struct.transport_type) {
        TRACE("%s", __func__);
        app_hfp_enable_audio_link(true);
        START_VOICE_STREAM_REQ_T req;
        req.voiceStreamPathToUse = APP_SMARTVOICE_PATH_TYPE_HFP;
        app_sv_send_command(OP_START_SMART_VOICE_STREAM, (uint8_t *)&req, sizeof(req));
    }
}

uint32_t app_sv_config_mtu(void *param1, uint32_t param2)  
{
    TRACE("%s is %d", __func__, param2);
    ai_struct.txbuff_send_max_size = (param2 - 3) - SMARTVOICE_CMD_CODE_SIZE - SMARTVOICE_ENCODED_DATA_SEQN_SIZE;
	TRACE("ai_struct.txbuff_send_max_size is %d", ai_struct.txbuff_send_max_size);
	
    return 0;
}


uint32_t app_sv_handle_init(void *param1, uint32_t param2)
{
    TRACE("%s", __func__);

    memset(&smartvoice_env,         0x00, sizeof(smartvoice_env));

    app_sv_cmd_handler_init();
    app_sv_data_reset_env();

	app_ai_register_ble_tx_done(NULL);
	app_ai_register__spp_tx_done(NULL);
    
    app_sv_throughput_test_init();
	
    return 0;
}

void app_sv_start_voice_stream(void)
{
    TRACE("%s ai_stream_opened %d", __func__, ai_struct.ai_stream.ai_stream_opened);
    
    if (!ai_struct.ai_stream.ai_stream_opened) {
        app_bt_stop_sniff(BT_DEVICE_ID_1);
        app_bt_stay_active(BT_DEVICE_ID_1);
        TRACE("Start smart voice stream.");
        ai_struct.ai_stream.ai_stream_opened = true;

        app_ai_voice_start_mic_stream();
    }
}

void app_sv_stop_stream(void)
{
    if (AI_TRANSPORT_IDLE != ai_struct.transport_type) {
        if (ai_struct.ai_stream.ai_stream_opened) {
            app_ai_control_mute_a2dp_state(false);
            ai_audio_stream_allowed_to_send_set(false);
            app_ai_voice_stop_mic_stream();
        	app_bt_allow_sniff(BT_DEVICE_ID_1);
        }
    }
}

uint32_t app_sv_stop_voice_stream(void *param1, uint32_t param2)
{
    TRACE("%s ai_stream_opened %d", __func__, ai_struct.ai_stream.ai_stream_opened);
    if (AI_TRANSPORT_IDLE != ai_struct.transport_type) {
        if (ai_struct.ai_stream.ai_stream_opened) {
            app_ai_control_mute_a2dp_state(false);
            ai_audio_stream_allowed_to_send_set(false);
            app_ai_voice_stop_mic_stream();
        	app_bt_allow_sniff(BT_DEVICE_ID_1);
            app_sv_send_command(OP_STOP_SMART_VOICE_STREAM, NULL, 0);
        }
    }
    return 0;
}

extern "C" uint8_t is_sco_mode (void);
uint32_t app_sv_wake_up(void *param1, uint32_t param2)
{
    APP_KEY_STATUS *status = (APP_KEY_STATUS *)param1;

	TRACE("%s transport_type %d event %d",__func__, ai_struct.transport_type, status->event);
    if (ai_struct.transport_type == AI_TRANSPORT_IDLE){
       TRACE("%s not ai connect", __func__);
       return 1;
    }
       
    if (is_sco_mode()){
        TRACE("%s is in sco mode", __func__);
        return 2;
    }

    switch(status->event)
    {
        case APP_KEY_EVENT_FIRST_DOWN:
            TRACE("smartvoice_wakeup_app_key!");
            app_ai_spp_deinit_tx_buf();
            
            if(AI_TRANSPORT_BLE == ai_struct.transport_type) {     //使用ble
                app_smartvoice_switch_ble_stream_status();    //send start cmd;
            }
            else if(AI_TRANSPORT_SPP == ai_struct.transport_type) {    //使用spp
                app_smartvoice_switch_spp_stream_status();  //send start cmd;
            } else {
                TRACE("it is error, not ble or spp connected");
                return 3;
            }
            ai_mailbox_put(SIGN_AI_WAKEUP, 0);
			app_sv_start_voice_stream();
            break;
        case APP_KEY_EVENT_UP:
            TRACE("smartvoice_stop_app_key");
            ai_function_handle(CALLBACK_STOP_SPEECH, NULL, 0);
            break;
    }
	
    return 0;
}

uint32_t app_sv_enable_handler(void *param1, uint32_t param2)
{
    TRACE("%s %d", __func__, param2);
    return 0;
}

#define SMARTVOICE_TRANSFER_DATA_MAX_LEN 400
uint32_t app_sv_stream_init(void *param1, uint32_t param2)
{
	app_capture_audio_mempool_get_buff(&(ai_struct.ai_stream.tmpBufForXfer),	 
		SMARTVOICE_TRANSFER_DATA_MAX_LEN);
    
	ai_transport_fifo_deinit();
    return 0;
}

uint32_t app_sv_stream_deinit(void *param1, uint32_t param2)
{
	ai_transport_fifo_deinit();
	ai_struct.ai_stream.ai_stream_opened=false;
	ai_set_power_key_start();
	
    return 0;
}

void app_sv_start_voice_stream_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    uint8_t voicePathType = ((START_VOICE_STREAM_REQ_T *)ptrParam)->voiceStreamPathToUse;
    TRACE("Received the wakeup req from the remote dev. Voice Path type %d", voicePathType);
    START_VOICE_STREAM_RSP_T rsp;
    rsp.voiceStreamPathToUse = voicePathType;
    
    if (!ai_struct.ai_stream.ai_stream_opened)
    {     
        if ((APP_SMARTVOICE_PATH_TYPE_BLE == voicePathType) ||
            (APP_SMARTVOICE_PATH_TYPE_SPP == voicePathType)) {
            if (is_sco_mode()) {
                TRACE("%s CMD_HANDLING_FAILED", __func__);
                app_sv_send_response_to_command(funcCode, CMD_HANDLING_FAILED, (uint8_t *)&rsp, sizeof(rsp));
            } else {
                app_sv_send_response_to_command(funcCode, NO_ERROR, (uint8_t *)&rsp, sizeof(rsp));
                
                app_sv_start_voice_stream();
                // start the data xfer
                app_smartvoice_start_xfer(); 
            }
        } else if ((APP_SMARTVOICE_PATH_TYPE_HFP == voicePathType) &&
            !is_sco_mode()) {
            app_sv_send_response_to_command(funcCode, CMD_HANDLING_FAILED, (uint8_t *)&rsp, sizeof(rsp));           
        } else {
            ai_struct.ai_stream.ai_stream_opened = true;
            app_sv_send_response_to_command(funcCode, NO_ERROR, (uint8_t *)&rsp, sizeof(rsp));
        }
    } else {
        app_sv_send_response_to_command(funcCode, DATA_XFER_ALREADY_STARTED, (uint8_t *)&rsp, sizeof(rsp));
    }
}

static void app_sv_start_voice_stream_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE("%s ret %d stream %d", __func__, retStatus, ai_struct.ai_stream.ai_stream_opened);
    if (NO_ERROR == retStatus) {
        START_VOICE_STREAM_RSP_T* rsp = (START_VOICE_STREAM_RSP_T *)ptrParam;
        TRACE("%s path %d", __func__, rsp->voiceStreamPathToUse);
        if ((APP_SMARTVOICE_PATH_TYPE_BLE == rsp->voiceStreamPathToUse) ||
            (APP_SMARTVOICE_PATH_TYPE_SPP == rsp->voiceStreamPathToUse)) {
            if (!ai_struct.ai_stream.ai_stream_opened) {                
                app_sv_start_voice_stream();
                // start the data xfer
                app_smartvoice_start_xfer();
            } else {
				app_smartvoice_start_xfer();				
			}
        }
    } else {
        // workaround, should be changed to false
        app_hfp_enable_audio_link(false);
    }
}

static void app_sv_stop_voice_stream_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE("Received the stop voice stream request from the remote dev.");
    if (ai_struct.ai_stream.ai_stream_opened)
    {
		app_sv_send_response_to_command(funcCode, NO_ERROR, NULL, 0);
        smartvoice_env.isDataXferOnGoing = false;
        app_sv_stop_stream();
        // Stop the data xfer
        app_smartvoice_stop_xfer(); 
    }
    else
    {
        app_sv_send_response_to_command(funcCode, DATA_XFER_NOT_STARTED_YET, NULL, 0);
    }
}

static void app_sv_stop_voice_stream_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE("Received the stop voice stream response %d from the remote dev.", retStatus);
    if ((NO_ERROR == retStatus) || (WAITING_RSP_TIMEOUT == retStatus))
    {
        app_sv_stop_stream();
        app_smartvoice_stop_xfer(); 
    }

    ai_struct.ai_stream.ai_stream_opened = false;
}

uint32_t app_voice_catpure_interval_in_ms(void)
{
	return VOB_VOICE_CAPTURE_INTERVAL_IN_MS;
}

void app_smartvoice_switch_hfp_stream_status(void)
{
    if (ai_struct.ai_stream.ai_stream_opened)
    {
        ai_function_handle(CALLBACK_STOP_SPEECH, NULL, 0);
    }
    else
    {
        app_sv_start_voice_stream_via_hfp();
    }
}

void app_smartvoice_switch_ble_stream_status(void)
{
    if (ai_struct.ai_stream.ai_stream_opened)
    {
        ai_function_handle(CALLBACK_STOP_SPEECH, NULL, 0);
    }
    else
    {
        app_sv_start_voice_stream_via_ble();
    }
}

void app_smartvoice_switch_spp_stream_status(void)
{
    if (ai_struct.ai_stream.ai_stream_opened)
    {
        ai_function_handle(CALLBACK_STOP_SPEECH, NULL, 0);
    }
    else
    {
        app_sv_start_voice_stream_via_spp();
    }
}

static void app_sv_cloud_music_playing_control_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE("Received the cloud music playing control request from the remote dev.");
}

static void app_sv_cloud_music_playing_control_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE("Music control returns %d", retStatus);
}

void app_sv_control_cloud_music_playing(CLOUD_MUSIC_CONTROL_REQ_E req)
{
    if (smartvoice_env.connType > 0)
    {
        if (ai_struct.ai_stream.ai_stream_opened)
        {
            CLOUD_MUSIC_PLAYING_CONTROL_REQ_T request;
            request.cloudMusicPlayingControlReq = (uint8_t)req;
            app_sv_send_command(OP_CLOUD_MUSIC_PLAYING_CONTROL, (uint8_t *)&request, sizeof(request));
        }
    }
}

static void app_sv_choose_cloud_platform(CLOUD_PLATFORM_E req)
{
    CLOUD_PLATFORM_CHOOSING_REQ_T request;
    request.cloudPlatform = req;
    app_sv_send_command(OP_CHOOSE_CLOUD_PLATFORM, (uint8_t *)&request, sizeof(request));
}

void app_sv_switch_cloud_platform(void)
{
    if (CLOUD_PLATFORM_0 == smartvoice_env.currentPlatform)
    {
        smartvoice_env.currentPlatform = CLOUD_PLATFORM_1;
    }
    else
    {
        smartvoice_env.currentPlatform = CLOUD_PLATFORM_0;
    }
    app_sv_choose_cloud_platform(smartvoice_env.currentPlatform);
}

static void app_sv_choose_cloud_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    
}

static void app_sv_spp_data_ack_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    TRACE("%s", __func__);
    if (AI_TRANSPORT_SPP == ai_struct.transport_type)
    {
        smartvoice_env.isDataXferOnGoing = false;
#if 0
        if (LengthOfCQueue(&(smartvoice_audio_stream.encodedDataBufQueue)) > 0)
        {
            app_smartvoice_send_encoded_audio_data();
        }
#endif
    }
}

static void app_sv_spp_data_ack_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{

}

static void app_sv_inform_algorithm_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
#if VOB_ENCODING_ALGORITHM==ENCODING_ALGORITHM_SBC  
    VOICE_SBC_CONFIG_INFO_T tInfo = 
    {
        ENCODING_ALGORITHM_SBC,  
        VOB_VOICE_CAPTURE_FRAME_CNT,
        VOB_VOICE_ENCODED_DATA_FRAME_SIZE,
        VOB_VOICE_PCM_FRAME_SIZE,
        
        VOICE_SBC_BLOCK_SIZE,
        VOICE_SBC_CHANNEL_COUNT     ,
        VOICE_SBC_CHANNEL_MODE 	    ,
        VOICE_SBC_BIT_POOL	 	    ,
        VOICE_SBC_SIZE_PER_SAMPLE   ,
        VOICE_SBC_SAMPLE_RATE	    ,
        VOICE_SBC_NUM_BLOCKS	    ,
        VOICE_SBC_NUM_SUB_BANDS	    ,
        VOICE_SBC_MSBC_FLAG		    ,
        VOICE_SBC_ALLOC_METHOD	    ,
    };
#else
    VOICE_OPUS_CONFIG_INFO_T tInfo = 
    {
        ENCODING_ALGORITHM_OPUS,        
        VOB_VOICE_ENCODED_DATA_FRAME_SIZE,
        VOB_VOICE_PCM_FRAME_SIZE,
        VOB_VOICE_CAPTURE_FRAME_CNT,

        VOICE_OPUS_CHANNEL_COUNT,
        VOICE_OPUS_COMPLEXITY,
        VOICE_OPUS_PACKET_LOSS_PERC,
        VOICE_SIZE_PER_SAMPLE,
        VOICE_OPUS_APP,
        VOICE_OPUS_BANDWIDTH,
        VOICE_OPUS_BITRATE,
        VOICE_OPUS_SAMPLE_RATE,
        VOICE_SIGNAL_TYPE,
        VOICE_FRAME_PERIOD,
        
        VOICE_OPUS_USE_VBR,
        VOICE_OPUS_CONSTRAINT_USE_VBR,
        VOICE_OPUS_USE_INBANDFEC,
        VOICE_OPUS_USE_DTX,
    };
#endif

    TRACE("Inform the information of the used algorithm.");

    uint32_t sentBytes = 0;
    uint8_t tmpBuf[sizeof(tInfo)+1];
    // the first byte will tell the offset of the info data, in case that 
    // payloadPerPacket is smaller than the info size and multiple packets have
    // to be sent
    uint32_t payloadPerPacket = smartvoice_env.smartVoiceDataSectorSize - 1;
    while ((sizeof(tInfo) - sentBytes) >= payloadPerPacket)
    {
        tmpBuf[0] = sentBytes;
        memcpy(&tmpBuf[1], ((uint8_t *)&tInfo) + sentBytes, payloadPerPacket);
        app_sv_send_response_to_command(funcCode, NO_ERROR, tmpBuf, payloadPerPacket + 1);  

        sentBytes += payloadPerPacket;
    }

    if (sentBytes < sizeof(tInfo))
    {
        tmpBuf[0] = sentBytes;
        memcpy(&tmpBuf[1], ((uint8_t *)&tInfo) + sentBytes, sizeof(tInfo)-sentBytes);
        app_sv_send_response_to_command(funcCode, NO_ERROR, tmpBuf, sizeof(tInfo) - sentBytes + 1);  
    }
}

#if !DEBUG_BLE_DATAPATH
CUSTOM_COMMAND_TO_ADD(OP_CLOUD_MUSIC_PLAYING_CONTROL, app_sv_cloud_music_playing_control_handler, TRUE,  
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_cloud_music_playing_control_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_START_SMART_VOICE_STREAM, app_sv_start_voice_stream_handler, TRUE,  
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_start_voice_stream_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_STOP_SMART_VOICE_STREAM, app_sv_stop_voice_stream_handler, TRUE,  
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_stop_voice_stream_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_SPP_DATA_ACK, app_sv_spp_data_ack_handler, FALSE,      
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_spp_data_ack_rsp_handler);

#else
CUSTOM_COMMAND_TO_ADD(OP_CLOUD_MUSIC_PLAYING_CONTROL, app_sv_cloud_music_playing_control_handler, FALSE,  
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_cloud_music_playing_control_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_START_SMART_VOICE_STREAM, app_sv_start_voice_stream_handler, FALSE,  
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_start_voice_stream_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_STOP_SMART_VOICE_STREAM, app_sv_stop_voice_stream_handler, FALSE,  
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_stop_voice_stream_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_SPP_DATA_ACK, app_sv_spp_data_ack_handler, FALSE,      
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_spp_data_ack_rsp_handler);

#endif

CUSTOM_COMMAND_TO_ADD(OP_INFORM_ALGORITHM_TYPE, app_sv_inform_algorithm_handler, FALSE,      
    0,    NULL);


CUSTOM_COMMAND_TO_ADD(OP_CHOOSE_CLOUD_PLATFORM, app_sv_choose_cloud_handler, FALSE,  
    0,    NULL );

typedef enum
{
    PATTERN_RANDOM = 0,
    PATTERN_11110000,
    PATTERN_10101010,
    PATTERN_ALL_1,
    PATTERN_ALL_0,
    PATTERN_00001111,
    PATTERN_0101,
} THROUGHPUT_TEST_DATA_PATTER_E;

typedef enum
{
    UP_STREAM = 0,
    DOWN_STREAM
} THROUGHPUT_TEST_DATA_DIRECTION_E;

typedef enum
{
    WITHOUT_RSP = 0,
    WITH_RSP
} THROUGHPUT_TEST_RESPONSE_MODEL_E;


typedef struct
{
    uint8_t     dataPattern;
    uint16_t    lastTimeInSecond;
    uint16_t    dataPacketSize;
    uint8_t     direction;
    uint8_t     responseModel;
    uint8_t     isToUseSpecificConnParameter;
    uint16_t    minConnIntervalInMs;
    uint16_t    maxConnIntervalInMs;
    uint8_t     reserve[4];
} __attribute__((__packed__)) THROUGHPUT_TEST_CONFIG_T;

static THROUGHPUT_TEST_CONFIG_T throughputTestConfig;

#define APP_SV_THROUGHPUT_PRE_CONFIG_PENDING_TIME_IN_MS    2000
static void app_sv_throughput_pre_config_pending_timer_cb(void const *n);
osTimerDef (APP_SV_THROUGHPUT_PRE_CONFIG_PENDING_TIMER, app_sv_throughput_pre_config_pending_timer_cb); 
osTimerId   app_sv_throughput_pre_config_pending_timer_id = NULL;

static void app_sv_throughput_test_data_xfer_lasting_timer_cb(void const *n);
osTimerDef (APP_SV_THROUGHPUT_TEST_DATA_XFER_LASTING_TIMER, app_sv_throughput_test_data_xfer_lasting_timer_cb); 
osTimerId   app_sv_throughput_test_data_xfer_lasting_timer_id = NULL;


static uint8_t app_sv_throughput_datapattern[MAXIMUM_DATA_PACKET_LEN];

static void app_sv_throughput_test_data_xfer_lasting_timer_cb(void const *n)
{
    app_sv_send_command(OP_THROUGHPUT_TEST_DONE, NULL, 0);

    app_sv_stop_throughput_test();
}

static void app_sv_throughput_pre_config_pending_timer_cb(void const *n)
{
    // inform the configuration
    app_sv_send_command(OP_INFORM_THROUGHPUT_TEST_CONFIG, 
        (uint8_t *)&throughputTestConfig, sizeof(throughputTestConfig));

    if (UP_STREAM == throughputTestConfig.direction)
    {
        app_sv_throughput_test_transmission_handler(NULL, 0);
        osTimerStart(app_sv_throughput_test_data_xfer_lasting_timer_id, 
            throughputTestConfig.lastTimeInSecond*1000);
    }
}

void app_sv_throughput_test_init(void)
{
    app_sv_throughput_pre_config_pending_timer_id = 
        osTimerCreate(osTimer(APP_SV_THROUGHPUT_PRE_CONFIG_PENDING_TIMER), 
        osTimerOnce, NULL);

    app_sv_throughput_test_data_xfer_lasting_timer_id = 
        osTimerCreate(osTimer(APP_SV_THROUGHPUT_TEST_DATA_XFER_LASTING_TIMER), 
        osTimerOnce, NULL);
}

#if 0
extern "C" void app_voicepath_ble_conn_parameter_updated(uint8_t conidx, uint16_t minConnIntervalInMs, uint16_t maxConnIntervalInMs)
{
    throughputTestConfig.minConnIntervalInMs = minConnIntervalInMs;
    throughputTestConfig.maxConnIntervalInMs = maxConnIntervalInMs;
}
#endif

uint32_t app_sv_throughput_test_transmission_handler(void *param1, uint32_t param2)
{
    if (UP_STREAM == throughputTestConfig.direction)
    {
        app_sv_send_command(OP_THROUGHPUT_TEST_DATA, 
            app_sv_throughput_datapattern, throughputTestConfig.dataPacketSize - 4);  
    }

    return 0;
}

static void app_sv_throughput_test_data_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    if ((WITH_RSP == throughputTestConfig.responseModel) && 
        (AI_TRANSPORT_SPP == ai_struct.transport_type))
    {
        app_sv_send_command(OP_THROUGHPUT_TEST_DATA_ACK, NULL, 0); 
    }
}

static void app_sv_throughput_test_data_ack_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    if (smartvoice_env.isThroughputTestOn &&
        (WITH_RSP == throughputTestConfig.responseModel))
    {
        app_sv_throughput_test_transmission_handler(NULL, 0);
    }
}

static void app_sv_throughput_test_done_signal_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    app_sv_stop_throughput_test();
}

void app_sv_stop_throughput_test(void)
{
    smartvoice_env.isThroughputTestOn = false;
    osTimerStop(app_sv_throughput_pre_config_pending_timer_id);
    osTimerStop(app_sv_throughput_test_data_xfer_lasting_timer_id);
}

static void app_sv_throughput_test_config_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    throughputTestConfig = *(THROUGHPUT_TEST_CONFIG_T *)ptrParam;

    TRACE("data patter is %d", throughputTestConfig.dataPattern);

    // generate the data pattern
    switch (throughputTestConfig.dataPattern)
    {
        case PATTERN_RANDOM:
        {
            for (uint32_t index = 0;index < MAXIMUM_DATA_PACKET_LEN;index++)
            {
                app_sv_throughput_datapattern[index] = (uint8_t)rand();                
            }
            break;
        }
        case PATTERN_11110000:
        {
            memset(app_sv_throughput_datapattern, 0xF0, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_10101010:
        {
            memset(app_sv_throughput_datapattern, 0xAA, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_ALL_1:
        {
            memset(app_sv_throughput_datapattern, 0xFF, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_ALL_0:
        {
            memset(app_sv_throughput_datapattern, 0, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_00001111:
        {
            memset(app_sv_throughput_datapattern, 0x0F, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_0101:
        {
            memset(app_sv_throughput_datapattern, 0x55, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        default:
            throughputTestConfig.dataPattern = 0;
            break;        
    }

    smartvoice_env.isThroughputTestOn = true;

    if (WITHOUT_RSP == throughputTestConfig.responseModel)
    {
        app_ai_register__spp_tx_done(app_sv_throughput_test_transmission_handler);
    }
    else
    {
        app_ai_register__spp_tx_done(NULL);
    }
    
    // check whether need to use the new conn parameter
    if (AI_TRANSPORT_BLE == ai_struct.transport_type
        && throughputTestConfig.isToUseSpecificConnParameter)
    {
        app_ai_update_conn_parameter(smartvoice_env.conidx, throughputTestConfig.minConnIntervalInMs,
            throughputTestConfig.maxConnIntervalInMs, 
            AI_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS,
            AI_BLE_CONNECTION_SLAVELATENCY);       
    
        osTimerStart(app_sv_throughput_pre_config_pending_timer_id, 
            APP_SV_THROUGHPUT_PRE_CONFIG_PENDING_TIME_IN_MS);
    }
    else
    {
        app_sv_send_command(OP_INFORM_THROUGHPUT_TEST_CONFIG, 
            (uint8_t *)&throughputTestConfig, sizeof(throughputTestConfig));

        if (UP_STREAM == throughputTestConfig.direction)
        {
            app_sv_throughput_test_transmission_handler(NULL, 0);
            osTimerStart(app_sv_throughput_test_data_xfer_lasting_timer_id, 
                throughputTestConfig.lastTimeInSecond*1000);
        }
    }
}

CUSTOM_COMMAND_TO_ADD(OP_INFORM_THROUGHPUT_TEST_CONFIG, app_sv_throughput_test_config_handler, FALSE,  
    0,    NULL );

CUSTOM_COMMAND_TO_ADD(OP_THROUGHPUT_TEST_DATA, app_sv_throughput_test_data_handler, FALSE,  
    0,    NULL );

CUSTOM_COMMAND_TO_ADD(OP_THROUGHPUT_TEST_DATA_ACK, app_sv_throughput_test_data_ack_handler, FALSE,  
    0,    NULL );

CUSTOM_COMMAND_TO_ADD(OP_THROUGHPUT_TEST_DONE, app_sv_throughput_test_done_signal_handler, FALSE,  
    0,    NULL );

/// TENCENT handlers function definition
const struct ai_func_handler smartvoice_handler_function_list[] =
{
    {API_HANDLE_INIT,         (ai_handler_func_t)app_sv_handle_init},
    {API_START_ADV,           (ai_handler_func_t)app_sv_start_advertising},
    {API_DATA_SEND,           (ai_handler_func_t)app_sv_send_voice_stream},
    {API_DATA_INIT,           (ai_handler_func_t)app_sv_stream_init},
    {API_DATA_DEINIT,         (ai_handler_func_t)app_sv_stream_deinit},
    {CALLBACK_CMD_RECEIVE,    (ai_handler_func_t)app_sv_rcv_cmd},
    {CALLBACK_DATA_RECEIVE,   (ai_handler_func_t)app_sv_rcv_data},
    {CALLBACK_DATA_PARSE,     (ai_handler_func_t)app_sv_parse_cmd},
    {CALLBACK_AI_CONNECT,     (ai_handler_func_t)app_sv_voice_connected},
    {CALLBACK_AI_DISCONNECT,  (ai_handler_func_t)app_sv_voice_disconnected},
    {CALLBACK_UPDATE_MTU,     (ai_handler_func_t)app_sv_config_mtu},
    {CALLBACK_WAKE_UP,        (ai_handler_func_t)app_sv_wake_up},
    {CALLBACK_AI_ENABLE,      (ai_handler_func_t)app_sv_enable_handler},
    {CALLBACK_START_SPEECH,   (ai_handler_func_t)NULL},
    {CALLBACK_ENDPOINT_SPEECH,(ai_handler_func_t)NULL},
    {CALLBACK_STOP_SPEECH,    (ai_handler_func_t)app_sv_stop_voice_stream},
    {CALLBACK_DATA_SEND_DONE, (ai_handler_func_t)app_sv_voice_send_done},
};

const struct ai_handler_func_table smartvoice_handler_function_table =
    {&smartvoice_handler_function_list[0], (sizeof(smartvoice_handler_function_list)/sizeof(struct ai_func_handler))};

AI_HANDLER_FUNCTION_TABLE_TO_ADD(AI_SPEC_BES, &smartvoice_handler_function_table)

