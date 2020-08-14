#ifndef __AI_CONTROL_H__
#define __AI_CONTROL_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"

#define ERROR_RETURN 0xFFFF
/// function for Smart Voice Profile 
 typedef enum sv_func_id_t
{
	API_HANDLE_INIT = 0,
    API_START_ADV,
    API_DATA_SEND,
    API_DATA_INIT,
    API_DATA_DEINIT,
    CALLBACK_CMD_RECEIVE,
    CALLBACK_DATA_RECEIVE,
    CALLBACK_DATA_PARSE,
    CALLBACK_AI_CONNECT,
    CALLBACK_AI_DISCONNECT,
    CALLBACK_UPDATE_MTU,
    CALLBACK_WAKE_UP,
    CALLBACK_AI_ENABLE,
	CALLBACK_START_SPEECH,
    CALLBACK_ENDPOINT_SPEECH,
	CALLBACK_STOP_SPEECH,
	CALLBACK_DATA_SEND_DONE,
} sv_func_id;

/// function Identifier. The number of function is limited to 0xFF.
/// The function ID parts:
/// bits[7~0]: function index(no more than 255 function per AI)
typedef uint8_t ai_func_id_t;

/// Format of a AI handler function
typedef uint32_t (*ai_handler_func_t)(void *param1, uint32_t param2);

/// Element of a AI handler table.
struct ai_func_handler
{
    /// Id of the handled function.
    ai_func_id_t id;
    /// Pointer to the handler function.
    ai_handler_func_t func;
};

/// Element of a state handler table.
struct ai_handler_func_table
{
    /// Pointer to the handler function table
    const struct ai_func_handler *func_table;
    /// Number of handled function
    uint16_t func_cnt;
};

uint32_t ai_function_handle(sv_func_id func_id, void *param1, uint32_t param2);
void ai_control_init(const struct ai_handler_func_table *func_table);

#ifdef __cplusplus
}
#endif

#endif

