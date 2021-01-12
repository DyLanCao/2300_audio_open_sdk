/**************************************
-----------------filter.h---------------------
product name：fir filter
module name：hello
date：2020.12.04
auther：dylan 
file describe: fir_filter process head file 
***************************************/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GCC_PHAT__
#define __GCC_PHAT__

#include "plat_types.h"

void GccPhatTdoa(const float *data, int num_channel, int num_sample, int ref, int margin, int *tdoa);

void intToFloat( int16_t *input, float *output, int length );
void floatToInt( float *input, int16_t *output, int length );

#endif

#ifdef __cplusplus
}
#endif
