#ifdef __cplusplus
extern "C" {
#endif

#ifndef __AUTOWAH_H__
#define __AUTOWAH_H__

void AutoWah_init(short effect_rate,short sampling,short maxf,short minf,short Q,float gainfactor,short freq_step);
float AutoWah_process(int xin);
void AutoWah_sweep(void);

#endif

#ifdef __cplusplus
}
#endif