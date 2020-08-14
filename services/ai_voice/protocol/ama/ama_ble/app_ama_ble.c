/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#ifdef __AMA_VOICE__

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app_task.h"                // application task definitions         
#include "co_bt.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include "string.h"

#include "ai_manager.h"
#include "ai_thread.h"
#include "ai_control.h"
#include "ai_gatt_server.h"
#include "app_ai_ble.h"
#include "app_ama_ble.h"                  // ama Voice Application Definitions
#include "ama_gatt_server.h"
#include "ai_manager.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */


void app_ama_send_via_notification(uint8_t* ptrData, uint32_t length)
{
    //TRACE("%s len= %d", __func__, length);
	struct ble_ai_send_data_req_t * req = KE_MSG_ALLOC_DYN(AMA_SEND_VIA_NOTIFICATION,
                                                prf_get_task_from_id(TASK_ID_AI),
                                                TASK_APP,
                                                ble_ai_send_data_req_t,
                                                length);
	req->connecionIndex = app_ai_env.connectionIndex;
	req->length = length;
	memcpy(req->value, ptrData, length);

	ke_msg_send(req);
}

void app_ama_send_via_indication(uint8_t* ptrData, uint32_t length)
{
    TRACE("%s len= %d", __func__, length);
	struct ble_ai_send_data_req_t * req = KE_MSG_ALLOC_DYN(AMA_SEND_VIA_INDICATION,
                                                prf_get_task_from_id(TASK_ID_AI),
                                                TASK_APP,
                                                ble_ai_send_data_req_t,
                                                length);
	req->connecionIndex = app_ai_env.connectionIndex;
	req->length = length;
	memcpy(req->value, ptrData, length);

	ke_msg_send(req);
}

static int app_ama_msg_handler(ke_msg_id_t const msgid,
                              void const *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    TRACE("%s", __func__);
    // Do nothing
    return (KE_MSG_CONSUMED);
}

static int app_ama_cmd_ccc_changed_handler(ke_msg_id_t const msgid,
                              struct ble_ai_tx_notif_config_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
	TRACE("%s %d", __func__, param->isNotificationEnabled);
    
    app_ai_env.isCmdNotificationEnabled = param->isNotificationEnabled;
	if (app_ai_env.isCmdNotificationEnabled)
	{
		if (BLE_INVALID_CONNECTION_INDEX == app_ai_env.connectionIndex)
		{
			uint8_t conidx = KE_IDX_GET(src_id);
			app_ai_connected_evt_handler(conidx);

			if (app_ai_env.mtu[conidx] > 0)
			{
                ai_function_handle(CALLBACK_UPDATE_MTU, NULL, app_ai_env.mtu[conidx]);
			}
		}
        ai_function_handle(CALLBACK_AI_CONNECT, NULL, AI_TRANSPORT_BLE);
	}
    else
    {
#ifdef IS_MULTI_AI_ENABLED
        ai_manager_update_spec_table_connected_status_value(AI_SPEC_AMA, 0);
#endif
    }



    return (KE_MSG_CONSUMED);
}

static int app_ama_tx_data_sent_handler(ke_msg_id_t const msgid,
                              struct ble_ai_tx_sent_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    if (NULL != ble_tx_done_callback)
    {
		ble_tx_done_callback(NULL, 0);
    }

    return (KE_MSG_CONSUMED);
}

static int app_ama_rx_received_handler(ke_msg_id_t const msgid,
                              struct ble_ai_rx_data_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
	TRACE("%s ======>", __func__);
	DUMP8("%02x ",param->data, param->length);
    ai_function_handle(CALLBACK_CMD_RECEIVE, param->data, param->length);
    return (KE_MSG_CONSUMED);
}

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_ama_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,    (ke_msg_func_t)app_ama_msg_handler},
    {AMA_CCC_CHANGED,     		(ke_msg_func_t)app_ama_cmd_ccc_changed_handler},
    {AMA_TX_DATA_SENT,    		(ke_msg_func_t)app_ama_tx_data_sent_handler},//ama ble data sent callback
    {AMA_RECEIVED,   			(ke_msg_func_t)app_ama_rx_received_handler},
};

const struct ke_state_handler app_ama_table_handler =
    {&app_ama_msg_handler_list[0], (sizeof(app_ama_msg_handler_list)/sizeof(struct ke_msg_handler))};

AI_BLE_HANDLER_TO_ADD(AI_SPEC_AMA, &app_ama_table_handler)

#endif //__AMA_VOICE__

/// @} APP
