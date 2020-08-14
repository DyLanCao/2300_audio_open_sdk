/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_SMARTVOICE)
#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "gapc_task.h"
#include "sv.h"
#include "sv_task.h"
#include "prf_utils.h"
#include "ke_mem.h"
#include "co_utils.h"


/*
 * SMART VOICE CMD PROFILE ATTRIBUTES
 ****************************************************************************************
 */
#define sv_service_uuid_128_content  			{0x00,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}
#define sv_cmd_tx_char_val_uuid_128_content  	{0x01,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}	
#define sv_cmd_rx_char_val_uuid_128_content  	{0x02,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}
#define sv_data_tx_char_val_uuid_128_content  	{0x03,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}	
#define sv_data_rx_char_val_uuid_128_content  	{0x04,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}

#define ATT_DECL_PRIMARY_SERVICE_UUID		{ 0x00, 0x28 }
#define ATT_DECL_CHARACTERISTIC_UUID		{ 0x03, 0x28 }
#define ATT_DESC_CLIENT_CHAR_CFG_UUID		{ 0x02, 0x29 }

static const uint8_t SMARTVOICE_SERVICE_UUID_128[ATT_UUID_128_LEN]	= sv_service_uuid_128_content;  

/// Full SMARTVOICE SERVER Database Description - Used to add attributes into the database
const struct attm_desc_128 sv_att_db[SMARTVOICE_IDX_NB] =
{
    // Service Declaration
    [SMARTVOICE_IDX_SVC] 		=   {ATT_DECL_PRIMARY_SERVICE_UUID, PERM(RD, ENABLE), 0, 0},

    // Command TX Characteristic Declaration
    [SMARTVOICE_IDX_CMD_TX_CHAR] 	=   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
    // Command TX Characteristic Value
    [SMARTVOICE_IDX_CMD_TX_VAL]     =   {sv_cmd_tx_char_val_uuid_128_content, PERM(NTF, ENABLE) | PERM(IND, ENABLE) | PERM(RD, ENABLE), 
    	PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), SMARTVOICE_MAX_LEN},
    // Command TX Characteristic - Client Characteristic Configuration Descriptor
    [SMARTVOICE_IDX_CMD_TX_NTF_CFG] =   {ATT_DESC_CLIENT_CHAR_CFG_UUID, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

    // Command RX Characteristic Declaration
    [SMARTVOICE_IDX_CMD_RX_CHAR]    =   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
    // Command RX Characteristic Value
    [SMARTVOICE_IDX_CMD_RX_VAL]     =   {sv_cmd_rx_char_val_uuid_128_content, 
    	PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE), 
    	PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), SMARTVOICE_MAX_LEN},

    // Data TX Characteristic Declaration
    [SMARTVOICE_IDX_DATA_TX_CHAR] 	=   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
    // Data TX Characteristic Value
    [SMARTVOICE_IDX_DATA_TX_VAL]     =   {sv_data_tx_char_val_uuid_128_content, PERM(NTF, ENABLE) | PERM(IND, ENABLE) | PERM(RD, ENABLE), 
    	PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), SMARTVOICE_MAX_LEN},
    // Data TX Characteristic - Client Characteristic Configuration Descriptor
    [SMARTVOICE_IDX_DATA_TX_NTF_CFG] =   {ATT_DESC_CLIENT_CHAR_CFG_UUID, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

    // Data RX Characteristic Declaration
    [SMARTVOICE_IDX_DATA_RX_CHAR]    =   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
    // Data RX Characteristic Value
    [SMARTVOICE_IDX_DATA_RX_VAL]     =   {sv_data_rx_char_val_uuid_128_content, 
    	PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE), 
    	PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), SMARTVOICE_MAX_LEN},	

};

/**
 ****************************************************************************************
 * @brief Initialization of the SMARTVOICE module.
 * This function performs all the initializations of the Profile module.
 *  - Creation of database (if it's a service)
 *  - Allocation of profile required memory
 *  - Initialization of task descriptor to register application
 *      - Task State array
 *      - Number of tasks
 *      - Default task handler
 *
 * @param[out]    env        Collector or Service allocated environment data.
 * @param[in|out] start_hdl  Service start handle (0 - dynamically allocated), only applies for services.
 * @param[in]     app_task   Application task number.
 * @param[in]     sec_lvl    Security level (AUTH, EKS and MI field of @see enum attm_value_perm_mask)
 * @param[in]     param      Configuration parameters of profile collector or service (32 bits aligned)
 *
 * @return status code to know if profile initialization succeed or not.
 ****************************************************************************************
 */
static uint8_t sv_init(struct prf_task_env* env, uint16_t* start_hdl, 
	uint16_t app_task, uint8_t sec_lvl, void* params)
{
	uint8_t status;
		
    //Add Service Into Database
    status = attm_svc_create_db_128(start_hdl, SMARTVOICE_SERVICE_UUID_128, NULL,
            SMARTVOICE_IDX_NB, NULL, env->task, &sv_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS))| PERM(SVC_MI, DISABLE)
            | PERM_VAL(SVC_UUID_LEN, PERM_UUID_128));


	BLE_GATT_DBG("attm_svc_create_db_128 returns %d start handle is %d", status, *start_hdl);

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        // Allocate SMARTVOICE required environment variable
        struct sv_env_tag* sv_env =
                (struct sv_env_tag* ) ke_malloc(sizeof(struct sv_env_tag), KE_MEM_ATT_DB);

        // Initialize SMARTVOICE environment
        env->env           = (prf_env_t*) sv_env;
        sv_env->shdl     = *start_hdl;

        sv_env->prf_env.app_task    = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        // Mono Instantiated task
        sv_env->prf_env.prf_task    = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_VOICEPATH;
        sv_task_init(&(env->desc));

        /* Put HRS in Idle state */
        ke_state_set(env->task, SMARTVOICE_IDLE);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the SMARTVOICE module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void sv_destroy(struct prf_task_env* env)
{
    struct sv_env_tag* sv_env = (struct sv_env_tag*) env->env;

    // free profile environment variables
    env->env = NULL;
    ke_free(sv_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void sv_create(struct prf_task_env* env, uint8_t conidx)
{

}

/**
 ****************************************************************************************
 * @brief Handles Disconnection
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 * @param[in]        reason     Detach reason
 ****************************************************************************************
 */
static void sv_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    /* Nothing to do */
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// SMARTVOICE Task interface required by profile manager
const struct prf_task_cbs sv_itf =
{
    (prf_init_fnct) sv_init,
    sv_destroy,
    sv_create,
    sv_cleanup,
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* sv_prf_itf_get(void)
{
   return &sv_itf;
}


#endif /* BLE_SMARTVOICE */

/// @} SMARTVOICE

