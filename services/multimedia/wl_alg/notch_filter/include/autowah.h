#ifdef __cplusplus
extern "C" {
#endif

#ifndef __AUTOWAH_H__
#define __AUTOWAH_H__

void notch_filter_init(void);
double notch_filter_process(double data);
void notch_filter_exit(void);

#endif

#ifdef __cplusplus
}
#endif