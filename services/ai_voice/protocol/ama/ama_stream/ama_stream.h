
#ifndef _AMA_STREAM_H_
#define _AMA_STREAM_H_

#ifdef __cplusplus
extern "C" {
#endif


#define AMA_CONTROL_STREAM_ID	0
#define AMA_VOICE_STREAM_ID		1

#define AMA_VER 1
#define AMA_FLAG 0  //reserved -- set to 0

#define AMA_SERIAL_NUM	"20180527000001"

#define AMA_DEVICE_NAME "ama_accessory"

#define AMA_DEVICE_TYPE "A2Y04QPFCANLPQ"


#define AMA_CTRL_STREAM_HEADER__INIT           \
    {                                          \
        AMA_VER, AMA_CONTROL_STREAM_ID, 0, 0, 0 \
    }

#define AMA_AUDIO_STREAM_HEADER__INIT           \
    {                                          \
        AMA_VER, AMA_VOICE_STREAM_ID, 0, 0, 0 \
    }

typedef struct
{
    uint16_t version 	: 	4;
    uint16_t streamId 	: 	5;
    uint16_t flags 		: 	6;
	uint16_t isUse16BitLength : 1;
    uint16_t length;
} ama_stream_header_t ;

typedef enum  {
	USER_CANCEL,
    TIME_OUT, 
    UNKNOWN,
} ama_stop_reason;


void ama_control_stream_handler(uint8_t* ptr, uint32_t len);

bool ama_stream_header_parse(uint8_t *buffer, ama_stream_header_t *header);

void ama_reset_connection (void);
void ama_start_speech_request(void);
void ama_stop_speech_request (ama_stop_reason stop_reason);
void ama_endpoint_speech_request(uint32_t dialog_id);


bool ama_audio_stream_send(uint8_t *in_buf,uint32_t len);

void ama_send_event_message_callback_init(int (*cb)(bool));
void ama_notify_device_configuration (uint8_t is_assistant_override);

#ifdef __cplusplus
}
#endif
#endif
