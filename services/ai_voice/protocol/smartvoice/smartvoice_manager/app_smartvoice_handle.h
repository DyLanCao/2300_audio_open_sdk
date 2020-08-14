#ifndef __APP_SMARTVOICE_HANDLE_H
#define __APP_SMARTVOICE_HANDLE_H

#undef  UUID_128_ENABLE
#define IS_ENABLE_START_DATA_XFER_STEP			0

#define IS_USE_ENCODED_DATA_SEQN_PROTECTION    	1
#define IS_SMARTVOICE_DEBUG_ON					1
#define IS_FORCE_STOP_VOICE_STREAM				0

#define APP_SMARTVOICE_PATH_TYPE_SPP	(1 << 0)
#define APP_SMARTVOICE_PATH_TYPE_BLE	(1 << 1)
#define APP_SMARTVOICE_PATH_TYPE_HFP	(1 << 2)

#if IS_USE_ENCODED_DATA_SEQN_PROTECTION
#define SMARTVOICE_ENCODED_DATA_SEQN_SIZE	    sizeof(uint8_t)
#else
#define SMARTVOICE_ENCODED_DATA_SEQN_SIZE	    0
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum {
    APP_SMARTVOICE_STATE_IDLE,
    APP_SMARTVOICE_STATE_MIC_STARTING,
    APP_SMARTVOICE_STATE_MIC_STARTED,
    APP_SMARTVOICE_STATE_SPEAKER_STARTING,
    APP_SMARTVOICE_STATE_SPEAKER_STARTED,
    APP_SMARTVOICE_STATE_MAX
};

typedef enum
{
	CLOUD_MUSIC_CONTROL_NEXT = 0,
	CLOUD_MUSIC_CONTROL_PRE,
	CLOUD_MUSIC_CONTROL_PAUSE,
	CLOUD_MUSIC_CONTROL_RESUME
} CLOUD_MUSIC_CONTROL_REQ_E;

extern const struct ai_handler_func_table smartvoice_handler_function_table;

void app_sv_start_voice_stream_via_hfp(void);
uint32_t app_sv_stop_voice_stream(void *param1, uint32_t param2);
void app_sv_start_voice_stream_via_ble(void);
void app_sv_control_cloud_music_playing(CLOUD_MUSIC_CONTROL_REQ_E req);

void app_smartvoice_switch_hfp_stream_status(void);
void app_smartvoice_switch_ble_stream_status(void);
void app_smartvoice_switch_spp_stream_status(void);

void app_sv_switch_cloud_platform(void);

void app_sv_throughput_test_init(void);
void app_sv_stop_throughput_test(void);
uint32_t app_sv_throughput_test_transmission_handler(void *param1, uint32_t param2);
#ifdef __cplusplus
}
#endif

#endif

