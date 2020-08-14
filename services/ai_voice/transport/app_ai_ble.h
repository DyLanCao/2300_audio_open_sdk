#ifndef APP_AI_BLE_H_
#define APP_AI_BLE_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief Smart Voice Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_APP_AI_VOICE)

#include <stdint.h>          // Standard Integer Definition
#include "ke_task.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t __ai_ble_handler_table_start[];
extern uint32_t __ai_ble_handler_table_end[];

#define INVALID_AI_BLE_HANDLER_ENTRY_INDEX 0xFFFF

#define AI_BLE_CONNECTION_INTERVAL_MIN_IN_MS			10
#define AI_BLE_CONNECTION_INTERVAL_MAX_IN_MS			15
#define AI_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS		20000
#define AI_BLE_CONNECTION_SLAVELATENCY					0

/// ama application environment structure
struct app_ai_env_tag
{
    uint8_t connectionIndex;
	uint8_t	isCmdNotificationEnabled;
	uint8_t	isDataNotificationEnabled;
	uint16_t mtu[BLE_CONNECTION_MAX];
};

typedef uint32_t(*app_ai_tx_done_t)(void *param1, uint32_t param2);

typedef void(*app_ai_activity_stopped_t)(void);

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

/// Health Thermomter Application environment
extern struct app_ai_env_tag app_ai_env;
extern app_ai_tx_done_t ble_tx_done_callback;

/// Table of message handlers
extern const struct ke_state_handler *app_ai_table_handler;


/**
 * @brief AI voice ble_table_handler definition data structure
 *
 */
typedef struct
{
	uint32_t				    ai_code;
    const struct ke_state_handler* 	ai_table_handler;  	/**< ai ble handler table */
} AI_BLE_HANDLER_INSTANCE_T;


#define AI_BLE_HANDLER_TO_ADD(ai_code, ai_table_handler)	\
	static AI_BLE_HANDLER_INSTANCE_T ai_code##_ble_handler __attribute__((used, section(".ai_ble_handler_table"))) = 	\
		{(ai_code), (ai_table_handler)};

#define AI_BLE_HANDLER_PTR_FROM_ENTRY_INDEX(index)	\
	((AI_BLE_HANDLER_INSTANCE_T *)((uint32_t)__ai_ble_handler_table_start + (index)*sizeof(AI_BLE_HANDLER_INSTANCE_T)))

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialize DataPath Server Application
 ****************************************************************************************
 */
void app_ai_init(void);

/**
 ****************************************************************************************
 * @brief Add a DataPath Server instance in the DB
 ****************************************************************************************
 */
void app_ai_add_ai(void);

void app_ai_connected_evt_handler(uint8_t conidx);
void app_ai_disconnected_evt_handler(uint8_t conidx);
bool app_ai_ble_is_connected(void);
void app_ai_disconnect_ble(void);

void app_ai_register_ble_tx_done(app_ai_tx_done_t callback);

void app_ai_update_conn_parameter(uint8_t conidx, uint32_t min_interval_in_ms, uint32_t max_interval_in_ms,
		uint32_t supervision_timeout_in_ms, uint8_t slaveLantency);

void app_ai_mtu_exchanged_handler(uint8_t conidx, uint16_t MTU);

#ifdef __cplusplus
	}
#endif


#endif //(BLE_APP_AI_VOICE)

/// @} APP

#endif // APP_AI_BLE_H_
