

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WL_NSX)

#define NSX_48K_BUFFER_SIZE (480)
#define NSX_44K_BUFFER_SIZE (440)

void WebRtcNsx_48k_denoise(short *i48k_buff, short *o48k_buff);
void WebRtcNsx_44k_denoise(short *i44k_buff, short *o44k_buff);
void wl_nsx_16k_denoise(short *i16k_buff, short *o16k_buff);
void wl_nsx_denoise_init(int nSample,int nMode, uint8_t* nsx_heap);
void nsx_resample_init(void);
int nsx_mempool_get_buff(uint16_t **buff, uint32_t size);
void wl_nsx_32k_denoise_process(const int16_t *i32k_buff, int16_t *o32k_buff);
void wl_nsx_exit(void);
#endif


#if defined(WL_AEC)
void WebRtc_aecm_init(int nSample,int nMode);
void WebRtc_aecm_process(short *near_frame, short *out_frame,int16_t frame_size);
void WebRtc_aecm_farend( const int16_t *farend, int16_t frame_size);
void wl_aec_exit(void);
#endif

#ifdef __cplusplus
}
#endif

