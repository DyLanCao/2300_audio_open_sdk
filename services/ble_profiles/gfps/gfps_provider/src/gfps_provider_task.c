/**
 ****************************************************************************************
 * @addtogroup GFPS STASK
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_GFPS_PROVIDER)
#include <string.h>

#include "co_utils.h"
#include "gap.h"
#include "gattc_task.h"
#include "gfps_provider.h"
//#include "crypto.h"
#include "gfps_provider_task.h"
#include "prf_utils.h"

#include "ke_mem.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GFPSP_SET_VALUE_REQ message.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gfpsp_set_value_req_handler(ke_msg_id_t const msgid,
                                      struct gfpsp_set_value_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Request status
    uint8_t status;
    // Characteristic Declaration attribute handle
    uint16_t handle;
    struct gfpsp_env_tag* gfpsp_env = PRF_ENV_GET(GFPSP, gfpsp);
    struct gfpsp_set_value_rsp* rsp;
    // Check Characteristic Code
    if (param->value < GFPSP_CHAR_MAX)
    {
        // Get Characteristic Declaration attribute handle
        handle = gfpsp_value_to_handle(gfpsp_env, param->value);

        // Check if the Characteristic exists in the database
        if (handle != ATT_INVALID_HDL)
        {
            // Check the value length
            status = gfpsp_check_val_len(param->value, param->length);

            if (status == GAP_ERR_NO_ERROR)
            {
                // Check value in already present in service
                struct gfpsp_val_elmt *val = (struct gfpsp_val_elmt *) co_list_pick(&(gfpsp_env->values));
                // loop until value found
                while(val != NULL)
                {
                    // if value already present, remove old one
                    if(val->value == param->value)
                    {
                        co_list_extract(&(gfpsp_env->values), &(val->hdr));
                        ke_free(val);
                        break;
                    }
                    val = (struct gfpsp_val_elmt *)val->hdr.next;
                }

                // allocate value data
                val = (struct gfpsp_val_elmt *) ke_malloc(sizeof(struct gfpsp_val_elmt) + param->length, KE_MEM_ATT_DB);
                val->value = param->value;
                val->length = param->length;
                memcpy(val->data, param->data, param->length);
                // insert value into the list
                co_list_push_back(&(gfpsp_env->values), &(val->hdr));
            }
        }
        else
        {
            status = PRF_ERR_INEXISTENT_HDL;
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    // send response to application
    rsp = KE_MSG_ALLOC(GFPSP_SET_VALUE_RSP, src_id, dest_id, gfpsp_set_value_rsp);
    rsp->value = param->value;
    rsp->status = status;
    ke_msg_send(rsp);


    return (KE_MSG_CONSUMED);
}


static void send_notifiction(uint8_t conidx, uint8_t contentType, const uint8_t* ptrData, uint32_t length)
{
    struct gfpsp_env_tag* gfpsp_env = PRF_ENV_GET(GFPSP, gfpsp);

    // Allocate the GATT notification message
    struct gattc_send_evt_cmd *report_ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&gfpsp_env->prf_env, conidx),
            gattc_send_evt_cmd, length);

    // Fill in the parameter structure
    report_ntf->operation = GATTC_NOTIFY;
    report_ntf->handle    = gfpsp_env->start_hdl + contentType;//GFPSP_IDX_KEY_BASED_PAIRING_VAL;//handle_offset;
    // pack measured value in database
    report_ntf->length    = length;
    memcpy(report_ntf->value, ptrData, length);
    // send notification to peer device
    ke_msg_send(report_ntf);
}

static void POSSIBLY_UNUSED send_indication(uint8_t conidx, uint8_t contentType, const uint8_t* ptrData, uint32_t length)
{
	uint16_t handle_offset = 0xFFFF;
    struct gfpsp_env_tag* gfpsp_env = PRF_ENV_GET(GFPSP, gfpsp);

    // Allocate the GATT notification message
    struct gattc_send_evt_cmd *report_ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&gfpsp_env->prf_env, conidx),
            gattc_send_evt_cmd, length);

    // Fill in the parameter structure
    report_ntf->operation = GATTC_INDICATE;
    report_ntf->handle    = gfpsp_env->start_hdl + handle_offset;
    // pack measured value in database
    report_ntf->length    = length;
    memcpy(report_ntf->value, ptrData, length);
    // send notification to peer device
    ke_msg_send(report_ntf);
}

__STATIC int send_data_via_notification_handler(ke_msg_id_t const msgid,
                                      struct gfpsp_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	send_notifiction(param->connecionIndex, GFPSP_IDX_KEY_BASED_PAIRING_VAL, param->value, param->length);
	return (KE_MSG_CONSUMED);
}
__STATIC int send_passkey_data_via_notification_handler(ke_msg_id_t const msgid,
                                      struct gfpsp_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	send_notifiction(param->connecionIndex, GFPSP_IDX_PASSKEY_VAL, param->value, param->length);
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
    int msg_status = KE_MSG_CONSUMED;
    ke_state_t state = ke_state_get(dest_id); 

    if(state == GFPSP_IDLE)
    {
        struct gfpsp_env_tag* gfpsp_env = PRF_ENV_GET(GFPSP, gfpsp);
        // retrieve value attribute
        uint8_t value = gfpsp_handle_to_value(gfpsp_env, param->handle);

        // Check Characteristic Code
        if (value < GFPSP_CHAR_MAX)
        {
            // Check value in already present in service
            struct gfpsp_val_elmt *val = (struct gfpsp_val_elmt *) co_list_pick(&(gfpsp_env->values));
            // loop until value found
            while(val != NULL)
            {
                // value is present in service
                if(val->value == value)
                {
                    break;
                }
                val = (struct gfpsp_val_elmt *)val->hdr.next;
            }

            if(val != NULL)
            {
                // Send value to peer device.
                struct gattc_read_cfm* cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, val->length);
                cfm->handle = param->handle;
                cfm->status = ATT_ERR_NO_ERROR;
                cfm->length = val->length;
                memcpy(cfm->value, val->data, val->length);
                ke_msg_send(cfm);
            }
            else
            {
                // request value to application
                gfpsp_env->req_val    = value;
                gfpsp_env->req_conidx = KE_IDX_GET(src_id);

                struct gfpsp_value_req_ind* req_ind = KE_MSG_ALLOC(GFPSP_VALUE_REQ_IND,
                        prf_dst_task_get(&(gfpsp_env->prf_env), KE_IDX_GET(src_id)),
                        dest_id, gfpsp_value_req_ind);
                req_ind->value = value;
                ke_msg_send(req_ind);

                // Put Service in a busy state
                ke_state_set(dest_id, GFPSP_BUSY);
            }
        }
        else
        {
            // application error, value cannot be retrieved
            struct gattc_read_cfm* cfm = KE_MSG_ALLOC(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm);
            cfm->handle = param->handle;
            cfm->status = ATT_ERR_APP_ERROR;
            ke_msg_send(cfm);
        }
    }
    // postpone request if profile is in a busy state - required for multipoint
    else if(state == GFPSP_BUSY)
    {
        msg_status = KE_MSG_SAVED;
    }

    return (msg_status);
}

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of write request message.
 * The handler will analyse what has been set and decide alert level
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
 uint8_t Encrypted_req[80]={
	0xb9,0x7b,0x2c,0x4b,0x84,0x59,0x3e,0x69,0xf9,0xc5,
	0x1c,0x6f,0xf9,0xba,0x7e,0xc0,0x27,0xa6,0x13,0x55,
	0x26,0x84,0xbc,0xa2,0xd8,0x95,0xd6,0xf8,0xdd,0x5e,
	0xb5,0x91,0xfe,0xf7,0x31,0x1c,0x19,0x3e,0x38,0x8e,
	0x5f,0x3a,0xe6,0x6b,0x68,0x46,0xc4,0x14,0x1c,0x03,
	0xcb,0xc3,0x18,0x06,0x6b,0x52,0xd9,0x5c,0xa0,0xa2,
	0xb5,0x80,0xd9,0x90,0x4b,0xed,0x46,0x23,0x22,0x9b,
	0x42,0xe7,0xc2,0xde,0x2e,0x2a,0xba,0x7c,0xac,0x2b
};

__STATIC int gattc_write_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_write_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{

    int msg_status = KE_MSG_CONSUMED;
    
    // Get the address of the environment
    struct gfpsp_env_tag* gfpsp_env = PRF_ENV_GET(GFPSP, gfpsp);
    uint8_t conidx = KE_IDX_GET(src_id);

    uint8_t status = GAP_ERR_NO_ERROR;

    //Send write response
    struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
            GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
    cfm->handle = param->handle;
    cfm->status = status;
    ke_msg_send(cfm);
	
    if (gfpsp_env != NULL)
    {
        switch (param->handle-gfpsp_env->start_hdl)
        {
            case GFPSP_IDX_KEY_BASED_PAIRING_VAL:
            {
    			//inform APP of data
                struct gfpsp_write_ind_t * ind = KE_MSG_ALLOC_DYN(GFPSP_KEY_BASSED_PAIRING_WRITE_IND,
                        prf_dst_task_get(&gfpsp_env->prf_env, conidx),
                        prf_src_task_get(&gfpsp_env->prf_env, conidx),
                        gfpsp_write_ind_t,
                        param->length);
    			ind->length = param->length; 
    			memcpy((uint8_t *)(ind->data),&(param->value), param->length);

                ke_msg_send(ind);	
				break;
            }
            case GFPSP_IDX_KEY_BASED_PAIRING_NTF_CFG:
			{
				uint16_t value = 0x0000;			
            	memcpy(&value, &(param->value), sizeof(uint16_t));
                struct app_gfps_key_based_notif_config_t * ind = KE_MSG_ALLOC(GFPSP_KEY_BASSED_PAIRING_NTF_CFG,
                        prf_dst_task_get(&gfpsp_env->prf_env, conidx),
                        prf_src_task_get(&gfpsp_env->prf_env, conidx),
                        app_gfps_key_based_notif_config_t);
                ind->isNotificationEnabled = value;
                ke_msg_send(ind);
                break;
            }
            case GFPSP_IDX_PASSKEY_NTF_CFG:
			{
                uint16_t value = 0x0000;			            	memcpy(&value, &(param->value), sizeof(uint16_t));
                struct app_gfps_pass_key_notif_config_t * ind = KE_MSG_ALLOC(GFPSP_KEY_PASS_KEY_NTF_CFG,
                        prf_dst_task_get(&gfpsp_env->prf_env, conidx),
                        prf_src_task_get(&gfpsp_env->prf_env, conidx),
                        app_gfps_pass_key_notif_config_t);

                ind->isNotificationEnabled = value;
                ke_msg_send(ind);
            	break;  
            }
            case GFPSP_IDX_PASSKEY_VAL:
            {
                //inform APP of data
                struct gfpsp_write_ind_t * ind = KE_MSG_ALLOC_DYN(GFPSP_KEY_PASS_KEY_WRITE_IND,
                        prf_dst_task_get(&gfpsp_env->prf_env, conidx),
                        prf_src_task_get(&gfpsp_env->prf_env, conidx),
                        gfpsp_write_ind_t,
                        param->length);
                ind->length = param->length; 
                memcpy((uint8_t *)(ind->data),&(param->value), param->length);
    
                ke_msg_send(ind); 
				break;
            }
			case GFPSP_IDX_ACCOUNT_KEY_VAL:
			{
				TRACE("Get account key:");
				DUMP8("%02x ", param->value, param->length);
				struct gfpsp_write_ind_t * ind = KE_MSG_ALLOC_DYN(GFPSP_KEY_ACCOUNT_KEY_WRITE_IND,
                        prf_dst_task_get(&gfpsp_env->prf_env, conidx),
                        prf_src_task_get(&gfpsp_env->prf_env, conidx),
                        gfpsp_write_ind_t,
                        param->length);
                ind->length = param->length; 
                memcpy((uint8_t *)(ind->data),&(param->value), param->length);
    
                ke_msg_send(ind);				
				break;
			}
            default:
			{
            	break;
            }
        }
    }

    if(ke_state_get(dest_id) == GFPSP_IDLE)
    {

    }
    else if(ke_state_get(dest_id) == GFPSP_BUSY)
    {
        msg_status = KE_MSG_SAVED;
    }

    return (msg_status);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the value confirmation from application
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gfpsp_value_cfm_handler(ke_msg_id_t const msgid,
                                      struct gfpsp_value_cfm const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    ke_state_t state = ke_state_get(dest_id);

    if(state == GFPSP_BUSY)
    {
        struct gfpsp_env_tag* gfpsp_env = PRF_ENV_GET(GFPSP, gfpsp);
        // retrieve value attribute
        uint16_t handle = gfpsp_value_to_handle(gfpsp_env, gfpsp_env->req_val);

        // chack if application provide correct value
        if (gfpsp_env->req_val == param->value)
        {
            // Send value to peer device.
            struct gattc_read_cfm* cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, KE_BUILD_ID(TASK_GATTC, gfpsp_env->req_conidx), dest_id, gattc_read_cfm, param->length);
            cfm->handle = handle;
            cfm->status = ATT_ERR_NO_ERROR;
            cfm->length = param->length;
            memcpy(cfm->value, param->data, param->length);
            ke_msg_send(cfm);
        }
        else
        {
            // application error, value provided by application is not the expected one
            struct gattc_read_cfm* cfm = KE_MSG_ALLOC(GATTC_READ_CFM, KE_BUILD_ID(TASK_GATTC, gfpsp_env->req_conidx), dest_id, gattc_read_cfm);
            cfm->handle = handle;
            cfm->status = ATT_ERR_APP_ERROR;
            ke_msg_send(cfm);
        }

        // return to idle state
        ke_state_set(dest_id, GFPSP_IDLE);
    }
    // else ignore request if not in busy state

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
//
/// Default State handlers definition
KE_MSG_HANDLER_TAB(gfpsp)
{
    {GFPSP_SET_VALUE_REQ,      	(ke_msg_func_t)gfpsp_set_value_req_handler},
    {GATTC_READ_REQ_IND,      	(ke_msg_func_t)gattc_read_req_ind_handler},
    {GATTC_WRITE_REQ_IND,    	(ke_msg_func_t) gattc_write_req_ind_handler},
    {GFPSP_VALUE_CFM,          	(ke_msg_func_t)gfpsp_value_cfm_handler},
    {GFPSP_KEY_BASSED_PAIRING_WRITE_NOTIFY,	(ke_msg_func_t)send_data_via_notification_handler},
    {GFPSP_KEY_PASS_KEY_WRITE_NOTIFY,		(ke_msg_func_t)send_passkey_data_via_notification_handler},
};

void gfpsp_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct gfpsp_env_tag *gfpsp_env = PRF_ENV_GET(GFPSP, gfpsp);

    task_desc->msg_handler_tab = gfpsp_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(gfpsp_msg_handler_tab);
    task_desc->state           = gfpsp_env->state;
    task_desc->idx_max         = GFPSP_IDX_MAX;
}

#endif //BLE_GFPS_SERVER

/// @} DISSTASK
