#ifndef __APP_WL_SMARTVOICE_CMD_H
#define __APP_WL_SMARTVOICE_CMD_H

#include "hal_trace.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_WL_SV_COMMAND_TYPE_KEY_CODE_STATUS                  (0xFF)
#define APP_WL_SV_COMMAND_TYPE_VOICE_SUPPORT                    (0x1F)
#define APP_WL_SV_COMMAND_TYPE_GET_SEQID                        (0x21)
#define APP_WL_SV_COMMAND_TYPE_SET_SEQID                        (0x31)
#define APP_WL_SV_COMMAND_TYPE_GET_ADDRESS                      (0x2A)
#define APP_WL_SV_COMMAND_TYPE_GET_VENDOR                       (0x22)
#define APP_WL_SV_COMMAND_TYPE_GET_VERSION                      (0x23)
#define APP_WL_SV_COMMAND_TYPE_GET_CUR_SESSION_NAME             (0x28)
#define APP_WL_SV_COMMAND_TYPE_GET_CUR_SPEAKER_NAME             (0x27)
#define APP_WL_SV_COMMAND_TYPE_RECORD_STATUS_IND                (0x2B)
#define APP_WL_SV_COMMAND_TYPE_NET_STATUS_IND                   (0x25)
#define APP_WL_SV_COMMAND_TYPE_PPT_SESSION_STATUS_IND           (0x24)
#define APP_WL_SV_COMMAND_TYPE_START_RECORD                     (0x34)
#define APP_WL_SV_COMMAND_TYPE_STOP_RECORD                      (0x35)
#define APP_WL_SV_COMMAND_TYPE_START_PLAY_RING                  (0x37)
#define APP_WL_SV_COMMAND_TYPE_STOP_PLAY_RING                   (0x38)
#define APP_WL_SV_COMMAND_TYPE_RING_FINISH_IND                  (0x26)

#define APP_WL_SV_COMMAND_TYPE_NOTIFY_SPP_READY                 (0x10)

enum {
    APP_WL_SV_KEY_CODE_PPT = 0x40,
    APP_WL_SV_KEY_CODE_PREV_GROUP,
    APP_WL_SV_KEY_CODE_NEXT_GROUP,
    APP_WL_SV_KEY_CODE_VOL_UP,
    APP_WL_SV_KEY_CODE_VOL_DOWN,
    APP_WL_SV_KEY_CODE_REWIND,
    APP_WL_SV_KEY_CODE_AI_VOICE,
    APP_WL_SV_KEY_CODE_SEND_CURRENT_POS,
    APP_WL_SV_KEY_CODE_MUTE_ALL,
    APP_WL_SV_KEY_CODE_SKIP_CURRENT,
    APP_WL_SV_KEY_CODE_BROADCAST,
    APP_WL_SV_KEY_CODE_GROUP1,
    APP_WL_SV_KEY_CODE_GROUP2,
    APP_WL_SV_KEY_CODE_GROUP3,
    APP_WL_SV_KEY_CODE_JUMP_TO_CURRENT,
};

enum {
    APP_WL_SV_KEY_STATUS_DOWN = 0x01,
    APP_WL_SV_KEY_STATUS_UP = 0x02,
    APP_WL_SV_KEY_STATUS_PRESS = 0x11,
    APP_WL_SV_KEY_STATUS_LONG = 0x12,
};

enum {
    APP_WL_SV_ERROR_CODE_SET_SEQID_OK = 0,
};

enum {
    APP_WL_SV_ERROR_CODE_OK = 1,
};

enum {
    APP_WL_SV_ENCODE_TYPE_SBC = 2,
    APP_WL_SV_ENCODE_TYPE_OPUS = 3,
};

enum {
    APP_WL_SV_STREAM_LENGTH_TYPE_FIXED = 0,
};

enum {
    APP_WL_SV_RECORD_STATUS_START = 0x01,
    APP_WL_SV_RECORD_STATUS_STOP,
};

enum {
    APP_WL_SV_NET_STATUS_CONNECTED = 1,
    APP_WL_SV_NET_STATUS_DISCONNECTED,
};

enum {
    APP_WL_SV_PPT_SESSION_STATUS_1 = 1,
    APP_WL_SV_PPT_SESSION_STATUS_2,
    APP_WL_SV_PPT_SESSION_STATUS_3,
    APP_WL_SV_PPT_SESSION_STATUS_4,
};

enum {
    APP_WL_SV_EVENT_TYPE_QUERY_VOICE_SUPPORT_IND,
    APP_WL_SV_EVENT_TYPE_GET_SEQID_IND,
    APP_WL_SV_EVENT_TYPE_SET_SEQID_IND,
    APP_WL_SV_EVENT_TYPE_GET_BT_ADDR_IND,
    APP_WL_SV_EVENT_TYPE_GET_VENDOR_IND,
    APP_WL_SV_EVENT_TYPE_GET_VERSION_IND,
    APP_WL_SV_EVENT_TYPE_CUR_SESSION_NAME_IND,
    APP_WL_SV_EVENT_TYPE_CUR_SPEAKER_NAME_IND,
    APP_WL_SV_EVENT_TYPE_NET_STATUS_IND,
    APP_WL_SV_EVENT_TYPE_PPT_SESSION_STATUS_IND,
    APP_WL_SV_EVENT_TYPE_START_RECORD_IND,
    APP_WL_SV_EVENT_TYPE_STOP_RECORD_IND,
    APP_WL_SV_EVENT_TYPE_START_PLAY_RING_IND,
    APP_WL_SV_EVENT_TYPE_STOP_PLAY_RING_IND,
    APP_WL_SV_EVENT_TYPE_NOTIFY_SPP_READY,
};

typedef struct {
    uint8_t event_id;
    uint8_t seqid[16];
    uint8_t session_name[16];
    uint16_t session_name_len;
    uint8_t speaker_name[16];
    uint16_t speaker_name_len;

    uint8_t net_status;
    uint8_t ppt_session_status;

    uint8_t play_ring_id;
} app_wl_sv_event_info_t;

typedef void (*EVENT_CB)(app_wl_sv_event_info_t*);

typedef struct {
    uint8_t cmd;
    uint8_t len;
    uint8_t data[100];
} app_wl_sv_cmd_t;


#define APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet) \
uint8_t buff[40]; \
uint16_t length; \
app_wl_sv_cmd_t _packet; \
app_wl_sv_cmd_t* packet = &_packet;


#define APP_WL_SV_CONTROL_CMD_SEND_END() \
serialize_control_packet(packet, buff, &length); \
app_wl_smartvoice_send_control_data(buff, length);

void app_wl_smartvoice_cmd_init(void);

void app_wl_sv_voice_support_report(uint8_t encode_type, uint8_t length_type, uint8_t frame_length);
void app_wl_sv_seqid_report(uint8_t seqid[16]);
void app_wl_sv_set_seqid_ack(uint8_t errcode);
void app_wl_sv_address_report(uint8_t addr[6]);
void app_wl_sv_vendor_report(uint8_t* vendor, uint16_t vendor_len);
void app_wl_sv_version_report(uint8_t* version, uint16_t version_len);
void app_wl_sv_spp_ready_version_report(uint8_t* version, uint16_t version_len);

/*** DEVICE->APP REQUEST ***/
void app_wl_sv_key_code_status_report(int8_t key_code, uint8_t key_status);
void app_wl_sv_get_cur_session_name(void);
void app_wl_sv_get_cur_speaker_name(void);
void app_wl_sv_record_status_report(uint8_t status);
/**************************/
void app_wl_sv_start_record_ack(uint8_t errcode);
void app_wl_sv_stop_record_ack(uint8_t errocode);
void app_wl_sv_start_play_ring_ack(uint8_t errcode);
void app_wl_sv_stop_play_ring_ack(uint8_t errcode);

void app_wl_smartvoice_event_subscribe(EVENT_CB cb);

void app_wl_sv_build_voice_stream(uint8_t* in, uint16_t in_len, uint8_t* out, uint16_t* out_len);

#ifdef __cplusplus
}
#endif

#endif