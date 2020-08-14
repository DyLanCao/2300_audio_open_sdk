#ifndef _AI_GATT_SERVER_H_
#define _AI_GATT_SERVER_H_

/**
 ****************************************************************************************
 * @addtogroup SMARTVOICE Datapath Profile Server
 * @ingroup SMARTVOICEP
 * @brief Datapath Profile Server
 *
 * Datapath Profile Server provides functionalities to upper layer module
 * application. The device using this profile takes the role of Datapath Server.
 *
 * The interface of this role to the Application is:
 *  - Enable the profile role (from APP)
 *  - Disable the profile role (from APP)
 *  - Send data to peer device via notifications  (from APP)
 *  - Receive data from peer device via write no response (from APP)
 *
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#include <stdint.h>
#include "rwip_task.h" // Task definitions


#if (BLE_AI_VOICE)
#include "prf_types.h"
#include "prf.h"
#include "attm.h"
#include "prf_utils.h"


extern uint32_t __ai_gatt_server_table_start[];
extern uint32_t __ai_gatt_server_table_end[];

#define INVALID_AI_GATT_SERVER_ENTRY_INDEX 0xFFFF

/*
 * DEFINES
 ****************************************************************************************
 */
#define AI_MAX_LEN            			(509)

/*
 * AMA CMD PROFILE ATTRIBUTES
 ****************************************************************************************
 */

#define ATT_DECL_PRIMARY_SERVICE_UUID		{ 0x00, 0x28 }
#define ATT_DECL_CHARACTERISTIC_UUID		{ 0x03, 0x28 }
#define ATT_DESC_CLIENT_CHAR_CFG_UUID		{ 0x02, 0x29 }

/*
 * DEFINES
 ****************************************************************************************
 */
/// Possible states of the SMARTVOICE task
enum
{
    /// Idle state
    AI_IDLE,
    /// Connected state
    AI_BUSY,

    /// Number of defined states.
    AI_STATE_MAX,
};



/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Datapath Profile Server environment variable
struct ai_env_tag
{
    /// profile environment
    prf_env_t 	prf_env;
	uint8_t		isCmdNotificationEnabled[BLE_CONNECTION_MAX];
	uint8_t		isDataNotificationEnabled[BLE_CONNECTION_MAX];
    /// Service Start Handle
    uint16_t 	shdl;
    /// State of different task instances
    ke_state_t 	state;
};


#define AI_CONTENT_TYPE_COMMAND		0 
#define AI_CONTENT_TYPE_DATA		1

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct ble_ai_tx_notif_config_t
{
	bool 		isNotificationEnabled;
};

struct ble_ai_rx_data_ind_t
{
	uint16_t	length;
	uint8_t		data[0];
};

struct ble_ai_tx_sent_ind_t
{
	uint8_t 	status;
};

struct ble_ai_send_data_req_t
{
	uint8_t 	connecionIndex;
	uint32_t 	length;
	uint8_t  	value[__ARRAY_EMPTY];
};



/**
 * @brief AI voice gatt_server definition data structure
 *
 */
typedef struct
{
	uint32_t				    ai_code;
    const struct prf_task_cbs* 	ai_itf;  	/**< ai interface */
} AI_GATT_SERVER_INSTANCE_T;


#define AI_GATT_SERVER_TO_ADD(ai_code, ai_gatt_server)	\
	static AI_GATT_SERVER_INSTANCE_T ai_code##_gatt_server __attribute__((used, section(".ai_gatt_server_table"))) = 	\
		{(ai_code), (ai_gatt_server)};

#define AI_GATT_SERVER_PTR_FROM_ENTRY_INDEX(index)	\
	((AI_GATT_SERVER_INSTANCE_T *)((uint32_t)__ai_gatt_server_table_start + (index)*sizeof(AI_GATT_SERVER_INSTANCE_T)))



#endif /* #if (BLE_AI_VOICE) */



#endif

