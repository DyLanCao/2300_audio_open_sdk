#ifndef _SMARTVOICE_H_
#define _SMARTVOICE_H_

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

#if (BLE_SMARTVOICE)
#include "prf_types.h"
#include "prf.h"
#include "sv_task.h"
#include "attm.h"
#include "prf_utils.h"


/*
 * DEFINES
 ****************************************************************************************
 */
#define SMARTVOICE_MAX_LEN            			(509)

/*
 * DEFINES
 ****************************************************************************************
 */
/// Possible states of the SMARTVOICE task
enum
{
    /// Idle state
    SMARTVOICE_IDLE,
    /// Connected state
    SMARTVOICE_BUSY,

    /// Number of defined states.
    SMARTVOICE_STATE_MAX,
};

///Attributes State Machine
enum
{
    SMARTVOICE_IDX_SVC,

    SMARTVOICE_IDX_CMD_TX_CHAR,
    SMARTVOICE_IDX_CMD_TX_VAL,
    SMARTVOICE_IDX_CMD_TX_NTF_CFG,

    SMARTVOICE_IDX_CMD_RX_CHAR,
    SMARTVOICE_IDX_CMD_RX_VAL,

    SMARTVOICE_IDX_DATA_TX_CHAR,
    SMARTVOICE_IDX_DATA_TX_VAL,
    SMARTVOICE_IDX_DATA_TX_NTF_CFG,

    SMARTVOICE_IDX_DATA_RX_CHAR,
    SMARTVOICE_IDX_DATA_RX_VAL,

    SMARTVOICE_IDX_NB,
};


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Datapath Profile Server environment variable
struct sv_env_tag
{
    /// profile environment
    prf_env_t 	prf_env;
	uint8_t		isCmdNotificationEnabled;
	uint8_t		isDataNotificationEnabled;
    /// Service Start Handle
    uint16_t 	shdl;
    /// State of different task instances
    ke_state_t 	state;
};


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve HRP service profile interface
 *
 * @return HRP service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* sv_prf_itf_get(void);


/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * Initialize task handler
 *
 * @param task_desc Task descriptor to fill
 ****************************************************************************************
 */
void sv_task_init(struct ke_task_desc *task_desc);



#endif /* #if (BLE_SMARTVOICE) */

/// @} SMARTVOICE

#endif /* _SMARTVOICE_H_ */

