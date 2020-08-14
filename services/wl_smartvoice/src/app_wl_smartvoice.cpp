#include <stdio.h>
#include <string.h>
#include "app_wl_smartvoice.h"
#include "app_wl_smartvoice_cmd.h"
#include "app_wl_smartvoice_ble.h"
#include "app_wl_smartvoice_channel.h"
#include "audioflinger.h"
#include "app_audio.h"
#include "app_wl_smartvoice_stream.h"
#include "factory_section.h"
#include "app_spp.h"
#include "besbt.h"

static uint8_t app_wl_sv_seqid[] = {
    0x4d, 0x98, 0xa8, 0xd0, 0x3a, 0xf6,  // 微喇批次序号
    0x00, 0x00, 0x00, 0x00,              // 中间补4字节0
    0xbb, 0xaa, 0x33, 0x22, 0x11, 0xcc,  // 设备蓝牙MAC地址
};
static app_wl_sv_ctx_t wl_sv_ctx;

static app_wl_sv_ctx_t* get_instance(void)
{
    return &wl_sv_ctx;
}

static void app_wl_sv_set_local_session_name(app_wl_sv_ctx_t* ctx, uint8_t* session_name, uint16_t session_name_len)
{
    memcpy(ctx->session_name, session_name, session_name_len);
    ctx->session_name_len = session_name_len;
}

static void app_wl_sv_set_local_speaker_name(app_wl_sv_ctx_t* ctx, uint8_t* speaker_name, uint16_t speaker_name_len)
{
    memcpy(ctx->session_name, speaker_name, speaker_name_len);
    ctx->session_name_len = speaker_name_len;
}

static void app_wl_sv_set_net_status(app_wl_sv_ctx_t* ctx, uint8_t net_status)
{
    ctx->net_status = net_status;
}

static void app_wl_sv_set_ppt_session_status(app_wl_sv_ctx_t* ctx, uint8_t ppt_session_status)
{
    ctx->ppt_session_status = ppt_session_status;
}

POSSIBLY_UNUSED static bool app_wl_sv_get_voice_support(app_wl_sv_ctx_t* ctx)
{
    return ctx->b_suport_voice;
}

static uint8_t* app_wl_sv_get_seqid(app_wl_sv_ctx_t* ctx)
{
    return ctx->seqid;
}

static void app_wl_sv_set_seqid(app_wl_sv_ctx_t* ctx, uint8_t seqid[16])
{
    memcpy(ctx->seqid, seqid, 16);
}

static uint8_t* app_wl_sv_get_bt_addr(app_wl_sv_ctx_t* ctx)
{
    return ctx->bt_addr;
}

static void app_wl_sv_get_vendor(app_wl_sv_ctx_t* ctx, uint8_t** vendor, uint16_t* vendor_len)
{
    *vendor = ctx->vendor;
    *vendor_len = ctx->vendor_len;
}

static void app_wl_sv_get_version(app_wl_sv_ctx_t* ctx, uint8_t** version, uint16_t* version_len)
{
    *version = ctx->version;
    *version_len = ctx->version_len;
}

static void app_wl_sv_channel_event_handler(int event)
{
    switch (event)
    {
    case APP_WL_SMARTVOICE_CHANNEL_EVENT_CONNECTED:
        break;

    case APP_WL_SMARTVOICE_CHANNEL_EVENT_DISCONNECTED:
        app_wl_smartvoice_stop_stream();
        break;
    
    default:
        break;
    }
}

static void app_wl_sv_event_handler(app_wl_sv_event_info_t* info)
{
    app_wl_sv_ctx_t* ctx;

    WL_SV_FUNC_ENTER();

    WL_SV_DEBUG("event id: %d", info->event_id);

    ctx = get_instance();
    switch (info->event_id)
    {
    case APP_WL_SV_EVENT_TYPE_NOTIFY_SPP_READY:
    {
        uint8_t* version;
        uint16_t version_len;

        app_wl_sv_get_version(ctx, &version, &version_len);
        app_wl_sv_spp_ready_version_report(version, version_len);
    }
        break;
    case APP_WL_SV_EVENT_TYPE_QUERY_VOICE_SUPPORT_IND:
        // app_wl_sv_voice_support_report(app_wl_sv_get_voice_support(ctx));
        app_wl_sv_voice_support_report(APP_WL_SV_ENCODE_TYPE_OPUS, APP_WL_SV_STREAM_LENGTH_TYPE_FIXED, APP_WL_SMARTVOICE_OPUS_FRAME_ENCODE_SIZE);
        break;

    case APP_WL_SV_EVENT_TYPE_GET_SEQID_IND:
        app_wl_sv_seqid_report(app_wl_sv_get_seqid(ctx));
        break;

    case APP_WL_SV_EVENT_TYPE_SET_SEQID_IND:
        app_wl_sv_set_seqid(ctx, info->seqid);
        app_wl_sv_set_seqid_ack(APP_WL_SV_ERROR_CODE_SET_SEQID_OK);
        break;

    case APP_WL_SV_EVENT_TYPE_GET_BT_ADDR_IND:
        app_wl_sv_address_report(app_wl_sv_get_bt_addr(ctx));
        break;

    case APP_WL_SV_EVENT_TYPE_GET_VENDOR_IND:
    {
        uint8_t* vendor;
        uint16_t vendor_len;

        app_wl_sv_get_vendor(ctx, &vendor, &vendor_len);
        app_wl_sv_vendor_report(vendor, vendor_len);
    }
        break;

    case APP_WL_SV_EVENT_TYPE_GET_VERSION_IND:
    {
        uint8_t* version;
        uint16_t version_len;

        app_wl_sv_get_version(ctx, &version, &version_len);
        app_wl_sv_version_report(version, version_len);
    }
        break;

    case APP_WL_SV_EVENT_TYPE_CUR_SESSION_NAME_IND:
        app_wl_sv_set_local_session_name(ctx, info->session_name, info->session_name_len);
        break;

    case APP_WL_SV_EVENT_TYPE_CUR_SPEAKER_NAME_IND:
        app_wl_sv_set_local_speaker_name(ctx, info->speaker_name, info->speaker_name_len);
        break;

    case APP_WL_SV_EVENT_TYPE_NET_STATUS_IND:
        app_wl_sv_set_net_status(ctx, info->net_status);
        break;

    case APP_WL_SV_EVENT_TYPE_PPT_SESSION_STATUS_IND:
        app_wl_sv_set_ppt_session_status(ctx, info->ppt_session_status);
        break;

    case APP_WL_SV_EVENT_TYPE_START_RECORD_IND:
        // 开始录音
        // TODO: 打开capture的语音流

        app_wl_sv_start_record_ack(APP_WL_SV_ERROR_CODE_OK);
        app_wl_smartvoice_start_stream();
        break;

    case APP_WL_SV_EVENT_TYPE_STOP_RECORD_IND:
        // 停止录音
        // TODO: 关闭capture的语音流

        app_wl_sv_stop_record_ack(APP_WL_SV_ERROR_CODE_OK);
        app_wl_smartvoice_stop_stream();
        break;

    case APP_WL_SV_EVENT_TYPE_START_PLAY_RING_IND:
        // 开始播放提示音
        // TODO: 播放指定提示音
        WL_SV_DEBUG("ring id: %d", info->play_ring_id);
        app_wl_sv_start_play_ring_ack(APP_WL_SV_ERROR_CODE_OK);
        break;

    case APP_WL_SV_EVENT_TYPE_STOP_PLAY_RING_IND:
        // 停止播放提示音
        // TODO: 停止播放相关提示音

        app_wl_sv_stop_play_ring_ack(APP_WL_SV_ERROR_CODE_OK);
        break;
    
    default:
        break;
    }

    WL_SV_FUNC_EXIT();
}

void app_wl_sv_key_pressed(void)
{
    WL_SV_FUNC_ENTER();

    if (app_wl_smartvoice_is_gonging_on()) {
        app_wl_sv_key_code_status_report(APP_WL_SV_KEY_CODE_PPT, APP_WL_SV_KEY_STATUS_UP);
    } else {
        app_wl_sv_key_code_status_report(APP_WL_SV_KEY_CODE_PPT, APP_WL_SV_KEY_STATUS_DOWN);
    }
    WL_SV_FUNC_EXIT();
}

void app_wl_sv_switch_next_group(void)
{
    app_wl_sv_key_code_status_report(APP_WL_SV_KEY_CODE_NEXT_GROUP, APP_WL_SV_KEY_STATUS_DOWN);
}

void app_wl_sv_switch_pre_group(void)
{
    app_wl_sv_key_code_status_report(APP_WL_SV_KEY_CODE_PREV_GROUP, APP_WL_SV_KEY_STATUS_DOWN);
}


static void app_wl_sv_ctx_init(app_wl_sv_ctx_t* ctx)
{
    memset(ctx, 0x00, sizeof(app_wl_sv_ctx_t));

    ctx->b_suport_voice = true;
    memcpy(&app_wl_sv_seqid[10], bt_get_local_address(), 6);
    memcpy(ctx->seqid, app_wl_sv_seqid, 16);

    ctx->vendor = (uint8_t*)WL_SMARTVOICE_VENDOR_NAME;
    ctx->vendor_len = strlen(WL_SMARTVOICE_VENDOR_NAME);

    ctx->version = (uint8_t*)WL_SMARTVOICE_VERSION;
    ctx->version_len = strlen(WL_SMARTVOICE_VERSION);

    memcpy(ctx->bt_addr, factory_section_get_bt_address(), 6);
    memcpy(ctx->session_name, WL_SMARTVOICE_SESSION_NAME, strlen(WL_SMARTVOICE_SESSION_NAME));
    ctx->session_name_len = strlen(WL_SMARTVOICE_SESSION_NAME);

    memcpy(ctx->speaker_name, WL_SMARTVOICE_SPEAKER_NAME, strlen(WL_SMARTVOICE_SPEAKER_NAME));
    ctx->speaker_name_len = strlen(WL_SMARTVOICE_SPEAKER_NAME);

    ctx->net_status = APP_WL_SV_NET_STATUS_DISCONNECTED;
    ctx->ppt_session_status = APP_WL_SV_PPT_SESSION_STATUS_1;
}


void app_wl_sv_init(void)
{
    WL_SV_FUNC_ENTER();

    app_wl_sv_ctx_init(get_instance());

    app_wl_smartvoice_ble_init();

    app_wl_smartvoice_channle_init();

    app_wl_smartvoice_cmd_init();

    app_wl_smartvoice_stream_init();

    app_wl_smartvoice_event_subscribe(app_wl_sv_event_handler);
    app_wl_smartvoice_channel_event_subscribe(app_wl_sv_channel_event_handler);

    app_spp_init();

    WL_SV_FUNC_EXIT();
}