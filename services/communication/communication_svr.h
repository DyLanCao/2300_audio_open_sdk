#ifndef __COMMUNICATION_SVR_H__
#define __COMMUNICATION_SVR_H__

#ifdef __cplusplus
extern "C" {
#endif

void communication_init(void);
int communication_send_buf(uint8_t * buf, uint8_t len);
void uart_audio_init();
void uart_audio_deinit();

#ifdef __cplusplus
}
#endif

#endif

