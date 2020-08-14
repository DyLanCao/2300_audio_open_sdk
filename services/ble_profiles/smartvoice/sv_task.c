/**
 ****************************************************************************************
 * @addtogroup SMARTVOICETASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_SMARTVOICE)
#include "gap.h"
#include "gattc_task.h"
#include "gapc_task.h"
#include "attm.h"
#include "sv.h"
#include "sv_task.h"

#include "prf_utils.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
									  struct gapc_disconnect_ind const *param,
									  ke_task_id_t const dest_id,
									  ke_task_id_t const src_id)
{
	struct sv_env_tag *sv_env = PRF_ENV_GET(SMARTVOICE, sv);
	sv_env->isCmdNotificationEnabled = false;
	sv_env->isDataNotificationEnabled = false;
	return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GL2C_CODE_ATT_WR_CMD_IND message.
 * The handler compares the new values with current ones and notifies them if they changed.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_write_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_write_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{	
    // Get the address of the environment
    struct sv_env_tag *sv_env = PRF_ENV_GET(SMARTVOICE, sv);
    uint8_t conidx = KE_IDX_GET(src_id);

    uint8_t status = GAP_ERR_NO_ERROR;

	BLE_GATT_DBG("gattc_write_req_ind_handler sv_env 0x%x write handle %d shdl %d", 
		sv_env, param->handle, sv_env->shdl);

    //Send write response
    struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
            GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
    cfm->handle = param->handle;
    cfm->status = status;
    ke_msg_send(cfm);

    if (sv_env != NULL)
    {
		if (param->handle == (sv_env->shdl + SMARTVOICE_IDX_CMD_TX_NTF_CFG))
		{
			uint16_t value = 0x0000;
			
            //Extract value before check
            memcpy(&value, &(param->value), sizeof(uint16_t));

            if (value == PRF_CLI_STOP_NTFIND)
            {
                sv_env->isCmdNotificationEnabled = false;
            }
			else if (value == PRF_CLI_START_NTF)
			{
				sv_env->isCmdNotificationEnabled = true;
			}
            else
            {
                status = PRF_APP_ERROR;
            }

            if (status == GAP_ERR_NO_ERROR)
            {
                //Inform APP of TX ccc change
                struct ble_sv_tx_notif_config_t * ind = KE_MSG_ALLOC(SMARTVOICE_CMD_CCC_CHANGED,
                        prf_dst_task_get(&sv_env->prf_env, conidx),
                        prf_src_task_get(&sv_env->prf_env, conidx),
                        ble_sv_tx_notif_config_t);

                ind->isNotificationEnabled = sv_env->isCmdNotificationEnabled;

                ke_msg_send(ind);
            }
		}
		else if (param->handle == (sv_env->shdl + SMARTVOICE_IDX_DATA_TX_NTF_CFG))
		{
			uint16_t value = 0x0000;
			
            //Extract value before check
            memcpy(&value, &(param->value), sizeof(uint16_t));

            if (value == PRF_CLI_STOP_NTFIND)
            {
                sv_env->isDataNotificationEnabled = false;
            }
			else if (value == PRF_CLI_START_NTF)
			{
				sv_env->isDataNotificationEnabled = true;
			}
            else
            {
                status = PRF_APP_ERROR;
            }

            if (status == GAP_ERR_NO_ERROR)
            {
                //Inform APP of TX ccc change
                struct ble_sv_tx_notif_config_t * ind = KE_MSG_ALLOC(SMARTVOICE_DATA_CCC_CHANGED,
                        prf_dst_task_get(&sv_env->prf_env, conidx),
                        prf_src_task_get(&sv_env->prf_env, conidx),
                        ble_sv_tx_notif_config_t);

                ind->isNotificationEnabled = sv_env->isDataNotificationEnabled;

                ke_msg_send(ind);
            }			
		}
		else if (param->handle == (sv_env->shdl + SMARTVOICE_IDX_CMD_RX_VAL))
		{
			//inform APP of data
            struct ble_sv_rx_data_ind_t * ind = KE_MSG_ALLOC_DYN(SMARTVOICE_CMD_RECEIVED,
                    prf_dst_task_get(&sv_env->prf_env, conidx),
                    prf_src_task_get(&sv_env->prf_env, conidx),
                    ble_sv_rx_data_ind_t,
                    sizeof(struct ble_sv_rx_data_ind_t) + param->length);

			ind->length = param->length;
			memcpy((uint8_t *)(ind->data), &(param->value), param->length);

            ke_msg_send(ind);	
		}
		else if (param->handle == (sv_env->shdl + SMARTVOICE_IDX_DATA_RX_VAL))
		{
			//inform APP of data
            struct ble_sv_rx_data_ind_t * ind = KE_MSG_ALLOC_DYN(SMARTVOICE_DATA_RECEIVED,
                    prf_dst_task_get(&sv_env->prf_env, conidx),
                    prf_src_task_get(&sv_env->prf_env, conidx),
                    ble_sv_rx_data_ind_t,
                    sizeof(struct ble_sv_rx_data_ind_t) + param->length);

			ind->length = param->length;
			memcpy((uint8_t *)(ind->data), &(param->value), param->length);

            ke_msg_send(ind);	
		}
		else
		{
			status = PRF_APP_ERROR;
		}
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles @ref GATTC_CMP_EVT for GATTC_NOTIFY message meaning that Measurement
 * notification has been correctly sent to peer device (but not confirmed by peer device).
 * *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_cmp_evt_handler(ke_msg_id_t const msgid,  struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
	// Get the address of the environment
    struct sv_env_tag *sv_env = PRF_ENV_GET(SMARTVOICE, sv);

	uint8_t conidx = KE_IDX_GET(dest_id);
		
	// notification or write request has been sent out
    if ((GATTC_NOTIFY == param->operation) || (GATTC_INDICATE == param->operation))
    {
		
        struct ble_sv_tx_sent_ind_t * ind = KE_MSG_ALLOC(SMARTVOICE_TX_DATA_SENT,
                prf_dst_task_get(&sv_env->prf_env, conidx),
                prf_src_task_get(&sv_env->prf_env, conidx),
                ble_sv_tx_sent_ind_t);

		ind->status = param->status;

        ke_msg_send(ind);

    }

	ke_state_set(dest_id, SMARTVOICE_IDLE);
	
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the read request from peer device
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_read_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_read_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{	
    // Get the address of the environment
    struct sv_env_tag *sv_env = PRF_ENV_GET(SMARTVOICE, sv);

	struct gattc_read_cfm* cfm;

	uint8_t status = GAP_ERR_NO_ERROR;

	BLE_GATT_DBG("gattc_read_req_ind_handler read handle %d shdl %d", param->handle, sv_env->shdl);

    if (param->handle == (sv_env->shdl + SMARTVOICE_IDX_CMD_TX_NTF_CFG))
	{
		uint16_t notify_ccc;
		cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, sizeof(notify_ccc));
		
		if (sv_env->isCmdNotificationEnabled)
		{
			notify_ccc = 1;
		}
		else
		{
			notify_ccc = 0;
		}
		cfm->length = sizeof(notify_ccc);
		memcpy(cfm->value, (uint8_t *)&notify_ccc, cfm->length);
	}
	if (param->handle == (sv_env->shdl + SMARTVOICE_IDX_DATA_TX_NTF_CFG))
	{
		uint16_t notify_ccc;
		cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, sizeof(notify_ccc));
		
		if (sv_env->isDataNotificationEnabled)
		{
			notify_ccc = 1;
		}
		else
		{
			notify_ccc = 0;
		}
		cfm->length = sizeof(notify_ccc);
		memcpy(cfm->value, (uint8_t *)&notify_ccc, cfm->length);
	}
	else
	{
		cfm = KE_MSG_ALLOC(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm);
		cfm->length = 0;
		status = ATT_ERR_REQUEST_NOT_SUPPORTED;
	}

	cfm->handle = param->handle;

	cfm->status = status;

	ke_msg_send(cfm);

	ke_state_set(dest_id, SMARTVOICE_IDLE);

    return (KE_MSG_CONSUMED);
}

static void send_notifiction(uint8_t conidx, uint8_t contentType, const uint8_t* ptrData, uint32_t length)
{
	uint16_t handle_offset = 0xFFFF;
	struct sv_env_tag *sv_env = PRF_ENV_GET(SMARTVOICE, sv);

	if ((SMARTVOICE_CONTENT_TYPE_COMMAND == contentType) && 
		(sv_env->isCmdNotificationEnabled))
	{
		handle_offset = SMARTVOICE_IDX_CMD_TX_VAL;
	}
	else if ((SMARTVOICE_CONTENT_TYPE_DATA == contentType) && 
		(sv_env->isDataNotificationEnabled))
	{
		handle_offset = SMARTVOICE_IDX_DATA_TX_VAL;
	}

	if (0xFFFF != handle_offset)
	{
        // Allocate the GATT notification message
        struct gattc_send_evt_cmd *report_ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&sv_env->prf_env, conidx),
                gattc_send_evt_cmd, length);

        // Fill in the parameter structure
        report_ntf->operation = GATTC_NOTIFY;
        report_ntf->handle    = sv_env->shdl + handle_offset;
        // pack measured value in database
        report_ntf->length    = length;
        memcpy(report_ntf->value, ptrData, length);
        // send notification to peer device
        ke_msg_send(report_ntf);
	}
}

static void send_indication(uint8_t conidx, uint8_t contentType, const uint8_t* ptrData, uint32_t length)
{
	uint16_t handle_offset = 0xFFFF;
	struct sv_env_tag *sv_env = PRF_ENV_GET(SMARTVOICE, sv);

	if ((SMARTVOICE_CONTENT_TYPE_COMMAND == contentType) && 
		(sv_env->isCmdNotificationEnabled))
	{
		handle_offset = SMARTVOICE_IDX_CMD_TX_VAL;
	}
	else if ((SMARTVOICE_CONTENT_TYPE_DATA == contentType) && 
		(sv_env->isDataNotificationEnabled))
	{
		handle_offset = SMARTVOICE_IDX_DATA_TX_VAL;
	}

	if (0xFFFF != handle_offset)
	{
        // Allocate the GATT notification message
        struct gattc_send_evt_cmd *report_ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&sv_env->prf_env, conidx),
                gattc_send_evt_cmd, length);

        // Fill in the parameter structure
        report_ntf->operation = GATTC_INDICATE;
        report_ntf->handle    = sv_env->shdl + handle_offset;
        // pack measured value in database
        report_ntf->length    = length;
        memcpy(report_ntf->value, ptrData, length);
        // send notification to peer device
        ke_msg_send(report_ntf);
	}
}

__STATIC int send_cmd_via_notification_handler(ke_msg_id_t const msgid,
                                      struct ble_sv_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	send_notifiction(param->connecionIndex, SMARTVOICE_CONTENT_TYPE_COMMAND, param->value, param->length);
	return (KE_MSG_CONSUMED);
}

__STATIC int send_cmd_via_indication_handler(ke_msg_id_t const msgid,
                                      struct ble_sv_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	send_indication(param->connecionIndex, SMARTVOICE_CONTENT_TYPE_COMMAND, param->value, param->length);
	return (KE_MSG_CONSUMED);
}

__STATIC int send_data_via_notification_handler(ke_msg_id_t const msgid,
                                      struct ble_sv_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	send_notifiction(param->connecionIndex, SMARTVOICE_CONTENT_TYPE_DATA, param->value, param->length);
	return (KE_MSG_CONSUMED);
}

__STATIC int send_data_via_indication_handler(ke_msg_id_t const msgid,
                                      struct ble_sv_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	send_indication(param->connecionIndex, SMARTVOICE_CONTENT_TYPE_DATA, param->value, param->length);
	return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the attribute info request message.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gattc_att_info_req_ind *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct gattc_att_info_cfm * cfm;

    //Send write response
    cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
    cfm->handle = param->handle;

	struct sv_env_tag *sv_env = PRF_ENV_GET(SMARTVOICE, sv);

	if ((param->handle == (sv_env->shdl + SMARTVOICE_IDX_CMD_TX_NTF_CFG)) ||
		(param->handle == (sv_env->shdl + SMARTVOICE_IDX_DATA_TX_NTF_CFG)))
	{
		// CCC attribute length = 2
        cfm->length = 2;
        cfm->status = GAP_ERR_NO_ERROR;
	}
	else if ((param->handle == (sv_env->shdl + SMARTVOICE_IDX_CMD_RX_VAL)) ||
		(param->handle == (sv_env->shdl + SMARTVOICE_IDX_DATA_RX_VAL)))
	{
        // force length to zero to reject any write starting from something != 0
        cfm->length = 0;
        cfm->status = GAP_ERR_NO_ERROR;			
	}
	else
	{
		cfm->length = 0;
    	cfm->status = ATT_ERR_WRITE_NOT_PERMITTED;
	}

    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/* Default State handlers definition. */
KE_MSG_HANDLER_TAB(sv)
{
	{GAPC_DISCONNECT_IND,		(ke_msg_func_t) gapc_disconnect_ind_handler},
    {GATTC_WRITE_REQ_IND,    	(ke_msg_func_t) gattc_write_req_ind_handler},
    {GATTC_CMP_EVT,          	(ke_msg_func_t) gattc_cmp_evt_handler},
    {GATTC_READ_REQ_IND,     	(ke_msg_func_t) gattc_read_req_ind_handler},
    {SMARTVOICE_SEND_CMD_VIA_NOTIFICATION,   	(ke_msg_func_t) send_cmd_via_notification_handler},
	{SMARTVOICE_SEND_CMD_VIA_INDICATION,   		(ke_msg_func_t) send_cmd_via_indication_handler},		
    {SMARTVOICE_SEND_DATA_VIA_NOTIFICATION,   	(ke_msg_func_t) send_data_via_notification_handler},
	{SMARTVOICE_SEND_DATA_VIA_INDICATION,   	(ke_msg_func_t) send_data_via_indication_handler},
	{GATTC_ATT_INFO_REQ_IND,                    (ke_msg_func_t) gattc_att_info_req_ind_handler },
};

void sv_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct sv_env_tag *sv_env = PRF_ENV_GET(SMARTVOICE, sv);

    task_desc->msg_handler_tab = sv_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(sv_msg_handler_tab);
    task_desc->state           = &(sv_env->state);
    task_desc->idx_max         = 1;
}

#endif /* #if (BLE_SMARTVOICE) */

/// @} SMARTVOICETASK
