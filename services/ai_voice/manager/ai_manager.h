#ifndef __AI_MANAGER_H__
#define __AI_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAI_TYPE_REBOOT_WITHOUT_OEM_APP

//the ai type now use
#define AI_SPEC_INIT    0
#define AI_SPEC_GSOUND  1
#define AI_SPEC_AMA     2
#define AI_SPEC_BES     3
#define AI_SPEC_BAIDU   4
#define AI_SPEC_TENCENT 5
#define AI_SPEC_COUNT   6

#define DEFAULT_AI_SPEC AI_SPEC_INIT

typedef struct
{
    uint8_t     connectedStatus;        // 1:has connected      0: not connected
    uint8_t     assistantStatus;        // 1:assistant enable   0: assistant disable
} AI_MANAGER_STAT_T;


bool ai_voicekey_is_enable(void);
void ai_voicekey_save_status(bool state);
void save_ai_mode_ext_section(uint8_t currentAiSpec,uint8_t setedCurrentAi, uint8_t ai_disable_flag, uint8_t ama_assistant_status);
void init_ai_mode(void);
void ai_manager_init(uint8_t spec,uint8_t saved, uint8_t ai_disable_flag, uint8_t ama_assistant_status);
void ai_manager_switch_spec(uint8_t spec);
void ai_manager_set_current_spec(uint8_t currentAiSpec,uint8_t SetedCurrentAi);
uint8_t ai_manager_get_current_spec(void);
void ai_manager_enable(bool isEnabled, uint8_t currentAiSpec);
void ai_manager_update_spec_table_connected_status_value(uint8_t specIndex, uint8_t conValue);
void ai_manager_update_spec_table_assistant_status_value(uint8_t specIndex, uint8_t selValue);
int8_t ai_manager_get_spec_connected_status(uint8_t specIndex);
int8_t ai_manager_get_spec_assistant_status(uint8_t specIndex);
bool ai_manager_spec_get_status_is_in_invalid();
uint8_t ai_manager_get_reboot_ai_spec_value();
void ai_manager_set_ama_assistant_enable_status(uint8_t ama_assistant_value);
uint8_t ai_manager_get_ama_assistant_enable_status();
void ai_manager_info_save_before_reboot();
void ai_manager_spec_update_start_reboot();
#ifdef __cplusplus
}
#endif

#endif

