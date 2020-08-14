
#ifndef __APP_AMA_HANDLE_H__
#define __APP_AMA_HANDLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"

#define APP_AMA_SPEECH_STATE_TIMEOUT_INTERVEL           (90000)

/// Table of ama handlers function
extern const struct ai_handler_func_table ama_handler_function_table;

int32_t app_ama_event_notify(uint32_t msgID,uint32_t msgPtr, uint32_t param0, uint32_t param1);


#ifdef __cplusplus
}
#endif
#endif
