/*
#if defined(WEBRTC_AECM) || defined(WEBRTC_NSX)

#include "noise_suppression_x.h"
#include "echo_control_mobile.h"
#include "signal_processing_library.h"
#include "hal_trace.h"
#include "ring_buffer.h"

#endif
*/

#if defined(WL_NSX)
#include "noise_suppression_x.h"
#include "plat_types.h"
#include "hal_trace.h"
#include "stdio.h"
#include "signal_processing_library.h"
#include "spl_inl.h"
#include "nsx_main.h"
#endif


#if defined(WEBRTC_AGC)
#include "agc_main.h"
#endif


#if defined(WEBRTC_AECM)
#include "ring_buffer.h"
#include "echo_control_mobile.h"
//#include "signal_processing_library.h"
#include "hal_trace.h"




void *aecmInst = NULL;
extern int aecm_crt;
extern int aecm_int;
extern int aecm_last;
void WebRtc_aecm_init(int nSample,int nMode)
{
    int ret_val;
    unsigned int count = 1;

	ret_val = WebRtcAecm_Create(&aecmInst);
	TRACE("retVal :%d aecm_crt:%d ",ret_val,aecm_crt);
	ret_val = WebRtcAecm_Init(aecmInst, nSample);
	TRACE("retVal :%d aecm_int:%d ",ret_val,aecm_int);

	AecmConfig config;
	config.cngMode = AecmTrue;
	config.echoMode = nMode;
	ret_val = WebRtcAecm_set_config(aecmInst, config);
	TRACE("retVal :%d aecm_last:%d ",ret_val,aecm_last);

}
#define  NFRAME 160
extern int aecm_cnt;

void WebRtc_aecm_process(short *near_frame, short *out_frame)
{

    int32_t retVal = 0;

    /* �������� 16k ������ �����ӳ��� 5-ms  �����ӳ���2-3 ms��������� 2-8֮�����*/
    retVal = WebRtcAecm_Process(aecmInst, near_frame, NULL, out_frame, NFRAME,2);//��������

    ASSERT(retVal == 0, "WebRtc_aecm_process failed aecm_cnt:%d ",aecm_cnt);
}


void WebRtc_aecm_farend( const int16_t *farend, int16_t nrOfSamples)
{

    int32_t retVal = 0;

    /* �������� 8k ������ �����ӳ��� 8ms */
    retVal = WebRtcAecm_BufferFarend(aecmInst, (short*)farend, NFRAME);//�Բο�����(����)�Ĵ���

    ASSERT(retVal == 0, "WebRtcAecm_BufferFarend failed retVal:%d ",retVal);
}


	


#endif

#if defined(WL_NSX)

#ifdef WEBRTC_AECM
#define NSX_AUDIO_BUFFER_SIZE (1024 * 19)
#else
#define NSX_AUDIO_BUFFER_SIZE (1024 * 1)
#endif

void* state1_;
void* state2_;
void* state3_;
void* state4_;
void* state3_48;
void* state4_48;

static int32_t array_1buff[8];//8 * sizeof(int32_t)
static int32_t array_2buff[8];//8 * sizeof(int32_t)
static int32_t array_3buff[24];//24 * sizeof(int32_t)
static int32_t array_4buff[16];//8 * sizeof(int32_t)

static int32_t array_3_48buff[32];//24 * sizeof(int32_t)

//static int32_t array_i22k_buff[104];//in_22k_size, sizeof(int32_t)
//static int32_t array_o22k_buff[88];//in_22k_size, sizeof(int32_t)


static uint16_t webrtc_audio_buff[NSX_AUDIO_BUFFER_SIZE];

static uint32_t nsx_buff_size_used;

int nsx_mempool_init()
{
    nsx_buff_size_used = 0;

    //if do not memset, memset in app level
    memset((uint8_t*)webrtc_audio_buff, 0, NSX_AUDIO_BUFFER_SIZE*sizeof(uint16_t));

    return 0;
}

uint32_t nsx_mempool_free_buff_size()
{
    return NSX_AUDIO_BUFFER_SIZE - nsx_buff_size_used;
}

int nsx_mempool_get_buff(uint16_t **buff, uint32_t size)
{
    uint32_t buff_size_free;

    buff_size_free = nsx_mempool_free_buff_size();

    ASSERT(size <= buff_size_free, "[%s] size = %d > free size = %d", __func__, size, buff_size_free);

    *buff = webrtc_audio_buff + nsx_buff_size_used;

    nsx_buff_size_used += size;

    return 0;
}


pNxsHandle ppNsxHandle = NULL;

void wl_nsx_denoise_init(int nSample,int nMode, uint8_t* nsx_heap)
{
    int ret_val;

    ret_val = wl_WebRtcNsx_Create(&ppNsxHandle, nsx_heap);
    ASSERT(0 == ret_val, "WebRtcNsx_Create failed");

    TRACE("WebRtcNs_denoise_initaaa:%d ",ret_val);

    ret_val = wl_WebRtcNsx_Init(ppNsxHandle,nSample);
    //ASSERT(0 == wl_WebRtcNsx_Init(ppNsxHandle,nSample), "WebRtcNsx_Init failed");

    TRACE("WebRtcNs_denoise_initbbb:%d ",ret_val);

    ret_val = wl_WebRtcNsx_set_policy(ppNsxHandle,nMode);
    ASSERT(0 == ret_val, "WebRtcNsx_set_policy failed");


    TRACE("WebRtcNs_denoise_initcccc:%d ",ret_val);

}
//��ʼ���ز�������
void nsx_resample_init(void)
{
    state1_ = (void*)array_1buff;
    state2_ = (void*)array_2buff;
    state3_ = (void*)array_3buff;
    state4_ = (void*)array_4buff;

    state3_48 = (void*)array_3_48buff;
    state4_48 = (void*)array_3buff;


    nsx_mempool_init();
}

void wl_nsx_Alg_Process(short *shBufferIn, short *shBufferOut)
{
    int ret_val;

    ret_val = wl_WebRtcNsx_Process(ppNsxHandle, shBufferIn, NULL, shBufferOut, NULL);
    //TRACE("nsx process rev val:%d ",ret_val);
    ASSERT(0 == ret_val, "failed in WebRtcNsx_Process :%d ",ret_val);

}

void WebRtcNsx_44k_denoise(short *i44k_buff, short *o44k_buff)
{
    short array_22k[220]; //tempSize_22k*sizeof(short)
    short array_i16k[160]; //tempSize_22k*sizeof(short)
    short array_o16k[160]; //tempSize_22k*sizeof(short)
    int tempSize_44k = 440;

#ifdef WEBRTC_AGC
    short array_agc_in[160]; // agc in
#endif

    int32_t array_i22k_buff[104];//in_22k_size, sizeof(int32_t)
    int32_t array_o22k_buff[88];//in_22k_size, sizeof(int32_t)

    WebRtcSpl_DownsampleBy2(i44k_buff, tempSize_44k, array_22k,(int32_t*)state1_);
#ifdef WEBRTC_AGC
    WebRtcSpl_Resample22khzTo16khz(array_22k,array_agc_in,(WebRtcSpl_State22khzTo16khz*)state3_,array_i22k_buff);
    WebRtcAgc_Alg_Process(array_agc_in,array_i16k);
    wl_nsx_Alg_Process(array_i16k,array_o16k);
#else
    WebRtcSpl_Resample22khzTo16khz(array_22k,array_i16k,(WebRtcSpl_State22khzTo16khz*)state3_,array_i22k_buff);

    wl_nsx_Alg_Process(array_i16k,array_o16k);
#endif
    WebRtcSpl_Resample16khzTo22khz(array_o16k,array_22k,(WebRtcSpl_State16khzTo22khz*)state4_,array_o22k_buff);
    WebRtcSpl_UpsampleBy2(array_22k, tempSize_44k/2, o44k_buff,(int32_t*)(state2_));
}

int32_t array_i48k_buff[496];//in_48k_size, sizeof(int32_t)
int32_t array_o48k_buff[336];//out_48k_size, sizeof(int32_t)

#ifdef WEBRTC_AGC
extern void WebRtcAgc_Alg_Process(short *pData, short *pOutData);
#endif

void WebRtcNsx_48k_denoise(short *i48k_buff, short *o48k_buff)
{
    short array_i16k[160]; //tempSize_22k*sizeof(short)
    short array_o16k[160]; //tempSize_22k*sizeof(short)
#ifdef WEBRTC_AGC
    short array_agc_in[160]; // agc in
#endif

#ifdef WEBRTC_AGC
    WebRtcSpl_Resample48khzTo16khz(i48k_buff,array_agc_in,(WebRtcSpl_State48khzTo16khz*)state3_48,array_i48k_buff);
    WebRtcAgc_Alg_Process(array_agc_in,array_i16k);
    wl_nsx_Alg_Process(array_i16k,array_o16k);
    WebRtcSpl_Resample16khzTo48khz(array_o16k,o48k_buff,(WebRtcSpl_State16khzTo48khz*)state4_48,array_o48k_buff);
#else
    WebRtcSpl_Resample48khzTo16khz(i48k_buff,array_i16k,(WebRtcSpl_State48khzTo16khz*)state3_48,array_i48k_buff);
    wl_nsx_Alg_Process(array_i16k,array_o16k);
    WebRtcSpl_Resample16khzTo48khz(array_o16k,o48k_buff,(WebRtcSpl_State16khzTo48khz*)state4_48,array_o48k_buff);
#endif
}

void wl_nsx_16k_denoise(short *i16k_buff, short *o16k_buff)
{

#ifdef WEBRTC_AGC
    short array_i16k[160]; //tempSize_22k*sizeof(short)
    //short array_o16k[160]; //tempSize_22k*sizeof(short)
	//WebRtcAgc_Alg_Process(i16k_buff,o16k_buff);
    //WebRtcSpl_Resample48khzTo16khz(i48k_buff,array_agc_in,(WebRtcSpl_State48khzTo16khz*)state3_48,array_i48k_buff);
    WebRtcAgc_Alg_Process(i16k_buff,array_i16k);
    wl_nsx_Alg_Process(array_i16k,o16k_buff);
    //WebRtcSpl_Resample16khzTo48khz(array_o16k,o48k_buff,(WebRtcSpl_State16khzTo48khz*)state4_48,array_o48k_buff);
#else
   // WebRtcSpl_Resample48khzTo16khz(i48k_buff,array_i16k,(WebRtcSpl_State48khzTo16khz*)state3_48,array_i48k_buff);
    wl_nsx_Alg_Process(i16k_buff,o16k_buff);
   // WebRtcSpl_Resample16khzTo48khz(array_o16k,o48k_buff,(WebRtcSpl_State16khzTo48khz*)state4_48,array_o48k_buff);
#endif
}

#endif


