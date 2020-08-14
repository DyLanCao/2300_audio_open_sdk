#include <stdio.h>
#include <string.h>

#include "app_wl_smartvoice_cmd.h"
#include "app_wl_smartvoice_channel.h"
#include "app_wl_smartvoice.h"

static EVENT_CB event_cb = NULL;
static uint8_t g_seq = 0x00;

static bool app_wl_smartvoice_spp_data_check(uint8_t* data, uint16_t length)
{
    uint16_t checksum;
    uint16_t calc_checksum;
    if ( !(0xA4 ==data[0] && 0xB4 == data[1]) ) {
        return false;
    }

    calc_checksum = 0;
    for (int i=2; i<length-4; i++) {
        calc_checksum += data[i];
    }

    memcpy(&checksum, data+length-2, 2);

    return checksum == calc_checksum;
}

static uint16_t calc_checksum(uint8_t* data, uint16_t length)
{
    uint16_t checksum;

    checksum = 0;
    for (int i=0; i< length; i++) {
        checksum += data[i];
    }

    return checksum;
}

static void app_wl_smartvoice_spp_data_restruct(uint8_t* in, uint16_t in_len, uint8_t* out, uint16_t* p_out_len)
{
    uint16_t out_index;
    uint16_t in_index;

    out_index = 0;
    in_index = 2;
    
    //拷贝命令信息和长度信息
    out[out_index++] = in[in_index++];
    out[out_index++] = in[in_index++];

    in_index++; // 跳过seq字段
    memcpy(out+out_index, in+in_index, in_len-in_index-2);
    out_index += in_len-in_index-2;

    *p_out_len = out_index;
}

static void post_event(app_wl_sv_event_info_t* info)
{
    if (event_cb) {
        event_cb(info);
    }
}

static void serialize_control_packet(app_wl_sv_cmd_t* packet, uint8_t* out, uint16_t* len)
{
    uint8_t index;
    uint8_t channel;
    uint16_t checksum;

    index = 0;
    channel = app_wl_smartvoice_current_channel_get();

    if (channel == APP_WL_SMARTVOICE_CHANNEL_SPP) {
        out[index++] = 0xA4;
        out[index++] = 0xB4;
        out[index++] = packet->cmd;
        out[index++] = packet->len;
        out[index++] = g_seq;
        memcpy(out+index, packet->data, packet->len);
        index += packet->len;

        checksum = calc_checksum(out+2, index-2);

        // 使用小端字节序
        memcpy(out+index, &checksum, 2);
        index += 2;

        *len = index;
        DUMP8("%02x ", out, index);
    } else if (channel == APP_WL_SMARTVOICE_CHANNEL_BLE) {
        out[index++] = packet->cmd;
        out[index++] = packet->len;
        memcpy(out+index, packet->data, packet->len);
        index += packet->len;

        *len = index;
    }
}

static void build_key_code_status_report(app_wl_sv_cmd_t* packet, uint8_t key_code, uint8_t key_status)
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_KEY_CODE_STATUS;
    packet->len = 2;
    packet->data[0] = key_code;
    packet->data[1] = key_status;
}

static void build_voice_support_report(app_wl_sv_cmd_t* packet, uint8_t encode_type, uint8_t length_type, uint8_t frame_length)
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_VOICE_SUPPORT;
    packet->len = 3;
    packet->data[0] = encode_type;
    packet->data[1] = length_type;
    packet->data[2] = frame_length;
}

static void build_get_seqid_report(app_wl_sv_cmd_t* packet, uint8_t seqid[16])
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_GET_SEQID;
    packet->len = 16;
    memcpy(packet->data, seqid, 16);
}

static void build_set_seqid_report(app_wl_sv_cmd_t* packet, uint8_t errcode)
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_SET_SEQID;
    packet->len = 1;
    packet->data[0] = errcode;
}

static void build_get_address_report(app_wl_sv_cmd_t* packet, uint8_t addr[6])
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_GET_ADDRESS;
    packet->len = 6;
    memcpy(packet->data, addr, 6);
}

static void build_get_vendor_report(app_wl_sv_cmd_t* packet, uint8_t* vendor, uint16_t vendor_len)
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_GET_VENDOR;
    packet->len = vendor_len;
    memcpy(packet->data, vendor, vendor_len);
}

static void build_get_version_report(app_wl_sv_cmd_t* packet, uint8_t* version, uint16_t version_len)
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_GET_VERSION;
    packet->len = version_len;
    memcpy(packet->data, version, version_len);
}

static void build_spp_ready_version_report(app_wl_sv_cmd_t* packet, uint8_t* version, uint16_t version_len)
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_NOTIFY_SPP_READY;
    packet->len = version_len;
    memcpy(packet->data, version, version_len);
}

static void build_get_cur_session_name_request(app_wl_sv_cmd_t* packet)
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_GET_CUR_SESSION_NAME;
    packet->len = 0;
}

static void build_get_cur_speaker_name_request(app_wl_sv_cmd_t* packet)
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_GET_CUR_SPEAKER_NAME;
    packet->len = 0;
}

static void build_record_status_report(app_wl_sv_cmd_t* packet, uint8_t status)
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_RECORD_STATUS_IND;
    packet->len = 1;
    packet->data[0] = status;
}

static void build_start_record_report(app_wl_sv_cmd_t* packet, uint8_t errcode)
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_START_RECORD;
    packet->len = 1;
    packet->data[0] = errcode;
}

static void build_stop_recode_report(app_wl_sv_cmd_t* packet, uint8_t errcode)
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_STOP_RECORD;
    packet->len = 1;
    packet->data[0] = errcode;
}

static void build_start_play_ring_report(app_wl_sv_cmd_t* packet, uint8_t errcode)
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_START_PLAY_RING;
    packet->len = 1;
    packet->data[0] = errcode;
}   

POSSIBLY_UNUSED static void build_stop_play_ring_report(app_wl_sv_cmd_t* packet, uint8_t errcode)
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_STOP_PLAY_RING;
    packet->len = 1;
    packet->data[0] = errcode;
}   

POSSIBLY_UNUSED static void build_finish_play_ring_report(app_wl_sv_cmd_t* packet)
{
    packet->cmd = APP_WL_SV_COMMAND_TYPE_RING_FINISH_IND;
    packet->len = 0;
}

void app_wl_sv_build_voice_stream(uint8_t* in, uint16_t in_len, uint8_t* out, uint16_t* out_len)
{
    uint16_t index;

    index = 0;
    out[index++] = 0xAA;
    out[index++] = 0xBB;
    out[index++] = in_len;

    memcpy(out+index, in, in_len);
    index += in_len;

    *out_len = index;
}

void app_wl_sv_key_code_status_report(int8_t key_code, uint8_t key_status)
{
    uint8_t buff[40];
    uint16_t length;
    app_wl_sv_cmd_t _packet;
    app_wl_sv_cmd_t* packet = &_packet;

    build_key_code_status_report(packet, key_code, key_status);
    serialize_control_packet(packet, buff, &length);
    app_wl_smartvoice_send_key_status(buff, length);
}

void app_wl_sv_voice_support_report(uint8_t encode_type, uint8_t length_type, uint8_t frame_length)
{
    APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet);
    build_voice_support_report(packet, encode_type, length_type, frame_length);
    APP_WL_SV_CONTROL_CMD_SEND_END();
}

void app_wl_sv_seqid_report(uint8_t seqid[16])
{
    APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet);
    build_get_seqid_report(packet, seqid);
    APP_WL_SV_CONTROL_CMD_SEND_END();
}

void app_wl_sv_set_seqid_ack(uint8_t errcode)
{
    APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet);
    build_set_seqid_report(packet, errcode);
    APP_WL_SV_CONTROL_CMD_SEND_END();
}

void app_wl_sv_address_report(uint8_t addr[6])
{
    APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet);
    build_get_address_report(packet, addr);
    APP_WL_SV_CONTROL_CMD_SEND_END();
}

void app_wl_sv_vendor_report(uint8_t* vendor, uint16_t vendor_len)
{
    APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet);
    build_get_vendor_report(packet, vendor, vendor_len);
    APP_WL_SV_CONTROL_CMD_SEND_END();
}

void app_wl_sv_version_report(uint8_t* version, uint16_t version_len)
{
    APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet);
    build_get_version_report(packet, version, version_len);
    APP_WL_SV_CONTROL_CMD_SEND_END();
}

void app_wl_sv_spp_ready_version_report(uint8_t* version, uint16_t version_len)
{
    APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet);
    build_spp_ready_version_report(packet, version, version_len);
    APP_WL_SV_CONTROL_CMD_SEND_END();
}

void app_wl_sv_get_cur_session_name(void)
{
    APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet);
    build_get_cur_session_name_request(packet);
    APP_WL_SV_CONTROL_CMD_SEND_END();
}

void app_wl_sv_get_cur_speaker_name(void)
{
    APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet);
    build_get_cur_speaker_name_request(packet);
    APP_WL_SV_CONTROL_CMD_SEND_END();
}

void app_wl_sv_record_status_report(uint8_t status)
{
    APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet);
    build_record_status_report(packet, status);
    APP_WL_SV_CONTROL_CMD_SEND_END();
}

void app_wl_sv_start_record_ack(uint8_t errcode)
{
    APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet);
    build_start_record_report(packet, errcode);
    APP_WL_SV_CONTROL_CMD_SEND_END();
}

void app_wl_sv_stop_record_ack(uint8_t errocode)
{
    APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet);
    build_stop_recode_report(packet, errocode);
    APP_WL_SV_CONTROL_CMD_SEND_END();
}

void app_wl_sv_start_play_ring_ack(uint8_t errcode)
{
    APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet);
    build_start_play_ring_report(packet, errcode);
    APP_WL_SV_CONTROL_CMD_SEND_END();
}

void app_wl_sv_stop_play_ring_ack(uint8_t errcode)
{
    APP_WL_SV_CONTROL_CMD_SEND_BEGIN(packet);
    build_stop_recode_report(packet, errcode);
    APP_WL_SV_CONTROL_CMD_SEND_END();
}

void app_wl_sv_command_received(uint8_t* data, uint16_t len, uint8_t channel)
{
    uint8_t cmd;
    static uint8_t buff[100];
    uint16_t length;
    app_wl_sv_event_info_t info;

    WL_SV_FUNC_ENTER();

    if (APP_WL_SMARTVOICE_CHANNEL_SPP == channel) {
        if (!app_wl_smartvoice_spp_data_check(data, len)) {
            WL_SV_DEBUG("Invalid Spp Data");
            return;
        }

        app_wl_smartvoice_spp_data_restruct(data, len, buff, &length);
        data = buff;
        len = length;
    }
    
    cmd = data[0];

    WL_SV_DEBUG("*** command: %d ***", cmd);
    switch (cmd)
    {
    case APP_WL_SV_COMMAND_TYPE_NOTIFY_SPP_READY:
        info.event_id = APP_WL_SV_EVENT_TYPE_NOTIFY_SPP_READY;
        post_event(&info);
        break;
    case APP_WL_SV_COMMAND_TYPE_VOICE_SUPPORT:
        info.event_id = APP_WL_SV_EVENT_TYPE_QUERY_VOICE_SUPPORT_IND;
        post_event(&info);
        break;

    case APP_WL_SV_COMMAND_TYPE_GET_SEQID:
        info.event_id = APP_WL_SV_EVENT_TYPE_GET_SEQID_IND;
        post_event(&info);

        break;

    case APP_WL_SV_COMMAND_TYPE_SET_SEQID:
    {
        uint8_t* p_seqid;

        p_seqid = &data[2];
        info.event_id = APP_WL_SV_EVENT_TYPE_SET_SEQID_IND;
        memcpy(info.seqid, p_seqid, 16);
        post_event(&info);
    }
        break;

    case APP_WL_SV_COMMAND_TYPE_GET_ADDRESS:
        info.event_id = APP_WL_SV_EVENT_TYPE_GET_BT_ADDR_IND;
        post_event(&info);
        break;

    case APP_WL_SV_COMMAND_TYPE_GET_VENDOR:
        info.event_id = APP_WL_SV_EVENT_TYPE_GET_VENDOR_IND;
        post_event(&info);
        break;

    case APP_WL_SV_COMMAND_TYPE_GET_VERSION:
        info.event_id = APP_WL_SV_EVENT_TYPE_GET_VERSION_IND;
        post_event(&info);
        break;

    // 这是一个response
    case APP_WL_SV_COMMAND_TYPE_GET_CUR_SESSION_NAME:
    {
        uint8_t* session_name;
        uint16_t session_name_len;

        session_name_len = data[1];
        session_name = &data[2];

        info.event_id = APP_WL_SV_EVENT_TYPE_CUR_SESSION_NAME_IND;
        memcpy(info.session_name, session_name, session_name_len);
        info.session_name_len = session_name_len;
        post_event(&info);
    }
        break;

    // 这是一个response
    case APP_WL_SV_COMMAND_TYPE_GET_CUR_SPEAKER_NAME:
    {
        uint8_t* speaker_name;
        uint16_t speaker_name_len;

        speaker_name_len = data[1];
        speaker_name = &data[2];

        info.event_id = APP_WL_SV_EVENT_TYPE_CUR_SPEAKER_NAME_IND;
        memcpy(info.speaker_name, speaker_name, speaker_name_len);
        info.session_name_len = speaker_name_len;
        post_event(&info);
    }
        break;

    case APP_WL_SV_COMMAND_TYPE_NET_STATUS_IND:
    {
        uint8_t net_status;
        net_status = data[2];

        info.event_id = APP_WL_SV_EVENT_TYPE_NET_STATUS_IND;
        info.net_status = net_status;
        post_event(&info);
    }
        break;

    case APP_WL_SV_COMMAND_TYPE_PPT_SESSION_STATUS_IND:
    {
        uint8_t ppt_session_status;
        ppt_session_status = data[2];

        info.event_id = APP_WL_SV_EVENT_TYPE_PPT_SESSION_STATUS_IND;
        info.ppt_session_status = ppt_session_status;
        post_event(&info);
    }
        break;

    // 开始录音
    case APP_WL_SV_COMMAND_TYPE_START_RECORD:
        info.event_id = APP_WL_SV_EVENT_TYPE_START_RECORD_IND;
        post_event(&info);
        break;

    // 停止录音
    case APP_WL_SV_COMMAND_TYPE_STOP_RECORD:
        info.event_id = APP_WL_SV_EVENT_TYPE_STOP_RECORD_IND;
        post_event(&info);
        break;

    case APP_WL_SV_COMMAND_TYPE_START_PLAY_RING:
    {
        uint8_t play_ring_id;

        play_ring_id = data[2];
        info.event_id = APP_WL_SV_EVENT_TYPE_START_PLAY_RING_IND;
        info.play_ring_id = play_ring_id;
        post_event(&info);
    }
        break;

    case APP_WL_SV_COMMAND_TYPE_STOP_PLAY_RING:
        info.event_id = APP_WL_SV_EVENT_TYPE_STOP_PLAY_RING_IND;
        post_event(&info);
        break;
    
    default:
        break;
    }

    WL_SV_FUNC_EXIT();
}

void app_wl_smartvoice_event_subscribe(EVENT_CB cb)
{
    event_cb = cb;
}

void app_wl_smartvoice_cmd_init(void)
{
    app_wl_smartvoice_channel_data_subscribe(app_wl_sv_command_received);
}