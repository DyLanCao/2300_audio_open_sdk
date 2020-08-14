#ifndef APP_AMA_BLE_H_
#define APP_AMA_BLE_H_

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

#ifdef __AMA_VOICE__

#include <stdint.h>          // Standard Integer Definition

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 ****************************************************************************************
 * @brief Add a DataPath Server instance in the DB
 ****************************************************************************************
 */
void app_ama_send_via_notification(uint8_t* ptrData, uint32_t length);
void app_ama_send_via_indication(uint8_t* ptrData, uint32_t length);


#ifdef __cplusplus
	}
#endif


#endif //__AMA_VOICE__

/// @} APP

#endif // APP_AMA_VOICE_H_
