#ifndef _AI_TRANSPORT_H_
#define _AI_TRANSPORT_H_

#include "cqueue.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TX_BLE_MTU_SIZE (20)
#define TX_SPP_MTU_SIZE (500)
#define TX_BUFF_SIZE (500) // for spp/ble, it is a medium value
#if VOB_ENCODING_ALGORITHM==ENCODING_ALGORITHM_SBC
#define AI_TRANSPORT_BUFF_FIFO_SIZE (4096)
#else
#if defined(CHIP_BEST1400) && defined(__AI_VOICE__)
#define AI_TRANSPORT_BUFF_FIFO_SIZE (1024)
#else
#define AI_TRANSPORT_BUFF_FIFO_SIZE (2048)
#endif
#endif
#define TRANSPORT_DATA_INDEX_MAX (40)

extern unsigned int transport_data_index_in;
extern unsigned int transport_data_index_out;
extern unsigned int transport_data_len[];
extern CQueue ai_stream_buf_queue;

void ai_transport_fifo_init(void);
void ai_transport_fifo_deinit(void);
void ai_data_transport_init(void  (* cb)(uint8_t*,uint32_t));
void ai_cmd_transport_init(void  (* cb)(uint8_t*,uint32_t));
void ai_audio_stream_allowed_to_send_set(bool isEnabled);
bool is_ai_audio_stream_allowed_to_send();
void updata_transfer_type();
void clean_transfer_type();
bool ai_transport_data_put(uint8_t *in_buf,uint32_t len);
bool ai_transport_handler(uint32_t len);
bool ai_cmd_transport(uint8_t *buf,uint32_t len);

void ai_ble_handler_init(uint32_t ai_spec);

#ifdef __cplusplus
}
#endif
#endif

