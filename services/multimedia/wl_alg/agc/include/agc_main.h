
#ifdef __cplusplus
extern "C" {
#endif

void WebRtcAgc_init(void);
void WebRtcAgc_Alg_Process(short *pData, short *pOutData);

void wl_agc_32k_init(void);
void wl_agc_32k_alg_process(int16_t *in_near, int16_t *in_near_H, int16_t *out, int16_t *out_H);

#ifdef __cplusplus
}
#endif
