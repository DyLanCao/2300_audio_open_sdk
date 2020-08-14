
#ifdef __cplusplus
extern "C" {
#endif

void WebRtcAgc_init(void);
void WebRtcAgc_Alg_Process(short *pData, short *pOutData);

#ifdef __cplusplus
}
#endif
