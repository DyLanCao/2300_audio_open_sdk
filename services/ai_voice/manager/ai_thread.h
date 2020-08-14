#ifndef __AI_THREAD_H__
#define __AI_THREAD_H__

#ifdef RTOS
#include "cmsis_os.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t __ai_handler_function_table_start[];
extern uint32_t __ai_handler_function_table_end[];

#define INVALID_AI_HANDLER_FUNCTION_ENTRY_INDEX 0xFFFF
#define APP_AI_VOICE_MIC_TIMEOUT_INTERVEL           (10000)
#define AI_MAILBOX_MAX (40)
#define MAX_TX_CREDIT 3

typedef enum  {
	SIGN_AI_NONE,
    SIGN_AI_CONNECT,
    SIGN_AI_WAKEUP,
    SIGN_AI_CMD_COME,
    SIGN_AI_TRANSPORT,
} ai_signal_t;

typedef struct {
	ai_signal_t signal;
    uint32_t    len;
} AI_MESSAGE_BLOCK;

//the ai transport type
#define AI_TRANSPORT_IDLE   0
#define AI_TRANSPORT_BLE    1
#define AI_TRANSPORT_SPP    2

//the ai wake up type
#define TYPE__NONE              0
#define TYPE__PRESS_AND_HOLD    1
#define TYPE__TAP               2
#define TYPE__KEYWORD_WAKEUP    3

//the ai speech type
#define TYPE__NORMAL_SPEECH     0
#define TYPE__PROVIDE_SPEECH    1

typedef struct
{
    uint32_t    sentDataSizeWithInFrame;
    uint8_t     seqNumWithInFrame;
    uint8_t*    tmpBufForXfer;
    volatile uint8_t 	ai_stream_opened    :1;
	volatile uint8_t    ai_stream_running	:1;
	volatile uint8_t    audio_algorithm_engine_reseted   :1;
	volatile uint8_t	ai_setup_complete   :1;
	volatile uint8_t	isEnableBleAdvUUID	:1;
	volatile uint8_t	ai_speech_type	    :1;
    volatile uint8_t    reserve             :2;

} APP_AI_STREAM_ENV_T;  

typedef struct
{
    uint32_t                currentAiSpec;  //ai_type
    uint32_t                trans_size;     //transport size one time
    uint32_t                trans_time;     //transport times
    uint8_t                 transport_type;
	uint8_t    				tx_credit;
    uint8_t                 encode_type;
    uint8_t                 wake_up_type;
    APP_AI_STREAM_ENV_T     ai_stream;
    unsigned int            txbuff_send_max_size;
    bool                    transfer_voice_start;
    
} AI_struct ;

/**
 * @brief AI voice ble_table_handler definition data structure
 *
 */
typedef struct
{
	uint32_t				            ai_code;
    const struct ai_handler_func_table* ai_handler_function_table;  	/**< ai handler function table */
} AI_HANDLER_FUNCTION_TABLE_INSTANCE_T;


#define AI_HANDLER_FUNCTION_TABLE_TO_ADD(ai_code, ai_handler_function_table)	\
	static AI_HANDLER_FUNCTION_TABLE_INSTANCE_T ai_code##_handler_function_table __attribute__((used, section(".ai_handler_function_table"))) = 	\
		{(ai_code), (ai_handler_function_table)};

#define AI_HANDLER_FUNCTION_TABLE_PTR_FROM_ENTRY_INDEX(index)	\
	((AI_HANDLER_FUNCTION_TABLE_INSTANCE_T *)((uint32_t)__ai_handler_function_table_start + (index)*sizeof(AI_HANDLER_FUNCTION_TABLE_INSTANCE_T)))

extern AI_struct ai_struct;

extern osTimerId app_ai_voice_timeout_timer_id;
bool app_ai_is_to_mute_a2dp_during_ai_starting_speech(void);
void app_ai_control_mute_a2dp_state(bool isEnabled);
void app_ai_set_ble_adv_uuid(bool isEnabled);
bool app_ai_is_ble_adv_uuid_enabled(void);
bool app_is_ai_voice_connected(void);

void ai_set_power_key_start(void);
void ai_set_power_key_stop(void);
int ai_get_power_key(void);
void ai_parse_data_init(void (*cb)(void));
int ai_mailbox_put(ai_signal_t signal, uint32_t len);
uint32_t ai_open(uint32_t AiSpec);

#ifdef __cplusplus
}
#endif

#endif

