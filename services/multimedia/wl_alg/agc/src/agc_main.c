#include "plat_types.h"
#include "hal_trace.h"
#include "stdio.h"

#include "analog_agc.h"


pAgc_t  pAgc = NULL;

//void *test = NULL;

void WebRtcAgc_init(void)
{
    int ret = 0;
    WebRtcAgc_Create((void*)&pAgc);
    int minLevel = 0;
    int maxLevel = 255;
    int agcMode  = kAgcModeAdaptiveDigital;
    WebRtcAgc_Init(pAgc, minLevel, maxLevel, agcMode, 16000);

    WebRtcAgc_config_t agcConfig;
#if 0
    agcConfig.compressionGaindB = 9;
    agcConfig.limiterEnable     = 1;
    agcConfig.targetLevelDbfs   = 6;
#else
    agcConfig.compressionGaindB = 15;
    agcConfig.limiterEnable     = 1;
    agcConfig.targetLevelDbfs   = 3;
#endif
    ret = wl_WebRtcAgc_set_config(pAgc, agcConfig);
    ASSERT(0 == ret, "failed in WebRtcAgc_init nAgcRet:%d ",ret);

    TRACE("webrtc agc init success");

}


void wl_agc_32k_init(void)
{
    int ret = 0;
    WebRtcAgc_Create((void*)&pAgc);
    int minLevel = 0;
    int maxLevel = 255;
    int agcMode  = kAgcModeAdaptiveDigital;
    WebRtcAgc_Init(pAgc, minLevel, maxLevel, agcMode, 32000);

    WebRtcAgc_config_t agcConfig;
#if 0
    agcConfig.compressionGaindB = 9;
    agcConfig.limiterEnable     = 1;
    agcConfig.targetLevelDbfs   = 6;
#else
    agcConfig.compressionGaindB = 15;
    agcConfig.limiterEnable     = 1;
    agcConfig.targetLevelDbfs   = 3;
#endif
    ret = wl_WebRtcAgc_set_config(pAgc, agcConfig);
    ASSERT(0 == ret, "failed in WebRtcAgc_init nAgcRet:%d ",ret);

    TRACE("webrtc agc init success");

}

int g_micLevelIn = 0;
int micLevelOut = 0;
#define FRAME_VALUE 160

void WebRtcAgc_Alg_Process(short *pData, short *pOutData)
{

    int inMicLevel  = g_micLevelIn;
    int32_t outMicLevel = 0;
    uint8_t saturationWarning;
    int nAgcRet = WebRtcAgc_Process(pAgc, pData, NULL, FRAME_VALUE, pOutData,NULL, inMicLevel, &outMicLevel, 0, &saturationWarning);
    ASSERT(0 == nAgcRet, "failed in WebRtcAgc_Process nAgcRet:%d ",nAgcRet);

    g_micLevelIn = outMicLevel;

}

void wl_agc_32k_alg_process(int16_t *in_near, int16_t *in_near_H, int16_t *out, int16_t *out_H)
{

    int inMicLevel  = g_micLevelIn;
    int32_t outMicLevel = 0;
    uint8_t saturationWarning;
    int nAgcRet = WebRtcAgc_Process(pAgc, in_near, in_near_H, 1*FRAME_VALUE, out,out_H, inMicLevel, &outMicLevel, 0, &saturationWarning);
    ASSERT(0 == nAgcRet, "failed in WebRtcAgc_Process nAgcRet:%d ",nAgcRet);

    g_micLevelIn = outMicLevel;

}



